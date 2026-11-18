---
id: INC-20080215-AWS-S3
title: AWS S3 首次重大中断（认证请求洪峰致单数据中心不可用约 2 小时，云计算"第一次大考"）
company: AWS
company_type: cloud-native
domain: cloud-infrastructure
date: 2008-02-15
duration_minutes: 120
severity: SEV-1
impact_scope: global
root_cause_category: capacity-overload
root_cause_tags: [s3, authentication, storage, cloud-computing, single-datacenter, capacity-spike, twitter, smugmug]
status: published
last_updated: 2026-07-29
sources:
  - https://techcrunch.com/2008/02/15/amazon-web-services-goes-down-takes-many-startup-sites-with-it/
  - https://aws.amazon.com/message/41926/
---

# AWS S3 首次中断（2008-02-15）：云计算的第一场全球大考

## 摘要

2008 年 2 月 15 日约 04:30 PT，AWS Simple Storage Service（S3）的三个地理区域之一**不可用约 2 小时**，波及 Twitter、SmugMug、37Signals 等数千家依赖 S3 存储的初创公司。这是 AWS 自 2006 年推出以来**首次大规模公开中断**，也是云计算产业的第一场公众大考。Amazon 官方解释根因：**来自多个用户的认证请求数量大幅增加**[^1]，导致单区域基础设施过载。该事件的意义不在于技术细节——而是云计算作为新兴模式的**信任分水岭**：在此之前，AWS 以"和传统数据中心一样可靠"为宣传；在此之后，**"设计时假设故障（Design for Failure）"** 成为云架构的根本原则。TechCrunch 当时的评论一针见血："除非云计算比传统数据中心更可靠，否则没人会把业务托付给它。"

[^1]: 第三方安全研究者推测为认证请求洪峰导致单数据中心容量瓶颈，但 AWS 声明未披露精确根因。

## 影响评估（CRE 视角）

- **影响面**：S3 单区域不可用约 2 小时，数千家依赖 AWS 的初创公司网站图片/文件存储不可访问
- **影响时长**：约 04:30-06:30 PT，约 2 小时
- **次生影响**：Twitter 文件托管、SmugMug 图片库、37Signals 应用等大量 Web 服务功能受损；"云计算不可靠"的论调首次进入公众视野
- **对外沟通评估**：尚可——Amazon PR 当日发布声明确认故障范围与恢复时间，但未发布详细技术复盘
- **定级依据**：核心存储服务全局不可用约 2 小时，且作为云计算标志性事件，SEV-1
- 未披露信息：精确根因（仅描述为"认证请求大幅增加"）、故障区域、受影响客户总数

## 时间线（太平洋时间，2008-02-15）

| 时间 | 事件 | 证据 |
|---|---|---|
| ~04:30 | S3 单区域不可用，认证请求洪峰致基础设施过载；Twitter/SmugMug 等服务开始报障 | E1/E3 |
| 04:30-06:30 | 工程师定位并处理，通过扩容与流量调度逐步恢复 | E1 |
| ~06:30 | 官方声明：三个区域之一已恢复至 99%+ 正常性能 | E1 |
| 02-15 全天 | Amazon PR 发布声明，解释根因为"认证请求大幅增加" | E1 |

**关键时间指标**：TTD = 即时 / TTM = 定位 & 扩容 / TTR ≈ 2h

## 技术细节与根因分析（SRE 视角）

### 背景架构

2008 年的 S3 采用三个地理区域架构，每个区域独立运行。认证请求经由共享基础设施处理。2008 年 AWS 尚处于早期阶段，基础设施的弹性伸缩能力有限。

### 因素三分

- **触发因素（Trigger）**：来自多个用户的认证请求数量大幅增加（第三方推断为认证洪峰），单区域基础设施过载。
- **根本原因（Root Cause）**：早期 S3 架构按"预期负载"设计，缺少应对突发认证洪峰的容量弹性与隔离机制。
- **扩大因素（Aggravating Factors）**：单区域无跨区域故障转移能力；认证入口与数据路径共享基础设施。
- **减轻因素（Mitigating Factor）**：其他两个区域正常；AWS 团队在 2 小时内恢复。

### 5 Whys

```
现象：S3 单区域不可用约 2 小时
Why1 → 认证请求洪峰致基础设施过载
Why2 → 认证路径容量按预期负载设计，未覆盖突发洪峰
Why3 → 早期 AWS 基础设施弹性伸缩能力有限
Why4 → 无跨区域故障转移机制
Why5 → 云计算尚处早期，"设计时假设故障"原则尚未成熟
        （系统性原因：新生代云基础设施的成长阵痛）
```

## 解决过程

AWS 工程师通过扩容认证基础设施与流量调度恢复服务，约 2 小时后恢复正常。Amazon 事后加强了对突发负载的容量规划与弹性设计。

## 经验教训

1. **云计算的"第一次大考"**：这次事件让全行业认识到——云服务并非天生可靠，必须"设计时假设故障"。
2. **认证入口是全局单点**：认证请求的突发洪峰可以击垮整个服务——认证路径应独立扩展并与数据路径隔离。
3. **单区域部署是最大风险**：2008 年的 S3 在区域级别无冗余——跨区域架构是云服务的基本要求。
4. **透明沟通建立信任**：Amazon 当日声明虽简单，但及时承认故障并量化恢复时间，为后续云厂商的沟通模式奠定了基础。

## 预防与改进措施

- **预防（Prevent）**：认证路径独立扩容与隔离；容量规划覆盖突发洪峰场景
- **减小爆炸半径（Contain）**：跨区域故障转移；认证入口与数据路径分离
- **快速检测（Detect）**：认证请求异常激增的早期告警
- **快速恢复（Recover）**：弹性扩容自动化；跨区域流量调度预案

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 云服务的信任始于第一次故障后的透明沟通——"Design for Failure" 原则源自这次事件的经验 |
| CRE | 客户对云服务的信任高度脆弱：一次中断即可动摇整个行业的信心——早期客户需要更主动的沟通与补偿 |
| FDE | 2008 年的取证手段有限，认证请求洪峰的具体原因至今未完全公开——早期云计算的事故透明度远低于今天 |
| SA（客情危机） | 数千家初创公司依赖 S3 中断 2 小时，云信任初建期的'第一次大考'；Amazon 当日及时声明安抚客户，透明沟通为后续云服务信任建设奠定基础 |
| SA（技术危机） | 单区域部署是最大风险——客户应尽早采用跨区域冗余架构；认证入口是全局单点，认证路径应独立扩展并与数据路径隔离，客户应监控认证 API 健康度 |

## 参考资料

1. [Amazon Web Services Goes Down, Takes Many Startup Sites With It (TechCrunch)](https://techcrunch.com/2008/02/15/amazon-web-services-goes-down-takes-many-startup-sites-with-it/) — E1
2. [Summary of the Amazon S3 Service Disruption (AWS 官方声明)](https://aws.amazon.com/message/41926/) — E1
3. [Amazon's S3 Outage: Usage spike or DDoS attack? (UW CSE)](https://secblog.cs.washington.edu/Security/2008/02/17/amazons-s3-outage-usage-spike-or-ddos-attack/) — E4