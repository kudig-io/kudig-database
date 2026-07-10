---
title: Prometheus 企业级监控部署指南
description: '# Prometheus 企业级监控部署指南'
summary: 'helm repo add prometheus-community https://prometheus-community.github.io/helm-charts'
category: enterprise-monitoring-alerting
tags:
- k8s
- monitoring
- alerting
- prometheus
- grafana
- helm
- hpa
- statefulset
- daemonset
- job
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
- Prometheus 企业级监控部署指南 是什么
- 如何 Prometheus 企业级监控部署指南
- Kubernetes 20 enterprise monitoring alerting 最佳实践
trigger_keywords:
- Prometheus
- 企业级监控部署指南
- enterprise
- monitoring
- alerting
prerequisites:
- kubectl-basics
- observability-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- logging-basics
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
  path: ../系统基础/topic-cheat-sheet/promql.md
  label: '速查卡: promql'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Prometheus|Prometheus]] 企业级监控部署指南

> **适用版本**: Prometheus v3.3.0 / kube-state-metrics v2.15 / Alertmanager v0.28  
> **最后更新**: 2026-04-24  
> **难度**: 中级 → 高级

---

## 📋 目录

- [一、架构设计](#一架构设计)
- [二、[[Helm|Helm]] 部署](#二helm-部署)
- [三、高可用配置](#三高可用配置)
- [四、告警规则最佳实践](#四告警规则最佳实践)
- [五、服务发现配置](#五服务发现配置)
- [六、性能调优](#六性能调优)
- [七、常见问题排查](#七常见问题排查)

---

## 一、架构设计

### 1.1 单实例架构 (< 100 节点)

```
┌────────────────────────────────────────┐
│           K8s Cluster                  │
│  ┌─────────────┐  ┌─────────────────┐  │
│  │ Prometheus  │  │ Alertmanager    │  │
│  │ (StatefulSet│  │ (StatefulSet x2)│  │
│  │  + PVC)     │  │                 │  │
│  └──────┬──────┘  └─────────────────┘  │
│         │ scrape                        │
│  ┌──────┴──────┐  ┌─────────────────┐  │
│  │kube-state   │  │node_exporter    │  │
│  │metrics      │  │(DaemonSet)      │  │
│  └─────────────┘  └─────────────────┘  │
└────────────────────────────────────────┘
```

### 1.2 联邦架构 (> 100 节点 / 多集群)

```
┌──────────────┐      ┌──────────────┐      ┌──────────────┐
│  Prometheus  │      │  Prometheus  │      │  Prometheus  │
│  Cluster A   │◄────►│  Cluster B   │◄────►│  Cluster C   │
└──────┬───────┘      └──────┬───────┘      └──────┬───────┘
       │ scrape (federation) │ scrape            │ scrape
       └─────────────────────┴───────────────────┘
                           │
                    ┌──────┴───────┐
                    │  Thanos      │
                    │  Query       │
                    │  + Store     │
                    └──────┬───────┘
                           │
                    ┌──────┴───────┐
                    │   Grafana    │
                    └──────────────┘
```

---

## 二、Helm 部署

### 2.1 kube-prometheus-stack (推荐)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加仓库
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update

# 准备自定义 values
# values-production.yaml
cat << 'EOF' > values-production.yaml
prometheus:
  prometheusSpec:
    retention: 30d
    retentionSize: "50GB"
    resources:
      requests:
        memory: "4Gi"
        cpu: "1000m"
      limits:
        memory: "8Gi"
        cpu: "2000m"
    storageSpec:
      volumeClaimTemplate:
        spec:
          storageClassName: standard
          accessModes: ["ReadWriteOnce"]
          resources:
            requests:
              storage: 100Gi
    additionalScrapeConfigs:
      # 自定义 job 示例
      - job_name: 'custom-app'
        kubernetes_sd_configs:
          - role: pod
            namespaces:
              names:
                - production
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

alertmanager:
  enabled: true
  config:
    global:
      smtp_smarthost: 'smtp.example.com:587'
      smtp_from: 'alert@example.com'
    route:
      receiver: 'default'
      group_by: ['alertname', 'namespace']
      group_wait: 30s
      group_interval: 5m
      repeat_interval: 12h
      routes:
      - match:
          severity: critical
        receiver: 'pagerduty-critical'
        continue: true
      - match:
          severity: warning
        receiver: 'slack-warning'
    receivers:
    - name: 'default'
      slack_configs:
      - api_url: 'https://hooks.slack.com/services/XXX'
        channel: '#alerts'
        title: '{% raw %}{{ .GroupLabels.alertname }}{% endraw %}'
        text: '{% raw %}{{ range .Alerts }}{{ .Annotations.summary }}\n{{ end }}{% endraw %}'
    - name: 'pagerduty-critical'
      pagerduty_configs:
      - routing_key: '<PAGERDUTY_KEY>'
    - name: 'slack-warning'
      slack_configs:
      - api_url: 'https://hooks.slack.com/services/YYY'
        channel: '#warnings'

grafana:
  enabled: true
  adminPassword: "changeme-strong-password"
  persistence:
    enabled: true
    size: 10Gi
  additionalDataSources:
    - name: Loki
      type: loki
      url: http://loki:3100
      access: proxy

kubeStateMetrics:
  enabled: true

nodeExporter:
  enabled: true
EOF

# 部署
helm install kube-prometheus-stack prometheus-community/kube-prometheus-stack \
  --namespace monitoring \
  --create-namespace \
  --values values-production.yaml \
  --version 69.8.0
```
---

## 三、高可用配置

### 3.1 Prometheus HA (Thanos Sidecar 模式)

```yaml
# 在每个 Prometheus Pod 旁部署 Thanos Sidecar
prometheus:
  prometheusSpec:
    containers:
      - name: thanos-sidecar
        image: quay.io/thanos/thanos:v0.38.0
        args:
          - sidecar
          - --tsdb.path=/prometheus
          - --prometheus.url=http://localhost:9090
          - --objstore.config-file=/etc/thanos/objstore.yml
        volumeMounts:
          - name: thanos-objstore
            mountPath: /etc/thanos
          - name: prometheus-data
            mountPath: /prometheus
    volumes:
      - name: thanos-objstore
        secret:
          secretName: thanos-objstore
```

### 3.2 Alertmanager HA

```yaml
alertmanager:
  alertmanagerSpec:
    replicas: 3  # Gossip 集群自动发现
    podAntiAffinity: hard  # 分布在不同节点
```

---

## 四、告警规则最佳实践

### 4.1 核心 K8s 告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: kubernetes-rules
  namespace: monitoring
spec:
  groups:
  - name: kubernetes-apps
    rules:
    # Pod 崩溃循环
    - alert: KubePodCrashLooping
      expr: |
        rate(kube_pod_container_status_restarts_total[10m]) > 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Pod {% raw %}{{ $labels.namespace }}/{{ $labels.pod }}{% endraw %} 正在崩溃循环"
        description: "容器 {% raw %}{{ $labels.container }}{% endraw %} 在 10 分钟内重启超过 0 次"

    # Pod 未就绪
    - alert: KubePodNotReady
      expr: |
        sum by (namespace, pod) (
          max by(namespace, pod) (
            kube_pod_status_phase{% raw %}{phase=~"Pending|Unknown"}{% endraw %}
          ) * on(namespace, pod) group_left(owner_kind) topk by(namespace, pod) (
            1, max by(namespace, pod, owner_kind) (kube_pod_owner{owner_kind!="Job"})
          )
        ) > 0
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "Pod 未就绪超过 15 分钟"

    # 节点内存压力
    - alert: NodeMemoryPressure
      expr: |
        (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) < 0.1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "节点 {% raw %}{{ $labels.instance }}{% endraw %} 内存不足"

    # 节点磁盘压力
    - alert: NodeDiskPressure
      expr: |
        (node_filesystem_avail_bytes / node_filesystem_size_bytes) < 0.1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "节点 {% raw %}{{ $labels.instance }}{% endraw %} 磁盘空间不足"

    # HPA 达到上限
    - alert: HpaMaxedOut
      expr: |
        kube_horizontalpodautoscaler_status_desired_replicas >= 
        kube_horizontalpodautoscaler_spec_max_replicas
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "HPA {% raw %}{{ $labels.horizontalpodautoscaler }}{% endraw %} 已达到最大副本数"
```

### 4.2 告警分级策略

| 级别 | 响应时间 | 通知渠道 | 示例 |
|:---|:---|:---|:---|
| critical | 5 分钟内 | PagerDuty + Slack + 电话 | 核心服务不可用、数据丢失风险 |
| warning | 30 分钟内 | Slack | 资源使用率高、Pod 重启 |
| info | 下一个工作日 | 邮件/Slack | 证书即将过期、版本可升级 |

---

## 五、服务发现配置

### 5.1 Pod 监控注解规范

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  template:
    metadata:
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "8080"
        prometheus.io/path: "/metrics"
    spec:
      containers:
      - name: app
        ports:
        - containerPort: 8080
          name: metrics
```

### 5.2 ServiceMonitor (Prometheus Operator)

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: myapp-metrics
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: myapp
  namespaceSelector:
    matchNames:
      - production
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
    honorLabels: true
```

---

## 六、性能调优

### 6.1 水平扩展参数

```yaml
prometheus:
  prometheusSpec:
    # 抓取并发数
    query:
      maxConcurrency: 20
    # 存储优化
    tsdb:
      minBlockDuration: 2h
      maxBlockDuration: 2h
      retentionSize: "45GB"  # 略小于 PVC 容量
    # 内存优化
    enableAdminAPI: false
    walCompression: true
```

### 6.2 远程写入 (Remote Write)

```yaml
prometheus:
  prometheusSpec:
    remoteWrite:
      - url: "http://thanos-receive:19291/api/v1/receive"
        queueConfig:
          maxSamplesPerSend: 1000
          maxShards: 200
        writeRelabelConfigs:
          - sourceLabels: [__name__]
            regex: 'go_.*'
            action: drop  # 过滤高基数指标
```

---

## 七、常见问题排查

| 问题 | 原因 | 解决 |
|:---|:---|:---|
| Prometheus OOMKilled | 抓取目标过多 / 高基数标签 | 增加内存限制、添加 relabel 过滤、减少 target |
| 磁盘快速增长 | retention 过长 / 高 cardinality | 缩短 retention、启用压缩、过滤无用指标 |
| 查询超时 | 复杂查询 / 数据量大 | 增加 query.timeout、使用 recording rules |
| Target 显示 down | 网络不通 / 指标路径错误 | 检查 [[Service|Service]]/Pod 注解、网络策略 |
| Alertmanager 未触发 | 路由规则不匹配 / inhibit | 检查 alertmanager 配置、路由树 |

---

## 参考链接

- [kube-prometheus-stack Helm Chart](https://github.com/prometheus-community/helm-charts/tree/main/charts/kube-prometheus-stack)
- [Prometheus 配置文档](https://prometheus.io/docs/prometheus/latest/configuration/configuration/)
- [Alertmanager 配置](https://prometheus.io/docs/alerting/latest/configuration/)
- [Thanos 部署指南](https://thanos.io/tip/thanos/getting-started.md/)

---

## Obsidian 相关文档

- observability/MOC.md|domain-20-enterprise-monitoring-alerting MOC]]
- [[可观测性/README.md|Domain 06: 企业级监控与告警 (Enterprise Monitoring & Alerting)]]
- [[可观测性/00-open-source-projects-index.md|Domain-20 企业监控与告警 — 开源项目索引]]
- Prometheus企业级监控系统深度实践
- Grafana Enterprise Observability Platform 深度实践
- OpenTelemetry分布式追踪与可观测性深度实践
- Thanos Enterprise Metrics Federation and Long-term Storage
- Datadog企业级APM深度实践
- Datadog 企业级监控平台深度实践
- Elastic Stack企业级日志分析深度实践
- Elastic Stack企业级可观测性平台深度实践
- Zabbix Enterprise Monitoring Platform 深度实践

## See Also

- 08-new-relic-enterprise-apm
- 99-distributed-tracing-guide
- 01-prometheus-enterprise-monitoring
- 02-grafana-enterprise-observability

- [[可观测性/README.md|返回目录]]

## Related

- [[生态参考/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]

```

<!-- risk-assessed -->
