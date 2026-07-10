/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Copyright (C) 2026, Google LLC
 * Author: Samiullah Khawaja <skhawaja@google.com>
 */

#ifndef _LINUX_KHO_ABI_IOMMU_H
#define _LINUX_KHO_ABI_IOMMU_H

#include <linux/mutex.h>
#include <linux/compiler.h>
#include <linux/types.h>

/**
 * DOC: IOMMU File-Lifecycle Bound (FLB) Live Update ABI
 *
 * This header defines the ABI for preserving IOMMU state across kexec using
 * Live Update File-Lifecycle Bound (FLB) data.
 *
 * This interface is a contract. Any modification to any of the serialization
 * structs defined here constitutes a breaking change. Such changes require
 * incrementing the version number in the IOMMU_LUO_FLB_COMPATIBLE string.
 *
 * Memory Layout of Serialization Structures:
 * ==========================================
 *
 * Each serialized type (IOMMU, Domain, Device) is stored in a linked list of
 * arrays. The first array is allocated initially. When an array is full, a new
 * array is allocated and its physical address is stored in the next_array_phys
 * field of the hdr of the current array.
 *
 * Top Level (struct iommu_flb_ser):
 * +---------------------------+
 * | - iommu_array_phys        |
 * | - iommu_domain_array_phys |
 * | - device_array_phys       |
 * +---------------------------+
 *
 * Each Array contains the serialized objects of the respective type. For
 * example see below the representation of struct iommu_domain_array_ser.
 *
 * +---------------------------+   +---------------------------+
 * | iommu_domain_array_ser    |-->| iommu_domain_array_ser    |--> NULL
 * | - hdr.next_array_phys     |   | - hdr.next_array_phys     |
 * | - hdr.nr_objects          |   | - hdr.nr_objects          |
 * |                           |   |                           |
 * | objects[]:                |   | objects[]:                |
 * | [ iommu_domain_ser ]      |   | [ iommu_domain_ser ]      |
 * | [ iommu_domain_ser ]      |   | [ iommu_domain_ser ]      |
 * | ...                       |   | ...                       |
 * +---------------------------+   +---------------------------+
 *
 * Each object in the array starts with a common header (iommu_hdr_ser).
 * For example, the layout of struct iommu_domain_ser is:
 *
 *   +-----------------------------+
 *   | iommu_domain_ser            |
 *   | +-------------------------+ |
 *   | | hdr (iommu_hdr_ser)     | |
 *   | | - ref_count             | |
 *   | | - deleted / incoming    | |
 *   | +-------------------------+ |
 *   | - top_table_phys          | |
 *   | - top_level               | |
 *   | - restored_domain         | |
 *   +-----------------------------+
 *
 * This pattern applies identically to iommu_device_ser and iommu_hw_ser.
 */

#define IOMMU_LUO_FLB_COMPATIBLE "iommu-liveupdate-v1"

enum iommu_type_ser {
	IOMMU_INVALID,
	IOMMU_INTEL,
};

/**
 * struct iommu_hdr_ser - Common header for all serialized IOMMU objects
 * @ref_count: Reference count for the object
 * @deleted: Flag indicating if the object is deleted
 * @incoming: Flag indicating if the object was preserved in previous kernel
 */
struct iommu_hdr_ser {
	u32 ref_count;
	u32 deleted:1;
	u32 incoming:1;
} __packed;

/**
 * struct iommu_domain_ser - Serialized state of an IOMMU domain
 * @hdr: Common object header
 * @top_table_phys: Physical address of the top-level page table
 * @top_level: Level of the top-level page table
 * @vasz: Virtual Address Size
 * @sign_extend: FEAT_SIGN_EXTEND is enabled for this domain
 * @restored_domain: Pointer to the restored domain (valid only after restore)
 */
struct iommu_domain_ser {
	struct iommu_hdr_ser hdr;
	u64 top_table_phys;
	u64 top_level;
	u32 vasz;
	u32 sign_extend:1;
	struct iommu_domain *restored_domain;
} __packed;

/**
 * struct iommu_dev_map_ser - Serialized mapping between device, domain,
 *				    and IOMMU instance.
 * @attachment_id: ID of the attachment between device and domain.
 * @domain_phys: Physical address of the domain
 * @iommu_phys: Physical address of the IOMMU
 */
struct iommu_dev_map_ser {
	u64 attachment_id;
	u64 domain_phys;
	u64 iommu_phys;
} __packed;

/**
 * struct iommu_device_intel_ser - Intel specific state of serialized device
 * @pasid_table: Physical address of pasid table
 * @max_pasid: Maximum supported pasid
 */
struct iommu_device_intel_ser {
	u64 pasid_table;
	u64 max_pasid;
} __packed;

/**
 * struct iommu_device_ser - Serialized state of a device
 * @hdr: Common object header
 * @devid: Device ID
 * @pci_domain_nr: PCI domain number
 * @domain_iommu_ser: Domain and IOMMU mapping
 */
struct iommu_device_ser {
	struct iommu_hdr_ser hdr;
	u32 devid;
	u32 pci_domain_nr;
	struct iommu_dev_map_ser domain_iommu_ser;
	union {
		struct iommu_device_intel_ser intel;
	};
} __packed;

/**
 * struct iommu_intel_ser - Serialized state of an Intel IOMMU instance
 * @restored: Whether IOMMU state is restored
 * @phys_addr: Physical address of the IOMMU register base
 * @root_table: Physical address of the root entry table
 */
struct iommu_intel_ser {
	u8 restored;
	u8 padding[7];
	u64 phys_addr;
	u64 root_table;
};

/**
 * struct iommu_hw_ser - Serialized state of an IOMMU instance
 * @hdr: Common object header
 * @token: Unique token for the IOMMU
 * @type: IOMMU type serialized state belongs to
 * @intel: Intel specific serialization data
 */
struct iommu_hw_ser {
	struct iommu_hdr_ser hdr;
	u64 token;
	u64 type;
	union {
		struct iommu_intel_ser intel;
	};
} __packed;

/**
 * struct iommu_array_hdr_ser - Header for an array of serialized objects
 * @next_array_phys: Physical address of the next array of objects
 * @nr_objects: Number of objects in the current array
 */
struct iommu_array_hdr_ser {
	u64 next_array_phys;
	u64 nr_objects;
} __packed;

/**
 * struct iommu_hw_array_ser - An array containing serialized IOMMU HWs
 * @hdr: Array header
 * @objects: Array of serialized IOMMU devices
 */
struct iommu_hw_array_ser {
	struct iommu_array_hdr_ser hdr;
	struct iommu_hw_ser objects[];
} __packed;

/**
 * struct iommu_domain_array_ser - An array containing serialized domains
 * @hdr: Array header
 * @objects: Array of serialized domains
 */
struct iommu_domain_array_ser {
	struct iommu_array_hdr_ser hdr;
	struct iommu_domain_ser objects[];
} __packed;

/**
 * struct iommu_device_array_ser - An array containing serialized devices
 * @hdr: Array header
 * @objects: Array of serialized devices
 */
struct iommu_device_array_ser {
	struct iommu_array_hdr_ser hdr;
	struct iommu_device_ser objects[];
} __packed;

/**
 * struct iommu_flb_ser - Top-level serialization structure
 * @iommu_array_phys: Physical address of the first array of IOMMU HWs
 * @iommu_domain_array_phys: Physical address of the first array of domains
 * @device_array_phys: Physical address of the first array of devices
 */
struct iommu_flb_ser {
	u64 iommu_array_phys;
	u64 iommu_domain_array_phys;
	u64 device_array_phys;
} __packed;

/**
 * struct iommu_flb_obj - FLB object allocated in current kernel pointing to
 *			  preserved state in FLB
 * @lock: Mutex protecting the object
 * @ser: Pointer to the serialized state in FLB
 * @curr_iommu_array: Pointer to the current array of IOMMU instances
 * @curr_domain_array: Pointer to the current array of domains
 * @curr_device_array: Pointer to the current array of devices
 */
struct iommu_flb_obj {
	/* @lock: Protects the serialized objects during concurrent preservation */
	struct mutex lock;
	struct iommu_flb_ser *ser;

	struct iommu_hw_array_ser *curr_iommu_array;
	struct iommu_domain_array_ser *curr_domain_array;
	struct iommu_device_array_ser *curr_device_array;
} __packed;

#endif /* _LINUX_KHO_ABI_IOMMU_H */
