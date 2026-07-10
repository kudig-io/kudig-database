.. SPDX-License-Identifier: GPL-2.0

========================
EEVDF pick path in this kernel
========================

1. Purpose and scope
====================

This document explains the fair-class EEVDF pick path used in this kernel,
with emphasis on the local identity/expel changes around ``pick_eevdf()``.

The current code in ``kernel/sched/fair.c`` splits the picker into two modes:

1. ``__pick_eevdf()``: the normal non-expel path, kept close to the baseline
   EEVDF algorithm, and
2. ``id_pick_eevdf()``: the expel-aware path selected only when
   ``rq_on_expel(rq_of(cfs_rq))`` is true.

The important reading rule is therefore: not every statement about the
expel-aware picker applies to all calls to ``pick_eevdf()``. The fixed
pick-side snapshot, strict winner, and fallback winner logic belong only to the
expel path.

The primary implementation lives in ``kernel/sched/fair.c``.


2. EEVDF background used by the code
====================================

EEVDF stands for Earliest Eligible Virtual Deadline First. The baseline pick
rule is:

1. a runnable entity must first be eligible, meaning it is still owed service,
2. among eligible entities, choose the one with the earliest virtual deadline.

The core lag relation used by the scheduler code is::

    lag_i = S - s_i = w_i * (V - v_i)

where:

- ``w_i`` is the entity weight,
- ``v_i`` is the entity virtual runtime,
- ``V`` is the runqueue virtual time,
- ``S`` and ``s_i`` are the ideal and actual service terms.

Eligibility is therefore::

    eligible(i) <=> lag_i >= 0 <=> V >= v_i

In the kernel, ``V`` is represented through a weighted-average form instead of
computing the ideal virtual time directly. The implementation comment above
``avg_vruntime()`` derives the relation from lag conservation::

    sum(lag_i) = 0
    sum(w_i * (V - v_i)) = 0
    V = sum(v_i * w_i) / sum(w_i)

To keep the arithmetic bounded, the scheduler tracks the relative form::

    v0   := cfs_rq->min_vruntime
    avg  := cfs_rq->avg_vruntime = sum((v_i - v0) * w_i)
    load := cfs_rq->avg_load     = sum(w_i)

This gives the pick-side eligibility comparison used in ``fair.c``::

    avg >= (v_i - min_vruntime) * load

which is implemented by ``vruntime_eligible()`` and, for the expel-aware path,
by ``vruntime_eligible_pick_avg()``.

Equivalently, if ``avg`` and ``load`` are treated as fixed during one pick,
then eligibility becomes a fixed threshold over ``vruntime``::

    eligible(se) <=> se->vruntime <= min_vruntime + avg / load

That fixed-threshold view is the key to understanding the expel-aware path.


3. Baseline kernel EEVDF pick algorithm
=======================================

The fair scheduler keeps runnable entities in an rb-tree ordered by virtual
deadline. The same tree is also augmented with subtree ``min_vruntime`` so it
can be searched like a heap for eligibility pruning.

The baseline EEVDF shape is:

- the task must be eligible,
- among eligible tasks, select the earliest virtual deadline,
- do so in ``O(log n)`` using an augmented rb-tree.

The augmentation is::

    se->min_vruntime = min(se->vruntime,
                           se->{left,right}->min_vruntime)

This means tree descent can safely prune on eligibility, because if a subtree's
``min_vruntime`` is already ineligible then no entity below it can be the best
eligible winner.

The current entity (``cfs_rq->curr``) is a special case. It is logically still
part of the runnable competition when ``curr->on_rq`` is true, but it is not in
the rb-tree. The generic eligibility helpers therefore add ``curr`` into the
comparison view when evaluating the normal path.


4. Dispatcher shape in the current code
=======================================

The current top-level ``pick_eevdf()`` is only a dispatcher::

    if (rq_on_expel(rq_of(cfs_rq)))
        return id_pick_eevdf(cfs_rq);

    return __pick_eevdf(cfs_rq);

So there are two distinct picker behaviors in the code base.

``__pick_eevdf()``
------------------

``__pick_eevdf()`` is the normal path. It stays close to the ordinary kernel
EEVDF walk:

- drop ``curr`` if it is off-rq or ineligible,
- honor ``RUN_TO_PARITY`` for an eligible protected current entity,
- test the leftmost entity first,
- descend left only when subtree ``min_vruntime`` proves there can still be an
  eligible entity there,
- choose the earliest-deadline eligible entity,
- finally arbitrate between ``curr`` and the queued winner with
  ``entity_before()``.

This path does **not** maintain an expel-aware fallback winner.

``id_pick_eevdf()``
-------------------

``id_pick_eevdf()`` is the expel-aware path. It is the only place that owns:

- the expel-aware pick snapshot via ``id_pick_eligible_data()``,
- strict winner vs fallback winner tracking,
- the project rule that the expel slow path must not return ``NULL`` when a
  selectable non-expellee runnable entity exists.

Any discussion below about fixed ``avg/load``, strict winner, or fallback winner
refers to ``id_pick_eevdf()``, not to all invocations of ``pick_eevdf()``.


5. Why identity/expel complicates picking
=========================================

This tree adds identity/expel policy on top of the baseline EEVDF model.

The important constraint is that expel is policy state, not EEVDF state.
Whether ``should_expel_se()`` returns true is not encoded in the rb-tree
augmentation. Therefore expel cannot be used as a safe pruning dimension
inside the tree walk.

This creates a corner case that does not exist in plain "pick earliest eligible"
logic:

- every eligible entity visible to the EEVDF math can be an expellee,
- every non-expellee entity can still be ineligible,
- the strict set ``eligible && !should_expel_se()`` can therefore be empty.

That corner case matters only to the expel-aware path. The normal path keeps
ordinary EEVDF behavior; the expel path needs an extra completion rule so it can
still choose a runnable non-expellee when policy hides every eligible winner.


6. Visible queue filtering before the expel-aware pick
======================================================

The local expel filtering starts before ``id_pick_eevdf()`` itself.

``skip_expellee_se()`` hides queued expellees from the left edge of the visible
queue. Those hidden entities are removed from the rb-tree with
``__dequeue_entity()``, which also removes their contribution from
``avg_vruntime`` and ``avg_load``. That keeps the visible queue and the
pick-side average accounting aligned for queued entities.

There is one extra wrinkle: ``curr`` is not in the rb-tree. If the generic
eligibility logic added ``curr`` back into ``avg/load`` unconditionally, an
expelled current entity would still skew the pick-side threshold even though the
queued expellees hidden by ``skip_expellee_se()`` had already been filtered out.

The helper ``id_pick_eligible_data()`` fixes that mismatch. It snapshots
``avg_vruntime`` and ``avg_load`` once at the start of the expel-aware pick,
and adds ``curr`` back only when all of the following are true:

- ``curr`` exists,
- ``curr->on_rq`` is true,
- ``should_expel_se(rq_of(cfs_rq), curr)`` is false.

This creates one consistent visibility view for the entire expel-aware pick.


7. Fixed avg/load snapshot in ``id_pick_eevdf()``
=================================================

A tempting but incorrect approach would be to change ``avg/load`` while scanning
past expelled candidates. That would make eligibility depend on how far the
search has progressed, which is unfair to earlier entities and breaks the notion
that EEVDF eligibility is defined against one runqueue virtual time.

The expel-aware path instead freezes ``avg`` and ``load`` once per call to
``id_pick_eevdf()``.

With that snapshot fixed, eligibility remains a stable predicate for the whole
search::

    eligible(se) <=> avg >= (se->vruntime - min_vruntime) * load

Nothing discovered later in the walk can retroactively change whether an earlier
entity was eligible under that pick's virtual-time view.

This fixed-view rule is what allows the expel-aware picker to search for a
strict winner and a fallback winner in parallel without dynamically reweighting
the runqueue.


8. Strict winner and fallback winner in the expel path
======================================================

``id_pick_eevdf()`` keeps two candidate sets.

The strict winner is the ordinary EEVDF-style winner subject to local expel
policy::

    strict winner = earliest deadline in
                    { se | eligible(se) && !should_expel_se(se) }

If that set is non-empty, the expel-aware pick still behaves like normal EEVDF
among selectable entities.

The new piece is the fallback winner::

    fallback winner = minimum eligibility deficit in
                      { se | !should_expel_se(se) }

where::

    deficit(se) = (se->vruntime - min_vruntime) * load - avg

The interpretation is simple:

- ``deficit <= 0`` means the entity is already eligible,
- a positive deficit measures how far the entity is from eligibility,
- the smallest positive deficit is the non-expellee closest to becoming
  eligible.

The helper ``entity_eligibility_deficit()`` computes that metric, and
``better_fallback_entity()`` compares two fallback candidates. If two
candidates have the same deficit, the tie is broken with ``entity_before()``,
which preserves deadline order.

This gives the expel-aware picker a precise answer to the project-specific
question:

"If all eligible entities are expellees, which non-expellee should run?"

The answer is: the non-expellee closest to becoming eligible under the same
fixed ``avg/load`` view.


9. Why pruning must stay eligibility-only
=========================================

The rb-tree still supports only one safe pruning dimension: eligibility via
subtree ``min_vruntime``.

During the expel-aware slow-path descent, the test
``vruntime_eligible_pick_avg(cfs_rq, avg, load, left->min_vruntime)`` answers
whether the left subtree can contain any eligible entity. If it can, the walk
must go left because anything there beats the current node on deadline order.

Expel state cannot be used the same way. The tree does not carry any augmented
"has non-expellee" summary, so pruning away a subtree because the current node
or one earlier node is expelled would be incorrect. A later descendant or
successor can still be the first strict non-expellee eligible winner.

That is why the code explicitly warns not to mix ``should_expel_se()`` into tree
descent logic.


10. Step-by-step shape of ``id_pick_eevdf()``
=============================================

The expel-aware implementation in ``kernel/sched/fair.c`` follows this order:

1. Snapshot ``avg`` and ``load`` with ``id_pick_eligible_data()``.
2. Examine ``curr``:

   - drop it if it is off-rq or currently expelled,
   - if it is eligible, treat it as the initial strict winner,
   - otherwise treat it as the initial fallback candidate.

3. If there is only one queued entity, return the best available choice among
   ``best``, ``fallback``, and the queued entity.
4. If ``RUN_TO_PARITY`` protects an eligible current winner, return it.
5. Check the leftmost visible queued entity returned by ``__pick_first_entity()``:

   - if it is selectable and eligible, it is immediately the best queued winner,
   - otherwise use it to improve the fallback candidate.

6. Walk the rb-tree:

   - descend left only when the left subtree can contain an eligible entity,
   - update ``fallback`` whenever a better non-expellee candidate is found,
   - when the first eligible node is reached:

     - if it is not expelled, it is the strict winner,
     - if it is expelled, continue scanning in in-order successor order until
       the first non-expellee eligible entity is found, while still updating the
       fallback candidate.

7. Return order is:

   - ``best`` if present,
   - otherwise ``fallback``,
   - otherwise the final ``curr`` value.

The important contract is that the expel-aware slow path keeps a valid
non-expellee fallback ready even when every eligible entity encountered so far
is expelled.


11. Why fallback does not violate EEVDF
=======================================

The fallback path does not redefine eligibility. Instead, it is a local policy
layer that applies only when the strict selectable winner set is empty in the
expel-aware path.

As long as there exists an entity in::

    { se | eligible(se) && !should_expel_se(se) }

that entity wins exactly as expected from EEVDF deadline ordering.

Fallback is consulted only when expel policy removes every eligible winner from
the selectable set. In that case, choosing the minimum-deficit non-expellee is
the closest approximation to the normal EEVDF choice while still respecting the
policy constraint.


12. Vmcore case study: why the expel path needs fallback
========================================================

The motivating crash scenario for this change was a child ``cfs_rq`` whose
strict selectable winner set was empty even though the queue itself was not.

The reconstructed state was:

- ``nr_queued = 2``
- ``curr = NULL``
- ``avg_vruntime = 1021560330``
- ``avg_load = 1759``
- ``min_vruntime = 328604213``

and the two queued entities were:

1. a normal entity

   - ``identity = 0``
   - ``vruntime = 329994091``
   - ``deadline = 331117992``

2. an expellee entity

   - ``identity = -1``
   - ``vruntime = 328604213``
   - ``deadline = 331404213``

Under the fixed eligibility test used by ``id_pick_eevdf()``::

    eligible(se) <=> avg >= (se->vruntime - min_vruntime) * load

we get:

- for the expellee:

  - ``(328604213 - 328604213) * 1759 = 0``
  - therefore it is eligible.

- for the normal entity:

  - ``(329994091 - 328604213) * 1759`` is positive and larger than
    ``avg = 1021560330``
  - therefore it is not eligible.

So the strict set is empty::

    { se | eligible(se) && !should_expel_se(se) } = empty

but the non-expellee set is not empty::

    { se | !should_expel_se(se) } != empty

This is exactly the case the fallback logic is meant to handle. The normal
entity is not yet eligible, but it is still the correct fallback because it is
the non-expellee with minimum eligibility deficit.

Without that fallback, the expel-aware slow path can walk a non-empty queue and
still fail to produce a selectable entity.


13. Comparison with the EEVDF paper and the upstream algorithmic model
======================================================================

The local picker still follows the same algorithmic backbone as EEVDF.

First, the concepts that remain unchanged:

- lag is still the basis of eligibility,
- virtual runtime is still the service coordinate,
- earliest virtual deadline among eligible entities is still the baseline EEVDF
  winner,
- the rb-tree is still ordered by deadline and pruned by eligibility.

In other words, this tree does not replace EEVDF. It adds a policy layer for
the expel-active case where filtering removes every eligible entity from the
selectable set.

It helps to separate the discussion into three layers.

Layer 1: paper-level EEVDF model
--------------------------------

At the paper level, EEVDF says:

- define eligibility from lag,
- define a virtual deadline for each entity,
- choose the earliest virtual deadline among eligible entities.

The paper-level model assumes the picker operates on the runnable set defined by
the scheduling algorithm itself.

Layer 2: upstream kernel implementation model
---------------------------------------------

The kernel turns that model into an efficient implementation by:

- representing runqueue virtual time through ``avg_vruntime`` and ``avg_load``,
- using ``min_vruntime`` as the moving origin,
- testing eligibility with the multiplication form instead of dividing,
- storing runnable entities in an rb-tree ordered by virtual deadline,
- augmenting each subtree with ``min_vruntime`` so the search can prune on
  eligibility.

That gives the usual strict pick rule::

    pick earliest deadline from the eligible set

Layer 3: this tree's local policy extension
-------------------------------------------

This tree introduces a second filter, ``should_expel_se()``, which is not part
of the EEVDF paper model and not part of the rb-tree augmentation.

That creates a gap between:

- entities that are EEVDF-eligible, and
- entities that are policy-selectable.

The expel-aware strict selectable winner therefore becomes::

    earliest deadline in { se | eligible(se) && !should_expel_se(se) }

The paper and the upstream model do not need an extra rule when that set is
empty, because they do not have this local expel filter. This tree does in the
expel-active mode.

That is why ``id_pick_eevdf()`` adds a fallback winner::

    minimum deficit in { se | !should_expel_se(se) }

This preserves as much of the EEVDF ordering as possible while resolving the
policy-only hole.

What is preserved from EEVDF
----------------------------

The fallback does not alter the definition of eligibility itself.

- the strict winner still uses the ordinary EEVDF rule,
- the tree walk is still pruned only by eligibility,
- the expel-aware pick still uses one fixed virtual-time view for the whole
  decision.

Those points are important because they keep the local policy extension from
mutating the EEVDF model in the middle of the search.

What is intentionally added beyond EEVDF
----------------------------------------

Two local rules are intentionally stronger than the bare EEVDF algorithm in the
expel-active path:

1. expelled entities are excluded from the selectable set,
2. the expel-aware slow path must not return ``NULL`` when a selectable
   non-expellee runnable entity exists.

The minimum-deficit fallback is the bridge between those two rules. It is not a
new EEVDF theorem; it is the project-specific completion rule used when local
policy empties the strict winner set.


14. Reading the code and simulator
==================================

The most useful code locations are:

- ``kernel/sched/fair.c``

  - ``avg_vruntime()`` for the weighted virtual-time derivation,
  - ``vruntime_eligible()`` for the base eligibility comparison,
  - ``__pick_eevdf()`` for the normal path,
  - ``id_pick_eligible_data()`` for the expel-aware fixed snapshot,
  - ``entity_eligibility_deficit()`` for the fallback metric,
  - ``better_fallback_entity()`` for fallback ordering,
  - ``id_pick_eevdf()`` for the strict/fallback winner logic,
  - ``pick_eevdf()`` for the top-level dispatcher,
  - ``skip_expellee_se()`` for visible queue filtering.

- ``tools/testing/selftests/sched/pick_eevdf_sim.c``

  - simulator normal mode mirrors ``__pick_eevdf()``,
  - simulator suppression mode mirrors ``id_pick_eevdf()`` selected by
    ``rq_on_expel()`` in the kernel.

- ``Documentation/scheduler/sched-design-CFS.rst``

  - general background on vruntime and rb-tree scheduling.

When debugging this area, keep one distinction in mind: EEVDF eligibility is a
property of the fixed pick-time virtual-time view, while expel is a separate
policy filter layered on top of that view.
