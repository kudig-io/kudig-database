---
title: 可观测性运维指南
description: 面向 Kubernetes 生产环境的可观测性运维手册，覆盖 SLO 评审节奏、告警调优、Dashboard as Code、数据保留归档与值班卫生。
summary: 面向 Kubernetes 生产环境的可观测性运维手册，覆盖 SLO 评审节奏、告警调优、Dashboard as Code、数据保留归档与值班卫生。
category: production-operations
tags:
- production
- best-practices
- playbook
- production-operations
- observability
- slo
- alerting
- grafana
- prometheus
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 30min
intent_queries:
- Kubernetes 可观测性运维如何做
- SLO 评审节奏与告警调优
- Dashboard as Code 最佳实践
- 监控数据保留与归档策略
- 值班卫生与告警疲劳治理
trigger_keywords:
- observability operations
- 可观测性运维
- SLO review
- alert tuning
- dashboard as code
- retention archival
- on-call hygiene
prerequisites:
- kubectl-basics
- prometheus-basics
- grafana-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

# 可观测性运维指南

本指南面向 SRE、运维工程师与平台工程师，聚焦 Kubernetes 可观测性体系的日常运维动作：如何让 SLO 评审形成闭环、如何让告警可行动、如何让 Dashboard 与告警规则可版本管理、如何规划数据保留与归档，以及如何保持值班团队对告警的敏感度。可观测性体系本身的稳定性直接决定了生产故障能否被及时发现与定位，因此它必须像业务服务一样被治理。

## 1. 适用场景与范围

本指南适用于以下场景：

- 已部署 Prometheus / Grafana / Alertmanager / Loki / Jaeger 等可观测性组件的 Kubernetes 集群。
- 需要建立或优化 SLO/SLI 评审、告警治理、Dashboard as Code 与数据生命周期管理流程的团队。

本指南不覆盖具体组件的安装配置，重点在运维节奏与治理机制。如需了解组件部署细节，请参考可观测性域的专项指南。

## 2. 前置条件与工具

在开始可观测性运维前，请确认以下前置条件已经满足：

- 可观测性组件已部署并通过 GitOps 管理配置。
- 已定义关键服务的 SLI/SLO，错误预算可在 Grafana 或内部平台查看。
- 告警路由已对接 On-Call 系统（PagerDuty/OpsGenie/内部值班系统）。
- 具备 promtool、amtool、jsonnet/tanka 或 Grafana Terraform Provider 等工具。

## 3. 核心概念

### 3.1 SLO 评审节奏

SLO 不是一次性设定，而应随业务与系统状态持续校准：

- **每日**：关注错误预算消耗是否异常加速。
- **每周**：召开 SLO 评审会，复盘上周 critical 告警与错误预算消耗根因。
- **每月**：评估 SLI 覆盖度，补充遗漏的关键用户旅程。
- **每季度**：调整 SLO 目标，发布新的错误预算政策。

SLO 评审的输出应直接驱动改进项：优化代码、调整阈值、补充监控或修改发布策略。没有改进项闭环的 SLO 评审只是形式化流程。

### 3.2 告警可行动化

每个告警必须包含：

- 现象描述
- 影响面评估（哪些服务/用户受影响）
- 建议排查命令或 Dashboard 链接
- 相关 Runbook 链接
- 升级路径

禁止只有“CPU 高”这类无上下文的告警。可行动化是降低 MTTR 的关键。当告警触发时，值班工程师应能在 1 分钟内理解问题、在 5 分钟内定位方向、在 15 分钟内开始处置。

### 3.3 Dashboard as Code

所有 Dashboard、告警规则、采集配置纳入 Git 管理，禁止在生产环境直接修改。推荐工具：

- Grafana：Jsonnet / Grafonnet / Terraform Provider
- Prometheus：PrometheusRule YAML + promtool check rules
- Alertmanager：ConfigMap / Secret + amtool check-config

Dashboard as Code 的好处包括版本可追溯、变更可审查、环境间可复现、灾难后可快速恢复。当集群重建时，可观测性配置应能随 GitOps 同步自动恢复。

## 4. 标准操作流程

### 4.1 每日 SLO 健康检查

```bash
# 查看当前告警
kubectl exec -n monitoring alertmanager-0 -- \
  amtool --alertmanager.url=http://localhost:9093 alert

# 检查 Prometheus 目标健康
kubectl port-forward svc/prometheus-k8s 9090:9090 -n monitoring &
curl -s 'http://localhost:9090/api/v1/targets' | \
  jq '.data.activeTargets[] | {job, health, lastError}'

# 今日错误预算消耗（示例 SLI）
curl -sG 'http://localhost:9090/api/v1/query' \
  --data-urlencode 'query=sum(increase(http_requests_total{status=~"5.."}[1d])) / sum(increase(http_requests_total[1d]))'
```

### 4.2 告警质量调优

每周召开告警质量会议，目标：

- critical 告警 < 5 条/周
- false positive < 5%
- 连续 3 次误报的告警必须修改阈值或下线

调优方法：

```bash
# 1. 分析告警频率
kubectl exec -n monitoring alertmanager-0 -- \
  amtool --alertmanager.url=http://localhost:9093 alert --silenced=false | \
  awk '{print $2}' | sort | uniq -c | sort -rn | head -20

# 2. 使用 promtool 验证规则
promtool check rules /path/to/prometheus-rules.yaml
promtool test rules /path/to/rule-tests.yaml

# 3. 优化抑制与分组
```

示例 Alertmanager 分组与抑制：

```yaml
route:
  group_by: ['alertname', 'namespace', 'severity']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 12h
  routes:
    - match:
        severity: critical
      receiver: pagerduty-critical
      continue: false

inhibit_rules:
  - source_match:
      severity: critical
    target_match:
      severity: warning
    equal: ['namespace', 'alertname']
```

### 4.3 Dashboard 版本管理

使用 Jsonnet 生成 Dashboard：

```jsonnet
local grafana = import 'grafonnet/grafana.libsonnet';
local dashboard = grafana.dashboard;

dashboard.new('K8s Capacity Overview')
.addPanel(
  grafana.graphPanel.new('CPU Utilization')
  .addTarget(grafana.prometheus.target('avg(rate(container_cpu_usage_seconds_total[5m])) by (namespace)'))
)
```

CI 中执行：

```bash
jsonnet -J vendor dashboard.jsonnet -o dashboard.json
# 通过 Grafana API 上传或作为 ConfigMap 挂载
kubectl create configmap grafana-dashboard-capacity \
  --from-file=dashboard.json \
  -n monitoring --dry-run=client -o yaml | kubectl apply -f -
```

对于使用 Terraform 的团队，建议通过 Grafana Provider 管理 folder、datasource 与 dashboard，确保所有可视化资源与基础设施代码统一管理。

### 4.4 数据保留与归档

| 数据类型 | 热存储 | 温存储 | 冷存储 |
|---|---|---|---|
| Prometheus 指标 | 15–30 天 | Thanos/Cortex/VictoriaMetrics 90 天 | 对象存储 1 年以上 |
| 容器日志 | 7 天 | 30 天 | 180 天以上（合规） |
| 链路追踪 | 7 天 | 30 天 | 按需 |
| 审计日志 | 30 天 | 90 天 | 1–7 年 |

配置 Prometheus retention：

```yaml
spec:
  retention: 30d
  retentionSize: "45GB"
  walCompression: true
```

配置 Loki 保留策略：

```yaml
schema_config:
  configs:
    - from: 2026-07-01
      store: tsdb
      object_store: s3
      schema: v13
      index:
        prefix: loki_index_
        period: 24h

compactor:
  retention_enabled: true
  retention_delete_delay: 2h
```

保留策略应在设计阶段与合规、成本团队达成一致，并定期评估存储成本增长。不要盲目延长保留期，而应根据实际查询需求与合规要求分级存储。

### 4.5 值班卫生

- **交接班**：记录未闭环告警、正在进行的排查、异常趋势。
- **告警响应**：critical 告警 5 分钟内确认，15 分钟内给出初步判断。
- **事后复盘**：所有 P0/P1 告警触发的事故必须有无责复盘，改进项有 Owner 与截止日期。
- **静默审计**：所有手工 silences 必须记录工单号、预期恢复时间和责任人。

值班卫生直接影响团队对告警的敏感度和事故响应效率。告警疲劳是比告警缺失更隐蔽的风险。

## 5. 关键检查点与验证命令

| 检查项 | 命令/配置 |
|---|---|
| Prometheus 目标健康 | `curl http://prometheus:9090/api/v1/targets` |
| Alertmanager 配置 | `amtool check-config /etc/alertmanager/alertmanager.yml` |
| 告警规则语法 | `promtool check rules /path/to/rules.yaml` |
| Dashboard 版本 | `git log --oneline dashboards/` |
| 数据保留配置 | `kubectl get prometheus -n monitoring -o yaml \| grep retention` |
| SLO 错误预算 | Grafana SLO Dashboard / PromQL `sum(increase(errors[window])) / sum(increase(total[window]))` |
| 可观测性组件自身健康 | `kubectl get pods -n monitoring` |
| 告警路由 | `amtool config routes test` |

## 6. 常见故障与 Remediation

| 现象 | 可能根因 | 处置 |
|---|---|---|
| Prometheus OOM | 高基数指标 / 抓取目标过多 / retention 过大 | 限制 cardinality；启用 recording rules；扩容内存或缩短 retention |
| 告警风暴 | 分组或抑制配置不当 / 基础设施级故障扩散 | 优化 Alertmanager `group_by` 与 `inhibit_rules`；临时聚合通知 |
| Dashboard 数据缺失 | 数据源配置错误 / 标签不一致 / 保留期过期 | 检查 Grafana datasource；校验 PromQL；确认 retention |
| 日志采集延迟 | Fluent Bit 资源不足 / 后端写入失败 | 增加 limits；启用本地缓冲；检查后端的 rate limit |
| 错误预算消耗过快 | 发布异常 / 下游依赖故障 / 阈值过严 | 触发发布门控；启动事故响应；重新评估 SLO |
| 可观测性组件自身故障 | 节点漂移 / PVC 满 / 配置漂移 | 配置 PodDisruptionBudget；扩容 PVC；纳入 GitOps |
| 告警疲劳 | 阈值过松 / 无意义告警过多 | 每周告警质量会议；连续误报警告下线或修改 |
| Trace 采样率过高导致存储爆炸 | 采样策略未按错误链路调整 | 启用头部采样与错误采样结合；按服务配置不同采样率 |

## 7. 风险与注意事项

- **可观测性平台不能依赖被观测对象**：必须建立自监控与外部探测（blackbox exporter）。
- **禁止生产环境直接修改 Dashboard**：所有变更通过 GitOps 流转，保留审计轨迹。
- **数据保留需提前规划**：存储成本随指标基数指数增长，保留策略应在设计阶段确定。
- **告警不是越多越好**：过多的 warning 会稀释 critical 告警的响应速度。
- **SLO 目标应切合实际**：目标过高达不到会失去信任，目标过低则无法驱动改进。
- **跨团队对齐**：SLI/SLO 的定义需要业务、开发与 SRE 共同认可，避免监控指标与业务目标脱节。

---

## 8. 可观测性组件自监控

### 自监控架构

```
外部探测 (Blackbox)          内部自监控
┌─────────────────┐    ┌─────────────────────┐
│ Blackbox Exporter │    │ Prometheus /metrics │
│ (外部集群)       │    │ Grafana /metrics    │
│ - HTTP 探测      │    │ Loki /metrics       │
│ - DNS 探测       │    │ Tempo /metrics      │
│ - TCP 探测       │    │ Alertmanager /metrics│
└────────┬────────┘    └──────────┬──────────┘
         │                       │
         └───────────┬───────────┘
                     ▼
         ┌─────────────────┐
         │ 独立 Prometheus  │
         │ (监控集群)       │
         └─────────────────┘
```

### 关键自监控指标

```yaml
# PrometheusRule: 可观测性组件自监控
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: observability-self-monitoring
  namespace: monitoring
spec:
  groups:
    - name: prometheus.rules
      rules:
        - alert: PrometheusDown
          expr: up{job="prometheus-k8s"} == 0
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "Prometheus 不可用"

        - alert: PrometheusHighCardinality
          expr: prometheus_tsdb_head_series > 5000000
          for: 30m
          labels:
            severity: warning
          annotations:
            summary: "Prometheus 时间序列数过高: {{ $value }}"

        - alert: PrometheusRuleEvaluationSlow
          expr: |
            prometheus_rule_evaluation_duration_seconds{quantile="0.99"} > 10
          for: 10m
          labels:
            severity: warning

    - name: alertmanager.rules
      rules:
        - alert: AlertmanagerDown
          expr: up{job="alertmanager"} == 0
          for: 2m
          labels:
            severity: critical

        - alert: AlertmanagerNotificationFailing
          expr: |
            rate(alertmanager_notifications_failed_total[5m]) > 0
          for: 5m
          labels:
            severity: warning

    - name: grafana.rules
      rules:
        - alert: GrafanaDown
          expr: up{job="grafana"} == 0
          for: 5m
          labels:
            severity: warning
```

### Blackbox 外部探测

```yaml
# 从外部集群探测可观测性组件可用性
apiVersion: monitoring.coreos.com/v1
kind: Probe
metadata:
  name: observability-endpoints
  namespace: monitoring
spec:
  interval: 30s
  module: http_2xx
  prober:
    url: blackbox-exporter.external:9115
  targets:
    staticConfig:
      static:
        - https://grafana.example.com/api/health
        - https://prometheus.example.com/-/healthy
        - https://alertmanager.example.com/-/healthy
---
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: blackbox-alerts
spec:
  groups:
    - name: blackbox.rules
      rules:
        - alert: ObservabilityEndpointDown
          expr: probe_success{job="probe/observability-endpoints"} == 0
          for: 3m
          labels:
            severity: critical
          annotations:
            summary: "可观测性端点不可达: {{ $labels.instance }}"
```

---

## 9. 多集群可观测性治理

### 架构模式

| 模式 | 适用 | 优势 | 劣势 |
|------|------|------|------|
| 每集群独立 | 小集群 | 简单、无跨集群依赖 | 无全局视图 |
| Thanos/Cortex 汇聚 | 中大规模 | 全局查询、去重 | 复杂度高 |
| VictoriaMetrics 集群 | 大规模 | 高性能、低存储 | 商业版功能 |
| Grafana Cloud | 全托管 | 零运维 | 成本、数据出境 |

### 多集群 Prometheus 联邦

```yaml
# 全局 Prometheus 配置 (Thanos Query)
apiVersion: v1
kind: ConfigMap
metadata:
  name: thanos-query-config
  namespace: monitoring
data:
  stores.yaml: |
    stores:
      - dnssrv+_grpc._tcp.thanos-store.prod-cn.svc:10901
      - dnssrv+_grpc._tcp.thanos-store.prod-us.svc:10901
      - dnssrv+_grpc._tcp.thanos-store.staging.svc:10901
```

### 全局告警去重

```yaml
# Thanos Ruler 全局告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: global-alerts
  namespace: monitoring
spec:
  groups:
    - name: global.rules
      rules:
        # 跨集群去重：同一告警只触发一次
        - alert: ServiceDownGlobal
          expr: |
            count by (service) (
              up{job=~".*-service"} == 0
            ) > 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "服务 {{ $labels.service }} 在多个集群中不可用"
```

---

## 10. 可观测性成本优化

### 成本分布分析

| 组件 | 主要成本驱动 | 优化策略 | 潜在节省 |
|------|------------|----------|----------|
| Prometheus | 时间序列数 × 保留期 | 降低基数、缩短保留 | 40-60% |
| Loki | 日志量 × 保留期 | 采样、过滤、分级 | 50-70% |
| Tempo | Span 量 × 保留期 | 尾部采样 | 60-80% |
| Grafana | 查询频率 | 缓存、降低刷新率 | 20-30% |

### 指标基数治理

```bash
# 🟢 查看 Top 高基数指标
curl -s 'http://prometheus:9090/api/v1/label/__name__/values' | \
  jq -r '.data[]' | while read metric; do
    count=$(curl -s "http://prometheus:9090/api/v1/query?query=count($metric)" | \
      jq '.data.result[0].value[1]')
    echo "$count $metric"
  done | sort -rn | head -20

# 🟢 查看高基数标签
curl -s 'http://prometheus:9090/api/v1/series?match[]=http_requests_total' | \
  jq '.data | length'
```

### Recording Rules 降低查询成本

```yaml
# 预计算常用查询，降低实时查询压力
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cost-optimization-rules
spec:
  groups:
    - name: recording.rules
      interval: 30s
      rules:
        - record: job:http_requests:rate5m
          expr: sum(rate(http_requests_total[5m])) by (job)
        - record: job:http_errors:ratio5m
          expr: |
            sum(rate(http_requests_total{code=~"5.."}[5m])) by (job)
            /
            sum(rate(http_requests_total[5m])) by (job)
        - record: namespace:cpu_usage:rate5m
          expr: |
            sum(rate(container_cpu_usage_seconds_total{
              container!="POD",container!=""
            }[5m])) by (namespace)
```

---

## 11. 可观测性容量规划

### 容量估算公式

```
Prometheus 存储 = 时间序列数 × 每样本字节 × 采样频率 × 保留时间
               = series × 1.5B × (1/scrape_interval) × retention

示例: 500K 序列, 30s 采样, 30天保留
= 500,000 × 1.5 × (1/30) × 30×24×3600
≈ 64.8 GB (压缩前)
≈ 10-15 GB (压缩后, ~5:1)
```

### 容量监控告警

```yaml
- alert: PrometheusStorageFull
  expr: |
    predict_linear(prometheus_tsdb_storage_retentions_total[6h], 72*3600) >
    prometheus_tsdb_storage_retentions_total
  for: 30m
  labels:
    severity: warning
  annotations:
    summary: "Prometheus 存储预计 3 天内耗尽"

- alert: LokiIngestionLagging
  expr: |
    loki_ingester_streams_created_total - loki_ingester_streams_removed_total > 100000
  for: 15m
  labels:
    severity: warning
```

## 8. 相关 Runbook / 推荐阅读

- [[13-生产运维/00-总览/99-production-readiness-operations-guide.md|生产运维域生产就绪运维指南]]
- [[09-可观测性/01-总览/99-production-readiness-operations-guide.md|可观测性生产就绪运维指南]]
- [[09-可观测性/06-SLO-SLI/99-slo-operations-guide.md|SLO 运维指南]]
- [[09-可观测性/06-SLO-SLI/02-slo-implementation-guide.md|SLO 设定与实施指南]]
- [[13-生产运维/03-事件响应/03-on-call-playbook.md|值班手册与告警响应规范]]
- [[13-生产运维/03-事件响应/04-incident-response-template.md|事故响应模板与流程规范]]
- 告警质量治理与告警疲劳缓解（待补充）

---

*可观测性运维的核心是让数据驱动决策，而不是让数据淹没工程师。建议每季度回顾 SLO、告警规则与保留策略，确保其与业务优先级和成本预算保持一致。*
