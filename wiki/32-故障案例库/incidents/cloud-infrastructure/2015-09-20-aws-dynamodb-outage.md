---
id: INC-20150920-AWS-DDB
title: AWS DynamoDB us-east-1 中断（DNS 解析故障致 DynamoDB 不可用，级联影响多个 AWS 服务）
company: AWS
company_type: cloud-native
domain: cloud-infrastructure
date: 2015-09-20
duration_minutes: -1
severity: SEV-1
impact_scope: single-region
root_cause_category: network-routing
root_cause_tags: [dynamodb, dns, us-east-1, cascading-failure, route53, ec2, ebs, elb, auto-scaling, cloudwatch]
status: published
last_updated: 2026-07-29
sources:
  - https://aws.amazon.com/premiumsupport/technology/pes/
---

# AWS DynamoDB us-east-1 中断（2015-09-20）：DNS 解析故障让 DynamoDB 从 us-east-1 消失

## 摘要

2015 年 9 月 20 日，AWS 的 **DynamoDB 服务**在 **us-east-1 区域** 经历了一次严重的服务中断。AWS 在事后发布的 PES（Post-Event Summary）中确认，根因为 **DynamoDB 的 DNS 解析系统出现故障**，导致 DynamoDB API 端点不可解析，所有依赖 DynamoDB 的服务与客户应用无法访问该服务。该故障还级联影响 EC2、EBS、ELB、Auto Scaling、CloudWatch 等依赖 DynamoDB 的 AWS 服务。这是 AWS 在 2015 年最严重的中断之一，也是"**DNS 故障**"作为云服务根因的高频案例。us-east-1 作为 AWS 最老也是最大的区域，其 DNS 基础设施的可靠性在 2015 年已成为行业内反复讨论的话题。该事件为后续 2017 年 S3 大规模中断和 2020 年 Kinesis 中断等 DNS 相关故障埋下了"us-east-1 可靠性"的争议伏笔。

## 影响评估（CRE 视角）

- **影响面**：us-east-1 区域 DynamoDB 不可用，级联影响 EC2、EBS、ELB、Auto Scaling、CloudWatch 等依赖 DynamoDB 的 AWS 服务
- **影响时长**：无统一恢复口径（因服务类型不同而异）
- **次生影响**：依赖 DynamoDB 的客户应用中断；AWS 服务之间的内部依赖关系被暴露
- **对外沟通评估**：良好——AWS 发布 PES（Post-Event Summary），但细节有限
- **定级依据**：核心数据库服务长时间不可用，级联影响多个 AWS 服务，SEV-1
- 未披露信息：DNS 故障的具体技术细节、恢复时间精确值、受影响客户数量

## 时间线（UTC，2015-09-20）

| 时间 | 事件 | 证据 |
|---|---|---|
| 09-20 白天 | DynamoDB DNS 解析系统故障，API 端点不可解析 | E1 |
| 09-20 白天起 | DynamoDB 不可用，级联影响 EC2/EBS/ELB 等依赖 DynamoDB 的服务 | E1 |
| 09-20 | 工程师定位到 DNS 故障并修复 | E1 |
| 09-20 后续 | 服务逐步恢复；AWS 发布 PES | E1 |

**关键时间指标**：TTD = 即时 / TTM = 定位 DNS 问题 / TTR = 因服务而异

## 技术细节与根因分析（SRE 视角）

### 背景架构

DynamoDB 是 AWS 的托管 NoSQL 数据库服务，被大量 AWS 内部服务（如 EC2、EBS、ELB、Auto Scaling、CloudWatch）作为后端存储使用。DynamoDB 的 API 端点通过 DNS 解析，DNS 解析系统故障时，所有客户端（包括 AWS 内部服务）无法连接 DynamoDB。

### 因素三分

- **触发因素（Trigger）**：DynamoDB 的 DNS 解析系统发生故障，导致 API 端点不可解析。
- **根本原因（Root Cause）**：DNS 解析系统的故障（具体细节未公开），导致 DynamoDB 端点从 DNS 中消失。
- **扩大因素（Aggravating Factors）**：
  1. DynamoDB 被大量 AWS 内部服务作为后端依赖，DNS 故障级联影响多个服务；
  2. 无 DNS 解析结果的缓存/降级机制，所有客户端直接失败；
  3. us-east-1 作为"默认区域"，大量客户与服务集中于此，实际影响面扩大。
- **减轻因素（Mitigating Factor）**：其他区域 DynamoDB 正常；DNS 修复后服务逐步恢复。

### 5 Whys

```
现象：us-east-1 DynamoDB 不可用，级联影响多个 AWS 服务
Why1 → DynamoDB API 端点 DNS 解析失败
Why2 → DynamoDB DNS 解析系统发生故障
Why3 → DNS 解析系统无冗余或故障转移机制
Why4 → DynamoDB 作为基础设施依赖，DNS 解析的可靠性未达
        到"基础设施级别"的预期
Why5 → 云服务的 DNS 解析链路是"被忽视的依赖"——
        所有服务依赖 DNS，但 DNS 的可靠性设计往往被低估
        （系统性原因：DNS 解析作为基础设施依赖的可靠性不足）
```

## 解决过程

工程师定位到 DNS 故障后修复 DNS 解析系统，DynamoDB 服务逐步恢复。AWS 事后加强了对 DynamoDB DNS 解析系统的冗余设计与监控。

## 经验教训

1. **DNS 是云服务最容易被忽视的全局单点**：DNS 故障时，上层的可用区/区域冗余设计全部失效——因为服务根本找不到入口。
2. **DynamoDB 是 AWS 内部服务的"关键依赖"**：DynamoDB 的故障不仅影响外部客户，还级联影响 EC2、EBS 等 AWS 自有服务——内部依赖的可靠性要求应高于外部服务。
3. **DNS 解析的降级策略是必要的**：当 DNS 解析系统不可用时，客户端应能使用缓存 DNS 结果或备用解析路径，而不是直接失败。
4. **us-east-1 的 DNS 问题在 2015 年已埋下隐患**：2017 年 S3 的 DNS 相关故障（S3 在 2017 年因 S3 团队调试命令导致）和 2025 年 DynamoDB DNS 故障，都说明 us-east-1 的 DNS 基础设施是长期风险点。

## 预防与改进措施

- **预防（Prevent）**：DNS 解析系统的冗余设计；DNS 故障的自动检测与切换
- **减小爆炸半径（Contain）**：DynamoDB 客户端的 DNS 缓存与降级策略；跨区域故障转移
- **快速检测（Detect）**：DNS 解析异常的实时告警；DynamoDB 可用性的全球探测
- **快速恢复（Recover）**：DNS 解析故障的自动修复机制；备用 DNS 解析路径

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | DNS 是云服务拓扑中的"隐形依赖"——所有服务依赖 DNS，但 DNS 的可用性 SLI 很少被纳入服务的 SLO 计算中 |
| CRE | DynamoDB 故障导致 AWS 内部服务也级联不可用——客户无法区分"DynamoDB 故障"与"EC2 故障"；服务依赖图是 CRE 必须掌握的客户视角 |
| FDE | DNS 解析故障的取证依赖 DNS 系统日志——DNS 查询失败的时间序列与 DynamoDB 不可用的时间线的对齐是根因定位的关键 |
| SA（客情危机） | DynamoDB us-east-1 中断级联多个 AWS 服务，客户对'DNS 隐形单点'的认知加深；AWS PES 披露详尽，但内部依赖链的脆弱性令企业客户审视自身对单一区域的信赖 |
| SA（技术危机） | DNS 是云服务最易被忽视的全局单点——客户应多区域部署并监控供应商 DNS 健康；DNS 解析降级策略（缓存+备用解析）应是客户架构的必备组件 |

## 参考资料

1. [AWS 的 PES 页面——可查阅该事件 Post-Event Summary](https://aws.amazon.com/premiumsupport/technology/pes/) — E1