---
title: longhorn v0.2 Release Notes
description: longhorn v0.2 Release Notes — Kubernetes 生产运维知识库
summary: longhorn v0.2 Release Notes — Kubernetes 生产运维知识库
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
- longhorn v0.2 Release Notes 是什么
- 如何 longhorn v0.2 Release Notes
trigger_keywords:
- longhorn
- v0.2
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




# [[Longhorn|longhorn]] v0.2 Release Notes

Source: [v0.2.0](https://github.com/longhorn/longhorn/releases/tag/v0.2.0)

Supports [[Kubernetes|Kubernetes]].

Longhorn Manager:
image: rancher/longhorn-manager:1ebf5cb
version: https://github.com/rancher/longhorn-engine/tree/8be099b76eb5acdbdcf2a7bcfffd6ce096884041

Longhorn Engine:
image: rancher/longhorn-engine:de88734
version: https://github.com/rancher/longhorn-engine/tree/8be099b76eb5acdbdcf2a7bcfffd6ce096884041

Longhorn UI:
image: rancher/longhorn-ui:4611040
version: https://github.com/rancher/longhorn-ui/tree/46110405960c25722b3e479ca96dd6f1d5e7ab5f

<!-- risk-assessed -->
