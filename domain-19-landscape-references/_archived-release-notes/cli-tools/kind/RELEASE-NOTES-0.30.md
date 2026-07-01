---
title: kind v0.30 Release Notes
description: kind v0.30 Release Notes — Kubernetes 生产运维知识库
summary: kind v0.30 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kind v0.30 Release Notes 是什么
- 如何 kind v0.30 Release Notes
trigger_keywords:
- kind
- v0.30
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# kind v0.30 Release Notes

Source: [v0.30.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.30.0)

This is small release containing patched dependencies and [[Kubernetes|Kubernetes]] 1.34, as well as a bugfix for Kubernetes v1.33.0+ cluster reboots.

<h1 id="breaking-changes">Breaking Changes</h1>

The default node image is now `kindest/node:v1.34.0@sha256:7416a61b42b1662ca6ca89f02028ac133a309a2a30ba309614e8ec94d976dc5a`

<h1 id="new-features">New Features</h1>

- Updated to [[containerd|containerd]] 2.1.4

Images pre-built for this release:
- v1.34.0: `kindest/node:v1.34.0@sha256:7416a61b42b1662ca6ca89f02028ac133a309a2a30ba309614e8ec94d976dc5a`
- v1.33.4: `kindest/node:v1.33.4@sha256:25a6018e48dfcaee478f4a59af81157a437f15e6e140bf103f85a2e7cd0cbbf2`
- v1.32.8: `kindest/node:v1.32.8@sha256:abd489f042d2b644e2d033f5c2d900bc707798d075e8186cb65e3f1367a9d5a1`
- v1.31.12: `kindest/node:v1.31.12@sha256:0f5cc49c5e73c0c2bb6e2df56e7df189240d83cf94edfa30946482eb08ec57d2`

**NOTE**: You _must_ use the `@sha256` digest to guarantee an image built for this release, until such a time as we switch to a different tagging scheme. Even then we will highly encourage digest pinning for security and reproducibility reasons.

See also: 
- https://kind.sigs.k8s.io/docs/user/quick-start/#creating-a-cluster
- https://kind.sigs.k8s.io/docs/user/quick-start/#building-images

NOTE: These node images support amd64 and arm64, both of our supported platforms. **You must use the same platform as your host,** for more context see https://github.com/kubernetes-sigs/kind/issues/2718

<h1 id="fixes">Fixes</h1>

- Fix an issue with rebooting v1.33.0+ clusters https://github.com/kubernetes-sigs/kind/issues/3941
- Add priority class system-critical to kindnetd
- Fix HA control-plane loadbalancer for podman https://github.com/kubernetes-sigs/kind/pull/3962
- Fix node-image builds with relative source paths

<h1 id="contributors">Contributors</h1>

Committers for this release:
- @BenTheElder
- @dims
- @k8s-ci-robot
- @oduludo
- @stmcginnis
- @tchap
- @tom1299 