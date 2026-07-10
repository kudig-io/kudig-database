// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 Alibaba Cloud. or its affiliates. All Rights Reserved.
 */

#include <linux/arm-smccc.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <asm/pv_cpufreq-abi.h>

static bool smccc_support_cpufreq;
static struct pv_vcpu_freq *new_cpu_freq;

inline unsigned int pv_cpufreq_get(void)
{
	struct arm_smccc_res res;
	unsigned int freq_khz;

	if (!smccc_support_cpufreq) {
		pr_warn_once("%s: SMCCC doesn't support to provide cpu frequency.\n", __func__);
		return 0;
	}

	arm_smccc_1_1_invoke(ARM_SMCCC_HV_PV_CPU_FREQ_GET, __pa(new_cpu_freq),
			     &res);
	if (res.a0 != SMCCC_RET_SUCCESS) {
		pr_err("%s: Failed to get cpu frequency from host by SMCCC.\n", __func__);
		return 0;
	}
	freq_khz = new_cpu_freq->freq_khz;

	return freq_khz;
}

/*
 * Detect SMCCC support for guest cpu frequency.
 */
static __init int detect_smccc_support_cpufreq(void)
{
	struct arm_smccc_res res;

	// Initialization.
	smccc_support_cpufreq = false;

	/* To detect the presence of PV CPUFREQ support we require SMCCC 1.1+ */
	if (arm_smccc_1_1_get_conduit() == SMCCC_CONDUIT_NONE) {
		pr_warn_once("Kernel doesn't support SMCCC 1.1+.\n");
		return -EOPNOTSUPP;
	}

	arm_smccc_1_1_invoke(ARM_SMCCC_ARCH_FEATURES_FUNC_ID,
			     ARM_SMCCC_HV_PV_CPU_FREQ_FEATURES, &res);
	if (res.a0 != SMCCC_RET_SUCCESS) {
		pr_warn_once("Hypervisor doesn't support pv cpufreq.\n");
		return -EOPNOTSUPP;
	}

	new_cpu_freq = kmalloc(sizeof(struct pv_vcpu_freq), GFP_KERNEL);
	if (!new_cpu_freq)
		return -ENOMEM;

	smccc_support_cpufreq = true;

	return 0;
}
late_initcall(detect_smccc_support_cpufreq);
