---
title: K8S 应用模式
category: concepts
tags: [patterns, microservices, event-driven, multi-tenancy, k8s]
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

# K8S 应用模式

Kubernetes 上的现代应用架构模式，涵盖服务网格、网关、事件驱动、批处理、有状态应用、多租户与分布式运行时。

## 无 Sidecar 服务网格

### Istio Ambient GA（v1.24）
Istio Ambient Mesh 是无 Sidecar 的服务网格架构，v1.24 正式 GA：

**双层架构设计：**
- **ztunnel（L4 层）**: 节点级 Rust 代理，处理 mTLS、TCP 转发、L4 策略
  - 每个节点一个 DaemonSet，资源开销极低
  - 基于 Rust 实现，零拷贝转发，性能优于 Envoy Sidecar
  - 透明代理：应用无需修改代码或 Sidecar 注入
  - 支持 HBONE（HTTP-Based Overlay Network Environment）隧道协议
- **waypoint（L7 层）**: 按命名空间/服务部署的 Envoy 代理
  - 处理 HTTP/gRPC 路由、重试、限流、可观测性
  - 按需部署：仅需要 L7 策略时创建 waypoint
  - 支持 AuthorizationPolicy L7 规则

**相比 Sidecar 模式的优势：**
- 资源节省：无 Sidecar 容器的 CPU/Memory 开销
- 启动加速：无需等待 Sidecar Ready
- 运维简化：升级网格不影响应用 Pod
- 延迟降低：ztunnel 共享节点代理，减少跳数

### Sidecar 模式 vs Ambient 模式对比

| 特性 | Sidecar 模式 | Ambient 模式 |
|------|-------------|-------------|
| 数据面代理 | 每 Pod Envoy Sidecar | ztunnel(DaemonSet) + waypoint(按需) |
| 资源开销 | 高（每 Pod 额外 100-200Mi） | 低（节点共享） |
| 启动依赖 | Sidecar 必须 Ready | 无启动依赖 |
| L4 mTLS | Envoy | ztunnel（Rust） |
| L7 策略 | Envoy Sidecar | waypoint（Envoy） |
| 应用侵入性 | 需注入 Sidecar | 完全透明 |
| 升级影响 | 需重启应用 Pod | 独立升级 |

## Gateway API v1.5

### 统一入口/网格/gRPC
Gateway API 已成为 K8S 入口流量管理的标准 API（v1.5）：

**核心资源：**
- **GatewayClass**: 由基础设施提供者定义的网关实现类
- **Gateway**: 声明式网关实例，监听端口/协议
- **HTTPRoute**: HTTP 流量路由规则（路径匹配、Header 匹配、权重分割）
- **GRPCRoute**: gRPC 流量专用路由（v1.0 GA）
- **TLSRoute**: TLS 流量直通路由
- **TCPRoute / UDPRoute**: 四层流量路由

**v1.5 新特性：**
- Backend TLS Policy：后端连接 TLS 配置
- 会话亲和性（Session Affinity）原生支持
- 跨命名空间路由引用（ReferenceGrant 增强）
- Gateway 基础设施标签/注解传播

### AI Gateway WG
Gateway API 的 AI Gateway 工作组推动 AI 流量管理标准化：

**Inference Extension：**
- `InferenceModel`: 定义 AI 模型的路由和策略
- `InferencePool`: 管理推理后端池
- 支持模型级别的流量分割（A/B 测试、金丝雀发布）
- 请求队列管理、超时、重试策略
- GPU 资源感知的负载均衡

**AI Gateway 典型功能：**
- Prompt 审计与过滤
- Token 级别限流
- 多模型路由与回退
- 推理延迟 SLA 管理
- 成本追踪与预算控制

## 事件驱动

### CloudEvents（CNCF Graduated）
CloudEvents 是云原生事件数据的标准规范：
- 标准化事件格式（source、type、subject、time 等属性）
- 支持 JSON、Avro、Protobuf 序列化
- HTTP、AMQP、Kafka、MQTT 传输绑定
- CNCF Graduated 项目，行业广泛采纳
- SDK 支持：Go、Java、Python、JavaScript、C#、Rust、Ruby

### NATS（400M+ 下载）
NATS 是轻量级高性能消息系统：
- 核心 NATS：At-Most-Once 消息传递，超低延迟
- JetStream：持久化消息、At-Least-Once/Exactly-Once 语义
- 支持 Pub/Sub、Queue Group、Request-Reply 模式
- 内置集群、Leaf Node、Super Cluster 多集群拓扑
- 资源占用极低，适合边缘和 IoT 场景
- 400M+ 容器下载量，CNCF Graduated

### Strimzi Kafka Operator
Strimzi 在 K8S 上运行生产级 Apache Kafka：
- CRD 管理 Kafka 集群、Topic、User、Connector
- 支持 KRaft 模式（无需 ZooKeeper）
- 内置 MirrorMaker 2 跨集群复制
- OAuth/OIDC 认证、ACL 授权
- Kafka Connect + Kafka Bridge HTTP 接口
- CNCF Sandbox 项目

## 批处理/AI Job

### JobSet（分布式训练）
JobSet 是 K8S 原生的分布式训练 Job 管理：
- 将分布式训练定义为一组相关 Job 的集合
- 支持 Pod 间通信拓扑（Headless Service 自动创建）
- 失败策略：重启整个 JobSet 或单个 Job
- 支持 PyTorch、TensorFlow、JAX 分布式训练框架
- CNCF SIG Batch 孵化

### Kueue 队列调度
Kueue 是 K8S 原生的批处理队列管理系统：
- **Queue**: 用户级作业队列，支持优先级排序
- **ClusterQueue**: 集群级资源配额管理
- **LocalQueue**: 命名空间级队列，绑定到 ClusterQueue
- 资源管理：Cohort（队列组）间借用/抢占
- 支持 Job、JobSet、RayJob 等多种工作负载
- Fair Share 调度保障公平性

### MultiKueue 跨集群
MultiKueue 实现跨集群批处理调度：
- 将 Job 分发到多个 K8S 集群执行
- 支持不同区域/云厂商的集群
- 统一队列视图，自动负载均衡
- 适用于 GPU 集群跨区域共享场景

### DRA（Dynamic Resource Allocation）GPU
DRA 是 K8S 的动态资源分配框架（v1.31 GA）：
- 替代传统 Device Plugin，提供声明式资源声明
- `ResourceClaim`：声明资源需求（如 GPU 型号、显存）
- `ResourceClass`：定义资源供给策略
- 支持 GPU MIG 分片、vGPU、RDMA 等复杂设备
- 与 Kueue 集成实现 GPU 队列管理
- 支持跨节点、跨集群资源池

## 有状态/Operator 模式

### Sail Operator 1.0
Sail Operator 管理 K8S 上的有状态应用生命周期：
- 声明式定义应用拓扑（主从、集群、分片）
- 自动处理扩缩、故障恢复、版本升级
- 集成备份/恢复、监控、告警

### Strimzi（Kafka Operator 模式）
Strimzi 是 K8S Operator 的经典范例：
- CRD 驱动：Kafka、KafkaConnect、KafkaMirrorMaker2、KafkaTopic、KafkaUser
- 滚动升级：零停机 Kafka 版本升级
- 存储管理：PVC 动态扩容、存储类切换
- 监控集成：JMX Exporter + Prometheus Operator

### 数据库/队列/AI Operator 生态

**数据库 Operator：**
- CloudNativePG（PostgreSQL CNCF Sandbox）
- MySQL Operator for K8S（Oracle 官方）
- MongoDB Community Operator
- Vitess Operator（分布式 MySQL）
- TiDB Operator（分布式 NewSQL）

**消息队列 Operator：**
- Strimzi（Kafka）
- NATS Operator
- RabbitMQ Cluster Operator（VMware）

**AI/ML Operator：**
- KubeFlow Training Operator（分布式训练）
- KAITO（Azure AI 模型推理）
- KServe（模型服务）
- Ray Operator（分布式计算）

## 多租户

### vCluster Platform 4.9
vCluster 是虚拟集群解决方案，4.9 版本引入三大创新：

**vCluster 核心：**
- 在物理集群内创建完全隔离的虚拟集群
- 每个 vCluster 拥有独立的 API Server、etcd
- 租户拥有完整集群管理员权限，互不干扰
- 同步核心资源（Pod、Service、Ingress）到底层集群

**vNode 内核级隔离：**
- 基于 eBPF 的节点级资源隔离
- CPU、内存、IO 的硬隔离保障
- 超越 cgroup 的隔离粒度
- 支持 NUMA 感知的资源分配

**vMetal 裸金属管理：**
- 将裸金属服务器纳入 vCluster 调度
- 支持 GPU 直通、RDMA 高性能网络
- 适用于 AI 训练、HPC 场景
- 与 vCluster 虚拟集群无缝集成

### Capsule（CNCF Sandbox）
Capsule 实现轻量级多租户：
- **Tenant**: 租户边界，包含多个命名空间
- 资源配额、Limit Range、Network Policy 自动注入
- 节点选择器、污点容忍的租户级控制
- 租户管理员自主管理内部命名空间
- 与 Hierarchical Namespace Controller 互补
- CNCF Sandbox 项目

### 多租户方案对比

| 特性 | vCluster | Capsule | 命名空间 + RBAC |
|------|---------|---------|----------------|
| 隔离级别 | 虚拟 API Server | 逻辑边界 | 命名空间 |
| 租户自由度 | 集群管理员 | 受限管理员 | 受限 |
| CRD 隔离 | ✅ | ❌ | ❌ |
| 资源开销 | 中（每 vCluster 一个 API Server） | 低 | 极低 |
| 适用场景 | 强隔离、自助服务 | 轻量多租户 | 简单隔离 |
| GPU/裸金属 | ✅ vMetal | 节点选择器 | 节点选择器 |

## Dapr

### CNCF 分布式应用运行时
Dapr（Distributed Application Runtime）是 CNCF 孵化项目，为微服务提供构建块：

**核心构建块：**
- **服务调用（Service Invocation）**: 服务间发现与调用，支持重试、mTLS
- **状态管理（State Management）**: 统一状态 API，支持 Redis、CosmosDB、PostgreSQL 等
- **发布/订阅（Pub/Sub）**: 统一消息 API，支持 Kafka、NATS、RabbitMQ、Redis Streams 等
- **绑定（Bindings）**: 外部系统触发器（Cron、AWS S3、Twilio 等）
- **可观测性（Observability）**: 分布式追踪、指标、日志自动收集
- **密钥管理（Secret Management）**: 统一密钥 API，对接 Vault、K8S Secrets、AWS SM
- **Actor 模型**: 虚拟 Actor 编程模型，简化有状态并发
- **工作流（Workflows）**: 长运行工作流编排（v1.13 GA）
- **分布式锁（Distributed Lock）**: 跨实例互斥
- **密码学（Cryptography）**: 加密/解密 API

**Dapr on K8S：**
- Sidecar 注入：`dapr.io/enabled: true` 注解自动注入
- 与服务网格共存：可与 Istio/Linkerd 同时使用
- CRD 管理：Component、Configuration、Resiliency 声明式定义
- Actor 状态存储：支持有状态应用的自动激活/停用

**Dapr vs 服务网格：**

| 维度 | Dapr | 服务网格 |
|------|------|---------|
| 关注点 | 应用级构建块 | 基础设施级流量管理 |
| 编程语言 | SDK 多语言 | 无侵入 |
| 状态管理 | ✅ | ❌ |
| Pub/Sub | ✅ | ❌ |
| mTLS | ✅ | ✅ |
| 流量控制 | ❌ | ✅（丰富路由规则） |
| 可组合性 | 构建块按需启用 | 全局启用 |

## 相关概念

- [[concepts/k8s-networking-evolution.md|k8s networking evolution]] — K8S 网络演进
- [[concepts/platform-engineering-idp.md|platform engineering idp]] — 平台工程与 IDP
- [[concepts/progressive-delivery-strategies.md|progressive delivery strategies]] — 渐进式交付策略
