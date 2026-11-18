---
id: INC-20260424-AZURE-EASTUS
title: Azure East US PubSub 网络控制面锁竞争致资源配给失败 11 小时 52 分
company: Microsoft Azure
company_type: cloud-native
domain: cloud-infrastructure
date: 2026-04-24
duration_minutes: 712
severity: SEV-2
impact_scope: single-region
root_cause_category: software-bug
root_cause_tags: [lock-contention, control-plane, pubsub-networking, provisioning-failure, east-us]
status: published
last_updated: 2026-10-23
sources:
  - https://azure.status.microsoft/en-us/status/history/
  - https://ideatives.com/briefs/cloud-outages/
---

# Azure East US 控制面锁竞争（2026-04-24）：一个锁 bug 让资源"配给不出来"近 12 小时

## 摘要

2026 年 4 月 24 日，Azure East US 区域的 **PubSub 网络控制面**发生锁竞争（lock contention），导致依赖该控制面的资源配给操作持续失败 **11 小时 52 分**。这是典型的"控制面可用性 ≠ 数据面可用性"事故：已运行的资源不受影响，但新建/变更/扩缩类操作被锁死。与 2 月 FNJ8-VQZ（存储策略）同属 Azure 2026 年控制面事故序列，根因形态不同但客户感受一致——平台"卡死"。

## 影响评估（CRE 视角）

- **影响面**：East US 单区域；受影响面为依赖 PubSub 网络控制面的资源生命周期操作
- **功能维度**：资源配给/网络配置下发失败；运行中负载不受影响
- **影响时长**：11h52m
- **错误预算**：对发布窗口落在该时段的团队，等效于 CI/CD 与扩容能力全天不可用
- **对外沟通评估**：一般（状态页通报，无深度复盘细节）
- **定级依据**：单区域控制面 12 小时级故障，数据面未受影响，定为 SEV-2

## 时间线（UTC）

| 时间 (UTC) | 事件 | 证据 |
|---|---|---|
| 04-24 早间 | 锁竞争出现，配给操作开始失败 | E2 |
| 04-24 全天 | 控制面持续劣化，客户配给请求失败/重试 | E2/E3 |
| 04-24 晚间 | 故障缓解，配给恢复 | E2 |

**关键时间指标**：TTD ≈ 未披露 / TTM ≈ 未披露 / TTR = 11h52m（第三方汇总口径）

## 技术细节与根因分析（SRE 视角）

### 背景架构

Azure 网络资源（如虚拟网络相关组件）的配给经由 PubSub 风格的网络控制面异步编排；控制面内部用分布式锁保证一致性。锁竞争 = 多个操作长时间争夺同一把锁，队列积压，配给超时。

### 因素三分

- **触发因素（Trigger）**：未披露的具体操作模式（推断为某类高频/长事务操作放大锁持有时间）
- **根本原因（Root Cause）**：控制面锁机制缺陷——锁粒度过粗或锁释放逻辑存在 bug（第三方汇总表述为 lock contention）
- **扩大因素（Aggravating Factors）**：客户与平台自身的重试进一步加剧锁竞争（retry 放大经典模式）；单区域控制面无分片隔离

### 5 Whys

```
现象：East US 资源配给失败 11h52m
Why1 → 配给请求在控制面排队超时
Why2 → PubSub 网络控制面锁竞争
Why3 → 锁粒度/释放逻辑存在缺陷
Why4 → 锁行为在高并发配给场景下未经压测覆盖
Why5 → 控制面缺少锁竞争的自保护（自适应锁升级、超时熔断、队列隔离）
        （系统性原因：共享控制面的内部并发缺陷缺乏与数据面同级的可靠性标准）
```

## 解决过程

官方未披露处置细节（第三方汇总仅确认持续时长与根因形态）；按此类事故常规，需定位热点锁、短期扩容/重启控制面组件消化积压、长期修锁逻辑。

## 经验教训

1. 当控制面使用分布式锁时，必须**监控锁等待时间与锁持有者分布**并设自适应熔断，否则锁竞争会无声地吃掉整个区域的生命周期操作。
2. 当配给失败时，平台与 SDK 双方都必须**有界重试 + 抖动**，否则恢复阶段的"重试洪峰"会延长故障（2 月 Azure 身份后端二次击穿已验证此模式）。

## 预防与改进措施

- **预防（Prevent）**：锁粒度细化与锁行为压测纳入控制面发布门禁（建议）
- **减小爆炸半径（Contain）**：控制面分片/单元化（cell 架构）隔离区域级影响
- **快速检测（Detect）**：锁等待时长、配给队列深度作为一级 SLI（建议）
- **快速恢复（Recover）**：锁热点隔离与队列丢弃策略；客户侧重试退避

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 分布式锁是控制面的隐形炸弹：锁指标必须与业务 SLI 同页展示 |
| CRE | 控制面故障的客户沟通必须明确"运行中资源安全、不能新建/变更"的边界，减少恐慌性迁移 |
| FDE | 配给失败率的区域级异常是此类故障最早的可用信号——比客户工单更早 |
| SA（客情危机） | East US 是 Azure 密度最高区域之一，12 小时配给中断影响发布节奏，客户会将此纳入区域选择评估 |
| SA（技术危机） | 客户侧：关键发布避开单区域窗口，保留跨区/跨云配给备选路径 |

## 参考资料

1. [Azure status history（官方）](https://azure.status.microsoft/en-us/status/history/) — E1/E2
2. [Cloud Outages 2026 事件日志（Azure East US control-plane lock contention 条目）](https://ideatives.com/briefs/cloud-outages/) — E3
