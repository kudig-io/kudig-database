---
title: rook v0.8 Release Notes
description: rook v0.8 Release Notes — Kubernetes 生产运维知识库
summary: rook v0.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rook
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v0.8 Release Notes 是什么
- 如何 rook v0.8 Release Notes
trigger_keywords:
- rook
- v0.8
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




# [[Rook|rook]] v0.8 Release Notes

Source: [v0.8.3](https://github.com/rook/rook/releases/tag/v0.8.3)

Rook v0.8.3 is a patch release limited in scope and focusing on bug fixes.

## Improvements
- OSD can now be configured in K8s clusters where the [hostname label is different from the node name](https://github.com/rook/rook/issues/2148)  (@travisn)
- Fix regression in v0.8.2 that caused [PVCs to fail](https://github.com/rook/rook/issues/2149) to be mounted in some clusters due to unexpected logging (@rootfs)

<!-- risk-assessed -->
