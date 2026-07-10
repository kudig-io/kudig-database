---
title: kops v1.14 Release Notes
description: kops v1.14 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.14 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- coredns
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.14 Release Notes 是什么
- 如何 kops v1.14 Release Notes
trigger_keywords:
- kops
- v1.14
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kops v1.14 Release Notes

Source: [1.14.1](https://github.com/kubernetes/kops/releases/tag/1.14.1)

Release in 1.14 series of kops, supporting [[Kubernetes|kubernetes]] 1.14 and earlier.

Please see the [release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.14-NOTES.md) for the full list of changes. 

For existing clusters, please update to kubernetes 1.12 before updating to kubernetes 1.13 and 1.14.  Technically kubernetes upgrades can only be done one minor version at a time, but this is particularly important because of the etcd-upgrade that is in kops 1.12.


## Significant Changes

* This release fixes a bug where [[CoreDNS|coredns]] updates would no longer be applied.

## 1.14.0 to 1.14.1

* fix(upup/models/cloudup/resources/addons/coredns.addons.k8s.io) missing resourceVersion [@phspagiari](https://github.com/phspagiari) [#7477](https://github.com/kubernetes/kops/pull/7477)



<!-- risk-assessed -->
