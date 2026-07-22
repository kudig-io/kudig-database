---
title: Dynamic Resource Allocation
summary: Dynamic Resource Allocation (DRA) 是 Kubernetes 中用于动态分配硬件资源的机制。
category: concepts
tags:
- dra
- scheduling
- resource-management
- visibility/public
tier: supporting
sources:
- conceptss/
created: 2026-05-24
updated: 2026-07
last_updated: 2026-07
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。

# Dynamic Resource Allocation (DRA)

## 概述

Dynamic Resource Allocation（DRA）是 Kubernetes 中用于动态分配硬件资源的全新框架，于 K8s 1.26 引入（alpha），1.32 进入 beta。它取代了传统的 Device Plugin 框架，提供了更灵活、更通化的硬件资源声明和分配机制。DRA 的核心价值在于支持**复杂的硬件拓扑**（如 GPU NVLink 拓扑、FPGA 区域）、**按参数声明**资源（如"分配一张至少 16GB 显存的 GPU"）、以及让驱动程序直接参与调度决策。

## 技术原理

### 与 Device Plugin 的对比

| 维度 | Device Plugin（旧） | DRA（新） |
|------|---------------------|-----------|
| 资源声明 | 整数计数（`nvidia.com/gpu: 2`） | 结构化参数（显存大小、拓扑偏好） |
| 拓扑感知 | 无原生支持 | 原生支持 NUMA、NVLink 等 |
| 调度集成 | 只报告可用数量 | 驱动可以 Claim/Reserve，参与调度 |
| 扩展性 | 每种硬件一套 Device Plugin | 统一的 ResourceClaim CRD |
| 厂商锁定制 | 通过 extended resources | 通过 ResourceClass + driver |

### 核心对象

DRA 引入了几个新的 API 对象：

- **ResourceClass**：类似于 StorageClass，定义资源供应者（如"NVIDIA GPU 驱动"）和默认参数
- **ResourceClaim**：用户对硬件资源的声明式请求，包含结构化参数
- **ResourceClaimTemplate**：在 Pod 级别生成 ResourceClaim 的模板

### 调度流程

```
1. Pod 引用 ResourceClaimTemplate
2. 准入控制器自动创建 ResourceClaim
3. 调度器遍历节点，调用 DRA 驱动的 Reserve() 方法
   → 驱动检查节点上的硬件是否满足 Claim 参数
   → 满足: 驱动标记该资源为已预留
   → 不满足: 调度器尝试下一个节点
4. Pod 绑定到节点后，kubelet 调用驱动分配实际硬件
5. 容器启动时通过 CDI（Container Device Interface）获取设备
```

## 生产示例

### 定义 ResourceClass

```yaml
apiVersion: resource.k8s.io/v1beta1
kind: ResourceClass
metadata:
  name: gpu.nvidia.com
driverName: gpu.nvidia.com            # DRA 驱动名称
```

### 定义带参数的 ResourceClaimTemplate

```yaml
apiVersion: resource.k8s.io/v1beta1
kind: ResourceClaimTemplate
metadata:
  name: gpu-claim-template
spec:
  spec:
    resourceClassName: gpu.nvidia.com
    devices:
      requests:
        - name: gpu
          deviceClassName: gpu.nvidia.com
          count: 1
          # 结构化参数：可以指定显存、计算能力等
```

### Pod 引用 ResourceClaim

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: ml-inference
spec:
  resourceClaims:
    - name: gpu-claim
      resourceClaimTemplateName: gpu-claim-template
  containers:
    - name: model-server
      image: vllm/vllm:latest
      resources:
        claims:
          - name: gpu-claim            # 容器使用该 ResourceClaim
```

## 最佳实践

- **逐步迁移**：K8s 1.32+ 新集群优先使用 DRA；旧集群保持 Device Plugin，通过版本策略逐步切换
- **CDI 设备路径验证**：升级到支持 CDI（Container Device Interface）的容器运行时（containerd 1.7+），确保设备正确注入容器
- **驱动健康检查**：监控 DRA 驱动 Pod 的健康状态，驱动崩溃会导致资源分配卡死
- **与 gang scheduling 配合**：分布式训练场景中，将 DRA 与 Volcano/Kueue 的 gang scheduling 结合使用
- **ResourceClaim 清理策略**：配置 Pod 删除后自动清理 ResourceClaim，避免资源泄漏

## 常见陷阱

- **alpha/beta 版本混用**：DRA API 在 alpha（1.26-1.31）和 beta（1.32+）之间有结构变化，跨版本升级需迁移 Claim 定义
- **驱动未安装**：如果集群中没有安装对应的 DRA 驱动，ResourceClaim 会一直处于未分配状态，Pod 永远 Pending
- **调度延迟增加**：DRA 的 Reserve/Unreserve 两阶段调度比 Device Plugin 的简单计数更耗时，大规模集群需关注调度吞吐

## 源码实现分析

### DRA 调度器集成流程

```go
// k8s.io/kubernetes/pkg/scheduler/framework/plugins/dynamicresources/dynamicresources.go
func (pl *dynamicResources) PreBind(ctx context.Context, state *framework.CycleState, pod *v1.Pod, nodeName string) *framework.Status {
    // 1. 获取 Pod 关联的所有 ResourceClaim
    claims := pl.getClaimsForPod(pod)
    for _, claim := range claims {
        // 2. 检查 Claim 是否已分配（AllocationResult）
        if claim.Status.Allocation == nil {
            // 3. 调用 DRA 驱动的 Allocate RPC
            allocation, err := pl.draManager.Allocate(ctx, claim, nodeName)
            // 4. 更新 Claim 的 Status.Allocation
            claim.Status.Allocation = allocation
            pl.client.ResourceV1().ResourceClaims(claim.Namespace).UpdateStatus(ctx, claim)
        }
    }
    return nil
}
// Reserve 阶段：预留设备，防止并发调度冲突
func (pl *dynamicResources) Reserve(ctx context.Context, state *framework.CycleState, pod *v1.Pod, nodeName string) *framework.Status {
    // 将已分配设备标记为 in-use，防止其他 Pod 抢占
    pl.draManager.ReserveDevices(pod, nodeName)
    return nil
}
// Unreserve 阶段：调度失败时释放预留
func (pl *dynamicResources) Unreserve(ctx context.Context, state *framework.CycleState, pod *v1.Pod, nodeName string) {
    pl.draManager.UnreserveDevices(pod, nodeName)
}
```

### DRA vs Device Plugin 架构对比

```
┌──────────────────────────────────────────────────────────┐
│  Device Plugin (旧)          DRA (新)                    │
├──────────────────────────────────────────────────────────┤
│  kubelet ←gRPC→ DP        scheduler ←gRPC→ DRA Driver  │
│  简单计数模型              结构化参数模型              │
│  节点本地分配              调度器全局分配              │
│  不支持跨节点/网络存储    支持任意设备拓扑            │
│  无延迟分配              支持 Reserve/Unreserve       │
│  K8s 1.10+               K8s 1.32+ (beta)             │
└──────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：GPU 资源声明与分配

```yaml
# 🟡 中风险：创建 DRA 资源影响调度
apiVersion: resource.k8s.io/v1beta1
kind: ResourceClaimTemplate
metadata:
  name: gpu-claim-template
spec:
  spec:
    devices:
      requests:
      - name: gpu
        deviceClassName: gpu.nvidia.com
        count: 2  # 请求 2 块 GPU
        selectors:
        - cel:
            expression: |
              device.attributes["nvidia.com"].product == "A100" &&
              device.attributes["nvidia.com"].memory >= quantity("40Gi")
```

### 场景二：检查 DRA 资源状态

```bash
# 🟢 低风险：只读查询
kubectl get resourceclaims -A  # 查看所有资源声明
kubectl get resourceclaims gpu-claim -o yaml  # 查看分配结果
kubectl get deviceclasses  # 查看设备类
kubectl describe resourceclaim gpu-claim  # 查看分配详情和事件
# 检查 DRA 驱动状态
kubectl get pods -n kube-system -l app=nvidia-dra-driver
kubectl logs -n kube-system -l app=nvidia-dra-driver --tail=50
```

### 场景三：分布式训练 Gang Scheduling + DRA

```yaml
# 🟡 中风险：创建分布式训练工作负载
apiVersion: v1
kind: Pod
metadata:
  name: training-worker-0
  labels:
    job-name: distributed-training
spec:
  resourceClaims:
  - name: gpu-claim
    resourceClaimTemplateName: gpu-claim-template
  containers:
  - name: trainer
    image: nvcr.io/nvidia/pytorch:24.05-py3
    resources:
      claims:
      - name: gpu-claim
  # 配合 Kueue/Volcano gang scheduling 确保所有 worker 同时获得 GPU
```

## 常见误区

| # | 误区 | 正确理解 |
|---|------|----------|
| 1 | DRA 完全替代 Device Plugin | DRA 是补充而非替代；简单 GPU 计数场景 Device Plugin 仍可用 |
| 2 | ResourceClaim 创建即分配 | Claim 创建后需等待调度器 Allocate；无可用设备时一直 Pending |
| 3 | DRA 只支持 GPU | DRA 支持任何设备：FPGA、RDMA NIC、NVMe、专用加速器 |
| 4 | alpha 和 beta API 兼容 | 1.26-1.31 alpha 与 1.32+ beta 结构不同，升级需迁移 |
| 5 | DRA 驱动崩溃无影响 | 驱动崩溃导致新 Claim 无法分配，Pod 永远 Pending |
| 6 | ResourceClaim 随 Pod 删除自动清理 | 需配置 deletionPolicy 或手动清理，否则资源泄漏 |

## 面试要点

1. **Q: DRA 与 Device Plugin 的核心区别是什么？**
   A: ① 分配位置：Device Plugin 在 kubelet 节点本地分配；DRA 在调度器全局分配（支持跨节点拓扑感知）。② 参数模型：Device Plugin 简单计数（nvidia.com/gpu: 2）；DRA 结构化参数（指定型号、显存、计算能力）。③ 生命周期：DRA 支持 Reserve/Unreserve 两阶段，防止并发冲突。④ 设备类型：DRA 支持任意设备（GPU/FPGA/RDMA/NVMe），不限于 kubelet 插件。

2. **Q: DRA 的调度流程是怎样的？**
   A: ① Pod 创建时引用 ResourceClaimTemplate；② 调度器 Filter 阶段检查节点是否有满足条件的设备；③ Reserve 阶段调用 DRA 驱动 Allocate RPC，获取具体设备分配；④ PreBind 阶段更新 Claim Status.Allocation；⑤ kubelet 通过 CDI 将设备注入容器；⑥ 调度失败时 Unreserve 释放预留。

3. **Q: 生产环境使用 DRA 需要注意什么？**
   A: ① 版本要求：K8s 1.32+ beta，需启用 DynamicResourceAllocation feature gate；② 驱动健康：监控 DRA 驱动 Pod，崩溃会导致资源分配卡死；③ 清理策略：配置 ResourceClaim 自动清理避免泄漏；④ Gang Scheduling：分布式训练配合 Kueue/Volcano 确保所有 worker 同时获得设备；⑤ CDI 兼容：containerd 1.7+ 支持 CDI 设备注入。

4. **Q: 什么场景应该用 DRA 而非 Device Plugin？**
   A: ① 需要指定设备属性（GPU 型号、显存大小）而非简单计数；② 需要跨节点设备拓扑感知（如 NVLink 连接的 GPU 组）；③ 需要非 GPU 设备（FPGA、RDMA NIC）；④ 需要设备共享/分区（MIG 切片）；⑤ 新集群（1.32+）建议直接用 DRA。简单 GPU 计数场景（如推理服务）Device Plugin 仍够用。

## 相关链接

- [[概念/kubernetes.md|Kubernetes]] — 核心概念
- [[概念/gang-scheduling.md|Gang Scheduling]] — 与 DRA 的协同调度
- [[概念/scheduling-algorithm.md|调度算法]] — 调度器扩展机制

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
