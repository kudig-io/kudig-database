// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#include "nbl_lag.h"
#include "nbl_dev.h"

struct list_head lag_resource_head;
/* mutex for lag resource */
struct mutex nbl_lag_mutex;

static inline void init_lag_instance(struct nbl_lag_instance *lag_info, struct net_device *bond_dev)
{
	lag_info->bond_netdev = bond_dev;
	INIT_LIST_HEAD(&lag_info->mem_list_head);
	lag_info->linkup = 0;
	lag_info->lag_enable = 0;
	lag_info->lag_id = NBL_INVALID_LAG_ID;
	memset(&lag_info->lag_upper_info, 0, sizeof(lag_info->lag_upper_info));
}

static struct nbl_lag_instance *find_lag_by_lagid(u32 board_key, u8 lag_id)
{
	struct nbl_lag_resource *find_resource = NULL;
	struct nbl_lag_resource *lag_resource_tmp;
	struct nbl_lag_instance *lag_tmp, *lag_info = NULL;

	if (!nbl_lag_id_valid(lag_id))
		goto ret;

	/* find the lag resource by the bus id, identify a card */
	list_for_each_entry(lag_resource_tmp, &lag_resource_head, resource_node) {
		if (lag_resource_tmp->board_key == board_key) {
			find_resource = lag_resource_tmp;
			break;
		}
	}

	if (!find_resource)
		goto ret;

	/* find the lag instance by lag_id */
	list_for_each_entry(lag_tmp, &find_resource->lag_instance_head, instance_node) {
		if (lag_tmp->lag_id == lag_id) {
			lag_info = lag_tmp;
			break;
		}
	}

ret:
	return lag_info;
}

static struct nbl_lag_instance *find_lag_by_bonddev(u32 board_key, struct net_device *bond_dev)
{
	struct nbl_lag_resource *find_resource = NULL;
	struct nbl_lag_resource *lag_resource_tmp;
	struct nbl_lag_instance *lag_tmp, *lag_info = NULL;

	if (!bond_dev)
		goto ret;

	/* find the lag resource by the bus id, identify a card */
	list_for_each_entry(lag_resource_tmp, &lag_resource_head, resource_node) {
		if (lag_resource_tmp->board_key == board_key) {
			find_resource = lag_resource_tmp;
			break;
		}
	}

	if (!find_resource)
		goto ret;

	/* find the lag instance by bonddev */
	list_for_each_entry(lag_tmp, &find_resource->lag_instance_head, instance_node) {
		if (lag_tmp->bond_netdev == bond_dev) {
			lag_info = lag_tmp;
			break;
		}
	}

ret:
	return lag_info;
}

static struct nbl_lag_instance *alloc_lag_instance(u32 board_key, struct net_device *bond_dev,
						   struct nbl_lag_resource **find_resource)
{
	struct nbl_lag_resource *lag_resource_tmp;
	struct nbl_lag_instance *lag_tmp, *lag_info = NULL;

	/* find the lag resource by the bus id, identify a card */
	list_for_each_entry(lag_resource_tmp, &lag_resource_head, resource_node) {
		if (lag_resource_tmp->board_key == board_key) {
			*find_resource = lag_resource_tmp;
			break;
		}
	}

	if (!(*find_resource))
		goto ret;

	/* find the lag instance by bond_dev */
	list_for_each_entry(lag_tmp, &(*find_resource)->lag_instance_head, instance_node) {
		/* mark the idle lag instance */
		if (!lag_info && !lag_tmp->bond_netdev)
			lag_info = lag_tmp;
		if (lag_tmp->bond_netdev == bond_dev) {
			lag_info = lag_tmp;
			break;
		}
	}
	/* if not found and no idle lag instance, then alloc a new lag instance */
	if (!lag_info) {
		lag_info = kzalloc(sizeof(*lag_info), GFP_KERNEL);
		if (!lag_info)
			goto ret;

		init_lag_instance(lag_info, bond_dev);
		list_add_tail(&lag_info->instance_node, &(*find_resource)->lag_instance_head);
	}

ret:
	return lag_info;
}

static void nbl_display_lag_info(struct nbl_dev_mgt *dev_mgt, u8 lag_id)
{
	struct nbl_dev_net *net_dev = dev_mgt->net_dev;
	struct net_device *current_netdev;
	const char *member_name, *upper_name;
	struct nbl_lag_member *mem_tmp;
	struct nbl_lag_instance *lag_info = NULL;
	u32 board_key;

	board_key = pci_domain_nr(dev_mgt->common->pdev->bus) << 16 |
			dev_mgt->common->pdev->bus->number;
	lag_info = find_lag_by_lagid(board_key, lag_id);

	if (!lag_info)
		return;

	current_netdev = net_dev->netdev;
	upper_name = lag_info->bond_netdev ? netdev_name(lag_info->bond_netdev) : "unset";
	nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
		 "bond dev %s: enabled is %u, lag_id is %u.\n",
		 upper_name, lag_info->lag_enable, lag_info->lag_id);

	if (lag_info && lag_info->lag_enable) {
		nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			 "bond dev %s: tx_type is %d, hash_type is %d.\n", upper_name,
			 lag_info->lag_upper_info.tx_type,
			 lag_info->lag_upper_info.hash_type);

		list_for_each_entry(mem_tmp, &lag_info->mem_list_head, mem_list_node) {
			member_name = current_netdev ?
					netdev_name(current_netdev) : "unset";
			nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
				 "%s(%s): lag_id: %d, eth_id: %d, bonded: %d, linkup: %d, tx_enabled: %d.\n",
				 upper_name, member_name, mem_tmp->lag_id,
				 mem_tmp->logic_eth_id, mem_tmp->bonded,
				 mem_tmp->lower_state.link_up,
				 mem_tmp->lower_state.tx_enabled);
		}
	}
}

static void nbl_lag_create_bond_adev(struct nbl_dev_mgt *dev_mgt,
				     struct nbl_lag_instance *lag_info)
{
	struct nbl_event_param event_data;
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_lag_member *mem_tmp, *notify_mem = NULL;
	struct nbl_lag_member_list_param *list_param = &event_data.param;
	struct nbl_rdma_register_param register_param = {0};
	int mem_num = 0;
	int i = 0;

	memset(&event_data, 0, sizeof(event_data));

	list_for_each_entry(mem_tmp, &lag_info->mem_list_head, mem_list_node) {
		event_data.param.member_list[mem_num].vsi_id = mem_tmp->vsi_id;
		event_data.param.member_list[mem_num].eth_id = mem_tmp->eth_id;
		mem_num++;
		if (!notify_mem || notify_mem->eth_id > mem_tmp->eth_id)
			notify_mem = mem_tmp;
	}

	if (!notify_mem) {
		nbl_err(common, NBL_DEBUG_MAIN,
			"notify to create the bond adev failed, member count %u.\n", mem_num);
		return;
	}
	event_data.param.lag_num = mem_num;

	/* Checking if we can support and create the rdma bond */
	serv_ops->register_rdma_bond(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
				     list_param, &register_param);

	if (!register_param.has_rdma) {
		nbl_warn(common, NBL_DEBUG_MAIN,
			 "Can not support to create rdma bond, vsi %u.\n", notify_mem->vsi_id);
		return;
	}

	for (i = 0; i < mem_num; i++) {
		event_data.subevent = NBL_SUBEVENT_RELEASE_ADEV;
		/* Notify the dev to release the rdma adev first. */
		nbl_event_notify(NBL_EVENT_RDMA_BOND_UPDATE, &event_data,
				 event_data.param.member_list[i].vsi_id,
				 NBL_COMMON_TO_BOARD_ID(common));
	}

	/* Notify the rdma dev to create the bond adev. */
	event_data.subevent = NBL_SUBEVENT_CREATE_BOND_ADEV;
	event_data.param.bond_netdev = lag_info->bond_netdev;
	event_data.param.lag_id = lag_info->lag_id;
	event_data.param.lag_num = mem_num;

	nbl_event_notify(NBL_EVENT_RDMA_BOND_UPDATE, &event_data, notify_mem->vsi_id,
			 NBL_COMMON_TO_BOARD_ID(common));

	notify_mem->is_bond_adev = true;
}

static void nbl_lag_member_recover_adev(struct nbl_dev_mgt *dev_mgt,
					struct nbl_lag_instance *lag_info,
					struct nbl_lag_member *lag_mem)
{
	struct nbl_event_param event_data;
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	struct nbl_lag_member *mem_tmp, *adev_mem = NULL;
	int i = 0, has_self = 0, mem_num = 0;

	memset(&event_data, 0, sizeof(event_data));

	list_for_each_entry(mem_tmp, &lag_info->mem_list_head, mem_list_node)
		if (mem_tmp == lag_mem)
			has_self = 1;

	if (!has_self)
		return;

	list_for_each_entry(mem_tmp, &lag_info->mem_list_head, mem_list_node) {
		event_data.param.member_list[mem_num].vsi_id = mem_tmp->vsi_id;
		event_data.param.member_list[mem_num].eth_id = mem_tmp->eth_id;
		mem_num++;

		if (mem_tmp->is_bond_adev)
			adev_mem = mem_tmp;
	}

	/* If we cannot find a member with adev, then we have nothing to do, return */
	if (!adev_mem)
		return;

	/* Notify the rdma dev to delete the bond adev. */
	event_data.subevent = NBL_SUBEVENT_RELEASE_BOND_ADEV;
	event_data.param.bond_netdev = lag_info->bond_netdev;
	event_data.param.lag_id = lag_info->lag_id;
	event_data.param.lag_num = mem_num;

	nbl_event_notify(NBL_EVENT_RDMA_BOND_UPDATE, &event_data, adev_mem->vsi_id,
			 NBL_COMMON_TO_BOARD_ID(common));

	for (i = 0; i < mem_num; i++) {
		event_data.subevent = NBL_SUBEVENT_CREATE_ADEV;
		/* Notify the dev to restore the rdma adev. */
		nbl_event_notify(NBL_EVENT_RDMA_BOND_UPDATE, &event_data,
				 event_data.param.member_list[i].vsi_id,
				 NBL_COMMON_TO_BOARD_ID(common));
	}

	adev_mem->is_bond_adev = false;
}

static void update_lag_member_list(struct nbl_dev_mgt *dev_mgt,
				   u8 lag_id,
				   struct nbl_lag_instance *lag_info,
				   struct nbl_lag_member *lag_mem)
{
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	struct nbl_lag_member *mem_tmp;
	struct nbl_event_param event_data;
	struct nbl_lag_member_list_param mem_list_param = {0};
	u16 mem_id, tx_enabled_id = U16_MAX;
	u8 fwd;

	list_for_each_entry(mem_tmp, &lag_info->mem_list_head, mem_list_node) {
		if (nbl_lag_mem_is_active(mem_tmp))
			tx_enabled_id = mem_tmp->eth_id;
	}

	memset(&event_data, 0, sizeof(event_data));
	mem_id = 0;
	list_for_each_entry(mem_tmp, &lag_info->mem_list_head, mem_list_node) {
		if (mem_id < NBL_LAG_MAX_PORTS) {
			/* The member list is mainly for dup-arp/nd cfg.
			 * If we only use port_list, which only contains active eth_id, the
			 * following problem will occur:
			 * 1. Add pf0 & pf1 to bond
			 * 2. pf0 up, pf0 cfg member_list, right now only pf0 is active, so
			 *    port_list contains only eth0
			 * 3. pf1 up, pf1 cfg member_list, now both pf0 and pf1 are up, so
			 *    port_list contains eth0 & eth1
			 * In this case, pf1 knows that it should dup-arp to two ports, but
			 * pf0 is unaware, so if kernel use pf0 to send pkts, it cannot dup.
			 */
			mem_list_param.member_list[mem_id].eth_id = mem_tmp->eth_id;
			mem_list_param.member_list[mem_id].vsi_id = mem_tmp->vsi_id;

			if (nbl_lag_mem_is_active(mem_tmp)) {
				mem_list_param.port_list[mem_id] = mem_tmp->eth_id;
				mem_list_param.member_list[mem_id].active = true;
			} else if (tx_enabled_id < U16_MAX) {
				mem_list_param.port_list[mem_id] = tx_enabled_id;
			}
		}
		mem_id++;
	}
	mem_list_param.lag_num = mem_id;
	if (lag_info->lag_upper_info.tx_type != NETDEV_LAG_TX_TYPE_ACTIVEBACKUP && mem_id > 1)
		mem_list_param.duppkt_enable = true;

	if (tx_enabled_id < U16_MAX)
		for ( ; mem_id < NBL_LAG_MAX_PORTS; )
			mem_list_param.port_list[mem_id++] = tx_enabled_id;

	mem_list_param.lag_id = lag_id;
	serv_ops->cfg_lag_member_list(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt), &mem_list_param);

	serv_ops->cfg_lag_member_up_attr(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					 lag_mem->eth_id, lag_id, lag_mem->bonded ? true : false);
	if (!lag_mem->bonded) {
		fwd = NBL_LAG_MEM_FWD_DROP;
		serv_ops->cfg_lag_member_fwd(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					     lag_mem->eth_id, lag_id, fwd);
	}

	mem_list_param.bond_netdev = lag_info->bond_netdev;
	memcpy(&event_data.param, &mem_list_param, sizeof(event_data.param));
	event_data.subevent = NBL_SUBEVENT_UPDATE_BOND_MEMBER;

	/* Make sure only notify the dev who has been created the rdma bond adev to update the
	 * bond member list info.
	 */
	list_for_each_entry(mem_tmp, &lag_info->mem_list_head, mem_list_node)
		if (mem_tmp->is_bond_adev)
			nbl_event_notify(NBL_EVENT_RDMA_BOND_UPDATE, &event_data, mem_tmp->vsi_id,
					 NBL_COMMON_TO_BOARD_ID(common));
}

static void nbl_update_lag_cfg(struct nbl_lag_member *lag_mem, u8 lag_id, u32 flag)
{
	struct nbl_dev_mgt *dev_mgt = NBL_NETDEV_TO_DEV_MGT(lag_mem->netdev);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_dev_net *net_dev = NBL_DEV_MGT_TO_NET_DEV(dev_mgt);
	struct nbl_dev_vsi *vsi = net_dev->vsi_ctrl.vsi_list[NBL_VSI_DATA];
	u16 eth_id;
	u8 fwd;
	const char *upper_name;
	struct nbl_lag_instance *lag_info = NULL;
	bool sfp_tx_enable, lag_enable;
	u32 board_key;

	eth_id = lag_mem->eth_id;
	board_key = pci_domain_nr(dev_mgt->common->pdev->bus) << 16 |
			dev_mgt->common->pdev->bus->number;

	if (flag & NBL_LAG_UPDATE_LACP_PKT) {
		lag_enable = lag_mem->bonded ? true : false;
		serv_ops->enable_lag_protocol(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					      eth_id, lag_enable);
		vsi->feature.has_lacp = lag_enable;

		nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			 "%s lag protocol for lag_id: %d.\n",
			 lag_enable ? "enable" : "disable", lag_id);
	}

	if (flag & NBL_LAG_UPDATE_SFP_TX) {
		if (lag_mem->bonded)
			sfp_tx_enable = lag_mem->lower_state.link_up;
		else
			sfp_tx_enable = true;
		serv_ops->set_sfp_state(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt), lag_mem->netdev,
					(u8)eth_id, sfp_tx_enable, true);
	}

	if (!nbl_lag_id_valid(lag_id)) {
		nbl_debug(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			  "lag_id: %d is invalid, flag: 0x%08x.\n", lag_id, flag);
		return;
	}

	lag_info = find_lag_by_lagid(board_key, lag_id);

	if (!lag_info)
		return;

	upper_name = lag_info->bond_netdev ? netdev_name(lag_info->bond_netdev) : "unset";
	nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
		 "bond dev %s: lag_id: %d, eth_id: %u, enabled: %d, linkup: %s, flag: 0x%08x.\n",
		 upper_name, lag_id, lag_mem->logic_eth_id, lag_info->lag_enable,
		 lag_info->linkup ? "up" : "down", flag);

	if (flag & NBL_LAG_UPDATE_HASH)
		serv_ops->cfg_lag_hash_algorithm(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt), eth_id,
						 lag_id, lag_info->lag_upper_info.hash_type);

	if (flag & NBL_LAG_UPDATE_LINK) {
		if (lag_mem->bonded) {
			fwd = NBL_LAG_MEM_FWD_DROP;
			if (lag_info->linkup)
				fwd = nbl_lag_mem_is_active(lag_mem) ?
						NBL_LAG_MEM_FWD_NORMAL : NBL_LAG_MEM_FWD_DROP;
			serv_ops->cfg_lag_member_fwd(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
						     eth_id, lag_id, fwd);
		}
	}

	if (flag & NBL_LAG_UPDATE_MEMBER)
		update_lag_member_list(dev_mgt, lag_id, lag_info, lag_mem);
}

static int del_lag_member(struct nbl_dev_mgt *dev_mgt,
			  struct nbl_lag_instance *lag_info,
			  struct netdev_notifier_changeupper_info *info)
{
	struct nbl_dev_net *net_dev = dev_mgt->net_dev;
	struct nbl_lag_member *lag_mem;
	u8 mem_count = 0;
	struct nbl_lag_member *mem_tmp = NULL;

	lag_mem = net_dev->lag_mem;

	list_for_each_entry(mem_tmp, &lag_info->mem_list_head, mem_list_node) {
		mem_count++;
		if (lag_mem == mem_tmp)
			break;
	}

	if (list_entry_is_head(mem_tmp, &lag_info->mem_list_head, mem_list_node))
		return -ENOENT;

	if (mem_count == 0 || mem_count > NBL_LAG_MAX_PORTS) {
		nbl_err(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			"lag member device has been deleted.\n");
		return -1;
	}

	lag_mem->bonded = 0;
	lag_mem->lag_id = NBL_INVALID_LAG_ID;
	memset(&lag_mem->lower_state, 0, sizeof(lag_mem->lower_state));
	list_del(&lag_mem->mem_list_node);

	return 0;
}

static int add_lag_member(struct nbl_dev_mgt *dev_mgt,
			  struct nbl_lag_instance *lag_info,
			  u8 lag_id,
			  struct netdev_notifier_changeupper_info *info)
{
	struct nbl_dev_net *net_dev = dev_mgt->net_dev;
	struct nbl_lag_member *lag_mem;
	u8 mem_count = 0;
	struct netdev_lag_upper_info *upper_info;
	struct nbl_lag_member *mem_tmp = NULL;

	lag_mem = net_dev->lag_mem;
	upper_info = info->upper_info;

	list_for_each_entry(mem_tmp, &lag_info->mem_list_head, mem_list_node) {
		mem_count++;
		if (lag_mem == mem_tmp)
			return 0;
	}

	if (mem_count < NBL_LAG_MAX_PORTS) {
		lag_mem->bonded = 1;
		lag_mem->lag_id = lag_id;
		list_add_tail(&lag_mem->mem_list_node, &lag_info->mem_list_head);
	} else {
		nbl_err(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			"no available lag member resource.\n");
		return -1;
	}
	return 0;
}

static bool is_lag_can_offload(struct nbl_dev_mgt *dev_mgt,
			       const struct nbl_lag_instance *lag_info)
{
	struct nbl_lag_resource *lag_resource_tmp;
	struct nbl_lag_instance *lag_info_tmp;
	u32 count = 0;

	if (!(lag_info->lag_upper_info.tx_type == NETDEV_LAG_TX_TYPE_ACTIVEBACKUP ||
	      lag_info->lag_upper_info.tx_type == NETDEV_LAG_TX_TYPE_HASH)) {
		nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			 "bond dev %s tx_type %d is not allowed.\n",
			 netdev_name(lag_info->bond_netdev), lag_info->lag_upper_info.tx_type);
		return false;
	}

	/* if the lag instance's all lag members only belong to one card, this lag can offload */
	list_for_each_entry(lag_resource_tmp, &lag_resource_head, resource_node) {
		list_for_each_entry(lag_info_tmp,
				    &lag_resource_tmp->lag_instance_head, instance_node) {
			if (lag_info_tmp->bond_netdev == lag_info->bond_netdev &&
			    !list_empty(&lag_info_tmp->mem_list_head))
				count++;
		}
	}

	return (count == 1) ? true : false;
}

static int enable_lag_instance(struct nbl_lag_resource *lag_resource,
			       struct nbl_lag_instance *lag_info)
{
	u8 lag_id;
	struct nbl_lag_member *lag_mem;

	if (lag_info->lag_enable)
		return 0;

	/* enable the lag instance, and distribute a lag id, then updating all members' lag id */
	lag_id = find_first_zero_bit(lag_resource->lag_id_bitmap, NBL_LAG_MAX_NUM);
	if (!nbl_lag_id_valid(lag_id))
		return -1;

	set_bit(lag_id, lag_resource->lag_id_bitmap);

	list_for_each_entry(lag_mem, &lag_info->mem_list_head, mem_list_node)
		lag_mem->lag_id = lag_id;

	lag_info->lag_id = lag_id;
	lag_info->lag_enable = 1;
	return 0;
}

static void disable_lag_instance(struct nbl_lag_resource *lag_resource,
				 struct nbl_lag_instance *lag_info)
{
	u8 lag_id;

	/* retrieving the lag id resource, then disable and init the lag instance.
	 * don't free the lag instance for reusing later if needed, all lag instance
	 * resource will be freed in lag dinit function.
	 */
	lag_id = lag_info->lag_id;
	clear_bit(lag_id, lag_resource->lag_id_bitmap);

	init_lag_instance(lag_info, NULL);
}

static void nbl_lag_changeupper_event(struct nbl_dev_mgt *dev_mgt, void *ptr, u32 *flag)
{
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_dev_net *net_dev = dev_mgt->net_dev;
	struct nbl_lag_member *lag_mem, *mem_tmp;
	struct nbl_lag_resource *lag_resource = NULL;
	struct netdev_notifier_changeupper_info *info;
	struct netdev_lag_upper_info *upper_info;
	struct net_device *netdev;
	struct nbl_lag_instance *lag_info;
	const char *upper_name, *device_name;
	struct net_device *current_netdev;
	u8 lag_id = NBL_INVALID_LAG_ID;
	u32 board_key;
	int ret;

	info = ptr;
	netdev = netdev_notifier_info_to_dev(ptr);

	lag_mem = net_dev->lag_mem;
	current_netdev = net_dev->netdev;
	board_key = pci_domain_nr(dev_mgt->common->pdev->bus) << 16 |
			dev_mgt->common->pdev->bus->number;

	/* not for this netdev */
	if (netdev != current_netdev)
		return;

	device_name = netdev ? netdev_name(netdev) : "unset";

	if (!info->upper_dev) {
		nbl_err(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			"changeupper(%s) event received, but upper dev is null\n", device_name);
		return;
	}

	upper_info = info->upper_info;
	upper_name = netdev_name(info->upper_dev);

	nbl_debug(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
		  "changeupper(%s) event bond %s, linking: %d, master: %d, tx_type: %d, hash_type: %d.\n",
		  device_name,
		  upper_name, info->linking, info->master,
		  upper_info ? upper_info->tx_type : 0,
		  upper_info ? upper_info->hash_type : 0);

	if (!netif_is_lag_master(info->upper_dev)) {
		nbl_debug(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			  "changeupper(%s) event received, but not master.\n", device_name);
		return;
	}

	lag_info = alloc_lag_instance(board_key, info->upper_dev, &lag_resource);
	if (!lag_info || !lag_resource) {
		nbl_debug(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			  "changeupper(%s) event received, but have no lag resource for board_key 0x%x.\n",
			  device_name, board_key);
		return;
	}

	lag_id = lag_info->lag_id;
	if (info->linking) {
		ret = add_lag_member(dev_mgt, lag_info, lag_id, info);
		if (!ret) {
			/* updating the lag info when the first device bonding to this lag */
			if (nbl_list_is_first(&lag_mem->mem_list_node, &lag_info->mem_list_head)) {
				lag_info->bond_netdev = info->upper_dev;
				lag_info->linkup = (lag_info->bond_netdev->flags & IFF_UP) ? 1 : 0;
				lag_info->lag_upper_info.tx_type = upper_info->tx_type;
				lag_info->lag_upper_info.hash_type = upper_info->hash_type;
			} else if (is_lag_can_offload(dev_mgt, lag_info)) {
				/* if the lag can offload after the second device bonding to it,
				 * will enable the lag instance and assign a lag id for this lag,
				 * then update the offloading configuration.
				 */
				if (enable_lag_instance(lag_resource, lag_info))
					nbl_err(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
						"enable lag failed, lag-bitmap: %lx.\n",
						lag_resource->lag_id_bitmap[0]);
				else
					nbl_lag_create_bond_adev(dev_mgt, lag_info);
			}
			if (lag_info->lag_enable) {
				*flag |= NBL_LAG_UPDATE_HASH | NBL_LAG_UPDATE_MEMBER |
					 NBL_LAG_UPDATE_LINK;
				list_for_each_entry(mem_tmp, &lag_info->mem_list_head,
						    mem_list_node)
					nbl_update_lag_cfg(mem_tmp, mem_tmp->lag_id, *flag);
				*flag = 0;
			}
			*flag = NBL_LAG_UPDATE_LACP_PKT;
			serv_ops->set_lag_info(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					       lag_info->bond_netdev, lag_info->lag_id);
		}
	} else {
		nbl_lag_member_recover_adev(dev_mgt, lag_info, lag_mem);

		ret = del_lag_member(dev_mgt, lag_info, info);
		if (!ret) {
			/* updating the offloading configuration if the lag enabled. If all
			 * members unbonded, will disable and init this lag instance, and
			 * retrieve the lag id resource.
			 */
			if (lag_info->lag_enable) {
				*flag |= NBL_LAG_UPDATE_MEMBER;
				nbl_update_lag_cfg(lag_mem, lag_id, *flag);
			}
			if (list_empty(&lag_info->mem_list_head))
				disable_lag_instance(lag_resource, lag_info);
			*flag = NBL_LAG_UPDATE_LACP_PKT | NBL_LAG_UPDATE_SFP_TX;
			serv_ops->unset_lag_info(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt));
		}
	}
}

static void nbl_lag_changelower_event(struct nbl_dev_mgt *dev_mgt, void *ptr, u32 *flag)
{
	struct nbl_dev_net *net_dev = dev_mgt->net_dev;
	struct nbl_lag_member *lag_mem;
	struct netdev_notifier_changelowerstate_info *info;
	struct netdev_lag_lower_state_info *lower_stat_info;
	struct net_device *netdev;
	const char *device_name;
	struct net_device *current_netdev;
	struct nbl_lag_instance *lag_info;
	u8 lag_id;
	u32 board_key;

	info = ptr;
	netdev = netdev_notifier_info_to_dev(ptr);
	lower_stat_info = info->lower_state_info;
	if (!lower_stat_info)
		return;

	device_name = netdev ? netdev_name(netdev) : "unset";

	lag_mem = net_dev->lag_mem;
	current_netdev = net_dev->netdev;

	/* not for this netdev */
	if (netdev != current_netdev)
		return;

	nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
		 "changelower(%s) event link_up: %d, tx_enabled: %d.\n",
		 device_name,
		 lower_stat_info->link_up,
		 lower_stat_info->tx_enabled);

	board_key = pci_domain_nr(dev_mgt->common->pdev->bus) << 16 |
			dev_mgt->common->pdev->bus->number;
	if (lag_mem->bonded) {
		lag_mem->lower_state.link_up = lower_stat_info->link_up;
		lag_mem->lower_state.tx_enabled = lower_stat_info->tx_enabled;
		lag_id = lag_mem->lag_id;
		lag_info = find_lag_by_lagid(board_key, lag_id);
		if (!lag_info) {
			nbl_debug(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
				  "changelower(%s) event received, but have no lag resource for board_key 0x%x.\n",
				  device_name, board_key);
			return;
		}

		if (lag_info->lag_enable)
			*flag |= NBL_LAG_UPDATE_MEMBER | NBL_LAG_UPDATE_LINK;
	}
}

static void nbl_lag_info_event(struct nbl_dev_mgt *dev_mgt, void *ptr, u32 *flag)
{
	struct nbl_dev_net *net_dev = dev_mgt->net_dev;
	struct nbl_lag_member *lag_mem;
	struct net_device *netdev;
	struct netdev_notifier_bonding_info *info;
	struct netdev_bonding_info *bonding_info;
	const char *lag_mem_name;
	struct net_device *current_netdev;
	struct nbl_lag_instance *lag_info;
	u8 lag_id;
	u32 board_key;

	info = ptr;
	netdev = netdev_notifier_info_to_dev(ptr);
	info = ptr;
	bonding_info = &info->bonding_info;
	lag_mem = net_dev->lag_mem;
	current_netdev = net_dev->netdev;

	if (!current_netdev || netdev != current_netdev)
		return;

	lag_mem_name = netdev_name(current_netdev);

	nbl_debug(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
		  "bondinfo(%s) event, bond_mode: %d, num_slaves: %d, miimon: %d.\n",
		  lag_mem_name, bonding_info->master.bond_mode,
		  bonding_info->master.num_slaves,
		  bonding_info->master.miimon);

	nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
		 "bondinfo(%s) event, slave_id: %d, slave_name: %s, link: %d, state: %d, failure_count: %d.\n",
		 lag_mem_name, bonding_info->slave.slave_id,
		 bonding_info->slave.slave_name, bonding_info->slave.link,
		 bonding_info->slave.state, bonding_info->slave.link_failure_count);

	if (bonding_info->master.bond_mode != BOND_MODE_ACTIVEBACKUP) {
		nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			 "Bonding event recv, but mode not active/backup.\n");
		return;
	}

	if (bonding_info->slave.state == BOND_STATE_BACKUP) {
		if (lag_mem->bonded) {
			lag_mem->lower_state.tx_enabled = 0;
			board_key = pci_domain_nr(dev_mgt->common->pdev->bus) << 16 |
					dev_mgt->common->pdev->bus->number;
			lag_id = lag_mem->lag_id;
			lag_info = find_lag_by_lagid(board_key, lag_id);
			if (!lag_info) {
				nbl_debug(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
					  "bondinfo(%s) event received, but have no lag resource for board_key 0x%x.\n",
					  lag_mem_name, board_key);
				return;
			}
			if (lag_info->lag_enable)
				*flag |= NBL_LAG_UPDATE_MEMBER | NBL_LAG_UPDATE_LINK;
		}
	}
}

static void nbl_lag_updown_event(struct nbl_dev_mgt *dev_mgt, void *ptr, bool is_up, u32 *flag)
{
	struct nbl_dev_net *net_dev = dev_mgt->net_dev;
	struct nbl_lag_member *lag_mem;
	struct netdev_notifier_info *info;
	struct net_device *event_netdev = NULL;
	struct nbl_lag_instance *lag_info = NULL;
	const char *device_name;
	u8 linkup;
	u32 board_key;

	info = ptr;
	event_netdev = netdev_notifier_info_to_dev(ptr);
	device_name = netdev_name(event_netdev);
	lag_mem = net_dev->lag_mem;
	if (!lag_mem->bonded)
		return;

	board_key = pci_domain_nr(dev_mgt->common->pdev->bus) << 16 |
			dev_mgt->common->pdev->bus->number;
	lag_info = find_lag_by_bonddev(board_key, event_netdev);

	if (!(lag_info || net_dev->netdev == event_netdev))
		return;

	linkup = is_up ? 1 : 0;
	nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
		 "netdev%s(%s) event received.\n", linkup ? "up" : "down", device_name);

	/* bond dev up/down event */
	if (lag_info) {
		lag_info->linkup = linkup;
		/* if the lag link change, update the member's fwd type */
		if (lag_info->lag_enable) {
			*flag |= NBL_LAG_UPDATE_LINK;
			if (linkup)
				*flag |= NBL_LAG_UPDATE_MEMBER;
		}
	} else { /* lag member dev up/down event */
		lag_mem->lower_state.link_up = linkup;
		*flag |= NBL_LAG_UPDATE_SFP_TX;
	}
}

static void nbl_lag_change_event(struct nbl_dev_mgt *dev_mgt, void *ptr, u32 *flag)
{
	struct nbl_dev_net *net_dev = dev_mgt->net_dev;
	struct nbl_lag_member *lag_mem;
	struct netdev_notifier_change_info *info;
	struct net_device *lag_netdev = NULL;
	struct bonding *bond;
	enum netdev_lag_hash new_hash;
	struct nbl_lag_instance *lag_info = NULL;
	const char *device_name;
	u32 board_key;

	info = ptr;
	lag_netdev = netdev_notifier_info_to_dev(ptr);

	device_name = lag_netdev ? netdev_name(lag_netdev) : "unset";
	lag_mem = net_dev->lag_mem;
	if (!lag_mem->bonded)
		return;

	board_key = pci_domain_nr(dev_mgt->common->pdev->bus) << 16 |
			dev_mgt->common->pdev->bus->number;

	lag_info = find_lag_by_bonddev(board_key, lag_netdev);

	if (!lag_info)
		return;

	bond = netdev_priv(lag_netdev);

	switch (bond->params.xmit_policy) {
	case BOND_XMIT_POLICY_LAYER2:
		new_hash = NETDEV_LAG_HASH_L2;
		break;
	case BOND_XMIT_POLICY_LAYER34:
		new_hash = NETDEV_LAG_HASH_L34;
		break;
	case BOND_XMIT_POLICY_LAYER23:
		new_hash = NETDEV_LAG_HASH_L23;
		break;
	case BOND_XMIT_POLICY_ENCAP23:
		new_hash = NETDEV_LAG_HASH_E23;
		break;
	case BOND_XMIT_POLICY_ENCAP34:
		new_hash = NETDEV_LAG_HASH_E34;
		break;
	case BOND_XMIT_POLICY_VLAN_SRCMAC:
		new_hash = NETDEV_LAG_HASH_VLAN_SRCMAC;
		break;
	default:
		new_hash = NETDEV_LAG_HASH_UNKNOWN;
		break;
	}

	nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
		 "netdevchange(%s) event received, old hash: %d, new hash: %d.\n",
		 device_name, lag_info->lag_upper_info.hash_type, new_hash);

	if (lag_info->lag_upper_info.hash_type != new_hash) {
		lag_info->lag_upper_info.hash_type = new_hash;
		if (lag_info->lag_enable)
			*flag |= NBL_LAG_UPDATE_HASH;
	}
}

static int
nbl_lag_event_handler(struct notifier_block *notify_blk, unsigned long event, void *ptr)
{
	struct net_device *netdev = netdev_notifier_info_to_dev(ptr);
	struct nbl_lag_member *lag_mem;
	struct nbl_dev_mgt *dev_mgt;
	u32 update_flag = 0;
	u8 bus_id;
	u8 lag_id = NBL_INVALID_LAG_ID;

	lag_mem = container_of(notify_blk, struct nbl_lag_member, notify_block);

	if (!lag_mem)
		return NOTIFY_DONE;

	dev_mgt = (struct nbl_dev_mgt *)NBL_NETDEV_TO_DEV_MGT(lag_mem->netdev);

	bus_id = NBL_DEV_MGT_TO_COMMON(dev_mgt)->bus;

	nbl_debug(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN, "nbl kernel(%s) receive event: %s.\n",
		  netdev_name(netdev), netdev_cmd_to_name(event));

	mutex_lock(&nbl_lag_mutex);
	/* record the bonded slave's lag_id */
	if (lag_mem->bonded)
		lag_id = lag_mem->lag_id;

	switch (event) {
	case NETDEV_CHANGEUPPER:
		nbl_lag_changeupper_event(dev_mgt, ptr, &update_flag);
		break;
	case NETDEV_CHANGELOWERSTATE:
		nbl_lag_changelower_event(dev_mgt, ptr, &update_flag);
		break;
	case NETDEV_BONDING_INFO:
		nbl_lag_info_event(dev_mgt, ptr, &update_flag);
		break;
	case NETDEV_DOWN:
		nbl_lag_updown_event(dev_mgt, ptr, false, &update_flag);
		break;
	case NETDEV_UP:
		nbl_lag_updown_event(dev_mgt, ptr, true, &update_flag);
		break;
	case NETDEV_CHANGE:
	case NETDEV_FEAT_CHANGE:
		nbl_lag_change_event(dev_mgt, ptr, &update_flag);
		break;
	default:
		goto unlock;
	}
	/* update the new slave's lag_id */
	if (!nbl_lag_id_valid(lag_id))
		lag_id = lag_mem->lag_id;

	if (update_flag) {
		nbl_update_lag_cfg(lag_mem, lag_id, update_flag);
		nbl_display_lag_info(dev_mgt, lag_id);
	}

unlock:
	mutex_unlock(&nbl_lag_mutex);

	return NOTIFY_DONE;
}

u32 nbl_lag_get_other_active_members(struct nbl_dev_mgt *dev_mgt,
				     u16 eth_list[], u32 array_size)
{
	u32 active_count = 0;
//	u8 lag_id;
//	struct nbl_adapter *adapter;
//	struct nbl_lag_member *lag_mem;
//	struct nbl_lag_instance *lag_info;
//	struct nbl_lag_member *mem_tmp;
//	struct list_head *iter;
//	const char *upper_name;
//
//	lag_mgt = NBL_RES_MGT_TO_LAG_MGT(res_mgt);
//	lag_mem = &lag_mgt->lag_mem;
//	lag_id = lag_mem->lag_id;
//
//	if (!nbl_lag_id_valid(lag_id)) {
//		nbl_err(NBL_ADAPTER_TO_COMMON(adapter), NBL_DEBUG_MAIN,
//			"params err, lag_id: %u.\n", lag_id);
//		return active_count;
//	}
//
//	mutex_lock(&nbl_lag_mutex);
//
//	lag_info = &nbl_lag_info[lag_id];
//
//	upper_name = lag_info->bond_netdev ? netdev_name(lag_info->bond_netdev) : "unset";
//	nbl_debug(NBL_ADAPTER_TO_COMMON(adapter), NBL_DEBUG_MAIN,
//		  "bond dev %s: lag_id: %u, enabled: %u.\n",
//		  upper_name, lag_id, lag_info->lag_enable);
//
//	if (lag_info->lag_enable) {
//		list_for_each(iter, &lag_info->mem_list_head) {
//			mem_tmp = list_entry(iter, struct nbl_lag_member, mem_list_node);
//			if (mem_tmp == lag_mem)
//				continue;
//			if (nbl_lag_mem_is_active(mem_tmp) && active_count < array_size)
//				eth_list[active_count++] = mem_tmp->eth_id;
//
//			nbl_debug(NBL_ADAPTER_TO_COMMON(adapter), NBL_DEBUG_MAIN,
//				  "eth_id: %u, bonded: %u, linkup: %u, tx_enabled: %u, "
//				  "active_count: %u, array_size: %u.\n",
//				  mem_tmp->logic_eth_id, mem_tmp->bonded,
//				  mem_tmp->lower_state.link_up,
//				  mem_tmp->lower_state.tx_enabled,
//				  active_count, array_size);
//		}
//	}
//
//	mutex_unlock(&nbl_lag_mutex);
	return active_count;
}

static void nbl_unregister_lag_handler(struct nbl_dev_mgt *dev_mgt)
{
	struct nbl_dev_net *net_dev = dev_mgt->net_dev;
	struct nbl_lag_member *lag_mem;
	struct notifier_block *notif_blk;
	struct netdev_net_notifier *netdevice_nn;

	lag_mem = net_dev->lag_mem;
	notif_blk = &lag_mem->notify_block;
	if (notif_blk->notifier_call) {
		netdevice_nn = &lag_mem->netdevice_nn;
		unregister_netdevice_notifier_dev_net(net_dev->netdev, notif_blk, netdevice_nn);
		nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			 "nbl lag event handler unregistered.\n");
	}
}

static int nbl_register_lag_handler(struct nbl_dev_mgt *dev_mgt)
{
	struct nbl_dev_net *net_dev = dev_mgt->net_dev;
	struct notifier_block *notify_blk;
	struct nbl_lag_member *lag_mem;
	struct netdev_net_notifier *netdevice_nn;

	lag_mem = net_dev->lag_mem;
	notify_blk = &lag_mem->notify_block;

	/* register the lag related event handler function for each device */
	if (!notify_blk->notifier_call) {
		notify_blk->notifier_call = nbl_lag_event_handler;
		netdevice_nn = &lag_mem->netdevice_nn;
		if (register_netdevice_notifier_dev_net(net_dev->netdev,
							notify_blk, netdevice_nn)) {
			notify_blk->notifier_call = NULL;
			nbl_err(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
				"Failed to register nbl lag event handler!\n");
			return -EINVAL;
		}
		nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			 "nbl lag event handler registered.\n");
	}
	return 0;
}

static int nbl_lag_alloc_resource(struct nbl_dev_mgt *dev_mgt)
{
	struct nbl_lag_resource *lag_resource_tmp;
	u32 lag_resource_num = 0;
	u32 board_key;

	board_key = pci_domain_nr(dev_mgt->common->pdev->bus) << 16 |
			dev_mgt->common->pdev->bus->number;
	/* find the lag resource with the bus_id firstly, increasing the refcount if found */
	list_for_each_entry(lag_resource_tmp, &lag_resource_head, resource_node) {
		lag_resource_num++;
		if (lag_resource_tmp->board_key == board_key) {
			kref_get(&lag_resource_tmp->kref);
			goto ret_ok;
		}
	}

	/* checking the max cards we supported */
	if (lag_resource_num >= NBL_LAG_MAX_RESOURCE_NUM) {
		nbl_err(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			"Lag resource num %u exceed the max num %u.\n",
			lag_resource_num, NBL_LAG_MAX_RESOURCE_NUM);
		goto ret_fail;
	}

	/* alloc the lag resource when the card's first device registered */
	lag_resource_tmp = kzalloc(sizeof(*lag_resource_tmp), GFP_KERNEL);
	if (!lag_resource_tmp)
		goto ret_fail;
	kref_init(&lag_resource_tmp->kref);
	lag_resource_tmp->board_key = board_key;
	INIT_LIST_HEAD(&lag_resource_tmp->lag_instance_head);
	nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
		 "Alloc lag resource for board_key 0x%x, refcount %u.\n",
		 board_key, kref_read(&lag_resource_tmp->kref));
	/* add the new lag resource into the resource list */
	list_add_tail(&lag_resource_tmp->resource_node, &lag_resource_head);

ret_ok:
	nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
		 "Return lag resource for board_key 0x%x, refcount %u.\n",
		 board_key, kref_read(&lag_resource_tmp->kref));
	return 0;
ret_fail:
	return -1;
}

static void delete_and_free_lag_resource(struct kref *kref)
{
	struct nbl_lag_resource *lag_resource_tmp;
	struct nbl_lag_instance *lag_info, *lag_tmp;

	lag_resource_tmp = container_of(kref, struct nbl_lag_resource, kref);

	/* release all lag instances firstly */
	list_for_each_entry_safe(lag_info, lag_tmp,
				 &lag_resource_tmp->lag_instance_head, instance_node) {
		list_del(&lag_info->instance_node);
		kfree(lag_info);
	}

	list_del(&lag_resource_tmp->resource_node);
	kfree(lag_resource_tmp);
}

static void nbl_lag_free_resource(struct nbl_dev_mgt *dev_mgt)
{
	struct nbl_lag_resource *lag_resource, *lag_resource_tmp;
	int ret;
	u32 board_key;

	board_key = pci_domain_nr(dev_mgt->common->pdev->bus) << 16 |
			dev_mgt->common->pdev->bus->number;
	list_for_each_entry_safe(lag_resource, lag_resource_tmp,
				 &lag_resource_head, resource_node) {
		if (lag_resource->board_key == board_key) {
			/* release the lag resource */
			ret = kref_put(&lag_resource->kref, delete_and_free_lag_resource);
			nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
				 "Release the lag resource for board_key 0x%x, refcount %d.\n",
				 board_key, ret ? -1 : kref_read(&lag_resource->kref));
		}
	}
}

int nbl_init_lag(struct nbl_dev_mgt *dev_mgt, struct nbl_init_param *param)
{
	int ret = 0;
	struct nbl_dev_net *net_dev = dev_mgt->net_dev;
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_lag_member *lag_mem;
	u8 lag_id;

	if (!param->caps.support_lag)
		return 0;

	lag_mem = net_dev->lag_mem;
	if (!lag_mem) {
		lag_mem = devm_kzalloc(NBL_DEV_MGT_TO_DEV(dev_mgt),
				       sizeof(*net_dev->lag_mem), GFP_KERNEL);
		if (!lag_mem)
			return -ENOMEM;
	}

	lag_mem->bonded = 0;
	lag_mem->lower_state.link_up = 0;
	lag_mem->lower_state.tx_enabled = 0;
	memset(&lag_mem->notify_block, 0, sizeof(lag_mem->notify_block));
	lag_mem->vsi_id = NBL_COMMON_TO_VSI_ID(NBL_DEV_MGT_TO_COMMON(dev_mgt));
	lag_mem->lag_id = NBL_INVALID_LAG_ID;
	lag_mem->eth_id = NBL_DEV_MGT_TO_COMMON(dev_mgt)->eth_id;
	lag_mem->logic_eth_id = NBL_DEV_MGT_TO_COMMON(dev_mgt)->logic_eth_id;
	lag_mem->netdev = net_dev->netdev;
	net_dev->lag_mem = lag_mem;

	mutex_lock(&nbl_lag_mutex);
	ret = nbl_lag_alloc_resource(dev_mgt);
	if (ret) {
		nbl_err(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			"Failed to alloc lag resource.\n");
		goto err_alloc;
	}

	for (lag_id = 0; nbl_lag_id_valid(lag_id); lag_id++)
		nbl_display_lag_info(dev_mgt, lag_id);

	mutex_unlock(&nbl_lag_mutex);

	ret = nbl_register_lag_handler(dev_mgt);
	if (ret) {
		nbl_err(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN,
			"Failed to register nbl lag event handler\n");
		goto err_register;
	}

	ret = serv_ops->register_indr_dev_tc_offload(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
						     net_dev->netdev);
	if (ret)
		goto err_reg_lag_tc_offload;

	net_dev->lag_inited = 1;

	nbl_info(NBL_DEV_MGT_TO_COMMON(dev_mgt), NBL_DEBUG_MAIN, "Init the nbl lag success.\n");
	return 0;

err_reg_lag_tc_offload:
	nbl_unregister_lag_handler(dev_mgt);
err_register:
	devm_kfree(NBL_DEV_MGT_TO_DEV(dev_mgt), net_dev->lag_mem);
	net_dev->lag_mem = NULL;
	nbl_lag_free_resource(dev_mgt);
	return ret;
err_alloc:
	devm_kfree(NBL_DEV_MGT_TO_DEV(dev_mgt), net_dev->lag_mem);
	net_dev->lag_mem = NULL;
	mutex_unlock(&nbl_lag_mutex);
	return ret;
}

int nbl_deinit_lag(struct nbl_dev_mgt *dev_mgt)
{
	struct nbl_dev_net *net_dev = dev_mgt->net_dev;
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);

	if (!net_dev->lag_inited)
		return 0;

	serv_ops->unregister_indr_dev_tc_offload(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
						 net_dev->netdev);
	nbl_unregister_lag_handler(dev_mgt);

	mutex_lock(&nbl_lag_mutex);
	nbl_lag_free_resource(dev_mgt);
	mutex_unlock(&nbl_lag_mutex);

	if (net_dev->lag_mem)
		devm_kfree(NBL_DEV_MGT_TO_DEV(dev_mgt), net_dev->lag_mem);
	net_dev->lag_mem = NULL;

	return 0;
}
