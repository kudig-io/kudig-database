---
title: rook v0.3 Release Notes
description: rook v0.3 Release Notes — Kubernetes 生产运维知识库
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

# rook v0.3 Release Notes

Source: [v0.3.1](https://github.com/rook/rook/releases/tag/v0.3.1)

* Deeper Kubernetes integration with the Rook Operator, including new [Storage Pool Third Party Resource](https://github.com/rook/rook/blob/master/Documentation/pool-tpr.md)
  * New [Cluster TPR settings](https://github.com/rook/rook/blob/master/Documentation/cluster-tpr.md) as well
* Ceph Monitor failover when the monitor is determined unhealthy
* [Monitoring of Rook via Prometheus integration](https://github.com/rook/rook/blob/master/Documentation/k8s-monitoring.md)
* New build option to build with Ceph Kraken or Luminous
* Reliability and general bug fixes