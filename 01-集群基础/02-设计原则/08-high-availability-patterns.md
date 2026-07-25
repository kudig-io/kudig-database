---
title: 08 - 高可用架构模式 (HA Patterns)
description: '# 08 - 高可用架构模式 (HA Patterns)'
summary: '在早期的 K8s 版本中，控制面组件（如 Scheduler）使用 `Endpoints` 或 `ConfigMap` 实现分布式锁。'
category: design-principles
tags:
- k8s
- design
- principles
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- minio
- redis
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- 高可用架构模式 (HA Patterns) 是什么
- 如何 高可用架构模式 (HA Patterns)
- Kubernetes 2 design principles 最佳实践
trigger_keywords:
- 高可用架构模式
- HA
- Patterns
- design
- principles
prerequisites:
- kubectl-basics
- kubernetes-concepts
- etcd-basics
- kafka-basics
- redis-basics
- mysql-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 08 - 高可用架构模式 (HA Patterns)

<!-- chunk: 资深视点：Lease API 为什么取代了 Endpoints 锁？ -->
## 资深视点：Lease API 为什么取代了 Endpoints 锁？

在早期的 K8s 版本中，控制面组件（如 Scheduler）使用 `Endpoints` 或 `ConfigMap` 实现分布式锁。

### Lease API 的优势
1. **性能**: `Lease` 对象非常小，更新时对 API Server 和 [[etcd|etcd]] 的负载极低。
2. **解耦**: 避免了频繁更新 Endpoints 导致的大规模 Watch 通知（Endpoints 的变更会通知到所有节点的 kube-proxy）。
3. **节点心跳**: 现代 K8s 使用 Lease 承载节点心跳，极大地减轻了集群规模扩大时 API Server 的压力。

<!-- chunk: 高可用核心指标 -->
## 高可用核心指标

| 指标 | 英文 | 说明 | 计算方式 |
|-----|-----|------|---------|
| 可用性 | Availability | 系统正常运行时间比例 | 正常时间/总时间 |
| MTBF | Mean Time Between Failures | 平均问题间隔 | 运行时间/问题次数 |
| MTTR | Mean Time To Repair | 平均修复时间 | 修复时间/问题次数 |
| RTO | Recovery Time Objective | 恢复时间目标 | 业务可接受停机时间 |
| RPO | Recovery Point Objective | 恢复点目标 | 可接受数据丢失量 |

<!-- chunk: 可用性等级 -->
## 可用性等级

| 等级 | 年停机时间 | 月停机时间 | 典型场景 |
|-----|----------|----------|---------|
| 99% | 3.65天 | 7.3小时 | 内部系统 |
| 99.9% | 8.76小时 | 43分钟 | 一般业务 |
| 99.99% | 52.6分钟 | 4.3分钟 | 核心业务 |
| 99.999% | 5.26分钟 | 26秒 | 金融/电信 |

<!-- chunk: K8s控制平面高可用 -->
## K8s控制平面高可用

| 组件 | 部署模式 | 最小副本 | 推荐副本 |
|-----|---------|---------|---------|
| etcd | 集群 | 3 | 5 |
| kube-apiserver | 多副本+LB | 2 | 3 |
| kube-scheduler | 主备选举 | 2 | 3 |
| kube-controller-manager | 主备选举 | 2 | 3 |

### 控制平面架构

```
                    ┌─────────────────┐
                    │   Load Balancer │
                    │  (VIP/云LB/HAProxy)
                    └────────┬────────┘
                             │
         ┌───────────────────┼───────────────────┐
         │                   │                   │
         ▼                   ▼                   ▼
  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐
  │ Master-1    │    │ Master-2    │    │ Master-3    │
  │ ┌─────────┐ │    │ ┌─────────┐ │    │ ┌─────────┐ │
  │ │apiserver│ │    │ │apiserver│ │    │ │apiserver│ │
  │ ├─────────┤ │    │ ├─────────┤ │    │ ├─────────┤ │
  │ │scheduler│ │    │ │scheduler│ │    │ │scheduler│ │
  │ │(standby)│ │    │ │(leader) │ │    │ │(standby)│ │
  │ ├─────────┤ │    │ ├─────────┤ │    │ ├─────────┤ │
  │ │ctrl-mgr │ │    │ │ctrl-mgr │ │    │ │ctrl-mgr │ │
  │ │(leader) │ │    │ │(standby)│ │    │ │(standby)│ │
  │ ├─────────┤ │    │ ├─────────┤ │    │ ├─────────┤ │
  │ │  etcd   │ │    │ │  etcd   │ │    │ │  etcd   │ │
  │ └─────────┘ │    │ └─────────┘ │    │ └─────────┘ │
  └─────────────┘    └─────────────┘    └─────────────┘
```

<!-- chunk: Leader选举机制 -->
## Leader选举机制

| 组件 | 选举方式 | 锁资源 |
|-----|---------|-------|
| kube-scheduler | Lease对象 | kube-system/kube-scheduler |
| kube-controller-manager | Lease对象 | kube-system/kube-controller-manager |
| 自定义控制器 | Lease/ConfigMap/Endpoint | 自定义 |

### Lease选举参数

| 参数 | 说明 | 默认值 |
|-----|------|-------|
| --leader-elect | 启用选举 | true |
| --leader-elect-lease-duration | 租约时长 | 15s |
| --leader-elect-renew-deadline | 续约截止时间 | 10s |
| --leader-elect-retry-period | 重试间隔 | 2s |

<!-- chunk: 工作负载高可用 -->
## 工作负载高可用

| 策略 | 说明 | 配置 |
|-----|------|------|
| 多副本 | 运行多个Pod | replicas >= 2 |
| 反亲和性 | 分散到不同节点 | podAntiAffinity |
| 跨AZ分布 | 分散到不同可用区 | topologySpreadConstraints |
| PDB | 限制同时不可用数 | PodDisruptionBudget |

### 高可用Deployment示例

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ha-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: ha-app
  template:
    metadata:
      labels:
        app: ha-app
    spec:
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: ha-app
      - maxSkew: 1
        topologyKey: kubernetes.io/hostname
        whenUnsatisfiable: ScheduleAnyway
        labelSelector:
          matchLabels:
            app: ha-app
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: ha-app
            topologyKey: kubernetes.io/hostname
---
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: ha-app-pdb
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: ha-app
```

<!-- chunk: 故障检测与恢复 -->
## 故障检测与恢复

| 机制 | 检测对象 | 恢复动作 |
|-----|---------|---------|
| livenessProbe | 应用健康 | 重启容器 |
| readinessProbe | 应用就绪 | 移除Endpoints |
| Node Controller | 节点心跳 | 驱逐Pod |
| [[ReplicaSet|ReplicaSet]] | Pod数量 | 创建新Pod |

<!-- chunk: 节点故障处理 -->
## 节点故障处理

| 阶段 | 时间 | 动作 |
|-----|------|------|
| 节点失联 | 0s | 心跳丢失 |
| Unknown状态 | 40s | node-monitor-grace-period |
| 开始驱逐 | 5m | pod-eviction-timeout |
| 创建新Pod | 5m+ | ReplicaSet调谐 |

### 加速故障恢复

```yaml
# kubelet配置
nodeStatusUpdateFrequency: 10s      # 上报频率(默认10s)

# kube-controller-manager配置
--node-monitor-period=5s            # 检查周期(默认5s)
--node-monitor-grace-period=40s     # 宽限期(默认40s)
--pod-eviction-timeout=30s          # 驱逐超时(默认5m)
```

<!-- chunk: 跨可用区高可用 -->
## 跨可用区高可用

| 层级 | 策略 |
|-----|------|
| 集群级 | 控制平面跨AZ部署 |
| 节点级 | 节点池跨AZ分布 |
| Pod级 | topologySpreadConstraints |
| 存储级 | 跨AZ复制存储 |
| 网络级 | 多AZ负载均衡 |

<!-- chunk: 服务高可用模式 -->
## 服务高可用模式

| 模式 | 说明 | 适用场景 |
|-----|------|---------|
| Active-Active | 多副本同时服务 | 无状态服务 |
| Active-Passive | 主备切换 | 有状态服务 |
| N+1 | N个活动+1个备用 | 容量冗余 |
| N+M | N个活动+M个备用 | 高冗余要求 |

<!-- chunk: 健康检查最佳实践 -->
## 健康检查最佳实践

| 实践 | 说明 |
|-----|------|
| 区分liveness和readiness | 不同目的,不同配置 |
| 合理设置超时 | 避免误判 |
| 使用startupProbe | 慢启动应用 |
| 专用健康端点 | 不依赖业务逻辑 |
| 级联检查 | 检查关键依赖 |

<!-- chunk: etcd 高可用切换场景深度分析 -->
## etcd 高可用切换场景深度分析

### etcd Leader 故障切换时间线

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    etcd Leader 故障切换详细时间线                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  T+0s      Leader 心跳停止 (进程崩溃/网络分区/磁盘问题)                     │
│  │                                                                          │
│  T+0.1s    Followers 等待 heartbeat-interval (默认 100ms) 心跳              │
│  │                                                                          │
│  T+1s      Followers 进入 election-timeout (默认 1000ms)                    │
│  │          → 转为 Candidate，自增 term，发起选举                           │
│  │                                                                          │
│  T+1.1s    收到多数派选票 (通常 < 200ms 完成选举)                            │
│  │          → 新 Leader 当选                                                │
│  │                                                                          │
│  T+1.5s    新 Leader 开始服务写入请求                                       │
│  │                                                                          │
│  总停写时间: ~1-2s (取决于网络延迟)                                          │
│                                                                              │
│  注意事项:                                                                    │
│  • election-timeout 不宜过短 (避免网络抖动触发误选举)                        │
│  • election-timeout 不宜过长 (延长控制面不可用时间)                           │
│  • 推荐范围: 1000ms - 5000ms，必须大于 heartbeat-interval 的 10 倍          │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### etcd 集群规模选择指南

| 集群规模 | 容忍问题数 | 读性能 | 写性能 | 推荐场景 |
|----------|-----------|--------|--------|---------|
| 1 节点 | 0 | 最高 | 最高 | 开发/测试 |
| 3 节点 | 1 | 高 | 中 | 小型生产 (< 100 节点) |
| 5 节点 | 2 | 中 | 低 | 大型生产 (> 100 节点) |
| 7 节点 | 3 | 低 | 最低 | 超大规模或合规要求 |

> **关键公式**：容忍问题数 = (N-1)/2，写入延迟与节点数正相关（需要多数派确认）。

### 多集群高可用架构

#### 集群级故障切换设计

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    多集群高可用架构                                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│                        ┌─────────────────────┐                               │
│                        │   Global DNS/LB      │                               │
│                        │  (Route 53 / F5)     │                               │
│                        └──────────┬──────────┘                               │
│                                   │                                          │
│          ┌────────────────────────┼────────────────────────┐                 │
│          │                        │                        │                 │
│          ▼                        ▼                        ▼                 │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐          │
│  │  Cluster A      │    │  Cluster B      │    │  Cluster C      │          │
│  │  (Active)       │    │  (Active)       │    │  (Standby)      │          │
│  │                 │    │                 │    │                 │          │
│  │  etcd (3节点)   │    │  etcd (3节点)   │    │  etcd (3节点)   │          │
│  │  APIServer x3   │    │  APIServer x3   │    │  APIServer x3   │          │
│  │  Worker x50     │    │  Worker x50     │    │  Worker x50     │          │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘          │
│           │                       │                       │                  │
│           └───────────────────────┼───────────────────────┘                  │
│                                   │                                          │
│                    ┌────────────────────────┐                                │
│                    │  联邦控制平面            │                                │
│                    │  (Karmada / KubeFed)   │                                │
│                    │  • 资源分发策略         │                                │
│                    │  • 跨集群负载均衡       │                                │
│                    │  • 问题自动转移         │                                │
│                    └────────────────────────┘                                │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### 集群故障切换策略

| 策略 | 说明 | RTO | 实现方式 |
|------|------|-----|---------|
| Active-Active | 多集群同时服务，DNS 权重分流 | 接近 0 | Global LB + 健康检查 |
| Active-Standby | 主集群问题后切换到备集群 | 分钟级 | DNS 切换 / VIP 漂移 |
| Pilot Light | 备集群运行最小资源，问题时扩容 | 10-30 分钟 | 自动扩缩 + 镜像预热 |
| Warm Standby | 备集群运行完整服务但低流量 | 秒-分钟级 | 权重调整逐步切流 |

### 控制面 HA 生产级配置参数

```yaml
# kube-apiserver HA 参数
api_server_ha:
  --etcd-servers: "https://etcd-1:2379,https://etcd-2:2379,https://etcd-3:2379"
  --etcd-servers-overrides: "/events#https://etcd-1:2379;https://etcd-2:2379;https://etcd-3:2379"
  --apiserver-count: 3
  --endpoint-reconciler-type: lease
  --max-requests-inflight: 800
  --max-mutating-requests-inflight: 400

# kube-controller-manager HA 参数
controller_manager_ha:
  --leader-elect: true
  --leader-elect-lease-duration: 15s
  --leader-elect-renew-deadline: 10s
  --leader-elect-retry-period: 2s
  --node-monitor-period: 5s
  --node-monitor-grace-period: 40s
  --pod-eviction-timeout: 300s

# kube-scheduler HA 参数
scheduler_ha:
  --leader-elect: true
  --leader-elect-lease-duration: 15s
  --leader-elect-renew-deadline: 10s
  --leader-elect-retry-period: 2s
```

### 数据面 HA 设计

#### 有状态应用 HA 策略

| 应用类型 | HA 策略 | 实现方式 | 示例 |
|----------|---------|---------|------|
| 数据库 (MySQL) | 主从复制 + 自动故障转移 | Operator (Orchestrator/Vitess) | MySQL Operator |
| 数据库 (PostgreSQL) | 流复制 + Patroni | Patroni + etcd | PGO Operator |
| 缓存 (Redis) | Sentinel / Cluster 模式 | Redis Operator | Redis Sentinel |
| 消息队列 (Kafka) | 多副本 + ISR | Kafka Operator ([[Strimzi|Strimzi]]) | Strimzi |
| 对象存储 (MinIO) | 纠删码 + 多节点 | MinIO Operator | MinIO Tenant |

#### PodDisruptionBudget 最佳实践矩阵

| 场景 | minAvailable | maxUnavailable | 说明 |
|------|-------------|----------------|------|
| 关键服务 (3 副本) | 2 | - | 始终保持至少 2 个可用 |
| 关键服务 (5 副本) | 4 | - | 始终保持至少 4 个可用 |
| 一般服务 (3 副本) | - | 1 | 最多 1 个不可用 |
| 批处理任务 | - | 100% | 允许全部不可用 |
| 单副本服务 | - | 0 | 阻止任何自愿中断 |

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: critical-service-pdb
spec:
  minAvailable: "66%"
  selector:
    matchLabels:
      app: critical-service
```

<!-- chunk: 常见问题场景 -->
## 常见问题场景

| 问题 | 影响 | 缓解措施 |
|-----|------|---------|
| 单节点问题 | 部分Pod不可用 | 多副本+反亲和 |
| AZ问题 | 单AZ Pod不可用 | 跨AZ分布 |
| 控制平面问题 | 无法变更资源 | 控制平面HA |
| etcd问题 | 集群不可用 | etcd集群化 |
| 网络分区 | 部分节点隔离 | 多网络路径 |

> **交叉引用**：控制平面 HA 的详细配置请参考 [Domain-3: 控制平面高可用](../03-%E6%8E%A7%E5%88%B6%E5%B9%B3%E9%9D%A2/03-plane-high-availability.md)。

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 KUDIG Database — Global MOC
- [[01-集群基础/README.md|Domain-2: Kubernetes 设计原则与核心机制]]
- Domain-2 设计原则 — 开源项目索引
- Kubernetes 设计原则与哲学
- 声明式 API 与面向终态设计
- 控制器模式与调谐循环
- 04 - List-Watch 机制深度解析 (List-Watch)
- 05 - Informer 架构与工作队列 (Informer & Workqueue)
- 06 - 资源版本与并发控制 (Concurrency Control)
- 07 - 分布式共识与 etcd 原理 (etcd & Raft)
- 09 - Kubernetes 源码结构与阅读指南 (Source Code)
- 10 - CAP 定理与分布式系统基础 (CAP Theorem)

## See Also

- 06-resource-version-control
- 07-distributed-consensus-etcd
- 09-source-code-walkthrough
- 10-cap-theorem-distributed-systems


<!-- risk-assessed -->
