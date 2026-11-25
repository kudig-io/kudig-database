---
title: Memcached 分布式缓存部署
description: 'Memcached on K8s 部署、一致性哈希、与 Redis 对比、多线程配置、监控指标、客户端优化'
summary: 'Memcached on K8s 部署、一致性哈希、与 Redis 对比、多线程配置、监控指标、客户端优化'
category: database-middleware
tags:
- database
- k8s
- memcached
- cache
- distributed
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- DBA
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Memcached 分布式缓存部署 是什么
- 如何 Memcached 分布式缓存部署
trigger_keywords:
- memcached
- 分布式缓存
- 一致性哈希
- 多线程
prerequisites:
- kubectl-basics
- database-basics
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Memcached 分布式缓存部署

## 1. 架构概述

```
┌──────────────────────────────────────────────────────────────────┐
│                        Application                               │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              客户端一致性哈希                              │   │
│  │  key → CRC32 → hash ring → 目标节点                      │   │
│  └──────────────────────────────────────────────────────────┘   │
└──────────────────────────┬───────────────────────────────────────┘
                           │
         ┌─────────────────┼─────────────────┐
         ▼                 ▼                 ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│ Memcached-0  │  │ Memcached-1  │  │ Memcached-2  │
│   4GB RAM    │  │   4GB RAM    │  │   4GB RAM    │
│   Port 11211 │  │   Port 11211 │  │   Port 11211 │
└──────────────┘  └──────────────┘  └──────────────┘
   (无副本、无持久化、纯内存缓存)
```

### 1.1 核心特点

- **简单高效**: 单线程事件驱动模型（现代版本支持多线程）
- **无持久化**: 纯内存存储，重启数据丢失
- **无副本**: 节点独立，无复制机制
- **客户端路由**: 一致性哈希由客户端实现
- **Slab 分配**: 预分配内存块，减少碎片

## 2. Kubernetes 部署

### 2.1 StatefulSet 部署

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: memcached
  namespace: cache
spec:
  serviceName: memcached-headless
  replicas: 6
  selector:
    matchLabels:
      app: memcached
  template:
    metadata:
      labels:
        app: memcached
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "9150"
    spec:
      containers:
      - name: memcached
        image: memcached:1.6.24
        ports:
        - containerPort: 11211
          name: memcached
        command:
        - memcached
        - -m 4096
        - -p 11211
        - -u memcache
        - -l 0.0.0.0
        - -c 10000
        - -t 4
        - -o modern
        - -v
        resources:
          requests:
            cpu: "2"
            memory: 4500Mi
          limits:
            cpu: "4"
            memory: 5Gi
        livenessProbe:
          tcpSocket:
            port: memcached
          initialDelaySeconds: 10
          periodSeconds: 10
        readinessProbe:
          exec:
            command:
            - sh
            - -c
            - echo "stats" | nc localhost 11211 | grep -q "STAT pid"
          initialDelaySeconds: 5
          periodSeconds: 5
      - name: exporter
        image: prom/memcached-exporter:v0.14.2
        ports:
        - containerPort: 9150
          name: metrics
        args:
        - --memcached.address=localhost:11211
        resources:
          requests:
            cpu: 100m
            memory: 64Mi
          limits:
            cpu: 200m
            memory: 128Mi
---
apiVersion: v1
kind: Service
metadata:
  name: memcached-headless
  namespace: cache
spec:
  clusterIP: None
  ports:
  - port: 11211
    name: memcached
  selector:
    app: memcached
---
apiVersion: v1
kind: Service
metadata:
  name: memcached
  namespace: cache
spec:
  clusterIP: None
  ports:
  - port: 11211
    name: memcached
  selector:
    app: memcached
```

### 2.2 配置参数说明

| 参数 | 默认值 | 推荐值 | 说明 |
|------|-------|-------|------|
| -m | 64MB | 按需 | 最大内存 (MB) |
| -c | 1024 | 10000 | 最大连接数 |
| -t | 1 | 4-8 | 工作线程数 |
| -o | - | modern | 启用现代优化 |
| -f | 1.25 | 1.25 | slab 增长因子 |
| -n | 48 | 按需 | 最小分配空间 |
| -I | 1MB | 按需 | 最大 item 大小 |
| -b | 1024 | 1024 | 监听队列长度 |

### 2.3 Slab 分配器配置

```
Slab 内存分配策略:

chunk_size = base_size × (factor ^ slab_id)
base_size = 80 bytes (默认)
factor = 1.25 (默认)

示例 (4GB 内存):
  Slab 1:  96B chunks    → 适合小对象
  Slab 2:  120B chunks
  Slab 3:  152B chunks
  ...
  Slab 40: 1MB chunks    → 适合大对象

调整因子:
  如果大量对象落在同一个 slab → 降低 factor
  如果 slab 碎片严重 → 增大 factor
```

## 3. 一致性哈希算法

### 3.1 原理

```
一致性哈希环 (0 ~ 2^32 - 1):

                    0
                    │
          Node C ───┤
         ╱          │
        ╱           │
  ─────●────────────┼────────────●─────
      ╱             │             ╲
     ╱              │              ╲
  Node A ────       │        ──── Node B
                    │
                  2^32

节点映射: 每个物理节点映射 N 个虚拟节点到环上
Key 路由: CRC32(key) → 环上顺时针找到的第一个节点

虚拟节点作用: 解决数据倾斜
  - 1 个物理节点 → 150 个虚拟节点
  - 数据分布更均匀
```

### 3.2 客户端配置

```yaml
# Go 客户端 (bradfitz/gomemcache)
apiVersion: v1
kind: ConfigMap
metadata:
  name: memcached-client-config
  namespace: application
data:
  config.yaml: |
    memcached:
      servers:
        - "memcached-0.memcached-headless.cache.svc.cluster.local:11211"
        - "memcached-1.memcached-headless.cache.svc.cluster.local:11211"
        - "memcached-2.memcached-headless.cache.svc.cluster.local:11211"
        - "memcached-3.memcached-headless.cache.svc.cluster.local:11211"
        - "memcached-4.memcached-headless.cache.svc.cluster.local:11211"
        - "memcached-5.memcached-headless.cache.svc.cluster.local:11211"
      max_idle_conns: 50
      timeout: 100ms
```

```go
// Go 示例
import (
    "github.com/bradfitz/gomemcache/memcache"
)

mc := memcache.New(
    "memcached-0.memcached-headless.cache:11211",
    "memcached-1.memcached-headless.cache:11211",
    "memcached-2.memcached-headless.cache:11211",
    "memcached-3.memcached-headless.cache:11211",
    "memcached-4.memcached-headless.cache:11211",
    "memcached-5.memcached-headless.cache:11211",
)
mc.MaxIdleConns = 50
mc.Timeout = 100 * time.Millisecond

// 使用
mc.Set(&memcache.Item{
    Key:        "user:1001",
    Value:      []byte(`{"name":"Alice"}`),
    Expiration: 3600,
})

item, err := mc.Get("user:1001")
```

```python
# Python 示例 (pymemcache)
from pymemcache.client.hash import HashClient

client = HashClient([
    ("memcached-0.memcached-headless.cache.svc.cluster.local", 11211),
    ("memcached-1.memcached-headless.cache.svc.cluster.local", 11211),
    ("memcached-2.memcached-headless.cache.svc.cluster.local", 11211),
])

client.set("user:1001", '{"name": "Alice"}', expire=3600)
value = client.get("user:1001")
```

## 4. 与 Redis 对比选型

### 4.1 功能对比

| 特性 | Memcached | Redis |
|------|-----------|-------|
| 数据结构 | 仅 String/KV | String/Hash/List/Set/ZSet/Stream |
| 持久化 | 不支持 | RDB + AOF |
| 复制 | 不支持 | 主从复制 |
| 集群 | 客户端分片 | 原生 Cluster |
| 多线程 | 支持 | Redis 6.0+ IO 多线程 |
| 内存效率 | 更高 (slab 分配) | 略低 |
| 原子操作 | CAS | Lua/Transaction/Pipeline |
| 发布订阅 | 不支持 | 支持 |
| 消息队列 | 不支持 | List/Stream |
| 适用场景 | 纯缓存 | 缓存 + 数据结构服务 |

### 4.2 选型决策树

```
需要选择缓存方案?
│
├── 只需要简单 KV 缓存?
│   ├── 是 → Memcached (更简单高效)
│   └── 否 → Redis
│
├── 需要数据持久化?
│   ├── 是 → Redis
│   └── 否 → 两者皆可
│
├── 需要复杂数据结构?
│   ├── 是 → Redis
│   └── 否 → Memcached
│
├── 需要发布订阅?
│   ├── 是 → Redis
│   └── 否 → 两者皆可
│
├── QPS 极高 + 数据简单?
│   └── Memcached (多线程优势)
│
└── 不确定 → Redis (功能更全面)
```

### 4.3 混合使用架构

```
┌─────────────────────────────────────────────────┐
│                  Application                     │
└───────┬────────────────────────────────────┬─────┘
        │                                    │
        ▼                                    ▼
┌───────────────┐                  ┌───────────────┐
│  Memcached    │                  │    Redis      │
│  - Session    │                  │  - 排行榜     │
│  - 页面缓存   │                  │  - 购物车     │
│  - API 响应   │                  │  - 消息队列   │
│  - 热点数据   │                  │  - 分布式锁   │
└───────────────┘                  └───────────────┘
  简单 KV 高 QPS                    复杂数据结构
```

## 5. 多线程配置

### 5.1 线程模型

```
Memcached 线程模型:

  Main Thread (监听 + 分发)
       │
       ├── Worker Thread 0 ──→ 处理请求 ──→ 网络 IO
       ├── Worker Thread 1 ──→ 处理请求 ──→ 网络 IO
       ├── Worker Thread 2 ──→ 处理请求 ──→ 网络 IO
       └── Worker Thread 3 ──→ 处理请求 ──→ 网络 IO

  -t 4: 启动 4 个 Worker 线程
  -o modern: 启用现代优化 (yield_on_event, no_hashexpand)
```

### 5.2 线程数调优

```bash
# 推荐配置: CPU 核数 / 2
# 4 核 → -t 2
# 8 核 → -t 4
# 16 核 → -t 8

# 监控线程状态
echo "stats" | nc localhost 11211 | grep -E "threads|conn"

# 关键指标
# - threads: 当前线程数
# - conn_yields: 连接让出次数 (高值说明需要更多线程)
```

### 5.3 连接池优化

```
客户端连接池配置:

max_idle_conns: 50-100 (每个节点)
conn_timeout: 100-200ms
read_timeout: 200-500ms
write_timeout: 200-500ms

避免:
  - 每次请求新建连接 (高延迟)
  - 连接数过多 (文件描述符耗尽)
  - 长连接不回收 (内存泄漏)
```

## 6. 监控指标

### 6.1 核心指标

```bash
# 获取所有指标
echo "stats" | nc memcached-0.memcached-headless 11211

# 关键指标解析
# pid: 进程 ID
# uptime: 运行时间 (秒)
# curr_connections: 当前连接数
# total_connections: 累计连接数
# cmd_get: GET 命令数
# cmd_set: SET 命令数
# get_hits: GET 命中数
# get_misses: GET 未命中数
# evictions: 淘汰数
# bytes: 当前存储字节数
# curr_items: 当前 item 数
# total_items: 累计 item 数
# bytes_read: 读取字节数
# bytes_written: 写入字节数
# threads: 线程数
```

### 6.2 命中率计算

```
命中率 = get_hits / (get_hits + get_misses)

健康阈值:
  - 命中率 > 90%: 良好
  - 命中率 80-90%: 需关注
  - 命中率 < 80%: 需优化

常见低命中率原因:
  1. 缓存容量不足 → 增加 -m
  2. TTL 设置过短 → 增加过期时间
  3. 缓存雪崩 → 添加随机 TTL 偏移
  4. 热点 Key 过期 → 延长热点 TTL
```

### 6.3 Prometheus 告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: memcached-alerts
  namespace: monitoring
spec:
  groups:
  - name: memcached
    rules:
    - alert: MemcachedLowHitRate
      expr: |
        memcached_commands_total{command="get",status="hit"} /
        (memcached_commands_total{command="get",status="hit"} +
         memcached_commands_total{command="get",status="miss"}) < 0.8
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Memcached 命中率低于 80%"
    - alert: MemcachedHighEvictions
      expr: rate(memcached_items_evicted_total[5m]) > 100
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Memcached 淘汰速率过高"
    - alert: MemcachedMemoryFull
      expr: |
        memcached_current_bytes / memcached_limit_bytes > 0.9
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Memcached 内存使用率超过 90%"
    - alert: MemcachedNodeDown
      expr: up{job="memcached"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Memcached 节点不可达"
```

## 7. 客户端优化

### 7.1 序列化优化

```
序列化方式对比:

JSON: 通用性好，体积大
Protobuf: 体积小，需要 schema
MessagePack: 体积小，无需 schema
FlatBuffers: 零拷贝，高性能

推荐:
  - 简单场景 → JSON
  - 高性能场景 → MessagePack / Protobuf
  - 超大对象 → 压缩 (snappy/lz4) + 序列化
```

### 7.2 批量操作

```python
# Python - 批量获取
from pymemcache.client.hash import HashClient

client = HashClient([...])

# 批量获取 (减少网络往返)
keys = ["user:1001", "user:1002", "user:1003"]
result = client.get_many(keys)

# 批量设置
data = {
    "user:1001": '{"name": "Alice"}',
    "user:1002": '{"name": "Bob"}',
    "user:1003": '{"name": "Charlie"}',
}
client.set_many(data, expire=3600)
```

### 7.3 缓存模式

```
Cache-Aside (旁路缓存):
  读: 先查缓存 → 命中返回 → 未命中查 DB → 写缓存 → 返回
  写: 更新 DB → 删除缓存 (非更新)
  优点: 简单、灵活
  缺点: 首次请求必然 miss

Read-Through:
  读: 缓存层自动回源 DB
  优点: 应用简单
  缺点: 缓存层复杂度高

Write-Through:
  写: 同时写缓存和 DB
  优点: 数据一致
  缺点: 写延迟增加

Write-Behind (异步写):
  写: 先写缓存 → 异步写 DB
  优点: 写延迟低
  缺点: 可能丢数据
```

## 8. 故障排查速查

| 问题 | 排查命令 | 常见原因 |
|------|---------|---------|
| 命中率低 | `echo "stats"` 计算 hit_rate | 容量不足、TTL 过短 |
| 高淘汰 | `echo "stats"` 检查 evictions | 内存满，增大 -m |
| 连接拒绝 | `echo "stats"` 检查 curr_connections | 达到 -c 限制 |
| 响应慢 | `echo "stats"` 检查 conn_yields | 线程不足，增加 -t |
| 数据丢失 | 检查 uptime | 进程重启 (无持久化) |
| 内存碎片 | `echo "stats slabs"` | slab 分配不合理 |
| 网络超时 | 检查客户端配置 | 超时时间过短 |


<!-- risk-assessed -->
