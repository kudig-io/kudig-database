---
id: INC-20250108-AZURE-EASTUS2
title: Azure East US 2 长达 50 小时的网络中断（区域网络配置变更致存储分区不可用，虚拟机/App Service/数据库等大量服务连续 3 天受损）
company: Microsoft Azure
company_type: cloud-native
domain: cloud-infrastructure
date: 2025-01-08
duration_minutes: 3000
severity: SEV-1
impact_scope: single-region
root_cause_category: change-management
root_cause_tags: [azure, east-us-2, networking, storage-partition, configuration-change, cascading-failure, disaster-recovery, private-link, virtual-machine, app-service, databricks, zonal-failure]
status: published
last_updated: 2026-08-01
sources:
  - https://azure.status.microsoft/en-us/status/history/
  - https://build5nines.com/major-azure-networking-outage-in-east-us-2-affecting-vms-app-service-and-more-started-january-8-2025/
  - https://www.theregister.com/off-prem/2025/01/10/microsoft-azure-networking-snafu-enters-day-2/
---

# Azure East US 2 50 小时中断（2025-01-08）：一次网络配置变更，三天才恢复

## 摘要

2025 年 1 月 8 日 22:00 UTC 至 1 月 11 日 04:30 UTC，Microsoft Azure East US 2 区域因一次**区域网络服务的配置变更**导致长达 **50 小时的严重中断**。根因是：网络配置变更导致一个可用区内的**三个存储分区（storage partition）变为不健康状态**，触发级联效应——虚拟机连接问题、资源分配失败、服务间通信中断。受影响的服务包括 Azure Databricks、App Service、Function Apps、Container Apps、Logic Apps、SQL Managed Instances、API Management、PowerBI、VMSS、PostgreSQL 等——几乎涵盖了该区域的所有核心 Azure 服务。这是 **Azure 历史上持续时间最长的单区域故障之一**，也是 2025 年云计算行业最大的故障之一。该事件暴露了 Azure 在**单 Zone 故障的存储分区依赖**方面的脆弱性。

## 影响评估（CRE 视角）

- **影响面**：East US 2 区域单 Zone 内大量 Azure 服务不可用或严重降级，持续 50 小时；受影响的客户无法创建新资源、无法连接现有虚拟机
- **影响时长**：约 22:00 UTC（1 月 8 日）至 04:30 UTC（1 月 11 日），约 50 小时
- **次生影响**：客户被迫启动灾难恢复（DR）流程；部分客户因单 Zone 部署而无法恢复；Microsoft 建议客户执行 DR 流程
- **对外沟通评估**：待改进——虽然 Microsoft 在状态页持续更新，但评估为"影响限于单 Zone"可能低估了实际影响（许多服务跨 Zone 依赖）
- **定级依据**：持续时间长达 50 小时，影响大量 Azure 核心服务，SEV-1
- 未披露信息：受影响客户精确数量、经济损失

## 时间线（UTC，2025-01-08 ~ 01-11）

| 时间 | 事件 | 证据 |
|---|---|---|
| 01-08 22:00 | 区域网络服务的配置变更导致 East US 2 单 Zone 网络异常 | E1/E2 |
| 01-08 | 三个存储分区变为不健康状态，大量服务开始受影响 | E1 |
| 01-09 | Microsoft 将流量从受影响 Zone 重路由，部分非 zonal 服务恢复 | E1 |
| 01-10 14:36 | 两个存储分区恢复，部分服务恢复功能 | E1 |
| 01-10 16:39 | 第三个存储分区恢复，所有存储分区重新上线 | E1 |
| 01-11 04:30 | Microsoft 确认所有 Azure 服务在 East US 2 区域恢复"良好"状态 | E1 |

**关键时间指标**：TTD = 即时 / TTM = 耗时数天 / TTR ≈ 50h

## 技术细节与根因分析（SRE 视角）

### 背景架构

Azure East US 2 区域采用可用区（Zone）架构，同一个区域内的不同 Zone 之间具有网络隔离。Azure 的存储分区是底层存储系统的基本单元，许多 Azure 服务（VM、App Service、Databricks 等）依赖存储分区进行状态管理和数据持久化。

### 因素三分

- **触发因素（Trigger）**：区域网络服务的配置变更导致 East US 2 中一个可用区的网络状态不一致。
- **根本原因（Root Cause）**：网络配置变更导致三个存储分区变为不健康。存储分区不可用后，依赖这些分区的服务无法正常工作。
- **扩大因素（Aggravating Factors）**：
  1. 存储分区的恢复需要"重新水合"（rehydration），过程耗时数天；
  2. 许多服务虽被设计为跨 Zone 冗余，但实际依赖共同的底层存储分区；
  3. Private Link 和 NSG 等网络功能也受影响，进一步扩大了故障范围；
  4. 部分受影响服务需要手动启动 DR 流程才能恢复。
- **减轻因素（Mitigating Factor）**：Microsoft 将非 zonal 服务的流量重路由至其他可用区，部分服务得到缓解；客户启动 DR 流程后可以恢复部分服务。

### 5 Whys

```
现象：East US 2 单 Zone 大量服务中断 50 小时
Why1 → 三个存储分区变为不健康，依赖这些分区的服务不可用
Why2 → 区域网络配置变更导致存储分区网络异常
Why3 → 存储分区的"重新水合"机制耗时极长
Why4 → 单 Zone 故障通过存储分区依赖级联到大量服务
Why5 → 存储分区的恢复时间目标（RTO）过短，
        未覆盖"网络配置变更致存储分区集体不可用"的场景
        （系统性原因：存储分区恢复机制在"网络层面故障"场景下效率不足）
```

## 解决过程

Microsoft 将流量从受影响 Zone 重路由，逐步恢复存储分区。第一个分区在 1 月 10 日 14:36 恢复，第三个在 16:39 恢复。全部恢复后 Microsoft 继续监控服务状态直至 1 月 11 日 04:30 确认完全恢复。

## 经验教训

1. **"存储分区"是单 Zone 故障的"放大器"**：网络配置变更本应只影响网络层面，但通过存储分区的依赖关系级联到大量服务——"存储是基础设施的基础设施"。
2. **50 小时的恢复时间不可接受**：对于单 Zone 故障，50 小时的恢复时间远远超出了大多数客户的预期——存储分区的"重新水合"机制需要优化。
3. **"单 Zone 部署"客户是最大受害者**：跨 Zone 部署的客户可以通过 DR 流程恢复，但单 Zone 部署的客户只能等待——Azure 应更主动地推动客户采用跨 Zone 架构。
4. **"影响限于单 Zone"的评估可能低估了实际影响**：许多服务虽然跨 Zone 部署，但依赖共同的底层存储分区——"Zone 隔离"在存储层面可能并未真正实现。

## 预防与改进措施

- **预防（Prevent）**：网络配置变更的自动化验证与灰度发布；存储分区对网络配置变更的隔离保护
- **减小爆炸半径（Contain）**：存储分区的跨 Zone 冗余；网络配置变更的分 Zone 灰度
- **快速检测（Detect）**：存储分区健康状态的实时监控；网络配置变更对存储分区影响的自动检测
- **快速恢复（Recover）**：存储分区的快速"重新水合"机制优化；网络配置变更的自动回滚

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 存储分区是"基础设施的基础设施"——网络配置变更通过存储分区依赖将单 Zone 故障放大为三天级灾难；存储分区的 RTO 是所有服务恢复时间的关键瓶颈 |
| CRE | 50 小时的故障对客户来说是一场噩梦——"按 SLA 赔偿"远远不够，因为客户在 50 小时内无法使用核心服务，业务损失远超赔偿金额 |
| FDE | 存储分区恢复的取证需要"存储分区健康状态 + 网络配置变更 + 服务恢复时间"的三维关联——"网络配置变更→存储分区故障→服务不可用"的因果链需要小时级精度的日志对齐 |
| SA（客情危机） | East US 2 区域 50 小时中断，企业客户连续 3 天受损，'恢复时间不可接受'的声讨强烈；Azure 复盘但单 Zone 客户的损失已成事实，客户对区域级故障的容忍度下降 |
| SA（技术危机） | 单 Zone 部署是最大受害者——客户应跨 Zone 部署并定期演练切换；存储分区是单 Zone 故障的放大器，客户应理解存储依赖链并配置多区冗余 |

## 参考资料

1. [Microsoft Azure Status History (Tracking ID: PLP3-1W8)](https://azure.status.microsoft/en-us/status/history/) — E1
2. [Major Azure Networking Outage in East US 2 (Build5Nines)](https://build5nines.com/major-azure-networking-outage-in-east-us-2-affecting-vms-app-service-and-more-started-january-8-2025/) — E1
3. [Microsoft Azure networking snafu enters day 2 (The Register)](https://www.theregister.com/off-prem/2025/01/10/microsoft-azure-networking-snafu-enters-day-2/) — E1