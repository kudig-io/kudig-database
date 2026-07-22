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
tier: core
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

### 案例1: PDB 阻止节点维护导致升级卡住

**时间线**:
- 22:00 计划节点滚动升级，执行 `kubectl drain node-1`
- 22:05 drain 卡住，提示 `Cannot evict pod as it would violate the pod's disruption budget`
- 22:10 确认根因: PDB minAvailable=3 但只有 3 个副本，无法驱逐任何 Pod
- 22:15 临时调整 PDB 后 drain 成功

**根因链**:
```
节点维护drain → 尝试驱逐Pod → PDB minAvailable=3
→ 当前可用=3，驱逐后<3 → 违反PDB → 驱逐被拒绝 → drain卡住
```

**修复**:
```bash
# 🟢 检查 PDB 状态
kubectl get pdb -A -o wide
kubectl describe pdb ${PDB_NAME} -n ${NS}
# 🟡 临时调整 PDB (维护窗口)
kubectl patch pdb ${PDB_NAME} -n ${NS} -p '{"spec":{"minAvailable":1}}'
# 维护完成后恢复
kubectl patch pdb ${PDB_NAME} -n ${NS} -p '{"spec":{"minAvailable":3}}'
```

### 案例2: PDB 配置错误导致服务不可用

**现象**: 滚动更新时所有 Pod 同时被终止，服务短暂不可用

**根因**: 未配置 PDB，且 maxUnavailable 设置过大

**修复**:
```bash
# 🟡 创建 PDB
kubectl apply -f - <<EOF
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: ${DEPLOY}-pdb
  namespace: ${NS}
spec:
  minAvailable: "50%"
  selector:
    matchLabels:
      app: ${DEPLOY}
EOF
```

## 预防与监控

### 告警规则

```yaml
groups:
- name: pdb-alerts
  rules:
  - alert: PDBBlockingEviction
    expr: kube_poddisruptionbudget_status_disruptions_allowed == 0
    for: 30m
    labels:
      severity: warning
  - alert: PDBNotHealthy
    expr: kube_poddisruptionbudget_status_current_healthy < kube_poddisruptionbudget_status_desired_healthy
    for: 15m
    labels:
      severity: warning
```

### 预防措施

| 措施 | 说明 | 优先级 |
|------|------|--------|
| 关键服务必配 PDB | 生产服务必须配置 PDB | P0 |
| 副本数与 PDB 匹配 | 确保副本数 > minAvailable | P0 |
| 维护窗口预案 | drain 前检查 PDB 状态 | P1 |
| 使用百分比 | minAvailable: "50%" 更灵活 | P1 |

## 面试要点

1. **Q: PDB 的作用和限制？**
   A: 保护自愿驱逐(节点维护/升级)时最小可用数；不影响非自愿驱逐(节点宕机/OOM)；不能阻止所有 Pod 被终止

2. **Q: PDB 与 maxUnavailable 的关系？**
   A: PDB 是集群级保护，maxUnavailable 是 Deployment 级；两者取更严格的约束；PDB 优先级更高

3. **Q: drain 被 PDB 阻塞的处理？**
   A: 检查 PDB 状态 → 确认当前可用副本数 → 临时调整 PDB → 或先扩容副本数 → drain 完成后恢复

## 相关链接

- [[技能/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[技能/skills-run-README.md|skills-run-README]] — Skills Demo — 本地运行工单诊断技能
- [[技能/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]] — Symptom Vector Matching Engine
- [[技能/skill-reference-root-cause-catalog.md|skill-reference-root-cause-catalog]] — Root Cause Catalog
- [[gpu-fta]] — GPU 异常故障树分析
- [[技能/ts-workloads.md|ts-workloads]] — 工作负载故障排查

- [[故障诊断/FTA故障树/list/pdb-fta.md|PDB 异常故障树分析]]
- [[技能/skill-reference-remediation-playbook.md|Remediation Playbook]] — Cross-reference
- [[技能/assessment-daily-check-quiz.md|Daily Check Quiz]] — Cross-reference


<!-- risk-assessed -->
