// SPDX-License-Identifier: GPL-2.0

/*
 * Copyright (c) 2026, Google LLC.
 * David Matlack <dmatlack@google.com>
 */

/**
 * DOC: PCI Live Update
 *
 * The PCI subsystem participates in the Live Update process to enable drivers
 * to preserve their PCI devices across kexec.
 *
 * Device preservation across Live Update is built on top of the Live Update
 * Orchestrator (LUO) support for file preservation across kexec. Userspace
 * indicates that a device should be preserved by preserving the file associated
 * with the device with ``ioctl(LIVEUPDATE_SESSION_PRESERVE_FD)``.
 *
 * .. note::
 *    The support for preserving PCI devices across Live Update is currently
 *    *partial* and should be considered *experimental*. It should only be
 *    used by developers working on the implementation for the time being.
 *
 *    To enable the support, enable ``CONFIG_PCI_LIVEUPDATE``.
 *
 * Driver API
 * ==========
 *
 * Drivers that support file-based device preservation must register their
 * ``liveupdate_file_handler`` with the PCI subsystem by calling
 * ``pci_liveupdate_register_flb()``. This ensures the PCI subsystem will be
 * notified whenever a device file is preserved so that ``struct pci_ser``
 * can be allocated to track all preserved devices. This struct is an ABI
 * and is eventually handed off to the next kernel via Kexec-Handover (KHO).
 *
 * In the "outgoing" kernel (before kexec), drivers should then notify the PCI
 * subsystem directly whenever the preservation status for a device changes:
 *
 *  * ``pci_liveupdate_preserve(pci_dev)``: The device is being preserved.
 *
 *  * ``pci_liveupdate_unpreserve(pci_dev)``: The device is no longer being
 *    preserved (preservation is cancelled).
 *
 * In the "incoming" kernel (after kexec), drivers should notify the PCI
 * subsystem with the following calls:
 *
 *  * ``pci_liveupdate_retrieve(pci_dev)``: The device file is being retrieved
 *    by userspace.
 *
 *  * ``pci_liveupdate_finish(pci_dev)``: The device is done participating in
 *    Live Update. After this point the device may no longer be even associated
 *    with the same driver.
 *
 * Incoming/Outgoing
 * =================
 *
 * The state of each device's participation in Live Update is stored in
 * ``struct pci_dev``:
 *
 *  * ``liveupdate_outgoing``: True if the device is being preserved in the
 *    outgoing kernel. Set in ``pci_liveupdate_preserve()`` and cleared in
 *    ``pci_liveupdate_unpreserve()``.
 *
 *  * ``liveupdate_incoming``: True if the device is preserved in the incoming
 *    kernel. Set during probing when the device is first created and cleared
 *    in ``pci_liveupdate_finish()``.
 *
 * Restrictions
 * ============
 *
 * Preserved devices currently have the following restrictions. Each of these
 * may be relaxed in the future.
 *
 *  * The device must not be a Virtual Function (VF).
 *
 *  * The device must not be a Physical Function (PF).
 *
 *  * The device must be the only device in its IOMMU group.
 *
 * Preservation Behavior
 * =====================
 *
 * The kernel preserves the following state for devices preserved across a Live
 * Update:
 *
 *  * The PCI Segment, Bus, Device, and Function numbers assigned to the device
 *    are guaranteed to remain the same across Live Update. Note that this is
 *    true even if pci=assign-busses is set on the command line. The kernel will
 *    always inherit bus numbers assigned by the previous kernel during a Live
 *    Update.
 *
 * This list will be extended in the future as new support is added.
 *
 * Driver Binding
 * ==============
 *
 * It is the driver's responsibility for ensuring that preserved devices are not
 * released or bound to a different driver for as long as they are preserved. In
 * practice, this is enforced by LUO taking an extra referenced to the preserved
 * device file for as long as it is preserved.
 *
 * However, there is a window of time in the incoming kernel when a device is
 * first probed and when userspace retrieves the device file with
 * ``LIVEUPDATE_SESSION_RETRIEVE_FD`` when the device could be bound to any
 * driver.
 *
 * It is currently userspace's responsibility to ensure that the device is bound
 * to the correct driver in this window.
 */

#include <linux/bsearch.h>
#include <linux/io.h>
#include <linux/iommu.h>
#include <linux/kexec_handover.h>
#include <linux/kho/abi/pci.h>
#include <linux/liveupdate.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/sort.h>

#include "pci.h"

#ifndef cmp_int
#define cmp_int(l, r) (((l) > (r)) - ((l) < (r)))
#endif

static DEFINE_MUTEX(pci_flb_outgoing_lock);

static int pci_flb_preserve(struct liveupdate_flb_op_args *args)
{
	struct pci_dev *dev = NULL;
	int max_nr_devices = 0;
	struct pci_ser *ser;
	unsigned long size;

	/*
	 * Don't both accounting for VFs that could be created after this
	 * since preserving VFs is not supported yet. Also don't account
	 * for devices that could be hot-plugged after this since preserving
	 * hot-plugged devices across Live Update is not yet an expected
	 * use-case.
	 */
	for_each_pci_dev(dev)
		max_nr_devices++;

	size = struct_size_t(struct pci_ser, devices, max_nr_devices);

	ser = kho_alloc_preserve(size);
	if (IS_ERR(ser))
		return PTR_ERR(ser);

	ser->max_nr_devices = max_nr_devices;

	args->obj = ser;
	args->data = virt_to_phys(ser);
	return 0;
}

static void pci_flb_unpreserve(struct liveupdate_flb_op_args *args)
{
	struct pci_ser *ser = args->obj;

	WARN_ON_ONCE(ser->nr_devices);
	kho_unpreserve_free(ser);
}

static int pci_flb_retrieve(struct liveupdate_flb_op_args *args)
{
	args->obj = phys_to_virt(args->data);
	return 0;
}

static void pci_flb_finish(struct liveupdate_flb_op_args *args)
{
	kho_restore_free(args->obj);
}

static struct liveupdate_flb_ops pci_liveupdate_flb_ops = {
	.preserve = pci_flb_preserve,
	.unpreserve = pci_flb_unpreserve,
	.retrieve = pci_flb_retrieve,
	.finish = pci_flb_finish,
	.owner = THIS_MODULE,
};

static struct liveupdate_flb pci_liveupdate_flb = {
	.ops = &pci_liveupdate_flb_ops,
	.compatible = PCI_LUO_FLB_COMPATIBLE,
};

#define INIT_PCI_DEV_SER(_dev) {		\
	.domain = pci_domain_nr((_dev)->bus),	\
	.bdf = pci_dev_id(_dev),		\
}

static int pci_dev_ser_cmp(const void *__a, const void *__b)
{
	const struct pci_dev_ser *a = __a, *b = __b;

	return cmp_int((u64)a->domain << 16 | a->bdf,
		       (u64)b->domain << 16 | b->bdf);
}

static struct pci_dev_ser *pci_ser_find(struct pci_ser *ser,
					struct pci_dev *dev)
{
	const struct pci_dev_ser key = INIT_PCI_DEV_SER(dev);

	return bsearch(&key, ser->devices, ser->nr_devices,
		       sizeof(key), pci_dev_ser_cmp);
}

static void pci_ser_delete(struct pci_ser *ser, struct pci_dev *dev)
{
	struct pci_dev_ser *dev_ser;
	int i;

	dev_ser = pci_ser_find(ser, dev);

	/*
	 * This should never happen unless there is a kernel bug or
	 * corruption that causes the state in struct pci_ser to get
	 * out of sync with struct pci_dev.
	 */
	if (pci_WARN_ONCE(dev, !dev_ser, "Cannot find preserved device!"))
		return;

	for (i = dev_ser - ser->devices; i < ser->nr_devices - 1; i++)
		ser->devices[i] = ser->devices[i + 1];

	ser->nr_devices--;
}

static int count_devices(struct device *dev, void *__nr_devices)
{
	(*(int *)__nr_devices)++;
	return 0;
}

static int pci_liveupdate_validate_iommu_group(struct pci_dev *dev)
{
	struct iommu_group *group;
	int nr_devices = 0;

	group = iommu_group_get(&dev->dev);
	if (group) {
		iommu_group_for_each_dev(group, &nr_devices, count_devices);
		iommu_group_put(group);
	}

	if (nr_devices != 1) {
		pci_warn(dev, "Live Update preserved devices must be in singleton iommu groups!");
		return -EINVAL;
	}

	return 0;
}

int pci_liveupdate_preserve(struct pci_dev *dev)
{
	struct pci_dev_ser new = INIT_PCI_DEV_SER(dev);
	struct pci_ser *ser;
	int i, ret;

	/* SR-IOV is not supported yet. */
	if (dev->is_virtfn || (dev->is_physfn && pci_num_vf(dev) > 0))
		return -EINVAL;

	ret = pci_liveupdate_validate_iommu_group(dev);
	if (ret)
		return ret;

	guard(mutex)(&pci_flb_outgoing_lock);

	if (dev->liveupdate_outgoing)
		return -EBUSY;

	ret = liveupdate_flb_get_outgoing(&pci_liveupdate_flb, (void **)&ser);
	if (ret)
		return ret;

	if (ser->nr_devices == ser->max_nr_devices)
		return -E2BIG;

	for (i = ser->nr_devices; i > 0; i--) {
		struct pci_dev_ser *prev = &ser->devices[i - 1];
		int cmp = pci_dev_ser_cmp(&new, prev);

		/*
		 * This should never happen unless there is a kernel bug or
		 * corruption that causes the state in struct pci_ser to get out
		 * of sync with struct pci_dev.
		 */
		if (WARN_ON_ONCE(!cmp))
			return -EBUSY;

		if (cmp > 0)
			break;

		ser->devices[i] = *prev;
	}

	ser->devices[i] = new;
	ser->nr_devices++;
	dev->liveupdate_outgoing = true;
	return 0;
}
EXPORT_SYMBOL_GPL(pci_liveupdate_preserve);

void pci_liveupdate_unpreserve(struct pci_dev *dev)
{
	struct pci_ser *ser;
	int ret;

	/* This should never happen unless the caller (driver) is buggy */
	if (WARN_ON_ONCE(!dev->liveupdate_outgoing))
		return;

	guard(mutex)(&pci_flb_outgoing_lock);

	ret = liveupdate_flb_get_outgoing(&pci_liveupdate_flb, (void **)&ser);

	/* This should never happen unless there is a bug in LUO */
	if (WARN_ON_ONCE(ret))
		return;

	pci_ser_delete(ser, dev);
	dev->liveupdate_outgoing = false;
}
EXPORT_SYMBOL_GPL(pci_liveupdate_unpreserve);

static int pci_liveupdate_flb_get_incoming(struct pci_ser **serp)
{
	int ret;

	ret = liveupdate_flb_get_incoming(&pci_liveupdate_flb, (void **)serp);

	/* Live Update is not enabled. */
	if (ret == -EOPNOTSUPP)
		return ret;

	/* Live Update is enabled, but there is no incoming FLB data. */
	if (ret == -ENODATA)
		return ret;

	/*
	 * Live Update is enabled and there is incoming FLB data, but none of it
	 * matches pci_liveupdate_flb.compatible.
	 *
	 * This could mean that no PCI FLB data was passed by the previous
	 * kernel, but it could also mean the previous kernel used a different
	 * compatibility string (i.e.a different ABI). The latter deserves at
	 * least a WARN_ON_ONCE() but it cannot be distinguished from the
	 * former.
	 */
	if (ret == -ENOENT) {
		pr_info_once("PCI: No incoming FLB data detected during Live Update");
		return ret;
	}

	/*
	 * There is incoming FLB data that matches pci_liveupdate_flb.compatible
	 * but it cannot be retrieved. Proceed with standard initialization as
	 * if there was not incoming PCI FLB data.
	 */
	WARN_ONCE(ret, "PCI: Failed to retrieve incoming FLB data during Live Update");
	return ret;
}

u32 pci_liveupdate_incoming_nr_devices(void)
{
	struct pci_ser *ser;

	if (pci_liveupdate_flb_get_incoming(&ser))
		return 0;

	return ser->nr_devices;
}

void pci_liveupdate_setup_device(struct pci_dev *dev)
{
	struct pci_ser *ser;

	if (pci_liveupdate_flb_get_incoming(&ser))
		return;

	if (!pci_ser_find(ser, dev))
		return;

	dev->liveupdate_incoming = true;
}

int pci_liveupdate_retrieve(struct pci_dev *dev)
{
	if (!dev->liveupdate_incoming)
		return -EINVAL;

	return pci_liveupdate_validate_iommu_group(dev);
}
EXPORT_SYMBOL_GPL(pci_liveupdate_retrieve);

void pci_liveupdate_finish(struct pci_dev *dev)
{
	dev->liveupdate_incoming = false;
}
EXPORT_SYMBOL_GPL(pci_liveupdate_finish);

int pci_liveupdate_register_flb(struct liveupdate_file_handler *fh)
{
	return liveupdate_register_flb(fh, &pci_liveupdate_flb);
}
EXPORT_SYMBOL_GPL(pci_liveupdate_register_flb);

void pci_liveupdate_unregister_flb(struct liveupdate_file_handler *fh)
{
	liveupdate_unregister_flb(fh, &pci_liveupdate_flb);
}
EXPORT_SYMBOL_GPL(pci_liveupdate_unregister_flb);
