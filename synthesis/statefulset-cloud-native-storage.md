---
title: StatefulSet × 云原生存储
category: synthesis
tags:
- statefulset
- storage
- csi
- kubernetes
- database
sources:
- domain-04-storage-data/04-stateful-app-storage/01-stateful-app-storage-patterns.md
- domain-04-storage-data/04-stateful-app-storage/02-mysql-statefulset-production.md
- domain-04-storage-data/04-stateful-app-storage/03-postgresql-statefulset-production.md
- domain-04-storage-data/04-stateful-app-storage/04-kafka-statefulset-production.md
- domain-04-storage-data/04-stateful-app-storage/05-redis-cluster-statefulset.md
created: "2026-06-26"
updated: "2026-06-26"
last_updated: 2026-06-26
summary: "StatefulSet 与云原生存储的交叉合成：有状态应用在 K8s 上的持久化、扩容与灾备模式。"
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.75
lifecycle: draft
lifecycle_changed: "2026-06-26"
---

# StatefulSet × 云原生存储

## The Connection

StatefulSet 是 Kubernetes 管理有状态应用的核心抽象，但它本身不提供数据持久化。云原生存储（CSI、云盘、NAS、分布式存储）为 StatefulSet 的 Pod 提供独立于生命周期的数据卷。二者的结合决定了有状态工作负载在 K8s 上能否稳定运行。^[inferred]

## Where They Co-occur

- MySQL/PostgreSQL 主从集群通过 StatefulSet + Headless Service 实现稳定网络标识，依赖 PVC 保证数据不随 Pod 重建而丢失
- Kafka、Redis Cluster 利用 StatefulSet 的 ordered rollout 和 PVC 模板实现分片数据的持久化
- 阿里云/专有云场景下，云盘 CSI 为 StatefulSet 提供块存储，NAS/OSS 提供共享文件访问
- Velero 备份 StatefulSet 时，必须同时备份 PVC 和底层 PV 快照才能完整恢复

## Cross-cutting Insight

StatefulSet 解决的是"身份"问题（稳定网络标识、有序部署），云原生存储解决的是"状态"问题（数据持久化、快照、扩容）。只有将两者视为一个整体架构来设计，才能避免"Pod 能重建但数据丢失"或"数据在但服务无法发现"的故障模式。^[inferred]

## Tensions and Trade-offs

| 维度 | StatefulSet 偏好 | 云原生存储偏好 | 权衡 |
|---|---|---|---|
| 扩容 | 稳定标识，扩缩容慢 | 动态 provisioning，快速绑定 | 有状态应用扩容需兼顾数据重平衡 |
| 存储类型 | 本地盘性能高但不灵活 | 网络盘可迁移但延迟高 | 数据库类应用倾向本地 SSD，分析类倾向共享存储 |
| 备份 | 应用层一致性备份复杂 | 存储层快照简单但可能 crash-consistent | 关键数据库通常需要两者结合 |
| 多云 | 行为一致 | 各云厂商 CSI 特性差异大 | 跨云迁移需抽象存储类策略 |

## Open Questions

- 在阿里云专有云环境下，StatefulSet 与云盘 CSI 的 AZ 亲和性如何保证容灾？
- 本地盘 StatefulSet 是否适合所有数据库，还是应优先使用网络存储以简化运维？
- 当 StatefulSet Pod 跨节点重建时，如何确保 PVC 的 zone/region 约束不被破坏？

## Related

- [[domain-04-storage-data/04-stateful-app-storage/01-stateful-app-storage-patterns.md|01 stateful app storage patterns]]
- [[domain-04-storage-data/04-stateful-app-storage/02-mysql-statefulset-production.md|02 mysql statefulset production]]
- [[domain-04-storage-data/04-stateful-app-storage/03-postgresql-statefulset-production.md|03 postgresql statefulset production]]
- [[domain-04-storage-data/04-stateful-app-storage/04-kafka-statefulset-production.md|04 kafka statefulset production]]
- [[domain-04-storage-data/04-stateful-app-storage/05-redis-cluster-statefulset.md|05 redis cluster statefulset]]
- [[domain-04-storage-data/03-distributed-storage/01-velero-backup-recovery.md|01 velero backup recovery]]
