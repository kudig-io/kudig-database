---
id: INC-20230124-CLOUDFLARE-ZT
title: Cloudflare Zero Trust 中断（Service Token 代码发布覆盖元数据致 121 分钟多服务不可用，WARP/Cache/R2 等受影响）
company: Cloudflare
company_type: cloud-native
domain: cdn-edge
date: 2023-01-24
duration_minutes: 121
severity: SEV-1
impact_scope: global
root_cause_category: software-bug
root_cause_tags: [cloudflare, zero-trust, service-token, metadata, release-rollback, warp, cache-purge, r2, workers, cascading-failure, credential-overwrite]
status: published
last_updated: 2026-08-01
sources:
  - https://blog.cloudflare.com/cloudflare-incident-on-january-24th-2023/
  - https://controld.com/blog/biggest-cloudflare-outages/
---

# Cloudflare 1·24（2023-01-24）：Service Token 元数据被覆盖，121 分钟的"自噬"故障

## 摘要

2023 年 1 月 24 日，Cloudflare 因一次 **Service Token 代码发布**中的元数据覆盖错误，导致多个核心服务中断 121 分钟。该故障的独特之处在于：**Cloudflare 运行在 Cloudflare 上**——为 Cloudflare 内部账户提供的 Service Token 被覆盖后，运行在这些账户上的 WARP、Zero Trust、Cache Purge、Cache Reserve、Images、R2 等服务相继失效。根因是：工程师在开发 Service Token 的最后使用时间显示功能时，**无意中覆盖了 Service Token 的其他元数据**，导致受影响账户的 token 全局失效。由于 token 的全球同步需要时间，故障呈现"渐进式恶化"的特点——从 WARP 开始，逐步蔓延到 Cache/R2 等更多服务。这是 Cloudflare "**自噬故障**"（自己运行在自己的平台上，自己的故障影响自己的服务）的典型案例。

## 影响评估（CRE 视角）

- **影响面**：Cloudflare WARP、Zero Trust、Cache Purge、Cache Reserve、Images、R2 等服务不可用约 121 分钟；部分客户账户的服务全面中断
- **影响时长**：约 16:55-18:56 UTC，约 121 分钟（因 token 全球同步延迟，不同客户受影响时间不同）
- **次生影响**：Cloudflare 自身基础设施依赖受影响服务；Cache Purge 不可用意味着客户无法及时清除缓存中的错误内容
- **对外沟通评估**：优秀——Cloudflare 发布详细技术复盘，含 Service Token 机制说明、时间线、改进措施
- **定级依据**：多个核心产品全面不可用，影响全球客户，SEV-1
- 未披露信息：受影响账户精确数量、受影响客户比例

## 时间线（UTC，2023-01-24）

| 时间 | 事件 | 证据 |
|---|---|---|
| 16:55 | Access 工程团队开始发布 Service Token 代码变更 | E1 |
| 17:05 | 团队发现不相关问题并回滚发布，但元数据已被覆盖 | E1 |
| 17:50 | 首个无效 Service Token（WARP 账户）同步至全球网络，WARP/Zero Trust 开始受影响 | E1 |
| 18:12 | 因 WARP 设备状态上传骤降至零，内部宣布事故 | E1 |
| 18:19 | Cloudflare API 账户的 Service Token 同步至全球，Cache Purge/Cache Reserve/Images/R2 开始受影响 | E1 |
| 18:21 | 定位到 Service Token 元数据被覆盖 | E1 |
| 18:28 | 事故升级，涵盖所有受影响产品 | E1 |
| 18:51 | 修复 WARP 账户的 Service Token，WARP/Zero Trust 恢复 | E1 |
| 18:56 | 修复 Cloudflare API 账户的 Service Token，Cache/R2 等恢复 | E1 |

**关键时间指标**：TTD = 37min（从 token 同步到告警）/ TTM = 86min / TTR = 121min

## 技术细节与根因分析（SRE 视角）

### 背景架构

Cloudflare 的 **Service Token** 机制允许自动化服务之间安全认证，由 Client ID 和 Client Secret 组成。Service Token 的元数据（包括 token 本身的有效期、权限等）存储在 Cloudflare 的全球网络中，变更后通过全球同步机制传播。

关键点：**Cloudflare 运行在 Cloudflare 上**——Cloudflare 自己的产品（WARP、Zero Trust、API 等）也使用 Service Token 进行内部认证。

### 因素三分

- **触发因素（Trigger）**：Access 工程团队发布代码变更，旨在增加"最后使用时间"显示功能——该变更无意中覆盖了 Service Token 的其他元数据。
- **根本原因（Root Cause）**：代码变更在 Service Token 的存储结构中错误地覆盖了元数据，而非追加新字段——导致已存在的 token 变为无效。
- **扩大因素（Aggravating Factors）**：
  1. Cloudflare 自身依赖 Service Token——内部账户的 token 失效后，运行在这些账户上的产品（WARP、Cache Purge、R2 等）全面不可用；
  2. token 变更的全球同步存在延迟，故障呈现"渐进式"恶化——从 WARP 开始，逐步蔓延到更多服务；
  3. 回滚发布（17:05）未能阻止已同步的 token 继续传播。
- **减轻因素（Mitigating Factor）**：一旦定位到根因，修复方案（将 Service Token 恢复为原始值）直接有效。

### 5 Whys

```
现象：Cloudflare WARP/Zero Trust/Cache/R2 等服务不可用约 121 分钟
Why1 → Service Token 元数据被覆盖，token 变为无效
Why2 → 代码变更错误地覆盖了已有元数据而非追加新字段
Why3 → 代码审核未发现元数据覆盖风险
Why4 → Service Token 的存储结构变更没有向后兼容性测试
Why5 → "Cloudflare 运行在 Cloudflare 上"意味着内部 Service Token
        的可靠性要求等同于面向客户的服务，但测试标准未匹配
        （系统性原因：内部基础设施组件的测试标准低于面向客户的服务）
```

## 解决过程

工程师定位到 Service Token 元数据被覆盖后，将受影响账户的 Service Token 恢复为原始值（通过直接更新 token 记录）。WARP 账户的 token 在 18:51 恢复，Cloudflare API 账户的 token 在 18:56 恢复。

## 经验教训

1. **"运行在自家平台上"是一把双刃剑**：Cloudflare 运行在 Cloudflare 上带来了"吃狗粮"（dogfooding）的好处，但也意味着"自己的故障可以影响自己"——内部 Service Token 的可靠性要求应等同于面向客户的服务。
2. **渐进式故障比瞬间故障更难诊断**：token 全球同步延迟导致故障从 WARP 逐步蔓延到 Cache/R2——"时间线不是一条线，而是一张网"。
3. **回滚发布不保证回滚状态**：在 17:05 回滚了发布，但已同步的 token 元数据覆盖已经发生——"回滚变更"不等于"撤销影响"。
4. **Service Token 是"隐形的基础设施"**：Service Token 本身不是面向用户的产品，但它的失效会导致 WARP、Cache、R2 等多个产品同时不可用——"认证基础设施"的可靠性投入应高于产品本身。

## 预防与改进措施

- **预防（Prevent）**：Service Token 存储结构的变更增加向后兼容性测试；代码审核增加元数据覆盖风险检查
- **减小爆炸半径（Contain）**：Service Token 变更的灰度发布，先影响少量账户再全面推广
- **快速检测（Detect）**：Service Token 有效性监控；WARP 设备状态上传速率的异常告警
- **快速恢复（Recover）**：Service Token 的"回滚到上一版本"机制；认证基础设施的快速故障转移

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | "运行在自家平台上"意味着内部基础设施组件的可靠性要求应等同于面向客户的服务——"dogfooding"的好处不应以牺牲可靠性为代价 |
| CRE | 客户无法区分"Cloudflare 的产品 A 出了问题"和"Cloudflare 的认证基础设施出了问题"——对他们来说，WARP 不可用就是 Cloudflare 不可用 |
| FDE | Service Token 元数据覆盖的取证需要"代码变更 diff + token 存储状态前后对比"——token 的全球同步日志是判断故障波及范围的关键证据 |
| SA（客情危机） | Zero Trust/WARP 等安全产品中断 121 分钟，'安全服务自身不可靠'的讽刺冲击客户信任；Cloudflare 复盘透明，但安全产品的可用性承诺被客户以更高标准审视 |
| SA（技术危机） | 安全供应商的可靠性=客户的信任基础——使用 Zero Trust 的客户应设计本地兜底与降级路径；'运行在自家平台上'的双刃剑效应提醒客户评估供应商内部依赖 |

## 参考资料

1. [Cloudflare incident on January 24, 2023（官方复盘）](https://blog.cloudflare.com/cloudflare-incident-on-january-24th-2023/) — E1