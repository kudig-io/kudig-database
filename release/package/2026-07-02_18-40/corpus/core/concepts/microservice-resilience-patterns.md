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

## Related

- [[istio]] — Istio
- [[envoy]] — Envoy
- [[concepts/production-operations-best-practices.md|production-operations-best-practices]] — [[entities/k8s-production-operations.md|Production Operations]]ns Best Practices|Production Operations Best Practices]]佳实践字典|Operations Best Practices]]
- [[concepts/service-mesh-architecture.md|service-mesh-architecture]] — [[concepts/service-mesh-architecture.md|Service Mesh Architecture]]
- [[concepts/service-mesh-architecture.md|Service Mesh Architecture]]
- [[concepts/production-operations-best-practices.md|Production Operations Best Practices]]
- [[istio|Istio]]

- 09-microservice-resilience-patterns

<!-- risk-assessed -->
