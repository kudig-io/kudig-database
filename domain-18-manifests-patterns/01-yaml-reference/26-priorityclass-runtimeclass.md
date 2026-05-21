---
title: 26 - PriorityClass / RuntimeClass YAML 配置参考
description: '# 26 - PriorityClass / RuntimeClass YAML 配置参考'
category: yaml-manifests
tags:
- k8s
- yaml
- manifest
- template
- kubelet
- scheduler
- containerd
- cri-o
- docker
- pdb
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- PriorityClass / RuntimeClass YAML 配置参考 是什么
- 如何 PriorityClass / RuntimeClass YAML 配置参考
- Kubernetes 32 yaml manifests 最佳实践
trigger_keywords:
- PriorityClass
- RuntimeClass
- YAML
- 配置参考
- yaml
- manifests
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
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

# 26 - PriorityClass / RuntimeClass YAML 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02  
> **主题**: PriorityClass 优先级、RuntimeClass 运行时、ResourceClaim 动态资源分配

<!-- chunk: 目录 -->## 目录

- [概述](#概述)
- [PriorityClass 完整配置](#priorityclass-完整配置)
- [RuntimeClass 完整配置](#runtimeclass-完整配置)
- [ResourceClaim / ResourceClaimTemplate](#resourceclaim--resourceclaimtemplate)
- [内部原理](#内部原理)
- [生产案例](#生产案例)
- [最佳实践](#最佳实践)
- [常见问题](#常见问题)

---

<!-- chunk: 概述 -->## 概述

#<!-- chunk: PriorityClass -->## PriorityClass
用于定义 Pod 的优先级，影响调度顺序和抢占行为。

**核心能力**:
- **优先级值**: -2³¹ 到 10⁹
- **全局默认**: `globalDefault`
- **抢占策略**: `PreemptLowerPriority` / `Never`
- **系统保留**: 值 ≥ 10⁹ 仅限系统使用

#<!-- chunk: RuntimeClass -->## RuntimeClass
定义容器运行时处理器（如 Kata、gVisor），支持沙箱隔离。

**核心能力**:
- **运行时处理器**: `handler` 字段
- **资源开销**: `overhead.podFixed`
- **调度约束**: `nodeSelector` / `tolerations`

#<!-- chunk: ResourceClaim（Dynamic Resource Allocation） -->## ResourceClaim（Dynamic Resource Allocation）
动态资源分配（DRA）用于 GPU、FPGA 等设备的动态管理。

**版本兼容性**:
- v1.26+: Alpha（需启用 `DynamicResourceAllocation` feature gate）
- v1.30+: Beta（默认启用）

---

<!-- chunk: PriorityClass 完整配置 -->## PriorityClass 完整配置

#<!-- chunk: 基础示例 -->## 基础示例

```yaml
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority
# 优先级值: -2147483648 到 1000000000
# 值越高优先级越高,系统关键组件通常使用 2000000000
value: 1000000
# 全局默认优先级类(集群中只能有一个 globalDefault: true)
globalDefault: false
# 抢占策略: PreemptLowerPriority(默认) 或 Never
preemptionPolicy: PreemptLowerPriority
# 人类可读的描述
description: "高优先级业务应用,可以抢占低优先级 Pod"
```

#<!-- chunk: 系统级优先级（保留范围） -->## 系统级优先级（保留范围）

```yaml
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: system-cluster-critical
# 系统级优先级(>= 1000000000,仅限系统组件)
value: 2000000000
globalDefault: false
preemptionPolicy: PreemptLowerPriority
description: "集群关键组件(如 kube-dns, metrics-server)"
```

**Kubernetes 内置 PriorityClass**:
- `system-cluster-critical`: 2000000000（集群关键组件）
- `system-node-critical`: 2000001000（节点关键组件，如 kubelet）

#<!-- chunk: 禁止抢占策略 -->## 禁止抢占策略

```yaml
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: high-priority-no-preemption
value: 500000
# Never: 即使优先级高也不会触发抢占
# 适用于: 重要但可以等待的任务
preemptionPolicy: Never
description: "高优先级但不会驱逐其他 Pod"
```

#<!-- chunk: 多层级优先级体系 -->## 多层级优先级体系

```yaml
---
# 生产业务 - 最高优先级
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: production-critical
value: 900000
preemptionPolicy: PreemptLowerPriority
description: "生产环境核心业务"
---
# 生产业务 - 普通优先级
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: production-normal
value: 500000
preemptionPolicy: PreemptLowerPriority
description: "生产环境普通业务"
---
# 测试/开发环境
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: dev-environment
value: 100000
preemptionPolicy: Never
description: "开发/测试环境,不触发抢占"
---
# 离线任务 - 最低优先级
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: batch-jobs
value: 0
# 全局默认: 未指定 priorityClassName 的 Pod 使用此优先级
globalDefault: true
preemptionPolicy: Never
description: "批处理任务,最低优先级"
```

#<!-- chunk: Pod 中使用 PriorityClass -->## Pod 中使用 PriorityClass

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: high-priority-app
spec:
  # 指定优先级类
  priorityClassName: high-priority
  containers:
  - name: app
    image: nginx:1.25
    resources:
      requests:
        cpu: "1"
        memory: "1Gi"
```

---

<!-- chunk: RuntimeClass 完整配置 -->## RuntimeClass 完整配置

#<!-- chunk: 基础示例（默认运行时） -->## 基础示例（默认运行时）

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: runc
# handler: 节点上 CRI 运行时的名称
# 必须与 containerd/cri-o 配置中的 runtime handler 匹配
handler: runc
```

#<!-- chunk: Kata Containers 沙箱运行时 -->## Kata Containers 沙箱运行时

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata-containers
# Kata 运行时处理器(需在节点 containerd 中配置)
handler: kata
# 资源开销: RuntimeClass 运行时额外消耗的资源
# 调度器会将此开销加到 Pod 的资源请求上
overhead:
  podFixed:
    cpu: "200m"      # Kata VM 启动开销
    memory: "256Mi"  # Kata VM 内存开销
# 调度约束: 仅调度到支持 Kata 的节点
scheduling:
  nodeSelector:
    runtime: kata
  # 容忍节点污点
  tolerations:
  - key: "kata-only"
    operator: "Equal"
    value: "true"
    effect: "NoSchedule"
```

#<!-- chunk: gVisor 沙箱运行时 -->## gVisor 沙箱运行时

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
overhead:
  podFixed:
    cpu: "100m"      # gVisor 用户态内核开销较小
    memory: "128Mi"
scheduling:
  nodeSelector:
    runtime: gvisor
  tolerations:
  - key: "sandbox"
    operator: "Exists"
    effect: "NoSchedule"
```

#<!-- chunk: NVIDIA GPU 运行时 -->## NVIDIA GPU 运行时

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: nvidia
# nvidia-container-runtime 处理器
handler: nvidia
scheduling:
  nodeSelector:
    accelerator: nvidia-gpu
  tolerations:
  - key: "nvidia.com/gpu"
    operator: "Exists"
    effect: "NoSchedule"
```

#<!-- chunk: Pod 中使用 RuntimeClass -->## Pod 中使用 RuntimeClass

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: secure-workload
spec:
  # 指定运行时类
  runtimeClassName: kata-containers
  containers:
  - name: app
    image: myapp:1.0
    resources:
      requests:
        # 实际调度时会加上 overhead 的 200m CPU + 256Mi Memory
        cpu: "500m"
        memory: "512Mi"
      limits:
        cpu: "1"
        memory: "1Gi"
```

---

<!-- chunk: ResourceClaim / ResourceClaimTemplate -->## ResourceClaim / ResourceClaimTemplate

> **Feature Gate**: `DynamicResourceAllocation`  
> **状态**: v1.26 Alpha → v1.30 Beta → v1.32 稳定中

#<!-- chunk: ResourceClaim 基础示例 -->## ResourceClaim 基础示例

```yaml
apiVersion: resource.k8s.io/v1alpha2
kind: ResourceClaim
metadata:
  name: gpu-claim
  namespace: default
spec:
  # 资源类 (由设备插件提供)
  resourceClassName: nvidia-gpu.resource.k8s.io
  # 参数引用 (可选)
  parametersRef:
    apiGroup: gpu.resource.k8s.io
    kind: GpuClaimParameters
    name: high-performance
  # 分配模式
  allocationMode: WaitForFirstConsumer  # 延迟绑定
```

#<!-- chunk: ResourceClaimTemplate（动态创建） -->## ResourceClaimTemplate（动态创建）

```yaml
apiVersion: resource.k8s.io/v1alpha2
kind: ResourceClaimTemplate
metadata:
  name: gpu-claim-template
  namespace: default
spec:
  # 模板规范 (与 ResourceClaim.spec 相同)
  spec:
    resourceClassName: nvidia-gpu.resource.k8s.io
    parametersRef:
      apiGroup: gpu.resource.k8s.io
      kind: GpuClaimParameters
      name: ml-training
```

#<!-- chunk: Pod 中使用 ResourceClaim -->## Pod 中使用 ResourceClaim

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: gpu-workload
spec:
  # 方式1: 引用已存在的 ResourceClaim
  resourceClaims:
  - name: gpu
    source:
      resourceClaimName: gpu-claim
  containers:
  - name: training
    image: tensorflow/tensorflow:latest-gpu
    # 容器内使用资源声明
    resources:
      claims:
      - name: gpu
        request: "1"  # 请求 1 个 GPU
```

#<!-- chunk: 使用 ResourceClaimTemplate -->## 使用 ResourceClaimTemplate

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: gpu-dynamic
spec:
  # 方式2: 使用模板动态创建
  resourceClaims:
  - name: gpu
    source:
      resourceClaimTemplateName: gpu-claim-template
  containers:
  - name: inference
    image: pytorch/pytorch:2.0-cuda11.8
    resources:
      claims:
      - name: gpu
```

#<!-- chunk: ResourceClass 配置（驱动侧） -->## ResourceClass 配置（驱动侧）

```yaml
apiVersion: resource.k8s.io/v1alpha2
kind: ResourceClass
metadata:
  name: nvidia-gpu.resource.k8s.io
# DRA 驱动名称
driverName: gpu.nvidia.com
# 参数引用 (可选, 全局配置)
parametersRef:
  apiGroup: gpu.resource.k8s.io
  kind: DeviceClassParameters
  name: default-gpu-config
# 适用节点选择器
suitableNodes:
  nodeSelectorTerms:
  - matchExpressions:
    - key: gpu.nvidia.com/present
      operator: In
      values: ["true"]
```

---

<!-- chunk: 内部原理 -->## 内部原理

#<!-- chunk: PriorityClass 优先级抢占算法 -->## PriorityClass 优先级抢占算法

##<!-- chunk: 调度流程 -->## 调度流程

```
1. Pod 进入调度队列
   ├─ 按 Priority 值排序 (高优先级优先)
   └─ 相同优先级按创建时间排序 (FIFO)

2. 调度器尝试调度
   ├─ 预选 (Predicates): 找到可用节点
   └─ 优选 (Priorities): 选择最佳节点

3. 无可用节点时触发抢占 (preemptionPolicy: PreemptLowerPriority)
   ├─ 找到候选节点
   ├─ 模拟驱逐低优先级 Pod
   ├─ 验证是否满足调度条件
   └─ 执行驱逐并等待资源释放
```

##<!-- chunk: 抢占算法 -->## 抢占算法

**关键代码逻辑** (`pkg/scheduler/framework/plugins/defaultpreemption`):
```go
// 伪代码
func SelectNodesForPreemption(pod *Pod) {
  if pod.Spec.PreemptionPolicy == Never {
    return nil  // 不触发抢占
  }
  
  for node in allNodes {
    // 找到可驱逐的低优先级 Pod
    victimsOnNode = findVictims(node, pod.Priority)
    if canScheduleAfterEviction(pod, node, victimsOnNode) {
      candidates = append(candidates, node)
    }
  }
  
  // 选择影响最小的节点 (驱逐 Pod 数量最少、优先级总和最低)
  return selectBestCandidate(candidates)
}
```

**抢占保护**:
- PDB (PodDisruptionBudget) 会阻止抢占
- `system-cluster-critical` 和 `system-node-critical` 不会被抢占
- 同优先级 Pod 之间不会互相抢占

#<!-- chunk: RuntimeClass Admission Controller -->## RuntimeClass Admission Controller

##<!-- chunk: 工作流程 -->## 工作流程

```
1. API Server 接收 Pod 创建请求
   └─ RuntimeClass Admission Controller 拦截

2. 读取 RuntimeClass 对象
   ├─ 验证 handler 有效性
   └─ 注入 overhead 和 scheduling 字段

3. Mutating Webhook 修改 Pod Spec
   ├─ pod.spec.overhead = runtimeClass.overhead.podFixed
   ├─ pod.spec.nodeSelector += runtimeClass.scheduling.nodeSelector
   └─ pod.spec.tolerations += runtimeClass.scheduling.tolerations

4. 调度器调度时
   ├─ 资源请求 = container.requests + pod.spec.overhead
   └─ 节点过滤考虑 nodeSelector 和 tolerations

5. Kubelet 创建容器
   └─ CRI 调用: RunPodSandbox(handler=runtimeClass.handler)
```

##<!-- chunk: Containerd 配置示例 -->## Containerd 配置示例

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes]
  # 默认 runc
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
    runtime_type = "io.containerd.runc.v2"
  
  # Kata Containers
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
    runtime_type = "io.containerd.kata.v2"
  
  # gVisor runsc
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc]
    runtime_type = "io.containerd.runsc.v1"
```

#<!-- chunk: Dynamic Resource Allocation (DRA) 设备分配 -->## Dynamic Resource Allocation (DRA) 设备分配

##<!-- chunk: 架构组件 -->## 架构组件

```
┌────────────────────────────────────────────────────────┐
│                   Kubernetes API Server                │
│  ResourceClaim, ResourceClass, ResourceClaimTemplate   │
└────────────────┬───────────────────────────┬───────────┘
                 │                           │
        ┌────────▼────────┐         ┌────────▼──────────┐
        │   Scheduler     │         │ DRA Controller    │
        │  (资源感知调度)   │         │ (资源分配协调器)    │
        └────────┬────────┘         └────────┬──────────┘
                 │                           │
        ┌────────▼────────────────────────────▼──────────┐
        │             DRA Driver (设备插件)              │
        │   - 设备发现和注册                              │
        │   - 分配算法实现                                │
        │   - 设备准备 (Prepare/Unprepare)               │
        └───────────────────┬─────────────────────────────┘
                            │
                   ┌────────▼────────┐
                   │     Kubelet     │
                   │   - 挂载设备     │
                   │   - 容器启动     │
                   └─────────────────┘
```

##<!-- chunk: 分配流程 -->## 分配流程

```
1. 用户创建 ResourceClaim
   └─ 状态: Pending

2. DRA Controller 监听 ResourceClaim
   ├─ 调用 DRA Driver Allocate API
   └─ Driver 返回分配结果 (设备 ID, 拓扑信息)

3. ResourceClaim 状态更新
   ├─ status.allocation = {...}
   └─ 状态: Allocated

4. Pod 引用 ResourceClaim
   └─ 调度器验证节点亲和性

5. Pod 绑定到节点后
   ├─ Kubelet 调用 DRA Driver NodePrepareResource
   ├─ Driver 挂载设备 (如 /dev/nvidia0)
   └─ Kubelet 启动容器并注入设备路径

6. Pod 删除时
   └─ Kubelet 调用 NodeUnprepareResource 清理
```

---

<!-- chunk: 生产案例 -->## 生产案例

#<!-- chunk: 案例1: 多租户集群优先级体系 -->## 案例1: 多租户集群优先级体系

**场景**: 企业多租户集群，需确保生产业务优先级。

```yaml
# 1. 系统组件 (最高优先级)
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: system-critical
value: 2000000000
description: "系统关键组件: kube-dns, metrics-server, ingress-controller"
---
# 2. 生产业务 - 在线服务
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: prod-online
value: 900000
preemptionPolicy: PreemptLowerPriority
description: "生产在线服务,可抢占离线任务"
---
# 3. 生产业务 - 离线任务
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: prod-batch
value: 500000
preemptionPolicy: Never
description: "生产离线任务,不触发抢占"
---
# 4. 测试环境
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: staging
value: 100000
preemptionPolicy: Never
description: "预发布环境"
---
# 5. 开发环境 (默认)
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: dev-default
value: 0
globalDefault: true  # 未指定的 Pod 使用此优先级
preemptionPolicy: Never
description: "开发环境默认优先级"
```

**部署示例**:

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: online-api
  namespace: production
spec:
  replicas: 3
  template:
    spec:
      # 生产在线服务使用高优先级
      priorityClassName: prod-online
      containers:
      - name: api
        image: api:v1.0
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
---
apiVersion: batch/v1
kind: Job
metadata:
  name: data-processing
  namespace: production
spec:
  template:
    spec:
      # 离线任务使用较低优先级
      priorityClassName: prod-batch
      containers:
      - name: processor
        image: processor:v1.0
```

**效果**:
- 资源紧张时，`online-api` 会抢占 `data-processing`
- 开发环境 Pod 最先被驱逐
- 系统组件始终受保护

#<!-- chunk: 案例2: 金融业务安全隔离 (Kata Containers) -->## 案例2: 金融业务安全隔离 (Kata Containers)

**场景**: 金融服务需强隔离沙箱运行时。

```yaml
# 1. 配置 RuntimeClass
---
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata-secure
handler: kata
overhead:
  podFixed:
    cpu: "250m"
    memory: "512Mi"  # Kata VM 额外开销
scheduling:
  nodeSelector:
    node-role.kubernetes.io/kata: ""
  tolerations:
  - key: "kata-only"
    operator: "Exists"
    effect: "NoSchedule"
---
# 2. PriorityClass 确保关键业务优先
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: finance-critical
value: 950000
description: "金融核心业务"
```

**应用部署**:

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: payment-service
  namespace: finance
spec:
  # 使用 Kata 沙箱运行时
  runtimeClassName: kata-secure
  # 高优先级
  priorityClassName: finance-critical
  containers:
  - name: payment
    image: payment-service:secure
    resources:
      requests:
        # 实际请求 = 500m + 250m(overhead) = 750m CPU
        cpu: "500m"
        memory: "1Gi"   # 实际 = 1Gi + 512Mi = 1.5Gi
      limits:
        cpu: "2"
        memory: "4Gi"
    securityContext:
      runAsNonRoot: true
      readOnlyRootFilesystem: true
```

**节点标签配置**:

```bash
# 为支持 Kata 的节点打标签
kubectl label nodes node-01 node-role.kubernetes.io/kata=""
kubectl taint nodes node-01 kata-only=true:NoSchedule
```

#<!-- chunk: 案例3: GPU 动态分配 (DRA) -->## 案例3: GPU 动态分配 (DRA)

**场景**: AI 训练平台，动态分配 NVIDIA GPU。

```yaml
# 1. ResourceClass 配置
---
apiVersion: resource.k8s.io/v1alpha2
kind: ResourceClass
metadata:
  name: nvidia-a100
driverName: gpu.nvidia.com
parametersRef:
  apiGroup: gpu.nvidia.com
  kind: GpuConfig
  name: a100-80gb
suitableNodes:
  nodeSelectorTerms:
  - matchExpressions:
    - key: nvidia.com/gpu.product
      operator: In
      values: ["NVIDIA-A100-SXM4-80GB"]
---
# 2. ResourceClaimTemplate 用于训练任务
apiVersion: resource.k8s.io/v1alpha2
kind: ResourceClaimTemplate
metadata:
  name: ml-training-gpu
  namespace: ml-platform
spec:
  spec:
    resourceClassName: nvidia-a100
    parametersRef:
      apiGroup: gpu.nvidia.com
      kind: GpuClaimParameters
      name: training-config
---
# 3. GPU 配置参数
apiVersion: gpu.nvidia.com/v1alpha1
kind: GpuClaimParameters
metadata:
  name: training-config
spec:
  # 共享模式 (MIG / Time-Slicing)
  sharing: false
  # GPU 内存需求
  memory: "40Gi"
  # 多 GPU 拓扑
  count: 4
  topology: nvlink  # 要求 NVLink 互联
```

**训练任务使用 GPU**:

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: bert-training
  namespace: ml-platform
spec:
  template:
    spec:
      # 高优先级确保训练任务不被抢占
      priorityClassName: ml-training
      # 引用 GPU 资源声明模板
      resourceClaims:
      - name: gpu
        source:
          resourceClaimTemplateName: ml-training-gpu
      containers:
      - name: trainer
        image: nvcr.io/nvidia/pytorch:23.12-py3
        command: ["python", "train.py"]
        resources:
          # 声明使用 4 个 GPU
          claims:
          - name: gpu
            request: "4"
        env:
        - name: NVIDIA_VISIBLE_DEVICES
          value: "all"  # DRA 驱动会自动注入正确的 GPU ID
```

**推理服务 (共享 GPU)**:

```yaml
---
apiVersion: gpu.nvidia.com/v1alpha1
kind: GpuClaimParameters
metadata:
  name: inference-config
spec:
  # 启用 Time-Slicing 共享
  sharing: true
  # 每个容器最大 GPU 利用率
  maxUtilization: 25
---
apiVersion: v1
kind: Pod
metadata:
  name: inference-service
spec:
  resourceClaims:
  - name: gpu
    source:
      resourceClaimName: inference-gpu  # 预创建的 ResourceClaim
  containers:
  - name: inference
    image: tritonserver:23.12
    resources:
      claims:
      - name: gpu
        request: "1"  # 共享 1/4 GPU 资源
```

---

<!-- chunk: 最佳实践 -->## 最佳实践

#<!-- chunk: PriorityClass 设计原则 -->## PriorityClass 设计原则

1. **明确分层体系**
   ```
   系统组件 (2000000000+)
   ├─ system-node-critical (2000001000): kubelet, kube-proxy
   └─ system-cluster-critical (2000000000): DNS, metrics-server
   
   生产业务 (500000-1000000)
   ├─ 在线服务 (800000-1000000): API, 前端应用
   └─ 离线任务 (500000-700000): 数据处理, 定时任务
   
   非生产 (0-100000)
   ├─ 预发布 (50000-100000)
   └─ 开发测试 (0, globalDefault: true)
   ```

2. **抢占策略选择**
   - **PreemptLowerPriority**: 在线服务、实时任务
   - **Never**: 离线任务、开发环境（避免雪崩）

3. **配额限制**
   ```yaml
   # 防止滥用高优先级
   apiVersion: v1
   kind: ResourceQuota
   metadata:
     name: high-priority-quota
     namespace: production
   spec:
     hard:
       pods: "100"
     scopeSelector:
       matchExpressions:
       - operator: In
         scopeName: PriorityClass
         values: ["prod-online"]
   ```

#<!-- chunk: RuntimeClass 使用建议 -->## RuntimeClass 使用建议

1. **合理评估 Overhead**
   - Kata: 200-500m CPU, 256-512Mi Memory
   - gVisor: 50-100m CPU, 64-128Mi Memory
   - 实际测试后调整

2. **节点隔离策略**
   ```bash
   # 专用节点组
   kubectl label nodes kata-node-{1..3} runtime=kata
   kubectl taint nodes kata-node-{1..3} kata-only=true:NoSchedule
   ```

3. **性能监控**
   ```yaml
   # 对比监控
   metrics:
   - pod_overhead_cpu{runtime_class="kata"}
   - pod_overhead_memory{runtime_class="kata"}
   - pod_startup_duration{runtime_class="kata"} vs {runtime_class="runc"}
   ```

#<!-- chunk: ResourceClaim DRA 注意事项 -->## ResourceClaim DRA 注意事项

1. **版本兼容性检查**
   ```bash
   # 检查 DRA 是否启用
   kubectl api-resources | grep resource.k8s.io
   
   # 查看 Feature Gate
   kubectl get --raw /metrics | grep dynamic_resource_allocation
   ```

2. **驱动健康监控**
   ```yaml
   # DRA 驱动 DaemonSet 健康检查
   apiVersion: apps/v1
   kind: DaemonSet
   metadata:
     name: gpu-dra-driver
   spec:
     template:
       spec:
         containers:
         - name: driver
           livenessProbe:
             httpGet:
               path: /healthz
               port: 8080
           readinessProbe:
             grpc:
               port: 9090  # DRA gRPC 端口
   ```

3. **资源泄漏防护**
   ```yaml
   # ResourceClaim 自动清理
   apiVersion: v1
   kind: Pod
   metadata:
     ownerReferences:
     - apiVersion: batch/v1
       kind: Job
       name: training-job
       # Job 删除时自动清理 ResourceClaim
       blockOwnerDeletion: true
   ```

---

<!-- chunk: 常见问题 -->## 常见问题

#<!-- chunk: PriorityClass FAQ -->## PriorityClass FAQ

**Q: Pod 无法调度，提示优先级不足？**

```bash
# 检查事件
kubectl describe pod <pod-name>
# Events:
#   Warning  FailedScheduling  pod has insufficient priority to preempt

# 解决方案: 提升优先级或等待资源释放
kubectl get priorityclasses
kubectl edit pod <pod-name>  # 修改 priorityClassName
```

**Q: 高优先级 Pod 频繁触发抢占，导致集群不稳定？**

**解决方案**:
1. 添加 PDB 保护低优先级关键服务
   ```yaml
   apiVersion: policy/v1
   kind: PodDisruptionBudget
   metadata:
     name: batch-job-pdb
   spec:
     minAvailable: 50%
     selector:
       matchLabels:
         app: batch-job
   ```

2. 使用 `preemptionPolicy: Never`
3. 增加集群节点容量

**Q: 如何禁止普通用户使用高优先级？**

**RBAC 限制**:
```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: restrict-high-priority
  namespace: dev-team
rules:
- apiGroups: [""]
  resources: ["pods"]
  verbs: ["create", "update"]
  # 限制只能使用特定 PriorityClass
  resourceNames: ["dev-default", "staging"]
```

#<!-- chunk: RuntimeClass FAQ -->## RuntimeClass FAQ

**Q: Pod 报错 `RuntimeClass not found`？**

```bash
# 检查 RuntimeClass 是否存在
kubectl get runtimeclass
kubectl describe runtimeclass kata-containers

# 检查节点是否支持
kubectl describe node <node-name> | grep -A 10 "Container Runtime"
```

**Q: RuntimeClass 无法正确注入 overhead？**

```bash
# 验证 Admission Controller 是否启用
kubectl get pod -o jsonpath='{.spec.overhead}' <pod-name>

# 输出应为: {"cpu":"200m","memory":"256Mi"}

# 如果为空, 检查 API Server 配置
--enable-admission-plugins=...,RuntimeClass,...
```

**Q: Containerd 报错 `unknown handler "kata"`？**

**排查步骤**:
```bash
# 1. 检查 containerd 配置
cat /etc/containerd/config.toml | grep -A 5 "kata"

# 2. 验证 Kata 安装
kata-runtime --version

# 3. 重启 containerd
systemctl restart containerd

# 4. 测试运行时
ctr run --runtime io.containerd.kata.v2 docker.io/library/busybox:latest test-kata
```

#<!-- chunk: ResourceClaim FAQ -->## ResourceClaim FAQ

**Q: ResourceClaim 一直 Pending？**

```bash
# 检查状态
kubectl describe resourceclaim <claim-name>

# 常见原因:
# 1. DRA Driver 未安装
kubectl get daemonset -n kube-system | grep dra-driver

# 2. ResourceClass 不存在
kubectl get resourceclass

# 3. 节点不满足 suitableNodes 条件
kubectl get nodes -L gpu.nvidia.com/present
```

**Q: Pod 启动失败 `failed to prepare resources`？**

**调试步骤**:
```bash
# 1. 查看 Kubelet 日志
journalctl -u kubelet -f | grep DRA

# 2. 检查 DRA Driver 日志
kubectl logs -n kube-system <dra-driver-pod> --tail=100

# 3. 验证设备可用性
kubectl get resourceclaim <claim-name> -o yaml | grep allocation -A 20
```

**Q: 如何释放已分配但未使用的 ResourceClaim？**

```bash
# 手动删除 (如果 Pod 已删除)
kubectl delete resourceclaim <claim-name>

# 检查泄漏的 Claim
kubectl get resourceclaim --all-namespaces -o json | \
  jq '.items[] | select(.status.reservedFor == null) | .metadata.name'
```

---

<!-- chunk: 版本兼容性对照 -->## 版本兼容性对照

| 功能                | v1.25 | v1.26 | v1.27 | v1.28 | v1.30 | v1.32 |
|---------------------|-------|-------|-------|-------|-------|-------|
| PriorityClass       | ✅ GA | ✅    | ✅    | ✅    | ✅    | ✅    |
| preemptionPolicy    | ✅ GA | ✅    | ✅    | ✅    | ✅    | ✅    |
| RuntimeClass        | ✅ GA | ✅    | ✅    | ✅    | ✅    | ✅    |
| overhead.podFixed   | ✅ GA | ✅    | ✅    | ✅    | ✅    | ✅    |
| ResourceClaim (DRA) | ❌    | 🧪 Alpha | 🧪 Alpha | 🧪 Alpha | 🟡 Beta | ✅ 接近 GA |

**图例**: ✅ GA (稳定) | 🟡 Beta (默认启用) | 🧪 Alpha (需启用 Feature Gate) | ❌ 不支持

---

<!-- chunk: 参考资料 -->## 参考资料

- [PriorityClass 官方文档](https://kubernetes.io/docs/concepts/scheduling-eviction/pod-priority-preemption/)
- [RuntimeClass 官方文档](https://kubernetes.io/docs/concepts/containers/runtime-class/)
- [Dynamic Resource Allocation KEP](https://github.com/kubernetes/enhancements/tree/master/keps/sig-node/3063-dynamic-resource-allocation)
- [Kata Containers 官网](https://katacontainers.io/)
- [gVisor 官网](https://gvisor.dev/)

---

**文档维护**: 建议每季度更新一次，关注 DRA 正式 GA 的版本变化。

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-18-manifests-patterns/MOC.md|domain-32-yaml-manifests MOC]]
- [[domain-18-manifests-patterns/README.md|Domain-32: Kubernetes YAML 配置完整参考手册]]
- [[domain-18-manifests-patterns/00-open-source-projects-index.md|Domain-32 YAML 清单 — 开源项目索引]]
- [[domain-18-manifests-patterns/01-yaml-syntax-resource-conventions.md|01 - YAML 语法基础与 Kubernetes 资源通用规范]]
- [[domain-18-manifests-patterns/02-namespace-resourcequota-limitrange.md|02 - Namespace / ResourceQuota / LimitRange YAML 配置参考]]
- [[domain-18-manifests-patterns/03-pod-specification-complete.md|03 - Pod 完整规格说明书]]
- [[domain-18-manifests-patterns/04-deployment-replicaset.md|04 - Deployment / ReplicaSet YAML 配置参考]]
- [[domain-18-manifests-patterns/05-statefulset-reference.md|05 - StatefulSet YAML 配置参考]]
- [[domain-18-manifests-patterns/06-daemonset-reference.md|06 - DaemonSet YAML 配置参考]]
- [[domain-18-manifests-patterns/07-job-cronjob-reference.md|07 - Job / CronJob YAML 配置参考]]
- [[domain-18-manifests-patterns/08-service-all-types.md|08 - Service 全类型 YAML 配置参考]]
- [[domain-18-manifests-patterns/09-endpoints-endpointslice.md|09 - Endpoints / EndpointSlice YAML 配置参考]]

## See Also

- [[domain-18-manifests-patterns/24-admission-webhook-configuration.md|24-admission-webhook-configuration]]
- [[domain-18-manifests-patterns/25-validatingadmissionpolicy.md|25-validatingadmissionpolicy]]
- [[domain-18-manifests-patterns/27-hpa-autoscaling-v2.md|27-hpa-autoscaling-v2]]
- [[domain-18-manifests-patterns/28-poddisruptionbudget-reference.md|28-poddisruptionbudget-reference]]

## Related

- [[domain-19-landscape-references/topic-index/scheduler-index|Scheduler 调度与弹性伸缩知识图谱索引]]
