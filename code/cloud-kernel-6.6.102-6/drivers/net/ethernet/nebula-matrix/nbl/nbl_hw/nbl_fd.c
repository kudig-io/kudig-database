// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#include "nbl_fd.h"
#include "nbl_p4_actions.h"

struct nbl_fd_get_tlv_udf {
	u64 val;
	u64 mask;
	bool valid;
};

struct nbl_fd_tcam_default_entry {
	union nbl_fd_tcam_default_data_u data;
	union nbl_fd_tcam_default_data_u mask;
	struct nbl_flow_direct_entry *entry;
	u32 action;
};

static int nbl_fd_get_profile_id(enum nbl_chan_fdir_flow_type type, u8 mode)
{
	switch (type) {
	case NBL_CHAN_FDIR_FLOW_TCP_IPv4:
	case NBL_CHAN_FDIR_FLOW_UDP_IPv4:
	case NBL_CHAN_FDIR_FLOW_IPv4:
		return mode == NBL_FD_MODE_DEFAULT ? NBL_FD_PROFILE_DEFAULT : NBL_FD_PROFILE_IPV4;
	case NBL_CHAN_FDIR_FLOW_TCP_IPv6:
	case NBL_CHAN_FDIR_FLOW_UDP_IPv6:
	case NBL_CHAN_FDIR_FLOW_IPv6:
	case NBL_CHAN_FDIR_FLOW_ETHER:
	case NBL_CHAN_FDIR_FLOW_FULL:
		return mode == NBL_FD_MODE_DEFAULT ? NBL_FD_PROFILE_DEFAULT :
						     NBL_FD_PROFILE_L2_IPV6;
	default:
		break;
	};

	return -1;
}

static struct nbl_flow_direct_entry *nbl_fd_find_flow(struct nbl_flow_direct_info *info,
						      enum nbl_chan_fdir_rule_type rule_type,
						      u32 loc)
{
	struct nbl_flow_direct_entry *entry = NULL;

	if (rule_type >= NBL_CHAN_FDIR_RULE_MAX)
		return NULL;

	list_for_each_entry(entry, &info->list[rule_type], node)
		if (entry->param.location == loc)
			return entry;

	return NULL;
}

static int nbl_fd_get_udf(u16 type, u16 length, u8 *val, void *data)
{
	struct nbl_fd_get_tlv_udf *udf = (struct nbl_fd_get_tlv_udf *)data;

	if (type != NBL_CHAN_FDIR_KEY_UDF)
		return 0;

	udf->valid = 1;
	udf->val = *(u64 *)val;
	udf->mask = *(u64 *)(val + 8);

	return 1;
}

static u16 nbl_fd_get_flow_layer(enum nbl_chan_fdir_flow_type type)
{
	switch (type) {
	case NBL_CHAN_FDIR_FLOW_ETHER:
		return 0;
	case NBL_CHAN_FDIR_FLOW_IPv4:
	case NBL_CHAN_FDIR_FLOW_IPv6:
		return 1;
	case NBL_CHAN_FDIR_FLOW_TCP_IPv4:
	case NBL_CHAN_FDIR_FLOW_TCP_IPv6:
	case NBL_CHAN_FDIR_FLOW_UDP_IPv4:
	case NBL_CHAN_FDIR_FLOW_UDP_IPv6:
	case NBL_CHAN_FDIR_FLOW_FULL:
	default:
		return 2;
	}
}

static int nbl_fd_validate_rule(struct nbl_flow_direct_mgt *fd_mgt,
				struct nbl_chan_param_fdir_replace *param,
				struct nbl_flow_direct_entry *entry)
{
	struct nbl_fd_get_tlv_udf udf = {0};
	int pid = -1;
	u16 udf_offset;
	u16 udf_layer;
	bool rule_udf = false;
	u8 *tlv;

	if (param->rule_type >= NBL_CHAN_FDIR_RULE_MAX)
		return -EINVAL;

	tlv = (u8 *)param + param->base_length;
	nbl_flow_direct_parse_tlv_data(tlv, param->tlv_length, nbl_fd_get_udf, &udf);
	if (udf.valid) {
		udf_offset = (udf.val & NBL_FD_UDF_FLEX_OFFS_M) >> NBL_FD_UDF_FLEX_OFFS_S;
		udf_layer = nbl_fd_get_flow_layer(param->flow_type);

		if (entry)
			rule_udf = entry->udf;

		/* Offset must be the same for all rules */
		if (fd_mgt->udf_cnt > 0 &&
		    (fd_mgt->udf_offset != udf_offset || fd_mgt->udf_layer != udf_layer) &&
		    (fd_mgt->udf_cnt != 1 || !rule_udf))
			return -EINVAL;

		if (udf_offset > 52)
			return -EINVAL;

		/* For offset, we don't support mask */
		if (((udf.mask & NBL_FD_UDF_FLEX_OFFS_M) >> NBL_FD_UDF_FLEX_OFFS_S) != 0xFFFFFFFF)
			return -EINVAL;
	}

	/* replace rule not check cnt current, alway keep full mode */
	if (entry)
		return 0;

	pid = nbl_fd_get_profile_id(param->flow_type, fd_mgt->mode);
	switch (pid) {
	case NBL_FD_PROFILE_DEFAULT:
		if (fd_mgt->cnt[NBL_FD_PROFILE_DEFAULT] >= NBL_FD_RULE_MAX_512)
			return -EINVAL;
		break;
	case NBL_FD_PROFILE_IPV4:
		if (fd_mgt->mode == NBL_FD_MODE_LITE &&
		    fd_mgt->cnt[NBL_FD_PROFILE_IPV4] >= NBL_FD_RULE_MAX_1536)
			return -EINVAL;
		if (fd_mgt->mode == NBL_FD_MODE_FULL &&
		    fd_mgt->cnt[NBL_FD_PROFILE_IPV4] >= NBL_FD_RULE_MAX_512 &&
		    fd_mgt->cnt[NBL_FD_PROFILE_L2_IPV6] > 0)
			return -EINVAL;
		break;
	case NBL_FD_PROFILE_L2_IPV6:
		/* We will always try to change the mode to FULL, so if we are in LITE now,
		 * then don't support any IPV6 rules whatsoever.
		 */
		if (fd_mgt->mode == NBL_FD_MODE_LITE ||
		    fd_mgt->cnt[NBL_FD_PROFILE_L2_IPV6] >= NBL_FD_RULE_MAX_512)
			return -EINVAL;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct nbl_flow_direct_entry *nbl_fd_add_flow(struct nbl_flow_direct_mgt *fd_mgt,
						     struct nbl_flow_direct_info *info,
						     struct nbl_chan_param_fdir_replace *param)
{
	struct nbl_flow_direct_entry *entry = NULL, *next = NULL;
	struct nbl_fd_get_tlv_udf udf = {0};
	u8 pid;

	pid = nbl_fd_get_profile_id(param->flow_type, fd_mgt->mode);
	if (pid > NBL_FD_PROFILE_MAX)
		return NULL;

	entry = kzalloc(sizeof(*entry) + param->tlv_length, GFP_KERNEL);
	if (!entry)
		return NULL;

	entry->pid = pid;
	memcpy(&entry->param, param, min_t(u32, sizeof(entry->param), param->base_length));
	memcpy(entry->param.tlv, ((u8 *)param + param->base_length), param->tlv_length);
	entry->param.base_length = sizeof(entry->param);

	/* Maintain order */
	if (param->order) {
		list_for_each_entry(next, &info->list[param->rule_type], node)
			if (next->param.location >= entry->param.location)
				break;

		if (list_entry_is_head(next, &info->list[param->rule_type], node))
			list_add(&entry->node, &info->list[param->rule_type]);
		else
			list_add(&entry->node, &list_prev_entry(next, node)->node);
	} else {
		list_add_tail(&entry->node, &info->list[param->rule_type]);
	}

	info->cnt[param->rule_type]++;
	fd_mgt->cnt[entry->pid]++;

	/* We have judged the capacity in validation, so we shouldn't have any trouble now. */
	if (fd_mgt->mode == NBL_FD_MODE_FULL &&
	    fd_mgt->cnt[NBL_FD_PROFILE_IPV4] > NBL_FD_RULE_MAX_512)
		fd_mgt->mode = NBL_FD_MODE_LITE;

	nbl_flow_direct_parse_tlv_data(param->tlv, param->tlv_length, nbl_fd_get_udf, &udf);
	if (udf.valid) {
		entry->udf = 1;
		fd_mgt->udf_offset = (udf.val & NBL_FD_UDF_FLEX_OFFS_M) >> NBL_FD_UDF_FLEX_OFFS_S;
		fd_mgt->udf_cnt++;
		fd_mgt->udf_layer = nbl_fd_get_flow_layer(param->flow_type);
	}

	return entry;
}

static void nbl_fd_del_flow(struct nbl_flow_direct_mgt *fd_mgt,
			    struct nbl_flow_direct_info *info,
			    struct nbl_flow_direct_entry *entry)
{
	info->cnt[entry->param.rule_type]--;
	fd_mgt->cnt[entry->pid]--;

	if (entry->udf)
		fd_mgt->udf_cnt--;

	if (fd_mgt->mode == NBL_FD_MODE_LITE &&
	    fd_mgt->cnt[NBL_FD_PROFILE_IPV4] <= NBL_FD_RULE_MAX_512)
		fd_mgt->mode = NBL_FD_MODE_FULL;

	list_del(&entry->node);
	kfree(entry);
}

static int nbl_fd_find_and_del_flow(struct nbl_flow_direct_mgt *fd_mgt,
				    struct nbl_flow_direct_info *info,
				    enum nbl_chan_fdir_rule_type rule_type,
				    u32 loc)
{
	struct nbl_flow_direct_entry *entry = nbl_fd_find_flow(info, rule_type, loc);

	if (!entry)
		return -ENOENT;

	nbl_fd_del_flow(fd_mgt, info, entry);

	return 0;
}

static void nbl_fd_del_flow_all(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	struct nbl_flow_direct_entry *entry = NULL, *entry_safe = NULL;
	int i = 0, j;

	for (i = 0; i < NBL_RES_MGT_TO_PF_NUM(res_mgt); i++)
		for (j = 0; j < NBL_CHAN_FDIR_RULE_MAX; j++)
			list_for_each_entry_safe(entry, entry_safe, &fd_mgt->info[i].list[j], node)
				nbl_fd_del_flow(fd_mgt, &fd_mgt->info[i], entry);
}

static int nbl_fd_setup_tcam_cfg(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	switch (fd_mgt->mode) {
	case NBL_FD_MODE_DEFAULT:
		phy_ops->set_fd_tcam_cfg_default(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
		break;
	case NBL_FD_MODE_FULL:
		phy_ops->set_fd_tcam_cfg_full(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
		break;
	case NBL_FD_MODE_LITE:
		phy_ops->set_fd_tcam_cfg_lite(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
		break;
	default:
		return -EINVAL;
	}

	if (fd_mgt->udf_cnt)
		phy_ops->set_fd_udf(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				    (u8)fd_mgt->udf_layer,
				    (u8)fd_mgt->udf_offset);
	else
		phy_ops->clear_fd_udf(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));

	return 0;
}

static int nbl_fd_config_default_key_action(u16 type, u16 length, u8 *val, void *data)
{
	struct nbl_fd_tcam_default_entry *tcam_entry = (struct nbl_fd_tcam_default_entry *)data;
	union nbl_fd_tcam_default_data_u *tcam_data = &tcam_entry->data;
	union nbl_fd_tcam_default_data_u *tcam_mask = &tcam_entry->mask;
	struct nbl_flow_direct_entry *entry = tcam_entry->entry;
	union nbl_action_data action = {{0}};
	u8 reverse_mac[ETH_ALEN];
	u64 temp, mask;
	u32 offset, udf_data, udf_mask;

	switch (type) {
	case NBL_CHAN_FDIR_KEY_SRC_MAC:
		nbl_convert_mac(val, reverse_mac);
		ether_addr_copy((u8 *)&temp, reverse_mac);
		tcam_data->info.src_mac = temp;
		nbl_convert_mac(val + ETH_ALEN, reverse_mac);
		ether_addr_copy((u8 *)&temp, reverse_mac);
		tcam_mask->info.src_mac = temp;
		break;
	case NBL_CHAN_FDIR_KEY_DST_MAC:
		nbl_convert_mac(val, reverse_mac);
		ether_addr_copy((u8 *)&temp, reverse_mac);
		tcam_data->info.dst_mac = temp;
		nbl_convert_mac(val + ETH_ALEN, reverse_mac);
		ether_addr_copy((u8 *)&temp, reverse_mac);
		tcam_mask->info.dst_mac = temp;
		break;
	case NBL_CHAN_FDIR_KEY_PROTO:
		tcam_data->info.ethertype = be16_to_cpu(*(u16 *)val);
		tcam_mask->info.ethertype = be16_to_cpu(*(u16 *)(val + 2));
		break;
	case NBL_CHAN_FDIR_KEY_SRC_IPv4:
		tcam_data->info.sip_l = be32_to_cpu(*(u32 *)val);
		tcam_mask->info.sip_l = be32_to_cpu(*(u32 *)(val + 4));
		break;
	case NBL_CHAN_FDIR_KEY_DST_IPv4:
		tcam_data->info.dip_l = be32_to_cpu(*(u32 *)val);
		tcam_mask->info.dip_l = be32_to_cpu(*(u32 *)(val + 4));
		break;
	case NBL_CHAN_FDIR_KEY_L4PROTO:
		tcam_data->info.l4_proto = *val;
		tcam_mask->info.l4_proto = *(val + 1);
		break;
	case NBL_CHAN_FDIR_KEY_SRC_IPv6:
		tcam_data->info.sip_l = be64_to_cpu(*((u64 *)val + 1));
		tcam_mask->info.sip_l = be64_to_cpu(*((u64 *)val + 3));
		tcam_data->info.sip_h = be64_to_cpu(*(u64 *)val);
		tcam_mask->info.sip_h = be64_to_cpu(*(u64 *)val + 2);
		break;
	case NBL_CHAN_FDIR_KEY_DST_IPv6:
		tcam_data->info.dip_l = be64_to_cpu(*((u64 *)val + 1));
		tcam_mask->info.dip_l = be64_to_cpu(*((u64 *)val + 3));
		tcam_data->info.dip_h = be64_to_cpu(*(u64 *)val);
		tcam_mask->info.dip_h = be64_to_cpu(*(u64 *)val + 2);
		break;
	case NBL_CHAN_FDIR_KEY_SPORT:
		/* hw generate key is little endian */
		tcam_data->info.l4_sport = be16_to_cpu(*(u16 *)val);
		tcam_mask->info.l4_sport = be16_to_cpu(*(u16 *)(val + 2));
		break;
	case NBL_CHAN_FDIR_KEY_DPORT:
		tcam_data->info.l4_dport = be16_to_cpu(*(u16 *)val);
		tcam_mask->info.l4_dport = be16_to_cpu(*(u16 *)(val + 2));
		break;
	case NBL_CHAN_FDIR_KEY_UDF:
		temp = *(u64 *)val;
		mask = *(u64 *)(val + 8);
		offset = (temp & NBL_FD_UDF_FLEX_OFFS_M) >> NBL_FD_UDF_FLEX_OFFS_S;
		udf_data = temp & NBL_FD_UDF_FLEX_WORD_M;
		udf_mask = mask & NBL_FD_UDF_FLEX_WORD_M;

		/* data: high addr means payload first bytes. */
		if (offset % 4 == 1) {
			udf_data = (u8)udf_data << 24 | udf_data >> 8;
			udf_mask = (u8)udf_mask << 24 | udf_mask >> 8;

		} else if (offset % 4 == 3) {
			udf_data = udf_data >> 24 | udf_data << 8;
			udf_mask = udf_mask >> 24 | udf_mask << 8;
		}

		tcam_data->info.udf = udf_data;
		tcam_mask->info.udf = udf_mask;
		break;
	case NBL_CHAN_FDIR_ACTION_QUEUE:
		if (entry->param.global_queue_id != 0xFFFF) {
			action.dqueue.que_id = entry->param.global_queue_id;
			tcam_entry->action = action.data + (NBL_ACT_SET_QUE_IDX << 16);
		} else {
			action.data = 0xFFF;
			tcam_entry->action = action.data + (NBL_ACT_SET_DPORT << 16);
		}
		break;
	case NBL_CHAN_FDIR_ACTION_VSI:
		if (entry->param.dport != 0xFFFF) {
			action.dport.up.port_type = SET_DPORT_TYPE_VSI_HOST;
			action.dport.up.port_id = entry->param.dport;
			action.dport.up.upcall_flag = AUX_KEEP_FWD_TYPE;
			action.dport.up.next_stg_sel = NEXT_STG_SEL_EPRO;
		} else {
			action.data = 0xFFF;
		}
		tcam_entry->action = action.data + (NBL_ACT_SET_DPORT << 16);

		break;
	default:
		break;
	}

	return 0;
}

static int nbl_fd_config_key(struct nbl_flow_direct_entry *entry, struct nbl_acl_tcam_param *data,
			     struct nbl_acl_tcam_param *mask, u32 *action, u16 vsi_id)
{
	struct nbl_fd_tcam_default_entry tcam_default_entry;

	memset(&tcam_default_entry, 0, sizeof(tcam_default_entry));
	tcam_default_entry.entry = entry;

	switch (entry->pid) {
	case NBL_FD_PROFILE_DEFAULT:
		nbl_flow_direct_parse_tlv_data(entry->param.tlv, entry->param.tlv_length,
					       nbl_fd_config_default_key_action,
					       &tcam_default_entry);

		tcam_default_entry.data.info.dport = (0x2 << 10) + vsi_id;
		tcam_default_entry.mask.info.dport = 0xFFFF;
		tcam_default_entry.data.info.pid = NBL_FD_PROFILE_DEFAULT;
		tcam_default_entry.mask.info.pid = 0xE;

		memcpy(&data->info.data, &tcam_default_entry.data, sizeof(tcam_default_entry.data));
		memcpy(&mask->info.data, &tcam_default_entry.mask, sizeof(tcam_default_entry.mask));
		data->len = sizeof(tcam_default_entry.data);
		mask->len = sizeof(tcam_default_entry.mask);
		*action = tcam_default_entry.action;

		break;
	case NBL_FD_PROFILE_IPV4:
	case NBL_FD_PROFILE_L2_IPV6:
	default:
		return -EINVAL;
	}

	return 0;
}

static int nbl_fd_get_tcam_index(struct nbl_fd_tcam_index_info *info, u8 pid,
				 u16 *ram_index, u16 *depth_index, int mode)
{
	switch (pid) {
	case NBL_FD_PROFILE_DEFAULT:
		if (info->default_index[0].depth_index >= NBL_FD_TCAM_DEPTH)
			return -EINVAL;

		*ram_index = 0;
		*depth_index = info->default_index[0].depth_index++;

		break;
	case NBL_FD_PROFILE_IPV4:
		if (mode != NBL_FD_MODE_LITE &&
		    (info->v4_cnt > 1 || info->v4[0].depth_index >= NBL_FD_TCAM_DEPTH))
			return -EINVAL;

		if (info->v4[info->v4_cnt].depth_index < NBL_FD_TCAM_DEPTH) {
			*ram_index = info->v4_cnt;
			*depth_index = info->v4[info->v4_cnt].depth_index++;
		} else {
			*ram_index = info->v4_cnt++;
			*depth_index = info->v4[info->v4_cnt].depth_index++;
		}

		break;
	case NBL_FD_PROFILE_L2_IPV6:
		if (mode == NBL_FD_MODE_LITE || info->v6[0].depth_index >= NBL_FD_TCAM_DEPTH)
			return -EINVAL;

		*ram_index = NBL_FD_IPV4_TCAM_WIDTH;
		*depth_index = info->v6[0].depth_index++;

		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static u16 nbl_fd_get_action_index(u16 ram_index)
{
	/* This is a bit tricky...
	 *
	 * For DEFAULT mode, ram_index is always 0, so we always use action_ram 0.
	 *
	 * For FULL mode, IPV4 rules always have ram_index 0, so they use action_ram 0, and
	 * IPV6 rules always have ram_index equals to NBL_FD_IPV4_TCAM_WIDTH, so they use
	 * action_ram 1.
	 *
	 * For LITE mode, every 512 IPV4 rules use one action_ram.
	 */
	return ram_index / NBL_FD_IPV4_TCAM_WIDTH;
}

static int nbl_fd_setup_tcam_for_list(struct nbl_resource_mgt *res_mgt,
				      struct nbl_fd_tcam_index_info *index_info,
				      struct list_head *head, u16 vsi_id)
{
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	struct nbl_acl_tcam_param data, mask;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_flow_direct_entry *entry = NULL;
	u16 ram_index = 0, depth_index = 0, action_index = 0;
	u32 action = 0;
	int ret;

	memset(&data, 0, sizeof(data));
	memset(&mask, 0, sizeof(mask));
	list_for_each_entry(entry, head, node) {
		ret = nbl_fd_get_tcam_index(index_info, entry->pid, &ram_index,
					    &depth_index, fd_mgt->mode);
		if (ret)
			return ret;

		nbl_fd_config_key(entry, &data, &mask, &action, vsi_id);
		action_index = nbl_fd_get_action_index(ram_index);

		entry->action_index = action_index;
		entry->depth_index = depth_index;
		ret = phy_ops->set_fd_action_ram(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						 action, action_index, depth_index);

		ret = phy_ops->set_fd_tcam_ram(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
						&data, &mask, ram_index, depth_index);
		if (ret)
			return ret;
	}

	return 0;
}

static int nbl_fd_setup_tcam(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	struct nbl_fd_tcam_index_info index_info;
	u16 vsi_id = 0;
	int i = 0, j, ret = 0;

	memset(&index_info, 0, sizeof(index_info));

	for (i = 0; i < NBL_RES_MGT_TO_PF_NUM(res_mgt); i++) {
		vsi_id = nbl_res_pfvfid_to_vsi_id(res_mgt, i, -1, NBL_VSI_DATA);
		for (j = 0; j < NBL_CHAN_FDIR_RULE_MAX; j++) {
			ret = nbl_fd_setup_tcam_for_list(res_mgt, &index_info,
							 &fd_mgt->info[i].list[j], vsi_id);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int nbl_fd_setup_flow(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	int ret = 0;

	if (fd_mgt->state != NBL_FD_STATE_ON)
		return 0;

	phy_ops->clear_acl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));

	ret = nbl_fd_setup_tcam_cfg(res_mgt);
	if (ret)
		goto fail;

	ret = nbl_fd_setup_tcam(res_mgt);
	if (ret)
		goto fail;

	return 0;

fail:
	phy_ops->clear_acl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
	return ret;
}

static void nbl_fd_remove_flow(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->clear_acl(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
}

static int nbl_fd_handle_queue_update(u16 type, void *event_data, void *callback_data)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)callback_data;
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	struct nbl_event_queue_update_data *data =
		(struct nbl_event_queue_update_data *)event_data;
	struct nbl_flow_direct_entry *entry = NULL;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	union nbl_action_data action = {{0}};
	int pf_id, vf_id;
	u32 action_data;
	u16 func_id = data->func_id;

	nbl_res_func_id_to_pfvfid(res_mgt, func_id, &pf_id, &vf_id);

	if (pf_id < 0 || pf_id >= NBL_MAX_PF)
		return 0;

	vf_id = vf_id + 1;
	list_for_each_entry(entry, &fd_mgt->info[pf_id].list[NBL_CHAN_FDIR_RULE_NORMAL], node) {
		if (entry->param.vf != vf_id)
			continue;

		if (entry->param.ring < data->ring_num) {
			entry->param.global_queue_id = data->map[entry->param.ring];
			action.dqueue.que_id = entry->param.global_queue_id;
			action_data = action.data + (NBL_ACT_SET_QUE_IDX << 16);
		} else {
			entry->param.global_queue_id = 0xFFFF;
			action.dport.up.port_type = SET_DPORT_TYPE_VSI_HOST;
			action.dport.up.port_id = 0x3FF;
			action.dport.up.upcall_flag = AUX_KEEP_FWD_TYPE;
			action.dport.up.next_stg_sel = NEXT_STG_SEL_EPRO;
			action_data = action.data + (NBL_ACT_SET_DPORT << 16);
		}

		phy_ops->set_fd_action_ram(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					   action_data, entry->action_index, entry->depth_index);
	}

	return 0;
}

static int nbl_fd_handle_state_update(u16 type, void *event_data, void *callback_data)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)callback_data;
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	struct nbl_event_acl_state_update_data *data =
		(struct nbl_event_acl_state_update_data *)event_data;

	if (fd_mgt->state == NBL_FD_STATE_OFF && !data->is_offload) {
		fd_mgt->state = NBL_FD_STATE_ON;
		nbl_fd_setup_flow(res_mgt);
	} else if (fd_mgt->state == NBL_FD_STATE_ON && data->is_offload) {
		nbl_fd_remove_flow(res_mgt);
		fd_mgt->state = NBL_FD_STATE_OFF;
	}

	return 0;
}

/* ---------  Res-layer ops Fucntions  --------- */

static int nbl_fd_get_fd_flow_cnt(void *priv, enum nbl_chan_fdir_rule_type rule_type, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	int pf_id = nbl_res_vsi_id_to_pf_id(res_mgt, vsi_id);

	if (pf_id < 0 || pf_id >= NBL_MAX_PF)
		return -EINVAL;

	if (rule_type >= NBL_CHAN_FDIR_RULE_MAX)
		return -EINVAL;

	return fd_mgt->info[pf_id].cnt[rule_type];
}

static int nbl_fd_get_fd_flow_all(void *priv, struct nbl_chan_param_get_fd_flow_all *param,
				  u32 *rule_locs)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	struct nbl_flow_direct_entry *entry = NULL;
	int pf_id = nbl_res_vsi_id_to_pf_id(res_mgt, param->vsi_id), index = 0;

	if (pf_id < 0 || pf_id >= NBL_MAX_PF)
		return -EINVAL;

	if (param->rule_type >= NBL_CHAN_FDIR_RULE_MAX)
		return -EINVAL;

	list_for_each_entry(entry, &fd_mgt->info[pf_id].list[param->rule_type], node) {
		if (index < param->start)
			continue;

		if (index >= param->start + param->num)
			break;

		rule_locs[index++] = entry->param.location;
	}

	return 0;
}

static int nbl_fd_get_fd_flow_max(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);

	return fd_mgt->max_spec;
}

static int nbl_fd_config_fd_flow_state(void *priv, enum nbl_chan_fdir_rule_type rule_type,
				       u16 vsi_id, u16 state)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	struct nbl_flow_direct_entry *entry = NULL, *entry_safe = NULL;
	int pf_id;

	pf_id = nbl_res_vsi_id_to_pf_id(res_mgt, vsi_id);
	if (pf_id < 0 || pf_id >= NBL_MAX_PF)
		return -EINVAL;

	if (rule_type >= NBL_CHAN_FDIR_RULE_MAX)
		return -EINVAL;

	if (state == NBL_FD_STATE_OFF || state == NBL_FD_STATE_FLUSH) {
		list_for_each_entry_safe(entry, entry_safe,
					 &fd_mgt->info[pf_id].list[rule_type], node)
			nbl_fd_del_flow(fd_mgt, &fd_mgt->info[pf_id], entry);
		nbl_fd_setup_flow(res_mgt);
	}
	if (state != NBL_FD_STATE_FLUSH)
		fd_mgt->info[pf_id].state[rule_type] = state;

	return 0;
}

static int nbl_fd_get_fd_flow(void *priv, u16 vsi_id, u32 location,
			      enum nbl_chan_fdir_rule_type rule_type,
			      struct nbl_chan_param_fdir_replace *cmd)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	struct nbl_flow_direct_entry *entry = NULL;
	int pf_id = nbl_res_vsi_id_to_pf_id(res_mgt, vsi_id);

	if (location >= fd_mgt->max_spec || pf_id < 0 || pf_id >= NBL_MAX_PF)
		return -EINVAL;

	entry = nbl_fd_find_flow(&fd_mgt->info[pf_id], rule_type, location);
	if (!entry)
		return -ENOENT;

	memcpy(cmd, &entry->param, sizeof(*cmd) + entry->param.tlv_length);
	return 0;
}

static int nbl_fd_replace_fd_flow(void *priv, struct nbl_chan_param_fdir_replace *param)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	struct nbl_flow_direct_info *info = NULL;
	struct nbl_flow_direct_entry *entry = NULL;
	int pf_id = nbl_res_vsi_id_to_pf_id(res_mgt, param->vsi), ret = 0;

	if (pf_id < 0 || pf_id >= NBL_MAX_PF || param->location >= fd_mgt->max_spec)
		return -EINVAL;

	if (param->rule_type == NBL_CHAN_FDIR_RULE_NORMAL &&
	    fd_mgt->info[pf_id].state[param->rule_type] == NBL_FD_STATE_OFF)
		return -EINVAL;

	info = &fd_mgt->info[pf_id];
	entry = nbl_fd_find_flow(info, param->rule_type, param->location);
	ret = nbl_fd_validate_rule(fd_mgt, param, entry);
	if (ret)
		return ret;

	if (entry)
		nbl_fd_del_flow(fd_mgt, info, entry);

	entry = nbl_fd_add_flow(fd_mgt, info, param);
	if (!entry)
		goto add_entry_fail;

	ret = nbl_fd_setup_flow(res_mgt);
	if (ret)
		goto setup_flow_fail;

	return 0;

setup_flow_fail:
	nbl_fd_find_and_del_flow(fd_mgt, info, param->rule_type, param->location);
add_entry_fail:
	return ret;
}

static int nbl_fd_remove_fd_flow(void *priv, enum nbl_chan_fdir_rule_type rule_type,
				 u32 loc, u16 vsi_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	struct nbl_flow_direct_info *info = NULL;
	int pf_id = nbl_res_vsi_id_to_pf_id(res_mgt, vsi_id);
	int ret;

	if (pf_id < 0 || pf_id >= NBL_MAX_PF || loc >= fd_mgt->max_spec)
		return -EINVAL;

	info = &fd_mgt->info[pf_id];
	ret = nbl_fd_find_and_del_flow(fd_mgt, info, rule_type, loc);
	if (ret)
		return ret;

	return nbl_fd_setup_flow(res_mgt);
}

static void nbl_fd_cfg_update_event(void *priv, bool enable)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_event_callback event_callback = {0};

	event_callback.callback_data = res_mgt;

	if (enable) {
		event_callback.callback = nbl_fd_handle_state_update;
		nbl_event_register(NBL_EVENT_ACL_STATE_UPDATE, &event_callback,
				   NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));
		event_callback.callback = nbl_fd_handle_queue_update;
		nbl_event_register(NBL_EVENT_QUEUE_ALLOC, &event_callback,
				   NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));
	} else {
		event_callback.callback = nbl_fd_handle_state_update;
		nbl_event_unregister(NBL_EVENT_ACL_STATE_UPDATE, &event_callback,
				     NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));
		event_callback.callback = nbl_fd_handle_queue_update;
		nbl_event_unregister(NBL_EVENT_QUEUE_ALLOC, &event_callback,
				     NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));
	}
}

char flow_type_name[NBL_CHAN_FDIR_FLOW_MAX_TYPE][16] = {
	"Full/Isolate",
	"ETHER",
	"IPV4",
	"IPV6",
	"TCP_V4",
	"TCP_V6",
	"UDP_V4",
	"UDP_V6",
};

char mode_name[NBL_FD_MODE_MAX][16] = {
	"DEFAULT",
	"FULL",
	"LITE",
};

static int nbl_fd_dump_entry_tlv(u16 type, u16 length, u8 *val, void *data)
{
	struct seq_file *m = (struct seq_file *)(data);

	switch (type) {
	case NBL_CHAN_FDIR_KEY_SRC_MAC:
		seq_printf(m, "\tCompo [ SRC-MAC ]: data %02x-%02x-%02x-%02x-%02x-%02x, mask %02x-%02x-%02x-%02x-%02x-%02x\n",
			   val[0], val[1], val[2], val[3], val[4], val[5],
			   val[6], val[7], val[8], val[9], val[10], val[11]);
		break;
	case NBL_CHAN_FDIR_KEY_DST_MAC:
		seq_printf(m, "\tCompo [ DST-MAC ]: data %02x-%02x-%02x-%02x-%02x-%02x, mask %02x-%02x-%02x-%02x-%02x-%02x\n",
			   val[0], val[1], val[2], val[3], val[4], val[5],
			   val[6], val[7], val[8], val[9], val[10], val[11]);
		break;
	case NBL_CHAN_FDIR_KEY_PROTO:
		seq_printf(m, "\tCompo [ ETHERTYPE ]: data 0x%04x, mask 0x%04x\n",
			   *(u16 *)val, *(u16 *)(val + 2));
		break;
	case NBL_CHAN_FDIR_KEY_SRC_IPv4:
		seq_printf(m, "\tCompo [ SRC-IPV4 ]: data %pI4, mask %pI4\n",
			   (u32 *)val, (u32 *)(val + 4));
		break;
	case NBL_CHAN_FDIR_KEY_DST_IPv4:
		seq_printf(m, "\tCompo [ DST-IPV4 ]: data %pI4, mask %pI4\n",
			   (u32 *)val, (u32 *)(val + 4));
		break;
	case NBL_CHAN_FDIR_KEY_L4PROTO:
		seq_printf(m, "\tCompo [ IPPROTO ]: data 0x%x, mask 0x%x\n",
			   *(u8 *)val, *(u8 *)(val + 1));
		break;
	case NBL_CHAN_FDIR_KEY_SRC_IPv6:
		seq_printf(m, "\tCompo [SRC-IPV6 ]: data %pI6, mask %pI6\n",
			   val, val + 12);
		break;
	case NBL_CHAN_FDIR_KEY_DST_IPv6:
		seq_printf(m, "\tCompo [DST-IPV6 ]: data %pI6, mask %pI6\n",
			   val, val + 12);
		break;
	case NBL_CHAN_FDIR_KEY_SPORT:
		seq_printf(m, "\tCompo [ L4-SPORT ]: data 0x%x, mask 0x%x\n",
			   *(u16 *)val, *(u16 *)(val + 2));
		break;
	case NBL_CHAN_FDIR_KEY_DPORT:
		seq_printf(m, "\tCompo [ L4-DPORT ]: data 0x%x, mask 0x%x\n",
			   *(u16 *)val, *(u16 *)(val + 2));
		break;
	case NBL_CHAN_FDIR_KEY_UDF:
		seq_printf(m, "\tCompo [ USER-DEF ]: data 0x%llx, mask 0x%llx\n",
			   *(u64 *)val, *(u64 *)(val + 8));
		break;
	case NBL_CHAN_FDIR_ACTION_QUEUE:
		seq_printf(m, "\tCompo [ GLOBAL-QUE ]: data 0x%llx\n", *(u64 *)val);
		break;
	case NBL_CHAN_FDIR_ACTION_VSI:
		seq_printf(m, "\tCompo [ VSI ]: vsi 0x%llx\n", *(u64 *)val);
		break;
	default:
		break;
	}

	return 0;
}

static void nbl_fd_dump_entry(struct seq_file *m, struct nbl_flow_direct_entry *entry)
{
	struct nbl_chan_param_fdir_replace *param = &entry->param;

	seq_printf(m, "\n[ %-10s]: pid %d, location %4d, global queue id %4u\n",
		   flow_type_name[param->flow_type], entry->pid,
		   param->location, param->global_queue_id);

	nbl_flow_direct_parse_tlv_data(entry->param.tlv, entry->param.tlv_length,
				       nbl_fd_dump_entry_tlv, m);
}

static void nbl_fd_dump_flow(void *priv, struct seq_file *m)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_flow_direct_mgt *fd_mgt = NBL_RES_MGT_TO_FD_MGT(res_mgt);
	struct nbl_flow_direct_entry *entry = NULL;
	int i, j;

	seq_puts(m, "\n/* -----------------------  Flow Direct  ----------------------- */\n\n");

	seq_printf(m, "[STATE\t\t %-4s\t]\n[MODE\t\t %-4s\t]\n[DEFAULT_CNT\t %-4d]\n[IPV4_CNT\t %-4d\t]\n[L2&IPV6_CNT\t %-4d\t]\n[UDF cnt/layer/offset:\t %-4d %-4d %-4d\t]\n",
		   fd_mgt->state == NBL_FD_STATE_OFF ? "OFF" : "ON", mode_name[fd_mgt->mode],
		   fd_mgt->cnt[NBL_FD_PROFILE_DEFAULT], fd_mgt->cnt[NBL_FD_PROFILE_IPV4],
		   fd_mgt->cnt[NBL_FD_PROFILE_L2_IPV6], fd_mgt->udf_cnt, fd_mgt->udf_layer,
		   fd_mgt->udf_offset);

	for (i = 0; i < NBL_RES_MGT_TO_PF_NUM(res_mgt); i++) {
		for (j = 0; j < NBL_CHAN_FDIR_RULE_MAX; j++) {
			seq_printf(m, "\nPF %d/%d: %d flows state %-4s -------------------\n",
				   i, j, fd_mgt->info[i].cnt[j],
				   fd_mgt->info[i].state[j] == NBL_FD_STATE_OFF ? "OFF" : "ON");

			list_for_each_entry(entry, &fd_mgt->info[i].list[j], node)
				nbl_fd_dump_entry(m, entry);
		}
	}

	seq_puts(m, "\n");
}

/* NBL_FD_SET_OPS(ops_name, func)
 *
 * Use X Macros to reduce setup and remove codes.
 */
#define NBL_FD_OPS_TBL									\
do {											\
	NBL_FD_SET_OPS(get_fd_flow, nbl_fd_get_fd_flow);				\
	NBL_FD_SET_OPS(get_fd_flow_cnt, nbl_fd_get_fd_flow_cnt);			\
	NBL_FD_SET_OPS(get_fd_flow_all, nbl_fd_get_fd_flow_all);			\
	NBL_FD_SET_OPS(get_fd_flow_max, nbl_fd_get_fd_flow_max);			\
	NBL_FD_SET_OPS(config_fd_flow_state, nbl_fd_config_fd_flow_state);		\
	NBL_FD_SET_OPS(replace_fd_flow, nbl_fd_replace_fd_flow);			\
	NBL_FD_SET_OPS(remove_fd_flow, nbl_fd_remove_fd_flow);				\
	NBL_FD_SET_OPS(cfg_fd_update_event, nbl_fd_cfg_update_event);		\
	NBL_FD_SET_OPS(dump_fd_flow, nbl_fd_dump_flow);					\
} while (0)

/* Structure starts here, adding an op should not modify anything below */
static int nbl_fd_setup_mgt(struct device *dev, struct nbl_flow_direct_mgt **fd_mgt)
{
	int i, j;

	*fd_mgt = devm_kzalloc(dev, sizeof(struct nbl_flow_direct_mgt), GFP_KERNEL);
	if (!*fd_mgt)
		return -ENOMEM;

	for (i = 0; i < NBL_MAX_PF; i++) {
		for (j = 0; j < NBL_CHAN_FDIR_RULE_MAX; j++) {
			INIT_LIST_HEAD(&(*fd_mgt)->info[i].list[j]);
			(*fd_mgt)->info[i].state[j] = NBL_FD_STATE_OFF;
		}
	}

	(*fd_mgt)->udf_cnt = 0;
	(*fd_mgt)->udf_layer = 0;

	(*fd_mgt)->mode = NBL_FD_MODE_DEFAULT;
	(*fd_mgt)->max_spec = NBL_FD_RULE_MAX_DEFAULT;
	(*fd_mgt)->state = NBL_FD_STATE_ON;

	return 0;
}

static void nbl_fd_remove_mgt(struct device *dev, struct nbl_flow_direct_mgt **fd_mgt)
{
	devm_kfree(dev, *fd_mgt);
	*fd_mgt = NULL;
}

int nbl_fd_mgt_start(struct nbl_resource_mgt *res_mgt)
{
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct nbl_flow_direct_mgt **fd_mgt = &NBL_RES_MGT_TO_FD_MGT(res_mgt);
	int ret = 0;

	ret = nbl_fd_setup_mgt(dev, fd_mgt);

	return ret;
}

void nbl_fd_mgt_stop(struct nbl_resource_mgt *res_mgt)
{
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct nbl_flow_direct_mgt **fd_mgt = &NBL_RES_MGT_TO_FD_MGT(res_mgt);

	if (!(*fd_mgt))
		return;

	nbl_fd_remove_flow(res_mgt);
	nbl_fd_del_flow_all(res_mgt);
	nbl_fd_remove_mgt(dev, fd_mgt);
}

int nbl_fd_setup_ops(struct nbl_resource_ops *res_ops)
{
#define NBL_FD_SET_OPS(name, func) do {res_ops->NBL_NAME(name) = func; ; } while (0)
	NBL_FD_OPS_TBL;
#undef  NBL_FD_SET_OPS

	return 0;
}

void nbl_fd_remove_ops(struct nbl_resource_ops *res_ops)
{
#define NBL_FD_SET_OPS(name, func) do {res_ops->NBL_NAME(name) = NULL; ; } while (0)
	NBL_FD_OPS_TBL;
#undef  NBL_FD_SET_OPS
}
