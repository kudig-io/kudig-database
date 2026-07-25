---
title: rook v1.15 Release Notes
description: rook v1.15 Release Notes — Kubernetes 生产运维知识库
summary: rook v1.15 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- rook
- ceph
- operator
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
- rook v1.15 Release Notes 是什么
- 如何 rook v1.15 Release Notes
trigger_keywords:
- rook
- v1.15
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




# [[Rook|rook]] v1.15 Release Notes

Source: [v1.15.9](https://github.com/rook/rook/releases/tag/v1.15.9)

# Improvements
Rook v1.15.9 is a patch release limited in scope and focusing on feature additions and bug fixes to the Ceph operator.

- ci: Push rook image to repositories quay.io and ghcr.io (#15274, @subhamkrai)
- security: bump go-jose package from 4.0.4 to 4.0.5 (#15456, @dependabot)
- mds: Correct parameters to mds liveness probe (#15424, @parth-gr)
- core: Query env vars instead of polling the operator settings configmap (#15462, @travisn)
- [[Helm|helm]]: Allow configurable namespace in the cephECBlockPool storage class (#15402, @KarolGongola)
- nfs: Formatting fix for nfs-ganesha config parser (#15393, @BlaineEXE)


<!-- risk-assessed -->
