---
id: INC-20200830-CENTURYLINK
title: CenturyLink/Level 3 全球骨干故障（错误 Flowspec 规则阻断 BGP）
company: CenturyLink (Lumen)
company_type: internet
domain: networking-dns
date: 2020-08-30
duration_minutes: 306
severity: SEV-1
impact_scope: global
root_cause_category: network-routing
root_cause_tags: [bgp, flowspec, backbone, route-withdrawal-failure, cascading-failure, blast-radius]
status: published
last_updated: 2026-07-29
sources:
  - https://blog.cloudflare.com/analysis-of-todays-centurylink-level-3-outage/
  - https://en.wikipedia.org/wiki/2020_CenturyLink_outage
---

# CenturyLink 骨干故障（2020-08-30）：一条 Flowspec 规则如何让 Tier-1 骨干"僵尸化"5 小时

## 摘要

2020 年 8 月 30 日 10:04 UTC，Tier-1 骨干运营商 CenturyLink/Level 3（AS3356）因在其骨干网中下发了一条**错误的 BGP Flowspec 规则**——该规则阻断了骨干路由器之间的 BGP 会话本身——导致其全球骨干网陷入约 **5 小时**的严重故障。诡异之处在于：BGP 会话被反复打断又重建，路由器持续向互联网**宣告早已失效的路由却无法正常撤销**，使得全网流量仍源源不断被吸入这张已经"僵尸化"的骨干网黑洞。Cloudflare 观测到自身全球流量下跌 3.5%，众多依赖 AS3356 的服务（包括部分 911 紧急呼叫）受损。由于故障网络自身已无法配置管理，CenturyLink 最终以极端手段恢复——**要求所有对等网络切断与 AS3356 的互联**、并在骨干上撤销全部 Flowspec 规则、重启大量设备，15:10 UTC 前后全球恢复。该事件是"**故障网络无法自我修复且持续对外投毒**"的教科书案例。

## 影响评估（CRE 视角）

- **影响面**：AS3356/AS209 全球骨干及其下游客户；Cloudflare 全球流量 -3.5%，大量美国 ISP、语音与 911 服务受损
- **影响时长**：约 5 小时（10:04 – 15:10 UTC）
- **次生影响**：美国多个地区 911 紧急服务中断，引发 FCC 调查关注
- **对外沟通评估**：一般——过程沟通有限；Cloudflare 的第三方技术分析成为事实上的公开复盘
- **定级依据**：Tier-1 骨干全球性故障并外溢至无关网络，SEV-1

## 时间线（UTC，2020-08-30）

| 时间 | 事件 | 证据 |
|---|---|---|
| 10:03-10:04 | 错误 Flowspec 规则在骨干下发，BGP 会话被大面积阻断，全球故障开始（Cloudflare 流量骤降） | E4 |
| 10:00+ | BGP 会话反复断开/重建，失效路由持续被宣告而无法撤销，外部流量继续被吸入故障骨干 | E4 |
| 上午 | 依赖单一 CenturyLink 接入的网络完全离线；多宿主网络因对方仍宣告路由而持续受损 | E4 |
| ~14:00 | CenturyLink 请求所有对等网络断开与 AS3356 的互联，同时在骨干撤销全部 Flowspec 规则并重启设备 | E4 |
| 15:10 | 路由收敛，全球流量恢复正常水平 | E4 |

**关键时间指标**：TTD ≈ 分钟级 / TTM ≈ 4h（断开对等+撤销规则）/ TTR ≈ 5h06min

## 技术细节与根因分析（SRE 视角）

### 背景架构

BGP Flowspec（RFC 5575）允许通过 BGP 向全网路由器批量分发流量过滤规则（常用于 DDoS 缓解），特点是**传播快、生效快、范围广**——这既是它的价值也是它的风险。AS3356 是全球最大的 Tier-1 骨干之一，大量 ISP 与企业单宿主或多宿主接入。

### 因素三分

- **触发因素（Trigger）**：一条错误的 Flowspec 规则被下发到全骨干，规则本身阻断了路由器间的 BGP 会话（含 179 端口流量）。
- **根本原因（Root Cause）**：Flowspec 规则下发缺乏"规则会否杀死控制面自身"的防护校验与灰度机制，全网瞬时生效。
- **扩大因素（Aggravating Factors）**：
  1. BGP 会话重建后错误规则随 Flowspec 再次分发，形成"断开-重建-再断开"循环，故障自锁；
  2. 路由撤销消息无法在瘫痪的控制面中传播，失效路由持续吸入外部流量（僵尸路由）；
  3. 管理通道同样依赖被阻断的网络，运维无法远程修复（与 Facebook 2021 同款困境）。

- **减轻因素（Mitigating Factor）**：Flowspec 规则撤回后 BGP 路由逐步恢复；其他运营商未受影响。

### 5 Whys

```
现象：Tier-1 骨干全球故障 5 小时，且持续把外部流量吸入黑洞
Why1 → 骨干路由器间 BGP 会话被大面积阻断，转发面与控制面脱节
Why2 → 一条 Flowspec 规则误匹配并丢弃了 BGP 会话流量
Why3 → 规则下发前未校验"是否影响控制面自身"，且无灰度、全网即时生效
Why4 → Flowspec 的设计目标（快速全网分发）缺少配套的安全护栏
Why5 → 控制面自我保护（protect-the-protocol）未作为强制设计约束
        （系统性原因：强力自动化工具的爆炸半径未被同等强度地约束）
```

## 解决过程

由于故障骨干已无法通过常规手段管理（配置下发依赖的控制面正是被阻断的对象），CenturyLink 采取双管齐下：请求全部对等网络**主动切断与 AS3356 的 BGP 互联**，阻止外部流量继续被吸入僵尸路由；同时在骨干范围撤销所有 Flowspec 规则并重启大量设备，让控制面从干净状态收敛。15:10 UTC 全球流量恢复。Cloudflare 在事发当日发布了基于自身全球观测的详细分析。

## 经验教训

1. **Flowspec 等全网即时生效的工具必须有"不能杀死控制面"的硬校验**——匹配 BGP/管理流量的规则应被默认拒绝。
2. **僵尸路由是骨干故障最恶劣的形态**：故障网络持续宣告失效路由，会把多宿主客户的冗余设计变成无效——冗余方需要能主动拒收故障上游的路由（本地路由策略预案）。
3. 极端恢复手段（请对等方断开互联）应写入预案而非现场发明；**降低故障网络的"传染性"优先于修复它**。
4. 管理面必须与生产控制面隔离（带外管理），否则修复工具与故障共生死。
5. 对用户侧：**多宿主 + 自动化的上游健康探测切换**是抵御 Tier-1 故障的唯一有效手段。

## 预防与改进措施

- **预防（Prevent）**：Flowspec 规则下发前的控制面影响校验与灰度发布
- **减小爆炸半径（Contain）**：规则分发范围分区；对等方可快速拒收路由的预案
- **快速检测（Detect）**：骨干 BGP 会话震荡与撤销失败的聚合告警
- **快速恢复（Recover）**：带外管理通道；全网规则一键撤销与设备批量重启预案

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | "自动化工具阻断控制面→故障自锁→无法自我修复"的完整闭环案例；恢复靠外部协作断连 |
| CRE | 下游客户即使多宿主也被僵尸路由拖累——SLA 之外，客户需要的是可执行的上游隔离手册 |
| FDE | Cloudflare 以第三方全球观测（流量 -3.5%、逐分钟曲线）完成事实重建，证明外部观测网络的取证价值 |
| SA（客情危机） | Tier-1 骨干 5 小时瘫痪，多宿主客户的冗余设计集体失效（僵尸路由），企业客户对'冗余真实有效'产生质疑；CenturyLink 复盘，客户开始要求供应商提供故障传染性隔离证明 |
| SA（技术危机） | 僵尸路由会破坏客户的冗余设计——客户应配置本地路由策略主动拒收故障上游的路由；多宿主的冗余价值取决于拒收能力，'冗余方需要能说不' |

## 参考资料

1. [Analysis of Today's CenturyLink/Level(3) Outage (Cloudflare Blog)](https://blog.cloudflare.com/analysis-of-todays-centurylink-level-3-outage/) — E4
2. [2020 CenturyLink outage (Wikipedia)](https://en.wikipedia.org/wiki/2020_CenturyLink_outage) — E4
