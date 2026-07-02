---
title: kind v0.29 Release Notes
description: kind v0.29 Release Notes — Kubernetes 生产运维知识库
summary: kind v0.29 Release Notes — Kubernetes 生产运维知识库
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
- kind v0.29 Release Notes 是什么
- 如何 kind v0.29 Release Notes
trigger_keywords:
- kind
- v0.29
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




# kind v0.29 Release Notes

Source: [v0.29.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.29.0)

This is a quick security release to pickup the [[containerd|containerd]] 2.1.1 [CVE-2025-47290](https://www.cve.org/CVERecord?id=CVE-2025-47290) fix.
See [v0.28.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.28.0) (release days ago!) for full release notes with recent changes:
https://github.com/kubernetes-sigs/kind/releases/tag/v0.28.0

<h1 id="breaking-changes">Breaking Changes</h1>

The default node image is now `kindest/node:v1.33.1@sha256:050072256b9a903bd914c0b2866828150cb229cea0efe5892e2b644d5dd3b34f`

<h1 id="new-features">New Features</h1>

- Updated to containerd 2.1.1

Images pre-built for this release:
- v1.33.1: `kindest/node:v1.33.1@sha256:050072256b9a903bd914c0b2866828150cb229cea0efe5892e2b644d5dd3b34f`
- v1.32.5: `kindest/node:v1.32.5@sha256:e3b2327e3a5ab8c76f5ece68936e4cafaa82edf58486b769727ab0b3b97a5b0d`
- v1.31.9: `kindest/node:v1.31.9@sha256:b94a3a6c06198d17f59cca8c6f486236fa05e2fb359cbd75dabbfc348a10b211`
- v1.30.13: `kindest/node:v1.30.13@sha256:397209b3d947d154f6641f2d0ce8d473732bd91c87d9575ade99049aa33cd648`

**NOTE**: You _must_ use the `@sha256` digest to guarantee an image built for this release, until such a time as we switch to a different tagging scheme. Even then we will highly encourage digest pinning for security and reproducibility reasons.

See also: 
- https://kind.sigs.k8s.io/docs/user/quick-start/#creating-a-cluster
- https://kind.sigs.k8s.io/docs/user/quick-start/#building-images

NOTE: These node images support amd64 and arm64, both of our supported platforms. **You must use the same platform as your host,** for more context see https://github.com/kubernetes-sigs/kind/issues/2718

<h1 id="fixes">Fixes</h1>

- Fixes containerd [CVE-2025-47290](https://www.cve.org/CVERecord?id=CVE-2025-47290) / GHSA-cm76-qm8v-3j95

<h1 id="contributors">Contributors</h1>

Committers for this release:
- @BenTheElder
- @k8s-ci-robot
- @stmcginnis

(Please see v0.28.0 https://github.com/kubernetes-sigs/kind/releases/tag/v0.28.0#Contributors)

<!-- risk-assessed -->
