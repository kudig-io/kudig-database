---
title: Kubernetes v1.29-v1.33 可观测性新特性指南
description: 'title: Kubernetes v1.29-v1.33 可观测性新特性指南'
category: general
tags:
- k8s
- observability
- prometheus
- monitoring
- guide
- apiserver
- kubelet
- scheduler
- controller-manager
- grafana
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Kubernetes是什么？
- 如何使用Kubernetes？
- Kubernetes的最佳实践是什么？
trigger_keywords:
- Kubernetes
- v1.29-v1.33
- 可观测性新特性指南
- observability
prerequisites:
- kubectl-basics
- observability-basics
- prometheus-basics
- monitoring-basics
- cilium-basics
- cni-basics
- logging-basics
- tracing-basics
---

title: Kubernetes v1.29-v1.33 可观测性新特性指南
description: '# Kubernetes v1.29-v1.33 可观测性新特性指南'
category: observability
tags:
- k8s
- observability
- monitoring
- logging
- tracing
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 监控工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes v1.29-v1.33 可观测性新特性指南 是什么
- 如何 Kubernetes v1.29-v1.33 可观测性新特性指南
- Kubernetes 8 observability 最佳实践
trigger_keywords:
- Kubernetes
- v1.29-v1.33
- 可观测性新特性指南
- observability
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
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Kubernetes v1.29-v1.33 可观测性新特性指南

> **适用版本**: Kubernetes v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **用途**: 可观测性体系新特性详解与集成实践

---

<!-- chunk: 📋 目录 -->
## 📋 目录

- [一、Kubelet OpenTelemetry Tracing (v1.31 GA)](#一kubelet-opentelemetry-tracing-v131-ga)
- [二、Kubelet Resource Metrics (v1.33 Beta)](#二kubelet-resource-metrics-v133-beta)
- [三、Structured Logging 增强](#三structured-logging-增强)
- [四、Node Log Query (v1.30 Alpha)](#四node-log-query-v130-alpha)
- [五、Pod 级资源指标与监控](#五pod-级资源指标与监控)
- [六、Event 流式传输优化](#六event-流式传输优化)
- [七、生产可观测性架构建议](#七生产可观测性架构建议)

---

<!-- chunk: 一、Kubelet OpenTelemetry Tracing (v1.31 GA) -->
## 一、Kubelet OpenTelemetry Tracing (v1.31 GA)

### 1.1 核心概念

KubeletTracing 将分布式追踪能力内建到 Kubelet 中，通过 OTLP 协议导出追踪数据。

### 1.2 架构

```
用户创建 Pod
    │
    ▼
APIServer ──Span──► Kubelet ──Span──► CRI (containerd/CRI-O)
    │                    │
    │                    ├── Span ──► CNI (Cilium/Calico)
    │                    │
    │                    └── Span ──► CSI (存储驱动)
    │
    └── 所有 Span 通过 OTLP 导出
                │
                ▼
        OpenTelemetry Collector
                │
        ┌───────┴───────┐
        ▼               ▼
    Jaeger/Tempo    Prometheus/Grafana
```

### 1.3 Kubelet 配置

```yaml
# /var/lib/kubelet/config.yaml
featureGates:
  KubeletTracing: true  # v1.31 GA，默认启用

tracing:
  endpoint: "localhost:4317"  # OTLP gRPC 端点
  samplingRatePerMillion: 100000  # 10% 采样率
```

### 1.4 验证追踪数据

```bash
# 检查 Kubelet 是否导出追踪
curl -s http://localhost:10248/healthz?verbose | grep tracing

# 查看追踪端点配置
kubectl get --raw /api/v1/nodes/NODE_NAME/proxy/configz | jq '.kubeletconfig.tracing'
```

### 1.5 追踪上下文传播

```yaml
# Pod 配置：启用追踪上下文注入
apiVersion: v1
kind: Pod
metadata:
  name: traced-app
spec:
  containers:
    - name: app
      image: myapp:v1.0
      env:
        # 通过 Downward API 注入追踪信息
        - name: OTEL_SERVICE_NAME
          value: "myapp"
        - name: OTEL_EXPORTER_OTLP_ENDPOINT
          value: "http://otel-collector.monitoring:4317"
```

---

<!-- chunk: 二、Kubelet Resource Metrics (v1.33 Beta) -->
## 二、Kubelet Resource Metrics (v1.33 Beta)

### 2.1 核心概念

提供标准化的节点资源利用率指标，无需依赖 metrics-server 即可获取核心资源使用情况。

### 2.2 指标端点

```
# Kubelet 资源指标端点
GET /metrics/resource

返回指标:
├── container_cpu_usage_seconds_total
├── container_memory_working_set_bytes
├── pod_cpu_usage_seconds_total
├── pod_memory_working_set_bytes
├── node_cpu_usage_seconds_total
├── node_memory_working_set_bytes
├── container_start_time_seconds
└── pod_start_time_seconds
```

### 2.3 启用配置

```yaml
# /var/lib/kubelet/config.yaml
featureGates:
  KubeletResourceMetrics: true  # v1.33 Beta，默认启用
```

### 2.4 Prometheus 抓取配置

```yaml
# Prometheus ServiceMonitor
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: kubelet-resource-metrics
  namespace: monitoring
spec:
  endpoints:
    - bearerTokenFile: /var/run/secrets/kubernetes.io/serviceaccount/token
      honorLabels: true
      interval: 15s
      path: /metrics/resource
      port: https-metrics
      scheme: https
      tlsConfig:
        insecureSkipVerify: true
  namespaceSelector:
    matchNames:
      - kube-system
  selector:
    matchLabels:
      k8s-app: kubelet
```

### 2.5 与 metrics-server 对比

| 特性 | metrics-server | KubeletResourceMetrics |
|:---|:---|:---|
| 数据来源 | Summary API (/stats/summary) | 专用端点 (/metrics/resource) |
| 数据格式 | JSON | Prometheus 格式 |
| 精度 | 累计值 | 标准化 counter/gauge |
| 开销 | 较高（JSON 序列化） | 较低 |
| 依赖 | 必须部署 | 内建于 kubelet |

---

<!-- chunk: 三、Structured Logging 增强 -->
## 三、Structured Logging 增强

### 3.1 核心概念

v1.29+ 逐步推进结构化日志，将传统文本日志迁移到键值对格式。

### 3.2 日志格式对比

```
传统日志 (text):
I0424 10:30:15.123456    1234 controller.go:456] "Pod created" pod="default/nginx-123"

结构化日志 (json):
{
  "ts": 1713957015.123456,
  "level": "info",
  "caller": "controller.go:456",
  "msg": "Pod created",
  "pod": "default/nginx-123",
  "controller": "deployment",
  "reconcileID": "abc-123-def"
}
```

### 3.3 组件配置

```bash
# API Server
kube-apiserver --logging-format=json

# Controller Manager
kube-controller-manager --logging-format=json

# Scheduler
kube-scheduler --logging-format=json

# Kubelet
# /var/lib/kubelet/config.yaml
logging:
  format: json
  verbosity: 2
```

### 3.4 Fluent Bit 解析配置

```yaml
# fluent-bit-config.yaml
[PARSER]
    Name        k8s-json
    Format      json
    Time_Key    ts
    Time_Format %s.%L
    Time_Keep   On

[FILTER]
    Name                kubernetes
    Match               kube.*
    Kube_URL            https://kubernetes.default.svc:443
    Kube_CA_File        /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
    Kube_Token_File     /var/run/secrets/kubernetes.io/serviceaccount/token
```

---

<!-- chunk: 四、Node Log Query (v1.30 Alpha) -->
## 四、Node Log Query (v1.30 Alpha)

### 4.1 核心概念

通过 kubectl 直接查询节点上的系统服务日志，无需 SSH 登录节点。

### 4.2 启用配置

```yaml
# /var/lib/kubelet/config.yaml
featureGates:
  NodeLogQuery: true
```

### 4.3 查询命令

```bash
# 查询所有节点的 kubelet 日志
kubectl node-logs --all-nodes --query="kubelet"

# 查询特定节点的 systemd 服务日志
kubectl node-logs node-1 --service=kubelet --since=1h

# 查询内核日志
kubectl node-logs node-1 --query="kernel"

# 查询容器运行时日志
kubectl node-logs node-1 --service=containerd
```

### 4.4 RBAC 配置

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: node-log-reader
rules:
  - apiGroups: [""]
    resources: ["nodes/log"]
    verbs: ["get"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: node-log-reader-binding
subjects:
  - kind: Group
    name: oncall-engineers
    apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: node-log-reader
  apiGroup: rbac.authorization.k8s.io
```

---

<!-- chunk: 五、Pod 级资源指标与监控 -->
## 五、Pod 级资源指标与监控

### 5.1 资源利用率监控

```yaml
# PrometheusRule: Pod 资源告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: pod-resource-alerts
  namespace: monitoring
spec:
  groups:
    - name: pod-resources
      rules:
        # CPU 使用率告警
        - alert: PodHighCPUUsage
          expr: |
            (
              rate(container_cpu_usage_seconds_total[5m])
              /
              kube_pod_container_resource_limits{resource="cpu"}
            ) > 0.8
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Pod {{ $labels.pod }} CPU 使用率超过 80%"
            
        # 内存使用率告警
        - alert: PodHighMemoryUsage
          expr: |
            (
              container_memory_working_set_bytes
              /
              kube_pod_container_resource_limits{resource="memory"}
            ) > 0.9
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "Pod {{ $labels.pod }} 内存使用率超过 90%"
```

### 5.2 基于 KubeletResourceMetrics 的 Dashboard

```json
{
  "dashboard": {
    "title": "Kubernetes Resource Utilization (v1.33+)",
    "panels": [
      {
        "title": "Node CPU Usage",
        "targets": [
          {
            "expr": "rate(node_cpu_usage_seconds_total[5m])",
            "legendFormat": "{{node}}"
          }
        ]
      },
      {
        "title": "Pod Memory Working Set",
        "targets": [
          {
            "expr": "pod_memory_working_set_bytes",
            "legendFormat": "{{pod}}"
          }
        ]
      }
    ]
  }
}
```

---

<!-- chunk: 六、Event 流式传输优化 -->
## 六、Event 流式传输优化

### 6.1 核心概念

v1.29+ 优化了 Event API 的性能，支持更高效的流式传输。

### 6.2 Event 查询优化

```bash
# 使用 field-selector 高效筛选
kubectl get events --field-selector reason=FailedScheduling

# 按时间范围查询
kubectl get events --sort-by='.lastTimestamp' | tail -50

# 使用 watch 流式监控
kubectl get events --watch --field-selector type=Warning
```

### 6.3 Event 持久化建议

```yaml
# Event Exporter 配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: kubernetes-event-exporter
  namespace: monitoring
spec:
  replicas: 1
  selector:
    matchLabels:
      app: event-exporter
  template:
    metadata:
      labels:
        app: event-exporter
    spec:
      containers:
        - name: exporter
          image: ghcr.io/resmoio/kubernetes-event-exporter:v1.7
          args:
            - --config=/config/config.yaml
          volumeMounts:
            - name: config
              mountPath: /config
      volumes:
        - name: config
          configMap:
            name: event-exporter-config
---
# 输出到 Loki
apiVersion: v1
kind: ConfigMap
metadata:
  name: event-exporter-config
data:
  config.yaml: |
    logLevel: info
    logFormat: json
    route:
      routes:
        - match:
            - receiver: loki
    receivers:
      - name: loki
        loki:
          url: http://loki.monitoring:3100/loki/api/v1/push
          streamLabels:
            source: kubernetes-event-exporter
```

---

<!-- chunk: 七、生产可观测性架构建议 -->
## 七、生产可观测性架构建议

### 7.1 v1.33 推荐架构

```
┌─────────────────────────────────────────────────────────────┐
│                     采集层 (Agent)                           │
├─────────────┬─────────────┬─────────────┬───────────────────┤
│  Prometheus │  Fluent Bit │  OTel Agent │  Event Exporter   │
│  (Metrics)  │  (Logs)     │  (Traces)   │  (Events)         │
└──────┬──────┴──────┬──────┴──────┬──────┴─────────┬─────────┘
       │             │             │                │
       ▼             ▼             ▼                ▼
┌─────────────────────────────────────────────────────────────┐
│                    处理层 (Pipeline)                         │
├─────────────┬─────────────┬─────────────────────────────────┤
│  Prometheus │  Loki       │  Tempo/Jaeger                   │
│  (TSDB)     │  (Log Store)│  (Trace Store)                  │
└──────┬──────┴──────┬──────┴────────────────┬────────────────┘
       │             │                       │
       ▼             ▼                       ▼
┌─────────────────────────────────────────────────────────────┐
│                    展示层 (Visualization)                    │
├─────────────────────────────────────────────────────────────┤
│                      Grafana Unified                         │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────────┐    │
│  │ Metrics │  │ Logs    │  │ Traces  │  │ Alerting    │    │
│  │ Dashboard│  │ Explore │  │ Explore │  │ Rules       │    │
│  └─────────┘  └─────────┘  └─────────┘  └─────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

### 7.2 关键指标清单

| 层级 | 指标 | 来源 | 告警阈值 |
|:---|:---|:---|:---|
| 集群 | 节点就绪率 | kube_node_status_condition | < 90% |
| 集群 | API Server 请求延迟 | apiserver_request_duration_seconds | P99 > 1s |
| 节点 | CPU 使用率 | node_cpu_usage_seconds_total | > 80% |
| 节点 | 内存使用率 | node_memory_working_set_bytes | > 90% |
| Pod | CPU 使用率 | container_cpu_usage_seconds_total | > limits 的 80% |
| Pod | 内存使用率 | container_memory_working_set_bytes | > limits 的 90% |
| Pod | 重启次数 | kube_pod_container_status_restarts_total | > 3/小时 |
| 存储 | PVC 使用率 | kubelet_volume_stats_used_bytes | > 85% |

### 7.3 版本特性启用检查清单

```bash
#!/bin/bash
# check-observability-features.sh

echo "=== K8s v1.33 可观测性特性检查 ==="

# 1. Kubelet Tracing
echo "[1] Kubelet Tracing (GA v1.31)"
kubectl get --raw /api/v1/nodes/$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')/proxy/configz | jq -r '.kubeletconfig.tracing.endpoint // "未配置"'

# 2. Kubelet Resource Metrics
echo "[2] Kubelet Resource Metrics (Beta v1.33)"
curl -sk https://$(kubectl get nodes -o jsonpath='{.items[0].status.addresses[0].address}'):10250/metrics/resource --header "Authorization: Bearer $(kubectl get secrets -n kube-system -o jsonpath='{.items[?(@.type=="kubernetes.io/service-account-token")].data.token}' | base64 -d)" 2>/dev/null | head -5 || echo "无法访问"

# 3. 结构化日志
echo "[3] 结构化日志格式"
kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[0].spec.containers[0].command}' | grep -o 'logging-format=[^,}]*' || echo "默认 text"

# 4. Node Log Query
echo "[4] Node Log Query (Alpha v1.30)"
kubectl get --raw /api/v1/nodes/$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')/proxy/configz | jq -r '.kubeletconfig.featureGates.NodeLogQuery // "未启用"'

echo "=== 检查完成 ==="
```

---

<!-- chunk: 参考链接 -->
## 参考链接

- [KEP-2831: Kubelet Tracing](https://github.com/kubernetes/enhancements/tree/master/keps/sig-instrumentation/2831-kubelet-tracing)
- [KEP-727: Kubelet Resource Metrics](https://github.com/kubernetes/enhancements/tree/master/keps/sig-instrumentation/727-resource-metrics)
- [Structured Logging](https://kubernetes.io/docs/concepts/cluster-administration/system-logs/)
- [Node Log Query](https://kubernetes.io/docs/concepts/cluster-administration/node-log-query/)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- [[domain-06-observability/MOC.md|domain-06-observability MOC]]
- [[domain-06-observability/README.md|Observability Domain (可观测性领域)]]
- [[domain-06-observability/00-open-source-projects-index.md|Domain-8 可观测性 — 开源项目索引]]
- [[domain-06-observability/01-observability-architecture-overview.md|Kubernetes 可观测性架构体系]]
- [[domain-06-observability/02-monitoring-metrics-system.md|指标监控体系详解]]
- [[domain-06-observability/03-logging-architecture.md|03 - 日志收集架构详解 (Logging Architecture)]]
- [[domain-06-observability/04-distributed-tracing.md|分布式追踪体系]]
- [[domain-06-observability/05-alerting-management.md|05 - 告警管理策略 (Alerting Management)]]
- [[domain-06-observability/06-monitoring-alerting-practice.md|06 - 监控告警实战与最佳实践 (Monitoring Alerting Practice)]]
- [[domain-06-observability/07-monitoring-dashboards.md|04 - 监控仪表板设计与最佳实践 (Monitoring Dashboards)]]
- [[domain-06-observability/08-logging-audit-compliance.md|08 - 日志审计与合规管理 (Logging Auditing & Compliance)]]
- [[domain-06-observability/09-events-audit-logs.md|05 - 事件与审计日志管理 (Events & Audit Logs)]]

## Related

- [[release-notes/12-demo-env-guide.md|12-demo-env-guide]]
- [[release-notes/21-platform-selection-guide.md|21-platform-selection-guide]]
- [[domain-02-workloads-applications/07-java-observability-kubernetes.md|07-java-observability-kubernetes]]

- [[domain-06-observability/README.md|返回目录]]- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]

## See Also

- [[domain-06-observability/27-performance-profiling-tools.md|27-performance-profiling-tools]]
- [[domain-06-observability/99-java-observability-kubernetes-guide.md|99-java-observability-kubernetes-guide]]
- [[domain-06-observability/FINAL-QUALITY-ASSESSMENT.md|FINAL-QUALITY-ASSESSMENT]]
- [[domain-06-observability/QUALITY-REPORT.md|QUALITY-REPORT]]
