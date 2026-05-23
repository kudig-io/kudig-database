---
title: velero v1.15 Release Notes
description: velero v1.15 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- velero v1.15 Release Notes 是什么
- 如何 velero v1.15 Release Notes
trigger_keywords:
- velero
- v1.15
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- backup-basics
created: "2026-05-23"
---

# velero v1.15 Release Notes

Source: [v1.15.2](https://github.com/vmware-tanzu/velero/releases/tag/v1.15.2)

## v1.15.2

### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.15.2

### Container Image
`velero/velero:v1.15.2`

### Documentation
https://velero.io/docs/v1.15/

### Upgrading
https://velero.io/docs/v1.15/upgrade-to-1.15/

### All Changes
* fix(pkg/repository/maintenance): don't panic when there's no container statuses (#8568, @mcluseau)
* Don't include excluded items in ItemBlocks (#8585, @kaovilai)
* Check the PVB status via podvolume Backupper rather than calling API server to avoid API server issue (#8596, @ywk253100)
