# Operator 模式

## 概述

Operator 是 Kubernetes 的软件扩展，它利用自定义资源（Custom Resources）来管理应用程序及其组件。Operator 遵循 Kubernetes 的设计原则，尤其是控制循环（Control Loop）模式。其核心目标是捕获人类运维专家管理服务的知识和行为，并通过代码实现自动化。

## 核心概念/原理

- **Operator 定义**：Operator 是 Kubernetes API 的客户端，充当自定义资源的控制器。它将一个或多个自定义资源与控制器关联起来，从而扩展集群的行为，而无需修改 Kubernetes 本身的代码。
- **控制循环**：Operator 持续观察自定义资源的实际状态，并通过调谐（reconcile）使其向期望状态靠拢。
- **声明式 API**：用户声明期望状态（如数据库副本数、版本），Operator 负责执行复杂的运维操作来实现该状态。

## 关键机制或特性

- **自定义资源 + 控制器**：Operator 通常由两部分组成：
  1. **CustomResourceDefinition（CRD）**：定义新的资源类型（如 `SampleDB`）。
  2. **控制器（Controller）**：运行在 Deployment 中的 Pod 内，持续监听 CR 的变化并执行相应的运维逻辑。
- **典型自动化能力**：
  - 按需部署应用
  - 执行应用状态的备份与恢复
  - 处理应用升级及关联变更（如数据库 schema 迁移、配置更新）
  - 为不支持 Kubernetes API 的应用发布 Service 以供发现
  - 模拟集群故障以测试弹性
  - 为分布式应用选举领导者
- **部署方式**：最常见的方式是将 CRD 和对应的控制器一起部署到集群中。控制器通常作为 Deployment 运行在控制平面之外。

## 使用场景

- 管理有状态应用（如数据库、消息队列、缓存集群），需要复杂的生命周期管理（部署、扩容、备份、恢复、升级）。
- 需要将人类运维专家的经验（如故障处理、配置优化）编码为自动化逻辑。
- 希望在 Kubernetes 中通过声明式方式管理第三方中间件或自定义应用。

## 最佳实践/注意事项

- 在生态中寻找已有的 Operator（如 OperatorHub.io），避免重复造轮子。
- 如果没有现成的 Operator，可以使用多种语言和框架自行开发，如 Go（kubebuilder、Operator SDK）、Python（Kopf）、Java（Java Operator SDK）、Rust（kube-rs）、.NET（KubeOps）等。
- Operator 控制器通常以 Deployment 形式运行，需要为其配置适当的 RBAC 权限。
- 设计 Operator 时，应充分考虑故障恢复、幂等性、升级兼容性以及与现有 Kubernetes 原生资源（如 StatefulSet、PersistentVolumeClaim）的协作。

## 参考链接

- https://kubernetes.io/docs/concepts/extend-kubernetes/operator/
