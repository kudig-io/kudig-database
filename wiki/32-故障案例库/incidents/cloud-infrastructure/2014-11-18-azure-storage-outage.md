---
id: INC-20141118-AZURE-STORE
title: Azure 存储服务全球中断（性能升级操作中的人为错误致存储集群大面积不可用约 11 小时）
company: Microsoft Azure
company_type: cloud-native
domain: cloud-infrastructure
date: 2014-11-18
duration_minutes: 660
severity: SEV-1
impact_scope: global
root_cause_category: change-management
root_cause_tags: [azure-storage, performance-upgrade, human-error, storage-cluster, cascading-failure, rollback, msn, office-365, xbox-live]
status: published
last_updated: 2026-07-29
sources:
  - https://azure.microsoft.com/en-us/blog/final-root-cause-analysis-and-improvement-areas-nov-18-azure-storage-service-interruption/
  - https://www.cio.de/article/3676107/human-error-root-cause-of-november-microsoft-azure-outage-3.html
  - https://www.silicon.co.uk/cloud/cloud-management/microsoft-azure-cloud-outage-157782
---

# Azure 存储中断（2014-11-18）：一次性能升级操作中的"人为错误"，让微软云瘫痪 11 小时

## 摘要

2014 年 11 月 18 日，微软 Azure 的**存储服务**经历了一次大规模中断，持续约 11 小时，波及 Azure 官网、Office 365、Xbox Live、MSN 等大量微软服务及第三方 SaaS 应用。微软事后发布最终根因分析（Final Root Cause Analysis），确认根因为**人为错误**——工程师在**执行存储集群性能升级操作**时，错误地**将生产存储集群的配置参数修改为不兼容的值**，导致存储集群大面积不可用。该故障的恢复受限于存储集群的规模——回滚操作涉及大量存储节点的数据一致性恢复，恢复时间被拉长至 11 小时。这是微软自 2012 年闰年 bug 以来最严重的 Azure 中断，也是"人为错误"在云基础设施中引发大规模故障的经典案例——与 2017 年 AWS S3 的"错误的调试命令"属于同一类别的故障模式。

## 影响评估（CRE 视角）

- **影响面**：Azure 存储服务全球大面积不可用，影响 Azure 官网、Office 365（部分）、Xbox Live、MSN 及大量第三方客户应用
- **影响时长**：约 11 小时（11 月 18 日起，部分区域持续至 11 月 19 日）
- **次生影响**：Xbox Live 玩家无法登录/联机；Office 365 企业用户受影响；Azure 2014 年最严重的中断
- **对外沟通评估**：优秀——微软发布最终根因分析（Final RCA），含详细技术细节，时间线与改进措施，透明度极高
- **定级依据**：核心存储服务全球中断超 11 小时，影响自有与客户服务，SEV-1
- 未披露信息：受影响客户数量、存储集群的具体配置参数

## 时间线（UTC，2014-11-18 ~ 11-19）

| 时间 | 事件 | 证据 |
|---|---|---|
| 11-18 白天 | 工程师执行存储集群性能升级操作，人为错误修改配置为不兼容值 | E1 |
| 11-18 白天起 | 存储集群大面积不可用，Azure 存储、Office 365、Xbox Live 等受影响 | E1/E3 |
| 11-18~11-19 | 工程师定位到人为错误，开始回滚配置操作 | E1 |
| 11-19 | 回滚完成，存储集群逐步恢复，总恢复时间约 11 小时 | E1 |
| 11 月后续 | 微软发布最终根因分析，含改进措施清单 | E1 |

**关键时间指标**：TTD = 即时 / TTM = 定位到人为错误 / TTR ≈ 11h

## 技术细节与根因分析（SRE 视角）

### 背景架构

Azure 存储服务是 Azure 平台的底层基础设施，为所有 Azure 服务（虚拟机、数据库、网站等）提供持久化存储。存储集群的性能升级操作涉及修改配置参数，以启用新的存储硬件或优化存储性能。

### 因素三分

- **触发因素（Trigger）**：工程师在存储集群性能升级操作中，将生产配置参数修改为不兼容的值。
- **根本原因（Root Cause）**：人为错误——性能升级操作的人工执行步骤中，配置参数被错误地修改，导致存储集群无法正常服务。
- **扩大因素（Aggravating Factors）**：
  1. 存储集群规模庞大，回滚操作涉及大量节点的数据一致性恢复；
  2. 存储服务是 Azure 的底层依赖，其故障级联影响所有上层服务；
  3. 性能升级操作的人工步骤缺乏自动化验证与防误操作保护。
- **减轻因素（Mitigating Factor）**：Azure 存储系统的数据持久性未受影响，数据未丢失。

### 5 Whys

```
现象：Azure 存储服务全球中断约 11 小时
Why1 → 存储集群性能升级操作中配置参数被错误修改
Why2 → 人为错误：执行人工步骤时输入了不兼容配置值
Why3 → 性能升级操作无自动化验证与防误操作保护
Why4 → 人工执行步骤的检查清单与验证机制不足
Why5 → 云基础设施的变更操作仍依赖人工判断，
        缺少"变更预验证→自动熔断→自动回滚"的自动化安全网
        （系统性原因：基础设施变更操作的安全性设计不足）
```

## 解决过程

工程师定位到人为错误后，执行回滚操作恢复配置。因存储集群规模大，回滚涉及数据一致性验证，恢复时间约 11 小时。微软整改：为存储集群的性能升级操作增加自动化验证、变更预检查、防误操作保护机制。

## 经验教训

1. **"人为错误"是云基础设施最危险的故障源**：一次配置参数输入错误，可以让整个云平台瘫痪 11 小时——自动化验证与防误操作是必需的安全网。
2. **存储服务的 MTTR 与数据量成正比**：存储集群的回滚不仅是改配置，还涉及数据一致性验证——恢复时间无法通过简单重启缩短。
3. **性能升级操作是高危操作**：本意是"提升性能"的操作，由于缺乏安全防护变成了"毁掉所有服务"的操作——升级操作的风险评估应覆盖"操作失误"场景。
4. **底层存储故障击穿所有上层抽象**：Azure 存储不可用时，所有依赖存储的服务（虚拟机、数据库、网站）同时不可用——存储的可靠性投入应远超其他组件。

## 预防与改进措施

- **预防（Prevent）**：配置变更的自动化预验证；人为操作步骤的防误保护
- **减小爆炸半径（Contain）**：存储集群的逐步变更（灰度），非全量同时变更
- **快速检测（Detect）**：存储集群健康状态实时监控；配置变更的异常检测告警
- **快速恢复（Recover）**：自动化回滚机制；存储集群配置变更的"一键回滚"能力

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 人为错误无法通过"加强培训"消除——自动化验证与防误操作机制是唯一可靠的防护手段；变更操作的风险评估应覆盖"如果操作者输入了错误值怎么办" |
| CRE | 存储是云服务的"最后一道防线"——存储故障的恢复时间以小时计，客户应评估关键数据的跨区域备份策略 |
| FDE | 人为错误的取证相对直接——变更日志中记录了操作者输入的值，但与"预期值"的差异需要人工审查；改进措施应聚焦"如何让系统阻止错误的值被应用"而非"如何让操作者不再犯错" |
| SA（客情危机） | 存储中断 11 小时波及 Office 365/Xbox/第三方 SaaS，企业客户信任受损；微软发布最终根因分析，但'人为错误瘫痪整个平台'的现实令客户对 Azure 存储可靠性信心受挫 |
| SA（技术危机） | 存储 MTTR 与数据量成正比——客户应跨区域复制+定期恢复演练；性能升级是高风险操作，客户应关注供应商变更管控，关键数据保留云外副本 |

## 参考资料

1. [Final Root Cause Analysis and Improvement Areas (Azure 官方复盘)](https://azure.microsoft.com/en-us/blog/final-root-cause-analysis-and-improvement-areas-nov-18-azure-storage-service-interruption/) — E1
2. [Human error root cause of November Microsoft Azure outage (CIO)](https://www.cio.de/article/3676107/human-error-root-cause-of-november-microsoft-azure-outage-3.html) — E3