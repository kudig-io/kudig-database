---
title: cilium v1.9 Release Notes
description: cilium v1.9 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
- envoy
- cilium
- helm
- docker
- operator
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cilium v1.9 Release Notes 是什么
- 如何 cilium v1.9 Release Notes
trigger_keywords:
- cilium
- v1.9
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- cilium-basics
---

# cilium v1.9 Release Notes

Source: [v1.9.18](https://github.com/cilium/cilium/releases/tag/v1.9.18)

We are pleased to release Cilium v1.9.18. This is the final bugfix release of the v1.9 series. See below for full notes.

Summary of Changes
------------------

**Minor Changes:**
* ui: v0.9.0 images and drop envoy proxy container (Backport PR #20365, Upstream PR #19565, @geakstr)

**Bugfixes:**
* Fix agent panic in some cases when service matcher local redirect policy was deployed prior to the selected service. (Backport PR #20180, Upstream PR #19522, @aditighag)
* Fix memory leak in the DNS cache when a long-lived endpoint makes many unique DNS lookups over time (Backport PR #20180, Upstream PR #19925, @christarazi)
* nodediscovery: ensure we cache the nodeResource correctly to avoid null pointer dereferencing (Backport PR #20365, Upstream PR #20158, @odinuge)

**Misc Changes:**
* [docs] Add training and support information to Getting Help (Backport PR #20365, Upstream PR #20194, @lizrice)
* Add ESP to firewall requirements in documentation for IPSec enabled C… (Backport PR #20365, Upstream PR #20314, @Kikiodazie)
* build(deps): bump helm/kind-action from 1.2.0 to 1.3.0 (#20199, @dependabot[bot])

**Other Changes:**
* install: Update image digests for v1.9.17 (#20221, @joestringer)
* v1.9: update cilium-{runtime,builder} (#20543, @joestringer)


## Docker Manifests

### cilium

`docker.io/cilium/cilium:v1.9.18@sha256:725a6c6e11b5c30daa0731c8846a26a5b331a0e293eb29b45cf1202dcbad7bc2`
`quay.io/cilium/cilium:v1.9.18@sha256:725a6c6e11b5c30daa0731c8846a26a5b331a0e293eb29b45cf1202dcbad7bc2`

### clustermesh-apiserver

`docker.io/cilium/clustermesh-apiserver:v1.9.18@sha256:707b11a188a4bacf3c9cb54da6bb712e6794371d5689dae1cc483a65fb0e07eb`
`quay.io/cilium/clustermesh-apiserver:v1.9.18@sha256:707b11a188a4bacf3c9cb54da6bb712e6794371d5689dae1cc483a65fb0e07eb`

### docker-plugin

`docker.io/cilium/docker-plugin:v1.9.18@sha256:ff1406efdfb2bbfb95524faf5e11241673c643c1e3f939890e528c97b9883242`
`quay.io/cilium/docker-plugin:v1.9.18@sha256:ff1406efdfb2bbfb95524faf5e11241673c643c1e3f939890e528c97b9883242`

### hubble-relay

`docker.io/cilium/hubble-relay:v1.9.18@sha256:2b4d1a7a530de8f48a555b42d3f4d834249d37e668e5ecd2351c5e24f3bb2b25`
`quay.io/cilium/hubble-relay:v1.9.18@sha256:2b4d1a7a530de8f48a555b42d3f4d834249d37e668e5ecd2351c5e24f3bb2b25`

### operator-aws

`docker.io/cilium/operator-aws:v1.9.18@sha256:64c96a7c5108a9075e9f8cd183ae0e799ca494854345fb1052144284e3d58598`
`quay.io/cilium/operator-aws:v1.9.18@sha256:64c96a7c5108a9075e9f8cd183ae0e799ca494854345fb1052144284e3d58598`

### operator-azure

`docker.io/cilium/operator-azure:v1.9.18@sha256:8c6c053f83d0e5eb0abf223d45eacd6cb3b563a84f5ecbadcfeecd54202cc7b7`
`quay.io/cilium/operator-azure:v1.9.18@sha256:8c6c053f83d0e5eb0abf223d45eacd6cb3b563a84f5ecbadcfeecd54202cc7b7`

### operator-generic

`docker.io/cilium/operator-generic:v1.9.18@sha256:d1cd7f32b74a35082f27cd64706d02550145cda083a295322bacf02be01daf0c`
`quay.io/cilium/operator-generic:v1.9.18@sha256:d1cd7f32b74a35082f27cd64706d02550145cda083a295322bacf02be01daf0c`

### operator

`docker.io/cilium/operator:v1.9.18@sha256:88047a736f179db44c68d4d6c8f6cecd8d30b9b381f8fa90640bdb4a3c20b040`
`quay.io/cilium/operator:v1.9.18@sha256:88047a736f179db44c68d4d6c8f6cecd8d30b9b381f8fa90640bdb4a3c20b040`