---
title: 健康探针设计模式
description: Liveness/Readiness/Startup Probe 配置最佳实践
summary: Kubernetes 三种健康探针的设计原则、参数调优及不同场景（HTTP/TCP/Exec/gRPC）的配置模式
category: manifests-patterns
tags:
- k8s
- manifests
- reliability
- health-probe
- liveness
- readiness
- startup
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
- 健康探针如何配置
- Liveness Readiness 区别
- Startup Probe 配置
trigger_keywords:
- liveness
- readiness
- startup
- probe
- health-check
prerequisites:
- k8s-pod-basics
authors:
- name: KUDIG Team
  role: contributor
---

# 健康探针设计模式

## 1. 三种探针对比

| 探针 | 作用 | 失败后果 | 适用场景 |
|------|------|----------|----------|
| **Startup** | 应用是否已启动 | 阻止其他探针 | 慢启动应用 |
| **Liveness** | 应用是否健康 | 重启 Pod | 死锁检测 |
| **Readiness** | 是否可接收流量 | 从 Service 摘除 | 临时不可用 |

## 2. 探针执行顺序

```
Pod 启动
  ↓
Startup Probe（直到成功）
  ↓ 成功
Liveness + Readiness 同时运行
  ↓
Readiness 成功 → 加入 Service Endpoints
```

## 3. HTTP 探针配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  template:
    spec:
      containers:
        - name: app
          image: registry.example.com/app:v1.0.0
          ports:
            - containerPort: 8080
          startupProbe:
            httpGet:
              path: /health/startup
              port: 8080
            initialDelaySeconds: 5      # 容器启动后等待
            periodSeconds: 5
            timeoutSeconds: 3
            failureThreshold: 30         # 最多 30 次失败（5*30=150s 启动时间）
          livenessProbe:
            httpGet:
              path: /health/live
              port: 8080
            periodSeconds: 10
            timeoutSeconds: 3
            failureThreshold: 3          # 3 次失败（30s）才重启
            successThreshold: 1
          readinessProbe:
            httpGet:
              path: /health/ready
              port: 8080
            periodSeconds: 5
            timeoutSeconds: 3
            failureThreshold: 2          # 2 次失败（10s）就摘除流量
            successThreshold: 1
```

## 4. 端点设计

```python
# 推荐的三个独立端点
from flask import Flask, jsonify
import threading
import time

app = Flask(__name__)
started_at = time.time()
ready = False

@app.route('/health/startup')
def startup():
    """检查应用是否完成初始化"""
    if time.time() - started_at < 30:  # 需要 30s 初始化
        return jsonify({"status": "starting"}), 503
    return jsonify({"status": "started"}), 200

@app.route('/health/live')
def live():
    """检查进程是否存活（不做依赖检查）"""
    return jsonify({"status": "alive"}), 200

@app.route('/health/ready')
def ready():
    """检查是否可以接收流量（检查依赖）"""
    if not ready:
        return jsonify({"status": "not ready"}), 503
    # 检查数据库连接
    if not check_db_connection():
        return jsonify({"status": "db unavailable"}), 503
    return jsonify({"status": "ready"}), 200
```

## 5. TCP 探针（非 HTTP 应用）

```yaml
livenessProbe:
  tcpSocket:
    port: 3306
  periodSeconds: 10
  failureThreshold: 3
readinessProbe:
  tcpSocket:
    port: 3306
  periodSeconds: 5
  failureThreshold: 2
```

## 6. gRPC 探针（v1.27+）

```yaml
livenessProbe:
  grpc:
    port: 9090
    service: myapp.Health    # gRPC health check service
  periodSeconds: 10
  failureThreshold: 3
readinessProbe:
  grpc:
    port: 9090
  periodSeconds: 5
```

## 7. Exec 探针

```yaml
livenessProbe:
  exec:
    command:
      - /bin/sh
      - -c
      - "ps aux | grep my-process | grep -v grep"
  periodSeconds: 30
  failureThreshold: 3
```

## 8. 参数调优指南

```yaml
# 通用参数说明
startupProbe:
  initialDelaySeconds: 0        # 容器启动后立即开始
  periodSeconds: 10             # 每 10 秒检查一次
  timeoutSeconds: 3             # 超时时间
  successThreshold: 1           # 成功 1 次即通过
  failureThreshold: 30          # 失败 30 次（300s）才放弃

# 启动时间计算: periodSeconds × failureThreshold = 最大容忍启动时间
```

### 8.1 按应用类型调优

| 应用类型 | Startup | Liveness | Readiness |
|----------|---------|----------|-----------|
| **Web API** | 30s | 10s/3次 | 5s/2次 |
| **数据库** | 300s | 30s/5次 | 10s/3次 |
| **ML 推理** | 300s | 30s/3次 | 10s/3次 |
| **消息消费者** | 60s | 15s/3次 | 5s/2次 |

## 9. 常见反模式

### 9.1 Liveness 检查外部依赖

```yaml
# ❌ 错误：Liveness 检查数据库
livenessProbe:
  httpGet:
    path: /health?check=db    # 数据库故障会导致 Pod 被重启
# ✅ 正确：Liveness 只检查进程本身
livenessProbe:
  httpGet:
    path: /health/live        # 不检查依赖
```

### 9.2 缺少 Startup Probe

```yaml
# ❌ 错误：Java 应用没有 Startup Probe
livenessProbe:
  httpGet:
    path: /health
  initialDelaySeconds: 120    # 硬编码等待时间
  failureThreshold: 3
# ✅ 正确：使用 Startup Probe
startupProbe:
  httpGet:
    path: /health/startup
  failureThreshold: 60         # 自动适应启动时间
```

### 9.3 Readiness 与 Liveness 用同一端点

```yaml
# ❌ 不推荐：两者用同一路径
livenessProbe:
  httpGet:
    path: /health
readinessProbe:
  httpGet:
    path: /health             # 无法区分"临时不可用"和"需要重启"
# ✅ 正确：分离端点
```

## 10. 生产实践

| 实践 | 说明 |
|------|------|
| 使用独立的三个端点 | `/health/startup`, `/health/live`, `/health/ready` |
| Liveness 不检查依赖 | 避免级联重启 |
| Readiness 检查依赖 | DB/Cache 不可用时摘除流量 |
| 慢启动应用用 Startup | Java/Python ML 应用 |
| 合理设置 `timeoutSeconds` | 避免偶发延迟误判 |
| 监控探针失败 | 配合告警 |

## Related

- [[03-清单模式/08-韧性模式/06-graceful-shutdown|优雅关闭]]
- [[03-清单模式/08-韧性模式/01-pdb-patterns|PDB 模式]]

## See Also

- [Configure Probes](https://kubernetes.io/docs/tasks/configure-pod-container/configure-liveness-readiness-startup-probes/)
- [gRPC Health Checking](https://kubernetes.io/docs/tasks/configure-pod-container/configure-liveness-readiness-startup-probes/#define-a-grpc-liveness-probe)

<!-- risk-assessed -->
