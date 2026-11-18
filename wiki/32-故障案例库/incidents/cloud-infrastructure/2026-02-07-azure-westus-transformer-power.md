---
id: INC-20260207-AZURE-WESTUS
title: Azure West US 现场变压器电气故障致数据中心市电全失，间歇性服务不可用 20 小时 26 分
company: Microsoft Azure
company_type: cloud-native
domain: cloud-infrastructure
date: 2026-02-07
duration_minutes: 1226
severity: SEV-2
impact_scope: single-region
root_cause_category: hardware-facility
root_cause_tags: [power-loss, transformer-failure, west-us, phased-recovery, utility-power, single-region]
status: published
last_updated: 2026-10-23
sources:
  - https://azure.status.microsoft/en-us/status/history/
  - https://www.datacenterdynamics.com/en/news/microsoft-suffered-power-interruption-at-west-us-cloud-region-in-february/
---

# Azure West US 变压器故障（2026-02-07）：市电还在，数据中心却断电了 20 小时

## 摘要

2026 年 2 月 7 日 07:58 UTC 至 2 月 8 日 04:24 UTC，Azure West US 区域发生长达 **20 小时 26 分**的服务事件：区域内一个数据中心的**现场变压器电气故障**导致数据中心失去市电——尽管外部电网（utility）本身工作正常。客户经历间歇性服务不可用、超时与高于平常的延迟。恢复分阶段推进，部分存储与计算基础设施未能立即恢复，拖长了依赖组件的恢复时间并造成遥测延迟。微软在 3 月发布的 Post Incident Report 中披露了上述细节。这是 2026 年 Azure West US 系列电力/网络事件的起点（随后是 5 月 West US 2 雷暴断电、7 月 West US 路由移除）。

## 影响评估（CRE 视角）

- **影响面**：West US 单区域内多个依赖工作负载；具体客户数与受影响服务清单未完整披露
- **功能维度**：区域内服务间歇性不可用、超时、延迟升高；部分存储/计算资源恢复滞后
- **影响时长**：07:58 UTC（2-07）→ 04:24 UTC（2-08），20h26m；客户实际感知为间歇性而非连续中断
- **错误预算**：按 99.9% SLO 计，单区域 20 小时的间歇性不可用足以消耗该区域全年错误预算的数倍
- **对外沟通评估**：一般——PIR 在事件一个月后发布，披露变压器故障与分阶段恢复事实，但未披露变压器失效机理与冗余为何未接管
- **定级依据**：单一大区域、20 小时级影响，但非完全中断且无限级联至其他区域，定为 SEV-2

## 时间线（UTC）

| 时间 (UTC) | 事件 | 证据 |
|---|---|---|
| 02-07 07:58 | 事件开始：数据中心市电中断，服务间歇性不可用开始 | E2 |
| 07:58 起数小时 | 故障转移与应急供电接管，部分服务间歇恢复 | E2/E3 |
| 随后十余小时 | 分阶段恢复；部分存储/计算基础设施未立即恢复，遥测延迟 | E2 |
| 02-08 04:24 | 事件结束，影响窗口关闭 | E2 |
| 03 月上旬 | 微软发布 Post Incident Report，确认为现场变压器电气故障 | E1 |

**关键时间指标**：TTD ≈ 未披露 / TTM ≈ 未披露 / TTR = 20h26m

## 技术细节与根因分析（SRE 视角）

### 背景架构

数据中心供电链路：外部市电（utility）→ 现场变压器（降压/配电）→ UPS 电池 → 柴油发电机。任何一环失效都会触发下游切换；本案的特殊之处在于**失效点在变压器这一环**——电网本身正常，但数据中心拿不到电。

### 因素三分

- **触发因素（Trigger）**：现场变压器电气故障
- **根本原因（Root Cause）**：变压器设备失效；未披露是绝缘老化、过载还是制造缺陷
- **扩大因素（Aggravating Factors）**：
  1. 部分存储与计算基础设施未能随主链路恢复，形成长尾恢复
  2. 遥测延迟反过来拖慢了恢复验证与客户沟通
  3. 单数据中心故障在该区域产生多工作负载的间歇性症状，定位与定界耗时
- **减轻因素**：间歇性而非连续性中断，说明冗余部分生效

### 5 Whys

```
现象：West US 区域服务间歇性不可用 20 小时
Why1 → 一个数据中心失去市电供电
Why2 → 现场变压器电气故障
Why3 → 变压器设备失效（具体机理未披露）
Why4 → 设备健康监测/预防性更换未能提前发现（推断）
Why5 → 关键电力设备的生命周期管理与状态监测覆盖不足
        （系统性原因：设施层单点失效的检测滞后于软件层监控成熟度）
```

## 解决过程

应急供电与故障转移先行保住部分负载；随后分阶段恢复基础设施，但存储与计算组件的长尾恢复拉长了整体窗口。微软未披露是否更换变压器品牌/型号或调整维护周期。

## 经验教训

1. 当电网正常而数据中心断电时，故障域在**现场配电环节**——设施监控必须覆盖"市电入口之后"的每一环，而不仅是 utility 状态。
2. 当恢复是分阶段时，必须**公开长尾组件清单与预计恢复时间**——遥测延迟造成的"看不见"会放大客户焦虑与误判。
3. 当同一区域在一年内多次发生重大事件（West US：2 月电力、5 月邻近区域雷暴、7 月路由）时，客户应把**区域级历史可靠性**纳入选址与灾备决策（本库跨案例统计支持该结论）。

## 预防与改进措施

- **预防（Prevent）**：变压器等关键电力设备的状态监测与预防性更换（官方 PIR 未详述，建议）
- **减小爆炸半径（Contain）**：数据中心间供电独立性；客户多 AZ/多区域部署
- **快速检测（Detect）**：现场配电链路分段监控（变压器温度/局放/油色谱）（建议）
- **快速恢复（Recover）**：分阶段恢复预案中的"长尾组件优先披露"机制；恢复验证自动化

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 设施层故障的检测与恢复成熟度落后于软件层——电力链路的每一环都应有与软件 SLI 等价的健康指标 |
| CRE | 间歇性中断的沟通难度高于完全中断——客户需要"哪些组件还没好"的清单而非笼统的"正在恢复" |
| FDE | 遥测随基础设施一起受损，形成恢复验证的盲区；关键遥测应有独立供电与离区采集路径（建议） |
| SA（客情危机） | 20 小时区域事件 + 同区域年度多发，将触发企业客户架构评审与合同谈判筹码 |
| SA（技术危机） | 灾备设计须区分"区域不可用"与"区域间歇性劣化"两种模式，后者对故障转移逻辑的健壮性要求更高 |

## 参考资料

1. [Microsoft suffered "power interruption" at West US cloud region in February（DataCenterDynamics）](https://www.datacenterdynamics.com/en/news/microsoft-suffered-power-interruption-at-west-us-cloud-region-in-february/) — E3
2. [Azure status history（官方）](https://azure.status.microsoft/en-us/status/history/) — E1/E2
