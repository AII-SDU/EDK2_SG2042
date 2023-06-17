/*
 * Copyright (c) 2016-2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <Uefi.h>
#include <Base.h>
#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>

#include "Sdhci.h"

#define SDCARD_INIT_FREQ	(200 * 1000)
#define SDCARD_TRAN_FREQ	(6 * 1000 * 1000)

STATIC VOID bm_sd_hw_init(VOID);

STATIC bm_sd_params_t bm_params = {
	.reg_base	= SDIO_BASE,
	.clk_rate	= 50 * 1000 * 1000,
	.bus_width	= MMC_BUS_WIDTH_4,
	.flags		= 0,
	.card_in	= SDCARD_STATUS_UNKNOWN,
};

INT32 bm_get_sd_clock(VOID)
{
	return 100*1000*1000;
}

STATIC EFI_STATUS bm_sd_send_cmd_with_data(struct mmc_cmd *cmd)
{
	UINTN base;
	UINT32 mode = 0, stat, dma_addr, flags = 0;
	UINT32 timeout;

	base = bm_params.reg_base;

	//make sure cmd line is clear
	while (1) {
		if (!(MmioRead32(base + SDHCI_PSTATE) & SDHCI_CMD_INHIBIT))
			break;
	}

	switch (cmd->cmd_idx) {
	case MMC_CMD(17):
	case MMC_CMD(18):
	case MMC_ACMD(51):
		mode = SDHCI_TRNS_BLK_CNT_EN | SDHCI_TRNS_MULTI | SDHCI_TRNS_READ;
		if (!(bm_params.flags & SD_USE_PIO))
			mode |= SDHCI_TRNS_DMA;
		break;
	case MMC_CMD(24):
	case MMC_CMD(25):
		mode = (SDHCI_TRNS_BLK_CNT_EN
			   | SDHCI_TRNS_MULTI) & ~SDHCI_TRNS_READ;
		if (!(bm_params.flags & SD_USE_PIO))
			mode |= SDHCI_TRNS_DMA;
		break;
	default:
	    ASSERT(0);
	}

	MmioWrite16(base + SDHCI_TRANSFER_MODE, mode);
	MmioWrite32(base + SDHCI_ARGUMENT, cmd->cmd_arg);

	// set cmd flags
	if (cmd->cmd_idx == MMC_CMD(0))
		flags |= SDHCI_CMD_RESP_NONE;
	else {
		if (cmd->resp_type & MMC_RSP_136)
			flags |= SDHCI_CMD_RESP_LONG;
		else
			flags |= SDHCI_CMD_RESP_SHORT;
		if (cmd->resp_type & MMC_RSP_CRC)
			flags |= SDHCI_CMD_CRC;
		if (cmd->resp_type & MMC_RSP_CMD_IDX)
			flags |= SDHCI_CMD_INDEX;
	}
	flags |= SDHCI_CMD_DATA;
	//verbose("SDHCI flags 0x%x\n", flags);

	// issue the cmd
	MmioWrite16(base + SDHCI_COMMAND, SDHCI_MAKE_CMD(cmd->cmd_idx, flags));

	// check cmd complete if necessary
	if ((MmioRead16(base + SDHCI_TRANSFER_MODE) & SDHCI_TRNS_RESP_INT) == 0) {
		timeout = 100000;
		while (1) {
			stat = MmioRead16(base + SDHCI_INT_STATUS);
			if (stat & SDHCI_INT_ERROR) {
				DEBUG ((DEBUG_ERROR, "%a: interrupt error: 0x%x 0x%x\n", __FUNCTION__,  MmioRead16(base + SDHCI_INT_STATUS),
				                        MmioRead16(base + SDHCI_ERR_INT_STATUS)));
				return EFI_DEVICE_ERROR;
			}
			if (stat & SDHCI_INT_CMD_COMPLETE) {
				MmioWrite16(base + SDHCI_INT_STATUS,
					      stat | SDHCI_INT_CMD_COMPLETE);
				break;
			}

			gBS->Stall (1);
			if (!timeout--) {
				DEBUG ((DEBUG_ERROR, "%a: timeout!\n", __FUNCTION__));
				return EFI_TIMEOUT;
			}
		}

		// get cmd respond
		if (flags != SDHCI_CMD_RESP_NONE)
			cmd->resp_data[0] = MmioRead32(base + SDHCI_RESPONSE_01);
		if (flags & SDHCI_CMD_RESP_LONG) {
			cmd->resp_data[1] = MmioRead32(base + SDHCI_RESPONSE_23);
			cmd->resp_data[2] = MmioRead32(base + SDHCI_RESPONSE_45);
			cmd->resp_data[3] = MmioRead32(base + SDHCI_RESPONSE_67);
		}
	}

	// check dma/transfer complete
	if (!(bm_params.flags & SD_USE_PIO)) {
		while (1) {
			stat = MmioRead16(base + SDHCI_INT_STATUS);
			if (stat & SDHCI_INT_ERROR) {
				DEBUG ((DEBUG_ERROR, "%a: interrupt error: 0x%x 0x%x\n", __FUNCTION__,  MmioRead16(base + SDHCI_INT_STATUS),
				                        MmioRead16(base + SDHCI_ERR_INT_STATUS)));
				return EFI_DEVICE_ERROR;
			}

			if (stat & SDHCI_INT_XFER_COMPLETE) {
				MmioWrite16(base + SDHCI_INT_STATUS, stat);
				break;
			}

			if (stat & SDHCI_INT_DMA_END) {
				MmioWrite16(base + SDHCI_INT_STATUS, stat);
				if (MmioRead16(base + SDHCI_HOST_CONTROL2) &
									SDHCI_HOST_VER4_ENABLE) {
					dma_addr = MmioRead32(base + SDHCI_ADMA_SA_LOW);
					MmioWrite32(base + SDHCI_ADMA_SA_LOW, dma_addr);
					MmioWrite32(base + SDHCI_ADMA_SA_HIGH, 0);
				} else {
					dma_addr = MmioRead32(base + SDHCI_DMA_ADDRESS);
					MmioWrite32(base + SDHCI_DMA_ADDRESS, dma_addr);
				}
			}

		}
	}

	return EFI_SUCCESS;
}

STATIC EFI_STATUS bm_sd_send_cmd_without_data(struct mmc_cmd *cmd)
{
	UINTN base;
	UINT32 stat, flags = 0x0;
	UINT32 timeout = 10000;

	base = bm_params.reg_base;

	// make sure cmd line is clear
	while (1) {
		if (!(MmioRead32(base + SDHCI_PSTATE) & SDHCI_CMD_INHIBIT))
			break;
	}

	// set cmd flags
	if (cmd->cmd_idx == MMC_CMD(0))
		flags |= SDHCI_CMD_RESP_NONE;
	else if (cmd->cmd_idx == MMC_CMD(1))
		flags |= SDHCI_CMD_RESP_SHORT;
	else if (cmd->cmd_idx == MMC_ACMD(41))
		flags |= SDHCI_CMD_RESP_SHORT;
	else {
		if (cmd->resp_type & MMC_RSP_136)
			flags |= SDHCI_CMD_RESP_LONG;
		else
			flags |= SDHCI_CMD_RESP_SHORT;
		if (cmd->resp_type & MMC_RSP_CRC)
			flags |= SDHCI_CMD_CRC;
		if (cmd->resp_type & MMC_RSP_CMD_IDX)
			flags |= SDHCI_CMD_INDEX;
	}
	//verbose("SDHCI flags 0x%x\n", flags);

	// make sure dat line is clear if necessary
	if (flags != SDHCI_CMD_RESP_NONE) {
		while (1) {
			if (!(MmioRead32(base + SDHCI_PSTATE) & SDHCI_CMD_INHIBIT_DAT))
				break;
		}
	}

	// issue the cmd
	MmioWrite32(base + SDHCI_ARGUMENT, cmd->cmd_arg);
	MmioWrite16(base + SDHCI_COMMAND, SDHCI_MAKE_CMD(cmd->cmd_idx, flags));

	// check cmd complete
	timeout = 100000;
	while (1) {
		stat = MmioRead16(base + SDHCI_INT_STATUS);
		if (stat & SDHCI_INT_ERROR) {
			DEBUG ((DEBUG_ERROR, "%a: interrupt error: 0x%x 0x%x\n", __FUNCTION__,  MmioRead16(base + SDHCI_INT_STATUS),
				                        MmioRead16(base + SDHCI_ERR_INT_STATUS)));
			return EFI_DEVICE_ERROR;
		}
		if (stat & SDHCI_INT_CMD_COMPLETE) {
			MmioWrite16(base + SDHCI_INT_STATUS,
				      stat | SDHCI_INT_CMD_COMPLETE);
			break;
		}

		gBS->Stall (1);
		if (!timeout--) {
			DEBUG ((DEBUG_ERROR, "%a: timeout!\n", __FUNCTION__));
			return EFI_TIMEOUT;
		}
	}

	// get cmd respond
	if (!(flags & SDHCI_CMD_RESP_NONE))
		cmd->resp_data[0] = MmioRead32(base + SDHCI_RESPONSE_01);
	if (flags & SDHCI_CMD_RESP_LONG) {
		cmd->resp_data[1] = MmioRead32(base + SDHCI_RESPONSE_23);
		cmd->resp_data[2] = MmioRead32(base + SDHCI_RESPONSE_45);
		cmd->resp_data[3] = MmioRead32(base + SDHCI_RESPONSE_67);
	}

	return EFI_SUCCESS;
}

EFI_STATUS bm_sd_send_cmd(UINT32 idx, UINT32 arg, UINT32 r_type, UINT32 *r_data)
{
	//DEBUG ((DEBUG_INFO, "%a: SDHCI cmd, idx=%d, arg=0x%x, resp_type=0x%x\n", __FUNCTION__, idx, arg, r_type));
    struct mmc_cmd cmd;
	EFI_STATUS Status;

	// memset(&cmd, 0, sizeof(struct mmc_cmd));
	ZeroMem(&cmd,sizeof(struct mmc_cmd));

	cmd.cmd_idx = idx;
	cmd.cmd_arg = arg;
	cmd.resp_type = r_type;

	switch (cmd.cmd_idx) {
	    case MMC_CMD(17):
	    case MMC_CMD(18):
	    case MMC_CMD(24):
	    case MMC_CMD(25):
	    case MMC_ACMD(51):
		    Status = bm_sd_send_cmd_with_data(&cmd);
			break;
	    default:
		    Status = bm_sd_send_cmd_without_data(&cmd);
	}

	if ((Status == EFI_SUCCESS) && (r_data != NULL)) {
        for (INT32 i = 0; i < 4; i++) {
			*r_data = cmd.resp_data[i];
			r_data++;
		}
	}
	return Status;
}

VOID bm_sd_set_clk(INT32 clk)
{
	INT32 i, div;
	UINTN base;

	ASSERT(clk > 0);

	if (bm_params.clk_rate <= clk) {
		div = 0;
	} else {
		for (div = 0x1; div < 0xFF; div++) {
			if (bm_params.clk_rate / (2 * div) <= clk)
				break;
		}
	}
	ASSERT(div <= 0xFF);

	base = bm_params.reg_base;
	if (MmioRead16(base + SDHCI_HOST_CONTROL2) & (1 << 15)) {
		//verbose("Use SDCLK Preset Value\n");
	} else {
		//verbose("Set SDCLK by driver. div=0x%x(%d)\n", div, div);
		MmioWrite16(base + SDHCI_CLK_CTRL,
			      MmioRead16(base + SDHCI_CLK_CTRL) & ~0x9); // disable INTERNAL_CLK_EN and PLL_ENABLE
		MmioWrite16(base + SDHCI_CLK_CTRL,
			      (MmioRead16(base + SDHCI_CLK_CTRL) & 0xDF) | div << 8); // set clk div
		MmioWrite16(base + SDHCI_CLK_CTRL,
			      MmioRead16(base + SDHCI_CLK_CTRL) | 0x1); // set INTERNAL_CLK_EN

		for (i = 0; i <= 150000; i += 100) {
			if (MmioRead16(base + SDHCI_CLK_CTRL) & 0x2)
				break;
			gBS->Stall (100);
		}

		if (i > 150000) {
			DEBUG ((DEBUG_ERROR, "%a: SD INTERNAL_CLK_EN setting FAILED!\n", __FUNCTION__));
			ASSERT(0);
		}

		MmioWrite16(base + SDHCI_CLK_CTRL,
			      MmioRead16(base + SDHCI_CLK_CTRL) | 0x8); // set PLL_ENABLE

		for (i = 0; i <= 150000; i += 100) {
			if (MmioRead16(base + SDHCI_CLK_CTRL) & 0x2)
				return;
			gBS->Stall (100);
		}
	}

	DEBUG ((DEBUG_INFO, "%a: SD PLL setting FAILED!\n", __FUNCTION__));
}

VOID bm_sd_change_clk(INT32 clk)
{
	INT32 i, div;
	UINTN base;

	ASSERT(clk > 0);

	if (bm_params.clk_rate <= clk) {
		div = 0;
	} else {
		for (div = 0x1; div < 0xFF; div++) {
			if (bm_params.clk_rate / (2 * div) <= clk)
				break;
		}
	}
	ASSERT(div <= 0xFF);

	base = bm_params.reg_base;

	MmioWrite16(base + SDHCI_CLK_CTRL,
		      MmioRead16(base + SDHCI_CLK_CTRL) & ~(0x1 << 2)); // stop SD clock

	MmioWrite16(base + SDHCI_CLK_CTRL,
		      MmioRead16(base + SDHCI_CLK_CTRL) & ~0x8); // disable  PLL_ENABLE

	if (MmioRead16(base + SDHCI_HOST_CONTROL2) & (1 << 15)) {
		//verbose("Use SDCLK Preset Value\n");
		MmioWrite16(base + SDHCI_HOST_CONTROL2,
			      MmioRead16(base + SDHCI_HOST_CONTROL2) & ~0x7); // clr UHS_MODE_SEL
	} else {
		//verbose("Set SDCLK by driver. div=0x%x(%d)\n", div, div);
		MmioWrite16(base + SDHCI_CLK_CTRL,
			      (MmioRead16(base + SDHCI_CLK_CTRL) & 0xDF) | div << 8); // set clk div
		MmioWrite16(base + SDHCI_CLK_CTRL,
			      MmioRead16(base + SDHCI_CLK_CTRL) & ~(0x1 << 5)); // CLK_GEN_SELECT
	}
	MmioWrite16(base + SDHCI_CLK_CTRL,
		      MmioRead16(base + SDHCI_CLK_CTRL) | 0xc); // enable  PLL_ENABLE

	for (i = 0; i <= 150000; i += 100) {
		if (MmioRead16(base + SDHCI_CLK_CTRL) & 0x2)
			return;
		gBS->Stall (100);
	}

	DEBUG ((DEBUG_INFO, "%a: SD PLL setting FAILED!\n", __FUNCTION__));
}

INT32 bm_sd_card_detect(VOID)
{
	UINTN base, reg;

	base = bm_params.reg_base;

	if (bm_params.card_in != SDCARD_STATUS_UNKNOWN)
		return bm_params.card_in;

	MmioWrite16(base + SDHCI_INT_STATUS_EN,
		      MmioRead16(base + SDHCI_INT_STATUS_EN) | SDHCI_INT_CARD_INSERTION_EN);

	reg = MmioRead32(base + SDHCI_PSTATE);

	if (reg & SDHCI_CARD_INSERTED)
		bm_params.card_in = SDCARD_STATUS_INSERTED;
	else
		bm_params.card_in = SDCARD_STATUS_NOT_INSERTED;

	return bm_params.card_in;
}

STATIC VOID bm_sd_hw_init(VOID)
{
	UINTN base, vendor_base;

	base = bm_params.reg_base;
	vendor_base = base + (MmioRead16(base + P_VENDOR_SPECIFIC_AREA) & ((1 << 12) - 1));
	bm_params.vendor_base = vendor_base;

	// deasset reset of phy
	MmioWrite32(base + SDHCI_P_PHY_CNFG, MmioRead32(base + SDHCI_P_PHY_CNFG) | (1 << PHY_CNFG_PHY_RSTN));

	// reset data & cmd
	MmioWrite8(base + SDHCI_SOFTWARE_RESET, 0x6);

	// init common parameters
	MmioWrite8(base + SDHCI_PWR_CONTROL, (0x7 << 1));
	MmioWrite8(base + SDHCI_TOUT_CTRL, 0xe);  // for TMCLK 50Khz
	MmioWrite16(base + SDHCI_HOST_CONTROL2,
		      MmioRead16(base + SDHCI_HOST_CONTROL2) | 1 << 11);  // set cmd23 support
	MmioWrite16(base + SDHCI_CLK_CTRL, MmioRead16(base + SDHCI_CLK_CTRL) & ~(0x1 << 5));  // divided clock mode

	// set host version 4 parameters
	MmioWrite16(base + SDHCI_HOST_CONTROL2,
		      MmioRead16(base + SDHCI_HOST_CONTROL2) | (1 << 12)); // set HOST_VER4_ENABLE
	if (MmioRead32(base + SDHCI_CAPABILITIES1) & (0x1 << 27)) {
		MmioWrite16(base + SDHCI_HOST_CONTROL2,
			      MmioRead16(base + SDHCI_HOST_CONTROL2) | 0x1 << 13); // set 64bit addressing
	}

	// if support asynchronous int
	if (MmioRead32(base + SDHCI_CAPABILITIES1) & (0x1 << 29))
		MmioWrite16(base + SDHCI_HOST_CONTROL2,
			      MmioRead16(base + SDHCI_HOST_CONTROL2) | (0x1 << 14)); // enable async int
	// give some time to power down card
	gBS->Stall (20000);

	MmioWrite16(base + SDHCI_HOST_CONTROL2,
		      MmioRead16(base + SDHCI_HOST_CONTROL2) & ~(0x1 << 8)); // clr UHS2_IF_ENABLE
	MmioWrite8(base + SDHCI_PWR_CONTROL,
		     MmioRead8(base + SDHCI_PWR_CONTROL) | 0x1); // set SD_BUS_PWR_VDD1
	MmioWrite16(base + SDHCI_HOST_CONTROL2,
		      MmioRead16(base + SDHCI_HOST_CONTROL2) & ~0x7); // clr UHS_MODE_SEL
	bm_sd_set_clk(SDCARD_INIT_FREQ);
	gBS->Stall (50000);

	MmioWrite16(base + SDHCI_CLK_CTRL,
		      MmioRead16(base + SDHCI_CLK_CTRL) | (0x1 << 2)); // supply SD clock
	gBS->Stall (400); // wait for voltage ramp up time at least 74 cycle, 400us is 80 cycles for 200Khz

	MmioWrite16(base + SDHCI_INT_STATUS, MmioRead16(base + SDHCI_INT_STATUS) | (0x1 << 6));

	//we enable all interrupt status here for testing
	MmioWrite16(base + SDHCI_INT_STATUS_EN, MmioRead16(base + SDHCI_INT_STATUS_EN) | 0xFFFF);
	MmioWrite16(base + SDHCI_ERR_INT_STATUS_EN, MmioRead16(base + SDHCI_ERR_INT_STATUS_EN) | 0xFFFF);

	//verbose("SD init done\n");
}

EFI_STATUS bm_sd_set_ios(UINT32 clk, UINT32 width)
{
	//verbose("SDHCI set ios %d %d\n", clk, width);

	switch (width) {
	case MMC_BUS_WIDTH_1:
		MmioWrite8(bm_params.reg_base + SDHCI_HOST_CONTROL,
			     MmioRead8(bm_params.reg_base + SDHCI_HOST_CONTROL) &
			     ~SDHCI_DAT_XFER_WIDTH);
		break;
	case MMC_BUS_WIDTH_4:
		MmioWrite8(bm_params.reg_base + SDHCI_HOST_CONTROL,
			     MmioRead8(bm_params.reg_base + SDHCI_HOST_CONTROL) |
			     SDHCI_DAT_XFER_WIDTH);
		break;
	default:
		ASSERT(0);
	}

	bm_sd_change_clk(clk);

	return EFI_SUCCESS;
}

EFI_STATUS bm_sd_prepare(INT32 lba, UINTN buf, UINTN size)
{
	UINTN load_addr = buf;
	UINTN base;
	UINT32 block_cnt, block_size;
	UINT8 tmp;

	// FlushDataCache(buf,size);

	if (size >= MMC_BLOCK_SIZE) {
		// CMD17, 18, 24, 25
		ASSERT(((load_addr & MMC_BLOCK_MASK) == 0) && ((size % MMC_BLOCK_SIZE) == 0));
		block_size = MMC_BLOCK_SIZE;
		block_cnt = size / MMC_BLOCK_SIZE;
	} else {
		// ACMD51
		ASSERT(((load_addr & 8) == 0) && ((size % 8) == 0));
		block_size = 8;
		block_cnt = size / 8;
	}

	base = bm_params.reg_base;

	if (!(bm_params.flags & SD_USE_PIO)) {
		if (MmioRead16(base + SDHCI_HOST_CONTROL2) & SDHCI_HOST_VER4_ENABLE) {
			MmioWrite32(base + SDHCI_ADMA_SA_LOW, load_addr);
			MmioWrite32(base + SDHCI_ADMA_SA_HIGH, (load_addr >> 32));
			MmioWrite32(base + SDHCI_DMA_ADDRESS, block_cnt);
			MmioWrite16(base + SDHCI_BLOCK_COUNT, 0);
		} else {
			ASSERT((load_addr >> 32) == 0);
			MmioWrite32(base + SDHCI_DMA_ADDRESS, load_addr);
			MmioWrite16(base + SDHCI_BLOCK_COUNT, block_cnt);
		}

		// 512K bytes SDMA buffer boundary
		MmioWrite16(base + SDHCI_BLOCK_SIZE, SDHCI_MAKE_BLKSZ(7, block_size));

		// select SDMA
		tmp = MmioRead8(base + SDHCI_HOST_CONTROL);
		tmp &= ~SDHCI_CTRL_DMA_MASK;
		tmp |= SDHCI_CTRL_SDMA;
		MmioWrite8(base + SDHCI_HOST_CONTROL, tmp);
	} else {
		MmioWrite16(base + SDHCI_BLOCK_SIZE, block_size);
		MmioWrite16(base + SDHCI_BLOCK_COUNT, block_cnt);
	}

	return EFI_SUCCESS;
}

EFI_STATUS bm_sd_read(INT32 lba, UINT32* buf, UINTN size)
{
	UINT32 timeout = 0;
	UINTN base = bm_params.reg_base;
	UINT32 *data = buf;
	UINT32 block_size = 0;
	UINT32 block_cnt = 0;
	UINT32 status = 0;

	if (bm_params.flags & SD_USE_PIO) {
		block_size = MmioRead16(base + SDHCI_BLOCK_SIZE);
		block_cnt = size / block_size;
		block_size /= 4;

		for (INT32 i = 0; i < block_cnt; ) {
			status = MmioRead16(base + SDHCI_INT_STATUS);
			if ((status & SDHCI_INT_BUF_RD_READY) &&
			    (MmioRead32(base + SDHCI_PSTATE) & SDHCI_BUF_RD_ENABLE)) {
				MmioWrite16(base + SDHCI_INT_STATUS, SDHCI_INT_BUF_RD_READY);
				for (INT32 j = 0; j < block_size; j++) {
					*(data++) = MmioRead32(base + SDHCI_BUF_DATA_R);
				}

				timeout = 0;
				i++;
			} else {
				gBS->Stall (1);;
				timeout++;
			}

			if (timeout >= 10000) {
				DEBUG ((DEBUG_INFO, "%a: sdhci read data timeout\n", __FUNCTION__));
				goto timeout;
			}
		}

		timeout = 0;
		while (1) {
			status = MmioRead16(base + SDHCI_INT_STATUS);
			if (status & SDHCI_INT_XFER_COMPLETE) {
				MmioWrite16(base + SDHCI_INT_STATUS,
					      status | SDHCI_INT_XFER_COMPLETE);

				return EFI_SUCCESS;

			} else {
				gBS->Stall (1);;
				timeout++;
			}

			if (timeout >= 10000) {
				DEBUG ((DEBUG_INFO, "%a:wait xfer complete timeout\n", __FUNCTION__));
				goto timeout;
			}
		}
	} else
		return EFI_SUCCESS;

timeout:
		return EFI_TIMEOUT;

}

EFI_STATUS bm_sd_write(INT32 lba, UINT32* buf, UINTN size)
{
	UINT32 timeout = 0;
	UINTN base = bm_params.reg_base;
	UINT32 *data = buf;
	UINT32 block_size = 0;
	UINT32 block_cnt = 0;
	UINT32 status = 0;

	if (bm_params.flags & SD_USE_PIO) {
		block_size = MmioRead16(base + SDHCI_BLOCK_SIZE);
		block_cnt = size / block_size;
		block_size /= 4;

		for (INT32 j = 0; j < block_size; j++) {
			MmioWrite32(base + SDHCI_BUF_DATA_R, *(data++));
		}

		for (INT32 i = 0; i < block_cnt-1; ) {
			status = MmioRead16(base + SDHCI_INT_STATUS);
			if ((status & SDHCI_INT_BUF_WR_READY) &&
			    (MmioRead32(base + SDHCI_PSTATE) &
			     SDHCI_BUF_WR_ENABLE)) {
				MmioWrite16(base + SDHCI_INT_STATUS,
					      SDHCI_INT_BUF_WR_READY);
				for (INT32 j = 0; j < block_size; j++) {
					MmioWrite32(base + SDHCI_BUF_DATA_R, *(data++));
				}

				timeout = 0;
				i++;
			} else {
				gBS->Stall (1);;
				timeout++;
			}

			if (timeout >= 10000000) {
				DEBUG ((DEBUG_INFO, "%a:sdhci write data timeout\n", __FUNCTION__));
				goto timeout;
			}
		}

		timeout = 0;
		while (1) {
			status = MmioRead16(base + SDHCI_INT_STATUS);
			if (status & SDHCI_INT_XFER_COMPLETE) {
				MmioWrite16(base + SDHCI_INT_STATUS,
					      status | SDHCI_INT_XFER_COMPLETE);

				return 0;

			} else {
				gBS->Stall (1);;
				timeout++;
			}

			if (timeout >= 10000) {
				DEBUG ((DEBUG_INFO, "%a:wait xfer complete timeout\n", __FUNCTION__));
				goto timeout;
			}
		}
	} else
		return 0;

timeout:
		return -1;
}

VOID
SdPhyInit (
	VOID
  )
{
  	UINTN base = SDIO_BASE;
	INT32 loop = 100;

	// reset hardware
	MmioWrite8(base + SDHCI_SOFTWARE_RESET, 0x7);
	while (MmioRead8(base + SDHCI_SOFTWARE_RESET)) {
		if (loop-- > 0)
			gBS->Stall (10000);
		else
			break;
	}

	// Wait for the PHY power on ready
	loop = 100;
	while (!(MmioRead32(base + SDHCI_P_PHY_CNFG) & (1 << PHY_CNFG_PHY_PWRGOOD))) {
		if (loop-- > 0)
			gBS->Stall (10000);
		else
			break;
	}

	// Asset reset of phy
	MmioAnd32(base + SDHCI_P_PHY_CNFG, ~(1 << PHY_CNFG_PHY_RSTN));

	// Set PAD_SN PAD_SP
	MmioWrite32(base + SDHCI_P_PHY_CNFG,
		      (1 << PHY_CNFG_PHY_PWRGOOD) | (0x9 << PHY_CNFG_PAD_SP) | (0x8 << PHY_CNFG_PAD_SN));

	// Set CMDPAD
	MmioWrite16(base + SDHCI_P_CMDPAD_CNFG,
		      (0x2 << PAD_CNFG_RXSEL) | (1 << PAD_CNFG_WEAKPULL_EN) |
		      (0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N));

	// Set DATAPAD
	MmioWrite16(base + SDHCI_P_DATPAD_CNFG,
		      (0x2 << PAD_CNFG_RXSEL) | (1 << PAD_CNFG_WEAKPULL_EN) |
		      (0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N));

	// Set CLKPAD
	MmioWrite16(base + SDHCI_P_CLKPAD_CNFG,
		      (0x2 << PAD_CNFG_RXSEL) | (0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N));

	// Set STB_PAD
	MmioWrite16(base + SDHCI_P_STBPAD_CNFG,
		      (0x2 << PAD_CNFG_RXSEL) | (0x2 << PAD_CNFG_WEAKPULL_EN) |
		      (0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N));

	// Set RSTPAD
	MmioWrite16(base + SDHCI_P_RSTNPAD_CNFG,
		      (0x2 << PAD_CNFG_RXSEL) | (1 << PAD_CNFG_WEAKPULL_EN) |
		      (0x3 << PAD_CNFG_TXSLEW_CTRL_P) | (0x2 << PAD_CNFG_TXSLEW_CTRL_N));

	// Set SDCLKDL_CNFG, EXTDLY_EN = 1, fix delay
	MmioWrite8(base + SDHCI_P_SDCLKDL_CNFG, (1 << SDCLKDL_CNFG_EXTDLY_EN));

	// Set SMPLDL_CNFG, Bypass
	MmioWrite8(base + SDHCI_P_SMPLDL_CNFG, (1 << SMPLDL_CNFG_BYPASS_EN));

	// Set ATDL_CNFG, tuning clk not use for init
	MmioWrite8(base + SDHCI_P_ATDL_CNFG, (2 << ATDL_CNFG_INPSEL_CNFG));

  return;
}

EFI_STATUS
SdInit (
  IN UINT32 flags
)
{
	bm_params.clk_rate = bm_get_sd_clock();
    DEBUG((DEBUG_INFO, "SD initializing %dHz\n", bm_params.clk_rate));

	bm_params.flags = flags;

	SdPhyInit ();

    bm_sd_hw_init();

    return EFI_SUCCESS;
}
