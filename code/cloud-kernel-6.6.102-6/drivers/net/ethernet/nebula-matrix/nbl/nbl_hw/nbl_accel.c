// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */
#include "nbl_accel.h"

static int nbl_res_alloc_ktls_tx_index(void *priv, u16 vsi)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_accel_mgt *accel_mgt;
	u32 index;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	index = find_first_zero_bit(accel_mgt->tx_ktls_bitmap, NBL_MAX_KTLS_SESSION);
	if (index >= NBL_MAX_KTLS_SESSION)
		return -ENOSPC;

	set_bit(index, accel_mgt->tx_ktls_bitmap);
	accel_mgt->dtls_cfg_info[index].vld = true;
	accel_mgt->dtls_cfg_info[index].vsi = vsi;
	return index;
}

static void nbl_res_free_ktls_tx_index(void *priv, u32 index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_accel_mgt *accel_mgt;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	clear_bit(index, accel_mgt->tx_ktls_bitmap);
	memset(&accel_mgt->dtls_cfg_info[index], 0, sizeof(struct nbl_tls_cfg_info));
}

static void nbl_res_cfg_ktls_tx_keymat(void *priv, u32 index, u8 mode,
				       u8 *salt, u8 *key, u8 key_len)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops;

	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->cfg_ktls_tx_keymat(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				    index, mode, salt, key, key_len);
}

static int nbl_res_alloc_ktls_rx_index(void *priv, u16 vsi)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_accel_mgt *accel_mgt;
	u32 index;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	index = find_first_zero_bit(accel_mgt->rx_ktls_bitmap, NBL_MAX_KTLS_SESSION);
	if (index >= NBL_MAX_KTLS_SESSION)
		return -ENOSPC;

	set_bit(index, accel_mgt->rx_ktls_bitmap);
	accel_mgt->utls_cfg_info[index].vld = true;
	accel_mgt->utls_cfg_info[index].vsi = vsi;
	return index;
}

static void nbl_res_free_ktls_rx_index(void *priv, u32 index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_accel_mgt *accel_mgt;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	clear_bit(index, accel_mgt->rx_ktls_bitmap);
	memset(&accel_mgt->utls_cfg_info[index], 0, sizeof(struct nbl_tls_cfg_info));
}

static void nbl_res_cfg_ktls_rx_keymat(void *priv, u32 index, u8 mode,
				       u8 *salt, u8 *key, u8 key_len)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops;

	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	phy_ops->cfg_ktls_rx_keymat(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				    index, mode, salt, key, key_len);
}

static void nbl_res_cfg_ktls_rx_record(void *priv, u32 index, u32 tcp_sn, u64 rec_num, bool init)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops;

	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	phy_ops->cfg_ktls_rx_record(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				    index, tcp_sn, rec_num, init);
}

static int nbl_res_alloc_ipsec_tx_index(void *priv, struct nbl_ipsec_cfg_info *cfg_info)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_accel_mgt *accel_mgt;
	u32 index;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	index = find_first_zero_bit(accel_mgt->tx_ipsec_bitmap, NBL_MAX_IPSEC_SESSION);
	if (index >= NBL_MAX_IPSEC_SESSION)
		return -ENOSPC;

	set_bit(index, accel_mgt->tx_ipsec_bitmap);
	memcpy(&accel_mgt->tx_cfg_info[index], cfg_info, sizeof(struct nbl_ipsec_cfg_info));
	return index;
}

static void nbl_res_free_ipsec_tx_index(void *priv, u32 index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_accel_mgt *accel_mgt;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	clear_bit(index, accel_mgt->tx_ipsec_bitmap);
	memset(&accel_mgt->tx_cfg_info[index], 0, sizeof(struct nbl_ipsec_cfg_info));
}

static int nbl_res_alloc_ipsec_rx_index(void *priv, struct nbl_ipsec_cfg_info *cfg_info)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_accel_mgt *accel_mgt;
	u32 index;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	index = find_first_zero_bit(accel_mgt->rx_ipsec_bitmap, NBL_MAX_IPSEC_SESSION);
	if (index >= NBL_MAX_IPSEC_SESSION)
		return -ENOSPC;

	set_bit(index, accel_mgt->rx_ipsec_bitmap);
	memcpy(&accel_mgt->rx_cfg_info[index], cfg_info, sizeof(struct nbl_ipsec_cfg_info));
	return index;
}

static void nbl_res_free_ipsec_rx_index(void *priv, u32 index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_accel_mgt *accel_mgt;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	clear_bit(index, accel_mgt->rx_ipsec_bitmap);
	memset(&accel_mgt->rx_cfg_info[index], 0, sizeof(struct nbl_ipsec_cfg_info));
}

static void nbl_res_cfg_ipsec_tx_sad(void *priv, u32 index, struct nbl_ipsec_sa_entry *sa_entry)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops;
	struct nbl_ipsec_esn_state *esn_state = &sa_entry->esn_state;
	struct nbl_accel_esp_xfrm_attrs *attrs = &sa_entry->attrs;
	struct nbl_ipsec_cfg_info *cfg_info = &sa_entry->cfg_info;
	struct aes_gcm_keymat *aes_gcm = &attrs->aes_gcm;
	u32 ip_data[NBL_DIPSEC_SAD_IP_TOTAL] = {0};
	int i;

	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	if (attrs->nat_flag)
		phy_ops->cfg_dipsec_nat(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), attrs->sport);

	phy_ops->cfg_dipsec_sad_iv(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), index, aes_gcm->seq_iv);

	phy_ops->cfg_dipsec_sad_esn(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				    index, esn_state->sn, esn_state->esn,
				    esn_state->wrap_en, esn_state->enable);

	phy_ops->cfg_dipsec_sad_lifetime(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					 index, cfg_info->lft_cnt, cfg_info->lft_diff,
					 cfg_info->limit_enable, cfg_info->limit_type);

	phy_ops->cfg_dipsec_sad_crypto(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				       index, aes_gcm->aes_key, aes_gcm->salt,
				       aes_gcm->crypto_type, attrs->tunnel_mode, aes_gcm->icv_len);

	if (attrs->is_ipv6) {
		for (i = 0; i < NBL_DIPSEC_SAD_IP_LEN; i++)
			ip_data[i] = ntohl(attrs->daddr.a6[NBL_DIPSEC_SAD_IP_LEN - i - 1]);

		for (i = 0; i < NBL_DIPSEC_SAD_IP_LEN; i++)
			ip_data[i + NBL_DIPSEC_SAD_IP_LEN] =
				ntohl(attrs->saddr.a6[NBL_DIPSEC_SAD_IP_LEN - i - 1]);
	} else {
		ip_data[0] = ntohl(attrs->daddr.a4);
		ip_data[NBL_DIPSEC_SAD_IP_LEN] = ntohl(attrs->saddr.a4);
	}

	phy_ops->cfg_dipsec_sad_encap(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				      index, attrs->nat_flag, attrs->dport, attrs->spi, ip_data);
}

static void nbl_res_cfg_ipsec_rx_sad(void *priv, u32 index, struct nbl_ipsec_sa_entry *sa_entry)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops;
	struct nbl_ipsec_esn_state *esn_state = &sa_entry->esn_state;
	struct nbl_accel_esp_xfrm_attrs *attrs = &sa_entry->attrs;
	struct nbl_ipsec_cfg_info *cfg_info = &sa_entry->cfg_info;
	struct aes_gcm_keymat *aes_gcm = &attrs->aes_gcm;

	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	if (attrs->nat_flag)
		phy_ops->cfg_uipsec_nat(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					attrs->nat_flag, attrs->dport);

	phy_ops->cfg_uipsec_sad_esn(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				    index, esn_state->sn, esn_state->esn,
				    esn_state->overlap, esn_state->enable);

	phy_ops->cfg_uipsec_sad_lifetime(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					 index, cfg_info->lft_cnt, cfg_info->lft_diff,
					 cfg_info->limit_enable, cfg_info->limit_type);

	phy_ops->cfg_uipsec_sad_crypto(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
				       index, aes_gcm->aes_key, aes_gcm->salt,
				       aes_gcm->crypto_type, attrs->tunnel_mode, aes_gcm->icv_len);

	if (esn_state->window_en)
		phy_ops->cfg_uipsec_sad_window(NBL_RES_MGT_TO_PHY_PRIV(res_mgt),
					       index, esn_state->window_en, esn_state->option);
}

static void nbl_uipsec_get_em_hash(struct nbl_flow_fem_entry *flow, u8 *key_data)
{
	u16 ht0_hash = 0;
	u16 ht1_hash = 0;
	u8 key[NBL_UIPSEC_BYTE_LEN];
	int i;

	for (i = 0; i < NBL_UIPSEC_BYTE_LEN; i++)
		key[NBL_UIPSEC_BYTE_LEN - 1 - i] = key_data[i];

	ht0_hash = NBL_CRC16_CCITT(key, NBL_UIPSEC_BYTE_LEN);
	ht1_hash = NBL_CRC16_IBM(key, NBL_UIPSEC_BYTE_LEN);

	flow->ht0_hash  = nbl_hash_transfer(ht0_hash, NBL_UIPSEC_POWER, 0);
	flow->ht1_hash = nbl_hash_transfer(ht1_hash, NBL_UIPSEC_POWER, 0);
}

static bool nbl_uipsec_ht0_ht1_search(struct nbl_ipsec_ht_mng *ipsec_ht0_mng, uint16_t ht0_hash,
				      struct nbl_ipsec_ht_mng *ipsec_ht1_mng, uint16_t ht1_hash,
				      struct nbl_common_info *common)
{
	struct nbl_flow_ht_tbl *node0 = NULL;
	struct nbl_flow_ht_tbl *node1 = NULL;
	u16 i = 0;

	node0 = ipsec_ht0_mng->hash_map[ht0_hash];
	if (node0)
		for (i = 0; i < NBL_HASH_CFT_MAX; i++)
			if (node0->key[i].vid == 1 && node0->key[i].ht_other_index == ht1_hash) {
				nbl_info(common, NBL_DEBUG_ACCEL,
					 "Conflicted ht on vid %d and kt_index %u\n",
					 node0->key[i].vid, node0->key[i].kt_index);
				return true;
			}

	node1 = ipsec_ht1_mng->hash_map[ht1_hash];
	if (node1)
		for (i = 0; i < NBL_HASH_CFT_MAX; i++)
			if (node1->key[i].vid == 1 && node1->key[i].ht_other_index == ht0_hash) {
				nbl_info(common, NBL_DEBUG_ACCEL,
					 "Conflicted ht on vid %d and kt_index %u\n",
					 node1->key[i].vid, node1->key[i].kt_index);
				return true;
			}

	return false;
}

static int nbl_uipsec_find_ht_avail_table(struct nbl_ipsec_ht_mng *ipsec_ht0_mng,
					  struct nbl_ipsec_ht_mng *ipsec_ht1_mng,
					  u16 ht0_hash, u16 ht1_hash)
{
	struct nbl_flow_ht_tbl *pp_ht0_node = NULL;
	struct nbl_flow_ht_tbl *pp_ht1_node = NULL;

	pp_ht0_node = ipsec_ht0_mng->hash_map[ht0_hash];
	pp_ht1_node = ipsec_ht1_mng->hash_map[ht1_hash];

	if (!pp_ht0_node && !pp_ht1_node) {
		return 0;
	} else if (pp_ht0_node && !pp_ht1_node) {
		if (pp_ht0_node->ref_cnt >= NBL_HASH_CFT_AVL)
			return 1;
		else
			return 0;
	} else if (!pp_ht0_node && pp_ht1_node) {
		if (pp_ht1_node->ref_cnt >= NBL_HASH_CFT_AVL)
			return 0;
		else
			return 1;
	} else {
		if ((pp_ht0_node->ref_cnt <= NBL_HASH_CFT_AVL ||
		     (pp_ht0_node->ref_cnt > NBL_HASH_CFT_AVL &&
		      pp_ht0_node->ref_cnt < NBL_HASH_CFT_MAX &&
		      pp_ht1_node->ref_cnt > NBL_HASH_CFT_AVL)))
			return 0;
		else if (((pp_ht0_node->ref_cnt > NBL_HASH_CFT_AVL &&
			   pp_ht1_node->ref_cnt <= NBL_HASH_CFT_AVL) ||
			  (pp_ht0_node->ref_cnt == NBL_HASH_CFT_MAX &&
			   pp_ht1_node->ref_cnt > NBL_HASH_CFT_AVL &&
			   pp_ht1_node->ref_cnt < NBL_HASH_CFT_MAX)))
			return 1;
		else
			return -1;
	}
}

static void nbl_uipsec_cfg_em_tcam(struct nbl_resource_mgt *res_mgt, u32 index,
				   u32 *data, struct nbl_flow_fem_entry *flow)
{
	struct nbl_accel_mgt *accel_mgt;
	struct nbl_common_info *common;
	struct nbl_phy_ops *phy_ops;
	u16 tcam_index;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	common = NBL_RES_MGT_TO_COMMON(res_mgt);
	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	tcam_index = find_first_zero_bit(accel_mgt->ipsec_tcam_id, NBL_MAX_IPSEC_TCAM);
	if (tcam_index >= NBL_MAX_IPSEC_TCAM) {
		nbl_err(common, NBL_DEBUG_ACCEL,
			"There is no available ipsec tcam id left for sa index %u\n", index);
		return;
	}

	nbl_info(common, NBL_DEBUG_ACCEL,
		 "put sad index %u to ipsec tcam index %u.\n", index, tcam_index);
	phy_ops->cfg_uipsec_em_tcam(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), tcam_index, data);
	phy_ops->cfg_uipsec_em_ad(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), tcam_index, index);

	flow->tcam_index = tcam_index;
	flow->tcam_flag = true;
	set_bit(tcam_index, accel_mgt->ipsec_tcam_id);
}

static int nbl_uipsec_insert_em_ht(struct nbl_ipsec_ht_mng *ipsec_ht_mng,
				   struct nbl_flow_fem_entry *flow)
{
	struct nbl_flow_ht_tbl *node;
	u16 ht_index;
	u16 ht_other_index;
	int i;

	ht_index = (flow->hash_table == NBL_HT0 ? flow->ht0_hash : flow->ht1_hash);
	ht_other_index = (flow->hash_table == NBL_HT0 ? flow->ht1_hash : flow->ht0_hash);

	node = ipsec_ht_mng->hash_map[ht_index];
	if (!node) {
		node = kzalloc(sizeof(*node), GFP_ATOMIC);
		if (!node)
			return -ENOMEM;
		ipsec_ht_mng->hash_map[ht_index] = node;
	}

	for (i = 0; i < NBL_HASH_CFT_MAX; i++) {
		if (node->key[i].vid == 0) {
			node->key[i].vid = 1;
			node->key[i].ht_other_index = ht_other_index;
			node->key[i].kt_index = flow->flow_id;
			node->ref_cnt++;
			flow->hash_bucket = i;
			break;
		}
	}

	return 0;
}

static void nbl_uipsec_cfg_em_flow(struct nbl_resource_mgt *res_mgt, u32 index,
				   u32 *data, struct nbl_flow_fem_entry *flow)
{
	struct nbl_phy_ops *phy_ops;
	u16 ht_table;
	u16 ht_index;
	u16 ht_other_index;
	u16 ht_bucket;

	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	ht_table = flow->hash_table;
	ht_index = (flow->hash_table == NBL_HT0 ? flow->ht0_hash : flow->ht1_hash);
	ht_other_index = (flow->hash_table == NBL_HT0 ? flow->ht1_hash : flow->ht0_hash);
	ht_bucket = flow->hash_bucket;

	phy_ops->cfg_uipsec_em_ht(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), index, ht_table,
				  ht_index, ht_other_index, ht_bucket);
	phy_ops->cfg_uipsec_em_kt(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), index, data);
}

static int nbl_accel_add_uipsec_rule(struct nbl_resource_mgt *res_mgt, u32 index, u32 *data,
				     struct nbl_flow_fem_entry *flow)
{
	struct nbl_accel_mgt *accel_mgt;
	struct nbl_common_info *common;
	struct nbl_phy_ops *phy_ops;
	struct nbl_ipsec_ht_mng *ipsec_ht_mng = NULL;
	u8 key_data[NBL_UIPSEC_BYTE_LEN];
	int ht_table;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	common = NBL_RES_MGT_TO_COMMON(res_mgt);
	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	flow->flow_id = index;
	memcpy(key_data, data, NBL_UIPSEC_BYTE_LEN);
	nbl_uipsec_get_em_hash(flow, key_data);

	/* two flows have the same ht0&ht1, put the conflicted one to tcam */
	if (nbl_uipsec_ht0_ht1_search(&accel_mgt->ipsec_ht0_mng, flow->ht0_hash,
				      &accel_mgt->ipsec_ht1_mng, flow->ht1_hash, common))
		flow->tcam_flag = true;

	ht_table = nbl_uipsec_find_ht_avail_table(&accel_mgt->ipsec_ht0_mng,
						  &accel_mgt->ipsec_ht1_mng,
						  flow->ht0_hash, flow->ht1_hash);
	if (ht_table < 0)
		flow->tcam_flag = true;

	if (flow->tcam_flag) {
		nbl_uipsec_cfg_em_tcam(res_mgt, index, data, flow);
		return 0;
	}

	ipsec_ht_mng =
		(ht_table == NBL_HT0 ? &accel_mgt->ipsec_ht0_mng : &accel_mgt->ipsec_ht1_mng);
	flow->hash_table = ht_table;
	if (nbl_uipsec_insert_em_ht(ipsec_ht_mng, flow))
		return -ENOMEM;

	nbl_info(common, NBL_DEBUG_ACCEL, "cfg uipsec flow_item: %u, %u, %u, %u, %u\n",
		 flow->flow_id, flow->hash_table, flow->ht0_hash,
		 flow->ht1_hash, flow->hash_bucket);
	nbl_uipsec_cfg_em_flow(res_mgt, index, data, flow);

	return 0;
}

static int nbl_res_add_ipsec_rx_flow(void *priv, u32 index, u32 *data, u16 vsi)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_accel_mgt *accel_mgt;
	struct nbl_accel_uipsec_rule *rule;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);

	list_for_each_entry(rule, &accel_mgt->uprbac_head, node)
		if (rule->index == index)
			return -EEXIST;

	rule = kzalloc(sizeof(*rule), GFP_ATOMIC);
	if (!rule)
		return -ENOMEM;

	if (nbl_accel_add_uipsec_rule(res_mgt, index, data,  &rule->uipsec_entry)) {
		kfree(rule);
		return -EFAULT;
	}

	rule->index = index;
	rule->vsi = vsi;
	list_add_tail(&rule->node, &accel_mgt->uprbac_head);

	return 0;
}

static void nbl_uipsec_clear_em_tcam(struct nbl_resource_mgt *res_mgt,
				     struct nbl_flow_fem_entry *flow)
{
	struct nbl_accel_mgt *accel_mgt;
	struct nbl_common_info *common;
	struct nbl_phy_ops *phy_ops;
	u16 tcam_index;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	common = NBL_RES_MGT_TO_COMMON(res_mgt);
	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	tcam_index = flow->tcam_index;

	nbl_info(common, NBL_DEBUG_ACCEL,
		 "del sad index %u from ipsec tcam index %u.\n", flow->flow_id, tcam_index);
	phy_ops->clear_uipsec_tcam_ad(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), tcam_index);
	clear_bit(tcam_index, accel_mgt->ipsec_tcam_id);
}

static void nbl_uipsec_remove_em_ht(struct nbl_ipsec_ht_mng *ipsec_ht_mng,
				    struct nbl_flow_fem_entry *flow)
{
	struct nbl_flow_ht_tbl *node;
	u16 ht_index;

	ht_index = (flow->hash_table == NBL_HT0 ? flow->ht0_hash : flow->ht1_hash);
	node = ipsec_ht_mng->hash_map[ht_index];
	if (!node)
		return;

	memset(&node->key[flow->hash_bucket], 0, sizeof(node->key[flow->hash_bucket]));
	node->ref_cnt--;
	if (!node->ref_cnt) {
		kfree(node);
		ipsec_ht_mng->hash_map[ht_index] = NULL;
	}
}

static void nbl_uipsec_clear_em_flow(struct nbl_resource_mgt *res_mgt,
				     struct nbl_flow_fem_entry *flow)
{
	struct nbl_phy_ops *phy_ops;
	u16 ht_table;
	u16 ht_index;
	u16 ht_bucket;

	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	ht_table = flow->hash_table;
	ht_index = (flow->hash_table == NBL_HT0 ? flow->ht0_hash : flow->ht1_hash);
	ht_bucket = flow->hash_bucket;

	phy_ops->clear_uipsec_ht_kt(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), flow->flow_id,
				    ht_table, ht_index, ht_bucket);
}

static void nbl_accel_del_uipsec_rule(struct nbl_resource_mgt *res_mgt,
				      struct nbl_flow_fem_entry *flow)
{
	struct nbl_accel_mgt *accel_mgt;
	struct nbl_common_info *common;
	struct nbl_phy_ops *phy_ops;
	struct nbl_ipsec_ht_mng *ipsec_ht_mng = NULL;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	common = NBL_RES_MGT_TO_COMMON(res_mgt);
	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	if (flow->tcam_flag) {
		nbl_uipsec_clear_em_tcam(res_mgt, flow);
		return;
	}

	ipsec_ht_mng = (flow->hash_table == NBL_HT0 ?
			&accel_mgt->ipsec_ht0_mng : &accel_mgt->ipsec_ht1_mng);
	nbl_uipsec_remove_em_ht(ipsec_ht_mng, flow);
	nbl_info(common, NBL_DEBUG_ACCEL, "del uipsec flow_item: %u, %u, %u, %u, %u\n",
		 flow->flow_id, flow->hash_table, flow->ht0_hash,
		 flow->ht1_hash, flow->hash_bucket);

	nbl_uipsec_clear_em_flow(res_mgt, flow);
}

static void nbl_res_del_ipsec_rx_flow(void *priv, u32 index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_accel_mgt *accel_mgt;
	struct nbl_accel_uipsec_rule *rule;

	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);

	list_for_each_entry(rule, &accel_mgt->uprbac_head, node)
		if (rule->index == index)
			break;

	if (list_entry_is_head(rule, &accel_mgt->uprbac_head, node))
		return;

	nbl_accel_del_uipsec_rule(res_mgt, &rule->uipsec_entry);
	list_del(&rule->node);
	kfree(rule);
}

static void nbl_res_flr_clear_accel(void *priv, u16 vf_id)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_accel_mgt *accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	struct nbl_accel_uipsec_rule *uipsec_rule, *uipsec_rule_safe;
	u16 func_id = vf_id + NBL_MAX_PF;
	u16 vsi_id = nbl_res_func_id_to_vsi_id(res_mgt, func_id, NBL_VSI_SERV_VF_DATA_TYPE);
	int i;

	if (nbl_res_vf_is_active(priv, func_id)) {
		for (i = 0; i < NBL_MAX_IPSEC_SESSION; i++) {
			if (accel_mgt->tx_cfg_info[i].vld &&
			    accel_mgt->tx_cfg_info[i].vsi == vsi_id) {
				clear_bit(i, accel_mgt->tx_ipsec_bitmap);
				memset(&accel_mgt->tx_cfg_info[i], 0,
				       sizeof(struct nbl_ipsec_cfg_info));
			}
		}

		list_for_each_entry_safe(uipsec_rule, uipsec_rule_safe,
					 &accel_mgt->uprbac_head, node)
			if (uipsec_rule->vsi == vsi_id) {
				nbl_accel_del_uipsec_rule(res_mgt, &uipsec_rule->uipsec_entry);
				list_del(&uipsec_rule->node);
				kfree(uipsec_rule);
			}

		for (i = 0; i < NBL_MAX_IPSEC_SESSION; i++) {
			if (accel_mgt->rx_cfg_info[i].vld &&
			    accel_mgt->rx_cfg_info[i].vsi == vsi_id) {
				clear_bit(i, accel_mgt->rx_ipsec_bitmap);
				memset(&accel_mgt->rx_cfg_info[i], 0,
				       sizeof(struct nbl_ipsec_cfg_info));
			}
		}

		for (i = 0; i < NBL_MAX_KTLS_SESSION; i++) {
			if (accel_mgt->dtls_cfg_info[i].vld &&
			    accel_mgt->dtls_cfg_info[i].vsi == vsi_id) {
				clear_bit(i, accel_mgt->tx_ktls_bitmap);
				memset(&accel_mgt->dtls_cfg_info[i], 0,
				       sizeof(struct nbl_tls_cfg_info));
			}
		}

		for (i = 0; i < NBL_MAX_KTLS_SESSION; i++) {
			if (accel_mgt->utls_cfg_info[i].vld &&
			    accel_mgt->utls_cfg_info[i].vsi == vsi_id) {
				clear_bit(i, accel_mgt->rx_ktls_bitmap);
				memset(&accel_mgt->utls_cfg_info[i], 0,
				       sizeof(struct nbl_tls_cfg_info));
			}
		}
	}
}

static bool nbl_res_check_ipsec_status(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops;
	u32 dipsec_status;
	u32 uipsec_status;

	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	dipsec_status = phy_ops->read_dipsec_status(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
	uipsec_status = phy_ops->read_uipsec_status(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));

	if ((dipsec_status & NBL_IPSEC_SOFT_EXPIRE) ||
	    (dipsec_status & NBL_IPSEC_HARD_EXPIRE) ||
	    ((uipsec_status) & NBL_IPSEC_SOFT_EXPIRE) ||
	    ((uipsec_status) & NBL_IPSEC_HARD_EXPIRE))
		return true;

	return false;
}

static u32 nbl_res_get_dipsec_lft_info(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops;
	union nbl_ipsec_lft_info lft_info;
	u32 dipsec_status;

	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	lft_info.data = phy_ops->read_dipsec_lft_info(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
	dipsec_status = phy_ops->reset_dipsec_status(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
	lft_info.soft_vld = !!(dipsec_status & NBL_IPSEC_SOFT_EXPIRE);
	lft_info.hard_vld = !!(dipsec_status & NBL_IPSEC_HARD_EXPIRE);

	return lft_info.data;
}

static void nbl_res_handle_dipsec_soft_expire(void *priv, u32 index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common;
	struct nbl_accel_mgt *accel_mgt;
	struct nbl_phy_ops *phy_ops;
	struct nbl_ipsec_cfg_info *cfg_info;
	u32 lifetime_diff;
	u32 flag_wen;
	u32 msb_wen;
	bool need = false;

	common = NBL_RES_MGT_TO_COMMON(res_mgt);
	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	cfg_info = &accel_mgt->tx_cfg_info[index];

	if (!cfg_info->vld)
		return;

	if (cfg_info->soft_round == 0) {
		nbl_info(common, NBL_DEBUG_ACCEL, "dipsec sa %u soft expire.\n", index);
		if (cfg_info->hard_round == 0) {
			lifetime_diff = 0;
			flag_wen = 1;
			msb_wen = 0;
			need = true;
		}
	}

	if (cfg_info->hard_round == 1) {
		if (cfg_info->hard_remain > cfg_info->soft_remain)
			lifetime_diff = cfg_info->hard_remain -
					cfg_info->soft_remain;
		else
			lifetime_diff = (1 << NBL_IPSEC_LIFETIME_ROUND) +
					cfg_info->hard_remain -
					cfg_info->soft_remain;
		flag_wen = 1;
		msb_wen = 0;
		need = true;
		if (cfg_info->soft_round > 0)
			nbl_info(common, NBL_DEBUG_ACCEL,
				 "dipsec sa %u soft expire in advance.\n", index);
	}

	if (cfg_info->hard_round > 1) {
		lifetime_diff = 0;
		flag_wen = 0;
		msb_wen = 1;
		need = true;
		if (cfg_info->soft_round)
			cfg_info->soft_round--;
		cfg_info->hard_round--;
	}

	if (need)
		phy_ops->cfg_dipsec_lft_info(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), index,
					     lifetime_diff, flag_wen, msb_wen);
}

static u32 nbl_res_get_uipsec_lft_info(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops;
	union nbl_ipsec_lft_info lft_info;
	u32 uipsec_status;

	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	lft_info.data = phy_ops->read_uipsec_lft_info(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
	uipsec_status = phy_ops->reset_uipsec_status(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
	lft_info.soft_vld = !!(uipsec_status & NBL_IPSEC_SOFT_EXPIRE);
	lft_info.hard_vld = !!(uipsec_status & NBL_IPSEC_HARD_EXPIRE);

	return lft_info.data;
}

static void nbl_res_handle_uipsec_soft_expire(void *priv, u32 index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common;
	struct nbl_accel_mgt *accel_mgt;
	struct nbl_phy_ops *phy_ops;
	struct nbl_ipsec_cfg_info *cfg_info;
	u32 lifetime_diff;
	u32 flag_wen;
	u32 msb_wen;
	bool need = false;

	common = NBL_RES_MGT_TO_COMMON(res_mgt);
	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	cfg_info = &accel_mgt->rx_cfg_info[index];

	if (!cfg_info->vld)
		return;

	if (cfg_info->soft_round == 0) {
		nbl_info(common, NBL_DEBUG_ACCEL, "uipsec sa %u soft expire.\n", index);
		if (cfg_info->hard_round == 0) {
			lifetime_diff = 0;
			flag_wen = 1;
			msb_wen = 0;
			need = true;
		}
	}

	if (cfg_info->hard_round == 1) {
		if (cfg_info->hard_remain > cfg_info->soft_remain)
			lifetime_diff = cfg_info->hard_remain -
					cfg_info->soft_remain;
		else
			lifetime_diff = (1 << NBL_IPSEC_LIFETIME_ROUND) +
					cfg_info->hard_remain -
					cfg_info->soft_remain;
		flag_wen = 1;
		msb_wen = 0;
		need = true;
		if (cfg_info->soft_round > 0)
			nbl_info(common, NBL_DEBUG_ACCEL,
				 "uipsec sa %u soft expire in advance.\n", index);
	}

	if (cfg_info->hard_round > 1) {
		lifetime_diff = 0;
		flag_wen = 0;
		msb_wen = 1;
		need = true;
		if (cfg_info->soft_round)
			cfg_info->soft_round--;
		cfg_info->hard_round--;
	}

	if (need)
		phy_ops->cfg_uipsec_lft_info(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), index,
					     lifetime_diff, flag_wen, msb_wen);
}

static void nbl_res_handle_dipsec_hard_expire(void *priv, u32 index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common;
	struct nbl_channel_ops *chan_ops;
	struct nbl_accel_mgt *accel_mgt;
	struct nbl_sa_search_key param;
	struct nbl_chan_send_info chan_send;
	u16 vsid;
	u16 dstid;

	chan_ops = NBL_RES_MGT_TO_CHAN_OPS(res_mgt);
	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	common = NBL_RES_MGT_TO_COMMON(res_mgt);

	if (!accel_mgt->tx_cfg_info[index].vld)
		return;

	vsid = accel_mgt->tx_cfg_info[index].vsi;
	dstid = nbl_res_vsi_id_to_func_id(res_mgt, vsid);
	param.family = accel_mgt->tx_cfg_info[index].sa_key.family;
	param.mark = accel_mgt->tx_cfg_info[index].sa_key.mark;
	param.spi = accel_mgt->tx_cfg_info[index].sa_key.spi;
	memcpy(&param.daddr, &accel_mgt->tx_cfg_info[index].sa_key.daddr, sizeof(param.daddr));

	nbl_info(common, NBL_DEBUG_ACCEL, "dipsec sa %u hard expire.\n", index);
	NBL_CHAN_SEND(chan_send, dstid, NBL_CHAN_MSG_NOTIFY_IPSEC_HARD_EXPIRE, &param,
		      sizeof(param), NULL, 0, 0);
	chan_ops->send_msg(NBL_RES_MGT_TO_CHAN_PRIV(res_mgt), &chan_send);
}

static void nbl_res_handle_uipsec_hard_expire(void *priv, u32 index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common;
	struct nbl_channel_ops *chan_ops;
	struct nbl_accel_mgt *accel_mgt;
	struct nbl_sa_search_key param;
	struct nbl_chan_send_info chan_send;
	u16 vsid;
	u16 dstid;

	chan_ops = NBL_RES_MGT_TO_CHAN_OPS(res_mgt);
	accel_mgt = NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	common = NBL_RES_MGT_TO_COMMON(res_mgt);

	if (!accel_mgt->rx_cfg_info[index].vld)
		return;

	vsid = accel_mgt->rx_cfg_info[index].vsi;
	dstid = nbl_res_vsi_id_to_func_id(res_mgt, vsid);
	param.family = accel_mgt->rx_cfg_info[index].sa_key.family;
	param.mark = accel_mgt->rx_cfg_info[index].sa_key.mark;
	param.spi = accel_mgt->rx_cfg_info[index].sa_key.spi;
	memcpy(&param.daddr, &accel_mgt->rx_cfg_info[index].sa_key.daddr, sizeof(param.daddr));

	nbl_info(common, NBL_DEBUG_ACCEL, "uipsec sa %u hard expire.\n", index);
	NBL_CHAN_SEND(chan_send, dstid, NBL_CHAN_MSG_NOTIFY_IPSEC_HARD_EXPIRE, &param,
		      sizeof(param), NULL, 0, 0);
	chan_ops->send_msg(NBL_RES_MGT_TO_CHAN_PRIV(res_mgt), &chan_send);
}

/* NBL_ACCEL_SET_OPS(ops_name, func)
 *
 * Use X Macros to reduce setup and remove codes.
 */
#define NBL_ACCEL_OPS_TBL								\
do {											\
	NBL_ACCEL_SET_OPS(alloc_ktls_tx_index, nbl_res_alloc_ktls_tx_index);		\
	NBL_ACCEL_SET_OPS(free_ktls_tx_index, nbl_res_free_ktls_tx_index);		\
	NBL_ACCEL_SET_OPS(cfg_ktls_tx_keymat, nbl_res_cfg_ktls_tx_keymat);		\
	NBL_ACCEL_SET_OPS(alloc_ktls_rx_index, nbl_res_alloc_ktls_rx_index);		\
	NBL_ACCEL_SET_OPS(free_ktls_rx_index, nbl_res_free_ktls_rx_index);		\
	NBL_ACCEL_SET_OPS(cfg_ktls_rx_keymat, nbl_res_cfg_ktls_rx_keymat);		\
	NBL_ACCEL_SET_OPS(cfg_ktls_rx_record, nbl_res_cfg_ktls_rx_record);		\
	NBL_ACCEL_SET_OPS(alloc_ipsec_tx_index, nbl_res_alloc_ipsec_tx_index);		\
	NBL_ACCEL_SET_OPS(free_ipsec_tx_index, nbl_res_free_ipsec_tx_index);		\
	NBL_ACCEL_SET_OPS(alloc_ipsec_rx_index, nbl_res_alloc_ipsec_rx_index);		\
	NBL_ACCEL_SET_OPS(free_ipsec_rx_index, nbl_res_free_ipsec_rx_index);		\
	NBL_ACCEL_SET_OPS(cfg_ipsec_tx_sad, nbl_res_cfg_ipsec_tx_sad);			\
	NBL_ACCEL_SET_OPS(cfg_ipsec_rx_sad, nbl_res_cfg_ipsec_rx_sad);			\
	NBL_ACCEL_SET_OPS(add_ipsec_rx_flow, nbl_res_add_ipsec_rx_flow);		\
	NBL_ACCEL_SET_OPS(del_ipsec_rx_flow, nbl_res_del_ipsec_rx_flow);		\
	NBL_ACCEL_SET_OPS(flr_clear_accel, nbl_res_flr_clear_accel);			\
	NBL_ACCEL_SET_OPS(check_ipsec_status, nbl_res_check_ipsec_status);		\
	NBL_ACCEL_SET_OPS(get_dipsec_lft_info, nbl_res_get_dipsec_lft_info);		\
	NBL_ACCEL_SET_OPS(handle_dipsec_soft_expire, nbl_res_handle_dipsec_soft_expire);\
	NBL_ACCEL_SET_OPS(handle_dipsec_hard_expire, nbl_res_handle_dipsec_hard_expire);\
	NBL_ACCEL_SET_OPS(get_uipsec_lft_info, nbl_res_get_uipsec_lft_info);		\
	NBL_ACCEL_SET_OPS(handle_uipsec_soft_expire, nbl_res_handle_uipsec_soft_expire);\
	NBL_ACCEL_SET_OPS(handle_uipsec_hard_expire, nbl_res_handle_uipsec_hard_expire);\
} while (0)

/* Structure starts here, adding an op should not modify anything below */
static int nbl_accel_setup_mgt(struct device *dev, struct nbl_accel_mgt **accel_mgt)
{
	*accel_mgt = devm_kzalloc(dev, sizeof(struct nbl_accel_mgt), GFP_KERNEL);
	if (!*accel_mgt)
		return -ENOMEM;

	INIT_LIST_HEAD(&(*accel_mgt)->uprbac_head);
	return 0;
}

static void nbl_accel_remove_mgt(struct device *dev, struct nbl_accel_mgt **accel_mgt)
{
	devm_kfree(dev, *accel_mgt);
	*accel_mgt = NULL;
}

int nbl_accel_mgt_start(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_accel_mgt **accel_mgt;
	struct nbl_phy_ops *phy_ops;
	struct device *dev;

	dev = NBL_RES_MGT_TO_DEV(res_mgt);
	accel_mgt = &NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);
	phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);

	phy_ops->init_dprbac(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
	phy_ops->init_uprbac(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));

	return nbl_accel_setup_mgt(dev, accel_mgt);
}

void nbl_accel_mgt_stop(struct nbl_resource_mgt *res_mgt)
{
	struct device *dev;
	struct nbl_accel_mgt **accel_mgt;

	dev = NBL_RES_MGT_TO_DEV(res_mgt);
	accel_mgt = &NBL_RES_MGT_TO_ACCEL_MGT(res_mgt);

	if (!(*accel_mgt))
		return;

	nbl_accel_remove_mgt(dev, accel_mgt);
}

int nbl_accel_setup_ops(struct nbl_resource_ops *res_ops)
{
	if (!res_ops)
		return -EINVAL;

#define NBL_ACCEL_SET_OPS(name, func) do {res_ops->NBL_NAME(name) = func; ; } while (0)
	NBL_ACCEL_OPS_TBL;
#undef  NBL_ACCEL_SET_OPS

	return 0;
}
