# 06-APM应用性能监控

> **适用范围**: Kubernetes v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

## 📋 概述

应用性能监控(APM)是保障微服务架构稳定运行的关键工具。本文档详细介绍分布式追踪、性能指标收集和应用监控的最佳实践。

## 🎯 APM架构设计

### 核心组件架构

#### 1. OpenTelemetry采集层
```yaml
# OpenTelemetry Collector配置
apiVersion: opentelemetry.io/v1alpha1
kind: OpenTelemetryCollector
metadata:
  name: otel-collector
  namespace: observability
spec:
  config: |
    receivers:
      otlp:
        protocols:
          grpc:
          http:
      jaeger:
        protocols:
          thrift_http:
          grpc:
      zipkin:
      
    processors:
      batch:
      memory_limiter:
        check_interval: 1s
        limit_mib: 4000
        spike_limit_mib: 500
      attributes:
        actions:
          - key: environment
            value: production
            action: insert
      
    exporters:
      otlp/tempo:
        endpoint: tempo:4317
        tls:
          insecure: true
      prometheus:
        endpoint: "0.0.0.0:8889"
        namespace: otel
        const_labels:
          exporter: prometheus
      logging:
      
    service:
      pipelines:
        traces:
          receivers: [otlp, jaeger, zipkin]
          processors: [memory_limiter, batch, attributes]
          exporters: [otlp/tempo, logging]
        metrics:
          receivers: [otlp]
          processors: [memory_limiter, batch]
          exporters: [prometheus, logging]
```

#### 2. 应用埋点配置
```yaml
# Java应用OpenTelemetry配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-otel-config
  namespace: production
data:
  otel-agent-config.yaml: |
    extensions:
      health_check:
      pprof:
      zpages:
    
    receivers:
      otlp:
        protocols:
          grpc:
            endpoint: 0.0.0.0:4317
          http:
            endpoint: 0.0.0.0:4318
    
    processors:
      batch:
      memory_limiter:
        check_interval: 1s
        limit_mib: 100
    
    exporters:
      otlp:
        endpoint: otel-collector:4317
        tls:
          insecure: true
    
    service:
      extensions: [health_check, pprof, zpages]
      pipelines:
        traces:
          receivers: [otlp]
          processors: [batch, memory_limiter]
          exporters: [otlp]
```

## 📊 分布式追踪

### Jaeger追踪配置

#### 1. Jaeger Operator部署
```yaml
# Jaeger实例配置
apiVersion: jaegertracing.io/v1
kind: Jaeger
metadata:
  name: jaeger-all-in-one
spec:
  strategy: allInOne
  allInOne:
    image: jaegertracing/all-in-one:1.40
    options:
      log-level: debug
  storage:
    type: elasticsearch
    options:
      es:
        server-urls: http://elasticsearch:9200
  ingress:
    enabled: true
    annotations:
      kubernetes.io/ingress.class: nginx
      nginx.ingress.kubernetes.io/auth-type: basic
      nginx.ingress.kubernetes.io/auth-secret: jaeger-basic-auth
```

#### 2. 应用追踪埋点
```java
// Java应用追踪示例
@RestController
public class UserController {
    
    @Autowired
    private Tracer tracer;
    
    @GetMapping("/users/{id}")
    public ResponseEntity<User> getUser(@PathVariable String id) {
        Span span = tracer.buildSpan("get-user")
            .withTag("user.id", id)
            .withTag("http.method", "GET")
            .start();
        
        try (Scope scope = tracer.scopeManager().activate(span)) {
            // 业务逻辑
            User user = userService.findById(id);
            
            span.setTag("user.found", user != null);
            if (user != null) {
                span.setTag("user.email.domain", 
                    user.getEmail().split("@")[1]);
            }
            
            return ResponseEntity.ok(user);
        } catch (Exception e) {
            Tags.ERROR.set(span, true);
            span.log(Collections.singletonMap("event", "error"));
            span.log(Collections.singletonMap("error.object", e));
            throw e;
        } finally {
            span.finish();
        }
    }
}
```

### 追踪数据采样

#### 1. 智能采样策略
```yaml
# Tempo采样配置
apiVersion: tempo.grafana.com/v1alpha1
kind: TempoMonolithic
metadata:
  name: tempo
  namespace: observability
spec:
  storage:
    traces:
      backend: s3
      s3:
        bucket: tempo-traces
        endpoint: s3.amazonaws.com
  sampling:
    policies:
    - always_sample: {}
    - numeric_attribute:
        key: http.status_code
        min_value: 500
        max_value: 599
    - string_attribute:
        key: service.name
        values:
          - critical-service
          - payment-service
    - rate_limiting:
        spans_per_second: 10
```

#### 2. 追踪数据过滤
```yaml
# 追踪数据过滤配置
processors:
  filter/traces:
    error_mode: ignore
    traces:
      span:
      - name: health_check
      - name: readiness_probe
      - attributes["http.route"] == "/metrics"
      - attributes["http.route"] == "/health"
```

## 📈 性能指标监控

### 应用指标收集

#### 1. Micrometer集成
```yaml
# Spring Boot应用指标配置
management:
  endpoints:
    web:
      exposure:
        include: health,info,metrics,prometheus
  metrics:
    export:
      prometheus:
        enabled: true
    distribution:
      percentiles-histogram:
        http.server.requests: true
      slo:
        http.server.requests: 100ms, 200ms, 500ms
    tags:
      application: ${spring.application.name}
      environment: ${spring.profiles.active}
```

#### 2. 自定义业务指标
```java
// 自定义业务指标
@Component
public class BusinessMetrics {
    
    private final MeterRegistry meterRegistry;
    private final Counter orderCounter;
    private final Timer orderProcessingTimer;
    private final Gauge activeUsersGauge;
    
    public BusinessMetrics(MeterRegistry meterRegistry) {
        this.meterRegistry = meterRegistry;
        
        // 订单计数器
        orderCounter = Counter.builder("business.orders.total")
            .description("Total number of orders")
            .tags("type", "ecommerce")
            .register(meterRegistry);
            
        // 订单处理耗时
        orderProcessingTimer = Timer.builder("business.order.processing")
            .description("Order processing time")
            .publishPercentileHistogram()
            .sla(Duration.ofMillis(100), Duration.ofMillis(200))
            .register(meterRegistry);
            
        // 活跃用户数
        activeUsersGauge = Gauge.builder("business.users.active")
            .description("Number of active users")
            .register(meterRegistry, this, BusinessMetrics::getActiveUserCount);
    }
    
    public void recordOrder(String orderType) {
        orderCounter.increment();
        meterRegistry.counter("business.orders.by_type", "type", orderType).increment();
    }
    
    public Sample startOrderProcessing() {
        return Timer.start(meterRegistry);
    }
    
    public void recordOrderProcessing(Sample sample) {
        sample.stop(orderProcessingTimer);
    }
}
```

### 数据库性能监控

#### 1. 数据库连接池监控
```yaml
# HikariCP监控配置
spring:
  datasource:
    hikari:
      pool-name: app-pool
      register-mbeans: true
      metrics-tracker-factory: com.zaxxer.hikari.metrics.prometheus.PrometheusMetricsTrackerFactory
      
management:
  metrics:
    export:
      prometheus:
        descriptions: true
```

#### 2. SQL执行监控
```java
// SQL执行监控切面
@Aspect
@Component
public class SqlMonitoringAspect {
    
    private final Timer sqlTimer;
    private final Counter sqlErrorCounter;
    
    public SqlMonitoringAspect(MeterRegistry meterRegistry) {
        sqlTimer = Timer.builder("database.sql.execution")
            .description("SQL execution time")
            .publishPercentileHistogram()
            .register(meterRegistry);
            
        sqlErrorCounter = Counter.builder("database.sql.errors")
            .description("SQL execution errors")
            .register(meterRegistry);
    }
    
    @Around("@annotation(org.springframework.transaction.annotation.Transactional)")
    public Object monitorSqlExecution(ProceedingJoinPoint joinPoint) throws Throwable {
        Timer.Sample sample = Timer.start(sqlTimer);
        
        try {
            return joinPoint.proceed();
        } catch (Exception e) {
            sqlErrorCounter.increment();
            throw e;
        } finally {
            sample.stop(sqlTimer);
        }
    }
}
```

## 🔍 异常监控告警

### 错误追踪配置

#### 1. Sentry集成
```yaml
# Sentry配置
sentry:
  dsn: ${SENTRY_DSN}
  environment: ${SPRING_PROFILES_ACTIVE}
  release: ${APP_VERSION}
  traces-sample-rate: 1.0
  enable-tracing: true
  logging:
    minimum-breadcrumb-level: INFO
    minimum-event-level: ERROR
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: sentry-relay
  namespace: observability
spec:
  replicas: 2
  selector:
    matchLabels:
      app: sentry-relay
  template:
    metadata:
      labels:
        app: sentry-relay
    spec:
      containers:
      - name: relay
        image: getsentry/relay:23.1.1
        ports:
        - containerPort: 3000
        env:
        - name: SENTRY_RELAY_MODE
          value: "managed"
        - name: SENTRY_RELAY_UPSTREAM
          value: "https://sentry.io/"
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 512Mi
```

#### 2. 异常指标告警
```yaml
# 异常监控告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: application-error-alerts
  namespace: monitoring
spec:
  groups:
  - name: application.rules
    rules:
    - alert: HighErrorRate
      expr: sum(rate(http_server_requests_seconds_count{status=~"5.."}[5m])) by (job) / sum(rate(http_server_requests_seconds_count[5m])) by (job) > 0.05
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High error rate detected in {{ $labels.job }}"
        
    - alert: DatabaseConnectionErrors
      expr: rate(database_connection_errors_total[5m]) > 1
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Database connection errors in {{ $labels.job }}"
        
    - alert: SlowAPIResponse
      expr: histogram_quantile(0.95, rate(http_server_requests_seconds_bucket{uri!~"/health|/metrics"}[5m])) > 2
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Slow API response time in {{ $labels.job }}"
```

## 🎨 可视化展示

### Grafana仪表板

#### 1. 应用性能总览
```json
{
  "dashboard": {
    "title": "Application Performance Overview",
    "panels": [
      {
        "title": "Request Rate and Error Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(http_server_requests_seconds_count[5m])) by (job)",
            "legendFormat": "{{job}} - Total"
          },
          {
            "expr": "sum(rate(http_server_requests_seconds_count{status=~\"5..\"}[5m])) by (job)",
            "legendFormat": "{{job}} - Errors"
          }
        ]
      },
      {
        "title": "Response Time Distribution",
        "type": "heatmap",
        "targets": [
          {
            "expr": "histogram_quantile(0.5, rate(http_server_requests_seconds_bucket[5m]))",
            "legendFormat": "p50"
          },
          {
            "expr": "histogram_quantile(0.95, rate(http_server_requests_seconds_bucket[5m]))",
            "legendFormat": "p95"
          },
          {
            "expr": "histogram_quantile(0.99, rate(http_server_requests_seconds_bucket[5m]))",
            "legendFormat": "p99"
          }
        ]
      },
      {
        "title": "Database Connection Pool",
        "type": "graph",
        "targets": [
          {
            "expr": "hikaricp_connections_active",
            "legendFormat": "Active Connections"
          },
          {
            "expr": "hikaricp_connections_idle",
            "legendFormat": "Idle Connections"
          },
          {
            "expr": "hikaricp_connections_pending",
            "legendFormat": "Pending Connections"
          }
        ]
      }
    ]
  }
}
```

#### 2. 业务指标仪表板
```json
{
  "dashboard": {
    "title": "Business Metrics Dashboard",
    "panels": [
      {
        "title": "Order Processing Metrics",
        "type": "graph",
        "targets": [
          {
            "expr": "business_orders_total",
            "legendFormat": "Total Orders"
          },
          {
            "expr": "rate(business_orders_total[5m])",
            "legendFormat": "Orders per Second"
          }
        ]
      },
      {
        "title": "User Activity",
        "type": "stat",
        "targets": [
          {
            "expr": "business_users_active",
            "legendFormat": "Active Users"
          },
          {
            "expr": "increase(business_user_sessions_total[1h])",
            "legendFormat": "Sessions (Last Hour)"
          }
        ]
      }
    ]
  }
}
```

## 🔧 性能调优

### 应用性能优化

#### 1. JVM性能监控
```yaml
# JVM监控配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: jvm-monitoring
  namespace: production
data:
  jvm-exporter.yaml: |
    lowercaseOutputName: true
    lowercaseOutputLabelNames: true
    rules:
    - pattern: 'java.lang<type=OperatingSystem><>(FreePhysicalMemorySize|TotalPhysicalMemorySize|FreeSwapSpaceSize|TotalSwapSpaceSize|SystemCpuLoad|ProcessCpuLoad|OpenFileDescriptorCount|MaxFileDescriptorCount)'
      name: os_$1
      type: GAUGE
      attrNameSnakeCase: true
    - pattern: 'java.lang<type=Threading><>(TotalStartedThreadCount|ThreadCount)'
      name: jvm_threads_$1
      type: GAUGE
      attrNameSnakeCase: true
```

#### 2. 缓存性能监控
```java
// Redis缓存监控
@Configuration
public class CacheMonitoringConfig {
    
    @Bean
    public CacheMetricsRegistrar cacheMetricsRegistrar(
            MeterRegistry meterRegistry,
            CacheManager cacheManager) {
        
        return new CacheMetricsRegistrar(meterRegistry, 
            Collections.singletonList(cacheManager));
    }
    
    @EventListener
    public void handleCacheStats(CacheStatisticsEvent event) {
        Cache cache = event.getCache();
        String cacheName = cache.getName();
        
        // 记录缓存命中率
        double hitRatio = (double) event.getHits() / 
            (event.getHits() + event.getMisses());
            
        meterRegistry.gauge("cache.hit.ratio", 
            Tags.of("cache", cacheName), hitRatio);
    }
}
```

## 🛡️ 安全与合规

### 数据隐私保护

#### 1. 敏感信息脱敏
```java
// 追踪数据脱敏
@Component
public class TraceDataSanitizer {
    
    private static final Pattern EMAIL_PATTERN = 
        Pattern.compile("\\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Z|a-z]{2,}\\b");
    
    private static final Pattern PHONE_PATTERN = 
        Pattern.compile("\\b\\d{3}-\\d{3}-\\d{4}\\b");
    
    public void sanitizeSpanAttributes(Span span) {
        Map<String, Object> tags = span.tags();
        
        // 脱敏邮箱地址
        tags.replaceAll((key, value) -> {
            if (value instanceof String) {
                String strValue = (String) value;
                strValue = EMAIL_PATTERN.matcher(strValue).replaceAll("[EMAIL]");
                strValue = PHONE_PATTERN.matcher(strValue).replaceAll("[PHONE]");
                return strValue;
            }
            return value;
        });
    }
}
```

#### 2. 访问控制配置
```yaml
# APM工具访问控制
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: apm-access-control
  namespace: observability
spec:
  podSelector:
    matchLabels:
      app: jaeger
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: monitoring
    - podSelector:
        matchLabels:
          role: sre
    ports:
    - protocol: TCP
      port: 16686
```

## 🔧 实施检查清单

### APM平台建设
- [ ] 选择合适的APM工具链(OpenTelemetry/Jaeger/Tempo)
- [ ] 部署分布式追踪基础设施
- [ ] 集成应用性能指标收集
- [ ] 配置异常监控和告警机制
- [ ] 建立可视化监控仪表板
- [ ] 实施数据采样和存储策略

### 应用集成
- [ ] 在关键应用中添加追踪埋点
- [ ] 配置业务指标收集
- [ ] 实施数据库和缓存性能监控
- [ ] 集成错误追踪和报告工具
- [ ] 配置安全和隐私保护措施
- [ ] 建立性能基线和阈值

### 运营维护
- [ ] 制定APM平台运维规范
- [ ] 建立性能问题排查流程
- [ ] 定期审查和优化监控配置
- [ ] 维护监控文档和最佳实践
- [ ] 持续改进监控覆盖范围
- [ ] 建立性能优化反馈机制

---

*本文档为企业级APM系统建设和应用性能监控提供全面的技术指导*