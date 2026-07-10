// SPDX-License-Identifier: GPL-2.0-only

#include <linux/netdevice.h>
#include <linux/notifier.h>
#include <linux/rtnetlink.h>
#include <net/net_namespace.h>
#include <net/sock.h>
#include <net/netdev_queues.h>
#include <net/busy_poll.h>

#include "netdev-genl-gen.h"

struct netdev_nl_dump_ctx {
	unsigned long	ifindex;
	unsigned int	rxq_idx;
	unsigned int	txq_idx;
	unsigned int	napi_id;
};

static inline int nla_put_uint(struct sk_buff *skb, int attrtype, u64 value)
{
	u64 tmp64 = value;
	u32 tmp32 = value;

	if (tmp64 == tmp32)
		return nla_put_u32(skb, attrtype, tmp32);
	return nla_put(skb, attrtype, sizeof(u64), &tmp64);
}

#define NL_ASSERT_CTX_FITS(type_name)					\
	BUILD_BUG_ON(sizeof(type_name) >				\
		     sizeof_field(struct netlink_callback, ctx))

static struct netdev_nl_dump_ctx *netdev_dump_ctx(struct netlink_callback *cb)
{
	NL_ASSERT_CTX_FITS(struct netdev_nl_dump_ctx);

	return (struct netdev_nl_dump_ctx *)cb->ctx;
}

static int
netdev_nl_dev_fill(struct net_device *netdev, struct sk_buff *rsp,
		   const struct genl_info *info)
{
	void *hdr;

	hdr = genlmsg_iput(rsp, info);
	if (!hdr)
		return -EMSGSIZE;

	if (nla_put_u32(rsp, NETDEV_A_DEV_IFINDEX, netdev->ifindex) ||
	    nla_put_u64_64bit(rsp, NETDEV_A_DEV_XDP_FEATURES,
			      netdev->xdp_features, NETDEV_A_DEV_PAD)) {
		genlmsg_cancel(rsp, hdr);
		return -EINVAL;
	}

	if (netdev->xdp_features & NETDEV_XDP_ACT_XSK_ZEROCOPY) {
		if (nla_put_u32(rsp, NETDEV_A_DEV_XDP_ZC_MAX_SEGS,
				netdev->xdp_zc_max_segs)) {
			genlmsg_cancel(rsp, hdr);
			return -EINVAL;
		}
	}

	genlmsg_end(rsp, hdr);

	return 0;
}

static void
netdev_genl_dev_notify(struct net_device *netdev, int cmd)
{
	struct genl_info info;
	struct sk_buff *ntf;

	if (!genl_has_listeners(&netdev_nl_family, dev_net(netdev),
				NETDEV_NLGRP_MGMT))
		return;

	genl_info_init_ntf(&info, &netdev_nl_family, cmd);

	ntf = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!ntf)
		return;

	if (netdev_nl_dev_fill(netdev, ntf, &info)) {
		nlmsg_free(ntf);
		return;
	}

	genlmsg_multicast_netns(&netdev_nl_family, dev_net(netdev), ntf,
				0, NETDEV_NLGRP_MGMT, GFP_KERNEL);
}

int netdev_nl_dev_get_doit(struct sk_buff *skb, struct genl_info *info)
{
	struct net_device *netdev;
	struct sk_buff *rsp;
	u32 ifindex;
	int err;

	if (GENL_REQ_ATTR_CHECK(info, NETDEV_A_DEV_IFINDEX))
		return -EINVAL;

	ifindex = nla_get_u32(info->attrs[NETDEV_A_DEV_IFINDEX]);

	rsp = genlmsg_new(GENLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!rsp)
		return -ENOMEM;

	rtnl_lock();

	netdev = __dev_get_by_index(genl_info_net(info), ifindex);
	if (netdev)
		err = netdev_nl_dev_fill(netdev, rsp, info);
	else
		err = -ENODEV;

	rtnl_unlock();

	if (err)
		goto err_free_msg;

	return genlmsg_reply(rsp, info);

err_free_msg:
	nlmsg_free(rsp);
	return err;
}

int netdev_nl_dev_get_dumpit(struct sk_buff *skb, struct netlink_callback *cb)
{
	struct net *net = sock_net(skb->sk);
	struct net_device *netdev;
	int err = 0;

	rtnl_lock();
	for_each_netdev_dump(net, netdev, cb->args[0]) {
		err = netdev_nl_dev_fill(netdev, skb, genl_info_dump(cb));
		if (err < 0)
			break;
	}
	rtnl_unlock();

	if (err != -EMSGSIZE)
		return err;

	return skb->len;
}

#define NETDEV_STAT_NOT_SET		(~0ULL)

static void netdev_nl_stats_add(void *_sum, const void *_add, size_t size)
{
	const u64 *add = _add;
	u64 *sum = _sum;

	while (size) {
		if (*add != NETDEV_STAT_NOT_SET && *sum != NETDEV_STAT_NOT_SET)
			*sum += *add;
		sum++;
		add++;
		size -= 8;
	}
}

static int netdev_stat_put(struct sk_buff *rsp, unsigned int attr_id, u64 value)
{
	if (value == NETDEV_STAT_NOT_SET)
		return 0;
	return nla_put_uint(rsp, attr_id, value);
}

static int
netdev_nl_stats_write_rx(struct sk_buff *rsp, struct netdev_queue_stats_rx *rx)
{
	if (netdev_stat_put(rsp, NETDEV_A_QSTATS_RX_PACKETS, rx->packets) ||
	    netdev_stat_put(rsp, NETDEV_A_QSTATS_RX_BYTES, rx->bytes))
		return -EMSGSIZE;
	return 0;
}

static int
netdev_nl_stats_write_tx(struct sk_buff *rsp, struct netdev_queue_stats_tx *tx)
{
	if (netdev_stat_put(rsp, NETDEV_A_QSTATS_TX_PACKETS, tx->packets) ||
	    netdev_stat_put(rsp, NETDEV_A_QSTATS_TX_BYTES, tx->bytes))
		return -EMSGSIZE;
	return 0;
}

static int
netdev_nl_stats_queue(struct net_device *netdev, struct sk_buff *rsp,
		      u32 q_type, int i, const struct genl_info *info)
{
	const struct netdev_stat_ops *ops = netdev->stat_ops;
	struct netdev_queue_stats_rx rx;
	struct netdev_queue_stats_tx tx;
	void *hdr;

	hdr = genlmsg_iput(rsp, info);
	if (!hdr)
		return -EMSGSIZE;
	if (nla_put_u32(rsp, NETDEV_A_QSTATS_IFINDEX, netdev->ifindex) ||
	    nla_put_u32(rsp, NETDEV_A_QSTATS_QUEUE_TYPE, q_type) ||
	    nla_put_u32(rsp, NETDEV_A_QSTATS_QUEUE_ID, i))
		goto nla_put_failure;

	switch (q_type) {
	case NETDEV_QUEUE_TYPE_RX:
		memset(&rx, 0xff, sizeof(rx));
		ops->get_queue_stats_rx(netdev, i, &rx);
		if (!memchr_inv(&rx, 0xff, sizeof(rx)))
			goto nla_cancel;
		if (netdev_nl_stats_write_rx(rsp, &rx))
			goto nla_put_failure;
		break;
	case NETDEV_QUEUE_TYPE_TX:
		memset(&tx, 0xff, sizeof(tx));
		ops->get_queue_stats_tx(netdev, i, &tx);
		if (!memchr_inv(&tx, 0xff, sizeof(tx)))
			goto nla_cancel;
		if (netdev_nl_stats_write_tx(rsp, &tx))
			goto nla_put_failure;
		break;
	}

	genlmsg_end(rsp, hdr);
	return 0;

nla_cancel:
	genlmsg_cancel(rsp, hdr);
	return 0;
nla_put_failure:
	genlmsg_cancel(rsp, hdr);
	return -EMSGSIZE;
}

static int
netdev_nl_stats_by_queue(struct net_device *netdev, struct sk_buff *rsp,
			 const struct genl_info *info,
			 struct netdev_nl_dump_ctx *ctx)
{
	const struct netdev_stat_ops *ops = netdev->stat_ops;
	int i, err;

	if (!(netdev->flags & IFF_UP))
		return 0;

	i = ctx->rxq_idx;
	while (ops->get_queue_stats_rx && i < netdev->real_num_rx_queues) {
		err = netdev_nl_stats_queue(netdev, rsp, NETDEV_QUEUE_TYPE_RX,
					    i, info);
		if (err)
			return err;
		ctx->rxq_idx = i++;
	}
	i = ctx->txq_idx;
	while (ops->get_queue_stats_tx && i < netdev->real_num_tx_queues) {
		err = netdev_nl_stats_queue(netdev, rsp, NETDEV_QUEUE_TYPE_TX,
					    i, info);
		if (err)
			return err;
		ctx->txq_idx = i++;
	}

	ctx->rxq_idx = 0;
	ctx->txq_idx = 0;
	return 0;
}

static int
netdev_nl_stats_by_netdev(struct net_device *netdev, struct sk_buff *rsp,
			  const struct genl_info *info)
{
	struct netdev_queue_stats_rx rx_sum, rx;
	struct netdev_queue_stats_tx tx_sum, tx;
	const struct netdev_stat_ops *ops;
	void *hdr;
	int i;

	ops = netdev->stat_ops;
	/* Netdev can't guarantee any complete counters */
	if (!ops->get_base_stats)
		return 0;

	memset(&rx_sum, 0xff, sizeof(rx_sum));
	memset(&tx_sum, 0xff, sizeof(tx_sum));

	ops->get_base_stats(netdev, &rx_sum, &tx_sum);

	/* The op was there, but nothing reported, don't bother */
	if (!memchr_inv(&rx_sum, 0xff, sizeof(rx_sum)) &&
	    !memchr_inv(&tx_sum, 0xff, sizeof(tx_sum)))
		return 0;

	hdr = genlmsg_iput(rsp, info);
	if (!hdr)
		return -EMSGSIZE;
	if (nla_put_u32(rsp, NETDEV_A_QSTATS_IFINDEX, netdev->ifindex))
		goto nla_put_failure;

	for (i = 0; i < netdev->real_num_rx_queues; i++) {
		memset(&rx, 0xff, sizeof(rx));
		if (ops->get_queue_stats_rx)
			ops->get_queue_stats_rx(netdev, i, &rx);
		netdev_nl_stats_add(&rx_sum, &rx, sizeof(rx));
	}
	for (i = 0; i < netdev->real_num_tx_queues; i++) {
		memset(&tx, 0xff, sizeof(tx));
		if (ops->get_queue_stats_tx)
			ops->get_queue_stats_tx(netdev, i, &tx);
		netdev_nl_stats_add(&tx_sum, &tx, sizeof(tx));
	}

	if (netdev_nl_stats_write_rx(rsp, &rx_sum) ||
	    netdev_nl_stats_write_tx(rsp, &tx_sum))
		goto nla_put_failure;

	genlmsg_end(rsp, hdr);
	return 0;

nla_put_failure:
	genlmsg_cancel(rsp, hdr);
	return -EMSGSIZE;
}

static int
netdev_nl_qstats_get_dump_one(struct net_device *netdev, unsigned int scope,
			      struct sk_buff *skb, const struct genl_info *info,
			      struct netdev_nl_dump_ctx *ctx)
{
	if (!netdev->stat_ops)
		return 0;

	switch (scope) {
	case 0:
		return netdev_nl_stats_by_netdev(netdev, skb, info);
	case NETDEV_QSTATS_SCOPE_QUEUE:
		return netdev_nl_stats_by_queue(netdev, skb, info, ctx);
	}

	return -EINVAL;	/* Should not happen, per netlink policy */
}

int netdev_nl_qstats_get_dumpit(struct sk_buff *skb,
				struct netlink_callback *cb)
{
	struct netdev_nl_dump_ctx *ctx = netdev_dump_ctx(cb);
	const struct genl_info *info = genl_info_dump(cb);
	struct net *net = sock_net(skb->sk);
	struct net_device *netdev;
	unsigned int ifindex;
	unsigned int scope;
	int err = 0;

	scope = 0;
	if (info->attrs[NETDEV_A_QSTATS_SCOPE])
		scope = nla_get_uint(info->attrs[NETDEV_A_QSTATS_SCOPE]);

	ifindex = 0;
	if (info->attrs[NETDEV_A_QSTATS_IFINDEX])
		ifindex = nla_get_u32(info->attrs[NETDEV_A_QSTATS_IFINDEX]);

	if (ifindex) {
		netdev = __dev_get_by_index(net, ifindex);
		if (!netdev) {
			NL_SET_BAD_ATTR(info->extack,
					info->attrs[NETDEV_A_QSTATS_IFINDEX]);
			return -ENODEV;
		}
		if (netdev->stat_ops) {
			err = netdev_nl_qstats_get_dump_one(netdev, scope, skb,
							    info, ctx);
		} else {
			NL_SET_BAD_ATTR(info->extack,
					info->attrs[NETDEV_A_QSTATS_IFINDEX]);
			err = -EOPNOTSUPP;
		}
		return err;
	}

	for_each_netdev_dump(net, netdev, ctx->ifindex) {
		err = netdev_nl_qstats_get_dump_one(netdev, scope, skb,
						    info, ctx);
		if (err < 0)
			break;
	}

	return err;
}

static int netdev_genl_netdevice_event(struct notifier_block *nb,
				       unsigned long event, void *ptr)
{
	struct net_device *netdev = netdev_notifier_info_to_dev(ptr);

	switch (event) {
	case NETDEV_REGISTER:
		netdev_genl_dev_notify(netdev, NETDEV_CMD_DEV_ADD_NTF);
		break;
	case NETDEV_UNREGISTER:
		netdev_genl_dev_notify(netdev, NETDEV_CMD_DEV_DEL_NTF);
		break;
	case NETDEV_XDP_FEAT_CHANGE:
		netdev_genl_dev_notify(netdev, NETDEV_CMD_DEV_CHANGE_NTF);
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block netdev_genl_nb = {
	.notifier_call	= netdev_genl_netdevice_event,
};

static int __init netdev_genl_init(void)
{
	int err;

	err = register_netdevice_notifier(&netdev_genl_nb);
	if (err)
		return err;

	err = genl_register_family(&netdev_nl_family);
	if (err)
		goto err_unreg_ntf;

	return 0;

err_unreg_ntf:
	unregister_netdevice_notifier(&netdev_genl_nb);
	return err;
}

subsys_initcall(netdev_genl_init);
