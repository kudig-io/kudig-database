---
id: INC-20230613-AWS-LAMBDA
title: AWS us-east-1 Lambda 大规模中断（单 cell 容量阈值触发潜在缺陷，STS/Console/EKS 等级联受损近 4 小时）
company: AWS
company_type: cloud-native
domain: cloud-infrastructure
date: 2023-06-13
duration_minutes: 228
severity: SEV-1
impact_scope: single-region
root_cause_category: software-bug
root_cause_tags: [aws, lambda, us-east-1, cellular-architecture, capacity-threshold, latent-bug, cascading-failure, sts, eks, eventbridge, single-region]
status: published
last_updated: 2026-08-01
sources:
  - https://aws.amazon.com/message/061323/
  - https://www.thestack.technology/us-east-1-aws-support-aws-outage/
  - https://hidekazu-konishi.com/entry/aws_postmortem_case_studies_design_lessons.html
  - https://www.datacenterknowledge.com/outages/a-history-of-aws-cloud-and-data-center-outages
---

# AWS 6·13（2023-06-13）：Lambda 单 cell 容量阈值触发潜在缺陷——cellular 架构"局部爆炸"近 4 小时

## 摘要

2023 年 6 月 13 日 **11:49 AM PDT** 起，AWS us-east-1 区域 Lambda 函数调用出现错误率和延迟上升，并级联影响 **Amazon STS、AWS Management Console、Amazon EKS、Amazon Connect、Amazon EventBridge** 等多个依赖 Lambda 的服务。根因是：Lambda 采用 **cellular（蜂窝）架构**，其中一个 cell 的 **Lambda Frontend** 在 10:01 AM 因日常流量增长开始扩容，11:49 AM 添加计算容量时**越过了单 cell 内此前从未达到的容量阈值**，触发潜在软件缺陷——执行环境（Execution Environments）被分配但未被 Frontend 实际使用，新调用找不到可用环境而报错。Lambda 调用在 13:45 PDT 恢复正常，全部服务 15:37 PDT 完全恢复，**总时长约 3 小时 48 分钟**。这是 AWS 官方 PES（Post-Event Summary）中 cellular 架构"爆炸半径受控"的教科书案例——**其他 cell 的函数调用完全未受影响**，但同时也暴露了"cell 扩容边界未设上限"的架构盲区。

## 影响评估（CRE 视角）

- **影响面**：us-east-1 区域 Lambda 调用错误率与延迟上升；STS（11:49-14:10，三段式影响）、AWS 控制台（11:48-14:02，显示 504/不可用）、EKS（新集群创建报错，存量集群不受影响）、Connect（呼叫/聊天无法发起，11:49-13:40）、EventBridge（投递延迟最高 801 秒）、AWS Support Center（11:49-14:38 降级）
- **影响时长**：11:49 PDT - 15:37 PDT，约 3 小时 48 分钟（Lambda 核心 1 小时 56 分钟）
- **次生影响**：SAML 联合登录（外部 IdP 联邦认证）受限；波士顿环球报、纽约 MTA、美联社等大型组织受影响（据 DCK）
- **对外沟通评估**：优秀——AWS 发布完整 PES，含架构讲解、影响面、根因与改进措施
- **定级依据**：区域级核心计算服务（Lambda）故障并级联多个服务，SEV-1；影响限于单区域
- 未披露信息：受影响调用量占比、受影响客户数量

## 时间线（PDT，2023-06-13）

| 时间 | 事件 | 证据 |
|---|---|---|
| 10:01 | 某 cell 的 Lambda Frontend 因日常流量增长开始扩容 | E1/E3 |
| 11:49 | 扩容中跨过单 cell 从未达到的容量阈值，潜在缺陷被触发，错误率上升 | E1 |
| 11:48-14:02 | AWS 控制台（us-east-1）不可用/504 | E1 |
| 11:49-14:10 | STS 错误率上升（三段式影响） | E1 |
| 11:49-13:40 | Amazon Connect 呼叫/聊天降级 | E1 |
| 13:45 | Lambda 调用恢复正常水平 | E1 |
| 14:38 | Support Center 完全恢复 | E1 |
| 15:37 | 全部受影响服务完全恢复 | E1 |

**关键时间指标**：TTD = 即时（内部） / TTM ≈ 2h（Lambda 恢复） / TTR ≈ 3h48m（全服务）

## 技术细节与根因分析（SRE 视角）

### 背景架构

Lambda 采用 **cellular 架构**：区域内划分多个 cell，每个 cell 包含 Lambda Frontend（接收并路由函数调用）和 Lambda Invocation Manager（按函数/账户管理底层计算容量，即 Execution Environments）。每个 cell 服务一部分函数调用——**cell 边界就是爆炸半径**。

### 因素三分

- **触发因素（Trigger）**：10:01 AM 开始的一次常规扩容（应对日常流量增长），11:49 AM 添加计算容量时跨过阈值。
- **根本原因（Root Cause）**：Frontend 扩容过程中**越过了单 cell 内此前从未达到的容量阈值**，触发潜在软件缺陷：执行环境被分配但未被 Frontend 完全使用，新调用找不到环境，出现错误和延迟。AWS 承认："这次事件暴露了 Lambda cellular 架构在 Frontend 扩容方面的缺口"。
- **扩大因素（Aggravating Factors）**：
  1. STS、Console、EKS、Connect、EventBridge 等大量服务内部依赖 Lambda——Lambda 故障形成服务级联；
  2. 单 cell 容量此前从未达到该规模，属于"未测试的规模区间"（untested scale）；
  3. EventBridge 投递延迟达 801 秒，放大了下游影响。
- **减轻因素（Mitigating Factor）**：cellular 架构生效——**其他 cell 的函数调用完全未受影响**；核心 Lambda 影响约 2 小时即恢复。

### 5 Whys

```
现象：us-east-1 Lambda 调用错误率上升近 4 小时，多个服务级联受损
Why1 → 某 cell 的 Frontend 无法为调用分配可用的执行环境
Why2 → 扩容中跨过单 cell 从未达到的容量阈值，触发潜在缺陷
Why3 → cell 扩容没有上限约束——达到阈值后没有"新建 cell"而是继续在
       cell 内扩容
Why4 → cell 的"已测试规模"边界没有被强制执行
Why5 → cellular 架构的扩容机制缺少"边界控制"设计——
       细胞化隔离了工作负载，但细胞的"生长"本身未被约束
       （系统性原因：对"未测试规模"缺少硬性护栏）
```

## 解决过程

AWS 团队定位到故障 cell 后，通过调整容量分配和恢复执行环境管理恢复了 Lambda 调用（13:45 PDT）。STS、Console 等依赖服务随 Lambda 恢复逐步恢复，15:37 PDT 全部服务恢复。AWS 在 PES 中宣布：**"更大的架构改进工作——将所有 cell 限制在经过充分测试的尺寸"**（将 cell 上限绑定到已测试规模，超出则新建 cell 而不是继续扩容）。

## 经验教训

1. **细胞化只是第一步，"细胞生长边界"才是关键**：cell 化隔离了爆炸半径（本案例其他 cell 毫发无损），但 cell 内部扩容失控仍造成 4 小时故障——"每个 cell 必须有显式的规模上限，超过上限触发新建 cell，而非继续扩容"。
2. **"此前从未达到的规模"是最贵的测试盲区**：潜在缺陷在阈值处暴露——"不要测试所有可能的规模，而是把 cell 保持在你测试过的规模之内"。
3. **认证服务常常隐藏着计算依赖**：STS、Console 都依赖 Lambda 内部路径——"你的应用能容忍 Lambda 慢，但不能容忍 STS 慢，那你其实间接依赖 Lambda"。
4. **短事件+受限范围是架构的胜利**：对比 2020 年 Kinesis 17 小时跨服务传播，本次 4 小时单 cell 故障是 containment 的成功案例——"架构的目标不是永不失败，而是失败保持小"。

## 预防与改进措施

- **预防（Prevent）**：cell 规模上限绑定已测试规模（AWS 已实施的架构改进）；扩容路径的容量阈值测试
- **减小爆炸半径（Contain）**：cell 扩容上限触发"新建 cell"而非继续扩容；Frontend 扩容的自动化护栏
- **快速检测（Detect）**：cell 级容量水位监控（接近未测试阈值时告警）；执行环境分配 vs 使用的偏离检测
- **快速恢复（Recover）**：故障 cell 的流量转移预案（把调用迁移至健康 cell）

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | "cell 化"必须包含"cell 生长边界"——只隔离不约束，故障只是变小而非消失；把规模上限绑定到已测试规模，是防止"阈值型潜在缺陷"的通用方法 |
| CRE | 大型组织（波士顿环球报/MTA/AP）因区域级 Lambda 故障受损——serverless 依赖链上的客户必须了解"我的服务间接依赖什么"，并把关键路径设计为可降级 |
| FDE | 取证的关键证据是"扩容动作的时间轴"（10:01 扩容→11:49 越界）——容量类故障的根因往往不在故障时刻，而在"此前从未测试的规模区间" |
| SA（客情危机） | Lambda 级联 STS/Console/EKS 等近 4 小时，企业客户（含大型组织）受损；AWS 承诺 cell 上限绑定已测试规模，但 serverless 依赖链的透明度问题被客户反复提出 |
| SA（技术危机） | serverless 客户必须绘制'我的服务间接依赖什么'——STS/Console 都可能依赖计算服务；关键路径设计为可降级，不要做'能容忍 Lambda 慢但不能容忍 STS 慢'的隐性依赖 |

## 参考资料

1. [Summary of the AWS Lambda Service Event in the Northern Virginia (US-EAST-1) Region（AWS 官方 PES）](https://aws.amazon.com/message/061323/) — E1
2. [Why did AWS Support fail with US-EAST-1 again?（The Stack）](https://www.thestack.technology/us-east-1-aws-support-aws-outage/) — E2
3. [AWS Postmortem Case Studies and Design Lessons（Hidekazu Konishi）](https://hidekazu-konishi.com/entry/aws_postmortem_case_studies_design_lessons.html) — E3
4. [A History of AWS Cloud and Data Center Outages（Data Center Knowledge）](https://www.datacenterknowledge.com/outages/a-history-of-aws-cloud-and-data-center-outages) — E4
