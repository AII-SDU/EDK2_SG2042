/*
 * Copyright (c) 2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MMC_H
#define MMC_H

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/BaseMemoryLib.h>
#include <Include/MmcHost.h>


#define MMC_BLOCK_SIZE			512U
#define MMC_BLOCK_MASK			(MMC_BLOCK_SIZE - 1U)
#define MMC_BOOT_CLK_RATE		(400 * 1000)

#define BIT_32(nr)			(1U << (nr))
#define BIT_64(nr)			(1ULL << (nr))

#define MMC_CMD(_x)			_x ## U

#define MMC_ACMD(_x)	    _x ## U

#define UINT64_C(c)	        c ## UL

#define GENMASK_64(h,l)    (((~UINT64_C(0)) << (l)) & (~UINT64_C(0) >> (64 - 1 - (h))))
#define GENMASK(h,l)		GENMASK_64(h,l)

#define OCR_POWERUP			BIT31
#define OCR_HCS				BIT30
#define OCR_BYTE_MODE			(0U << 29)
#define OCR_SECTOR_MODE			(2U << 29)
#define OCR_ACCESS_MODE_MASK	(3U << 29)
#define OCR_3_5_3_6			BIT23
#define OCR_3_4_3_5			BIT22
#define OCR_3_3_3_4			BIT21
#define OCR_3_2_3_3			BIT20
#define OCR_3_1_3_2			BIT19
#define OCR_3_0_3_1			BIT18
#define OCR_2_9_3_0			BIT17
#define OCR_2_8_2_9			BIT16
#define OCR_2_7_2_8			BIT15
#define OCR_VDD_MIN_2V7	    GENMASK(23, 15)
#define OCR_VDD_MIN_2V0		GENMASK(14, 8)
#define OCR_VDD_MIN_1V7		BIT7

/* Value randomly chosen for eMMC RCA, it should be > 1 */
#define MMC_FIX_RCA			6
#define RCA_SHIFT_OFFSET		16

#define CMD_EXTCSD_PARTITION_CONFIG	179
#define CMD_EXTCSD_BUS_WIDTH		183
#define CMD_EXTCSD_HS_TIMING		185
#define CMD_EXTCSD_PART_SWITCH_TIME	199
#define CMD_EXTCSD_SEC_CNT		212

#define EXT_CSD_PART_CONFIG_ACC_MASK	GENMASK(2, 0)
#define PART_CFG_BOOT_PARTITION1_ENABLE	(1U << 3)
#define PART_CFG_BOOT_PARTITION1_ACCESS (1U << 0)
#define PART_CFG_BOOT_PART_EN_MASK		GENMASK(5, 3)
#define PART_CFG_BOOT_PART_EN_SHIFT		3
#define PART_CFG_CURRENT_BOOT_PARTITION(x)	(((x) & PART_CFG_BOOT_PART_EN_MASK) >> \
	PART_CFG_BOOT_PART_EN_SHIFT)

/* Values in EXT CSD register */
#define MMC_BUS_WIDTH_1			0U
#define MMC_BUS_WIDTH_4			1U
#define MMC_BUS_WIDTH_8			2U
#define MMC_BUS_WIDTH_DDR_4		5U
#define MMC_BUS_WIDTH_DDR_8		6U
#define MMC_BOOT_MODE_BACKWARD		(0U << 3)
#define MMC_BOOT_MODE_HS_TIMING		(1U << 3)
#define MMC_BOOT_MODE_DDR		(2U << 3)

#define EXTCSD_SET_CMD			(0U << 24)
#define EXTCSD_SET_BITS			(1U << 24)
#define EXTCSD_CLR_BITS			(2U << 24)
#define EXTCSD_WRITE_BYTES		(3U << 24)
#define EXTCSD_CMD(x)			(((x) & 0xff) << 16)
#define EXTCSD_VALUE(x)			(((x) & 0xff) << 8)
#define EXTCSD_CMD_SET_NORMAL		1U

#define CSD_TRAN_SPEED_UNIT_MASK	GENMASK(2, 0)
#define CSD_TRAN_SPEED_MULT_MASK	GENMASK(6, 3)
#define CSD_TRAN_SPEED_MULT_SHIFT	3

#define STATUS_CURRENT_STATE(x)		(((x) & 0xf) << 9)
#define STATUS_READY_FOR_DATA		BIT8
#define STATUS_SWITCH_ERROR		BIT7
#define MMC_GET_STATE(x)		(((x) >> 9) & 0xf)
#define MMC_STATE_IDLE			0
#define MMC_STATE_READY			1
#define MMC_STATE_IDENT			2
#define MMC_STATE_STBY			3
#define MMC_STATE_TRAN			4
#define MMC_STATE_DATA			5
#define MMC_STATE_RCV			6
#define MMC_STATE_PRG			7
#define MMC_STATE_DIS			8
#define MMC_STATE_BTST			9
#define MMC_STATE_SLP			10

#define MMC_FLAG_CMD23			(1U << 0)

#define CMD8_CHECK_PATTERN		0xAAU
#define VHS_2_7_3_6_V			BIT8

#define SD_SCR_BUS_WIDTH_1		BIT8
#define SD_SCR_BUS_WIDTH_4		BIT10

struct mmc_cmd {
	UINT32	cmd_idx;
	UINT32	cmd_arg;
	UINT32	resp_type;
	UINT32	resp_data[4];
};
/*
struct mmc_ops {
	VOID (*init)(VOID);
	EFI_STATUS (*send_cmd)(struct mmc_cmd *cmd);
	EFI_STATUS (*set_ios)(UINT32 clk, UINT32 width);
	EFI_STATUS (*prepare)(INT32 lba, UINTN* buf, UINTN size);
	EFI_STATUS (*read)(INT32 lba, UINT32* buf, UINTN size);
	EFI_STATUS (*write)(INT32 lba, UINT32* buf, UINTN size);
};*/

/*
 * designware can't read out response bit 0-7, it only returns
 * bit 8-135, so we shift 8 bits here.
 */
struct mmc_csd_emmc {
#ifdef FULL_CSD
	UINT32		not_used:		1;
	UINT32		crc:			7;
#endif
	UINT32		ecc:			2;
	UINT32		file_format:		2;
	UINT32		tmp_write_protect:	1;
	UINT32		perm_write_protect:	1;
	UINT32		copy:			1;
	UINT32		file_format_grp:	1;

	UINT32		reserved_1:		5;
	UINT32		write_bl_partial:	1;
	UINT32		write_bl_len:		4;
	UINT32		r2w_factor:		3;
	UINT32		default_ecc:		2;
	UINT32		wp_grp_enable:		1;

	UINT32		wp_grp_size:		5;
	UINT32		erase_grp_mult:		5;
	UINT32		erase_grp_size:		5;
	UINT32		c_size_mult:		3;
	UINT32		vdd_w_curr_max:		3;
	UINT32		vdd_w_curr_min:		3;
	UINT32		vdd_r_curr_max:		3;
	UINT32		vdd_r_curr_min:		3;
	UINT32		c_size_low:		2;

	UINT32		c_size_high:		10;
	UINT32		reserved_2:		2;
	UINT32		dsr_imp:		1;
	UINT32		read_blk_misalign:	1;
	UINT32		write_blk_misalign:	1;
	UINT32		read_bl_partial:	1;
	UINT32		read_bl_len:		4;
	UINT32		ccc:			12;

	UINT32		tran_speed:		8;
	UINT32		nsac:			8;
	UINT32		taac:			8;
	UINT32		reserved_3:		2;
	UINT32		spec_vers:		4;
	UINT32		csd_structure:		2;
	UINT32		reserved_4:		8;
}__attribute__ ((__packed__));

struct mmc_csd_sd_v2 {
#ifdef FULL_CSD
	UINT32		not_used:		1;
	UINT32		crc:			7;
#endif
	UINT32		reserved_1:		2;
	UINT32		file_format:		2;
	UINT32		tmp_write_protect:	1;
	UINT32		perm_write_protect:	1;
	UINT32		copy:			1;
	UINT32		file_format_grp:	1;

	UINT32		reserved_2:		5;
	UINT32		write_bl_partial:	1;
	UINT32		write_bl_len:		4;
	UINT32		r2w_factor:		3;
	UINT32		reserved_3:		2;
	UINT32		wp_grp_enable:		1;

	UINT32		wp_grp_size:		7;
	UINT32		sector_size:		7;
	UINT32		erase_block_en:		1;
	UINT32		reserved_4:		1;
	UINT32		c_size_low:		16;

	UINT32		c_size_high:		6;
	UINT32		reserved_5:		6;
	UINT32		dsr_imp:		1;
	UINT32		read_blk_misalign:	1;
	UINT32		write_blk_misalign:	1;
	UINT32		read_bl_partial:	1;
	UINT32		read_bl_len:		4;
	UINT32		ccc:			12;

	UINT32		tran_speed:		8;
	UINT32		nsac:			8;
	UINT32		taac:			8;
	UINT32		reserved_6:		6;
	UINT32		csd_structure:		2;
	UINT32		reserved_7:		8;
}__attribute__ ((__packed__));

enum mmc_device_type {
	MMC_IS_EMMC,
	MMC_IS_SD,
	MMC_IS_SD_HC,
};

struct mmc_device_info {
	UINT64	device_size;	/* Size of device in bytes */
	UINT32		block_size;	/* Block size in bytes */
	UINT32		max_bus_freq;	/* Max bus freq in Hz */
	UINT32		ocr_voltage;	/* OCR voltage */
	enum mmc_device_type	mmc_dev_type;	/* Type of MMC */
};


EFI_STATUS mmc_send_cmd(UINT32 idx, UINT32 arg, UINT32 r_type, UINT32 *r_data);
// EFI_STATUS mmc_init(CONST struct mmc_ops *ops_ptr, UINT32 clk, UINT32 width, UINT32 flags,
// 	     struct mmc_device_info *device_info);

#endif /* MMC_H */