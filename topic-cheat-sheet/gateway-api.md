---
title: "Kubernetes Gateway API 速查卡"
title_en: "Kubernetes Gateway API Cheat Sheet"
description: "Gateway API (替代 Ingress) 快速参考, 覆盖 Gateway/HTTPRoute/TLS/流量分割/认证"
category: cheatsheet
tags: [gateway-api, k8s, ingress, networking, envoy, cheatsheet, quick-reference]
last_updated: "2026-05"
difficulty: "advanced"
reading_level: "advanced"
audience: ["SRE", "网络工程师", "架构师"]
estimated_read_time: "10min"
intent_queries:
  - "Gateway API 和 Ingress 有什么区别"
  - "Gateway API 怎么配置"
  - "HTTPRoute 路由规则怎么写"
  - "Gateway API 流量分割怎么做"
  - "Gateway API TLS 配置"
trigger_keywords:
  - "Gateway API"
  - "HTTPRoute"
  - "Gateway"
  - "流量分割"
  - "金丝雀"
---

# Kubernetes Gateway API 速查卡

> **适用版本**: Gateway API v1.0+ (GA) | **最后更新**: 2026-05

---

## Gateway API vs Ingress

| 维度 | Ingress | Gateway API |
|------|---------|-------------|
| 角色分离 | 单一资源 | GatewayClass → Gateway → Route 三层分离 |
| 协议支持 | HTTP/HTTPS | HTTP, HTTPS, gRPC, TCP, TLS, UDP |
| 流量分割 | 不支持 (需注解) | 原生支持权重路由 |
| Header 匹配 | 有限 | 完整的 Header/Query/Method 匹配 |
| 状态 | 冻结 | GA (v1.0+), 活跃演进 |

---

## 核心资源关系

```
GatewayClass (基础设施提供者)
    └── Gateway (集群运维者)
            └── HTTPRoute / GRPCRoute / TCPRoute (应用开发者)
```

---

## GatewayClass

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: envoy-gateway
spec:
  controllerName: gateway.envoyproxy.io/gatewayclass-controller
```

---

## Gateway

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: production-gateway
  namespace: infra
spec:
  gatewayClassName: envoy-gateway
  listeners:
    - name: http
      protocol: HTTP
      port: 80
    - name: https
      protocol: HTTPS
      port: 443
      tls:
        mode: Terminate
        certificateRefs:
          - name: wildcard-tls
      allowedRoutes:
        namespaces:
          from: All    # All | Same | Selector
```

---

## HTTPRoute

### 基本路由

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: my-app-route
  namespace: production
spec:
  parentRefs:
    - name: production-gateway
      namespace: infra
      sectionName: https       # 绑定到哪个 listener
  hostnames:
    - "app.example.com"
  rules:
    - matches:
        - path:
            type: PathPrefix
            value: /api
      backendRefs:
        - name: api-service
          port: 8080
          weight: 100
```

### Header 匹配

```yaml
rules:
  - matches:
      - headers:
          - name: X-Canary
            value: "true"
    backendRefs:
      - name: api-canary
        port: 8080
```

### 流量分割 (金丝雀)

```yaml
rules:
  - backendRefs:
      - name: api-stable
        port: 8080
        weight: 90
      - name: api-canary
        port: 8080
        weight: 10
```

### 请求改写

```yaml
rules:
  - matches:
      - path:
          type: PathPrefix
            value: /old-api
    filters:
      - type: URLRewrite
        urlRewrite:
          path:
            type: ReplacePrefixMatch
            replacePrefixMatch: /v2
    backendRefs:
      - name: api-service
        port: 8080
```

### 重定向

```yaml
rules:
  - filters:
      - type: RequestRedirect
        requestRedirect:
          scheme: https
          statusCode: 301
```

---

## GRPCRoute

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: GRPCRoute
metadata:
  name: grpc-route
spec:
  parentRefs:
    - name: production-gateway
  hostnames:
    - "grpc.example.com"
  rules:
    - matches:
        - method:
            service: myapp.UserService
            method: GetUser
      backendRefs:
        - name: user-grpc-service
          port: 50051
```

---

## 参考实现

| 实现 | GatewayClass Controller | 特点 |
|------|------------------------|------|
| Envoy Gateway | gateway.envoyproxy.io | 基于 Envoy, 功能最全 |
| Cilium | io.cilium/gateway-controller | eBPF 加速, 高性能 |
| Istio | istio.io/gateway-controller | 与 Service Mesh 统一 |
| Nginx | gateway.nginx.org/nginx-gateway-controller | Nginx 生态 |
| Kong | konghq.com/kic-gateway-controller | API 管理能力 |
| Traefik | traefik.io/gateway-controller | 自动服务发现 |

---

## 迁移检查清单

- [ ] 确认集群版本 K8s 1.26+ (Gateway API v1.0 GA)
- [ ] 安装 Gateway API CRD: `kubectl apply -f https://github.com/kubernetes-sigs/gateway-api/releases/latest/download/standard-install.yaml`
- [ ] 选择并安装参考实现 (推荐 Envoy Gateway 或 Cilium)
- [ ] 创建 GatewayClass → Gateway → HTTPRoute
- [ ] 验证路由规则: `curl -H "Host: app.example.com" http://<LB-IP>/`
- [ ] 配置 TLS: 创建 Secret + Gateway listener TLS
- [ ] 迁移 Ingress 注解功能 (限流/认证等) 为 Gateway API 原生 filter
- [ ] 灰度切换: 先并行运行 Ingress + Gateway API, 验证后切换
- [ ] 下线旧 Ingress 资源
