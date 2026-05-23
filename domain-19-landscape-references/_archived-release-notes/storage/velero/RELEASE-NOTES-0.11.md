---
title: velero v0.11 Release Notes
description: velero v0.11 Release Notes — Kubernetes 生产运维知识库
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
- velero v0.11 Release Notes 是什么
- 如何 velero v0.11 Release Notes
trigger_keywords:
- velero
- v0.11
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

# velero v0.11 Release Notes

Source: [v0.11.1](https://github.com/vmware-tanzu/velero/releases/tag/v0.11.1)

## v0.11.1
#### 2019-05-17

### Download
- https://github.com/heptio/velero/releases/tag/v0.11.1

### Highlights
* Added the `velero migrate-backups` command to migrate legacy Ark backup metadata to the current Velero format in object storage. This command needs to be run in preparation for upgrading to v1.0, **if** you have backups that were originally created prior to v0.11 (i.e. when the project was named Ark).