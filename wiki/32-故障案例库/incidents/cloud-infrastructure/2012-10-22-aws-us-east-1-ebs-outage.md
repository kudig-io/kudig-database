---
id: INC-20121022-AWS-USEAST
title: AWS us-east-1 EBS 大规模中断（运维数据采集代理 latent bug 导致 EBS 存储服务器大面积故障）
company: AWS
company_type: cloud-native
domain: cloud-infrastructure
date: 2012-10-22
duration_minutes: -1
severity: SEV-1
impact_scope: single-region
root_cause_category: software-bug
root_cause_tags: [ebs, us-east-1, latent-bug, data-collection-agent, cascading-failure, ec2, netflix, monitoring]
status: published
last_updated: 2026-07-29
sources:
  - https://aws.amazon.com/message/680342/
  - https://techblog.netflix.com/2012/10/post-mortem-of-10222012-aws-degradation.html
---

# AWS us-east-1 10·22（2012-10-22）：一个潜伏的数据采集 bug，让 EBS 大面积瘫痪

## 摘要

2012 年 10 月 22 日，AWS us-east-1 区域经历了一次大规模服务降级，核心原因是 **EBS（弹性块存储）存储服务器**上的一个**运维数据采集代理（operational data collection agent）存在潜伏 bug**。该代理在特定条件下触发，导致 EBS 存储服务器进入高负载状态，大量 EBS 卷不可用，并级联影响 EC2 及其他依赖 EBS 的云服务。该事件波及大量知名网站与服务，包括 Reddit、Netflix、Pinterest、Foursquare 等。Netflix 甚至在 AWS 发布官方复盘之前就发布了**自己的复盘**，详细说明了 Netflix 如何受到该事件影响以及其 Chaos Monkey 和故障转移机制的表现。这是云计算史上著名的"**第三方运维代理触发 EBS 风暴**"案例，与 2011 年 4 月的 EBS re-mirroring 风暴并列为 AWS 早期的两次 EBS 重大事故。

## 影响评估（CRE 视角）

- **影响面**：us-east-1 区域 EBS 存储大面积不可用，级联影响 EC2、Redshift 等依赖 EBS 的服务；大量知名网站（Reddit、Netflix、Pinterest、Foursquare）受影响
- **影响时长**：无统一恢复口径（因服务类型与 EBS 卷状态而异）
- **次生影响**：Netflix 发布独立复盘，其 Chaos Monkey 自动故障转移使部分客户免受影响；AWS 可靠性再次受到行业审视
- **对外沟通评估**：良好——AWS 发布官方 PES（Post-Event Summary），确认根因为运维数据采集代理的 latent bug
- **定级依据**：核心存储服务大面积不可用，影响大量知名客户，SEV-1
- 未披露信息：受影响 EBS 卷精确数量、恢复时间精确值

## 时间线（UTC，2012-10-22）

| 时间 | 事件 | 证据 |
|---|---|---|
| 10-22 白天 | 运维数据采集代理的 latent bug 被触发，EBS 存储服务器进入高负载状态 | E1 |
| 10-22 | EBS 卷大面积不可用，级联影响 EC2 等；Reddit/Netflix/Pinterest 等报告中断 | E1/E3 |
| 10-22 | 工程师定位到运维代理 bug 并修复，逐步恢复 | E1 |
| 10-22 后续 | AWS 发布官方 PES；Netflix 发布独立复盘 | E1/E3 |

**关键时间指标**：TTD = 即时 / TTM = 定位运维代理 bug / TTR = 因服务而异

## 技术细节与根因分析（SRE 视角）

### 背景架构

EBS（弹性块存储）是 AWS 的核心存储服务，为 EC2 实例提供持久化块存储。EBS 存储服务器上运行运维数据采集代理，用于收集服务器的性能指标与状态数据供监控系统使用。

### 因素三分

- **触发因素（Trigger）**：运维数据采集代理的 latent bug 在特定条件下触发（具体触发条件未公开），导致代理大量消耗存储服务器资源。
- **根本原因（Root Cause）**：运维数据采集代理存在软件 bug，在特定负载/状态下导致存储服务器进入高负载甚至不可用状态。
- **扩大因素（Aggravating Factors）**：
  1. EBS 是 EC2 的底层依赖，EBS 故障直接导致 EC2 实例不可用；
  2. 多可用区同时受影响，跨可用区冗余失效；
  3. 运维代理本身的设计目的是"提高可观测性"，却成为故障源。
- **减轻因素（Mitigating Factor）**：Netflix 等客户通过 Chaos Monkey 验证的跨可用区部署自动转移，部分减少了影响。

### 5 Whys

```
现象：us-east-1 区域 EBS 大面积不可用，知名网站中断
Why1 → 运维数据采集代理消耗大量资源，EBS 存储服务器不可用
Why2 → 运维代理存在 latent bug，在特定条件下触发
Why3 → 运维代理的测试覆盖未覆盖该触发条件
Why4 → 运维代理作为"监控组件"的可靠性要求低于生产组件
Why5 → "监控/运维组件"的可靠性被低估——它们拥有
        生产级别权限，但按开发测试级别管理
        （系统性原因：运维组件的可靠性管理标准低于生产组件）
```

## 解决过程

工程师定位到运维数据采集代理的 bug 并修复，逐步恢复 EBS 服务。AWS 事后加强了运维组件的可靠性测试与变更管理流程。

## 经验教训

1. **"运维/监控组件"拥有生产级权限，但可靠性标准往往低于生产组件**：数据采集代理、监控系统、日志收集器等运维组件，因其"非面向用户"的属性，容易被忽视——但它们拥有与生产组件相同的访问权限，且故障时直接影响生产。
2. **EBS 是 AWS 生态的"关键依赖"**：EBS 故障时，EC2、Redshift、RDS 等大量服务同时受影响——底层存储的可靠性投入应远超其他组件。
3. **客户自己的容灾设计可以兜底**：Netflix 的 Chaos Monkey 和跨可用区部署使其受影响较小——客户侧的容灾设计是云服务不可靠时的最后防线。
4. **latent bug 可能在系统中潜伏数年**：运维代理的 bug 可能在系统中存在很长时间，直到特定条件触发——"混沌工程"可以帮助发现这类潜伏 bug。

## 预防与改进措施

- **预防（Prevent）**：运维组件按生产标准进行测试与变更管理；运维代理的代码审计与测试覆盖强化
- **减小爆炸半径（Contain）**：运维代理的资源使用隔离；跨可用区冗余设计
- **快速检测（Detect）**：运维代理异常行为的实时告警
- **快速恢复（Recover）**：运维代理的快速禁用/回滚机制

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 运维组件是"隐形风险源"——它们拥有 root 级别的访问权限，但可靠性测试标准往往低于面向用户的服务；"监控系统本身需要被监控" |
| CRE | Netflix 的独立复盘是客户侧 Resilience Engineering 的标杆——Chaos Monkey 不是实验，而是生产级的故障演练 |
| FDE | 运维代理的 latent bug 取证需要源代码级别的审计——运行日志可能不包含 bug 触发前的全部状态信息，依赖代码审查补全 |
| SA（客情危机） | EBS 大规模降级连带 EC2/Redshift/RDS，客户对'底层依赖'的认知被刷新；AWS 透明披露，但'运维组件可靠性低于生产组件'的现实令企业客户加强自身监控 |
| SA（技术危机） | EBS 是 AWS 生态的关键依赖——客户应使用跨可用区快照与多区域复制策略；运维/监控组件拥有生产级权限但可靠性标准低，客户应对供应商此短板保持警惕 |

## 参考资料

1. [Summary of the October 22, 2012 AWS Service Event（官方 PES）](https://aws.amazon.com/message/680342/) — E1
2. [Post-mortem of October 22, 2012 AWS degradation (Netflix Tech Blog)](https://techblog.netflix.com/2012/10/post-mortem-of-10222012-aws-degradation.html) — E1