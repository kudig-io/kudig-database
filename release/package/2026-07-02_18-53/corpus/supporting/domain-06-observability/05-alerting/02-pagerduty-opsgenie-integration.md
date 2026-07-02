---
title: PagerDuty 与 Opsgenie 告警集成
description: '告警平台集成：PagerDuty Service/Integration Key 配置、Opsgenie Team/Responder 配置、值班排班、升级策略、告警去重'
summary: 'PagerDuty/Opsgenie 集成、值班排班与升级策略配置'
category: observability
tags:
- pagerduty
- opsgenie
- oncall
- escalation
- alert-integration
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
- PagerDuty 与 Opsgenie 告警集成是什么
- 如何配置 PagerDuty Service
trigger_keywords:
- PagerDuty
- Opsgenie
- 值班排班
- 升级策略
- 告警去重
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


# PagerDuty 与 Opsgenie 告警集成

## 概述

PagerDuty 和 Opsgenie 是主流的事件管理平台，提供告警接收、值班排班、升级策略和协同响应能力。本文档涵盖两个平台与 Kubernetes 告警系统的完整集成方案。

## 1. PagerDuty 集成

### 1.1 Service 配置

```bash
# 创建 PagerDuty Service
# 1. 登录 PagerDuty → Services → New Service
# 2. 选择 "Use our API directly" 作为 Integration Type
# 3. 记录 Integration Key

# Integration Key 示例：a1b2c3d4e5f6g7h8i9j0k1l2m3n4o5p6
```

### 1.2 Alertmanager 配置

```yaml
# alertmanager.yaml
global:
  pagerduty_url: 'https://events.pagerduty.com/v2/enqueue'

receivers:
# P0 紧急告警
- name: pagerduty-p0
  pagerduty_configs:
  - routing_key: '<p0-integration-key>'
    severity: critical
    description: '[P0] {{ .CommonLabels.alertname }} - {{ .CommonLabels.namespace }}'
    details:
      firing_count: '{{ .Alerts.Firing | len }}'
      resolved_count: '{{ .Alerts.Resolved | len }}'
      cluster: '{{ .CommonLabels.cluster }}'
      namespace: '{{ .CommonLabels.namespace }}'
      runbook_url: '{{ (index .Alerts 0).Annotations.runbook_url }}'
      grafana_url: '{{ (index .Alerts 0).GeneratorURL }}'
    source: '{{ .CommonLabels.source }}'
    component: '{{ .CommonLabels.component }}'
    group: '{{ .GroupLabels.namespace }}'
    class: '{{ .GroupLabels.alertname }}'
    links:
    - href: '{{ (index .Alerts 0).GeneratorURL }}'
      text: 'View in Grafana'
    - href: '{{ (index .Alerts 0).Annotations.runbook_url }}'
      text: 'Runbook'

# P1 高优先级告警
- name: pagerduty-p1
  pagerduty_configs:
  - routing_key: '<p1-integration-key>'
    severity: error
    description: '[P1] {{ .CommonLabels.alertname }} - {{ .CommonLabels.namespace }}'
    details:
      cluster: '{{ .CommonLabels.cluster }}'
      namespace: '{{ .CommonLabels.namespace }}'

# 通用告警
- name: pagerduty-general
  pagerduty_configs:
  - routing_key: '<general-integration-key>'
    severity: '{{ .CommonLabels.severity }}'
    description: '{{ .CommonLabels.alertname }}'
```

### 1.3 事件去重

```yaml
# PagerDuty 自动去重（基于 routing_key + source + severity）
# Alertmanager 已按 group_by 去重，但仍可能有重复
# 使用 custom_details.dedup_key 控制去重

receivers:
- name: pagerduty-dedup
  pagerduty_configs:
  - routing_key: '<integration-key>'
    description: '{{ .CommonLabels.alertname }}'
    details:
      dedup_key: '{{ .CommonLabels.alertname }}-{{ .CommonLabels.namespace }}-{{ .CommonLabels.pod }}'
```

### 1.4 升级策略配置

```yaml
# PagerDuty Escalation Policy（通过 API 配置）
# Level 1: On-call 工程师（立即通知）
# Level 2: 团队 Lead（5 分钟后）
# Level 3: 工程经理（15 分钟后）
# Level 4: VP Engineering（30 分钟后）

# API 配置示例
curl -X POST https://api.pagerduty.com/escalation_policies \
  -H "Authorization: Token token=<api-token>" \
  -H "Content-Type: application/json" \
  -d '{
    "escalation_policy": {
      "name": "K8s Production Escalation",
      "description": "Escalation policy for K8s production alerts",
      "num_loops": 3,
      "escalation_rules": [
        {
          "escalation_delay_in_minutes": 5,
          "targets": [
            { "id": "<sre-team-id>", "type": "schedule_reference" }
          ]
        },
        {
          "escalation_delay_in_minutes": 10,
          "targets": [
            { "id": "<team-lead-id>", "type": "user_reference" }
          ]
        },
        {
          "escalation_delay_in_minutes": 15,
          "targets": [
            { "id": "<engineering-manager-id>", "type": "user_reference" }
          ]
        }
      ]
    }
  }'
```

## 2. Opsgenie 集成

### 2.1 Team 与 Integration 配置

```bash
# 创建 Opsgenie Integration
# 1. 登录 Opsgenie → Teams → Create Team
# 2. 添加 Integration → Prometheus
# 3. 记录 API Key

# API Key 示例: 12345678-1234-1234-1234-123456789012
```

### 2.2 Alertmanager 配置

```yaml
# alertmanager.yaml
receivers:
- name: opsgenie-production
  opsgenie_configs:
  - api_key: '<opsgenie-api-key>'
    api_url: 'https://api.opsgenie.com'
    message: '[{{ .Status | toUpper }}] {{ .CommonLabels.alertname }}'
    description: |
      {{ .CommonLabels.alertname }}
      Namespace: {{ .CommonLabels.namespace }}
      Severity: {{ .CommonLabels.severity }}
    source: '{{ .CommonLabels.source }}'
    tags:
    - '{{ .CommonLabels.severity }}'
    - '{{ .CommonLabels.namespace }}'
    - '{{ .CommonLabels.alertname }}'
    details:
      firing: '{{ .Alerts.Firing | len }}'
      resolved: '{{ .Alerts.Resolved | len }}'
      cluster: '{{ .CommonLabels.cluster }}'
      runbook_url: '{{ (index .Alerts 0).Annotations.runbook_url }}'
    responders:
    - name: 'SRE Team'
      type: team
    - name: 'On-Call Schedule'
      type: schedule
    priority: '{{ if eq .CommonLabels.severity "critical" }}P1{{ else if eq .CommonLabels.severity "warning" }}P3{{ else }}P5{{ end }}'
    entity: '{{ .CommonLabels.namespace }}'
    note: 'Auto-generated alert from Prometheus'
```

### 2.3 Opsgenie 值班排班

```yaml
# Opsgenie Schedule 配置（通过 Terraform）
resource "opsgenie_schedule" "sre_oncall" {
  name        = "SRE On-Call"
  description = "SRE team on-call rotation"
  timezone    = "Asia/Shanghai"

  rules {
    frequency  = "weekly"
    start_day  = "monday"
    end_day    = "sunday"
    start_hour = 0
    start_min  = 0
    end_hour   = 24
    end_min    = 0
    participants {
      type = "user"
      id   = opsgenie_user.sre1.id
    }
    participants {
      type = "user"
      id   = opsgenie_user.sre2.id
    }
    participants {
      type = "user"
      id   = opsgenie_user.sre3.id
    }
  }

  overrides {
    name        = "Holiday Coverage"
    start_date  = "2026-07-04T00:00:00+08:00"
    end_date    = "2026-07-05T00:00:00+08:00"
    recipient {
      type = "user"
      id   = opsgenie_user.backup_sre.id
    }
  }
}
```

### 2.4 Opsgenie 升级策略

```yaml
# Opsgenie Escalation 配置
resource "opsgenie_escalation" "production" {
  name = "Production Escalation"

  rule {
    delay    = 0
    recipient {
      type = "schedule"
      id   = opsgenie_schedule.sre_oncall.id
    }
    notify_type = "default"
  }

  rule {
    delay    = 5
    recipient {
      type = "team"
      id   = opsgenie_team.platform.id
    }
    notify_type = "default"
  }

  rule {
    delay    = 15
    recipient {
      type = "user"
      id   = opsgenge_user.engineering_manager.id
    }
    notify_type = "all"
  }

  rule {
    delay    = 30
    recipient {
      type = "user"
      id   = opsgenie_user.vp_engineering.id
    }
    notify_type = "all"
  }

  repeat {
    wait_interval = 15
    count         = 3
    reset_recipient_type = "to-previous"
  }
}
```

## 3. 告警去重策略

### 3.1 Alertmanager 去重

```yaml
# 基于 group_by 的去重
route:
  group_by: ['alertname', 'namespace', 'severity']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 4h

  routes:
  # 按 Pod 精细去重
  - match:
      app: api-gateway
    group_by: ['alertname', 'namespace', 'pod']
```

### 3.2 平台级去重

```yaml
# PagerDuty 去重（基于 routing_key + source + severity）
# 自动去重窗口：默认 24 小时

# Opsgenie 去重（基于 alias）
receivers:
- name: opsgenie-dedup
  opsgenie_configs:
  - api_key: '<api-key>'
    alias: '{{ .CommonLabels.alertname }}-{{ .CommonLabels.namespace }}-{{ .CommonLabels.severity }}'
```

## 4. 最佳实践

```
告警平台集成检查清单：

□ 配置 PagerDuty/Opsgenie Service 和 Integration Key
□ 设置分级告警路由（P0/P1/P2/P3）
□ 配置值班排班（Schedule）
□ 设置升级策略（Escalation Policy）
□ 配置告警去重（group_by + alias）
□ 设置通知渠道（Push/SMS/Email/Phone）
□ 配置静默和维护窗口
□ 定期审查告警规则
□ 测试告警升级流程
□ 监控告警响应 SLA
```

## Related

- [[domain-06-observability/05-alerting/01-alertmanager-deep-configuration|Alertmanager 深度配置]]
- [[domain-06-observability/05-alerting/03-alert-fatigue-reduction-strategies|告警疲劳治理]]

## See Also

- [PagerDuty API 文档](https://developer.pagerduty.com/docs/rest-api-v2/)
- [Opsgenie API 文档](https://docs.opsgenie.com/docs/api-overview)


<!-- risk-assessed -->
