---
title: flux v0.27 Release Notes
description: flux v0.27 Release Notes — Kubernetes 生产运维知识库
summary: flux v0.27 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- flux
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
- flux v0.27 Release Notes 是什么
- 如何 flux v0.27 Release Notes
trigger_keywords:
- flux
- v0.27
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




# [[Flux|flux]] v0.27 Release Notes

Source: [v0.27.4](https://github.com/fluxcd/flux2/releases/tag/v0.27.4)

Flux v0.27.4 is a patch release that comes with patches to the Deployment manifest of helm-controller and the-notification controller, to set the `.spec.securityContext.fsGroup`, which may be required for some EKS setups as reported in https://github.com/fluxcd/flux2/issues/2537. Users are encouraged to upgrade for the best experience.

## Components changelog

- helm-controller [v0.17.2](https://github.com/fluxcd/helm-controller/blob/v0.17.2/CHANGELOG.md)
- notification-controller [v0.22.3](https://github.com/fluxcd/notification-controller/blob/v0.22.3/CHANGELOG.md)


<!-- risk-assessed -->
