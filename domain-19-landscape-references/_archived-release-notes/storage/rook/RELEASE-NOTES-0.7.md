---
title: rook v0.7 Release Notes
description: rook v0.7 Release Notes — Kubernetes 生产运维知识库
summary: rook v0.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- rook
- ceph
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v0.7 Release Notes 是什么
- 如何 rook v0.7 Release Notes
trigger_keywords:
- rook
- v0.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Rook|rook]] v0.7 Release Notes

Source: [v0.7.1](https://github.com/rook/rook/releases/tag/v0.7.1)

Rook v0.7.1 is a patch release limited in scope and focusing on bug fixes.

### Improvements
* The version of Ceph has been updated to [Luminous 12.2.4](http://docs.ceph.com/docs/master/release-notes/#v12-2-4-luminous) (@bassam)
* When a Ceph monitor is failed over, it will be assigned an appropriate IP address when host networking is being used (@galexrt)
* The [upgrade user guide](https://rook.io/docs/rook/v0.7/upgrade.html) has been updated to include steps for upgrading from v0.6.x to the v0.7 releases (@travisn)
* An issue was fixed that prevented the [[Helm|Helm]] charts from being correctly published to https://charts.rook.io/ (@bassam)
* In environments where the Kubernetes cluster does not have a version set, the Helm charts will now appropriately proceed (@TimJones)

<!-- risk-assessed -->
