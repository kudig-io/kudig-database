---
title: velero v1.10 Release Notes
description: velero v1.10 Release Notes — Kubernetes 生产运维知识库
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
- velero v1.10 Release Notes 是什么
- 如何 velero v1.10 Release Notes
trigger_keywords:
- velero
- v1.10
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

# velero v1.10 Release Notes

Source: [v1.10.3](https://github.com/vmware-tanzu/velero/releases/tag/v1.10.3)

## v1.10.3
### 2023-05-08

### Download
https://github.com/vmware-tanzu/velero/releases/tag/v1.10.3

### Container Image
`velero/velero:v1.10.3`

### Documentation
https://velero.io/docs/v1.10/

### Upgrading
https://velero.io/docs/v1.10/upgrade-to-1.10/

### All changes
  * Fix issue #6182. If pod is not running, don't treat it as an error, let it go and leave a warning. (#6188, @Lyndon-Li)
  * Bump v1.10's Golang version to v1.20 (#6143, @blackpiglet)
  * Ignore not found error during patching managedFields (#6135, @ywk253100)
  * Fix issue #5972, don't assume errorField as error type when dealing with logger.WithError (#6086, @Lyndon-Li)
  * Restore Services before Clusters (#6058, @ywk253100)
