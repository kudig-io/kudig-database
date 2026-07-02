---
title: containerd v1.7 Release Notes
description: containerd v1.7 Release Notes — Kubernetes 生产运维知识库
summary: containerd v1.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
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
- containerd v1.7 Release Notes 是什么
- 如何 containerd v1.7 Release Notes
trigger_keywords:
- containerd
- v1.7
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




# [[containerd|containerd]] v1.7 Release Notes

Source: [v1.7.30](https://github.com/containerd/containerd/releases/tag/v1.7.30)

Welcome to the v1.7.30 release of containerd!

The thirtieth patch release for containerd 1.7 contains various fixes
and updates.

### Highlights

#### [[concepts/container-runtime.md|Container Runtime]] Interface (CRI)

* **Fix NRI dropping requested CDI devices silently** ([#12650](https://github.com/containerd/containerd/pull/12650))
* **Redact all query parameters in CRI error logs** ([#12551](https://github.com/containerd/containerd/pull/12551))

#### Runtime

* **Update runc binary to v1.3.4** ([#12619](https://github.com/containerd/containerd/pull/12619))

Please try out the release binaries and report any issues at
https://github.com/containerd/containerd/issues.

### Contributors

* Derek McGowan
* Akihiro Suda
* Austin Vazquez
* Mike Brown
* Wei Fu
* Andrey Noskov
* CrazyMax
* Davanum Srinivas
* Jin Dong
* Krisztian Litkey
* Maksym Pavlenko
* Paweł Gronowski
* Phil Estes
* Samuel Karp

### Changes
<details><summary>26 commits</summary>
<p>

* Prepare release notes for v1.7.30 ([#12652](https://github.com/containerd/containerd/pull/12652))
  * [`3d0ca6d2e`](https://github.com/containerd/containerd/commit/3d0ca6d2e7ba597bf0423e5f5f49e47b81c1e7a0) Prepare release notes for v1.7.30
* Fix NRI dropping requested CDI devices silently ([#12650](https://github.com/containerd/containerd/pull/12650))
  * [`0bc74f47e`](https://github.com/containerd/containerd/commit/0bc74f47e708bd843e676c5a8617f0498ea6459a) cri,nri: don't drop requested CDI devices silently.
* script/setup/install-cni: install CNI plugins v1.9.0 ([#12660](https://github.com/containerd/containerd/pull/12660))
  * [`7db16b562`](https://github.com/containerd/containerd/commit/7db16b5627a550caf05d9a902e16cb0d04bf1ee1) script/setup/install-cni: install CNI plugins v1.9.0
* go.mod: golang.org/x/crypto v0.45.0 (drop support for Go 1.23) ([#12640](https://github.com/containerd/containerd/pull/12640))
  * [`bca897b47`](https://github.com/containerd/containerd/commit/bca897b4739fef9b6a34c54ac6050d1621e53f92) go.mod: golang.org/x/crypto v0.45.0
  * [`37cbd2224`](https://github.com/containerd/containerd/commit/37cbd2224e674c317e25b03bbf4ab5a9ed644a5d) CI: drop Go 1.23
  * [`ee49d1747`](https://github.com/containerd/containerd/commit/ee49d1747c357cd45119750d4db464f957f4d793) Update Go requirements in BUILDING
* ci: bump Go 1.24.11, 1.25.5 ([#12627](https://github.com/containerd/containerd/pull/12627))
  * [`145978224`](https://github.com/containerd/containerd/commit/1459782247cc3eb2a6a2693a614766a86bcf6826) ci: bump Go 1.24.11, 1.25.5
  * [`3dbadfaa1`](https://github.com/containerd/containerd/commit/3dbadfaa158ebff9862606fe14a454f668a41d49) ci: bump Go 1.24.10, 1.25.4
  * [`2bac971f0`](https://github.com/containerd/containerd/commit/2bac971f0c91a1668bf1ec9c5ec1aa3f0484bd03) ci(release): set GO_VERSION in Dockerfile
* Update runc binary to v1.3.4 ([#12619](https://github.com/containerd/containerd/pull/12619))
  * [`34b89a574`](https://github.com/containerd/containerd/commit/34b89a5744330f098774356cc592bcc38f3279d8) runc: Update runc binary to v1.3.4
* ci: update CIFuzz actions to support Ubuntu 24.04 ([#12635](https://github.com/containerd/containerd/pull/12635))
  * [`6e0dd8956`](https://github.com/containerd/containerd/commit/6e0dd89566553cc1be1cd4823df5d5faeb839b31) ci: update CIFuzz actions to support Ubuntu 24.04
* build(deps): bump github.com/opencontainers/selinux ([#12591](https://github.com/containerd/containerd/pull/12591))
  * [`3eea2a4af`](https://github.com/containerd/containerd/commit/3eea2a4af8b7e20c6c299551782aeccab0a10c0a) build(deps): bump github.com/opencontainers/selinux
* remove sha256-simd ([#12576](https://github.com/containerd/containerd/pull/12576))
  * [`1194f5128`](https://github.com/containerd/containerd/commit/1194f5128f342d40b3ef0c3a4baea3d5ecf15d3b) remove sha256-simd
* .github: skip 5 critest cases for window-2022 ([#12586](https://github.com/containerd/containerd/pull/12586))
  * [`ce2d3a67f`](https://github.com/containerd/containerd/commit/ce2d3a67f2b4e3dc2e804bb9ecde609f04e1e1f6) .github: skip 5 critest cases in window CI pipeline
* Redact all query parameters in CRI error logs ([#12551](https://github.com/containerd/containerd/pull/12551))
  * [`65271ea89`](https://github.com/containerd/containerd/commit/65271ea895cd62016f2baf0e758b1cd7388344e7) fix: redact all query parameters in CRI error logs
</p>
</details>

### Dependency Changes

* **github.com/cyphar/filepath-securejoin**  v0.5.1 **_new_**
* **github.com/opencontainers/selinux**      v1.11.0 -> v1.13.1
* **golang.org/x/crypto**                    v0.40.0 -> v0.45.0
* **golang.org/x/mod**                       v0.26.0 -> v0.29.0
* **golang.org/x/net**                       v0.42.0 -> v0.47.0
* **golang.org/x/sync**                      v0.16.0 -> v0.18.0
* **golang.org/x/sys**                       v0.34.0 -> v0.38.0
* **golang.org/x/term**                      v0.33.0 -> v0.37.0
* **golang.org/x/text**                      v0.27.0 -> v0.31.0

Previous release can be found at [v1.7.29](https://github.com/containerd/containerd/releases/tag/v1.7.29)


<!-- risk-assessed -->
