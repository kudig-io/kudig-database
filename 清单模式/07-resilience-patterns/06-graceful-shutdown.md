---
title: 优雅关闭模式
description: preStop Hook + terminationGracePeriodSeconds 实现零停机关闭
summary: 使用 preStop Hook、SIGTERM 处理、连接排空和 terminationGracePeriodSeconds 实现应用优雅关闭
category: manifests-patterns
tags:
- k8s
- manifests
- reliability
- graceful-shutdown
- prestop
- lifecycle
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 平台工程师
- SRE
- 开发工程师
estimated_read_time: 10min
intent_queries:
- Kubernetes 优雅关闭
- preStop Hook 配置
- 零停机滚动更新
trigger_keywords:
- graceful-shutdown
- prestop
- sigterm
- terminationgraceperiodseconds
- lifecycle
prerequisites:
- k8s-pod-basics
- deployment-basics
authors:
- name: KUDIG Team
  role: contributor
---

# 优雅关闭模式

## 1. Pod 终止流程

```
1. kubelet 发送 SIGTERM 信号
2. preStop Hook 执行（如果有）
3. 等待 terminationGracePeriodSeconds（默认 30s）
4. 发送 SIGKILL（强制终止）
5. 容器被删除
```

> ⚠️ 关键问题：SIGTERM 发送后，Pod **立即**从 Endpoints 摘除，但 kube-proxy 更新 iptables/ipvs 规则有**延迟**。在规则更新前，旧流量仍会发到正在终止的 Pod。

## 2. preStop Hook 解决流量丢失

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  template:
    spec:
      terminationGracePeriodSeconds: 60   # 总宽限期
      containers:
        - name: app
          image: registry.example.com/app:v1.0.0
          lifecycle:
            preStop:
              exec:
                command:
                  - /bin/sh
                  - -c
                  - "sleep 15"   # 等待 kube-proxy 更新规则
          # 同时应用需要处理 SIGTERM
```

**流程**：
```
SIGTERM → preStop: sleep 15 → 应用开始排空连接
  ↓ 15s（kube-proxy 已更新规则）
应用继续处理存量请求
  ↓ 45s 内完成排空
正常退出
```

## 3. 完整优雅关闭配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-server
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 0           # 不允许减少可用副本
      maxSurge: 1                 # 最多多 1 个副本
  template:
    spec:
      terminationGracePeriodSeconds: 60
      containers:
        - name: api
          image: registry.example.com/api:v1.0.0
          ports:
            - containerPort: 8080
          lifecycle:
            preStop:
              exec:
                command:
                  - /bin/sh
                  - -c
                  - |
                    echo "Graceful shutdown initiated..."
                    # 调用应用的 drain 端点
                    curl -X POST http://localhost:8080/drain
                    # 等待现有请求完成
                    sleep 15
                    # 从注册中心注销
                    curl -X DELETE http://consul:8500/v1/agent/service/deregister/api-server
                    echo "Shutdown complete"
          livenessProbe:
            httpGet:
              path: /health/live
              port: 8080
          readinessProbe:
            httpGet:
              path: /health/ready
              port: 8080
```

## 4. 应用层 SIGTERM 处理

### 4.1 Go 示例

```go
package main

import (
    "context"
    "log"
    "net/http"
    "os"
    "os/signal"
    "sync/atomic"
    "syscall"
    "time"
)

var ready atomic.Bool

func main() {
    ready.Store(true)

    server := &http.Server{Addr: ":8080"}

    // 设置 /ready 端点
    http.HandleFunc("/health/ready", func(w http.ResponseWriter, r *http.Request) {
        if ready.Load() {
            w.WriteHeader(200)
        } else {
            w.WriteHeader(503) // 排空中返回 503
        }
    })

    // 监听 SIGTERM
    quit := make(chan os.Signal, 1)
    signal.Notify(quit, syscall.SIGTERM, syscall.SIGINT)

    go func() {
        if err := server.ListenAndServe(); err != nil && err != http.ErrServerClosed {
            log.Fatalf("Server failed: %v", err)
        }
    }()

    <-quit
    log.Println("SIGTERM received, shutting down...")

    // 标记为不健康
    ready.Store(false)

    // 优雅关闭，等待 30s
    ctx, cancel := context.WithTimeout(context.Background(), 30*time.Second)
    defer cancel()

    if err := server.Shutdown(ctx); err != nil {
        log.Fatalf("Forced shutdown: %v", err)
    }
    log.Println("Server exited properly")
}
```

### 4.2 Python/Flask 示例

```python
import signal
import threading
from flask import Flask, jsonify

app = Flask(__name__)
shutting_down = False
active_requests = 0
request_lock = threading.Lock()

@app.before_request
def before_request():
    if shutting_down:
        return jsonify({"error": "shutting down"}), 503
    global active_requests
    with request_lock:
        active_requests += 1

@app.after_request
def after_request(response):
    global active_requests
    with request_lock:
        active_requests -= 1
    return response

@app.route('/health/ready')
def ready():
    if shutting_down:
        return jsonify({"status": "draining"}), 503
    return jsonify({"status": "ready"}), 200

def graceful_shutdown(signum, frame):
    global shutting_down
    shutting_down = True
    print("SIGTERM received, draining connections...")

    # 等待活跃请求完成
    import time
    while active_requests > 0:
        time.sleep(1)
        print(f"Waiting for {active_requests} active requests...")

    print("All requests completed, exiting")
    import os
    os._exit(0)

signal.signal(signal.SIGTERM, graceful_shutdown)
```

## 5. 不同语言的 terminationGracePeriodSeconds

| 应用类型 | 推荐值 | 说明 |
|----------|--------|------|
| **Web API** | 60s | 有活跃请求需排空 |
| **消息消费者** | 120s | 处理中的消息需完成 |
| **数据库** | 300s | 有状态，需刷盘 |
| **ML 推理** | 60s | 长请求需完成 |
| **Sidecar** | 同主容器 | 自动跟随主容器 |

## 6. 连接排空（应用层）

```yaml
# 使用 Nginx 作为前端代理时
apiVersion: v1
kind: ConfigMap
metadata:
  name: nginx-config
data:
  nginx.conf: |
    upstream backend {
      server backend:8080;
      # 健康检查（主动）
      health_check interval=5s fails=2 passes=1;
    }
    server {
      location / {
        proxy_pass http://backend;
        # 当后端返回 503 时自动重试到其他 Pod
        proxy_next_upstream error timeout http_503;
        proxy_next_upstream_tries 3;
      }
    }
```

## 7. 有状态应用特殊处理

```yaml
# 数据库关闭前刷盘
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgresql
spec:
  template:
    spec:
      terminationGracePeriodSeconds: 300
      containers:
        - name: postgres
          image: postgres:16
          lifecycle:
            preStop:
              exec:
                command:
                  - /bin/sh
                  - -c
                  - |
                    # 停止接受新连接
                    psql -U postgres -c "SELECT pg_switch_wal();"
                    # 等待复制同步
                    psql -U postgres -c "SELECT pg_wal_replay_wait('syncrep');"
                    # 正常关闭
                    pg_ctl -D /var/lib/postgresql/data stop -m fast
```

## 8. 生产实践

| 实践 | 说明 |
|------|------|
| 设置 `terminationGracePeriodSeconds` | 至少 60s |
| 使用 `preStop: sleep` | 等待 kube-proxy 规则更新（5-15s） |
| 应用处理 SIGTERM | 停止接收新请求，完成存量 |
| `readinessProbe` 返回 503 | 摘除流量 |
| `maxUnavailable: 0` | 滚动更新时不减少可用副本 |
| 测试关闭流程 | 验证零请求丢失 |

## 9. 验证

```bash
# 🟢 低风险：关闭测试
# 模拟 Pod 终止并检查日志
kubectl delete pod api-server-xxx --grace-period=60

# 观察关闭过程
kubectl logs api-server-xxx -f --tail=50

# 检查是否有 503 错误（在网关层）
# 应该看到 0 个请求丢失
```

## 10. 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| 请求 502/503 | 未用 preStop sleep | 添加 `preStop: sleep 15` |
| Pod 被 SIGKILL | `terminationGracePeriodSeconds` 太短 | 增加宽限期 |
| 关闭时连接断开 | 应用未处理 SIGTERM | 添加信号处理 |
| 滚动更新有错误 | `maxUnavailable` 设置不当 | 设为 0 |

## Related

- [[清单模式/07-resilience-patterns/05-health-probe-patterns|健康探针设计]]
- [[清单模式/07-resilience-patterns/01-pdb-patterns|PDB 模式]]

## See Also

- [Pod 终止流程](https://kubernetes.io/docs/concepts/workloads/pods/pod-lifecycle/#pod-termination)
- [Graceful Shutdown 最佳实践](https://learnk8s.io/graceful-shutdown)

<!-- risk-assessed -->
