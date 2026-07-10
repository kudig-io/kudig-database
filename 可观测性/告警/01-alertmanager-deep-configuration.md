---
title: Alertmanager 深度配置
description: 'Alertmanager 深度配置：高可用集群部署、路由树设计、抑制规则/静默规则、分组策略、Receiver 集成'
summary: 'Alertmanager 高可用、路由树、抑制/静默规则与 Receiver 集成'
category: observability
tags:
- alertmanager
- alerting
- prometheus
- routing
- notification
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 安全工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Alertmanager 深度配置是什么
- 如何配置 Alertmanager 高可用集群
trigger_keywords:
- Alertmanager
- 告警路由
- 抑制规则
- 静默规则
- Receiver
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Alertmanager 深度配置

## 概述

Alertmanager 负责接收、去重、分组和路由 Prometheus 产生的告警，并发送到各种通知渠道。本文档涵盖高可用部署、路由树设计、抑制/静默规则和主流 Receiver 集成。

## 1. 高可用集群部署

### 1.1 Gossip 集群架构

```
┌──────────────────────────────────────────────────────────┐
│              Alertmanager HA Cluster                      │
│                                                          │
│  ┌─────────────┐   ┌─────────────┐   ┌─────────────┐   │
│  │ AM-1        │   │ AM-2        │   │ AM-3        │   │
│  │ :9093       │   │ :9093       │   │ :9093       │   │
│  │ :9094(Gossip)│   │ :9094(Gossip)│   │ :9094(Gossip)│   │
│  └──────┬──────┘   └──────┬──────┘   └──────┬──────┘   │
│         │                 │                 │            │
│         └─────────────────┼─────────────────┘            │
│                           │                              │
│                    Gossip Protocol                        │
│                    (去重 + 路由)                          │
└──────────────────────────────────────────────────────────┘
```

### 1.2 Kubernetes 部署配置

```yaml
apiVersion: monitoring.coreos.com/v1
kind: Alertmanager
metadata:
  name: main
  namespace: monitoring
spec:
  replicas: 3
  image: prom/alertmanager:v0.27.0
  resources:
    requests:
      cpu: 200m
      memory: 256Mi
    limits:
      cpu: 500m
      memory: 512Mi
  portName: web
  listenLocal: false
  securityContext:
    fsGroup: 2000
    runAsNonRoot: true
    runAsUser: 1000
  storage:
    volumeClaimTemplate:
      spec:
        accessModes: ["ReadWriteOnce"]
        resources:
          requests:
            storage: 10Gi
  nodeSelector:
    kubernetes.io/os: linux
  tolerations:
  - key: "node-role.kubernetes.io/control-plane"
    effect: "NoSchedule"
  alertmanagerConfigSelector:
    matchLabels:
      alertmanager: main
```

### 1.3 Gossip 配置

```yaml
# alertmanager.yaml
global:
  resolve_timeout: 5m

# Gossip 配置（Kubernetes Service Discovery）
cluster:
  listen-address: "0.0.0.0:9094"
  peers:
  - alertmanager-0.alertmanager-operated:9094
  - alertmanager-1.alertmanager-operated:9094
  - alertmanager-2.alertmanager-operated:9094
  settle_timeout: 15s
  retransmit_factor: 4
  probe_interval: 5s
  probe_timeout: 500ms
```

## 2. 路由树设计

### 2.1 多级路由策略

```yaml
route:
  # 默认接收者
  receiver: slack-default
  # 默认分组
  group_by: ['alertname', 'namespace', 'severity']
  # 分组等待时间
  group_wait: 30s
  # 分组间隔
  group_interval: 5m
  # 重复间隔
  repeat_interval: 4h

  routes:
  # P0 紧急告警（立即通知）
  - match:
      severity: critical
    receiver: pagerduty-critical
    group_wait: 10s
    group_interval: 1m
    repeat_interval: 15m
    continue: true

  # 生产环境告警
  - match_re:
      namespace: ^production$
    receiver: slack-production
    group_by: ['alertname', 'namespace', 'pod']
    routes:
    # API 服务告警
    - match:
        app: api-gateway
      receiver: slack-api-team
      group_wait: 15s

    # 数据库告警
    - match:
        app: postgresql
      receiver: slack-dba-team
      group_wait: 10s

    # 告警升级（15 分钟未解决）
    - match:
        severity: critical
      receiver: pagerduty-critical
      routes:
      - match:
          namespace: production
        receiver: phone-call
        group_wait: 5s
        repeat_interval: 5m

  # 非生产环境
  - match_re:
      namespace: ^(staging|development)$
    receiver: slack-staging
    group_wait: 5m
    repeat_interval: 12h

  # Watchdog（存活探针）
  - match:
      alertname: Watchdog
    receiver: null
```

### 2.2 继承与覆盖

```yaml
route:
  receiver: default-receiver
  group_by: ['alertname']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 4h

  routes:
  # 子路由继承父路由的 group_by，但可以覆盖
  - match:
      severity: critical
    receiver: critical-receiver
    # 覆盖分组策略
    group_by: ['alertname', 'namespace', 'pod']
    # 覆盖时间参数
    group_wait: 10s
    repeat_interval: 15m
    # 继承父路由的 match（默认 false）
    continue: false
```

## 3. 抑制规则

### 3.1 级联抑制

```yaml
inhibit_rules:
# 节点 Down 时抑制该节点上的所有 Pod 告警
- source_match:
    alertname: NodeDown
    severity: critical
  target_match_re:
    alertname: .*
  equal: ['node']

# 集群级别告警抑制命名空间级别告警
- source_match:
    alertname: ClusterUnhealthy
    severity: critical
  target_match:
    severity: warning
  equal: ['cluster']

# 关键服务 Down 抑制该服务的性能告警
- source_match:
    alertname: ServiceDown
    severity: critical
  target_match_re:
    alertname: (HighLatency|HighErrorRate)
  equal: ['namespace', 'service']

# P0 告警抑制 P1/P2 告警
- source_match:
    severity: critical
  target_match:
    severity: warning
  equal: ['namespace', 'alertname']
```

### 3.2 时间窗口抑制

```yaml
inhibit_rules:
# 维护窗口抑制
- source_match:
    alertname: MaintenanceWindow
  target_match_re:
    alertname: .*
  equal: ['namespace']

# 部署期间抑制
- source_match:
    alertname: DeploymentInProgress
  target_match_re:
    alertname: (PodRestart|ContainerCrashLooping)
  equal: ['namespace', 'deployment']
```

## 4. 静默规则

### 4.1 API 创建静默

```bash
# 创建静默（2 小时）
curl -X POST http://alertmanager:9093/api/v2/silences \
  -H "Content-Type: application/json" \
  -d '{
    "matchers": [
      {
        "name": "alertname",
        "value": "HighMemoryUsage",
        "isRegex": false,
        "isEqual": true
      },
      {
        "name": "namespace",
        "value": "production",
        "isRegex": false,
        "isEqual": true
      }
    ],
    "startsAt": "2026-07-02T10:00:00Z",
    "endsAt": "2026-07-02T12:00:00Z",
    "createdBy": "sre-team",
    "comment": "Scheduled maintenance window",
    "status": {
      "state": "active"
    }
  }'
```

### 4.2 定期维护静默

```yaml
# 定期维护窗口（每周三 22:00-23:00）
apiVersion: monitoring.coreos.com/v1alpha1
kind: AlertmanagerConfig
metadata:
  name: maintenance-silence
  namespace: monitoring
spec:
  matchers:
  - name: namespace
    value: production
  inhibitRules:
  - sourceMatch:
    - name: alertname
      value: MaintenanceWindow
    targetMatch:
    - name: severity
      value: warning
```

## 5. 分组策略

### 5.1 最佳分组配置

```yaml
route:
  # 基础分组
  group_by: ['alertname', 'namespace']

  routes:
  # 按 Pod 精细分组（适用于密集告警）
  - match:
      app: api-gateway
    group_by: ['alertname', 'namespace', 'pod']

  # 按集群分组（适用于多集群告警）
  - match:
      severity: critical
    group_by: ['alertname', 'cluster']

  # 不分组（适用于低频重要告警）
  - match:
      alertname: Watchdog
    group_by: [...]
```

### 5.2 时间参数优化

```yaml
route:
  # group_wait: 收到第一个告警后等待多久再发送（合并同组告警）
  group_wait: 30s

  # group_interval: 同组告警发送间隔
  group_interval: 5m

  # repeat_interval: 未解决告警的重复通知间隔
  repeat_interval: 4h

  routes:
  # 紧急告警：缩短所有时间
  - match:
      severity: critical
    group_wait: 10s
    group_interval: 1m
    repeat_interval: 15m

  # 低优先级告警：延长时间
  - match:
      severity: info
    group_wait: 5m
    group_interval: 30m
    repeat_interval: 24h
```

## 6. Receiver 集成

### 6.1 Slack 集成

```yaml
receivers:
- name: slack-production
  slack_configs:
  - api_url: 'https://hooks.slack.com/services/T00/B00/xxx'
    channel: '#alerts-production'
    send_resolved: true
    title: |
      [{{ .Status | toUpper }}{{ if eq .Status "firing" }}:{{ .Alerts.Firing | len }}{{ end }}]
      {{ .CommonLabels.alertname }}
    text: |
      {{ range .Alerts }}
      *Alert:* {{ .Annotations.summary }} - `{{ .Labels.severity }}`
      *Description:* {{ .Annotations.description }}
      *Namespace:* {{ .Labels.namespace }}
      *Pod:* {{ .Labels.pod }}
      *Started:* {{ .StartsAt }}
      {{ if .EndsAt }}*Resolved:* {{ .EndsAt }}{{ end }}
      {{ end }}
    actions:
    - type: button
      text: 'View in Grafana :grafana:'
      url: '{{ (index .Alerts 0).GeneratorURL }}'
    - type: button
      text: 'Runbook :book:'
      url: '{{ (index .Alerts 0).Annotations.runbook_url }}'
```

### 6.2 PagerDuty 集成

```yaml
receivers:
- name: pagerduty-critical
  pagerduty_configs:
  - routing_key: '<pagerduty-integration-key>'
    severity: '{{ .GroupLabels.severity }}'
    description: |
      [{{ .Status | toUpper }}] {{ .CommonLabels.alertname }}
    details:
      firing: '{{ .Alerts.Firing | len }}'
      resolved: '{{ .Alerts.Resolved | len }}'
      namespace: '{{ .GroupLabels.namespace }}'
      cluster: '{{ .GroupLabels.cluster }}'
    source: '{{ .CommonLabels.source }}'
    component: '{{ .CommonLabels.component }}'
    group: '{{ .GroupLabels.namespace }}'
    class: '{{ .GroupLabels.alertname }}'
```

### 6.3 Webhook 集成

```yaml
receivers:
- name: webhook-integration
  webhook_configs:
  - url: 'http://webhook-handler.monitoring.svc:8080/alerts'
    send_resolved: true
    http_config:
      basic_auth:
        username: 'alertmanager'
        password_file: '/etc/alertmanager/webhook-password'
      tls_config:
        ca_file: '/etc/alertmanager/ca.crt'
```

### 6.4 Email 集成

```yaml
global:
  smtp_smarthost: 'smtp.example.com:587'
  smtp_from: 'alertmanager@example.com'
  smtp_auth_username: 'alertmanager@example.com'
  smtp_auth_password: '<password>'
  smtp_require_tls: true

receivers:
- name: email-team
  email_configs:
  - to: 'sre-team@example.com'
    send_resolved: true
    headers:
      subject: '[{{ .Status | toUpper }}] {{ .CommonLabels.alertname }}'
    html: |
      <h2>{{ .CommonLabels.alertname }}</h2>
      <p>Status: {{ .Status }}</p>
      {{ range .Alerts }}
      <hr>
      <p><b>Alert:</b> {{ .Annotations.summary }}</p>
      <p><b>Description:</b> {{ .Annotations.description }}</p>
      <p><b>Namespace:</b> {{ .Labels.namespace }}</p>
      <p><b>Started:</b> {{ .StartsAt }}</p>
      {{ end }}
```

## 7. 最佳实践

```
Alertmanager 配置检查清单：

□ 部署 3 副本高可用集群
□ 设计清晰的路由树（按 severity → namespace → app）
□ 配置合理的分组策略（避免告警风暴）
□ 配置抑制规则（级联抑制）
□ 使用静默规则处理维护窗口
□ 集成多种 Receiver（Slack/PagerDuty/Email）
□ 配置告警升级策略
□ 定期审查路由规则
□ 测试告警通知链路
□ 监控 Alertmanager 自身健康
```

## Related

- [[可观测性/告警/02-pagerduty-opsgenie-integration|告警平台集成]]
- [[可观测性/告警/03-alert-fatigue-reduction-strategies|告警疲劳治理]]

## See Also

- [Alertmanager 文档](https://prometheus.io/docs/alerting/latest/alertmanager/)
- [Alertmanager 配置参考](https://prometheus.io/docs/alerting/latest/configuration/)


<!-- risk-assessed -->
