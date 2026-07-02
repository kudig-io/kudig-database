---
title: grafana v7.1 Release Notes
description: grafana v7.1 Release Notes — Kubernetes 生产运维知识库
summary: grafana v7.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v7.1 Release Notes 是什么
- 如何 grafana v7.1 Release Notes
trigger_keywords:
- grafana
- v7.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# grafana v7.1 Release Notes

Source: [v7.1.5](https://github.com/grafana/grafana/releases/tag/v7.1.5)

[Download Page](https://grafana.com/grafana/download/7.1.5)
[What's New Highlights](https://grafana.com/docs/grafana/latest/guides/whats-new-in-v7-1/)
[Release Notes](https://community.grafana.com/t/release-notes-v7-1-x/32967)

### Features / Enhancements
* **Stats**: Stop counting the same user multiple times. [#26777](https://github.com/grafana/grafana/pull/26777), [@sakjur](https://github.com/sakjur)

### Bug Fixes
* **Alerting**: remove LongToWide call in alerting. [#27140](https://github.com/grafana/grafana/pull/27140), [@kylebrandt](https://github.com/kylebrandt)
* **AzureMonitor**: fix panic introduced in 7.1.4 when unit was unspecified and alias was used. [#27113](https://github.com/grafana/grafana/pull/27113), [@kylebrandt](https://github.com/kylebrandt)
* **Variables**: Fixes issue with All variable not being resolved. [#27151](https://github.com/grafana/grafana/pull/27151), [@hugohaggmark](https://github.com/hugohaggmark)

<!-- risk-assessed -->
