---
title: velero v0.8 Release Notes
description: velero v0.8 Release Notes — Kubernetes 生产运维知识库
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
- velero v0.8 Release Notes 是什么
- 如何 velero v0.8 Release Notes
trigger_keywords:
- velero
- v0.8
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- backup-basics
---

# velero v0.8 Release Notes

Source: [v0.8.3](https://github.com/vmware-tanzu/velero/releases/tag/v0.8.3)

##### Bug Fixes:
  * Don't restore backup and restore resources to avoid possible data corruption (#622, @ncdc)

#### Binary checksums:
```
0e00a5d41f1bd4a3e625b2e96844b44e213f6e701604820445f6900a1c12ca89  ark-v0.8.3-darwin-amd64.tar.gz
42d38700ad4c0a7bd9e25183d31707f49e1e5d4d27ad7aa6dd5f8c765138081b  ark-v0.8.3-linux-amd64.tar.gz
dd12ff96784693c5c0f66c31b1838a4325970b246589dd61812a9b359dd96f13  ark-v0.8.3-linux-arm.tar.gz
b4707139f0acabfbbc52b61c05dd1eaaf3c659d3f64c0b871f2d4bd998bc16f4  ark-v0.8.3-linux-arm64.tar.gz
c5031f644411b5ba286540b9c742fe4569b0c629f44935e973e85e363b34e66a  ark-v0.8.3-windows-amd64.tar.gz
7786e4efff3cf51569d1017068fb925d80bbae89fb418fce5c0db8d153d25a8e  CHECKSUM
```