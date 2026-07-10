---
title: Kubernetes v0.11 Release Notes
description: Kubernetes v0.11 Release Notes — Kubernetes 生产运维知识库
summary: Kubernetes v0.11 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes v0.11 Release Notes 是什么
- 如何 Kubernetes v0.11 Release Notes
trigger_keywords:
- Kubernetes
- v0.11
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




# [[Kubernetes|Kubernetes]] v0.11 Release Notes

Source: GitHub Release [v0.11.0](https://github.com/kubernetes/kubernetes/releases/tag/v0.11.0)

### Changes since 0.10.0
- Secret API Resources
- Better error handling in various places
- Improved RackSpace support
- Fix `kubectl` patch behavior
- Health check failures fire events
- Don't delete the pod infrastructure container on health check failures
- Improvements to Pod Status detection and reporting
- Reduce the size of scheduled [[Pods|pods]] in [[etcd|etcd]]
- Fix some bugs in namespace clashing
- More detailed info on failed image pulls
- Remove pods from a failed node
- Safe format and mount of GCE PDs
- Make events more resilient to etcd watch failures
- Upgrade to container-vm 01-29-2015
  
  | binary | hash alg | hash |
  | --- | --- | --- |
  | `kubernetes.tar.gz` | md5 | `b7e67a4a4b09ce120379f83b8193ac3f` |
  | `kubernetes.tar.gz` | sha1 | `aa884b8200681d3bb8ca0f12398c7424942be500` |


<!-- risk-assessed -->
