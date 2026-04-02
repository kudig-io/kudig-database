# Sidecar Containers

## 概述
Sidecar 容器是与主应用容器运行在同一 Pod 内的辅助容器，用于增强或扩展主应用功能，如日志收集、监控、安全代理或数据同步。

## 核心概念/原理
- **Kubernetes 实现方式**：自 v1.29（默认启用 `SidecarContainers` 特性门控）起，Sidecar 被实现为一种特殊的 init 容器——即在 `initContainers` 列表中声明，并设置 `restartPolicy: Always`。
- **生命周期**：Sidecar 容器在 Pod 启动时先于主容器启动，并在整个 Pod 生命周期内持续运行；Pod 终止时，Sidecar 在主容器完全停止后才接收终止信号。
- **独立性**：Sidecar 容器拥有独立的重启策略，可独立于主容器启动、停止或重启。

## 关键机制或特性
- **启动顺序**：Sidecar 作为 init 容器享有顺序保证，可与普通 init 容器混合编排复杂的初始化流程。某个 Sidecar 启动并处于 `started=true` 状态后，才会启动下一个 init 容器。
- **终止顺序**：Sidecar 按定义顺序的反向依次终止，确保辅助服务在主应用需要时始终可用。
- **探针支持**：与普通 init 容器不同，Sidecar 支持 `livenessProbe`、`readinessProbe` 和 `startupProbe`。
- **Job 兼容性**：在 Job 中使用 Kubernetes 原生 Sidecar 容器时，Sidecar 不会阻止 Job 在主容器完成后标记为完成。
- **资源共享**：Sidecar 与主容器共享网络和存储命名空间；Pod 的有效资源请求/限制为：Pod Overhead + max(非 init 容器之和, 有效 init 容器值)。

## 使用场景
- 日志/指标收集代理（如 Fluent Bit、Prometheus exporter）。
- 服务网格代理（如 Istio Envoy）。
- 配置重载或文件同步工具。
- 安全审计或身份验证代理。

## 最佳实践/注意事项
- 优先使用原生 Sidecar（`initContainers` + `restartPolicy: Always`）而不是普通多容器 Pod，以获得更好的生命周期控制。
- Sidecar 的优雅终止优先级较低；若主容器占用了全部优雅终止时间，Sidecar 可能收到 SIGTERM 后很快收到 SIGKILL，因此其非零退出码在 Pod 终止时是正常现象。
- 更改 Sidecar 镜像不会导致整个 Pod 重启，仅会触发该容器重启。
- 如无需控制启动/停止顺序，也可直接在 `containers` 字段中运行多容器。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/sidecar-containers/
