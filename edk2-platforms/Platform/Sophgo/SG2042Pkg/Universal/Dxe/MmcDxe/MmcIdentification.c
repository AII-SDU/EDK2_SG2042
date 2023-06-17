/*
 * Copyright (c) 2018-2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Define a simple and generic interface to access eMMC and SD-card devices. */

#include <Include/SG2042Mmc.h>
#include "Mmc.h"

#define MMC_DEFAULT_MAX_RETRIES		5
#define SEND_OP_COND_MAX_RETRIES	100

#define MULT_BY_512K_SHIFT		19

STATIC UINT32 mmc_ocr_value;
STATIC struct mmc_csd_emmc mmc_csd;
STATIC UINT8 mmc_ext_csd[512] __attribute__ ((aligned(16)));
STATIC struct mmc_device_info mmc_dev_info = {
	.mmc_dev_type = MMC_IS_SD_HC,
	.ocr_voltage = 0x00300000, // OCR 3.2~3.3 3.3~3.4
};
STATIC UINT32 rca;
STATIC UINT32 scr[2] __attribute__ ((aligned(16))) = { 0 };

STATIC CONST UINT8 tran_speed_base[16] = {
	0, 10, 12, 13, 15, 20, 26, 30, 35, 40, 45, 52, 55, 60, 70, 80
};

STATIC CONST UINT8 sd_tran_speed_base[16] = {
	0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80
};

STATIC 
EFI_STATUS 
mmc_device_state(
	IN MMC_HOST_INSTANCE     *MmcHostInstance,
	IN UINT32 *State
)
{
	INT32 retries = MMC_DEFAULT_MAX_RETRIES;
	UINT32 resp_data[4];

	do {
		EFI_STATUS Status;

		if (retries == 0) {
			// pr_err("CMD13 failed after %d retries\n",
			//       MMC_DEFAULT_MAX_RETRIES);
			DEBUG ((DEBUG_ERROR, "%a: CMD13 failed after %d retries\n", __FUNCTION__, MMC_DEFAULT_MAX_RETRIES));
			return EFI_DEVICE_ERROR;
		}

		Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(13), rca << RCA_SHIFT_OFFSET,
				   MMC_RESPONSE_R1, &resp_data[0]);
		if (EFI_ERROR (Status)) {
			retries--;
			continue;
		}

		if ((resp_data[0] & STATUS_SWITCH_ERROR) != 0U) {
			return EFI_DEVICE_ERROR;
		}

		retries--;
	} while ((resp_data[0] & STATUS_READY_FOR_DATA) == 0U);

    DEBUG ((DEBUG_ERROR, "%a: sd state %x\n", __FUNCTION__, MMC_GET_STATE(resp_data[0])));
	*State = MMC_GET_STATE(resp_data[0]);
	return EFI_SUCCESS;
}

STATIC 
EFI_STATUS 
mmc_set_ext_csd(
	IN MMC_HOST_INSTANCE     *MmcHostInstance,
	IN UINT32 ext_cmd, 
	IN UINT32 value
)
{
	EFI_STATUS Status;
	UINT32 State;

	Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(6),
			   EXTCSD_WRITE_BYTES | EXTCSD_CMD(ext_cmd) |
			   EXTCSD_VALUE(value) | EXTCSD_CMD_SET_NORMAL,
			   MMC_RESPONSE_R1B, NULL);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	do {
		Status = mmc_device_state(MmcHostInstance, &State);
		if (EFI_ERROR (Status)) {
			return Status;
		}
	} while (State == MMC_STATE_PRG);

	return EFI_SUCCESS;
}

STATIC 
EFI_STATUS 
mmc_sd_switch(
    IN MMC_HOST_INSTANCE     *MmcHostInstance,
    IN UINT32 bus_width
  )
{
	EFI_STATUS Status;
	UINT32 State;
	INT32 retries = MMC_DEFAULT_MAX_RETRIES;
	UINT32 bus_width_arg = 0;

	Status = MmcHostInstance->MmcHost->Prepare(MmcHostInstance->MmcHost, 0, sizeof(scr), (UINTN)&scr);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	/* CMD55: Application Specific Command */
	Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(55), rca << RCA_SHIFT_OFFSET,
			   MMC_RESPONSE_R5, NULL);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	/* ACMD51: SEND_SCR */
	do {
		Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_ACMD(51), 0, MMC_RESPONSE_R1, NULL);
		if ((EFI_ERROR (Status)) && (retries == 0)) {
			// pr_err("ACMD51 failed after %d retries (ret=%d)\n",
			//       MMC_DEFAULT_MAX_RETRIES, ret);
			DEBUG ((DEBUG_ERROR, "%a: ACMD51 failed after %d retries (Status=%r)\n", __FUNCTION__, MMC_DEFAULT_MAX_RETRIES, Status));
			return Status;
		}

		retries--;
	} while (EFI_ERROR (Status));

	Status = MmcHostInstance->MmcHost->ReadBlockData(MmcHostInstance->MmcHost, 0, sizeof(scr), scr);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	if (((scr[0] & SD_SCR_BUS_WIDTH_4) != 0U) &&
	    (bus_width == MMC_BUS_WIDTH_4)) {
		bus_width_arg = 2;
	}

	/* CMD55: Application Specific Command */
	Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(55), rca << RCA_SHIFT_OFFSET,
			   MMC_RESPONSE_R5, NULL);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	/* ACMD6: SET_BUS_WIDTH */
	Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_ACMD(6), bus_width_arg, MMC_RESPONSE_R1, NULL);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	do {
		Status = mmc_device_state(MmcHostInstance, &State);
		if (EFI_ERROR (Status)) {
			return Status;
		}
	} while (State == MMC_STATE_PRG);

	return EFI_SUCCESS;
}

STATIC 
EFI_STATUS 
mmc_set_ios(  
  IN MMC_HOST_INSTANCE     *MmcHostInstance,
  IN UINT32 clk, 
  IN UINT32 bus_width
  )
{
	EFI_STATUS Status;
	UINT32 width = bus_width;

	if (mmc_dev_info.mmc_dev_type != MMC_IS_EMMC) {
		if (width == MMC_BUS_WIDTH_8) {
			// pr_err("Wrong bus config for SD-card, force to 4\n");
			DEBUG ((DEBUG_INFO, "%a: Wrong bus config for SD-card, force to 4\n", __FUNCTION__));
			width = MMC_BUS_WIDTH_4;
		}
		Status = mmc_sd_switch(MmcHostInstance, width);
		if (EFI_ERROR (Status)) {
			return Status;
		}
	} else if (mmc_csd.spec_vers == 4U) {
		Status = mmc_set_ext_csd(MmcHostInstance, CMD_EXTCSD_BUS_WIDTH,
				      (UINT32)width);
		if (EFI_ERROR (Status)) {
			return Status;
		}
	} else {
		// pr_err("Wrong MMC type or spec version\n");
		DEBUG ((DEBUG_INFO, "%a: Wrong MMC type or spec version\n", __FUNCTION__));
	}

	return MmcHostInstance->MmcHost->SetIos(MmcHostInstance->MmcHost, clk, width);
}

STATIC 
EFI_STATUS 
mmc_fill_device_info(
    IN MMC_HOST_INSTANCE     *MmcHostInstance
)
{
	UINTN c_size;
	UINT32 speed_idx;
	UINT32 nb_blocks;
	UINT32 freq_unit;
	EFI_STATUS Status = EFI_SUCCESS;
	UINT32 State;
	struct mmc_csd_sd_v2 *csd_sd_v2;

	switch (mmc_dev_info.mmc_dev_type) {
	case MMC_IS_EMMC:
		mmc_dev_info.block_size = MMC_BLOCK_SIZE;

		Status = MmcHostInstance->MmcHost->Prepare(MmcHostInstance->MmcHost, 0, sizeof(mmc_ext_csd), (UINTN)&mmc_ext_csd);
		
		if (EFI_ERROR (Status)) {
			return Status;
		}

		/* MMC CMD8: SEND_EXT_CSD */
		Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(8), 0, MMC_RESPONSE_R1, NULL);
		if (EFI_ERROR (Status)) {
			return Status;
		}

		Status = MmcHostInstance->MmcHost->ReadBlockData(MmcHostInstance->MmcHost, 0, sizeof(mmc_ext_csd), (UINT32*)mmc_ext_csd);
		if (EFI_ERROR (Status)) {
			return Status;
		}

		do {
			Status = mmc_device_state(MmcHostInstance, &State);
			if (EFI_ERROR (Status)) {
				return Status;
			}
		} while (State != MMC_STATE_TRAN);

		nb_blocks = (mmc_ext_csd[CMD_EXTCSD_SEC_CNT] << 0) |
			    (mmc_ext_csd[CMD_EXTCSD_SEC_CNT + 1] << 8) |
			    (mmc_ext_csd[CMD_EXTCSD_SEC_CNT + 2] << 16) |
			    (mmc_ext_csd[CMD_EXTCSD_SEC_CNT + 3] << 24);

		mmc_dev_info.device_size = (unsigned long long)nb_blocks *
			mmc_dev_info.block_size;

		break;

	case MMC_IS_SD:
		/*
		 * Use the same mmc_csd struct, as required fields here
		 * (READ_BL_LEN, C_SIZE, CSIZE_MULT) are common with eMMC.
		 */
		mmc_dev_info.block_size = BIT_32(mmc_csd.read_bl_len);

		c_size = ((unsigned long long)mmc_csd.c_size_high << 2U) |
			 (unsigned long long)mmc_csd.c_size_low;
		ASSERT(c_size != 0xFFFU);

		mmc_dev_info.device_size = (c_size + 1U) *
					    BIT_64(mmc_csd.c_size_mult + 2U) *
					    mmc_dev_info.block_size;

		break;

	case MMC_IS_SD_HC:
	    MmcHostInstance->CardInfo.CardType = SD_CARD_2_HIGH;
		
		ASSERT(mmc_csd.csd_structure == 1U);

		mmc_dev_info.block_size = MMC_BLOCK_SIZE;

		/* Need to use mmc_csd_sd_v2 struct */
		csd_sd_v2 = (struct mmc_csd_sd_v2 *)&mmc_csd;
		c_size = ((unsigned long long)csd_sd_v2->c_size_high << 16) |
			 (unsigned long long)csd_sd_v2->c_size_low;

		mmc_dev_info.device_size = (c_size + 1U) << MULT_BY_512K_SHIFT;
		break;

	default:
		Status = EFI_DEVICE_ERROR;
		break;
	}

	if (EFI_ERROR (Status)) {
		return Status;
	}

	speed_idx = (mmc_csd.tran_speed & CSD_TRAN_SPEED_MULT_MASK) >>
			 CSD_TRAN_SPEED_MULT_SHIFT;

	ASSERT(speed_idx > 0U);

	if (mmc_dev_info.mmc_dev_type == MMC_IS_EMMC) {
		mmc_dev_info.max_bus_freq = tran_speed_base[speed_idx];
	} else {
		mmc_dev_info.max_bus_freq = sd_tran_speed_base[speed_idx];
	}

	freq_unit = mmc_csd.tran_speed & CSD_TRAN_SPEED_UNIT_MASK;
	while (freq_unit != 0U) {
		mmc_dev_info.max_bus_freq *= 10U;
		--freq_unit;
	}

	mmc_dev_info.max_bus_freq *= 10000U;

	return EFI_SUCCESS;
}

STATIC 
EFI_STATUS 
sd_send_op_cond(
    IN MMC_HOST_INSTANCE     *MmcHostInstance
)
{
	INT32 n;
	UINT32 resp_data[4];

	for (n = 0; n < SEND_OP_COND_MAX_RETRIES; n++) {
		EFI_STATUS Status;

		/* CMD55: Application Specific Command */
		Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(55), 0, MMC_RESPONSE_R1, NULL);
		if (EFI_ERROR (Status)) {
			return Status;
		}

		/* ACMD41: SD_SEND_OP_COND */
		Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_ACMD(41), OCR_HCS |
			mmc_dev_info.ocr_voltage, MMC_RESPONSE_R3,
			&resp_data[0]);
		if (EFI_ERROR (Status)) {
			return Status;
		}

		if ((resp_data[0] & OCR_POWERUP) != 0U) {
			mmc_ocr_value = resp_data[0];

			if ((mmc_ocr_value & OCR_HCS) != 0U) {
				mmc_dev_info.mmc_dev_type = MMC_IS_SD_HC;
				MmcHostInstance->CardInfo.OCRData.AccessMode = BIT1;
			} else {
				mmc_dev_info.mmc_dev_type = MMC_IS_SD;
				MmcHostInstance->CardInfo.OCRData.AccessMode = 0x0;
			}

			return EFI_SUCCESS;
		}

		gBS->Stall (10000);
	}

	// pr_err("ACMD41 failed after %d retries\n", SEND_OP_COND_MAX_RETRIES);
	DEBUG ((DEBUG_ERROR, "%a: ACMD41 failed after %d retries\n", __FUNCTION__, SEND_OP_COND_MAX_RETRIES));

	return EFI_DEVICE_ERROR;
}

STATIC 
EFI_STATUS 
mmc_reset_to_idle(
  IN MMC_HOST_INSTANCE     *MmcHostInstance
)
{
	EFI_STATUS Status;

	/* CMD0: reset to IDLE */
	Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(0), 0, 0, NULL);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	gBS->Stall (2000);

	return EFI_SUCCESS;
}

STATIC 
EFI_STATUS 
mmc_send_op_cond(
	IN MMC_HOST_INSTANCE     *MmcHostInstance
)
{
	INT32 n;
	EFI_STATUS Status;
	UINT32 resp_data[4];

	Status = mmc_reset_to_idle(MmcHostInstance);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	for (n = 0; n < SEND_OP_COND_MAX_RETRIES; n++) {
		Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(1), OCR_SECTOR_MODE |
				   OCR_VDD_MIN_2V7 | OCR_VDD_MIN_1V7,
				   MMC_RESPONSE_R3, &resp_data[0]);
		if (EFI_ERROR (Status)) {
			return Status;
		}

		if ((resp_data[0] & OCR_POWERUP) != 0U) {
			mmc_ocr_value = resp_data[0];
			return EFI_SUCCESS;
		}

		gBS->Stall (10000);
	}

	// pr_err("CMD1 failed after %d retries\n", SEND_OP_COND_MAX_RETRIES);
	DEBUG ((DEBUG_ERROR, "%a: CMD1 failed after %d retries\n", __FUNCTION__, SEND_OP_COND_MAX_RETRIES));

	return EFI_DEVICE_ERROR;
}

STATIC 
EFI_STATUS 
mmc_enumerate(  
  IN MMC_HOST_INSTANCE     *MmcHostInstance,
  IN UINT32                clk, 
  IN UINT32                bus_width
  )
{
	EFI_STATUS Status;
	UINT32     State;
	UINT32 resp_data[4];

	Status = mmc_reset_to_idle(MmcHostInstance);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	if (mmc_dev_info.mmc_dev_type == MMC_IS_EMMC) {
		Status = mmc_send_op_cond(MmcHostInstance);
	} else {
		/* CMD8: Send Interface Condition Command */
		Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(8), VHS_2_7_3_6_V | CMD8_CHECK_PATTERN,
				   MMC_RESPONSE_R5, &resp_data[0]);

		if ((Status == EFI_SUCCESS) && ((resp_data[0] & 0xffU) == CMD8_CHECK_PATTERN)) {
			Status = sd_send_op_cond(MmcHostInstance);
		}
	}
	if (EFI_ERROR (Status)) {
		return Status;
	}

	/* CMD2: Card Identification */
	Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(2), 0, MMC_RESPONSE_R2, NULL);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	/* CMD3: Set Relative Address */
	if (mmc_dev_info.mmc_dev_type == MMC_IS_EMMC) {
		rca = MMC_FIX_RCA;
		Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(3), rca << RCA_SHIFT_OFFSET,
				   MMC_RESPONSE_R1, NULL);
		if (EFI_ERROR (Status)) {
			return Status;
		}
	} else {
		Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(3), 0,
				   MMC_RESPONSE_R6, &resp_data[0]);
		if (EFI_ERROR (Status)) {
			return Status;
		}

		rca = (resp_data[0] & 0xFFFF0000U) >> 16;
	}

	/* CMD9: CSD Register */
	Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(9), rca << RCA_SHIFT_OFFSET,
			   MMC_RESPONSE_R2, &resp_data[0]);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	CopyMem(&mmc_csd, &resp_data, sizeof(resp_data));

	/* CMD7: Select Card */
	Status = MmcHostInstance->MmcHost->SendCommand(MmcHostInstance->MmcHost, MMC_CMD(7), rca << RCA_SHIFT_OFFSET,
			   MMC_RESPONSE_R1, NULL);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	do {
		Status = mmc_device_state(MmcHostInstance, &State);
		if (EFI_ERROR (Status)) {
			return Status;
		}
	} while (State != MMC_STATE_TRAN);

	Status = mmc_set_ios(MmcHostInstance, clk, bus_width);
	if (EFI_ERROR (Status)) {
		return Status;
	}

	return mmc_fill_device_info(MmcHostInstance);
}

STATIC
EFI_STATUS
EFIAPI
MmcIdentificationMode (
  IN MMC_HOST_INSTANCE     *MmcHostInstance
  )
{
  EFI_STATUS              Status;
  UINTN                   CmdArg;
  BOOLEAN                 IsHCS;
  EFI_MMC_HOST_PROTOCOL   *MmcHost;

  MmcHost = MmcHostInstance->MmcHost;
  CmdArg = 0;
  IsHCS = FALSE;

  if (MmcHost == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // We can get into this function if we restart the identification mode
  if (MmcHostInstance->State == MmcHwInitializationState) {
    // Initialize the MMC Host HW
    Status = MmcNotifyState (MmcHostInstance, MmcHwInitializationState);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Error MmcHwInitializationState, Status=%r.\n", Status));
      return Status;
    }
  }

  Status = mmc_enumerate(MmcHostInstance, 50 * 1000 * 1000, MMC_BUS_WIDTH_4);

  if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "MmcIdentificationMode() : Error mmc_enumerate, Status=%r.\n", Status));
      return Status;
  }

  MmcHostInstance->CardInfo.RCA = rca;
  MmcHostInstance->BlockIo.Media->LastBlock = ((mmc_dev_info.device_size >> 9) - 1);
  MmcHostInstance->BlockIo.Media->BlockSize = mmc_dev_info.block_size;
  MmcHostInstance->BlockIo.Media->ReadOnly = MmcHost->IsReadOnly (MmcHost);
  MmcHostInstance->BlockIo.Media->MediaPresent = TRUE;
  MmcHostInstance->BlockIo.Media->MediaId++;

  return EFI_SUCCESS;
}

EFI_STATUS
InitializeMmcDevice (
  IN  MMC_HOST_INSTANCE   *MmcHostInstance
  )
{
  EFI_STATUS              Status;
  EFI_MMC_HOST_PROTOCOL   *MmcHost;
  UINTN                   BlockCount;

  BlockCount = 1;
  MmcHost = MmcHostInstance->MmcHost;

  Status = MmcIdentificationMode (MmcHostInstance);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "InitializeMmcDevice(): Error in Identification Mode, Status=%r\n", Status));
    return Status;
  }

  Status = MmcNotifyState (MmcHostInstance, MmcTransferState);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "InitializeMmcDevice(): Error MmcTransferState, Status=%r\n", Status));
    return Status;
  }

  // Set Block Length
  Status = MmcHost->SendCommand (MmcHost, MMC_CMD16, MmcHostInstance->BlockIo.Media->BlockSize, MMC_RESPONSE_R1, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR,
      "InitializeMmcDevice(MMC_CMD16): Error MmcHostInstance->BlockIo.Media->BlockSize: %d and Error = %r\n",
      MmcHostInstance->BlockIo.Media->BlockSize, Status));
    return Status;
  }

  // Block Count (not used). Could return an error for SD card
  if (MmcHostInstance->CardInfo.CardType == MMC_CARD) {
    Status = MmcHost->SendCommand (MmcHost, MMC_CMD23, BlockCount, MMC_RESPONSE_R1, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "InitializeMmcDevice(MMC_CMD23): Error, Status=%r\n", Status));
      return Status;
    }
  }

  return EFI_SUCCESS;
}
