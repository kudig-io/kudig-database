---
id: INC-20200717-CLOUDFLARE-BB
title: Cloudflare 骨干网中断（骨干网络配置错误导致 27 分钟全球中断，Discord/Shopify 等连带受影响）
company: Cloudflare
company_type: cloud-native
domain: networking-dns
date: 2020-07-17
duration_minutes: 27
severity: SEV-1
impact_scope: global
root_cause_category: config-error
root_cause_tags: [backbone, configuration-error, bgp, routing, discord, shopify, politico, traffic-routing, maintenance]
status: published
last_updated: 2026-07-29
sources:
  - https://blog.cloudflare.com/cloudflare-outage-on-july-17-2020/
  - https://techcrunch.com/2020/07/17/cloudflare-dns-goes-down-taking-a-large-piece-of-the-internet-with-it/
---

# Cloudflare 7·17（2020-07-17）：骨干网配置错误，27 分钟让大半个互联网"消失"

## 摘要

2020 年 7 月 17 日约 14:15 PT，Cloudflare 的**骨干网络（backbone network）**因一次**配置错误（configuration error）**导致全球服务中断约 27 分钟，影响 Discord、Shopify、Politico 等大量依赖 Cloudflare 的网站与服务。Cloudflare 事后发布详细复盘确认根因：在骨干网络维护过程中，一次**路由配置变更**因错误部署，导致全球流量路由异常，Cloudflare 的 IP 前缀从全球互联网中部分 withdraw。工程师在 27 分钟内完成定位并回滚配置。该事件的独特之处在于：**骨干网络**（用于连接 Cloudflare 全球各数据中心的高速网络）的配置错误直接影响了所有通过 Cloudflare 接入的流量——与 CDN/边缘代理的配置错误不同，骨干网的问题更像"互联网的脊椎出问题"——范围更广，修复更复杂。这是 Cloudflare 在 2019 年 WAF 中断后又一次重大配置错误类故障。

## 影响评估（CRE 视角）

- **影响面**：全球 Cloudflare 骨干网络中断约 27 分钟，影响 Discord、Shopify、Politico 等大量网站与服务
- **影响时长**：约 14:15-14:42 PT，约 27 分钟
- **次生影响**：Discord 游戏通讯中断，Shopify 电商商家无法收款，Politico 新闻网站不可访问
- **对外沟通评估**：优秀——Cloudflare 在数小时内发布详细复盘，含时间线、根因与改进措施
- **定级依据**：全球骨干网络中断，影响大量客户服务，SEV-1
- 未披露信息：受影响流量百分比、具体错误的配置项

## 时间线（太平洋时间，2020-07-17）

| 时间 | 事件 | 证据 |
|---|---|---|
| ~14:15 | 骨干网络维护中的路由配置错误部署，全球流量路由异常 | E1 |
| 14:15-14:20 | 大量网站报障，Discord/Shopify 等不可用 | E1/E3 |
| 14:20-14:40 | 工程师定位到配置错误，回滚至上一版本 | E1 |
| ~14:42 | 配置回滚完成，全球服务恢复 | E1 |
| 07-17 后续 | Cloudflare 发布详细技术复盘 | E1 |

**关键时间指标**：TTD = 即时 / TTM = 定位配置错误 / TTR ≈ 27min

## 技术细节与根因分析（SRE 视角）

### 背景架构

Cloudflare 的骨干网络（backbone）是连接全球各数据中心的高速私有网络，所有客户流量通过骨干网在数据中心之间路由。骨干网的路由配置由全局策略管理，变更需经过验证与灰度发布流程。

### 因素三分

- **触发因素（Trigger）**：骨干网络维护中的路由配置变更包含错误，部署后导致全球流量路由异常。
- **根本原因（Root Cause）**：配置错误——变更部署前未完全验证，错误配置进入生产环境，导致骨干网路由表异常。
- **扩大因素（Aggravating Factors）**：
  1. 骨干网是全局共享基础网络，影响所有客户流量；
  2. 路由配置错误难以快速本地化，恢复需要全局回滚。
- **减轻因素（Mitigating Factor）**：快速回滚（27 分钟），配置系统支持快速回退至上一版本。

### 5 Whys

```
现象：Cloudflare 骨干网全球中断 27 分钟
Why1 → 骨干网路由配置错误部署至生产
Why2 → 维护变更的验证流程未发现配置错误
Why3 → 配置变更的灰度发布步骤不足
Why4 → 骨干网配置变更的测试覆盖未覆盖该场景
Why5 → 骨干网作为"基础设施的基础设施"，
        其变更管理需要比普通服务更高的安全标准
        （系统性原因：骨干网变更管理的安全标准仍需提升）
```

## 解决过程

工程师定位到配置错误后，回滚至上一版本配置，骨干网在 27 分钟内恢复。Cloudflare 事后加强了骨干网配置变更的验证与灰度发布流程。

## 经验教训

1. **骨干网是"基础设施的基础设施"**：骨干网的配置错误影响所有客户——比 CDN 配置错误更严重，因为骨干网是"所有流量"的底层通道。
2. **27 分钟的恢复时间得益于快速回滚**：配置系统的回滚能力是限制 MTTR 的关键——"能否快速回滚"比"配置是否正确"更重要。
3. **骨干网的变更管理需要比普通服务更高的标准**：全局基础网络的变更，应经过更严格的验证、更细粒度的灰度、更自动化的回滚。
4. **Cloudflare 2019-2020 年的配置错误类故障链**：2019 年 WAF → 2020 年骨干网 → 后续多次——配置错误成为 Cloudflare 最频繁的故障模式。

## 预防与改进措施

- **预防（Prevent）**：骨干网配置变更的自动化验证与预检查；灰度发布流程加固
- **减小爆炸半径（Contain）**：骨干网配置变更的分区域灰度发布
- **快速检测（Detect）**：骨干网路由状态的实时监控；异常配置的自动检测告警
- **快速恢复（Recover）**：上一版本配置的一键回滚；骨干网变更的自动化回滚机制

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 骨干网是"所有流量"的底层通道——其配置变更的风险等级是所有服务中最高的；操作骨干网应像操作核反应堆一样谨慎 |
| CRE | 客户无法区分"CDN 故障"与"骨干网故障"——对他们来说，网站打不开就是 Cloudflare 的问题；快速恢复是信任修复的关键 |
| FDE | 骨干网配置错误的取证依赖配置变更日志与路由表日志的关联——"变更了什么→路由如何变化→流量如何受影响"的完整链路 |
| SA（客情危机） | 27 分钟全球中断，Discord/Shopify 等客户业务受损；Cloudflare 快速发布复盘，沟通获认可，但'骨干网=所有流量的底层通道'令客户对 CDN 单点依赖加深警惕 |
| SA（技术危机） | 骨干网是基础设施的基础设施——客户应关注供应商骨干架构冗余与变更管理标准；'能否快速回滚'比'配置是否正确'更重要，客户应评估供应商的回滚能力 |

## 参考资料

1. [Cloudflare outage on July 17, 2020（官方复盘）](https://blog.cloudflare.com/cloudflare-outage-on-july-17-2020/) — E1
2. [Cloudflare outage takes down Discord, Shopify, Politico (TechCrunch)](https://techcrunch.com/2020/07/17/cloudflare-dns-goes-down-taking-a-large-piece-of-the-internet-with-it/) — E1