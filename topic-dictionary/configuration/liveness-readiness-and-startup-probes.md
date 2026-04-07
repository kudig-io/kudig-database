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

## 生产 YAML 示例

### 完整的三探针配置（Java 微服务）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: order-service
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: order-service
  template:
    metadata:
      labels:
        app: order-service
    spec:
      containers:
        - name: app
          image: registry.example.com/order:v3.0
          ports:
            - containerPort: 8080
              name: http
            - containerPort: 8081
              name: management
          # Startup Probe — JVM 启动慢，最长等待 5 分钟
          startupProbe:
            httpGet:
              path: /actuator/health/liveness
              port: management
            initialDelaySeconds: 10
            periodSeconds: 5
            failureThreshold: 60           # 5s × 60 = 最多等待 300s
            timeoutSeconds: 3
          # Liveness Probe — 运行时健康检查，简单快速
          livenessProbe:
            httpGet:
              path: /actuator/health/liveness
              port: management
            periodSeconds: 10
            failureThreshold: 3
            timeoutSeconds: 3
          # Readiness Probe — 检查依赖就绪状态
          readinessProbe:
            httpGet:
              path: /actuator/health/readiness
              port: management
            periodSeconds: 5
            failureThreshold: 3
            successThreshold: 1
            timeoutSeconds: 3
          resources:
            requests:
              cpu: "500m"
              memory: 1Gi
            limits:
              cpu: "1"
              memory: 2Gi
```

### gRPC 健康检查探针

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: grpc-service
  namespace: production
spec:
  containers:
    - name: server
      image: registry.example.com/grpc-server:v2.0
      ports:
        - containerPort: 50051
      livenessProbe:
        grpc:
          port: 50051
          service: "myservice"             # 可选：gRPC 健康检查服务名
        periodSeconds: 10
        failureThreshold: 3
      readinessProbe:
        grpc:
          port: 50051
        periodSeconds: 5
      resources:
        requests:
          cpu: "250m"
          memory: 256Mi
```

### TCP + exec 探针（数据库 Sidecar）

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: db-proxy
  namespace: production
spec:
  containers:
    - name: proxy
      image: registry.example.com/pgbouncer:v1.20
      ports:
        - containerPort: 5432
      livenessProbe:
        tcpSocket:
          port: 5432
        periodSeconds: 10
      readinessProbe:
        exec:
          command:
            - /bin/sh
            - -c
            - "pg_isready -h 127.0.0.1 -p 5432"
        periodSeconds: 5
        timeoutSeconds: 2
      resources:
        requests:
          cpu: "100m"
          memory: 64Mi
```

## 探针类型决策参考

| 检查方式 | 适用场景 | 优点 | 注意事项 |
|----------|----------|------|----------|
| `httpGet` | Web 服务、REST API | 语义清晰，支持自定义 Header | 确保探针端点不触发写操作 |
| `tcpSocket` | 数据库代理、TCP 服务 | 简单轻量 | 只验证端口可连接，不验证应用逻辑 |
| `exec` | 自定义健康检查脚本 | 灵活度最高 | 进程创建有开销；确保命令快速返回 |
| `grpc` | gRPC 服务 | 原生协议支持 | 需要应用实现 gRPC Health Checking |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 容器反复重启（CrashLoopBackOff） | Liveness 探针在应用启动前就失败 | 添加 Startup Probe 或增大 `initialDelaySeconds` |
| Pod Running 但无流量 | Readiness 探针失败，Pod 被从 Endpoints 摘除 | `kubectl describe pod` 查看 Readiness 事件；检查探针端点 |
| 探针超时（Unhealthy: timeout） | `timeoutSeconds` 过短或应用响应慢 | 增大 `timeoutSeconds`；优化探针端点响应时间 |
| exec 探针失败但手动执行成功 | 容器内 PATH 或权限不同 | 使用绝对路径；确认脚本有执行权限 |
| 外部依赖故障导致 Liveness 失败引发重启雪崩 | Liveness 探针检查了外部依赖 | Liveness 只检查应用自身状态；外部依赖检查放在 Readiness |

## 生产检查清单

- [ ] 所有生产服务配置 Readiness Probe（流量控制）
- [ ] 启动慢的应用（JVM / Python ML 模型加载）配置 Startup Probe
- [ ] Liveness Probe 只检查应用自身状态，不依赖外部服务
- [ ] Readiness Probe 可检查关键依赖（数据库连接、缓存就绪）
- [ ] 探针端点不触发业务写操作或副作用
- [ ] `timeoutSeconds` 设置合理（建议 2-5 秒）
- [ ] `failureThreshold` 不要太小（建议 >= 3）避免抖动
- [ ] 探针使用独立的管理端口（如 8081）避免业务流量干扰

## 命令快速参考

```bash
# 查看 Pod 探针配置
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[0].livenessProbe}' | jq .

# 查看探针事件（成功/失败）
kubectl describe pod <pod-name> | grep -A 5 -E "(Liveness|Readiness|Startup)"

# 手动测试 HTTP 探针端点
kubectl exec <pod-name> -- curl -s http://localhost:8081/actuator/health/liveness

# 查看 Pod 的 Ready 条件
kubectl get pods -o custom-columns='NAME:.metadata.name,READY:.status.conditions[?(@.type=="Ready")].status'

# 查看非 Ready 的 Pod
kubectl get pods --field-selector=status.phase=Running -o json | jq '.items[] | select(.status.conditions[] | select(.type=="Ready" and .status=="False")) | .metadata.name'
```

## 交叉引用

- [ConfigMaps](./configmaps.md) — 探针端口/路径可通过 ConfigMap 配置化
- [Pod 和容器的资源管理](./resource-management-for-pods-and-containers.md) — 资源不足可导致探针超时
- [Secrets](./secrets.md) — 需要认证的探针端点可使用 Secret 中的凭据

## 参考链接

- [Kubernetes 官方文档 - Liveness, Readiness and Startup Probes](https://kubernetes.io/docs/concepts/configuration/liveness-readiness-startup-probes/)
