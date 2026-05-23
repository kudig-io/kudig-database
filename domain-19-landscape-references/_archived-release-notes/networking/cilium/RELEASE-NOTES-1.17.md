---
title: cilium v1.17 Release Notes
description: cilium v1.17 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- apiserver
- envoy
- cilium
- docker
- ingress
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cilium v1.17 Release Notes 是什么
- 如何 cilium v1.17 Release Notes
trigger_keywords:
- cilium
- v1.17
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
created: "2026-05-23"
---

# [[Cilium|cilium]] v1.17 Release Notes

Source: [v1.17.14](https://github.com/cilium/cilium/releases/tag/v1.17.14)

Summary of Changes
------------------

**Bugfixes:**
* bpf: nodeport: use hairpin redirect for L7 LB on bridge devices (Backport PR cilium/cilium#44709, Upstream PR cilium/cilium#44658, @smagnani96)
* Fix [[Envoy|envoy]] admin socket being created as world-accessible (Backport PR cilium/cilium#44591, Upstream PR cilium/cilium#44512, @0xch4z)
* l7lb: fix bypassing [[Ingress|ingress]] policies for local backends (Backport PR cilium/cilium#44805, Upstream PR cilium/cilium#44693, @smagnani96)

**CI Changes:**
* pkg: Mark node_linux_test.go as unparallel (Backport PR cilium/cilium#44591, Upstream PR cilium/cilium#38172, @jschwinger233)

**Misc Changes:**
* [1.17] gha: Use eks 1.30 from us-west-2 (cilium/cilium#44752, @sayboras)
* chore(deps): update all github action dependencies (v1.17) (cilium/cilium#44376, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.17) (cilium/cilium#44485, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.17) (cilium/cilium#44583, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.17) (cilium/cilium#44687, @cilium-renovate[bot])
* chore(deps): update all github action dependencies (v1.17) (cilium/cilium#44794, @cilium-renovate[bot])
* chore(deps): update all-dependencies (v1.17) (cilium/cilium#44373, @cilium-renovate[bot])
* chore(deps): update base-images to v1.25.8 (v1.17) (cilium/cilium#44811, @cilium-renovate[bot])
* chore(deps): update cilium/cilium-cli action to v0.19.1 (v1.17) (cilium/cilium#44345, @cilium-renovate[bot])
* chore(deps): update dependency cilium/cilium-cli to v0.19.1 (v1.17) (cilium/cilium#44402, @cilium-renovate[bot])
* chore(deps): update dependency cilium/cilium-cli to v0.19.2 (v1.17) (cilium/cilium#44552, @cilium-renovate[bot])
* chore(deps): update dependency mfridman/protoc-gen-go-json to v1.6.0 (v1.17) (cilium/cilium#44684, @cilium-renovate[bot])
* chore(deps): update dependency protocolbuffers/protobuf to v34 (v1.17) (cilium/cilium#44584, @cilium-renovate[bot])
* chore(deps): update dependency sphinx-tabs to v3.5.0 (v1.17) (cilium/cilium#44685, @cilium-renovate[bot])
* chore(deps): update docker.io/library/ubuntu:24.04 docker digest to d1e2e92 (v1.17) (cilium/cilium#44481, @cilium-renovate[bot])
* chore(deps): update gcr.io/distroless/static:nonroot docker digest to e3f9456 (v1.17) (cilium/cilium#44798, @cilium-renovate[bot])
* chore(deps): update gcr.io/distroless/static:nonroot docker digest to f512d81 (v1.17) (cilium/cilium#44581, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/certgen docker tag to v0.4.1 (v1.17) (cilium/cilium#44686, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1770979049-232ed4a26881e4ab4f766f251f258ed424fff663 (v1.17) (cilium/cilium#44374, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1771585526-532310e626e42c7086de4ef3ea913736125bbd31 (v1.17) (cilium/cilium#44483, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1773220507-ffc0948a7ec4868e6b552a71cf4d3860e78b53cc (v1.17) (cilium/cilium#44682, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1773405792-4046425704636ea5b770460c20c065069cf572dc (v1.17) (cilium/cilium#44792, @cilium-renovate[bot])
* chore(deps): update quay.io/cilium/cilium-envoy docker tag to v1.35.9-1773656288-7b052e66eb2cfc5ac130ce0a5be66202a10d83be (v1.17) (cilium/cilium#44808, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.17) (patch) (cilium/cilium#44375, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.17) (patch) (cilium/cilium#44484, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.17) (patch) (cilium/cilium#44683, @cilium-renovate[bot])
* chore(deps): update stable lvh-images (v1.17) (patch) (cilium/cilium#44793, @cilium-renovate[bot])
* fix(deps): update k8s.io patch updates stable (v1.17) (patch) (cilium/cilium#44508, @cilium-renovate[bot])
* fix(deps): update k8s.io patch updates stable to v0.32.13 (v1.17) (patch) (cilium/cilium#44582, @cilium-renovate[bot])
* fix(deps): update k8s.io/utils digest to b8788ab (v1.17) (cilium/cilium#44482, @cilium-renovate[bot])
* Include the results of `find /sys/fs/bpf` in bugtool output (Backport PR cilium/cilium#44591, Upstream PR cilium/cilium#38980, @ti-mo)

**Other Changes:**
* Fix gke channels (cilium/cilium#44558, @Artyop)
* install: Update image digests for v1.17.13 (cilium/cilium#44325, @cilium-release-bot[bot])


## Docker Manifests

### cilium

`quay.io/cilium/cilium:v1.17.14@sha256:cdcfab5b4466d607f713d1ada281ee4513dd3982eb2c48ef2d0cc708cc3d1ba3`

### clustermesh-apiserver

`quay.io/cilium/clustermesh-apiserver:v1.17.14@sha256:6cc4e47b2a50649e739dbb61f266497e7ef53d048b60dc32ba563bd4efd7f0ba`

### docker-plugin

`quay.io/cilium/docker-plugin:v1.17.14@sha256:087072e60566cc37e21facec0e4096d49bef2e83cd340896ae477a7746819067`

### hubble-relay

`quay.io/cilium/hubble-relay:v1.17.14@sha256:ce5b991bb011fa744c94e04fd7f1a7d3c8e3ce7d2da0652766abe6c468ead990`

### operator-alibabacloud

`quay.io/cilium/operator-alibabacloud:v1.17.14@sha256:bdfa469e453986b995632f889cfb90bc501b80a809ff4b8be8d236eba5fcc2cb`

### operator-aws

`quay.io/cilium/operator-aws:v1.17.14@sha256:182c13e6edda041bfc885932d5e87b1d8ac3588f6f6af309944efee46a2193b2`

### operator-azure

`quay.io/cilium/operator-azure:v1.17.14@sha256:a462e7265ee34a667905c6144b7aa5d5ee8328ee1a4eca3f44bdc1463cc69741`

### operator-generic

`quay.io/cilium/operator-generic:v1.17.14@sha256:773886ec9337f6628ba84e36ac7e3e554c1622024fc2a8b04a3377970aee8889`

### operator

`quay.io/cilium/operator:v1.17.14@sha256:2113d66000847f39135722c61545ddb2c1bbd9fc4479f10dca175fc4bf9bda1b`

