# Workload Management

## 概述
Kubernetes 提供多个内置 API 用于声明式地管理工作负载及其组件。虽然应用最终运行在 Pod 中，但直接管理单个 Pod 非常繁琐。工作负载对象提供了更高层次的抽象，控制平面会根据定义自动管理 Pod 的生命周期。

## 核心概念/原理
主要的工作负载管理 API 包括：
- **Deployment（及间接的 ReplicaSet）**：管理无状态应用的最常用方式。Pod 之间可互换，任意 Pod 失败均可被替换。
- **StatefulSet**：用于管理需要稳定、唯一网络标识和持久存储的有状态应用。每个 Pod 具有持久标识，并与 PersistentVolume 关联。
- **DaemonSet**：确保所有（或部分）节点上运行一个 Pod 副本。常用于节点级服务，如日志收集、监控代理、网络插件等。
- **Job / CronJob**：用于运行一次性或定时批处理任务。Job 运行到完成即停止，CronJob 按时间表重复创建 Job。
- **ReplicationController**：旧版 API，用于维护指定数量的 Pod 副本，现已被 Deployment 和 ReplicaSet 取代。

## 关键机制或特性
- **声明式管理**：用户描述期望状态，控制器负责将实际状态收敛到期望状态。
- **自动恢复**：当 Pod 因节点故障、维护或配置错误而终止时，控制器会自动创建替代 Pod。
- **滚动更新**：Deployment 和 StatefulSet 支持滚动更新策略，可在不中断服务的情况下更新应用。
- **扩缩容**：大多数工作负载支持手动或自动的水平扩缩容。

## 使用场景
- **Deployment**：Web 前端、API 服务、无状态微服务。
- **StatefulSet**：数据库（MySQL、PostgreSQL、MongoDB）、消息队列（Kafka、RabbitMQ）、分布式存储（ZooKeeper、etcd）。
- **DaemonSet**：节点监控（Prometheus Node Exporter）、日志收集（Fluentd/Fluent Bit）、CNI 插件、存储驱动。
- **Job/CronJob**：数据备份、报表生成、定时清理任务、批处理计算。

## 最佳实践/注意事项
- 优先使用 Deployment 管理无状态应用，而不是直接使用 ReplicaSet 或 ReplicationController。
- StatefulSet 需要配合 Headless Service 使用，以提供稳定的网络标识。
- DaemonSet Pod 通常需要较高的优先级，以确保在节点上优先调度。
- 对于预期会自行终止的任务，使用 Job 而非 ReplicaSet/Deployment。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/controllers/
