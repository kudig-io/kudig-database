---
title: "弹性模式：Circuit Breaker 与容错设计"
description: "K8s 环境下的弹性模式实现：Circuit Breaker、Bulkhead、Retry、Timeout、Fallback、Rate Limiting 在 Istio/Envoy/应用层的实践"
summary: "系统化的弹性模式实践指南，覆盖 Circuit Breaker 熔断器、Bulkhead 舱壁隔离、Retry 重试策略、Timeout 超时控制、Fallback 降级方案和 Rate Limiting 限流在 Kubernetes 中的多层实现，包括 Istio Service Mesh、Envoy Sidecar 和应用层 SDK 的配置与调优"
category: 可靠性
tags:
- circuit-breaker
- bulkhead
- retry
- timeout
- fallback
- rate-limiting
- resilience
- istio
- envoy
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- "Kubernetes 中如何实现 Circuit Breaker 熔断"
- "Istio 弹性策略如何配置"
- "微服务容错模式在 K8s 中的最佳实践"
trigger_keywords:
- circuit-breaker
- 熔断
- bulkhead
- retry
- timeout
- rate-limiting
- 弹性模式
prerequisites:
- kubectl-basics
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

# 弹性模式：Circuit Breaker 与容错设计

## 概述

分布式系统中，单个服务的故障可能通过调用链级联传播，最终导致整个系统不可用——这就是"雪崩效应"。弹性模式（Resilience Patterns）是应对分布式故障的核心工程实践，通过 Circuit Breaker（熔断）、Bulkhead（舱壁隔离）、Retry（重试）、Timeout（超时）、Fallback（降级）和 Rate Limiting（限流）等模式，将故障控制在局部范围内，保护系统整体可用性。

在 Kubernetes 环境中，弹性模式可以在三个层次实现：基础设施层（Istio/Envoy Service Mesh）、平台层（API Gateway/Ingress）和应用层（SDK/框架）。本文覆盖所有三个层次的配置实践，帮助 SRE 和平台工程师构建多层防御的弹性体系。

与 [[可靠性/SRE实践/05-error-budget-automation.md|错误预算自动化]] 侧重 SLO 驱动的策略不同，本文聚焦于请求级别的容错机制设计与实现。

## 核心概念

### 弹性模式全景图

```
┌─────────────────────────────────────────────────────────────────┐
│                    弹性模式层次架构                                │
│                                                                   │
│  请求流入                                                         │
│     │                                                             │
│     ▼                                                             │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  Layer 1: Rate Limiting（入口限流）                        │    │
│  │  • 全局 QPS 限制                                          │    │
│  │  • 按租户/用户限流                                        │    │
│  │  • 突发流量整形 (Token Bucket / Leaky Bucket)             │    │
│  └──────────────────────────────────────────────────────────┘    │
│     │                                                             │
│     ▼                                                             │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  Layer 2: Timeout（超时控制）                              │    │
│  │  • 连接超时 / 请求超时 / 空闲超时                          │    │
│  │  • 级联超时预算 (Deadline Propagation)                    │    │
│  └──────────────────────────────────────────────────────────┘    │
│     │                                                             │
│     ▼                                                             │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  Layer 3: Circuit Breaker（熔断器）                        │    │
│  │  • Closed → Open → Half-Open 状态机                      │    │
│  │  • 错误率/延迟触发                                        │    │
│  │  • 快速失败，保护下游                                     │    │
│  └──────────────────────────────────────────────────────────┘    │
│     │                                                             │
│     ▼                                                             │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  Layer 4: Retry（重试策略）                                │    │
│  │  • 指数退避 + 抖动 (Exponential Backoff + Jitter)         │    │
│  │  • 重试预算 (Retry Budget)                                │    │
│  │  • 幂等性保证                                             │    │
│  └──────────────────────────────────────────────────────────┘    │
│     │                                                             │
│     ▼                                                             │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  Layer 5: Bulkhead（舱壁隔离）                             │    │
│  │  • 连接池隔离                                             │    │
│  │  • 线程池隔离                                             │    │
│  │  • 资源配额隔离 (K8s ResourceQuota)                       │    │
│  └──────────────────────────────────────────────────────────┘    │
│     │                                                             │
│     ▼                                                             │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  Layer 6: Fallback（降级方案）                             │    │
│  │  • 缓存降级 / 默认值降级                                  │    │
│  │  • 功能降级 / 服务降级                                    │    │
│  │  • 静态响应 / 排队等待                                    │    │
│  └──────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

### 弹性模式对比

| 模式 | 保护对象 | 触发条件 | 恢复方式 | 实现层 |
|------|---------|---------|---------|--------|
| Circuit Breaker | 下游服务 | 错误率/延迟超阈值 | 半开探测 | Mesh/应用 |
| Bulkhead | 本服务资源 | 资源耗尽 | 资源释放 | 应用/K8s |
| Retry | 瞬时故障 | 请求失败 | 自动重试 | Mesh/应用 |
| Timeout | 调用方时间 | 响应超时 | 快速失败 | Mesh/应用 |
| Fallback | 用户体验 | 主路径不可用 | 备用路径 | 应用 |
| Rate Limiting | 系统容量 | 流量超限 | 拒绝/排队 | Gateway/Mesh |

### Circuit Breaker 状态机

```
         错误率 > 阈值                    探测成功
    ┌─────────────────┐            ┌─────────────────┐
    │                 ▼            │                 │
┌───┴───┐      ┌──────────┐      ┌┴────────┐       │
│ CLOSED │      │   OPEN   │─────▶│HALF-OPEN│       │
│(正常通行)│      │(快速失败) │ 超时后 │(有限探测) │       │
└───┬───┘      └──────────┘      └┬────────┘       │
    │                              │                 │
    │         探测失败              │  探测成功        │
    │◀─────────────────────────────┘─────────────────┘
    │
    │  错误率 < 阈值（持续正常）
    └──────────────────────────────────────────────▶ 保持 CLOSED
```

## 生产部署/实现

### Istio DestinationRule：Circuit Breaker + 连接池隔离

```yaml
# 🟡 中风险：修改流量策略影响服务间通信行为
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: payment-service-resilience
  namespace: production
spec:
  host: payment-service.production.svc.cluster.local
  trafficPolicy:
    # Circuit Breaker 配置
    outlierDetection:
      # 连续 5 次 5xx 错误触发熔断
      consecutive5xxErrors: 5
      # 连续网关错误（连接失败等）
      consecutiveGatewayErrors: 3
      # 错误率超过 50% 触发熔断（需要最小请求量）
      # 注意：Istio 不直接支持错误率触发，通过 outlierDetection 间接实现
      interval: 10s
      # 熔断持续时间（Open 状态时长）
      baseEjectionTime: 30s
      # 最大熔断比例（不超过 50% 的实例被熔断）
      maxEjectionPercent: 50
      # 最小健康比例（低于此值不再熔断更多实例）
      minHealthPercent: 30

    # 连接池配置（Bulkhead 模式）
    connectionPool:
      tcp:
        # 最大 TCP 连接数（舱壁隔离）
        maxConnections: 100
        # 连接超时
        connectTimeout: 5s
        # TCP keepalive
        tcpKeepalive:
          time: 7200s
          interval: 75s
      http:
        # 每连接最大请求数（HTTP/2 多路复用）
        maxRequestsPerConnection: 100
        # 最大并发请求数
        http2MaxRequests: 1000
        # 空闲超时
        idleTimeout: 60s
        # 重试时不重试已有副作用的请求
        h2UpgradePolicy: DEFAULT

    # 负载均衡策略
    loadBalancer:
      simple: LEAST_REQUEST
---
# 针对特定端口的差异化策略
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: order-service-resilience
  namespace: production
spec:
  host: order-service.production.svc.cluster.local
  trafficPolicy:
    portLevelSettings:
    - port:
        number: 8080
      connectionPool:
        http:
          http2MaxRequests: 500
          maxRequestsPerConnection: 50
      outlierDetection:
        consecutive5xxErrors: 3
        interval: 5s
        baseEjectionTime: 60s
        maxEjectionPercent: 30
    - port:
        number: 9090
      connectionPool:
        http:
          http2MaxRequests: 100
      outlierDetection:
        consecutive5xxErrors: 10
        interval: 30s
        baseEjectionTime: 10s
```

### Istio VirtualService：Retry + Timeout + Fallback

```yaml
# 🟡 中风险：修改路由规则影响请求处理行为
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: checkout-service-routes
  namespace: production
spec:
  hosts:
  - checkout-service.production.svc.cluster.local
  http:
  # 主路由：带重试和超时
  - name: primary-route
    match:
    - uri:
        prefix: /api/v1/checkout
    route:
    - destination:
        host: checkout-service.production.svc.cluster.local
        port:
          number: 8080
        subset: v2
      weight: 90
    - destination:
        host: checkout-service.production.svc.cluster.local
        port:
          number: 8080
        subset: v1
      weight: 10
    # 超时配置
    timeout: 10s
    # 重试策略
    retries:
      attempts: 3
      perTryTimeout: 3s
      retryOn: "5xx,reset,connect-failure,retriable-4xx"
      retryRemoteLocalities: true
    # 故障注入（混沌测试用，生产环境禁用）
    # fault:
    #   abort:
    #     percentage:
    #       value: 0
    #     httpStatus: 503

  # 降级路由：当主服务不可用时返回缓存/默认响应
  - name: fallback-route
    match:
    - uri:
        prefix: /api/v1/checkout
      headers:
        x-fallback-enabled:
          exact: "true"
    directResponse:
      status: 200
      body:
        string: '{"status":"degraded","message":"Service temporarily unavailable, please retry later","fallback":true}'
---
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: inventory-service-routes
  namespace: production
spec:
  hosts:
  - inventory-service.production.svc.cluster.local
  http:
  - route:
    - destination:
        host: inventory-service.production.svc.cluster.local
        port:
          number: 8080
    timeout: 5s
    retries:
      attempts: 2
      perTryTimeout: 2s
      retryOn: "5xx,reset,connect-failure"
    # 镜像流量（用于新版本验证，不影响主流量）
    mirror:
      host: inventory-service.production.svc.cluster.local
      subset: v2-canary
    mirrorPercentage:
      value: 5.0
```

### Envoy Rate Limiting（全局限流）

```yaml
# 🟡 中风险：限流配置不当可能拒绝合法流量
apiVersion: networking.istio.io/v1alpha3
kind: EnvoyFilter
metadata:
  name: global-rate-limit
  namespace: istio-system
spec:
  workloadSelector:
    labels:
      app: istio-ingressgateway
  configPatches:
  - applyTo: HTTP_FILTER
    match:
      context: GATEWAY
      listener:
        filterChain:
          filter:
            name: envoy.filters.network.http_connection_manager
            subFilter:
              name: envoy.filters.http.router
    patch:
      operation: INSERT_BEFORE
      value:
        name: envoy.filters.http.local_ratelimit
        typed_config:
          "@type": type.googleapis.com/udpa.type.v1.TypedStruct
          type_url: type.googleapis.com/envoy.extensions.filters.http.local_ratelimit.v3.LocalRateLimit
          value:
            stat_prefix: http_local_rate_limiter
            token_bucket:
              # 每秒允许的请求数
              max_tokens: 10000
              tokens_per_fill: 10000
              fill_interval: 1s
            filter_enabled:
              runtime_key: local_rate_limit_enabled
              default_value:
                numerator: 100
                denominator: HUNDRED
            filter_enforced:
              runtime_key: local_rate_limit_enforced
              default_value:
                numerator: 100
                denominator: HUNDRED
            response_headers_to_add:
            - append_action: OVERWRITE_IF_EXISTS_OR_ADD
              header:
                key: x-local-rate-limit
                value: "true"
            status:
              code: TooManyRequests
---
# 按路由的差异化限流
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: api-gateway-ratelimit
  namespace: production
spec:
  hosts:
  - api.company.com
  http:
  - name: premium-tier
    match:
    - uri:
        prefix: /api/v1
      headers:
        x-tier:
          exact: premium
    route:
    - destination:
        host: api-backend.production.svc.cluster.local
    # Premium 用户不限流（通过 header 标记跳过限流）

  - name: standard-tier
    match:
    - uri:
        prefix: /api/v1
    route:
    - destination:
        host: api-backend.production.svc.cluster.local
    # 标准用户限流通过 EnvoyFilter 全局生效
```

### 应用层弹性模式（Spring Cloud / Go SDK）

```yaml
# 🟢 低风险：应用配置，不影响集群基础设施
apiVersion: v1
kind: ConfigMap
metadata:
  name: resilience-config
  namespace: production
data:
  application-resilience.yaml: |
    # Spring Cloud Circuit Breaker 配置示例
    resilience4j:
      circuitbreaker:
        configs:
          default:
            slidingWindowType: COUNT_BASED
            slidingWindowSize: 100
            minimumNumberOfCalls: 20
            failureRateThreshold: 50
            slowCallRateThreshold: 80
            slowCallDurationThreshold: 2s
            waitDurationInOpenState: 30s
            permittedNumberOfCallsInHalfOpenState: 10
            automaticTransitionFromOpenToHalfOpenEnabled: true
            recordExceptions:
            - java.io.IOException
            - java.util.concurrent.TimeoutException
            - org.springframework.web.client.HttpServerErrorException
          payment-service:
            slidingWindowSize: 50
            failureRateThreshold: 30
            waitDurationInOpenState: 60s
        instances:
          paymentService:
            baseConfig: payment-service
          inventoryService:
            baseConfig: default

      retry:
        configs:
          default:
            maxAttempts: 3
            waitDuration: 500ms
            enableExponentialBackoff: true
            exponentialBackoffMultiplier: 2
            enableRandomizedWait: true
            randomizedWaitFactor: 0.5
            retryExceptions:
            - java.io.IOException
            - java.net.SocketTimeoutException
            ignoreExceptions:
            - com.company.exception.BusinessException
            - com.company.exception.ValidationException

      bulkhead:
        configs:
          default:
            maxConcurrentCalls: 50
            maxWaitDuration: 100ms
          payment:
            maxConcurrentCalls: 20
            maxWaitDuration: 500ms

      timelimiter:
        configs:
          default:
            timeoutDuration: 5s
            cancelRunningFuture: true
          payment:
            timeoutDuration: 10s

      ratelimiter:
        configs:
          default:
            limitForPeriod: 100
            limitRefreshPeriod: 1s
            timeoutDuration: 0s
```

### Kubernetes 资源级 Bulkhead

```yaml
# 🟡 中风险：ResourceQuota 限制可能影响新 Pod 创建
apiVersion: v1
kind: ResourceQuota
metadata:
  name: payment-team-quota
  namespace: production
spec:
  hard:
    # CPU 和内存总量限制（团队级舱壁）
    requests.cpu: "20"
    requests.memory: 40Gi
    limits.cpu: "40"
    limits.memory: 80Gi
    # Pod 数量限制
    pods: "50"
    # PVC 数量限制
    persistentvolumeclaims: "10"
---
# 优先级类：确保关键服务在资源争抢时优先调度
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: critical-service
value: 1000000
globalDefault: false
preemptionPolicy: PreemptLowerPriority
description: "Critical production services - highest priority"
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: standard-service
value: 100000
globalDefault: false
description: "Standard production services"
```

## 运维操作

### 弹性策略验证

```bash
# 🟢 低风险：只读验证
# 查看 Istio DestinationRule 生效状态
kubectl get destinationrule -n production -o yaml | grep -A20 "outlierDetection"

# 查看 Envoy 集群的熔断状态（通过 istioctl）
istioctl proxy-config cluster payment-service-xxx.production -o json | \
  jq '.[] | select(.name | contains("payment")) | .outlier_detection'

# 查看当前被熔断的实例
istioctl proxy-config endpoints payment-service-xxx.production -o json | \
  jq '.[] | .lb_endpoints[] | select(.health_status != "HEALTHY")'

# 查看重试统计
kubectl exec -n production deployment/checkout-service -- \
  curl -s localhost:15000/stats | grep "retry\|upstream_rq"
```

### 熔断器状态监控

```bash
# 🟢 低风险：只读监控
# 通过 Prometheus 查询熔断指标
curl -s 'http://prometheus-server.monitoring.svc:9090/api/v1/query' \
  --data-urlencode 'query=envoy_cluster_outlier_detection_ejections_active{cluster_name=~".*payment.*"}' | \
  jq '.data.result[] | {cluster: .metric.cluster_name, ejected: .value[1]}'

# 查看熔断触发事件
curl -s 'http://prometheus-server.monitoring.svc:9090/api/v1/query' \
  --data-urlencode 'query=increase(envoy_cluster_outlier_detection_ejections_consecutive_5xx[5m])' | \
  jq '.data.result[] | select(.value[1] | tonumber > 0)'

# 查看重试率
curl -s 'http://prometheus-server.monitoring.svc:9090/api/v1/query' \
  --data-urlencode 'query=sum(rate(envoy_cluster_retry[5m])) by (cluster_name)' | \
  jq '.data.result[] | {cluster: .metric.cluster_name, retry_rate: .value[1]}'
```

### 弹性策略调优

```bash
# 🟡 中风险：修改弹性策略影响服务行为
# 临时调整熔断阈值（故障期间放宽）
kubectl patch destinationrule payment-service-resilience -n production \
  --type='json' \
  -p='[{"op":"replace","path":"/spec/trafficPolicy/outlierDetection/consecutive5xxErrors","value":10}]'

# 临时禁用重试（排查重试风暴）
kubectl patch virtualservice checkout-service-routes -n production \
  --type='json' \
  -p='[{"op":"replace","path":"/spec/http/0/retries/attempts","value":1}]'

# 恢复默认配置
kubectl apply -f resilience-configs/payment-service-destinationrule.yaml
```

## 故障排查

### 熔断器误触发

```bash
# 🟢 低风险：只读诊断
# 1. 确认熔断是否真的触发
istioctl proxy-config cluster payment-service-xxx.production -o json | \
  jq '.[] | select(.name | contains("payment")) | {
    name: .name,
    outlier_detection: .outlier_detection,
    hosts_health: .load_assignment.endpoints[0].lb_endpoints | map(.health_status)
  }'

# 2. 查看被熔断实例的错误日志
kubectl logs -n production -l app=payment-service --tail=50 | grep -i "error\|5xx\|timeout"

# 3. 检查是否是网络问题导致误判
kubectl exec -n production deployment/checkout-service -- \
  curl -sv http://payment-service.production.svc:8080/healthz 2>&1 | tail -20

# 4. 查看 Envoy 访问日志中的响应码分布
kubectl logs -n production -l app=istio-ingressgateway --tail=200 | \
  awk '{print $NF}' | sort | uniq -c | sort -rn | head -10
```

### 重试风暴排查

```bash
# 🟢 低风险：只读诊断
# 检查重试放大效应
curl -s 'http://prometheus-server.monitoring.svc:9090/api/v1/query' \
  --data-urlencode 'query=sum(rate(envoy_cluster_upstream_rq_total[1m])) by (cluster_name)' | \
  jq '.data.result[] | {cluster: .metric.cluster_name, qps: .value[1]}'

# 对比重试前后的请求量
curl -s 'http://prometheus-server.monitoring.svc:9090/api/v1/query' \
  --data-urlencode 'query=sum(rate(envoy_cluster_upstream_rq_retry[1m])) / sum(rate(envoy_cluster_upstream_rq_total[1m]))' | \
  jq '.data.result[0].value[1]'

# 如果重试率 > 20%，说明存在重试风暴
```

### 限流误杀排查

```bash
# 🟢 低风险：只读诊断
# 查看被限流的请求
kubectl logs -n istio-system -l app=istio-ingressgateway --tail=100 | grep "429"

# 检查限流配置
kubectl get envoyfilter global-rate-limit -n istio-system -o yaml | grep -A10 "token_bucket"

# 查看当前 QPS 是否接近限流阈值
curl -s 'http://prometheus-server.monitoring.svc:9090/api/v1/query' \
  --data-urlencode 'query=sum(rate(istio_requests_total{destination_service_name="api-gateway"}[1m]))'
```

## 最佳实践

### 弹性策略设计原则

1. **超时预算递减**：调用链上游的超时必须大于下游。例如：Gateway 10s → Service A 8s → Service B 5s → DB 3s。

2. **重试必须有预算**：全局重试率不超过 20%，避免重试风暴。使用 Retry Budget 而非固定重试次数。

3. **熔断器参数基于 SLO**：熔断阈值应与 SLO 对齐。如果 SLO 是 99.9% 可用性，熔断阈值不应低于 99%。

4. **Bulkhead 按业务域隔离**：关键业务路径（支付、认证）使用独立的连接池和资源配额。

5. **Fallback 必须预先设计**：降级方案需要在设计阶段确定，而非故障时临时决定。

### 多层防御策略

| 层次 | 工具 | 职责 |
|------|------|------|
| L1 入口 | Istio IngressGateway + Rate Limit | 全局流量整形、DDoS 防护 |
| L2 服务间 | Istio DestinationRule | Circuit Breaker、连接池、重试 |
| L3 应用内 | Resilience4j / go-resilience | 细粒度熔断、Bulkhead、Fallback |
| L4 基础设施 | K8s ResourceQuota + PriorityClass | 资源隔离、优先级保障 |

### 混沌工程验证

弹性策略必须通过 [[可观测性/总览/14-chaos-engineering.md|混沌工程]] 验证：
- 注入 5xx 错误验证 Circuit Breaker 触发
- 注入延迟验证 Timeout 和 Fallback 生效
- 杀死 Pod 验证重试和负载均衡
- 注入网络分区验证 Bulkhead 隔离效果

## Related

- [[可靠性/SRE实践/05-error-budget-automation.md|错误预算自动化]]
- [[可观测性/总览/14-chaos-engineering.md|混沌工程]]
- [[可靠性/SRE实践/03-incident-command-system.md|事件指挥系统]]
- [[可观测性/告警/07-aiops-intelligent-alerting.md|AIOps 智能告警]]
- [[可靠性/SRE实践/09-multi-active-architecture.md|多活架构设计]]
- [[发布变更/变更管理/07-rollback-automation-patterns.md|回滚自动化模式]]
- [[可观测性/总览/01-observability-architecture-overview.md|可观测性架构总览]]
