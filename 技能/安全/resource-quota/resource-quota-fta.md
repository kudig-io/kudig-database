---
title: ResourceQuota 异常故障树分析 (skills)
description: OR0 --> CONF[配置错误]
summary: OR0 --> CONF[配置错误]
category: skills
tags:
- k8s
- fta
- troubleshooting
- etcd
- calico
- hpa
- job
- webhook
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
- ResourceQuota 异常故障树分析 是什么
- 如何 ResourceQuota 异常故障树分析
trigger_keywords:
- ResourceQuota
- 异常故障树分析
prerequisites:
- kubectl-basics
- cni-basics
- etcd-basics
fta_id: FTA-RESOURCE_QUOTA-001
component: Resource Quota
severity: medium
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# ResourceQuota 异常故障树分析

<!-- condition: kubectl get events -A | grep -E 'exceeded quota|forbidden.*quota' 显示配额超限 -->

# [[技能/resource-quota-fta.md|ResourceQuota 异常 FTA 树]]

## 适用范围与说明
- **目标**：覆盖资源配额耗尽、配额计算异常与误拦截的关键成因与路径。
- **范围**：命名空间配额、LimitRange、资源请求/限制、控制面与审计。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: 资源配额异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> QUO[配额耗尽]
  OR0 --> CALC[配额计算异常]
  OR0 --> CONF[配置错误]
  OR0 --> CTRL[控制面异常]
  OR0 --> AUDIT[审计与回滚缺失]

  %% ========== 1. 配额耗尽 ==========
  QUO_OR{{OR}}
  QUO --> QUO_OR
  QUO_OR --> QUO_BURST[突发请求耗尽]
  QUO_OR --> QUO_LEAK[资源泄漏/未释放]
  QUO_OR --> QUO_SCOPE[配额范围不足]

  %% 1.1 突发请求耗尽
  QUO_BURST_OR{{OR}}
  QUO_BURST --> QUO_BURST_OR
  QUO_BURST_OR --> QUO_BURST1[批量 Pod 创建]
  QUO_BURST_OR --> QUO_BURST2[Job 并发过高]
  QUO_BURST_OR --> QUO_BURST3[HPA 扩容触发]

  %% 1.2 资源泄漏/未释放
  QUO_LEAK_OR{{OR}}
  QUO_LEAK --> QUO_LEAK_OR
  QUO_LEAK_OR --> QUO_LEAK1[Completed Pod 未清理]
  QUO_LEAK_OR --> QUO_LEAK2[孤儿 PVC 未释放]
  QUO_LEAK_OR --> QUO_LEAK3[终止中资源占用配额]

  %% 1.3 配额范围不足
  QUO_SCOPE_OR{{OR}}
  QUO_SCOPE --> QUO_SCOPE_OR
  QUO_SCOPE_OR --> QUO_SCOPE1[CPU/Memory 配额过低]
  QUO_SCOPE_OR --> QUO_SCOPE2[Pod 数量配额过低]
  QUO_SCOPE_OR --> QUO_SCOPE3[PVC 数量/容量配额过低]

  %% ========== 2. 配额计算异常 ==========
  CALC_OR{{OR}}
  CALC --> CALC_OR
  CALC_OR --> CALC_DELAY[统计延迟]
  CALC_OR --> CALC_DRIFT[状态漂移]
  CALC_OR --> CALC_SCOPE[作用域异常]

  %% 2.1 统计延迟
  CALC_DELAY_OR{{OR}}
  CALC_DELAY --> CALC_DELAY_OR
  CALC_DELAY_OR --> CALC_DELAY1[Controller 同步延迟]
  CALC_DELAY_OR --> CALC_DELAY2[API Server 缓存延迟]
  CALC_DELAY_OR --> CALC_DELAY3[etcd Watch 延迟]

  %% 2.2 状态漂移
  CALC_DRIFT_OR{{OR}}
  CALC_DRIFT --> CALC_DRIFT_OR
  CALC_DRIFT_OR --> CALC_DRIFT1[配额计数与实际不符]
  CALC_DRIFT_OR --> CALC_DRIFT2[对象删除后配额未释放]

  %% AND 门：对象已删除 + 配额未释放
  AND_DRIFT{{"AND: 对象删除 + 配额未释放"}}
  CALC_DRIFT --> AND_DRIFT
  AND_DRIFT --> AND_DRIFT1[Pod/PVC 已删除]
  AND_DRIFT --> AND_DRIFT2[ResourceQuota.status.used 未更新]

  %% 2.3 作用域异常
  CALC_SCOPE_OR{{OR}}
  CALC_SCOPE --> CALC_SCOPE_OR
  CALC_SCOPE_OR --> CALC_SCOPE1[scopeSelector 配置错误]
  CALC_SCOPE_OR --> CALC_SCOPE2[priorityClass 配额计算错误]

  %% ========== 3. 配置错误 ==========
  CONF_OR{{OR}}
  CONF --> CONF_OR
  CONF_OR --> CONF_QUOTA[ResourceQuota 配置]
  CONF_OR --> CONF_LIMIT[LimitRange 配置]
  CONF_OR --> CONF_CONFLICT[配置冲突]

  %% 3.1 ResourceQuota 配置
  CONF_QUOTA_OR{{OR}}
  CONF_QUOTA --> CONF_QUOTA_OR
  CONF_QUOTA_OR --> CONF_QUOTA1[hard 限制设置过低]
  CONF_QUOTA_OR --> CONF_QUOTA2[资源类型配置错误]
  CONF_QUOTA_OR --> CONF_QUOTA3[跨命名空间配额不一致]

  %% 3.2 LimitRange 配置
  CONF_LIMIT_OR{{OR}}
  CONF_LIMIT --> CONF_LIMIT_OR
  CONF_LIMIT_OR --> CONF_LIMIT1[default 值设置不当]
  CONF_LIMIT_OR --> CONF_LIMIT2[min/max 范围过窄]
  CONF_LIMIT_OR --> CONF_LIMIT3[defaultRequest 与 limit 不匹配]

  %% AND 门：无 request + 无 LimitRange default
  AND_LIMIT{{"AND: 无 request + 无 default"}}
  CONF_LIMIT --> AND_LIMIT
  AND_LIMIT --> AND_LIMIT1[Pod 未指定 resources.requests]
  AND_LIMIT --> AND_LIMIT2[命名空间无 LimitRange default]

  %% 3.3 配置冲突
  CONF_CON

## 生产案例

### 案例 1: ResourceQuota 耗尽导致新 Pod 无法创建

| 时间 | 事件 |
|------|------|
| 15:00 | 扩容失败: "forbidden: exceeded quota: compute-quota" |
| 15:05 | `kubectl describe quota -n prod` 显示 pods: 50/50, cpu: 100/100 |
| 15:08 | 业务增长超出原始配额规划 |
| 15:10 | 🟡 调整 ResourceQuota 上限，或清理无用 Pod |

**根因**: 初始配额设置未考虑业务增长，未配置配额使用率告警。

### 案例 2: LimitRange 默认值导致 Pod 资源不足

**现象**: 新 Pod 启动后性能极差，CPU 被限制在 100m。

**诊断**: LimitRange 设置了 default cpu limit=100m，Pod 未显式指定资源

**修复**: 🟢 Pod 显式设置 resources，或调整 LimitRange 默认值

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 配额耗尽影响核心业务 | 紧急提升配额 |
| P1 | 部分资源创建被拒绝 | 检查配额使用情况 |
| P2 | 配额使用率 >80% | 规划扩容 |

## 面试要点

1. **Q: ResourceQuota 与 LimitRange 的区别？**
   A: ResourceQuota: namespace 级别的资源总量上限(pods/cpu/memory/services 等)；LimitRange: 单 Pod/Container 的资源默认值和范围约束(min/max/default)。两者配合使用。

2. **Q: ResourceQuota 支持哪些资源类型？**
   A: 计算资源(pods/cpu/memory)、存储资源(persistentvolumeclaims/requests.storage)、对象数量(services/secrets/configmaps/replicationcontrollers)、扩展资源(nvidia.com/gpu)。

3. **Q: 多租户场景下如何设计配额策略？**
   A: ① 每个团队一个 namespace + ResourceQuota ② LimitRange 设置合理默认值 ③ 配合 RBAC 限制配额修改权限 ④ 监控配额使用率 + 80% 告警 ⑤ 定期审查和调整。

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[webhook-admission-fta]] — Admission Webhook 异常 FTA 树
- [[calico-fta]] — Calico Fta
- [[技能/ts-gitops-devops.md|ts-gitops-devops]] — GitOps/DevOps 排查
- [[技能/Agent Orchestration Patterns.md|Agent Orchestration Patterns]] — Agent Orchestration Patterns for FTA
- [[service-fta]] — Service 异常故障树分析

- [[故障诊断/FTA故障树/list/resource-quota-fta.md|ResourceQuota 异常故障树分析]]
- [[技能/skills-run-README.md|Skills Demo — 本地运行工单诊断技能]] — Cross-reference


<!-- risk-assessed -->
