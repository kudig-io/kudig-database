---
title: grafana v4.4 Release Notes
description: grafana v4.4 Release Notes — Kubernetes 生产运维知识库
summary: grafana v4.4 Release Notes — Kubernetes 生产运维知识库
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
- grafana v4.4 Release Notes 是什么
- 如何 grafana v4.4 Release Notes
trigger_keywords:
- grafana
- v4.4
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




# grafana v4.4 Release Notes

Source: [v4.4.3](https://github.com/grafana/grafana/releases/tag/v4.4.3)

[Download Page](http://grafana.org/download)
[Installation Guide](http://docs.grafana.org/installation/)

To view screenshots and examples of the new features read the [What's New in v4.4](http://docs.grafana.org/guides/whats-new-in-v4-4/) article.

## Bug Fixes

* **Search**: Fix for issue that casued search view to hide  when you clicked starred or tags filters, fixes [#8981](https://github.com/grafana/grafana/issues/8981)
* **Modals**: ESC key now closes modal again, fixes [#8981](https://github.com/grafana/grafana/issues/8988), thx [@j-white](https://github.com/j-white)

<!-- risk-assessed -->
