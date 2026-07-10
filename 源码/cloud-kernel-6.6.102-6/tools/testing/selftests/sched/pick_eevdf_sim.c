// SPDX-License-Identifier: GPL-2.0-only

#include "../kselftest_harness.h"

#include <errno.h>
#include <limits.h>
#include <linux/rbtree_augmented.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTITIES 512
#define RANDOM_CASES 4000
#define RANDOM_SEEDS 4
#define LARGE_RANDOM_CASES 1500
#define SOAK_CASES 6000
#define EXTRA_SOAK_CASES 20000

struct test_entity {
	struct rb_node run_node;
	u64 vruntime;
	u64 deadline;
	u64 min_vruntime;
	bool expellee;
	bool on_rq;
	int id;
};

struct test_cfs_rq {
	struct rb_root_cached tasks_timeline;
	u64 min_vruntime;
	s64 avg_vruntime;
	long avg_load;
	int nr_queued;
	/* Mirrors rq_on_expel() selecting id_pick_eevdf() in the kernel. */
	bool suppression;
	struct test_entity *curr;
};

static inline bool entity_before(const struct test_entity *a,
					 const struct test_entity *b)
{
	s64 diff = (s64)(a->deadline - b->deadline);

	if (diff)
		return diff < 0;

	return (s64)(a->vruntime - b->vruntime) < 0;
}

static inline bool tree_less(struct rb_node *node, const struct rb_node *parent)
{
	struct test_entity *se = rb_entry(node, struct test_entity, run_node);
	const struct test_entity *p = rb_entry(parent, struct test_entity, run_node);

	return entity_before(se, p);
}

static inline bool min_vruntime_update(struct test_entity *se, bool exit)
{
	u64 old = se->min_vruntime;
	struct rb_node *node = &se->run_node;

	se->min_vruntime = se->vruntime;
	if (node->rb_left) {
		struct test_entity *left = rb_entry(node->rb_left, struct test_entity,
						   run_node);

		if (left->min_vruntime < se->min_vruntime)
			se->min_vruntime = left->min_vruntime;
	}
	if (node->rb_right) {
		struct test_entity *right = rb_entry(node->rb_right, struct test_entity,
						    run_node);

		if (right->min_vruntime < se->min_vruntime)
			se->min_vruntime = right->min_vruntime;
	}

	return exit && se->min_vruntime == old;
}

RB_DECLARE_CALLBACKS(static, min_vruntime_cb, struct test_entity,
			     run_node, min_vruntime, min_vruntime_update);

static void enqueue_entity(struct test_cfs_rq *cfs_rq, struct test_entity *se)
{
	struct rb_node **link = &cfs_rq->tasks_timeline.rb_root.rb_node;
	struct rb_node *parent = NULL;
	bool leftmost = true;

	se->min_vruntime = se->vruntime;

	while (*link) {
		struct test_entity *cur;

		parent = *link;
		cur = rb_entry(parent, struct test_entity, run_node);
		if (entity_before(se, cur)) {
			link = &parent->rb_left;
		} else {
			link = &parent->rb_right;
			leftmost = false;
		}
	}

	rb_link_node(&se->run_node, parent, link);
	min_vruntime_cb_propagate(parent, NULL);
	rb_insert_augmented_cached(&se->run_node, &cfs_rq->tasks_timeline, leftmost,
				   &min_vruntime_cb);
	cfs_rq->nr_queued++;
}

static inline bool should_expel_se(const struct test_entity *se)
{
	return se->expellee;
}

static inline bool vruntime_eligible_pick_avg(const struct test_cfs_rq *cfs_rq,
					      s64 avg, long load,
					      u64 vruntime)
{
	return avg >= (s64)(vruntime - cfs_rq->min_vruntime) * load;
}

static inline s64 entity_eligibility_deficit(const struct test_cfs_rq *cfs_rq,
					     s64 avg, long load,
					     const struct test_entity *se)
{
	return (s64)(se->vruntime - cfs_rq->min_vruntime) * load - avg;
}

static inline bool better_fallback_entity(const struct test_cfs_rq *cfs_rq,
					  s64 avg, long load,
					  const struct test_entity *se,
					  const struct test_entity *best)
{
	s64 deficit = entity_eligibility_deficit(cfs_rq, avg, load, se);
	s64 best_deficit = entity_eligibility_deficit(cfs_rq, avg, load, best);

	if (deficit < best_deficit)
		return true;
	if (deficit > best_deficit)
		return false;

	return entity_before(se, best);
}

static void pick_eligible_data_normal(const struct test_cfs_rq *cfs_rq,
					   s64 *avg, long *load)
{
	*avg = cfs_rq->avg_vruntime;
	*load = cfs_rq->avg_load;

	if (cfs_rq->curr && cfs_rq->curr->on_rq) {
		*avg += (s64)(cfs_rq->curr->vruntime - cfs_rq->min_vruntime);
		*load += 1;
	}
}

static void pick_eligible_data_suppressed(const struct test_cfs_rq *cfs_rq,
					      s64 *avg, long *load)
{
	*avg = cfs_rq->avg_vruntime;
	*load = cfs_rq->avg_load;

	if (cfs_rq->curr && cfs_rq->curr->on_rq && !should_expel_se(cfs_rq->curr)) {
		*avg += (s64)(cfs_rq->curr->vruntime - cfs_rq->min_vruntime);
		*load += 1;
	}
}

static struct test_entity *oracle_pick_normal(const struct test_cfs_rq *cfs_rq,
					      struct test_entity *entities,
					      int nr_entities)
{
	struct test_entity *best = NULL;
	struct test_entity *curr = cfs_rq->curr;
	int i;

	if (curr && (!curr->on_rq || !vruntime_eligible_pick_avg(cfs_rq,
						     cfs_rq->avg_vruntime,
						     cfs_rq->avg_load,
						     curr->vruntime)))
		curr = NULL;

	for (i = 0; i < nr_entities; i++) {
		struct test_entity *se = &entities[i];

		if (vruntime_eligible_pick_avg(cfs_rq, cfs_rq->avg_vruntime,
					     cfs_rq->avg_load,
					     se->vruntime) &&
		    (!best || entity_before(se, best)))
			best = se;
	}

	if (!best || (curr && entity_before(curr, best)))
		best = curr;

	return best;
}

static struct test_entity *oracle_pick_suppressed(const struct test_cfs_rq *cfs_rq,
						  struct test_entity *entities,
						  int nr_entities)
{
	struct test_entity *best = NULL;
	struct test_entity *fallback = NULL;
	struct test_entity *curr = cfs_rq->curr;
	s64 avg;
	long load;
	int i;

	pick_eligible_data_suppressed(cfs_rq, &avg, &load);

	if (curr) {
		if (should_expel_se(curr) || !curr->on_rq)
			curr = NULL;
		else if (vruntime_eligible_pick_avg(cfs_rq, avg, load, curr->vruntime))
			best = curr;
		else
			fallback = curr;
	}

	for (i = 0; i < nr_entities; i++) {
		struct test_entity *se = &entities[i];
		bool eligible;

		if (should_expel_se(se))
			continue;

		eligible = vruntime_eligible_pick_avg(cfs_rq, avg, load, se->vruntime);
		if (eligible) {
			if (!best || entity_before(se, best))
				best = se;
			continue;
		}

		if (!fallback || better_fallback_entity(cfs_rq, avg, load, se, fallback))
			fallback = se;
	}

	if (best)
		return best;

	return fallback;
}

static struct test_entity *oracle_pick(const struct test_cfs_rq *cfs_rq,
				       struct test_entity *entities,
				       int nr_entities)
{
	if (cfs_rq->suppression)
		return oracle_pick_suppressed(cfs_rq, entities, nr_entities);

	return oracle_pick_normal(cfs_rq, entities, nr_entities);
}

static struct test_entity *sim_pick_eevdf_normal(struct test_cfs_rq *cfs_rq)
{
	struct rb_node *node = cfs_rq->tasks_timeline.rb_root.rb_node;
	struct test_entity *se = NULL;
	struct test_entity *best = NULL;

	if (cfs_rq->nr_queued == 0)
		return NULL;

	if (rb_first_cached(&cfs_rq->tasks_timeline))
		se = rb_entry(rb_first_cached(&cfs_rq->tasks_timeline),
				      struct test_entity, run_node);

	if (cfs_rq->nr_queued == 1)
		return cfs_rq->curr && cfs_rq->curr->on_rq ? cfs_rq->curr : se;

	if (cfs_rq->curr && (!cfs_rq->curr->on_rq ||
		!vruntime_eligible_pick_avg(cfs_rq, cfs_rq->avg_vruntime,
					    cfs_rq->avg_load,
					    cfs_rq->curr->vruntime)))
		cfs_rq->curr = NULL;

	if (se && vruntime_eligible_pick_avg(cfs_rq, cfs_rq->avg_vruntime,
					     cfs_rq->avg_load,
					     se->vruntime))
		best = se;

	while (node) {
		struct rb_node *left = node->rb_left;

		if (left) {
			struct test_entity *left_se = rb_entry(left, struct test_entity,
						      run_node);

			if (vruntime_eligible_pick_avg(cfs_rq, cfs_rq->avg_vruntime,
						       cfs_rq->avg_load,
						       left_se->min_vruntime)) {
				node = left;
				continue;
			}
		}

		se = rb_entry(node, struct test_entity, run_node);
		if (vruntime_eligible_pick_avg(cfs_rq, cfs_rq->avg_vruntime,
					     cfs_rq->avg_load,
					     se->vruntime)) {
			best = se;
			break;
		}

		node = node->rb_right;
	}

	if (!best || (cfs_rq->curr && entity_before(cfs_rq->curr, best)))
		best = cfs_rq->curr;

	return best;
}

static struct test_entity *sim_pick_eevdf_suppressed(struct test_cfs_rq *cfs_rq)
{
	struct rb_node *node = cfs_rq->tasks_timeline.rb_root.rb_node;
	struct test_entity *se = NULL;
	struct test_entity *best = NULL;
	struct test_entity *fallback = NULL;
	s64 avg;
	long load;

	if (cfs_rq->nr_queued == 0)
		return NULL;

	pick_eligible_data_suppressed(cfs_rq, &avg, &load);

	if (cfs_rq->curr) {
		if (should_expel_se(cfs_rq->curr) || !cfs_rq->curr->on_rq)
			cfs_rq->curr = NULL;
		else if (vruntime_eligible_pick_avg(cfs_rq, avg, load,
						   cfs_rq->curr->vruntime))
			best = cfs_rq->curr;
		else
			fallback = cfs_rq->curr;
	}

	if (rb_first_cached(&cfs_rq->tasks_timeline))
		se = rb_entry(rb_first_cached(&cfs_rq->tasks_timeline),
				      struct test_entity, run_node);
	if (se && !should_expel_se(se)) {
		if (vruntime_eligible_pick_avg(cfs_rq, avg, load, se->vruntime)) {
			best = se;
			goto found;
		}

		if (!fallback || better_fallback_entity(cfs_rq, avg, load, se,
						       fallback))
			fallback = se;
	}

	while (node) {
		struct rb_node *left = node->rb_left;

		if (left) {
			struct test_entity *left_se = rb_entry(left, struct test_entity,
						      run_node);

			if (vruntime_eligible_pick_avg(cfs_rq, avg, load,
						       left_se->min_vruntime)) {
				node = left;
				continue;
			}
		}

		se = rb_entry(node, struct test_entity, run_node);

		if (vruntime_eligible_pick_avg(cfs_rq, avg, load, se->vruntime)) {
			for (node = &se->run_node; node; node = rb_next(node)) {
				se = rb_entry(node, struct test_entity, run_node);

				if (!should_expel_se(se) &&
				    (!fallback || better_fallback_entity(cfs_rq, avg,
							      load, se,
							      fallback)))
					fallback = se;

				if (!vruntime_eligible_pick_avg(cfs_rq, avg, load,
							       se->vruntime))
					continue;
				if (should_expel_se(se))
					continue;

				best = se;
				break;
			}
			break;
		}

		node = node->rb_right;
	}

	for (node = rb_first_cached(&cfs_rq->tasks_timeline); node;
	     node = rb_next(node)) {
		se = rb_entry(node, struct test_entity, run_node);

		if (!should_expel_se(se) &&
		    (!fallback || better_fallback_entity(cfs_rq, avg, load, se,
						      fallback)))
			fallback = se;
	}
found:
	if (best && (!fallback || entity_before(best, fallback)))
		return best;
	if (best)
		return best;
	if (fallback)
		return fallback;

	return cfs_rq->curr;
}

static struct test_entity *sim_pick_eevdf(struct test_cfs_rq *cfs_rq)
{
	/*
	 * Kernel mapping:
	 *   suppression == false -> __pick_eevdf()
	 *   suppression == true  -> id_pick_eevdf() via rq_on_expel()
	 */
	if (cfs_rq->suppression)
		return sim_pick_eevdf_suppressed(cfs_rq);

	return sim_pick_eevdf_normal(cfs_rq);
}

static void dump_entities(struct __test_metadata *_metadata,
			  const struct test_cfs_rq *cfs_rq,
			  struct test_entity *entities,
			  int nr_entities)
{
	int i;

	TH_LOG("min_vruntime=%llu avg_vruntime=%lld avg_load=%ld nr=%d",
	       (unsigned long long)cfs_rq->min_vruntime,
	       (long long)cfs_rq->avg_vruntime, cfs_rq->avg_load, nr_entities);
	for (i = 0; i < nr_entities; i++) {
		s64 deficit = entity_eligibility_deficit(cfs_rq, cfs_rq->avg_vruntime,
							cfs_rq->avg_load,
							&entities[i]);

		TH_LOG("id=%d vr=%llu dl=%llu exp=%d on=%d deficit=%lld eligible=%d",
		       entities[i].id,
		       (unsigned long long)entities[i].vruntime,
		       (unsigned long long)entities[i].deadline,
		       entities[i].expellee,
		       entities[i].on_rq,
		       (long long)deficit,
		       vruntime_eligible_pick_avg(cfs_rq, cfs_rq->avg_vruntime,
						     cfs_rq->avg_load,
						     entities[i].vruntime));	}
}

static void init_cfs_rq(struct test_cfs_rq *cfs_rq, u64 min_vruntime,
			s64 avg_vruntime, long avg_load)
{
	memset(cfs_rq, 0, sizeof(*cfs_rq));
	cfs_rq->tasks_timeline = RB_ROOT_CACHED;
	cfs_rq->min_vruntime = min_vruntime;
	cfs_rq->avg_vruntime = avg_vruntime;
	cfs_rq->avg_load = avg_load;
	cfs_rq->suppression = true;
}

static void init_entity(struct test_entity *se, int id, u64 vruntime,
			u64 deadline, bool expellee)
{
	memset(se, 0, sizeof(*se));
	se->id = id;
	se->vruntime = vruntime;
	se->deadline = deadline;
	se->expellee = expellee;
	se->on_rq = true;
	se->min_vruntime = vruntime;
}

static void build_tree(struct test_cfs_rq *cfs_rq, struct test_entity *entities,
		       int nr_entities, const int *order)
{
	int i;

	for (i = 0; i < nr_entities; i++)
		enqueue_entity(cfs_rq, &entities[order[i]]);
}

static bool entity_same_key(const struct test_entity *a,
			    const struct test_entity *b)
{
	return !entity_before(a, b) && !entity_before(b, a);
}

static bool picks_are_semantically_equal(const struct test_cfs_rq *cfs_rq,
					 const struct test_entity *sim,
					 const struct test_entity *oracle)
{
	s64 avg;
	long load;
	bool sim_eligible;
	bool oracle_eligible;

	if (cfs_rq->suppression)
		pick_eligible_data_suppressed(cfs_rq, &avg, &load);
	else {
		avg = cfs_rq->avg_vruntime;
		load = cfs_rq->avg_load;
	}

	if (sim == oracle)
		return true;
	if (!sim || !oracle)
		return false;
	if (should_expel_se(sim) || should_expel_se(oracle))
		return false;
	if (!entity_same_key(sim, oracle))
		return false;

	sim_eligible = vruntime_eligible_pick_avg(cfs_rq, avg, load, sim->vruntime);
	oracle_eligible = vruntime_eligible_pick_avg(cfs_rq, avg,
						     load,
						     oracle->vruntime);
	if (sim_eligible != oracle_eligible)
		return false;
	if (!sim_eligible &&
	    entity_eligibility_deficit(cfs_rq, avg, load, sim) !=
	    entity_eligibility_deficit(cfs_rq, avg, load, oracle))
		return false;

	return true;
}

static void expect_pick_matches(struct __test_metadata *_metadata,
				const struct test_cfs_rq *cfs_rq,
				struct test_entity *entities,
				int nr_entities,
				struct test_entity *sim,
				struct test_entity *oracle)
{
	if (!picks_are_semantically_equal(cfs_rq, sim, oracle)) {
		dump_entities(_metadata, cfs_rq, entities, nr_entities);
		TH_LOG("sim=%d oracle=%d sim=(%llu,%llu) oracle=(%llu,%llu)",
		       sim ? sim->id : -1,
		       oracle ? oracle->id : -1,
		       sim ? (unsigned long long)sim->deadline : 0,
		       sim ? (unsigned long long)sim->vruntime : 0,
		       oracle ? (unsigned long long)oracle->deadline : 0,
		       oracle ? (unsigned long long)oracle->vruntime : 0);	}
	ASSERT_TRUE(picks_are_semantically_equal(cfs_rq, sim, oracle));
}

TEST(pick_eligible_nonexpellee)
{
	struct test_cfs_rq cfs_rq;
	struct test_entity entities[3];
	int order[] = { 0, 1, 2 };
	struct test_entity *sim;
	struct test_entity *oracle;

	init_cfs_rq(&cfs_rq, 100, 50, 10);
	init_entity(&entities[0], 0, 100, 1000, true);
	init_entity(&entities[1], 1, 102, 1010, false);
	init_entity(&entities[2], 2, 109, 1020, false);
	build_tree(&cfs_rq, entities, ARRAY_SIZE(entities), order);

	sim = sim_pick_eevdf(&cfs_rq);
	oracle = oracle_pick(&cfs_rq, entities, ARRAY_SIZE(entities));
	expect_pick_matches(_metadata, &cfs_rq, entities, ARRAY_SIZE(entities),
			   sim, oracle);
	ASSERT_NE(NULL, sim);
	EXPECT_EQ(1, sim->id);
}

TEST(pick_single_nonexpellee_fallback)
{
	struct test_cfs_rq cfs_rq;
	struct test_entity entities[4];
	int order[] = { 0, 1, 2, 3 };
	struct test_entity *sim;
	struct test_entity *oracle;

	init_cfs_rq(&cfs_rq, 100, 20, 10);
	init_entity(&entities[0], 0, 100, 1000, true);
	init_entity(&entities[1], 1, 101, 1010, true);
	init_entity(&entities[2], 2, 107, 1020, false);
	init_entity(&entities[3], 3, 102, 1030, true);
	build_tree(&cfs_rq, entities, ARRAY_SIZE(entities), order);

	sim = sim_pick_eevdf(&cfs_rq);
	oracle = oracle_pick(&cfs_rq, entities, ARRAY_SIZE(entities));
	expect_pick_matches(_metadata, &cfs_rq, entities, ARRAY_SIZE(entities),
			   sim, oracle);
	ASSERT_NE(NULL, sim);
	EXPECT_EQ(2, sim->id);
}

TEST(pick_eligible_after_expellee_scan)
{
	struct test_cfs_rq cfs_rq;
	struct test_entity entities[5];
	int order[] = { 0, 1, 2, 3, 4 };
	struct test_entity *sim;
	struct test_entity *oracle;

	init_cfs_rq(&cfs_rq, 100, 20, 10);
	init_entity(&entities[0], 0, 100, 1000, true);
	init_entity(&entities[1], 1, 101, 1010, true);
	init_entity(&entities[2], 2, 102, 1020, false);
	init_entity(&entities[3], 3, 106, 1030, false);
	init_entity(&entities[4], 4, 104, 1040, true);
	build_tree(&cfs_rq, entities, ARRAY_SIZE(entities), order);

	sim = sim_pick_eevdf(&cfs_rq);
	oracle = oracle_pick(&cfs_rq, entities, ARRAY_SIZE(entities));
	expect_pick_matches(_metadata, &cfs_rq, entities, ARRAY_SIZE(entities),
			   sim, oracle);
	ASSERT_NE(NULL, sim);
	EXPECT_EQ(2, sim->id);
}

TEST(pick_minimum_deficit_fallback)
{
	struct test_cfs_rq cfs_rq;
	struct test_entity entities[5];
	int order[] = { 0, 1, 2, 3, 4 };
	struct test_entity *sim;
	struct test_entity *oracle;

	init_cfs_rq(&cfs_rq, 100, 25, 10);
	init_entity(&entities[0], 0, 100, 1000, true);
	init_entity(&entities[1], 1, 104, 1010, false);
	init_entity(&entities[2], 2, 103, 1020, false);
	init_entity(&entities[3], 3, 108, 1030, false);
	init_entity(&entities[4], 4, 101, 1040, true);
	build_tree(&cfs_rq, entities, ARRAY_SIZE(entities), order);

	sim = sim_pick_eevdf(&cfs_rq);
	oracle = oracle_pick(&cfs_rq, entities, ARRAY_SIZE(entities));
	expect_pick_matches(_metadata, &cfs_rq, entities, ARRAY_SIZE(entities),
			   sim, oracle);
	ASSERT_NE(NULL, sim);
	EXPECT_EQ(2, sim->id);
}

TEST(pick_deficit_tie_breaks_by_deadline)
{
	struct test_cfs_rq cfs_rq;
	struct test_entity entities[4];
	int order[] = { 0, 1, 2, 3 };
	struct test_entity *sim;
	struct test_entity *oracle;

	init_cfs_rq(&cfs_rq, 100, 20, 10);
	init_entity(&entities[0], 0, 100, 1000, true);
	init_entity(&entities[1], 1, 103, 1020, false);
	init_entity(&entities[2], 2, 103, 1010, false);
	init_entity(&entities[3], 3, 101, 1030, true);
	build_tree(&cfs_rq, entities, ARRAY_SIZE(entities), order);

	sim = sim_pick_eevdf(&cfs_rq);
	oracle = oracle_pick(&cfs_rq, entities, ARRAY_SIZE(entities));
	expect_pick_matches(_metadata, &cfs_rq, entities, ARRAY_SIZE(entities),
			   sim, oracle);
	ASSERT_NE(NULL, sim);
	EXPECT_EQ(2, sim->id);
}

TEST(pick_eligible_beats_closer_ineligible_fallback)
{
	struct test_cfs_rq cfs_rq;
	struct test_entity entities[5];
	int order[] = { 0, 1, 2, 3, 4 };
	struct test_entity *sim;
	struct test_entity *oracle;

	init_cfs_rq(&cfs_rq, 100, 20, 10);
	init_entity(&entities[0], 0, 100, 1000, true);
	init_entity(&entities[1], 1, 102, 1040, false);
	init_entity(&entities[2], 2, 103, 1010, false);
	init_entity(&entities[3], 3, 101, 1020, true);
	init_entity(&entities[4], 4, 104, 1030, true);
	build_tree(&cfs_rq, entities, ARRAY_SIZE(entities), order);

	sim = sim_pick_eevdf(&cfs_rq);
	oracle = oracle_pick(&cfs_rq, entities, ARRAY_SIZE(entities));
	expect_pick_matches(_metadata, &cfs_rq, entities, ARRAY_SIZE(entities),
			   sim, oracle);
	ASSERT_NE(NULL, sim);
	EXPECT_EQ(1, sim->id);
}

TEST(pick_identical_eligible_keys_allow_either_matching_winner)
{
	struct test_cfs_rq cfs_rq;
	struct test_entity entities[5];
	int order[] = { 2, 0, 4, 1, 3 };
	struct test_entity *sim;
	struct test_entity *oracle;

	init_cfs_rq(&cfs_rq, 100, 20, 10);
	init_entity(&entities[0], 0, 102, 1010, false);
	init_entity(&entities[1], 1, 102, 1010, false);
	init_entity(&entities[2], 2, 100, 1000, true);
	init_entity(&entities[3], 3, 103, 1020, true);
	init_entity(&entities[4], 4, 104, 1030, true);
	build_tree(&cfs_rq, entities, ARRAY_SIZE(entities), order);

	sim = sim_pick_eevdf(&cfs_rq);
	oracle = oracle_pick(&cfs_rq, entities, ARRAY_SIZE(entities));
	expect_pick_matches(_metadata, &cfs_rq, entities, ARRAY_SIZE(entities),
			   sim, oracle);
	ASSERT_NE(NULL, sim);
	EXPECT_FALSE(sim->expellee);
	EXPECT_EQ(102ULL, sim->vruntime);
	EXPECT_EQ(1010ULL, sim->deadline);
	EXPECT_TRUE(sim == &entities[0] || sim == &entities[1]);
}

TEST(pick_identical_fallback_keys_allow_either_matching_winner)
{
	struct test_cfs_rq cfs_rq;
	struct test_entity entities[5];
	int order[] = { 3, 1, 4, 0, 2 };
	struct test_entity *sim;
	struct test_entity *oracle;

	init_cfs_rq(&cfs_rq, 100, 5, 10);
	init_entity(&entities[0], 0, 102, 1010, false);
	init_entity(&entities[1], 1, 102, 1010, false);
	init_entity(&entities[2], 2, 100, 1000, true);
	init_entity(&entities[3], 3, 101, 1020, true);
	init_entity(&entities[4], 4, 104, 1030, true);
	build_tree(&cfs_rq, entities, ARRAY_SIZE(entities), order);

	sim = sim_pick_eevdf(&cfs_rq);
	oracle = oracle_pick(&cfs_rq, entities, ARRAY_SIZE(entities));
	expect_pick_matches(_metadata, &cfs_rq, entities, ARRAY_SIZE(entities),
			   sim, oracle);
	ASSERT_NE(NULL, sim);
	EXPECT_FALSE(sim->expellee);
	EXPECT_EQ(102ULL, sim->vruntime);
	EXPECT_EQ(1010ULL, sim->deadline);
	EXPECT_TRUE(sim == &entities[0] || sim == &entities[1]);
}

static void permute_unique_nonexpellee(struct __test_metadata *_metadata,
				       struct test_entity *entities,
				       int *order, int l, int r,
				       int nr_entities, int unique_id,
				       int *checked)
{
	int i;

	if (l == r) {
		struct test_cfs_rq cfs_rq;
		struct test_entity local[MAX_ENTITIES];
		struct test_entity *sim;
		struct test_entity *oracle;

		memcpy(local, entities, sizeof(local[0]) * nr_entities);
		init_cfs_rq(&cfs_rq, 100, 20, 10);
		build_tree(&cfs_rq, local, nr_entities, order);
		sim = sim_pick_eevdf(&cfs_rq);
		oracle = oracle_pick(&cfs_rq, local, nr_entities);
		expect_pick_matches(_metadata, &cfs_rq, local, nr_entities, sim,
				   oracle);
		ASSERT_NE(NULL, sim);
		EXPECT_EQ(unique_id, sim->id);
		(*checked)++;
		return;
	}

	for (i = l; i <= r; i++) {
		int tmp = order[l];

		order[l] = order[i];
		order[i] = tmp;
		permute_unique_nonexpellee(_metadata, entities, order, l + 1, r,
					  nr_entities, unique_id, checked);
		tmp = order[l];
		order[l] = order[i];
		order[i] = tmp;
	}
}

TEST(pick_unique_nonexpellee_across_permutations)
{
	struct test_entity entities[5];
	int order[] = { 0, 1, 2, 3, 4 };
	int checked = 0;

	init_entity(&entities[0], 0, 100, 1000, true);
	init_entity(&entities[1], 1, 101, 1010, true);
	init_entity(&entities[2], 2, 107, 1020, false);
	init_entity(&entities[3], 3, 102, 1030, true);
	init_entity(&entities[4], 4, 103, 1040, true);

	permute_unique_nonexpellee(_metadata, entities, order, 0,
				  ARRAY_SIZE(order) - 1,
				  ARRAY_SIZE(entities), 2, &checked);
	ASSERT_GT(checked, 0);
}

static void permute_unique_eligible_nonexpellee(struct __test_metadata *_metadata,
						struct test_entity *entities,
						int *order, int l, int r,
						int nr_entities, int unique_id,
						int *checked)
{
	int i;

	if (l == r) {
		struct test_cfs_rq cfs_rq;
		struct test_entity local[MAX_ENTITIES];
		struct test_entity *sim;
		struct test_entity *oracle;

		memcpy(local, entities, sizeof(local[0]) * nr_entities);
		init_cfs_rq(&cfs_rq, 100, 20, 10);
		build_tree(&cfs_rq, local, nr_entities, order);
		sim = sim_pick_eevdf(&cfs_rq);
		oracle = oracle_pick(&cfs_rq, local, nr_entities);
		expect_pick_matches(_metadata, &cfs_rq, local, nr_entities, sim,
				   oracle);
		ASSERT_NE(NULL, sim);
		EXPECT_EQ(unique_id, sim->id);
		EXPECT_FALSE(sim->expellee);
		EXPECT_TRUE(vruntime_eligible_pick_avg(&cfs_rq, cfs_rq.avg_vruntime,
						     cfs_rq.avg_load,
						     sim->vruntime));
		(*checked)++;
		return;
	}

	for (i = l; i <= r; i++) {
		int tmp = order[l];

		order[l] = order[i];
		order[i] = tmp;
		permute_unique_eligible_nonexpellee(_metadata, entities, order, l + 1, r,
						   nr_entities, unique_id,
						   checked);
		tmp = order[l];
		order[l] = order[i];
		order[i] = tmp;
	}
}

TEST(pick_unique_eligible_nonexpellee_across_permutations)
{
	struct test_entity entities[5];
	int order[] = { 0, 1, 2, 3, 4 };
	int checked = 0;

	init_entity(&entities[0], 0, 100, 1000, true);
	init_entity(&entities[1], 1, 101, 1010, true);
	init_entity(&entities[2], 2, 102, 1020, false);
	init_entity(&entities[3], 3, 103, 1030, true);
	init_entity(&entities[4], 4, 104, 1040, true);

	permute_unique_eligible_nonexpellee(_metadata, entities, order, 0,
					   ARRAY_SIZE(order) - 1,
					   ARRAY_SIZE(entities), 2, &checked);
	ASSERT_GT(checked, 0);
}

static void permute_size_bounded_unique_nonexpellee_at_index(struct __test_metadata *_metadata,
						     int nr_entities,
						     int unique_idx)
{
	struct test_entity entities[MAX_ENTITIES];
	int order[MAX_ENTITIES];
	int checked = 0;
	int i;

	for (i = 0; i < nr_entities; i++) {
		init_entity(&entities[i], i, 100 + i, 1000 + i * 11, true);
		order[i] = i;
	}

	entities[unique_idx].expellee = false;
	entities[unique_idx].vruntime = 108;
	entities[unique_idx].min_vruntime = 108;

	permute_unique_nonexpellee(_metadata, entities, order, 0,
				  nr_entities - 1,
				  nr_entities, unique_idx, &checked);
	ASSERT_GT(checked, 0);
}

TEST(pick_unique_nonexpellee_various_sizes)
{
	int nr_entities;
	int unique_idx;

	for (nr_entities = 3; nr_entities <= 8; nr_entities++) {
		for (unique_idx = 0; unique_idx < nr_entities; unique_idx++)
			permute_size_bounded_unique_nonexpellee_at_index(_metadata,
								 nr_entities,
								 unique_idx);
	}
}

static void
permute_unique_nonexpellee_at_index(struct __test_metadata *_metadata,
				    int nr_entities, int unique_idx)
{
	struct test_entity entities[MAX_ENTITIES];
	int order[MAX_ENTITIES];
	int checked = 0;
	int i;

	for (i = 0; i < nr_entities; i++) {
		init_entity(&entities[i], i, 100 + i, 1000 + i * 11, true);
		order[i] = i;
	}

	entities[unique_idx].expellee = false;
	entities[unique_idx].vruntime = 101;
	entities[unique_idx].min_vruntime = 101;

	permute_unique_eligible_nonexpellee(_metadata, entities, order, 0,
					   nr_entities - 1,
					   nr_entities, unique_idx, &checked);
	ASSERT_GT(checked, 0);
}

TEST(pick_unique_eligible_nonexpellee_various_sizes)
{
	int nr_entities;
	int unique_idx;

	for (nr_entities = 3; nr_entities <= 8; nr_entities++) {
		for (unique_idx = 0; unique_idx < nr_entities; unique_idx++)
			permute_unique_nonexpellee_at_index(_metadata, nr_entities, unique_idx);
	}
}

static bool next_permutation(int *order, int nr)
{
	int i;
	int j;

	for (i = nr - 2; i >= 0; i--) {
		if (order[i] < order[i + 1])
			break;
	}
	if (i < 0)
		return false;

	for (j = nr - 1; j > i; j--) {
		if (order[j] > order[i])
			break;
	}

	{
		int tmp = order[i];

		order[i] = order[j];
		order[j] = tmp;
	}

	for (j = nr - 1, i++; i < j; i++, j--) {
		int tmp = order[i];

		order[i] = order[j];
		order[j] = tmp;
	}

	return true;
}

static void exhaust_small_state_space(struct __test_metadata *_metadata,
				      int nr_entities)
{
	struct test_entity entities[MAX_ENTITIES];
	int order[MAX_ENTITIES];
	uint32_t mask_limit = 1U << nr_entities;
	uint32_t vruntime_states = 1;
	uint32_t state;
	int i;

	for (i = 0; i < nr_entities; i++) {
		order[i] = i;
		vruntime_states *= 3;
	}

	for (state = 0; state < vruntime_states; state++) {
		uint32_t tmp = state;

		for (i = 0; i < nr_entities; i++) {
			u64 vruntime = 100 + (tmp % 3);
			u64 deadline = 1000 + i * 17;

			init_entity(&entities[i], i, vruntime, deadline, true);
			tmp /= 3;
		}

		do {
			uint32_t mask;

			for (mask = 1; mask < mask_limit; mask++) {
				struct test_cfs_rq cfs_rq;
				struct test_entity local[MAX_ENTITIES];
				struct test_entity *sim;
				struct test_entity *oracle;

				memcpy(local, entities, sizeof(local[0]) * nr_entities);
				for (i = 0; i < nr_entities; i++)
					local[i].expellee = !!(mask & (1U << i));

				if (mask == mask_limit - 1)
					local[0].expellee = false;

				init_cfs_rq(&cfs_rq, 100, 10, 10);
				build_tree(&cfs_rq, local, nr_entities, order);
				sim = sim_pick_eevdf(&cfs_rq);
				oracle = oracle_pick(&cfs_rq, local, nr_entities);
				expect_pick_matches(_metadata, &cfs_rq, local, nr_entities,
						   sim, oracle);
				ASSERT_NE(NULL, sim);
				EXPECT_FALSE(sim->expellee);
			}
		} while (next_permutation(order, nr_entities));
	}
}

TEST(pick_exhaustive_small_state_space)
{
	exhaust_small_state_space(_metadata, 4);
}

TEST(pick_parameter_matrix_matches_oracle)
{
	static const u64 min_vruntimes[] = { 100, 1000 };
	static const s64 avg_vruntimes[] = { 5, 13, 27, 55 };
	static const long loads[] = { 1, 3, 7, 11 };
	int min_idx;
	int avg_idx;
	int load_idx;

	for (min_idx = 0; min_idx < ARRAY_SIZE(min_vruntimes); min_idx++) {
		for (avg_idx = 0; avg_idx < ARRAY_SIZE(avg_vruntimes); avg_idx++) {
			for (load_idx = 0; load_idx < ARRAY_SIZE(loads); load_idx++) {
				struct test_cfs_rq cfs_rq;
				struct test_entity entities[6];
				int order[] = { 3, 1, 5, 0, 4, 2 };
				struct test_entity *sim;
				struct test_entity *oracle;
				u64 base = min_vruntimes[min_idx];

				init_cfs_rq(&cfs_rq, base, avg_vruntimes[avg_idx],
					    loads[load_idx]);
				init_entity(&entities[0], 0, base + 0, 1000, true);
				init_entity(&entities[1], 1, base + 1, 1010, false);
				init_entity(&entities[2], 2, base + 2, 1020, false);
				init_entity(&entities[3], 3, base + 3, 1005, true);
				init_entity(&entities[4], 4, base + 4, 1015, false);
				init_entity(&entities[5], 5, base + 2, 1008, false);
				build_tree(&cfs_rq, entities, ARRAY_SIZE(entities), order);

				sim = sim_pick_eevdf(&cfs_rq);
				oracle = oracle_pick(&cfs_rq, entities, ARRAY_SIZE(entities));
				expect_pick_matches(_metadata, &cfs_rq, entities,
						   ARRAY_SIZE(entities), sim, oracle);
				ASSERT_NE(NULL, sim);
				EXPECT_FALSE(sim->expellee);
			}
		}
	}
}

TEST(pick_large_pruning_adversarial_matches_oracle)
{
	struct test_cfs_rq cfs_rq;
	struct test_entity entities[96];
	int order[96];
	struct test_entity *sim;
	struct test_entity *oracle;
	int i;

	init_cfs_rq(&cfs_rq, 1000, 35, 11);
	for (i = 0; i < ARRAY_SIZE(entities); i++) {
		u64 vruntime;
		u64 deadline;
		bool expellee;

		if (i < 40) {
			vruntime = 1000 + (i % 3);
			deadline = 1000 + i;
			expellee = true;
		} else if (i == 40) {
			vruntime = 1003;
			deadline = 2000;
			expellee = false;
		} else if (i == 41) {
			vruntime = 1009;
			deadline = 2001;
			expellee = false;
		} else {
			vruntime = 1010 + (i % 11);
			deadline = 3000 + i * 3;
			expellee = (i & 1) != 0;
		}

		init_entity(&entities[i], i, vruntime, deadline, expellee);
		order[i] = i;
	}

	for (i = 0; i < ARRAY_SIZE(order) / 2; i++) {
		int tmp = order[i];

		order[i] = order[ARRAY_SIZE(order) - 1 - i];
		order[ARRAY_SIZE(order) - 1 - i] = tmp;
	}

	build_tree(&cfs_rq, entities, ARRAY_SIZE(entities), order);
	sim = sim_pick_eevdf(&cfs_rq);
	oracle = oracle_pick(&cfs_rq, entities, ARRAY_SIZE(entities));
	expect_pick_matches(_metadata, &cfs_rq, entities, ARRAY_SIZE(entities),
			   sim, oracle);
	ASSERT_NE(NULL, sim);
	EXPECT_FALSE(sim->expellee);
	EXPECT_EQ(40, sim->id);
}

static void init_large_unique_nonexpellee_case(struct test_entity *entities,
					       int *order,
					       int nr_entities,
					       int unique_idx,
					       bool eligible)
{
	int i;

	for (i = 0; i < nr_entities; i++) {
		init_entity(&entities[i], i, 1000 + (i % 13), 5000 + i * 7, true);
		order[i] = i;
	}

	entities[unique_idx].expellee = false;
	entities[unique_idx].vruntime = eligible ? 1001 : 1010;
	entities[unique_idx].min_vruntime = entities[unique_idx].vruntime;
}

TEST(pick_large_unique_nonexpellee_positions)
{
	static const int sizes[] = { 129, 257, 511 };
	int size_idx;

	for (size_idx = 0; size_idx < ARRAY_SIZE(sizes); size_idx++) {
		int nr_entities = sizes[size_idx];
		int positions[] = { 0, nr_entities / 7, nr_entities / 3,
				   nr_entities / 2, nr_entities - 2, nr_entities - 1 };
		int pos_idx;

		for (pos_idx = 0; pos_idx < ARRAY_SIZE(positions); pos_idx++) {
			struct test_cfs_rq cfs_rq;
			struct test_entity entities[MAX_ENTITIES];
			int order[MAX_ENTITIES];
			struct test_entity *sim;
			struct test_entity *oracle;
			int unique_idx = positions[pos_idx];

			init_cfs_rq(&cfs_rq, 1000, 35, 11);
			init_large_unique_nonexpellee_case(entities, order, nr_entities,
							 unique_idx, false);
			build_tree(&cfs_rq, entities, nr_entities, order);
			sim = sim_pick_eevdf(&cfs_rq);
			oracle = oracle_pick(&cfs_rq, entities, nr_entities);
			expect_pick_matches(_metadata, &cfs_rq, entities, nr_entities,
					   sim, oracle);
			ASSERT_NE(NULL, sim);
			EXPECT_EQ(unique_idx, sim->id);
		}
	}
}

TEST(pick_large_unique_eligible_nonexpellee_positions)
{
	static const int sizes[] = { 129, 257, 511 };
	int size_idx;

	for (size_idx = 0; size_idx < ARRAY_SIZE(sizes); size_idx++) {
		int nr_entities = sizes[size_idx];
		int positions[] = { 0, nr_entities / 7, nr_entities / 3,
				   nr_entities / 2, nr_entities - 2, nr_entities - 1 };
		int pos_idx;

		for (pos_idx = 0; pos_idx < ARRAY_SIZE(positions); pos_idx++) {
			struct test_cfs_rq cfs_rq;
			struct test_entity entities[MAX_ENTITIES];
			int order[MAX_ENTITIES];
			struct test_entity *sim;
			struct test_entity *oracle;
			int unique_idx = positions[pos_idx];

			init_cfs_rq(&cfs_rq, 1000, 35, 11);
			init_large_unique_nonexpellee_case(entities, order, nr_entities,
							 unique_idx, true);
			build_tree(&cfs_rq, entities, nr_entities, order);
			sim = sim_pick_eevdf(&cfs_rq);
			oracle = oracle_pick(&cfs_rq, entities, nr_entities);
			expect_pick_matches(_metadata, &cfs_rq, entities, nr_entities,
					   sim, oracle);
			ASSERT_NE(NULL, sim);
			EXPECT_EQ(unique_idx, sim->id);
			EXPECT_TRUE(vruntime_eligible_pick_avg(&cfs_rq, cfs_rq.avg_vruntime,
						      cfs_rq.avg_load,
						      sim->vruntime));
		}
	}
}

static void make_skewed_order(int *order, int nr_entities)
{
	int left = 0;
	int right = nr_entities - 1;
	int out = 0;

	while (left <= right) {
		order[out++] = left++;
		if (left <= right)
			order[out++] = right--;
	}
}

TEST(pick_layered_adversarial_clusters_match_oracle)
{
	struct test_cfs_rq cfs_rq;
	struct test_entity entities[192];
	int order[192];
	struct test_entity *sim;
	struct test_entity *oracle;
	int i;

	init_cfs_rq(&cfs_rq, 2000, 44, 13);
	for (i = 0; i < ARRAY_SIZE(entities); i++) {
		u64 vruntime;
		u64 deadline;
		bool expellee;

		if (i < 48) {
			vruntime = 2000 + (i % 2);
			deadline = 1000 + i;
			expellee = true;
		} else if (i < 96) {
			vruntime = 2003 + (i % 3);
			deadline = 2000 + i;
			expellee = true;
		} else if (i == 96) {
			vruntime = 2002;
			deadline = 5000;
			expellee = false;
		} else if (i == 97 || i == 98) {
			vruntime = 2005;
			deadline = 5001;
			expellee = false;
		} else if (i < 144) {
			vruntime = 2006 + (i % 5);
			deadline = 6000 + i;
			expellee = (i % 3) != 0;
		} else {
			vruntime = 2012 + (i % 7);
			deadline = 7000 + i * 2;
			expellee = (i & 1) != 0;
		}

		init_entity(&entities[i], i, vruntime, deadline, expellee);
		order[i] = i;
	}

	make_skewed_order(order, ARRAY_SIZE(order));
	build_tree(&cfs_rq, entities, ARRAY_SIZE(entities), order);
	sim = sim_pick_eevdf(&cfs_rq);
	oracle = oracle_pick(&cfs_rq, entities, ARRAY_SIZE(entities));
	expect_pick_matches(_metadata, &cfs_rq, entities, ARRAY_SIZE(entities),
			   sim, oracle);
	ASSERT_NE(NULL, sim);
	EXPECT_FALSE(sim->expellee);
	EXPECT_EQ(96, sim->id);
}

static uint32_t rng_next(uint32_t *state)
{
	*state = (*state * 1103515245u) + 12345u;
	return *state;
}

static void shuffle_order(uint32_t *seed, int *order, int nr_entities)
{
	int i;

	for (i = nr_entities - 1; i > 0; i--) {
		int j = rng_next(seed) % (i + 1);
		int tmp = order[i];

		order[i] = order[j];
		order[j] = tmp;
	}
}

static void run_random_case_with_size_and_order(struct __test_metadata *_metadata,
						uint32_t *seed, int iter,
						bool tie_heavy,
						bool allow_identical_keys,
						int nr_entities,
						bool skewed_order)
{
	struct test_cfs_rq cfs_rq;
	struct test_entity entities[MAX_ENTITIES];
	int order[MAX_ENTITIES];
	int i;
	int non_expellees = 0;
	struct test_entity *sim;
	struct test_entity *oracle;

	init_cfs_rq(&cfs_rq, 100,
		    tie_heavy ? (rng_next(seed) % 21) : (10 + (rng_next(seed) % 60)),
		    10 + (rng_next(seed) % 4));
	for (i = 0; i < nr_entities; i++) {
		u64 vruntime;
		u64 deadline;
		bool expellee;

		if (tie_heavy) {
			vruntime = 100 + (rng_next(seed) % 3);
			deadline = 1000 + (rng_next(seed) % 3) * 10;
		} else {
			vruntime = 100 + (rng_next(seed) % 10);
			deadline = 1000 + i * 10 + (rng_next(seed) % 7);
		}
		if (!allow_identical_keys)
			deadline += i * 100;
		expellee = (rng_next(seed) & 3) != 0;

		init_entity(&entities[i], i, vruntime, deadline, expellee);
		order[i] = i;
		if (!expellee)
			non_expellees++;
	}

	if (!non_expellees)
		entities[rng_next(seed) % nr_entities].expellee = false;

	if (skewed_order)
		make_skewed_order(order, nr_entities);
	else
		shuffle_order(seed, order, nr_entities);
	build_tree(&cfs_rq, entities, nr_entities, order);

	sim = sim_pick_eevdf(&cfs_rq);
	oracle = oracle_pick(&cfs_rq, entities, nr_entities);
	if (!picks_are_semantically_equal(&cfs_rq, sim, oracle))
		TH_LOG("random iter=%d seed=0x%x tie_heavy=%d identical=%d skewed=%d",
		       iter, *seed, tie_heavy, allow_identical_keys, skewed_order);
	expect_pick_matches(_metadata, &cfs_rq, entities, nr_entities, sim, oracle);
	ASSERT_NE(NULL, sim);
	EXPECT_FALSE(sim->expellee);
}

static void run_random_case_with_size(struct __test_metadata *_metadata,
				      uint32_t *seed, int iter,
				      bool tie_heavy,
				      bool allow_identical_keys,
				      int nr_entities)
{
	run_random_case_with_size_and_order(_metadata, seed, iter, tie_heavy,
					 allow_identical_keys, nr_entities, false);
}

static void run_random_case(struct __test_metadata *_metadata, uint32_t *seed,
			    int iter, bool tie_heavy, bool allow_identical_keys)
{
	run_random_case_with_size(_metadata, seed, iter, tie_heavy,
				allow_identical_keys,
				4 + (rng_next(seed) % 5));
}

TEST(pick_randomized_matches_oracle)
{
	static const uint32_t initial_seeds[RANDOM_SEEDS] = {
		0x12345678,
		0xdeadbeef,
		0x31415926,
		0x0badc0de,
	};
	int seed_idx;

	for (seed_idx = 0; seed_idx < ARRAY_SIZE(initial_seeds); seed_idx++) {
		uint32_t seed = initial_seeds[seed_idx];
		int iter;

		for (iter = 0; iter < RANDOM_CASES; iter++)
			run_random_case(_metadata, &seed, iter, false, false);
	}
}

TEST(pick_randomized_tie_heavy_matches_oracle)
{
	static const uint32_t initial_seeds[RANDOM_SEEDS] = {
		0x89abcdef,
		0x13579bdf,
		0x2468ace0,
		0xc001d00d,
	};
	int seed_idx;

	for (seed_idx = 0; seed_idx < ARRAY_SIZE(initial_seeds); seed_idx++) {
		uint32_t seed = initial_seeds[seed_idx];
		int iter;

		for (iter = 0; iter < RANDOM_CASES; iter++)
			run_random_case(_metadata, &seed, iter, true, false);
	}
}

TEST(pick_randomized_identical_keys_match_oracle_semantics)
{
	static const uint32_t initial_seeds[RANDOM_SEEDS] = {
		0xa5a5a5a5,
		0x5a5a5a5a,
		0x11223344,
		0x55667788,
	};
	int seed_idx;

	for (seed_idx = 0; seed_idx < ARRAY_SIZE(initial_seeds); seed_idx++) {
		uint32_t seed = initial_seeds[seed_idx];
		int iter;

		for (iter = 0; iter < RANDOM_CASES; iter++)
			run_random_case(_metadata, &seed, iter, true, true);
	}
}

TEST(pick_large_randomized_matches_oracle)
{
	static const uint32_t initial_seeds[RANDOM_SEEDS] = {
		0x10293847,
		0x89abcdef,
		0x55aa55aa,
		0xf0e1d2c3,
	};
	int seed_idx;

	for (seed_idx = 0; seed_idx < ARRAY_SIZE(initial_seeds); seed_idx++) {
		uint32_t seed = initial_seeds[seed_idx];
		int iter;

		for (iter = 0; iter < LARGE_RANDOM_CASES; iter++) {
			int nr_entities = 32 + (rng_next(&seed) % 97);

			run_random_case_with_size(_metadata, &seed, iter, false, false,
					      nr_entities);
		}
	}
}

TEST(pick_large_randomized_identical_keys_match_oracle_semantics)
{
	static const uint32_t initial_seeds[RANDOM_SEEDS] = {
		0xcafef00d,
		0x1ee7c0de,
		0x42424242,
		0xabcdef01,
	};
	int seed_idx;

	for (seed_idx = 0; seed_idx < ARRAY_SIZE(initial_seeds); seed_idx++) {
		uint32_t seed = initial_seeds[seed_idx];
		int iter;

		for (iter = 0; iter < LARGE_RANDOM_CASES; iter++) {
			int nr_entities = 32 + (rng_next(&seed) % 97);

			run_random_case_with_size(_metadata, &seed, iter, true, true,
					      nr_entities);
		}
	}
}

TEST(pick_huge_randomized_matches_oracle)
{
	static const uint32_t initial_seeds[RANDOM_SEEDS] = {
		0x77aa33cc,
		0x1234abcd,
		0x87654321,
		0x0f1e2d3c,
	};
	int seed_idx;

	for (seed_idx = 0; seed_idx < ARRAY_SIZE(initial_seeds); seed_idx++) {
		uint32_t seed = initial_seeds[seed_idx];
		int iter;

		for (iter = 0; iter < LARGE_RANDOM_CASES; iter++) {
			int nr_entities = 256 + (rng_next(&seed) % 257);

			run_random_case_with_size(_metadata, &seed, iter, false, false,
					      nr_entities);
		}
	}
}

TEST(pick_huge_randomized_identical_keys_match_oracle_semantics)
{
	static const uint32_t initial_seeds[RANDOM_SEEDS] = {
		0x3141dead,
		0x2718beef,
		0x1618cafe,
		0x9999aaaa,
	};
	int seed_idx;

	for (seed_idx = 0; seed_idx < ARRAY_SIZE(initial_seeds); seed_idx++) {
		uint32_t seed = initial_seeds[seed_idx];
		int iter;

		for (iter = 0; iter < LARGE_RANDOM_CASES; iter++) {
			int nr_entities = 256 + (rng_next(&seed) % 257);

			run_random_case_with_size(_metadata, &seed, iter, true, true,
					      nr_entities);
		}
	}
}

TEST(pick_huge_skewed_insertion_matches_oracle)
{
	static const int sizes[] = { 257, 511 };
	int size_idx;

	for (size_idx = 0; size_idx < ARRAY_SIZE(sizes); size_idx++) {
		struct test_cfs_rq cfs_rq;
		struct test_entity entities[MAX_ENTITIES];
		int order[MAX_ENTITIES];
		struct test_entity *sim;
		struct test_entity *oracle;
		int nr_entities = sizes[size_idx];
		int i;

		init_cfs_rq(&cfs_rq, 1000, 37, 11);
		for (i = 0; i < nr_entities; i++) {
			u64 vruntime;
			u64 deadline = 10000 + i * 5;
			bool expellee;

			if (i == nr_entities - 3) {
				vruntime = 1003;
				expellee = false;
			} else if (i == nr_entities - 2) {
				vruntime = 1008;
				expellee = false;
			} else {
				vruntime = 1000 + (i % 4);
				expellee = i != nr_entities / 2;
			}

			init_entity(&entities[i], i, vruntime, deadline, expellee);
		}

		make_skewed_order(order, nr_entities);
		build_tree(&cfs_rq, entities, nr_entities, order);
		sim = sim_pick_eevdf(&cfs_rq);
		oracle = oracle_pick(&cfs_rq, entities, nr_entities);
		expect_pick_matches(_metadata, &cfs_rq, entities, nr_entities,
				   sim, oracle);
		ASSERT_NE(NULL, sim);
		EXPECT_FALSE(sim->expellee);
	}
}

TEST(pick_huge_randomized_soak_matches_oracle)
{
	static const uint32_t initial_seeds[RANDOM_SEEDS] = {
		0x6b8b4567,
		0x327b23c6,
		0x643c9869,
		0x66334873,
	};
	int seed_idx;

	for (seed_idx = 0; seed_idx < ARRAY_SIZE(initial_seeds); seed_idx++) {
		uint32_t seed = initial_seeds[seed_idx];
		int iter;

		for (iter = 0; iter < SOAK_CASES; iter++) {
			int nr_entities = 256 + (rng_next(&seed) % 257);

			run_random_case_with_size_and_order(_metadata, &seed, iter,
						     false, false,
						     nr_entities, false);
		}
	}
}

TEST(pick_huge_randomized_identical_key_soak_matches_oracle)
{
	static const uint32_t initial_seeds[RANDOM_SEEDS] = {
		0x74b0dc51,
		0x19495cff,
		0x2ae8944a,
		0x625558ec,
	};
	int seed_idx;

	for (seed_idx = 0; seed_idx < ARRAY_SIZE(initial_seeds); seed_idx++) {
		uint32_t seed = initial_seeds[seed_idx];
		int iter;

		for (iter = 0; iter < SOAK_CASES; iter++) {
			int nr_entities = 256 + (rng_next(&seed) % 257);

			run_random_case_with_size_and_order(_metadata, &seed, iter,
						     true, true,
						     nr_entities, false);
		}
	}
}

TEST(pick_huge_skewed_insertion_soak_matches_oracle)
{
	static const uint32_t initial_seeds[RANDOM_SEEDS] = {
		0x238e1f29,
		0x46e87ccd,
		0x3d1b58ba,
		0x507ed7ab,
	};
	int seed_idx;

	for (seed_idx = 0; seed_idx < ARRAY_SIZE(initial_seeds); seed_idx++) {
		uint32_t seed = initial_seeds[seed_idx];
		int iter;

		for (iter = 0; iter < SOAK_CASES; iter++) {
			int nr_entities = 256 + (rng_next(&seed) % 257);

			run_random_case_with_size_and_order(_metadata, &seed, iter,
						     false, false,
						     nr_entities, true);
		}
	}
}

TEST(pick_huge_extra_soak_matches_oracle)
{
	static const uint32_t initial_seeds[RANDOM_SEEDS] = {
		0x13572468,
		0x24681357,
		0xabcdef98,
		0x89fedcba,
	};
	int seed_idx;

	for (seed_idx = 0; seed_idx < ARRAY_SIZE(initial_seeds); seed_idx++) {
		uint32_t seed = initial_seeds[seed_idx];
		int iter;

		for (iter = 0; iter < EXTRA_SOAK_CASES; iter++) {
			int nr_entities = 256 + (rng_next(&seed) % 257);
			bool tie_heavy = !!(rng_next(&seed) & 1);
			bool identical = !!(rng_next(&seed) & 1);
			bool skewed = !!(rng_next(&seed) & 1);

			run_random_case_with_size_and_order(_metadata, &seed, iter,
						     tie_heavy, identical,
						     nr_entities, skewed);
		}
	}
}

TEST(pick_huge_extra_skewed_identical_soak_matches_oracle)
{
	static const uint32_t initial_seeds[RANDOM_SEEDS] = {
		0x55cc33aa,
		0xaa33cc55,
		0xdead1234,
		0xbeef5678,
	};
	int seed_idx;

	for (seed_idx = 0; seed_idx < ARRAY_SIZE(initial_seeds); seed_idx++) {
		uint32_t seed = initial_seeds[seed_idx];
		int iter;

		for (iter = 0; iter < EXTRA_SOAK_CASES; iter++) {
			int nr_entities = 256 + (rng_next(&seed) % 257);

			run_random_case_with_size_and_order(_metadata, &seed, iter,
						     true, true,
						     nr_entities, true);
		}
	}
}

TEST_HARNESS_MAIN
