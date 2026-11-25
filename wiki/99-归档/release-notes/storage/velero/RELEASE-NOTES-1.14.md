---
title: velero v1.14 Release Notes
description: velero v1.14 Release Notes — Kubernetes 生产运维知识库
summary: velero v1.14 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- vpa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- velero v1.14 Release Notes 是什么
- 如何 velero v1.14 Release Notes
trigger_keywords:
- velero
- v1.14
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




# velero v1.14 Release Notes

Source: [v1.14.1](https://github.com/vmware-tanzu/velero/releases/tag/v1.14.1)

## v1.14.1

### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.14.1

### Container Image
`velero/velero:v1.14.1`

### Documentation
https://velero.io/docs/v1.14/

### Upgrading
https://velero.io/docs/v1.14/upgrade-to-1.14/

### All Changes
  * Avoid wrapping failed PVB status with empty message. (#8037, @mrnold)
  * Make PVPatchMaximumDuration timeout configurable (#8035, @shubham-pampattiwar)
  * Reuse existing plugin manager for get/put volume info (#8016, @sseago)
  * Skip PV patch step in Restoe workflow for WaitForFirstConsumer VolumeBindingMode Pending state PVCs (#8006, @shubham-pampattiwar)
  * Check whether the namespaces specified in namespace filter exist. (#7998, @blackpiglet)
  * Check whether the volume's source is PVC before fetching its PV. (#7976, @blackpiglet)
  * Fix issue #7904, add the limitation clarification for change PVC selected-node feature (#7949, @Lyndon-Li)
  * Expose the VolumeHelper to third-party plugins. (#7944, @blackpiglet)
  * Don't consider unschedulable [[Pods|pods]] unrecoverable (#7926, @sseago)

<!-- risk-assessed -->
