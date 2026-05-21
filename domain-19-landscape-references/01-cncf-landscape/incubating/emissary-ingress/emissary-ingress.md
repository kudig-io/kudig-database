---
title: Emissary-Ingress
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- envoy
- helm
- ingress
- gateway
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Emissary-Ingress 是什么
- 如何 Emissary-Ingress
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Emissary-Ingress
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
---

title: Emissary-Ingress
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- envoy
- helm
- ingress
- gateway
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Emissary-Ingress 是什么
- 如何 Emissary-Ingress
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Emissary-Ingress
- cncf
- landscape
cross_refs:
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/ingress-fta.md
  label: '故障树: ingress'
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

# Emissary-Ingress

> **成熟度**: Incubating | **加入时间**: 2018-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://www.getambassador.io |
| **GitHub** | https://github.com/emissary-ingress/emissary |
| **许可证** | Apache-2.0 |
| **主要语言** | Python, Go |
| **CNCF 分类** | Networking & API Gateway |

---

## 项目概述

Emissary-Ingress（原 Ambassador API Gateway）是 Kubernetes 原生的 API 网关，基于 Envoy Proxy 构建。它提供丰富的流量管理、认证授权和可观测性能力，是微服务架构的入口层解决方案。

## 核心特性

- **Kubernetes 原生**: CRD 方式配置，声明式管理
- **基于 Envoy**: 利用 Envoy 的高性能和可扩展性
- **自助服务**: 开发者可自主配置路由规则
- **金丝雀发布**: 支持权重路由和 A/B 测试
- **认证集成**: OAuth2、JWT、API Key、外部认证
- **速率限制**: 细粒度的流量控制
- **可观测性**: 链路追踪、指标导出、日志管理

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                   Emissary-Ingress Architecture                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    External Traffic                        │ │
│  │           (Load Balancer / NodePort / HostPort)           │ │
│  └────────────────────────────┬───────────────────────────────┘ │
│                               │                                  │
│                               ▼                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   Emissary-Ingress                         │ │
│  │                                                            │ │
│  │  ┌─────────────────────────────────────────────────────┐  │ │
│  │  │                    Envoy Proxy                      │  │ │
│  │  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌──────────┐ │  │ │
│  │  │  │Listener │ │ Filter  │ │ Cluster │ │  Route   │ │  │ │
│  │  │  │         │ │  Chain  │ │ Manager │ │  Table   │ │  │ │
│  │  │  └─────────┘ └─────────┘ └─────────┘ └──────────┘ │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  │                                                            │ │
│  │  ┌─────────────────────────────────────────────────────┐  │ │
│  │  │              Emissary Control Plane                 │  │ │
│  │  │  ┌───────────┐ ┌───────────┐ ┌─────────────────┐  │  │ │
│  │  │  │ Diagd     │ │  Ambex    │ │   Edge Stack    │  │  │ │
│  │  │  │(Diagnosis)│ │(Envoy xDS)│ │   (Optional)    │  │  │ │
│  │  │  └───────────┘ └───────────┘ └─────────────────┘  │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                               │                                  │
│               ┌───────────────┼───────────────┐                 │
│               ▼               ▼               ▼                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │  Service A   │  │  Service B   │  │  Service C   │          │
│  │  (API)       │  │  (Web)       │  │  (gRPC)      │          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心 CRD

| CRD | 说明 |
|-----|------|
| Mapping | 路由规则定义 |
| Host | 域名和 TLS 配置 |
| TLSContext | TLS 证书配置 |
| Module | 全局配置模块 |
| RateLimitService | 速率限制服务 |
| AuthService | 外部认证服务 |

---

## 快速开始

### 安装 Emissary-Ingress

```bash
# Helm 安装
helm repo add datawire https://app.getambassador.io
helm repo update

kubectl create namespace emissary
kubectl apply -f https://app.getambassador.io/yaml/emissary/3.9.1/emissary-crds.yaml

helm install emissary-ingress datawire/emissary-ingress \
  --namespace emissary \
  --set replicaCount=3
```

### 配置 Listener

```yaml
apiVersion: getambassador.io/v3alpha1
kind: Listener
metadata:
  name: https-listener
  namespace: emissary
spec:
  port: 8443
  protocol: HTTPS
  securityModel: XFP
  hostBinding:
    namespace:
      from: ALL
```

### 配置路由

```yaml
# 基本路由
apiVersion: getambassador.io/v3alpha1
kind: Mapping
metadata:
  name: api-mapping
spec:
  hostname: api.example.com
  prefix: /api/
  service: api-service:8080
---
# 带重写的路由
apiVersion: getambassador.io/v3alpha1
kind: Mapping
metadata:
  name: backend-mapping
spec:
  hostname: api.example.com
  prefix: /backend/
  rewrite: /
  service: backend-service:8080
---
# gRPC 路由
apiVersion: getambassador.io/v3alpha1
kind: Mapping
metadata:
  name: grpc-mapping
spec:
  hostname: grpc.example.com
  prefix: /
  service: grpc-service:50051
  grpc: true
```

---

## 高级路由

### 金丝雀发布

```yaml
apiVersion: getambassador.io/v3alpha1
kind: Mapping
metadata:
  name: service-stable
spec:
  prefix: /api/
  service: service-v1:8080
  weight: 90
---
apiVersion: getambassador.io/v3alpha1
kind: Mapping
metadata:
  name: service-canary
spec:
  prefix: /api/
  service: service-v2:8080
  weight: 10
```

### Header 路由

```yaml
apiVersion: getambassador.io/v3alpha1
kind: Mapping
metadata:
  name: header-routing
spec:
  prefix: /api/
  service: service-beta:8080
  headers:
    x-beta-user: "true"
```

### 负载均衡策略

```yaml
apiVersion: getambassador.io/v3alpha1
kind: Mapping
metadata:
  name: lb-mapping
spec:
  prefix: /api/
  service: api-service:8080
  load_balancer:
    policy: round_robin  # ring_hash, maglev, random
```

---

## TLS 配置

### Host 和证书

```yaml
apiVersion: getambassador.io/v3alpha1
kind: Host
metadata:
  name: api-host
spec:
  hostname: api.example.com
  tlsSecret:
    name: api-tls-secret
  requestPolicy:
    insecure:
      action: Redirect
---
# TLS Secret
apiVersion: v1
kind: Secret
metadata:
  name: api-tls-secret
type: kubernetes.io/tls
data:
  tls.crt: <base64-encoded-cert>
  tls.key: <base64-encoded-key>
```

### 自动证书（ACME）

```yaml
apiVersion: getambassador.io/v3alpha1
kind: Host
metadata:
  name: auto-tls-host
spec:
  hostname: api.example.com
  acmeProvider:
    authority: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
```

---

## 认证与授权

### JWT 验证

```yaml
apiVersion: getambassador.io/v3alpha1
kind: FilterPolicy
metadata:
  name: jwt-policy
spec:
  rules:
    - host: api.example.com
      path: /api/*
      filters:
        - name: jwt-filter
          namespace: emissary
          arguments:
            issuer: https://auth.example.com
            audience: api
            jwksURI: https://auth.example.com/.well-known/jwks.json
```

### 外部认证

```yaml
apiVersion: getambassador.io/v3alpha1
kind: AuthService
metadata:
  name: ext-auth
spec:
  auth_service: "auth-service:3000"
  path_prefix: "/auth"
  allowed_request_headers:
    - "Authorization"
  allowed_authorization_headers:
    - "X-User-ID"
```

---

## 速率限制

```yaml
apiVersion: getambassador.io/v3alpha1
kind: RateLimitService
metadata:
  name: ratelimit
spec:
  service: "ratelimit:8081"
  protocol_version: v3
---
apiVersion: getambassador.io/v3alpha1
kind: Mapping
metadata:
  name: rate-limited-api
spec:
  prefix: /api/
  service: api-service:8080
  labels:
    ambassador:
      - request_label_group:
        - source_cluster:
            key: x-api-key
            header: x-api-key
        - destination_cluster:
            key: service
            default: api
```

---

## 可观测性

### Prometheus 指标

```yaml
apiVersion: getambassador.io/v3alpha1
kind: Module
metadata:
  name: ambassador
spec:
  config:
    diagnostics:
      enabled: true
    statistics:
      enabled: true
      statsd_exporter:
        enabled: true
```

### 链路追踪

```yaml
apiVersion: getambassador.io/v3alpha1
kind: TracingService
metadata:
  name: tracing
spec:
  service: "zipkin:9411"
  driver: zipkin
  config:
    collector_endpoint: /api/v2/spans
    collector_endpoint_version: HTTP_JSON
```

---

## 最佳实践

1. **高可用**: 部署多副本，配置 PodDisruptionBudget
2. **资源限制**: 为 Envoy 配置合适的 CPU/Memory
3. **渐进部署**: 使用金丝雀发布验证新版本
4. **监控告警**: 配置请求延迟和错误率告警
5. **安全加固**: 启用 TLS、认证、速率限制

---

## 参考资源

- [官方文档](https://www.getambassador.io/docs/emissary)
- [GitHub Repo](https://github.com/emissary-ingress/emissary)
- [CRD 参考](https://www.getambassador.io/docs/emissary/latest/topics/using/)
- [迁移指南](https://www.getambassador.io/docs/emissary/latest/topics/install/migration/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-19-landscape-references/04-cncf-fta-index.md|04-cncf-fta-index]]
- [[man/INSTALL.md|INSTALL]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/networking.md|networking]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/nginx-ingress-index|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
