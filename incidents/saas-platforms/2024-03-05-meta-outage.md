---
id: INC-20240305-META
title: Meta 全球大规模中断（Facebook/Instagram/Messenger/Threads 登录与信息流故障约 2 小时，2024 年 Meta 首次全球级故障）
company: Meta (Facebook)
company_type: internet
domain: saas-platforms
date: 2024-03-05
duration_minutes: 150
severity: SEV-1
impact_scope: global
root_cause_category: software-bug
root_cause_tags: [meta, facebook, instagram, messenger, threads, login, backend, single-failure-domain, global-outage, technical-issue]
status: published
last_updated: 2026-08-01
sources:
  - https://www.reuters.com/technology/metas-facebook-instagram-down-thousands-downdetector-shows-2024-03-05/
  - https://www.theguardian.com/technology/2024/mar/05/facebook-instagram-outages-disruption-meta-google
  - https://www.thousandeyes.com/blog/meta-outage-analysis-march-5-2024
  - https://www.cnn.com/2024/03/05/tech/facebook-instagram-outages
---

# Meta 3·5（2024-03-05）：后台技术故障让 Meta 全家桶再次"集体掉线"——登录风暴下的 2 小时全球中断

## 摘要

2024 年 3 月 5 日约 **10:00 ET（15:00 UTC）** 起，Meta 旗下 **Facebook、Instagram、Messenger、Threads** 在全球范围内出现大面积故障：用户被强制登出、无法登录、信息流卡死或显示错误。DownDetector 峰值时 Facebook 报告超 **30 万**、Instagram 超 **4 万**。故障持续 **超过 2 小时**（约 10:00-12:30 ET），Meta 官方仅确认是 **"技术问题"（technical issue）**，未披露详细根因。ThousandEyes 分析显示 Meta 网络可达、服务未宕机，问题出在**后端系统**。该事件是 Meta 自 2021-10-04 BGP 故障后的又一次家族级中断，也是 2024 年 Meta 的首次全球级故障——巧合的是，**同一时段 Google 平台也出现登录问题**（Guardian 报道称两者或有共同原因，但均未证实）。事件再次验证：**Meta 家族产品的"共享后端"仍是最大单点失效面**。

## 影响评估（CRE 视角）

- **影响面**：Facebook、Instagram、Messenger、Threads 全球登录/信息流故障；DownDetector 峰值 Facebook 30 万+、Instagram 4 万+ 报告
- **影响时长**：约 10:00-12:30 ET，超过 2 小时
- **次生影响**：依赖 Facebook/Instagram 登录（OAuth）的第三方应用受影响；企业社媒投放与运营中断；部分用户被强制登出后无法重新登录
- **对外沟通评估**：差——Meta 官方仅发简短声明确认"技术问题"，未发布详细复盘（延续了 Facebook 一贯的低透明度风格）
- **定级依据**：全球数十万用户报告、四大产品同时受损、时长 2 小时+，SEV-1
- 未披露信息：根因细节（官方从未公开）、受影响用户精确数量

## 时间线（ET，2024-03-05）

| 时间 | 事件 | 证据 |
|---|---|---|
| ~10:00 | 用户开始报告被登出/无法登录，DownDetector 报告激增 | E1/E2 |
| 10:00-12:00 | 影响持续扩大，Facebook 报告峰值超 30 万 | E1 |
| ~11:00 | Meta 官方承认"技术问题"，称正在修复 | E1/E4 |
| ~12:30 | 服务陆续恢复，报告量下降 | E1 |
| 恢复后 | Meta 未发布详细根因说明 | E1/E3 |

**关键时间指标**：TTD = 分钟级 / TTM ≈ 1h（官方承认） / TTR ≈ 2h30m

## 技术细节与根因分析（SRE 视角）

### 背景架构

Meta 旗下产品共享统一的**账号体系（Meta Account）与后端基础设施**：登录认证、账号状态、信息流推荐等核心链路由共享后端服务支撑。一次后端故障即可让多个产品同时"集体掉线"。ThousandEyes 的观测证实：**网络路径与边缘设施正常，故障发生在 Meta 后端**——用户"看得见服务、进不了服务"。

### 因素三分

- **触发因素（Trigger）**：官方未披露。行业分析指向后端系统（登录/账号服务）的异常。
- **根本原因（Root Cause）**：Meta 官方仅定性为"技术问题"，未公开根因。与 2019-03-13（配置变更）、2021-10-04（BGP）的公开度形成对比。
- **扩大因素（Aggravating Factors）**：
  1. 强制登出效应：大量用户同时被登出后集中重登，登录风暴放大后端负载（可能形成自我强化循环）；
  2. 共享账号/后端基础设施使四大产品同时受损；
  3. 登录类故障天然"传播"：被登出的用户反复尝试，报告量与真实影响同步膨胀。
- **减轻因素（Mitigating Factor）**：2.5 小时内恢复；无数据丢失报告；部分区域/用户未受影响。

### 5 Whys

```
现象：Meta 四大产品全球登录/信息流故障约 2.5 小时
Why1 → 共享后端（登录/账号服务）出现异常
Why2 → 官方未披露具体触发原因（"技术问题"）
Why3 → 强制登出引发登录风暴，放大后端负载
Why4 → 共享后端缺少对"登录风暴"模式的防护
Why5 → 家族级共享架构的故障防护与透明度双双不足
        （系统性原因：Meta 对共享基础设施故障的公开度
         与防护机制长期不足）
```

## 解决过程

Meta 团队定位并修复后端问题，约 12:30 ET 服务陆续恢复。官方未发布详细技术复盘，外部主要通过 ThousandEyes 等第三方观测还原事件——**网络正常、后端故障**是本事件最确定的证据链。

## 经验教训

1. **"被登出"是最糟糕的故障形态**：强制登出后用户集体重登，形成登录风暴——"故障 + 用户自救行为"叠加会显著放大影响；登录服务必须为"风暴式重登"留足容量。
2. **Meta 的透明度问题持续存在**：2019-03-13、2021-10-04 之后，2024-03-05 依然只有"技术问题"四个字——"不透明的故障会催生谣言与猜疑"，客户与监管的信任修复成本远高于一次透明复盘。
3. **共享后端 = 家族级单点**：四大产品同时受损再次证明——"产品多样化"不等于"基础设施多样化"。
4. **第三方观测是事件还原的重要证据**：ThousandEyes 等外部监控在供应商不透明时成为唯一可信证据源——"关键依赖必须保持外部可观测性"。

## 预防与改进措施

- **预防（Prevent）**：登录/账号服务的故障注入演练；对"登录风暴"模式的容量预置与限流
- **减小爆炸半径（Contain）**：产品级故障隔离（一个产品受损不牵连全家桶）；登录服务的分区承载
- **快速检测（Detect）**：强制登出率的异常监控（登出潮 = 故障前兆）；登录成功率 SLO
- **快速恢复（Recover）**：登录风暴的降级模式（延长令牌有效期、部分服务免登录访问）

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | "登出风暴"是认证类故障的放大器——把"用户被登出后的重登流量"纳入容量规划，是登录服务设计的必修课 |
| CRE | 数十万用户在 2.5 小时内无法使用 Facebook/Instagram，且官方只给"技术问题"四个字——透明度的缺失让用户只能从第三方渠道获取信息，信任修复成本被显著抬高 |
| FDE | 官方不披露时，取证依靠外部观测证据（网络正常、后端异常）与间接证据（登出潮时间轴）——"排除法取证"（网络正常→故障在后端）是供应商不透明时的标准方法 |
| SA（客情危机） | 全球强制登出约 2 小时，用户恐慌+谣言四起；Meta 仅'技术问题'四字，透明度问题持续（2019/2021/2024 三次全球故障均披露有限），客户与监管信任修复成本高昂 |
| SA（技术危机） | '被登出'是最糟糕的故障形态——客户（平台方）应为'风暴式重登'留足容量；登录服务必须容忍突发重登流量，客户应理解'故障+用户自救行为'的叠加放大效应 |

## 参考资料

1. [Meta's Facebook, Instagram back up after global outage（Reuters）](https://www.reuters.com/technology/metas-facebook-instagram-down-thousands-downdetector-shows-2024-03-05/) — E1
2. [Facebook and Instagram: Meta services hit by widespread disruption（The Guardian）](https://www.theguardian.com/technology/2024/mar/05/facebook-instagram-outages-disruption-meta-google) — E2
3. [Meta Outage Analysis: March 5, 2024（ThousandEyes）](https://www.thousandeyes.com/blog/meta-outage-analysis-march-5-2024) — E3
4. [Facebook and Instagram outage: Widespread disruption（CNN）](https://www.cnn.com/2024/03/05/tech/facebook-instagram-outages) — E4
