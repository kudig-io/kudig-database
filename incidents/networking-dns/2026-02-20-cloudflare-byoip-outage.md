---
id: INC-20260220-CLOUDFLARE-BYOIP
title: Cloudflare BYOIP 全球前缀撤回故障（清理子任务 API 参数 bug 致 25% BYOIP 前缀被 BGP 撤回约 6 小时）
company: Cloudflare
company_type: cloud-native
domain: networking-dns
date: 2026-02-20
duration_minutes: 367
severity: SEV-1
impact_scope: global
root_cause_category: software-bug
root_cause_tags: [cloudflare, byoip, bgp, addressing-api, pending_delete, api-param-bug, automation, prefix-withdrawal, code-orange, fail-small, magic-transit, spectrum, cascading-failure]
status: published
last_updated: 2026-08-21
sources:
  - https://blog.cloudflare.com/cloudflare-outage-february-20-2026/
  - https://blog.cloudflare.com/fail-small-resilience-plan/
  - https://www.crn.com/news/cloud/2026/the-10-biggest-cloud-outages-of-2026-so-far
---

# Cloudflare 2·20（2026-02-20）：BYOIP 前缀大撤退——清理任务 API 参数 bug 让 25% 客户 IP 从互联网消失 6 小时

## 摘要

2026 年 2 月 20 日 17:48 UTC，Cloudflare 开始经历一次影响 **BYOIP（Bring Your Own IP）** 客户的服务中断：一个新增的**前缀清理自动化子任务**因 API 参数解析 bug，把"待删除前缀"查询错误解释为"全部前缀"，系统开始系统性删除所有 BYOIP 前缀及其关联的 service bindings（服务绑定），并通过 BGP 从互联网上**撤回约 1,100 个前缀**（占全部 4,306 个 BYOIP 前缀的 25%）。受影响客户的网站、Spectrum 应用、Magic Transit 保护的网络、Dedicated Egress 等从互联网不可达，Cloudflare 递归 DNS 官网 one.one.one.one 也短暂返回 403。整个事件持续 **6 小时 7 分钟**，大部分时间花在恢复被撤回前缀的配置上；约 300 个前缀因软件 bug 导致边缘配置被删除，只能由工程师手工重建（23:03 UTC 全部恢复）。根因是：为自动化"客户删除前缀"这一手工流程而新增的定期清理子任务，以 `?pending_delete`（无值）调用 API，而服务端把空值解析为"请求全部前缀"，于是将所有前缀当作待删除对象处理。这一事件是 Cloudflare **Code Orange: Fail Small**（小步失败）可靠性计划推进过程中的一次重大教训——**自动化改造本身成了故障源**，而同期推进的"健康中介部署"系统尚未上线，导致回滚无法快速完成。

## 影响评估（CRE 视角）

- **影响面**：约 1,100/4,306 个 BYOIP 前缀（25%）被撤回；CDN/安全服务、Spectrum、Dedicated Egress、Magic Transit 客户受影响；one.one.one.one 网站 403（1.1.1.1 解析器本身未受影响）
- **影响时长**：17:48-23:03 UTC（约 6 小时 7 分钟），其中前缀撤回发生在 17:56-18:46 UTC（50 分钟内撤回 1,100 个）
- **次生影响**：因 BYOIP 架构的"隐藏依赖"特性，受影响服务的用户难以定位故障源——他们以为目标网站坏了，实际是网站的 IP 从互联网消失了；部分客户通过 Cloudflare 控制台自助恢复，另一部分需等待全局配置重建
- **对外沟通评估**：优秀——Cloudflare 在次日发布深度技术复盘，含代码级根因（API 查询语句）、影响面数据与完整时间线
- **定级依据**：全球性 BGP 前缀撤回、多个产品线受损、持续时间 6 小时，SEV-1
- 未披露信息：受影响客户精确数量、受影响流量比例

## 时间线（UTC，2026-02-20）

| 时间 | 事件 | 证据 |
|---|---|---|
| 02-05 21:53 | 有 bug 的清理子进程代码合入代码库 | E1 |
| 02-20 17:46 | Addressing API 新版本部署上线，含故障子进程 | E1 |
| 17:48 | 故障开始（前缀广告更新开始传播） | E1 |
| 17:56 | 前缀开始被撤回——影响开始 | E1 |
| 18:13 | Cloudflare 因 one.one.one.one 故障被呼叫 | E1 |
| 18:18 | 内部事件宣告 | E1 |
| 18:46 | 工程师定位并终止故障子进程，开始恢复 | E1 |
| 19:11 | 恢复工作开始（重新宣告前缀） | E1 |
| 19:19 | 部分客户通过控制台自助恢复（影响降级） | E1 |
| 20:30 | 最终恢复流程启动（仍有绑定前缀 + 已删除绑定前缀） | E1 |
| 21:08 | 全局机器配置部署开始（恢复被删绑定的前缀） | E1 |
| 23:03 | 全局配置部署完成，所有前缀恢复——影响结束 | E1 |

**关键时间指标**：TTD ≈ 25min（17:56 影响开始 → 18:21 团队被 paged）/ TTM ≈ 50min（18:46 终止故障进程）/ TTR ≈ 5h17m（自终止后）

## 技术细节与根因分析（SRE 视角）

### 背景架构

Cloudflare 的 **Addressing API** 是网络中客户 IP 地址的权威数据源（source of truth）。任何对数据集的变更会立即反映到全球网络：客户通过公开 API 或 BGP Control 配置地址，API 写入数据库并触发操作工作流传播到边缘，路由器收到足够多机器通知后更新 BGP 前缀广告。**BYOIP 服务绑定（service bindings）** 将产品（Magic Transit、Spectrum、CDN）分配到特定 IP 段。此前"客户要求删除前缀"是一个**手工操作流程**；为减少高危手工操作（Code Orange 目标之一），Cloudflare 新增了一个定期运行的清理子任务来自动化该流程。

### 因素三分

- **触发因素（Trigger）**：清理子任务以 `GET /v1/prefixes?pending_delete`（无值参数）查询 API；服务端 `Query().Get("pending_delete")` 返回空字符串，代码将空值判断为"非空"从而跳过特殊分支——实际效果是**返回了全部前缀**，子任务把所有前缀当作待删除对象逐一删除。
- **根本原因（Root Cause）**：API 参数处理缺陷——布尔标志 `pending_delete` 被当作字符串处理，"无值"与"未提供"无法区分，导致清理子任务误删全部前缀及其关联 service bindings。更深层：**自动化改造（把手工删除流程转为自动子任务）时未覆盖"任务运行器独立执行变更"的测试场景**，测试只覆盖了客户自助 API 旅程。
- **扩大因素（Aggravating Factors）**：
  1. 配置变更秒级传播到全球边缘（无灰度、无健康中介），撤回速度远超恢复速度（50 分钟撤回 1,100 个，恢复用了 5 小时+）；
  2. 前缀处于三种不同受损状态（仅撤回 / 撤回+部分绑定被删 / 撤回+全部绑定被删），最后一种需要全局机器配置重建，无法自助恢复；
  3. 一个软件 bug 还导致部分前缀的边缘配置被删除，需要手工重建；
  4. 缺少"变更太快/太广"的熔断机制——没有任何监控能在批量撤回前缀时自动停止变更。
- **减轻因素（Mitigating Factor）**：变更按迭代方式应用而非瞬时全量，受影响面被限制在 25%；工程师快速终止子进程；客户可自助恢复部分前缀；1.1.1.1 解析器本身未受影响。

### 5 Whys

```
现象：25% 的 BYOIP 前缀被 BGP 撤回，客户 IP 从互联网消失
Why1 → 新增的清理子任务删除了所有前缀及服务绑定
Why2 → 子任务查询 `?pending_delete`（无值），服务端返回了全部前缀
Why3 → API 把空字符串参数解析为"请求所有前缀"，客户端无值调用与"查待删"无法区分
Why4 → 参数语义设计缺陷 + 测试只覆盖了客户自助旅程，未覆盖任务运行器独立执行场景
Why5 → 自动化改造流程缺少对"自动化本身"的验证与防护（健康中介部署、熔断器未就绪）
        （系统性原因：Code Orange 推进中的自动化改造先于防护系统上线）
```

## 解决过程

工程师在 18:46 定位并终止故障子进程后，按前缀受损状态分级恢复：可自助的客户通过控制台重新宣告前缀；仍有 service bindings 的前缀由 Cloudflare 重新宣告（20:30 起）；已删除全部绑定的前缀需要全局机器配置重建（21:08 起），于 23:03 UTC 完成。整个恢复过程长达 5 小时以上，主因是不同前缀处于不同受损状态、缺少一键回滚机制。

## 经验教训

1. **自动化改造必须先于"防护系统"就绪**：Cloudflare 一边部署自动化删除流程，一边建设健康中介部署与熔断机制——防护未上线，自动化先落地，结果自动化成了故障源。安全、可回滚的部署机制应先行。
2. **布尔参数的语义必须显式设计**：`?pending_delete` 空值与缺省值在 HTTP 查询参数中不可区分，服务端必须显式校验"参数是否存在"而非"值是否非空"，否则默认行为会变成"全量操作"——最危险的默认值。
3. **批量操作的爆炸半径必须有熔断**：50 分钟内撤回 1,100 个前缀没有任何告警停止机制。凡是"一次可影响大量对象的操作"，都需要变更速率/范围监控与熔断器。
4. **回滚能力决定恢复速度**：本次恢复慢的根因是没有"快照回滚"——操作状态与配置状态共用同一数据库，无法快速回到已知良好状态。配置快照 + 健康中介回滚是标配。
5. **隐藏依赖让故障难以归因**：BYOIP 客户的用户看到"网站连不上"却查不到 Cloudflare 故障——对客户而言，把关键 IP 与供应商配置的依赖关系显式化、可观测化，才能加速归因。

## 预防与改进措施

- **预防（Prevent）**：API schema 标准化（布尔标志显式类型化）；测试覆盖"任务运行器独立执行变更"场景；操作状态与配置状态分离
- **减小爆炸半径（Contain）**：批量前缀撤回的速率/范围监控与熔断器；配置变更改为健康中介的分段部署（Code Orange 一号工作流）
- **快速检测（Detect）**：监测前缀广告数量异常下降；监测客户服务健康信号作为熔断输入
- **快速恢复（Recover）**：配置数据库快照 + 一键回滚机制；区分"配置回滚"与"状态重建"的恢复流程

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 自动化改造必须先于防护系统就绪——没有健康中介部署和熔断器的批量自动化就是"定时炸弹"；布尔参数的"空值默认=全量"是最危险的默认行为 |
| CRE | BYOIP 客户遭遇"隐藏依赖故障"——服务不可达但供应商状态页正常；CRE 应帮客户梳理 IP 归属与供应商依赖，并准备"非 Cloudflare"的备选接入路径 |
| FDE | 本次事故证据链完整（代码合入→部署→参数解析→批量删除→BGP 撤回），归因清晰；但"前缀处于三种受损状态"提示：自动化的每个中间状态都要可枚举、可恢复 |
| SA（客情危机） | 25% BYOIP 前缀撤回致客户 IP 从互联网消失 6 小时，但用户把故障归因于目标网站而非 Cloudflare——"隐藏依赖"令供应商故障在客户侧被误解，舆情归因错位 |
| SA（技术危机） | 自动化改造的防护系统必须先于自动化落地（熔断/健康中介/快照回滚）；依赖供应商"自带 IP"服务的企业应评估单供应商 IP 管理风险与备选方案 |

## 参考资料

1. [Cloudflare outage on February 20, 2026 (Cloudflare Blog)](https://blog.cloudflare.com/cloudflare-outage-february-20-2026/) — E1
2. [Code Orange: Fail Small (Cloudflare Blog)](https://blog.cloudflare.com/fail-small-resilience-plan/) — E1
3. [The 10 Biggest Cloud Outages Of 2026 (So Far) (CRN)](https://www.crn.com/news/cloud/2026/the-10-biggest-cloud-outages-of-2026-so-far) — E3
