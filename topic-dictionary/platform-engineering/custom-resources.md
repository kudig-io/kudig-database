# 自定义资源

## 概述

自定义资源（Custom Resources）是 Kubernetes API 的扩展，允许用户在不修改 Kubernetes 核心代码的情况下，为集群添加新的资源类型。自定义资源可以通过动态注册在运行的集群中出现或消失，安装后用户可以像操作内置资源（如 Pod）一样使用 `kubectl` 来创建和访问它们。

## 核心概念/原理

- **资源（Resource）**：Kubernetes API 中的一个端点，存储特定类型的 API 对象集合（例如内置的 pods 资源存储 Pod 对象集合）。
- **自定义资源（Custom Resource）**：默认 Kubernetes 安装中不一定可用的 API 扩展，代表对特定 Kubernetes 安装的定制。如今，许多核心 Kubernetes 功能也是基于自定义资源构建的，使 Kubernetes 更加模块化。
- **自定义控制器（Custom Controller）**：单独使用自定义资源只能存储和检索结构化数据；当与自定义控制器结合时，自定义资源提供了真正的声明式 API。控制器负责将当前状态与声明的期望状态保持同步。
- **Operator 模式**：将自定义资源与自定义控制器相结合，用于将特定应用的领域知识编码到 Kubernetes API 扩展中。

## 关键机制或特性

- **动态注册**：自定义资源可在集群运行时动态注册和更新，独立于集群生命周期。
- **声明式 API**：用户声明期望状态，控制器负责持续调谐（reconcile）。这与命令式 API（直接指示服务器执行操作）形成对比。
- **字段选择器（Field Selectors）**：自 Kubernetes v1.32 起稳定支持，允许客户端根据自定义资源字段的值进行筛选。除 `metadata.name` 和 `metadata.namespace` 外，还可在 CRD 的 `spec.versions[*].selectableFields` 中声明其他可选字段。
- **两种实现方式**：
  - **CRD**：无需编程，由 API server 直接服务和存储。
  - **API Aggregation**：需要编写自定义 API server，提供更大的灵活性。

## 使用场景

- 需要为集群引入新的抽象层来管理应用或基础设施配置。
- 希望通过 `kubectl` 读写新类型资源，并在 Kubernetes UI 中展示。
- 需要构建自动化工具，监听新资源的更新并相应地创建、修改其他资源。
- 希望使用 Kubernetes API 约定（如 `.spec`、`.status`、`.metadata`）来封装一组受控资源。

## 最佳实践/注意事项

- **CRD vs. ConfigMap**：如果配置已有明确的文件格式（如 `mysql.cnf`）、主要用于 Pod 内程序消费、希望通过 Deployment 滚动更新文件时，优先使用 ConfigMap（敏感数据用 Secret）。如果需要 `kubectl` 顶级支持、构建自动化、使用 Kubernetes API 约定，则优先使用自定义资源。
- **是否聚合 API 的决策**：如果 API 是声明式的、需要 `kubectl` 和 UI 支持、资源自然按集群或命名空间范围划分，则考虑聚合到 Kubernetes API；否则可保持为独立 API。
- **避免数据存储滥用**：不要将自定义资源用作应用数据、终端用户数据或监控数据的存储，这会导致与 Kubernetes 过度耦合。
- **存储影响**：自定义资源占用 API server 存储空间，创建过多会压垮存储。资源按当前存储版本存入 etcd，更新时会使用定义的存储版本。
- **RBAC 授权**：新资源默认不会被现有 RBAC 角色授予访问权限（除 cluster-admin 或通配符规则外），需显式授权。

## 参考链接

- https://kubernetes.io/docs/concepts/extend-kubernetes/api-extension/custom-resources/
