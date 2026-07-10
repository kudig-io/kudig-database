---
title: longhorn v0.5 Release Notes
description: longhorn v0.5 Release Notes — Kubernetes 生产运维知识库
summary: longhorn v0.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- helm
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Longhorn|longhorn]] v0.5 Release Notes

Source: [v0.5.0](https://github.com/longhorn/longhorn/releases/tag/v0.5.0)

Highlights:
1. Users now can use Disaster Recovery Volume support (#495 ) to recover the volume in another [[Kubernetes|Kubernetes]]es 集群配置最佳实践|Kubernetes cluster]] with defined RTO and RPO. See [here](https://github.com/rancher/longhorn/blob/v0.5.0/docs/dr-volume.md) for details
2. Users now can see Kubernetes workload information and create PV/PVC in Longhorn UI (#461 ) .See [here](https://github.com/rancher/longhorn/blob/v0.5.0/docs/k8s-workload.md) for details
3. Users now can set backup scheduling in the storage class (#362)
4. We now add the Helm chart in the Longhorn repo, in addition to Rancher Apps. (#445 )

See all the issues resolved in v0.5.0 at:
https://github.com/rancher/longhorn/milestone/3?closed=1

The volume engines would need to upgrade to v0.5.0 as well. Please follow the instruction to upgrade engines.

For Rancher v2.2 users with Catalog Apps, please make sure install Longhorn in `longhorn-system` namespace (instead of `longhorn` namespace). There is a recent [bug](https://github.com/rancher/longhorn/issues/487) in Rancher v2.2 affects the fresh installation of Longhorn App. The fix will be in Rancher v2.2.4 release.

<!-- risk-assessed -->
