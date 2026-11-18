---
id: INC-20251118-CLOUDFLARE-BOT
title: Cloudflare 全球 5xx 中断（Bot Management 配置生成 bug 触发性文件超限引发 Rust panic，约 28% 核心流量受损 5.8 小时）
company: Cloudflare
company_type: cloud-native
domain: cdn-edge
date: 2025-11-18
duration_minutes: 346
severity: SEV-1
impact_scope: global
root_cause_category: software-bug
root_cause_tags: [bot-management, rust-panic, config-file, feature-limit, database-permission, clickhouse, fl2-proxy, workers-kv, access, turnstile, angry-storage]
status: published
last_updated: 2026-07-29
sources:
  - https://blog.cloudflare.com/18-november-2025-outage/
  - https://www.radware.com/security/threat-advisories-and-attack-reports/everything-you-need-to-know-about-the-cloudflare-outage/
---

# Cloudflare 11·18（2025-11-18）：当 Bot Management 配置文件超出了 200 个 feature 的限制

## 摘要

2025 年 11 月 18 日，Cloudflare 遭遇了自 2019 年 7 月以来最严重的全球性服务中断。约 11:20 UTC 起，全球大量托管在 Cloudflare 之后的网站（约 28% 的核心 CDN 流量无法正常访问）返回 **5xx 错误**；同时影响 Turnstile（CAPTCHA）、Workers KV、Access 与 Dashboard 登录。Cloudflare 在新浪博客（blog.cloudflare.com）上发布了详细复盘：根因是一条数据库权限变更触发了 Bot Management 模块的**配置生成 bug**。该权限变更使 ClickHouse 查询返回**重复行**，导致 Bot Management 的特征文件（feature file）**超过 200 个 feature 的预分配限制**——FL2 代理（Rust 实现）在解析该文件时触发 **panic 崩溃**，进而产生 5xx 错误。由于 Bot Management 配置文件每 5 分钟重新生成一次，且数据库权限变更逐步扩散，不同 ClickHouse 节点交替生成好坏文件，故障呈现"每 5 分钟波动"的特征。工程师最初误判为大规模 DDoS 攻击，耗时约 3 小时才定位到 Bot Management 配置文件问题。最终通过**停止生成坏文件 + 替换为已知良好文件**恢复核心流量，14:30 核心流量恢复，17:06 全部服务恢复。该事件是"**数据库变更→配置生成→语言级崩溃**"的经典级联故障链，也是 Rust 语言 panic 在生产环境中引发全局中断的典型案例。

## 影响评估（CRE 视角）

- **影响面**：约 28% 的核心 CDN 流量返回 5xx 错误；Turnstile、Workers KV、Access、Dashboard 登录不可用
- **影响时长**：11:20-14:30 核心流量恢复（约 3h10m），17:06 全部服务恢复（约 5h46m，346 分钟）
- **次生影响**：全球大量网站间歇不可用（包括本库的 GitHub Pages 等）；Turnstile 故障使依赖 CAPTCHA 验证的网站连带受影响；Cloudflare 2025 年第二次全球级中断
- **对外沟通评估**：优秀——状态页持续更新，复盘详细（含时间线、ClickHouse 权限变更、Rust panic 机制、逐步恢复过程）
- **定级依据**：全球约 28% HTTP 流量受损、多个子产品停摆近 6 小时，SEV-1
- 未披露信息：受影响网站总数、数据库权限变更的具体执行者

## 时间线（UTC，2025-11-18）

| 时间 | 事件 | 证据 |
|---|---|---|
| ~11:05 | 数据库权限变更开始逐步扩散（DELETE 权限被移除，系统转而以不同 schema 查询同一表） | E1 |
| ~11:11 | 首次生成超过 200 个 feature 的 Bot Management 配置文件（重复行导致） | E1 |
| 11:20-11:28 | FL2 代理加载该文件触发 Rust panic，全球 5xx 开始；状态页故障 | E1 |
| 11:20-13:00 | 工程师误判为 DDoS 攻击，启用缓解措施无效；故障每 5 分钟波动 | E1 |
| ~13:00 | 定位到 Bot Management 配置文件异常，开始绕过方案 | E1 |
| 13:05 | 对 Workers KV 与 Access 部署绕过方案 | E1 |
| 14:24 | 停止生成新 Bot Management 配置文件 | E1 |
| 14:30 | 替换为已知良好文件，核心 CDN 流量恢复 | E1 |
| 17:06 | 全部服务恢复（包括状态页、Dashboard 等） | E1 |

**关键时间指标**：TTD ≈ 数分钟（但误判为 DDoS）/ TTM ≈ 2h（定位到 Bot Management 文件）/ TTR ≈ 3h10m（核心流量）/ 5h46m（全量恢复）

## 技术细节与根因分析（SRE 视角）

### 背景架构

Cloudflare 的 Bot Management 模块依赖 ClickHouse 数据库存储特征规则，通过一个每 5 分钟运行的生成任务将数据库查询结果编译为配置文件（feature file）。FL2 是 Cloudflare 核心代理的 Rust 实现，该文件在启动/重载时被加载解析，预分配 200 个 feature 的向量空间。

### 因素三分

- **触发因素（Trigger）**：数据库权限变更使 ClickHouse 查询返回重复行，配置生成逻辑未去重，导致文件超过 200 个 feature 的预分配限制。
- **根本原因（Root Cause）**：配置生成逻辑缺少去重机制，且对数据源异常（重复行）无熔断保护；Rust 的 unwrap/panic 设计使文件解析失败直接导致进程崩溃，而非优雅降级。
- **扩大因素（Aggravating Factors）**：
  1. 配置文件每 5 分钟重新生成，故障持续波动；
  2. 权限变更逐步扩散，节点间状态不一致增加了定位难度；
  3. 初始误判为 DDoS 攻击，排查方向错误浪费约 2 小时；
  4. 状态页本身也受故障影响，外部沟通滞后。
- **减轻因素（Mitigating Factor）**：绕过方案（已知良好文件替换）有效恢复核心流量。

### 5 Whys

```
现象：全球约 28% CDN 流量 5xx，Turnstile/Access/KV 不可用
Why1 → FL2 代理加载 Bot Management 配置文件时 Rust panic 崩溃
Why2 → 配置文件超过 200 个 feature 的预分配限制
Why3 → 权限变更使 ClickHouse 查询返回重复行，配置生成无去重
Why4 → 配置生成逻辑对数据源异常无熔断与验证，直接输出坏文件
Why5 → 配置文件解析失败使用 panic 而非优雅降级，且生成逻辑
        无"文件有效性验证 + 自动回退至上一良好文件"机制
        （系统性原因：生成的配置文件缺乏端到端验证保护）
```

## 解决过程

工程师定位到 Bot Management 配置文件后，部署绕过方案保障 Workers KV 与 Access 继续运行，然后停止生成新配置文件、替换为已知良好文件恢复核心 CDN 流量，最后统一恢复所有服务。Cloudflare 整改包括：配置生成增加去重与数据源异常检测、文件解析失败改为优雅降级而非 panic、配置文件有效性验证与自动回滚至上一良好版本、增强 Bot Management 可观测性以快速区分配置问题与攻击。

## 经验教训

1. **生成式配置是不安全的配置**：从数据库实时生成配置文件并自动下发，本质上是"编译"-级操作——任何输入异常都会导致输出产物损坏，必须增加产物验证与自动回退。
2. **Rust panic 需要应对"坏数据"场景**：Rust 的 panic 设计在正常数据下可靠，但面对超出预期的输入时，panic 就是全局崩溃——对可能从外部输入生成的文件，应使用优雅降级模式。
3. **"数据库权限变更→配置异常→语言级崩溃"的级联链**：看似不相关的数据库维护操作，通过配置生成管道转化为全球中断——每个转换环节都是潜在的风险倍增器。
4. **误判为 DDoS 延误了 2 小时**：当监控显示"5xx 飙升"时，团队自然地假设是攻击——必须建立"配置问题→攻击→容量"的快速排除框架，以免浪费黄金响应时间。
5. **状态页不应与被监控系统在同一故障域**：状态页本身被故障影响，导致对外沟通延迟——状态页服务应独立于核心网络。

## 预防与改进措施

- **预防（Prevent）**：配置生成增加去重与数据源异常检测；文件解析优雅降级（非 panic）
- **减小爆炸半径（Contain）**：生成文件的预验证与自动回退至上一良好版本；文件部署分区域灰度
- **快速检测（Detect）**：配置文件异常检测告警；快速区分"配置问题/攻击/容量"的排查框架
- **快速恢复（Recover）**：已知良好文件一键替换机制；状态页独立于核心网络故障域

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 生成式配置管道是复杂的软件系统——需要与生产代码相同的测试、验证与回退机制；数据库变更的影响范围必须包括"所有消费该数据的下游系统" |
| CRE | 28% 核心流量受损约 6 小时——CDN 级故障的爆炸半径取决于客户部署策略，多 CDN 客户影响较小；Cloudflare 的透明复盘是信任修复的标杆 |
| FDE | 级联故障链（DB 权限→ClickHouse 查询→配置生成→Rust panic）的取证需要跨系统日志关联——数据库变更日志、配置生成任务日志、代理崩溃日志的时序对齐是关键 |
| SA（客情危机） | 28% 核心流量 5.8 小时 5xx，2025 年 Cloudflare 多次全球故障加剧客户观望；'生成式配置'管线缺陷冲击客户对自动化配置的信任 |
| SA（技术危机） | 生成式配置是不安全的配置——客户应关注供应商配置管线的产物验证与自动回退能力；对依赖 Bot Management 的业务应准备降级开关 |

## 参考资料

1. [Cloudflare outage on November 18, 2025（官方复盘）](https://blog.cloudflare.com/18-november-2025-outage/) — E1
2. [Everything You Need to Know About the Cloudflare Outage (Radware)](https://www.radware.com/security/threat-advisories-and-attack-reports/everything-you-need-to-know-about-the-cloudflare-outage/) — E3