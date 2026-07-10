// SPDX-License-Identifier: GPL-2.0-only
/*
 * Shared Memory Communications over RDMA (SMC-R) and RoCE
 *
 * SMC statistics netlink routines
 *
 * Copyright IBM Corp. 2021
 *
 * Author(s):  Guvenc Gulce
 */
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/percpu.h>
#include <linux/ctype.h>
#include <linux/smc.h>
#include <net/genetlink.h>
#include <net/sock.h>
#include "smc_netlink.h"
#include "smc_stats.h"
#include "smc_cdc.h"

int smc_stats_init(struct net *net)
{
	net->smc.fback_rsn = kzalloc(sizeof(*net->smc.fback_rsn), GFP_KERNEL);
	if (!net->smc.fback_rsn)
		goto err_fback;
	net->smc.smc_stats = alloc_percpu(struct smc_stats);
	if (!net->smc.smc_stats)
		goto err_stats;
	mutex_init(&net->smc.mutex_fback_rsn);
	return 0;

err_stats:
	kfree(net->smc.fback_rsn);
err_fback:
	return -ENOMEM;
}

void smc_stats_exit(struct net *net)
{
	kfree(net->smc.fback_rsn);
	if (net->smc.smc_stats)
		free_percpu(net->smc.smc_stats);
}

static int smc_nl_fill_stats_rmb_data(struct sk_buff *skb,
				      struct smc_stats *stats, int tech,
				      int type)
{
	struct smc_stats_rmbcnt *stats_rmb_cnt;
	struct nlattr *attrs;

	if (type == SMC_NLA_STATS_T_TX_RMB_STATS)
		stats_rmb_cnt = &stats->smc[tech].rmb_tx;
	else
		stats_rmb_cnt = &stats->smc[tech].rmb_rx;

	attrs = nla_nest_start(skb, type);
	if (!attrs)
		goto errout;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_RMB_REUSE_CNT,
			      stats_rmb_cnt->reuse_cnt,
			      SMC_NLA_STATS_RMB_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_RMB_SIZE_SM_PEER_CNT,
			      stats_rmb_cnt->buf_size_small_peer_cnt,
			      SMC_NLA_STATS_RMB_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_RMB_SIZE_SM_CNT,
			      stats_rmb_cnt->buf_size_small_cnt,
			      SMC_NLA_STATS_RMB_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_RMB_FULL_PEER_CNT,
			      stats_rmb_cnt->buf_full_peer_cnt,
			      SMC_NLA_STATS_RMB_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_RMB_FULL_CNT,
			      stats_rmb_cnt->buf_full_cnt,
			      SMC_NLA_STATS_RMB_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_RMB_ALLOC_CNT,
			      stats_rmb_cnt->alloc_cnt,
			      SMC_NLA_STATS_RMB_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_RMB_DGRADE_CNT,
			      stats_rmb_cnt->dgrade_cnt,
			      SMC_NLA_STATS_RMB_PAD))
		goto errattr;

	nla_nest_end(skb, attrs);
	return 0;

errattr:
	nla_nest_cancel(skb, attrs);
errout:
	return -EMSGSIZE;
}

static int smc_nl_fill_stats_bufsize_data(struct sk_buff *skb,
					  struct smc_stats *stats, int tech,
					  int type)
{
	struct smc_stats_memsize *stats_pload;
	struct nlattr *attrs;

	if (type == SMC_NLA_STATS_T_TXPLOAD_SIZE)
		stats_pload = &stats->smc[tech].tx_pd;
	else if (type == SMC_NLA_STATS_T_RXPLOAD_SIZE)
		stats_pload = &stats->smc[tech].rx_pd;
	else if (type == SMC_NLA_STATS_T_TX_RMB_SIZE)
		stats_pload = &stats->smc[tech].tx_rmbsize;
	else if (type == SMC_NLA_STATS_T_RX_RMB_SIZE)
		stats_pload = &stats->smc[tech].rx_rmbsize;
	else
		goto errout;

	attrs = nla_nest_start(skb, type);
	if (!attrs)
		goto errout;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_PLOAD_8K,
			      stats_pload->buf[SMC_BUF_8K],
			      SMC_NLA_STATS_PLOAD_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_PLOAD_16K,
			      stats_pload->buf[SMC_BUF_16K],
			      SMC_NLA_STATS_PLOAD_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_PLOAD_32K,
			      stats_pload->buf[SMC_BUF_32K],
			      SMC_NLA_STATS_PLOAD_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_PLOAD_64K,
			      stats_pload->buf[SMC_BUF_64K],
			      SMC_NLA_STATS_PLOAD_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_PLOAD_128K,
			      stats_pload->buf[SMC_BUF_128K],
			      SMC_NLA_STATS_PLOAD_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_PLOAD_256K,
			      stats_pload->buf[SMC_BUF_256K],
			      SMC_NLA_STATS_PLOAD_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_PLOAD_512K,
			      stats_pload->buf[SMC_BUF_512K],
			      SMC_NLA_STATS_PLOAD_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_PLOAD_1024K,
			      stats_pload->buf[SMC_BUF_1024K],
			      SMC_NLA_STATS_PLOAD_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_PLOAD_G_1024K,
			      stats_pload->buf[SMC_BUF_G_1024K],
			      SMC_NLA_STATS_PLOAD_PAD))
		goto errattr;

	nla_nest_end(skb, attrs);
	return 0;

errattr:
	nla_nest_cancel(skb, attrs);
errout:
	return -EMSGSIZE;
}

static int smc_nl_fill_stats_tech_data(struct sk_buff *skb,
				       struct smc_stats *stats, int tech)
{
	struct smc_stats_tech *smc_tech;
	struct nlattr *attrs;

	smc_tech = &stats->smc[tech];
	if (tech == SMC_TYPE_D)
		attrs = nla_nest_start(skb, SMC_NLA_STATS_SMCD_TECH);
	else
		attrs = nla_nest_start(skb, SMC_NLA_STATS_SMCR_TECH);

	if (!attrs)
		goto errout;
	if (smc_nl_fill_stats_rmb_data(skb, stats, tech,
				       SMC_NLA_STATS_T_TX_RMB_STATS))
		goto errattr;
	if (smc_nl_fill_stats_rmb_data(skb, stats, tech,
				       SMC_NLA_STATS_T_RX_RMB_STATS))
		goto errattr;
	if (smc_nl_fill_stats_bufsize_data(skb, stats, tech,
					   SMC_NLA_STATS_T_TXPLOAD_SIZE))
		goto errattr;
	if (smc_nl_fill_stats_bufsize_data(skb, stats, tech,
					   SMC_NLA_STATS_T_RXPLOAD_SIZE))
		goto errattr;
	if (smc_nl_fill_stats_bufsize_data(skb, stats, tech,
					   SMC_NLA_STATS_T_TX_RMB_SIZE))
		goto errattr;
	if (smc_nl_fill_stats_bufsize_data(skb, stats, tech,
					   SMC_NLA_STATS_T_RX_RMB_SIZE))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_CLNT_V1_SUCC,
			      smc_tech->clnt_v1_succ_cnt,
			      SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_CLNT_V2_SUCC,
			      smc_tech->clnt_v2_succ_cnt,
			      SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_SRV_V1_SUCC,
			      smc_tech->srv_v1_succ_cnt,
			      SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_SRV_V2_SUCC,
			      smc_tech->srv_v2_succ_cnt,
			      SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_RX_BYTES,
			      smc_tech->rx_bytes,
			      SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_TX_BYTES,
			      smc_tech->tx_bytes,
			      SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_RX_RMB_USAGE,
			      smc_tech->rx_rmbuse, SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_TX_RMB_USAGE,
			      smc_tech->tx_rmbuse, SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_RX_CNT,
			      smc_tech->rx_cnt,
			      SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_TX_CNT,
			      smc_tech->tx_cnt,
			      SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_SENDPAGE_CNT,
			      0,
			      SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_CORK_CNT,
			      smc_tech->cork_cnt,
			      SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_NDLY_CNT,
			      smc_tech->ndly_cnt,
			      SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_SPLICE_CNT,
			      smc_tech->splice_cnt,
			      SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_T_URG_DATA_CNT,
			      smc_tech->urg_data_cnt,
			      SMC_NLA_STATS_PAD))
		goto errattr;

	nla_nest_end(skb, attrs);
	return 0;

errattr:
	nla_nest_cancel(skb, attrs);
errout:
	return -EMSGSIZE;
}

int smc_nl_get_stats(struct sk_buff *skb,
		     struct netlink_callback *cb)
{
	struct smc_nl_dmp_ctx *cb_ctx = smc_nl_dmp_ctx(cb);
	struct net *net = sock_net(skb->sk);
	struct smc_stats *stats;
	struct nlattr *attrs;
	int cpu, i, size;
	void *nlh;
	u64 *src;
	u64 *sum;

	if (cb_ctx->pos[0])
		goto errmsg;
	nlh = genlmsg_put(skb, NETLINK_CB(cb->skb).portid, cb->nlh->nlmsg_seq,
			  &smc_gen_nl_family, NLM_F_MULTI,
			  SMC_NETLINK_GET_STATS);
	if (!nlh)
		goto errmsg;

	attrs = nla_nest_start(skb, SMC_GEN_STATS);
	if (!attrs)
		goto errnest;
	stats = kzalloc(sizeof(*stats), GFP_KERNEL);
	if (!stats)
		goto erralloc;
	size = sizeof(*stats) / sizeof(u64);
	for_each_possible_cpu(cpu) {
		src = (u64 *)per_cpu_ptr(net->smc.smc_stats, cpu);
		sum = (u64 *)stats;
		for (i = 0; i < size; i++)
			*(sum++) += *(src++);
	}
	if (smc_nl_fill_stats_tech_data(skb, stats, SMC_TYPE_D))
		goto errattr;
	if (smc_nl_fill_stats_tech_data(skb, stats, SMC_TYPE_R))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_CLNT_HS_ERR_CNT,
			      stats->clnt_hshake_err_cnt,
			      SMC_NLA_STATS_PAD))
		goto errattr;
	if (nla_put_u64_64bit(skb, SMC_NLA_STATS_SRV_HS_ERR_CNT,
			      stats->srv_hshake_err_cnt,
			      SMC_NLA_STATS_PAD))
		goto errattr;

	nla_nest_end(skb, attrs);
	genlmsg_end(skb, nlh);
	cb_ctx->pos[0] = 1;
	kfree(stats);
	return skb->len;

errattr:
	kfree(stats);
erralloc:
	nla_nest_cancel(skb, attrs);
errnest:
	genlmsg_cancel(skb, nlh);
errmsg:
	return skb->len;
}

static int smc_nl_get_fback_details(struct sk_buff *skb,
				    struct netlink_callback *cb, int pos,
				    bool is_srv)
{
	struct smc_nl_dmp_ctx *cb_ctx = smc_nl_dmp_ctx(cb);
	struct net *net = sock_net(skb->sk);
	int cnt_reported = cb_ctx->pos[2];
	struct smc_stats_fback *trgt_arr;
	struct nlattr *attrs;
	int rc = 0;
	void *nlh;

	if (is_srv)
		trgt_arr = &net->smc.fback_rsn->srv[0];
	else
		trgt_arr = &net->smc.fback_rsn->clnt[0];
	if (!trgt_arr[pos].fback_code)
		return -ENODATA;
	nlh = genlmsg_put(skb, NETLINK_CB(cb->skb).portid, cb->nlh->nlmsg_seq,
			  &smc_gen_nl_family, NLM_F_MULTI,
			  SMC_NETLINK_GET_FBACK_STATS);
	if (!nlh)
		goto errmsg;
	attrs = nla_nest_start(skb, SMC_GEN_FBACK_STATS);
	if (!attrs)
		goto errout;
	if (nla_put_u8(skb, SMC_NLA_FBACK_STATS_TYPE, is_srv))
		goto errattr;
	if (!cnt_reported) {
		if (nla_put_u64_64bit(skb, SMC_NLA_FBACK_STATS_SRV_CNT,
				      net->smc.fback_rsn->srv_fback_cnt,
				      SMC_NLA_FBACK_STATS_PAD))
			goto errattr;
		if (nla_put_u64_64bit(skb, SMC_NLA_FBACK_STATS_CLNT_CNT,
				      net->smc.fback_rsn->clnt_fback_cnt,
				      SMC_NLA_FBACK_STATS_PAD))
			goto errattr;
		cnt_reported = 1;
	}

	if (nla_put_u32(skb, SMC_NLA_FBACK_STATS_RSN_CODE,
			trgt_arr[pos].fback_code))
		goto errattr;
	if (nla_put_u16(skb, SMC_NLA_FBACK_STATS_RSN_CNT,
			trgt_arr[pos].count))
		goto errattr;

	cb_ctx->pos[2] = cnt_reported;
	nla_nest_end(skb, attrs);
	genlmsg_end(skb, nlh);
	return rc;

errattr:
	nla_nest_cancel(skb, attrs);
errout:
	genlmsg_cancel(skb, nlh);
errmsg:
	return -EMSGSIZE;
}

int smc_nl_get_fback_stats(struct sk_buff *skb, struct netlink_callback *cb)
{
	struct smc_nl_dmp_ctx *cb_ctx = smc_nl_dmp_ctx(cb);
	struct net *net = sock_net(skb->sk);
	int rc_srv = 0, rc_clnt = 0, k;
	int skip_serv = cb_ctx->pos[1];
	int snum = cb_ctx->pos[0];
	bool is_srv = true;

	mutex_lock(&net->smc.mutex_fback_rsn);
	for (k = 0; k < SMC_MAX_FBACK_RSN_CNT; k++) {
		if (k < snum)
			continue;
		if (!skip_serv) {
			rc_srv = smc_nl_get_fback_details(skb, cb, k, is_srv);
			if (rc_srv && rc_srv != -ENODATA)
				break;
		} else {
			skip_serv = 0;
		}
		rc_clnt = smc_nl_get_fback_details(skb, cb, k, !is_srv);
		if (rc_clnt && rc_clnt != -ENODATA) {
			skip_serv = 1;
			break;
		}
		if (rc_clnt == -ENODATA && rc_srv == -ENODATA)
			break;
	}
	mutex_unlock(&net->smc.mutex_fback_rsn);
	cb_ctx->pos[1] = skip_serv;
	cb_ctx->pos[0] = k;
	return skb->len;
}

static struct net_device *smc_net_set_dump_ndev(struct net *net,
						struct net_device *ndev)
{
	struct net_device *orig_ndev;

	spin_lock(&net->smc.dump_ctx->dump_ndev_lock);
	orig_ndev = rcu_replace_pointer(net->smc.dump_ctx->dump_ndev,
					ndev, true);
	spin_unlock(&net->smc.dump_ctx->dump_ndev_lock);
	synchronize_rcu();
	/* no one references orig_ndev now */

	return orig_ndev;
}

int smc_nl_set_dump_ndev(struct sk_buff *skb, struct genl_info *info)
{
	struct nlattr *nla_dev = info->attrs[SMC_NLA_DUMP_DEV_NAME];
	char ndev_name[SMC_MAX_DUMP_DEV_LEN + 1] = { 0 };
	struct net_device *ndev, *orig_ndev;
	struct net *net = sock_net(skb->sk);

	if (!nla_dev ||
	    nla_len(nla_dev) > SMC_MAX_DUMP_DEV_LEN + 1)
		return -EINVAL;

	nla_strscpy(ndev_name, nla_dev, SMC_MAX_DUMP_DEV_LEN);
	/* put when reset dump ndev or smc_dump_exit() */
	ndev = dev_get_by_name(net, ndev_name);
	if (!ndev)
		return -ENODEV;

	orig_ndev = smc_net_set_dump_ndev(net, ndev);
	/* put the old dump ndev */
	if (orig_ndev)
		dev_put(orig_ndev);
	return 0;
}

int smc_nl_reset_dump_ndev(struct sk_buff *skb, struct genl_info *info)
{
	struct net *net = sock_net(skb->sk);
	struct net_device *ndev;

	ndev = smc_net_set_dump_ndev(net, NULL);
	if (!ndev)
		return -ENODEV;

	dev_put(ndev);
	return 0;
}

int smc_nl_get_dump_ndev(struct sk_buff *skb, struct netlink_callback *cb)
{
	struct smc_nl_dmp_ctx *cb_ctx = smc_nl_dmp_ctx(cb);
	char ndev_name[SMC_MAX_DUMP_DEV_LEN + 1] = { 0 };
	struct net *net = sock_net(skb->sk);
	struct net_device *ndev;
	void *hdr;

	if (cb_ctx->pos[0])
		return skb->len;

	hdr = genlmsg_put(skb, NETLINK_CB(cb->skb).portid, cb->nlh->nlmsg_seq,
			  &smc_gen_nl_family, NLM_F_MULTI,
			  SMC_NETLINK_GET_DUMP_DEV);
	if (!hdr)
		return -ENOMEM;

	rcu_read_lock();
	ndev = rcu_dereference(net->smc.dump_ctx->dump_ndev);
	if (!ndev)
		goto end;
	strscpy(ndev_name, ndev->name, SMC_MAX_DUMP_DEV_LEN);

	if (nla_put_string(skb, SMC_NLA_DUMP_DEV_NAME, ndev_name))
		goto err;

end:
	rcu_read_unlock();
	genlmsg_end(skb, hdr);
	cb_ctx->pos[0]++;
	return skb->len;
err:
	rcu_read_unlock();
	genlmsg_cancel(skb, hdr);
	return -EMSGSIZE;
}

static int smc_dump_netdev_event(struct notifier_block *this,
				 unsigned long event, void *ptr)
{
	struct net_device *event_dev = netdev_notifier_info_to_dev(ptr);
	struct net *net = dev_net(event_dev);
	struct smc_dump_ctx *dump_ctx;
	struct net_device *dump_ndev;

	dump_ctx = net->smc.dump_ctx;
	switch (event) {
	case NETDEV_REBOOT:
	case NETDEV_UNREGISTER:
		spin_lock(&dump_ctx->dump_ndev_lock);
		dump_ndev = rcu_dereference(dump_ctx->dump_ndev);
		if (!dump_ndev || dump_ndev != event_dev) {
			spin_unlock(&dump_ctx->dump_ndev_lock);
			return NOTIFY_DONE;
		}
		/* event occurred on dump_ndev */
		rcu_assign_pointer(dump_ctx->dump_ndev, NULL);
		spin_unlock(&dump_ctx->dump_ndev_lock);
		synchronize_rcu();
		dev_put(dump_ndev);
		return NOTIFY_OK;
	default:
		return NOTIFY_DONE;
	}
}

static struct notifier_block smc_dump_notifier = {
	.notifier_call = smc_dump_netdev_event
};

int smc_dump_init(struct net *net)
{
	int rc;

	net->smc.dump_ctx =
		kzalloc(sizeof(struct smc_dump_ctx), GFP_KERNEL);
	if (!net->smc.dump_ctx)
		return -ENOMEM;
	spin_lock_init(&net->smc.dump_ctx->dump_ndev_lock);
	rc = register_netdevice_notifier_net(net, &smc_dump_notifier);
	if (rc)
		goto out;
	return 0;
out:
	kfree(net->smc.dump_ctx);
	return rc;
}

void smc_dump_exit(struct net *net)
{
	unregister_netdevice_notifier_net(net, &smc_dump_notifier);
	kfree(net->smc.dump_ctx);
}

static int smc_dump_fill_skb_data(struct sk_buff *skb, void *data, int length)
{
	struct skb_shared_info *shinfo = skb_shinfo(skb);
	int i, left = length, len, off, sum = 0;
	struct page *page;
	char *p = data;

	/* too large to fit */
	if (left > SMC_DUMP_MAX_DATA_LEN)
		return -ENOBUFS;

	/* nolinear part: frags */
	for (i = 0; i < MAX_SKB_FRAGS; i++) {
		page = is_vmalloc_addr(p) ?
			vmalloc_to_page(p) : virt_to_page(p);
		off = i ? 0 : offset_in_page(p);
		len = min_t(size_t, PAGE_SIZE - off, left);

		shinfo->frags[i].bv_page = page;
		shinfo->frags[i].bv_offset = off;
		shinfo->frags[i].bv_len = len;

		/* unref in skb_release_data */
		__skb_frag_ref(&shinfo->frags[i]);

		p += len;
		sum += len;
		left -= len;
		if (!left)
			break;
	}
	shinfo->nr_frags = i + 1;
	skb->data_len += sum;
	skb->len += sum;
	skb->truesize += sum;
	return sum;
}

static int smc_dump_fill_skb_header(struct smc_sock *smc,
				    struct sk_buff *skb,
				    struct net_device *dev,
				    int data_len, int type, bool is_rx)
{
	struct smc_link_group *lgr;
	struct smc_dumphdr *smch;
	struct udphdr *udph;
	struct sock *clcsk;
	struct ethhdr *eh;
	struct iphdr *iph;

	/* too large to fit */
	if (data_len > SMC_DUMP_MAX_DATA_LEN)
		return -ENOBUFS;

	clcsk = smc->clcsock->sk;
	if (!clcsk)
		return -EINVAL;

	lgr = smc->conn.lgr;
	if (!lgr)
		return -EINVAL;
	smch = skb_push(skb, sizeof(struct smc_dumphdr));
	smch->magic = htonl(0xCFD3E7A5);
	smch->hdr_ver = SMC_DUMP_VER;
	smch->smc_ver = lgr->smc_version;
	smch->mode = lgr->is_smcd ? 2 : 1; /* SMC-R: 1, SMC-D: 2*/
	smch->type = type;
	smch->len = htons(sizeof(struct smc_dumphdr) + data_len);
	memset(smch->reserved, 0, sizeof(smch->reserved));

	udph = skb_push(skb, sizeof(struct udphdr));
	udph->source = is_rx ? clcsk->sk_dport : htons(clcsk->sk_num);
	udph->dest = is_rx ? htons(clcsk->sk_num) : clcsk->sk_dport;
	udph->len = htons(sizeof(struct udphdr) +
			  sizeof(struct smc_dumphdr) + data_len);
	udph->check = 0;
	skb_set_transport_header(skb, 0);

	/* only support IPv4 now */
	iph = skb_push(skb, sizeof(struct iphdr));
	iph->version = IPVERSION;
	iph->ihl = 5;	/* 20 bytes */
	iph->tos = 0;
	iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) +
			     sizeof(struct smc_dumphdr) + data_len);
	iph->id = 0;
	iph->frag_off = 0;
	iph->ttl = 64;
	iph->protocol = IPPROTO_UDP;
	iph->check = 0;
	iph->saddr = is_rx ? clcsk->sk_daddr : clcsk->sk_rcv_saddr;
	iph->daddr = is_rx ? clcsk->sk_rcv_saddr : clcsk->sk_daddr;
	skb_set_network_header(skb, 0);

	eh = skb_push(skb, sizeof(struct ethhdr));
	memcpy(eh->h_dest, dev->dev_addr, ETH_ALEN);
	memcpy(eh->h_source, dev->dev_addr, ETH_ALEN);
	eh->h_proto = htons(ETH_P_IP);
	return 0;
}

/* The caller must ensure that the buffer of @len starting
 * from @data is valid, e.g. no wrapping.
 * And the data @len is no larger than SMC_DUMP_MAX_DATA_LEN.
 */
static int __smc_dump_forward_data(struct smc_sock *smc,
				   struct net_device *dump_ndev, void *data,
				   int len, int type, bool is_rx)
{
	int header_size, data_size;
	struct sk_buff *skb;
	int rc, i;

	/* too large to fit */
	if (len > SMC_DUMP_MAX_DATA_LEN)
		return -ENOBUFS;

	/* pretend to be a UDP packet */
	header_size = sizeof(struct smc_dumphdr) + sizeof(struct udphdr) +
			sizeof(struct iphdr) + sizeof(struct ethhdr);
	skb = alloc_skb(header_size, GFP_ATOMIC);
	if (!skb) {
		rc = -ENOMEM;
		goto out;
	}
	skb_reserve(skb, header_size);

	/* assume total packet size is less than 65535 */
	data_size = smc_dump_fill_skb_data(skb, data, len);
	if (data_size < 0) {
		rc = data_size;
		goto out_skb;
	}
	rc = smc_dump_fill_skb_header(smc, skb, dump_ndev, data_size,
				      type, is_rx);
	if (rc)
		goto out_unref;

	skb->dev = dump_ndev;
	skb->protocol = htons(ETH_P_IP); /* only support IPv4 now */
	skb->ip_summed = CHECKSUM_UNNECESSARY;

	/* regardless of the return value, the skb is consumed. */
	rc = dev_queue_xmit(skb);
	if (rc != NET_XMIT_SUCCESS) {
		rc = -EPIPE;
		goto out;
	}

	return data_size;

out_unref:
	for (i = 0; i < skb_shinfo(skb)->nr_frags; i++)
		__skb_frag_unref(&(skb_shinfo(skb)->frags[i]), false);
out_skb:
	kfree(skb);
out:
	return rc;
}

int smc_dump_raw_data(struct smc_connection *conn, int offset,
		      int length, bool is_rx)
{
	struct smc_buf_desc *buf = is_rx ? conn->rmb_desc : conn->sndbuf_desc;
	struct smc_sock *smc = container_of(conn, struct smc_sock, conn);
	struct net *net = sock_net(&smc->sk);
	bool is_smcd = conn->lgr->is_smcd;
	int chunk, chunk_len, chunk_off;
	struct net_device *dump_ndev;
	int total_left, l, f, rc;
	char *p;

	rcu_read_lock();
	dump_ndev = rcu_dereference(net->smc.dump_ctx->dump_ndev);
	if (!dump_ndev) {
		rc = -ENODEV;
		goto out;
	}
	total_left = length;
	chunk_off = offset;

	/* skip the section at the front of SMC-D DMB that
	 * contains CDC messages.
	 */
	p = (is_smcd && is_rx) ?
		(char *)buf->cpu_addr + sizeof(struct smcd_cdc_msg) + offset :
		(char *)buf->cpu_addr + offset;
	for (chunk = 0; chunk < 2; chunk++) {
		chunk_len = min_t(int, total_left, buf->len - chunk_off);
		while (chunk_len) {
			/* split into maximum data size */
			l = min_t(int, chunk_len, SMC_DUMP_MAX_DATA_LEN);
			f = __smc_dump_forward_data(smc, dump_ndev, p, l,
						    SMC_DUMP_T_RAW_DATA, is_rx);
			if (f <= 0) {
				/* error */
				rc = f ? f : -EAGAIN;
				goto out;
			}
			p += f;
			chunk_len -= f;
			total_left -= f;
		}
		if (!total_left)
			break;	/* either on 1st or 2nd iteration */
		chunk_off = 0;
		p = (is_smcd && is_rx) ?
			(char *)buf->cpu_addr + sizeof(struct smcd_cdc_msg) :
			buf->cpu_addr;
	}
	rc = length - total_left;
out:
	rcu_read_unlock();
	return rc;
}

int smc_dump_cdc_msg(struct smc_connection *conn, void *buf,
		     int length, bool is_rx)
{
	struct smc_sock *smc = container_of(conn, struct smc_sock, conn);
	struct net *net = sock_net(&smc->sk);
	struct net_device *dump_ndev;
	int f, rc;

	rcu_read_lock();
	dump_ndev = rcu_dereference(net->smc.dump_ctx->dump_ndev);
	if (!dump_ndev) {
		rc = -ENODEV;
		goto out;
	}
	/* CDC msg size is mush smaller than
	 * SMC_DUMP_MAX_DATA_LEN, so send it directly.
	 */
	f = __smc_dump_forward_data(smc, dump_ndev, buf, length,
				    SMC_DUMP_T_CDC_MSG, is_rx);
	if (f <= 0) {
		rc = f ? f : -EAGAIN;
		goto out;
	}
	rc = f;
out:
	rcu_read_unlock();
	return rc;
}

int smc_dump_cdc_msg_rwwi(struct smc_connection *conn, u32 imm_data,
			  union smc_host_cursor *prod,
			  union smc_host_cursor *cons, bool is_rx)
{
	struct smc_sock *smc = container_of(conn, struct smc_sock, conn);
	struct net *net = sock_net(&smc->sk);
	struct smc_host_cdc_msg *local;
	struct net_device *dump_ndev;
	union smc_wr_imm_msg imm_msg;
	union smc_host_cursor save;
	struct smc_cdc_msg cdc;
	int f, rc;
	u32 token;

	rcu_read_lock();
	dump_ndev = rcu_dereference(net->smc.dump_ctx->dump_ndev);
	if (!dump_ndev) {
		rc = -ENODEV;
		goto out;
	}

	/* Unlike smc_dump_cdc_msg(), we need to construct a cdc message
	 * based on local_{rx|tx}_ctrl and imm_data, similar to what we
	 * do in smc_host_msg_to_cdc().
	 */
	memset(&cdc, 0, sizeof(cdc));
	imm_msg.imm_data = imm_data;
	local = is_rx ? &conn->local_rx_ctrl : &conn->local_tx_ctrl;
	cdc.common.type = local->common.type;
	cdc.len = local->len;
	/* in rwwi mode, seqno is not generated and imm_msg
	 * does not pass seqno as well.
	 */
	token = imm_msg.hdr.token;
	cdc.token = htonl(token);
	smc_host_cursor_to_cdc(&cdc.prod, prod, &save, conn);
	smc_host_cursor_to_cdc(&cdc.cons, cons, &save, conn);
	cdc.prod_flags = local->prod_flags;
	cdc.conn_state_flags = local->conn_state_flags;
	/* local_rx_ctrl doesn't have following information,
	 * we need to set on our own.
	 */
	cdc.common.type = SMC_CDC_MSG_TYPE;
	cdc.len = SMC_WR_TX_SIZE;

	switch (imm_msg.hdr.opcode) {
	case SMC_WR_OP_DATA:
	case SMC_WR_OP_CTRL:
	case SMC_WR_OP_DATA_WITH_FLAGS:
		/* nothing to do */
		break;
	case SMC_WR_OP_DATA_CR:
		cdc.credits = imm_msg.data_cr.credits;
		break;
	case SMC_WR_OP_DATA_WITH_FLAGS_CR:
		cdc.credits = imm_msg.data_with_flags_cr.credits;
		break;
	default:
		rc = -EINVAL;
		goto out;
	}

	/* CDC msg size is mush smaller than
	 * SMC_DUMP_MAX_DATA_LEN, so send it directly.
	 */
	f = __smc_dump_forward_data(smc, dump_ndev, &cdc,
				    sizeof(struct smc_cdc_msg),
				    SMC_DUMP_T_CDC_MSG, is_rx);
	if (f <= 0) {
		rc = f ? f : -EAGAIN;
		goto out;
	}
	rc = f;
out:
	rcu_read_unlock();
	return rc;
}
