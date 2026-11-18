---
id: INC-20260723-AZURE-WESTUS
title: Azure West US 区域进出流量中断（维护请求转换软件 bug 移除过多 IP 路由，约 5 小时 19 个下游服务级联）
company: Microsoft Azure
company_type: cloud-native
domain: cloud-infrastructure
date: 2026-07-23
duration_minutes: 297
severity: SEV-1
impact_scope: single-region
root_cause_category: software-bug
root_cause_tags: [azure, west-us, maintenance-automation, route-removal, request-conversion, ingress-egress, network-outage, datacenter, cascading-failure, mongodb-cloud, datadog, wiz, edtech]
status: published
last_updated: 2026-08-21
sources:
  - https://blog.incidenthub.cloud/azure-west-us-outage-jul-23-2026
  - https://azure.status.microsoft/en-us/status/history/
  - https://w.media/microsoft-azure-outage-cloud-services-disrupted-after-west-us-network-failure-now-resolved/
---

# Azure West US 7·23（2026-07-23）：维护自动化的"过杀"——一次例行设备维护把整个区域的进出流量切了 5 小时

## 摘要

2026 年 7 月 23 日 14:44-19:41 UTC，Microsoft Azure **West US 区域**发生约 **4 小时 57 分钟**的进出流量中断。微软 Preliminary PIR 给出的根因是：**维护请求转换软件（maintenance request conversion software）的 bug 把比预期更多的设备错误标记为维护对象**，导致从**数据中心到广域网（WAN）** 之间比预期更多的设备上被移除了 IP 路由——区域内所有**进出流量**被切断。**区域内部流量不受影响**，但对用户而言，区域外无法访问区域内任何服务，与"整个区域宕机"无异。影响波及 26+ 个 Azure/微软服务（App Service、Cosmos DB、AKS、API Management、ExpressRoute、VPN Gateway、Microsoft Graph 等），并级联到 **19 个下游 SaaS**（MongoDB Cloud、Datadog、Wiz、Octopus Deploy、Nintex、MeridianLink、Ellucian 等）；其中 16 个下游服务在 Azure 宣布恢复（19:41）后仍挂着 open incident——**上游恢复≠下游恢复**，恢复尾长达数小时。这是继 7-16 CloudFront 全球故障之后，一周内第二起"**自动化变更安全校验存在但被 bug 绕过**"的云巨头事故——与 2025 年 Azure Front Door（配置绕过验证）、2024 年 CrowdStrike（校验缺失）同属"变更防护失效"家族。

## 影响评估（CRE 视角）

- **影响面**：West US 区域进出流量全部中断；App Service、Application Gateway、Cosmos DB、AKS、API Management、Azure Firewall、ExpressRoute、VPN Gateway、Microsoft Graph 等 26+ 服务受损；级联 19 个下游 SaaS（MongoDB Cloud 集群连接异常、Datadog 指标延迟、Wiz 门户/API 降级等）
- **影响时长**：14:44-19:41 UTC（4 小时 57 分钟）；网络回滚 18:26 完成；下游恢复尾持续到 21:28+（部分服务）
- **次生影响**：区域内身份（Azure AD B2C）、监控（Azure Monitor/Log Analytics）、AI（Azure AI Search/Speech）等基础服务同时不可用，下游 SaaS 的"监控自己故障的能力"也受损（观测性级联）；教育科技 Ellucian 学生财务系统离线
- **对外沟通评估**：良好——Azure 状态历史发布 Preliminary PIR（tracking ID ZJV6-SGG），含根因与时间线；但下游恢复情况未主动披露
- **定级依据**：区域级进出流量全面中断、26+ 服务受损、19 个下游级联，SEV-1
- 未披露信息：受影响设备数量、触发维护的具体内容、根因修复细节

## 时间线（UTC，2026-07-23）

| 时间 | 事件 | 证据 |
|---|---|---|
| 14:44 | 设备维护开始；最早客户影响出现；多个 Azure 服务检测到劣化 | E1 |
| 14:45 | 网络/服务团队与事件响应者开始审查流量异常、路由行为、丢包与近期变更 | E1 |
| 15:00-16:00 | 影响范围与爆炸半径经报告与遥测验证；大规模 WAN 路由抖动调查中 | E1 |
| 16:00-17:45 | 调查收窄至异常路由/不当路由宣告，与近期光纤维护活动关联 | E1 |
| 17:45 | 发起维护变更回滚 | E1 |
| 18:26 | 回滚完成；开始监控恢复中的遥测与服务 | E1 |
| 19:41 | 所有受影响服务恢复 | E1 |
| 19:41 后 | 19 个下游中 16 个仍挂着 open incident（恢复尾持续数小时） | E1 |

**关键时间指标**：TTD ≈ 即时（遥测）/ TTM ≈ 3h（17:45 回滚）/ TTR = 4h57m（网络 3h42m 恢复）

## 技术细节与根因分析（SRE 视角）

### 背景架构

Azure 区域内的数据中心通过**广域网（WAN）** 与外界互联。例行**设备维护**需要隔离特定网络路径，流程为：维护请求 → **请求转换软件**将维护请求转为系统可读指令 → 安全校验确认**两条冗余路径中至少一条保持健康** → 执行隔离。该流程自带安全设计（冗余路径保护），目的是把维护影响降到最小。区域的进出流量依赖数据中心与 WAN 之间的路由表。

### 因素三分

- **触发因素（Trigger）**：一次例行设备维护活动（涉及光纤维护）启动，维护请求进入转换流程。
- **根本原因（Root Cause）**：**维护请求转换软件的 bug** 将额外设备错误标记为维护事件的一部分，导致**从比预期更多的设备上移除了 IP 路由**（数据中心 ↔ WAN 之间），切断了整个 West US 区域的进出流量。安全校验存在（冗余路径检查），但 bug 在转换层绕过了它——**校验针对"原始请求范围"，而 bug 扩大了实际影响范围**。
- **扩大因素（Aggravating Factors）**：
  1. 区域进出流量是**公共依赖点**：区域内计算/存储/数据库全部健康但不可达，对用户与"区域宕机"无法区分，且 **AZ 冗余完全无效**；
  2. 26+ 服务同时受损，包括身份、监控、AI 等"支撑其他服务"的基础服务，下游 SaaS 的观测能力也受影响（观测性级联）；
  3. 下游恢复滞后：16/19 个下游服务在 Azure 恢复后仍 open（各自的状态页滞后、队列积压、重连负载）；
  4. 多个下游 SaaS 开多个 incident（MeridianLink 5 个、Nintex 2 个）——组件级拆分让客户更难聚合理解。
- **减轻因素（Mitigating Factor）**：区域内部流量不受影响；维护变更可回滚（17:45 发起）；部分下游（如 SendGrid 同类）有区域故障转移能力可快速恢复。

### 5 Whys

```
现象：West US 区域进出流量全部中断约 5 小时
Why1 → 数据中心与 WAN 之间的 IP 路由被从过多设备上移除
Why2 → 维护请求转换软件把额外设备错误标记为维护对象
Why3 → 转换层 bug 扩大了维护范围，安全校验（冗余路径检查）被绕过
Why4 → 校验基于原始请求范围，未覆盖"转换后范围"的二次验证
Why5 → 维护自动化的"范围扩散"缺少独立的最终防线（变更范围闸门/影响面审计）
        （系统性原因：变更防护链路上单个组件 bug 即可击穿整体防护）
```

## 解决过程

调查在 16:00-17:45 期间将根因收窄到与近期光纤维护相关的异常路由宣告，17:45 发起维护变更回滚，18:26 回滚完成、网络恢复，19:41 所有受影响服务恢复。下游服务各自处理重连与积压（如 MongoDB Cloud 在 Azure 缓解后恢复集群连接）。

## 经验教训

1. **"进出流量"是区域里最脆弱的公共依赖点**：区域边界一旦切断，内部一切健康都无意义——对客户而言"区域外不可达 = 区域挂了"。架构评估必须把 ingress/egress 组件当作一等公民。
2. **校验存在≠校验有效**：Azure 的安全校验（冗余路径检查）设计正确，但被转换层 bug 绕过——变更防护需要**多层独立校验**，且校验范围要覆盖"转换后"的实际影响面。
3. **上游恢复≠下游恢复**：19 个下游中 16 个在 Azure 恢复后仍挂着——状态页滞后、队列积压、重连负载让"恢复尾"长达数小时。客户沟通必须基于自己的监控，而非供应商状态页。
4. **观测性也会级联失效**：Azure Monitor/Log Analytics 本身受损，下游 SaaS 连"监控故障的能力"都没了——观测体系需要跨供应商冗余。
5. **区域容灾的边界是"区域边界"**：进出流量故障时，区域内冗余与 AZ 多活全部失效——多区域部署且流量入口分散，才是区域级网络故障的真正逃生路径。

## 预防与改进措施

- **预防（Prevent）**：维护请求转换的"范围扩散"二次验证；变更影响面审计（预期 vs 实际设备清单比对）
- **减小爆炸半径（Contain）**：区域进出流量的独立健康探针；客户侧多区域入口 + 区域出口分散
- **快速检测（Detect）**：区域边界流量异常（进出方向）的独立监控；WAN 路由抖动告警
- **快速恢复（Recover）**：维护变更的一键回滚；下游 SaaS 的积压处理与重连退避预案（防惊群）

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 变更防护需要多层独立校验——转换层 bug 可以击穿"正确的安全设计"；区域进出流量应作为独立 SLO 与探针对象，AZ 冗余对此类故障无效 |
| CRE | 26+ 服务 + 19 个下游级联证明：区域边界故障的爆炸半径是"整个依赖该区域的企业群"——CRE 应推动客户做多区域入口设计，并把"监控供应商的能力"纳入容灾 |
| FDE | 观测性级联（Azure Monitor 受损致下游诊断能力下降）是本案例最重要的取证教训——取证工具本身不能与故障组件同依赖；恢复尾数据（16/19 延迟恢复）是级联影响的核心证据 |
| SA（客情危机） | 区域进出中断让客户"看起来像全挂了"，下游 SaaS 需要独立向用户解释"是 Azure 的问题"——状态页拆分（MeridianLink 5 个 incident）反而增加客户理解成本 |
| SA（技术危机） | 区域级网络故障无视 AZ 冗余——关键业务应多区域部署；与供应商的维护窗口/变更公告机制应提前建立，维护自动化范围需客户侧可观测 |

## 参考资料

1. [The July 23 2026 Azure West US Outage: IP Route Removal and Downstream Impact (IncidentHub)](https://blog.incidenthub.cloud/azure-west-us-outage-jul-23-2026) — E1
2. [Microsoft Azure Status History - Issues connecting to resources in West US (tracking ID ZJV6-SGG)](https://azure.status.microsoft/en-us/status/history/) — E1
3. [Microsoft Azure outage: Cloud services disrupted after West US network failure (W.Media)](https://w.media/microsoft-azure-outage-cloud-services-disrupted-after-west-us-network-failure-now-resolved/) — E3
