/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Shared Memory Communications over RDMA (SMC-R) and RoCE
 *
 * Work Requests exploiting Infiniband API
 *
 * Copyright IBM Corp. 2016
 *
 * Author(s):  Steffen Maier <maier@linux.vnet.ibm.com>
 */

#ifndef SMC_WR_H
#define SMC_WR_H

#include <linux/atomic.h>
#include <rdma/ib_verbs.h>
#include <asm/div64.h>

#include "smc.h"
#include "smc_core.h"

#define SMC_WR_BUF_CNT 64	/* # of ctrl buffers per link, SMC_WR_BUF_CNT
				 * should not be less than 2 * SMC_RMBS_PER_LGR_MAX,
				 * since every connection at least has two rq/sq
				 * credits in average, otherwise may result in
				 * waiting for credits in sending process.
				 */

#define SMC_WR_TX_WAIT_FREE_SLOT_TIME	(10 * HZ)

#define SMC_WR_TX_SIZE 44 /* actual size of wr_send data (<=SMC_WR_BUF_SIZE) */

#define SMC_WR_TX_PEND_PRIV_SIZE 32

struct smc_wr_tx_pend_priv {
	u8			priv[SMC_WR_TX_PEND_PRIV_SIZE];
};

typedef void (*smc_wr_tx_handler)(struct smc_wr_tx_pend_priv *,
				  struct smc_link *,
				  enum ib_wc_status);

typedef bool (*smc_wr_tx_filter)(struct smc_wr_tx_pend_priv *,
				 unsigned long);

typedef void (*smc_wr_tx_dismisser)(struct smc_wr_tx_pend_priv *);

struct smc_wr_rx_handler {
	struct hlist_node	list;	/* hash table collision resolution */
	void			(*handler)(struct ib_wc *, void *);
	u8			type;
};

/* SMC_WR_OP_xx should not exceed 7, as op field is 3bits */
enum {
	SMC_WR_OP_DATA = 0,
	SMC_WR_OP_DATA_WITH_FLAGS,
	SMC_WR_OP_CTRL,
	SMC_WR_OP_RSVD,
	SMC_WR_OP_DATA_CR = 6,
	SMC_WR_OP_DATA_WITH_FLAGS_CR = 7
};

/* used to replace member 'data' in union smc_wr_imm_msg
 * when imm_data carries flags info
 */
struct smc_wr_imm_data_msg {
#if defined(__BIG_ENDIAN_BITFIELD)
		u32 token : 8;
		u32	opcode : 3;
		u32	diff_cons : 21;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
		u32	diff_cons : 21;
		u32	opcode : 3;
		u32	token : 8;
#endif
} __packed;

/* the value of SMC_PROD_FLAGS_MASK is related to
 * the definition of struct smc_wr_imm_data_with_flags_msg
 */
struct smc_wr_imm_data_with_flags_msg {
#if defined(__BIG_ENDIAN_BITFIELD)
		u32 token : 8;
		u32 opcode : 3;
		u32	write_blocked : 1;
		u32	urg_data_pending : 1;
		u32	urg_data_present : 1;
		u32 reserved : 1;
		u32 diff_cons : 17;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
		u32 diff_cons : 17;
		u32 reserved : 1;
		u32	urg_data_present : 1;
		u32	urg_data_pending : 1;
		u32	write_blocked : 1;
		u32 opcode : 3;
		u32 token : 8;
#endif
} __packed;

struct smc_wr_imm_ctrl_msg {
		struct smc_cdc_conn_state_flags	csflags;
		struct smc_cdc_producer_flags pflags;
#if defined(__BIG_ENDIAN_BITFIELD)
		u8 opcode : 3;
		u8 reserved : 5;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
		u8 reserved : 5;
		u8 opcode : 3;
#endif
		u8 token;
} __packed;

struct smc_wr_imm_data_cr_msg {
#if defined(__BIG_ENDIAN_BITFIELD)
		u32 token : 8;
		u32	opcode : 3;
		u32 credits : 8;
		u32	diff_cons : 13;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
		u32	diff_cons : 13;
		u32 credits : 8;
		u32	opcode : 3;
		u32	token : 8;
#endif
} __packed;

struct smc_wr_imm_data_with_flags_cr_msg {
#if defined(__BIG_ENDIAN_BITFIELD)
		u32 token : 8;
		u32 opcode : 3;
		u32	write_blocked : 1;
		u32	urg_data_pending : 1;
		u32	urg_data_present : 1;
		u32 credits : 8;
		u32 diff_cons : 10;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
		u32 diff_cons : 10;
		u32 credits : 8;
		u32	urg_data_present : 1;
		u32	urg_data_pending : 1;
		u32	write_blocked : 1;
		u32 opcode : 3;
		u32 token : 8;
#endif
} __packed;

struct smc_wr_imm_header {
#if defined(__BIG_ENDIAN_BITFIELD)
		u32 token : 8;
		u32 opcode : 3;
		u32 data : 21;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
		u32 data : 21;
		u32 opcode : 3;
		u32 token : 8;
#endif
} __packed;

/* the 11-bit (token and opcode) definition of struct
 * smc_wr_imm_xxx_msg should be consistent with
 * that of struct smc_wr_imm_header
 */
union smc_wr_imm_msg {
	u32 imm_data;
	struct smc_wr_imm_header hdr;
	struct smc_wr_imm_data_msg data;
	struct smc_wr_imm_data_with_flags_msg data_with_flags;
	struct smc_wr_imm_ctrl_msg ctrl;
	struct smc_wr_imm_data_cr_msg data_cr;
	struct smc_wr_imm_data_with_flags_cr_msg data_with_flags_cr;
};

union smc_wr_rwwi_tx_id {
	u64 data;
	struct {
#if defined(__BIG_ENDIAN_BITFIELD)
		u64	rwwi_flag : 1;
		u64	reserved : 7;
		u64	token : 8;
		u64	inflight_cons : 24;
		u64	inflight_sent : 24;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
		u64	inflight_sent : 24;
		u64	inflight_cons : 24;
		u64	token : 8;
		u64	reserved : 7;
		u64	rwwi_flag : 1;
#endif
	};
} __packed;

/* diff_cons only holds 17 bits in DATA_WITH_FLAGS imm_data,
 * and holds 21 bits in DATA imm_data,
 * so diff_cons value is limited in one WRITE_WITH_IMM
 * it is related to the definition of union smc_wr_imm_msg
 */
#define SMC_DATA_MAX_DIFF_CONS				((1 << 21) - 1)
#define SMC_DATA_WITH_FLAGS_MAX_DIFF_CONS	((1 << 17) - 1)
#define SMC_DATA_CR_MAX_DIFF_CONS				((1 << 13) - 1)
#define SMC_DATA_WITH_FLAGS_CR_MAX_DIFF_CONS	((1 << 10) - 1)
#define SMC_PROD_FLAGS_MASK				0xE0
#define SMC_WR_ID_SEQ_MASK				((uint64_t)0x7FFFFFFFFFFFFFFFULL)
#define SMC_WR_ID_FLAG_RWWI				(((uint64_t)1) << 63)
#define SMC_WR_IS_TX_RWWI(wr_id)			((wr_id) & SMC_WR_ID_FLAG_RWWI)

/* Only used by RDMA write WRs.
 * All other WRs (CDC/LLC) use smc_wr_tx_send handling WR_ID implicitly
 */
static inline long smc_wr_tx_get_next_wr_id(struct smc_link *link)
{
	return atomic_long_add_return(2, &link->wr_tx_id) & SMC_WR_ID_SEQ_MASK;
}

static inline void smc_wr_tx_set_wr_id(atomic_long_t *wr_tx_id, long val)
{
	atomic_long_set(wr_tx_id, val);
}

static inline bool smc_wr_tx_link_hold(struct smc_link *link)
{
	if (!smc_link_sendable(link))
		return false;
	percpu_ref_get(&link->wr_tx_refs);
	return true;
}

static inline void smc_wr_tx_link_put(struct smc_link *link)
{
	percpu_ref_put(&link->wr_tx_refs);
}

static inline void smc_wr_drain_cq(struct smc_link *lnk)
{
	wait_event(lnk->wr_rx_empty_wait, lnk->wr_rx_id_compl == lnk->wr_rx_id);
}

static inline void smc_wr_wakeup_tx_wait(struct smc_link *lnk)
{
	wake_up_all(&lnk->wr_tx_wait);
}

static inline void smc_wr_wakeup_reg_wait(struct smc_link *lnk)
{
	wake_up(&lnk->wr_reg_wait);
}

/* get one tx credit, and peer rq credits dec */
static inline int smc_wr_tx_get_credit(struct smc_link *link)
{
	return !link->credits_enable ||
	       atomic_dec_if_positive(&link->peer_rq_credits) >= 0;
}

/* put tx credits, when some failures occurred after tx credits got
 * or receive announce credits msgs
 */
static inline void smc_wr_tx_put_credits(struct smc_link *link, int credits,
					 bool wakeup)
{
	if (link->credits_enable && credits) {
		atomic_add(credits, &link->peer_rq_credits);
		if (wakeup && wq_has_sleeper(&link->wr_tx_wait))
			wake_up_nr(&link->wr_tx_wait, credits);
	}
}

/* to check whether peer rq credits is lower than watermark. */
static inline int smc_wr_tx_credits_need_announce(struct smc_link *link)
{
	return link->credits_enable && atomic_read(&link->peer_rq_credits) <=
					       link->peer_cr_watermark_low;
}

/* get local rq credits and set credits to zero.
 * may called when announcing credits.
 */
static inline int smc_wr_rx_get_credits(struct smc_link *link)
{
	return link->credits_enable ?
		       atomic_fetch_and(0, &link->local_rq_credits) : 0;
}

/* called when post_recv a rqe */
static inline void smc_wr_rx_put_credits(struct smc_link *link, int credits)
{
	if (link->credits_enable && credits)
		atomic_add(credits, &link->local_rq_credits);
}

/* to check whether local rq credits is higher than watermark. */
static inline int smc_wr_rx_credits_need_announce(struct smc_link *link)
{
	return link->credits_enable && atomic_read(&link->local_rq_credits) >=
					       link->local_cr_watermark_high;
}

static inline int smc_wr_rx_credits_need_announce_frequent(struct smc_link *link)
{
	/* announce when local rq credits accumulated more than credits_update_limit, or
	 * peer rq credits is empty. As peer credits empty and local credits is less than
	 * credits_update_limit, may results in credits deadlock.
	 */
	return link->credits_enable && (atomic_read(&link->local_rq_credits) >=
						link->credits_update_limit ||
					!atomic_read(&link->peer_rq_credits));
}

/* post a new receive work request to fill a completed old work request entry */
static inline int smc_wr_rx_post(struct smc_link *link)
{
	int rc;
	u64 wr_id, temp_wr_id;
	u32 index;

	link->wr_rx_id += 2;
	wr_id = link->wr_rx_id & SMC_WR_ID_SEQ_MASK; /* tasklet context, thus not atomic */
	temp_wr_id = wr_id / 2;
	index = do_div(temp_wr_id, link->wr_rx_cnt);
	link->wr_rx_ibs[index].wr_id = wr_id;
	rc = ib_post_recv(link->roce_qp, &link->wr_rx_ibs[index], NULL);
	if (!rc)
		smc_wr_rx_put_credits(link, 1);
	return rc;
}

static inline bool smc_wr_id_is_rx(u64 wr_id)
{
	return wr_id % 2;
}

int smc_wr_create_link(struct smc_link *lnk);
int smc_wr_alloc_link_mem(struct smc_link *lnk);
int smc_wr_alloc_lgr_mem(struct smc_link_group *lgr);
void smc_wr_free_link(struct smc_link *lnk);
void smc_wr_free_link_mem(struct smc_link *lnk);
void smc_wr_free_lgr_mem(struct smc_link_group *lgr);
void smc_wr_remember_qp_attr(struct smc_link *lnk);
void smc_wr_remove_dev(struct smc_ib_device *smcibdev);
void smc_wr_add_dev(struct smc_ib_device *smcibdev);

int smc_wr_tx_get_free_slot(struct smc_link *link, smc_wr_tx_handler handler,
			    struct smc_wr_buf **wr_buf,
			    struct smc_rdma_wr **wrs,
			    struct smc_wr_tx_pend_priv **wr_pend_priv);
int smc_wr_tx_get_v2_slot(struct smc_link *link,
			  smc_wr_tx_handler handler,
			  struct smc_wr_v2_buf **wr_buf,
			  struct smc_wr_tx_pend_priv **wr_pend_priv);
int smc_wr_tx_put_slot(struct smc_link *link,
		       struct smc_wr_tx_pend_priv *wr_pend_priv);
int smc_wr_tx_send(struct smc_link *link,
		   struct smc_wr_tx_pend_priv *wr_pend_priv);
int smc_wr_tx_v2_send(struct smc_link *link,
		      struct smc_wr_tx_pend_priv *priv, int len);
int smc_wr_tx_send_wait(struct smc_link *link, struct smc_wr_tx_pend_priv *priv,
			unsigned long timeout);
void smc_wr_cq_handler(struct ib_cq *ib_cq, void *cq_context);
void smc_wr_tx_wait_no_pending_sends(struct smc_link *link);

int smc_wr_rx_register_handler(struct smc_wr_rx_handler *handler);
int smc_wr_rx_post_init(struct smc_link *link);
int smc_wr_reg_send(struct smc_link *link, struct ib_mr *mr);

#endif /* SMC_WR_H */
