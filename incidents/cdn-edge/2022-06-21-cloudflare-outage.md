---
id: INC-20220621-CLOUDFLARE
title: Cloudflare 全球中断（BGP 前缀通告策略变更中 term 重排序致 19 个数据中心离线约 75 分钟，50% 全球流量受损）
company: Cloudflare
company_type: cloud-native
domain: cdn-edge
date: 2022-06-21
duration_minutes: 75
severity: SEV-1
impact_scope: global
root_cause_category: config-error
root_cause_tags: [bgp, prefix-advertisement, multi-colo-pop, clos-network, spine-router, config-error, cascading-failure, rollout-stagger, commit-confirm, site-local]
status: published
last_updated: 2026-08-01
sources:
  - https://blog.cloudflare.com/cloudflare-outage-on-june-21-2022/
  - https://www.theregister.com/2022/06/21/cloudflare_outage/
---

# Cloudflare 6·21（2022-06-21）：BGP 策略变更中的 term 重排序，推倒了 19 个数据中心

## 摘要

2022 年 6 月 21 日约 06:27 UTC，Cloudflare 因一次 **BGP 前缀通告策略变更**中的 **term 重排序**，导致 19 个采用 **MCP（Multi-Colo PoP）架构**的数据中心同时离线约 75 分钟。这 19 个数据中心虽仅占全球节点的 4%，却承载了约 **50% 的全球流量**——包括阿姆斯特丹、阿什本、芝加哥、法兰克福、伦敦、东京、新加坡等核心枢纽。根因是：在向 BGP 策略增加信息性 community 时，**`4-ADV-SITE-LOCALS` 和 `6-ADV-SITE-LOCALS` 两个 term 被意外移动到 `REJECT-THE-REST` term 之后**，导致 site-local 前缀被全部撤回，MCP 数据中心自断连接。恢复过程中还出现了"工程师互相覆盖回滚"的混乱，导致问题反复出现。这是 Cloudflare 在 2022 年最严重的一次故障，也是**"BGP 配置变更中 term 顺序敏感性问题"的经典教科书案例**。

## 影响评估（CRE 视角）

- **影响面**：全球 19 个 MCP 数据中心离线，约 50% 的 Cloudflare 流量受损；Fitbit、Peloton 等大量依赖 Cloudflare 的网站与服务不可用
- **影响时长**：约 06:27-07:42 UTC，约 75 分钟
- **次生影响**：MCP 内部负载均衡系统 Multimog 失效，小集群因无法转发而过载；不同区域用户受影响程度差异巨大
- **对外沟通评估**：优秀——Cloudflare 在同日发布详细技术复盘，含时间线、BGP 配置 diff 与改进措施
- **定级依据**：全球 50% 流量受损，核心数据中心离线，SEV-1
- 未披露信息：受影响客户精确数量、经济损失估算

## 时间线（UTC，2022-06-21）

| 时间 | 事件 | 证据 |
|---|---|---|
| 03:56 | 变更部署至非 MCP 数据中心，未发现问题 | E1 |
| 06:17 | 变更部署至 MCP 之外的繁忙数据中心，仍无影响 | E1 |
| 06:27 | 变更部署至 MCP 架构的 spine 路由器，19 个数据中心立即离线 | E1 |
| 06:32 | Cloudflare 内部宣布 P0 事故 | E1 |
| 06:51 | 工程师在路由器上做出第一个变更以验证根因 | E1 |
| 06:58 | 定位根因（BGP term 重排序），开始回滚 | E1 |
| 07:42 | 最后一个数据中心回滚完成，恢复期间因工程师互相覆盖回滚导致问题反复 | E1 |
| 08:00 | 事故关闭 | E1 |

**关键时间指标**：TTD = 5min / TTM = 31min / TTR = 75min

## 技术细节与根因分析（SRE 视角）

### 背景架构

Cloudflare 在过去 18 个月中将其最繁忙的 19 个数据中心升级为 **MCP（Multi-Colo PoP）架构**——一种基于 Clos 网络的更灵活、更可靠的设计。MCP 的关键新增是一层 spine 路由器的 mesh 连接，通过 BGP 前缀通告 policy 管理 site-local 前缀（用于数据中心内部服务器间通信及连接客户源站）。

### 因素三分

- **触发因素（Trigger）**：在标准化 BGP community 标注的变更中，策略语句的 term 被意外重排序——`4-ADV-SITE-LOCALS` 和 `6-ADV-SITE-LOCALS` 从顶部移到了 `REJECT-THE-REST` 之后。
- **根本原因（Root Cause）**：配置变更的 diff 显示 term 被重排序（以 `!` 标记），但审核者未注意到 `REJECT-THE-REST` 是显式拒绝——site-local 前缀被该 term 拒绝后全部撤回。
- **扩大因素（Aggravating Factors）**：
  1. 19 个 MCP 数据中心同时部署，无灰度分批；
  2. site-local 前缀撤回致工程师无法远程访问受影响数据中心（雪上加霜）；
  3. 恢复过程中工程师互相覆盖回滚操作，导致问题反复出现；
  4. 内部负载均衡系统 Multimog 因 site-local 前缀不可用而失效，小集群因流量不均匀而过载。
- **减轻因素（Mitigating Factor）**：备份带外管理流程成功启用，使工程师能重新获得对受影响数据中心的控制。

### 5 Whys

```
现象：19 个 MCP 数据中心同时离线，全球 50% 流量受损
Why1 → BGP 前缀通告 policy 中 site-local 前缀被撤回
Why2 → 配置变更中 term 重排序，site-local 被移到 REJECT-THE-REST 之后
Why3 → 变更 diff 审核未发现 term 顺序变化（"!" 标记被忽略）
Why4 → 变更的灰度部署策略未包含 MCP 数据中心作为独立步骤
Why5 → MCP 数据中心的 "高可用性" 架构引入的新组件（spine 路由器的
        BGP policy）的变更管理标准未相应提升
        （系统性原因：新架构的运维变更流程未同步升级）
```

## 解决过程

工程师定位到 BGP term 重排序后，开始回滚配置变更。但恢复过程中多个工程师同时操作，互相覆盖回滚，导致问题反复出现。最终通过带外管理流程恢复对受影响数据中心的控制，完成配置回滚。

## 经验教训

1. **"50% 的流量在 4% 的节点上"——流量集中度的风险**：MCP 数据中心虽仅占 4% 的节点数，却承载 50% 的流量——高流量集中度意味着一小部分数据中心的故障会带来不成比例的巨大影响。
2. **BGP 配置变更中 term 顺序是"隐形地雷"**：配置 diff 中的 `!` 标记（表示 term 重排序）极易被忽略——配置审核工具应自动检测 term 顺序变化并标记为高风险。
3. **灰度部署策略必须包含所有架构变体**：这次变更的灰度步骤没有包含 MCP 数据中心作为一个独立阶段——"灰度"不仅是"分区域"，还要"分架构"。
4. **恢复过程中的"多工程师冲突"需要协调机制**：多名工程师同时回滚时互相覆盖——恢复操作需要一个"指挥官"来协调，或者使用"commit-confirm"自动回滚。
5. **"带外管理"是最后的救命稻草**：site-local 前缀撤回后工程师无法远程访问——带外管理（backup out-of-band management）是这类故障的唯一恢复路径。

## 预防与改进措施

- **预防（Prevent）**：BGP 配置审核工具自动检测 term 重排序；MCP 架构变更增加独立测试步骤
- **减小爆炸半径（Contain）**：MCP 数据中心的分批灰度部署（每次 1-2 个，而非全部 19 个同时）
- **快速检测（Detect）**：BGP 前缀通告变更的实时监控；site-local 前缀可达性告警
- **快速恢复（Recover）**：自动化的 "commit-confirm" 回滚机制（若变更后规定时间内未确认则自动回滚）；恢复操作的指挥官协调机制

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | "高可用性架构"引入的新组件（spine 路由器的 BGP policy）带来了新的失效模式——新架构的可靠性提升不是自动的，运维变更流程必须同步升级 |
| CRE | 50% 的流量受损意味着客户可能发现自己"完全不可用"或"完全正常"——取决于他们是否正好访问了 MCP 数据中心覆盖的区域；客户的感知是"非黑即白"的 |
| FDE | BGP 配置 diff 中 `!` 标记的取证需要"变更前后的 policy 全文对比"——运行日志中的 diff 摘要可能不显示 term 顺序变化，必须依赖完整的配置版本对比 |
| SA（客情危机） | 50% 全球流量受损 75 分钟，企业客户业务中断；Cloudflare 当日发布博客复盘，沟通及时透明，但'4% 节点承载 50% 流量'的集中度引发客户对供应商架构的审视 |
| SA（技术危机） | 流量集中度风险——客户应了解供应商 PoP 布局与容量分布，关键业务保持多供应商冗余；BGP term 顺序是隐形地雷，配置审核工具应自动标记高风险变更 |

## 参考资料

1. [Cloudflare outage on June 21, 2022（官方复盘）](https://blog.cloudflare.com/cloudflare-outage-on-june-21-2022/) — E1
2. [Cloudflare 6·21 outage coverage (The Register)](https://www.theregister.com/2022/06/21/cloudflare_outage/) — E1