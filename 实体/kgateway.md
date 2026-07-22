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

## 安装与配置

```bash
# Helm 安装 kgateway
helm repo add kgateway https://kgateway.io/charts
helm repo update
helm install kgateway kgateway/kgateway -n kgateway-system --create-namespace

# 验证安装
kubectl get pods -n kgateway-system
kubectl get gatewayclass kgateway
```

### Gateway + HTTPRoute 配置

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: my-gateway
  namespace: default
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
          - name: my-tls-secret
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
      filters:
        - type: RequestHeaderModifier
          requestHeaderModifier:
            add:
              - name: X-Gateway-Version
                value: "kgateway-v2"
```

### Upstream 自定义路由

```yaml
apiVersion: gateway.kgateway.dev/v1alpha1
kind: Upstream
metadata:
  name: legacy-service
spec:
  static:
    hosts:
      - addr: 10.0.1.100
        port: 8443
  sslConfig:
    verifySubjectAltNames: ["legacy.internal"]
```

## 运维操作

```bash
# 🟢 查看 Gateway 状态
kubectl get gateways -A
kubectl describe gateway my-gateway

# 🟢 查看路由状态
kubectl get httproutes -A
kubectl describe httproute myapp-route

# 🟢 查看 Envoy 代理 Pod
kubectl get pods -n kgateway-system -l app=kgateway-proxy

# 🟡 更新 Gateway 配置（触发 Envoy 热更新）
kubectl apply -f gateway.yaml

# 🟡 调整副本数
kubectl scale deployment kgateway-proxy -n kgateway-system --replicas=3

# 🔴 删除 Gateway（影响所有关联路由）
kubectl delete gateway my-gateway
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Gateway 未分配地址 | GatewayClass 未就绪 | `kubectl get gatewayclass kgateway -o yaml` | 检查 controller 日志 |
| HTTPRoute 未生效 | parentRef 不匹配 | `kubectl describe httproute` | 确认 hostname/listener 匹配 |
| 503 错误 | 后端 Service 无 Endpoints | `kubectl get endpoints myapp-service` | 检查 Pod 标签选择器 |
| TLS 握手失败 | 证书 Secret 不存在 | `kubectl get secret my-tls-secret` | 创建/更新 TLS Secret |
| 路由权重不生效 | 多个 Route 冲突 | `kubectl get httproutes -o wide` | 检查 priority 和 hostname 冲突 |

```
排查流程:
├── Gateway 状态异常
│   ├── kubectl get gatewayclass → 检查 Accepted
│   └── kubectl logs -n kgateway-system -l app=kgateway-controller
├── 路由不生效
│   ├── kubectl describe httproute → 检查 Conditions
│   └── 确认 parentRef.name 与 Gateway 名称匹配
└── 后端不可达
    ├── kubectl get endpoints → 确认有活跃端点
    └── kubectl exec proxy-pod -- curl -v backend:port
```

## 生产案例

### 案例 1: Gateway API 迁移导致流量中断

- **场景**: 从 Ingress 迁移到 Gateway API，切换期间部分流量 404
- **排查**: HTTPRoute 的 hostname 与 Gateway listener 的 hostname 不匹配
- **方案**: 在 Gateway listener 中移除 hostname 限制，由 HTTPRoute 控制路由；灰度期间保留 Ingress 和 Gateway 双活
- **效果**: 零中断完成迁移，回滚时间从 15min 缩短到 1min

### 案例 2: 金丝雀发布权重漂移

- **场景**: 配置 90/10 权重但实际流量比例偏差大
- **排查**: 两个 backendRef 指向同一 Service 的不同端口，Envoy 连接池复用导致统计偏差
- **方案**: 使用独立的 canary Service 和独立的 Pod 标签；配置 outlierDetection 排除异常端点
- **效果**: 流量比例精确控制在 ±2% 以内

## 对比

| 特性 | kgateway | Envoy Gateway | Contour | Istio Gateway | 适用场景 |
|------|----------|---------------|---------|--------------|----------|
| Gateway API | ✅ 完整 | ✅ | ✅ | ✅ | 标准入口 |
| Lambda 路由 | ✅ | ❌ | ❌ | ❌ | Serverless 集成 |
| Service Mesh | ✅ Ambient | ❌ | ❌ | ✅ 核心 | 服务网格 |
| 自定义 CRD | ✅ Upstream | ❌ | ⚠️ | ❌ | 混合云路由 |
| CNCF 状态 | Sandbox | Incubating | Graduated | Graduated | 生态成熟度 |

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
