---
title: gatekeeper v3.11 Release Notes
description: gatekeeper v3.11 Release Notes — Kubernetes 生产运维知识库
summary: gatekeeper v3.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- containerd
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- gatekeeper v3.11 Release Notes 是什么
- 如何 gatekeeper v3.11 Release Notes
trigger_keywords:
- gatekeeper
- v3.11
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




# gatekeeper v3.11 Release Notes

Source: [v3.11.1](https://github.com/open-policy-agent/gatekeeper/releases/tag/v3.11.1)

## Bug Fixes
- cutpath for ../ paths (#2508) [#2508](https://github.com/open-policy-agent/gatekeeper/pull/2508) ([alex](https://github.com/open-policy-agent/gatekeeper/commit/6a534703ded69fb4aadc48b668a4f6bf4e5dd1ad))
- [release-3.11] fix golang.org/x/net and github.[[实体/containerd.md|containerd]]/containerd vulns (#2711) [#2711](https://github.com/open-policy-agent/gatekeeper/pull/2711) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/5ab9a969fba77a308914c79cd958e8ac039d2ec5))
- [release-3.11] cherry pick #2690 (#2717) [#2717](https://github.com/open-policy-agent/gatekeeper/pull/2717) ([Sertaç Özercan](https://github.com/open-policy-agent/gatekeeper/commit/042991e3c9f440f6aae367b90d1ed2b606279942))

## Chores
- Prepare v3.11.1 release (#2718) [#2718](https://github.com/open-policy-agent/gatekeeper/pull/2718) ([github-actions[bot]](https://github.com/open-policy-agent/gatekeeper/commit/1f6f701644ba11046074fe6675127151e91b7773))

<!-- risk-assessed -->
