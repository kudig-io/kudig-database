# 15 - 监控告警体系 (Monitoring & Alerting System)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [prometheus.io](https://prometheus.io/) | [grafana.com](https://grafana.com/)

## 监控架构设计

### 现代监控体系架构

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           监控告警体系架构                               │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐     │
│  │   数据采集层     │    │   存储分析层     │    │   展示告警层     │    │
│  │                 │    │                 │    │                 │    │
│  │  Prometheus     │    │  Prometheus     │    │   Grafana       │    │
│  │  Node Exporter  │◄──►│  Long-term      │◄──►│   Alertmanager  │    │
│  │  Kube-State     │    │  Storage        │    │   Thanos        │    │
│  │  cAdvisor       │    │  (Thanos)       │    │   Loki          │    │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘     │
│           │                       │                       │              │
│           ▼                       ▼                       ▼              │
│  ┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐     │
│  │   应用指标       │    │   历史数据分析    │    │   可视化展示     │    │
│  │                 │    │                 │    │                 │    │
│  │  App Metrics    │    │  Trend Analysis │    │  Dashboards     │    │
│  │  Business KPIs  │    │  Capacity Plan  │    │  Alert History  │    │
│  │  SLI/SLO        │    │  Anomaly Detect │    │  Report Gen     │    │
│  └─────────────────┘    └─────────────────┘    └─────────────────┘     │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

## Prometheus 监控体系

### 核心组件部署

```yaml
# Prometheus Operator 部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: prometheus-operator
  namespace: monitoring
spec:
  replicas: 2
  selector:
    matchLabels:
      app: prometheus-operator
  template:
    metadata:
      labels:
        app: prometheus-operator
    spec:
      serviceAccountName: prometheus-operator
      containers:
      - name: prometheus-operator
        image: quay.io/prometheus-operator/prometheus-operator:v0.68.0
        args:
        - --kubelet-service=kube-system/kubelet
        - --prometheus-config-reloader=quay.io/prometheus-operator/prometheus-config-reloader:v0.68.0
        - --thanos-default-base-image=quay.io/thanos/thanos:v0.32.0
        - --secret-field-selector=type!=helm.sh/release.v1
        ports:
        - containerPort: 8080
          name: http
        resources:
          requests:
            cpu: 100m
            memory: 100Mi
          limits:
            cpu: 200m
            memory: 200Mi
        securityContext:
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: true

---
# Prometheus 实例配置
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: k8s-prometheus
  namespace: monitoring
spec:
  replicas: 2
  serviceAccountName: prometheus-k8s
  serviceMonitorSelector:
    matchLabels:
      team: frontend
  ruleSelector:
    matchLabels:
      team: frontend
  alerting:
    alertmanagers:
    - namespace: monitoring
      name: alertmanager-main
      port: web
  resources:
    requests:
      memory: 400Mi
    limits:
      memory: 2Gi
  storage:
    volumeClaimTemplate:
      spec:
        storageClassName: fast-ssd
        resources:
          requests:
            storage: 50Gi
  enableAdminAPI: false
  externalLabels:
    cluster: production-cluster
    region: us-west-2
  remoteWrite:
  - url: http://thanos-receive.monitoring.svc:19291/api/v1/receive
    writeRelabelConfigs:
    - sourceLabels: [__name__]
      regex: ^(up|scrape_samples_scraped)$
      action: keep
```

### ServiceMonitor 配置

```yaml
# Kubernetes 核心组件监控
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: kube-apiserver
  namespace: monitoring
  labels:
    team: sre
spec:
  jobLabel: component
  selector:
    matchLabels:
      component: apiserver
      provider: kubernetes
  namespaceSelector:
    matchNames:
    - default
  endpoints:
  - port: https
    scheme: https
    tlsConfig:
      caFile: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
      serverName: kubernetes
    bearerTokenFile: /var/run/secrets/kubernetes.io/serviceaccount/token
    interval: 30s
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: (apiserver|etcd|scheduler|controller_manager)_.*
      action: keep

---
# 应用服务监控
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: application-monitoring
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: myapp
  endpoints:
  - port: metrics
    path: /metrics
    interval: 15s
    scrapeTimeout: 10s
    relabelings:
    - sourceLabels: [__meta_kubernetes_service_name]
      targetLabel: service
    - sourceLabels: [__meta_kubernetes_pod_name]
      targetLabel: pod
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: http_request_duration_seconds_bucket
      action: keep
```

### 告警规则配置

```yaml
# 核心告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: kubernetes-rules
  namespace: monitoring
  labels:
    prometheus: k8s
    role: alert-rules
spec:
  groups:
  - name: kubernetes-system
    rules:
    # 关键系统组件告警
    - alert: KubeAPIServerDown
      expr: absent(up{job="kubernetes-apiservers"} == 1)
      for: 5m
      labels:
        severity: critical
        tier: 1
      annotations:
        summary: "Kubernetes API Server不可用"
        description: "API Server在过去5分钟内无法访问，可能影响整个集群操作"
        
    - alert: KubeletDown
      expr: kubelet_up == 0
      for: 10m
      labels:
        severity: warning
        tier: 2
      annotations:
        summary: "Kubelet服务异常"
        description: "节点 {{ $labels.node }} 上的Kubelet服务停止响应"
        
    - alert: EtcdInsufficientMembers
      expr: count(etcd_server_has_leader) < 3
      for: 5m
      labels:
        severity: critical
        tier: 1
      annotations:
        summary: "Etcd成员不足"
        description: "Etcd集群成员数不足，当前只有 {{ $value }} 个成员在线"
        
    # 资源使用告警
    - alert: NodeMemoryHigh
      expr: (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100 > 90
      for: 10m
      labels:
        severity: warning
        tier: 2
      annotations:
        summary: "节点内存使用率过高"
        description: "节点 {{ $labels.instance }} 内存使用率达到 {{ $value }}%"
        
    - alert: NodeDiskSpaceLow
      expr: node_filesystem_avail_bytes / node_filesystem_size_bytes * 100 < 10
      for: 15m
      labels:
        severity: warning
        tier: 2
      annotations:
        summary: "节点磁盘空间不足"
        description: "节点 {{ $labels.instance }} 磁盘使用率超过90%，剩余空间不足"
        
    # Pod状态告警
    - alert: PodCrashLooping
      expr: rate(kube_pod_container_status_restarts_total[5m]) * 60 * 5 > 0
      for: 5m
      labels:
        severity: warning
        tier: 2
      annotations:
        summary: "Pod频繁重启"
        description: "Pod {{ $labels.pod }} 在过去5分钟内频繁重启，重启次数: {{ $value }}"
        
    - alert: PodPending
      expr: kube_pod_status_phase{phase="Pending"} == 1
      for: 10m
      labels:
        severity: warning
        tier: 2
      annotations:
        summary: "Pod长时间处于Pending状态"
        description: "Pod {{ $labels.pod }} 已经Pending超过10分钟"
```

## Grafana 可视化平台

### Grafana 部署配置

```yaml
# Grafana 高可用部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: grafana
  namespace: monitoring
spec:
  replicas: 2
  selector:
    matchLabels:
      app: grafana
  template:
    metadata:
      labels:
        app: grafana
    spec:
      containers:
      - name: grafana
        image: grafana/grafana:10.0.3
        ports:
        - containerPort: 3000
        env:
        - name: GF_SECURITY_ADMIN_USER
          valueFrom:
            secretKeyRef:
              name: grafana-credentials
              key: admin-user
        - name: GF_SECURITY_ADMIN_PASSWORD
          valueFrom:
            secretKeyRef:
              name: grafana-credentials
              key: admin-password
        - name: GF_USERS_ALLOW_SIGN_UP
          value: "false"
        - name: GF_AUTH_ANONYMOUS_ENABLED
          value: "false"
        - name: GF_SERVER_ROOT_URL
          value: "https://grafana.example.com"
        volumeMounts:
        - name: grafana-storage
          mountPath: /var/lib/grafana
        - name: grafana-config
          mountPath: /etc/grafana/provisioning
        readinessProbe:
          httpGet:
            path: /api/health
            port: 3000
          initialDelaySeconds: 60
          periodSeconds: 10
        resources:
          requests:
            cpu: 100m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 1Gi
      volumes:
      - name: grafana-storage
        persistentVolumeClaim:
          claimName: grafana-pvc
      - name: grafana-config
        configMap:
          name: grafana-provisioning

---
# 数据源配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: grafana-datasources
  namespace: monitoring
data:
  datasources.yaml: |
    apiVersion: 1
    datasources:
    - name: Prometheus
      type: prometheus
      access: proxy
      url: http://prometheus-k8s.monitoring.svc:9090
      isDefault: true
      jsonData:
        timeInterval: "30s"
        exemplarTraceIdDestinations:
        - datasourceUid: tempo
          name: traceID
    - name: Loki
      type: loki
      access: proxy
      url: http://loki.monitoring.svc:3100
      jsonData:
        maxLines: 1000
    - name: Tempo
      type: tempo
      access: proxy
      url: http://tempo.monitoring.svc:3100
      jsonData:
        tracesToLogs:
          datasourceUid: 'loki'
          tags: ['job', 'instance', 'pod', 'namespace']
          mappedTags: [{ key: 'service.name', value: 'service' }]
```

### 核心仪表盘配置

```json
{
  "dashboard": {
    "id": null,
    "title": "Kubernetes Cluster Overview",
    "timezone": "browser",
    "schemaVersion": 38,
    "version": 1,
    "refresh": "30s",
    "panels": [
      {
        "type": "stat",
        "title": "集群健康状态",
        "gridPos": { "x": 0, "y": 0, "w": 4, "h": 4 },
        "targets": [
          {
            "expr": "sum(kube_node_status_condition{condition=\"Ready\",status=\"true\"})",
            "instant": true
          }
        ],
        "thresholds": {
          "mode": "absolute",
          "steps": [
            { "color": "red", "value": null },
            { "color": "orange", "value": 2 },
            { "color": "green", "value": 3 }
          ]
        }
      },
      {
        "type": "timeseries",
        "title": "CPU使用率",
        "gridPos": { "x": 4, "y": 0, "w": 8, "h": 8 },
        "targets": [
          {
            "expr": "100 - (avg(rate(node_cpu_seconds_total{mode=\"idle\"}[5m])) * 100)",
            "legendFormat": "CPU使用率"
          }
        ]
      },
      {
        "type": "timeseries",
        "title": "内存使用率",
        "gridPos": { "x": 12, "y": 0, "w": 8, "h": 8 },
        "targets": [
          {
            "expr": "(1 - (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes)) * 100",
            "legendFormat": "内存使用率"
          }
        ]
      },
      {
        "type": "table",
        "title": "Top 10 高CPU消耗Pod",
        "gridPos": { "x": 0, "y": 8, "w": 12, "h": 8 },
        "targets": [
          {
            "expr": "topk(10, sum(rate(container_cpu_usage_seconds_total[5m])) by (pod, namespace))",
            "format": "table"
          }
        ]
      }
    ]
  }
}
```

## Alertmanager 告警管理

### 告警路由配置

```yaml
# Alertmanager 配置
global:
  resolve_timeout: 5m
  smtp_from: alertmanager@example.com
  smtp_smarthost: smtp.gmail.com:587
  smtp_auth_username: alertmanager@example.com
  smtp_auth_password: "your-app-password"
  smtp_require_tls: true

route:
  group_by: ['alertname', 'cluster', 'service']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 3h
  receiver: 'default-receiver'
  
  routes:
  # 关键告警立即通知
  - matchers:
    - severity = "critical"
    receiver: 'critical-alerts'
    group_wait: 10s
    group_interval: 1m
    repeat_interval: 30m
    
  # 警告级别告警
  - matchers:
    - severity = "warning"
    receiver: 'warning-alerts'
    group_wait: 1m
    group_interval: 10m
    repeat_interval: 2h
    
  # 信息级别告警
  - matchers:
    - severity = "info"
    receiver: 'info-alerts'
    group_wait: 5m
    group_interval: 30m
    repeat_interval: 12h

receivers:
- name: 'default-receiver'
  webhook_configs:
  - url: 'http://notification-service.monitoring.svc:8080/webhook'
    send_resolved: true

- name: 'critical-alerts'
  pagerduty_configs:
  - routing_key: "your-pagerduty-routing-key"
    severity: "critical"
    send_resolved: true
  slack_configs:
  - api_url: "https://hooks.slack.com/services/YOUR/SLACK/WEBHOOK"
    channel: '#alerts-critical'
    send_resolved: true
    title: '{{ template "slack.title" . }}'
    text: '{{ template "slack.text" . }}'

- name: 'warning-alerts'
  email_configs:
  - to: 'team@example.com'
    send_resolved: true
    html: '{{ template "email.html" . }}'
  slack_configs:
  - api_url: "https://hooks.slack.com/services/YOUR/SLACK/WEBHOOK"
    channel: '#alerts-warning'
    send_resolved: true

- name: 'info-alerts'
  webhook_configs:
  - url: 'http://logging-service.monitoring.svc:8080/info-alerts'
    send_resolved: true
```

### 告警抑制规则

```yaml
# 告警抑制配置
inhibit_rules:
- source_matchers:
  - alertname = "KubeAPIServerDown"
  target_matchers:
  - alertname =~ "KubeletDown|NodeNotReady"
  equal: ['cluster']

- source_matchers:
  - alertname = "NodeDiskSpaceLow"
  target_matchers:
  - alertname = "NodeFilesystemAlmostFull"
  equal: ['instance']

- source_matchers:
  - severity = "critical"
  target_matchers:
  - severity = "warning"
  equal: ['alertname', 'cluster']
```

## 日志收集与分析 (Loki)

### Loki 部署配置

```yaml
# Loki 分布式部署
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: loki
  namespace: monitoring
spec:
  serviceName: loki-headless
  replicas: 3
  selector:
    matchLabels:
      app: loki
  template:
    metadata:
      labels:
        app: loki
    spec:
      containers:
      - name: loki
        image: grafana/loki:2.9.2
        args:
        - -config.file=/etc/loki/loki.yaml
        ports:
        - containerPort: 3100
        volumeMounts:
        - name: config
          mountPath: /etc/loki
        - name: storage
          mountPath: /data
        readinessProbe:
          httpGet:
            path: /ready
            port: 3100
          initialDelaySeconds: 45
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: 1
            memory: 1Gi
      volumes:
      - name: config
        configMap:
          name: loki-config
  volumeClaimTemplates:
  - metadata:
      name: storage
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 50Gi

---
# Loki 配置文件
apiVersion: v1
kind: ConfigMap
metadata:
  name: loki-config
  namespace: monitoring
data:
  loki.yaml: |
    auth_enabled: false
    
    server:
      http_listen_port: 3100
    
    common:
      path_prefix: /data
      storage:
        filesystem:
          chunks_directory: /data/chunks
          rules_directory: /data/rules
      replication_factor: 3
      ring:
        kvstore:
          store: memberlist
    
    schema_config:
      configs:
      - from: 2020-10-24
        store: boltdb-shipper
        object_store: filesystem
        schema: v11
        index:
          prefix: index_
          period: 24h
    
    storage_config:
      boltdb_shipper:
        active_index_directory: /data/index
        cache_location: /data/boltdb-cache
        cache_ttl: 24h
        shared_store: filesystem
      filesystem:
        directory: /data/chunks
    
    compactor:
      working_directory: /data/compactor
      shared_store: filesystem
    
    limits_config:
      retention_period: 168h  # 7天保留
      max_entries_limit_per_query: 5000
```

### Promtail 日志收集

```yaml
# Promtail DaemonSet
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: promtail
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: promtail
  template:
    metadata:
      labels:
        app: promtail
    spec:
      serviceAccountName: promtail
      containers:
      - name: promtail
        image: grafana/promtail:2.9.2
        args:
        - -config.file=/etc/promtail/promtail.yaml
        env:
        - name: HOSTNAME
          valueFrom:
            fieldRef:
              fieldPath: spec.nodeName
        volumeMounts:
        - name: config
          mountPath: /etc/promtail
        - name: varlog
          mountPath: /var/log
          readOnly: true
        - name: varlibdockercontainers
          mountPath: /var/lib/docker/containers
          readOnly: true
        - name: runcontainerd
          mountPath: /run/containerd
          readOnly: true
        securityContext:
          runAsUser: 0
          capabilities:
            drop:
            - ALL
            add:
            - DAC_READ_SEARCH
            - CHOWN
            - SETGID
            - SETUID
      volumes:
      - name: config
        configMap:
          name: promtail-config
      - name: varlog
        hostPath:
          path: /var/log
      - name: varlibdockercontainers
        hostPath:
          path: /var/lib/docker/containers
      - name: runcontainerd
        hostPath:
          path: /run/containerd
```

## 分布式追踪 (Tempo)

### Tempo 部署配置

```yaml
# Tempo 分布式追踪
apiVersion: apps/v1
kind: Deployment
metadata:
  name: tempo
  namespace: monitoring
spec:
  replicas: 2
  selector:
    matchLabels:
      app: tempo
  template:
    metadata:
      labels:
        app: tempo
    spec:
      containers:
      - name: tempo
        image: grafana/tempo:2.3.1
        args:
        - -config.file=/conf/tempo.yaml
        ports:
        - containerPort: 3100  # Jaeger UI
        - containerPort: 14268 # Jaeger collector
        - containerPort: 4317  # OTLP gRPC
        - containerPort: 4318  # OTLP HTTP
        volumeMounts:
        - name: tempo-config
          mountPath: /conf
        - name: tempo-storage
          mountPath: /data
        readinessProbe:
          httpGet:
            path: /ready
            port: 3100
        resources:
          requests:
            cpu: 200m
            memory: 512Mi
          limits:
            cpu: 1
            memory: 2Gi
      volumes:
      - name: tempo-config
        configMap:
          name: tempo-config
      - name: tempo-storage
        persistentVolumeClaim:
          claimName: tempo-pvc

---
# Tempo 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: tempo-config
  namespace: monitoring
data:
  tempo.yaml: |
    server:
      http_listen_port: 3100
    
    distributor:
      receivers:
        jaeger:
          protocols:
            thrift_http:
              endpoint: 0.0.0.0:14268
        otlp:
          protocols:
            grpc:
              endpoint: 0.0.0.0:4317
            http:
              endpoint: 0.0.0.0:4318
    
    ingester:
      max_block_duration: 5m
    
    compactor:
      compaction:
        block_retention: 168h  # 7天保留
    
    storage:
      trace:
        backend: local
        local:
          path: /data/tempo/blocks
```

## 性能优化与调优

### Prometheus 性能优化

```yaml
# Prometheus 性能调优配置
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: optimized-prometheus
  namespace: monitoring
spec:
  # 查询优化
  query:
    lookbackDelta: 5m
    maxConcurrency: 20
    maxSamples: 50000000
    timeout: 2m
    
  # 存储优化
  storage:
    tsdb:
      outOfOrderTimeWindow: 30m
      walCompression: true
      
  # 远程写优化
  remoteWrite:
  - url: http://thanos-receive.monitoring.svc:19291/api/v1/receive
    queueConfig:
      capacity: 10000
      maxShards: 100
      minShards: 1
      maxSamplesPerSend: 500
      batchSendDeadline: 5s
      minBackoff: 30ms
      maxBackoff: 100ms
      
  # 规则评估优化
  ruleSelector:
    matchLabels:
      prometheus: k8s
  evaluationInterval: 30s
```

### 告警去噪配置

```yaml
# 智能告警去噪
groups:
- name: smart-alerting
  rules:
  # 基于历史数据的智能告警
  - alert: HighErrorRateAnomaly
    expr: |
      rate(http_requests_total{status=~"5.."}[5m]) > 
      (avg_over_time(rate(http_requests_total{status=~"5.."}[1h])[1d:5m]) * 2)
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "HTTP错误率异常增长"
      
  # 趋势预测告警
  - alert: ResourceExhaustionPredicted
    expr: |
      predict_linear(node_filesystem_free_bytes[1h], 4*3600) < 0
    for: 30m
    labels:
      severity: warning
    annotations:
      summary: "预计4小时内磁盘空间耗尽"
```

## 监控最佳实践

### SLO驱动的监控策略

```yaml
# SLO定义和告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: slo-rules
  namespace: monitoring
spec:
  groups:
  - name: slo-monitoring
    rules:
    # 可用性SLO (99.9%)
    - record: http_request:availability_30d
      expr: |
        1 - (
          sum(rate(http_requests_total{status=~"5.."}[30d]))
          /
          sum(rate(http_requests_total[30d]))
        )
        
    - alert: AvailabilityBelowSLO
      expr: http_request:availability_30d < 0.999
      for: 1h
      labels:
        severity: critical
      annotations:
        summary: "HTTP可用性低于SLO目标(99.9%)"
        
    # 延迟SLO (95%请求<200ms)
    - record: http_request:latency_95p_30d
      expr: histogram_quantile(0.95, rate(http_request_duration_seconds_bucket[30d]))
      
    - alert: LatencyAboveSLO
      expr: http_request:latency_95p_30d > 0.2
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "HTTP延迟高于SLO目标(95% < 200ms)"
```

### 多维度监控策略

```yaml
# 多维度监控标签
global:
  external_labels:
    cluster: production-cluster
    region: us-west-2
    environment: production
    team: platform-team

# 应用监控标签标准化
metric_labels:
  mandatory:
    - cluster
    - namespace
    - pod
    - service
    - version
  optional:
    - team
    - environment
    - region
    - owner
```

## 故障排除工具

### 监控系统诊断脚本

```bash
#!/bin/bash
# monitoring-diagnostics.sh

check_prometheus_health() {
    echo "=== Prometheus健康检查 ==="
    kubectl exec -n monitoring prometheus-k8s-0 -- wget -qO- http://localhost:9090/-/healthy
    
    echo "检查存储空间:"
    kubectl exec -n monitoring prometheus-k8s-0 -- df -h /prometheus
    
    echo "检查TSDB状态:"
    kubectl exec -n monitoring prometheus-k8s-0 -- \
        curl -s http://localhost:9090/api/v1/status/tsdb | jq '.data'
}

check_alertmanager_status() {
    echo "=== Alertmanager状态检查 ==="
    kubectl exec -n monitoring alertmanager-main-0 -- \
        curl -s http://localhost:9093/api/v2/status | jq '.clusterStatus'
        
    echo "检查待发送告警:"
    kubectl exec -n monitoring alertmanager-main-0 -- \
        curl -s http://localhost:9093/api/v2/alerts | jq 'length'
}

check_grafana_connectivity() {
    echo "=== Grafana连接检查 ==="
    kubectl port-forward -n monitoring svc/grafana 3000:3000 &
    sleep 3
    
    # 检查数据源连接
    curl -s -u admin:admin http://localhost:3000/api/datasources | jq '.[] | {name: .name, type: .type, status: .status}'
    
    kill %1 2>/dev/null
}

# 执行诊断
check_prometheus_health
check_alertmanager_status
check_grafana_connectivity

echo "=== 诊断完成 ==="
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)