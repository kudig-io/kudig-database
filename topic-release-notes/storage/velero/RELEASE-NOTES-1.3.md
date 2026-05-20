---
title: velero v1.3 Release Notes
description: velero v1.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- velero v1.3 Release Notes 是什么
- 如何 velero v1.3 Release Notes
trigger_keywords:
- velero
- v1.3
- Release
- Notes
- release
- notes
---

# velero v1.3 Release Notes

Source: [v1.3.2](https://github.com/vmware-tanzu/velero/releases/tag/v1.3.2)

### Container Image
`velero/velero:v1.3.2`

### Documentation
https://velero.io/docs/v1.3.2/

### Upgrading
https://velero.io/docs/v1.3.2/upgrade-to-1.3/

### All Changes
* Allow `plugins/` as a valid top-level directory within backup storage locations. This directory is a place for plugin authors to store arbitrary data as needed. It is recommended to create an additional subdirectory under `plugins/` specifically for your plugin, e.g. `plugins/my-plugin-data/`. (#2350, @skriss)
* bug fix: don't panic in `velero restic repo get` when last maintenance time is `nil` (#2315, @skriss)