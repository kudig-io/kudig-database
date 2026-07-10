// SPDX-License-Identifier: GPL-2.0-only

/*
 * Copyright (C) 2026, Google LLC
 * Author: Samiullah Khawaja <skhawaja@google.com>
 */

#define pr_fmt(fmt)    "iommu: liveupdate: " fmt

#include <linux/kexec_handover.h>
#include <linux/liveupdate.h>
#include <linux/iommu-liveupdate.h>
#include <linux/iommu.h>
#include <linux/pci.h>
#include <linux/errno.h>

#define iommu_max_objs_per_page(_array) \
	((PAGE_SIZE - sizeof(struct iommu_array_hdr_ser)) / sizeof((_array)->objects[0]))

#define iommu_liveupdate_for_each_obj(_arr, _obj, _idx)				\
	for (; (_arr);								\
	     (_arr) = (_arr)->hdr.next_array_phys ?				\
	     phys_to_virt((_arr)->hdr.next_array_phys) : NULL)			\
		for ((_idx) = 0, (_obj) = (_arr)->objects;			\
		     (_idx) < (_arr)->hdr.nr_objects; (_idx)++, (_obj)++)	\
			if (!(_obj)->hdr.deleted)

static void *iommu_liveupdate_restore_array(u64 array_phys)
{
	struct iommu_array_hdr_ser *array_hdr;
	void *vaddr = array_phys ? phys_to_virt(array_phys) : NULL;

	while (array_phys) {
		/*
		 * Failure to restore preserved IOMMU state is considered fatal.
		 *
		 * This is because the IOMMU translations for preserved IOMMUs
		 * were kept enabled in the previous kernel and the preserved
		 * devices have their IOMMU domains still present. Not being
		 * able to restore means that the memory mapped into preserved
		 * domains might be already corrupted by the preserved devices.
		 *
		 * There is no way to confirm the integrity of the memory that
		 * was mapped. BUG_ON is the safest option at this point.
		 */
		BUG_ON(!kho_restore_folio(array_phys));
		array_hdr = phys_to_virt(array_phys);
		array_phys = array_hdr->next_array_phys;
	}

	return vaddr;
}

static void iommu_liveupdate_unpreserve_free(u64 array_phys)
{
	struct iommu_array_hdr_ser *array_hdr;

	while (array_phys) {
		array_hdr = phys_to_virt(array_phys);
		array_phys = array_hdr->next_array_phys;
		kho_unpreserve_free(array_hdr);
	}
}

static void iommu_liveupdate_folio_put(u64 array_phys)
{
	struct iommu_array_hdr_ser *array_hdr;

	while (array_phys) {
		array_hdr = phys_to_virt(array_phys);
		array_phys = array_hdr->next_array_phys;
		folio_put(virt_to_folio(array_hdr));
	}
}

static void iommu_liveupdate_flb_free(struct iommu_flb_obj *obj)
{
	if (obj->ser->iommu_domain_array_phys)
		iommu_liveupdate_unpreserve_free(obj->ser->iommu_domain_array_phys);

	if (obj->ser->device_array_phys)
		iommu_liveupdate_unpreserve_free(obj->ser->device_array_phys);

	if (obj->ser->iommu_array_phys)
		iommu_liveupdate_unpreserve_free(obj->ser->iommu_array_phys);

	kho_unpreserve_free(obj->ser);
	kfree(obj);
}

static int iommu_liveupdate_flb_preserve(struct liveupdate_flb_op_args *argp)
{
	struct iommu_flb_obj *obj;
	struct iommu_flb_ser *ser;
	void *mem;

	/* obj exists only in the current kernel to track preserved state */
	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj)
		return -ENOMEM;

	mutex_init(&obj->lock);

	/* mem is allocated via KHO and will survive the kexec */
	mem = kho_alloc_preserve(sizeof(*ser));
	if (IS_ERR(mem))
		goto err_free_obj;

	ser = mem;
	obj->ser = ser;

	mem = kho_alloc_preserve(PAGE_SIZE);
	if (IS_ERR(mem))
		goto err_free_ser;

	obj->curr_domain_array = mem;
	ser->iommu_domain_array_phys = virt_to_phys(obj->curr_domain_array);

	mem = kho_alloc_preserve(PAGE_SIZE);
	if (IS_ERR(mem))
		goto err_free_domains;

	obj->curr_device_array = mem;
	ser->device_array_phys = virt_to_phys(obj->curr_device_array);

	mem = kho_alloc_preserve(PAGE_SIZE);
	if (IS_ERR(mem))
		goto err_free_devices;

	obj->curr_iommu_array = mem;
	ser->iommu_array_phys = virt_to_phys(obj->curr_iommu_array);

	argp->obj = obj;
	argp->data = virt_to_phys(ser);
	return 0;

err_free_devices:
	kho_unpreserve_free(obj->curr_device_array);
err_free_domains:
	kho_unpreserve_free(obj->curr_domain_array);
err_free_ser:
	kho_unpreserve_free(obj->ser);
err_free_obj:
	kfree(obj);
	return PTR_ERR(mem);
}

static void iommu_liveupdate_flb_unpreserve(struct liveupdate_flb_op_args *argp)
{
	iommu_liveupdate_flb_free(argp->obj);
}

static void iommu_liveupdate_flb_finish(struct liveupdate_flb_op_args *argp)
{
	struct iommu_flb_obj *obj = argp->obj;

	iommu_liveupdate_folio_put(obj->ser->iommu_domain_array_phys);
	iommu_liveupdate_folio_put(obj->ser->device_array_phys);
	iommu_liveupdate_folio_put(obj->ser->iommu_array_phys);

	folio_put(virt_to_folio(obj->ser));
	kfree(obj);
}

static int iommu_liveupdate_flb_retrieve(struct liveupdate_flb_op_args *argp)
{
	struct iommu_flb_obj *obj;
	struct iommu_flb_ser *ser;

	obj = kzalloc(sizeof(*obj), GFP_KERNEL);
	if (!obj)
		return -ENOMEM;

	/* Data must be present and valid from the previous kernel */
	BUG_ON(!kho_restore_folio(argp->data));

	mutex_init(&obj->lock);
	ser = phys_to_virt(argp->data);
	obj->ser = ser;

	obj->curr_domain_array = iommu_liveupdate_restore_array(ser->iommu_domain_array_phys);
	obj->curr_device_array = iommu_liveupdate_restore_array(ser->device_array_phys);
	obj->curr_iommu_array = iommu_liveupdate_restore_array(ser->iommu_array_phys);
	argp->obj = obj;
	return 0;
}

static struct liveupdate_flb_ops iommu_flb_ops = {
	.preserve = iommu_liveupdate_flb_preserve,
	.unpreserve = iommu_liveupdate_flb_unpreserve,
	.finish = iommu_liveupdate_flb_finish,
	.retrieve = iommu_liveupdate_flb_retrieve,
};

static struct liveupdate_flb iommu_flb = {
	.compatible = IOMMU_LUO_FLB_COMPATIBLE,
	.ops = &iommu_flb_ops,
};

int iommu_liveupdate_register_flb(struct liveupdate_file_handler *handler)
{
	return liveupdate_register_flb(handler, &iommu_flb);
}
EXPORT_SYMBOL(iommu_liveupdate_register_flb);

void iommu_liveupdate_unregister_flb(struct liveupdate_file_handler *handler)
{
	liveupdate_unregister_flb(handler, &iommu_flb);
}
EXPORT_SYMBOL(iommu_liveupdate_unregister_flb);

int iommu_for_each_preserved_device(iommu_preserved_device_iter_fn fn,
				    void *arg)
{
	struct iommu_flb_obj *flb_obj;
	struct iommu_device_array_ser *array;
	struct iommu_device_ser *device_ser;
	int ret, idx;

	ret = liveupdate_flb_get_incoming(&iommu_flb, (void **)&flb_obj);
	if (ret)
		return -ENOENT;

	array = phys_to_virt(flb_obj->ser->device_array_phys);
	iommu_liveupdate_for_each_obj(array, device_ser, idx) {
		ret = fn(device_ser, arg);
		if (ret)
			goto out;
	}

out:
	liveupdate_flb_put_incoming(&iommu_flb);
	return ret;
}
EXPORT_SYMBOL(iommu_for_each_preserved_device);

static inline bool match_device_ser(struct iommu_device_ser *match,
				    struct pci_dev *pdev)
{
	return match->devid == pci_dev_id(pdev) && match->pci_domain_nr == pci_domain_nr(pdev->bus);
}

struct iommu_device_ser *iommu_get_device_preserved_data(struct device *dev)
{
	struct iommu_device_ser *device_ser = NULL;
	struct iommu_device_array_ser *array;
	struct iommu_flb_obj *flb_obj;
	int ret, idx;

	if (!dev_is_pci(dev))
		return NULL;

	ret = liveupdate_flb_get_incoming(&iommu_flb, (void **)&flb_obj);
	if (ret)
		return NULL;

	array = phys_to_virt(flb_obj->ser->device_array_phys);
	iommu_liveupdate_for_each_obj(array, device_ser, idx) {
		if (match_device_ser(device_ser, to_pci_dev(dev))) {
			device_ser->hdr.incoming = true;
			goto out;
		}
	}

	device_ser = NULL;
out:
	liveupdate_flb_put_incoming(&iommu_flb);
	return device_ser;
}
EXPORT_SYMBOL(iommu_get_device_preserved_data);

struct iommu_hw_ser *iommu_get_preserved_data(u64 token, enum iommu_type_ser type)
{
	struct iommu_hw_ser *iommu_ser = NULL;
	struct iommu_hw_array_ser *array;
	struct iommu_flb_obj *flb_obj;
	int ret, idx;

	ret = liveupdate_flb_get_incoming(&iommu_flb, (void **)&flb_obj);
	if (ret)
		return NULL;

	array = phys_to_virt(flb_obj->ser->iommu_array_phys);
	iommu_liveupdate_for_each_obj(array, iommu_ser, idx) {
		if (iommu_ser->token == token && iommu_ser->type == type)
			goto out;
	}

	iommu_ser = NULL;
out:
	liveupdate_flb_put_incoming(&iommu_flb);
	return iommu_ser;
}
EXPORT_SYMBOL(iommu_get_preserved_data);

static int alloc_object_ser(struct iommu_array_hdr_ser **curr_array_ptr, u64 max_objs)
{
	struct iommu_array_hdr_ser *curr_array = *curr_array_ptr;
	struct iommu_array_hdr_ser *next_array;

	if (curr_array->nr_objects >= max_objs) {
		next_array = kho_alloc_preserve(PAGE_SIZE);
		if (IS_ERR(next_array))
			return PTR_ERR(next_array);

		curr_array->next_array_phys = virt_to_phys(next_array);
		*curr_array_ptr = next_array;
		curr_array = next_array;
	}

	return curr_array->nr_objects++;
}

static struct iommu_domain_ser *alloc_iommu_domain_ser(struct iommu_flb_obj *flb)
{
	int idx;

	idx = alloc_object_ser((struct iommu_array_hdr_ser **)&flb->curr_domain_array,
			       iommu_max_objs_per_page(flb->curr_domain_array));
	if (idx < 0)
		return ERR_PTR(idx);

	flb->curr_domain_array->objects[idx].hdr.ref_count = 1;
	return &flb->curr_domain_array->objects[idx];
}

int iommu_domain_preserve(struct iommu_domain *domain, struct iommu_domain_ser **ser)
{
	struct iommu_domain_ser *domain_ser;
	struct iommu_flb_obj *flb_obj;
	int ret;

	if (!domain->ops->preserve)
		return -EOPNOTSUPP;

	ret = liveupdate_flb_get_outgoing(&iommu_flb, (void **)&flb_obj);
	if (ret)
		return ret;

	guard(mutex)(&flb_obj->lock);
	domain_ser = alloc_iommu_domain_ser(flb_obj);
	if (IS_ERR(domain_ser))
		return PTR_ERR(domain_ser);

	ret = domain->ops->preserve(domain, domain_ser);
	if (ret) {
		domain_ser->hdr.deleted = true;
		return ret;
	}

	domain->preserved_state = domain_ser;
	*ser = domain_ser;
	return 0;
}
EXPORT_SYMBOL_GPL(iommu_domain_preserve);

void iommu_domain_unpreserve(struct iommu_domain *domain)
{
	struct iommu_domain_ser *domain_ser;
	struct iommu_flb_obj *flb_obj;
	int ret;

	if (!domain->ops->unpreserve)
		return;

	ret = liveupdate_flb_get_outgoing(&iommu_flb, (void **)&flb_obj);
	if (WARN_ON(ret))
		return;

	guard(mutex)(&flb_obj->lock);

	if (!domain->preserved_state)
		return;

	/*
	 * There is no check for attached devices here. The correctness relies
	 * on the Live Update Orchestrator's session lifecycle. All resources
	 * (iommufd, vfio devices) are preserved within a single session. If the
	 * session is torn down, the .unpreserve callbacks for all files will be
	 * invoked, ensuring a consistent cleanup without needing explicit
	 * refcounting for the serialized objects here.
	 */
	domain_ser = domain->preserved_state;
	domain->ops->unpreserve(domain, domain_ser);
	domain_ser->hdr.deleted = true;
	domain->preserved_state = NULL;
}
EXPORT_SYMBOL_GPL(iommu_domain_unpreserve);

static struct iommu_hw_ser *alloc_iommu_hw_ser(struct iommu_flb_obj *flb)
{
	int idx;

	idx = alloc_object_ser((struct iommu_array_hdr_ser **)&flb->curr_iommu_array,
			       iommu_max_objs_per_page(flb->curr_iommu_array));
	if (idx < 0)
		return ERR_PTR(idx);

	flb->curr_iommu_array->objects[idx].hdr.ref_count = 1;
	return &flb->curr_iommu_array->objects[idx];
}

static int iommu_preserve_locked(struct iommu_device *iommu,
				 struct iommu_flb_obj *flb_obj)
{
	struct iommu_hw_ser *iommu_hw_ser;
	int ret;

	if (!iommu->ops->preserve)
		return -EOPNOTSUPP;

	lockdep_assert_held(&flb_obj->lock);
	if (iommu->outgoing_preserved_state) {
		iommu->outgoing_preserved_state->hdr.ref_count++;
		return 0;
	}

	iommu_hw_ser = alloc_iommu_hw_ser(flb_obj);
	if (IS_ERR(iommu_hw_ser))
		return PTR_ERR(iommu_hw_ser);

	ret = iommu->ops->preserve(iommu, iommu_hw_ser);
	if (ret) {
		iommu_hw_ser->hdr.deleted = true;
		return ret;
	}

	iommu->outgoing_preserved_state = iommu_hw_ser;
	return ret;
}

static void iommu_unpreserve_locked(struct iommu_device *iommu,
				    struct iommu_flb_obj *flb_obj)
{
	struct iommu_hw_ser *iommu_hw_ser = iommu->outgoing_preserved_state;

	lockdep_assert_held(&flb_obj->lock);
	iommu_hw_ser->hdr.ref_count--;
	if (iommu_hw_ser->hdr.ref_count)
		return;

	iommu->outgoing_preserved_state = NULL;
	iommu->ops->unpreserve(iommu, iommu_hw_ser);
	iommu_hw_ser->hdr.deleted = true;
}

static struct iommu_device_ser *alloc_iommu_device_ser(struct iommu_flb_obj *flb)
{
	int idx;

	idx = alloc_object_ser((struct iommu_array_hdr_ser **)&flb->curr_device_array,
			       iommu_max_objs_per_page(flb->curr_device_array));
	if (idx < 0)
		return ERR_PTR(idx);

	flb->curr_device_array->objects[idx].hdr.ref_count = 1;
	return &flb->curr_device_array->objects[idx];
}

int iommu_preserve_device(struct iommu_domain *domain,
			  struct device *dev, u64 *preserved_state)
{
	struct iommu_flb_obj *flb_obj;
	struct iommu_device_ser *device_ser;
	struct dev_iommu *iommu;
	struct pci_dev *pdev;
	int ret;

	if (!dev_is_pci(dev))
		return -EOPNOTSUPP;

	if (!domain->preserved_state)
		return -EINVAL;

	if (!iommu_group_dma_owner_claimed(dev->iommu_group))
		return -EINVAL;

	pdev = to_pci_dev(dev);
	iommu = dev->iommu;
	if (!iommu->iommu_dev->ops->preserve_device ||
	    !iommu->iommu_dev->ops->preserve)
		return -EOPNOTSUPP;

	ret = liveupdate_flb_get_outgoing(&iommu_flb, (void **)&flb_obj);
	if (ret)
		return ret;

	guard(mutex)(&flb_obj->lock);
	device_ser = alloc_iommu_device_ser(flb_obj);
	if (IS_ERR(device_ser))
		return PTR_ERR(device_ser);

	ret = iommu_preserve_locked(iommu->iommu_dev, flb_obj);
	if (ret) {
		device_ser->hdr.deleted = true;
		return ret;
	}

	device_ser->domain_iommu_ser.domain_phys = __pa(domain->preserved_state);
	device_ser->domain_iommu_ser.iommu_phys = __pa(iommu->iommu_dev->outgoing_preserved_state);
	device_ser->devid = pci_dev_id(pdev);
	device_ser->pci_domain_nr = pci_domain_nr(pdev->bus);

	ret = iommu->iommu_dev->ops->preserve_device(dev, device_ser);
	if (ret) {
		device_ser->hdr.deleted = true;
		iommu_unpreserve_locked(iommu->iommu_dev, flb_obj);
		return ret;
	}

	dev->iommu->device_ser = device_ser;
	*preserved_state = virt_to_phys(device_ser);
	return 0;
}
EXPORT_SYMBOL_GPL(iommu_preserve_device);

void iommu_unpreserve_device(struct iommu_domain *domain, struct device *dev)
{
	struct iommu_flb_obj *flb_obj;
	struct iommu_device_ser *iommu_device_ser;
	struct dev_iommu *iommu;
	struct pci_dev *pdev;
	int ret;

	if (!dev_is_pci(dev))
		return;

	if (!iommu_group_dma_owner_claimed(dev->iommu_group))
		return;

	pdev = to_pci_dev(dev);
	iommu = dev->iommu;
	if (!iommu->iommu_dev->ops->unpreserve_device ||
	    !iommu->iommu_dev->ops->unpreserve)
		return;

	ret = liveupdate_flb_get_outgoing(&iommu_flb, (void **)&flb_obj);
	if (WARN_ON(ret))
		return;

	guard(mutex)(&flb_obj->lock);
	iommu_device_ser = dev_iommu_preserved_state(dev);
	if (WARN_ON(!iommu_device_ser))
		return;

	iommu->iommu_dev->ops->unpreserve_device(dev, iommu_device_ser);
	dev->iommu->device_ser = NULL;

	iommu_unpreserve_locked(iommu->iommu_dev, flb_obj);
}
EXPORT_SYMBOL_GPL(iommu_unpreserve_device);

struct iommu_domain *iommu_restore_domain(struct device *dev,
					  struct iommu_device_ser *ser,
					  void **owner)
{
	struct iommu_domain_ser *domain_ser;
	struct iommu_flb_obj *flb_obj;
	struct iommu_domain *domain;
	int ret;

	domain_ser = phys_to_virt(ser->domain_iommu_ser.domain_phys);

	ret = liveupdate_flb_get_incoming(&iommu_flb, (void **)&flb_obj);
	if (ret)
		return ERR_PTR(ret);

	guard(mutex)(&flb_obj->lock);
	if (domain_ser->restored_domain) {
		domain = domain_ser->restored_domain;
		goto out;
	}

	domain_ser->hdr.incoming =  true;
	domain = iommu_paging_domain_alloc(dev);
	if (IS_ERR(domain))
		goto out;

	ret = domain->ops->restore(domain, domain_ser);
	if (ret) {
		iommu_domain_free(domain);
		domain = ERR_PTR(ret);
		goto out;
	}

	/* The device is owned by the preserved state. */
	*owner = ser;
	domain->preserved_state = domain_ser;
	domain_ser->restored_domain = domain;

out:
	liveupdate_flb_put_incoming(&iommu_flb);
	return domain;
}
