---
title: longhorn v0.6 Release Notes
description: longhorn v0.6 Release Notes — Kubernetes 生产运维知识库
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
- longhorn v0.6 Release Notes 是什么
- 如何 longhorn v0.6 Release Notes
trigger_keywords:
- longhorn
- v0.6
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# longhorn v0.6 Release Notes

Source: [v0.6.2](https://github.com/longhorn/longhorn/releases/tag/v0.6.2)

Longhorn v0.6.2 is an alpha-quality release.

This release is for addressing [the high CPU utilization issue](https://github.com/longhorn/longhorn/issues/767), as well as some other issues we found since v0.6.1 release.

See the list of issues resolved [here](https://github.com/longhorn/longhorn/milestone/13?closed=1).

Please follow [the offline upgrade steps](https://github.com/longhorn/longhorn/blob/master/docs/upgrade.md#offline-upgrade) to upgrade engine image to v0.6.2 to fix the high CPU utilization issue.