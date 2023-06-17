#include <sbi_utils/timer/fdt_timer.h>

extern struct fdt_timer fdt_timer_mtimer;
extern struct fdt_timer fdt_timer_plmt;

struct fdt_timer *fdt_timer_drivers[] = {
	&fdt_timer_mtimer,
	&fdt_timer_plmt,
};

unsigned long fdt_timer_drivers_size = sizeof(fdt_timer_drivers) / sizeof(struct fdt_timer *);
