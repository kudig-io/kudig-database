---
title: kind v0.28 Release Notes
description: kind v0.28 Release Notes — Kubernetes 生产运维知识库
summary: kind v0.28 Release Notes — Kubernetes 生产运维知识库
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
- kind v0.28 Release Notes 是什么
- 如何 kind v0.28 Release Notes
trigger_keywords:
- kind
- v0.28
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kind v0.28 Release Notes

Source: [v0.28.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.28.0)

This release moves to [[Kubernetes|Kubernetes]] to 1.33.1 by default.

<h1 id="breaking-changes">Breaking Changes</h1>

The default node image is now `kindest/node:v1.33.1@sha256:8d866994839cd096b3590681c55a6fa4a071fdaf33be7b9660e5697d2ed13002`

<h1 id="new-features">New Features</h1>

- Updated to [[containerd|containerd]] 2.1 and runc 1.3
- Updated default node image to Kubernetes 1.33.1
- Updated go to 1.24.2

Images pre-built for this release:
- v1.33.1: `kindest/node:v1.33.1@sha256:8d866994839cd096b3590681c55a6fa4a071fdaf33be7b9660e5697d2ed13002`
- v1.32.5: `kindest/node:v1.32.5@sha256:36187f6c542fa9b78d2d499de4c857249c5a0ac8cc2241bef2ccd92729a7a259`
- v1.31.9: `kindest/node:v1.31.9@sha256:156da58ab617d0cb4f56bbdb4b493f4dc89725505347a4babde9e9544888bb92`
- v1.30.13: `kindest/node:v1.30.13@sha256:8673291894dc400e0fb4f57243f5fdc6e355ceaa765505e0e73941aa1b6e0b80`

**NOTE**: You _must_ use the `@sha256` digest to guarantee an image built for this release, until such a time as we switch to a different tagging scheme. Even then we will highly encourage digest pinning for security and reproducibility reasons.

See also: 
- https://kind.sigs.k8s.io/docs/user/quick-start/#creating-a-cluster
- https://kind.sigs.k8s.io/docs/user/quick-start/#building-images

NOTE: These node images support amd64 and arm64, both of our supported platforms. **You must use the same platform as your host,** for more context see https://github.com/kubernetes-sigs/kind/issues/2718

<h1 id="fixes">Fixes</h1>

- Node image now sets `net.ipv4.conf.all.arp_ignore` to 0 to address some container networking failures

<h1 id="contributors">Contributors</h1>

**Thank you to everyone who contributed to this kind over the years!**

Committers for this release:
- @adrianmoisey
- @AkihiroSuda
- @aojea
- @BenTheElder
- @k8s-ci-robot
- @mwdomino
- @null
- @pellared
- @rooty0
- @shaneutt
- @vonhatnam1212

<!-- risk-assessed -->
