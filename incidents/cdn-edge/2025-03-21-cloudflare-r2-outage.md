---
id: INC-20250321-CLOUDFLARE-R2
title: Cloudflare R2 全球中断（密钥轮换中 `--env production` 参数遗漏致 100% 写入失败 67 分钟，R2/Cache Reserve/Stream 等受损）
company: Cloudflare
company_type: cloud-native
domain: cdn-edge
date: 2025-03-21
duration_minutes: 67
severity: SEV-1
impact_scope: global
root_cause_category: operational-safeguard
root_cause_tags: [cloudflare, r2, key-rotation, credential-mismanagement, wrangler-cli, env-parameter, human-error, write-failure, cache-reserve, stream, vectorize, images]
status: published
last_updated: 2026-08-01
sources:
  - https://blog.cloudflare.com/cloudflare-incident-march-21-2025/
  - https://safeguard.sh/resources/blog/cloudflare-r2-march-2025-credential-rotation-outage
---

# Cloudflare R2 密钥轮换事故（2025-03-21）：一个 `--env` 参数，67 分钟全球写入瘫痪

## 摘要

2025 年 3 月 21 日约 21:38 UTC，Cloudflare R2 对象存储因一次**密钥轮换操作**中的 CLI 参数遗漏，导致全球服务严重降级 67 分钟：**100% 的写入操作失败**，约 **35% 的读取操作失败**。根因是：R2 工程团队在轮换 R2 Gateway 服务使用的存储基础设施凭据时，`wrangler secret put` 和 `wrangler deploy` 命令均**遗漏了 `--env production` 参数**，导致新凭据被部署到**默认（开发）环境而非生产环境**。当旧凭据被删除后，生产环境的 R2 Gateway 无法认证存储后端，写入全部失败。该事件级联影响了 Cache Reserve、Images、Log Delivery、Stream、Vectorize 等多个依赖 R2 的 Cloudflare 服务。这是**"密钥轮换操作中的人为错误"的教科书案例**——一个最简单的疏忽（忘记 `--env`），导致全球范围的服务中断。

## 影响评估（CRE 视角）

- **影响面**：全球 R2 写入 100% 失败，读取约 35% 失败；级联影响 Cache Reserve、Images、Log Delivery、Stream、Vectorize、Billing、Email Security、Key Transparency Auditor 等
- **影响时长**：约 21:38-22:45 UTC，约 67 分钟
- **次生影响**：Stream Live 完全中断；Cache Reserve 用户回源请求激增；Billing 发票下载不可用
- **对外沟通评估**：优秀——Cloudflare 发布详细技术复盘，含每种服务的影响分析、时间线、改进措施
- **定级依据**：核心存储服务写入 100% 失败，全局影响，SEV-1
- 未披露信息：受影响客户数量、经济损失估算

## 时间线（UTC，2025-03-21）

| 时间 | 事件 | 证据 |
|---|---|---|
| 19:49 | R2 工程团队开始密钥轮换流程，创建新凭据 | E1 |
| 20:19 | 执行 `wrangler secret put` 和 `wrangler deploy`——**遗漏 `--env production`**，凭据部署到开发环境 | E1 |
| 20:20 | 错误地认为凭据已更新到生产环境 Worker | E1 |
| 20:37 | 从存储基础设施删除旧凭据（以为轮换完成） | E1 |
| 21:38 | **影响开始**——R2 可用性指标开始恶化 | E1 |
| 21:45 | R2 全球可用性告警触发（2% error budget 消耗率） | E1 |
| 21:50 | 内部宣布事故 | E1 |
| 22:05 | 发布公开状态页面 | E1 |
| 22:15 | 创建新凭据尝试强制重新传播，无改善 | E1 |
| 22:30 | 再次部署新凭据但**仍遗漏 `--env` 参数** | E1 |
| 22:36 | **定位根因**——发现凭据部署到了非生产 Worker | E1 |
| 22:45 | **影响结束**——将凭据部署到正确的生产 Worker | E1 |
| 22:54 | 事故关闭 | E1 |

**关键时间指标**：TTD = 7min（从影响开始到告警）/ TTM = 58min / TTR = 67min

## 技术细节与根因分析（SRE 视角）

### 背景架构

R2 Gateway 是 Cloudflare R2 对象的 API 前端，使用 Cloudflare Workers 运行。Gateway Worker 通过凭据（ID 和密钥对）认证到存储基础设施。轮换凭据时，先在 Worker 中部署新凭据，待确认生效后删除旧凭据。

### 因素三分

- **触发因素（Trigger）**：`wrangler secret put` 和 `wrangler deploy` 命令遗漏 `--env production` 参数，凭据部署到默认（开发）环境。
- **根本原因（Root Cause）**：人为错误——CLI 命令参数遗漏。工程师在轮换流程中未使用 `--env production` 标识，导致生产环境的 Worker 仍使用旧凭据。
- **扩大因素（Aggravating Factors）**：
  1. 凭据部署到错误环境后**无验证步骤**检查凭据是否正确部署到生产环境 Worker；
  2. 旧凭据删除后，生产环境 Worker 完全无法认证存储后端；
  3. 工程师在 22:30 再次部署新凭据时**仍遗漏了 `--env` 参数**——重复了同样的错误；
  4. 缺乏"凭据版本"的可观测性——无法快速确认生产环境 Worker 使用的是哪套凭据。
- **减轻因素（Mitigating Factor）**：无数据丢失——所有返回成功状态码的写入操作均已持久化。

### 5 Whys

```
现象：R2 写入 100% 失败，全球 67 分钟
Why1 → 生产环境 R2 Gateway 无法认证存储后端
Why2 → 凭据被部署到开发环境而非生产环境
Why3 → wrangler CLI 命令遗漏了 `--env production` 参数
Why4 → 凭据轮换流程缺乏"部署后验证"步骤
Why5 → 密钥轮换这一高风险操作依赖手动 CLI 命令，
        且缺乏"凭据版本"的可观测性来检测部署是否正确
        （系统性原因：高风险操作的手动流程缺乏自动化验证）
```

## 解决过程

定位根因后，将正确的凭据部署到生产环境 R2 Gateway Worker，服务立即恢复。但整个过程中工程师在 22:30 重复了同样的错误（仍遗漏 `--env`），说明在高压环境下手动操作的可靠性极低。

## 经验教训

1. **"最危险的操作是看起来最简单的操作"**：密钥轮换在概念上很简单（生成新密钥→部署→删除旧密钥），但手动 CLI 操作中的一个小疏忽（遗漏 `--env`）可以导致全球性故障。
2. **高压环境下的手动操作可靠性极低**：工程师在事故中重复了同样的错误（22:30 再次遗漏 `--env`）——"手动操作"在事故恢复中是不可靠的。
3. **密钥轮换流程需要"部署后验证"步骤**：部署新凭据后，应自动验证生产环境是否正确使用了新凭据——"确认生效后再删除旧密钥"是基本原则。
4. **"凭据版本"的可观测性可以缩短 MTTR 50%**：如果工程师能快速确认"生产环境 Worker 使用的是哪套凭据"，根因定位时间可以从 58 分钟缩短到几分钟。
5. **安全实践（定期轮换凭据）与可靠性目标之间的冲突**：定期轮换凭据是安全最佳实践，但每次轮换都是"高风险操作"——安全与可靠性之间的平衡需要自动化来弥合。

## 预防与改进措施

- **预防（Prevent）**：`wrangler secret put` 和 `wrangler deploy` 在 `--env` 参数缺失时增加警告/确认提示；密钥轮换流程自动化（而非手动 CLI）
- **减小爆炸半径（Contain）**：凭据部署到生产环境前先在灰度环境验证
- **快速检测（Detect）**：凭据版本的可观测性——监控生产环境 Worker 实际使用的凭据版本
- **快速恢复（Recover）**：旧凭据的"保留窗口"（在删除旧凭据后保留一段时间，以便快速回滚）

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 密钥轮换是"高风险常规操作"——看似简单的操作（生成→部署→删除）中任何一步出错都会导致全局故障；自动化是唯一可靠的方式 |
| CRE | 客户无法区分"R2 故障"和"Cache Reserve 故障"——任何依赖 R2 的产品同时不可用，客户的信任同时受损；恢复后客户仍需确认数据完整性 |
| FDE | 凭据部署错误的取证需要"Worker 部署历史 + 凭据版本日志"的关联——CLI 命令的审计日志是判断"凭据去了哪里"的关键证据 |
| SA（客情危机） | R2 100% 写入失败 67 分钟，存储客户数据写入受阻引发恐慌；Cloudflare 快速披露根因，但对象存储的可用性承诺在'密钥轮换'这类基础操作上受质疑 |
| SA（技术危机） | 对象存储客户应设计多区域冗余与写入降级策略；供应商密钥轮换等'看起来简单'的操作实为高危动作，客户应关注其自动化验证与部署后校验能力 |

## 参考资料

1. [Cloudflare incident on March 21, 2025（官方复盘）](https://blog.cloudflare.com/cloudflare-incident-march-21-2025/) — E1