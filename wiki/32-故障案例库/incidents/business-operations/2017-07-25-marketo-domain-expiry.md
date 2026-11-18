---
id: INC-20170725-MARKETO-DOMAIN
title: Marketo 全球服务中断（marketo.com 域名忘记续费而过期，网站/客户控制台/API 全线下线数小时）
company: Marketo
company_type: internet
domain: business-operations
date: 2017-07-25
duration_minutes: -1
severity: SEV-1
impact_scope: global
root_cause_category: operational-safeguard
root_cause_tags: [domain-expiry, renewal-failure, asset-lifecycle, dns, operational-process, no-monitoring]
status: published
last_updated: 2026-08-25
sources:
  - https://www.thousandeyes.com/blog/what-happened-when-marketos-domain-name-expired
  - https://www.itnews.com.au/news/marketo-suffers-major-outage-after-domain-renewal-fail-469460
  - https://www.cmswire.com/marketing-automation/marketo-outage-caused-by-failure-to-renew-its-domain/
---

# Marketo 7·25（2017-07-25）：没续费的域名，让整个 SaaS 从互联网上消失

## 摘要

2017 年 7 月 25 日凌晨 4:25（太平洋时间），全球营销自动化平台 Marketo 的服务开始大规模中断——**marketo.com 域名因未续费而过期**，注册商 Network Solutions 将域名名称服务器切到 `ns1.pendingrenewaldeletion.com` 等过期处理服务器，全球 DNS 解析随之失效。网站、客户控制台、API、邮件链接全部不可用，依赖 Marketo 做营销自动化的企业客户（包括其自身官网与 Salesforce 集成的下游）在数小时内无法触达任何 Marketo 资产。Marketo 官方随后公开道歉并确认根因为"未能及时续费域名"。这是"**关键数字资产生命周期管理失误**"的教科书案例：没有任何技术故障，仅仅因为一个到期未续费的域名，一个年营收数亿美元的 SaaS 公司就从互联网上"消失"了。

## 影响评估（CRE 视角）

- **影响面**：marketo.com 全球 DNS 解析失败，官网、客户控制台（登录/使用产品）、API 端点、邮件中的链接全部不可用；受影响的是全部 Marketo 客户（全球数千家企业营销团队）
- **功能维度**：营销自动化全流程受损——无法登录控制台、无法运行 Campaign、落地页与邮件链接全部 404/DNS 失败；部分客户的官网也嵌入了 Marketo 域名资源，出现连锁不可用
- **影响时长**：官方未披露精确恢复时刻；第三方观测（ThousandEyes）确认中断从 04:25 PT 开始，持续数小时（客户感知窗口）
- **错误预算**：按 99.9% SLO 估算，数小时中断已耗尽单月错误预算
- **次生影响**：营销团队当日营销活动全面停摆；行业对"SaaS 公司的域名管理"投以空前关注；后续 Marketo 被 Adobe 收购时，该事件仍是客户信任讨论中的经典反例
- **对外沟通评估**：一般——官方致歉并确认根因，但未发布详细技术复盘；细节由 ThousandEyes 等第三方观测补全
- **定级依据**：全球全产品线不可用数小时，SEV-1
- 未披露信息：精确恢复时间、受影响客户数量、域名为何未自动续费（支付方式失效/联系人邮箱无人看管，均为行业推断）

## 时间线（太平洋时间，2017-07-25）

| 时间 | 事件 | 证据 |
|---|---|---|
| 04:25 | marketo.com 过期，Network Solutions 切换 NS 至 pendingrenewaldeletion 服务器，全球 DNS 解析开始失败 | E2 |
| 04:25-早晨 | 用户报告无法访问 Marketo 官网/控制台；ThousandEyes 全球探测确认解析失败 | E2/E3 |
| 上午 | Marketo 紧急续费域名、协调注册商恢复 NS 记录 | E3 |
| 数小时后 | 域名恢复有效状态，DNS 缓存逐步过期，服务陆续恢复 | E2/E3 |
| 7 月 26 日 | Marketo 官方确认根因为域名未续费并致歉 | E1 |

**关键时间指标**：TTD ≈ 依赖外部探测发现 / TTM ≈ 数小时（域名续费+NS 恢复）/ TTR ≈ 数小时（受 DNS 缓存 TTL 拖尾）

## 技术细节与根因分析（SRE 视角）

### 背景架构

Marketo 是纯 SaaS 架构，其全部数字入口（官网、产品控制台、API、营销邮件中的 CTA 链接）都挂在 `marketo.com` 这个单一域名之下。域名由注册商 Network Solutions 托管。SaaS 的"店面"与"产品"共用同一域名，意味着该域名既是营销资产也是生产入口——它的失效等价于整个产品线从互联网消失。

### 因素三分

- **触发因素（Trigger）**：marketo.com 到期且未在宽限期内续费，注册商按流程将域名转入过期处理状态（pending renewal deletion），NS 记录被替换。
- **根本原因（Root Cause）**：关键数字资产（域名）的**生命周期管理缺少防护机制**——没有自动化续费、没有到期预警、没有"过期前 N 天"的人工确认流程；续费这个低频高危操作完全依赖人工记忆，一次遗漏即全站失联。
- **扩大因素（Aggravating Factors）**：
  1. 所有入口（官网/控制台/API/邮件）共用单一域名，爆炸半径 = 100% 产品线；
  2. 注册商在过期后自动切换 NS 至挂起服务器，且无任何客户侧确认环节；
  3. 即使续费成功，全球 DNS 缓存刷新仍需时间，恢复被 TTL 拖尾。
- **减轻因素（Mitigating Factor）**：域名在赎回期内找回，未进入公开拍卖/被抢注；Marketo 域名注册信息完整，得以快速赎回。

### 5 Whys

```
现象：marketo.com 全球解析失败，全产品线不可用数小时
Why1 → 域名未续费而过期，NS 被注册商切换
Why2 → 续费环节无人执行，到期前无任何拦截
Why3 → 缺少自动化续费与到期预警机制
Why4 → 域名被视为"一次买断的商务资产"而非"需要运维的生产依赖"
Why5 → 数字资产的运维化意识缺失：域名/证书等"到期型资产"
        未纳入 SRE 依赖管理与监控体系（系统性原因）
```

## 解决过程

Marketo 在发现中断后紧急联系注册商 Network Solutions 完成域名续费（域名处于赎回期，需缴纳赎回费），恢复 NS 记录后，全球 DNS 缓存逐步刷新，服务陆续恢复。官方随后公开道歉，但未披露后续改进措施；行业普遍建议的方向包括：开启自动续费、多域名冗余、域名到期监控（WHOIS 探测）、注册商锁定（防止未授权转移）。

## 经验教训

1. **域名是"到期型生产依赖"**：当 <域名/证书等资产承载全部业务入口> 时，必须 <将其纳入 SRE 资产清单并建立到期前自动化预警与双人确认>，否则 <一次遗漏即可让全产品线从互联网消失>。
2. **单一域名 = 100% 爆炸半径**：当 <官网、控制台、API、邮件共用同一域名> 时，应当 <评估备用域名/独立入口域名>，否则 <域名的商务性失效会同时摧毁营销资产与生产入口>。
3. **低频高危操作必须制度化**：当 <续费/迁移/下线等操作每年只发生几次> 时，必须 <用自动化 + 日历提醒 + 双人复核取代人工记忆>，否则 <遗漏概率随业务繁忙度上升>。
4. **DNS 恢复永远有 TTL 拖尾**：当 <域名失效导致中断> 时，恢复速度受全球缓存刷新限制——关键域名应 <使用合理短 TTL> 并 <预先演练过期场景>。
5. **注册商是隐形的第三方依赖**：当 <域名托管于单一注册商> 时，应 <评估注册商锁定、多注册商备份、WHOIS 状态监控>，否则 <注册商的流程瑕疵（如自动转挂起）会直接成为故障放大器>。

## 预防与改进措施

- **预防（Prevent）**：域名自动续费 + 提前 90/30/7 天三级到期预警；注册商账号双人管理、支付方式有效性季度核验
- **减小爆炸半径（Contain）**：关键业务域名与营销域名分离；跨注册商/跨 TLD 冗余解析入口
- **快速检测（Detect）**：WHOIS/域名状态码/NS 记录主动探测（外部监控优先——自己挂了就收不到告警）；DNS 解析全球探测
- **快速恢复（Recover）**：预置"域名过期"应急 Runbook（联系注册商、赎回流程、DNS 预刷新）；备域名切换预案

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | "到期型资产"（域名/证书/备案）必须纳入 SRE 依赖清单并自动化监控——技术再可靠，商务资产失效等于全站失联 |
| CRE | 客户感知的是"Marketo 崩了"，不会关心是技术故障还是没交续费费——SaaS 的一切资产失效都是客户故障 |
| FDE | 内部监控全部随域名一起失联，取证依赖外部探测（ThousandEyes 等第三方网络观测）——关键资产的外部可观测性是盲区救星 |
| SA（客情危机） | 全球企业客户数小时无法运行营销活动，Marketo 公开道歉仍难以挽回'连域名都管不好'的信任损伤；对后续 Adobe 收购谈判中的客户信心也是负面筹码 |
| SA（技术危机） | 客户的营销入口全部挂在一个域名上本身就是架构风险——SA 应推动客户将域名视为生产资产：自动续费、WHOIS 监控、备域名、短 TTL 四件套 |

## 参考资料

1. [What Happened When Marketo's Domain Name Expired（ThousandEyes，E2/E3 外部观测）](https://www.thousandeyes.com/blog/what-happened-when-marketos-domain-name-expired)
2. [Marketo suffers major outage after domain renewal fail（iTnews）](https://www.itnews.com.au/news/marketo-suffers-major-outage-after-domain-renewal-fail-469460)
3. [Marketo Outage Caused by Failure to Renew Its Domain（CMSWire）](https://www.cmswire.com/marketing-automation/marketo-outage-caused-by-failure-to-renew-its-domain/)
