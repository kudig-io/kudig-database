---
title: openGemini (entities)
description: '## 概述'
summary: 'openGemini 是一个高性能、分布式时序数据库，专为物联网 (IoT)、可观测性和工业互联网场景设计。它基于 InfluxDB 协议兼容，提供高速写入、低延迟查询和高效压缩，支持每秒千万级数据点的写入和 PB 级数据存储。openGemini 采用存算分离架构，可独立扩展计算和存储资源。'
category: entities
tags:
- k8s
- cncf
- database
- opengemini
- coredns
- flux
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- openGemini 是什么
- 如何 openGemini
trigger_keywords:
- openGemini
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# openGemini

> **CNCF 状态**: Sandbox | **类别**: Database | **主要语言**: Go

## 概述

OpenGemini 是一个 CNCF 沙箱项目，由华为开源，是一个高性能的分布式时间序列数据库（TSDB）。它专为 IoT、监控、运维数据分析等大规模时间序列数据场景设计，提供高写入吞吐、低查询延迟和高效的数据压缩。OpenGemini 采用存算分离架构，支持水平扩展和云原生部署。与 InfluxDB 兼容的写入/查询 API，方便迁移。

## Key Features（核心能力）

- **高性能写入**：单节点支持百万级 TPS 写入
- **存算分离**：计算节点和存储节点独立扩展，灵活适应不同负载
- **InfluxDB 协议兼容**：兼容 InfluxDB v2 写入和查询 API
- **SQL+Flux 查询**：支持 SQL 和 Flux 两种查询语言
- **多维标签索引**：高效的倒排索引支持多维度标签查询
- **数据降采样**：自动数据下采样（Downsampling）管理长期数据

## 架构与工作原理

OpenGemini 采用三节点分离架构：SQL Node（ts-sql）处理查询请求和 SQL 解析；Store Node（ts-store）负责数据写入、存储和索引；Meta Node（ts-meta）管理集群元数据和分区路由。数据按时间分片（Shard）存储，通过 Raft 协议保证元数据一致性。存储引擎基于 LSM-Tree，结合列式存储和倒排索引实现高效的时间序列查询。

## K8s 集成

OpenGemini 可通过 Helm Chart 部署到 Kubernetes 集群。ts-sql 以 Deployment 部署，通过 Service 暴露查询接口；ts-store 以 StatefulSet 部署，使用 PVC 提供持久化存储；ts-meta 以 3 节点 StatefulSet 部署保证高可用。与 Prometheus Remote Read/Write 集成，作为长期监控数据存储后端。通过 HPA 根据查询负载自动扩展 ts-sql 节点。

## 生产用例

- **监控数据存储**：作为 Prometheus 的远程存储后端
- **IoT 数据平台**：海量 IoT 设备的时间序列数据采集和查询
- **运维数据分析**：大规模 IT 基础设施的性能和日志数据分析
- **金融数据分析**：股票行情、交易指标等时间序列数据管理

## 安装与配置

```bash
# 🟢 Helm 安装
helm repo add opengemini https://opengemini.github.io/opengemini-helm
helm install opengemini opengemini/opengemini \
  -n database --create-namespace \
  --set sql.replicaCount=3 \
  --set store.replicaCount=3 \
  --set meta.replicaCount=3

# 🟢 验证安装
kubectl get pods -n database
kubectl get svc -n database

# 🟢 测试连接
kubectl run og-client --image=opengemini/client --rm -it -- bash
# 在容器内:
# og-cli -host opengemini-sql.database.svc -port 8086 -database mydb

# 🟢 写入测试数据
curl -XPOST 'http://opengemini-sql.database.svc:8086/write?db=mydb' \
  --data-binary 'cpu,host=server01 usage_idle=95.0,usage_user=3.0 1465839830100400200'

# 🟢 查询测试
curl -G 'http://opengemini-sql.database.svc:8086/query?db=mydb' \
  --data-urlencode 'q=SELECT * FROM cpu WHERE host=\'server01\''
```

### Prometheus Remote Write 集成

```yaml
# prometheus.yml 配置
remote_write:
- url: http://opengemini-sql.database.svc:8086/api/v1/write?db=prometheus
  queue_config:
    max_samples_per_send: 10000
    batch_send_deadline: 5s

remote_read:
- url: http://opengemini-sql.database.svc:8086/api/v1/read?db=prometheus
  read_recent: false
```

### 数据保留策略

```sql
-- 创建数据库
CREATE DATABASE monitoring WITH DURATION 30d REPLICATION 3

-- 创建降采样策略
CREATE RETENTION POLICY "downsample_1h" ON "monitoring" DURATION 365d REPLICATION 1

-- 创建连续查询 (自动降采样)
CREATE CONTINUOUS QUERY "cq_cpu_1h" ON "monitoring"
BEGIN
  SELECT mean(usage_idle) AS usage_idle, mean(usage_user) AS usage_user
  INTO "downsample_1h"."cpu_1h"
  FROM "cpu"
  GROUP BY time(1h), "host"
END
```

## 运维操作

### 常用命令

```bash
# 🟢 查看组件状态
kubectl get pods -n database -l app=opengemini

# 🟢 查看 SQL Node 日志
kubectl logs -n database -l component=ts-sql --tail=50

# 🟢 查看 Store Node 日志
kubectl logs -n database -l component=ts-store --tail=50

# 🟢 查看 Meta Node 日志
kubectl logs -n database -l component=ts-meta --tail=50

# 🟢 查看数据库列表
curl 'http://opengemini-sql.database.svc:8086/query?q=SHOW+DATABASES'

# 🟢 查看测量列表
curl 'http://opengemini-sql.database.svc:8086/query?db=monitoring&q=SHOW+MEASUREMENTS'

# 🟢 查看分片信息
curl 'http://opengemini-sql.database.svc:8086/query?q=SHOW+SHARDS'

# 🟡 删除过期数据
curl -XPOST 'http://opengemini-sql.database.svc:8086/query?db=monitoring' \
  --data-urlencode 'q=DROP SERIES FROM cpu WHERE time < now() - 90d'

# 🟢 查看集群状态
curl 'http://opengemini-sql.database.svc:8086/debug/requests'
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 写入失败 | Store Node 不可用 | `kubectl logs -l component=ts-store` | 检查 Store Pod 状态和 PVC |
| 查询超时 | 数据量过大/索引缺失 | 查看 SQL Node 日志 | 优化查询/添加标签索引 |
| 集群不可用 | Meta Node 多数失败 | `kubectl get pods -l component=ts-meta` | 确保 Meta >= 3 副本 |
| 磁盘空间不足 | 数据保留策略未配置 | `kubectl exec -it <store-pod> -- df -h` | 配置 retention policy |
| 写入延迟高 | 分片过多/内存不足 | 查看 Store 指标 | 调整分片策略/增加内存 |

### 排查流程

```
1. kubectl get pods -n database → 确认组件状态
2. kubectl logs -l component=ts-sql → 查看查询层日志
3. kubectl logs -l component=ts-store → 查看存储层日志
4. kubectl logs -l component=ts-meta → 查看元数据日志
5. 检查 PVC 使用率和节点资源
```

## 生产案例

### 案例1: 大规模 IoT 数据平台
- **场景**: 10万+ IoT 设备每秒上报数据，需要实时查询和历史分析
- **方案**: openGemini 存算分离架构，10 Store Node + 5 SQL Node
- **效果**: 写入 TPS 达 500万/s，查询延迟 < 100ms

### 案例2: Prometheus 长期存储
- **场景**: 监控数据需保留 1年，Prometheus 本地存储不足
- **方案**: openGemini 作为 Remote Write/Read 后端，配置降采样
- **效果**: 存储成本降低 70%，查询性能满足 Grafana 展示需求

## 对比替代方案

| 维度 | openGemini | InfluxDB | TimescaleDB | VictoriaMetrics |
|------|-----------|----------|-------------|----------------|
| 架构 | 存算分离 | 单机/集群 | PostgreSQL 扩展 | 单机/集群 |
| 水平扩展 | 原生支持 | 企业版 | 有限 | 支持 |
| 协议兼容 | InfluxDB | 原生 | PostgreSQL | Prometheus |
| 查询语言 | SQL+Flux | InfluxQL+Flux | SQL | MetricsQL |
| 写入性能 | 极高 | 高 | 中 | 极高 |
| CNCF | Sandbox | 非 CNCF | 非 CNCF | 非 CNCF |

## 检查清单

- [ ] Meta Node 副本数 >= 3 (Raft 多数派)
- [ ] Store Node 使用高性能 SSD PVC
- [ ] 配置了数据保留策略 (retention policy)
- [ ] 配置了降采样策略 (长期数据)
- [ ] SQL Node 配置 HPA 应对查询峰值
- [ ] 监控写入 TPS、查询延迟、磁盘使用率
- [ ] 定期备份元数据

## Related

- [[notary-project]] — Notary Project
- [[coredns]] — CoreDNS
- [[contour]] — Contour
- [[kured]] — Kured (KUbernetes REboot Daemon)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- opengemini
- [[23-实体/12-数据与消息/oxia.md|Oxia]]
- [[23-实体/15-参考与索引/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
