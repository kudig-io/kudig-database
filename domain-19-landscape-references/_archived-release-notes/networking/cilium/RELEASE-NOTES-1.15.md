---
title: cilium v1.15 Release Notes
description: cilium v1.15 Release Notes — Kubernetes 生产运维知识库
summary: cilium v1.15 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
- envoy
- cilium
- docker
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cilium v1.15 Release Notes 是什么
- 如何 cilium v1.15 Release Notes
trigger_keywords:
- cilium
- v1.15
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
---



# [[Cilium|cilium]] v1.15 Release Notes

Source: [v1.15.19](https://github.com/cilium/cilium/releases/tag/v1.15.19)

Summary of Changes
------------------

**Bugfixes:**
* LBIPAM: Fix deletion of CiliumLoadBalancerIPPool with multiple IP blocks that led to an operator crash (Backport PR cilium/cilium#40092, Upstream PR cilium/cilium#40013, @pippolo84)

**Misc Changes:**
* .github/workflows: remove reviewers if ciliumbot approved PR (Backport PR cilium/cilium#40092, Upstream PR cilium/cilium#39989, @aanm)
* auto-approve: add repository as part command (Backport PR cilium/cilium#40092, Upstream PR cilium/cilium#40050, @aanm)
* auto-approve: add repository as part command (Backport PR cilium/cilium#40329, Upstream PR cilium/cilium#40089, @aanm)
* chore(deps): update all-dependencies (v1.15) (cilium/cilium#40371, @cilium-renovate[bot])
* chore(deps): update dependency cilium/cilium-cli to v0.18.5 (v1.15) (cilium/cilium#40328, @cilium-renovate[bot])
* chore(deps): update go (v1.15) (cilium/cilium#40426, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-[[Envoy|envoy]] docker tag to v1.33.3-1752058676-6ef6da8f651016be6a86e85775aa2a7b9081c138 (v1.15) (cilium/cilium#40439, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.33.4-1752151664-7c2edb0b44cf95f326d628b837fcdd845102ba68 (v1.15) (cilium/cilium#40467, @cilium-renovate[bot])
* contrib/git: add merge drivers to automate post-merge commands (Backport PR cilium/cilium#40293, Upstream PR cilium/cilium#40189, @aanm)
* disk-cleanup: parallelize cleanup process to speed up step (Backport PR cilium/cilium#40092, Upstream PR cilium/cilium#40054, @aanm)
* docs/ipsec: Fix incorrect statement on hostns encryption (Backport PR cilium/cilium#40172, Upstream PR cilium/cilium#40133, @pchaigno)
* operator/secretsync: silence reconciliation logs (Backport PR cilium/cilium#40223, Upstream PR cilium/cilium#40217, @tklauser)
* proxy: Use upstream envoy control plane API (Backport PR cilium/cilium#40244, Upstream PR cilium/cilium#39672, @sayboras)

**Other Changes:**
* [v1.15] deps: Update cilium-envoy image to 1.33.x (cilium/cilium#40164, @sayboras)
* install: Update image digests for v1.15.18 (cilium/cilium#40116, @cilium-release-bot[bot])
* v1.15: docs: Document encapsulation options (cilium/cilium#40473, @pchaigno)


## Docker Manifests

### cilium

`quay.io/cilium/cilium:v1.15.19@sha256:c50d1580194320508dd24d6544a77039fba2ce85458887698486a34769598539`

### clustermesh-apiserver

`quay.io/cilium/clustermesh-apiserver:v1.15.19@sha256:b990ef67a6707fcf94b9c1bb52b289efb0b64f57dd0fd384302c1c4aff6e50fe`

### docker-plugin

`quay.io/cilium/docker-plugin:v1.15.19@sha256:3ce9dc0970848257b42d5b6c4a7f4ef0690fcf2eee07db03a262a785ff8f5037`

### hubble-relay

`quay.io/cilium/hubble-relay:v1.15.19@sha256:8962877952181743c1d44a723d73de4eb2fd36761025b8e6fa250f10e2653fdf`

### operator-alibabacloud

`quay.io/cilium/operator-alibabacloud:v1.15.19@sha256:cee59e05650b99214f94649148e1a0bd1fc442cc59eff5336757eb884b3000a9`

### operator-aws

`quay.io/cilium/operator-aws:v1.15.19@sha256:c76174d31ea267c91838e9cd89b2cd9cb95de77c099e2050ed60896f027aabb0`

### operator-azure

`quay.io/cilium/operator-azure:v1.15.19@sha256:0c21df53904b27a95549871ac147b87fddb4a185252ebb108f1e196417cd7c3f`

### operator-generic

`quay.io/cilium/operator-generic:v1.15.19@sha256:391c192af8f11a5733bcdc88cc09e2f3768d2c3aa663bf136208e72e07fa80bc`

### operator

`quay.io/cilium/operator:v1.15.19@sha256:9d50dfb799d00e682fc693044d1e21d3d9dfa39e9e6c245247e3b8e3b9ac9438`

