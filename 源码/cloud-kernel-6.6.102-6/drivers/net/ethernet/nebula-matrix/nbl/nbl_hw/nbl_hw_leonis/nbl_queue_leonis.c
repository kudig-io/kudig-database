// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#include "nbl_queue_leonis.h"
#include "nbl_resource_leonis.h"

static int nbl_res_queue_reset_uvn_pkt_drop_stats(void *priv, u16 func_id, u16 global_queue_id);

static struct nbl_queue_vsi_info *
nbl_res_queue_get_vsi_info(struct nbl_resource_mgt *res_mgt, u16 vsi_id)
{
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info;
	u16 func_id;
	int i;

	func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	queue_info = &queue_mgt->queue_info[func_id];

	for (i = 0; i < NBL_VSI_MAX; i++)
		if (queue_info->vsi_info[i].vsi_id == vsi_id)
			return &queue_info->vsi_info[i];

	return NULL;
}

static int nbl_res_queue_get_net_id(u16 func_id, u16 vsi_type)
{
	int net_id;

	switch (vsi_type) {
	case NBL_VSI_DATA:
	case NBL_VSI_XDP:
		net_id = func_id;
		break;
	case NBL_VSI_USER:
	case NBL_VSI_CTRL:
		net_id = func_id + NBL_SPECIFIC_VSI_NET_ID_OFFSET;
		break;
	default:
		net_id = func_id;
		break;
	}

	return net_id;
}

static int nbl_res_queue_setup_queue_info(struct nbl_resource_mgt *res_mgt, u16 func_id,
					  u16 num_queues)
{
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];
	struct nbl_event_queue_update_data event_data;
	u16 *txrx_queues, *queues_context;
	u32 *uvn_stat_pkt_drop;
	u16 queue_index;
	int i, ret = 0;

	nbl_info(common, NBL_DEBUG_QUEUE,
		 "Setup qid map, func_id:%d, num_queues:%d", func_id, num_queues);

	txrx_queues = kcalloc(num_queues, sizeof(txrx_queues[0]), GFP_ATOMIC);
	if (!txrx_queues) {
		ret = -ENOMEM;
		goto alloc_txrx_queues_fail;
	}

	queues_context = kcalloc(num_queues * 2, sizeof(txrx_queues[0]), GFP_ATOMIC);
	if (!queues_context) {
		ret = -ENOMEM;
		goto alloc_queue_contex_fail;
	}

	uvn_stat_pkt_drop = kcalloc(num_queues, sizeof(*uvn_stat_pkt_drop), GFP_ATOMIC);
	if (!uvn_stat_pkt_drop) {
		ret = -ENOMEM;
		goto alloc_uvn_stat_pkt_drop_fail;
	}

	queue_info->num_txrx_queues = num_queues;
	queue_info->txrx_queues = txrx_queues;
	queue_info->queues_context = queues_context;
	queue_info->uvn_stat_pkt_drop = uvn_stat_pkt_drop;

	for (i = 0; i < num_queues; i++) {
		queue_index = find_first_zero_bit(queue_mgt->txrx_queue_bitmap, NBL_MAX_TXRX_QUEUE);
		if (queue_index == NBL_MAX_TXRX_QUEUE) {
			ret = -ENOSPC;
			goto get_txrx_queue_fail;
		}
		txrx_queues[i] = queue_index;
		set_bit(queue_index, queue_mgt->txrx_queue_bitmap);
	}

	event_data.func_id = func_id;
	event_data.ring_num = num_queues;
	event_data.map = txrx_queues;
	nbl_event_notify(NBL_EVENT_QUEUE_ALLOC, &event_data, NBL_COMMON_TO_VSI_ID(common),
			 NBL_COMMON_TO_BOARD_ID(common));

	return 0;

get_txrx_queue_fail:
	kfree(uvn_stat_pkt_drop);
	while (--i + 1) {
		queue_index = txrx_queues[i];
		clear_bit(queue_index, queue_mgt->txrx_queue_bitmap);
	}
	queue_info->num_txrx_queues = 0;
	queue_info->txrx_queues = NULL;
alloc_uvn_stat_pkt_drop_fail:
	kfree(queues_context);
alloc_queue_contex_fail:
	kfree(txrx_queues);
alloc_txrx_queues_fail:
	return ret;
}

static void nbl_res_queue_remove_queue_info(struct nbl_resource_mgt *res_mgt, u16 func_id)
{
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];
	u16 i;

	for (i = 0; i < queue_info->num_txrx_queues; i++)
		clear_bit(queue_info->txrx_queues[i], queue_mgt->txrx_queue_bitmap);

	kfree(queue_info->txrx_queues);
	kfree(queue_info->queues_context);
	kfree(queue_info->uvn_stat_pkt_drop);
	queue_info->txrx_queues = NULL;
	queue_info->queues_context = NULL;
	queue_info->uvn_stat_pkt_drop = NULL;

	queue_info->num_txrx_queues = 0;
}

static inline u64 nbl_res_queue_qid_map_key(struct nbl_qid_map_table qid_map)
{
	u64 notify_addr_l = qid_map.notify_addr_l;
	u64 notify_addr_h = qid_map.notify_addr_h;

	return (notify_addr_h << NBL_QID_MAP_NOTIFY_ADDR_LOW_PART_LEN) | notify_addr_l;
}

static void nbl_res_queue_set_qid_map_table(struct nbl_resource_mgt *res_mgt, u16 tail)
{
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_qid_map_param param;
	int i;

	param.qid_map = kcalloc(tail, sizeof(param.qid_map[0]), GFP_ATOMIC);
	if (!param.qid_map)
		return;

	for (i = 0; i < tail; i++)
		param.qid_map[i] = queue_mgt->qid_map_table[i];

	param.start = 0;
	param.len = tail;

	phy_ops->set_qid_map_table(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), &param,
				   queue_mgt->qid_map_select);
	queue_mgt->qid_map_select = !queue_mgt->qid_map_select;

	if (!queue_mgt->qid_map_ready) {
		phy_ops->set_qid_map_ready(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), true);
		queue_mgt->qid_map_ready = true;
	}

	kfree(param.qid_map);
}

int nbl_res_queue_setup_qid_map_table_leonis(struct nbl_resource_mgt *res_mgt, u16 func_id,
					     u64 notify_addr)
{
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];
	struct nbl_qid_map_table qid_map;
	u64 key;
	u16 *txrx_queues = queue_info->txrx_queues;
	u16 qid_map_entries = queue_info->num_txrx_queues, qid_map_base, tail;
	int i;

	/* Get base location */
	queue_info->notify_addr = notify_addr;
	key = notify_addr >> NBL_QID_MAP_NOTIFY_ADDR_SHIFT;

	for (i = 0; i < NBL_QID_MAP_TABLE_ENTRIES; i++) {
		WARN_ON(key == nbl_res_queue_qid_map_key(queue_mgt->qid_map_table[i]));
		if (key < nbl_res_queue_qid_map_key(queue_mgt->qid_map_table[i])) {
			qid_map_base = i;
			break;
		}
	}
	if (i == NBL_QID_MAP_TABLE_ENTRIES) {
		nbl_err(common, NBL_DEBUG_QUEUE, "No valid qid map key for func %d", func_id);
		return -ENOSPC;
	}

	/* Calc tail, we will set the qid_map from 0 to tail.
	 * We have to make sure that this range (0, tail) can cover all the changes, which need to
	 * consider all the two tables. Therefore, it is necessary to store each table's tail, and
	 * always use the larger one between this table's tail and the added tail.
	 *
	 * The reason can be illustrated in the following example:
	 * Step 1: del some entries, which happens on table 1, and each table could be
	 *      Table 0: 0 - 31 used
	 *      Table 1: 0 - 15 used
	 *      SW     : queue_mgt->total_qid_map_entries = 16
	 * Step 2: add 2 entries, which happens on table 0, if we use 16 + 2 as the tail, then
	 *      Table 0: 0 - 17 correctly added, 18 - 31 garbage data
	 *      Table 1: 0 - 15 used
	 *      SW     : queue_mgt->total_qid_map_entries = 18
	 * And this is definitely wrong, it should use 32, table 0's original tail
	 */
	queue_mgt->total_qid_map_entries += qid_map_entries;
	tail = max(queue_mgt->total_qid_map_entries,
		   queue_mgt->qid_map_tail[queue_mgt->qid_map_select]);
	queue_mgt->qid_map_tail[queue_mgt->qid_map_select] = queue_mgt->total_qid_map_entries;

	/* Update qid map */
	for (i = NBL_QID_MAP_TABLE_ENTRIES - qid_map_entries; i > qid_map_base; i--)
		queue_mgt->qid_map_table[i - 1 + qid_map_entries] = queue_mgt->qid_map_table[i - 1];

	for (i = 0; i < queue_info->num_txrx_queues; i++) {
		qid_map.local_qid = 2 * i + 1;
		qid_map.notify_addr_l = key;
		qid_map.notify_addr_h = key >> NBL_QID_MAP_NOTIFY_ADDR_LOW_PART_LEN;
		qid_map.global_qid = txrx_queues[i];
		qid_map.ctrlq_flag = 0;
		queue_mgt->qid_map_table[qid_map_base + i] = qid_map;
	}

	nbl_res_queue_set_qid_map_table(res_mgt, tail);

	return 0;
}

void nbl_res_queue_remove_qid_map_table_leonis(struct nbl_resource_mgt *res_mgt, u16 func_id)
{
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];
	struct nbl_qid_map_table qid_map;
	u64 key;
	u16 qid_map_entries = queue_info->num_txrx_queues, qid_map_base, tail;
	int i;

	/* Get base location */
	key = queue_info->notify_addr >> NBL_QID_MAP_NOTIFY_ADDR_SHIFT;

	for (i = 0; i < NBL_QID_MAP_TABLE_ENTRIES; i++) {
		if (key == nbl_res_queue_qid_map_key(queue_mgt->qid_map_table[i])) {
			qid_map_base = i;
			break;
		}
	}
	if (i == NBL_QID_MAP_TABLE_ENTRIES) {
		nbl_err(common, NBL_DEBUG_QUEUE, "No valid qid map key for func %d", func_id);
		return;
	}

	/* Calc tail, we will set the qid_map from 0 to tail.
	 * We have to make sure that this range (0, tail) can cover all the changes, which need to
	 * consider all the two tables. Therefore, it is necessary to store each table's tail, and
	 * always use the larger one between this table's tail and the driver-stored tail.
	 *
	 * The reason can be illustrated in the following example:
	 * Step 1: del some entries, which happens on table 1, and each table could be
	 *      Table 0: 0 - 31 used
	 *      Table 1: 0 - 15 used
	 *      SW     : queue_mgt->total_qid_map_entries = 16
	 * Step 2: del 2 entries, which happens on table 0, if we use 16 as the tail, then
	 *      Table 0: 0 - 13 correct, 14 - 31 garbage data
	 *      Table 1: 0 - 15 used
	 *      SW     : queue_mgt->total_qid_map_entries = 14
	 * And this is definitely wrong, it should use 32, table 0's original tail
	 */
	tail = max(queue_mgt->total_qid_map_entries,
		   queue_mgt->qid_map_tail[queue_mgt->qid_map_select]);
	queue_mgt->total_qid_map_entries -= qid_map_entries;
	queue_mgt->qid_map_tail[queue_mgt->qid_map_select] = queue_mgt->total_qid_map_entries;

	/* Update qid map */
	memset(&qid_map, U8_MAX, sizeof(qid_map));

	for (i = qid_map_base; i < NBL_QID_MAP_TABLE_ENTRIES - qid_map_entries; i++)
		queue_mgt->qid_map_table[i] = queue_mgt->qid_map_table[i + qid_map_entries];
	for (; i < NBL_QID_MAP_TABLE_ENTRIES; i++)
		queue_mgt->qid_map_table[i] = qid_map;

	nbl_res_queue_set_qid_map_table(res_mgt, tail);
}

static int
nbl_res_queue_get_rss_ret_base(struct nbl_resource_mgt *res_mgt, u16 count, u16 rss_entry_size,
			       struct nbl_queue_vsi_info *vsi_info)
{
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	u32 rss_ret_base_start;
	u32 rss_ret_base_end;
	u16 func_id;
	u16 rss_entry_count;
	u16 index, i, j, k;
	int success = 1;
	int ret = -EFAULT;

	func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_info->vsi_id);
	if (func_id < NBL_MAX_ETHERNET &&
	    (vsi_info->vsi_index == NBL_VSI_DATA || vsi_info->vsi_index == NBL_VSI_USER)) {
		rss_ret_base_start = 0;
		rss_ret_base_end = NBL_EPRO_PF_RSS_RET_TBL_DEPTH;
		vsi_info->rss_entry_size = NBL_EPRO_PF_RSS_ENTRY_SIZE;
		rss_entry_count = NBL_EPRO_PF_RSS_RET_TBL_COUNT;
	} else {
		rss_ret_base_start = NBL_EPRO_PF_RSS_RET_TBL_DEPTH;
		rss_ret_base_end = NBL_EPRO_RSS_RET_TBL_DEPTH;
		vsi_info->rss_entry_size = rss_entry_size;
		rss_entry_count = count;
	}

	for (i = rss_ret_base_start; i < rss_ret_base_end;) {
		index = find_next_zero_bit(queue_mgt->rss_ret_bitmap,
					   rss_ret_base_end, i);
		if (index == rss_ret_base_end) {
			nbl_err(common, NBL_DEBUG_QUEUE, "There is no available rss ret left");
			break;
		}

		success = 1;
		for (j = index + 1; j < (index + rss_entry_count); j++) {
			if (j >= rss_ret_base_end) {
				success = 0;
				break;
			}

			if (test_bit(j, queue_mgt->rss_ret_bitmap)) {
				success = 0;
				break;
			}
		}
		if (success) {
			for (k = index; k < (index + rss_entry_count); k++)
				set_bit(k, queue_mgt->rss_ret_bitmap);
			vsi_info->rss_ret_base = index;
			ret = 0;
			break;
		}
		i = j;
	}

	return ret;
}

static int nbl_res_queue_setup_q2vsi(void *priv, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_info *queue_info = NULL;
	struct nbl_queue_vsi_info *vsi_info = NULL;
	u16 func_id;
	int ret = 0, i;

	func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	queue_info = &queue_mgt->queue_info[func_id];
	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return -ENOENT;

	/* config ipro queue tbl */
	for (i = vsi_info->queue_offset;
	     i < vsi_info->queue_offset + vsi_info->queue_num &&
	     i < queue_info->num_txrx_queues; i++) {
		ret = phy_ops->cfg_ipro_queue_tbl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						  queue_info->txrx_queues[i], vsi_id, 1);
		if (ret) {
			while (--i + 1)
				phy_ops->cfg_ipro_queue_tbl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
							    queue_info->txrx_queues[i], 0, 0);
			return ret;
		}
	}

	return 0;
}

static void nbl_res_queue_remove_q2vsi(void *priv, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_info *queue_info = NULL;
	struct nbl_queue_vsi_info *vsi_info = NULL;
	u16 func_id;
	int i;

	func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	queue_info = &queue_mgt->queue_info[func_id];
	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return;

	/*config ipro queue tbl*/
	for (i = vsi_info->queue_offset;
	     i < vsi_info->queue_offset + vsi_info->queue_num && i < queue_info->num_txrx_queues;
	     i++)
		phy_ops->cfg_ipro_queue_tbl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					    queue_info->txrx_queues[i], 0, 0);
}

static int nbl_res_queue_setup_rss(void *priv, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_vsi_info *vsi_info = NULL;
	u16 rss_entry_size, count;
	int ret = 0;

	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return -ENOENT;

	rss_entry_size = (vsi_info->queue_num + NBL_EPRO_RSS_ENTRY_SIZE_UNIT - 1)
			  / NBL_EPRO_RSS_ENTRY_SIZE_UNIT;

	rss_entry_size = ilog2(roundup_pow_of_two(rss_entry_size));
	count = NBL_EPRO_RSS_ENTRY_SIZE_UNIT << rss_entry_size;

	ret = nbl_res_queue_get_rss_ret_base(res_mgt, count, rss_entry_size, vsi_info);
	if (ret)
		return -ENOSPC;

	vsi_info->rss_vld = true;

	return 0;
}

static void nbl_res_queue_remove_rss(void *priv, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_vsi_info *vsi_info = NULL;
	u16 rss_ret_base, rss_entry_size, count;
	int i;

	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return;

	if (!vsi_info->rss_vld)
		return;

	rss_ret_base = vsi_info->rss_ret_base;
	rss_entry_size = vsi_info->rss_entry_size;
	count = NBL_EPRO_RSS_ENTRY_SIZE_UNIT << rss_entry_size;

	for (i = rss_ret_base; i < (rss_ret_base + count); i++)
		clear_bit(i, queue_mgt->rss_ret_bitmap);

	vsi_info->rss_vld = false;
}

static void nbl_res_queue_setup_queue_cfg(struct nbl_queue_mgt *queue_mgt,
					  struct nbl_queue_cfg_param *cfg_param,
					  struct nbl_txrx_queue_param *queue_param,
					  bool is_tx, u16 func_id)
{
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];

	cfg_param->desc = queue_param->dma;
	cfg_param->size = queue_param->desc_num;
	cfg_param->global_vector = queue_param->global_vector_id;
	cfg_param->global_queue_id = queue_info->txrx_queues[queue_param->local_queue_id];

	cfg_param->avail = queue_param->avail;
	cfg_param->used = queue_param->used;
	cfg_param->extend_header = queue_param->extend_header;
	cfg_param->split = queue_param->split;
	cfg_param->last_avail_idx = queue_param->cxt;

	cfg_param->intr_en = queue_param->intr_en;
	cfg_param->intr_mask = queue_param->intr_mask;

	cfg_param->tx = is_tx;
	cfg_param->rxcsum = queue_param->rxcsum;
	cfg_param->half_offload_en = queue_param->half_offload_en;
}

static void nbl_res_queue_update_netid_refnum(struct nbl_queue_mgt *queue_mgt, u16 net_id, bool add)
{
	if (net_id >= NBL_MAX_NET_ID)
		return;

	if (add) {
		queue_mgt->net_id_ref_vsinum[net_id]++;
	} else {
		/* probe call clear_queue first, so judge nor zero to support disable dsch more than
		 * once
		 */
		if (queue_mgt->net_id_ref_vsinum[net_id])
			queue_mgt->net_id_ref_vsinum[net_id]--;
	}
}

static u16 nbl_res_queue_get_netid_refnum(struct nbl_queue_mgt *queue_mgt, u16 net_id)
{
	if (net_id >= NBL_MAX_NET_ID)
		return 0;

	return queue_mgt->net_id_ref_vsinum[net_id];
}

static void nbl_res_queue_setup_hw_dq(struct nbl_resource_mgt *res_mgt,
				      struct nbl_queue_cfg_param *queue_cfg,
				      u16 func_id, u16 vsi_id)
{
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];
	struct nbl_queue_vsi_info *vsi_info;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_vnet_queue_info_param param = {0};
	u16 global_queue_id = queue_cfg->global_queue_id;
	u8 bus, dev, func;

	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return;

	nbl_res_func_id_to_bdf(res_mgt, func_id, &bus, &dev, &func);
	queue_info->split = queue_cfg->split;
	queue_info->queue_size = queue_cfg->size;

	param.function_id = func;
	param.device_id = dev;
	param.bus_id = bus;
	param.valid = 1;

	if (queue_cfg->intr_en) {
		param.msix_idx = queue_cfg->global_vector;
		param.msix_idx_valid = 1;
	}

	if (queue_cfg->tx) {
		phy_ops->set_vnet_queue_info(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), &param,
					     NBL_PAIR_ID_GET_TX(global_queue_id));
		phy_ops->reset_dvn_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue_id);
		if (!queue_cfg->extend_header)
			phy_ops->restore_dvn_context(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						     global_queue_id, queue_cfg->split,
						     queue_cfg->last_avail_idx);
		phy_ops->cfg_tx_queue(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				      queue_cfg, global_queue_id);
		if (nbl_res_queue_get_netid_refnum(queue_mgt, vsi_info->net_id))
			phy_ops->cfg_q2tc_netid(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						global_queue_id, vsi_info->net_id, 1);

	} else {
		phy_ops->set_vnet_queue_info(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), &param,
					     NBL_PAIR_ID_GET_RX(global_queue_id));
		phy_ops->reset_uvn_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue_id);
		nbl_res_queue_reset_uvn_pkt_drop_stats(res_mgt, func_id, global_queue_id);
		if (!queue_cfg->extend_header)
			phy_ops->restore_uvn_context(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						     global_queue_id, queue_cfg->split,
						     queue_cfg->last_avail_idx);
		phy_ops->cfg_rx_queue(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), queue_cfg,
				      global_queue_id);
	}
}

static void nbl_res_queue_remove_hw_dq(struct nbl_resource_mgt *res_mgt,
				       struct nbl_queue_cfg_param *queue_cfg, u16 func_id)
{
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	u16 global_queue_id = queue_cfg->global_queue_id;
	int ret = 0;

	if (queue_cfg->tx) {
		ret = phy_ops->lso_dsch_drain(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue_id);
		if (ret) {
			pr_err("lso_dsch_drain failed\n");
			return;
		}
		phy_ops->clear_vnet_queue_info(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					       NBL_PAIR_ID_GET_TX(global_queue_id));

		phy_ops->disable_dvn(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue_id);

		queue_info->queues_context[NBL_PAIR_ID_GET_TX(global_queue_id)] =
			phy_ops->save_dvn_ctx(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					      global_queue_id, queue_info->split);
		phy_ops->reset_dvn_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue_id);
	} else {
		phy_ops->clear_vnet_queue_info(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					       NBL_PAIR_ID_GET_RX(global_queue_id));
		phy_ops->disable_uvn(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue_id);
		phy_ops->rsc_cache_drain(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue_id);

		queue_info->queues_context[NBL_PAIR_ID_GET_RX(global_queue_id)] =
			phy_ops->save_uvn_ctx(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					      global_queue_id, queue_info->split,
					      queue_info->queue_size);

		phy_ops->reset_uvn_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue_id);
		nbl_res_queue_reset_uvn_pkt_drop_stats(res_mgt, func_id, global_queue_id);
	}
}

static void nbl_res_queue_remove_all_hw_dq(struct nbl_resource_mgt *res_mgt, u16 func_id,
					   struct nbl_queue_vsi_info *vsi_info)
{
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	u16 start = vsi_info->queue_offset, end = vsi_info->queue_offset + vsi_info->queue_num;
	u16 global_queue;
	int i;

	for (i = start; i < end; i++) {
		global_queue = queue_info->txrx_queues[i];

		phy_ops->lso_dsch_drain(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);
		phy_ops->disable_dvn(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);
	}

	for (i = start; i < end; i++) {
		global_queue = queue_info->txrx_queues[i];

		phy_ops->disable_uvn(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);
		phy_ops->rsc_cache_drain(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);
	}

	for (i = start; i < end; i++) {
		global_queue = queue_info->txrx_queues[i];
		queue_info->queues_context[NBL_PAIR_ID_GET_RX(i)] =
			phy_ops->save_uvn_ctx(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					      global_queue, queue_info->split,
					      queue_info->queue_size);
		queue_info->queues_context[NBL_PAIR_ID_GET_TX(i)] =
			phy_ops->save_dvn_ctx(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					      global_queue, queue_info->split);
	}

	for (i = start; i < end; i++) {
		global_queue = queue_info->txrx_queues[i];
		phy_ops->reset_uvn_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);
		nbl_res_queue_reset_uvn_pkt_drop_stats(res_mgt, func_id, global_queue);
		phy_ops->reset_dvn_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);
	}

	for (i = start; i < end; i++) {
		global_queue = queue_info->txrx_queues[i];
		phy_ops->clear_vnet_queue_info(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					       NBL_PAIR_ID_GET_RX(global_queue));
		phy_ops->clear_vnet_queue_info(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					       NBL_PAIR_ID_GET_TX(global_queue));
	}
}

int nbl_res_queue_init_qid_map_table(struct nbl_resource_mgt *res_mgt,
				     struct nbl_queue_mgt *queue_mgt,
				     struct nbl_phy_ops *phy_ops)
{
	struct nbl_qid_map_table invalid_qid_map;
	u16 i;

	queue_mgt->qid_map_ready = 0;
	queue_mgt->qid_map_select = NBL_MASTER_QID_MAP_TABLE;

	memset(&invalid_qid_map, 0, sizeof(invalid_qid_map));
	invalid_qid_map.local_qid = 0x1FF;
	invalid_qid_map.notify_addr_l = 0x7FFFFF;
	invalid_qid_map.notify_addr_h = 0xFFFFFFFF;
	invalid_qid_map.global_qid = 0xFFF;
	invalid_qid_map.ctrlq_flag = 0X1;

	for (i = 0; i < NBL_QID_MAP_TABLE_ENTRIES; i++)
		queue_mgt->qid_map_table[i] = invalid_qid_map;

	phy_ops->init_qid_map_table(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));

	return 0;
}

static int nbl_res_queue_init_epro_rss_key(struct nbl_resource_mgt *res_mgt,
					   struct nbl_phy_ops *phy_ops)
{
	int ret = 0;

	ret = phy_ops->init_epro_rss_key(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
	return ret;
}

static int nbl_res_queue_init_epro_vpt_table(struct nbl_resource_mgt *res_mgt, u16 func_id)
{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_sriov_info *sriov_info = &NBL_RES_MGT_TO_SRIOV_INFO(res_mgt)[func_id];
	int pfid, vfid;
	u16 vsi_id, vf_vsi_id;
	u16 i;

	vsi_id = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_SERV_PF_DATA_TYPE);
	nbl_res_func_id_to_pfvfid(res_mgt, func_id, &pfid, &vfid);

	if (sriov_info->bdf != 0) {
		/* init pf vsi */
		for (i = NBL_VSI_SERV_PF_DATA_TYPE; i <= NBL_VSI_SERV_PF_XDP_TYPE; i++) {
			vsi_id = nbl_res_func_id_to_vsi_id(res_mgt, func_id, i);
			phy_ops->init_epro_vpt_tbl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id);
		}

		for (vfid = 0; vfid < sriov_info->num_vfs; vfid++) {
			vf_vsi_id = nbl_res_pfvfid_to_vsi_id(res_mgt, pfid, vfid, NBL_VSI_DATA);
			if (vf_vsi_id == 0xFFFF)
				continue;

			phy_ops->init_epro_vpt_tbl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vf_vsi_id);
		}
	}

	return 0;
}

static int nbl_res_queue_init_ipro_dn_sport_tbl(struct nbl_resource_mgt *res_mgt,
						u16 func_id, u16 bmode, bool binit)

{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_sriov_info *sriov_info = &NBL_RES_MGT_TO_SRIOV_INFO(res_mgt)[func_id];
	int pfid, vfid;
	u16 eth_id, vsi_id, vf_vsi_id;
	int i;

	vsi_id = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_SERV_PF_DATA_TYPE);
	nbl_res_func_id_to_pfvfid(res_mgt, func_id, &pfid, &vfid);

	if (sriov_info->bdf != 0) {
		eth_id =  nbl_res_vsi_id_to_eth_id(res_mgt, vsi_id);

		for (i = 0; i < NBL_VSI_MAX; i++)
			phy_ops->cfg_ipro_dn_sport_tbl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						       vsi_id + i, eth_id, bmode, binit);

		for (vfid = 0; vfid < sriov_info->num_vfs; vfid++) {
			vf_vsi_id = nbl_res_pfvfid_to_vsi_id(res_mgt, pfid, vfid, NBL_VSI_DATA);
			if (vf_vsi_id == 0xFFFF)
				continue;

			phy_ops->cfg_ipro_dn_sport_tbl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
							vf_vsi_id, eth_id, bmode, binit);
		}
	}

	return 0;
}

static int nbl_res_queue_set_bridge_mode(void *priv, u16 func_id, u16 bmode)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	return nbl_res_queue_init_ipro_dn_sport_tbl(res_mgt, func_id, bmode, false);
}

static int nbl_res_queue_init_rss(struct nbl_resource_mgt *res_mgt,
				  struct nbl_queue_mgt *queue_mgt,
				  struct nbl_phy_ops *phy_ops)
{
	return nbl_res_queue_init_epro_rss_key(res_mgt, phy_ops);
}

static int nbl_res_queue_alloc_txrx_queues(void *priv, u16 vsi_id, u16 queue_num)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	u64 notify_addr;
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	int ret = 0;

	notify_addr = nbl_res_get_func_bar_base_addr(res_mgt, func_id);

	ret = nbl_res_queue_setup_queue_info(res_mgt, func_id, queue_num);
	if (ret)
		goto setup_queue_info_fail;

	ret = nbl_res_queue_setup_qid_map_table_leonis(res_mgt, func_id, notify_addr);
	if (ret)
		goto setup_qid_map_fail;

	return 0;

setup_qid_map_fail:
	nbl_res_queue_remove_queue_info(res_mgt, func_id);
setup_queue_info_fail:
	return ret;
}

static void nbl_res_queue_free_txrx_queues(void *priv, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	struct nbl_event_queue_update_data event_data;

	nbl_res_queue_remove_qid_map_table_leonis(res_mgt, func_id);
	nbl_res_queue_remove_queue_info(res_mgt, func_id);
	event_data.func_id = func_id;
	event_data.ring_num = 0;
	event_data.map = NULL;
	nbl_event_notify(NBL_EVENT_QUEUE_ALLOC, &event_data, NBL_COMMON_TO_VSI_ID(res_mgt->common),
			 NBL_COMMON_TO_BOARD_ID(res_mgt->common));
}

static int nbl_res_queue_setup_queue(void *priv, struct nbl_txrx_queue_param *param, bool is_tx)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_cfg_param cfg_param = {0};
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, param->vsi_id);

	nbl_res_queue_setup_queue_cfg(NBL_RES_MGT_TO_QUEUE_MGT(res_mgt),
				      &cfg_param, param, is_tx, func_id);
	nbl_res_queue_setup_hw_dq(res_mgt, &cfg_param, func_id, param->vsi_id);
	return 0;
}

static int nbl_res_queue_remove_queue(void *priv, struct nbl_txrx_queue_param *param, bool is_tx)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_cfg_param cfg_param = {0};
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, param->vsi_id);

	nbl_res_queue_setup_queue_cfg(NBL_RES_MGT_TO_QUEUE_MGT(res_mgt),
				      &cfg_param, param, is_tx, func_id);
	nbl_res_queue_remove_hw_dq(res_mgt, &cfg_param, func_id);
	return 0;
}

static void nbl_res_queue_remove_all_queues(void *priv, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	struct nbl_queue_vsi_info *vsi_info = NULL;

	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return;

	nbl_res_queue_remove_all_hw_dq(res_mgt, func_id, vsi_info);
}

static int nbl_res_queue_register_vsi2q(void *priv, u16 vsi_index, u16 vsi_id,
					u16 queue_offset, u16 queue_num)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = NULL;
	struct nbl_queue_vsi_info *vsi_info = NULL;
	u16 func_id;

	func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	queue_info = &queue_mgt->queue_info[func_id];
	vsi_info = &queue_info->vsi_info[vsi_index];

	memset(vsi_info, 0, sizeof(*vsi_info));
	vsi_info->vld = 1;
	vsi_info->vsi_index = vsi_index;
	vsi_info->vsi_id = vsi_id;
	vsi_info->queue_offset = queue_offset;
	vsi_info->queue_num = queue_num;
	vsi_info->net_id = nbl_res_queue_get_net_id(func_id, vsi_info->vsi_index);

	return 0;
}

static int nbl_res_queue_cfg_dsch(void *priv, u16 vsi_id, bool vld)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_vsi_info *vsi_info;
	u16 group_id = nbl_res_vsi_id_to_eth_id(res_mgt, vsi_id); /* group_id is same with eth_id */
	u16 start = 0, end = 0;
	int i, ret = 0;

	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return -ENOENT;

	start = vsi_info->queue_offset;
	end = vsi_info->queue_num + vsi_info->queue_offset;

	/* When setting up, g2p -> n2g -> q2tc; when down, q2tc -> n2g -> g2p */
	if (!vld) {
		phy_ops->deactive_shaping(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_info->net_id);
		for (i = start; i < end; i++)
			phy_ops->cfg_q2tc_netid(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						queue_info->txrx_queues[i], vsi_info->net_id, vld);
		nbl_res_queue_update_netid_refnum(queue_mgt, vsi_info->net_id, false);
	}

	if (!nbl_res_queue_get_netid_refnum(queue_mgt, vsi_info->net_id)) {
		ret = phy_ops->cfg_dsch_net_to_group(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					     vsi_info->net_id, group_id, vld);
		if (ret)
			return ret;
	}

	if (vld) {
		for (i = start; i < end; i++)
			phy_ops->cfg_q2tc_netid(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						queue_info->txrx_queues[i], vsi_info->net_id, vld);
		phy_ops->active_shaping(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_info->net_id);
		nbl_res_queue_update_netid_refnum(queue_mgt, vsi_info->net_id, true);
	}

	return 0;
}

static int nbl_res_queue_setup_cqs(void *priv, u16 vsi_id, u16 real_qps, bool rss_indir_set)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info;
	struct nbl_queue_vsi_info *vsi_info = NULL;
	u16 func_id;

	func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	queue_info = &queue_mgt->queue_info[func_id];

	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return -ENOENT;

	if (real_qps == vsi_info->curr_qps)
		return 0;

	if (real_qps && rss_indir_set)
		phy_ops->cfg_epro_rss_ret(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					  vsi_info->rss_ret_base,
					  vsi_info->rss_entry_size, real_qps,
					  queue_info->txrx_queues + vsi_info->queue_offset, NULL);

	if (!vsi_info->curr_qps)
		phy_ops->set_epro_rss_pt(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id,
					 vsi_info->rss_ret_base, vsi_info->rss_entry_size);

	vsi_info->curr_qps = real_qps;
	vsi_info->curr_qps_static = real_qps;
	return 0;
}

static void nbl_res_queue_remove_cqs(void *priv, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_vsi_info *vsi_info = NULL;

	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return;

	phy_ops->clear_epro_rss_pt(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id);

	vsi_info->curr_qps = 0;
}

static int nbl_res_queue_init_switch(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_eth_info *eth_info = NBL_RES_MGT_TO_ETH_INFO(res_mgt);
	int i;

	for_each_set_bit(i, eth_info->eth_bitmap, NBL_MAX_ETHERNET)
		phy_ops->setup_queue_switch(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), i);

	return 0;
}

static int nbl_res_queue_init(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt;
	struct nbl_phy_ops *phy_ops;
	int i, ret = 0;

	if (!res_mgt)
		return -EINVAL;

	queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	ret = nbl_res_queue_init_qid_map_table(res_mgt, queue_mgt, phy_ops);
	if (ret)
		goto init_queue_fail;

	ret = nbl_res_queue_init_rss(res_mgt, queue_mgt, phy_ops);
	if (ret)
		goto init_queue_fail;

	ret = nbl_res_queue_init_switch(res_mgt);
	if (ret)
		goto init_queue_fail;

	for (i = 0; i < NBL_RES_MGT_TO_PF_NUM(res_mgt); i++) {
		nbl_res_queue_init_epro_vpt_table(res_mgt, i);
		nbl_res_queue_init_ipro_dn_sport_tbl(res_mgt, i, BRIDGE_MODE_VEB, true);
	}
	phy_ops->init_pfc(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), NBL_MAX_ETHERNET);

	return 0;

init_queue_fail:
	return ret;
}

static int nbl_res_queue_get_queue_err_stats(void *priv, u16 func_id, u8 queue_id,
					     struct nbl_queue_err_stats *queue_err_stats,
					     bool is_tx)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];
	struct nbl_phy_ops *phy_ops;
	u16 global_queue_id;

	if (queue_id >= queue_info->num_txrx_queues)
		return -EINVAL;

	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	global_queue_id = queue_info->txrx_queues[queue_id];

	if (is_tx)
		phy_ops->get_tx_queue_err_stats(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						global_queue_id, queue_err_stats);
	else
		phy_ops->get_rx_queue_err_stats(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						global_queue_id, queue_err_stats);

	return 0;
}

static int nbl_res_queue_cfg_qdisc_mqprio(void *priv, struct nbl_tc_qidsc_param *param)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *res_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info;
	struct nbl_queue_vsi_info *vsi_info = NULL;
	u64 total_tx_rate = 0, max_rate = 0, max_tc_rate = 0;
	u16 func_id, curr_qps = 0, queue_id = 0;
	u8 *weight;
	bool is_active = false;
	int i, j, gravity;

	func_id = nbl_res_vsi_id_to_func_id(res_mgt, param->vsi_id);
	queue_info = &queue_mgt->queue_info[func_id];
	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, param->vsi_id);

	switch (res_info->board_info.eth_speed) {
	case NBL_FW_PORT_SPEED_100G:
		max_rate = NBL_RATE_MBPS_100G;
		break;
	case NBL_FW_PORT_SPEED_25G:
		max_rate = NBL_RATE_MBPS_25G;
		break;
	case NBL_FW_PORT_SPEED_10G:
		max_rate = NBL_RATE_MBPS_10G;
		break;
	default:
		return -EOPNOTSUPP;
	}

	for (i = 0; i < param->num_tc; i++)
		total_tx_rate += param->info[i].max_tx_rate;

	if (total_tx_rate > max_rate) {
		nbl_err(NBL_RES_MGT_TO_COMMON(res_mgt), NBL_DEBUG_QUEUE,
			"Invalid total_tx_rate: %llu mbps, should within %llu mbps",
			total_tx_rate, max_rate);
		return -EINVAL;
	}

	for (i = 0; i < vsi_info->curr_qps; i++) {
		queue_id = queue_info->txrx_queues[i + vsi_info->queue_offset];
		phy_ops->lso_dsch_drain(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), queue_id);
		is_active |= phy_ops->check_q2tc(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), queue_id);
	}

	/* Config tc */
	for (i = 0; i < param->num_tc; i++)
		for (j = 0; j < param->info[i].count; j++) {
			phy_ops->cfg_q2tc_tcid(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					       queue_info->txrx_queues[curr_qps +
								       vsi_info->queue_offset], i);
			curr_qps++;
		}

	for (i = curr_qps; i < param->origin_qps; i++)
		phy_ops->cfg_q2tc_tcid(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				       queue_info->txrx_queues[i + vsi_info->queue_offset], 0);

	/* Config weight */
	weight = kcalloc(param->num_tc, sizeof(*weight), GFP_KERNEL);
	if (!weight)
		return -ENOMEM;

	for (i = 0; i < param->num_tc; i++)
		if (param->info[i].max_tx_rate > max_tc_rate)
			max_tc_rate = param->info[i].max_tx_rate;

	gravity = max_tc_rate / NBL_SHAPING_WGT_MAX + 1;

	for (i = 0; i < param->num_tc; i++)
		weight[i] = param->info[i].max_tx_rate / gravity;

	phy_ops->set_tc_wgt(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_info->net_id,
			    weight, param->num_tc);

	/* Config shaping */
	phy_ops->set_shaping(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_info->net_id, total_tx_rate, 0,
			     param->enable && total_tx_rate, is_active);

	kfree(weight);
	return 0;
}

static int nbl_res_queue_set_tc_wgt(void *priv, u16 vsi_id, u8 *weight, u8 num_tc)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_vsi_info *vsi_info = NULL;
	u8 *weight_to_set;
	int i;

	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	weight_to_set = kcalloc(num_tc, sizeof(*weight_to_set), GFP_KERNEL);
	if (!weight_to_set)
		return -ENOMEM;

	for (i = 0; i < num_tc; i++)
		weight_to_set[i] = weight[i] * NBL_SHAPING_WGT_MAX / NBL_TC_MAX_BW;
	phy_ops->set_tc_wgt(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_info->net_id,
			    weight_to_set, num_tc);

	kfree(weight_to_set);
	return 0;
}

static void nbl_res_restore_tc_mgt(void *priv, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_vsi_info *vsi_info = NULL;
	u8 *weight;
	int i;

	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return;
	weight = kcalloc(NBL_MAX_TC_NUM, sizeof(*weight), GFP_KERNEL);
	if (!weight)
		return;

	for (i = 0; i < NBL_MAX_TC_NUM; i++)
		weight[i] = 1;
	phy_ops->set_tc_wgt(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_info->net_id,
			    weight, NBL_MAX_TC_NUM);

	kfree(weight);
}

static u16 nbl_res_queue_get_local_queue_id(void *priv, u16 vsi_id, u16 global_queue_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info;
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	int i;

	queue_info = &queue_mgt->queue_info[func_id];

	if (queue_info->txrx_queues)
		for (i = 0; i < queue_info->num_txrx_queues; i++)
			if (global_queue_id == queue_info->txrx_queues[i])
				return i;

	return U16_MAX;
}

static u16 nbl_res_queue_get_vsi_global_qid(void *priv, u16 vsi_id, u16 local_qid)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];

	if (!queue_info->num_txrx_queues)
		return 0xffff;

	return queue_info->txrx_queues[local_qid];
}

static void nbl_res_queue_get_rxfh_indir_size(void *priv, u16 vsi_id, u32 *rxfh_indir_size)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_vsi_info *vsi_info = NULL;

	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return;

	*rxfh_indir_size = NBL_EPRO_RSS_ENTRY_SIZE_UNIT << vsi_info->rss_entry_size;
}

static void nbl_res_queue_get_rxfh_indir(void *priv, u16 vsi_id, u32 *indir)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_vsi_info *vsi_info = NULL;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	u16 i, indir_size;

	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return;

	phy_ops->read_rss_indir(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id, indir,
				vsi_info->rss_ret_base, vsi_info->rss_entry_size);

	indir_size = NBL_EPRO_RSS_ENTRY_SIZE_UNIT << vsi_info->rss_entry_size;
	for (i = 0; i < indir_size; i++) {
		indir[i] = nbl_res_queue_get_local_queue_id(res_mgt, vsi_id, indir[i]);
		indir[i] -= vsi_info->queue_offset;
	}
}

static int nbl_res_queue_set_rxfh_indir(void *priv, u16 vsi_id, const u32 *indir, u32 indir_size)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_vsi_info *vsi_info = NULL;
	struct nbl_queue_info *queue_info = NULL;
	u32 *rss_ret;
	u16 func_id = 0;
	int i = 0;

	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return -ENOENT;

	if (indir) {
		rss_ret = kcalloc(indir_size, sizeof(indir[0]), GFP_KERNEL);
		if (!rss_ret)
			return -ENOMEM;
		func_id = NBL_COMMON_TO_MGT_PF(common);
		queue_info = &queue_mgt->queue_info[func_id];
		/* local queue to global queue */
		for (i = 0; i < indir_size; i++)
			rss_ret[i] = nbl_res_queue_get_vsi_global_qid(res_mgt, vsi_id,
								      vsi_info->queue_offset +
								      indir[i]);
		phy_ops->cfg_epro_rss_ret(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					  vsi_info->rss_ret_base,
					  vsi_info->rss_entry_size, 0,
					  NULL, rss_ret);
		kfree(rss_ret);
	}

	if (!vsi_info->curr_qps)
		phy_ops->set_epro_rss_pt(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id,
					 vsi_info->rss_ret_base, vsi_info->rss_entry_size);

	return 0;
}

static void nbl_res_queue_get_rxfh_rss_key_size(void *priv, u32 *rxfh_rss_key_size)
{
	*rxfh_rss_key_size = NBL_EPRO_RSS_SK_SIZE;
}

static void nbl_res_rss_key_reverse_order(u8 *key)
{
	u8 temp;
	int i;

	for (i = 0; i < (NBL_EPRO_RSS_PER_KEY_SIZE / 2); i++) {
		temp = key[i];
		key[i] = key[NBL_EPRO_RSS_PER_KEY_SIZE - 1 - i];
		key[NBL_EPRO_RSS_PER_KEY_SIZE - 1 - i] = temp;
	}
}

static void nbl_res_queue_get_rss_key(void *priv, u8 *rss_key)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	int i;

	phy_ops->read_rss_key(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), rss_key);

	for (i = 0; i < NBL_EPRO_RSS_KEY_NUM; i++)
		nbl_res_rss_key_reverse_order(rss_key + i * NBL_EPRO_RSS_PER_KEY_SIZE);
}

static void nbl_res_queue_get_rss_alg_sel(void *priv, u16 vsi_id, u8 *alg_sel)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->get_rss_alg_sel(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id, alg_sel);
}

static int nbl_res_queue_set_rss_alg_sel(void *priv, u16 vsi_id, u8 alg_sel)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	int ret = 0;

	ret = phy_ops->set_rss_alg_sel(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id, alg_sel);
	return ret;
}

static void nbl_res_queue_clear_queues(void *priv, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];
	struct nbl_tc_qidsc_param param;

	nbl_res_queue_remove_rss(priv, vsi_id);
	nbl_res_queue_remove_q2vsi(priv, vsi_id);
	nbl_res_restore_tc_mgt(priv, vsi_id);
	if (!queue_info->num_txrx_queues)
		return;

	memset(&param, 0, sizeof(param));
	/* clear shapping */
	param.vsi_id = vsi_id;
	param.enable = false;
	nbl_res_queue_cfg_qdisc_mqprio(priv, &param);
	nbl_res_queue_remove_cqs(res_mgt, vsi_id);
	nbl_res_queue_cfg_dsch(res_mgt, vsi_id, false);
	nbl_res_queue_remove_all_queues(res_mgt, vsi_id);
	nbl_res_queue_free_txrx_queues(res_mgt, vsi_id);
}

static int nbl_res_queue_cfg_log(void *priv, u16 vsi_id, u16 qps, bool vld)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	u16 global_queue, i;

	if (!queue_info->num_txrx_queues)
		return 0;

	for (i = 0; i < qps; i++) {
		global_queue = queue_info->txrx_queues[i];
		phy_ops->cfg_vnet_qinfo_log(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					    NBL_PAIR_ID_GET_RX(global_queue), vld);
		phy_ops->cfg_vnet_qinfo_log(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					    NBL_PAIR_ID_GET_TX(global_queue), vld);
	}

	return 0;
}

static u16 nbl_req_queue_get_ctx(void *priv, u16 vsi_id, u16 qid)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	u16 func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];

	if (!queue_info->num_txrx_queues)
		return 0xffff;

	return queue_info->queues_context[qid];
}

static u16 nbl_get_adapt_desc_gother_level(u16 last_level, u64 rates)
{
	switch (last_level) {
	case NBL_ADAPT_DESC_GOTHER_LEVEL0:
		if (rates > NBL_ADAPT_DESC_GOTHER_LEVEL1_TH)
			return NBL_ADAPT_DESC_GOTHER_LEVEL1;
		else
			return NBL_ADAPT_DESC_GOTHER_LEVEL0;
	case NBL_ADAPT_DESC_GOTHER_LEVEL1:
		if (rates > NBL_ADAPT_DESC_GOTHER_LEVEL1_DOWNGRADE_TH)
			return NBL_ADAPT_DESC_GOTHER_LEVEL1;
		else
			return NBL_ADAPT_DESC_GOTHER_LEVEL0;
	default:
		return NBL_ADAPT_DESC_GOTHER_LEVEL0;
	}
}

static u16 nbl_get_adapt_desc_gother_timeout(u16 level)
{
	switch (level) {
	case NBL_ADAPT_DESC_GOTHER_LEVEL0:
		return NBL_ADAPT_DESC_GOTHER_LEVEL0_TIMEOUT;
	case NBL_ADAPT_DESC_GOTHER_LEVEL1:
		return NBL_ADAPT_DESC_GOTHER_LEVEL1_TIMEOUT;
	default:
		return NBL_ADAPT_DESC_GOTHER_LEVEL0_TIMEOUT;
	}
}

static void nbl_res_queue_adapt_desc_gother(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_adapt_desc_gother *adapt_desc_gother = &queue_mgt->adapt_desc_gother;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	u32 last_uvn_desc_rd_entry = adapt_desc_gother->uvn_desc_rd_entry;
	u64 last_get_stats_jiffies = adapt_desc_gother->get_desc_stats_jiffies;
	u64 time_diff;
	u32 uvn_desc_rd_entry;
	u32 rx_rate;
	u16 level, last_level, timeout;

	last_level = adapt_desc_gother->level;
	time_diff = jiffies - last_get_stats_jiffies;
	uvn_desc_rd_entry = phy_ops->get_uvn_desc_entry_stats(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
	rx_rate = (uvn_desc_rd_entry - last_uvn_desc_rd_entry) / time_diff * HZ;
	adapt_desc_gother->get_desc_stats_jiffies = jiffies;
	adapt_desc_gother->uvn_desc_rd_entry = uvn_desc_rd_entry;

	level = nbl_get_adapt_desc_gother_level(last_level, rx_rate);
	if (level != last_level) {
		timeout = nbl_get_adapt_desc_gother_timeout(level);
		phy_ops->set_uvn_desc_wr_timeout(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), timeout);
		adapt_desc_gother->level = level;
	}
}

static void nbl_res_queue_set_desc_high_throughput(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_adapt_desc_gother *adapt_desc_gother = &queue_mgt->adapt_desc_gother;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	if (adapt_desc_gother->level != NBL_ADAPT_DESC_GOTHER_LEVEL1) {
		phy_ops->set_uvn_desc_wr_timeout(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						NBL_ADAPT_DESC_GOTHER_LEVEL1_TIMEOUT);
		adapt_desc_gother->level = NBL_ADAPT_DESC_GOTHER_LEVEL1;
	}
}

static void nbl_res_flr_clear_queues(void *priv, u16 vf_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	u16 func_id = vf_id + NBL_MAX_PF;
	u16 vsi_id = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_SERV_VF_DATA_TYPE);

	if (nbl_res_vf_is_active(priv, func_id))
		nbl_res_queue_clear_queues(priv, vsi_id);
}

static int nbl_res_queue_restore_tx_queue(struct nbl_resource_mgt *res_mgt, u16 vsi_id,
					  u16 local_queue_id, dma_addr_t dma)
{
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_info *queue_info;
	struct nbl_queue_vsi_info *vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	struct nbl_queue_cfg_param queue_cfg = {0};
	u16 global_queue, func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);

	if (!vsi_info)
		return -ENOSPC;

	queue_info = &queue_mgt->queue_info[func_id];
	global_queue = queue_info->txrx_queues[local_queue_id];

	phy_ops->get_tx_queue_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), &queue_cfg, global_queue);
	/* Rectify size, in register it is log2(size) */
	queue_cfg.size = queue_info->queue_size;
	/* DMA addr is realloced, updated it */
	queue_cfg.desc = dma;

	phy_ops->lso_dsch_drain(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);
	phy_ops->disable_dvn(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);

	phy_ops->reset_dvn_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);

	phy_ops->cfg_tx_queue(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), &queue_cfg, global_queue);
	phy_ops->cfg_q2tc_netid(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				global_queue, vsi_info->net_id, 1);

	return 0;
}

static int nbl_res_queue_restore_rx_queue(struct nbl_resource_mgt *res_mgt, u16 vsi_id,
					  u16 local_queue_id, dma_addr_t dma)
{
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_info *queue_info;
	struct nbl_queue_cfg_param queue_cfg = {0};
	u16 global_queue, func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);

	queue_info = &queue_mgt->queue_info[func_id];
	global_queue = queue_info->txrx_queues[local_queue_id];

	phy_ops->get_rx_queue_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), &queue_cfg, global_queue);
	/* Rectify size, in register it is log2(size) */
	queue_cfg.size = queue_info->queue_size;
	/* DMA addr is realloced, updated it */
	queue_cfg.desc = dma;

	phy_ops->disable_uvn(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);
	phy_ops->rsc_cache_drain(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);

	phy_ops->reset_uvn_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);
	nbl_res_queue_reset_uvn_pkt_drop_stats(res_mgt, func_id, global_queue);

	phy_ops->cfg_rx_queue(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), &queue_cfg, global_queue);

	return 0;
}

static int nbl_res_queue_restore_hw_queue(void *priv, u16 vsi_id, u16 local_queue_id,
					  dma_addr_t dma, int type)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	switch (type) {
	case NBL_TX:
		return nbl_res_queue_restore_tx_queue(res_mgt, vsi_id, local_queue_id, dma);
	case NBL_RX:
		return nbl_res_queue_restore_rx_queue(res_mgt, vsi_id, local_queue_id, dma);
	default:
		break;
	}

	return -EINVAL;
}

static int
nbl_res_queue_stop_abnormal_hw_queue(void *priv, u16 vsi_id, u16 local_queue_id, int type)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_info *queue_info;
	u16 global_queue, func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);

	queue_info = &queue_mgt->queue_info[func_id];
	global_queue = queue_info->txrx_queues[local_queue_id];
	switch (type) {
	case NBL_TX:
		phy_ops->lso_dsch_drain(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);
		phy_ops->disable_dvn(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);

		phy_ops->reset_dvn_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);
		return 0;
	case NBL_RX:
		phy_ops->disable_uvn(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);
		phy_ops->rsc_cache_drain(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);

		phy_ops->reset_uvn_cfg(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), global_queue);
		nbl_res_queue_reset_uvn_pkt_drop_stats(res_mgt, func_id, global_queue);
		return 0;
	default:
		break;
	}

	return -EINVAL;
}

static int nbl_res_queue_set_tx_rate(void *priv, u16 func_id, int tx_rate, int burst)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *res_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];
	struct nbl_queue_vsi_info *vsi_info = NULL;
	u16 vsi_id, queue_id;
	bool is_active = false;
	int max_rate = 0, i;

	vsi_id = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_SERV_VF_DATA_TYPE);
	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);

	if (!vsi_info)
		return 0;

	switch (res_info->board_info.eth_speed) {
	case NBL_FW_PORT_SPEED_100G:
		max_rate = NBL_RATE_MBPS_100G;
		break;
	case NBL_FW_PORT_SPEED_25G:
		max_rate = NBL_RATE_MBPS_25G;
		break;
	case NBL_FW_PORT_SPEED_10G:
		max_rate = NBL_RATE_MBPS_10G;
		break;
	default:
		return -EOPNOTSUPP;
	}

	if (tx_rate > max_rate)
		return -EINVAL;

	if (queue_info->txrx_queues)
		for (i = 0; i < vsi_info->curr_qps; i++) {
			queue_id = queue_info->txrx_queues[i + vsi_info->queue_offset];
			is_active |= phy_ops->check_q2tc(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
							 queue_id);
		}

	/* Config shaping */
	return phy_ops->set_shaping(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_info->net_id, tx_rate,
				    burst, !!(tx_rate), is_active);
}

static int nbl_res_queue_set_rx_rate(void *priv, u16 func_id, int rx_rate, int burst)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_resource_info *res_info = NBL_RES_MGT_TO_RES_INFO(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_vsi_info *vsi_info = NULL;
	u16 vsi_id;
	int max_rate = 0;

	vsi_id = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_DATA);
	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);

	if (!vsi_info)
		return 0;

	switch (res_info->board_info.eth_speed) {
	case NBL_FW_PORT_SPEED_100G:
		max_rate = NBL_RATE_MBPS_100G;
		break;
	case NBL_FW_PORT_SPEED_25G:
		max_rate = NBL_RATE_MBPS_25G;
		break;
	case NBL_FW_PORT_SPEED_10G:
		max_rate = NBL_RATE_MBPS_10G;
		break;
	default:
		return -EOPNOTSUPP;
	}

	if (rx_rate > max_rate)
		return -EINVAL;

	/* Config ucar */
	return phy_ops->set_ucar(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), vsi_id, rx_rate,
				 burst, !!(rx_rate));
}

static void nbl_res_queue_get_active_func_bitmaps(void *priv, unsigned long *bitmap, int max_func)
{
	int i;
	int func_id_end;
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	func_id_end = max_func > NBL_MAX_FUNC ? NBL_MAX_FUNC : max_func;
	for (i = 0; i < func_id_end; i++) {
		if (!nbl_res_check_func_active_by_queue(res_mgt, i))
			continue;

		set_bit(i, bitmap);
	}
}

static int nbl_res_queue_configure_mirror_table(void *priv, bool mirror_en, u16 func_id, u8 mt_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info;
	struct nbl_event_mirror_outputport_data blacklist_data = {0};
	u16 vsi_id;
	u16 queue_id = 0;

	queue_info =  &queue_mgt->queue_info[func_id];
	if (!queue_info->num_txrx_queues) {
		dev_err(dev, "func:%d num_txrx_queues is 0!", func_id);
		return -EINVAL;
	}

	blacklist_data.opcode = mirror_en;
	blacklist_data.func_id = func_id;
	nbl_event_notify(NBL_EVENT_MIRROR_OUTPUTPORT, &blacklist_data, NBL_COMMON_TO_VSI_ID(common),
			 NBL_COMMON_TO_BOARD_ID(common));

	queue_id = queue_info->txrx_queues[0];
	vsi_id = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_SERV_VF_DATA_TYPE);

	return phy_ops->configure_mirror_table(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					       mirror_en, vsi_id, queue_id, mt_id);
}

static void nbl_res_queue_set_dvn_desc_req(void *priv, u32 desc_req)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->set_dvn_desc_req(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), desc_req);
}

static u32 nbl_res_queue_get_dvn_desc_req(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	return phy_ops->get_dvn_desc_req(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
}

static int nbl_res_queue_reset_uvn_pkt_drop_stats(void *priv, u16 func_id, u16 global_queue_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = &queue_mgt->queue_info[func_id];
	u16 vsi_id = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_SERV_PF_DATA_TYPE);
	u16 local_queue_id;

	local_queue_id = nbl_res_queue_get_local_queue_id(res_mgt, vsi_id, global_queue_id);
	queue_info->uvn_stat_pkt_drop[local_queue_id] = 0;
	return 0;
}

static int nbl_res_queue_get_uvn_pkt_drop_stats(void *priv, u16 vsi_id,
						u16 num_queues, u32 *uvn_stat_pkt_drop)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_queue_mgt *queue_mgt = NBL_RES_MGT_TO_QUEUE_MGT(res_mgt);
	struct nbl_queue_info *queue_info = NULL;
	struct nbl_queue_vsi_info *vsi_info = NULL;
	u16 func_id = 0;
	u32 pkt_drop_num = 0;
	int i = 0;

	func_id = nbl_res_vsi_id_to_func_id(res_mgt, vsi_id);
	queue_info = &queue_mgt->queue_info[func_id];
	vsi_info = nbl_res_queue_get_vsi_info(res_mgt, vsi_id);
	if (!vsi_info)
		return -ENOENT;

	for (i = vsi_info->queue_offset;
	     i < vsi_info->queue_offset + num_queues &&
	     i < queue_info->num_txrx_queues; i++) {
		phy_ops->get_uvn_pkt_drop_stats(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						queue_info->txrx_queues[i], &pkt_drop_num);
		*uvn_stat_pkt_drop = pkt_drop_num - queue_info->uvn_stat_pkt_drop[i];
		uvn_stat_pkt_drop++;
		queue_info->uvn_stat_pkt_drop[i] = pkt_drop_num;
	}

	return 0;
}

/* NBL_QUEUE_SET_OPS(ops_name, func)
 *
 * Use X Macros to reduce setup and remove codes.
 */
#define NBL_QUEUE_OPS_TBL									\
do {												\
	NBL_QUEUE_SET_OPS(alloc_txrx_queues, nbl_res_queue_alloc_txrx_queues);			\
	NBL_QUEUE_SET_OPS(free_txrx_queues, nbl_res_queue_free_txrx_queues);			\
	NBL_QUEUE_SET_OPS(register_vsi2q, nbl_res_queue_register_vsi2q);			\
	NBL_QUEUE_SET_OPS(setup_q2vsi, nbl_res_queue_setup_q2vsi);				\
	NBL_QUEUE_SET_OPS(remove_q2vsi, nbl_res_queue_remove_q2vsi);				\
	NBL_QUEUE_SET_OPS(setup_rss, nbl_res_queue_setup_rss);					\
	NBL_QUEUE_SET_OPS(remove_rss, nbl_res_queue_remove_rss);				\
	NBL_QUEUE_SET_OPS(setup_queue, nbl_res_queue_setup_queue);				\
	NBL_QUEUE_SET_OPS(remove_queue, nbl_res_queue_remove_queue);				\
	NBL_QUEUE_SET_OPS(remove_all_queues, nbl_res_queue_remove_all_queues);			\
	NBL_QUEUE_SET_OPS(cfg_dsch, nbl_res_queue_cfg_dsch);					\
	NBL_QUEUE_SET_OPS(setup_cqs, nbl_res_queue_setup_cqs);					\
	NBL_QUEUE_SET_OPS(remove_cqs, nbl_res_queue_remove_cqs);				\
	NBL_QUEUE_SET_OPS(queue_init, nbl_res_queue_init);					\
	NBL_QUEUE_SET_OPS(get_queue_err_stats, nbl_res_queue_get_queue_err_stats);		\
	NBL_QUEUE_SET_OPS(cfg_qdisc_mqprio, nbl_res_queue_cfg_qdisc_mqprio);			\
	NBL_QUEUE_SET_OPS(get_rxfh_indir_size, nbl_res_queue_get_rxfh_indir_size);		\
	NBL_QUEUE_SET_OPS(get_rxfh_indir, nbl_res_queue_get_rxfh_indir);			\
	NBL_QUEUE_SET_OPS(set_rxfh_indir, nbl_res_queue_set_rxfh_indir);			\
	NBL_QUEUE_SET_OPS(get_rxfh_rss_key_size, nbl_res_queue_get_rxfh_rss_key_size);		\
	NBL_QUEUE_SET_OPS(get_rxfh_rss_key, nbl_res_queue_get_rss_key);				\
	NBL_QUEUE_SET_OPS(get_rss_alg_sel, nbl_res_queue_get_rss_alg_sel);			\
	NBL_QUEUE_SET_OPS(set_rss_alg_sel, nbl_res_queue_set_rss_alg_sel);			\
	NBL_QUEUE_SET_OPS(clear_queues, nbl_res_queue_clear_queues);				\
	NBL_QUEUE_SET_OPS(get_vsi_global_queue_id, nbl_res_queue_get_vsi_global_qid);		\
	NBL_QUEUE_SET_OPS(cfg_queue_log, nbl_res_queue_cfg_log);				\
	NBL_QUEUE_SET_OPS(get_queue_ctx, nbl_req_queue_get_ctx);				\
	NBL_QUEUE_SET_OPS(adapt_desc_gother, nbl_res_queue_adapt_desc_gother);			\
	NBL_QUEUE_SET_OPS(set_desc_high_throughput, nbl_res_queue_set_desc_high_throughput);	\
	NBL_QUEUE_SET_OPS(flr_clear_queues, nbl_res_flr_clear_queues);				\
	NBL_QUEUE_SET_OPS(restore_hw_queue, nbl_res_queue_restore_hw_queue);			\
	NBL_QUEUE_SET_OPS(get_local_queue_id, nbl_res_queue_get_local_queue_id);		\
	NBL_QUEUE_SET_OPS(set_bridge_mode, nbl_res_queue_set_bridge_mode);			\
	NBL_QUEUE_SET_OPS(set_tx_rate, nbl_res_queue_set_tx_rate);				\
	NBL_QUEUE_SET_OPS(set_rx_rate, nbl_res_queue_set_rx_rate);				\
	NBL_QUEUE_SET_OPS(stop_abnormal_hw_queue, nbl_res_queue_stop_abnormal_hw_queue);	\
	NBL_QUEUE_SET_OPS(get_active_func_bitmaps, nbl_res_queue_get_active_func_bitmaps);	\
	NBL_QUEUE_SET_OPS(set_tc_wgt, nbl_res_queue_set_tc_wgt);				\
	NBL_QUEUE_SET_OPS(configure_mirror_table, nbl_res_queue_configure_mirror_table);	\
	NBL_QUEUE_SET_OPS(get_dvn_desc_req, nbl_res_queue_get_dvn_desc_req);			\
	NBL_QUEUE_SET_OPS(set_dvn_desc_req, nbl_res_queue_set_dvn_desc_req);			\
	NBL_QUEUE_SET_OPS(get_uvn_pkt_drop_stats, nbl_res_queue_get_uvn_pkt_drop_stats);	\
} while (0)

int nbl_queue_setup_ops_leonis(struct nbl_resource_ops *res_ops)
{
#define NBL_QUEUE_SET_OPS(name, func) do {res_ops->NBL_NAME(name) = func; ; } while (0)
	NBL_QUEUE_OPS_TBL;
#undef  NBL_QUEUE_SET_OPS

	return 0;
}

void nbl_queue_remove_ops_leonis(struct nbl_resource_ops *res_ops)
{
#define NBL_QUEUE_SET_OPS(name, func) do {res_ops->NBL_NAME(name) = NULL; ; } while (0)
	NBL_QUEUE_OPS_TBL;
#undef  NBL_QUEUE_SET_OPS
}

void nbl_queue_mgt_init_leonis(struct nbl_queue_mgt *queue_mgt)
{
	queue_mgt->qid_map_select = NBL_MASTER_QID_MAP_TABLE;
}
