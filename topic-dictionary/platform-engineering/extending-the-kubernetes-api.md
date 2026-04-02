# 扩展 Kubernetes API

## 概述

Kubernetes API 是平台的核心，扩展 Kubernetes API 允许用户在不修改 Kubernetes 核心代码的情况下，为集群添加新的资源类型和功能。Kubernetes 提供了两种主要的 API 扩展方式：CustomResourceDefinitions（CRD）和 API Aggregation（AA）。

## 核心概念/原理

- **CustomResourceDefinition（CRD）**：以声明式方式定义新的自定义 API，包括 API 组（group）、类别（kind）和模式（schema）。Kubernetes 控制平面负责服务和存储这些自定义资源。使用 CRD 无需编写自定义 API server。
- **API Aggregation（聚合层）**：在主 API server 之后运行一个聚合层，作为主 API server 的代理。通过编写和部署自己的 API server，可以为自定义资源提供专门的实现，主 API server 将对应请求委托给扩展 API server。

## 关键机制或特性

- **CRD 的易用性**：无需编程，用户可用任何语言编写控制器；无需额外运行服务，由 API server 直接处理；升级和维护由 Kubernetes 主版本升级覆盖。
- **API Aggregation 的灵活性**：需要编程、构建二进制和镜像，并运行额外服务；但允许对 API 行为进行更精细的控制，如自定义存储层、自定义业务逻辑、任意验证、Protocol Buffers 支持等。
- **功能对比**：
  - 两者均支持：CRUD、Watch、Discovery、多版本、Scale/Status 子资源、HTTPS、内置认证授权、Finalizers、Admission Webhooks 等。
  - 仅 AA 支持：自定义存储、其他子资源（如 logs/exec）、strategic-merge-patch、Protocol Buffers。

## 使用场景

- **选择 CRD**：资源字段较少、仅在内部或小规模开源项目使用、对易用性要求高、不需要特殊存储或高级 API 行为时。
- **选择 API Aggregation**：需要自定义存储后端（如时序数据库）、需要复杂的验证或转换逻辑、需要提供非 CRUD 子资源、面向商业产品且需要最大灵活性时。

## 最佳实践/注意事项

- 不要将自定义资源用作应用程序数据、终端用户数据或监控数据的存储，这会导致与 Kubernetes API 过度耦合。云原生架构提倡组件间松耦合，常规操作所需的支撑服务应作为独立组件运行。
- 安装第三方 CRD 时，通常会同时部署实现业务逻辑的第三方控制器，带来新的故障点；安装聚合 API 则必然引入新的 Deployment。
- 自定义资源会占用 API server 的存储空间，创建过多可能压垮存储。
- 使用 RBAC 时，默认角色通常不会授予新资源的访问权限，需要显式配置权限。

## 参考链接

- https://kubernetes.io/docs/concepts/extend-kubernetes/api-extension/
