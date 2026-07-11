---
title: kgateway
description: '## 概述'
summary: 'K Gateway（原 Gloo Gateway）是一个基于 Envoy 的 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] Gateway，完全实现了 Kubernetes Gateway API 标准。'
category: entities
tags:
- k8s
- cncf
- networking
- kgateway
- istio
- envoy
- gateway
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kgateway 是什么
- 如何 kgateway
trigger_keywords:
- kgateway
prerequisites:
- kubectl-basics
- service-mesh-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# kgateway

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

K Gateway（kgateway，原 Solo.io 的 Gloo Gateway 开源版）是一个基于 **Envoy Proxy** 的高性能 Kubernetes API Gateway。2024 年进入 CNCF Sandbox。它完全实现了 **Kubernetes Gateway API** 标准（GA），为集群提供南北向流量管理、API 路由、认证授权、限流、请求转换等能力。kgateway 是 Gloo Gateway 2.0 的开源重构版本，与 Istio 的 Ambient Mesh 模式深度集成。

kgateway 支持将流量路由到 Kubernetes Service、外部服务、AWS Lambda 函数等多种上游目标。它通过自定义的 Upstream CRD 抽象上游服务，结合 Gateway API 标准 CRD（GatewayClass、Gateway、HTTPRoute）实现声明式路由配置。

## Key Features

- **Gateway API 标准**：完全实现 Kubernetes Gateway API，保持可移植性
- **Envoy 数据面**：基于 Envoy 的高性能代理，支持 HTTP/2/gRPC/WebSocket
- **多协议路由**：HTTP、TCP、gRPC、Lambda 函数统一路由
- **认证授权**：集成 OIDC、JWT、API Key、OAuth2 多种认证方式
- **限流与熔断**：全局限流、本地限流、断路器、重试策略
- **Lambda 集成**：直接路由到 AWS Lambda 函数，无需 API Gateway

## Architecture

kgateway 由 **Control Plane**（kgateway POD，包含 Discovery、Transformation、Auth 等插件）和 **Data Plane**（Envoy Proxy Pod）组成。Control Plane 监听 Kubernetes CRD（Gateway、HTTPRoute、Upstream 等），动态生成 Envoy 配置并通过 xDS 协议下发。数据面 Envoy 处理实际流量转发。与 Istio Ambient Mesh 配合时，kgateway 作为 zTunnel 之上的 L7 代理运行。

## K8s 集成

kgateway 通过标准 Gateway API CRD 与 Kubernetes 集成。用户创建 `Gateway` 和 `HTTPRoute` 资源定义路由规则，kgateway Controller 自动将其翻译为 Envoy 配置。也提供自定义 CRD（`Upstream`、`AuthConfig`、`RateLimitConfig`）扩展标准 Gateway API 的能力。支持通过 Helm 或 Gateway API 标准方式安装。

## 生产部署要点

- **Gateway API 优先**：使用标准 Gateway API 资源定义路由，保持可移植性
- **TLS 终止**：在 Gateway 层终止 TLS，后端使用明文通信减少复杂度
- **限流分层**：组合全局限流和本地限流实现多层保护
- **健康检查**：配置上游健康检查，自动剔除问题后端
- **灰度发布**：利用 HTTPRoute 的 weight 字段实现金丝雀发布

## 生产场景

1. **统一 API 网关**：集群入口流量统一管理，认证、限流、路由一站式
2. **Lambda 无服务器路由**：HTTP 请求直接路由到 AWS Lambda，无需 API Gateway
3. **混合云流量管理**：流量路由到 K8s Service 和外部遗留服务
4. **多租户 API**：基于 Host 和 Path 的多租户 API 路由和认证隔离

## 安装

```bash
# Helm 安装 kgateway
helm repo add kgateway https://kgateway.io/charts
helm repo update
helm install kgateway kgateway/kgateway -n kgateway-system --create-namespace

# 创建 Gateway 和 HTTPRoute
kubectl apply -f - <<EOF
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: my-gateway
spec:
  gatewayClassName: kgateway
  listeners:
    - name: http
      port: 80
      protocol: HTTP
---
apiVersion: gateway.networking.k8s.io/v1beta1
kind: HTTPRoute
metadata:
  name: myapp-route
spec:
  parentRefs:
    - name: my-gateway
  hostnames: ["myapp.example.com"]
  rules:
    - matches:
        - path: { type: PathPrefix, value: / }
      backendRefs:
        - name: myapp-service
          port: 8080
          weight: 90
        - name: myapp-canary
          port: 8080
          weight: 10
EOF
```

## 对比

| 特性 | kgateway | Envoy Gateway | Contour | Istio Gateway |
|------|----------|---------------|---------|--------------|
| Gateway API | ✅ 完整 | ✅ | ✅ | ✅ |
| Lambda 路由 | ✅ | ❌ | ❌ | ❌ |
| Service Mesh | ✅ Ambient | ❌ | ❌ | ✅ 核心 |
| 自定义 CRD | ✅ Upstream | ❌ | ⚠️ | ❌ |

## 参考链接

- [[istio]]

## Related

- [[kuberhealthy]] — Kuberhealthy
- [[tokenetes]] — Tokenetes
- [[dex]] — Dex
- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kgateway
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
