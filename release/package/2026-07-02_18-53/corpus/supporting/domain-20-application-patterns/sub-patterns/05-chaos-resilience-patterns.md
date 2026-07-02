---
title: 弹性与混沌模式
description: '微服务弹性设计：熔断、限流、重试、超时、Bulkhead隔离与混沌工程实践'
summary: '微服务弹性设计：熔断、限流、重试、超时、Bulkhead隔离与混沌工程实践'
category: application-patterns
tags:
- resilience
- circuit-breaker
- rate-limit
- retry
- bulkhead
- chaos-engineering
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 所有工程师
- 架构师
- SRE
estimated_read_time: 15min
intent_queries:
- 弹性模式 是什么
- 如何 实现熔断限流
trigger_keywords:
- 熔断
- 限流
- 重试
- 超时
- Bulkhead
- 混沌工程
prerequisites:
- kubectl-basics
- microservice-basics
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


# 弹性与混沌模式

## 1. 概述

弹性模式使分布式系统在部分组件故障时仍能提供降级服务。本文档覆盖六大核心弹性模式的实现方案，以及通过混沌工程验证系统韧性的实践方法。

## 2. 弹性模式全景

```
弹性模式层级:

Level 1: 超时 (Timeout)
  → 最基本的防护，防止无限等待

Level 2: 重试 (Retry)
  → 处理瞬时故障，指数退避

Level 3: 限流 (Rate Limiting)
  → 保护系统不被过载

Level 4: 熔断 (Circuit Breaker)
  → 快速失败，防止级联故障

Level 5: Bulkhead (隔舱)
  → 故障隔离，防止资源耗尽

Level 6: 降级 (Fallback)
  → 提供降级服务，保证核心功能

组合策略:
  Timeout → Retry → Circuit Breaker → Bulkhead → Fallback
```

## 3. 熔断模式 (Circuit Breaker)

### 3.1 状态机

```
熔断器状态机:

CLOSED (正常)
  │
  │ 失败率 > 阈值
  ▼
OPEN (熔断)
  │
  │ 超时窗口到期
  ▼
HALF_OPEN (半开)
  │
  ├── 测试请求成功 → CLOSED
  └── 测试请求失败 → OPEN

参数配置:
  failure_rate_threshold: 50%     # 失败率阈值
  slow_call_rate_threshold: 80%   # 慢调用率阈值
  slow_call_duration: 2s          # 慢调用定义
  wait_duration_in_open: 30s      # 熔断持续时间
  sliding_window_size: 100        # 滑动窗口大小
  minimum_number_of_calls: 10     # 最小调用数
```

### 3.2 Resilience4j 实现

```java
// Resilience4j 熔断器配置
CircuitBreakerConfig config = CircuitBreakerConfig.custom()
    .failureRateThreshold(50)
    .slowCallRateThreshold(80)
    .slowCallDurationThreshold(Duration.ofSeconds(2))
    .waitDurationInOpenState(Duration.ofSeconds(30))
    .slidingWindowType(SlidingWindowType.COUNT_BASED)
    .slidingWindowSize(100)
    .minimumNumberOfCalls(10)
    .permittedNumberOfCallsInHalfOpenState(5)
    .recordExceptions(IOException.class, TimeoutException.class)
    .ignoreExceptions(BusinessException.class)
    .build();

CircuitBreaker circuitBreaker = CircuitBreaker.of("userService", config);

// 使用熔断器装饰调用
Supplier<User> decoratedSupplier = CircuitBreaker
    .decorateSupplier(circuitBreaker, () -> userService.getUser(userId));

// 监听熔断器事件
circuitBreaker.getEventPublisher()
    .onStateTransition(event -> log.info("State transition: {}", event))
    .onError(event -> log.error("Error: {}", event))
    .onSuccess(event -> log.debug("Success: {}", event));
```

### 3.3 Istio 熔断配置

```yaml
# Istio DestinationRule 熔断
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: user-service-circuit-breaker
spec:
  host: user-service
  trafficPolicy:
    connectionPool:
      tcp:
        maxConnections: 100
      http:
        h2UpgradePolicy: DEFAULT
        http1MaxPendingRequests: 100
        http2MaxRequests: 1000
        maxRequestsPerConnection: 10
        maxRetries: 3
    outlierDetection:
      consecutive5xxErrors: 5
      interval: 10s
      baseEjectionTime: 30s
      maxEjectionPercent: 50
      minHealthPercent: 30
```

## 4. 限流模式 (Rate Limiting)

### 4.1 限流算法对比

| 算法 | 特点 | 适用场景 | 实现复杂度 |
|------|------|---------|-----------|
| **固定窗口** | 简单，有边界突发 | 简单场景 | 低 |
| **滑动窗口** | 平滑，无边界突发 | 通用场景 | 中 |
| **令牌桶** | 允许突发，平均速率 | API 限流 | 中 |
| **漏桶** | 严格平滑 | 流量整形 | 低 |
| **分布式限流** | 全局限流 | 微服务架构 | 高 |

### 4.2 Guava RateLimiter 实现

```java
// 单机限流
RateLimiter limiter = RateLimiter.create(100.0); // 100 QPS

public Response handleRequest(Request req) {
    if (!limiter.tryAcquire(100, TimeUnit.MILLISECONDS)) {
        return Response.status(429)
            .header("Retry-After", "1")
            .body("Rate limit exceeded");
    }
    return processRequest(req);
}
```

### 4.3 Redis 分布式限流

```go
// Redis 滑动窗口限流
func RateLimit(ctx context.Context, redis *redis.Client, key string, limit int, window time.Duration) (bool, error) {
    now := time.Now().UnixMilli()
    windowStart := now - window.Milliseconds()

    pipe := redis.Pipeline()
    // 移除窗口外的记录
    pipe.ZRemRangeByScore(ctx, key, "0", strconv.FormatInt(windowStart, 10))
    // 添加当前请求
    pipe.ZAdd(ctx, key, &redis.Z{Score: float64(now), Member: now})
    // 统计窗口内请求数
    count := pipe.ZCard(ctx, key)
    // 设置过期时间
    pipe.Expire(ctx, key, window)
    _, err := pipe.Exec(ctx)
    if err != nil {
        return false, err
    }

    return count.Val() <= int64(limit), nil
}
```

### 4.4 Envoy 全局限流

```yaml
# Envoy Rate Limit Service
apiVersion: v1
kind: ConfigMap
metadata:
  name: envoy-ratelimit-config
data:
  config.yaml: |
    domain: api-gateway
    descriptors:
      - key: remote_address
        rate_limit:
          unit: second
          requests_per_unit: 100
      - key: remote_address
        value: premium_user
        rate_limit:
          unit: second
          requests_per_unit: 1000
      - key: path
        value: /api/v1/search
        rate_limit:
          unit: second
          requests_per_unit: 10
```

## 5. 重试模式 (Retry)

### 5.1 指数退避 + Jitter

```go
// 指数退避重试实现
type RetryConfig struct {
    MaxRetries      int
    BaseDelay       time.Duration
    MaxDelay        time.Duration
    BackoffFactor   float64
    JitterFraction  float64
    RetryableErrors []error
}

func RetryWithBackoff(ctx context.Context, config RetryConfig, fn func() error) error {
    var lastErr error
    for attempt := 0; attempt <= config.MaxRetries; attempt++ {
        err := fn()
        if err == nil {
            return nil
        }

        lastErr = err

        // 检查是否可重试
        if !isRetryable(err, config.RetryableErrors) {
            return err
        }

        // 检查上下文
        if ctx.Err() != nil {
            return ctx.Err()
        }

        // 计算退避时间
        delay := config.BaseDelay * time.Duration(math.Pow(config.BackoffFactor, float64(attempt)))
        if delay > config.MaxDelay {
            delay = config.MaxDelay
        }

        // 添加 Jitter
        jitter := time.Duration(float64(delay) * config.JitterFraction * (2*rand.Float64() - 1))
        delay += jitter

        select {
        case <-ctx.Done():
            return ctx.Err()
        case <-time.After(delay):
        }
    }
    return fmt.Errorf("max retries exceeded: %w", lastErr)
}

// 使用示例
config := RetryConfig{
    MaxRetries:     3,
    BaseDelay:      100 * time.Millisecond,
    MaxDelay:       5 * time.Second,
    BackoffFactor:  2.0,
    JitterFraction: 0.3,
}

err := RetryWithBackoff(ctx, config, func() error {
    return httpClient.Do(request)
})
```

### 5.2 重试策略配置

```yaml
# Istio 重试策略
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: user-service-retry
spec:
  hosts:
    - user-service
  http:
    - route:
        - destination:
            host: user-service
      retries:
        attempts: 3
        perTryTimeout: 2s
        retryOn: "connect-failure,refused-stream,unavailable,cancelled,retriable-status-codes"
        retryRemoteLocalities: true
      timeout: 10s
```

## 6. 超时模式 (Timeout)

### 6.1 分级超时设计

```yaml
# 分级超时配置
timeout_levels:
  L1_快速查询:
    timeout: 100ms
    适用: 缓存查询、配置读取
    策略: 立即失败

  L2_标准查询:
    timeout: 1s
    适用: 数据库查询、本地服务调用
    策略: 重试1次

  L3_复杂查询:
    timeout: 5s
    适用: 搜索、聚合查询
    策略: 返回部分结果

  L4_异步处理:
    timeout: 30s
    适用: 批量操作、文件处理
    策略: 返回任务ID，异步查询结果

  L5_长时间任务:
    timeout: 300s
    适用: 数据导出、报表生成
    策略: 异步执行，回调通知
```

### 6.2 超时传播

```go
// 超时传播实现
func CallDownstream(ctx context.Context, req *Request) (*Response, error) {
    // 继承上游超时
    deadline, ok := ctx.Deadline()
    if !ok {
        // 无上游超时，设置默认超时
        var cancel context.CancelFunc
        ctx, cancel = context.WithTimeout(ctx, 5*time.Second)
        defer cancel()
    } else {
        // 预留 20% 时间给本服务处理
        remaining := time.Until(deadline)
        adjustedTimeout := time.Duration(float64(remaining) * 0.8)
        var cancel context.CancelFunc
        ctx, cancel = context.WithTimeout(ctx, adjustedTimeout)
        defer cancel()
    }

    return downstreamClient.Call(ctx, req)
}
```

## 7. Bulkhead 隔舱模式

### 7.1 线程池隔离

```java
// Resilience4j Bulkhead 配置
BulkheadConfig config = BulkheadConfig.custom()
    .maxConcurrentCalls(25)       // 最大并发数
    .maxWaitDuration(Duration.ofSeconds(5))  // 等待超时
    .build();

Bulkhead bulkhead = Bulkhead.of("paymentService", config);

// 使用隔舱装饰调用
Supplier<PaymentResult> decorated = Bulkhead
    .decorateSupplier(bulkhead, () -> paymentService.charge(amount));

// 线程池隔离配置
ThreadPoolBulkheadConfig threadPoolConfig = ThreadPoolBulkheadConfig.custom()
    .maxThreadPoolSize(10)
    .coreThreadPoolSize(5)
    .queueCapacity(20)
    .keepAliveDuration(Duration.ofSeconds(20))
    .build();
```

### 7.2 信号量隔离

```go
// Go 信号量隔离
type Bulkhead struct {
    sem     chan struct{}
    timeout time.Duration
}

func NewBulkhead(maxConcurrent int, timeout time.Duration) *Bulkhead {
    return &Bulkhead{
        sem:     make(chan struct{}, maxConcurrent),
        timeout: timeout,
    }
}

func (b *Bulkhead) Execute(fn func() error) error {
    select {
    case b.sem <- struct{}{}:
        defer func() { <-b.sem }()
        return fn()
    case <-time.After(b.timeout):
        return errors.New("bulkhead: max concurrent exceeded")
    }
}

// 使用示例
paymentBulkhead := NewBulkhead(25, 5*time.Second)
err := paymentBulkhead.Execute(func() error {
    return paymentService.Charge(amount)
})
```

## 8. 降级策略 (Fallback)

```go
// 降级策略实现
func GetUserWithFallback(ctx context.Context, userID string) (*User, error) {
    // 1. 尝试从主服务获取
    user, err := userService.Get(ctx, userID)
    if err == nil {
        return user, nil
    }

    // 2. 尝试从缓存获取
    user, err = cache.Get(ctx, "user:"+userID)
    if err == nil {
        log.Warn("serving from cache due to service error")
        return user, nil
    }

    // 3. 返回默认/降级数据
    return &User{
        ID:       userID,
        Name:     "Unknown User",
        Avatar:   "/assets/default-avatar.png",
        Degraded: true,
    }, nil
}
```

## 9. 混沌工程实践

### 9.1 Chaos Mesh 部署

```yaml
# Chaos Mesh 混沌实验
apiVersion: chaos-mesh.org/v1alpha1
kind: PodChaos
metadata:
  name: pod-failure-user-service
  namespace: chaos-testing
spec:
  action: pod-failure
  mode: one
  selector:
    namespaces:
      - production
    labelSelectors:
      app: user-service
  duration: "30s"
  scheduler:
    cron: "@every 1h"
---
apiVersion: chaos-mesh.org/v1alpha1
kind: NetworkChaos
metadata:
  name: network-delay-payment
  namespace: chaos-testing
spec:
  action: delay
  mode: one
  selector:
    namespaces:
      - production
    labelSelectors:
      app: payment-service
  delay:
    latency: "200ms"
    jitter: "50ms"
    correlation: "50"
  duration: "5m"
  scheduler:
    cron: "@every 2h"
```

### 9.2 Litmus Chaos 实验

```yaml
# Litmus 混沌实验
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: engine-nginx
  namespace: default
spec:
  engineState: active
  appinfo:
    appns: default
    applabel: app=nginx
    appkind: deployment
  chaosServiceAccount: litmus-admin
  experiments:
    - name: pod-delete
      spec:
        components:
          env:
            - name: TOTAL_CHAOS_DURATION
              value: "30"
            - name: CHAOS_INTERVAL
              value: "10"
            - name: FORCE
              value: "false"
```

### 9.3 混沌实验清单

```
混沌工程实验场景:

网络故障:
  □ Pod 间网络延迟 +200ms
  □ Pod 间网络丢包 10%
  □ DNS 解析失败
  □ 网络分区（Split Brain）

Pod 故障:
  □ 随机杀死 Pod
  □ Pod OOM
  □ CPU 压力 80%
  □ 内存压力 80%
  □ 磁盘 IO 压力

应用故障:
  □ 依赖服务不可用
  □ 依赖服务响应超时
  □ 数据库连接池耗尽
  □ 消息队列积压

基础设施:
  □ 节点宕机
  □ ETCD 延迟
  □ API Server 压力
  □ 存储延迟
```

## 10. 综合配置示例

```yaml
# Istio 完整弹性配置
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: payment-service-resilience
spec:
  host: payment-service
  trafficPolicy:
    connectionPool:
      tcp:
        maxConnections: 100
        connectTimeout: 1s
      http:
        http1MaxPendingRequests: 100
        http2MaxRequests: 1000
        maxRequestsPerConnection: 10
        maxRetries: 3
    outlierDetection:
      consecutive5xxErrors: 5
      interval: 10s
      baseEjectionTime: 30s
      maxEjectionPercent: 50
    loadBalancer:
      simple: LEAST_REQUEST
---
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: payment-service-vs
spec:
  hosts:
    - payment-service
  http:
    - route:
        - destination:
            host: payment-service
      timeout: 10s
      retries:
        attempts: 3
        perTryTimeout: 3s
        retryOn: "connect-failure,refused-stream,unavailable"
```

## Related

- [[domain-20-application-patterns/sub-patterns/04-sidecar-ambassador-patterns|Sidecar 与 Ambassador 模式]]
- domain-09-reliability-engineering/
- domain-10-troubleshooting-diagnostics/

## See Also

- Resilience4j 官方文档
- Istio 流量管理
- Chaos Mesh 使用指南


<!-- risk-assessed -->
