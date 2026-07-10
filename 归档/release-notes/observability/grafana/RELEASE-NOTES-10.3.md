---
title: grafana v10.3 Release Notes
description: grafana v10.3 Release Notes — Kubernetes 生产运维知识库
summary: grafana v10.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v10.3 Release Notes 是什么
- 如何 grafana v10.3 Release Notes
trigger_keywords:
- grafana
- v10.3
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




# grafana v10.3 Release Notes

Source: [v10.3.12](https://github.com/grafana/grafana/releases/tag/v10.3.12)

[Download page](https://grafana.com/grafana/download/10.3.12)
[What's new highlights](https://grafana.com/docs/grafana/latest/whatsnew/)

### Bug fixes

- **Alerting:** Fix incorrect permission on POST external rule groups endpoint [CVE-2024-8118] [#93945](https://github.com/grafana/grafana/pull/93945), [@alexweav](https://github.com/alexweav)
- **Dashboard:** Make dashboard search faster [#94704](https://github.com/grafana/grafana/pull/94704), [@knuzhdin](https://github.com/knuzhdin)

<!-- risk-assessed -->
