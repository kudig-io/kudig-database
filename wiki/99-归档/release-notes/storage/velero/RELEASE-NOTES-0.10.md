---
title: velero v0.10 Release Notes
description: velero v0.10 Release Notes — Kubernetes 生产运维知识库
summary: velero v0.10 Release Notes — Kubernetes 生产运维知识库
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
- velero v0.10 Release Notes 是什么
- 如何 velero v0.10 Release Notes
trigger_keywords:
- velero
- v0.10
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# velero v0.10 Release Notes

Source: [v0.10.2](https://github.com/vmware-tanzu/velero/releases/tag/v0.10.2)

### Changes
  * upgrade restic to v0.9.4 & replace --hostname flag with --host (#1156, @skriss)
  * use 'restic stats' instead of 'restic check' to determine if repo exists (#1171, @skriss)
  * Fix concurrency bug in code ensuring restic repository exists (#1235, @skriss)

<!-- risk-assessed -->
