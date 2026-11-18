---
id: INC-20260529-AZURE-WESTUS2
title: 雷暴致 Azure West US 2 多数据中心市电全失，发电机部分失效 + 制冷中断，故障超出 N+1 冗余持续 22 小时 6 分
company: Microsoft Azure
company_type: cloud-native
domain: cloud-infrastructure
date: 2026-05-29
duration_minutes: 1326
severity: SEV-1
impact_scope: single-region
root_cause_category: hardware-facility
root_cause_tags: [lightning, power-loss, generator-failure, cooling-failure, n-plus-1-exceeded, cascading-failure, west-us-2]
status: published
last_updated: 2026-10-23
sources:
  - https://azure.status.microsoft/en-us/status/history/
  - https://www.linkedin.com/posts/durgeshmishra1_azure-westus2-outage-activity-7468539125055373312-Scze
  - https://status.gorelo.io/incident/908083
---

# Azure West US 2 雷暴断电（2026-05-29）：N+1 冗余被级联故障击穿，22 小时的区域级停电

## 摘要

2026 年 5 月 29 日约 04:27 UTC，强雷暴击中 Azure West US 2 数据中心群，造成**多个数据中心的市电（utility power）完全中断**。随后出现了云计算设施故障中最不愿见到的剧本：**发电机故障转移部分失效**（同步与热保护问题）且**制冷系统中断**，级联故障**超出了 N+1 设计冗余**。Azure SQL DB、Redis、Functions、Storage、VM/Container Apps、Log Analytics 等大批服务受损，部分存储组件直至次日晚间仍在做数据完整性校验。影响持续 **22 小时 6 分**（至 5 月 30 日约 02:33 UTC），是 2026 年全球云厂商公开事件中最长的一次。同日 Azure OpenAI 还发生了另一起独立事故（路由层重试风暴，见 INC-20260529-AZURE-OPENAI-ROUTING）——同一区域同一天两场事故，放大了客户侧的整体受损面。

## 影响评估（CRE 视角）

- **影响面**：West US 2 区域级、跨两个可用区的数据中心群；服务清单覆盖 Azure SQL、Storage、Redis、Functions、Compute、Application Insights/Log Analytics 等
- **功能维度**：区域服务整体受损；Log Analytics 最后恢复，且其不可用直接剥夺客户与服务商自身的观测能力
- **影响时长**：22h06m；存储组件长尾更久（数据完整性校验）
- **次生影响**：下游 SaaS（依赖该区域的服务）间歇性降级；客户无法通过 Log Analytics 验证自身业务是否恢复，形成"观测黑洞"
- **对外沟通评估**：一般——分阶段更新确认了电力/制冷/发电机细节，但无完整 RCA
- **定级依据**：区域级、双 AZ 数据中心群、冗余被击穿、22 小时级中断，定为 SEV-1

## 时间线（UTC）

| 时间 (UTC) | 事件 | 证据 |
|---|---|---|
| 05-29 04:27 | 强雷暴致 West US 2 多数据中心市电完全中断 | E1/E3 |
| 04:30 | Azure 状态页报告 USW 区域电力事件 | E1 |
| 随后 | 发电机故障转移部分失效（同步 + 热保护问题），制冷中断，级联超出 N+1 | E1/E3 |
| ~08:00–17:30 | 下游客户观测到邮件、连接、后端性能间歇性受损 | E3 |
| 05-29 傍晚 | 市电恢复，制冷正常化，大部分网络/存储基础设施恢复 | E1 |
| 05-29 21:00 前后 | 两个存储组件仍在完成恢复与数据完整性校验 | E1/E3 |
| 05-30 ~02:33 | 事件结束（22h06m）；Log Analytics 为最后缓解的服务之一 | E1/E2 |

**关键时间指标**：TTD ≈ 状态页 04:30 通报 / TTM ≈ 应急供电与修复并行 / TTR = 22h06m

## 技术细节与根因分析（SRE 视角）

### 背景架构

数据中心供电与制冷的容错设计：市电双路 + 柴油发电机（N+1）+ UPS 电池撑住切换间隙；制冷通常依赖市电/发电机供电。设计的隐含假设：**单点失效**可被冗余吸收。本案是多点并发失效：市电全失 + 发电机部分失效 + 制冷中断。

### 因素三分

- **触发因素（Trigger）**：强雷暴导致多数据中心市电中断
- **根本原因（Root Cause）**：级联故障超出 N+1 冗余——发电机同步与热保护问题导致部分机组未能接管，制冷系统随之中断；N+1 按"单机组失效"设计，未覆盖"市电全失 + 部分发电机失效 + 制冷中断"的组合场景
- **扩大因素（Aggravating Factors）**：
  1. 影响跨两个可用区的数据中心群，区域级冗余同时受压
  2. 制冷中断使高温风险叠加，恢复必须"先降温再上电"，拉长 RTO
  3. Log Analytics 受损形成观测黑洞，恢复验证与客户沟通双双受阻
- **减轻因素**：存储最终完成数据完整性校验，未报告数据丢失

### 5 Whys

```
现象：West US 2 区域服务中断 22 小时
Why1 → 多数据中心供电与制冷同时不可用
Why2 → 市电中断 + 发电机部分失效 + 制冷中断的级联
Why3 → 发电机同步/热保护缺陷 + 制冷对供电的依赖未被独立兜底
Why4 → 冗余设计按"单点失效"验证，未按"组合失效场景"做故障树分析
Why5 → 设施冗余的验证方法论落后于极端天气频率上升的现实
        （系统性原因：气候驱动的多故障并发场景未纳入设计基线）
```

## 解决过程

先以应急手段恢复供电链路（修复发电机转移、恢复市电），制冷正常化后分阶段上电；存储组件最后完成完整性校验；Log Analytics 独立跟进最后缓解。

## 经验教训

1. 当评估设施冗余时，必须按**故障组合场景（市电 × 发电机 × 制冷）做故障树分析**，而不是逐一验证单点冗余——N+1 只承诺单点失效。
2. 当制冷依赖供电链路时，必须**为制冷设计独立的兜底**（储冷、独立机组），否则"电没了"必然推出"热炸了"。
3. 当区域进入长时间中断时，必须**把观测平台（Log Analytics 类）列入最高恢复优先级**——观测黑洞会同时拖慢恢复决策与客户信任。
4. 当极端天气频率上升时，区域选址与设施设计基线必须更新（与 AWS us-east-1 热失控、本案例构成同主题跨案例证据链）。

## 预防与改进措施

- **预防（Prevent）**：组合失效场景的压力测试纳入设施认证（建议）；发电机同步/热保护缺陷排查
- **减小爆炸半径（Contain）**：跨 AZ 供电/制冷真正独立；客户跨区部署
- **快速检测（Detect）**：气象预警与设施状态的联动告警（雷暴前预置应急资源）
- **快速恢复（Recover）**："先降温后上电"的标准作业程序；观测平台独立供电与离区采集（建议）

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | N+1 是单点失效承诺而非组合失效承诺——设施可靠性验证必须升级为多故障注入 |
| CRE | 22 小时区域中断 + 同日第二场事故：客户需要的不是分服务状态页，而是区域级统一事件视图 |
| FDE | Log Analytics 最后恢复 = 观测平台必须有自己的灾备等级；22 小时无遥测的恢复验证如何做预案 |
| SA（客情危机） | 同区域同日两场独立事故严重冲击企业客户对区域可靠性的信心，灾备演练需求将显著上升 |
| SA（技术危机） | 企业架构评审必须把"组合设施失效"写入故障假设清单，并按 24h+ 区域不可用设计兜底 |

## 参考资料

1. [Azure West US 2 outage caused by severe thunderstorm（LinkedIn 事故梳理）](https://www.linkedin.com/posts/durgeshmishra1_azure-westus2-outage-activity-7468539125055373312-Scze) — E3
2. [Gorelo 状态页：US West Azure power outage 完整时间线（含 Azure 通报转述）](https://status.gorelo.io/incident/908083) — E3
3. [Azure status history（官方）](https://azure.status.microsoft/en-us/status/history/) — E1/E2
