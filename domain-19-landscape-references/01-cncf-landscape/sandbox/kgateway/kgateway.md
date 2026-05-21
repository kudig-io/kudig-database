---
title: K Gateway (formerly Gloo Gateway)
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- istio
- envoy
- helm
- ingress
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K Gateway (formerly Gloo Gateway) 是什么
- 如何 K Gateway (formerly Gloo Gateway)
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Gateway
- formerly
- Gloo
- Gateway
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- service-mesh-basics
---

title: K Gateway (formerly Gloo Gateway)
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- istio
- envoy
- helm
- ingress
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- K Gateway (formerly Gloo Gateway) 是什么
- 如何 K Gateway (formerly Gloo Gateway)
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Gateway
- formerly
- Gloo
- Gateway
- cncf
- landscape
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
# K Gateway (formerly Gloo Gateway)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kgateway.dev/ |
| **GitHub** | https://github.com/kgateway-dev/kgateway |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

K Gateway（原 Gloo Gateway）是一个基于 Envoy 的 Kubernetes API Gateway，完全实现了 Kubernetes Gateway API 标准。它为 Kubernetes 集群提供南北向流量管理、API 路由、认证授权、限流、请求转换等能力，同时支持将流量路由到 Kubernetes Service、外部服务、Lambda 函数等多种上游目标。

### 核心特性

- **Gateway API**: 完整实现 Kubernetes Gateway API (HTTPRoute, GRPCRoute, TCPRoute)
- **Envoy 数据面**: 基于 Envoy Proxy 的高性能数据面
- **API 路由**: 丰富的路由规则，支持 Header、Path、Query 匹配
- **认证**: 支持 OAuth2/OIDC、API Key、JWT 验证、外部认证
- **限流**: 全局和本地限流策略
- **请求转换**: 请求/响应的 Header 和 Body 转换
- **多上游**: Kubernetes Service、外部服务、gRPC、Lambda

---

## 快速开始

### 安装

```bash
# 使用 Helm 安装
helm repo add kgateway https://kgateway-dev.github.io/kgateway/
helm install kgateway kgateway/kgateway \
  --namespace kgateway-system \
  --create-namespace
```

### 创建 Gateway 和路由

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: main-gateway
  namespace: kgateway-system
spec:
  gatewayClassName: kgateway
  listeners:
    - name: http
      port: 80
      protocol: HTTP
    - name: https
      port: 443
      protocol: HTTPS
      tls:
        certificateRefs:
          - name: my-tls-cert

---
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: api-routes
spec:
  parentRefs:
    - name: main-gateway
      namespace: kgateway-system
  hostnames:
    - "api.example.com"
  rules:
    - matches:
        - path:
            type: PathPrefix
            value: /v1/users
      backendRefs:
        - name: user-service
          port: 8080
    - matches:
        - path:
            type: PathPrefix
            value: /v1/orders
      backendRefs:
        - name: order-service
          port: 8080
          weight: 90
        - name: order-service-v2
          port: 8080
          weight: 10
```

### 限流策略

```yaml
apiVersion: gateway.kgateway.dev/v1alpha1
kind: RateLimitPolicy
metadata:
  name: api-rate-limit
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: api-routes
  rateLimit:
    global:
      descriptors:
        - entries:
            - headerValueMatch:
                headers:
                  - name: x-api-tier
                    value: free
          limit:
            requestsPerUnit: 100
            unit: minute
        - entries:
            - headerValueMatch:
                headers:
                  - name: x-api-tier
                    value: premium
          limit:
            requestsPerUnit: 10000
            unit: minute
```

---

## 与其他方案对比

| 特性 | K Gateway | Istio Gateway | Kong | NGINX Ingress |
|:---|:---|:---|:---|:---|
| 标准 | Gateway API | Gateway API | 专有 + GW API | Ingress/GW API |
| 数据面 | Envoy | Envoy | NGINX/Kong | NGINX |
| 认证 | OAuth2/JWT/API Key | JWT/mTLS | 丰富插件 | 基础 |
| 限流 | 全局+本地 | 本地 | 插件 | 基础 |
| 请求转换 | 丰富 | 基础 | 插件 | 有限 |
| Lambda 路由 | 支持 | 不支持 | 插件 | 不支持 |

---

## 最佳实践

1. **Gateway API 优先**: 使用标准 Gateway API 资源定义路由，保持可移植性
2. **TLS 终止**: 在 Gateway 层终止 TLS，后端使用明文通信减少复杂度
3. **限流分层**: 组合全局限流和本地限流实现多层保护
4. **健康检查**: 配置上游健康检查，自动剔除故障后端
5. **灰度发布**: 利用 HTTPRoute 的 weight 字段实现金丝雀发布

---

## 参考资源

- [K Gateway 文档](https://kgateway.dev/docs/)
- [K Gateway GitHub](https://github.com/kgateway-dev/kgateway)
- [Kubernetes Gateway API](https://gateway-api.sigs.k8s.io/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/gateway-api.md|gateway-api]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
