---
title: velero v0.4 Release Notes
description: velero v0.4 Release Notes — Kubernetes 生产运维知识库
summary: velero v0.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- velero v0.4 Release Notes 是什么
- 如何 velero v0.4 Release Notes
trigger_keywords:
- velero
- v0.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- backup-basics
---



# velero v0.4 Release Notes

Source: [v0.4.0](https://github.com/vmware-tanzu/velero/releases/tag/v0.4.0)

Breaking changes:
- Snapshot and restore volumes by default (#45)
- `ark restore create`: `--namespaces` replaced by `--include-namespaces` and `--exclude-namespaces` (#59)

New features:
- Add support for S3 SSE with KMS (#29)
- Validate cloud provider configurations & make persistentVolumeProvider optional (#35)
- Add [[domain-17-system-foundation/topic-dictionary/fundamentals/garbage-collection.md|garbage collection]] of Restore objects (#63)
- Save logs per backup (#40)
- Save logs per restore (#79)
- Add `--include-resources/--exclude-resources` for restores (#78)

Bug fixes:
- Only save/use iops for io1 volumes on AWS (#37)
- When restoring, try to retrieve the Backup directly from object storage if it's not found (#57)
- When syncing Backups from object storage to [[Kubernetes|Kubernetes]], don't return at the first error encountered (#66)
- More closely match how `kubectl` performs kubeconfig resolution (#62)
- Increase default Azure API request timeout to 2 minutes (#90)
- Update Azure diskURI to match diskName (#89)

Thanks again to @jrnt30 for contributing to this release!