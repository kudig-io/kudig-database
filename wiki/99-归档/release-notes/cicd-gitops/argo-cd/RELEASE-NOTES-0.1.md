---
title: argo-cd v0.1 Release Notes
description: argo-cd v0.1 Release Notes — Kubernetes 生产运维知识库
summary: argo-cd v0.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- argo-cd v0.1 Release Notes 是什么
- 如何 argo-cd v0.1 Release Notes
trigger_keywords:
- argo-cd
- v0.1
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




# argo-cd v0.1 Release Notes

Source: [v0.1.0](https://github.com/argoproj/argo-cd/releases/tag/v0.1.0)

- Define app in Github with dev and preprod environment using KSonnet
- Add cluster Diff App with a cluster Deploy app in a cluster
-  Deploy a new version of the app in the cluster
- App sync based on Github app config change - polling only
- Basic UI: App diff between Git and k8s cluster for all environments Basic GUI

<!-- risk-assessed -->
