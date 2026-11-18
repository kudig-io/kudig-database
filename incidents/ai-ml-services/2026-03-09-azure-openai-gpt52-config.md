---
id: INC-20260309-AZURE-OPENAI
title: Azure OpenAI GPT-5.2 配置与已部署引擎版本不兼容，模型调用跨区失败持续 20 小时 12 分
company: Microsoft Azure
company_type: cloud-native
domain: ai-ml-services
date: 2026-03-09
duration_minutes: 1212
severity: SEV-2
impact_scope: multi-region
root_cause_category: config-error
root_cause_tags: [model-config, version-mismatch, azure-openai, gpt-5.2, multi-region, control-plane]
status: published
last_updated: 2026-10-23
sources:
  - https://learn.microsoft.com/en-us/answers/questions/5815380/we-are-not-able-to-use-gpt-5-2-model-in-azure-in-e
  - https://learn.microsoft.com/en-us/answers/questions/5815711/is-there-a-gpt-5-2-outage-in-us-east-2
  - https://ideatives.com/briefs/cloud-outages/
---

# Azure OpenAI GPT-5.2 配置不匹配（2026-03-09）：新模型配置与老引擎的 20 小时不兼容

## 摘要

2026 年 3 月 9–10 日，Azure OpenAI 的 **GPT-5.2 系列模型**（gpt-5.2 / gpt-5.2-codex）在多区域出现大面积极调用失败：Foundry  playground 报"There was an issue with your request"， Responses API 与 Completions 双双失效，East US 2、West US 3 等区域客户从 3 月 10 日上午（CET 07:00 起）开始受影响。根因是 **GPT-5.2 的发布配置与已部署引擎版本不兼容**。官方 3 月 10 日宣布事件、3 月 21 日确认缓解并进入恢复滚动。客户视角的主影响窗口约 20 小时 12 分，但部分客户的配额与部署状态回滚恢复持续更久。这是"模型即服务"时代配置面事故的样本：**模型配置与推理引擎的兼容性关系，成为与代码依赖同等关键的新型供应链**。

## 影响评估（CRE 视角）

- **影响面**：East US 2、West US 3 等多区域的 GPT-5.2 / gpt-5.2-codex 部署客户；以开发者与企业 AI 应用为主
- **功能维度**：模型推理调用整体失败（非降级）；Responses API、Completions、Foundry playground 均受影响
- **影响时长**：主故障窗口 20h12m（3-09 晚至 3-10）；官方口径 3-10 宣布、3-21 缓解，长尾 11 天的"恢复滚动"说明影响与恢复存在分客户差异
- **次生影响**：大量客户被迫切换备用模型/非 Azure 路径；社区 Q&A 中"429 但实际未超 TPM"的次生限流报告在恢复后仍持续数日
- **对外沟通评估**：一般——微软在社区 Q&A 中确认"ongoing issue"并逐日更新，但未发布正式 RCA 与 tracking ID 的公开技术细节
- **定级依据**：多区域、单一模型族完全不可用 20 小时级；非全平台中断，定为 SEV-2

## 时间线（UTC，部分为社区报告当地时间折算）

| 时间 (UTC) | 事件 | 证据 |
|---|---|---|
| 03-09 晚间 | 主故障窗口开始（官方 20h12m 窗口推算） | E2/E3 |
| 03-10 ~06:00 | 欧洲客户（CET 07:00 起）大面积报告 GPT-5.2 调用失败，多区域同时失效 | E3 |
| 03-10 日间 | 微软在 Q&A 确认 ongoing issue，PG 团队调查 | E2 |
| 03-11 | Playground 部分恢复，但 429 限流报告增多 | E3 |
| 03-17–03-18 | 官方确认产品组已缓解，要求客户复测 | E2 |
| 03-21 | 社区记录：官方宣布 3-10 立案、3-21 缓解，恢复滚动进行中 | E3 |

**关键时间指标**：TTD ≈ 客户报告（社区先发现）/ TTM ≈ 数日 / TTR = 20h12m（主窗口）；长尾至 3-21（第三方记录）

## 技术细节与根因分析（SRE 视角）

### 背景架构

Azure OpenAI 的模型发布链：模型配置（版本、能力声明、API 形态）由平台控制面下发到各区域的推理引擎集群。配置与引擎版本必须严格匹配——配置先行而引擎未升级（或反之）即产生不兼容。

### 因素三分

- **触发因素（Trigger）**：GPT-5.2 配置变更发布
- **根本原因（Root Cause）**：新配置与已部署引擎版本不兼容（官方口径为 config mismatch）
- **扩大因素（Aggravating Factors）**：
  1. 多区域同时发布：一个配置错误同时落在多个区域，无区域级灰度隔离
  2. 模型族绑定：同族所有 SKU（gpt-5.2 / codex）一起失效，客户无同族降级路径
  3. 恢复长尾：配额/部署状态的恢复滚动持续 11 天，部分客户经历"宣布缓解后仍异常"
- **减轻因素**：旧模型（GPT-4.1、GPT-5 前代）不受影响，客户提供降级选择

### 5 Whys

```
现象：多区域 GPT-5.2 调用整体失败 20 小时
Why1 → 请求被推理层拒绝（参数/模式不兼容）
Why2 → 发布的模型配置与已部署引擎版本不匹配
Why3 → 配置发布与引擎升级的编排/校验缺失
Why4 → 发布流水线缺少"配置-引擎兼容性"的门禁检查与自动回滚
Why5 → MaaS 平台把模型配置当作普通配置管理，未升级为"兼容性契约"
        （系统性原因：模型供应链的配置面治理滞后于模型发布速度）
```

## 解决过程

产品组回滚/修正配置并推动引擎-配置对齐；随后进入漫长的恢复滚动，处理遗留的配额与限流状态。官方未公开兼容性校验的整改细节。

## 经验教训

1. 当发布模型配置时，必须**把"配置 × 引擎版本"作为显式兼容性矩阵校验项**并设发布门禁，否则一个配置错误等于全球该模型族下线。
2. 当多区域同时发布模型时，必须**区域级灰度**——配置错误的爆炸半径应与区域数解耦。
3. 当事故进入"恢复滚动"长尾时，必须**向客户给出分账户/分区域的恢复状态查询手段**，否则"宣布缓解"与"客户仍异常"之间的不一致会持续消耗信任。

## 预防与改进措施

- **预防（Prevent）**：配置-引擎兼容性契约与发布门禁（官方未详述，建议）
- **减小爆炸半径（Contain）**：模型配置分区域灰度发布；同族多 SKU 错峰
- **快速检测（Detect）**：发布后合成探针调用（canary inference）按区域巡检
- **快速恢复（Recover）**：配置一键回滚；恢复状态按客户可查

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 模型发布是新型变更管理场景：配置-引擎兼容性矩阵、灰度、回滚三件套必须落地到 MaaS 流水线 |
| CRE | 单模型族 20 小时不可用让客户"为预览模型付费却拿不到 SLA 承诺"的体验问题凸显——preview 模型的期望管理需要更清晰的沟通 |
| FDE | 社区 Q&A 成为事实上的故障数据源；官方缺位时第三方时间线（立案/缓解日期）成为唯一锚点——厂商应主动提供结构化事件 feed |
| SA（客情危机） | 把生产关键工作流押在 preview 模型 + Global Standard 容量的客户在此类事件中无 SLA 兜底，客情受损集中在这一群体 |
| SA（技术危机） | 企业 AI 架构应内置"模型降级阶梯"（同厂旧版 → 跨厂等价模型 → 静态兜底），并定期演练切换 |

## 参考资料

1. [We are not able to use gpt-5.2 model in Azure（Microsoft Q&A，官方人员跟进）](https://learn.microsoft.com/en-us/answers/questions/5815380/we-are-not-able-to-use-gpt-5-2-model-in-azure-in-e) — E2
2. [Is there a gpt 5.2 outage in us east 2?（Microsoft Q&A，含 3-10 立案 / 3-21 缓解记录）](https://learn.microsoft.com/en-us/answers/questions/5815711/is-there-a-gpt-5-2-outage-in-us-east-2) — E2/E3
3. [Cloud Outages 2026 事件日志（第三方汇总，GPT-5.2 config mismatch 条目）](https://ideatives.com/briefs/cloud-outages/) — E3
