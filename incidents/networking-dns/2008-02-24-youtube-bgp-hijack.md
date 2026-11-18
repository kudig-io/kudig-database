---
id: INC-20080224-YOUTUBE
title: YouTube 全球 BGP 劫持（巴基斯坦电信黑洞路由泄漏）
company: YouTube (Google)
company_type: internet
domain: networking-dns
date: 2008-02-24
duration_minutes: 120
severity: SEV-1
impact_scope: global
root_cause_category: network-routing
root_cause_tags: [bgp, hijack, blackhole-route, route-leak, censorship, more-specific-prefix]
status: published
last_updated: 2026-07-29
sources:
  - https://www.ripe.net/publications/news/youtube-hijacking-a-ripe-ncc-ris-case-study/
  - https://en.wikipedia.org/wiki/YouTube#February_2008_Pakistan_blockage
---

# YouTube 全球劫持（2008-02-24）：一条国内封禁指令如何黑洞了全世界的 YouTube

## 摘要

2008 年 2 月 24 日，巴基斯坦电信（Pakistan Telecom, AS17557）为执行政府对 YouTube 的国内封禁令，在其网络中配置了指向 YouTube 前缀的**黑洞路由**——宣告了比 YouTube 自身路由（208.65.152.0/22）**更具体的 208.65.153.0/24**。这条本应只在国内生效的宣告**未经过滤地泄漏给了上游 PCCW（AS3491）**，并由 PCCW 传播至全球。由于更具体前缀全网优选，**全世界访问 YouTube 的流量在约两分钟内被吸入巴基斯坦电信的黑洞**，YouTube 全球不可达约 **2 小时**。YouTube 先以宣告同样的 /24 对抗、再拆分为两个 /25 争夺选路优先级，最终 PCCW 切断了巴基斯坦电信的会话后全球路由收敛。RIPE NCC 基于 RIS 数据发布了逐分钟的路由传播分析，成为 BGP 劫持研究的经典教材，也再次（在 AS7007 事件 11 年后）证明**上游对客户宣告不过滤**仍是互联网的系统性漏洞。

## 影响评估（CRE 视角）

- **影响面**：YouTube 全球用户不可访问（当时已是全球最大视频网站）
- **影响时长**：主要影响约 2 小时（18:47 UTC 劫持开始，21:01 UTC 前后全面恢复）
- **次生影响**：推动 RPKI/ROA 与 BGP 监测服务的发展；成为"审查外溢为全球故障"的标志案例
- **对外沟通评估**：良好——RIPE NCC 发布了基于 RIS 观测的详细路由级复盘（第三方权威分析）
- **定级依据**：全球核心互联网服务完全不可达 2 小时，SEV-1

## 时间线（UTC，2008-02-24）

| 时间 | 事件 | 证据 |
|---|---|---|
| 18:47 | AS17557 开始宣告 208.65.153.0/24，PCCW 将其传播至全球，YouTube 流量被吸入黑洞 | E2 |
| 18:49 | 劫持路由传播至全球绝大多数骨干，YouTube 全球不可达 | E2 |
| 20:07 | YouTube（AS36561）宣告同一 /24 与劫持路由竞争，部分网络恢复 | E2 |
| 20:18 | YouTube 进一步宣告两个 /25（更具体前缀），夺回更多流量 | E2 |
| 20:51 | AS17557 的宣告 AS 路径被前置修改，影响进一步缩小 | E2 |
| 21:01 | PCCW 切断 AS17557 会话，劫持路由从全球路由表消失，全面恢复 | E2 |

**关键时间指标**：TTD ≈ 2min（全球监测可见）/ TTM ≈ 80min（YouTube 对抗宣告）/ TTR ≈ 2h14min

## 技术细节与根因分析（SRE 视角）

### 背景架构

YouTube 当时以 208.65.152.0/22 宣告其服务地址段。BGP 最长前缀匹配规则下，任意第三方宣告其中的 /24 即可在全网优选。运营商执行政府封禁的常见手段是在域内注入黑洞路由——该手段安全的前提是**路由不出域**。

### 因素三分

- **触发因素（Trigger）**：巴基斯坦电信将封禁用黑洞路由（208.65.153.0/24）宣告进 BGP，且未限制传播范围。
- **根本原因（Root Cause）**：上游 PCCW 未对客户宣告做前缀过滤，将不属于巴基斯坦电信的前缀传播至全球。
- **扩大因素（Aggravating Factors）**：
  1. /24 比 YouTube 的 /22 更具体，全网强制优选；
  2. 无路由源验证（RPKI 尚未部署），任何 AS 都能替任意前缀"代言"；
  3. 周日晚间时段，人工响应链路较慢。

- **减轻因素（Mitigating Factor）**：YouTube 服务本身未故障；BGP 路由在全球收敛后自动恢复，无数据丢失。

### 5 Whys

```
现象：YouTube 全球流量被吸入巴基斯坦电信黑洞约 2 小时
Why1 → 全网优选了 AS17557 宣告的更具体 /24 前缀
Why2 → 域内封禁用黑洞路由被宣告给了上游并全球传播
Why3 → PCCW 对客户会话无前缀过滤（该前缀并不属于其客户）
Why4 → 行业前缀过滤实践依赖各运营商自律，无强制机制
Why5 → BGP 无内建路由源认证，AS7007 之后 11 年系统性漏洞未修复
        （系统性原因：全网安全依赖最薄弱一环的运营纪律）
```

## 解决过程

YouTube/Google NOC 通过 BGP 监测发现劫持后，先宣告相同 /24 与劫持者竞争选路（恢复了部分网络），再拆分宣告两个 /25 以更具体前缀夺回流量；根治依赖 PCCW 定位问题并切断与巴基斯坦电信的 BGP 会话，21:01 UTC 劫持路由撤出全球路由表。RIPE NCC 随后基于 RIS 数据发布逐分钟传播复盘。

## 经验教训

1. **上游前缀过滤缺失的后果由全网承担**：AS7007 的教训 11 年后重演，自律性防护不可靠，需要 RPKI/ROA 这类可验证机制。
2. **封禁/黑洞类域内操作必须有出域防护**（no-export、明确的宣告过滤），否则国内策略会外溢为全球故障。
3. 被劫持方的**紧急对抗手段是宣告更具体前缀**——服务方应预留 /24 级宣告预案（更细粒度当时不可全球传播）。
4. 独立的路由监测（RIS/RouteViews 及商业 BGP 监控）是劫持检测的生命线，TTD 决定损失规模。

## 预防与改进措施

- **预防（Prevent）**：上游客户会话前缀过滤 + IRR/RPKI 路由源验证；域内黑洞路由强制 no-export
- **减小爆炸半径（Contain）**：关键前缀的更具体宣告预案
- **快速检测（Detect）**：关键前缀的第三方 BGP 劫持监控告警
- **快速恢复（Recover）**：与上游/对等网络的紧急联络通道（本次根治靠 PCCW 断会话）

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 域内策略操作外溢为全球故障的原型；对抗性恢复（前缀竞争）作为止血手段首次大规模实战 |
| CRE | 用户看到的是"YouTube 挂了"，实际是第三方国家级网络的操作——服务方需要对不可控依赖的沟通话术 |
| FDE | RIPE RIS 的逐分钟路由传播重建（18:47/20:07/20:18/21:01）是路由取证的黄金标准范本 |
| SA（客情危机） | YouTube 全球中断约 2 小时，用户感知明显；Google 紧急对抗宣告+行业推动 RPKI 体系，事件成为'国内策略外溢为全球故障'的经典教材 |
| SA（技术危机） | 被劫持方应预留 /24 级对抗宣告预案——客户应关注 BGP 安全（RPKI/ROA 部署）；封禁/黑洞类域内操作必须有出域防护（no-export），防止单域策略外溢 |

## 参考资料

1. [YouTube Hijacking: A RIPE NCC RIS case study](https://www.ripe.net/publications/news/youtube-hijacking-a-ripe-ncc-ris-case-study/) — E2
2. [YouTube February 2008 Pakistan blockage (Wikipedia)](https://en.wikipedia.org/wiki/YouTube#February_2008_Pakistan_blockage) — E4
