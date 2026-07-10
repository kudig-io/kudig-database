/* SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause */

/* Authors: Cheng Xu <chengyou@linux.alibaba.com> */
/*          Kai Shen <kaishen@linux.alibaba.com> */
/* Copyright (c) 2020-2022, Alibaba Group. */

/*
 * Copyright 2018-2021 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef __KCOMPAT_H__
#define __KCOMPAT_H__

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <rdma/ib_cache.h>

#define ERDMA_MAJOR_VER 0
#define ERDMA_MEDIUM_VER 2
#define ERDMA_MINOR_VER 38

#include <rdma/ib_verbs.h>
#define RDMA_DRIVER_ERDMA 19

#define upper_16_bits(n) ((u16)((n) >> 16))
#define lower_16_bits(n) ((u16)((n) & 0xffff))

typedef u32 port_t;

#include <rdma/ib_verbs.h>
#include <rdma/rdma_user_cm.h>
#include <net/sock.h>
#include <linux/tcp.h>
#include <linux/sched/signal.h>

#define IB_QP_CREATE_IWARP_WITHOUT_CM (1 << 27)

#endif
