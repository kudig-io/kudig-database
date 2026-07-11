---
title: Kanister (entities)
description: '## 概述'
summary: 'Kanister 是一个面向 Kubernetes 的应用级数据管理框架，专门用于有状态应用（数据库、消息队列等）的备份和恢复。'
category: entities
tags:
- k8s
- cncf
- storage
- kanister
- postgresql
- job
- cronjob
- crd
- operator
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kanister 是什么
- 如何 Kanister
trigger_keywords:
- Kanister
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Kanister

> **CNCF 状态**: Sandbox | **类别**: Storage/Backup | **主要语言**: Go

## 概述

Kanister 是一个面向 Kubernetes 的应用级数据管理框架，由 Kasten（Veeam 旗下）开发，2021 年加入 CNCF 沙箱。它专门用于有状态应用（如 PostgreSQL、MySQL、MongoDB、Cassandra、Elasticsearch 等）的备份和恢复。Kanister 使用 Blueprint CRD 定义应用特定的备份/恢复操作流程，支持应用一致性的快照和备份——在备份数据前调用应用的 quiesce API（如 PostgreSQL 的 `pg_dump` 或 `pg_start_backup`），确保备份的数据处于一致状态。Kanister 可以与应用的数据保护 API 深度集成，还支持将备份数据推送到 S3/Azure Blob/GCS 等对象存储，实现异地灾备。

## 核心能力

- **应用一致性备份**: 在备份前调用应用 quiesce API（如 pg_dump、mongodump），确保数据一致
- **Blueprint CRD**: 声明式定义应用备份/恢复/删除流程（由 Phase 和 Action 组成）
- **多应用支持**: PostgreSQL、MySQL、MongoDB、Cassandra、Elasticsearch、Kafka 等
- **对象存储集成**: 将备份数据推送到 S3、Azure Blob、GCS、MinIO 等
- **ActionSet 调度**: 通过 ActionSet CRD 触发备份/恢复操作
- **CronJob 集成**: 结合 CronJob 实现定期自动化备份

## 架构

Kanister 采用 Blueprint + ActionSet 模式：

- **Kanister Operator**: 核心 Controller，监听 ActionSet 和 Blueprint 资源
- **Blueprint CRD**: 应用数据保护流程定义（Phase → Action → Args → Output）
- **ActionSet CRD**: 触发特定操作的资源（执行 Blueprint 中的某个 Action）
- **Kanister Tool (Job)**: 执行实际操作的 Pod（运行 kutul/kando CLI 工具）
- **Repository Server**: 可选的数据去重和加密服务（基于 Kopia）
- **Profile CRD**: 定义备份存储位置（S3/Azure/GCS）和凭据

备份流程：`ActionSet (backup) → Operator → Blueprint (phases) → Job → pg_dump → S3`

## K8s 集成

Kanister 以 Operator 模式部署在 Kubernetes 集群中。Blueprint CRD 定义每种应用的备份/恢复流程（如 PostgreSQL Blueprint 定义了 backup、restore、delete 三个 Action）。用户创建 ActionSet CRD 触发操作，Operator 解析 Blueprint 中的 Phase 顺序，创建 Kanister Tool Job 执行实际命令。Kanister Tool 通过 kubectl/API 访问应用 Pod（如执行 `kubectl exec postgres-pod -- pg_dump`），将备份结果推送到对象存储。恢复时从对象存储拉取数据，通过 `kubectl exec` 恢复到应用。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 Job、Secret、ConfigMap 深度集成。

## 生产场景

1. **数据库定期备份**: 结合 CronJob 每日执行 PostgreSQL/MySQL 的应用一致性备份
2. **灾难恢复**: 从 S3 恢复数据库到新集群
3. **数据迁移**: 将一个集群的数据库备份恢复到另一个集群
4. **备份验证**: 定期执行备份+恢复验证，确保备份可用性

## 安装

```bash
# Helm 安装 Kanister Operator
helm repo add kanister https://charts.kanister.io
helm install kanister kanister/kanister-operator -n kanister --create-namespace

# 创建 S3 备份 Profile
kubectl apply -f - <<EOF
apiVersion: cr.kanister.io/v1alpha1
kind: Profile
metadata:
  name: s3-backup
  namespace: default
spec:
  location:
    type: s3Compliant
    s3Compliant:
      endpoint: https://s3.amazonaws.com
      bucket: my-backup-bucket
      prefix: kanister
  credential:
    type: keyPair
    keyPair:
      idField: AWS_ACCESS_KEY_ID
      secretField: AWS_SECRET_ACCESS_KEY
      secret:
        apiVersion: v1
        kind: Secret
        name: aws-credentials
        namespace: default
EOF

# 创建 PostgreSQL 备份 ActionSet
kubectl apply -f - <<EOF
apiVersion: cr.kanister.io/v1alpha1
kind: ActionSet
metadata:
  name: pg-backup
  namespace: default
spec:
  actions:
  - name: backup
    blueprint: postgres-blueprint
    object:
      kind: StatefulSet
      name: postgres
      namespace: default
    profile:
      name: s3-backup
      namespace: default
EOF

# 查看备份状态
kubectl describe actionset pg-backup
```

## 对比

| 特性 | Kanister | Velero | K8up | Stash |
|------|----------|--------|------|-------|
| 应用一致性 | ✅ Blueprint | ⚠️ 仅磁盘快照 | ⚠️ | ⚠️ |
| 自定义流程 | ✅ | ⚠️ | ⚠️ | ⚠️ |
| 多应用支持 | ✅ 丰富 | ❌ | ⚠️ | ⚠️ |
| CNCF 状态 | Sandbox | Incubating | Sandbox | 非 CNCF |

## 架构定位

在 CNCF 生态中，Kanister 属于 **Storage/Backup** 类别，为云原生应用提供应用级数据管理能力。

## 参考链接

- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[47-terway-troubleshooting-fta]]

- changelog.md|ecosystem-changelog]]

- [[42-terway-usage-guide]]

- metal3-io

- inspektor-gadget

- [[kubearmor]] — KubeArmor
- [[实体/cncf-cicd.md|cncf-cicd]] — CNCF CI/CD 与发布管理项目全景
- [[实体/cncf-networking.md|cncf-networking]] — CNCF 网络与服务网格项目全景
- [[armada]] — Armada
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- digest-2026-05-21-full
- kanister
- [[实体/k8up.md|K8up]]
- [[实体/openebs.md|OpenEBS]]
- [[实体/hwameistor.md|HwameiStor]]
- [[实体/carina.md|Carina]]
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[生态参考/领域索引/backup-dr-index.md|Backup & DR 备份与灾备知识图谱索引]]
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
