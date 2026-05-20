---
title: CloudNativePG
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- prometheus
- helm
- minio
- postgresql
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- CloudNativePG 是什么
- 如何 CloudNativePG
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- CloudNativePG
- cncf
- landscape
---

# CloudNativePG

> **成熟度**: Sandbox | **加入时间**: 2022-05 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://cloudnative-pg.io |
| **GitHub** | https://github.com/cloudnative-pg/cloudnative-pg |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Database & Storage |

---

## 项目概述

CloudNativePG 是 Kubernetes 上的 PostgreSQL Operator，提供完整的数据库生命周期管理。它原生支持 PostgreSQL 流复制、自动故障转移、备份恢复和监控集成。

## 核心特性

- **高可用**: 基于 Patroni 的自动故障转移
- **声明式配置**: CRD 方式管理 PostgreSQL 集群
- **备份恢复**: 支持 S3/Azure/GCS 的连续归档和 PITR
- **原生集成**: 无需外部依赖（如 etcd）
- **监控**: 内置 Prometheus 指标导出
- **安全**: TLS 加密、证书轮换、密钥管理

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                 CloudNativePG Architecture                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   Kubernetes Cluster                       │ │
│  │                                                            │ │
│  │  ┌──────────────────────────────────────────────────────┐ │ │
│  │  │              CloudNativePG Operator                  │ │ │
│  │  └─────────────────────────┬────────────────────────────┘ │ │
│  │                            │                              │ │
│  │                      manages                              │ │
│  │                            ▼                              │ │
│  │  ┌──────────────────────────────────────────────────────┐ │ │
│  │  │              PostgreSQL Cluster                      │ │ │
│  │  │                                                      │ │ │
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────────────┐  │ │ │
│  │  │  │ Primary  │  │ Replica  │  │     Replica      │  │ │ │
│  │  │  │   Pod    │─▶│   Pod    │─▶│      Pod         │  │ │ │
│  │  │  │          │  │          │  │                  │  │ │ │
│  │  │  │ ┌──────┐ │  │ ┌──────┐ │  │  ┌────────────┐ │  │ │ │
│  │  │  │ │  PG  │ │  │ │  PG  │ │  │  │     PG     │ │  │ │ │
│  │  │  │ │ 16   │ │  │ │ 16   │ │  │  │     16     │ │  │ │ │
│  │  │  │ └──────┘ │  │ └──────┘ │  │  └────────────┘ │  │ │ │
│  │  │  │ ┌──────┐ │  │ ┌──────┐ │  │  ┌────────────┐ │  │ │ │
│  │  │  │ │ PVC  │ │  │ │ PVC  │ │  │  │    PVC     │ │  │ │ │
│  │  │  │ └──────┘ │  │ └──────┘ │  │  └────────────┘ │  │ │ │
│  │  │  └──────────┘  └──────────┘  └──────────────────┘  │ │ │
│  │  │                                                      │ │ │
│  │  │  ┌────────────────────────────────────────────────┐ │ │ │
│  │  │  │   Services: RW (Primary) / RO (Replicas)       │ │ │ │
│  │  │  └────────────────────────────────────────────────┘ │ │ │
│  │  └──────────────────────────────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│                        WAL Archive                               │
│                              ▼                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │     Object Storage (S3 / Azure Blob / GCS / MinIO)       │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 Operator

```bash
# kubectl 安装
kubectl apply -f https://raw.githubusercontent.com/cloudnative-pg/cloudnative-pg/release-1.22/releases/cnpg-1.22.0.yaml

# 或 Helm 安装
helm repo add cnpg https://cloudnative-pg.github.io/charts
helm install cnpg cnpg/cloudnative-pg --namespace cnpg-system --create-namespace
```

### 创建 PostgreSQL 集群

```yaml
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: my-postgres
spec:
  instances: 3
  imageName: ghcr.io/cloudnative-pg/postgresql:16.2
  
  storage:
    size: 10Gi
    storageClass: standard
    
  postgresql:
    parameters:
      max_connections: "200"
      shared_buffers: "256MB"
      
  bootstrap:
    initdb:
      database: app
      owner: app
      secret:
        name: app-user-secret
        
  resources:
    requests:
      memory: "512Mi"
      cpu: "250m"
    limits:
      memory: "1Gi"
      cpu: "1"
```

### 备份配置

```yaml
apiVersion: postgresql.cnpg.io/v1
kind: Cluster
metadata:
  name: my-postgres
spec:
  instances: 3
  
  backup:
    barmanObjectStore:
      destinationPath: s3://my-bucket/postgres-backup
      s3Credentials:
        accessKeyId:
          name: s3-creds
          key: ACCESS_KEY_ID
        secretAccessKey:
          name: s3-creds
          key: SECRET_ACCESS_KEY
      wal:
        compression: gzip
        maxParallel: 2
    retentionPolicy: "30d"
---
# 定时备份
apiVersion: postgresql.cnpg.io/v1
kind: ScheduledBackup
metadata:
  name: daily-backup
spec:
  schedule: "0 0 * * *"
  cluster:
    name: my-postgres
  backupOwnerReference: cluster
```

---

## 连接数据库

```bash
# 获取连接信息
kubectl get secret my-postgres-app -o jsonpath='{.data.uri}' | base64 -d

# 端口转发
kubectl port-forward svc/my-postgres-rw 5432:5432

# 连接
psql -h localhost -U app -d app
```

---

## 高可用与故障转移

```yaml
spec:
  instances: 3
  
  # 主节点选举
  primaryUpdateStrategy: unsupervised
  
  # 故障转移延迟
  failoverDelay: 0
  
  # 同步复制
  postgresql:
    synchronous:
      method: any
      number: 1
```

---

## 监控

```yaml
spec:
  monitoring:
    enablePodMonitor: true
    customQueriesConfigMap:
      - name: custom-queries
        key: queries.yaml
```

```yaml
# 关键指标
- cnpg_backends_total
- cnpg_pg_replication_lag
- cnpg_pg_database_size_bytes
- cnpg_collector_up
```

---

## 最佳实践

1. **副本数量**: 生产环境至少 3 个实例
2. **资源配置**: 根据负载配置合理的 shared_buffers
3. **备份策略**: 配置 WAL 归档和定期全量备份
4. **监控告警**: 监控复制延迟和连接数
5. **存储类型**: 使用高性能 SSD 存储

---

## 参考资源

- [官方文档](https://cloudnative-pg.io/documentation/)
- [GitHub Repo](https://github.com/cloudnative-pg/cloudnative-pg)
- [备份恢复指南](https://cloudnative-pg.io/documentation/current/backup_recovery/)

---

**维护者**: Kudig Team | **许可证**: MIT
