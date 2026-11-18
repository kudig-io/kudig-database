---
id: INC-20260529-AZURE-OPENAI-ROUTING
title: Azure OpenAI 上游内部工作负载重试风暴压垮路由层，服务降级 7 小时 26 分
company: Microsoft Azure
company_type: cloud-native
domain: ai-ml-services
date: 2026-05-29
duration_minutes: 446
severity: SEV-2
impact_scope: multi-region
root_cause_category: capacity-overload
root_cause_tags: [retry-storm, routing-layer, azure-openai, internal-workload, throttling, cascading-failure]
status: published
last_updated: 2026-10-23
sources:
  - https://azure.status.microsoft/en-us/status/history/
  - https://ideatives.com/briefs/cloud-outages/
---

# Azure OpenAI 路由层重试风暴（2026-05-29）：自家内部工作负载把路由层打挂了 7 个多小时

## 摘要

2026 年 5 月 29 日（与 West US 2 雷暴断电同一天），Azure OpenAI 发生一起独立的**内部重试风暴**事故：一个上游内部工作负载的**重试流量**淹没了路由层，服务降级持续 **7 小时 26 分**。这是"重试即负载"教科书的又一页：故障源不是外部客户洪峰，而是平台自己的内部组件在无退避重试。它与当日的 West US 2 电力事故互不相关，但时间上重叠，构成 Azure 2026 年"黑色同日"——客户在同一天同时面对区域电力中断与 AI 服务降级。

## 影响评估（CRE 视角）

- **影响面**：Azure OpenAI 服务的路由层，影响面按第三方汇总为多区域降级
- **功能维度**：模型请求延迟升高、间歇失败、限流加剧
- **影响时长**：7h26m
- **次生影响**：与同日 West US 2 电力事件叠加，客户难以区分根因，排障与沟通成本上升
- **对外沟通评估**：一般
- **定级依据**：多区域 AI 核心服务降级 7 小时级，定为 SEV-2

## 时间线（UTC）

| 时间 (UTC) | 事件 | 证据 |
|---|---|---|
| 05-29 日间 | 上游内部工作负载重试流量激增，路由层过载 | E2/E3 |
| 05-29 全天 | 客户观测到延迟与失败率升高 | E2 |
| 05-29 晚间 | 重试收敛，路由层恢复，共 7h26m | E2 |

**关键时间指标**：TTD ≈ 未披露 / TTM ≈ 未披露 / TTR = 7h26m（第三方汇总口径）

## 技术细节与根因分析（SRE 视角）

### 背景架构

Azure OpenAI 请求路径：客户端 → 路由层（分发/限流）→ 后端推理池。上游内部工作负载（如平台自身的代理、编排或批处理组件）与客户流量共享路由层。

### 因素三分

- **触发因素（Trigger）**：上游内部工作负载产生异常重试流量
- **根本原因（Root Cause）**：内部工作负载缺少有界退避重试，故障时重试放大为流量洪峰
- **扩大因素（Aggravating Factors）**：内部流量与客户流量共享路由层容量；路由层对"内部来源"可能缺少与客户流量同等的限流隔离
- **减轻因素**：降级而非完全中断

### 5 Whys

```
现象：路由层过载 7h26m
Why1 → 请求量超出路由层容量
Why2 → 内部工作负载持续重试放大流量
Why3 → 内部组件重试无界/无退避
Why4 → 内部流量未纳入与外部流量同等的重试治理与配额约束
Why5 → 平台内部依赖被默认信任，缺乏"内部也是租户"的隔离原则
        （系统性原因：内部工作负载的可靠性治理落后于客户侧）
```

## 解决过程

定位并重试源头收敛后路由层恢复；官方未披露细节。

## 经验教训

1. 当平台有内部工作负载时，必须**把内部流量当作不可信租户治理**：同样的配额、退避、熔断。
2. 当重试是默认恢复策略时，必须**全栈统一有界重试 + 抖动**，包括平台自己的组件——内部组件是重试风暴最容易被忽视的源头。

## 预防与改进措施

- **预防（Prevent）**：内部组件重试策略统一治理（官方未详述，建议）
- **减小爆炸半径（Contain）**：内部与客户流量在路由层物理隔离/独立配额
- **快速检测（Detect）**：路由层按来源维度的流量异常检测
- **快速恢复（Recover）**：来源级流控熔断

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 内部流量=不可信租户：隔离、配额、退避三件套对客户与内部一视同仁 |
| CRE | 与同日区域电力事故叠加时，客户需要厂商主动说明"两起独立事件"，否则信任损耗大于实际影响 |
| FDE | 按来源维度的路由层流量拆分是此类事故的第一道可观测防线 |
| SA（客情危机） | 同日双事故放大客情冲击；事件沟通需明确独立性与各自影响面 |
| SA（技术危机） | 客户侧：AI 依赖的告警要区分"平台过载"与"区域故障"两类根因，预案分别准备 |

## 参考资料

1. [Azure status history（官方）](https://azure.status.microsoft/en-us/status/history/) — E1/E2
2. [Cloud Outages 2026 事件日志（Azure OpenAI retry storm 条目）](https://ideatives.com/briefs/cloud-outages/) — E3
