# 可观测性工具

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [Prometheus](https://prometheus.io/) | [OpenTelemetry](https://opentelemetry.io/)

## 工具生态对比

| 工具 | 类型 | 数据类型 | 存储 | 查询语言 | 可视化 | 云原生 | 生产推荐 |
|------|------|---------|------|---------|--------|--------|---------|
| **Prometheus** | 指标 | Metrics | 本地TSDB | PromQL | Grafana | ⭐⭐⭐⭐⭐ | 强烈推荐 |
| **Grafana** | 可视化 | All | N/A | 多数据源 | 内置 | ⭐⭐⭐⭐⭐ | 强烈推荐 |
| **Loki** | 日志 | Logs | 对象存储 | LogQL | Grafana | ⭐⭐⭐⭐⭐ | 推荐 |
| **Jaeger** | 链路 | Traces | 多种 | UI查询 | 内置 | ⭐⭐⭐⭐ | 推荐 |
| **Tempo** | 链路 | Traces | 对象存储 | TraceQL | Grafana | ⭐⭐⭐⭐ | 推荐 |
| **OpenTelemetry** | 采集框架 | All | 多后端 | N/A | N/A | ⭐⭐⭐⭐⭐ | 标准化 |
| **Thanos** | 长期存储 | Metrics | 对象存储 | PromQL | Grafana | ⭐⭐⭐⭐⭐ | 大规模 |

---

## Prometheus 生产部署

### 架构模式

```
┌──────────────────────────────────────────┐
│           Prometheus Operator            │
├──────────────────────────────────────────┤
│  ┌────────────┐  ┌─────────────────────┐ │
│  │ Prometheus │  │  ServiceMonitor     │ │
│  │  Servers   │  │  PodMonitor         │ │
│  │ (Sharded)  │  │  PrometheusRule     │ │
│  └─────┬──────┘  └──────────┬──────────┘ │
└────────┼──────────────────────┼───────────┘
         │                      │
         │ scrape               │ auto-discover
         v                      v
┌─────────────────────────────────────────┐
│        Kubernetes Cluster               │
│  ┌────────┐  ┌────────┐  ┌────────┐   │
│  │  Pod   │  │  Pod   │  │  Pod   │   │
│  └────────┘  └────────┘  └────────┘   │
└─────────────────────────────────────────┘
         │
         │ remote_write
         v
┌─────────────────────┐
│  Thanos / Mimir     │  (长期存储)
│  (S3/OSS)           │
└─────────────────────┘
```

### Helm 安装 kube-prometheus-stack

```bash
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update

helm install kube-prometheus prometheus-community/kube-prometheus-stack \
  -n monitoring --create-namespace \
  --set prometheus.prometheusSpec.retention=15d \
  --set prometheus.prometheusSpec.storageSpec.volumeClaimTemplate.spec.resources.requests.storage=100Gi \
  --set prometheus.prometheusSpec.resources.requests.memory=4Gi \
  --set prometheus.prometheusSpec.resources.limits.memory=8Gi \
  --set grafana.adminPassword=SecureP@ss123 \
  --set alertmanager.enabled=true
```

### Prometheus CRD 配置

#### ServiceMonitor 示例

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: myapp-metrics
  namespace: production
  labels:
    release: kube-prometheus  # 匹配Prometheus的selector
spec:
  selector:
    matchLabels:
      app: myapp
  endpoints:
    - port: metrics
      interval: 30s
      path: /metrics
      relabelings:
        - sourceLabels: [__meta_kubernetes_pod_name]
          targetLabel: pod
        - sourceLabels: [__meta_kubernetes_namespace]
          targetLabel: namespace
```

#### PodMonitor (无Service场景)

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PodMonitor
metadata:
  name: gpu-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      gpu-monitoring: "true"
  podMetricsEndpoints:
    - port: metrics
      interval: 15s
      path: /metrics
```

#### PrometheusRule (告警规则)

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: myapp-alerts
  namespace: production
spec:
  groups:
    - name: myapp.rules
      interval: 30s
      rules:
        # 告警规则
        - alert: HighErrorRate
          expr: |
            rate(http_requests_total{status=~"5.."}[5m])
            / rate(http_requests_total[5m]) > 0.05
          for: 10m
          labels:
            severity: critical
            team: backend
          annotations:
            summary: "应用错误率过高"
            description: "{{ $labels.namespace }}/{{ $labels.pod }} 错误率 {{ $value | humanizePercentage }}"
        
        # 预聚合规则(Recording Rule)
        - record: job:http_requests:rate5m
          expr: rate(http_requests_total[5m])
```

### Prometheus 高可用配置

```yaml
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: main
  namespace: monitoring
spec:
  replicas: 2  # 高可用副本
  shards: 3    # 水平分片
  replicaExternalLabelName: "__replica__"  # 去重标签
  prometheusExternalLabelName: "prometheus"
  
  # 资源配置
  resources:
    requests:
      memory: 4Gi
      cpu: 2000m
    limits:
      memory: 8Gi
      cpu: 4000m
  
  # 存储配置
  retention: 15d
  retentionSize: 90GB
  storageSpec:
    volumeClaimTemplate:
      spec:
        accessModes: ["ReadWriteOnce"]
        resources:
          requests:
            storage: 100Gi
        storageClassName: ssd
  
  # 远程写入(Thanos)
  remoteWrite:
    - url: http://thanos-receive:19291/api/v1/receive
      queueConfig:
        capacity: 10000
        maxShards: 50
        minShards: 1
  
  # 服务发现配置
  serviceMonitorSelector:
    matchLabels:
      release: kube-prometheus
  podMonitorSelector: {}
  
  # 告警管理器
  alerting:
    alertmanagers:
      - namespace: monitoring
        name: alertmanager-main
        port: web
```

---

## Grafana 企业实践

### 数据源配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: grafana-datasources
  namespace: monitoring
data:
  datasources.yaml: |
    apiVersion: 1
    datasources:
      # Prometheus
      - name: Prometheus
        type: prometheus
        access: proxy
        url: http://prometheus-operated:9090
        isDefault: true
        jsonData:
          timeInterval: 30s
          httpMethod: POST
      
      # Loki
      - name: Loki
        type: loki
        access: proxy
        url: http://loki-gateway:80
        jsonData:
          maxLines: 1000
      
      # Tempo
      - name: Tempo
        type: tempo
        access: proxy
        url: http://tempo-query-frontend:3100
        jsonData:
          tracesToLogs:
            datasourceUid: loki
            tags: ['job', 'instance', 'pod']
          serviceMap:
            datasourceUid: prometheus
```

### Dashboard 即代码

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: myapp-dashboard
  namespace: monitoring
  labels:
    grafana_dashboard: "1"  # Grafana Operator自动加载
data:
  myapp.json: |
    {
      "dashboard": {
        "title": "MyApp Metrics",
        "panels": [
          {
            "id": 1,
            "title": "QPS",
            "targets": [
              {
                "expr": "rate(http_requests_total{app=\"myapp\"}[5m])",
                "legendFormat": "{{pod}}"
              }
            ]
          }
        ]
      }
    }
```

---

## Loki 日志聚合

### 架构部署

```bash
# Loki分布式部署
helm install loki grafana/loki-distributed \
  -n monitoring \
  --set loki.storage.type=s3 \
  --set loki.storage.s3.endpoint=oss-cn-hangzhou.aliyuncs.com \
  --set loki.storage.bucketNames.chunks=loki-chunks \
  --set loki.storage.bucketNames.ruler=loki-ruler \
  --set ingester.replicas=3 \
  --set querier.replicas=3
```

### Promtail 日志采集

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: promtail-config
  namespace: monitoring
data:
  promtail.yaml: |
    server:
      http_listen_port: 3101
    
    positions:
      filename: /tmp/positions.yaml
    
    clients:
      - url: http://loki-gateway/loki/api/v1/push
    
    scrape_configs:
      # Kubernetes Pods
      - job_name: kubernetes-pods
        kubernetes_sd_configs:
          - role: pod
        relabel_configs:
          - source_labels: [__meta_kubernetes_pod_label_app]
            target_label: app
          - source_labels: [__meta_kubernetes_namespace]
            target_label: namespace
          - source_labels: [__meta_kubernetes_pod_name]
            target_label: pod
        pipeline_stages:
          # JSON日志解析
          - json:
              expressions:
                level: level
                message: message
                trace_id: trace_id
          - labels:
              level:
              trace_id:
```

### LogQL 查询示例

```logql
# 基础查询
{app="myapp", namespace="production"}

# 错误日志
{app="myapp"} |= "error" | json | level="ERROR"

# 正则过滤
{app="myapp"} |~ "status: (500|502|503)"

# 指标转换(日志聚合)
rate({app="myapp"} |= "error" [5m])

# 关联Trace ID
{app="myapp"} | json | trace_id="abc123"
```

---

## Jaeger / Tempo 分布式追踪

### OpenTelemetry Collector 部署

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: otel-collector-config
  namespace: monitoring
data:
  otel-collector-config.yaml: |
    receivers:
      otlp:
        protocols:
          grpc:
            endpoint: 0.0.0.0:4317
          http:
            endpoint: 0.0.0.0:4318
      jaeger:
        protocols:
          grpc:
            endpoint: 0.0.0.0:14250
    
    processors:
      batch:
        timeout: 10s
        send_batch_size: 1024
      
      # 采样策略
      probabilistic_sampler:
        sampling_percentage: 10  # 采样10%
      
      # 添加资源属性
      resource:
        attributes:
          - key: cluster
            value: production
            action: upsert
    
    exporters:
      # Tempo
      otlp/tempo:
        endpoint: tempo-distributor:4317
        tls:
          insecure: true
      
      # Jaeger
      jaeger:
        endpoint: jaeger-collector:14250
        tls:
          insecure: true
      
      # Prometheus(Span指标)
      prometheus:
        endpoint: "0.0.0.0:8889"
    
    service:
      pipelines:
        traces:
          receivers: [otlp, jaeger]
          processors: [batch, probabilistic_sampler, resource]
          exporters: [otlp/tempo, prometheus]
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: otel-collector
  namespace: monitoring
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
          image: otel/opentelemetry-collector-contrib:0.91.0
          args:
            - "--config=/conf/otel-collector-config.yaml"
          volumeMounts:
            - name: config
              mountPath: /conf
          ports:
            - containerPort: 4317  # OTLP gRPC
            - containerPort: 4318  # OTLP HTTP
            - containerPort: 8889  # Prometheus metrics
      volumes:
        - name: config
          configMap:
            name: otel-collector-config
```

### 应用集成示例 (Go)

```go
import (
    "go.opentelemetry.io/otel"
    "go.opentelemetry.io/otel/exporters/otlp/otlptrace/otlptracegrpc"
    "go.opentelemetry.io/otel/sdk/trace"
)

func initTracer() {
    exporter, _ := otlptracegrpc.New(context.Background(),
        otlptracegrpc.WithEndpoint("otel-collector:4317"),
        otlptracegrpc.WithInsecure(),
    )
    
    tp := trace.NewTracerProvider(
        trace.WithBatcher(exporter),
        trace.WithResource(resource.NewWithAttributes(
            semconv.ServiceNameKey.String("myapp"),
        )),
        trace.WithSampler(trace.ParentBased(trace.TraceIDRatioBased(0.1))),
    )
    
    otel.SetTracerProvider(tp)
}
```

---

## Thanos 长期存储

### 组件架构

```
┌────────────────┐
│  Prometheus    │──────┐
│  (Sidecar)     │      │
└────────────────┘      │
         │              │ remote_write
         │ upload       │
         v              v
┌────────────────┐  ┌────────────────┐
│ Object Storage │  │ Thanos Receive │
│   (S3/OSS)     │  └────────────────┘
└────────┬───────┘
         │
    ┌────┴─────┬──────────┬─────────┐
    v          v          v         v
┌───────┐  ┌───────┐  ┌────────┐  ┌────────┐
│ Store │  │ Query │  │ Compact│  │ Ruler  │
└───────┘  └───────┘  └────────┘  └────────┘
             │
             v
        ┌─────────┐
        │ Grafana │
        └─────────┘
```

### Helm 部署

```bash
helm install thanos bitnami/thanos \
  -n monitoring \
  --set objstoreConfig='{
    type: s3,
    config: {
      bucket: thanos-metrics,
      endpoint: oss-cn-hangzhou.aliyuncs.com,
      access_key: <ACCESS_KEY>,
      secret_key: <SECRET_KEY>
    }
  }' \
  --set query.replicaCount=3 \
  --set storegateway.replicaCount=3 \
  --set compactor.enabled=true \
  --set ruler.enabled=true
```

---

## 统一观测平台 (Grafana LGTM Stack)

```
┌──────────────────────────────────────┐
│          Grafana                     │
│  (统一可视化 + Alerting)              │
└──────────┬───────────────────────────┘
           │
    ┌──────┴──────┬──────────┬─────────┐
    │             │          │         │
    v             v          v         v
┌───────┐    ┌──────┐   ┌───────┐  ┌────────┐
│ Loki  │    │Grafana│   │ Tempo │  │ Mimir  │
│ (日志)│    │ Agent │   │ (链路)│  │ (指标) │
└───────┘    └───┬──┘   └───────┘  └────────┘
                 │
                 v
         ┌───────────────┐
         │ Applications  │
         └───────────────┘
```

---

## 告警管理最佳实践

```yaml
# AlertManager 配置
global:
  resolve_timeout: 5m

route:
  group_by: ['alertname', 'cluster', 'namespace']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 4h
  receiver: 'default'
  routes:
    # 严重告警立即发送
    - match:
        severity: critical
      receiver: 'pagerduty'
      continue: true
    
    # 团队路由
    - match:
        team: backend
      receiver: 'backend-team'
    
    - match:
        team: frontend
      receiver: 'frontend-team'

receivers:
  - name: 'default'
    webhook_configs:
      - url: 'http://alertmanager-webhook:8080/alerts'
  
  - name: 'pagerduty'
    pagerduty_configs:
      - service_key: '<PAGERDUTY_KEY>'
  
  - name: 'backend-team'
    email_configs:
      - to: 'backend@company.com'
    slack_configs:
      - api_url: '<SLACK_WEBHOOK>'
        channel: '#backend-alerts'
```

---

## 工具选型建议

| 场景 | 推荐方案 | 原因 |
|------|---------|------|
| **中小规模** | Prometheus + Grafana + Loki | 轻量级、易维护 |
| **大规模集群** | Prometheus + Thanos/Mimir | 长期存储、多集群 |
| **云原生标准** | OpenTelemetry + Tempo + Loki | 厂商中立、未来趋势 |
| **完整可观测** | Grafana LGTM Stack | 统一平台、无缝集成 |

---

## 常见问题

**Q: Prometheus数据保留多久?**  
A: 本地保留15-30天，长期存储使用Thanos/Mimir(对象存储)。

**Q: 如何减少Prometheus内存占用?**  
A: 降低采集频率、减少高基数标签、启用分片、使用Recording Rules。

**Q: Loki vs ELK如何选择?**  
A: Loki成本更低(不索引日志内容)、与Grafana无缝集成，适合云原生场景。
