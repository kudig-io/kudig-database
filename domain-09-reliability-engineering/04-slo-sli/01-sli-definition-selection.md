---
title: SLI 定义与选择方法论
description: '| 网络 | 带宽使用率 | 连接队列溢出 |'
category: domain
tags:
- sre
- slo
- sli
- observability
- reliability
- etcd
- apiserver
- kubelet
- scheduler
- statefulset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SLI 定义与选择方法论 是什么
- 如何 SLI 定义与选择方法论
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- SLI
- 定义与选择方法论
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- etcd-basics
---

# SLI 定义与选择方法论

> **核心原则**: SLI 不是技术指标，而是用户体验的量化代理。好的 SLI 应该让用户说"这就是我关心的"。

## 什么是 SLI

**SLI (Service Level Indicator)** — 服务级别指标，是经过审慎选择的量化指标，用于衡量服务某个方面的可靠性水平。

### SLI 的核心特征

| 特征 | 说明 | 反例 |
|------|------|------|
| **用户视角** | 反映用户真实体验 | CPU 使用率 ❌ |
| **可量化** | 能用数字精确表达 | "系统很稳定" ❌ |
| **可聚合** | 支持时间窗口内的汇总 | 单次请求延迟 ✅ |
| **可比较** | 能跨版本/跨集群对比 | 错误数 vs 错误率 |

## 四大黄金信号 (Four Golden Signals)

Google SRE 提出的四个核心维度，适用于绝大多数服务：

### 1. 延迟 (Latency)

**定义**: 服务响应请求所需的时间。

**关键区分**:
```
⚠️ 必须区分成功请求的延迟 vs 失败请求的延迟

错误示例: 平均延迟 200ms
  → 包含大量 5xx 快速失败（< 10ms），拉低平均值，掩盖真实问题

正确做法:
  - 成功请求 P99 延迟: 180ms
  - 失败请求 P99 延迟: 5ms
```

**Kubernetes 场景下的延迟 SLI**:

| 服务类型 | 延迟测量点 | 建议 SLI |
|---------|-----------|---------|
| HTTP API | Ingress → Pod | P99 响应时间 < 500ms |
| gRPC 服务 | Service → Pod | P99 响应时间 < 200ms |
| 数据库查询 | App → DB | P99 查询时间 < 100ms |
| 消息消费 | Queue → Consumer | P99 消费延迟 < 5s |

### 2. 流量 (Traffic)

**定义**: 系统承受的请求量或负载。

**流量测量维度**:
```
HTTP 服务: 每秒请求数 (QPS/RPS)
gRPC 服务: 每秒 RPC 调用数
消息队列: 每秒消息处理数
批处理: 每小时处理记录数
```

**流量 SLI 的应用场景**:
- 容量规划基准
- 故障影响范围评估（"峰值流量时的故障影响 X 用户"）
- 弹性伸缩触发条件

### 3. 错误 (Errors)

**定义**: 请求失败的比率。

**错误分类**:
```
明确错误 (Explicit):
  - HTTP 5xx
  - gRPC status != OK
  - 超时（超过阈值视为错误）

隐式错误 (Implicit):
  - 返回 200 但内容错误
  - 返回 200 但处理时间超过用户容忍度
  - 部分成功（如批量接口部分失败）
```

**错误率计算**:
```
错误率 = 错误请求数 / 总请求数

⚠️ 注意: 总请求数 = 成功请求数 + 明确错误请求数 + 隐式错误请求数
```

### 4. 饱和度 (Saturation)

**定义**: 服务容量的使用程度，接近 100% 时性能通常下降。

**饱和度与利用率的区别**:
```
利用率 (Utilization): 资源被占用的比例
  → CPU 利用率 80%

饱和度 (Saturation): 因资源不足而等待的比例
  → 请求在队列中等待 CPU 的比例
```

**Kubernetes 场景下的饱和度 SLI**:

| 资源 | 利用率指标 | 饱和度指标 |
|------|-----------|-----------|
| CPU | CPU 使用率 | CPU 节流次数 (CPU throttling) |
| 内存 | 内存使用率 | OOM Kill 次数 |
| 磁盘 | 磁盘使用率 | I/O 等待时间 |
| 网络 | 带宽使用率 | 连接队列溢出 |

## RED 方法 vs USE 方法

### RED 方法（面向请求的服务）

适用于: HTTP API、gRPC、消息队列消费者

```
R - Rate (请求率)
E - Errors (错误率)
D - Duration (持续时间/延迟)
```

**RED 检查清单**:
- [ ] 能测量每秒请求数
- [ ] 能分类成功/失败请求
- [ ] 能测量延迟分布（非平均值）

### USE 方法（面向资源的服务）

适用于: 数据库、缓存、消息队列、节点

```
U - Utilization (利用率)
S - Saturation (饱和度)
E - Errors (错误数)
```

**USE 检查清单**:
- [ ] 能测量资源利用率（0-100%）
- [ ] 能检测资源饱和度（排队、等待）
- [ ] 能检测资源错误（OOM、I/O error）

### 方法选择决策树

```
服务类型?
├── 处理请求/事件 → RED 方法
│   ├── HTTP API → Rate, Errors(5xx%), Duration(p99)
│   ├── gRPC → Rate, Errors(grpc_code != OK), Duration(p99)
│   └── 消息消费者 → Rate, Errors(处理失败), Duration(消费延迟)
│
└── 提供资源/存储 → USE 方法
    ├── 数据库节点 → CPU%(U), 连接队列(S), 查询错误(E)
    ├── 缓存节点 → 内存%(U), 驱逐率(S), 连接错误(E)
    └── 存储节点 → 磁盘%(U), I/O wait(S), 磁盘错误(E)
```

## Kubernetes 场景 SLI 映射表

### 工作负载级别 SLI

| 对象 | 黄金信号 | 建议 SLI | 数据来源 |
|------|---------|---------|---------|
| **Deployment** | 延迟 | P99 响应时间 | Ingress Controller Metrics |
| | 错误率 | 5xx 比率 | Ingress/Service Metrics |
| | 流量 | QPS | Ingress/Service Metrics |
| | 饱和度 | Pod CPU Throttling | kubelet metrics |
| **StatefulSet** | 延迟 | P99 查询时间 | 应用 Exporter |
| | 错误率 | 连接失败率 | 应用 Exporter |
| | 流量 | 每秒事务数 | 应用 Exporter |
| | 饱和度 | 连接池使用率 | 应用 Exporter |
| **DaemonSet** | 延迟 | 节点同步延迟 | 应用 Exporter |
| | 错误率 | 同步失败率 | 应用 Exporter |
| | 饱和度 | 节点 CPU 使用率 | kube-state-metrics |

### 基础设施级别 SLI

| 组件 | SLI | 阈值建议 | 数据来源 |
|------|-----|---------|---------|
| **API Server** | 请求延迟 | P99 < 1s | apiserver_request_duration_seconds |
| | 错误率 | < 1% | apiserver_request_total |
| | 饱和度 | etcd 请求队列深度 | etcd_request_duration_seconds |
| **etcd** | 磁盘同步延迟 | P99 < 10ms | etcd_disk_wal_fsync_duration_seconds |
| | 提交延迟 | P99 < 100ms | etcd_disk_backend_commit_duration_seconds |
| | 饱和度 | DB 大小增长率 | etcd_mvcc_db_total_size_in_bytes |
| **kubelet** | PLEG 延迟 | < 1s | kubelet_pleg_relist_duration_seconds |
| | 节点状态 | NotReady 比率 | kube_node_status_condition |
| **Scheduler** | 调度延迟 | P99 < 10s | scheduler_e2e_scheduling_duration_seconds |
| | 调度失败率 | < 0.1% | scheduler_schedule_attempts_total |

## SLI 选择实践流程

### Step 1: 识别用户旅程

```
示例: 电商订单服务

用户旅程:
1. 浏览商品列表 → 商品服务
2. 查询商品详情 → 商品服务
3. 加入购物车 → 购物车服务
4. 提交订单 → 订单服务
5. 支付 → 支付服务
6. 查看订单状态 → 订单服务
```

### Step 2: 识别关键路径

```
关键路径: 直接影响收入或用户体验的路径

高优先级:
  - 提交订单 → 支付（直接影响收入）
  - 订单状态查询（用户焦虑点）

中优先级:
  - 商品详情（可容忍短暂延迟）

低优先级:
  - 商品列表推荐（非实时性要求）
```

### Step 3: 为关键路径选择 SLI

```
订单提交服务:
  SLI-1: 订单创建成功率 > 99.9%
    → 成功: HTTP 200 且订单写入数据库
    → 失败: HTTP 5xx 或 超时或数据库写入失败

  SLI-2: 订单创建 P99 延迟 < 500ms
    → 测量点: 用户点击"提交"到收到响应

  SLI-3: 支付回调处理成功率 > 99.99%
    → 成功: 支付状态正确更新
    → 失败: 回调丢失或状态不一致
```

### Step 4: 定义 SLI 计算方式

```yaml
# SLI 定义模板
sli_name: order_creation_success_rate
service: order-service
measurement:
  good_events: |
    sum(increase(order_created_total{status="success"}[1m]))
  total_events: |
    sum(increase(order_created_total[1m]))
  formula: good_events / total_events
window: 28d  # SLO 评估窗口
```

## 常见反模式

### 反模式 1: 使用平均值

```
❌ 平均响应时间 100ms
  → 掩盖了长尾延迟，用户实际体验可能是 50ms 或 2s

✅ P99 响应时间 < 200ms
  → 99% 的请求都在 200ms 内完成
```

### 反模式 2: 使用系统指标替代用户体验

```
❌ CPU 使用率 < 80%
  → 用户不关心 CPU，关心的是响应速度

✅ P99 响应时间 < 200ms
  → 直接反映用户体验
```

### 反模式 3: 监控所有端点

```
❌ 监控 /healthz、/metrics、/debug 的延迟
  → 这些端点的性能不代表用户感知

✅ 只监控业务端点（如 /api/v1/orders、/api/v1/pay）
  → 反映真实用户体验
```

### 反模式 4: 忽略依赖服务

```
❌ 订单服务 99.9% 可用
  → 但如果数据库不可用，订单服务实际上无法工作

✅ 订单服务 99.9% 可用
  + 数据库查询成功率 99.95%
  → 关注完整调用链
```

## 相关

- [[domain-09-reliability-engineering/04-slo-sli/02-slo-implementation-guide]] — SLO 设定与实施指南
- [[domain-09-reliability-engineering/04-slo-sli/03-error-budget-management]] — 错误预算管理
- [[domain-06-observability/06-slo-sli/18-slo-sli-system]] — SLO/SLI 体系概述
- [[domain-06-observability/02-metrics/02-monitoring-metrics-system]] — 指标监控系统
