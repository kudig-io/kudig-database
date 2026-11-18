---
id: INC-20260122-CLOUDFLARE-MIAMI
title: Cloudflare 迈阿密路由器自动路由策略配置错误致 BGP 前缀意外泄漏
company: Cloudflare
company_type: cloud-native
domain: networking-dns
date: 2026-01-22
duration_minutes: -1
severity: SEV-3
impact_scope: partial
root_cause_category: config-error
root_cause_tags: [bgp, route-leak, miami, automated-routing-policy, prefix-announcement]
status: published
last_updated: 2026-10-23
sources:
  - https://blog.cloudflare.com/tag/post-mortem/
---

# Cloudflare 迈阿密路由泄漏（2026-01-22）：自动化路由策略的误报文

## 摘要

2026 年 1 月 22 日，Cloudflare **迈阿密数据中心**一台路由器的**自动路由策略（automated routing policy）配置错误**，导致部分 BGP 前缀被**意外向上游泄漏**。Cloudflare 于 1 月 23 日发布复盘，说明影响并宣布整改。这是 Cloudflare 2026 年初 BGP 系列事件的第一起（其后 2 月 20 日发生更大规模的 BYOIP 前缀撤销，见库内 INC-20260220-CLOUDFLARE），根因形态不同：本案是"多发了前缀"，BYOIP 案是"撤回了前缀"——BGP 配置面的两类基本错误在一个月内各上演一次。

## 影响评估（CRE 视角）

- **影响面**：部分 BGP 前缀；第三方网络收到非预期路由通告
- **功能维度**：路由泄漏的常规风险（流量绕行、黑洞、上游过滤压力）；官方复盘确认影响并致歉，具体规模见官方原文
- **影响时长**：未披露统一口径，记 -1
- **对外沟通评估**：优秀——次日发布专项复盘
- **定级依据**：局部路由泄漏、无广泛服务中断证据，定为 SEV-3

## 时间线（UTC）

| 时间 (UTC) | 事件 | 证据 |
|---|---|---|
| 01-22 | 迈阿密路由器自动路由策略配置错误，前缀泄漏 | E1 |
| 01-22 | 泄漏被处置 | E1 |
| 01-23 | 官方复盘发布 | E1 |

**关键时间指标**：TTD ≈ 未披露 / TTM ≈ 未披露 / TTR ≈ 未披露

## 技术细节与根因分析（SRE 视角）

### 背景架构

Cloudflare 全球 anycast 网络通过自动化路由策略管理成千上万的前缀通告。策略生成/下发的自动化一旦出错，错误会以 BGP 报文的形式即时扩散至对等方与上游。

### 因素三分

- **触发因素（Trigger）**：自动路由策略配置错误
- **根本原因（Root Cause）**：策略自动化生成/校验缺陷（官方归因于 automated routing policy configuration error）
- **扩大因素（Aggravating Factors）**：BGP 错误通告的传播不依赖 Cloudflare 自身修复——依赖对等方/上游的过滤与撤回；自动化策略数量大、人工稽核困难

### 5 Whys

```
现象：BGP 前缀意外泄漏
Why1 → 路由器发出了非预期的前缀通告
Why2 → 自动路由策略配置错误
Why3 → 策略自动化缺少发布前的通告内容校验
Why4 → 策略系统以"意图"为输入，未以"实际通告集合"为验收
Why5 → 路由自动化按控制面逻辑验收，未按数据面事实验收
        （系统性原因：自动化的验收对象错位）
```

## 解决过程

撤回错误通告并修复策略；官方复盘含整改措施。

## 经验教训

1. 当路由策略自动化时，验收对象必须是**实际发出的通告集合**（对 peer 可见的事实），而非策略意图。
2. 当 BGP 错误发生时，**自有修复与外部传播衰减并行**——与上游/peers 的应急协同流程应常备。

## 预防与改进措施

- **预防（Prevent）**：通告集合发布前自动 diff 校验（官方有整改，详见原文）
- **减小爆炸半径（Contain）**：分区域策略灰度
- **快速检测（Detect）**：对外部 BGP 视角的持续监控（looking glass/RouteViews 类）
- **快速恢复（Recover）**：通告快速撤回 SOP

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 自动化验收要落在"外部可观测事实"（实际通告）而非内部意图 |
| CRE | 路由泄漏的客户感知间接，但会体现为延迟/绕行；复盘公开度是信任修复关键 |
| FDE | 第三方 BGP 视角监控是独立于自有系统的真相源 |
| SA（客情危机） | 与 2/20 BYOIP 事件叠加，Cloudflare 2026 年初 BGP 治理进入客户视野 |
| SA（技术危机） | 使用 BYOIP/ anycast 服务的客户应将提供商 BGP 事件历史纳入供应商风险评估 |

## 参考资料

1. [Cloudflare Blog：Route leak incident on January 22, 2026（官方复盘，2026-01-23）](https://blog.cloudflare.com/) — E1（发布于官方博客 Post Mortem 栏目）
