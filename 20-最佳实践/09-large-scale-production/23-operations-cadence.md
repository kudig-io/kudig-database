---
title: Day-2 运营节奏与值班体系
description: 大规模 Kubernetes 集群 Day-2 运营总纲：日/周/月/季/年例行操作日历、值班体系与升级路径、巡检自动化、Runbook 维护制度，与专题各文档的执行映射
summary: 把专题全部"做什么"落到"什么时候做、谁来做"：运营日历 × 值班体系 × 巡检自动化 × 文档维护制度
category: references
tags:
- k8s
- operations
- oncall
- sre
- production
tier: core
created: '2026-08-04'
last_updated: '2026-08-04'
difficulty: intermediate
audience:
- SRE
- 运维负责人
estimated_read_time: 15min
---

# Day-2 运营节奏与值班体系

> 本专题其他文档回答"做什么、怎么做"，本文回答**"什么时候做、谁来做、如何不漏"**。大规模集群的可靠性 = 正确的实践 × 稳定的执行节奏。

## 1. 运营日历（例行操作节奏）

### 每日（值班巡检，目标 30 分钟内完成）

| 项 | 内容 | 依据 |
|---|---|---|
| 控制面健康 | apiserver P99、etcd 延迟/Leader、scheduler 积压巡检 | [[09-observability#2. 控制面黄金指标（大规模必配告警）]] |
| 告警消化 | 夜间告警清零与归因，P1/P2 必须当日闭环 | [[09-observability#4. 告警体系设计]] |
| 容量水位 | 集群 allocatable 使用率、IPAM、PVC 水位扫一眼 | [[11-autoscaling-capacity#5. 容量规划方法论]] |
| 异常对象 | Pending Pod、频繁重启 Pod、NotReady 节点 | [[17-incident-playbooks]] |
| 证书有效期 | 自动巡检结果确认（< 90 天进入处理流程） | [[13-upgrade-certificate-runbook#6. 证书生命周期管理]] |

### 每周

| 项 | 内容 |
|---|---|
| 告警回顾 | 触发 Top 10 告警根因分析：修根因或调阈值 |
| 备份验证 | etcd 快照、Velero 任务成功率检查（非恢复演练，只看任务） |
| 安全巡检 | 新增公网入口、新增宽权 RBAC、新增特权 Pod 扫描（[[08-security-defense-checklist#9. 快速自查命令集]]） |
| 重平衡 | descheduler 低峰执行（[[22-node-lifecycle#6. 资源碎片与重平衡]]） |
| 发布质量 | 本周变更失败率、回滚次数 review（[[21-release-engineering#6. 发布可观测性与质量度量]]） |

### 每月

| 项 | 内容 |
|---|---|
| 成本评审 | 分团队账单、Top 浪费项、优化认领（[[14-cost-finops#5. 运营节奏（FinOps 是流程不是工具）]]） |
| 容量评审 | 水位趋势、扩容预测、云配额余量 |
| 补丁窗口 | 节点月度补丁轮换（[[22-node-lifecycle#4. OS 补丁与内核升级]]） |
| 单场景演练 | 从演练矩阵轮选一个场景执行（[[15-slo-chaos-engineering#4. GameDay 制度化]]） |
| 证书台账更新 | 到期时间登记核对 |

### 每季度

| 项 | 内容 |
|---|---|
| 清单回归 | 三份 checklist 全量回归：[[06-initialization-checklist]] / [[07-pre-production-checklist]] / [[08-security-defense-checklist]] |
| 恢复演练 | etcd 快照恢复 + Velero 单资源恢复（[[18-disaster-recovery-runbook#6. 演练制度（不演练 = 没有备份）]]） |
| 版本评估 | K8s 新版本评估与升级规划（[[13-upgrade-certificate-runbook#1. 版本策略]]） |
| right-sizing 回归 | VPA 建议复核应用（[[14-cost-finops#2. Right-sizing（收益最高的动作）]]） |
| 综合演练 | 复合故障 GameDay |
| 发布回滚演练 | 金丝雀自动回滚真实触发一次（[[21-release-engineering#2. 渐进式交付（金丝雀的工程化）]]） |

### 每半年 / 每年

| 周期 | 项 |
|---|---|
| 每半年 | 整集群重建演练（[[18-disaster-recovery-runbook#4. 整集群重建（S4）]]）；容灾切换演练（[[10-multi-cluster#5. 跨集群容灾与多活]]）；架构级成本审查 |
| 每年 | 全集群证书轮换实操演练；灾难恢复大考（实测 RTO/RPO）；CA 到期前 1 年启动轮换专项；专题文档全面复审 |

## 2. 值班体系

### 2.1 分层值守

| 层 | 角色 | 职责 | 响应 SLA |
|---|---|---|---|
| L1 | 值班工程师（oncall 轮值） | 告警响应、一线处置、按 Runbook 止血 | P1 5min / P2 30min |
| L2 | 领域专家（网络/存储/控制面） | L1 升级后的深度处置 | 被呼叫后 15min 上线 |
| L3 | 架构师/负责人 | 重大决策（回滚、切流、数据取舍） | 立即 |

### 2.2 值班制度要点

- 轮值周期 ≤ 1 周，交接必须有书面交班记录（进行中的问题、风险点、冻结日历）
- oncall 负载治理：每周 P1 > 3 次说明系统或告警有问题，触发专项改进——**值班痛苦是指示器不是荣誉**
- 护网/大促期间升级值守：双人值班 + 专家备勤（[[08-security-defense-checklist#8. 护网期间运行机制]]）
- 事件时间线制度：任何 P1/P2 处置过程实时记录，24h 内复盘，改进项进 backlog 跟踪到人（[[17-incident-playbooks#7. 通用处置纪律]]）

## 3. 巡检自动化

人巡检必然衰减，每日项必须自动化：

1. **巡检脚本化**：每日项封装为定时任务（CronJob/CI 定时流水线），输出结构化报告推送值班群
2. **报告即异常**：只报告异常项与趋势拐点，全绿报告一行字——值班注意力是稀缺资源
3. **巡检覆盖补充**：自动巡检未覆盖的项（如"镜像仓库 GC 状态"）每月评审补充进脚本
4. **台账在线化**：证书台账、网段表、账号权限清单、暴露面清单放共享文档/CMDB，变更即更新，季度核对

## 4. 文档与 Runbook 维护制度

- 本专题每份 Runbook（[[13-upgrade-certificate-runbook]] / [[17-incident-playbooks]] / [[18-disaster-recovery-runbook]]）**每次实战使用后必须更新偏差**——文档与现实脱节比没有文档更危险
- 新故障模式处置完成后 1 周内沉淀为新剧本或补充进既有剧本
- 专题文档年度全面复审（版本参数时效性、工具链更替、官方建议更新）
- 变更来源原则：事实性内容更新需附来源（官方文档/厂商公告/实战记录），登记进 [[16-references]]

## 5. 运营成熟度自评

| 级别 | 特征 |
|---|---|
| L1 救火式 | 故障驱动，无日历无值班，靠个人经验 |
| L2 规范化 | 有值班有日历，核心 Runbook 齐备，执行靠自觉 |
| 目标态 | 日历自动化执行、演练制度化、度量闭环（SLI/变更失败率/MTTR 趋势可见）、文档随实战进化 |

自评问题：上季度的恢复演练做了吗？最近一次 P1 的复盘改进项闭环了吗？证书台账上次更新是什么时候？——三个问题答不上来任何一个，运营体系就还在 L2 以下。

## Related

- [[09-observability|可观测性体系（巡检指标来源）]]
- [[15-slo-chaos-engineering|SLO 与混沌工程（演练节奏）]]
- [[22-node-lifecycle|节点生命周期与 OS 运维（补丁轮换）]]
- [[13-生产运维/README|生产运维域]]
