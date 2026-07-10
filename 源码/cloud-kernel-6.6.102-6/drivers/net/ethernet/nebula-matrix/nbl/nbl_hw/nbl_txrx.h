/* SPDX-License-Identifier: GPL-2.0*/
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#ifndef _NBL_TXRX_H_
#define _NBL_TXRX_H_

#include "nbl_resource.h"

#define NBL_RING_TO_COMMON(ring)	((ring)->common)
#define NBL_RING_TO_DEV(ring)		((ring)->dma_dev)
#define NBL_RING_TO_DMA_DEV(ring)	((ring)->dma_dev)

#define NBL_MIN_DESC_NUM			128
#define NBL_MAX_DESC_NUM			32768

#define DEFAULT_MAX_PF_QUEUE_PAIRS_NUM		16
#define DEFAULT_MAX_VF_QUEUE_PAIRS_NUM		2

#define NBL_PACKED_DESC_F_NEXT			1
#define NBL_PACKED_DESC_F_WRITE			2
#define NBL_PACKED_DESC_F_AVAIL			7
#define NBL_PACKED_DESC_F_USED			15

#define NBL_TX_DESC(tx_ring, i)			(&(((tx_ring)->desc)[i]))
#define NBL_RX_DESC(rx_ring, i)			(&(((rx_ring)->desc)[i]))
#define NBL_TX_BUF(tx_ring, i)			(&(((tx_ring)->tx_bufs)[i]))
#define NBL_RX_BUF(rx_ring, i)			(&(((rx_ring)->rx_bufs)[i]))

#define NBL_RX_BUF_256				256
#define NBL_RX_HDR_SIZE				NBL_RX_BUF_256
#define NBL_BUFFER_HDR_LEN			(sizeof(struct nbl_rx_extend_head))
#define NBL_RX_PAD				(NET_IP_ALIGN + NET_SKB_PAD)
#define NBL_RX_BUFSZ				(2048)
#define NBL_RXBUF_MIN_ORDER			(10)
#define NBL_RX_DMA_ATTR				(DMA_ATTR_SKIP_CPU_SYNC | DMA_ATTR_WEAK_ORDERING)

#define NBL_TX_TOTAL_HEADERLEN_SHIFT		24
#define DESC_NEEDED				(MAX_SKB_FRAGS + 4)
#define NBL_TX_POLL_WEIGHT			256
#define NBL_TXD_DATALEN_BITS			16
#define NBL_TXD_DATALEN_MAX			BIT(NBL_TXD_DATALEN_BITS)
#define MAX_DESC_NUM_PER_PKT			(32)

#define NBL_TX_TSO_MSS_MIN			(256)
#define NBL_TX_TSO_MSS_MAX			(16383)
#define NBL_TX_TSO_L2L3L4_HDR_LEN_MIN		(42)
#define NBL_TX_TSO_L2L3L4_HDR_LEN_MAX		(128)
#define NBL_TX_CHECKSUM_OFFLOAD_L2L3L4_HDR_LEN_MAX (255)
#define IP_VERSION_V4				(4)
#define NBL_TX_FLAGS_TSO			BIT(0)

#define NBL_KTLS_INIT_PAD_LEN			28
#define NBL_KTLS_SYNC_PKT_LEN			30
#define NBL_KTLS_PER_CELL_LEN			4096
#define NBL_KTLS_MAX_CELL_LEN			6144

/* TX inner IP header type */
enum nbl_tx_iipt {
	NBL_TX_IIPT_NONE = 0x0,
	NBL_TX_IIPT_IPV6 = 0x1,
	NBL_TX_IIPT_IPV4 = 0x2,
	NBL_TX_IIPT_RSV  = 0x3
};

/* TX L4 packet type */
enum nbl_tx_l4t {
	NBL_TX_L4T_NONE = 0x0,
	NBL_TX_L4T_TCP  = 0x1,
	NBL_TX_L4T_UDP  = 0x2,
	NBL_TX_L4T_RSV  = 0x3
};

struct nbl_tx_hdr_param {
	u8 l4s_pbrac_mode;
	u8 l4s_hdl_ind;
	u8 l4s_sync_ind;
	u8 tso;
	u16 l4s_sid;
	u16 mss;
	u8 mac_len;
	u8 ip_len;
	u8 l4_len;
	u8 l4_type;
	u8 inner_ip_type;
	u8 l3_csum_en;
	u8 l4_csum_en;
	u16 total_hlen;
	u16 dport_id:10;
	u16 fwd:2;
	u16 dport:3;
	u16 rss_lag_en:1;
};

union nbl_tx_extend_head {
	struct {
		/* DW0 */
		u32 mac_len :5;
		u32 ip_len :5;
		u32 l4_len :4;
		u32 l4_type :2;
		u32 inner_ip_type :2;
		u32 external_ip_type :2;
		u32 external_ip_len :5;
		u32 l4_tunnel_type :2;
		u32 l4_tunnel_len :5;
		/* DW1 */
		u32 l4s_sid :10;
		u32 l4s_sync_ind :1;
		u32 l4s_redun_ind :1;
		u32 l4s_redun_head_ind :1;
		u32 l4s_hdl_ind :1;
		u32 l4s_pbrac_mode :1;
		u32 rsv0 :2;
		u32 mss :14;
		u32 tso :1;
		/* DW2 */
		/* if dport = NBL_TX_DPORT_ETH; dport_info = 0
		 * if dport = NBL_TX_DPORT_HOST; dport_info = host queue id
		 * if dport = NBL_TX_DPORT_ECPU; dport_info = ecpu queue_id
		 */
		u32 dport_info :11;
		/* if dport = NBL_TX_DPORT_ETH; dport_id[3:0] = eth port id, dport_id[9:4] = lag id
		 * if dport = NBL_TX_DPORT_HOST; dport_id[9:0] = host vsi_id
		 * if dport = NBL_TX_DPORT_ECPU; dport_id[9:0] = ecpu vsi_id
		 */
		u32 dport_id :10;
#define NBL_TX_DPORT_ID_LAG_OFFSET	(4)
		u32 dport :3;
#define NBL_TX_DPORT_ETH		(0)
#define NBL_TX_DPORT_HOST		(1)
#define NBL_TX_DPORT_ECPU		(2)
#define NBL_TX_DPORT_EMP		(3)
#define NBL_TX_DPORT_BMC		(4)
		u32 fwd :2;
#define NBL_TX_FWD_TYPE_DROP		(0)
#define NBL_TX_FWD_TYPE_NORMAL		(1)
#define NBL_TX_FWD_TYPE_RSV		(2)
#define NBL_TX_FWD_TYPE_CPU_ASSIGNED	(3)
		u32 rss_lag_en :1;
		u32 l4_csum_en :1;
		u32 l3_csum_en :1;
		u32 rsv1 :3;
	};
	struct bootis_hdr {
		/* DW0 */
		u32 mac_len :5;
		u32 ip_len :5;
		u32 l4_len :4;
		u32 l4_type :2;
		u32 inner_ip_type :2;
		u32 external_ip_type :2;
		u32 external_ip_len :5;
		u32 l4_tunnel_type :2;
		u32 l4_tunnel_len :5;
		/* DW1 */
		u32 l4s_sid :10;
		u32 inner_l3_cs :1;
		u32 inner_l4_cs :1;
		u32 dport :3;
		u32 tag_idx :2;
		u32 mss :14;
		u32 tso :1;
		/* DW2 */
		u32 dport_info :11;
		u32 dport_id :12;
		u32 tag_en :1;
		u32 fwd :2;
		u32 rss_lag_en :1;
		u32 l4_csum_en :1;
		u32 l3_csum_en :1;
		u32 rsv1 :3;
	} bootis;
};

struct nbl_rx_extend_head {
	/* DW0 */
	/* 0x0:eth, 0x1:host, 0x2:ecpu, 0x3:emp, 0x4:bcm */
	uint32_t sport :3;
	uint32_t dport_info :11;
	/* sport = 0, sport_id[3:0] = eth id,
	 * sport = 1, sport_id[9:0] = host vsi_id,
	 * sport = 2, sport_id[9:0] = ecpu vsi_id,
	 */
	uint32_t sport_id :10;
	/* 0x0:drop, 0x1:normal, 0x2:cpu upcall */
	uint32_t fwd :2;
	uint32_t rsv0 :6;
	/* DW1 */
	uint32_t error_code :6;
	uint32_t ptype :10;
	uint32_t profile_id :4;
	uint32_t checksum_status :1;
	uint32_t rsv1 :1;
	uint32_t l4s_sid :10;
	/* DW2 */
	uint32_t rsv3 :2;
	uint32_t l4s_hdl_ind :1;
	uint32_t l4s_tcp_offset :14;
	uint32_t l4s_resync_ind :1;
	uint32_t l4s_check_ind :1;
	uint32_t l4s_dec_ind :1;
	uint32_t rsv2 :4;
	uint32_t num_buffers :8;
} __packed;

struct nbl_ktls_init_payload {
	/* DW0 */
	u16 initial:1;
	u16 rsv1:7;
	u16 sync:1;
	u16 rsv2:7;
	u16 sid:10;
	u16 rsv3:6;
	/* DW1 */
	u16 rsv4;
	u16 rsv5;
	/* DWX */
	u8 rec_num[NBL_KTLS_REC_LEN];
	u8 iv[NBL_KTLS_IV_LEN];
	u8 pad[NBL_KTLS_INIT_PAD_LEN];
};

struct nbl_ktls_sync_payload {
	/* DW0 */
	u16 initial:1;
	u16 rsv1:7;
	u16 sync:1;
	u16 rsv2:7;
	u16 sid:10;
	u16 rsv3:6;
	/* DW1 */
	u16 rsv4;
	u16 rsv5;
	/* DWX */
	u8 rec_num[NBL_KTLS_REC_LEN];
	__be16 redlen;
	u8 redata[NBL_KTLS_MAX_CELL_LEN];
};

struct nbl_ktls_init_packet {
	union nbl_tx_extend_head pkthdr;
	struct nbl_ktls_init_payload init_payload;
};

struct nbl_ktls_sync_packet {
	union nbl_tx_extend_head pkthdr;
	struct nbl_ktls_sync_payload sync_payload;
};

enum nbl_ktls_sync_retval {
	NBL_KTLS_SYNC_DONE,
	NBL_KTLS_SYNC_SKIP_NO_DATA,
	NBL_KTLS_SYNC_FAIL,
};

struct nbl_tx_resync_info {
	u64 rec_num;
	u32 resync_len;
	u32 nr_frags;
	skb_frag_t frags[MAX_SKB_FRAGS];
};

#define NBL_XDP_FLAG_TX			BIT(0)
#define NBL_XDP_FLAG_REDIRECT		BIT(1)
#define NBL_XDP_FLAG_DROP		BIT(2)
#define NBL_XDP_FLAG_OVERSIZE		BIT(3)
#define NBL_XDP_FLAG_MULTICAST		BIT(4)

struct nbl_xdp_output {
	u64 bytes;
	u16 desc_done_num;
	u16 flags;
};
DECLARE_STATIC_KEY_FALSE(nbl_xdp_locking_key);
static inline u16 nbl_unused_rx_desc_count(struct nbl_res_rx_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->desc_num) + ntc - ntu - 1;
}

static inline u16 nbl_unused_tx_desc_count(struct nbl_res_tx_ring *ring)
{
	u16 ntc = ring->next_to_clean;
	u16 ntu = ring->next_to_use;

	return ((ntc > ntu) ? 0 : ring->desc_num) + ntc - ntu - 1;
}

static inline bool nbl_ktls_device_offload(struct sk_buff *skb)
{
#ifdef CONFIG_TLS_DEVICE
	return tls_is_skb_tx_device_offloaded(skb);
#else
	return false;
#endif
}

static inline void nbl_ktls_bigint_decrement(u8 *data, int len)
{
	int i;

	for (i = len - 1; i >= 0; i--) {
		if (data[i] == 0) {
			data[i] = 0xFF;
		} else {
			--data[i];
			break;
		}
	}
}

static inline
struct nbl_res_tx_ring *nbl_res_txrx_select_xdp_ring(struct nbl_txrx_mgt *txrx_mgt)
{
	int ring_idx;
	int cpu_id = smp_processor_id();
	struct nbl_res_tx_ring *xdp_ring;

	if (!txrx_mgt->xdp_ring_num)
		return NULL;

	if (static_key_enabled(&nbl_xdp_locking_key))
		ring_idx = cpu_id % txrx_mgt->xdp_ring_num;
	else
		ring_idx = cpu_id;

	xdp_ring = txrx_mgt->tx_rings[ring_idx + txrx_mgt->xdp_ring_offset];
	return xdp_ring;
}

static inline bool nbl_res_txrx_is_xdp_ring(struct nbl_res_tx_ring *ring)
{
	return READ_ONCE(ring->xdp_prog) ? true : false;
}

#endif
