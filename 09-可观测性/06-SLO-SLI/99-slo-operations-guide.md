---
title: SLO 运维指南
description: Kubernetes 生产环境 SLO 运维指南，覆盖 SLI/SLO/SLA 定义、错误预算、burn-rate 告警、告警 review 机制与 Dashboard-as-Code。
summary: SLO 运维指南，覆盖 SLI/SLO/SLA、错误预算、burn-rate 告警、告警 review 与 Dashboard-as-Code。
category: observability
tags:
- production
- best-practices
- playbook
- observability
- slo
- sli
- sla
- error-budget
- burn-rate
- dashboard-as-code
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- 监控工程师
estimated_read_time: 25min
intent_queries:
- SLO 运维指南是什么
- 如何在 Kubernetes 定义 SLI/SLO/SLA
- burn-rate 告警与错误预算如何落地
trigger_keywords:
- SLO
- SLI
- SLA
- 错误预算
- burn-rate
- 告警 review
- Dashboard-as-Code
prerequisites:
- kubectl-basics
- prometheus-basics
- observability-basics
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# SLO 运维指南

> **适用范围**: 在 Kubernetes 上为平台组件与业务服务定义、监控和运营 SLO 的团队。
> **目标读者**: SRE、平台工程师、监控工程师。
> **最后更新**: 2026-07-01

本指南是 [[09-可观测性/01-总览/99-production-readiness-operations-guide.md|可观测性生产就绪运维指南]] 的 SLO 专项 runbook，参考 [[36-报告/assessments/domain-content-gap-analysis-2026-07-01.md|域内容缺口分析]] 中“Observability / SLO Operations”缺口，系统覆盖 SLI/SLO/SLA、错误预算、burn-rate 告警、告警 review 机制与 Dashboard-as-Code。

---

## 1. 适用场景与范围

- 为 Kubernetes 平台组件（Ingress、CoreDNS、GitOps 控制器等）定义 SLO。
- 为业务服务制定 SLI、SLO、SLA 与错误预算政策。
- 配置 burn-rate 告警，实现快速耗尽时的提前预警。
- 建立告警质量 review cadence，降低告警疲劳。
- 使用 GitOps 管理 Grafana Dashboard 与 PrometheusRule。

---

## 2. 前置条件与工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 必备工具
kubectl version --client
helm version
promtool --version
# 可选：slo-generator / pyrra
```
- 已部署 Prometheus + Alertmanager + Grafana。
- 已采集关键服务 RED 指标（Request Rate、Errors、Duration）。
- 已建立变更管理与事故响应流程。

---

## 3. 核心概念/架构

| 术语 | 定义 | 示例 |
|---|---|---|
| **SLI** | 服务质量指标，可量化 | 7 天内 P99 延迟 < 200ms 的请求占比 |
| **SLO** | 服务等级目标，SLI 的目标值 | P99 延迟 < 200ms 的占比 ≥ 99.9% |
| **SLA** | 对外承诺，通常附带赔偿 | 月度可用性 ≥ 99.95% |
| **Error Budget** | SLO 允许的失败比例 | 月度不可用预算 ≈ 0.1%（约 43 分钟） |

错误预算消耗速度决定告警灵敏度：**burn rate** 越高，说明故障越急迫。

---

## 4. 标准操作流程

### 4.1 选择 SLI

对常见服务类型建议：

| 服务类型 | 推荐 SLI | 说明 |
|---|---|---|
| Web / API | 可用性 + 延迟 | 2xx/5xx 比例、P99 延迟 |
| 异步处理 | 吞吐量 + 错误率 | 处理速率、死信队列长度 |
| 存储 | 可用性 + 一致性 | 读写成功率、复制延迟 |
| 批处理 | 完成率 + 耗时 | 任务成功率、P95 完成时间 |

### 4.2 定义 SLO 与错误预算

示例：API 可用性 SLO = 99.9%（月度）。

```text
月度错误预算 = (1 - 0.999) × 30 天 × 24 小时 × 60 分钟 = 43.2 分钟
```

```yaml
# PrometheusRule 示例：错误预算与 burn rate
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: api-slo
  namespace: monitoring
spec:
  groups:
  - name: slo
    rules:
    - record: slo:api_availability:ratio_rate30d
      expr: |
        sum(rate(http_requests_total{job="api",code!~"5.."}[30d]))
        /
        sum(rate(http_requests_total{job="api"}[30d]))

    - alert: APIErrorBudgetBurnFast
      expr: |
        (
          sum(rate(http_requests_total{job="api",code=~"5.."}[1h]))
          /
          sum(rate(http_requests_total{job="api"}[1h]))
        ) > (1 - 0.999) * 14.4
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "API 错误预算快速燃烧（2% in 1h）"

    - alert: APIErrorBudgetBurnSlow
      expr: |
        (
          sum(rate(http_requests_total{job="api",code=~"5.."}[6h]))
          /
          sum(rate(http_requests_total{job="api"}[6h]))
        ) > (1 - 0.999) * 6
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "API 错误预算缓慢燃烧，预计月度超标"
EOF
```

### 4.3 Burn-Rate 告警规则

Google SRE Workbook 推荐 multiwindow multi-burn-rate：

| Burn Rate | 窗口 | 含义 | 响应目标 |
|---|---|---|---|
| 14.4x | 1h / 5m | 2% 预算将在 1h 内耗尽 | 立即 page |
| 6x | 6h / 30m | 5% 预算将在 6h 内耗尽 | 工作时间内处理 |
| 3x | 3d / 6h | 长期劣化 | 次日处理 |

### 4.4 Dashboard-as-Code

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 Grafana Operator 或 ConfigMap 管理 Dashboard
kubectl create configmap grafana-dashboard-api-slo \
  --from-file=api-slo.json \
  -n monitoring \
  --dry-run=client -o yaml | kubectl apply -f -

# 推荐通过 Jsonnet / Grizzly 在 CI 中生成并校验
```
Dashboard 必备面板：

- SLI 实时值与 SLO 阈值对比。
- 错误预算剩余百分比与消耗趋势。
- Burn-rate 当前值。
- 近 30 天告警列表与误标率。

### 4.5 告警 Review Cadence

```bash
# 每周统计告警触发次数与误报率
curl -s 'http://alertmanager:9093/api/v1/alerts' | \
  jq -r '.data[] | .labels.alertname' | sort | uniq -c | sort -rn
```

- **每周告警质量会**: review critical 告警，目标 critical 告警 < 5 条/周，误报 < 5%。
- **每月 SLO 复盘**: 检查错误预算消耗原因，更新 SLO 或业务目标。
- **每季度阈值评审**: 根据业务变化调整 burn-rate 系数与窗口。

---

## 5. 关键检查点与验证命令

| 检查项 | 验证命令 | 通过标准 |
|---|---|---|
| SLI 指标存在 | `curl -s 'http://prometheus:9090/api/v1/label/__name__/values' \| grep slo` | 已定义 recording rule |
| SLO Dashboard 可访问 | `curl -s http://grafana:3000/api/dashboards/uid/<uid>` | 200 OK |
| Burn-rate 告警生效 | `kubectl get prometheusrules -n monitoring` | 存在并 VALID |
| 错误预算趋势可见 | Grafana 面板 | 剩余预算百分比持续更新 |
| 告警路由正确 | `amtool config routes test` | critical 路由到 PagerDuty |

---

## 6. 常见故障与 remediation

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|---|---|---|---|
| SLO Dashboard 显示错误预算耗尽 | 真实故障或 SLI 计算错误 | `promtool query instant` | 验证 SLI 表达式；真实故障启动事故响应 |
| Burn-rate 告警频繁误报 | SLO 过严或流量低谷期抖动 | `rate(http_requests_total[1h])` | 放宽 SLO、增大 for 窗口、使用 min 请求数过滤 |
| 告警未触发但服务已降级 | SLI 缺失或标签不匹配 | `curl /api/v1/query?query=<sli>` | 补齐缺失指标、修正 label selector |
| 错误预算人为耗尽 | 计划内维护未排除 | `sum(rate(...{maintenance!="true"}))` | 在 SLI 中排除计划维护窗口 |
| Dashboard 与规则不一致 | GitOps 未同步 | `argocd app diff <app>` | 强制同步或回滚到上一版本 |

---

## 7. 多窗口 Burn-Rate 告警体系

### 完整告警矩阵

| 窗口 | Burn Rate | 消耗预算 | 响应 | 级别 |
|------|-----------|----------|------|------|
| 5m | 14.4x | 2% in 1h | 立即响应 | Critical (Page) |
| 30m | 6x | 5% in 6h | 尽快响应 | Critical (Page) |
| 2h | 3x | 10% in 2d | 工作时间内 | Warning (Ticket) |
| 6h | 1x | 10% in 30d | 观察 | Info |

### 完整 PrometheusRule

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: api-slo-complete
  namespace: monitoring
spec:
  groups:
    - name: api.slo.recording
      interval: 30s
      rules:
        # SLI 计算
        - record: sli:api_availability:ratio_rate5m
          expr: |
            sum(rate(http_requests_total{job="api",code!~"5.."}[5m]))
            /
            sum(rate(http_requests_total{job="api"}[5m]))
        - record: sli:api_availability:ratio_rate30m
          expr: |
            sum(rate(http_requests_total{job="api",code!~"5.."}[30m]))
            /
            sum(rate(http_requests_total{job="api"}[30m]))
        - record: sli:api_availability:ratio_rate2h
          expr: |
            sum(rate(http_requests_total{job="api",code!~"5.."}[2h]))
            /
            sum(rate(http_requests_total{job="api"}[2h]))
        - record: sli:api_availability:ratio_rate30d
          expr: |
            sum(rate(http_requests_total{job="api",code!~"5.."}[30d]))
            /
            sum(rate(http_requests_total{job="api"}[30d]))
        # 错误预算剩余
        - record: slo:api_error_budget_remaining:ratio
          expr: |
            1 - (
              (1 - sli:api_availability:ratio_rate30d)
              /
              (1 - 0.999)
            )

    - name: api.slo.alerts
      rules:
        # 快速燃烧 (Critical - Page)
        - alert: APIErrorBudgetFastBurn
          expr: |
            sli:api_availability:ratio_rate5m < (1 - (1-0.999)*14.4)
            and
            sli:api_availability:ratio_rate1h:availability < (1 - (1-0.999)*14.4)
          for: 2m
          labels:
            severity: critical
            slo: api-availability
            team: platform
          annotations:
            summary: "API 错误预算快速燃烧 (14.4x)"
            description: "当前错误率将在 1h 内消耗 2% 错误预算"
            runbook: "https://runbooks.example.com/api-slo"

        # 中速燃烧 (Critical - Page)
        - alert: APIErrorBudgetMediumBurn
          expr: |
            sli:api_availability:ratio_rate30m < (1 - (1-0.999)*6)
          for: 15m
          labels:
            severity: critical
            slo: api-availability
          annotations:
            summary: "API 错误预算中速燃烧 (6x)"

        # 慢速燃烧 (Warning - Ticket)
        - alert: APIErrorBudgetSlowBurn
          expr: |
            sli:api_availability:ratio_rate2h < (1 - (1-0.999)*3)
          for: 1h
          labels:
            severity: warning
            slo: api-availability
          annotations:
            summary: "API 错误预算慢速燃烧 (3x)"
```

## 8. 错误预算政策与执行

### 预算耗尽时的决策框架

```
错误预算剩余 > 50%:
└── 正常发布节奏，鼓励创新

错误预算剩余 25-50%:
└── 加强变更审查，金丝雀时间加倍

错误预算剩余 10-25%:
└── 仅允许可靠性修复，暂停功能发布

错误预算剩余 < 10%:
└── 发布冻结，全力修复可靠性问题
└── 必须完成复盘 + 改进计划后才解冻
```

### 自动化预算检查（发布门禁）

```yaml
# Argo Rollouts AnalysisTemplate — 发布前检查错误预算
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: error-budget-check
spec:
  metrics:
    - name: error-budget-remaining
      interval: 5m
      successCondition: result[0] > 0.1  # 剩余 > 10% 才允许发布
      failureCondition: result[0] <= 0.1
      failureLimit: 1
      provider:
        prometheus:
          address: http://prometheus.monitoring:9090
          query: |
            1 - (
              (1 - sum(rate(http_requests_total{job="api",code!~"5.."}[30d]))
              / sum(rate(http_requests_total{job="api"}[30d])))
              /
              (1 - 0.999)
            )
```

## 9. SLO 报告与审查

### 月度 SLO 报告模板

```markdown
# SLO 月报 — 2026-07

## 总览
| 服务 | SLO 目标 | 实际达成 | 错误预算剩余 | 趋势 |
|------|----------|----------|--------------|------|
| API Gateway | 99.9% | 99.95% | 52% | ↑ |
| Order Service | 99.9% | 99.87% | -3% | ↓ |
| Payment | 99.99% | 99.99% | 78% | → |

## 错误预算消耗事件
| 日期 | 服务 | 消耗 | 原因 | 改进 |
|------|------|------|------|------|
| 07-05 | Order | 8% | DB 连接池耗尽 | 增加池大小+告警 |
| 07-12 | API | 3% | 上游超时 | 调整超时配置 |

## 告警质量
- 总告警数: 47
- 真实告警: 38 (81%)
- 误报: 9 (19%)
- 行动: 调整 3 个告警阈值

## 下月重点
- [ ] Order Service 连接池监控加强
- [ ] 新增延迟 SLO (P99 < 500ms)
- [ ] 告警误报率降至 < 10%
```

### 审查节奏

| 频率 | 内容 | 参与者 | 输出 |
|------|------|--------|------|
| 每日 | 错误预算消耗检查 | 值班 SRE | 异常记录 |
| 每周 | 告警质量审查 | SRE 团队 | 阈值调整 |
| 每月 | SLO 达成报告 | SRE + 产品 | 月报 + 改进项 |
| 每季 | SLO 目标校准 | 全团队 | SLO 调整决策 |

## 10. 不同服务类型的 SLO 示例

| 服务 | SLI | SLO | 窗口 | 备注 |
|------|-----|-----|------|------|
| 用户 API | 可用性 | 99.9% | 30d | 排除计划维护 |
| 用户 API | P99 延迟 | < 500ms | 30d | 排除缓存未命中 |
| 支付服务 | 成功率 | 99.99% | 30d | 含重试后成功 |
| 消息队列 | 消费延迟 | P95 < 5s | 7d | 端到端 |
| 批处理 | 完成率 | > 99.5% | 7d | 含重试成功 |
| 数据库 | 查询延迟 | P99 < 50ms | 30d | 排除慢查询 |
| DNS | 解析成功率 | 99.99% | 30d | 内部 DNS |
| Ingress | 5xx 率 | < 0.1% | 30d | 排除后端 5xx |

---

## 11. 风险与注意事项

- **SLO 不是越高越好**: 99.999% 的成本可能远超业务价值，应基于用户可感知影响设定。
- **错误预算不可无限累积**: 未使用的预算不代表可以任意挥霍，应作为发布与创新的安全缓冲。
- **计划内维护应排除**: 否则 SLO 会被正常的变更窗口拖低，失去指导意义。
- **避免单一指标驱动**: 同时监控 latency、error、throughput，防止为保可用性牺牲延迟。
- **Dashboard-as-Code 需版本化**: 所有 Dashboard、Rule 必须走 GitOps，禁止生产环境直接修改。

---

## 8. 相关 Runbook / 推荐阅读

### 同域核心文档

- [[09-可观测性/01-总览/99-production-readiness-operations-guide.md|可观测性生产就绪运维指南]]
- [[09-可观测性/06-SLO-SLI/18-slo-sli-system.md|SLO/SLI 体系建设与管理]]
- [[09-可观测性/06-SLO-SLI/01-slo-engineering-practice.md|SLO 工程实践]]
- [[09-可观测性/06-SLO-SLI/02-error-budget-policy.md|错误预算政策]]
- [[09-可观测性/06-SLO-SLI/03-sli-implementation-guide.md|SLI 实施指南]]
- [[09-可观测性/02-指标/99-prometheus-enterprise-guide.md|Prometheus 企业级监控部署指南]]
- [[09-可观测性/05-告警/21-monitoring-playbooks.md|监控 Playbooks]]

### 跨域参考

- [[36-报告/assessments/domain-content-gap-analysis-2026-07-01.md|域内容缺口分析]]
- [[12-可靠性/README.md|可靠性工程]]
- [[13-生产运维/README.md|生产运维]]

---

*本指南应每月根据错误预算消耗、告警误报率和业务变化进行 review。建议将 SLO 达成率纳入团队 OKR。*


<!-- risk-assessed -->
