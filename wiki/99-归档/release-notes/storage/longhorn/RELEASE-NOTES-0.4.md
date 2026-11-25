---
title: longhorn v0.4 Release Notes
description: longhorn v0.4 Release Notes — Kubernetes 生产运维知识库
summary: longhorn v0.4 Release Notes — Kubernetes 生产运维知识库
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
- longhorn v0.4 Release Notes 是什么
- 如何 longhorn v0.4 Release Notes
trigger_keywords:
- longhorn
- v0.4
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




# [[Longhorn|longhorn]] v0.4 Release Notes

Source: [v0.4.1](https://github.com/longhorn/longhorn/releases/tag/v0.4.1)

Longhorn v0.4.1 has added support for RancherOS and [[k3s|K3S]].

Highlights:
1. Support for RancherOS and K3S. See https://github.com/rancher/longhorn/blob/v0.4.1/docs/rancheros.md
2. Support for restoring a backup into an image file. See https://github.com/rancher/longhorn/blob/v0.4.1/docs/restore-to-file.md
3. Improve logging mechanism across the board.

See all the issues resolved in v0.4.1 at https://github.com/rancher/longhorn/milestone/10?closed=1

The volume engines would need to upgrade to v0.4.1 as well. Please follow the instruction to upgrade the engines.

<!-- risk-assessed -->
