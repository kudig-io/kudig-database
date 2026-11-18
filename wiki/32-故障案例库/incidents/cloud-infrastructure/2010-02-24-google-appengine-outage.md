---
id: INC-20100224-GAE-OUTAGE
title: Google App Engine 全平台中断（数据中心维护操作失误致 App Engine 控制面不可用约 2 小时）
company: Google
company_type: cloud-native
domain: cloud-infrastructure
date: 2010-02-24
duration_minutes: 120
severity: SEV-1
impact_scope: global
root_cause_category: operational-safeguard
root_cause_tags: [app-engine, datacenter-maintenance, power-issue, operational-mistake, paas, control-plane, google-cloud]
status: published
last_updated: 2026-07-29
sources:
  - https://www.availabilitydigest.com/public_articles/0504/google_power_out.pdf
  - https://groups.google.com/g/google-appengine/c/p2QKJ0OSLc8
---

# Google App Engine 故障（2010-02-24）：数据中心维护中的一个小失误，让整个 PaaS 平台瘫痪

## 摘要

2010 年 2 月 24 日，Google App Engine 经历了自 2008 年发布以来最严重的中断。Google 后续发布的事后复盘（Post-mortem）详细记录了事件经过：当天上午，Google 一处数据中心进行**计划性维护操作**，但维护过程中的一个**操作失误**导致 App Engine 的**控制面（控制面板与 API）不可用**约 2 小时，部分应用的读写操作也受影响。Google 公布的详细时间线显示，故障从上午 10:15 PT 开始，经过定位、修复、验证，到 12:30 PT 逐步恢复。这是 Google App Engine 作为**最早 PaaS 平台之一**的早期成长阵痛，也是云计算平台在"运维操作失误"类别中的经典案例。

## 影响评估（CRE 视角）

- **影响面**：App Engine 控制面板与 API 不可用约 2 小时，部分应用读写受影响；开发者和企业无法部署/管理应用
- **影响时长**：约 10:15-12:30 PT，约 2 小时 15 分钟
- **次生影响**：App Engine 是 2010 年最流行的 PaaS 平台之一，中断影响大量早期云原生开发者
- **对外沟通评估**：优秀——Google 发布详细事后复盘，含精确时间线与根因分析，是早期云计算中透明度最高的复盘之一
- **定级依据**：PaaS 平台级中断，影响所有租户约 2 小时，SEV-1
- 未披露信息：数据中心维护操作的具体类型、受影响应用数量

## 时间线（太平洋时间，2010-02-24）

| 时间 | 事件 | 证据 |
|---|---|---|
| 10:15 | 数据中心计划性维护操作中发生失误，App Engine 控制面开始不可用 | E1 |
| 10:15-10:45 | 工程师定位到为数据中心维护操作引发的控制面问题 | E1 |
| 10:45-12:00 | 修复中，部分应用读写受影响 | E1 |
| 12:00-12:30 | 逐步恢复，验证服务正常 | E1 |
| 02-24 之后 | Google 发布详细事后复盘，含时间线与根因 | E1 |

**关键时间指标**：TTD = 即时 / TTM ≈ 30min / TTR ≈ 2h15min

## 技术细节与根因分析（SRE 视角）

### 背景架构

2010 年的 Google App Engine 构建在 Google 的基础设施之上，提供 PaaS 服务。控制面负责应用部署、管理、监控等操作。数据中心维护操作涉及物理基础设施变更（供电/网络/制冷等）。

### 因素三分

- **触发因素（Trigger）**：数据中心计划性维护操作中的操作失误。
- **根本原因（Root Cause）**：维护操作未充分评估对上层 App Engine 控制面的影响，操作失误导致控制面不可用。
- **扩大因素（Aggravating Factors）**：控制面故障影响所有租户的管理操作与部分数据读写。
- **减轻因素（Mitigating Factor）**：应用本身的数据面未完全中断，已运行的应用继续处理请求。

### 5 Whys

```
现象：App Engine 控制面不可用约 2 小时
Why1 → 数据中心维护操作中的失误影响 App Engine 控制面
Why2 → 维护操作未充分评估对上层 PaaS 服务的影响
Why3 → 缺乏"维护操作→PaaS 控制面"影响映射
Why4 → 物理基础设施维护与 PaaS 控制面之间的依赖关系文档不足
Why5 → 早期 PaaS 平台尚未建立"底层操作→上层影响"的
        风险评估体系
        （系统性原因：PaaS 平台运维流程的成熟度不足）
```

## 解决过程

工程师定位到根因后，修复了维护操作导致的问题并逐步恢复服务。Google 在事后复盘中详细列出了改进措施，包括加强维护操作的风险评估流程、建立基础设施维护对 PaaS 服务影响的检查清单。

## 经验教训

1. **PaaS 控制面是"管理入口"也是"故障入口"**：控制面的故障不仅影响部署操作，还可能影响部分数据读写——控制面与数据面的隔离需要更严格。
2. **底层基础设施维护是 PaaS 的隐形风险**：数据中心的一台断电、一根光纤、一个维护操作，都可能通过复杂的依赖链击垮 PaaS 控制面。
3. **透明复盘是早期云平台的信任基石**：Google 2010 年发布详细复盘的行为，在当时并不常见——这为后续云厂商的透明沟通树立了标杆。
4. **"操作失误"是最难预防的故障类别**：流程再完善，操作员仍可能犯错——自动化与人工审核的"双保险"是必需品。

## 预防与改进措施

- **预防（Prevent）**：维护操作的 PaaS 影响预评估流程；自动化操作减少人工失误
- **减小爆炸半径（Contain）**：控制面与数据面严格隔离；控制面跨机房冗余
- **快速检测（Detect）**：PaaS 控制面健康状态实时监控
- **快速恢复（Recover）**：控制面故障的快速切换机制

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 底层维护操作对上层服务的影响不能只靠"相信脚本"——需要建立从物理层到应用层的依赖映射与变更影响评估 |
| CRE | PaaS 客户依赖平台进行应用管理——控制面的故障使客户不仅无法管理，还可能影响运行中的应用 |
| FDE | 操作失误的取证依赖操作日志与系统变更记录——Google 2010 年已具备完整的日志体系，使复盘时间线精确到分钟 |
| SA（客情危机） | App Engine 发布以来最严重中断约 2 小时，开发者平台信任受损；Google 发布详细公开复盘，在当时成为行业透明标杆，有效缓和开发者情绪 |
| SA（技术危机） | PaaS 控制面=故障入口——客户应了解平台维护窗口与底层依赖链；数据面与控制面的隔离程度决定了故障波及范围，关键应用应具备降级运行能力 |

## 参考资料

1. [Post-mortem for February 24th, 2010 outage (Google Groups)](https://groups.google.com/g/google-appengine/c/p2QKJ0OSLc8) — E1
2. [Poor Documentation Snags Google (Availability Digest)](https://www.availabilitydigest.com/public_articles/0504/google_power_out.pdf) — E3