---
id: INC-20250416-ZOOM-DOMAIN
title: Zoom 全球服务中断（GoDaddy Registry 误封 zoom.us 域名，DNS 解析失败致网站/会议/App 不可用约 2 小时）
company: Zoom
company_type: internet
domain: saas-platforms
date: 2025-04-16
duration_minutes: 107
severity: SEV-1
impact_scope: global
root_cause_category: dependency-failure
root_cause_tags: [dns, domain-registry, registrar, godaddy, markmonitor, third-party, communication-error, server-block, name-resolution]
status: published
last_updated: 2026-07-29
sources:
  - https://status.zoom.us/
  - https://cybersecuritynews.com/global-zoom-outage/
---

# Zoom 4·16（2025-04-16）：域名注册商误封了 zoom.us，全球会议停摆两小时

## 摘要

2025 年 4 月 16 日太平洋时间约 11:25，全球用户突然无法访问 zoom.us 网站、登录桌面客户端或发起视频会议。Zoom 状态页确认"我们正调查无法访问 zoom.us 与 Zoom 服务的报告"。事后查明根因链：Zoom 的域名注册管理委托给 **Markmonitor**（全球最大的域名管理服务商）；Markmonitor 与 **GoDaddy Registry**（负责运营 .us 顶级域名的注册局）之间的**通信出现错误**，导致 GoDaddy Registry 对 zoom.us 域名**误执行了服务器封禁（server hold）**，该域名的 **NS 记录（名称服务器记录）被从 .us 顶级域区移除**，全球 DNS 解析中断。GoDaddy Registry 在约 13:12 PT 移除该封禁，DNS 缓存逐步恢复，Zoom 服务在约 13:50 PT 全面恢复。该事件是"**第三方依赖链中的通信故障**"的典型案例：Zoom 自身无任何故障，但其域名的一级注册商（Markmonitor）与二级注册局（GoDaddy）之间的信息传递错误，直接导致整个产品线从全球 DNS 中消失。没有 DNS 解析，再可靠的架构也无法被发现——**域名注册链路是互联网服务最容易被忽视的全局单点**。

## 影响评估（CRE 视角）

- **影响面**：zoom.us 全网 DNS 解析失败，导致网站、桌面/移动客户端、会议（包括活跃会议的新参与者加入）、API 全部不可用；全球大量企业与教育机构视频会议中断
- **影响时长**：约 11:25-13:50 PT，核心约 107 分钟（DNS 封禁移除至缓存恢复）
- **次生影响**：正值远程办公与在线教育时段，大量会议中断；Zoom 因"第三方域名商"故障成为全球新闻；暴露了 .us 顶级域的管理风险
- **对外沟通评估**：一般——状态页及时更新，但事后未发布详细技术复盘，根因主要靠第三方报道补全
- **定级依据**：全球全产品线不可用约 2 小时，SEV-1
- 未披露信息：Markmonitor 与 GoDaddy Registry 之间的通信错误具体内容、受影响用户数量

## 时间线（太平洋时间，2025-04-16）

| 时间 | 事件 | 证据 |
|---|---|---|
| ~11:25 | zoom.us 全球 DNS 解析中断，网站/App/会议不可用；用户报告涌入 | E1/E3 |
| 11:25-11:30 | Zoom 状态页确认异常，初步调查中 | E1 |
| 上午-中午 | 定位到根因为 .us 注册局对 zoom.us 执行了 server hold，NS 记录被移除 | E4 |
| 13:12 | GoDaddy Registry 移除 server hold，NS 记录重新发布至 TLD 区 | E4 |
| 13:12-13:50 | DNS 缓存逐步过期，全球解析陆续恢复 | E1 |
| 13:50 | Zoom 确认全面恢复 | E1 |

**关键时间指标**：TTD ≈ 即时 / TTM ≈ 1h47m（从封禁到移除）/ TTR ≈ 2h25m

## 技术细节与根因分析（SRE 视角）

### 背景架构

Zoom 的域名 zoom.us 注册在 .us 顶级域下。域名管理采用三层架构：Zoom → Markmonitor（注册商/域名管理代理）→ GoDaddy Registry（.us 注册局，负责 TLD 区维护）。DNS 解析依赖该链路的正确协作：注册商更新域名状态，注册局将 NS 记录发布到 TLD 区，全球递归 DNS 服务器查询该区获取权威名称服务器地址。

### 因素三分

- **触发因素（Trigger）**：Markmonitor 与 GoDaddy Registry 之间的通信错误（具体细节未公开），导致 GoDaddy 对 zoom.us 执行了 server hold。
- **根本原因（Root Cause）**：域名注册链路（注册商↔注册局）的通信过程缺乏错误检测与防护机制——一方错误解读另一方指令时，注册局直接执行了"域名封禁"这一最高级别的危险动作，且无交叉验证。
- **扩大因素（Aggravating Factors）**：
  1. 域名是互联网服务的入口——DNS 断了解析，所有依赖该域名的服务同时消失；
  2. server hold 是注册局级别的操作，普通 DNS 监控无法提前预警；
  3. 恢复受 DNS 缓存 TTL 限制，即使封禁解除也需要时间等待缓存刷新。
- **减轻因素（Mitigating Factor）**：Zoom 自身服务未受影响；GoDaddy 移除封禁后，未受复杂 TTL 策略过度延迟（约 38 分钟缓存恢复）。

### 5 Whys

```
现象：zoom.us 全球 DNS 解析中断，全产品线不可用 2 小时
Why1 → .us 注册局对 zoom.us 执行了 server hold，NS 记录被移除
Why2 → Markmonitor 与 GoDaddy Registry 之间的通信发生错误
Why3 → 注册局对注册商指令的"错误解读"无交叉验证即执行
Why4 → 域名注册链路的通信协议缺乏防误操作保护
Why5 → 域名注册被作为"基础设施"而非"关键入口"管理，
        注册商↔注册局链路的故障模式未纳入 SRE 依赖分析
        （系统性原因：域名注册链路的可靠性设计落后于互联网的发展）
```

## 解决过程

GoDaddy Registry 在约 13:12 PT 移除 server hold 后，NS 记录重新发布至 .us TLD 区，全球 DNS 解析逐步恢复，Zoom 状态页于 13:50 确认全面恢复。Zoom 未发布详细技术复盘；行业推断的改进方向包括：使用多域名冗余（zoom.us + zoom.com 等）、域名注册状态主动监控、与注册商建立封禁类操作的紧急沟通渠道。

## 经验教训

1. **域名注册链路是"隐形全局单点"**：你的服务架构再可靠，入口域名被人从注册局封禁，一切等于零——域名注册的可靠性分析应纳入 SRE 依赖管理。
2. **三层域名代理链增加事故概率**：Zoom → Markmonitor → GoDaddy Registry，每一层都是通信错误的潜在来源。关键域名应评估跨注册商/跨 TLD 冗余。
3. **server hold 是最高级别的危险动作**：注册局级别的域名封禁应需要多重验证，且应有紧急沟通渠道供受影响方快速申诉解除。
4. **DNS 缓存 TTL 决定了恢复速度的下限**：即使封禁快速解除，全球 DNS 缓存刷新也需要时间——关键业务域名应使用合理的短 TTL（如 5-10 分钟）加速恢复。
5. **第三方依赖的故障是你的事故**：对客户来说，"Zoom 崩了"就是 Zoom 崩了——无论根因在哪个第三方。关键依赖的 SLA 和故障模式必须纳入应急计划。

## 预防与改进措施

- **预防（Prevent）**：多域名冗余（主/备域名）；跨注册商、跨 TLD 注册策略
- **减小爆炸半径（Contain）**：注册商↔注册局链路的通信防误操作协议；紧急解除封禁的 24/7 沟通渠道
- **快速检测（Detect）**：域名注册状态主动监控（WHOIS/NS 记录/域名状态码）；DNS 解析异常的全球探测
- **快速恢复（Recover）**：备域名切换预案；DNS 缓存预刷新机制

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 域名注册链路的可靠性分析必须包含"注册局封禁"等外部操纵场景——这类故障的恢复时间受 DNS 缓存 TTL 和第三方审批流程双重约束 |
| CRE | 客户不会区分"域名被绑"与"产品故障"——关键第三方依赖的 SLI 应当纳入服务拓扑，并设置可观测性预案 |
| FDE | 通信错误发生在两家外部公司之间，取证依赖外部日志（Markmonitor 与 GoDaddy 各自的操作记录）——故障溯源需要跨组织协作 |
| SA（客情危机） | zoom.us 域名被误封 2 小时，全球用户无法开会，企业会议与远程办公中断；Zoom 快速恢复，但'域名=隐形全局单点'的风险令企业客户审视对单一入口的依赖 |
| SA（技术危机） | 域名注册链路是隐形全局单点——客户应评估跨注册商/跨 TLD 冗余（关键域名）；server hold 是最高级别危险动作，域名管理的可靠性分析应纳入依赖管理 |

## 参考资料

1. [Zoom 状态页（官方状态记录）](https://status.zoom.us/) — E1
2. [Global Zoom Outage Caused by Server Block (Cybersecurity News)](https://cybersecuritynews.com/global-zoom-outage/) — E4