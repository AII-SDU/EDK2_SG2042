/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2020 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#include <sbi/sbi_console.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/reset/fdt_reset.h>

/* List of FDT reset drivers generated at compile time */
// extern struct fdt_reset *fdt_reset_drivers[];
// extern unsigned long fdt_reset_drivers_size;
extern struct fdt_reset fdt_reset_sifive_test;
extern struct fdt_reset fdt_reset_htif;
extern struct fdt_reset fdt_reset_sophgo_mcu;
extern struct fdt_reset fdt_reset_sunxi_wdt;
extern struct fdt_reset fdt_reset_thead;

static struct fdt_reset *fdt_reset_drivers[] = {
	&fdt_reset_sifive_test,
	&fdt_reset_htif,
	&fdt_reset_sophgo_mcu,
	&fdt_reset_sunxi_wdt,
	&fdt_reset_thead,
};

int fdt_reset_driver_init(void *fdt, struct fdt_reset *drv)
{
	int noff, rc = SBI_ENODEV;
	const struct fdt_match *match;

	noff = fdt_find_match(fdt, -1, drv->match_table, &match);
	if (noff < 0)
		return SBI_ENODEV;

	if (drv->init) {
		rc = drv->init(fdt, noff, match);
		if (rc && rc != SBI_ENODEV) {
			sbi_printf("%s: %s init failed, %d\n",
				   __func__, match->compatible, rc);
		}
	}

	return rc;
}

int fdt_reset_init(void)
{
	int pos, rc;
	void *fdt = fdt_get_address();

	for (pos = 0; pos < array_size(fdt_reset_drivers); pos++){
        rc = fdt_reset_driver_init(fdt, fdt_reset_drivers[pos]);
		if (rc == SBI_ENODEV)
			continue;
		if (rc)
			return rc;
	}

	return 0;	
}
