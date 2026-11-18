---
id: INC-20201125-KINESIS
title: AWS Kinesis 线程上限故障（前端 fleet 扩容触发 OS 线程数超限，us-east-1 级联 17 小时）
company: AWS
company_type: cloud-native
domain: messaging-streaming
date: 2020-11-25
duration_minutes: 1020
severity: SEV-1
impact_scope: single-region
root_cause_category: capacity-overload
root_cause_tags: [kinesis, thread-limit, frontend-fleet, gossip-protocol, cognito, cloudwatch, cascading-failure, us-east-1]
status: published
last_updated: 2026-07-29
sources:
  - https://aws.amazon.com/message/11201/
---

# AWS Kinesis 故障（2020-11-25）：加几台机器，撞碎了操作系统的线程天花板

## 摘要

2020 年 11 月 25 日（美国感恩节前一天），AWS us-east-1 区域的 Kinesis Data Streams 发生约 **17 小时**的严重故障，并级联拖垮 Cognito（登录认证）、CloudWatch（监控）、Lambda、EventBridge 等大批依赖服务——Roku、Ring、Adobe、纽约地铁信息屏等大量互联网服务随之瘫痪。诱因看似无害：运维团队向 Kinesis **前端（frontend）fleet 添加少量新服务器**以扩容。但 Kinesis 前端集群的成员关系与分片路由信息通过**每台服务器与其他所有服务器建立独立线程**的方式维护（全网状 gossip）——线程数随集群规模线性增长。这次扩容使每台前端服务器的线程数**超过了操作系统线程数上限**，全体前端服务器无法再构建完整的分片路由图（shard-map），请求处理全面失败。更痛的是恢复：前端 fleet 的成员信息传播极慢，**只能以极小批次重启**，每一批都要等待数小时的路由图重建，总恢复耗时远超故障本身。AWS 官方复盘坦承对该架构瓶颈"没有在扩容前意识到已如此逼近上限"，并披露 CloudWatch 受损期间连 AWS 自己的服务状态页（Service Health Dashboard）更新都遇到障碍。这是"隐藏的扩展性天花板"与"恢复速度不对称"的教科书案例。

## 影响评估（CRE 视角）

- **影响面**：us-east-1 的 Kinesis 全量客户；级联 Cognito（用户无法登录）、CloudWatch（指标/告警缺失）、Lambda、EventBridge、AWS 状态页更新链路；下游 Roku/Ring/Adobe 等大量知名服务
- **影响时长**：约 17 小时（05:15 PST 故障确立至 22:23 PST 完全恢复）
- **次生影响**：CloudWatch 失效造成全行业客户"监控失明"数小时；状态页更新受阻放大信任危机
- **对外沟通评估**：优秀——官方复盘详细披露线程上限机制、恢复慢的原因与整改路线
- **定级依据**：区域级、超长时间、监控与认证双基础设施级联，SEV-1
- 未披露信息：前端 fleet 具体规模、线程上限具体数值

## 时间线（太平洋时间，2020-11-25）

| 时间 | 事件 | 证据 |
|---|---|---|
| 02:44 | 运维向 Kinesis 前端 fleet 添加新容量（例行扩容） | E1 |
| 05:15 | Kinesis 错误率显著上升，故障确立；工程师开始排查（初期怀疑新容量本身有问题） | E1 |
| 07:51 | 移除新容量未见好转，排查范围缩小到前端集群整体异常 | E1 |
| 09:39 | 确认根因：前端服务器线程数超过 OS 上限，shard-map 构建失败 | E1 |
| 10:07+ | 制定恢复方案：调大上限+极小批次重启前端 fleet（成员信息传播慢，无法激进重启） | E1 |
| 下午~晚间 | 分批重启缓慢推进，Kinesis 错误率阶梯式下降；Cognito/CloudWatch 等随之逐步恢复 | E1 |
| 22:23 | Kinesis 完全恢复正常 | E1 |

**关键时间指标**：TTD ≈ 2.5h（从扩容到错误率确立）/ 根因定位 ≈ 4.5h / TTR ≈ 17h（恢复速度受限于架构）

## 技术细节与根因分析（SRE 视角）

### 背景架构

Kinesis 前端 fleet 负责请求接入、鉴权、路由到后端存储集群。每台前端服务器维护完整 shard-map（分片→后端映射），成员关系与路由信息通过前端服务器**两两互联的专用线程**持续同步——线程数 = 集群规模，随扩容线性增长。Cognito 使用 Kinesis 做数据流分析、CloudWatch 依赖 Kinesis 传输指标数据，形成隐式级联链。

### 因素三分

- **触发因素（Trigger）**：例行扩容增加前端服务器数量，使每台服务器的同步线程数突破操作系统线程上限。
- **根本原因（Root Cause）**：O(N) 线程的全网状同步架构存在隐藏扩展性天花板，且无监控告警度量"距离上限还有多远"；扩容操作前无该维度的容量核查。
- **扩大因素（Aggravating Factors）**：
  1. 恢复速度不对称：成员信息传播极慢，只能小批次重启，17 小时大半耗在恢复上；
  2. 初期误判方向（怀疑新容量本身），根因定位花费近 5 小时；
  3. Cognito/CloudWatch 对 Kinesis 的隐式依赖使故障级联为"登录+监控"双瘫；
  4. 感恩节前夕流量场景放大社会影响。

- **减轻因素（Mitigating Factor）**：Kinesis 数据持久性未受影响；已写入的数据在恢复后正常可读。

### 5 Whys

```
现象：Kinesis us-east-1 故障 17 小时，Cognito/CloudWatch 级联瘫痪
Why1 → 前端服务器无法构建 shard-map，请求处理全面失败
Why2 → 每台前端服务器线程数超过 OS 上限，成员同步线程创建失败
Why3 → 全网状 O(N) 线程同步架构，线程数随集群规模线性增长
Why4 → 无"线程数距上限余量"监控，扩容前未核查该维度容量
Why5 → 架构的扩展性天花板未被识别为风险，恢复路径（小批次慢重启）
        也从未演练过
        （系统性原因：隐藏扩展上限 + 恢复速度不对称双重缺陷）
```

## 解决过程

工程师定位线程上限后，调大系统上限并以极小批次重启前端 fleet（避免成员信息风暴），历经一天恢复。官方整改：前端 fleet 迁移到更大机型（同容量下服务器数减少、线程数下降）、推进**蜂窝化（cellularization）**将前端拆分为多个独立 cell 限制爆炸半径、shard-map 构建与成员同步解耦、为 Cognito/CloudWatch 建设对 Kinesis 依赖的降级路径（本地缓冲）、状态页更新链路与受影响服务解耦。

## 经验教训

1. **每个系统都有隐藏的扩展性天花板**（线程数、fd 数、连接数、ARP 表……）：必须为这些 OS/内核级资源建立"距上限余量"监控——撞上才知道存在的上限最致命。
2. **O(N) 全网状架构是定时炸弹**：成员同步应走 gossip 抽样/分层聚合，而非两两直连。
3. **恢复速度必须与故障速度对称设计**：秒级挂掉、小时级恢复的系统，SLA 由恢复端决定；"冷启动/全量重建要多久"应是架构评审必答题。
4. **监控系统依赖被监控的基础设施 = 嵌套故障**：CloudWatch 依赖 Kinesis 的教训与 Slack 2021 监控失明同源。
5. **蜂窝化（cell-based architecture）**是超大规模服务限制爆炸半径的核心手段——AWS 此后多项服务改造均源于此案。

## 预防与改进措施

- **预防（Prevent）**：OS 级资源（线程/fd/连接）余量监控与扩容前核查；更大机型减少节点数
- **减小爆炸半径（Contain）**：前端 fleet 蜂窝化，单 cell 故障不外溢；关键依赖方（Cognito/CloudWatch）本地缓冲降级
- **快速检测（Detect）**：shard-map 完整性与成员同步健康度专项监控
- **快速恢复（Recover）**：shard-map 构建与成员同步解耦以加速冷启动；小批次重启预案工具化

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | "扩容把系统扩死"的悖论案例：容量管理必须覆盖 OS 内核资源维度，而非只看 CPU/内存 |
| CRE | 监控（CloudWatch）与登录（Cognito）同时消失让客户既瞎又瘫——向客户说明隐式依赖链是信任基础 |
| FDE | 近 5 小时的根因定位提醒：撞上限类故障的现场证据（线程创建失败日志）需要被显式采集与告警 |
| SA（客情危机） | 感恩节前 Kinesis 17 小时级联故障，Cognito/CloudWatch 等依赖服务受损，客户业务高峰受损；AWS 复盘详尽，但'隐藏扩展性天花板'的教训令企业客户审视资源余量监控 |
| SA（技术危机） | 每个系统都有隐藏的扩展性天花板（线程/fd/连接/ARP）——客户应建立'距上限余量'监控；恢复速度必须与故障速度对称设计，冷启动/全量重建时长要有答案并演练 |

## 参考资料

1. [Summary of the Amazon Kinesis Event in the Northern Virginia (US-EAST-1) Region (AWS 官方复盘)](https://aws.amazon.com/message/11201/) — E1
