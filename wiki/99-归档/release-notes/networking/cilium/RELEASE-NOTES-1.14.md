---
title: cilium v1.14 Release Notes
description: cilium v1.14 Release Notes — Kubernetes 生产运维知识库
summary: cilium v1.14 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
- prometheus
- envoy
- cilium
- docker
- job
- ingress
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- cilium v1.14 Release Notes 是什么
- 如何 cilium v1.14 Release Notes
trigger_keywords:
- cilium
- v1.14
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Cilium|cilium]] v1.14 Release Notes

Source: [v1.14.19](https://github.com/cilium/cilium/releases/tag/v1.14.19)

Summary of Changes
------------------

**Major Changes:**
* Add feature tracking in Cilium agent as [[Prometheus|prometheus]] metrics (Backport PR cilium/cilium#36519, Upstream PR cilium/cilium#35852, @aanm)
* Add feature tracking in Cilium Operator as prometheus metrics (Backport PR cilium/cilium#36519, Upstream PR cilium/cilium#36077, @aanm)

**Minor Changes:**
* [[Envoy|envoy]]: Use yaml format for bootstrap config (Backport PR cilium/cilium#36876, Upstream PR cilium/cilium#36820, @sayboras)
* Reject CNP/CCNP with CIDR rules where CIDRGroupRef is used in combination with ExceptCIDRs (cilium/cilium#36559, @pippolo84)

**Bugfixes:**
* envoy: Configure internal address config based on IP family (Backport PR cilium/cilium#36876, Upstream PR cilium/cilium#36733, @sayboras)
* metrics/features: remove reporting metrics' defaults by default (Backport PR cilium/cilium#36519, Upstream PR cilium/cilium#36298, @aanm)
* ui: drop CORS headers from api response (Backport PR cilium/cilium#36870, Upstream PR cilium/cilium#35762, @geakstr)

**CI Changes:**
* [v1.14] .github: Remove CI Fuzz workflow (cilium/cilium#36643, @joestringer)
* [v1.14] gha: use /test to trigger tests in stable branches (cilium/cilium#36675, @giorio94)
* [v1.14] Unblock verifier test LVH image updates (cilium/cilium#36688, @tklauser)
* ci: fix job names for various ci workflows (Backport PR cilium/cilium#36519, Upstream PR cilium/cilium#36397, @marseel)
* Extend the check-ipsec-leak bpftrace script to capture additional details of leaked packets (Backport PR cilium/cilium#36870, Upstream PR cilium/cilium#33398, @giorio94)
* gha: bump ubuntu version in conformance-externalworkloads (Backport PR cilium/cilium#36984, Upstream PR cilium/cilium#36859, @giorio94)
* gha: correctly downgrade to patch release in ipsec workflows (Backport PR cilium/cilium#36984, Upstream PR cilium/cilium#36858, @giorio94)
* gha: merge artifacts in net-perf-gke workflow (Backport PR cilium/cilium#36519, Upstream PR cilium/cilium#36236, @giorio94)
* gha: Use ubuntu-24.04 for integration-test (Backport PR cilium/cilium#36661, Upstream PR cilium/cilium#36628, @sayboras)
* Use Clang from cilium-builder image to build BPF code in CI (Backport PR cilium/cilium#36870, Upstream PR cilium/cilium#31754, @gentoo-root)

**Misc Changes:**
* .github/workflows: always install cilium-cli (Backport PR cilium/cilium#36519, Upstream PR cilium/cilium#36234, @aanm)
* .github/workflows: do not fail ginkgo if unable to fetch features (Backport PR cilium/cilium#36519, Upstream PR cilium/cilium#36461, @aanm)
* .github: fix conformance-k8s NP test (Backport PR cilium/cilium#36519, Upstream PR cilium/cilium#36355, @aanm)
* [v1.14] Use bash syntax to consume env variable (cilium/cilium#36633, @ferozsalam)
* Add more features tracking in Cilium agent as prometheus metrics (Backport PR cilium/cilium#36519, Upstream PR cilium/cilium#36078, @aanm)
* Add policy-related features tracking in Cilium agent as prometheus metrics (Backport PR cilium/cilium#36519, Upstream PR cilium/cilium#36203, @aanm)
* build: Remove debug leftover from Makefile (Backport PR cilium/cilium#36984, Upstream PR cilium/cilium#36917, @gentoo-root)
* chore(deps): update all github action dependencies (v1.14) (cilium/cilium#36909, @cilium-renovate[bot])
* chore(deps): update all-dependencies (v1.14) (cilium/cilium#36904, @cilium-renovate[bot])
* chore(deps): update dependency cilium/cilium-cli to v0.16.23 (v1.14) (cilium/cilium#36896, @cilium-renovate[bot])
* chore(deps): update dependency cilium/hubble to v1.16.5 (v1.14) (cilium/cilium#36840, @cilium-renovate[bot])
* chore(deps): update docker.io/library/golang:1.22.10 docker digest to 1a6e657 (v1.14) (cilium/cilium#36907, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.30.9-1734560096-c1e57e20d9a5f4e462163e5354f787bfa0d2b50f (v1.14) (cilium/cilium#36708, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.14) (patch) (cilium/cilium#36908, @cilium-renovate[bot])
* docs: Clarify the behavior of CiliumNetworkPolicies toCIDRSet (Backport PR cilium/cilium#36639, Upstream PR cilium/cilium#36549, @verysonglaa)
* Fix `make -C Documentation update-cmdref` when make uses `--jobserver-style=fifo`. (Backport PR cilium/cilium#36870, Upstream PR cilium/cilium#36788, @gentoo-root)
* fix(deps): update module golang.org/x/net to v0.33.0 [security] (v1.14) (cilium/cilium#36713, @cilium-renovate[bot])
* ingress, gateway-api: Convert test fixtures to file based (Backport PR cilium/cilium#36784, Upstream PR cilium/cilium#36732, @sayboras)
* metrics/features: enable ClusterMesh (Backport PR cilium/cilium#36519, Upstream PR cilium/cilium#36402, @aanm)
* metrics/features: refactor metric names (Backport PR cilium/cilium#36519, Upstream PR cilium/cilium#36209, @aanm)
* Remove reference to DNS polling (Backport PR cilium/cilium#36870, Upstream PR cilium/cilium#36679, @JacobHenner)

**Other Changes:**
* install: Update image digests for v1.14.18 (cilium/cilium#36654, @cilium-release-bot[bot])
* Speed up datapath compilation by up to 50% (cilium/cilium#36670, @ti-mo)


## Docker Manifests

### cilium

`docker.io/cilium/cilium:v1.14.19@sha256:dfee0589d6dbb64fccde38588e5ee963a8578cfa029539cbabae4e15589b9c3b`
`quay.io/cilium/cilium:v1.14.19@sha256:dfee0589d6dbb64fccde38588e5ee963a8578cfa029539cbabae4e15589b9c3b`

### clustermesh-apiserver

`docker.io/cilium/clustermesh-apiserver:v1.14.19@sha256:fecccb6f1c8b27637ea950bf7ce40bd6bb597f0cd35f9f9322049d5a3f29578b`
`quay.io/cilium/clustermesh-apiserver:v1.14.19@sha256:fecccb6f1c8b27637ea950bf7ce40bd6bb597f0cd35f9f9322049d5a3f29578b`

### docker-plugin

`docker.io/cilium/docker-plugin:v1.14.19@sha256:ab5500874aade9f8d295d2d55576929f0bd0dfb206ed1d498ecf4cc99d4f2ede`
`quay.io/cilium/docker-plugin:v1.14.19@sha256:ab5500874aade9f8d295d2d55576929f0bd0dfb206ed1d498ecf4cc99d4f2ede`

### hubble-relay

`docker.io/cilium/hubble-relay:v1.14.19@sha256:64599363dc856b93a2f7586dce587a9af0a60b6a4c6fa7b8d89543b354832c0e`
`quay.io/cilium/hubble-relay:v1.14.19@sha256:64599363dc856b93a2f7586dce587a9af0a60b6a4c6fa7b8d89543b354832c0e`

### kvstoremesh

`docker.io/cilium/kvstoremesh:v1.14.19@sha256:815188117840f69a3d1eb1fce7bbac539cc5e0292c1c4b39b89a31c22d601d89`
`quay.io/cilium/kvstoremesh:v1.14.19@sha256:815188117840f69a3d1eb1fce7bbac539cc5e0292c1c4b39b89a31c22d601d89`

### operator-alibabacloud

`docker.io/cilium/operator-alibabacloud:v1.14.19@sha256:98398bbaa93c93d07046cf01037015a7bfc848532c9e0ca9286df9eb7859b49d`
`quay.io/cilium/operator-alibabacloud:v1.14.19@sha256:98398bbaa93c93d07046cf01037015a7bfc848532c9e0ca9286df9eb7859b49d`

### operator-aws

`docker.io/cilium/operator-aws:v1.14.19@sha256:a3914c09f74e822086fc861d5d287ad07e10ce31d7c41cd0e12556e5ac61c74b`
`quay.io/cilium/operator-aws:v1.14.19@sha256:a3914c09f74e822086fc861d5d287ad07e10ce31d7c41cd0e12556e5ac61c74b`

### operator-azure

`docker.io/cilium/operator-azure:v1.14.19@sha256:c46d2b59c318430be2dc19ec2ad9724414915b3e46124356bbcaa38c95401701`
`quay.io/cilium/operator-azure:v1.14.19@sha256:c46d2b59c318430be2dc19ec2ad9724414915b3e46124356bbcaa38c95401701`

### operator-generic

`docker.io/cilium/operator-generic:v1.14.19@sha256:3201b8a127dc5344f31c89b5c199f15d90eb5a56a997ba933707ba0dbf69322e`
`quay.io/cilium/operator-generic:v1.14.19@sha256:3201b8a127dc5344f31c89b5c199f15d90eb5a56a997ba933707ba0dbf69322e`

### operator

`docker.io/cilium/operator:v1.14.19@sha256:03ff2ea917a6de911acc3c42bc8bc33e7ae251c15b82851c1e8f222eb578fdca`
`quay.io/cilium/operator:v1.14.19@sha256:03ff2ea917a6de911acc3c42bc8bc33e7ae251c15b82851c1e8f222eb578fdca`



<!-- risk-assessed -->
