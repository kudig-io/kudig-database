---
title: kops v1.8 Release Notes
description: kops v1.8 Release Notes — Kubernetes 生产运维知识库
summary: kops v1.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- flannel
- calico
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kops v1.8 Release Notes 是什么
- 如何 kops v1.8 Release Notes
trigger_keywords:
- kops
- v1.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kops v1.8 Release Notes

Source: [1.8.1](https://github.com/kubernetes/kops/releases/tag/1.8.1)

Release 1.8.1 is a small patch release, which updates network plugins, but also tolerates a new schema
file that will be added in kops 1.9.0.  This will provide a downgrade option from kops 1.9.0.

* Ignore keyset.yaml files; provides a downgrade option from (upcoming) kops 1.9.0
* Update flannel, weave, romana, kopeio-networking, calico, canal
* Stop passing deprecated require-kubeconfig flag for [[Kubernetes|kubernetes]] >= 1.9

<!-- risk-assessed -->
