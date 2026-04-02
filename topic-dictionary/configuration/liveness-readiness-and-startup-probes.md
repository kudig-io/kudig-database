# Liveness, Readiness, and Startup Probes

## 概述

Kubernetes 提供三种探针（Probe）来持续监控 Pod 中容器的健康状态。根据探针返回的结果，Kubernetes 可以决定是否需要重启不健康的容器，或者是否将流量路由到尚未就绪的容器。这三种探针分别是：Startup Probe（启动探针）、Liveness Probe（存活探针）和 Readiness Probe（就绪探针）。

## 核心概念/原理

### Startup Probe（启动探针）

- **作用**：验证容器内的应用是否已经完成启动。
- **执行时机**：仅在容器启动时执行，成功后才会开始执行 Liveness 和 Readiness 探针。
- **适用场景**：用于启动时间较长的应用，避免在应用尚未初始化完成时就被 Liveness 探针误判为不健康而重启。

### Liveness Probe（存活探针）

- **作用**：判断容器是否处于存活状态，决定是否需要重启容器。
- **执行时机**：在容器整个生命周期中周期性执行。
- **适用场景**：用于捕获应用死锁、无限循环或无法继续处理请求但进程仍在运行的情况。若探针多次失败，kubelet 将重启该容器。
- **注意事项**：Liveness 探针不会等待 Readiness 探针成功；如果需要延迟执行，可设置 `initialDelaySeconds` 或使用 Startup Probe。

### Readiness Probe（就绪探针）

- **作用**：判断容器是否已准备好接收流量。
- **执行时机**：在容器整个生命周期中周期性执行。
- **适用场景**：用于等待应用完成耗时初始化任务（如建立网络连接、加载配置文件、预热缓存），或在应用暂时过载、故障恢复时暂时将 Pod 从 Service 端点中移除。若探针失败，Kubernetes 会将该 Pod 从所有匹配的 Service 端点中摘除。

## 关键机制或特性

- **探针检查方式**：
  - `exec`：在容器内执行命令，根据退出码判断。
  - `httpGet`：发送 HTTP GET 请求，根据响应状态码判断（2xx-3xx 为成功）。
  - `tcpSocket`：尝试连接指定 TCP 端口，连接成功即为健康。
  - `grpc`：使用 gRPC 健康检查协议进行探测（需应用支持）。
- **配置参数**：
  - `initialDelaySeconds`：容器启动后首次探测前的等待时间。
  - `periodSeconds`：探测周期，默认为 10 秒。
  - `timeoutSeconds`：探测超时时间，默认为 1 秒。
  - `failureThreshold`：连续失败次数达到该阈值后才认为探针失败，默认为 3 次。
  - `successThreshold`：连续成功次数达到该阈值后才认为探针成功（仅 Readiness 和 Startup 支持，默认为 1）。

## 使用场景

- **Startup Probe**：微服务框架启动慢、JVM 应用预热、大数据任务初始化等需要较长启动时间的场景。
- **Liveness Probe**：检测应用内部死锁、内存泄漏导致的服务无响应、依赖服务永久性不可用导致应用挂起等。
- **Readiness Probe**：数据库连接池初始化、缓存预热、外部依赖临时不可用时的流量熔断、版本发布时的平滑过渡。

## 最佳实践/注意事项

- **合理设置初始延迟**：避免探针过早开始检测导致误杀，尤其是启动慢的应用应优先使用 Startup Probe。
- **Liveness 探针应简单快速**：Liveness 探针失败会导致容器重启，探测逻辑应尽量简单，避免因依赖外部服务故障而误重启。
- **Readiness 探针可检查依赖**：Readiness 探针可以检查应用依赖的外部服务状态，临时不可用时优雅地摘除流量而非重启容器。
- **避免探针重叠副作用**：确保探针请求不会影响业务数据或状态，例如避免 HTTP 探针触发写操作。
- **配置适当的超时和阈值**：根据应用实际响应时间调整 `timeoutSeconds` 和 `failureThreshold`，减少误报。
- **Startup + Liveness 组合**：对于启动慢的应用，使用 Startup Probe 保护启动过程，Liveness Probe 配置较短的周期用于运行时健康检查。

## 参考链接

- [Kubernetes 官方文档 - Liveness, Readiness and Startup Probes](https://kubernetes.io/docs/concepts/configuration/liveness-readiness-startup-probes/)
