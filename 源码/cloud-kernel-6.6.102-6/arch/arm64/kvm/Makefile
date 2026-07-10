# SPDX-License-Identifier: GPL-2.0
#
# Makefile for Kernel-based Virtual Machine module
#

ccflags-y += -I $(srctree)/$(src)

include $(srctree)/virt/kvm/Makefile.kvm

obj-$(CONFIG_KVM) += kvm.o

kvm-y += arm.o mmu.o mmio.o psci.o hypercalls.o pvtime.o pvsched.o \
	 inject_fault.o handle_exit.o pv_cpufreq.o \
	 guest.o debug.o reset.o sys_regs.o \
	 vgic-sys-reg-v3.o fpsimd.o \
	 arch_timer.o trng.o vmid.o emulate-nested.o nested.o \
	 vgic/vgic.o vgic/vgic-init.o \
	 vgic/vgic-irqfd.o vgic/vgic-v2.o \
	 vgic/vgic-v3.o vgic/vgic-v4.o \
	 vgic/vgic-mmio.o vgic/vgic-mmio-v2.o \
	 vgic/vgic-mmio-v3.o vgic/vgic-kvm-device.o \
	 vgic/vgic-its.o vgic/vgic-debug.o

obj-y += kvm_host.o

kvm-$(CONFIG_VIRT_PLAT_DEV)  += vgic/shadow_dev.o
kvm-$(CONFIG_HW_PERF_EVENTS)  += pmu-emul.o pmu.o
kvm-$(CONFIG_KVM) += hisilicon/hisi_virt.o

ifdef CONFIG_KVM_ARM_HOST_VHE_ONLY
ccflags-y += -I $(srctree)/$(src)/hyp/include

kvm-y += $(addprefix hyp/, vgic-v3-sr.o aarch32.o vgic-v2-cpuif-proxy.o entry.o \
	   fpsimd.o hyp-entry.o exception.o pgtable.o)
kvm-y += $(addprefix hyp/vhe/, debug-sr.o sysreg-sr.o timer-sr.o tlb.o)

$(obj)/hyp.lds: $(src)/hyp/hyp.lds.S FORCE
	$(call if_changed_dep,cpp_lds_S)

$(obj)/switch.tmp.o: $(src)/hyp/vhe/switch.c FORCE
	$(call if_changed_rule,cc_o_c)

LDFLAGS_switch.o := -r -T
$(obj)/switch.o: $(obj)/hyp.lds $(obj)/switch.tmp.o FORCE
	$(call if_changed,ld)

kvm-y += switch.o
else
kvm-y += va_layout.o stacktrace.o pkvm.o
obj-$(CONFIG_KVM) += hyp/

always-y := hyp_constants.h hyp-constants.s

define rule_gen_hyp_constants
	$(call filechk,offsets,__HYP_CONSTANTS_H__)
endef

CFLAGS_hyp-constants.o = -I $(srctree)/$(src)/hyp/include
$(obj)/hyp-constants.s: $(src)/hyp/hyp-constants.c FORCE
	$(call if_changed_dep,cc_s_c)

$(obj)/hyp_constants.h: $(obj)/hyp-constants.s FORCE
	$(call if_changed_rule,gen_hyp_constants)

obj-kvm := $(addprefix $(obj)/, $(kvm-y))
$(obj-kvm): $(obj)/hyp_constants.h
endif
