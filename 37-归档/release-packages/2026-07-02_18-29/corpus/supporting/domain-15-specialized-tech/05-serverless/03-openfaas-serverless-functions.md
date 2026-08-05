---
title: OpenFaaS 无服务器函数平台
description: 'OpenFaaS on Kubernetes：组件安装、函数模板、自动扩缩策略与 Secret 管理'
summary: 'OpenFaaS on Kubernetes：组件安装、函数模板、自动扩缩策略与 Secret 管理'
category: specialized-tech
tags:
- openfaas
- serverless
- functions
- faasd
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- OpenFaaS 是什么
- 如何在 Kubernetes 上安装 OpenFaaS
- 如何开发 OpenFaaS 函数
trigger_keywords:
- openfaas
- faasd
- serverless
- functions
- faas-cli
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# OpenFaaS 无服务器函数平台

## 1. OpenFaaS 架构

OpenFaaS 提供两种部署形态：

| 形态 | 特点 | 适用场景 |
|------|------|----------|
| **faasd** | 轻量级，单节点，使用 containerd | 边缘、IoT、开发 |
| **OpenFaaS Pro** | 完整 K8s 集群，生产级 | 生产、多租户 |

核心组件：

```
API Gateway → Function Watcher → Kubernetes API → Pod(Function)
     │              │
     │              └── Autoscaler (根据 RPS/扩缩容)
     └── UI / CLI (faas-cli)
```

## 2. 安装部署

### 2.1 OpenFaaS Pro on Kubernetes

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建命名空间
kubectl apply -f https://raw.githubusercontent.com/openfaas/faas-netes/master/namespaces.yml

# 添加 Helm 仓库
helm repo add openfaas https://openfaas.github.io/faas-netes/
helm repo update

# 安装 OpenFaaS
helm upgrade openfaas --install openfaas/openfaas \
  --namespace openfaas \
  --set functionNamespace=openfaas-fn \
  --set generateBasicAuth=true \
  --set gateway.replicas=2 \
  --set gateway.directFunctions=false \
  --set queueWorker.replicas=2 \
  --set queueWorker.maxInflight=1 \
  --set alertmanager.enabled=true \
  --set prometheus.enabled=true \
  --set operator.create=true

# 获取初始密码
PASSWORD=$(kubectl -n openfaas get secret basic-auth -o jsonpath="{.data.basic-auth-password}" | base64 --decode)
echo "OpenFaaS password: $PASSWORD"

# 安装 CLI
curl -sSL https://cli.openfaas.com | sh
faas-cli login --password-stdin <<< "$PASSWORD"
```
### 2.2 faasd 安装（轻量级）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在单节点上安装
curl -sSL https://get.faasd.sh | sh

# 或手动安装
faasd install
systemctl status faasd
```
### 2.3 高可用配置

```yaml
# values-ha.yaml
gateway:
  replicas: 3
  resources:
    requests:
      memory: 256Mi
      cpu: 100m
    limits:
      memory: 512Mi
      cpu: 500m
  affinity:
    podAntiAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
        - weight: 100
          podAffinityTerm:
            labelSelector:
              matchLabels:
                app: gateway
            topologyKey: kubernetes.io/hostname

queueWorker:
  replicas: 3
  maxInflight: 5

alertmanager:
  create: true
  replicas: 1

prometheus:
  create: true
  replicas: 1
  resources:
    requests:
      memory: 512Mi
```

## 3. 函数开发模板

### 3.1 Python 模板

```python
# handler.py
import json
import os

def handle(event, context):
    """主处理函数"""
    body = event.body
    if isinstance(body, bytes):
        body = body.decode("utf-8")

    try:
        data = json.loads(body) if body else {}
    except json.JSONDecodeError:
        data = {"raw": body}

    # 业务逻辑
    result = {
        "message": "Processed",
        "input": data,
        "env": os.environ.get("ENVIRONMENT", "unknown")
    }

    return {
        "statusCode": 200,
        "body": json.dumps(result)
    }
```

```yaml
# stack.yml
provider:
  name: openfaas
  gateway: http://127.0.0.1:8080

functions:
  python-processor:
    lang: python3-http
    handler: ./python-processor
    image: my-registry/python-processor:latest
    environment:
      ENVIRONMENT: production
      write_debug: "true"
    limits:
      cpu: "500m"
      memory: "256Mi"
    requests:
      cpu: "100m"
      memory: "128Mi"
    min_replicas: 1
    max_replicas: 10
    scaling_factor: 50
```

### 3.2 Go 模板

```go
// handler.go
package function

import (
    "encoding/json"
    "fmt"
    "net/http"
)

type Request struct {
    Name    string `json:"name"`
    Message string `json:"message"`
}

type Response struct {
    Status  string `json:"status"`
    Greeting string `json:"greeting"`
}

func Handle(w http.ResponseWriter, r *http.Request) {
    var req Request
    if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
        http.Error(w, err.Error(), http.StatusBadRequest)
        return
    }

    resp := Response{
        Status:   "ok",
        Greeting: fmt.Sprintf("Hello, %s! %s", req.Name, req.Message),
    }

    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(resp)
}
```

### 3.3 Node.js 模板

```javascript
// handler.js
"use strict";

module.exports = async (event, context) => {
    const body = event.body || {};
    const name = body.name || "World";

    return context
        .status(200)
        .succeed({
            message: `Hello, ${name}!`,
            timestamp: new Date().toISOString(),
        });
};
```

### 3.4 Dockerfile 自定义模板

```dockerfile
# template/python3-http-custom/Dockerfile
FROM python:3.11-slim

WORKDIR /home/app
COPY index.py requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

COPY function/ ./function/
RUN pip install --no-cache-dir -r function/requirements.txt

ENV fprocess="python index.py"
EXPOSE 8080

CMD ["fwatchdog"]
```

## 4. 自动扩缩策略

### 4.1 基于 RPS 的扩缩容

```yaml
functions:
  api-handler:
    lang: python3-http
    handler: ./api-handler
    image: my-registry/api-handler:latest
    environment:
      max_inflight: "5"
    min_replicas: 2
    max_replicas: 50
    # 每 50 个 RPS 增加一个副本
    scaling_factor: 50
    # 扩缩容配置（通过 annotations）
    annotations:
      com.openfaas.scaling.type: rps
      com.openfaas.scaling.minScale: "2"
      com.openfaas.scaling.maxScale: "50"
```

### 4.2 基于 CPU 的扩缩容

```yaml
functions:
  cpu-intensive:
    lang: go
    handler: ./cpu-intensive
    image: my-registry/cpu-intensive:latest
    limits:
      cpu: "2"
      memory: "1Gi"
    requests:
      cpu: "500m"
      memory: "256Mi"
    min_replicas: 1
    max_replicas: 20
    annotations:
      com.openfaas.scaling.type: cpu
      com.openfaas.scaling.target: "80"    # 目标 CPU 利用率
```

### 4.3 HPA 自定义配置

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: python-processor
  namespace: openfaas-fn
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: python-processor
  minReplicas: 2
  maxReplicas: 100
  metrics:
    - type: Pods
      pods:
        metric:
          name: http_requests_per_second
        target:
          type: AverageValue
          averageValue: "50"
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 30
      policies:
        - type: Pods
          value: 10
          periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
        - type: Percent
          value: 10
          periodSeconds: 60
```

## 5. Secret 管理

### 5.1 OpenFaaS Secret

```bash
# 创建 Secret
faas-cli secret create db-password --from-literal="P@ssw0rd123"

# 列出 Secrets
faas-cli secret list

# 更新 Secret
faas-cli secret update db-password --from-literal="N3wP@ss"

# 删除 Secret
faas-cli secret remove db-password
```

### 5.2 在函数中使用 Secret

```yaml
functions:
  db-connector:
    lang: python3-http
    handler: ./db-connector
    image: my-registry/db-connector:latest
    secrets:
      - db-password
      - api-key
    environment:
      DB_HOST: postgres.production
      DB_PORT: "5432"
```

```python
# handler.py
import os

def handle(event, context):
    # Secret 通过文件挂载到 /var/openfaas/secrets/
    secret_path = "/var/openfaas/secrets/db-password"
    with open(secret_path, "r") as f:
        db_password = f.read().strip()

    # 使用 Secret 连接数据库
    # ...
```

### 5.3 与 External Secrets 集成

```yaml
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: openfaas-db-creds
  namespace: openfaas
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: vault-backend
    kind: ClusterSecretStore
  target:
    name: db-password
    creationPolicy: Owner
  data:
    - secretKey: password
      remoteRef:
        key: secret/data/database
        property: password
```

## 6. 日志收集

### 6.1 函数日志查询

```bash
# 查看函数日志（CLI）
faas-cli logs python-processor --since 1h

# 实时跟踪日志
faas-cli logs python-processor -f

# 通过 API 查询
curl -s "http://gateway.openfaas:8080/system/logs?name=python-processor&since=1h"
```

### 6.2 集成 Loki

```yaml
# 函数日志收集到 Loki
functions:
  api-handler:
    lang: python3-http
    handler: ./api-handler
    image: my-registry/api-handler:latest
    annotations:
      com.openfaas.logging: "json"
    labels:
      com.openfaas.logging.enabled: "true"
```

### 6.3 结构化日志

```python
import json
import sys
import time

def handle(event, context):
    log_entry = {
        "timestamp": time.time(),
        "level": "info",
        "function": "api-handler",
        "request_id": event.headers.get("X-Call-Id", "unknown"),
        "message": "Request processed",
        "duration_ms": 42,
    }
    print(json.dumps(log_entry), file=sys.stderr)
    return {"status": "ok"}
```

## 7. 网络与 Ingress 配置

### 7.1 暴露函数

```yaml
# 通过 Ingress 暴露
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: api-handler-ingress
  namespace: openfaas-fn
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
spec:
  rules:
    - host: api.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service: api-handler
              port:
                number: 8080
```

### 7.2 函数间调用

```python
import requests
import os

def handle(event, context):
    gateway_url = os.getenv("OPENFAAS_GATEWAY", "http://gateway.openfaas:8080")

    # 同步调用另一个函数
    resp = requests.post(
        f"{gateway.openfaas}/function/data-enricher",
        json=event.json,
        headers={"X-Call-Id": event.headers.get("X-Call-Id", "")},
        timeout=30
    )

    enriched = resp.json()
    return {"statusCode": 200, "body": enriched}
```

## 8. 生产部署最佳实践

```yaml
# stack.yml 生产模板
provider:
  name: openfaas
  gateway: https://gateway.example.com

functions:
  order-processor:
    lang: python3-http
    handler: ./order-processor
    image: my-registry/order-processor:1.2.3
    environment:
      ENVIRONMENT: production
      LOG_LEVEL: info
      write_timeout: 30s
      read_timeout: 30s
      write_debug: "false"
    limits:
      cpu: "1"
      memory: "512Mi"
    requests:
      cpu: "200m"
      memory: "256Mi"
    min_replicas: 3
    max_replicas: 100
    scaling_factor: 50
    secrets:
      - db-credentials
      - redis-password
    labels:
      com.openfaas.scale.min: "3"
      com.openfaas.scale.max: "100"
      com.openfaas.scale.type: "rps"
    annotations:
      com.openfaas.health.http.path: "/health"
      com.openfaas.health.http.initialDelay: "5s"
```

---

## Related

- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-15-specialized-tech/05-serverless/01-knative-serving-deep-dive|Knative Serving 深度解析]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/domain-15-specialized-tech/05-serverless/02-knative-eventing-patterns|Knative Eventing 事件驱动模式]]

## See Also

- [OpenFaaS 官方文档](https://docs.openfaas.com/)
- [OpenFaaS 模板仓库](https://github.com/openfaas/templates)


<!-- risk-assessed -->
