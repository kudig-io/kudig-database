---
title: 'Week 3: 节点与工作负载管理 (Days 15-21)'
description: 'title: Week 3: 节点与工作负载管理'
summary: 'title: Week 3: 节点与工作负载管理'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- coredns
- containerd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- 'Week 3: 节点与工作负载管理 (Days 15-21) 是什么'
- '如何 Week 3: 节点与工作负载管理 (Days 15-21)'
trigger_keywords:
- Week
- '3:'
- 节点与工作负载管理
- Days
- 15-21
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Week 3: 节点与工作负载管理
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - ACK week 3 node [[系统基础/知识字典/workloads/workload-management.md|workload management]] curriculum
  - [[Kubernetes|Kubernetes]] node pool management learning path
  - Pod lifecycle scheduling management
  - Kubernetes core components operations
  - Week 3 project based learning
trigger_keywords:
  - week 3
  - node
  - workload
  - 节点
  - 工作负载
  - nodepool
  - 节点池
  - Pod
  - 调度
  - component
  - 组件
reading_level: intermediate
audience:
  - All week 3 learners
  - ACK operators
  - SRE engineers
estimated_read_time: 30min
related_domains:
  - domain-3-node
  - domain-9-workload
  - 云厂商
related_topics:
  - node-basics
  - node-advanced
  - nodepool-basics
  - nodepool-advanced
  - pod-basics
  - pod-advanced
  - component-ops
---

# Week 3: 节点与工作负载管理 (Days 15-21)

## 概述

第三周进入 Kubernetes 运维的核心实战领域——节点管理与工作负载管理。在前两周中，你已经了解了集群的生命周期管理（创建、删除、升级）和安全监控体系。本周将深入到集群内部的日常运维操作：如何管理 Node 节点、如何使用节点池实现高效运维、如何管理 Pod 的生命周期与调度、以及如何维护 K8s 核心组件的稳定运行。

节点和工作负载是 Kubernetes 最基础也最重要的两个概念。节点是集群的计算资源单元，工作负载（尤其是 Pod）是应用的运行载体。一个优秀的 K8s 运维工程师需要深刻理解这两个层面的工作原理，才能在问题发生时快速定位问题、在架构设计时做出合理决策。

### 学习目标

- 深入理解 Node 节点的架构组成、状态机制与日常管理操作
- 掌握 ACK 节点池的概念、创建配置、扩缩容与生命周期管理
- 理解 Pod 的完整生命周期、健康检查机制与调度策略
- 掌握 K8s 核心组件（kube-apiserver、etcd、kube-scheduler 等）的运维方法
- **产出**: 能够独立管理节点池、排查 Pod 问题、维护 K8s 核心组件

---

## 核心概念详解

### Node 节点架构深度解析

Kubernetes 中的每个 Node 节点都运行着三个核心组件：**kubelet**、**kube-proxy** 和 **容器运行时（Container Runtime）**。

**kubelet** 是节点上的"大管家"。它通过 Watch 机制持续监听 API Server 上分配到本节点的 Pod 定义，然后调用容器运行时来创建、启动、停止容器。kubelet 还负责执行探针检查（liveness、readiness、startup），并向 API Server 汇报节点状态和 Pod 状态。kubelet 的配置文件通常位于 `/etc/kubernetes/kubelet-config.yaml`，你可以通过修改该文件来调整节点级别的参数，如最大 Pod 数量、资源预留等。

kubelet 的关键启动参数包括：

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `--max-pods` | 节点上最大 Pod 数量 | 110 |
| `--pod-cidr` | Pod IP 地址范围 | 根据集群配置 |
| `--eviction-hard` | 硬驱逐阈值 | memory.available<100Mi |
| `--system-reserved` | 系统资源预留 | cpu=0,memory=0 |
| `--kube-reserved` | K8s 组件资源预留 | cpu=0,memory=0 |
| `--max-open-files` | 最大打开文件数 | 1000000 |

**kube-proxy** 负责实现 Service 的网络转发规则。它监听 API Server 上 Service 和 Endpoints 的变化，并在节点上维护相应的 iptables 或 IPVS 规则。当请求到达 Service 的 ClusterIP 时，kube-proxy 配置的规则会将流量转发到实际的 Pod IP。IPVS 模式在大规模集群中性能优于 iptables 模式，因为它使用哈希表查找而非线性遍历。

**容器运行时**（通常是 containerd）负责实际的容器生命周期管理——拉取镜像、创建容器命名空间、启动容器进程。Kubernetes 通过 CRI（Container Runtime Interface）与容器运行时交互，这意味着你可以替换不同的运行时实现（containerd、CRI-O 等）而不影响上层功能。

### 节点状态与 Conditions

节点的健康状态通过 Conditions 机制来报告。每个节点有五种核心 Condition：

- **Ready**: 节点是否准备好接收 Pod 调度。True 表示健康，False 表示不健康，Unknown 表示 kubelet 未在规定时间内上报状态（通常是 40 秒）
- **MemoryPressure**: 节点内存是否紧张。当可用内存低于阈值（默认为系统内存的 5%）时变为 True
- **DiskPressure**: 节点磁盘是否紧张。当文件系统的可用空间低于阈值时变为 True
- **PIDPressure**: 节点进程数是否过多。当运行中的进程数量接近内核限制时变为 True
- **NetworkUnavailable**: 节点网络是否配置正确。通常由网络插件在初始化完成后设置为 False

当节点出现 MemoryPressure 或 DiskPressure 时，kubelet 会触发驱逐（Eviction）机制，按照优先级顺序终止 Pod 以释放资源。理解这个机制对于排查 Pod 被意外驱逐的问题至关重要。

驱逐优先级从低到高：

| 优先级 | Pod 类型 | 说明 |
|--------|---------|------|
| 最低 | BestEffort | 未设置 requests/limits 的 Pod |
| 低 | Burstable | 设置了 requests 但未设置 limits，或 limits > requests |
| 高 | Guaranteed | requests == limits 的 Pod |
| 最高 | 系统组件 | kube-system 中的 Pod |
| 不驱逐 | DaemonSet | DaemonSet 管理的 Pod 默认不会被驱逐 |

### ACK 节点池的设计哲学

在阿里云 ACK 中，节点池（NodePool）是一个非常重要的抽象概念。它将一组具有相同配置（实例规格、网络配置、标签、污点等）的节点组织在一起进行统一管理。

**托管节点池** 是 ACK 的特色功能。启用托管模式后，ACK 平台会自动处理节点的运维工作：当节点出现 NotReady 状态超过一定时间后，系统会自动创建新节点并替换问题节点；当有新的安全补丁或 K8s 版本更新时，系统可以自动执行节点轮换。这极大减轻了运维人员的日常工作负担。

**自管理节点池** 则给予用户完全的控制权。用户需要自行处理节点的修复、升级和替换。适用于有特殊定制需求的场景，如需要安装特定的内核模块、使用自定义的操作系统镜像等。

生产环境中的节点池设计通常遵循"分层隔离"原则：

- **系统节点池**: 专门运行 K8s 系统组件（如 CoreDNS、Ingress Controller、监控 Agent），使用污点（Taint）阻止业务 Pod 调度
- **业务节点池**: 运行应用工作负载，可以按业务类型进一步细分（如在线服务池、离线任务池）
- **专用节点池**: 运行 GPU 任务、高性能计算等特殊工作负载，使用污点和标签实现精确调度

### Pod 生命周期与调度

Pod 是 Kubernetes 中最小的可部署单元。理解 Pod 的生命周期对于日常运维至关重要。

Pod 的生命周期包括以下阶段：Pending（等待调度或拉取镜像）→ Running（至少一个容器正在运行）→ Succeeded（所有容器成功退出）→ Failed（至少一个容器异常退出）→ Unknown（无法获取 Pod 状态，通常是节点失联）。

**健康检查（探针）** 是保证应用可用性的关键机制：

- **livenessProbe**: 存活探针。检测失败时，K8s 会重启容器。适用于检测死锁等需要重启才能恢复的场景
- **readinessProbe**: 就绪探针。检测失败时，Pod 会从 Service 的 Endpoints 中移除，不再接收流量。适用于应用启动慢或依赖外部服务的场景
- **startupProbe**: 启动探针（K8s 1.18+）。用于判断容器是否已完成初始化。在 startupProbe 成功之前，其他探针不会执行

**调度策略** 决定了 Pod 被分配到哪个节点：

- **nodeSelector**: 最简单的调度约束，通过标签精确匹配节点
- **nodeAffinity**: 更灵活的节点亲和性，支持 In、NotIn、Exists 等运算符，支持软性和硬性两种约束
- **podAffinity/podAntiAffinity**: Pod 之间的亲和性/反亲和性，用于将相关的 Pod 调度到同一拓扑域，或将它们分散开来
- **Taint 和 Toleration**: 污点和容忍度。节点可以设置污点来排斥 Pod，Pod 可以设置容忍度来接受污点

### K8s 核心组件运维

K8s 核心组件的稳定运行是整个集群健康的基础。在托管集群（如 ACK 托管版）中，控制平面组件由平台管理，但了解它们的工作原理和排障方法仍然非常重要。

**kube-apiserver** 是所有操作的入口。它提供 RESTful API，处理认证、授权、准入控制，并将数据持久化到 etcd。API Server 的性能直接影响整个集群的响应速度。常见的性能问题包括：请求速率过高、etcd 写入延迟大、大 List 请求占用过多内存。

**etcd** 是集群的状态存储。它使用 Raft 协议保证数据一致性。etcd 的健康直接决定了集群的可用性。关键运维指标包括：磁盘写入延迟（建议低于 10ms）、数据库大小（建议不超过 8GB）、Leader 选举频率（频繁选举意味着不稳定）。

**kube-scheduler** 负责将未调度的 Pod 分配到合适的节点。调度过程分为两个阶段：Filter（过滤不满足条件的节点）和 Score（对满足条件的节点打分排序）。理解调度算法有助于排查 Pod Pending 问题。

**kube-controller-manager** 运行着多种控制器（Deployment Controller、ReplicaSet Controller、Node Controller 等）。每个控制器通过 Watch/List 机制监听资源变化，并通过 Reconcile 循环将实际状态向期望状态收敛。

---

## 实战演练

### 每日学习导航

| Day | 主题 | 文件 |
|-----|------|------|
| Day 15 | Node 节点基础 | [day-15-node-basics.md](./day-15-node-basics.md) |
| Day 16 | Node 节点进阶 | [day-16-node-advanced.md](./day-16-node-advanced.md) |
| Day 17 | 节点池基础 | [day-17-nodepool-basics.md](./day-17-nodepool-basics.md) |
| Day 18 | 节点池进阶 | [day-18-nodepool-advanced.md](./day-18-nodepool-advanced.md) |
| Day 19 | Pod 容器组基础 | [day-19-pod-basics.md](./day-19-pod-basics.md) |
| Day 20 | Pod 容器组进阶 | [day-20-pod-advanced.md](./day-20-pod-advanced.md) |
| Day 21 | K8S 组件运维 | [day-21-component-ops.md](./day-21-component-ops.md) |

### 环境准备

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确认集群状态
kubectl get nodes -o wide
# 预期输出:
# NAME     STATUS   ROLES    AGE   VERSION   INTERNAL-IP   OS-IMAGE
# node-1   Ready    <none>   30d   v1.28.3   10.0.0.1      Alibaba Cloud Linux
# node-2   Ready    <none>   30d   v1.28.3   10.0.0.2      Alibaba Cloud Linux
# node-3   Ready    <none>   30d   v1.28.3   10.0.0.3      Alibaba Cloud Linux

kubectl get pods -A
# 预期输出: 所有命名空间的 Pod 状态

# 创建本周练习用的命名空间
kubectl create namespace week3-practice
# 预期输出: namespace/week3-practice created

# 查看当前节点标签
kubectl get nodes --show-labels
# 预期输出: 每个节点的标签列表

# 查看节点详细信息（重点关注 Capacity、Allocatable、Conditions）
kubectl describe node $(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')
# 预期输出:
# Capacity:
#   cpu:                4
#   ephemeral-storage:  500Gi
#   memory:             16384Mi
#   pods:               110
# Allocatable:
#   cpu:                3800m
#   ephemeral-storage:  450Gi
#   memory:             15384Mi
#   pods:               110
# Conditions:
#   Type             Status  LastHeartbeatTime
#   Ready            True    ...
#   MemoryPressure   False   ...
#   DiskPressure     False   ...
```
### 本周自测

完成本周学习后，请完成 [checkpoint.md](./checkpoint.md) 中的自测题。

### 本周实践项目

**项目 P3**: [节点与工作负载运维实战](../projects/p3-node-workload-management.md)

---

## 配置示例

### 节点资源预留配置

```yaml
# kubelet 配置片段 (/etc/kubernetes/kubelet-config.yaml)
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
maxPods: 110
podCidr: "172.20.0.0/24"
systemReserved:
  cpu: "500m"
  memory: "512Mi"
  ephemeral-storage: "1Gi"
kubeReserved:
  cpu: "500m"
  memory: "512Mi"
  ephemeral-storage: "1Gi"
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"
evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"
evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "2m"
```

### 多节点池架构配置

```bash
# 系统节点池
aliyun cs POST /clusters/<cluster_id>/nodepools --body '{
  "nodepool_info": {"name": "system-pool"},
  "scaling_group": {
    "vswitch_ids": ["<vsw-id>"],
    "instance_types": ["ecs.g6.xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "desired_size": 2
  },
  "kubernetes_config": {
    "labels": [{"key": "node-role", "value": "system"}],
    "taints": [{"key": "node-role", "value": "system", "effect": "NoSchedule"}]
  }
}'

# 业务节点池
aliyun cs POST /clusters/<cluster_id>/nodepools --body '{
  "nodepool_info": {"name": "app-pool"},
  "scaling_group": {
    "vswitch_ids": ["<vsw-id>"],
    "instance_types": ["ecs.g6.2xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "desired_size": 3
  },
  "kubernetes_config": {
    "labels": [{"key": "node-role", "value": "app"}]
  },
  "auto_scaling": {
    "enable": true,
    "min_instances": 2,
    "max_instances": 10
  }
}'
```

---

## 常见问题

### Q1: 节点 NotReady 后上面的 Pod 会怎样？

节点 NotReady 后，K8s 会等待 `pod-eviction-timeout`（默认 5 分钟），之后将节点上的 Pod 标记为需要驱逐。但对于 DaemonSet 管理的 Pod 不会被驱逐。如果 Pod 使用了本地存储（emptyDir），数据可能会丢失。建议在关键应用中使用 PersistentVolume 而非 emptyDir。

### Q2: 托管节点池的自动修复是如何工作的？

托管节点池会定期检查节点健康状态。当检测到节点连续 NotReady 超过配置的时间阈值后，系统会自动创建一个新的节点来替换问题节点。在新节点 Ready 后，系统会安全地排空（drain）问题节点上的 Pod，然后释放问题节点的 ECS 实例。

### Q3: Pod 一直处于 Pending 状态怎么排查？

Pod Pending 通常有两个原因：一是没有节点满足调度条件（资源不足、标签不匹配、污点排斥等），二是 PVC 无法绑定到 PV。排查步骤：`kubectl describe pod <pod-name>` 查看 Events 部分，通常会明确说明调度失败的原因。

### Q4: 如何选择 livenessProbe 和 readinessProbe？

如果应用在启动后就不应该被重启（除非真的崩溃），不要使用 livenessProbe，只用 readinessProbe。livenessProbe 失败会触发容器重启，可能导致服务雪崩。推荐做法：livenessProbe 只检查最基本的存活状态（如 TCP 端口是否监听），readinessProbe 检查更完整的服务就绪状态（如 HTTP 健康检查接口返回 200）。

### Q5: kubelet 资源预留如何配置？

生产环境建议配置 `systemReserved`（为系统进程预留）和 `kubeReserved`（为 K8s 组件预留），避免系统进程和 K8s 组件与业务 Pod 争抢资源。推荐值：系统预留 CPU 500m / 内存 512Mi，K8s 预留 CPU 500m / 内存 512Mi。具体值需要根据节点规格和应用负载调整。

### Q6: 如何查看节点的实际资源使用情况？

使用 `kubectl top nodes` 查看节点级别的 CPU 和内存使用（需要 metrics-server）。使用 `kubectl describe node <name>` 查看 Allocatable 和已分配的资源量。注意：Allocatable = Capacity - systemReserved - kubeReserved - evictionThreshold。

---

## 要点总结

| 主题 | 关键知识点 | 学习日 |
|------|-----------|--------|
| 节点管理 | kubelet、kube-proxy、Conditions、资源查看 | Day 15-16 |
| 节点池 | 创建配置、托管/自管理、扩缩容 | Day 17-18 |
| Pod 管理 | 生命周期、探针、调度策略 | Day 19-20 |
| 组件运维 | API Server、etcd、Scheduler、Controller | Day 21 |

本周是整个培训中知识点最密集的一周。节点和 Pod 是日常运维操作最频繁的对象，务必确保每一步操作都亲手实践。

---

## 延伸阅读

- [K8s 架构与组件深入](../../集群基础/02-core-components-deep-dive.md)
- [ACK 服务总览](../../云厂商/04-alicloud-ack/alicloud-ack-overview.md)
- [ECS 计算资源](../../云厂商/04-alicloud-ack/240-ack-ecs-compute.md)
- [Pod 生命周期事件](../../工作负载/11-pod-lifecycle-events.md)
- [HPA/VPA 自动伸缩](../../工作负载/21-hpa-vpa-autoscaling.md)
- [节点 NotReady 诊断](../../故障诊断/06-node-notready-diagnosis.md)
- [Pod 综合排障](../../故障诊断/08-pod-comprehensive-troubleshooting.md)
- [集群自动伸缩排障](../../故障诊断/28-cluster-autoscaler-troubleshooting.md)

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

```

<!-- risk-assessed -->
