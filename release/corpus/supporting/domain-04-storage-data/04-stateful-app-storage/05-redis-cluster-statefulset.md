---
title: Redis Cluster on K8s 生产部署指南
description: 面向阿里云/专有云 K8s 的 Redis Cluster 生产部署方案，涵盖槽位分配、故障转移、数据迁移、持久化与监控告警。
summary: 面向阿里云/专有云 K8s 的 Redis Cluster 生产部署方案，涵盖槽位分配、故障转移、数据迁移、持久化与监控告警。
category: storage
tags:
- k8s
- statefulset
- redis
- redis-cluster
- slot
- failover
- alicloud
- apsara-stack
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 缓存运维
- 应用架构师
estimated_read_time: 25min
intent_queries:
- Redis Cluster StatefulSet 生产部署
- K8s 上 Redis Cluster 槽位与故障转移
- 阿里云 K8s Redis Cluster 数据迁移
trigger_keywords:
- Redis
- Redis Cluster
- slot
- failover
- 数据迁移
- 持久化
prerequisites:
- kubectl-basics
- statefulset-basics
- redis-basics
- storage-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---



# Redis Cluster on K8s 生产部署指南

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，讲解 Redis Cluster 在 StatefulSet 上的生产部署、槽位管理、故障转移与数据迁移。

## 目录

1. [架构选型](#架构选型)
2. [StatefulSet 部署 Redis Cluster](#statefulset-部署-redis-cluster)
3. [集群初始化与槽位分配](#集群初始化与槽位分配)
4. [故障转移机制](#故障转移机制)
5. [数据迁移与再平衡](#数据迁移与再平衡)
6. [持久化与备份](#持久化与备份)
7. [监控告警](#监控告警)
8. [常见问题与优化](#常见问题与优化)
9. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. 架构选型

### 1.1 Redis 部署模式对比

Redis 在 Kubernetes 上常见的部署模式包括单实例、主从 + Sentinel 和 Redis Cluster。选择合适的模式需要综合考虑数据规模、可用性要求和扩展性需求。

| 模式 | 副本数 | 自动切换 | 扩展性 | 适用场景 |
|:---|:---:|:---:|:---:|:---|
| 单实例 | 1 | 否 | 无 | 开发测试 |
| 主从 + Sentinel | 1 主 + 2 从 | 是 | 无 | 中小规模缓存 |
| Redis Cluster | 6+（3 主 3 从） | 是 | 水平扩展 | 大规模缓存 |
| 阿里云 Tair/Redis | 托管 | 是 | 强 | 核心生产优先 |

### 1.2 Redis Cluster 核心概念

Redis Cluster 通过分片机制实现数据水平扩展，理解以下概念对部署和运维至关重要：

| 概念 | 说明 |
|:---|:---|
| 槽位 slot | 共 16384 个，每个 key 通过 CRC16 映射到 slot |
| 主节点 | 负责处理 slot 范围的读写 |
| 从节点 | 复制主节点，主故障时提升为主 |
| cluster-require-full-coverage | 默认 yes，部分槽位不可用时集群停止服务 |
| Gossip 协议 | 节点间通过 Gossip 交换状态信息 |

---

## 2. StatefulSet 部署 Redis Cluster

### 2.1 Headless Service

Redis Cluster 节点需要通过稳定的 DNS 名称互相发现，因此使用 Headless Service。

```yaml
apiVersion: v1
kind: Service
metadata:
  name: redis-cluster
  namespace: production
spec:
  clusterIP: None
  selector:
    app: redis-cluster
  ports:
    - port: 6379
      name: redis
    - port: 16379
      name: cluster
```

### 2.2 ConfigMap

Redis Cluster 需要开启集群模式、配置集群端口、持久化策略和内存限制。

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: redis-cluster-config
  namespace: production
data:
  redis.conf: |
    port 6379
    cluster-enabled yes
    cluster-config-file nodes.conf
    cluster-node-timeout 5000
    appendonly yes
    appendfsync everysec
    dir /data
    maxmemory 6gb
    maxmemory-policy allkeys-lru
    save ""
    bind 0.0.0.0
    protected-mode no
```

### 2.3 StatefulSet

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: redis-cluster
  namespace: production
spec:
  serviceName: redis-cluster
  replicas: 6
  selector:
    matchLabels:
      app: redis-cluster
  template:
    metadata:
      labels:
        app: redis-cluster
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
                        - redis-cluster
                topologyKey: topology.kubernetes.io/zone
      containers:
        - name: redis
          image: redis:7.2
          command:
            - redis-server
            - /usr/local/etc/redis/redis.conf
          ports:
            - containerPort: 6379
              name: redis
            - containerPort: 16379
              name: cluster
          resources:
            requests:
              cpu: "2"
              memory: "8Gi"
            limits:
              cpu: "4"
              memory: "8Gi"
          volumeMounts:
            - name: data
              mountPath: /data
            - name: config
              mountPath: /usr/local/etc/redis
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: openebs-lvmpv-local-ssd
        resources:
          requests:
            storage: 100Gi
```

---

## 3. 集群初始化与槽位分配

### 3.1 初始化集群

等 6 个 Pod 全部 Running 后，执行 `redis-cli --cluster create`：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 收集所有 Pod 的地址
PODS=$(kubectl get pods -n production -l app=redis-cluster -o jsonpath='{range .items[*]}{.metadata.name}.redis-cluster.production.svc.cluster.local:6379 {end}')
echo $PODS

# 初始化集群：前 3 个为主，后 3 个为从
kubectl exec -it redis-cluster-0 -n production -- redis-cli --cluster create \
  redis-cluster-0.redis-cluster.production.svc.cluster.local:6379 \
  redis-cluster-1.redis-cluster.production.svc.cluster.local:6379 \
  redis-cluster-2.redis-cluster.production.svc.cluster.local:6379 \
  redis-cluster-3.redis-cluster.production.svc.cluster.local:6379 \
  redis-cluster-4.redis-cluster.production.svc.cluster.local:6379 \
  redis-cluster-5.redis-cluster.production.svc.cluster.local:6379 \
  --cluster-replicas 1 --cluster-yes
```

### 3.2 查看集群状态

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看节点、槽位、主从关系
kubectl exec -it redis-cluster-0 -n production -- redis-cli cluster nodes

# 查看集群整体健康状态
kubectl exec -it redis-cluster-0 -n production -- redis-cli cluster info
```

---

## 4. 故障转移机制

### 4.1 自动故障转移

Redis Cluster 使用 gossip 协议检测节点状态。当主节点超时未响应时，从节点会发起选举并成为新主节点。选举过程需要多数主节点参与投票，因此集群至少需要 3 个主节点。

### 4.2 模拟故障转移

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 查看当前主节点
kubectl exec -it redis-cluster-0 -n production -- redis-cli cluster nodes

# 2. 删除一个主节点 Pod
kubectl delete pod redis-cluster-1 -n production

# 3. 等待一段时间后再次查看，原从节点应提升为主
kubectl exec -it redis-cluster-0 -n production -- redis-cli cluster nodes
```

### 4.3 手动故障转移

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 在从节点上执行手动切换，切换期间几乎无中断
kubectl exec -it redis-cluster-4 -n production -- redis-cli CLUSTER FAILOVER
```

---

## 5. 数据迁移与再平衡

### 5.1 扩容集群

从 6 节点扩容到 8 节点：

```bash
kubectl scale sts redis-cluster -n production --replicas=8
```

新节点加入集群：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -it redis-cluster-0 -n production -- redis-cli --cluster add-node \
  redis-cluster-6.redis-cluster.production.svc.cluster.local:6379 \
  redis-cluster-0.redis-cluster.production.svc.cluster.local:6379
```

### 5.2 重新分配槽位

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 执行重分配，将所有槽位重新平衡
kubectl exec -it redis-cluster-0 -n production -- redis-cli --cluster reshard \
  redis-cluster-0.redis-cluster.production.svc.cluster.local:6379 \
  --cluster-from all \
  --cluster-to <new-node-id> \
  --cluster-slots 4096 \
  --cluster-yes
```

### 5.3 缩容前迁移槽位

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 将下线节点的槽位迁出
kubectl exec -it redis-cluster-0 -n production -- redis-cli --cluster reshard \
  redis-cluster-0.redis-cluster.production.svc.cluster.local:6379 \
  --cluster-from <node-id-to-remove> \
  --cluster-to <target-node-id> \
  --cluster-slots 5461 \
  --cluster-yes

# 移除节点
kubectl exec -it redis-cluster-0 -n production -- redis-cli --cluster del-node \
  redis-cluster-0.redis-cluster.production.svc.cluster.local:6379 <node-id-to-remove>
```

---

## 6. 持久化与备份

### 6.1 AOF 与 RDB 策略

生产环境推荐 AOF + RDB 双保险：

| 策略 | 配置 | 说明 |
|:---|:---|:---|
| AOF | `appendonly yes`、`appendfsync everysec` | 每秒刷盘，数据更可靠 |
| RDB | `save ""` 关闭定时 RDB | 由 AOF rewrite 触发 |

### 6.2 定时备份 AOF 到 OSS

```yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: redis-aof-backup
  namespace: production
spec:
  schedule: "0 3 * * *"
  concurrencyPolicy: Forbid
  jobTemplate:
    spec:
      template:
        spec:
          containers:
            - name: backup
              image: redis:7.2
              command:
                - /bin/sh
                - -c
                - |
                  DATE=$(date +%Y%m%d)
                  kubectl cp production/redis-cluster-0:/data/appendonly.aof /tmp/redis-${DATE}.aof
                  ossutil cp /tmp/redis-${DATE}.aof oss://redis-backup-bucket/production/
                  rm -f /tmp/redis-${DATE}.aof
          restartPolicy: OnFailure
```

---

## 7. 监控告警

### 7.1 关键指标

| 指标 | 告警阈值 | 说明 |
|:---|:---|:---|
| redis_cluster_state | != 1 | 集群状态异常 |
| redis_cluster_slots_fail | > 0 | 槽位失败 |
| redis_cluster_nodes_fail | > 0 | 节点故障 |
| redis_memory_used_bytes / redis_memory_max_bytes | > 85% | 内存使用率 |
| redis_keyspace_hits / (hits + misses) | < 90% | 命中率下降 |
| redis_connected_clients | 突增 | 连接数异常 |

### 7.2 PrometheusRule

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: redis-cluster-alerts
  namespace: monitoring
spec:
  groups:
    - name: redis-cluster.rules
      rules:
        - alert: RedisClusterDown
          expr: redis_cluster_state{job="redis-cluster"} != 1
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "Redis Cluster 状态异常"
        - alert: RedisClusterNodeFail
          expr: redis_cluster_nodes_fail{job="redis-cluster"} > 0
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "Redis Cluster 存在故障节点"
        - alert: RedisMemoryHigh
          expr: |
            redis_memory_used_bytes / redis_memory_max_bytes > 0.85
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Redis 内存使用率超过 85%"
```

---

## 8. 常见问题与优化

### 8.1 集群状态 fail

当部分槽位不可用时，集群可能进入 fail 状态。排查方法：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看具体失败的槽位
kubectl exec -it redis-cluster-0 -n production -- redis-cli cluster info

# 查看节点状态
kubectl exec -it redis-cluster-0 -n production -- redis-cli cluster nodes
```

常见原因包括节点宕机、网络分区或槽位未分配。解决方法包括重启故障节点、修复网络或重新分配槽位。

### 8.2 内存使用率高

Redis 是内存数据库，内存使用率过高会导致 OOM 或性能下降。优化建议：
- 设置合理的 `maxmemory` 和 `maxmemory-policy`。
- 定期清理过期 key。
- 对大 key 进行拆分或压缩。
- 监控内存碎片率，必要时重启节点。

### 8.3 连接数突增

连接数突增可能由应用连接池配置不当或客户端异常引起。排查方法：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看连接数
kubectl exec -it redis-cluster-0 -n production -- redis-cli INFO clients
```

优化建议：
- 应用端使用连接池。
- 设置 `timeout` 自动关闭空闲连接。
- 限制单个客户端的连接数。

---

---

## 9. 阿里云/专有云生产实践

### 9.1 内存与持久化平衡

Redis 是内存数据库，所有数据默认存储在内存中。在阿里云/专有云环境中，应根据业务数据量合理选择节点规格。建议每个 Redis 节点的内存使用率长期保持在 80% 以下，预留足够的缓冲空间应对流量突增。同时，开启 AOF 持久化以保障数据可靠性，但对于纯缓存场景，可以适当放宽持久化策略以提升性能。

### 9.2 跨可用区部署

Redis Cluster 的节点应分布在多个可用区，以提升集群的整体可用性。建议每个可用区至少部署一个主节点和一个从节点，确保单个可用区故障时，集群仍然能够正常提供服务。在专有云环境中，需要提前确认各可用区之间的网络延迟和带宽是否满足 Redis Cluster 的通信要求。

### 9.3 数据备份与迁移

虽然 Redis 主要用于缓存，但部分业务可能将 Redis 作为持久化存储使用。建议对关键 Redis 数据定期进行 RDB 或 AOF 备份，并存储到 OSS。在进行集群扩容、缩容或数据迁移时，务必先在测试环境验证迁移脚本和流程，避免生产环境数据丢失或服务中断。

---

## 9. 最佳实践检查清单

| 检查项 | 要求 | 验证命令 |
|:---|:---|:---|
| 节点数 | 6+（3 主 3 从） | `kubectl get sts redis-cluster` |
| 槽位全部分配 | 16384 个 | `redis-cli cluster info` |
| 每个主节点有从节点 | 副本因子 >= 1 | `redis-cli cluster nodes` |
| 跨可用区 | Pod 反亲和性 zone | `kubectl get pod -o wide` |
| AOF 持久化开启 | appendonly=yes | 查看 ConfigMap |
| 内存上限设置 | maxmemory 配置 | `CONFIG GET maxmemory` |
| 备份任务 | 每日成功 | `kubectl get cj` |
| 故障转移演练 | 每季度一次 | 演练报告 |

---

## Redis Cluster 运维常用命令速查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看集群节点与槽位
kubectl exec -it -n production redis-cluster-0 -- redis-cli cluster nodes

# 查看集群信息
kubectl exec -it -n production redis-cluster-0 -- redis-cli cluster info

# 查看 key 分布
kubectl exec -it -n production redis-cluster-0 -- redis-cli --cluster info   redis-cluster-0.redis-cluster-headless.production.svc.cluster.local:6379

# 手动故障转移
kubectl exec -it -n production redis-cluster-3 -- redis-cli CLUSTER FAILOVER

# 添加新节点到集群
kubectl exec -it -n production redis-cluster-0 -- redis-cli --cluster add-node   redis-cluster-6.redis-cluster-headless.production.svc.cluster.local:6379   redis-cluster-0.redis-cluster-headless.production.svc.cluster.local:6379
```

### Redis 与阿里云产品集成

| 阿里云产品 | 用途 |
|:---|:---|
| 阿里云 Redis 企业版 | 托管高可用缓存 |
| 阿里云 Tair | 自研高性能内存数据库 |
| 云数据库 Redis 集群版 | 大容量缓存场景 |
| 云监控 CMS | Redis 监控告警 |

## Related

- [[domain-04-storage-data/04-stateful-app-storage/01-stateful-app-storage-patterns.md|有状态应用 Kubernetes 存储模式]]
- [[domain-04-storage-data/01-k8s-storage/08-storage-performance-tuning.md|存储性能调优]]

## See Also

- [[domain-06-observability/02-metrics/01-prometheus-enterprise-monitoring.md|Prometheus 企业监控]]
- [[domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/21-statefulset-troubleshooting.md|StatefulSet 故障诊断]]
