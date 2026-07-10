---
title: prometheus v2.54 Release Notes
description: prometheus v2.54 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v2.54 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- prometheus v2.54 Release Notes 是什么
- 如何 prometheus v2.54 Release Notes
trigger_keywords:
- prometheus
- v2.54
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




# [[Prometheus|prometheus]] v2.54 Release Notes

Source: [v2.54.1](https://github.com/prometheus/prometheus/releases/tag/v2.54.1)

* [BUGFIX] Scraping: allow multiple samples on same series, with explicit timestamps. #14685
* [BUGFIX] Docker SD: fix crash in `match_first_network` mode when container is reconnected to a new network. #14654
* [BUGFIX] PromQL: fix experimental native histogram counter reset detection on stale samples. #14514
* [BUGFIX] PromQL: fix experimental native histograms getting corrupted due to vector selector bug in range queries. #14538
* [BUGFIX] PromQL: fix experimental native histogram memory corruption when using histogram_count or histogram_sum. #14605

**Full Changelog**: https://github.com/prometheus/prometheus/compare/v2.54.0...v2.54.1

<!-- risk-assessed -->
