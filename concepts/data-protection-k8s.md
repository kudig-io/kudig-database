---
title: Kubernetes 数据保护策略
description: → 配置备份 (GitOps)
category: synthesis
tags:
- data-protection
- backup
- disaster-recovery
- k8s
- velero
- csi
- etcd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 数据保护策略 是什么
- 如何 Kubernetes 数据保护策略
trigger_keywords:
- Kubernetes
- 数据保护策略
prerequisites:
- kubectl-basics
- etcd-basics
- backup-basics
created: "2026-05-23"
relationships:
  - target: "[[entities/kubernetes.md]]"
    type: uses
  - target: "[[domain-17-system-foundation/topic-cheat-sheet/k8s.md]]"
    type: related_to
  - target: "[[skills/best-practices/best-practices/infrastructure/storage.md]]"
    type: related_to
---

# [[entities/kubernetes.md|Kubernetes]] 数据保护策略

## 分层保护

```
应用层:
  → 配置备份 (GitOps)
  → Secret 加密备份

data 层:
  → CSI 快照
  → 卷备份 (Velero)
  → 数据库逻辑备份

集群层:
  → etcd 备份
  → 集群状态导出
```

## Velero + CSI 快照

```bash
# 备份命名空间
velero backup create prod-backup \
  --include-namespaces production \
  --snapshot-volumes \
  --volume-snapshot-locations aws

# 灾难恢复
velero restore create --from-backup prod-backup
```

## 3-2-1 原则在 [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|K8s]] 中的实践

```
3 份数据:
  - 生产数据
  - 本地备份
  - 异地备份

2 种介质:
  - 块存储快照
  - 对象存储备份

1 份异地:
  - 跨区域对象存储
```

## 相关 Domain

- domain-09-reliability-engineering/01-backup-recovery/01-backup-strategies
- domain-04-[[skills/best-practices/best-practices/infrastructure/storage.md|storage]]-data/03-csi/01-csi-snapshot
## Related

- [[domain-01-cluster-fundamentals/01-architecture-overview/01-kubernetes-architecture-overview.md|Kubernetes 架构全景图 (Architecture Overview)]]
- [[domain-19-landscape-references/02-papers/01-kubernetes-production-readiness-assessment.md|Kubernetes 生产就绪性评估框架 (Production Readiness Assessment Framework)]]
