/* SPDX-License-Identifier: GPL-2.0 */
/**
 * Copyright (C), 2020, Linkdata Technologies Co., Ltd.
 *
 * @file: sxe_compat.h
 * @author: Linkdata
 * @date: 2025.02.16
 * @brief:
 * @note:
 */
#ifndef __SXE_COMPAT_H__
#define __SXE_COMPAT_H__

#include "sxe_compat_gcc.h"
#include <linux/filter.h>
#include <linux/version.h>

#define HAVE_ETHTOOL_COALESCE_EXTACK
#define HAVE_ETHTOOL_EXTENDED_RINGPARAMS

#define HAVE_XDP_SUPPORT
#define HAVE_AF_XDP_ZERO_COPY
#define HAVE_MEM_TYPE_XSK_BUFF_POOL
#define HAVE_XDP_BUFF_DATA_META
#define HAVE_XDP_BUFF_FRAME_SIZE
#define XDP_XMIT_FRAME_FAILED_NEED_FREE
#define HAVE_NETDEV_BPF_XSK_BUFF_POOL
#define HAVE_NETDEV_XDP_FEATURES

#define HAVE_SKB_XMIT_MORE
#define HAVE_TIMEOUT_TXQUEUE_IDX

#define HAVE_NETDEV_NESTED_PRIV
#define HAVE_NET_PREFETCH_API
#define HAVE_NDO_FDB_ADD_EXTACK
#define HAVE_NDO_BRIDGE_SETLINK_EXTACK
#define HAVE_NDO_SET_VF_LINK_STATE
#define HAVE_NDO_XSK_WAKEUP
#define HAVE_MACVLAN_OFFLOAD_SUPPORT

#define HAVE_PTP_CLOCK_INFO_ADJFINE

#define BPF_WARN_INVALID_XDP_ACTION_API_NEED_3_PARAMS
#define u64_stats_fetch_begin_irq u64_stats_fetch_begin
#define u64_stats_fetch_retry_irq u64_stats_fetch_retry
#define CLASS_CREATE_NEED_1_PARAM
#define DEFINE_SEMAPHORE_NEED_CNT
#define DELETE_PCIE_ERROR_REPORTING
#define NETIF_NAPI_ADD_API_NEED_3_PARAMS
#define HAVE_ETH_HW_ADDR_SET_API

#define HAVE_XDP_BUFF_INIT_API
#define HAVE_XDP_PREPARE_BUFF_API
#define HAVE_NDO_ETH_IOCTL

#define SXE_LOG_OLD_FS
#define SXE_LOG_FS_NOTIFY

#endif
