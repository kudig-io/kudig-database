---
title: Microservice Resilience Patterns
description: Microservice Resilience Patterns — Kubernetes 生产运维知识库
summary: Microservice Resilience Patterns — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- microservices
- resilience
- circuit-breaker
- retry
- timeout
- rate-limiting
- istio
- envoy
- gateway
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Microservice Resilience Patterns 是什么
- 如何 Microservice Resilience Patterns
trigger_keywords:
- Microservice
- Resilience
- Patterns
prerequisites:
- kubectl-basics
- service-mesh-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Microservice Resilience Patterns

## Five Core Patterns

### 1. Circuit Breaker

Prevents cascading failures by stopping calls to failing services. States: Closed (normal) -> Open (failures exceed threshold, all calls rejected) -> Half-Open (trial requests to test recovery).

- **Mesh layer**: Istio OutlierDetection (Envoy-level, monitors response codes/latency)
- **App layer**: Resilience4j CircuitBreaker (application-level, business-aware)

### 2. Retry with Backoff

Automatically retries transient failures using exponential backoff with jitter to avoid retry storms.

- **Mesh layer**: Istio VirtualService retries (configure attempts, per-try timeout, retryable conditions)
- **App layer**: Resilience4j Retry (constant or exponential backoff, custom retry conditions)
- **Key rule**: Configure retries at only ONE layer to avoid double-retry amplification. Retry only idempotent GET requests.

### 3. Layered Timeout

Hierarchical timeout design: outer timeout > inner timeout at each layer.

- Gateway timeout (5s) > Mesh timeout (3s) > Application timeout (2s) > Database timeout (1s)
- Mesh layer: Istio VirtualService timeout
- App layer: Resilience4j TimeLimiter, HTTP client timeout

### 4. Bulkhead Isolation

Limits resource consumption so one [[Service|service]] failure does not exhaust all resources.

- **Mesh layer**: Istio DestinationRule connectionPool (max connections, max pending requests)
- **App layer**: Resilience4j Bulkhead (semaphore or thread pool isolation)

### 5. Rate Limiting

Protects downstream services from traffic overload.

- **Three layers**: Gateway (global rate limit) -> Mesh (per-service rate limit) -> Application (per-endpoint rate limit)
- Mesh: Istio EnvoyFilter with rate-limit service
- App: Resilience4j RateLimiter

## Coordination Between Layers

Mesh and application resilience features must be coordinated:
- Mesh handles mTLS, connection pools, node-level circuit breaking, global retries (GET only)
- App handles application-level circuit breaking, method-level timeouts, thread isolation (Bulkhead), business rate limiting, fallback methods
- Key principle: configure retries at only one layer; follow outer-greater-than-inner timeout hierarchy

## Production Configuration Example

Istio circuit breaker (OutlierDetection):
- consecutive5xxErrors: 5 (open after 5 consecutive 5xx errors)
- interval: 30s (check interval)
- baseEjectionTime: 30s (minimum ejection duration)
- maxEjectionPercent: 50 (never eject more than 50% of endpoints)

## 源码实现分析

### Envoy 熔断器实现

```cpp
// envoy/source/common/upstream/outlier_detection_impl.cc
void DetectorImpl::onConsecutiveError(Http::Code code) {
    // 每个上游主机维护独立的错误计数器
    host->outlierDetector().incConsecutiveError();
    
    if (host->outlierDetector().consecutiveError() >= threshold_) {
        // 触发熔断: 将主机从负载均衡池中移除
        host->healthFlag(HealthFlag::FAILED_OUTLIER_CHECK);
        // baseEjectionTime 后尝试半开（Half-Open）
        timer_.enableTimer(baseEjectionTime_);
    }
}
// 半开状态: 允许少量请求通过，成功则恢复，失败则继续熔断
```

### 韧性模式状态机

```
熔断器状态机:
┌────────┐  连续失败 ≥ N  ┌────────┐
│ Closed │───────────────►│  Open  │
│(正常)  │                │(熔断)  │
└────┬───┘                └────┬───┘
     │                         │ 等待超时
     │ 试探成功                ▼
     │                    ┌──────────┐
     └───────────────────│ Half-Open│
                         │(试探)    │
                         └──────────┘
                         试探失败 → 回到 Open
```

## 使用场景

### 场景一：Istio 完整韧性配置

```yaml
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: payment-service
spec:
  host: payment.default.svc.cluster.local
  trafficPolicy:
    connectionPool:           # Bulkhead
      tcp:
        maxConnections: 100
      http:
        h2UpgradePolicy: DEFAULT
        http1MaxPendingRequests: 50
        http2MaxRequests: 200
    outlierDetection:         # Circuit Breaker
      consecutive5xxErrors: 3
      interval: 10s
      baseEjectionTime: 30s
      maxEjectionPercent: 50
---
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: payment-routes
spec:
  hosts: [payment.default.svc.cluster.local]
  http:
  - route:
    - destination:
        host: payment.default.svc.cluster.local
    timeout: 3s              # Layered Timeout
    retries:
      attempts: 3            # Retry with Backoff
      perTryTimeout: 1s
      retryOn: 5xx,reset,connect-failure
```

### 场景二：应用层 Resilience4j

```java
// Spring Boot + Resilience4j
@CircuitBreaker(name = "paymentService", fallbackMethod = "fallback")
@Retry(name = "paymentService")
@TimeLimiter(name = "paymentService")
public CompletableFuture<Payment> processPayment(Order order) {
    return CompletableFuture.supplyAsync(() -> 
        paymentClient.charge(order));
}

// 配置: application.yml
// resilience4j.circuitbreaker.instances.paymentService:
//   failureRateThreshold: 50
//   waitDurationInOpenState: 30s
//   slidingWindowSize: 10
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 重试次数越多越好 | 重试放大流量（retry storm），只重试幂等 GET，且只在一层配置 |
| 熔断后所有请求失败 | 熔断是快速失败（fail-fast），避免级联故障，配合 fallback 降级 |
| 超时设置越短越好 | 超时应分层（外>内），太短导致正常请求被截断 |
| Mesh 层和应用层都配重试 | 双层重试导致指数级放大（3×3=9次），只在一层配置 |
| 限流只保护服务端 | 限流也保护客户端（避免队列积压），应在多层配置 |
| Bulkhead 只是线程池 | 还包括连接池、信号量、资源配额等多维度隔离 |

## 面试要点

1. **熔断器三种状态如何转换？** — Closed（正常，统计失败率）→ 失败率超阈值 → Open（熔断，快速失败）→ 等待超时 → Half-Open（允许少量试探）→ 成功则 Closed，失败则 Open。核心目的：防止级联故障。

2. **Mesh 层与应用层韧性如何分工？** — Mesh（Istio/Envoy）：mTLS、连接池、节点级熔断、全局重试（仅 GET）；应用（Resilience4j）：业务级熔断、方法级超时、线程隔离、业务降级、自定义重试条件。原则：重试只配一层，超时外>内。

3. **如何避免 Retry Storm？** — 指数退避 + 随机抨动（jitter）；只重试幂等操作；限制重试次数（≤3）；只在一层配置；配合熔断器（失败率高时停止重试）；服务端返回 429 时客户端应退避。

4. **生产环境韧性设计检查清单？** — 每个服务调用都有超时；重试只用于幂等 GET；熔断器配置 fallback；连接池有上限；多层限流（网关+Mesh+应用）；健康检查 + 就绪探针；优雅关闭（drain 存量请求）。

## Related

- [[istio]] — Istio
- [[envoy]] — Envoy
- [[概念/production-operations-best-practices.md|production-operations-best-practices]] — [[实体/k8s-production-operations.md|Production Operations]]ns Best Practices|Production Operations Best Practices]]佳实践字典|Operations Best Practices]]
- [[概念/service-mesh-architecture.md|service-mesh-architecture]] — [[概念/service-mesh-architecture.md|Service Mesh Architecture]]
- [[概念/service-mesh-architecture.md|Service Mesh Architecture]]
- [[概念/production-operations-best-practices.md|Production Operations Best Practices]]
- [[istio|Istio]]

- 09-microservice-resilience-patterns

<!-- risk-assessed -->
