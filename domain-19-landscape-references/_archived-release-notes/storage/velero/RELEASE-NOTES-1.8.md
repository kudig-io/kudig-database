---
title: velero v1.8 Release Notes
description: velero v1.8 Release Notes — Kubernetes 生产运维知识库
summary: velero v1.8 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- velero v1.8 Release Notes 是什么
- 如何 velero v1.8 Release Notes
trigger_keywords:
- velero
- v1.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- backup-basics
---



# velero v1.8 Release Notes

Source: [v1.8.1](https://github.com/vmware-tanzu/velero/releases/tag/v1.8.1)

## v1.8.1
### 2022-03-15

### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.8.1

### Container Image
`velero/velero:v1.8.1`

### Documentation
https://velero.io/docs/v1.8

### Upgrading
https://velero.io/docs/v1.8/upgrade-to-1.8/

### All changes
* Bypass the remap CRD version plugin when v1beta1 CRD is not supported (#4706, @reasonerjt)
* Support regional pv for GKE (#4691, @jxun)
* Bump up golang to 1.17.8 (#4721, @ywk253100)

