#include <sbi_utils/irqchip/fdt_irqchip.h>

extern struct fdt_irqchip fdt_irqchip_aplic;
extern struct fdt_irqchip fdt_irqchip_imsic;
extern struct fdt_irqchip fdt_irqchip_plic;

struct fdt_irqchip *fdt_irqchip_drivers[] = {
	&fdt_irqchip_aplic,
	&fdt_irqchip_imsic,
	&fdt_irqchip_plic,
};

unsigned long fdt_irqchip_drivers_size = sizeof(fdt_irqchip_drivers) / sizeof(struct fdt_irqchip *);
