---
title: thanos v0.2 Release Notes
description: thanos v0.2 Release Notes — Kubernetes 生产运维知识库
summary: thanos v0.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- thanos v0.2 Release Notes 是什么
- 如何 thanos v0.2 Release Notes
trigger_keywords:
- thanos
- v0.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Thanos|thanos]] v0.2 Release Notes

Source: [v0.2.1](https://github.com/thanos-io/thanos/releases/tag/v0.2.1)

Xmas patch to release 2 critical fixes (Azure, DNS SD) and awesome, new store UI page.

This also includes first mitigation for https://github.com/improbable-eng/thanos/issues/335

Changelog also available [here](./CHANGELOG.md). 

### Added

- Relabel drop for Thanos Ruler to enable replica label drop and alert deduplication on AM side.
- Query: Stores UI page available at `/stores`.

![](./docs/img/query_ui_stores.png)

### Fixed

- Thanos Rule Alertmanager DNS SD bug.
- DNS SD bug when having SRV results with different ports.
- Move handling of HA alertmanagers to be the same as [[Prometheus|Prometheus]].
- Azure iteration implementation flaw.

<!-- risk-assessed -->
