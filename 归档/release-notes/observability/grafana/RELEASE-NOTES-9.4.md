---
title: grafana v9.4 Release Notes
description: grafana v9.4 Release Notes — Kubernetes 生产运维知识库
summary: grafana v9.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- rbac
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v9.4 Release Notes 是什么
- 如何 grafana v9.4 Release Notes
trigger_keywords:
- grafana
- v9.4
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




# grafana v9.4 Release Notes

Source: [v9.4.17](https://github.com/grafana/grafana/releases/tag/v9.4.17)

[Download page](https://grafana.com/grafana/download/9.4.17)
[What's new highlights](https://grafana.com/docs/grafana/latest/whatsnew/)

### Features and enhancements

- **Chore:** Upgrade Go to 1.20.10. [#76370](https://github.com/grafana/grafana/issues/76370), [@zerok](https://github.com/zerok)
- **SSE:** DSNode to update result with names to make each value identifiable by labels (only Graphite and TestData). [#74615](https://github.com/grafana/grafana/issues/74615), [@yuri-tceretian](https://github.com/yuri-tceretian)

### Bug fixes

- **BrowseDashboards:** Only remember the most recent expanded folder. [#74812](https://github.com/grafana/grafana/issues/74812), [@joshhunt](https://github.com/joshhunt)
- **SQL Datasources:** Fix variable throwing error if query returns no data. [#74609](https://github.com/grafana/grafana/issues/74609), [@mdvictor](https://github.com/mdvictor)
- **RBAC:** Chore fix hasPermissionInOrg. (Enterprise)

<!-- risk-assessed -->
