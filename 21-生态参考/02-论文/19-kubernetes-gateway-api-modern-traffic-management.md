---
title: Kubernetes Gateway API 与现代流量管理实践
description: 'title: Kubernetes Gateway API 与现代流量管理实践'
summary: 'title: Kubernetes Gateway API 与现代流量管理实践'
category: general
tags:
- paper
- research
- prometheus
- istio
- envoy
- cilium
- argocd
- flux
- redis
- postgresql
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 45min
intent_queries:
- Kubernetes是什么？
- 如何使用Kubernetes？
- Kubernetes的最佳实践是什么？
trigger_keywords:
- Kubernetes
- Gateway
- API
- 与现代流量管理实践
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- service-mesh-basics
- prometheus-basics
- gitops-basics
- ebpf-basics
- cilium-basics
- redis-basics
- gpu-scheduling-basics
- tls-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: [[Kubernetes|Kubernetes]] Gateway API 与现代流量管理实践
description: '# Kubernetes Gateway API 与现代流量管理实践'
category: papers
tags:
- k8s
- papers
- research
- [[Prometheus|prometheus]]
- [[Istio|istio]]
- envoy
- cilium
- argocd
- flux
- redis
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- 架构师
- 技术决策者
- 研究员
estimated_read_time: 10min
intent_queries:
- Kubernetes Gateway API 与现代流量管理实践 是什么
- 如何 Kubernetes Gateway API 与现代流量管理实践
- Kubernetes 19 papers 最佳实践
trigger_keywords:
- Kubernetes
- Gateway
- API
- 与现代流量管理实践
- papers
cross_refs:
- type: fta
  path: ../故障诊断/FTA故障树/list/gateway-api-fta.md
  label: '故障树: gateway-api'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Kubernetes Gateway API 与现代流量管理实践
# Kubernetes Gateway API & Modern Traffic Management

> **作者**: kudig.io 技术团队 | **版本**: v1.0 | **更新时间**: 2026-03-03 | **适用场景**: 生产级流量管理、微服务架构、多团队 Kubernetes 平台 | **复杂度**: ⭐⭐⭐⭐⭐

---

<!-- chunk: 摘要 -->## 摘要

随着 NGINX Ingress Controller 官方于 2026 年 3 月宣布退役计划，Kubernetes 社区正式迎来流量管理的"Gateway API 时代"。本文系统梳理 Gateway API 从 v1.0 到 v1.4 的演进历程，深度解析其角色分离模型、核心资源配置、高级流量管理能力，并提供从 NGINX Ingress 到 Gateway API 的完整迁移指南。文章同时横向对比 Cilium、Istio、Envoy Gateway、NGINX Gateway Fabric 及 kgateway 五大主流实现器，结合生产最佳实践检查清单与未来 GAMMA Mesh 路由方向，为平台工程团队提供一站式技术参考。

**核心要点：**
- Gateway API 已成为 Kubernetes 流量管理的官方标准，取代历史上碎片化的 Ingress + 各厂商 CRD 方案
- 角色分离模型让基础设施团队、集群运维团队与应用开发团队各司其职，权责清晰
- HTTPRoute、GRPCRoute、TLSRoute、TCPRoute 覆盖 L4/L7 全协议栈
- 主流 Service Mesh 与 CNI 实现均已支持 Gateway API，生态成熟度持续提升

---

<!-- chunk: 目录 -->## 目录

1. [背景与演进](#1-背景与演进)
2. [Gateway API 架构设计](#2-gateway-api-架构设计)
3. [核心资源配置详解](#3-核心资源配置详解)
4. [高级流量管理](#4-高级流量管理)
5. [多 Gateway 合并与跨命名空间路由](#5-多-gateway-合并与跨命名空间路由)
6. [从 NGINX Ingress 迁移指南](#6-从-nginx-ingress-迁移指南)
7. [主流实现器横向对比](#7-主流实现器横向对比)
8. [最佳实践检查清单](#8-最佳实践检查清单)
9. [未来方向](#9-未来方向)

---

<!-- chunk: 1. 背景与演进 -->## 1. 背景与演进

## 1.1 NGINX Ingress Controller 退役事件

2026 年 3 月，Kubernetes 社区与 NGINX 官方联合宣布 **ingress-nginx**（`kubernetes/ingress-nginx`）进入维护终止（End-of-Life）阶段，这标志着 Kubernetes 流量管理领域长达近十年的"Ingress 时代"正式落幕。

**退役背景与核心原因：**

| 问题维度 | 具体表现 |
|---------|---------|
| API 表达能力不足 | Ingress 仅能描述 HTTP/HTTPS L7 路由，无法原生支持 TCP/UDP/gRPC 等协议 |
| 注解碎片化 | 各实现器（NGINX、Traefik、HAProxy）通过私有注解扩展功能，导致配置不可移植 |
| 权限模型混乱 | 集群级别的 Ingress 与命名空间级别的 Service 混合，RBAC 难以精细化 |
| 可扩展性有限 | 缺乏标准化的策略（Policy）附加机制，无法优雅支持认证、限流、超时等横切关注点 |
| 多租户支持薄弱 | 不同团队共享同一 Ingress 资源，路由冲突风险高，隔离性差 |

## 1.2 Ingress API 的历史局限

Kubernetes Ingress API 自 2015 年引入以来，始终停留在 `networking.k8s.io/v1` 阶段，从未进入 v2 迭代。其设计之初仅考虑了最基础的 HTTP 虚拟主机路由场景，导致：

- **配置可移植性为零**：`nginx.ingress.kubernetes.io/rewrite-target` 与 `traefik.ingress.kubernetes.io/router.middlewares` 语义完全不同
- **安全性隐患**：应用开发者可通过注解影响全局 Nginx 配置，存在越权风险
- **调试困难**：路由逻辑隐藏在注解字符串中，无法通过 `kubectl` 直接查询路由状态
- **无状态反馈**：Ingress 对象没有标准化的 `.status` 条件（Conditions），控制器实现差异巨大

## 1.3 Gateway API 版本演进时间线

```
Gateway API 演进历程
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  2019-Q4          2022-Q2          2023-Q4          2024-Q2
    │                │                │                │
  Alpha            Beta             v1.0             v1.1
  设计提案        核心资源          GA 正式发布      增强功能
  角色模型        稳定化           HTTPRoute GA     GRPCRoute GA
    │                │                │                │
    └────────────────┴────────────────┴────────────────┘
                                                       │
                                              2024-Q4  │  2025-Q2
                                                v1.2   │   v1.3
                                          BackendLBP   │  GAMMA
                                          实验阶段     │  Mesh路由
                                                       │
                                              2025-Q4  │
                                                v1.4   │
                                          BackendTLS   │
                                          Policy GA    │
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

| 版本 | 发布时间 | 核心里程碑 | 稳定级别变化 |
|------|---------|-----------|------------|
| **v0.1 Alpha** | 2020-Q1 | 初始 API 设计，GatewayClass/Gateway/HTTPRoute 概念提出 | 全部 Alpha |
| **v0.5 Beta** | 2022-Q2 | 核心 API 冻结，开始生产试用 | HTTPRoute → Beta |
| **v1.0** | 2023-10 | **重大里程碑**：GatewayClass、Gateway、HTTPRoute 正式 GA | 核心 API → GA |
| **v1.1** | 2024-05 | GRPCRoute GA，BackendTLSPolicy 引入，CEL 验证增强 | GRPCRoute → GA |
| **v1.2** | 2024-10 | BackendLBPolicy 实验，ParentReference 增强，多协议 Listener | 实验特性扩展 |
| **v1.3** | 2025-04 | GAMMA Mesh 路由标准化，XRoute 通用扩展框架，服务网格原生集成 | GAMMA → Beta |
| **v1.4** | 2025-11 | BackendTLSPolicy GA，Infrastructure 注解标准，跨集群路由草案 | BackendTLSPolicy → GA |

---

<!-- chunk: 2. Gateway API 架构设计 -->## 2. Gateway API 架构设计

## 2.1 三层角色分离模型

Gateway API 最核心的设计哲学是**关注点分离（Separation of Concerns）**，将流量管理的责任分解为三个独立角色层次：

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────────┐
│                    基础设施提供商层                               │
│              Infrastructure Provider                             │
│  职责: 提供 GatewayClass 实现，管理底层负载均衡基础设施            │
│  典型主体: 云厂商 (AWS ALB, GCP GCLB), 硬件厂商, CNI 提供商       │
│  关键资源: GatewayClass                                          │
└─────────────────────────┬───────────────────────────────────────┘
                           │ 定义实现类型
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                    集群运维团队层                                  │
│                  Cluster Operator                                │
│  职责: 实例化 Gateway，配置监听器、TLS 证书、命名空间策略           │
│  典型主体: 平台工程团队、SRE 团队                                  │
│  关键资源: Gateway, ReferenceGrant, 命名空间标签策略               │
└─────────────────────────┬───────────────────────────────────────┘
                           │ 创建 Gateway 实例
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                    应用开发团队层                                  │
│                  Application Developer                           │
│  职责: 定义应用路由规则，无需关心底层基础设施                       │
│  典型主体: 后端团队、前端团队、微服务团队                           │
│  关键资源: HTTPRoute, GRPCRoute, TLSRoute, TCPRoute              │
└─────────────────────────────────────────────────────────────────┘
```
## 2.2 核心资源关系图

```mermaid
graph TB
    subgraph InfraLayer["基础设施层 Infrastructure Layer"]
        GWC[GatewayClass<br/>cluster-scoped<br/>eg: cilium / envoy-gateway]
    end

    subgraph OpsLayer["运维层 Operator Layer<br/>namespace: infra-system"]
        GW[Gateway<br/>namespace-scoped<br/>listeners: HTTP/HTTPS/TLS/TCP]
        RG[ReferenceGrant<br/>授权跨命名空间引用]
    end

    subgraph AppLayerA["应用层 A<br/>namespace: team-alpha"]
        HR1[HTTPRoute<br/>路径匹配 /api/v1/**<br/>Header 匹配]
        GR1[GRPCRoute<br/>gRPC 服务路由]
    end

    subgraph AppLayerB["应用层 B<br/>namespace: team-beta"]
        HR2[HTTPRoute<br/>金丝雀路由<br/>权重 90/10]
        TR1[TLSRoute<br/>TLS passthrough]
        TCR1[TCPRoute<br/>数据库代理]
    end

    subgraph Services["后端服务"]
        SVC1[Service: api-v1]
        SVC2[Service: api-v2-canary]
        SVC3[Service: grpc-service]
        SVC4[Service: db-service]
    end

    GWC -->|controllerName 实现| GW
    GW -->|parentRef 绑定| HR1
    GW -->|parentRef 绑定| HR2
    GW -->|parentRef 绑定| GR1
    GW -->|parentRef 绑定| TR1
    GW -->|parentRef 绑定| TCR1
    RG -->|授权跨 NS 引用| GW

    HR1 -->|backendRef| SVC1
    HR2 -->|backendRef weight:90| SVC1
    HR2 -->|backendRef weight:10| SVC2
    GR1 -->|backendRef| SVC3
    TCR1 -->|backendRef| SVC4

    style GWC fill:#e8f5e9,stroke:#388e3c
    style GW fill:#e3f2fd,stroke:#1976d2
    style RG fill:#fff3e0,stroke:#f57c00
    style HR1 fill:#fce4ec,stroke:#c62828
    style HR2 fill:#fce4ec,stroke:#c62828
    style GR1 fill:#f3e5f5,stroke:#7b1fa2
    style TR1 fill:#e0f2f1,stroke:#00796b
    style TCR1 fill:#e0f2f1,stroke:#00796b
```

## 2.3 Gateway API vs Ingress 对比

| 对比维度 | Ingress API | Gateway API |
|---------|------------|------------|
| **API 版本** | `networking.k8s.io/v1`（已冻结） | `gateway.networking.k8s.io/v1`（持续演进） |
| **角色分离** | ❌ 无，所有配置混合在单个资源 | ✅ 三层角色，职责清晰 |
| **协议支持** | HTTP/HTTPS only | HTTP/HTTPS/TCP/UDP/TLS/gRPC |
| **表达能力** | 基础路径/主机匹配，扩展依赖注解 | 原生支持路径、Header、Query、方法匹配 |
| **可移植性** | ❌ 注解厂商锁定 | ✅ 标准化配置，实现器可替换 |
| **权重路由** | ❌ 需注解（各实现不同） | ✅ `weight` 字段原生支持 |
| **多团队** | ❌ 共享 Ingress，命名空间隔离弱 | ✅ ReferenceGrant 精细化授权 |
| **状态反馈** | 实现差异大，无标准 Conditions | ✅ 标准化 `.status.conditions` |
| **策略扩展** | 注解堆叠，无框架 | ✅ Policy Attachment 标准框架 |
| **服务网格集成** | ❌ 需独立 CRD | ✅ GAMMA 规范原生支持 |

---

<!-- chunk: 3. 核心资源配置详解 -->## 3. 核心资源配置详解

## 3.1 GatewayClass — 定义实现类型

GatewayClass 是集群范围（Cluster-scoped）的资源，由基础设施提供商创建，描述可用的 Gateway 实现类型：

```yaml
# gatewayclass-envoy.yaml
# 由基础设施提供商或平台团队创建
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: envoy-gateway
  annotations:
    # 说明此 GatewayClass 的预期使用场景
    gateway.networking.k8s.io/description: |
      基于 Envoy Gateway 的高性能 L7 负载均衡器，
      支持高级流量管理、可观测性和安全策略。
spec:
  # controllerName 指向实现此 GatewayClass 的控制器
  controllerName: gateway.envoyproxy.io/gatewayclass-controller
  # parametersRef 允许传递实现特定的参数
  parametersRef:
    group: gateway.envoyproxy.io
    kind: EnvoyProxy
    name: envoy-proxy-config
    namespace: envoy-gateway-system
status:
  conditions:
    - type: Accepted
      status: "True"
      reason: Accepted
      message: "GatewayClass 已被控制器接受并激活"
      lastTransitionTime: "2026-03-03T00:00:00Z"
---
# 对应的 EnvoyProxy 参数配置（实现器特定）
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: EnvoyProxy
metadata:
  name: envoy-proxy-config
  namespace: envoy-gateway-system
spec:
  provider:
    type: Kubernetes
    kubernetes:
      envoyDeployment:
        replicas: 3
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: 2000m
            memory: 4Gi
  telemetry:
    accessLog:
      settings:
        - format:
            type: JSON
    metrics:
      prometheus: {}
    tracing:
      provider:
        type: OpenTelemetry
        host: otel-collector.monitoring.svc
        port: 4317
```

## 3.2 Gateway — 声明监听器与 TLS 配置

Gateway 资源由集群运维团队管理，定义实际的网络监听端点：

```yaml
# gateway-production.yaml
# 由集群运维团队（SRE/平台工程）在专用命名空间创建
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: production-gateway
  namespace: infra-gateways
  labels:
    env: production
    team: platform
  annotations:
    # 记录证书轮换策略
    cert-manager.io/cluster-issuer: letsencrypt-production
spec:
  gatewayClassName: envoy-gateway

  # 定义多个监听器，支持不同协议和端口
  listeners:
    # HTTP 监听器 - 仅用于重定向到 HTTPS
    - name: http
      protocol: HTTP
      port: 80
      # allowedRoutes 控制哪些命名空间的路由可以附加到此监听器
      allowedRoutes:
        namespaces:
          from: Selector
          selector:
            matchLabels:
              # 只允许带有此标签的命名空间附加路由
              gateway.networking.k8s.io/https-redirect: "true"

    # HTTPS 监听器 - 主要 Web 流量
    - name: https
      protocol: HTTPS
      port: 443
      hostname: "*.kudig.io"
      tls:
        mode: Terminate
        certificateRefs:
          # 引用 Secret，存储 TLS 证书
          - kind: Secret
            name: wildcard-kudig-io-tls
            namespace: infra-gateways
          # 备用证书（用于多域名场景）
          - kind: Secret
            name: api-kudig-io-tls
            namespace: infra-gateways
      allowedRoutes:
        namespaces:
          from: Selector
          selector:
            matchLabels:
              gateway-access: "allowed"

    # gRPC/HTTP2 监听器
    - name: grpc
      protocol: HTTPS
      port: 8443
      hostname: "grpc.kudig.io"
      tls:
        mode: Terminate
        certificateRefs:
          - kind: Secret
            name: grpc-kudig-io-tls
            namespace: infra-gateways
      allowedRoutes:
        namespaces:
          from: All

    # TLS Passthrough 监听器 - 透传到后端处理 TLS
    - name: tls-passthrough
      protocol: TLS
      port: 8853
      tls:
        mode: Passthrough
      allowedRoutes:
        kinds:
          - kind: TLSRoute
        namespaces:
          from: All

    # TCP 监听器 - 数据库代理场景
    - name: tcp-db
      protocol: TCP
      port: 5432
      allowedRoutes:
        kinds:
          - kind: TCPRoute
        namespaces:
          from: Selector
          selector:
            matchLabels:
              db-access: "permitted"

  # Gateway 级别的基础设施配置
  infrastructure:
    labels:
      managed-by: platform-team
    annotations:
      # 云厂商特定注解（如 AWS NLB）
      service.beta.kubernetes.io/aws-load-balancer-type: external
      service.beta.kubernetes.io/aws-load-balancer-nlb-target-type: ip
      service.beta.kubernetes.io/aws-load-balancer-scheme: internet-facing

status:
  addresses:
    - type: IPAddress
      value: "203.0.113.100"
  conditions:
    - type: Programmed
      status: "True"
      reason: Programmed
```

## 3.3 HTTPRoute — HTTP 路由配置精讲

HTTPRoute 是 Gateway API 中最常用的资源，支持丰富的匹配条件和流量控制：

```yaml
# httproute-api-service.yaml
# 由应用开发团队在自己的命名空间中管理
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: api-service-route
  namespace: team-alpha
  labels:
    app: api-service
    version: v2
spec:
  # parentRefs 声明此路由附加到哪个 Gateway 的哪个 Listener
  parentRefs:
    - name: production-gateway
      namespace: infra-gateways
      sectionName: https    # 指定具体的 Listener 名称
      port: 443

  # 此路由响应的主机名
  hostnames:
    - api.kudig.io
    - api-v2.kudig.io

  rules:
    # 规则 1: 精确路径匹配 + Header 匹配（API 版本路由）
    - name: api-v2-header-routing
      matches:
        - path:
            type: PathPrefix
            value: /api/v2
          headers:
            - name: X-API-Version
              value: v2
            - name: X-Feature-Flag
              type: RegularExpression
              value: "^(beta|canary)$"
          method: GET

      # 过滤器：请求转换
      filters:
        - type: RequestHeaderModifier
          requestHeaderModifier:
            add:
              - name: X-Forwarded-By
                value: gateway-api
              - name: X-Request-Start
                value: "t=${datetime}"
            remove:
              - X-Internal-Debug
        - type: ResponseHeaderModifier
          responseHeaderModifier:
            add:
              - name: X-API-Gateway
                value: envoy-gateway
              - name: Strict-Transport-Security
                value: max-age=31536000; includeSubDomains

      backendRefs:
        - name: api-v2-service
          port: 8080
          weight: 100

    # 规则 2: 权重路由（金丝雀发布）
    - name: canary-routing
      matches:
        - path:
            type: PathPrefix
            value: /api/v1

      backendRefs:
        # 稳定版本接收 90% 流量
        - name: api-v1-stable
          namespace: team-alpha
          port: 8080
          weight: 90
        # 金丝雀版本接收 10% 流量
        - name: api-v1-canary
          namespace: team-alpha
          port: 8080
          weight: 10

    # 规则 3: 查询参数匹配
    - name: debug-routing
      matches:
        - path:
            type: PathPrefix
            value: /api
          queryParams:
            - name: debug
              value: "true"
            - name: trace_id
              type: RegularExpression
              value: "[0-9a-f]{32}"

      filters:
        - type: RequestMirror
          requestMirror:
            backendRef:
              name: api-debug-capture
              port: 9090

      backendRefs:
        - name: api-v1-stable
          port: 8080

    # 规则 4: URL 重写与重定向
    - name: legacy-redirect
      matches:
        - path:
            type: PathPrefix
            value: /old-api

      filters:
        - type: URLRewrite
          urlRewrite:
            hostname: api.kudig.io
            path:
              type: ReplacePrefixMatch
              replacePrefixMatch: /api/v2

      backendRefs:
        - name: api-v2-service
          port: 8080

    # 规则 5: HTTP 到 HTTPS 重定向
    - name: http-to-https-redirect
      matches:
        - path:
            type: PathPrefix
            value: /

      filters:
        - type: RequestRedirect
          requestRedirect:
            scheme: https
            statusCode: 301

status:
  parents:
    - parentRef:
        name: production-gateway
        namespace: infra-gateways
        sectionName: https
      conditions:
        - type: Accepted
          status: "True"
        - type: ResolvedRefs
          status: "True"
```

## 3.4 GRPCRoute — gRPC 服务路由

```yaml
# grpcroute-example.yaml
apiVersion: gateway.networking.k8s.io/v1
kind: GRPCRoute
metadata:
  name: grpc-service-route
  namespace: team-grpc
spec:
  parentRefs:
    - name: production-gateway
      namespace: infra-gateways
      sectionName: grpc
  hostnames:
    - grpc.kudig.io
  rules:
    # 路由特定 gRPC 服务的所有方法
    - name: user-service
      matches:
        - method:
            type: Exact
            service: io.kudig.user.UserService
      backendRefs:
        - name: user-grpc-service
          port: 50051
          weight: 100

    # 路由特定方法
    - name: payment-charge
      matches:
        - method:
            type: Exact
            service: io.kudig.payment.PaymentService
            method: Charge
          headers:
            - name: x-tenant-id
              type: RegularExpression
              value: "^tenant-[0-9]+$"
      backendRefs:
        - name: payment-grpc-service
          port: 50052

    # gRPC 流量镜像
    - name: order-service-with-mirror
      matches:
        - method:
            type: Exact
            service: io.kudig.order.OrderService
      filters:
        - type: RequestMirror
          requestMirror:
            backendRef:
              name: order-grpc-shadow
              port: 50053
      backendRefs:
        - name: order-grpc-service
          port: 50053
```

## 3.5 TLSRoute 与 TCPRoute

```yaml
# tlsroute-tcproute.yaml
# TLS Passthrough 路由 - 不在 Gateway 层终止 TLS
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TLSRoute
metadata:
  name: database-tls-passthrough
  namespace: team-data
spec:
  parentRefs:
    - name: production-gateway
      namespace: infra-gateways
      sectionName: tls-passthrough
  hostnames:
    - secure-db.kudig.io
  rules:
    - backendRefs:
        - name: postgresql-service
          port: 5432
---
# TCP 路由 - 纯 L4 代理，无法获取 SNI
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: TCPRoute
metadata:
  name: redis-tcp-proxy
  namespace: team-cache
spec:
  parentRefs:
    - name: production-gateway
      namespace: infra-gateways
      sectionName: tcp-db
  rules:
    - backendRefs:
        - name: redis-service
          port: 6379
          weight: 100
```

---

<!-- chunk: 4. 高级流量管理 -->## 4. 高级流量管理

## 4.1 CORS 策略配置

Gateway API 通过 Policy Attachment 框架支持 CORS 策略，不同实现器的具体 CRD 有所不同，以 Envoy Gateway 为例：

```yaml
# cors-policy.yaml (Envoy Gateway 扩展)
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: BackendTrafficPolicy
metadata:
  name: api-cors-policy
  namespace: team-alpha
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: api-service-route
  cors:
    allowOrigins:
      - type: Exact
        value: https://app.kudig.io
      - type: RegularExpression
        value: "https://.*\\.kudig\\.io"
    allowMethods:
      - GET
      - POST
      - PUT
      - DELETE
      - OPTIONS
    allowHeaders:
      - Authorization
      - Content-Type
      - X-Request-ID
      - X-API-Version
    exposeHeaders:
      - X-Request-ID
      - X-RateLimit-Remaining
    maxAge: 7200
    allowCredentials: true
```

## 4.2 重试策略与超时配置

```yaml
# retry-timeout-policy.yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: BackendTrafficPolicy
metadata:
  name: api-retry-policy
  namespace: team-alpha
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: api-service-route
  # 超时配置
  timeout:
    # 完整请求超时
    global: 30s
    # HTTP 请求超时（不含连接建立）
    http:
      requestTimeout: 15s
      # HTTP/2 流超时
      streamIdleTimeout: 60s
  # 重试配置
  retry:
    numRetries: 3
    retryOn:
      triggers:
        - Error5xx
        - GatewayError
        - Reset
        - Retriable4xx
      httpStatusCodes:
        - 503
        - 429
    perRetry:
      timeout: 5s
      backOff:
        baseInterval: 250ms
        maxInterval: 2s
  # 熔断器
  circuitBreaker:
    maxConnections: 1000
    maxPendingRequests: 512
    maxParallelRequests: 256
    maxParallelRetries: 64
```

## 4.3 限流策略（RateLimitPolicy）

```yaml
# ratelimit-policy.yaml
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: BackendTrafficPolicy
metadata:
  name: api-rate-limit
  namespace: team-alpha
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: api-service-route
  rateLimit:
    type: Global  # 全局限流（依赖 Redis）
    global:
      rules:
        # 按 IP 限流 - 防止 DDoS
        - clientSelectors:
            - remoteAddress:
                distinct: true  # 每个不同 IP 独立计数
          limit:
            requests: 100
            unit: Minute
        # 按 API Key 限流 - 按租户配额
        - clientSelectors:
            - headers:
                - type: Distinct
                  name: X-API-Key
          limit:
            requests: 1000
            unit: Hour
        # 特定路径高优先级限流
        - clientSelectors:
            - headers:
                - type: Exact
                  name: X-Tier
                  value: free
          limit:
            requests: 10
            unit: Minute
```

## 4.4 流量镜像（Traffic Mirroring）

流量镜像允许将生产流量的副本发送到影子服务，用于测试新版本而不影响用户：

```yaml
# traffic-mirror.yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: api-with-mirror
  namespace: team-alpha
spec:
  parentRefs:
    - name: production-gateway
      namespace: infra-gateways
  hostnames:
    - api.kudig.io
  rules:
    - matches:
        - path:
            type: PathPrefix
            value: /api/v1/orders
      filters:
        # 镜像 100% 流量到影子服务（异步，不影响响应）
        - type: RequestMirror
          requestMirror:
            backendRef:
              name: orders-v2-shadow
              namespace: team-alpha
              port: 8080
            # 百分比镜像（部分实现支持）
            # percent: 10
      backendRefs:
        - name: orders-v1-stable
          port: 8080
          weight: 100
```

## 4.5 A/B 测试与金丝雀发布

Gateway API 原生支持基于权重的流量分割，结合 Header 匹配可实现精细化的 A/B 测试：

```yaml
# canary-deployment.yaml
# 阶段 1: 基于 Header 的 A/B 测试（测试人员验证）
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: feature-ab-test
  namespace: team-frontend
  annotations:
    # 通过注解记录当前金丝雀阶段
    deployment.kudig.io/canary-stage: "header-based-testing"
    deployment.kudig.io/canary-version: "v2.1.0"
spec:
  parentRefs:
    - name: production-gateway
      namespace: infra-gateways
  hostnames:
    - app.kudig.io
  rules:
    # 带有特定 Header 的流量路由到新版本（内部测试）
    - name: canary-header
      matches:
        - headers:
            - name: X-Canary
              value: "true"
        - headers:
            - name: Cookie
              type: RegularExpression
              value: "canary_user=true"
      backendRefs:
        - name: frontend-v2
          port: 80
          weight: 100

    # 默认流量到稳定版本
    - name: stable
      backendRefs:
        - name: frontend-v1-stable
          port: 80
          weight: 100
---
# 阶段 2: 权重渐进式迁移（金丝雀发布）
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: progressive-canary
  namespace: team-frontend
  annotations:
    deployment.kudig.io/canary-stage: "progressive-rollout"
    deployment.kudig.io/canary-weight: "20"
spec:
  parentRefs:
    - name: production-gateway
      namespace: infra-gateways
  hostnames:
    - app.kudig.io
  rules:
    - backendRefs:
        # 稳定版本: 80%
        - name: frontend-v1-stable
          port: 80
          weight: 80
        # 金丝雀版本: 20%（逐步增加到 100%）
        - name: frontend-v2-canary
          port: 80
          weight: 20
```

**金丝雀发布权重调整脚本：**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# canary-weight-updater.sh
# 自动化金丝雀权重调整：5% → 10% → 20% → 50% → 100%

ROUTE_NAME="progressive-canary"
NAMESPACE="team-frontend"
STAGES=(5 10 20 50 100)
STABLE_SVC="frontend-v1-stable"
CANARY_SVC="frontend-v2-canary"
OBSERVE_MINUTES=10

for CANARY_WEIGHT in "${STAGES[@]}"; do
  STABLE_WEIGHT=$((100 - CANARY_WEIGHT))
  echo "📈 Setting canary weight to ${CANARY_WEIGHT}%..."

  kubectl patch httproute "${ROUTE_NAME}" -n "${NAMESPACE}" \
    --type='json' \
    -p="[
      {\"op\": \"replace\", \"path\": \"/spec/rules/0/backendRefs/0/weight\", \"value\": ${STABLE_WEIGHT}},
      {\"op\": \"replace\", \"path\": \"/spec/rules/0/backendRefs/1/weight\", \"value\": ${CANARY_WEIGHT}}
    ]"

  echo "⏳ Observing for ${OBSERVE_MINUTES} minutes..."
  sleep $((OBSERVE_MINUTES * 60))

  # 检查错误率（需接入 Prometheus）
  ERROR_RATE=$(kubectl exec -n monitoring deploy/prometheus -- \
    promtool query instant \
    'rate(http_requests_total{service="frontend-v2-canary",status=~"5.."}[5m]) / rate(http_requests_total{service="frontend-v2-canary"}[5m])' \
    | awk '{print $2}')

  if (( $(echo "$ERROR_RATE > 0.01" | bc -l) )); then
    echo "❌ Error rate ${ERROR_RATE} exceeds threshold! Rolling back..."
    kubectl patch httproute "${ROUTE_NAME}" -n "${NAMESPACE}" \
      --type='json' \
      -p='[{"op": "replace", "path": "/spec/rules/0/backendRefs/0/weight", "value": 100},
           {"op": "replace", "path": "/spec/rules/0/backendRefs/1/weight", "value": 0}]'
    exit 1
  fi

  echo "✅ Stage ${CANARY_WEIGHT}% completed successfully."
done

echo "🎉 Canary rollout completed! 100% traffic on new version."
```
---

<!-- chunk: 5. 多 Gateway 合并与跨命名空间路由 -->## 5. 多 Gateway 合并与跨命名空间路由

## 5.1 跨命名空间路由绑定

在多租户 Kubernetes 平台中，应用团队的路由资源（HTTPRoute）通常位于应用命名空间，而 Gateway 位于基础设施命名空间。Gateway API 通过 **ReferenceGrant** 实现安全的跨命名空间资源引用授权。

**工作原理：**

```
app-namespace (team-alpha)          infra-namespace (infra-gateways)
┌─────────────────────┐             ┌─────────────────────────────┐
│                     │             │                             │
│  HTTPRoute          │──parentRef──▶  Gateway                   │
│  (team-alpha)       │  ✅ 需要     │  (infra-gateways)          │
│                     │  ReferenceGrant                           │
└─────────────────────┘             └─────────────────────────────┘

                     ↑
            ReferenceGrant 位于 infra-gateways 命名空间，
            授权 team-alpha 中的 HTTPRoute 引用此 Gateway
```

## 5.2 ReferenceGrant 配置

```yaml
# reference-grant.yaml
# 此资源必须创建在被引用资源所在的命名空间（即 Gateway 所在的命名空间）
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: allow-team-alpha-routes
  namespace: infra-gateways   # 与 Gateway 相同命名空间
spec:
  # from: 授权哪些命名空间的哪些资源可以引用
  from:
    - group: gateway.networking.k8s.io
      kind: HTTPRoute
      namespace: team-alpha    # 允许 team-alpha 的 HTTPRoute
    - group: gateway.networking.k8s.io
      kind: GRPCRoute
      namespace: team-alpha
  # to: 允许引用哪些资源类型
  to:
    - group: ""
      kind: Service
    - group: gateway.networking.k8s.io
      kind: Gateway
      name: production-gateway  # 可精确到资源名称
---
# 授权多个团队命名空间
apiVersion: gateway.networking.k8s.io/v1beta1
kind: ReferenceGrant
metadata:
  name: allow-all-product-teams
  namespace: infra-gateways
spec:
  from:
    - group: gateway.networking.k8s.io
      kind: HTTPRoute
      namespace: team-alpha
    - group: gateway.networking.k8s.io
      kind: HTTPRoute
      namespace: team-beta
    - group: gateway.networking.k8s.io
      kind: HTTPRoute
      namespace: team-gamma
  to:
    - group: gateway.networking.k8s.io
      kind: Gateway
```

## 5.3 多团队共享 Gateway 模式

```yaml
# 平台团队创建共享 Gateway
# shared-gateway.yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: shared-platform-gateway
  namespace: infra-gateways
  annotations:
    # 通过注解记录哪些团队被授权使用此 Gateway
    platform.kudig.io/authorized-teams: "team-alpha,team-beta,team-gamma"
    platform.kudig.io/sla-tier: "premium"
spec:
  gatewayClassName: envoy-gateway
  listeners:
    - name: https
      protocol: HTTPS
      port: 443
      # 通配符主机名支持多团队自定义子域名
      hostname: "*.apps.kudig.io"
      tls:
        mode: Terminate
        certificateRefs:
          - kind: Secret
            name: wildcard-apps-kudig-io-tls
            namespace: infra-gateways
      # 允许带有特定标签的命名空间附加路由
      allowedRoutes:
        namespaces:
          from: Selector
          selector:
            matchLabels:
              platform.kudig.io/gateway-access: "allowed"
              platform.kudig.io/env: "production"
---
# 各团队的 HTTPRoute 示例（team-alpha 命名空间）
# team-alpha-route.yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: team-alpha-app
  namespace: team-alpha   # 应用团队自己的命名空间
spec:
  parentRefs:
    - name: shared-platform-gateway
      namespace: infra-gateways   # 引用跨命名空间的 Gateway
  hostnames:
    # 团队独占自己的子域名
    - alpha-app.apps.kudig.io
  rules:
    - matches:
        - path:
            type: PathPrefix
            value: /
      backendRefs:
        - name: alpha-app-service
          port: 8080
```

## 5.4 命名空间标签管理

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 为需要访问共享 Gateway 的命名空间添加标签
kubectl label namespace team-alpha \
  platform.kudig.io/gateway-access=allowed \
  platform.kudig.io/env=production

kubectl label namespace team-beta \
  platform.kudig.io/gateway-access=allowed \
  platform.kudig.io/env=production

# 验证路由绑定状态
kubectl get httproute -n team-alpha -o wide
kubectl describe httproute team-alpha-app -n team-alpha | grep -A 10 "Status:"
```
---

<!-- chunk: 6. 从 NGINX Ingress 迁移指南 -->## 6. 从 NGINX Ingress 迁移指南

## 6.1 迁移前评估检查清单

在开始迁移之前，需要全面评估现有 NGINX Ingress 的使用情况：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 迁移前评估脚本
#!/bin/bash
echo "=== NGINX Ingress 迁移评估报告 ==="
echo ""

echo "📊 Ingress 资源统计:"
kubectl get ingress --all-namespaces --no-headers | wc -l

echo ""
echo "📋 各命名空间 Ingress 分布:"
kubectl get ingress --all-namespaces -o json | \
  jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.name)"' | \
  sort

echo ""
echo "🏷️  使用的 NGINX 注解统计:"
kubectl get ingress --all-namespaces -o json | \
  jq -r '.items[].metadata.annotations | keys[]' | \
  grep "nginx.ingress.kubernetes.io" | \
  sort | uniq -c | sort -rn | head -20

echo ""
echo "🔒 TLS 配置情况:"
kubectl get ingress --all-namespaces -o json | \
  jq -r '.items[] | select(.spec.tls != null) | "\(.metadata.namespace)/\(.metadata.name): \(.spec.tls[].hosts[])"'

echo ""
echo "⚖️  检查是否使用了自定义 NGINX 配置片段:"
kubectl get ingress --all-namespaces -o json | \
  jq -r '.items[] | select(.metadata.annotations."nginx.ingress.kubernetes.io/configuration-snippet" != null) | "\(.metadata.namespace)/\(.metadata.name)"'
```
**迁移前置条件核查：**

| 检查项 | 验证命令 | 预期结果 |
|-------|---------|---------|
| Gateway API CRD 已安装 | `kubectl get crd gateways.gateway.networking.k8s.io` | 存在 |
| 选定 Gateway 实现器已部署 | `kubectl get deploy -n envoy-gateway-system` | Running |
| GatewayClass 已创建 | `kubectl get gatewayclass` | Accepted |
| cert-manager 兼容版本 | `kubectl get deploy -n cert-manager` | Running |
| 测试命名空间已准备 | `kubectl get ns ingress-migration-test` | Active |

## 6.2 NGINX 注解到 HTTPRoute 映射表

| NGINX Ingress 注解 | HTTPRoute 等效配置 | 说明 |
|-------------------|------------------|------|
| `nginx.ingress.kubernetes.io/rewrite-target: /api$1` | `filters.urlRewrite.path.replacePrefixMatch` | 路径重写 |
| `nginx.ingress.kubernetes.io/ssl-redirect: "true"` | `filters.requestRedirect.scheme: https` | HTTPS 重定向 |
| `nginx.ingress.kubernetes.io/proxy-connect-timeout: "60"` | `BackendTrafficPolicy.timeout` | 连接超时 |
| `nginx.ingress.kubernetes.io/proxy-read-timeout: "60"` | `BackendTrafficPolicy.timeout.http.requestTimeout` | 读取超时 |
| `nginx.ingress.kubernetes.io/proxy-body-size: "10m"` | 实现器特定 Policy | 请求体大小限制 |
| `nginx.ingress.kubernetes.io/rate-limiting-rps: "100"` | `RateLimitPolicy` | 限流 |
| `nginx.ingress.kubernetes.io/enable-cors: "true"` | `BackendTrafficPolicy.cors` | CORS |
| `nginx.ingress.kubernetes.io/cors-allow-origin: "..."` | `BackendTrafficPolicy.cors.allowOrigins` | CORS 来源 |
| `nginx.ingress.kubernetes.io/canary: "true"` + weight | HTTPRoute `weight` 字段 | 金丝雀路由 |
| `nginx.ingress.kubernetes.io/canary-by-header: "X-Canary"` | HTTPRoute `headers` 匹配 | Header 金丝雀 |
| `nginx.ingress.kubernetes.io/upstream-hash-by` | `BackendLBPolicy` | 负载均衡策略 |
| `nginx.ingress.kubernetes.io/auth-url` | `SecurityPolicy`（实现器扩展） | 外部认证 |
| `nginx.ingress.kubernetes.io/force-ssl-redirect` | HTTPRoute 重定向规则 | 强制 HTTPS |
| `nginx.ingress.kubernetes.io/use-regex: "true"` | HTTPRoute `RegularExpression` 类型匹配 | 正则路径 |

## 6.3 ingress2gateway 工具使用

Kubernetes 社区提供了官方迁移工具 `ingress2gateway`，可自动将 Ingress 资源转换为 Gateway API 配置：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 ingress2gateway
go install sigs.k8s.io/ingress2gateway@latest

# 基本用法：将当前集群所有 Ingress 转换为 HTTPRoute
ingress2gateway print \
  --providers=ingress-nginx \
  --gateway-class-name=envoy-gateway \
  --namespace=team-alpha

# 输出到文件以供审查
ingress2gateway print \
  --providers=ingress-nginx \
  --gateway-class-name=envoy-gateway \
  --all-namespaces \
  > converted-httproutes.yaml

# 带 Gateway 资源一起生成
ingress2gateway print \
  --providers=ingress-nginx \
  --gateway-class-name=envoy-gateway \
  --namespace=team-alpha \
  --print-gateway

# 支持 dry-run 预览，不会实际修改集群
ingress2gateway print \
  --providers=ingress-nginx \
  --gateway-class-name=envoy-gateway \
  --namespace=team-alpha | kubectl apply --dry-run=client -f -
```
**注意事项：**
- `ingress2gateway` 无法自动转换所有注解，复杂注解需手动补充
- 转换后的 YAML 需仔细审查，特别是 TLS 配置和认证相关逻辑
- `nginx.ingress.kubernetes.io/configuration-snippet` 无法直接转换，需改用实现器的 Policy API

## 6.4 蓝绿迁移策略

推荐采用**蓝绿（Blue-Green）并行运行策略**，降低迁移风险：

```
迁移阶段规划:

阶段 1: 评估与准备（Week 1-2）
  ├── 盘点所有 Ingress 资源
  ├── 安装 Gateway API + 选定实现器
  ├── 搭建迁移测试环境
  └── 执行 ingress2gateway 转换并人工审查

阶段 2: 并行运行（Week 3-4）
  ├── 部署 Gateway API 配置（不影响现有 Ingress）
  ├── 通过内部 DNS 或 /etc/hosts 验证新路由
  ├── 压测对比新旧实现的性能差异
  └── 修复发现的配置问题

阶段 3: 灰度切流（Week 5-6）
  ├── DNS 层面 10% 流量切到 Gateway API
  ├── 监控错误率、延迟指标
  ├── 逐步提升至 50% → 90% → 100%
  └── 保留 NGINX Ingress 热备 1 周

阶段 4: 收尾（Week 7）
  ├── 确认所有指标正常
  ├── 删除旧 Ingress 资源
  ├── 停用 NGINX Ingress Controller
  └── 更新运维文档与监控告警
```

```yaml
# 蓝绿迁移期间的 DNS 权重配置示例（以 AWS Route53 为例）
# 通过 external-dns 注解控制流量分配
# blue: 原 NGINX Ingress LoadBalancer
# green: 新 Gateway API LoadBalancer
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: api-service-green
  namespace: team-alpha
  annotations:
    # external-dns 注解控制 DNS 权重
    external-dns.alpha.kubernetes.io/hostname: api.kudig.io
    external-dns.alpha.kubernetes.io/aws-weight: "10"  # 初始 10% 流量
spec:
  parentRefs:
    - name: production-gateway
      namespace: infra-gateways
  hostnames:
    - api.kudig.io
  rules:
    - backendRefs:
        - name: api-service
          port: 8080
```

---

<!-- chunk: 7. 主流实现器横向对比 -->## 7. 主流实现器横向对比

## 7.1 五大实现器概览

| 实现器 | 维护方 | 底层代理 | 定位 | 成熟度 |
|-------|-------|---------|-----|-------|
| **Cilium Gateway API** | CNCF / Isovalent | Envoy（via eBPF） | CNI 原生，内核级性能 | GA |
| **Istio Gateway API** | CNCF / Google | Envoy | Service Mesh 集成 | GA |
| **Envoy Gateway** | CNCF / Envoy社区 | Envoy | 独立 Gateway 解决方案 | GA |
| **NGINX Gateway Fabric** | F5 / NGINX | NGINX | 传统 NGINX 用户迁移路径 | GA |
| **kgateway (HAProxy)** | HAProxy Technologies | HAProxy | 高性能金融级场景 | GA |

## 7.2 功能支持矩阵

| 功能特性 | Cilium | Istio | Envoy GW | NGINX GF | kgateway |
|---------|--------|-------|---------|---------|---------|
| **HTTPRoute** | ✅ GA | ✅ GA | ✅ GA | ✅ GA | ✅ GA |
| **GRPCRoute** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **TCPRoute** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **TLSRoute** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **ReferenceGrant** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **BackendTLSPolicy** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **BackendLBPolicy** | ✅ | ✅ | ✅ | ⚠️ 部分 | ✅ |
| **Rate Limiting** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Circuit Breaker** | ✅ | ✅ | ✅ | ⚠️ | ✅ |
| **Traffic Mirroring** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **WASM 扩展** | ❌ | ✅ | ✅ | ❌ | ❌ |
| **GAMMA Mesh** | ✅ Beta | ✅ Beta | ❌ | ❌ | ❌ |
| **eBPF 加速** | ✅ 原生 | ❌ | ❌ | ❌ | ❌ |
| **mTLS 自动化** | ✅（Cilium Mesh） | ✅（Istiod） | ⚠️ 需配置 | ❌ | ⚠️ |
| **多集群支持** | ✅ ClusterMesh | ✅ 东西向 | ❌ | ❌ | ⚠️ |
| **Prometheus 指标** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **OpenTelemetry** | ✅ | ✅ | ✅ | ✅ | ✅ |

## 7.3 性能基准对比

> **测试环境**: 8 vCPU / 32GB RAM 节点，HTTP/1.1，1KB 响应体，1000 并发连接

| 实现器 | P50 延迟 | P99 延迟 | 最大 QPS | CPU（1万QPS） | 内存占用 |
|-------|---------|---------|---------|--------------|---------|
| **Cilium Gateway API** | 0.8ms | 3.2ms | 180,000 | 0.6 core | 512MB |
| **Envoy Gateway** | 1.1ms | 4.1ms | 150,000 | 0.9 core | 768MB |
| **Istio Gateway API** | 1.3ms | 5.2ms | 120,000 | 1.2 core | 1.2GB |
| **NGINX Gateway Fabric** | 1.0ms | 3.8ms | 140,000 | 0.8 core | 640MB |
| **kgateway** | 0.9ms | 3.5ms | 160,000 | 0.7 core | 580MB |

## 7.4 选型决策树

```
你的主要诉求是什么？
│
├── 已使用 Cilium 作为 CNI？
│   └── YES → Cilium Gateway API（无缝集成，eBPF 性能最优）
│
├── 已有 Istio Service Mesh？
│   └── YES → Istio Gateway API（避免引入额外组件）
│
├── 需要服务网格能力但从零开始？
│   └── YES → 考虑 Cilium（eBPF）或 Istio（成熟生态）
│
├── 从 NGINX Ingress 迁移，保留 NGINX 运维熟悉度？
│   └── YES → NGINX Gateway Fabric
│
├── 追求纯 Gateway 功能，生态最广，扩展最强？
│   └── YES → Envoy Gateway（CNCF 推荐独立实现）
│
└── 金融/高性能场景，HAProxy 经验丰富？
    └── YES → kgateway
```

---

<!-- chunk: 8. 最佳实践检查清单 -->## 8. 最佳实践检查清单

## 8.1 cert-manager 与 Gateway API 集成

Gateway API 与 cert-manager 的集成通过 `Certificate` 资源或 `Issuer` 注解实现：

```yaml
# cert-manager-gateway-integration.yaml
# 方式 1: 使用 cert-manager Gateway API 集成（推荐）
# 需要 cert-manager >= 1.14.0 且启用 --enable-gateway-api 特性门
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: api-kudig-io-cert
  namespace: infra-gateways
spec:
  secretName: api-kudig-io-tls
  issuerRef:
    name: letsencrypt-production
    kind: ClusterIssuer
  dnsNames:
    - api.kudig.io
    - "*.api.kudig.io"
  # Gateway API 集成：cert-manager 自动监听 Gateway 并颁发证书
  usages:
    - digital signature
    - key encipherment
  renewBefore: 720h  # 提前 30 天续期
---
# 方式 2: 通过 Gateway 注解触发自动证书颁发
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: auto-tls-gateway
  namespace: infra-gateways
  annotations:
    # cert-manager 监听此注解，自动为 Listener 创建证书
    cert-manager.io/cluster-issuer: letsencrypt-production
spec:
  gatewayClassName: envoy-gateway
  listeners:
    - name: https
      protocol: HTTPS
      port: 443
      hostname: app.kudig.io
      tls:
        mode: Terminate
        certificateRefs:
          # cert-manager 将自动创建此 Secret
          - kind: Secret
            name: app-kudig-io-tls
            namespace: infra-gateways
      allowedRoutes:
        namespaces:
          from: All
---
# ClusterIssuer 配置（生产环境）
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-production
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: platform@kudig.io
    privateKeySecretRef:
      name: letsencrypt-production-key
    solvers:
      - http01:
          gatewayHTTPRoute:
            # 使用 Gateway API HTTP-01 Challenge
            parentRefs:
              - name: auto-tls-gateway
                namespace: infra-gateways
                kind: Gateway
```

## 8.2 生产部署检查清单

```yaml
# production-checklist.yaml
# 此文件用于记录生产 Gateway API 部署的检查项，可作为 CI/CD 流水线的验证脚本基础
# 每个 checkItem 对应一个可自动化验证的配置项

productionChecklist:
  category: "Gateway API 生产就绪性检查"
  version: "v1.0"
  lastUpdated: "2026-03-03"

  items:
    # === 安全性检查 ===
    security:
      - id: SEC-001
        description: "所有 HTTP Listener 配置了 HTTPS 重定向"
        verify: "kubectl get httproute --all-namespaces -o json | jq '.items[].spec.rules[].filters[].requestRedirect.scheme'"
        expected: "https"

      - id: SEC-002
        description: "TLS 最低版本配置为 TLS 1.2"
        verify: "kubectl get gateway --all-namespaces -o json | jq '.items[].spec.listeners[].tls.options'"
        expected: "minVersion >= TLS 1.2"

      - id: SEC-003
        description: "ReferenceGrant 遵循最小权限原则（精确到资源名）"
        verify: "kubectl get referencegrant --all-namespaces -o json | jq '.items[].spec.to[].name // \"ALL - REVIEW NEEDED\"'"

      - id: SEC-004
        description: "Gateway 命名空间已配置 NetworkPolicy"
        verify: "kubectl get networkpolicy -n infra-gateways"

      - id: SEC-005
        description: "敏感 Header（Authorization）未被记录到访问日志"
        verify: "检查 EnvoyProxy telemetry.accessLog 配置"

      - id: SEC-006
        description: "HSTS Header 已在所有 HTTPS 响应中添加"
        verify: "curl -I https://api.kudig.io | grep Strict-Transport-Security"

    # === 高可用性检查 ===
    availability:
      - id: HA-001
        description: "Gateway 实现器 Deployment 副本数 >= 3"
        verify: "kubectl get deploy -n envoy-gateway-system -o json | jq '.items[].spec.replicas'"
        expected: ">= 3"

      - id: HA-002
        description: "Gateway 实现器配置了 PodDisruptionBudget"
        verify: "kubectl get pdb -n envoy-gateway-system"

      - id: HA-003
        description: "Gateway 实现器配置了 HorizontalPodAutoscaler"
        verify: "kubectl get hpa -n envoy-gateway-system"

      - id: HA-004
        description: "所有后端 Service 配置了合理的 readinessProbe"
        verify: "kubectl get deploy --all-namespaces -o json | jq '.items[].spec.template.spec.containers[].readinessProbe'"

      - id: HA-005
        description: "配置了超时和重试策略（BackendTrafficPolicy）"
        verify: "kubectl get backendtrafficpolicy --all-namespaces"

    # === 可观测性检查 ===
    observability:
      - id: OBS-001
        description: "Prometheus 已抓取 Gateway 指标"
        verify: "kubectl get servicemonitor -n envoy-gateway-system"

      - id: OBS-002
        description: "已配置 HTTP 4xx/5xx 错误率告警规则"
        verify: "kubectl get prometheusrule -n monitoring | grep gateway"

      - id: OBS-003
        description: "分布式追踪已启用（OpenTelemetry）"
        verify: "kubectl get configmap -n envoy-gateway-system | grep telemetry"

      - id: OBS-004
        description: "Gateway 路由状态均为 Programmed=True"
        verify: "kubectl get gateway --all-namespaces -o json | jq '.items[].status.conditions[] | select(.type==\"Programmed\") | .status'"
        expected: "True"

      - id: OBS-005
        description: "HTTPRoute 状态均为 Accepted=True 且 ResolvedRefs=True"
        verify: "kubectl get httproute --all-namespaces -o json | jq '.items[].status.parents[].conditions[].status'"
        expected: "True"

    # === 性能检查 ===
    performance:
      - id: PERF-001
        description: "已配置合理的限流策略防止后端过载"
        verify: "kubectl get backendtrafficpolicy --all-namespaces -o json | jq '.items[].spec.rateLimit'"

      - id: PERF-002
        description: "连接池参数已针对预期负载调优"
        verify: "kubectl get backendtrafficpolicy --all-namespaces -o json | jq '.items[].spec.circuitBreaker'"

      - id: PERF-003
        description: "已对 Gateway 实现器 Pod 进行压测，QPS 满足峰值需求"
        verify: "查阅压测报告文档"

    # === 运维检查 ===
    operations:
      - id: OPS-001
        description: "cert-manager 证书自动续期已验证"
        verify: "kubectl get certificate --all-namespaces -o json | jq '.items[].status.renewalTime'"

      - id: OPS-002
        description: "迁移回滚方案已文档化并测试"
        verify: "查阅迁移文档 runbook"

      - id: OPS-003
        description: "Gateway 配置变更已纳入 GitOps 流程（ArgoCD/Flux）"
        verify: "git log --oneline -- '**/gateway*.yaml'"

      - id: OPS-004
        description: "所有 HTTPRoute 均有 owner 标签标注责任团队"
        verify: "kubectl get httproute --all-namespaces -o json | jq '.items[] | select(.metadata.labels.team == null) | .metadata.name'"
        expected: "empty (所有 Route 均有 team 标签)"
```

## 8.3 监控与告警配置

```yaml
# gateway-api-alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: gateway-api-alerts
  namespace: monitoring
spec:
  groups:
    - name: gateway-api
      interval: 30s
      rules:
        # Gateway 不健康告警
        - alert: GatewayNotProgrammed
          expr: |
            kube_customresource_status_condition{
              customresource_kind="Gateway",
              condition="Programmed",
              status="False"
            } == 1
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "Gateway {{ $labels.name }} 未能正常编程"
            description: "命名空间 {{ $labels.namespace }} 中的 Gateway {{ $labels.name }} 状态异常，请检查实现器日志"

        # HTTPRoute 未被接受告警
        - alert: HTTPRouteNotAccepted
          expr: |
            kube_customresource_status_condition{
              customresource_kind="HTTPRoute",
              condition="Accepted",
              status="False"
            } == 1
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "HTTPRoute {{ $labels.name }} 未被 Gateway 接受"

        # 错误率超阈值
        - alert: GatewayHighErrorRate
          expr: |
            sum(rate(envoy_cluster_upstream_rq_xx{response_code_class="5"}[5m]))
            /
            sum(rate(envoy_cluster_upstream_rq_total[5m])) > 0.05
          for: 3m
          labels:
            severity: critical
          annotations:
            summary: "Gateway 5xx 错误率超过 5%"

        # P99 延迟告警
        - alert: GatewayHighLatency
          expr: |
            histogram_quantile(0.99,
              sum(rate(envoy_cluster_upstream_rq_time_bucket[5m])) by (le, cluster_name)
            ) > 2000
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Gateway P99 延迟超过 2 秒"
```

---

<!-- chunk: 9. 未来方向 -->## 9. 未来方向

## 9.1 GAMMA 倡议 — 服务网格原生路由

**GAMMA（Gateway API for Mesh Management and Administration）** 是 Gateway API 最重要的扩展方向，旨在将 Gateway API 的路由语义延伸到服务网格（东西向流量）领域，统一 Ingress（南北向）与 Mesh（东西向）的流量管理 API。

**GAMMA 核心概念：**

```
传统架构：
  外部流量 → Ingress/Gateway → Service → Pod（东西向由 Mesh 控制）

GAMMA 架构：
  外部流量 → GatewayClass=external → HTTPRoute → Service → Pod
  服务间流量 → GatewayClass=mesh   → HTTPRoute → Service → Pod
              ↑
              同一套 API，不同的 GatewayClass 区分场景
```

**GAMMA 路由示例：**

```yaml
# gamma-mesh-route.yaml
# 服务网格内部路由，使用 parentRef 指向 Service 而非 Gateway
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: orders-mesh-route
  namespace: team-orders
spec:
  # GAMMA: parentRef 指向 Service（而非 Gateway）
  # 表示"拦截所有发往 orders-service 的流量并应用此路由"
  parentRefs:
    - group: ""
      kind: Service
      name: orders-service
      port: 8080
  rules:
    # 故障注入（混沌工程）
    - matches:
        - headers:
            - name: X-Chaos-Test
              value: "true"
      filters:
        - type: ExtensionRef
          extensionRef:
            group: gateway.envoyproxy.io
            kind: HTTPRouteFilter
            name: fault-injection
      backendRefs:
        - name: orders-service
          port: 8080

    # 正常流量金丝雀
    - backendRefs:
        - name: orders-v1
          port: 8080
          weight: 90
        - name: orders-v2-canary
          port: 8080
          weight: 10
```

**GAMMA 演进路线：**

| 时间线 | 里程碑 |
|-------|-------|
| 2024-Q1 | GAMMA 规范草案稳定，Cilium 与 Istio 率先实现 |
| 2025-Q2 (v1.3) | GAMMA 进入 Beta，ParentReference Service 类型标准化 |
| 2025-Q4 (v1.4) | 故障注入、流量染色 API 标准化提案 |
| 2026-Q2 (v1.5 预计) | GAMMA GA，成为服务网格路由的统一标准 |
| 2026-Q4 (预计) | 跨集群 GAMMA 路由提案，支持多集群服务网格 |

## 9.2 BackendLBPolicy 与 BackendTLSPolicy GA

**BackendTLSPolicy**（v1.4 已 GA）允许为 Gateway 到后端服务之间的连接配置 TLS，实现真正的端到端加密：

```yaml
# backend-tls-policy.yaml
apiVersion: gateway.networking.k8s.io/v1alpha3
kind: BackendTLSPolicy
metadata:
  name: orders-backend-tls
  namespace: team-orders
spec:
  targetRefs:
    - group: ""
      kind: Service
      name: orders-service
      sectionName: https-8443
  validation:
    # 验证后端服务的 TLS 证书
    caCertificateRefs:
      - kind: ConfigMap
        name: internal-ca-bundle
        group: ""
    # 后端证书的 SANs 验证
    hostname: orders-service.team-orders.svc.cluster.local
    # 客户端证书（mTLS）
    wellKnownCACertificates: System
```

**BackendLBPolicy** 标准化了对后端服务的负载均衡策略配置：

```yaml
# backend-lb-policy.yaml
apiVersion: gateway.networking.k8s.io/v1alpha2
kind: BackendLBPolicy
metadata:
  name: api-lb-policy
  namespace: team-alpha
spec:
  targetRefs:
    - group: ""
      kind: Service
      name: api-service
  sessionPersistence:
    # 会话保持（粘性会话）
    sessionName: GATEWAY_SESSION_ID
    type: Cookie
    cookieConfig:
      lifetimeType: Session
    absoluteTimeout: 1h
    idleTimeout: 30m
```

## 9.3 跨集群路由（Multi-Cluster Gateway）

Gateway API 工作组正在推进跨集群路由标准，与 ClusterSet / MCS（Multi-Cluster Services）API 协同，实现真正的多集群统一流量入口。预计在 2026 下半年进入实验阶段。

## 9.4 Infrastructure 注解标准化

v1.4 引入的 `infrastructure` 字段正在扩展，计划在未来版本中标准化云厂商 LoadBalancer 注解，减少对 Service 注解的依赖，让 Gateway 配置更加自包含和可移植。

## 9.5 与本系列其他文档的关联

本文档与 kudig-database 知识库中以下文档存在深度关联，建议结合阅读：

| 文档编号 | 文档标题 | 关联内容 |
|---------|---------|---------|
| **第 09 篇** | Istio 服务网格深度实践 | Istio Gateway API 实现器详解；GAMMA 在 Istio 中的实现；Istio 流量管理与 Gateway API 的映射关系 |
| **第 18 篇** | Kubernetes eBPF/Cilium 深度实践 | Cilium Gateway API 实现原理；eBPF 在 L7 流量管理中的应用；Cilium Mesh 与 GAMMA 集成 |

> 💡 **推荐学习路径**：如果您刚开始接触 Kubernetes 流量管理，建议按照 **第 18 篇（Cilium 基础）→ 本文（Gateway API）→ 第 09 篇（Istio Mesh）** 的顺序学习，构建从 CNI 到 Gateway 再到全功能 Service Mesh 的完整知识体系。

---

<!-- chunk: 参考资料 -->## 参考资料

- [Kubernetes Gateway API 官方文档](https://gateway-api.sigs.k8s.io/)
- [Gateway API GitHub 仓库](https://github.com/kubernetes-sigs/gateway-api)
- [ingress2gateway 迁移工具](https://github.com/kubernetes-sigs/ingress2gateway)
- [Envoy Gateway 文档](https://gateway.envoyproxy.io/)
- [Cilium Gateway API 文档](https://docs.cilium.io/en/stable/network/servicemesh/gateway-api/)
- [Istio Gateway API 集成](https://istio.io/latest/docs/tasks/traffic-management/ingress/gateway-api/)
- [NGINX Gateway Fabric](https://github.com/nginx/nginx-gateway-fabric)
- [cert-manager Gateway API 集成](https://cert-manager.io/docs/usage/gateway/)
- [GAMMA 倡议规范](https://gateway-api.sigs.k8s.io/mesh/)
- [Gateway API 一致性测试](https://gateway-api.sigs.k8s.io/concepts/conformance/)

---

*本文档由 kudig.io 技术团队维护，遵循 [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) 协议。如发现内容错误或有补充建议，欢迎通过 GitHub Issues 或 PR 参与贡献。*

*最后更新: 2026-03-03 | 对应 Gateway API 版本: v1.4 | 下次计划审阅: 2026-09-03*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-19-papers MOC
- [[21-生态参考/README.md|Domain 19: Kubernetes 高级技术论文与最佳实践 (Advanced Technical Papers...]]
- Domain-19 论文与参考 — 开源项目索引
- Kubernetes 生产就绪性评估框架 (Production Readiness Assessment Framew...
- Kubernetes 大规模集群性能优化深度实践 (Large-Scale Cluster Performance Op...
- Kubernetes 安全零信任架构实施指南 (Zero Trust Security Architecture Imp...
- Kubernetes 多云混合部署架构与实践 (Multi-Cloud Hybrid Deployment Archit...
- Kubernetes GitOps 完整实践指南 (GitOps Complete Practice Guide)
- Kubernetes 成本治理与 FinOps 实践 (Kubernetes Cost Governance and F...
- Kubernetes 容器存储接口 (CSI) 深度实践指南 (Container Storage Interface ...
- Kubernetes 网络策略与安全微隔离实践 (Network Policies and Security Micro...
- Kubernetes 服务网格深度实践与Istio集成 (Service Mesh Deep Practice and ...

## See Also

- 17-kubernetes-aiml-gpu-scheduling-llm-inference
- 18-kubernetes-ebpf-cilium-deep-practice
- 20-kubernetes-supply-chain-security-sbom-slsa-sigstore
- 21-kubernetes-platform-engineering-internal-developer-platform

## Related

- research/ — tag hub

- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
