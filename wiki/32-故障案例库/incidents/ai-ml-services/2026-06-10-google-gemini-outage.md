---
id: INC-20260610-GOOGLE-GEMINI
title: Google Gemini 全球服务中断（工具部署元数据数据库索引热点致极端读争用，约 7 小时错误率升高）
company: Google
company_type: cloud-native
domain: ai-ml-services
date: 2026-06-10
duration_minutes: 425
severity: SEV-1
impact_scope: global
root_cause_category: software-bug
root_cause_tags: [google, gemini, database, index-design, hotspotting, read-contention, cache-ttl, load-amplification, shard-overload, error-1076, error-1099, ai-service]
status: published
last_updated: 2026-08-21
sources:
  - https://www.google.com/appsstatus/dashboard/incidents/CzZUn98mhTcEiCJo27Kv
  - https://www.ilert.com/postmortems/gemini-database-hotspotting-cache-ttl-outage-jun-2026
  - https://www.thousandeyes.com/blog/google-gemini-outage-analysis-june-10-2026
  - https://www.crn.com/news/cloud/2026/the-10-biggest-cloud-outages-of-2026-so-far
---

# Google Gemini 6·10（2026-06-10）：数据库热点引爆 950M 用户 AI 服务——一个索引设计缺陷让 Gemini 全球瘫痪 7 小时

## 摘要

2026 年 6 月 10 日 03:30 US/Pacific（10:30 UTC），Google Gemini 服务在 **web、移动端、Chrome 集成等全部表面** 出现可用性下降与错误率升高，持续约 **7 小时 5 分钟**，官方 incident report 称约 **15 小时后完全恢复**。用户看到错误码 1076/1099（"Something went wrong"），10,000+ 用户在全球报告问题。根因是：管理**工具部署元数据（tool deployment metadata）** 的基础数据库服务出现**极端读争用**——用于追踪部署过期时间的列因索引设计缺陷聚集了大量相似值，流量"热点化"（hotspotting）集中到少量数据库分片（shards）将其压垮；同时 1 分钟的缓存 TTL 在压力期无法有效吸收请求，形成负载放大。这是继 2025 年 8 月 Anthropic 质量劣化、2025 年 12 月 OpenAI 故障之后，**AI 服务"控制面即故障面"** 系列的又一起案例——**AI 平台的控制面（模型/工具部署元数据）成为新的单点故障**。修复措施包括重构数据库索引、改进缓存策略防止压力期负载放大、加强分片负载分布不均的监控告警。

## 影响评估（CRE 视角）

- **影响面**：Gemini 全表面（web、iOS/Android 应用、Chrome 集成、API）不可用或错误率升高；约 950M 月活用户中受影响者众多；企业客户（Workspace 集成）同样受影响
- **影响时长**：错误率升高约 7 小时 5 分钟（03:20/03:30 起）；完全恢复约 15 小时后
- **次生影响**：依赖 Gemini API/集成的第三方应用连带受损；用户流失信任；正值 Google 与 Apple Siri AI 竞争的敏感期
- **对外沟通评估**：中等——Google Workspace Status Dashboard 发布 incident report 并说明根因（索引问题、热点化、缓存策略），但无代码级细节
- **定级依据**：全球性 AI 服务长时间不可用、950M 用户基数、企业依赖广泛，SEV-1
- 未披露信息：受影响用户比例、API 流量受损比例、具体索引列名

## 时间线（US/Pacific，2026-06-10）

| 时间 | 事件 | 证据 |
|---|---|---|
| 03:20-03:30 | Gemini 全表面开始出现错误率升高 | E1/E3 |
| ~10:10 UTC | ThousandEyes 检测到 Gemini 服务劣化 | E3 |
| 03:30 起 | 用户大量报告错误 1076/1099，社区与媒体关注 | E4 |
| 约 7 小时 | 错误率持续升高，无可用 workaround（官方报告确认） | E1 |
| ~10:25 | 错误率恢复（官方 incident 结束时间口径之一） | E2 |
| 约 15 小时 | 全部服务完全恢复 | E1 |
| 事后 | Google 发布 incident report：根因=工具部署元数据数据库极端读争用，索引设计缺陷致热点化，缓存策略改进中 | E1/E4 |

**关键时间指标**：TTD = 即时 / TTM = 数小时内定位 / TTR = 主影响 7h05m，完全恢复 ~15h

## 技术细节与根因分析（SRE 视角）

### 背景架构

Gemini 的模型/工具部署由**控制面服务**管理：工具部署元数据（部署版本、过期时间等）存储在一个**基础数据库服务**中，前端在响应请求时需要查询该元数据来决定路由到哪个模型版本、启用哪些工具。该数据库采用**分片（sharding）架构**，大量读请求按 key 分布到各分片；缓存层（TTL 约 1 分钟）用于吸收高频重复查询，减轻数据库压力。**索引**用于加速按列（如部署过期时间）的查询。

### 因素三分

- **触发因素（Trigger）**：某个时刻起，针对工具部署元数据库的读请求激增（可能与新模型/工具部署相关）；由于部署过期时间列存在**大量相似值**（大量行在同一时间窗口过期），索引失效，查询退化为扫描并集中命中少量分片。
- **根本原因（Root Cause）**：**索引设计缺陷**——追踪部署过期时间的列包含高基数重复值，导致查询无法利用索引有效分散，流量"热点化"集中到少量数据库分片，将其压垮；分片过载后读争用进一步升级为极端读争用（extreme read contention），形成恶性循环。
- **扩大因素（Aggravating Factors）**：
  1. **1 分钟缓存 TTL**：压力期后端变慢时，缓存快速过期、穿透请求叠加到已过载的数据库，形成**负载放大**（load amplification）——短 TTL 在正常期效果好，在压力期是放大器；
  2. 分片负载分布不均缺少有效监控与告警，热点形成未被及时识别；
  3. Gemini 全表面共享同一控制面，web/App/Chrome/API 同时受损，无备用路径。
- **减轻因素（Mitigating Factor）**：Google 快速识别根因并采取措施（负载疏散/扩容/缓存调整）；错误码 1076/1099 让用户侧能识别服务端问题而非用户操作问题。

### 5 Whys

```
现象：Gemini 全表面错误率升高约 7 小时
Why1 → 工具部署元数据数据库出现极端读争用
Why2 → 流量热点化集中在少量分片，分片被压垮
Why3 → 部署过期时间列存在大量相似值，索引设计无法分散查询
Why4 → 索引设计未考虑该列的高重复值分布特征（索引选择性差）
Why5 → 数据库负载分布不均的监控/告警不足 + 缓存 TTL 策略在压力期放大负载
        （系统性原因：AI 控制面扩展速度超过了底层数据库的容量与索引工程成熟度）
```

## 解决过程

Google 识别到数据库热点后，通过负载疏散与分片调整缓解争用，同时调整缓存行为；完全恢复用了约 15 小时（从开始计算）。修复措施：重构数据库索引（适配高重复值分布）、改进缓存策略防止压力期负载放大、加强分片负载分布不均的监控与告警、改善负载均衡。

## 经验教训

1. **索引设计必须考虑真实数据分布**：选择性差（高重复值）的列上建索引可能完全失效——索引工程要基于真实数据画像（cardinality、分布、热点），而非 schema 直觉。
2. **缓存 TTL 是双刃剑**：1 分钟 TTL 在正常期高效，在压力期放大后端负载——缓存策略必须区分"正常态"与"过载态"，压力期应自动延长 TTL 或降级（fail-open 而非 fail-closed）。
3. **热点不是突发，而是累积**：分片负载分布不均的监控与告警应常态化——热点化通常有先兆（分片延迟上升、队列堆积），提前识别可避免极端读争用。
4. **AI 服务的控制面是新的单点故障**：模型/工具部署元数据、推理路由等控制面服务被所有表面共享——AI 平台的可靠性工程重心应从"推理 GPU 集群"转向"控制面数据库"。
5. **AI 时代的故障归因**：错误码（1076/1099）让用户能区分"我的问题"与"服务端问题"——AI 服务应设计清晰的错误语义，帮助用户与客服快速归因。

## 预防与改进措施

- **预防（Prevent）**：索引设计引入数据分布画像（cardinality/热点分析）；对高重复值列采用复合索引/哈希分片/分区裁剪等适配方案
- **减小爆炸半径（Contain）**：缓存压力期自动延长 TTL；分片热点自动疏散（resharding/热点 key 拆分）；控制面分级（核心/非核心元数据分离）
- **快速检测（Detect）**：分片级延迟/队列/争用监控与告警；负载分布不均指标（热点系数）
- **快速恢复（Recover）**：预置数据库过载响应 Runbook（临时降级、只读模式、缓存强制刷新策略）；备用控制面路径

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 索引选择性是数据库性能的隐形杀手——高重复值列的索引设计必须基于真实数据画像；缓存 TTL 策略必须区分正常态与过载态，压力期短 TTL 是负载放大器 |
| CRE | 950M 用户产品 7 小时不可用证明：AI 平台的控制面（部署元数据）是新的单点故障——依赖 Gemini API 的企业应建立模型供应商冗余与降级预案 |
| FDE | 故障链完整（索引缺陷→热点→读争用→负载放大），但"错误码 1076/1099"提示：用户侧错误语义设计是 AI 服务归因的第一环，证据要追溯到分片级指标 |
| SA（客情危机） | 全球 10,000+ 用户报错、媒体广泛报道，正值与 Apple Siri AI 竞争敏感期——950M 用户产品的长时间故障对品牌信任冲击巨大，错误码统一让用户侧归因清晰 |
| SA（技术危机） | 依赖 AI 供应商的企业应审视"控制面依赖"：模型/工具部署元数据故障无跨区域逃生路径；多模型供应商冗余与本地降级方案应纳入 AI 架构 |

## 参考资料

1. [Google Workspace Status Dashboard - Gemini incident (Google)](https://www.google.com/appsstatus/dashboard/incidents/CzZUn98mhTcEiCJo27Kv) — E1
2. [Gemini: How database hotspotting and a one-minute cache TTL amplified a major outage (ilert)](https://www.ilert.com/postmortems/gemini-database-hotspotting-cache-ttl-outage-jun-2026) — E2
3. [Google Gemini Outage Analysis: June 10, 2026 (ThousandEyes)](https://www.thousandeyes.com/blog/google-gemini-outage-analysis-june-10-2026) — E3
4. [The 10 Biggest Cloud Outages Of 2026 (So Far) (CRN)](https://www.crn.com/news/cloud/2026/the-10-biggest-cloud-outages-of-2026-so-far) — E3
