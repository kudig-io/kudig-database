---
title: "SLO/Error Budget 框架"
category: concepts
tags:
  - sre
  - slo
  - sli
  - error-budget
  - reliability
  - k8s
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

# SLO/Error Budget 框架

> 可靠性不是越高越好——它是**刚好够用**的工程权衡。Error Budget 是连接可靠性目标与产品迭代速度的桥梁。

相关索引：[[domain-19-landscape-references/98-merged-indexes/index.md|index]] · [[concepts/incident-management-patterns.md|incident management patterns]]

---

## 1. SLI / SLO / Error Budget 三层模型

```
┌─────────────────────────────────────────────────────┐
│  SLI（Service Level Indicator）                      │
│  "我们用什么数字衡量用户体验？"                         │
│  例：请求成功率 = 成功请求数 / 总请求数                  │
├─────────────────────────────────────────────────────┤
│  SLO（Service Level Objective）                      │
│  "SLI 应该达到什么水平？"                              │
│  例：30 天滚动窗口内 SLI ≥ 99.9%                      │
├─────────────────────────────────────────────────────┤
│  Error Budget                                        │
│  "我们可以承受多少失败？"                              │
│  = 1 - SLO = 允许的错误比例                           │
│  例：99.9% SLO → 0.1% Error Budget                   │
│      30 天 = 43,200 秒 → 允许 ~43.2 秒故障             │
└─────────────────────────────────────────────────────┘
```

### 1.1 SLI 设计原则

| 原则 | 说明 |
|------|------|
| 用户视角 | 衡量用户真正感知到的体验，而非系统内部指标 |
| 可操作性 | SLI 劣化时团队能采取具体行动 |
| 代表性 | 单个 SLI 应反映一类用户旅程 |
| 可聚合 | 支持按服务/区域/客户群聚合 |

**常见 SLI 模式：**

```yaml
# 可用性 SLI
availability_sli: successful_requests / total_requests

# 延迟 SLI（以 P99 为例）
latency_sli: requests_completed_under_threshold / total_requests

# 正确性 SLI
correctness_sli: correct_responses / total_responses

# 吞吐量 SLI
throughput_sli: requests_processed_per_second >= target
```

### 1.2 SLO 分层策略

```
                99.99% (4个9)
               ┌──────────┐
               │ 关键路径  │  金融交易、支付、认证
               ├──────────┤
            99.9% (3个9)  │
           ┌──────────────┐
           │ 核心服务      │  API Gateway、主站、数据库
           ├──────────────┤
        99.5%              │
       ┌──────────────────┐
       │ 重要服务          │  搜索、推荐、通知
       ├──────────────────┤
    99.0%                  │
   ┌──────────────────────┐
   │ 内部工具 / 非关键服务  │  内部 Dashboard、批处理
   └──────────────────────┘
```

---

## 2. 多层级 Error Budget 策略

Error Budget 剩余比例决定团队的工作重心：

```
Error Budget 剩余    状态       行动策略
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  > 75%            🟢 正常     全速推进功能开发，常规可靠性改进
  50% ~ 75%        🟡 谨慎     功能开发需评估风险，加强变更审查
  25% ~ 50%        🟠 可靠性优先  新功能暂缓，集中修复可靠性债务
  < 25%            🔴 功能冻结  停止非必要变更，全力投入可靠性
   0%              ⛔ 紧急     全员紧急响应，停止一切新功能
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 2.1 策略执行矩阵

```yaml
error_budget_policy:
  # 30 天滚动窗口示例（SLO 99.9% → Budget 0.1% → ~43.2 分钟/月）

  normal:  # > 75% 剩余（已消耗 < 10.8 分钟）
    feature_velocity: full
    change_approval: standard
    reliability_work: backlog_priority
    deploy_freeze: none

  cautious:  # 50-75% 剩余（已消耗 10.8 ~ 21.6 分钟）
    feature_velocity: reviewed
    change_approval: enhanced_review
    reliability_work: elevated_priority
    deploy_freeze: none
    action: "评估每次发布的风险收益比"

  reliability_first:  # 25-50% 剩余（已消耗 21.6 ~ 32.4 分钟）
    feature_velocity: paused
    change_approval: senior_approval_required
    reliability_work: top_priority
    deploy_freeze: non_critical_only
    action: "暂停新功能，集中修复可靠性债务"

  feature_freeze:  # < 25% 剩余（已消耗 > 32.4 分钟）
    feature_velocity: stopped
    change_approval: director_approval
    reliability_work: emergency_priority
    deploy_freeze: all_non_emergency
    action: "全面功能冻结，可靠性专项冲刺"

  emergency:  # 0% 剩余（Budget 耗尽）
    feature_velocity: halted
    change_approval: vp_approval
    reliability_work: war_room
    deploy_freeze: total
    action: "启动紧急响应流程，与管理层同步"
```

### 2.2 Budget 耗尽时的处理流程

```
Budget 耗尽
    │
    ├── 1. 通知管理层 & 产品负责人
    ├── 2. 暂停所有非紧急发布
    ├── 3. 成立可靠性专项小组
    ├── 4. 分析 Budget 耗尽根因
    │       ├── 是单次大故障？
    │       └── 还是多次小故障累积？
    ├── 5. 制定恢复计划（时间线 + 里程碑）
    ├── 6. 执行修复
    └── 7. Budget 恢复后逐步解冻
```

---

## 3. Burn-Rate 告警策略

Burn Rate 表示 Error Budget 的消耗速度。`1x` = 按预期速度消耗（刚好在窗口结束时耗尽）。

### 3.1 Burn Rate 计算

```
burn_rate = actual_error_rate / expected_error_rate

例：SLO = 99.9%（允许 0.1% 错误）
    实际错误率 = 1.44%
    burn_rate = 1.44% / 0.1% = 14.4x

含义：以 14.4 倍速度消耗 Budget，
      本应 30 天耗尽的 Budget 在 ~2 天内耗尽
```

### 3.2 多窗口多 Burn Rate 告警

Google SRE 推荐的分层告警策略：

```
严重性     Burn Rate    短窗口    长窗口    触发条件      30天Budget消耗
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
P1 紧急    14.4x       1 小时    6 小时    AND-gate     ~2天内耗尽
P2 警告     6.0x       6 小时    1 天      AND-gate     ~5天内耗尽
P3 通知     3.0x       1 天      3 天      AND-gate     ~10天内耗尽
P4 低       1.0x       3 天      5 天      AND-gate     30天内耗尽
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 3.3 AND-Gate 原理

**为什么需要双窗口 AND-gate？**

```
单一窗口的问题：
  - 短窗口太敏感 → 误报多（短暂抖动触发告警）
  - 长窗口太迟钝 → 漏报多（故障已发生才告警）

AND-gate 解决方案：
  短窗口（快速检测） AND 长窗口（确认趋势）
  → 只有两个窗口同时满足才告警
  → 大幅减少误报，同时保持快速检测能力

示意图：

错误率 ▲
       │    ┌──┐  短暂抖动
       │    │  │  ← 短窗口触发，长窗口不触发 → 不告警 ✅
       │    │  │
       │    └──┘
       │
       │         ┌─────────────┐  持续劣化
       │         │             │  ← 短窗口+长窗口均触发 → 告警 🔔
       │         │             │
       │─────────┘             └───
       └──────────────────────────▶ 时间
```

### 3.4 Prometheus 告警规则示例

```yaml
# P1: 14.4x burn rate，1h + 6h AND-gate
groups:
  - name: slo_burn_rate
    rules:
      # 14.4x burn rate - P1 紧急
      - alert: SLOBurnRateCritical
        expr: |
          (
            # 短窗口：过去 1 小时 burn rate > 14.4x
            sum(rate(http_requests_total{code=~"5.."}[1h]))
            /
            sum(rate(http_requests_total[1h]))
          ) > (14.4 * (1 - 0.999))
          and
          (
            # 长窗口：过去 6 小时 burn rate > 14.4x
            sum(rate(http_requests_total{code=~"5.."}[6h]))
            /
            sum(rate(http_requests_total[6h]))
          ) > (14.4 * (1 - 0.999))
        for: 2m
        labels:
          severity: critical
        annotations:
          summary: "SLO Error Budget 以 14.4x 速度消耗"
          description: "按当前速度，2天内 Budget 将耗尽"

      # 6x burn rate - P2 警告
      - alert: SLOBurnRateWarning
        expr: |
          (
            sum(rate(http_requests_total{code=~"5.."}[6h]))
            /
            sum(rate(http_requests_total[6h]))
          ) > (6 * (1 - 0.999))
          and
          (
            sum(rate(http_requests_total{code=~"5.."}[1d]))
            /
            sum(rate(http_requests_total[1d]))
          ) > (6 * (1 - 0.999))
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "SLO Error Budget 以 6x 速度消耗"
          description: "按当前速度，5天内 Budget 将耗尽"
```

---

## 4. SLO as Code 工具对比

### 4.1 总览

| 特性 | OpenSLO | Sloth | Pyrra |
|------|---------|-------|-------|
| **定位** | 厂商中立标准/规范 | 最简单的 CLI + Operator | 最完整的 K8S Operator + UI |
| **格式** | OpenSLO YAML 标准 | 简化 YAML | OpenSLO / 自有格式 |
| **部署形态** | 规范 + 各实现 | CLI 生成 / Kubernetes Operator | Kubernetes Operator + Web UI |
| **Prometheus** | 支持（通过实现） | 原生支持 | 原生支持 |
| **多后端** | 设计支持（Datadog/Dynatrace等） | 仅 Prometheus | 仅 Prometheus |
| **学习曲线** | 中等 | 最低 | 中等 |
| **社区活跃度** | 高（CNCF 相关） | 中高 | 中 |
| **最佳场景** | 多厂商环境、标准化治理 | 快速上手、单一 Prometheus 栈 | 完整 K8S 原生体验 |

### 4.2 OpenSLO — 厂商中立标准

```yaml
# OpenSLO 格式示例
apiVersion: openslo/v1
kind: SLO
metadata:
  name: api-availability
spec:
  service: my-api
  description: "API 可用性 SLO"
  budgetingMethod: Occurrences
  objectives:
    - displayName: availability
      target: 0.999
      ratioMetrics:
        good:
          source: prometheus
          queryType: promql
          query: sum(rate(http_requests_total{code!~"5.."}[{{.window}}]))
        total:
          source: prometheus
          queryType: promql
          query: sum(rate(http_requests_total[{{.window}}]))
  timeWindow:
    - duration: 30d
      isRolling: true
  alertPolicies:
    - kind: AlertPolicy
      metadata:
        name: api-availability-critical
      spec:
        conditions:
          - kind: burnrate
            threshold: 14.4
            lookbackWindow: 1h
```

**优势：**
- 作为规范标准被多家工具采纳
- 避免供应商锁定
- 适合组织级 SLO 治理

**劣势：**
- 需要配合具体实现（如 OpenSLO Controller）
- 生态仍在成熟中

### 4.3 Sloth — 最简单的 SLO 工具

```yaml
# Sloth 格式示例
sloth/prometheus/v1:
  service: "my-api"
  labels:
    team: "platform"
  slos:
    - name: "availability"
      objective: 99.9
      description: "API 请求成功率"
      sli:
        events:
          error_query: sum(rate(http_requests_total{code=~"5.."}[{{.window}}]))
          total_query: sum(rate(http_requests_total[{{.window}}]))
      alerting:
        page_alert:
          labels:
            severity: critical
        ticket_alert:
          labels:
            severity: warning
```

**使用方式：**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# CLI 生成 Prometheus 规则
sloth generate -i slo.yaml -o prometheus-rules.yaml

# Kubernetes Operator 模式
# 自动将 Sloth CRD 转换为 PrometheusRule
kubectl apply -f slo.yaml
```

**优势：**
- 学习成本最低，5 分钟上手
- 自动生成多窗口 Burn-Rate 告警规则
- 支持 CLI + K8S Operator 双模式

**劣势：**
- 仅支持 Prometheus
- 自定义能力有限

### 4.4 Pyrra — 最完整的 K8S 原生方案

```yaml
# Pyrra 格式示例（兼容 OpenSLO）
apiVersion: pyrra.dev/v1alpha1
kind: ServiceLevelObjective
metadata:
  name: api-availability
  namespace: monitoring
spec:
  target: "99.9"
  window: 30d
  description: "API 可用性"
  indicator:
    ratio:
      errors:
        metric: http_requests_total{code=~"5.."}
      total:
        metric: http_requests_total
```

**架构：**

```
┌─────────┐     ┌──────────────────┐     ┌──────────────┐
│ Pyrra   │────▶│ Kubernetes API   │────▶│ Prometheus   │
│ UI      │     │ (SLO CRD)        │     │ Rules        │
└─────────┘     └──────────────────┘     └──────────────┘
     │                                        │
     │           ┌──────────────────┐         │
     └──────────▶│ Grafana Dashboard│◀────────┘
                 │ (自动创建)        │
                 └──────────────────┘
```

**优势：**
- Web UI 直观展示 Budget 状态
- 自动创建 Grafana Dashboard
- 自动生成 Prometheus 告警规则
- K8S 原生 CRD 管理

**劣势：**
- 仅支持 Prometheus
- 运维组件较多（Operator + UI + 前端）

### 4.5 选型决策树

```
需要多后端支持（Datadog/Dynatrace/...）？
    ├── 是 → OpenSLO（+ 对应 Connector）
    └── 否 → 仅 Prometheus？
                ├── 是 → 需要 Web UI？
                │         ├── 是 → Pyrra
                │         └── 否 → 快速上手优先？
                │                   ├── 是 → Sloth
                │                   └── 否 → Pyrra（功能最全）
                └── 否 → OpenSLO + 自定义适配
```

---

## 5. Error Budget 策略执行流程

### 5.1 端到端流程

```
阶段 1：定义                    阶段 2：度量
┌─────────────────┐           ┌─────────────────┐
│ 识别关键用户旅程  │           │ 部署 SLI 采集    │
│ → 选择 SLI       │           │ → Prometheus     │
│ → 设定 SLO 目标  │           │ → 计算 Error     │
│ → 确定时间窗口   │           │   Budget 剩余    │
└────────┬────────┘           └────────┬────────┘
         │                             │
         ▼                             ▼
阶段 3：告警                    阶段 4：策略执行
┌─────────────────┐           ┌─────────────────┐
│ 配置 Burn-Rate  │           │ Budget 剩余阈值  │
│ 多窗口告警规则   │           │ → 触发对应策略   │
│ → PagerDuty     │           │ → 通知相关方     │
│ → Slack 通知    │           │ → 调整工作优先级  │
└────────┬────────┘           └────────┬────────┘
         │                             │
         ▼                             ▼
阶段 5：回顾                    阶段 6：迭代
┌─────────────────┐           ┌─────────────────┐
│ 月度 SLO 审查    │           │ 调整 SLO 目标    │
│ → Budget 消耗   │           │ → 优化 SLI 准确性│
│   分析          │           │ → 改进告警规则   │
│ → 根因复盘      │           │ → 更新策略阈值   │
└─────────────────┘           └─────────────────┘
```

### 5.2 Budget 计算公式

```
# 基础计算
error_budget = 1 - SLO_target
budget_minutes_per_window = window_days × 24 × 60 × error_budget

# 示例
SLO = 99.9%, window = 30d
error_budget = 0.1% = 0.001
budget_minutes = 30 × 24 × 60 × 0.001 = 43.2 分钟/月

# 剩余 Budget 百分比
budget_remaining_pct = 1 - (total_bad_minutes / budget_minutes)

# Burn Rate
burn_rate = (current_error_rate / allowed_error_rate)
time_to_exhaustion = budget_remaining / current_consumption_rate
```

### 5.3 自动化策略执行（示例）

```yaml
# Budget 策略自动化配置
apiVersion: reliability/v1
kind: ErrorBudgetPolicy
metadata:
  name: api-service-policy
spec:
  sloRef: api-availability
  evaluationInterval: 5m
  thresholds:
    - name: normal
      minBudgetRemaining: 75%
      actions:
        - notify: slack#dev-team
        - set_label: "release-gate=green"

    - name: cautious
      minBudgetRemaining: 50%
      actions:
        - notify: slack#dev-team, slack#leads
        - set_label: "release-gate=yellow"
        - require_approval: tech-lead

    - name: reliability_first
      minBudgetRemaining: 25%
      actions:
        - notify: slack#all-eng, email#director
        - set_label: "release-gate=orange"
        - pause_releases: non-critical
        - create_jira: reliability-sprint

    - name: feature_freeze
      minBudgetRemaining: 0%
      actions:
        - notify: slack#all-eng, email#vp-eng
        - set_label: "release-gate=red"
        - pause_releases: all
        - create_incident: budget-exhausted

    - name: emergency
      minBudgetRemaining: -100%  # 超支
      actions:
        - notify: pagerduty#p1
        - pause_releases: all
        - escalate: cto
```

---

## 6. 最佳实践

### 6.1 SLO 设定

| 实践 | 说明 |
|------|------|
| 从用户旅程出发 | 不要从技术指标出发，从"用户期望什么体验"开始 |
| 宁低勿高 | 99.9% 比 99.99% 更实际——后者几乎不允许任何变更 |
| 少即是多 | 每个服务 2-4 个 SLO 足够，不要定义 20 个 |
| 合作定义 | SLO 应由 SRE + 产品 + 工程共同商定 |
| 定期审视 | 每季度审视 SLO 是否仍然反映用户期望 |

### 6.2 Error Budget 管理

| 实践 | 说明 |
|------|------|
| 预算透明 | Dashboard 实时展示 Budget 消耗，全团队可见 |
| 策略自动化 | 用代码定义阈值和动作，避免人为判断的滞后 |
| 区分计划与非计划 | 维护窗口的 downtime 应从 Budget 计算中排除 |
| 考虑季节性 | 大促期间适当调整 SLO 或 Budget 期望 |
| 记录例外 | 每次 Budget 例外使用都需要书面审批和复盘 |

### 6.3 告警最佳实践

| 实践 | 说明 |
|------|------|
| 多窗口 AND-gate | 减少误报的关键——单窗口告警不可靠 |
| Burn Rate 分级 | 不同严重性对应不同 Burn Rate 和窗口 |
| 聚焦 Page，降噪 Ticket | 高 Burn Rate → Page，低 Burn Rate → Ticket |
| 告警静默要谨慎 | 静默 SLO 告警等同于忽略用户影响 |
| 关联上下文 | 告警消息应包含 Budget 剩余、Burn Rate、影响范围 |

### 6.4 常见反模式

```
❌ 反模式 1：SLO 设定过高
   "我们要 99.999% 可用性"
   → 不允许任何变更，团队被 SLO 绑架

❌ 反模式 2：SLI 不代表用户体验
   "CPU 使用率 < 80%"
   → 用户不关心 CPU，关心的是请求是否成功

❌ 反模式 3：Budget 不执行
   "Budget 消耗完了但继续发版"
   → SLO 形同虚设，团队失去信任

❌ 反模式 4：告警太多
   "每次 CPU > 70% 就告警"
   → 告警疲劳，真正的 SLO 告警被淹没

❌ 反模式 5：忽略 Error Budget 的价值
   "Budget 只是数字，不重要"
   → 失去产品迭代与可靠性的平衡工具
```

### 6.5 成熟度路线图

```
Level 1：基础感知
  □ 定义 2-3 个关键 SLI
  □ 设定初始 SLO 目标
  □ 手动计算 Error Budget

Level 2：自动化度量
  □ 部署 SLI 采集 Pipeline
  □ 自动计算 Budget 剩余
  □ 基础 Dashboard 可视化

Level 3：告警集成
  □ 多窗口 Burn-Rate 告警
  □ 与 PagerDuty/Slack 集成
  □ 告警→事件→复盘闭环

Level 4：策略自动化
  □ Budget 阈值自动触发策略
  □ 与 CI/CD Pipeline 集成
  □ 发版门禁基于 Budget 状态

Level 5：组织级治理
  □ 全组织 SLO 标准化
  □ 跨团队 Budget 可视化
  □ SLO 驱动的架构决策
```

---

## 参考资源

- Google SRE Book: [Service Level Objectives](https://sre.google/sre-book/service-level-objectives/)
- Google SRE Workbook: [Alerting on SLOs](https://sre.google/workbook/alerting-on-slos/)
- [OpenSLO 规范](https://openslo.com/)
- [Sloth 项目](https://github.com/slok/sloth)
- [Pyrra 项目](https://github.com/pyrra-dev/pyrra)
- [SLO as Code 工具对比](https://sloth.sh/)

---

> **核心理念**：SLO 不是监控指标，是产品决策工具。Error Budget 不是惩罚机制，是创新与稳定的平衡器。

## Related

- [[concepts/incident-management-patterns.md|incident management patterns]] — 事件管理与响应模式
- [[concepts/chaos-engineering-platforms.md|chaos engineering platforms]] — 混沌工程平台
- [[concepts/k8s-observability-stack.md|k8s observability stack]] — K8S 可观测性技术栈
