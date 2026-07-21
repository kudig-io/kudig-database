---
title: 调度与资源管理知识词典
description: 涵盖 Kubernetes 调度器、自动扩缩容、资源管理、拓扑约束、GPU 调度等完整术语体系与技术参考
summary: 调度与资源管理领域词典，覆盖 Scheduler、HPA/VPA、Cluster Autoscaler、Volcano、KEDA、拓扑约束等核心概念
category: dictionary
tags:
- dictionary
- scheduling
- autoscaling
- resource-management
- topology
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: intermediate
audience:
- 平台工程师
- SRE
- 开发工程师
---

# 调度与资源管理知识词典（Scheduling & Resource Management）

> 本词典覆盖 Kubernetes 调度与资源管理领域的核心术语、技术组件及工程实践，是平台工程师和 SRE 优化集群资源利用率的权威参考。

## 领域概述

调度是 Kubernetes 的核心能力，决定 Pod 在哪个节点运行：

- **调度决策**：基于资源、亲和性、污点、拓扑等约束选择最优节点
- **自动扩缩容**：根据负载自动调整 Pod/节点数量
- **资源治理**：配额、限制、优先级、抢占、驱逐
- **批量调度**：AI/HPC 场景的 Gang Scheduling、队列管理

## 核心术语定义

### 调度器核心

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| kube-scheduler | K8s 默认调度器，为 Pod 选择节点 | 插件化架构、可扩展 |
| Scheduling Framework | 调度框架，定义调度插件扩展点 | PreFilter/Filter/Score/Reserve/Bind |
| Node Selector | 最简单的节点选择约束 | 标签匹配 |
| Affinity | 节点/Pod 亲和性调度 | required/preferred、拓扑域 |
| Anti-Affinity | 反亲和性，打散 Pod 分布 | 高可用必备 |
| Taint/Toleration | 节点污点与 Pod 容忍 | NoSchedule/NoExecute/PreferNoSchedule |
| Topology Spread | 拓扑分布约束，跨 AZ/Zone 均匀分布 | maxSkew、topologyKey |
| Pod Priority | Pod 优先级，影响调度和抢占 | PriorityClass |
| Preemption | 高优先级 Pod 抢占低优先级 Pod 资源 | 调度失败时触发 |

### 资源管理

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| Resource Request | 容器最小资源保证 | 调度依据 |
| Resource Limit | 容器资源上限 | CPU 限流/Memory OOMKill |
| QoS Class | 服务质量等级 | Guaranteed/Burstable/BestEffort |
| ResourceQuota | 命名空间资源配额 | 限制总量/Pod 数 |
| LimitRange | 默认资源限制 | 自动注入 requests/limits |
| Pod Overhead | Pod 基础设施额外开销 | 沙箱容器额外资源 |
| DRA (Dynamic Resource Allocation) | 动态资源分配（GPU 等） | K8s 1.30+ Beta |
| Bin Packing | 资源装箱算法，提高节点利用率 | MostAllocated 策略 |

### 自动扩缩容

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| HPA | 水平 Pod 自动扩缩容 | 基于 CPU/Memory/自定义指标 |
| VPA | 垂直 Pod 自动扩缩容 | 自动调整 requests/limits |
| Cluster Autoscaler | 节点级自动扩缩容 | 云厂商节点池 |
| Karpenter | AWS 新一代节点自动扩缩 | 直接创建节点、更快 |
| KEDA | 事件驱动自动扩缩容 | 支持 60+ 事件源 |

### 批量与高级调度

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Volcano | CNCF 批量调度引擎 | Gang Scheduling、队列、公平共享 |
| Gang Scheduling | 一组 Pod 全部调度或全部不调度 | Volcano/Kueue |
| Kueue | K8s 原生作业队列管理 | 配额、优先级、抢占 |
| Koordinator | 阿里开源混部调度系统 | 在离线混部、GPU 共享 |
| HAMi | 异构 GPU 共享调度 | vGPU、MIG 管理 |
| KAITO | AI 模型自动部署 | GPU 节点自动配置 |
| KubeFleet | 多集群资源调度 | 跨集群工作负载分发 |

### 驱逐与节点压力

| 术语 | 定义 | 触发条件 |
|------|------|----------|
| Node Pressure Eviction | 节点资源压力驱逐 | Memory/Disk/PID 压力 |
| API Initiated Eviction | API 发起的主动驱逐 | Eviction API、PDB 保护 |
| Pod Disruption Budget | Pod 中断预算 | 保证最小可用数 |

## 技术组件索引

### 调度器核心类

- [[系统基础/知识字典/scheduling/kubernetes-scheduler.md|Kubernetes Scheduler（调度器）]]
- [[系统基础/知识字典/scheduling/scheduler.md|Scheduler（调度原理）]]
- [[系统基础/知识字典/scheduling/scheduling-framework.md|Scheduling Framework（调度框架）]]
- [[系统基础/知识字典/scheduling/scheduler-performance-tuning.md|调度器性能调优]]

### 调度约束类

- [[系统基础/知识字典/scheduling/affinity.md|Affinity（亲和性）]]
- [[系统基础/知识字典/scheduling/anti-affinity.md|Anti-Affinity（反亲和）]]
- [[系统基础/知识字典/scheduling/node-selector.md|Node Selector（节点选择）]]
- [[系统基础/知识字典/scheduling/taint.md|Taint（污点）]]
- [[系统基础/知识字典/scheduling/toleration.md|Toleration（容忍）]]
- [[系统基础/知识字典/scheduling/taints-and-tolerations.md|Taints & Tolerations（综合）]]
- [[系统基础/知识字典/scheduling/topology-spread-constraints.md|Topology Spread Constraints]]
- [[系统基础/知识字典/scheduling/pod-topology-spread-constraints.md|Pod 拓扑分布约束]]
- [[系统基础/知识字典/scheduling/topology.md|Topology（拓扑）]]
- [[系统基础/知识字典/scheduling/assigning-pods-to-nodes.md|Pod 节点分配]]
- [[系统基础/知识字典/scheduling/pod-priority-and-preemption.md|优先级与抢占]]
- [[系统基础/知识字典/scheduling/pod-scheduling-readiness.md|Pod 调度就绪]]
- [[系统基础/知识字典/scheduling/node-declared-features.md|节点声明特性]]

### 资源管理类

- [[系统基础/知识字典/scheduling/resource-request.md|Resource Request]]
- [[系统基础/知识字典/scheduling/resource-limit.md|Resource Limit]]
- [[系统基础/知识字典/scheduling/resource-quota.md|Resource Quota]]
- [[系统基础/知识字典/scheduling/limitrange.md|LimitRange]]
- [[系统基础/知识字典/scheduling/qos.md|QoS（服务质量）]]
- [[系统基础/知识字典/scheduling/pod-overhead.md|Pod Overhead]]
- [[系统基础/知识字典/scheduling/resource-bin-packing.md|Resource Bin Packing]]
- [[系统基础/知识字典/scheduling/dynamic-resource-allocation.md|DRA（动态资源分配）]]

### 自动扩缩容类

- [[系统基础/知识字典/scheduling/hpa.md|HPA（水平扩缩）]]
- [[系统基础/知识字典/scheduling/vpa.md|VPA（垂直扩缩）]]
- [[系统基础/知识字典/scheduling/cluster-autoscaler.md|Cluster Autoscaler]]
- [[系统基础/知识字典/scheduling/karpenter-autoscaling.md|Karpenter]]
- [[系统基础/知识字典/scheduling/keda.md|KEDA（事件驱动扩缩）]]

### 批量与高级调度类

- [[系统基础/知识字典/scheduling/volcano.md|Volcano（批量调度）]]
- [[系统基础/知识字典/scheduling/gang-scheduling.md|Gang Scheduling]]
- [[系统基础/知识字典/scheduling/koordinator.md|Koordinator（混部调度）]]
- [[系统基础/知识字典/scheduling/hami.md|HAMi（GPU 共享）]]
- [[系统基础/知识字典/scheduling/kaito.md|KAITO（AI 部署）]]
- [[系统基础/知识字典/scheduling/kubefleet.md|KubeFleet（多集群调度）]]

### 驱逐类

- [[系统基础/知识字典/scheduling/node-pressure-eviction.md|节点压力驱逐]]
- [[系统基础/知识字典/scheduling/api-initiated-eviction.md|API 发起驱逐]]

## 调度流程深度解析

### kube-scheduler 调度周期

```
Pod 调度流程:

1. Pod 创建 (spec.nodeName 为空)
   │
2. 调度队列 (ActiveQ)
   │
3. 调度周期 (Scheduling Cycle) - 串行
   ├── PreFilter: 预处理/检查 (如 PVC 绑定)
   ├── Filter: 过滤不满足条件的节点
   │   ├── NodeResourcesFit (资源是否足够)
   │   ├── NodeAffinity (节点亲和)
   │   ├── TaintToleration (污点容忍)
   │   └── PodTopologySpread (拓扑约束)
   ├── PostFilter: 过滤后无可用节点时触发
   │   └── Preemption (抢占低优先级 Pod)
   ├── PreScore: 打分预处理
   ├── Score: 节点打分
   │   ├── LeastAllocated (资源均衡)
   │   ├── MostAllocated (装箱优化)
   │   └── BalancedAllocation (CPU/Mem 均衡)
   └── Reserve: 预留资源
   │
4. 绑定周期 (Binding Cycle) - 并行
   ├── PreBind: 绑定前准备 (如创建 PV)
   ├── Bind: 绑定 Pod 到节点
   └── PostBind: 绑定后清理
   │
5. Pod 在目标节点启动
```

### HPA 扩缩容算法

```
HPA 扩缩容决策:

期望副本数 = ceil[当前副本数 × (当前指标值 / 目标指标值)]

示例:
- 当前: 3 Pod, CPU 使用率 80%
- 目标: CPU 50%
- 期望: ceil(3 × 80/50) = ceil(4.8) = 5 Pod

扩缩容行为控制 (behavior):
- scaleUp.stabilizationWindowSeconds: 0 (快速扩容)
- scaleDown.stabilizationWindowSeconds: 300 (慢速缩容)
- 策略: Pods/Percent 限制每次扩缩幅度
```

## 生产最佳实践

### 调度策略

1. **高可用服务**：必须配置 Pod Anti-Affinity + Topology Spread
2. **资源请求**：基于压测数据设置 requests，不要拍脑袋
3. **优先级分层**：核心服务 Priority 1000，普通服务 100，批量任务 10
4. **节点池隔离**：通过 Taint 隔离 GPU/高内存/特殊硬件节点

### 自动扩缩容

1. **HPA + VPA 不混用**：基于 CPU/Memory 的 HPA 与 VPA 冲突
2. **缩容冷却**：scaleDown stabilization 至少 5min，避免抖动
3. **PDB 保护**：有状态服务必须配置 PodDisruptionBudget
4. **Cluster Autoscaler + HPA 联动**：HPA 扩 Pod → CA 扩节点

### GPU 调度

1. **GPU 节点 Taint**：`nvidia.com/gpu=present:NoSchedule` 防止普通 Pod 调度
2. **MIG 分区**：多租户场景使用 MIG 硬件隔离
3. **GPU 共享**：开发/测试用 HAMi/vGPU，生产用 MIG
4. **队列管理**：Kueue 管理 GPU 配额，避免资源死锁

## 故障排查要点

| 故障现象 | 可能原因 | 排查方向 |
|----------|----------|----------|
| Pod Pending | 资源不足/亲和不满足/污点未容忍 | `kubectl describe pod` 查看 Events |
| HPA 不扩容 | metrics-server 异常/指标未暴露 | 检查 `kubectl get hpa`、metrics API |
| 节点 NotReady | kubelet 异常/网络不通 | 检查节点状态、kubelet 日志 |
| 抢占未触发 | PriorityClass 未配置/PDB 保护 | 检查 PriorityClass、PDB 配置 |
| GPU Pod Pending | Device Plugin 异常/GPU 资源不足 | 检查 nvidia-device-plugin、`nvidia-smi` |
| 驱逐频繁 | 节点资源压力/requests 设置过低 | 检查节点资源、调整 requests |

## 学习路径

```
基础: Node Selector → Affinity → Taint/Toleration
进阶: HPA/VPA → Cluster Autoscaler → Topology Spread
高级: Scheduling Framework → Volcano → Koordinator
专家: DRA → GPU 调度优化 → 混部调度 → 自定义调度器
```

## 参考链接

- https://kubernetes.io/docs/concepts/scheduling-eviction/
- https://kubernetes.io/docs/concepts/scheduling-eviction/scheduling-framework/
- https://volcano.sh/
- https://keda.sh/
- https://koordinator.sh/
- https://project-hami.io/

## Related

- [[系统基础/知识字典/configuration/resource-management-for-pods-and-containers.md|资源管理]]
- [[系统基础/知识字典/specialized-workloads/gpu-resource-management-and-partitioning.md|GPU 资源管理]]
- [[系统基础/知识字典/workloads/deployment.md|Deployment 工作负载]]
- [[系统基础/知识字典/operations/capacity-planning.md|容量规划]]

## 调度配置示例

### 完整的高可用调度配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-server
spec:
  replicas: 6
  template:
    spec:
      # 优先级
      priorityClassName: high-priority  # value: 1000
      # 拓扑分布：跨 3 个 AZ 均匀分布
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: web-server
      # Pod 反亲和：不同节点
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: web-server
              topologyKey: kubernetes.io/hostname
      # 容忍控制平面污点（可选）
      tolerations:
      - key: node-role.kubernetes.io/control-plane
        operator: Exists
        effect: NoSchedule
      containers:
      - name: web
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            memory: 1Gi  # CPU 不设 limits
---
# HPA 配置
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-server-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-server
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 60
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 0
      policies:
      - type: Percent
        value: 100
        periodSeconds: 30
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Pods
        value: 2
        periodSeconds: 60
---
# PDB 保护
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: web-server-pdb
spec:
  minAvailable: 50%
  selector:
    matchLabels:
      app: web-server
```

### Volcano 批量调度配置

```yaml
# Volcano Job: 分布式训练任务
apiVersion: batch.volcano.sh/v1alpha1
kind: Job
metadata:
  name: pytorch-training
spec:
  minAvailable: 4  # Gang Scheduling: 至少 4 个 Pod 同时就绪
  schedulerName: volcano
  queue: gpu-training  # 队列管理
  plugins:
    ssh: []
    svc: []
  tasks:
  - replicas: 1
    name: master
    template:
      spec:
        containers:
        - name: pytorch
          image: pytorch/pytorch:2.2.0-cuda12.1-cudnn8-runtime
          resources:
            limits:
              nvidia.com/gpu: 4
  - replicas: 3
    name: worker
    template:
      spec:
        containers:
        - name: pytorch
          image: pytorch/pytorch:2.2.0-cuda12.1-cudnn8-runtime
          resources:
            limits:
              nvidia.com/gpu: 4
---
# Volcano Queue: GPU 配额管理
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: gpu-training
spec:
  weight: 2
  capability:
    nvidia.com/gpu: 32  # 最多 32 GPU
  reclaimable: true
```

## 生产案例研究

### 案例：电商大促弹性扩缩容

**背景：** 某电商平台双11大促，流量从日常 10万 QPS 飙升到 100万 QPS。

**架构方案：**
- HPA: 基于自定义指标 (QPS) 扩缩 Pod
- Karpenter: 快速创建节点（比 Cluster Autoscaler 快 3x）
- KEDA: 基于消息队列深度扩缩 Worker
- 预热: 大促前 1h 提前扩容到基线

**关键成果：**
- 扩容时间: 从 10min 降至 2min (Karpenter)
- 资源成本: 大促后 2h 内缩容到日常水平
- 零故障: PDB + 滚动更新保障服务连续

## 常用运维命令速查

```bash
# === 调度诊断 ===
# 查看 Pod 调度事件
kubectl describe pod my-pod | grep -A10 "Events"
# 查看调度失败原因
kubectl get events --field-selector reason=FailedScheduling -A
# 查看节点资源分配
kubectl describe node my-node | grep -A10 "Allocated resources"

# === HPA/VPA ===
# 查看 HPA 状态
kubectl get hpa -A
kubectl describe hpa my-hpa
# 查看 VPA 推荐
kubectl get vpa -A -o yaml | grep -A5 "recommendation"

# === 节点管理 ===
# 添加污点
kubectl taint nodes my-node gpu=true:NoSchedule
# 移除污点
kubectl taint nodes my-node gpu=true:NoSchedule-
# 节点不可调度 (cordon)
kubectl cordon my-node
# 驱逐 Pod (drain)
kubectl drain my-node --ignore-daemonsets --delete-emptydir-data

# === Volcano ===
# 查看队列状态
kubectl get queues
# 查看 Volcano Job
kubectl get vcjob -A
# 查看 PodGroup
kubectl get podgroups -A

# === 资源分析 ===
# 查看节点资源利用率
kubectl top nodes
# 查看命名空间配额使用
kubectl describe resourcequota -n my-namespace
# 查看 Pod QoS 分布
kubectl get pods -A -o jsonpath='{range .items[*]}{.status.qosClass}{"\n"}{end}' | sort | uniq -c
```

## 常见问题 FAQ

**Q1: HPA 和 VPA 能同时用吗？**

A: 不能同时基于 CPU/Memory。HPA 和 VPA 都读取相同的 metrics-server 数据，会互相干扰。可以：HPA 基于自定义指标 + VPA 基于 CPU/Memory（updateMode=Off 仅推荐）。

**Q2: Cluster Autoscaler 和 Karpenter 怎么选？**

A: 
- Cluster Autoscaler: 基于节点组/节点池扩缩，支持多云，但较慢（5-10min）
- Karpenter: AWS 专用，直接创建 EC2（跳过节点组），更快（1-2min），支持更灵活的实例选择
AWS 环境优先 Karpenter，多云环境用 Cluster Autoscaler。

**Q3: 为什么 Pod 一直 Pending？**

A: 常见原因排查顺序：
1. `kubectl describe pod` 查看 Events
2. 资源不足 → 检查节点 Allocatable vs Allocated
3. 亲和/反亲和不满足 → 检查标签匹配
4. 污点未容忍 → 检查节点 Taints
5. PVC 未绑定 → 检查 StorageClass/PV
6. PDB 阻止驱逐 → 检查 PDB 配置

**Q4: Gang Scheduling 解决什么问题？**

A: 分布式训练/MPI 任务需要所有 Worker 同时就绪。没有 Gang Scheduling：
- 4 个 Worker，只调度了 3 个，第 4 个 Pending
- 已调度的 3 个空等，浪费 GPU
- 可能死锁：多个任务各占部分资源
Gang Scheduling 确保“全有或全无”。

**Q5: 如何优化节点资源利用率？**

A: 
1. 精确设置 requests（基于压测，非拍脑袋）
2. 使用 Bin Packing 策略（MostAllocated）提高装箱率
3. 在离线混部（Koordinator）：白天在线服务 + 夜间批量任务
4. GPU 共享（HAMi/MIG）：避免 GPU 独占浪费
5. VPA 推荐模式：发现 requests 设置过高的工作负载

## 缩略语表

| 缩写 | 全称 | 说明 |
|------|------|------|
| HPA | Horizontal Pod Autoscaler | 水平 Pod 自动扩缩 |
| VPA | Vertical Pod Autoscaler | 垂直 Pod 自动扩缩 |
| CA | Cluster Autoscaler | 集群自动扩缩 |
| DRA | Dynamic Resource Allocation | 动态资源分配 |
| PDB | Pod Disruption Budget | Pod 中断预算 |
| QoS | Quality of Service | 服务质量等级 |
| AZ | Availability Zone | 可用区 |
| MPI | Message Passing Interface | 消息传递接口 |
| vGPU | Virtual GPU | 虚拟 GPU |

