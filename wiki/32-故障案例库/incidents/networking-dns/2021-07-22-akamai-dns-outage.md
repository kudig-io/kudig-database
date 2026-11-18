---
id: INC-20210722-AKAMAI
title: Akamai Edge DNS 全球中断（DNS 软件缺陷致 Steam/PSN/AWS/Reddit/NYT 等全球主流网站瘫痪约 1 小时）
company: Akamai
company_type: cloud-native
domain: networking-dns
date: 2021-07-22
duration_minutes: 60
severity: SEV-1
impact_scope: global
root_cause_category: software-bug
root_cause_tags: [akamai, edge-dns, dns, software-bug, software-update, cascading-failure, third-party-dependency, single-failure-domain]
status: published
last_updated: 2026-08-01
sources:
  - https://www.reuters.com/technology/websites-airlines-banks-tech-companies-down-widespread-outage-2021-07-22/
  - https://www.bleepingcomputer.com/news/security/akamai-dns-global-outage-takes-down-major-websites-online-services/
  - https://www.thousandeyes.com/blog/seven-outages-shook-up-2021
  - https://www.cnbc.com/2021/07/22/many-major-websites-are-offline.html
---

# Akamai 7·22（2021-07-22）：Edge DNS 软件缺陷让半个互联网"找不到路"——CDN 巨头 DNS 服务 1 小时全球瘫痪

## 摘要

2021 年 7 月 22 日，全球最大 CDN 服务商 Akamai 的 **Edge DNS 服务**因一次软件更新中的缺陷发生全球性故障，导致依赖 Akamai DNS 解析的大量网站和在线服务在约 **1 小时**内无法访问——包括 **Steam、PlayStation Network、Newegg、AWS（部分站点）、Amazon、Reddit、纽约时报（NYT）**，以及多家航空公司、银行和证券交易所网站。用户端的表现为 DNS 解析失败（"网站找不到"），而非服务本身宕机——**DNS 是互联网的"寻址系统"，DNS 挂了，一切服务都"失联"**。该事件与 2021-06-08 Fastly 全球中断（49 分钟）、2021-10-04 Facebook BGP 故障并列，构成 2021 年"CDN/DNS 单点依赖"系列事故，再次证明：**依赖单一 DNS/CDN 供应商是互联网公司最大的隐性风险**。

## 影响评估（CRE 视角）

- **影响面**：全球范围内使用 Akamai Edge DNS 的网站无法解析；Steam、PSN、Newegg、Reddit、NYT、AWS 官网等大规模受影响；多国航空公司（如美联航、西南航空）、银行、券商网站一度无法访问
- **影响时长**：约 1 小时（美东时间 7 月 22 日上午，UTC 约 13:00-14:00）
- **次生影响**：依赖上述网站的电商交易、航班值机、游戏登录、云控制台访问中断；新闻机构无法更新内容
- **对外沟通评估**：一般——Akamai 状态页一度无法访问，通过社交媒体/媒体间接沟通；恢复后确认根因为软件缺陷
- **定级依据**：全球性 DNS 基础设施故障，波及银行/航空/交易所等关键行业，SEV-1
- 未披露信息：受影响的域名解析量占比、软件缺陷的详细技术描述

## 时间线（UTC，2021-07-22）

| 时间 | 事件 | 证据 |
|---|---|---|
| ~12:56 | 用户开始报告大量网站无法访问（DNS 解析失败） | E2 |
| ~13:00+ | 影响扩大至 Steam/PSN/Newegg/AWS/Amazon/Reddit/NYT 等 | E1/E2 |
| 13:20 | Akamai 确认 Edge DNS 服务存在问题 | E2 |
| ~14:00 | Akamai 完成修复，服务陆续恢复 | E1 |
| 恢复后 | Akamai 归因于软件更新触发的缺陷（官方未公开详细复盘） | E1/E4 |

**关键时间指标**：TTD = 分钟级 / TTM ≈ 30min / TTR ≈ 60min

## 技术细节与根因分析（SRE 视角）

### 背景架构

Akamai Edge DNS 是 Akamai 的权威 DNS 托管服务：客户把域名解析托管给 Akamai，用户请求解析时由 Akamai 的边缘 DNS 服务器应答。**DNS 是"最先被访问、最容易出问题、最容易被忽略"的基础设施**——网站本身完全正常，但用户找不到 IP 地址就无法访问。由于 DNS 的分布式缓存特性，恢复也会滞后：即使 DNS 服务恢复，下游递归解析器的 TTL 缓存也需要时间刷新。

### 因素三分

- **触发因素（Trigger）**：一次软件更新部署。
- **根本原因（Root Cause）**：软件更新引入的缺陷导致 Edge DNS 服务故障（Reuters： "a bug in the domain name system (DNS) service"）。Akamai 未发布详细公开技术复盘。
- **扩大因素（Aggravating Factors）**：
  1. DNS 是互联网单点依赖——大量客户把域名解析托管给单一供应商；
  2. 影响客户覆盖面极广（游戏/电商/云/航空/金融/媒体）；
  3. DNS 缓存刷新滞后导致恢复时间延长。
- **减轻因素（Mitigating Factor）**：1 小时即恢复；未涉及数据丢失；使用第三方 DNS（如 Google 8.8.8.8）或自带备份解析的用户不受影响。

### 5 Whys

```
现象：全球大量网站 DNS 解析失败约 1 小时
Why1 → Akamai Edge DNS 服务故障，解析请求无法应答
Why2 → 软件更新引入的缺陷导致服务异常
Why3 → 更新未经过充分的回归测试/灰度验证
Why4 → DNS 服务作为关键基础设施，其更新流程保障不足
Why5 → 供应商对"单点依赖"的认知不足——
        客户把整个域名解析押在单一供应商上，供应商的变更管理
        却没有按"单点失效=生态级故障"的级别对待
        （系统性原因：关键基础设施的变更管理标准不足）
```

## 解决过程

Akamai 定位到 Edge DNS 问题后，通过修复软件缺陷恢复服务，约 1 小时后全球解析恢复正常。Akamai 事后确认根因为软件更新缺陷，但**未发布详细的公开技术复盘**，后续主要通过行业分析（ThousandEyes 等）进行事件还原。

## 经验教训

1. **DNS 是"静默的单点"**：网站/服务本身完好，但 DNS 挂了用户就"失联"——"DNS 双供应商/独立备份解析"应是所有关键业务的底线配置。
2. **CDN/DNS 供应商的变更管理 = 客户的安全**：Akamai 的一次软件更新可以瘫痪半个互联网——供应商必须把关键服务的更新按"全球基础设施变更"级别管理（灰度+回滚预案）。
3. **2021 年是"单点依赖"教训年**：6 月 Fastly（CDN 配置）、7 月 Akamai（DNS 软件）、10 月 Facebook（BGP）——三次全球性故障全部源于"单一供应商/单一配置"——"消除单点依赖"成为 2021 年后互联网架构的共识。
4. **DNS TTL 策略影响恢复速度**：过长的 TTL 会在供应商恢复后继续放大故障窗口（缓存过期前持续失败）——关键记录应使用适中 TTL 并做好降级预案。

## 预防与改进措施

- **预防（Prevent）**：关键业务配置多 DNS 供应商（secondary DNS）；供应商侧：软件更新的灰度发布与自动回滚
- **减小爆炸半径（Contain）**：DNS 区域按客户分片承载；更新按区域分批部署
- **快速检测（Detect）**：DNS 解析失败率的外部探测（从不同网络位置持续解析关键域名）；供应商状态页的独立监控
- **快速恢复（Recover）**：预配置的备用解析路径（第三方 DNS 或直接 IP 访问预案）；切换 secondary DNS 的演练

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | DNS/CDN 属于"信任外包"类依赖：供应商的变更管理等级直接决定你的可用性——选择供应商时必须评估其变更管理能力，而非只看功能与价格 |
| CRE | 航空/银行/交易所等关键行业网站同时瘫痪 1 小时——B2C 客户分不清"Akamai 挂了"还是"这家公司挂了"，供应商故障会直接转化为客户对终端品牌的信任损失 |
| FDE | 事件取证必须区分"服务不可用"与"解析不可用"：本案例中下游系统日志全部正常，只有 DNS 层异常——取证时需覆盖"用户视角"（解析失败）与"服务视角"（服务正常）的双重证据链 |
| SA（客情危机） | Steam/PSN/AWS/Reddit/NYT 等全球主流网站瘫痪约 1 小时，'DNS 静默单点'被公众认知；Akamai 快速恢复，但游戏/电商客户的假日高峰受损引发对 DNS 冗余的重视 |
| SA（技术危机） | DNS 双供应商/独立备份解析应是所有关键业务的底线配置——客户应配置备用解析路径并演练切换；供应商的关键服务更新应按'全球基础设施变更'级别管理，客户应关注其灰度能力 |

## 参考资料

1. [Websites back up after brief global outage linked to Akamai（Reuters）](https://www.reuters.com/technology/websites-airlines-banks-tech-companies-down-widespread-outage-2021-07-22/) — E1
2. [Akamai DNS global outage takes down major websites, online services（Bleeping Computer）](https://www.bleepingcomputer.com/news/security/akamai-dns-global-outage-takes-down-major-websites-online-services/) — E2
3. [Seven Outages That Shook Up 2021（ThousandEyes）](https://www.thousandeyes.com/blog/seven-outages-shook-up-2021) — E3
4. [Many major websites are offline（CNBC）](https://www.cnbc.com/2021/07/22/many-major-websites-are-offline.html) — E4
