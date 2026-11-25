---
title: grafana v1.2 Release Notes
description: grafana v1.2 Release Notes — Kubernetes 生产运维知识库
summary: grafana v1.2 Release Notes — Kubernetes 生产运维知识库
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
- grafana v1.2 Release Notes 是什么
- 如何 grafana v1.2 Release Notes
trigger_keywords:
- grafana
- v1.2
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




# grafana v1.2 Release Notes

Source: [v1.2.0](https://github.com/grafana/grafana/releases/tag/v1.2.0)

New features:
#70 Grid Thresholds (warning and error regions or lines in graph)
#72 Added an example of a scripted dashboard and a short [wiki article](https://github.com/torkelo/grafana/wiki/Scripted-dashboards) documenting scripted dashboards.

Fixes:
#81 Grid min/max values are ignored bug
#80 "stacked as percent" graphs should always use "max" value of 100 bug
#73 Left Y format change did not work 
#42 Fixes to grid min/max auto scaling
#69 Fixes to lexer/parser for metrics segments like "10-20". 
#67 Allow decimal input for scale function 
#68 Bug when trying to open dashboard while in edit mode


<!-- risk-assessed -->
