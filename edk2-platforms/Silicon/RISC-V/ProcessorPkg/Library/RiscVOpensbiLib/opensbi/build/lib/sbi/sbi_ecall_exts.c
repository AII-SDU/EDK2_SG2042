#include <sbi/sbi_ecall.h>

extern struct sbi_ecall_extension ecall_time;
extern struct sbi_ecall_extension ecall_rfence;
extern struct sbi_ecall_extension ecall_ipi;
extern struct sbi_ecall_extension ecall_base;
extern struct sbi_ecall_extension ecall_hsm;
extern struct sbi_ecall_extension ecall_srst;
extern struct sbi_ecall_extension ecall_susp;
extern struct sbi_ecall_extension ecall_pmu;
extern struct sbi_ecall_extension ecall_dbcn;
extern struct sbi_ecall_extension ecall_legacy;
extern struct sbi_ecall_extension ecall_vendor;

struct sbi_ecall_extension *sbi_ecall_exts[] = {
	&ecall_time,
	&ecall_rfence,
	&ecall_ipi,
	&ecall_base,
	&ecall_hsm,
	&ecall_srst,
	&ecall_susp,
	&ecall_pmu,
	&ecall_dbcn,
	&ecall_legacy,
	&ecall_vendor,
};

unsigned long sbi_ecall_exts_size = sizeof(sbi_ecall_exts) / sizeof(struct sbi_ecall_extension *);
