// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#include "nbl_fc.h"

static void nbl_fc_update_stats(__maybe_unused struct flow_stats *flow_stats,
				__maybe_unused u64 bytes, __maybe_unused u64 pkts,
				__maybe_unused u64 drops, __maybe_unused u64 lastused)
{
	flow_stats_update(flow_stats, bytes, pkts, drops, lastused,
			  FLOW_ACTION_HW_STATS_DELAYED);
}

static int nbl_fc_get_stats(void *priv, struct nbl_stats_param *param)
{
	int idx;
	int i;
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_fc_mgt *mgt;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	u64 pkts = 0;
	u64 bytes = 0;
	struct nbl_flow_counter *counter = NULL;
	unsigned long cookie = param->f->cookie;
	struct nbl_index_key_extra extra_key;

	if (phy_ops->get_hw_status(NBL_RES_MGT_TO_PHY_PRIV(res_mgt)) == NBL_HW_FATAL_ERR)
		return -EIO;

	mgt = NBL_RES_MGT_TO_COUNTER_MGT(res_mgt);
	if (!mgt) {
		nbl_err(common, NBL_DEBUG_FLOW, "nbl flow fc not been init.");
		return -EPERM;
	}

	spin_lock(&mgt->counter_lock);
	NBL_INDEX_EXTRA_KEY_INIT(&extra_key, 0, 0, true);
	for (i = 0; i < NBL_FC_TYPE_MAX; i++) {
		idx = nbl_common_get_index_with_data(mgt->cls_cookie_tbl[i], &cookie, &extra_key,
						     NULL, 0, (void **)&counter);
		if (idx != U32_MAX)
			break;
	}

	if (!counter || i >= NBL_FC_TYPE_MAX) {
		spin_unlock(&mgt->counter_lock);
		return -EINVAL;
	}

	if (i == NBL_FC_SPEC_TYPE)
		mgt->fc_ops.get_spec_stats(counter, &pkts, &bytes);
	else
		mgt->fc_ops.get_flow_stats(counter, &pkts, &bytes);

	counter->lastpackets = counter->cache.packets;
	counter->lastbytes = counter->cache.bytes;

	nbl_fc_update_stats(&param->f->stats, bytes, pkts, 0, counter->lastuse);

	spin_unlock(&mgt->counter_lock);

	return 0;
}

static void flow_counter_update(struct nbl_fc_mgt *mgt, enum nbl_pp_fc_type fc_type)
{
	u32 idx = 0;
	u32 flow_num = 0;
	u32 i = 0;
	struct nbl_flow_counter *iter_counter = NULL;
	struct nbl_flow_query_counter counter_array;
	struct list_head *counter_list;

	memset(&counter_array, 0, sizeof(counter_array));

	if (fc_type == NBL_FC_COMMON_TYPE)
		counter_list = &mgt->counter_hash_list;
	else
		counter_list = &mgt->counter_stat_hash_list;

	spin_lock(&mgt->counter_lock);
	list_for_each_entry(iter_counter, counter_list, entries) {
		mgt->counter_update_list[idx].counter_id = iter_counter->counter_id;
		mgt->counter_update_list[idx].cookie = iter_counter->cookie;
		idx++;
	}
	spin_unlock(&mgt->counter_lock);
	/* using command queue */
	for (i = 0; i < idx; i++) {
		counter_array.counter_id[flow_num] = mgt->counter_update_list[i].counter_id;
		counter_array.cookie[flow_num] = mgt->counter_update_list[i].cookie;
		++flow_num;

		/* send bluk of cmdqueue query */
		if (flow_num == NBL_FLOW_COUNT_NUM) {
			mgt->fc_ops.update_stats(mgt, &counter_array, flow_num, 0, fc_type);
			flow_num = 0;
		}
	}

	if (flow_num) {
		mgt->fc_ops.update_stats(mgt, &counter_array, flow_num, 0, fc_type);
		flow_num = 0;
	}

	nbl_debug(mgt->common, NBL_DEBUG_FLOW, "nbl flow fc start update counter type %d, all=%u",
		  fc_type, idx);
}

static void nbl_fc_stats_work(struct work_struct *work)
{
	struct delayed_work *delayed_work = to_delayed_work(work);
	struct nbl_fc_mgt *mgt = container_of(delayed_work,
						   struct nbl_fc_mgt, counter_work);
	unsigned long now = jiffies;

	if (!list_empty(&mgt->counter_hash_list) || !list_empty(&mgt->counter_stat_hash_list))
		queue_delayed_work(mgt->counter_wq, &mgt->counter_work, mgt->query_interval);

	/* no need too much overhead in counter update */
	if (time_before(now, mgt->next_query))
		return;
	flow_counter_update(mgt, NBL_FC_COMMON_TYPE);
	flow_counter_update(mgt, NBL_FC_SPEC_TYPE);
	mgt->next_query = now + mgt->query_interval;
}

static void nbl_fc_free_res(struct nbl_fc_mgt *mgt)
{
	int i;

	kfree(mgt->counter_update_list);
	mgt->counter_update_list = NULL;

	for (i = 0; i < NBL_FC_TYPE_MAX; i++) {
		nbl_common_remove_index_table(mgt->cls_cookie_tbl[i], NULL);
		mgt->cls_cookie_tbl[i] = NULL;
	}
}

static int nbl_fc_init_hash_map(struct nbl_fc_mgt *mgt)
{
	int i;
	u32 idx_num[NBL_FC_TYPE_MAX] = {NBL_COUNTER_MAX_ID, NBL_COUNTER_MAX_STAT_ID};
	struct nbl_index_tbl_key tbl_key;

	mgt->counter_update_list = kcalloc(NBL_COUNTER_MAX_ID, sizeof(*mgt->counter_update_list),
					   GFP_KERNEL);
	if (!mgt->counter_update_list)
		goto alloc_counter_list_failed;

	for (i = 0; i < NBL_FC_TYPE_MAX; i++) {
		NBL_INDEX_TBL_KEY_INIT(&tbl_key, NBL_COMMON_TO_DEV(mgt->common), 0,
				       idx_num[i], sizeof(unsigned long));
		mgt->cls_cookie_tbl[i] = nbl_common_init_index_table(&tbl_key);
		if (!mgt->cls_cookie_tbl[i])
			goto alloc_index_tbl_failed;
	}

	return 0;

alloc_index_tbl_failed:
alloc_counter_list_failed:
	return -ENOMEM;
}

/* NBL_COUNTER_SET_OPS(ops_name, func)
 *
 * Use X Macros to reduce setup and remove codes.
 */
#define NBL_COUNTER_OPS_TBL						\
do {									\
	NBL_COUNTER_SET_OPS(query_tc_stats, nbl_fc_get_stats);	\
} while (0)

static void nbl_fc_remove_mgt(struct device *dev, struct nbl_fc_mgt **fc_mgt)
{
	devm_kfree(dev, *fc_mgt);
	*fc_mgt = NULL;
}

int nbl_fc_set_stats(struct nbl_fc_mgt *mgt, void *data, unsigned long cookie)
{
	int ret = 0;
	int i;
	int idx;
	struct nbl_stats_data *data_info = (struct nbl_stats_data *)data;
	struct nbl_flow_counter *counter_node = NULL;
	struct nbl_index_key_extra extra_key;

	spin_lock(&mgt->counter_lock);
	NBL_INDEX_EXTRA_KEY_INIT(&extra_key, 0, 0, true);
	for (i = 0; i < NBL_FC_TYPE_MAX; i++) {
		idx = nbl_common_get_index_with_data(mgt->cls_cookie_tbl[i],  &cookie, &extra_key,
						     NULL, 0, (void **)&counter_node);
		if (idx != U32_MAX)
			break;
	}

	if (!counter_node) {
		nbl_debug(mgt->common, NBL_DEBUG_FLOW, "nbl flow fc cookie %lu is not exist now",
			  cookie);
		ret = -ENOKEY;
		goto counter_rte_hash_lookup_err;
	}

	if (data_info->packets != counter_node->cache.packets) {
		counter_node->cache.packets = data_info->packets;
		counter_node->cache.bytes = data_info->bytes;
		counter_node->lastuse = jiffies;
	}

	spin_unlock(&mgt->counter_lock);
	return 0;

counter_rte_hash_lookup_err:
	spin_unlock(&mgt->counter_lock);
	return ret;
}

int nbl_fc_setup_mgt(struct device *dev, struct nbl_fc_mgt **fc_mgt)
{
	struct nbl_fc_mgt *mgt;
	*fc_mgt = devm_kzalloc(dev, sizeof(struct nbl_fc_mgt), GFP_KERNEL);

	mgt = *fc_mgt;
	if (!mgt)
		return -ENOMEM;

	spin_lock_init(&mgt->counter_lock);
	INIT_LIST_HEAD(&mgt->counter_hash_list);
	INIT_LIST_HEAD(&mgt->counter_stat_hash_list);
	mgt->query_interval = NBL_COUNTER_PERIOD_INTERVAL;

	return 0;
}

int nbl_fc_add_stats(void *priv, enum nbl_pp_fc_type fc_type, unsigned long cookie)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_fc_mgt *fc_mgt;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_flow_counter *counter_node;
	struct list_head *counter_list;
	int ret = 0;
	int idx = 0;
	struct nbl_flow_counter counter_data;
	struct nbl_index_key_extra extra_key;

	fc_mgt = NBL_RES_MGT_TO_COUNTER_MGT(res_mgt);
	if (!fc_mgt) {
		nbl_info(common, NBL_DEBUG_FLOW, "nbl flow fc add failed: counter not init.");
		return -EINVAL;
	}

	spin_lock(&fc_mgt->counter_lock);
	if (fc_type == NBL_FC_COMMON_TYPE)
		counter_list = &fc_mgt->counter_hash_list;
	else
		counter_list = &fc_mgt->counter_stat_hash_list;

	NBL_INDEX_EXTRA_KEY_INIT(&extra_key, 0, 0, true);
	idx = nbl_common_get_index_with_data(fc_mgt->cls_cookie_tbl[fc_type], &cookie, &extra_key,
					     NULL, 0, (void **)&counter_node);
	if (idx != U32_MAX) {
		nbl_err(common, NBL_DEBUG_FLOW, "nbl flow fc add failed: cookie exist(%lu-%d)\n",
			cookie, fc_type);
		ret = -EEXIST;
		goto add_counter_failed;
	}

	memset(&counter_data, 0, sizeof(counter_data));
	counter_data.cookie = cookie;
	idx = nbl_common_alloc_index(fc_mgt->cls_cookie_tbl[fc_type], &cookie, NULL, &counter_data,
				     sizeof(counter_data), (void **)&counter_node);
	if (idx == U32_MAX)
		goto add_counter_failed;

	counter_node->counter_id = (u32)idx;
	list_add(&counter_node->entries, counter_list);

	/* wake up update worker */
	mod_delayed_work(fc_mgt->counter_wq, &fc_mgt->counter_work, 0);
	nbl_debug(common, NBL_DEBUG_FLOW, "nbl flow fc add counter(%u-%lu-%d) success\n",
		  idx, cookie, fc_type);
	ret = (int)idx;

add_counter_failed:
	spin_unlock(&fc_mgt->counter_lock);
	return ret;
}

int nbl_fc_del_stats(void *priv, unsigned long cookie)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_fc_mgt *fc_mgt;
	struct nbl_flow_counter *counter_node = NULL;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct list_head *counter_list;
	int ret = 0;
	int idx;
	int i;
	struct nbl_flow_query_counter counter_array;
	struct nbl_index_key_extra extra_key;

	fc_mgt = NBL_RES_MGT_TO_COUNTER_MGT(res_mgt);
	if (!fc_mgt) {
		nbl_info(common, NBL_DEBUG_FLOW, "nbl flow fc del failed: counter not init.");
		return -EINVAL;
	}

	memset(&counter_array, 0, sizeof(counter_array));

	spin_lock(&fc_mgt->counter_lock);
	NBL_INDEX_EXTRA_KEY_INIT(&extra_key, 0, 0, true);
	for (i = 0; i < NBL_FC_TYPE_MAX; i++) {
		idx = nbl_common_get_index_with_data(fc_mgt->cls_cookie_tbl[i], &cookie,
						     &extra_key, NULL, 0, (void **)&counter_node);
		if (idx != U32_MAX)
			break;
	}

	if (!counter_node || i >= NBL_FC_TYPE_MAX) {
		nbl_debug(common, NBL_DEBUG_FLOW, "nbl flow fc del key(%lu) not exist", cookie);
		ret = -ENOKEY;
		goto del_counter_failed;
	}

	if (i == NBL_FC_COMMON_TYPE)
		counter_list = &fc_mgt->counter_hash_list;
	else
		counter_list = &fc_mgt->counter_stat_hash_list;

	counter_array.counter_id[0] = idx;
	counter_array.cookie[0] = cookie;
	fc_mgt->fc_ops.update_stats(fc_mgt, &counter_array, 1, 1, i);
	list_del(&counter_node->entries);
	nbl_common_free_index(fc_mgt->cls_cookie_tbl[i], &cookie);
	nbl_debug(common, NBL_DEBUG_FLOW, "nbl flow fc del counter(%lu-%d) success\n", cookie, i);
del_counter_failed:
	spin_unlock(&fc_mgt->counter_lock);
	return ret;
}

int nbl_fc_mgt_start(struct nbl_fc_mgt *mgt)
{
	int ret = -ENOMEM;

	mgt->counter_wq = alloc_workqueue("%s", WQ_MEM_RECLAIM | WQ_UNBOUND, 1, "nbl_fc_wq");
	if (!mgt->counter_wq)
		goto init_counter_fail;

	ret = nbl_fc_init_hash_map(mgt);
	if (ret)
		goto init_counter_fail;

	INIT_DELAYED_WORK(&mgt->counter_work, nbl_fc_stats_work);
	queue_delayed_work(mgt->counter_wq, &mgt->counter_work, mgt->query_interval);

	nbl_info(mgt->common, NBL_DEBUG_FLOW, "nbl flow fc init success in tc mode");
	return 0;

init_counter_fail:
	nbl_err(mgt->common, NBL_DEBUG_FLOW, "nbl flow fc init failed in tc mode");
	return ret;
}

void nbl_fc_mgt_stop(struct nbl_resource_mgt *res_mgt)
{
	struct device *dev;
	struct nbl_fc_mgt **fc_mgt;
	struct nbl_fc_mgt *mgt;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);

	dev = NBL_RES_MGT_TO_DEV(res_mgt);
	fc_mgt = &NBL_RES_MGT_TO_COUNTER_MGT(res_mgt);
	mgt = (*fc_mgt);
	if (!mgt)
		return;

	cancel_delayed_work_sync(&mgt->counter_work);
	destroy_workqueue(mgt->counter_wq);
	nbl_fc_free_res(mgt);
	nbl_fc_remove_mgt(dev, fc_mgt);
	nbl_info(common, NBL_DEBUG_FLOW, "nbl flow fc deinit success in tc mode");
}

int nbl_fc_setup_ops(struct nbl_resource_ops *res_ops)
{
#define NBL_COUNTER_SET_OPS(name, func) do {res_ops->NBL_NAME(name) = func; ; } while (0)
	NBL_COUNTER_OPS_TBL;
#undef  NBL_COUNTER_SET_OPS

	return 0;
}

void nbl_fc_remove_ops(struct nbl_resource_ops *res_ops)
{
#define NBL_COUNTER_SET_OPS(name, func) do {res_ops->NBL_NAME(name) = NULL; ; } while (0)
	NBL_COUNTER_OPS_TBL;
#undef  NBL_COUNTER_SET_OPS
}
