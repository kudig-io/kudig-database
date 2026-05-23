---
title: 06 - 工作负载监控与告警体系 (Workload Monitoring & Alerting System)
description: 'title: 06 - 工作负载监控与告警体系 (Workload Monitoring & Alerting System)'
category: general
tags:
- k8s
- workload
- pod
- deployment
- monitoring
- etcd
- prometheus
- grafana
- jaeger
- statefulset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 如何监控06-workload-monitoring-alerting？
- 06-workload-monitoring-alerting的关键指标有哪些？
- 06-workload-monitoring-alerting的告警规则
trigger_keywords:
- 工作负载监控与告警体系
- Workload
- Monitoring
- Alerting
- System
- workloads
- applications
prerequisites:
- kubectl-basics
- pod-lifecycle
- prometheus-basics
- monitoring-basics
- etcd-basics
- logging-basics
- tracing-basics
created: "2026-05-23"
---

title: 06 - 工作负载监控与告警体系 (Workload Monitoring & Alerting System)
description: '# 06 - 工作负载监控与告警体系 (Workload Monitoring & Alerting System)'
category: workloads
tags:
- k8s
- workload
- pod
- deployment
- [[StatefulSet|statefulset]]
- [[etcd|etcd]]
- prometheus
- grafana
- jaeger
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 工作负载监控与告警体系 (Workload Monitoring & Alerting System) 是什么
- 如何 工作负载监控与告警体系 (Workload Monitoring & Alerting System)
- Kubernetes 4 workloads 最佳实践
trigger_keywords:
- 工作负载监控与告警体系
- Workload
- Monitoring
- Alerting
- System
- workloads
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/monitoring-fta.md
  label: '故障树: monitoring'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
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

# 06 - 工作负载监控与告警体系 (Workload Monitoring & Alerting System)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Monitoring Guide](https://kubernetes.io/docs/tasks/debug/debug-cluster/resource-metrics-pipeline/)

<!-- chunk: 监控体系架构设计 -->
## 监控体系架构设计

### 1. 监控层次模型

```mermaid
graph TD
    A[监控体系] --> B[基础设施层]
    A --> C[平台层]
    A --> D[应用层]
    A --> E[业务层]
    
    B --> B1[节点指标]
    B --> B2[资源使用率]
    B --> B3[硬件健康]
    
    C --> C1[Pod状态]
    C --> C2[控制器指标]
    C --> C3[API Server]
    C --> C4[etcd性能]
    
    D --> D1[应用性能]
    D --> D2[自定义指标]
    D --> D3[日志聚合]
    D --> D4[追踪数据]
    
    E --> E1[业务SLI]
    E --> E2[用户体验]
    E --> E3[收入指标]
```

### 2. 核心监控组件栈

#### 2.1 Prometheus 监控栈

```yaml
# Prometheus Operator 部署配置
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: k8s-prometheus
  namespace: monitoring
spec:
  serviceAccountName: prometheus
  serviceMonitorSelector:
    matchLabels:
      team: frontend
  ruleSelector:
    matchLabels:
      role: alert-rules
  resources:
    requests:
      memory: 400Mi
  enableAdminAPI: false
  
  # 持久化存储
  storage:
    volumeClaimTemplate:
      spec:
        storageClassName: fast-ssd
        resources:
          requests:
            storage: 50Gi
  
  # 远程写配置
  remoteWrite:
  - url: http://thanos-receive.monitoring.svc:19291/api/v1/receive
    writeRelabelConfigs:
    - sourceLabels: [__name__]
      regex: 'container_.+'
      action: keep
```

#### 2.2 ServiceMonitor 配置模板

```yaml
# 应用服务监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: application-monitor
  namespace: production
  labels:
    app: my-app
    team: devops
spec:
  selector:
    matchLabels:
      app: my-app
  namespaceSelector:
    matchNames:
    - production
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
    scheme: http
    bearerTokenFile: /var/run/secrets/kubernetes.io/serviceaccount/token
    tlsConfig:
      insecureSkipVerify: true
    
    # 指标重标记
    relabelings:
    - sourceLabels: [__meta_kubernetes_pod_name]
      targetLabel: pod
    - sourceLabels: [__meta_kubernetes_namespace]
      targetLabel: namespace
    
    # 指标过滤
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'go_.+'
      action: drop
```

### 3. 关键监控指标体系

#### 3.1 Pod 级别核心指标

```promql
# Pod 状态监控
groups:
- name: pod_health
  rules:
  # Pod 不可用检测
  - alert: PodNotReady
    expr: kube_pod_status_ready{condition="true"} == 0
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 未就绪超过5分钟"
  
  # Pod 重启频繁
  - alert: PodCrashLooping
    expr: rate(kube_pod_container_status_restarts_total[5m]) > 0.1
    for: 10m
    labels:
      severity: critical
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 重启过于频繁"
  
  # Pod 等待状态
  - alert: PodPending
    expr: kube_pod_status_phase{phase="Pending"} == 1
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 长时间处于Pending状态"
```

#### 3.2 Deployment 控制器指标

```promql
# Deployment 健康度监控
groups:
- name: deployment_health
  rules:
  # 副本数不匹配
  - alert: DeploymentReplicasMismatch
    expr: |
      kube_deployment_status_replicas_available != 
      kube_deployment_spec_replicas
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} 副本数不匹配"
  
  # 滚动更新卡住
  - alert: DeploymentStuck
    expr: |
      kube_deployment_status_replicas_updated != 
      kube_deployment_spec_replicas
    for: 15m
    labels:
      severity: critical
    annotations:
      summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} 更新卡住"
  
  # 更新失败
  - alert: DeploymentGenerationMismatch
    expr: |
      kube_deployment_status_observed_generation != 
      kube_deployment_metadata_generation
    for: 15m
    labels:
      severity: critical
    annotations:
      summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} 观察世代不匹配"
```

#### 3.3 资源使用率监控

```promql
# 资源使用率告警
groups:
- name: resource_utilization
  rules:
  # CPU 使用率过高
  - alert: ContainerCPULimitReached
    expr: |
      rate(container_cpu_usage_seconds_total{container!="POD",container!=""}[5m]) /
      container_spec_cpu_quota * 100 > 85
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "容器 {{ $labels.namespace }}/{{ $labels.pod }}/{{ $labels.container }} CPU使用率超过85%"
  
  # 内存使用率过高
  - alert: ContainerMemoryLimitReached
    expr: |
      container_memory_working_set_bytes{container!="POD",container!=""} /
      container_spec_memory_limit_bytes * 100 > 90
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "容器 {{ $labels.namespace }}/{{ $labels.pod }}/{{ $labels.container }} 内存使用率超过90%"
  
  # 节点资源紧张
  - alert: NodeMemoryPressure
    expr: |
      (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes * 100) < 15
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "节点 {{ $labels.node }} 可用内存不足15%"
```

### 4. 高级监控策略

#### 4.1 黑盒监控配置

```yaml
# Blackbox Exporter 配置
apiVersion: monitoring.coreos.com/v1
kind: Probe
metadata:
  name: application-health-check
  namespace: monitoring
spec:
  jobName: application-probe
  prober:
    url: blackbox-exporter.monitoring.svc:9115
  module: http_2xx
  targets:
    staticConfig:
      static:
      - http://my-app.production.svc.cluster.local:8080/health
      - https://app.example.com/health
      relabeling:
      - sourceLabels: [__address__]
        targetLabel: __param_target
      - sourceLabels: [__param_target]
        targetLabel: instance
      - targetLabel: __address__
        replacement: blackbox-exporter.monitoring.svc:9115
```

#### 4.2 日志监控集成

```yaml
# Loki 日志监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: loki-monitor
  namespace: logging
spec:
  selector:
    matchLabels:
      app: loki
  endpoints:
  - port: http-metrics
    interval: 30s
    path: /metrics
```

#### 4.3 分布式追踪监控

```yaml
# Jaeger 追踪监控
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: jaeger-monitor
  namespace: tracing
spec:
  selector:
    matchLabels:
      app: jaeger
  endpoints:
  - port: admin-http
    interval: 30s
    path: /metrics
```

### 5. 告警通知策略

#### 5.1 Alertmanager 配置

```yaml
# Alertmanager 配置
global:
  resolve_timeout: 5m
  smtp_smarthost: 'smtp.company.com:587'
  smtp_from: 'alertmanager@company.com'
  smtp_auth_username: 'alertmanager'
  smtp_auth_password: 'password'

route:
  group_by: ['alertname', 'cluster', 'service']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 3h
  receiver: 'default-receiver'
  
  routes:
  # 关键业务告警
  - match:
      severity: critical
    receiver: 'pagerduty'
    group_wait: 10s
    repeat_interval: 30m
  
  # 一般告警
  - match:
      severity: warning
    receiver: 'slack-warning'
    group_wait: 1m
    repeat_interval: 2h
  
  # 通知抑制规则
  inhibit_rules:
  - source_match:
      severity: 'critical'
    target_match:
      severity: 'warning'
    equal: ['alertname', 'namespace', 'pod']

receivers:
- name: 'default-receiver'
  email_configs:
  - to: 'team-devops@company.com'
    send_resolved: true

- name: 'pagerduty'
  pagerduty_configs:
  - service_key: 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'
    send_resolved: true

- name: 'slack-warning'
  slack_configs:
  - api_url: 'https://hooks.slack.com/services/XXX/YYY/ZZZ'
    channel: '#alerts-warning'
    send_resolved: true
    title: '{{ template "slack.warning.title" . }}'
    text: '{{ template "slack.warning.text" . }}'
```

### 6. 监控面板设计

#### 6.1 Grafana Dashboard 模板

```json
{
  "dashboard": {
    "title": "Production Workload Overview",
    "panels": [
      {
        "title": "Deployment Health Status",
        "type": "stat",
        "targets": [
          {
            "expr": "count(kube_deployment_status_replicas_available == kube_deployment_spec_replicas)",
            "legendFormat": "Healthy Deployments"
          },
          {
            "expr": "count(kube_deployment_status_replicas_available != kube_deployment_spec_replicas)",
            "legendFormat": "Unhealthy Deployments"
          }
        ]
      },
      {
        "title": "Pod Restart Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(kube_pod_container_status_restarts_total[5m])) by (namespace, pod)",
            "legendFormat": "{{namespace}}/{{pod}}"
          }
        ]
      },
      {
        "title": "Resource Utilization",
        "type": "heatmap",
        "targets": [
          {
            "expr": "rate(container_cpu_usage_seconds_total[5m])",
            "legendFormat": "CPU Usage"
          }
        ]
      }
    ]
  }
}
```

### 7. 监控最佳实践

#### 7.1 指标命名规范

```yaml
# 推荐的指标命名格式
# {application}_{component}_{metric}_{unit}
# 示例：
http_requests_total: 计数器类型
http_request_duration_seconds: 直方图类型
cpu_usage_ratio: 比率类型
memory_bytes: 字节单位
```

#### 7.2 标签设计原则

```yaml
# 标准化标签体系
labels:
  namespace: "应用命名空间"
  pod: "Pod名称"
  container: "容器名称"
  app: "应用名称"
  version: "应用版本"
  tier: "应用层级(frontend/backend)"
  env: "环境(production/staging)"
  team: "负责团队"
  owner: "负责人"
```

#### 7.3 告警级别定义

| 级别 | 响应时间 | 通知渠道 | 处理要求 |
|------|----------|----------|----------|
| **Critical** | 5分钟内 | 电话/PagerDuty | 立即处理 |
| **Warning** | 30分钟内 | Slack/Email | 2小时内处理 |
| **Info** | 2小时内 | Email | 下个工作日处理 |

---

**监控原则**: 全面覆盖、分级告警、快速响应、持续优化

---
**文档维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-02-workloads-applications MOC
- [[domain-02-workloads-applications/README|Domain-4: Kubernetes工作负载管理]]
- Domain-4 工作负载 — 开源项目索引
- 01 - Kubernetes 工作负载架构概览 (Workload Architecture Overview)
- 02 - Deployment 生产模式与最佳实践 (Deployment Production Patterns)
- 03 - StatefulSet 高级运维指南 (StatefulSet Advanced Operations)
- 04 - DaemonSet 管理策略与最佳实践 (DaemonSet Management Strategies)
- 05 - Job 与 CronJob 高级用法 (Job & CronJob Advanced Usage)
- 07 - 工作负载故障排查与应急响应手册 (Workload Troubleshooting & Incident Re...
- 08 - 多云混合部署工作负载管理策略 (Multi-Cloud Hybrid Deployment Workload ...
- 09 - 边缘计算工作负载部署模式 (Edge Computing Workload Deployment Patter...
- 工作负载控制器详解

## See Also

- 04-daemonset-management
- 05-job-cronjob-advanced
- 07-workload-troubleshooting-handbook
- 08-multi-cloud-workload-strategy

## Related

- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
