---
title: Dynamic Resource Allocation
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- scheduler
- gpu
- nvidia
tier: core
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dynamic Resource Allocation 是什么
- 如何 Dynamic Resource Allocation
trigger_keywords:
- Dynamic
- Resource
- Allocation
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Dynamic Resource Allocation

## 概述

动态资源分配（Dynamic Resource Allocation，DRA）是 [[Kubernetes|Kubernetes]] v1.35 中达到 stable 的特性。它允许用户在 Pod 之间请求和共享资源，这些资源通常是附加设备，如硬件加速器。DRA 提供了比 Device Plugin 更灵活的设备分类、请求和使用方式。

## 核心概念/原理

DRA 涉及以下几类用户：

- **设备所有者**：负责设备，创建支持 DRA 的驱动程序，创建 ResourceSlices 提供节点和资源信息，可选创建设备类（DeviceClass）。
- **集群管理员**：负责配置集群和节点、附加设备、安装驱动程序，可选创建设备类。
- **工作负载操作员**：负责部署和管理工作负载，创建 ResourceClaims 或 ResourceClaimTemplates 来请求设备配置。

### 核心 API 类型

- **DeviceClass**：定义可声明的设备类别，以及如何在声明中选择特定设备属性。
- **ResourceClaim**：描述对集群中附加资源（如设备）的访问请求。为 Pod 提供对特定资源的访问。
- **ResourceClaimTemplate**：定义模板，Kubernetes 用它为工作负载创建每个 Pod 的 ResourceClaim。
- **ResourceSlice**：表示附加到节点的一个或多个资源。驱动程序在集群中创建和管理 ResourceSlice。

## 关键机制或特性

- **灵活设备过滤**：使用通用表达式语言（CEL）对特定设备属性进行细粒度过滤。
- **设备共享**：通过引用相应的 ResourceClaim，多个容器或 Pod 可以共享同一资源。
- **集中式设备分类**：设备驱动和集群管理员可以使用 DeviceClass 为应用操作员提供针对各种用例优化的硬件类别。
- **简化 Pod 请求**：应用操作员无需在 Pod 资源请求中指定设备数量，只需引用 ResourceClaim 即可。
- **优先列表**（v1.34+ beta）：可以在 ResourceClaim 或 ResourceClaimTemplate 的请求中提供优先级子请求列表，调度器会选择第一个可分配的子请求。
- **ResourceClaim 设备状态**（v1.33+ beta）：DRA 驱动可以为 ResourceClaim 中分配的每个设备报告驱动特定的设备状态数据。
- **设备健康监控**（v1.31+ alpha）：监控和报告动态分配基础设施资源的健康状况，通过 Pod 状态中的 `allocatedResourcesStatus` 字段暴露。
- **管理员访问**（v1.34+ beta）：将 ResourceClaim 或 ResourceClaimTemplate 中的请求标记为具有特权功能，用于维护和故障排查。

### Alpha 特性

- **DRA 扩展资源分配**（v1.34+ alpha）：为 DeviceClass 提供扩展资源名称，允许 Pod 继续使用扩展资源请求来请求 DRA 设备。
- **可分区设备**（v1.33+ alpha）：设备不一定是连接到单台机器的单个单元，也可以是由多台机器连接的多个设备组成的逻辑设备，通过 CounterSets 管理资源消耗。
- **可消耗容量**（v1.34+ alpha）：同一设备可被多个独立的 ResourceClaim 消费，调度器管理每个声明消耗的设备容量。
- **设备污点和容忍度**（v1.33+ alpha）：类似于节点污点，可对单个设备设置污点，并通过 DeviceTaintRule API 由管理员设置。
- **设备绑定条件**（v1.34+ alpha）：允许调度器延迟 Pod 绑定，直到外部资源（如 fabric-attached GPU）准备就绪。

## 使用场景

- AI/ML 工作负载需要动态分配 GPU、TPU 等加速器。
- 多个 Pod 或容器需要共享同一个硬件设备。
- 需要基于设备属性（如型号、性能等级）进行细粒度设备选择。
- 网络设备、FPGA 等需要动态配置和准备的外部资源。

## 最佳实践/注意事项

- 避免使用 `spec.nodeName` 绕过调度器，因为这可能导致 Pod 在 ResourceClaim 未分配时阻塞节点上的正常资源。
- 管理员访问是特权模式，不应在多租户集群中授予普通用户。
- 使用设备污点和容忍度等 alpha 特性时，需要启用相应的特性门控和 API 版本。
- DRA 驱动必须正确实现 ResourceSlice 的创建和更新，以反映集群中资源容量的变化。

## 生产 YAML 示例

### DeviceClass + ResourceClaimTemplate + Pod（GPU 分配）

```yaml
# 1. 设备类定义
apiVersion: resource.k8s.io/v1
kind: DeviceClass
metadata:
  name: gpu-a100
spec:
  selectors:
    - cel:
        expression: "device.driver == 'gpu.nvidia.com' && device.attributes['model'] == 'A100'"
---
# 2. ResourceClaimTemplate（每 Pod 一个 claim）
apiVersion: resource.k8s.io/v1
kind: ResourceClaimTemplate
metadata:
  name: gpu-claim-template
  namespace: ml-platform
spec:
  spec:
    devices:
      requests:
        - name: gpu
          deviceClassName: gpu-a100
          count: 1
---
# 3. 使用 DRA 的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: inference-server
  namespace: ml-platform
spec:
  containers:
    - name: model-server
      image: registry.example.com/inference:v3.0
      resources:
        requests:
          cpu: "4"
          memory: 16Gi
      resourceClaims:
        - name: gpu-claim
  resourceClaims:
    - name: gpu-claim
      resourceClaimTemplateName: gpu-claim-template
  restartPolicy: Always
```

### ResourceClaim 共享（多容器共享同一 GPU）

```yaml
apiVersion: resource.k8s.io/v1
kind: ResourceClaim
metadata:
  name: shared-gpu
  namespace: ml-platform
spec:
  devices:
    requests:
      - name: gpu
        deviceClassName: gpu-a100
        count: 1
---
apiVersion: v1
kind: Pod
metadata:
  name: multi-container-gpu
  namespace: ml-platform
spec:
  containers:
    - name: trainer
      image: registry.example.com/trainer:v2.0
      resourceClaims:
        - name: shared
    - name: monitor
      image: registry.example.com/gpu-monitor:v1.0
      resourceClaims:
        - name: shared
  resourceClaims:
    - name: shared
      resourceClaimName: shared-gpu        # 两个容器共享同一个 claim
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pod Pending，提示 ResourceClaim 未分配 | DRA 驱动未安装或 ResourceSlice 缺失 | `kubectl get resourceslices` 确认驱动已注册设备 |
| DeviceClass 不匹配任何设备 | CEL 表达式属性名不正确 | `kubectl get resourceslices -o yaml` 检查设备属性名 |
| 使用 `spec.nodeName` 后 Pod 卡住 | 绕过调度器导致 ResourceClaim 未分配 | 改用 nodeSelector / nodeAffinity 而非 nodeName |
| ResourceClaim 分配后设备健康状态异常 | 设备硬件问题 | 检查 Pod 状态 `allocatedResourcesStatus` 字段 |
| 管理员访问请求被拒绝 | 未启用 DRAAdminAccess 特性门控 | 确认 apiserver 启用 `DRAAdminAccess` 特性门控 |

## 生产检查清单

- [ ] 安装对应硬件的 DRA 驱动（GPU / FPGA / 网络设备）
- [ ] 确认 DRA 驱动正确创建和更新 ResourceSlice
- [ ] 为常用设备创建 DeviceClass（如 gpu-a100、gpu-h100、fpga-xilinx）
- [ ] 使用 ResourceClaimTemplate 为每个 Pod 创建独立的 claim
- [ ] 多容器共享设备时使用具名 ResourceClaim
- [ ] 避免使用 `spec.nodeName` 绕过调度器
- [ ] 在多租户集群中限制管理员访问权限
- [ ] 监控设备健康状态（`allocatedResourcesStatus`）

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看集群中的 DeviceClass
kubectl get deviceclasses

# 查看 ResourceSlice（驱动注册的设备）
kubectl get resourceslices -o wide

# 查看 ResourceClaim 状态
kubectl get resourceclaims -n ml-platform

# 查看 ResourceClaim 详情（含分配信息）
kubectl describe resourceclaim shared-gpu -n ml-platform

# 查看 Pod 的设备分配状态
kubectl get pod inference-server -n ml-platform -o jsonpath='{.status.allocatedResourcesStatus}'

# 查看 ResourceClaimTemplate
kubectl get resourceclaimtemplates -n ml-platform
```
## 交叉引用

- [Kubernetes 调度器](./kubernetes-scheduler.md) — 调度器如何处理 ResourceClaim
- [调度框架](./scheduling-framework.md) — Reserve / PreBind 阶段与 DRA 的交互
- [[系统基础/topic-dictionary/scheduling/gang-scheduling.md|Gang Scheduling]]](./gang-scheduling.md) — 分布式 GPU 训练需要 DRA + gang 调度
- [[系统基础/topic-dictionary/scheduling/pod-overhead.md|Pod Overhead]]](./pod-overhead.md) — DRA 设备的额外资源开销
- Karpenter 自动扩缩容](./karpenter-autoscaling.md) — 为 DRA 设备需求自动扩展 GPU 节点

## 参考链接

- [Kubernetes 官方文档 - Dynamic Resource Allocation](https://kubernetes.io/docs/concepts/scheduling-eviction/dynamic-resource-allocation/)

## Related
- [[生态参考/topic-index/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]


<!-- risk-assessed -->
