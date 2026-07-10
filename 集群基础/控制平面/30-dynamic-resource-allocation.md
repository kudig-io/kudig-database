---
title: 30 - 动态资源分配 (Dynamic Resource Allocation)
description: '**适用版本**: Kubernetes v1.30+ (Alpha/Beta 演进中) | **最后更新**: 2026-04 | **文档类型**:
  特性设计文档'
summary: '**适用版本**: Kubernetes v1.30+ (Alpha/Beta 演进中) | **最后更新**: 2026-04 | **文档类型**:
  特性设计文档'
category: control-plane
tags:
- k8s
- control-plane
- etcd
- apiserver
- scheduler
- controller-manager
- kubelet
- prometheus
- containerd
- cri-o
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 10min
intent_queries:
- 动态资源分配 (Dynamic Resource Allocation) 是什么
- 如何 动态资源分配 (Dynamic Resource Allocation)
- Kubernetes 3 control plane 最佳实践
trigger_keywords:
- 动态资源分配
- Dynamic
- Resource
- Allocation
- control
- plane
prerequisites:
- kubectl-basics
- kubernetes-concepts
- prometheus-basics
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
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../工作负载/
  label: '相关知识域: 工作负载'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../存储/
  label: '相关知识域: 存储'
- type: domain
  path: ../安全/
  label: '相关知识域: 安全'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 30 - 动态资源分配 ([[系统基础/知识字典/scheduling/dynamic-resource-allocation.md|Dynamic Resource Allocation]])

> **适用版本**: [[Kubernetes|Kubernetes]] v1.30+ (Alpha/Beta 演进中) | **最后更新**: 2026-04 | **文档类型**: 特性设计文档

---

<!-- chunk: 目录 -->
## 目录

1. [核心概念与演进背景](#1-核心概念与演进背景)
2. [DRA 与 Device Plugin 对比](#2-dra-与-device-plugin-对比)
3. [核心 API 对象](#3-核心-api-对象)
4. [Feature Gate 与启用方式](#4-feature-gate-与启用方式)
5. [完整 YAML 配置示例](#5-完整-yaml-配置示例)
6. [内部架构与工作流程](#6-内部架构与工作流程)
7. [使用场景与最佳实践](#7-使用场景与最佳实践)
8. [当前限制与未来演进](#8-当前限制与未来演进)
9. [Device Plugin 迁移路径](#9-device-plugin-迁移路径)

---

<!-- chunk: 1. 核心概念与演进背景 -->
## 1. 核心概念与演进背景

### 1.1 为什么需要 DRA

传统的 Device Plugin 机制自 Kubernetes v1.10 引入以来，在 GPU、FPGA 等异构硬件调度方面发挥了重要作用。然而随着 AI/ML 工作负载的爆发式增长，Device Plugin 的局限性日益明显：

| 局限性 | 说明 | 影响 |
|--------|------|------|
| 资源粒度粗 | 只能以整卡 (device) 为单位分配 | GPU 共享 (MIG/MPS) 难以表达 |
| 调度耦合弱 | 资源分配在 [[kubelet|kubelet]] 阶段完成 | 调度器无法感知设备拓扑和约束 |
| 配置僵化 | 设备参数通过注解传递 | 缺乏结构化、类型化的配置能力 |
| 组合能力缺失 | 无法表达多设备组合需求 | GPU+NIC 亲和性分配困难 |

动态资源分配 (Dynamic Resource Allocation, DRA) 是 Kubernetes 下一代硬件资源调度机制，旨在替代传统 Device Plugin，提供更灵活、更精确的资源建模和分配能力。

### 1.2 DRA 核心设计目标

```
+-------------------------------------------------------------------------------+
|                    DRA Design Goals (Kubernetes v1.30+)                        |
+-------------------------------------------------------------------------------+
|                                                                                |
|  +-----------------+  +-----------------+  +-----------------+  +-----------+  |
|  |  细粒度资源建模  |  |  调度深度集成   |  |  参数化配置     |  |  多设备组合 |  |
|  |  (Sub-device)   |  |  (Scheduler)    |  |  (Structured)   |  |  (Compound)|  |
|  +-----------------+  +-----------------+  +-----------------+  +-----------+  |
|           |                    |                    |                  |       |
|           v                    v                    v                  v       |
|  +----------------------------------------------------------------------------+|
|  |                          统一资源抽象层 (ResourceClass)                     ||
|  |                    驱动无关的声明式 API + 结构化参数                          ||
|  +----------------------------------------------------------------------------+|
|                                    |                                           |
|                                    v                                           |
|  +----------------------------------------------------------------------------+|
|  |                         DRA Driver (节点本地)                                ||
|  |              kubelet <-> DRA Driver <-> 设备运行时 (CDI/设备原生)            ||
|  +----------------------------------------------------------------------------+|
|                                                                                |
+-------------------------------------------------------------------------------+
```

---

<!-- chunk: 2. DRA 与 Device Plugin 对比 -->
## 2. DRA 与 Device Plugin 对比

### 2.1 能力对比矩阵

| 对比维度 | 传统 Device Plugin | 动态资源分配 (DRA) | 说明 |
|:---------|:-------------------|:-------------------|:-----|
| **资源粒度** | 整设备 (如 `nvidia.com/gpu: 1`) | 子设备/组合设备 (MIG slice, 显存块) | DRA 支持将物理设备切分为逻辑单元 |
| **调度集成** | kubelet 阶段通过 `Allocate` [[gRPC|gRPC]] 调用 | 通过调度框架插件 (DRA Plugin) | DRA 在调度阶段即确定资源分配 |
| **分配灵活性** | 静态，无参数 | 支持参数化配置 (`parametersRef`) | 用户可指定计算模式、显存大小等 |
| **拓扑感知** | 有限 (通过 `TopologyHint`) | 原生支持 NUMA/PCIe 拓扑约束 | DRA 在 Filter/Score 阶段考虑拓扑 |
| **多设备组合** | 不支持 | 支持跨 ResourceClass 组合 | 如 `GPU + NIC + RDMA` 联合分配 |
| **资源状态可见性** | 节点本地 (kubelet 维护) | 集群级 (ResourceClaim 对象) | 资源分配状态通过 API Server 持久化 |
| **分配语义** | 计数式 (count-based) | 选择式 (claim-based) | DRA 精确指定「哪块设备」而非「多少个」 |
| **配置方式** | 设备特定注解/环境变量 | 结构化参数 (ConfigMap/CRD) | 类型安全，可校验 |

### 2.2 架构差异对比

```
+-------------------------------------------------------------------------------+
|                         Device Plugin Architecture                             |
+-------------------------------------------------------------------------------+
|                                                                                |
|   User                    Scheduler                     Kubelet                |
|    |                        |                             |                    |
|    |  Pod (nvidia.com/gpu)  |                             |                    |
|    | ----------------------> |                             |                    |
|    |                        |  1. 仅按数量过滤节点          |                    |
|    |                        |  2. 选择节点                  |                    |
|    |                        | --------------------------> |                    |
|    |                        |                             |  3. gRPC Allocate()|
|    |                        |                             | ------> Device     |
|    |                        |                             |      Plugin        |
|    |                        |                             |  4. 挂载 /dev/xxx  |
|    |                        |                             | <------ 返回路径   |
|    |                        |                             |                    |
|   Problem: 调度器对设备无感知，分配在绑定后由 kubelet 完成                      |
|                                                                                |
+-------------------------------------------------------------------------------+

+-------------------------------------------------------------------------------+
|                         DRA Architecture                                       |
+-------------------------------------------------------------------------------+
|                                                                                |
|   User                    Scheduler (+ DRA Plugin)        Kubelet              |
|    |                        |                                 |                |
|    |  ResourceClaim         |                                 |                |
|    |  Pod (claimName)       |                                 |                |
|    | ----------------------> |                                 |                |
|    |                        |  1. DRA Filter: 检查 ResourceSlice                |
|    |                        |  2. DRA Score: 拓扑最优排序     |                |
|    |                        |  3. Reserve: 预占 ResourceClaim |                |
|    |                        |  4. Bind: 分配结果写入 API Server                 |
|    |                        |                                 |  5. kubelet 读取|
|    |                        |                                 |     ResourceClaim|
|    |                        |                                 |  6. DRA Driver  |
|    |                        |                                 |     准备设备    |
|    |                        |                                 |  7. CRI 创建容器|
|   Advantage: 调度阶段即完成精确资源分配，状态持久化到集群                      |
|                                                                                |
+-------------------------------------------------------------------------------+
```

---

<!-- chunk: 3. 核心 API 对象 -->
## 3. 核心 API 对象

### 3.1 对象关系图

```
+-------------------------------------------------------------------------------+
|                         DRA Core API Objects                                   |
+-------------------------------------------------------------------------------+
|                                                                                |
|  +----------------------+         +----------------------+                     |
|  |   ResourceClass      |<-------|  ResourceClaimTemplate|                     |
|  |  (资源类定义)         |         |  (模板化声明)         |                     |
|  |  - driverName        |         |  - spec 模板          |                     |
|  |  - parametersRef     |         |  - 为 Pod 自动生成    |                     |
|  +----------------------+         +----------+-----------+                     |
|           |                                  |                                 |
|           |  引用                            |  生成                           |
|           v                                  v                                 |
|  +------------------------------------------------------+                      |
|  |              ResourceClaim (资源声明)                 |                      |
|  |  - 用户创建 或 由 Template 自动生成                    |                      |
|  |  - 包含资源需求规格 (parametersRef)                   |                      |
|  |  - 记录分配结果 (allocation)                          |                      |
|  |  - 状态: Pending -> Allocated -> Reserved -> Prepared |                      |
|  +------------------------------------------------------+                      |
|                              |                                                 |
|                              |  消费                                           |
|                              v                                                 |
|  +------------------------------------------------------+                      |
|  |  Pod                                                 |                      |
|  |  spec.resourceClaims:                                |                      |
|  |    - name: gpu                                       |                      |
|  |      source.resourceClaimName: my-gpu-claim          |                      |
|  |  containers.resources.claims:                        |                      |
|  |    - name: gpu                                       |                      |
|  +------------------------------------------------------+                      |
|                                                                                |
|  +----------------------+         +----------------------+                     |
|  |   ResourceSlice      |         |   Device Taint/      |                     |
|  |  (节点可用资源)       |         |   Toleration         |                     |
|  |  - 由 DRA Driver 上报 |         |  (设备级容忍调度)     |                     |
|  |  - 描述设备拓扑/属性  |         |                      |                     |
|  +----------------------+         +----------------------+                     |
|                                                                                |
+-------------------------------------------------------------------------------+
```

### 3.2 各对象详解

| API 对象 | API 版本 | 作用域 | 说明 |
|:---------|:---------|:-------|:-----|
| **ResourceClass** | `resource.k8s.io/v1beta2` | 集群级 | 定义一类资源（如 `gpu.nvidia.com`），关联驱动名称和默认参数 |
| **ResourceClaim** | `resource.k8s.io/v1beta2` | 命名空间级 | 用户声明具体资源需求，由调度器完成分配 |
| **ResourceClaimTemplate** | `resource.k8s.io/v1beta2` | 命名空间级 | Pod 控制器（Deployment/StatefulSet）使用，自动为每个 Pod 创建 Claim |
| **ResourceSlice** | `resource.k8s.io/v1beta2` | 集群级 | DRA Driver 在节点上发现并上报可用设备信息 |
| **DeviceClass** (v1.32+) | `resource.k8s.io/v1beta2` | 集群级 | 标准化设备属性定义，便于跨驱动互操作 |
| **DeviceTaint** / **DeviceToleration** | `resource.k8s.io/v1beta2` | 集群级/Claim级 | 设备级污点与容忍，类似于 Node Taint |

### 3.3 ResourceClass 关键字段

```yaml
apiVersion: resource.k8s.io/v1beta2
kind: ResourceClass
metadata:
  name: nvidia-gpu-h100
spec:
  # 驱动名称，对应节点上的 DRA Driver
  driverName: gpu.nvidia.com
  
  # 引用默认参数配置 (可选)
  parametersRef:
    apiGroup: resource.k8s.io/v1beta2
    kind: ResourceClaimParameters
    name: nvidia-gpu-default-params
  
  # 结构化参数模式 (v1.30+ 支持)
  structuredParameters:
    # 启用内置验证模式
    cel:
      - name: "memorySize"
        expression: "device.attributes['nvidia.com/gpu'].memory >= int(resource.claims['memory'])"
  
  # 分配模式: Exact (精确匹配) / MostAllocated / LeastAllocated
  allocationMode: Exact
  
  # 是否允许多个 Pod 共享同一个 Claim
  shareable: false
  
  # 节点筛选器，限制该 ResourceClass 可用的节点范围
  nodeSelector:
    nodeSelectorTerms:
      - matchExpressions:
          - key: nvidia.com/gpu.present
            operator: In
            values: ["true"]
  
  # 设备级污点容忍
  tolerations:
    - key: "nvidia.com/gpu.maintenance"
      operator: "Exists"
      effect: "NoSchedule"
```

### 3.4 ResourceClaim 生命周期状态

| 状态 | 含义 | 转换条件 |
|:-----|:-----|:---------|
| **Pending** | 等待分配 | Claim 创建后初始状态 |
| **Allocated** | 已分配 | 调度器完成设备选择，写入 allocation 字段 |
| **Reserved** | 已预留 | Pod 被调度到目标节点，Claim 绑定节点 |
| **Prepared** | 已就绪 | kubelet + DRA Driver 完成设备准备（CDI 注入） |
| **Deallocated** | 已释放 | Pod 删除，资源回收完成 |

---

<!-- chunk: 4. Feature Gate 与启用方式 -->
## 4. Feature Gate 与启用方式

### 4.1 特性门控配置

DRA 功能通过以下 Feature Gate 控制：

| Feature Gate | 默认值 (v1.30) | 默认值 (v1.32) | 说明 |
|:-------------|:---------------|:---------------|:-----|
| `DynamicResourceAllocation` | `false` (Alpha) | `true` (Beta) | 主开关，启用 DRA API 和调度插件 |
| `DRAControlPlaneController` | `false` | `true` (Beta) | 启用控制平面自动控制器 |
| `DRAResourceClaimDeviceStatus` | `false` | `true` (Beta) | 允许 ResourceClaim 记录设备状态 |
| `DevicePluginCDIDevices` | `true` | `true` (GA) | Device Plugin 使用 CDI 规范传递设备 |

### 4.2 各组件启用配置

```yaml
# ============================================
# 1. kube-apiserver 启用 DRA
# ============================================
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
spec:
  containers:
    - name: kube-apiserver
      image: registry.k8s.io/kube-apiserver:v1.32.0
      command:
        - kube-apiserver
        - --feature-gates=DynamicResourceAllocation=true
        - --runtime-config=resource.k8s.io/v1beta2=true
        # ... 其他参数

---
# ============================================
# 2. kube-scheduler 启用 DRA 插件
# ============================================
apiVersion: v1
kind: ConfigMap
metadata:
  name: scheduler-config
  namespace: kube-system
data:
  scheduler-config.yaml: |
    apiVersion: kubescheduler.config.k8s.io/v1
    kind: KubeSchedulerConfiguration
    profiles:
      - schedulerName: default-scheduler
        plugins:
          queueSort:
            enabled:
              - name: DRAQueueSort
          preFilter:
            enabled:
              - name: DRAPreFilter
          filter:
            enabled:
              - name: DRAFilter
          preScore:
            enabled:
              - name: DRAPreScore
          score:
            enabled:
              - name: DRAScore
              - weight: 10
          reserve:
            enabled:
              - name: DRAReserve
          permit:
            enabled:
              - name: DRAPermit
          preBind:
            enabled:
              - name: DRAPreBind
          postBind:
            enabled:
              - name: DRAPostBind
        pluginConfig:
          - name: DRAPreFilter
            args:
              structuredParameters:
                enabled: true

---
# ============================================
# 3. kubelet 启用 DRA
# ============================================
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
featureGates:
  DynamicResourceAllocation: true
  GRPCContainerProbe: true
driverRegistration:
  enabled: true
```

---

<!-- chunk: 5. 完整 YAML 配置示例 -->
## 5. 完整 YAML 配置示例

### 5.1 场景：AI 训练 Pod 请求 NVIDIA H100 GPU (MIG 分区)

```yaml
# ============================================
# Step 1: 定义 ResourceClass (集群管理员)
# ============================================
apiVersion: resource.k8s.io/v1beta2
kind: ResourceClass
metadata:
  name: nvidia-gpu-h100-mig
  labels:
    vendor: nvidia
    product: h100
spec:
  driverName: gpu.nvidia.com
  parametersRef:
    apiGroup: resource.k8s.io/v1beta2
    kind: ResourceClaimParameters
    name: h100-mig-default
  shareable: false
  nodeSelector:
    nodeSelectorTerms:
      - matchExpressions:
          - key: nvidia.com/gpu.product
            operator: In
            values: ["NVIDIA-H100-80GB-HBM3"]

---
# ============================================
# Step 2: 定义 ResourceClaimParameters (可选配置)
# ============================================
apiVersion: resource.k8s.io/v1beta2
kind: ResourceClaimParameters
metadata:
  name: h100-mig-default
  namespace: default
spec:
  driverName: gpu.nvidia.com
  parameters:
    apiVersion: gpu.nvidia.com/v1alpha1
    kind: GpuConfig
    profile: "1g.10gb"
    computeMode: "exclusive"

---
# ============================================
# Step 3: 定义 ResourceClaimTemplate (用于 StatefulSet/Deployment)
# ============================================
apiVersion: resource.k8s.io/v1beta2
kind: ResourceClaimTemplate
metadata:
  name: nvidia-gpu-claim-template
  namespace: ai-training
spec:
  metadata:
    labels:
      app: training-job
  spec:
    resourceClassName: nvidia-gpu-h100-mig
    parametersRef:
      apiGroup: resource.k8s.io/v1beta2
      kind: ResourceClaimParameters
      name: training-gpu-params
    devices: 2

---
# ============================================
# Step 4: 创建 Pod，使用 ResourceClaim (直接模式)
# ============================================
apiVersion: v1
kind: Pod
metadata:
  name: llm-training-job
  namespace: ai-training
spec:
  resourceClaims:
    - name: gpu
      resourceClaimTemplateName: nvidia-gpu-claim-template
  containers:
    - name: pytorch
      image: nvcr.io/nvidia/pytorch:24.02-py3
      resources:
        claims:
          - name: gpu
      env:
        - name: NVIDIA_VISIBLE_DEVICES
          value: "all"
        - name: CUDA_VISIBLE_DEVICES
          value: "all"
      volumeMounts:
        - name: data
          mountPath: /data
  volumes:
    - name: data
      persistentVolumeClaim:
        claimName: training-data-pvc

---
# ============================================
# Step 5: 使用 StatefulSet + ResourceClaimTemplate (推荐模式)
# ============================================
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: distributed-training
  namespace: ai-training
spec:
  serviceName: training-headless
  replicas: 4
  selector:
    matchLabels:
      app: training-worker
  template:
    metadata:
      labels:
        app: training-worker
    spec:
      resourceClaims:
        - name: gpu
          resourceClaimTemplateName: nvidia-gpu-claim-template
      containers:
        - name: worker
          image: nvcr.io/nvidia/pytorch:24.02-py3
          resources:
            claims:
              - name: gpu
          command:
            - python
            - -m
            - torch.distributed.run
            - --nproc_per_node=auto
            - /app/train.py
```

### 5.2 场景：多设备组合 (GPU + RDMA NIC)

```yaml
# ============================================
# 多设备组合: GPU + 关联 RDMA NIC
# ============================================
apiVersion: v1
kind: Pod
metadata:
  name: gpudirect-rdma-workload
  namespace: ai-training
spec:
  resourceClaims:
    - name: gpu
      resourceClaimName: h100-gpu-claim
    - name: rdma
      resourceClaimName: mellanox-rdma-claim
  containers:
    - name: training
      image: nvcr.io/nvidia/pytorch:24.02-py3
      resources:
        claims:
          - name: gpu
          - name: rdma
      env:
        - name: NCCL_IB_HCA
          value: "mlx5_0"
```

---

<!-- chunk: 6. 内部架构与工作流程 -->
## 6. 内部架构与工作流程

### 6.1 整体架构图

```
+-------------------------------------------------------------------------------+
|                    DRA Internal Architecture                                   |
+-------------------------------------------------------------------------------+
|                                                                                |
|  +-------------------------------------------------------------------------+   |
|  |                        Control Plane                                     |   |
|  |  +------------------------------------------------------------------+   |   |
|  |  |                     kube-scheduler                                |   |   |
|  |  |  +------------+ +------------+ +------------+ +--------------+   |   |   |
|  |  |  |DRAQueueSort| |DRAPreFilter| | DRAFilter  | |  DRAPreScore |   |   |   |
|  |  |  +------------+ +------------+ +------------+ +--------------+   |   |   |
|  |  |  +------------+ +------------+ +------------+ +--------------+   |   |   |
|  |  |  |  DRAScore  | |DRAReserve  | |  DRAPermit | |  DRAPreBind  |   |   |   |
|  |  |  +------------+ +------------+ +------------+ +--------------+   |   |   |
|  |  +------------------------------------------------------------------+   |   |
|  |                                    |                                    |   |
|  |                                    v                                    |   |
|  |  +------------------------------------------------------------------+   |   |
|  |  |                    API Server                                     |   |   |
|  |  |  ResourceClaim  ResourceSlice  ResourceClass  Pod(binding)       |   |   |
|  |  +------------------------------------------------------------------+   |   |
|  +-------------------------------------------------------------------------+   |
|                                    |                                           |
|                                    v                                           |
|  +-------------------------------------------------------------------------+   |
|  |                        Worker Node                                       |   |
|  |  +------------------------------------------------------------------+   |   |
|  |  |                      kubelet                                      |   |   |
|  |  |  +-----------------+  +-----------------+  +------------------+   |   |   |
|  |  |  |  DRA Manager    |  |  Device Plugin  |  |   CSI Driver     |   |   |   |
|  |  |  |  (v1.30+新组件) |  |  (兼容模式)     |  |                  |   |   |   |
|  |  |  +-----------------+  +-----------------+  +------------------+   |   |   |
|  |  |            |                   |                                   |   |   |
|  |  |            v                   v                                   |   |   |
|  |  |  +-----------------+  +-----------------+                          |   |   |
|  |  |  |  DRA Driver     |  |  Device Plugin  |                          |   |   |
|  |  |  |  (gRPC)         |  |  (gRPC)         |                          |   |   |
|  |  |  |  - NodePrepare  |  |  - ListAndWatch |                          |   |   |
|  |  |  |  - NodeUnprepare|  |  - Allocate     |                          |   |   |
|  |  |  +-----------------+  +-----------------+                          |   |   |
|  |  |            |                   |                                   |   |   |
|  |  |            v                   v                                   |   |   |
|  |  |  +----------------------------------------------------------+     |   |   |
|  |  |  |  设备运行时 (CDI Spec)                                      |     |   |   |
|  |  |  |  /var/run/cdi/nvidia.com-gpu-xxx.json                     |     |   |   |
|  |  |  +----------------------------------------------------------+     |   |   |
|  |  |            |                                                        |   |   |
|  |  |            v                                                        |   |   |
|  |  |  +----------------------------------------------------------+     |   |   |
|  |  |  |  containerd/CRI-O                                           |     |   |   |
|  |  |  |  根据 CDI Spec 注入设备到容器 namespace                       |     |   |   |
|  |  |  +----------------------------------------------------------+     |   |   |
|  |  +------------------------------------------------------------------+   |   |
|  +-------------------------------------------------------------------------+   |
|                                                                                |
+-------------------------------------------------------------------------------+
```

### 6.2 调度阶段详细流程

| 阶段 | 插件 | 职责 |
|:-----|:-----|:-----|
| **1. PreFilter** | `DRAPreFilter` | 解析 Pod 引用的 ResourceClaim，验证 Claim 状态是否可分配 (Pending) |
| **2. Filter** | `DRAFilter` | 逐节点过滤：检查 ResourceSlice 是否包含满足条件的设备；验证拓扑约束 (NUMA/PCIe 亲和性)；检查设备污点与 Claim 容忍度 |
| **3. PreScore** | `DRAPreScore` | 计算各节点可用设备的拓扑评分；考虑设备间的通信延迟 (NVLink, PCIe switch) |
| **4. Score** | `DRAScore` | 拓扑最优排序 (LeastTopologyDistance)；负载均衡 (MostAllocated/LeastAllocated) |
| **5. Reserve** | `DRAReserve` | 在选定节点上预占资源 (乐观锁)；更新 ResourceClaim 的 allocation 字段；设置 reservedFor 指向目标 Pod |
| **6. Permit/PreBind/Bind** | `DRAPermit`, `DRAPreBind`, `DRAPostBind` | 确认 ResourceClaim 进入 Reserved 状态；写入 Pod 的 nodeName 绑定结果 |

### 6.3 节点阶段详细流程

| 阶段 | 执行者 | 操作 |
|:-----|:-------|:-----|
| **1. 解析 Claim** | kubelet | 读取 Pod spec.resourceClaims；解析 claimName -> ResourceClaim 对象；检查 Claim 是否已 Allocated 且绑定到本节点 |
| **2. 设备准备** | DRA Driver (gRPC NodePrepareResources) | 传入 Claim allocation 结果 (选定设备列表)；Driver 执行设备初始化 (如 MIG 分区创建)；Driver 生成 CDI Spec 文件到 /var/run/cdi/ |
| **3. 状态更新** | kubelet | 更新 ResourceClaim 状态为 Prepared；确认设备已就绪，容器可以启动 |
| **4. 容器创建** | CRI (containerd/CRI-O) | 读取 CDI Spec；配置设备 cgroup 权限 (/dev/nvidia*)；注入环境变量 (NVIDIA_VISIBLE_DEVICES 等) |
| **5. 资源释放** | DRA Driver (gRPC NodeUnprepareResources) | 回收设备资源 (删除 MIG 分区等)；清理 CDI Spec 文件；更新 ResourceClaim 状态为 Deallocated |

---

<!-- chunk: 7. 使用场景与最佳实践 -->
## 7. 使用场景与最佳实践

### 7.1 GPU 共享与分区 (MIG, MPS)

| 模式 | 传统 Device Plugin | DRA 方式 | 优势 |
|:-----|:-------------------|:---------|:-----|
| **MIG (Multi-Instance GPU)** | 需 NVIDIA Device Plugin + GPU Operator 复杂配置 | ResourceClass + parametersRef 声明 `profile` | 调度器原生感知 MIG slice，精确分配 |
| **MPS (Multi-Process Service)** | 通过时间片模拟共享，无隔离 | DRA 允许多个 Pod 引用同一 Claim (`shareable: true`) | 显存/计算资源半虚拟化隔离 |
| **显存超售** | 不支持 | CEL 表达式验证显存需求 | 细粒度显存分配与限制 |

```yaml
# MIG 分区示例: 声明 2 个 3g.40gb 分区
apiVersion: resource.k8s.io/v1beta2
kind: ResourceClaim
metadata:
  name: mig-3g40gb-claim
  namespace: ai-training
spec:
  resourceClassName: nvidia-gpu-h100-mig
  parametersRef:
    apiGroup: resource.k8s.io/v1beta2
    kind: ResourceClaimParameters
    name: mig-3g40gb-params
---
apiVersion: resource.k8s.io/v1beta2
kind: ResourceClaimParameters
metadata:
  name: mig-3g40gb-params
  namespace: ai-training
spec:
  driverName: gpu.nvidia.com
  parameters:
    apiVersion: gpu.nvidia.com/v1alpha1
    kind: GpuConfig
    profile: "3g.40gb"
    count: 2
```

### 7.2 FPGA 动态重配置

FPGA 设备的特点是同一硬件可通过不同 bitstream 实现不同功能。DRA 的 `parametersRef` 完美支持此场景：

```yaml
apiVersion: resource.k8s.io/v1beta2
kind: ResourceClass
metadata:
  name: xilinx-fpga-u50
spec:
  driverName: fpga.xilinx.com
---
apiVersion: resource.k8s.io/v1beta2
kind: ResourceClaimParameters
metadata:
  name: fpga-crypto-accel
  namespace: default
spec:
  driverName: fpga.xilinx.com
  parameters:
    apiVersion: fpga.xilinx.com/v1
    kind: FpgaConfig
    bitstream: "crypto-accel-v2.1.xclbin"
    shellVersion: "xilinx_u50_gen3x16_xdma_201920_3"
    memoryBanks: ["bank0", "bank1"]
```

| 阶段 | 行为 |
|:-----|:-----|
| 调度 | DRA Filter 检查节点 FPGA 是否支持目标 shellVersion |
| 预留 | DRAReserve 锁定目标 FPGA 设备 |
| 准备 | DRA Driver 调用 `xbutil program` 加载 bitstream |
| 运行 | CDI 注入 FPGA 设备到容器，应用获得加速能力 |
| 释放 | Driver 可选择保留 bitstream 或恢复默认 shell |

### 7.3 RDMA 网络资源分配

```yaml
# RDMA 资源类: Mellanox ConnectX-7
apiVersion: resource.k8s.io/v1beta2
kind: ResourceClass
metadata:
  name: mellanox-rdma-cx7
spec:
  driverName: rdma.mellanox.com
  nodeSelector:
    nodeSelectorTerms:
      - matchExpressions:
          - key: network.mellanox.com/rdma-capable
            operator: In
            values: ["true"]
---
# RDMA Claim 参数: 指定带宽和队列对数量
apiVersion: resource.k8s.io/v1beta2
kind: ResourceClaimParameters
metadata:
  name: rdma-100gbe-params
  namespace: ai-training
spec:
  driverName: rdma.mellanox.com
  parameters:
    apiVersion: rdma.mellanox.com/v1alpha1
    kind: RdmaConfig
    minBandwidthGbps: 100
    maxQueuePairs: 1024
    roceVersion: "v2"
    pfcEnabled: true
```

### 7.4 多设备组合 (GPU + NIC)

DRA 的核心优势之一是支持跨 ResourceClass 的设备组合，并确保拓扑亲和性：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: nccl-allreduce-job
  namespace: ai-training
spec:
  resourceClaims:
    - name: gpu
      resourceClaimTemplateName: nvidia-gpu-h100-claim-template
    - name: rdma
      resourceClaimTemplateName: mellanox-rdma-claim-template
  topologySpreadConstraints:
    - maxSkew: 1
      topologyKey: topology.kubernetes.io/numa-id
      whenUnsatisfiable: DoNotSchedule
      labelSelector:
        matchLabels:
          app: nccl-job
  containers:
    - name: training
      image: nvcr.io/nvidia/pytorch:24.02-py3
      resources:
        claims:
          - name: gpu
          - name: rdma
```

| 组合场景 | DRA 实现方式 | 拓扑保证 |
|:---------|:-------------|:---------|
| GPU + NVLink Bridge | 同一 ResourceClass 内多设备 | 调度器选择 NVLink 直连的 GPU 对 |
| GPU + RDMA NIC | 跨 ResourceClass 组合 Claim | DRA Filter 强制同一 NUMA/PCIe Switch |
| GPU + Local NVMe | 跨 ResourceClass + 节点亲和 | Score 插件优先本地存储节点 |
| Multi-GPU (8x) | 同一 Claim 请求 8 设备 | 拓扑评分选择 HGX 基板最优布局 |

---

<!-- chunk: 8. 当前限制与未来演进 -->
## 8. 当前限制与未来演进

### 8.1 v1.30 - v1.32 已知限制

| 限制类别 | 具体描述 | 影响 | 预计解决版本 |
|:---------|:---------|:-----|:-------------|
| **API 稳定性** | `resource.k8s.io/v1beta2` 仍在演进，v1beta1 已废弃 | 升级需注意 API 变更 | v1.33+ (v1 GA) |
| **驱动生态** | 主流厂商 DRA Driver 尚未完全成熟 | NVIDIA/AMD/Intel 驱动仍在开发 | 2025-2026 |
| **调度性能** | 复杂设备拓扑评分增加调度延迟 | 大规模集群 (>1000节点) 可能受影响 | v1.32+ 持续优化 |
| **Preemption** | DRA ResourceClaim 的抢占策略不完善 | 高优先级 Pod 可能无法抢占 DRA 资源 | v1.33 |
| **Cluster Autoscaler** | 自动扩缩容尚未完全支持 DRA | GPU 节点弹性伸缩需手动配置 | v1.33 |
| **监控指标** | DRA 专用 metrics 不完善 | 设备利用率监控需补充 | v1.32+ |
| **Device Plugin 共存** | 同一节点混用 DRA 和 Device Plugin 需谨慎 | 资源重复计算风险 | v1.32+ 改进 |
| **Windows 支持** | Windows 节点 DRA 支持有限 | Windows 容器异构设备调度受限 | 待定 |

### 8.2 未来演进路线

```
+-------------------------------------------------------------------------------+
|                    DRA Evolution Roadmap                                       |
+-------------------------------------------------------------------------------+
|                                                                                |
|  v1.30 (Alpha)        v1.31-v1.32 (Beta)         v1.33+ (GA/Stable)          |
|       |                     |                            |                     |
|       v                     v                            v                     |
|  +----------+          +----------+               +----------+                |
|  | 基础 API |    --->  | 调度插件 |         --->  | 完整生态 |                |
|  | 结构化   |          | 自动控制器|               | 抢占支持 |                |
|  | 参数     |          | 设备状态  |               | 弹性伸缩 |                |
|  +----------+          +----------+               +----------+                |
|       |                     |                            |                     |
|  +----------+          +----------+               +----------+                |
|  | 单一     |    --->  | 多设备   |         --->  | 智能     |                |
|  | 设备分配 |          | 组合分配  |               | 调度     |                |
|  |          |          | 拓扑感知  |               | 预测     |                |
|  +----------+          +----------+               +----------+                |
|       |                     |                            |                     |
|  +----------+          +----------+               +----------+                |
|  | 手动     |    --->  | 模板化   |         --->  | 全自动化 |                |
|  | Claim管理|          | Template  |               | 运维     |                |
|  +----------+          +----------+               +----------+                |
|                                                                                |
+-------------------------------------------------------------------------------+
```

### 8.3 社区重点关注方向

1. **DRA + Cluster Autoscaler**: 实现基于 ResourceClaim 的节点自动扩展
2. **DRA + Pod Scheduling Readiness**: 支持更复杂的资源依赖调度
3. **跨节点资源聚合**: 支持网络连接设备的集群级视图
4. **DRA Metrics 标准化**: 通过 Prometheus 暴露设备分配状态
5. **Device Health 监控**: ResourceClaim 中集成设备健康检查

---

<!-- chunk: 9. Device Plugin 迁移路径 -->
## 9. Device Plugin 迁移路径

### 9.1 迁移策略总览

| 阶段 | 策略 | 适用场景 | 时间线 |
|:-----|:-----|:---------|:-------|
| **共存** | 同一集群部分节点用 DRA，部分用 Device Plugin | 大型集群，逐步迁移 | 当前 - v1.33 |
| **兼容** | Device Plugin 实现 DRA 兼容层 (CDI 模式) | 厂商驱动过渡 | v1.30 - v1.34 |
| **切换** | 全节点切换为 DRA Driver | 新集群或重建节点 | v1.32+ |
| **废弃** | Device Plugin 机制标记为废弃 | 长期规划 | v1.35+ (预计) |

### 9.2 共存模式配置

```yaml
# 节点标签区分 DRA 和 Device Plugin 节点
apiVersion: v1
kind: Node
metadata:
  name: gpu-node-01
  labels:
    nvidia.com/dra-enabled: "true"
    nvidia.com/gpu.product: "H100"
spec: {}
---
apiVersion: v1
kind: Node
metadata:
  name: gpu-node-02
  labels:
    nvidia.com/dra-enabled: "false"
    nvidia.com/gpu.product: "A100"
spec: {}
```

```yaml
# DRA Pod 调度到 DRA 节点
apiVersion: v1
kind: Pod
metadata:
  name: dra-workload
spec:
  nodeSelector:
    nvidia.com/dra-enabled: "true"
  resourceClaims:
    - name: gpu
      resourceClaimTemplateName: nvidia-gpu-dra-template
  containers:
    - name: training
      image: nvcr.io/nvidia/pytorch:24.02-py3
      resources:
        claims:
          - name: gpu
---
# Device Plugin Pod 调度到传统节点
apiVersion: v1
kind: Pod
metadata:
  name: device-plugin-workload
spec:
  nodeSelector:
    nvidia.com/dra-enabled: "false"
  containers:
    - name: training
      image: nvcr.io/nvidia/pytorch:24.02-py3
      resources:
        limits:
          nvidia.com/gpu: "1"
```

### 9.3 驱动迁移检查清单

| 检查项 | Device Plugin 实现 | DRA Driver 实现 | 备注 |
|:-------|:-------------------|:----------------|:-----|
| **设备发现** | `ListAndWatch` gRPC | `GetAvailableResources` + ResourceSlice | DRA 通过 API 对象上报 |
| **设备分配** | `Allocate` gRPC | `NodePrepareResources` gRPC | DRA 增加设备准备阶段 |
| **设备释放** | 隐式 (kubelet 断开 gRPC) | `NodeUnprepareResources` gRPC | DRA 显式释放资源 |
| **参数传递** | 环境变量/注解 | `parametersRef` 结构化参数 | DRA 类型安全 |
| **健康检查** | `Health` stream | ResourceClaim deviceStatus | DRA 集群级可见 |
| **CDI 支持** | 可选 | 必需 | DRA 依赖 CDI 规范 |

### 9.4 推荐的迁移步骤

```
+-------------------------------------------------------------------------------+
|                    Recommended Migration Steps                                 |
+-------------------------------------------------------------------------------+
|                                                                                |
|  Step 1: 评估                                                                  |
|  +----------------------------------------------------------+                  |
|  | - 清点当前 Device Plugin 管理的设备类型                  |                  |
|  | - 确认厂商是否已提供 DRA Driver (或 CDI 兼容层)          |                  |
|  | - 检查 Kubernetes 版本是否 >= v1.30 (推荐 v1.32+)        |                  |
|  +----------------------------------------------------------+                  |
|         |                                                                      |
|         v                                                                      |
|  Step 2: 试点                                                                  |
|  +----------------------------------------------------------+                  |
|  | - 选择 1-2 个节点作为 DRA 试点                           |                  |
|  | - 部署 DRA Driver (保持 Device Plugin 在其他节点运行)    |                  |
|  | - 部署测试应用，验证 ResourceClaim -> Pod 端到端流程     |                  |
|  +----------------------------------------------------------+                  |
|         |                                                                      |
|         v                                                                      |
|  Step 3: 共存                                                                  |
|  +----------------------------------------------------------+                  |
|  | - 使用节点标签区分 DRA 节点和传统节点                    |                  |
|  | - 逐步将新业务切换到 DRA，旧业务保持 Device Plugin       |                  |
|  | - 监控 DRA 调度性能和资源利用率                          |                  |
|  +----------------------------------------------------------+                  |
|         |                                                                      |
|         v                                                                      |
|  Step 4: 切换                                                                  |
|  +----------------------------------------------------------+                  |
|  | - 节点级切换: 停止 Device Plugin，启用 DRA Driver        |                  |
|  | - 应用级迁移: 将 Pod 的 limits 改为 resourceClaims       |                  |
|  | - 使用 ResourceClaimTemplate 简化 Deployment/StatefulSet |                  |
|  +----------------------------------------------------------+                  |
|         |                                                                      |
|         v                                                                      |
|  Step 5: 废弃                                                                  |
|  +----------------------------------------------------------+                  |
|  | - 全部节点完成 DRA 切换                                  |                  |
|  | - 卸载 Device Plugin DaemonSet                           |                  |
|  | - 关闭 Device Plugin 相关 Feature Gate                   |                  |
|  +----------------------------------------------------------+                  |
|                                                                                |
+-------------------------------------------------------------------------------+
```

### 9.5 代码迁移示例

| 原 Device Plugin 方式 | DRA 方式 |
|:----------------------|:---------|
| `limits: nvidia.com/gpu: "2"` | `resourceClaims: [{name: gpu, resourceClaimTemplateName: ...}]` + `resources.claims: [{name: gpu}]` |
| `spec.nodeSelector: {nvidia.com/gpu.product: H100}` | `ResourceClass.nodeSelector` 中统一配置 |
| `annotations: nvidia.com/mig.config: "all-1g.10gb"` | `ResourceClaimParameters.parameters.profile: "1g.10gb"` |
| `spec.containers[*].env: [{name: NVIDIA_VISIBLE_DEVICES, value: "0,1"}]` | CDI 自动注入，通常无需手动设置 |

---

<!-- chunk: 参考文档 -->
## 参考文档

| 文档 | 链接 |
|:-----|:-----|
| KEP-3063: Dynamic Resource Allocation | https://github.com/kubernetes/enhancements/tree/master/keps/sig-scheduling/3063-dynamic-resource-allocation |
| Kubernetes DRA 官方文档 | https://kubernetes.io/docs/concepts/scheduling-eviction/dynamic-resource-allocation/ |
| CDI (Container Device Interface) Spec | https://github.com/cncf-tags/container-device-interface |
| NVIDIA DRA Driver (alpha) | https://github.com/NVIDIA/k8s-dra-driver |
| DRA 调度框架插件设计 | https://github.com/kubernetes-sigs/scheduler-plugins |

---

> **总结**: Dynamic Resource Allocation 代表了 Kubernetes 异构资源调度的未来方向。虽然当前 (v1.30-v1.32) 仍处于 Alpha/Beta 阶段，生态尚未完全成熟，但对于需要细粒度 GPU 分区、FPGA 动态重配置、RDMA 网络分配或多设备组合拓扑感知的场景，DRA 已展现出传统 Device Plugin 无法比拟的优势。建议在生产环境中采用「共存渐进」策略，先从新集群或 AI/ML 专用节点开始试点，逐步完成迁移。

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 MOC
- [[集群基础/README.md|Domain-3: Kubernetes控制平面]]
- Domain-3 控制平面 — 开源项目索引
- Kubernetes 控制平面架构总览 (Control Plane Architecture Overview)
- 控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)
- 控制平面高可用部署模式 (Control Plane High Availability Deployment Patt...
- 控制平面安全加固指南 (Control Plane Security Hardening Guide)
- 控制平面监控与可观测性 (Control Plane Monitoring & Observability)
- 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)
- 控制平面升级与迁移策略 (Control Plane Upgrade & Migration Strategy)
- 控制平面性能基准测试 (Control Plane Performance Benchmarking)
- 控制平面扩缩容指南 (Control Plane Scalability Guide)

## See Also

- 28-api-extension-deep-dive
- 29-in-place-pod-resize
- 31-kubectl-complete-reference
- 32-kubeadm-cluster-lifecycle

## Related

- [[生态参考/领域索引/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]


<!-- risk-assessed -->
