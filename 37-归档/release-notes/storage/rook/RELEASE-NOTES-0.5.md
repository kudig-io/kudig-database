---
title: rook v0.5 Release Notes
description: rook v0.5 Release Notes — Kubernetes 生产运维知识库
summary: rook v0.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- helm
- rook
- ceph
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v0.5 Release Notes 是什么
- 如何 rook v0.5 Release Notes
trigger_keywords:
- rook
- v0.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Rook|rook]] v0.5 Release Notes

Source: [v0.5.1](https://github.com/rook/rook/releases/tag/v0.5.1)

Rook v0.5.1 is a patch release limited in scope and focusing on bug fixes and build improvements.

### Improvements
* Ceph Luminous has been upgraded to [12.1.3](http://ceph.com/releases/v12-1-3-luminous-rc-released/)
* [[Helm|Helm]] charts are now built and published as part of the continuous integration pipeline.  Details can be found in the [Helm Chart readme](https://rook.io/docs/rook/v0.5/helm-operator.html)
* Improve initial monitor quorum performance so a Rook cluster can be bootstrapped more quickly
* Rook's metrics and monitoring via [[Prometheus|Prometheus]] is now fully compatible with Ceph Luminous
* Allow [placement policy](https://rook.io/docs/rook/v0.5/cluster-crd.html#placement-configuration-settings) to be applied to manager pods

<!-- risk-assessed -->
