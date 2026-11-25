---
sources:
- "平台工程/运维/14-cluster-api-deep-dive.md"
title: Cluster API (CAPI) 深度解析
summary: 解析 Kubernetes 原生集群生命周期管理项目 Cluster API 的 CRD 模型、Provider 协作与生产实践。
category: concepts
tags:
- cluster-api
- capi
- lifecycle
- multi-cluster
- provider
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 平台架构师
- SRE
estimated_read_time: 25min
intent_queries:
- Cluster API 是什么
- CAPI 如何管理集群生命周期
- Cluster API 与 vcluster 区别
- MachineDeployment 如何工作
trigger_keywords:
- Cluster API
- CAPI
- 集群生命周期
- Machine
- Provider
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **内容已并入主场**：[[wiki/12-平台工程/02-运维/11-cluster-api-deep-dive.md|Cluster API (CAPI) 深度解析]]（本页为重排去重桩，正文见主场）
