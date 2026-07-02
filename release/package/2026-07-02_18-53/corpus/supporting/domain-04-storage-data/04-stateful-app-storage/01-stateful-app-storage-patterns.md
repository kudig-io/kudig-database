---
title: 有状态应用 Kubernetes 存储模式
description: MySQL、PostgreSQL、Kafka、Elasticsearch、Redis 在阿里云与专有云 K8s 上的存储选型、StatefulSet + Headless Service、本地盘与网络存储对比、备份策略与存储对接
category: storage
tags:
- k8s
- statefulset
- storage
- mysql
- postgresql
- kafka
- elasticsearch
- redis
- alicloud
- apsara-stack
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 应用架构师
- 运维工程师
estimated_read_time: 35min
intent_queries:
- K8s 有状态应用存储如何选型
- MySQL PostgreSQL Kafka Redis K8s 存储模式
- 阿里云 K8s 有状态应用备份策略
trigger_keywords:
- 有状态应用
- StatefulSet
- Headless Service
- MySQL
- PostgreSQL
- Kafka
- Elasticsearch
- Redis
prerequisites:
- kubectl-basics
- storage-basics
- statefulset-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
created: "2026-06-26"
updated: "2026-06-26"
summary: '2. [StatefulSet + Headless Service 模式](#statefulset--headless-service-模式)'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 有状态应用 Kubernetes 存储模式

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，系统梳理 MySQL、PostgreSQL、Kafka、Elasticsearch、Redis 五大典型有状态应用的存储选型、部署模式、备份策略与运维注意事项。

<!-- chunk: 目录 -->
## 目录

1. [存储选型总览](#存储选型总览)
2. [StatefulSet + Headless Service 模式](#statefulset--headless-service-模式)
3. [本地盘 vs 网络存储](#本地盘-vs-网络存储)
4. [MySQL 存储模式](#mysql-存储模式)
5. [PostgreSQL 存储模式](#postgresql-存储模式)
6. [Kafka 存储模式](#kafka-存储模式)
7. [Elasticsearch 存储模式](#elasticsearch-存储模式)
8. [Redis 存储模式](#redis-存储模式)
9. [备份策略通用框架](#备份策略通用框架)
10. [阿里云/专有云存储对接](#阿里云专有云存储对接)
11. [故障排查速查](#故障排查速查)
12. [最佳实践检查清单](#最佳实践检查清单)

---

<!-- chunk: 1. 存储选型总览 -->
## 1. 存储选型总览

| 应用 | 推荐存储类型 | 访问模式 | 典型延迟要求 | 阿里云对应 |
|:---|:---|:---:|:---|:---|
| MySQL | ESSD 云盘 / 本地 SSD | RWO | 低 | 阿里云云盘 CSI / LVM Local PV |
| PostgreSQL | ESSD 云盘 / 本地 SSD | RWO | 低 | 阿里云云盘 CSI / LVM Local PV |
| Kafka | 本地 SSD / ESSD | RWO | 中低 | 本地盘 CSI / 云盘 CSI |
| Elasticsearch | ESSD / 本地 SSD | RWO | 低 | 云盘 CSI / 本地盘 CSI |
| Redis（主从） | 本地 SSD / ESSD | RWO | 极低 | 本地盘 CSI / 云盘 CSI |
| Redis Cluster | 本地 SSD / ESSD | RWO | 极低 | 本地盘 CSI / 云盘 CSI |

---

<!-- chunk: 2. StatefulSet + Headless Service -->
## 2. StatefulSet + Headless Service 模式

```yaml
apiVersion: v1
kind: Service
metadata:
  name: mysql-headless
  namespace: production
spec:
  clusterIP: None
  selector:
    app: mysql
  ports:
    - port: 3306
      name: mysql
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: production
spec:
  serviceName: mysql-headless
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      containers:
        - name: mysql
          image: mysql:8.0
          ports:
            - containerPort: 3306
          volumeMounts:
            - name: data
              mountPath: /var/lib/mysql
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: alicloud-disk-essd
        resources:
          requests:
            storage: 100Gi
```

**关键特性**：
- 稳定网络标识：`mysql-0.mysql-headless.production.svc.cluster.local`
- 稳定存储：PVC 与 Pod 序号绑定，重建后复用原 PVC
- 有序部署/扩缩容：按序号 0,1,2 顺序启动或逆序终止

---

<!-- chunk: 3. 本地盘 vs 网络存储 -->
## 3. 本地盘 vs 网络存储

| 维度 | 本地盘 (Local PV) | 网络存储 (云盘/NAS) |
|:---|:---|:---|
| 性能 | 高，低延迟 | 中等，受网络影响 |
| 可用性 | 节点绑定，节点故障时卷不可用 | 独立于节点，可跨节点挂载 |
| 调度 | 需考虑数据分布，使用延迟绑定 | 调度灵活 |
| 备份 | 依赖应用层或快照代理 | 支持 CSI 快照 |
| 成本 | 低 | 中等 |
| 适用场景 | Kafka、ES、Redis、高 IOPS 数据库 | 通用数据库、共享文件 |

### 3.1 Local PV StorageClass 示例

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: openebs-lvmpv-local-ssd
provisioner: local.csi.openebs.io
volumeBindingMode: WaitForFirstConsumer
```

---

<!-- chunk: 4. MySQL 存储模式 -->
## 4. MySQL 存储模式

### 4.1 推荐架构

| 模式 | 副本数 | 存储类型 | 适用场景 |
|:---|:---:|:---|:---|
| 单实例 | 1 | ESSD 云盘 | 开发测试、轻量业务 |
| 主从复制 | 1 主 + 2 从 | ESSD 云盘 | 生产读多写少 |
| MySQL Group Replication | 3 | ESSD 云盘 | 生产自动故障切换 |
| 阿里云 RDS | N/A | 托管 | 核心生产优先推荐 |

### 4.2 关键配置

```yaml
volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: alicloud-disk-essd
      resources:
        requests:
          storage: 500Gi
```

### 4.3 备份命令

```bash
# 物理备份（Percona XtraBackup）
xtrabackup --backup --target-dir=/backup/mysql/$(date +%Y%m%d)

# 逻辑备份
mysqldump -u root -p --all-databases --single-transaction --quick | gzip > /backup/mysql-$(date +%Y%m%d).sql.gz

# binlog 归档
mysqlbinlog --read-from-remote-server --raw mysql-bin.000001 > /backup/binlog/
```

---

<!-- chunk: 5. PostgreSQL 存储模式 -->
## 5. PostgreSQL 存储模式

### 5.1 推荐架构

| 模式 | 副本数 | 存储类型 | 适用场景 |
|:---|:---:|:---|:---|
| 单实例 | 1 | ESSD 云盘 | 开发测试 |
| Patroni + etcd | 3 | ESSD 云盘 | 生产 HA |
| 阿里云 PolarDB | N/A | 托管 | 核心生产优先推荐 |

### 5.2 关键配置

```yaml
volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: alicloud-disk-essd
      resources:
        requests:
          storage: 500Gi
```

### 5.3 备份命令

```bash
# 逻辑备份
pg_dumpall -U postgres -h localhost | gzip > /backup/postgres-$(date +%Y%m%d).sql.gz

# 物理备份（pg_basebackup）
pg_basebackup -D /backup/base-$(date +%Y%m%d) -Ft -z -P -X stream

# WAL 归档
archive_command = 'cp %p /backup/wal/%f'
```

---

<!-- chunk: 6. Kafka 存储模式 -->
## 6. Kafka 存储模式

### 6.1 推荐架构

| 模式 | 副本数 | 存储类型 | 适用场景 |
|:---|:---:|:---|:---|
| Kraft 模式 | 3 | 本地 SSD | K8s 原生推荐 |
| ZooKeeper 模式 | 3 Broker + 3 ZK | 本地 SSD | 传统部署 |
| 阿里云 Kafka | N/A | 托管 | 核心生产优先推荐 |

### 6.2 关键配置

```yaml
volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: openebs-lvmpv-local-ssd
      resources:
        requests:
          storage: 1Ti
```

### 6.3 重要参数

```properties
# server.properties
log.dirs=/var/lib/kafka/data
log.retention.hours=168
log.segment.bytes=1073741824
num.io.threads=8
num.network.threads=8
offsets.topic.replication.factor=3
default.replication.factor=3
min.insync.replicas=2
```

---

<!-- chunk: 7. Elasticsearch 存储模式 -->
## 7. Elasticsearch 存储模式

### 7.1 推荐架构

| 模式 | 节点数 | 存储类型 | 适用场景 |
|:---|:---:|:---|:---|
| 单角色混合 | 3 | ESSD 云盘 | 中小规模 |
| 冷热分离 | 热节点 3 + 温节点 3 | ESSD / 高效云盘 | 日志场景 |
| 阿里云 ES | N/A | 托管 | 核心生产优先推荐 |

### 7.2 关键配置

```yaml
volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: alicloud-disk-essd
      resources:
        requests:
          storage: 2Ti
```

### 7.3 重要参数

```yaml
env:
  - name: ES_JAVA_OPTS
    value: "-Xms16g -Xmx16g"
  - name: node.roles
    value: "data,ingest,master"
```

---

<!-- chunk: 8. Redis 存储模式 -->
## 8. Redis 存储模式

### 8.1 推荐架构

| 模式 | 副本数 | 存储类型 | 适用场景 |
|:---|:---:|:---|:---|
| 主从 + Sentinel | 1 主 + 2 从 + 3 Sentinel | 本地 SSD | 生产缓存 |
| Redis Cluster | 6 节点（3 主 3 从） | 本地 SSD | 大规模缓存 |
| 阿里云 Tair/Redis | N/A | 托管 | 核心生产优先推荐 |

### 8.2 关键配置

```yaml
volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: openebs-lvmpv-local-ssd
      resources:
        requests:
          storage: 50Gi
```

### 8.3 持久化策略

| 策略 | 说明 | 适用场景 |
|:---|:---|:---|
| RDB | 定时快照 | 容忍部分数据丢失 |
| AOF | 日志追加 | 数据可靠性要求高 |
| RDB + AOF | 双重保障 | 生产推荐 |

---

<!-- chunk: 9. 备份策略通用框架 -->
## 9. 备份策略通用框架

| 应用 | 备份方式 | 频率 | 保留周期 | 存储位置 |
|:---|:---|:---:|:---:|:---|
| MySQL | xtrabackup / mysqldump | 每日全量 + 每小时增量 binlog | 30 天 | OSS |
| PostgreSQL | pg_basebackup / pg_dumpall | 每日全量 + WAL 归档 | 30 天 | OSS |
| Kafka | MirrorMaker 2 / 双集群 | 实时 | 按主题策略 | 异地集群 |
| Elasticsearch | snapshot | 每日 | 14 天 | OSS/S3 |
| Redis | RDB + AOF + 主从复制 | 实时复制 | 按业务需求 | 异地实例 |

### 9.1 通用备份 CronJob 模板

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: database-backup
  namespace: production
spec:
  schedule: "0 2 * * *"
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: backup
              image: registry.cn-hangzhou.aliyuncs.com/acs/aliyun-cli:latest
              command:
                - /bin/sh
                - -c
                - |
                  /app/backup.sh
                  ossutil cp /backup/latest.tar.gz oss://db-backup-bucket/latest.tar.gz
          restartPolicy: OnFailure
```

---

<!-- chunk: 10. 阿里云/专有云存储对接 -->
## 10. 阿里云/专有云存储对接

### 10.1 阿里云 ACK 存储类矩阵

| 存储类型 | StorageClass | 访问模式 | 适用应用 |
|:---|:---|:---:|:---|
| ESSD 云盘 | alicloud-disk-essd | RWO | MySQL/PostgreSQL/ES |
| SSD 云盘 | alicloud-disk-ssd | RWO | 一般数据库 |
| 高效云盘 | alicloud-disk-efficiency | RWO | 非关键应用 |
| NAS | alicloud-nas | RWX | 共享文件、日志 |
| OSS | alicloud-oss | RWX | 静态资源、备份 |
| 本地盘 | openebs-lvmpv-local-ssd | RWO | Kafka/Redis/ES |

### 10.2 专有云 ASO/天基对接

- 通过 ASO 控制台申请块存储、NAS、OSS 资源
- 确认 CSI 插件镜像已同步到专有云镜像仓库
- 在 `StorageClass` 中指定正确的 `regionId`、`zoneId` 与 `encrypt` 参数
- 备份目标优先使用专有云 OSS 内网 Endpoint

---

<!-- chunk: 11. 故障排查速查 -->
## 11. 故障排查速查

| 问题 | 排查命令 | 常见根因 |
|:---|:---|:---|
| Pod 一直 Pending | `kubectl describe pod` | PVC 未绑定、StorageClass 不存在 |
| PVC 一直 Pending | `kubectl describe pvc` | 云盘库存不足、CSI 异常 |
| 数据库 IO 高 | `iostat -x 1` | 存储类型不匹配、索引缺失 |
| Kafka 副本不同步 | `kafka-reassign-partitions.sh` | 节点故障、网络分区 |
| Redis 主从切换频繁 | `redis-cli info replication` | 节点资源不足、超时配置不当 |

---

<!-- chunk: 12. 最佳实践检查清单 -->
## 12. 最佳实践检查清单

| 检查项 | 要求 | 验证命令 |
|:---|:---|:---|
| StatefulSet 使用 Headless Service | 所有有状态应用 | `kubectl get svc` |
| PVC 使用合适 StorageClass | 按应用选型 | `kubectl get pvc` |
| Pod 反亲和性 | 副本跨节点/可用区 | `kubectl get pod -o wide` |
| 资源限制设置 | CPU/内存 limits | `kubectl describe sts` |
| 备份任务成功 | 每日检查 CronJob | `kubectl get cj` |
| 存储扩容测试 | 验证 PVC 可扩容 | 编辑 PVC storage |
| 节点故障演练 | 验证数据不丢失 | 模拟节点下线 |
| 监控告警 | 磁盘/延迟/副本 | PrometheusRule |
| 阿里云 CLI 检查 | 云盘状态正常 | `aliyun ecs DescribeDisks` |

---

## Related

- [[domain-04-storage-data/01-k8s-storage/10-storage-backup-disaster-recovery|10 - 存储备份与灾难恢复]]
- [[domain-04-storage-data/README|Storage Domain 存储领域知识库]]

## See Also

- [[domain-04-storage-data/03-distributed-storage/01-velero-backup-recovery|Velero 阿里云专有云备份恢复实战]]
- [[domain-04-storage-data/03-distributed-storage/02-rook-ceph-production|Rook-Ceph 生产指南]]
- [[domain-04-storage-data/03-distributed-storage/03-longhorn-production|Longhorn 生产指南]]

---

## 阿里云 ACK StorageClass 完整示例

以下示例为 MySQL 主库创建一个高性能 ESSD StorageClass，并在 StatefulSet 中引用。

```yaml
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-mysql-ssd
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  regionId: cn-hangzhou
  zoneId: cn-hangzhou-g
  diskType: cloud_essd
  performanceLevel: PL2
  fstype: ext4
reclaimPolicy: Retain
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
```

```yaml
# StatefulSet 片段
volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: alicloud-mysql-ssd
      resources:
        requests:
          storage: 500Gi
```

---

## 有状态应用迁移与扩容注意事项

| 操作 | 风险 | 建议 |
|:---|:---|:---|
| 扩容 PVC | 部分存储类不支持在线扩容 | 先确认 `allowVolumeExpansion` |
| 迁移 StorageClass | 需要重建 PVC 并复制数据 | 使用 Velero 或应用级工具迁移 |
| 更换节点 | 本地盘数据无法随 Pod 迁移 | 使用网络存储或提前迁移 |
| 缩容 StatefulSet | 数据仍保留在 PVC 中 | 确认不再需要后手动清理 |

---

## 专有云 Apsara Stack 存储选型建议

| 应用 | 专有云存储选项 | 说明 |
|:---|:---|:---|
| MySQL 主库 | 专有云 ESSD / 本地 SSD | 低延迟、高 IOPS |
| PostgreSQL | 专有云 ESSD / PolarDB | 按需选择托管或自建 |
| Kafka | 本地 SSD / ESSD | 顺序写性能优先 |
| Elasticsearch | ESSD PL2/PL3 | 索引吞吐 |
| Redis | 本地 SSD / ESSD | AOF 持久化低延迟 |


<!-- risk-assessed -->
