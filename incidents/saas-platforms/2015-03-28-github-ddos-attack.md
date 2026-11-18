---
id: INC-20150328-GITHUB-DDOS
title: GitHub 历史上最大规模 DDoS 攻击（中国境内 Baidu 流量被劫持重定向至 GitHub，持续数日）
company: GitHub
company_type: internet
domain: saas-platforms
date: 2015-03-28
duration_minutes: -1
severity: SEV-1
impact_scope: global
root_cause_category: security-attack
root_cause_tags: [ddos, github, baidu, china, traffic-redirection, large-scale, political-motivation, great-firewall, censorship]
status: published
last_updated: 2026-07-29
sources:
  - https://www.cloudflare.com/learning/ddos/famous-ddos-attacks/
  - https://www.bbc.com/news/technology-32138088
  - https://www.theguardian.com/technology/2015/mar/30/github-cleans-up-cyber-attack
---

# GitHub 2015 大 DDoS（2015-03-28）：中国互联网流量被劫持攻击 GitHub，史上最大规模 DDoS 之一

## 摘要

2015 年 3 月下旬，GitHub 遭遇了当时**历史上最大规模的 DDoS 攻击**之一，持续数日。攻击的独特之处在于：攻击流量并非来自僵尸网络，而是来自**中国境内大量互联网用户**——攻击者利用中国"长城防火墙"的流量劫持机制，将原本访问百度（Baidu）广告和分析服务的流量，**重定向至 GitHub**。当中国用户访问包含百度广告/分析代码的网站时，浏览器自动向被劫持的域名发送请求，这些流量汇聚成洪峰冲击 GitHub。GitHub 被迫接入 DDoS 防护服务，包括 Cloudflare 和 Akamai 的保护。该事件是"**反射放大攻击的创新变种**"——攻击者没有直接攻击，而是利用合法流量作为武器。据《纽约时报》报道，安全研究者认为该攻击与**中国政府对 GitHub 上反审查项目的打压**有关。该事件至今仍是云计算/互联网安全史上最具**地缘政治色彩**的 DDoS 攻击案例之一。

## 影响评估（CRE 视角）

- **影响面**：GitHub 网站、API、Git 服务间歇不可用，持续数日
- **影响时长**：持续数日（3 月 28 日前后），具体恢复时间因攻击持续波动而异
- **次生影响**：全球开发者社区受影响；中国互联网流量被用作武器的案例引发国际关注；GitHub 加速采用 DDoS 防护
- **对外沟通评估**：良好——GitHub 在状态页持续更新，但未发布详细技术复盘
- **定级依据**：全球最大代码托管平台持续数日不可用，SEV-1
- 未披露信息：攻击流量精确峰值、受影响用户/仓库数量

## 时间线（UTC，2015-03-28 ~ 03-31）

| 时间 | 事件 | 证据 |
|---|---|---|
| 03-28 起 | GitHub 开始遭受大规模 DDoS 攻击，流量来自中国境内 | E1/E3 |
| 03-28~03-31 | 攻击持续数日，攻击者利用 Baidu 流量劫持扩大攻击 | E1/E3 |
| 03-30 | GitHub 确认遭受"历史上最大规模 DDoS 攻击"之一 | E1 |
| 03-31 | 纽约时报报道：攻击者利用 Baidu 流量劫持攻击 GitHub，疑似与中国打击反审查项目有关 | E1 |
| 后续 | GitHub 接入 Cloudflare/Akamai 等 DDoS 防护，攻击逐步缓解 | E1 |

**关键时间指标**：TTD = 即时 / TTM = 定位到攻击来源 / TTR = 数日

## 技术细节与根因分析（SRE 视角）

### 背景架构

GitHub 运行在自建基础设施上，2015 年时尚未大规模使用专业的 DDoS 防护服务。攻击者利用中国"长城防火墙"的流量劫持机制，将原本访问 Baidu 广告和分析系统的请求重定向至 GitHub 的 IP 地址。

### 因素三分

- **触发因素（Trigger）**：攻击者利用中国 GFW 机制，将 Baidu 的广告/分析流量劫持重定向至 GitHub 的 IP 地址。
- **根本原因（Root Cause）**：GitHub 未部署足够规模的 DDoS 防护基础设施，面对利用中国境内合法流量发起的海量攻击无法承受。
- **扩大因素（Aggravating Factors）**：
  1. 攻击流量来自合法用户（中国普通网民），无法通过 IP 封禁简单处理；
  2. 持续数日，攻击者不断调整策略；
  3. 地缘政治背景使事件复杂化。
- **减轻因素（Mitigating Factor）**：GitHub 接入 Cloudflare 和 Akamai 等专业 DDoS 防护服务后，攻击逐步缓解。

### 5 Whys

```
现象：GitHub 持续数日遭受大规模 DDoS 攻击
Why1 → 攻击流量来自中国境内大量互联网用户
Why2 → 攻击者利用 GFW 劫持 Baidu 流量重定向至 GitHub
Why3 → 攻击者利用合法流量作为"反射放大"武器
Why4 → GitHub 2015 年时 DDoS 防护能力不足
Why5 → 互联网基础设施在面对"合法流量被劫持"的
        攻击模式时，防御难度极高
        （系统性原因：DDoS 防护的"源端溯源"能力不足）
```

## 解决过程

GitHub 接入 Cloudflare 和 Akamai 等专业 DDoS 防护服务，逐步过滤恶意流量，服务恢复。事件后 GitHub 大幅加强了 DDoS 防护基础设施。

## 经验教训

1. **"合法流量"也可以成为 DDoS 武器**：攻击者不需要僵尸网络，只要找到一条"合法流量的重定向路径"，就可以发动大规模攻击。
2. **地缘政治是 DDoS 防护的不可控因素**：当攻击涉及国家级的网络基础设施时，技术防护手段的效果有限——需要外交/法律层面的应对。
3. **DDoS 防护不是可选项，是基础设施**：任何互联网服务都必须假设自己会是 DDoS 攻击的目标——防护能力应在被攻击之前建立。
4. **"反射放大"攻击模式的创新永无止境**：从 DNS 反射到 NTP 放大，再到这次利用 GFW 劫持——防御者必须跟踪攻击模式的创新。

## 预防与改进措施

- **预防（Prevent）**：专业 DDoS 防护服务（Cloudflare/Akamai 等）；流量清洗能力
- **减小爆炸半径（Contain）**：DDoS 防护的分层策略（网络层→应用层）
- **快速检测（Detect）**：异常流量模式的实时告警；攻击溯源能力
- **快速恢复（Recover）**：DDoS 防护的自动切换；备用 IP/域名的快速部署

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | DDoS 攻击的防护不是"加带宽"就能解决的——当攻击利用合法流量时，需要应用层语义分析来区分"正常用户"与"被劫持的流量" |
| CRE | 持续数日的 DDoS 攻击对客户的影响远超短期中断——开发者无法 push 代码，业务无法交付，客户信任的修复需要更长时间 |
| FDE | 攻击者的流量劫持路径的取证需要跨网络域的日志关联——GFW 的劫持规则、Baidu 的 DNS 记录、GitHub 的访问日志，三者时序对齐是关键 |
| SA（客情危机） | GitHub 数日遭受大规模 DDoS，开发者体验受损；GitHub 接入专业防护+行业认知'合法流量也可作武器'，开发者社区理解并推动 DDoS 防护基础设施建设 |
| SA（技术危机） | DDoS 防护不是可选项是基础设施——客户应假设自己会是攻击目标并提前部署清洗能力；地缘政治是 DDoS 的不可控因素，关键业务应有跨地域/跨供应商的备选路径 |

## 参考资料

1. [Famous DDoS attacks - GitHub 2015 (Cloudflare)](https://www.cloudflare.com/learning/ddos/famous-ddos-attacks/) — E1
2. [Evidence links China to GitHub cyber-attack (BBC)](https://www.bbc.com/news/technology-32138088) — E1
3. [GitHub cleans up after cyber-attack (The Guardian)](https://www.theguardian.com/technology/2015/mar/30/github-cleans-up-cyber-attack) — E1