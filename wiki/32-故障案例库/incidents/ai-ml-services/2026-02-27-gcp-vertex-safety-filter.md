---
id: INC-20260227-GCP-VERTEXAI
title: Google Cloud Vertex AI 安全过滤服务配置变更致 Gemini 模型 429/503 错误 1 小时 58 分
company: Google Cloud
company_type: cloud-native
domain: ai-ml-services
date: 2026-02-27
duration_minutes: 118
severity: SEV-3
impact_scope: multi-region
root_cause_category: change-management
root_cause_tags: [safety-filter, config-change, gemini, vertex-ai, http-429, http-503]
status: published
last_updated: 2026-10-23
sources:
  - https://ideatives.com/briefs/cloud-outages/
---

# Vertex AI 安全过滤配置变更（2026-02-27）：一道内容安全闸变成模型服务闸门

## 摘要

2026 年 2 月 27 日，Google Cloud 对 **Vertex AI 安全过滤（safety-filtering）服务**的配置变更导致 Gemini 模型请求大面积返回 **429/503 错误**，持续 **1 小时 58 分**。安全过滤作为模型服务的**内联依赖**位于请求关键路径上，其配置错误的表现与模型本身过载完全一致（429），给排障制造了初始迷雾。这是 AI 服务架构特有的故障形态：**控制模型"能不能说话"的组件，其可用性必须与"说话"本身同等级管理**。

## 影响评估（CRE 视角）

- **影响面**：Vertex AI 上调用 Gemini 模型的客户（多区域）
- **功能维度**：模型推理请求失败（429/503）
- **影响时长**：1h58m
- **对外沟通评估**：一般（事件日志级披露）
- **定级依据**：时长 <2h、限流型错误为主、无数据面事故，定为 SEV-3

## 时间线（UTC）

| 时间 (UTC) | 事件 | 证据 |
|---|---|---|
| 02-27 日间 | 安全过滤服务配置变更发布 | E2/E3 |
| 02-27 | Gemini 请求 429/503 激增，持续 1h58m | E2/E3 |
| 02-27 | 变更回滚/修正，服务恢复 | E2 |

**关键时间指标**：TTD ≈ 未披露 / TTM ≈ 未披露 / TTR = 1h58m（第三方汇总口径）

## 技术细节与根因分析（SRE 视角）

### 背景架构

Vertex AI 的模型请求路径经过安全过滤服务（内容审核策略执行点）。该服务位于关键路径：过滤服务异常 → 请求无法完成，即使推理池完全健康。

### 因素三分

- **触发因素（Trigger）**：安全过滤服务配置变更
- **根本原因（Root Cause）**：变更引入的配置错误（具体未披露）
- **扩大因素（Aggravating Factors）**：429 错误形态诱导客户/值班误判为容量问题；内联依赖无旁路（fail-open vs fail-closed 的两难）

### 5 Whys

```
现象：Gemini 请求 429/503 1h58m
Why1 → 请求在安全过滤环节被拒绝
Why2 → 过滤服务配置错误
Why3 → 变更缺少针对错误码语义的发布前验证
Why4 → 安全组件的变更测试未覆盖"其对可用性错误码的影响"
Why5 → 安全依赖被当作普通配置项管理，未纳入模型服务可用性契约
        （系统性原因：安全内联依赖的可靠性等级未对齐主链路）
```

## 解决过程

回滚/修正配置恢复；官方未披露细节。

## 经验教训

1. 当安全组件位于请求关键路径时，必须**为其定义与主链路一致的 SLO**，并在其变更发布时跑同样的可用性探针——429 语义混淆是本案的排障陷阱。
2. 当安全过滤需要 fail-closed 时，必须**为其设计独立的降级容量**（如本地策略缓存），否则安全即单点。

## 预防与改进措施

- **预防（Prevent）**：安全组件变更门禁含错误码语义验证（建议）
- **减小爆炸半径（Contain）**：过滤策略缓存/旁路容量（需与安全合规权衡）
- **快速检测（Detect）**：按依赖环节拆分的错误码归因
- **快速恢复（Recover）**：配置快速回滚

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 关键路径上的每个内联依赖（含安全组件）都是隐式 SLO 责任人 |
| CRE | 错误码语义准确（429=限流 vs 5xx=故障）直接影响客户自救路径的正确性 |
| FDE | 按请求环节拆分的错误归因仪表板可节省分钟级排障时间 |
| SA（客情危机） | 影响较小；但对 AI 合规敏感客户的"安全组件也会引发可用性事故"认知有教育价值 |
| SA（技术危机） | 模型服务架构图必须显式画出安全过滤层及其故障模式 |

## 参考资料

1. [Cloud Outages 2026 事件日志（Vertex AI safety-filter change 条目）](https://ideatives.com/briefs/cloud-outages/) — E3
