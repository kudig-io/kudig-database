---
title: OpenTelemetry分布式追踪与可观测性深度实践
description: '# OpenTelemetry分布式追踪与可观测性深度实践'
summary: '# OpenTelemetry分布式追踪与可观测性深度实践'
category: enterprise-monitoring-alerting
tags:
- k8s
- monitoring
- alerting
- prometheus
- grafana
- jaeger
- opa
- postgresql
- kafka
- elasticsearch
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 监控工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- OpenTelemetry分布式追踪与可观测性深度实践 是什么
- 如何 OpenTelemetry分布式追踪与可观测性深度实践
- Kubernetes 20 enterprise monitoring alerting 最佳实践
trigger_keywords:
- OpenTelemetry分布式追踪与可观测性深度实践
- enterprise
- monitoring
- alerting
prerequisites:
- kubectl-basics
- observability-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- policy-basics
- logging-basics
- tracing-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/promql.md
  label: '速查卡: promql'
---



# OpenTelemetry分布式追踪与可观测性深度实践

> **作者**: 可观测性架构专家 | **版本**: v1.0 | **更新时间**: 2026-02-07
> **适用场景**: 企业级分布式系统可观测性 | **复杂度**: ⭐⭐⭐⭐⭐

<!-- chunk: 🎯 摘要 -->## 🎯 摘要

本文档深入探讨了OpenTelemetry分布式追踪与可观测性体系的架构设计、实施部署和最佳实践，基于微服务架构的生产环境经验，提供从追踪埋点到数据可视化的完整技术指南，帮助企业构建统一、标准的可观测性平台。

<!-- chunk: 1. OpenTelemetry架构深度解析 -->## 1. OpenTelemetry架构深度解析

## 1.1 核心组件架构

```mermaid
graph TB
    subgraph "应用层埋点"
        A[应用程序] --> B[OpenTelemetry SDK]
        C[手动埋点] --> B
        D[自动埋点] --> B
        E[业务指标] --> B
    end
    
    subgraph "数据收集层"
        B --> F[Collector Agent]
        F --> G[Collector Gateway]
        G --> H[Collector Processor]
    end
    
    subgraph "数据处理层"
        H --> I[Trace Processor]
        H --> J[Metric Processor]
        H --> K[Log Processor]
        I --> L[Sampling策略]
        J --> M[Aggregation聚合]
        K --> N[Enrichment增强]
    end
    
    subgraph "存储后端层"
        L --> O[Jaeger后端]
        M --> P[Prometheus]
        N --> Q[Elasticsearch]
        O --> R[分布式追踪存储]
        P --> S[时序数据库]
        Q --> T[日志存储]
    end
    
    subgraph "可视化层"
        U[Grafana Tempo]
        V[Jaeger UI]
        W[Kibana]
        X[Grafana仪表板]
    end
    
    R --> U
    R --> V
    S --> X
    T --> W
```

## 1.2 OpenTelemetry数据模型

```yaml
otel_data_model:
  traces:
    trace_id: "全局唯一追踪ID (16字节)"
    span_id: "跨度唯一ID (8字节)"
    parent_span_id: "父跨度ID"
    name: "跨度名称"
    kind: "SERVER/CLIENT/PRODUCER/CONSUMER/INTERNAL"
    start_time: "开始时间戳"
    end_time: "结束时间戳"
    attributes: "键值对属性"
    events: "时间点事件"
    links: "跨度间关联"
    status: "执行状态"
  
  metrics:
    name: "指标名称"
    description: "指标描述"
    unit: "单位"
    data:
      gauge: "瞬时值"
      sum: "累积值"
      histogram: "直方图分布"
      summary: "摘要统计"
    attributes: "维度标签"
  
  logs:
    timestamp: "时间戳"
    severity_text: "严重性文本"
    severity_number: "严重性数值"
    body: "日志内容"
    attributes: "结构化属性"
    trace_id: "关联追踪ID"
    span_id: "关联跨度ID"
```

<!-- chunk: 2. 企业级部署架构 -->## 2. 企业级部署架构

## 2.1 OpenTelemetry Collector部署

```yaml
# otel-collector-config.yaml
receivers:
  otlp:
    protocols:
      grpc:
        endpoint: 0.0.0.0:4317
      http:
        endpoint: 0.0.0.0:4318
  
  jaeger:
    protocols:
      thrift_http:
        endpoint: 0.0.0.0:14268
      grpc:
        endpoint: 0.0.0.0:14250
  
  zipkin:
    endpoint: 0.0.0.0:9411
  
  prometheus:
    config:
      scrape_configs:
        - job_name: 'otel-collector'
          static_configs:
            - targets: ['localhost:8888']

processors:
  batch:
    timeout: 5s
    send_batch_size: 1024
  
  memory_limiter:
    check_interval: 1s
    limit_mib: 4000
    spike_limit_mib: 500
  
  attributes:
    actions:
      - key: environment
        value: production
        action: insert
      - key: service.version
        action: update
        value: ${SERVICE_VERSION}
  
  filter:
    traces:
      span:
        - name == "health_check"
      attribute:
        - key: http.method
          value: "HEAD"
  
  probabilistic_sampler:
    sampling_percentage: 10.0
  
  spanmetrics:
    metrics_exporter: prometheus
    latency_histogram_buckets: [1ms, 2ms, 5ms, 10ms, 20ms, 50ms, 100ms, 200ms, 500ms, 1s, 2s, 5s]
    dimensions_cache_size: 1000

exporters:
  otlp/jaeger:
    endpoint: jaeger-collector:4317
    tls:
      insecure: true
  
  prometheus:
    endpoint: "0.0.0.0:8889"
    namespace: otel
    const_labels:
      exporter: otel-collector
  
  elasticsearch:
    endpoints:
      - http://elasticsearch:9200
    logs_index: otel-logs
    traces_index: otel-traces
    user: ${ES_USER}
    password: ${ES_PASSWORD}
  
  kafka:
    brokers:
      - kafka-0:9092
      - kafka-1:9092
      - kafka-2:9092
    topic: otel-data
    encoding: otlp_proto

extensions:
  health_check:
    endpoint: 0.0.0.0:13133
  pprof:
    endpoint: :1777
  zpages:
    endpoint: :55679

service:
  extensions: [health_check, pprof, zpages]
  pipelines:
    traces:
      receivers: [otlp, jaeger, zipkin]
      processors: [memory_limiter, batch, attributes, probabilistic_sampler]
      exporters: [otlp/jaeger, kafka]
    
    metrics:
      receivers: [otlp, prometheus]
      processors: [memory_limiter, batch, attributes, spanmetrics]
      exporters: [prometheus, kafka]
    
    logs:
      receivers: [otlp]
      processors: [memory_limiter, batch, attributes]
      exporters: [elasticsearch, kafka]
```

## 2.2 Kubernetes部署配置

```yaml
# otel-collector-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: otel-collector
  namespace: observability
spec:
  replicas: 3
  selector:
    matchLabels:
      app: otel-collector
  template:
    metadata:
      labels:
        app: otel-collector
    spec:
      containers:
      - name: otel-collector
        image: otel/opentelemetry-collector-contrib:0.88.0
        args:
        - --config=/conf/otel-collector-config.yaml
        ports:
        - name: otlp-grpc
          containerPort: 4317
          protocol: TCP
        - name: otlp-http
          containerPort: 4318
          protocol: TCP
        - name: jaeger-thrift
          containerPort: 14268
          protocol: TCP
        - name: jaeger-grpc
          containerPort: 14250
          protocol: TCP
        - name: zipkin
          containerPort: 9411
          protocol: TCP
        - name: metrics
          containerPort: 8889
          protocol: TCP
        - name: health-check
          containerPort: 13133
          protocol: TCP
        env:
        - name: SERVICE_VERSION
          valueFrom:
            fieldRef:
              fieldPath: metadata.labels['version']
        - name: ES_USER
          valueFrom:
            secretKeyRef:
              name: elasticsearch-secret
              key: username
        - name: ES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: elasticsearch-secret
              key: password
        volumeMounts:
        - name: config
          mountPath: /conf
        resources:
          requests:
            memory: "1Gi"
            cpu: "500m"
          limits:
            memory: "2Gi"
            cpu: "1000m"
        livenessProbe:
          httpGet:
            path: /
            port: 13133
          initialDelaySeconds: 5
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /
            port: 13133
          initialDelaySeconds: 1
          periodSeconds: 10
      volumes:
      - name: config
        configMap:
          name: otel-collector-config
---
# otel-collector-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: otel-collector
  namespace: observability
spec:
  selector:
    app: otel-collector
  ports:
  - name: otlp-grpc
    port: 4317
    protocol: TCP
    targetPort: 4317
  - name: otlp-http
    port: 4318
    protocol: TCP
    targetPort: 4318
  - name: jaeger-thrift
    port: 14268
    protocol: TCP
    targetPort: 14268
  - name: metrics
    port: 8889
    protocol: TCP
    targetPort: 8889
  type: ClusterIP
```

<!-- chunk: 3. 应用程序埋点实践 -->## 3. 应用程序埋点实践

## 3.1 Java应用埋点

```java
// OpenTelemetry Java示例
import io.opentelemetry.api.GlobalOpenTelemetry;
import io.opentelemetry.api.OpenTelemetry;
import io.opentelemetry.api.trace.Span;
import io.opentelemetry.api.trace.Tracer;
import io.opentelemetry.context.Scope;
import io.opentelemetry.instrumentation.annotations.WithSpan;
import io.opentelemetry.instrumentation.annotations.SpanAttribute;

@Service
public class UserService {
    
    private final Tracer tracer = GlobalOpenTelemetry.getTracer("user-service");
    
    @WithSpan("getUserById")
    public User getUserById(@SpanAttribute("userId") String userId) {
        Span span = Span.current();
        
        try {
            // 添加业务属性
            span.setAttribute("user.id", userId);
            span.setAttribute("db.system", "postgresql");
            
            // 模拟数据库查询
            User user = database.findById(userId);
            
            if (user == null) {
                span.setStatus(StatusCode.ERROR, "User not found");
                return null;
            }
            
            // 添加用户相关信息到span
            span.setAttribute("user.email", user.getEmail());
            span.setAttribute("user.role", user.getRole());
            
            return user;
            
        } catch (Exception e) {
            span.recordException(e);
            span.setStatus(StatusCode.ERROR, e.getMessage());
            throw e;
        }
    }
    
    public List<User> getUsersByDepartment(String department) {
        // 手动创建span
        Span span = tracer.spanBuilder("getUsersByDepartment")
                .setAttribute("department", department)
                .startSpan();
        
        try (Scope scope = span.makeCurrent()) {
            // 添加事件
            span.addEvent("Starting database query");
            
            List<User> users = database.findByDepartment(department);
            
            span.addEvent("Database query completed", 
                Attributes.of(AttributeKey.longKey("user.count"), (long) users.size()));
            
            return users;
            
        } finally {
            span.end();
        }
    }
}
```

## 3.2 Python应用埋点

```python
# OpenTelemetry Python示例
from opentelemetry import trace
from opentelemetry.trace import SpanKind
from opentelemetry.instrumentation.flask import FlaskInstrumentor
from opentelemetry.instrumentation.requests import RequestsInstrumentor
from opentelemetry.instrumentation.psycopg2 import Psycopg2Instrumentor
from opentelemetry.exporter.otlp.proto.grpc.trace_exporter import OTLPSpanExporter
from opentelemetry.sdk.resources import SERVICE_NAME, Resource
from opentelemetry.sdk.trace import TracerProvider
from opentelemetry.sdk.trace.export import BatchSpanProcessor
import psycopg2

# 初始化OpenTelemetry
resource = Resource(attributes={
    SERVICE_NAME: "order-service"
})

provider = TracerProvider(resource=resource)
processor = BatchSpanProcessor(OTLPSpanExporter(endpoint="otel-collector:4317"))
provider.add_span_processor(processor)
trace.set_tracer_provider(provider)

tracer = trace.get_tracer(__name__)

# 自动仪器化
FlaskInstrumentor().instrument_app(app)
RequestsInstrumentor().instrument()
Psycopg2Instrumentor().instrument()

@app.route('/orders/<order_id>')
def get_order(order_id):
    with tracer.start_as_current_span("get_order", kind=SpanKind.SERVER) as span:
        span.set_attribute("order.id", order_id)
        span.set_attribute("http.method", request.method)
        span.set_attribute("http.route", f"/orders/{order_id}")
        
        try:
            # 数据库操作span
            with tracer.start_as_current_span("database_query") as db_span:
                db_span.set_attribute("db.system", "postgresql")
                db_span.set_attribute("db.operation", "SELECT")
                db_span.set_attribute("db.statement", "SELECT * FROM orders WHERE id = %s")
                
                conn = psycopg2.connect(database_url)
                cur = conn.cursor()
                cur.execute("SELECT * FROM orders WHERE id = %s", (order_id,))
                order_data = cur.fetchone()
                cur.close()
                conn.close()
                
                db_span.set_attribute("db.row_count", 1 if order_data else 0)
            
            if not order_data:
                span.set_status(trace.Status(trace.StatusCode.ERROR, "Order not found"))
                return {"error": "Order not found"}, 404
            
            # 外部API调用span
            with tracer.start_as_current_span("external_api_call") as api_span:
                api_span.set_attribute("http.url", "https://payment-service/api/verify")
                api_span.set_attribute("http.method", "POST")
                
                response = requests.post(
                    "https://payment-service/api/verify",
                    json={"order_id": order_id}
                )
                api_span.set_attribute("http.status_code", response.status_code)
            
            span.set_status(trace.Status(trace.StatusCode.OK))
            return {
                "order_id": order_id,
                "data": order_data,
                "verified": response.json().get("verified", False)
            }
            
        except Exception as e:
            span.record_exception(e)
            span.set_status(trace.Status(trace.StatusCode.ERROR, str(e)))
            return {"error": str(e)}, 500
```

## 3.3 Go应用埋点

```go
// OpenTelemetry Go示例
package main

import (
    "context"
    "database/sql"
    "encoding/json"
    "net/http"
    
    "go.opentelemetry.io/otel"
    "go.opentelemetry.io/otel/attribute"
    "go.opentelemetry.io/otel/exporters/otlp/otlptrace"
    "go.opentelemetry.io/otel/exporters/otlp/otlptrace/otlptracegrpc"
    "go.opentelemetry.io/otel/propagation"
    "go.opentelemetry.io/otel/sdk/resource"
    sdktrace "go.opentelemetry.io/otel/sdk/trace"
    "go.opentelemetry.io/otel/semconv/v1.17.0"
    "go.opentelemetry.io/otel/trace"
    
    _ "github.com/lib/pq"
)

func initTracer() (*sdktrace.TracerProvider, error) {
    ctx := context.Background()
    
    res, err := resource.New(ctx,
        resource.WithAttributes(
            semconv.ServiceName("inventory-service"),
            semconv.ServiceVersion("1.0.0"),
        ),
    )
    if err != nil {
        return nil, err
    }
    
    traceExporter, err := otlptrace.New(
        ctx,
        otlptracegrpc.NewClient(
            otlptracegrpc.WithEndpoint("otel-collector:4317"),
            otlptracegrpc.WithInsecure(),
        ),
    )
    if err != nil {
        return nil, err
    }
    
    tp := sdktrace.NewTracerProvider(
        sdktrace.WithBatcher(traceExporter),
        sdktrace.WithResource(res),
    )
    
    otel.SetTracerProvider(tp)
    otel.SetTextMapPropagator(propagation.NewCompositeTextMapPropagator(
        propagation.TraceContext{},
        propagation.Baggage{},
    ))
    
    return tp, nil
}

type InventoryService struct {
    db     *sql.DB
    tracer trace.Tracer
}

func NewInventoryService(db *sql.DB) *InventoryService {
    return &InventoryService{
        db:     db,
        tracer: otel.Tracer("inventory-service"),
    }
}

func (s *InventoryService) GetProductStock(ctx context.Context, productID string) (int, error) {
    ctx, span := s.tracer.Start(ctx, "GetProductStock", 
        trace.WithSpanKind(trace.SpanKindServer),
        trace.WithAttributes(
            attribute.String("product.id", productID),
        ))
    defer span.End()
    
    // 数据库查询span
    dbCtx, dbSpan := s.tracer.Start(ctx, "database.query",
        trace.WithSpanKind(trace.SpanKindClient),
        trace.WithAttributes(
            attribute.String("db.system", "postgresql"),
            attribute.String("db.operation", "SELECT"),
        ))
    defer dbSpan.End()
    
    var stock int
    query := "SELECT stock FROM products WHERE id = $1"
    err := s.db.QueryRowContext(dbCtx, query, productID).Scan(&stock)
    if err != nil {
        if err == sql.ErrNoRows {
            span.SetStatus(codes.Error, "Product not found")
            return 0, err
        }
        dbSpan.RecordError(err)
        span.RecordError(err)
        span.SetStatus(codes.Error, err.Error())
        return 0, err
    }
    
    dbSpan.SetAttributes(attribute.Int("db.row_count", 1))
    span.SetAttributes(attribute.Int("product.stock", stock))
    span.SetStatus(codes.Ok, "")
    
    return stock, nil
}

func (s *InventoryService) UpdateStock(ctx context.Context, productID string, quantity int) error {
    ctx, span := s.tracer.Start(ctx, "UpdateStock",
        trace.WithSpanKind(trace.SpanKindServer),
        trace.WithAttributes(
            attribute.String("product.id", productID),
            attribute.Int("quantity.change", quantity),
        ))
    defer span.End()
    
    tx, err := s.db.BeginTx(ctx, nil)
    if err != nil {
        span.RecordError(err)
        span.SetStatus(codes.Error, err.Error())
        return err
    }
    defer tx.Rollback()
    
    // 检查当前库存
    var currentStock int
    err = tx.QueryRowContext(ctx, "SELECT stock FROM products WHERE id = $1 FOR UPDATE", productID).Scan(&currentStock)
    if err != nil {
        span.RecordError(err)
        span.SetStatus(codes.Error, err.Error())
        return err
    }
    
    newStock := currentStock + quantity
    if newStock < 0 {
        span.SetStatus(codes.Error, "Insufficient stock")
        return fmt.Errorf("insufficient stock: current=%d, requested=%d", currentStock, -quantity)
    }
    
    // 更新库存
    _, err = tx.ExecContext(ctx, "UPDATE products SET stock = $1 WHERE id = $2", newStock, productID)
    if err != nil {
        span.RecordError(err)
        span.SetStatus(codes.Error, err.Error())
        return err
    }
    
    if err = tx.Commit(); err != nil {
        span.RecordError(err)
        span.SetStatus(codes.Error, err.Error())
        return err
    }
    
    span.SetAttributes(attribute.Int("product.new_stock", newStock))
    span.SetStatus(codes.Ok, "")
    return nil
}

func main() {
    tp, err := initTracer()
    if err != nil {
        log.Fatal(err)
    }
    defer func() {
        if err := tp.Shutdown(context.Background()); err != nil {
            log.Printf("Error shutting down tracer provider: %v", err)
        }
    }()
    
    db, err := sql.Open("postgres", "postgresql://user:pass@localhost:5432/inventory")
    if err != nil {
        log.Fatal(err)
    }
    defer db.Close()
    
    service := NewInventoryService(db)
    
    http.HandleFunc("/products/{id}/stock", func(w http.ResponseWriter, r *http.Request) {
        ctx := r.Context()
        productID := r.PathValue("id")
        
        stock, err := service.GetProductStock(ctx, productID)
        if err != nil {
            http.Error(w, err.Error(), http.StatusNotFound)
            return
        }
        
        json.NewEncoder(w).Encode(map[string]interface{}{
            "product_id": productID,
            "stock":      stock,
        })
    })
    
    log.Println("Server starting on :8080")
    log.Fatal(http.ListenAndServe(":8080", nil))
}
```

<!-- chunk: 4. 采样策略与配置 -->## 4. 采样策略与配置

## 4.1 采样策略配置

```yaml
# sampling-strategies.yaml
sampling:
  default:
    # 默认采样率 - 10%
    probability: 0.1
  
  service_specific:
    # 关键服务提高采样率
    user-service:
      probability: 0.5
    
    payment-service:
      probability: 1.0  # 100%采样
    
    # 非关键服务降低采样率
    logging-service:
      probability: 0.01
  
  error_based:
    # 错误自动采样
    enabled: true
    error_probability: 1.0  # 错误100%采样
  
  rate_limiting:
    # 速率限制采样
    spans_per_second: 1000
    burst_size: 2000
  
  adaptive:
    # 自适应采样
    enabled: true
    target_spans_per_second: 1000
    backoff_factor: 2
    min_sampling_probability: 0.01
    max_sampling_probability: 1.0
```

## 4.2 高级采样配置

```json
{
  "default_sampling_probability": 0.1,
  "service_strategies": [
    {
      "service": "user-service",
      "type": "probabilistic",
      "param": 0.5
    },
    {
      "service": "payment-service",
      "type": "ratelimiting",
      "param": 1000
    }
  ],
  "operation_strategies": [
    {
      "operation": "HTTP GET /health",
      "type": "probabilistic",
      "param": 0.0
    },
    {
      "operation": "database.query",
      "type": "probabilistic",
      "param": 0.2
    }
  ],
  "error_sampling": {
    "enabled": true,
    "probability": 1.0
  }
}
```

<!-- chunk: 5. 数据可视化与分析 -->## 5. 数据可视化与分析

## 5.1 Grafana Tempo配置

```yaml
# grafana-tempo-datasource.yaml
apiVersion: 1
datasources:
  - name: Tempo
    type: tempo
    access: proxy
    url: http://tempo:3100
    jsonData:
      httpMethod: GET
      tracesToLogs:
        datasourceUid: 'loki'
        tags: ['job', 'instance', 'pod', 'namespace']
        mappedTags: [{key: 'service.name', value: 'service'}]
        mapTagNamesEnabled: false
        spanStartTimeShift: '1h'
        spanEndTimeShift: '1h'
        filterByTraceID: false
        filterBySpanID: false
      tracesToMetrics:
        datasourceUid: 'prometheus'
        tags: [{'key': 'service.name', 'value': 'service'}, {'key': 'job'}]
        queries:
          - name: 'Latency'
            query: 'histogram_quantile(0.95, sum(rate(traces_spanmetrics_latency_bucket{$$__tags}[5m])) by (le))'
          - name: 'Throughput'
            query: 'sum(rate(traces_spanmetrics_calls_total{$$__tags}[5m]))'
      serviceMap:
        datasourceUid: 'prometheus'
      search:
        hide: false
      nodeGraph:
        enabled: true
```

## 5.2 追踪查询DSL

```python
# Tempo查询示例
import requests
import json

class TempoQuery:
    def __init__(self, tempo_url):
        self.tempo_url = tempo_url
    
    def search_traces(self, service_name=None, operation_name=None, 
                     min_duration=None, max_duration=None, 
                     start_time=None, end_time=None, limit=20):
        """
        搜索追踪数据
        """
        query_params = {
            'service-name': service_name,
            'operation-name': operation_name,
            'min-duration': min_duration,
            'max-duration': max_duration,
            'start': start_time,
            'end': end_time,
            'limit': limit
        }
        
        # 过滤None值
        query_params = {k: v for k, v in query_params.items() if v is not None}
        
        response = requests.get(
            f"{self.tempo_url}/api/search",
            params=query_params
        )
        
        return response.json()
    
    def get_trace_by_id(self, trace_id):
        """
        根据追踪ID获取完整追踪
        """
        response = requests.get(
            f"{self.tempo_url}/api/traces/{trace_id}"
        )
        
        return response.json()
    
    def analyze_service_dependencies(self, service_name, time_range="1h"):
        """
        分析服务依赖关系
        """
        # 查询服务的所有spans
        traces = self.search_traces(
            service_name=service_name,
            start_time=f"now-{time_range}"
        )
        
        dependencies = {}
        
        for trace in traces.get('traces', []):
            for span in trace.get('spans', []):
                # 分析span之间的调用关系
                if span.get('kind') == 'SPAN_KIND_CLIENT':
                    peer_service = span.get('attributes', {}).get('peer.service')
                    if peer_service:
                        dependencies[peer_service] = dependencies.get(peer_service, 0) + 1
        
        return dependencies

# 使用示例
tempo = TempoQuery("http://tempo:3100")

# 搜索慢查询
slow_traces = tempo.search_traces(
    min_duration="500ms",
    limit=10
)

# 分析服务依赖
dependencies = tempo.analyze_service_dependencies("user-service")
print("Service Dependencies:", dependencies)

# 获取特定追踪详情
trace_detail = tempo.get_trace_by_id("abcdef123456")
```

<!-- chunk: 6. 性能优化与最佳实践 -->## 6. 性能优化与最佳实践

## 6.1 追踪数据优化

```yaml
# otel-performance-optimization.yaml
performance_optimization:
  batching:
    # 批量处理配置
    send_batch_size: 1024
    send_batch_max_size: 2048
    timeout: 5s
    max_concurrent_exports: 10
  
  memory_management:
    # 内存限制
    check_interval: 1s
    limit_mib: 4000
    spike_limit_mib: 500
    ballast_size_mib: 1000
  
  compression:
    # 数据压缩
    enabled: true
    algorithm: gzip
    level: 6
  
  queuing:
    # 队列配置
    queue_size: 5000
    num_consumers: 20
    retry_on_failure:
      enabled: true
      initial_interval: 5s
      max_interval: 30s
      max_elapsed_time: 300s
  
  resource_attributes:
    # 资源属性优化
    include:
      - service.name
      - service.version
      - service.instance.id
      - host.name
      - host.arch
    exclude:
      - telemetry.auto.version
      - process.command_args
```

## 6.2 最佳实践指南

```markdown
<!-- chunk: 📊 OpenTelemetry实施最佳实践 -->## 📊 OpenTelemetry实施最佳实践

## 1. 埋点策略
- **自动埋点优先**: 使用自动仪器化减少手动工作
- **关键路径埋点**: 重点关注业务核心流程
- **适度冗余**: 避免过度埋点影响性能
- **语义化命名**: 使用清晰、一致的命名规范

## 2. 采样策略
- **分层采样**: 不同服务采用不同采样率
- **错误优先**: 错误和异常100%采样
- **自适应调整**: 根据系统负载动态调整采样率
- **业务相关**: 关键业务流程提高采样频率

## 3. 数据治理
- **属性标准化**: 建立统一的属性命名规范
- **敏感信息过滤**: 自动过滤PII等敏感数据
- **数据生命周期**: 合理设置数据保留期限
- **成本控制**: 监控和优化数据存储成本

## 4. 性能考量
- **异步处理**: 避免阻塞主线程
- **批量发送**: 减少网络请求次数
- **内存控制**: 设置合理的内存限制
- **问题降级**: 确保追踪不影响主业务
```

<!-- chunk: 7. 故障排查与监控 -->## 7. 故障排查与监控

## 7.1 常见问题诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash
# otel-troubleshooting.sh

# 1. 检查Collector状态
echo "=== Collector Health Check ==="
curl -s http://otel-collector:13133/ | jq '.'

# 2. 检查接收器状态
echo "=== Receiver Stats ==="
curl -s http://otel-collector:8888/metrics | grep receiver_

# 3. 检查处理器状态
echo "=== Processor Stats ==="
curl -s http://otel-collector:8888/metrics | grep processor_

# 4. 检查导出器状态
echo "=== Exporter Stats ==="
curl -s http://otel-collector:8888/metrics | grep exporter_

# 5. 检查应用埋点状态
echo "=== Application Instrumentation Check ==="
kubectl exec -it <app-pod> -- curl -s http://localhost:13133/

# 6. 验证追踪数据流
echo "=== Trace Data Validation ==="
trace_id=$(curl -s http://tempo:3100/api/search?service-name=test-service | jq -r '.traces[0].traceID')
if [ -n "$trace_id" ]; then
    curl -s "http://tempo:3100/api/traces/$trace_id" | jq '.'
fi
```

## 7.2 监控告警配置

```yaml
# otel-monitoring-alerts.yaml
groups:
- name: opentelemetry.rules
  rules:
  # Collector健康检查
  - alert: OtelCollectorDown
    expr: up{job="otel-collector"} == 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "OpenTelemetry Collector is down"
      description: "OpenTelemetry Collector instance {{ $labels.instance }} is not responding"
  
  # 数据接收异常
  - alert: OtelReceiverErrors
    expr: rate(otelcol_receiver_refused_spans_total[5m]) > 0
    for: 1m
    labels:
      severity: warning
    annotations:
      summary: "OpenTelemetry receiver errors"
      description: "Receiver is refusing spans at {{ $value }}/second"
  
  # 导出失败
  - alert: OtelExporterFailures
    expr: rate(otelcol_exporter_send_failed_spans_total[5m]) > 10
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "OpenTelemetry export failures"
      description: "Export failures at {{ $value }}/second"
  
  # 内存使用过高
  - alert: OtelCollectorMemoryHigh
    expr: container_memory_usage_bytes{container="otel-collector"} / container_spec_memory_limit_bytes{container="otel-collector"} * 100 > 80
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "OpenTelemetry Collector memory usage high"
      description: "Memory usage is {{ $value }}%"
  
  # 队列积压
  - alert: OtelQueueBacklog
    expr: otelcol_processor_queued_retry_queue_length > 1000
    for: 2m
    labels:
      severity: warning
    annotations:
      summary: "OpenTelemetry queue backlog"
      description: "Queue length is {{ $value }}"
```

<!-- chunk: 8. 未来发展与趋势 -->## 8. 未来发展与趋势

## 8.1 技术演进方向

```yaml
future_trends:
  auto_instrumentation:
    - 零代码埋点技术
    - AI驱动的智能埋点
    - 运行时动态插桩
    - 无侵入式监控
  
  enhanced_analytics:
    - 实时根因分析
    - 预测性故障检测
    - 智能异常识别
    - 自动化性能优化
  
  platform_convergence:
    - 统一可观测性平台
    - 跨云原生集成
    - 边缘计算支持
    - Serverless环境适配
  
  standard_evolution:
    - OpenTelemetry标准完善
    - 跨语言互操作性
    - 行业最佳实践固化
    - 合规性框架集成
```

---
*本文档基于企业级分布式追踪实践经验编写，持续更新最新技术和最佳实践。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-20-enterprise-monitoring-alerting KUDIG Database — Global MOC
- [[domain-06-observability/README.md|[[Domain 20: 企业级监控与告警 (Enterprise Monitoring & Alerting)|Domain 20: 企业级监控与告警 (Enterprise Monitoring & Alerting)]]nitoring]] & Alerting)]]
- index.md|Domain-20 企业监控与告警 — 开源项目索引]]
- [[entities/prometheus.md|prometheus]]
- Grafana Enterprise Observability Platform 深度实践
- Thanos Enterprise Metrics Federation and Long-term Storage
- Datadog企业级APM深度实践
- Datadog 企业级监控平台深度实践
- Elastic Stack企业级日志分析深度实践
- Elastic Stack企业级可观测性平台深度实践
- Zabbix Enterprise Monitoring Platform 深度实践
- New Relic Enterprise APM Platform 深度实践

## See Also

- 01-prometheus-enterprise-monitoring
- 02-grafana-enterprise-observability
- 04-thanos-enterprise-metrics-federation
- 05-datadog-enterprise-apm

- [[domain-06-observability/README.md|返回目录]]

## Related

- [[domain-19-landscape-references/topic-index/observability-index.md|Observability 可观测性知识图谱索引]]
