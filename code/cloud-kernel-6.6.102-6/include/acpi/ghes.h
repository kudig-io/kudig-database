/* SPDX-License-Identifier: GPL-2.0 */
#ifndef GHES_H
#define GHES_H

#include <acpi/apei.h>
#include <acpi/hed.h>

/*
 * One struct ghes is created for each generic hardware error source.
 * It provides the context for APEI hardware error timer/IRQ/SCI/NMI
 * handler.
 *
 * estatus: memory buffer for error status block, allocated during
 * HEST parsing.
 */
#define GHES_EXITING		0x0002

struct ghes {
	union {
		struct acpi_hest_generic *generic;
		struct acpi_hest_generic_v2 *generic_v2;
	};
	struct acpi_hest_generic_status *estatus;
	unsigned int estatus_length;
	unsigned long flags;
	union {
		struct list_head list;
		struct timer_list timer;
		unsigned int irq;
	};
	struct device *dev;
	struct list_head elist;
};

struct ghes_estatus_node {
	struct llist_node llnode;
	struct acpi_hest_generic *generic;
	struct ghes *ghes;
};

struct ghes_estatus_cache {
	u32 estatus_len;
	atomic_t count;
	struct acpi_hest_generic *generic;
	unsigned long long time_in;
	struct rcu_head rcu;
};

enum {
	GHES_SEV_NO = 0x0,
	GHES_SEV_CORRECTED = 0x1,
	GHES_SEV_RECOVERABLE = 0x2,
	GHES_SEV_PANIC = 0x3,
};

#ifdef CONFIG_ACPI_APEI_GHES
/**
 * ghes_register_vendor_record_notifier - register a notifier for vendor
 * records that the kernel would otherwise ignore.
 * @nb: pointer to the notifier_block structure of the event handler.
 *
 * return 0 : SUCCESS, non-zero : FAIL
 */
int ghes_register_vendor_record_notifier(struct notifier_block *nb);

/**
 * ghes_unregister_vendor_record_notifier - unregister the previously
 * registered vendor record notifier.
 * @nb: pointer to the notifier_block structure of the vendor record handler.
 */
void ghes_unregister_vendor_record_notifier(struct notifier_block *nb);

struct list_head *ghes_get_devices(void);

void ghes_estatus_pool_region_free(unsigned long addr, u32 size);
#else
static inline struct list_head *ghes_get_devices(void) { return NULL; }

static inline void ghes_estatus_pool_region_free(unsigned long addr, u32 size) { return; }
#endif

int ghes_estatus_pool_init(unsigned int num_ghes);

static inline int acpi_hest_get_version(struct acpi_hest_generic_data *gdata)
{
	return gdata->revision >> 8;
}

static inline void *acpi_hest_get_payload(struct acpi_hest_generic_data *gdata)
{
	if (acpi_hest_get_version(gdata) >= 3)
		return (void *)(((struct acpi_hest_generic_data_v300 *)(gdata)) + 1);

	return gdata + 1;
}

static inline int acpi_hest_get_error_length(struct acpi_hest_generic_data *gdata)
{
	return ((struct acpi_hest_generic_data *)(gdata))->error_data_length;
}

static inline int acpi_hest_get_size(struct acpi_hest_generic_data *gdata)
{
	if (acpi_hest_get_version(gdata) >= 3)
		return sizeof(struct acpi_hest_generic_data_v300);

	return sizeof(struct acpi_hest_generic_data);
}

static inline int acpi_hest_get_record_size(struct acpi_hest_generic_data *gdata)
{
	return (acpi_hest_get_size(gdata) + acpi_hest_get_error_length(gdata));
}

static inline void *acpi_hest_get_next(struct acpi_hest_generic_data *gdata)
{
	return (void *)(gdata) + acpi_hest_get_record_size(gdata);
}

#define apei_estatus_for_each_section(estatus, section)			\
	for (section = (struct acpi_hest_generic_data *)(estatus + 1);	\
	     (void *)section - (void *)(estatus + 1) < estatus->data_length; \
	     section = acpi_hest_get_next(section))

#ifdef CONFIG_ACPI_APEI_SEA
int ghes_notify_sea(void);
#else
static inline int ghes_notify_sea(void) { return -ENOENT; }
#endif

struct notifier_block;
extern void ghes_register_report_chain(struct notifier_block *nb);
extern void ghes_unregister_report_chain(struct notifier_block *nb);

#ifdef CONFIG_YITIAN_CPER_RAWDATA
#pragma pack(1)
struct yitian_raw_data_header {
	uint32_t signature; /* 'r' 'a' 'w' 'd' */
	uint8_t type;
	uint8_t common_reg_nr;
	/* one record may have multiple sub-record (up to 6) */
	uint8_t sub_type[6];
};

struct yitian_ras_common_reg {
	uint64_t fr;
	uint64_t ctrl;
	uint64_t status;
	uint64_t addr;
	uint64_t misc0;
	uint64_t misc1;
	uint64_t misc2;
	uint64_t misc3;
};

enum yitian_ras_type {
	ERR_TYPE_GENERIC = 0x40,
	ERR_TYPE_CORE = 0x41,
	ERR_TYPE_GIC = 0x42,
	ERR_TYPE_CMN = 0x43,
	ERR_TYPE_SMMU = 0x44,
	ERR_TYPE_DDR = 0x50,
	ERR_TYPE_PCI = 0x60
};

enum cmn_node_type {
	NODE_TYPE_DVM = 0x1,
	NODE_TYPE_CFG = 0x2,
	NODE_TYPE_DTC = 0x3,
	NODE_TYPE_HN_I = 0x4,
	NODE_TYPE_HN_F = 0x5,
	NODE_TYPE_XP = 0x6,
	NODE_TYPE_SBSX = 0x7,
	NODE_TYPE_MPAM_S = 0x8,
	NODE_TYPE_MPAM_NS = 0x9,
	NODE_TYPE_RN_I = 0xA,
	NODE_TYPE_RN_D = 0xD,
	NODE_TYPE_RN_SAM = 0xF,
	NODE_TYPE_HN_P = 0x11,
	/* Coherent Multichip Link (CML) node types */
	NODE_TYPE_CML_BASE = 0x100,
	NODE_TYPE_CXRA = 0x100,
	NODE_TYPE_CXHA = 0x101,
	NODE_TYPE_CXLA = 0x102,
	NODE_TYPE_CCRA = 0x103,
	NODE_TYPE_CCHA = 0x104,
	NODE_TYPE_CCLA = 0x105,
};

struct yitian_ddr_sys_reg {
	uint64_t esr;
	uint64_t elr;
	uint64_t far;
	uint64_t scr;
	uint64_t sctlr;
	uint64_t lr;
};

struct yitian_ddr_ecc_data {
	uint32_t eccerrcnt;
	uint32_t eccstat;
	uint32_t adveccstat;
	uint32_t eccsymbol;
	uint32_t eccerrcntstat;
	uint32_t eccerrcnt0;
	uint32_t eccerrcnt1;
	uint32_t ecccaddr0;
	uint32_t ecccaddr1;
	uint32_t ecccdata0;
	uint32_t ecccdata1;
	uint32_t eccuaddr0;
	uint32_t eccuaddr1;
	uint32_t eccudata0;
	uint32_t eccudata1;
};

struct yitian_ddr_raw_data {
	uint32_t intr; /* interrupt num, valid for interrupt only, for exception intr=0 */
	uint8_t ex_type; /* 1:sync exception 2:interrupt 3:Serror */
	uint8_t el_nr; /* error el, only valid for ex_type==1, 0:el0 1:el1 2:el2 */
	uint8_t err_type; /* 1:ecc 2:CA parity 3:R/W CRC */
	struct yitian_ddr_sys_reg sys_regs; /* Only valid for ex_type==1 */
	struct yitian_ddr_ecc_data ecc_data; /* Only valid for err_type==1 */
};

#pragma pack()

#define yitian_estatus_for_each_raw_reg_common(header, reg, nr) \
	for (reg = (struct yitian_ras_common_reg *)(header + 1); \
	     nr < header->common_reg_nr; \
	     reg++, nr++)
#endif /* CONFIG_YITIAN_CPER_RAWDATA */

#endif /* GHES_H */
