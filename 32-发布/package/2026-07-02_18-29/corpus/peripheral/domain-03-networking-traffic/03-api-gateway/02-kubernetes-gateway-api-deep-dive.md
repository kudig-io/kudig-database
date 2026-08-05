---
title: 02 - Kubernetes Gateway API 标准深度解析
description: '# 02 - Kubernetes Gateway API 标准深度解析'
summary: '1. [Gateway API 概述与设计动机](#1-gateway-api-概述与设计动机)'
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- envoy
- apisix
- higress
- ingress
- gateway
- crd
- rag
- wasm
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 架构师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Gateway API 标准深度解析 是什么
- 如何 Kubernetes Gateway API 标准深度解析
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- Kubernetes
- Gateway
- API
- 标准深度解析
- cloud
- native
- api
- gateway
prerequisites:
- kubectl-basics
- networking-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/gateway-api-fta.md
  label: '故障树: gateway-api'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 02 - [[Kubernetes|Kubernetes]] Gateway API 标准深度解析

> **文档版本**: v1.0 | **适用版本**: Gateway API v1.0 - v1.2, Kubernetes 1.25+ | **更新日期**: 2026-03-04 | **关键词**: Gateway API, GatewayClass, HTTPRoute, ReferenceGrant, 一致性测试

<!-- chunk: 目录 -->## 目录

1. [Gateway API 概述与设计动机](#1-gateway-api-概述与设计动机)
2. [核心资源模型](#2-核心资源模型)
3. [角色分离模型](#3-角色分离模型)
4. [HTTPRoute 深度解析](#4-httproute-深度解析)
5. [高级路由类型](#5-高级路由类型)
6. [跨命名空间路由与 ReferenceGrant](#6-跨命名空间路由与-referencegrant)
7. [一致性测试与合规等级](#7-一致性测试与合规等级)
8. [各产品 Gateway API 支持矩阵](#8-各产品-gateway-api-支持矩阵)
9. [从 [[Ingress|Ingress]] 迁移到 Gateway API](#9-从-ingress-迁移到-gateway-api)

---

<!-- chunk: 1. Gateway API 概述与设计动机 -->## 1. Gateway API 概述与设计动机

Gateway API 是 Kubernetes SIG-Network 主导的下一代流量路由标准，旨在解决传统 Ingress 资源的核心痛点：

| 痛点 | Ingress | Gateway API |
|------|---------|-------------|
| **配置碎片化** | 大量厂商注解，不可移植 | 统一 CRD 规范，跨实现可移植 |
| **角色混淆** | 单一资源，权限难分离 | 三层角色模型，职责清晰 |
| **能力有限** | 仅支持 HTTP 基础路由 | HTTP/gRPC/TCP/TLS/UDP 全协议 |
| **扩展性差** | 注解字符串，无类型安全 | 强类型 CRD，Policy Attachment |
| **跨 NS 路由** | 不支持或非标准 | ReferenceGrant 原生支持 |

## API 版本演进

| 版本 | 发布时间 | 关键特性 |
|------|---------|---------|
| v0.5.0 | 2022-07 | GatewayClass/Gateway/HTTPRoute Beta |
| v1.0.0 | 2023-10 | HTTPRoute GA；GatewayClass/Gateway GA |
| v1.1.0 | 2024-05 | BackendTLSPolicy、GRPCRoute Standard |
| v1.2.0 | 2024-11 | Gateway API for Mesh (GAMMA) 实验性支持 |

<!-- chunk: 2. 核心资源模型 -->## 2. 核心资源模型

```
┌──────────────────────────────────────────────────────────────────┐
│                    Gateway API 资源层次模型                        │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────────┐                                            │
│  │  GatewayClass    │  ← 基础设施提供者定义（类似 StorageClass）     │
│  │  (集群级)         │     每个 API 网关实现注册一个 GatewayClass    │
│  └────────┬─────────┘                                            │
│           │                                                      │
│  ┌────────▼─────────┐                                            │
│  │  Gateway          │  ← 平台运维团队创建（绑定监听端口和证书）      │
│  │  (命名空间级)      │     定义 listener: port, protocol, TLS      │
│  └────────┬─────────┘                                            │
│           │  ← allowedRoutes 控制哪些 NS 可以挂载路由               │
│  ┌────────▼─────────┐                                            │
│  │  *Route           │  ← 应用开发者创建（定义路由规则）              │
│  │  (命名空间级)      │     HTTPRoute / GRPCRoute / TCPRoute       │
│  └──────────────────┘     TLSRoute / UDPRoute                    │
│                                                                  │
│  ┌──────────────────┐                                            │
│  │  ReferenceGrant  │  ← 跨命名空间引用授权                        │
│  │  (命名空间级)     │     允许 Route 引用其他 NS 的 Service         │
│  └──────────────────┘                                            │
│                                                                  │
│  ┌──────────────────┐                                            │
│  │  Policy (扩展)    │  ← 直接附加策略（限流、超时、重试等）          │
│  │  BackendTLSPolicy │     通过 targetRef 关联到 Gateway/Route     │
│  │  (命名空间级)     │                                            │
│  └──────────────────┘                                            │
└──────────────────────────────────────────────────────────────────┘
```

## GatewayClass

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: higress
spec:
  controllerName: higress.io/gateway-controller
  description: "Higress cloud-native API gateway"
```

## Gateway

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: api-gateway
  namespace: gateway-system
spec:
  gatewayClassName: higress
  listeners:
  - name: http
    port: 80
    protocol: HTTP
    allowedRoutes:
      namespaces:
        from: All
  - name: https
    port: 443
    protocol: HTTPS
    tls:
      mode: Terminate
      certificateRefs:
      - name: wildcard-cert
        kind: Secret
    allowedRoutes:
      namespaces:
        from: Selector
        selector:
          matchLabels:
            gateway-access: "true"
```

## HTTPRoute

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: app-routes
  namespace: app-team
spec:
  parentRefs:
  - name: api-gateway
    namespace: gateway-system
  hostnames:
  - "api.example.com"
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /v1/users
      headers:
      - name: x-api-version
        value: "v1"
    backendRefs:
    - name: users-service
      port: 8080
      weight: 90
    - name: users-service-canary
      port: 8080
      weight: 10
    filters:
    - type: RequestHeaderModifier
      requestHeaderModifier:
        add:
        - name: x-request-id
          value: "generated"
```

<!-- chunk: 3. 角色分离模型 -->## 3. 角色分离模型

Gateway API 通过资源层次实现了清晰的职责分离：

| 角色 | 职责 | 管理的资源 | 典型团队 |
|------|------|-----------|---------|
| **基础设施提供者** | 部署和维护网关控制器 | GatewayClass | 云平台团队 |
| **集群运维** | 创建 Gateway 实例，配置监听和证书 | Gateway, Secret(TLS) | SRE/平台团队 |
| **应用开发者** | 定义路由规则和后端 | HTTPRoute, GRPCRoute | 应用团队 |
| **策略管理员** | 附加安全和流量策略 | Policy (限流、超时等) | 安全/SRE 团队 |

```
基础设施提供者                集群运维                  应用开发者
     │                        │                         │
     ▼                        ▼                         ▼
 GatewayClass ──────────▶ Gateway ──────────────▶ HTTPRoute
 (定义实现)            (绑定端口/证书)          (定义路由规则)
                              │
                              ▼
                       allowedRoutes
                   (控制哪些 NS 可挂路由)
```

<!-- chunk: 4. HTTPRoute 深度解析 -->## 4. HTTPRoute 深度解析

## 匹配规则优先级

HTTPRoute 匹配按以下优先级排序：

1. **精确路径** > **前缀路径** > **正则路径**
2. **更长的路径前缀** > **更短的路径前缀**
3. **Header 匹配数量多** > **Header 匹配数量少**
4. **Query 参数匹配数量多** > **Query 参数匹配数量少**

## 流量分割（金丝雀发布）

```yaml
rules:
- matches:
  - path:
      type: PathPrefix
      value: /api
  backendRefs:
  - name: api-v1
    port: 8080
    weight: 95    # 95% 流量到稳定版
  - name: api-v2
    port: 8080
    weight: 5     # 5% 流量到金丝雀版
```

## 请求/响应转换

```yaml
rules:
- matches:
  - path:
      type: PathPrefix
      value: /legacy
  filters:
  - type: URLRewrite
    urlRewrite:
      hostname: new-service.internal
      path:
        type: ReplacePrefixMatch
        replacePrefixMatch: /v2
  - type: RequestHeaderModifier
    requestHeaderModifier:
      set:
      - name: x-forwarded-prefix
        value: /legacy
      remove:
      - x-internal-header
  backendRefs:
  - name: new-service
    port: 8080
```

## 请求镜像

```yaml
rules:
- matches:
  - path:
      type: PathPrefix
      value: /api
  backendRefs:
  - name: api-prod
    port: 8080
  filters:
  - type: RequestMirror
    requestMirror:
      backendRef:
        name: api-shadow
        port: 8080
```

<!-- chunk: 5. 高级路由类型 -->## 5. 高级路由类型

## GRPCRoute（v1.1 Standard）

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: GRPCRoute
metadata:
  name: grpc-route
spec:
  parentRefs:
  - name: api-gateway
  hostnames:
  - "grpc.example.com"
  rules:
  - matches:
    - method:
        service: helloworld.Greeter
        method: SayHello
    backendRefs:
    - name: greeter-service
      port: 50051
```

## TCPRoute

```yaml
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TCPRoute
metadata:
  name: tcp-route
spec:
  parentRefs:
  - name: api-gateway
    sectionName: tcp-listener
  rules:
  - backendRefs:
    - name: database-service
      port: 5432
```

## TLSRoute（TLS Passthrough）

```yaml
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TLSRoute
metadata:
  name: tls-passthrough
spec:
  parentRefs:
  - name: api-gateway
    sectionName: tls-passthrough-listener
  hostnames:
  - "secure.example.com"
  rules:
  - backendRefs:
    - name: backend-tls-service
      port: 8443
```

<!-- chunk: 6. 跨命名空间路由与 ReferenceGrant -->## 6. 跨命名空间路由与 ReferenceGrant

ReferenceGrant 解决了多团队共享网关时的跨 NS 安全问题：

```yaml
# 在 backend-ns 命名空间中创建
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: allow-route-from-app
  namespace: backend-ns
spec:
  from:
  - group: gateway.networking.k8s.io
    kind: HTTPRoute
    namespace: app-ns         # 允许 app-ns 中的 HTTPRoute
  to:
  - group: ""
    kind: Service             # 引用本 NS 中的 Service
```

```
┌────────────────┐         ┌────────────────┐
│  app-ns        │         │  backend-ns    │
│                │         │                │
│  HTTPRoute ────│────────▶│  Service       │
│  (引用 backend │  允许   │  (被引用)       │
│   -ns/svc)     │◀────────│                │
│                │ Ref     │  ReferenceGrant│
│                │ Grant   │  (授权 app-ns) │
└────────────────┘         └────────────────┘
```

<!-- chunk: 7. 一致性测试与合规等级 -->## 7. 一致性测试与合规等级

Gateway API 定义了分层的一致性配置文件：

| 合规等级 | 包含能力 | 说明 |
|---------|---------|------|
| **Core** | 基础 HTTPRoute 匹配和路由 | 所有实现必须通过 |
| **Extended** | Header 匹配、URL 重写、流量分割 | 推荐支持 |
| **Implementation-Specific** | 厂商自定义扩展 | 通过 Policy Attachment |

## 运行一致性测试

```bash
# 安装 Gateway API 一致性测试套件
go install sigs.k8s.io/gateway-api/conformance/echo-basic@latest

# 运行一致性测试
go test ./conformance/... -run TestConformance \
  -gateway-class=higress \
  -supported-features=HTTPRouteQueryParamMatching,HTTPRouteMethodMatching
```

<!-- chunk: 8. 各产品 Gateway API 支持矩阵 -->## 8. 各产品 Gateway API 支持矩阵

| 能力 | Higress | APISIX | Kong | [[Envoy|Envoy]] GW | Traefik |
|------|---------|--------|------|----------|---------|
| **GatewayClass** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Gateway** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **HTTPRoute** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **GRPCRoute** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **TCPRoute** | ✅ | ⚠️ 实验 | ✅ | ✅ | ✅ |
| **TLSRoute** | ✅ | ⚠️ 实验 | ✅ | ✅ | ✅ |
| **UDPRoute** | ❌ | ❌ | ✅ | ✅ | ✅ |
| **ReferenceGrant** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **BackendTLSPolicy** | ✅ | ❌ | ✅ | ✅ | ❌ |
| **一致性等级** | Extended | Core | Extended | Extended | Extended |

> 注意：各产品的 Gateway API 支持状态在持续演进中，请以官方文档为准。

<!-- chunk: 9. 从 Ingress 迁移到 Gateway API -->## 9. 从 Ingress 迁移到 Gateway API

## 资源映射关系

| Ingress 概念 | Gateway API 对应 |
|-------------|-----------------|
| `ingressClassName` | `GatewayClass` + `Gateway.spec.gatewayClassName` |
| `Ingress.spec.tls` | `Gateway.spec.listeners[].tls` |
| `Ingress.spec.rules[].host` | `HTTPRoute.spec.hostnames` |
| `Ingress.spec.rules[].http.paths` | `HTTPRoute.spec.rules[].matches` |
| `backend.service` | `HTTPRoute.spec.rules[].backendRefs` |
| Nginx 注解 (rewrite) | `HTTPRoute.spec.rules[].filters` |
| Nginx 注解 (rate-limit) | Policy Attachment (厂商扩展) |

## 迁移示例

**Ingress (迁移前):**
```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /$2
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
spec:
  ingressClassName: nginx
  tls:
  - hosts: ["api.example.com"]
    secretName: api-tls
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /api(/|$)(.*)
        pathType: ImplementationSpecific
        backend:
          service:
            name: api-svc
            port:
              number: 80
```

**Gateway API (迁移后):**
```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: app
spec:
  parentRefs:
  - name: api-gateway
  hostnames:
  - "api.example.com"
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api
    filters:
    - type: URLRewrite
      urlRewrite:
        path:
          type: ReplacePrefixMatch
          replacePrefixMatch: /
    backendRefs:
    - name: api-svc
      port: 80
```

> 完整的迁移指南请参考 [09-传统 Ingress 控制器向云原生 API 网关迁移](./09-nginx-ingress-migration-guide.md)。

---

<!-- chunk: 参考资料 -->## 参考资料

- [Gateway API 官方文档](https://gateway-api.sigs.k8s.io/)
- [Gateway API GitHub](https://github.com/kubernetes-sigs/gateway-api)
- [GEP (Gateway Enhancement Proposals)](https://gateway-api.sigs.k8s.io/geps/overview/)
- [Domain-5: 网络 - Gateway API 概览](32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-03-networking-traffic/00-core-k8s-networking/27-gateway-api-overview.md)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-40-cloud-native-api-gateway MOC
- [[domain-03-networking-traffic/README.md|Domain 03: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technolo...]]
- Domain-40 云原生 API 网关 — 开源项目索引
- 01 - 云原生 API 网关架构总览
- 03 - API 网关选型指南与对比矩阵
- 04 - Higress 云原生 API 网关企业级实践
- 05 - Apache APISIX 企业级 API 网关实践
- 06 - Kong API 网关企业级实践
- 07 - Envoy Gateway 企业级实践
- 08 - Traefik API 网关企业级实践
- 09 - 传统 Ingress 控制器向云原生 API 网关迁移
- 10 - Wasm 插件生态与开发实践

## See Also

- 99-envoy-gateway-enterprise-guide
- 01-api-gateway-architecture-overview
- 03-api-gateway-selection-guide
- 04-higress-enterprise-gateway


<!-- risk-assessed -->
