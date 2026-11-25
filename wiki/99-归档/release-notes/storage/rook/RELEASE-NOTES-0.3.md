---
title: rook v0.3 Release Notes
description: rook v0.3 Release Notes — Kubernetes 生产运维知识库
summary: rook v0.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- rook
- ceph
- operator
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
- rook v0.3 Release Notes 是什么
- 如何 rook v0.3 Release Notes
trigger_keywords:
- rook
- v0.3
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




# [[Rook|rook]] v0.3 Release Notes

Source: [v0.3.1](https://github.com/rook/rook/releases/tag/v0.3.1)

* Deeper [[Kubernetes|Kubernetes]] integration with the Rook Operator, including new Storage Pool Third Party Resource](https://github.com/rook/rook/blob/master/Documentation/pool-tpr.md)
  * New [Cluster TPR settings](https://github.com/rook/rook/blob/master/Documentation/cluster-tpr.md) as well
* Ceph Monitor failover when the monitor is determined unhealthy
* Monitoring of Rook via [[Prometheus|Prometheus]] integration](https://github.com/rook/rook/blob/master/Documentation/k8s-monitoring.md)
* New build option to build with Ceph Kraken or Luminous
* Reliability and general bug fixes

<!-- risk-assessed -->
