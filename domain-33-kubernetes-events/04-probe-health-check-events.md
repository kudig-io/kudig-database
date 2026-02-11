# 04 - 探针与健康检查事件

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

---

## 概述

探针（Probe）与健康检查是 Kubernetes 容器生命周期管理的核心机制，用于判断容器的健康状态并采取相应的自愈措施。本文档详细记录所有与探针健康检查相关的事件、配置参数、最佳实践和常见问题。

### 探针类型对比

| 探针类型 | 引入版本 | 失败行为 | 主要用途 | 生产频率 |
|:---|:---|:---|:---|:---|
| **livenessProbe** | v1.0+ | 重启容器 | 检测死锁、无响应进程，确保容器可以自愈 | 高频 |
| **readinessProbe** | v1.0+ | 从 Service 端点移除 | 控制流量路由，只有就绪的 Pod 才接收流量 | 高频 |
| **startupProbe** | v1.16+ (Beta)<br>v1.20+ (GA) | 阻塞其他探针，失败则重启 | 保护启动缓慢的容器，给予足够的启动时间 | 中频 |

### 探针机制对比

| 机制类型 | 引入版本 | 检测方式 | 适用场景 | 性能开销 |
|:---|:---|:---|:---|:---|
| **httpGet** | v1.0+ | HTTP GET 请求，检查状态码 2xx/3xx | Web 服务、REST API | 低 |
| **tcpSocket** | v1.0+ | TCP 连接测试 | 数据库、缓存、非 HTTP 服务 | 极低 |
| **exec** | v1.0+ | 容器内执行命令，检查退出码 | 自定义复杂健康检查逻辑 | 中高 |
| **grpc** | v1.24+ (Alpha)<br>v1.27+ (GA) | gRPC 健康检查协议 | gRPC 微服务 | 低 |

---

## 探针配置参数

### 核心参数详解

| 参数名称 | 引入版本 | 默认值 | 说明 | 生产建议 |
|:---|:---|:---|:---|:---|
| **initialDelaySeconds** | v1.0+ | 0 | 容器启动后多久开始探测 | 根据应用启动时间设置，通常 10-30s |
| **periodSeconds** | v1.0+ | 10 | 探测频率（秒） | 关键服务 5-10s，一般服务 10-30s |
| **timeoutSeconds** | v1.0+ | 1 | 探测超时时间（秒） | 根据响应时间设置，通常 1-3s |
| **successThreshold** | v1.0+ | 1 | 从失败到成功需要的连续成功次数 | Liveness/Startup 必须为 1，Readiness 可设置 1-3 |
| **failureThreshold** | v1.0+ | 3 | 从成功到失败需要的连续失败次数 | 根据容忍度设置，通常 3-5 次 |
| **terminationGracePeriodSeconds** | v1.25+ | 继承 Pod 设置 | 探测失败后容器终止的宽限期 | 根据应用关闭时间设置，通常 30-60s |

### 关键计算公式

```
最大启动时间 = initialDelaySeconds + (failureThreshold × periodSeconds)

示例：initialDelaySeconds=10, failureThreshold=3, periodSeconds=10
最大启动时间 = 10 + (3 × 10) = 40 秒
```

---

## 探针事件详解

### `Unhealthy` (Liveness Probe Failed) - 存活探针失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 |

#### 事件含义

Liveness Probe（存活探针）失败表明容器内的主进程虽然在运行，但已经进入不健康状态（如死锁、资源耗尽、无法处理请求等）。kubelet 会在连续失败达到 `failureThreshold` 次数后重启容器，这是 Kubernetes 的自愈机制核心。

**与 Readiness Probe 的关键区别**：
- Liveness 失败 → 重启容器（更激进的恢复措施）
- Readiness 失败 → 仅移除流量（保守的隔离措施）

#### 典型事件消息

**HTTP 探针失败**：
```bash
$ kubectl describe pod myapp-76d8f9c5d-xk2lp

Events:
  Type     Reason     Age                From               Message
  ----     ------     ----               ----               -------
  Warning  Unhealthy  2m (x5 over 3m)    kubelet            Liveness probe failed: HTTP probe failed with statuscode: 503
  Warning  Unhealthy  1m (x3 over 2m)    kubelet            Liveness probe failed: Get "http://10.244.1.5:8080/healthz": dial tcp 10.244.1.5:8080: connect: connection refused
  Normal   Killing    30s                kubelet            Container myapp failed liveness probe, will be restarted
```

**TCP 探针失败**：
```bash
Warning  Unhealthy  1m    kubelet  Liveness probe failed: dial tcp 10.244.1.5:6379: connect: connection refused
```

**Exec 探针失败**：
```bash
Warning  Unhealthy  45s   kubelet  Liveness probe failed: command "/bin/check-health.sh" exited with code 1
Warning  Unhealthy  30s   kubelet  Liveness probe failed: OCI runtime exec failed: exec failed: container_linux.go:380: starting container process caused: exec: "/healthcheck": stat /healthcheck: no such file or directory
```

**gRPC 探针失败（v1.27+）**：
```bash
Warning  Unhealthy  1m    kubelet  Liveness probe failed: gRPC probe failed: rpc error: code = Unavailable desc = connection error
```

#### 影响面说明

- **用户影响**: 
  - 容器重启期间（通常 5-30 秒）服务完全不可用
  - 如果配合 Readiness Probe，用户请求会被路由到其他健康 Pod
  - 频繁重启会导致用户体验下降，出现间歇性错误
  
- **服务影响**: 
  - 容器重启导致进程状态丢失（内存缓存、连接池等）
  - 重启次数累计可能触发 CrashLoopBackOff（退避重启）
  - 如果是单副本 Pod，会造成服务完全中断
  
- **集群影响**: 
  - 频繁的容器重启增加 kubelet 和容器运行时（containerd/CRI-O）的负载
  - 镜像拉取可能增加网络和镜像仓库压力
  - 大量 Pod 同时重启可能触发集群级联故障
  
- **关联事件链**: 
  ```
  Unhealthy (Liveness failed) 
    → Killing (Container killed)
    → BackOff (CrashLoopBackOff if repeated)
    → Pulled (Pull new image if imagePullPolicy=Always)
    → Created (New container created)
    → Started (New container started)
    → Unhealthy (Readiness probe may still fail)
  ```

#### 排查建议

**1. 查看探针配置**
```bash
# 查看 Pod 的探针配置
kubectl get pod myapp-76d8f9c5d-xk2lp -o jsonpath='{.spec.containers[*].livenessProbe}' | jq

# 查看完整的容器配置
kubectl get pod myapp-76d8f9c5d-xk2lp -o yaml | grep -A 15 livenessProbe
```

**2. 检查探针端点/命令**
```bash
# 对于 HTTP 探针，手动测试端点
kubectl exec myapp-76d8f9c5d-xk2lp -- wget -O- http://localhost:8080/healthz
kubectl exec myapp-76d8f9c5d-xk2lp -- curl -v http://localhost:8080/healthz

# 对于 TCP 探针，测试端口连接
kubectl exec myapp-76d8f9c5d-xk2lp -- nc -zv localhost 6379

# 对于 Exec 探针，手动执行命令
kubectl exec myapp-76d8f9c5d-xk2lp -- /bin/check-health.sh
```

**3. 查看应用日志**
```bash
# 查看当前容器日志
kubectl logs myapp-76d8f9c5d-xk2lp

# 查看上一个容器日志（如果已重启）
kubectl logs myapp-76d8f9c5d-xk2lp --previous

# 实时跟踪日志
kubectl logs -f myapp-76d8f9c5d-xk2lp
```

**4. 检查资源使用**
```bash
# 查看容器资源使用情况
kubectl top pod myapp-76d8f9c5d-xk2lp

# 查看资源限制配置
kubectl get pod myapp-76d8f9c5d-xk2lp -o jsonpath='{.spec.containers[*].resources}'
```

**5. 分析重启历史**
```bash
# 查看 Pod 重启次数和状态
kubectl get pod myapp-76d8f9c5d-xk2lp -o wide

# 查看详细事件历史
kubectl describe pod myapp-76d8f9c5d-xk2lp | grep -A 50 Events

# 查看所有容器的重启情况
kubectl get pods -o custom-columns='NAME:.metadata.name,RESTARTS:.status.containerStatuses[*].restartCount'
```

#### 解决建议

| 常见原因 | 症状特征 | 解决方案 | 预防措施 |
|:---|:---|:---|:---|
| **initialDelaySeconds 过短** | 容器启动时立即失败 | 增加 `initialDelaySeconds` 至应用完全启动的时间 | 测量实际启动时间，设置为 1.5 倍 |
| **timeoutSeconds 过短** | 偶发性失败，高负载时更频繁 | 增加 `timeoutSeconds` 至 3-5 秒 | 监控探针响应时间，设置 P95 + 缓冲 |
| **failureThreshold 过低** | 网络抖动导致频繁重启 | 增加 `failureThreshold` 至 5-10 | 根据业务容忍度调整 |
| **应用真实死锁/挂起** | 日志停止输出，CPU 使用异常 | 修复应用逻辑，添加超时机制 | 代码审查、压测、Chaos Engineering |
| **依赖服务不可用** | 健康检查依赖外部服务（数据库、Redis） | **反模式**：Liveness 不应检查依赖，移至 Readiness | Liveness 只检查自身进程，Readiness 检查依赖 |
| **探针端点资源消耗高** | 探针触发重负载操作（数据库查询） | 优化健康检查端点，使用轻量级检查 | 健康检查应该 < 100ms，无副作用 |
| **内存/CPU 不足** | OOMKilled 或 CPU 节流 | 增加资源 limits/requests | 根据监控数据调整资源配额 |
| **探针路径/端口错误** | 404 Not Found, Connection Refused | 修正 `httpGet.path`、`httpGet.port` 配置 | 使用 CI 测试验证探针配置 |
| **证书/权限问题** | 403 Forbidden, TLS handshake failed | 配置正确的 `httpHeaders`、`scheme: HTTPS` | 使用专门的健康检查端点，避免认证 |

---

### `Unhealthy` (Readiness Probe Failed) - 就绪探针失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 高频 |

#### 事件含义

Readiness Probe（就绪探针）失败表明容器虽然在运行，但暂时无法处理请求（如正在初始化、依赖服务不可用、过载等）。kubelet 会将此 Pod 从 Service 的 Endpoints 中移除，阻止新流量路由到此 Pod，但**不会重启容器**。

**核心设计理念**：
- **优雅降级**：临时故障时保护流量，而非激进地重启
- **依赖检查**：适合检查外部依赖（数据库、缓存、消息队列）
- **流量控制**：滚动更新时控制新 Pod 何时接收流量

#### 典型事件消息

```bash
$ kubectl describe pod backend-api-7d9c8f-zx4mp

Events:
  Type     Reason     Age                  From               Message
  ----     ------     ----                 ----               -------
  Warning  Unhealthy  3m (x15 over 10m)    kubelet            Readiness probe failed: HTTP probe failed with statuscode: 500
  Warning  Unhealthy  2m (x8 over 5m)      kubelet            Readiness probe failed: Get "http://10.244.2.10:8080/ready": context deadline exceeded (Client.Timeout exceeded while awaiting headers)
  Warning  Unhealthy  1m                   kubelet            Readiness probe failed: {"status":"unhealthy","dependencies":{"database":"disconnected","redis":"timeout"}}
```

**Exec 探针示例**：
```bash
Warning  Unhealthy  45s   kubelet  Readiness probe failed: command "/app/readiness-check" exited with code 1: output: "Database connection pool exhausted"
```

#### 影响面说明

- **用户影响**: 
  - 如果所有副本都 Unready，Service 无可用端点，用户收到 503 Service Unavailable
  - 如果部分副本 Unready，用户无影响（流量自动路由到健康副本）
  - Ingress/LoadBalancer 会移除不健康的后端
  
- **服务影响**: 
  - Pod 仍在运行，但被隔离不接收流量
  - 如果是临时故障（依赖恢复），Pod 可自动恢复 Ready
  - 长期 Unready 可能需要人工介入（与 Liveness 配合使用）
  
- **集群影响**: 
  - Endpoints Controller 更新 Endpoints 对象，触发 kube-proxy 更新 iptables/IPVS 规则
  - 大量 Pod 频繁切换 Ready 状态会增加控制平面负载
  
- **关联事件链**: 
  ```
  Unhealthy (Readiness failed)
    → Endpoints 更新（Pod IP 从 Endpoints.subsets[].addresses 移至 notReadyAddresses）
    → kube-proxy 更新转发规则
    → 流量停止路由到此 Pod
    → (如果依赖恢复) Readiness 成功
    → Endpoints 更新（Pod IP 重新加入 addresses）
    → 流量恢复
  ```

#### 排查建议

**1. 检查 Service Endpoints**
```bash
# 查看 Service 的后端 Pod 列表
kubectl get endpoints myservice

# 详细查看 Endpoints（区分 ready 和 notReady）
kubectl get endpoints myservice -o yaml

# 查看 Service 的选择器和匹配的 Pod
kubectl describe service myservice
kubectl get pods -l app=myapp --show-labels
```

**2. 对比 Liveness 和 Readiness**
```bash
# 查看 Pod 的探针状态
kubectl get pod backend-api-7d9c8f-zx4mp -o jsonpath='{.status.conditions[?(@.type=="Ready")]}' | jq
kubectl get pod backend-api-7d9c8f-zx4mp -o jsonpath='{.status.containerStatuses[*].ready}'

# 同时查看 Liveness 和 Readiness 配置
kubectl get pod backend-api-7d9c8f-zx4mp -o yaml | grep -A 10 -E '(liveness|readiness)Probe'
```

**3. 测试依赖连接**
```bash
# 进入容器测试数据库连接
kubectl exec backend-api-7d9c8f-zx4mp -- nc -zv mysql-service 3306

# 测试 Redis 连接
kubectl exec backend-api-7d9c8f-zx4mp -- redis-cli -h redis-service ping

# 测试 HTTP 依赖服务
kubectl exec backend-api-7d9c8f-zx4mp -- curl -v http://auth-service/health
```

**4. 分析探针响应内容**
```bash
# 手动执行 HTTP Readiness 探针
kubectl exec backend-api-7d9c8f-zx4mp -- curl -i http://localhost:8080/ready

# 查看探针返回的详细信息
kubectl exec backend-api-7d9c8f-zx4mp -- wget -O- -S http://localhost:8080/ready
```

#### 解决建议

| 常见原因 | 症状特征 | 解决方案 | 最佳实践 |
|:---|:---|:---|:---|
| **依赖服务不可用** | 日志显示数据库/缓存连接失败 | 修复依赖服务，或调整探针逻辑支持降级 | Readiness 应检查关键依赖，非关键依赖失败可返回降级状态 |
| **应用初始化未完成** | 启动后立即失败，随后恢复 | 增加 `initialDelaySeconds` | 对于需要预热的应用（如 JVM），设置足够的延迟 |
| **流量过载** | 高峰期失败，低峰期正常 | 增加副本数（HPA），优化应用性能 | 设置 `successThreshold=2` 避免抖动 |
| **探针与 Liveness 相同** | 依赖故障导致容器重启 | **反模式**：分离关注点，Liveness 检查进程，Readiness 检查依赖 | 永远不要让 Readiness 和 Liveness 完全相同 |
| **网络分区/DNS 问题** | 偶发性连接超时 | 检查网络策略、DNS 解析，增加 `timeoutSeconds` | 使用 Service DNS 而非 IP 地址 |
| **滚动更新期间** | 新 Pod 长时间 Unready | 检查新版本代码、配置、依赖版本 | 使用 `minReadySeconds` 确保稳定性 |
| **资源配额不足** | CPU 节流导致响应慢 | 增加 CPU requests/limits | 监控 throttling metrics |

---

### `Unhealthy` (Startup Probe Failed) - 启动探针失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.16+ (Beta), v1.20+ (GA) |
| **生产频率** | 中频 |

#### 事件含义

Startup Probe（启动探针）是 v1.20 正式引入的 GA 功能，专门用于保护**启动缓慢的容器**。在 Startup Probe 成功之前，Liveness 和 Readiness Probe 会被禁用，避免容器在启动阶段被过早杀死。

**设计背景**：
- 传统应用（如 Java Spring Boot、大型 Node.js 应用）启动可能需要 1-5 分钟
- 如果 `initialDelaySeconds` 设置过大，会延迟故障检测
- Startup Probe 允许给予足够的启动时间，同时在运行后快速检测故障

**工作机制**：
```
容器启动 
  → Startup Probe 开始检测
  → Liveness/Readiness Probe 被阻塞
  → Startup Probe 成功
  → Liveness/Readiness Probe 开始工作
```

#### 典型事件消息

```bash
$ kubectl describe pod java-app-5d8c9f-kl3mp

Events:
  Type     Reason     Age                From               Message
  ----     ------     ----               ----               -------
  Warning  Unhealthy  5m (x30 over 10m)  kubelet            Startup probe failed: HTTP probe failed with statuscode: 503
  Warning  Unhealthy  4m (x20 over 9m)   kubelet            Startup probe failed: Get "http://10.244.1.15:8080/startup": dial tcp 10.244.1.15:8080: connect: connection refused
  Normal   Killing    3m                 kubelet            Container java-app failed startup probe, will be restarted
```

**与 Liveness 失败的区别**：
```bash
# Startup 失败
Warning  Unhealthy  2m   kubelet  Startup probe failed: ...
Normal   Killing    1m   kubelet  Container app failed startup probe, will be restarted

# Liveness 失败
Warning  Unhealthy  2m   kubelet  Liveness probe failed: ...
Normal   Killing    1m   kubelet  Container app failed liveness probe, will be restarted
```

#### 影响面说明

- **用户影响**: 
  - 启动阶段失败，Pod 进入 CrashLoopBackOff，服务无法启动
  - 如果是滚动更新，旧版本继续服务，新版本启动失败（保护机制）
  
- **服务影响**: 
  - 启动时间过长可能触发超时（默认最大等待时间可配置）
  - 频繁失败会导致退避重启，延长服务恢复时间
  
- **集群影响**: 
  - 大量启动失败会占用资源配额但不提供服务
  - 影响调度器和 kubelet 性能
  
- **关联事件链**: 
  ```
  Created → Started
    → Startup Probe 开始
    → Unhealthy (Startup failed) × N
    → (failureThreshold × periodSeconds 时间后)
    → Killing (Container killed)
    → BackOff (等待重启)
    → Pulled (拉取镜像)
    → Created → Started (重新启动)
  ```

#### 排查建议

**1. 检查启动时间预算**
```bash
# 查看 Startup Probe 配置
kubectl get pod java-app-5d8c9f-kl3mp -o jsonpath='{.spec.containers[*].startupProbe}' | jq

# 计算最大允许启动时间
# 公式：initialDelaySeconds + (failureThreshold × periodSeconds)
# 示例：0 + (30 × 10) = 300 秒（5 分钟）
```

**2. 手动测试启动过程**
```bash
# 查看容器启动日志
kubectl logs java-app-5d8c9f-kl3mp

# 实时监控启动过程
kubectl logs -f java-app-5d8c9f-kl3mp --timestamps

# 测试启动端点
kubectl exec java-app-5d8c9f-kl3mp -- curl http://localhost:8080/startup
```

**3. 对比配置**
```bash
# 查看所有探针配置
kubectl get pod java-app-5d8c9f-kl3mp -o yaml | yq '.spec.containers[0] | {startupProbe, livenessProbe, readinessProbe}'
```

**4. 检查资源限制**
```bash
# 资源不足会延长启动时间
kubectl describe pod java-app-5d8c9f-kl3mp | grep -A 5 "Limits\|Requests"

# 查看实际资源使用
kubectl top pod java-app-5d8c9f-kl3mp
```

#### 解决建议

| 常见原因 | 症状特征 | 解决方案 | 配置示例 |
|:---|:---|:---|:---|
| **启动时间超过配置预算** | 日志显示应用正常初始化，但探针已失败 | 增加 `failureThreshold` 或 `periodSeconds` | `failureThreshold: 30`<br>`periodSeconds: 10` → 5 分钟 |
| **应用启动真实失败** | 日志有报错，依赖连接失败 | 修复应用配置、依赖服务、资源配额 | 检查日志根因 |
| **探针端点启动晚于应用** | 应用已启动但 HTTP Server 未初始化 | 调整应用架构，优先启动健康检查端点 | 使用轻量级 HTTP Server |
| **没有配置 Startup Probe** | 慢启动应用频繁被 Liveness 杀死 | **添加 Startup Probe** | 见下方最佳实践 |
| **Startup 和 Liveness 冲突** | 启动成功后仍然失败 | 确保 Startup 成功后 Liveness 能通过 | 使用相同端点但不同阈值 |

**配置示例（Java Spring Boot 应用）**：
```yaml
startupProbe:
  httpGet:
    path: /actuator/health/liveness
    port: 8080
  initialDelaySeconds: 0
  periodSeconds: 10
  failureThreshold: 30  # 最多等待 300 秒（5 分钟）
  
livenessProbe:
  httpGet:
    path: /actuator/health/liveness
    port: 8080
  periodSeconds: 10
  failureThreshold: 3   # 启动后，30 秒内失败则重启
  
readinessProbe:
  httpGet:
    path: /actuator/health/readiness
    port: 8080
  periodSeconds: 5
  failureThreshold: 3
```

---

### `ProbeWarning` - 探针警告

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.21+ |
| **生产频率** | 低频 |

#### 事件含义

`ProbeWarning` 是 v1.21 引入的新事件类型，用于记录**探针执行成功但有异常输出**的情况。这允许应用在健康检查通过的同时，向 Kubernetes 报告潜在问题或降级状态。

**典型场景**：
- 应用主功能正常，但部分非关键功能降级
- 依赖服务部分不可用，但应用可继续运行
- 资源使用接近阈值（如内存 80%）
- 性能指标异常但未达到失败条件

#### 典型事件消息

```bash
$ kubectl describe pod api-server-6d7c8f-pm9kl

Events:
  Type     Reason        Age                From               Message
  ----     ------        ----               ----               -------
  Warning  ProbeWarning  5m (x10 over 15m)  kubelet            Readiness probe succeeded with warnings: {"status":"degraded","cache":"disconnected","primary_db":"healthy"}
  Warning  ProbeWarning  3m                 kubelet            Liveness probe succeeded with warnings: memory usage at 85%, approaching limit
```

#### 影响面说明

- **用户影响**: 
  - Pod 仍然正常服务（探针成功）
  - 但可能存在性能下降或部分功能不可用
  
- **服务影响**: 
  - 服务处于降级状态，需要关注但无需立即干预
  - 可能预示即将发生的故障
  
- **集群影响**: 
  - 作为早期预警信号，可触发告警和自动扩容
  
- **关联事件链**: 
  ```
  ProbeWarning (降级警告)
    → (如果情况恶化) Unhealthy (探针失败)
    → (根据探针类型) 重启或移除流量
  ```

#### 排查建议

**1. 查看警告详情**
```bash
# 查看完整事件消息
kubectl describe pod api-server-6d7c8f-pm9kl | grep ProbeWarning -A 2

# 查看探针输出
kubectl logs api-server-6d7c8f-pm9kl | grep -i warning
```

**2. 手动执行探针**
```bash
# 获取探针返回的警告信息
kubectl exec api-server-6d7c8f-pm9kl -- curl http://localhost:8080/health

# 输出示例：
# {"status":"pass","warnings":["cache_hit_rate_low","db_connection_pool_75%"]}
```

**3. 检查指标**
```bash
# 查看资源使用情况
kubectl top pod api-server-6d7c8f-pm9kl

# 查看应用指标（如果暴露 Prometheus metrics）
kubectl exec api-server-6d7c8f-pm9kl -- curl http://localhost:9090/metrics | grep -i warning
```

#### 解决建议

| 警告类型 | 处理建议 | 预防措施 |
|:---|:---|:---|
| **依赖服务降级** | 检查降级依赖，评估是否需要人工介入 | 实现优雅降级逻辑，非关键依赖失败不影响主流程 |
| **资源接近上限** | 增加资源配额或优化资源使用 | 设置告警阈值，自动扩容（HPA） |
| **性能下降** | 分析慢查询、优化代码、增加缓存 | 性能测试、监控、分布式追踪 |
| **配置异常** | 检查 ConfigMap、Secret 是否正确加载 | 配置验证、版本管理 |

---

## 探针事件时间线图

### 正常启动流程

```
时间轴    容器生命周期                     探针状态                      事件
─────────────────────────────────────────────────────────────────────────────
t=0s      容器 Created                     
t=1s      容器 Started                     ┌─ Startup Probe 开始
                                           │  (Liveness/Readiness 阻塞)
t=5s      应用初始化中...                  │  [Startup: Checking...]
t=10s                                      │  [Startup: Checking...]
t=15s                                      │  [Startup: Checking...]
t=20s     应用启动完成                     │  [Startup: Success ✓]
                                           └─ Startup Probe 成功
t=21s                                      ┌─ Liveness Probe 开始
                                           ├─ Readiness Probe 开始
t=21s                                      │  [Readiness: Checking deps...]
t=26s                                      │  [Readiness: Success ✓]        → Ready=True (加入 Endpoints)
t=31s                                      │  [Liveness: Success ✓]
t=36s                                      │  [Readiness: Success ✓]
t=∞       正常运行                         └─ 定期检查...
```

### 启动失败流程（Startup Probe Timeout）

```
时间轴    容器生命周期                     探针状态                      事件
─────────────────────────────────────────────────────────────────────────────
t=0s      容器 Created                     
t=1s      容器 Started                     ┌─ Startup Probe 开始
t=5s      应用初始化中...                  │  [Startup: Failed] × 1        → Unhealthy (Startup probe failed)
t=15s     依赖服务连接失败                 │  [Startup: Failed] × 2        → Unhealthy
t=25s     仍在重试连接...                  │  [Startup: Failed] × 3        → Unhealthy
  ...     (重复)
t=295s                                     │  [Startup: Failed] × 29       → Unhealthy
t=305s    达到 failureThreshold=30         └─ [Startup: Timeout]           → Killing (Container killed)
t=310s    容器终止                         
t=320s    等待退避重启（BackOff）                                           → BackOff (CrashLoopBackOff)
t=330s    容器重新 Created                                                  → Pulled, Created, Started
t=331s    重新开始 Startup Probe           ┌─ Startup Probe 开始
  ...     (循环)
```

### Liveness 失败重启流程

```
时间轴    容器生命周期                     探针状态                      事件
─────────────────────────────────────────────────────────────────────────────
t=0s      容器正常运行                     [Liveness: Success ✓]
                                           [Readiness: Success ✓]
t=60s     应用进入死锁状态                 
t=70s                                      [Liveness: Failed] × 1         → Unhealthy (Liveness probe failed)
                                           [Readiness: Success ✓]
t=80s                                      [Liveness: Failed] × 2         → Unhealthy
                                           [Readiness: Success ✓]
t=90s     达到 failureThreshold=3          [Liveness: Failed] × 3         → Unhealthy
                                           └─ 触发容器重启                  → Killing (Container killed)
t=95s     容器终止                         
t=100s    容器重新 Created/Started                                          → Created, Started
t=101s    Startup Probe 开始               ┌─ Startup Probe 开始
t=120s    启动完成                         └─ [Startup: Success ✓]
t=121s    Liveness/Readiness 恢复          ┌─ [Liveness: Success ✓]
                                           └─ [Readiness: Success ✓]      → Ready=True (重新加入 Endpoints)
```

### Readiness 失败隔离流量

```
时间轴    容器生命周期                     探针状态                      事件/网络影响
─────────────────────────────────────────────────────────────────────────────────────
t=0s      容器正常运行                     [Liveness: Success ✓]
                                           [Readiness: Success ✓]         Endpoints: [10.244.1.5] (Ready)
t=60s     数据库连接池耗尽                 
t=65s                                      [Readiness: Failed] × 1        → Unhealthy (Readiness probe failed)
                                           [Liveness: Success ✓]          (容器不重启)
t=70s                                      [Readiness: Failed] × 2        → Unhealthy
t=75s     达到 failureThreshold=3          [Readiness: Failed] × 3        → Unhealthy
                                           └─ Pod 标记为 Not Ready          → Ready=False
                                                                           Endpoints: [] (NotReady: [10.244.1.5])
                                                                           kube-proxy 更新 iptables 规则
                                                                           流量停止路由到此 Pod
t=120s    数据库连接恢复                   
t=125s                                     [Readiness: Success] × 1
t=130s    达到 successThreshold=1          [Readiness: Success ✓]         → Ready=True
                                                                           Endpoints: [10.244.1.5] (Ready)
                                                                           kube-proxy 更新 iptables 规则
                                                                           流量恢复路由
```

---

## 常见错误配置模式与最佳实践

### ❌ 反模式 1：Liveness 检查外部依赖

**错误配置**：
```yaml
livenessProbe:
  httpGet:
    path: /health  # 此端点检查数据库、Redis、消息队列连接
    port: 8080
  periodSeconds: 10
  failureThreshold: 3
```

**问题**：
- 数据库临时不可用 → Liveness 失败 → 容器重启
- 所有 Pod 同时重启 → 级联故障 → 服务完全中断
- 重启无法解决外部依赖问题

**正确配置**：
```yaml
livenessProbe:
  httpGet:
    path: /livez  # 仅检查进程自身是否健康（无外部依赖）
    port: 8080
  periodSeconds: 10
  failureThreshold: 3

readinessProbe:
  httpGet:
    path: /readyz  # 检查外部依赖（数据库、缓存等）
    port: 8080
  periodSeconds: 5
  failureThreshold: 3
```

**健康检查端点示例（Go）**：
```go
// Liveness: 只检查进程是否活跃
http.HandleFunc("/livez", func(w http.ResponseWriter, r *http.Request) {
    w.WriteHeader(http.StatusOK)
    w.Write([]byte("alive"))
})

// Readiness: 检查依赖和服务能力
http.HandleFunc("/readyz", func(w http.ResponseWriter, r *http.Request) {
    if err := db.Ping(); err != nil {
        w.WriteHeader(http.StatusServiceUnavailable)
        w.Write([]byte("database unavailable"))
        return
    }
    if err := redisClient.Ping(); err != nil {
        w.WriteHeader(http.StatusServiceUnavailable)
        w.Write([]byte("redis unavailable"))
        return
    }
    w.WriteHeader(http.StatusOK)
    w.Write([]byte("ready"))
})
```

---

### ❌ 反模式 2：initialDelaySeconds 过短

**错误配置**（Java Spring Boot 应用）：
```yaml
livenessProbe:
  httpGet:
    path: /actuator/health
    port: 8080
  initialDelaySeconds: 10  # Spring Boot 启动通常需要 30-60 秒
  periodSeconds: 10
  failureThreshold: 3
```

**问题**：
- 应用启动需要 45 秒
- 10 秒后开始探测 → 失败 × 3 → 30 秒后重启
- 应用永远无法启动成功 → CrashLoopBackOff

**临时解决方案**（不推荐）：
```yaml
livenessProbe:
  httpGet:
    path: /actuator/health
    port: 8080
  initialDelaySeconds: 90  # 设置为预估启动时间的 2 倍
  periodSeconds: 10
  failureThreshold: 3
```

**问题**：
- 如果启动后 10 秒就死锁，需要等待 90 秒才能检测到

**✅ 推荐解决方案：使用 Startup Probe**：
```yaml
startupProbe:
  httpGet:
    path: /actuator/health/liveness
    port: 8080
  initialDelaySeconds: 0
  periodSeconds: 10
  failureThreshold: 30  # 最多等待 300 秒（5 分钟）

livenessProbe:
  httpGet:
    path: /actuator/health/liveness
    port: 8080
  periodSeconds: 10
  failureThreshold: 3  # 启动后，只需 30 秒即可检测到故障

readinessProbe:
  httpGet:
    path: /actuator/health/readiness
    port: 8080
  periodSeconds: 5
  failureThreshold: 3
```

---

### ❌ 反模式 3：探针超时设置不合理

**错误配置**：
```yaml
readinessProbe:
  httpGet:
    path: /ready  # 此端点执行复杂的数据库查询，耗时 2-3 秒
    port: 8080
  timeoutSeconds: 1  # 默认值，过短
  periodSeconds: 10
```

**问题**：
- 正常情况下响应 2 秒，探针超时失败
- 高负载时更频繁失败 → 健康的 Pod 被移除流量

**✅ 正确配置**：
```yaml
readinessProbe:
  httpGet:
    path: /ready
    port: 8080
  timeoutSeconds: 5  # 根据 P95 响应时间设置
  periodSeconds: 10
  failureThreshold: 3
```

**更好的解决方案：优化健康检查端点**：
```go
// 不要在健康检查中执行重负载操作
http.HandleFunc("/ready", func(w http.ResponseWriter, r *http.Request) {
    // ❌ 错误：复杂查询
    // rows, err := db.Query("SELECT COUNT(*) FROM large_table WHERE status = 'active'")
    
    // ✅ 正确：轻量级连接测试
    if err := db.Ping(); err != nil {
        w.WriteHeader(http.StatusServiceUnavailable)
        return
    }
    w.WriteHeader(http.StatusOK)
})
```

---

### ❌ 反模式 4：Liveness 和 Readiness 完全相同

**错误配置**：
```yaml
livenessProbe:
  httpGet:
    path: /health
    port: 8080
  periodSeconds: 10
  failureThreshold: 3

readinessProbe:
  httpGet:
    path: /health  # 与 Liveness 相同
    port: 8080
  periodSeconds: 10
  failureThreshold: 3
```

**问题**：
- 无法区分"进程死锁"和"依赖不可用"
- 依赖故障导致不必要的容器重启

**✅ 正确配置**：
```yaml
livenessProbe:
  httpGet:
    path: /livez  # 只检查进程自身
    port: 8080
  periodSeconds: 10
  failureThreshold: 3

readinessProbe:
  httpGet:
    path: /readyz  # 检查依赖和流量处理能力
    port: 8080
  periodSeconds: 5
  failureThreshold: 3
  successThreshold: 1
```

---

### ✅ 最佳实践总结

| 探针类型 | 检查内容 | 失败行为 | 典型配置 | 端点示例 |
|:---|:---|:---|:---|:---|
| **Startup** | 应用是否启动完成 | 重启容器 | `failureThreshold: 30`<br>`periodSeconds: 10` | `/livez`（简单检查） |
| **Liveness** | 进程是否活跃（无死锁、无挂起） | 重启容器 | `failureThreshold: 3`<br>`periodSeconds: 10` | `/livez`（无外部依赖） |
| **Readiness** | 是否可以处理流量（含依赖检查） | 移除流量 | `failureThreshold: 3`<br>`periodSeconds: 5` | `/readyz`（含依赖检查） |

---

## 生产级配置示例

### 示例 1：Web 应用（Node.js Express）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-app
  template:
    metadata:
      labels:
        app: web-app
    spec:
      containers:
      - name: app
        image: myregistry/web-app:v1.2.3
        ports:
        - containerPort: 3000
          name: http
        
        # 启动探针：Node.js 通常启动较快，但仍给予足够时间
        startupProbe:
          httpGet:
            path: /healthz/startup
            port: 3000
            scheme: HTTP
          initialDelaySeconds: 0
          periodSeconds: 5
          timeoutSeconds: 3
          successThreshold: 1
          failureThreshold: 12  # 最多等待 60 秒
        
        # 存活探针：检查进程是否响应
        livenessProbe:
          httpGet:
            path: /healthz/liveness
            port: 3000
            httpHeaders:
            - name: X-Probe-Type
              value: liveness
          initialDelaySeconds: 0  # Startup 成功后立即开始
          periodSeconds: 10
          timeoutSeconds: 3
          successThreshold: 1
          failureThreshold: 3  # 30 秒内失败则重启
        
        # 就绪探针：检查依赖（数据库、Redis）
        readinessProbe:
          httpGet:
            path: /healthz/readiness
            port: 3000
          initialDelaySeconds: 0
          periodSeconds: 5
          timeoutSeconds: 3
          successThreshold: 1
          failureThreshold: 3
        
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 512Mi
```

**健康检查端点实现（Express）**：
```javascript
const express = require('express');
const app = express();

// Startup probe: 检查应用是否完成初始化
let isStarted = false;
app.get('/healthz/startup', (req, res) => {
  if (isStarted) {
    res.status(200).send('started');
  } else {
    res.status(503).send('not started');
  }
});

// Liveness probe: 仅检查进程是否响应
app.get('/healthz/liveness', (req, res) => {
  res.status(200).send('alive');
});

// Readiness probe: 检查依赖服务
app.get('/healthz/readiness', async (req, res) => {
  try {
    // 检查数据库连接
    await dbPool.query('SELECT 1');
    
    // 检查 Redis 连接
    await redisClient.ping();
    
    res.status(200).json({ status: 'ready' });
  } catch (error) {
    console.error('Readiness check failed:', error);
    res.status(503).json({ status: 'not ready', error: error.message });
  }
});

// 应用启动完成后标记为已启动
app.listen(3000, () => {
  console.log('Server started on port 3000');
  isStarted = true;  // 标记启动完成
});
```

---

### 示例 2：Java Spring Boot 应用（慢启动）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: java-api
spec:
  replicas: 2
  selector:
    matchLabels:
      app: java-api
  template:
    metadata:
      labels:
        app: java-api
    spec:
      containers:
      - name: api
        image: myregistry/java-api:v2.1.0
        ports:
        - containerPort: 8080
          name: http
        
        # 启动探针：Java 应用启动慢，给予充足时间
        startupProbe:
          httpGet:
            path: /actuator/health/liveness
            port: 8080
          initialDelaySeconds: 10  # JVM 初始化时间
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 30  # 最多等待 310 秒（~5 分钟）
        
        # 存活探针：启动后快速检测
        livenessProbe:
          httpGet:
            path: /actuator/health/liveness
            port: 8080
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 3
        
        # 就绪探针：检查依赖和应用状态
        readinessProbe:
          httpGet:
            path: /actuator/health/readiness
            port: 8080
          periodSeconds: 5
          timeoutSeconds: 5
          successThreshold: 1
          failureThreshold: 3
        
        resources:
          requests:
            cpu: 1000m
            memory: 1Gi
          limits:
            cpu: 2000m
            memory: 2Gi
        
        env:
        - name: JAVA_OPTS
          value: "-Xms1g -Xmx1g -XX:+UseG1GC"
```

**Spring Boot 配置（application.yml）**：
```yaml
management:
  endpoint:
    health:
      probes:
        enabled: true
      group:
        liveness:
          include: "ping"  # 仅检查应用是否响应
        readiness:
          include: "ping,db,redis"  # 检查依赖
  health:
    db:
      enabled: true
    redis:
      enabled: true
```

---

### 示例 3：gRPC 微服务（v1.27+）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: grpc-service
spec:
  replicas: 3
  selector:
    matchLabels:
      app: grpc-service
  template:
    metadata:
      labels:
        app: grpc-service
    spec:
      containers:
      - name: service
        image: myregistry/grpc-service:v1.0.0
        ports:
        - containerPort: 50051
          name: grpc
        
        # Startup probe: gRPC 探针
        startupProbe:
          grpc:
            port: 50051
            service: grpc.health.v1.Health  # 标准 gRPC 健康检查服务
          initialDelaySeconds: 0
          periodSeconds: 5
          failureThreshold: 12
        
        # Liveness probe: gRPC 探针
        livenessProbe:
          grpc:
            port: 50051
          periodSeconds: 10
          timeoutSeconds: 3
          failureThreshold: 3
        
        # Readiness probe: gRPC 探针
        readinessProbe:
          grpc:
            port: 50051
          periodSeconds: 5
          timeoutSeconds: 3
          failureThreshold: 3
        
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: 1000m
            memory: 512Mi
```

**gRPC 健康检查实现（Go）**：
```go
import (
    "google.golang.org/grpc/health"
    "google.golang.org/grpc/health/grpc_health_v1"
)

func main() {
    server := grpc.NewServer()
    
    // 注册 gRPC 健康检查服务
    healthServer := health.NewServer()
    grpc_health_v1.RegisterHealthServer(server, healthServer)
    
    // 启动时标记为 NOT_SERVING
    healthServer.SetServingStatus("", grpc_health_v1.HealthCheckResponse_NOT_SERVING)
    
    // 应用初始化...
    initDatabase()
    initCache()
    
    // 初始化完成后标记为 SERVING
    healthServer.SetServingStatus("", grpc_health_v1.HealthCheckResponse_SERVING)
    
    server.Serve(listener)
}
```

---

### 示例 4：数据库（PostgreSQL StatefulSet）

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
spec:
  serviceName: postgres
  replicas: 1
  selector:
    matchLabels:
      app: postgres
  template:
    metadata:
      labels:
        app: postgres
    spec:
      containers:
      - name: postgres
        image: postgres:15
        ports:
        - containerPort: 5432
          name: postgres
        
        # Startup probe: 数据库初始化可能较慢
        startupProbe:
          exec:
            command:
            - /bin/sh
            - -c
            - pg_isready -U postgres
          initialDelaySeconds: 5
          periodSeconds: 5
          timeoutSeconds: 3
          failureThreshold: 30  # 最多等待 155 秒
        
        # Liveness probe: 检查数据库进程
        livenessProbe:
          exec:
            command:
            - /bin/sh
            - -c
            - pg_isready -U postgres
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 3
        
        # Readiness probe: 检查数据库是否接受连接
        readinessProbe:
          exec:
            command:
            - /bin/sh
            - -c
            - psql -U postgres -c "SELECT 1"
          periodSeconds: 5
          timeoutSeconds: 3
          failureThreshold: 3
        
        env:
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: postgres-secret
              key: password
        
        volumeMounts:
        - name: data
          mountPath: /var/lib/postgresql/data
  
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: [ "ReadWriteOnce" ]
      resources:
        requests:
          storage: 10Gi
```

---

## 故障排查流程图

```
探针失败事件报告
         │
         ├─ 判断探针类型
         │
         ├─ Startup Probe Failed
         │    │
         │    ├─ 检查启动时间预算
         │    │   └─ initialDelaySeconds + (failureThreshold × periodSeconds)
         │    │
         │    ├─ 查看容器日志
         │    │   └─ kubectl logs <pod> --previous
         │    │
         │    └─ 常见原因
         │        ├─ 启动时间超过配置 → 增加 failureThreshold
         │        ├─ 应用启动失败 → 修复代码/配置
         │        ├─ 资源不足 → 增加 requests/limits
         │        └─ 镜像拉取失败 → 检查 imagePullPolicy
         │
         ├─ Liveness Probe Failed
         │    │
         │    ├─ 检查探针是否包含外部依赖
         │    │   └─ 是 → 反模式！移至 Readiness Probe
         │    │
         │    ├─ 手动测试探针端点
         │    │   └─ kubectl exec <pod> -- curl http://localhost:8080/livez
         │    │
         │    └─ 常见原因
         │        ├─ 应用死锁/挂起 → 代码分析、Thread Dump
         │        ├─ 内存不足 → 检查 OOMKilled，增加内存
         │        ├─ CPU 节流 → 检查 throttling metrics
         │        ├─ 探针超时过短 → 增加 timeoutSeconds
         │        └─ 探针端点错误 → 修正 path/port
         │
         └─ Readiness Probe Failed
              │
              ├─ 检查 Service Endpoints
              │   └─ kubectl get endpoints <service>
              │
              ├─ 手动测试探针端点和依赖
              │   ├─ kubectl exec <pod> -- curl http://localhost:8080/readyz
              │   ├─ kubectl exec <pod> -- nc -zv mysql-service 3306
              │   └─ kubectl exec <pod> -- redis-cli -h redis-service ping
              │
              └─ 常见原因
                  ├─ 依赖服务不可用 → 修复依赖，或调整探针逻辑
                  ├─ 应用初始化未完成 → 增加 initialDelaySeconds
                  ├─ 流量过载 → 增加副本数（HPA）
                  ├─ 网络问题 → 检查 NetworkPolicy、DNS
                  └─ 配置错误 → 检查 ConfigMap、Secret
```

---

## 监控和告警建议

### 关键指标

| 指标 | PromQL 示例 | 告警阈值 | 说明 |
|:---|:---|:---|:---|
| **探针失败率** | `rate(kube_pod_container_status_restarts_total[5m]) > 0` | > 0.1/min | 容器频繁重启 |
| **Unready Pod 数量** | `kube_pod_status_ready{condition="false"} > 0` | > 1 (持续 5 分钟) | 服务容量下降 |
| **CrashLoopBackOff** | `kube_pod_container_status_waiting_reason{reason="CrashLoopBackOff"} > 0` | > 0 | 严重故障 |
| **探针延迟** | `kubelet_http_requests_duration_seconds{handler="/healthz"}` | P95 > 2s | 探针响应慢 |

### Prometheus 告警规则示例

```yaml
groups:
- name: kubernetes-probes
  interval: 30s
  rules:
  
  # 高频率容器重启
  - alert: HighContainerRestartRate
    expr: rate(kube_pod_container_status_restarts_total[15m]) > 0.1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 容器频繁重启"
      description: "重启率: {{ $value | humanize }}/秒，可能是 Liveness Probe 配置不当或应用故障"
  
  # Pod 长时间 Unready
  - alert: PodNotReady
    expr: kube_pod_status_ready{condition="false"} == 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 长时间未就绪"
      description: "Readiness Probe 持续失败，检查依赖服务和探针配置"
  
  # 所有副本都 Unready
  - alert: ServiceCompletelyDown
    expr: |
      sum by (namespace, deployment) (kube_pod_status_ready{condition="true"})
      / 
      sum by (namespace, deployment) (kube_pod_status_ready)
      < 0.5
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "服务 {{ $labels.namespace }}/{{ $labels.deployment }} 超过 50% 副本不可用"
      description: "当前可用率: {{ $value | humanizePercentage }}，服务可能完全中断"
  
  # CrashLoopBackOff
  - alert: PodCrashLooping
    expr: kube_pod_container_status_waiting_reason{reason="CrashLoopBackOff"} == 1
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 进入 CrashLoopBackOff"
      description: "Startup/Liveness Probe 持续失败，检查应用日志和探针配置"
```

---

## 相关文档

### Domain-33 内部文档
- **01-pod-lifecycle-events.md**: Pod 生命周期事件（Created, Started, Killing 等）
- **02-scheduling-events.md**: 调度相关事件（FailedScheduling 等）
- **03-image-pull-events.md**: 镜像拉取事件（Pulling, Pulled, Failed 等）
- **05-resource-events.md**: 资源事件（OOMKilled, Evicted 等）

### 跨域参考
- **Domain-1: 架构基础**: `01-kubernetes-architecture-overview.md`（kubelet 组件）
- **Domain-2: 核心概念**: `05-pod-lifecycle.md`（Pod 生命周期详解）
- **Domain-6: 可观测性**: `01-logging-architecture.md`（日志收集）
- **Domain-6: 可观测性**: `02-metrics-monitoring.md`（Prometheus 监控）
- **Topic: 故障排查**: `topic-structural-trouble-shooting/05-workloads/01-pod-troubleshooting.md`

### 官方文档
- [Configure Liveness, Readiness and Startup Probes](https://kubernetes.io/docs/tasks/configure-pod-container/configure-liveness-readiness-startup-probes/)
- [Pod Lifecycle](https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/)
- [gRPC Health Checking Protocol](https://github.com/grpc/grpc/blob/master/doc/health-checking.md)

---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 04/15
