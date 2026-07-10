---
title: 监控告警体系
description: 深入解析企业级监控告警体系：Prometheus/Grafana 深度集成、SLO/SLI/SLA 工程实践、告警收敛与静默、On-Call
  值班流程与 MTTR 优化
summary: 深入解析企业级监控告警体系：Prometheus/Grafana 深度集成、SLO/SLI/SLA 工程实践、告警收敛与静默、On-Call 值班流程与
  MTTR 优化
category: domain-07-platform-engineering
tags:
- k8s
- monitoring
- alerting
- prometheus
- grafana
- slo
- sli
- sla
- oncall
- mttr
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 监控告警体系 是什么
- 如何 监控告警体系
- Kubernetes 9 platform ops 最佳实践
trigger_keywords:
- 监控告警体系
- platform
- ops
prerequisites:
- kubectl-basics
- platform-engineering-basics
- prometheus-basics
- monitoring-basics
- logging-basics
- tracing-basics
k8s_versions:
- '1.25'
- '1.26'
- '1.27'
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
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: domain
  path: ../domain-15-specialized-tech/
  label: '相关知识域: domain-15-specialized-tech'
- type: domain
  path: ../domain-10-troubleshooting-diagnostics/
  label: '相关知识域: domain-10-troubleshooting-diagnostics'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/monitoring-fta.md
  label: '故障树: monitoring'
related_docs:
- path: 01-platform-ops-overview.md
  type: depth
  desc: 平台运维概述
- path: 02-cluster-lifecycle-management.md
  type: depth
  desc: 集群生命周期管理
- path: ../domain-06-observability/02-monitoring-metrics-system.md
  type: depth
  desc: 指标监控体系
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 监控告警体系 (Monitoring & Alerting System)

<!-- chunk: 概述 -->
## 概述

监控告警体系是平台运维的眼睛和耳朵，通过全面的数据采集、智能的分析告警和可视化的展示，确保平台的稳定运行和快速问题响应。

<!-- chunk: 核心组件架构 -->
## 核心组件架构

### 1. 数据采集层
```
指标采集 → 日志收集 → 链路追踪 → 事件监控
```

### 2. 数据处理层
```
数据清洗 → 格式转换 → 聚合计算 → 存储持久化
```

### 3. 分析告警层
```
异常检测 → 根因分析 → 告警生成 → 通知分发
```

### 4. 展示管理层
```
仪表板 → 报表分析 → 历史查询 → 决策支持
```

<!-- chunk: Prometheus监控系统 -->
## Prometheus监控系统

### 核心组件
```yaml
# Prometheus部署配置
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: prometheus
spec:
  serviceName: prometheus
  replicas: 3
  selector:
    matchLabels:
      app: prometheus
  template:
    metadata:
      labels:
        app: prometheus
    spec:
      containers:
      - name: prometheus
        image: prom/prometheus:v3.2.1
        args:
        - '--config.file=/etc/prometheus/prometheus.yml'
        - '--storage.tsdb.path=/prometheus'
        - '--web.console.libraries=/etc/prometheus/console_libraries'
        - '--web.console.templates=/etc/prometheus/consoles'
        - '--storage.tsdb.retention.time=30d'
        ports:
        - containerPort: 9090
        volumeMounts:
        - name: prometheus-config
          mountPath: /etc/prometheus
        - name: prometheus-storage
          mountPath: /prometheus
```

### 服务发现配置
```yaml
# Kubernetes服务发现
scrape_configs:
- job_name: 'kubernetes-nodes'
  kubernetes_sd_configs:
  - role: node
  relabel_configs:
  - source_labels: [__address__]
    regex: '(.*):10250'
    target_label: __address__
    replacement: '${1}:9100'

- job_name: 'kubernetes-pods'
  kubernetes_sd_configs:
  - role: pod
  relabel_configs:
  - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_scrape]
    action: keep
    regex: true
```

### 关键监控指标

#### 集群级别指标
```promql
# 集群健康状态
up{job="kubernetes-apiservers"} == 1
kube_node_status_condition{condition="Ready",status="true"} == 1

# 资源使用率
sum(kube_pod_container_resource_requests{resource="cpu"}) / sum(kube_node_status_allocatable{resource="cpu"})
sum(kube_pod_container_resource_requests{resource="memory"}) / sum(kube_node_status_allocatable{resource="memory"})

# Pod状态统计
count(kube_pod_status_phase{phase="Running"})
count(kube_pod_status_phase{phase="Pending"})
count(kube_pod_status_phase{phase="Failed"})
```

#### 节点级别指标
```promql
# CPU使用率
100 - (avg(rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100)

# 内存使用率
(1 - (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes)) * 100

# 磁盘IO
rate(node_disk_reads_completed_total[5m])
rate(node_disk_writes_completed_total[5m])

# 网络流量
rate(node_network_receive_bytes_total{device!="lo"}[5m])
rate(node_network_transmit_bytes_total{device!="lo"}[5m])
```

#### 应用级别指标
```promql
# HTTP请求延迟
histogram_quantile(0.95, rate(http_request_duration_seconds_bucket[5m]))

# 错误率
rate(http_requests_total{status=~"5.."}[5m]) / rate(http_requests_total[5m])

# 吞吐量
rate(http_requests_total[5m])
```

<!-- chunk: Grafana可视化 -->
## Grafana可视化

### 仪表板设计原则
```json
{
  "dashboard": {
    "title": "Kubernetes Cluster Overview",
    "panels": [
      {
        "title": "集群健康状态",
        "type": "stat",
        "targets": [
          {
            "expr": "sum(up{job=\"kubernetes-apiservers\"})",
            "legendFormat": "API Server在线数"
          }
        ]
      },
      {
        "title": "资源使用趋势",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(kube_pod_container_resource_requests{resource=\"cpu\"})",
            "legendFormat": "CPU请求总量"
          },
          {
            "expr": "sum(kube_pod_container_resource_limits{resource=\"cpu\"})",
            "legendFormat": "CPU限制总量"
          }
        ]
      }
    ]
  }
}
```

### 常用仪表板模板
- **Cluster Overview**: 集群整体状态概览
- **Node Resources**: 节点资源使用情况
- **Pod Performance**: Pod性能指标
- **Network Traffic**: 网络流量分析
- **Storage Usage**: 存储使用统计
- **Application Metrics**: 应用性能监控

<!-- chunk: AlertManager告警管理 -->
## AlertManager告警管理

### 告警规则配置
```yaml
# 告警规则示例
groups:
- name: kubernetes.rules
  rules:
  - alert: KubeAPIDown
    expr: absent(up{job="kubernetes-apiservers"} == 1)
    for: 10m
    labels:
      severity: critical
    annotations:
      summary: "Kubernetes API Server不可用"
      description: "API Server在过去10分钟内无法访问"

  - alert: NodeNotReady
    expr: kube_node_status_condition{condition="Ready",status!="true"} == 1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "节点{{ $labels.node }}未就绪"
      description: "节点状态异常超过5分钟"

  - alert: HighCPUUsage
    expr: (1 - avg(rate(node_cpu_seconds_total{mode="idle"}[5m]))) * 100 > 80
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "节点CPU使用率过高"
      description: "节点{{ $labels.instance }} CPU使用率超过80%"
```

### 通知路由配置
```yaml
# 通知路由
route:
  group_by: ['alertname']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 3h
  receiver: 'default'
  
  routes:
  - matchers:
    - severity="critical"
    receiver: pagerduty
  - matchers:
    - severity="warning"
    receiver: slack
  - matchers:
    - service=~"^(frontend|backend)$"
    receiver: service-team
receivers:
- name: 'default'
  email_configs:
  - to: 'alerts@example.com'
- name: 'pagerduty'
  pagerduty_configs:
  - routing_key: '<pagerduty-service-key>'
- name: 'slack'
  slack_configs:
  - api_url: '<slack-webhook-url>'
    channel: '#alerts'
```

<!-- chunk: 日志收集系统 -->
## 日志收集系统

### Fluentd配置
```xml
<source>
  @type tail
  path /var/log/containers/*.log
  pos_file /var/log/fluentd-containers.log.pos
  tag kubernetes.*
  read_from_head true
  <parse>
    @type json
    time_format %Y-%m-%dT%H:%M:%S.%NZ
  </parse>
</source>

<filter kubernetes.**>
  @type kubernetes_metadata
</filter>

<match kubernetes.var.log.containers.**_nginx_**.log>
  @type elasticsearch
  host elasticsearch
  port 9200
  logstash_format true
  logstash_prefix nginx-access
</match>
```

### Loki日志系统
```yaml
# Loki配置
auth_enabled: false

server:
  http_listen_port: 3100

ingester:
  lifecycler:
    address: 127.0.0.1
    ring:
      kvstore:
        store: inmemory
      replication_factor: 1
  chunk_idle_period: 1h
  max_chunk_age: 1h
  chunk_target_size: 1048576
  chunk_retain_period: 30s

schema_config:
  configs:
  - from: 2020-10-24
    store: boltdb-shipper
    object_store: filesystem
    schema: v11
    index:
      prefix: index_
      period: 24h
```

<!-- chunk: 链路追踪系统 -->
## 链路追踪系统

### Jaeger配置
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: jaeger
spec:
  selector:
    matchLabels:
      app: jaeger
  template:
    metadata:
      labels:
        app: jaeger
    spec:
      containers:
      - name: jaeger
        image: jaegertracing/all-in-one:1.47
        ports:
        - containerPort: 16686
          name: query-http
        - containerPort: 14268
          name: collector-http
        env:
        - name: SPAN_STORAGE_TYPE
          value: badger
        - name: BADGER_EPHEMERAL
          value: "false"
        - name: BADGER_DIRECTORY_VALUE
          value: "/badger/data"
        - name: BADGER_DIRECTORY_KEY
          value: "/badger/key"
        volumeMounts:
        - name: data
          mountPath: /badger
      volumes:
      - name: data
        emptyDir: {}
```

<!-- chunk: SLO/SLI定义 -->
## SLO/SLI定义

### 服务质量指标
```yaml
# SLO配置示例
slos:
- name: api-server-availability
  objective: 99.9
  sli:
    good: up{job="kubernetes-apiservers"} == 1
    total: up{job="kubernetes-apiservers"}
  window: 30d

- name: pod-startup-latency
  objective: 95
  sli:
    good: histogram_quantile(0.95, rate(pod_startup_duration_seconds_bucket[5m])) < 30
    total: rate(pod_startup_duration_seconds_count[5m])
  window: 7d
```

### 告警阈值设定
- **API Server可用性**: < 99.9% 发送critical告警
- **节点就绪率**: < 95% 发送warning告警
- **Pod重启率**: > 10次/小时 发送warning告警
- **CPU使用率**: > 80% 发送warning告警，> 90% 发送critical告警

<!-- chunk: 最佳实践 -->
## 最佳实践

### 1. 分层监控策略
- 基础设施层监控
- 平台服务层监控
- 应用业务层监控
- 用户体验层监控

### 2. 智能告警机制
- 告警去重和抑制
- 告警分级处理
- 根因关联分析
- 自动恢复验证

### 3. 可视化设计原则
- 关键指标突出显示
- 趋势变化清晰可见
- 异常情况及时标红
- 交互操作便捷友好

### 4. 持续优化改进
- 定期审查告警规则
- 优化监控指标覆盖
- 改进告警准确性
- 提升用户体验

通过建立完善的监控告警体系，可以实现对平台状态的全面掌控，快速发现问题并及时响应，为业务稳定运行提供有力保障。

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-07-platform-engineering KUDIG Database — Global MOC
- [[domain-07-platform-engineering/README.md|[[Platform Ops Domain (平台运维领域)|Platform Ops Domain (平台运维领域)]]]]
- Domain-9 平台运维 — 开源项目索引
- 平台运维概述
- 集群生命周期管理
- [[domain-07-platform-engineering/治理/03-capacity-planning-resource-assessment.md|03 capacity planning resource assessment]]
- 性能基准测试与调优 (Performance Benchmarking & Tuning)
- 运维指标体系建设 (Operations Metrics System)
- GitOps配置管理 (GitOps Configuration Management)
- 运维自动化工具链 (Operations Automation Toolchain)
- 成本优化与FinOps实践 (Cost Optimization & FinOps)
- 安全合规管理 (Security & Compliance Management)

## Related

- [[README]]

- 平台运维概述
- 集群生命周期管理
- 指标监控体系
- 相关知识域: domain-06-observability
- 相关知识域: domain-15-specialized-tech
- 相关知识域: domain-10-troubleshooting-diagnostics
- [[domain-19-landscape-references/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]

## See Also

- 04-performance-benchmarking-tuning
- 05-operations-metrics-system
- 07-gitops-configuration-management
- 08-automation-toolchain


<!-- risk-assessed -->
