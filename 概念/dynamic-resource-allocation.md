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

## 相关链接

- [[概念/kubernetes.md|Kubernetes]] — 核心概念
- [[概念/gang-scheduling.md|Gang Scheduling]] — 与 DRA 的协同调度
- [[概念/scheduling-algorithm.md|调度算法]] — 调度器扩展机制

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
