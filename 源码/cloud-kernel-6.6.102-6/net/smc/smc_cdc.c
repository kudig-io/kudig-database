// SPDX-License-Identifier: GPL-2.0
/*
 * Shared Memory Communications over RDMA (SMC-R) and RoCE
 *
 * Connection Data Control (CDC)
 * handles flow control
 *
 * Copyright IBM Corp. 2016
 *
 * Author(s):  Ursula Braun <ubraun@linux.vnet.ibm.com>
 */

#include <linux/spinlock.h>

#include "smc.h"
#include "smc_wr.h"
#include "smc_cdc.h"
#include "smc_tx.h"
#include "smc_rx.h"
#include "smc_close.h"
#include "smc_ism.h"
#include "smc_stats.h"

/********************************** send *************************************/

/* handler for send/transmission completion of a CDC msg */
static void smc_cdc_tx_handler(struct smc_wr_tx_pend_priv *pnd_snd,
			       struct smc_link *link,
			       enum ib_wc_status wc_status)
{
	struct smc_cdc_tx_pend *cdcpend = (struct smc_cdc_tx_pend *)pnd_snd;
	struct smc_connection *conn = cdcpend->conn;
	struct smc_buf_desc *sndbuf_desc;
	struct smc_sock *smc;
	int diff;

	if (unlikely(link->lgr->use_rwwi)) {
		pr_err_once("smc: unexpected cdc msg tx work completion when rwwi enabled.\n");
		return;
	}

	sndbuf_desc = conn->sndbuf_desc;
	smc = container_of(conn, struct smc_sock, conn);
	bh_lock_sock(&smc->sk);
	if (!wc_status && sndbuf_desc) {
		diff = smc_curs_diff(sndbuf_desc->len,
				     &cdcpend->conn->tx_curs_fin,
				     &cdcpend->cursor);
		/* sndbuf_space is decreased in smc_sendmsg */
		smp_mb__before_atomic();
		atomic_add(diff, &cdcpend->conn->sndbuf_space);
		/* guarantee 0 <= sndbuf_space <= sndbuf_desc->len */
		smp_mb__after_atomic();
		smc_curs_copy(&conn->tx_curs_fin, &cdcpend->cursor, conn);
		smc_curs_copy(&conn->local_tx_ctrl_fin, &cdcpend->p_cursor,
			      conn);
		conn->tx_cdc_seq_fin = cdcpend->ctrl_seq;
	}

	if (atomic_dec_and_test(&conn->cdc_pend_tx_wr)) {
		/* If user owns the sock_lock, mark the connection need sending.
		 * User context will later try to send when it release sock_lock
		 * in smc_release_cb()
		 */
		if (sock_owned_by_user(&smc->sk))
			conn->tx_in_release_sock = true;
		else
			smc_tx_pending(conn);

		if (unlikely(wq_has_sleeper(&conn->cdc_pend_tx_wq)))
			wake_up(&conn->cdc_pend_tx_wq);
	}
	WARN_ON(atomic_read(&conn->cdc_pend_tx_wr) < 0);

	smc_tx_sndbuf_nonfull(smc);
	bh_unlock_sock(&smc->sk);
}

void smc_cdc_tx_handler_rwwi(struct ib_wc *wc)
{
	struct smc_link *link = wc->qp->qp_context;
	struct smc_link_group *lgr = link->lgr;
	struct smc_connection *conn = NULL;
	union smc_wr_rwwi_tx_id wr_id;
	struct smc_sock *smc = NULL;
	int diff;

	if (unlikely(!lgr->use_rwwi)) {
		pr_err_once("smc: unexpected rwwi msg tx work completion when rwwi disabled.\n");
		return;
	}

	wr_id.data = wc->wr_id;

	read_lock_bh(&lgr->conns_lock);
	smc = smc_lgr_get_sock(wr_id.token, lgr);
	read_unlock_bh(&lgr->conns_lock);
	if (!smc)
		return;

	conn = &smc->conn;
	bh_lock_sock(&smc->sk);

	if (!wc->status) {
		diff = wr_id.inflight_sent;
		/* sndbuf_space is decreased in smc_sendmsg */
		smp_mb__before_atomic();
		atomic_add(diff, &conn->sndbuf_space);
		/* guarantee 0 <= sndbuf_space <= sndbuf_desc->len */
		smp_mb__after_atomic();

		smc_curs_add(conn->sndbuf_desc->len, &conn->tx_curs_fin, diff);
		smc_curs_add(conn->sndbuf_desc->len, &conn->local_tx_ctrl_fin, diff);
	}

	if (atomic_dec_and_test(&conn->cdc_pend_tx_wr)) {
		if (sock_owned_by_user(&smc->sk))
			conn->tx_in_release_sock = true;
		else
			smc_tx_pending(conn);

		if (unlikely(wq_has_sleeper(&conn->cdc_pend_tx_wq)))
			wake_up(&conn->cdc_pend_tx_wq);
	}

	WARN_ON(atomic_read(&conn->cdc_pend_tx_wr) < 0);

	smc_tx_sndbuf_nonfull(smc);
	bh_unlock_sock(&smc->sk);
	sock_put(&smc->sk); /* sock_hold in smc_lgr_get_sock */
}

int smc_cdc_get_free_slot(struct smc_connection *conn,
			  struct smc_link *link,
			  struct smc_wr_buf **wr_buf,
			  struct smc_rdma_wr **wr_rdma_buf,
			  struct smc_cdc_tx_pend **pend)
{
	int rc;

	rc = smc_wr_tx_get_free_slot(link, smc_cdc_tx_handler, wr_buf,
				     wr_rdma_buf,
				     (struct smc_wr_tx_pend_priv **)pend);
	if (conn->killed) {
		/* abnormal termination */
		if (!rc)
			smc_wr_tx_put_slot(link,
					   (struct smc_wr_tx_pend_priv *)(*pend));
		rc = -EPIPE;
	}
	return rc;
}

static inline void smc_cdc_add_pending_send(struct smc_connection *conn,
					    struct smc_cdc_tx_pend *pend)
{
	BUILD_BUG_ON_MSG(
		sizeof(struct smc_cdc_msg) > SMC_WR_BUF_SIZE,
		"must increase SMC_WR_BUF_SIZE to at least sizeof(struct smc_cdc_msg)");
	BUILD_BUG_ON_MSG(
		offsetofend(struct smc_cdc_msg, reserved) > SMC_WR_TX_SIZE,
		"must adapt SMC_WR_TX_SIZE to sizeof(struct smc_cdc_msg); if not all smc_wr upper layer protocols use the same message size any more, must start to set link->wr_tx_sges[i].length on each individual smc_wr_tx_send()");
	BUILD_BUG_ON_MSG(
		sizeof(struct smc_cdc_tx_pend) > SMC_WR_TX_PEND_PRIV_SIZE,
		"must increase SMC_WR_TX_PEND_PRIV_SIZE to at least sizeof(struct smc_cdc_tx_pend)");
	pend->conn = conn;
	pend->cursor = conn->tx_curs_sent;
	pend->p_cursor = conn->local_tx_ctrl.prod;
	pend->ctrl_seq = conn->tx_cdc_seq;
}

int smc_cdc_msg_send(struct smc_connection *conn,
		     struct smc_wr_buf *wr_buf,
		     struct smc_cdc_tx_pend *pend)
{
	struct smc_link *link = conn->lnk;
	struct smc_cdc_msg *cdc_msg = (struct smc_cdc_msg *)wr_buf;
	union smc_host_cursor cfed;
	u8 saved_credits = 0;
	int rc;

	if (unlikely(link->lgr->use_rwwi)) {
		pr_err_once("smc: send unexpected cdc msg when rwwi enabled.\n");
		return -EINVAL;
	}

	smc_cdc_add_pending_send(conn, pend);

	conn->tx_cdc_seq++;
	conn->local_tx_ctrl.seqno = conn->tx_cdc_seq;
	smc_host_msg_to_cdc(cdc_msg, conn, &cfed);
	if (smc_wr_rx_credits_need_announce_frequent(link))
		saved_credits = (u8)smc_wr_rx_get_credits(link);
	cdc_msg->credits = saved_credits;

	atomic_inc(&conn->cdc_pend_tx_wr);
	smp_mb__after_atomic(); /* Make sure cdc_pend_tx_wr added before post */

	smc_dump_cdc_msg(conn, cdc_msg, sizeof(struct smc_cdc_msg), false);
	rc = smc_wr_tx_send(link, (struct smc_wr_tx_pend_priv *)pend);
	if (likely(!rc)) {
		smc_curs_copy(&conn->rx_curs_confirmed, &cfed, conn);
		conn->local_rx_ctrl.prod_flags.cons_curs_upd_req = 0;
	} else {
		conn->tx_cdc_seq--;
		conn->local_tx_ctrl.seqno = conn->tx_cdc_seq;
		smc_wr_rx_put_credits(link, saved_credits);
		atomic_dec(&conn->cdc_pend_tx_wr);
	}

	return rc;
}

/* send a validation msg indicating the move of a conn to an other QP link */
int smcr_cdc_msg_send_validation(struct smc_connection *conn,
				 struct smc_cdc_tx_pend *pend,
				 struct smc_wr_buf *wr_buf)
{
	struct smc_host_cdc_msg *local = &conn->local_tx_ctrl;
	struct smc_link *link = conn->lnk;
	struct smc_cdc_msg *peer;
	int rc;

	peer = (struct smc_cdc_msg *)wr_buf;
	peer->common.type = local->common.type;
	peer->len = local->len;
	peer->seqno = htons(conn->tx_cdc_seq_fin); /* seqno last compl. tx */
	peer->token = htonl(local->token);
	peer->prod_flags.failover_validation = 1;

	/* We need to set pend->conn here to make sure smc_cdc_tx_handler()
	 * can handle properly
	 */
	smc_cdc_add_pending_send(conn, pend);

	atomic_inc(&conn->cdc_pend_tx_wr);
	smp_mb__after_atomic(); /* Make sure cdc_pend_tx_wr added before post */

	rc = smc_wr_tx_send(link, (struct smc_wr_tx_pend_priv *)pend);
	if (unlikely(rc))
		atomic_dec(&conn->cdc_pend_tx_wr);

	return rc;
}

static int smcr_cdc_get_slot_and_msg_send(struct smc_connection *conn)
{
	struct smc_cdc_tx_pend *pend;
	struct smc_wr_buf *wr_buf;
	struct smc_link *link;
	bool again = false;
	int rc;

again:
	link = conn->lnk;
	if (!smc_wr_tx_link_hold(link))
		return -ENOLINK;
	rc = smc_cdc_get_free_slot(conn, link, &wr_buf, NULL, &pend);
	if (rc)
		goto put_out;

	spin_lock_bh(&conn->send_lock);
	if (link != conn->lnk) {
		/* link of connection changed, try again one time*/
		spin_unlock_bh(&conn->send_lock);
		smc_wr_tx_put_slot(link,
				   (struct smc_wr_tx_pend_priv *)pend);
		smc_wr_tx_link_put(link);
		if (again)
			return -ENOLINK;
		again = true;
		goto again;
	}
	rc = smc_cdc_msg_send(conn, wr_buf, pend);
	spin_unlock_bh(&conn->send_lock);
put_out:
	smc_wr_tx_link_put(link);
	return rc;
}

int smc_cdc_get_slot_and_msg_send(struct smc_connection *conn)
{
	int rc;

	if (!smc_conn_lgr_valid(conn) ||
	    (conn->lgr->is_smcd && conn->lgr->peer_shutdown))
		return -EPIPE;

	if (conn->lgr->is_smcd) {
		spin_lock_bh(&conn->send_lock);
		rc = smcd_cdc_msg_send(conn);
		spin_unlock_bh(&conn->send_lock);
	} else {
		rc = smcr_cdc_get_slot_and_msg_send(conn);
	}

	return rc;
}

void smc_cdc_wait_pend_tx_wr(struct smc_connection *conn)
{
	wait_event(conn->cdc_pend_tx_wq, !atomic_read(&conn->cdc_pend_tx_wr));
}

/* Send a SMC-D CDC header.
 * This increments the free space available in our send buffer.
 * Also update the confirmed receive buffer with what was sent to the peer.
 */
int smcd_cdc_msg_send(struct smc_connection *conn)
{
	struct smc_sock *smc = container_of(conn, struct smc_sock, conn);
	union smc_host_cursor curs;
	struct smcd_cdc_msg cdc;
	int rc, diff;

	memset(&cdc, 0, sizeof(cdc));
	cdc.common.type = SMC_CDC_MSG_TYPE;
	curs.acurs.counter = atomic64_read(&conn->local_tx_ctrl.prod.acurs);
	cdc.prod.wrap = curs.wrap;
	cdc.prod.count = curs.count;
	curs.acurs.counter = atomic64_read(&conn->local_tx_ctrl.cons.acurs);
	cdc.cons.wrap = curs.wrap;
	cdc.cons.count = curs.count;
	cdc.cons.prod_flags = conn->local_tx_ctrl.prod_flags;
	cdc.cons.conn_state_flags = conn->local_tx_ctrl.conn_state_flags;
	smc_dump_cdc_msg(conn, &cdc, sizeof(struct smcd_cdc_msg), false);
	rc = smcd_tx_ism_write(conn, &cdc, sizeof(cdc), 0, 1);
	if (rc)
		return rc;
	smc_curs_copy(&conn->rx_curs_confirmed, &curs, conn);
	conn->local_rx_ctrl.prod_flags.cons_curs_upd_req = 0;

	if (smc_ism_support_dmb_nocopy(conn->lgr->smcd))
		/* if local sndbuf shares the same memory region with
		 * peer DMB, then don't update the tx_curs_fin
		 * and sndbuf_space until peer has consumed the data.
		 */
		return 0;

	/* Calculate transmitted data and increment free send buffer space */
	diff = smc_curs_diff(conn->sndbuf_desc->len, &conn->tx_curs_fin,
			     &conn->tx_curs_sent);
	/* increased by confirmed number of bytes */
	smp_mb__before_atomic();
	atomic_add(diff, &conn->sndbuf_space);
	/* guarantee 0 <= sndbuf_space <= sndbuf_desc->len */
	smp_mb__after_atomic();
	smc_curs_copy(&conn->tx_curs_fin, &conn->tx_curs_sent, conn);

	smc_tx_sndbuf_nonfull(smc);
	return 0;
}

/********************************* receive ***********************************/

static inline bool smc_cdc_before(u16 seq1, u16 seq2)
{
	return (s16)(seq1 - seq2) < 0;
}

static void smc_cdc_handle_urg_data_arrival(struct smc_sock *smc,
					    int *diff_prod)
{
	struct smc_connection *conn = &smc->conn;
	char *base;

	/* new data included urgent business */
	smc_curs_copy(&conn->urg_curs, &conn->local_rx_ctrl.prod, conn);
	conn->urg_state = SMC_URG_VALID;
	if (!sock_flag(&smc->sk, SOCK_URGINLINE))
		/* we'll skip the urgent byte, so don't account for it */
		(*diff_prod)--;
	base = (char *)conn->rmb_desc->cpu_addr + conn->rx_off;
	if (conn->urg_curs.count)
		conn->urg_rx_byte = *(base + conn->urg_curs.count - 1);
	else
		conn->urg_rx_byte = *(base + conn->rmb_desc->len - 1);
	sk_send_sigurg(&smc->sk);
}

static void smc_cdc_msg_validate(struct smc_sock *smc, struct smc_cdc_msg *cdc,
				 struct smc_link *link)
{
	struct smc_connection *conn = &smc->conn;
	u16 recv_seq = ntohs(cdc->seqno);
	s16 diff;

	/* check that seqnum was seen before */
	diff = conn->local_rx_ctrl.seqno - recv_seq;
	if (diff < 0) { /* diff larger than 0x7fff */
		/* drop connection */
		conn->out_of_sync = 1;	/* prevent any further receives */
		spin_lock_bh(&conn->send_lock);
		conn->local_tx_ctrl.conn_state_flags.peer_conn_abort = 1;
		conn->lnk = link;
		spin_unlock_bh(&conn->send_lock);
		sock_hold(&smc->sk); /* sock_put in abort_work */
		if (!queue_work(smc_close_wq, &conn->abort_work))
			sock_put(&smc->sk);
	}
}

static void __smc_cdc_msg_recv_action(struct smc_sock *smc,
				      int diff_prod, int diff_cons)
{
	struct smc_connection *conn = &smc->conn;
	int diff_tx;

	if (diff_cons) {
		/* peer_rmbe_space is decreased during data transfer with RDMA
		 * write
		 */
		smp_mb__before_atomic();
		atomic_add(diff_cons, &conn->peer_rmbe_space);
		/* guarantee 0 <= peer_rmbe_space <= peer_rmbe_size */
		smp_mb__after_atomic();

		/* if local sndbuf shares the same memory region with
		 * peer RMB, then update tx_curs_fin and sndbuf_space
		 * here since peer has already consumed the data.
		 */
		if (conn->lgr->is_smcd &&
		    smc_ism_support_dmb_nocopy(conn->lgr->smcd)) {
			/* Calculate consumed data and
			 * increment free send buffer space.
			 */
			diff_tx = smc_curs_diff(conn->sndbuf_desc->len,
						&conn->tx_curs_fin,
						&conn->local_rx_ctrl.cons);
			/* increase local sndbuf space and fin_curs */
			smp_mb__before_atomic();
			atomic_add(diff_tx, &conn->sndbuf_space);
			/* guarantee 0 <= sndbuf_space <= sndbuf_desc->len */
			smp_mb__after_atomic();
			smc_curs_copy(&conn->tx_curs_fin,
				      &conn->local_rx_ctrl.cons, conn);

			smc_tx_sndbuf_nonfull(smc);
		}
	}
	if (diff_prod) {
		if (conn->local_rx_ctrl.prod_flags.urg_data_present)
			smc_cdc_handle_urg_data_arrival(smc, &diff_prod);
		/* bytes_to_rcv is decreased in smc_recvmsg */
		smp_mb__before_atomic();
		atomic_add(diff_prod, &conn->bytes_to_rcv);
		/* guarantee 0 <= bytes_to_rcv <= rmb_desc->len */
		smp_mb__after_atomic();
		smc->sk.sk_data_ready(&smc->sk);
	} else {
		if (conn->local_rx_ctrl.prod_flags.write_blocked)
			smc->sk.sk_data_ready(&smc->sk);
		if (conn->local_rx_ctrl.prod_flags.urg_data_pending)
			conn->urg_state = SMC_URG_NOTYET;
	}

	/* trigger sndbuf consumer: RDMA write into peer RMBE and CDC */
	if ((diff_cons && smc_tx_prepared_sends(conn) &&
	     conn->local_tx_ctrl.prod_flags.write_blocked) ||
	    conn->local_rx_ctrl.prod_flags.cons_curs_upd_req ||
	    conn->local_rx_ctrl.prod_flags.urg_data_pending) {
		if (!sock_owned_by_user(&smc->sk))
			smc_tx_pending(conn);
		else
			conn->tx_in_release_sock = true;
	}

	if (diff_cons && conn->urg_tx_pend &&
	    atomic_read(&conn->peer_rmbe_space) == conn->peer_rmbe_size) {
		/* urg data confirmed by peer, indicate we're ready for more */
		conn->urg_tx_pend = false;
		smc->sk.sk_write_space(&smc->sk);
	}

	if (conn->local_rx_ctrl.conn_state_flags.peer_conn_abort)
		smc->sk.sk_err = ECONNRESET;

	if (smc_cdc_rxed_any_close_or_senddone(conn)) {
		smc->sk.sk_shutdown |= RCV_SHUTDOWN;
		if (smc->clcsock && smc->clcsock->sk)
			smc->clcsock->sk->sk_shutdown |= RCV_SHUTDOWN;
		smc_sock_set_flag(&smc->sk, SOCK_DONE);
		sock_hold(&smc->sk); /* sock_put in close_work */
		if (!queue_work(smc_close_wq, &conn->close_work))
			sock_put(&smc->sk);
	}
}

static void smc_cdc_msg_recv_action(struct smc_sock *smc,
				    struct smc_cdc_msg *cdc)
{
	union smc_host_cursor cons_old, prod_old;
	struct smc_connection *conn = &smc->conn;
	int diff_cons, diff_prod;

	smc_curs_copy(&prod_old, &conn->local_rx_ctrl.prod, conn);
	smc_curs_copy(&cons_old, &conn->local_rx_ctrl.cons, conn);
	smc_cdc_msg_to_host(&conn->local_rx_ctrl, cdc, conn);

	diff_cons = smc_curs_diff(conn->peer_rmbe_size, &cons_old,
				  &conn->local_rx_ctrl.cons);
	diff_prod = smc_curs_diff(conn->rmb_desc->len, &prod_old,
				  &conn->local_rx_ctrl.prod);
	if (diff_prod)
		smc_dump_raw_data(conn, prod_old.count, diff_prod, true);
	if (conn->lgr->is_smcd)
		smc_dump_cdc_msg(conn, cdc, sizeof(struct smcd_cdc_msg), true);
	else
		smc_dump_cdc_msg(conn, cdc, sizeof(struct smc_cdc_msg), true);
	__smc_cdc_msg_recv_action(smc, diff_prod, diff_cons);
}

/* called under tasklet context */
static void smc_cdc_msg_recv(struct smc_sock *smc, struct smc_cdc_msg *cdc)
{
	sock_hold(&smc->sk);
	bh_lock_sock(&smc->sk);
	smc_cdc_msg_recv_action(smc, cdc);
	bh_unlock_sock(&smc->sk);
	sock_put(&smc->sk); /* no free sk in softirq-context */
}

/* Schedule a tasklet for this connection. Triggered from the ISM device IRQ
 * handler to indicate update in the DMBE.
 *
 * Context:
 * - tasklet context
 */
static void smcd_cdc_rx_tsklet(struct tasklet_struct *t)
{
	struct smc_connection *conn = from_tasklet(conn, t, rx_tsklet);
	struct smcd_cdc_msg *data_cdc;
	struct smcd_cdc_msg cdc;
	struct smc_sock *smc;

	if (!conn || conn->killed)
		return;

	data_cdc = (struct smcd_cdc_msg *)conn->rmb_desc->cpu_addr;
	smcd_curs_copy(&cdc.prod, &data_cdc->prod, conn);
	smcd_curs_copy(&cdc.cons, &data_cdc->cons, conn);
	smc = container_of(conn, struct smc_sock, conn);
	smc_cdc_msg_recv(smc, (struct smc_cdc_msg *)&cdc);
}

/* Initialize receive tasklet. Called from ISM device IRQ handler to start
 * receiver side.
 */
void smcd_cdc_rx_init(struct smc_connection *conn)
{
	tasklet_setup(&conn->rx_tsklet, smcd_cdc_rx_tsklet);
}

/***************************** init, exit, misc ******************************/

static void smc_cdc_rx_handler(struct ib_wc *wc, void *buf)
{
	struct smc_link *link = (struct smc_link *)wc->qp->qp_context;
	struct smc_cdc_msg *cdc = buf;
	struct smc_connection *conn;
	struct smc_link_group *lgr;
	struct smc_sock *smc;

	if (unlikely(link->lgr->use_rwwi)) {
		pr_err_once("smc: recv unexpected cdc msg when rwwi enabled.\n");
		return;
	}

	if (wc->byte_len < offsetof(struct smc_cdc_msg, reserved))
		return; /* short message */
	if (cdc->len != SMC_WR_TX_SIZE)
		return; /* invalid message */

	if (cdc->credits)
		smc_wr_tx_put_credits(link, cdc->credits, true);

	/* lookup connection */
	lgr = smc_get_lgr(link);
	read_lock_bh(&lgr->conns_lock);
	conn = smc_lgr_find_conn(ntohl(cdc->token), lgr);
	read_unlock_bh(&lgr->conns_lock);
	if (!conn || conn->out_of_sync)
		return;
	smc = container_of(conn, struct smc_sock, conn);

	if (cdc->prod_flags.failover_validation) {
		smc_cdc_msg_validate(smc, cdc, link);
		return;
	}
	if (smc_cdc_before(ntohs(cdc->seqno),
			   conn->local_rx_ctrl.seqno))
		/* received seqno is old */
		return;

	smc_cdc_msg_recv(smc, cdc);
}

static void smc_cdc_handle_rwwi_data_msg(struct smc_sock *smc,
					 union smc_wr_imm_msg *imm_msg, int diff_prod)
{
	struct smc_connection *conn = &smc->conn;
	int diff_cons;

	diff_cons = imm_msg->data.diff_cons;
	if (diff_cons)
		smc_curs_add_safe(conn->peer_rmbe_size, &conn->local_rx_ctrl.cons, diff_cons, conn);
	/* cause this imm_data contains no conn_state_flags and prod_flags info, clean them */
	memset(&conn->local_rx_ctrl.conn_state_flags, 0,
	       sizeof(struct smc_cdc_conn_state_flags));
	memset(&conn->local_rx_ctrl.prod_flags, 0,
	       sizeof(struct smc_cdc_producer_flags));

	smc_dump_cdc_msg_rwwi(conn, imm_msg->imm_data,
			      &conn->local_rx_ctrl.prod,
			      &conn->local_rx_ctrl.cons, true);
	__smc_cdc_msg_recv_action(smc, diff_prod, diff_cons);
}

static void smc_cdc_handle_rwwi_data_with_flags_msg(struct smc_sock *smc,
						    union smc_wr_imm_msg *imm_msg, int diff_prod)
{
	struct smc_connection *conn = &smc->conn;
	struct smc_cdc_producer_flags *pflags;
	int diff_cons;

	diff_cons = imm_msg->data_with_flags.diff_cons;
	if (diff_cons)
		smc_curs_add_safe(conn->peer_rmbe_size, &conn->local_rx_ctrl.cons, diff_cons, conn);
	/* clean prod_flags that are not carried by this imm_data */
	memset(&conn->local_rx_ctrl.prod_flags, 0,
	       sizeof(struct smc_cdc_producer_flags));
	pflags = &conn->local_rx_ctrl.prod_flags;
	pflags->write_blocked = imm_msg->data_with_flags.write_blocked;
	pflags->urg_data_present = imm_msg->data_with_flags.urg_data_present;
	pflags->urg_data_pending = imm_msg->data_with_flags.urg_data_pending;
	/* cause this imm_data contains no conn_state_flagsinfo, clean it */
	memset(&conn->local_rx_ctrl.conn_state_flags, 0,
	       sizeof(struct smc_cdc_conn_state_flags));

	smc_dump_cdc_msg_rwwi(conn, imm_msg->imm_data,
			      &conn->local_rx_ctrl.prod,
			      &conn->local_rx_ctrl.cons, true);
	__smc_cdc_msg_recv_action(smc, diff_prod, diff_cons);
}

static void smc_cdc_handle_rwwi_data_cr_msg(struct smc_sock *smc,
					    union smc_wr_imm_msg *imm_msg, int diff_prod)
{
	struct smc_connection *conn = &smc->conn;
	int diff_cons;

	if (imm_msg->data_cr.credits)
		smc_wr_tx_put_credits(conn->lnk, imm_msg->data_cr.credits, true);

	diff_cons = imm_msg->data_cr.diff_cons;
	if (diff_cons)
		smc_curs_add_safe(conn->peer_rmbe_size, &conn->local_rx_ctrl.cons, diff_cons, conn);
	/* cause this imm_data contains no conn_state_flags and prod_flags info, clean them */
	memset(&conn->local_rx_ctrl.conn_state_flags, 0,
	       sizeof(struct smc_cdc_conn_state_flags));
	memset(&conn->local_rx_ctrl.prod_flags, 0,
	       sizeof(struct smc_cdc_producer_flags));

	smc_dump_cdc_msg_rwwi(conn, imm_msg->imm_data,
			      &conn->local_rx_ctrl.prod,
			      &conn->local_rx_ctrl.cons, true);
	__smc_cdc_msg_recv_action(smc, diff_prod, diff_cons);
}

static void smc_cdc_handle_rwwi_data_with_flags_cr_msg(struct smc_sock *smc,
						       union smc_wr_imm_msg *imm_msg, int diff_prod)
{
	struct smc_connection *conn = &smc->conn;
	struct smc_cdc_producer_flags *pflags;
	int diff_cons;

	if (imm_msg->data_with_flags_cr.credits)
		smc_wr_tx_put_credits(conn->lnk, imm_msg->data_with_flags_cr.credits, true);

	diff_cons = imm_msg->data_with_flags_cr.diff_cons;
	if (diff_cons)
		smc_curs_add_safe(conn->peer_rmbe_size, &conn->local_rx_ctrl.cons, diff_cons, conn);
	/* clean prod_flags that are not carried by this imm_data */
	memset(&conn->local_rx_ctrl.prod_flags, 0,
	       sizeof(struct smc_cdc_producer_flags));
	pflags = &conn->local_rx_ctrl.prod_flags;
	pflags->write_blocked = imm_msg->data_with_flags_cr.write_blocked;
	pflags->urg_data_present = imm_msg->data_with_flags_cr.urg_data_present;
	pflags->urg_data_pending = imm_msg->data_with_flags_cr.urg_data_pending;
	/* cause this imm_data contains no conn_state_flagsinfo, clean it */
	memset(&conn->local_rx_ctrl.conn_state_flags, 0,
	       sizeof(struct smc_cdc_conn_state_flags));

	smc_dump_cdc_msg_rwwi(conn, imm_msg->imm_data,
			      &conn->local_rx_ctrl.prod,
			      &conn->local_rx_ctrl.cons, true);
	__smc_cdc_msg_recv_action(smc, diff_prod, diff_cons);
}

static void smc_cdc_handle_rwwi_ctrl_msg(struct smc_sock *smc,
					 union smc_wr_imm_msg *imm_msg, int diff_prod)
{
	struct smc_connection *conn = &smc->conn;

	conn->local_rx_ctrl.prod_flags = imm_msg->ctrl.pflags;
	conn->local_rx_ctrl.conn_state_flags = imm_msg->ctrl.csflags;

	smc_dump_cdc_msg_rwwi(conn, imm_msg->imm_data,
			      &conn->local_rx_ctrl.prod,
			      &conn->local_rx_ctrl.cons, true);
	/* this imm_data contains no diff_cons info, clean it */
	__smc_cdc_msg_recv_action(smc, diff_prod, 0);
}

void smc_cdc_rx_handler_rwwi(struct ib_wc *wc)
{
	struct smc_link *link = wc->qp->qp_context;
	struct smc_link_group *lgr = link->lgr;
	struct smc_connection *conn = NULL;
	union smc_wr_imm_msg imm_msg;
	struct smc_sock *smc = NULL;
	int diff_prod;

	if (unlikely(!link->lgr->use_rwwi)) {
		pr_err_once("smc: recv unexpected rwwi msg when rwwi disabled.\n");
		return;
	}

	imm_msg.imm_data = be32_to_cpu(wc->ex.imm_data);
	read_lock_bh(&lgr->conns_lock);
	smc = smc_lgr_get_sock(imm_msg.hdr.token, lgr);
	read_unlock_bh(&lgr->conns_lock);
	if (!smc)
		return;

	conn = &smc->conn;
	bh_lock_sock(&smc->sk);
	diff_prod = wc->byte_len;
	if (diff_prod) {
		smc_dump_raw_data(conn, conn->local_rx_ctrl.prod.count,
				  diff_prod, true);
		smc_curs_add_safe(conn->rmb_desc->len, &conn->local_rx_ctrl.prod, diff_prod, conn);
	}
	switch (imm_msg.hdr.opcode) {
	case SMC_WR_OP_DATA:
		smc_cdc_handle_rwwi_data_msg(smc, &imm_msg, diff_prod);
		break;
	case SMC_WR_OP_DATA_WITH_FLAGS:
		smc_cdc_handle_rwwi_data_with_flags_msg(smc, &imm_msg, diff_prod);
		break;
	case SMC_WR_OP_CTRL:
		smc_cdc_handle_rwwi_ctrl_msg(smc, &imm_msg, diff_prod);
		break;
	case SMC_WR_OP_DATA_CR:
		smc_cdc_handle_rwwi_data_cr_msg(smc, &imm_msg, diff_prod);
		break;
	case SMC_WR_OP_DATA_WITH_FLAGS_CR:
		smc_cdc_handle_rwwi_data_with_flags_cr_msg(smc, &imm_msg, diff_prod);
		break;
	}

	bh_unlock_sock(&smc->sk);
	sock_put(&smc->sk); /* sock_hold in smc_lgr_get_sock */
}

static struct smc_wr_rx_handler smc_cdc_rx_handlers[] = {
	{
		.handler	= smc_cdc_rx_handler,
		.type		= SMC_CDC_MSG_TYPE
	},
	{
		.handler	= NULL,
	}
};

int __init smc_cdc_init(void)
{
	struct smc_wr_rx_handler *handler;
	int rc = 0;

	for (handler = smc_cdc_rx_handlers; handler->handler; handler++) {
		INIT_HLIST_NODE(&handler->list);
		rc = smc_wr_rx_register_handler(handler);
		if (rc)
			break;
	}
	return rc;
}
