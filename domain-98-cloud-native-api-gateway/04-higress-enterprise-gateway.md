# 04 - Higress 云原生 API 网关企业级实践

> **文档版本**: v1.0 | **适用版本**: Higress v1.x - v2.x, Kubernetes 1.25+ | **更新日期**: 2026-03-04 | **关键词**: Higress, Envoy, Istiod, Wasm, AI Gateway, 阿里云

## 目录

1. [Higress 项目概述](#1-higress-项目概述)
2. [核心架构](#2-核心架构)
3. [部署安装](#3-部署安装)
4. [路由配置](#4-路由配置)
5. [插件生态](#5-插件生态)
6. [AI 网关能力](#6-ai-网关能力)
7. [Gateway API 集成](#7-gateway-api-集成)
8. [可观测性](#8-可观测性)
9. [生产环境调优](#9-生产环境调优)
10. [与 Istio 协同](#10-与-istio-协同)

---

## 1. Higress 项目概述

Higress 是阿里巴巴开源的云原生 API 网关，基于 Istio 和 Envoy 构建，2022 年开源，2023 年进入 CNCF Sandbox。

### 核心定位

- **云原生 API 网关**: 面向 Kubernetes 环境的南北向流量管理
- **AI 网关**: 原生支持 LLM 代理路由、Token 级限流、多模型 Fallback
- **Ingress 控制器**: 兼容 Kubernetes Ingress 和 Gateway API 标准

### 发展历程

| 时间 | 里程碑 |
|------|--------|
| 2022-10 | 阿里巴巴开源 Higress |
| 2023-05 | 发布 v1.0，支持 Ingress + 自定义 CRD |
| 2023-10 | 进入 CNCF Sandbox |
| 2024-06 | 发布 AI 网关能力 |
| 2025-03 | Gateway API v1.1 Extended 一致性通过 |
| 2025-12 | v2.0 发布，架构重构 |

## 2. 核心架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      Higress 架构总览                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────────────────────────────────────┐       │
│  │                   控制平面                             │       │
│  │  ┌──────────────────┐  ┌──────────────────┐          │       │
│  │  │  Higress         │  │  Istiod (定制版)   │          │       │
│  │  │  Controller      │  │  - ServiceEntry   │          │       │
│  │  │  - Ingress 转换  │  │  - xDS 下发       │          │       │
│  │  │  - CRD 管理      │  │  - 证书管理       │          │       │
│  │  │  - Gateway API   │  │  - 配置验证       │          │       │
│  │  └────────┬─────────┘  └────────┬─────────┘          │       │
│  │           │         xDS (gRPC)  │                    │       │
│  └───────────│─────────────────────│────────────────────┘       │
│              │                     │                            │
│  ┌───────────▼─────────────────────▼────────────────────┐       │
│  │                   数据平面                             │       │
│  │  ┌──────────────────────────────────────────┐        │       │
│  │  │              Envoy Proxy                  │        │       │
│  │  │  ┌─────────┐ ┌─────────┐ ┌────────────┐ │        │       │
│  │  │  │ Listener│→│ Filter  │→│ Cluster    │ │        │       │
│  │  │  │ (端口)  │ │ Chain   │ │ (Upstream) │ │        │       │
│  │  │  └─────────┘ │ - Wasm  │ └────────────┘ │        │       │
│  │  │              │ - Lua   │                 │        │       │
│  │  │              │ - RBAC  │                 │        │       │
│  │  │              │ - 限流  │                 │        │       │
│  │  │              └─────────┘                 │        │       │
│  │  └──────────────────────────────────────────┘        │       │
│  └──────────────────────────────────────────────────────┘       │
│                                                                 │
│  ┌──────────────────┐                                           │
│  │  Higress Console │  Web 管理控制台（可选）                      │
│  │  - 路由管理      │                                           │
│  │  - 插件配置      │                                           │
│  │  - 证书管理      │                                           │
│  │  - 服务发现      │                                           │
│  └──────────────────┘                                           │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 说明 |
|------|------|
| **Higress Controller** | 将 Ingress/CRD/Gateway API 资源转换为 Istio 内部模型 |
| **Istiod (定制版)** | 基于 Istio Pilot 定制的控制平面，通过 xDS 协议下发配置 |
| **Envoy Proxy** | 高性能数据平面，处理实际流量请求 |
| **Higress Console** | 可选的 Web 管理控制台 |

## 3. 部署安装

### Helm 安装（推荐）

```bash
# 添加 Helm 仓库
helm repo add higress https://higress.io/helm-charts
helm repo update

# 安装 Higress（Kubernetes 环境）
helm install higress higress/higress \
  -n higress-system \
  --create-namespace \
  --set global.local=false \
  --set higress-core.gateway.replicas=2

# 安装 Higress Console（可选）
helm install higress-console higress/higress-console \
  -n higress-system
```

### All-in-One 安装（开发/测试）

```bash
# 单机 Docker 部署
curl -fsSL https://higress.io/standalone/get-higress.sh | bash -s -- \
  -a -c nacos://192.168.0.1:8848
```

### 验证安装

```bash
# 检查 Pod 状态
kubectl get pods -n higress-system

# 预期输出
# higress-controller-xxx    1/1  Running
# higress-gateway-xxx       1/1  Running
# higress-gateway-xxx       1/1  Running

# 检查 Gateway 服务
kubectl get svc -n higress-system higress-gateway
```

## 4. 路由配置

### 方式一：Kubernetes Ingress

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: demo
  annotations:
    higress.io/exact-match-header-x-env: gray
spec:
  ingressClassName: higress
  rules:
  - host: demo.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: demo-service
            port:
              number: 8080
```

### 方式二：Gateway API

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: demo-route
spec:
  parentRefs:
  - name: higress-gateway
    namespace: higress-system
  hostnames:
  - "demo.example.com"
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api
    backendRefs:
    - name: demo-service
      port: 8080
```

### 方式三：Higress 自定义 CRD

```yaml
apiVersion: networking.higress.io/v1
kind: McpBridge
metadata:
  name: nacos-bridge
  namespace: higress-system
spec:
  registries:
  - name: nacos
    type: nacos2
    domain: nacos.example.com
    port: 8848
    nacosGroups:
    - DEFAULT_GROUP
```

### 金丝雀发布

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: demo-canary
  annotations:
    higress.io/canary: "true"
    higress.io/canary-weight: "20"
    higress.io/canary-header: x-canary
    higress.io/canary-header-value: "true"
spec:
  ingressClassName: higress
  rules:
  - host: demo.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: demo-service-v2
            port:
              number: 8080
```

## 5. 插件生态

### 插件分类

| 类别 | 内置插件 |
|------|---------|
| **认证鉴权** | key-auth, jwt-auth, hmac-auth, basic-auth, oidc |
| **流量管控** | key-rate-limit, request-block, bot-detect |
| **安全防护** | waf, cors, ip-restriction, csrf |
| **协议转换** | transformer, request-validation, response-rewrite |
| **可观测性** | prometheus, request-log, skywalking |
| **AI 网关** | ai-proxy, ai-token-ratelimit, ai-cache, ai-prompt-template |

### Wasm 插件开发（Go/TinyGo）

```go
package main

import (
    "github.com/higress-group/proxy-wasm-go-sdk/proxywasm"
    "github.com/higress-group/proxy-wasm-go-sdk/proxywasm/types"
)

type myPlugin struct {
    proxywasm.DefaultPluginContext
}

type myHttpContext struct {
    proxywasm.DefaultHttpContext
}

func main() {
    proxywasm.SetVMContext(&myPlugin{})
}

func (p *myPlugin) NewHttpContext(contextID uint32) types.HttpContext {
    return &myHttpContext{}
}

func (ctx *myHttpContext) OnHttpRequestHeaders(numHeaders int, endOfStream bool) types.Action {
    // 添加自定义请求头
    proxywasm.AddHttpRequestHeader("x-custom-header", "higress-wasm")
    return types.ActionContinue
}
```

### 插件配置

```yaml
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: custom-auth
  namespace: higress-system
spec:
  url: oci://registry.example.com/higress/custom-auth:v1.0
  phase: AUTHN
  priority: 100
  matchRules:
  - ingress:
    - demo
    config:
      allowList:
      - "api-key-001"
      - "api-key-002"
```

## 6. AI 网关能力

Higress 内置 AI 网关能力，支持 LLM 代理路由、Token 级限流、多模型 Fallback 等：

### AI 代理路由

```yaml
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: ai-proxy
  namespace: higress-system
spec:
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/ai-proxy:latest
  matchRules:
  - ingress:
    - ai-route
    config:
      provider:
        type: openai
        apiTokens:
        - "${OPENAI_API_KEY}"
        modelMapping:
          "gpt-4": "gpt-4-turbo"
          "*": "gpt-3.5-turbo"
```

### Token 级限流

```yaml
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: ai-token-ratelimit
  namespace: higress-system
spec:
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/ai-token-ratelimit:latest
  matchRules:
  - ingress:
    - ai-route
    config:
      rule_name: "default"
      rule_items:
      - limit_by_per_ip: true
        limit_keys:
        - key: "tokens_per_minute"
          token_per_minute: 10000
        - key: "tokens_per_day"
          token_per_day: 100000
```

### 多模型 Fallback

```yaml
apiVersion: extensions.higress.io/v1alpha1
kind: WasmPlugin
metadata:
  name: ai-proxy-fallback
  namespace: higress-system
spec:
  url: oci://higress-registry.cn-hangzhou.cr.aliyuncs.com/plugins/ai-proxy:latest
  matchRules:
  - ingress:
    - ai-route
    config:
      provider:
        type: openai
        apiTokens:
        - "${OPENAI_API_KEY}"
      fallbackConfig:
        enabled: true
        fallbackProvider:
          type: dashscope
          apiTokens:
          - "${DASHSCOPE_API_KEY}"
          modelMapping:
            "*": "qwen-max"
```

## 7. Gateway API 集成

Higress 支持 Gateway API v1.1 Extended 一致性级别：

```yaml
# 创建 GatewayClass
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: higress
spec:
  controllerName: higress.io/gateway-controller

---
# 创建 Gateway
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: higress-gw
  namespace: higress-system
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
      - name: tls-cert
    allowedRoutes:
      namespaces:
        from: All
```

## 8. 可观测性

### Prometheus 指标

```yaml
# Higress 默认暴露以下核心指标
# istio_requests_total           - 请求总数
# istio_request_duration_milliseconds - 请求延迟
# istio_request_bytes_total      - 请求字节数
# istio_response_bytes_total     - 响应字节数
# envoy_cluster_upstream_cx_active - 活跃上游连接数
```

### ServiceMonitor 配置

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: higress-gateway
  namespace: higress-system
spec:
  selector:
    matchLabels:
      app: higress-gateway
  endpoints:
  - port: http-envoy-prom
    path: /stats/prometheus
    interval: 15s
```

### 访问日志

```yaml
# 通过 Higress ConfigMap 配置访问日志格式
apiVersion: v1
kind: ConfigMap
metadata:
  name: higress-config
  namespace: higress-system
data:
  accessLogFormat: |
    {"timestamp":"%START_TIME%","method":"%REQ(:METHOD)%","path":"%REQ(X-ENVOY-ORIGINAL-PATH?:PATH)%","protocol":"%PROTOCOL%","response_code":"%RESPONSE_CODE%","duration":"%DURATION%","upstream":"%UPSTREAM_HOST%","request_id":"%REQ(X-REQUEST-ID)%"}
```

## 9. 生产环境调优

### 资源配置

```yaml
# Gateway Pod 资源配置建议
resources:
  requests:
    cpu: "2"
    memory: "2Gi"
  limits:
    cpu: "4"
    memory: "4Gi"
```

### HPA 配置

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: higress-gateway
  namespace: higress-system
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: higress-gateway
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
```

### Envoy 性能调优

```yaml
# 通过 EnvoyFilter 或 Higress 配置调优
# 关键参数
concurrency: 4                    # Worker 线程数，建议等于 CPU 核数
connection_idle_timeout: 3600s    # 连接空闲超时
per_connection_buffer_limit: 32KB # 每连接缓冲区
```

## 10. 与 Istio 协同

Higress 基于 Istiod 构建控制平面，可与 Istio 服务网格无缝协同：

```
┌────────────────────────────────────────────────────┐
│                 Higress + Istio 协同架构              │
│                                                    │
│  外部流量 → Higress Gateway (南北向)                  │
│                    │                               │
│                    ▼                               │
│              K8s Service                           │
│                    │                               │
│                    ▼                               │
│         Istio Sidecar (东西向)                      │
│                    │                               │
│                    ▼                               │
│              Backend Pod                           │
│                                                    │
│  共享 Istiod 控制平面:                               │
│  - 统一服务发现                                     │
│  - 统一证书管理                                     │
│  - 统一配置下发 (xDS)                               │
└────────────────────────────────────────────────────┘
```

如果集群中已部署 Istio，Higress 可以复用现有的 Istiod 实例，减少控制平面资源消耗。

---

## 参考资料

- [Higress 官方文档](https://higress.io/docs/overview/what-is-higress)
- [Higress GitHub](https://github.com/alibaba/higress)
- [Higress 插件市场](https://higress.io/plugin)
- [Domain-34: CNCF Landscape](../domain-34-cncf-landscape)
