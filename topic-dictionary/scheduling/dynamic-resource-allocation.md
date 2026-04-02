# Dynamic Resource Allocation

## 概述

动态资源分配（Dynamic Resource Allocation，DRA）是 Kubernetes v1.35 中达到 stable 的特性。它允许用户在 Pod 之间请求和共享资源，这些资源通常是附加设备，如硬件加速器。DRA 提供了比 Device Plugin 更灵活的设备分类、请求和使用方式。

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

## 参考链接

- [Kubernetes 官方文档 - Dynamic Resource Allocation](https://kubernetes.io/docs/concepts/scheduling-eviction/dynamic-resource-allocation/)
