---
title: SRE 核心原则
summary: 'SRE 核心原则：用软件工程方法解决运维问题——SLO 驱动、错误预算、消除琐事、渐进式变更与无责复盘，是 K8s 生产运维体系的方法论基座。'
category: concepts
tags:
- sre
- reliability
- slo
- error-budget
- toil
- k8s
tier: core
created: 2026-07-27
last_updated: 2026-07-27
status: stable
---

# SRE 核心原则

> Site Reliability Engineering（站点可靠性工程）源自 Google，核心思想是"用软件工程的方法解决运维问题"。本文提炼其在 Kubernetes 生产环境中的落地原则。

## 1. 七大核心原则

| 原则 | 含义 | K8s 落地形态 |
|------|------|-------------|
| 拥抱风险 | 100% 可靠性不是目标，成本随 9 的个数指数上升 | 按业务分级设定 SLO（如 99.9% vs 99.99%） |
| SLO 驱动 | 以用户可感知的服务指标定义可靠性目标 | SLI 采集（延迟/错误率/饱和度）→ SLO → 告警 |
| 错误预算 | 1 - SLO 即为可消耗的不可靠额度 | 预算耗尽则冻结发布，倒逼质量投入 |
| 消除琐事 | 自动化重复性、无长期价值的手工操作 | Operator、GitOps、自愈控制器 |
| 监控与可观测 | 面向症状告警，面向原因排查 | 指标/日志/链路三支柱 + 告警分级 |
| 渐进式变更 | 小批量、可回滚、可度量的发布 | 金丝雀/蓝绿、Argo Rollouts、PDB |
| 无责复盘 | 聚焦系统性缺陷而非个人过失 | Postmortem 模板 + 行动项闭环跟踪 |

## 2. SLO 与错误预算的运行机制

```
SLI 采集 → SLO 目标 (如 30 天 99.9%)
              │
              ▼
错误预算 = 43.2 分钟/月
              │
    ┌─────────┴─────────┐
    预算充足              预算耗尽
    正常发布节奏          冻结非紧急发布
    可执行混沌实验        专注可靠性改进
```

错误预算把"研发要速度"与"运维要稳定"的对立转化为同一账本下的量化决策，是 SRE 区别于传统运维的关键机制。详见 [[09-可观测性/06-SLO-SLI/02-error-budget-policy|错误预算策略]]。

## 3. 琐事（Toil）判定与治理

琐事的六个特征：手工的、重复的、可自动化的、战术性的、无持久价值的、随规模线性增长的。SRE 实践要求琐事占比不超过工程师时间的 50%，超出即触发自动化投入。治理路径见 [[22-概念/08-可靠性与运维/toil-elimination|琐事消除]]。

## 4. 组织与角色

- **SRE 团队模型**：嵌入式（embedded）、平台式（platform SRE）、咨询式（consulting）三种形态；K8s 平台团队通常采用平台式
- **On-call 纪律**：单次值班事件数有上限（Google 实践为每 12h 班次 ≤2 起），超限说明系统性问题未解决
- **50% 工程时间红线**：保证 SRE 有能力做自动化与架构改进，而非沦为救火队

## 5. 在 KUDIG 知识体系中的位置

- 指标与告警落地：[[09-可观测性/06-SLO-SLI/01-slo-engineering-practice|SLO 工程实践]]
- 事件响应与复盘：[[22-概念/08-可靠性与运维/incident-management-patterns|事件管理与复盘模式]]
- 容量与混沌：[[12-可靠性/03-容量规划/01-capacity-planning-framework|容量规划框架]]、[[12-可靠性/04-混沌工程/01-chaos-engineering-overview|混沌工程总览]]
- MTTR 优化：[[12-可靠性/06-SRE实践/14-mttr-framework-optimization|MTTR 框架优化]]

## 相关阅读

- [[12-可靠性/README|可靠性域总览]]
- [[22-概念/08-可靠性与运维/slo-error-budget-framework|SLO 与错误预算框架]]
- [[12-可靠性/05-事后复盘/01-blameless-postmortem-template|无责复盘模板]]
