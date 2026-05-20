---
title: velero v1.5 Release Notes
description: velero v1.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- controller-manager
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
---

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
