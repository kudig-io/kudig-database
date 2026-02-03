# 131 - Ingress 高级路由与流量管理

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01

---

## 一、高级路由策略总览

### 1.1 路由策略类型

| 策略类型 | 英文 | 说明 | 适用场景 |
|---------|------|------|---------|
| **基于主机名** | Host-based | 根据 HTTP Host 头分发 | 多域名托管 |
| **基于路径** | Path-based | 根据 URL 路径分发 | 微服务路由 |
| **基于 Header** | Header-based | 根据请求头分发 | A/B 测试、版本路由 |
| **基于 Cookie** | Cookie-based | 根据 Cookie 值分发 | 用户分组测试 |
| **基于权重** | Weight-based | 按比例分配流量 | 金丝雀发布 |
| **基于查询参数** | Query-based | 根据 URL 参数分发 | 功能开关 |
| **基于源 IP** | Source IP-based | 根据客户端 IP 分发 | 地域路由、灰度 |
| **正则匹配** | Regex-based | 使用正则表达式匹配 | 复杂路由规则 |

### 1.2 流量管理策略

| 策略 | 英文 | 说明 | 用途 |
|-----|------|------|------|
| **金丝雀发布** | Canary Deployment | 小比例流量测试新版本 | 降低发布风险 |
| **蓝绿部署** | Blue-Green Deployment | 两套环境快速切换 | 零停机部署 |
| **A/B 测试** | A/B Testing | 多版本对比测试 | 产品验证 |
| **流量镜像** | Traffic Mirroring | 复制流量到测试环境 | 生产验证 |
| **流量分割** | Traffic Splitting | 按规则分配流量 | 渐进式发布 |
| **熔断** | Circuit Breaking | 故障时快速失败 | 服务保护 |
| **限流** | Rate Limiting | 限制请求速率 | 过载保护 |
| **重试** | Retry | 失败时自动重试 | 提高可用性 |
| **超时** | Timeout | 限制响应等待时间 | 资源保护 |

---

## 二、金丝雀发布 (Canary Deployment)

### 2.1 金丝雀发布策略对比

| 策略 | 触发条件 | 优先级 | 适用场景 |
|-----|---------|-------|---------|
| **Header 金丝雀** | 特定请求头 | 最高 | 内部测试、指定用户 |
| **Header 值金丝雀** | 特定请求头值 | 次高 | 精确用户控制 |
| **Header 正则金丝雀** | 请求头正则匹配 | 次高 | 模式匹配 |
| **Cookie 金丝雀** | 特定 Cookie | 中 | 用户分组 |
| **权重金丝雀** | 随机比例 | 最低 | 流量百分比控制 |

### 2.2 基于权重的金丝雀

```yaml
# 稳定版本 Ingress (90% 流量)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-stable
  namespace: production
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v1
            port:
              number: 80
---
# 金丝雀版本 Ingress (10% 流量)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary
  namespace: production
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "10"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
```

### 2.3 基于 Header 的金丝雀

```yaml
# 场景1: Header 存在即路由到金丝雀
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary-header
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    # 只要存在 X-Canary 头就路由到金丝雀
    nginx.ingress.kubernetes.io/canary-by-header: "X-Canary"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
---
# 场景2: Header 值精确匹配
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary-header-value
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-by-header: "X-Test-Group"
    # X-Test-Group: beta 时路由到金丝雀
    nginx.ingress.kubernetes.io/canary-by-header-value: "beta"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
---
# 场景3: Header 正则匹配
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary-header-pattern
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-by-header: "X-User-Email"
    # 测试用户邮箱匹配正则
    nginx.ingress.kubernetes.io/canary-by-header-pattern: ".*@testing\\.example\\.com$"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
```

### 2.4 基于 Cookie 的金丝雀

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary-cookie
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    # Cookie 名称为 canary，值为 always 时路由到金丝雀
    # 值为 never 时不路由到金丝雀
    nginx.ingress.kubernetes.io/canary-by-cookie: "canary"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
```

### 2.5 组合金丝雀策略

```yaml
# 组合策略: Header > Cookie > Weight
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary-combined
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    # 1. Header 优先级最高: 测试团队成员
    nginx.ingress.kubernetes.io/canary-by-header: "X-Test-Team"
    nginx.ingress.kubernetes.io/canary-by-header-value: "true"
    # 2. Cookie 次之: 已选择体验新版的用户
    nginx.ingress.kubernetes.io/canary-by-cookie: "beta-user"
    # 3. 权重最低: 5% 随机流量
    nginx.ingress.kubernetes.io/canary-weight: "5"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-v2
            port:
              number: 80
```

### 2.6 金丝雀发布渐进策略

| 阶段 | 权重 | 持续时间 | 观察指标 | 回滚条件 |
|-----|------|---------|---------|---------|
| **阶段1** | 1% | 1小时 | 错误率、延迟 | 错误率 >1% |
| **阶段2** | 5% | 2小时 | 错误率、延迟、业务指标 | 错误率 >0.5% |
| **阶段3** | 10% | 4小时 | 全面监控 | 异常告警 |
| **阶段4** | 25% | 8小时 | 全面监控 | 异常告警 |
| **阶段5** | 50% | 12小时 | 全面监控 | 异常告警 |
| **阶段6** | 100% | - | 完成切换 | - |

```yaml
# 渐进式金丝雀发布脚本示例
# canary-rollout.sh
#!/bin/bash
WEIGHTS=(1 5 10 25 50 100)
INTERVALS=(3600 7200 14400 28800 43200 0)

for i in "${!WEIGHTS[@]}"; do
  weight=${WEIGHTS[$i]}
  interval=${INTERVALS[$i]}
  
  echo "Setting canary weight to ${weight}%"
  kubectl patch ingress app-canary -n production -p \
    "{\"metadata\":{\"annotations\":{\"nginx.ingress.kubernetes.io/canary-weight\":\"${weight}\"}}}"
  
  if [ $interval -gt 0 ]; then
    echo "Waiting ${interval} seconds for observation..."
    sleep $interval
    
    # 检查错误率 (示例)
    error_rate=$(curl -s "http://prometheus:9090/api/v1/query?query=rate(http_requests_total{status=~\"5..\",app=\"app-v2\"}[5m])" | jq '.data.result[0].value[1]')
    if (( $(echo "$error_rate > 0.01" | bc -l) )); then
      echo "Error rate too high, rolling back..."
      kubectl patch ingress app-canary -n production -p \
        '{"metadata":{"annotations":{"nginx.ingress.kubernetes.io/canary-weight":"0"}}}'
      exit 1
    fi
  fi
done

echo "Canary rollout completed successfully"
```

---

## 三、蓝绿部署 (Blue-Green Deployment)

### 3.1 蓝绿部署架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                            蓝绿部署架构                                          │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│                              ┌──────────────┐                                   │
│                              │    Ingress   │                                   │
│                              │    (切换点)   │                                   │
│                              └──────┬───────┘                                   │
│                                     │                                           │
│                    ┌────────────────┼────────────────┐                         │
│                    │                │                │                         │
│                    ▼                ▼                ▼                         │
│           ┌─────────────┐   ┌─────────────┐  ┌─────────────┐                  │
│           │ Service-Blue│   │Service-Green│  │  Service    │                  │
│           │   (v1.0)    │   │   (v1.1)    │  │ (统一入口)  │                  │
│           └──────┬──────┘   └──────┬──────┘  └──────┬──────┘                  │
│                  │                 │                │                         │
│        ┌─────────┴─────────┐      │       ┌────────┴────────┐                │
│        ▼         ▼         ▼      ▼       ▼        ▼        ▼                │
│   ┌────────┐┌────────┐┌────────┐┌────────┐┌────────┐┌────────┐┌────────┐    │
│   │Pod Blue││Pod Blue││Pod Blue││Pod Green│Pod Green│Pod Green│        │    │
│   │  (v1.0)││ (v1.0) ││ (v1.0) ││ (v1.1) ││ (v1.1) ││ (v1.1) ││        │    │
│   └────────┘└────────┘└────────┘└────────┘└────────┘└────────┘└────────┘    │
│                                                                                  │
│   ← 当前生产 (Blue) →          ← 新版本 (Green) →                              │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 蓝绿部署实现

```yaml
# Blue 环境 (当前生产)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-blue
  namespace: production
  labels:
    app: myapp
    version: blue
spec:
  replicas: 3
  selector:
    matchLabels:
      app: myapp
      version: blue
  template:
    metadata:
      labels:
        app: myapp
        version: blue
    spec:
      containers:
      - name: app
        image: myapp:v1.0
        ports:
        - containerPort: 8080
---
apiVersion: v1
kind: Service
metadata:
  name: app-blue
  namespace: production
spec:
  selector:
    app: myapp
    version: blue
  ports:
  - port: 80
    targetPort: 8080
---
# Green 环境 (新版本)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-green
  namespace: production
  labels:
    app: myapp
    version: green
spec:
  replicas: 3
  selector:
    matchLabels:
      app: myapp
      version: green
  template:
    metadata:
      labels:
        app: myapp
        version: green
    spec:
      containers:
      - name: app
        image: myapp:v1.1
        ports:
        - containerPort: 8080
---
apiVersion: v1
kind: Service
metadata:
  name: app-green
  namespace: production
spec:
  selector:
    app: myapp
    version: green
  ports:
  - port: 80
    targetPort: 8080
---
# Ingress - 指向当前活跃环境
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  namespace: production
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-blue  # 切换时改为 app-green
            port:
              number: 80
```

### 3.3 蓝绿切换脚本

```bash
#!/bin/bash
# blue-green-switch.sh

NAMESPACE="production"
INGRESS_NAME="app-ingress"
CURRENT_SERVICE=$(kubectl get ingress $INGRESS_NAME -n $NAMESPACE -o jsonpath='{.spec.rules[0].http.paths[0].backend.service.name}')

if [ "$CURRENT_SERVICE" == "app-blue" ]; then
  TARGET_SERVICE="app-green"
else
  TARGET_SERVICE="app-blue"
fi

echo "Switching from $CURRENT_SERVICE to $TARGET_SERVICE"

# 验证目标服务就绪
READY_PODS=$(kubectl get pods -n $NAMESPACE -l app=myapp,version=${TARGET_SERVICE#app-} --field-selector=status.phase=Running -o name | wc -l)
if [ "$READY_PODS" -lt 3 ]; then
  echo "Error: Target service not ready (only $READY_PODS pods running)"
  exit 1
fi

# 执行切换
kubectl patch ingress $INGRESS_NAME -n $NAMESPACE -p \
  "{\"spec\":{\"rules\":[{\"host\":\"app.example.com\",\"http\":{\"paths\":[{\"path\":\"/\",\"pathType\":\"Prefix\",\"backend\":{\"service\":{\"name\":\"$TARGET_SERVICE\",\"port\":{\"number\":80}}}}]}}]}}"

echo "Switch completed: $CURRENT_SERVICE -> $TARGET_SERVICE"
```

---

## 四、A/B 测试

### 4.1 A/B 测试策略

| 策略 | 实现方式 | 适用场景 |
|-----|---------|---------|
| **随机分流** | 权重金丝雀 | 统计显著性测试 |
| **用户 ID 分流** | Header/Cookie | 用户体验一致性 |
| **地域分流** | 源 IP 路由 | 地域功能测试 |
| **设备分流** | User-Agent Header | 平台特定测试 |
| **功能开关** | 查询参数/Header | 快速功能验证 |

### 4.2 基于用户 ID 的 A/B 测试

```yaml
# 使用自定义 Header 实现用户分流
# 前端设置: X-User-ID-Hash: <hash(user_id) % 100>

# A 组: hash < 50
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-variant-a
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-by-header: "X-AB-Group"
    nginx.ingress.kubernetes.io/canary-by-header-value: "A"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-variant-a
            port:
              number: 80
---
# B 组: hash >= 50 (默认)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-stable
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-variant-b
            port:
              number: 80
```

### 4.3 基于地域的 A/B 测试

```yaml
# 使用 GeoIP 模块或外部服务设置 X-Country Header
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-cn-variant
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-by-header: "X-Country"
    nginx.ingress.kubernetes.io/canary-by-header-value: "CN"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-cn
            port:
              number: 80
```

---

## 五、流量镜像 (Traffic Mirroring)

### 5.1 流量镜像概念

| 特性 | 说明 |
|-----|------|
| **目的** | 将生产流量复制到测试环境，验证新版本 |
| **特点** | 不影响生产响应，镜像请求独立处理 |
| **响应** | 镜像请求的响应被丢弃 |
| **用途** | 性能测试、问题复现、新版本验证 |

### 5.2 NGINX Ingress 流量镜像

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-with-mirror
  annotations:
    # 镜像流量到指定服务
    nginx.ingress.kubernetes.io/mirror-uri: "http://app-mirror.testing.svc.cluster.local"
    # 是否镜像请求体
    nginx.ingress.kubernetes.io/mirror-request-body: "on"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
```

### 5.3 使用 configuration-snippet 实现镜像

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-custom-mirror
  annotations:
    nginx.ingress.kubernetes.io/configuration-snippet: |
      # 镜像到测试环境
      mirror /mirror;
      mirror_request_body on;
    nginx.ingress.kubernetes.io/server-snippet: |
      location = /mirror {
        internal;
        proxy_pass http://app-test.testing.svc.cluster.local$request_uri;
        proxy_set_header Host $host;
        proxy_set_header X-Original-URI $request_uri;
        proxy_set_header X-Mirror-Request "true";
      }
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
```

---

## 六、负载均衡策略

### 6.1 负载均衡算法对比

| 算法 | 英文 | 说明 | 适用场景 | NGINX Ingress 支持 |
|-----|------|------|---------|-------------------|
| **轮询** | Round Robin | 依次分配请求 | 后端性能均衡 | ✅ 默认 |
| **加权轮询** | Weighted Round Robin | 按权重分配 | 后端性能不均 | ✅ |
| **最少连接** | Least Connections | 分配给连接最少的后端 | 长连接场景 | ✅ |
| **IP 哈希** | IP Hash | 相同 IP 到相同后端 | 会话保持 | ✅ |
| **EWMA** | Exponentially Weighted Moving Average | 指数加权移动平均 | 动态负载 | ✅ |
| **随机** | Random | 随机选择后端 | 简单均衡 | ❌ |
| **最快响应** | Fastest Response | 分配给响应最快的后端 | 延迟敏感 | ❌ |

### 6.2 负载均衡配置

```yaml
# 轮询 (默认)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-round-robin
  annotations:
    nginx.ingress.kubernetes.io/load-balance: "round_robin"
spec:
  # ...

---
# 最少连接
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-least-conn
  annotations:
    nginx.ingress.kubernetes.io/load-balance: "least_conn"
spec:
  # ...

---
# EWMA (推荐用于动态负载)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ewma
  annotations:
    nginx.ingress.kubernetes.io/load-balance: "ewma"
spec:
  # ...

---
# IP 哈希 (会话保持)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ip-hash
  annotations:
    nginx.ingress.kubernetes.io/upstream-hash-by: "$remote_addr"
spec:
  # ...

---
# 基于请求 URI 的一致性哈希
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-uri-hash
  annotations:
    nginx.ingress.kubernetes.io/upstream-hash-by: "$request_uri"
spec:
  # ...

---
# 基于 Header 的一致性哈希
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-header-hash
  annotations:
    nginx.ingress.kubernetes.io/upstream-hash-by: "$http_x_user_id"
spec:
  # ...
```

### 6.3 会话亲和性配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-sticky-session
  annotations:
    # 启用 Cookie 会话亲和
    nginx.ingress.kubernetes.io/affinity: "cookie"
    # 亲和模式: balanced (默认，故障时重新分配) / persistent (严格保持)
    nginx.ingress.kubernetes.io/affinity-mode: "persistent"
    # Cookie 配置
    nginx.ingress.kubernetes.io/session-cookie-name: "SERVERID"
    nginx.ingress.kubernetes.io/session-cookie-max-age: "3600"
    nginx.ingress.kubernetes.io/session-cookie-path: "/"
    nginx.ingress.kubernetes.io/session-cookie-samesite: "Strict"
    nginx.ingress.kubernetes.io/session-cookie-change-on-failure: "true"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: stateful-app
            port:
              number: 80
```

---

## 七、重试与超时配置

### 7.1 超时配置详解

| 超时类型 | 注解 | 默认值 | 说明 |
|---------|------|-------|------|
| **连接超时** | `proxy-connect-timeout` | 5s | 与后端建立连接的超时 |
| **读取超时** | `proxy-read-timeout` | 60s | 等待后端响应的超时 |
| **发送超时** | `proxy-send-timeout` | 60s | 向后端发送请求的超时 |

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-with-timeouts
  annotations:
    # 连接超时 10 秒
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "10"
    # 读取超时 120 秒 (长请求)
    nginx.ingress.kubernetes.io/proxy-read-timeout: "120"
    # 发送超时 60 秒
    nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
```

### 7.2 重试配置详解

| 配置项 | 注解 | 默认值 | 说明 |
|-------|------|-------|------|
| **重试条件** | `proxy-next-upstream` | error timeout | 触发重试的条件 |
| **重试次数** | `proxy-next-upstream-tries` | 3 | 最大重试次数 |
| **重试超时** | `proxy-next-upstream-timeout` | 0 | 重试总超时 (0=无限制) |

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-with-retry
  annotations:
    # 重试条件: 错误、超时、5xx 错误
    nginx.ingress.kubernetes.io/proxy-next-upstream: "error timeout http_502 http_503 http_504"
    # 最大重试 3 次
    nginx.ingress.kubernetes.io/proxy-next-upstream-tries: "3"
    # 重试总超时 30 秒
    nginx.ingress.kubernetes.io/proxy-next-upstream-timeout: "30"
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-service
            port:
              number: 80
```

### 7.3 重试条件说明

| 条件 | 说明 | 建议 |
|-----|------|------|
| `error` | 与后端建立连接或通信时发生错误 | ✅ 启用 |
| `timeout` | 与后端通信超时 | ✅ 启用 |
| `invalid_header` | 后端返回无效响应 | ⚠️ 谨慎 |
| `http_500` | 后端返回 500 | ⚠️ 谨慎 (可能加重负载) |
| `http_502` | 后端返回 502 | ✅ 启用 |
| `http_503` | 后端返回 503 | ✅ 启用 |
| `http_504` | 后端返回 504 | ✅ 启用 |
| `non_idempotent` | 允许对非幂等请求重试 | ❌ 危险 |
| `off` | 禁用重试 | - |

---

## 八、正则路径与高级匹配

### 8.1 正则路径匹配

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-regex-routing
  annotations:
    nginx.ingress.kubernetes.io/use-regex: "true"
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      # 匹配 /api/v1/*, /api/v2/*
      - path: /api/v[0-9]+/.*
        pathType: ImplementationSpecific
        backend:
          service:
            name: api-service
            port:
              number: 8080
      # 匹配用户 ID 路径 /users/123
      - path: /users/[0-9]+
        pathType: ImplementationSpecific
        backend:
          service:
            name: user-service
            port:
              number: 8080
      # 匹配静态资源
      - path: /static/.*\.(css|js|png|jpg|gif)$
        pathType: ImplementationSpecific
        backend:
          service:
            name: static-service
            port:
              number: 80
```

### 8.2 URL 重写与捕获组

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-rewrite
  annotations:
    nginx.ingress.kubernetes.io/use-regex: "true"
    # /api/v1/users -> /v1/users
    nginx.ingress.kubernetes.io/rewrite-target: /$2
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /api(/|$)(.*)
        pathType: ImplementationSpecific
        backend:
          service:
            name: api-service
            port:
              number: 8080
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-version-rewrite
  annotations:
    nginx.ingress.kubernetes.io/use-regex: "true"
    # /old/v1/path -> /new/v1/path
    nginx.ingress.kubernetes.io/rewrite-target: /new/$1/$2
spec:
  ingressClassName: nginx
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /old/(v[0-9]+)/(.*)
        pathType: ImplementationSpecific
        backend:
          service:
            name: api-service
            port:
              number: 8080
```

---

## 九、多后端与流量分割

### 9.1 使用 Service 实现流量分割

```yaml
# 主版本 Service
apiVersion: v1
kind: Service
metadata:
  name: app-service
spec:
  selector:
    app: myapp
  ports:
  - port: 80
    targetPort: 8080
---
# 通过调整 Deployment 副本数实现流量分割
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-v1
spec:
  replicas: 9  # 90% 流量
  selector:
    matchLabels:
      app: myapp
      version: v1
  template:
    metadata:
      labels:
        app: myapp
        version: v1
    spec:
      containers:
      - name: app
        image: myapp:v1
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-v2
spec:
  replicas: 1  # 10% 流量
  selector:
    matchLabels:
      app: myapp
      version: v2
  template:
    metadata:
      labels:
        app: myapp
        version: v2
    spec:
      containers:
      - name: app
        image: myapp:v2
```

### 9.2 使用 Gateway API 实现流量分割

```yaml
# Gateway API 原生支持流量分割
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: app-traffic-split
spec:
  parentRefs:
  - name: main-gateway
  hostnames:
  - "app.example.com"
  rules:
  - backendRefs:
    - name: app-v1
      port: 80
      weight: 90
    - name: app-v2
      port: 80
      weight: 10
```

---

## 十、流量管理最佳实践

### 10.1 发布策略选择

| 场景 | 推荐策略 | 理由 |
|-----|---------|------|
| **高风险变更** | 金丝雀 (1% -> 5% -> 10% -> ...) | 渐进验证，快速回滚 |
| **紧急修复** | 蓝绿部署 | 快速全量切换 |
| **功能验证** | A/B 测试 | 数据驱动决策 |
| **性能测试** | 流量镜像 | 不影响生产 |
| **日常发布** | 金丝雀 (10% -> 50% -> 100%) | 平衡效率与风险 |

### 10.2 流量管理检查清单

| 检查项 | 说明 | 状态 |
|-------|------|------|
| 监控就绪 | 错误率、延迟、吞吐量监控 | ☐ |
| 回滚方案 | 明确的回滚步骤和触发条件 | ☐ |
| 健康检查 | 后端服务健康检查配置 | ☐ |
| 合理超时 | 连接、读取、发送超时配置 | ☐ |
| 重试策略 | 重试条件、次数、超时配置 | ☐ |
| 限流保护 | 防止过载的限流配置 | ☐ |
| 日志追踪 | 请求 ID 追踪 | ☐ |
| 告警配置 | 异常情况告警 | ☐ |

---

**流量管理原则**: 渐进发布 → 实时监控 → 快速回滚 → 数据驱动

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
