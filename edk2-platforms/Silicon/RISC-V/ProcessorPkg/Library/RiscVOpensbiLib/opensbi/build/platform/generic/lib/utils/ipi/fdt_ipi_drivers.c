#include <sbi_utils/ipi/fdt_ipi.h>

extern struct fdt_ipi fdt_ipi_mswi;
extern struct fdt_ipi fdt_ipi_plicsw;

struct fdt_ipi *fdt_ipi_drivers[] = {
	&fdt_ipi_mswi,
	&fdt_ipi_plicsw,
};

unsigned long fdt_ipi_drivers_size = sizeof(fdt_ipi_drivers) / sizeof(struct fdt_ipi *);
