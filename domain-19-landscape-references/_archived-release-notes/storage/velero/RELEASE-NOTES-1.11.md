---
title: velero v1.11 Release Notes
description: velero v1.11 Release Notes — Kubernetes 生产运维知识库
summary: velero v1.11 Release Notes — Kubernetes 生产运维知识库
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
- velero v1.11 Release Notes 是什么
- 如何 velero v1.11 Release Notes
trigger_keywords:
- velero
- v1.11
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- backup-basics
---



# velero v1.11 Release Notes

Source: [v1.11.1](https://github.com/vmware-tanzu/velero/releases/tag/v1.11.1)

## v1.11.1
### 2023-07-25

### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.11.1

### Container Image
`velero/velero:v1.11.1`

### Documentation
https://velero.io/docs/v1.11/

### Upgrading
https://velero.io/docs/v1.11/upgrade-to-1.11/

### All changes
  * Add support for OpenStack [[entities/csi-drivers.md|CSI drivers]] topology keys (#6488, @kayrus)
  * Enhance the code because of #6297, the return value of GetBucketRegion is not recorded, as a result, when it fails, we have no way to get the cause (#6477, @Lyndon-Li)
  * Fixed a bug where status.progress is not getting updated for backups. (#6324, @blackpiglet)
  * Restore Endpoints before Services (#6316, @ywk253100)
  * Fix issue #6182. If pod is not running, don't treat it as an error, let it go and leave a warning. (#6189, @Lyndon-Li)

