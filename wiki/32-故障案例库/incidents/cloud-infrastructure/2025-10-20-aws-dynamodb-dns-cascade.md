---
id: INC-20251020-AWS-DYNAMODB
title: AWS us-east-1 大规模级联故障（DynamoDB DNS Enactor 竞态删除端点记录）
company: AWS
company_type: cloud-native
domain: cloud-infrastructure
date: 2025-10-20
duration_minutes: 870
severity: SEV-1
impact_scope: single-region
root_cause_category: software-bug
root_cause_tags: [dynamodb, dns-automation, race-condition, cascading-failure, us-east-1, control-plane, ec2-launch]
status: published
last_updated: 2026-07-29
sources:
  - https://aws.amazon.com/message/101925/
---

# AWS us-east-1 级联故障（2025-10-20）：DNS 自动化竞态引爆的 14 小时

## 摘要

2025 年 10 月 19 日深夜至 20 日（太平洋时间），AWS us-east-1 区域发生近年最严重的级联故障，持续约 **14.5 小时**，全球数千个服务与网站（Snapchat、Reddit、Fortnite、多家银行与航司）受影响。起点是 DynamoDB 的 **DNS 管理自动化**：该系统由 DNS Planner（生成端点变更计划）与多个并发的 DNS Enactor（执行计划）组成。一次罕见的**竞态条件**——一个执行异常缓慢的 Enactor 与一个执行清理逻辑的新 Enactor 相互交错——导致 **DynamoDB 区域端点（dynamodb.us-east-1.amazonaws.com）的 DNS 记录被错误清空**，且系统进入无法自动修复的不一致状态。DynamoDB 是 AWS 自身控制面的基座依赖：**EC2 实例启动子系统（DropletWorkflow Manager）**依赖 DynamoDB 维护物理主机租约，DNS 修复后 DWFM 又因海量积压租约陷入**拥塞性崩溃（congestive collapse）**，需要人工干预；随后**网络负载均衡器（NLB）健康检查**在 EC2 网络状态传播延迟下大规模误判摘除节点，形成第三波故障。Lambda、ECS/EKS、Connect、STS 等 100+ 服务先后受损。AWS 官方复盘完整披露了这条三级级联链，并在全球禁用了涉事 DNS 自动化。

## 影响评估（CRE 视角）

- **影响面**：us-east-1 区域 100+ AWS 服务异常；全球依赖 us-east-1 的数千服务（社交、游戏、金融、航空）中断或降级
- **影响时长**：约 14.5 小时（10-19 23:48 PDT 起，10-20 14:20 PDT 主要恢复）
- **次生影响**：us-east-1 作为"互联网单点"的集中度风险再次成为全球讨论焦点；多国监管关注云集中度
- **对外沟通评估**：优秀——官方复盘详细披露三级级联机制（DNS 竞态→DWFM 租约崩溃→NLB 健康检查风暴）
- **定级依据**：超大区域级、超长时间、全网外溢，SEV-1

## 时间线（太平洋时间）

| 时间 | 事件 | 证据 |
|---|---|---|
| 10-19 23:48 | DNS Enactor 竞态触发，DynamoDB 区域端点 DNS 记录被清空，新建连接开始失败 | E1 |
| 10-20 00:26 | AWS 确认 DynamoDB 端点 DNS 解析失败为直接原因 | E1 |
| 00:38-02:25 | 工程师人工介入修复 DNS 状态（自动化已进入不可自愈的不一致态），02:25 DynamoDB DNS 恢复 | E1 |
| 02:25+ | 第二波：EC2 DWFM 主机租约海量过期，恢复请求风暴导致拥塞性崩溃，新实例启动失败 | E1 |
| 05:28 | AWS 通过选择性重启 DWFM 主机等干预恢复租约系统，EC2 启动逐步恢复 | E1 |
| 06:21+ | 第三波：EC2 网络状态传播延迟使 NLB 健康检查大规模误判，节点被反复摘除/加回，连接错误激增 | E1 |
| 09:36 | AWS 禁用 NLB 自动故障切换的误判路径，NLB 恢复稳定 | E1 |
| 14:20 | 各服务积压消化完成，区域主要恢复正常 | E1 |

**关键时间指标**：TTD ≈ 数十分钟 / DNS 修复 ≈ 2.5h / 全链路 TTR ≈ 14.5h

## 技术细节与根因分析（SRE 视角）

### 背景架构

DynamoDB 端点由数十万条 DNS 记录支撑，自动化体系为 Planner（周期生成新计划）+ 多个冗余 Enactor（并发执行计划、清理旧计划）。EC2 的 DWFM 依赖 DynamoDB 存储物理主机租约；NLB 依赖 EC2 网络状态传播做健康检查。us-east-1 是 AWS 最大也是最老的区域，承载大量全球性服务的隐式依赖（如 STS、IAM 更新路径）。

### 因素三分

- **触发因素（Trigger）**：一个异常缓慢的 Enactor 与另一个已完成新计划并执行"清理旧计划"的 Enactor 交错执行——慢 Enactor 把旧计划当作最新应用，清理逻辑随后删除了该计划对应的记录，端点 DNS 被清空。
- **根本原因（Root Cause）**：DNS Enactor 的计划应用与清理之间缺乏原子性/世代校验（stale-plan guard），竞态窗口在极端时序下产生破坏性结果，且系统无法检测"端点记录为空"这一不变量被破坏。
- **扩大因素（Aggravating Factors）**：
  1. 自动化进入不一致状态后**无自愈路径**，只能人工修复（2.5 小时）；
  2. AWS 内部控制面对 DynamoDB 的深度依赖使故障向 EC2/NLB/Lambda 级联；
  3. DWFM 恢复请求无退避/分片，形成拥塞性崩溃，需选择性重启；
  4. NLB 健康检查在状态传播延迟下的激进摘除策略制造第三波震荡；
  5. us-east-1 的全球隐式依赖（全球服务的控制面锚点）放大为世界级影响。

- **减轻因素（Mitigating Factor）**：DynamoDB 数据持久性未受影响；数据面在 DNS 记录恢复后自动恢复正常。

### 5 Whys

```
现象：us-east-1 级联故障 14.5 小时，全球数千服务受影响
Why1 → DynamoDB 区域端点 DNS 记录被清空，新连接全部失败
Why2 → 慢 Enactor 与清理 Enactor 竞态，旧计划覆盖后又被清理删除
Why3 → 计划应用/清理无世代一致性校验，"空端点"不变量无守护检查
Why4 → 极端时序竞态未被测试覆盖，自动化无不一致自检与自愈
Why5 → 区域核心依赖（DynamoDB）的故障模式未被下游（DWFM/NLB）按
        "基座不可用"演练，恢复路径彼此踩踏
        （系统性原因：多级隐式依赖 + 恢复机制间的相互放大）
```

## 解决过程

工程师人工重建 DNS 状态（02:25 恢复 DynamoDB）；针对 DWFM 拥塞性崩溃采取限流与选择性主机重启，05:28 租约系统恢复；09:36 关闭 NLB 误判摘除路径；随后各服务消化积压，14:20 主要恢复。事后 AWS 在全球**禁用了 DynamoDB DNS Planner/Enactor 自动化**直至加装竞态防护，为 NLB 增加健康检查抑制机制，并承诺改进 DWFM 的恢复限流与测试覆盖。

## 经验教训

1. **自动化必须守护系统不变量**："端点必须有记录"这类不变量要有独立检查与自动阻断——自动化能做坏事的速度远超人类。
2. **并发自动化需要世代/租约一致性协议**：多 Enactor 并发执行+清理而无 stale guard，就是分布式系统教科书里的经典竞态。
3. **恢复机制会互相踩踏**：DNS 修复→租约风暴→健康检查风暴，三波故障中两波来自"恢复"本身；大规模恢复要全链路演练。
4. **区域集中度是全行业风险**：us-east-1 的隐式全球依赖让单区域故障成为互联网事件——关键业务必须验证"无 us-east-1 也能活"。
5. 官方复盘的三级级联链证明：**根因分析必须穿透到第 N 波故障**，只修 DNS 竞态不足以防止下次 14 小时。

## 预防与改进措施

- **预防（Prevent）**：Enactor 世代校验与计划清理原子化；竞态场景注入测试
- **减小爆炸半径（Contain）**：控制面对 DynamoDB 依赖的降级路径；NLB 健康检查摘除速率上限
- **快速检测（Detect）**：端点 DNS 记录数不变量监控；租约过期规模告警
- **快速恢复（Recover）**：DNS 状态人工修复预案工具化；DWFM 恢复限流与分片重启预案

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | "自动化竞态→基座失效→恢复风暴级联"三段式是 2020 Kinesis 之后 us-east-1 故障模式的集大成者 |
| CRE | 客户损失以"全球互联网半天"计——多区域架构从最佳实践升格为对关键客户的合规要求 |
| FDE | 复盘对竞态时序（慢 Enactor vs 清理 Enactor）的毫秒级重建，是分布式竞态取证的范文 |
| SA（客情危机） | 14.5 小时级联故障波及 Snapchat/Reddit/银行航司，全球用户感知强烈；AWS 复盘披露自动化竞态，客户对'自动化做坏事的速度'与 us-east-1 依赖风险加剧警觉 |
| SA（技术危机） | 自动化必须守护系统不变量——客户应关注供应商自动化护栏（世代/租约一致性）；恢复机制会互相踩踏，客户侧应演练大规模恢复场景而非单点故障 |

## 参考资料

1. [Summary of the Amazon DynamoDB Service Disruption in US-EAST-1 (AWS 官方复盘)](https://aws.amazon.com/message/101925/) — E1
