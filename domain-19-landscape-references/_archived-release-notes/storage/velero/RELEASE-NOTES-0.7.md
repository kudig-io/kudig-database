---
title: velero v0.7 Release Notes
description: velero v0.7 Release Notes — Kubernetes 生产运维知识库
summary: velero v0.7 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- velero v0.7 Release Notes 是什么
- 如何 velero v0.7 Release Notes
trigger_keywords:
- velero
- v0.7
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- backup-basics
---



# velero v0.7 Release Notes

Source: [v0.7.1](https://github.com/vmware-tanzu/velero/releases/tag/v0.7.1)

Bug fixed:
- Install the Ark server in its own namespace, separate from backups/schedules/restores/config. This helps avoid the situation where it's impossible to delete the `heptio-ark` namespace and/or backups in that namespace. (#322 #323, @ncdc @Bradamant3)

Binary checksums:

```
09cdd26b71ddc3474992dd95f77df984d3de21415c4b9f313a32117c94e78aee  ark-v0.7.1-darwin-amd64.tar.gz
0b13a7b50b4ec263f4dbce7d631192166f329730c6edfe4fba0ed6a53a0793ec  ark-v0.7.1-linux-amd64.tar.gz
532ee9e6b94190e7248511997fcd89b459526cc7caa89ad32dc23bec574fca57  ark-v0.7.1-linux-arm64.tar.gz
18e262668093953249c7d492b254ef00afcbe9f0f1ac05ec5ca40ed137a02b79  ark-v0.7.1-linux-arm.tar.gz
f35d836da06d00cd3598dc07bf4388a05ff9bcf2561c1fcaf56aa024f4865429  ark-v0.7.1-windows-amd64.tar.gz
da423b746a1c45c461cf59d4da31a766ca8c6661dd032238707d5422113a6f9d  CHECKSUM
```