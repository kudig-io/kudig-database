# 04-企业级监控体系

> **适用范围**: Kubernetes v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

## 📋 概述

企业级监控体系是保障Kubernetes生产环境稳定运行的核心基础设施。本文档详细介绍完整的监控架构设计、组件选型和最佳实践。

## 🏗️ 监控架构设计

### 三层监控架构

#### 1. 基础设施层监控
```yaml
# Node Exporter DaemonSet配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-exporter
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: node-exporter
  template:
    metadata:
      labels:
        app: node-exporter
    spec:
      hostNetwork: true
      hostPID: true
      containers:
      - name: node-exporter
        image: quay.io/prometheus/node-exporter:v1.5.0
        args:
        - --web.listen-address=:9100
        - --path.procfs=/host/proc
        - --path.sysfs=/host/sys
        - --collector.filesystem.mount-points-exclude=^/(dev|proc|sys|var/lib/docker/.+)($|/)
        - --collector.filesystem.fs-types-exclude=^(autofs|binfmt_misc|cgroup|configfs|debugfs|devpts|devtmpfs|fusectl|hugetlbfs|mqueue|overlay|proc|procfs|pstore|rpc_pipefs|securityfs|sysfs|tracefs)$
        ports:
        - containerPort: 9100
        volumeMounts:
        - name: proc
          mountPath: /host/proc
          readOnly: true
        - name: sys
          mountPath: /host/sys
          readOnly: true
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
      volumes:
      - name: proc
        hostPath:
          path: /proc
      - name: sys
        hostPath:
          path: /sys
```

#### 2. Kubernetes组件监控
```yaml
# kube-state-metrics配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: kube-state-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: kube-state-metrics
  replicas: 2
  template:
    metadata:
      labels:
        app: kube-state-metrics
    spec:
      containers:
      - name: kube-state-metrics
        image: registry.k8s.io/kube-state-metrics/kube-state-metrics:v2.7.0
        ports:
        - containerPort: 8080
          name: http-metrics
        - containerPort: 8081
          name: telemetry
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 5
          timeoutSeconds: 5
        readinessProbe:
          httpGet:
            path: /
            port: 8081
          initialDelaySeconds: 5
          timeoutSeconds: 5
---
apiVersion: v1
kind: Service
metadata:
  name: kube-state-metrics
  namespace: monitoring
  labels:
    app: kube-state-metrics
spec:
  ports:
  - name: http-metrics
    port: 8080
    targetPort: http-metrics
  - name: telemetry
    port: 8081
    targetPort: telemetry
  selector:
    app: kube-state-metrics
```

#### 3. 应用层监控
```yaml
# 应用监控Sidecar模式
apiVersion: apps/v1
kind: Deployment
metadata:
  name: monitored-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "8080"
        prometheus.io/path: "/metrics"
    spec:
      containers:
      - name: app
        image: myapp:latest
        ports:
        - containerPort: 8080
        env:
        - name: METRICS_ENABLED
          value: "true"
```

## 📊 Prometheus监控栈

### 核心组件配置

#### 1. Prometheus Server配置
```yaml
# Prometheus配置文件
global:
  scrape_interval: 15s
  evaluation_interval: 15s
  external_labels:
    cluster: production
    region: us-west-2

rule_files:
  - "rules/alerts.yml"
  - "rules/recording.yml"

alerting:
  alertmanagers:
  - static_configs:
    - targets:
      - alertmanager.monitoring.svc:9093

scrape_configs:
  # Kubernetes节点监控
  - job_name: 'kubernetes-nodes'
    kubernetes_sd_configs:
    - role: node
    relabel_configs:
    - source_labels: [__address__]
      regex: '(.*):10250'
      target_label: __address__
      replacement: '${1}:9100'
    - target_label: __scheme__
      replacement: http

  # Kubernetes Pods监控
  - job_name: 'kubernetes-pods'
    kubernetes_sd_configs:
    - role: pod
    relabel_configs:
    - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_scrape]
      action: keep
      regex: true
    - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_path]
      action: replace
      target_label: __metrics_path__
      regex: (.+)
    - source_labels: [__address__, __meta_kubernetes_pod_annotation_prometheus_io_port]
      action: replace
      regex: ([^:]+)(?::\d+)?;(\d+)
      replacement: $1:$2
      target_label: __address__

  # kube-state-metrics
  - job_name: 'kube-state-metrics'
    static_configs:
    - targets: ['kube-state-metrics:8080']
```

#### 2. 长期存储配置
```yaml
# Thanos Sidecar配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: prometheus-thanos
  namespace: monitoring
spec:
  replicas: 1
  selector:
    matchLabels:
      app: prometheus-thanos
  template:
    metadata:
      labels:
        app: prometheus-thanos
    spec:
      containers:
      - name: thanos-sidecar
        image: quay.io/thanos/thanos:v0.30.0
        args:
        - sidecar
        - --http-address=0.0.0.0:10902
        - --grpc-address=0.0.0.0:10901
        - --prometheus.url=http://localhost:9090
        - --objstore.config-file=/etc/thanos/objstore.yml
        - --tsdb.path=/prometheus
        ports:
        - name: http
          containerPort: 10902
        - name: grpc
          containerPort: 10901
        volumeMounts:
        - name: prometheus-storage
          mountPath: /prometheus
        - name: thanos-config
          mountPath: /etc/thanos
      volumes:
      - name: prometheus-storage
        persistentVolumeClaim:
          claimName: prometheus-pvc
      - name: thanos-config
        configMap:
          name: thanos-objstore-config
```

### 告警规则配置

#### 1. 核心告警规则
```yaml
# 核心告警规则
groups:
- name: kubernetes.rules
  rules:
  # 节点相关告警
  - alert: NodeDown
    expr: up{job="kubernetes-nodes"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Node {{ $labels.instance }} is down"
      description: "Node has been down for more than 5 minutes"

  - alert: NodeCPUHigh
    expr: 100 - (avg by(instance) (irate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 85
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "High CPU usage on node {{ $labels.instance }}"
      description: "CPU usage is above 85% for more than 10 minutes"

  # Pod相关告警
  - alert: PodCrashLooping
    expr: rate(kube_pod_container_status_restarts_total[15m]) * 60 * 5 > 0
    for: 15m
    labels:
      severity: critical
    annotations:
      summary: "Pod {{ $labels.pod }} is crash looping"
      description: "Pod is restarting more than 5 times per hour"

  - alert: PodPending
    expr: kube_pod_status_phase{phase="Pending"} == 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Pod {{ $labels.pod }} is pending"
      description: "Pod has been in Pending state for more than 10 minutes"
```

## 🎨 Grafana可视化

### 核心仪表板配置

#### 1. 集群概览仪表板
```json
{
  "dashboard": {
    "title": "Kubernetes Cluster Overview",
    "panels": [
      {
        "title": "Cluster Health Status",
        "type": "stat",
        "datasource": "Prometheus",
        "targets": [
          {
            "expr": "sum(up{job=\"kubernetes-nodes\"})",
            "legendFormat": "Nodes Up"
          },
          {
            "expr": "count(kube_pod_info)",
            "legendFormat": "Total Pods"
          },
          {
            "expr": "sum(kube_deployment_status_replicas_available)",
            "legendFormat": "Available Deployments"
          }
        ]
      },
      {
        "title": "Resource Utilization",
        "type": "graph",
        "datasource": "Prometheus",
        "targets": [
          {
            "expr": "100 * sum(kube_pod_container_resource_requests{resource=\"cpu\"}) / sum(kube_node_status_allocatable{resource=\"cpu\"})",
            "legendFormat": "CPU Requested %"
          },
          {
            "expr": "100 * sum(kube_pod_container_resource_limits{resource=\"cpu\"}) / sum(kube_node_status_allocatable{resource=\"cpu\"})",
            "legendFormat": "CPU Limits %"
          }
        ]
      }
    ]
  }
}
```

#### 2. 应用性能仪表板
```json
{
  "dashboard": {
    "title": "Application Performance",
    "panels": [
      {
        "title": "HTTP Request Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(http_requests_total[5m])) by (app)",
            "legendFormat": "{{app}}"
          }
        ]
      },
      {
        "title": "Error Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(http_requests_total{status=~\"5..\"}[5m])) by (app) / sum(rate(http_requests_total[5m])) by (app) * 100",
            "legendFormat": "{{app}} Error %"
          }
        ]
      },
      {
        "title": "Latency Distribution",
        "type": "heatmap",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(http_request_duration_seconds_bucket[5m]))",
            "legendFormat": "p95 Latency"
          }
        ]
      }
    ]
  }
}
```

## 🚨 Alertmanager告警管理

### 告警路由配置

#### 1. 多级告警路由
```yaml
# Alertmanager配置
global:
  smtp_smarthost: 'smtp.example.com:587'
  smtp_from: 'alerts@example.com'
  smtp_auth_username: 'alerts'
  smtp_auth_password: 'password'

route:
  group_by: ['alertname', 'cluster']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 3h
  receiver: 'default-receiver'
  
  routes:
  - match:
      severity: critical
    receiver: 'pagerduty'
    group_wait: 10s
    repeat_interval: 1h
    
  - match:
      severity: warning
    receiver: 'slack-warning'
    group_wait: 1m
    
  - match:
      team: sre
    receiver: 'sre-team'
    routes:
    - match:
        service: database
      receiver: 'db-team'

receivers:
- name: 'default-receiver'
  email_configs:
  - to: 'team@example.com'
    send_resolved: true

- name: 'pagerduty'
  pagerduty_configs:
  - service_key: 'YOUR_PAGERDUTY_KEY'
    send_resolved: true

- name: 'slack-warning'
  slack_configs:
  - api_url: 'https://hooks.slack.com/services/YOUR/SLACK/WEBHOOK'
    channel: '#alerts-warning'
    send_resolved: true
    title: '{{ template "slack.warning.title" . }}'
    text: '{{ template "slack.warning.text" . }}'
```

#### 2. 告警抑制规则
```yaml
# 告警抑制配置
inhibit_rules:
- source_match:
    alertname: 'NodeDown'
  target_match:
    alertname: 'ServiceDown'
  equal: ['instance']

- source_match:
    alertname: 'ClusterDown'
  target_match_re:
    alertname: '.*'
  equal: ['cluster']
```

## 📈 性能优化

### 监控系统调优

#### 1. Prometheus性能优化
```yaml
# Prometheus存储优化配置
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: prometheus
spec:
  template:
    spec:
      containers:
      - name: prometheus
        image: prom/prometheus:v2.40.0
        args:
        - --storage.tsdb.retention.time=30d
        - --storage.tsdb.retention.size=50GB
        - --storage.tsdb.wal-compression
        - --web.enable-lifecycle
        - --web.enable-admin-api
        - --query.max-concurrency=20
        - --query.timeout=2m
        resources:
          requests:
            cpu: 2
            memory: 8Gi
          limits:
            cpu: 4
            memory: 16Gi
```

#### 2. 查询优化策略
```yaml
# Recording规则优化
groups:
- name: recording.rules
  rules:
  # 预计算高频查询
  - record: job:node_cpu_utilization:avg5m
    expr: avg by(job) (rate(node_cpu_seconds_total{mode!="idle"}[5m]))
    
  - record: cluster:memory_utilization:ratio
    expr: sum(node_memory_MemTotal_bytes - node_memory_MemAvailable_bytes) / sum(node_memory_MemTotal_bytes)
    
  # 聚合降采样
  - record: instance:network_bytes:rate1m
    expr: rate(node_network_receive_bytes_total[1m]) + rate(node_network_transmit_bytes_total[1m])
```

## 🔧 实施检查清单

### 监控体系建设
- [ ] 设计完整的三层监控架构
- [ ] 部署核心监控组件(Prometheus、Grafana、Alertmanager)
- [ ] 配置基础设施层监控(Node Exporter、kube-state-metrics)
- [ ] 实现应用层监控集成
- [ ] 建立完善的告警规则体系
- [ ] 配置多渠道告警通知

### 性能与可靠性
- [ ] 优化Prometheus存储和查询性能
- [ ] 实施监控数据长期存储方案
- [ ] 配置监控系统的高可用部署
- [ ] 建立监控数据备份和恢复机制
- [ ] 实施监控系统容量规划
- [ ] 定期审查和优化告警规则

### 运营维护
- [ ] 建立监控仪表板标准化模板
- [ ] 实施监控数据质量监控
- [ ] 建立告警响应和处理流程
- [ ] 定期进行监控系统健康检查
- [ ] 维护监控文档和操作手册
- [ ] 持续改进监控覆盖范围

---

*本文档为企业级Kubernetes监控体系提供全面的技术指导和实施框架*