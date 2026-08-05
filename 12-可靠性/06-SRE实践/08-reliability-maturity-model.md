---
title: 可靠性工程成熟度模型
description: 可靠性工程成熟度五级模型，用于评估组织在 SLO、混沌工程、事件管理等维度的成熟度
summary: 五级成熟度（无意识→被动→主动→量化→优化）+ 六维度自评 + 升级路径
category: reliability
tags:
- slo
- sli
- reliability
- maturity-model
- assessment
- governance
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 架构师
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 可靠性工程成熟度模型

> **核心原则**：成熟度模型不是"评分攀比工具"，而是**自诊短板、规划下一步投入的地图**。一个组织不必所有维度都冲到 5 级——关键是识别当前最痛的那个维度，针对性升级。盲目追求全面 5 级会浪费资源在最不痛的维度上。

## 五级成熟度总览

```
L1 无意识 ──▶ L2 被动 ──▶ L3 主动 ──▶ L4 量化 ──▶ L5 优化
  出事才救     有 runbook    预防为主    SLO 驱动    自适应优化
  全靠英雄     能恢复        有监控      预算约束    持续演进
```

| 级别 | 典型特征 | 可用性 |
|------|---------|--------|
| **L1 无意识** | 无监控、出事靠人记、无复盘 | 不稳定 |
| **L2 被动** | 有基础监控、有 runbook、事后救火 | 95–99% |
| **L3 主动** | 预防性监控、容量规划、混沌实验起步 | 99–99.5% |
| **L4 量化** | SLO 驱动、错误预算约束、自动化门控 | 99.5–99.9% |
| **L5 优化** | 全自动化韧性、自适应 SLO、可靠性内生于设计 | 99.9%+ |

## 六维度自评

### 维度 1：SLO/SLI 实践

| 级别 | 特征 |
|------|------|
| L1 | 没有 SLO，用"感觉"判断服务好坏 |
| L2 | 有 SLO 但靠人记，未约束行为 |
| L3 | SLO 有仪表盘，定期评审 |
| L4 | SLO 驱动发布决策，错误预算自动化执行 |
| L5 | SLO 自适应调整，按用户旅程多维度细化 |

### 维度 2：监控与可观测性

| 级别 | 特征 |
|------|------|
| L1 | 只看 CPU/内存，出事才查 |
| L2 | 有基础指标面板，RED/USE 起步 |
| L3 | 完整 RED+USE+日志+追踪，有告警 |
| L4 | SLO 仪表盘 + 错误预算燃烧率 + 多窗口告警 |
| L5 | 自服务观测平台，每个团队自助建面板 |

### 维度 3：事件管理

| 级别 | 特征 |
|------|------|
| L1 | 无流程，谁在线谁处理 |
| L2 | 有 on-call 轮值，无分级 |
| L3 | 有 Sev 分级 + 事件指挥（IC）角色 |
| L4 | 标准化 IC 手册 + 通讯节奏 + 自动化升级 |
| L5 | 事件复盘闭环、改进项跟踪、知识沉淀到系统 |

见 [[12-可靠性/06-SRE实践/10-incident-command-field-guide.md]]。

### 维度 4：混沌工程

| 级别 | 特征 |
|------|------|
| L1 | 不做 |
| L2 | Staging 偶尔手动跑 |
| L3 | 定期 Game Day，有 runbook |
| L4 | 混沌实验在 CI 自动化、阻断发布 |
| L5 | 混沌实验自适应、爆炸半径自动调优 |

见 [[12-可靠性/04-混沌工程/05-chaos-experiment-automation.md]]。

### 维度 5：容量与扩缩容

| 级别 | 特征 |
|------|------|
| L1 | 手动加机器 |
| L2 | 有 HPA，靠经验设阈值 |
| L3 | HPA + VPA + CA/Karpenter，有容量规划 |
| L4 | 数据驱动右调优、预测性扩容、成本优化 |
| L5 | 自适应扩缩容、多区域弹性、FinOps 闭环 |

见 [[12-可靠性/03-容量规划/04-autoscaling-best-practices.md]]。

### 维度 6：灾备

| 级别 | 特征 |
|------|------|
| L1 | 有备份，没验证过恢复 |
| L2 | 定期备份 + 恢复演练 |
| L3 | 多区域灾备 + 季度切换演练 |
| L4 | 灾备自动化、RTO/RPO 量化承诺 |
| L5 | 灾备自愈、故障注入常态化验证 |

见 [[12-可靠性/02-灾难恢复/03-dr-automation-playbook.md]]。

## 自评打分表

```
组织/团队: __________  日期: 2026-07-11

维度                  当前级  目标级  差距  优先级
─────────────────────────────────────────────
SLO/SLI 实践           L?     L?     ___   □高□中□低
监控与可观测性          L?     L?     ___   □高□中□低
事件管理               L?     L?     ___   □高□中□低
混沌工程               L?     L?     ___   □高□中□低
容量与扩缩容           L?     L?     ___   □高□中□低
灾备                  L?     L?     ___   □高□中□低
```

**升级优先级排序原则**：优先升级"差距大 + 影响核心服务 + 投入产出比高"的维度，而非全面铺开。

## 升级路径建议（典型）

```
L1 → L2 (1–3 个月)
  重点：建监控、写 runbook、设 on-call
  投入：1 名 SRE + 基础工具

L2 → L3 (3–6 个月)
  重点：容量规划、Sev 分级、首个 Game Day
  投入：SRE 团队 + 产品配合

L3 → L4 (6–12 个月)
  重点：SLO 落地、错误预算、CI 混沌门控
  投入：全组织对齐，CTO 背书

L4 → L5 (12+ 个月)
  重点：自适应、平台化、可靠性内生于设计
  投入：持续演进，无终点
```

## 常见陷阱

1. **跨级跳跃**：从 L2 直接追 L4，跳过基础监控和 runbook → SLO 无数据支撑。逐级走。
2. **维度失衡**：混沌工程冲到 L5，但事件管理还 L2 → 实验出事处理不了。各维度差距应 < 2 级。
3. **评分自欺**：自己打 L4 但实际没人遵守错误预算。自评必须配合客观证据（如"冻结是否真的被执行"）。
4. **成熟度 = 目标**：成熟度是手段不是目的。最终衡量标准是“用户感知的可靠性”，不是等级数字。

## 评估问卷

### SLO/SLI 实践评估

```markdown
## SLO/SLI 实践评估问卷

### L1 无意识
- [ ] 没有定义任何 SLO
- [ ] 用“感觉”判断服务好坏
- [ ] 没有 SLI 指标收集

### L2 被动
- [ ] 定义了 SLO 但未文档化
- [ ] SLO 未约束发布决策
- [ ] 偶尔查看 SLO 仪表盘

### L3 主动
- [ ] SLO 有正式文档和仪表盘
- [ ] 定期评审 SLO (月度)
- [ ] SLO 与告警关联

### L4 量化
- [ ] SLO 驱动发布决策
- [ ] 错误预算自动化执行
- [ ] 多窗口燃烧率告警

### L5 优化
- [ ] SLO 自适应调整
- [ ] 按用户旅程多维度细化
- [ ] SLO 即代码 (SLO-as-Code)
```

### 事件管理评估

```markdown
## 事件管理评估问卷

### L1 无意识
- [ ] 无正式事件响应流程
- [ ] 谁在线谁处理
- [ ] 无事件记录

### L2 被动
- [ ] 有 on-call 轮值
- [ ] 无 Sev 分级
- [ ] 事后补写记录

### L3 主动
- [ ] 有 Sev 分级标准
- [ ] 有 IC 角色
- [ ] 有事件时间线记录

### L4 量化
- [ ] 标准化 IC 手册
- [ ] 通讯节奏规范
- [ ] 自动化升级机制

### L5 优化
- [ ] 复盘闭环跟踪
- [ ] 改进项自动跟踪
- [ ] 知识沉淀到系统
```

## 改进计划模板

### 季度改进计划

```markdown
# 可靠性改进计划 Q3 2026

## 当前状态

| 维度 | 当前级 | 目标级 | 差距 |
|-----|-------|-------|------|
| SLO/SLI | L2 | L4 | 2 |
| 监控 | L3 | L4 | 1 |
| 事件管理 | L2 | L3 | 1 |
| 混沌工程 | L1 | L3 | 2 |
| 容量规划 | L2 | L3 | 1 |
| 灾备 | L2 | L3 | 1 |

## 优先级排序

1. **SLO/SLI** (差距 2，影响核心服务)
2. **混沌工程** (差距 2，预防性投入)
3. **事件管理** (差距 1，快速见效)

## 行动计划

### SLO/SLI (L2 → L4)

| 任务 | 负责人 | 截止日期 | 状态 |
|-----|-------|---------|------|
| 定义核心服务 SLO | @sre-team | 7/15 | ☐ |
| 部署 SLO 仪表盘 | @sre-team | 7/31 | ☐ |
| 配置错误预算告警 | @sre-team | 8/15 | ☐ |
| 发布门控集成 | @platform | 8/31 | ☐ |

### 混沌工程 (L1 → L3)

| 任务 | 负责人 | 截止日期 | 状态 |
|-----|-------|---------|------|
| 部署 Chaos Mesh | @sre-team | 7/15 | ☐ |
| 设计首个实验 | @sre-team | 7/31 | ☐ |
| 首次 Game Day | @sre-team | 8/15 | ☐ |
| 建立实验库 | @sre-team | 8/31 | ☐ |

## 资源需求

- 1 名 SRE 全职投入
- 工具预算: ¥50,000
- 培训预算: ¥20,000

## 成功指标

- SLO 覆盖率: 100% 核心服务
- MTTR: 从 2h 降至 30min
- 混沌实验: 每月至少 1 次
```

## 成熟度指标收集

### 自动化指标收集脚本

```bash
#!/bin/bash
# 🟢 低风险：收集成熟度指标
set -euo pipefail

OUTPUT_FILE="/tmp/maturity-metrics-$(date +%Y%m%d).json"

echo "=== 收集成熟度指标 ==="

# SLO 覆盖率
TOTAL_SERVICES=$(kubectl get deploy -A --no-headers | wc -l)
SLO_SERVICES=$(kubectl get slo -A --no-headers 2>/dev/null | wc -l)
SLO_COVERAGE=$(echo "scale=2; $SLO_SERVICES / $TOTAL_SERVICES * 100" | bc)

# 监控覆盖率
MONITORED_SERVICES=$(kubectl get servicemonitor -A --no-headers 2>/dev/null | wc -l)
MONITOR_COVERAGE=$(echo "scale=2; $MONITORED_SERVICES / $TOTAL_SERVICES * 100" | bc)

# 混沌实验频率
CHAOS_EXPERIMENTS=$(kubectl get podchaos,networkchaos -A --no-headers 2>/dev/null | wc -l)

# 事件响应指标
MTTR=$(query-prometheus 'avg(incident_resolution_time_seconds) / 60')

# 灾备演练
LAST_DR_DRILL=$(kubectl get cronworkflow dr-drill -n dr -o jsonpath='{.status.lastScheduleTime}' 2>/dev/null || echo "never")

cat > $OUTPUT_FILE <<EOF
{
  "timestamp": "$(date -Iseconds)",
  "metrics": {
    "slo_coverage_percent": $SLO_COVERAGE,
    "monitor_coverage_percent": $MONITOR_COVERAGE,
    "chaos_experiments_count": $CHAOS_EXPERIMENTS,
    "mttr_minutes": $MTTR,
    "last_dr_drill": "$LAST_DR_DRILL"
  },
  "maturity_estimate": {
    "slo_sli": "L$([ $(echo "$SLO_COVERAGE > 80" | bc) -eq 1 ] && echo 4 || echo 2)",
    "monitoring": "L$([ $(echo "$MONITOR_COVERAGE > 80" | bc) -eq 1 ] && echo 4 || echo 3)",
    "chaos_engineering": "L$([ $CHAOS_EXPERIMENTS -gt 0 ] && echo 3 || echo 1)"
  }
}
EOF

echo "指标已收集: $OUTPUT_FILE"
cat $OUTPUT_FILE
```

### PrometheusRule 成熟度指标

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: maturity-metrics
  namespace: monitoring
spec:
  groups:
    - name: maturity.rules
      rules:
        # SLO 覆盖率
        - record: maturity:slo_coverage:ratio
          expr: |
            count(slo_target{}) / count(up{job=~".*-api"})

        # 监控覆盖率
        - record: maturity:monitor_coverage:ratio
          expr: |
            count(servicemonitor{}) / count(up{job=~".*-api"})

        # 混沌实验频率
        - record: maturity:chaos_frequency:monthly
          expr: |
            count(increase(chaos_experiment_total[30d]))

        # MTTR
        - record: maturity:mttr:minutes
          expr: |
            avg(incident_resolution_time_seconds) / 60
```

## 组织变革管理

### 变革阻力应对

| 阻力类型 | 表现 | 应对策略 |
|---------|------|----------|
| 认知阻力 | “我们不需要 SLO” | 用事故数据说话，展示 MTTR 与收入损失 |
| 资源阻力 | “没人没时间” | 从小处着手，证明 ROI 后再扩大 |
| 技术阻力 | “工具太复杂” | 提供模板和培训，降低门槛 |
| 文化阻力 | “出错就追责” | 建立无责复盘文化，强调学习而非惩罚 |

### 沟通计划

```markdown
## 可靠性改进沟通计划

### 第 1 周: 启动会
- 受众: 全体工程团队
- 内容: 为什么做、目标、路线图
- 形式: 全员大会 + Q&A

### 第 2-4 周: 培训
- 受众: SRE + 核心开发
- 内容: SLO 定义、监控配置、事件响应
- 形式: 工作坊 + 实操

### 第 5-8 周: 试点
- 受众: 1-2 个试点团队
- 内容: 实际落地 SLO、混沌实验
- 形式: 一对一辅导

### 第 9-12 周: 推广
- 受众: 全体团队
- 内容: 试点经验分享、全面推广
- 形式: 分享会 + 文档
```

## 案例研究

### 案例 1: 电商平台从 L2 到 L4

**背景**: 某电商平台，年 GMV 100亿，大促期间频繁宕机。

**初始状态 (L2)**:
- 有基础监控，但无 SLO
- 事件响应靠英雄，无标准流程
- 无混沌实验，问题靠生产发现

**改进措施**:
1. 定义核心交易链路 SLO (99.95%)
2. 建立错误预算机制，超预算冻结发布
3. 每月 Game Day，模拟大促流量
4. 自动化事件响应，MTTR 从 2h 降至 15min

**结果 (L4)**:
- 可用性从 99.5% 提升至 99.95%
- 大促零宕机
- 发布频率从每周 1 次提升至每日多次

### 案例 2: SaaS 公司混沌工程落地

**背景**: 某 B2B SaaS，客户对可用性要求极高 (SLA 99.9%)。

**初始状态 (L1)**:
- 无混沌实验
- 故障靠客户报告发现
- 恢复时间不可预测

**改进措施**:
1. 部署 Chaos Mesh，从 Staging 开始
2. 每周自动化实验 (Pod Kill、网络延迟)
3. 实验失败阻断发布
4. 季度 Game Day，全员参与

**结果 (L3)**:
- 主动发现并修复 20+ 潜在问题
- MTTR 从 1h 降至 10min
- 客户投诉减少 80%

## 相关

- [[12-可靠性/06-SRE实践/05-error-budget-policy-template.md|04 error budget policy template]]
- [[12-可靠性/06-SRE实践/04-slo-sli-guide.md|03 slo sli guide]]
- [[12-可靠性/00-总览/01-production-readiness-operations-guide.md|99 production readiness operations guide]]
- [[12-可靠性/index.md|可靠性 index]]

<!-- risk-assessed -->
