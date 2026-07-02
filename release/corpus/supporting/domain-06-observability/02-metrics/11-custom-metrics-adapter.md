---
title: 07 - 自定义指标适配器与HPA扩展 (Custom Metrics Adapter & HPA Extension)
description: 本文档从生产环境运维专家视角，深入解析 Kubernetes 自定义指标适配器体系，涵盖 Prometheus Adapter、外部指标集成、HPA高级配置、指标管道优化等核心内容，结合大规模集群实践经验，为企业构建灵活、高效的自动扩缩容系统提供完整指导。
summary: 本文档从生产环境运维专家视角，深入解析 Kubernetes 自定义指标适配器体系，涵盖 Prometheus Adapter、外部指标集成、HPA高级配置、指标管道优化等核心内容，结合大规模集群实践经验，为企业构建灵活、高效的自动扩缩容系统提供完整指导。
category: observability
tags:
- k8s
- observability
- monitoring
- logging
- tracing
- kubelet
- prometheus
- helm
- hpa
- job
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 监控工程师
estimated_read_time: 5min
intent_queries:
- 自定义指标适配器与HPA扩展 (Custom Metrics Adapter & HPA Extension) 是什么
- 如何 自定义指标适配器与HPA扩展 (Custom Metrics Adapter & HPA Extension)
- Kubernetes 8 observability 最佳实践
trigger_keywords:
- 自定义指标适配器与HPA扩展
- Custom
- Metrics
- Adapter
- HPA
- Extension
- observability
prerequisites:
- kubectl-basics
- observability-basics
- helm-basics
- prometheus-basics
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
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-02-workloads-applications/
  label: '相关知识域: domain-02-workloads-applications'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-07-platform-engineering/
  label: '相关知识域: domain-07-platform-engineering'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/promql.md
  label: '速查卡: promql'
---



# 07 - 自定义指标适配器与HPA扩展 (Custom Metrics Adapter & HPA Extension)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [[entities/kubernetes.md|kubernetes]].io/docs/tasks/run-application/horizontal-pod-autoscale](https://kubernetes.io/docs/tasks/run-application/horizontal-pod-autoscale/)

本文档从生产环境运维专家视角，深入解析 Kubernetes 自定义指标适配器体系，涵盖 [[Prometheus|Prometheus]] Adapter、外部指标集成、HPA高级配置、指标管道优化等核心内容，结合大规模集群实践经验，为企业构建灵活、高效的自动扩缩容系统提供完整指导。

| API | 路径 | 提供者 | 用途 | 版本支持 |
|-----|------|-------|------|---------|
| **Resource Metrics** | metrics.k8s.io/v1beta1 | Metrics Server | CPU/Memory | 稳定 |
| **Custom Metrics** | custom.metrics.k8s.io/v1beta1 | Prometheus Adapter等 | 自定义Pod指标 | 稳定 |
| **External Metrics** | external.metrics.k8s.io/v1beta1 | 外部适配器 | 外部系统指标 | 稳定 |

<!-- chunk: Metrics Server -->
## Metrics Server

```yaml
# Metrics Server部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: metrics-server
  namespace: kube-system
spec:
  selector:
    matchLabels:
      k8s-app: metrics-server
  template:
    metadata:
      labels:
        k8s-app: metrics-server
    spec:
      containers:
      - name: metrics-server
        image: registry.k8s.io/metrics-server/metrics-server:v0.7.0
        args:
        - --cert-dir=/tmp
        - --secure-port=10250
        - --kubelet-preferred-address-types=InternalIP,ExternalIP,Hostname
        - --kubelet-use-node-status-port
        - --metric-resolution=15s
        # 测试环境可能需要
        # - --kubelet-insecure-tls
        resources:
          requests:
            cpu: 100m
            memory: 200Mi
```

<!-- chunk: Prometheus Adapter -->
## Prometheus Adapter

```yaml
# Prometheus Adapter Helm安装
# helm install prometheus-adapter prometheus-community/prometheus-adapter -f values.yaml

# values.yaml示例
prometheus:
  url: http://prometheus.monitoring.svc
  port: 9090

rules:
  default: true
  custom:
  # 自定义指标规则
  - seriesQuery: 'http_requests_total{namespace!="",pod!=""}'
    resources:
      overrides:
        namespace: {resource: "namespace"}
        pod: {resource: "pod"}
    name:
      matches: "^(.*)_total$"
      as: "${1}_per_second"
    metricsQuery: 'sum(rate(<<.Series>>{<<.LabelMatchers>>}[2m])) by (<<.GroupBy>>)'
  
  # 外部指标规则
  external:
  - seriesQuery: 'queue_messages_total{queue_name!=""}'
    resources:
      template: <<.Resource>>
    name:
      matches: "^(.*)_total$"
      as: "${1}"
    metricsQuery: 'sum(<<.Series>>{<<.LabelMatchers>>}) by (<<.GroupBy>>)'
```

<!-- chunk: Custom Metrics HPA -->
## Custom Metrics HPA

```yaml
# 基于自定义指标的HPA
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa-custom
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app
  minReplicas: 2
  maxReplicas: 20
  metrics:
  # 资源指标
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  # 自定义Pod指标
  - type: Pods
    pods:
      metric:
        name: http_requests_per_second
      target:
        type: AverageValue
        averageValue: "1000"
  # 自定义对象指标
  - type: Object
    object:
      metric:
        name: requests_per_second
      describedObject:
        apiVersion: networking.k8s.io/v1
        kind: Ingress
        name: main-ingress
      target:
        type: Value
        value: "10000"
  # 外部指标
  - type: External
    external:
      metric:
        name: queue_messages
        selector:
          matchLabels:
            queue: main-queue
      target:
        type: AverageValue
        averageValue: "30"
```

<!-- chunk: [[KEDA|KEDA]](Kubernetes Event-driven Autoscaling) -->
## KEDA(Kubernetes Event-driven Autoscaling)

```yaml
# KEDA安装
# kubectl apply -f https://github.com/kedacore/keda/releases/download/v2.12.0/keda-2.12.0.yaml

# ScaledObject示例
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: rabbitmq-scaledobject
spec:
  scaleTargetRef:
    name: consumer-deployment
  pollingInterval: 30
  cooldownPeriod: 300
  minReplicaCount: 1
  maxReplicaCount: 100
  triggers:
  - type: rabbitmq
    metadata:
      host: amqp://user:pass@rabbitmq:5672/
      queueName: tasks
      queueLength: "50"
---
# Prometheus触发器
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: prometheus-scaledobject
spec:
  scaleTargetRef:
    name: app-deployment
  triggers:
  - type: prometheus
    metadata:
      serverAddress: http://prometheus:9090
      metricName: http_requests_total
      threshold: "100"
      query: sum(rate(http_requests_total{deployment="app"}[2m]))
---
# Cron触发器(定时扩缩容)
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: cron-scaledobject
spec:
  scaleTargetRef:
    name: app-deployment
  triggers:
  - type: cron
    metadata:
      timezone: Asia/Shanghai
      start: 0 8 * * 1-5   # 工作日8点
      end: 0 20 * * 1-5    # 工作日20点
      desiredReplicas: "10"
```

<!-- chunk: 应用暴露自定义指标 -->
## 应用暴露自定义指标

```go
// Go应用暴露Prometheus指标示例
package main

import (
    "net/http"
    "github.com/prometheus/client_golang/prometheus"
    "github.com/prometheus/client_golang/prometheus/promhttp"
)

var (
    httpRequestsTotal = prometheus.NewCounterVec(
        prometheus.CounterOpts{
            Name: "http_requests_total",
            Help: "Total number of HTTP requests",
        },
        []string{"method", "path", "status"},
    )
    httpRequestDuration = prometheus.NewHistogramVec(
        prometheus.HistogramOpts{
            Name:    "http_request_duration_seconds",
            Help:    "HTTP request duration in seconds",
            Buckets: prometheus.DefBuckets,
        },
        []string{"method", "path"},
    )
)

func init() {
    prometheus.MustRegister(httpRequestsTotal)
    prometheus.MustRegister(httpRequestDuration)
}

func main() {
    http.Handle("/metrics", promhttp.Handler())
    http.ListenAndServe(":8080", nil)
}
```

```yaml
# Pod配置Prometheus抓取
apiVersion: v1
kind: Pod
metadata:
  annotations:
    prometheus.io/scrape: "true"
    prometheus.io/port: "8080"
    prometheus.io/path: "/metrics"
spec:
  containers:
  - name: app
    image: app:latest
    ports:
    - containerPort: 8080
      name: metrics
```

<!-- chunk: ServiceMonitor(Prometheus Operator) -->
## ServiceMonitor(Prometheus Operator)

```yaml
# ServiceMonitor定义
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: app-monitor
  labels:
    release: prometheus
spec:
  selector:
    matchLabels:
      app: myapp
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
  namespaceSelector:
    matchNames:
    - production
---
# PodMonitor定义
apiVersion: monitoring.coreos.com/v1
kind: PodMonitor
metadata:
  name: app-pod-monitor
spec:
  selector:
    matchLabels:
      app: myapp
  podMetricsEndpoints:
  - port: metrics
    interval: 30s
```

<!-- chunk: 指标聚合规则 -->
## 指标聚合规则

```yaml
# PrometheusRule定义
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: app-rules
spec:
  groups:
  - name: app.rules
    interval: 30s
    rules:
    # 记录规则(预计算)
    - record: job:http_requests:rate5m
      expr: sum(rate(http_requests_total[5m])) by (job)
    # 告警规则
    - alert: HighErrorRate
      expr: |
        sum(rate(http_requests_total{status=~"5.."}[5m])) by (job)
        /
        sum(rate(http_requests_total[5m])) by (job) > 0.05
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High error rate detected"
        description: "Error rate is {{ $value | humanizePercentage }}"
```

<!-- chunk: 验证自定义指标 -->
## 验证自定义指标

```bash
# 验证Custom Metrics API
kubectl get --raw "/apis/custom.metrics.k8s.io/v1beta1" | jq
kubectl get --raw "/apis/custom.metrics.k8s.io/v1beta1/namespaces/default/pods/*/http_requests_per_second" | jq

# 验证External Metrics API
kubectl get --raw "/apis/external.metrics.k8s.io/v1beta1" | jq
kubectl get --raw "/apis/external.metrics.k8s.io/v1beta1/namespaces/default/queue_messages" | jq

# 检查HPA状态
kubectl describe hpa <name>
kubectl get hpa -w
```

<!-- chunk: ACK监控扩展 -->
## ACK监控扩展

| 功能 | 产品 | 集成方式 |
|-----|------|---------|
| **Prometheus托管** | ARMS | 组件安装 |
| **自定义指标HPA** | ARMS Adapter | 自动配置 |
| **业务监控** | ARMS应用监控 | Agent注入 |
| **日志指标** | SLS | 日志聚合 |

---

**监控扩展原则**: 暴露业务指标，配置合理阈值，实现自动扩缩容

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- observability/MOC.md|domain-06-observability MOC]]
- [[domain-06-observability/README.md|Observability Domain (可观测性领域)]]
- [[domain-06-observability/00-open-source-projects-index.md|Domain-8 可观测性 — 开源项目索引]]
- Kubernetes 可观测性架构体系
- 指标监控体系详解
- 03 - 日志收集架构详解 (Logging Architecture)
- 分布式追踪体系
- 05 - 告警管理策略 (Alerting Management)
- 06 - 监控告警实战与最佳实践 (Monitoring Alerting Practice)
- 04 - 监控仪表板设计与最佳实践 (Monitoring Dashboards)
- 08 - 日志审计与合规管理 (Logging Auditing & Compliance)
- 05 - 事件与审计日志管理 (Events & Audit Logs)

## See Also

- 09-events-audit-logs
- 10-monitoring-metrics-prometheus
- 12-logging-auditing
- 13-cluster-health-check

- [[domain-06-observability/README.md|返回目录]]

## Related

- [[domain-19-landscape-references/topic-index/observability-index.md|Observability 可观测性知识图谱索引]]
