---
title: longhorn v0.5 Release Notes
description: longhorn v0.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- longhorn v0.5 Release Notes 是什么
- 如何 longhorn v0.5 Release Notes
trigger_keywords:
- longhorn
- v0.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
---

# longhorn v0.5 Release Notes

Source: [v0.5.0](https://github.com/longhorn/longhorn/releases/tag/v0.5.0)

Highlights:
1. Users now can use Disaster Recovery Volume support (#495 ) to recover the volume in another Kubernetes cluster with defined RTO and RPO. See [here](https://github.com/rancher/longhorn/blob/v0.5.0/docs/dr-volume.md) for details
2. Users now can see Kubernetes workload information and create PV/PVC in Longhorn UI (#461 ) .See [here](https://github.com/rancher/longhorn/blob/v0.5.0/docs/k8s-workload.md) for details
3. Users now can set backup scheduling in the storage class (#362)
4. We now add the Helm chart in the Longhorn repo, in addition to Rancher Apps. (#445 )

See all the issues resolved in v0.5.0 at:
https://github.com/rancher/longhorn/milestone/3?closed=1

The volume engines would need to upgrade to v0.5.0 as well. Please follow the instruction to upgrade engines.

For Rancher v2.2 users with Catalog Apps, please make sure install Longhorn in `longhorn-system` namespace (instead of `longhorn` namespace). There is a recent [bug](https://github.com/rancher/longhorn/issues/487) in Rancher v2.2 affects the fresh installation of Longhorn App. The fix will be in Rancher v2.2.4 release.