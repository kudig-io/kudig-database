// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Alibaba Cloud. or its affiliates. All Rights Reserved.
 */

#include <kvm/arm_hypercalls.h>
#include <asm/kvm_mmu.h>
#include <asm/pv_cpufreq-abi.h>

u32 kvm_pv_cpu_freq_get(struct kvm_vcpu *vcpu)
{
	u64 host_cpufreq_khz;
	gpa_t guest_cpu_freq;
	struct pv_vcpu_freq vcpu_freq = {};

	host_cpufreq_khz = arch_cpufreq_get_khz(smp_processor_id());
	if (!host_cpufreq_khz)
		return SMCCC_RET_NOT_SUPPORTED;

	guest_cpu_freq = smccc_get_arg1(vcpu);
	vcpu_freq.freq_khz = cpu_to_le64(host_cpufreq_khz);
	kvm_write_guest_lock(vcpu->kvm, guest_cpu_freq, &vcpu_freq,
			     sizeof(struct pv_vcpu_freq));
	return SMCCC_RET_SUCCESS;
}
