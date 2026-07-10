/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */

/* Authors: Cheng Xu <chengyou@linux.alibaba.com> */
/*          Kai Shen <kaishen@linux.alibaba.com> */
/* Copyright (c) 2020-2022, Alibaba Group. */

#ifndef __ERDMA_SW_H__
#define __ERDMA_SW_H__

#include "kcompat.h"
#include "erdma_verbs.h"

int erdma_compat_init(void);
void erdma_compat_exit(void);

void erdma_gen_port_from_qpn(u32 sip, u32 dip, u32 lqpn, u32 rqpn, u16 *sport,
			     u16 *dport);

int erdma_handle_compat_attr(struct erdma_qp *qp, struct ib_qp_attr *attr,
			     int attr_mask);

int erdma_add_gid(const struct ib_gid_attr *attr, void **context);

int erdma_del_gid(const struct ib_gid_attr *attr, void **context);

int erdma_create_ah(struct ib_ah *ibah,
		    struct rdma_ah_init_attr *init_attr,
		    struct ib_udata *udata);

int erdma_destroy_ah(struct ib_ah *ibah, u32 flags);

#include "compat/sw_verbs.h"
#include "compat/sw_net.h"

int erdma_modify_mad_qp(struct ib_qp *ibqp, struct ib_qp_attr *attr,
			int attr_mask, struct ib_udata *udata);

int erdma_create_mad_qp(struct ib_qp *ibqp, struct ib_qp_init_attr *attrs,
			struct ib_udata *udata);
int erdma_post_send_mad(struct ib_qp *ibqp, const struct ib_send_wr *send_wr,
			const struct ib_send_wr **bad_send_wr);
int erdma_post_recv_mad(struct ib_qp *ibqp, const struct ib_recv_wr *recv_wr,
			const struct ib_recv_wr **bad_recv_wr);

int erdma_create_qp_mad(struct ib_qp *ibqp, struct ib_qp_init_attr *attrs,
			struct ib_udata *udata);
int attach_sw_dev(struct erdma_dev *dev);
void detach_sw_dev(struct erdma_dev *dev);
int erdma_mad_poll_cq(struct ib_cq *ibcq, int num_entries, struct ib_wc *wc);
int erdma_mad_req_notify_cq(struct ib_cq *ibcq, enum ib_cq_notify_flags flags);
void erdma_destroy_mad_qp(struct ib_qp *ibqp);
void detach_sw_pd(struct erdma_pd *pd);
void detach_sw_cq(struct erdma_cq *cq);
#endif
