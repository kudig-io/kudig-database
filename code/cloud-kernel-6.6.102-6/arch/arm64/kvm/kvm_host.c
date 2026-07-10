// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2024 Ant Group.
 */
#include <linux/kvm_host.h>
#include <kvm/arm_pmu.h>
#include <asm/arm_pmuv3.h>
#include <asm/vectors.h>

#include "vgic/vgic.h"

static enum kvm_mode kvm_mode = KVM_MODE_DEFAULT;

DEFINE_STATIC_KEY_FALSE(kvm_protected_mode_initialized);
EXPORT_SYMBOL_FOR_KVM(kvm_protected_mode_initialized);

static int __init early_kvm_mode_cfg(char *arg)
{
	if (!arg)
		return -EINVAL;

	if (strcmp(arg, "none") == 0) {
		kvm_mode = KVM_MODE_NONE;
		return 0;
	}

	if (!is_hyp_mode_available()) {
		pr_warn_once("KVM is not available. Ignoring kvm-arm.mode\n");
		return 0;
	}

	if (strcmp(arg, "protected") == 0) {
		if (!is_kernel_in_hyp_mode())
			kvm_mode = KVM_MODE_PROTECTED;
		else
			pr_warn_once("Protected KVM not available with VHE\n");

		return 0;
	}

	if (strcmp(arg, "nvhe") == 0 && !WARN_ON(is_kernel_in_hyp_mode())) {
		kvm_mode = KVM_MODE_DEFAULT;
		return 0;
	}

	if (strcmp(arg, "nested") == 0 && !WARN_ON(!is_kernel_in_hyp_mode())) {
		kvm_mode = KVM_MODE_NV;
		return 0;
	}

	return -EINVAL;
}
early_param("kvm-arm.mode", early_kvm_mode_cfg);

enum kvm_mode kvm_get_mode(void)
{
	return kvm_mode;
}
EXPORT_SYMBOL_FOR_KVM(kvm_get_mode);

struct gic_kvm_info *gic_kvm_info;
EXPORT_SYMBOL_FOR_KVM(gic_kvm_info);

void __init vgic_set_kvm_info(const struct gic_kvm_info *info)
{
	BUG_ON(gic_kvm_info != NULL);
	gic_kvm_info = kmalloc(sizeof(*info), GFP_KERNEL);
	if (gic_kvm_info)
		*gic_kvm_info = *info;
}

DEFINE_STATIC_KEY_FALSE(kvm_arm_pmu_available);
EXPORT_SYMBOL_FOR_KVM(kvm_arm_pmu_available);

LIST_HEAD(arm_pmus);
EXPORT_SYMBOL_FOR_KVM(arm_pmus);
DEFINE_MUTEX(arm_pmus_lock);
EXPORT_SYMBOL_FOR_KVM(arm_pmus_lock);

void kvm_host_pmu_init(struct arm_pmu *pmu)
{
	struct arm_pmu_entry *entry;

	/*
	 * Check the sanitised PMU version for the system, as KVM does not
	 * support implementations where PMUv3 exists on a subset of CPUs.
	 */
	if (!pmuv3_implemented(kvm_arm_pmu_get_pmuver_limit()))
		return;

	mutex_lock(&arm_pmus_lock);

	entry = kmalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		goto out_unlock;

	entry->arm_pmu = pmu;
	list_add_tail(&entry->entry, &arm_pmus);

	if (list_is_singular(&arm_pmus))
		static_branch_enable(&kvm_arm_pmu_available);

out_unlock:
	mutex_unlock(&arm_pmus_lock);
}

u8 kvm_arm_pmu_get_pmuver_limit(void)
{
	u64 tmp;

	tmp = read_sanitised_ftr_reg(SYS_ID_AA64DFR0_EL1);
	tmp = cpuid_feature_cap_perfmon_field(tmp,
					      ID_AA64DFR0_EL1_PMUVer_SHIFT,
					      ID_AA64DFR0_EL1_PMUVer_V3P5);
	return FIELD_GET(ARM64_FEATURE_MASK(ID_AA64DFR0_EL1_PMUVer), tmp);
}
EXPORT_SYMBOL_FOR_KVM(kvm_arm_pmu_get_pmuver_limit);

#ifdef CONFIG_KVM_ARM_HOST_VHE_ONLY
/* PMU events callbacks, use RCU and static call similar to perf_guest_cbs. */
struct kvm_pmu_ops __rcu *kvm_pmu_ops;

DEFINE_STATIC_CALL_NULL(__kvm_set_pmu_events, *kvm_pmu_ops->set_pmu_events);
DEFINE_STATIC_CALL_NULL(__kvm_clr_pmu_events, *kvm_pmu_ops->clr_pmu_events);
DEFINE_STATIC_CALL_RET0(__kvm_set_pmuserenr, *kvm_pmu_ops->set_pmuserenr);
DEFINE_STATIC_CALL_NULL(__kvm_vcpu_pmu_resync_el0, *kvm_pmu_ops->vcpu_pmu_resync_el0);

void kvm_register_pmu_handlers(struct kvm_pmu_ops *ops)
{
	if (WARN_ON_ONCE(rcu_access_pointer(kvm_pmu_ops)))
		return;

	rcu_assign_pointer(kvm_pmu_ops, ops);
	static_call_update(__kvm_set_pmu_events, ops->set_pmu_events);
	static_call_update(__kvm_clr_pmu_events, ops->clr_pmu_events);
	static_call_update(__kvm_set_pmuserenr, ops->set_pmuserenr);
	static_call_update(__kvm_vcpu_pmu_resync_el0, ops->vcpu_pmu_resync_el0);
}
EXPORT_SYMBOL_FOR_KVM(kvm_register_pmu_handlers);

void kvm_unregister_pmu_handlers(struct kvm_pmu_ops *ops)
{
	if (WARN_ON_ONCE(rcu_access_pointer(kvm_pmu_ops) != ops))
		return;

	rcu_assign_pointer(kvm_pmu_ops, NULL);
	static_call_update(__kvm_set_pmu_events, NULL);
	static_call_update(__kvm_clr_pmu_events, NULL);
	static_call_update(__kvm_set_pmuserenr, (void *)&__static_call_return0);
	static_call_update(__kvm_vcpu_pmu_resync_el0, NULL);
	synchronize_rcu();
}
EXPORT_SYMBOL_FOR_KVM(kvm_unregister_pmu_handlers);

void kvm_patch_vector_branch(struct alt_instr *alt,
			     __le32 *origptr, __le32 *updptr, int nr_inst)
{
	if (!cpus_have_cap(ARM64_SPECTRE_V3A) ||
	    WARN_ON_ONCE(cpus_have_cap(ARM64_HAS_VIRT_HOST_EXTN)))
		return;
}
EXPORT_SYMBOL_FOR_KVM(kvm_patch_vector_branch);

EXPORT_SYMBOL_FOR_KVM(vectors);
#endif
