---
title: rook v1.8 Release Notes
description: rook v1.8 Release Notes — Kubernetes 生产运维知识库
summary: rook v1.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rook
- ceph
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- rook v1.8 Release Notes 是什么
- 如何 rook v1.8 Release Notes
trigger_keywords:
- rook
- v1.8
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




# [[Rook|rook]] v1.8 Release Notes

Source: [v1.8.10](https://github.com/rook/rook/releases/tag/v1.8.10)

# Improvements
Rook v1.8.10 is a patch release limited in scope and focusing on small feature additions and bug fixes to the Ceph operator.

- core: Improve detection of filesystem properties for disk in use (#10230, @leseb)
- osd: Remove broken argument for upgraded OSDs on PVCs in legacy lvm mode (#10298, @leseb)
- osd: Allow the osd to take two hours to start in case of ceph maintenance (#10250, @travisn)
- operator: Report telemetry 'rook/version' in mon store (#10161, @BlaineEXE)



<!-- risk-assessed -->
