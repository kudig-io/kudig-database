# Pods

## 概述
Pod 是 Kubernetes 中最小的可部署计算单元，它是一组共享存储和网络资源、并协同运行的一个或多个容器的集合。Pod 中的容器始终被共位（co-located）和共调度（co-scheduled），在共享上下文中运行，相当于一个应用专属的“逻辑主机”。

## 核心概念/原理
- **单容器 Pod**：最常见的使用方式，Kubernetes 直接管理 Pod，而不是直接管理容器。
- **多容器 Pod**：当多个容器需要紧密耦合、共享资源时，可放入同一个 Pod。例如主应用容器 + Sidecar 容器。
- **共享资源**：Pod 内的容器共享网络命名空间（IP 地址、端口空间）和存储卷（Volumes），可通过 `localhost` 互相通信。
- **Pod 模板（PodTemplate）**：工作负载控制器（如 Deployment、Job）通过 Pod 模板来创建和管理 Pod。
- **Static Pods**：由 kubelet 直接管理，不经过 API Server，常用于自托管控制平面。

## 关键机制或特性
- **Pod OS**：通过 `.spec.os.name` 指定 `linux` 或 `windows`。
- **Pod 更新与替换**：直接修改运行中 Pod 的字段有限制；工作负载控制器通常通过创建新 Pod 来应用模板更新。
- **Pod 子资源**：包括 `resize`（调整资源）、`ephemeralContainers`（临时容器）、`status`、`binding`。
- **Pod generation**：`metadata.generation` 在 spec 变更时递增，`status.observedGeneration` 用于跟踪状态同步。
- **容器探针（Probes）**：支持 `livenessProbe`、`readinessProbe`、`startupProbe`，由 kubelet 定期执行诊断。

## 使用场景
- 运行单一应用容器（最常见）。
- 需要多个紧密耦合容器协同工作的场景（如 Web 服务器 + 日志收集 Sidecar）。
- 需要共享网络或存储卷的微服务组件。

## 最佳实践/注意事项
- 通常情况下不要直接创建 Pod，而是通过 Deployment、StatefulSet、Job 等工作负载资源来管理。
- Pod 名称需符合 DNS 子域名规范，建议遵循更严格的 DNS Label 规则。
- 需要横向扩展时，应使用多个 Pod 副本，而不是在同一个 Pod 内运行多个相同容器。
- 注意 CPU limit 的权衡：可防止 noisy neighbor，但也可能在节点有空闲 CPU 时导致限流。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/
