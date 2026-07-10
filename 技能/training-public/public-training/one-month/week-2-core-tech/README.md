---
title: 'Week 2: 核心技术构建期 (Days 8-14)'
description: '- "K8s 网络栈包括什么"'
summary: '- "K8s 网络栈包括什么"'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- scheduler
- cilium
- flannel
- calico
- coredns
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- 'Week 2: 核心技术构建期 (Days 8-14) 是什么'
- '如何 Week 2: 核心技术构建期 (Days 8-14)'
trigger_keywords:
- Week
- '2:'
- 核心技术构建期
- Days
- 8-14
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- kafka-basics
- mysql-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Week 2: 核心技术构建期 (Days 8-14)

```yaml
---
id: LEARN-ONE-MONTH-W2-README
title: Week 2 - 核心技术构建期 (Days 8-14)
topic: kubernetes
type: guide
tags: [week-2, control-plane, workloads, networking, storage, kubernetes, one-month]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "K8s 控制平面组件有哪些"
  - "Deployment StatefulSet DaemonSet 区别"
  - "K8s 网络栈包括什么"
  - "PV/PVC/StorageClass 关系"
  - "HPA 自动扩缩容原理"
trigger_keywords:
  - 控制平面
  - etcd
  - API Server
  - Scheduler
  - Controller Manager
  - 工作负载
  - Deployment
  - StatefulSet
  - 网络
  - CNI
  - Service
  - Ingress
  - 存储
  - PV
  - PVC
  - StorageClass
  - CSI
reading_level: intermediate
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 30min
related_domains:
  - 集群基础
  - 工作负载
  - 网络
  - 存储
related_topics:
  - kubernetes
  - control-plane
  - workloads
  - networking
  - storage
related:
  - 生产运维/topic-learn/public-training/one-month/week-1-foundation/README.md
  - 生产运维/topic-learn/public-training/one-month/week-3-devops-toolchain/README.md
---
```

## 概述

第二周是整个学习计划中技术密度最高的一周。在第一周的地基之上，本周将深入 K8s 的四大核心技术领域：**控制平面**、**工作负载**、**网络栈**和**存储体系**。这四个领域覆盖了 K8s 日常运维中 80% 以上的工作内容，是成为合格的 K8s 运维工程师必须跨越的技术门槛。

本周的学习理念是"理解原理 + 动手实践"。每个主题都包含理论讲解和对应的实操任务，确保你不仅能"知道是什么"，更能"知道为什么"和"知道怎么做"。

### 学习目标

- 深入理解控制平面各组件（etcd、API Server、Scheduler、Controller Manager）的工作机制
- 掌握所有主要工作负载类型（Deployment、StatefulSet、DaemonSet、Job、[[CronJob|CronJob]]）及生产级配置模式
- 掌握 K8s 网络栈的完整体系（CNI、Service、DNS、Ingress、[[NetworkPolicy|NetworkPolicy]]）
- 掌握存储体系的核心机制（PV/PVC、StorageClass、CSI）
- **产出**: 生产级应用编排方案

---

## 核心概念详解

### 控制平面深入

控制平面是 K8s 的"大脑"，理解其工作机制对于排障和性能调优至关重要。

**etcd** 是 K8s 集群的状态存储引擎。它是一个分布式的、一致的键值存储，使用 Raft 协议保证数据一致性。etcd 的几个关键特性需要理解：

- **线性一致性读**: 任何读取操作都返回最近写入的值。这是通过 ReadIndex 机制实现的——读取请求需要先从 Leader 确认当前的 Commit Index
- **MVCC（多版本并发控制）**: etcd 保存数据的多个历史版本，支持范围查询和历史查询。这也是 K8s Watch 机制的实现基础。但 MVCC 也意味着 etcd 的数据库会持续增长，需要定期通过 compaction 和 defrag 来回收空间
- **性能敏感因素**: etcd 对磁盘 IO 延迟非常敏感（建议使用 SSD，fsync 延迟低于 10ms），同时受网络延迟影响。在多 Member 的 etcd 集群中，Member 之间的 RTT 应低于 10ms

etcd 集群规模建议：

| 节点数 | 容错能力 | 适用场景 |
|--------|---------|---------|
| 3 | 允许 1 个问题 | 生产环境推荐 |
| 5 | 允许 2 个问题 | 大规模生产环境 |
| 7 | 允许 3 个问题 | 极高可用要求 |

**API Server** 是所有组件交互的中心枢纽。它的工作流程包括：认证（Authentication）→ 授权（Authorization）→ 准入控制（Admission Control）→ 数据验证 → 写入 etcd → 触发 Watch 通知。API Server 的性能直接影响整个集群的响应速度。常见性能问题：大量 List 请求占用过多内存、etcd 写入延迟导致请求排队、Webhook 准入控制器超时。

API Server 性能调优参数：

| 参数 | 说明 | 推荐值 |
|------|------|--------|
| --max-requests-inflight | 最大非 mutating 请求数 | 400-800 |
| --max-mutating-requests-inflight | 最大 mutating 请求数 | 200-400 |
| --request-timeout | 请求超时 | 60s |
| --enable-watch-cache | 启用 Watch 缓存 | true |

**Scheduler** 的工作分为两个阶段：

- **Filter（过滤）**: 排除不满足 Pod 调度条件的节点。过滤条件包括：节点资源是否充足、节点是否有对应的标签（nodeSelector/nodeAffinity）、Pod 是否能容忍节点的污点（Taint/Toleration）、持久卷是否在节点所在可用区等
- **[[Score|Score]]（打分）**: 对通过过滤的节点进行打分排序。打分策略包括：资源均衡（优先选择资源充裕的节点）、镜像本地性（优先选择已有所需镜像的节点）、亲和性/反亲和性等

**Controller Manager** 运行着多种控制器。每个控制器遵循 Reconcile（调和）模式：通过 Watch/List 监听资源变化 → 对比期望状态与实际状态 → 执行操作使实际状态趋向期望状态。例如，Deployment Controller 监听 Deployment 的变化，当 Replicas 从 2 变为 4 时，创建两个新的 [[ReplicaSet|ReplicaSet]]。

### 工作负载类型与生产模式

**Deployment** 是最常用的工作负载类型，用于管理无状态应用。它的核心特性是滚动更新（Rolling Update）和回滚（Rollback）。滚动更新策略通过 `maxSurge`（最多超出期望副本数多少）和 `maxUnavailable`（最多允许多少不可用）来控制更新节奏。生产建议：设置 `readinessProbe` 确保 Pod 就绪后才接收流量；使用 `progressDeadlineSeconds` 自动检测更新卡住的情况；保留足够的 `revisionHistoryLimit` 以支持回滚。

Deployment 更新策略对比：

| 策略 | maxSurge | maxUnavailable | 特点 |
|------|----------|----------------|------|
| 保守 | 1 | 0 | 先创建新 Pod，就绪后再删旧 Pod，始终全部可用 |
| 平衡 | 25% | 25% | 同时创建和删除，平衡速度和可用性 |
| 激进 | 50% | 50% | 快速更新，可能短暂减少可用副本 |

**StatefulSet** 用于管理有状态应用。与 Deployment 的关键区别：每个 Pod 有固定的有序名称（如 app-0、app-1）；Pod 按顺序创建和删除；每个 Pod 可以关联独立的 PersistentVolume；通过 Headless Service 为每个 Pod 提供稳定的网络标识。StatefulSet 适合数据库（MySQL、PostgreSQL）、消息队列（Kafka、RabbitMQ）等有状态应用。

**DaemonSet** 确保每个（或特定）节点上运行一个 Pod 副本。典型使用场景：日志采集 Agent（如 Filebeat、Promtail）、监控 Agent（如 Node Exporter）、网络插件（如 Calico、Flannel）、存储插件（如 CSI Driver）。

**HPA（Horizontal Pod Autoscaler）** 根据 CPU 使用率、内存使用率或自定义指标自动调整 Deployment/StatefulSet 的副本数。HPA 的工作原理：Metrics Server 定期采集 Pod 指标 → HPA Controller 计算所需副本数 → 调整目标资源的 Replicas 字段 → Scheduler 调度新 Pod。

### K8s 网络栈

K8s 网络是整个体系中最复杂的部分。网络栈由多个层次组成：

**CNI（Container Network Interface）** 负责为 Pod 分配 IP 地址并配置网络连通性。常见的 CNI 插件包括：Calico（支持 BGP 路由和网络策略）、Flannel（简单的 VxLAN 覆盖网络）、Cilium（基于 eBPF 的高性能网络）、Terway（阿里云 ENI 模式）。CNI 的选择需要考虑网络规模、性能要求、网络策略需求等因素。

CNI 插件对比：

| CNI | 网络模式 | NetworkPolicy | 性能 | 复杂度 |
|-----|---------|---------------|------|--------|
| Flannel | VxLAN Overlay | 不支持 | 中 | 低 |
| Calico | BGP / IPIP | 支持 | 高 | 中 |
| Cilium | eBPF | 支持 | 最高 | 中高 |
| Terway | ENI 直通 | 支持 | 高 | 低（ACK 托管） |

**Service** 提供了稳定的访问入口。四种 Service 类型：

- **ClusterIP**: 默认类型，分配集群内部 IP，只能在集群内访问
- **NodePort**: 在每个节点上开放一个端口（30000-32767），外部可以通过 `<NodeIP>:<NodePort>` 访问
- **LoadBalancer**: 创建外部负载均衡器（如 SLB），自动指向 NodePort
- **ExternalName**: 将 Service 映射到外部 DNS 名称（CNAME 记录）

**DNS（CoreDNS）** 为集群内的服务提供名称解析。Service 的 DNS 格式为 `<service-name>.<namespace>.svc.cluster.local`。Pod 的 DNS 格式为 `<pod-ip-dashed>.<namespace>.pod.cluster.local`。CoreDNS 支持自定义配置，如添加上游 DNS 服务器、配置存根域等。

**Ingress** 是集群入口的 HTTP/HTTPS 路由规则。Ingress Controller（如 Nginx Ingress Controller、ALB Ingress Controller）监听 Ingress 资源的变化，并配置实际的负载均衡规则。Ingress 支持基于域名和路径的路由、TLS 终止、流量分割等功能。

**NetworkPolicy** 提供了 Pod 级别的网络访问控制。默认情况下，K8s 中所有 Pod 可以互相通信。NetworkPolicy 可以限制 Pod 的入站和出站流量，实现网络隔离。

### 存储体系

K8s 的存储体系由以下核心概念组成：

**Volume** 是最基础的存储抽象，将存储挂载到 Pod 的文件系统中。Volume 的生命周期与 Pod 相同——Pod 删除时 Volume 也随之消失。emptyDir 和 hostPath 是两种最基本的 Volume 类型。

**PersistentVolume（PV）** 是集群级别的存储资源，由管理员预先创建或通过 StorageClass 动态供给。PV 有独立的生命周期，不依赖于 Pod。

**PersistentVolumeClaim（PVC）** 是用户对存储的"申请"。PVC 指定所需的存储大小、访问模式（ReadWriteOnce、ReadOnlyMany、ReadWriteMany）和 StorageClass。K8s 会自动将 PVC 绑定到满足条件的 PV。

**StorageClass** 定义了存储的"类别"和动态供给方式。当 PVC 指定了 StorageClass 时，K8s 会自动创建对应的 PV。在阿里云 ACK 中，常用的 StorageClass 包括：云盘（ESSD）、NAS 文件系统、OSS 对象存储。

**CSI（Container Storage Interface）** 是 K8s 与存储系统交互的标准接口。阿里云提供了 disk-driver、nas-driver、oss-driver 等 CSI 驱动，将阿里云的存储服务接入 K8s 体系。

---

## 实战演练

### 每日学习导航

| Day | 主题 | 文件 |
|-----|------|------|
| Day 8 | 控制平面: etcd + API Server | [day-8-control-plane-1.md](./day-8-control-plane-1.md) |
| Day 9 | 控制平面: Scheduler + Controller Manager | [day-9-control-plane-2.md](./day-9-control-plane-2.md) |
| Day 10 | 工作负载: Deployment + StatefulSet + DaemonSet | [day-10-workloads-1.md](./day-10-workloads-1.md) |
| Day 11 | 工作负载: Pod 生命周期 + 资源管理 + HPA | [day-11-workloads-2.md](./day-11-workloads-2.md) |
| Day 12 | 网络栈: CNI + Service + DNS | [day-12-networking-1.md](./day-12-networking-1.md) |
| Day 13 | 网络栈: Ingress + NetworkPolicy | [day-13-networking-2.md](./day-13-networking-2.md) |
| Day 14 | 存储体系 + 综合实践 | [day-14-storage-practice.md](./day-14-storage-practice.md) |

### 本周自测

完成本周学习后，请完成 [checkpoint.md](./checkpoint.md) 中的自测题。

### 本周实践项目

**项目 P2**: [生产级应用全栈编排](../projects/p2-production-app-orchestration.md)

---

## 配置示例

### HPA 配置（多指标）

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-app-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "1000"
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
```

### NetworkPolicy 配置

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: backend-policy
  namespace: production
spec:
  podSelector:
    matchLabels:
      tier: backend
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          tier: frontend
    ports:
    - port: 8080
      protocol: TCP
  egress:
  - to:
    - podSelector:
        matchLabels:
          app: database
    ports:
    - port: 3306
      protocol: TCP
  - to:
    - namespaceSelector:
        matchLabels:
          name: kube-system
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - port: 53
      protocol: UDP
```

---

## 常见问题

### Q1: etcd 集群应该部署几个节点？

推荐 3 个或 5 个节点（奇数个）。Raft 协议需要多数派同意才能提交数据，所以 3 节点允许 1 个问题，5 节点允许 2 个问题。超过 5 个节点会增加写入延迟但不提高容错能力，不建议。在 ACK 托管版中，etcd 由阿里云管理，无需关心。

### Q2: Deployment 和 StatefulSet 如何选择？

如果应用满足以下条件之一，使用 StatefulSet：需要稳定的网络标识、需要稳定的持久存储、需要有序的部署和扩展。否则使用 Deployment。大多数 Web 应用、API 服务都是无状态的，应该使用 Deployment。数据库、消息队列等有状态应用应该使用 StatefulSet。

### Q3: Service 和 Ingress 有什么区别？

Service 是四层（TCP/UDP）负载均衡，工作在传输层。Ingress 是七层（HTTP/HTTPS）负载均衡，工作在应用层，支持基于域名和路径的路由。如果你只需要暴露一个服务，使用 LoadBalancer 类型的 Service 即可。如果需要暴露多个服务并根据域名/路径路由，使用 Ingress。

### Q4: 本周最难的内容是什么？

网络栈（Day 12-13）通常是初学者觉得最难的部分。网络涉及 CNI、Service、DNS、Ingress 等多个层次的交互，调试网络问题也相对复杂。建议多花时间实践 Day 12-13 的实操任务。

### Q5: 如何调试网络策略？

网络策略需要 CNI 插件支持（如 Calico、Cilium、Terway）。调试方法：1）使用 `kubectl describe networkpolicy <name>` 确认策略规则；2）在 Pod 中使用 `curl` 或 `wget` 测试连通性；3）使用 Calico 的 `calicoctl` 或 Cilium 的 `cilium` CLI 查看策略生效状态；4）检查 Pod 的标签是否与 NetworkPolicy 的 selector 匹配。

### Q6: PV 和 PVC 的绑定过程是怎样的？

K8s 的 PersistentVolume Controller 持续监控未绑定的 PVC 和可用的 PV。绑定条件：PVC 的 storageClassName 与 PV 匹配（或都不指定）、PVC 的 accessModes 是 PV accessModes 的子集、PVC 的 storage 不超过 PV 的 capacity。如果使用动态供给，CSI 驱动会先创建底层存储，再创建 PV，最后绑定到 PVC。

---

## 要点总结

| 模块 | 核心知识点 | 学习日 |
|------|-----------|--------|
| 控制平面 | etcd Raft、API Server 请求链、Scheduler 调度算法 | Day 8-9 |
| 工作负载 | Deployment 滚动更新、StatefulSet 有状态管理、HPA 自动伸缩 | Day 10-11 |
| 网络栈 | CNI 选型、Service 四种类型、Ingress 七层路由、NetworkPolicy | Day 12-13 |
| 存储体系 | PV/PVC 绑定、StorageClass 动态供给、CSI 驱动 | Day 14 |

---

## 延伸阅读

- [etcd 深入](../../集群基础/11-etcd-deep-dive.md)
- [API Server 深入](../../集群基础/12-apiserver-deep-dive.md)
- [Scheduler 深入](../../集群基础/20-kube-scheduler-deep-dive.md)
- [Deployment 生产模式](../../工作负载/02-deployment-production-patterns.md)
- [StatefulSet 高级操作](../../工作负载/03-statefulset-advanced-operations.md)
- [网络架构总览](../../网络/01-network-architecture-overview.md)
- [Service 概念与类型](../../网络/06-service-concepts-types.md)
- [Ingress 基础](../../网络/19-ingress-fundamentals.md)
- [存储架构总览](../../存储/01-storage-architecture-overview.md)
- [StorageClass 动态供给](../../存储/04-storageclass-dynamic-provisioning.md)

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[实体/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[概念/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[概念/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[技能/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[技能/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[平台工程/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[实体/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
