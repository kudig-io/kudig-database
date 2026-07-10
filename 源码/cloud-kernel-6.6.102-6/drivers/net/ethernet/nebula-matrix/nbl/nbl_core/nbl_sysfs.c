// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#include "nbl_dev.h"

#define NBL_SET_RO_ATTR(dev_name_attr, attr_name, attr_show) do {			\
	typeof(dev_name_attr) _name_attr = (dev_name_attr);				\
	(_name_attr)->attr.name = __stringify(attr_name);				\
	(_name_attr)->attr.mode = SYSFS_PREALLOC | VERIFY_OCTAL_PERMISSIONS(0444);	\
	(_name_attr)->show = attr_show;							\
	(_name_attr)->store = NULL;							\
} while (0)

static ssize_t net_rep_show(struct device *dev,
			    struct nbl_netdev_name_attr *attr, char *buf)
{
	return scnprintf(buf, IFNAMSIZ, "%s\n", attr->net_dev_name);
}

const char *const nbl_sysfs_qos_name[] = {
	/* rdma */
	"save",
	"tc2pri",
	"sq_pri_map",
	"raq_pri_map",
	"pri_imap",
	"pfc_imap",
	"db_to_csch_en",
	"sw_db_csch_th",
	"csch_qlen_th",
	"poll_wgt",
	"sp_wrr",
	"tc_wgt",

	"pfc",
	"pfc_buffer",
	"trust",
	"dscp2prio",
	"rdma_bw",
	"rdma_rate",
	"net_rate",
};

const char *const nbl_sysfs_mirror_name[] = {
	"configure_down_mirror",
	"configure_up_mirror",
};

static ssize_t rdma_rate_show(struct nbl_sysfs_qos_info *qos_info, char *buf)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	u32 rdma_rate = 0;

	serv_ops->get_rdma_rate(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt), &rdma_rate);

	return sprintf(buf, "%u\n", rdma_rate);
}

static ssize_t rdma_rate_store(struct nbl_sysfs_qos_info *qos_info, const char *buf, size_t count)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	unsigned long rate;
	int ret;

	ret = kstrtoul(buf, 10, &rate);
	if (ret)
		return -EINVAL;

	ret = serv_ops->set_rate_limit(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
				       NBL_TRAFFIC_RDMA_TYPE, rate);
	if (ret) {
		netdev_err(net_dev->netdev, "configure_rdma_rate_limit: %s failed\n", buf);
		return -EIO;
	}

	return count;
}

static ssize_t net_rate_show(struct nbl_sysfs_qos_info *qos_info, char *buf)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	u32 net_rate = 0;

	serv_ops->get_net_rate(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt), &net_rate);

	return sprintf(buf, "%u\n", net_rate);
}

static ssize_t net_rate_store(struct nbl_sysfs_qos_info *qos_info, const char *buf, size_t count)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	unsigned long rate;
	int ret;

	ret = kstrtoul(buf, 10, &rate);
	if (ret)
		return -EINVAL;

	ret = serv_ops->set_rate_limit(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
				       NBL_TRAFFIC_NET_TYPE, rate);
	if (ret) {
		netdev_err(net_dev->netdev, "configure_net_rate_limit: %s failed\n", buf);
		return -EIO;
	}

	return count;
}

static ssize_t rdma_bw_show(struct nbl_sysfs_qos_info *qos_info, char *buf)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	int rdma_bw = 0;
	ssize_t ret;

	serv_ops->get_rdma_bw(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt), &rdma_bw);

	ret = snprintf(buf, PAGE_SIZE, "rdma:%d, normal:%d\n",
		       rdma_bw, NBL_MAX_BW - rdma_bw);
	return ret;
}

static ssize_t rdma_bw_store(struct nbl_sysfs_qos_info *qos_info, const char *buf, size_t count)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	int rdma = 0, normal = 0;
	int ret;

	if (sscanf(buf, "rdma:%d,normal:%d", &rdma, &normal) != 2) {
		pr_err("Invalid format, expected: rdma:<x>,normal:<y>\n");
		return -EINVAL;
	}

	if (rdma + normal != NBL_MAX_BW) {
		pr_err("Invalid value: sum must be 100\n");
		return -EINVAL;
	}

	ret = serv_ops->configure_rdma_bw(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					  NBL_COMMON_TO_ETH_ID(common), rdma);
	if (ret) {
		netdev_err(net_dev->netdev, "configure_rdma_bw: %s failed\n", buf);
		return -EIO;
	}

	return count;
}

static ssize_t dscp2prio_show(struct nbl_sysfs_qos_info *qos_info, char *buf)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);

	return serv_ops->dscp2prio_show(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					NBL_COMMON_TO_ETH_ID(common), buf);
}

static ssize_t dscp2prio_store(struct nbl_sysfs_qos_info *qos_info, const char *buf, size_t count)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);

	return serv_ops->configure_dscp2prio(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					     NBL_COMMON_TO_ETH_ID(common),
					     buf, count);
}

static ssize_t trust_mode_show(struct nbl_sysfs_qos_info *qos_info, char *buf)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);

	return serv_ops->trust_mode_show(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					 NBL_COMMON_TO_ETH_ID(common), buf);
}

static ssize_t trust_mode_store(struct nbl_sysfs_qos_info *qos_info, const char *buf, size_t count)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	u8 trust_mode;
	int ret;

	if (strncmp(buf, "dscp", 4) == 0) {
		trust_mode = NBL_TRUST_MODE_DSCP;
	} else if (strncmp(buf, "802.1p", 6) == 0) {
		trust_mode = NBL_TRUST_MODE_8021P;
	} else {
		netdev_err(net_dev->netdev, "Invalid trust mode: %s\n", buf);
			return -EINVAL;
	}

	ret = serv_ops->configure_trust(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					NBL_COMMON_TO_ETH_ID(common), trust_mode);
	if (ret) {
		netdev_err(net_dev->netdev, "configure_qos trust mode: %s failed\n", buf);
		return -EIO;
	}

	netdev_info(net_dev->netdev, "Trust mode set to %s\n", buf);
	return count;
}

static ssize_t pfc_buffer_size_show(struct nbl_sysfs_qos_info *qos_info, char *buf)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);

	return serv_ops->pfc_buffer_size_show(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					      NBL_COMMON_TO_ETH_ID(common), buf);
}

static ssize_t pfc_buffer_size_store(struct nbl_sysfs_qos_info *qos_info,
				     const char *buf, size_t count)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	int prio, xoff, xon;
	int ret;

	if (sscanf(buf, "%d,%d,%d", &prio, &xoff, &xon) != 3)
		return -EINVAL;

	if (prio < 0 || prio >= NBL_MAX_PFC_PRIORITIES)
		return -EINVAL;

	ret = serv_ops->set_pfc_buffer_size(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					    NBL_COMMON_TO_ETH_ID(common), prio, xoff, xon);
	if (ret) {
		netdev_err(net_dev->netdev, "set_pfc_buffer_size failed\n");
		return ret;
	}

	return count;
}

static ssize_t pfc_show(struct nbl_sysfs_qos_info *qos_info, char *buf)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);

	return serv_ops->pfc_show(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
				  NBL_COMMON_TO_ETH_ID(common), buf);
}

static ssize_t pfc_store(struct nbl_sysfs_qos_info *qos_info, const char *buf, size_t count)
{
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	u8 pfc_config[NBL_MAX_PFC_PRIORITIES];
	int ret, i;
	ssize_t len = count;

	while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == ' '))
		len--;

	if (len == 0) {
		netdev_err(net_dev->netdev, "Invalid input: no data to parse.\n");
		return count;
	}

	if (len != 15) {
		netdev_err(net_dev->netdev, "Invalid input length %ld.\n", len);
		return -EINVAL;
	}

	ret = sscanf(buf, "%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd,%hhd",
		     &pfc_config[0], &pfc_config[1], &pfc_config[2], &pfc_config[3],
		     &pfc_config[4], &pfc_config[5], &pfc_config[6], &pfc_config[7]);

	if (ret != NBL_MAX_PFC_PRIORITIES) {
		netdev_err(net_dev->netdev, "Failed to parse PFC. Expected 8 got %d\n", ret);
		return -EINVAL;
	}

	netdev_info(net_dev->netdev, "Parsed PFC configuration: %u %u %u %u %u %u %u %u\n",
		    pfc_config[0], pfc_config[1], pfc_config[2], pfc_config[3],
		    pfc_config[4], pfc_config[5], pfc_config[6], pfc_config[7]);

	for (i = 0; i < NBL_MAX_PFC_PRIORITIES; i++)
		if (pfc_config[i] > 1)
			return -EINVAL;

	ret = serv_ops->configure_pfc(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
				      NBL_COMMON_TO_ETH_ID(common), pfc_config);
	if (ret) {
		netdev_err(net_dev->netdev, "configure_qos trust mode: %s failed\n", buf);
		return -EIO;
	}

	return count;
}

static ssize_t nbl_qos_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct nbl_sysfs_qos_info *qos_info =
					container_of(attr, struct nbl_sysfs_qos_info, kobj_attr);
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);

	switch (qos_info->offset) {
	case NBL_QOS_PFC:
		return pfc_show(qos_info, buf);
	case NBL_QOS_TRUST:
		return trust_mode_show(qos_info, buf);
	case NBL_QOS_DSCP2PRIO:
		return dscp2prio_show(qos_info, buf);
	case NBL_QOS_PFC_BUFFER:
		return pfc_buffer_size_show(qos_info, buf);
	case NBL_QOS_RDMA_BW:
		return rdma_bw_show(qos_info, buf);
	case NBL_QOS_RDMA_RATE:
		return rdma_rate_show(qos_info, buf);
	case NBL_QOS_NET_RATE:
		return net_rate_show(qos_info, buf);
	case NBL_QOS_RDMA_SAVE:
	case NBL_QOS_RDMA_TC2PRI:
	case NBL_QOS_RDMA_SQ_PRI_MAP:
	case NBL_QOS_RDMA_RAQ_PRI_MAP:
	case NBL_QOS_RDMA_PRI_IMAP:
	case NBL_QOS_RDMA_PFC_IMAP:
	case NBL_QOS_RDMA_DB_TO_CSCH_EN:
	case NBL_QOS_RDMA_SW_DB_CSCH_TH:
	case NBL_QOS_RDMA_CSCH_QLEN_TH:
	case NBL_QOS_RDMA_POLL_WGT:
	case NBL_QOS_RDMA_SP_WRR:
	case NBL_QOS_RDMA_TC_WGT:
		return nbl_dev_rdma_qos_cfg_show(dev_mgt, qos_info->offset, buf);
	default:
		return -EINVAL;
	}
}

static ssize_t nbl_qos_store(struct kobject *kobj, struct kobj_attribute *attr,
			     const char *buf, size_t count)
{
	struct nbl_sysfs_qos_info *qos_info =
					container_of(attr, struct nbl_sysfs_qos_info, kobj_attr);
	struct nbl_dev_net *net_dev = qos_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);

	switch (qos_info->offset) {
	case NBL_QOS_PFC:
		return pfc_store(qos_info, buf, count);
	case NBL_QOS_TRUST:
		return trust_mode_store(qos_info, buf, count);
	case NBL_QOS_DSCP2PRIO:
		return dscp2prio_store(qos_info, buf, count);
	case NBL_QOS_PFC_BUFFER:
		return pfc_buffer_size_store(qos_info, buf, count);
	case NBL_QOS_RDMA_BW:
		return rdma_bw_store(qos_info, buf, count);
	case NBL_QOS_RDMA_RATE:
		return rdma_rate_store(qos_info, buf, count);
	case NBL_QOS_NET_RATE:
		return net_rate_store(qos_info, buf, count);
	case NBL_QOS_RDMA_SAVE:
	case NBL_QOS_RDMA_TC2PRI:
	case NBL_QOS_RDMA_SQ_PRI_MAP:
	case NBL_QOS_RDMA_RAQ_PRI_MAP:
	case NBL_QOS_RDMA_PRI_IMAP:
	case NBL_QOS_RDMA_PFC_IMAP:
	case NBL_QOS_RDMA_DB_TO_CSCH_EN:
	case NBL_QOS_RDMA_SW_DB_CSCH_TH:
	case NBL_QOS_RDMA_CSCH_QLEN_TH:
	case NBL_QOS_RDMA_POLL_WGT:
	case NBL_QOS_RDMA_SP_WRR:
	case NBL_QOS_RDMA_TC_WGT:
		return nbl_dev_rdma_qos_cfg_store(dev_mgt, qos_info->offset, buf, count);
	default:
		return -EINVAL;
	}
}

static ssize_t nbl_mirror_select_port_show(struct nbl_sysfs_mirror_info *mirror_info,
					   char *buf)
{
	ssize_t ret;

	ret = snprintf(buf, PAGE_SIZE, "mirror_en: %d, mirror_port: vf%d\n",
		       mirror_info->mirror_en, mirror_info->vf_id);
	return ret;
}

static ssize_t nbl_mirror_select_port_store(struct nbl_sysfs_mirror_info *mirror_info,
					    const char *buf, size_t count, int dir)
{
	struct nbl_dev_net *net_dev = mirror_info->net_dev;
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	int vf_id;
	int mirror_en;
	int ret;
	u16 function_id = U16_MAX;
	u8 mt_id;

	if (sscanf(buf, "mirror_en: %d, mirror_port: vf%d", &mirror_en, &vf_id) != 2)
		return -EINVAL;

	function_id = serv_ops->get_vf_function_id(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt), vf_id);
	if (function_id == U16_MAX) {
		netdev_info(net_dev->netdev, "vf id %d invalid\n", vf_id);
		return -EINVAL;
	}

	serv_ops->get_mirror_table_id(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
				      NBL_COMMON_TO_VSI_ID(common), dir, !!mirror_en, &mt_id);

	if (mt_id == 8) {
		netdev_err(net_dev->netdev, "The mirror table configuration is full!");
		return -EINVAL;
	}

	ret = serv_ops->configure_mirror(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt), common->mgt_pf,
					 !!mirror_en, dir, mt_id);
	if (ret) {
		netdev_err(net_dev->netdev, "configure mirror failed\n");
		return -EIO;
	}

	ret = serv_ops->configure_mirror_table(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt),
					       !!mirror_en, function_id, mt_id);
	if (ret) {
		netdev_err(net_dev->netdev, "configure mirror table failed\n");
		return -EIO;
	}

	mirror_info->mirror_en = mirror_en;
	mirror_info->vf_id = vf_id;
	return ret ? ret : count;
}

static ssize_t nbl_mirror_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	struct nbl_sysfs_mirror_info *mirror_info =
					container_of(attr, struct nbl_sysfs_mirror_info, kobj_attr);

	switch (mirror_info->offset) {
	case NBL_MIRROR_SELECT_SRC_PORT:
	case NBL_MIRROR_SELECT_DST_PORT:
		return nbl_mirror_select_port_show(mirror_info, buf);
	default:
		return -EINVAL;
	}
}

static ssize_t nbl_mirror_store(struct kobject *kobj, struct kobj_attribute *attr,
				const char *buf, size_t count)
{
	struct nbl_sysfs_mirror_info *mirror_info =
					container_of(attr, struct nbl_sysfs_mirror_info, kobj_attr);

	switch (mirror_info->offset) {
	case NBL_MIRROR_SELECT_SRC_PORT:
	case NBL_MIRROR_SELECT_DST_PORT:
		return nbl_mirror_select_port_store(mirror_info, buf, count,
						    mirror_info->offset);
	default:
		return -EINVAL;
	}
}

int nbl_netdev_add_sysfs(struct net_device *netdev, struct nbl_dev_net *net_dev)
{
	int ret;
	int i;

	net_dev->qos_config.qos_kobj = kobject_create_and_add("qos", &netdev->dev.kobj);
	if (!net_dev->qos_config.qos_kobj)
		return -ENOMEM;

	for (i = 0; i < NBL_QOS_TYPE_MAX; i++) {
		net_dev->qos_config.qos_info[i].net_dev = net_dev;
		net_dev->qos_config.qos_info[i].offset = i;
		/* create qos sysfs */
		sysfs_attr_init(&net_dev->qos_config.qos_info[i].kobj_attr.attr);
		net_dev->qos_config.qos_info[i].kobj_attr.attr.name = nbl_sysfs_qos_name[i];
		net_dev->qos_config.qos_info[i].kobj_attr.attr.mode = 0644;
		net_dev->qos_config.qos_info[i].kobj_attr.show = nbl_qos_show;
		net_dev->qos_config.qos_info[i].kobj_attr.store = nbl_qos_store;
		ret = sysfs_create_file(net_dev->qos_config.qos_kobj,
					&net_dev->qos_config.qos_info[i].kobj_attr.attr);
		if (ret)
			netdev_err(netdev, "Failed to create %s sysfs file\n",
				   nbl_sysfs_qos_name[i]);
	}

	return 0;
}

int nbl_netdev_add_mirror_sysfs(struct net_device *netdev, struct nbl_dev_net *net_dev)
{
	int ret;
	int i;

	net_dev->mirror_config.mirror_kobj = kobject_create_and_add("mirror", &netdev->dev.kobj);
	if (!net_dev->mirror_config.mirror_kobj)
		return -ENOMEM;

	for (i = 0; i < NBL_MIRROR_TYPE_MAX; i++) {
		net_dev->mirror_config.mirror_info[i].net_dev = net_dev;
		net_dev->mirror_config.mirror_info[i].offset = i;

		sysfs_attr_init(&net_dev->mirror_config.mirror_info[i].kobj_attr.attr);
		net_dev->mirror_config.mirror_info[i].kobj_attr.attr.name =
								nbl_sysfs_mirror_name[i];
		net_dev->mirror_config.mirror_info[i].kobj_attr.attr.mode = 0644;
		net_dev->mirror_config.mirror_info[i].kobj_attr.show = nbl_mirror_show;
		net_dev->mirror_config.mirror_info[i].kobj_attr.store = nbl_mirror_store;

		ret = sysfs_create_file(net_dev->mirror_config.mirror_kobj,
					&net_dev->mirror_config.mirror_info[i].kobj_attr.attr);

		if (ret)
			netdev_err(netdev, "Failed to create %s sysfs file\n",
				   nbl_sysfs_mirror_name[i]);
	}
	return 0;
}

void nbl_netdev_remove_sysfs(struct nbl_dev_net *net_dev)
{
	int i;

	if (!net_dev->qos_config.qos_kobj)
		return;

	for (i = 0; i < NBL_QOS_TYPE_MAX; i++)
		sysfs_remove_file(net_dev->qos_config.qos_kobj,
				  &net_dev->qos_config.qos_info[i].kobj_attr.attr);

	kobject_put(net_dev->qos_config.qos_kobj);
}

void nbl_netdev_remove_mirror_sysfs(struct nbl_dev_net *net_dev)
{
	struct nbl_netdev_priv *net_priv = netdev_priv(net_dev->netdev);
	struct nbl_adapter *adapter = net_priv->adapter;
	struct nbl_dev_mgt *dev_mgt = NBL_ADAPTER_TO_DEV_MGT(adapter);
	struct nbl_common_info *common = NBL_DEV_MGT_TO_COMMON(dev_mgt);
	struct nbl_service_ops *serv_ops = NBL_DEV_MGT_TO_SERV_OPS(dev_mgt);
	int i;

	serv_ops->clear_mirror_cfg(NBL_DEV_MGT_TO_SERV_PRIV(dev_mgt), common->mgt_pf);

	if (!net_dev->mirror_config.mirror_kobj)
		return;

	for (i = 0; i < NBL_MIRROR_TYPE_MAX; i++)
		sysfs_remove_file(net_dev->mirror_config.mirror_kobj,
				  &net_dev->mirror_config.mirror_info[i].kobj_attr.attr);

	kobject_put(net_dev->mirror_config.mirror_kobj);
}

void nbl_net_add_name_attr(struct nbl_netdev_name_attr *attr, char *rep_name)
{
	sysfs_attr_init(&attr->attr);
	NBL_SET_RO_ATTR(attr, dev_name, net_rep_show);
	strscpy(attr->net_dev_name, rep_name, IFNAMSIZ);
}

void nbl_net_remove_dev_attr(struct nbl_dev_net *net_dev)
{
	sysfs_remove_file(&net_dev->netdev->dev.kobj, &net_dev->dev_attr.dev_name_attr.attr);
}
