// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#include "nbl_resource_leonis.h"

MODULE_VERSION(NBL_LEONIS_DRIVER_VERSION);

static void nbl_res_setup_common_ops(struct nbl_resource_mgt *res_mgt)
{
}

static int nbl_res_pf_to_eth_id(struct nbl_resource_mgt *res_mgt, u16 pf_id)
{
	struct nbl_eth_info *eth_info = NBL_RES_MGT_TO_ETH_INFO(res_mgt);

	if (pf_id >= NBL_MAX_PF)
		return 0;

	return eth_info->eth_id[pf_id];
}

static u32 nbl_res_get_pfvf_queue_num(struct nbl_resource_mgt *res_mgt, int pfid, int vfid)
{
	struct nbl_resource_info *res_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	struct nbl_net_ring_num_info *num_info = &res_info->net_ring_num_info;
	u16 func_id = nbl_res_pfvfid_to_func_id(res_mgt, pfid, vfid);
	u32 queue_num = 0;

	if (vfid >= 0) {
		if (num_info->net_max_qp_num[func_id] != 0)
			queue_num = num_info->net_max_qp_num[func_id];
		else
			queue_num = num_info->vf_def_max_net_qp_num;
	} else {
		if (num_info->net_max_qp_num[func_id] != 0)
			queue_num = num_info->net_max_qp_num[func_id];
		else
			queue_num = num_info->pf_def_max_net_qp_num;
	}

	if (queue_num > NBL_MAX_TXRX_QUEUE_PER_FUNC) {
		nbl_warn(NBL_RES_MGT_TO_COMMON(res_mgt), NBL_DEBUG_QUEUE,
			 "Invalid queue num %u for func %d, use default", queue_num, func_id);
		queue_num = vfid >= 0 ? NBL_DEFAULT_VF_HW_QUEUE_NUM : NBL_DEFAULT_PF_HW_QUEUE_NUM;
	}

	return queue_num;
}

static void nbl_res_get_rep_queue_info(void *priv, u16 *queue_num, u16 *queue_size)
{
	*queue_size = NBL_DEFAULT_DESC_NUM;
	*queue_num = NBL_DEFAULT_REP_HW_QUEUE_NUM;
}

static void nbl_res_get_user_queue_info(void *priv, u16 *queue_num, u16 *queue_size, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *res_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	struct nbl_net_ring_num_info *num_info = &res_info->net_ring_num_info;
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	u16 default_queue;

	if (num_info->net_max_qp_num[func_id] != 0)
		default_queue = num_info->net_max_qp_num[func_id];
	else
		default_queue = num_info->pf_def_max_net_qp_num;

	*queue_num = min_t(u16, default_queue, NBL_VSI_PF_LEGACY_QUEUE_NUM_MAX - default_queue);
	*queue_size = NBL_DEFAULT_DESC_NUM;

	if (*queue_num > NBL_MAX_TXRX_QUEUE_PER_FUNC) {
		nbl_warn(NBL_RES_MGT_TO_COMMON(res_mgt), NBL_DEBUG_QUEUE,
			 "Invalid user queue num %d for func %d, use default", *queue_num, func_id);
		*queue_num = NBL_DEFAULT_PF_HW_QUEUE_NUM;
	}
}

static int __maybe_unused nbl_res_get_queue_num(struct nbl_resource_mgt *res_mgt,
						u16 func_id, u16 *tx_queue_num, u16 *rx_queue_num)
{
	int pfid, vfid;

	nbl_res_func_id_to_pfvfid(res_mgt, func_id, &pfid, &vfid);

	*tx_queue_num = nbl_res_get_pfvf_queue_num(res_mgt, pfid, vfid);
	*rx_queue_num = nbl_res_get_pfvf_queue_num(res_mgt, pfid, vfid);

	return 0;
}

static int nbl_res_save_vf_bar_info(struct nbl_resource_mgt *res_mgt,
				    u16 func_id, struct nbl_register_net_param *register_param)
{
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_sriov_info *sriov_info = &NBL_RES_MGT_TO_SRIOV_INFO(res_mgt)[func_id];
	u64 pf_bar_start;
	u64 vf_bar_start;
	u16 pf_bdf;
	u64 vf_bar_size;
	u16 total_vfs;
	u16 offset;
	u16 stride;

	if (func_id < NBL_RES_MGT_TO_PF_NUM(res_mgt)) {
		pf_bar_start = phy_ops->get_pf_bar_addr(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), func_id);
		sriov_info->pf_bar_start = pf_bar_start;
		dev_info(dev, "sriov_info, pf_bar_start:%llx\n", sriov_info->pf_bar_start);
	}

	pf_bdf = (u16)sriov_info->bdf;
	vf_bar_size = register_param->vf_bar_size;
	total_vfs = register_param->total_vfs;
	offset = register_param->offset;
	stride = register_param->stride;

	if (total_vfs) {
		sriov_info->offset = offset;
		sriov_info->stride = stride;
		vf_bar_start = phy_ops->get_vf_bar_addr(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), func_id);
		sriov_info->vf_bar_start = vf_bar_start;
		sriov_info->vf_bar_len = vf_bar_size / total_vfs;

		dev_info(dev, "sriov_info, bdf:%x:%x.%x, num_vfs:%d, start_vf_func_id:%d,",
			 PCI_BUS_NUM(pf_bdf), PCI_SLOT(pf_bdf & 0xff), PCI_FUNC(pf_bdf & 0xff),
			 sriov_info->num_vfs, sriov_info->start_vf_func_id);
		dev_info(dev, "offset:%d, stride:%d, vf_bar_start: %llx",
			 offset, stride, sriov_info->vf_bar_start);
	}

	return 0;
}

static int nbl_res_prepare_vf_chan(struct nbl_resource_mgt *res_mgt,
				   u16 func_id, struct nbl_register_net_param *register_param)
{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_sriov_info *sriov_info = &NBL_RES_MGT_TO_SRIOV_INFO(res_mgt)[func_id];
	u16 total_vfs;
	u16 offset;
	u16 stride;
	u8 pf_bus;
	u8 pf_devfn;
	u16 vf_id;
	u8 bus;
	u8 devfn;
	u8 devid;
	u8 function;
	u16 vf_func_id;

	total_vfs = register_param->total_vfs;
	offset = register_param->offset;
	stride = register_param->stride;

	if (total_vfs) {
		/* Configure mailbox qinfo_map_table for the pf's all vf,
		 * so vf's mailbox is ready, vf can use mailbox.
		 */
		pf_bus = PCI_BUS_NUM(sriov_info->bdf);
		pf_devfn = sriov_info->bdf & 0xff;
		for (vf_id = 0; vf_id < sriov_info->num_vfs; vf_id++) {
			vf_func_id = sriov_info->start_vf_func_id + vf_id;

			bus = pf_bus + ((pf_devfn + offset + stride * vf_id) >> 8);
			devfn = (pf_devfn + offset + stride * vf_id) & 0xff;
			devid = PCI_SLOT(devfn);
			function = PCI_FUNC(devfn);

			phy_ops->cfg_mailbox_qinfo(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						   vf_func_id, bus, devid, function);
		}
	}

	return 0;
}

static int nbl_res_update_active_vf_num(struct nbl_resource_mgt *res_mgt, u16 func_id,
					bool add_flag)
{
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_resource_info *resource_info = res_mgt->resource_info;
	struct nbl_sriov_info *sriov_info = res_mgt->resource_info->sriov_info;
	int pfid = 0;
	int vfid = 0;
	int ret;

	ret = nbl_res_func_id_to_pfvfid(res_mgt, func_id, &pfid, &vfid);
	if (ret) {
		nbl_err(common, NBL_DEBUG_RESOURCE, "convert func id to pfvfid failed\n");
		return ret;
	}

	if (vfid == U32_MAX)
		return 0;

	if (add_flag) {
		if (!test_bit(func_id, resource_info->func_bitmap)) {
			sriov_info[pfid].active_vf_num++;
			set_bit(func_id, resource_info->func_bitmap);
		}
	} else if (sriov_info[pfid].active_vf_num) {
		if (test_bit(func_id, resource_info->func_bitmap)) {
			sriov_info[pfid].active_vf_num--;
			clear_bit(func_id, resource_info->func_bitmap);
		}
	}

	return 0;
}

static u32 nbl_res_get_quirks(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->get_quirks(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
}

static int nbl_res_register_net(void *priv, u16 func_id,
				struct nbl_register_net_param *register_param,
				struct nbl_register_net_result *register_result)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_vsi_info *vsi_info = NBL_RES_MGT_TO_VSI_INFO(res_mgt);
	struct nbl_resource_info *resource_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	struct nbl_vdpa_status **vf_status = NBL_RES_MGT_TO_VDPA_VF_STATS(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	netdev_features_t csumo_features = 0;
	netdev_features_t tso_features = 0;
	netdev_features_t pf_features = 0;
	netdev_features_t vlano_features = 0;
	u16 tx_queue_num, rx_queue_num;
	u8 mac[ETH_ALEN] = {0};
	u32 quirks;
	u16 vsi_id;
	int ret = 0;

	if (func_id < NBL_MAX_PF) {
		nbl_res_get_eth_mac(res_mgt, mac, nbl_res_pf_to_eth_id(res_mgt, func_id));
		pf_features = NBL_FEATURE(NETIF_F_NTUPLE);
		register_result->trusted = 1;
	} else {
		ether_addr_copy(mac, vsi_info->mac_info[func_id].mac);
		register_result->trusted = vsi_info->mac_info[func_id].trusted;
	}
	ether_addr_copy(register_result->mac, mac);

	quirks = nbl_res_get_quirks(res_mgt);
	if (performance_mode & BIT(NBL_QUIRKS_NO_TOE) ||
	    !(quirks & BIT(NBL_QUIRKS_NO_TOE))) {
		csumo_features = NBL_FEATURE(NETIF_F_RXCSUM) |
				NBL_FEATURE(NETIF_F_IP_CSUM) |
				NBL_FEATURE(NETIF_F_IPV6_CSUM);
		tso_features = NBL_FEATURE(NETIF_F_TSO) |
			NBL_FEATURE(NETIF_F_TSO6) |
			NBL_FEATURE(NETIF_F_GSO_UDP_L4);
	}

	if (func_id < NBL_MAX_PF) /* vf unsupport */
		vlano_features = NBL_FEATURE(NETIF_F_HW_VLAN_CTAG_TX) |
				 NBL_FEATURE(NETIF_F_HW_VLAN_CTAG_RX) |
				 NBL_FEATURE(NETIF_F_HW_VLAN_STAG_TX) |
				 NBL_FEATURE(NETIF_F_HW_VLAN_STAG_RX);

	register_result->hw_features |= pf_features |
					csumo_features |
					tso_features |
					vlano_features |
					NBL_FEATURE(NETIF_F_SG) |
					NBL_FEATURE(NETIF_F_HW_TC) |
					NBL_FEATURE(NETIF_F_RXHASH);

	register_result->features |= register_result->hw_features |
				     NBL_FEATURE(NETIF_F_HW_TC) |
				     NBL_FEATURE(NETIF_F_HW_VLAN_CTAG_FILTER) |
				     NBL_FEATURE(NETIF_F_HW_VLAN_STAG_FILTER);

	register_result->vlan_features = register_result->features;

	register_result->max_mtu = NBL_MAX_JUMBO_FRAME_SIZE - NBL_PKT_HDR_PAD;

	register_result->vlan_proto = vsi_info->mac_info[func_id].vlan_proto;
	register_result->vlan_tci = vsi_info->mac_info[func_id].vlan_tci;
	register_result->rate = vsi_info->mac_info[func_id].rate;

	if (loongarch_low_version) {
		if (common->is_vf) {
			tx_queue_num = NBL_LOONGSON64_VF_MAX_QUEUE_NUM;
			rx_queue_num = NBL_LOONGSON64_VF_MAX_QUEUE_NUM;
		} else {
			tx_queue_num = NBL_LOONGSON64_MAX_QUEUE_NUM;
			rx_queue_num = NBL_LOONGSON64_MAX_QUEUE_NUM;
		}
	} else {
		nbl_res_get_queue_num(res_mgt, func_id, &tx_queue_num, &rx_queue_num);
	}

	register_result->tx_queue_num = tx_queue_num;
	register_result->rx_queue_num = rx_queue_num;
	register_result->queue_size = NBL_DEFAULT_DESC_NUM;

	ret = nbl_res_update_active_vf_num(res_mgt, func_id, 1);
	if (ret) {
		nbl_err(common, NBL_DEBUG_RESOURCE, "change active vf num failed with ret: %d\n",
			ret);
		goto update_active_vf_fail;
	}

	if (register_param->is_vdpa) {
		set_bit(func_id, resource_info->vdpa.vdpa_func_bitmap);

		if (!vf_status[func_id]) {
			vf_status[func_id] = devm_kzalloc(dev, sizeof(struct nbl_vdpa_status),
							  GFP_KERNEL);
			if (!vf_status[func_id]) {
				ret = -ENOMEM;
				goto alloc_nbl_vf_stats_fail;
			}
		}
		vsi_id = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_DATA);
		phy_ops->get_dstat_vsi_stat(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id,
					    &vf_status[func_id]->init_stats.tx_packets,
					    &vf_status[func_id]->init_stats.tx_bytes);
		phy_ops->get_ustat_vsi_stat(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id,
					    &vf_status[func_id]->init_stats.rx_packets,
					    &vf_status[func_id]->init_stats.rx_bytes);
		memcpy(&vf_status[func_id]->prev_stats, &vf_status[func_id]->init_stats,
		       sizeof(vf_status[func_id]->prev_stats));
		vf_status[func_id]->timestamp = jiffies;
	} else {
		clear_bit(func_id, resource_info->vdpa.vdpa_func_bitmap);
	}

	if (func_id >= NBL_RES_MGT_TO_PF_NUM(res_mgt))
		return 0;

	ret = nbl_res_save_vf_bar_info(res_mgt, func_id, register_param);
	if (ret)
		goto save_vf_bar_info_fail;

	ret = nbl_res_prepare_vf_chan(res_mgt, func_id, register_param);
	if (ret)
		goto prepare_vf_chan_fail;

	nbl_res_open_sfp(res_mgt, nbl_res_pf_to_eth_id(res_mgt, func_id));

	return ret;

prepare_vf_chan_fail:
save_vf_bar_info_fail:
alloc_nbl_vf_stats_fail:
update_active_vf_fail:
	return ret;
}

static int nbl_res_unregister_net(void *priv, u16 func_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	return nbl_res_update_active_vf_num(res_mgt, func_id, 0);
}

static u16 nbl_res_get_vsi_id(void *priv, u16 func_id, u16 type)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	return nbl_res_func_id_to_vsi_id(res_mgt, func_id, type);
}

static void nbl_res_get_eth_id(void *priv, u16 vsi_id, u8 *eth_mode, u8 *eth_id, u8 *logic_eth_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_eth_info *eth_info = NBL_RES_MGT_TO_ETH_INFO(res_mgt);
	u16 pf_id = nbl_res_vsi_id_to_pf_id(res_mgt, vsi_id);

	*eth_mode = eth_info->eth_num;
	if (pf_id < eth_info->eth_num) {
		*eth_id = eth_info->eth_id[pf_id];
		*logic_eth_id = pf_id;
	/* if pf_id > eth_num, use eth_id 0 */
	} else {
		*eth_id = eth_info->eth_id[0];
		*logic_eth_id = 0;
	}
}

static DEFINE_IDA(nbl_adev_ida);

static void nbl_res_setup_rdma_id(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *resource_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	u16 func_id;

	for_each_set_bit(func_id, resource_info->rdma_info.func_cap, NBL_MAX_FUNC)
		resource_info->rdma_info.rdma_id[func_id] = ida_alloc(&nbl_adev_ida, GFP_KERNEL);
}

static void nbl_res_remove_rdma_id(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *resource_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	u16 func_id;

	for_each_set_bit(func_id, resource_info->rdma_info.func_cap, NBL_MAX_FUNC)
		ida_free(&nbl_adev_ida, resource_info->rdma_info.rdma_id[func_id]);
}

static void nbl_res_register_rdma(void *priv, u16 vsi_id, struct nbl_rdma_register_param *param)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *resource_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);

	/* Even if we don't have capability, we would still return mem_type */
	param->has_rdma = false;
	param->mem_type = resource_info->rdma_info.mem_type;

	if (test_bit(func_id, resource_info->rdma_info.func_cap)) {
		param->has_rdma = true;
		param->intr_num = NBL_RES_RDMA_INTR_NUM;

		param->id = resource_info->rdma_info.rdma_id[func_id];
	}
}

static void nbl_res_unregister_rdma(void *priv, u16 vsi_id)
{
}

static void nbl_res_register_rdma_bond(void *priv, struct nbl_lag_member_list_param *list_param,
				       struct nbl_rdma_register_param *register_param)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *resource_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	u16 func_id = 0;
	int i;

	register_param->has_rdma = false;
	register_param->mem_type = resource_info->rdma_info.mem_type;

	/* Rdma bond can be created only if all members have rdma cap */
	for (i = 0; i < list_param->lag_num; i++) {
		func_id = nbl_res_vsi_id_to_func_id(res_mgt, list_param->member_list[i].vsi_id);

		if (!test_bit(func_id, resource_info->rdma_info.func_cap))
			return;
	}

	register_param->has_rdma = true;
	register_param->intr_num = NBL_RES_RDMA_INTR_NUM;
}

static void nbl_res_unregister_rdma_bond(void *priv, u16 lag_id)
{
}

static u8 __iomem *nbl_res_get_hw_addr(void *priv, size_t *size)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->get_hw_addr(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), size);
}

static u64 nbl_res_get_real_hw_addr(void *priv, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);

	return nbl_res_get_func_bar_base_addr(res_mgt, func_id);
}

static u16 nbl_res_get_function_id(void *priv, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	return nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
}

static void nbl_res_get_real_bdf(void *priv, u16 vsi_id, u8 *bus, u8 *dev, u8 *function)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);

	nbl_res_func_id_to_bdf(res_mgt, func_id, bus, dev, function);
}

static u32 nbl_res_check_active_vf(void *priv, u16 func_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_sriov_info *sriov_info = res_mgt->resource_info->sriov_info;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	int pfid = 0;
	int vfid = 0;
	int ret;

	ret = nbl_res_func_id_to_pfvfid(res_mgt, func_id, &pfid, &vfid);
	if (ret) {
		nbl_err(common, NBL_DEBUG_RESOURCE, "convert func id to pfvfid failed\n");
		return ret;
	}

	return sriov_info[pfid].active_vf_num;
}

static void nbl_res_set_dport_fc_th_vld(void *priv, u8 eth_id, bool vld)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->set_dport_fc_th_vld(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), eth_id, vld);
}

static void nbl_res_set_shaping_dport_vld(void *priv, u8 eth_id, bool vld)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->set_shaping_dport_vld(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), eth_id, vld);
}

static int nbl_res_set_phy_flow(struct nbl_resource_mgt *res_mgt, u8 eth_id, bool status)
{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_vsi_info *vsi_info = NBL_RES_MGT_TO_VSI_INFO(res_mgt);
	u8 pf_id = nbl_res_eth_id_to_pf_id(res_mgt, eth_id);
	int i, ret = 0;

	for (i = 0; i < NBL_VSI_SERV_MAX_TYPE; i++) {
		ret = phy_ops->cfg_phy_flow(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					    vsi_info->serv_info[pf_id][i].base_id,
					    vsi_info->serv_info[pf_id][i].num, eth_id, status);
		if (ret)
			return ret;
	}

	nbl_res_set_dport_fc_th_vld(res_mgt, eth_id, !status);
	nbl_res_set_shaping_dport_vld(res_mgt, eth_id, !status);
	phy_ops->cfg_eth_port_priority_replace(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), eth_id, status);

	return 0;
}

static void nbl_res_get_base_mac_addr(void *priv, u8 *mac)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	nbl_res_get_eth_mac(res_mgt, mac, nbl_res_pf_to_eth_id(res_mgt, 0));
}

static int nbl_res_update_offload_status(struct nbl_resource_mgt_leonis *res_mgt_leonis)
{
	struct nbl_rep_offload_status *rep_status =
				&res_mgt_leonis->pmd_status.rep_status;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_eth_bond_info *eth_bond_info = NBL_RES_MGT_TO_ETH_BOND_INFO(res_mgt);
	struct nbl_event_acl_state_update_data event_data = {0};
	struct nbl_sriov_info *sriov_info;
	bool status;
	int i, j, start, end, vsi_match, eth_id, eth_tmp, lag_id, ret = 0;

	for (i = 0; i < NBL_RES_MGT_TO_PF_NUM(res_mgt); i++) {
		status = false;
		eth_id = nbl_res_pf_to_eth_id(res_mgt, i);

		start = nbl_res_pfvfid_to_vsi_id(res_mgt, i, U32_MAX, NBL_VSI_DATA);
		sriov_info = NBL_RES_MGT_TO_SRIOV_INFO(res_mgt) + i;
		end = nbl_res_pfvfid_to_vsi_id(res_mgt, i, sriov_info->num_vfs, NBL_VSI_DATA);
		vsi_match = find_next_bit(rep_status->rep_vsi_bitmap,
					  NBL_OFFLOAD_STATUS_MAX_VSI, start);
		if (vsi_match <= end || test_bit(eth_id, rep_status->rep_eth_bitmap))
			status = true;

		if (rep_status->status[eth_id] != status) {
			ret = nbl_res_set_phy_flow(res_mgt, eth_id, status);
			if (ret)
				return ret;
			rep_status->status[eth_id] = status;
		}
	}

	/* Update bond offload status.
	 * For bond, there will be only one pf is bind to ovs-dpdk, but all pfs should
	 * change to offload.
	 */
	for (i = 0; i < NBL_RES_MGT_TO_PF_NUM(res_mgt); i++) {
		status = false;
		eth_id = nbl_res_pf_to_eth_id(res_mgt, i);
		lag_id = nbl_res_eth_id_to_lag_id(res_mgt, eth_id);

		if (lag_id >= 0 && lag_id < NBL_LAG_MAX_NUM) {
			for (j = 0; j < eth_bond_info->entry[lag_id].lag_num &&
			     NBL_ETH_BOND_VALID_PORT(j); j++) {
				/* If bond, any port is offload means all ports are offload */
				eth_tmp = eth_bond_info->entry[lag_id].eth_id[j];
				if (rep_status->status[eth_tmp]) {
					status = true;
					break;
				}
			}

			if (rep_status->status[eth_id] != status) {
				ret = nbl_res_set_phy_flow(res_mgt, eth_id, status);
				if (ret)
					return ret;
				rep_status->status[eth_id] = status;
			}
		}
	}

	event_data.is_offload = false;

	for (i = 0; i < NBL_RES_MGT_TO_PF_NUM(res_mgt); i++) {
		eth_id = nbl_res_pf_to_eth_id(res_mgt, i);
		if (rep_status->status[eth_id])
			event_data.is_offload = true;
	}

	nbl_event_notify(NBL_EVENT_ACL_STATE_UPDATE, &event_data, NBL_COMMON_TO_VSI_ID(common),
			 NBL_COMMON_TO_BOARD_ID(common));

	return 0;
}

static int nbl_res_set_pmd_debug(void *priv, bool pmd_debug)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
				(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_rep_offload_status *rep_status =
				&res_mgt_leonis->pmd_status.rep_status;

	rep_status->pmd_debug = pmd_debug;
	return 0;
}

static void nbl_res_set_offload_status(void *priv, u16 func_id)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
				(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_upcall_port_info *upcall_port_info =
				&res_mgt_leonis->pmd_status.upcall_port_info;
	struct nbl_rep_offload_status *rep_status =
				&res_mgt_leonis->pmd_status.rep_status;

	if (!upcall_port_info->upcall_port_active ||
	    upcall_port_info->func_id != func_id)
		return;

	rep_status->timestamp = jiffies;
}

static void nbl_res_vdpa_itr_update(struct nbl_resource_mgt *res_mgt,
				    u16 func_id, bool active)
{
	struct nbl_vdpa_info *vdpa_info = &res_mgt->resource_info->vdpa;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_vdpa_status *vdpa_vf_stats = vdpa_info->vf_stats[func_id];
	struct nbl_vf_stats cur_stats = {0}, *prev_stats;
	u64 tx_rates = 0, rx_rates = 0, pkt_rates = 0, time_diff;
	u16 itr_level = 0;
	u16 vsi_id;

	if (!vdpa_vf_stats)
		return;

	if (active) {
		vsi_id = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_DATA);
		phy_ops->get_dstat_vsi_stat(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id,
					    &cur_stats.tx_packets, &cur_stats.tx_bytes);
		phy_ops->get_ustat_vsi_stat(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id,
					    &cur_stats.rx_packets, &cur_stats.rx_bytes);

		time_diff = jiffies - vdpa_vf_stats->timestamp;
		if (time_diff > 0) {
			prev_stats = &vdpa_vf_stats->prev_stats;
			tx_rates = (cur_stats.tx_packets - prev_stats->tx_packets) / time_diff * HZ;
			rx_rates = (cur_stats.rx_packets - prev_stats->rx_packets) / time_diff * HZ;
			pkt_rates = max_t(u64, tx_rates, rx_rates);

			itr_level = nbl_res_intr_get_suppress_level(res_mgt, pkt_rates,
								    vdpa_vf_stats->itr_level);
		} else {
			itr_level = vdpa_vf_stats->itr_level;
		}

		memcpy(&vdpa_vf_stats->prev_stats, &cur_stats, sizeof(cur_stats));
		vdpa_vf_stats->timestamp = jiffies;
	}

	if (itr_level != vdpa_vf_stats->itr_level) {
		nbl_res_intr_set_intr_suppress_level(res_mgt, func_id, 0, U16_MAX, itr_level);
		vdpa_vf_stats->itr_level = itr_level;
	}
}

static int nbl_res_check_offload_status(void *priv, bool *is_down)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
				(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_resource_info *res_info = res_mgt->resource_info;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_upcall_port_info *upcall_port_info =
				&res_mgt_leonis->pmd_status.upcall_port_info;
	struct nbl_rep_offload_status *rep_status =
				&res_mgt_leonis->pmd_status.rep_status;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	int i;
	u16 func_id;
	u32 start, batch_cnt;

	if (!upcall_port_info->upcall_port_active)
		return 0;

	/* check pmd debug, no check if pmd_debug is on */
	if (rep_status->pmd_debug) {
		nbl_info(common, NBL_DEBUG_FLOW, "pmd is in debug mode now");
		rep_status->timestamp = jiffies;
		return 0;
	}

	start = res_info->vdpa.start;
	batch_cnt = NBL_VDPA_ITR_BATCH_CNT;
	if (rep_status->timestamp && time_after(jiffies, rep_status->timestamp + 30 * HZ)) {
		for (i = 0; i < NBL_OFFLOAD_STATUS_MAX_VSI; i++)
			clear_bit(i, rep_status->rep_vsi_bitmap);

		for (i = 0; i < NBL_OFFLOAD_STATUS_MAX_ETH; i++)
			clear_bit(i, rep_status->rep_eth_bitmap);

		upcall_port_info->upcall_port_active = false;
		nbl_err(common, NBL_DEBUG_FLOW, "offload found inactive!");
		phy_ops->clear_profile_table_action(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
		phy_ops->ipro_chksum_err_ctrl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), 0);
		nbl_res_update_offload_status(res_mgt_leonis);
		*is_down = true;

		start = 0;
		batch_cnt = NBL_MAX_FUNC;
	}

	i = 0;
	for (; start < NBL_MAX_FUNC;) {
		func_id = find_next_bit(res_info->vdpa.vdpa_func_bitmap, NBL_MAX_FUNC, start);
		if (func_id >= NBL_MAX_FUNC) {
			start = 0;
			break;
		}
		i++;
		start = func_id + 1;

		nbl_res_vdpa_itr_update(res_mgt, func_id,
					upcall_port_info->upcall_port_active);
		if (i >= batch_cnt)
			break;
	}

	res_info->vdpa.start = start;

	return 0;
}

static void nbl_res_get_rep_feature(void *priv, struct nbl_register_net_result *register_result)
{
	netdev_features_t csumo_features;

	csumo_features = NBL_FEATURE(NETIF_F_RXCSUM) |
			 NBL_FEATURE(NETIF_F_IP_CSUM) |
			 NBL_FEATURE(NETIF_F_IPV6_CSUM) |
			 NBL_FEATURE(NETIF_F_SCTP_CRC);
	register_result->hw_features = csumo_features | NBL_FEATURE(NETIF_F_HW_TC);
	register_result->features |= csumo_features | NBL_FEATURE(NETIF_F_HW_TC);
	register_result->max_mtu = NBL_MAX_JUMBO_FRAME_SIZE - NBL_PKT_HDR_PAD;
}

static void nbl_res_set_eswitch_mode(void *priv, u16 switch_mode)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *resource_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_tx_ring *tx_ring;
	struct nbl_res_rx_ring *rx_ring;
	int i;

	resource_info->eswitch_info->mode = switch_mode;

	/* set ring info switch_mode */
	for (i = 0; i < txrx_mgt->rx_ring_num; i++) {
		rx_ring = NBL_RES_MGT_TO_RX_RING(res_mgt, i);
		tx_ring = NBL_RES_MGT_TO_TX_RING(res_mgt, i);

		tx_ring->mode = switch_mode;
		rx_ring->mode = switch_mode;
	}
}

static u16 nbl_res_get_eswitch_mode(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *resource_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);

	if (resource_info->eswitch_info)
		return resource_info->eswitch_info->mode;
	else
		return NBL_ESWITCH_NONE;
}

static int nbl_res_alloc_rep_data(void *priv, int num_vfs, u16 vf_base_vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_eswitch_info *eswitch_info = NBL_RES_MGT_TO_ESWITCH_INFO(res_mgt);
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);

	eswitch_info->rep_data = devm_kcalloc(dev, num_vfs,
					      sizeof(struct nbl_rep_data), GFP_KERNEL);
	if (!eswitch_info->rep_data)
		return -ENOMEM;
	eswitch_info->num_vfs = num_vfs;
	eswitch_info->vf_base_vsi_id = vf_base_vsi_id;
	return 0;
}

static void nbl_res_free_rep_data(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_eswitch_info **eswitch_info = &NBL_RES_MGT_TO_ESWITCH_INFO(res_mgt);
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);

	if ((*eswitch_info)->rep_data) {
		devm_kfree(dev, (*eswitch_info)->rep_data);
		(*eswitch_info)->rep_data = NULL;
	}
	(*eswitch_info)->num_vfs = 0;
}

static void nbl_res_set_rep_netdev_info(void *priv, void *rep_data)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_eswitch_info *eswitch_info = NBL_RES_MGT_TO_ESWITCH_INFO(res_mgt);
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_rep_data *rep = (struct nbl_rep_data *)rep_data;
	u16 rep_data_index;

	rep_data_index = nbl_res_get_rep_idx(eswitch_info, rep->rep_vsi_id);
	if (rep_data_index >= eswitch_info->num_vfs)
		return;
	eswitch_info->rep_data[rep_data_index].rep_vsi_id = rep->rep_vsi_id;
	eswitch_info->rep_data[rep_data_index].netdev = rep->netdev;
	nbl_info(common, NBL_DEBUG_RESOURCE, "nbl set rep netdev rep_vsi_id %d netdev %p\n",
		 rep->rep_vsi_id, rep->netdev);
}

static void nbl_res_unset_rep_netdev_info(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_eswitch_info *eswitch_info = NBL_RES_MGT_TO_ESWITCH_INFO(res_mgt);

	memset(eswitch_info->rep_data, 0,
	       eswitch_info->num_vfs * sizeof(struct nbl_rep_data));
}

static struct net_device *nbl_res_get_rep_netdev_info(void *priv, u16 rep_data_index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_eswitch_info *eswitch_info = NBL_RES_MGT_TO_ESWITCH_INFO(res_mgt);

	if (rep_data_index >= eswitch_info->num_vfs)
		return NULL;
	return eswitch_info->rep_data[rep_data_index].netdev;
}

static int nbl_res_disable_phy_flow(void *priv, u8 eth_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	return nbl_res_set_phy_flow(res_mgt, eth_id, true);
}

static int nbl_res_enable_phy_flow(void *priv, u8 eth_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	return nbl_res_set_phy_flow(res_mgt, eth_id, false);
}

static void nbl_res_init_acl(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	if (!res_mgt->resource_info->init_acl_refcnt)
		phy_ops->init_acl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));

	res_mgt->resource_info->init_acl_refcnt++;
}

static void nbl_res_uninit_acl(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	res_mgt->resource_info->init_acl_refcnt--;

	if (!res_mgt->resource_info->init_acl_refcnt)
		phy_ops->uninit_acl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
}

static int nbl_res_set_upcall_rule(void *priv, u8 eth_id, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->set_upcall_rule(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), eth_id, vsi_id);
}

static int nbl_res_unset_upcall_rule(void *priv, u8 eth_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->unset_upcall_rule(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), eth_id);
}

static void nbl_res_get_rep_stats(void *priv, u16 rep_vsi_id,
				  struct nbl_rep_stats *rep_stats, bool is_tx)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_eswitch_info *eswitch_info = NBL_RES_MGT_TO_ESWITCH_INFO(res_mgt);
	struct nbl_rep_data *rep_data;
	unsigned int start;
	u16 rep_data_index = 0;

	if (!eswitch_info || eswitch_info->mode != NBL_ESWITCH_OFFLOADS ||
	    ((nbl_res_get_rep_idx(eswitch_info, rep_vsi_id)) == U32_MAX))
		return;

	rep_data_index = nbl_res_get_rep_idx(eswitch_info, rep_vsi_id);
	if (rep_data_index >= eswitch_info->num_vfs)
		return;
	rep_data = &eswitch_info->rep_data[rep_data_index];
	if (rep_data->rep_vsi_id != rep_vsi_id)
		return;

	if (is_tx) {
		do {
			start = u64_stats_fetch_begin(&rep_data->rep_syncp);
			rep_stats->packets = rep_data->tx_packets;
			rep_stats->bytes   = rep_data->tx_bytes;
		} while (u64_stats_fetch_retry(&rep_data->rep_syncp, start));
	} else {
		do {
			start = u64_stats_fetch_begin(&rep_data->rep_syncp);
			rep_stats->packets = rep_data->rx_packets;
			rep_stats->bytes   = rep_data->rx_bytes;
		} while (u64_stats_fetch_retry(&rep_data->rep_syncp, start));
	}
}

static u16 nbl_res_get_rep_index(void *priv, u16 rep_vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_eswitch_info *eswitch_info = NBL_RES_MGT_TO_ESWITCH_INFO(res_mgt);

	return nbl_res_get_rep_idx(eswitch_info, rep_vsi_id);
}

static void nbl_res_register_net_rep(void *priv, u16 pf_id, u16 vf_id,
				     struct nbl_register_net_rep_result *result)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
				(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_rep_offload_status *rep_status =
				&res_mgt_leonis->pmd_status.rep_status;
	int pf_id_tmp, vf_id_tmp;

	pf_id_tmp = pf_id;
	if (vf_id == U16_MAX)
		vf_id_tmp = U32_MAX;
	else
		vf_id_tmp = vf_id;

	result->vsi_id = nbl_res_pfvfid_to_vsi_id(res_mgt, pf_id_tmp, vf_id_tmp, NBL_VSI_DATA);
	result->func_id = nbl_res_pfvfid_to_func_id(res_mgt, pf_id_tmp, vf_id_tmp);

	if (result->vsi_id >= NBL_OFFLOAD_STATUS_MAX_VSI) {
		nbl_err(common, NBL_DEBUG_RESOURCE,
			"register_net_rep pf %d vf %d vsi_id %d err\n",
			pf_id, vf_id, result->vsi_id);
		return;
	}

	set_bit(result->vsi_id, rep_status->rep_vsi_bitmap);
	nbl_res_update_offload_status(res_mgt_leonis);
}

static void nbl_res_unregister_net_rep(void *priv, u16 vsi_id)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
				(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_rep_offload_status *rep_status =
				&res_mgt_leonis->pmd_status.rep_status;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	if (vsi_id >= NBL_OFFLOAD_STATUS_MAX_VSI) {
		nbl_err(common, NBL_DEBUG_RESOURCE,
			"unregister_net_rep vsi_id %d err\n", vsi_id);
		return;
	}

	/* set rss to l4 */
	phy_ops->set_epro_rss_default(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id);
	clear_bit(vsi_id, rep_status->rep_vsi_bitmap);
	nbl_res_update_offload_status(res_mgt_leonis);
}

static void nbl_res_register_eth_rep(void *priv, u8 eth_id)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
				(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_rep_offload_status *rep_status =
				&res_mgt_leonis->pmd_status.rep_status;

	if (eth_id >= NBL_OFFLOAD_STATUS_MAX_ETH) {
		nbl_err(common, NBL_DEBUG_RESOURCE,
			"register_eth_rep eth_id %d err\n", eth_id);
		return;
	}
	set_bit(eth_id, rep_status->rep_eth_bitmap);
	nbl_res_update_offload_status(res_mgt_leonis);
}

static void nbl_res_unregister_eth_rep(void *priv, u8 eth_id)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
				(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_rep_offload_status *rep_status =
				&res_mgt_leonis->pmd_status.rep_status;

	if (eth_id >= NBL_OFFLOAD_STATUS_MAX_ETH) {
		nbl_err(common, NBL_DEBUG_RESOURCE,
			"unregister_eth_rep eth_id %d err\n", eth_id);
		return;
	}

	clear_bit(eth_id, rep_status->rep_eth_bitmap);
	nbl_res_update_offload_status(res_mgt_leonis);
}

static int nbl_res_register_upcall_port(void *priv, u16 func_id)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
				(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_upcall_port_info *upcall_port_info =
				&res_mgt_leonis->pmd_status.upcall_port_info;
	struct nbl_rep_offload_status *rep_status =
				&res_mgt_leonis->pmd_status.rep_status;
	u16 vsi_id = nbl_res_func_id_to_vsi_id(&res_mgt_leonis->res_mgt, func_id,
					       NBL_VSI_SERV_PF_DATA_TYPE);
	int i;

	rep_status->timestamp = jiffies;

	if (!upcall_port_info->upcall_port_active) {
		upcall_port_info->func_id = func_id;
		upcall_port_info->upcall_port_active = true;

		for (i = 0; i < NBL_OFFLOAD_STATUS_MAX_VSI; i++)
			clear_bit(i, rep_status->rep_vsi_bitmap);

		for (i = 0; i < NBL_OFFLOAD_STATUS_MAX_ETH; i++)
			clear_bit(i, rep_status->rep_eth_bitmap);

		set_bit(vsi_id, rep_status->rep_vsi_bitmap);

		nbl_res_update_offload_status(res_mgt_leonis);
		return 0;
	}

	if (func_id != upcall_port_info->func_id) {
		nbl_err(NBL_RES_MGT_TO_COMMON(&res_mgt_leonis->res_mgt), NBL_DEBUG_RESOURCE,
			"can not add rep port with two pf port, register_upcall_port failed\n");
		return -EINVAL;
	}

	return 0;
}

static void nbl_res_unregister_upcall_port(void *priv, u16 func_id)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
				(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_upcall_port_info *upcall_port_info =
				&res_mgt_leonis->pmd_status.upcall_port_info;
	struct nbl_rep_offload_status *rep_status =
				&res_mgt_leonis->pmd_status.rep_status;
	int i;

	if (!upcall_port_info->upcall_port_active ||
	    upcall_port_info->func_id != func_id) {
		nbl_err(common, NBL_DEBUG_RESOURCE,
			"func_id %d unregister upcall failed\n", func_id);
		return;
	}

	for (i = 0; i < NBL_OFFLOAD_STATUS_MAX_VSI; i++)
		clear_bit(i, rep_status->rep_vsi_bitmap);

	for (i = 0; i < NBL_OFFLOAD_STATUS_MAX_ETH; i++)
		clear_bit(i, rep_status->rep_eth_bitmap);

	nbl_res_update_offload_status(res_mgt_leonis);
	upcall_port_info->upcall_port_active = false;
}

static void nbl_res_init_offload_fwd(void *priv, u16 func_id)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
		(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->init_offload_fwd(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), func_id);
}

static void nbl_res_init_cmdq(void *priv, void *data, u16 func_id)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
		(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_chan_cmdq_init_info *cmdq_param =
		(struct nbl_chan_cmdq_init_info *)data;
	u8 bus;
	u8 dev;
	u8 func;

	nbl_res_func_id_to_bdf(res_mgt, func_id, &bus, &dev, &func);
	cmdq_param->bdf_num = (u16)PCI_DEVID(bus, PCI_DEVFN(dev, func));

	phy_ops->init_cmdq(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), data, func_id);
}

static void nbl_res_destroy_cmdq(void *priv)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
		(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->destroy_cmdq(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
}

static void nbl_res_reset_cmdq(void *priv)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
		(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->reset_cmdq(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
}

static void nbl_res_init_rep(void *priv, u16 vsi_id, u8 inner_type,
			     u8 outer_type, u8 rep_type)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
		(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->init_rep(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id,
			  inner_type, outer_type, rep_type);
}

static void nbl_res_init_flow(void *priv, void *param)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
		(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->init_flow(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), param);
}

static void nbl_res_deinit_flow(void *priv)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
		(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->deinit_flow(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
}

static void nbl_res_offload_flow_rule(void *priv, void *data)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
		(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->offload_flow_rule(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), data);
}

static void nbl_res_get_flow_acl_switch(void *priv, u8 *acl_enable)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
		(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->get_flow_acl_switch(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				     acl_enable);
}

static void nbl_res_get_line_rate_info(void *priv, void *data, void *result)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
		(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->get_line_rate_info(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), data, result);
}

/* return value need to convert to Mil degree Celsius(1/1000) */
static u32 nbl_res_get_chip_temperature(void *priv, enum nbl_hwmon_type type, u32 senser_id)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
		(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->get_chip_temperature(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), type, senser_id);
}

static int nbl_res_init_vdpaq(void *priv, u16 func_id, u64 pa, u32 size)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
		(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	u8 bus, dev, func;
	u16 bdf;

	nbl_res_func_id_to_bdf(res_mgt, func_id, &bus, &dev, &func);
	bdf = PCI_DEVID(bus, PCI_DEVFN(dev, func));

	return phy_ops->init_vdpaq(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), func_id, bdf, pa, size);
}

static void nbl_res_destroy_vdpaq(void *priv)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
		(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->destroy_vdpaq(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
}

static int nbl_res_get_upcall_port(void *priv, u16 *bdf)
{
	struct nbl_resource_mgt_leonis *res_mgt_leonis =
				(struct nbl_resource_mgt_leonis *)priv;
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_upcall_port_info *upcall_port_info =
				&res_mgt_leonis->pmd_status.upcall_port_info;
	u8 bus, dev, func;

	if (!upcall_port_info->upcall_port_active)
		return U32_MAX;

	nbl_res_func_id_to_bdf(res_mgt, upcall_port_info->func_id, &bus, &dev, &func);
	*bdf = (u16)PCI_DEVID(common->bus, PCI_DEVFN(dev, func));
	return 0;
}

static void nbl_res_get_reg_dump(void *priv, u32 *data, u32 len)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->get_reg_dump(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), data, len);
}

static int nbl_res_get_reg_dump_len(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->get_reg_dump_len(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
}

static int nbl_res_process_abnormal_event(void *priv, struct nbl_abnormal_event_info *abnomal_info)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->process_abnormal_event(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), abnomal_info);
}

static int nbl_res_cfg_lag_hash_algorithm(void *priv, u16 eth_id, u16 lag_id,
					  enum netdev_lag_hash hash_type)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->cfg_lag_hash_algorithm(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					       eth_id, lag_id, hash_type);
}

static int nbl_res_cfg_lag_member_fwd(void *priv, u16 eth_id, u16 lag_id, u8 fwd)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->cfg_lag_member_fwd(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), eth_id, lag_id, fwd);
}

static int nbl_res_cfg_lag_member_list(void *priv, struct nbl_lag_member_list_param *param)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->cfg_lag_member_list(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), param);
}

static int nbl_res_cfg_lag_member_up_attr(void *priv, u16 eth_id, u16 lag_id, bool enable)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->cfg_lag_member_up_attr(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					       eth_id, lag_id, enable);
}

static int nbl_res_cfg_bond_shaping(void *priv, u8 eth_id, bool enable)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->cfg_bond_shaping(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), eth_id,
					 res_mgt->resource_info->board_info.eth_speed, enable);
}

static void nbl_res_cfg_bgid_back_pressure(void *priv, u8 main_eth_id, u8 other_eth_id, bool enable)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->cfg_bgid_back_pressure(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), main_eth_id, other_eth_id,
					enable, res_mgt->resource_info->board_info.eth_speed);
}

static int nbl_res_switchdev_init_cmdq(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_channel_mgt *chan_mgt = NBL_RES_MGT_TO_CHAN_PRIV(res_mgt);
	struct nbl_channel_ops *chan_ops = NBL_RES_MGT_TO_CHAN_OPS(res_mgt);
	struct device *dev = NBL_COMMON_TO_DEV(NBL_RES_MGT_TO_COMMON(res_mgt));

	return chan_ops->init_cmdq(dev, chan_mgt);
}

static int nbl_res_switchdev_deinit_cmdq(void *priv, u8 index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_channel_mgt *chan_mgt = NBL_RES_MGT_TO_CHAN_PRIV(res_mgt);
	struct nbl_channel_ops *chan_ops = NBL_RES_MGT_TO_CHAN_OPS(res_mgt);
	struct device *dev = NBL_COMMON_TO_DEV(NBL_RES_MGT_TO_COMMON(res_mgt));

	return chan_ops->deinit_cmdq(dev, chan_mgt, index);
}

static int nbl_res_set_tc_flow_info(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_tc_flow_mgt *tc_flow_mgt = NBL_RES_MGT_TO_TC_FLOW_MGT(res_mgt);
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	int i = 0;

	if (common->tc_inst_id >= NBL_TC_FLOW_INST_COUNT) {
		nbl_err(common, NBL_DEBUG_FLOW, "tc flow set inst_id=%d is invalid.\n",
			common->tc_inst_id);
		return -EINVAL;
	}

	if (!tc_flow_mgt->pf_set_tc_count) {
		nbl_tc_set_flow_info(tc_flow_mgt, common->tc_inst_id);
		nbl_info(common, NBL_DEBUG_FLOW, "tc flow set inst_id=%d success.\n",
			 common->tc_inst_id);

		nbl_info(common, NBL_DEBUG_FLOW, "tc flow set kgen cvlan zero, set ped vsi type zero\n");
		phy_ops->set_tc_kgen_cvlan_zero(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));

		for (i = 0; i < NBL_TPID_PORT_NUM; i++)
			phy_ops->set_ped_tab_vsi_type(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), i, 0);
	}

	tc_flow_mgt->pf_set_tc_count++;
	phy_ops->ipro_chksum_err_ctrl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), 1);
	nbl_info(common, NBL_DEBUG_FLOW, "tc flow set pf_set_tc_count++=%d\n",
		 tc_flow_mgt->pf_set_tc_count);

	return 0;
}

static int nbl_res_unset_tc_flow_info(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_tc_flow_mgt *tc_flow_mgt = NBL_RES_MGT_TO_TC_FLOW_MGT(res_mgt);
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	int ret = 0;
	int i = 0;

	if (common->tc_inst_id >= NBL_TC_FLOW_INST_COUNT) {
		nbl_err(common, NBL_DEBUG_FLOW, "tc flow unset inst_id=%d is invalid.\n",
			common->tc_inst_id);
		return -EINVAL;
	}

	tc_flow_mgt->pf_set_tc_count--;
	nbl_info(common, NBL_DEBUG_FLOW, "tc flow set pf_set_tc_count--=%d\n",
		 tc_flow_mgt->pf_set_tc_count);

	if (!tc_flow_mgt->pf_set_tc_count) {
		ret = nbl_tc_flow_flush_flow(res_mgt);
		if (ret)
			return -EINVAL;

		nbl_info(common, NBL_DEBUG_FLOW, "tc flow unset kgen cvlan, set ped vsi type zero\n");
		phy_ops->unset_tc_kgen_cvlan(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));

		for (i = 0; i < NBL_TPID_PORT_NUM; i++) {
			if (tc_flow_mgt->port_tpid_type[i] != 0) {
				phy_ops->set_ped_tab_vsi_type(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
							      i, 0);
				tc_flow_mgt->port_tpid_type[i] = 0;
			}
		}

		nbl_tc_unset_flow_info(common->tc_inst_id);
		nbl_info(common, NBL_DEBUG_FLOW, "tc flow unset inst_id=%d success.\n",
			 common->tc_inst_id);

		phy_ops->ipro_chksum_err_ctrl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), 0);
	}

	return 0;
}

static int nbl_res_get_tc_flow_info(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);

	if (common->tc_inst_id >= NBL_TC_FLOW_INST_COUNT) {
		nbl_err(common, NBL_DEBUG_FLOW, "tc flow get inst_id=%d is invalid.\n",
			common->tc_inst_id);
		return -EINVAL;
	}

	if (NBL_COMMON_TO_PCI_FUNC_ID(common))
		NBL_RES_MGT_TO_TC_FLOW_MGT(res_mgt) = nbl_tc_get_flow_info(common->tc_inst_id);
	nbl_info(common, NBL_DEBUG_FLOW, "tc flow get inst_id=%d success.\n",
		 common->tc_inst_id);

	return 0;
}

static int nbl_res_get_driver_info(void *priv, struct nbl_driver_info *driver_info)
{
	strscpy(driver_info->driver_version, NBL_LEONIS_DRIVER_VERSION,
		sizeof(driver_info->driver_version));
	return 1;
}

static int nbl_res_get_p4_info(void *priv, char *verify_code)
{
	/* We actually only care about the snic-v3r1 part, won't check m181xx */
	strscpy(verify_code, "snic_v3r1_m181xx", NBL_P4_NAME_LEN);

	return NBL_P4_DEFAULT;
}

static int nbl_res_get_p4_used(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *resource_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);

	return resource_info->p4_used;
}

static int nbl_res_set_p4_used(void *priv, int p4_type)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *resource_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);

	resource_info->p4_used = p4_type;

	return 0;
}

static u32 nbl_res_get_p4_version(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	return res_mgt->resource_info->board_info.p4_version;
}

static int nbl_res_load_p4(void *priv, struct nbl_load_p4_param *p4_param)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	if (p4_param->start || p4_param->end)
		return 0;

	phy_ops->load_p4(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), p4_param->addr,
			 p4_param->size, p4_param->data);

	return 0;
}

static void nbl_res_get_board_info(void *priv, struct nbl_board_port_info *board_info)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	memcpy(board_info, &res_mgt->resource_info->board_info, sizeof(*board_info));
}

static u16 nbl_res_get_vf_base_vsi_id(void *priv, u16 pf_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	return nbl_res_pfvfid_to_vsi_id(res_mgt, pf_id, 0, NBL_VSI_DATA);
}

static void nbl_res_flr_clear_net(void *priv, u16 vf_id)
{
	u16 func_id = vf_id + NBL_MAX_PF;
	u16 vsi_id;

	vsi_id = nbl_res_func_id_to_vsi_id(priv, func_id, NBL_VSI_SERV_VF_DATA_TYPE);
	nbl_res_unregister_rdma(priv, vsi_id);

	if (nbl_res_vf_is_active(priv, func_id))
		nbl_res_unregister_net(priv, func_id);
}

static void nbl_res_flr_clear_rdma(void *priv, u16 vf_id)
{
	u16 func_id = vf_id + NBL_MAX_PF;
	u16 vsi_id;

	vsi_id = nbl_res_func_id_to_vsi_id(priv, func_id, NBL_VSI_SERV_VF_DATA_TYPE);
	nbl_res_unregister_rdma(priv, vsi_id);
}

static u16 nbl_res_covert_vfid_to_vsi_id(void *priv, u16 vf_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	u16 func_id = vf_id + NBL_MAX_PF;

	return nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_SERV_VF_DATA_TYPE);
}

static bool nbl_res_check_vf_is_active(void *priv, u16 func_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	return nbl_res_vf_is_active(res_mgt, func_id);
}

static int nbl_res_check_vf_is_vdpa(void *priv, u16 func_id, u8 *is_vdpa)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *resource_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);

	*is_vdpa = test_bit(func_id, resource_info->vdpa.vdpa_func_bitmap);
	return 0;
}

static int nbl_res_get_vdpa_vf_stats(void *priv, u16 func_id, struct nbl_vf_stats *vf_stats)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct nbl_vdpa_status *vdpa_vf_stats = NULL;
	struct nbl_vf_stats vdpa_vf_stats_current = {0}, *init_stats;
	u16 vsi_id;

	if (NBL_RES_MGT_TO_VDPA_VF_STATS(res_mgt) &&
	    NBL_RES_MGT_TO_VDPA_VF_STATS(res_mgt)[func_id]) {
		vdpa_vf_stats = NBL_RES_MGT_TO_VDPA_VF_STATS(res_mgt)[func_id];
		init_stats = &vdpa_vf_stats->init_stats;
	} else {
		dev_err(dev, "function %d vdpa_vf_stats is NULL\n", func_id);
		return -EFAULT;
	}

	vsi_id = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_DATA);
	phy_ops->get_dstat_vsi_stat(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id,
				    &vdpa_vf_stats_current.tx_packets,
				    &vdpa_vf_stats_current.tx_bytes);
	phy_ops->get_ustat_vsi_stat(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id,
				    &vdpa_vf_stats_current.rx_packets,
				    &vdpa_vf_stats_current.rx_bytes);

	vf_stats->tx_packets = vdpa_vf_stats_current.tx_packets - init_stats->tx_packets;
	vf_stats->tx_bytes = vdpa_vf_stats_current.tx_bytes - init_stats->tx_bytes;
	vf_stats->rx_packets = vdpa_vf_stats_current.rx_packets - init_stats->rx_packets;
	vf_stats->rx_bytes = vdpa_vf_stats_current.rx_bytes - init_stats->rx_bytes;

	return 0;
}

static int nbl_res_get_ustore_pkt_drop_stats(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_eth_info *eth_info = NBL_RES_MGT_TO_ETH_INFO(res_mgt);
	struct nbl_ustore_stats *ustore_stats = NBL_RES_MGT_TO_USTORE_STATS(res_mgt);
	struct nbl_ustore_stats ustore_stats_temp = {0};
	u8 eth_id = 0;
	int i = 0;

	for (i = 0; i < eth_info->eth_num; i++) {
		eth_id = eth_info->eth_id[i];
		phy_ops->get_ustore_pkt_drop_stats(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						   eth_id, &ustore_stats_temp);
		ustore_stats[eth_id].rx_drop_packets += ustore_stats_temp.rx_drop_packets;
		ustore_stats[eth_id].rx_trun_packets += ustore_stats_temp.rx_trun_packets;
	}

	return 0;
}

static int nbl_res_get_ustore_total_pkt_drop_stats(void *priv, u8 eth_id,
						   struct nbl_ustore_stats *nbl_ustore_stats)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_ustore_stats *ustore_stats = NBL_RES_MGT_TO_USTORE_STATS(res_mgt);

	nbl_ustore_stats->rx_drop_packets = ustore_stats[eth_id].rx_drop_packets;
	nbl_ustore_stats->rx_trun_packets = ustore_stats[eth_id].rx_trun_packets;
	return 0;
}

static int nbl_res_get_board_id(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);

	return NBL_COMMON_TO_BOARD_ID(common);
}

static int nbl_res_cfg_eth_bond_info(void *priv, struct nbl_lag_member_list_param *param)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_eth_bond_info *eth_bond_info = NBL_RES_MGT_TO_ETH_BOND_INFO(res_mgt);
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_eth_bond_entry origin_entry;
	struct nbl_eth_bond_entry *entry = NULL;
	struct nbl_event_link_status_update_data *event_data = NULL;
	u8 eth_btm[NBL_MAX_ETHERNET] = {0};
	int num = 0, i = 0, j = 0;

	if (param->lag_id >= NBL_LAG_MAX_NUM)
		return -EINVAL;

	entry = &eth_bond_info->entry[param->lag_id];
	memcpy(&origin_entry, entry, sizeof(origin_entry));

	/* We always clear it first, in case lag member changed. */
	memset(entry, 0, sizeof(*entry));

	if (param->lag_num > 1) {
		for (i = 0; i < param->lag_num && NBL_ETH_BOND_VALID_PORT(i); i++) {
			entry->eth_id[i] = param->member_list[i].eth_id;
			eth_btm[param->member_list[i].eth_id] = 1;
		}

		entry->lag_id = param->lag_id;
		entry->lag_num = param->lag_num;
	}

	/* If lag member changed, notify both original and new related vfs to update link_state */
	for (i = 0; i < origin_entry.lag_num && NBL_ETH_BOND_VALID_PORT(i); i++)
		eth_btm[origin_entry.eth_id[i]] = 1;

	for (i = 0; i < NBL_MAX_ETHERNET; i++)
		if (eth_btm[i])
			num++;

	nbl_res_update_offload_status((struct nbl_resource_mgt_leonis *)res_mgt);

	event_data = kzalloc(sizeof(*event_data), GFP_KERNEL);
	if (!event_data)
		return -ENOMEM;

	for (i = 0; i < NBL_MAX_ETHERNET; i++)
		if (eth_btm[i])
			event_data->eth_id[j++] = i;

	event_data->num = num;

	nbl_event_notify(NBL_EVENT_LINK_STATE_UPDATE, event_data,
			 NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));

	kfree(event_data);
	return 0;
}

static int nbl_res_get_eth_bond_info(void *priv, struct nbl_bond_param *param)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_eth_bond_info *eth_bond_info = NBL_RES_MGT_TO_ETH_BOND_INFO(res_mgt);
	struct nbl_eth_bond_entry *entry = NULL;
	int num = 0, i = 0, j = 0, pf_id = 0;

	for (i = 0; i < NBL_LAG_MAX_NUM; i++) {
		entry = &eth_bond_info->entry[i];

		if (entry->lag_num < NBL_LAG_VALID_PORTS || entry->lag_num > NBL_LAG_MAX_PORTS)
			continue;

		for (j = 0; j < entry->lag_num; j++) {
			pf_id = nbl_res_eth_id_to_pf_id(res_mgt, entry->eth_id[j]);

			param->info[num].port[j].eth_id = entry->eth_id[j];
			param->info[num].port[j].vsi_id =
				nbl_res_pfvfid_to_vsi_id(res_mgt, pf_id, -1, NBL_VSI_DATA);
			param->info[num].port[j].is_active =
				phy_ops->get_lag_fwd(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						     entry->eth_id[j]);
		}

		param->info[num].mem_num = entry->lag_num;
		param->info[num].lag_id = entry->lag_id;

		num++;
	}

	param->lag_num = num;

	return 0;
}

static void nbl_res_get_driver_version(void *priv, char *ver, int len)
{
	strscpy(ver, NBL_LEONIS_DRIVER_VERSION, len);
}

static void nbl_res_get_xdp_queue_info(void *priv, u16 *queue_num, u16 *queue_size, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *res_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	struct nbl_net_ring_num_info *num_info = &res_info->net_ring_num_info;
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	u16 default_queue;

	if (num_info->net_max_qp_num[func_id] != 0)
		default_queue = num_info->net_max_qp_num[func_id];
	else
		default_queue = num_info->pf_def_max_net_qp_num;

	*queue_num = min_t(u16, default_queue, NBL_VSI_PF_LEGACY_QUEUE_NUM_MAX - default_queue);

	if (*queue_num > NBL_MAX_TXRX_QUEUE_PER_FUNC) {
		nbl_warn(NBL_RES_MGT_TO_COMMON(res_mgt), NBL_DEBUG_QUEUE,
			 "Invalid xdp queue num %d for func %d, use default", *queue_num, func_id);
		*queue_num = NBL_DEFAULT_PF_HW_QUEUE_NUM;
	}
}

static int nbl_res_get_pfc_buffer_size(void *priv, u8 eth_id, u8 prio, int *xoff, int *xon)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->get_pfc_buffer_size(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), eth_id, prio, xoff, xon);

	return 0;
}

static int nbl_res_set_pfc_buffer_size(void *priv, u8 eth_id, u8 prio, int xoff, int xon)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->set_pfc_buffer_size(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), eth_id,
					    prio, xoff, xon);
}

static int nbl_res_configure_qos(void *priv, u8 eth_id, u8 *pfc, u8 trust, u8 *dscp2prio_map)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->configure_qos(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), eth_id, pfc, trust, dscp2prio_map);

	return 0;
}

static int nbl_res_configure_rdma_bw(void *priv, u8 eth_id, int rdma_bw)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->configure_rdma_bw(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), eth_id, rdma_bw);

	return 0;
}

static int nbl_res_set_rate_limit(void *priv, u16 func_id, enum nbl_traffic_type type, u32 rate)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->set_rate_limit(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), func_id, type, rate);

	return 0;
}

static u32 nbl_res_get_perf_dump_length(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->get_perf_dump_length(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
}

static u32 nbl_res_get_perf_dump_data(void *priv, u8 *buffer, u32 length)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->get_perf_dump_data(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), buffer, length);
}

static void nbl_res_register_dev_name(void *priv, u16 vsi_id, char *name)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *resource_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	u32 pf_id;

	pf_id = nbl_res_vsi_id_to_pf_id(res_mgt, vsi_id);
	WARN_ON(pf_id >= NBL_MAX_PF);
	strscpy(resource_info->pf_name_list[pf_id], name, IFNAMSIZ);
	nbl_info(NBL_RES_MGT_TO_COMMON(res_mgt), NBL_DEBUG_RESOURCE,
		 "vsi:%u-pf:%u register a pf_name->%s", vsi_id, pf_id, name);
}

static void nbl_res_get_dev_name(void *priv, u16 vsi_id, char *name)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *resource_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	int pf_id, vf_id;
	u16 func_id;
	int name_len;

	func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	nbl_res_func_id_to_pfvfid(res_mgt, func_id, &pf_id, &vf_id);
	WARN_ON(pf_id >= NBL_MAX_PF);
	name_len = snprintf(name, IFNAMSIZ, "%sv%d", resource_info->pf_name_list[pf_id], vf_id);
	if (name_len >= IFNAMSIZ)
		nbl_err(NBL_RES_MGT_TO_COMMON(res_mgt), NBL_DEBUG_RESOURCE,
			"vsi:%u-pf%uvf%u get name over length", vsi_id, pf_id, vf_id);

	nbl_debug(NBL_RES_MGT_TO_COMMON(res_mgt), NBL_DEBUG_RESOURCE,
		  "vsi:%u-pf%uvf%u get a pf_name->%s", vsi_id, pf_id, vf_id, name);
}

static int nbl_res_get_mirror_table_id(void *priv, u16 vsi_id, int dir, bool mirror_en,
				       u8 *mt_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->get_mirror_table_id(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					    vsi_id, dir, mirror_en, mt_id);
}

static int nbl_res_configure_mirror(void *priv, u16 func_id, bool mirror_en, int dir,
				    u8 mt_id)
{
	u16 data_vsi, user_vsi;
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	data_vsi = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_SERV_PF_DATA_TYPE);
	user_vsi = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_SERV_PF_USER_TYPE);

	phy_ops->configure_mirror(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), data_vsi, mirror_en, dir,
				  mt_id);
	phy_ops->configure_mirror(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), user_vsi, mirror_en, dir,
				  mt_id);

	return 0;
}

static int nbl_res_clear_mirror_cfg(void *priv, u16 func_id)
{
	u16 data_vsi, user_vsi;
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	data_vsi = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_SERV_PF_DATA_TYPE);
	user_vsi = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_SERV_PF_USER_TYPE);

	phy_ops->clear_mirror_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), data_vsi);
	phy_ops->clear_mirror_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), user_vsi);

	return 0;
}

static struct nbl_resource_ops res_ops = {
	.register_net = nbl_res_register_net,
	.unregister_net = nbl_res_unregister_net,
	.check_active_vf = nbl_res_check_active_vf,
	.get_base_mac_addr = nbl_res_get_base_mac_addr,
	.get_vsi_id = nbl_res_get_vsi_id,
	.get_eth_id = nbl_res_get_eth_id,
	.get_rep_feature = nbl_res_get_rep_feature,
	.get_rep_queue_info = nbl_res_get_rep_queue_info,
	.get_user_queue_info = nbl_res_get_user_queue_info,
	.set_eswitch_mode = nbl_res_set_eswitch_mode,
	.get_eswitch_mode = nbl_res_get_eswitch_mode,
	.alloc_rep_data = nbl_res_alloc_rep_data,
	.free_rep_data = nbl_res_free_rep_data,
	.set_rep_netdev_info = nbl_res_set_rep_netdev_info,
	.unset_rep_netdev_info = nbl_res_unset_rep_netdev_info,
	.get_rep_netdev_info = nbl_res_get_rep_netdev_info,
	.disable_phy_flow = nbl_res_disable_phy_flow,
	.enable_phy_flow = nbl_res_enable_phy_flow,
	.init_acl = nbl_res_init_acl,
	.uninit_acl = nbl_res_uninit_acl,
	.set_upcall_rule = nbl_res_set_upcall_rule,
	.unset_upcall_rule = nbl_res_unset_upcall_rule,
	.set_shaping_dport_vld = nbl_res_set_shaping_dport_vld,
	.set_dport_fc_th_vld = nbl_res_set_dport_fc_th_vld,
	.get_rep_stats = nbl_res_get_rep_stats,
	.get_rep_index = nbl_res_get_rep_index,
	.setup_rdma_id = nbl_res_setup_rdma_id,
	.remove_rdma_id = nbl_res_remove_rdma_id,
	.register_rdma = nbl_res_register_rdma,
	.unregister_rdma = nbl_res_unregister_rdma,
	.register_rdma_bond = nbl_res_register_rdma_bond,
	.unregister_rdma_bond = nbl_res_unregister_rdma_bond,
	.get_hw_addr = nbl_res_get_hw_addr,
	.get_real_hw_addr = nbl_res_get_real_hw_addr,
	.get_function_id = nbl_res_get_function_id,
	.get_real_bdf = nbl_res_get_real_bdf,
	.get_product_flex_cap = nbl_res_get_flex_capability,
	.get_product_fix_cap = nbl_res_get_fix_capability,
	.register_net_rep = nbl_res_register_net_rep,
	.unregister_net_rep = nbl_res_unregister_net_rep,
	.register_eth_rep = nbl_res_register_eth_rep,
	.unregister_eth_rep = nbl_res_unregister_eth_rep,
	.register_upcall_port = nbl_res_register_upcall_port,
	.unregister_upcall_port = nbl_res_unregister_upcall_port,
	.check_offload_status = nbl_res_check_offload_status,
	.set_offload_status = nbl_res_set_offload_status,
	.init_offload_fwd = nbl_res_init_offload_fwd,
	.init_cmdq = nbl_res_init_cmdq,
	.destroy_cmdq = nbl_res_destroy_cmdq,
	.reset_cmdq = nbl_res_reset_cmdq,
	.init_rep = nbl_res_init_rep,
	.init_flow = nbl_res_init_flow,
	.deinit_flow = nbl_res_deinit_flow,
	.offload_flow_rule = nbl_res_offload_flow_rule,
	.get_flow_acl_switch = nbl_res_get_flow_acl_switch,
	.get_line_rate_info = nbl_res_get_line_rate_info,
	.get_chip_temperature = nbl_res_get_chip_temperature,
	.get_driver_info = nbl_res_get_driver_info,
	.get_board_info = nbl_res_get_board_info,
	.flr_clear_net = nbl_res_flr_clear_net,
	.flr_clear_rdma = nbl_res_flr_clear_rdma,
	.covert_vfid_to_vsi_id = nbl_res_covert_vfid_to_vsi_id,
	.check_vf_is_active = nbl_res_check_vf_is_active,
	.check_vf_is_vdpa = nbl_res_check_vf_is_vdpa,
	.get_vdpa_vf_stats = nbl_res_get_vdpa_vf_stats,
	.get_ustore_pkt_drop_stats = nbl_res_get_ustore_pkt_drop_stats,
	.get_ustore_total_pkt_drop_stats = nbl_res_get_ustore_total_pkt_drop_stats,

	.init_vdpaq = nbl_res_init_vdpaq,
	.destroy_vdpaq = nbl_res_destroy_vdpaq,
	.get_upcall_port = nbl_res_get_upcall_port,

	.get_reg_dump = nbl_res_get_reg_dump,
	.get_reg_dump_len = nbl_res_get_reg_dump_len,
	.process_abnormal_event = nbl_res_process_abnormal_event,

	.cfg_lag_hash_algorithm = nbl_res_cfg_lag_hash_algorithm,
	.cfg_lag_member_fwd = nbl_res_cfg_lag_member_fwd,
	.cfg_lag_member_list = nbl_res_cfg_lag_member_list,
	.cfg_lag_member_up_attr = nbl_res_cfg_lag_member_up_attr,
	.cfg_bond_shaping = nbl_res_cfg_bond_shaping,
	.cfg_bgid_back_pressure = nbl_res_cfg_bgid_back_pressure,

	.cfg_eth_bond_info = nbl_res_cfg_eth_bond_info,
	.get_eth_bond_info = nbl_res_get_eth_bond_info,

	.switchdev_init_cmdq = nbl_res_switchdev_init_cmdq,
	.switchdev_deinit_cmdq = nbl_res_switchdev_deinit_cmdq,
	.set_tc_flow_info = nbl_res_set_tc_flow_info,
	.unset_tc_flow_info = nbl_res_unset_tc_flow_info,
	.get_tc_flow_info = nbl_res_get_tc_flow_info,

	.get_p4_info = nbl_res_get_p4_info,
	.get_p4_used = nbl_res_get_p4_used,
	.set_p4_used = nbl_res_set_p4_used,
	.get_vf_base_vsi_id = nbl_res_get_vf_base_vsi_id,
	.load_p4 = nbl_res_load_p4,
	.get_p4_version = nbl_res_get_p4_version,

	.get_board_id = nbl_res_get_board_id,
	.set_pmd_debug = nbl_res_set_pmd_debug,

	.get_driver_version = nbl_res_get_driver_version,
	.get_xdp_queue_info = nbl_res_get_xdp_queue_info,
	.set_hw_status = nbl_res_set_hw_status,

	.configure_qos = nbl_res_configure_qos,
	.configure_rdma_bw = nbl_res_configure_rdma_bw,
	.set_pfc_buffer_size = nbl_res_set_pfc_buffer_size,
	.get_pfc_buffer_size = nbl_res_get_pfc_buffer_size,
	.set_rate_limit = nbl_res_set_rate_limit,

	.get_perf_dump_length = nbl_res_get_perf_dump_length,
	.get_perf_dump_data = nbl_res_get_perf_dump_data,

	.register_dev_name = nbl_res_register_dev_name,
	.get_dev_name = nbl_res_get_dev_name,

	.get_mirror_table_id = nbl_res_get_mirror_table_id,
	.configure_mirror = nbl_res_configure_mirror,
	.clear_mirror_cfg = nbl_res_clear_mirror_cfg,
};

static struct nbl_res_product_ops product_ops = {
	.queue_mgt_init			= nbl_queue_mgt_init_leonis,
	.setup_qid_map_table		= nbl_res_queue_setup_qid_map_table_leonis,
	.remove_qid_map_table		= nbl_res_queue_remove_qid_map_table_leonis,
	.init_qid_map_table		= nbl_res_queue_init_qid_map_table,
};

static bool is_ops_inited;
static int nbl_res_setup_res_mgt(struct nbl_common_info *common,
				 struct nbl_resource_mgt_leonis **res_mgt_leonis)
{
	struct device *dev;
	struct nbl_resource_info *resource_info;

	dev = NBL_COMMON_TO_DEV(common);
	*res_mgt_leonis = devm_kzalloc(dev, sizeof(struct nbl_resource_mgt_leonis), GFP_KERNEL);
	if (!*res_mgt_leonis)
		return -ENOMEM;
	NBL_RES_MGT_TO_COMMON(&(*res_mgt_leonis)->res_mgt) = common;

	resource_info = devm_kzalloc(dev, sizeof(struct nbl_resource_info), GFP_KERNEL);
	if (!resource_info)
		return -ENOMEM;
	NBL_RES_MGT_TO_RES_INFO(&(*res_mgt_leonis)->res_mgt) = resource_info;

	return 0;
}

static void nbl_res_remove_res_mgt(struct nbl_common_info *common,
				   struct nbl_resource_mgt_leonis **res_mgt_leonis)
{
	struct device *dev;

	dev = NBL_COMMON_TO_DEV(common);
	devm_kfree(dev, NBL_RES_MGT_TO_RES_INFO(&(*res_mgt_leonis)->res_mgt));
	devm_kfree(dev, *res_mgt_leonis);
	*res_mgt_leonis = NULL;
}

static void nbl_res_remove_ops(struct device *dev, struct nbl_resource_ops_tbl **res_ops_tbl)
{
	devm_kfree(dev, *res_ops_tbl);
	*res_ops_tbl = NULL;
}

static int nbl_res_setup_ops(struct device *dev, struct nbl_resource_ops_tbl **res_ops_tbl,
			     struct nbl_resource_mgt_leonis *res_mgt_leonis)
{
	int ret = 0;

	*res_ops_tbl = devm_kzalloc(dev, sizeof(struct nbl_resource_ops_tbl), GFP_KERNEL);
	if (!*res_ops_tbl)
		return -ENOMEM;

	if (!is_ops_inited) {
		ret = nbl_flow_setup_ops_leonis(&res_ops);
		if (ret)
			goto setup_fail;

		ret = nbl_tc_flow_setup_ops_leonis(&res_ops);
		if (ret)
			goto setup_fail;

		ret = nbl_queue_setup_ops_leonis(&res_ops);
		if (ret)
			goto setup_fail;

		ret = nbl_txrx_setup_ops(&res_ops);
		if (ret)
			goto setup_fail;

		ret = nbl_intr_setup_ops(&res_ops);
		if (ret)
			goto setup_fail;

		ret = nbl_vsi_setup_ops(&res_ops);
		if (ret)
			goto setup_fail;

		ret = nbl_adminq_setup_ops(&res_ops);
		if (ret)
			goto setup_fail;

		ret = nbl_accel_setup_ops(&res_ops);
		if (ret)
			goto setup_fail;

		ret = nbl_fd_setup_ops(&res_ops);
		if (ret)
			goto setup_fail;

		is_ops_inited = true;
	}

	NBL_RES_OPS_TBL_TO_OPS(*res_ops_tbl) = &res_ops;
	NBL_RES_OPS_TBL_TO_PRIV(*res_ops_tbl) = res_mgt_leonis;

	return 0;

setup_fail:
	nbl_res_remove_ops(dev, res_ops_tbl);
	return -EAGAIN;
}

static int nbl_res_dev_setup_eswitch_info(struct nbl_resource_mgt *res_mgt)
{
	struct device *dev;
	struct nbl_eswitch_info *eswitch_info;

	dev = NBL_RES_MGT_TO_DEV(res_mgt);
	eswitch_info = devm_kzalloc(dev, sizeof(struct nbl_eswitch_info), GFP_KERNEL);
	if (!eswitch_info)
		return -ENOMEM;
	NBL_RES_MGT_TO_ESWITCH_INFO(res_mgt) = eswitch_info;

	return 0;
}

static void nbl_res_pf_dev_remove_eswitch_info(struct nbl_resource_mgt *res_mgt)
{
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct nbl_eswitch_info **eswitch_info = &NBL_RES_MGT_TO_ESWITCH_INFO(res_mgt);

	if (!(*eswitch_info))
		return;
	devm_kfree(dev, *eswitch_info);
	*eswitch_info = NULL;
}

static int nbl_res_ctrl_dev_setup_eth_info(struct nbl_resource_mgt *res_mgt)
{
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct nbl_eth_info *eth_info;
	struct nbl_eth_bond_info *eth_bond_info;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	u32 eth_num = 0;
	u32 eth_bitmap, eth_id;
	int i;

	eth_info = devm_kzalloc(dev, sizeof(struct nbl_eth_info), GFP_KERNEL);
	if (!eth_info)
		return -ENOMEM;

	NBL_RES_MGT_TO_ETH_INFO(res_mgt) = eth_info;

	eth_bond_info = devm_kzalloc(dev, sizeof(struct nbl_eth_bond_info), GFP_KERNEL);
	if (!eth_bond_info)
		return -ENOMEM;

	NBL_RES_MGT_TO_ETH_BOND_INFO(res_mgt) = eth_bond_info;

	eth_info->eth_num = (u8)phy_ops->get_fw_eth_num(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
	eth_bitmap = phy_ops->get_fw_eth_map(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
	/* for 2 eth port board, the eth_id is 0, 2 */
	for (i = 0; i < NBL_MAX_ETHERNET; i++) {
		if ((1 << i) & eth_bitmap) {
			set_bit(i, eth_info->eth_bitmap);
			eth_info->eth_id[eth_num] = i;
			eth_info->logic_eth_id[i] = eth_num;
			eth_num++;
		}
	}

	for (i = 0; i < NBL_RES_MGT_TO_PF_NUM(res_mgt); i++) {
		/* if pf_id <= eth_num, the pf relate corresponding eth_id*/
		if (i < eth_num) {
			eth_id = eth_info->eth_id[i];
			eth_info->pf_bitmap[eth_id] |= BIT(i);
		}
		/* if pf_id > eth_num, the pf relate eth 0*/
		else
			eth_info->pf_bitmap[0] |= BIT(i);
	}

	return 0;
}

static void nbl_res_ctrl_dev_remove_eth_info(struct nbl_resource_mgt *res_mgt)
{
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct nbl_eth_info **eth_info = &NBL_RES_MGT_TO_ETH_INFO(res_mgt);
	struct nbl_eth_bond_info **eth_bond_info = &NBL_RES_MGT_TO_ETH_BOND_INFO(res_mgt);

	if (*eth_bond_info) {
		devm_kfree(dev, *eth_bond_info);
		*eth_bond_info = NULL;
	}

	if (*eth_info) {
		devm_kfree(dev, *eth_info);
		*eth_info = NULL;
	}
}

static int nbl_res_ctrl_dev_sriov_info_init(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_resource_info *res_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct device *dev =  NBL_COMMON_TO_DEV(common);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_sriov_info *sriov_info;
	u32 vf_fid, vf_startid, vf_endid = NBL_MAX_VF;
	u16 func_id;
	u16 function;

	sriov_info = devm_kcalloc(dev, NBL_RES_MGT_TO_PF_NUM(res_mgt),
				  sizeof(struct nbl_sriov_info), GFP_KERNEL);
	if (!sriov_info)
		return -ENOMEM;

	NBL_RES_MGT_TO_SRIOV_INFO(res_mgt) = sriov_info;

	for (func_id = 0; func_id < NBL_RES_MGT_TO_PF_NUM(res_mgt); func_id++) {
		sriov_info = &NBL_RES_MGT_TO_SRIOV_INFO(res_mgt)[func_id];
		function = NBL_COMMON_TO_PCI_FUNC_ID(common) + func_id;

		common->hw_bus = (u8)phy_ops->get_real_bus(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
		sriov_info->bdf = PCI_DEVID(common->hw_bus,
					    PCI_DEVFN(common->devid, function));
		vf_fid = phy_ops->get_host_pf_fid(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), func_id);
		vf_startid = vf_fid & 0xFFFF;
		vf_endid = (vf_fid >> 16) & 0xFFFF;
		sriov_info->start_vf_func_id = vf_startid + NBL_MAX_PF_LEONIS;
		sriov_info->num_vfs = vf_endid - vf_startid;
	}

	res_info->max_vf_num = vf_endid;

	return 0;
}

static void nbl_res_ctrl_dev_sriov_info_remove(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_sriov_info **sriov_info = &NBL_RES_MGT_TO_SRIOV_INFO(res_mgt);
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);

	if (!(*sriov_info))
		return;

	devm_kfree(dev, *sriov_info);
	*sriov_info = NULL;
}

static void nbl_res_ctrl_dev_vdpa_vf_stats_remove(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_vdpa_status **vf_status = NBL_RES_MGT_TO_VDPA_VF_STATS(res_mgt);
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	int i = 0;

	for (i = 0; i < NBL_MAX_FUNC; i++) {
		if (vf_status[i]) {
			devm_kfree(dev, vf_status[i]);
			vf_status[i] = NULL;
		}
	}
}

static int nbl_res_ctrl_dev_vsi_info_init(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct device *dev =  NBL_COMMON_TO_DEV(common);
	struct nbl_vsi_info *vsi_info;
	struct nbl_sriov_info *sriov_info;
	struct nbl_eth_info *eth_info = NBL_RES_MGT_TO_ETH_INFO(res_mgt);
	int i;

	vsi_info = devm_kcalloc(dev, NBL_RES_MGT_TO_PF_NUM(res_mgt),
				sizeof(struct nbl_vsi_info), GFP_KERNEL);
	if (!vsi_info)
		return -ENOMEM;

	NBL_RES_MGT_TO_VSI_INFO(res_mgt) = vsi_info;
	/**
	 * case 1 two port(2pf)
	 * pf0,pf1(NBL_VSI_SERV_PF_DATA_TYPE) vsi is 0,512
	 * pf0,pf1(NBL_VSI_SERV_PF_CTLR_TYPE) vsi is 1,513
	 * pf0,pf1(NBL_VSI_SERV_PF_USER_TYPE) vsi is 2,514
	 * pf0,pf1(NBL_VSI_SERV_PF_XDP_TYPE) vsi is 3,515
	 * pf0.vf0-pf0.vf255(NBL_VSI_SERV_VF_DATA_TYPE) vsi is 4-259
	 * pf1.vf0-pf1.vf255(NBL_VSI_SERV_VF_DATA_TYPE) vsi is 516-771
	 * pf2-pf7(NBL_VSI_SERV_PF_EXTRA_TYPE) vsi 260-265(if exist)
	 * case 2 four port(4pf)
	 * pf0,pf1,pf2,pf3(NBL_VSI_SERV_PF_DATA_TYPE) vsi is 0,256,512,768
	 * pf0,pf1,pf2,pf3(NBL_VSI_SERV_PF_CTLR_TYPE) vsi is 1,257,513,769
	 * pf0,pf1,pf2,pf3(NBL_VSI_SERV_PF_USER_TYPE) vsi is 2,258,514,770
	 * pf0,pf1,pf2,pf3(NBL_VSI_SERV_PF_XDP_TYPE) vsi is 3,259,515,771
	 * pf0.vf0-pf0.vf127(NBL_VSI_SERV_VF_DATA_TYPE) vsi is 4-131
	 * pf1.vf0-pf1.vf127(NBL_VSI_SERV_VF_DATA_TYPE) vsi is 260-387
	 * pf2.vf0-pf2.vf127(NBL_VSI_SERV_VF_DATA_TYPE) vsi is 516-643
	 * pf3.vf0-pf3.vf127(NBL_VSI_SERV_VF_DATA_TYPE) vsi is 772-899
	 * pf4-pf7(NBL_VSI_SERV_PF_EXTRA_TYPE) vsi 132-135(if exist)
	 */

	vsi_info->num = eth_info->eth_num;
	for (i = 0; i < vsi_info->num; i++) {
		vsi_info->serv_info[i][NBL_VSI_SERV_PF_DATA_TYPE].base_id = i
			* NBL_VSI_ID_GAP(vsi_info->num);
		vsi_info->serv_info[i][NBL_VSI_SERV_PF_DATA_TYPE].num = 1;
		vsi_info->serv_info[i][NBL_VSI_SERV_PF_CTLR_TYPE].base_id =
		    vsi_info->serv_info[i][NBL_VSI_SERV_PF_DATA_TYPE].base_id
		    + vsi_info->serv_info[i][NBL_VSI_SERV_PF_DATA_TYPE].num;
		vsi_info->serv_info[i][NBL_VSI_SERV_PF_CTLR_TYPE].num = 1;
		vsi_info->serv_info[i][NBL_VSI_SERV_PF_USER_TYPE].base_id =
		    vsi_info->serv_info[i][NBL_VSI_SERV_PF_CTLR_TYPE].base_id
		    + vsi_info->serv_info[i][NBL_VSI_SERV_PF_CTLR_TYPE].num;
		vsi_info->serv_info[i][NBL_VSI_SERV_PF_USER_TYPE].num = 1;
		vsi_info->serv_info[i][NBL_VSI_SERV_PF_XDP_TYPE].base_id =
		    vsi_info->serv_info[i][NBL_VSI_SERV_PF_USER_TYPE].base_id
		    + vsi_info->serv_info[i][NBL_VSI_SERV_PF_USER_TYPE].num;
		vsi_info->serv_info[i][NBL_VSI_SERV_PF_XDP_TYPE].num = 1;
		vsi_info->serv_info[i][NBL_VSI_SERV_VF_DATA_TYPE].base_id =
		    vsi_info->serv_info[i][NBL_VSI_SERV_PF_XDP_TYPE].base_id
		    + vsi_info->serv_info[i][NBL_VSI_SERV_PF_XDP_TYPE].num;
		sriov_info = NBL_RES_MGT_TO_SRIOV_INFO(res_mgt) + i;
		vsi_info->serv_info[i][NBL_VSI_SERV_VF_DATA_TYPE].num = sriov_info->num_vfs;
	}

	/* pf_id >= eth_num, it belong pf0's switch */
	vsi_info->serv_info[0][NBL_VSI_SERV_PF_EXTRA_TYPE].base_id =
	    vsi_info->serv_info[0][NBL_VSI_SERV_VF_DATA_TYPE].base_id
	    + vsi_info->serv_info[0][NBL_VSI_SERV_VF_DATA_TYPE].num;
	vsi_info->serv_info[0][NBL_VSI_SERV_PF_EXTRA_TYPE].num =
		NBL_RES_MGT_TO_PF_NUM(res_mgt) - vsi_info->num;

	return 0;
}

static void nbl_res_ctrl_dev_remove_vsi_info(struct nbl_resource_mgt *res_mgt)
{
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct nbl_vsi_info **vsi_info = &NBL_RES_MGT_TO_VSI_INFO(res_mgt);

	if (!(*vsi_info))
		return;

	devm_kfree(dev, *vsi_info);
	*vsi_info = NULL;
}

static int nbl_res_ring_num_info_init(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_resource_info *resource_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	struct nbl_net_ring_num_info *num_info = &resource_info->net_ring_num_info;

	num_info->pf_def_max_net_qp_num = NBL_DEFAULT_PF_HW_QUEUE_NUM;
	num_info->vf_def_max_net_qp_num = NBL_DEFAULT_VF_HW_QUEUE_NUM;

	return 0;
}

static int nbl_res_ctrl_dev_ustore_stats_init(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct device *dev =  NBL_COMMON_TO_DEV(common);
	struct nbl_ustore_stats *ustore_stats;

	ustore_stats = devm_kcalloc(dev, NBL_MAX_ETHERNET,
				    sizeof(struct nbl_ustore_stats), GFP_KERNEL);
	if (!ustore_stats)
		return -ENOMEM;

	NBL_RES_MGT_TO_USTORE_STATS(res_mgt) = ustore_stats;

	return 0;
}

static void nbl_res_ctrl_dev_ustore_stats_remove(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_ustore_stats **ustore_stats = &NBL_RES_MGT_TO_USTORE_STATS(res_mgt);
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);

	if (!(*ustore_stats))
		return;

	devm_kfree(dev, *ustore_stats);
	*ustore_stats = NULL;
}

static int nbl_res_check_fw_working(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	unsigned long fw_pong_current;
	unsigned long seconds_current = 0;
	unsigned long timeout_us = 500 * USEC_PER_MSEC;
	unsigned long sleep_us = USEC_PER_MSEC;
	ktime_t timeout = ktime_add_us(ktime_get(), timeout_us);
	bool sleep_before_read = false;

	seconds_current = (unsigned long)ktime_get_real_seconds();
	phy_ops->set_fw_pong(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), seconds_current - 1);
	phy_ops->set_fw_ping(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), seconds_current);

	might_sleep_if(sleep_us != 0);
	if (sleep_before_read && sleep_us)
		usleep_range((sleep_us >> 2) + 1, sleep_us);
	for (;;) {
		fw_pong_current = phy_ops->get_fw_pong(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
		if (fw_pong_current == seconds_current)
			break;
		if (timeout_us && ktime_compare(ktime_get(), timeout) > 0) {
			fw_pong_current = phy_ops->get_fw_pong(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
			break;
		}
		if (sleep_us)
			usleep_range((sleep_us >> 2) + 1, sleep_us);
	}
	return (fw_pong_current == seconds_current) ? 0 : -ETIMEDOUT;
}

static int nbl_res_init_pf_num(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	u32 pf_mask;
	u32 pf_num = 0;
	int i;

	pf_mask = phy_ops->get_host_pf_mask(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
	for (i = 0; i < NBL_MAX_PF_LEONIS; i++) {
		if (!(pf_mask & (1 << i)))
			pf_num++;
		else
			break;
	}

	NBL_RES_MGT_TO_PF_NUM(res_mgt) = pf_num;

	if (!pf_num)
		return -1;

	return 0;
}

static void nbl_res_init_board_info(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->get_board_info(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				&res_mgt->resource_info->board_info);
}

static void nbl_res_stop(struct nbl_resource_mgt_leonis *res_mgt_leonis)
{
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);

	nbl_fd_mgt_stop(res_mgt);
	nbl_queue_mgt_stop(res_mgt);
	nbl_txrx_mgt_stop(res_mgt);
	nbl_intr_mgt_stop(res_mgt);
	nbl_adminq_mgt_stop(res_mgt);
	nbl_vsi_mgt_stop(res_mgt);
	nbl_accel_mgt_stop(res_mgt);
	nbl_flow_mgt_stop_leonis(res_mgt);
	nbl_res_ctrl_dev_ustore_stats_remove(res_mgt);
	nbl_res_ctrl_dev_vdpa_vf_stats_remove(res_mgt);
	nbl_res_ctrl_dev_remove_vsi_info(res_mgt);
	nbl_res_ctrl_dev_remove_eth_info(res_mgt);
	nbl_res_ctrl_dev_sriov_info_remove(res_mgt);
	nbl_res_pf_dev_remove_eswitch_info(res_mgt);

	/*only pf0 need tc_flow_mgt_stop*/
	if (!common->is_vf && !NBL_COMMON_TO_PCI_FUNC_ID(common)) {
		nbl_info(common, NBL_DEBUG_FLOW, "tc flow stop tc flow mgt");
		nbl_tc_flow_mgt_stop_leonis(res_mgt);
	}
}

static int nbl_res_start(struct nbl_resource_mgt_leonis *res_mgt_leonis,
			 struct nbl_func_caps caps)
{
	struct nbl_resource_mgt *res_mgt = &res_mgt_leonis->res_mgt;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_upcall_port_info *upcall_port_info =
				&res_mgt_leonis->pmd_status.upcall_port_info;
	u32 quirks;
	int ret = 0;

	if (caps.has_ctrl) {
		ret = nbl_res_check_fw_working(res_mgt);
		if (ret) {
			nbl_err(common, NBL_DEBUG_RESOURCE, "fw is not working");
			return ret;
		}

		nbl_res_init_board_info(res_mgt);

		ret = nbl_res_init_pf_num(res_mgt);
		if (ret) {
			nbl_err(common, NBL_DEBUG_RESOURCE, "pf number is illegal");
			return ret;
		}

		ret = nbl_res_ctrl_dev_sriov_info_init(res_mgt);
		if (ret) {
			nbl_err(common, NBL_DEBUG_RESOURCE, "Failed to init sr_iov info");
			return ret;
		}

		ret = nbl_res_ctrl_dev_setup_eth_info(res_mgt);
		if (ret)
			goto start_fail;

		ret = nbl_res_ctrl_dev_vsi_info_init(res_mgt);
		if (ret)
			goto start_fail;

		ret = nbl_res_ring_num_info_init(res_mgt);
		if (ret)
			goto start_fail;

		ret = nbl_res_ctrl_dev_ustore_stats_init(res_mgt);
		if (ret)
			goto start_fail;

		ret = nbl_flow_mgt_start_leonis(res_mgt);
		if (ret)
			goto start_fail;

		ret = nbl_tc_flow_mgt_start_leonis(res_mgt);
		if (ret)
			goto start_fail;

		ret = nbl_queue_mgt_start(res_mgt);
		if (ret)
			goto start_fail;

		ret = nbl_vsi_mgt_start(res_mgt);
		if (ret)
			goto start_fail;

		ret = nbl_adminq_mgt_start(res_mgt);
		if (ret)
			goto start_fail;

		ret = nbl_intr_mgt_start(res_mgt);
		if (ret)
			goto start_fail;

		ret = nbl_accel_mgt_start(res_mgt);
		if (ret)
			goto start_fail;

		ret = nbl_fd_mgt_start(res_mgt);
		if (ret)
			goto start_fail;

		nbl_res_set_flex_capability(res_mgt, NBL_DUMP_FLOW_CAP);
		nbl_res_set_flex_capability(res_mgt, NBL_DUMP_FD_CAP);
		nbl_res_set_fix_capability(res_mgt, NBL_TASK_OFFLOAD_NETWORK_CAP);
		nbl_res_set_fix_capability(res_mgt, NBL_TASK_FW_HB_CAP);
		nbl_res_set_fix_capability(res_mgt, NBL_TASK_FW_RESET_CAP);
		nbl_res_set_fix_capability(res_mgt, NBL_TASK_CLEAN_ADMINDQ_CAP);
		nbl_res_set_fix_capability(res_mgt, NBL_RESTOOL_CAP);
		nbl_res_set_fix_capability(res_mgt, NBL_TASK_ADAPT_DESC_GOTHER);
		nbl_res_set_fix_capability(res_mgt, NBL_PROCESS_FLR_CAP);
		nbl_res_set_fix_capability(res_mgt, NBL_TASK_RESET_CTRL_CAP);
		/* leonis af need a pmd_debug for dpdk gdb debug */
		nbl_res_set_fix_capability(res_mgt, NBL_PMD_DEBUG);
		nbl_res_set_fix_capability(res_mgt, NBL_HIGH_THROUGHPUT_CAP);
		nbl_res_set_fix_capability(res_mgt, NBL_TASK_HEALTH_REPORT_TEMP_CAP);
		nbl_res_set_fix_capability(res_mgt, NBL_TASK_HEALTH_REPORT_REBOOT_CAP);
		nbl_res_set_fix_capability(res_mgt, NBL_DVN_DESC_REQ_SYSFS_CAP);
		nbl_res_set_flex_capability(res_mgt, NBL_SECURITY_ACCEL_CAP);
		nbl_res_set_fix_capability(res_mgt, NBL_TASK_IPSEC_AGE_CAP);
		upcall_port_info->upcall_port_active = false;
	}

	if (caps.has_net) {
		ret = nbl_txrx_mgt_start(res_mgt);
		if (ret)
			goto start_fail;

		if (!caps.is_vf) {
			ret = nbl_res_dev_setup_eswitch_info(res_mgt);
			if (ret)
				goto start_fail;
		}
	}

	nbl_res_set_fix_capability(res_mgt, NBL_HWMON_TEMP_CAP);
	nbl_res_set_fix_capability(res_mgt, NBL_TASK_CLEAN_MAILBOX_CAP);
	nbl_res_set_fix_capability(res_mgt, NBL_ITR_DYNAMIC);
	nbl_res_set_fix_capability(res_mgt, NBL_P4_CAP);
	nbl_res_set_fix_capability(res_mgt, NBL_TASK_RESET_CAP);
	nbl_res_set_fix_capability(res_mgt, NBL_QOS_SYSFS_CAP);
	nbl_res_set_fix_capability(res_mgt, NBL_MIRROR_SYSFS_CAP);

	nbl_res_set_fix_capability(res_mgt, NBL_XDP_CAP);

	quirks = nbl_res_get_quirks(res_mgt);
	if (quirks & BIT(NBL_QUIRKS_NO_TOE)) {
		nbl_res_set_fix_capability(res_mgt, NBL_TASK_KEEP_ALIVE);
		if (caps.has_ctrl)
			nbl_res_set_fix_capability(res_mgt, NBL_RECOVERY_ABNORMAL_STATUS);
	}

	return 0;

start_fail:
	nbl_res_stop(res_mgt_leonis);
	return ret;
}

int nbl_res_init_leonis(void *p, struct nbl_init_param *param)
{
	struct nbl_adapter *adapter = (struct nbl_adapter *)p;
	struct device *dev;
	struct nbl_common_info *common;
	struct nbl_resource_mgt_leonis **res_mgt_leonis;
	struct nbl_resource_ops_tbl **res_ops_tbl;
	struct nbl_phy_ops_tbl *phy_ops_tbl;
	struct nbl_channel_ops_tbl *chan_ops_tbl;
	int ret = 0;

	dev = NBL_ADAPTER_TO_DEV(adapter);
	common = NBL_ADAPTER_TO_COMMON(adapter);
	res_mgt_leonis = (struct nbl_resource_mgt_leonis **)&NBL_ADAPTER_TO_RES_MGT(adapter);
	res_ops_tbl = &NBL_ADAPTER_TO_RES_OPS_TBL(adapter);
	phy_ops_tbl = NBL_ADAPTER_TO_PHY_OPS_TBL(adapter);
	chan_ops_tbl = NBL_ADAPTER_TO_CHAN_OPS_TBL(adapter);

	ret = nbl_res_setup_res_mgt(common, res_mgt_leonis);
	if (ret)
		goto setup_mgt_fail;

	nbl_res_setup_common_ops(&(*res_mgt_leonis)->res_mgt);
	NBL_RES_MGT_TO_CHAN_OPS_TBL(&(*res_mgt_leonis)->res_mgt) = chan_ops_tbl;
	NBL_RES_MGT_TO_PHY_OPS_TBL(&(*res_mgt_leonis)->res_mgt) = phy_ops_tbl;

	NBL_RES_MGT_TO_PROD_OPS(&(*res_mgt_leonis)->res_mgt) = &product_ops;

	ret = nbl_res_start(*res_mgt_leonis, param->caps);
	if (ret)
		goto start_fail;

	ret = nbl_res_setup_ops(dev, res_ops_tbl, *res_mgt_leonis);
	if (ret)
		goto setup_ops_fail;

	return 0;

setup_ops_fail:
	nbl_res_stop(*res_mgt_leonis);
start_fail:
	nbl_res_remove_res_mgt(common, res_mgt_leonis);
setup_mgt_fail:
	return ret;
}

void nbl_res_remove_leonis(void *p)
{
	struct nbl_adapter *adapter = (struct nbl_adapter *)p;
	struct device *dev;
	struct nbl_common_info *common;
	struct nbl_resource_mgt_leonis **res_mgt;
	struct nbl_resource_ops_tbl **res_ops_tbl;

	dev = NBL_ADAPTER_TO_DEV(adapter);
	common = NBL_ADAPTER_TO_COMMON(adapter);
	res_mgt = (struct nbl_resource_mgt_leonis **)&NBL_ADAPTER_TO_RES_MGT(adapter);
	res_ops_tbl = &NBL_ADAPTER_TO_RES_OPS_TBL(adapter);

	nbl_res_remove_ops(dev, res_ops_tbl);
	nbl_res_stop(*res_mgt);
	nbl_res_remove_res_mgt(common, res_mgt);
}
