---
title: StatefulSet
description: StatefulSet — Kubernetes 生产运维知识库
summary: StatefulSet — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- statefulset
- workload
- stateful
- ordered
- persistent-storage
- mysql
- postgresql
- kafka
- elasticsearch
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- StatefulSet 是什么
- 如何 StatefulSet
trigger_keywords:
- StatefulSet
prerequisites:
- kubectl-basics
- kafka-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# StatefulSet

## Role

StatefulSet manages stateful workloads that require stable identity and persistent storage, such as databases (MySQL, PostgreSQL, Elasticsearch, Kafka).

## Key Properties

| Property | Description |
|----------|-------------|
| **Stable Pod identity** | [[Pods|Pods]] named `{name}-{0}`, `{name}-{1}`, ... in order |
| **Ordered operations** | Pods created 0→N, terminated N→0 |
| **Persistent storage** | Each Pod gets its own PVC from `volumeClaimTemplates` |
| **Stable network** | DNS via Headless [[Service|Service]]: `pod-0.service.ns.svc.cluster.local` |
| **PVC retention** | PVCs survive Pod deletion (data persists) |

## Update Strategy

| Strategy | Behavior |
|----------|----------|
| **RollingUpdate** | Update Pods in reverse order (N→0), waiting for each to be Ready |
| **OnDelete** | Only update when Pods are manually deleted |
| **Partition** | Update only Pods with index >= partition (canary rollout) |

## Volume Claim Templates

Each Pod gets a dedicated PVC. Unlike Deployment where Pods share a volume template, StatefulSet creates unique PVCs per Pod, ensuring data isolation.

## Use Cases

Databases (MySQL, PostgreSQL, MongoDB), message brokers (Kafka, RabbitMQ), search engines (Elasticsearch), and any application requiring persistent identity and storage.

## Related
- [[concepts/Operator 模式 × Pod 生命周期.md|Operator 模式 × Pod 生命周期]] — 综合

- [[skills/deployment-workload-selection.md|deployment-workload-selection]] — 工作负载控制器选型
- [[skills/skill-21-statefulset-failure.md|skill-21-statefulset-failure]] — StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation
- [[INDEX]] — Wiki Index
- [[deployment]] — Deployment
- [[concepts/storage-model.md|storage-model]] — Persistent Storage Model (PV/PVC/StorageClass)
- [[deployment|Deployment]]
- [[concepts/storage-model.md|Persistent Storage Model]]
- [[pod-lifecycle|Pod Lifecycle]]
- Headless Service

- 08-statefulset-daemonset-events
- 05-statefulset-reference
- 03-statefulset-advanced-operations
- [[故障诊断/资源排障/21-statefulset-troubleshooting.md|21-statefulset-troubleshooting]]
- [[故障诊断/FTA故障树/list/statefulset-fta.md|StatefulSet 异常故障树分析]]
- [[故障诊断/高级排障/05-workloads/03-statefulset-troubleshooting.md|03-statefulset-troubleshooting]]
- [[skills/statefulset-fta.md|StatefulSet 异常故障树分析]] — Cross-reference


<!-- risk-assessed -->
