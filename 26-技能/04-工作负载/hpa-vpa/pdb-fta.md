---
title: PDB 异常故障树分析 (skills)
description: '- **目标**：覆盖 PDB 阻塞驱逐、配置错误与升级失败的关键成因与路径。'
summary: '- **目标**：覆盖 PDB 阻塞驱逐、配置错误与升级失败的关键成因与路径。'
category: skills
tags:
- k8s
- fta
- troubleshooting
- pdb
- statefulset
- gpu
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- PDB 异常故障树分析 是什么
- 如何 PDB 异常故障树分析
trigger_keywords:
- PDB
- 异常故障树分析
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
fta_id: FTA-PDB-001
component: Pdb
severity: medium
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# PDB 异常故障树分析

<!-- condition: kubectl get events -A | grep -E 'CannotEvict|PdbViolations|Eviction' 显示 PDB 相关阻止事件 -->

# PDB 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 PDB 阻塞驱逐、配置错误与升级失败的关键成因与路径。
- **范围**：PDB 配置、驱逐控制器、滚动升级与维护窗口、控制面依赖。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: PDB 异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> CONF[配置错误]
  OR0 --> EVICT[驱逐异常]
  OR0 --> UP[升级/维护异常]
  OR0 --> CTRL[控制面异常]
  OR0 --> AUDIT[审计与回滚缺失]

  %% ========== 1. 配置错误 ==========
  CONF_OR{{OR}}
  CONF --> CONF_OR
  CONF_OR --> CONF_MIN[minAvailable 异常]
  CONF_OR --> CONF_MAX[maxUnavailable 异常]
  CONF_OR --> CONF_SEL[selector 异常]

  CONF_MIN_OR{{OR}}
  CONF_MIN --> CONF_MIN_OR
  CONF_MIN_OR --> CONF_MIN1[minAvailable 过高]
  CONF_MIN_OR --> CONF_MIN2[minAvailable 为百分比计算错误]

  CONF_MAX_OR{{OR}}
  CONF_MAX --> CONF_MAX_OR
  CONF_MAX_OR --> CONF_MAX1[maxUnavailable 过低]
  CONF_MAX_OR --> CONF_MAX2[maxUnavailable 为 0]

  CONF_SEL_OR{{OR}}
  CONF_SEL --> CONF_SEL_OR
  CONF_SEL_OR --> CONF_SEL1[selector 不匹配任何 Pod]
  CONF_SEL_OR --> CONF_SEL2[selector 匹配范围过宽]

  %% AND 门：minAvailable=replicas + maxUnavailable=0
  AND_CONF{{"AND: 无法驱逐任何 Pod"}}
  CONF --> AND_CONF
  AND_CONF --> AND_CONF1[minAvailable >= 当前副本数]
  AND_CONF --> AND_CONF2[maxUnavailable = 0 或未设置]

  %% ========== 2. 驱逐异常 ==========
  EVICT_OR{{OR}}
  EVICT --> EVICT_OR
  EVICT_OR --> EVICT_REJ[驱逐被拒绝]
  EVICT_OR --> EVICT_STATE[状态不一致]
  EVICT_OR --> EVICT_DEAD[死锁]

  EVICT_REJ_OR{{OR}}
  EVICT_REJ --> EVICT_REJ_OR
  EVICT_REJ_OR --> EVICT_REJ1[disruptionsAllowed 为 0]
  EVICT_REJ_OR --> EVICT_REJ2[Pod 不健康但计入可用]

  EVICT_STATE_OR{{OR}}
  EVICT_STATE --> EVICT_STATE_OR
  EVICT_STATE_OR --> EVICT_STATE1[currentHealthy 计数错误]
  EVICT_STATE_OR --> EVICT_STATE2[expectedPods 与实际不符]

  EVICT_DEAD_OR{{OR}}
  EVICT_DEAD --> EVICT_DEAD_OR
  EVICT_DEAD_OR --> EVICT_DEAD1[Pod 卡在 Terminating]
  EVICT_DEAD_OR --> EVICT_DEAD2[新 Pod 无法调度]

  %% AND 门：Pod Terminating + 新 Pod 无法调度
  AND_DEAD{{"AND: 驱逐死锁"}}
  EVICT_DEAD --> AND_DEAD
  AND_DEAD --> AND_DEAD1[旧 Pod 卡在 Terminating]
  AND_DEAD --> AND_DEAD2[新 Pod 无法调度导致健康数不足]

  %% ========== 3. 升级/维护异常 ==========
  UP_OR{{OR}}
  UP --> UP_OR
  UP_OR --> UP_ROLL[滚动升级异常]
  UP_OR --> UP_DRAIN[节点 Drain 异常]
  UP_OR --> UP_WINDOW[维护窗口异常]

  UP_ROLL_OR{{OR}}
  UP_ROLL --> UP_ROLL_OR
  UP_ROLL_OR --> UP_ROLL1[Deployment 更新被阻塞]
  UP_ROLL_OR --> UP_ROLL2[StatefulSet 更新卡住]

  UP_DRAIN_OR{{OR}}
  UP_DRAIN --> UP_DRAIN_OR
  UP_DRAIN_OR --> UP_DRAIN1[kubectl drain 超时]
  UP_DRAIN_OR --> UP_DRAIN2[CA 缩容被阻塞]

  UP_WINDOW_OR{{OR}}
  UP_WINDOW --> UP_WINDOW_OR
  UP_WINDOW_OR --> UP_WINDOW1[维护窗口配置缺失]
  UP_WINDOW_OR --> UP_WINDOW2[维护期间 PDB 未调整]

  %% ========== 4. 控制面异常 ==========
  CTRL_OR{{OR}}
  CTRL --> CTRL_OR
  CTRL_OR --> CTRL_API[API Server 异常]
  CTRL_OR --> CTRL_DC[Disruption Controller 异常]

  CTRL_API_OR{{OR}}
  CTRL_API --> CTRL_API_OR
  CTRL_API_OR --> CTRL_API1[Eviction API 超时]
  CTRL_API_OR --> CTRL_API2[API Server 过载]

  CTRL_DC_OR{{OR}}
  CTR

## 生产案例

### 案例 1: PDB 配置不当导致节点维护无法完成

| 时间 | 事件 |
|------|------|
| 02:00 | 执行 `kubectl drain node-1` 卡住，等待 Pod 驱逐 |
| 02:05 | 提示 "Cannot evict pod as it would violate the pod's PodDisruptionBudget" |
| 02:08 | PDB minAvailable=3 但只有 3 个副本，无法驱逐任何 Pod |
| 02:10 | 🟡 临时调整 PDB minAvailable=2，完成 drain |

**根因**: PDB minAvailable 设置等于副本数，导致任何驱逐都被拒绝。

### 案例 2: 缺少 PDB 导致滚动更新期间服务中断

**现象**: 节点维护时所有 Pod 同时被驱逐，服务完全中断。

**诊断**: 未配置 PDB，驱逐无保护

**修复**: 🟢 添加 PDB: maxUnavailable=1，确保至少 N-1 个 Pod 可用

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | PDB 阻止关键维护操作 | 临时调整 PDB |
| P1 | 缺少 PDB 导致服务中断 | 添加 PDB |
| P2 | PDB 配置不合理 | 审查并调整 |

## 面试要点

1. **Q: PDB 的 minAvailable 与 maxUnavailable 如何选择？**
   A: minAvailable: 保证最少可用数(适合关键服务)；maxUnavailable: 允许最多不可用数(适合可降级服务)。例如 5 副本: minAvailable=4 或 maxUnavailable=1 等价。

2. **Q: PDB 保护哪些操作？**
   A: 保护自愿驱逐(voluntary disruption): kubectl drain、节点维护、集群缩容。不保护非自愿驱逐: 节点宕机、OOMKill、硬件故障。

3. **Q: PDB 与 Cluster Autoscaler 的交互？**
   A: CA 缩容时会检查 PDB，如果驱逐 Pod 会违反 PDB 则跳过该节点。设置 `cluster-autoscaler.kubernetes.io/safe-to-evict: "true"` 可覆盖 PDB 保护。

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[26-技能/04-工作负载/pod/方法论/skills-run-README.md|skills-run-README]] — Skills Demo — 本地运行工单诊断技能
- [[26-技能/04-工作负载/pod/方法论/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]] — Symptom Vector Matching Engine
- [[26-技能/04-工作负载/pod/方法论/skill-reference-root-cause-catalog.md|skill-reference-root-cause-catalog]] — Root Cause Catalog
- [[gpu-fta]] — GPU 异常故障树分析
- [[26-技能/04-工作负载/pod/诊断排障/ts-workloads.md|ts-workloads]] — 工作负载故障排查

- [[19-故障诊断/06-FTA故障树/list/pdb-fta.md|PDB 异常故障树分析]]
- [[26-技能/04-工作负载/pod/方法论/skill-reference-remediation-playbook.md|Remediation Playbook]] — Cross-reference
- [[26-技能/04-工作负载/pod/培训/测验/assessment-daily-check-quiz.md|Daily Check Quiz]] — Cross-reference


<!-- risk-assessed -->
