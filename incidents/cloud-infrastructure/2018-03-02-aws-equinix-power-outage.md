---
id: INC-20180302-AWS-EQUINIX
title: AWS us-east-1 第三方机房电源故障（Equinix 断电致 Alexa/Atlassian/Slack/Twilio 等大面积服务中断约 2 小时）
company: AWS
company_type: cloud-native
domain: cloud-infrastructure
date: 2018-03-02
duration_minutes: 120
severity: SEV-1
impact_scope: single-region
root_cause_category: hardware-facility
root_cause_tags: [aws, equinix, power-outage, third-party-dependency, us-east-1, datacenter, direct-connect, cascading-failure, alexa]
status: published
last_updated: 2026-08-01
sources:
  - https://www.datacenterknowledge.com/outages/equinix-power-outage-one-reason-behind-aws-cloud-disruption
  - https://www.cnbc.com/2018/03/02/amazon-cloud-networking-outage-affecting-atlassian-twilio-slack.html
  - https://www.geekwire.com/2018/widespread-outage-amazon-web-services-u-s-east-region-takes-alexa-atlassian-developer-tools/
  - https://virtualizationreview.com/articles/2018/03/05/aws-outage.aspx
---

# AWS 3·2（2018-03-02）：Equinix 机房断电让 Alexa"失声"——第三方数据中心依赖的教科书级事故

## 摘要

2018 年 3 月 2 日上午（美东时间），位于北弗吉尼亚的 **Equinix DC4-1 数据中心发生电源故障**，导致 AWS us-east-1 区域与该机房的网络互联（Direct Connect 等）大面积中断，进而造成 **Alexa、Atlassian（Jira/Confluence）、Slack、Twilio、Capital One** 等大量企业服务瘫痪或严重降级约 **2 小时**。AWS 官方称"us-east-1 区域部分 Direct Connect 客户遭遇网络丢包/中断"；Equinix 事后确认其 DC4-1 设施发生电源事件。这是**第三方数据中心依赖**故障的经典案例：AWS 自身的计算/存储服务并未故障，但客户与 AWS 之间的"网络管道"穿过第三方机房，机房断电即管道中断——**客户租用 AWS 的能力，却受制于客户自己选的网络路径**。该事件与 2023-03-29 腾讯广州机房事故（冷却故障）共同说明：**设施类故障不受云厂商 SLA 覆盖，是云依赖链条中最隐蔽的一环**。

## 影响评估（CRE 视角）

- **影响面**：AWS us-east-1 的 Direct Connect/网络互联中断；Alexa 智能音箱"失声"（AWS 自家服务受影响）；Atlassian、Slack、Twilio、Capital One 等企业服务中断或降级（数百家企业服务受影响，据 Virtualization Review）
- **影响时长**：约 2 小时（大部分服务在上午 11:00-12:00 ET 前后恢复）
- **次生影响**：依赖上述服务的下游企业无法使用开发工具、消息协作、语音助手；智能家居/IoT 设备（依赖 Alexa）大面积失效
- **对外沟通评估**：良好——AWS 与 Equinix 均在当日确认事件并归因；但双方对"责任边界"的表述一度存在模糊（AWS 称第三方网络问题，Equinix 确认电源事件）
- **定级依据**：数百家企业服务中断，含 Alexa 等国民级消费产品，SEV-1；影响限于 us-east-1 区域网络互联
- 未披露信息：受影响客户精确数量、Equinix 电源故障的具体技术原因

## 时间线（ET，2018-03-02）

| 时间 | 事件 | 证据 |
|---|---|---|
| ~09:00 | AWS 报告 us-east-1 部分 Direct Connect 客户网络丢包/中断 | E1/E2 |
| 上午 | 影响扩散：Alexa 失声，Atlassian/Slack/Twilio/Capital One 等服务中断 | E3/E4 |
| ~10:26 | GeekWire 报道"AWS 大范围故障"（Alexa/Atlassian 受影响） | E3 |
| ~11:00-12:00 | Equinix 抢修电源，服务陆续恢复 | E1 |
| 当日 | Equinix 确认 DC4-1 电源事件；AWS 确认问题源于第三方网络设施 | E1/E2 |

**关键时间指标**：TTD = 分钟级 / TTM ≈ 1h / TTR ≈ 2h

## 技术细节与根因分析（SRE 视角）

### 背景架构

AWS us-east-1 是全球最繁忙的云区域。企业客户常通过 **AWS Direct Connect**（专线）或经 Equinix 等 **colocation（托管机房）** 与 AWS 互联——客户的网络设备、专线接入点部署在 Equinix 机房内。**该链路穿过第三方机房：机房断电 = 客户与 AWS 之间的"管道"中断**，即使 AWS 侧完全健康，客户也无法访问。Alexa 的情况则说明 AWS 自家部分服务（语音处理链路的某环节）也经由此类设施承载。

### 因素三分

- **触发因素（Trigger）**：Equinix DC4-1 机房电源故障（具体原因未披露）。
- **根本原因（Root Cause）**：第三方机房断电导致网络互联中断——AWS 服务本身未故障，故障发生在"客户-云"依赖链的中间环节。
- **扩大因素（Aggravating Factors）**：
  1. 大量企业把专线接入点集中在少数几个 Equinix 机房——"机房聚合"放大了单点风险；
  2. Direct Connect 类依赖让客户失去"公有云弹性"的保护——网络路径是客户自己选定的，不在 AWS 故障域内；
  3. Alexa 等消费级产品受影响，把企业级故障放大为社会级影响。
- **减轻因素（Mitigating Factor）**：约 2 小时恢复；使用 VPN/IPSec 隧道或公网访问的客户不受影响；AWS 计算/存储服务本身未故障。

### 5 Whys

```
现象：数百家企业服务中断约 2 小时，Alexa"失声"
Why1 → 客户与 AWS 之间的网络互联中断
Why2 → Equinix DC4-1 机房电源故障
Why3 → 大量客户把专线接入点集中部署在该机房
Why4 → 客户网络架构缺少"多机房/多路径"冗余
Why5 → 第三方设施依赖未被纳入可靠性设计——
       客户把"网络路径"的冗余交给了单一第三方机房，
       而第三方的设施风险不在任何云厂商 SLA 覆盖范围内
       （系统性原因：第三方依赖的冗余设计缺失）
```

## 解决过程

Equinix 抢修恢复 DC4-1 机房电源后，网络互联陆续恢复，约 2 小时内大部分服务恢复正常。AWS 在状态页持续更新，Equinix 确认电源事件。双方后续未发布联合详细复盘。

## 经验教训

1. **"网络路径"是客户自己的责任域**：Direct Connect 穿过第三方机房，机房断电即中断——"选择专线/托管接入时，必须把第三方机房的设施风险纳入自己的可用性计算（多机房、多路径冗余）"。
2. **SLA 不覆盖第三方**：AWS SLA 覆盖计算/存储，Equinix 的电源故障不在任何 SLA 内——"你的可用性 = 你所有依赖（含第三方设施）的交集，不只是云厂商 SLA"。
3. **聚合效应**：大量客户集中选择同一机房/同一专线区域，形成事实上的"公共单点"——"接入架构需要主动分散（不同机房、不同运营商），而非被动跟随热点"。
4. **消费级影响放大企业故障**：Alexa 失声让本次企业级故障获得了国民级关注——"IoT/消费硬件依赖云服务的架构，必须设计离线降级模式"。

## 预防与改进措施

- **预防（Prevent）**：Direct Connect 多机房/多路径冗余（至少两个不同机房）；VPN/IPSec 备用通道常备
- **减小爆炸半径（Contain）**：接入点分散部署（不同 Equinix 机房 + 其他机房）；避免单机房承载全部关键流量
- **快速检测（Detect）**：专线路由/丢包率的外部探测（从多个网络位置监控）；机房设施状态的第三方监控
- **快速恢复（Recover）**：预配置的自动切换路径（专线故障自动切 VPN）；关键业务的公网访问降级预案

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | "可用性边界"比"服务边界"更重要——客户实际可用性取决于整条依赖链（云厂商 + 网络 + 第三方机房），可靠性设计必须覆盖链条而非仅覆盖云服务 |
| CRE | 企业客户（Capital One/Twilio 等）与消费用户（Alexa 用户）同时受损——供应商（Equinix）故障的沟通责任落在 AWS 与客户双方，客户需要"第三方故障"的沟通预案 |
| FDE | 取证链条显示：AWS 服务健康 ≠ 客户可用——故障证据必须覆盖"中间网络层"（专线状态、机房电力日志），单一来源的监控会得出错误结论 |
| SA（客情危机） | Alexa/Atlassian/Slack/Twilio 等大面积中断 2 小时，客户首次直观感受'第三方机房不在 SLA 内'；AWS 快速恢复，但客户对可用性责任边界（云厂商 vs 第三方设施）的认知被刷新 |
| SA（技术危机） | 网络路径是客户自己的责任域——Direct Connect 应多机房多路径冗余；客户的可用性=所有依赖（含第三方设施）的交集，选择专线/托管接入时必须把第三方设施风险计入架构 |

## 参考资料

1. [Equinix Power Outage One Reason Behind AWS Cloud Disruption（Data Center Knowledge）](https://www.datacenterknowledge.com/outages/equinix-power-outage-one-reason-behind-aws-cloud-disruption) — E1
2. [Amazon cloud networking outage affecting Atlassian, Twilio, Slack（CNBC）](https://www.cnbc.com/2018/03/02/amazon-cloud-networking-outage-affecting-atlassian-twilio-slack.html) — E2
3. [Widespread outage at Amazon Web Services' U.S. East region takes down Alexa, Atlassian developer tools（GeekWire）](https://www.geekwire.com/2018/widespread-outage-amazon-web-services-u-s-east-region-takes-alexa-atlassian-developer-tools/) — E3
4. [Hundreds of Enterprise Services Reportedly Hit by AWS Outage（Virtualization Review）](https://virtualizationreview.com/articles/2018/03/05/aws-outage.aspx) — E4
