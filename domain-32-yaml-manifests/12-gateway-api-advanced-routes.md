---
title: 12 - Gateway API 高级路由 YAML 配置参考
description: '# 12 - Gateway API 高级路由 YAML 配置参考'
category: yaml-manifests
tags:
- k8s
- yaml
- manifest
- template
- prometheus
- istio
- envoy
- coredns
- opa
- redis
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 10min
intent_queries:
- Gateway API 高级路由 YAML 配置参考 是什么
- 如何 Gateway API 高级路由 YAML 配置参考
- Kubernetes 32 yaml manifests 最佳实践
trigger_keywords:
- Gateway
- API
- 高级路由
- YAML
- 配置参考
- yaml
- manifests
cross_refs:
- type: fta
  path: ../topic-fta/list/gateway-api-fta.md
  label: '故障树: gateway-api'
---


# 12 - Gateway API 高级路由 YAML 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 + Gateway API v1.0+ | **最后更新**: 2026-02  
> **相关领域**: [域5-网络](../domain-5-networking/) | **前置知识**: Gateway API 核心资源  
> **关联配置**: [11-核心资源](./11-gateway-api-core.md) | [Service参考](./02-service-all-types.md)

---

## 📋 目录

1. [高级路由概述](#高级路由概述)
2. [GRPCRoute 配置](#grpcroute-配置)
3. [TCPRoute 配置](#tcproute-配置)
4. [TLSRoute 配置](#tlsroute-配置)
5. [UDPRoute 配置](#udproute-配置)
6. [ReferenceGrant 跨命名空间授权](#referencegrant-跨命名空间授权)
7. [BackendTLSPolicy 配置](#backendtlspolicy-配置)
8. [生产实战案例](#生产实战案例)
9. [版本兼容性与最佳实践](#版本兼容性与最佳实践)

---

## 高级路由概述

### 路由类型与成熟度

| 路由类型 | API 版本 | 成熟度 | 协议支持 | 用途 |
|----------|----------|--------|----------|------|
| **HTTPRoute** | v1 | Standard (GA) | HTTP, HTTPS | Web应用、REST API |
| **GRPCRoute** | v1alpha2 | Experimental | gRPC (HTTP/2) | 微服务RPC通信 |
| **TCPRoute** | v1alpha2 | Experimental | TCP | 数据库、消息队列 |
| **TLSRoute** | v1alpha2 | Experimental | TLS (SNI) | TLS 透传 |
| **UDPRoute** | v1alpha2 | Experimental | UDP | DNS、视频流 |

### 安装 Experimental Channel

```yaml
# Standard Channel 仅包含 HTTPRoute (GA)
# kubectl apply -f https://github.com/kubernetes-sigs/gateway-api/releases/download/v1.0.0/standard-install.yaml

# Experimental Channel 包含所有路由类型
kubectl apply -f https://github.com/kubernetes-sigs/gateway-api/releases/download/v1.0.0/experimental-install.yaml

# 验证安装
kubectl get crd | grep gateway
# 应输出:
# grpcroutes.gateway.networking.k8s.io
# tcproutes.gateway.networking.k8s.io
# tlsroutes.gateway.networking.k8s.io
# udproutes.gateway.networking.k8s.io
```

### 控制器支持矩阵

| 控制器实现 | HTTPRoute | GRPCRoute | TCPRoute | TLSRoute | UDPRoute |
|------------|-----------|-----------|----------|----------|----------|
| **Istio** | ✅ GA | ✅ Experimental | ✅ Experimental | ✅ Experimental | ❌ |
| **Envoy Gateway** | ✅ GA | ✅ Experimental | ✅ Experimental | ✅ Experimental | ✅ Experimental |
| **Nginx Gateway Fabric** | ✅ GA | 🚧 Planned | ❌ | ❌ | ❌ |
| **Traefik** | ✅ GA | ✅ Experimental | ✅ Experimental | ✅ Experimental | ✅ Experimental |

---

## GRPCRoute 配置

### API 信息

| 属性 | 值 |
|------|-----|
| **API Group** | `gateway.networking.k8s.io` |
| **API Version** | `v1alpha2` |
| **Kind** | `GRPCRoute` |
| **成熟度** | Experimental |
| **Gateway 协议要求** | `HTTPS` (gRPC over HTTP/2) |

### 字段规格表

| 字段路径 | 类型 | 必填 | 版本 | 说明 |
|----------|------|------|------|------|
| `spec.parentRefs[]` | array | ✅ | v1alpha2 | 绑定的 Gateway |
| `spec.hostnames[]` | array | ❌ | v1alpha2 | 主机名匹配 |
| `spec.rules[]` | array | ✅ | v1alpha2 | 路由规则列表 |
| `spec.rules[].matches[]` | array | ❌ | v1alpha2 | gRPC 匹配条件 |
| `spec.rules[].matches[].method` | object | ❌ | v1alpha2 | gRPC 方法匹配 |
| `spec.rules[].matches[].method.service` | string | ❌ | v1alpha2 | gRPC 服务名 (如 `my.service`) |
| `spec.rules[].matches[].method.method` | string | ❌ | v1alpha2 | gRPC 方法名 (如 `GetUser`) |
| `spec.rules[].matches[].headers[]` | array | ❌ | v1alpha2 | gRPC Header 匹配 |
| `spec.rules[].filters[]` | array | ❌ | v1alpha2 | 流量处理过滤器 |
| `spec.rules[].backendRefs[]` | array | ✅ | v1alpha2 | 后端 gRPC 服务 |

### 基础 gRPC 路由

```yaml
# 前置: Gateway 必须配置 HTTPS 监听器
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: grpc-gateway
  namespace: gateway-system
spec:
  gatewayClassName: istio
  
  listeners:
  # gRPC 需要 HTTPS 协议 (HTTP/2)
  - name: grpc
    port: 443
    protocol: HTTPS
    hostname: "*.grpc.example.com"
    tls:
      mode: Terminate
      certificateRefs:
      - name: grpc-tls-cert
    allowedRoutes:
      kinds:
      - kind: GRPCRoute
      namespaces:
        from: All

---
# 基础 gRPC 路由
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: GRPCRoute
metadata:
  name: user-grpc-service
  namespace: services
spec:
  # 绑定到 Gateway
  parentRefs:
  - name: grpc-gateway
    namespace: gateway-system
  
  # 主机名匹配
  hostnames:
  - "user-service.grpc.example.com"
  
  rules:
  # 所有 gRPC 调用路由到 user-service
  - backendRefs:
    - name: user-service
      port: 9090
```

### gRPC 方法级路由

```yaml
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: GRPCRoute
metadata:
  name: user-service-methods
  namespace: services
spec:
  parentRefs:
  - name: grpc-gateway
    namespace: gateway-system
  
  hostnames:
  - "api.grpc.example.com"
  
  rules:
  # 规则1: 匹配特定 Service 的特定 Method
  - matches:
    - method:
        # gRPC Service 全限定名
        service: "user.v1.UserService"
        # gRPC 方法名
        method: "GetUser"
    backendRefs:
    - name: user-read-service
      port: 9090
  
  # 规则2: 匹配特定 Service 的所有 Method
  - matches:
    - method:
        service: "user.v1.UserService"
        # 省略 method 表示匹配所有方法
    backendRefs:
    - name: user-service
      port: 9090
  
  # 规则3: 匹配特定 Method (任意 Service)
  - matches:
    - method:
        method: "CreateUser"
    backendRefs:
    - name: user-write-service
      port: 9091
  
  # 规则4: 默认路由(无 matches)
  - backendRefs:
    - name: default-grpc-backend
      port: 9090
```

### gRPC Header 匹配

```yaml
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: GRPCRoute
metadata:
  name: grpc-header-routing
  namespace: services
spec:
  parentRefs:
  - name: grpc-gateway
    namespace: gateway-system
  
  hostnames:
  - "api.grpc.example.com"
  
  rules:
  # Header 匹配: 路由到特定版本
  - matches:
    - headers:
      # gRPC Header 名称 (遵循 HTTP/2 Header 规范)
      - type: Exact
        name: x-api-version
        value: "v2"
    backendRefs:
    - name: api-v2-service
      port: 9090
  
  # Header 正则匹配: 识别客户端类型
  - matches:
    - headers:
      - type: RegularExpression
        name: user-agent
        value: "grpc-go/.*"
    backendRefs:
    - name: go-optimized-service
      port: 9090
```

### gRPC 权重路由(金丝雀)

```yaml
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: GRPCRoute
metadata:
  name: grpc-canary
  namespace: production
spec:
  parentRefs:
  - name: grpc-gateway
    namespace: gateway-system
  
  hostnames:
  - "order.grpc.example.com"
  
  rules:
  # 90% 流量到 v1, 10% 到 v2
  - matches:
    - method:
        service: "order.v1.OrderService"
    
    backendRefs:
    - name: order-service-v1
      port: 9090
      weight: 90
    - name: order-service-v2
      port: 9090
      weight: 10
```

### gRPC 流量镜像

```yaml
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: GRPCRoute
metadata:
  name: grpc-mirror
  namespace: production
spec:
  parentRefs:
  - name: grpc-gateway
    namespace: gateway-system
  
  rules:
  - matches:
    - method:
        service: "payment.v1.PaymentService"
    
    filters:
    # 镜像到测试环境
    - type: RequestMirror
      requestMirror:
        backendRef:
          name: payment-test-service
          namespace: testing
          port: 9090
    
    # 主流量到生产环境
    backendRefs:
    - name: payment-prod-service
      port: 9090
```

---

## TCPRoute 配置

### API 信息

| 属性 | 值 |
|------|-----|
| **API Version** | `v1alpha2` |
| **Kind** | `TCPRoute` |
| **成熟度** | Experimental |
| **Gateway 协议要求** | `TCP` |

### 字段规格表

| 字段路径 | 类型 | 必填 | 说明 |
|----------|------|------|------|
| `spec.parentRefs[]` | array | ✅ | 绑定的 Gateway |
| `spec.rules[]` | array | ✅ | 路由规则 |
| `spec.rules[].backendRefs[]` | array | ✅ | 后端 TCP 服务 |
| `spec.rules[].backendRefs[].name` | string | ✅ | Service 名称 |
| `spec.rules[].backendRefs[].port` | int32 | ✅ | Service 端口 |
| `spec.rules[].backendRefs[].weight` | int32 | ❌ | 流量权重 |

### TCP Gateway 配置

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: tcp-gateway
  namespace: gateway-system
spec:
  gatewayClassName: envoy
  
  listeners:
  # TCP 监听器 - 数据库端口
  - name: mysql
    port: 3306
    protocol: TCP
    allowedRoutes:
      kinds:
      - kind: TCPRoute
  
  # TCP 监听器 - Redis 端口
  - name: redis
    port: 6379
    protocol: TCP
    allowedRoutes:
      kinds:
      - kind: TCPRoute
```

### 基础 TCP 路由

```yaml
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TCPRoute
metadata:
  name: mysql-route
  namespace: database
spec:
  parentRefs:
  # 绑定到特定 listener
  - name: tcp-gateway
    namespace: gateway-system
    sectionName: mysql  # 指定 Gateway 的 listener 名称
  
  rules:
  # TCP 路由无匹配条件, 仅支持后端选择
  - backendRefs:
    - name: mysql-primary
      port: 3306
```

### TCP 读写分离

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: mysql-gateway
  namespace: database
spec:
  gatewayClassName: istio
  
  listeners:
  # 写入端口 (主库)
  - name: mysql-write
    port: 3306
    protocol: TCP
    allowedRoutes:
      kinds:
      - kind: TCPRoute
  
  # 读取端口 (从库)
  - name: mysql-read
    port: 3307
    protocol: TCP
    allowedRoutes:
      kinds:
      - kind: TCPRoute

---
# 主库路由
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TCPRoute
metadata:
  name: mysql-primary-route
  namespace: database
spec:
  parentRefs:
  - name: mysql-gateway
    sectionName: mysql-write
  
  rules:
  - backendRefs:
    - name: mysql-primary
      port: 3306

---
# 从库路由 (负载均衡)
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TCPRoute
metadata:
  name: mysql-replica-route
  namespace: database
spec:
  parentRefs:
  - name: mysql-gateway
    sectionName: mysql-read
  
  rules:
  - backendRefs:
    # 权重分配到多个从库
    - name: mysql-replica-1
      port: 3306
      weight: 50
    - name: mysql-replica-2
      port: 3306
      weight: 50
```

### Redis 集群路由

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: redis-gateway
  namespace: cache
spec:
  gatewayClassName: envoy
  
  listeners:
  - name: redis
    port: 6379
    protocol: TCP
    allowedRoutes:
      kinds:
      - kind: TCPRoute

---
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TCPRoute
metadata:
  name: redis-cluster-route
  namespace: cache
spec:
  parentRefs:
  - name: redis-gateway
  
  rules:
  # 负载均衡到 Redis 集群节点
  - backendRefs:
    - name: redis-node-1
      port: 6379
      weight: 1
    - name: redis-node-2
      port: 6379
      weight: 1
    - name: redis-node-3
      port: 6379
      weight: 1
```

---

## TLSRoute 配置

### API 信息

| 属性 | 值 |
|------|-----|
| **API Version** | `v1alpha2` |
| **Kind** | `TLSRoute` |
| **成熟度** | Experimental |
| **Gateway 协议要求** | `TLS` (Passthrough 模式) |
| **匹配机制** | SNI (Server Name Indication) |

### 字段规格表

| 字段路径 | 类型 | 必填 | 说明 |
|----------|------|------|------|
| `spec.parentRefs[]` | array | ✅ | 绑定的 Gateway |
| `spec.hostnames[]` | array | ❌ | SNI 主机名匹配 |
| `spec.rules[]` | array | ✅ | 路由规则 |
| `spec.rules[].backendRefs[]` | array | ✅ | 后端 TLS 服务 |

### TLS Passthrough Gateway

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: tls-passthrough-gateway
  namespace: gateway-system
spec:
  gatewayClassName: istio
  
  listeners:
  # TLS Passthrough 监听器
  - name: tls-passthrough
    port: 443
    protocol: TLS
    # Passthrough 模式: 不解密 TLS, 直接转发到后端
    tls:
      mode: Passthrough
    allowedRoutes:
      kinds:
      - kind: TLSRoute
      namespaces:
        from: All
```

### 基于 SNI 的 TLS 路由

```yaml
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TLSRoute
metadata:
  name: secure-app-route
  namespace: production
spec:
  parentRefs:
  - name: tls-passthrough-gateway
    namespace: gateway-system
  
  # SNI 主机名匹配
  hostnames:
  - "secure-app.example.com"
  
  rules:
  # 透传 TLS 到后端服务(后端自行处理 TLS)
  - backendRefs:
    - name: secure-app-service
      port: 8443
```

### 多域名 TLS 路由

```yaml
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TLSRoute
metadata:
  name: multi-domain-tls
  namespace: production
spec:
  parentRefs:
  - name: tls-passthrough-gateway
    namespace: gateway-system
  
  # 通配符 SNI 匹配
  hostnames:
  - "*.apps.example.com"
  
  rules:
  - backendRefs:
    - name: app-backend
      port: 8443

---
# 精确域名优先级更高
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TLSRoute
metadata:
  name: specific-domain-tls
  namespace: production
spec:
  parentRefs:
  - name: tls-passthrough-gateway
    namespace: gateway-system
  
  hostnames:
  - "admin.apps.example.com"
  
  rules:
  - backendRefs:
    - name: admin-backend
      port: 9443
```

### TLS 权重路由

```yaml
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TLSRoute
metadata:
  name: tls-canary
  namespace: production
spec:
  parentRefs:
  - name: tls-passthrough-gateway
    namespace: gateway-system
  
  hostnames:
  - "api.example.com"
  
  rules:
  # 金丝雀部署: 10% 流量到新版本
  - backendRefs:
    - name: api-v1
      port: 8443
      weight: 90
    - name: api-v2
      port: 8443
      weight: 10
```

---

## UDPRoute 配置

### API 信息

| 属性 | 值 |
|------|-----|
| **API Version** | `v1alpha2` |
| **Kind** | `UDPRoute` |
| **成熟度** | Experimental |
| **Gateway 协议要求** | `UDP` |
| **典型用途** | DNS, QUIC, 视频流, 游戏服务器 |

### 字段规格表

| 字段路径 | 类型 | 必填 | 说明 |
|----------|------|------|------|
| `spec.parentRefs[]` | array | ✅ | 绑定的 Gateway |
| `spec.rules[]` | array | ✅ | 路由规则 |
| `spec.rules[].backendRefs[]` | array | ✅ | 后端 UDP 服务 |

### UDP Gateway 配置

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: udp-gateway
  namespace: gateway-system
spec:
  gatewayClassName: envoy
  
  listeners:
  # DNS 服务
  - name: dns
    port: 53
    protocol: UDP
    allowedRoutes:
      kinds:
      - kind: UDPRoute
  
  # QUIC 服务
  - name: quic
    port: 443
    protocol: UDP
    allowedRoutes:
      kinds:
      - kind: UDPRoute
```

### DNS 服务路由

```yaml
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: UDPRoute
metadata:
  name: dns-route
  namespace: infrastructure
spec:
  parentRefs:
  - name: udp-gateway
    namespace: gateway-system
    sectionName: dns
  
  rules:
  # UDP 路由无匹配条件
  - backendRefs:
    # 负载均衡到多个 CoreDNS 实例
    - name: coredns
      port: 53
```

### 游戏服务器路由

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: game-gateway
  namespace: gaming
spec:
  gatewayClassName: envoy
  
  listeners:
  # 游戏服务器端口范围
  - name: game-server-1
    port: 7777
    protocol: UDP
    allowedRoutes:
      kinds:
      - kind: UDPRoute
  - name: game-server-2
    port: 7778
    protocol: UDP
    allowedRoutes:
      kinds:
      - kind: UDPRoute

---
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: UDPRoute
metadata:
  name: game-room-1
  namespace: gaming
spec:
  parentRefs:
  - name: game-gateway
    sectionName: game-server-1
  
  rules:
  - backendRefs:
    - name: game-room-1-backend
      port: 7777

---
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: UDPRoute
metadata:
  name: game-room-2
  namespace: gaming
spec:
  parentRefs:
  - name: game-gateway
    sectionName: game-server-2
  
  rules:
  - backendRefs:
    - name: game-room-2-backend
      port: 7777
```

---

## ReferenceGrant 跨命名空间授权

### API 信息

| 属性 | 值 |
|------|-----|
| **API Version** | `v1beta1` |
| **Kind** | `ReferenceGrant` |
| **用途** | 授权跨命名空间资源引用 |
| **部署位置** | 被引用资源的命名空间 |

### 字段规格表

| 字段路径 | 类型 | 必填 | 说明 |
|----------|------|------|------|
| `spec.from[]` | array | ✅ | 允许的引用来源 |
| `spec.from[].group` | string | ✅ | API 组 (如 `gateway.networking.k8s.io`) |
| `spec.from[].kind` | string | ✅ | 资源类型 (如 `HTTPRoute`) |
| `spec.from[].namespace` | string | ✅ | 来源命名空间 |
| `spec.to[]` | array | ✅ | 允许的引用目标 |
| `spec.to[].group` | string | ✅ | 目标资源 API 组 (如 `""` 表示 core) |
| `spec.to[].kind` | string | ✅ | 目标资源类型 (如 `Service`) |
| `spec.to[].name` | string | ❌ | 特定资源名称(可选) |

### 跨命名空间路由授权

```yaml
# 场景: team-a 的 HTTPRoute 引用 shared-services 命名空间的 Service

# HTTPRoute 在 team-a 命名空间
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: app-route
  namespace: team-a
spec:
  parentRefs:
  - name: shared-gateway
    namespace: gateway-system
  
  rules:
  - backendRefs:
    # 跨命名空间引用 Service
    - name: shared-backend
      namespace: shared-services
      port: 8080

---
# ReferenceGrant 在被引用资源的命名空间 (shared-services)
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: allow-team-a-to-shared-services
  namespace: shared-services  # 部署在被引用资源的命名空间
spec:
  # 来源: 允许来自 team-a 命名空间的 HTTPRoute
  from:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    namespace: team-a
  
  # 目标: 允许引用本命名空间的所有 Service
  to:
  - group: ""  # 空字符串表示 core API group
    kind: Service
```

### 限制特定资源授权

```yaml
# 仅允许引用特定名称的 Service
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: allow-specific-service
  namespace: shared-services
spec:
  from:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    namespace: team-a
  
  to:
  - group: ""
    kind: Service
    name: public-api-service  # 仅允许引用此特定 Service
```

### 跨命名空间 TLS 证书授权

```yaml
# 场景: Gateway 在 gateway-system, Secret 在 cert-manager 命名空间

# Gateway 引用其他命名空间的 Secret
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: production-gateway
  namespace: gateway-system
spec:
  gatewayClassName: istio
  listeners:
  - name: https
    port: 443
    protocol: HTTPS
    tls:
      certificateRefs:
      # 跨命名空间引用证书
      - name: wildcard-tls-cert
        namespace: cert-manager
        kind: Secret

---
# ReferenceGrant 在 Secret 所在的命名空间
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: allow-gateway-to-certs
  namespace: cert-manager
spec:
  from:
  - group: gateway.networking.k8s.io
    kind: Gateway
    namespace: gateway-system
  
  to:
  - group: ""
    kind: Secret
```

### 多来源多目标授权

```yaml
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: multi-source-grant
  namespace: shared-services
spec:
  # 允许多个来源命名空间
  from:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    namespace: team-a
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    namespace: team-b
  - group: gateway.networking.k8s.io
    kind: GRPCRoute
    namespace: team-a
  
  # 允许多种目标资源
  to:
  - group: ""
    kind: Service
  - group: ""
    kind: Secret
```

---

## BackendTLSPolicy 配置

### API 信息

| 属性 | 值 |
|------|-----|
| **API Version** | `v1alpha2` |
| **Kind** | `BackendTLSPolicy` |
| **成熟度** | Experimental (Gateway API v1.1+) |
| **用途** | 配置网关到后端的 TLS 连接 |

### 字段规格表

| 字段路径 | 类型 | 必填 | 说明 |
|----------|------|------|------|
| `spec.targetRef` | object | ✅ | 目标后端 Service |
| `spec.targetRef.group` | string | ✅ | 通常为 `""` (core) |
| `spec.targetRef.kind` | string | ✅ | 通常为 `Service` |
| `spec.targetRef.name` | string | ✅ | Service 名称 |
| `spec.targetRef.namespace` | string | ❌ | Service 命名空间 |
| `spec.validation.caCertificateRefs[]` | array | ✅ | CA 证书引用 (ConfigMap/Secret) |
| `spec.validation.hostname` | string | ✅ | 后端 TLS 主机名验证 |

### 基础 mTLS 后端配置

```yaml
# 场景: 网关与后端服务之间使用 mTLS 加密通信

# 1. 后端 Service
apiVersion: v1
kind: Service
metadata:
  name: secure-backend
  namespace: production
spec:
  selector:
    app: secure-app
  ports:
  - port: 8443
    name: https

---
# 2. CA 证书 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: backend-ca-cert
  namespace: production
data:
  ca.crt: |
    -----BEGIN CERTIFICATE-----
    MIIDXTCCAkWgAwIBAgIJAK...
    -----END CERTIFICATE-----

---
# 3. BackendTLSPolicy
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: BackendTLSPolicy
metadata:
  name: secure-backend-tls
  namespace: production
spec:
  # 目标后端 Service
  targetRef:
    group: ""
    kind: Service
    name: secure-backend
  
  # TLS 验证配置
  validation:
    # CA 证书引用
    caCertificateRefs:
    - name: backend-ca-cert
      group: ""
      kind: ConfigMap
    
    # 后端证书主机名验证
    hostname: secure-backend.production.svc.cluster.local

---
# 4. HTTPRoute 使用后端
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: secure-route
  namespace: production
spec:
  parentRefs:
  - name: production-gateway
    namespace: gateway-system
  
  rules:
  - backendRefs:
    # 网关会自动应用 BackendTLSPolicy
    - name: secure-backend
      port: 8443
```

### 使用 Secret 存储 CA 证书

```yaml
# CA 证书存储在 Secret 中
apiVersion: v1
kind: Secret
metadata:
  name: backend-ca-secret
  namespace: production
type: Opaque
data:
  # base64 编码的 CA 证书
  ca.crt: LS0tLS1CRUdJTi...

---
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: BackendTLSPolicy
metadata:
  name: secure-backend-tls
  namespace: production
spec:
  targetRef:
    group: ""
    kind: Service
    name: secure-backend
  
  validation:
    caCertificateRefs:
    - name: backend-ca-secret
      group: ""
      kind: Secret  # 使用 Secret 类型
    
    hostname: secure-backend.production.svc.cluster.local
```

### 跨命名空间后端 TLS

```yaml
# HTTPRoute 引用其他命名空间的后端
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: cross-ns-route
  namespace: team-a
spec:
  parentRefs:
  - name: shared-gateway
    namespace: gateway-system
  
  rules:
  - backendRefs:
    # 跨命名空间引用
    - name: shared-secure-backend
      namespace: shared-services
      port: 8443

---
# BackendTLSPolicy 在后端所在的命名空间
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: BackendTLSPolicy
metadata:
  name: shared-backend-tls
  namespace: shared-services
spec:
  targetRef:
    group: ""
    kind: Service
    name: shared-secure-backend
  
  validation:
    caCertificateRefs:
    - name: shared-ca-cert
      kind: ConfigMap
    hostname: shared-secure-backend.shared-services.svc.cluster.local

---
# ReferenceGrant 授权跨命名空间引用
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: allow-team-a-to-shared-backend
  namespace: shared-services
spec:
  from:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    namespace: team-a
  to:
  - group: ""
    kind: Service
    name: shared-secure-backend
```

---

## 生产实战案例

### 案例1: 微服务 gRPC 网关

```yaml
# 场景: 统一 gRPC 网关入口, 路由多个微服务

# 1. Gateway 配置
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: grpc-api-gateway
  namespace: api-platform
spec:
  gatewayClassName: istio
  
  listeners:
  - name: grpc
    port: 443
    protocol: HTTPS
    hostname: "*.api.company.com"
    tls:
      mode: Terminate
      certificateRefs:
      - name: api-wildcard-cert
    allowedRoutes:
      kinds:
      - kind: GRPCRoute
      namespaces:
        from: All

---
# 2. 用户服务 gRPC 路由
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: GRPCRoute
metadata:
  name: user-grpc-route
  namespace: user-service
spec:
  parentRefs:
  - name: grpc-api-gateway
    namespace: api-platform
  
  hostnames:
  - "user.api.company.com"
  
  rules:
  # GetUser, ListUsers 等读操作 -> 读副本
  - matches:
    - method:
        service: "user.v1.UserService"
        method: "GetUser"
    - method:
        service: "user.v1.UserService"
        method: "ListUsers"
    
    backendRefs:
    - name: user-read-replica
      port: 9090
  
  # CreateUser, UpdateUser 等写操作 -> 主实例
  - matches:
    - method:
        service: "user.v1.UserService"
        method: "CreateUser"
    - method:
        service: "user.v1.UserService"
        method: "UpdateUser"
    
    backendRefs:
    - name: user-primary
      port: 9090

---
# 3. 订单服务 gRPC 路由
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: GRPCRoute
metadata:
  name: order-grpc-route
  namespace: order-service
spec:
  parentRefs:
  - name: grpc-api-gateway
    namespace: api-platform
  
  hostnames:
  - "order.api.company.com"
  
  rules:
  # 金丝雀发布
  - matches:
    - method:
        service: "order.v2.OrderService"
    
    backendRefs:
    - name: order-v1
      port: 9090
      weight: 80
    - name: order-v2
      port: 9090
      weight: 20

---
# 4. 授权跨命名空间引用
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: allow-grpc-routes
  namespace: api-platform
spec:
  from:
  - group: gateway.networking.k8s.io
    kind: GRPCRoute
    namespace: user-service
  - group: gateway.networking.k8s.io
    kind: GRPCRoute
    namespace: order-service
  to:
  - group: gateway.networking.k8s.io
    kind: Gateway
```

### 案例2: 数据库四层代理网关

```yaml
# 场景: 为多个数据库提供统一网关入口, 支持读写分离

# 1. Gateway 配置
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: database-gateway
  namespace: database-infra
spec:
  gatewayClassName: envoy
  
  listeners:
  # MySQL 主库端口
  - name: mysql-primary
    port: 3306
    protocol: TCP
    allowedRoutes:
      kinds:
      - kind: TCPRoute
      namespaces:
        from: Selector
        selector:
          matchLabels:
            db-access: "enabled"
  
  # MySQL 从库端口
  - name: mysql-replica
    port: 3307
    protocol: TCP
    allowedRoutes:
      kinds:
      - kind: TCPRoute
      namespaces:
        from: Selector
        selector:
          matchLabels:
            db-access: "enabled"
  
  # PostgreSQL 端口
  - name: postgres
    port: 5432
    protocol: TCP
    allowedRoutes:
      kinds:
      - kind: TCPRoute
      namespaces:
        from: Selector
        selector:
          matchLabels:
            db-access: "enabled"
  
  # Redis 端口
  - name: redis
    port: 6379
    protocol: TCP
    allowedRoutes:
      kinds:
      - kind: TCPRoute

---
# 2. MySQL 主库路由
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TCPRoute
metadata:
  name: mysql-primary-route
  namespace: mysql-production
  labels:
    db-access: "enabled"
spec:
  parentRefs:
  - name: database-gateway
    namespace: database-infra
    sectionName: mysql-primary
  
  rules:
  - backendRefs:
    - name: mysql-primary-svc
      port: 3306

---
# 3. MySQL 从库路由 (负载均衡)
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TCPRoute
metadata:
  name: mysql-replica-route
  namespace: mysql-production
  labels:
    db-access: "enabled"
spec:
  parentRefs:
  - name: database-gateway
    namespace: database-infra
    sectionName: mysql-replica
  
  rules:
  - backendRefs:
    - name: mysql-replica-1-svc
      port: 3306
      weight: 1
    - name: mysql-replica-2-svc
      port: 3306
      weight: 1

---
# 4. Redis Cluster 路由
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TCPRoute
metadata:
  name: redis-route
  namespace: redis-production
spec:
  parentRefs:
  - name: database-gateway
    namespace: database-infra
    sectionName: redis
  
  rules:
  - backendRefs:
    # Redis Cluster 模式自动分片
    - name: redis-cluster-svc
      port: 6379
```

### 案例3: TLS Passthrough 多租户网关

```yaml
# 场景: 多租户 SaaS, 每个租户使用自己的 TLS 证书(透传)

# 1. Gateway 配置
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: saas-tls-gateway
  namespace: platform-infra
spec:
  gatewayClassName: istio
  
  listeners:
  - name: tls-passthrough
    port: 443
    protocol: TLS
    tls:
      mode: Passthrough  # 不在网关层解密
    allowedRoutes:
      kinds:
      - kind: TLSRoute
      namespaces:
        from: Selector
        selector:
          matchLabels:
            tenant: "true"

---
# 2. 租户 A 路由 (自管理证书)
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-a
  labels:
    tenant: "true"
---
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TLSRoute
metadata:
  name: tenant-a-route
  namespace: tenant-a
spec:
  parentRefs:
  - name: saas-tls-gateway
    namespace: platform-infra
  
  # SNI 主机名匹配
  hostnames:
  - "tenant-a.company.com"
  
  rules:
  # 透传 TLS 到租户的后端服务(后端自行解密)
  - backendRefs:
    - name: tenant-a-backend
      port: 8443

---
# 3. 租户 B 路由
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-b
  labels:
    tenant: "true"
---
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TLSRoute
metadata:
  name: tenant-b-route
  namespace: tenant-b
spec:
  parentRefs:
  - name: saas-tls-gateway
    namespace: platform-infra
  
  hostnames:
  - "tenant-b.company.com"
  
  rules:
  - backendRefs:
    - name: tenant-b-backend
      port: 8443

---
# 4. 租户后端 Pod 配置 (示例)
apiVersion: v1
kind: Pod
metadata:
  name: tenant-a-backend
  namespace: tenant-a
  labels:
    app: tenant-a-backend
spec:
  containers:
  - name: app
    image: nginx:latest
    ports:
    - containerPort: 8443
    volumeMounts:
    # 挂载租户自己的 TLS 证书
    - name: tls-cert
      mountPath: /etc/nginx/ssl
      readOnly: true
  volumes:
  - name: tls-cert
    secret:
      secretName: tenant-a-tls-secret
```

---

## 版本兼容性与最佳实践

### Gateway API 版本演进

| Gateway API 版本 | 新增高级路由特性 | Kubernetes 最低版本 |
|------------------|------------------|---------------------|
| v0.5.0 | GRPCRoute, TCPRoute, UDPRoute (Alpha) | v1.21+ |
| v0.6.0 | TLSRoute (Alpha), ReferenceGrant (Beta) | v1.23+ |
| v1.0.0 | HTTPRoute (GA), 其他路由保持 Experimental | v1.25+ |
| v1.1.0 | BackendTLSPolicy (Alpha), 改进 GRPCRoute | v1.27+ |

### 控制器支持检查

```bash
# 检查控制器支持的路由类型
kubectl explain grpcroute
kubectl explain tcproute
kubectl explain tlsroute

# 查看 Gateway 支持的路由类型
kubectl get gateway <gateway-name> -o yaml | grep -A 10 allowedRoutes
```

### 最佳实践

#### 1. 选择合适的路由类型

| 场景 | 推荐路由类型 | 原因 |
|------|--------------|------|
| REST API | HTTPRoute | 标准 GA, 支持完整 HTTP 匹配 |
| 微服务 RPC | GRPCRoute | 原生 gRPC 方法匹配 |
| 数据库代理 | TCPRoute | 四层负载均衡 |
| 自签名证书透传 | TLSRoute | SNI 路由, 后端自行处理 TLS |
| DNS 服务 | UDPRoute | 无状态 UDP 协议 |

#### 2. GRPCRoute 性能优化

```yaml
# 推荐: 为 gRPC 启用 HTTP/2
apiVersion: v1
kind: Service
metadata:
  name: grpc-service
  annotations:
    # Istio 特定: 强制 HTTP/2
    networking.istio.io/appProtocol: grpc
spec:
  ports:
  - port: 9090
    name: grpc  # 端口名称必须以 grpc 开头
    protocol: TCP
```

#### 3. TCPRoute 健康检查

```yaml
# TCP 后端 Service 配置健康检查
apiVersion: v1
kind: Service
metadata:
  name: mysql-primary
  annotations:
    # Envoy Gateway 健康检查配置(取决于实现)
    gateway.envoyproxy.io/health-check: |
      timeout: 5s
      interval: 10s
      unhealthyThreshold: 3
      healthyThreshold: 2
spec:
  ports:
  - port: 3306
    name: mysql
```

#### 4. ReferenceGrant 安全策略

```yaml
# 最小权限原则: 仅授权必要的资源
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: minimal-grant
  namespace: shared-services
spec:
  from:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    namespace: team-a  # 明确指定命名空间
  to:
  - group: ""
    kind: Service
    name: public-api-only  # 明确指定资源名称
```

#### 5. 可观测性

```yaml
# 为高级路由添加监控标签
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: GRPCRoute
metadata:
  name: monitored-grpc-route
  namespace: production
  labels:
    monitoring: "enabled"
    team: "backend"
  annotations:
    prometheus.io/scrape: "true"
spec:
  parentRefs:
  - name: grpc-gateway
  
  rules:
  - backendRefs:
    - name: grpc-backend
      port: 9090
```

### FAQ

#### Q1: GRPCRoute 与 HTTPRoute 有何区别?

**A:** 核心差异:
- **协议**: GRPCRoute 专为 gRPC (HTTP/2) 设计
- **匹配**: GRPCRoute 支持 `method.service` 和 `method.method` 匹配
- **性能**: GRPCRoute 优化了 HTTP/2 流处理
- **后端**: 两者都可以路由到相同的 Service, 但 GRPCRoute 更精确

```yaml
# HTTPRoute 也可以路由 gRPC, 但匹配不精确
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: grpc-via-http
spec:
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /user.v1.UserService/  # 路径匹配不如 GRPCRoute 直观
    backendRefs:
    - name: user-service
      port: 9090
```

#### Q2: TCPRoute 如何实现会话保持?

**A:** TCPRoute 本身不支持会话保持, 需依赖:
1. **Service 层**: 使用 `sessionAffinity: ClientIP`
```yaml
apiVersion: v1
kind: Service
metadata:
  name: mysql-primary
spec:
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800  # 3小时
  ports:
  - port: 3306
```

2. **控制器特性**: 某些实现支持 Session Persistence (Experimental)

#### Q3: UDPRoute 适用于哪些场景?

**A:** 典型用途:
- **DNS 服务**: CoreDNS, BIND
- **QUIC 协议**: HTTP/3 传输层
- **实时通信**: VoIP, 视频会议
- **游戏服务器**: 低延迟 UDP 连接
- **IoT 数据**: 传感器数据上报

**不适用**: 需要可靠传输的场景(文件传输, 数据库)

#### Q4: TLSRoute Passthrough 与 Terminate 如何选择?

| 模式 | TLS 解密位置 | 用途 | 优势 | 劣势 |
|------|--------------|------|------|------|
| **Terminate** | Gateway | HTTPRoute, GRPCRoute | 网关可检查/修改流量, 集中证书管理 | 增加 Gateway 负载 |
| **Passthrough** | Backend | TLSRoute | 端到端加密, 自定义 TLS 配置 | 无法在网关层过滤流量 |

#### Q5: BackendTLSPolicy 何时必需?

**A:** 必需场景:
- Gateway 与后端之间使用 mTLS
- 后端服务使用自签名证书
- 需要验证后端证书主机名
- 零信任网络架构

**非必需**: 后端使用 HTTP 或网关到后端的网络已加密(如 Istio mTLS)

---

## 相关资源

### 官方文档
- Gateway API 高级路由: https://gateway-api.sigs.k8s.io/guides/
- GRPCRoute 规范: https://gateway-api.sigs.k8s.io/references/spec/#gateway.networking.k8s.io/v1alpha2.GRPCRoute
- ReferenceGrant 指南: https://gateway-api.sigs.k8s.io/api-types/referencegrant/

### 实现文档
- Istio Gateway API: https://istio.io/latest/docs/tasks/traffic-management/ingress/gateway-api/
- Envoy Gateway: https://gateway.envoyproxy.io/latest/user/grpc-routing/
- Traefik: https://doc.traefik.io/traefik/routing/providers/kubernetes-gateway/

### 本知识库相关文档
- [11 - Gateway API 核心资源](./11-gateway-api-core.md)
- [Service 全类型参考](./02-service-all-types.md)
- [Service Mesh 故障排查](../topic-structural-trouble-shooting/03-networking/05-service-mesh-istio-troubleshooting.md)

---

**最后更新**: 2026-02 | **维护者**: Kudig.io 社区 | **反馈**: [GitHub Issues](https://github.com/kudig-io/kudig-database)
