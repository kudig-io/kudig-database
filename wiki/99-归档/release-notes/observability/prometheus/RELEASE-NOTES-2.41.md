---
title: prometheus v2.41 Release Notes
description: prometheus v2.41 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v2.41 Release Notes — Kubernetes 生产运维知识库
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
- prometheus v2.41 Release Notes 是什么
- 如何 prometheus v2.41 Release Notes
trigger_keywords:
- prometheus
- v2.41
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




# [[Prometheus|prometheus]] v2.41 Release Notes

Source: [v2.41.0](https://github.com/prometheus/prometheus/releases/tag/v2.41.0)

* [FEATURE] Relabeling: Add `keepequal` and `dropequal` relabel actions. #11564
* [FEATURE] Add support for HTTP proxy headers. #11712
* [ENHANCEMENT] Reload private certificates when changed on disk. #11685
* [ENHANCEMENT] Add `max_version` to specify maximum TLS version in `tls_config`. #11685
* [ENHANCEMENT] Add `goos` and `goarch` labels to `prometheus_build_info`. #11685
* [ENHANCEMENT] SD: Add proxy support for EC2 and LightSail SDs #11611
* [ENHANCEMENT] SD: Add new metric `prometheus_sd_file_watcher_errors_total`. #11066
* [ENHANCEMENT] Remote Read: Use a pool to speed up marshalling. #11357
* [ENHANCEMENT] TSDB: Improve handling of tombstoned chunks in iterators. #11632
* [ENHANCEMENT] TSDB: Optimize postings offset table reading. #11535
* [BUGFIX] Scrape: Validate the metric name, label names, and label values after relabeling. #11074
* [BUGFIX] Remote Write receiver and rule manager: Fix error handling. #11727


<!-- risk-assessed -->
