/* SPDX-License-Identifier: GPL-2.0*/
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#ifndef _NBL_EXPORT_RDMA_H_
#define _NBL_EXPORT_RDMA_H_

enum nbl_core_reset_event {
	NBL_CORE_FATAL_ERR_EVENT,  /* Most hw module is not work nomal exclude pcie/emp */
	NBL_CORE_RESET_MAX_EVENT
};

#include <linux/auxiliary_bus.h>

#define RDMA_MSG_MAX_SIZE 256
#define NBL_COREDEV_TO_DMA_DEV(core)		((core)->dma_dev)

struct nbl_chan_rdma_resp {
	u8 resp_data[RDMA_MSG_MAX_SIZE];
	u16 data_len;
};

struct nbl_core_dev_lag_mem {
	u16 vsi_id;
	u8 eth_id;
	bool active;
};

#define NBL_RDMA_LAG_MAX_PORTS		2
struct nbl_core_dev_lag_info {
	struct net_device *bond_netdev;
	struct nbl_core_dev_lag_mem lag_mem[NBL_RDMA_LAG_MAX_PORTS];
	u16 lag_id;
	u8 lag_num;
};

struct nbl_core_dev_info {
	/* Devices */
	struct pci_dev *pdev;
	struct net_device *netdev;
	struct device *dma_dev;
	/* Bar addr */
	u8 __iomem *hw_addr;
	u64 real_hw_addr;
	/* Interrupts */
	struct msix_entry *msix_entries;
	u16 *global_vector_id;
	u16 msix_count;
	/* VSI */
	u16 vsi_id;
	u8 real_bus;
	u8 real_dev;
	u8 real_function;
	/* Send function */
	int (*send)(struct pci_dev *pdev, u8 *req_args, u8 req_len,
		    void *resp, u16 resp_len);
	u8 eth_mode;
	u16 function_id;
	u8 eth_id;
	/* Lag info */
	struct nbl_core_dev_lag_info lag_info;
	int (*lag_mem_notify)(struct auxiliary_device *adev,
			      struct nbl_core_dev_lag_info *lag_info);
	int (*offload_status_notify)(struct auxiliary_device *adev, bool status);
	int (*register_bond)(struct pci_dev *pdev, bool enable);
	bool is_lag;
	/* Info */
	u32 mem_type;
	u16 rdma_cap_num;
	int (*change_mtu_notify)(struct auxiliary_device *adev, int new_mtu);
	bool mirror_enable;
};

struct nbl_aux_dev {
	struct auxiliary_device adev;
	struct nbl_core_dev_info *cdev_info;
	void (*recv)(struct auxiliary_device *device, void *req_args, u16 req_len,
		     struct nbl_chan_rdma_resp *resp);
	void (*abnormal_event_process)(struct auxiliary_device *grc_adev);
	void (*process_flr_event)(struct auxiliary_device *grc_adev, u16 vsi_id);
	int (*reset_event_notify)(struct auxiliary_device *adev, enum nbl_core_reset_event event);
	ssize_t (*qos_cfg_store)(struct auxiliary_device *adev, int offset,
				 const char *buf, size_t count);
	ssize_t (*qos_cfg_show)(struct auxiliary_device *adev, int offset, char *buf);
	int (*mirror_enable_notify)(struct auxiliary_device *adev, bool enable);
};

#endif
