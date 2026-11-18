---
id: INC-20130222-AZURE-SSL
title: Azure 全球存储中断（HTTPS 证书过期）
company: Microsoft Azure
company_type: cloud-native
domain: cloud-infrastructure
date: 2013-02-22
duration_minutes: 720
severity: SEV-1
impact_scope: global
root_cause_category: operational-safeguard
root_cause_tags: [ssl-certificate, certificate-expiry, secret-management, storage, dependency-failure, automation-gap]
status: published
last_updated: 2026-07-29
sources:
  - https://azure.microsoft.com/en-us/blog/details-of-the-february-22nd-2013-windows-azure-storage-disruption/
  - https://www.zdnet.com/article/microsoft-azure-storage-issue-hits-customers-worldwide/
---

# Azure 证书过期（2013-02-22）：一张忘记续期的证书放倒全球云存储

## 摘要

2013 年 2 月 22 日 12:44 PST，Windows Azure（现 Microsoft Azure）**全球所有区域**的存储服务 HTTPS 访问同时中断——原因简单得令人错愕：**Azure Storage 使用的 SSL 证书过期了**。所有通过 HTTPS 访问 Blob、Table、Queue 存储的请求立即失败，而 Azure 平台自身大量服务（虚拟机、管理门户、服务总线等）依赖存储服务，故障迅速级联为**平台级全球中断**，波及全球 52 个数据中心的客户。核心 HTTPS 中断持续约 **12 小时**，部分级联服务的完全恢复超过 24 小时。根因是证书续期依赖**人工流程**：新证书已经准备好，却没有在到期前完成部署——续期提醒被淹没、部署排期错过了到期日。微软事后公开道歉、赔偿客户，并将证书管理纳入自动化监控与部署体系。该事件与 2021 年 2 月（同样因证书过期导致 Teams/Azure AD 中断）遥相呼应，是"**最低级的原因造成最高级的故障**"的永恒警示：全球分布式系统的可用性可以毁于一个日历事件。

## 影响评估（CRE 视角）

- **影响面**：全球所有 Azure 区域的存储 HTTPS 访问；级联影响虚拟机、管理门户等依赖存储的平台服务，波及全球客户
- **影响时长**：核心 HTTPS 中断约 12 小时；级联服务完全恢复超 24 小时
- **次生影响**：正值 Azure 与 AWS 竞争关键期，企业客户信任受挫；催动全行业证书生命周期自动化（证书监控成为 SRE 标配检查项）
- **对外沟通评估**：良好——官方博客发布详细复盘并向受影响客户提供赔偿
- **定级依据**：全球全区域核心服务中断，SEV-1
- 未披露信息：受影响请求量、具体客户数

## 时间线（太平洋时间，2013-02-22 起）

| 时间 | 事件 | 证据 |
|---|---|---|
| 事前数周 | 新 SSL 证书已获取，续期部署被排入常规发布计划——晚于证书到期日 | E1 |
| 02-22 12:44 | 证书到期，全球存储 HTTPS 请求开始批量失败 | E1 |
| 12:44+ | 依赖存储的平台服务（VM、门户等）级联异常，全球客户报障 | E1/E3 |
| 下午 | 微软确认根因为证书过期，启动紧急证书部署——需向全球海量存储前端安全分发 | E1 |
| ~00:30 (02-23) | 存储 HTTPS 恢复（约 12 小时） | E1/E3 |
| 02-23~24 | 级联受影响的服务逐步完全恢复 | E1 |

**关键时间指标**：TTD ≈ 分钟级 / 根因确认 ≈ 1h 内 / TTR ≈ 12h（核心），24h+（级联）

## 技术细节与根因分析（SRE 视角）

### 背景架构

Azure Storage 为全球所有区域提供统一的 HTTPS 端点证书体系；证书续期当时属于**人工管理流程**（获取→排期→随发布部署）。Azure 平台服务（VM 磁盘、门户、消息）在架构上强依赖存储服务，存储是事实上的平台级单点依赖。

### 因素三分

- **触发因素（Trigger）**：SSL 证书在 12:44 PST 到期，HTTPS 握手全部失败。
- **根本原因（Root Cause）**：证书生命周期管理依赖人工——虽然新证书已获取，但部署被排入到期日之后的发布窗口，且无强制到期前部署的拦截机制。
- **扩大因素（Aggravating Factors）**：
  1. 全球所有区域共享同一证书体系，无区域隔离，到期即全球同时失效；
  2. 平台服务对存储的强依赖使故障级联放大为平台级；
  3. 紧急证书部署要覆盖全球海量前端节点，恢复本身耗时 12 小时。

- **减轻因素（Mitigating Factor）**：Azure 存储的数据持久性未受影响；证书更新后服务快速恢复（约 12 小时）。

### 5 Whys

```
现象：全球 Azure 存储 HTTPS 中断 12 小时，平台服务级联故障
Why1 → 存储服务的 SSL 证书到期，HTTPS 握手失败
Why2 → 新证书已备好但未在到期前部署
Why3 → 证书续期部署走常规发布排期，与到期日之间无强制校验
Why4 → 证书生命周期无自动化监控（到期倒计时告警/自动轮转）
Why5 → 证书被当作"配置杂项"而非"全球可用性关键依赖"管理
        （系统性原因：关键依赖的运维等级与其爆炸半径不匹配）
```

## 解决过程

微软在故障后快速确认证书过期，随即启动紧急部署——但向全球存储前端分发新证书本身是大规模操作，加上验证与逐区放量，核心 HTTPS 服务约 12 小时后恢复，级联服务继续收敛超过一天。事后微软公开复盘并赔偿，整改包括：证书到期自动化监控与告警、证书部署与到期日的强制校验闸门、平台服务对存储依赖的韧性改进。

## 经验教训

1. **证书/密钥/域名的到期是"确定会发生的故障"**：必须自动监控（多级提前告警）+ 自动轮转，人工流程在规模化面前必然失误。
2. **全球共享的安全材料 = 全球同时失效**：证书应按区域/服务分片，同一到期日的证书是自相关故障源。
3. **平台内部依赖图决定级联半径**：存储这类底座服务的任何故障模式都会被整个平台继承——底座服务需要最高等级的运维防护。
4. **恢复路径要预演**：紧急证书全球分发耗时 12 小时，说明"修复手段的执行时间"本身要纳入 RTO 设计。
5. 同类事故反复发生（微软 2021 Teams 证书过期、Ericsson 2018 全球移动网证书过期）——**清单化+自动化是唯一解**。

## 预防与改进措施

- **预防（Prevent）**：证书生命周期自动化（发现、告警、轮转）；部署与到期日强制校验
- **减小爆炸半径（Contain）**：证书按区域/服务分片，错开到期日
- **快速检测（Detect）**：证书到期倒计时多级告警（90/30/7/1 天）；HTTPS 握手失败率秒级监控
- **快速恢复（Recover）**：紧急证书分发通道的容量与时效预演

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | "确定性故障"（到期日）竟成全球事故——运维自动化清单里证书监控从此是第一页 |
| CRE | 客户无法接受"忘记续证书"级别的原因，沟通中根因的"低级感"比故障本身更伤信任 |
| FDE | 取证极简（到期时间戳=故障开始时间戳），但恢复耗时暴露的分发瓶颈才是深层发现 |
| SA（客情危机） | 全球存储 HTTPS 同时中断，'巨头也会犯低级错误'令客户震惊；微软快速修复，但'全球共享安全材料=全球同时失效'的风险令企业客户重新审视平台依赖 |
| SA（技术危机） | 证书/密钥到期是'确定会发生的故障'——客户应自建多级提前告警与自动轮转；平台内部依赖图决定级联半径，底座服务故障会被整个平台继承 |

## 参考资料

1. [Details of the February 22nd 2013 Windows Azure Storage Disruption (Azure 官方复盘)](https://azure.microsoft.com/en-us/blog/details-of-the-february-22nd-2013-windows-azure-storage-disruption/) — E1
2. [Microsoft Azure storage issue hits customers worldwide (ZDNet)](https://www.zdnet.com/article/microsoft-azure-storage-issue-hits-customers-worldwide/) — E3
