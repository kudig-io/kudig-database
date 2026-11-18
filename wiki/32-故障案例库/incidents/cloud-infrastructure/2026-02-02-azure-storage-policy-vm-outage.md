---
id: INC-20260202-AZURE-VM
title: Azure 存储策略修复误删扩展包匿名读权限，全球 VM/VMSS/AKS 生命周期操作中断，恢复洪峰再击穿托管身份服务
company: Microsoft Azure
company_type: cloud-native
domain: cloud-infrastructure
date: 2026-02-02
duration_minutes: 619
severity: SEV-2
impact_scope: multi-region
root_cause_category: change-management
root_cause_tags: [control-plane, policy-remediation, storage-permissions, managed-identity, cascading-failure, blast-radius]
status: published
last_updated: 2026-10-23
sources:
  - https://azure.status.microsoft/en-us/status/history/
  - https://windowsforum.com/threads/azure-outage-february-2026-vm-provisioning-and-identity-service-disruption.400049/post-956912
  - https://outagecost.com/azure-outages
---

# Azure 存储策略修复失控（2026-02-02）：一条合规工作流如何让全球虚拟机"生不出来、扩不动、装不了扩展"

## 摘要

2026 年 2 月 2 日 19:46 UTC 起，Azure 全球多个区域的客户对虚拟机执行创建/更新/扩缩容/启停等生命周期操作时开始报错（Tracking ID **FNJ8-VQZ**）。根因是一条**安全合规修复工作流**错误地禁用了微软托管存储账户（存放 VM 扩展安装包的 blob 容器）的匿名只读访问，且其**目标定位逻辑中的数据同步故障**把变更传播到了超出预期的范围——VM Agent 下载扩展包失败，VM、VM Scale Set、AKS 节点池的全球生命周期操作大面积失败。约 4 小时后初始缓解完成，但恢复阶段的流量洪峰又压垮了 East US / West US 的 **Managed Identities 令牌签发后端**（Tracking ID **M5B-9RZ**，00:10–06:05 UTC），身份依赖型操作二次受损。运行中的工作负载本身大多未受影响，受损的是"控制面"：不能扩、不能建、不能装扩展、不能拿令牌。全程约 10 小时 19 分（两阶段叠加窗口）。

## 影响评估（CRE 视角）

- **影响面**：全球多区域 VM/VMSS/AKS 生命周期操作失败；第二阶段波及 East US / West US 的托管身份令牌获取；Azure DevOps、GitHub Actions 流水线中调用扩展的安装步骤连带失败
- **功能维度**：新建/扩容/扩展安装/身份令牌获取四类控制面操作；**运行中的业务负载基本不受影响**——这是本案例最重要的影响结构特征
- **影响时长**：19:46 UTC（2 日）→ 06:05 UTC（3 日），其中 VM 管理操作主窗口约 6.5 小时、托管身份二次窗口约 6 小时
- **错误预算**：对依赖自动扩缩容支撑流量的客户（电商大促、批处理队列），控制面不可用等效于数据面不可用——扩容失败即容量停滞即服务降级
- **次生影响**：CI/CD 流水线成批失败；大量"为什么我的部署卡住了"的工单涌入；AKS 节点池无法扩容直接影响上新
- **对外沟通评估**：良好——Tracking ID 清晰、分阶段状态页更新、社区渠道有官方时间线锚点；但未发布深度技术复盘
- **定级依据**：全球范围核心平台操作失败约 6.5h + 区域身份服务二次失效，但数据面运行负载未中断，无数据丢失，定为 SEV-2

## 时间线（UTC）

| 时间 (UTC) | 事件 | 证据 |
|---|---|---|
| 02-02 19:46 | FNJ8-VQZ 立案：全球多区域 VM 管理操作报错 | E2 |
| 19:46–20:00 | 定位：合规修复工作流禁用托管存储账户匿名读，目标定位数据同步故障致范围扩散 | E2/E3 |
| 20:00–22:00 | 分区域缓解开始，部分区域恢复 | E2 |
| 02-03 00:08–00:10 | 恢复流量洪峰压垮 Managed Identities 令牌后端（M5B-9RZ，East US / West US） | E2 |
| 00:10–04:20 | 身份操作失败/排队，基础设施节点逐步恢复 | E2 |
| 04:20–06:05 | 令牌服务缓慢放量回填积压，06:05 宣布恢复 | E2 |

**关键时间指标**：TTD ≈ 客户报告即立案（分钟级）/ TTM ≈ 40 分钟内开始分区域缓解 / TTR ≈ 10h19m（含二次身份窗口）

## 技术细节与根因分析（SRE 视角）

### 背景架构

Azure VM 生命周期依赖一条隐形的共享依赖链：扩展安装包由微软托管的 Azure Storage 账户（blob.core.windows.net 下的平台容器）托管，VM Agent 默认通过**匿名公共读**下载；大量资源的编排又依赖 Managed Identities 令牌签发后端。两者都是典型的"平台级共享依赖"——客户看不见，也无处冗余。

### 因素三分

- **触发因素（Trigger）**：安全合规修复工作流执行"禁用匿名访问"策略变更
- **根本原因（Root Cause）**：工作流目标定位逻辑的数据同步故障，使策略变更**传播到超出目标的存储账户集合**；变更本身缺少按账户灰度与自动回滚
- **扩大因素（Aggravating Factors）**：
  1. 平台共享依赖：一个存储权限问题即放大为全球 VM 生命周期故障
  2. 恢复洪峰：初始缓解后的重试/排队流量直接压垮托管身份后端，造成二次事故
  3. 匿名读是设计内行为：修复"安全隐患"时没有为"这个匿名读是被平台依赖的"建模
- **减轻因素**：数据面与生命周期操作解耦，运行负载未受影响；分区域缓解避免了全球同步恢复的风险

### 5 Whys

```
现象：全球 VM/VMSS/AKS 生命周期操作失败，随后 East/West US 身份令牌失败
Why1 → VM Agent 无法从托管存储下载扩展包 / 令牌后端过载
Why2 → 托管存储账户匿名读被禁用 / 恢复洪峰超出令牌后端容量
Why3 → 合规工作流的定位逻辑同步故障扩大了变更范围 / 后端无针对恢复洪峰的限流与扩容预案
Why4 → 工作流上线前缺乏"目标集合漂移"的校验与灰度 / 恢复流程未做压测与慢启动（slow ramp）设计
Why5 → 平台级自动化变更缺少强制爆炸半径校验（blast-radius guardrail）
        （系统性原因：共享平台依赖 + 无边界变更 = 全局控制面脆弱性）
```

## 解决过程

先在部分区域回滚策略恢复匿名读验证有效，再全球推广；身份后端采取缓慢放量（ramp）消化积压令牌请求，避免再次过载。两阶段处置均有效，但串行发生使总时长翻倍。

## 经验教训

1. 当平台存在"客户不可见但全平台依赖"的共享组件（托管存储、令牌后端）时，必须**为这些依赖单独建模并注册为关键依赖项**，否则合规修复会无意中拆除承重墙。
2. 当自动化修复工作流的目标定位依赖数据同步时，必须**在执行前对目标集合做快照比对与超限熔断**（如"影响账户数 > N 即中止"），否则同步故障 = 无限爆炸半径。
3. 当恢复一个大流量控制面服务时，必须**用慢启动放量 + 队列背压消化积压**，否则恢复本身就是下一次事故的触发器（本案例二次击穿的直接原因）。
4. 当设计安全修复（如禁用匿名访问）时，必须**先枚举该行为的全部合法消费者**再动手；安全默认值不能替代依赖审计。

## 预防与改进措施

- **预防（Prevent）**：平台级变更强制爆炸半径校验与分账户灰度；共享依赖注册与依赖审计
- **减小爆炸半径（Contain）**：托管存储按区域/分区隔离；策略变更按目标集合分批执行
- **快速检测（Detect）**：目标集合漂移检测（执行前快照比对）；控制面操作成功率的全球异常告警
- **快速恢复（Recover）**：恢复流程内置慢启动放量模板；令牌后端按"恢复洪峰"场景压测（建议）

## SRE / CRE / FDE / SA 视角速览

| 视角 | 本案例核心结论 |
|---|---|
| SRE | 共享平台依赖的变更必须有爆炸半径熔断；"恢复即负载"必须纳入容量模型 |
| CRE | 控制面事故的客户感知是"不能变"而非"不能用"——沟通话术需区分，但依赖自动扩缩容的客户实际受损与数据面同级 |
| FDE | 目标定位逻辑的数据同步故障是本案隐形凶手——自动化系统的元数据（定位数据）本身需要健康度监控 |
| SA（客情危机） | 合规修复引发全球故障，客户对"平台安全整改"的信任受冲击；单一 Tracking ID 串两阶段事故，便于客户索赔定损 |
| SA（技术危机） | 客户侧教训：不要把 CI/CD 与弹性伸缩绑定在单一云控制面上；保留自托管 runner 与跨云预案作为 break-glass 手段 |

## 参考资料

1. [Azure status history（官方）](https://azure.status.microsoft/en-us/status/history/) — E1/E2
2. [Azure Outage February 2026 VM Provisioning and Identity Service Disruption（WindowsForum 梳理）](https://windowsforum.com/threads/azure-outage-february-2026-vm-provisioning-and-identity-service-disruption.400049/post-956912) — E3
3. [Azure Outage Cost - History（FNJ8-VQZ 摘要）](https://outagecost.com/azure-outages) — E3
