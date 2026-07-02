---
title: prometheus v2.28 Release Notes
description: prometheus v2.28 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v2.28 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v2.28 Release Notes 是什么
- 如何 prometheus v2.28 Release Notes
trigger_keywords:
- prometheus
- v2.28
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




# [[Prometheus|prometheus]] v2.28 Release Notes

Source: [v2.28.1](https://github.com/prometheus/prometheus/releases/tag/v2.28.1)

* [BUGFIX]: HTTP SD: Allow `charset` specification in `Content-Type` header. #8981
* [BUGFIX]: HTTP SD: Fix handling of disappeared target groups. #9019
* [BUGFIX]: Fix incorrect log-level handling after moving to go-kit/log. #9021


<!-- risk-assessed -->
