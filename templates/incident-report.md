---
# ============ 结构化元数据（供 tools/ 检索与索引，字段枚举见 docs/severity-and-taxonomy.md）============
id: INC-YYYYMMDD-COMPANY          # 全库唯一，格式 INC-<YYYY><MMDD>-<公司大写slug>
title: <公司> <一句话故障标题>
company: <公司名>
company_type: cloud-native        # cloud-native | ai-native | internet
domain: <领域目录名>               # 必须与所在目录一致（含 business-operations / sa-architecture）
date: YYYY-MM-DD                  # 故障开始日期（UTC）
duration_minutes: 0               # 客户感知的主要影响时长（分钟）；未披露填 -1
severity: SEV-1                   # SEV-1 | SEV-2 | SEV-3 | SEV-4
impact_scope: global              # global | multi-region | single-region | partial
root_cause_category: change-management   # 单选主根因，见分类表（含 commercial / architecture-decision）
root_cause_tags: [tag-a, tag-b]   # 2~6 个小写连字符标签
status: draft                     # draft | published | updated
last_updated: YYYY-MM-DD
sources:
  - <官方 Postmortem / 状态页 / 权威来源 URL>
---

# <标题：公司 + 时间 + 一句话概括>

## 摘要

> 3~5 句话：什么系统、什么时间、发生了什么、影响多大、根因一句话、多久恢复。要求读完摘要即可转述该故障。

## 影响评估（CRE 视角）

- **影响面**：受影响用户/客户比例、地域分布（量化优先，未披露则标注）
- **功能维度**：哪些用户旅程受损（读/写/登录/支付/API/...）
- **影响时长**：客户视角不可用窗口；与内部完全恢复时间的差异
- **错误预算**：按典型 SLO（99.9% / 99.99%）估算本次消耗
- **次生影响**：下游服务、生态、社会面影响
- **对外沟通评估**：首次通告时延、通告质量、复盘公开度（优秀/良好/一般/不足）
- **定级依据**：为何定为当前 severity

## 时间线（UTC）

> 每个关键节点标注证据级别（E1~E4，见 FDE 方法论）；未披露的时间段显式说明。

| 时间 (UTC) | 事件 | 证据 |
|---|---|---|
| HH:MM | 变更执行 / 触发事件 | E1 |
| HH:MM | 指标突变 / 首个告警 | E1 |
| HH:MM | 状态页首次通告 | E2 |
| HH:MM | 关键处置动作 | E1 |
| HH:MM | 影响显著缓解 | E1 |
| HH:MM | 完全恢复 | E2 |

**关键时间指标**：TTD ≈ ？ / TTM ≈ ？ / TTR ≈ ？（未披露项标注）

## 技术细节与根因分析（SRE 视角）

### 背景架构

> 理解本故障所需的最小架构上下文。

### 因素三分

- **触发因素（Trigger）**：
- **根本原因（Root Cause）**：
- **扩大因素（Aggravating Factors）**：

### 5 Whys

```
现象：...
Why1 → ...
Why2 → ...
Why3 → ...
Why4 → ...
Why5 → ...（系统性原因）
```

## 解决过程

> 处置动作按时间顺序：尝试了什么、什么有效、什么无效、为什么恢复慢/快。

## 经验教训

> 每条满足"可迁移 + 可操作"，句式：当 <条件> 时，必须/应当 <工程约束>，否则 <风险>。

1. ...
2. ...

## 预防与改进措施

> 按防御纵深分层；官方已宣布的措施标注"官方"，本库补充建议标注"推断/建议"。

- **预防（Prevent）**：
- **减小爆炸半径（Contain）**：
- **快速检测（Detect）**：
- **快速恢复（Recover）**：

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | （系统性缺陷一句话） |
| CRE | （客户影响与沟通表现一句话） |
| FDE | （可观测性缺口/数据教训一句话） |
| SA（客情危机） | （客户/公众感知、舆情与信任冲击一句话） |
| SA（技术危机） | （解决方案架构层面的技术教训一句话） |

## 参考资料

1. [官方复盘](URL) — E1
2. [其他来源](URL) — E2/E3/E4
