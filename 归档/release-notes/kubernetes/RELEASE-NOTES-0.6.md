---
title: Kubernetes v0.6 Release Notes
description: Kubernetes v0.6 Release Notes — Kubernetes 生产运维知识库
summary: Kubernetes v0.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- Kubernetes v0.6 Release Notes 是什么
- 如何 Kubernetes v0.6 Release Notes
trigger_keywords:
- Kubernetes
- v0.6
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] v0.6 Release Notes

Source: GitHub Release [v0.6.2](https://github.com/kubernetes/kubernetes/releases/tag/v0.6.2)

### Changes since v0.6.1
- Upgrade to Docker 1.3.3 for debian/salt based cluster [[Deployments|deployments]].

| binary | hash alg | hash |
| --- | --- | --- |
| `kubernetes.tar.gz` | md5 | `8c103659e9c1625633aa1ac186f71105` |
| `kubernetes.tar.gz` | sha1 | `0962eb595396808990c2c64182948e29a27f3491` |


<!-- risk-assessed -->
