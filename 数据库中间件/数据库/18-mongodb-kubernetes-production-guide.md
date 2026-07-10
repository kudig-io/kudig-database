---
title: MongoDB Kubernetes 生产指南
description: 面向生产环境的 MongoDB on Kubernetes 运维手册，覆盖 ReplicaSet 与 Sharded 集群选型、Operator 部署、备份恢复、TLS、mTLS、监控告警与故障转移。
summary: 面向生产环境的 MongoDB on Kubernetes 运维手册，覆盖 ReplicaSet 与 Sharded 集群选型、Operator 部署、备份恢复、TLS、mTLS、监控告警与故障转移。
category: database-middleware
tags:
- production
- best-practices
- playbook
- database-middleware
- mongodb
- operator
- replicaset
- sharded
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 30min
intent_queries:
- MongoDB on Kubernetes 生产环境如何部署
- MongoDB ReplicaSet 与 Sharded 集群在 K8s 中如何选择
- MongoDB Operator 备份恢复与 TLS 配置
- MongoDB on K8s 故障转移与监控
trigger_keywords:
- mongodb kubernetes
- mongodb operator
- replicaset
- sharded cluster
- mongodb 生产指南
- mongodb 备份恢复
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- mongodb-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

# MongoDB Kubernetes 生产指南

本指南面向需要在 Kubernetes 生产环境中运行 MongoDB 的 SRE、DBA 与平台工程师，提供从架构选型、Operator 部署、安全加固、备份恢复到监控告警与故障转移的完整运行手册。MongoDB 作为文档型 NoSQL 数据库的代表，其灵活的 Schema、强大的水平扩展能力与丰富的查询能力使其在内容管理、物联网、实时分析与用户画像等场景得到广泛应用。在 Kubernetes 上运行 MongoDB 时，必须特别关注数据持久化、副本集高可用、分片集群设计以及备份恢复的可操作性。

## 1. 适用场景与范围

本指南适用于以下场景：

- 文档型 NoSQL 数据库的 Kubernetes 化部署，适用于内容管理、物联网、实时分析、用户画像等场景。
- 使用 MongoDB Community Operator 或 Percona Operator for MongoDB 的声明式管理。
- 覆盖 ReplicaSet 与 Sharded Cluster 两种拓扑；不覆盖裸 StatefulSet 手工部署。
- 重点关注生产就绪要求，包括数据持久化、高可用、安全、备份恢复与可观测性。

本指南不深入讲解 MongoDB 应用层设计，如 Schema 设计、索引优化等，这些内容应由 DBA 与开发团队共同负责。

## 2. 前置条件与工具

在开始部署前，请确认以下前置条件已经满足：

- Kubernetes 1.28–1.33，工作节点具备跨 AZ 分布。
- StorageClass 使用 SSD backing，建议启用 `allowVolumeExpansion: true`。
- 已安装 Helm 3、kubectl，具备管理 CRD 与 Secret 的权限。
- 已部署 Prometheus + Grafana，用于指标采集。
- 可选：cert-manager 用于 TLS；Velero 或 CSI 快照用于备份。
- 建议预先完成 MongoDB 镜像与 Operator 镜像的内部同步，避免生产环境依赖公网拉取。

## 3. 核心概念与架构

### 3.1 ReplicaSet vs Sharded Cluster

| 维度 | ReplicaSet | Sharded Cluster |
|---|---|---|
| 适用规模 | TB 级以下、读写可在 Primary 承受 | TB–PB 级、写入/并发需水平扩展 |
| 高可用 | 自动故障转移，至少 3 成员 | Config Server + mongos + Shard 均冗余 |
| 运维复杂度 | 低 | 高，需要分片键设计与均衡监控 |
| K8s 资源 | StatefulSet + PVC | 多个 StatefulSet、mongos Deployment、Config Server |
| 扩展方式 | 垂直扩容 / 增加 Secondary | 增加 Shard 或拆分热点分片 |
| 一致性 | 强一致性，写操作落在 Primary | 跨 Shard 事务性能下降 |

生产建议：数据量 < 1TB 且 QPS 可控时优先使用 ReplicaSet；超过单节点写入瓶颈或数据增长预期 > 2TB/年时采用 Sharded Cluster。分片键选择是 Sharded Cluster 设计的核心，应满足高基数、低频率、非单调三个条件。错误选择的分片键会导致数据倾斜、热点 Chunk 与均衡困难，最终影响集群性能与可用性。

### 3.2 Operator 选型

- **MongoDB Community Operator**：官方开源，功能覆盖 ReplicaSet、用户、TLS，适合标准场景。
- **Percona Operator for MongoDB**：功能更完整，支持备份、PITR、监控、Sharded Cluster，适合企业级需求。

本指南以 Community Operator 为例，关键概念同样适用于 Percona Operator。对于需要 PITR 或多集群灾备的场景，建议评估 Percona Operator。Operator 本身应配置多个副本并跨 AZ 分布，避免因 Operator 单点故障导致数据库集群无法协调。

### 3.3 存储与资源规划

- WiredTiger 缓存默认占用 `(RAM - 1GB) / 2`，容器 limits.memory 应至少为 `2 × cacheSizeGB + 2GB`。
- Oplog 大小建议能容纳 24–72 小时写操作，避免 Secondary 离线后需要全量重新同步。
- 对 IOPS 敏感的工作负载，使用高 IOPS StorageClass 并将数据目录与日志目录分离。
- 生产环境建议为 MongoDB Pod 设置 Guaranteed QoS，即 requests 等于 limits。

### 3.4 高可用与拓扑分布

MongoDB 的故障转移依赖于 ReplicaSet 多数派。因此：

- 成员数必须为奇数，推荐 3 或 5。
- Pod 必须通过 PodDisruptionBudget 与反亲和性跨 AZ/节点分布。
- Arbiter 可用于偶数节点场景，但仲裁节点不存储数据，不能替代 Secondary 的读取能力。

在 Kubernetes 环境中，StatefulSet 的有序部署特性与 MongoDB 的副本集初始化天然契合，但仍需通过反亲和性确保副本不会集中在同一节点或同一 AZ。

## 4. 标准操作流程

### 4.1 安装 MongoDB Community Operator

```bash
helm repo add mongodb https://mongodb.github.io/helm-charts
helm repo update

kubectl create namespace mongodb

helm install community-operator mongodb/community-operator \
  --namespace mongodb \
  --set operator.watchNamespace='mongodb' \
  --set operator.resources.limits.memory=512Mi
```

验证：

```bash
kubectl get deployment community-operator -n mongodb
kubectl get crd | grep mongodb.com
```

### 4.2 部署 ReplicaSet

```yaml
apiVersion: mongodbcommunity.mongodb.com/v1
kind: MongoDBCommunity
metadata:
  name: prod-mongodb
  namespace: mongodb
spec:
  members: 3
  type: ReplicaSet
  version: "7.0.5"
  security:
    authentication:
      modes: ["SCRAM"]
  users:
    - name: admin
      db: admin
      passwordSecretRef:
        name: admin-password
      roles:
        - name: clusterAdmin
          db: admin
        - name: userAdminAnyDatabase
          db: admin
      scramCredentialsSecretName: admin-scram
  statefulSet:
    spec:
      serviceName: prod-mongodb-svc
      replicas: 3
      selector: {}
      template:
        spec:
          affinity:
            podAntiAffinity:
              preferredDuringSchedulingIgnoredDuringExecution:
                - weight: 100
                  podAffinityTerm:
                    labelSelector:
                      matchExpressions:
                        - key: app
                          operator: In
                          values:
                            - prod-mongodb-svc
                    topologyKey: topology.kubernetes.io/zone
          containers:
            - name: mongod
              resources:
                limits:
                  cpu: "4"
                  memory: 8Gi
                requests:
                  cpu: "4"
                  memory: 8Gi
      volumeClaimTemplates:
        - metadata:
            name: data-volume
          spec:
            accessModes: ["ReadWriteOnce"]
            storageClassName: fast-ssd
            resources:
              requests:
                storage: 200Gi
```

创建 Secret 并应用：

```bash
kubectl create secret generic admin-password \
  --from-literal=password='$(openssl rand -base64 32)' -n mongodb

kubectl apply -f mongodb-replicaset.yaml
kubectl wait mongodbcommunity/prod-mongodb --for=condition=ReplicaSetReady --timeout=300s -n mongodb
```

### 4.3 启用 TLS

使用 cert-manager 签发证书：

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: prod-mongodb-cert
  namespace: mongodb
spec:
  secretName: prod-mongodb-tls
  issuerRef:
    name: mongodb-ca-issuer
    kind: ClusterIssuer
  dnsNames:
    - prod-mongodb-svc.mongodb.svc.cluster.local
    - prod-mongodb-0.prod-mongodb-svc.mongodb.svc.cluster.local
    - prod-mongodb-1.prod-mongodb-svc.mongodb.svc.cluster.local
    - prod-mongodb-2.prod-mongodb-svc.mongodb.svc.cluster.local
```

在 `MongoDBCommunity` CR 中引用：

```yaml
spec:
  security:
    tls:
      enabled: true
      certificateKeySecretRef:
        name: prod-mongodb-tls
      caConfigMapRef:
        name: prod-mongodb-ca
```

### 4.4 备份与恢复

使用 `mongodump` / `mongorestore` 或 Percona Backup 进行逻辑备份；对大规模数据使用 CSI 快照或文件系统备份。

```bash
# 逻辑备份示例
kubectl exec -n mongodb prod-mongodb-0 -- \
  mongodump --uri="mongodb://admin:$(kubectl get secret admin-password -n mongodb -o jsonpath='{.data.password}' | base64 -d)@localhost:27017/admin?authSource=admin" \
  --out=/backup/$(date +%F)

# 恢复演练（隔离命名空间）
velero restore create --from-backup mongodb-daily \
  --namespace-mappings mongodb:mongodb-restore
```

生产建议：

- 每日执行全量逻辑备份或文件系统快照。
- 关键集合启用 Oplog 备份，支持时间点恢复（PITR）。
- 每月至少执行一次恢复演练，验证备份完整性与 RTO。
- 备份文件应存放在与生产集群不同的区域或对象存储桶。

### 4.5 扩容 PVC

```bash
kubectl patch pvc data-volume-prod-mongodb-0 -n mongodb -p \
  '{"spec":{"resources":{"requests":{"storage":"400Gi"}}}}'
```

扩容后观察 StatefulSet 滚动更新与副本同步状态。注意：StorageClass 必须启用 `allowVolumeExpansion: true`，否则 patch 会失败。

## 5. 关键检查点与验证命令

| 检查项 | 命令/配置 |
|---|---|
| ReplicaSet 健康 | `kubectl exec -n mongodb prod-mongodb-0 -- mongosh --eval 'rs.status()'` |
| 主从复制延迟 | `kubectl exec ... -- mongosh --eval 'rs.printSecondaryReplicationInfo()'` |
| 连接数 | `kubectl exec ... -- mongosh --eval 'db.serverStatus().connections'` |
| 存储使用 | `kubectl top pvc -n mongodb` |
| WiredTiger 缓存命中率 | `db.serverStatus().wiredTiger.cache` |
| TLS 证书有效期 | `kubectl get secret prod-mongodb-tls -n mongodb -o jsonpath='{.data.tls\.crt}' \| base64 -d \| openssl x509 -noout -dates` |
| Oplog 大小 | `kubectl exec ... -- mongosh --eval 'rs.printReplicationInfo()'` |
| Pod 分布 | `kubectl get pods -n mongodb -o wide` |

## 6. 常见故障与 Remediation

| 现象 | 可能根因 | 处置 |
|---|---|---|
| Secondary 同步滞后 | 网络延迟 / Oplog 不足 / Secondary IO 饱和 | 扩容 Oplog；检查 `rs.printSecondaryReplicationInfo()`；提升磁盘 IOPS |
| Primary 无法选举 | 多数成员离线 / 网络分区 | 确认 Pod 跨 AZ 分布；检查 NetworkPolicy；必要时手动触发 `rs.reconfig` |
| Pod OOMKilled | WiredTiger 缓存过大 / 聚合查询内存溢出 | 调整 limits.memory 与 `cacheSizeGB`；优化查询 |
| PVC 容量耗尽 | 数据增长未预期 | 启用 StorageClass 扩容；设置 75%/85% 容量告警 |
| 连接数打满 | 连接池泄漏 / maxConn 过低 | 修复应用连接池；调整 `net.maxIncomingConnections` |
| 写入延迟突增 | 磁盘 IO 饱和 / 热点文档锁竞争 | `iostat -x 1`；拆分热点集合；提升存储类型 |
| TLS 连接失败 | 证书过期 / DNS 名不匹配 | 检查 cert-manager 续期；更新 Certificate DNSNames |
| 分片不均衡 | 分片键选择不当 / Chunk 迁移失败 | `sh.status()`；手动拆分热点 chunk；重新选择分片键 |

## 7. 风险与注意事项

- **强一致性需求**：Sharded Cluster 的事务跨多个 Shard 时性能会显著下降，设计阶段应评估是否需要单 Shard 内事务。
- **分片键选择**：分片键一旦确定， resharding 在 5.0+ 虽支持，但仍是重量级操作，需提前建模访问模式。
- **PodDisruptionBudget**：必须为 mongod、mongos、Config Server 配置 PDB，避免节点维护时同时下线多数副本。
- **持久化存储**：禁止对 Primary 使用 EmptyDir；所有数据卷必须通过 PVC 挂载。
- **安全基线**：生产环境强制启用 TLS 与 SCRAM-SHA-256，关闭默认未认证端口；敏感操作开启审计日志。
- **密码管理**：禁止在 Git 中明文存储密码，使用 External Secrets Operator 或 Vault 注入。

## 8. 相关 Runbook / 推荐阅读

- [[数据库中间件/数据库/05-mongodb-enterprise-database.md|MongoDB 企业级数据库运维深度实践]]
- [[数据库中间件/99-production-readiness-operations-guide.md|Database & Middleware 生产就绪运维指南]]
- [[可靠性/99-production-readiness-operations-guide.md|可靠性工程生产就绪运维指南]]
- [[生产运维/99-production-readiness-operations-guide.md|生产运维域生产就绪运维指南]]
- MongoDB Sharded Cluster on Kubernetes 深度指南（待补充）
- MongoDB 安全加固与审计日志（待补充）

---

*本指南基于 MongoDB 7.0 与 Community Operator 编写，Sharded Cluster 场景建议结合 Percona Operator 进一步评估。*
