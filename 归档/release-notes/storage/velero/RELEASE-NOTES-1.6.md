---
title: velero v1.6 Release Notes
description: velero v1.6 Release Notes — Kubernetes 生产运维知识库
summary: velero v1.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rbac
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- velero v1.6 Release Notes 是什么
- 如何 velero v1.6 Release Notes
trigger_keywords:
- velero
- v1.6
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




# velero v1.6 Release Notes

Source: [v1.6.3](https://github.com/vmware-tanzu/velero/releases/tag/v1.6.3)

## v1.6.3
### 2021-08-12

### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.6.3

### Container Image
`velero/velero:v1.6.3`

### Documentation
https://velero.io/docs/v1.6/

### Upgrading
https://velero.io/docs/v1.6/upgrade-to-1.6/

### Highlights

This release introduces changes to provide compatibility with [[Kubernetes|Kubernetes]] v1.22.

The `apiextensions.k8s.io/v1beta1` API version of `CustomResourceDefinition` will no longer be served in Kubernetes v1.22.
Velero will now use the cluster preferred API version for the `CustomResourceDefinitions` that it creates.

If you are using Kubernetes v1.15 or earlier, the `apiextensions.k8s.io/v1beta1` API version will be used.
If you are using Kubernetes v1.22 or later, the `apiextensions.k8s.io/v1` API version will be used.
For clusters between these versions, the cluster preferred API version will be used.

The `rbac.authorization.k8s.io/v1beta1` API version of `ClusterRoleBinding` will no longer be served in Kubernetes v1.22.
Velero will now use the `rbac.authorization.k8s.io/v1` API version for the `ClusterRoleBinding`s that it creates.
This API version was introduced in Kubernetes v1.8.

### All Changes

  * enable e2e tests to choose crd apiVersion (#3941, @sseago)
  * Upgrade Velero ClusterRoleBinding to use v1 API (#3995, @jenting)
  * Install Kubernetes preferred CRDs API version (v1beta1/v1). (#3999, @jenting)
  * Use the cluster preferred CRD API version when polling for Velero CRD readiness. (#4015, @zubron)
  * Add a RestoreItemAction plugin (`velero.io/apiservice`) which skips the restore of any `APIService` which is managed by Kubernetes. These are identified using the `kube-aggregator.kubernetes.io/automanaged` label. (#4028, @zubron)

<!-- risk-assessed -->
