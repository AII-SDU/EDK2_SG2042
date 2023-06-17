#include <sbi_utils/reset/fdt_reset.h>

extern struct fdt_reset fdt_reset_atcwdt200;
extern struct fdt_reset fdt_poweroff_gpio;
extern struct fdt_reset fdt_reset_gpio;
extern struct fdt_reset fdt_reset_htif;
extern struct fdt_reset fdt_reset_sifive_test;
extern struct fdt_reset fdt_reset_sophgo_mcu;
extern struct fdt_reset fdt_reset_sunxi_wdt;
extern struct fdt_reset fdt_reset_thead;

struct fdt_reset *fdt_reset_drivers[] = {
	&fdt_reset_atcwdt200,
	&fdt_poweroff_gpio,
	&fdt_reset_gpio,
	&fdt_reset_htif,
	&fdt_reset_sifive_test,
	&fdt_reset_sophgo_mcu,
	&fdt_reset_sunxi_wdt,
	&fdt_reset_thead,
};

unsigned long fdt_reset_drivers_size = sizeof(fdt_reset_drivers) / sizeof(struct fdt_reset *);
