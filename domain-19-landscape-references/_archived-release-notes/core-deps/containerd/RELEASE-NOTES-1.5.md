---
title: containerd v1.5 Release Notes
description: containerd v1.5 Release Notes — Kubernetes 生产运维知识库
summary: containerd v1.5 Release Notes — Kubernetes 生产运维知识库
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
estimated_read_time: 5min
intent_queries:
- containerd v1.5 Release Notes 是什么
- 如何 containerd v1.5 Release Notes
trigger_keywords:
- containerd
- v1.5
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




# [[containerd|containerd]] v1.5 Release Notes

Source: [v1.5.18](https://github.com/containerd/containerd/releases/tag/v1.5.18)

Welcome to the v1.5.18 release of containerd!

The eighteenth patch release for containerd 1.5 includes fixes for CVE-2023-25153 and CVE-2023-25173
along with a security update for Go.

### Notable Updates

* **Fix supplementary groups not being set up properly** ([GHSA-hmfx-3pcx-653p](https://github.com/containerd/containerd/security/advisories/GHSA-hmfx-3pcx-653p))
* **Fix OCI image importer memory exhaustion** ([GHSA-259w-8hf6-59c2](https://github.com/containerd/containerd/security/advisories/GHSA-259w-8hf6-59c2))
* **Update Go to 1.19.6** ([#8112](https://github.com/containerd/containerd/pull/8112))

See the changelog for complete list of changes

Please try out the release binaries and report any issues at
https://github.com/containerd/containerd/issues.

### Contributors

* Akihiro Suda
* Derek McGowan
* Ye Sijun
* Samuel Karp
* Phil Estes
* Swagat Bora
* Wei Fu

### Changes
<details><summary>17 commits</summary>
<p>

* [release/1.5] Prepare release notes for v1.5.18 ([#8117](https://github.com/containerd/containerd/pull/8117))
  * [`ddf9de6cb`](https://github.com/containerd/containerd/commit/ddf9de6cbb30f9edf1b04d304eac67d1383e406b) Prepare release notes for v1.5.18
* Github Security Advisory [GHSA-hmfx-3pcx-653p](https://github.com/containerd/containerd/security/advisories/GHSA-hmfx-3pcx-653p)
  * [`a62c38bf2`](https://github.com/containerd/containerd/commit/a62c38bf2173faa813018939710fc8491e4f7dba) oci: fix additional GIDs
  * [`3b89da580`](https://github.com/containerd/containerd/commit/3b89da580b76471d6c03cb1fc6c14db6aa23d3db) oci: fix loop iterator aliasing
  * [`b07ec6b25`](https://github.com/containerd/containerd/commit/b07ec6b251bd51f06bc72ef408f31e3f6e6e87f9) oci: skip checking gid for WithAppendAdditionalGroups
  * [`356672cb5`](https://github.com/containerd/containerd/commit/356672cb56fd5a0eed11e5089ac824c7ab09ffac) refactor: reduce duplicate code
  * [`6a7b7617c`](https://github.com/containerd/containerd/commit/6a7b7617cfbd90009a2e05e0e5eff4ef92028d7b) add WithAdditionalGIDs test
  * [`832bcf300`](https://github.com/containerd/containerd/commit/832bcf300b1ec29c9b08326aab2d4eafee58dd85) add WithAppendAdditionalGroups helper
* Github Security Advisory [GHSA-259w-8hf6-59c2](https://github.com/containerd/containerd/security/advisories/GHSA-259w-8hf6-59c2)
  * [`19a347e45`](https://github.com/containerd/containerd/commit/19a347e456f3ab66909edca5351aa6f4ed1be177) importer: stream oci-layout and manifest.json
* [release/1.5] Go 1.19.6 ([#8112](https://github.com/containerd/containerd/pull/8112))
  * [`4209dc243`](https://github.com/containerd/containerd/commit/4209dc243005af926fbd0382cbd6cf4cd26beeef) Go 1.19.6
* [release/1.5] Fix retry logic within devmapper device deactivation ([#8089](https://github.com/containerd/containerd/pull/8089))
  * [`0d16d045d`](https://github.com/containerd/containerd/commit/0d16d045dfd0d800a00dc362736b815f6cc96de8) Fix retry logic within devmapper device deactivation
* [release/1.5] CI: skip some jobs when `repo != containerd/containerd` ([#8084](https://github.com/containerd/containerd/pull/8084))
  * [`34451bc66`](https://github.com/containerd/containerd/commit/34451bc66453aad2f911aa8bffa6817061e4a72b) CI: skip some jobs when `repo != containerd/containerd`
</p>
</details>

### Dependency Changes

This release has no dependency changes

Previous release can be found at [v1.5.17](https://github.com/containerd/containerd/releases/tag/v1.5.17)


<!-- risk-assessed -->
