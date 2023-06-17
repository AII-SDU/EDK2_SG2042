#include <platform_override.h>

extern const struct platform_override starfive_jh7110;
extern const struct platform_override sophgo_mango;
extern const struct platform_override sifive_fu540;
extern const struct platform_override sifive_fu740;
extern const struct platform_override renesas_rzfive;
extern const struct platform_override andes_ae350;
extern const struct platform_override sun20i_d1;

const struct platform_override *platform_override_modules[] = {
	&starfive_jh7110,
	&sophgo_mango,
	&sifive_fu540,
	&sifive_fu740,
	&renesas_rzfive,
	&andes_ae350,
	&sun20i_d1,
};

unsigned long platform_override_modules_size = sizeof(platform_override_modules) / sizeof(const struct platform_override *);
