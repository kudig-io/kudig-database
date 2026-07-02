---
title: containerd v1.6 Release Notes
description: containerd v1.6 Release Notes — Kubernetes 生产运维知识库
summary: containerd v1.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- job
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- containerd v1.6 Release Notes 是什么
- 如何 containerd v1.6 Release Notes
trigger_keywords:
- containerd
- v1.6
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




# [[containerd|containerd]] v1.6 Release Notes

Source: [v1.6.39](https://github.com/containerd/containerd/releases/tag/v1.6.39)

Welcome to the v1.6.39 release of containerd!

The thirty-ninth patch release for containerd 1.6 contains various fixes
and updates.

### Highlights

#### Runtime

* Fix close container io not closed when runtime create failed ([#12052](https://github.com/containerd/containerd/pull/12052))
* Update runc binary to v1.3.0 ([#11799](https://github.com/containerd/containerd/pull/11799))

Please try out the release binaries and report any issues at
https://github.com/containerd/containerd/issues.

### Contributors

* Akihiro Suda
* Austin Vazquez
* Phil Estes
* Derek McGowan
* Kirtana Ashok
* Akhil Mohan
* Maksym Pavlenko
* Mike Brown
* madraceee
* ningmingxiao
* zounengren

### Changes
<details><summary>33 commits</summary>
<p>

* Prepare release notes for v1.6.39 ([#12045](https://github.com/containerd/containerd/pull/12045))
  * [`22134cbfe`](https://github.com/containerd/containerd/commit/22134cbfea295649f9c43212c1fb14444cfe93ed) Prepare release notes for v1.6.39
* ci: bump Go 1.23.11, 1.24.5 ([#12118](https://github.com/containerd/containerd/pull/12118))
  * [`067a639f6`](https://github.com/containerd/containerd/commit/067a639f6076d3a655533edf470aa0534930eb0d) ci: bump Go 1.23.11, 1.24.5
* Backport windows test fixes ([#12122](https://github.com/containerd/containerd/pull/12122))
  * [`9cc952fb0`](https://github.com/containerd/containerd/commit/9cc952fb0b8e092b40c6187209dc9624377cb6cd) Fix intermittent test failures on Windows CIs
  * [`555a34af0`](https://github.com/containerd/containerd/commit/555a34af0511f64eafcc1141b5a0a0e996f2751e) Remove WS2025 from CIs due to regression
* ci: use fedora 39 archive ([#12125](https://github.com/containerd/containerd/pull/12125))
  * [`b58df07d6`](https://github.com/containerd/containerd/commit/b58df07d680e1872bd598b48c4b304c81d6697e4) ci: use fedora 39 archive
* go.mod: github.com/containerd/btrfs v1.0.1 ([#12105](https://github.com/containerd/containerd/pull/12105))
  * [`fa4b325e0`](https://github.com/containerd/containerd/commit/fa4b325e079be2b2b859f8dc8d6d1bab4ea14d29) go.mod: github.com/containerd/btrfs v1.0.1
* go.mod:  golang.org/x/* latest,  github.com/go-jose/go-jose/v3 v3.0.4 ([#12095](https://github.com/containerd/containerd/pull/12095))
  * [`2c9f5778f`](https://github.com/containerd/containerd/commit/2c9f5778f04dc51ffa26f9dc9fae2bdd8b9699c8) Fix lint failures
  * [`b2576bb82`](https://github.com/containerd/containerd/commit/b2576bb82454a36b0f3f65e906d1365f44003d61) go.mod: github.com/go-jose/go-jose/v3 v3.0.4
  * [`262e98e90`](https://github.com/containerd/containerd/commit/262e98e90504eca34a2420003d3eaaffd353cd46) go.mod: golang.org/x/* latest
* Fix close container io not closed when runtime create failed ([#12052](https://github.com/containerd/containerd/pull/12052))
  * [`22f669a7c`](https://github.com/containerd/containerd/commit/22f669a7c0bc30beaa7337a02646ec882d3f2174) bugfix:close container io when runtime create failed
* backport windows runner and golang toolchain updates ([#12005](https://github.com/containerd/containerd/pull/12005))
  * [`c165cc68b`](https://github.com/containerd/containerd/commit/c165cc68beec6ab59f037da8cfb37fe768e98848) ci: bump Go 1.24.4 in CI
  * [`ffacabc05`](https://github.com/containerd/containerd/commit/ffacabc054b3fc21eea11482aff2f56f732c0526) ci: bump golang [1.23.9, 1.24.3] in build and release
  * [`3ec9965e8`](https://github.com/containerd/containerd/commit/3ec9965e8e5669cdd10813d4f5dc71df46547fbf) use go1.23.8 as the default go version
  * [`e62a059a2`](https://github.com/containerd/containerd/commit/e62a059a2a0b2b63c60bac0130d4053eb1b4207a) update to go 1.24.2, 1.23.8
  * [`d430f3277`](https://github.com/containerd/containerd/commit/d430f3277ea945385d408e6b1ffde9ab7e8ac9f5) Enable CIs to run on WS2022 and WS2025
* Update runc binary to v1.3.0 ([#11799](https://github.com/containerd/containerd/pull/11799))
  * [`d00ccf523`](https://github.com/containerd/containerd/commit/d00ccf523dfee664e2ec158a19f88d731fdff237) Update runc binary to v1.3.0
* test: added runc v1 support in vagrant ([#11913](https://github.com/containerd/containerd/pull/11913))
  * [`9e49725bf`](https://github.com/containerd/containerd/commit/9e49725bf455606d0843360268acb549b0da7967) test: added runc v1 support in vagrant
* : Fix CI ([#11804](https://github.com/containerd/containerd/pull/11804))
  * [`57250c719`](https://github.com/containerd/containerd/commit/57250c7197b60b6a06d65f2c1a9b07b0b8605a83) Skip criu on Arms
  * [`9d350bbbd`](https://github.com/containerd/containerd/commit/9d350bbbdabb45ea248cd5266322965874290ed2) Address cgroup mountpoint does not exist
  * [`78cbefc95`](https://github.com/containerd/containerd/commit/78cbefc954ec04caa26e7e09b8d8de12960988a0) ci: update GitHub Actions release runner to ubuntu-24.04
* Update runc binary to v1.2.6 ([#11585](https://github.com/containerd/containerd/pull/11585))
  * [`2325157ed`](https://github.com/containerd/containerd/commit/2325157ed07ac38f8fe260063c1cd52d73a36a01) Update runc binary to v1.2.6
</p>
</details>

### Changes from containerd/btrfs
<details><summary>12 commits</summary>
<p>

* Fix `error: implicit declaration of function ‘memcpy’` ([containerd/btrfs#44](https://github.com/containerd/btrfs/pull/44))
  * [`3fb5c91`](https://github.com/containerd/btrfs/commit/3fb5c91f016ebdfc72a0c64e81889defdb1dd51d) CI: update (Go 1.23, etc.)
  * [`cab79ec`](https://github.com/containerd/btrfs/commit/cab79ec9ea7e1b910e9aef01afbf87efb57ee674) CI: enable [[Jobs|jobs]] for release/1.0
  * [`12b3998`](https://github.com/containerd/btrfs/commit/12b3998bdd04e4c8b36d69faf5e65d8157be94c8) Fix `error: implicit declaration of function ‘memcpy’`
* Update GitHub actions CI workflow ([containerd/btrfs#38](https://github.com/containerd/btrfs/pull/38))
  * [`5d1f727`](https://github.com/containerd/btrfs/commit/5d1f7270e597460ff18660b50a5fbc96d81dd6d6) Update GitHub actions CI workflow
* Upgrade Go compiler from Go 1.16 to Go 1.19 ([containerd/btrfs#39](https://github.com/containerd/btrfs/pull/39))
  * [`d16e22b`](https://github.com/containerd/btrfs/commit/d16e22bc2cf48d71f14ee79d1c3a6d8c944dd759) Upgrade Go compiler from Go 1.16 to Go 1.19
* replace pkg/errors ([containerd/btrfs#35](https://github.com/containerd/btrfs/pull/35))
  * [`9933796`](https://github.com/containerd/btrfs/commit/9933796ae83cea9d4d9b239c76440c1ff14c4e7b) replace pkg/errors
* Branch rename for GH Actions ([containerd/btrfs#33](https://github.com/containerd/btrfs/pull/33))
  * [`1aff978`](https://github.com/containerd/btrfs/commit/1aff97820a2be844266702bb611b1767d4cfcc00) Branch rename for GH Actions
</p>
</details>

### Dependency Changes

* **cloud.google.com/go/compute/metadata**  v0.2.3 -> v0.3.0
* **github.com/containerd/btrfs**           v1.0.0 -> v1.0.1
* **github.com/go-jose/go-jose/v3**         v3.0.3 -> v3.0.4
* **golang.org/x/crypto**                   v0.21.0 -> v0.40.0
* **golang.org/x/net**                      v0.23.0 -> v0.42.0
* **golang.org/x/oauth2**                   v0.11.0 -> v0.30.0
* **golang.org/x/sync**                     v0.3.0 -> v0.16.0
* **golang.org/x/sys**                      v0.18.0 -> v0.34.0
* **golang.org/x/term**                     v0.18.0 -> v0.33.0
* **golang.org/x/text**                     v0.14.0 -> v0.27.0
* **golang.org/x/time**                     1f47c861a9ac -> v0.12.0

Previous release can be found at [v1.6.38](https://github.com/containerd/containerd/releases/tag/v1.6.38)


<!-- risk-assessed -->
