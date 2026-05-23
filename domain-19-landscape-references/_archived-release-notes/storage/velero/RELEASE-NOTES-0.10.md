---
title: velero v0.10 Release Notes
description: velero v0.10 Release Notes — Kubernetes 生产运维知识库
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
- velero v0.10 Release Notes 是什么
- 如何 velero v0.10 Release Notes
trigger_keywords:
- velero
- v0.10
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

# velero v0.10 Release Notes

Source: [v0.10.2](https://github.com/vmware-tanzu/velero/releases/tag/v0.10.2)

### Changes
  * upgrade restic to v0.9.4 & replace --hostname flag with --host (#1156, @skriss)
  * use 'restic stats' instead of 'restic check' to determine if repo exists (#1171, @skriss)
  * Fix concurrency bug in code ensuring restic repository exists (#1235, @skriss)