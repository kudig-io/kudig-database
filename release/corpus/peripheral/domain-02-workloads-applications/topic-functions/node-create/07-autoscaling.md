---
title: 节点弹性伸缩 — Cluster Autoscaler 源码分析
description: 'description: ''## 概述'''
summary: 'description: ''## 概述'''
category: general
tags:
- reference
- prometheus
- pdb
- statefulset
- daemonset
- job
- gpu
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点弹性伸缩 — Cluster Autoscaler 源码分析 是什么
- 如何 节点弹性伸缩 — Cluster Autoscaler 源码分析
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 节点弹性伸缩
- Cluster
- Autoscaler
- 源码分析
- platform
- engineering
- code
- analysis
prerequisites:
- kubectl-basics
- platform-engineering-basics
- prometheus-basics
- gpu-scheduling-basics
---



title: 节点弹性伸缩 Cluster Autoscaler 源码分析
description: '## 概述'
category: functions
tags:
- k8s
- operations
- cluster-management
- prometheus
- pdb
- statefulset
- daemonset
- job
- gpu
last_updated: 2026-05-18
difficulty: advanced
reading_level: advanced
audience:
- Kubernetes 运维工程师
- 平台工程师
- DevOps 工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Cluster Autoscaler working mechanism
- node autoscaling scale up scale down
- Cluster Autoscaler AWS ASG GCP MIG Azure VMSS
- unschedulable pod trigger scale up
- node group expander strategy
trigger_keywords:
- Cluster Autoscaler
- autoscaling
- scale up
- scale down
- node group
- ASG
- MIG
- VMSS
- unschedulable pod
- node pool
- expander
- least-waste
- scale-down-utilization-threshold
- PodDisruptionBudget
- safe-to-evict
related_domains:
- domain-9-orchestration
- domain-01-cluster-fundamentals
related_topics:
- node-create/01-overview
- node-create/08-troubleshooting
- cluster-create/01-overview
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 节点弹性伸缩 — Cluster Autoscaler 源码分析

## 概述

节点弹性伸缩是 Kubernetes 集群实现成本优化和弹性容量的核心能力。Cluster Autoscaler（CA）是 Kubernetes 官方提供的节点自动伸缩组件，它通过监控集群中不可调度的 Pod（unschedulable Pod）来动态增加节点，通过检测空闲节点来减少节点，从而实现集群容量的自动调整。

Cluster Autoscaler 的设计哲学是"按需伸缩"——当有 Pod 因为资源不足无法调度时，自动扩容节点；当节点上的资源利用率持续低于阈值时，自动缩容节点。这种机制特别适用于以下场景：

- **突发流量**：业务流量突增导致 Pod 需要快速扩容
- **成本优化**：在低峰期自动释放空闲节点以降低云资源成本
- **批处理任务**：临时创建大量节点处理批处理任务，完成后自动释放

Cluster Autoscaler 本身运行在集群中作为一个 Deployment，它通过云厂商的 API（如 AWS ASG、GCP MIG、Azure VMSS）来管理节点的生命周期。本文档详细分析 Cluster Autoscaler 的工作原理、配置方法、各云厂商的集成方式以及常见故障排查。

---

## 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| Cluster Autoscaler | `kubernetes/autoscaler/cluster-autoscaler/` | CA 核心逻辑 |
| Node Lifecycle Controller | `pkg/controller/nodelifecycle/` | 节点生命周期管理 |
| Cloud Provider 接口 | `pkg/cloudprovider/` | 云厂商接口定义 |
| AWS Cloud Provider | `k8s.io/cloud-provider-aws/` | AWS 实现 |
| GCP Cloud Provider | `k8s.io/cloud-provider-gcp/` | GCP 实现 |
| Azure Cloud Provider | `k8s.io/legacy-cloud-providers/azure/` | Azure 实现 |

---

## 一、Cluster Autoscaler 工作原理

### 1.1 核心扩缩流程

```
Cluster Autoscaler 工作循环:
  ┌─────────────────────────────────────────────────────────────┐
  │  1. 定期扫描集群状态 (默认每 10 秒)                          │
  └─────────────────────────────────────────────────────────────┘
                            │
                            ▼
  ┌─────────────────────────────────────────────────────────────┐
  │  2. 检查是否有 unschedulable Pod                             │
  │     (Pod 的 condition 中有 PodScheduled=False,               │
  │      reason=Unschedulable)                                   │
  └─────────────────────────────────────────────────────────────┘
                            │
               ┌────────────┴────────────┐
               │ 有 unschedulable Pod     │ 无 unschedulable Pod
               ▼                          ▼
  ┌─────────────────────┐   ┌─────────────────────────────────┐
  │ 3a. 扩容流程        │   │ 3b. 缩容流程                     │
  │ - 计算需要的资源    │   │ - 检查空闲节点                   │
  │ - 选择合适的节点组  │   │ - 检查 Pod 可驱逐性              │
  │ - 调用云 API 扩容   │   │ - 调用云 API 缩容               │
  └─────────────────────┘   └─────────────────────────────────┘
```

### 1.2 扩容决策过程

Cluster Autoscaler 的扩容决策经过以下步骤：

1. **识别 unschedulable Pod**：扫描所有命名空间中的 Pod，找出因资源不足无法调度的 Pod
2. **分析 Pod 需求**：计算这些 Pod 需要的 CPU、内存、GPU 等资源量
3. **评估节点组**：遍历所有配置的节点组（Node Group），计算每个节点组可以提供的资源
4. **选择最优节点组**：优先选择单位资源成本最低的节点组（或使用 `--expander` 策略）
5. **调用云 API**：增加目标节点组的实例数量
6. **等待节点就绪**：新节点启动、注册、通过健康检查后，Pod 被调度到新节点

### 1.3 缩容决策过程

缩容的决策条件比扩容更严格，以避免频繁的节点增减：

```
节点缩容条件:
  ┌─────────────────────────────────────────────────────────────┐
  │  所有条件必须同时满足:                                        │
  │  1. 节点上所有 Pod 的 CPU/内存请求之和低于节点容量的 50%     │
  │     (可通过 --scale-down-utilization-threshold 调整)         │
  │  2. 节点空闲时间超过 --scale-down-unneeded-time (默认 10 分钟) │
  │  3. 节点上没有以下类型的 Pod:                                 │
  │     - DaemonSet Pod (不受影响)                               │
  │     - Pod 有 PodDisruptionBudget 阻止                        │
  │     - Pod 有 controller 不允许缩小                           │
  │     - 非 ReplicaSet/StatefulSet/Job 管理的独立 Pod           │
  │  4. 缩容后节点组不会低于最小实例数                            │
  │  5. 距离上次扩容超过 --scale-down-delay-after-add (默认 10 分钟) │
  └─────────────────────────────────────────────────────────────┘
```

---

## 二、部署 Cluster Autoscaler

### 2.1 AWS EKS 部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
  labels:
    app: cluster-autoscaler
spec:
  replicas: 1
  selector:
    matchLabels:
      app: cluster-autoscaler
  template:
    metadata:
      labels:
        app: cluster-autoscaler
    spec:
      serviceAccountName: cluster-autoscaler
      containers:
      - image: registry.k8s.io/autoscaling/cluster-autoscaler:v1.28.0
        name: cluster-autoscaler
        args:
        - --cloud-provider=aws
        - --nodes=1:10:my-asg-name              # min:max:asg-name
        - --scale-down-delay-after-add=10m       # 扩容后多久允许缩容
        - --scale-down-unneeded-time=10m         # 节点空闲多久后缩容
        - --scale-down-utilization-threshold=0.5 # 资源利用率低于 50% 视为空闲
        - --balance-similar-node-groups          # 平衡相似节点组
        - --skip-nodes-with-system-pods=false    # 允许缩容有 kube-system Pod 的节点
        - --expander=least-waste                 # 扩容策略：最少浪费
        resources:
          limits:
            cpu: 200m
            memory: 300Mi
          requests:
            cpu: 100m
            memory: 200Mi
        volumeMounts:
        - name: ssl-certs
          mountPath: /etc/ssl/certs/ca-certificates.crt
          readOnly: true
      volumes:
      - name: ssl-certs
        hostPath:
          path: /etc/ssl/certs/ca-certificates.crt
```

### 2.2 GCP GKE 部署

GKE 默认内置了 Cluster Autoscaler，可以通过 gcloud 命令启用：

```bash
# 启用 GKE Cluster Autoscaler
gcloud container clusters update my-cluster \
  --enable-autoscaling \
  --min-nodes=1 \
  --max-nodes=10 \
  --node-pool=my-node-pool

# 自定义参数
gcloud container clusters update my-cluster \
  --autoscaling-profile=optimize-utilization  # 优化资源利用率
```

### 2.3 Azure AKS 部署

```bash
# 启用 AKS Cluster Autoscaler
az aks update \
  --resource-group myResourceGroup \
  --name myAKSCluster \
  --enable-cluster-autoscaler \
  --min-count 1 \
  --max-count 10

# 更新节点池配置
az aks nodepool update \
  --resource-group myResourceGroup \
  --cluster-name myAKSCluster \
  --name myNodePool \
  --update-cluster-autoscaler \
  --min-count 1 \
  --max-count 10
```

---

## 三、节点组（Node Group）配置

### 3.1 各云厂商节点组概念

| 云厂商 | 节点组名称 | 配置方式 | 说明 |
|--------|-----------|---------|------|
| AWS | Auto Scaling Group (ASG) | `--nodes=min:max:asg-name` | EC2 实例的自动伸缩组 |
| GCP | Managed Instance Group (MIG) | gcloud 命令 | GCE 实例的托管实例组 |
| Azure | Virtual Machine Scale Set (VMSS) | az 命令 | Azure 虚拟机规模集 |
| 阿里云 | Scaling Group | aliyun CLI | ECS 实例的伸缩组 |

### 3.2 多节点组配置

```bash
# AWS 多 ASG 配置
--nodes=1:10:cpu-asg          # CPU 节点组
--nodes=0:5:gpu-asg           # GPU 节点组（允许缩容到 0）
--nodes=1:5:highmem-asg       # 高内存节点组

# GCP 多节点池
gcloud container clusters update my-cluster \
  --node-pool=cpu-pool --enable-autoscaling --min-nodes=1 --max-nodes=10
gcloud container clusters update my-cluster \
  --node-pool=gpu-pool --enable-autoscaling --min-nodes=0 --max-nodes=5
```

### 3.3 Expander 策略

Cluster Autoscaler 支持多种扩容策略（`--expander` 参数）：

| 策略 | 说明 | 适用场景 |
|------|------|---------|
| `random` | 随机选择满足条件的节点组 | 测试环境 |
| `most-pods` | 选择能调度最多 Pod 的节点组 | Pod 密度优先 |
| `least-waste` | 选择资源浪费最少的节点组（默认推荐） | 生产环境 |
| `price` | 选择成本最低的节点组 | 成本敏感 |
| `priority` | 按用户定义的优先级选择 | 精确控制 |

---

## 四、Pod 与 Autoscaler 的交互

### 4.1 阻止 Pod 被缩容影响

```yaml
# 方法 1: PodDisruptionBudget
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: my-app-pdb
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: my-app

# 方法 2: cluster-autoscaler.kubernetes.io/safe-to-evict 注解
apiVersion: v1
kind: Pod
metadata:
  name: my-pod
  annotations:
    cluster-autoscaler.kubernetes.io/safe-to-evict: "false"  # 阻止缩容
spec:
  containers:
  - name: app
    image: nginx
```

### 4.2 阻止节点被缩容

```yaml
# 在 Node 对象上添加注解
apiVersion: v1
kind: Node
metadata:
  name: my-node
  annotations:
    cluster-autoscaler.kubernetes.io/scale-down-disabled: "true"  # 禁止缩容此节点
```

### 4.3 优先级与抢占

```yaml
# 低优先级工作负载（优先被缩容）
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: low-priority
value: 100
preemptionPolicy: PreemptLowerPriority
globalDefault: false
```

---

## 五、监控与调试

### 5.1 Cluster Autoscaler 日志

```bash
# 查看 CA 日志
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=100

# 增加日志详细程度
# 在 Deployment 的 args 中添加:
- --v=4

# 关键日志:
# "Pod <pod> is unschedulable"          → 检测到无法调度的 Pod
# "Scale up: setting asg xxx to 5"      → 扩容决策
# "Scale down: removing node xxx"        → 缩容决策
# "No pod can be moved from node xxx"    → 缩容被阻止
```

### 5.2 Cluster Autoscaler 状态

```bash
# 查看 CA 配置
kubectl get configmap cluster-autoscaler-status -n kube-system -o yaml

# 查看 CA 当前状态（写在 ConfigMap 中）
kubectl describe configmap cluster-autoscaler-status -n kube-system
```

### 5.3 关键指标

```bash
# Prometheus 指标
cluster_autoscaler_nodes_count{group="cpu-asg"}           # 当前节点数
cluster_autoscaler_scaled_up_nodes_total{group="cpu-asg"} # 扩容节点总数
cluster_autoscaler_scaled_down_nodes_total                # 缩容节点总数
cluster_autoscaler_unschedulable_pods_count               # 不可调度 Pod 数
cluster_autoscaler_last_activity                          # 最后一次活动时间
```

---

## 六、常见错误与排查

| 错误 | 原因 | 排查命令 | 解决方案 |
|------|------|---------|---------|
| Pod 一直 Pending | 无可扩容的节点组或节点组达到最大值 | `kubectl logs -n kube-system -l app=cluster-autoscaler` | 增加 ASG/VMSS 最大值，检查 Pod 资源请求 |
| 节点无法缩容 | PDB 阻止或 Pod 有 `safe-to-evict: false` 注解 | `kubectl describe configmap cluster-autoscaler-status -n kube-system` | 调整 PDB 或移除注解 |
| 新节点无法加入集群 | Bootstrap Token 过期 | `kubeadm token list` | 刷新 Bootstrap Token |
| 扩容后立即缩容 | utilization-threshold 过高 | 查看 CA 日志 | 降低 `--scale-down-utilization-threshold` |
| `insufficient quota` | 云厂商配额不足 | 查看云厂商控制台 | 申请提高配额 |
| ASG/VMSS 不匹配 | CA 配置的 ASG 名称错误 | `kubectl get configmap -n kube-system` | 检查 `--nodes` 参数中的 ASG 名称 |
| 扩容节点类型不对 | Pod 需要特定资源（GPU/高内存） | `kubectl describe pod <pending-pod>` | 创建专用节点组，配合 nodeSelector |

---

## 相关函数

| 函数/组件 | 源码位置 | 说明 |
|----------|---------|------|
| `Autoscaler.RunOnce` | `cluster-autoscaler/core/scale_up.go` | CA 主循环 |
| `ScaleUp` | `cluster-autoscaler/core/scale_up.go` | 扩容逻辑 |
| `ScaleDown` | `cluster-autoscaler/core/scale_down.go` | 缩容逻辑 |
| `nodegroup.TemplateNodeInfo` | `cluster-autoscaler/cloudprovider/` | 节点组模板 |
| `FilterOutNodes` | `cluster-autoscaler/utils/` | 节点过滤 |
| `nodeGarbageCollector` | `pkg/controller/nodelifecycle/` | 节点 GC |

## Related

- [[reference|#reference Hub]] — tag hub

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[concepts/node-lifecycle-management.md|node-lifecycle-management]]
- [[entities/kubernetes.md|kubernetes]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/nodes.md|nodes]]
