---
title: 错误预算政策与 Burn Rate Alert
description: 面向阿里云/专有云 K8s 的错误预算政策与 burn rate alert 设计，讲解多窗口告警、消耗速率、快速与慢速燃烧策略。
summary: 面向阿里云/专有云 K8s 的错误预算政策与 burn rate alert 设计，讲解多窗口告警、消耗速率、快速与慢速燃烧策略。
category: observability
tags:
- k8s
- slo
- error-budget
- burn-rate
- alerting
- observability
- sre
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 监控工程师
- 运维工程师
estimated_read_time: 20min
intent_queries:
- 错误预算政策
- burn rate alert
- SLO 多窗口告警
trigger_keywords:
- error budget
- burn rate
- 错误预算
- 消耗速率
- 多窗口告警
prerequisites:
- kubectl-basics
- observability-basics
- prometheus-basics
- slo-basics
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




# 错误预算政策与 Burn Rate Alert

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，讲解错误预算政策的制定与 burn rate alert 的实现。

## 目录

1. [错误预算基础](#错误预算基础)
2. [Burn Rate 概念](#burn-rate-概念)
3. [多窗口告警策略](#多窗口告警策略)
4. [Prometheus 实现](#prometheus-实现)
5. [告警分级](#告警分级)
6. [响应流程](#响应流程)
7. [报告与复盘](#报告与复盘)
8. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. 错误预算基础

### 1.1 错误预算计算

```
错误预算 = (1 - SLO) × 测量窗口
```

示例：
- SLO 99.9%，30 天窗口
- 错误预算 = 0.001 × 30 × 24 × 60 = 43.2 分钟

### 1.2 错误预算政策

| 剩余预算 | 行动 |
|:---|:---|
| > 50% | 正常迭代 |
| 25% - 50% | 高风险变更需额外审批 |
| 10% - 25% | 仅允许修复性发布 |
| < 10% | 发布冻结 |

---

## 2. Burn Rate 概念

### 2.1 什么是 Burn Rate

Burn Rate 表示错误预算的消耗速度：

```
Burn Rate = 当前错误率 / (1 - SLO)
```

| Burn Rate | 含义 |
|:---|:---|
| 1 | 按预算匀速消耗 |
| 2 | 2 倍速消耗，预算将在窗口一半时间耗尽 |
| 10 | 10 倍速消耗，预算将在 1/10 窗口时间耗尽 |

### 2.2 快速燃烧 vs 慢速燃烧

| 类型 | Burn Rate | 窗口 | 场景 |
|:---|---:|---:|:---|
| 快速燃烧 | 14.4 | 1 小时 | 突发性故障 |
| 慢速燃烧 | 2 | 3 天 | 累积性劣化 |

---

## 3. 多窗口告警策略

### 3.1 Google SRE 推荐的多窗口告警

| 告警 | Burn Rate | 短窗口 | 长窗口 | 消耗比例 |
|:---|---:|---:|---:|---:|
| 紧急 | 14.4 | 5m | 1h | 2% |
| 快速 | 6 | 30m | 6h | 5% |
| 慢速 | 2 | 6h | 3d | 10% |

---

## 4. Prometheus 实现

### 4.1 错误预算与 Burn Rate 记录规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: error-budget-rules
  namespace: monitoring
spec:
  groups:
    - name: error.budget
      interval: 60s
      rules:
        - record: slo:error_budget:ratio_30d
          expr: 1 - 0.999
        - record: slo:error_rate:ratio_5m
          expr: |
            sum(rate(http_requests_total{status=~"5.."}[5m]))
            /
            sum(rate(http_requests_total[5m]))
        - record: slo:burn_rate:5m
          expr: |
            slo:error_rate:ratio_5m / slo:error_budget:ratio_30d
```

### 4.2 Burn Rate Alert 规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: burn-rate-alerts
  namespace: monitoring
spec:
  groups:
    - name: burn.rate
      rules:
        - alert: ErrorBudgetBurnRateFast
          expr: |
            (
              sum(rate(http_requests_total{status=~"5.."}[1h]))
              /
              sum(rate(http_requests_total[1h]))
            ) / (1 - 0.999) > 14.4
            and
            (
              sum(rate(http_requests_total{status=~"5.."}[5m]))
              /
              sum(rate(http_requests_total[5m]))
            ) / (1 - 0.999) > 14.4
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "错误预算快速燃烧"
            description: "服务 {{ $labels.service }} 错误预算燃烧速率超过 14.4，预计 1 天内耗尽"
        - alert: ErrorBudgetBurnRateSlow
          expr: |
            (
              sum(rate(http_requests_total{status=~"5.."}[3d]))
              /
              sum(rate(http_requests_total[3d]))
            ) / (1 - 0.999) > 2
            and
            (
              sum(rate(http_requests_total{status=~"5.."}[6h]))
              /
              sum(rate(http_requests_total[6h]))
            ) / (1 - 0.999) > 2
          for: 1h
          labels:
            severity: warning
          annotations:
            summary: "错误预算慢速燃烧"
            description: "服务 {{ $labels.service }} 错误预算燃烧速率超过 2，存在累积性风险"
```

---

## 5. 告警分级

### 5.1 告警响应矩阵

| 告警级别 | Burn Rate | 响应时间 | 通知对象 |
|:---:|:---|:---:|:---|
| Critical | > 14.4 | 5 分钟 | 值班长 + SRE |
| Warning | 2-14.4 | 30 分钟 | SRE |
| Info | < 2 | 次日 | 团队邮件 |

---

## 6. 响应流程

### 6.1 快速燃烧响应

```
收到 Critical 告警
    │
    ▼
确认受影响服务与范围
    │
    ▼
决定是否立即回滚
    │
    ├─ 是 → 执行回滚
    │
    └─ 否 → 定位根因并修复
              │
              ▼
        验证 SLO 恢复
              │
              ▼
        更新事件记录
```

---

## 7. 报告与复盘

### 7.1 错误预算消耗报告

```markdown
# 错误预算报告

## 服务：payment-service
- 时间窗口：2026-06-01 ~ 2026-06-30
- SLO：99.95%
- 错误预算：21.6 分钟
- 已消耗：18 分钟（83%）

## 消耗事件
| 时间 | 事件 | 消耗 | Burn Rate |
|---|---|---:|---:|
| 06-10 | 数据库主从切换 | 8m | 14.4 |
| 06-18 | 缓存击穿 | 6m | 6 |
| 06-25 | 发布引入 bug | 4m | 2 |

## 改进措施
- 增加数据库切换演练
- 优化缓存预热机制
- 强化灰度发布
```

---

## 8. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| 错误预算计算 | 自动化 | PrometheusRule |
| Burn Rate 告警 | 多窗口配置 | Alertmanager |
| 告警分级 | P1/P2 区分 | 告警规则 |
| 响应流程 | 明确 SOP | 值班手册 |
| 月度报告 | 定期输出 | 邮件/文档 |
| 复盘机制 | 每次耗尽后复盘 | 会议记录 |

---

## 错误预算与发布节奏

错误预算直接影响发布节奏。预算充足时可以承担更多创新风险，预算紧张时则应收紧发布窗口。

| 预算状态 | 发布节奏 | 变更风险 |
|:---:|:---|:---:|
| 绿色 | 正常迭代，允许 A/B 测试 | 中 |
| 黄色 | 降低频率，仅必要变更 | 低 |
| 红色 | 暂停发布，仅紧急修复 | 极低 |

### 多服务依赖预算

当服务 A 依赖服务 B 时，A 的可用性 SLO 不应高于 B 的 SLO。可通过乘法计算组合可用性：

```
组合可用性 = SLO_A × SLO_B
```

例如，API 网关 99.99% 依赖后端服务 99.9%，则组合可用性约为 99.89%。

### 预算恢复机制

错误预算通常在自然周期结束后重置。对于重大事故导致的异常消耗，可启动预算恢复计划：

1. 识别并修复根因
2. 通过混沌演练验证改进效果
3. 连续两个周期未再发生同类事件
4. 委员会审批后恢复预算信誉

## 错误预算与团队协作

错误预算政策是 SRE 与开发团队协作的基础。当预算充足时，开发团队可以更积极地发布新功能；当预算紧张时，团队应优先投入稳定性改进。

### 预算使用记录

建议每次事件后记录预算消耗：

| 日期 | 事件 | 消耗预算 | 剩余预算 | 改进项 |
|:---|:---|:---:|:---:|:---|
| 2026-06-01 | 发布导致 5xx 升高 | 5 分钟 | 38 分钟 | 加强灰度分析 |
| 2026-06-15 | 数据库主从切换 | 10 分钟 | 28 分钟 | 优化切换时间 |

### 跨团队沟通

| 预算状态 | 对开发团队 | 对运维团队 |
|:---|:---|:---|
| 绿色 | 可正常排期发布 | 保持监控 |
| 黄色 | 暂缓非紧急功能 | 加强值班与 review |
| 红色 | 暂停新功能，投入稳定性 | 协助定位慢性问题 |

## 典型工单场景与处理

**场景**：错误预算一周内从 80% 降至 10%。

处理步骤：
1. 分析告警与事件，定位主要消耗来源。
2. 召开紧急评审，决定是否暂停发布。
3. 制定修复计划并设定每日跟踪。
4. 修复后评估效果并更新预防措施。

## 错误预算与发布计划

错误预算应成为发布计划的重要输入。在制定月度发布计划时，需先评估剩余预算：

| 剩余预算 | 发布策略 |
|:---:|:---|
| > 70% | 可安排常规发布与高风险实验 |
| 30% - 70% | 可安排常规发布，谨慎安排实验 |
| < 30% | 仅允许紧急修复，暂停新功能 |
| 0% | 暂停所有非修复类发布 |

### 错误预算告警响应

| 告警 | 响应人 | 动作 |
|:---|:---|:---|
| 快速 burn rate | 值班 SRE | 立即排查并准备回滚 |
| 中速 burn rate | 值班 SRE + 开发 | 1 小时内定位根因 |
| 慢速 burn rate | SRE 负责人 | 纳入周会跟踪 |

### 错误预算报告

每周向团队发送错误预算报告，包含：

- 本周错误预算消耗量
- 主要消耗事件
- 剩余预算与状态
- 建议的发布策略

## 错误预算与 SLO 调整

当业务或技术能力发生变化时，需重新评估 SLO 与错误预算：

| 情况 | 建议 |
|:---|:---|
| 连续多月预算充足 | 可适当收紧 SLO，提升服务质量 |
| 连续多月预算耗尽 | 放宽 SLO 或加大稳定性投入 |
| 业务增长导致峰值异常 | 增加峰值窗口的 SLO 豁免 |
| 重大架构升级 | 设立临时 SLO，升级完成后再恢复 |

### 预算耗尽后的沟通

错误预算耗尽后，应向管理层与业务方同步：

1. 预算消耗的主要原因与事件。
2. 当前服务状态与剩余风险。
3. 已采取与计划采取的改进措施。
4. 对发布计划的影响与调整建议。

## Related

- [[domain-06-observability/SLO-SLI/18-slo-sli-system.md|SLO/SLI体系建设与管理]]
- [[domain-06-observability/SLO-SLI/01-slo-engineering-practice.md|SLO 工程实践]]

## See Also

- [[domain-09-reliability-engineering/SLO-SLI/03-error-budget-management.md|错误预算管理]]
- [[domain-06-observability/告警/05-alerting-management.md|告警管理策略]]


<!-- risk-assessed -->
