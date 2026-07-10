// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022, Alibaba Group.
 * Copyright (c) 2019, Mellanox Technologies inc.  All rights reserved.
 */

#include <linux/cpufreq.h>
#include "smc_dim.h"

#define SMC_IS_SIGNIFICANT_DIFF(val, ref, threshold) \
	((ref) && (((100UL * abs((val) - (ref))) / (ref)) >= (threshold)))

#define SMC_CPMS_THRESHOLD		5
#define SMC_CPERATIO_THRESHOLD		25
#define SMC_MAX_FLUCTUATIONS		3
#define CPU_IDLE_UTIL_THRESHOLD	5
#define CPU_SOFTIRQ_UTIL_THRESHOLD	10

#define SMC_DIM_PARAMS_NUM_PROFILES	4
#define SMC_DIM_START_PROFILE		0

static const struct dim_cq_moder
smc_dim_profile[SMC_DIM_PARAMS_NUM_PROFILES] = {
	{1,  0, 2,  0},
	{4,  0, 8,  0},
	{16, 0, 16,  0},
	{32, 0, 32, 0},
};

static void smc_dim_work(struct work_struct *w)
{
	struct dim *dim = container_of(w, struct dim, work);
	struct ib_cq *cq = dim->priv;

	u16 usec = smc_dim_profile[dim->profile_ix].usec;
	u16 comps = smc_dim_profile[dim->profile_ix].comps;

	dim->state = DIM_START_MEASURE;
	cq->device->ops.modify_cq(cq, comps, usec);
}

void smc_dim_init(struct ib_cq *cq)
{
	struct smc_dim *smc_dim;
	struct dim *dim;

	if (!cq->device->ops.modify_cq)
		return;

	smc_dim = kzalloc(sizeof(*smc_dim), GFP_KERNEL);
	if (!smc_dim)
		return;

	smc_dim->use_dim = cq->device->use_cq_dim;
	dim = to_dim(smc_dim);
	dim->state = DIM_START_MEASURE;
	dim->tune_state = DIM_GOING_RIGHT;
	dim->profile_ix = SMC_DIM_START_PROFILE;
	dim->priv = cq;
	cq->dim = dim;
	INIT_WORK(&dim->work, smc_dim_work);
}

void smc_dim_destroy(struct ib_cq *cq)
{
	if (!cq->dim)
		return;

	cancel_work_sync(&cq->dim->work);
	kfree(cq->dim);
}

static inline void smc_dim_param_clear(struct dim *dim)
{
	dim->steps_right  = 0;
	dim->steps_left   = 0;
	dim->tired        = 0;
	dim->profile_ix   = SMC_DIM_START_PROFILE;
	dim->tune_state   = DIM_GOING_RIGHT;
}

static inline void smc_dim_reset(struct dim *dim)
{
	int prev_ix = dim->profile_ix;

	smc_dim_param_clear(dim);
	if (prev_ix != dim->profile_ix)
		schedule_work(&dim->work);
	else
		dim->state = DIM_START_MEASURE;
}

static int smc_dim_step(struct dim *dim)
{
	if (dim->tune_state == DIM_GOING_RIGHT) {
		if (dim->profile_ix == (SMC_DIM_PARAMS_NUM_PROFILES - 1))
			return DIM_ON_EDGE;
		dim->profile_ix++;
		dim->steps_right++;
	}
	if (dim->tune_state == DIM_GOING_LEFT) {
		if (dim->profile_ix == 0)
			return DIM_ON_EDGE;
		dim->profile_ix--;
		dim->steps_left++;
	}

	return DIM_STEPPED;
}

static int smc_dim_stats_compare(struct dim_stats *curr, struct dim_stats *prev)
{
	/* first stat */
	if (!prev->cpms)
		return DIM_STATS_BETTER;

	if (SMC_IS_SIGNIFICANT_DIFF(curr->cpms, prev->cpms, SMC_CPMS_THRESHOLD))
		return (curr->cpms > prev->cpms) ? DIM_STATS_BETTER :
						DIM_STATS_WORSE;

	if (SMC_IS_SIGNIFICANT_DIFF(curr->cpe_ratio, prev->cpe_ratio, SMC_CPERATIO_THRESHOLD))
		return (curr->cpe_ratio > prev->cpe_ratio) ? DIM_STATS_BETTER :
						DIM_STATS_WORSE;

	return DIM_STATS_SAME;
}

static void smc_dim_exit_parking(struct dim *dim)
{
	dim->tune_state = dim->profile_ix ? DIM_GOING_LEFT : DIM_GOING_RIGHT;
	smc_dim_step(dim);
	dim->tired = 0;
}

static bool smc_dim_decision(struct dim_stats *curr_stats, struct dim *dim)
{
	int prev_state = dim->tune_state;
	int prev_ix = dim->profile_ix;
	int stats_res = smc_dim_stats_compare(curr_stats,
						  &dim->prev_stats);

	if (curr_stats->cpms < 50) {
		smc_dim_param_clear(dim);
		goto out;
	}

	switch (dim->tune_state) {
	case DIM_PARKING_ON_TOP:
		if (stats_res != DIM_STATS_SAME) {
			if (dim->tired++ > SMC_MAX_FLUCTUATIONS)
				smc_dim_exit_parking(dim);
		} else {
			dim->tired = 0;
		}
		break;
	case DIM_GOING_RIGHT:
	case DIM_GOING_LEFT:
		if (stats_res != DIM_STATS_BETTER) {
			dim_turn(dim);
		} else if (dim_on_top(dim)) {
			dim_park_on_top(dim);
			break;
		}

		if (smc_dim_step(dim) == DIM_ON_EDGE)
			dim_park_on_top(dim);
		break;
	}

out:
	if (prev_state != DIM_PARKING_ON_TOP ||
	    dim->tune_state != DIM_PARKING_ON_TOP)
		dim->prev_stats = *curr_stats;

	return dim->profile_ix != prev_ix;
}

static bool smc_dim_check_utilization(struct dim *dim)
{
	struct smc_dim *smc_dim = to_smcdim(dim);
	int cpu = smp_processor_id();
	struct kernel_cpustat kcpustat;
	u32 idle_percent, softirq_percent;
	u64 wall, wall_idle, diff_wall, softirq;

	wall_idle = get_cpu_idle_time(cpu, &wall, 1);
	kcpustat_cpu_fetch(&kcpustat, cpu);

	softirq = div_u64(kcpustat_field(&kcpustat, CPUTIME_SOFTIRQ, cpu), NSEC_PER_USEC);
	diff_wall = wall - smc_dim->prev_wall;

	/* 100 percent means utilization unsatisfy, do not dim */
	idle_percent = !diff_wall ? 100 :
			div64_u64(100 * (wall_idle - smc_dim->prev_idle), diff_wall);
	softirq_percent = !diff_wall ? 100 :
			  div64_u64(100 * (softirq - smc_dim->prev_softirq), diff_wall);

	smc_dim->prev_softirq = softirq;
	smc_dim->prev_idle = wall_idle;
	smc_dim->prev_wall = wall;

	return idle_percent < CPU_IDLE_UTIL_THRESHOLD &&
			softirq_percent >= CPU_SOFTIRQ_UTIL_THRESHOLD;
}

void smc_dim(struct dim *dim, u64 completions)
{
	struct ib_cq *cq = dim->priv;
	struct smc_dim *smc_dim = to_smcdim(dim);
	struct dim_sample *curr_sample = &dim->measuring_sample;
	struct dim_stats curr_stats;
	u32 nevents;

	if (unlikely(smc_dim->use_dim != cq->device->use_cq_dim)) {
		smc_dim->use_dim = cq->device->use_cq_dim;
		if (!smc_dim->use_dim)
			smc_dim_reset(dim);
	}

	if (!smc_dim->use_dim)
		return;

	dim_update_sample_with_comps(curr_sample->event_ctr + 1, 0, 0,
				     curr_sample->comp_ctr + completions,
				     &dim->measuring_sample);

	switch (dim->state) {
	case DIM_MEASURE_IN_PROGRESS:
		nevents = curr_sample->event_ctr - dim->start_sample.event_ctr;
		if (nevents < DIM_NEVENTS)
			break;
		if (!smc_dim_check_utilization(dim)) {
			smc_dim_reset(dim);
			break;
		}
		dim_calc_stats(&dim->start_sample, curr_sample, &curr_stats);
		if (smc_dim_decision(&curr_stats, dim)) {
			dim->state = DIM_APPLY_NEW_PROFILE;
			schedule_work(&dim->work);
			break;
		}
		fallthrough;
	case DIM_START_MEASURE:
		dim->state = DIM_MEASURE_IN_PROGRESS;
		dim_update_sample_with_comps(curr_sample->event_ctr, 0, 0,
					     curr_sample->comp_ctr,
					     &dim->start_sample);
		break;
	case DIM_APPLY_NEW_PROFILE:
		break;
	}
}
