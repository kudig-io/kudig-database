/* SPDX-License-Identifier: GPL-2.0*/
/*
 * Copyright (c) 2022 nebula-matrix Limited.
 * Author: Bennie Yan <bennie@nebula-matrix.com>
 */

#ifndef _NBL_FD_H_
#define _NBL_FD_H_

#include "nbl_resource.h"

#define NBL_FD_RULE_MAX_512				(512)
#define NBL_FD_RULE_MAX_1024				(1024)
#define NBL_FD_RULE_MAX_1536				(1536)
#define NBL_FD_RULE_MAX_DEFAULT				(NBL_FD_RULE_MAX_512)
#define NBL_FD_RULE_MAX					(NBL_FD_RULE_MAX_1536)

#define NBL_FD_TCAM_DEPTH				(512)

#define NBL_FD_IPV4_TCAM_WIDTH				(5)
#define NBL_FD_L2_IPV6_TCAM_WIDTH			(10)
#define NBL_FD_DEFAULT_MODE_DEPTH			(1)
#define NBL_FD_LITE_MODE_DEPTH				(4)
#define NBL_FD_FULL_MODE_DEPTH				(1)

#define NBL_FD_UDF_FLEX_WORD_M				GENMASK_ULL(31, 0)
#define NBL_FD_UDF_FLEX_OFFS_S				32
#define NBL_FD_UDF_FLEX_OFFS_M				GENMASK_ULL(63, NBL_FD_UDF_FLEX_OFFS_S)
#define NBL_FD_UDF_FLEX_FLTR_M				GENMASK_ULL(63, 0)

union nbl_fd_tcam_default_data_u {
	struct nbl_fd_tcam_default_data {
		u64 rsv1:12;
		u64 dport:16;
		u64 padding:8;
		u64 l4_proto:8;
		u64 l4_dport:16;
		u64 l4_sport:16;
		u64 ethertype:16;
		u64 src_mac:48;
		u64 dst_mac:48;
		u64 udf:32;
		u64 dip_l:64;
		u64 dip_h:64;
		u64 sip_l:64;
		u64 sip_h:64;
		u64 pid:4;
	} __packed info;
#define NBL_FD_TCAM_DEFAULT_DATA_TAB_WIDTH (sizeof(struct nbl_fd_tcam_default_data) / sizeof(u32))
	u32 data[NBL_FD_TCAM_DEFAULT_DATA_TAB_WIDTH];
	u8 hash_key[sizeof(struct nbl_fd_tcam_default_data)];
};

union nbl_fd_tcam_ipv4_data_u {
	struct nbl_fd_tcam_ipv4_data {
		u64 rsv1:28;
		u64 dport:16;
		u64 padding:8;
		u64 l4_proto:8;
		u64 l4_dport:16;
		u64 l4_sport:16;
		u64 udf:32;
		u64 dip:32;
		u64 sip:32;
		u64 pid:4;
	} __packed info;
#define NBL_FD_TCAM_IPV4_DATA_TAB_WIDTH (sizeof(struct nbl_fd_tcam_ipv4_data) / sizeof(u32))
	u32 data[NBL_FD_TCAM_IPV4_DATA_TAB_WIDTH];
	u8 hash_key[sizeof(struct nbl_fd_tcam_ipv4_data)];
};

union nbl_fd_tcam_l2_ipv6_data_u {
	struct nbl_fd_tcam_l2_ipv6_data {
		u64 rsv:28;
		u64 dport:16;
		u64 padding:8;
		u64 l4_proto:8;
		u64 l4_dport:16;
		u64 l4_sport:16;
		u64 ehtertype:16;
		u64 udf:32;
		u32 dip[NBL_IPV6_U32LEN];
		u32 sip[NBL_IPV6_U32LEN];
		u64 pid:4;
	} __packed info;
#define NBL_FD_TCAM_L2_IPV6_DATA_TAB_WIDTH (sizeof(struct nbl_fd_tcam_l2_ipv6_data) / sizeof(u32))
	u32 data[NBL_FD_TCAM_L2_IPV6_DATA_TAB_WIDTH];
	u8 hash_key[sizeof(struct nbl_fd_tcam_l2_ipv6_data)];
};

struct nbl_fd_tcam_index {
	u16 depth_index;
};

struct nbl_fd_tcam_index_info {
	struct nbl_fd_tcam_index default_index[NBL_FD_DEFAULT_MODE_DEPTH];
	struct nbl_fd_tcam_index v4[NBL_FD_LITE_MODE_DEPTH];
	struct nbl_fd_tcam_index v6[NBL_FD_FULL_MODE_DEPTH];
	u8 default_cnt;
	u8 v4_cnt;
	u8 v6_cnt;
};

#endif
