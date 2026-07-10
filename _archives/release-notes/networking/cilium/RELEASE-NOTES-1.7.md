---
title: cilium v1.7 Release Notes
description: cilium v1.7 Release Notes — Kubernetes 生产运维知识库
summary: cilium v1.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- cilium v1.7 Release Notes 是什么
- 如何 cilium v1.7 Release Notes
trigger_keywords:
- cilium
- v1.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Cilium|cilium]] v1.7 Release Notes

Source: v1.7.16](https://github.com/cilium/cilium/releases/tag/v1.7.16)

We are pleased to release Cilium v1.7.16. This release updates [[Envoy|Envoy]] to v1.17.2 to address CVE-2021-28682, CVE-2021-28683, CVE-2021-29258. Cilium v1.7.16 will likely be the last release of the v1.7.x series as this series reaches end of life with the release of v1.10.

Summary of Changes
------------------

**Bugfixes:**
* Fix possible deadlock when querying network interfaces for arping (#15430, @brb)
* Envoy is updated to release 1.17.2 (#15740, @jrajahalme)
* Fix channel panic from ipcache kvstore reconnect (Backport PR #15767, Upstream PR #15668, @jomenxiao)
* vendor: Bump vishvananda/netlink dependency (#15408, @tgraf)

**CI Changes:**
* test: ensure kubectl version is available for test run (Backport PR #15751, Upstream PR #15748, @nebril)

**Misc Changes:**
* [1.7] contrib: Skip image digests during release prep (#15289, @joestringer)
* [v1.7]  Bump cilium-runtime image dependency (#15720, @joestringer)
* backporting: Update instructions for backporting workflow (Backport PR #15767, Upstream PR #15118, @aditighag)
* bugtool: dump iptables-legacy and iptables-nft (Backport PR #15401, Upstream PR #15363, @h3llix)
* Bugtool: route tables are dynamically dumped (Backport PR #15401, Upstream PR #14488, @youssefazrak)
* build(deps): bump actions/download-artifact from 4a7a711286f30c025902c28b541c10e147a9b843 to 2.0.9 (#15652, @dependabot[bot])
* build(deps): bump actions/upload-artifact from e448a9b857ee2131e752b06002bf0e093c65e571 to 2.2.3 (#15643, @dependabot[bot])
* build(deps): bump docker/setup-buildx-action from 154c24e1f33dbb5865a021c99f1318cfebf27b32 to 1.1.2 (#15671, @dependabot[bot])
* build(deps): bump pygments from 2.4.2 to 2.7.4 in /Documentation (Backport PR #15529, Upstream PR #15495, @dependabot[bot])
* build(deps): update docker/build-push-action requirement to e1b7f96249f2e4c8e4ac1519b9608c0d48944a1f (#15686, @dependabot[bot])
* contrib: fix remote overriding (Backport PR #15401, Upstream PR #15328, @kaworu)
* docs: Fix commands for IPSec key rotations (Backport PR #15529, Upstream PR #15481, @pchaigno)
* docs: Hide "Edit on GitHub" buttons (Backport PR #15612, Upstream PR #15579, @joestringer)
* Documentation: fix key rotation command in encryption guide (Backport PR #15401, Upstream PR #15365, @mauriciovasquezbernal)
* Improve release scripts (Backport PR #15529, Upstream PR #15294, @joestringer)

**Other Changes:**
* install: Update image digests for v1.7.15 (#15293, @joestringer)

## Docker Manifests

### cilium

`docker.io/cilium/cilium:v1.7.16@sha256:ba7d3256138fed70f772d35202454f44f3255ad1b0ccf6916ae5a5360fa3a524`
`quay.io/cilium/cilium:v1.7.16@sha256:ba7d3256138fed70f772d35202454f44f3255ad1b0ccf6916ae5a5360fa3a524`

### docker-plugin

`docker.io/cilium/docker-plugin:v1.7.16@sha256:f728a1339cbe16a94693b135535ae5dedb6fd34ab5f2d10a90dd294bf586e1da`
`quay.io/cilium/docker-plugin:v1.7.16@sha256:f728a1339cbe16a94693b135535ae5dedb6fd34ab5f2d10a90dd294bf586e1da`

### operator

`docker.io/cilium/operator:v1.7.16@sha256:88d68932abf6b29ff88c48e5cb4f143a57e78d213ced383427f988a87d771a0d`
`quay.io/cilium/operator:v1.7.16@sha256:88d68932abf6b29ff88c48e5cb4f143a57e78d213ced383427f988a87d771a0d`

<!-- risk-assessed -->
