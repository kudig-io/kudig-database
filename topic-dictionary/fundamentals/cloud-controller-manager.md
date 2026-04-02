# Cloud Controller Manager（云控制器管理器）

## 概述

Cloud Controller Manager 是 Kubernetes 控制平面的一个组件，它将云厂商特定的控制逻辑嵌入到 Kubernetes 中。它使集群能够连接到云提供商的 API，并将与云平台交互的组件与仅与集群交互的组件分离开来，从而实现解耦，允许云厂商以不同于主 Kubernetes 项目的节奏发布新特性。

## 核心概念/原理

- **插件机制**：cloud-controller-manager 采用插件化架构，不同云厂商可以通过实现 `CloudProvider` 接口（定义在 `kubernetes/cloud-provider` 的 `cloud.go` 中）将自己的平台集成到 Kubernetes。
- **运行方式**：通常以 Pod 中的容器形式在控制平面中作为多副本进程运行。每个 cloud-controller-manager 在一个进程中实现了多个控制器。

## 关键机制或特性

cloud-controller-manager 内部包含以下主要控制器：

- **Node Controller**：
  - 当云基础设施中创建新服务器时，更新 Node 对象
  - 从云提供商获取租户内运行的主机信息
  - 在云环境中，若节点不健康，向云提供商查询该 VM 是否仍可用，若不可用则从节点列表中删除
- **Route Controller**：
  - 负责在云中配置路由，使集群中不同节点上的容器能够互相通信
  - 根据云提供商实现，可能还会为 Pod 网络分配 IP 地址块
- **Service Controller**：
  - 监听 Service 的创建、更新和删除事件
  - 与云提供商 API 交互，为需要外部负载均衡器等基础设施组件的 Service 配置相应的云资源（如托管负载均衡器、IP 地址、网络包过滤、目标健康检查）

### RBAC 权限要求

cloud-controller-manager 需要以下 API 对象访问权限（以 ClusterRole 形式示例）：

- **Nodes**：完全读写权限（`*`）
- **Nodes/Status**：`patch` 权限
- **Services**：`list`、`watch` 权限
- **Services/Status**：`patch`、`update` 权限
- **Events**：`create`、`patch`、`update` 权限
- **ServiceAccounts**：`create` 权限
- **PersistentVolumes**：`get`、`list`、`update`、`watch` 权限

## 使用场景

- 在公有云、私有云或混合云上运行 Kubernetes
- 将云特定的基础设施管理逻辑（如负载均衡器、路由、节点生命周期）从核心 Kubernetes 中解耦
- 云厂商希望独立于 Kubernetes 发布周期迭代新特性

## 最佳实践/注意事项

- 使用支持 cloud-controller-manager 的 Kubernetes 发行版或云厂商提供的实现
- 正确配置 RBAC，确保 cloud-controller-manager 拥有所需的最低权限
- 升级高可用控制平面以使用 cloud-controller-manager 时，参考官方迁移指南
- 如需实现自己的 cloud-controller-manager，需实现 `CloudProvider` 接口

## 参考链接

- https://kubernetes.io/docs/concepts/architecture/cloud-controller/
