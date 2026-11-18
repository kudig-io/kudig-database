---
id: INC-20210315-AZUREAD
title: Azure AD 全球认证中断（认证系统变更致 Microsoft 365 全家桶认证失败约 3 小时，Teams/Outlook/Xbox 等全部中招）
company: Microsoft Azure
company_type: cloud-native
domain: identity-access
date: 2021-03-15
duration_minutes: 180
severity: SEV-1
impact_scope: global
root_cause_category: change-management
root_cause_tags: [azure-ad, microsoft-365, teams, authentication, identity-provider, single-failure-domain, change-management, token-expiry, cascading-failure]
status: published
last_updated: 2026-08-01
sources:
  - https://practical365.com/azure-ad-outage-march-15-2021/
  - https://www.exoprise.com/2021/03/15/microsoft-365-outage-march-15th-2021/
  - https://redmondmag.com/articles/2021/03/16/azure-ad-outage.aspx
  - https://learn.microsoft.com/en-us/answers/questions/315440/notification-authentication-errors-across-multiple
---

# Azure AD 3·15（2021-03-15）：认证系统变更让 Microsoft 365 全球"锁门"——一次变更击穿整个微软生态

## 摘要

2021 年 3 月 15 日约 **19:15 UTC**，Microsoft 365 用户在全球范围内开始遭遇认证失败：Teams、Exchange Online、Office、Dynamics、**Xbox Live**、**Azure Portal** 等依赖 Azure AD 认证的服务全部无法登录。根因是 **"对认证系统的一次近期变更"（a recent change to an authentication system）**——微软在约 21:25 UTC 开始全球部署缓解措施，22:10 UTC 前后核心影响结束，整个过程约 **3 小时**。该事件是 Azure AD（微软的"身份大脑"）故障模式的经典样本：**Azure AD 是 Microsoft 365 的单点失效点**——当它倒下时，整个微软生态（办公、协作、游戏、云平台）同时"锁门"。仅 3 天后（3 月 18 日），微软再次出现类似认证问题，进一步暴露了认证系统变更管理的脆弱性。

## 影响评估（CRE 视角）

- **影响面**：全球 Microsoft 365 用户（企业版为主）；Teams、Outlook/Exchange、Office、Dynamics、Xbox Live、Azure Portal 等所有依赖 Azure AD 的服务
- **影响时长**：约 19:15-22:10 UTC，核心约 3 小时；部分用户恢复更慢（与令牌刷新周期有关）
- **次生影响**：Teams 桌面客户端每小时刷新令牌，受影响最明显；Teams 频道邮箱连接器认证失败；依赖 Microsoft 认证的第三方 ISV 应用大面积失效；微软自家的 Service Health Dashboard 也因认证失败无法显示事件详情（"自己咬自己"）
- **对外沟通评估**：良好——微软通过 @MSFT365Status 和状态页持续更新，约 20:10 UTC 确认根因为"认证系统的变更"
- **定级依据**：全球范围内 Microsoft 365 生态认证不可用，影响数百万企业用户，SEV-1
- 未披露信息：受影响租户精确数量、变更的具体内容

## 时间线（UTC，2021-03-15）

| 时间 | 事件 | 证据 |
|---|---|---|
| ~19:00 | DownDetector 开始报告 Teams/Office 登录问题 | E1 |
| 19:15 | Azure 状态页确认事件 MO244568：部分客户认证遇到问题 | E1/E4 |
| ~20:10 | @MSFT365Status：问题源于"对认证系统的一次近期变更" | E1 |
| ~21:15 | 微软确认已定位底层原因，开始部署缓解 | E1 |
| 21:25 | 缓解措施开始全球滚动部署，部分客户开始恢复 | E1 |
| 22:10 | 更新完成部署，错误率开始下降 | E1 |
| 后续 | 3-18 再次出现认证问题（同类事件复发） | E1 |

**关键时间指标**：TTD = 分钟级 / TTM ≈ 1h / TTR ≈ 3h

## 技术细节与根因分析（SRE 视角）

### 背景架构

Azure AD 是微软生态的**统一身份提供方**：所有 Microsoft 365 服务、Xbox Live、Azure Portal 的登录认证都经过它。客户端持有短期访问令牌（如 Teams 桌面端每小时刷新一次），令牌过期时必须向 Azure AD 发起认证请求——**因此 Azure AD 的短暂故障会延迟性地"波及"所有持有过期令牌的客户端**，影响不会瞬间清零。

### 因素三分

- **触发因素（Trigger）**：一次针对认证系统的代码/配置变更部署。
- **根本原因（Root Cause）**：变更使认证请求处理出现故障。微软的初步定性与 2020-09-28 事件（Safe Deployment Process 潜在代码缺陷直投生产）属于同类——**认证系统的变更管理缺陷**。
- **扩大因素（Aggravating Factors）**：
  1. Azure AD 是单点失效点——Office/Dynamics/Xbox/Azure 全部依赖它；
  2. 令牌刷新机制导致"影响后移"：已登录用户不会立即掉线，而是随令牌逐批过期逐渐失联，影响面随时间扩大；
  3. 微软自身工具（Service Health Dashboard）依赖认证，事故期间"看不见自己"。
- **减轻因素（Mitigating Factor）**：已持有有效令牌的用户可继续工作；缓解措施部署后 45 分钟内核心影响结束。

### 5 Whys

```
现象：Microsoft 365 全球认证失败约 3 小时
Why1 → Azure AD 无法处理认证请求，所有依赖服务"锁门"
Why2 → 近期对认证系统的变更引入了缺陷
Why3 → 认证系统变更缺少充分的安全部署流程保障
Why4 → 认证变更的回归测试未覆盖关键路径
Why5 → 身份基础设施的变更管理标准低于其关键性级别
        （系统性原因：单点基础设施的变更风险评估不足）
```

## 解决过程

微软定位到根因后，在 21:25 UTC 开始全球滚动部署缓解更新，22:10 UTC 前完成部署，错误率随之下降。值得注意的是，**3 月 18 日同类认证问题再次出现**，说明第一次修复并未彻底解决底层问题，后续微软发布了正式 PIR（Post Incident Review）。

## 经验教训

1. **"Azure AD 是 Office 365 的阿喀琉斯之踵"**（Tony Redmond 语）：身份服务是生态的单点失效点，一次变更击穿所有产品——"身份基础设施的变更 = 生态级变更"。
2. **令牌机制把故障变成"慢性病"**：短期令牌 + 定期刷新意味着认证故障的影响不会瞬间爆发，而是随令牌过期逐批扩散——"影响曲线"的滞后性会让事件响应误判规模。
3. **认证系统变更必须走最高级别变更管理**：2020-09-28 与 2021-03-15 两次同类事件，说明"认证系统变更"需要比普通服务变更更严苛的部署保障。
4. **监控系统不能依赖被监控对象**：Service Health Dashboard 依赖 Azure AD 认证，认证挂了就看不了状态页——"监控必须独立于被监控系统"。

## 预防与改进措施

- **预防（Prevent）**：认证系统变更的强化测试与灰度；针对认证路径的故障注入演练（Chaos Engineering）
- **减小爆炸半径（Contain）**：认证服务按租户/区域分片；缓解更新按批次滚动部署
- **快速检测（Detect）**：认证失败率的独立监控（不依赖 Azure AD 自身）；令牌刷新失败率的告警
- **快速恢复（Recover）**：认证故障的预写回滚方案；缓存认证结果的降级模式

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 身份/认证服务必须被当作"最高风险变更面"管理——它的爆炸半径是整个生态；变更管理等级应该与它的关键性等级一致，而不是与它的代码量一致 |
| CRE | 客户在工作日早晨无法登录 Teams/Outlook 数小时，且微软自家状态页也无法工作——"工具失效时的沟通"是 CRE 预案必须覆盖的场景 |
| FDE | 令牌刷新机制让故障的取证窗口拉长：最终用户的报障时间与真实根因时间存在系统性偏差，取证时必须先重建"令牌生命周期"时间线 |
| SA（客情危机） | Microsoft 365 全家桶认证失败约 3 小时，企业客户办公中断；Azure 复盘获认可，但'身份基础设施=生态单点'的现实令企业客户评估身份冗余方案（本地 AD 混合、备用身份源） |
| SA（技术危机） | 身份基础设施的变更=生态级变更——客户应理解令牌 TTL 机制（短期令牌+定期刷新使影响逐批扩散）；认证系统变更必须按生态级管控，客户侧应设计备用认证路径 |
|  | 3 月 18 日同类故障复发是"修复有效性"的取证证据——单一事件复盘不足以证明根因已消除 |

## 参考资料

1. [Azure AD Suffers Another Big Authentication Outage（Practical365）](https://practical365.com/azure-ad-outage-march-15-2021/) — E1
2. [Microsoft 365 Outage, March 15th 2021（Exoprise）](https://www.exoprise.com/2021/03/15/microsoft-365-outage-march-15th-2021/) — E2
3. [Microsoft Offers Preliminary Explanation for March 15 Azure AD Issue（Redmond Mag）](https://redmondmag.com/articles/2021/03/16/azure-ad-outage.aspx) — E3
4. [Notification: Authentication errors across multiple Microsoft services（Microsoft Q&A）](https://learn.microsoft.com/en-us/answers/questions/315440/notification-authentication-errors-across-multiple) — E4
