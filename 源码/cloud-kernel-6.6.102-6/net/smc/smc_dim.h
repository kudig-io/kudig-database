/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022, Alibaba Group.
 */

#ifndef _SMC_DIM_H
#define _SMC_DIM_H

#include <linux/dim.h>
#include <rdma/ib_verbs.h>

struct smc_dim {
	struct dim dim;
	bool use_dim;
	u64 prev_idle;
	u64 prev_softirq;
	u64 prev_wall;
};

static inline struct smc_dim *to_smcdim(struct dim *dim)
{
	return (struct smc_dim *)dim;
}

static inline struct dim *to_dim(struct smc_dim *smcdim)
{
	return (struct dim *)smcdim;
}

void smc_dim_init(struct ib_cq *cq);
void smc_dim_destroy(struct ib_cq *cq);
void smc_dim(struct dim *dim, u64 completions);

#endif /* _SMC_DIM_H */
