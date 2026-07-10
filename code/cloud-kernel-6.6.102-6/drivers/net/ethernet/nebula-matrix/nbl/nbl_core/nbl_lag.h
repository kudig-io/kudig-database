/* SPDX-License-Identifier: GPL-2.0*/
/*
 * Copyright (c) 2021 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#ifndef _NBL_LAG_H
#define _NBL_LAG_H
#include <linux/kref.h>
#include "nbl_dev.h"

#define NBL_INVALID_LAG_ID	0xf
#define nbl_lag_id_valid(lag_id)	((lag_id) < NBL_LAG_MAX_NUM)

#define NBL_LAG_ENABLE		BIT(0)
#define NBL_LAG_DISABLE		BIT(1)
#define NBL_LAG_UPDATE_HASH	BIT(2)
#define NBL_LAG_UPDATE_MEMBER	BIT(3)
#define NBL_LAG_UPDATE_LINK	BIT(4)
#define NBL_LAG_UPDATE_SFP_TX	BIT(5)
#define NBL_LAG_UPDATE_LACP_PKT	BIT(6)

enum nbl_lag_mem_fwd {
	NBL_LAG_MEM_FWD_DROP		= 0,
	NBL_LAG_MEM_FWD_NORMAL		= 1,
};

struct nbl_lag_instance {
	struct net_device *bond_netdev;
	struct netdev_lag_upper_info lag_upper_info;
	struct list_head mem_list_head;
	struct list_head instance_node;
	u8 linkup;
	u8 lag_enable;
	u8 lag_id;
};

struct nbl_lag_resource {
	struct kref kref;
	struct list_head resource_node;
	u32 board_key; /* domain << 16 | bus_id */
	DECLARE_BITMAP(lag_id_bitmap, NBL_LAG_MAX_NUM);
	struct list_head lag_instance_head;
};

static inline bool nbl_lag_mem_is_active(const struct nbl_lag_member *lag_mem)
{
	return lag_mem->bonded && lag_mem->lower_state.link_up && lag_mem->lower_state.tx_enabled;
}

int nbl_init_lag(struct nbl_dev_mgt *dev_mgt, struct nbl_init_param *param);
int nbl_deinit_lag(struct nbl_dev_mgt *dev_mgt);
u32 nbl_lag_get_other_active_members(struct nbl_dev_mgt *dev_mgt,
				     u16 eth_list[], u32 array_size);
#endif /* _NBL_LAG_H */
