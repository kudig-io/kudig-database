// SPDX-License-Identifier: GPL-2.0-only

/*
 * Copyright (C) 2026, Google LLC
 * Author: Samiullah Khawaja <skhawaja@google.com>
 */

#define pr_fmt(fmt)    "DMAR: liveupdate: " fmt

#include <linux/kexec_handover.h>
#include <linux/liveupdate.h>
#include <linux/iommu-liveupdate.h>
#include <linux/module.h>
#include <linux/pci.h>

#include "iommu.h"
#include "pasid.h"
#include "../iommu-pages.h"

static void unpreserve_iommu_context_table(struct intel_iommu *iommu, int end)
{
	struct context_entry *context;
	int i;

	for (i = 0; i < end; i++) {
		context = iommu_context_addr(iommu, i, 0, 0);
		if (context)
			iommu_unpreserve_page(context);

		if (!sm_supported(iommu))
			continue;

		context = iommu_context_addr(iommu, i, 0x80, 0);
		if (context)
			iommu_unpreserve_page(context);
	}
}

static int preserve_iommu_context_table(struct intel_iommu *iommu)
{
	struct context_entry *context;
	int ret;
	int i;

	for (i = 0; i < ROOT_ENTRY_NR; i++) {
		/*
		 * Alloc the context tables now to make sure the iommu unit is
		 * properly preserved. These might stay unused and wastes around
		 * 32MB max in scalable mode.
		 */
		spin_lock(&iommu->lock);
		context = iommu_context_addr(iommu, i, 0, 1);
		spin_unlock(&iommu->lock);
		if (!context) {
			ret = -ENOMEM;
			goto error;
		}
		ret = iommu_preserve_page(context);
		if (ret)
			goto error;

		if (!sm_supported(iommu))
			continue;

		spin_lock(&iommu->lock);
		context = iommu_context_addr(iommu, i, 0x80, 1);
		spin_unlock(&iommu->lock);
		if (!context) {
			ret = -ENOMEM;
			goto error_sm;
		}
		ret = iommu_preserve_page(context);
		if (ret)
			goto error_sm;
	}

	return 0;

error_sm:
	context = iommu_context_addr(iommu, i, 0, 0);
	iommu_unpreserve_page(context);
error:
	unpreserve_iommu_context_table(iommu, i);
	return ret;
}

static void restore_iommu_context(struct intel_iommu *iommu)
{
	struct context_entry *context;
	int i;

	for (i = 0; i < ROOT_ENTRY_NR; i++) {
		context = iommu_context_addr(iommu, i, 0, 0);
		if (context)
			BUG_ON(!kho_restore_folio(virt_to_phys(context)));

		if (!sm_supported(iommu))
			continue;

		context = iommu_context_addr(iommu, i, 0x80, 0);
		if (context)
			BUG_ON(!kho_restore_folio(virt_to_phys(context)));
	}
}

static int _restore_used_domain_ids(struct iommu_device_ser *ser, void *arg)
{
	int id = ser->domain_iommu_ser.attachment_id;
	struct iommu_hw_ser *iommu_hw_ser;
	struct intel_iommu *iommu = arg;

	iommu_hw_ser = phys_to_virt(ser->domain_iommu_ser.iommu_phys);
	if (iommu_hw_ser->type != IOMMU_INTEL)
		return 0;

	/* Only allocate domain ID from associated IOMMU HW unit */
	if (iommu_hw_ser->intel.phys_addr != iommu->reg_phys)
		return 0;

	/*
	 * This can fail as multiple preserved devices can share the same domain
	 * ID. Since this is done during DMAR init so these failures can be
	 * ignored.
	 */
	ida_alloc_range(&iommu->domain_ida, id, id, GFP_ATOMIC);
	return 0;
}

void intel_iommu_liveupdate_restore_root_table(struct intel_iommu *iommu,
					       struct iommu_hw_ser *iommu_ser)
{
	if (!iommu_ser->intel.restored)
		BUG_ON(!kho_restore_folio(iommu_ser->intel.root_table));

	iommu->root_entry = __va(iommu_ser->intel.root_table);

	if (!iommu_ser->intel.restored)
		restore_iommu_context(iommu);

	iommu_ser->intel.restored = 1;
	iommu_for_each_preserved_device(_restore_used_domain_ids, iommu);
}

enum pasid_lu_op {
	PASID_LU_OP_PRESERVE = 1,
	PASID_LU_OP_UNPRESERVE,
	PASID_LU_OP_RESTORE,
	PASID_LU_OP_FREE,
};

static int pasid_lu_do_op(void *table, enum pasid_lu_op op)
{
	int ret = 0;

	switch (op) {
	case PASID_LU_OP_PRESERVE:
		ret = iommu_preserve_page(table);
		break;
	case PASID_LU_OP_UNPRESERVE:
		iommu_unpreserve_page(table);
		break;
	case PASID_LU_OP_RESTORE:
		iommu_restore_page(virt_to_phys(table));
		break;
	case PASID_LU_OP_FREE:
		iommu_free_pages(table);
		break;
	}

	return ret;
}

static int pasid_lu_handle_pd(struct pasid_dir_entry *dir, enum pasid_lu_op op)
{
	struct pasid_entry *table;
	int ret;

	/* Only preserve first table for NO_PASID. */
	table = get_pasid_table_from_pde(&dir[0]);
	if (!table)
		return -EINVAL;

	ret = pasid_lu_do_op(table, op);
	if (ret)
		return ret;

	ret = pasid_lu_do_op(dir, op);
	if (ret)
		goto err;

	return 0;
err:
	if (op == PASID_LU_OP_PRESERVE)
		pasid_lu_do_op(table, PASID_LU_OP_UNPRESERVE);

	return ret;
}

void pasid_cleanup_preserved_table(struct device *dev)
{
	struct pasid_table *pasid_table;
	struct pasid_dir_entry *dir;
	struct pasid_entry *table;
	size_t dir_size;

	pasid_table = intel_pasid_get_table(dev);
	if (!pasid_table)
		return;

	dir = pasid_table->table;
	table = get_pasid_table_from_pde(&dir[0]);
	if (!table)
		return;

	/* Clear everything except the first entry in table. */
	memset(&table[1], 0, SZ_4K - sizeof(*table));

	/* Use the folio order to calculate the size of Pasid Directory */
	dir_size = (1 << (folio_order(virt_to_folio(dir)) + PAGE_SHIFT));

	/* Clear everything except the first entry in directory */
	memset(&dir[1], 0, dir_size - sizeof(struct pasid_dir_entry));

	clflush_cache_range(&table[0], SZ_4K);
	clflush_cache_range(&dir[0], dir_size);
}

int intel_iommu_preserve_device(struct device *dev,
				struct iommu_device_ser *device_ser)
{
	struct device_domain_info *info = dev_iommu_priv_get(dev);
	struct pasid_table *pasid_table;
	int ret;

	if (!dev_is_pci(dev)) {
		dev_err(dev, "Cannot preserve non-PCI device\n");
		return -EOPNOTSUPP;
	}

	if (!info)
		return -EINVAL;

	device_ser->domain_iommu_ser.attachment_id = domain_id_iommu(info->domain,
								     info->iommu);

	if (!sm_supported(info->iommu))
		return 0;

	pasid_table = intel_pasid_get_table(dev);
	if (!pasid_table)
		return -EINVAL;

	ret = pasid_lu_handle_pd(pasid_table->table, PASID_LU_OP_PRESERVE);
	if (ret)
		return ret;

	device_ser->intel.pasid_table = virt_to_phys(pasid_table->table);
	device_ser->intel.max_pasid = pasid_table->max_pasid;
	return 0;
}

void intel_iommu_unpreserve_device(struct device *dev,
				   struct iommu_device_ser *device_ser)
{
	struct device_domain_info *info = dev_iommu_priv_get(dev);
	struct pasid_table *pasid_table;

	if (!dev_is_pci(dev))
		return;

	if (!info)
		return;

	if (!sm_supported(info->iommu))
		return;

	pasid_table = intel_pasid_get_table(dev);
	if (!pasid_table)
		return;

	pasid_lu_handle_pd(pasid_table->table, PASID_LU_OP_UNPRESERVE);
}

int intel_iommu_preserve(struct iommu_device *iommu_dev,
			 struct iommu_hw_ser *ser)
{
	struct intel_iommu *iommu;
	int ret;

	iommu = container_of(iommu_dev, struct intel_iommu, iommu);

	ret = preserve_iommu_context_table(iommu);
	if (ret)
		return ret;

	ret = iommu_preserve_page(iommu->root_entry);
	if (ret) {
		unpreserve_iommu_context_table(iommu, ROOT_ENTRY_NR);
		return ret;
	}

	ser->intel.phys_addr = iommu->reg_phys;
	ser->intel.root_table = __pa(iommu->root_entry);
	ser->type = IOMMU_INTEL;
	ser->token = ser->intel.phys_addr;

	return 0;
}

void intel_iommu_unpreserve(struct iommu_device *iommu_dev,
			    struct iommu_hw_ser *iommu_ser)
{
	struct intel_iommu *iommu;

	iommu = container_of(iommu_dev, struct intel_iommu, iommu);

	unpreserve_iommu_context_table(iommu, ROOT_ENTRY_NR);
	iommu_unpreserve_page(iommu->root_entry);
}

void *intel_pasid_try_restore_table(struct device *dev, u64 max_pasid)
{
	struct iommu_device_ser *ser = dev_iommu_restored_state(dev);

	if (!ser)
		return NULL;

	BUG_ON(pasid_lu_handle_pd(phys_to_virt(ser->intel.pasid_table),
				  PASID_LU_OP_RESTORE));
	if (WARN_ON_ONCE(ser->intel.max_pasid != max_pasid)) {
		pasid_lu_handle_pd(phys_to_virt(ser->intel.pasid_table),
				   PASID_LU_OP_FREE);
		return NULL;
	}

	return phys_to_virt(ser->intel.pasid_table);
}
