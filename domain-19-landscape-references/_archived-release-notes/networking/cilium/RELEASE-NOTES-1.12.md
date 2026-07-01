---
title: cilium v1.12 Release Notes
description: cilium v1.12 Release Notes — Kubernetes 生产运维知识库
summary: cilium v1.12 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- envoy
- cilium
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cilium v1.12 Release Notes 是什么
- 如何 cilium v1.12 Release Notes
trigger_keywords:
- cilium
- v1.12
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
---



# [[Cilium|cilium]] v1.12 Release Notes

Source: [v1.12.19](https://github.com/cilium/cilium/releases/tag/v1.12.19)

We are pleased to release Cilium v1.12.19. This release contains various bug fixes and CI / usability improvements.

Summary of Changes
------------------

**Minor Changes:**
* api/cli: Encryption status now includes rendering IPsec status in JSON. (Backport PR #30390, Upstream PR #30167, @viktor-kurchenko)

**CI Changes:**
* ci/ipsec: Fix version retrieval for downgrades to closest patch release (Backport PR #30678, Upstream PR #30503, @qmonnet)
* gha: explicilty specify beefier runner type for clustermesh workflows (Backport PR #30390, Upstream PR #30335, @giorio94)
* gha: make runner type for clustermesh workflows configurable (Backport PR #30678, Upstream PR #30496, @giorio94)
* Rework GHA workflows to checkout the untrusted context in a separate directory for increased separation (Backport PR #30390, Upstream PR #30207, @giorio94)

**Misc Changes:**
* bpf: lb: return drop reasons from __lb4_rev_nat() (Backport PR #30511, Upstream PR #30410, @julianwiedmann)
* chore(deps): update docker.io/library/golang docker tag to v1.21.6 (v1.12) (#30243, @renovate[bot])
* chore(deps): update hubble cli to v0.13.0 (v1.12) (minor) (#30276, @renovate[bot])
* doc: Add Azure CNI Powered by cilium as external installer (Backport PR #30390, Upstream PR #28286, @tamilmani1989)
* docs: warn users that IPsec and KPR are mutual exclusive (Backport PR #30511, Upstream PR #30403, @f1ko)

**Other Changes:**
* [v1.12] ci/ipsec: Fix downgrade version for release preparation commits (#30714, @qmonnet)
* [[Envoy|envoy]]: Bump envoy version to v1.26.7 (#30695, @sayboras)
* gke: Bump gke minimum versions (#30676, @sayboras)
* install: Update image digests for v1.12.18 (#30316, @gentoo-root)