---
title: longhorn v0.4 Release Notes
description: longhorn v0.4 Release Notes — Kubernetes 生产运维知识库
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
- longhorn v0.4 Release Notes 是什么
- 如何 longhorn v0.4 Release Notes
trigger_keywords:
- longhorn
- v0.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# longhorn v0.4 Release Notes

Source: [v0.4.1](https://github.com/longhorn/longhorn/releases/tag/v0.4.1)

Longhorn v0.4.1 has added support for RancherOS and K3S.

Highlights:
1. Support for RancherOS and K3S. See https://github.com/rancher/longhorn/blob/v0.4.1/docs/rancheros.md
2. Support for restoring a backup into an image file. See https://github.com/rancher/longhorn/blob/v0.4.1/docs/restore-to-file.md
3. Improve logging mechanism across the board.

See all the issues resolved in v0.4.1 at https://github.com/rancher/longhorn/milestone/10?closed=1

The volume engines would need to upgrade to v0.4.1 as well. Please follow the instruction to upgrade the engines.