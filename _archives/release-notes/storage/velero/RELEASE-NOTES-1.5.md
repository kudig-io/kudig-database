---
title: velero v1.5 Release Notes
description: velero v1.5 Release Notes — Kubernetes 生产运维知识库
summary: velero v1.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- controller-manager
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- velero v1.5 Release Notes 是什么
- 如何 velero v1.5 Release Notes
trigger_keywords:
- velero
- v1.5
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




# velero v1.5 Release Notes

Source: [v1.5.4](https://github.com/vmware-tanzu/velero/releases/tag/v1.5.4)

## v1.5.4
### 2021-03-31
### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.5.4

### Container Image
`velero/velero:v1.5.4`

### Documentation
https://velero.io/docs/v1.5/

### Upgrading
https://velero.io/docs/v1.5/upgrade-to-1.5/

  * Fixed a bug where restic volumes would not be restored when using a namespace mapping. (#3475, @zubron)
  * Add CAPI Cluster and ClusterResourceSets to default restore priorities so that the capi-controller-manager does not panic on restores. (#3446, @nrb)


<!-- risk-assessed -->
