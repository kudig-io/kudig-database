---
title: "MTTR 优化框架"
description: "MTTR 全链路优化：MTTD/MTTI/MTTR 分解、自动化诊断、Runbook 自动化、事后分析驱动改进与度量体系"
summary: "系统化的 MTTR 优化方法论，覆盖故障生命周期分解（MTTD/MTTI/MTTR/MTTV）、自动化诊断工具链建设、Runbook 即代码的自动化实践、Blameless 事后分析驱动持续改进以及 MTTR 度量体系设计与团队对标"
category: 可靠性
tags:
- mttr
- mttd
- mtti
- automation
- runbook
- postmortem
- incident-management
- observability
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- "如何系统性地降低 MTTR"
- "Runbook 自动化如何实施"
- "事后分析如何驱动 MTTR 持续改进"
trigger_keywords:
- MTTR
- MTTD
- MTTI
- 故障恢复
- runbook
- 事后分析
- 诊断自动化
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

# MTTR 优化框架

## 概述

MTTR（Mean Time To Recovery）是衡量组织故障恢复能力的核心指标，但它只是一个结果指标——真正驱动改进的是对故障生命周期的精细分解。一次完整的故障经历包含：发现（Detection）→ 响应（Engagement）→ 诊断（Diagnosis）→ 修复（Remediation）→ 验证（Verification）五个阶段，每个阶段都有独立的优化空间。

本文提供系统化的 MTTR 优化框架：从度量体系设计（让问题可见）到自动化诊断（缩短诊断时间）到 Runbook 自动化（缩短修复时间）到事后分析（驱动持续改进），形成完整的优化闭环。与 [[12-可靠性/06-SRE实践/03-incident-command-system.md|事件指挥系统]] 侧重组织流程不同，本文聚焦于技术工具和自动化能力建设。

## 核心概念

### 故障生命周期分解

```
┌─────────────────────────────────────────────────────────────────┐
│                  故障生命周期与 MTTR 分解                          │
│                                                                   │
│  故障发生    告警触发    On-call响应   根因定位    修复执行    验证恢复│
│     │          │          │          │          │          │     │
│     ▼          ▼          ▼          ▼          ▼          ▼     │
│  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐   │
│  │      │  │ MTTD │  │ MTTI │  │ MTTL │  │ MTTR │  │ MTTV │   │
│  │      │  │      │  │      │  │      │  │      │  │      │   │
│  │      │  │ 发现  │  │ 响应  │  │ 定位  │  │ 修复  │  │ 验证  │   │
│  │      │  │ 时间  │  │ 时间  │  │ 时间  │  │ 时间  │  │ 时间  │   │
│  └──────┘  └──────┘  └──────┘  └──────┘  └──────┘  └──────┘   │
│                                                                   │
│  ◀─────────────────── Total MTTR ──────────────────────────▶     │
│                                                                   │
│  各阶段典型耗时（优化前 → 优化后）:                                │
│  • MTTD: 15-30min → 1-5min（智能告警 + 合成监控）                 │
│  • MTTI: 10-30min → 1-5min（自动 Page + 一键接入）                │
│  • MTTL: 30-120min → 5-30min（自动化诊断 + 拓扑关联）             │
│  • MTTR: 15-60min → 2-10min（Runbook 自动化 + 一键回滚）          │
│  • MTTV: 10-30min → 2-5min（自动化验证 + 金丝雀指标）             │
└─────────────────────────────────────────────────────────────────┘
```

### MTTR 各阶段优化策略

| 阶段 | 瓶颈原因 | 优化策略 | 工具/技术 | 预期提升 |
|------|---------|---------|----------|---------|
| MTTD（发现） | 告警缺失/阈值不当/无合成监控 | 多层监控 + 异常检测 + 用户侧探测 | Prometheus + AIOps + Blackbox Exporter | 80% |
| MTTI（响应） | 通知延迟/找不到人/信息不足 | 自动 Page + 事件上下文富化 | PagerDuty + Slack Bot + 自动 War Room | 70% |
| MTTL（定位） | 手动排查/信息分散/缺乏关联 | 自动化诊断 + 拓扑关联 + AI 根因分析 | 诊断 Runbook + Service Graph + RCA Engine | 60% |
| MTTR（修复） | 手动操作/审批流程/回滚复杂 | Runbook 自动化 + 一键回滚 + 自愈 | Argo Rollouts + Feature Flag + 自愈控制器 | 70% |
| MTTV（验证） | 手动验证/缺乏自动化测试 | 合成事务验证 + 金丝雀指标 + 自动确认 | Smoke Test + SLO Dashboard + 自动关闭 | 60% |

### MTTR 度量体系

| 指标 | 定义 | 目标值 | 数据来源 |
|------|------|--------|---------|
| MTTR (P50) | 50% 故障在此时间内恢复 | < 30min | 事件管理系统 |
| MTTR (P95) | 95% 故障在此时间内恢复 | < 2h | 事件管理系统 |
| MTTD | 从故障发生到被发现的时间 | < 5min | 监控系统 |
| MTTI | 从告警到 On-call 开始处理的时间 | < 5min | PagerDuty |
| MTTL | 从开始处理到定位根因的时间 | < 30min | 事件时间线 |
| 首次修复成功率 | 第一次修复尝试即成功的比例 | > 80% | 事件回顾 |
| 重复故障率 | 同一根因 30 天内再次发生的比例 | < 5% | 事后分析跟踪 |

## 生产部署/实现

### 自动化诊断工具链

构建一键式诊断能力，将常见故障的排查步骤自动化：

```yaml
# 🟢 低风险：只读诊断工具部署
apiVersion: apps/v1
kind: Deployment
metadata:
  name: incident-diagnostic-bot
  namespace: sre-tools
spec:
  replicas: 2
  selector:
    matchLabels:
      app: incident-diagnostic-bot
  template:
    metadata:
      labels:
        app: incident-diagnostic-bot
    spec:
      serviceAccountName: diagnostic-bot
      containers:
      - name: bot
        image: registry.internal/sre-tools/diagnostic-bot:v2.0.0
        ports:
        - containerPort: 8080
          name: http
        env:
        - name: PROMETHEUS_URL
          value: "http://prometheus-server.monitoring.svc:9090"
        - name: LOKI_URL
          value: "http://loki.observability.svc:3100"
        - name: TEMPO_URL
          value: "http://tempo-query.observability.svc:16686"
        - name: ALERTMANAGER_URL
          value: "http://alertmanager.monitoring.svc:9093"
        - name: SLACK_WEBHOOK_URL
          valueFrom:
            secretKeyRef:
              name: diagnostic-bot-secrets
              key: slack-webhook
        - name: DIAGNOSTIC_TIMEOUT
          value: "120s"
        - name: MAX_CONCURRENT_DIAGNOSTICS
          value: "10"
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: "2"
            memory: 2Gi
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
        readinessProbe:
          httpGet:
            path: /readyz
            port: 8080
---
apiVersion: v1
kind: ServiceAccount
metadata:
  name: diagnostic-bot
  namespace: sre-tools
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: diagnostic-bot-reader
rules:
- apiGroups: [""]
  resources: ["pods", "services", "endpoints", "events", "nodes", "namespaces", "configmaps"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments", "replicasets", "statefulsets", "daemonsets"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["networking.k8s.io"]
  resources: ["ingresses", "networkpolicies"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["autoscaling"]
  resources: ["horizontalpodautoscalers"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["argoproj.io"]
  resources: ["rollouts", "analysisruns"]
  verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: diagnostic-bot-reader
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: diagnostic-bot-reader
subjects:
- kind: ServiceAccount
  name: diagnostic-bot
  namespace: sre-tools
```

### Runbook 即代码（Runbook-as-Code）

将运维操作手册转化为可执行的自动化脚本：

```yaml
# 🟡 中风险：Runbook 自动化执行会修改集群状态
apiVersion: v1
kind: ConfigMap
metadata:
  name: runbook-pod-crashloop
  namespace: sre-tools
data:
  runbook.yaml: |
    name: pod-crashloop-diagnosis-and-fix
    description: "Pod CrashLoopBackOff 自动诊断与修复"
    trigger:
      alert: KubePodCrashLooping
      severity: critical
    parameters:
      namespace: "{{ alert.labels.namespace }}"
      pod: "{{ alert.labels.pod }}"
      container: "{{ alert.labels.container }}"
    steps:
    - name: collect-info
      type: diagnostic
      risk: low
      actions:
      - cmd: "kubectl get pod {{ pod }} -n {{ namespace }} -o yaml"
        save_as: pod_spec
      - cmd: "kubectl logs {{ pod }} -n {{ namespace }} -c {{ container }} --previous --tail=100"
        save_as: previous_logs
      - cmd: "kubectl describe pod {{ pod }} -n {{ namespace }}"
        save_as: pod_events
      - cmd: "kubectl get events -n {{ namespace }} --field-selector involvedObject.name={{ pod }} --sort-by='.lastTimestamp'"
        save_as: events

    - name: diagnose
      type: analysis
      risk: low
      rules:
      - condition: "previous_logs contains 'OOMKilled'"
        conclusion: "OOM - 内存不足"
        fix_step: increase-memory
      - condition: "previous_logs contains 'Connection refused'"
        conclusion: "依赖服务不可用"
        fix_step: check-dependencies
      - condition: "previous_logs contains 'permission denied'"
        conclusion: "权限问题"
        fix_step: check-rbac
      - condition: "pod_events contains 'FailedScheduling'"
        conclusion: "调度失败 - 资源不足"
        fix_step: check-resources
      - condition: "previous_logs contains 'ImagePullBackOff'"
        conclusion: "镜像拉取失败"
        fix_step: check-image

    - name: increase-memory
      type: remediation
      risk: medium
      requires_approval: true
      actions:
      - cmd: "kubectl patch deployment {{ deployment }} -n {{ namespace }} --type='json' -p='[{\"op\":\"replace\",\"path\":\"/spec/template/spec/containers/0/resources/limits/memory\",\"value\":\"{{ new_memory_limit }}\"}]'"
      - cmd: "kubectl rollout status deployment/{{ deployment }} -n {{ namespace }} --timeout=300s"

    - name: verify
      type: verification
      risk: low
      actions:
      - cmd: "kubectl get pod -n {{ namespace }} -l app={{ app_label }} --field-selector=status.phase=Running"
      - cmd: "sleep 60 && kubectl get pod -n {{ namespace }} -l app={{ app_label }} | grep -c Running"
      success_condition: "running_count >= expected_replicas"

    - name: notify
      type: notification
      actions:
      - channel: "#incidents"
        message: |
          ✅ 自动修复完成: {{ pod }} ({{ namespace }})
          根因: {{ conclusion }}
          修复操作: {{ fix_action }}
          验证结果: {{ verification_result }}
---
# OOM 自动修复 Runbook
apiVersion: v1
kind: ConfigMap
metadata:
  name: runbook-oom-auto-fix
  namespace: sre-tools
data:
  runbook.yaml: |
    name: oom-auto-remediation
    description: "OOM Kill 自动扩容修复"
    trigger:
      alert: KubePodOOMKilled
      severity: warning
    parameters:
      namespace: "{{ alert.labels.namespace }}"
      pod: "{{ alert.labels.pod }}"
    steps:
    - name: analyze-oom
      type: diagnostic
      actions:
      - cmd: "kubectl top pod {{ pod }} -n {{ namespace }} --containers"
      - promql: "container_memory_working_set_bytes{pod='{{ pod }}', namespace='{{ namespace }}'} / container_spec_memory_limit_bytes{pod='{{ pod }}', namespace='{{ namespace }}'}"
        save_as: memory_utilization

    - name: auto-scale-memory
      type: remediation
      risk: medium
      condition: "memory_utilization > 0.9 AND restart_count < 5"
      actions:
      # 将内存限制增加 50%（上限为当前值的 2 倍）
      - cmd: |
          CURRENT_LIMIT=$(kubectl get pod {{ pod }} -n {{ namespace }} -o jsonpath='{.spec.containers[0].resources.limits.memory}')
          NEW_LIMIT=$(echo "$CURRENT_LIMIT" | python3 -c "
          import sys
          val = sys.stdin.read().strip()
          if val.endswith('Gi'):
              print(f'{float(val[:-2]) * 1.5:.1f}Gi')
          elif val.endswith('Mi'):
              print(f'{int(int(val[:-2]) * 1.5)}Mi')
          ")
          kubectl patch deployment {{ deployment }} -n {{ namespace }} \
            --type='json' \
            -p="[{\"op\":\"replace\",\"path\":\"/spec/template/spec/containers/0/resources/limits/memory\",\"value\":\"$NEW_LIMIT\"}]"
      requires_approval: false  # OOM 自动修复无需审批（有上限保护）
      max_auto_scale_factor: 2.0
```

### 事后分析自动化跟踪

```yaml
# 🟢 低风险：事后分析跟踪系统配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: postmortem-tracking-config
  namespace: sre-tools
data:
  config.yaml: |
    postmortem:
      # 事后分析触发条件
      triggers:
      - severity: SEV1
        required: true
        deadline_hours: 48
      - severity: SEV2
        required: true
        deadline_hours: 72
      - severity: SEV3
        required: false
        mttr_exceeded: true
        deadline_hours: 168

      # 事后分析模板
      template:
        sections:
        - title: "事件摘要"
          fields: [incident_id, severity, duration, affected_services, user_impact]
        - title: "时间线"
          fields: [detection_time, engagement_time, diagnosis_time, fix_time, verification_time]
        - title: "根因分析"
          fields: [root_cause, contributing_factors, trigger]
        - title: "影响评估"
          fields: [affected_users, revenue_impact, slo_impact, error_budget_consumed]
        - title: "改进行动项"
          fields: [action_items, owners, deadlines, priority]
        - title: "经验教训"
          fields: [what_went_well, what_went_poorly, where_got_lucky]

      # 行动项跟踪
      action_item_tracking:
        reminder_days: [3, 7, 14, 30]
        escalation_after_days: 14
        auto_create_jira: true
        jira_project: SRE
        jira_labels: [postmortem, action-item]

      # 度量报告
      reporting:
        weekly_summary:
          channel: "#sre-metrics"
          metrics:
          - mttr_p50_trend
          - mttr_p95_trend
          - incident_count_by_severity
          - repeat_incident_rate
          - action_item_completion_rate
        monthly_review:
          audience: [engineering-leads]
          include:
          - top_3_mttr_drivers
          - automation_opportunities
          - training_needs
```

### 自愈控制器

```yaml
# 🟡 中风险：自愈控制器会自动修改集群状态
apiVersion: apps/v1
kind: Deployment
metadata:
  name: self-healing-controller
  namespace: sre-tools
spec:
  replicas: 2
  selector:
    matchLabels:
      app: self-healing-controller
  template:
    metadata:
      labels:
        app: self-healing-controller
    spec:
      serviceAccountName: self-healing-controller
      containers:
      - name: controller
        image: registry.internal/sre-tools/self-healing-controller:v1.5.0
        env:
        - name: PROMETHEUS_URL
          value: "http://prometheus-server.monitoring.svc:9090"
        - name: CHECK_INTERVAL
          value: "30s"
        - name: MAX_AUTO_REMEDIATIONS_PER_HOUR
          value: "5"
        - name: COOLDOWN_PERIOD
          value: "30m"
        - name: DRY_RUN_MODE
          value: "false"
        - name: NOTIFICATION_CHANNEL
          value: "#auto-remediation"
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 512Mi
        volumeMounts:
        - name: runbooks
          mountPath: /runbooks
      volumes:
      - name: runbooks
        configMap:
          name: self-healing-runbooks
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: self-healing-runbooks
  namespace: sre-tools
data:
  healing-rules.yaml: |
    rules:
    # 规则 1：Pod 持续 CrashLoop 且原因是 OOM → 自动扩容内存
    - name: oom-auto-scale
      trigger:
        alert: KubePodOOMKilled
        condition: "restart_count >= 3 AND restart_count < 10"
      action:
        type: scale_memory
        factor: 1.5
        max_factor: 2.0
        cooldown: 1h
      notification:
        message: "Auto-scaled memory for {{ pod }} in {{ namespace }}: {{ old_limit }} → {{ new_limit }}"

    # 规则 2：Deployment 副本数不足 → 自动恢复期望副本数
    - name: replica-recovery
      trigger:
        condition: "available_replicas < desired_replicas * 0.5 for 5m"
      action:
        type: restart_deployment
        cooldown: 30m
      notification:
        message: "Restarted deployment {{ deployment }} in {{ namespace }} due to low availability"

    # 规则 3：磁盘使用率过高 → 自动清理日志
    - name: disk-cleanup
      trigger:
        condition: "node_filesystem_avail_bytes / node_filesystem_size_bytes < 0.1"
      action:
        type: cleanup_logs
        target: "/var/log/pods"
        retention: "24h"
      notification:
        message: "Cleaned old logs on node {{ node }}, freed {{ freed_space }}"

    # 规则 4：证书即将过期 → 自动续期
    - name: cert-renewal
      trigger:
        condition: "cert_manager_certificate_expiration_timestamp_seconds - time() < 86400 * 7"
      action:
        type: renew_certificate
        issuer: letsencrypt-prod
      notification:
        message: "Auto-renewed certificate {{ cert_name }} in {{ namespace }}"
```

## 运维操作

### MTTR 度量查询

```bash
# 🟢 低风险：只读度量查询
# 查询过去 30 天的事件统计
curl -s -H "Authorization: Bearer $INCIDENT_API_TOKEN" \
  "https://incidents.internal/api/v1/incidents?since=2026-06-19&status=resolved" | \
  jq '{
    total: length,
    by_severity: group_by(.severity) | map({severity: .[0].severity, count: length}),
    mttr_p50: (map(.resolution_time_minutes) | sort | .[length/2 | floor]),
    mttr_p95: (map(.resolution_time_minutes) | sort | .[length * 0.95 | floor])
  }'

# 按服务统计 MTTR
curl -s -H "Authorization: Bearer $INCIDENT_API_TOKEN" \
  "https://incidents.internal/api/v1/incidents?since=2026-06-19" | \
  jq 'group_by(.service) | map({
    service: .[0].service,
    incident_count: length,
    avg_mttr: (map(.resolution_time_minutes) | add / length | round)
  }) | sort_by(.avg_mttr) | reverse'

# 查看事后分析行动项完成率
curl -s -H "Authorization: Bearer $INCIDENT_API_TOKEN" \
  "https://incidents.internal/api/v1/action-items?status=all" | \
  jq '{
    total: length,
    completed: map(select(.status == "done")) | length,
    overdue: map(select(.status != "done" and .deadline < "2026-07-19")) | length,
    completion_rate: (map(select(.status == "done")) | length) * 100.0 / length
  }'
```

### 诊断 Bot 使用

```bash
# 🟢 低风险：触发自动诊断
# 通过 Slack 命令触发诊断
# /diagnose pod payment-service-xxx -n production

# 通过 API 触发诊断
curl -X POST http://incident-diagnostic-bot.sre-tools.svc:8080/api/v1/diagnose \
  -H "Content-Type: application/json" \
  -d '{
    "target": {
      "kind": "pod",
      "name": "payment-service-7d4f8b6c5-x2k9p",
      "namespace": "production"
    },
    "context": {
      "alert": "KubePodCrashLooping",
      "severity": "critical"
    }
  }'

# 查看诊断结果
curl -s http://incident-diagnostic-bot.sre-tools.svc:8080/api/v1/diagnose/{diagnosis-id} | \
  jq '{conclusion, confidence, evidence, recommended_actions}'
```

### Runbook 执行与审计

```bash
# 🟡 中风险：执行 Runbook 会修改集群状态
# 手动触发 Runbook 执行
curl -X POST http://incident-diagnostic-bot.sre-tools.svc:8080/api/v1/runbooks/execute \
  -H "Content-Type: application/json" \
  -d '{
    "runbook": "pod-crashloop-diagnosis-and-fix",
    "parameters": {
      "namespace": "production",
      "pod": "payment-service-7d4f8b6c5-x2k9p",
      "container": "payment-service"
    },
    "approved_by": "oncall-sre",
    "dry_run": false
  }'

# 查看 Runbook 执行历史
curl -s http://incident-diagnostic-bot.sre-tools.svc:8080/api/v1/runbooks/history?limit=20 | \
  jq '.[] | {runbook, timestamp, status, duration, actions_taken}'

# 审计自愈操作
kubectl get events -n sre-tools --field-selector reason=SelfHealing --sort-by='.lastTimestamp'
```

## 故障排查

### 诊断 Bot 不可用

```bash
# 🟢 低风险：只读诊断
# 检查诊断 Bot 状态
kubectl get pods -n sre-tools -l app=incident-diagnostic-bot
kubectl logs -n sre-tools deployment/incident-diagnostic-bot --tail=50

# 检查 RBAC 权限是否足够
kubectl auth can-i list pods --as=system:serviceaccount:sre-tools:diagnostic-bot --all-namespaces
kubectl auth can-i get events --as=system:serviceaccount:sre-tools:diagnostic-bot --all-namespaces

# 检查 Prometheus 连接
kubectl exec -n sre-tools deployment/incident-diagnostic-bot -- \
  wget -qO- http://prometheus-server.monitoring.svc:9090/-/healthy
```

### 自愈控制器误操作

```bash
# 🔴 高风险：紧急停止自愈控制器
# 立即停止所有自愈操作
kubectl scale deployment self-healing-controller -n sre-tools --replicas=0

# 查看最近的自愈操作
kubectl logs -n sre-tools deployment/self-healing-controller --tail=100 | grep "REMEDIATION"

# 回滚自愈操作（如果是错误的扩容）
kubectl rollout undo deployment/affected-service -n production

# 恢复自愈控制器（修复规则后）
kubectl scale deployment self-healing-controller -n sre-tools --replicas=2
```

### 度量数据不准确

```bash
# 🟢 低风险：只读诊断
# 验证事件时间线数据完整性
curl -s -H "Authorization: Bearer $INCIDENT_API_TOKEN" \
  "https://incidents.internal/api/v1/incidents/INC-2026-0719" | \
  jq '{
    detection_time,
    engagement_time,
    diagnosis_time,
    fix_time,
    verification_time,
    gaps: (
      if .engagement_time == null then ["missing_engagement"]
      elif .diagnosis_time == null then ["missing_diagnosis"]
      else []
      end
    )
  }'
```

## 最佳实践

### MTTR 优化实施路径

**第一阶段（1-2 月）：度量可见**
- 建立 MTTR 分解度量体系（MTTD/MTTI/MTTL/MTTR/MTTV）
- 每次事件记录完整时间线
- 建立周/月 MTTR 报告

**第二阶段（2-4 月）：诊断加速**
- 部署自动化诊断 Bot
- 编写 Top 10 故障场景的诊断 Runbook
- 集成可观测性三支柱（Metrics + Logs + Traces）关联查询

**第三阶段（4-6 月）：修复自动化**
- 实现 Top 5 故障场景的自动修复
- 部署自愈控制器（从低风险操作开始）
- 与 [[11-发布变更/04-变更管理/07-rollback-automation-patterns.md|回滚自动化]] 集成

**第四阶段（持续）：文化驱动**
- Blameless 事后分析制度化
- 行动项跟踪闭环（14 天未完成自动升级）
- 季度 MTTR 回顾与目标调整

### 事后分析最佳实践

1. **Blameless 原则**：聚焦系统和流程改进，不追究个人责任。

2. **5 Whys 根因分析**：不停留在表面原因，追问到系统性根因。

3. **行动项 SMART 原则**：每个行动项必须有明确的 Owner、Deadline 和验收标准。

4. **经验沉淀**：将事后分析中的诊断路径转化为自动化 Runbook。

5. **定期回顾**：每月回顾行动项完成情况，每季度回顾 MTTR 趋势。

### 与现有体系集成

- [[09-可观测性/05-告警/06-aiops-intelligent-alerting.md|AIOps 智能告警]]：缩短 MTTD
- [[12-可靠性/06-SRE实践/03-incident-command-system.md|事件指挥系统]]：规范 MTTI 流程
- [[09-可观测性/01-总览/01-observability-architecture-overview.md|可观测性架构]]：支撑 MTTL 诊断
- [[11-发布变更/04-变更管理/07-rollback-automation-patterns.md|回滚自动化]]：缩短 MTTR
- [[12-可靠性/06-SRE实践/07-error-budget-automation.md|错误预算自动化]]：MTTR 与 SLO 联动

## Related

- [[12-可靠性/06-SRE实践/03-incident-command-system.md|事件指挥系统]]
- [[12-可靠性/06-SRE实践/07-error-budget-automation.md|错误预算自动化]]
- [[09-可观测性/05-告警/06-aiops-intelligent-alerting.md|AIOps 智能告警]]
- [[11-发布变更/04-变更管理/07-rollback-automation-patterns.md|回滚自动化模式]]
- [[09-可观测性/01-总览/01-observability-architecture-overview.md|可观测性架构总览]]
- [[12-可靠性/06-SRE实践/06-toil-reduction-automation.md|Toil 消减自动化]]
- [[12-可靠性/05-事后复盘/index|05-事后复盘]]
