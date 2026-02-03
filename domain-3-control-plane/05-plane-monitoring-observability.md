# 控制平面监控与可观测性 (Control Plane Monitoring & Observability)

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 监控配置指南

---

## 目录

1. [监控架构设计](#1-监控架构设计)
2. [核心组件监控](#2-核心组件监控)
3. [指标采集配置](#3-指标采集配置)
4. [日志收集分析](#4-日志收集分析)
5. [追踪与链路监控](#5-追踪与链路监控)
6. [告警策略配置](#6-告警策略配置)
7. [可视化仪表板](#7-可视化仪表板)
8. [性能基准建立](#8-性能基准建立)
9. [故障诊断工具](#9-故障诊断工具)
10. [监控最佳实践](#10-监控最佳实践)

---

## 1. 监控架构设计

### 1.1 监控系统架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          Monitoring Architecture                                 │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  数据采集层 (Data Collection Layer)                                             │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Metric Collectors                              │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐│    │
│  │  │ Prometheus  │ │ Node Exporter│ │ Kube-state- │ │ cAdvisor            ││    │
│  │  │ Server      │ │             │ │ metrics     │ │                     ││    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────────┘│    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│          │                                                                      │
│          ▼                                                                      │
│  数据存储层 (Data Storage Layer)                                                │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Time Series DB                                 │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐│    │
│  │  │ Prometheus  │ │ Thanos      │ │ M3DB        │ │ VictoriaMetrics     ││    │
│  │  │             │ │             │ │             │ │                     ││    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────────┘│    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│          │                                                                      │
│          ▼                                                                      │
│  数据分析层 (Data Analysis Layer)                                               │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Alerting Engine                                │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐│    │
│  │  │ Alertmanager│ │ Grafana     │ │ Loki        │ │ Tempo               ││    │
│  │  │             │ │ OnCall      │ │             │ │                     ││    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────────┘│    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│          │                                                                      │
│          ▼                                                                      │
│  展示层 (Visualization Layer)                                                   │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                           Dashboards                                     │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐│    │
│  │  │ Grafana     │ │ Kiali       │ │ Jaeger      │ │ Custom Web UI       ││    │
│  │  │             │ │             │ │             │ │                     ││    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────────────┘│    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 监控数据流

```
监控数据采集流程:

┌─────────────────────────────────────────────────────────────────────────┐
│                           Data Flow                                      │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  1. 指标采集阶段                                                        │
│     Control Plane Components                                            │
│     │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐                 │
│     │  │ API Server  │ │ etcd        │ │ KCM/Sched   │                 │
│     │  │ /metrics    │ │ /metrics    │ │ /metrics    │                 │
│     │  └──────┬──────┘ └──────┬──────┘ └──────┬──────┘                 │
│     │         │               │               │                          │
│     │         ▼               ▼               ▼                          │
│     │  ┌─────────────────────────────────────────────────────────────┐  │
│     │  │                    Service Discovery                        │  │
│     │  │              (Prometheus SD or K8s SD)                      │  │
│     │  └─────────────────────────┬───────────────────────────────────┘  │
│     │                            ▼                                      │
│     │  ┌─────────────────────────────────────────────────────────────┐  │
│     │  │                      Scraping                               │  │
│     │  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐│  │
│     │  │  │ Prometheus  │ │ Pushgateway │ │ Custom Exporters        ││  │
│     │  │  │ Server      │ │             │ │                         ││  │
│     │  │  └─────────────┘ └─────────────┘ └─────────────────────────┘│  │
│     │  └─────────────────────────────────────────────────────────────┘  │
│     │                                                                     │
│     ▼                                                                     │
│  2. 日志收集阶段                                                        │
│     ┌─────────────────────────────────────────────────────────────────┐  │
│     │                           Log Pipeline                           │  │
│     │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │  │
│     │  │ Filebeat    │ │ Fluent Bit  │ │ Promtail    │               │  │
│     │  │             │ │             │ │             │               │  │
│     │  └──────┬──────┘ └──────┬──────┘ └──────┬──────┘               │  │
│     │         │               │               │                        │  │
│     │         ▼               ▼               ▼                        │  │
│     │  ┌─────────────────────────────────────────────────────────────┐│  │
│     │  │                      Log Storage                            ││  │
│     │  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐││  │
│     │  │  │ Elasticsearch│ │ Loki        │ │ Cloud Storage           │││  │
│     │  │  │             │ │             │ │ (S3/GCS)                │││  │
│     │  │  └─────────────┘ └─────────────┘ └─────────────────────────┘││  │
│     │  └─────────────────────────────────────────────────────────────┘│  │
│     │                                                                     │
│     ▼                                                                     │
│  3. 追踪数据收集                                                        │
│     ┌─────────────────────────────────────────────────────────────────┐  │
│     │                         Tracing Pipeline                         │  │
│     │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │  │
│     │  │ OpenTelemetry│ │ Jaeger      │ │ Zipkin      │               │  │
│     │  │ Collector    │ │ Agent       │ │ Reporter    │               │  │
│     │  └──────┬──────┘ └──────┬──────┘ └──────┬──────┘               │  │
│     │         │               │               │                        │  │
│     │         ▼               ▼               ▼                        │  │
│     │  ┌─────────────────────────────────────────────────────────────┐│  │
│     │  │                     Trace Storage                           ││  │
│     │  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐││  │
│     │  │  │ Jaeger      │ │ Tempo       │ │ Elasticsearch           │││  │
│     │  │  │ Backend     │ │             │ │                         │││  │
│     │  │  └─────────────┘ └─────────────┘ └─────────────────────────┘││  │
│     │  └─────────────────────────────────────────────────────────────┘│  │
│     │                                                                     │
│     ▼                                                                     │
│  4. 告警处理阶段                                                        │
│     ┌─────────────────────────────────────────────────────────────────┐  │
│     │                         Alerting Pipeline                        │  │
│     │  ┌─────────────────────────────────────────────────────────────┐│  │
│     │  │                      Alertmanager                           ││  │
│     │  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐││  │
│     │  │  │ Routing     │ │ Silencing   │ │ Notification            │││  │
│     │  │  │             │ │             │ │ Channels                │││  │
│     │  │  └─────────────┘ └─────────────┘ └─────────────────────────┘││  │
│     │  └─────────────────────────────────────────────────────────────┘│  │
│     │         │               │               │                        │  │
│     │         ▼               ▼               ▼                        │  │
│     │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │  │
│     │  │ Slack/Webhook│ │ Email       │ │ PagerDuty   │               │  │
│     │  │             │ │             │ │             │               │  │
│     │  └─────────────┘ └─────────────┘ └─────────────┘               │  │
│     │                                                                     │
│     ▼                                                                     │
│  5. 可视化展示阶段                                                      │
│     ┌─────────────────────────────────────────────────────────────────┐  │
│     │                           Dashboards                             │  │
│     │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐               │  │
│     │  │ Grafana     │ │ Kibana      │ │ Custom UI   │               │  │
│     │  │             │ │             │ │             │               │  │
│     │  └─────────────┘ └─────────────┘ └─────────────┘               │  │
│     └─────────────────────────────────────────────────────────────────┘  │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 核心组件监控

### 2.1 API Server监控

```yaml
# API Server监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: kube-apiserver
  namespace: monitoring
spec:
  endpoints:
  - bearerTokenFile: /var/run/secrets/kubernetes.io/serviceaccount/token
    interval: 30s
    metricRelabelings:
    - action: keep
      regex: apiserver_request_(latency|count|duration)|etcd_(server|disk|network)
      sourceLabels:
      - __name__
    port: https
    scheme: https
    tlsConfig:
      caFile: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
      serverName: kubernetes
  jobLabel: component
  namespaceSelector:
    matchNames:
    - default
  selector:
    matchLabels:
      component: apiserver
      provider: kubernetes

---
# API Server关键指标
api_server_metrics:
  # 请求延迟指标
  - apiserver_request_duration_seconds{verb="GET",resource="pods"}
  - apiserver_request_duration_seconds{verb="POST",resource="pods"}
  
  # 请求成功率
  - apiserver_request_total{code=~"2.."}
  - apiserver_request_total{code=~"5.."}
  
  # etcd相关指标
  - etcd_request_duration_seconds
  - etcd_object_counts
  
  # 认证授权指标
  - authentication_attempts_total
  - authorization_attempts_total
```

### 2.2 etcd监控

```yaml
# etcd监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: etcd
  namespace: monitoring
spec:
  endpoints:
  - interval: 30s
    metricRelabelings:
    - action: keep
      regex: etcd_(server|disk|network|mvcc)
      sourceLabels:
      - __name__
    port: metrics
    scheme: https
    tlsConfig:
      caFile: /etc/prometheus/secrets/etcd-client-certs/ca.crt
      certFile: /etc/prometheus/secrets/etcd-client-certs/client.crt
      keyFile: /etc/prometheus/secrets/etcd-client-certs/client.key
  jobLabel: component
  namespaceSelector:
    matchNames:
    - kube-system
  selector:
    matchLabels:
      component: etcd

---
# etcd关键监控指标
etcd_metrics:
  # 集群健康状态
  - etcd_server_has_leader == 1
  - etcd_server_leader_changes_seen_total < 3
  
  # 性能指标
  - etcd_disk_backend_commit_duration_seconds{quantile="0.99"} < 0.25
  - etcd_disk_wal_fsync_duration_seconds{quantile="0.99"} < 0.1
  - etcd_network_peer_round_trip_time_seconds{quantile="0.99"} < 0.1
  
  # 存储使用
  - etcd_mvcc_db_total_size_in_bytes < 8589934592  # 8GB
  - etcd_mvcc_db_total_size_in_use_in_bytes
  - etcd_debugging_mvcc_keys_total
  
  # 操作统计
  - etcd_server_proposals_committed_total
  - etcd_server_proposals_applied_total
  - etcd_server_proposals_pending
  - etcd_server_proposals_failed_total == 0
```

### 2.3 Controller Manager监控

```yaml
# Controller Manager监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: kube-controller-manager
  namespace: monitoring
spec:
  endpoints:
  - bearerTokenFile: /var/run/secrets/kubernetes.io/serviceaccount/token
    interval: 30s
    metricRelabelings:
    - action: keep
      regex: workqueue_|node_collector_|replicaset_controller_|deployment_controller_
      sourceLabels:
      - __name__
    port: https-metrics
    scheme: https
  jobLabel: component
  namespaceSelector:
    matchNames:
    - kube-system
  selector:
    matchLabels:
      component: kube-controller-manager

---
# Controller Manager关键指标
controller_manager_metrics:
  # 工作队列指标
  - workqueue_depth{name="deployment"}
  - workqueue_queue_duration_seconds{job="kube-controller-manager"}
  - workqueue_adds_total
  
  # 控制器同步指标
  - deployment_controller_rate_limiter_use
  - replicaset_controller_revisions
  - node_collector_evictions_number
  
  # 资源操作指标
  - rest_client_requests_total
  - rest_client_request_latency_seconds
```

### 2.4 Scheduler监控

```yaml
# Scheduler监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: kube-scheduler
  namespace: monitoring
spec:
  endpoints:
  - bearerTokenFile: /var/run/secrets/kubernetes.io/serviceaccount/token
    interval: 30s
    metricRelabelings:
    - action: keep
      regex: scheduler_|rest_client_|workqueue_
      sourceLabels:
      - __name__
    port: https-metrics
    scheme: https
  jobLabel: component
  namespaceSelector:
    matchNames:
    - kube-system
  selector:
    matchLabels:
      component: kube-scheduler

---
# Scheduler关键指标
scheduler_metrics:
  # 调度性能指标
  - scheduler_e2e_scheduling_duration_seconds{quantile="0.99"} < 5
  - scheduler_scheduling_attempt_duration_seconds
  - scheduler_goroutines
  
  # 调度队列指标
  - scheduler_pending_pods{queue="active"}
  - scheduler_unschedulable_pods
  - scheduler_pod_scheduling_attempts
  
  # 过滤和评分指标
  - scheduler_binding_duration_seconds
  - scheduler_scheduling_algorithm_duration_seconds
  - scheduler_volume_scheduling_duration_seconds
```

---

## 3. 指标采集配置

### 3.1 Prometheus配置

```yaml
# Prometheus主配置文件
global:
  scrape_interval: 30s
  scrape_timeout: 10s
  evaluation_interval: 30s
  external_labels:
    cluster: production-cluster
    region: us-west-2

rule_files:
  - "/etc/prometheus/rules/*.rules"

scrape_configs:
# Control Plane组件抓取配置
- job_name: 'kubernetes-apiservers'
  kubernetes_sd_configs:
  - role: endpoints
  scheme: https
  tls_config:
    ca_file: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
  bearer_token_file: /var/run/secrets/kubernetes.io/serviceaccount/token
  relabel_configs:
  - source_labels: [__meta_kubernetes_namespace, __meta_kubernetes_service_name, __meta_kubernetes_endpoint_port_name]
    action: keep
    regex: default;kubernetes;https

- job_name: 'kubernetes-nodes'
  kubernetes_sd_configs:
  - role: node
  scheme: https
  tls_config:
    ca_file: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
  bearer_token_file: /var/run/secrets/kubernetes.io/serviceaccount/token
  relabel_configs:
  - action: labelmap
    regex: __meta_kubernetes_node_label_(.+)
  - target_label: __address__
    replacement: kubernetes.default.svc:443
  - source_labels: [__meta_kubernetes_node_name]
    regex: (.+)
    target_label: __metrics_path__
    replacement: /api/v1/nodes/${1}/proxy/metrics

- job_name: 'etcd'
  static_configs:
  - targets: 
    - 'etcd-0.etcd:2379'
    - 'etcd-1.etcd:2379'
    - 'etcd-2.etcd:2379'
  scheme: https
  tls_config:
    ca_file: /etc/prometheus/secrets/etcd-client-certs/ca.crt
    cert_file: /etc/prometheus/secrets/etcd-client-certs/client.crt
    key_file: /etc/prometheus/secrets/etcd-client-certs/client.key

# 告警配置
alerting:
  alertmanagers:
  - static_configs:
    - targets:
      - alertmanager.monitoring.svc:9093
```

### 3.2 指标保留策略

```yaml
# Prometheus存储配置
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: prometheus-k8s
  namespace: monitoring
spec:
  retention: 30d
  retentionSize: "50GB"
  storage:
    volumeClaimTemplate:
      spec:
        storageClassName: fast-ssd
        resources:
          requests:
            storage: 100Gi
            
  # 远程写入配置
  remoteWrite:
  - url: http://thanos-receive.thanos.svc:19291/api/v1/receive
    writeRelabelConfigs:
    - sourceLabels: [__name__]
      regex: (apiserver|etcd|scheduler|controller_manager).*
      action: keep
      
  # 存储TSDB配置
  tsdb:
    outOfOrderTimeWindow: 30m
    
  # 压缩配置
  enableAdminAPI: false
  logLevel: info
  logFormat: json
```

---

## 4. 日志收集分析

### 4.1 日志收集架构

```yaml
# Fluent Bit日志收集配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-config
  namespace: logging
data:
  fluent-bit.conf: |
    [SERVICE]
        Flush         1
        Log_Level     info
        Daemon        off
        Parsers_File  parsers.conf
        HTTP_Server   On
        HTTP_Listen   0.0.0.0
        HTTP_Port     2020

    [INPUT]
        Name              tail
        Tag               kube.*
        Path              /var/log/containers/*.log
        Parser            docker
        DB                /var/log/flb_kube.db
        Mem_Buf_Limit     5MB
        Skip_Long_Lines   On
        Refresh_Interval  10

    [INPUT]
        Name              systemd
        Tag               host.*
        Systemd_Filter    _SYSTEMD_UNIT=kubelet.service
        Systemd_Filter    _SYSTEMD_UNIT=docker.service
        Read_From_Tail    On

    [FILTER]
        Name                kubernetes
        Match               kube.*
        Kube_URL            https://kubernetes.default.svc:443
        Kube_CA_File        /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
        Kube_Token_File     /var/run/secrets/kubernetes.io/serviceaccount/token
        Kube_Tag_Prefix     kube.var.log.containers.
        Merge_Log           On
        Merge_Log_Key       log_processed
        K8S-Logging.Parser  On
        K8S-Logging.Exclude Off

    [OUTPUT]
        Name            es
        Match           kube.*
        Host            ${ELASTICSEARCH_HOST}
        Port            ${ELASTICSEARCH_PORT}
        Logstash_Format On
        Logstash_Prefix k8s-logs
        Time_Key        @timestamp
        Replace_Dots    On
        Retry_Limit     False

    [OUTPUT]
        Name            loki
        Match           host.*
        Url             http://loki.logging.svc:3100/loki/api/v1/push
        BatchWait       1s
        BatchSize       30720
        Labels          {job="systemd-logs", nodename="${NODE_NAME}"}
```

### 4.2 关键日志源配置

```bash
#!/bin/bash
# 控制平面日志收集脚本

# 1. API Server日志配置
configure_apiserver_logging() {
    cat > /etc/kubernetes/logging/apiserver.yaml << EOF
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
spec:
  containers:
  - name: kube-apiserver
    image: registry.k8s.io/kube-apiserver:v1.30.0
    command:
    - kube-apiserver
    # 日志配置
    - --audit-log-path=/var/log/kubernetes/audit.log
    - --audit-log-maxage=30
    - --audit-log-maxbackup=10
    - --audit-log-maxsize=100
    - --audit-log-format=json
    - --audit-policy-file=/etc/kubernetes/audit/policy.yaml
    - --v=2  # 详细日志级别
    
    volumeMounts:
    - name: audit-log
      mountPath: /var/log/kubernetes
    - name: audit-policy
      mountPath: /etc/kubernetes/audit
      
  volumes:
  - name: audit-log
    hostPath:
      path: /var/log/kubernetes
      type: DirectoryOrCreate
  - name: audit-policy
    configMap:
      name: audit-policy
EOF
}

# 2. etcd日志配置
configure_etcd_logging() {
    cat > /etc/kubernetes/logging/etcd.yaml << EOF
apiVersion: v1
kind: Pod
metadata:
  name: etcd
  namespace: kube-system
spec:
  containers:
  - name: etcd
    image: quay.io/coreos/etcd:v3.5.12
    command:
    - etcd
    # 日志配置
    - --logger=zap
    - --log-level=info
    - --log-outputs=stderr
    
    env:
    - name: ETCD_LOG_LEVEL
      value: "info"
      
    volumeMounts:
    - name: etcd-data
      mountPath: /var/lib/etcd
    - name: etcd-logs
      mountPath: /var/log/etcd
      
  volumes:
  - name: etcd-data
    hostPath:
      path: /var/lib/etcd
      type: DirectoryOrCreate
  - name: etcd-logs
    hostPath:
      path: /var/log/etcd
      type: DirectoryOrCreate
EOF
}

# 3. 日志轮转配置
configure_log_rotation() {
    cat > /etc/logrotate.d/kubernetes << EOF
/var/log/kubernetes/*.log {
    daily
    rotate 30
    compress
    delaycompress
    missingok
    notifempty
    create 0644 root root
    postrotate
        systemctl reload rsyslog
    endscript
}

/var/log/etcd/*.log {
    daily
    rotate 30
    compress
    delaycompress
    missingok
    notifempty
    create 0644 etcd etcd
}
EOF
}
```

### 4.3 日志分析规则

```yaml
# Loki日志分析规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: log-analysis-rules
  namespace: logging
spec:
  groups:
  - name: log.analysis.rules
    rules:
    # API Server错误日志检测
    - alert: APIServerErrorLogs
      expr: |
        sum(rate({job="kubernetes-apiserver"} 
        |= "error" 
        |~ "failed|timeout|unauthorized"[5m])) > 10
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High rate of API Server error logs"
        description: "{{ $value }} error logs per second in the last 5 minutes"

    # etcd性能问题检测
    - alert: EtcdSlowOperations
      expr: |
        sum(rate({job="etcd"} 
        |= "slow" 
        |~ "took too long"[5m])) > 5
      for: 10m
      labels:
        severity: critical
      annotations:
        summary: "etcd slow operations detected"
        description: "{{ $value }} slow operations per second"

    # 控制器异常检测
    - alert: ControllerErrors
      expr: |
        sum(rate({job="kube-controller-manager"} 
        |= "error" 
        |~ "failed to sync|[Ee]rror"[5m])) > 3
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Controller manager errors detected"
        description: "{{ $value }} controller errors per second"
```

---

## 5. 追踪与链路监控

### 5.1 OpenTelemetry配置

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
            endpoint: 0.0.0.0:4317
          http:
            endpoint: 0.0.0.0:4318
            
      jaeger:
        protocols:
          grpc:
            endpoint: 0.0.0.0:14250
          thrift_http:
            endpoint: 0.0.0.0:14268
            
      zipkin:
        endpoint: 0.0.0.0:9411

    processors:
      batch:
        timeout: 5s
        send_batch_size: 8192
        
      memory_limiter:
        limit_mib: 400
        spike_limit_mib: 100
        
      attributes:
        actions:
          - key: cluster.name
            value: "production-cluster"
            action: insert

    exporters:
      jaeger:
        endpoint: jaeger-collector.observability.svc:14250
        tls:
          insecure: true
          
      prometheus:
        endpoint: "0.0.0.0:8889"
        namespace: otel
        const_labels:
          cluster: "production"

    service:
      pipelines:
        traces:
          receivers: [otlp, jaeger, zipkin]
          processors: [memory_limiter, batch, attributes]
          exporters: [jaeger]
          
        metrics:
          receivers: [otlp]
          processors: [memory_limiter, batch]
          exporters: [prometheus]
```

### 5.2 应用追踪配置

```yaml
# 应用追踪注解配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: traced-application
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: traced-app
  template:
    metadata:
      labels:
        app: traced-app
      annotations:
        instrumentation.opentelemetry.io/inject-sdk: "true"
        instrumentation.opentelemetry.io/otel-exporter-otlp-endpoint: "otel-collector.observability.svc:4317"
    spec:
      containers:
      - name: app
        image: myapp:latest
        env:
        - name: OTEL_SERVICE_NAME
          value: "traced-application"
        - name: OTEL_EXPORTER_OTLP_ENDPOINT
          value: "http://otel-collector.observability.svc:4317"
        - name: OTEL_TRACES_SAMPLER
          value: "traceidratio"
        - name: OTEL_TRACES_SAMPLER_ARG
          value: "0.1"  # 10%采样率
```

### 5.3 追踪数据分析

```python
# 追踪数据分析脚本
import requests
import json
from datetime import datetime, timedelta

class TraceAnalyzer:
    def __init__(self, jaeger_url):
        self.jaeger_url = jaeger_url
        
    def get_trace_by_id(self, trace_id):
        """根据trace ID获取完整追踪"""
        url = f"{self.jaeger_url}/api/traces/{trace_id}"
        response = requests.get(url)
        return response.json()
        
    def analyze_api_performance(self, service_name, hours=24):
        """分析API性能"""
        end_time = datetime.now()
        start_time = end_time - timedelta(hours=hours)
        
        params = {
            'service': service_name,
            'start': int(start_time.timestamp() * 1000000),
            'end': int(end_time.timestamp() * 1000000),
            'lookback': '1d',
            'limit': 1000
        }
        
        url = f"{self.jaeger_url}/api/traces"
        response = requests.get(url, params=params)
        traces = response.json().get('data', [])
        
        # 分析延迟分布
        latencies = []
        error_spans = []
        
        for trace in traces:
            for span in trace.get('spans', []):
                if span.get('operationName') == 'API_CALL':
                    duration = span.get('duration', 0) / 1000  # 转换为毫秒
                    latencies.append(duration)
                    
                    # 检查错误
                    if any(tag.get('key') == 'error' and tag.get('value') for tag in span.get('tags', [])):
                        error_spans.append(span)
        
        return {
            'avg_latency': sum(latencies) / len(latencies) if latencies else 0,
            'p95_latency': sorted(latencies)[int(len(latencies) * 0.95)] if latencies else 0,
            'error_rate': len(error_spans) / len(traces) if traces else 0,
            'total_traces': len(traces)
        }

# 使用示例
analyzer = TraceAnalyzer("http://jaeger-query.observability.svc:16686")
performance = analyzer.analyze_api_performance("kube-apiserver")
print(f"Average latency: {performance['avg_latency']:.2f}ms")
print(f"95th percentile: {performance['p95_latency']:.2f}ms")
print(f"Error rate: {performance['error_rate']:.2%}")
```

---

## 6. 告警策略配置

### 6.1 核心告警规则

```yaml
# 核心告警规则配置
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: control-plane-alerts
  namespace: monitoring
spec:
  groups:
  - name: control-plane.rules
    rules:
    # API Server告警
    - alert: APIServerDown
      expr: up{job="apiserver"} == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "API Server is down"
        description: "Kubernetes API Server has been down for more than 5 minutes"

    - alert: APIServerHighLatency
      expr: histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m])) > 1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "API Server high latency"
        description: "API Server 99th percentile latency is above 1 second"

    - alert: APIServerErrorRate
      expr: sum(rate(apiserver_request_total{code=~"5.."}[5m])) / sum(rate(apiserver_request_total[5m])) > 0.05
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "High API Server error rate"
        description: "API Server error rate is above 5%"

    # etcd告警
    - alert: EtcdInsufficientMembers
      expr: count(etcd_server_has_leader) < 3
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "etcd cluster has insufficient members"
        description: "etcd cluster has {{ $value }} members, less than required 3"

    - alert: EtcdHighCommitLatency
      expr: histogram_quantile(0.99, rate(etcd_disk_backend_commit_duration_seconds_bucket[5m])) > 0.25
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "etcd high commit latency"
        description: "etcd backend commit latency is high"

    - alert: EtcdDatabaseSpaceLow
      expr: etcd_mvcc_db_total_size_in_bytes / etcd_server_quota_backend_bytes > 0.8
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "etcd database space low"
        description: "etcd database usage is above 80%"

    # 控制器告警
    - alert: ControllerManagerDown
      expr: up{job="kube-controller-manager"} == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Controller Manager is down"
        description: "Kubernetes Controller Manager has been down for more than 5 minutes"

    - alert: SchedulerDown
      expr: up{job="kube-scheduler"} == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Scheduler is down"
        description: "Kubernetes Scheduler has been down for more than 5 minutes"

    # 节点告警
    - alert: NodeNotReady
      expr: kube_node_status_condition{condition="Ready",status="false"} == 1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Node is not ready"
        description: "Node {{ $labels.node }} has been not ready for more than 10 minutes"

    - alert: NodeMemoryPressure
      expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Node memory pressure"
        description: "Node {{ $labels.node }} is experiencing memory pressure"

    # 工作负载告警
    - alert: DeploymentReplicasMismatch
      expr: kube_deployment_spec_replicas != kube_deployment_status_replicas_available
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "Deployment replicas mismatch"
        description: "Deployment {{ $labels.deployment }} available replicas don't match desired replicas"
```

### 6.2 Alertmanager配置

```yaml
# Alertmanager配置
global:
  resolve_timeout: 5m
  smtp_from: 'alertmanager@company.com'
  smtp_smarthost: 'smtp.company.com:587'
  smtp_auth_username: 'alertmanager'
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
    group_interval: 1m
    repeat_interval: 30m
    
  - match:
      severity: warning
    receiver: 'slack-warning'
    group_wait: 1m
    group_interval: 10m
    repeat_interval: 2h

receivers:
- name: 'default-receiver'
  email_configs:
  - to: 'team@company.com'
    send_resolved: true

- name: 'pagerduty'
  pagerduty_configs:
  - service_key: 'YOUR_PAGERDUTY_SERVICE_KEY'
    send_resolved: true

- name: 'slack-warning'
  slack_configs:
  - api_url: 'https://hooks.slack.com/services/YOUR/SLACK/WEBHOOK'
    channel: '#alerts'
    send_resolved: true
    title: '{{ template "slack.title" . }}'
    text: '{{ template "slack.text" . }}'
    icon_emoji: ':warning:'
    
templates:
- '/etc/alertmanager/template/*.tmpl'
```

---

## 7. 可视化仪表板

### 7.1 Grafana仪表板配置

```json
{
  "dashboard": {
    "id": null,
    "title": "Kubernetes Control Plane Overview",
    "timezone": "browser",
    "schemaVersion": 16,
    "version": 0,
    "refresh": "30s",
    "panels": [
      {
        "type": "graph",
        "title": "API Server Request Latency",
        "gridPos": {
          "h": 8,
          "w": 12,
          "x": 0,
          "y": 0
        },
        "targets": [
          {
            "expr": "histogram_quantile(0.5, sum(rate(apiserver_request_duration_seconds_bucket[5m])) by (le))",
            "legendFormat": "50th percentile"
          },
          {
            "expr": "histogram_quantile(0.9, sum(rate(apiserver_request_duration_seconds_bucket[5m])) by (le))",
            "legendFormat": "90th percentile"
          },
          {
            "expr": "histogram_quantile(0.99, sum(rate(apiserver_request_duration_seconds_bucket[5m])) by (le))",
            "legendFormat": "99th percentile"
          }
        ],
        "yaxes": [
          {
            "format": "s",
            "label": "Latency"
          },
          {
            "format": "short"
          }
        ]
      },
      {
        "type": "singlestat",
        "title": "etcd Members",
        "gridPos": {
          "h": 4,
          "w": 4,
          "x": 12,
          "y": 0
        },
        "targets": [
          {
            "expr": "count(etcd_server_has_leader)",
            "instant": true
          }
        ],
        "thresholds": "2,3",
        "colorBackground": true,
        "colorValue": false,
        "colors": [
          "#d44a3a",
          "rgba(237, 129, 40, 0.89)",
          "#299c46"
        ]
      },
      {
        "type": "graph",
        "title": "Control Plane Component Status",
        "gridPos": {
          "h": 8,
          "w": 12,
          "x": 0,
          "y": 8
        },
        "targets": [
          {
            "expr": "up{job=~\"apiserver|etcd|kube-controller-manager|kube-scheduler\"}",
            "legendFormat": "{{job}}"
          }
        ],
        "yaxes": [
          {
            "format": "short",
            "label": "Status (1=Up, 0=Down)"
          },
          {
            "format": "short"
          }
        ]
      }
    ]
  }
}
```

### 7.2 自定义监控面板

```yaml
# 自定义监控面板配置
apiVersion: grafana.integreatly.org/v1beta1
kind: GrafanaDashboard
metadata:
  name: control-plane-custom
  namespace: monitoring
spec:
  instanceSelector:
    matchLabels:
      dashboards: "grafana"
  json: |
    {
      "annotations": {
        "list": [
          {
            "builtIn": 1,
            "datasource": "-- Grafana --",
            "enable": true,
            "hide": true,
            "iconColor": "rgba(0, 211, 255, 1)",
            "name": "Annotations & Alerts",
            "type": "dashboard"
          }
        ]
      },
      "editable": true,
      "gnetId": null,
      "graphTooltip": 0,
      "links": [],
      "panels": [
        {
          "aliasColors": {},
          "bars": false,
          "dashLength": 10,
          "dashes": false,
          "datasource": "Prometheus",
          "fill": 1,
          "fillGradient": 0,
          "gridPos": {
            "h": 8,
            "w": 12,
            "x": 0,
            "y": 0
          },
          "hiddenSeries": false,
          "id": 2,
          "legend": {
            "avg": false,
            "current": false,
            "max": false,
            "min": false,
            "show": true,
            "total": false,
            "values": false
          },
          "lines": true,
          "linewidth": 1,
          "nullPointMode": "null",
          "options": {
            "dataLinks": []
          },
          "percentage": false,
          "pointradius": 2,
          "points": false,
          "renderer": "flot",
          "seriesOverrides": [],
          "spaceLength": 10,
          "stack": false,
          "steppedLine": false,
          "targets": [
            {
              "expr": "sum(rate(workqueue_depth{job=\"kube-controller-manager\"}[5m])) by (name)",
              "legendFormat": "{{name}}",
              "refId": "A"
            }
          ],
          "thresholds": [],
          "timeFrom": null,
          "timeRegions": [],
          "timeShift": null,
          "title": "Controller Work Queue Depth",
          "tooltip": {
            "shared": true,
            "sort": 0,
            "value_type": "individual"
          },
          "type": "graph",
          "xaxis": {
            "buckets": null,
            "mode": "time",
            "name": null,
            "show": true,
            "values": []
          },
          "yaxes": [
            {
              "format": "short",
              "label": null,
              "logBase": 1,
              "max": null,
              "min": null,
              "show": true
            },
            {
              "format": "short",
              "label": null,
              "logBase": 1,
              "max": null,
              "min": null,
              "show": true
            }
          ],
          "yaxis": {
            "align": false,
            "alignLevel": null
          }
        }
      ],
      "schemaVersion": 22,
      "style": "dark",
      "tags": [],
      "templating": {
        "list": []
      },
      "time": {
        "from": "now-6h",
        "to": "now"
      },
      "timepicker": {
        "refresh_intervals": [
          "5s",
          "10s",
          "30s",
          "1m",
          "5m",
          "15m",
          "30m",
          "1h",
          "2h",
          "1d"
        ]
      },
      "timezone": "",
      "title": "Control Plane Custom Dashboard",
      "uid": "control-plane-custom",
      "version": 1
    }
```

---

## 8. 性能基准建立

### 8.1 基准测试脚本

```bash
#!/bin/bash
# 控制平面性能基准测试

set -euo pipefail

CLUSTER_NAME="benchmark-cluster"
RESULTS_DIR="/tmp/benchmark-results"
mkdir -p $RESULTS_DIR

# 1. API Server性能测试
test_apiserver_performance() {
    echo "Testing API Server performance..."
    
    # 测试GET请求延迟
    start_time=$(date +%s%3N)
    for i in {1..100}; do
        curl -sk https://kubernetes.default.svc/api/v1/namespaces/default/pods >/dev/null
    done
    end_time=$(date +%s%3N)
    
    avg_latency=$(( ($end_time - $start_time) / 100 ))
    echo "Average GET latency: ${avg_latency}ms" > $RESULTS_DIR/apiserver-get-latency.txt
    
    # 测试POST请求延迟
    start_time=$(date +%s%3N)
    for i in {1..50}; do
        cat > /tmp/test-pod.yaml << EOF
apiVersion: v1
kind: Pod
metadata:
  name: test-pod-$i
  namespace: default
spec:
  containers:
  - name: test
    image: nginx:latest
EOF
        kubectl apply -f /tmp/test-pod.yaml >/dev/null
    done
    end_time=$(date +%s%3N)
    
    avg_post_latency=$(( ($end_time - $start_time) / 50 ))
    echo "Average POST latency: ${avg_post_latency}ms" > $RESULTS_DIR/apiserver-post-latency.txt
}

# 2. etcd性能测试
test_etcd_performance() {
    echo "Testing etcd performance..."
    
    # 写入性能测试
    start_time=$(date +%s%3N)
    for i in {1..1000}; do
        ETCDCTL_API=3 etcdctl put "test-key-$i" "test-value-$i" >/dev/null
    done
    end_time=$(date +%s%3N)
    
    write_latency=$(( ($end_time - $start_time) / 1000 ))
    echo "Average write latency: ${write_latency}ms" > $RESULTS_DIR/etcd-write-latency.txt
    
    # 读取性能测试
    start_time=$(date +%s%3N)
    for i in {1..1000}; do
        ETCDCTL_API=3 etcdctl get "test-key-$i" >/dev/null
    done
    end_time=$(date +%s%3N)
    
    read_latency=$(( ($end_time - $start_time) / 1000 ))
    echo "Average read latency: ${read_latency}ms" > $RESULTS_DIR/etcd-read-latency.txt
}

# 3. 调度器性能测试
test_scheduler_performance() {
    echo "Testing scheduler performance..."
    
    # 创建大量Pod测试调度延迟
    cat > /tmp/scale-test.yaml << EOF
apiVersion: apps/v1
kind: Deployment
metadata:
  name: scale-test
  namespace: default
spec:
  replicas: 100
  selector:
    matchLabels:
      app: scale-test
  template:
    metadata:
      labels:
        app: scale-test
    spec:
      containers:
      - name: test
        image: nginx:latest
        resources:
          requests:
            memory: "64Mi"
            cpu: "250m"
EOF

    start_time=$(date +%s)
    kubectl apply -f /tmp/scale-test.yaml
    
    # 等待所有Pod Running
    while [[ $(kubectl get pods -n default -l app=scale-test --no-headers | grep Running | wc -l) -lt 100 ]]; do
        sleep 5
    done
    
    end_time=$(date +%s)
    scheduling_time=$(( end_time - start_time ))
    echo "Total scheduling time for 100 pods: ${scheduling_time}s" > $RESULTS_DIR/scheduler-time.txt
}

# 4. 生成基准报告
generate_benchmark_report() {
    echo "Generating benchmark report..."
    
    cat > $RESULTS_DIR/benchmark-report.txt << EOF
=====================================
Kubernetes Control Plane Benchmark Report
Cluster: $CLUSTER_NAME
Date: $(date)
=====================================

API Server Performance:
$(cat $RESULTS_DIR/apiserver-get-latency.txt)
$(cat $RESULTS_DIR/apiserver-post-latency.txt)

etcd Performance:
$(cat $RESULTS_DIR/etcd-write-latency.txt)
$(cat $RESULTS_DIR/etcd-read-latency.txt)

Scheduler Performance:
$(cat $RESULTS_DIR/scheduler-time.txt)

Recommendations:
- API Server GET latency should be < 50ms
- API Server POST latency should be < 200ms
- etcd write latency should be < 10ms
- etcd read latency should be < 5ms
- 100 pod scheduling should complete in < 300s

EOF
    
    echo "Benchmark report saved to $RESULTS_DIR/benchmark-report.txt"
}

# 执行所有测试
main() {
    echo "Starting control plane benchmark tests..."
    
    test_apiserver_performance
    test_etcd_performance
    test_scheduler_performance
    generate_benchmark_report
    
    echo "Benchmark tests completed!"
}

main "$@"
```

### 8.2 性能基线指标

```yaml
# 性能基线配置
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: performance-baseline
  namespace: monitoring
spec:
  groups:
  - name: performance.baseline.rules
    rules:
    # API Server性能基线
    - record: apiserver:get_latency:baseline
      expr: histogram_quantile(0.95, rate(apiserver_request_duration_seconds_bucket{verb="GET"}[1h]))
      
    - record: apiserver:post_latency:baseline
      expr: histogram_quantile(0.95, rate(apiserver_request_duration_seconds_bucket{verb="POST"}[1h]))
      
    # etcd性能基线
    - record: etcd:write_latency:baseline
      expr: histogram_quantile(0.95, rate(etcd_disk_backend_commit_duration_seconds_bucket[1h]))
      
    - record: etcd:read_latency:baseline
      expr: histogram_quantile(0.95, rate(etcd_network_peer_round_trip_time_seconds_bucket[1h]))
      
    # 调度器性能基线
    - record: scheduler:scheduling_duration:baseline
      expr: histogram_quantile(0.95, rate(scheduler_e2e_scheduling_duration_seconds_bucket[1h]))

    # 性能偏差告警
    - alert: APIServerPerformanceDegradation
      expr: |
        histogram_quantile(0.95, rate(apiserver_request_duration_seconds_bucket{verb="GET"}[5m])) 
        > on() apiserver:get_latency:baseline * 1.5
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "API Server performance degradation detected"
        description: "Current GET latency is 50% higher than baseline"
```

---

## 9. 故障诊断工具

### 9.1 诊断脚本集合

```bash
#!/bin/bash
# 控制平面故障诊断工具包

set -euo pipefail

DIAG_DIR="/tmp/control-plane-diagnosis-$(date +%Y%m%d-%H%M%S)"
mkdir -p $DIAG_DIR

# 1. 集群状态诊断
diagnose_cluster_status() {
    echo "Diagnosing cluster status..."
    
    # 检查节点状态
    kubectl get nodes -o wide > $DIAG_DIR/nodes-status.txt
    kubectl describe nodes > $DIAG_DIR/nodes-describe.txt
    
    # 检查控制平面Pod状态
    kubectl get pods -n kube-system > $DIAG_DIR/control-plane-pods.txt
    kubectl describe pods -n kube-system > $DIAG_DIR/control-plane-pods-describe.txt
    
    # 检查组件健康状态
    kubectl get componentstatuses > $DIAG_DIR/component-status.txt
}

# 2. API Server诊断
diagnose_apiserver() {
    echo "Diagnosing API Server..."
    
    # 检查API Server端点
    kubectl get --raw=/healthz > $DIAG_DIR/apiserver-health.txt
    kubectl get --raw=/livez > $DIAG_DIR/apiserver-live.txt
    kubectl get --raw=/readyz > $DIAG_DIR/apiserver-ready.txt
    
    # 检查API Server日志
    kubectl logs -n kube-system -l component=kube-apiserver --tail=1000 > $DIAG_DIR/apiserver-logs.txt
    
    # 检查API Server配置
    kubectl get pod -n kube-system -l component=kube-apiserver -o yaml > $DIAG_DIR/apiserver-config.txt
}

# 3. etcd诊断
diagnose_etcd() {
    echo "Diagnosing etcd..."
    
    # 检查etcd集群状态
    ETCDCTL_API=3 etcdctl endpoint health --cluster > $DIAG_DIR/etcd-health.txt
    ETCDCTL_API=3 etcdctl endpoint status --cluster -w table > $DIAG_DIR/etcd-status.txt
    
    # 检查etcd性能指标
    ETCDCTL_API=3 etcdctl check perf --load=s > $DIAG_DIR/etcd-performance.txt
    
    # 检查etcd日志
    kubectl logs -n kube-system -l component=etcd --tail=1000 > $DIAG_DIR/etcd-logs.txt
}

# 4. 控制器诊断
diagnose_controllers() {
    echo "Diagnosing controllers..."
    
    # 检查控制器日志
    kubectl logs -n kube-system -l component=kube-controller-manager --tail=500 > $DIAG_DIR/controller-logs.txt
    
    # 检查控制器工作队列
    kubectl get --raw=/metrics | grep workqueue > $DIAG_DIR/controller-metrics.txt
    
    # 检查特定控制器状态
    kubectl get deployments -A > $DIAG_DIR/deployments-status.txt
    kubectl get replicasets -A > $DIAG_DIR/replicasets-status.txt
}

# 5. 调度器诊断
diagnose_scheduler() {
    echo "Diagnosing scheduler..."
    
    # 检查调度器日志
    kubectl logs -n kube-system -l component=kube-scheduler --tail=500 > $DIAG_DIR/scheduler-logs.txt
    
    # 检查调度器指标
    kubectl get --raw=/metrics | grep scheduler > $DIAG_DIR/scheduler-metrics.txt
    
    # 检查未调度Pod
    kubectl get pods --all-namespaces --field-selector=status.phase!=Running,status.phase!=Succeeded > $DIAG_DIR/unscheduled-pods.txt
}

# 6. 网络诊断
diagnose_network() {
    echo "Diagnosing network..."
    
    # 检查网络连通性
    kubectl get services --all-namespaces > $DIAG_DIR/services.txt
    kubectl get endpoints --all-namespaces > $DIAG_DIR/endpoints.txt
    
    # 检查网络策略
    kubectl get networkpolicies --all-namespaces > $DIAG_DIR/networkpolicies.txt
    
    # 检查DNS解析
    kubectl run -it --rm debug-dns --image=busybox:1.28 -- nslookup kubernetes.default
}

# 7. 生成诊断报告
generate_diagnosis_report() {
    echo "Generating diagnosis report..."
    
    cat > $DIAG_DIR/diagnosis-report.txt << EOF
=====================================
Kubernetes Control Plane Diagnosis Report
Generated: $(date)
=====================================

Cluster Status Summary:
$(grep -E "(Ready|NotReady)" $DIAG_DIR/nodes-status.txt | head -10)

Critical Issues Found:
$(grep -i "error\|fail\|warning" $DIAG_DIR/*.txt | head -20)

Recommendations:
1. Check the detailed logs in individual files
2. Review component configurations
3. Monitor resource utilization
4. Verify network connectivity

Files Generated:
$(ls -la $DIAG_DIR/*.txt)

EOF
    
    echo "Diagnosis report saved to $DIAG_DIR/diagnosis-report.txt"
    echo "All diagnostic data saved to $DIAG_DIR/"
}

# 主诊断流程
main() {
    echo "Starting control plane diagnosis..."
    
    diagnose_cluster_status
    diagnose_apiserver
    diagnose_etcd
    diagnose_controllers
    diagnose_scheduler
    diagnose_network
    generate_diagnosis_report
    
    echo "Diagnosis completed! Results saved to $DIAG_DIR/"
}

# 如果脚本被直接执行
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
```

### 9.2 实时监控脚本

```python
#!/usr/bin/env python3
# 实时控制平面监控脚本

import subprocess
import time
import json
from datetime import datetime

class RealTimeMonitor:
    def __init__(self):
        self.metrics_history = {}
        
    def get_component_status(self):
        """获取组件状态"""
        try:
            result = subprocess.run([
                'kubectl', 'get', 'pods', '-n', 'kube-system',
                '-o', 'jsonpath={range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\n"}{end}'
            ], capture_output=True, text=True, timeout=10)
            
            status_dict = {}
            for line in result.stdout.strip().split('\n'):
                if line:
                    name, phase = line.split('\t')
                    status_dict[name] = phase
                    
            return status_dict
        except Exception as e:
            print(f"Error getting component status: {e}")
            return {}
    
    def get_api_metrics(self):
        """获取API Server指标"""
        try:
            # 这里应该连接到Prometheus获取实时指标
            # 简化示例使用kubectl top
            result = subprocess.run([
                'kubectl', 'top', 'pods', '-n', 'kube-system'
            ], capture_output=True, text=True, timeout=10)
            
            return result.stdout
        except Exception as e:
            print(f"Error getting API metrics: {e}")
            return ""
    
    def check_health(self):
        """健康检查"""
        checks = {
            'timestamp': datetime.now().isoformat(),
            'components': self.get_component_status(),
            'api_metrics': self.get_api_metrics(),
            'warnings': []
        }
        
        # 检查关键组件状态
        critical_components = ['kube-apiserver', 'etcd', 'kube-controller-manager', 'kube-scheduler']
        
        for component in critical_components:
            component_pods = [k for k in checks['components'].keys() if component in k]
            if not component_pods:
                checks['warnings'].append(f"No {component} pods found")
                continue
                
            for pod in component_pods:
                if checks['components'][pod] != 'Running':
                    checks['warnings'].append(f"{pod} is not Running (status: {checks['components'][pod]})")
        
        return checks
    
    def monitor_loop(self, interval=30):
        """监控循环"""
        print("Starting real-time monitoring...")
        print("Press Ctrl+C to stop")
        
        try:
            while True:
                status = self.check_health()
                
                # 显示状态
                print(f"\n[{status['timestamp']}] Cluster Status:")
                print("-" * 50)
                
                for component, status_phase in status['components'].items():
                    status_icon = "✓" if status_phase == 'Running' else "✗"
                    print(f"{status_icon} {component}: {status_phase}")
                
                if status['warnings']:
                    print("\n⚠️  Warnings:")
                    for warning in status['warnings']:
                        print(f"  - {warning}")
                
                # 保存历史数据
                timestamp_key = status['timestamp']
                self.metrics_history[timestamp_key] = status
                
                time.sleep(interval)
                
        except KeyboardInterrupt:
            print("\nMonitoring stopped.")
            self.save_history()
    
    def save_history(self):
        """保存历史数据"""
        filename = f"monitoring-history-{datetime.now().strftime('%Y%m%d-%H%M%S')}.json"
        with open(filename, 'w') as f:
            json.dump(self.metrics_history, f, indent=2)
        print(f"History saved to {filename}")

# 使用示例
if __name__ == "__main__":
    monitor = RealTimeMonitor()
    monitor.monitor_loop(interval=30)
```

---

## 10. 监控最佳实践

### 10.1 监控策略清单

```yaml
# 监控策略检查清单
monitoring_checklist:
  foundational_monitoring:
    - [x] 部署Prometheus监控系统
    - [x] 配置Alertmanager告警
    - [x] 部署Grafana可视化
    - [x] 设置日志收集系统
    - [x] 配置追踪系统
    
  control_plane_monitoring:
    - [x] API Server指标监控
    - [x] etcd集群健康监控
    - [x] Controller Manager工作队列监控
    - [x] Scheduler调度性能监控
    - [x] 节点状态监控
    
  application_monitoring:
    - [ ] 应用自定义指标
    - [ ] 业务关键指标监控
    - [ ] SLI/SLO定义和监控
    - [ ] 用户体验指标收集
    - [ ] 第三方服务依赖监控
    
  alerting_strategy:
    - [x] 分层告警策略
    - [x] 告警抑制规则
    - [x] 告警路由配置
    - [x] 通知渠道设置
    - [x] 告警模板定制
    
  performance_benchmarking:
    - [x] 建立性能基线
    - [x] 定期性能测试
    - [x] 容量规划监控
    - [x] 资源利用率分析
    - [x] 性能趋势分析
    
  security_monitoring:
    - [x] 认证授权监控
    - [x] 网络安全监控
    - [x] 异常行为检测
    - [x] 安全事件告警
    - [x] 合规性监控
```

### 10.2 监控成熟度模型

```
监控成熟度等级:

Level 1 - 基础监控 (Basic Monitoring)
├── 核心组件状态监控
├── 基本告警配置
├── 简单仪表板展示
└── 手动故障排查

Level 2 - 标准监控 (Standard Monitoring)
├── 全面指标收集
├── 自动化告警
├── 丰富的可视化
├── 标准化监控流程
└── 基础性能分析

Level 3 - 高级监控 (Advanced Monitoring)
├── 智能告警策略
├── 预测性分析
├── 自动化诊断
├── 成本优化监控
└── 用户体验监控

Level 4 - 智能监控 (Intelligent Monitoring)
├── AI驱动的异常检测
├── 自适应阈值设置
├── 根因分析自动化
├── 智能容量规划
└── 业务影响评估

Level 5 - 自主运维 (Autonomous Operations)
├── 完全自动化的运维
├── 预防性问题解决
├── 动态资源优化
├── 业务连续性保障
└── 持续改进机制
```

通过实施这套完整的监控和可观测性解决方案，您可以全面掌握Kubernetes控制平面的运行状态，快速发现和解决问题，确保系统的稳定性和可靠性。