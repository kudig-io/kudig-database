---
title: velero v1.9 Release Notes
description: velero v1.9 Release Notes — Kubernetes 生产运维知识库
summary: velero v1.9 Release Notes — Kubernetes 生产运维知识库
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
- velero v1.9 Release Notes 是什么
- 如何 velero v1.9 Release Notes
trigger_keywords:
- velero
- v1.9
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




# velero v1.9 Release Notes

Source: [v1.9.7](https://github.com/vmware-tanzu/velero/releases/tag/v1.9.7)

## v1.9.7
### 2023-04-17

### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.9.7

### Container Image
`velero/velero:v1.9.7`

### Documentation
https://velero.io/docs/v1.9/

### Upgrading
https://velero.io/docs/v1.9/upgrade-to-1.9/

### All changes
  * Bump Golang version to v1.19.8 (#6148, @blackpiglet)

<!-- risk-assessed -->
