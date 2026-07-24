---
title: VPA 异常故障树分析 (skills)
description: COMP_UPD_OR --> COMP_UPD2[Updater 配置错误]
summary: COMP_UPD_OR --> COMP_UPD2[Updater 配置错误]
category: general
tags:
- k8s
- kubelet
- prometheus
- vpa
- pdb
- webhook
- rag
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- VPA 异常故障树分析 是什么
- 如何 VPA 异常故障树分析
trigger_keywords:
- VPA
- 异常故障树分析
prerequisites:
- kubectl-basics
- prometheus-basics
fta_id: FTA-VPA-001
component: Vpa
severity: medium
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "VPA 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get vpa -A -o jsonpath='{range .items[?(@.status.condition.Type!=\'Ready\')]} {.metadata.namespace}/{.metadata.name}{\'\n\'}{end}' 显示 VPA 异常 --> - **目标**..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["故障诊断/FTA故障树/list/vpa-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# VPA 异常故障树分析

<!-- condition: kubectl get vpa -A -o jsonpath='{range .items[?(@.status.condition.Type!=\"Ready\")]} {.metadata.namespace}/{.metadata.name}{\"\n\"}{end}' 显示 VPA 异常 -->

# VPA 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 VPA 推荐异常、驱逐误操作与指标缺失的关键成因与路径。
- **范围**：VPA 组件、指标采集、驱逐策略、目标对象与资源配额。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: VPA 异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> COMP[VPA 组件异常]
  OR0 --> MET[指标异常]
  OR0 --> REC[推荐异常]
  OR0 --> EVICT[驱逐异常]
  OR0 --> OBJ[目标对象异常]
  OR0 --> AUDIT[审计与回滚缺失]

  %% ========== 1. VPA 组件异常 ==========
  COMP_OR{{OR}}
  COMP --> COMP_OR
  COMP_OR --> COMP_REC[Recommender 异常]
  COMP_OR --> COMP_UPD[Updater 异常]
  COMP_OR --> COMP_ADM[Admission Controller 异常]

  COMP_REC_OR{{OR}}
  COMP_REC --> COMP_REC_OR
  COMP_REC_OR --> COMP_REC1[Recommender Pod 不可用]
  COMP_REC_OR --> COMP_REC2[Recommender OOM]

  COMP_UPD_OR{{OR}}
  COMP_UPD --> COMP_UPD_OR
  COMP_UPD_OR --> COMP_UPD1[Updater Pod 不可用]
  COMP_UPD_OR --> COMP_UPD2[Updater 配置错误]

  COMP_ADM_OR{{OR}}
  COMP_ADM --> COMP_ADM_OR
  COMP_ADM_OR --> COMP_ADM1[Admission Controller 不可用]
  COMP_ADM_OR --> COMP_ADM2[Webhook 证书过期]

  %% ========== 2. 指标异常 ==========
  MET_OR{{OR}}
  MET --> MET_OR
  MET_OR --> MET_SRV[Metrics Server 异常]
  MET_OR --> MET_HIST[历史指标异常]
  MET_OR --> MET_PROM[Prometheus 异常]

  MET_SRV_OR{{OR}}
  MET_SRV --> MET_SRV_OR
  MET_SRV_OR --> MET_SRV1[Metrics Server 不可用]
  MET_SRV_OR --> MET_SRV2[指标采集延迟]
  MET_SRV_OR --> MET_SRV3[kubelet 指标 API 异常]

  MET_HIST_OR{{OR}}
  MET_HIST --> MET_HIST_OR
  MET_HIST_OR --> MET_HIST1[历史数据不足]
  MET_HIST_OR --> MET_HIST2[Checkpoint 丢失]

  %% AND 门：指标不可用 + updateMode=Auto
  AND_MET{{"AND: 指标不可用 + Auto 模式"}}
  MET --> AND_MET
  AND_MET --> AND_MET1[Metrics Server 不可用]
  AND_MET --> AND_MET2[VPA updateMode 为 Auto]

  %% ========== 3. 推荐异常 ==========
  REC_OR{{OR}}
  REC --> REC_OR
  REC_OR --> REC_VAL[推荐值异常]
  REC_OR --> REC_CONF[推荐配置异常]
  REC_OR --> REC_ALGO[算法异常]

  REC_VAL_OR{{OR}}
  REC_VAL --> REC_VAL_OR
  REC_VAL_OR --> REC_VAL1[推荐值过高]
  REC_VAL_OR --> REC_VAL2[推荐值过低]
  REC_VAL_OR --> REC_VAL3[推荐值震荡]

  REC_CONF_OR{{OR}}
  REC_CONF --> REC_CONF_OR
  REC_CONF_OR --> REC_CONF1[minAllowed/maxAllowed 配置不当]
  REC_CONF_OR --> REC_CONF2[containerPolicies 冲突]

  %% ========== 4. 驱逐异常 ==========
  EVICT_OR{{OR}}
  EVICT --> EVICT_OR
  EVICT_OR --> EVICT_POL[驱逐策略异常]
  EVICT_OR --> EVICT_EXEC[驱逐执行异常]
  EVICT_OR --> EVICT_IMPACT[驱逐影响异常]

  EVICT_POL_OR{{OR}}
  EVICT_POL --> EVICT_POL_OR
  EVICT_POL_OR --> EVICT_POL1[驱逐过于频繁]
  EVICT_POL_OR --> EVICT_POL2[minReplicas 配置错误]

  EVICT_EXEC_OR{{OR}}
  EVICT_EXEC --> EVICT_EXEC_OR
  EVICT_EXEC_OR --> EVICT_EXEC1[PDB 阻塞驱逐]
  EVICT_EXEC_OR --> EVICT_EXEC2[驱逐超时]

  %% AND 门：驱逐触发 + PDB 阻塞
  AND_EVICT{{"AND: 驱逐触发 + PDB 阻塞"}}
  EVICT_EXEC --> AND_EVICT
  AND_EVICT --> AND_EVICT1[VPA 触发驱逐更新]
  AND_EVICT --> AND_EVICT2[PDB 不允许驱逐]

  %% ========== 5. 目标对象异常 ==========
  OBJ_OR{{OR}}
  OBJ --> OBJ_OR
  OBJ_OR --> OBJ_TARGET[目标

## 生产案例

### 案例 1: VPA 推荐值过低导致 Pod 频繁 OOMKill

| 时间 | 事件 |
|------|------|
| 14:00 | 启用 VPA Auto 模式，Pod 反复 OOMKill |
| 14:05 | `kubectl get vpa app-vpa -o yaml` 显示推荐 memory 128Mi |
| 14:08 | 实际应用需要 512Mi，VPA 基于历史数据推荐过低 |
| 14:10 | 🟡 设置 minAllowed.memory=512Mi，VPA 重新计算 |

**根因**: VPA 基于短期历史数据推荐，未考虑业务峰值内存需求。

### 案例 2: VPA 与 HPA 同时基于 CPU 导致冲突

**现象**: HPA 扩容副本，VPA 调整单 Pod 资源，两者振荡。

**诊断**: VPA targetRef 与 HPA scaleTargetRef 指向同一 Deployment

**修复**: 🟡 VPA 设为 Off(仅推荐)模式，或 HPA 改用自定义指标

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | VPA 导致 Pod 频繁重启 | 切换为 Off 模式 |
| P1 | 推荐值不合理 | 调整 minAllowed/maxAllowed |
| P2 | VPA 与 HPA 冲突 | 分离指标源 |

## 面试要点

1. **Q: VPA 的三种更新模式？**
   A: Off: 仅推荐不执行；Initial: 仅 Pod 创建时应用；Auto: 自动驱逐并重建 Pod 应用新资源。生产建议先 Off 观察，确认合理后切 Auto。

2. **Q: VPA 的推荐算法基于什么？**
   A: 基于历史资源使用数据，使用指数衰减直方图(Exponentially Weighted Histogram)，默认取 P90 作为推荐值，确保 90% 时间资源充足。

3. **Q: VPA 与 HPA 能否同时使用？**
   A: 不能基于相同指标(cpu/memory)同时使用。可以: VPA 管理 memory + HPA 基于自定义指标(QPS)管理副本数。或使用 MultidimPodAutoscaler(Alpha)。

## 相关链接

- [[技能/工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## See Also

- [[技能/存储/csi-storage/诊断排障/ts-storage.md|ts-storage]]
- [[技能/工作负载/pod/诊断排障/ts-workloads.md|ts-workloads]]
- [[技能/安全/webhook-admission/webhook-admission-fta.md|webhook-admission-fta]]
- [[技能/工作负载/pod/方法论/agent/Agent Orchestration Patterns.md|Agent Orchestration Patterns]]


<!-- risk-assessed -->
