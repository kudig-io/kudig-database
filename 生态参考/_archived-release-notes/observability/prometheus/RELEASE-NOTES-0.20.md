---
title: prometheus v0.20 Release Notes
description: prometheus v0.20 Release Notes — Kubernetes 生产运维知识库
summary: prometheus v0.20 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- kubelet
- prometheus
- docker
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
- prometheus v0.20 Release Notes 是什么
- 如何 prometheus v0.20 Release Notes
trigger_keywords:
- prometheus
- v0.20
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




# [[Prometheus|prometheus]] v0.20 Release Notes

Source: [0.20.0](https://github.com/prometheus/prometheus/releases/tag/0.20.0)

This release contains multiple breaking changes to the configuration schema.
- [FEATURE] Allow configuring multiple Alertmanagers
- [FEATURE] Add server name to TLS configuration
- [FEATURE] Add labels for all node addresses and discover node port if available in [[Kubernetes|Kubernetes]] SD
- [ENHANCEMENT] More meaningful configuration errors
- [ENHANCEMENT] Round scraping timestamps to milliseconds in web UI
- [ENHANCEMENT] Make number of storage fingerprint locks configurable
- [BUGFIX] Fix date parsing in console template graphs
- [BUGFIX] Fix static console files in Docker images
- [BUGFIX] Fix console JS XHR requests for IE11
- [BUGFIX] Add missing path prefix in new status page
- [CHANGE] Rename `target_groups` to `static_configs` in config files
- [CHANGE] Rename `names` to `files` in file SD configuration
- [CHANGE] Remove kubelet port config option in Kubernetes SD configuration


<!-- risk-assessed -->
