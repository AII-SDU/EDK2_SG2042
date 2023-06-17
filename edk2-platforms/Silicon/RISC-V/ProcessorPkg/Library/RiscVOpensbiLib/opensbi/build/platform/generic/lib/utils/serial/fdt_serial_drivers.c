#include <sbi_utils/serial/fdt_serial.h>

extern struct fdt_serial fdt_serial_cadence;
extern struct fdt_serial fdt_serial_gaisler;
extern struct fdt_serial fdt_serial_htif;
extern struct fdt_serial fdt_serial_renesas_scif;
extern struct fdt_serial fdt_serial_shakti;
extern struct fdt_serial fdt_serial_sifive;
extern struct fdt_serial fdt_serial_litex;
extern struct fdt_serial fdt_serial_uart8250;
extern struct fdt_serial fdt_serial_xlnx_uartlite;

struct fdt_serial *fdt_serial_drivers[] = {
	&fdt_serial_cadence,
	&fdt_serial_gaisler,
	&fdt_serial_htif,
	&fdt_serial_renesas_scif,
	&fdt_serial_shakti,
	&fdt_serial_sifive,
	&fdt_serial_litex,
	&fdt_serial_uart8250,
	&fdt_serial_xlnx_uartlite,
};

unsigned long fdt_serial_drivers_size = sizeof(fdt_serial_drivers) / sizeof(struct fdt_serial *);
