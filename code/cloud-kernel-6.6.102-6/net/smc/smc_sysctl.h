/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  Shared Memory Communications over RDMA (SMC-R) and RoCE
 *
 *  smc_sysctl.c: sysctl interface to SMC subsystem.
 *
 *  Copyright (c) 2022, Alibaba Inc.
 *
 *  Author: Tony Lu <tonylu@linux.alibaba.com>
 *
 */

#ifndef _SMC_SYSCTL_H
#define _SMC_SYSCTL_H

#include <linux/swap.h>

static inline void smc_mem_init(struct net *net)
{
	/* Set memory limits to no more than 1/4 the whole memory */
	unsigned long mem_min = nr_free_buffer_pages() << (PAGE_SHIFT - 2);

	mem_min = max_t(unsigned long, mem_min, SMC_BUF_MIN_SIZE * 2); /* one conn at least */

	if (net_eq(net, &init_net)) {
		atomic_long_set(&smc_global_memory_allocated, 0);
		sysctl_global_mem[0] = mem_min;		/* 25% */
		sysctl_global_mem[1] = mem_min * 2;	/* 50% */
		sysctl_global_mem[2] = mem_min * 3;	/* 75% */
	}

	atomic_long_set(&net->smc.memory_allocated, 0);
	net->smc.sysctl_mem[0] = mem_min;	/* 25% */
	net->smc.sysctl_mem[1] = mem_min * 2;	/* 50% */
	net->smc.sysctl_mem[2] = mem_min * 3;	/* 75% */
}

#ifdef CONFIG_SYSCTL

int __net_init smc_sysctl_net_init(struct net *net);
void __net_exit smc_sysctl_net_exit(struct net *net);

#else

static inline int smc_sysctl_net_init(struct net *net)
{
	net->smc.sysctl_autocorking_size = SMC_AUTOCORKING_DEFAULT_SIZE;
	net->smc.sysctl_max_links_per_lgr = SMC_LINKS_PER_LGR_MAX_PREFER;
	net->smc.sysctl_max_conns_per_lgr = SMC_CONN_PER_LGR_PREFER;
	smc_mem_init(net);
	return 0;
}

static inline void smc_sysctl_net_exit(struct net *net) { }

#endif /* CONFIG_SYSCTL */

#endif /* _SMC_SYSCTL_H */
