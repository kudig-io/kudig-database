---
id: INC-20260724-AWS-USWEST2
title: AWS us-west-2 区域网络中断（区域至 Seattle Metro 网络硬件故障，主影响 20 分钟但下游恢复尾达 10 小时）
company: AWS
company_type: cloud-native
domain: cloud-infrastructure
date: 2026-07-24
duration_minutes: 77
severity: SEV-1
impact_scope: single-region
root_cause_category: network-routing
root_cause_tags: [aws, us-west-2, network-hardware, seattle-metro, direct-connect, route-reconvergence, ingress-egress, cascading-failure, recovery-tail, thundering-herd, ninjaone, sparkpost, sendgrid]
status: published
last_updated: 2026-08-21
sources:
  - https://blog.incidenthub.cloud/aws-us-west-2-outage-jul-24-2026
  - https://health.aws.amazon.com/health/status?eventID=arn:aws:health:us-east-1::event/MULTIPLE_SERVICES/AWS_MULTIPLE_SERVICES_OPERATIONAL_ISSUE/AWS_MULTIPLE_SERVICES_OPERATIONAL_ISSUE_BA540_514A652BE1A
  - https://x.com/AWSSupport/status/2080641928991735993
---

# AWS us-west-2 7·24（2026-07-24）：20 分钟的小故障，10 小时的大尾巴——区域边界网络故障的恢复级联

## 摘要

2026 年 7 月 24 日 10:55 UTC，AWS **us-west-2（俄勒冈）区域与 Seattle Metro 之间**的网络连接中断：承载区域与 Seattle Metro 之间路由的**网络硬件故障**，切断了跨区域边界的进出流量。主影响窗口仅 **20 分钟**（10:55-11:15），随后路由重收敛（reconvergence）又造成 11:47-11:59 的间歇性抖动；使用 EqSe2/西雅图 Westin Building Exchange 的 **Direct Connect 客户受影响 1 小时 17 分钟**（10:55-12:12）。区域内部流量完全正常——但"区域外不可达"对用户与"区域宕机"无异，部分客户连 AWS Management Console 都进不去。**真正值得学习的是恢复尾（recovery tail）**：9 个确认 + 3 个疑似下游服务级联受影响，其中 NinjaOne 到 21:28 UTC 才恢复（**落后 AWS 路由恢复 9.5 小时**），SparkPost/SendGrid 处理邮件积压数小时——**上游 20 分钟的故障，下游要还 10 小时**。重连风暴（thundering herd）与队列积压是恢复尾的两大主因：NinjaOne 15 万+ 设备同时重连，设备状态处理服务被迫扩容。这也是 7 月"九天三起基础设施故障"（7-16 CloudFront、7-23 Azure West US、7-24 AWS us-west-2）的最后一起——两起都断在**区域与外部网络的边界**上。

## 影响评估（CRE 视角）

- **影响面**：us-west-2 进出流量中断；AWS Direct Connect、Global Accelerator、API Gateway、EC2、ECS、ELB、VPC、IoT Core、Site-to-Site VPN、Internet Connectivity 等 10 个服务受损；部分客户无法访问 AWS Management Console；级联 9 确认 + 3 疑似下游服务
- **影响时长**：主影响 20 分钟（10:55-11:15）；路由重收敛抖动 12 分钟（11:47-11:59）；Direct Connect（EqSe2）1 小时 17 分钟；下游恢复尾最长至 21:28（10.5 小时）
- **次生影响**：NinjaOne 15 万+ 设备重连风暴；SparkPost/SendGrid 邮件积压数小时；Bambu Lab（3D 打印机云服务）部分区域切换；观测类服务（Datadog 等）自身也受损
- **对外沟通评估**：良好——AWS 内部检测快（11:01 自动接入）、状态页 45 分钟首更；但首次公开更新晚于自身恢复（11:40 vs 11:15）
- **定级依据**：区域级进出流量中断 + 10 服务受损 + 多下游级联，SEV-1；但主影响仅 20 分钟
- 未披露信息：网络硬件故障的具体原因、受影响设备型号

## 时间线（UTC，2026-07-24）

| 时间 | 事件 | 证据 |
|---|---|---|
| 10:55 | us-west-2 与 Seattle Metro 连接丢失（影响开始）；区域内流量正常 | E1 |
| 11:01 | AWS 告警系统自动接入工程师 | E1 |
| 11:15 | 初始缓解完成，连接性开始恢复 | E1 |
| 11:40 | 状态页首次公开更新（距影响开始 45 分钟，距自身恢复 25 分钟） | E1 |
| 11:47-11:59 | 路由重收敛：部分客户间歇性连接问题 | E1 |
| 11:59 | 路由完全恢复，服务指标回到故障前水平 | E1 |
| 12:12 | EqSe2（Westin Building Exchange）Direct Connect 路由恢复 | E1 |
| 12:30 | 公开更新：披露根因，确认缓解完成 | E1 |
| 13:01 | 总结发布 | E1 |
| 后续 | 下游恢复尾：SparkPost 18:44、SendGrid 19:31、NinjaOne 21:28 才关闭 incident | E1 |

**关键时间指标**：TTD = 6min（11:01 自动接入）/ TTM ≈ 20min（11:15 初始缓解）/ TTR = 1h04m（12:12 Direct Connect 全恢复）；下游恢复尾最长 10.5h

## 技术细节与根因分析（SRE 视角）

### 背景架构

AWS 区域的进出流量通过**区域边界路由器/网络硬件**与外部网络（含 Seattle Metro 的 Direct Connect 设施）互联。客户访问区域内服务（包括 AWS Management Console）依赖这条区域 ↔ 外部网络的路径。**Direct Connect** 客户通过西雅图多个接入点（如 EqSe2、Westin Building Exchange）与 AWS 直连，AWS 建议客户通过不同接入点建立冗余路径。路由协议（BGP）持续交换路径信息，任何路由变化都需要**收敛**时间——收敛期间路由器对路径认知不一致，出现"死路由/慢路由"。

### 因素三分

- **触发因素（Trigger）**：承载 us-west-2 与 Seattle Metro 之间路由的**网络硬件故障**（具体原因未披露）。
- **根本原因（Root Cause）**：区域边界网络硬件故障导致区域进出路由不可用；**多条缓解路径并行执行**，第一条于 11:15 恢复连接。
- **扩大因素（Aggravating Factors）**：
  1. **路由重收敛**：缓解措施本身引发第二波影响——新路由宣告后 11:47-11:59 收敛期间间歇性断连，**修复动作制造了第二个影响窗口**；
  2. 区域进出流量是公共依赖点：10 个服务同时受损，区域内一切健康但不可达；
  3. 下游**重连风暴**：故障期间断开的客户端（如 NinjaOne 的 150,000+ 设备）同时重连，设备状态处理服务被压垮，需扩容——即使有退避+抖动（backoff & jitter），恢复仍以小时计；
  4. 下游**队列积压**：SparkPost/SendGrid 故障期间继续收件、出件被阻塞，恢复后需数小时排空积压；
  5. 状态页滞后：下游 11/12 个 incident 在 AWS 首次公开更新（11:40）之前就打开了——**供应商公开沟通滞后于下游感知**。
- **减轻因素（Mitigating Factor）**：区域内部流量不受影响；AWS 内部检测快（6 分钟自动接入）；有多条冗余 Direct Connect 接入点的客户未受影响；SendGrid 等有区域故障转移能力（11:21 将故障区域移出负载均衡，用户无感）。

### 5 Whys

```
现象：us-west-2 进出流量中断（主影响 20 分钟）
Why1 → 区域与 Seattle Metro 之间的路由不可用
Why2 → 承载路由的网络硬件故障
Why3 → 硬件故障的具体诱因未披露（老化/环境/配置触发）
Why4 → 硬件层冗余未能覆盖该路径（或冗余切换失败）
Why5 → 区域边界路径缺少"无中断冗余"设计 + 缓解动作引发路由重收敛第二波影响
        （系统性原因：区域边界是单点依赖，硬件故障 + 收敛抖动叠加放大）
```

## 解决过程

AWS 告警自动接入工程师（11:01），并行执行多条缓解路径，第一条 11:15 恢复连接；随后处理路由重收敛（11:47-11:59 全恢复）；EqSe2 Direct Connect 路径 12:12 恢复；12:30 公开披露根因。下游服务各自消化重连风暴与队列积压（NinjaOne 扩容设备状态处理服务，SparkPost 调优投递速率排空积压）。

## 经验教训

1. **上游时长预测不了下游时长**：AWS 主故障 20 分钟，下游恢复尾 10.5 小时——**故障的"总成本"在下游**。规划恢复时要把重连风暴、队列积压的消化时间算进去。
2. **修复动作本身可能制造第二波影响**：路由重收敛让"已恢复"变成"又抖了"——监控与客户沟通不应把第一次恢复信号当作终点，要持续验证到稳定。
3. **重连风暴要有"设计内"的消化机制**：NinjaOne 有退避+抖动仍花数小时——重连容量要预留、设备状态处理要有队列与自动扩容，否则恢复期就是第二次故障。
4. **区域边界 = 单点依赖，AZ 冗余无效**：两天内两起"区域边界"故障（Azure West US、AWS us-west-2）——区域内部多 AZ 部署救不了边界故障，多区域入口 + 分散 Direct Connect 接入点才有效。
5. **供应商公开沟通滞后于下游感知**：11 个下游 incident 早于 AWS 首次公开更新打开——依赖方不要等供应商状态页，自己的监控 + 探针才是第一信号。

## 预防与改进措施

- **预防（Prevent）**：区域边界路径的硬件冗余与故障切换演练；Direct Connect 多接入点冗余（AWS 已建议）
- **减小爆炸半径（Contain）**：客户侧多区域入口；区域故障转移预案（SendGrid 模式：快速移出负载均衡）
- **快速检测（Detect）**：区域边界连通性探针（进出方向独立探测）；供应商状态页事件联动
- **快速恢复（Recover）**：重连退避+抖动 + 自动扩容预案；队列积压排空计划（限速恢复、优先级队列）；恢复期持续验证（重收敛窗口）

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 恢复尾才是故障总成本——重连风暴与队列积压让 20 分钟故障变成 10 小时恢复；修复动作可能制造第二波影响（路由重收敛），恢复验证必须持续到稳定 |
| CRE | 区域边界故障无视 AZ 冗余：两起边界故障证明"区域内健康≠服务可用"——CRE 应推动客户验证"入口/出口"级别的容灾而非仅计算层；Direct Connect 多接入点冗余是硬要求 |
| FDE | 证据亮点：下游 11/12 个 incident 早于供应商首次公开更新——下游感知是供应商披露的独立证据源；NinjaOne 的退避+抖动仍不够的实证说明重连容量要按峰值预留 |
| SA（客情危机） | 20 分钟故障但客户感受 10 小时——恢复尾的客户沟通比故障本身更重要；Bambu Lab 等消费端云服务中断引发普通用户困惑，下游需独立公告 |
| SA（技术危机） | 区域边界硬件是"看不见的单点"——关键业务多区域部署且流量入口分散；恢复期重连/积压消化时间应计入对外承诺的恢复时间（ETR） |

## 参考资料

1. [The July 24, 2026 AWS us-west-2 Outage (IncidentHub)](https://blog.incidenthub.cloud/aws-us-west-2-outage-jul-24-2026) — E1
2. [AWS Health Dashboard - Multiple services [Connectivity Issues] us-west-2 (AWS)](https://health.aws.amazon.com/health/status?eventID=arn:aws:health:us-east-1::event/MULTIPLE_SERVICES/AWS_MULTIPLE_SERVICES_OPERATIONAL_ISSUE/AWS_MULTIPLE_SERVICES_OPERATIONAL_ISSUE_BA540_514A652BE1A) — E1
3. [US-WEST-2 Region Issue Resolved (AWS Support)](https://x.com/AWSSupport/status/2080641928991735993) — E1
