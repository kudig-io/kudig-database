---
title: Kubernetes 上的 Redis 生产运维指南
description: 面向在 Kubernetes 上运行 Redis 的 SRE 指南，覆盖 HA 拓扑（Sentinel/Cluster）、持久化、备份、网络策略、资源 QoS、监控与故障转移
summary: Kubernetes 上 Redis 生产运维指南，覆盖 Sentinel/Cluster 高可用拓扑、AOF/RDB 持久化、备份恢复、NetworkPolicy、资源 QoS、监控告警与故障转移演练。
category: database-middleware
tags:
- production
- best-practices
- playbook
- database-middleware
- redis
- kubernetes
- sentinel
- cluster
- persistence
- backup
- network-policy
- qos
- monitoring
- failover
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- DBA
estimated_read_time: 30min
intent_queries:
- Kubernetes 上的 Redis 生产运维指南是什么
- 如何运维 Kubernetes 上的 Redis
- Redis Sentinel Cluster 持久化 备份 网络策略 资源 QoS 监控 故障转移 最佳实践
trigger_keywords:
- Redis
- Kubernetes Redis
- Redis Sentinel
- Redis Cluster
- Redis 持久化
- Redis 备份
- Redis 故障转移
prerequisites:
- kubectl-basics
- redis-basics
- helm-basics
- storage-basics
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes 上的 Redis 生产运维指南

> **适用范围**: Redis 6.x / 7.x on Kubernetes v1.28-v1.33。  
> **目标读者**: SRE、平台工程师、DBA。  
> **维护状态**: 持续更新 | **风险等级**: 高 — 涉及有状态数据面。

本指南面向在 Kubernetes 上运行 Redis 的 SRE 与平台工程师，覆盖高可用拓扑选型（Sentinel / Cluster）、持久化策略、备份恢复、网络策略、资源 QoS、监控告警与故障转移。目标是让 Redis 在 K8s 生产环境中具备自愈能力、可观测性与可控的运维节奏。

---

## 1. 适用场景与范围

- 需要在 Kubernetes 上部署生产级 Redis 缓存/数据库。
- 需要选择 Sentinel 或 Cluster 拓扑。
- 需要配置 AOF/RDB 持久化与备份策略。
- 需要通过网络策略限制 Redis 访问面。
- 需要定义资源 QoS 与故障转移流程。
- 需要建立 Redis 监控告警与容量规划基线。

---

## 2. 前置条件与工具

- 已部署 Redis Operator 或 Helm Chart（如 Bitnami Redis、Redis Cluster、OT-Container-Kit/redis-operator）。
- 已配置 StorageClass 并支持 VolumeSnapshot。
- 已部署 Prometheus + Grafana（建议 redis_exporter）。
- 已安装 `redis-cli` 用于诊断。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Redis Operator
kubectl get deployment -n operators -l app.kubernetes.io/name=redis-operator

# 检查 StorageClass
kubectl get sc

# 检查 redis-cli
redis-cli --version
```
---

## 3. 核心概念/架构

```
Redis HA 拓扑对比:
┌─────────────────────────────────────────────────────────┐
│  Sentinel 模式                                          │
│  - 1 主 + N 从 + 3 Sentinel（推荐 3）                   │
│  - 自动故障转移，读写分离                                │
│  - 适合数据量 < 100GB、需要强一致读的场景                │
├─────────────────────────────────────────────────────────┤
│  Cluster 模式                                           │
│  - 多主多从，数据分片（16384 slots）                     │
│  - 水平扩展，适合大数据量与高并发                         │
│  - 最少 3 主 3 从，客户端需支持 cluster protocol         │
│  - 多 key 操作、事务需在同一 slot                        │
└─────────────────────────────────────────────────────────┘
```

- **持久化**: AOF（everysec）保证较高持久性；RDB 提供点时间快照；建议两者结合。
- **Cluster Bus**: Cluster 节点间通过端口 16379 通信，必须放行。
- **Sentinel**: 客户端需支持 Sentinel 协议，由 Sentinel 返回当前 master 地址。

### 3.1 拓扑选型建议

| 维度 | Sentinel | Cluster |
|---|---|---|
| 数据规模 | < 100GB | > 100GB 或需要水平扩展 |
| 分片需求 | 否 | 是 |
| 客户端复杂度 | 低 | 高 |
| 一致性 | 最终一致 | 最终一致 |
| 推荐副本数 | 1 主 2 从 + 3 Sentinel | 3 主 3 从 |

---

## 4. 标准操作流程

### 4.1 部署 Redis Sentinel（Helm 示例）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建命名空间与凭据 Secret
kubectl create namespace redis-prod
kubectl create secret generic redis-credentials \
  --from-literal=redis-password='<strong-password>' -n redis-prod

helm upgrade --install redis bitnami/redis \
  --namespace redis-prod --create-namespace \
  --set architecture=replication \
  --set auth.enabled=true \
  --set auth.existingSecret=redis-credentials \
  --set sentinel.enabled=true \
  --set sentinel.quorum=2 \
  --set replica.replicaCount=3 \
  --set persistence.enabled=true \
  --set persistence.storageClass=fast-ssd \
  --set persistence.size=20Gi \
  --set metrics.enabled=true \
  --set metrics.serviceMonitor.enabled=true
```
### 4.2 部署 Redis Cluster

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm upgrade --install redis-cluster bitnami/redis-cluster \
  --namespace redis-prod --create-namespace \
  --set cluster.nodes=6 \
  --set cluster.replicas=1 \
  --set auth.enabled=true \
  --set auth.existingSecret=redis-credentials \
  --set persistence.enabled=true \
  --set persistence.storageClass=fast-ssd \
  --set persistence.size=50Gi \
  --set metrics.enabled=true \
  --set metrics.serviceMonitor.enabled=true

# 查看分片与主从分布
kubectl exec -it redis-cluster-0 -n redis-prod -- redis-cli -a '<password>' CLUSTER NODES
```
### 4.3 持久化配置

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看当前持久化配置
kubectl exec -it redis-master-0 -n redis-prod -- redis-cli CONFIG GET appendonly
kubectl exec -it redis-master-0 -n redis-prod -- redis-cli CONFIG GET save

# 推荐配置：AOF everysec + RDB 快照
kubectl apply -f - <<EOF
apiVersion: v1
kind: ConfigMap
metadata:
  name: redis-config
  namespace: redis-prod
data:
  redis.conf: |
    appendonly yes
    appendfsync everysec
    no-appendfsync-on-rewrite yes
    save 900 1
    save 300 10
    save 60 10000
    maxmemory 16gb
    maxmemory-policy allkeys-lru
    slowlog-log-slower-than 10000
    slowlog-max-len 128
EOF
```
AOF everysec 在大多数场景下能在性能与持久性之间取得平衡；若磁盘延迟较高，可评估 no-appendfsync-on-rewrite 与 SSD 升级。

### 4.4 备份（RDB + VolumeSnapshot）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 BGSAVE 生成 RDB
kubectl exec -it redis-master-0 -n redis-prod -- redis-cli BGSAVE

# 等待保存完成
kubectl exec -it redis-master-0 -n redis-prod -- redis-cli INFO persistence | grep rdb_bgsave_in_progress

# 创建 PVC 快照
kubectl apply -f - <<EOF
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: redis-master-snap-$(date +%Y%m%d)
  namespace: redis-prod
spec:
  volumeSnapshotClassName: csi-snapclass
  source:
    persistentVolumeClaimName: redis-data-redis-master-0
EOF

# 使用 Velero 备份命名空间
velero backup create redis-prod-backup --include-namespaces redis-prod --ttl 720h0m0s

# 建议将 RDB 文件定期上传对象存储
aws s3 cp /var/lib/redis/dump.rdb s3://my-redis-backups/redis-prod/
```
### 4.5 网络策略

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: redis-default-deny
  namespace: redis-prod
spec:
  podSelector: {}
  policyTypes:
  - Ingress
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: redis-allow-app
  namespace: redis-prod
spec:
  podSelector:
    matchLabels:
      app.kubernetes.io/name: redis
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: prod
    ports:
    - protocol: TCP
      port: 6379
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: redis-allow-cluster-bus
  namespace: redis-prod
spec:
  podSelector:
    matchLabels:
      app.kubernetes.io/name: redis-cluster
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app.kubernetes.io/name: redis-cluster
    ports:
    - protocol: TCP
      port: 16379
EOF
```
### 4.6 资源 QoS

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: redis-master
  namespace: redis-prod
spec:
  template:
    spec:
      containers:
      - name: redis
        resources:
          requests:
            cpu: "2"
            memory: 16Gi
          limits:
            cpu: "4"
            memory: 32Gi
        # 推荐 Guaranteed QoS：requests == limits
```

### 4.7 监控告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: redis-alerts
  namespace: monitoring
spec:
  groups:
  - name: redis
    rules:
    - alert: RedisDown
      expr: redis_up{namespace="redis-prod"} == 0
      for: 1m
      labels:
        severity: critical
    - alert: RedisMemoryHigh
      expr: |
        redis_memory_used_bytes{namespace="redis-prod"} /
        redis_memory_max_bytes{namespace="redis-prod"} > 0.85
      for: 5m
      labels:
        severity: warning
    - alert: RedisReplicationLag
      expr: |
        redis_master_repl_offset{namespace="redis-prod"} -
        redis_slave_offset{namespace="redis-prod"} > 1000000
      for: 5m
      labels:
        severity: warning
    - alert: RedisRejectedConnections
      expr: redis_rejected_connections_total{namespace="redis-prod"} > 0
      for: 1m
      labels:
        severity: critical
```

### 4.8 故障转移演练

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看当前主从关系
kubectl exec -it redis-master-0 -n redis-prod -- redis-cli INFO replication

# 手动触发切换（Sentinel 模式）
kubectl exec -it redis-node-0 -n redis-prod -- redis-cli -p 26379 SENTINEL failover mymaster

# 观察新主节点选举
kubectl exec -it redis-node-0 -n redis-prod -- redis-cli -p 26379 SENTINEL get-master-addr-by-name mymaster

# Cluster 模式：查看 slots 与 master 分布
kubectl exec -it redis-cluster-0 -n redis-prod -- redis-cli CLUSTER SLOTS

# Cluster 重新分片（扩容后）
kubectl exec -it redis-cluster-0 -n redis-prod -- redis-cli --cluster rebalance redis-cluster-0.redis-cluster-headless:6379
```
### 4.9 内存与热 key 治理

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看大 key
kubectl exec -it redis-master-0 -n redis-prod -- redis-cli --bigkeys

# 查看热 key（需启用 redis-cli --hotkeys，要求 maxmemory-policy 为 allkeys-lfu 或 volatile-lfu）
kubectl exec -it redis-master-0 -n redis-prod -- redis-cli --hotkeys

# 设置合理 TTL 与最大内存策略
kubectl exec -it redis-master-0 -n redis-prod -- redis-cli CONFIG SET maxmemory-policy allkeys-lru
```
---

## 5. 关键检查点与验证命令

| 检查项 | 验证命令 | 通过标准 |
|---|---|---|
| 集群拓扑 | `kubectl get pods -n redis-prod -o wide` | 主从/分片分布跨 AZ |
| 主从复制 | `redis-cli INFO replication` | master_link_status: up |
| 持久化状态 | `redis-cli INFO persistence` | aof_enabled:1 / rdb_bgsave_in_progress:0 |
| 内存使用 | `redis-cli INFO memory` | used_memory < maxmemory 80% |
| 网络策略 | `kubectl get networkpolicy -n redis-prod` | default-deny + allow-app 存在 |
| 备份成功 | `kubectl get volumesnapshot -n redis-prod` | 最近 24h 有快照 |
| 告警规则 | `kubectl get prometheusrules redis-alerts` | 规则存在 |
| 连接数 | `redis-cli INFO clients` | connected_clients < maxclients 80% |
| Cluster 健康 | `redis-cli CLUSTER INFO \| grep cluster_state` | cluster_state:ok |

---

## 6. 常见故障与 remediation

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|---|---|---|---|
| Redis 主从切换失败 | Sentinel 数量不足或网络分区 | `redis-cli -p 26379 SENTINEL master mymaster` | 检查 Sentinel 配置与网络 |
| 内存使用突增 | 热 key / 大 key / 缓存穿透 | `redis-cli --bigkeys` / `INFO memory` | 优化数据结构、设置 TTL、限流 |
| 持久化阻塞 | BGSAVE / AOF rewrite 占用大量 CPU/IO | `redis-cli INFO persistence` | 调整 save 策略、升级磁盘 |
| Cluster 节点通信失败 | 网络策略或 DNS 解析异常 | `redis-cli CLUSTER NODES` | 放行 cluster bus 端口 16379 |
| 从库复制延迟高 | 网络抖动 / 从库 IO 饱和 | `redis-cli INFO replication` | 扩容从库、优化网络 |
| 连接数打满 | 应用未使用连接池 | `redis-cli INFO clients` | 应用端启用连接池、提升 maxclients |
| Pod 反复 OOMKilled | 内存 limits 过低 | `kubectl describe pod` | 提升 limits 或调整 maxmemory |
| Cluster slot 不均衡 | 节点加入后未重新分片 | `redis-cli CLUSTER SLOTS` | 执行 CLUSTER REBALANCE |

---

## 7. 风险与注意事项

- **数据丢失风险**: Redis 默认异步复制，故障切换可能丢失部分数据，关键业务需评估是否启用 WAIT 命令或持久化策略。
- **Cluster 模式限制**: CLUSTER 命令、多 key 操作、事务跨 slot 受限，应用选型前需确认兼容性。
- **持久化性能影响**: AOF everysec 在磁盘延迟高时可能阻塞，生产环境应使用 SSD/NVMe。
- **网络策略**: 务必放行 cluster bus 端口（16379）和 Sentinel 端口（26379）。
- **资源 QoS**: Redis 对 CPU 调度敏感，关键实例建议 Guaranteed QoS 与独立节点池。
- **备份一致性**: BGSAVE 生成的 RDB 是点时间快照，建议在低峰期执行，避免与 AOF rewrite 冲突。
- **密码管理**: 避免在命令行明文传递密码，优先使用 Secret 与 redis.conf 挂载。


## 4.15 持久化策略选择

Redis 提供 RDB 和 AOF 两种持久化方式，生产环境通常结合使用：

- **RDB**: 时点快照，恢复速度快，但可能丢失两次快照间的数据。适合对数据丢失有一定容忍度的缓存场景。
- **AOF**: 日志追加，数据安全性更高，但文件较大、恢复速度较慢。`appendfsync everysec` 是性能与安全的折中。
- **混合模式**: Redis 4.0+ 支持 RDB + AOF 混合持久化，AOF 重写时生成 RDB 前缀，兼顾恢复速度与数据安全。

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 AOF 重写状态
kubectl exec -it redis-master-0 -n redis-prod -- redis-cli INFO persistence | grep aof_rewrite

# 手动触发 AOF 重写
kubectl exec -it redis-master-0 -n redis-prod -- redis-cli BGREWRITEAOF
```
## 4.16 运维自动化

大规模 Redis 运维应借助 Operator 和自动化脚本：

- 使用 Redis Operator 管理集群生命周期、故障转移和备份。
- 使用 PrometheusRule 和 Alertmanager 实现告警自动化。
- 使用 CronJob 定期执行 BGSAVE 和备份上传。
- 使用 GitOps 管理 Redis 配置，避免手动修改。

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 CronJob 自动备份 RDB
kubectl create cronjob redis-backup --image=redis:7 --schedule="0 2 * * *"   --namespace redis-prod --   sh -c 'redis-cli BGSAVE && aws s3 cp /data/dump.rdb s3://my-redis-backups/'
```
---

## 8. 相关 Runbook / 推荐阅读

- [[数据库中间件/99-production-readiness-operations-guide.md|Database & Middleware 生产就绪运维指南]]
- [[数据库中间件/数据库/06-redis-enterprise-cache.md|Redis 企业级缓存]]
- [[数据库中间件/数据库/07-redis-kubernetes-operator.md|Redis Kubernetes Operator]]
- [[数据库中间件/缓存/01-redis-cluster-sentinel-topology.md|Redis Cluster/Sentinel 拓扑]]
- [[存储/99-production-readiness-operations-guide.md|存储与数据生产就绪运维指南]]
- [[安全/99-production-readiness-operations-guide.md|安全与合规生产就绪运维指南]]
- [[可观测性/99-production-readiness-operations-guide.md|可观测性生产就绪运维指南]]

---

*本指南聚焦 Kubernetes 上的 Redis 生产运维，实际执行前请结合具体 Operator/Chart 版本与业务数据一致性要求进行裁剪。*


<!-- risk-assessed -->
