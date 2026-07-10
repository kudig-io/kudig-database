// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Alibaba Cloud. or its affiliates. All Rights Reserved.
 */

#ifndef __ASM_PV_CPUFREQ_ABI_H
#define __ASM_PV_CPUFREQ_ABI_H

struct pv_vcpu_freq {
	__le64 freq_khz;
	/* Structure must be 64 byte aligned, pad to that size */
	u8 padding[56];
} __packed;

#ifdef CONFIG_PARAVIRT_GUEST_CPUFREQ
unsigned int pv_cpufreq_get(void);
#else
static inline unsigned int pv_cpufreq_get(void)
{
	return 0;
}
#endif /* CONFIG_PARAVIRT_GUEST_CPUFREQ */

#endif /* __ASM_PVSCHED_ABI_H  */
