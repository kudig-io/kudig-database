---
title: Service Mesh 零信任安全架构
summary: Service Mesh 零信任安全架构：零信任（Zero Trust）的核心原则是"从不信任、始终验证"——无论流量来自集群外部还是内部，都必须经过认证和授权。Service
  Mesh（以 entities/istio.md 为代表）是零信任理念在 entities/kubernetes.md 服务间通信层的具体技术实现：通过自动 mTLS、L7
  授权策略和统一身份框架，将安全从"网络边界...
category: synthesis
tags:
- k8s
- service-mesh
- zero-trust
- mtls
- istio
- authorizationpolicy
- spiffe
- spire
- security
- networkpolicy
- identity
tier: supporting
sources:
- 网络/02-service-mesh
- 网络/05-service-mesh
- 安全/02-network-security
- 安全/01-identity-access
created: 2026-05-21 14:00:00+00:00
updated: 2026-05-21 14:00:00+00:00
last_updated: 2026-05-21 14:00:00+00:00
relationships:
- target: '[[entities/cilium.md]]'
  type: uses
- target: '[[entities/istio.md]]'
  type: uses
- target: '[[entities/kubernetes.md]]'
  type: uses
- target: '[[系统基础/知识字典/networking/service-mesh.md]]'
  type: uses
- target: '[[concepts/Cilium eBPF × 可观测性.md]]'
  type: uses
---



# [[系统基础/知识字典/networking/service-mesh.md|Service Mesh]] 零信任安全架构

## 概述

零信任（Zero Trust）的核心原则是"从不信任、始终验证"——无论流量来自集群外部还是内部，都必须经过认证和授权。Service Mesh（以 [[entities/istio.md|Istio]] 为代表）是零信任理念在 [[entities/kubernetes.md|Kubernetes]] 服务间通信层的具体技术实现：通过自动 mTLS、L7 授权策略和统一身份框架，将安全从"网络边界"下沉到"每次服务调用"。本页连接 网络 的服务网格能力与 安全 的零信任安全框架，展示如何在 K8s 环境中构建服务网格驱动的零信任架构。

## 核心连接

| 域 | 核心能力 | 零信任的桥接作用 |
|---|---|---|
| **Networking (domain-03)** | 服务发现、流量管理、L7 代理 | 网格提供 mTLS、授权策略、流量加密的执行层 |
| **Security (domain-05)** | 身份认证、访问控制、安全审计 | 零信任提供策略框架（谁可以访问什么、如何验证） |

**关键洞察：传统安全模型是"城堡+护城河"——内部信任、外部不信任。零信任 + 服务网格将安全模型转变为"每个房间都有门禁"——每次服务调用都是独立的信任验证。**

## 架构图

### 零信任服务网格架构

```mermaid
graph TB
    subgraph External["外部流量"]
        User[终端用户]
        GW[Ingress Gateway]
    end

    subgraph Mesh["Service Mesh (Istio)"]
        subgraph NS1["namespace: frontend"]
            P1[Pod A<br/>Envoy Sidecar]
            P2[Pod B<br/>Envoy Sidecar]
        end
        subgraph NS2["namespace: backend"]
            P3[Pod C<br/>Envoy Sidecar]
            P4[Pod D<br/>Envoy Sidecar]
        end
        subgraph NS3["namespace: data"]
            P5[Pod E<br/>Envoy Sidecar]
        end
        
        I[istiod<br/>控制平面]
        CA[Istio CA<br/>证书签发]
    end

    subgraph Identity["身份层"]
        Spire[SPIRE Server]
        Svid[SPIFFE SVID]
    end

    User -->|TLS 1.3| GW
    GW -->|mTLS| P1
    P1 -->|mTLS + AuthZ| P3
    P3 -->|mTLS + AuthZ| P5
    P2 -->|mTLS + AuthZ| P4
    
    I -->|配置下发| P1
    I -->|配置下发| P3
    I -->|配置下发| P5
    CA -->|证书| P1
    CA -->|证书| P3
    Spire -->|身份| CA
```

### mTLS 握手流程

```mermaid
sequenceDiagram
    participant Client as Client Envoy
    participant Server as Server Envoy
    participant CA as Istio CA

    Note over Client,Server: 启动阶段
    Client->>CA: 请求证书 (CSR)
    CA->>Client: 返回 X.509 证书
    Server->>CA: 请求证书 (CSR)
    CA->>Server: 返回 X.509 证书

    Note over Client,Server: mTLS 握手
    Client->>Server: ClientHello + 支持的加密套件
    Server->>Client: ServerHello + 证书 + 请求客户端证书
    Client->>Server: 客户端证书 + 密钥交换
    Server->>Client: 验证客户端证书
    Client->>Server: 验证服务端证书
    Note over Client,Server: 双向认证完成，建立加密通道
    Client->>Server: HTTP/2 gRPC 加密请求
    Server->>Client: 加密响应
```

### AuthorizationPolicy 决策流程

```mermaid
flowchart TD
    A[收到 mTLS 连接] --> B{目标端口?}
    B -->|15001/15006| C[Envoy 代理拦截]
    C --> D{是否有 AuthZ Policy?}
    D -->|否| E[默认拒绝<br/>PERMISSIVE 模式下允许]
    D -->|是| F[评估策略]
    F --> G{来源匹配?}
    G -->|否| H[拒绝 RbacAccessDenied]
    G -->|是| I{操作匹配?}
    I -->|否| H
    I -->|是| J{路径/方法匹配?}
    J -->|否| H
    J -->|是| K[允许转发到应用]
    K --> L[应用处理请求]
```

## 核心机制

### 零信任四大原则 × 服务网格实现

| 零信任原则 | 服务网格机制 | 具体实现 |
|---|---|---|
| **永不信任、始终验证** | 自动 mTLS | Istio 为每个服务自动签发证书，所有服务间通信加密 |
| **最小权限访问** | AuthorizationPolicy | L7 细粒度控制：基于身份、命名空间、HTTP 方法、路径 |
| **持续监控与审计** | 可观测性 | 自动导出黄金指标（延迟/流量/错误/饱和度）和访问日志 |
| **微分段** | L7 网络策略 | NetworkPolicy 做 L3/L4 粗粒度，AuthZ Policy 做 L7 细粒度 |

### mTLS 模式演进

```
Istio mTLS 模式:
  PERMISSIVE (宽容模式)
    → 允许明文和 mTLS 同时存在
    → 迁移期使用，逐步切流
    
  STRICT (严格模式)
    → 仅允许 mTLS 连接
    → 生产目标状态
    
  DISABLE (禁用模式)
    → 仅明文通信
    → 仅用于特殊兼容场景
```

```yaml
# 全局强制 mTLS
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: default
  namespace: istio-system
spec:
  mtls:
    mode: STRICT
---
# 命名空间级例外（迁移期）
apiVersion: security.istio.io/v1beta1
kind: PeerAuthentication
metadata:
  name: legacy-exception
  namespace: legacy-system
spec:
  mtls:
    mode: PERMISSIVE
  selector:
    matchLabels:
      app: legacy-service
```

### AuthorizationPolicy 策略层级

```yaml
# 层级 1: 默认拒绝所有
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: deny-all
  namespace: production
spec:
  {}
---
# 层级 2: 允许 frontend 访问 backend
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: backend-policy
  namespace: production
spec:
  selector:
    matchLabels:
      app: backend-api
  action: ALLOW
  rules:
    - from:
        - source:
            namespaces: ["frontend"]
            principals: ["cluster.local/ns/frontend/sa/frontend-sa"]
      to:
        - operation:
            methods: ["GET", "POST"]
            paths: ["/api/v1/*"]
      when:
        - key: request.headers[x-request-id]
          values: ["*"]
---
# 层级 3: 允许 backend 访问 database
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: database-policy
  namespace: production
spec:
  selector:
    matchLabels:
      app: database
  action: ALLOW
  rules:
    - from:
        - source:
            namespaces: ["production"]
            principals: ["cluster.local/ns/production/sa/backend-sa"]
      to:
        - operation:
            methods: ["POST"]
            ports: ["5432"]
```

### SPIFFE/SPIRE 身份框架

```mermaid
graph TB
    subgraph SPIRE["SPIRE 架构"]
        S[SPIRE Server]
        A[SPIRE Agent<br/>每节点]
        W[Workload API<br/>Unix Domain Socket]
    end
    subgraph Workloads["工作负载"]
        P1[Pod A<br/>SPIFFE ID: spiffe://cluster.local/ns/prod/sa/web]
        P2[Pod B<br/>SPIFFE ID: spiffe://cluster.local/ns/prod/sa/api]
    end
    subgraph Istio["Istio 集成"]
        CA[Istio CA]
        AuthZ[AuthorizationPolicy]
    end

    S -->|下发 SVID| A
    A -->|通过 UDS| W
    W -->|获取 SVID| P1
    W -->|获取 SVID| P2
    P1 -->|mTLS + SVID| CA
    P2 -->|mTLS + SVID| CA
    CA -->|验证身份| AuthZ
```

```yaml
# SPIRE 与 Istio 集成配置
apiVersion: spire.spiffe.io/v1alpha1
kind: ClusterSPIFFEID
metadata:
  name: workload-id
spec:
  spiffeIDTemplate: "spiffe://{{ .TrustDomain }}/ns/{{ .PodMeta.Namespace }}/sa/{{ .PodSpec.ServiceAccountName }}"
  podSelector:
    matchLabels:
      spiffe.io/spire-managed-identity: "true"
  workloadSelectorTemplates:
    - "k8s:ns:{{ .PodMeta.Namespace }}"
    - "k8s:sa:{{ .PodSpec.ServiceAccountName }}"
```

### 零信任网络分段

```mermaid
graph TB
    subgraph DMZ["DMZ 层"]
        GW[Ingress Gateway]
        LB[External LB]
    end
    subgraph App["应用层"]
        FE[Frontend Service]
        API[API Service]
        BFF[BFF Service]
    end
    subgraph Data["数据层"]
        DB[(Database)]
        Cache[(Redis)]
        MQ[Message Queue]
    end
    subgraph Admin["管理平面"]
        AdminSvc[Admin Service]
    end

    LB -->|TLS| GW
    GW -->|mTLS| FE
    FE -->|mTLS + AuthZ| BFF
    BFF -->|mTLS + AuthZ| API
    API -->|mTLS + AuthZ| DB
    API -->|mTLS + AuthZ| Cache
    API -->|mTLS + AuthZ| MQ
    
    AdminSvc -.->|禁止| FE
    AdminSvc -.->|禁止| BFF
    FE -.->|禁止| DB
    MQ -.->|禁止| GW
```

**分段策略：**
- DMZ → App：允许，但仅特定端口
- App → Data：允许，但仅数据库 ServiceAccount
- App → App：按 AuthZ Policy 细粒度控制
- Data → 任何：默认拒绝
- Admin → App：仅管理端口允许

## 最佳实践

### 1. 渐进式 mTLS 迁移

```
mTLS 迁移五步法:
┌─────────────────────────────────────────┐
│  步骤 1: 部署 Istio，PERMISSIVE 模式      │
│  → 观察流量，确认无影响                   │
├─────────────────────────────────────────┤
│  步骤 2: 启用 mTLS 指标监控               │
│  → 确认 mTLS 连接比例                     │
├─────────────────────────────────────────┤
│  步骤 3: 非关键命名空间切换到 STRICT      │
│  → 验证上下游兼容性                       │
├─────────────────────────────────────────┤
│  步骤 4: 核心命名空间切换到 STRICT        │
│  → 生产核心业务验证                       │
├─────────────────────────────────────────┤
│  步骤 5: 全局 STRICT，遗留系统例外        │
│  → 长期维护例外清单                       │
└─────────────────────────────────────────┘
```

### 2. AuthorizationPolicy 设计模式

```yaml
# 模式 1: 默认拒绝 + 显式允许（推荐）
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: default-deny
  namespace: production
spec:
  {}
---
# 模式 2: 命名空间隔离
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: ns-isolation
  namespace: team-a
spec:
  action: ALLOW
  rules:
    - from:
        - source:
            namespaces: ["team-a", "istio-system"]
---
# 模式 3: 路径级访问控制
apiVersion: security.istio.io/v1beta1
kind: AuthorizationPolicy
metadata:
  name: path-level
  namespace: production
spec:
  selector:
    matchLabels:
      app: api-gateway
  action: ALLOW
  rules:
    - to:
        - operation:
            methods: ["GET"]
            paths: ["/public/*", "/health"]
    - from:
        - source:
            namespaces: ["frontend"]
      to:
        - operation:
            methods: ["GET", "POST"]
            paths: ["/api/*"]
    - from:
        - source:
            namespaces: ["admin"]
      to:
        - operation:
            methods: ["*"]
            paths: ["/admin/*", "/api/*"]
```

### 3. 零信任可观测性

```promql
# mTLS 连接健康度
istio_tcp_connections_opened_total{connection_security_policy="mutual_tls"}
/ 
istio_tcp_connections_opened_total

# AuthZ 拒绝率
sum(rate(istio_request_denials_total[5m]))
/
sum(rate(istio_requests_total[5m]))

# 证书即将过期告警
(
  istio_cert_expiry_seconds / 86400
) < 7  # 7 天内过期
```

```yaml
# 证书轮换监控
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: mesh-security-alerts
spec:
  groups:
    - name: mesh-security
      rules:
        - alert: IstioCertificateExpiringSoon
          expr: |
            (
              istio_cert_expiry_seconds{}
              / 86400
            ) < 7
          for: 1h
          labels:
            severity: critical
          annotations:
            summary: "Istio 证书将在 7 天内过期"
        - alert: HighAuthorizationDenialRate
          expr: |
            sum(rate(istio_request_denials_total[5m]))
            /
            sum(rate(istio_requests_total[5m]))
            > 0.01
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "授权拒绝率超过 1%，可能存在策略配置错误"
```

### 4. 多集群零信任

```mermaid
graph TB
    subgraph Cluster1["Cluster 1 (us-east)"]
        I1[istiod]
        P1[Pod A]
        P2[Pod B]
    end
    subgraph Cluster2["Cluster 2 (eu-west)"]
        I2[istiod]
        P3[Pod C]
        P4[Pod D]
    end
    subgraph RootCA["根 CA 联邦"]
        Root[共享根 CA]
        IC1[中间 CA - Cluster 1]
        IC2[中间 CA - Cluster 2]
    end

    Root --> IC1
    Root --> IC2
    IC1 --> I1
    IC2 --> I2
    I1 -->|mTLS| P1
    I1 -->|mTLS| P2
    I2 -->|mTLS| P3
    I2 -->|mTLS| P4
    P1 -.->|跨集群 mTLS| P3
```

```yaml
# 多集群信任域配置
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
spec:
  profile: default
  meshConfig:
    trustDomain: cluster.local
    defaultConfig:
      proxyMetadata:
        ISTIO_META_DNS_CAPTURE: "true"
  components:
    pilot:
      k8s:
        env:
          - name: PILOT_ENABLE_CROSS_CLUSTER_WORKLOAD_ENTRY
            value: "true"
```

### 5. 零信任排障决策树

```mermaid
flowchart TD
    A[服务 A 无法访问服务 B] --> B{mTLS 错误?}
    B -->|是| C[检查 PeerAuthentication]
    B -->|否| D{503/拒绝?}
    C -->|PERMISSIVE/STRICT 不匹配| E[统一 mTLS 模式]
    C -->|证书错误| F[检查证书过期/CA 信任]
    D -->|是| G[检查 AuthorizationPolicy]
    D -->|否| H[检查应用层]
    G -->|来源不匹配| I[修正 source.principals]
    G -->|路径不匹配| J[修正 operation.paths]
    G -->|端口不匹配| K[修正 operation.ports]
```

## 工具推荐

| 工具 | 角色 | 与零信任的集成 |
|---|---|---|
| **Istio** | 服务网格 | mTLS、AuthZ Policy、可观测性 |
| **[[entities/cilium.md|Cilium]]** | eBPF 网络 | 替代方案：L4 mTLS + NetworkPolicy |
| **SPIRE** | 身份管理 | SPIFFE SVID 签发和轮换 |
| **Cert-Manager** | 证书管理 | 与 Istio CA 集成 |
| **Falco** | 运行时安全 | 检测绕过网格的异常流量 |
| **Tetragon** | eBPF 安全 | 内核级网络策略执行 |
| **OPA/Gatekeeper** | 策略即代码 | 验证 AuthZ Policy 配置合规 |

## 张力与权衡

| 张力 | 详情 |
|---|---|
| **Sidecar 开销 vs 安全深度** | Istio Sidecar 增加 ~100MB 内存和 ~5% CPU。对于安全要求极高的场景，这是可接受的；但对于边缘计算或资源受限环境，Ambient 模式或 Cilium eBPF 是更好的选择。 |
| **策略复杂度 vs 安全覆盖** | 细粒度的 AuthZ Policy（到路径级）提供强安全，但配置复杂且容易出错。"默认拒绝 + 逐步放开"是推荐策略，但初期阻力大。 |
| **mTLS 性能 vs 兼容性** | 严格 mTLS（STRICT）最安全，但遗留系统可能不支持。PERMISSIVE 模式是过渡方案，但长期存在会降低安全收益。 |
| **SPIFFE 身份 vs K8s ServiceAccount** | SPIFFE 提供全局统一身份，但引入新组件（SPIRE）增加复杂度。Istio 原生使用 K8s ServiceAccount 作为身份，简单但限于单集群。 |
| **网格覆盖 vs 非网格流量** | 服务网格只能保护网格内的流量。Pod 到外部服务（如 RDS、S3）的流量不在网格内，需要额外的出口网关或 VPC 防火墙策略。 |

## 开放问题

- **加密流量的可见性：** mTLS 加密了服务间通信，但也使传统网络监控工具无法查看内容。如何在保持加密的同时满足安全审计的"可见性"要求？
- **零信任与性能的终极平衡：** Cilium eBPF 可以将 L4 mTLS 性能损失降至 <1%，但牺牲了 L7 授权能力。是否存在一种方案能同时实现 L7 安全 + 接近零的性能损失？
- **供应链安全与网格身份：** 如果容器镜像被篡改（供应链攻击），网格的 mTLS 身份认证仍然会通过——因为身份是 Pod 级别的，不是代码级别的。如何将代码签名与网格身份关联？
- **零信任的成本度量：** 实施零信任（Istio + SPIRE + 审计）有明确的工程和计算成本，但安全收益难以量化。如何构建零信任的 ROI 模型？

## 相关 Domain

- 网络/02-service-mesh
- 网络/05-service-mesh
- 安全/02-network-security
- 安全/01-identity-access
- [[concepts/服务网格 x 零信任安全.md|服务网格 x 零信任安全]]
- Cilium eBPF × 可观测性.md|Cilium eBPF × 可观测性]]

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]
