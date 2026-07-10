/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Copyright (C) 2026, Google LLC
 * Author: Samiullah Khawaja <skhawaja@google.com>
 */

#ifndef _LINUX_IOMMU_LIVEUPDATE_H
#define _LINUX_IOMMU_LIVEUPDATE_H

#include <linux/device.h>
#include <linux/iommu.h>
#include <linux/liveupdate.h>
#include <linux/kho/abi/iommu.h>

typedef int (*iommu_preserved_device_iter_fn)(struct iommu_device_ser *ser,
					      void *arg);
#ifdef CONFIG_IOMMU_LIVEUPDATE
static inline void *dev_iommu_preserved_state(struct device *dev)
{
	struct iommu_device_ser *ser;

	if (!dev->iommu)
		return NULL;

	ser = dev->iommu->device_ser;
	if (ser && !ser->hdr.incoming)
		return ser;

	return NULL;
}

static inline void *dev_iommu_restored_state(struct device *dev)
{
	struct iommu_device_ser *ser;

	if (!dev->iommu)
		return NULL;

	ser = dev->iommu->device_ser;
	if (ser && ser->hdr.incoming)
		return ser;

	return NULL;
}

static inline void *iommu_domain_restored_state(struct iommu_domain *domain)
{
	struct iommu_domain_ser *ser;

	ser = domain->preserved_state;
	if (ser && ser->hdr.incoming)
		return ser;

	return NULL;
}

static inline int dev_iommu_restore_did(struct device *dev, struct iommu_domain *domain)
{
	struct iommu_device_ser *ser = dev_iommu_restored_state(dev);

	if (ser && iommu_domain_restored_state(domain))
		return ser->domain_iommu_ser.attachment_id;

	return -1;
}

struct iommu_domain *iommu_restore_domain(struct device *dev,
					  struct iommu_device_ser *ser,
					  void **owner);
int iommu_for_each_preserved_device(iommu_preserved_device_iter_fn fn,
				    void *arg);
struct iommu_device_ser *iommu_get_device_preserved_data(struct device *dev);
struct iommu_hw_ser *iommu_get_preserved_data(u64 token, enum iommu_type_ser type);
int iommu_domain_preserve(struct iommu_domain *domain, struct iommu_domain_ser **ser);
void iommu_domain_unpreserve(struct iommu_domain *domain);
int iommu_preserve_device(struct iommu_domain *domain,
			  struct device *dev, u64 *preserved_state);
void iommu_unpreserve_device(struct iommu_domain *domain, struct device *dev);

static inline void *iommu_preserved_state(struct iommu_device *iommu)
{
	return iommu->outgoing_preserved_state;
}
#else
static inline void *dev_iommu_preserved_state(struct device *dev)
{
	return NULL;
}

static inline void *dev_iommu_restored_state(struct device *dev)
{
	return NULL;
}

static inline int dev_iommu_restore_did(struct device *dev, struct iommu_domain *domain)
{
	return -1;
}

static inline void *iommu_domain_restored_state(struct iommu_domain *domain)
{
	return NULL;
}

static inline struct iommu_domain *iommu_restore_domain(struct device *dev,
							struct iommu_device_ser *ser,
							void **owner)
{
	return NULL;
}

static inline int iommu_for_each_preserved_device(iommu_preserved_device_iter_fn fn, void *arg)
{
	return -EOPNOTSUPP;
}

static inline struct iommu_device_ser *iommu_get_device_preserved_data(struct device *dev)
{
	return NULL;
}

static inline struct iommu_hw_ser *iommu_get_preserved_data(u64 token, enum iommu_type_ser type)
{
	return NULL;
}

static inline int iommu_domain_preserve(struct iommu_domain *domain, struct iommu_domain_ser **ser)
{
	return -EOPNOTSUPP;
}

static inline void iommu_domain_unpreserve(struct iommu_domain *domain)
{
}

static inline int iommu_preserve_device(struct iommu_domain *domain,
					struct device *dev, u64 *preserved_state)
{
	return -EOPNOTSUPP;
}

static inline void iommu_unpreserve_device(struct iommu_domain *domain, struct device *dev)
{
}

static inline void *iommu_preserved_state(struct iommu_device *iommu)
{
	return NULL;
}
#endif

int iommu_liveupdate_register_flb(struct liveupdate_file_handler *handler);
void iommu_liveupdate_unregister_flb(struct liveupdate_file_handler *handler);

#endif /* _LINUX_IOMMU_LIVEUPDATE_H */
