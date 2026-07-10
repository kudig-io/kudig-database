// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#include "nbl_dev_rdma.h"

static int nbl_dev_create_rdma_aux_dev(struct nbl_dev_mgt *dev_mgt, u8 type,
				       struct nbl_core_dev_lag_info *lag_info);
static void nbl_dev_destroy_rdma_aux_dev(struct nbl_dev_rdma *rdma_dev,
					 struct auxiliary_device **adev);

static void nbl_dev_rdma_pending_and_flush_event_task(struct nbl_dev_rdma *rdma_dev)
{
	atomic_inc(&rdma_dev->adev_busy);
	nbl_common_flush_task(&rdma_dev->event_task);
}

static void nbl_dev_rdma_resume_event_task(struct nbl_dev_rdma *rdma_dev)
{
	atomic_dec(&rdma_dev->adev_busy);
}

static int nbl_dev_rdma_bond_active_num(struct nbl_core_dev_info *cdev_info)
{
	int i, count = 0;

	if (!cdev_info->is_lag)
		return 0;

	for (i = 0; i < NBL_RDMA_LAG_MAX_PORTS; i++)
		if (cdev_info->lag_info.lag_mem[i].active)
			count++;

	return count;
}

static void nbl_dev_rdma_cfg_bond(struct nbl_dev_mgt *dev_mgt, struct nbl_core_dev_info *cdev_info,
				  bool enable)
{
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	int other_eth_id = -1, i;

	/* TODO: if we need to support bond with more than two ports, need to modify here */
	for (i = 0; i < NBL_LAG_MAX_PORTS; i++)
		if (cdev_info->lag_info.lag_mem[i].eth_id != NBL_COMMON_TO_ETH_ID(common))
			other_eth_id = cdev_info->lag_info.lag_mem[i].eth_id;

	if (other_eth_id == -1) {
		nbl_warn(common, NBL_DEBUG_MAIN, "Fail to find bond other eth id, rdma cfg abort");
		return;
	}

	serv_ops->cfg_bond_shaping(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
				   NBL_COMMON_TO_ETH_ID(common), enable);
	serv_ops->cfg_bgid_back_pressure(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					 NBL_COMMON_TO_ETH_ID(common), other_eth_id, enable);

	rdma_dev->bond_shaping_configed = enable;
}

static int nbl_dev_chan_grc_process_req(void *priv, u8 *req_args, u8 req_len,
					void *resp, u16 resp_len)
{
	struct nbl_dev_mgt *dev_mgt = (struct nbl_dev_mgt *)priv;
	struct nbl_channel_ops *chan_ops = NBL_DEV_MGT_TO_CHAN_OPS(dev_mgt);
	struct nbl_chan_rdma_resp param = {0};
	struct nbl_chan_rdma_resp result = {0};
	struct nbl_chan_send_info chan_send = {0};
	struct nbl_common_info *common;
	int ret = 0;

	if (!chan_ops)
		return 0;

	memcpy(param.resp_data, req_args, req_len);
	param.data_len = req_len;

	common = NBL_DEV_MGT_TO_COMMON(dev_mgt);

	NBL_CHAN_SEND(chan_send, common->mgt_pf, NBL_CHAN_MSG_GRC_PROCESS, &param, sizeof(param),
		      &result, sizeof(result), 1);
	ret = chan_ops->send_msg(NBL_DEV_MGT_TO_CHAN_PRIV(dev_mgt), &chan_send);
	if (ret)
		return ret;

	resp_len = min(resp_len, result.data_len);
	memcpy(resp, result.resp_data, resp_len);

	return 0;
}

static void nbl_dev_chan_grc_process_resp(void *priv, u16 src_id, u16 msg_id,
					  void *data, u32 data_len)
{
	struct nbl_dev_mgt *dev_mgt = (struct nbl_dev_mgt *)priv;
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DEV_MGT_TO_CHAN_OPS(dev_mgt);
	struct nbl_chan_rdma_resp *param;
	struct nbl_chan_rdma_resp result = {0};
	struct nbl_chan_ack_info chan_ack;
	int err = NBL_CHAN_RESP_OK;
	struct nbl_aux_dev *dev_link = container_of(rdma_dev->grc_adev, struct nbl_aux_dev, adev);

	param = (struct nbl_chan_rdma_resp *)data;

	if (!dev_link->recv) {
		err = NBL_CHAN_RESP_ERR;
		NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GRC_PROCESS,
			     msg_id, err, &result, sizeof(result));
		chan_ops->send_ack(NBL_DEV_MGT_TO_CHAN_PRIV(dev_mgt), &chan_ack);
		return;
	}

	dev_link->recv(rdma_dev->grc_adev, param->resp_data, param->data_len, &result);

	NBL_CHAN_ACK(chan_ack, src_id, NBL_CHAN_MSG_GRC_PROCESS,
		     msg_id, err, &result, sizeof(result));
	chan_ops->send_ack(NBL_DEV_MGT_TO_CHAN_PRIV(dev_mgt), &chan_ack);
}

static int nbl_dev_grc_process_send(struct pci_dev *pdev, u8 *req_args, u8 req_len,
				    void *resp, u16 resp_len)
{
	struct nbl_adapter *adapter;
	struct nbl_dev_mgt *dev_mgt;
	struct nbl_chan_rdma_resp chan_resp = {0};
	struct nbl_dev_rdma *rdma_dev;
	struct nbl_aux_dev *dev_link;

	if (!pdev || !req_args || !resp)
		return -EINVAL;

	adapter = pci_get_drvdata(pdev);
	dev_mgt = (struct nbl_dev_mgt *)NBL_ADAPTER_TO_DEV_MGT(adapter);
	rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	dev_link = container_of(rdma_dev->grc_adev, struct nbl_aux_dev, adev);

	if (rdma_dev->has_grc) {
		int ret = 0;

		if (dev_link->recv) {
			dev_link->recv(rdma_dev->grc_adev, req_args, req_len, &chan_resp);
		} else {
			chan_resp.data_len = 1;
			chan_resp.resp_data[0] = 1;
			ret = -EINVAL;
		}
		resp_len = min(chan_resp.data_len, resp_len);
		memcpy(resp, chan_resp.resp_data, resp_len);
		return ret;
	} else {
		return nbl_dev_chan_grc_process_req(dev_mgt, req_args, req_len, resp, resp_len);
	}
}

static void nbl_dev_rdma_handle_abnormal_event_task(struct work_struct *work)
{
	struct nbl_dev_rdma *rdma_dev = container_of(work, struct nbl_dev_rdma,
						     abnormal_event_task);
	struct nbl_aux_dev *dev_link = NULL;

	if (rdma_dev->is_halting)
		return;

	if (rdma_dev && rdma_dev->grc_adev)
		dev_link = container_of(rdma_dev->grc_adev, struct nbl_aux_dev, adev);
	else if (rdma_dev && rdma_dev->adev)
		dev_link = container_of(rdma_dev->adev, struct nbl_aux_dev, adev);
	else if (rdma_dev && rdma_dev->bond_adev)
		dev_link = container_of(rdma_dev->bond_adev, struct nbl_aux_dev, adev);
	else
		return;

	if (dev_link && dev_link->abnormal_event_process)
		dev_link->abnormal_event_process(&dev_link->adev);
}

void nbl_dev_rdma_process_abnormal_event(struct nbl_dev_rdma *rdma_dev)
{
	if (rdma_dev && !rdma_dev->is_halting && rdma_dev->has_abnormal_event_task)
		nbl_common_queue_work_rdma(&rdma_dev->abnormal_event_task, false);
}

void nbl_dev_rdma_process_flr_event(struct nbl_dev_rdma *rdma_dev, u16 vsi_id)
{
	struct nbl_aux_dev *dev_link = container_of(rdma_dev->grc_adev, struct nbl_aux_dev, adev);

	if (rdma_dev && rdma_dev->grc_adev && dev_link->process_flr_event)
		dev_link->process_flr_event(rdma_dev->grc_adev, vsi_id);
}

static int nbl_dev_rdma_register_bond(struct pci_dev *pdev, bool enable)
{
	struct nbl_adapter *adapter;
	struct nbl_dev_mgt *dev_mgt;
	struct nbl_dev_rdma *rdma_dev;
	struct nbl_aux_dev *dev_link;
	struct nbl_common_info *common;
	struct nbl_service_ops *serv_ops;

	if (!pdev)
		return -EINVAL;

	adapter = pci_get_drvdata(pdev);
	dev_mgt = (struct nbl_dev_mgt *)NBL_ADAPTER_TO_DEV_MGT(adapter);
	serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	common = NBL_DEV_MGT_TO_COMMON(dev_mgt);

	if (!rdma_dev->bond_adev)
		return -EINVAL;

	dev_link = container_of(rdma_dev->bond_adev, struct nbl_aux_dev, adev);

	rdma_dev->bond_registered = enable;

	if (rdma_dev->bond_registered && nbl_dev_rdma_bond_active_num(dev_link->cdev_info) > 1 &&
	    !rdma_dev->bond_shaping_configed)
		nbl_dev_rdma_cfg_bond(dev_mgt, dev_link->cdev_info, true);
	else if (!rdma_dev->bond_registered && rdma_dev->bond_shaping_configed)
		nbl_dev_rdma_cfg_bond(dev_mgt, dev_link->cdev_info, false);

	return 0;
}

static void nbl_dev_rdma_form_lag_info(struct nbl_core_dev_lag_info *lag_info,
				       struct nbl_lag_member_list_param *list_param,
				       struct nbl_common_info *common)
{
	int i;

	lag_info->lag_num = list_param->lag_num;
	lag_info->lag_id = list_param->lag_id;
	nbl_debug(common, NBL_DEBUG_MAIN, "update lag id %u, lag num %u.",
		  list_param->lag_id, list_param->lag_num);

	for (i = 0; i < NBL_RDMA_LAG_MAX_PORTS; i++) {
		nbl_debug(common, NBL_DEBUG_MAIN, "update lag member %u, eth_id %u, vsi_id %u, active %u.",
			  i, list_param->member_list[i].eth_id,
			  list_param->member_list[i].vsi_id, list_param->member_list[i].active);
		lag_info->lag_mem[i].vsi_id = list_param->member_list[i].vsi_id;
		lag_info->lag_mem[i].eth_id = list_param->member_list[i].eth_id;
		lag_info->lag_mem[i].active = list_param->member_list[i].active;
	}
}

static void nbl_dev_rdma_update_bond_member(struct nbl_dev_mgt *dev_mgt,
					    struct nbl_lag_member_list_param *list_param)
{
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	struct nbl_core_dev_lag_info lag_info = {0};
	struct nbl_aux_dev *dev_link;

	if (!rdma_dev->bond_adev) {
		nbl_err(common, NBL_DEBUG_MAIN, "Something wrong, lag adev err");
		return;
	}

	dev_link = container_of(rdma_dev->bond_adev, struct nbl_aux_dev, adev);
	rdma_dev->lag_id = list_param->lag_id;

	nbl_dev_rdma_form_lag_info(&lag_info, list_param, common);

	memcpy(&dev_link->cdev_info->lag_info, &lag_info, sizeof(lag_info));

	if (dev_link->cdev_info->lag_mem_notify)
		dev_link->cdev_info->lag_mem_notify(rdma_dev->bond_adev, &lag_info);

	if (rdma_dev->bond_registered && nbl_dev_rdma_bond_active_num(dev_link->cdev_info) > 1 &&
	    !rdma_dev->bond_shaping_configed)
		nbl_dev_rdma_cfg_bond(dev_mgt, dev_link->cdev_info, true);
	else if (nbl_dev_rdma_bond_active_num(dev_link->cdev_info) < 2 &&
		 rdma_dev->bond_shaping_configed)
		nbl_dev_rdma_cfg_bond(dev_mgt, dev_link->cdev_info, false);
}

static int nbl_dev_rdma_update_adev_mtu(struct nbl_dev_mgt *dev_mgt,
					struct nbl_event_param *event_param)
{
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	int new_mtu = event_param->mtu;
	struct nbl_aux_dev *dev_link = NULL;

	if (rdma_dev && rdma_dev->grc_adev)
		dev_link = container_of(rdma_dev->grc_adev, struct nbl_aux_dev, adev);
	else if (rdma_dev && rdma_dev->adev)
		dev_link = container_of(rdma_dev->adev, struct nbl_aux_dev, adev);
	else if (rdma_dev && rdma_dev->bond_adev)
		dev_link = container_of(rdma_dev->bond_adev, struct nbl_aux_dev, adev);
	else
		return 0;

	if (dev_link && dev_link->cdev_info && dev_link->cdev_info->change_mtu_notify)
		dev_link->cdev_info->change_mtu_notify(&dev_link->adev, new_mtu);

	return 0;
}

static int nbl_dev_rdma_handle_bond_event(u16 type, void *event_data, void *callback_data)
{
	struct nbl_dev_mgt *dev_mgt = (struct nbl_dev_mgt *)callback_data;
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_dev_rdma_event_data *data = NULL;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	memcpy(&data->event_data, event_data, sizeof(data->event_data));
	data->type = type;
	data->callback_data = callback_data;

	/* Why we need a list here?
	 *
	 * First, we have to make sure we don't lose any notify. When we try to queue_work when
	 * there is already a work being processed, we don't want to lose that notify. e.g.
	 *
	 * CONTEXT_0: add_slave0 -> notify_0(lag_num=1) -> add_slave1 -> notify_1(lag_num=2)
	 * CONTEXT_1:                                   | -- process notify_0 -------> |
	 *
	 * Then why not simply use a single variable to store it? e.g.
	 *
	 * CONTEXT_0: add_slave0 -> notify_0 -> add_slave1 -> notify_1
	 * CONTEXT_1:                                                 | -- process notify_1 -> |
	 * VARIABLE:                | --   lag_num = 0   -- | | --        lag_num = 1        --|
	 *
	 * or
	 *
	 * CONTEXT_0: add_slave0 -> notify_0 -> add_slave1         -> notify_1
	 * CONTEXT_1:                           | process notify_0 |          | process notify_1 |
	 * VARIABLE:                | --       lag_num = 0      -- |  | --     lag_num = 1     --|
	 *
	 * This make sure that we always use the lastest param, functionally correct.
	 *
	 * But this will require the task function(nbl_dev_rdma_process_event_task) to lock all its
	 * body, for that we must make sure that once we get a param, we will use it until we
	 * finished all the process, or else we will have trouble for using differnet param while
	 * processing.
	 *
	 * But this requirement cannot be fulfilled. Consider this situation:
	 * CONTEXT_0: rtnl_lock -> add_slave0 -> notify_0 -> add_slave1 -> notify_1 -> event_lock
	 * CONTEXT_1:                      | --notify_0 -> event_lock -> ib_func -> rtnl_lock -- |
	 *
	 * At this moment, CONTEXT_0 have rtnl_lock but need event_lock, CONTEXT_1 have event_lock
	 * but need rtnl_lock, thus deadlock.
	 *
	 * Based on all of the above, we need a list to fix it. Each time we want to queue work, we
	 * queue a new entry on the list, and each time a task processing, it dequeues a entry.
	 * Then the lock only needs to lock the list itself(rather than the whole aux_dev process),
	 * thus no trouble for deadlock.
	 */
	mutex_lock(&rdma_dev->event_lock);
	/* Always add_tail and dequeue the first, to maintain the order of notify */
	list_add_tail(&data->node, &rdma_dev->event_param_list);
	mutex_unlock(&rdma_dev->event_lock);

	if (rdma_dev->event_ready)
		nbl_common_queue_work_rdma(&rdma_dev->event_task, true);

	return 0;
}

static int
nbl_dev_rdma_handle_mirror_outputport_event(u16 type, void *event_data, void *callback_data)
{
	bool mirror_enable = *(bool *)event_data;
	struct nbl_dev_mgt *dev_mgt = (struct nbl_dev_mgt *)callback_data;
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_dev_rdma_event_data *data = NULL;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->type = type;
	data->callback_data = callback_data;
	if (mirror_enable)
		data->event_data.subevent = NBL_SUBEVENT_RELEASE_ADEV;
	else
		data->event_data.subevent = NBL_SUBEVENT_CREATE_ADEV;

	mutex_lock(&rdma_dev->event_lock);
	/* Always add_tail and dequeue the first, to maintain the order of notify */
	list_add_tail(&data->node, &rdma_dev->event_param_list);
	mutex_unlock(&rdma_dev->event_lock);

	if (rdma_dev->event_ready)
		nbl_common_queue_work_rdma(&rdma_dev->event_task, true);

	return 0;
}

static int
nbl_dev_rdma_handle_mirror_selectport_event(u16 type, void *event_data, void *callback_data)
{
	bool mirror_enable = *(bool *)event_data;
	struct nbl_dev_mgt *dev_mgt = (struct nbl_dev_mgt *)callback_data;
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_aux_dev *dev_link;
	struct auxiliary_device *adev;

	nbl_dev_rdma_pending_and_flush_event_task(rdma_dev);

	adev = rdma_dev->adev ? rdma_dev->adev : rdma_dev->bond_adev;
	if (!adev)
		goto resume_event_task;

	if (rdma_dev->mirror_enable == mirror_enable)
		goto resume_event_task;

	rdma_dev->mirror_enable = mirror_enable;
	dev_link = container_of(adev, struct nbl_aux_dev, adev);
	if (!dev_link->cdev_info)
		goto resume_event_task;

	dev_link->cdev_info->mirror_enable = mirror_enable;
	if (dev_link->mirror_enable_notify)
		dev_link->mirror_enable_notify(adev, mirror_enable);

resume_event_task:
	nbl_dev_rdma_resume_event_task(rdma_dev);
	return 0;
}

static int nbl_dev_rdma_handle_offload_status(u16 type, void *event_data, void *callback_data)
{
	struct nbl_dev_mgt *dev_mgt = (struct nbl_dev_mgt *)callback_data;
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_event_offload_status_data *data =
		(struct nbl_event_offload_status_data *)event_data;
	struct nbl_aux_dev *dev_link;

	nbl_dev_rdma_pending_and_flush_event_task(rdma_dev);
	if (!rdma_dev->bond_adev)
		goto resume_event_task;

	if (data->pf_vsi_id != NBL_COMMON_TO_VSI_ID(NBL_DEV_MGT_TO_COMMON(dev_mgt)))
		goto resume_event_task;

	dev_link = container_of(rdma_dev->bond_adev, struct nbl_aux_dev, adev);
	if (dev_link->cdev_info && dev_link->cdev_info->offload_status_notify)
		dev_link->cdev_info->offload_status_notify(rdma_dev->bond_adev, data->status);

resume_event_task:
	nbl_dev_rdma_resume_event_task(rdma_dev);
	return 0;
}

static int nbl_dev_rdma_process_adev_event(void *event_data, void *callback_data)
{
	struct nbl_dev_mgt *dev_mgt = (struct nbl_dev_mgt *)callback_data;
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_event_param *event = (struct nbl_event_param *)event_data;
	struct nbl_lag_member_list_param *list_param = &event->param;
	struct nbl_rdma_register_param register_param = {0};
	struct nbl_core_dev_lag_info lag_info = {0};

	switch (event->subevent) {
	case NBL_SUBEVENT_CREATE_ADEV:
		if (!rdma_dev->adev) {
			serv_ops->register_rdma(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
						NBL_COMMON_TO_VSI_ID(common), &register_param);
			if (register_param.has_rdma)
				nbl_dev_create_rdma_aux_dev(dev_mgt, NBL_AUX_DEV_ROCE, NULL);
		}
		break;
	case NBL_SUBEVENT_RELEASE_ADEV:
		if (rdma_dev->adev) {
			nbl_dev_destroy_rdma_aux_dev(rdma_dev, &rdma_dev->adev);
			serv_ops->unregister_rdma(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
						  NBL_COMMON_TO_VSI_ID(common));
		}
		break;
	case NBL_SUBEVENT_CREATE_BOND_ADEV:
		if (!rdma_dev->bond_adev) {
			serv_ops->register_rdma_bond(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
						     list_param, &register_param);

			nbl_dev_rdma_form_lag_info(&lag_info, list_param, common);

			if (register_param.has_rdma) {
				rdma_dev->lag_id = list_param->lag_id;
				nbl_dev_create_rdma_aux_dev(dev_mgt, NBL_AUX_DEV_BOND, &lag_info);
			}
		}
		break;
	case NBL_SUBEVENT_RELEASE_BOND_ADEV:
		if (rdma_dev->bond_adev) {
			nbl_dev_destroy_rdma_aux_dev(rdma_dev, &rdma_dev->bond_adev);
			serv_ops->unregister_rdma_bond(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
						       rdma_dev->lag_id);
		}
		break;
	default:
		break;
	}

	return 0;
}

static int nbl_dev_rdma_process_event_task(struct work_struct *work)
{
	struct nbl_dev_rdma *rdma_dev = container_of(work, struct nbl_dev_rdma, event_task);
	struct nbl_dev_mgt *dev_mgt;
	struct nbl_common_info *common;
	struct nbl_lag_member_list_param *list_param;
	struct nbl_dev_rdma_event_data *data = NULL;
	struct nbl_event_param *event_param = NULL;

	if (!!atomic_read(&rdma_dev->adev_busy)) {
		msleep(20);
		goto queue_rework;
	}

	mutex_lock(&rdma_dev->event_lock);

	if (!nbl_list_empty(&rdma_dev->event_param_list)) {
		data = list_first_entry(&rdma_dev->event_param_list,
					struct nbl_dev_rdma_event_data, node);
		list_del(&data->node);
	}

	mutex_unlock(&rdma_dev->event_lock);

	if (!data)
		return 0;

	dev_mgt = (struct nbl_dev_mgt *)data->callback_data;
	common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	event_param = &data->event_data;
	list_param = &event_param->param;

	nbl_info(common, NBL_DEBUG_MAIN, "process rdma lag subevent %u.", event_param->subevent);

	switch (event_param->subevent) {
	case NBL_SUBEVENT_UPDATE_BOND_MEMBER:
		nbl_dev_rdma_update_bond_member(dev_mgt, list_param);
		break;
	case NBL_SUBEVENT_UPDATE_MTU:
		nbl_dev_rdma_update_adev_mtu(dev_mgt, event_param);
		break;
	default:
		nbl_dev_rdma_process_adev_event(event_param, dev_mgt);
		break;
	}

	kfree(data);

queue_rework:
	/* Always queue it again, because we don't know if there is another param need to process */
	nbl_common_queue_work_rdma(&rdma_dev->event_task, true);

	return 0;
}

static int nbl_dev_rdma_handle_reset_event(u16 type, void *event_data, void *callback_data)
{
	struct nbl_dev_rdma *rdma_dev = (struct nbl_dev_rdma *)callback_data;
	enum nbl_core_reset_event event = *(enum nbl_core_reset_event *)event_data;
	struct nbl_aux_dev *dev_link;
	struct auxiliary_device *adev;

	nbl_dev_rdma_pending_and_flush_event_task(rdma_dev);

	adev = rdma_dev->adev ? rdma_dev->adev : rdma_dev->bond_adev;
	if (!adev)
		goto resume_event_task;

	dev_link = container_of(adev, struct nbl_aux_dev, adev);
	if (dev_link->reset_event_notify)
		dev_link->reset_event_notify(adev, event);

	if (rdma_dev->has_grc && rdma_dev->grc_adev) {
		adev = rdma_dev->grc_adev;
		dev_link = container_of(adev, struct nbl_aux_dev, adev);
		if (dev_link->reset_event_notify)
			dev_link->reset_event_notify(adev, event);
	}

resume_event_task:
	nbl_dev_rdma_resume_event_task(rdma_dev);
	return 0;
}

static int nbl_dev_rdma_handle_change_mtu_event(u16 type, void *event_data, void *callback_data)
{
	struct nbl_dev_mgt *dev_mgt = (struct nbl_dev_mgt *)callback_data;
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	int new_mtu = *(int *)event_data;
	struct nbl_dev_rdma_event_data *data = NULL;

	/* Move mtu update event to adev task, to avoid adev driver probe hold the rtnl_lock.
	 * if flush the adev task will dead loop(the os has hold the rtnl_lock before call driver's
	 * set_mtu ops.
	 */

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->type = type;
	data->callback_data = callback_data;
	data->event_data.mtu = new_mtu;
	data->event_data.subevent = NBL_SUBEVENT_UPDATE_MTU;

	mutex_lock(&rdma_dev->event_lock);
	/* Always add_tail and dequeue the first, to maintain the order of notify */
	list_add_tail(&data->node, &rdma_dev->event_param_list);
	mutex_unlock(&rdma_dev->event_lock);

	if (rdma_dev->event_ready)
		nbl_common_queue_work_rdma(&rdma_dev->event_task, true);

	return 0;
}

size_t nbl_dev_rdma_qos_cfg_store(struct nbl_dev_mgt *dev_mgt, int offset,
				  const char *buf, size_t count)
{
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_aux_dev *dev_link;
	struct auxiliary_device *adev;

	if (rdma_dev->bond_adev) {
		adev = rdma_dev->bond_adev;
		dev_link = container_of(rdma_dev->bond_adev, struct nbl_aux_dev, adev);
	} else if (rdma_dev->adev) {
		adev = rdma_dev->adev;
		dev_link = container_of(adev, struct nbl_aux_dev, adev);
	} else {
		return -EINVAL;
	}

	if (dev_link->qos_cfg_store)
		return dev_link->qos_cfg_store(adev, offset, buf, count);

	return -EINVAL;
}

size_t nbl_dev_rdma_qos_cfg_show(struct nbl_dev_mgt *dev_mgt, int offset, char *buf)
{
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_aux_dev *dev_link;
	struct auxiliary_device *adev;

	if (rdma_dev->bond_adev) {
		adev = rdma_dev->bond_adev;
		dev_link = container_of(rdma_dev->bond_adev, struct nbl_aux_dev, adev);
	} else if (rdma_dev->adev) {
		adev = rdma_dev->adev;
		dev_link = container_of(adev, struct nbl_aux_dev, adev);
	} else {
		return -EINVAL;
	}

	if (dev_link->qos_cfg_show)
		return dev_link->qos_cfg_show(adev, offset, buf);

	return -EINVAL;
}

static struct nbl_core_dev_info *
nbl_dev_rdma_setup_cdev_info(struct nbl_dev_mgt *dev_mgt, u8 type,
			     struct nbl_core_dev_lag_info *lag_info)
{
	struct nbl_dev_common *common_dev = NBL_DEV_MGT_TO_COMMON_DEV(dev_mgt);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	struct nbl_msix_info *msix_info = NBL_DEV_COMMON_TO_MSIX_INFO(common_dev);
	struct nbl_core_dev_info *cdev_info = NULL;
	u16 base_vector_id = msix_info->serv_info[NBL_MSIX_RDMA_TYPE].base_vector_id;
	int irq_num, i;

	cdev_info = kzalloc(sizeof(*cdev_info), GFP_KERNEL);
	if (!cdev_info)
		goto malloc_cdev_info_err;

	cdev_info->dma_dev = NBL_COMMON_TO_DMA_DEV(common);
	cdev_info->pdev = NBL_COMMON_TO_PDEV(common);
	cdev_info->hw_addr = serv_ops->get_hw_addr(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt), NULL);
	cdev_info->real_hw_addr = serv_ops->get_real_hw_addr(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
							     NBL_COMMON_TO_VSI_ID(common));
	cdev_info->function_id = serv_ops->get_function_id(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
							   NBL_COMMON_TO_VSI_ID(common));
	cdev_info->netdev = NBL_DEV_MGT_TO_NET_DEV(dev_mgt)->netdev;

	cdev_info->vsi_id = NBL_COMMON_TO_VSI_ID(common);
	cdev_info->eth_mode = NBL_COMMON_TO_ETH_MODE(common);
	cdev_info->eth_id = NBL_COMMON_TO_ETH_ID(common);
	cdev_info->mem_type = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt)->mem_type;

	if (type == NBL_AUX_DEV_GRC)
		cdev_info->rdma_cap_num =
				serv_ops->get_rdma_cap_num(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt));

	serv_ops->get_real_bdf(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt), NBL_COMMON_TO_VSI_ID(common),
			       &cdev_info->real_bus, &cdev_info->real_dev,
			       &cdev_info->real_function);

	cdev_info->send = nbl_dev_grc_process_send;

	/* grc aux dev needs no interrupt */
	if (type == NBL_AUX_DEV_GRC)
		goto out;

	irq_num = msix_info->serv_info[NBL_MSIX_RDMA_TYPE].num;
	cdev_info->msix_entries = kcalloc(irq_num, sizeof(*cdev_info->msix_entries), GFP_KERNEL);
	if (!cdev_info->msix_entries)
		goto malloc_msix_entries_err;

	cdev_info->global_vector_id = kcalloc(irq_num, sizeof(*cdev_info->global_vector_id),
					      GFP_KERNEL);
	if (!cdev_info->global_vector_id)
		goto malloc_global_vector_id_err;

	for (i = 0; i < irq_num; i++) {
		memcpy(&cdev_info->msix_entries[i], &msix_info->msix_entries[i + base_vector_id],
		       sizeof(cdev_info->msix_entries[i]));
		cdev_info->global_vector_id[i] =
			serv_ops->get_global_vector(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
						    i + base_vector_id);
	}
	cdev_info->msix_count = irq_num;

	if (type == NBL_AUX_DEV_BOND && lag_info) {
		memcpy(&cdev_info->lag_info, lag_info, sizeof(cdev_info->lag_info));
		cdev_info->is_lag = true;
		cdev_info->register_bond = nbl_dev_rdma_register_bond;
	}

out:
	return cdev_info;

malloc_global_vector_id_err:
	kfree(cdev_info->msix_entries);
malloc_msix_entries_err:
	kfree(cdev_info);
malloc_cdev_info_err:
	return NULL;
}

static void nbl_dev_rdma_remove_cdev_info(struct nbl_core_dev_info *cdev_info)
{
	kfree(cdev_info->msix_entries);
	kfree(cdev_info->global_vector_id);
	kfree(cdev_info);
}

static void nbl_dev_adev_release(struct device *dev)
{
	struct nbl_aux_dev *dev_link;

	dev_link = container_of(dev, struct nbl_aux_dev, adev.dev);
	nbl_dev_rdma_remove_cdev_info(dev_link->cdev_info);
	kfree(dev_link);
}

static int nbl_dev_create_rdma_aux_dev(struct nbl_dev_mgt *dev_mgt, u8 type,
				       struct nbl_core_dev_lag_info *lag_info)
{
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	struct device *dev = NBL_DEV_MGT_TO_DEV(dev_mgt);
	struct nbl_aux_dev *dev_link;
	struct auxiliary_device *adev, **temp_adev = NULL;
	bool is_grc = false;
	int ret = 0;

	dev_link = kzalloc(sizeof(*dev_link), GFP_KERNEL);
	if (!dev_link)
		return -ENOMEM;

	adev = &dev_link->adev;

	adev->id = type == NBL_AUX_DEV_GRC ? NBL_COMMON_TO_BOARD_ID(common) : rdma_dev->adev_index;
	adev->dev.parent = dev;
	adev->dev.release = nbl_dev_adev_release;

	switch (type) {
	case NBL_AUX_DEV_GRC:
		rdma_dev->grc_adev = adev;
		adev->name = "nbl.roce_grc";
		temp_adev = &rdma_dev->grc_adev;
		is_grc = true;
		break;
	case NBL_AUX_DEV_ROCE:
		rdma_dev->adev = adev;
		adev->name = "nbl.roce";
		temp_adev = &rdma_dev->adev;
		break;
	case NBL_AUX_DEV_BOND:
		rdma_dev->bond_adev = adev;
		adev->name = "nbl.roce_bond";
		temp_adev = &rdma_dev->bond_adev;
		break;
	default:
		goto unknown_type_err;
	}

	dev_link->cdev_info = nbl_dev_rdma_setup_cdev_info(dev_mgt, type, lag_info);
	if (!dev_link->cdev_info) {
		ret = -ENOMEM;
		goto malloc_cdev_info_err;
	}

	dev_link->cdev_info->mirror_enable = rdma_dev->mirror_enable;
	ret = auxiliary_device_init(adev);
	if (ret) {
		dev_err(dev, "auxiliary_device_init fail ret= %d", ret);
		goto aux_dev_init_err;
	}

	ret = __auxiliary_device_add(adev, "nbl");
	if (ret) {
		dev_err(dev, "__auxiliary_device_add fail ret= %d", ret);
		goto aux_dev_add_err;
	}

	dev_info(dev, "nbl plug %d auxiliary device OK", type);
	return 0;

aux_dev_add_err:
	/* When uninit, it will call nbl_dev_adev_release, which will free dev_link.
	 * So just return.
	 */
	auxiliary_device_uninit(adev);
	if (temp_adev)
		*temp_adev = NULL;
	return ret;
aux_dev_init_err:
	nbl_dev_rdma_remove_cdev_info(dev_link->cdev_info);
malloc_cdev_info_err:
unknown_type_err:
	kfree(dev_link);
	if (temp_adev)
		*temp_adev = NULL;
	return ret;
}

static void nbl_dev_destroy_rdma_aux_dev(struct nbl_dev_rdma *rdma_dev,
					 struct auxiliary_device **adev)
{
	rdma_dev->is_halting = true;

	if (!adev || !*adev)
		return;

	if (rdma_dev->has_abnormal_event_task)
		nbl_common_flush_task(&rdma_dev->abnormal_event_task);

	auxiliary_device_delete(*adev);
	auxiliary_device_uninit(*adev);

	*adev = NULL;

	rdma_dev->is_halting = false;
}

int nbl_dev_setup_rdma_dev(struct nbl_adapter *adapter, struct nbl_init_param *param)
{
	struct nbl_dev_mgt *dev_mgt = (struct nbl_dev_mgt *)NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_dev_rdma *rdma_dev;
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_dev_common *common_dev = NBL_DEV_MGT_TO_COMMON_DEV(dev_mgt);
	struct nbl_msix_info *msix_info = NBL_DEV_COMMON_TO_MSIX_INFO(common_dev);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	struct nbl_rdma_register_param register_param = {0};
	struct nbl_event_callback event_callback = {0};
	bool has_grc = false;

	/* This must be performed after ctrl dev setup */
	if (param->caps.has_ctrl)
		serv_ops->setup_rdma_id(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt));

	serv_ops->register_rdma(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
				NBL_COMMON_TO_VSI_ID(common), &register_param);

	if (param->caps.has_grc)
		has_grc = true;

	if (!register_param.has_rdma && !has_grc)
		return 0;

	rdma_dev = devm_kzalloc(NBL_ADAPTER_TO_DEV(adapter),
				sizeof(struct nbl_dev_rdma), GFP_KERNEL);
	if (!rdma_dev)
		return -ENOMEM;

	rdma_dev->has_rdma = register_param.has_rdma;
	rdma_dev->has_grc = has_grc;
	rdma_dev->mem_type = register_param.mem_type;
	rdma_dev->adev_index = register_param.id;
	msix_info->serv_info[NBL_MSIX_RDMA_TYPE].num += register_param.intr_num;

	nbl_common_alloc_task(&rdma_dev->event_task, (void *)nbl_dev_rdma_process_event_task);
	INIT_LIST_HEAD(&rdma_dev->event_param_list);
	mutex_init(&rdma_dev->event_lock);
	if (!NBL_COMMON_TO_VF_CAP(common)) {
		event_callback.callback_data = dev_mgt;
		event_callback.callback = nbl_dev_rdma_handle_bond_event;
		nbl_event_register(NBL_EVENT_RDMA_BOND_UPDATE, &event_callback,
				   NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));
		event_callback.callback_data = dev_mgt;
		event_callback.callback = nbl_dev_rdma_handle_mirror_selectport_event;
		nbl_event_register(NBL_EVENT_MIRROR_SELECTPORT, &event_callback,
				   NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));
	} else {
		event_callback.callback_data = dev_mgt;
		event_callback.callback = nbl_dev_rdma_handle_mirror_outputport_event;
		nbl_event_register(NBL_EVENT_MIRROR_OUTPUTPORT_DEVLAYER, &event_callback,
				   NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));
	}

	NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt) = rdma_dev;

	return 0;
}

void nbl_dev_remove_rdma_dev(struct nbl_adapter *adapter)
{
	struct nbl_dev_mgt *dev_mgt = (struct nbl_dev_mgt *)NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	struct nbl_dev_rdma_event_data *data, *data_safe;
	struct nbl_event_callback event_callback = {0};

	if (!rdma_dev)
		return;

	if (!NBL_COMMON_TO_VF_CAP(common)) {
		event_callback.callback_data = dev_mgt;
		event_callback.callback = nbl_dev_rdma_handle_bond_event;
		nbl_event_unregister(NBL_EVENT_RDMA_BOND_UPDATE, &event_callback,
				     NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));

		event_callback.callback_data = dev_mgt;
		event_callback.callback = nbl_dev_rdma_handle_mirror_selectport_event;
		nbl_event_unregister(NBL_EVENT_MIRROR_SELECTPORT, &event_callback,
				     NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));
	} else {
		event_callback.callback_data = dev_mgt;
		event_callback.callback = nbl_dev_rdma_handle_mirror_outputport_event;
		nbl_event_unregister(NBL_EVENT_MIRROR_OUTPUTPORT_DEVLAYER, &event_callback,
				     NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));
	}

	mutex_lock(&rdma_dev->event_lock);
	list_for_each_entry_safe(data, data_safe, &rdma_dev->event_param_list, node) {
		list_del(&data->node);
		kfree(data);
	}

	mutex_unlock(&rdma_dev->event_lock);
	nbl_common_release_task(&rdma_dev->event_task);

	if (rdma_dev->has_rdma)
		serv_ops->unregister_rdma(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					  NBL_COMMON_TO_VSI_ID(common));

	if (NBL_DEV_MGT_TO_CTRL_DEV(dev_mgt))
		serv_ops->remove_rdma_id(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt));

	devm_kfree(NBL_ADAPTER_TO_DEV(adapter), rdma_dev);
	NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt) = NULL;
}

int nbl_dev_start_rdma_dev(struct nbl_adapter *adapter)
{
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	struct nbl_channel_ops *chan_ops = NBL_DEV_MGT_TO_CHAN_OPS(dev_mgt);
	struct nbl_event_callback event_callback = {0};
	int ret;

	if (!rdma_dev || (!rdma_dev->has_rdma && !rdma_dev->has_grc))
		return 0;

	if (!!NBL_DEV_MGT_TO_CTRL_DEV(dev_mgt)) {
		nbl_common_alloc_task(&rdma_dev->abnormal_event_task,
				      nbl_dev_rdma_handle_abnormal_event_task);
		rdma_dev->has_abnormal_event_task = true;
	}

	if (chan_ops->check_queue_exist(NBL_DEV_MGT_TO_CHAN_PRIV(dev_mgt),
					NBL_CHAN_TYPE_MAILBOX))
		chan_ops->register_msg(NBL_DEV_MGT_TO_CHAN_PRIV(dev_mgt), NBL_CHAN_MSG_GRC_PROCESS,
				       nbl_dev_chan_grc_process_resp, dev_mgt);

	if (rdma_dev->has_grc) {
		ret = nbl_dev_create_rdma_aux_dev(dev_mgt, NBL_AUX_DEV_GRC, NULL);
		if (ret)
			return ret;
	}

	if (rdma_dev->has_rdma) {
		ret = nbl_dev_create_rdma_aux_dev(dev_mgt, NBL_AUX_DEV_ROCE, NULL);
		if (ret)
			goto create_rdma_aux_err;
	}

	if (!NBL_COMMON_TO_VF_CAP(common)) {
		event_callback.callback_data = dev_mgt;
		event_callback.callback = nbl_dev_rdma_handle_offload_status;
		nbl_event_register(NBL_EVENT_OFFLOAD_STATUS_CHANGED, &event_callback,
				   NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));
	}

	event_callback.callback_data = rdma_dev;
	event_callback.callback = nbl_dev_rdma_handle_reset_event;
	nbl_event_register(NBL_EVENT_RESET_EVENT, &event_callback,
			   NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));

	event_callback.callback_data = dev_mgt;
	event_callback.callback = nbl_dev_rdma_handle_change_mtu_event;
	nbl_event_register(NBL_EVENT_CHANGE_MTU, &event_callback,
			   NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));

	rdma_dev->event_ready = true;
	nbl_common_queue_work_rdma(&rdma_dev->event_task, true);

	return 0;

create_rdma_aux_err:
	nbl_dev_destroy_rdma_aux_dev(rdma_dev, &rdma_dev->grc_adev);
	return ret;
}

void nbl_dev_stop_rdma_dev(struct nbl_adapter *adapter)
{
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	struct nbl_event_callback event_callback = {0};

	if (!rdma_dev)
		return;

	if (!NBL_COMMON_TO_VF_CAP(common)) {
		event_callback.callback_data = dev_mgt;
		event_callback.callback = nbl_dev_rdma_handle_offload_status;
		nbl_event_unregister(NBL_EVENT_OFFLOAD_STATUS_CHANGED, &event_callback,
				     NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));
	}

	rdma_dev->event_ready = false;
	nbl_common_flush_task(&rdma_dev->event_task);

	event_callback.callback_data = rdma_dev;
	event_callback.callback = nbl_dev_rdma_handle_reset_event;
	nbl_event_unregister(NBL_EVENT_RESET_EVENT, &event_callback,
			     NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));

	event_callback.callback_data = dev_mgt;
	event_callback.callback = nbl_dev_rdma_handle_change_mtu_event;
	nbl_event_unregister(NBL_EVENT_CHANGE_MTU, &event_callback,
			     NBL_COMMON_TO_VSI_ID(common), NBL_COMMON_TO_BOARD_ID(common));

	nbl_dev_destroy_rdma_aux_dev(rdma_dev, &rdma_dev->bond_adev);
	nbl_dev_destroy_rdma_aux_dev(rdma_dev, &rdma_dev->adev);
	nbl_dev_destroy_rdma_aux_dev(rdma_dev, &rdma_dev->grc_adev);

	if (rdma_dev->has_abnormal_event_task)
		nbl_common_release_task(&rdma_dev->abnormal_event_task);
}

int nbl_dev_resume_rdma_dev(struct nbl_adapter *adapter)
{
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);

	if (!rdma_dev || (!rdma_dev->has_rdma && !rdma_dev->has_grc))
		return 0;

	if (rdma_dev->has_abnormal_event_task)
		nbl_common_alloc_task(&rdma_dev->abnormal_event_task,
				      nbl_dev_rdma_handle_abnormal_event_task);

	nbl_common_alloc_task(&rdma_dev->event_task, nbl_dev_rdma_process_event_task);

	return 0;
}

int nbl_dev_suspend_rdma_dev(struct nbl_adapter *adapter)
{
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_dev_rdma *rdma_dev = NBL_DEV_MGT_TO_RDMA_DEV(dev_mgt);

	if (!rdma_dev)
		return 0;

	nbl_common_release_task(&rdma_dev->event_task);

	if (rdma_dev->has_abnormal_event_task)
		nbl_common_release_task(&rdma_dev->abnormal_event_task);

	return 0;
}
