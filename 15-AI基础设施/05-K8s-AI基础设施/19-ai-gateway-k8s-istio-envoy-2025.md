---
title: "AI Gateway on Kubernetes：Istio AI 扩展与 Envoy AI Gateway 2025"
description: "2025 年 Kubernetes 上的 AI Gateway 架构模式：Istio AI 扩展、Envoy AI Gateway、KAI Scheduler 语义路由与 LLM 流量治理"
summary: "全面覆盖 AI Gateway 在 K8s 上的落地方案：Envoy AI Gateway（LLM 路由/限流/可观测性）、Istio AI 扩展（Wasm 插件/智能路由）、KGateway/Kong AI Gateway、Token 级限流、语义缓存、模型故障转移与成本感知路由"
category: AI基础设施
tags:
- ai-gateway
- envoy
- istio
- llm-routing
- token-rate-limit
- semantic-cache
- kubernetes
- service-mesh
- api-gateway
- llm-observability
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- AI 工程师
- SRE
- 架构师
estimated_read_time: 22min
intent_queries:
- "K8s 上如何部署 AI Gateway"
- "Envoy AI Gateway 如何配置 LLM 路由"
- "Istio 如何做 LLM 流量治理"
- "AI Gateway Token 限流如何实现"
trigger_keywords:
- AI Gateway
- Envoy AI Gateway
- Istio AI
- LLM 路由
- Token 限流
- 语义缓存
prerequisites:
- kubectl-basics
- istio-basics
- envoy-basics
sources:
- https://gateway.envoyproxy.io/
- https://github.com/envoyproxy/ai-gateway
- https://istio.io/latest/docs/
- https://docs.konghq.com/hub/kong-inc/ai-proxy/
---

# AI Gateway on Kubernetes：Istio AI 扩展与 Envoy AI Gateway 2025

> AI Gateway 是 2024-2025 年 AI 基础设施的关键组件，专门针对 LLM 流量的特殊性（长连接、流式响应、Token 计量）设计。

## AI Gateway 架构概述

```
┌──────────────────────────────────────────────────────────┐
│                    外部流量 / 内部调用                     │
└─────────────────────────┬────────────────────────────────┘
                          │
┌─────────────────────────▼────────────────────────────────┐
│                    AI Gateway 层                          │
│  ┌──────────────┐ ┌──────────────┐ ┌─────────────────┐  │
│  │ 认证/授权     │ │ Token 限流   │ │ 语义缓存        │  │
│  │ API Key 管理  │ │ TPM/RPM      │ │ Redis 向量缓存  │  │
│  └──────────────┘ └──────────────┘ └─────────────────┘  │
│  ┌──────────────┐ ┌──────────────┐ ┌─────────────────┐  │
│  │ 模型路由     │ │ 故障转移     │ │ 可观测性        │  │
│  │ A/B 测试     │ │ 多 Provider  │ │ Token 追踪      │  │
│  └──────────────┘ └──────────────┘ └─────────────────┘  │
└────────┬─────────────────┬─────────────────┬─────────────┘
         │                 │                 │
    ┌────▼────┐      ┌─────▼──────┐   ┌──────▼──────┐
    │ vLLM    │      │  OpenAI    │   │   Bedrock   │
    │ K8s 集群│      │  API       │   │   Claude    │
    └─────────┘      └────────────┘   └─────────────┘
```

---

## Envoy AI Gateway

Envoy AI Gateway 是 Envoy 社区在 2024 年推出的专用 LLM 流量管理扩展，2025 年进入 Beta 阶段。

### 安装与基础配置

```bash
# 通过 Helm 安装 Envoy Gateway（含 AI 扩展）
helm repo add envoy-gateway https://gateway.envoyproxy.io/helm-repo
helm repo update

helm install eg envoy-gateway/gateway \
  --namespace envoy-gateway-system \
  --create-namespace \
  --version v1.2.0 \
  --set ai.enabled=true

# 等待就绪
kubectl wait --timeout=300s -n envoy-gateway-system \
  deployment/envoy-gateway --for=condition=Available
```

### LLM 后端路由配置

```yaml
# GatewayClass
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: ai-gateway-class
spec:
  controllerName: gateway.envoyproxy.io/gatewayclass-controller
---
# Gateway
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: ai-gateway
  namespace: ai-services
spec:
  gatewayClassName: ai-gateway-class
  listeners:
  - name: https
    port: 443
    protocol: HTTPS
    tls:
      mode: Terminate
      certificateRefs:
      - name: ai-gateway-tls
---
# AIGatewayRoute：LLM 路由规则
apiVersion: aigateway.envoyproxy.io/v1alpha1
kind: AIGatewayRoute
metadata:
  name: llm-route
  namespace: ai-services
spec:
  targetRefs:
  - name: ai-gateway
    kind: Gateway
    group: gateway.networking.k8s.io
  rules:
  # 规则1：GPT-4 高质量请求路由到 OpenAI
  - matches:
    - headers:
      - name: x-model-tier
        value: premium
    backendRefs:
    - name: openai-backend
      weight: 100
  # 规则2：普通请求路由到内部 vLLM
  - matches:
    - path:
        type: PathPrefix
        value: /v1/chat/completions
    backendRefs:
    - name: vllm-service
      weight: 80
    - name: openai-fallback
      weight: 20
---
# AI Backend 定义
apiVersion: aigateway.envoyproxy.io/v1alpha1
kind: AIBackend
metadata:
  name: vllm-service
  namespace: ai-services
spec:
  protocol: openai
  endpoint:
    host: vllm-service.ai-serving.svc.cluster.local
    port: 8000
    path: /v1
  schema:
    model: "Qwen2.5-72B-Instruct"
    maxTokens: 4096
```

### Token 级限流

```yaml
apiVersion: aigateway.envoyproxy.io/v1alpha1
kind: AITokenRateLimit
metadata:
  name: token-rate-limits
  namespace: ai-services
spec:
  targetRef:
    name: ai-gateway
    kind: Gateway
  limits:
  # 全局 Token 限制
  - type: Global
    tokenPerMinute: 1000000   # 1M TPM 全局上限
    requestPerMinute: 10000
  # 按 API Key 限制
  - type: PerAPIKey
    tokenPerMinute: 50000     # 5万 TPM/Key
    requestPerMinute: 500
    headerName: Authorization
  # 按用户 ID 限制
  - type: PerUser
    tokenPerMinute: 10000
    requestPerMinute: 100
    headerName: x-user-id
  # 按模型限制
  - type: PerModel
    limits:
    - model: "gpt-4"
      tokenPerMinute: 100000
    - model: "gpt-3.5-turbo"
      tokenPerMinute: 500000
```

### 语义缓存配置

```yaml
apiVersion: aigateway.envoyproxy.io/v1alpha1
kind: AISemanticCache
metadata:
  name: llm-semantic-cache
  namespace: ai-services
spec:
  targetRef:
    name: ai-gateway
    kind: Gateway
  cache:
    provider: redis
    endpoint: redis-cache.ai-infra.svc.cluster.local:6379
    ttl: 3600s
    maxSize: "10Gi"
  similarity:
    threshold: 0.95           # 余弦相似度阈值
    embeddingModel: text-embedding-3-small
    embeddingDimensions: 1536
  # 不缓存的请求特征
  bypass:
    - headerName: Cache-Control
      value: no-cache
    - hasStreamingResponse: true
    - hasTools: true          # 工具调用不缓存
```

### 可观测性配置

```yaml
apiVersion: aigateway.envoyproxy.io/v1alpha1
kind: AIObservability
metadata:
  name: ai-gateway-obs
  namespace: ai-services
spec:
  metrics:
    enabled: true
    additionalLabels:
      - model
      - api_key_id
      - user_id
      - provider
    customMetrics:
    - name: ai_gateway_tokens_total
      type: counter
      description: "Total tokens processed"
      labels: [model, direction, provider]
    - name: ai_gateway_cost_usd_total
      type: counter
      description: "Total cost in USD"
      labels: [model, provider, api_key_id]
    - name: ai_gateway_ttft_seconds
      type: histogram
      description: "Time to first token"
      buckets: [0.1, 0.25, 0.5, 1.0, 2.0, 5.0]
  tracing:
    enabled: true
    provider: otlp
    endpoint: otel-collector.monitoring.svc.cluster.local:4317
    sampleRate: 0.1
  logging:
    requestBody: false      # 不记录请求体（隐私保护）
    responseBody: false
    tokenCounts: true       # 记录 Token 用量
```

---

## Istio AI 扩展

### Wasm 插件实现 LLM 流量治理

Istio 1.22+ 支持通过 WasmPlugin 扩展实现 AI 专属流量治理：

```yaml
# 部署 LLM 流量治理 Wasm 插件
apiVersion: extensions.istio.io/v1alpha1
kind: WasmPlugin
metadata:
  name: llm-token-limiter
  namespace: ai-serving
spec:
  selector:
    matchLabels:
      app: vllm-service
  url: oci://ghcr.io/kudig-io/wasm/llm-token-limiter:v1.2.0
  phase: AUTHN
  pluginConfig:
    redis_host: "redis-cache.ai-infra.svc.cluster.local:6379"
    token_per_minute: 100000
    count_input_tokens: true
    count_output_tokens: true
    model_header: "x-model-name"
```

### VirtualService AI 路由

```yaml
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: llm-ai-router
  namespace: ai-serving
spec:
  hosts:
  - llm-gateway.ai-serving.svc.cluster.local
  http:
  # 金丝雀：新模型版本 10% 流量
  - match:
    - headers:
        x-canary-user:
          exact: "true"
    route:
    - destination:
        host: vllm-qwen2-5-72b-v2
        port:
          number: 8000
      weight: 100
  # 内容路由：代码任务到 DeepSeek Coder
  - match:
    - headers:
        x-task-type:
          exact: "code"
    route:
    - destination:
        host: vllm-deepseek-coder-v3
        port:
          number: 8000
  # 默认路由
  - route:
    - destination:
        host: vllm-qwen2-5-72b
        port:
          number: 8000
      weight: 100
    timeout: 300s
    retries:
      attempts: 1
      retryOn: 5xx,reset,connect-failure
```

### EnvoyFilter 注入 Token 计量

```yaml
apiVersion: networking.istio.io/v1alpha3
kind: EnvoyFilter
metadata:
  name: llm-token-meter
  namespace: ai-serving
spec:
  workloadSelector:
    labels:
      app: vllm-service
  configPatches:
  - applyTo: HTTP_FILTER
    match:
      context: SIDECAR_INBOUND
      listener:
        filterChain:
          filter:
            name: envoy.filters.network.http_connection_manager
    patch:
      operation: INSERT_BEFORE
      value:
        name: envoy.filters.http.lua
        typed_config:
          "@type": type.googleapis.com/envoy.extensions.filters.http.lua.v3.LuaPerRoute
          inline_code: |
            function envoy_on_response(response_handle)
              local usage = response_handle:headers():get("x-token-usage")
              if usage then
                response_handle:streamInfo():dynamicMetadata():set(
                  "llm_tokens", "total", tonumber(usage))
              end
            end
```

---

## Kong AI Gateway

Kong 的 AI Proxy 插件 2025 年已成为企业级 AI Gateway 主流选项之一。

```yaml
# KongPlugin：AI Proxy 配置
apiVersion: configuration.konghq.com/v1
kind: KongPlugin
metadata:
  name: ai-proxy-openai
  namespace: ai-services
config:
  route_type: "llm/v1/chat"
  auth:
    header_name: Authorization
    header_value: "Bearer $(OPENAI_API_KEY)"
  model:
    provider: openai
    name: gpt-4o
    options:
      max_tokens: 4096
      temperature: 0.7
  logging:
    log_statistics: true
    log_payloads: false
plugin: ai-proxy
---
# KongPlugin：AI Rate Limiting（Token 级）
apiVersion: configuration.konghq.com/v1
kind: KongPlugin
metadata:
  name: ai-rate-limit
  namespace: ai-services
config:
  llm_providers:
  - name: openai
    limit_type: tokens
    limits:
      minute: 100000
      hour: 2000000
      day: 20000000
  identifier: consumer
plugin: ai-rate-limiting-advanced
```

---

## AI Gateway 选型对比（2025）

| 维度 | Envoy AI Gateway | Istio + WasmPlugin | Kong AI Gateway | KGateway |
|------|-----------------|-------------------|----------------|---------|
| 成熟度 | Beta | GA | GA | Alpha |
| Token 限流 | 原生 | 插件 | 原生 | 有限 |
| 语义缓存 | 原生 | 需插件 | 插件 | 无 |
| 多 Provider 路由 | 原生 | 需定制 | 原生 | 部分 |
| 流式响应 | 完整 | 完整 | 完整 | 部分 |
| 成本追踪 | 原生 | 需开发 | 插件 | 无 |
| K8s Gateway API | 完整 | 完整 | 部分 | 完整 |
| 社区活跃度 | 高 | 高 | 高 | 中 |

### 推荐场景

```
企业多租户 AI 平台  →  Envoy AI Gateway（Token 治理最完整）
已有 Istio 服务网格  →  Istio + WasmPlugin（复用已有基础设施）
需要商业支持       →  Kong AI Gateway
纯 K8s Gateway API →  Envoy Gateway（遵循标准）
```

---

## 生产运维最佳实践

### AI Gateway 高可用部署

```yaml
# Envoy AI Gateway 高可用配置
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: EnvoyProxy
metadata:
  name: ai-gateway-proxy
  namespace: ai-services
spec:
  provider:
    type: Kubernetes
    kubernetes:
      envoyDeployment:
        replicas: 3
        pod:
          affinity:
            podAntiAffinity:
              requiredDuringSchedulingIgnoredDuringExecution:
              - labelSelector:
                  matchLabels:
                    app: envoy-ai-gateway
                topologyKey: kubernetes.io/hostname
          resources:
            requests:
              cpu: "2"
              memory: "4Gi"
            limits:
              cpu: "4"
              memory: "8Gi"
      envoyService:
        type: LoadBalancer
        annotations:
          service.beta.kubernetes.io/aws-load-balancer-type: nlb
```

### 监控告警

```yaml
# Prometheus 告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ai-gateway-alerts
  namespace: monitoring
spec:
  groups:
  - name: ai-gateway
    rules:
    - alert: AIGatewayHighTokenUsage
      expr: |
        rate(ai_gateway_tokens_total[5m]) > 50000
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "AI Gateway Token 使用率过高"
        description: "当前 TPM: {{ $value | humanize }}"
    - alert: AIGatewayHighErrorRate
      expr: |
        rate(ai_gateway_requests_total{status=~"5.."}[5m]) /
        rate(ai_gateway_requests_total[5m]) > 0.05
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "AI Gateway 错误率超过 5%"
    - alert: AIGatewayHighTTFT
      expr: |
        histogram_quantile(0.95, rate(ai_gateway_ttft_seconds_bucket[5m])) > 3
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "AI Gateway P95 首 Token 延迟过高"
```

---

## 参考资源

- [Envoy AI Gateway GitHub](https://github.com/envoyproxy/ai-gateway)
- [Envoy Gateway 文档](https://gateway.envoyproxy.io/)
- [Istio WasmPlugin 参考](https://istio.io/latest/docs/reference/config/proxy_extensions/wasm-plugin/)
- [Kong AI Gateway 文档](https://docs.konghq.com/hub/kong-inc/ai-proxy/)
- [KAI Scheduler](https://github.com/NVIDIA/KAI-Scheduler)
