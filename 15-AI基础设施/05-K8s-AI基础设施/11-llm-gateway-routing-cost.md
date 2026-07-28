---
title: "LLM Gateway 与推理路由"
description: "K8s 上 LLM Gateway 部署运维：统一 API 网关、多模型路由、Token 成本追踪、限流配额与语义缓存"
summary: "覆盖 LiteLLM/Portkey/OpenRouter 等开源 LLM Gateway 方案对比，K8s 部署实践，cost-based/latency-based/fallback 路由策略，Token 用量追踪与成本归因，限流配额管理及语义缓存"
category: AI基础设施
tags:
- llm-gateway
- litellm
- routing
- cost-tracking
- rate-limiting
- semantic-cache
- api-gateway
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- MLOps 工程师
- SRE
estimated_read_time: 20min
intent_queries:
- "LLM Gateway 怎么在 K8s 上部署"
- "如何追踪多模型的 Token 成本"
- "LiteLLM 和 Portkey 怎么选"
trigger_keywords:
- llm-gateway
- litellm
- portkey
- routing
- cost-tracking
- rate-limiting
- token
prerequisites:
- kubectl-basics
- helm-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# LLM Gateway 与推理路由

## 概述

随着企业 AI 应用从单一模型调用演进为多模型、多供应商的复杂架构，LLM Gateway（也称为 AI Gateway 或 LLM Proxy）成为连接应用层与模型层的关键基础设施。它提供统一的 API 接口、智能路由、成本追踪、限流配额、故障转移和缓存等能力，使 AI 应用无需关心底层模型的部署细节和供应商差异。

LLM Gateway 解决的核心问题包括：多供应商 API 格式不统一（OpenAI/Anthropic/Azure/自部署模型）、Token 成本不可控、缺乏统一的限流和配额管理、无法实现模型级别的故障转移和负载均衡。本文覆盖开源方案对比、K8s 生产部署、路由策略设计、成本归因和故障排查。

相关页面：[[15-AI基础设施/05-K8s-AI基础设施/02-vllm-inference-serving-production|vLLM推理引擎部署]]、[[22-概念/03-网络/ingress|K8s Ingress与流量管理]]、[[23-实体/07-可观测性/prometheus|Prometheus监控体系]]、[[15-AI基础设施/05-K8s-AI基础设施/12-ai-observability-arize-phoenix|AI可观测性平台]]、[[17-系统基础/06-知识字典/configuration/resource-quota|K8s资源配额与LimitRange]]

## 架构与核心概念

### LLM Gateway 功能架构

```
LLM Gateway 核心功能:

统一 API 层:
  - OpenAI 兼容 API（/v1/chat/completions, /v1/embeddings）
  - 多供应商适配（OpenAI, Anthropic, Azure, Bedrock, 自部署 vLLM）
  - 请求/响应格式标准化
  - Streaming SSE 支持

智能路由:
  - Cost-based: 优先选择低成本模型
  - Latency-based: 优先选择低延迟模型
  - Fallback: 主模型失败自动切换备选
  - Load-balancing: 多实例/多区域负载均衡
  - Content-based: 根据 prompt 复杂度选择模型

成本管控:
  - Token 用量实时追踪（input/output 分别计费）
  - 按 team/user/project 成本归因
  - 预算上限和告警
  - 成本报表和趋势分析

流量治理:
  - 按 key/user/team 限流（RPM/TPM）
  - 并发请求数控制
  - 请求队列和优先级
  - 配额管理（日/月 Token 上限）

缓存与优化:
  - Semantic Cache（语义相似查询复用）
  - Prompt Cache（相同 prompt 前缀复用）
  - 响应缓存（TTL 控制）
```

### 开源方案对比

| 维度 | LiteLLM | Portkey | OpenRouter (self-hosted) | Kong AI Gateway |
|------|---------|---------|------------------------|-----------------|
| 定位 | LLM Proxy + Router | AI Gateway (SaaS + 开源) | 模型聚合路由 | API Gateway + AI 插件 |
| 部署模式 | Proxy / SDK | Cloud / Self-hosted | Cloud only | Self-hosted |
| 支持模型数 | 100+ | 200+ | 200+ | 插件扩展 |
| 路由策略 | Fallback/LoadBalance/Cost | 全部 + A/B test | Cost/Latency | 插件自定义 |
| 成本追踪 | 内置（按 key/team） | 内置 + 高级分析 | 内置 | 插件 |
| 限流 | 内置（RPM/TPM/Budget） | 内置 + 高级配额 | 内置 | 原生支持 |
| Semantic Cache | 支持（Redis） | 支持 | 不支持 | 插件 |
| 可观测性 | Prometheus + Langfuse | 内置 Dashboard | 内置 | Datadog 等 |
| K8s 部署 | Helm / Docker | Helm | N/A (SaaS) | Helm |
| 性能 | 高（Python async） | 高（Node.js） | N/A | 极高（Lua/OpenResty） |
| 适用场景 | 中小团队快速上手 | 企业级全功能 | 个人/小团队 | 已有 Kong 基础设施 |

### 路由策略设计

```
多模型路由决策流程:

请求进入 → 认证 & 限流检查
  │
  ├─ 缓存命中? → 返回缓存响应
  │
  ├─ 路由策略评估:
  │   ├─ 模型指定? → 直接路由到指定模型
  │   ├─ Cost-based → 选择满足质量要求的最低成本模型
  │   ├─ Latency-based → 选择 P95 延迟最低的可用模型
  │   ├─ Load-based → 选择当前负载最低的实例
  │   └─ Content-based → 分析 prompt 复杂度选择模型
  │
  ├─ 主模型调用
  │   ├─ 成功 → 记录指标 → 返回响应
  │   └─ 失败/超时 → Fallback 链
  │       ├─ Fallback-1 (同供应商不同模型)
  │       ├─ Fallback-2 (不同供应商)
  │       └─ Fallback-3 (自部署模型)
  │
  └─ 记录: Token 用量、延迟、成本、错误
```

## 生产部署

### LiteLLM 部署

```yaml
# 🟡 中风险：LiteLLM K8s 部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: litellm-proxy
  namespace: ai-gateway
  labels:
    app: litellm-proxy
spec:
  replicas: 3
  selector:
    matchLabels:
      app: litellm-proxy
  template:
    metadata:
      labels:
        app: litellm-proxy
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "4000"
        prometheus.io/path: /metrics
    spec:
      containers:
      - name: litellm
        image: ghcr.io/berriai/litellm:main-v1.52.0
        ports:
        - containerPort: 4000
          name: http
        command:
        - litellm
        - --config=/app/config.yaml
        - --port=4000
        - --num_workers=4
        env:
        - name: LITELLM_MASTER_KEY
          valueFrom:
            secretKeyRef:
              name: litellm-secrets
              key: master-key
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: litellm-secrets
              key: database-url
        - name: OPENAI_API_KEY
          valueFrom:
            secretKeyRef:
              name: llm-provider-keys
              key: openai-key
        - name: ANTHROPIC_API_KEY
          valueFrom:
            secretKeyRef:
              name: llm-provider-keys
              key: anthropic-key
        - name: REDIS_HOST
          value: "redis.ai-gateway.svc"
        - name: REDIS_PORT
          value: "6379"
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
          limits:
            cpu: "4"
            memory: "8Gi"
        livenessProbe:
          httpGet:
            path: /health/liveliness
            port: 4000
          initialDelaySeconds: 10
          periodSeconds: 15
        readinessProbe:
          httpGet:
            path: /health/readiness
            port: 4000
          initialDelaySeconds: 5
          periodSeconds: 10
        volumeMounts:
        - name: config
          mountPath: /app/config.yaml
          subPath: config.yaml
      volumes:
      - name: config
        configMap:
          name: litellm-config
---
apiVersion: v1
kind: Service
metadata:
  name: litellm-proxy
  namespace: ai-gateway
spec:
  selector:
    app: litellm-proxy
  ports:
  - port: 4000
    targetPort: 4000
    name: http
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: litellm-proxy-hpa
  namespace: ai-gateway
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: litellm-proxy
  minReplicas: 3
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Pods
    pods:
      metric:
        name: litellm_proxy_active_requests
      target:
        type: AverageValue
        averageValue: "50"
```

### LiteLLM 路由配置

```yaml
# 🟡 中风险：LiteLLM 路由和模型配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: litellm-config
  namespace: ai-gateway
data:
  config.yaml: |
    model_list:
      # 自部署 vLLM 模型（低成本）
      - model_name: internal-llama-70b
        litellm_params:
          model: openai/internal-llama-70b
          api_base: http://vllm-llama70b.ai-inference.svc:8000/v1
          api_key: os.environ/VLLM_API_KEY
          rpm: 100
          tpm: 500000

      # OpenAI GPT-4o（高质量）
      - model_name: gpt-4o
        litellm_params:
          model: openai/gpt-4o
          api_key: os.environ/OPENAI_API_KEY
          rpm: 500
          tpm: 2000000

      # Anthropic Claude（长文本）
      - model_name: claude-sonnet
        litellm_params:
          model: anthropic/claude-sonnet-4-20250514
          api_key: os.environ/ANTHROPIC_API_KEY
          rpm: 200
          tpm: 1000000

      # Azure OpenAI（合规场景）
      - model_name: azure-gpt4o
        litellm_params:
          model: azure/gpt-4o-deployment
          api_base: https://myresource.openai.azure.com
          api_key: os.environ/AZURE_API_KEY
          api_version: "2024-08-01-preview"

    # 路由策略
    router_settings:
      routing_strategy: simple-shuffle  # 或 least-busy, latency-based-routing
      num_retries: 2
      timeout: 60
      retry_after: 5
      fallbacks:
      - model_name: gpt-4o
        litellm_params:
          model: anthropic/claude-sonnet-4-20250514
      - model_name: claude-sonnet
        litellm_params:
          model: openai/internal-llama-70b

    # 限流配置
    litellm_settings:
      max_budget: 10000  # 月预算上限 $10000
      budget_duration: 30d
      cache: true
      cache_type: redis
      cache_params:
        host: redis.ai-gateway.svc
        port: 6379
        ttl: 3600
        supported_call_types:
        - acompletion
        - aembedding

    # 通用设置
    general_settings:
      master_key: os.environ/LITELLM_MASTER_KEY
      database_url: os.environ/DATABASE_URL
      enable_spend_tracking: true
      spend_metric_name: litellm_spend_metric
```

### 密钥管理

```yaml
# 🔴 高风险：LLM 供应商 API Key 存储（使用 External Secrets 或 Sealed Secrets）
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: llm-provider-keys
  namespace: ai-gateway
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: vault-backend
    kind: ClusterSecretStore
  target:
    name: llm-provider-keys
    creationPolicy: Owner
  data:
  - secretKey: openai-key
    remoteRef:
      key: ai-gateway/providers
      property: openai_api_key
  - secretKey: anthropic-key
    remoteRef:
      key: ai-gateway/providers
      property: anthropic_api_key
  - secretKey: azure-key
    remoteRef:
      key: ai-gateway/providers
      property: azure_api_key
```

## 运维操作

### Token 用量追踪与成本归因

```bash
# 🟢 低风险：查看 LiteLLM 支出统计
LITELLM_URL="http://litellm-proxy.ai-gateway.svc:4000"
MASTER_KEY="sk-litellm-master-key"

# 按 key 查看支出
curl -s "${LITELLM_URL}/spend/keys" \
  -H "Authorization: Bearer ${MASTER_KEY}" | jq .

# 按用户查看支出
curl -s "${LITELLM_URL}/spend/users" \
  -H "Authorization: Bearer ${MASTER_KEY}" | jq .

# 按模型查看支出
curl -s "${LITELLM_URL}/spend/models" \
  -H "Authorization: Bearer ${MASTER_KEY}" | jq '.[] | {model, spend, tokens}'

# 🟡 中风险：创建团队 Key 并设置预算
curl -s -X POST "${LITELLM_URL}/key/generate" \
  -H "Authorization: Bearer ${MASTER_KEY}" \
  -H "Content-Type: application/json" \
  -d '{
    "team_id": "team-ml-platform",
    "key_alias": "ml-platform-prod",
    "max_budget": 5000,
    "budget_duration": "30d",
    "tpm_limit": 1000000,
    "rpm_limit": 200,
    "models": ["gpt-4o", "claude-sonnet", "internal-llama-70b"],
    "metadata": {
      "team": "ML Platform",
      "cost_center": "CC-AI-001"
    }
  }'
```

### 限流与配额管理

```bash
# 🟢 低风险：查看当前限流状态
curl -s "${LITELLM_URL}/key/info?key=sk-team-ml-xxx" \
  -H "Authorization: Bearer ${MASTER_KEY}" | \
  jq '{tpm_limit, rpm_limit, tpm_used, rpm_used, max_budget, spend}'

# 🟡 中风险：动态调整限流（应对突发流量）
curl -s -X POST "${LITELLM_URL}/key/update" \
  -H "Authorization: Bearer ${MASTER_KEY}" \
  -H "Content-Type: application/json" \
  -d '{
    "key": "sk-team-ml-xxx",
    "tpm_limit": 2000000,
    "rpm_limit": 500,
    "duration": "2h"
  }'

# 🟡 中风险：设置全局预算告警
curl -s -X POST "${LITELLM_URL}/budget/alert" \
  -H "Authorization: Bearer ${MASTER_KEY}" \
  -H "Content-Type: application/json" \
  -d '{
    "alert_threshold": 0.8,
    "alert_email": "ai-platform@company.com",
    "alert_webhook": "https://hooks.slack.com/services/xxx"
  }'
```

### 语义缓存配置

```bash
# 🟢 低风险：测试语义缓存命中
# 第一次请求（缓存未命中）
time curl -s -X POST "${LITELLM_URL}/v1/chat/completions" \
  -H "Authorization: Bearer sk-team-ml-xxx" \
  -H "Content-Type: application/json" \
  -d '{
    "model": "gpt-4o",
    "messages": [{"role": "user", "content": "什么是 Kubernetes?"}],
    "max_tokens": 200
  }'

# 第二次请求（语义相似，应命中缓存）
time curl -s -X POST "${LITELLM_URL}/v1/chat/completions" \
  -H "Authorization: Bearer sk-team-ml-xxx" \
  -H "Content-Type: application/json" \
  -d '{
    "model": "gpt-4o",
    "messages": [{"role": "user", "content": "Kubernetes 是什么?"}],
    "max_tokens": 200
  }'
```

## 故障排查

### 上游模型超时

```bash
# 🟢 低风险：诊断上游超时
# Step 1: 检查 LiteLLM 日志
kubectl logs -n ai-gateway deploy/litellm-proxy --tail=100 | grep -i "timeout\|error\|fallback"

# Step 2: 检查上游模型服务健康
kubectl get pods -n ai-inference -l app=vllm-llama70b
kubectl exec -n ai-inference deploy/vllm-llama70b -- curl -s http://localhost:8000/health

# Step 3: 检查网络连通性
kubectl exec -n ai-gateway deploy/litellm-proxy -- \
  curl -s -o /dev/null -w "%{http_code} %{time_total}s" \
  http://vllm-llama70b.ai-inference.svc:8000/v1/models

# Step 4: 检查 Fallback 是否生效
kubectl logs -n ai-gateway deploy/litellm-proxy --tail=50 | grep -i "fallback\|retry"

# 解决方案:
# 1. 增加 timeout 配置（默认 60s，长文本生成可能需要 120s+）
# 2. 配置 Fallback 链（主模型超时自动切换）
# 3. 检查上游模型 GPU 利用率（可能过载）
# 4. 增加上游模型副本数
```

### 限流触发排查

```bash
# 🟢 低风险：诊断限流问题
# 检查 429 错误
kubectl logs -n ai-gateway deploy/litellm-proxy --tail=200 | grep "429\|rate_limit\|RateLimitError"

# 查看当前 RPM/TPM 使用
curl -s "${LITELLM_URL}/global/spend" \
  -H "Authorization: Bearer ${MASTER_KEY}" | jq .

# 检查供应商侧限流
kubectl logs -n ai-gateway deploy/litellm-proxy --tail=100 | grep "provider.*rate\|upstream.*429"

# 常见原因:
# 1. 团队 TPM 配额用尽 → 增加 tpm_limit 或等待重置
# 2. 供应商侧限流（OpenAI tier limit）→ 配置多 key 轮转
# 3. 突发流量 → 配置请求队列和退避重试
# 4. 预算超限 → 检查 max_budget 设置
```

### 成本异常排查

| 故障现象 | 可能原因 | 排查方法 | 解决方案 |
|---------|---------|---------|---------|
| 日成本突增 3x+ | 某团队/应用异常调用 | 按 key/team 查看 spend | 定位异常 key，临时限流 |
| Token 用量与预期不符 | 缓存未命中/重复调用 | 检查 cache hit rate | 优化缓存策略、去重 |
| 模型选择非最优 | 路由配置错误 | 检查 routing_strategy | 修正路由规则 |
| 预算告警频繁 | 预算设置过低或用量增长 | 查看 30 天趋势 | 调整预算或优化 prompt |
| 成本归因不准 | Key 共享/未标记 team | 检查 key metadata | 实施一团队一 key 策略 |

### 监控告警

```yaml
# 🟢 低风险：LLM Gateway Prometheus 告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: llm-gateway-alerts
  namespace: monitoring
spec:
  groups:
  - name: llm-gateway.rules
    rules:
    - alert: LLMGatewayHighErrorRate
      expr: rate(litellm_proxy_failed_requests_total[5m]) / rate(litellm_proxy_requests_total[5m]) > 0.1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "LLM Gateway 错误率超过 10%"
    - alert: LLMGatewayHighLatency
      expr: histogram_quantile(0.95, rate(litellm_proxy_request_duration_seconds_bucket[5m])) > 30
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "LLM Gateway P95 延迟超过 30s"
    - alert: LLMGatewayBudgetExceeded
      expr: litellm_team_spend{team="ml-platform"} > litellm_team_budget{team="ml-platform"} * 0.9
      for: 1m
      labels:
        severity: warning
      annotations:
        summary: "团队 ML Platform 预算使用超过 90%"
    - alert: LLMGatewayUpstreamDown
      expr: litellm_proxy_model_healthy{model="internal-llama-70b"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "上游模型 internal-llama-70b 不可用"
```

## 最佳实践

### 路由策略设计

1. **分层路由**：简单查询 → 自部署小模型（低成本）；复杂推理 → GPT-4o/Claude（高质量）
2. **Fallback 链**：自部署模型 → Azure OpenAI → OpenAI → Anthropic（按成本和可用性排序）
3. **A/B 测试**：新模型上线时按 10% 流量灰度，对比质量和成本
4. **超时分级**：聊天场景 30s，批处理场景 120s，embedding 10s

### 成本优化

```
成本优化策略:

1. 缓存优先:
   - Semantic Cache 命中率目标 > 30%
   - 高频 FAQ 类查询缓存 TTL 24h
   - 减少重复计算

2. 模型降级:
   - 简单任务用 GPT-4o-mini / Llama-70B
   - 复杂任务才用 GPT-4o / Claude Opus
   - 基于 prompt token 数自动选择

3. Prompt 优化:
   - 系统 prompt 精简（减少 input token）
   - 使用 Prompt Caching（Anthropic/OpenAI）
   - 批量请求合并

4. 预算管控:
   - 每团队月度预算硬限制
   - 80% 预算告警
   - 超预算自动降级到自部署模型
```

### 安全与合规

1. **API Key 管理**：使用 External Secrets / Vault 存储供应商 Key，禁止明文 ConfigMap
2. **访问控制**：每团队独立 Key，绑定可用模型列表和预算
3. **审计日志**：记录所有请求的 model/tokens/cost/user（脱敏 prompt 内容）
4. **数据驻留**：敏感数据仅路由到自部署模型或合规区域（Azure 特定 region）
5. **PII 过滤**：Gateway 层集成 PII 检测，阻止敏感信息发送到外部供应商

### 高可用设计

- **多副本部署**：LiteLLM Proxy 至少 3 副本，HPA 按并发请求数扩缩
- **Redis 高可用**：缓存和限流状态使用 Redis Sentinel/Cluster
- **数据库**：Spend 数据使用 PostgreSQL（CloudNativePG），定期备份
- **多区域**：关键供应商配置多区域 endpoint（Azure 多 region failover）
- **降级策略**：所有外部供应商不可用时，自动降级到自部署模型

## Related

- [[15-AI基础设施/05-K8s-AI基础设施/02-vllm-inference-serving-production|vLLM推理引擎部署]]
- [[22-概念/03-网络/ingress|K8s Ingress与流量管理]]
- [[23-实体/07-可观测性/prometheus|Prometheus监控体系]]
- [[15-AI基础设施/05-K8s-AI基础设施/12-ai-observability-arize-phoenix|AI可观测性平台]]
- [[17-系统基础/06-知识字典/configuration/resource-quota|K8s资源配额与LimitRange]]
