# Kubernetes API 聚合层

## 概述

API 聚合层（Aggregation Layer）允许 Kubernetes 通过额外的 API 进行扩展，这些 API 超出了核心 Kubernetes API 所提供的范围。无论是现成的解决方案（如 metrics server），还是用户自行开发的 API，都可以通过聚合层无缝集成到 Kubernetes API 中。

## 核心概念/原理

- **聚合层与 CRD 的区别**：
  - **CRD** 是让 kube-apiserver 识别新类型对象的一种方式。
  - **聚合层** 则是在 kube-apiserver 进程内运行，充当代理，将特定 API 路径的请求转发到后端的扩展 API server。
- **APIService 对象**：要注册一个扩展 API，需要创建一个 `APIService` 对象来“声明” Kubernetes API 中的 URL 路径（例如 `/apis/myextension.mycompany.io/v1/…`）。注册后，聚合层会将发往该路径的所有请求代理到对应的 APIService。
- **扩展 API server**：最常见的实现方式是在集群的 Pod 中运行一个扩展 API server。如果该扩展 API server 用于管理集群中的资源，通常还会配套一个或多个控制器。`apiserver-builder` 库提供了扩展 API server 及其控制器的脚手架代码。

## 关键机制或特性

- **代理机制**：聚合层与 kube-apiserver 在同一进程内运行。未注册扩展资源时，聚合层不执行任何操作；注册后，相关路径的请求会被透明代理到扩展 API server。
- **低延迟要求**：扩展 API server 与 kube-apiserver 之间的网络延迟必须很低。Discovery 请求要求在 5 秒内完成往返。如果无法满足，需要优化网络或架构。
- **对用户透明**：通过聚合层扩展的 API 对用户来说与原生 Kubernetes API 无异，可直接使用 `kubectl`、客户端库等工具访问。

## 使用场景

- 需要为集群添加自定义 API，且该 API 需要特殊的存储后端或复杂的业务逻辑，CRD 无法满足时。
- 需要集成现成的 Kubernetes 生态组件（如 metrics-server）以扩展 API 能力。
- 希望复用 Kubernetes 的认证、授权、审计等基础设施，同时提供自定义 API 行为时。

## 最佳实践/注意事项

- 确保扩展 API server 与 kube-apiserver 之间的网络连接低延迟且高可用，否则会影响 API 发现和响应性能。
- 使用 `apiserver-builder` 等工具可以加速扩展 API server 和控制器的开发。
- 聚合层需要在环境中进行正确配置（如 CA 证书、代理设置）才能正常工作。
- 如果扩展 API 的需求较简单，优先考虑使用 CRD，以降低运维复杂度；仅在需要高级特性时才选择聚合层。

## 参考链接

- https://kubernetes.io/docs/concepts/extend-kubernetes/api-extension/apiserver-aggregation/
