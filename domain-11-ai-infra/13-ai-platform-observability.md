# AI平台可观测性体系

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [Prometheus](https://prometheus.io/) | [OpenTelemetry](https://opentelemetry.io/) | [Grafana](https://grafana.com/)

## 一、AI平台可观测性全景架构

### 1.1 统一监控架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                          AI Platform Observability Architecture                     │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                              数据采集层 (Collection)                           │  │
│  │                                                                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │  Prometheus │  │   DCGM      │  │ OpenTelemetry│  │   Fluentd   │          │  │
│  │  │   Server    │  │  Exporter   │  │   Collector  │  │   Agent     │          │  │
│  │  │             │  │             │  │             │  │             │          │  │
│  │  │ • K8s指标   │  │ • GPU利用率 │  │ • 推理延迟  │  │ • 应用日志  │          │  │
│  │  │ • Pod状态   │  │ • 显存使用  │  │ • Token数   │  │ • 训练日志  │          │  │
│  │  │ • 节点资源  │  │ • 温度功耗  │  │ • 错误率    │  │ • 系统日志  │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  │                                                                               │  │
│  └─────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                              存储层 (Storage)                                 │  │
│  │                                                                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │  Prometheus │  │    Loki     │  │   Tempo     │  │   Mimir     │          │  │
│  │  │   TSDB      │  │   Log Store │  │ Trace Store │  │   Backend   │          │  │
│  │  │             │  │             │  │             │  │             │          │  │
│  │  │ • 15天保留  │  │ • 30天保留  │  │ • 3天保留   │  │ • 长期存储  │          │  │
│  │  │ • 100GB     │  │ • 500GB     │  │ • 50GB      │  │ • S3/GCS    │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  │                                                                               │  │
│  └─────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                             分析处理层 (Processing)                           │  │
│  │                                                                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │ Alertmanager│  │   Grafana   │  │  Pyroscope  │  │  Custom     │          │  │
│  │  │             │  │             │  │             │  │  Analytics  │          │  │
│  │  │ • 告警路由  │  │ • 可视化    │  │ • 性能分析  │  │ • 成本分析  │          │  │
│  │  │ • 抑制静默  │  │ • 仪表板    │  │ • 火焰图    │  │ • 趋势预测  │          │  │
│  │  │ • 通知分组  │  │ • 探索查询  │  │ • 持续分析  │  │ • 异常检测  │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  │                                                                               │  │
│  └─────────────────────────────────────┬─────────────────────────────────────────┘  │
│                                       │                                             │
│                                       ▼                                             │
│  ┌───────────────────────────────────────────────────────────────────────────────┐  │
│  │                             展示告警层 (Presentation)                         │  │
│  │                                                                               │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          │  │
│  │  │   Grafana   │  │   Slack     │  │    Email    │  │  Webhook    │          │  │
│  │  │  Dashboards │  │  Channels   │  │  Templates  │  │ Integrations│          │  │
│  │  │             │  │             │  │             │  │             │          │  │
│  │  │ • GPU监控   │  │ • 实时告警  │  │ • 详细报告  │  │ • 自动修复  │          │  │
│  │  │ • 成本分析  │  │ • 升级通知  │  │ • 周报月报  │  │ • 运维工单  │          │  │
│  │  │ • 模型性能  │  │ • 团队频道  │  │ • 管理汇报  │  │ • CMDB同步  │          │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘          │  │
│  │                                                                               │  │
│  └───────────────────────────────────────────────────────────────────────────────┘  │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 监控指标体系

#### 核心监控维度

| 维度 | 指标类别 | 关键指标 | 告警阈值 | 采集频率 |
|------|----------|----------|----------|----------|
| **基础设施** | 节点/GPU/网络 | CPU/Memory/GPU Utilization | 80%/85%/90% | 15s |
| **训练作业** | 训练进度/资源 | Loss/Accuracy/GPU Mem | Loss突增/准确率下降 | 30s |
| **推理服务** | 性能/质量 | Latency/Throughput/Error Rate | P99>500ms/错误率>1% | 10s |
| **存储系统** | 容量/性能 | Disk IO/Latency/Usage | 使用率>85%/延迟>100ms | 30s |
| **成本管理** | 资源消耗 | GPU Hours/Cost/Utilization | 成本超预算/利用率<30% | 5min |

---

## 二、Prometheus监控体系部署

### 2.1 完整监控栈部署

```yaml
# ai-monitoring-stack.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: ai-monitoring
  labels:
    istio-injection: enabled
---
# Prometheus Server配置
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: ai-prometheus
  namespace: ai-monitoring
spec:
  serviceAccountName: prometheus
  serviceMonitorSelector:
    matchLabels:
      team: ai-platform
  ruleSelector:
    matchLabels:
      team: ai-platform
  resources:
    requests:
      memory: 4Gi
      cpu: 2
    limits:
      memory: 8Gi
      cpu: 4
  storage:
    volumeClaimTemplate:
      spec:
        storageClassName: fast-ssd
        resources:
          requests:
            storage: 200Gi
  # 远程写配置 - 长期存储
  remoteWrite:
  - url: http://mimir-remote-write:9009/api/v1/push
    writeRelabelConfigs:
    - sourceLabels: [__name__]
      regex: '(kubecost|dcgm|llm)_.*'
      action: keep
---
# GPU监控ServiceMonitor
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: dcgm-exporter
  namespace: ai-monitoring
  labels:
    team: ai-platform
spec:
  selector:
    matchLabels:
      app: dcgm-exporter
  endpoints:
  - port: metrics
    interval: 15s
    path: /metrics
    relabelings:
    - sourceLabels: [__meta_kubernetes_pod_node_name]
      targetLabel: node
    - sourceLabels: [__meta_kubernetes_pod_label_nvidia_com_gpu_product]
      targetLabel: gpu_type
---
# 推理服务监控
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: llm-inference
  namespace: ai-monitoring
  labels:
    team: ai-platform
spec:
  selector:
    matchLabels:
      app: vllm
  endpoints:
  - port: http
    interval: 10s
    path: /metrics
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'vllm:(.*)'
      targetLabel: __name__
      replacement: 'llm_$1'
```

### 2.2 GPU监控指标配置

```yaml
# dcgm-exporter-values.yaml
# DCGM Exporter Helm values
dcgmExporter:
  # 启用的指标
  metrics:
    - DCGM_FI_DEV_GPU_TEMP
    - DCGM_FI_DEV_POWER_USAGE
    - DCGM_FI_DEV_GPU_UTIL
    - DCGM_FI_DEV_MEM_COPY_UTIL
    - DCGM_FI_DEV_FB_USED
    - DCGM_FI_DEV_FB_FREE
    - DCGM_FI_DEV_SM_CLOCK
    - DCGM_FI_DEV_MEM_CLOCK
    - DCGM_FI_DEV_PCIE_TX_THROUGHPUT
    - DCGM_FI_DEV_PCIE_RX_THROUGHPUT
  
  # 自定义指标标签
  serviceMonitor:
    enabled: true
    additionalLabels:
      team: ai-platform
    
  # 资源限制
  resources:
    limits:
      cpu: 100m
      memory: 128Mi
    requests:
      cpu: 50m
      memory: 64Mi
```

---

## 三、Grafana仪表板配置

### 3.1 GPU资源监控面板

```json
{
  "dashboard": {
    "title": "AI Platform - GPU Monitoring",
    "tags": ["ai", "gpu", "infrastructure"],
    "timezone": "browser",
    "panels": [
      {
        "type": "timeseries",
        "title": "GPU Utilization by Node",
        "datasource": "Prometheus",
        "targets": [
          {
            "expr": "avg by(node, gpu_type)(DCGM_FI_DEV_GPU_UTIL)",
            "legendFormat": "{{node}} - {{gpu_type}}"
          }
        ],
        "thresholds": [
          { "value": 80, "color": "orange" },
          { "value": 90, "color": "red" }
        ]
      },
      {
        "type": "stat",
        "title": "Total GPU Hours Today",
        "datasource": "Prometheus",
        "targets": [
          {
            "expr": "sum(increase(DCGM_FI_DEV_GPU_UTIL[24h])) / 100",
            "instant": true
          }
        ]
      }
    ]
  }
}
```

### 3.2 推理服务性能面板

```json
{
  "dashboard": {
    "title": "LLM Inference Performance",
    "panels": [
      {
        "type": "timeseries",
        "title": "Request Latency (P50/P95/P99)",
        "targets": [
          {
            "expr": "histogram_quantile(0.50, sum(rate(llm_request_duration_seconds_bucket[5m])) by (le))",
            "legendFormat": "P50"
          },
          {
            "expr": "histogram_quantile(0.95, sum(rate(llm_request_duration_seconds_bucket[5m])) by (le))",
            "legendFormat": "P95"
          },
          {
            "expr": "histogram_quantile(0.99, sum(rate(llm_request_duration_seconds_bucket[5m])) by (le))",
            "legendFormat": "P99"
          }
        ]
      },
      {
        "type": "gauge",
        "title": "Current Throughput (req/sec)",
        "targets": [
          {
            "expr": "sum(rate(llm_requests_total[1m]))",
            "instant": true
          }
        ],
        "thresholds": [
          { "value": 100, "color": "green" },
          { "value": 50, "color": "orange" },
          { "value": 10, "color": "red" }
        ]
      }
    ]
  }
}
```

---

## 四、告警规则配置

### 4.1 核心告警规则

```yaml
# ai-alert-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ai-platform-alerts
  namespace: ai-monitoring
  labels:
    team: ai-platform
spec:
  groups:
  - name: ai.gpu.rules
    rules:
    # GPU利用率过高告警
    - alert: HighGPUUtilization
      expr: avg by(node)(DCGM_FI_DEV_GPU_UTIL) > 95
      for: 5m
      labels:
        severity: warning
        team: ai-platform
      annotations:
        summary: "GPU利用率过高 ({{ $labels.node }})"
        description: "节点 {{ $labels.node }} 上GPU平均利用率达到 {{ $value }}%"
        
    # GPU温度异常告警
    - alert: HighGPUTemperature
      expr: DCGM_FI_DEV_GPU_TEMP > 85
      for: 2m
      labels:
        severity: critical
        team: ai-platform
      annotations:
        summary: "GPU温度过高 ({{ $labels.node }})"
        description: "节点 {{ $labels.node }} 上GPU温度达到 {{ $value }}°C"
        
  - name: ai.inference.rules
    rules:
    # 推理延迟过高告警
    - alert: HighInferenceLatency
      expr: histogram_quantile(0.99, sum(rate(llm_request_duration_seconds_bucket[5m])) by (le)) > 0.5
      for: 3m
      labels:
        severity: warning
        service: llm-inference
      annotations:
        summary: "推理延迟过高"
        description: "P99延迟超过500ms，当前值: {{ $value }}s"
        
    # 推理错误率上升告警
    - alert: HighInferenceErrorRate
      expr: sum(rate(llm_requests_failed_total[5m])) / sum(rate(llm_requests_total[5m])) > 0.01
      for: 2m
      labels:
        severity: critical
        service: llm-inference
      annotations:
        summary: "推理错误率异常"
        description: "错误率超过1%，当前值: {{ $value | humanizePercentage }}"
```

### 4.2 告警通知配置

```yaml
# alertmanager-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: alertmanager-config
  namespace: ai-monitoring
data:
  alertmanager.yml: |
    global:
      resolve_timeout: 5m
      smtp_smarthost: 'smtp.company.com:587'
      smtp_from: 'alerts@company.com'
      
    route:
      group_by: ['alertname', 'team']
      group_wait: 30s
      group_interval: 5m
      repeat_interval: 3h
      
      # AI平台告警路由
      routes:
      - match:
          team: ai-platform
        receiver: ai-slack
        continue: true
        
      - match:
          severity: critical
        receiver: ai-critical
        
    receivers:
    - name: ai-slack
      slack_configs:
      - api_url: 'https://hooks.slack.com/services/YOUR/SLACK/WEBHOOK'
        channel: '#ai-platform-alerts'
        send_resolved: true
        title: '{{ template "slack.title" . }}'
        text: '{{ template "slack.text" . }}'
        
    - name: ai-critical
      email_configs:
      - to: 'ai-team@company.com'
        send_resolved: true
        html: '{{ template "email.html" . }}'
```

---

## 五、日志收集与分析

### 5.1 Fluentd配置

```yaml
# fluentd-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluentd-config
  namespace: ai-monitoring
data:
  fluent.conf: |
    <source>
      @type tail
      path /var/log/containers/*_ai-*_*.log
      pos_file /var/log/fluentd-containers.log.pos
      tag kubernetes.*
      read_from_head true
      <parse>
        @type json
        time_key time
        time_format %Y-%m-%dT%H:%M:%S.%NZ
      </parse>
    </source>
    
    <filter kubernetes.**>
      @type kubernetes_metadata
      @id filter_kube_metadata
    </filter>
    
    # AI特定日志处理
    <filter kubernetes.var.log.containers.*_ai-*_*.log>
      @type record_transformer
      <record>
        log_type ${if record["log"].include?("TRAINING"); "training";
                 elsif record["log"].include?("INFERENCE"); "inference";
                 else "general"; end}
        model_name ${record["kubernetes"]["labels"]["model"] || "unknown"}
        job_name ${record["kubernetes"]["labels"]["job-name"] || "unknown"}
      </record>
    </filter>
    
    <match kubernetes.**>
      @type loki
      url "http://loki:3100"
      tenant_id "ai-platform"
      <label>
        job kubernetes
        node ${record.dig("kubernetes", "host")}
        namespace ${record.dig("kubernetes", "namespace_name")}
        pod ${record.dig("kubernetes", "pod_name")}
        container ${record.dig("kubernetes", "container_name")}
        log_type ${record["log_type"]}
        model_name ${record["model_name"]}
      </label>
    </match>
```

### 5.2 Loki查询示例

```logql
# 查询训练日志中的loss值变化
{namespace="ai-training", log_type="training"} 
|~ "loss=" 
| regexp "loss=(?P<loss>[0-9.]+)" 
| unwrap loss 
| __error__="" 

# 查询推理错误日志
{namespace="ai-inference", log_type="inference"} 
|= "ERROR" 
| json 
| level="ERROR"

# 统计各模型的错误次数
sum by(model_name) (
  count_over_time(
    {namespace="ai-inference"} |= "ERROR" [1h]
  )
)
```

---

## 六、成本监控集成

### 6.1 Kubecost集成配置

```yaml
# kubecost-ai-integration.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: kubecost-ai-config
  namespace: ai-monitoring
data:
  # AI工作负载成本分配规则
  cost-analyzer-config.yaml: |
    # 按标签分配成本
    allocation:
      labels:
        - team
        - project
        - model
        - environment
        - cost-center
      
      # AI特定的分摊规则
      sharedNamespaces:
        - ai-monitoring
        - ai-ops
      sharedLabels:
        team: ai-platform
        
    # GPU成本自定义定价
    pricing:
      customPrices:
        GPU:
          nvidia.com/gpu:
            price: "32.77"  # A100按需价格
            spotPrice: "10.00"
            
    # 成本告警配置
    alerts:
      budgets:
        - name: "ai-monthly-budget"
          amount: 50000
          aggregation: "namespace"
          filter: "namespace: ai-*"
          window: "month"
          
      efficiency:
        - name: "gpu-utilization"
          threshold: 30
          filter: "label_app: vllm or label_app: training-job"
          window: "7d"
```

### 6.2 成本可视化面板

```json
{
  "dashboard": {
    "title": "AI Platform Cost Analysis",
    "panels": [
      {
        "type": "timeseries",
        "title": "Daily GPU Cost Trend",
        "targets": [
          {
            "expr": "sum by(namespace)(kubecost_node_gpu_hourly_cost * 24)",
            "legendFormat": "{{namespace}}"
          }
        ]
      },
      {
        "type": "piechart",
        "title": "Cost by Model Type",
        "targets": [
          {
            "expr": "topk(5, sum by(label_model)(kubecost_namespace_gpu_cost_total))",
            "legendFormat": "{{label_model}}"
          }
        ]
      }
    ]
  }
}
```

---

## 七、运维最佳实践

### 7.1 监控配置检查清单

✅ **基础设施监控**
- [ ] 所有GPU节点都有DCGM Exporter
- [ ] 节点资源使用率监控到位
- [ ] 网络带宽和延迟监控配置
- [ ] 存储IO和容量监控启用

✅ **应用性能监控**
- [ ] 训练任务Loss/Accuracy指标采集
- [ ] 推理服务延迟/吞吐量监控
- [ ] 模型版本和部署状态跟踪
- [ ] 错误率和成功率监控

✅ **告警配置**
- [ ] 关键指标都有相应告警规则
- [ ] 告警分级和路由配置正确
- [ ] 通知渠道测试通过
- [ ] 告警抑制和静默规则设置

✅ **日志管理**
- [ ] 结构化日志格式统一
- [ ] 关键字段都有适当标签
- [ ] 日志保留策略明确
- [ ] 异常日志能够快速检索

### 7.2 常见问题排查

**GPU监控无数据**
```bash
# 检查DCGM Exporter状态
kubectl get pods -n ai-monitoring -l app=dcgm-exporter
kubectl logs -n ai-monitoring -l app=dcgm-exporter

# 验证指标采集
kubectl port-forward svc/dcgm-exporter 9400:9400
curl http://localhost:9400/metrics | grep DCGM_FI
```

**推理延迟告警频繁**
```bash
# 检查推理服务资源使用
kubectl top pods -n ai-inference -l app=vllm
kubectl describe nodes | grep -A 10 "Allocated resources"

# 分析慢查询日志
kubectl logs -n ai-inference -l app=vllm --since=1h | grep "slow"
```

**成本超出预算**
```bash
# 查看实时成本
kubectl get --raw /apis/metrics.k8s.io/v1beta1/namespaces/ai-training/pods

# 分析资源浪费
kubectl get pods -n ai-training -o wide | grep -E "(Pending|Evicted)"
```

---