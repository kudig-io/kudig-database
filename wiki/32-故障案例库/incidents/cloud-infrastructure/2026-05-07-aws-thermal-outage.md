---
id: INC-20260507-AWS-USEAST1
title: AWS us-east-1 热失控故障（单可用区冷却失效致服务器过热断电，EC2/EBS 等 150+ 服务受损）
company: AWS
company_type: cloud-native
domain: cloud-infrastructure
date: 2026-05-07
duration_minutes: -1
severity: SEV-1
impact_scope: single-region
root_cause_category: hardware-facility
root_cause_tags: [cooling-failure, thermal-event, us-east-1, power-loss, single-az, ec2, ebs, cascading-failure, coinbase, fanDuel]
status: published
last_updated: 2026-07-29
sources:
  - https://www.theregister.com/off-prem/2026/05/08/aws-warns-of-ec2-impairment-as-power-loss-hits-notorious-us-east-1-region/5235509
  - https://www.networkworld.com/article/4168878/aws-hit-by-us-east-1-outage-after-data-center-thermal-event.html
  - https://mlq.ai/news/aws-data-center-overheating-triggers-major-us-east-1-outage/
---

# AWS us-east-1 热失控（2026-05-07）：冷却系统失灵，us-east-1 单可用区中暑瘫痪

## 摘要

2026 年 5 月 7 日傍晚（太平洋时间 17:25，UTC 5-08 00:25），AWS 状态页报告 us-east-1 区域检测到 **EC2 实例与 EBS 卷的可用性受损**。原因是该区域**一个可用区的数据中心冷却系统失效**，导致服务器温度持续升高——**热失控（thermal event）** 触发了硬件的保护性自动断电。AWS 确认受影响的可用区内的 EC2 实例、EBS 卷、EKS 集群、Redshift 节点、SageMaker 训练任务、ElastiCache 缓存等大面积不可用，并级联影响超过 150 个下游云服务与第三方 SaaS 平台。Coinbase 因匹配引擎锁定在该可用区而无法处理交易，发布事后复盘；FanDuel 等多家在线服务也报告服务中断。这是 **us-east-1 区域又一次因物理设施问题引发的大规模中断**（延续 2017 S3、2021 网络过载、2022 DynamoDB 等系列事故），也是云计算行业"**精密软件栈在物理设施故障面前的脆弱性**"的再验证。冷却系统作为数据中心最底层的物理保障，其故障将软件定义的一切清零。因恢复时间因服务类型而异（部分依赖自动恢复，部分需手动迁移），且受影响的单可用区为主，持续时间无统一口径，记为 -1。

## 影响评估（CRE 视角）

- **影响面**：us-east-1 单可用区内 EC2/EBS/EKS/Redshift/SageMaker/ElastiCache 等大面积不可用；级联超过 150 个下游服务；Coinbase 交易中断数小时
- **影响时长**：无统一恢复口径（因服务类型与客户部署而异），受影响单可用区为主；部分客户多可用区部署自动转移，单可用区部署中断持续数小时至数十小时
- **次生影响**：Coinbase 因匹配引擎无法跨区切换，发布独立复盘；多家金融科技/游戏公司业务中断；"us-east-1 又崩了"成为全球新闻
- **对外沟通评估**：中规中矩——状态页及时更新，但未发布深度技术复盘；恢复对客户依赖其 EC2 自动恢复能力或手动迁移
- **定级依据**：单可用区全硬件下线，级联 150+ 服务，SEV-1
- 未披露信息：冷却系统失效的具体原因、受影响 AZ 编号、设备损坏数量

## 时间线（太平洋时间，2026-05-07 ~ 05-08）

| 时间 | 事件 | 证据 |
|---|---|---|
| 05-07 下午 | 数据中心冷却系统开始失效，温度持续升高 | E1/E3 |
| ~17:25 | 服务器温度达到临界阈值，硬件保护性自动断电开始 | E1 |
| 17:25 | AWS 状态页报告 EC2/EBS 可用性受损，sus-east-1 受影响 | E1 |
| 17:25 起 | 受影响 AZ 内 EC2/EBS 等大面积不可用；Coinbase、FanDuel 等报告中断 | E1/E3 |
| 晚间~次日 | 冷却系统恢复后，逐步恢复供电与服务器上线；部分实例自动恢复，部分需手动操作 | E1 |
| 后续 | 客户迁移至其他可用区；AWS 提供受影响实例/卷的详细说明 | E1 |

**关键时间指标**：TTD = 即时（状态页报告）/ TTM = 启动恢复 / TTR = 因服务与客户而异（无统一口径）

## 技术细节与根因分析（SRE 视角）

### 背景架构

AWS us-east-1 是 AWS 最老、规模最大的区域，拥有多个可用区（AZ）。每个可用区有独立的物理基础设施（供电、冷却、网络）。冷却系统是数据中心最底层的物理保障——精密空调持续运行维持服务器温度在安全范围内。冷却失效时，服务器温度升高至临界值后将触发硬件级保护性断电。

### 因素三分

- **触发因素（Trigger）**：单可用区数据中心冷却系统失效，温度持续升高至硬件保护性断电阈值。
- **根本原因（Root Cause）**：冷却系统的物理故障（具体原因未公开）；但更深层的问题是：us-east-1 区域年代久远，其物理基础设施的设计冗余与维护周期可能未跟上负载密度的持续增长。
- **扩大因素（Aggravating Factors）**：
  1. 单可用区部署的客户无跨区容灾，业务直接中断数小时；
  2. 级联效应：EC2/EBS 下线导致依赖这些资源的 150+ 下游服务与 SaaS 平台连带受损；
  3. 部分应用（如 Coinbase 匹配引擎）因架构绑定特定可用区，无法自动故障转移；
  4. us-east-1 作为"默认区域"，大量客户与服务集中部署于此，实际影响面远超单 AZ 的物理容量。
- **减轻因素（Mitigating Factor）**：多可用区部署的客户自动转移至其他 AZ；AWS 的自动恢复机制（EC2 Auto Recovery）减少了部分手动恢复工作。

### 5 Whys

```
现象：us-east-1 单可用区全部服务器断电，150+ 服务受损
Why1 → 温度达到临界值，硬件保护性自动断电
Why2 → 数据中心冷却系统失效
Why3 → 冷却系统的物理故障（机械/电气/制冷剂等）触发
Why4 → 冷却系统的冗余机制未能覆盖此失效模式（或冗余也失效）
Why5 → 物理基础设施的维护周期与冗余设计未跟上负载密度增长
        （系统性原因：老区域物理基础设施的可靠性滞后于负载增长）
```

## 解决过程

冷却系统修复后，逐步恢复供电并上线硬件；受影响的 EC2 实例部分通过自动恢复机制上线，部分需手动操作或迁移至其他可用区。AWS 未发布详细技术复盘，客户通过状态页获取恢复进展。

## 经验教训

1. **物理基础设施的可靠性决定了云服务的可用性上限**：再精密的软件容灾设计，当底层物理设施（冷却/供电）失效时，一切软件层保障归零。
2. **单可用区部署是最大的风险敞口**：冷却故障只影响一个 AZ，但单 AZ 部署的客户 100% 受影响。多 AZ 部署是云上容灾的基本门槛。
3. **us-east-1 的"默认区域"陷阱**：大量客户默认选择 us-east-1，而该区域老旧设施的可靠性风险已被多次验证——选择区域时应考虑"物理设施年代"这一隐性因素。
4. **物理设施故障的恢复时间以小时到天计**：冷却系统修复需要时间，硬件重新上电与验证也需要时间——客户应假设物理故障的恢复时间远长于软件故障。
5. **关键服务的 AZ 锁定是架构风险**：Coinbase 匹配引擎绑定特定 AZ 无法故障转移，证明应用层也需要跨 AZ 设计——基础设施的 AZ 冗余不等于应用的 AZ 弹性。

## 预防与改进措施

- **预防（Prevent）**：冷却系统冗余设计与定期维护；温度监测与早期预警
- **减小爆炸半径（Contain）**：可用区隔离设计确保单 AZ 故障不影响其他 AZ；客户侧多 AZ 部署
- **快速检测（Detect）**：数据中心温度/湿度/供电的多维监测与告警
- **快速恢复（Recover）**：EC2 Auto Recovery 与跨 AZ 自动迁移；冷却系统快速修复预案

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 物理设施故障的恢复时间模型（RTM）应与软件故障区分——冷却系统修复的 MTTR 以小时到天计，SLI/SLO 的"可用性"应区分物理故障与软件故障 |
| CRE | 单可用区部署的客户是事故主要受害者——CRE 应持续评估客户跨 AZ 部署成熟度，并将"物理设施故障"纳入 SLA 偏离模型 |
| FDE | 冷却系统故障的取证依赖数据中心物理监控数据（温度时序、制冷系统日志、供电参数）——这些数据通常不在云厂商的公开复盘范围内，客户需自行评估可用区历史可靠性 |
| SA（客情危机） | us-east-1 冷却失效致 150+ 服务受损，单 AZ 客户 100% 受影响；AWS 状态页披露，但'默认区域'陷阱令大量客户反思'为什么把业务放在 us-east-1' |
| SA（技术危机） | 物理基础设施决定云服务可用性上限——客户应多 AZ 部署作为基本门槛；us-east-1 的'默认区域'陷阱：客户应评估业务对老旧设施的依赖并规划区域多样性 |

## 参考资料

1. [AWS EC2 impairment from power loss in us-east-1 (The Register)](https://www.theregister.com/off-prem/2026/05/08/aws-warns-of-ec2-impairment-as-power-loss-hits-notorious-us-east-1-region/5235509) — E3
2. [AWS hit by US-East-1 outage after data center thermal event (Network World)](https://www.networkworld.com/article/4168878/aws-hit-by-us-east-1-outage-after-data-center-thermal-event.html) — E3
3. [AWS Data Center Overheating Triggers Major US-East-1 Outage (MLQ.ai)](https://mlq.ai/news/aws-data-center-overheating-triggers-major-us-east-1-outage/) — E3