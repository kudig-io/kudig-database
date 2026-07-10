// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#include "nbl_dispatch.h"

static int nbl_disp_chan_add_macvlan_req(void *priv, u8 *mac, u16 vlan, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_add_macvlan param;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	if (!disp_mgt || !mac)
		return -EINVAL;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	memcpy(param.mac, mac, sizeof(param.mac));
	param.vlan = vlan;
	param.vsi = vsi;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_ADD_MACVLAN, &param, sizeof(param),
		      NULL, 0, 1);

	if (chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send))
		return -EFAULT;

	return 0;
}

static void nbl_disp_chan_add_macvlan_resp(void *priv, u16 src_id, u16 msg_id,
					   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_add_macvlan *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_add_macvlan *)data;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->add_macvlan,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->mac,
				param->vlan, param->vsi);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ADD_MACVLAN, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_ADD_MACVLAN);
}

static void nbl_disp_chan_del_macvlan_req(void *priv, u8 *mac, u16 vlan, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_del_macvlan param;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	if (!disp_mgt || !mac)
		return;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	memcpy(param.mac, mac, sizeof(param.mac));
	param.vlan = vlan;
	param.vsi = vsi;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_DEL_MACVLAN, &param, sizeof(param),
		      NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_del_macvlan_resp(void *priv, u16 src_id, u16 msg_id,
					   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_del_macvlan *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_chan_param_del_macvlan *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->del_macvlan,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			  param->mac, param->vlan, param->vsi);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_DEL_MACVLAN, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_add_multi_rule_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	if (!disp_mgt)
		return -EINVAL;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_ADD_MULTI_RULE,
		      &vsi_id, sizeof(vsi_id), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_add_multi_rule_resp(void *priv, u16 src_id, u16 msg_id,
					      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	u8 broadcast_mac[ETH_ALEN];
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;
	u16 vsi_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	vsi_id = *(u16 *)data;
	memset(broadcast_mac, 0xFF, ETH_ALEN);
	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->add_macvlan,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), broadcast_mac, 0, vsi_id);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ADD_MULTI_RULE, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_del_multi_rule_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	if (!disp_mgt)
		return;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_DEL_MULTI_RULE,
		      &vsi_id, sizeof(vsi_id), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_del_multi_rule_resp(void *priv, u16 src_id, u16 msg_id,
					      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	u8 broadcast_mac[ETH_ALEN];
	int err = NBL_CHAN_RESP_OK;
	u16 vsi_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	vsi_id = *(u16 *)data;
	memset(broadcast_mac, 0xFF, ETH_ALEN);
	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->del_macvlan,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), broadcast_mac, 0, vsi_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_DEL_MULTI_RULE, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_cfg_multi_mcast(void *priv, u16 vsi, u16 enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	if (enable)
		ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->add_multi_mcast,
					NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi);
	else
		NBL_OPS_CALL_LOCK(disp_mgt, res_ops->del_multi_mcast,
				  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi);
	return ret;
}

static int nbl_disp_chan_cfg_multi_mcast_req(void *priv, u16 vsi_id, u16 enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;
	struct nbl_chan_param_cfg_multi_mcast mcast;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	mcast.vsi = vsi_id;
	mcast.enable = enable;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_CFG_MULTI_MCAST_RULE,
		      &mcast, sizeof(mcast), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_multi_mcast_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_cfg_multi_mcast *mcast;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	mcast = (struct nbl_chan_param_cfg_multi_mcast *)data;

	if (mcast->enable)
		ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->add_multi_mcast,
					NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), mcast->vsi);
	else
		NBL_OPS_CALL_LOCK(disp_mgt, res_ops->del_multi_mcast,
				  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), mcast->vsi);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_MULTI_MCAST_RULE, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_setup_multi_group_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SETUP_MULTI_GROUP,
		      NULL, 0, NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_setup_multi_group_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->setup_multi_group,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SETUP_MULTI_GROUP, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_remove_multi_group_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_REMOVE_MULTI_GROUP,
		      NULL, 0, NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_remove_multi_group_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_multi_group,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REMOVE_MULTI_GROUP, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_register_net_req(void *priv,
					  struct nbl_register_net_param *register_param,
					  struct nbl_register_net_result *register_result)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_register_net_info param = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;
	int ret = 0;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.pf_bar_start = register_param->pf_bar_start;
	param.pf_bdf = register_param->pf_bdf;
	param.vf_bar_start = register_param->vf_bar_start;
	param.vf_bar_size = register_param->vf_bar_size;
	param.total_vfs = register_param->total_vfs;
	param.offset = register_param->offset;
	param.stride = register_param->stride;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_REGISTER_NET, &param, sizeof(param),
		      (void *)register_result, sizeof(*register_result), 1);

	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	return ret;
}

static void nbl_disp_chan_register_net_resp(void *priv, u16 src_id, u16 msg_id,
					    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_register_net_info param;
	struct nbl_register_net_result result = {0};
	struct nbl_register_net_param register_param = {0};
	struct nbl_chan_ack_info chan_ack;
	int copy_len;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	memset(&param, 0, sizeof(struct nbl_chan_param_register_net_info));
	copy_len = data_len < sizeof(struct nbl_chan_param_register_net_info) ?
			data_len : sizeof(struct nbl_chan_param_register_net_info);
	memcpy(&param, data, copy_len);

	register_param.pf_bar_start = param.pf_bar_start;
	register_param.pf_bdf = param.pf_bdf;
	register_param.vf_bar_start = param.vf_bar_start;
	register_param.vf_bar_size = param.vf_bar_size;
	register_param.total_vfs = param.total_vfs;
	register_param.offset = param.offset;
	register_param.stride = param.stride;
	register_param.is_vdpa = param.is_vdpa;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->register_net,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id, &register_param, &result);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REGISTER_NET,
		     msg_id, err, &result, sizeof(result));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d, src_id:%d\n",
			ret, NBL_CHAN_MSG_REGISTER_NET, src_id);
}

static int nbl_disp_unregister_net(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->unregister_net,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), 0);
}

static int nbl_disp_chan_unregister_net_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_UNREGISTER_NET, NULL, 0, NULL, 0, 1);

	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_unregister_net_resp(void *priv, u16 src_id, u16 msg_id,
					      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->unregister_net,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_UNREGISTER_NET,
		     msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d, src_id:%d\n",
			ret, NBL_CHAN_MSG_UNREGISTER_NET, src_id);
}

static int nbl_disp_chan_alloc_txrx_queues_req(void *priv, u16 vsi_id, u16 queue_num)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_alloc_txrx_queues param = {0};
	struct nbl_chan_param_alloc_txrx_queues result = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	param.queue_num = queue_num;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_ALLOC_TXRX_QUEUES, &param,
		      sizeof(param), &result, sizeof(result), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_alloc_txrx_queues_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_alloc_txrx_queues *param;
	struct nbl_chan_param_alloc_txrx_queues result = {0};
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_chan_param_alloc_txrx_queues *)data;
	result.queue_num = param->queue_num;

	err = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->alloc_txrx_queues,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				param->vsi_id, param->queue_num);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ALLOC_TXRX_QUEUES,
		     msg_id, err, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_free_txrx_queues_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_FREE_TXRX_QUEUES,
		      &vsi_id, sizeof(vsi_id), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_free_txrx_queues_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	u16 vsi_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	vsi_id = *(u16 *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->free_txrx_queues,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_FREE_TXRX_QUEUES, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_register_vsi2q_req(void *priv, u16 vsi_index, u16 vsi_id,
					    u16 queue_offset, u16 queue_num)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_register_vsi2q param = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_index = vsi_index;
	param.vsi_id = vsi_id;
	param.queue_offset = queue_offset;
	param.queue_num = queue_num;
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_REGISTER_VSI2Q, &param,
		      sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_register_vsi2q_resp(void *priv, u16 src_id, u16 msg_id,
					      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_register_vsi2q *param = NULL;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_register_vsi2q *)data;

	err = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->register_vsi2q,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				param->vsi_index, param->vsi_id,
				param->queue_offset, param->queue_num);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REGISTER_VSI2Q, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_setup_q2vsi_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SETUP_Q2VSI, &vsi_id,
		      sizeof(vsi_id), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_setup_q2vsi_resp(void *priv, u16 src_id, u16 msg_id,
					   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	u16 vsi_id;

	vsi_id = *(u16 *)data;

	err = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->setup_q2vsi,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SETUP_Q2VSI, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_remove_q2vsi_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_REMOVE_Q2VSI, &vsi_id,
		      sizeof(vsi_id), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_remove_q2vsi_resp(void *priv, u16 src_id, u16 msg_id,
					    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	u16 vsi_id;

	vsi_id = *(u16 *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_q2vsi, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			  vsi_id);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REMOVE_Q2VSI, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_setup_rss_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SETUP_RSS, &vsi_id,
		      sizeof(vsi_id), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_setup_rss_resp(void *priv, u16 src_id, u16 msg_id,
					 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	u16 vsi_id;

	vsi_id = *(u16 *)data;
	err = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->setup_rss, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				vsi_id);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SETUP_RSS, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_remove_rss_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_REMOVE_RSS, &vsi_id,
		      sizeof(vsi_id), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_remove_rss_resp(void *priv, u16 src_id, u16 msg_id,
					  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	u16 vsi_id;

	vsi_id = *(u16 *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_rss, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			  vsi_id);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REMOVE_RSS, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_setup_queue_req(void *priv, struct nbl_txrx_queue_param *queue_param,
					 bool is_tx)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_setup_queue param;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	memcpy(&param.queue_param, queue_param, sizeof(param.queue_param));
	param.is_tx = is_tx;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SETUP_QUEUE, &param, sizeof(param),
		      NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_setup_queue_resp(void *priv, u16 src_id, u16 msg_id,
					   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_setup_queue *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_chan_param_setup_queue *)data;

	err = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->setup_queue, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				&param->queue_param, param->is_tx);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SETUP_QUEUE, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_remove_queue_req(void *priv, struct nbl_txrx_queue_param *queue_param,
					  bool is_tx)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_setup_queue param;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	memcpy(&param.queue_param, queue_param, sizeof(param.queue_param));
	param.is_tx = is_tx;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_REMOVE_QUEUE, &param, sizeof(param),
		      NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_remove_queue_resp(void *priv, u16 src_id, u16 msg_id,
					    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_setup_queue *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_chan_param_setup_queue *)data;

	err = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_queue, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				&param->queue_param, param->is_tx);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REMOVE_QUEUE, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_remove_all_queues_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_REMOVE_ALL_QUEUES,
		      &vsi_id, sizeof(vsi_id), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_remove_all_queues_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	u16 vsi_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	vsi_id = *(u16 *)data;
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_all_queues,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REMOVE_ALL_QUEUES, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_cfg_dsch_req(void *priv, u16 vsi_id, bool vld)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_cfg_dsch param = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	param.vld = vld;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_CFG_DSCH, &param, sizeof(param),
		      NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_dsch_resp(void *priv, u16 src_id, u16 msg_id,
					void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_cfg_dsch *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_chan_param_cfg_dsch *)data;

	err = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->cfg_dsch,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id, param->vld);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_DSCH, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_setup_cqs(void *priv, u16 vsi_id, u16 real_qps, bool rss_indir_set)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->setup_cqs,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				vsi_id, real_qps, rss_indir_set);
	return ret;
}

static int nbl_disp_chan_setup_cqs_req(void *priv, u16 vsi_id, u16 real_qps, bool rss_indir_set)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_setup_cqs param = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	param.real_qps = real_qps;
	param.rss_indir_set = rss_indir_set;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SETUP_CQS, &param, sizeof(param),
		      NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_setup_cqs_resp(void *priv, u16 src_id, u16 msg_id,
					 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_setup_cqs param;
	struct nbl_chan_ack_info chan_ack;
	int copy_len;
	int err = NBL_CHAN_RESP_OK;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	memset(&param, 0, sizeof(struct nbl_chan_param_setup_cqs));
	param.rss_indir_set = true;
	copy_len = data_len < sizeof(struct nbl_chan_param_setup_cqs) ?
			data_len : sizeof(struct nbl_chan_param_setup_cqs);
	memcpy(&param, data, copy_len);

	err = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->setup_cqs,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				param.vsi_id, param.real_qps, param.rss_indir_set);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SETUP_CQS, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_remove_cqs_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_REMOVE_CQS, &vsi_id, sizeof(vsi_id),
		      NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_remove_cqs_resp(void *priv, u16 src_id, u16 msg_id,
					  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	u16 vsi_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	vsi_id = *(u16 *)data;
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_cqs,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REMOVE_CQS, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_set_promisc_mode(void *priv, u16 vsi_id, u16 mode)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	if (!disp_mgt)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_promisc_mode,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, mode);
	return ret;
}

static int nbl_disp_chan_set_promisc_mode_req(void *priv, u16 vsi_id, u16 mode)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_set_promisc_mode param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	param.mode = mode;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SET_PROSISC_MODE,
		      &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_promisc_mode_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_set_promisc_mode *param = NULL;
	int err = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_set_promisc_mode *)data;
	err = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_promisc_mode,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id, param->mode);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_PROSISC_MODE, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_cfg_qdisc_mqprio_req(void *priv, struct nbl_tc_qidsc_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_CFG_QDISC_MQPRIO,
		      param, sizeof(*param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_qdisc_mqprio_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_tc_qidsc_param *param = (struct nbl_tc_qidsc_param *)data;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	err = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->cfg_qdisc_mqprio,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_QDISC_MQPRIO, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_set_spoof_check_addr_req(void *priv, u16 vsi_id, u8 *mac)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_spoof_check_addr param = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	ether_addr_copy(param.mac, mac);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SET_SPOOF_CHECK_ADDR,
		      &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_spoof_check_addr_resp(void *priv, u16 src_id, u16 msg_id,
						    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_spoof_check_addr *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_set_spoof_check_addr *)data;
	err = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_spoof_check_addr,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id, param->mac);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_SPOOF_CHECK_ADDR, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_set_vf_spoof_check_req(void *priv, u16 vsi_id, int vf_id, u8 enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_vf_spoof_check param = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	param.vf_id = vf_id;
	param.enable = enable;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SET_VF_SPOOF_CHECK,
		      &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_vf_spoof_check_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_vf_spoof_check *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_set_vf_spoof_check *)data;
	err = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_vf_spoof_check,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id,
				param->vf_id, param->enable);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_VF_SPOOF_CHECK, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_get_base_mac_addr_req(void *priv, u8 *mac)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_GET_BASE_MAC_ADDR,
		      NULL, 0, mac, ETH_ALEN, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_base_mac_addr_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	u8 mac[ETH_ALEN];

	NBL_OPS_CALL(res_ops->get_base_mac_addr,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), mac));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_BASE_MAC_ADDR, msg_id, err,
		     mac, ETH_ALEN);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_get_firmware_version_req(void *priv, char *firmware_verion, u8 max_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_FIRMWARE_VERSION, NULL, 0,
		      firmware_verion, max_len, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_firmware_version_resp(void *priv, u16 src_id, u16 msg_id,
						    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	char firmware_verion[ETHTOOL_FWVERS_LEN] = "";
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	ret = NBL_OPS_CALL(res_ops->get_firmware_version,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), firmware_verion));
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "get emp version failed with ret: %d\n", ret);
	}

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_FIRMWARE_VERSION, msg_id, err,
		     firmware_verion, ETHTOOL_FWVERS_LEN);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_GET_FIRMWARE_VERSION, src_id);
}

static int nbl_disp_get_queue_err_stats(void *priv, u8 queue_id,
					struct nbl_queue_err_stats *queue_err_stats, bool is_tx)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL(res_ops->get_queue_err_stats,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			    0, queue_id, queue_err_stats, is_tx));
}

static int nbl_disp_chan_get_queue_err_stats_req(void *priv, u8 queue_id,
						 struct nbl_queue_err_stats *queue_err_stats,
						 bool is_tx)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_get_queue_err_stats param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.queue_id = queue_id;
	param.is_tx = is_tx;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_QUEUE_ERR_STATS, &param,
		      sizeof(param), queue_err_stats, sizeof(*queue_err_stats), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_queue_err_stats_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_get_queue_err_stats *param;
	struct nbl_chan_ack_info chan_ack;
	struct nbl_queue_err_stats queue_err_stats = { 0 };
	int err = NBL_CHAN_RESP_OK;
	int ret;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_get_queue_err_stats *)data;

	ret = NBL_OPS_CALL(res_ops->get_queue_err_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id, param->queue_id,
			   &queue_err_stats, param->is_tx));
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "disp get queue err stats_resp failed with ret: %d\n", ret);
	}
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_QUEUE_ERR_STATS, msg_id, err,
		     &queue_err_stats, sizeof(queue_err_stats));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "disp chan send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_GET_QUEUE_ERR_STATS, src_id);
}

static int nbl_disp_get_eth_abnormal_stats(void *priv, u8 eth_id,
					   struct nbl_eth_abnormal_stats *eth_abnormal_stats)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_eth_abnormal_stats,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, eth_abnormal_stats);
}

static int
nbl_disp_chan_get_eth_abnormal_stats_req(void *priv, u8 eth_id,
					 struct nbl_eth_abnormal_stats *eth_abnormal_stats)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_ETH_ABNORMAL_STATS, &eth_id,
		      sizeof(eth_id), eth_abnormal_stats, sizeof(*eth_abnormal_stats), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_eth_abnormal_stats_resp(void *priv, u16 src_id, u16 msg_id,
						      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	struct nbl_eth_abnormal_stats eth_abnormal_stats = { 0 };
	int err = NBL_CHAN_RESP_OK;
	int ret;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_eth_abnormal_stats,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *(u8 *)data,
				&eth_abnormal_stats);
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "disp get eth abnormal stats resp failed with ret: %d\n", ret);
	}
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_ETH_ABNORMAL_STATS, msg_id, err,
		     &eth_abnormal_stats, sizeof(eth_abnormal_stats));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "disp chan send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_GET_ETH_ABNORMAL_STATS, src_id);
}

static void nbl_disp_chan_get_coalesce_req(void *priv, u16 vector_id,
					   struct nbl_chan_param_get_coalesce *ec)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_GET_COALESCE, &vector_id, sizeof(vector_id),
		      ec, sizeof(*ec), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_coalesce_resp(void *priv, u16 src_id, u16 msg_id,
					    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	struct nbl_chan_param_get_coalesce ec = { 0 };
	u16 vector_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	vector_id = *(u16 *)data;

	NBL_OPS_CALL(res_ops->get_coalesce,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id,
		      vector_id, &ec));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_COALESCE, msg_id, ret,
		     &ec, sizeof(ec));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_set_coalesce_req(void *priv, u16 vector_id,
					   u16 vector_num, u16 pnum, u16 rate)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_coalesce param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.local_vector_id = vector_id;
	param.vector_num = vector_num;
	param.rx_max_coalesced_frames = pnum;
	param.rx_coalesce_usecs = rate;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SET_COALESCE, &param, sizeof(param),
		      NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_coalesce_resp(void *priv, u16 src_id, u16 msg_id,
					    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_coalesce *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_set_coalesce *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_coalesce,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id, param->local_vector_id,
			  param->vector_num, param->rx_max_coalesced_frames,
			  param->rx_coalesce_usecs);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_COALESCE, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_get_rxfh_indir_size_req(void *priv, u16 vsi_id, u32 *rxfh_indir_size)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_RXFH_INDIR_SIZE,
		      &vsi_id, sizeof(vsi_id), rxfh_indir_size, sizeof(u32), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_rxfh_indir_size_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	u32 rxfh_indir_size = 0;
	int ret = NBL_CHAN_RESP_OK;
	u16 vsi_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	vsi_id = *(u16 *)data;
	NBL_OPS_CALL(res_ops->get_rxfh_indir_size,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, &rxfh_indir_size));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_RXFH_INDIR_SIZE, msg_id,
		     ret, &rxfh_indir_size, sizeof(u32));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_get_rxfh_indir_req(void *priv, u16 vsi_id, u32 *indir, u32 indir_size)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_get_rxfh_indir param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	param.rxfh_indir_size = indir_size;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_RXFH_INDIR, &param,
		      sizeof(param), indir, indir_size * sizeof(u32), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_rxfh_indir_resp(void *priv, u16 src_id, u16 msg_id,
					      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_get_rxfh_indir *param;
	struct nbl_chan_ack_info chan_ack;
	u32 *indir;
	int ret = NBL_CHAN_RESP_OK;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_get_rxfh_indir *)data;

	indir = kcalloc(param->rxfh_indir_size, sizeof(u32), GFP_KERNEL);
	NBL_OPS_CALL(res_ops->get_rxfh_indir,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id, indir));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_RXFH_INDIR, msg_id, ret,
		     indir, param->rxfh_indir_size * sizeof(u32));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);

	kfree(indir);
}

static void nbl_disp_chan_get_rxfh_rss_key_req(void *priv, u8 *rss_key, u32 rss_key_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_GET_RXFH_RSS_KEY, &rss_key_len,
		      sizeof(rss_key_len), rss_key, rss_key_len, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_rxfh_rss_key_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	u8 *rss_key;
	int ret = NBL_CHAN_RESP_OK;
	u32 rss_key_len;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	rss_key_len = *(u32 *)data;

	rss_key = kzalloc(rss_key_len, GFP_KERNEL);
	NBL_OPS_CALL(res_ops->get_rxfh_rss_key, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), rss_key));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_RXFH_RSS_KEY, msg_id, ret,
		     rss_key, rss_key_len);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);

	kfree(rss_key);
}

static void nbl_disp_chan_get_rxfh_rss_alg_sel_req(void *priv, u16 vsi_id, u8 *rss_alg_sel)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_RXFH_RSS_ALG_SEL, &vsi_id,
		      sizeof(vsi_id), rss_alg_sel, sizeof(u8), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_rxfh_rss_alg_sel_resp(void *priv, u16 src_id, u16 msg_id,
						    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	u16 vsi_id;
	u8 rss_alg_sel;
	int ret = NBL_CHAN_RESP_OK;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	vsi_id = *(u16 *)data;

	NBL_OPS_CALL(res_ops->get_rss_alg_sel,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, &rss_alg_sel));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_RXFH_RSS_ALG_SEL, msg_id, ret,
		     &rss_alg_sel, sizeof(rss_alg_sel));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_set_rxfh_rss_alg_sel(void *priv, u16 vsi_id, u8 rss_alg_sel)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->set_rss_alg_sel,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, rss_alg_sel));
	return ret;
}

static int nbl_disp_chan_set_rxfh_rss_alg_sel_req(void *priv, u16 vsi_id, u8 rss_alg_sel)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_rxfh_rss_alg_sel param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	param.rss_alg_sel = rss_alg_sel;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_SET_RXFH_RSS_ALG_SEL, &param,
		      sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_rxfh_rss_alg_sel_resp(void *priv, u16 src_id, u16 msg_id,
						    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_rxfh_rss_alg_sel *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_set_rxfh_rss_alg_sel *)data;

	err = NBL_OPS_CALL(res_ops->set_rss_alg_sel,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			    param->vsi_id, param->rss_alg_sel));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_RXFH_RSS_ALG_SEL, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_get_phy_caps_req(void *priv, u8 eth_id, struct nbl_phy_caps *phy_caps)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_PHY_CAPS, &eth_id,
		      sizeof(eth_id), phy_caps, sizeof(*phy_caps), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_phy_caps_resp(void *priv, u16 src_id, u16 msg_id,
					    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	struct nbl_phy_caps phy_caps = { 0 };
	u8 eth_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	eth_id = *(u8 *)data;

	NBL_OPS_CALL(res_ops->get_phy_caps,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, &phy_caps));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_PHY_CAPS, msg_id, ret,
		     &phy_caps, sizeof(phy_caps));

	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_set_sfp_state_req(void *priv, u8 eth_id, u8 state)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_sfp_state param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.eth_id = eth_id;
	param.state = state;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SET_SFP_STATE, &param,
		      sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_sfp_state_resp(void *priv, u16 src_id, u16 msg_id,
					     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_set_sfp_state *param;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_set_sfp_state *)data;

	ret = NBL_OPS_CALL(res_ops->set_sfp_state,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->eth_id, param->state));
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "set sfp state failed with ret: %d\n", ret);
	}

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_SFP_STATE, msg_id, err, NULL, 0);

	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_SET_SFP_STATE, src_id);
}

static void nbl_disp_chan_register_rdma_req(void *priv, u16 vsi_id,
					    struct nbl_rdma_register_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_REGISTER_RDMA,
		      &vsi_id, sizeof(vsi_id), param, sizeof(*param), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_register_rdma_resp(void *priv, u16 src_id, u16 msg_id,
					     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_rdma_register_param result = {0};
	struct nbl_chan_ack_info chan_ack;
	u16 *vsi_id;
	int ret = NBL_CHAN_RESP_OK;

	vsi_id = (u16 *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->register_rdma,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *vsi_id, &result);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REGISTER_RDMA,
		     msg_id, ret, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_unregister_rdma_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_UNREGISTER_RDMA, &vsi_id, sizeof(vsi_id),
		      NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_unregister_rdma_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	u16 *vsi_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	vsi_id = (u16 *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->unregister_rdma,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *vsi_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_UNREGISTER_RDMA, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static u64 nbl_disp_chan_get_real_hw_addr_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;
	u64 addr = 0;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_GET_REAL_HW_ADDR, &vsi_id,
		      sizeof(vsi_id), &addr, sizeof(addr), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return addr;
}

static void nbl_disp_chan_get_real_hw_addr_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	u16 vsi_id;
	u64 addr;

	vsi_id = *(u16 *)data;
	addr = NBL_OPS_CALL(res_ops->get_real_hw_addr,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_REAL_HW_ADDR, msg_id,
		     ret, &addr, sizeof(addr));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static u16 nbl_disp_chan_get_function_id_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;
	u16 func_id = 0;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_GET_FUNCTION_ID, &vsi_id,
		      sizeof(vsi_id), &func_id, sizeof(func_id), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return func_id;
}

static void nbl_disp_chan_get_function_id_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	u16 vsi_id, func_id;

	vsi_id = *(u16 *)data;

	func_id = NBL_OPS_CALL(res_ops->get_function_id,
			       (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_FUNCTION_ID, msg_id,
		     ret, &func_id, sizeof(func_id));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_get_real_bdf_req(void *priv, u16 vsi_id, u8 *bus, u8 *dev, u8 *function)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_result_get_real_bdf result = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_REAL_BDF, &vsi_id,
		      sizeof(vsi_id), &result, sizeof(result), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	*bus = result.bus;
	*dev = result.dev;
	*function = result.function;
}

static void nbl_disp_chan_get_real_bdf_resp(void *priv, u16 src_id, u16 msg_id,
					    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_result_get_real_bdf result = {0};
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	u16 vsi_id;

	vsi_id = *(u16 *)data;
	NBL_OPS_CALL(res_ops->get_real_bdf,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id,
		      &result.bus, &result.dev, &result.function));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_REAL_BDF, msg_id,
		     ret, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_get_mbx_irq_num_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;
	int result = 0;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_MBX_IRQ_NUM, NULL, 0,
		      &result, sizeof(result), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return result;
}

static void nbl_disp_chan_get_mbx_irq_num_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int result, ret = NBL_CHAN_RESP_OK;

	result = NBL_OPS_CALL(res_ops->get_mbx_irq_num, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_MBX_IRQ_NUM, msg_id,
		     ret, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_clear_accel_flow_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_CLEAR_ACCEL_FLOW, &vsi_id, sizeof(vsi_id),
		      NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_clear_accel_flow_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	u16 *vsi_id = (u16 *)data;

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->clear_accel_flow,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *vsi_id);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CLEAR_ACCEL_FLOW, msg_id,
		     NBL_CHAN_RESP_OK, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_clear_flow_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_CLEAR_FLOW, &vsi_id, sizeof(vsi_id),
		      NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_clear_flow_resp(void *priv, u16 src_id, u16 msg_id,
					  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	u16 *vsi_id = (u16 *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->clear_flow,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *vsi_id);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CLEAR_FLOW, msg_id,
		     NBL_CHAN_RESP_OK, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_clear_queues_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_CLEAR_QUEUE, &vsi_id,
		      sizeof(vsi_id), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_clear_queues_resp(void *priv, u16 src_id, u16 msg_id,
					    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	u16 *vsi_id = (u16 *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->clear_queues,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *vsi_id);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CLEAR_QUEUE, msg_id,
		     NBL_CHAN_RESP_OK, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_disable_phy_flow_req(void *priv, u8 eth_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_DISABLE_PHY_FLOW, &eth_id,
		      sizeof(eth_id), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_disable_phy_flow_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	u8 *eth_id = (u8 *)data;
	int err = NBL_CHAN_RESP_OK;
	int ret;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->disable_phy_flow,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *eth_id);
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "disp disable phy flow resp failed with ret: %d\n", ret);
	}

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_DISABLE_PHY_FLOW, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "disp chan send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_DISABLE_PHY_FLOW, src_id);
}

static int nbl_disp_chan_enable_phy_flow_req(void *priv, u8 eth_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_ENABLE_PHY_FLOW, &eth_id,
		      sizeof(eth_id), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_enable_phy_flow_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	u8 *eth_id = (u8 *)data;
	int err = NBL_CHAN_RESP_OK;
	int ret;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->enable_phy_flow,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *eth_id);
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "disp enable phy flow resp failed with ret: %d\n", ret);
	}
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ENABLE_PHY_FLOW, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "disp chan send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_ENABLE_PHY_FLOW, src_id);
}

static void nbl_disp_chan_init_acl_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_INIT_ACL, NULL, 0, NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_init_acl_resp(void *priv, u16 src_id, u16 msg_id,
					void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->init_acl, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_INIT_ACL, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_uninit_acl_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_UNINIT_ACL, NULL, 0, NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_uninit_acl_resp(void *priv, u16 src_id, u16 msg_id,
					  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->uninit_acl, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_UNINIT_ACL, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_set_upcall_rule_req(void *priv, u8 eth_id, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_set_upcall param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.eth_id = eth_id;
	param.vsi_id = vsi_id;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SET_UPCALL_RULE,
		      &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_upcall_rule_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_set_upcall *param;
	int err = NBL_CHAN_RESP_OK;
	int ret;

	param = (struct nbl_chan_param_set_upcall *)data;

	ret = NBL_OPS_CALL(res_ops->set_upcall_rule,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->eth_id, param->vsi_id));
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "disp set upcall rule resp failed with ret: %d\n", ret);
	}

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_UPCALL_RULE, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "disp chan send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_SET_UPCALL_RULE, src_id);
}

static int nbl_disp_chan_unset_upcall_rule_req(void *priv, u8 eth_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_UNSET_UPCALL_RULE,
		      &eth_id, sizeof(eth_id), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_unset_upcall_rule_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	u8 *eth_id = (u8 *)data;
	int err = NBL_CHAN_RESP_OK;
	int ret;

	ret = NBL_OPS_CALL(res_ops->unset_upcall_rule,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *eth_id));
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "disp unset upcall rule resp failed with ret: %d\n", ret);
	}
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_UNSET_UPCALL_RULE, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "disp chan send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_UNSET_UPCALL_RULE, src_id);
}

static void nbl_disp_chan_set_shaping_dport_vld_req(void *priv, u8 eth_id, bool vld)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_set_func_vld param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.eth_id = eth_id;
	param.vld = vld;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_SET_SHAPING_DPORT_VLD,
		      &param, sizeof(param), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_shaping_dport_vld_resp(void *priv, u16 src_id, u16 msg_id,
						     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_set_func_vld *param;
	int err = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_set_func_vld *)data;

	NBL_OPS_CALL(res_ops->set_shaping_dport_vld,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->eth_id, param->vld));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_SHAPING_DPORT_VLD, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_set_dport_fc_th_vld_req(void *priv, u8 eth_id, bool vld)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_set_func_vld param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.eth_id = eth_id;
	param.vld = vld;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_SET_DPORT_FC_TH_VLD,
		      &param, sizeof(param), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_dport_fc_th_vld_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_set_func_vld *param;
	int err = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_set_func_vld *)data;

	NBL_OPS_CALL(res_ops->set_dport_fc_th_vld,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->eth_id, param->vld));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_DPORT_FC_TH_VLD, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static u16 nbl_disp_chan_get_vsi_id_req(void *priv, u16 func_id, u16 type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_get_vsi_id param = {0};
	struct nbl_chan_param_get_vsi_id result = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.type = type;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_VSI_ID, &param,
		      sizeof(param), &result, sizeof(result), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return result.vsi_id;
}

static void nbl_disp_chan_get_vsi_id_resp(void *priv, u16 src_id, u16 msg_id,
					  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_get_vsi_id *param;
	struct nbl_chan_param_get_vsi_id result;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	param = (struct nbl_chan_param_get_vsi_id *)data;

	result.vsi_id = NBL_OPS_CALL(res_ops->get_vsi_id,
				     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id, param->type));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_VSI_ID,
		     msg_id, err, &result, sizeof(result));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_GET_VSI_ID);
}

static void
nbl_disp_chan_get_eth_id_req(void *priv, u16 vsi_id, u8 *eth_mode, u8 *eth_id, u8 *logic_eth_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_get_eth_id param = {0};
	struct nbl_chan_param_get_eth_id result = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_ETH_ID,  &param,
		      sizeof(param), &result, sizeof(result), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	*eth_mode = result.eth_mode;
	*eth_id = result.eth_id;
	*logic_eth_id = result.logic_eth_id;
}

static void nbl_disp_chan_get_eth_id_resp(void *priv, u16 src_id, u16 msg_id,
					  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_get_eth_id *param;
	struct nbl_chan_param_get_eth_id result = {0};
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	param = (struct nbl_chan_param_get_eth_id *)data;

	NBL_OPS_CALL(res_ops->get_eth_id, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id,
					   &result.eth_mode, &result.eth_id, &result.logic_eth_id));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_ETH_ID,
		     msg_id, err, &result, sizeof(result));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_GET_ETH_ID);
}

static int nbl_disp_alloc_rings(void *priv, struct net_device *netdev,
				struct nbl_ring_param *ring_param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->alloc_rings,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), netdev, ring_param));
	return ret;
}

static void nbl_disp_remove_rings(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	if (!disp_mgt)
		return;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->remove_rings, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static dma_addr_t nbl_disp_start_tx_ring(void *priv, u8 ring_index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	dma_addr_t addr = 0;

	if (!disp_mgt)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	addr = NBL_OPS_CALL(res_ops->start_tx_ring,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ring_index));
	return addr;
}

static void nbl_disp_stop_tx_ring(void *priv, u8 ring_index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	if (!disp_mgt)
		return;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->stop_tx_ring, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ring_index));
}

static dma_addr_t nbl_disp_start_rx_ring(void *priv, u8 ring_index, bool use_napi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	dma_addr_t addr = 0;

	if (!disp_mgt)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	addr = NBL_OPS_CALL(res_ops->start_rx_ring,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ring_index, use_napi));

	return addr;
}

static void nbl_disp_stop_rx_ring(void *priv, u8 ring_index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	if (!disp_mgt)
		return;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->stop_rx_ring, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ring_index));
}

static void nbl_disp_kick_rx_ring(void *priv, u16 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->kick_rx_ring, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index));
}

static int nbl_disp_dump_ring(void *priv, struct seq_file *m, bool is_tx, int index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->dump_ring,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), m, is_tx, index));
	return ret;
}

static int nbl_disp_dump_ring_stats(void *priv, struct seq_file *m, bool is_tx, int index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->dump_ring_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), m, is_tx, index));
	return ret;
}

static void nbl_disp_set_rings_xdp_prog(void *priv, void *prog)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->set_rings_xdp_prog,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), prog));
}

static int nbl_disp_register_xdp_rxq(void *priv, u8 ring_index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->register_xdp_rxq,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ring_index));
	return ret;
}

static void nbl_disp_unregister_xdp_rxq(void *priv, u8 ring_index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->unregister_xdp_rxq, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ring_index));
}

static struct nbl_napi_struct *nbl_disp_get_vector_napi(void *priv, u16 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL(res_ops->get_vector_napi,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index));
}

static void nbl_disp_set_vector_info(void *priv, u8 *irq_enable_base,
				     u32 irq_data, u16 index, bool mask_en)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->set_vector_info,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
		      irq_enable_base, irq_data, index, mask_en));
}

static void nbl_disp_register_vsi_ring(void *priv, u16 vsi_index, u16 ring_offset, u16 ring_num)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->register_vsi_ring,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_index, ring_offset, ring_num));
}

static void nbl_disp_get_res_pt_ops(void *priv, struct nbl_resource_pt_ops *pt_ops)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->get_resource_pt_ops,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), pt_ops));
}

static int nbl_disp_register_net(void *priv, struct nbl_register_net_param *register_param,
				 struct nbl_register_net_result *register_result)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->register_net,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), 0,
				register_param, register_result);
	return ret;
}

static int nbl_disp_alloc_txrx_queues(void *priv, u16 vsi_id, u16 queue_num)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->alloc_txrx_queues,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, queue_num);
	return ret;
}

static void nbl_disp_free_txrx_queues(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->free_txrx_queues,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static int nbl_disp_register_vsi2q(void *priv, u16 vsi_index, u16 vsi_id,
				   u16 queue_offset, u16 queue_num)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->register_vsi2q,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_index, vsi_id,
				 queue_offset, queue_num);
}

static int nbl_disp_setup_q2vsi(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->setup_q2vsi,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static void nbl_disp_remove_q2vsi(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_q2vsi,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static int nbl_disp_setup_rss(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->setup_rss,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static void nbl_disp_remove_rss(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_rss,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static int nbl_disp_setup_queue(void *priv, struct nbl_txrx_queue_param *param, bool is_tx)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->setup_queue,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param, is_tx);
	return ret;
}

static void nbl_disp_remove_all_queues(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_all_queues,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static int nbl_disp_remove_queue(void *priv, struct nbl_txrx_queue_param *param, bool is_tx)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_queue,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param, is_tx);
}

static int nbl_disp_cfg_dsch(void *priv, u16 vsi_id, bool vld)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->cfg_dsch,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, vld);
	return ret;
}

static void nbl_disp_remove_cqs(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_cqs,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static u8 *nbl_disp_get_msix_irq_enable_info(void *priv, u16 global_vector_id, u32 *irq_data)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	if (!disp_mgt)
		return NULL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL(res_ops->get_msix_irq_enable_info,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), global_vector_id, irq_data));
}

static int nbl_disp_add_macvlan(void *priv, u8 *mac, u16 vlan, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	if (!disp_mgt || !mac)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->add_macvlan,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), mac, vlan, vsi);
	return ret;
}

static void nbl_disp_del_macvlan(void *priv, u8 *mac, u16 vlan, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	if (!disp_mgt || !mac)
		return;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->del_macvlan,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), mac, vlan, vsi);
}

static int nbl_disp_add_multi_rule(void *priv, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	u8 broadcast_mac[ETH_ALEN];
	int ret = 0;

	memset(broadcast_mac, 0xFF, ETH_ALEN);
	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->add_macvlan,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), broadcast_mac, 0, vsi);

	return ret;
}

static void nbl_disp_del_multi_rule(void *priv, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	u8 broadcast_mac[ETH_ALEN];

	memset(broadcast_mac, 0xFF, ETH_ALEN);
	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->del_macvlan,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), broadcast_mac, 0, vsi);
}

static int nbl_disp_setup_multi_group(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->setup_multi_group,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
}

static void nbl_disp_remove_multi_group(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_multi_group,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
}

static void nbl_disp_get_net_stats(void *priv, struct nbl_stats *net_stats)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->get_net_stats, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), net_stats));
}

static void nbl_disp_get_private_stat_len(void *priv, u32 *len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_private_stat_len,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), len);
}

static int nbl_disp_get_pause_stats(void *priv, u32 eth_id,
				    struct nbl_pause_stats *pause_stats, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_pause_stats,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, pause_stats);
}

static int nbl_disp_chan_get_pause_stats_req(void *priv, u32 eth_id,
					     struct nbl_pause_stats *pause_stats, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_PAUSE_STATS, &eth_id,
		      sizeof(eth_id), pause_stats, data_len, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_pause_stats_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_pause_stats pause_stats = {0};
	u32 *param = (u32 *)(data);
	int err = NBL_CHAN_RESP_OK;
	int ret;

	ret = NBL_OPS_CALL(res_ops->get_pause_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *param, &pause_stats));
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "disp get eth pause stats failed with ret: %d\n", ret);
	}
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_PAUSE_STATS, msg_id,
		     ret, &pause_stats, sizeof(struct nbl_pause_stats));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "disp chan send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_GET_PAUSE_STATS, src_id);
}

static void nbl_disp_get_private_stat_data(void *priv, u32 eth_id, u64 *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_private_stat_data,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, data, data_len);
}

static void nbl_disp_get_private_stat_data_req(void *priv, u32 eth_id, u64 *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_get_private_stat_data param = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.eth_id = eth_id;
	param.data_len = data_len;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_ETH_STATS, &param,
		      sizeof(param), data, data_len, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_private_stat_data_resp(void *priv, u16 src_id, u16 msg_id,
						     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_get_private_stat_data *param;
	struct nbl_chan_ack_info chan_ack;
	u64 *recv_data;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_get_private_stat_data *)data;
	recv_data = kmalloc(param->data_len, GFP_ATOMIC);
	if (!recv_data) {
		dev_err(dev, "Allocate memory to private_stat_data failed\n");
		return;
	}

	NBL_OPS_CALL(res_ops->get_private_stat_data,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->eth_id,
		     recv_data, param->data_len));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_ETH_STATS, msg_id,
		     ret, recv_data, param->data_len);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);

	kfree(recv_data);
}

static int nbl_disp_get_eth_ctrl_stats(void *priv, u32 eth_id,
				       struct nbl_eth_ctrl_stats *eth_ctrl_stats,
				       u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_eth_ctrl_stats,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id,
				 eth_ctrl_stats);
}

static int nbl_disp_chan_get_eth_ctrl_stats_req(void *priv, u32 eth_id,
						struct nbl_eth_ctrl_stats *eth_ctrl_stats,
						u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_ETH_CTRL_STATS, &eth_id,
		      sizeof(eth_id), eth_ctrl_stats, data_len, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_eth_ctrl_stats_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_eth_ctrl_stats eth_ctrl_stats = {0};
	struct nbl_chan_ack_info chan_ack;
	u32 *param = (u32 *)(data);
	int err = NBL_CHAN_RESP_OK;
	int ret;

	ret = NBL_OPS_CALL(res_ops->get_eth_ctrl_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *param, &eth_ctrl_stats));
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "disp get eth ctrl stats failed with ret: %d\n", ret);
	}
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_ETH_CTRL_STATS, msg_id,
		     ret, &eth_ctrl_stats, sizeof(struct nbl_eth_ctrl_stats));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "disp chan send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_GET_ETH_CTRL_STATS, src_id);
}

static int nbl_disp_get_eth_mac_stats(void *priv, u32 eth_id,
				      struct nbl_eth_mac_stats *eth_mac_stats, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_eth_mac_stats,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, eth_mac_stats);
}

static int nbl_disp_chan_get_eth_mac_stats_req(void *priv, u32 eth_id,
					       struct nbl_eth_mac_stats *eth_mac_stats,
					       u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_ETH_MAC_STATS, &eth_id,
		      sizeof(eth_id), eth_mac_stats, data_len, 1);

	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_eth_mac_stats_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_eth_mac_stats eth_mac_stats = {0};
	u32 *param = (u32 *)(data);
	int err = NBL_CHAN_RESP_OK;
	int ret;

	ret = NBL_OPS_CALL(res_ops->get_eth_mac_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *param, &eth_mac_stats));
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "disp get eth mac stats failed with ret: %d\n", ret);
	}
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_ETH_MAC_STATS, msg_id,
		     ret, &eth_mac_stats, sizeof(struct nbl_eth_mac_stats));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "disp chan send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_GET_ETH_MAC_STATS, src_id);
}

static int nbl_disp_get_rmon_stats(void *priv, u32 eth_id,
				   struct nbl_rmon_stats *rmon_stats, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_rmon_stats,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, rmon_stats);
}

static int nbl_disp_chan_get_rmon_stats_req(void *priv, u32 eth_id,
					    struct nbl_rmon_stats *rmon_stats,
					       u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_RMON_STATS, &eth_id,
		      sizeof(eth_id), rmon_stats, data_len, 1);

	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_rmon_stats_resp(void *priv, u16 src_id, u16 msg_id,
					      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_rmon_stats rmon_stats = {0};
	u32 *param = (u32 *)(data);
	int err = NBL_CHAN_RESP_OK;
	int ret;

	ret = NBL_OPS_CALL(res_ops->get_rmon_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *param, &rmon_stats));
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "disp get eth mac stats failed with ret: %d\n", ret);
	}
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_RMON_STATS, msg_id,
		     ret, &rmon_stats, sizeof(struct nbl_rmon_stats));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "disp chan send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_GET_RMON_STATS, src_id);
}

static void nbl_disp_fill_private_stat_strings(void *priv, u8 *strings)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->fill_private_stat_strings,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), strings);
}

static u16 nbl_disp_get_max_desc_num(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	u16 ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_max_desc_num, ());
	return ret;
}

static u16 nbl_disp_get_min_desc_num(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	u16 ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_min_desc_num, ());
	return ret;
}

static int nbl_disp_cfg_qdisc_mqprio(void *priv, struct nbl_tc_qidsc_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->cfg_qdisc_mqprio,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param);
	return ret;
}

static int nbl_disp_set_spoof_check_addr(void *priv, u16 vsi_id, u8 *mac)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_spoof_check_addr,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, mac);
	return ret;
}

static int nbl_disp_set_vf_spoof_check(void *priv, u16 vsi_id, int vf_id, u8 enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_vf_spoof_check,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, vf_id, enable);
	return ret;
}

static void nbl_disp_get_base_mac_addr(void *priv, u8 *mac)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_base_mac_addr,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), mac);
}

static u16 nbl_disp_get_tx_desc_num(void *priv, u32 ring_index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	u16 ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_tx_desc_num,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ring_index));
	return ret;
}

static u16 nbl_disp_get_rx_desc_num(void *priv, u32 ring_index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	u16 ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_rx_desc_num,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ring_index));
	return ret;
}

static void nbl_disp_set_tx_desc_num(void *priv, u32 ring_index, u16 desc_num)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->set_tx_desc_num,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ring_index, desc_num));
}

static void nbl_disp_set_rx_desc_num(void *priv, u32 ring_index, u16 desc_num)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->set_rx_desc_num,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ring_index, desc_num));
}

static void nbl_disp_cfg_txrx_vlan(void *priv, u16 vlan_tci, u16 vlan_proto, u8 vsi_index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->cfg_txrx_vlan,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vlan_tci, vlan_proto, vsi_index));
}

static void nbl_disp_get_rep_stats(void *priv, u16 rep_vsi_id,
				   struct nbl_rep_stats *rep_stats, bool is_tx)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->get_rep_stats,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), rep_vsi_id, rep_stats, is_tx));
}

static u16 nbl_disp_get_rep_index(void *priv, u16 rep_vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL(res_ops->get_rep_index,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), rep_vsi_id));
}

static void nbl_disp_get_queue_stats(void *priv, u8 queue_id,
				     struct nbl_queue_stats *queue_stats, bool is_tx)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->get_queue_stats,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), queue_id, queue_stats, is_tx));
}

static void nbl_disp_get_firmware_version(void *priv, char *firmware_verion, u8 max_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_firmware_version,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), firmware_verion));
	if (ret)
		dev_err(dev, "get emp version failed with ret: %d\n", ret);
}

static int nbl_disp_get_driver_info(void *priv, struct nbl_driver_info *driver_info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_driver_info,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), driver_info));
}

static void nbl_disp_get_coalesce(void *priv, u16 vector_id,
				  struct nbl_chan_param_get_coalesce *ec)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->get_coalesce,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), 0, vector_id, ec));
}

static void nbl_disp_set_coalesce(void *priv, u16 vector_id, u16 vector_num, u16 pnum, u16 rate)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_coalesce,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), 0, vector_id,
			  vector_num, pnum, rate);
}

static void nbl_disp_get_rxfh_indir_size(void *priv, u16 vsi_id, u32 *rxfh_indir_size)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->get_rxfh_indir_size,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, rxfh_indir_size));
}

static void nbl_disp_get_rxfh_rss_key_size(void *priv, u32 *rxfh_rss_key_size)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->get_rxfh_rss_key_size,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), rxfh_rss_key_size));
}

static void nbl_disp_get_rxfh_indir(void *priv, u16 vsi_id, u32 *indir, u32 indir_size)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->get_rxfh_indir, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, indir));
}

static int nbl_disp_set_rxfh_indir(void *priv, u16 vsi_id, const u32 *indir, u32 indir_size)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->set_rxfh_indir,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, indir, indir_size));
	return ret;
}

static int nbl_disp_chan_set_rxfh_indir_req(void *priv,
					    u16 vsi_id, const u32 *indir, u32 indir_size)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_rxfh_indir *param = NULL;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;
	int ret = 0;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param = kzalloc(sizeof(*param), GFP_KERNEL);
	if (!param)
		return -ENOMEM;

	param->vsi_id = vsi_id;
	param->indir_size = indir_size;
	memcpy(param->indir, indir, indir_size * sizeof(param->indir[0]));

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_SET_RXFH_INDIR, param,
		      sizeof(*param), NULL, 0, 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	kfree(param);
	return ret;
}

static void nbl_disp_chan_set_rxfh_indir_resp(void *priv, u16 src_id, u16 msg_id,
					      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_rxfh_indir *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_set_rxfh_indir *)data;

	err = NBL_OPS_CALL(res_ops->set_rxfh_indir,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			    param->vsi_id, param->indir, param->indir_size));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_RXFH_INDIR, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_get_rxfh_rss_key(void *priv, u8 *rss_key, u32 key_size)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->get_rxfh_rss_key, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), rss_key));
}

static void nbl_disp_get_rxfh_rss_alg_sel(void *priv, u16 vsi_id, u8 *alg_sel)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->get_rss_alg_sel,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, alg_sel));
}

static void nbl_disp_get_phy_caps(void *priv, u8 eth_id, struct nbl_phy_caps *phy_caps)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->get_phy_caps, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, phy_caps));
}

static int nbl_disp_set_sfp_state(void *priv, u8 eth_id, u8 state)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->set_sfp_state,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, state));
	return ret;
}

static int nbl_disp_init_chip_module(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	if (!disp_mgt)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->init_chip_module, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	return ret;
}

static int nbl_disp_queue_init(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	if (!disp_mgt)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->queue_init, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	return ret;
}

static int nbl_disp_vsi_init(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	if (!disp_mgt)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->vsi_init, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	return ret;
}

static int nbl_disp_configure_msix_map(void *priv, u16 num_net_msix, u16 num_others_msix,
				       bool net_msix_mask_en)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	if (!disp_mgt)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->configure_msix_map,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), 0, num_net_msix,
				num_others_msix, net_msix_mask_en);
	return ret;
}

static int nbl_disp_chan_configure_msix_map_req(void *priv, u16 num_net_msix, u16 num_others_msix,
						bool net_msix_mask_en)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_cfg_msix_map param = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	if (!disp_mgt)
		return -EINVAL;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.num_net_msix = num_net_msix;
	param.num_others_msix = num_others_msix;
	param.msix_mask_en = net_msix_mask_en;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_CONFIGURE_MSIX_MAP,
		      &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_configure_msix_map_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_cfg_msix_map *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_cfg_msix_map *)data;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->configure_msix_map,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id,
				param->num_net_msix, param->num_others_msix, param->msix_mask_en);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CONFIGURE_MSIX_MAP, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_CONFIGURE_MSIX_MAP);
}

static int nbl_disp_chan_destroy_msix_map_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	if (!disp_mgt)
		return -EINVAL;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_DESTROY_MSIX_MAP,
		      NULL, 0, NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_destroy_msix_map_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_cfg_msix_map *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_cfg_msix_map *)data;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->destroy_msix_map,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_DESTROY_MSIX_MAP, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_DESTROY_MSIX_MAP);
}

static int nbl_disp_chan_enable_mailbox_irq_req(void *priv, u16 vector_id, bool enable_msix)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_enable_mailbox_irq param = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	if (!disp_mgt)
		return -EINVAL;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vector_id = vector_id;
	param.enable_msix = enable_msix;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_MAILBOX_ENABLE_IRQ,
		      &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_enable_mailbox_irq_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_enable_mailbox_irq *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_enable_mailbox_irq *)data;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->enable_mailbox_irq,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id,
				param->vector_id, param->enable_msix);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_MAILBOX_ENABLE_IRQ, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_MAILBOX_ENABLE_IRQ);
}

static u16 nbl_disp_chan_get_global_vector_req(void *priv, u16 vsi_id, u16 local_vector_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_get_global_vector param = {0};
	struct nbl_chan_param_get_global_vector result = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	if (!disp_mgt)
		return -EINVAL;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	param.vector_id = local_vector_id;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_GLOBAL_VECTOR,  &param,
		      sizeof(param), &result, sizeof(result), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return result.vector_id;
}

static void nbl_disp_chan_get_global_vector_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_get_global_vector *param;
	struct nbl_chan_param_get_global_vector result;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_get_global_vector *)data;

	result.vector_id = NBL_OPS_CALL(res_ops->get_global_vector,
					(NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
					 param->vsi_id, param->vector_id));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_GLOBAL_VECTOR,
		     msg_id, err, &result, sizeof(result));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_GET_GLOBAL_VECTOR);
}

static int nbl_disp_destroy_msix_map(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	if (!disp_mgt)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->destroy_msix_map,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), 0);
	return ret;
}

static int nbl_disp_enable_mailbox_irq(void *priv, u16 vector_id, bool enable_msix)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	if (!disp_mgt)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->enable_mailbox_irq,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), 0, vector_id, enable_msix);
	return ret;
}

static int nbl_disp_enable_abnormal_irq(void *priv, u16 vector_id, bool enable_msix)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	if (!disp_mgt)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->enable_abnormal_irq,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vector_id, enable_msix));
	return ret;
}

static int nbl_disp_enable_adminq_irq(void *priv, u16 vector_id, bool enable_msix)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	if (!disp_mgt)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->enable_adminq_irq,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vector_id, enable_msix));
	return ret;
}

static u16 nbl_disp_get_global_vector(void *priv, u16 vsi_id, u16 local_vector_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	u16 ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->get_global_vector,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, local_vector_id));
	return ret;
}

static u16 nbl_disp_get_msix_entry_id(void *priv, u16 vsi_id, u16 local_vector_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	u16 ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->get_msix_entry_id,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, local_vector_id));
	return ret;
}

static void nbl_disp_dump_flow(void *priv, struct seq_file *m)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->dump_flow, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), m);
}

static u16 nbl_disp_get_vsi_id(void *priv, u16 func_id, u16 type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	if (!disp_mgt)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL(res_ops->get_vsi_id, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			    func_id, type));
}

static void nbl_disp_get_eth_id(void *priv, u16 vsi_id, u8 *eth_mode, u8 *eth_id, u8 *logic_eth_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->get_eth_id, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
					   vsi_id, eth_mode, eth_id, logic_eth_id));
}

static void nbl_disp_get_rep_feature(void *priv,
				     struct nbl_register_net_result *register_result)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->get_rep_feature,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), register_result));
}

static void nbl_disp_set_eswitch_mode(void *priv, u16 eswitch_mode)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->set_eswitch_mode,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eswitch_mode));
}

static u16 nbl_disp_get_eswitch_mode(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	u16 ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->get_eswitch_mode, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	return ret;
}

static int nbl_disp_alloc_rep_data(void *priv, int num_vfs, u16 vf_base_vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL(res_ops->alloc_rep_data,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), num_vfs, vf_base_vsi_id));
}

static void nbl_disp_free_rep_data(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->free_rep_data, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static void nbl_disp_set_rep_netdev_info(void *priv, void *rep_data)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->set_rep_netdev_info,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), rep_data));
}

static void nbl_disp_unset_rep_netdev_info(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->unset_rep_netdev_info, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static struct net_device *nbl_disp_get_rep_netdev_info(void *priv, u16 rep_data_index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL(res_ops->get_rep_netdev_info, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			    rep_data_index));
}

static int nbl_disp_enable_lag_protocol(void *priv, u16 eth_id, bool lag_en)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->enable_lag_protocol,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, lag_en));
}

static int nbl_disp_chan_cfg_lag_hash_algorithm_req(void *priv, u16 eth_id, u16 lag_id,
						    enum netdev_lag_hash hash_type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_cfg_lag_hash_algorithm param = {0};
	struct nbl_chan_send_info chan_send;

	param.eth_id = eth_id;
	param.lag_id = lag_id;
	param.hash_type = hash_type;

	NBL_CHAN_SEND(chan_send, 0, NBL_CHAN_MSG_CFG_LAG_HASH_ALGORITHM, &param, sizeof(param),
		      NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_lag_hash_algorithm_resp(void *priv, u16 src_id, u16 msg_id,
						      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_cfg_lag_hash_algorithm *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	param = (struct nbl_chan_param_cfg_lag_hash_algorithm *)data;

	ret = NBL_OPS_CALL(res_ops->cfg_lag_hash_algorithm,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			    param->eth_id, param->lag_id, param->hash_type));
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_LAG_HASH_ALGORITHM, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_CFG_LAG_HASH_ALGORITHM);
}

static int nbl_disp_cfg_lag_hash_algorithm(void *priv, u16 eth_id, u16 lag_id,
					   enum netdev_lag_hash hash_type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->cfg_lag_hash_algorithm,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, lag_id, hash_type));
}

static int nbl_disp_chan_cfg_lag_member_fwd_req(void *priv, u16 eth_id, u16 lag_id, u8 fwd)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_cfg_lag_member_fwd param = {0};
	struct nbl_chan_send_info chan_send;

	param.eth_id = eth_id;
	param.lag_id = lag_id;
	param.fwd = fwd;

	NBL_CHAN_SEND(chan_send, 0, NBL_CHAN_MSG_CFG_LAG_MEMBER_FWD, &param, sizeof(param),
		      NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_lag_member_fwd_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_cfg_lag_member_fwd *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	param = (struct nbl_chan_param_cfg_lag_member_fwd *)data;

	ret = NBL_OPS_CALL(res_ops->cfg_lag_member_fwd,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			    param->eth_id, param->lag_id, param->fwd));
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_LAG_MEMBER_FWD, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_CFG_LAG_MEMBER_FWD);
}

static int nbl_disp_cfg_lag_member_fwd(void *priv, u16 eth_id, u16 lag_id, u8 fwd)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->cfg_lag_member_fwd,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, lag_id, fwd));
}

static int nbl_disp_chan_cfg_lag_member_list_req(void *priv,
						 struct nbl_lag_member_list_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_lag_member_list_param chan_param = {0};
	struct nbl_chan_send_info chan_send;

	memcpy(&chan_param, param, sizeof(chan_param));

	NBL_CHAN_SEND(chan_send, 0, NBL_CHAN_MSG_CFG_LAG_MEMBER_LIST, &chan_param,
		      sizeof(chan_param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_lag_member_list_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_lag_member_list_param *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	param = (struct nbl_lag_member_list_param *)data;

	ret = NBL_OPS_CALL(res_ops->cfg_lag_member_list,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param));
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_LAG_MEMBER_LIST, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_CFG_LAG_MEMBER_LIST);
}

static int nbl_disp_cfg_lag_member_list(void *priv, struct nbl_lag_member_list_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->cfg_lag_member_list,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param));
}

static int nbl_disp_chan_cfg_lag_member_up_attr_req(void *priv, u16 eth_id, u16 lag_id, bool enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_cfg_lag_member_up_attr param = {0};
	struct nbl_chan_send_info chan_send;

	param.eth_id = eth_id;
	param.eth_id = enable;
	param.enable = enable;

	NBL_CHAN_SEND(chan_send, 0, NBL_CHAN_MSG_CFG_LAG_MEMBER_UP_ATTR, &param, sizeof(param),
		      NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_lag_member_up_attr_resp(void *priv, u16 src_id, u16 msg_id,
						      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_cfg_lag_member_up_attr *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	param = (struct nbl_chan_param_cfg_lag_member_up_attr *)data;

	ret = NBL_OPS_CALL(res_ops->cfg_lag_member_up_attr,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->eth_id, param->lag_id,
			    param->enable));
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_LAG_MEMBER_UP_ATTR, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_CFG_LAG_MEMBER_UP_ATTR);
}

static int nbl_disp_cfg_lag_member_up_attr(void *priv, u16 eth_id, u16 lag_id, bool enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->cfg_lag_member_up_attr,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, lag_id, enable));
}

static int nbl_disp_chan_add_lag_flow_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send;

	NBL_CHAN_SEND(chan_send, 0, NBL_CHAN_MSG_ADD_LAG_FLOW, &vsi_id, sizeof(vsi_id),
		      NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_add_lag_flow_resp(void *priv, u16 src_id, u16 msg_id,
					    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->add_lag_flow,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *(u16 *)data);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ADD_LAG_FLOW, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_ADD_LAG_FLOW);
}

static int nbl_disp_add_lag_flow(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->add_lag_flow,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static void nbl_disp_chan_del_lag_flow_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send;

	NBL_CHAN_SEND(chan_send, 0, NBL_CHAN_MSG_DEL_LAG_FLOW, &vsi_id, sizeof(vsi_id),
		      NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_del_lag_flow_resp(void *priv, u16 src_id, u16 msg_id,
					    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->del_lag_flow, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			  *(u16 *)data);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_DEL_LAG_FLOW, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_DEL_LAG_FLOW);
}

static void nbl_disp_del_lag_flow(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->del_lag_flow,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static int nbl_disp_chan_add_lldp_flow_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send;

	NBL_CHAN_SEND(chan_send, 0, NBL_CHAN_MSG_ADD_LLDP_FLOW, &vsi_id, sizeof(vsi_id),
		      NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_add_lldp_flow_resp(void *priv, u16 src_id, u16 msg_id,
					     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->add_lldp_flow,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *(u16 *)data);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ADD_LLDP_FLOW, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_ADD_LLDP_FLOW);
}

static int nbl_disp_add_lldp_flow(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->add_lldp_flow,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static void nbl_disp_chan_del_lldp_flow_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send;

	NBL_CHAN_SEND(chan_send, 0, NBL_CHAN_MSG_DEL_LLDP_FLOW, &vsi_id, sizeof(vsi_id),
		      NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_del_lldp_flow_resp(void *priv, u16 src_id, u16 msg_id,
					     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->del_lldp_flow, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			  *(u16 *)data);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_DEL_LLDP_FLOW, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_DEL_LLDP_FLOW);
}

static void nbl_disp_del_lldp_flow(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->del_lldp_flow,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static int nbl_disp_cfg_duppkt_info(void *priv, struct nbl_lag_member_list_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->cfg_duppkt_info, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param));
}

static int nbl_disp_chan_cfg_duppkt_mcc_req(void *priv, struct nbl_lag_member_list_param *mem_param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_lag_member_list_param param = {0};
	struct nbl_chan_send_info chan_send;

	memcpy(&param, mem_param, sizeof(param));

	NBL_CHAN_SEND(chan_send, 0, NBL_CHAN_MSG_CFG_DUPPKT_MCC, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_duppkt_mcc_resp(void *priv, u16 src_id, u16 msg_id,
					      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_lag_member_list_param *param = NULL;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	param = (struct nbl_lag_member_list_param *)data;

	err = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->cfg_duppkt_mcc,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_DUPPKT_MCC, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_CFG_DUPPKT_MCC);
}

static int nbl_disp_cfg_duppkt_mcc(void *priv, struct nbl_lag_member_list_param *mem_param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->cfg_duppkt_mcc,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), mem_param);
}

static int nbl_disp_chan_cfg_bond_shaping_req(void *priv, u8 eth_id, bool enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_cfg_bond_shaping param = {0};
	struct nbl_chan_send_info chan_send;

	param.eth_id = eth_id;
	param.enable = enable;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_CFG_BOND_SHAPING,
		      &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_bond_shaping_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(NBL_DISP_MGT_TO_COMMON(disp_mgt));
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_cfg_bond_shaping *param = NULL;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	param = (struct nbl_chan_param_cfg_bond_shaping *)data;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->cfg_bond_shaping,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->eth_id, param->enable);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_BOND_SHAPING, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_CFG_BOND_SHAPING);
}

static int nbl_disp_cfg_bond_shaping(void *priv, u8 eth_id, bool enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->cfg_bond_shaping,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, enable);
}

static void nbl_disp_chan_cfg_bgid_back_pressure_req(void *priv, u8 main_eth_id, u8 other_eth_id,
						     bool enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_cfg_bgid_back_pressure param = {0};
	struct nbl_chan_send_info chan_send;

	param.main_eth_id = main_eth_id;
	param.other_eth_id = other_eth_id;
	param.enable = enable;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_CFG_BGID_BACK_PRESSURE,
		      &param, sizeof(param), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_bgid_back_pressure_resp(void *priv, u16 src_id, u16 msg_id,
						      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(NBL_DISP_MGT_TO_COMMON(disp_mgt));
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_cfg_bgid_back_pressure *param = NULL;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	param = (struct nbl_chan_param_cfg_bgid_back_pressure *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->cfg_bgid_back_pressure,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->main_eth_id,
			  param->other_eth_id, param->enable);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_BGID_BACK_PRESSURE, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_CFG_BGID_BACK_PRESSURE);
}

static void nbl_disp_cfg_bgid_back_pressure(void *priv, u8 main_eth_id, u8 other_eth_id,
					    bool enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->cfg_bgid_back_pressure,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), main_eth_id, other_eth_id, enable);
}

static u32 nbl_disp_get_tx_headroom(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	u32 ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_tx_headroom, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	return ret;
}

static void nbl_disp_register_rdma(void *priv, u16 vsi_id, struct nbl_rdma_register_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->register_rdma,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, param);
}

static void nbl_disp_unregister_rdma(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->unregister_rdma,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static u8 __iomem *nbl_disp_get_hw_addr(void *priv, size_t *size)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	u8 __iomem *addr = NULL;

	addr = NBL_OPS_CALL(res_ops->get_hw_addr, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), size));
	return addr;
}

static u64 nbl_disp_get_real_hw_addr(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	u64 ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_real_hw_addr,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id));
	return ret;
}

static u16 nbl_disp_get_function_id(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	u16 ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_function_id, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id));
	return ret;
}

static void nbl_disp_get_real_bdf(void *priv, u16 vsi_id, u8 *bus, u8 *dev, u8 *function)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->get_real_bdf,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, bus, dev, function));
}

static bool nbl_disp_check_fw_heartbeat(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = false;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->check_fw_heartbeat, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	return ret;
}

static bool nbl_disp_check_fw_reset(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL(res_ops->check_fw_reset, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_flash_lock(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->flash_lock, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_flash_unlock(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->flash_unlock, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_flash_prepare(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->flash_prepare, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_flash_image(void *priv, u32 module, const u8 *data, size_t len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->flash_image,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), module, data, len));
}

static int nbl_disp_flash_activate(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->flash_activate, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_set_eth_loopback(void *priv, u8 enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	u8 eth_id = NBL_DISP_MGT_TO_COMMON(disp_mgt)->eth_id;

	return NBL_OPS_CALL(res_ops->setup_loopback,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, enable));
}

static int nbl_disp_chan_set_eth_loopback_req(void *priv, u8 enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_set_eth_loopback param = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.eth_port_id = NBL_DISP_MGT_TO_COMMON(disp_mgt)->eth_id;
	param.enable = enable;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SET_ETH_LOOPBACK,  &param,
		      sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_eth_loopback_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_set_eth_loopback *param;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_chan_param_set_eth_loopback *)data;
	ret = NBL_OPS_CALL(res_ops->setup_loopback,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->eth_port_id, param->enable));
	if (ret) {
		dev_err(dev, "setup loopback adminq failed with ret: %d\n", ret);
		err = NBL_CHAN_RESP_ERR;
	}

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_ETH_LOOPBACK,
		     msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_SET_ETH_LOOPBACK);
}

static struct sk_buff *nbl_disp_clean_rx_lb_test(void *priv, u32 ring_index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->clean_rx_lb_test,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ring_index));
}

static u32 nbl_disp_check_active_vf(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->check_active_vf,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), 0));
}

static u32 nbl_disp_chan_check_active_vf_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct device *dev = NBL_DISP_MGT_TO_DEV(disp_mgt);
	u32 active_vf_num = 0;
	int ret;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_CHECK_ACTIVE_VF,  NULL, 0,
		      &active_vf_num, sizeof(active_vf_num), 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	if (ret)
		dev_err(dev, "channel check active vf send msg failed with ret: %d\n", ret);

	return active_vf_num;
}

static void nbl_disp_chan_check_active_vf_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	u32 active_vf_num;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	active_vf_num = NBL_OPS_CALL(res_ops->check_active_vf,
				     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CHECK_ACTIVE_VF,
		     msg_id, err, &active_vf_num, sizeof(active_vf_num));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_SET_ETH_LOOPBACK);
}

static u32 nbl_disp_get_adminq_tx_buf_size(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	return chan_ops->get_adminq_tx_buf_size(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt));
}

static int nbl_disp_adminq_emp_console_write(void *priv, char *buf, size_t count)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send;

	NBL_CHAN_SEND(chan_send, NBL_CHAN_ADMINQ_FUNCTION_ID,
		      NBL_CHAN_MSG_ADMINQ_EMP_CONSOLE_WRITE,
		      buf, count, NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static bool nbl_disp_get_product_flex_cap(void *priv, enum nbl_flex_cap_type cap_type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	bool has_cap = false;

	has_cap = NBL_OPS_CALL(res_ops->get_product_flex_cap, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
							       cap_type));
	return has_cap;
}

static int nbl_disp_set_pmd_debug(void *priv, bool pmd_debug)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->set_pmd_debug,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), pmd_debug));
}

static bool nbl_disp_chan_get_product_flex_cap_req(void *priv, enum nbl_flex_cap_type cap_type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;
	bool has_cap = false;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_PRODUCT_FLEX_CAP, &cap_type,
		      sizeof(cap_type), &has_cap, sizeof(has_cap), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return has_cap;
}

static void nbl_disp_chan_get_product_flex_cap_resp(void *priv, u16 src_id, u16 msg_id,
						    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	enum nbl_flex_cap_type *cap_type = (enum nbl_flex_cap_type *)data;
	struct nbl_chan_ack_info chan_ack = {0};
	bool has_cap = false;

	has_cap = NBL_OPS_CALL(res_ops->get_product_flex_cap,
			       (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *cap_type));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_PRODUCT_FLEX_CAP, msg_id,
		     NBL_CHAN_RESP_OK, &has_cap, sizeof(has_cap));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static bool nbl_disp_get_product_fix_cap(void *priv, enum nbl_fix_cap_type cap_type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	bool has_cap = false;

	has_cap = NBL_OPS_CALL(res_ops->get_product_fix_cap, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
							      cap_type));
	return has_cap;
}

static int nbl_disp_alloc_ktls_tx_index(void *priv, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int index = 0;

	index = NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->alloc_ktls_tx_index,
				       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi);
	return index;
}

static int nbl_disp_chan_alloc_ktls_tx_index_req(void *priv, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;
	int index = 0;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_ALLOC_KTLS_TX_INDEX, &vsi, sizeof(u16),
		      &index, sizeof(index), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return index;
}

static void nbl_disp_chan_alloc_ktls_tx_index_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack = {0};
	int index;
	u16 vsi;

	vsi = *(u16 *)data;
	index = NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->alloc_ktls_tx_index,
				       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ALLOC_KTLS_TX_INDEX, msg_id,
		     NBL_CHAN_RESP_OK, &index, sizeof(index));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_free_ktls_tx_index(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->free_ktls_tx_index,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index);
}

static void nbl_disp_chan_free_ktls_tx_index_req(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_FREE_KTLS_TX_INDEX, &index,
		      sizeof(index), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_free_ktls_tx_index_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack = {0};
	u32 index;

	index = *(u32 *)data;
	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->free_ktls_tx_index,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_FREE_KTLS_TX_INDEX, msg_id,
		     NBL_CHAN_RESP_OK, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_cfg_ktls_tx_keymat(void *priv, u32 index, u8 mode,
					u8 *salt, u8 *key, u8 key_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->cfg_ktls_tx_keymat,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index, mode, salt,
			       key, key_len);
}

static void nbl_disp_chan_cfg_ktls_tx_keymat_req(void *priv, u32 index, u8 mode,
						 u8 *salt, u8 *key, u8 key_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ktls_keymat param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	param.index = index;
	param.mode = mode;
	memcpy(param.salt, salt, sizeof(param.salt));
	memcpy(param.key, key, key_len);
	param.key_len = key_len;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_CFG_KTLS_TX_KEYMAT, &param,
		      sizeof(param), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_ktls_tx_keymat_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ktls_keymat *param;
	struct nbl_chan_ack_info chan_ack;

	param = (struct nbl_chan_cfg_ktls_keymat *)data;
	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->cfg_ktls_tx_keymat,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->index,
			       param->mode, param->salt, param->key, param->key_len);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_KTLS_TX_KEYMAT, msg_id,
		     NBL_CHAN_RESP_OK, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_alloc_ktls_rx_index(void *priv, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int index = 0;

	index = NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->alloc_ktls_rx_index,
				       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi);
	return index;
}

static int nbl_disp_chan_alloc_ktls_rx_index_req(void *priv, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;
	int index = 0;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_ALLOC_KTLS_RX_INDEX, &vsi, sizeof(u16),
		      &index, sizeof(index), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return index;
}

static void nbl_disp_chan_alloc_ktls_rx_index_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack = {0};
	int index;
	u16 vsi;

	vsi = *(u16 *)data;
	index = NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->alloc_ktls_rx_index,
				       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ALLOC_KTLS_RX_INDEX, msg_id,
		     NBL_CHAN_RESP_OK, &index, sizeof(index));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_free_ktls_rx_index(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->free_ktls_rx_index,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index);
}

static void nbl_disp_chan_free_ktls_rx_index_req(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_FREE_KTLS_RX_INDEX, &index,
		      sizeof(index), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_free_ktls_rx_index_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack = {0};
	u32 index;

	index = *(u32 *)data;
	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->free_ktls_rx_index,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_FREE_KTLS_RX_INDEX, msg_id,
		     NBL_CHAN_RESP_OK, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_cfg_ktls_rx_keymat(void *priv, u32 index, u8 mode,
					u8 *salt, u8 *key, u8 key_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->cfg_ktls_rx_keymat,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index, mode,
			       salt, key, key_len);
}

static void nbl_disp_chan_cfg_ktls_rx_keymat_req(void *priv, u32 index, u8 mode,
						 u8 *salt, u8 *key, u8 key_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ktls_keymat param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.index = index;
	param.mode = mode;
	memcpy(param.salt, salt, sizeof(param.salt));
	memcpy(param.key, key, key_len);
	param.key_len = key_len;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_CFG_KTLS_RX_KEYMAT, &param,
		      sizeof(param), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_ktls_rx_keymat_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ktls_keymat *param;
	struct nbl_chan_ack_info chan_ack;

	param = (struct nbl_chan_cfg_ktls_keymat *)data;
	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->cfg_ktls_rx_keymat,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->index,
			       param->mode, param->salt, param->key, param->key_len);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_KTLS_RX_KEYMAT, msg_id,
		     NBL_CHAN_RESP_OK, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_cfg_ktls_rx_record(void *priv, u32 index, u32 tcp_sn, u64 rec_num, bool init)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->cfg_ktls_rx_record,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index, tcp_sn, rec_num, init);
}

static void nbl_disp_chan_cfg_ktls_rx_record_req(void *priv, u32 index,
						 u32 tcp_sn, u64 rec_num, bool init)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ktls_record param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.init = init;
	param.index = index;
	param.tcp_sn = tcp_sn;
	param.rec_num = rec_num;
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_CFG_KTLS_RX_RECORD, &param,
		      sizeof(param), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_ktls_rx_record_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ktls_record *param;
	struct nbl_chan_ack_info chan_ack;

	param = (struct nbl_chan_cfg_ktls_record *)data;
	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->cfg_ktls_rx_record,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			       param->index, param->tcp_sn, param->rec_num, param->init);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_KTLS_RX_RECORD, msg_id,
		     NBL_CHAN_RESP_OK, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_add_ktls_rx_flow(void *priv, u32 index, u32 *data, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->add_ktls_rx_flow,
				      NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index, data, vsi);
}

static int nbl_disp_chan_add_ktls_rx_flow_req(void *priv, u32 index, u32 *data, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ktls_flow param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.index = index;
	param.vsi = vsi;
	memcpy(param.data, data, sizeof(param.data));
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_ADD_KTLS_RX_FLOW, &param,
		      sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_add_ktls_rx_flow_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ktls_flow *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	param = (struct nbl_chan_cfg_ktls_flow *)data;
	ret = NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->add_ktls_rx_flow,
				     NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->index,
				     param->data, param->vsi);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ADD_KTLS_RX_FLOW, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_del_ktls_rx_flow(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->del_ktls_rx_flow,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index);
}

static void nbl_disp_chan_del_ktls_rx_flow_req(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ktls_flow param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.index = index;
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_DEL_KTLS_RX_FLOW, &param,
		      sizeof(param), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_del_ktls_rx_flow_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ktls_flow *param;
	struct nbl_chan_ack_info chan_ack;

	param = (struct nbl_chan_cfg_ktls_flow *)data;
	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->del_ktls_rx_flow,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->index);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_DEL_KTLS_RX_FLOW, msg_id,
		     NBL_CHAN_RESP_OK, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_alloc_ipsec_tx_index(void *priv, struct nbl_ipsec_cfg_info *cfg_info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int index = 0;

	index = NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->alloc_ipsec_tx_index,
				       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), cfg_info);
	return index;
}

static int nbl_disp_chan_alloc_ipsec_tx_index_req(void *priv, struct nbl_ipsec_cfg_info *cfg_info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ipsec_index param = {0};
	struct nbl_chan_ipsec_index result = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	memcpy(&param.cfg_info, cfg_info, sizeof(param.cfg_info));
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_ALLOC_IPSEC_TX_INDEX, &param,
		      sizeof(param), &result, sizeof(result), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return result.index;
}

static void nbl_disp_chan_alloc_ipsec_tx_index_resp(void *priv, u16 src_id, u16 msg_id,
						    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ipsec_index *param;
	struct nbl_chan_ipsec_index result = {0};
	struct nbl_chan_ack_info chan_ack = {0};

	param = (struct nbl_chan_ipsec_index *)data;
	result.index = NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->alloc_ipsec_tx_index,
					      NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
					      &param->cfg_info);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ALLOC_IPSEC_TX_INDEX, msg_id,
		     NBL_CHAN_RESP_OK, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_free_ipsec_tx_index(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->free_ipsec_tx_index,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index);
}

static void nbl_disp_chan_free_ipsec_tx_index_req(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ipsec_index param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.index = index;
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_FREE_IPSEC_TX_INDEX, &param,
		      sizeof(param), NULL, 0, 0);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_free_ipsec_tx_index_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_chan_ipsec_index *param;

	param = (struct nbl_chan_ipsec_index *)data;
	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->free_ipsec_tx_index,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->index);
}

static int nbl_disp_alloc_ipsec_rx_index(void *priv, struct nbl_ipsec_cfg_info *cfg_info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int index = 0;

	index = NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->alloc_ipsec_rx_index,
				       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), cfg_info);
	return index;
}

static int nbl_disp_chan_alloc_ipsec_rx_index_req(void *priv, struct nbl_ipsec_cfg_info *cfg_info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ipsec_index param = {0};
	struct nbl_chan_ipsec_index result = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	memcpy(&param.cfg_info, cfg_info, sizeof(param.cfg_info));
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_ALLOC_IPSEC_RX_INDEX, &param,
		      sizeof(param), &result, sizeof(result), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return result.index;
}

static void nbl_disp_chan_alloc_ipsec_rx_index_resp(void *priv, u16 src_id, u16 msg_id,
						    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ipsec_index *param;
	struct nbl_chan_ipsec_index result = {0};
	struct nbl_chan_ack_info chan_ack = {0};

	param = (struct nbl_chan_ipsec_index *)data;
	result.index = NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->alloc_ipsec_rx_index,
					      NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
					      &param->cfg_info);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ALLOC_IPSEC_RX_INDEX, msg_id,
		     NBL_CHAN_RESP_OK, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_free_ipsec_rx_index(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->free_ipsec_rx_index,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index);
}

static void nbl_disp_chan_free_ipsec_rx_index_req(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ipsec_index param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.index = index;
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_FREE_IPSEC_RX_INDEX, &param,
		      sizeof(param), NULL, 0, 0);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_free_ipsec_rx_index_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_chan_ipsec_index *param;

	param = (struct nbl_chan_ipsec_index *)data;
	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->free_ipsec_rx_index,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->index);
}

static void nbl_disp_cfg_ipsec_tx_sad(void *priv, u32 index, struct nbl_ipsec_sa_entry *sa_entry)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->cfg_ipsec_tx_sad,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index, sa_entry);
}

static void nbl_disp_chan_cfg_ipsec_tx_sad_req(void *priv, u32 index,
					       struct nbl_ipsec_sa_entry *sa_entry)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ipsec_sad param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.index = index;
	memcpy(&param.sa_entry, sa_entry, sizeof(param.sa_entry));
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_CFG_IPSEC_TX_SAD, &param,
		      sizeof(param), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_ipsec_tx_sad_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ipsec_sad *param;
	struct nbl_chan_ack_info chan_ack;

	param = (struct nbl_chan_cfg_ipsec_sad *)data;
	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->cfg_ipsec_tx_sad,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->index,
			       &param->sa_entry);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_IPSEC_TX_SAD, msg_id,
		     NBL_CHAN_RESP_OK, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_cfg_ipsec_rx_sad(void *priv, u32 index, struct nbl_ipsec_sa_entry *sa_entry)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->cfg_ipsec_rx_sad,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index, sa_entry);
}

static void nbl_disp_chan_cfg_ipsec_rx_sad_req(void *priv, u32 index,
					       struct nbl_ipsec_sa_entry *sa_entry)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ipsec_sad param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.index = index;
	memcpy(&param.sa_entry, sa_entry, sizeof(param.sa_entry));
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_CFG_IPSEC_RX_SAD, &param,
		      sizeof(param), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_ipsec_rx_sad_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ipsec_sad *param;
	struct nbl_chan_ack_info chan_ack;

	param = (struct nbl_chan_cfg_ipsec_sad *)data;
	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->cfg_ipsec_rx_sad,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->index,
			       &param->sa_entry);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_IPSEC_RX_SAD, msg_id,
		     NBL_CHAN_RESP_OK, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_add_ipsec_tx_flow(void *priv, u32 index, u32 *data, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->add_ipsec_tx_flow,
				      NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index, data, vsi);
}

static int nbl_disp_chan_add_ipsec_tx_flow_req(void *priv, u32 index, u32 *data, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ipsec_flow param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.index = index;
	param.vsi = vsi;
	memcpy(param.data, data, sizeof(param.data));
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_ADD_IPSEC_TX_FLOW, &param,
		      sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_add_ipsec_tx_flow_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ipsec_flow *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	param = (struct nbl_chan_cfg_ipsec_flow *)data;
	ret = NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->add_ipsec_tx_flow,
				     NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				     param->index, param->data, param->vsi);
	if (ret)
		err = NBL_CHAN_RESP_ERR;
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ADD_IPSEC_TX_FLOW, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_del_ipsec_tx_flow(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->del_ipsec_tx_flow,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index);
}

static void nbl_disp_chan_del_ipsec_tx_flow_req(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ipsec_flow param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.index = index;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_DEL_IPSEC_TX_FLOW, &param,
		      sizeof(param), NULL, 0, 0);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_del_ipsec_tx_flow_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_chan_cfg_ipsec_flow *param;

	param = (struct nbl_chan_cfg_ipsec_flow *)data;
	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->del_ipsec_tx_flow,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->index);
}

static int nbl_disp_add_ipsec_rx_flow(void *priv, u32 index, u32 *data, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->add_ipsec_rx_flow,
				      NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index, data, vsi);
}

static int nbl_disp_chan_add_ipsec_rx_flow_req(void *priv, u32 index, u32 *data, u16 vsi)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ipsec_flow param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.index = index;
	param.vsi = vsi;
	memcpy(param.data, data, sizeof(param.data));
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_ADD_IPSEC_RX_FLOW, &param,
		      sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_add_ipsec_rx_flow_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ipsec_flow *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	param = (struct nbl_chan_cfg_ipsec_flow *)data;
	ret = NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->add_ipsec_rx_flow,
				     NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				     param->index, param->data, param->vsi);
	if (ret)
		err = NBL_CHAN_RESP_ERR;
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ADD_IPSEC_RX_FLOW, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_del_ipsec_rx_flow(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->del_ipsec_rx_flow,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index);
}

static void nbl_disp_chan_del_ipsec_rx_flow_req(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_cfg_ipsec_flow param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.index = index;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_DEL_IPSEC_RX_FLOW, &param,
		      sizeof(param), NULL, 0, 0);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_del_ipsec_rx_flow_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_chan_cfg_ipsec_flow *param;

	param = (struct nbl_chan_cfg_ipsec_flow *)data;
	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->del_ipsec_rx_flow,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->index);
}

static bool nbl_disp_check_ipsec_status(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->check_ipsec_status, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static u32 nbl_disp_get_dipsec_lft_info(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->get_dipsec_lft_info,
				      NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
}

static void nbl_disp_handle_dipsec_soft_expire(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->handle_dipsec_soft_expire,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index);
}

static void nbl_disp_handle_dipsec_hard_expire(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->handle_dipsec_hard_expire,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index);
}

static u32 nbl_disp_get_uipsec_lft_info(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->get_uipsec_lft_info,
				      NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
}

static void nbl_disp_handle_uipsec_soft_expire(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->handle_uipsec_soft_expire,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index);
}

static void nbl_disp_handle_uipsec_hard_expire(void *priv, u32 index)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->handle_uipsec_hard_expire,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), index);
}

static int nbl_disp_get_mbx_irq_num(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_mbx_irq_num, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_get_adminq_irq_num(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_adminq_irq_num, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_get_abnormal_irq_num(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_abnormal_irq_num, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static void nbl_disp_clear_accel_flow(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->clear_accel_flow,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static void nbl_disp_clear_flow(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->clear_flow,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static void nbl_disp_clear_queues(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->clear_queues,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
}

static int nbl_disp_disable_phy_flow(void *priv, u8 eth_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->disable_phy_flow,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id);
}

static int nbl_disp_enable_phy_flow(void *priv, u8 eth_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->enable_phy_flow,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id);
}

static void nbl_disp_init_acl(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->init_acl, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static void nbl_disp_uninit_acl(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->uninit_acl, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_set_upcall_rule(void *priv, u8 eth_id, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->set_upcall_rule,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, vsi_id));
}

static int nbl_disp_unset_upcall_rule(void *priv, u8 eth_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->unset_upcall_rule,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id));
}

static void nbl_disp_set_shaping_dport_vld(void *priv, u8 eth_id, bool vld)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->set_shaping_dport_vld,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, vld));
}

static void nbl_disp_set_dport_fc_th_vld(void *priv, u8 eth_id, bool vld)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->set_dport_fc_th_vld,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, vld));
}

static u16 nbl_disp_get_vsi_global_qid(void *priv, u16 vsi_id, u16 local_qid)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_vsi_global_queue_id,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, local_qid));
}

static u16
nbl_disp_chan_get_vsi_global_qid_req(void *priv, u16 vsi_id, u16 local_qid)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_vsi_qid_info param = {0};
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	param.local_qid = local_qid;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_VSI_GLOBAL_QUEUE_ID, &param,
		      sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void
nbl_disp_chan_get_vsi_global_qid_resp(void *priv, u16 src_id, u16 msg_id,
				      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_vsi_qid_info *param;
	struct nbl_chan_ack_info chan_ack;
	u16 global_qid;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_chan_vsi_qid_info *)data;
	global_qid = NBL_OPS_CALL(res_ops->get_vsi_global_queue_id,
				  (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				  param->vsi_id, param->local_qid));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_VSI_GLOBAL_QUEUE_ID,
		     msg_id, global_qid, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_get_line_rate_info_resp(void *priv, u16 src_id, u16 msg_id,
				      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	struct nbl_rep_line_rate_info result = {0};

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->get_line_rate_info, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), data,
						   &result));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_LINE_RATE_INFO,
		     msg_id, 0, &result, sizeof(struct nbl_rep_line_rate_info));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_register_net_rep_resp(void *priv, u16 src_id, u16 msg_id,
				    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_register_net_rep *param;
	struct nbl_chan_ack_info chan_ack;
	struct nbl_register_net_rep_result result = {0};

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_chan_param_register_net_rep *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->register_net_rep,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->pf_id,
			  param->vf_id, &result);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REGISTER_NET_REP,
		     msg_id, 0, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_unregister_net_rep_resp(void *priv, u16 src_id, u16 msg_id,
				      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	u16 vsi_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	vsi_id = *(u16 *)data;
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->unregister_net_rep,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_UNREGISTER_NET_REP,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_register_eth_rep_resp(void *priv, u16 src_id, u16 msg_id,
				    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	u8 eth_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	eth_id = *(u8 *)data;
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->register_eth_rep,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REGISTER_ETH_REP,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_get_queue_cxt_resp(void *priv, u16 src_id, u16 msg_id,
				 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_get_queue_cxt *param;
	struct nbl_chan_ack_info chan_ack;
	u16 cxt;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_chan_param_get_queue_cxt *)data;

	cxt = NBL_OPS_CALL(res_ops->get_queue_ctx,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id, param->local_queue));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_QUEUE_CXT,
		     msg_id, 0, &cxt, sizeof(cxt));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_init_vdpaq_resp(void *priv, u16 src_id, u16 msg_id,
			      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_vdpaq_init_info *param;
	struct nbl_chan_ack_info chan_ack;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_chan_vdpaq_init_info *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->init_vdpaq,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id, param->pa, param->size);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_INIT_VDPAQ,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_destroy_vdpaq_resp(void *priv, u16 src_id, u16 msg_id,
				 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->destroy_vdpaq,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_DESTROY_VDPAQ,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_get_upcall_port_resp(void *priv, u16 src_id, u16 msg_id,
				   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int ret;
	u16 bdf;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	ret = NBL_OPS_CALL(res_ops->get_upcall_port,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), &bdf));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_GET_UPCALL_PORT,
		     msg_id, ret, &bdf, sizeof(u16));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_get_board_info(void *priv, struct nbl_board_port_info *board_info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_board_info,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), board_info));
}

static void
nbl_disp_chan_get_board_info_req(void *priv, struct nbl_board_port_info *board_info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_BOARD_INFO, NULL,
		      0, board_info, sizeof(*board_info), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void
nbl_disp_chan_get_board_info_resp(void *priv, u16 src_id, u16 msg_id,
				  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	struct nbl_board_port_info board_info = {0};

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->get_board_info,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), &board_info));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_BOARD_INFO,
		     msg_id, 0, &board_info, sizeof(board_info));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_cfg_log_resp(void *priv, u16 src_id, u16 msg_id,
			   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_cfg_log *param;
	struct nbl_chan_ack_info chan_ack;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_chan_param_cfg_log *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->cfg_queue_log,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id,
			  param->qps, param->vld);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_LOG,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_unregister_eth_rep_resp(void *priv, u16 src_id, u16 msg_id,
				      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	u8 eth_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	eth_id = *(u8 *)data;
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->unregister_eth_rep,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_UNREGISTER_ETH_REP,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_register_upcall_port_resp(void *priv, u16 src_id, u16 msg_id,
					void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int ret;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->register_upcall_port,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REGISTER_UPCALL_PORT,
		     msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_unregister_upcall_port_resp(void *priv, u16 src_id, u16 msg_id,
					  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->unregister_upcall_port,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_UNREGISTER_UPCALL_PORT,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_set_offload_status_resp(void *priv, u16 src_id, u16 msg_id,
				      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_offload_status,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id);
}

static int nbl_disp_check_offload_status(void *priv, bool *is_down)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL(res_ops->check_offload_status,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), is_down));
}

static int nbl_disp_get_port_attributes(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_port_attributes, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	if (ret)
		dev_err(dev, "get port attributes failed with ret: %d\n", ret);

	return ret;
}

static int nbl_disp_update_ring_num(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->update_ring_num, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_update_rdma_cap(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->update_rdma_cap, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static u16 nbl_disp_get_rdma_cap_num(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_rdma_cap_num, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_update_rdma_mem_type(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->update_rdma_mem_type, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_set_ring_num(void *priv, struct nbl_fw_cmd_net_ring_num_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->set_ring_num, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param));
}

static int nbl_disp_enable_port(void *priv, bool enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int ret = 0;

	ret = NBL_OPS_CALL(res_ops->enable_port, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), enable));
	if (ret)
		dev_err(dev, "enable port failed with ret: %d\n", ret);

	return ret;
}

static void nbl_disp_init_port(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->init_port, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static void nbl_disp_chan_recv_port_notify_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL(res_ops->recv_port_notify,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), data));
}

static int nbl_disp_get_fec_stats(void *priv, u8 eth_id,
				  struct nbl_fec_stats *fec_stats)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_fec_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, fec_stats));
	return ret;
}

static int nbl_disp_chan_get_fec_stats_req(void *priv, u8 eth_id,
					   struct nbl_fec_stats *fec_stats)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_GET_FEC_STATS, &eth_id, sizeof(eth_id),
		      fec_stats, sizeof(*fec_stats), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_fec_stats_resp(void *priv, u16 src_id, u16 msg_id,
					     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_fec_stats info = {0};
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_fec_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *(u8 *)data, &info));
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "disp get eth fec stats failed with ret: %d\n", ret);
	}
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_FEC_STATS, msg_id, err,
		     &info, sizeof(info));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "disp chan send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_GET_FEC_STATS, src_id);
}

static int nbl_disp_get_port_state(void *priv, u8 eth_id,
				   struct nbl_port_state *port_state)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_port_state,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, port_state));
	return ret;
}

static int nbl_disp_chan_get_port_state_req(void *priv, u8 eth_id,
					    struct nbl_port_state *port_state)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_GET_PORT_STATE, &eth_id, sizeof(eth_id),
		      port_state, sizeof(*port_state), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_port_state_resp(void *priv, u16 src_id, u16 msg_id,
					      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	struct nbl_port_state info = {0};
	int ret = 0;
	u8 eth_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	eth_id = *(u8 *)data;
	ret = NBL_OPS_CALL(res_ops->get_port_state,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, &info));
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_PORT_STATE, msg_id, err,
		     &info, sizeof(info));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_set_port_advertising(void *priv,
					 struct nbl_port_advertising *port_advertising)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int ret = 0;

	ret = NBL_OPS_CALL(res_ops->set_port_advertising,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), port_advertising));
	return ret;
}

static int nbl_disp_chan_set_port_advertising_req(void *priv,
						  struct nbl_port_advertising *port_advertising)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SET_PORT_ADVERTISING,
		      port_advertising, sizeof(*port_advertising),
		      NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_port_advertising_resp(void *priv, u16 src_id, u16 msg_id,
						    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_port_advertising *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_port_advertising *)data;

	ret = res_ops->set_port_advertising(NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_PORT_ADVERTISING, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_module_info(void *priv, u8 eth_id, struct ethtool_modinfo *info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return res_ops->get_module_info(NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, info);
}

static int nbl_disp_chan_get_module_info_req(void *priv, u8 eth_id, struct ethtool_modinfo *info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_GET_MODULE_INFO, &eth_id,
		      sizeof(eth_id), info, sizeof(*info), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_module_info_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	struct ethtool_modinfo info;
	int ret = 0;
	u8 eth_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	eth_id = *(u8 *)data;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_module_info,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, &info);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_MODULE_INFO, msg_id, err,
		     &info, sizeof(info));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_module_eeprom(void *priv, u8 eth_id,
				      struct ethtool_eeprom *eeprom, u8 *data)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return res_ops->get_module_eeprom(NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, eeprom, data);
}

static int nbl_disp_chan_get_module_eeprom_req(void *priv, u8 eth_id,
					       struct ethtool_eeprom *eeprom, u8 *data)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_get_module_eeprom param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.eth_id = eth_id;
	memcpy(&param.eeprom, eeprom, sizeof(struct ethtool_eeprom));

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_MODULE_EEPROM, &param,
		      sizeof(param), data, eeprom->len, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_module_eeprom_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_get_module_eeprom *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	u8 eth_id;
	struct ethtool_eeprom *eeprom;
	u8 *recv_data;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_chan_param_get_module_eeprom *)data;
	eth_id = param->eth_id;
	eeprom = &param->eeprom;
	recv_data = kmalloc(eeprom->len, GFP_ATOMIC);
	if (!recv_data) {
		dev_err(dev, "Allocate memory to store module eeprom failed\n");
		return;
	}

	ret = res_ops->get_module_eeprom(NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
					 eth_id, eeprom, recv_data);
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "Get module eeprom failed with ret: %d\n", ret);
	}

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_MODULE_EEPROM, msg_id, err,
		     recv_data, eeprom->len);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_GET_MODULE_EEPROM, src_id);
	kfree(recv_data);
}

static int nbl_disp_get_link_state(void *priv, u8 eth_id, struct nbl_eth_link_info *eth_link_info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	/* if donot have res_ops->get_link_state(), default eth is up */
	if (res_ops->get_link_state)
		ret = res_ops->get_link_state(NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
					      eth_id, eth_link_info);
	else
		eth_link_info->link_status = 1;

	return ret;
}

static int nbl_disp_chan_get_link_state_req(void *priv, u8 eth_id,
					    struct nbl_eth_link_info *eth_link_info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf,
		      NBL_CHAN_MSG_GET_LINK_STATE, &eth_id,
		      sizeof(eth_id), eth_link_info, sizeof(*eth_link_info), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_link_state_resp(void *priv, u16 src_id, u16 msg_id,
					      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	u8 eth_id;
	struct nbl_eth_link_info eth_link_info = {0};
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	eth_id = *(u8 *)data;
	ret = res_ops->get_link_state(NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
					 eth_id, &eth_link_info);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_LINK_STATE, msg_id, err,
		     &eth_link_info, sizeof(eth_link_info));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_link_down_count(void *priv, u8 eth_id, u64 *link_down_count)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_link_down_count,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, link_down_count));
}

static int nbl_disp_chan_get_link_down_count_req(void *priv, u8 eth_id, u64 *link_down_count)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_LINK_DOWN_COUNT, &eth_id,
		      sizeof(eth_id), link_down_count, sizeof(*link_down_count), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_link_down_count_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	u8 eth_id;
	u64 link_down_count = 0;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	eth_id = *(u8 *)data;
	ret = res_ops->get_link_down_count(NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
					 eth_id, &link_down_count);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_LINK_DOWN_COUNT, msg_id, err,
		     &link_down_count, sizeof(link_down_count));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_link_status_opcode(void *priv, u8 eth_id, u32 *link_status_opcode)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_link_status_opcode,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, link_status_opcode));
}

static int nbl_disp_chan_get_link_status_opcode_req(void *priv, u8 eth_id, u32 *link_status_opcode)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_LINK_STATUS_OPCODE, &eth_id,
		      sizeof(eth_id), link_status_opcode, sizeof(*link_status_opcode), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_link_status_opcode_resp(void *priv, u16 src_id, u16 msg_id,
						      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	u8 eth_id;
	u32 link_status_opcode = 0;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	eth_id = *(u8 *)data;
	ret = res_ops->get_link_status_opcode(NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
					 eth_id, &link_status_opcode);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_LINK_STATUS_OPCODE, msg_id, err,
		     &link_status_opcode, sizeof(link_status_opcode));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_get_reg_dump(void *priv, u32 *data, u32 len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->get_reg_dump, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), data, len));
}

static void nbl_disp_chan_get_reg_dump_req(void *priv, u32 *data, u32 len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;
	u32 *result = NULL;

	result = kmalloc(len, GFP_KERNEL);
	if (!result)
		return;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_REG_DUMP, &len, sizeof(len),
		      result, len, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	memcpy(data, result, len);
	kfree(result);
}

static int nbl_disp_set_wol(void *priv, u8 eth_id, bool enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->set_wol,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, enable));
}

static int nbl_disp_chan_set_wol_req(void *priv, u8 eth_id, bool enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_send_info chan_send;
	struct nbl_chan_param_set_wol param = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.eth_id = eth_id;
	param.enable = enable;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_SET_WOL, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_wol_resp(void *priv, u16 src_id, u16 msg_id,
				       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_set_wol *param;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	param = (struct nbl_chan_param_set_wol *)data;
	ret = res_ops->set_wol(NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->eth_id, param->enable);
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_WOL, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_get_reg_dump_resp(void *priv, u16 src_id, u16 msg_id,
					    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	u32 *result = NULL;
	u32 len = 0;

	len = *(u32 *)data;
	result = kmalloc(len, GFP_KERNEL);
	if (!result)
		return;

	NBL_OPS_CALL(res_ops->get_reg_dump, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), result, len));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_REG_DUMP, msg_id, err, result, len);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	kfree(result);
}

static int nbl_disp_get_reg_dump_len(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_reg_dump_len, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_chan_get_reg_dump_len_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;
	int result = 0;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_REG_DUMP_LEN, NULL, 0,
		      &result, sizeof(result), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return result;
}

static void nbl_disp_chan_get_reg_dump_len_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int result = 0;

	result = NBL_OPS_CALL(res_ops->get_reg_dump_len, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_REG_DUMP_LEN, msg_id, err,
		     &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_init_offload_fwd_resp(void *priv, u16 src_id, u16 msg_id,
				    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	u16 vsi_id;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	vsi_id = *(u16 *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->init_offload_fwd,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_INIT_OFLD,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_init_cmdq_resp(void *priv, u16 src_id, u16 msg_id,
			     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->init_cmdq,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), data, src_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_INIT_CMDQ,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_destroy_cmdq_resp(void *priv, u16 src_id, u16 msg_id,
				void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->destroy_cmdq, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_DESTROY_CMDQ,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_reset_cmdq_resp(void *priv, u16 src_id, u16 msg_id,
			      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->reset_cmdq, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_RESET_CMDQ,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_offload_flow_rule_resp(void *priv, u16 src_id, u16 msg_id,
				     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->offload_flow_rule,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), data);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_OFFLOAD_FLOW_RULE,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_get_flow_acl_switch_resp(void *priv, u16 src_id, u16 msg_id,
				       void *data, u32 data_len)
{
	u8 acl_enable = false;
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_flow_acl_switch,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), &acl_enable);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_ACL_SWITCH,
		     msg_id, 0, &acl_enable, sizeof(u8));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_init_rep_resp(void *priv, u16 src_id, u16 msg_id,
			    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_rep_cfg_info *param;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_rep_cfg_info *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->init_rep,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			  param->vsi_id, param->inner_type, param->outer_type, param->rep_type);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_INIT_REP,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_init_flow_resp(void *priv, u16 src_id, u16 msg_id,
			     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->init_flow,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), data);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_INIT_FLOW,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void
nbl_disp_chan_deinit_flow_resp(void *priv, u16 src_id, u16 msg_id,
			       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_ack_info chan_ack;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->deinit_flow,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_DEINIT_FLOW,
		     msg_id, 0, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_configure_rdma_msix_off(void *priv, u16 vector)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return res_ops->configure_rdma_msix_off(NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vector);
}

static int nbl_disp_set_eth_mac_addr(void *priv, u8 *mac, u8 eth_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->set_eth_mac_addr,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), mac, eth_id));
}

static int nbl_disp_chan_set_eth_mac_addr_req(void *priv, u8 *mac, u8 eth_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_eth_mac_addr param;
	struct nbl_chan_send_info chan_send;
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	memcpy(param.mac, mac, sizeof(param.mac));
	param.eth_id = eth_id;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SET_ETH_MAC_ADDR,
		      &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_eth_mac_addr_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_eth_mac_addr *param;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_set_eth_mac_addr *)data;

	ret = NBL_OPS_CALL(res_ops->set_eth_mac_addr,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->mac, param->eth_id));
	if (ret)
		err = NBL_CHAN_RESP_ERR;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_ETH_MAC_ADDR, msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_MSG_SET_ETH_MAC_ADDR);
}

static u32 nbl_disp_get_chip_temperature(void *priv, enum nbl_hwmon_type type, u32 senser_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_chip_temperature,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), type, senser_id));
}

static u32 nbl_disp_chan_get_chip_temperature_req(void *priv,
						  enum nbl_hwmon_type type, u32 senser_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_get_hwmon param = {0};
	struct nbl_common_info *common;
	u32 chip_tempetature = 0;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	param.senser_id = senser_id;
	param.type = type;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_CHIP_TEMPERATURE, &param, sizeof(param),
		      &chip_tempetature, sizeof(chip_tempetature), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return chip_tempetature;
}

static void nbl_disp_chan_get_chip_temperature_resp(void *priv, u16 src_id, u16 msg_id,
						    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_get_hwmon *param = (struct nbl_chan_param_get_hwmon *)data;
	int ret = NBL_CHAN_RESP_OK;
	u32 chip_tempetature = 0;

	chip_tempetature = NBL_OPS_CALL(res_ops->get_chip_temperature,
					(NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
					param->type, param->senser_id));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_CHIP_TEMPERATURE, msg_id,
		     ret, &chip_tempetature, sizeof(chip_tempetature));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_module_temperature(void *priv, u8 eth_id,
					   enum nbl_hwmon_type type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_module_temperature,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, type));
}

static int nbl_disp_chan_get_module_temperature_req(void *priv, u8 eth_id,
						    enum nbl_hwmon_type type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	int module_temp;
	struct nbl_chan_param_get_hwmon param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	param.senser_id = eth_id;
	param.type = type;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_GET_MODULE_TEMPERATURE,
		      &param, sizeof(param), &module_temp, sizeof(module_temp), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return module_temp;
}

static void nbl_disp_chan_get_module_temperature_resp(void *priv, u16 src_id, u16 msg_id,
						      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	int module_temp;
	struct nbl_chan_param_get_hwmon *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_get_hwmon *)data;
	module_temp = NBL_OPS_CALL(res_ops->get_module_temperature,
				   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				    param->senser_id, param->type));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_MODULE_TEMPERATURE, msg_id,
		     ret, &module_temp, sizeof(module_temp));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_process_abnormal_event(void *priv, struct nbl_abnormal_event_info *abnomal_info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return res_ops->process_abnormal_event(NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), abnomal_info);
}

static int nbl_disp_chan_switchdev_init_cmdq_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;
	int ret_status = 0;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SWITCHDEV_INIT_CMDQ,
		      NULL, 0, &ret_status, sizeof(ret_status), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	return ret_status;
}

static void nbl_disp_chan_switchdev_init_cmdq_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	int ret_status = 0;

	ret_status = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->switchdev_init_cmdq,
				       (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SWITCHDEV_INIT_CMDQ, msg_id,
		     ret, &ret_status, sizeof(ret_status));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_switchdev_init_cmdq(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->switchdev_init_cmdq,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
}

static int nbl_disp_chan_switchdev_deinit_cmdq_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;
	int ret_status = 0;
	u8 tc_inst_id;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	tc_inst_id = common->tc_inst_id;
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SWITCHDEV_DEINIT_CMDQ,
		      &tc_inst_id, sizeof(tc_inst_id), &ret_status, sizeof(ret_status), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	if (!ret_status)
		common->tc_inst_id = NBL_TC_FLOW_INST_COUNT;
	return 0;
}

static void nbl_disp_chan_switchdev_deinit_cmdq_resp(void *priv, u16 src_id, u16 msg_id,
						     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	int ret_status = 0;
	u8 tc_inst_id;

	tc_inst_id = *(u8 *)data;
	ret_status = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->switchdev_deinit_cmdq,
				       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), tc_inst_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SWITCHDEV_DEINIT_CMDQ, msg_id,
		     ret, &ret_status, sizeof(ret_status));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_switchdev_deinit_cmdq(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->switchdev_deinit_cmdq,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				 common->tc_inst_id);
}

static int nbl_disp_add_tc_flow(void *priv, struct nbl_tc_flow_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->add_tc_flow,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param);
	return ret;
}

static int nbl_disp_del_tc_flow(void *priv, struct nbl_tc_flow_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	if (!param)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->del_tc_flow,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param);
	return ret;
}

static bool nbl_disp_tc_tun_encap_lookup(void *priv,
					 struct nbl_rule_action *rule_act,
					 struct nbl_tc_flow_param *param)
{
	bool ret = 0;
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	if (!rule_act || !param)
		return false;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->tc_tun_encap_lookup,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				rule_act, param);
	return ret;
}

static int nbl_disp_tc_tun_encap_del(void *priv, struct nbl_encap_key *key)
{
	int ret = 0;
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	if (!key)
		return -EINVAL;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->tc_tun_encap_del,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), key);
	return ret;
}

static int nbl_disp_tc_tun_encap_add(void *priv, struct nbl_rule_action *action)
{
	int ret = 0;
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	if (!action)
		return -EINVAL;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->tc_tun_encap_add,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), action);
	return ret;
}

static int nbl_disp_flow_index_lookup(void *priv, struct nbl_flow_index_key key)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->flow_index_lookup,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), key);
	return ret;
}

static int nbl_disp_query_tc_stats(void *priv, struct nbl_stats_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	if (!param)
		return -EINVAL;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->query_tc_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param));
	return ret;
}

static int nbl_disp_set_tc_flow_info(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_tc_flow_info,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
}

static int nbl_disp_chan_set_tc_flow_info_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;
	int ret_status = 0;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SET_TC_FLOW_INFO,
		      NULL, 0, &ret_status, sizeof(ret_status), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	return ret_status;
}

static void nbl_disp_chan_set_tc_flow_info_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	int ret_status = 0;

	ret_status = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_tc_flow_info,
				       (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_TC_FLOW_INFO, msg_id,
		     ret, &ret_status, sizeof(ret_status));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_unset_tc_flow_info(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->unset_tc_flow_info,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
}

static int nbl_disp_chan_unset_tc_flow_info_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;
	int ret_status = 0;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_UNSET_TC_FLOW_INFO,
		      NULL, 0, &ret_status, sizeof(ret_status), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	return 0;
}

static void nbl_disp_chan_unset_tc_flow_info_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	int ret_status = 0;

	ret_status = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->unset_tc_flow_info,
				       (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_UNSET_TC_FLOW_INFO, msg_id,
		     ret, &ret_status, sizeof(ret_status));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_tc_flow_info(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_tc_flow_info,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
}

static void nbl_disp_adapt_desc_gother(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->adapt_desc_gother, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static void nbl_disp_set_desc_high_throughput(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->set_desc_high_throughput, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static void nbl_disp_flr_clear_rdma(void *priv, u16 vf_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->flr_clear_rdma,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vf_id);
}

static void nbl_disp_flr_clear_net(void *priv, u16 vf_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->flr_clear_net,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vf_id);
}

static void nbl_disp_flr_clear_accel(void *priv, u16 vf_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->flr_clear_accel,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vf_id);
}

static void nbl_disp_flr_clear_queues(void *priv, u16 vf_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->flr_clear_queues,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vf_id);
}

static void nbl_disp_flr_clear_accel_flow(void *priv, u16 vf_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_SPIN_LOCK(disp_mgt, res_ops->flr_clear_accel_flow,
			       NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vf_id);
}

static void nbl_disp_flr_clear_flows(void *priv, u16 vf_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->flr_clear_flows,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vf_id);
}

static void nbl_disp_flr_clear_interrupt(void *priv, u16 vf_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->flr_clear_interrupt,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vf_id);
}

static u16 nbl_disp_covert_vfid_to_vsi_id(void *priv, u16 vfid)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->covert_vfid_to_vsi_id,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vfid);
}

static void nbl_disp_unmask_all_interrupts(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->unmask_all_interrupts,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
}

static u32 nbl_disp_get_perf_dump_length(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_perf_dump_length,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
}

static u32 nbl_disp_get_perf_dump_data(void *priv, u8 *buffer, u32 size)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_perf_dump_data,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), buffer, size);
}

static void nbl_disp_keep_alive_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_KEEP_ALIVE,
		      NULL, 0, NULL, 0, 1);

	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_keep_alive_resp(void *priv, u16 src_id, u16 msg_id,
					  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_KEEP_ALIVE, msg_id,
		     0, NULL, 0);

	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_get_rep_queue_info_req(void *priv, u16 *queue_num, u16 *queue_size)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_get_queue_info result = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_REP_QUEUE_INFO,
		      NULL, 0, &result, sizeof(result), 1);

	if (!chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send)) {
		*queue_num = result.queue_num;
		*queue_size = result.queue_size;
	}
}

static void nbl_disp_chan_get_rep_queue_info_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_get_queue_info result = {0};
	int ret = NBL_CHAN_RESP_OK;

	NBL_OPS_CALL(res_ops->get_rep_queue_info,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), &result.queue_num, &result.queue_size));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_REP_QUEUE_INFO, msg_id,
		     ret, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_get_rep_queue_info(void *priv, u16 *queue_num, u16 *queue_size)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->get_rep_queue_info,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), queue_num, queue_size));
}

static void nbl_disp_chan_get_user_queue_info_req(void *priv, u16 *queue_num, u16 *queue_size,
						  u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_get_queue_info result = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_GET_USER_QUEUE_INFO,
		      &vsi_id, sizeof(vsi_id), &result, sizeof(result), 1);

	if (!chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send)) {
		*queue_num = result.queue_num;
		*queue_size = result.queue_size;
	}
}

static void nbl_disp_chan_get_user_queue_info_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_get_queue_info result = {0};
	int ret = NBL_CHAN_RESP_OK;

	NBL_OPS_CALL(res_ops->get_user_queue_info,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), &result.queue_num,
		      &result.queue_size, *(u16 *)data));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_USER_QUEUE_INFO, msg_id,
		     ret, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_get_user_queue_info(void *priv, u16 *queue_num, u16 *queue_size, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->get_user_queue_info,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), queue_num, queue_size, vsi_id));
}

static int nbl_disp_ctrl_port_led(void *priv, u8 eth_id,
				  enum nbl_led_reg_ctrl led_ctrl, u32 *led_reg)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->ctrl_port_led,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, led_ctrl, led_reg));
}

static int nbl_disp_chan_ctrl_port_led_req(void *priv, u8 eth_id,
					   enum nbl_led_reg_ctrl led_ctrl,
					   u32 *led_reg)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_ctrl_port_led param = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.eth_id = eth_id;
	param.led_status = led_ctrl;
	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_CTRL_PORT_LED,
		      &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_ctrl_port_led_resp(void *priv, u16 src_id, u16 msg_id,
					     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_ctrl_port_led *param = {0};
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_ctrl_port_led *)data;
	ret = NBL_OPS_CALL(res_ops->ctrl_port_led,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			   param->eth_id, param->led_status, NULL));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CTRL_PORT_LED, msg_id,
		     ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_passthrough_fw_cmd(void *priv, struct nbl_passthrough_fw_cmd_param *param,
				       struct nbl_passthrough_fw_cmd_param *result)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->passthrough_fw_cmd,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param, result));
}

static int nbl_disp_nway_reset(void *priv, u8 eth_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->nway_reset, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id));
}

static int nbl_disp_chan_nway_reset_req(void *priv, u8 eth_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_NWAY_RESET,
		      &eth_id, sizeof(eth_id), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_nway_reset_resp(void *priv, u16 src_id, u16 msg_id,
					  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	u8 *eth_id;
	int ret = NBL_CHAN_RESP_OK;

	eth_id = (u8 *)data;
	ret = NBL_OPS_CALL(res_ops->nway_reset,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *eth_id));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_NWAY_RESET, msg_id,
		     ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static u16 nbl_disp_get_vf_base_vsi_id(void *priv, u16 func_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_vf_base_vsi_id,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id));
}

static u16 nbl_disp_chan_get_vf_base_vsi_id_req(void *priv, u16 func_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	u16 vf_base_vsi_id = 0;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_VF_BASE_VSI_ID,
		      NULL, 0, &vf_base_vsi_id, sizeof(vf_base_vsi_id), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return vf_base_vsi_id;
}

static void nbl_disp_chan_get_vf_base_vsi_id_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	u16 vf_base_vsi_id;

	vf_base_vsi_id = NBL_OPS_CALL(res_ops->get_vf_base_vsi_id,
				      (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_VF_BASE_VSI_ID, msg_id,
		     ret, &vf_base_vsi_id, sizeof(vf_base_vsi_id));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static u16 nbl_disp_get_intr_suppress_level(void *priv, u64 pkt_rates, u16 last_level)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	return NBL_OPS_CALL(res_ops->get_intr_suppress_level,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), pkt_rates, last_level));
}

static void nbl_disp_set_intr_suppress_level(void *priv, u16 vector_id, u16 vector_num, u16 level)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_intr_suppress_level,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), common->mgt_pf,
			  vector_id, vector_num, level);
}

static void nbl_disp_chan_set_intr_suppress_level_req(void *priv, u16 vector_id,
						      u16 vector_num, u16 level)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_intr_suppress_level param = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;

	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.local_vector_id = vector_id;
	param.vector_num = vector_num;
	param.level = level;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_SET_INTL_SUPPRESS_LEVEL,
		      &param, sizeof(param), NULL, 0, 0);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_intr_suppress_level_resp(void *priv, u16 src_id, u16 msg_id,
						       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	struct nbl_channel_ops *chan_ops;
	struct nbl_chan_param_set_intr_suppress_level *param;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	param = (struct nbl_chan_param_set_intr_suppress_level *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_intr_suppress_level,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id, param->local_vector_id,
			  param->vector_num, param->level);
}

static u32 nbl_disp_get_p4_version(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_p4_version,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_get_p4_info(void *priv, char *verify_code)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_p4_info,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), verify_code));
}

static int nbl_disp_load_p4(void *priv, struct nbl_load_p4_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->load_p4, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param));
}

static int nbl_disp_load_p4_default(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->load_p4_default, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_chan_get_p4_used_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	int p4_type;

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GET_P4_USED,
		      NULL, 0, &p4_type, sizeof(p4_type), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return p4_type;
}

static void nbl_disp_chan_get_p4_used_resp(void *priv, u16 src_id, u16 msg_id,
					   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	int p4_type;

	p4_type = NBL_OPS_CALL(res_ops->get_p4_used, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_P4_USED, msg_id,
		     ret, &p4_type, sizeof(p4_type));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_p4_used(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_p4_used, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_set_p4_used(void *priv, int p4_type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->set_p4_used, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), p4_type));
}

static int nbl_disp_chan_cfg_eth_bond_info_req(void *priv, struct nbl_lag_member_list_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_CFG_ETH_BOND_INFO,
		      param, sizeof(*param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_cfg_eth_bond_info_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	ret = NBL_OPS_CALL(res_ops->cfg_eth_bond_info, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
							(struct nbl_lag_member_list_param *)data));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_ETH_BOND_INFO, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_cfg_eth_bond_info(void *priv, struct nbl_lag_member_list_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->cfg_eth_bond_info, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			    param));
}

static int nbl_disp_chan_add_nd_upcall_flow(void *priv, u16 vsi_id, bool for_pmd)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->add_nd_upcall_flow,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, for_pmd);
}

static int nbl_disp_chan_add_nd_upcall_flow_req(void *priv, u16 vsi_id, bool for_pmd)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = { 0 };
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_param_nd_upcall param = { 0 };

	param.vsi_id = vsi_id;
	param.for_pmd = for_pmd;
	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_ADD_ND_UPCALL_FLOW,
		      &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_add_nd_upcall_flow_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_nd_upcall *param =
		(struct nbl_chan_param_nd_upcall *)data;
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	int ret;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->add_nd_upcall_flow,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				param->vsi_id, param->for_pmd);
	if (ret) {
		err = NBL_CHAN_RESP_ERR;
		dev_err(dev, "disp set nd dup rule failed with ret: %d\n", ret);
	}

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_ADD_ND_UPCALL_FLOW,
		     msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "disp chan send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_SET_UPCALL_RULE, src_id);
}

static void nbl_disp_chan_del_nd_upcall_flow(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->del_nd_upcall_flow,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));
}

static void nbl_disp_chan_del_nd_upcall_flow_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_DEL_ND_UPCALL_FLOW,
		      NULL, 0, NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_del_nd_upcall_flow_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_common_info *common;
	int err = NBL_CHAN_RESP_OK;
	int ret;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->del_nd_upcall_flow,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_DEL_ND_UPCALL_FLOW,
		     msg_id, err, NULL, 0);
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "disp chan send ack failed with ret: %d, msg_type: %d, src_id: %d\n",
			ret, NBL_CHAN_MSG_SET_UPCALL_RULE, src_id);
}

static int nbl_disp_chan_get_board_id_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	int result = -1;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_GET_BOARD_ID,
		      NULL, 0, &result, sizeof(result), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return result;
}

static void nbl_disp_chan_get_board_id_resp(void *priv, u16 src_id, u16 msg_id,
					    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK, result = -1;

	result = NBL_OPS_CALL(res_ops->get_board_id, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_BOARD_ID,
		     msg_id, ret, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_board_id(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_board_id, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static void nbl_disp_chan_register_rdma_bond_req(void *priv,
						 struct nbl_lag_member_list_param *list_param,
						 struct nbl_rdma_register_param *register_param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_REGISTER_RDMA_BOND,
		      list_param, sizeof(*list_param), register_param, sizeof(*register_param), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_register_rdma_bond_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_lag_member_list_param *list_param = NULL;
	struct nbl_rdma_register_param register_param = {0};
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	list_param = (struct nbl_lag_member_list_param *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->register_rdma_bond, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			  list_param, &register_param);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REGISTER_RDMA_BOND,
		     msg_id, ret, &register_param, sizeof(register_param));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_register_rdma_bond(void *priv, struct nbl_lag_member_list_param *list_param,
					struct nbl_rdma_register_param *register_param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->register_rdma_bond,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), list_param, register_param);
}

static void nbl_disp_chan_unregister_rdma_bond_req(void *priv, u16 lag_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_UNREGISTER_RDMA_BOND, &lag_id, sizeof(lag_id), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_unregister_rdma_bond_resp(void *priv, u16 src_id, u16 msg_id,
						    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->unregister_rdma_bond,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *(u16 *)data);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_UNREGISTER_RDMA_BOND, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_unregister_rdma_bond(void *priv, u16 lag_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->unregister_rdma_bond,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), lag_id);
}

static dma_addr_t nbl_disp_restore_abnormal_ring(void *priv, int ring_index, int type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->restore_abnormal_ring,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ring_index, type));
}

static int nbl_disp_restart_abnormal_ring(void *priv, int ring_index, int type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->restart_abnormal_ring,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ring_index, type));
}

static int nbl_disp_chan_restore_hw_queue_req(void *priv, u16 vsi_id, u16 local_queue_id,
					      dma_addr_t dma, int type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_param_restore_hw_queue param = {0};
	struct nbl_chan_send_info chan_send = {0};

	param.vsi_id = vsi_id;
	param.local_queue_id = local_queue_id;
	param.dma = dma;
	param.type = type;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_RESTORE_HW_QUEUE, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_restore_hw_queue_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_restore_hw_queue *param = NULL;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_restore_hw_queue *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->restore_hw_queue, NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			  param->vsi_id, param->local_queue_id, param->dma, param->type);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_RESTORE_HW_QUEUE, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_restore_hw_queue(void *priv, u16 vsi_id, u16 local_queue_id,
				     dma_addr_t dma, int type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->restore_hw_queue,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				 vsi_id, local_queue_id, dma, type);
}

static int
nbl_disp_chan_stop_abnormal_hw_queue_req(void *priv, u16 vsi_id, u16 local_queue_id, int type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_param_stop_abnormal_hw_queue param = {0};
	struct nbl_chan_send_info chan_send = {0};

	param.vsi_id = vsi_id;
	param.local_queue_id = local_queue_id;
	param.type = type;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_STOP_ABNORMAL_HW_QUEUE, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void
nbl_disp_chan_stop_abnormal_hw_queue_resp(void *priv, u16 src_id, u16 msg_id,
					  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_stop_abnormal_hw_queue *param = NULL;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_stop_abnormal_hw_queue *)data;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->stop_abnormal_hw_queue,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id, param->local_queue_id,
			  param->type);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_STOP_ABNORMAL_HW_QUEUE, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_stop_abnormal_hw_queue(void *priv, u16 vsi_id, u16 local_queue_id, int type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->stop_abnormal_hw_queue,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				 vsi_id, local_queue_id, type);
}

static int nbl_disp_stop_abnormal_sw_queue(void *priv, u16 local_queue_id, int type)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->stop_abnormal_sw_queue,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				 local_queue_id, type);
}

static u16 nbl_disp_get_local_queue_id(void *priv, u16 vsi_id, u16 global_queue_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_local_queue_id, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			    vsi_id, global_queue_id));
}

static int nbl_disp_chan_get_eth_bond_info_req(void *priv, struct nbl_bond_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_ETH_BOND_INFO, NULL, 0, param, sizeof(*param), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_eth_bond_info_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_bond_param result;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	memset(&result, 0, sizeof(result));

	NBL_OPS_CALL(res_ops->get_eth_bond_info, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), &result));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_ETH_BOND_INFO,
		     msg_id, ret, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_eth_bond_info(void *priv, struct nbl_bond_param *param)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_eth_bond_info,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param));
}

static void nbl_disp_cfg_eth_bond_event(void *priv, bool enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->cfg_eth_bond_event, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), enable));
}

static int nbl_disp_set_bridge_mode(void *priv, u16 bmode)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_bridge_mode,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				 NBL_COMMON_TO_MGT_PF(common), bmode);
}

static int nbl_disp_chan_set_bridge_mode_req(void *priv, u16 bmode)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_SET_BRIDGE_MODE, &bmode, sizeof(bmode), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_bridge_mode_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	u16 *bmode;

	bmode = (u16 *)data;
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_bridge_mode,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id, *bmode);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_BRIDGE_MODE,
		     msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static u16 nbl_disp_get_vf_function_id(void *priv, u16 vsi_id, int vf_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_vf_function_id,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, vf_id));
}

static u16 nbl_disp_chan_get_vf_function_id_req(void *priv, u16 vsi_id, int vf_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_get_vf_func_id param;
	struct nbl_common_info *common;
	u16 func_id = 0;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	param.vsi_id = vsi_id;
	param.vf_id = vf_id;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_VF_FUNCTION_ID, &param,
		      sizeof(param), &func_id, sizeof(func_id), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return func_id;
}

static void nbl_disp_chan_get_vf_function_id_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_get_vf_func_id *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	u16 func_id;

	param = (struct nbl_chan_param_get_vf_func_id *)data;
	func_id = NBL_OPS_CALL(res_ops->get_vf_function_id,
			       (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id, param->vf_id));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_VF_FUNCTION_ID, msg_id,
		     ret, &func_id, sizeof(func_id));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static u16 nbl_disp_get_vf_vsi_id(void *priv, u16 vsi_id, int vf_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_vf_vsi_id,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, vf_id));
}

static u16 nbl_disp_chan_get_vf_vsi_id_req(void *priv, u16 vsi_id, int vf_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_get_vf_vsi_id param;
	struct nbl_common_info *common;
	u16 vf_vsi = 0;

	common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	param.vsi_id = vsi_id;
	param.vf_id = vf_id;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_VF_VSI_ID, &param,
		      sizeof(param), &vf_vsi, sizeof(vf_vsi), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return vf_vsi;
}

static void nbl_disp_chan_get_vf_vsi_id_resp(void *priv, u16 src_id, u16 msg_id,
					     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_get_vf_vsi_id *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	u16 vsi_id;

	param = (struct nbl_chan_param_get_vf_vsi_id *)data;
	vsi_id = NBL_OPS_CALL(res_ops->get_vf_vsi_id,
			      (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id, param->vf_id));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_VF_VSI_ID, msg_id,
		     ret, &vsi_id, sizeof(vsi_id));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_register_func_mac(void *priv, u8 *mac, u16 func_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->register_func_mac,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), mac, func_id));
}

static bool nbl_disp_check_vf_is_active(void *priv, u16 func_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = false;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->check_vf_is_active,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id));
	return ret;
}

static bool nbl_disp_chan_check_vf_is_active_req(void *priv, u16 func_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	bool is_active;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_CHECK_VF_IS_ACTIVE, &func_id, sizeof(func_id),
		      &is_active, sizeof(is_active), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return is_active;
}

static void nbl_disp_chan_check_vf_is_active_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_chan_ack_info chan_ack;
	u16 func_id;
	bool is_active;
	int err = NBL_CHAN_RESP_OK;
	int ret = 0;

	func_id = *(u16 *)data;

	is_active = NBL_OPS_CALL(res_ops->check_vf_is_active,
				 (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_CHECK_VF_IS_ACTIVE, msg_id,
		     err, &is_active, sizeof(is_active));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_CHECK_VF_IS_ACTIVE);
}

static int nbl_disp_check_vf_is_vdpa(void *priv, u16 func_id, u8 *is_vdpa)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = false;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->check_vf_is_vdpa,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id, is_vdpa));
	return ret;
}

static int nbl_disp_chan_check_vf_is_vdpa_req(void *priv, u16 func_id, u8 *is_vdpa)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_CHECK_VF_IS_VDPA, &func_id, sizeof(func_id),
		      is_vdpa, sizeof(*is_vdpa), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_check_vf_is_vdpa_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct device *dev = NBL_COMMON_TO_DEV(disp_mgt->common);
	struct nbl_chan_ack_info chan_ack;
	u16 func_id;
	int err = NBL_CHAN_RESP_OK;
	u8 is_vdpa = 0;
	int ret = 0;

	func_id = *(u16 *)data;

	err = NBL_OPS_CALL(res_ops->check_vf_is_vdpa,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id, &is_vdpa));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_CHECK_VF_IS_VDPA, msg_id,
		     err, &is_vdpa, sizeof(is_vdpa));
	ret = chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	if (ret)
		dev_err(dev, "channel send ack failed with ret: %d, msg_type: %d\n",
			ret, NBL_CHAN_CHECK_VF_IS_VDPA);
}

static int nbl_disp_get_vdpa_vf_stats(void *priv, u16 func_id, struct nbl_vf_stats *vf_stats)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = false;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->get_vdpa_vf_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id, vf_stats));
	return ret;
}

static int nbl_disp_chan_get_vdpa_vf_stats_req(void *priv, u16 func_id,
					       struct nbl_vf_stats *vf_stats)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_GET_VDPA_VF_STATS,
		      &func_id, sizeof(func_id), vf_stats, sizeof(*vf_stats), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_vdpa_vf_stats_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	u16 func_id;
	struct nbl_vf_stats vf_stats = {0};
	int err = NBL_CHAN_RESP_OK;

	func_id = *(u16 *)data;

	err = NBL_OPS_CALL(res_ops->get_vdpa_vf_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id, &vf_stats));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_VDPA_VF_STATS, msg_id,
		     err, &vf_stats, sizeof(vf_stats));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_uvn_pkt_drop_stats(void *priv, u16 vsi_id,
					   u16 num_queues, u32 *uvn_stat_pkt_drop)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int ret = 0;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_uvn_pkt_drop_stats,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				vsi_id, num_queues, uvn_stat_pkt_drop);
	return ret;
}

static int nbl_disp_chan_get_uvn_pkt_drop_stats_req(void *priv, u16 vsi_id, u16 num_queues,
						    u32 *uvn_stat_pkt_drop)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_get_uvn_pkt_drop_stats param = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	param.num_queues = num_queues;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_GET_UVN_PKT_DROP_STATS,
		      &param, sizeof(param),
		      uvn_stat_pkt_drop, num_queues * sizeof(*uvn_stat_pkt_drop), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_uvn_pkt_drop_stats_resp(void *priv, u16 src_id, u16 msg_id,
						      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_get_uvn_pkt_drop_stats *param = {0};
	struct nbl_chan_ack_info chan_ack;
	u32 *uvn_stat_pkt_drop = NULL;
	int err = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_get_uvn_pkt_drop_stats *)data;
	uvn_stat_pkt_drop = kcalloc(param->num_queues, sizeof(*uvn_stat_pkt_drop), GFP_KERNEL);
	if (!uvn_stat_pkt_drop) {
		err = -ENOMEM;
		goto send_ack;
	}

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_uvn_pkt_drop_stats,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			  param->vsi_id, param->num_queues, uvn_stat_pkt_drop);
send_ack:
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_GET_UVN_PKT_DROP_STATS, msg_id,
		     err, uvn_stat_pkt_drop, param->num_queues * sizeof(*uvn_stat_pkt_drop));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);

	kfree(uvn_stat_pkt_drop);
}

static int nbl_disp_get_ustore_pkt_drop_stats(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_ustore_pkt_drop_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	return ret;
}

static int nbl_disp_chan_get_ustore_pkt_drop_stats_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_GET_USTORE_PKT_DROP_STATS,
		      NULL, 0, NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_ustore_pkt_drop_stats_resp(void *priv, u16 src_id, u16 msg_id,
							 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;

	err = NBL_OPS_CALL(res_ops->get_ustore_pkt_drop_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_GET_USTORE_PKT_DROP_STATS, msg_id,
		     err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_ustore_total_pkt_drop_stats(void *priv, u8 eth_id,
						    struct nbl_ustore_stats *ustore_stats)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_ustore_total_pkt_drop_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, ustore_stats));

	return ret;
}

static int nbl_disp_chan_get_ustore_total_pkt_drop_stats_req(void *priv, u8 eth_id,
							     struct nbl_ustore_stats *ustore_stats)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_GET_USTORE_TOTAL_PKT_DROP_STATS,
		      &eth_id, sizeof(eth_id), ustore_stats, sizeof(*ustore_stats), 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_get_ustore_total_pkt_drop_stats_resp(void *priv, u16 src_id, u16 msg_id,
							       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	u8 eth_id;
	struct nbl_ustore_stats ustore_stats = {0};
	int err = NBL_CHAN_RESP_OK;

	eth_id = *(u8 *)data;

	err = NBL_OPS_CALL(res_ops->get_ustore_total_pkt_drop_stats,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, &ustore_stats));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_GET_USTORE_TOTAL_PKT_DROP_STATS, msg_id,
		     err, &ustore_stats, sizeof(ustore_stats));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_chan_register_func_mac_req(void *priv, u8 *mac, u16 func_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_register_func_mac param;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.func_id = func_id;
	ether_addr_copy(param.mac, mac);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_REGISTER_FUNC_MAC, &param, sizeof(param), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_register_func_mac_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_register_func_mac *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_register_func_mac *)data;
	NBL_OPS_CALL(res_ops->register_func_mac,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->mac, param->func_id));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REGISTER_FUNC_MAC, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_register_func_trust(void *priv, u16 func_id,
					bool trusted, bool *should_notify)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->register_func_trust,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id,
			    trusted, should_notify));
}

static int nbl_disp_chan_register_func_trust_req(void *priv, u16 func_id,
						 bool trusted, bool *should_notify)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_register_trust param;
	bool result;
	int ret;

	param.func_id = func_id;
	param.trusted = trusted;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_REGISTER_FUNC_TRUST, &param, sizeof(param),
		      &result, sizeof(result), 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	if (!ret)
		*should_notify = result;

	return ret;
}

static void nbl_disp_chan_register_func_trust_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_register_trust *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	bool notify = false;

	param = (struct nbl_chan_param_register_trust *)data;
	ret = NBL_OPS_CALL(res_ops->register_func_trust,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->func_id,
			   param->trusted, &notify));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REGISTER_FUNC_TRUST,
		     msg_id, ret, &notify, sizeof(notify));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_register_func_vlan(void *priv, u16 func_id, u16 vlan_tci,
				       u16 vlan_proto, bool *should_notify)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->register_func_vlan,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id, vlan_tci,
			    vlan_proto, should_notify));
}

static int nbl_disp_chan_register_func_vlan_req(void *priv, u16 func_id, u16 vlan_tci,
						u16 vlan_proto, bool *should_notify)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_register_vlan param;
	bool result;
	int ret;

	param.func_id = func_id;
	param.vlan_tci = vlan_tci;
	param.vlan_proto = vlan_proto;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_REGISTER_FUNC_VLAN, &param, sizeof(param),
		      &result, sizeof(result), 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	if (!ret)
		*should_notify = result;

	return ret;
}

static void nbl_disp_chan_register_func_vlan_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_register_vlan *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;
	bool notify = false;

	param = (struct nbl_chan_param_register_vlan *)data;
	ret = NBL_OPS_CALL(res_ops->register_func_vlan,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->func_id,
			   param->vlan_tci, param->vlan_proto, &notify));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REGISTER_FUNC_VLAN,
		     msg_id, ret, &notify, sizeof(notify));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_register_func_rate(void *priv, u16 func_id, int rate)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->register_func_rate,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id, rate));
}

static int nbl_disp_chan_register_func_rate_req(void *priv, u16 func_id, int tx_rate)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_set_tx_rate param;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.func_id = func_id;
	param.tx_rate = tx_rate;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_REGISTER_FUNC_RATE, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_register_func_rate_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_set_tx_rate *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_set_tx_rate *)data;
	ret = NBL_OPS_CALL(res_ops->register_func_rate,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->func_id, param->tx_rate));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REGISTER_FUNC_RATE, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_set_tx_rate(void *priv, u16 func_id, int tx_rate, int burst)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->set_tx_rate,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id, tx_rate, burst));
}

static int nbl_disp_chan_set_tx_rate_req(void *priv, u16 func_id, int tx_rate, int burst)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_set_txrx_rate param;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.func_id = func_id;
	param.txrx_rate = tx_rate;
	param.burst = burst;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_SET_TX_RATE, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_tx_rate_resp(void *priv, u16 src_id, u16 msg_id,
					   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_set_txrx_rate *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_set_txrx_rate *)data;
	ret = NBL_OPS_CALL(res_ops->set_tx_rate,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->func_id,
			   param->txrx_rate, param->burst));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_TX_RATE, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_set_rx_rate(void *priv, u16 func_id, int rx_rate, int burst)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->set_rx_rate,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id, rx_rate, burst));
}

static int nbl_disp_chan_set_rx_rate_req(void *priv, u16 func_id, int rx_rate, int burst)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_set_txrx_rate param;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.func_id = func_id;
	param.txrx_rate = rx_rate;
	param.burst = burst;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_SET_RX_RATE, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_rx_rate_resp(void *priv, u16 src_id, u16 msg_id,
					   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_set_txrx_rate *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_set_txrx_rate *)data;
	ret = NBL_OPS_CALL(res_ops->set_rx_rate,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->func_id,
			   param->txrx_rate, param->burst));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_RX_RATE, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_register_func_link_forced(void *priv, u16 func_id, u8 link_forced,
					      bool *should_notify)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->register_func_link_forced,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id,
			     link_forced, should_notify));
}

static int nbl_disp_chan_register_func_link_forced_req(void *priv, u16 func_id, u8 link_forced,
						       bool *should_notify)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_register_func_link_forced param;
	struct nbl_chan_param_register_func_link_forced result;
	int ret = 0;

	param.func_id = func_id;
	param.link_forced = link_forced;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_REGISTER_FUNC_LINK_FORCED, &param, sizeof(param),
		      &result, sizeof(result), 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	if (ret)
		return ret;

	*should_notify = result.should_notify;
	return 0;
}

static void nbl_disp_chan_register_func_link_forced_resp(void *priv, u16 src_id, u16 msg_id,
							 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_register_func_link_forced *param;
	struct nbl_chan_param_register_func_link_forced result = {0};
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_register_func_link_forced *)data;
	ret = NBL_OPS_CALL(res_ops->register_func_link_forced,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			   param->func_id, param->link_forced, &result.should_notify));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REGISTER_FUNC_LINK_FORCED,
		     msg_id, ret, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_link_forced(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_link_forced,
			    (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id));
}

static int nbl_disp_chan_get_link_forced_req(void *priv, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	int link_forced = 0;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_LINK_FORCED, &vsi_id, sizeof(vsi_id),
		      &link_forced, sizeof(link_forced), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return link_forced;
}

static void nbl_disp_chan_get_link_forced_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = 0;

	ret = NBL_OPS_CALL(res_ops->get_link_forced,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), *(u16 *)data));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_LINK_FORCED,
		     msg_id, NBL_CHAN_RESP_OK, &ret, sizeof(ret));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_get_driver_version(void *priv, char *ver, int len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->get_driver_version, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), ver, len));
}

static void nbl_disp_setup_rdma_id(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->setup_rdma_id, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static void nbl_disp_remove_rdma_id(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->remove_rdma_id, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_get_max_mtu(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->get_max_mtu, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	return ret;
}

static int nbl_disp_set_mtu(void *priv, u16 vsi_id, u16 mtu)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops;
	int ret = 0;

	res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	ret = NBL_OPS_CALL(res_ops->set_mtu, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, mtu));
	return ret;
}

static int nbl_disp_chan_set_mtu_req(void *priv, u16 vsi_id, u16 mtu)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_set_mtu param = {0};

	param.mtu = mtu;
	param.vsi_id = vsi_id;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_MTU_SET,
		      &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt),
				  &chan_send);
}

static void nbl_disp_chan_set_mtu_resp(void *priv,
				       u16 src_id, u16 msg_id, void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_set_mtu *param = NULL;
	int err = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_set_mtu *)data;
	err = NBL_OPS_CALL(res_ops->set_mtu,
			   (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id, param->mtu));

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_MTU_SET, msg_id, err, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_chan_get_fd_flow_req(void *priv, u16 vsi_id, u32 location,
					 enum nbl_chan_fdir_rule_type rule_type,
					 struct nbl_chan_param_fdir_replace *cmd)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_get_fd_flow param = {0};
	int ret = 0;

	param.vsi_id = vsi_id;
	param.location = location;
	param.rule_type = rule_type;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_FD_FLOW, &param,
		      sizeof(param), cmd, NBL_CHAN_FDIR_FLOW_RULE_SIZE, 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	if (ret)
		return ret;

	return 0;
}

static void nbl_disp_chan_get_fd_flow_resp(void *priv, u16 src_id, u16 msg_id,
					   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_get_fd_flow *param = NULL;
	struct nbl_chan_param_fdir_replace *result;
	struct nbl_chan_ack_info chan_ack;
	int ret = 0;

	result = kzalloc(NBL_CHAN_FDIR_FLOW_RULE_SIZE, GFP_KERNEL);
	if (!result) {
		ret = -ENOMEM;
		goto send_ack;
	}
	param = (struct nbl_chan_param_get_fd_flow *)data;
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_fd_flow,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id, param->location,
				param->rule_type, result);
send_ack:
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_FD_FLOW, msg_id,
		     ret, result, sizeof(*result) + result->tlv_length);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
	kfree(result);
}

static int nbl_disp_get_fd_flow(void *priv, u16 vsi_id, u32 location,
				enum nbl_chan_fdir_rule_type rule_type,
				struct nbl_chan_param_fdir_replace *cmd)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_fd_flow,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, location,
				 rule_type, cmd);
}

static int nbl_disp_chan_get_fd_flow_cnt_req(void *priv, enum nbl_chan_fdir_rule_type rule_type,
					     u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_fdir_flowcnt param;
	int result = 0, ret = 0;

	param.rule_type = rule_type;
	param.vsi = vsi_id;
	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_FD_FLOW_CNT, &param,
		      sizeof(param), &result, sizeof(result), 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	if (ret)
		return ret;

	return result;
}

static void nbl_disp_chan_get_fd_flow_cnt_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_fdir_flowcnt *param;
	int result = 0, err = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_fdir_flowcnt *)data;
	result = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_fd_flow_cnt,
				   NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				   param->rule_type, param->vsi);
	if (result < 0) {
		err = result;
		result = 0;
	}

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_FD_FLOW_CNT, msg_id,
		     err, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_fd_flow_cnt(void *priv, enum nbl_chan_fdir_rule_type rule_type, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_fd_flow_cnt,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), rule_type, vsi_id);
}

static int nbl_disp_chan_get_fd_flow_all_req(void *priv,
					     struct nbl_chan_param_get_fd_flow_all *param,
					     u32 *rule_locs)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_result_get_fd_flow_all *result = NULL;
	int ret = 0;

	result = (struct nbl_chan_result_get_fd_flow_all *)rule_locs;
	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_FD_FLOW_ALL, param,
		      sizeof(*param), result, sizeof(*result), 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	if (ret)
		goto send_fail;
send_fail:
	return ret;
}

static void nbl_disp_chan_get_fd_flow_all_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_get_fd_flow_all *param = NULL;
	struct nbl_chan_result_get_fd_flow_all *result = NULL;
	struct nbl_chan_ack_info chan_ack;
	int ret = 0;

	result = kzalloc(sizeof(*result), GFP_KERNEL);
	if (!result) {
		ret = -ENOMEM;
		goto send_ack;
	}

	param = (struct nbl_chan_param_get_fd_flow_all *)data;
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_fd_flow_all,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param, result->rule_locs);

send_ack:
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_FD_FLOW_ALL, msg_id,
		     ret, result, sizeof(*result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);

	kfree(result);
}

static int nbl_disp_get_fd_flow_all(void *priv, struct nbl_chan_param_get_fd_flow_all *param,
				    u32 *rule_locs)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_fd_flow_all,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param, rule_locs);
}

static int nbl_disp_chan_get_fd_flow_max_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	int ret = 0, result = 0;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_FD_FLOW_MAX, NULL, 0, &result, sizeof(result), 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
	if (ret)
		return ret;

	return result;
}

static void nbl_disp_chan_get_fd_flow_max_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int result = 0, err = NBL_CHAN_RESP_OK;

	result = NBL_OPS_CALL(res_ops->get_fd_flow_max, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
	if (result < 0) {
		err = result;
		result = 0;
	}

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_FD_FLOW_MAX, msg_id,
		     err, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_fd_flow_max(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_fd_flow_max, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static int nbl_disp_chan_replace_fd_flow_req(void *priv, struct nbl_chan_param_fdir_replace *info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_REPLACE_FD_FLOW, info,
		      sizeof(struct nbl_chan_param_fdir_replace) + info->tlv_length, NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_replace_fd_flow_resp(void *priv, u16 src_id, u16 msg_id,
					       void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_fdir_replace *param = NULL;
	struct nbl_chan_ack_info chan_ack;
	int ret = 0;

	param = (struct nbl_chan_param_fdir_replace *)data;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->replace_fd_flow,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REPLACE_FD_FLOW, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_replace_fd_flow(void *priv, struct nbl_chan_param_fdir_replace *info)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->replace_fd_flow,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), info);
}

static int nbl_disp_chan_remove_fd_flow_req(void *priv, enum nbl_chan_fdir_rule_type rule_type,
					    u32 loc, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_param_fdir_del param = {0};
	struct nbl_chan_send_info chan_send = {0};

	param.rule_type = rule_type;
	param.location = loc;
	param.vsi = vsi_id;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_REMOVE_FD_FLOW, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_remove_fd_flow_resp(void *priv, u16 src_id, u16 msg_id,
					      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_fdir_del *param = NULL;
	struct nbl_chan_ack_info chan_ack;
	int ret = 0;

	param = (struct nbl_chan_param_fdir_del *)data;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_fd_flow,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->rule_type,
				param->location, param->vsi);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REMOVE_FD_FLOW, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_remove_fd_flow(void *priv, enum nbl_chan_fdir_rule_type rule_type,
				   u32 loc, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->remove_fd_flow,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), rule_type, loc, vsi_id);
}

static int nbl_disp_chan_config_fd_flow_state_req(void *priv,
						  enum nbl_chan_fdir_rule_type rule_type,
						  u16 vsi_id, u16 state)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	struct nbl_chan_param_config_fd_flow_state param = {0};
	struct nbl_chan_send_info chan_send = {0};

	param.rule_type = rule_type;
	param.vsi_id = vsi_id;
	param.state = state;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_CFG_FD_FLOW_STATE, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_config_fd_flow_state_resp(void *priv, u16 src_id, u16 msg_id,
						    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_config_fd_flow_state *param = NULL;
	struct nbl_chan_ack_info chan_ack;
	int ret = 0;

	param = (struct nbl_chan_param_config_fd_flow_state *)data;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->config_fd_flow_state,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->rule_type,
				param->vsi_id, param->state);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CFG_FD_FLOW_STATE, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_config_fd_flow_state(void *priv, enum nbl_chan_fdir_rule_type rule_type,
					 u16 vsi_id, u16 state)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->config_fd_flow_state,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), rule_type, vsi_id, state);
}

static void nbl_disp_cfg_fd_update_event(void *priv, bool enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->cfg_fd_update_event,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), enable);
}

static void nbl_disp_cfg_mirror_outputport_event(void *priv, bool enable)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->cfg_mirror_outputport_event,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), enable);
}

static void nbl_disp_dump_fd_flow(void *priv, struct seq_file *m)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->dump_fd_flow,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), m);
}

static void nbl_disp_chan_get_xdp_queue_info_req(void *priv, u16 *queue_num, u16 *queue_size,
						 u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_get_queue_info result = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common), NBL_CHAN_MSG_GET_XDP_QUEUE_INFO,
		      &vsi_id, sizeof(vsi_id), &result, sizeof(result), 1);

	if (!chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send)) {
		*queue_num = result.queue_num;
		*queue_size = result.queue_size;
	}
}

static void nbl_disp_chan_get_xdp_queue_info_resp(void *priv, u16 src_id, u16 msg_id,
						  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_get_queue_info result = {0};
	int ret = NBL_CHAN_RESP_OK;

	NBL_OPS_CALL(res_ops->get_xdp_queue_info,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), &result.queue_num,
		      &result.queue_size, *(u16 *)data));
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_XDP_QUEUE_INFO, msg_id,
		     ret, &result, sizeof(result));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_get_xdp_queue_info(void *priv, u16 *queue_num, u16 *queue_size, u16 vsi_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->get_xdp_queue_info,
		     (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), queue_num, queue_size, vsi_id));
}

static void nbl_disp_set_hw_status(void *priv, enum nbl_hw_status hw_status)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_hw_status,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), hw_status);
}

static void nbl_disp_get_active_func_bitmaps(void *priv, unsigned long *bitmap, int max_func)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_active_func_bitmaps,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), bitmap, max_func);
}

static int nbl_disp_set_tc_wgt(void *priv, u16 vsi_id, u8 *weight, u8 num_tc)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_tc_wgt,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, weight, num_tc);
}

static int nbl_disp_chan_set_tc_wgt_req(void *priv, u16 vsi_id, u8 *weight, u8 num_tc)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_set_tc_wgt param;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	param.num_tc = num_tc;
	memcpy(param.weight, weight, num_tc);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_SET_TC_WGT, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_tc_wgt_resp(void *priv, u16 src_id, u16 msg_id,
					  void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_set_tc_wgt *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_set_tc_wgt *)data;
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_tc_wgt,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				param->vsi_id, param->weight, param->num_tc);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_TC_WGT, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_configure_rdma_bw(void *priv, u8 eth_id, int rdma_bw)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->configure_rdma_bw,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, rdma_bw);
}

static int nbl_disp_chan_configure_rdma_bw_req(void *priv, u8 eth_id, int rdma_bw)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_configure_rdma_bw param;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.eth_id = eth_id;
	param.rdma_bw = rdma_bw;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_CONFIGURE_RDMA_BW, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_configure_rdma_bw_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_configure_rdma_bw *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_configure_rdma_bw *)data;
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->configure_rdma_bw,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->eth_id, param->rdma_bw);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CONFIGURE_RDMA_BW, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_configure_qos(void *priv, u8 eth_id, u8 *pfc, u8 trust, u8 *dscp2prio_map)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int ret;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->configure_qos,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, pfc,
				trust, dscp2prio_map);
	if (ret)
		return ret;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_eth_pfc,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, pfc);

	return ret;
}

static int nbl_disp_chan_configure_qos_req(void *priv, u8 eth_id, u8 *pfc,
					   u8 trust, u8 *dscp2prio_map)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_configure_qos param;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.eth_id = eth_id;
	memcpy(param.pfc, pfc, NBL_MAX_PFC_PRIORITIES);
	memcpy(param.dscp2prio_map, dscp2prio_map, NBL_DSCP_MAX);
	param.trust = trust;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_CONFIGURE_QOS, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_configure_qos_resp(void *priv, u16 src_id, u16 msg_id,
					     void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_configure_qos *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_configure_qos *)data;
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->configure_qos,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				param->eth_id, param->pfc, param->trust, param->dscp2prio_map);
	if (ret)
		goto send_ack;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_eth_pfc,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->eth_id, param->pfc);

send_ack:
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CONFIGURE_QOS, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_set_pfc_buffer_size(void *priv, u8 eth_id, u8 prio, int xoff, int xon)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int ret;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_pfc_buffer_size,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, prio, xoff, xon);

	return ret;
}

static int nbl_disp_chan_set_pfc_buffer_size_req(void *priv, u8 eth_id, u8 prio, int xoff, int xon)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_set_pfc_buffer_size param;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.eth_id = eth_id;
	param.prio = prio;
	param.xoff = xoff;
	param.xon = xon;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_SET_PFC_BUFFER_SIZE, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_pfc_buffer_size_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_set_pfc_buffer_size *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_set_pfc_buffer_size *)data;
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_pfc_buffer_size,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				param->eth_id, param->prio, param->xoff, param->xon);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_PFC_BUFFER_SIZE, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_pfc_buffer_size(void *priv, u8 eth_id, u8 prio, int *xoff, int *xon)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int ret;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_pfc_buffer_size,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), eth_id, prio, xoff, xon);

	return ret;
}

static int
nbl_disp_chan_get_pfc_buffer_size_req(void *priv, u8 eth_id, u8 prio, int *xoff, int *xon)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_get_pfc_buffer_size param = {0};
	struct nbl_chan_param_get_pfc_buffer_size_resp resp;
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	int ret;

	param.eth_id = eth_id;
	param.prio = prio;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_PFC_BUFFER_SIZE, &param, sizeof(param),
		      &resp, sizeof(resp), 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	*xoff = resp.xoff;
	*xon = resp.xon;

	return ret;
}

static void nbl_disp_chan_get_pfc_buffer_size_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_get_pfc_buffer_size *param;
	struct nbl_chan_param_get_pfc_buffer_size_resp resp;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_get_pfc_buffer_size *)data;
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_pfc_buffer_size,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				param->eth_id, param->prio, &resp.xoff, &resp.xon);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_PFC_BUFFER_SIZE, msg_id, ret,
		     &resp, sizeof(resp));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_set_rate_limit(void *priv, enum nbl_traffic_type type, u32 rate)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	int ret;

	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_rate_limit,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), 0, type, rate);

	return ret;
}

static int
nbl_disp_chan_set_rate_limit_req(void *priv, enum nbl_traffic_type type, u32 rate)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_set_rate_limit param = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.type = type;
	param.rate = rate;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_SET_RATE_LIMIT, &param, sizeof(param), NULL, 0, 1);
	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_set_rate_limit_resp(void *priv, u16 src_id, u16 msg_id,
					      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_set_rate_limit *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_set_rate_limit *)data;
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->set_rate_limit,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				src_id, param->type, param->rate);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_SET_RATE_LIMIT, msg_id, ret,
		     NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_register_dev_name(void *priv, u16 vsi_id, char *name)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	 NBL_OPS_CALL_LOCK(disp_mgt, res_ops->register_dev_name,
			   NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, name);
}

static void
nbl_disp_chan_register_dev_name_req(void *priv, u16 vsi_id, char *name)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_pf_name param = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	strscpy(param.dev_name, name, IFNAMSIZ);

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_REGISTER_PF_NAME, &param, sizeof(param), NULL, 0, 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void nbl_disp_chan_register_dev_name_resp(void *priv, u16 src_id, u16 msg_id,
						 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_pf_name *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_pf_name *)data;
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->register_dev_name,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id, param->dev_name);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_REGISTER_PF_NAME, msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static void nbl_disp_get_dev_name(void *priv, u16 vsi_id, char *name)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	 NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_dev_name,
			   NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vsi_id, name);
}

static void
nbl_disp_chan_get_dev_name_req(void *priv, u16 vsi_id, char *name)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_pf_name param = {0};
	struct nbl_chan_param_pf_name resp = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vsi_id = vsi_id;
	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_PF_NAME, &param, sizeof(param), &resp, sizeof(resp), 1);
	chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	strscpy(name, resp.dev_name, IFNAMSIZ);
}

static void nbl_disp_chan_get_dev_name_resp(void *priv, u16 src_id, u16 msg_id,
					    void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_pf_name *param;
	struct nbl_chan_param_pf_name resp = {0};
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_pf_name *)data;
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_dev_name,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vsi_id, resp.dev_name);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_PF_NAME, msg_id, ret, &resp, sizeof(resp));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_get_mirror_table_id(void *priv, u16 vsi_id, int dir,
					bool mirror_en, u8 *mt_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_mirror_table_id,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
				 vsi_id, dir, mirror_en, mt_id);
}

static int nbl_disp_chan_get_mirror_table_id_req(void *priv, u16 vsi_id, int dir,
						 bool mirror_en, u8 *mt_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_get_mirror_table_id param = {0};
	struct nbl_chan_param_get_mirror_table_id resp = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	int ret;

	param.vsi_id = vsi_id;
	param.dir = dir;
	param.mirror_en = mirror_en;
	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_GET_MIRROR_TABLE_ID, &param, sizeof(param),
		      &resp, sizeof(resp), 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	*mt_id = resp.mt_id;

	return ret;
}

static void nbl_disp_chan_get_mirror_table_id_resp(void *priv, u16 src_id, u16 msg_id,
						   void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_get_mirror_table_id *param;
	struct nbl_chan_param_get_mirror_table_id resp = {0};
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_get_mirror_table_id *)data;
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->get_mirror_table_id,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			  param->vsi_id, param->dir, param->mirror_en, &resp.mt_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GET_MIRROR_TABLE_ID, msg_id, ret,
		     &resp, sizeof(resp));
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_configure_mirror(void *priv, u16 func_id, bool mirror_en, int dir,
				     u8 mt_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->configure_mirror,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id,
				 mirror_en, dir, mt_id);
}

static int nbl_disp_chan_configure_mirror_req(void *priv, u16 func_id, bool mirror_en,
					      int dir, u8 mt_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_mirror param = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	int ret;

	param.mirror_en = mirror_en;
	param.dir = dir;
	param.mt_id = mt_id;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_CONFIGURE_MIRROR, &param, sizeof(param),
		      NULL, 0, 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return ret;
}

static void nbl_disp_chan_configure_mirror_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_mirror *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_mirror *)data;
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->configure_mirror,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			  src_id, param->mirror_en, param->dir, param->mt_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CONFIGURE_MIRROR, msg_id, ret,
		     NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_configure_mirror_table(void *priv, bool mirror_en,
					   u16 func_id, u8 mt_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->configure_mirror_table,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), mirror_en,
				 func_id, mt_id);
}

static int nbl_disp_chan_configure_mirror_table_req(void *priv, bool mirror_en,
						    u16 func_id, u8 mt_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_mirror_table param = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	int ret;

	param.mirror_en = mirror_en;
	param.func_id = func_id;
	param.mt_id = mt_id;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_CONFIGURE_MIRROR_TABLE, &param, sizeof(param),
		      NULL, 0, 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return ret;
}

static void nbl_disp_chan_configure_mirror_table_resp(void *priv, u16 src_id, u16 msg_id,
						      void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_param_mirror_table *param;
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	param = (struct nbl_chan_param_mirror_table *)data;
	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->configure_mirror_table,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt),
			  param->mirror_en, param->func_id, param->mt_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CONFIGURE_MIRROR_TABLE, msg_id, ret,
		     NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_clear_mirror_cfg(void *priv, u16 func_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->clear_mirror_cfg,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), func_id);
}

static int nbl_disp_chan_clear_mirror_cfg_req(void *priv, u16 func_id)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);
	int ret;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_CLEAR_MIRROR_CFG, NULL, 0, NULL, 0, 1);
	ret = chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);

	return ret;
}

static void nbl_disp_chan_clear_mirror_cfg_resp(void *priv, u16 src_id, u16 msg_id,
						void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	int ret = NBL_CHAN_RESP_OK;

	NBL_OPS_CALL_LOCK(disp_mgt, res_ops->clear_mirror_cfg,
			  NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), src_id);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CLEAR_MIRROR_CFG, msg_id, ret,
		     NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static int nbl_disp_check_flow_table_spec(void *priv, u16 vlan_list_cnt,
					  u16 unicast_mac_cnt, u16 multi_mac_cnt)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL_LOCK(disp_mgt, res_ops->check_flow_table_spec,
				 NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), vlan_list_cnt,
				 unicast_mac_cnt, multi_mac_cnt);
}

static int
nbl_disp_chan_check_flow_table_spec_req(void *priv, u16 vlan_list_cnt,
					u16 unicast_mac_cnt, u16 multi_mac_cnt)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_chan_param_check_flow_spec param = {0};
	struct nbl_common_info *common = NBL_DISP_MGT_TO_COMMON(disp_mgt);

	param.vlan_list_cnt = vlan_list_cnt;
	param.unicast_mac_cnt = unicast_mac_cnt;
	param.multi_mac_cnt = multi_mac_cnt;

	NBL_CHAN_SEND(chan_send, NBL_COMMON_TO_MGT_PF(common),
		      NBL_CHAN_MSG_CHECK_FLOWTABLE_SPEC, &param,
		      sizeof(param), NULL, 0, 1);

	return chan_ops->send_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_send);
}

static void
nbl_disp_chan_check_flow_table_spec_resp(void *priv, u16 src_id, u16 msg_id,
					 void *data, u32 data_len)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);
	struct nbl_chan_ack_info chan_ack;
	struct nbl_chan_param_check_flow_spec *param = {0};
	int ret;

	param = (struct nbl_chan_param_check_flow_spec *)data;
	ret = NBL_OPS_CALL_LOCK(disp_mgt, res_ops->check_flow_table_spec,
				NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), param->vlan_list_cnt,
				param->unicast_mac_cnt, param->multi_mac_cnt);
	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_CHECK_FLOWTABLE_SPEC,
		     msg_id, ret, NULL, 0);
	chan_ops->send_ack(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt), &chan_ack);
}

static u32 nbl_disp_get_dvn_desc_req(void *priv)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	return NBL_OPS_CALL(res_ops->get_dvn_desc_req, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt)));
}

static void nbl_disp_set_dvn_desc_req(void *priv, u32 desc_req)
{
	struct nbl_dispatch_mgt *disp_mgt = (struct nbl_dispatch_mgt *)priv;
	struct nbl_resource_ops *res_ops = NBL_DISP_MGT_TO_RES_OPS(disp_mgt);

	NBL_OPS_CALL(res_ops->set_dvn_desc_req, (NBL_DISP_MGT_TO_RES_PRIV(disp_mgt), desc_req));
}

/* NBL_DISP_SET_OPS(disp_op_name, res_func, ctrl_lvl, msg_type, msg_req, msg_resp)
 * ctrl_lvl is to define when this disp_op should go directly to res_op, not sending a channel msg.
 *
 * Use X Macros to reduce codes in channel_op and disp_op setup/remove
 */
#define NBL_DISP_OPS_TBL									\
do {												\
	NBL_DISP_SET_OPS(init_chip_module, nbl_disp_init_chip_module,				\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(get_resource_pt_ops, nbl_disp_get_res_pt_ops,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(queue_init, nbl_disp_queue_init,					\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(vsi_init, nbl_disp_vsi_init,						\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(configure_msix_map, nbl_disp_configure_msix_map,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CONFIGURE_MSIX_MAP,		\
			 nbl_disp_chan_configure_msix_map_req,					\
			 nbl_disp_chan_configure_msix_map_resp);				\
	NBL_DISP_SET_OPS(destroy_msix_map, nbl_disp_destroy_msix_map,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_DESTROY_MSIX_MAP,			\
			 nbl_disp_chan_destroy_msix_map_req,					\
			 nbl_disp_chan_destroy_msix_map_resp);					\
	NBL_DISP_SET_OPS(enable_mailbox_irq, nbl_disp_enable_mailbox_irq,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_MAILBOX_ENABLE_IRQ,		\
			 nbl_disp_chan_enable_mailbox_irq_req,					\
			 nbl_disp_chan_enable_mailbox_irq_resp);				\
	NBL_DISP_SET_OPS(enable_abnormal_irq, nbl_disp_enable_abnormal_irq,			\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(enable_adminq_irq, nbl_disp_enable_adminq_irq,				\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(get_global_vector, nbl_disp_get_global_vector,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_GLOBAL_VECTOR,			\
			 nbl_disp_chan_get_global_vector_req,					\
			 nbl_disp_chan_get_global_vector_resp);					\
	NBL_DISP_SET_OPS(get_msix_entry_id, nbl_disp_get_msix_entry_id,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(alloc_rings, nbl_disp_alloc_rings,					\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(remove_rings, nbl_disp_remove_rings,					\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(start_tx_ring, nbl_disp_start_tx_ring,					\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(stop_tx_ring, nbl_disp_stop_tx_ring,					\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(start_rx_ring, nbl_disp_start_rx_ring,					\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(stop_rx_ring, nbl_disp_stop_rx_ring,					\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(kick_rx_ring, nbl_disp_kick_rx_ring,					\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(dump_ring, nbl_disp_dump_ring,						\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(dump_ring_stats, nbl_disp_dump_ring_stats,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(set_rings_xdp_prog, nbl_disp_set_rings_xdp_prog,			\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(register_xdp_rxq, nbl_disp_register_xdp_rxq,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(unregister_xdp_rxq, nbl_disp_unregister_xdp_rxq,			\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(get_vector_napi, nbl_disp_get_vector_napi,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(set_vector_info, nbl_disp_set_vector_info,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(register_vsi_ring, nbl_disp_register_vsi_ring,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(register_net, nbl_disp_register_net,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REGISTER_NET,			\
			 nbl_disp_chan_register_net_req, nbl_disp_chan_register_net_resp);	\
	NBL_DISP_SET_OPS(unregister_net, nbl_disp_unregister_net,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_UNREGISTER_NET,			\
			 nbl_disp_chan_unregister_net_req, nbl_disp_chan_unregister_net_resp);	\
	NBL_DISP_SET_OPS(alloc_txrx_queues, nbl_disp_alloc_txrx_queues,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ALLOC_TXRX_QUEUES,			\
			 nbl_disp_chan_alloc_txrx_queues_req,					\
			 nbl_disp_chan_alloc_txrx_queues_resp);					\
	NBL_DISP_SET_OPS(free_txrx_queues, nbl_disp_free_txrx_queues,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_FREE_TXRX_QUEUES,			\
			 nbl_disp_chan_free_txrx_queues_req,					\
			 nbl_disp_chan_free_txrx_queues_resp);					\
	NBL_DISP_SET_OPS(register_vsi2q, nbl_disp_register_vsi2q,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REGISTER_VSI2Q,			\
			 nbl_disp_chan_register_vsi2q_req,					\
			 nbl_disp_chan_register_vsi2q_resp);					\
	NBL_DISP_SET_OPS(setup_q2vsi, nbl_disp_setup_q2vsi,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SETUP_Q2VSI,			\
			 nbl_disp_chan_setup_q2vsi_req,						\
			 nbl_disp_chan_setup_q2vsi_resp);					\
	NBL_DISP_SET_OPS(remove_q2vsi, nbl_disp_remove_q2vsi,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REMOVE_Q2VSI,			\
			 nbl_disp_chan_remove_q2vsi_req,					\
			 nbl_disp_chan_remove_q2vsi_resp);					\
	NBL_DISP_SET_OPS(setup_rss, nbl_disp_setup_rss,						\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SETUP_RSS,				\
			 nbl_disp_chan_setup_rss_req,						\
			 nbl_disp_chan_setup_rss_resp);						\
	NBL_DISP_SET_OPS(remove_rss, nbl_disp_remove_rss,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REMOVE_RSS,			\
			 nbl_disp_chan_remove_rss_req,						\
			 nbl_disp_chan_remove_rss_resp);					\
	NBL_DISP_SET_OPS(setup_queue, nbl_disp_setup_queue,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SETUP_QUEUE,			\
			 nbl_disp_chan_setup_queue_req, nbl_disp_chan_setup_queue_resp);	\
	NBL_DISP_SET_OPS(remove_queue, nbl_disp_remove_queue,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REMOVE_QUEUE,			\
			 nbl_disp_chan_remove_queue_req, nbl_disp_chan_remove_queue_resp);	\
	NBL_DISP_SET_OPS(remove_all_queues, nbl_disp_remove_all_queues,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REMOVE_ALL_QUEUES,			\
			 nbl_disp_chan_remove_all_queues_req,					\
			 nbl_disp_chan_remove_all_queues_resp);					\
	NBL_DISP_SET_OPS(cfg_dsch, nbl_disp_cfg_dsch,						\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_DSCH,				\
			 nbl_disp_chan_cfg_dsch_req, nbl_disp_chan_cfg_dsch_resp);		\
	NBL_DISP_SET_OPS(setup_cqs, nbl_disp_setup_cqs,						\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SETUP_CQS,				\
			 nbl_disp_chan_setup_cqs_req, nbl_disp_chan_setup_cqs_resp);		\
	NBL_DISP_SET_OPS(remove_cqs, nbl_disp_remove_cqs,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REMOVE_CQS,			\
			 nbl_disp_chan_remove_cqs_req, nbl_disp_chan_remove_cqs_resp);		\
	NBL_DISP_SET_OPS(cfg_qdisc_mqprio, nbl_disp_cfg_qdisc_mqprio,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_QDISC_MQPRIO,			\
			 nbl_disp_chan_cfg_qdisc_mqprio_req,					\
			 nbl_disp_chan_cfg_qdisc_mqprio_resp);					\
	NBL_DISP_SET_OPS(get_msix_irq_enable_info, nbl_disp_get_msix_irq_enable_info,		\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(add_macvlan, nbl_disp_add_macvlan,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ADD_MACVLAN,			\
			 nbl_disp_chan_add_macvlan_req, nbl_disp_chan_add_macvlan_resp);	\
	NBL_DISP_SET_OPS(del_macvlan, nbl_disp_del_macvlan,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_DEL_MACVLAN,			\
			 nbl_disp_chan_del_macvlan_req, nbl_disp_chan_del_macvlan_resp);	\
	NBL_DISP_SET_OPS(add_multi_rule, nbl_disp_add_multi_rule,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ADD_MULTI_RULE,			\
			 nbl_disp_chan_add_multi_rule_req, nbl_disp_chan_add_multi_rule_resp);	\
	NBL_DISP_SET_OPS(del_multi_rule, nbl_disp_del_multi_rule,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_DEL_MULTI_RULE,			\
			 nbl_disp_chan_del_multi_rule_req, nbl_disp_chan_del_multi_rule_resp);	\
	NBL_DISP_SET_OPS(cfg_multi_mcast, nbl_disp_cfg_multi_mcast,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_MULTI_MCAST_RULE,		\
			 nbl_disp_chan_cfg_multi_mcast_req, nbl_disp_chan_cfg_multi_mcast_resp);\
	NBL_DISP_SET_OPS(setup_multi_group, nbl_disp_setup_multi_group,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SETUP_MULTI_GROUP,			\
			 nbl_disp_chan_setup_multi_group_req,					\
			 nbl_disp_chan_setup_multi_group_resp);					\
	NBL_DISP_SET_OPS(remove_multi_group, nbl_disp_remove_multi_group,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REMOVE_MULTI_GROUP,		\
			 nbl_disp_chan_remove_multi_group_req,					\
			 nbl_disp_chan_remove_multi_group_resp);				\
	NBL_DISP_SET_OPS(dump_flow, nbl_disp_dump_flow,						\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(get_vsi_id, nbl_disp_get_vsi_id,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_VSI_ID,			\
			 nbl_disp_chan_get_vsi_id_req, nbl_disp_chan_get_vsi_id_resp);		\
	NBL_DISP_SET_OPS(get_eth_id, nbl_disp_get_eth_id,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_ETH_ID,			\
			 nbl_disp_chan_get_eth_id_req, nbl_disp_chan_get_eth_id_resp);		\
	NBL_DISP_SET_OPS(enable_lag_protocol, nbl_disp_enable_lag_protocol,			\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(cfg_lag_hash_algorithm, nbl_disp_cfg_lag_hash_algorithm,		\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_LAG_HASH_ALGORITHM,		\
			 nbl_disp_chan_cfg_lag_hash_algorithm_req,				\
			 nbl_disp_chan_cfg_lag_hash_algorithm_resp);				\
	NBL_DISP_SET_OPS(cfg_lag_member_fwd, nbl_disp_cfg_lag_member_fwd,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_LAG_MEMBER_FWD,		\
			 nbl_disp_chan_cfg_lag_member_fwd_req,					\
			 nbl_disp_chan_cfg_lag_member_fwd_resp);				\
	NBL_DISP_SET_OPS(cfg_lag_member_list, nbl_disp_cfg_lag_member_list,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_LAG_MEMBER_LIST,		\
			 nbl_disp_chan_cfg_lag_member_list_req,					\
			 nbl_disp_chan_cfg_lag_member_list_resp);				\
	NBL_DISP_SET_OPS(cfg_lag_member_up_attr, nbl_disp_cfg_lag_member_up_attr,		\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_LAG_MEMBER_UP_ATTR,		\
			 nbl_disp_chan_cfg_lag_member_up_attr_req,				\
			 nbl_disp_chan_cfg_lag_member_up_attr_resp);				\
	NBL_DISP_SET_OPS(add_lldp_flow, nbl_disp_add_lldp_flow,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ADD_LLDP_FLOW,			\
			 nbl_disp_chan_add_lldp_flow_req, nbl_disp_chan_add_lldp_flow_resp);	\
	NBL_DISP_SET_OPS(del_lldp_flow, nbl_disp_del_lldp_flow,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_DEL_LLDP_FLOW,			\
			 nbl_disp_chan_del_lldp_flow_req, nbl_disp_chan_del_lldp_flow_resp);	\
	NBL_DISP_SET_OPS(add_lag_flow, nbl_disp_add_lag_flow,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ADD_LAG_FLOW,			\
			 nbl_disp_chan_add_lag_flow_req, nbl_disp_chan_add_lag_flow_resp);	\
	NBL_DISP_SET_OPS(del_lag_flow, nbl_disp_del_lag_flow,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_DEL_LAG_FLOW,			\
			 nbl_disp_chan_del_lag_flow_req, nbl_disp_chan_del_lag_flow_resp);	\
	NBL_DISP_SET_OPS(cfg_duppkt_info, nbl_disp_cfg_duppkt_info,				\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(cfg_duppkt_mcc, nbl_disp_cfg_duppkt_mcc,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_DUPPKT_MCC,			\
			 nbl_disp_chan_cfg_duppkt_mcc_req, nbl_disp_chan_cfg_duppkt_mcc_resp);	\
	NBL_DISP_SET_OPS(cfg_bond_shaping, nbl_disp_cfg_bond_shaping,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_BOND_SHAPING,			\
			 nbl_disp_chan_cfg_bond_shaping_req,					\
			 nbl_disp_chan_cfg_bond_shaping_resp);					\
	NBL_DISP_SET_OPS(cfg_bgid_back_pressure, nbl_disp_cfg_bgid_back_pressure,		\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_BGID_BACK_PRESSURE,		\
			 nbl_disp_chan_cfg_bgid_back_pressure_req,				\
			 nbl_disp_chan_cfg_bgid_back_pressure_resp);				\
	NBL_DISP_SET_OPS(set_promisc_mode, nbl_disp_set_promisc_mode,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_PROSISC_MODE,			\
			 nbl_disp_chan_set_promisc_mode_req,					\
			 nbl_disp_chan_set_promisc_mode_resp);					\
	NBL_DISP_SET_OPS(set_spoof_check_addr, nbl_disp_set_spoof_check_addr,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_SPOOF_CHECK_ADDR,		\
			 nbl_disp_chan_set_spoof_check_addr_req,				\
			 nbl_disp_chan_set_spoof_check_addr_resp);				\
	NBL_DISP_SET_OPS(set_vf_spoof_check, nbl_disp_set_vf_spoof_check,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_VF_SPOOF_CHECK,		\
			 nbl_disp_chan_set_vf_spoof_check_req,					\
			 nbl_disp_chan_set_vf_spoof_check_resp);				\
	NBL_DISP_SET_OPS(get_base_mac_addr, nbl_disp_get_base_mac_addr,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_BASE_MAC_ADDR,			\
			 nbl_disp_chan_get_base_mac_addr_req,					\
			 nbl_disp_chan_get_base_mac_addr_resp);					\
	NBL_DISP_SET_OPS(get_eth_mac_stats, nbl_disp_get_eth_mac_stats,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_ETH_MAC_STATS,			\
			 nbl_disp_chan_get_eth_mac_stats_req,					\
			 nbl_disp_chan_get_eth_mac_stats_resp);					\
	NBL_DISP_SET_OPS(get_rmon_stats, nbl_disp_get_rmon_stats,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_RMON_STATS,			\
			 nbl_disp_chan_get_rmon_stats_req,					\
			 nbl_disp_chan_get_rmon_stats_resp);					\
	NBL_DISP_SET_OPS(get_tx_headroom, nbl_disp_get_tx_headroom,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(get_rep_feature, nbl_disp_get_rep_feature,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(set_eswitch_mode, nbl_disp_set_eswitch_mode,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(get_eswitch_mode, nbl_disp_get_eswitch_mode,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(alloc_rep_data, nbl_disp_alloc_rep_data,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(free_rep_data, nbl_disp_free_rep_data,					\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(set_rep_netdev_info, nbl_disp_set_rep_netdev_info,			\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(unset_rep_netdev_info, nbl_disp_unset_rep_netdev_info,			\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(get_rep_netdev_info, nbl_disp_get_rep_netdev_info,			\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(get_rep_stats, nbl_disp_get_rep_stats,					\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_rep_index, nbl_disp_get_rep_index,					\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_firmware_version, nbl_disp_get_firmware_version,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_FIRMWARE_VERSION,		\
			 nbl_disp_chan_get_firmware_version_req,				\
			 nbl_disp_chan_get_firmware_version_resp);				\
	NBL_DISP_SET_OPS(get_driver_info, nbl_disp_get_driver_info,				\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_queue_stats, nbl_disp_get_queue_stats,				\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_queue_err_stats, nbl_disp_get_queue_err_stats,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_QUEUE_ERR_STATS,		\
			 nbl_disp_chan_get_queue_err_stats_req,					\
			 nbl_disp_chan_get_queue_err_stats_resp);				\
	NBL_DISP_SET_OPS(get_net_stats, nbl_disp_get_net_stats,					\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_private_stat_len, nbl_disp_get_private_stat_len,			\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_private_stat_data, nbl_disp_get_private_stat_data,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_ETH_STATS,			\
			 nbl_disp_get_private_stat_data_req,					\
			 nbl_disp_chan_get_private_stat_data_resp);				\
	NBL_DISP_SET_OPS(get_eth_ctrl_stats, nbl_disp_get_eth_ctrl_stats,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_ETH_CTRL_STATS,		\
			 nbl_disp_chan_get_eth_ctrl_stats_req,					\
			 nbl_disp_chan_get_eth_ctrl_stats_resp);				\
	NBL_DISP_SET_OPS(get_pause_stats, nbl_disp_get_pause_stats,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_PAUSE_STATS,			\
			 nbl_disp_chan_get_pause_stats_req,					\
			 nbl_disp_chan_get_pause_stats_resp);					\
	NBL_DISP_SET_OPS(fill_private_stat_strings, nbl_disp_fill_private_stat_strings,		\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_eth_abnormal_stats, nbl_disp_get_eth_abnormal_stats,		\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_ETH_ABNORMAL_STATS,		\
			 nbl_disp_chan_get_eth_abnormal_stats_req,				\
			 nbl_disp_chan_get_eth_abnormal_stats_resp);				\
	NBL_DISP_SET_OPS(get_max_desc_num, nbl_disp_get_max_desc_num,				\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_min_desc_num, nbl_disp_get_min_desc_num,				\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_tx_desc_num, nbl_disp_get_tx_desc_num,				\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_rx_desc_num, nbl_disp_get_rx_desc_num,				\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(set_tx_desc_num, nbl_disp_set_tx_desc_num,				\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(set_rx_desc_num, nbl_disp_set_rx_desc_num,				\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(set_eth_loopback, nbl_disp_set_eth_loopback,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_ETH_LOOPBACK,			\
			 nbl_disp_chan_set_eth_loopback_req,					\
			 nbl_disp_chan_set_eth_loopback_resp);					\
	NBL_DISP_SET_OPS(clean_rx_lb_test, nbl_disp_clean_rx_lb_test,				\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_coalesce, nbl_disp_get_coalesce,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_COALESCE,			\
			 nbl_disp_chan_get_coalesce_req,					\
			 nbl_disp_chan_get_coalesce_resp);					\
	NBL_DISP_SET_OPS(set_coalesce, nbl_disp_set_coalesce,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_COALESCE,			\
			 nbl_disp_chan_set_coalesce_req,					\
			 nbl_disp_chan_set_coalesce_resp);					\
	NBL_DISP_SET_OPS(get_intr_suppress_level, nbl_disp_get_intr_suppress_level,		\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(set_intr_suppress_level, nbl_disp_set_intr_suppress_level,		\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_INTL_SUPPRESS_LEVEL,		\
			 nbl_disp_chan_set_intr_suppress_level_req,				\
			 nbl_disp_chan_set_intr_suppress_level_resp);				\
	NBL_DISP_SET_OPS(get_rxfh_indir_size, nbl_disp_get_rxfh_indir_size,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_RXFH_INDIR_SIZE,		\
			 nbl_disp_chan_get_rxfh_indir_size_req,					\
			 nbl_disp_chan_get_rxfh_indir_size_resp);				\
	NBL_DISP_SET_OPS(get_rxfh_rss_key_size, nbl_disp_get_rxfh_rss_key_size,			\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_rxfh_indir, nbl_disp_get_rxfh_indir,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_RXFH_INDIR,			\
			 nbl_disp_chan_get_rxfh_indir_req, nbl_disp_chan_get_rxfh_indir_resp);	\
	NBL_DISP_SET_OPS(set_rxfh_indir, nbl_disp_set_rxfh_indir,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_RXFH_INDIR,			\
			 nbl_disp_chan_set_rxfh_indir_req, nbl_disp_chan_set_rxfh_indir_resp);	\
	NBL_DISP_SET_OPS(get_rxfh_rss_key, nbl_disp_get_rxfh_rss_key,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_RXFH_RSS_KEY,			\
			 nbl_disp_chan_get_rxfh_rss_key_req,					\
			 nbl_disp_chan_get_rxfh_rss_key_resp);					\
	NBL_DISP_SET_OPS(get_rxfh_rss_alg_sel, nbl_disp_get_rxfh_rss_alg_sel,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_RXFH_RSS_ALG_SEL,		\
			 nbl_disp_chan_get_rxfh_rss_alg_sel_req,				\
			 nbl_disp_chan_get_rxfh_rss_alg_sel_resp);				\
	NBL_DISP_SET_OPS(set_rxfh_rss_alg_sel, nbl_disp_set_rxfh_rss_alg_sel,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_RXFH_RSS_ALG_SEL,		\
			 nbl_disp_chan_set_rxfh_rss_alg_sel_req,				\
			 nbl_disp_chan_set_rxfh_rss_alg_sel_resp);				\
	NBL_DISP_SET_OPS(cfg_txrx_vlan, nbl_disp_cfg_txrx_vlan,					\
			 NBL_DISP_CTRL_LVL_NET,	-1, NULL, NULL);				\
	NBL_DISP_SET_OPS(setup_rdma_id, nbl_disp_setup_rdma_id,					\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(remove_rdma_id, nbl_disp_remove_rdma_id,				\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(register_rdma, nbl_disp_register_rdma,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REGISTER_RDMA,			\
			 nbl_disp_chan_register_rdma_req, nbl_disp_chan_register_rdma_resp);	\
	NBL_DISP_SET_OPS(unregister_rdma, nbl_disp_unregister_rdma,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_UNREGISTER_RDMA,			\
			 nbl_disp_chan_unregister_rdma_req, nbl_disp_chan_unregister_rdma_resp);\
	NBL_DISP_SET_OPS(register_rdma_bond, nbl_disp_register_rdma_bond,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REGISTER_RDMA_BOND,		\
			 nbl_disp_chan_register_rdma_bond_req,					\
			 nbl_disp_chan_register_rdma_bond_resp);				\
	NBL_DISP_SET_OPS(unregister_rdma_bond, nbl_disp_unregister_rdma_bond,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_UNREGISTER_RDMA_BOND,		\
			 nbl_disp_chan_unregister_rdma_bond_req,				\
			 nbl_disp_chan_unregister_rdma_bond_resp);				\
	NBL_DISP_SET_OPS(get_hw_addr, nbl_disp_get_hw_addr,					\
			 NBL_DISP_CTRL_LVL_ALWAYS, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_real_hw_addr, nbl_disp_get_real_hw_addr,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_REAL_HW_ADDR,			\
			 nbl_disp_chan_get_real_hw_addr_req,					\
			 nbl_disp_chan_get_real_hw_addr_resp);					\
	NBL_DISP_SET_OPS(get_function_id, nbl_disp_get_function_id,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_FUNCTION_ID,			\
			 nbl_disp_chan_get_function_id_req, nbl_disp_chan_get_function_id_resp);\
	NBL_DISP_SET_OPS(get_real_bdf, nbl_disp_get_real_bdf,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_REAL_BDF,			\
			 nbl_disp_chan_get_real_bdf_req, nbl_disp_chan_get_real_bdf_resp);	\
	NBL_DISP_SET_OPS(check_fw_heartbeat, nbl_disp_check_fw_heartbeat,			\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(check_fw_reset, nbl_disp_check_fw_reset,				\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(flash_lock, nbl_disp_flash_lock,					\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(flash_unlock, nbl_disp_flash_unlock,					\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(flash_prepare, nbl_disp_flash_prepare,					\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(flash_image, nbl_disp_flash_image,					\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(flash_activate, nbl_disp_flash_activate,				\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_phy_caps, nbl_disp_get_phy_caps,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_PHY_CAPS,			\
			 nbl_disp_chan_get_phy_caps_req,					\
			 nbl_disp_chan_get_phy_caps_resp);					\
	NBL_DISP_SET_OPS(set_sfp_state, nbl_disp_set_sfp_state,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_SFP_STATE,			\
			 nbl_disp_chan_set_sfp_state_req,					\
			 nbl_disp_chan_set_sfp_state_resp);					\
	NBL_DISP_SET_OPS(passthrough_fw_cmd, nbl_disp_passthrough_fw_cmd,			\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(check_active_vf, nbl_disp_check_active_vf,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CHECK_ACTIVE_VF,			\
			 nbl_disp_chan_check_active_vf_req,					\
			 nbl_disp_chan_check_active_vf_resp);					\
	NBL_DISP_SET_OPS(get_adminq_tx_buf_size, nbl_disp_get_adminq_tx_buf_size,		\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(emp_console_write, nbl_disp_adminq_emp_console_write,			\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_product_flex_cap, nbl_disp_get_product_flex_cap,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_PRODUCT_FLEX_CAP,		\
			 nbl_disp_chan_get_product_flex_cap_req,				\
			 nbl_disp_chan_get_product_flex_cap_resp);				\
	NBL_DISP_SET_OPS(get_product_fix_cap, nbl_disp_get_product_fix_cap,			\
			 NBL_DISP_CTRL_LVL_ALWAYS, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(alloc_ktls_tx_index, nbl_disp_alloc_ktls_tx_index,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ALLOC_KTLS_TX_INDEX,		\
			 nbl_disp_chan_alloc_ktls_tx_index_req,					\
			 nbl_disp_chan_alloc_ktls_tx_index_resp);				\
	NBL_DISP_SET_OPS(free_ktls_tx_index, nbl_disp_free_ktls_tx_index,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_FREE_KTLS_TX_INDEX,		\
			 nbl_disp_chan_free_ktls_tx_index_req,					\
			 nbl_disp_chan_free_ktls_tx_index_resp);				\
	NBL_DISP_SET_OPS(cfg_ktls_tx_keymat, nbl_disp_cfg_ktls_tx_keymat,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_KTLS_TX_KEYMAT,		\
			 nbl_disp_chan_cfg_ktls_tx_keymat_req,					\
			 nbl_disp_chan_cfg_ktls_tx_keymat_resp);				\
	NBL_DISP_SET_OPS(alloc_ktls_rx_index, nbl_disp_alloc_ktls_rx_index,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ALLOC_KTLS_RX_INDEX,		\
			 nbl_disp_chan_alloc_ktls_rx_index_req,					\
			 nbl_disp_chan_alloc_ktls_rx_index_resp);				\
	NBL_DISP_SET_OPS(free_ktls_rx_index, nbl_disp_free_ktls_rx_index,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_FREE_KTLS_RX_INDEX,		\
			 nbl_disp_chan_free_ktls_rx_index_req,					\
			 nbl_disp_chan_free_ktls_rx_index_resp);				\
	NBL_DISP_SET_OPS(cfg_ktls_rx_keymat, nbl_disp_cfg_ktls_rx_keymat,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_KTLS_RX_KEYMAT,		\
			 nbl_disp_chan_cfg_ktls_rx_keymat_req,					\
			 nbl_disp_chan_cfg_ktls_rx_keymat_resp);				\
	NBL_DISP_SET_OPS(cfg_ktls_rx_record, nbl_disp_cfg_ktls_rx_record,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_KTLS_RX_RECORD,		\
			 nbl_disp_chan_cfg_ktls_rx_record_req,					\
			 nbl_disp_chan_cfg_ktls_rx_record_resp);				\
	NBL_DISP_SET_OPS(add_ktls_rx_flow, nbl_disp_add_ktls_rx_flow,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ADD_KTLS_RX_FLOW,			\
			 nbl_disp_chan_add_ktls_rx_flow_req,					\
			 nbl_disp_chan_add_ktls_rx_flow_resp);					\
	NBL_DISP_SET_OPS(del_ktls_rx_flow, nbl_disp_del_ktls_rx_flow,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_DEL_KTLS_RX_FLOW,			\
			 nbl_disp_chan_del_ktls_rx_flow_req,					\
			 nbl_disp_chan_del_ktls_rx_flow_resp);					\
	NBL_DISP_SET_OPS(alloc_ipsec_tx_index, nbl_disp_alloc_ipsec_tx_index,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ALLOC_IPSEC_TX_INDEX,		\
			 nbl_disp_chan_alloc_ipsec_tx_index_req,				\
			 nbl_disp_chan_alloc_ipsec_tx_index_resp);				\
	NBL_DISP_SET_OPS(free_ipsec_tx_index, nbl_disp_free_ipsec_tx_index,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_FREE_IPSEC_TX_INDEX,		\
			 nbl_disp_chan_free_ipsec_tx_index_req,					\
			 nbl_disp_chan_free_ipsec_tx_index_resp);				\
	NBL_DISP_SET_OPS(alloc_ipsec_rx_index, nbl_disp_alloc_ipsec_rx_index,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ALLOC_IPSEC_RX_INDEX,		\
			 nbl_disp_chan_alloc_ipsec_rx_index_req,				\
			 nbl_disp_chan_alloc_ipsec_rx_index_resp);				\
	NBL_DISP_SET_OPS(free_ipsec_rx_index, nbl_disp_free_ipsec_rx_index,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_FREE_IPSEC_RX_INDEX,		\
			 nbl_disp_chan_free_ipsec_rx_index_req,					\
			 nbl_disp_chan_free_ipsec_rx_index_resp);				\
	NBL_DISP_SET_OPS(cfg_ipsec_tx_sad, nbl_disp_cfg_ipsec_tx_sad,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_IPSEC_TX_SAD,			\
			 nbl_disp_chan_cfg_ipsec_tx_sad_req,					\
			 nbl_disp_chan_cfg_ipsec_tx_sad_resp);					\
	NBL_DISP_SET_OPS(cfg_ipsec_rx_sad, nbl_disp_cfg_ipsec_rx_sad,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_IPSEC_RX_SAD,			\
			 nbl_disp_chan_cfg_ipsec_rx_sad_req,					\
			 nbl_disp_chan_cfg_ipsec_rx_sad_resp);					\
	NBL_DISP_SET_OPS(add_ipsec_tx_flow, nbl_disp_add_ipsec_tx_flow,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ADD_IPSEC_TX_FLOW,			\
			 nbl_disp_chan_add_ipsec_tx_flow_req,					\
			 nbl_disp_chan_add_ipsec_tx_flow_resp);					\
	NBL_DISP_SET_OPS(del_ipsec_tx_flow, nbl_disp_del_ipsec_tx_flow,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_DEL_IPSEC_TX_FLOW,			\
			 nbl_disp_chan_del_ipsec_tx_flow_req,					\
			 nbl_disp_chan_del_ipsec_tx_flow_resp);					\
	NBL_DISP_SET_OPS(add_ipsec_rx_flow, nbl_disp_add_ipsec_rx_flow,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ADD_IPSEC_RX_FLOW,			\
			 nbl_disp_chan_add_ipsec_rx_flow_req,					\
			 nbl_disp_chan_add_ipsec_rx_flow_resp);					\
	NBL_DISP_SET_OPS(del_ipsec_rx_flow, nbl_disp_del_ipsec_rx_flow,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_DEL_IPSEC_RX_FLOW,			\
			 nbl_disp_chan_del_ipsec_rx_flow_req,					\
			 nbl_disp_chan_del_ipsec_rx_flow_resp);					\
	NBL_DISP_SET_OPS(check_ipsec_status, nbl_disp_check_ipsec_status,			\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(get_dipsec_lft_info, nbl_disp_get_dipsec_lft_info,			\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(handle_dipsec_soft_expire, nbl_disp_handle_dipsec_soft_expire,		\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(handle_dipsec_hard_expire, nbl_disp_handle_dipsec_hard_expire,		\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(get_uipsec_lft_info, nbl_disp_get_uipsec_lft_info,			\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(handle_uipsec_soft_expire, nbl_disp_handle_uipsec_soft_expire,		\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(handle_uipsec_hard_expire, nbl_disp_handle_uipsec_hard_expire,		\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(get_mbx_irq_num, nbl_disp_get_mbx_irq_num,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_MBX_IRQ_NUM,			\
			 nbl_disp_chan_get_mbx_irq_num_req,					\
			 nbl_disp_chan_get_mbx_irq_num_resp);					\
	NBL_DISP_SET_OPS(get_adminq_irq_num, nbl_disp_get_adminq_irq_num,			\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_abnormal_irq_num, nbl_disp_get_abnormal_irq_num,			\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(clear_accel_flow, nbl_disp_clear_accel_flow,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CLEAR_ACCEL_FLOW,			\
			 nbl_disp_chan_clear_accel_flow_req,					\
			 nbl_disp_chan_clear_accel_flow_resp);					\
	NBL_DISP_SET_OPS(clear_flow, nbl_disp_clear_flow,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CLEAR_FLOW,			\
			 nbl_disp_chan_clear_flow_req, nbl_disp_chan_clear_flow_resp);		\
	NBL_DISP_SET_OPS(clear_queues, nbl_disp_clear_queues,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CLEAR_QUEUE,			\
			 nbl_disp_chan_clear_queues_req, nbl_disp_chan_clear_queues_resp);	\
	NBL_DISP_SET_OPS(disable_phy_flow, nbl_disp_disable_phy_flow,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_DISABLE_PHY_FLOW,			\
			 nbl_disp_chan_disable_phy_flow_req,					\
			 nbl_disp_chan_disable_phy_flow_resp);					\
	NBL_DISP_SET_OPS(enable_phy_flow, nbl_disp_enable_phy_flow,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ENABLE_PHY_FLOW,			\
			 nbl_disp_chan_enable_phy_flow_req,					\
			 nbl_disp_chan_enable_phy_flow_resp);					\
	NBL_DISP_SET_OPS(init_acl, nbl_disp_init_acl,						\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_INIT_ACL,				\
			 nbl_disp_chan_init_acl_req,						\
			 nbl_disp_chan_init_acl_resp);						\
	NBL_DISP_SET_OPS(uninit_acl, nbl_disp_uninit_acl,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_UNINIT_ACL,			\
			 nbl_disp_chan_uninit_acl_req,						\
			 nbl_disp_chan_uninit_acl_resp);					\
	NBL_DISP_SET_OPS(set_upcall_rule, nbl_disp_set_upcall_rule,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_UPCALL_RULE,			\
			 nbl_disp_chan_set_upcall_rule_req,					\
			 nbl_disp_chan_set_upcall_rule_resp);					\
	NBL_DISP_SET_OPS(unset_upcall_rule, nbl_disp_unset_upcall_rule,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_UNSET_UPCALL_RULE,			\
			 nbl_disp_chan_unset_upcall_rule_req,					\
			 nbl_disp_chan_unset_upcall_rule_resp);					\
	NBL_DISP_SET_OPS(set_shaping_dport_vld, nbl_disp_set_shaping_dport_vld,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_SHAPING_DPORT_VLD,		\
			 nbl_disp_chan_set_shaping_dport_vld_req,				\
			 nbl_disp_chan_set_shaping_dport_vld_resp);				\
	NBL_DISP_SET_OPS(set_dport_fc_th_vld, nbl_disp_set_dport_fc_th_vld,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_DPORT_FC_TH_VLD,		\
			 nbl_disp_chan_set_dport_fc_th_vld_req,					\
			 nbl_disp_chan_set_dport_fc_th_vld_resp);				\
	NBL_DISP_SET_OPS(check_offload_status, nbl_disp_check_offload_status,			\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_reg_dump, nbl_disp_get_reg_dump,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_REG_DUMP,			\
			 nbl_disp_chan_get_reg_dump_req,					\
			 nbl_disp_chan_get_reg_dump_resp);					\
	NBL_DISP_SET_OPS(get_reg_dump_len, nbl_disp_get_reg_dump_len,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_REG_DUMP_LEN,			\
			 nbl_disp_chan_get_reg_dump_len_req,					\
			 nbl_disp_chan_get_reg_dump_len_resp);					\
	NBL_DISP_SET_OPS(get_p4_info, nbl_disp_get_p4_info,					\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(load_p4, nbl_disp_load_p4,						\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(load_p4_default, nbl_disp_load_p4_default,				\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_p4_used, nbl_disp_get_p4_used,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_P4_USED,			\
			 nbl_disp_chan_get_p4_used_req,	nbl_disp_chan_get_p4_used_resp);	\
	NBL_DISP_SET_OPS(set_p4_used, nbl_disp_set_p4_used,					\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_p4_version, nbl_disp_get_p4_version,				\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_board_id, nbl_disp_get_board_id,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_BOARD_ID,			\
			 nbl_disp_chan_get_board_id_req, nbl_disp_chan_get_board_id_resp);	\
	NBL_DISP_SET_OPS(restore_abnormal_ring, nbl_disp_restore_abnormal_ring,			\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(restart_abnormal_ring, nbl_disp_restart_abnormal_ring,			\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(restore_hw_queue, nbl_disp_restore_hw_queue,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_RESTORE_HW_QUEUE,			\
			 nbl_disp_chan_restore_hw_queue_req,					\
			 nbl_disp_chan_restore_hw_queue_resp);					\
	NBL_DISP_SET_OPS(stop_abnormal_hw_queue, nbl_disp_stop_abnormal_hw_queue,		\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_STOP_ABNORMAL_HW_QUEUE,		\
			 nbl_disp_chan_stop_abnormal_hw_queue_req,				\
			 nbl_disp_chan_stop_abnormal_hw_queue_resp);				\
	NBL_DISP_SET_OPS(stop_abnormal_sw_queue, nbl_disp_stop_abnormal_sw_queue,		\
			 NBL_DISP_CTRL_LVL_NET, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_local_queue_id, nbl_disp_get_local_queue_id,			\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_REGISTER_NET_REP, NULL,					\
			 nbl_disp_chan_register_net_rep_resp);					\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_UNREGISTER_NET_REP, NULL,					\
			 nbl_disp_chan_unregister_net_rep_resp);				\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_REGISTER_ETH_REP, NULL,					\
			 nbl_disp_chan_register_eth_rep_resp);					\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_UNREGISTER_ETH_REP, NULL,					\
			 nbl_disp_chan_unregister_eth_rep_resp);				\
	NBL_DISP_SET_OPS(get_vsi_global_queue_id, nbl_disp_get_vsi_global_qid,			\
			 NBL_DISP_CTRL_LVL_MGT,							\
			 NBL_CHAN_MSG_GET_VSI_GLOBAL_QUEUE_ID,					\
			 nbl_disp_chan_get_vsi_global_qid_req,					\
			 nbl_disp_chan_get_vsi_global_qid_resp);				\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_GET_LINE_RATE_INFO,					\
			 NULL, nbl_disp_chan_get_line_rate_info_resp);				\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_REGISTER_UPCALL_PORT, NULL,				\
			 nbl_disp_chan_register_upcall_port_resp);				\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_UNREGISTER_UPCALL_PORT, NULL,				\
			 nbl_disp_chan_unregister_upcall_port_resp);				\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_SET_OFFLOAD_STATUS, NULL,					\
			 nbl_disp_chan_set_offload_status_resp);				\
	NBL_DISP_SET_OPS(get_port_attributes, nbl_disp_get_port_attributes,			\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(update_ring_num, nbl_disp_update_ring_num,				\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(update_rdma_cap, nbl_disp_update_rdma_cap,				\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_rdma_cap_num, nbl_disp_get_rdma_cap_num,				\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(update_rdma_mem_type, nbl_disp_update_rdma_mem_type,			\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(set_ring_num, nbl_disp_set_ring_num,					\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(enable_port, nbl_disp_enable_port,					\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(init_port, nbl_disp_init_port,						\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(dummy_func, NULL,							\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_ADMINQ_PORT_NOTIFY,		\
			 NULL,									\
			 nbl_disp_chan_recv_port_notify_resp);					\
	NBL_DISP_SET_OPS(get_port_state, nbl_disp_get_port_state,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_PORT_STATE,			\
			 nbl_disp_chan_get_port_state_req,					\
			 nbl_disp_chan_get_port_state_resp);					\
	NBL_DISP_SET_OPS(get_fec_stats, nbl_disp_get_fec_stats,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_FEC_STATS,			\
			 nbl_disp_chan_get_fec_stats_req,					\
			 nbl_disp_chan_get_fec_stats_resp);					\
	NBL_DISP_SET_OPS(set_port_advertising, nbl_disp_set_port_advertising,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_PORT_ADVERTISING,		\
			 nbl_disp_chan_set_port_advertising_req,				\
			 nbl_disp_chan_set_port_advertising_resp);				\
	NBL_DISP_SET_OPS(get_module_info, nbl_disp_get_module_info,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_MODULE_INFO,			\
			 nbl_disp_chan_get_module_info_req,					\
			 nbl_disp_chan_get_module_info_resp);					\
	NBL_DISP_SET_OPS(get_module_eeprom, nbl_disp_get_module_eeprom,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_MODULE_EEPROM,			\
			 nbl_disp_chan_get_module_eeprom_req,					\
			 nbl_disp_chan_get_module_eeprom_resp);					\
	NBL_DISP_SET_OPS(get_link_state, nbl_disp_get_link_state,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_LINK_STATE,			\
			 nbl_disp_chan_get_link_state_req,					\
			 nbl_disp_chan_get_link_state_resp);					\
	NBL_DISP_SET_OPS(get_link_down_count, nbl_disp_get_link_down_count,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_LINK_DOWN_COUNT,		\
			 nbl_disp_chan_get_link_down_count_req,					\
			 nbl_disp_chan_get_link_down_count_resp);				\
	NBL_DISP_SET_OPS(get_link_status_opcode, nbl_disp_get_link_status_opcode,		\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_LINK_STATUS_OPCODE,		\
			 nbl_disp_chan_get_link_status_opcode_req,				\
			 nbl_disp_chan_get_link_status_opcode_resp);				\
	NBL_DISP_SET_OPS(set_wol, nbl_disp_set_wol,						\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_WOL,				\
			 nbl_disp_chan_set_wol_req, nbl_disp_chan_set_wol_resp);		\
	NBL_DISP_SET_OPS(cfg_eth_bond_event, nbl_disp_cfg_eth_bond_event,			\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_INIT_OFLD, NULL,						\
			 nbl_disp_chan_init_offload_fwd_resp);					\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_INIT_CMDQ, NULL,						\
			 nbl_disp_chan_init_cmdq_resp);						\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_DESTROY_CMDQ, NULL,					\
			 nbl_disp_chan_destroy_cmdq_resp);					\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_RESET_CMDQ, NULL,						\
			 nbl_disp_chan_reset_cmdq_resp);					\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_OFFLOAD_FLOW_RULE, NULL,					\
			 nbl_disp_chan_offload_flow_rule_resp);					\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_GET_ACL_SWITCH, NULL,					\
			 nbl_disp_chan_get_flow_acl_switch_resp);				\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_INIT_REP, NULL,						\
			 nbl_disp_chan_init_rep_resp);						\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_INIT_FLOW, NULL,						\
			 nbl_disp_chan_init_flow_resp);						\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_DEINIT_FLOW, NULL,					\
			 nbl_disp_chan_deinit_flow_resp);					\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_GET_QUEUE_CXT, NULL,					\
			 nbl_disp_chan_get_queue_cxt_resp);					\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_CFG_LOG, NULL,						\
			 nbl_disp_chan_cfg_log_resp);						\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_INIT_VDPAQ, NULL,						\
			 nbl_disp_chan_init_vdpaq_resp);					\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_MSG_DESTROY_VDPAQ, NULL,					\
			 nbl_disp_chan_destroy_vdpaq_resp);					\
	NBL_DISP_SET_OPS(dummy_func, NULL, NBL_DISP_CTRL_LVL_MGT,				\
			 NBL_CHAN_GET_UPCALL_PORT, NULL,					\
			 nbl_disp_chan_get_upcall_port_resp);					\
	NBL_DISP_SET_OPS(configure_rdma_msix_off, nbl_disp_configure_rdma_msix_off,		\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(set_eth_mac_addr, nbl_disp_set_eth_mac_addr,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_ETH_MAC_ADDR,			\
			 nbl_disp_chan_set_eth_mac_addr_req,					\
			 nbl_disp_chan_set_eth_mac_addr_resp);					\
	NBL_DISP_SET_OPS(get_chip_temperature, nbl_disp_get_chip_temperature,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_CHIP_TEMPERATURE,		\
			 nbl_disp_chan_get_chip_temperature_req,				\
			 nbl_disp_chan_get_chip_temperature_resp);				\
	NBL_DISP_SET_OPS(get_module_temperature, nbl_disp_get_module_temperature,		\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_MODULE_TEMPERATURE,		\
			 nbl_disp_chan_get_module_temperature_req,				\
			 nbl_disp_chan_get_module_temperature_resp);				\
	NBL_DISP_SET_OPS(process_abnormal_event, nbl_disp_process_abnormal_event,		\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(switchdev_init_cmdq, nbl_disp_switchdev_init_cmdq,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SWITCHDEV_INIT_CMDQ,		\
			 nbl_disp_chan_switchdev_init_cmdq_req,					\
			 nbl_disp_chan_switchdev_init_cmdq_resp);				\
	NBL_DISP_SET_OPS(switchdev_deinit_cmdq, nbl_disp_switchdev_deinit_cmdq,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SWITCHDEV_DEINIT_CMDQ,		\
			 nbl_disp_chan_switchdev_deinit_cmdq_req,				\
			 nbl_disp_chan_switchdev_deinit_cmdq_resp);				\
	NBL_DISP_SET_OPS(add_tc_flow, nbl_disp_add_tc_flow,					\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(del_tc_flow, nbl_disp_del_tc_flow,					\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(tc_tun_encap_lookup, nbl_disp_tc_tun_encap_lookup,			\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(tc_tun_encap_del, nbl_disp_tc_tun_encap_del,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(tc_tun_encap_add, nbl_disp_tc_tun_encap_add,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(flow_index_lookup, nbl_disp_flow_index_lookup,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(set_tc_flow_info, nbl_disp_set_tc_flow_info,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_TC_FLOW_INFO,			\
			 nbl_disp_chan_set_tc_flow_info_req,					\
			 nbl_disp_chan_set_tc_flow_info_resp);					\
	NBL_DISP_SET_OPS(unset_tc_flow_info, nbl_disp_unset_tc_flow_info,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_UNSET_TC_FLOW_INFO,		\
			 nbl_disp_chan_unset_tc_flow_info_req,					\
			 nbl_disp_chan_unset_tc_flow_info_resp);				\
	NBL_DISP_SET_OPS(get_tc_flow_info, nbl_disp_get_tc_flow_info,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(query_tc_stats, nbl_disp_query_tc_stats,				\
			 NBL_DISP_CTRL_LVL_NET, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(adapt_desc_gother, nbl_disp_adapt_desc_gother,				\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(set_desc_high_throughput, nbl_disp_set_desc_high_throughput,		\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(flr_clear_net, nbl_disp_flr_clear_net,					\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(flr_clear_accel, nbl_disp_flr_clear_accel,				\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(flr_clear_queues, nbl_disp_flr_clear_queues,				\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(flr_clear_accel_flow, nbl_disp_flr_clear_accel_flow,			\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(flr_clear_flows, nbl_disp_flr_clear_flows,				\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(flr_clear_interrupt, nbl_disp_flr_clear_interrupt,			\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(flr_clear_rdma, nbl_disp_flr_clear_rdma,				\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(covert_vfid_to_vsi_id, nbl_disp_covert_vfid_to_vsi_id,			\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(unmask_all_interrupts, nbl_disp_unmask_all_interrupts,			\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(keep_alive, nbl_disp_keep_alive_req,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_KEEP_ALIVE,			\
			 nbl_disp_keep_alive_req,						\
			 nbl_disp_chan_keep_alive_resp);					\
	NBL_DISP_SET_OPS(ctrl_port_led, nbl_disp_ctrl_port_led,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CTRL_PORT_LED,			\
			 nbl_disp_chan_ctrl_port_led_req, nbl_disp_chan_ctrl_port_led_resp);	\
	NBL_DISP_SET_OPS(nway_reset, nbl_disp_nway_reset,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_NWAY_RESET,			\
			 nbl_disp_chan_nway_reset_req, nbl_disp_chan_nway_reset_resp);		\
	NBL_DISP_SET_OPS(get_rep_queue_info, nbl_disp_get_rep_queue_info,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_REP_QUEUE_INFO,		\
			 nbl_disp_chan_get_rep_queue_info_req,					\
			 nbl_disp_chan_get_rep_queue_info_resp);				\
	NBL_DISP_SET_OPS(get_user_queue_info, nbl_disp_get_user_queue_info,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_USER_QUEUE_INFO,		\
			 nbl_disp_chan_get_user_queue_info_req,					\
			 nbl_disp_chan_get_user_queue_info_resp);				\
	NBL_DISP_SET_OPS(get_board_info, nbl_disp_get_board_info, NBL_DISP_CTRL_LVL_MGT,	\
			 NBL_CHAN_MSG_GET_BOARD_INFO, nbl_disp_chan_get_board_info_req,		\
			 nbl_disp_chan_get_board_info_resp);					\
	NBL_DISP_SET_OPS(get_vf_base_vsi_id, nbl_disp_get_vf_base_vsi_id,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_VF_BASE_VSI_ID,		\
			 nbl_disp_chan_get_vf_base_vsi_id_req,					\
			 nbl_disp_chan_get_vf_base_vsi_id_resp);				\
	NBL_DISP_SET_OPS(cfg_eth_bond_info, nbl_disp_cfg_eth_bond_info,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_ETH_BOND_INFO,			\
			 nbl_disp_chan_cfg_eth_bond_info_req,					\
			 nbl_disp_chan_cfg_eth_bond_info_resp);					\
	NBL_DISP_SET_OPS(get_eth_bond_info, nbl_disp_get_eth_bond_info,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_ETH_BOND_INFO,			\
			 nbl_disp_chan_get_eth_bond_info_req,					\
			 nbl_disp_chan_get_eth_bond_info_resp);					\
	NBL_DISP_SET_OPS(add_nd_upcall_flow, nbl_disp_chan_add_nd_upcall_flow,			\
			 NBL_DISP_CTRL_LVL_MGT,							\
			 NBL_CHAN_MSG_ADD_ND_UPCALL_FLOW,					\
			 nbl_disp_chan_add_nd_upcall_flow_req,					\
			 nbl_disp_chan_add_nd_upcall_flow_resp);				\
	NBL_DISP_SET_OPS(del_nd_upcall_flow, nbl_disp_chan_del_nd_upcall_flow,			\
			 NBL_DISP_CTRL_LVL_MGT,							\
			 NBL_CHAN_MSG_DEL_ND_UPCALL_FLOW,					\
			 nbl_disp_chan_del_nd_upcall_flow_req,					\
			 nbl_disp_chan_del_nd_upcall_flow_resp);				\
	NBL_DISP_SET_OPS(set_bridge_mode, nbl_disp_set_bridge_mode,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_BRIDGE_MODE,			\
			 nbl_disp_chan_set_bridge_mode_req,					\
			 nbl_disp_chan_set_bridge_mode_resp);					\
	NBL_DISP_SET_OPS(get_vf_function_id, nbl_disp_get_vf_function_id,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_VF_FUNCTION_ID,		\
			 nbl_disp_chan_get_vf_function_id_req,					\
			 nbl_disp_chan_get_vf_function_id_resp);				\
	NBL_DISP_SET_OPS(get_vf_vsi_id, nbl_disp_get_vf_vsi_id,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_VF_VSI_ID,			\
			 nbl_disp_chan_get_vf_vsi_id_req,					\
			 nbl_disp_chan_get_vf_vsi_id_resp);					\
	NBL_DISP_SET_OPS(check_vf_is_active, nbl_disp_check_vf_is_active,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_CHECK_VF_IS_ACTIVE,			\
			 nbl_disp_chan_check_vf_is_active_req,					\
			 nbl_disp_chan_check_vf_is_active_resp);				\
	NBL_DISP_SET_OPS(check_vf_is_vdpa, nbl_disp_check_vf_is_vdpa,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_CHECK_VF_IS_VDPA,			\
			 nbl_disp_chan_check_vf_is_vdpa_req,					\
			 nbl_disp_chan_check_vf_is_vdpa_resp);					\
	NBL_DISP_SET_OPS(get_vdpa_vf_stats, nbl_disp_get_vdpa_vf_stats,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_VDPA_VF_STATS,			\
			 nbl_disp_chan_get_vdpa_vf_stats_req,					\
			 nbl_disp_chan_get_vdpa_vf_stats_resp);					\
	NBL_DISP_SET_OPS(get_uvn_pkt_drop_stats, nbl_disp_get_uvn_pkt_drop_stats,		\
			 NBL_DISP_CTRL_LVL_MGT,							\
			 NBL_CHAN_GET_UVN_PKT_DROP_STATS,					\
			 nbl_disp_chan_get_uvn_pkt_drop_stats_req,				\
			 nbl_disp_chan_get_uvn_pkt_drop_stats_resp);				\
	NBL_DISP_SET_OPS(get_ustore_pkt_drop_stats, nbl_disp_get_ustore_pkt_drop_stats,		\
			 NBL_DISP_CTRL_LVL_MGT,							\
			 NBL_CHAN_GET_USTORE_PKT_DROP_STATS,					\
			 nbl_disp_chan_get_ustore_pkt_drop_stats_req,				\
			 nbl_disp_chan_get_ustore_pkt_drop_stats_resp);				\
	NBL_DISP_SET_OPS(get_ustore_total_pkt_drop_stats,					\
			 nbl_disp_get_ustore_total_pkt_drop_stats,				\
			 NBL_DISP_CTRL_LVL_MGT,							\
			 NBL_CHAN_GET_USTORE_TOTAL_PKT_DROP_STATS,				\
			 nbl_disp_chan_get_ustore_total_pkt_drop_stats_req,			\
			 nbl_disp_chan_get_ustore_total_pkt_drop_stats_resp);			\
	NBL_DISP_SET_OPS(set_pmd_debug, nbl_disp_set_pmd_debug,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_PMD_DEBUG,			\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(register_func_mac, nbl_disp_register_func_mac,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REGISTER_FUNC_MAC,			\
			 nbl_disp_chan_register_func_mac_req,					\
			 nbl_disp_chan_register_func_mac_resp);					\
	NBL_DISP_SET_OPS(set_tx_rate, nbl_disp_set_tx_rate,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_TX_RATE,			\
			 nbl_disp_chan_set_tx_rate_req, nbl_disp_chan_set_tx_rate_resp);	\
	NBL_DISP_SET_OPS(set_rx_rate, nbl_disp_set_rx_rate,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_RX_RATE,			\
			 nbl_disp_chan_set_rx_rate_req, nbl_disp_chan_set_rx_rate_resp);	\
	NBL_DISP_SET_OPS(register_func_link_forced, nbl_disp_register_func_link_forced,		\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REGISTER_FUNC_LINK_FORCED,		\
			 nbl_disp_chan_register_func_link_forced_req,				\
			 nbl_disp_chan_register_func_link_forced_resp);				\
	NBL_DISP_SET_OPS(get_link_forced, nbl_disp_get_link_forced,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_LINK_FORCED,			\
			 nbl_disp_chan_get_link_forced_req, nbl_disp_chan_get_link_forced_resp);\
	NBL_DISP_SET_OPS(get_driver_version, nbl_disp_get_driver_version,			\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(register_func_trust, nbl_disp_register_func_trust,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REGISTER_FUNC_TRUST,		\
			 nbl_disp_chan_register_func_trust_req,					\
			 nbl_disp_chan_register_func_trust_resp);				\
	NBL_DISP_SET_OPS(register_func_vlan, nbl_disp_register_func_vlan,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REGISTER_FUNC_VLAN,		\
			 nbl_disp_chan_register_func_vlan_req,					\
			 nbl_disp_chan_register_func_vlan_resp);				\
	NBL_DISP_SET_OPS(register_func_rate, nbl_disp_register_func_rate,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REGISTER_FUNC_RATE,		\
			 nbl_disp_chan_register_func_rate_req,					\
			 nbl_disp_chan_register_func_rate_resp);				\
	NBL_DISP_SET_OPS(set_mtu, nbl_disp_set_mtu,						\
			 NBL_DISP_CTRL_LVL_MGT,	NBL_CHAN_MSG_MTU_SET,				\
			 nbl_disp_chan_set_mtu_req,						\
			 nbl_disp_chan_set_mtu_resp);						\
	NBL_DISP_SET_OPS(get_max_mtu, nbl_disp_get_max_mtu,					\
			 NBL_DISP_CTRL_LVL_NET,	-1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_fd_flow, nbl_disp_get_fd_flow,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_FD_FLOW,			\
			 nbl_disp_chan_get_fd_flow_req, nbl_disp_chan_get_fd_flow_resp);	\
	NBL_DISP_SET_OPS(get_fd_flow_cnt, nbl_disp_get_fd_flow_cnt,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_FD_FLOW_CNT,			\
			 nbl_disp_chan_get_fd_flow_cnt_req, nbl_disp_chan_get_fd_flow_cnt_resp);\
	NBL_DISP_SET_OPS(get_fd_flow_all, nbl_disp_get_fd_flow_all,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_FD_FLOW_ALL,			\
			 nbl_disp_chan_get_fd_flow_all_req, nbl_disp_chan_get_fd_flow_all_resp);\
	NBL_DISP_SET_OPS(get_fd_flow_max, nbl_disp_get_fd_flow_max,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_FD_FLOW_MAX,			\
			 nbl_disp_chan_get_fd_flow_max_req, nbl_disp_chan_get_fd_flow_max_resp);\
	NBL_DISP_SET_OPS(replace_fd_flow, nbl_disp_replace_fd_flow,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REPLACE_FD_FLOW,			\
			 nbl_disp_chan_replace_fd_flow_req, nbl_disp_chan_replace_fd_flow_resp);\
	NBL_DISP_SET_OPS(remove_fd_flow, nbl_disp_remove_fd_flow,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REMOVE_FD_FLOW,			\
			 nbl_disp_chan_remove_fd_flow_req, nbl_disp_chan_remove_fd_flow_resp);	\
	NBL_DISP_SET_OPS(config_fd_flow_state, nbl_disp_config_fd_flow_state,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CFG_FD_FLOW_STATE,			\
			 nbl_disp_chan_config_fd_flow_state_req,				\
			 nbl_disp_chan_config_fd_flow_state_resp);				\
	NBL_DISP_SET_OPS(cfg_fd_update_event, nbl_disp_cfg_fd_update_event,			\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(dump_fd_flow, nbl_disp_dump_fd_flow,					\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_xdp_queue_info, nbl_disp_get_xdp_queue_info,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_XDP_QUEUE_INFO,		\
			 nbl_disp_chan_get_xdp_queue_info_req,					\
			 nbl_disp_chan_get_xdp_queue_info_resp);				\
	NBL_DISP_SET_OPS(set_hw_status, nbl_disp_set_hw_status,					\
			 NBL_DISP_CTRL_LVL_ALWAYS, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(get_active_func_bitmaps, nbl_disp_get_active_func_bitmaps,		\
			 NBL_DISP_CTRL_LVL_ALWAYS, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(configure_rdma_bw, nbl_disp_configure_rdma_bw,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CONFIGURE_RDMA_BW,			\
			 nbl_disp_chan_configure_rdma_bw_req,					\
			 nbl_disp_chan_configure_rdma_bw_resp);					\
	NBL_DISP_SET_OPS(configure_qos, nbl_disp_configure_qos,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CONFIGURE_QOS,			\
			 nbl_disp_chan_configure_qos_req,					\
			 nbl_disp_chan_configure_qos_resp);					\
	NBL_DISP_SET_OPS(set_tc_wgt, nbl_disp_set_tc_wgt,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_TC_WGT,			\
			 nbl_disp_chan_set_tc_wgt_req,						\
			 nbl_disp_chan_set_tc_wgt_resp);					\
	NBL_DISP_SET_OPS(get_pfc_buffer_size, nbl_disp_get_pfc_buffer_size,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_PFC_BUFFER_SIZE,		\
			 nbl_disp_chan_get_pfc_buffer_size_req,					\
			 nbl_disp_chan_get_pfc_buffer_size_resp);				\
	NBL_DISP_SET_OPS(set_pfc_buffer_size, nbl_disp_set_pfc_buffer_size,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_PFC_BUFFER_SIZE,		\
			 nbl_disp_chan_set_pfc_buffer_size_req,					\
			 nbl_disp_chan_set_pfc_buffer_size_resp);				\
	NBL_DISP_SET_OPS(set_rate_limit, nbl_disp_set_rate_limit,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_SET_RATE_LIMIT,			\
			 nbl_disp_chan_set_rate_limit_req,					\
			 nbl_disp_chan_set_rate_limit_resp);					\
	NBL_DISP_SET_OPS(get_perf_dump_length, nbl_disp_get_perf_dump_length,			\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(get_perf_dump_data, nbl_disp_get_perf_dump_data,			\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(register_dev_name, nbl_disp_register_dev_name,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_REGISTER_PF_NAME,			\
			 nbl_disp_chan_register_dev_name_req,					\
			 nbl_disp_chan_register_dev_name_resp);					\
	NBL_DISP_SET_OPS(get_dev_name, nbl_disp_get_dev_name,					\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_PF_NAME,			\
			 nbl_disp_chan_get_dev_name_req,					\
			 nbl_disp_chan_get_dev_name_resp);					\
	NBL_DISP_SET_OPS(get_mirror_table_id, nbl_disp_get_mirror_table_id,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_GET_MIRROR_TABLE_ID,		\
			 nbl_disp_chan_get_mirror_table_id_req,					\
			 nbl_disp_chan_get_mirror_table_id_resp);				\
	NBL_DISP_SET_OPS(configure_mirror, nbl_disp_configure_mirror,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CONFIGURE_MIRROR,			\
			 nbl_disp_chan_configure_mirror_req,					\
			 nbl_disp_chan_configure_mirror_resp);					\
	NBL_DISP_SET_OPS(configure_mirror_table, nbl_disp_configure_mirror_table,		\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CONFIGURE_MIRROR_TABLE,		\
			 nbl_disp_chan_configure_mirror_table_req,				\
			 nbl_disp_chan_configure_mirror_table_resp);				\
	NBL_DISP_SET_OPS(clear_mirror_cfg, nbl_disp_clear_mirror_cfg,				\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CLEAR_MIRROR_CFG,			\
			 nbl_disp_chan_clear_mirror_cfg_req,					\
			 nbl_disp_chan_clear_mirror_cfg_resp);					\
	NBL_DISP_SET_OPS(cfg_mirror_outputport_event, nbl_disp_cfg_mirror_outputport_event,	\
			 NBL_DISP_CTRL_LVL_MGT, -1, NULL, NULL);				\
	NBL_DISP_SET_OPS(check_flow_table_spec, nbl_disp_check_flow_table_spec,			\
			 NBL_DISP_CTRL_LVL_MGT, NBL_CHAN_MSG_CHECK_FLOWTABLE_SPEC,		\
			 nbl_disp_chan_check_flow_table_spec_req,				\
			 nbl_disp_chan_check_flow_table_spec_resp);				\
	NBL_DISP_SET_OPS(get_dvn_desc_req, nbl_disp_get_dvn_desc_req,				\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
	NBL_DISP_SET_OPS(set_dvn_desc_req, nbl_disp_set_dvn_desc_req,				\
			 NBL_DISP_CTRL_LVL_MGT, -1,						\
			 NULL, NULL);								\
} while (0)

/* Structure starts here, adding an op should not modify anything below */
static int nbl_disp_setup_msg(struct nbl_dispatch_mgt *disp_mgt)
{
	struct nbl_channel_ops *chan_ops = NBL_DISP_MGT_TO_CHAN_OPS(disp_mgt);
	int ret = 0;

	if (!chan_ops->check_queue_exist(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt),
					 NBL_CHAN_TYPE_MAILBOX))
		return 0;

	mutex_init(&disp_mgt->ops_mutex_lock);
	spin_lock_init(&disp_mgt->ops_spin_lock);
	disp_mgt->ops_lock_required = true;

#define NBL_DISP_SET_OPS(disp_op, res_func, ctrl_lvl, msg_type, msg_req, msg_resp)		\
do {												\
	typeof(msg_type) _msg_type = (msg_type);						\
	if (_msg_type >= 0)									\
		ret += chan_ops->register_msg(NBL_DISP_MGT_TO_CHAN_PRIV(disp_mgt),		\
					      _msg_type, msg_resp, disp_mgt);			\
} while (0)
	NBL_DISP_OPS_TBL;
#undef  NBL_DISP_SET_OPS

	return ret;
}

/* Ctrl lvl means that if a certain level is set, then all disp_ops that decleared this lvl
 * will go directly to res_ops, rather than send a channel msg, and vice versa.
 */
static int nbl_disp_setup_ctrl_lvl(struct nbl_dispatch_mgt *disp_mgt, u32 lvl)
{
	struct nbl_dispatch_ops *disp_ops;

	disp_ops = NBL_DISP_MGT_TO_DISP_OPS(disp_mgt);

	set_bit(lvl, disp_mgt->ctrl_lvl);

#define NBL_DISP_SET_OPS(disp_op, res_func, ctrl, msg_type, msg_req, msg_resp)			\
do {												\
	disp_ops->NBL_NAME(disp_op) = test_bit(ctrl, disp_mgt->ctrl_lvl) ? res_func : msg_req; ;\
} while (0)
	NBL_DISP_OPS_TBL;
#undef  NBL_DISP_SET_OPS

	return 0;
}

static int nbl_disp_setup_disp_mgt(struct nbl_common_info *common,
				   struct nbl_dispatch_mgt **disp_mgt)
{
	struct device *dev;

	dev = NBL_COMMON_TO_DEV(common);
	*disp_mgt = devm_kzalloc(dev, sizeof(struct nbl_dispatch_mgt), GFP_KERNEL);
	if (!*disp_mgt)
		return -ENOMEM;

	NBL_DISP_MGT_TO_COMMON(*disp_mgt) = common;
	return 0;
}

static void nbl_disp_remove_disp_mgt(struct nbl_common_info *common,
				     struct nbl_dispatch_mgt **disp_mgt)
{
	struct device *dev;

	dev = NBL_COMMON_TO_DEV(common);
	devm_kfree(dev, *disp_mgt);
	*disp_mgt = NULL;
}

static void nbl_disp_remove_ops(struct device *dev, struct nbl_dispatch_ops_tbl **disp_ops_tbl)
{
	devm_kfree(dev, NBL_DISP_OPS_TBL_TO_OPS(*disp_ops_tbl));
	devm_kfree(dev, *disp_ops_tbl);
	*disp_ops_tbl = NULL;
}

static int nbl_disp_setup_ops(struct device *dev, struct nbl_dispatch_ops_tbl **disp_ops_tbl,
			      struct nbl_dispatch_mgt *disp_mgt)
{
	struct nbl_dispatch_ops *disp_ops;

	*disp_ops_tbl = devm_kzalloc(dev, sizeof(struct nbl_dispatch_ops_tbl), GFP_KERNEL);
	if (!*disp_ops_tbl)
		return -ENOMEM;

	disp_ops = devm_kzalloc(dev, sizeof(struct nbl_dispatch_ops), GFP_KERNEL);
	if (!disp_ops)
		return -ENOMEM;

	NBL_DISP_OPS_TBL_TO_OPS(*disp_ops_tbl) = disp_ops;
	NBL_DISP_OPS_TBL_TO_PRIV(*disp_ops_tbl) = disp_mgt;

	return 0;
}

int nbl_disp_init(void *p, struct nbl_init_param *param)
{
	struct nbl_adapter *adapter = (struct nbl_adapter *)p;
	struct device *dev = NBL_ADAPTER_TO_DEV(adapter);
	struct nbl_common_info *common = NBL_ADAPTER_TO_COMMON(adapter);
	struct nbl_dispatch_mgt **disp_mgt =
		(struct nbl_dispatch_mgt **)&NBL_ADAPTER_TO_DISP_MGT(adapter);
	struct nbl_dispatch_ops_tbl **disp_ops_tbl = &NBL_ADAPTER_TO_DISP_OPS_TBL(adapter);
	struct nbl_resource_ops_tbl *res_ops_tbl = NBL_ADAPTER_TO_RES_OPS_TBL(adapter);
	struct nbl_channel_ops_tbl *chan_ops_tbl = NBL_ADAPTER_TO_CHAN_OPS_TBL(adapter);
	int ret = 0;

	ret = nbl_disp_setup_disp_mgt(common, disp_mgt);
	if (ret)
		goto setup_mgt_fail;

	ret = nbl_disp_setup_ops(dev, disp_ops_tbl, *disp_mgt);
	if (ret)
		goto setup_ops_fail;

	NBL_DISP_MGT_TO_RES_OPS_TBL(*disp_mgt) = res_ops_tbl;
	NBL_DISP_MGT_TO_CHAN_OPS_TBL(*disp_mgt) = chan_ops_tbl;
	NBL_DISP_MGT_TO_DISP_OPS_TBL(*disp_mgt) = *disp_ops_tbl;

	ret = nbl_disp_setup_msg(*disp_mgt);
	if (ret)
		goto setup_msg_fail;

	if (param->caps.has_ctrl || param->caps.has_factory_ctrl) {
		ret = nbl_disp_setup_ctrl_lvl(*disp_mgt, NBL_DISP_CTRL_LVL_MGT);
		if (ret)
			goto setup_msg_fail;
	}

	if (param->caps.has_net || param->caps.has_factory_ctrl) {
		ret = nbl_disp_setup_ctrl_lvl(*disp_mgt, NBL_DISP_CTRL_LVL_NET);
		if (ret)
			goto setup_msg_fail;
	}

	ret = nbl_disp_setup_ctrl_lvl(*disp_mgt, NBL_DISP_CTRL_LVL_ALWAYS);
	if (ret)
		goto setup_msg_fail;

	return 0;

setup_msg_fail:
	nbl_disp_remove_ops(dev, disp_ops_tbl);
setup_ops_fail:
	nbl_disp_remove_disp_mgt(common, disp_mgt);
setup_mgt_fail:
	return ret;
}

void nbl_disp_remove(void *p)
{
	struct nbl_adapter *adapter = (struct nbl_adapter *)p;
	struct device *dev;
	struct nbl_common_info *common;
	struct nbl_dispatch_mgt **disp_mgt;
	struct nbl_dispatch_ops_tbl **disp_ops_tbl;

	if (!adapter)
		return;

	dev = NBL_ADAPTER_TO_DEV(adapter);
	common = NBL_ADAPTER_TO_COMMON(adapter);
	disp_mgt = (struct nbl_dispatch_mgt **)&NBL_ADAPTER_TO_DISP_MGT(adapter);
	disp_ops_tbl = &NBL_ADAPTER_TO_DISP_OPS_TBL(adapter);

	nbl_disp_remove_ops(dev, disp_ops_tbl);

	nbl_disp_remove_disp_mgt(common, disp_mgt);
}
