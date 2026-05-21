---
title: 'Day 22: 企业监控 - Prometheus 企业级 + Grafana'
description: 'title: Day 22: 企业监控 - Prometheus 企业级 + Grafana'
category: learning
tags:
- k8s
- training
- hands-on
- prometheus
- grafana
- gateway
- operator
- webhook
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 22: 企业监控 - Prometheus 企业级 + Grafana 是什么'
- '如何 Day 22: 企业监控 - Prometheus 企业级 + Grafana'
trigger_keywords:
- Day
- '22:'
- 企业监控
- Prometheus
- 企业级
- Grafana
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
---

---
title: Day 22: 企业监控 - Prometheus 企业级 + Grafana
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - Prometheus 企业级监控架构
  - Thanos 跨集群监控
  - Grafana 企业级配置
  - SLO/SLI 体系设计
trigger_keywords:
  - Thanos
  - SLO
  - SLI
  - 企业监控
  - Prometheus
  - Grafana
  - 错误预算
  - 黄金信号
reading_level: advanced
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 240min
related_domains:
  - domain-20-enterprise-monitoring-alerting
  - domain-06-observability
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-23-logging-gitops
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/[[domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-17-observability-1|day-17-observability-1]]
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
---

# Day 22: 企业监控 - Prometheus 企业级 + Grafana

## 概述

今天进入企业级监控体系的学习。在前面的课程中，你已经掌握了 Prometheus 的基础用法——部署、配置 Target、编写 PromQL 查询和创建基础告警规则。今天将视野扩展到企业级场景：如何实现跨集群监控？如何设计 SLO/SLI 体系？如何构建高质量的 Grafana Dashboard？

企业级监控与基础监控的核心区别在于三个维度：**规模**（从单集群到多集群、从数十节点到数千节点）、**深度**（从基础指标到业务指标、从技术指标到 SLO）、**流程**（从看监控面板到系统化的告警响应和错误预算管理）。

### 学习目标

- 理解 Thanos 跨集群监控方案的架构与部署
- 掌握 SLO/SLI 体系的设计方法与错误预算管理
- 能够设计高质量的企业级 Grafana Dashboard
- 理解告警优化策略（分组、抑制、静默、路由）

---

## 核心概念详解

### Prometheus 企业级架构

单集群的 Prometheus 在数据采集和存储方面有以下局限性：

- **数据孤岛**: 每个集群有独立的 Prometheus 实例，无法在一个面板上查看所有集群的数据
- **存储容量有限**: Prometheus 本地存储（TSDB）的容量受单机磁盘限制，通常只能保存 15-30 天的数据
- **高可用不足**: 单个 Prometheus 实例故障会导致监控数据丢失

**Thanos** 是解决这些问题的主流方案。它通过一组组件将多个 Prometheus 实例整合为统一的监控平台：

- **Thanos Sidecar**: 部署在每个 Prometheus 实例旁（作为 Sidecar Container），有两个核心功能：将 Prometheus 的数据块（Block）上传到对象存储（如 OSS），以及暴露 StoreAPI 让 Querier 可以查询这个 Prometheus 的数据
- **Thanos Querier**: 统一的查询入口。它实现了 Prometheus 的 HTTP API，可以同时查询多个数据源（包括 Sidecar、Store Gateway 和其他 Querier）。当你执行一个 PromQL 查询时，Querier 会将查询分发到所有数据源，汇总结果后返回
- **Thanos Store Gateway**: 从对象存储中读取历史数据。当 Querier 需要查询超过 Prometheus 本地保留期的历史数据时，Store Gateway 从对象存储中读取并返回
- **Thanos Compactor**: 对对象存储中的数据块进行压缩和降采样。它将多个小块合并为大块以减少对象存储的请求次数，同时生成 5 分钟和 1 小时的降采样数据以加速长时间范围查询
- **Thanos Ruler**: 支持跨集群的告警规则评估和 Recording Rules。当需要在全局维度评估告警（如所有集群的总错误率）时，使用 Thanos Ruler

**高可用部署**的关键实践：

- 每个 Prometheus 实例配置两个副本（使用相同的外部标签），Thanos Querier 会自动去重
- 使用 Prometheus 的 `--storage.tsdb.retention.time` 设置本地保留期（通常 2 小时即可，因为数据会上传到对象存储）
- Thanos Querier 使用 `--query.replica-label` 参数指定用于去重的标签

### SLO/SLI 体系设计

SLO/SLI 体系是 Google SRE 方法的核心。它将服务的可靠性从模糊的"尽量不出故障"转变为可量化的、可管理的数据驱动流程。

**SLI（Service Level Indicator）** 是衡量服务健康度的具体指标。一个好的 SLI 应该直接反映用户体验。常见的 SLI 包括：

- **可用性 SLI**: 成功请求的比例。计算公式：`sum(rate(http_requests_total{status!~"5.."}[5m])) / sum(rate(http_requests_total[5m])) * 100`
- **延迟 SLI**: 请求响应时间的分位值。计算公式：`histogram_quantile(0.99, sum(rate(http_request_duration_seconds_bucket[5m])) by (le))`
- **质量 SLI**: 请求的非错误但质量下降的比例（如返回了过期的缓存数据）

**SLO（Service Level Objective）** 是为 SLI 设定的目标值。例如：可用性 SLO 设为 99.9%，意味着 99.9% 的请求应该成功。SLO 的设定应该基于用户需求而非技术能力——如果用户需要 99.9% 的可用性，你的系统就应该以此为目标，而不是因为"技术上很难实现"就降低标准。

**错误预算（Error Budget）** 是 SLO 的逆向思考：如果 SLO 是 99.9%，那么错误预算就是 0.1%。在 30 天的窗口内，错误预算是 30 * 24 * 60 * 0.001 = 43.2 分钟的不可用时间。错误预算的用途：

- **指导发布决策**: 如果错误预算还有剩余，可以放心发布新功能；如果预算已耗尽，应该暂停发布，专注于修复稳定性问题
- **协调团队目标**: 开发团队和 SRE 团队共享错误预算。开发团队用预算来发布新功能，SRE 团队用预算来降低风险
- **告警分级**: 错误预算消耗速度异常时触发告警，而不是等到预算完全耗尽

### Grafana Dashboard 设计原则

高质量的 Dashboard 应该遵循"从上到下，从总到分"的设计原则：

**第一层：黄金信号总览**

Google 的四个黄金信号是监控的核心维度：

- **Latency（延迟）**: 请求的响应时间，区分成功和失败请求的延迟
- **Traffic（流量）**: 请求的吞吐量（QPS）
- **Errors（错误）**: 失败请求的比例
- **Saturation（饱和度）**: 资源使用的接近极限程度（如 CPU 使用率、连接池使用率）

**第二层：SLO 达成面板**

- 当前 SLO 达成率（如 99.95%）
- 错误预算剩余量和消耗趋势
- 30 天 SLO 趋势线

**第三层：资源使用面板**

- CPU 使用率（Request vs Limit）
- 内存使用率（Request vs Limit）
- 网络流量（入站/出站）
- 磁盘 IO

**第四层：详细指标面板**

- 单个 Pod 的指标
- 慢请求分布
- 错误码分布

### 告警优化策略

Alertmanager 提供了多种告警优化机制：

- **分组（Grouping）**: 将相关的告警合并为一个通知。例如，当某个集群的多个节点同时异常时，只发送一条汇总通知而非几十条独立告警
- **抑制（Inhibition）**: 当某个高级别告警触发时，自动静默相关的低级别告警。例如，当"集群不可达"告警触发时，抑制该集群下所有的"Pod 异常"告警
- **静默（Silencing）**: 在维护窗口或已知问题期间，暂时关闭特定告警的通知
- **路由（Routing）**: 根据告警的标签（severity、team、cluster 等）将告警发送到不同的通知渠道（钉钉、企微、邮件、PagerDuty）

---

## 实战演练

### 任务 1: SLO/SLI 设计与告警规则 (1.5h)

```bash
# 创建 SLO 告警规则
cat > slo-rules.yaml << 'EOF'
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: slo-alerts
  namespace: monitoring
spec:
  groups:
  - name: slo-availability
    rules:
    - alert: HighErrorRate
      expr: |
        (
          sum(rate(http_requests_total{status=~"5.."}[5m]))
          /
          sum(rate(http_requests_total[5m]))
        ) > 0.001
      for: 5m
      labels:
        severity: critical
        slo: availability
      annotations:
        summary: "Error rate > 0.1% (SLO: 99.9%)"
        error_budget_consumed: "{{ $value | humanizePercentage }}"
    - alert: ErrorBudgetBurnRate
      expr: |
        (
          sum(rate(http_requests_total{status=~"5.."}[1h]))
          /
          sum(rate(http_requests_total[1h]))
        ) > (1 - 0.999) * 14.4
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Error budget burning too fast (14.4x rate)"
  - name: slo-latency
    rules:
    - alert: HighLatencyP99
      expr: |
        histogram_quantile(0.99, 
          sum(rate(http_request_duration_seconds_bucket[5m])) by (le)
        ) > 0.5
      for: 10m
      labels:
        severity: warning
        slo: latency
      annotations:
        summary: "P99 latency > 500ms (SLO: P99 < 500ms)"
EOF

kubectl apply -f slo-rules.yaml

# 创建 Recording Rules 预计算 SLO 指标
cat > slo-recording-rules.yaml << 'EOF'
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: slo-recording
  namespace: monitoring
spec:
  groups:
  - name: slo.rules
    interval: 30s
    rules:
    - record: slo:request_availability:ratio_rate5m
      expr: |
        sum(rate(http_requests_total{status!~"5.."}[5m]))
        /
        sum(rate(http_requests_total[5m]))
    - record: slo:request_latency:p99_rate5m
      expr: |
        histogram_quantile(0.99,
          sum(rate(http_request_duration_seconds_bucket[5m])) by (le)
        )
EOF

kubectl apply -f slo-recording-rules.yaml
```

### 任务 2: 高级 Grafana Dashboard (1.5h)

```bash
# 访问 Grafana
kubectl port-forward -n monitoring svc/prometheus-grafana 3000:80

# 默认登录: admin / prom-operator
# 导入以下社区 Dashboard:
# - Dashboard ID: 315 (Kubernetes cluster monitoring)
# - Dashboard ID: 6417 (Kubernetes pods monitoring)
# - Dashboard ID: 1860 (Node Exporter Full)
# - Dashboard ID: 7249 (Kubernetes Deployment)

# 手动创建黄金信号 Dashboard:
# 在 Grafana UI 中创建新的 Dashboard，添加以下 Panel:

# Panel 1: 请求率 (Rate)
# Query: sum(rate(http_requests_total[5m])) by (service)

# Panel 2: 错误率 (Errors)
# Query: sum(rate(http_requests_total{status=~"5.."}[5m])) by (service) / sum(rate(http_requests_total[5m])) by (service) * 100

# Panel 3: 延迟 (Latency)
# Query: histogram_quantile(0.99, sum(rate(http_request_duration_seconds_bucket[5m])) by (le, service))

# Panel 4: 饱和度 (Saturation)
# Query: (1 - (sum(kube_node_status_allocatable{resource="cpu"}) - sum(node_cpu_seconds_total{mode!="idle"})) / sum(kube_node_status_allocatable{resource="cpu"})) * 100
```

### 任务 3: Alertmanager 告警路由配置 (1h)

```bash
# 查看 Alertmanager 配置
kubectl get secret -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager \
  -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d

# 配置告警路由
cat > alertmanager-config.yaml << 'EOF'
apiVersion: v1
kind: Secret
metadata:
  name: alertmanager-prometheus-kube-prometheus-alertmanager
  namespace: monitoring
stringData:
  alertmanager.yaml: |
    global:
      resolve_timeout: 5m
    
    route:
      group_by: ['alertname', 'cluster', 'namespace']
      group_wait: 30s
      group_interval: 5m
      repeat_interval: 4h
      receiver: 'default'
      routes:
      - match:
          severity: critical
        receiver: 'critical'
        group_wait: 10s
        repeat_interval: 1h
      - match:
          severity: warning
        receiver: 'warning'
        group_wait: 30s
    
    inhibit_rules:
    - source_match:
        severity: 'critical'
      target_match:
        severity: 'warning'
      equal: ['alertname', 'cluster', 'namespace']
    
    receivers:
    - name: 'default'
      webhook_configs:
      - url: 'http://alertmanager-webhook:8080/alerts'
    - name: 'critical'
      webhook_configs:
      - url: 'http://alertmanager-webhook:8080/critical'
    - name: 'warning'
      webhook_configs:
      - url: 'http://alertmanager-webhook:8080/warning'
EOF

kubectl apply -f alertmanager-config.yaml
```

---

## 常见问题

### Q1: Thanos 和 VictoriaMetrics 如何选择？

Thanos 是 CNCF 孵化项目，生态成熟，与 Prometheus 完全兼容。VictoriaMetrics 是一个高性能的时序数据库，兼容 Prometheus 协议，部署更简单。如果你已经有多套 Prometheus 且需要统一的查询入口，选 Thanos。如果你从零开始且追求性能和简单性，可以考虑 VictoriaMetrics。

### Q2: SLO 设定得太高导致错误预算不够用怎么办？

SLO 不是越高越好。过高的 SLO 意味着几乎不允许出错，这会极大限制团队的迭代速度。建议根据业务影响来确定 SLO：核心支付服务可以设 99.99%，内部工具设 99.9% 即可。当 SLO 经常被突破时，不要急于提高 SLO，而是先分析是系统性问题还是偶发事件。

### Q3: 告警太多导致"告警疲劳"怎么办？

告警疲劳是运维团队最常见的问题。解决方法：1) 定期审查告警规则，删除不再有价值的告警；2) 调整告警阈值，减少误报；3) 使用 Alertmanager 的分组和抑制功能减少重复告警；4) 每个告警都应该有明确的响应动作，如果不知道该怎么做，这个告警就没有价值。

### Q4: Grafana Dashboard 太多，如何管理？

推荐按以下层级组织 Dashboard：1) 全局概览 Dashboard（一个集群一个）；2) 服务级别 Dashboard（每个核心服务一个）；3) 基础设施 Dashboard（节点、网络、存储）；4) 调试 Dashboard（用于深入排查特定问题）。使用 Grafana 的文件夹和标签功能进行分类管理。

---

## 要点总结

| 知识点 | 要点 |
|--------|------|
| Thanos 架构 | Sidecar + Querier + Store Gateway + Compactor 实现跨集群查询和长期存储 |
| SLO/SLI | SLI 衡量用户体验，SLO 设定目标，错误预算指导决策 |
| 黄金信号 | Latency、Traffic、Errors、Saturation |
| Dashboard 设计 | 从总到分、从黄金信号到详细指标 |
| 告警优化 | 分组、抑制、静默、路由 |

---

## 延伸阅读

- [Prometheus 企业级监控](../../domain-06-observability/01-prometheus-enterprise-monitoring.md)
- [Grafana 企业级可观测性](../../domain-06-observability/02-grafana-enterprise-observability.md)
- [SLO/SLI 体系](../../domain-06-observability/18-slo-sli-system.md)
- [可观测性架构总览](../../domain-06-observability/01-observability-architecture-overview.md)
- [Prometheus 监控](../../domain-06-observability/10-monitoring-metrics-prometheus.md)
