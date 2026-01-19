# 74 - 容器生命周期钩子 (Container Lifecycle Hooks)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 中级

## 容器生命周期架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        容器完整生命周期                                              │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌────────────────────────────────────────────────────────────────────────────────┐ │
│  │                        Pod 创建阶段                                            │ │
│  │                                                                                 │ │
│  │  kubectl apply                                                                  │ │
│  │       │                                                                         │ │
│  │       ▼                                                                         │ │
│  │  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐                       │ │
│  │  │  Pending    │────▶│ Init        │────▶│ Container   │                       │ │
│  │  │             │     │ Containers  │     │ Creating    │                       │ │
│  │  └─────────────┘     └─────────────┘     └──────┬──────┘                       │ │
│  │                                                  │                              │ │
│  └──────────────────────────────────────────────────┼──────────────────────────────┘ │
│                                                     │                                │
│  ┌──────────────────────────────────────────────────┼──────────────────────────────┐ │
│  │                        Pod 运行阶段              │                              │ │
│  │                                                  ▼                              │ │
│  │  ┌──────────────────────────────────────────────────────────────────────────┐  │ │
│  │  │                    容器启动流程                                           │  │ │
│  │  │                                                                          │  │ │
│  │  │  容器创建 ──▶ PostStart Hook ──▶ StartupProbe ──▶ Readiness/Liveness    │  │ │
│  │  │     │              │                  │                    │             │  │ │
│  │  │     │              │ (阻塞)           │ (阻塞)             │             │  │ │
│  │  │     │              ▼                  ▼                    ▼             │  │ │
│  │  │     │         执行完成后          探针通过后          定期检查           │  │ │
│  │  │     │         才标记为           才开始接收          持续运行           │  │ │
│  │  │     │         Running            流量                                   │  │ │
│  │  │     │                                                                    │  │ │
│  │  │     └──────▶ 主进程启动 (ENTRYPOINT/CMD)                                │  │ │
│  │  │                    │                                                     │  │ │
│  │  │                    ▼                                                     │  │ │
│  │  │              应用正常运行                                                │  │ │
│  │  │                                                                          │  │ │
│  │  └──────────────────────────────────────────────────────────────────────────┘  │ │
│  │                                                                                 │ │
│  └─────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                      │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐ │
│  │                        Pod 终止阶段                                             │ │
│  │                                                                                 │ │
│  │  kubectl delete / 自动驱逐                                                      │ │
│  │       │                                                                         │ │
│  │       ▼                                                                         │ │
│  │  ┌──────────────────────────────────────────────────────────────────────────┐  │ │
│  │  │                    容器终止流程                                           │  │ │
│  │  │                                                                          │  │ │
│  │  │  Terminating ──▶ PreStop Hook ──▶ SIGTERM ──▶ 宽限期 ──▶ SIGKILL        │  │ │
│  │  │      │               │              │           │           │            │  │ │
│  │  │      │               │              │           │           │            │  │ │
│  │  │      ▼               ▼              ▼           ▼           ▼            │  │ │
│  │  │  标记状态       执行清理       通知主进程    等待退出    强制终止        │  │ │
│  │  │  从Endpoints   (并行执行)      优雅关闭      (30s默认)                   │  │ │
│  │  │  移除                                                                    │  │ │
│  │  │                                                                          │  │ │
│  │  │  时间线:                                                                 │  │ │
│  │  │  |←─ PreStop ─→|←── terminationGracePeriodSeconds (30s) ──→|            │  │ │
│  │  │  T0            T1                                          T30          │  │ │
│  │  │                                                                          │  │ │
│  │  └──────────────────────────────────────────────────────────────────────────┘  │ │
│  │                                                                                 │ │
│  └─────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

## 生命周期钩子类型

| 钩子 | 触发时机 | 阻塞行为 | 用途 | 失败影响 |
|-----|---------|---------|------|---------|
| **PostStart** | 容器创建后立即执行 | 阻塞，完成前容器不标记为 Running | 初始化任务、依赖检查 | 容器重启 |
| **PreStop** | 容器终止前执行 | 阻塞，与宽限期并行计时 | 清理任务、优雅关闭 | 不影响终止 |

## 钩子执行方式详解

| 方式 | 说明 | 适用场景 | 版本 |
|-----|------|---------|------|
| **exec** | 在容器内执行命令 | 运行脚本、调用内部 API | 全版本 |
| **httpGet** | 发送 HTTP GET 请求 | 通知外部服务、健康检查 | 全版本 |
| **tcpSocket** | TCP 连接检查 | 端口可用性检查 | v1.25+ |
| **sleep** | 简单休眠等待 | 延迟终止、等待连接排空 | v1.29+ GA |

## 完整生命周期配置示例

### 基础配置

```yaml
# lifecycle-complete.yaml
apiVersion: v1
kind: Pod
metadata:
  name: lifecycle-demo
  labels:
    app: lifecycle-demo
spec:
  # 优雅终止时间 (PreStop + SIGTERM 等待时间)
  terminationGracePeriodSeconds: 60
  
  containers:
    - name: app
      image: myapp:v1.0
      ports:
        - containerPort: 8080
          name: http
      
      # 生命周期钩子
      lifecycle:
        # 启动后钩子
        postStart:
          exec:
            command:
              - /bin/sh
              - -c
              - |
                echo "[$(date)] PostStart: Initializing application..."
                
                # 等待应用内部初始化完成
                until curl -sf http://localhost:8080/internal/ready; do
                  echo "Waiting for internal initialization..."
                  sleep 1
                done
                
                # 注册到服务发现
                curl -X POST http://service-discovery:8500/v1/agent/service/register \
                  -H "Content-Type: application/json" \
                  -d '{"name":"myapp","port":8080}'
                
                echo "[$(date)] PostStart: Initialization complete"
        
        # 停止前钩子
        preStop:
          exec:
            command:
              - /bin/sh
              - -c
              - |
                echo "[$(date)] PreStop: Starting graceful shutdown..."
                
                # 从服务发现注销
                curl -X PUT http://service-discovery:8500/v1/agent/service/deregister/myapp
                
                # 标记应用不接收新请求
                touch /tmp/shutdown-marker
                
                # 等待现有请求完成 (与 readinessProbe 配合)
                echo "Waiting for in-flight requests to complete..."
                sleep 15
                
                # 通知应用开始关闭
                curl -X POST http://localhost:8080/internal/shutdown
                
                echo "[$(date)] PreStop: Graceful shutdown preparation complete"
      
      # 探针配置 (配合生命周期钩子)
      startupProbe:
        httpGet:
          path: /healthz
          port: http
        initialDelaySeconds: 0
        periodSeconds: 5
        failureThreshold: 30  # 最多等待 150s
      
      readinessProbe:
        httpGet:
          path: /ready
          port: http
        initialDelaySeconds: 0
        periodSeconds: 5
        failureThreshold: 3
      
      livenessProbe:
        httpGet:
          path: /healthz
          port: http
        initialDelaySeconds: 0
        periodSeconds: 10
        failureThreshold: 3
      
      resources:
        requests:
          cpu: 100m
          memory: 128Mi
        limits:
          cpu: 500m
          memory: 512Mi
```

### HTTP 钩子配置

```yaml
# http-hooks.yaml
apiVersion: v1
kind: Pod
metadata:
  name: http-hook-demo
spec:
  terminationGracePeriodSeconds: 45
  
  containers:
    - name: app
      image: myapp:v1.0
      ports:
        - containerPort: 8080
          name: http
        - containerPort: 9090
          name: admin
      
      lifecycle:
        # HTTP GET 启动后钩子
        postStart:
          httpGet:
            path: /api/lifecycle/started
            port: admin
            scheme: HTTP
            httpHeaders:
              - name: X-Lifecycle-Event
                value: "post-start"
              - name: X-Pod-Name
                valueFrom:
                  fieldRef:
                    fieldPath: metadata.name
        
        # HTTP GET 停止前钩子
        preStop:
          httpGet:
            path: /api/lifecycle/stopping
            port: admin
            scheme: HTTP
            httpHeaders:
              - name: X-Lifecycle-Event
                value: "pre-stop"
              - name: X-Shutdown-Reason
                value: "Container terminating"
              - name: X-Grace-Period
                value: "45"
```

### Sleep 钩子配置 (v1.29+)

```yaml
# sleep-hook.yaml (v1.29+)
apiVersion: v1
kind: Pod
metadata:
  name: sleep-hook-demo
spec:
  terminationGracePeriodSeconds: 30
  
  containers:
    - name: app
      image: nginx:latest
      
      lifecycle:
        preStop:
          # 简单的延迟关闭，等待负载均衡器更新
          sleep:
            seconds: 10
---
# 混合使用 sleep 和 exec
apiVersion: v1
kind: Pod
metadata:
  name: mixed-hook-demo
spec:
  terminationGracePeriodSeconds: 45
  
  containers:
    - name: app
      image: myapp:v1.0
      
      lifecycle:
        preStop:
          exec:
            command:
              - /bin/sh
              - -c
              - |
                # 先执行清理逻辑
                /scripts/cleanup.sh
                
                # 然后等待 (如果 sleep hook 不可用时的替代方案)
                sleep 15
```

## 优雅终止完整流程

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        优雅终止详细流程                                              │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  T0: 收到终止信号 (kubectl delete / 自动驱逐)                                        │
│      │                                                                               │
│      ├── 1. Pod 状态标记为 Terminating                                              │
│      │                                                                               │
│      ├── 2. Service Endpoints 开始移除 Pod (异步)                                   │
│      │      └── 负载均衡器开始排除此 Pod                                            │
│      │                                                                               │
│      └── 3. 开始终止流程计时器 (terminationGracePeriodSeconds)                      │
│             │                                                                        │
│             ▼                                                                        │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                     并行执行                                                   │  │
│  │                                                                                │  │
│  │  ┌─────────────────────────┐    ┌─────────────────────────────────────────┐  │  │
│  │  │     PreStop Hook        │    │        主进程信号处理                    │  │  │
│  │  │                         │    │                                          │  │  │
│  │  │  • 执行 PreStop 命令    │    │  • 等待 PreStop 完成                     │  │  │
│  │  │  • 超时受宽限期限制     │    │  • PreStop 完成后发送 SIGTERM            │  │  │
│  │  │  • 失败不阻止终止       │    │  • 主进程收到信号开始优雅关闭            │  │  │
│  │  │                         │    │                                          │  │  │
│  │  └─────────────────────────┘    └─────────────────────────────────────────┘  │  │
│  │                                                                                │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│             │                                                                        │
│             ▼                                                                        │
│  T30 (默认): 宽限期结束                                                             │
│      │                                                                               │
│      └── 4. 发送 SIGKILL 强制终止                                                   │
│             │                                                                        │
│             ▼                                                                        │
│  T30+: 容器彻底终止，资源释放                                                        │
│                                                                                      │
│  ════════════════════════════════════════════════════════════════════════════════   │
│                                                                                      │
│  最佳实践配置:                                                                       │
│                                                                                      │
│  terminationGracePeriodSeconds: 60                                                  │
│                                                                                      │
│  |←── PreStop (15s) ──→|←──── SIGTERM 处理 (45s) ────→|                            │
│  T0                    T15                             T60                           │
│                                                        │                             │
│                                                        └── SIGKILL (如需要)          │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 优雅终止最佳实践配置

```yaml
# graceful-shutdown-best-practice.yaml
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
      # 足够的优雅终止时间
      terminationGracePeriodSeconds: 60
      
      containers:
        - name: app
          image: web-app:v1.0
          ports:
            - containerPort: 8080
          
          lifecycle:
            preStop:
              exec:
                command:
                  - /bin/sh
                  - -c
                  - |
                    # 1. 通知应用停止接收新连接
                    curl -X POST http://localhost:8080/admin/drain
                    
                    # 2. 等待现有连接完成
                    # 这个时间应该小于 terminationGracePeriodSeconds
                    sleep 15
                    
                    # 3. 应用会收到 SIGTERM，开始关闭
          
          # 应用需要正确处理 SIGTERM
          # 示例 Go 代码:
          # signal.Notify(sigChan, syscall.SIGTERM)
          # <-sigChan
          # server.Shutdown(ctx)
          
          readinessProbe:
            httpGet:
              path: /ready
              port: 8080
            periodSeconds: 5
            # preStop 执行后，应用应返回非 200
            # 这样 Endpoints Controller 会更快移除 Pod
```

## 钩子执行注意事项

| 注意点 | 说明 | 最佳实践 |
|-------|------|---------|
| **阻塞性** | PostStart 阻塞时容器不会标记为 Running | PostStart 应快速完成或异步执行 |
| **超时** | 钩子执行受 terminationGracePeriodSeconds 限制 | PreStop 时间 + 应用关闭时间 < 宽限期 |
| **重试** | 钩子失败不重试 | 钩子脚本内部实现重试逻辑 |
| **并发** | PreStop 和 SIGTERM 可能并发 | 应用应同时处理 PreStop 和 SIGTERM |
| **日志** | 钩子输出不记录到容器日志 | 使用 kubectl describe 查看事件 |
| **返回值** | 非零返回值视为失败 | PostStart 失败导致容器重启 |
| **网络** | 钩子执行时网络可能已不可用 | PreStop 中避免依赖外部服务 |

## 常见 PostStart 用例

### 1. 等待依赖服务

```yaml
postStart:
  exec:
    command:
      - /bin/sh
      - -c
      - |
        echo "Waiting for dependencies..."
        
        # 等待数据库
        until nc -z database 5432; do
          echo "Waiting for database..."
          sleep 2
        done
        
        # 等待 Redis
        until nc -z redis 6379; do
          echo "Waiting for redis..."
          sleep 2
        done
        
        # 等待消息队列
        until nc -z rabbitmq 5672; do
          echo "Waiting for rabbitmq..."
          sleep 2
        done
        
        echo "All dependencies ready"
```

### 2. 注册到服务发现

```yaml
postStart:
  exec:
    command:
      - /bin/sh
      - -c
      - |
        # Consul 注册
        curl -X PUT http://consul:8500/v1/agent/service/register \
          -H "Content-Type: application/json" \
          -d '{
            "Name": "'$SERVICE_NAME'",
            "ID": "'$POD_NAME'",
            "Address": "'$POD_IP'",
            "Port": 8080,
            "Check": {
              "HTTP": "http://'$POD_IP':8080/health",
              "Interval": "10s"
            }
          }'
```

### 3. 初始化配置和数据

```yaml
postStart:
  exec:
    command:
      - /bin/sh
      - -c
      - |
        # 从 ConfigMap/Secret 挂载的文件生成配置
        envsubst < /config/template.conf > /app/config/app.conf
        
        # 初始化数据库 schema (幂等操作)
        /app/bin/migrate up
        
        # 预热缓存
        /app/bin/warmup-cache
```

### 4. 发送启动通知

```yaml
postStart:
  httpGet:
    path: /webhooks/pod-started
    port: 8080
    httpHeaders:
      - name: X-Pod-Name
        value: $(POD_NAME)
      - name: X-Pod-IP
        value: $(POD_IP)
      - name: X-Node-Name
        value: $(NODE_NAME)
```

## 常见 PreStop 用例

### 1. Web 服务器优雅关闭

```yaml
# Nginx 优雅关闭
preStop:
  exec:
    command:
      - /bin/sh
      - -c
      - |
        echo "Starting nginx graceful shutdown..."
        
        # 发送 QUIT 信号，停止接收新连接
        nginx -s quit
        
        # 等待所有 worker 进程退出
        while pgrep -x nginx > /dev/null; do
          echo "Waiting for nginx workers to finish..."
          sleep 1
        done
        
        echo "Nginx shutdown complete"
---
# Java Spring Boot 优雅关闭
preStop:
  exec:
    command:
      - /bin/sh
      - -c
      - |
        # Spring Boot Actuator 优雅关闭
        curl -X POST http://localhost:8081/actuator/shutdown
        
        # 等待进程退出
        while pgrep -f "java.*spring" > /dev/null; do
          sleep 1
        done
```

### 2. 从负载均衡器移除

```yaml
preStop:
  exec:
    command:
      - /bin/sh
      - -c
      - |
        # 从 ALB/NLB 注销
        curl -X DELETE "http://lb-controller/api/targets/${POD_IP}"
        
        # 等待负载均衡器健康检查失败
        # 这确保不会有新流量进入
        sleep 10
        
        # 等待现有连接完成
        sleep 20
```

### 3. 数据持久化

```yaml
# Redis 数据持久化
preStop:
  exec:
    command:
      - /bin/sh
      - -c
      - |
        echo "Starting Redis shutdown..."
        
        # 触发 RDB 快照
        redis-cli bgsave
        
        # 等待后台保存完成
        while [ "$(redis-cli lastsave)" == "$(redis-cli lastsave)" ]; do
          sleep 1
        done
        
        # 正常关闭 Redis
        redis-cli shutdown save
---
# 数据库连接池清理
preStop:
  exec:
    command:
      - /bin/sh
      - -c
      - |
        # 停止接收新请求
        touch /tmp/shutdown
        
        # 等待正在执行的事务完成
        sleep 10
        
        # 关闭数据库连接
        curl -X POST http://localhost:8080/admin/drain-connections
```

### 4. 消息队列消费者优雅关闭

```yaml
preStop:
  exec:
    command:
      - /bin/sh
      - -c
      - |
        # 停止从队列拉取新消息
        curl -X POST http://localhost:8080/admin/stop-consuming
        
        # 等待正在处理的消息完成
        while curl -s http://localhost:8080/admin/in-flight-count | grep -v "^0$"; do
          echo "Waiting for in-flight messages..."
          sleep 2
        done
        
        # 提交所有已处理消息的 offset
        curl -X POST http://localhost:8080/admin/commit-offsets
```

### 5. 从服务发现注销

```yaml
preStop:
  exec:
    command:
      - /bin/sh
      - -c
      - |
        # Consul 注销
        curl -X PUT "http://consul:8500/v1/agent/service/deregister/${POD_NAME}"
        
        # 等待其他服务更新缓存
        sleep 5
        
        # Eureka 注销
        curl -X DELETE "http://eureka:8761/eureka/apps/${APP_NAME}/${POD_NAME}"
        
        # 等待注销生效
        sleep 10
```

## 与探针配合使用

```yaml
# probes-with-lifecycle.yaml
apiVersion: v1
kind: Pod
metadata:
  name: full-lifecycle-example
spec:
  terminationGracePeriodSeconds: 60
  
  containers:
    - name: app
      image: myapp:v1.0
      ports:
        - containerPort: 8080
          name: http
      
      # ========== 探针配置 ==========
      
      # 启动探针: 处理慢启动应用
      # - 应用启动期间，liveness 和 readiness 探针不会执行
      # - 失败次数超过 failureThreshold 后容器重启
      startupProbe:
        httpGet:
          path: /healthz
          port: http
        initialDelaySeconds: 0
        periodSeconds: 5
        failureThreshold: 60  # 最多等待 5 * 60 = 300s
        successThreshold: 1
      
      # 就绪探针: 控制是否接收流量
      # - 失败时从 Service Endpoints 移除
      # - preStop 执行后应返回失败，加速流量切换
      readinessProbe:
        httpGet:
          path: /ready
          port: http
        initialDelaySeconds: 0
        periodSeconds: 5
        failureThreshold: 3
        successThreshold: 1
      
      # 存活探针: 检测应用是否存活
      # - 失败时容器重启
      # - 不应与优雅关闭冲突
      livenessProbe:
        httpGet:
          path: /healthz
          port: http
        initialDelaySeconds: 0
        periodSeconds: 10
        failureThreshold: 3
        successThreshold: 1
      
      # ========== 生命周期钩子 ==========
      
      lifecycle:
        postStart:
          exec:
            command:
              - /bin/sh
              - -c
              - |
                # 等待应用内部初始化
                until curl -sf http://localhost:8080/internal/ready; do
                  sleep 1
                done
        
        preStop:
          exec:
            command:
              - /bin/sh
              - -c
              - |
                # 标记应用为 not ready
                # 这会使 readinessProbe 失败，从 Endpoints 移除
                touch /tmp/shutdown
                
                # 等待负载均衡器更新
                sleep 10
                
                # 等待现有请求完成
                sleep 15
      
      # ========== 应用配置 ==========
      
      env:
        - name: SHUTDOWN_MARKER_FILE
          value: "/tmp/shutdown"
      
      # 应用 /ready 端点实现:
      # if os.path.exists(os.environ['SHUTDOWN_MARKER_FILE']):
      #     return Response(status=503)
      # return Response(status=200)
```

## 调试钩子执行

```bash
# ==================== 查看事件 ====================

# 查看 Pod 事件 (包括钩子执行情况)
kubectl describe pod <pod-name>

# 过滤 lifecycle 相关事件
kubectl describe pod <pod-name> | grep -A 5 "Events:"

# ==================== 查看容器状态 ====================

# 查看容器当前状态
kubectl get pod <pod-name> -o jsonpath='{.status.containerStatuses[*].state}'

# 查看上一次状态 (如果重启过)
kubectl get pod <pod-name> -o jsonpath='{.status.containerStatuses[*].lastState}'

# 查看终止原因
kubectl get pod <pod-name> -o jsonpath='{.status.containerStatuses[*].lastState.terminated}'

# ==================== 实时监控 ====================

# 监控 Pod 状态变化
kubectl get pod <pod-name> -w

# 监控事件
kubectl get events --field-selector involvedObject.name=<pod-name> -w

# ==================== 日志检查 ====================

# 查看当前容器日志
kubectl logs <pod-name>

# 查看上一个容器的日志 (如果重启过)
kubectl logs <pod-name> --previous

# ==================== 调试脚本 ====================

# 测试 postStart 钩子
kubectl exec <pod-name> -- /bin/sh -c "echo 'Testing postStart command'"

# 测试 preStop 钩子
kubectl exec <pod-name> -- /bin/sh -c "/scripts/prestop.sh"

# ==================== 常见问题诊断 ====================

# PostStart 失败导致 CrashLoopBackOff
kubectl describe pod <pod-name> | grep -A 10 "State:"

# PreStop 执行时间过长
kubectl get pod <pod-name> -o jsonpath='{.spec.terminationGracePeriodSeconds}'
```

## 生命周期钩子故障排查

| 问题 | 症状 | 排查方法 | 解决方案 |
|-----|------|---------|---------|
| PostStart 超时 | 容器不进入 Running | 检查事件和容器状态 | 优化脚本或增加超时 |
| PostStart 失败 | CrashLoopBackOff | 查看 lastState.terminated.reason | 修复脚本错误 |
| PreStop 被忽略 | 容器立即终止 | 检查宽限期配置 | 增加 terminationGracePeriodSeconds |
| PreStop 执行不完整 | 清理任务未完成 | 检查宽限期是否足够 | PreStop 时间 < 宽限期 |
| 钩子与主进程冲突 | 应用行为异常 | 检查并发处理逻辑 | 正确处理 SIGTERM |

## 版本变更记录

| 版本 | 变更内容 |
|------|---------|
| v1.25 | tcpSocket 钩子支持 |
| v1.27 | Pod 生命周期改进 |
| v1.29 | sleep 钩子 GA |
| v1.30 | 钩子超时配置改进 |
| v1.31 | 钩子执行日志增强 |

## 最佳实践清单

- [ ] **PostStart 快速完成**: 避免长时间阻塞容器启动
- [ ] **PreStop 时间规划**: PreStop + 应用关闭时间 < terminationGracePeriodSeconds
- [ ] **幂等操作**: 钩子脚本应支持重复执行
- [ ] **错误处理**: 脚本内部处理错误，避免意外退出
- [ ] **日志记录**: 关键操作写入日志便于排查
- [ ] **配合探针**: readinessProbe 配合 PreStop 加速流量切换
- [ ] **SIGTERM 处理**: 应用正确响应 SIGTERM 信号
- [ ] **避免网络依赖**: PreStop 中避免强依赖外部服务

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
