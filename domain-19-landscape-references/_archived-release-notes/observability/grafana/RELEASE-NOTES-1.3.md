---
title: grafana v1.3 Release Notes
description: grafana v1.3 Release Notes — Kubernetes 生产运维知识库
summary: grafana v1.3 Release Notes — Kubernetes 生产运维知识库
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
- grafana v1.3 Release Notes 是什么
- 如何 grafana v1.3 Release Notes
trigger_keywords:
- grafana
- v1.3
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




# grafana v1.3 Release Notes

Source: [v1.3.0](https://github.com/grafana/grafana/releases/tag/v1.3.0)

New features or improvements: 
#86  Dashboard tags and search (see [wiki article](https://github.com/torkelo/grafana/wiki/Search-features) for details)
#54 Enhancement to filter / template. "Include All" improvement 
#82 Dashboard search result sorted in alphabetical order 

Fixes:
#91 Custom date selector is one day behind
#89 Filter / template does not work after switching dashboard
#88 Closed / Minimized row css bug 
#85 Added all parameters to summarize function
#83 Stack as percent should now work a lot better!


<!-- risk-assessed -->
