---
id: INC-20230125-M365
title: Microsoft 365 全球中断（WAN 路由器 IP 变更引发全网路由重算）
company: Microsoft
company_type: cloud-native
domain: networking-dns
date: 2023-01-25
duration_minutes: 338
severity: SEV-1
impact_scope: global
root_cause_category: change-management
root_cause_tags: [wan, router-ip-change, route-recomputation, safe-deployment, packet-forwarding, azure]
status: published
last_updated: 2026-07-29
sources:
  - https://azure.status.microsoft/en-us/status/history/
  - https://en.wikipedia.org/wiki/Timeline_of_Microsoft_Azure_outages
---

# Microsoft 365/Azure 全球中断（2023-01-25）：一条改 IP 的命令触发全球 WAN 重算

## 摘要

2023 年 1 月 25 日 07:05 UTC，微软全球广域网（WAN）中一台路由器上执行的一条**修改路由器 IP 地址的命令**，导致**整个 WAN 内所有其他路由器的数据包转发被打断、全网路由重新计算**。微软 WAN 承载着 Azure 区域之间、以及 Azure 与公网之间的全部流量，因此 Microsoft 365 全家桶（Teams、Outlook、Exchange Online、SharePoint、OneDrive）与大量 Azure 服务在全球范围内出现连接失败与超时，故障编号 Tracking ID **VSG1-B90**。IP 变更命令在不同型号路由器上的行为不一致——在该型号上触发了非预期的全网消息泛洪与路由重算。自动化恢复系统使大部分网络在 09:00 UTC 前后恢复，微软随后暂停所有高风险变更操作，至 12:43 UTC 宣布所有服务恢复，总时长约 **5.6 小时**。该事件与 2021 年 Facebook、2022 年 Rogers 一起，构成"**一条网络命令放倒一家巨头**"的三部曲，凸显 WAN 级变更的安全部署（Safe Deployment Practice）必须与软件发布同等严格。

## 影响评估（CRE 视角）

- **影响面**：全球 Microsoft 365 用户（Teams/Outlook/Exchange/SharePoint/OneDrive）与依赖跨区域连接的 Azure 服务；影响遍及各大洲
- **影响时长**：约 5.6 小时（07:05 – 12:43 UTC），大部分影响在前 2 小时内
- **次生影响**：全球大量企业晨间协作瘫痪（欧洲工作时段、亚太下午）；再次引发对 SaaS 单供应商集中依赖的讨论
- **对外沟通评估**：良好——状态页持续更新，事后发布含初步与最终版的 PIR（Post Incident Review）
- **定级依据**：全球性多产品线中断，SEV-1

## 时间线（UTC，2023-01-25）

| 时间 | 事件 | 证据 |
|---|---|---|
| 07:05 | WAN 一台路由器执行 IP 地址变更命令，全 WAN 路由器数据包转发中断，路由重算开始 | E1 |
| 07:05-07:30 | Microsoft 365 与 Azure 跨区域流量大面积失败，全球用户报障激增 | E1/E3 |
| 07:30+ | 微软定位到 WAN 变更，网络开始自动恢复；工程师暂停所有其他高风险变更 | E1 |
| ~09:00 | WAN 与大部分服务恢复正常，残余服务（依赖队列/重连）继续收敛 | E1 |
| 12:43 | 微软宣布所有服务恢复，事件关闭 | E1 |

**关键时间指标**：TTD ≈ 分钟级 / TTM ≈ 2h（网络自动恢复至大体正常）/ TTR ≈ 5h38min

## 技术细节与根因分析（SRE 视角）

### 背景架构

微软全球 WAN 连接所有 Azure 区域、边缘站点与公网出口，是 M365 与 Azure 的公共底座。WAN 路由器变更由网络运维命令执行；不同厂商/型号路由器对同一操作的副作用不同。微软的软件发布有严格的 Safe Deployment Practice（SDP，分环递进），但**网络设备命令类变更**当时未完全纳入同等级防护。

### 因素三分

- **触发因素（Trigger）**：对 WAN 路由器执行 IP 地址修改命令。
- **根本原因（Root Cause）**：该命令在此型号路由器上的行为与预期不同，触发全 WAN 范围的消息传播与路由重算；变更流程未识别此设备差异，也未按高危变更管控。
- **扩大因素（Aggravating Factors）**：
  1. WAN 是全产品线共享底座，无隔离分区，重算影响全球；
  2. 变更时间处于欧洲工作时段起点，业务影响被放大；
  3. 跨区域依赖使非网络服务（如依赖异地副本的存储/身份）也连带异常。

- **减轻因素（Mitigating Factor）**：Microsoft 365 的数据面在 WAN 恢复后自动恢复正常；Teams 等应用在恢复后缓存数据可用。

### 5 Whys

```
现象：M365 与 Azure 全球连接失败约 5.6 小时
Why1 → 全球 WAN 路由器转发中断并进行全网路由重算
Why2 → 一台路由器的 IP 变更命令触发了非预期的全网消息泛洪
Why3 → 该型号路由器对此命令的行为与其他型号不同，变更前未识别
Why4 → 网络命令类变更未纳入与软件发布同级的安全部署管控（评审/仿真/灰度）
Why5 → WAN 作为全产品共享底座，其变更爆炸半径未被架构性分区约束
        （系统性原因：网络变更防护成熟度落后于软件变更）
```

## 解决过程

微软通过 WAN 监控快速关联到 07:05 的路由器变更；网络具备自动恢复机制，路由收敛后大部分服务在 2 小时内恢复。工程师同时暂停了所有计划中的高风险网络变更以防叠加。事后 PIR 承诺：将网络设备命令纳入完整的变更审批与仿真验证流程、按设备型号差异建立命令行为基线、强化 WAN 变更的灰度与自动回滚。

## 经验教训

1. **网络命令就是变更**：必须享受与代码发布同等的评审、仿真、灰度、回滚待遇——"一条命令"心态是巨头级故障的共同起点。
2. **同一命令在不同型号设备上的行为差异**是网络运维的暗雷，需要设备行为基线库与变更前仿真。
3. 共享 WAN 底座的**爆炸半径需要架构性约束**（区域化/分区化路由域），而非仅靠流程防护。
4. 自动恢复机制显著缩短了 TTR（对比 Rogers 的 19 小时），**网络自愈能力建设的投资回报在故障日兑现**。
5. 故障期间**暂停一切其他变更**是标准动作，防止复合故障。

## 预防与改进措施

- **预防（Prevent）**：网络命令纳入 SDP；设备型号命令行为基线；变更前仿真
- **减小爆炸半径（Contain）**：WAN 路由域分区，单域重算不外溢
- **快速检测（Detect）**：WAN 全网转发健康秒级监控与变更自动关联
- **快速恢复（Recover）**：路由自动收敛与回滚；高风险变更全局熔断机制

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | "软件变更有 SDP、网络命令裸奔"的防护不对称被精确惩罚；自动恢复能力决定了损失是 2 小时还是 19 小时 |
| CRE | 全球企业办公同时中断——SaaS 集中化时代，供应商的一条网络命令即是所有客户的业务风险 |
| FDE | Tracking ID VSG1-B90 的 PIR 将根因精确到单条命令与设备型号行为差异，是变更关联取证的范例 |
| SA（客情危机） | Microsoft 365 全球中断约 5.5 小时，企业办公全面停滞；微软复盘披露单条命令触发全网路由重算，'一条命令=巨头级故障'的教训令企业客户审视变更管理文化 |
| SA（技术危机） | 网络命令就是变更——客户应要求网络运维与代码发布同等评审/仿真/灰度/回滚；共享 WAN 底座的爆炸半径需要架构性约束（区域化路由域），客户应评估自身对共享底座的依赖 |

## 参考资料

1. [Azure status history / PIR: Network connectivity issues (VSG1-B90)](https://azure.status.microsoft/en-us/status/history/) — E1
2. [Timeline of Microsoft Azure outages (Wikipedia)](https://en.wikipedia.org/wiki/Timeline_of_Microsoft_Azure_outages) — E4
