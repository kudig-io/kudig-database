---
title: grafana v2.1 Release Notes
description: grafana v2.1 Release Notes — Kubernetes 生产运维知识库
summary: grafana v2.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v2.1 Release Notes 是什么
- 如何 grafana v2.1 Release Notes
trigger_keywords:
- grafana
- v2.1
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




# grafana v2.1 Release Notes

Source: [v2.1.2](https://github.com/grafana/grafana/releases/tag/v2.1.2)

Patch release for latest major release (2.1) 
- [Download](http://grafana.org/download/) 
- [What's New in Grafana 2.1](http://docs.grafana.org/v2.1/guides/whats-new-in-v2-1)
- [Installation guide](http://docs.grafana.org/v2.1/installation/)
- [Migrating from 1.x to 2.x](http://docs.grafana.org/v2.1/installation/migrating_to2/)
- Changelog](https://github.com/grafana/grafana/blob/master/CHANGELOG.md)

**Fixes since 2.1.1**
- [Issue #2558](https://github.com/grafana/grafana/issues/2558). DragDrop: Fix for broken drag drop behavior (introduced in v2.1.1)
- [Issue #2534](https://github.com/grafana/grafana/issues/2534). Templating: fix for setting template variable value via url and having repeated panels or rows


<!-- risk-assessed -->
