// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#include "nbl_txrx.h"
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>
#include <linux/sctp.h>
#include <linux/if_vlan.h>
#include <net/page_pool/helpers.h>

#include <linux/bpf_trace.h>

DEFINE_STATIC_KEY_FALSE(nbl_xdp_locking_key);

static bool nbl_txrx_within_vsi(struct nbl_txrx_vsi_info *vsi_info, u16 ring_index)
{
	return ring_index >= vsi_info->ring_offset &&
	       ring_index < vsi_info->ring_offset + vsi_info->ring_num;
}

static struct netdev_queue *txring_txq(const struct nbl_res_tx_ring *ring)
{
	return netdev_get_tx_queue(ring->netdev, ring->queue_index);
}

static struct nbl_res_tx_ring *
nbl_alloc_tx_ring(struct nbl_resource_mgt *res_mgt, struct net_device *netdev, u16 ring_index,
		  u16 desc_num)
{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_txrx_mgt *txrx_mgt = res_mgt->txrx_mgt;
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct nbl_res_tx_ring *ring;

	ring = devm_kzalloc(dev, sizeof(struct nbl_res_tx_ring), GFP_KERNEL);
	if (!ring)
		return NULL;

	ring->vsi_info = txrx_mgt->vsi_info;
	ring->dma_dev = common->dma_dev;
	ring->product_type = common->product_type;
	ring->eth_id = common->eth_id;
	ring->queue_index = ring_index;
	ring->notify_addr = phy_ops->get_tail_ptr(NBL_RES_MGT_TO_PHY_PRIV(res_mgt));
	ring->notify_qid = NBL_RES_NOFITY_QID(res_mgt, ring_index * 2 + 1);
	ring->netdev = netdev;
	ring->desc_num = desc_num;
	ring->used_wrap_counter = 1;
	ring->avail_used_flags |= BIT(NBL_PACKED_DESC_F_AVAIL);

	if (res_mgt->resource_info->eswitch_info)
		ring->mode = res_mgt->resource_info->eswitch_info->mode;

	return ring;
}

static int nbl_alloc_tx_rings(struct nbl_resource_mgt *res_mgt, struct net_device *netdev,
			      u16 tx_num, u16 desc_num)
{
	struct nbl_txrx_mgt *txrx_mgt = res_mgt->txrx_mgt;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct nbl_res_tx_ring *ring;
	u32 ring_index;

	if (txrx_mgt->tx_rings) {
		nbl_err(common, NBL_DEBUG_RESOURCE,
			"Try to allocate tx_rings which already exists\n");
		return -EINVAL;
	}

	txrx_mgt->tx_ring_num = tx_num;

	txrx_mgt->tx_rings = devm_kcalloc(dev, tx_num,
					  sizeof(struct nbl_res_tx_ring *), GFP_KERNEL);
	if (!txrx_mgt->tx_rings)
		return -ENOMEM;

	for (ring_index = 0; ring_index < tx_num; ring_index++) {
		ring = txrx_mgt->tx_rings[ring_index];
		WARN_ON(ring);
		ring = nbl_alloc_tx_ring(res_mgt, netdev, ring_index, desc_num);
		if (!ring)
			goto alloc_tx_ring_failed;

		WRITE_ONCE(txrx_mgt->tx_rings[ring_index], ring);
	}

	return 0;

alloc_tx_ring_failed:
	while (ring_index--)
		devm_kfree(dev, txrx_mgt->tx_rings[ring_index]);
	devm_kfree(dev, txrx_mgt->tx_rings);
	txrx_mgt->tx_rings = NULL;
	return -ENOMEM;
}

static void nbl_free_tx_rings(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_txrx_mgt *txrx_mgt = res_mgt->txrx_mgt;
	struct nbl_res_tx_ring *ring;
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	u16 ring_count;
	u16 ring_index;

	ring_count = txrx_mgt->tx_ring_num;
	for (ring_index = 0; ring_index < ring_count; ring_index++) {
		ring = txrx_mgt->tx_rings[ring_index];
		devm_kfree(dev, ring);
	}
	devm_kfree(dev, txrx_mgt->tx_rings);
	txrx_mgt->tx_rings = NULL;
}

static int nbl_alloc_rx_rings(struct nbl_resource_mgt *res_mgt, struct net_device *netdev,
			      u16 rx_num, u16 desc_num)
{
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_txrx_mgt *txrx_mgt = res_mgt->txrx_mgt;
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct nbl_res_rx_ring *ring;
	u32 ring_index;

	if (txrx_mgt->rx_rings) {
		nbl_err(common, NBL_DEBUG_RESOURCE,
			"Try to allocate rx_rings which already exists\n");
		return -EINVAL;
	}

	txrx_mgt->rx_ring_num = rx_num;

	txrx_mgt->rx_rings = devm_kcalloc(dev, rx_num,
					  sizeof(struct nbl_res_rx_ring *), GFP_KERNEL);
	if (!txrx_mgt->rx_rings)
		return -ENOMEM;

	for (ring_index = 0; ring_index < rx_num; ring_index++) {
		ring = txrx_mgt->rx_rings[ring_index];
		WARN_ON(ring);
		ring = devm_kzalloc(dev, sizeof(struct nbl_res_rx_ring), GFP_KERNEL);
		if (!ring)
			goto alloc_rx_ring_failed;

		ring->common = common;
		ring->txrx_mgt = txrx_mgt;
		ring->dma_dev = common->dma_dev;
		ring->queue_index = ring_index;
		ring->notify_qid = NBL_RES_NOFITY_QID(res_mgt, ring_index * 2);
		ring->netdev = netdev;
		ring->desc_num = desc_num;
		/* RX buffer length is determined by mtu,
		 * when netdev up we will set buf_len according to its mtu
		 */
		ring->buf_len = PAGE_SIZE / 2 - NBL_RX_PAD;

		ring->used_wrap_counter = 1;
		ring->avail_used_flags |= BIT(NBL_PACKED_DESC_F_AVAIL);
		WRITE_ONCE(txrx_mgt->rx_rings[ring_index], ring);
	}

	return 0;

alloc_rx_ring_failed:
	while (ring_index--)
		devm_kfree(dev, txrx_mgt->rx_rings[ring_index]);
	devm_kfree(dev, txrx_mgt->rx_rings);
	txrx_mgt->rx_rings = NULL;
	return -ENOMEM;
}

static void nbl_free_rx_rings(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_txrx_mgt *txrx_mgt = res_mgt->txrx_mgt;
	struct nbl_res_rx_ring *ring;
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	u16 ring_count;
	u16 ring_index;

	ring_count = txrx_mgt->rx_ring_num;
	for (ring_index = 0; ring_index < ring_count; ring_index++) {
		ring = txrx_mgt->rx_rings[ring_index];
		devm_kfree(dev, ring);
	}
	devm_kfree(dev, txrx_mgt->rx_rings);
	txrx_mgt->rx_rings = NULL;
}

static int nbl_alloc_vectors(struct nbl_resource_mgt *res_mgt, u16 total_num, u16 xdp_ring_offset)
{
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_txrx_mgt *txrx_mgt = res_mgt->txrx_mgt;
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct nbl_res_vector *vector;
	u32 index;
	u16 xdp_ring_num;

	if (txrx_mgt->vectors) {
		nbl_err(common, NBL_DEBUG_RESOURCE,
			"Try to allocate vectors which already exists\n");
		return -EINVAL;
	}

	txrx_mgt->vectors = devm_kcalloc(dev, xdp_ring_offset, sizeof(struct nbl_res_vector *),
					 GFP_KERNEL);
	if (!txrx_mgt->vectors)
		return -ENOMEM;

	for (index = 0; index < xdp_ring_offset; index++) {
		vector = txrx_mgt->vectors[index];
		WARN_ON(vector);
		vector = devm_kzalloc(dev, sizeof(struct nbl_res_vector), GFP_KERNEL);
		if (!vector)
			goto alloc_vector_failed;

		vector->rx_ring = txrx_mgt->rx_rings[index];
		vector->tx_ring = txrx_mgt->tx_rings[index];
		WRITE_ONCE(txrx_mgt->vectors[index], vector);
	}

	xdp_ring_num = total_num - xdp_ring_offset;
	for (index = 0; index < xdp_ring_num; index++) {
		vector = txrx_mgt->vectors[index];
		vector->xdp_ring = txrx_mgt->tx_rings[index + xdp_ring_offset];
	}

	return 0;

alloc_vector_failed:
	while (index--)
		devm_kfree(dev, txrx_mgt->vectors[index]);
	devm_kfree(dev, txrx_mgt->vectors);
	txrx_mgt->vectors = NULL;
	return -ENOMEM;
}

static void nbl_free_vectors(struct nbl_resource_mgt *res_mgt)
{
	struct nbl_txrx_mgt *txrx_mgt = res_mgt->txrx_mgt;
	struct nbl_res_vector *vector;
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	u16 count, index;

	count = txrx_mgt->xdp_ring_offset;
	for (index = 0; index < count; index++) {
		vector = txrx_mgt->vectors[index];
		devm_kfree(dev, vector);
	}
	devm_kfree(dev, txrx_mgt->vectors);
	txrx_mgt->vectors = NULL;
}

static int nbl_res_txrx_alloc_rings(void *priv, struct net_device *netdev,
				    struct nbl_ring_param *param)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_txrx_mgt *txrx_mgt = res_mgt->txrx_mgt;
	int err = 0;

	err = nbl_alloc_tx_rings(res_mgt, netdev, param->tx_ring_num, param->queue_size);
	if (err)
		return err;

	err = nbl_alloc_rx_rings(res_mgt, netdev, param->rx_ring_num, param->queue_size);
	if (err)
		goto alloc_rx_rings_err;

	err = nbl_alloc_vectors(res_mgt, param->tx_ring_num, param->xdp_ring_offset);
	if (err)
		goto alloc_vectors_err;

	txrx_mgt->xdp_ring_offset = param->xdp_ring_offset;
	txrx_mgt->xdp_ring_num = param->tx_ring_num - param->xdp_ring_offset;

	if (txrx_mgt->xdp_ring_num && num_online_cpus() > txrx_mgt->xdp_ring_num)
		static_branch_inc(&nbl_xdp_locking_key);

	nbl_info(res_mgt->common, NBL_DEBUG_RESOURCE,
		 "Alloc rings for %d tx, %d rx, %d xdp_offset, %d desc\n",
		 param->tx_ring_num, param->rx_ring_num, param->xdp_ring_offset, param->queue_size);
	return 0;

alloc_vectors_err:
	nbl_free_rx_rings(res_mgt);
alloc_rx_rings_err:
	nbl_free_tx_rings(res_mgt);
	return err;
}

static void nbl_res_txrx_remove_rings(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_txrx_mgt *txrx_mgt = res_mgt->txrx_mgt;

	if (txrx_mgt->xdp_ring_num && num_online_cpus() > txrx_mgt->xdp_ring_num &&
	    static_key_enabled(&nbl_xdp_locking_key))
		static_branch_dec(&nbl_xdp_locking_key);

	nbl_free_vectors(res_mgt);
	nbl_free_tx_rings(res_mgt);
	nbl_free_rx_rings(res_mgt);
	nbl_info(res_mgt->common, NBL_DEBUG_RESOURCE, "Remove rings");
}

static dma_addr_t nbl_res_txrx_start_tx_ring(void *priv, u8 ring_index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct device *dma_dev = NBL_RES_MGT_TO_DMA_DEV(res_mgt);
	struct nbl_res_tx_ring *tx_ring = NBL_RES_MGT_TO_TX_RING(res_mgt, ring_index);

	if (tx_ring->tx_bufs) {
		nbl_err(res_mgt->common, NBL_DEBUG_RESOURCE,
			"Try to setup a TX ring with buffer management array already allocated\n");
		return (dma_addr_t)NULL;
	}

	tx_ring->tx_bufs = devm_kcalloc(dev, tx_ring->desc_num, sizeof(*tx_ring->tx_bufs),
					GFP_KERNEL);
	if (!tx_ring->tx_bufs)
		return (dma_addr_t)NULL;

	/* Alloc twice memory, and second half is used to back up the desc for desc checking */
	tx_ring->size = ALIGN(tx_ring->desc_num * sizeof(struct nbl_ring_desc), PAGE_SIZE);
	tx_ring->desc = dmam_alloc_coherent(dma_dev, tx_ring->size, &tx_ring->dma,
					    GFP_KERNEL | __GFP_ZERO);
	if (!tx_ring->desc)
		goto alloc_dma_err;

	tx_ring->next_to_use = 0;
	tx_ring->next_to_clean = 0;
	tx_ring->tail_ptr = 0;

	tx_ring->valid = true;
	nbl_debug(res_mgt->common, NBL_DEBUG_RESOURCE, "Start tx ring %d", ring_index);
	return tx_ring->dma;

alloc_dma_err:
	devm_kfree(dev, tx_ring->tx_bufs);
	tx_ring->tx_bufs = NULL;
	tx_ring->size = 0;
	return (dma_addr_t)NULL;
}

static inline bool nbl_rx_cache_get(struct nbl_res_rx_ring *rx_ring, struct nbl_dma_info *dma_info)
{
	struct nbl_page_cache *cache = &rx_ring->page_cache;
	struct nbl_rx_queue_stats *stats = &rx_ring->rx_stats;

	if (unlikely(cache->head == cache->tail)) {
		stats->rx_cache_empty++;
		return false;
	}

	if (page_ref_count(cache->page_cache[cache->head].page) != 1) {
		stats->rx_cache_busy++;
		return false;
	}

	*dma_info = cache->page_cache[cache->head];
	cache->head = (cache->head + 1) & (NBL_MAX_CACHE_SIZE - 1);
	stats->rx_cache_reuse++;

	dma_sync_single_for_device(rx_ring->dma_dev, dma_info->addr,
				   dma_info->size, DMA_FROM_DEVICE);
	return true;
}

static inline int nbl_page_alloc_pool(struct nbl_res_rx_ring *rx_ring,
				      struct nbl_dma_info *dma_info)
{
	if (nbl_rx_cache_get(rx_ring, dma_info))
		return 0;

	dma_info->page = page_pool_dev_alloc_pages(rx_ring->page_pool);
	if (unlikely(!dma_info->page))
		return -ENOMEM;

	dma_info->addr = dma_map_page_attrs(rx_ring->dma_dev, dma_info->page, 0, dma_info->size,
					    DMA_FROM_DEVICE, NBL_RX_DMA_ATTR);

	if (unlikely(dma_mapping_error(rx_ring->dma_dev, dma_info->addr))) {
		page_pool_recycle_direct(rx_ring->page_pool, dma_info->page);
		dma_info->page = NULL;
		return -ENOMEM;
	}

	return 0;
}

static inline int nbl_get_rx_frag(struct nbl_res_rx_ring *rx_ring, struct nbl_rx_buffer *buffer)
{
	int err = 0;

	/* first buffer alloc page */
	if (buffer->offset == buffer->rx_pad)
		err = nbl_page_alloc_pool(rx_ring, buffer->di);

	return err;
}

static inline bool nbl_alloc_rx_bufs(struct nbl_res_rx_ring *rx_ring, u16 count)
{
	u32 buf_len;
	u16 next_to_use, head;
	__le16 head_flags = 0;
	struct nbl_ring_desc *rx_desc, *head_desc;
	struct nbl_rx_buffer *rx_buf;
	int i;

	if (unlikely(!rx_ring || !count)) {
		nbl_warn(NBL_RING_TO_COMMON(rx_ring), NBL_DEBUG_RESOURCE,
			 "invalid input parameters, rx_ring is %p, count is %d.\n", rx_ring, count);
		return -EINVAL;
	}

	buf_len = rx_ring->buf_len;
	next_to_use = rx_ring->next_to_use;

	head = next_to_use;
	head_desc = NBL_RX_DESC(rx_ring, next_to_use);
	rx_desc = NBL_RX_DESC(rx_ring, next_to_use);
	rx_buf = NBL_RX_BUF(rx_ring, next_to_use);

	if (unlikely(!rx_desc || !rx_buf)) {
		nbl_warn(NBL_RING_TO_COMMON(rx_ring), NBL_DEBUG_RESOURCE,
			 "invalid input parameters, next_to_use:%d, rx_desc is %p, rx_buf is %p.\n",
			 next_to_use, rx_desc, rx_buf);
		return -EINVAL;
	}

	do {
		if (nbl_get_rx_frag(rx_ring, rx_buf))
			break;

		for (i = 0; i < rx_ring->frags_num_per_page; i++, rx_desc++, rx_buf++) {
			rx_desc->addr = cpu_to_le64(rx_buf->di->addr + rx_buf->offset);
			rx_desc->len = cpu_to_le32(buf_len);
			rx_desc->id = cpu_to_le16(next_to_use);

			if (likely(head != next_to_use || i))
				rx_desc->flags = cpu_to_le16(rx_ring->avail_used_flags |
							     NBL_PACKED_DESC_F_WRITE);
			else
				head_flags = cpu_to_le16(rx_ring->avail_used_flags |
							 NBL_PACKED_DESC_F_WRITE);
		}

		next_to_use += rx_ring->frags_num_per_page;
		rx_ring->tail_ptr += rx_ring->frags_num_per_page;
		count -= rx_ring->frags_num_per_page;
		if (next_to_use == rx_ring->desc_num) {
			next_to_use = 0;
			rx_desc = NBL_RX_DESC(rx_ring, next_to_use);
			rx_buf = NBL_RX_BUF(rx_ring, next_to_use);
			rx_ring->avail_used_flags ^=
				BIT(NBL_PACKED_DESC_F_AVAIL) |
				BIT(NBL_PACKED_DESC_F_USED);
		}
	} while (count);

	if (next_to_use != head) {
		/* wmb */
		wmb();
		head_desc->flags = head_flags;
		rx_ring->next_to_use = next_to_use;
	}

	return !!count;
}

static void nbl_unmap_and_free_tx_resource(struct nbl_res_tx_ring *ring,
					   struct nbl_tx_buffer *tx_buffer,
					   bool free, bool in_napi)
{
	struct device *dma_dev = NBL_RING_TO_DMA_DEV(ring);

	if (tx_buffer->skb) {
		if (likely(!nbl_res_txrx_is_xdp_ring(ring))) {
			if (likely(free)) {
				if (in_napi)
					napi_consume_skb(tx_buffer->skb, NBL_TX_POLL_WEIGHT);
				else
					dev_kfree_skb_any(tx_buffer->skb);
			}
		} else {
			if (likely(free))
				page_frag_free(tx_buffer->raw_buff);
		}

		if (dma_unmap_len(tx_buffer, len))
			dma_unmap_single(dma_dev, dma_unmap_addr(tx_buffer, dma),
					 dma_unmap_len(tx_buffer, len),
					 DMA_TO_DEVICE);
	} else if (tx_buffer->page && dma_unmap_len(tx_buffer, len)) {
		dma_unmap_page(dma_dev, dma_unmap_addr(tx_buffer, dma),
			       dma_unmap_len(tx_buffer, len),
			       DMA_TO_DEVICE);
	} else if (dma_unmap_len(tx_buffer, len)) {
		dma_unmap_single(dma_dev, dma_unmap_addr(tx_buffer, dma),
				 dma_unmap_len(tx_buffer, len),
				 DMA_TO_DEVICE);
	}

	kfree(tx_buffer->tls_pkthdr);
	tx_buffer->tls_pkthdr = NULL;
	tx_buffer->next_to_watch = NULL;
	tx_buffer->skb = NULL;
	tx_buffer->page = 0;
	tx_buffer->bytecount = 0;
	tx_buffer->gso_segs = 0;
	dma_unmap_len_set(tx_buffer, len, 0);
}

static void nbl_free_tx_ring_bufs(struct nbl_res_tx_ring *tx_ring)
{
	struct nbl_tx_buffer *tx_buffer;
	u16 i;

	i = tx_ring->next_to_clean;
	tx_buffer = NBL_TX_BUF(tx_ring, i);
	while (i != tx_ring->next_to_use) {
		nbl_unmap_and_free_tx_resource(tx_ring, tx_buffer, true, false);
		i++;
		tx_buffer++;
		if (i == tx_ring->desc_num) {
			i = 0;
			tx_buffer = NBL_TX_BUF(tx_ring, i);
		}
	}

	tx_ring->next_to_clean = 0;
	tx_ring->next_to_use = 0;
	tx_ring->tail_ptr = 0;

	tx_ring->used_wrap_counter = 1;
	tx_ring->avail_used_flags = BIT(NBL_PACKED_DESC_F_AVAIL);
	memset(tx_ring->desc, 0, tx_ring->size);
}

static void nbl_res_txrx_stop_tx_ring(void *priv, u8 ring_index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct device *dma_dev = NBL_RES_MGT_TO_DMA_DEV(res_mgt);
	struct nbl_res_tx_ring *tx_ring = NBL_RES_MGT_TO_TX_RING(res_mgt, ring_index);
	struct nbl_res_vector *vector = NULL;

	if (!nbl_txrx_within_vsi(&tx_ring->vsi_info[NBL_VSI_XDP], ring_index))
		vector = NBL_RES_MGT_TO_VECTOR(res_mgt, ring_index);

	if (vector) {
		vector->started = false;
		/* Flush napi task, to ensue the sched napi finish. So napi will no to access the
		 * ring memory(wild point), bacause the vector->started has set false.
		 */
		napi_synchronize(&vector->nbl_napi.napi);
	}

	tx_ring->valid = false;

	nbl_free_tx_ring_bufs(tx_ring);
	WRITE_ONCE(NBL_RES_MGT_TO_TX_RING(res_mgt, ring_index), tx_ring);

	devm_kfree(dev, tx_ring->tx_bufs);
	tx_ring->tx_bufs = NULL;

	dmam_free_coherent(dma_dev, tx_ring->size, tx_ring->desc, tx_ring->dma);
	tx_ring->desc = NULL;
	tx_ring->dma = (dma_addr_t)NULL;
	tx_ring->size = 0;

	if (nbl_txrx_within_vsi(&tx_ring->vsi_info[NBL_VSI_DATA], tx_ring->queue_index))
		netdev_tx_reset_queue(txring_txq(tx_ring));

	nbl_debug(res_mgt->common, NBL_DEBUG_RESOURCE, "Stop tx ring %d", ring_index);
}

static inline bool nbl_dev_page_is_reusable(struct page *page, u8 nid)
{
	return likely(page_to_nid(page) == nid && !page_is_pfmemalloc(page));
}

static inline int nbl_rx_cache_put(struct nbl_res_rx_ring *rx_ring, struct nbl_dma_info *dma_info)
{
	struct nbl_page_cache *cache = &rx_ring->page_cache;
	u32 tail_next = (cache->tail + 1) & (NBL_MAX_CACHE_SIZE - 1);
	struct nbl_rx_queue_stats *stats = &rx_ring->rx_stats;

	if (tail_next == cache->head) {
		stats->rx_cache_full++;
		return 0;
	}

	if (!nbl_dev_page_is_reusable(dma_info->page, rx_ring->nid)) {
		stats->rx_cache_waive++;
		return 1;
	}

	cache->page_cache[cache->tail] = *dma_info;
	cache->tail = tail_next;

	return 2;
}

static inline void nbl_page_release_dynamic(struct nbl_res_rx_ring *rx_ring,
					    struct nbl_dma_info *dma_info, bool recycle)
{
	u32 ret;

	if (likely(recycle)) {
		ret = nbl_rx_cache_put(rx_ring, dma_info);
		if (ret == 2)
			return;
		if (ret == 1)
			goto free_page;
		dma_unmap_page_attrs(rx_ring->dma_dev, dma_info->addr, dma_info->size,
				     DMA_FROM_DEVICE, NBL_RX_DMA_ATTR);
		page_pool_recycle_direct(rx_ring->page_pool, dma_info->page);

		return;
	}
free_page:
	dma_unmap_page_attrs(rx_ring->dma_dev, dma_info->addr, dma_info->size,
			     DMA_FROM_DEVICE, NBL_RX_DMA_ATTR);
	page_pool_put_page(rx_ring->page_pool, dma_info->page, dma_info->size, true);
}

static inline void nbl_put_rx_frag(struct nbl_res_rx_ring *rx_ring,
				   struct nbl_rx_buffer *buffer, bool recycle)
{
	if (buffer->last_in_page)
		nbl_page_release_dynamic(rx_ring, buffer->di, recycle);
}

static void nbl_free_rx_ring_bufs(struct nbl_res_rx_ring *rx_ring)
{
	struct nbl_rx_buffer *rx_buf;
	u16 i;

	i = rx_ring->next_to_clean;
	rx_buf = NBL_RX_BUF(rx_ring, i);
	while (i != rx_ring->next_to_use) {
		nbl_put_rx_frag(rx_ring, rx_buf, false);
		i++;
		rx_buf++;
		if (i == rx_ring->desc_num) {
			i = 0;
			rx_buf = NBL_RX_BUF(rx_ring, i);
		}
	}

	for (i = rx_ring->page_cache.head; i != rx_ring->page_cache.tail;
	     i = (i + 1) & (NBL_MAX_CACHE_SIZE - 1)) {
		struct nbl_dma_info *dma_info = &rx_ring->page_cache.page_cache[i];

		nbl_page_release_dynamic(rx_ring, dma_info, false);
	}

	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;
	rx_ring->tail_ptr = 0;
	rx_ring->page_cache.head = 0;
	rx_ring->page_cache.tail = 0;

	rx_ring->used_wrap_counter = 1;
	rx_ring->avail_used_flags = BIT(NBL_PACKED_DESC_F_AVAIL);
	memset(rx_ring->desc, 0, rx_ring->size);
}

static dma_addr_t nbl_res_txrx_start_rx_ring(void *priv, u8 ring_index, bool use_napi)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct device *dma_dev = NBL_RES_MGT_TO_DMA_DEV(res_mgt);
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_rx_ring *rx_ring = NBL_RES_MGT_TO_RX_RING(res_mgt, ring_index);
	struct nbl_res_vector *vector = NULL;
	struct page_pool_params pp_params = {0};
	int pkt_len_shift = 0;
	int pkt_len = 0, order = 0;
	int dma_size = 0, buf_size = 0;
	int i, j;
	u16 rx_pad, tailroom;

	if (rx_ring->rx_bufs) {
		nbl_err(common, NBL_DEBUG_RESOURCE,
			"Try to setup a RX ring with buffer management array already allocated\n");
		return (dma_addr_t)NULL;
	}

	if (!nbl_txrx_within_vsi(&txrx_mgt->vsi_info[NBL_VSI_XDP], ring_index))
		vector = NBL_RES_MGT_TO_VECTOR(res_mgt, ring_index);

	rx_pad = NBL_RX_PAD;
	tailroom = 0;
	if (rx_ring->xdp_prog) {
		rx_pad = XDP_PACKET_HEADROOM;
		tailroom = SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
	}
	if (!!adaptive_rxbuf_len_disable && !rx_ring->xdp_prog) {
		buf_size = NBL_RX_BUFSZ;
		pkt_len_shift = PAGE_SHIFT - 1;
	} else {
		pkt_len = rx_pad + ETH_HLEN + (VLAN_HLEN * 2) + rx_ring->netdev->mtu +
			tailroom + NBL_BUFFER_HDR_LEN;
		pkt_len_shift = ilog2((pkt_len) - 1) + 1;
		pkt_len_shift = max(pkt_len_shift, NBL_RXBUF_MIN_ORDER);
		buf_size = 1UL << pkt_len_shift;
	}

	if (pkt_len_shift >= PAGE_SHIFT) {
		order = pkt_len_shift - PAGE_SHIFT;
		rx_ring->frags_num_per_page = 1;
	} else {
		order = 0;
		rx_ring->frags_num_per_page = PAGE_SIZE / buf_size;
		WARN_ON(rx_ring->frags_num_per_page > NBL_MAX_BATCH_DESC);
	}
	dma_size = PAGE_SIZE << order;

	rx_ring->buf_len = buf_size - rx_pad - tailroom;

	pp_params.order = order;
	pp_params.flags = 0;
	pp_params.pool_size = rx_ring->desc_num;
	pp_params.nid = dev_to_node(dev);
	pp_params.dev = dev;
	pp_params.dma_dir = DMA_FROM_DEVICE;

	if (dev_to_node(dev) == NUMA_NO_NODE)
		rx_ring->nid = 0;
	else
		rx_ring->nid = dev_to_node(dev);

	rx_ring->page_pool = page_pool_create(&pp_params);
	if (IS_ERR(rx_ring->page_pool)) {
		nbl_err(common, NBL_DEBUG_RESOURCE, "Page_pool Allocate %u Failed failed\n",
			rx_ring->queue_index);
		return (dma_addr_t)NULL;
	}

	rx_ring->di = kvzalloc_node(array_size(rx_ring->desc_num / rx_ring->frags_num_per_page,
					       sizeof(struct nbl_dma_info)),
					       GFP_KERNEL, dev_to_node(dev));
	if (!rx_ring->di) {
		nbl_err(common, NBL_DEBUG_RESOURCE, "Dma info Allocate %u Failed failed\n",
			rx_ring->queue_index);
		goto alloc_di_err;
	}

	rx_ring->rx_bufs = devm_kcalloc(dev, rx_ring->desc_num, sizeof(*rx_ring->rx_bufs),
					GFP_KERNEL);
	if (!rx_ring->rx_bufs)
		goto alloc_buffers_err;

	/* Alloc twice memory, and second half is used to back up the desc for desc checking */
	rx_ring->size = ALIGN(rx_ring->desc_num * sizeof(struct nbl_ring_desc), PAGE_SIZE);
	rx_ring->desc = dmam_alloc_coherent(dma_dev, rx_ring->size, &rx_ring->dma,
					    GFP_KERNEL | __GFP_ZERO);
	if (!rx_ring->desc) {
		nbl_err(common, NBL_DEBUG_RESOURCE,
			"Allocate %u bytes descriptor DMA memory for RX queue %u failed\n",
			rx_ring->size, rx_ring->queue_index);
		goto alloc_dma_err;
	}

	rx_ring->next_to_use = 0;
	rx_ring->next_to_clean = 0;
	rx_ring->tail_ptr = 0;

	j = 0;
	for (i = 0; i < rx_ring->desc_num / rx_ring->frags_num_per_page; i++) {
		struct nbl_dma_info *di = &rx_ring->di[i];
		struct nbl_rx_buffer *buffer = &rx_ring->rx_bufs[j];
		int f;

		di->size = dma_size;
		for (f = 0; f < rx_ring->frags_num_per_page; f++, j++) {
			buffer = &rx_ring->rx_bufs[j];
			buffer->di = di;
			buffer->size = buf_size;
			buffer->offset = rx_pad + f * buf_size;
			buffer->rx_pad = rx_pad;
			buffer->last_in_page = false;
		}

		buffer->last_in_page = true;
	}

	if (nbl_alloc_rx_bufs(rx_ring, rx_ring->desc_num - NBL_MAX_BATCH_DESC))
		goto alloc_rx_bufs_err;

	rx_ring->valid = true;
	if (use_napi && vector)
		vector->started = true;

	nbl_debug(common, NBL_DEBUG_RESOURCE, "Start rx ring %d", ring_index);
	return rx_ring->dma;

alloc_rx_bufs_err:
	nbl_free_rx_ring_bufs(rx_ring);
	dmam_free_coherent(dma_dev, rx_ring->size, rx_ring->desc, rx_ring->dma);
	rx_ring->desc = NULL;
	rx_ring->dma = (dma_addr_t)NULL;
alloc_dma_err:
	devm_kfree(dev, rx_ring->rx_bufs);
	rx_ring->rx_bufs = NULL;
alloc_buffers_err:
	kvfree(rx_ring->di);
alloc_di_err:
	page_pool_destroy(rx_ring->page_pool);
	rx_ring->size = 0;
	return (dma_addr_t)NULL;
}

static void nbl_res_txrx_stop_rx_ring(void *priv, u8 ring_index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct device *dev = NBL_RES_MGT_TO_DEV(res_mgt);
	struct device *dma_dev = NBL_RES_MGT_TO_DMA_DEV(res_mgt);
	struct nbl_res_rx_ring *rx_ring = NBL_RES_MGT_TO_RX_RING(res_mgt, ring_index);

	rx_ring->valid = false;

	nbl_free_rx_ring_bufs(rx_ring);
	WRITE_ONCE(NBL_RES_MGT_TO_RX_RING(res_mgt, ring_index), rx_ring);

	devm_kfree(dev, rx_ring->rx_bufs);
	kvfree(rx_ring->di);
	rx_ring->rx_bufs = NULL;

	dmam_free_coherent(dma_dev, rx_ring->size, rx_ring->desc, rx_ring->dma);
	rx_ring->desc = NULL;
	rx_ring->dma = (dma_addr_t)NULL;
	rx_ring->size = 0;

	page_pool_destroy(rx_ring->page_pool);

	nbl_debug(res_mgt->common, NBL_DEBUG_RESOURCE, "Stop rx ring %d", ring_index);
}

static inline bool nbl_ring_desc_used(struct nbl_ring_desc *ring_desc, bool used_wrap_counter)
{
	bool avail;
	bool used;
	u16 flags;

	flags = le16_to_cpu(ring_desc->flags);
	avail = !!(flags & BIT(NBL_PACKED_DESC_F_AVAIL));
	used = !!(flags & BIT(NBL_PACKED_DESC_F_USED));

	return avail == used && used == used_wrap_counter;
}

static inline void nbl_rep_update_tx_stats(struct net_device *netdev, struct nbl_tx_buffer *buffer)
{
	struct nbl_resource_mgt *res_mgt =
				NBL_ADAPTER_TO_RES_MGT(NBL_NETDEV_TO_ADAPTER(netdev));
	struct nbl_eswitch_info *eswitch_info = NBL_RES_MGT_TO_ESWITCH_INFO(res_mgt);
	u16 rep_data_index = 0, rep_vsi_id;

	if (!eswitch_info || eswitch_info->mode != NBL_ESWITCH_OFFLOADS)
		return;

	if (!buffer->skb)
		return;

	rep_vsi_id = *(u16 *)&buffer->skb->cb[NBL_SKB_FILL_VSI_ID_OFF];
	rep_data_index = nbl_res_get_rep_idx(eswitch_info, rep_vsi_id);
	if (rep_data_index >= eswitch_info->num_vfs)
		return;

	if (eswitch_info->rep_data[rep_data_index].rep_vsi_id == rep_vsi_id) {
		u64_stats_update_begin(&eswitch_info->rep_data[rep_data_index].rep_syncp);
		eswitch_info->rep_data[rep_data_index].tx_packets += buffer->gso_segs;
		eswitch_info->rep_data[rep_data_index].tx_bytes += buffer->bytecount;
		u64_stats_update_end(&eswitch_info->rep_data[rep_data_index].rep_syncp);
	}
}

static struct net_device *nbl_get_rep_netdev(struct nbl_resource_mgt *res_mgt, u16 rep_vsi_id)
{
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_eswitch_info *eswitch_info = NBL_RES_MGT_TO_ESWITCH_INFO(res_mgt);
	u16 rep_data_index = 0;

	rep_data_index = nbl_res_get_rep_idx(eswitch_info, rep_vsi_id);
	if (rep_data_index >= eswitch_info->num_vfs)
		return NULL;
	if (eswitch_info->rep_data[rep_data_index].rep_vsi_id == rep_vsi_id)
		return eswitch_info->rep_data[rep_data_index].netdev;
	nbl_info(common, NBL_DEBUG_RESOURCE, "get rep netdev error rep_vsi_id:%d\n", rep_vsi_id);
	return NULL;
}

static inline void nbl_rep_update_rx_stats(struct net_device *netdev,
					   struct sk_buff *skb, u16 sport_id)
{
	struct nbl_resource_mgt *res_mgt =
				NBL_ADAPTER_TO_RES_MGT(NBL_NETDEV_TO_ADAPTER(netdev));
	struct nbl_eswitch_info *eswitch_info = NBL_RES_MGT_TO_ESWITCH_INFO(res_mgt);
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct net_device *rep_netdev = NULL;
	u16 rep_data_index = 0;

	if (!eswitch_info || eswitch_info->mode != NBL_ESWITCH_OFFLOADS)
		return;

	rep_data_index = nbl_res_get_rep_idx(eswitch_info, sport_id);
	if (rep_data_index >= eswitch_info->num_vfs)
		return;

	rep_netdev = nbl_get_rep_netdev(res_mgt, sport_id);
	if (!rep_netdev) {
		/* it's a common case when switchdev mode is opening */
		nbl_info(common, NBL_DEBUG_RESOURCE,
			 "rep update netdev fail. sport_id:%d\n", sport_id);
		return;
	}
	skb->dev = rep_netdev;

	if (eswitch_info->rep_data[rep_data_index].rep_vsi_id == sport_id) {
		u64_stats_update_begin(&eswitch_info->rep_data[rep_data_index].rep_syncp);
		eswitch_info->rep_data[rep_data_index].rx_packets += 1;
		eswitch_info->rep_data[rep_data_index].rx_bytes += skb->len;
		u64_stats_update_end(&eswitch_info->rep_data[rep_data_index].rep_syncp);
	}
}

static int nbl_res_txrx_clean_tx_irq(struct nbl_res_tx_ring *tx_ring)
{
	struct nbl_tx_buffer *tx_buffer;
	struct nbl_ring_desc *tx_desc;
	unsigned int i = tx_ring->next_to_clean;
	unsigned int total_tx_pkts = 0;
	unsigned int total_tx_bytes = 0;
	unsigned int total_tx_descs = 0;
	int count = 64;

	tx_buffer = NBL_TX_BUF(tx_ring, i);
	tx_desc = NBL_TX_DESC(tx_ring, i);
	i -= tx_ring->desc_num;

	do {
		struct nbl_ring_desc *end_desc = tx_buffer->next_to_watch;

		if (!end_desc)
			break;

		/* smp_rmb */
		smp_rmb();

		if (!nbl_ring_desc_used(tx_desc, tx_ring->used_wrap_counter))
			break;

		total_tx_pkts += tx_buffer->gso_segs;
		total_tx_bytes += tx_buffer->bytecount;

		if (nbl_txrx_within_vsi(&tx_ring->vsi_info[NBL_VSI_CTRL], tx_ring->queue_index))
			nbl_rep_update_tx_stats(tx_ring->netdev, tx_buffer);

		while (true) {
			total_tx_descs++;
			nbl_unmap_and_free_tx_resource(tx_ring, tx_buffer, true, true);
			if (tx_desc == end_desc)
				break;
			i++;
			tx_buffer++;
			tx_desc++;
			if (unlikely(!i)) {
				i -= tx_ring->desc_num;
				tx_buffer = NBL_TX_BUF(tx_ring, 0);
				tx_desc = NBL_TX_DESC(tx_ring, 0);
				tx_ring->used_wrap_counter ^= 1;
			}
		}

		tx_buffer++;
		tx_desc++;
		i++;
		if (unlikely(!i)) {
			i -= tx_ring->desc_num;
			tx_buffer = NBL_TX_BUF(tx_ring, 0);
			tx_desc = NBL_TX_DESC(tx_ring, 0);
			tx_ring->used_wrap_counter ^= 1;
		}

		prefetch(tx_desc);

	} while (--count);

	i += tx_ring->desc_num;

	tx_ring->next_to_clean = i;

	u64_stats_update_begin(&tx_ring->syncp);
	tx_ring->stats.bytes += total_tx_bytes;
	tx_ring->stats.packets += total_tx_pkts;
	tx_ring->stats.descs += total_tx_descs;
	u64_stats_update_end(&tx_ring->syncp);
	if (nbl_txrx_within_vsi(&tx_ring->vsi_info[NBL_VSI_DATA], tx_ring->queue_index))
		netdev_tx_completed_queue(txring_txq(tx_ring), total_tx_pkts, total_tx_bytes);

#define TX_WAKE_THRESHOLD (DESC_NEEDED * 2)
	if (unlikely(total_tx_pkts && netif_carrier_ok(tx_ring->netdev) &&
		     nbl_txrx_within_vsi(&tx_ring->vsi_info[NBL_VSI_DATA], tx_ring->queue_index) &&
		     (nbl_unused_tx_desc_count(tx_ring) >= TX_WAKE_THRESHOLD))) {
		/* Make sure that anybody stopping the queue after this
		 * sees the new next_to_clean.
		 */
		smp_mb();

		if (__netif_subqueue_stopped(tx_ring->netdev, tx_ring->queue_index)) {
			netif_wake_subqueue(tx_ring->netdev, tx_ring->queue_index);
			dev_dbg(NBL_RING_TO_DEV(tx_ring), "wake queue %u\n", tx_ring->queue_index);
		}
	}

	return count;
}

static void nbl_rx_csum(struct nbl_res_rx_ring *rx_ring, struct sk_buff *skb,
			struct nbl_rx_extend_head *hdr)
{
	skb->ip_summed = CHECKSUM_NONE;
	skb_checksum_none_assert(skb);

	/* if user disable RX Checksum Offload, then stack verify the rx checksum */
	if (!(rx_ring->netdev->features & NETIF_F_RXCSUM))
		return;

	if (!hdr->checksum_status)
		return;

	if (hdr->error_code) {
		rx_ring->rx_stats.rx_csum_errors++;
		return;
	}

	skb->ip_summed = CHECKSUM_UNNECESSARY;
	rx_ring->rx_stats.rx_csum_packets++;
}

static inline void nbl_add_rx_frag(struct nbl_rx_buffer *rx_buffer,
				   struct sk_buff *skb, unsigned int size)
{
	page_ref_inc(rx_buffer->di->page);
	skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, rx_buffer->di->page,
			rx_buffer->offset, size, rx_buffer->size);
}

#ifdef CONFIG_TLS_DEVICE
static void nbl_resync_update_sn(struct net_device *netdev, struct sk_buff *skb, u16 offset)
{
	struct ethhdr *eth = (struct ethhdr *)(skb->data);
	struct net *net = dev_net(netdev);
	struct sock *sk;
	struct tls_context *tls_ctx;
	struct nbl_ktls_offload_context_rx **ctx;
	struct nbl_ktls_offload_context_rx *priv;
	struct iphdr *iph;
	struct tcphdr *th;
	int depth = 0;
	__be32 seq;

	skb->mac_len = ETH_HLEN;
	(void)__vlan_get_protocol(skb, eth->h_proto, &depth);
	iph = (struct iphdr *)(skb->data + depth);

	if (iph->version == 4) {
		depth += iph->ihl * 4;
		th = (void *)iph + iph->ihl * 4;

		sk = inet_lookup_established(net, net->ipv4.tcp_death_row.hashinfo,
					     iph->saddr, th->source, iph->daddr,
					     th->dest, netdev->ifindex);
	} else {
		struct ipv6hdr *ipv6h = (struct ipv6hdr *)iph;

		depth += sizeof(struct ipv6hdr);
		th = (void *)ipv6h + sizeof(struct ipv6hdr);

		sk = __inet6_lookup_established(net, net->ipv4.tcp_death_row.hashinfo,
						&ipv6h->saddr, th->source,
						&ipv6h->daddr, ntohs(th->dest),
						netdev->ifindex, 0);
	}

	depth += th->doff * 4;
	if (unlikely(!sk))
		return;

	if (unlikely(sk->sk_state == TCP_TIME_WAIT))
		goto unref;
	seq = th->seq;
	seq = htonl(ntohl(seq) + offset - depth - 1);
	tls_offload_rx_resync_request(sk, seq);
	tls_ctx = tls_get_ctx(sk);
	ctx = __tls_driver_ctx(tls_ctx, TLS_OFFLOAD_CTX_DIR_RX);
	priv = *ctx;
	priv->tcp_seq = ntohl(th->seq);

unref:
	sock_gen_put(sk);
}

static int nbl_ktls_rx_handle_skb(struct nbl_res_rx_ring *rx_ring, struct sk_buff *skb,
				  struct nbl_rx_extend_head *hdr)
{
	if (!hdr->l4s_hdl_ind)
		return 0;

	if (hdr->l4s_dec_ind) {
		skb->decrypted = 1;
		rx_ring->rx_stats.tls_decrypted_packets++;
	} else if (hdr->l4s_resync_ind) {
		rx_ring->rx_stats.tls_resync_req_num++;
		nbl_resync_update_sn(rx_ring->netdev, skb, hdr->l4s_tcp_offset);
		dev_dbg(NBL_RING_TO_DEV(rx_ring), "ingress ktls %u resync sn\n", hdr->l4s_sid);
	} else if (!hdr->l4s_check_ind) {
		dev_dbg(NBL_RING_TO_DEV(rx_ring), "ingress ktls %u auth fail\n", hdr->l4s_sid);
	} else {
		dev_err(NBL_RING_TO_DEV(rx_ring), "ingress ktls %u unknown error\n", hdr->l4s_sid);
	}

	return 0;
}

#else
static int nbl_ktls_rx_handle_skb(struct nbl_res_rx_ring *rx_ring, struct sk_buff *skb,
				  struct nbl_rx_extend_head *hdr)
{
	return 0;
}
#endif

static inline int nbl_rx_vlan_pop(struct nbl_res_rx_ring *rx_ring, struct sk_buff *skb)
{
	struct vlan_ethhdr *veth = (struct vlan_ethhdr *)skb->data;

	if (!rx_ring->vlan_proto)
		return 0;

	if (rx_ring->vlan_proto != ntohs(veth->h_vlan_proto) ||
	    (rx_ring->vlan_tci & VLAN_VID_MASK) != (ntohs(veth->h_vlan_TCI) & VLAN_VID_MASK))
		return 1;

	memmove(skb->data + VLAN_HLEN, skb->data, 2 * ETH_ALEN);
	__skb_pull(skb, VLAN_HLEN);

	return 0;
}

static void nbl_txrx_register_vsi_ring(void *priv, u16 vsi_index, u16 ring_offset, u16 ring_num)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);

	txrx_mgt->vsi_info[vsi_index].ring_offset = ring_offset;
	txrx_mgt->vsi_info[vsi_index].ring_num = ring_num;
}

static void nbl_res_txrx_cfg_txrx_vlan(void *priv, u16 vlan_tci, u16 vlan_proto, u8 vsi_index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_txrx_vsi_info *vsi_info = &txrx_mgt->vsi_info[vsi_index];
	struct nbl_res_tx_ring *tx_ring;
	struct nbl_res_rx_ring *rx_ring;
	u16 i;

	if (!txrx_mgt->tx_rings || !txrx_mgt->rx_rings)
		return;

	for (i = vsi_info->ring_offset; i < vsi_info->ring_offset + vsi_info->ring_num; i++) {
		tx_ring = txrx_mgt->tx_rings[i];
		rx_ring = txrx_mgt->rx_rings[i];

		if (tx_ring) {
			tx_ring->vlan_tci = vlan_tci;
			tx_ring->vlan_proto = vlan_proto;
		}

		if (rx_ring) {
			rx_ring->vlan_tci = vlan_tci;
			rx_ring->vlan_proto = vlan_proto;
		}
	}
}

/**
 * Current version support merging multiple descriptor for one packet.
 */
static struct sk_buff *nbl_construct_skb(struct nbl_res_rx_ring *rx_ring, struct napi_struct *napi,
					 struct nbl_rx_buffer *rx_buf, struct xdp_buff *xdp)
{
	struct sk_buff *skb;
	int tailroom, shinfo_size = SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
	unsigned int truesize = rx_buf->size;
	unsigned int headlen;
	unsigned int size = xdp->data_end - xdp->data;
	u8 metasize = xdp->data - xdp->data_meta;

	tailroom = truesize - size - rx_buf->rx_pad - NBL_BUFFER_HDR_LEN;

	if (size > NBL_RX_HDR_SIZE && tailroom >= shinfo_size) {
		skb = build_skb(xdp->data_hard_start, truesize);
		if (unlikely(!skb))
			return NULL;

		page_ref_inc(rx_buf->di->page);
		skb_reserve(skb, xdp->data - xdp->data_hard_start);
		skb_put(skb, xdp->data_end - xdp->data);
		if (metasize)
			skb_metadata_set(skb, metasize);
		goto ok;
	}

	skb = napi_alloc_skb(napi, NBL_RX_HDR_SIZE);
	if (unlikely(!skb))
		return NULL;

	headlen = size;
	if (headlen > NBL_RX_HDR_SIZE)
		headlen = eth_get_headlen(skb->dev, xdp->data, NBL_RX_HDR_SIZE);

	memcpy(__skb_put(skb, headlen), xdp->data, ALIGN(headlen, sizeof(long)));
	size -= headlen;
	if (size) {
		page_ref_inc(rx_buf->di->page);
		skb_add_rx_frag(skb, 0, rx_buf->di->page,
				rx_buf->offset + NBL_BUFFER_HDR_LEN + headlen,
				size, truesize);
	}
ok:
	skb_record_rx_queue(skb, rx_ring->queue_index);

	return skb;
}

static inline struct nbl_rx_buffer *nbl_get_rx_buf(struct nbl_res_rx_ring *rx_ring)
{
	struct nbl_rx_buffer *rx_buf;

	rx_buf = NBL_RX_BUF(rx_ring, rx_ring->next_to_clean);
	prefetchw(rx_buf->di->page);

	dma_sync_single_range_for_cpu(rx_ring->dma_dev, rx_buf->di->addr, rx_buf->offset,
				      rx_ring->buf_len, DMA_FROM_DEVICE);

	return rx_buf;
}

static inline void nbl_put_rx_buf(struct nbl_res_rx_ring *rx_ring, struct nbl_rx_buffer *rx_buf)
{
	u16 ntc = rx_ring->next_to_clean + 1;

	/* if at the end of the ring, reset ntc and flip used wrap bit */
	if (unlikely(ntc >= rx_ring->desc_num)) {
		ntc = 0;
		rx_ring->used_wrap_counter ^= 1;
	}

	rx_ring->next_to_clean = ntc;
	prefetch(NBL_RX_DESC(rx_ring, ntc));

	nbl_put_rx_frag(rx_ring, rx_buf, true);
}

static inline int nbl_maybe_stop_tx(struct nbl_res_tx_ring *tx_ring, unsigned int size)
{
	if (likely(nbl_unused_tx_desc_count(tx_ring) >= size))
		return 0;

	if (!nbl_txrx_within_vsi(&tx_ring->vsi_info[NBL_VSI_DATA], tx_ring->queue_index))
		return -EBUSY;

	dev_dbg(NBL_RING_TO_DEV(tx_ring), "unused_desc_count:%u, size:%u, stop queue %u\n",
		nbl_unused_tx_desc_count(tx_ring), size, tx_ring->queue_index);
	netif_stop_subqueue(tx_ring->netdev, tx_ring->queue_index);

	/* smp_mb */
	smp_mb();

	if (likely(nbl_unused_tx_desc_count(tx_ring) < size))
		return -EBUSY;

	dev_dbg(NBL_RING_TO_DEV(tx_ring), "unused_desc_count:%u, size:%u, start queue %u\n",
		nbl_unused_tx_desc_count(tx_ring), size, tx_ring->queue_index);
	netif_start_subqueue(tx_ring->netdev, tx_ring->queue_index);

	return 0;
}

static int nbl_res_txrx_xmit_xdp_ring(struct nbl_res_tx_ring *xdp_ring, struct xdp_frame *xdpf)
{
	u16 index  = xdp_ring->next_to_use;
	u16 avail_used_flags = xdp_ring->avail_used_flags;
	unsigned int size;
	dma_addr_t dma;
	union nbl_tx_extend_head *hdr;
	struct device *dma_dev = NBL_RING_TO_DMA_DEV(xdp_ring);
	struct nbl_tx_buffer *tx_buffer = NBL_TX_BUF(xdp_ring, index);
	struct nbl_ring_desc *tx_desc = NBL_TX_DESC(xdp_ring, index);
	const struct ethhdr *eth;

	if (xdpf->headroom < sizeof(union nbl_tx_extend_head))
		return -EOVERFLOW;

	if (unlikely(nbl_maybe_stop_tx(xdp_ring, 1))) {
		xdp_ring->tx_stats.tx_busy++;
		return NETDEV_TX_BUSY;
	}

	size = xdpf->len;
	eth = (struct ethhdr *)xdpf->data;
	xdpf->headroom -= sizeof(union nbl_tx_extend_head);
	xdpf->data -= sizeof(union nbl_tx_extend_head);
	hdr = xdpf->data;
	memset(hdr, 0, sizeof(union nbl_tx_extend_head));
	hdr->fwd = NBL_TX_FWD_TYPE_NORMAL;
	xdpf->len += sizeof(union nbl_tx_extend_head);
	dma = dma_map_single(dma_dev, xdpf->data, xdpf->len, DMA_TO_DEVICE);
	if (dma_mapping_error(dma_dev, dma)) {
		xdp_ring->tx_stats.tx_dma_busy++;
		return NETDEV_TX_BUSY;
	}

	dma_unmap_addr_set(tx_buffer, dma, dma);
	dma_unmap_len_set(tx_buffer, len, xdpf->len);
	tx_buffer->raw_buff = xdpf->data;
	tx_buffer->gso_segs = 1;
	tx_buffer->bytecount = size;
	tx_desc->addr = cpu_to_le64(dma);
	tx_desc->len = xdpf->len;
	tx_desc->id = 0;
	index++;
	if (index == xdp_ring->desc_num) {
		index = 0;
		xdp_ring->avail_used_flags ^=
			1 << NBL_PACKED_DESC_F_AVAIL |
			1 << NBL_PACKED_DESC_F_USED;
	}

	/* todo:xdp add multicast case */
	xdp_ring->tx_stats.tx_unicast_packets++;
	tx_buffer->next_to_watch = tx_desc;

	/* wmb */
	wmb();

	xdp_ring->next_to_use = index;
	tx_desc->flags = cpu_to_le16(avail_used_flags);

	return NETDEV_TX_OK;
}

static int nbl_res_txrx_xmit_xdp_buff(struct nbl_res_rx_ring *rx_ring, struct xdp_buff *xdp_buff)
{
	int ret;
	struct nbl_res_tx_ring *xdp_ring;
	struct xdp_frame *xdpf;
	struct nbl_txrx_mgt *txrx_mgt = rx_ring->txrx_mgt;

	xdpf = xdp_convert_buff_to_frame(xdp_buff);
	if (unlikely(!xdpf))
		goto buff_to_frame_failed;

	xdp_ring = nbl_res_txrx_select_xdp_ring(txrx_mgt);
	if (static_branch_unlikely(&nbl_xdp_locking_key))
		spin_lock(&xdp_ring->xmit_lock);

	ret = nbl_res_txrx_xmit_xdp_ring(xdp_ring, xdpf);
	if (static_branch_unlikely(&nbl_xdp_locking_key))
		spin_unlock(&xdp_ring->xmit_lock);

	return ret;
buff_to_frame_failed:
	return -1;
}

static int
nbl_res_txrx_run_xdp(struct nbl_res_rx_ring *rx_ring, struct nbl_rx_buffer *rx_buf,
		     struct nbl_xdp_output *xdp_output, struct xdp_buff *xdp_buff)
{
	struct nbl_rx_extend_head *hdr;
	struct nbl_ring_desc *rx_desc;
	const struct ethhdr *eth;
	int i;
	int err;
	enum xdp_action act;
	int nbl_act;
	u16 num_buffers = 0;

	hdr = xdp_buff->data - NBL_BUFFER_HDR_LEN;
	net_prefetch(hdr);
	num_buffers = le16_to_cpu(hdr->num_buffers);

	/* receive xdp only support one desc for one packet */
	if (num_buffers > 1)
		goto drop_big_packet;

	xdp_output->bytes = xdp_buff->data_end - xdp_buff->data;
	eth = (struct ethhdr *)(hdr + 1);
	if (unlikely(is_multicast_ether_addr(eth->h_dest)))
		xdp_output->flags |= NBL_XDP_FLAG_MULTICAST;

	xdp_output->desc_done_num++;
	xdp_init_buff(xdp_buff, rx_buf->size, &rx_ring->xdp_rxq);
	act = bpf_prog_run_xdp(rx_ring->xdp_prog, xdp_buff);
	switch (act) {
	case XDP_PASS:
		nbl_act = 0;
		break;
	case XDP_TX:
		nbl_act = 1;
		page_ref_inc(rx_buf->di->page);
		err = nbl_res_txrx_xmit_xdp_buff(rx_ring, xdp_buff);
		if (unlikely(err)) {
			page_ref_dec(rx_buf->di->page);
			goto xdp_aborted;
		}

		xdp_output->flags |= NBL_XDP_FLAG_TX;
		break;
	case XDP_REDIRECT:
		nbl_act = 1;
		page_ref_inc(rx_buf->di->page);
		err = xdp_do_redirect(rx_ring->netdev, xdp_buff, rx_ring->xdp_prog);
		if (unlikely(err)) {
			page_ref_dec(rx_buf->di->page);
			goto xdp_aborted;
		}

		xdp_output->flags |= NBL_XDP_FLAG_REDIRECT;
		break;
	default:
		bpf_warn_invalid_xdp_action(rx_ring->netdev, rx_ring->xdp_prog, act);
		fallthrough;
	case XDP_ABORTED:
xdp_aborted:
		trace_xdp_exception(rx_ring->netdev, rx_ring->xdp_prog, act);
		fallthrough;
	case XDP_DROP:
		xdp_output->flags |= NBL_XDP_FLAG_DROP;
		nbl_act = 1;
		break;
	}

	if (nbl_act)
		nbl_put_rx_buf(rx_ring, rx_buf);

	return nbl_act;

drop_big_packet:
	nbl_put_rx_buf(rx_ring, rx_buf);
	xdp_output->desc_done_num++;
	xdp_output->flags |= NBL_XDP_FLAG_OVERSIZE;
	for (i = 1; i < num_buffers; i++) {
		rx_desc = NBL_RX_DESC(rx_ring, rx_ring->next_to_clean);
		if (!nbl_ring_desc_used(rx_desc, rx_ring->used_wrap_counter))
			break;

		dma_rmb();
		xdp_output->bytes += le32_to_cpu(rx_desc->len);
		xdp_output->desc_done_num++;
		rx_buf = nbl_get_rx_buf(rx_ring);
		nbl_put_rx_buf(rx_ring, rx_buf);
	}

	return 1;
}

static int
nbl_res_txrx_xdp_xmit(struct net_device *netdev, int n, struct xdp_frame **frame, u32 flags)
{
	int ret;
	int i;
	int nxmit = 0;
	struct nbl_res_tx_ring *xdp_ring;
	struct nbl_resource_mgt *res_mgt =
				NBL_ADAPTER_TO_RES_MGT(NBL_NETDEV_TO_ADAPTER(netdev));
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);

	if (unlikely(flags & ~XDP_XMIT_FLAGS_MASK))
		return -EINVAL;

	xdp_ring = nbl_res_txrx_select_xdp_ring(txrx_mgt);
	if (unlikely(!xdp_ring))
		return -ENXIO;

	if (unlikely(!xdp_ring->valid))
		return -ENETDOWN;

	if (unlikely(!nbl_res_txrx_is_xdp_ring(xdp_ring)))
		return -ENXIO;

	if (static_branch_unlikely(&nbl_xdp_locking_key))
		spin_lock(&xdp_ring->xmit_lock);

	for (i = 0; i < n; i++) {
		ret = nbl_res_txrx_xmit_xdp_ring(xdp_ring, frame[i]);
		if (ret)
			break;

		nxmit++;
	}

	if (unlikely(flags & XDP_XMIT_FLUSH && nxmit))
		writel(xdp_ring->notify_qid, xdp_ring->notify_addr);

	if (static_branch_unlikely(&nbl_xdp_locking_key))
		spin_unlock(&xdp_ring->xmit_lock);

	return nxmit;
}

static void
nbl_res_txrx_update_xdp_tail_locked(struct nbl_res_rx_ring *rx_ring)
{
	struct nbl_res_tx_ring *xdp_ring;
	struct nbl_txrx_mgt *txrx_mgt = rx_ring->txrx_mgt;

	xdp_ring = nbl_res_txrx_select_xdp_ring(txrx_mgt);
	if (static_branch_unlikely(&nbl_xdp_locking_key))
		spin_lock(&xdp_ring->xmit_lock);

	writel(xdp_ring->notify_qid, xdp_ring->notify_addr);

	if (static_branch_unlikely(&nbl_xdp_locking_key))
		spin_unlock(&xdp_ring->xmit_lock);
}

static int nbl_res_txrx_register_xdp_rxq(void *priv, u8 ring_index)
{
	int err;
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_res_vector *vector = NBL_RES_MGT_TO_VECTOR(res_mgt, ring_index);
	struct nbl_res_rx_ring *rx_ring = NBL_RES_MGT_TO_RX_RING(res_mgt, ring_index);

	err = xdp_rxq_info_reg(&rx_ring->xdp_rxq, rx_ring->netdev, rx_ring->queue_index,
			       vector->nbl_napi.napi.napi_id);
	if (err < 0) {
		nbl_err(common, NBL_DEBUG_RESOURCE, "Register xdp rxq err\n");
		return -1;
	}

	err = xdp_rxq_info_reg_mem_model(&rx_ring->xdp_rxq, MEM_TYPE_PAGE_SHARED, NULL);
	if (err < 0) {
		nbl_err(common, NBL_DEBUG_RESOURCE, "Register xdp rxq mem model err\n");
		xdp_rxq_info_unreg(&rx_ring->xdp_rxq);
		return -1;
	}

	return 0;
}

static void nbl_res_txrx_unregister_xdp_rxq(void *priv, u8 ring_index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_res_rx_ring *rx_ring = NBL_RES_MGT_TO_RX_RING(res_mgt, ring_index);

	xdp_rxq_info_unreg(&rx_ring->xdp_rxq);
}

static inline void nbl_res_txrx_build_xdp_buff(struct nbl_rx_buffer *rx_buf,
					       struct nbl_ring_desc *rx_desc,
					       struct xdp_buff *xdp)
{
	char *p, *buf;
	u32 size;

	p = page_address(rx_buf->di->page) + rx_buf->offset;
	buf = p - rx_buf->rx_pad;
	size = rx_desc->len - NBL_BUFFER_HDR_LEN;
	xdp_prepare_buff(xdp, buf, rx_buf->rx_pad + NBL_BUFFER_HDR_LEN, size, true);
}

static int nbl_res_txrx_clean_rx_irq(struct nbl_res_rx_ring *rx_ring,
				     struct napi_struct *napi,
				     int budget)
{
	struct nbl_xdp_output xdp_output;
	struct xdp_buff xdp;
	struct nbl_ring_desc *rx_desc;
	struct nbl_rx_buffer *rx_buf;
	struct nbl_rx_extend_head *hdr;
	struct sk_buff *skb = NULL;
	unsigned int total_rx_pkts = 0;
	unsigned int total_rx_bytes = 0;
	unsigned int xdp_tx_pkts = 0;
	unsigned int xdp_redirect_pkts = 0;
	unsigned int xdp_oversize = 0;
	unsigned int xdp_drop = 0;
	unsigned int size;
	int nbl_act;
	u32 rx_multicast_packets = 0;
	u32 rx_unicast_packets = 0;
	u16 desc_count = 0;
	u16 num_buffers = 0;
	u16 cleaned_count = nbl_unused_rx_desc_count(rx_ring);
	u16 sport_id, sport_type;
	bool failure = 0;
	bool drop = 0;

	while (likely(total_rx_pkts < budget)) {
		rx_desc = NBL_RX_DESC(rx_ring, rx_ring->next_to_clean);
		if (!nbl_ring_desc_used(rx_desc, rx_ring->used_wrap_counter))
			break;

		// nbl_trace(clean_rx_irq, rx_ring, rx_desc);

		dma_rmb();
		size = le32_to_cpu(rx_desc->len);
		rx_buf = nbl_get_rx_buf(rx_ring);

		nbl_res_txrx_build_xdp_buff(rx_buf, rx_desc, &xdp);

		if (READ_ONCE(rx_ring->xdp_prog)) {
			memset(&xdp_output, 0, sizeof(xdp_output));
			nbl_act = nbl_res_txrx_run_xdp(rx_ring, rx_buf, &xdp_output, &xdp);
			if (nbl_act) {
				cleaned_count += xdp_output.desc_done_num;
				if (unlikely(xdp_output.flags & NBL_XDP_FLAG_MULTICAST))
					rx_multicast_packets++;
				else
					rx_unicast_packets++;

				xdp_tx_pkts += !!(xdp_output.flags & NBL_XDP_FLAG_TX);
				xdp_redirect_pkts += !!(xdp_output.flags & NBL_XDP_FLAG_REDIRECT);
				xdp_drop += !!(xdp_output.flags & NBL_XDP_FLAG_DROP);
				xdp_oversize += !!(xdp_output.flags & NBL_XDP_FLAG_OVERSIZE);

				total_rx_pkts++;
				total_rx_bytes += xdp_output.bytes;
				continue;
			}
		}

		desc_count++;

		if (skb) {
			nbl_add_rx_frag(rx_buf, skb, size);
		} else {
			hdr = page_address(rx_buf->di->page) + rx_buf->offset;
			net_prefetch(hdr);
			skb = nbl_construct_skb(rx_ring, napi, rx_buf, &xdp);
			if (unlikely(!skb)) {
				rx_ring->rx_stats.rx_alloc_buf_err_cnt++;
				break;
			}

			num_buffers = le16_to_cpu(hdr->num_buffers);
			sport_id = hdr->sport_id;
			sport_type = hdr->sport;
			nbl_rx_csum(rx_ring, skb, hdr);
			nbl_ktls_rx_handle_skb(rx_ring, skb, hdr);
			drop = nbl_rx_vlan_pop(rx_ring, skb);
		}

		cleaned_count++;
		nbl_put_rx_buf(rx_ring, rx_buf);
		if (desc_count < num_buffers)
			continue;
		desc_count = 0;

		if (unlikely(eth_skb_pad(skb))) {
			skb = NULL;
			drop = 0;
			continue;
		}

		if (unlikely(drop)) {
			kfree(skb);
			skb = NULL;
			drop = 0;
			continue;
		}

		total_rx_bytes += skb->len;
		skb->protocol = eth_type_trans(skb, rx_ring->netdev);
		if (unlikely(skb->pkt_type == PACKET_BROADCAST ||
			     skb->pkt_type == PACKET_MULTICAST))
			rx_multicast_packets++;
		else
			rx_unicast_packets++;

		if (sport_type)
			nbl_rep_update_rx_stats(rx_ring->netdev, skb, sport_id);

		// nbl_trace(clean_rx_irq_indicate, rx_ring, rx_desc, skb);
		napi_gro_receive(napi, skb);
		skb = NULL;
		drop = 0;
		total_rx_pkts++;
	}

	if (xdp_redirect_pkts)
		xdp_do_flush();

	if (xdp_tx_pkts)
		nbl_res_txrx_update_xdp_tail_locked(rx_ring);
	if (cleaned_count & (~(NBL_MAX_BATCH_DESC - 1)))
		failure = nbl_alloc_rx_bufs(rx_ring, cleaned_count & (~(NBL_MAX_BATCH_DESC - 1)));

	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += total_rx_pkts;
	rx_ring->stats.bytes += total_rx_bytes;
	rx_ring->rx_stats.rx_multicast_packets += rx_multicast_packets;
	rx_ring->rx_stats.rx_unicast_packets += rx_unicast_packets;
	rx_ring->rx_stats.xdp_tx_packets += xdp_tx_pkts;
	rx_ring->rx_stats.xdp_redirect_packets += xdp_redirect_pkts;
	rx_ring->rx_stats.xdp_oversize_packets += xdp_oversize;
	rx_ring->rx_stats.xdp_drop_packets += xdp_drop;
	u64_stats_update_end(&rx_ring->syncp);

	return failure ? budget : total_rx_pkts;
}

static int nbl_res_napi_poll(struct napi_struct *napi, int budget)
{
	struct nbl_napi_struct *nbl_napi = container_of(napi, struct nbl_napi_struct, napi);
	struct nbl_res_vector *vector = container_of(nbl_napi, struct nbl_res_vector, nbl_napi);
	struct nbl_res_tx_ring *tx_ring;
	struct nbl_res_tx_ring *xdp_ring;
	struct nbl_res_rx_ring *rx_ring;
	int complete = 1, cleaned = 0, tx_done = 1, xdp_done = 1;

	tx_ring = vector->tx_ring;
	rx_ring = vector->rx_ring;
	xdp_ring = vector->xdp_ring;

	if (vector->started) {
		tx_done = nbl_res_txrx_clean_tx_irq(tx_ring);
		if (xdp_ring && xdp_ring->valid)
			xdp_done = nbl_res_txrx_clean_tx_irq(xdp_ring);

		cleaned = nbl_res_txrx_clean_rx_irq(rx_ring, napi, budget);
	}

	if (!tx_done || !xdp_done)
		complete = 0;

	if (cleaned >= budget)
		complete = 0;

	if (!complete)
		return budget;

	if (!napi_complete_done(napi, cleaned))
		return min_t(int, cleaned, budget - 1);

	/* unmask irq passthrough for performace */
	if (vector->net_msix_mask_en)
		writel(vector->irq_data, vector->irq_enable_base);

	return min_t(int, cleaned, budget - 1);
}

static inline unsigned int nbl_txd_use_count(unsigned int size)
{
	/* TODO: how to compute tx desc needed more efficiently */
	return DIV_ROUND_UP(size, NBL_TXD_DATALEN_MAX);
}

static unsigned int nbl_xmit_desc_count(struct sk_buff *skb)
{
	const skb_frag_t *frag = &skb_shinfo(skb)->frags[0];
	unsigned int nr_frags = skb_shinfo(skb)->nr_frags;
	unsigned int size;
	unsigned int count;

	/* We need: 1 descriptor per page * PAGE_SIZE/NBL_MAX_DATA_PER_TX_DESC,
	 *          + 1 desc for skb_headlen/NBL_MAX_DATA_PER_TX_DESC,
	 *          + 2 desc gap to keep tail from touching head,
	 * otherwise try next time.
	 */
	size = skb_headlen(skb);
	count = 2;
	for (;;) {
		count += nbl_txd_use_count(size);

		if (!nr_frags--)
			break;

		size = skb_frag_size(frag++);
	}

	return count;
}

/* set up TSO(TCP Segmentation Offload) */
static int nbl_tx_tso(struct nbl_tx_buffer *first, struct nbl_tx_hdr_param *hdr_param)
{
	struct sk_buff *skb = first->skb;
	union {
		struct iphdr *v4;
		struct ipv6hdr *v6;
		unsigned char *hdr;
	} ip;
	union {
		struct tcphdr *tcp;
		struct udphdr *udp;
		unsigned char *hdr;
	} l4;
	u8 l4_start;
	u32 payload_len;
	u8 header_len = 0;
	int err;

	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return 1;

	if (!skb_is_gso(skb))
		return 1;

	err = skb_cow_head(skb, 0);
	if (err < 0)
		return err;

	ip.hdr = skb_network_header(skb);
	l4.hdr = skb_transport_header(skb);

	/* initialize IP header fields*/
	if (ip.v4->version == IP_VERSION_V4) {
		ip.v4->tot_len = 0;
		ip.v4->check = 0;
	} else {
		ip.v6->payload_len = 0;
	}

	/* length of (MAC + IP) header */
	l4_start = (u8)(l4.hdr - skb->data);

	/* l4 packet length */
	payload_len = skb->len - l4_start;

	/* remove l4 packet length from L4 pseudo-header checksum */
	if (skb_shinfo(skb)->gso_type & SKB_GSO_UDP_L4) {
		csum_replace_by_diff(&l4.udp->check, (__force __wsum)htonl(payload_len));
		/* compute length of UDP segmentation header */
		header_len = (u8)sizeof(l4.udp) + l4_start;
	} else {
		csum_replace_by_diff(&l4.tcp->check, (__force __wsum)htonl(payload_len));
		/* compute length of TCP segmentation header */
		header_len = (u8)(l4.tcp->doff * 4 + l4_start);
	}

	hdr_param->tso = 1;
	hdr_param->mss = skb_shinfo(skb)->gso_size;
	hdr_param->total_hlen = header_len;

	first->gso_segs = skb_shinfo(skb)->gso_segs;
	first->bytecount += (first->gso_segs - 1) * header_len;
	first->tx_flags = NBL_TX_FLAGS_TSO;

	return first->gso_segs;
}

/* set up Tx checksum offload */
static int nbl_tx_csum(struct nbl_tx_buffer *first, struct nbl_tx_hdr_param *hdr_param)
{
	struct sk_buff *skb = first->skb;
	union {
		struct iphdr *v4;
		struct ipv6hdr *v6;
		unsigned char *hdr;
	} ip;
	union {
		struct tcphdr *tcp;
		struct udphdr *udp;
		unsigned char *hdr;
	} l4;
	__be16 frag_off, protocol;
	u8 inner_ip_type = 0, l4_type = 0, l4_csum = 0, l4_proto = 0;
	u32 l2_len = 0, l3_len = 0, l4_len = 0;
	unsigned char *exthdr;
	int ret;

	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return 0;

	ip.hdr = skb_network_header(skb);
	l4.hdr = skb_transport_header(skb);

	/* compute outer L2 header size */
	l2_len = ip.hdr - skb->data;

	protocol = vlan_get_protocol(skb);

	if (protocol == htons(ETH_P_IP)) {
		inner_ip_type = NBL_TX_IIPT_IPV4;
		l4_proto = ip.v4->protocol;
	} else if (protocol == htons(ETH_P_IPV6)) {
		inner_ip_type = NBL_TX_IIPT_IPV6;
		exthdr = ip.hdr + sizeof(*ip.v6);
		l4_proto = ip.v6->nexthdr;

		if (l4.hdr != exthdr) {
			ret = ipv6_skip_exthdr(skb, exthdr - skb->data, &l4_proto, &frag_off);
			if (ret < 0)
				return -1;
		}
	} else {
		return -1;
	}

	l3_len = l4.hdr - ip.hdr;

	switch (l4_proto) {
	case IPPROTO_TCP:
		l4_type = NBL_TX_L4T_TCP;
		l4_len = l4.tcp->doff;
		l4_csum = 1;
		break;
	case IPPROTO_UDP:
		l4_type = NBL_TX_L4T_UDP;
		l4_len = (sizeof(struct udphdr) >> 2);
		l4_csum = 1;
		break;
	case IPPROTO_SCTP:
		if (first->tx_flags & NBL_TX_FLAGS_TSO)
			return -1;
		l4_type = NBL_TX_L4T_RSV;
		l4_len = (sizeof(struct sctphdr) >> 2);
		l4_csum = 1;
		break;
	default:
		if (first->tx_flags & NBL_TX_FLAGS_TSO)
			return -2;

		/* unsopported L4 protocol, device cannot offload L4 checksum,
		 * so software compute L4 checskum
		 */
		skb_checksum_help(skb);
		return 0;
	}

	hdr_param->mac_len = l2_len >> 1;
	hdr_param->ip_len = l3_len >> 2;
	hdr_param->l4_len = l4_len;
	hdr_param->l4_type = l4_type;
	hdr_param->inner_ip_type = inner_ip_type;
	hdr_param->l3_csum_en = 0;
	hdr_param->l4_csum_en = l4_csum;

	return 1;
}

static int nbl_map_skb(struct nbl_res_tx_ring *tx_ring, struct sk_buff *skb,
		       u16 first, u16 *desc_index)
{
	u16 index = *desc_index;
	const skb_frag_t *frag;
	unsigned int frag_num = skb_shinfo(skb)->nr_frags;
	struct device *dma_dev = NBL_RING_TO_DMA_DEV(tx_ring);
	struct nbl_tx_buffer *tx_buffer = NBL_TX_BUF(tx_ring, index);
	struct nbl_ring_desc *tx_desc = NBL_TX_DESC(tx_ring, index);
	unsigned int i;
	unsigned int size;
	dma_addr_t dma;

	size = skb_headlen(skb);
	dma = dma_map_single(dma_dev, skb->data, size, DMA_TO_DEVICE);
	if (dma_mapping_error(dma_dev, dma))
		return -1;

	tx_buffer->dma = dma;
	tx_buffer->len = size;

	tx_desc->addr = cpu_to_le64(dma);
	tx_desc->len = size;
	if (!first)
		tx_desc->flags = cpu_to_le16(tx_ring->avail_used_flags | NBL_PACKED_DESC_F_NEXT);

	index++;
	tx_desc++;
	tx_buffer++;
	if (index == tx_ring->desc_num) {
		index = 0;
		tx_ring->avail_used_flags ^=
			1 << NBL_PACKED_DESC_F_AVAIL |
			1 << NBL_PACKED_DESC_F_USED;
		tx_desc = NBL_TX_DESC(tx_ring, 0);
		tx_buffer = NBL_TX_BUF(tx_ring, 0);
	}

	if (!frag_num) {
		*desc_index = index;
		return 0;
	}

	frag = &skb_shinfo(skb)->frags[0];
	for (i = 0; i < frag_num; i++) {
		size = skb_frag_size(frag);
		dma = skb_frag_dma_map(dma_dev, frag, 0, size, DMA_TO_DEVICE);
		if (dma_mapping_error(dma_dev, dma)) {
			*desc_index = index;
			return -1;
		}

		tx_buffer->dma = dma;
		tx_buffer->len = size;
		tx_buffer->page = 1;

		tx_desc->addr = cpu_to_le64(dma);
		tx_desc->len = size;
		tx_desc->flags = cpu_to_le16(tx_ring->avail_used_flags | NBL_PACKED_DESC_F_NEXT);
		index++;
		tx_desc++;
		tx_buffer++;
		if (index == tx_ring->desc_num) {
			index = 0;
			tx_ring->avail_used_flags ^=
				1 << NBL_PACKED_DESC_F_AVAIL |
				1 << NBL_PACKED_DESC_F_USED;
			tx_desc = NBL_TX_DESC(tx_ring, 0);
			tx_buffer = NBL_TX_BUF(tx_ring, 0);
		}
		frag++;
	}

	*desc_index = index;
	return 0;
}

static inline void nbl_tx_fill_tx_extend_header_bootis(union nbl_tx_extend_head *pkthdr,
						       struct nbl_tx_hdr_param *param)
{
	pkthdr->bootis.tso = param->tso;
	pkthdr->bootis.mss = param->mss;
	pkthdr->bootis.dport_info = 0;
	pkthdr->bootis.dport_id = param->dport_id;
	pkthdr->bootis.dport = NBL_TX_DPORT_ETH;
	/* 0x0: drop, 0x1: normal fwd, 0x2: rsv, 0x3: cpu set dport */
	pkthdr->bootis.fwd = NBL_TX_FWD_TYPE_CPU_ASSIGNED;
	pkthdr->bootis.rss_lag_en = param->rss_lag_en;

	pkthdr->bootis.mac_len = param->mac_len;
	pkthdr->bootis.ip_len = param->ip_len;
	pkthdr->bootis.inner_ip_type = param->inner_ip_type;
	pkthdr->bootis.l3_csum_en = param->l3_csum_en;

	pkthdr->bootis.l4_len = param->l4_len;
	pkthdr->bootis.l4_type = param->l4_type;
	pkthdr->bootis.l4_csum_en = param->l4_csum_en;
}

static inline void nbl_tx_fill_tx_extend_header_leonis(union nbl_tx_extend_head *pkthdr,
						       struct nbl_tx_hdr_param *param)
{
	pkthdr->mac_len = param->mac_len;
	pkthdr->ip_len = param->ip_len;
	pkthdr->l4_len = param->l4_len;
	pkthdr->l4_type = param->l4_type;
	pkthdr->inner_ip_type = param->inner_ip_type;

	pkthdr->l4s_sid = param->l4s_sid;
	pkthdr->l4s_sync_ind = param->l4s_sync_ind;
	pkthdr->l4s_hdl_ind = param->l4s_hdl_ind;
	pkthdr->l4s_pbrac_mode = param->l4s_pbrac_mode;

	pkthdr->mss = param->mss;
	pkthdr->tso = param->tso;

	pkthdr->fwd = param->fwd;
	pkthdr->rss_lag_en = param->rss_lag_en;
	pkthdr->dport = param->dport;
	pkthdr->dport_id = param->dport_id;

	pkthdr->l3_csum_en = param->l3_csum_en;
	pkthdr->l4_csum_en = param->l4_csum_en;
}

#ifdef CONFIG_TLS_DEVICE
static bool nbl_ktls_send_init_packet(struct nbl_resource_mgt *res_mgt,
				      struct nbl_res_tx_ring *tx_ring,
				      struct nbl_ktls_offload_context_tx *priv_tx)
{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct device *dma_dev = NBL_RES_MGT_TO_DMA_DEV(res_mgt);
	struct nbl_tx_buffer *first;
	struct nbl_ring_desc *first_desc;
	struct nbl_ktls_init_packet *init_packet;
	struct nbl_notify_param notify_param = {0};
	dma_addr_t hdrdma;
	u16 avail_used_flags = tx_ring->avail_used_flags;
	u16 head = tx_ring->next_to_use;
	u16 i = head;

	first = NBL_TX_BUF(tx_ring, head);
	first_desc = NBL_TX_DESC(tx_ring, head);

	init_packet = kzalloc(sizeof(*init_packet), GFP_KERNEL);
	if (!init_packet)
		return false;

	init_packet->pkthdr.l4s_sid = priv_tx->index;
	init_packet->pkthdr.l4s_sync_ind = 1;
	init_packet->pkthdr.l4s_hdl_ind = 1;
	init_packet->init_payload.initial = 1;
	init_packet->init_payload.sync = 0;
	init_packet->init_payload.sid = priv_tx->index;
	memcpy(init_packet->init_payload.iv, priv_tx->iv, NBL_KTLS_IV_LEN);
	memcpy(init_packet->init_payload.rec_num, priv_tx->rec_num, NBL_KTLS_REC_LEN);
	/* Since the logic will add 1 to iv and rec_seq before using them,
	 * to ensure the consistency of software and hardware,
	 * the software will be delivered after subtracting 1
	 */
	nbl_ktls_bigint_decrement(init_packet->init_payload.iv, NBL_KTLS_IV_LEN);
	nbl_ktls_bigint_decrement(init_packet->init_payload.rec_num, NBL_KTLS_REC_LEN);

	hdrdma = dma_map_single(dma_dev, init_packet, sizeof(*init_packet), DMA_TO_DEVICE);
	if (dma_mapping_error(dma_dev, hdrdma)) {
		kfree(init_packet);
		return false;
	}

	first_desc->len = cpu_to_le32(sizeof(*init_packet));
	first_desc->addr = cpu_to_le64(hdrdma);
	first_desc->id = cpu_to_le16(head);
	first_desc->flags = cpu_to_le16(avail_used_flags);

	first->dma = hdrdma;
	first->len = sizeof(*init_packet);
	first->tls_pkthdr = &init_packet->pkthdr;
	i++;
	if (i == tx_ring->desc_num) {
		i = 0;
		tx_ring->avail_used_flags ^= 1 << NBL_PACKED_DESC_F_AVAIL |
			1 << NBL_PACKED_DESC_F_USED;
	}

	first->next_to_watch = first_desc;
	tx_ring->next_to_use = i;

	notify_param.notify_qid = tx_ring->notify_qid;
	notify_param.tail_ptr = i;
	phy_ops->update_tail_ptr(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), &notify_param);

	return true;
}

static enum nbl_ktls_sync_retval
nbl_tls_resync_info_get(struct nbl_ktls_offload_context_tx *priv_tx, u32 seq,
			int datalen, struct nbl_tx_resync_info *info)
{
	int remaining = 0;
	int i = 0;
	enum nbl_ktls_sync_retval ret = NBL_KTLS_SYNC_DONE;
	struct tls_record_info  *record;
	struct tls_offload_context_tx *tx_ctx;
	unsigned long flags;
	bool ends_before;

	tx_ctx = priv_tx->tx_ctx;

	spin_lock_irqsave(&tx_ctx->lock, flags);
	record = tls_get_record(tx_ctx, seq, &info->rec_num);
	if (!record) {
		ret = NBL_KTLS_SYNC_FAIL;
		goto out;
	}

	ends_before = before(seq + datalen - 1, tls_record_start_seq(record));

	if (unlikely(tls_record_is_start_marker(record))) {
		ret = ends_before ? NBL_KTLS_SYNC_SKIP_NO_DATA : NBL_KTLS_SYNC_FAIL;
		goto out;
	} else if (ends_before) {
		ret = NBL_KTLS_SYNC_FAIL;
		goto out;
	}

	info->resync_len = seq - tls_record_start_seq(record);
	remaining = info->resync_len;

	while (remaining > 0) {
		skb_frag_t *frag = &record->frags[i];

		remaining -= skb_frag_size(frag);
		info->frags[i++] = *frag;
	}

	if (remaining < 0)
		skb_frag_size_add(&info->frags[i - 1], remaining);

	info->nr_frags = i;
out:
	spin_unlock_irqrestore(&tx_ctx->lock, flags);
	return ret;
}

static bool nbl_ktls_send_resync_one(struct nbl_resource_mgt *res_mgt,
				     struct nbl_res_tx_ring *tx_ring,
				     struct nbl_ktls_sync_packet *sync_packet,
				     struct nbl_tx_resync_info *info)
{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct device *dma_dev = NBL_RES_MGT_TO_DMA_DEV(res_mgt);
	struct nbl_tx_buffer *tx_buffer;
	struct nbl_ring_desc *tx_desc;
	struct nbl_notify_param notify_param = {0};
	dma_addr_t hdrdma;
	u16 head = tx_ring->next_to_use;
	u32 red_off = 0;
	int len, k;

	tx_buffer = NBL_TX_BUF(tx_ring, head);
	tx_desc = NBL_TX_DESC(tx_ring, head);

	dev_dbg(NBL_RING_TO_DEV(tx_ring), "send one resync packet.\n");
	for (k = 0; k < info->nr_frags; k++) {
		skb_frag_t *f = &info->frags[k];
		u8 *vaddr = kmap_local_page(skb_frag_page(f));
		u32 f_off = skb_frag_off(f);
		u32 fsz = skb_frag_size(f);

		memcpy(sync_packet->sync_payload.redata + red_off, vaddr + f_off, fsz);
		kunmap_local(vaddr);
		red_off += fsz;
	}

	len = info->resync_len + NBL_KTLS_SYNC_PKT_LEN;

	hdrdma = dma_map_single(dma_dev, sync_packet, len, DMA_TO_DEVICE);
	if (dma_mapping_error(dma_dev, hdrdma)) {
		kfree(sync_packet);
		return false;
	}

	tx_desc->addr = cpu_to_le64(hdrdma);
	tx_desc->len = cpu_to_le32(len);
	tx_desc->id = cpu_to_le16(head);
	tx_desc->flags = cpu_to_le16(tx_ring->avail_used_flags);

	tx_buffer->dma = hdrdma;
	tx_buffer->len = len;
	tx_buffer->next_to_watch = tx_desc;
	tx_buffer->tls_pkthdr = &sync_packet->pkthdr;

	if (head + 1 == tx_ring->desc_num) {
		tx_ring->next_to_use = 0;
		tx_ring->avail_used_flags ^= 1 << NBL_PACKED_DESC_F_AVAIL |
					     1 << NBL_PACKED_DESC_F_USED;
	} else {
		tx_ring->next_to_use = head + 1;
	}

	notify_param.notify_qid = tx_ring->notify_qid;
	notify_param.tail_ptr = tx_ring->next_to_use;
	phy_ops->update_tail_ptr(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), &notify_param);

	return true;
}

static bool nbl_ktls_send_resync_mul(struct nbl_resource_mgt *res_mgt,
				     struct nbl_res_tx_ring *tx_ring,
				     struct nbl_ktls_sync_packet *sync_packet,
				     struct nbl_tx_resync_info *info)
{
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct device *dma_dev = NBL_RES_MGT_TO_DMA_DEV(res_mgt);
	union nbl_tx_extend_head *pkthdr;
	struct nbl_tx_buffer *head_buffer;
	struct nbl_ring_desc *head_desc;
	struct nbl_tx_buffer *tx_buffer = NBL_TX_BUF(tx_ring, tx_ring->next_to_use);
	struct nbl_ring_desc *tx_desc;
	struct nbl_notify_param notify_param = {0};
	dma_addr_t hdrdma;
	dma_addr_t bufdma;
	dma_addr_t firstdma = 0;
	skb_frag_t *frag;
	u16 avail_used_flags = tx_ring->avail_used_flags;
	u16 head = tx_ring->next_to_use;
	u16 index = head;
	u32 total_len = 0;
	u32 remain_len;
	int last_len;
	int len, k;
	unsigned int fsz;

	last_len = info->resync_len % NBL_KTLS_PER_CELL_LEN + NBL_KTLS_PER_CELL_LEN;
	if (last_len > NBL_KTLS_MAX_CELL_LEN)
		last_len -= NBL_KTLS_PER_CELL_LEN;

	dev_dbg(NBL_RING_TO_DEV(tx_ring), "send mul resync packet.\n");
	/* Each packet in the middle is 512 bytes */
	remain_len = info->resync_len - last_len;

	head_buffer = NBL_TX_BUF(tx_ring, head);
	head_desc = NBL_TX_DESC(tx_ring, head);

	hdrdma = dma_map_single(dma_dev, sync_packet, NBL_KTLS_SYNC_PKT_LEN, DMA_TO_DEVICE);
	if (dma_mapping_error(dma_dev, hdrdma)) {
		kfree(sync_packet);
		goto dma_map_error;
	}

	head_desc->addr = cpu_to_le64(hdrdma);
	head_desc->len = cpu_to_le32(NBL_KTLS_SYNC_PKT_LEN);
	head_desc->id = cpu_to_le16(head);

	head_buffer->dma = hdrdma;
	head_buffer->len = NBL_KTLS_SYNC_PKT_LEN;
	head_buffer->tls_pkthdr = &sync_packet->pkthdr;

	for (k = 0; k < info->nr_frags; k++) {
		frag = &info->frags[k];
		fsz = skb_frag_size(frag);
		dev_dbg(NBL_RING_TO_DEV(tx_ring), "send frag %d len %u.\n", k, fsz);
		bufdma = skb_frag_dma_map(dma_dev, frag, 0, fsz, DMA_TO_DEVICE);
		if (dma_mapping_error(dma_dev, bufdma)) {
			index++;
			goto dma_map_error;
		}
		firstdma = bufdma;
		total_len = fsz;
		while (fsz) {
			index++;
			if (index == tx_ring->desc_num) {
				index = 0;
				tx_buffer = NBL_TX_BUF(tx_ring, 0);
				tx_desc = NBL_TX_DESC(tx_ring, 0);
				tx_ring->avail_used_flags ^=
					1 << NBL_PACKED_DESC_F_AVAIL |
					1 << NBL_PACKED_DESC_F_USED;
			} else {
				tx_buffer = NBL_TX_BUF(tx_ring, index);
				tx_desc = NBL_TX_DESC(tx_ring, index);
			}

			len = remain_len % NBL_KTLS_PER_CELL_LEN;
			len = (len) ? (len) : min_t(unsigned int, fsz, NBL_KTLS_PER_CELL_LEN);
			if (fsz < len || remain_len == 0)
				len = fsz;

			tx_desc->addr = cpu_to_le64(bufdma);
			tx_desc->len = cpu_to_le32(len);
			tx_desc->id = cpu_to_le16(head);
			tx_desc->flags = cpu_to_le16(tx_ring->avail_used_flags);
			dev_dbg(NBL_RING_TO_DEV(tx_ring),
				"send %u packet len %d remain_len %u.\n", head, len, remain_len);

			head_buffer->next_to_watch = tx_desc;

			bufdma += len;
			fsz -= len;
			if (remain_len == 0) {
				last_len -= len;
				if (last_len > 0)
					tx_desc->flags = cpu_to_le16(le16_to_cpu(tx_desc->flags) |
								     NBL_PACKED_DESC_F_NEXT);
				continue;
			}

			remain_len -= len;
			if (remain_len % NBL_KTLS_PER_CELL_LEN) {
				tx_desc->flags = cpu_to_le16(le16_to_cpu(tx_desc->flags) |
							     NBL_PACKED_DESC_F_NEXT);
				continue;
			}

			head_desc->flags = cpu_to_le16(avail_used_flags | NBL_PACKED_DESC_F_NEXT);

			index++;
			if (index == tx_ring->desc_num) {
				index = 0;
				head = 0;
				head_buffer = NBL_TX_BUF(tx_ring, 0);
				head_desc = NBL_TX_DESC(tx_ring, 0);
				tx_ring->avail_used_flags ^=
					1 << NBL_PACKED_DESC_F_AVAIL |
					1 << NBL_PACKED_DESC_F_USED;
			} else {
				head = index;
				head_buffer = NBL_TX_BUF(tx_ring, head);
				head_desc = NBL_TX_DESC(tx_ring, head);
			}
			avail_used_flags = tx_ring->avail_used_flags;

			pkthdr = kzalloc(sizeof(*pkthdr), GFP_KERNEL);
			if (!pkthdr)
				goto dma_map_error;

			pkthdr->l4s_sid = sync_packet->pkthdr.l4s_sid;
			pkthdr->l4s_redun_ind = 1;
			pkthdr->l4s_hdl_ind = 1;
			hdrdma = dma_map_single(dma_dev, pkthdr,
						sizeof(union nbl_tx_extend_head), DMA_TO_DEVICE);
			if (dma_mapping_error(dma_dev, hdrdma)) {
				kfree(pkthdr);
				goto dma_map_error;
			}

			head_desc->addr = cpu_to_le64(hdrdma);
			head_desc->len = cpu_to_le32(sizeof(union nbl_tx_extend_head));
			head_desc->id = cpu_to_le16(head);

			head_buffer->dma = hdrdma;
			head_buffer->len = sizeof(union nbl_tx_extend_head);
			head_buffer->tls_pkthdr = pkthdr;
		}
		tx_buffer->dma = firstdma;
		tx_buffer->len = total_len;
	}
	/* wmb for head desc */
	wmb();

	head_desc->flags = cpu_to_le16(avail_used_flags | NBL_PACKED_DESC_F_NEXT);

	if (index + 1 == tx_ring->desc_num) {
		tx_ring->next_to_use = 0;
		tx_ring->avail_used_flags ^= 1 << NBL_PACKED_DESC_F_AVAIL |
					     1 << NBL_PACKED_DESC_F_USED;
	} else {
		tx_ring->next_to_use = index + 1;
	}

	notify_param.notify_qid = tx_ring->notify_qid;
	notify_param.tail_ptr = tx_ring->next_to_use;
	phy_ops->update_tail_ptr(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), &notify_param);

	return true;

dma_map_error:
	while (index != tx_ring->next_to_use) {
		if (unlikely(!index))
			index = tx_ring->desc_num;
		index--;
		nbl_unmap_and_free_tx_resource(tx_ring, NBL_TX_BUF(tx_ring, index), false, false);
	}
	tx_ring->avail_used_flags = avail_used_flags;

	return false;
}

/* Set the maximum packet length to 768 bytes and occur in the first or last packets.
 * The middle packets are all 512 bytes long bacause the hardware cell is 512 bytes.
 * If pkt_len > 768, pkt_len -= 512, make sure the packet is greater than 256 bytes.
 */
static bool nbl_ktls_send_resync_packet(struct nbl_resource_mgt *res_mgt,
					struct nbl_res_tx_ring *tx_ring,
					struct nbl_ktls_offload_context_tx *priv_tx,
					struct nbl_tx_resync_info *info)
{
	struct nbl_ktls_sync_packet *sync_packet;
	__be64 rec_num;

	sync_packet = kzalloc(sizeof(*sync_packet), GFP_KERNEL);
	if (!sync_packet)
		return false;

	sync_packet->pkthdr.l4s_sid = priv_tx->index;
	sync_packet->pkthdr.l4s_redun_ind = 1;
	sync_packet->pkthdr.l4s_redun_head_ind = 1;
	sync_packet->pkthdr.l4s_hdl_ind = 1;
	sync_packet->sync_payload.sync = 1;
	sync_packet->sync_payload.sid = priv_tx->index;

	if (info->resync_len == 0)
		info->rec_num = info->rec_num - 1;
	rec_num = cpu_to_be64(info->rec_num);
	memcpy(sync_packet->sync_payload.rec_num, &rec_num, NBL_KTLS_REC_LEN);

	if (info->resync_len <= NBL_KTLS_MAX_CELL_LEN) {
		sync_packet->sync_payload.redlen = htons(info->resync_len);
		return nbl_ktls_send_resync_one(res_mgt, tx_ring, sync_packet, info);
	}

	sync_packet->sync_payload.redlen = htons(NBL_KTLS_PER_CELL_LEN);
	return nbl_ktls_send_resync_mul(res_mgt, tx_ring, sync_packet, info);
}

/* Handle packet out-of-order function */
static enum nbl_ktls_sync_retval
nbl_ktls_tx_handle_ooo(struct nbl_resource_mgt *res_mgt, struct nbl_res_tx_ring *tx_ring,
		       struct nbl_ktls_offload_context_tx *priv_tx,
		       u32 tcp_seq, int datalen)
{
	enum nbl_ktls_sync_retval ret;
	struct nbl_tx_resync_info resync_info = {0};

	ret = nbl_tls_resync_info_get(priv_tx, tcp_seq, datalen, &resync_info);
	if (unlikely(ret != NBL_KTLS_SYNC_DONE))
		return ret;

	dev_dbg(NBL_RING_TO_DEV(tx_ring), "rec_num %llu, resync_len %u, nr_frags %u.\n",
		resync_info.rec_num, resync_info.resync_len, resync_info.nr_frags);
	if (unlikely(!nbl_ktls_send_resync_packet(res_mgt, tx_ring, priv_tx, &resync_info)))
		return NBL_KTLS_SYNC_FAIL;

	return NBL_KTLS_SYNC_DONE;
}

static bool nbl_ktls_tx_offload_handle(struct nbl_resource_mgt *res_mgt,
				       struct nbl_res_tx_ring *tx_ring,
				       struct sk_buff *skb,
				       struct nbl_tx_hdr_param *accel_state)
{
	struct net_device *netdev = tx_ring->netdev;
	struct tls_context *tls_ctx;
	struct nbl_ktls_offload_context_tx **ctx;
	struct nbl_ktls_offload_context_tx *priv_tx;
	enum nbl_ktls_sync_retval ret;
	u32 tcp_seq = 0;
	int datalen = 0;

	datalen = skb->len - (skb_transport_offset(skb) + tcp_hdrlen(skb));
	if (!datalen)
		return 0;

	tls_ctx = tls_get_ctx(skb->sk);
	if (WARN_ON_ONCE(tls_ctx->netdev != netdev))
		goto err_out;

	ctx = __tls_driver_ctx(tls_ctx, TLS_OFFLOAD_CTX_DIR_TX);
	priv_tx = *ctx;
	/* config data to hardware */
	if (priv_tx->ctx_post_pending) {
		priv_tx->ctx_post_pending = false;
		if (!nbl_ktls_send_init_packet(res_mgt, tx_ring, priv_tx))
			goto err_out;
	}

	tcp_seq = ntohl(tcp_hdr(skb)->seq);
	dev_dbg(NBL_RING_TO_DEV(tx_ring), "ktls tx tcp_seq %u.\n", tcp_seq);
	if (unlikely(priv_tx->expected_tcp != tcp_seq)) {
		dev_dbg(NBL_RING_TO_DEV(tx_ring), "ktls tx tcp_seq %u, but expected_tcp %u.\n",
			tcp_seq, priv_tx->expected_tcp);
		ret = nbl_ktls_tx_handle_ooo(res_mgt, tx_ring, priv_tx, tcp_seq, datalen);
		tx_ring->tx_stats.tls_ooo_packets++;
		switch (ret) {
		case NBL_KTLS_SYNC_DONE:
			break;
		case NBL_KTLS_SYNC_SKIP_NO_DATA:
			if (likely(!skb->decrypted))
				goto out;
			WARN_ON_ONCE(1);
			goto err_out;
		case NBL_KTLS_SYNC_FAIL:
			goto err_out;
		}
	}

	priv_tx->expected_tcp = tcp_seq + datalen;
	accel_state->l4s_sid = priv_tx->index;
	accel_state->l4s_pbrac_mode = 0;
	accel_state->l4s_hdl_ind = 1;

	tx_ring->tx_stats.tls_encrypted_packets += skb_is_gso(skb) ? skb_shinfo(skb)->gso_segs : 1;
	tx_ring->tx_stats.tls_encrypted_bytes += datalen;

out:
	return 0;

err_out:
	dev_kfree_skb_any(skb);
	return 1;
}
#else
static bool nbl_ktls_tx_offload_handle(struct nbl_resource_mgt *res_mgt,
				       struct nbl_res_tx_ring *tx_ring,
				       struct sk_buff *skb,
				       struct nbl_tx_hdr_param *accel_state)
{
	return true;
}
#endif

static bool nbl_tx_map_need_broadcast_check(struct sk_buff *skb)
{
	__be16 protocol;

	protocol = vlan_get_protocol(skb);

	if (protocol == htons(ETH_P_ARP)) {
		return true;
	} else if (protocol == htons(ETH_P_IPV6)) {
		if (pskb_may_pull(skb, sizeof(struct ipv6hdr) + sizeof(struct nd_msg)) &&
		    ipv6_hdr(skb)->nexthdr == IPPROTO_ICMPV6) {
			struct nd_msg *m = (struct nd_msg *)(ipv6_hdr(skb) + 1);

			if (m->icmph.icmp6_code == 0 && (m->icmph.icmp6_type ==
			    NDISC_NEIGHBOUR_SOLICITATION ||
			    m->icmph.icmp6_type == NDISC_NEIGHBOUR_ADVERTISEMENT)) {
				return true;
			}
		}
	}
	return false;
}

static bool nbl_skb_is_lacp_or_lldp(struct sk_buff *skb)
{
	__be16 protocol;

	protocol = vlan_get_protocol(skb);
	if (protocol == htons(ETH_P_SLOW) || protocol == htons(ETH_P_LLDP))
		return true;

	return false;
}

static int nbl_tx_map(struct nbl_res_tx_ring *tx_ring, struct sk_buff *skb,
		      struct nbl_tx_hdr_param *hdr_param)
{
	struct device *dma_dev = NBL_RING_TO_DMA_DEV(tx_ring);
	struct nbl_tx_buffer *first;
	struct nbl_ring_desc *first_desc;
	struct nbl_ring_desc *tx_desc;
	union nbl_tx_extend_head *pkthdr;
	dma_addr_t hdrdma;
	int tso, csum;
	u16 desc_index = tx_ring->next_to_use;
	u16 head = desc_index;
	u16 avail_used_flags = tx_ring->avail_used_flags;
	u32 pkthdr_len;
	bool can_push;
	bool doorbell = true;

	first_desc = NBL_TX_DESC(tx_ring, desc_index);
	first = NBL_TX_BUF(tx_ring, desc_index);
	first->gso_segs = 1;
	first->bytecount = skb->len;
	first->tx_flags = 0;
	first->skb = skb;
	skb_tx_timestamp(skb);

	can_push = !skb_header_cloned(skb) && skb_headroom(skb) >= sizeof(*pkthdr);

	if (can_push)
		pkthdr = (union nbl_tx_extend_head *)(skb->data - sizeof(*pkthdr));
	else
		pkthdr = (union nbl_tx_extend_head *)(skb->cb);

	tso = nbl_tx_tso(first, hdr_param);
	if (tso < 0) {
		netdev_err(tx_ring->netdev, "tso ret:%d\n", tso);
		goto out_drop;
	}

	csum = nbl_tx_csum(first, hdr_param);
	if (csum < 0) {
		netdev_err(tx_ring->netdev, "csum ret:%d\n", csum);
		goto out_drop;
	}

	memset(pkthdr, 0, sizeof(*pkthdr));
	switch (tx_ring->product_type) {
	case NBL_LEONIS_TYPE:
		nbl_tx_fill_tx_extend_header_leonis(pkthdr, hdr_param);
		break;
	default:
		netdev_err(tx_ring->netdev, "fill tx extend header failed, product type: %d, eth: %u.\n",
			   tx_ring->product_type, hdr_param->dport_id);
		goto out_drop;
	}

	pkthdr_len = sizeof(union nbl_tx_extend_head);

	if (can_push) {
		__skb_push(skb, pkthdr_len);
		if (nbl_map_skb(tx_ring, skb, 1, &desc_index))
			goto dma_map_error;
		__skb_pull(skb, pkthdr_len);
	} else {
		hdrdma = dma_map_single(dma_dev, pkthdr, pkthdr_len, DMA_TO_DEVICE);
		if (dma_mapping_error(dma_dev, hdrdma)) {
			tx_ring->tx_stats.tx_dma_busy++;
			return NETDEV_TX_BUSY;
		}

		first_desc->addr = cpu_to_le64(hdrdma);
		first_desc->len = pkthdr_len;

		first->dma = hdrdma;
		first->len = pkthdr_len;

		desc_index++;
		if (desc_index == tx_ring->desc_num) {
			desc_index = 0;
			tx_ring->avail_used_flags ^= 1 << NBL_PACKED_DESC_F_AVAIL |
						     1 << NBL_PACKED_DESC_F_USED;
		}
		if (nbl_map_skb(tx_ring, skb, 0, &desc_index))
			goto dma_map_error;
	}

	/* stats */
	if (is_multicast_ether_addr(skb->data))
		tx_ring->tx_stats.tx_multicast_packets += tso;
	else
		tx_ring->tx_stats.tx_unicast_packets += tso;

	if (tso > 1) {
		tx_ring->tx_stats.tso_packets++;
		tx_ring->tx_stats.tso_bytes += skb->len;
	}
	tx_ring->tx_stats.tx_csum_packets += csum;

	tx_desc = NBL_TX_DESC(tx_ring, (desc_index == 0 ? tx_ring->desc_num : desc_index) - 1);
	tx_desc->flags &= cpu_to_le16(~NBL_PACKED_DESC_F_NEXT);
	first_desc->len += (hdr_param->total_hlen << NBL_TX_TOTAL_HEADERLEN_SHIFT);
	first_desc->id = cpu_to_le16(skb_shinfo(skb)->gso_size);

	tx_ring->next_to_use = desc_index;
	nbl_maybe_stop_tx(tx_ring, DESC_NEEDED);
	if (nbl_txrx_within_vsi(&tx_ring->vsi_info[NBL_VSI_DATA], tx_ring->queue_index))
		doorbell = __netdev_tx_sent_queue(txring_txq(tx_ring),
						  first->bytecount, netdev_xmit_more());
	/* wmb */
	wmb();

	first->next_to_watch = tx_desc;
	/* first desc last set flag */
	if (first_desc == tx_desc)
		first_desc->flags = cpu_to_le16(avail_used_flags);
	else
		first_desc->flags = cpu_to_le16(avail_used_flags | NBL_PACKED_DESC_F_NEXT);

	/* kick doorbell passthrough for performace */
	if (doorbell)
		writel(tx_ring->notify_qid, tx_ring->notify_addr);

	// nbl_trace(tx_map_ok, tx_ring, skb, head, first_desc, pkthdr);

	return NETDEV_TX_OK;

dma_map_error:
	while (desc_index != head) {
		if (unlikely(!desc_index))
			desc_index = tx_ring->desc_num;
		desc_index--;
		nbl_unmap_and_free_tx_resource(tx_ring, NBL_TX_BUF(tx_ring, desc_index),
					       false, false);
	}

	tx_ring->avail_used_flags = avail_used_flags;
	tx_ring->tx_stats.tx_dma_busy++;
	return NETDEV_TX_BUSY;

out_drop:
	netdev_err(tx_ring->netdev, "tx_map, free_skb\n");
	tx_ring->tx_stats.tx_skb_free++;
	// nbl_trace(tx_map_drop, tx_ring, skb);
	dev_kfree_skb_any(skb);
	return NETDEV_TX_OK;
}

static netdev_tx_t nbl_res_txrx_rep_xmit(struct sk_buff *skb,
					 struct net_device *netdev)
{
	struct nbl_resource_mgt *res_mgt =
				NBL_ADAPTER_TO_RES_MGT(NBL_NETDEV_TO_ADAPTER(netdev));
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_tx_ring *tx_ring = txrx_mgt->tx_rings[skb_get_queue_mapping(skb)];
	struct nbl_tx_hdr_param hdr_param = {
		.mac_len = 14 >> 1,
		.ip_len = 20 >> 2,
		.l4_len = 20 >> 2,
		.mss = 256,
	};
	unsigned int count;
	int ret = 0;

	count = nbl_xmit_desc_count(skb);
	/* TODO: we can not tranmit a packet with more than 32 descriptors */
	WARN_ON(count > MAX_DESC_NUM_PER_PKT);
	if (unlikely(nbl_maybe_stop_tx(tx_ring, count))) {
		if (net_ratelimit())
			dev_dbg(NBL_RING_TO_DEV(tx_ring), "no desc to tx pkt in queue %u\n",
				tx_ring->queue_index);
		tx_ring->tx_stats.tx_busy++;
		return NETDEV_TX_BUSY;
	}

	eth_skb_pad(skb);

	hdr_param.dport_id = *(u16 *)(&skb->cb[NBL_SKB_FILL_VSI_ID_OFF]);
	hdr_param.dport = NBL_TX_DPORT_HOST;
	hdr_param.rss_lag_en = 1;
	hdr_param.fwd = NBL_TX_FWD_TYPE_CPU_ASSIGNED;

	ret = nbl_tx_map(tx_ring, skb, &hdr_param);

	return ret;
}

static netdev_tx_t nbl_res_txrx_self_test_start_xmit(struct sk_buff *skb, struct net_device *netdev)
{
	struct nbl_resource_mgt *res_mgt =
				NBL_ADAPTER_TO_RES_MGT(NBL_NETDEV_TO_ADAPTER(netdev));
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_tx_ring *tx_ring = txrx_mgt->tx_rings[skb_get_queue_mapping(skb)];
	struct nbl_tx_hdr_param hdr_param = {
		.mac_len = 14 >> 1,
		.ip_len = 20 >> 2,
		.l4_len = 20 >> 2,
		.mss = 256,
	};
	unsigned int count;

	count = nbl_xmit_desc_count(skb);
	/* TODO: we can not tranmit a packet with more than 32 descriptors */
	WARN_ON(count > MAX_DESC_NUM_PER_PKT);
	if (unlikely(nbl_maybe_stop_tx(tx_ring, count))) {
		if (net_ratelimit())
			dev_dbg(NBL_RING_TO_DEV(tx_ring), "no desc to tx pkt in queue %u\n",
				tx_ring->queue_index);
		tx_ring->tx_stats.tx_busy++;
		return NETDEV_TX_BUSY;
	}

	/* for dstore and eth, min packet len is 60 */
	eth_skb_pad(skb);

	hdr_param.fwd = NBL_TX_FWD_TYPE_CPU_ASSIGNED;
	hdr_param.dport = NBL_TX_DPORT_ETH;
	hdr_param.dport_id = tx_ring->eth_id;
	hdr_param.rss_lag_en = 0;

	return nbl_tx_map(tx_ring, skb, &hdr_param);
}

static netdev_tx_t nbl_res_txrx_start_xmit(struct sk_buff *skb,
					   struct net_device *netdev)
{
	struct nbl_resource_mgt *res_mgt =
				NBL_ADAPTER_TO_RES_MGT(NBL_NETDEV_TO_ADAPTER(netdev));
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_tx_ring *tx_ring = txrx_mgt->tx_rings[skb_get_queue_mapping(skb)];
	struct nbl_tx_hdr_param hdr_param = {
		.mac_len = 14 >> 1,
		.ip_len = 20 >> 2,
		.l4_len = 20 >> 2,
		.mss = 256,
	};
	u16 vlan_tci;
	u16 vlan_proto;
	struct sk_buff *skb2 = NULL;
	unsigned int count;
	int ret = 0;

	// nbl_trace(xmit_frame_ring, tx_ring, skb);

	count = nbl_xmit_desc_count(skb);
	/* TODO: we can not tranmit a packet with more than 32 descriptors */
	WARN_ON(count > MAX_DESC_NUM_PER_PKT);
	if (unlikely(nbl_maybe_stop_tx(tx_ring, count))) {
		if (net_ratelimit())
			dev_dbg(NBL_RING_TO_DEV(tx_ring), "no desc to tx pkt in queue %u\n",
				tx_ring->queue_index);
		tx_ring->tx_stats.tx_busy++;
		return NETDEV_TX_BUSY;
	}

	if (tx_ring->vlan_proto || skb_vlan_tag_present(skb)) {
		if (tx_ring->vlan_proto) {
			vlan_proto = htons(tx_ring->vlan_proto);
			vlan_tci = tx_ring->vlan_tci;
		}

		if (skb_vlan_tag_present(skb)) {
			vlan_proto = skb->vlan_proto;
			vlan_tci = skb_vlan_tag_get(skb);
		}

		skb = vlan_insert_tag_set_proto(skb, vlan_proto, vlan_tci);
		if (!skb)
			return NETDEV_TX_OK;
	}

	if (nbl_ktls_device_offload(skb))
		if (nbl_ktls_tx_offload_handle(res_mgt, tx_ring, skb, &hdr_param))
			return NETDEV_TX_OK;

	/* for dstore and eth, min packet len is 60 */
	eth_skb_pad(skb);

	hdr_param.dport_id = tx_ring->eth_id;
	hdr_param.fwd = 1;
	hdr_param.rss_lag_en = 0;

	/* ipro fwd to eth port */
	if (tx_ring->mode == NBL_ESWITCH_OFFLOADS) {
		hdr_param.fwd = NBL_TX_FWD_TYPE_CPU_ASSIGNED;
		hdr_param.dport = NBL_TX_DPORT_ETH;
		if (txrx_mgt->bond_info.bond_enable && !nbl_skb_is_lacp_or_lldp(skb)) {
			hdr_param.dport_id = txrx_mgt->bond_info.lag_id <<
					     NBL_TX_DPORT_ID_LAG_OFFSET;
			hdr_param.rss_lag_en = 1;
		}
	}

	if (nbl_skb_is_lacp_or_lldp(skb)) {
		hdr_param.fwd = NBL_TX_FWD_TYPE_CPU_ASSIGNED;
		hdr_param.dport = NBL_TX_DPORT_ETH;
	}

	/* for unicast packet tx_map all */
	if (txrx_mgt->bond_info.bond_enable && nbl_tx_map_need_broadcast_check(skb)) {
		int ret2;

		hdr_param.fwd = NBL_TX_FWD_TYPE_CPU_ASSIGNED;
		hdr_param.dport = NBL_TX_DPORT_ETH;
		hdr_param.dport_id = txrx_mgt->bond_info.eth_id[0];
		hdr_param.rss_lag_en = 0;

		skb2 = skb_copy(skb, GFP_ATOMIC);
		ret |= nbl_tx_map(tx_ring, skb, &hdr_param);
		if (likely(skb2)) {
			hdr_param.dport_id = txrx_mgt->bond_info.eth_id[1];
			ret2 = nbl_tx_map(tx_ring, skb2, &hdr_param);
			if (ret2)
				dev_kfree_skb_any(skb2);
		}

	} else {
		ret = nbl_tx_map(tx_ring, skb, &hdr_param);
	}

	return ret;
}

static void nbl_res_txrx_kick_rx_ring(void *priv, u16 index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_phy_ops *phy_ops = NBL_RES_MGT_TO_PHY_OPS(res_mgt);
	struct nbl_notify_param notify_param = {0};
	struct nbl_res_rx_ring *rx_ring = NBL_RES_MGT_TO_RX_RING(res_mgt, index);

	notify_param.notify_qid = rx_ring->notify_qid;
	notify_param.tail_ptr = rx_ring->tail_ptr;
	phy_ops->update_tail_ptr(NBL_RES_MGT_TO_PHY_PRIV(res_mgt), &notify_param);
}

static int nbl_res_txring_is_invalid(struct nbl_resource_mgt *res_mgt,
				     struct seq_file *m, int index)
{
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_tx_ring *tx_ring;
	u16 ring_num = txrx_mgt->tx_ring_num;

	if (index >= ring_num) {
		seq_printf(m, "Invalid tx index %d, max ring num is %d\n", index, ring_num);
		return -EINVAL;
	}

	tx_ring = NBL_RES_MGT_TO_TX_RING(res_mgt, index);
	if (!tx_ring || !tx_ring->valid) {
		seq_puts(m, "Ring doesn't exist, wrong index or the netdev might be stopped\n");
		return -EINVAL;
	}

	return 0;
}

static int nbl_res_rxring_is_invalid(struct nbl_resource_mgt *res_mgt,
				     struct seq_file *m, int index)
{
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_rx_ring *rx_ring;
	u16 ring_num = txrx_mgt->rx_ring_num;

	if (index >= ring_num) {
		seq_printf(m, "Invalid rx index %d, max ring num is %d\n", index, ring_num);
		return -EINVAL;
	}

	rx_ring = NBL_RES_MGT_TO_RX_RING(res_mgt, index);
	if (!rx_ring || !rx_ring->valid) {
		seq_puts(m, "Ring doesn't exist, wrong index or the netdev might be stopped\n");
		return -EINVAL;
	}

	return 0;
}

static int nbl_res_rx_dump_ring(struct nbl_resource_mgt *res_mgt, struct seq_file *m, int index)
{
	struct nbl_res_rx_ring *ring = NBL_RES_MGT_TO_RX_RING(res_mgt, index);
	struct nbl_ring_desc *desc;
	int i;

	if (nbl_res_rxring_is_invalid(res_mgt, m, index))
		return 0;

	seq_printf(m, "queue_index %d desc_num %d used_wrap_counter 0x%x avail_used_flags 0x%x\n",
		   ring->queue_index, ring->desc_num,
		   ring->used_wrap_counter, ring->avail_used_flags);
	seq_printf(m, "ntu 0x%x, ntc 0x%x, tail_ptr 0x%x\n",
		   ring->next_to_use, ring->next_to_clean, ring->tail_ptr);
	seq_printf(m, "desc dma 0x%llx, HZ %u\n", ring->dma, HZ);

	seq_puts(m, "desc:\n");
	for (i = 0; i < ring->desc_num; i++) {
		desc = ring->desc + i;
		seq_printf(m, "desc id %d, addr 0x%llx len %d flag 0x%x\n",
			   desc->id, desc->addr, desc->len, desc->flags);
	}

	return 0;
}

static int nbl_res_tx_dump_ring(struct nbl_resource_mgt *res_mgt, struct seq_file *m, int index)
{
	struct nbl_res_tx_ring *ring = NBL_RES_MGT_TO_TX_RING(res_mgt, index);
	struct nbl_ring_desc *desc;
	u32 total_header_len;
	u32 desc_len;
	int i;

	if (nbl_res_txring_is_invalid(res_mgt, m, index))
		return 0;

	seq_printf(m, "queue_index %d desc_num %d used_wrap_counter 0x%x avail_used_flags 0x%x\n",
		   ring->queue_index, ring->desc_num,
		   ring->used_wrap_counter, ring->avail_used_flags);
	seq_printf(m, "ntu 0x%x, ntc 0x%x tail_ptr 0x%x\n",
		   ring->next_to_use, ring->next_to_clean, ring->tail_ptr);
	seq_printf(m, "desc dma 0x%llx, HZ %u\n", ring->dma, HZ);
	seq_printf(m, "tx_skb_free %llu\n", ring->tx_stats.tx_skb_free);

	seq_puts(m, "desc:\n");
	for (i = 0; i < ring->desc_num; i++) {
		desc = ring->desc + i;
		total_header_len = desc->len >> NBL_TX_TOTAL_HEADERLEN_SHIFT;
		desc_len = desc->len & 0xFFFFFF;
		seq_printf(m, "desc %d: id/gso_size %d, addr 0x%llx len %d header_len %d flag 0x%x\n",
			   i, desc->id, desc->addr, desc_len, total_header_len, desc->flags);
	}

	return 0;
}

static int nbl_res_txrx_dump_ring(void *priv, struct seq_file *m, bool is_tx, int index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	if (is_tx)
		return nbl_res_tx_dump_ring(res_mgt, m, index);
	else
		return nbl_res_rx_dump_ring(res_mgt, m, index);
}

static int nbl_res_tx_dump_ring_stats(struct nbl_resource_mgt *res_mgt,
				      struct seq_file *m, int index)
{
	struct nbl_res_tx_ring *ring = NBL_RES_MGT_TO_TX_RING(res_mgt, index);

	if (nbl_res_txring_is_invalid(res_mgt, m, index))
		return 0;

	seq_printf(m, "pkts: %lld, bytes: %lld, descs: %lld\n",
		   ring->stats.packets, ring->stats.bytes, ring->stats.descs);
	seq_printf(m, "tso_pkts: %lld, tso_bytes: %lld, tx_checksum_pkts: %lld\n",
		   ring->tx_stats.tso_packets, ring->tx_stats.tso_bytes,
		   ring->tx_stats.tx_csum_packets);
	seq_printf(m, "tx_busy: %lld, tx_dma_busy: %lld\n",
		   ring->tx_stats.tx_busy, ring->tx_stats.tx_dma_busy);
	seq_printf(m, "tx_multicast_pkts: %lld, tx_unicast_pkts: %lld\n",
		   ring->tx_stats.tx_multicast_packets,
		   ring->tx_stats.tx_unicast_packets);
	seq_printf(m, "tx_skb_free: %lld, tx_desc_addr_err: %lld, tx_desc_len_err: %lld\n",
		   ring->tx_stats.tx_skb_free, ring->tx_stats.tx_desc_addr_err_cnt,
		   ring->tx_stats.tx_desc_len_err_cnt);
	return 0;
}

static int nbl_res_rx_dump_ring_stats(struct nbl_resource_mgt *res_mgt,
				      struct seq_file *m, int index)
{
	struct nbl_res_rx_ring *ring = NBL_RES_MGT_TO_RX_RING(res_mgt, index);

	if (nbl_res_rxring_is_invalid(res_mgt, m, index))
		return 0;

	seq_printf(m, "rx_checksum_pkts: %lld, rx_checksum_errors: %lld\n",
		   ring->rx_stats.rx_csum_packets, ring->rx_stats.rx_csum_errors);
	seq_printf(m, "rx_multicast_pkts: %lld, rx_unicast_pkts: %lld\n",
		   ring->rx_stats.rx_multicast_packets,
		   ring->rx_stats.rx_unicast_packets);
	seq_printf(m, "rx_desc_addr_err: %lld\n",
		   ring->rx_stats.rx_desc_addr_err_cnt);
	seq_printf(m, "rx_alloc_buf_err_cnt: %lld\n",
		   ring->rx_stats.rx_alloc_buf_err_cnt);

	return 0;
}

static int nbl_res_txrx_dump_ring_stats(void *priv, struct seq_file *m, bool is_tx, int index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;

	if (is_tx)
		return nbl_res_tx_dump_ring_stats(res_mgt, m, index);
	else
		return nbl_res_rx_dump_ring_stats(res_mgt, m, index);
}

static struct nbl_napi_struct *nbl_res_txrx_get_vector_napi(void *priv, u16 index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_txrx_mgt *txrx_mgt = res_mgt->txrx_mgt;

	if (!txrx_mgt->vectors || index >= txrx_mgt->rx_ring_num) {
		nbl_err(common, NBL_DEBUG_RESOURCE, "vectors not allocated\n");
		return NULL;
	}

	return &txrx_mgt->vectors[index]->nbl_napi;
}

static void nbl_res_txrx_set_vector_info(void *priv, u8 *irq_enable_base,
					 u32 irq_data, u16 index, bool mask_en)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_txrx_mgt *txrx_mgt = res_mgt->txrx_mgt;

	if (!txrx_mgt->vectors || index >= txrx_mgt->rx_ring_num) {
		nbl_err(common, NBL_DEBUG_RESOURCE, "vectors not allocated\n");
		return;
	}

	txrx_mgt->vectors[index]->irq_enable_base = irq_enable_base;
	txrx_mgt->vectors[index]->irq_data = irq_data;
	txrx_mgt->vectors[index]->net_msix_mask_en = mask_en;
}

static void nbl_res_get_pt_ops(void *priv, struct nbl_resource_pt_ops *pt_ops)
{
	pt_ops->start_xmit = nbl_res_txrx_start_xmit;
	pt_ops->rep_xmit = nbl_res_txrx_rep_xmit;
	pt_ops->self_test_xmit = nbl_res_txrx_self_test_start_xmit;
	pt_ops->napi_poll = nbl_res_napi_poll;
	pt_ops->xdp_xmit = nbl_res_txrx_xdp_xmit;
}

static u32 nbl_res_txrx_get_tx_headroom(void *priv)
{
	return sizeof(union nbl_tx_extend_head);
}

static void nbl_res_txrx_get_queue_stats(void *priv, u8 queue_id,
					 struct nbl_queue_stats *queue_stats, bool is_tx)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct u64_stats_sync *syncp;
	struct nbl_queue_stats *stats;
	unsigned int start;

	if (is_tx) {
		struct nbl_res_tx_ring *ring = NBL_RES_MGT_TO_TX_RING(res_mgt, queue_id);

		syncp = &ring->syncp;
		stats = &ring->stats;
	} else {
		struct nbl_res_rx_ring *ring = NBL_RES_MGT_TO_RX_RING(res_mgt, queue_id);

		syncp = &ring->syncp;
		stats = &ring->stats;
	}

	do {
		start = u64_stats_fetch_begin(syncp);
		memcpy(queue_stats, stats, sizeof(*stats));
	} while (u64_stats_fetch_retry(syncp, start));
}

static bool nbl_res_is_ctrlq(struct nbl_txrx_mgt *txrx_mgt, u16 qid)
{
	u16 ring_num = txrx_mgt->vsi_info[NBL_VSI_CTRL].ring_num;
	u16 ring_offset = txrx_mgt->vsi_info[NBL_VSI_CTRL].ring_offset;

	if (qid >= ring_offset && qid < ring_offset + ring_num)
		return true;

	return false;
}

static void nbl_res_txrx_get_net_stats(void *priv, struct nbl_stats *net_stats)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_rx_ring *rx_ring;
	struct nbl_res_tx_ring *tx_ring;
	int i;
	u64 bytes = 0, packets = 0;
	u64 tso_packets = 0, tso_bytes = 0;
	u64 tx_csum_packets = 0;
	u64 rx_csum_packets = 0, rx_csum_errors = 0;
	u64 tx_multicast_packets = 0, tx_unicast_packets = 0;
	u64 rx_multicast_packets = 0, rx_unicast_packets = 0;
#ifdef CONFIG_TLS_DEVICE
	u64 tls_encrypted_packets = 0;
	u64 tls_encrypted_bytes = 0;
	u64 tls_ooo_packets = 0;
	u64 tls_decrypted_packets = 0;
	u64 tls_resync_req_num = 0;
#endif
	u64 tx_busy = 0, tx_dma_busy = 0;
	u64 tx_desc_addr_err_cnt = 0;
	u64 tx_desc_len_err_cnt = 0;
	u64 rx_desc_addr_err_cnt = 0;
	u64 rx_alloc_buf_err_cnt = 0;
	u64 rx_cache_reuse = 0;
	u64 rx_cache_full = 0;
	u64 rx_cache_empty = 0;
	u64 rx_cache_busy = 0;
	u64 rx_cache_waive = 0;
	u64 tx_skb_free = 0;
	u64 xdp_tx_packets = 0;
	u64 xdp_redirect_packets = 0;
	u64 xdp_oversize_packets = 0;
	u64 xdp_drop_packets = 0;
	unsigned int start;

	rcu_read_lock();
	for (i = 0; i < txrx_mgt->rx_ring_num; i++) {
		if (nbl_res_is_ctrlq(txrx_mgt, i))
			continue;

		rx_ring = NBL_RES_MGT_TO_RX_RING(res_mgt, i);
		do {
			start = u64_stats_fetch_begin(&rx_ring->syncp);
			bytes += rx_ring->stats.bytes;
			packets += rx_ring->stats.packets;
			rx_csum_packets += rx_ring->rx_stats.rx_csum_packets;
			rx_csum_errors += rx_ring->rx_stats.rx_csum_errors;
			rx_multicast_packets += rx_ring->rx_stats.rx_multicast_packets;
			rx_unicast_packets += rx_ring->rx_stats.rx_unicast_packets;
			rx_desc_addr_err_cnt += rx_ring->rx_stats.rx_desc_addr_err_cnt;
			rx_alloc_buf_err_cnt += rx_ring->rx_stats.rx_alloc_buf_err_cnt;
			rx_cache_reuse += rx_ring->rx_stats.rx_cache_reuse;
			rx_cache_full += rx_ring->rx_stats.rx_cache_full;
			rx_cache_empty += rx_ring->rx_stats.rx_cache_empty;
			rx_cache_busy += rx_ring->rx_stats.rx_cache_busy;
			rx_cache_waive += rx_ring->rx_stats.rx_cache_waive;
			xdp_tx_packets += rx_ring->rx_stats.xdp_tx_packets;
			xdp_redirect_packets += rx_ring->rx_stats.xdp_redirect_packets;
			xdp_oversize_packets += rx_ring->rx_stats.xdp_oversize_packets;
			xdp_drop_packets += rx_ring->rx_stats.xdp_drop_packets;
#ifdef CONFIG_TLS_DEVICE
			tls_decrypted_packets += rx_ring->rx_stats.tls_decrypted_packets;
			tls_resync_req_num += rx_ring->rx_stats.tls_resync_req_num;
#endif
		} while (u64_stats_fetch_retry(&rx_ring->syncp, start));
	}

	net_stats->rx_packets = packets;
	net_stats->rx_bytes = bytes;

	net_stats->rx_csum_packets = rx_csum_packets;
	net_stats->rx_csum_errors = rx_csum_errors;
	net_stats->rx_multicast_packets = rx_multicast_packets;
	net_stats->rx_unicast_packets = rx_unicast_packets;
	net_stats->xdp_tx_packets = xdp_tx_packets;
	net_stats->xdp_redirect_packets = xdp_redirect_packets;
	net_stats->xdp_oversize_packets = xdp_oversize_packets;
	net_stats->xdp_drop_packets = xdp_drop_packets;
#ifdef CONFIG_TLS_DEVICE
	net_stats->tls_decrypted_packets = tls_decrypted_packets;
	net_stats->tls_resync_req_num = tls_resync_req_num;
#endif

	bytes = 0;
	packets = 0;

	for (i = 0; i < txrx_mgt->tx_ring_num; i++) {
		if (nbl_res_is_ctrlq(txrx_mgt, i))
			continue;

		tx_ring = NBL_RES_MGT_TO_TX_RING(res_mgt, i);
		do {
			start = u64_stats_fetch_begin(&tx_ring->syncp);
			bytes += tx_ring->stats.bytes;
			packets += tx_ring->stats.packets;
			tso_packets += tx_ring->tx_stats.tso_packets;
			tso_bytes += tx_ring->tx_stats.tso_bytes;
			tx_csum_packets += tx_ring->tx_stats.tx_csum_packets;
			tx_busy += tx_ring->tx_stats.tx_busy;
			tx_dma_busy += tx_ring->tx_stats.tx_dma_busy;
			tx_multicast_packets += tx_ring->tx_stats.tx_multicast_packets;
			tx_unicast_packets += tx_ring->tx_stats.tx_unicast_packets;
			tx_skb_free += tx_ring->tx_stats.tx_skb_free;
			tx_desc_addr_err_cnt += tx_ring->tx_stats.tx_desc_addr_err_cnt;
			tx_desc_len_err_cnt += tx_ring->tx_stats.tx_desc_len_err_cnt;
#ifdef CONFIG_TLS_DEVICE
			tls_encrypted_packets += tx_ring->tx_stats.tls_encrypted_packets;
			tls_encrypted_bytes += tx_ring->tx_stats.tls_encrypted_bytes;
			tls_ooo_packets += tx_ring->tx_stats.tls_ooo_packets;
#endif
		} while (u64_stats_fetch_retry(&tx_ring->syncp, start));
	}

	rcu_read_unlock();

	net_stats->tx_bytes = bytes;
	net_stats->tx_packets = packets;
	net_stats->tso_packets = tso_packets;
	net_stats->tso_bytes = tso_bytes;
	net_stats->tx_csum_packets = tx_csum_packets;
	net_stats->tx_busy = tx_busy;
	net_stats->tx_dma_busy = tx_dma_busy;
	net_stats->tx_multicast_packets = tx_multicast_packets;
	net_stats->tx_unicast_packets = tx_unicast_packets;
	net_stats->tx_skb_free = tx_skb_free;
	net_stats->tx_desc_addr_err_cnt = tx_desc_addr_err_cnt;
	net_stats->tx_desc_len_err_cnt = tx_desc_len_err_cnt;
	net_stats->rx_desc_addr_err_cnt = rx_desc_addr_err_cnt;
	net_stats->rx_alloc_buf_err_cnt = rx_alloc_buf_err_cnt;
	net_stats->rx_cache_reuse = rx_cache_reuse;
	net_stats->rx_cache_full = rx_cache_full;
	net_stats->rx_cache_empty = rx_cache_empty;
	net_stats->rx_cache_busy = rx_cache_busy;
	net_stats->rx_cache_waive = rx_cache_waive;
#ifdef CONFIG_TLS_DEVICE
	net_stats->tls_encrypted_packets = tls_encrypted_packets;
	net_stats->tls_encrypted_bytes = tls_encrypted_bytes;
	net_stats->tls_ooo_packets = tls_ooo_packets;
#endif
}

static u16 nbl_res_txrx_get_max_desc_num(void)
{
	return NBL_MAX_DESC_NUM;
}

static u16 nbl_res_txrx_get_min_desc_num(void)
{
	return NBL_MIN_DESC_NUM;
}

static u16 nbl_res_txrx_get_tx_desc_num(void *priv, u32 ring_index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_tx_ring *ring = txrx_mgt->tx_rings[ring_index];

	return ring->desc_num;
}

static u16 nbl_res_txrx_get_rx_desc_num(void *priv, u32 ring_index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_rx_ring *ring = txrx_mgt->rx_rings[ring_index];

	return ring->desc_num;
}

static void nbl_res_txrx_set_tx_desc_num(void *priv, u32 ring_index, u16 desc_num)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_tx_ring *ring = txrx_mgt->tx_rings[ring_index];

	ring->desc_num = desc_num;
}

static void nbl_res_txrx_set_rx_desc_num(void *priv, u32 ring_index, u16 desc_num)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_rx_ring *ring = txrx_mgt->rx_rings[ring_index];

	ring->desc_num = desc_num;
}

static struct sk_buff *nbl_fetch_rx_buffer_lb_test(struct nbl_res_rx_ring *rx_ring,
						   const struct nbl_ring_desc *rx_desc,
						   u16 *num_buffers)
{
	struct nbl_rx_buffer *rx_buf;
	struct sk_buff *skb;
	const struct page *page;
	const void *page_addr;
	struct nbl_rx_extend_head *hdr;
	u32 size = 256;

	rx_buf = nbl_get_rx_buf(rx_ring);
	page = rx_buf->di->page;
	prefetchw(page);

	page_addr = page_address(page) + rx_buf->offset;
	prefetch(page_addr);

	skb = alloc_skb(size, GFP_KERNEL);
	if (unlikely(!skb))
		return NULL;

	prefetchw(skb->data);
	/* get number of buffers */
	hdr = (struct nbl_rx_extend_head *)page_addr;
	*num_buffers = le16_to_cpu(hdr->num_buffers);
	nbl_rx_csum(rx_ring, skb, hdr);

	memcpy(__skb_put(skb, size), page_addr + sizeof(*hdr), ALIGN(size, sizeof(long)));

	nbl_put_rx_buf(rx_ring, rx_buf);

	return skb;
}

static struct sk_buff *nbl_res_txrx_clean_rx_lb_test(void *priv, u32 ring_index)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_common_info *common = NBL_RES_MGT_TO_COMMON(res_mgt);
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_rx_ring *rx_ring = txrx_mgt->rx_rings[ring_index];
	struct nbl_ring_desc *rx_desc;
	struct sk_buff *skb;
	u16 num_buffers = 0;
	u16 cleaned_count = nbl_unused_rx_desc_count(rx_ring);

	if (cleaned_count & (~(NBL_MAX_BATCH_DESC - 1))) {
		nbl_alloc_rx_bufs(rx_ring, cleaned_count & (~(NBL_MAX_BATCH_DESC - 1)));
		cleaned_count = 0;
	}

	rx_desc = NBL_RX_DESC(rx_ring, rx_ring->next_to_clean);
	if (!nbl_ring_desc_used(rx_desc, rx_ring->used_wrap_counter))
		return NULL;

	/* rmb for desc used */
	rmb();

	skb = nbl_fetch_rx_buffer_lb_test(rx_ring, rx_desc, &num_buffers);
	if (!skb)
		return NULL;

	cleaned_count++;

	if (num_buffers > 1)
		nbl_err(common, NBL_DEBUG_RESOURCE, "More than one desc in lb rx, not supported\n");

	if (cleaned_count & (~(NBL_MAX_BATCH_DESC - 1)))
		nbl_alloc_rx_bufs(rx_ring, cleaned_count & (~(NBL_MAX_BATCH_DESC - 1)));

	return skb;
}

static int nbl_res_txrx_cfg_duppkt_info(void *priv, struct nbl_lag_member_list_param *param)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_txrx_bond_info *bond_info = &txrx_mgt->bond_info;
	int i = 0;

	if (!param->duppkt_enable) {
		memset(bond_info, 0, sizeof(*bond_info));
		return 0;
	} else if (param->lag_num > 1) {
		for (i = 0; i < param->lag_num && i < NBL_LAG_MAX_NUM; i++)
			bond_info->eth_id[i] = param->member_list[i].eth_id;
		bond_info->bond_enable = 1;
		bond_info->lag_id = param->lag_id;
	}

	return 0;
}

static int
nbl_res_queue_stop_abnormal_sw_queue(void *priv, u16 local_queue_id, int type)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_res_vector *vector =  NULL;
	struct nbl_res_tx_ring *tx_ring = NBL_RES_MGT_TO_TX_RING(res_mgt, local_queue_id);

	if (type != NBL_TX)
		return 0;

	if (tx_ring && !nbl_txrx_within_vsi(&tx_ring->vsi_info[NBL_VSI_XDP], local_queue_id))
		vector = NBL_RES_MGT_TO_VECTOR(res_mgt, local_queue_id);

	if (!tx_ring->valid)
		return -EINVAL;

	if (vector && !vector->started)
		return -EINVAL;

	if (vector) {
		vector->started = false;
		napi_synchronize(&vector->nbl_napi.napi);
		netif_stop_subqueue(tx_ring->netdev, local_queue_id);
	}

	return 0;
}

static dma_addr_t nbl_res_txrx_restore_abnormal_ring(void *priv, int ring_index, int type)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_res_vector *vector =  NULL;
	struct nbl_res_tx_ring *tx_ring = NBL_RES_MGT_TO_TX_RING(res_mgt, ring_index);
	struct nbl_res_rx_ring *rx_ring = NBL_RES_MGT_TO_RX_RING(res_mgt, ring_index);

	if (tx_ring && !nbl_txrx_within_vsi(&tx_ring->vsi_info[NBL_VSI_XDP], ring_index))
		vector = NBL_RES_MGT_TO_VECTOR(res_mgt, ring_index);

	switch (type) {
	case NBL_TX:
		if (tx_ring && tx_ring->valid) {
			nbl_res_txrx_stop_tx_ring(res_mgt, ring_index);
			return nbl_res_txrx_start_tx_ring(res_mgt, ring_index);
		} else {
			return (dma_addr_t)NULL;
		}
		break;
	case NBL_RX:
		if (rx_ring && rx_ring->valid) {
			nbl_res_txrx_stop_rx_ring(res_mgt, ring_index);
			return nbl_res_txrx_start_rx_ring(res_mgt, ring_index, true);
		} else {
			return (dma_addr_t)NULL;
		}
		break;
	default:
		break;
	}

	return (dma_addr_t)NULL;
}

static int nbl_res_txrx_restart_abnormal_ring(void *priv, int ring_index, int type)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_res_tx_ring *tx_ring = NBL_RES_MGT_TO_TX_RING(res_mgt, ring_index);
	struct nbl_res_rx_ring *rx_ring = NBL_RES_MGT_TO_RX_RING(res_mgt, ring_index);
	struct nbl_res_vector *vector = NULL;
	int ret = 0;

	if (tx_ring && !nbl_txrx_within_vsi(&tx_ring->vsi_info[NBL_VSI_XDP], ring_index))
		vector = NBL_RES_MGT_TO_VECTOR(res_mgt, ring_index);

	switch (type) {
	case NBL_TX:
		if (tx_ring && tx_ring->valid) {
			writel(tx_ring->notify_qid, tx_ring->notify_addr);
			netif_start_subqueue(tx_ring->netdev, ring_index);
		} else {
			ret = -EINVAL;
		}
		break;
	case NBL_RX:
		if (rx_ring && rx_ring->valid)
			nbl_res_txrx_kick_rx_ring(res_mgt, ring_index);
		else
			ret = -EINVAL;
		break;
	default:
		break;
	}

	if (vector) {
		if (vector->net_msix_mask_en)
			writel(vector->irq_data, vector->irq_enable_base);
		vector->started = true;
	}

	return ret;
}

static void nbl_res_txrx_set_xdp_prog(void *priv, void *prog)
{
	int i;
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_rx_ring *rx_ring;
	struct nbl_res_tx_ring *tx_ring;

	for (i = 0; i < txrx_mgt->xdp_ring_num; i++) {
		rx_ring = NBL_RES_MGT_TO_RX_RING(res_mgt, i);
		if (!rx_ring)
			continue;

		WRITE_ONCE(rx_ring->xdp_prog, prog);
	}

	for (i = 0; i < txrx_mgt->xdp_ring_num; i++) {
		tx_ring = NBL_RES_MGT_TO_TX_RING(res_mgt, i + txrx_mgt->xdp_ring_offset);
		if (!tx_ring)
			continue;

		WRITE_ONCE(tx_ring->xdp_prog, prog);
	}
}

static int nbl_res_get_max_mtu(void *priv)
{
	struct nbl_resource_mgt *res_mgt = (struct nbl_resource_mgt *)priv;
	struct nbl_txrx_mgt *txrx_mgt = NBL_RES_MGT_TO_TXRX_MGT(res_mgt);
	struct nbl_res_rx_ring *rx_ring;

	rx_ring = NBL_RES_MGT_TO_RX_RING(res_mgt, 0);

	if (!!(txrx_mgt->xdp_ring_num) && rx_ring->xdp_prog)
		return rx_ring->buf_len - NBL_BUFFER_HDR_LEN - ETH_HLEN - (2 * VLAN_HLEN);
	return NBL_MAX_JUMBO_FRAME_SIZE - NBL_PKT_HDR_PAD;
}

/* NBL_TXRX_SET_OPS(ops_name, func)
 *
 * Use X Macros to reduce setup and remove codes.
 */
#define NBL_TXRX_OPS_TBL								\
do {											\
	NBL_TXRX_SET_OPS(get_resource_pt_ops, nbl_res_get_pt_ops);			\
	NBL_TXRX_SET_OPS(alloc_rings, nbl_res_txrx_alloc_rings);			\
	NBL_TXRX_SET_OPS(remove_rings, nbl_res_txrx_remove_rings);			\
	NBL_TXRX_SET_OPS(start_tx_ring, nbl_res_txrx_start_tx_ring);			\
	NBL_TXRX_SET_OPS(stop_tx_ring, nbl_res_txrx_stop_tx_ring);			\
	NBL_TXRX_SET_OPS(start_rx_ring, nbl_res_txrx_start_rx_ring);			\
	NBL_TXRX_SET_OPS(stop_rx_ring, nbl_res_txrx_stop_rx_ring);			\
	NBL_TXRX_SET_OPS(kick_rx_ring, nbl_res_txrx_kick_rx_ring);			\
	NBL_TXRX_SET_OPS(dump_ring, nbl_res_txrx_dump_ring);				\
	NBL_TXRX_SET_OPS(dump_ring_stats, nbl_res_txrx_dump_ring_stats);		\
	NBL_TXRX_SET_OPS(get_vector_napi, nbl_res_txrx_get_vector_napi);		\
	NBL_TXRX_SET_OPS(set_vector_info, nbl_res_txrx_set_vector_info);		\
	NBL_TXRX_SET_OPS(get_tx_headroom, nbl_res_txrx_get_tx_headroom);		\
	NBL_TXRX_SET_OPS(get_queue_stats, nbl_res_txrx_get_queue_stats);		\
	NBL_TXRX_SET_OPS(get_net_stats, nbl_res_txrx_get_net_stats);			\
	NBL_TXRX_SET_OPS(get_max_desc_num, nbl_res_txrx_get_max_desc_num);		\
	NBL_TXRX_SET_OPS(get_min_desc_num, nbl_res_txrx_get_min_desc_num);		\
	NBL_TXRX_SET_OPS(get_tx_desc_num, nbl_res_txrx_get_tx_desc_num);		\
	NBL_TXRX_SET_OPS(get_rx_desc_num, nbl_res_txrx_get_rx_desc_num);		\
	NBL_TXRX_SET_OPS(set_tx_desc_num, nbl_res_txrx_set_tx_desc_num);		\
	NBL_TXRX_SET_OPS(set_rx_desc_num, nbl_res_txrx_set_rx_desc_num);		\
	NBL_TXRX_SET_OPS(clean_rx_lb_test, nbl_res_txrx_clean_rx_lb_test);		\
	NBL_TXRX_SET_OPS(cfg_duppkt_info, nbl_res_txrx_cfg_duppkt_info);		\
	NBL_TXRX_SET_OPS(stop_abnormal_sw_queue, nbl_res_queue_stop_abnormal_sw_queue);	\
	NBL_TXRX_SET_OPS(restore_abnormal_ring, nbl_res_txrx_restore_abnormal_ring);	\
	NBL_TXRX_SET_OPS(restart_abnormal_ring, nbl_res_txrx_restart_abnormal_ring);	\
	NBL_TXRX_SET_OPS(register_vsi_ring, nbl_txrx_register_vsi_ring);		\
	NBL_TXRX_SET_OPS(cfg_txrx_vlan, nbl_res_txrx_cfg_txrx_vlan);			\
	NBL_TXRX_SET_OPS(set_rings_xdp_prog, nbl_res_txrx_set_xdp_prog);		\
	NBL_TXRX_SET_OPS(register_xdp_rxq, nbl_res_txrx_register_xdp_rxq);		\
	NBL_TXRX_SET_OPS(unregister_xdp_rxq, nbl_res_txrx_unregister_xdp_rxq);		\
	NBL_TXRX_SET_OPS(get_max_mtu, nbl_res_get_max_mtu);				\
} while (0)

/* Structure starts here, adding an op should not modify anything below */
static int nbl_txrx_setup_mgt(struct device *dev, struct nbl_txrx_mgt **txrx_mgt)
{
	*txrx_mgt = devm_kzalloc(dev, sizeof(struct nbl_txrx_mgt), GFP_KERNEL);
	if (!*txrx_mgt)
		return -ENOMEM;

	return 0;
}

static void nbl_txrx_remove_mgt(struct device *dev, struct nbl_txrx_mgt **txrx_mgt)
{
	devm_kfree(dev, *txrx_mgt);
	*txrx_mgt = NULL;
}

int nbl_txrx_mgt_start(struct nbl_resource_mgt *res_mgt)
{
	struct device *dev;
	struct nbl_txrx_mgt **txrx_mgt;

	dev = NBL_RES_MGT_TO_DEV(res_mgt);
	txrx_mgt = &NBL_RES_MGT_TO_TXRX_MGT(res_mgt);

	return nbl_txrx_setup_mgt(dev, txrx_mgt);
}

void nbl_txrx_mgt_stop(struct nbl_resource_mgt *res_mgt)
{
	struct device *dev;
	struct nbl_txrx_mgt **txrx_mgt;

	dev = NBL_RES_MGT_TO_DEV(res_mgt);
	txrx_mgt = &NBL_RES_MGT_TO_TXRX_MGT(res_mgt);

	if (!(*txrx_mgt))
		return;

	nbl_txrx_remove_mgt(dev, txrx_mgt);
}

int nbl_txrx_setup_ops(struct nbl_resource_ops *res_ops)
{
#define NBL_TXRX_SET_OPS(name, func) do {res_ops->NBL_NAME(name) = func; ; } while (0)
	NBL_TXRX_OPS_TBL;
#undef  NBL_TXRX_SET_OPS

	return 0;
}

void nbl_txrx_remove_ops(struct nbl_resource_ops *res_ops)
{
#define NBL_TXRX_SET_OPS(name, func) do {res_ops->NBL_NAME(name) = NULL; ; } while (0)
	NBL_TXRX_OPS_TBL;
#undef  NBL_TXRX_SET_OPS
}
