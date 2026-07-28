---
title: Cluster Autoscaler 异常故障树分析 (skills)
description: '<!-- condition: kubectl get pods -A --field-selector=status.phase=Pending
  -o jsonpath=''{range .items[?(@.spec.nodeName==null)]} {.metadata.namespace}/{.metadata.name}{\"\n\"}{end}''
  显示有未调度的 Pending Pod'
summary: '<!-- condition: kubectl get pods -A --field-selector=status.phase=Pending
  -o jsonpath=''{range .items[?(@.spec.nodeName==null)]} {.metadata.namespace}/{.metadata.name}{\"\n\"}{end}''
  显示有未调度的 Pending Pod'
category: general
tags:
- k8s
- kubelet
- cilium
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cluster Autoscaler 异常故障树分析 是什么
- 如何 Cluster Autoscaler 异常故障树分析
trigger_keywords:
- Cluster
- Autoscaler
- 异常故障树分析
prerequisites:
- kubectl-basics
- cilium-basics
fta_id: FTA-CLUSTER_AUTOSCALER-001
component: Cluster Autoscaler
severity: medium
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "Cluster Autoscaler 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get [[pods|pods]] -A --field-selector=status.phase=Pending -o jsonpath='{range .items[?(@.spec.nodeName==null)]} {.metadata.namespace}/{.metadata.name}{\'\n\'}{en..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["故障诊断/FTA故障树/list/cluster-autoscaler-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# Cluster Autoscaler 异常故障树分析

<!-- condition: kubectl get pods -A --field-selector=status.phase=Pending -o jsonpath='{range .items[?(@.spec.nodeName==null)]} {.metadata.namespace}/{.metadata.name}{\"\n\"}{end}' 显示有未调度的 Pending Pod -->

# Cluster Autoscaler 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖自动扩缩容失效、扩容延迟与误缩容的关键成因与路径。
- **范围**：CA 控制器、云平台 API、节点池/伸缩组、调度与配额。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: Cluster Autoscaler 异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> CA[CA 控制器异常]
  OR0 --> CLOUD[云平台 API 异常]
  OR0 --> NODEPOOL[节点池异常]
  OR0 --> SCHED[调度信号异常]
  OR0 --> QUO[配额与资源限制]
  OR0 --> AUDIT[审计与回滚缺失]

  %% ========== 1. CA 控制器异常 ==========
  CA_OR{{OR}}
  CA --> CA_OR
  CA_OR --> CA_PROC[CA 进程异常]
  CA_OR --> CA_CONF[配置错误]
  CA_OR --> CA_LOGIC[扩缩逻辑异常]

  %% 1.1 CA 进程异常
  CA_PROC_OR{{OR}}
  CA_PROC --> CA_PROC_OR
  CA_PROC_OR --> CA_PROC1[CA Pod 未运行]
  CA_PROC_OR --> CA_PROC2[CA OOM/资源不足]
  CA_PROC_OR --> CA_PROC3[CA Leader 选举失败]

  %% 1.2 配置错误
  CA_CONF_OR{{OR}}
  CA_CONF --> CA_CONF_OR
  CA_CONF_OR --> CA_CONF1[节点组配置错误]
  CA_CONF_OR --> CA_CONF2[扩缩范围配置错误]
  CA_CONF_OR --> CA_CONF3[云凭证配置错误]

  %% 1.3 扩缩逻辑异常
  CA_LOGIC_OR{{OR}}
  CA_LOGIC --> CA_LOGIC_OR
  CA_LOGIC_OR --> CA_LOGIC1[expander 策略不当]
  CA_LOGIC_OR --> CA_LOGIC2[scale-down 参数过激]
  CA_LOGIC_OR --> CA_LOGIC3[优先级配置冲突]

  %% ========== 2. 云平台 API 异常 ==========
  CLOUD_OR{{OR}}
  CLOUD --> CLOUD_OR
  CLOUD_OR --> CLOUD_API[API 调用异常]
  CLOUD_OR --> CLOUD_INST[实例异常]
  CLOUD_OR --> CLOUD_AUTH[认证授权异常]

  %% 2.1 API 调用异常
  CLOUD_API_OR{{OR}}
  CLOUD_API --> CLOUD_API_OR
  CLOUD_API_OR --> CLOUD_API1[API 限流]
  CLOUD_API_OR --> CLOUD_API2[API 超时]
  CLOUD_API_OR --> CLOUD_API3[API 返回错误]

  %% 2.2 实例异常
  CLOUD_INST_OR{{OR}}
  CLOUD_INST --> CLOUD_INST_OR
  CLOUD_INST_OR --> CLOUD_INST1[实例规格不可用]
  CLOUD_INST_OR --> CLOUD_INST2[可用区库存不足]
  CLOUD_INST_OR --> CLOUD_INST3[竞价实例被回收]

  %% AND 门：规格不可用 + 无备选规格
  AND_INST{{"AND: 规格不可用 + 无备选"}}
  CLOUD_INST1 --> AND_INST
  AND_INST --> AND_INST1[主规格库存不足]
  AND_INST --> AND_INST2[未配置备选规格]

  %% 2.3 认证授权异常
  CLOUD_AUTH_OR{{OR}}
  CLOUD_AUTH --> CLOUD_AUTH_OR
  CLOUD_AUTH_OR --> CLOUD_AUTH1[AccessKey 过期/错误]
  CLOUD_AUTH_OR --> CLOUD_AUTH2[RAM/IAM 权限不足]
  CLOUD_AUTH_OR --> CLOUD_AUTH3[ServiceAccount Token 异常]

  %% ========== 3. 节点池异常 ==========
  NODEPOOL_OR{{OR}}
  NODEPOOL --> NODEPOOL_OR
  NODEPOOL_OR --> NP_SCALE[扩容失败]
  NODEPOOL_OR --> NP_INIT[初始化失败]
  NODEPOOL_OR --> NP_JOIN[节点加入失败]

  %% 3.1 扩容失败
  NP_SCALE_OR{{OR}}
  NP_SCALE --> NP_SCALE_OR
  NP_SCALE_OR --> NP_SCALE1[节点池已达上限]
  NP_SCALE_OR --> NP_SCALE2[伸缩组异常]
  NP_SCALE_OR --> NP_SCALE3[扩容请求被拒绝]

  %% 3.2 初始化失败
  NP_INIT_OR{{OR}}
  NP_INIT --> NP_INIT_OR
  NP_INIT_OR --> NP_INIT1[bootstrap 脚本失败]
  NP_INIT_OR --> NP_INIT2[kubelet 启动失败]
  NP_INIT_OR --> NP_INIT3[网络配置失败]

  %% 3.3 节点加入失败
  NP_JOIN_OR{{OR}}
  NP_JOIN --> NP_JOIN_OR
  NP_JOIN_OR --> NP_JOIN1[无法连接 API Server]
  NP_JOIN_OR --> NP_JOIN2[CSR 审批超时]
  NP_JOIN_OR --> NP_JOIN3[节点注册超时]

  %% ========== 4. 调度信号异常

## 生产案例

### 案例 1: Cluster Autoscaler 无法扩容——云 API 配额耗尽

| 时间 | 事件 |
|------|------|
| 22:00 | HPA 触发扩容，Pod Pending: Insufficient cpu |
| 22:05 | `kubectl logs -n kube-system -l app=cluster-autoscaler` 显示 "failed to increase node group size: quota exceeded" |
| 22:10 | 云控制台提升 ECS 实例配额 |
| 22:15 | Autoscaler 自动重试，新节点加入 |

**根因**: 云账号 vCPU 配额未随业务增长提前调整。

### 案例 2: 节点缩容导致有状态 Pod 被驱逐

**现象**: 缩容时数据库 Pod 被驱逐，服务中断 30s。

**诊断**: 未配置 `cluster-autoscaler.kubernetes.io/safe-to-evict: "false"` annotation

**修复**: 🟢 为有状态 Pod 添加 safe-to-evict=false，或配置 PDB

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 扩容失败且业务受损 | 手动创建节点 + 检查云配额 |
| P1 | Autoscaler Pod 异常 | 检查 RBAC 和云凭据 |
| P2 | 缩容过于激进 | 调整 scale-down-utilization-threshold |

## 面试要点

1. **Q: Cluster Autoscaler 的扩容触发条件？**
   A: 当存在 Pending Pod 且调度器确认无可用节点时，Autoscaler 模拟调度找到合适的 NodeGroup，调用云 API 扩容。触发条件: Pod 有 FailedScheduling 事件 + 现有节点无法容纳。

2. **Q: 缩容的决策逻辑？**
   A: 节点利用率 < threshold(默认 50%) 持续 scale-down-delay(默认 10min)，且无 PDB 保护、无 local storage、非 kube-system 关键 Pod，则标记为可缩容，驱逐 Pod 后删除节点。

3. **Q: Cluster Autoscaler 与 Karpenter 的区别？**
   A: CA: 基于 NodeGroup/ASG 扩缩，粒度粗；Karpenter: 直接创建最优实例，无 NodeGroup 概念，支持更灵活的实例类型选择和更快的扩容速度。

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## See Also

- [[26-技能/05-网络/cni/cilium-fta.md|cilium-fta]]
- [[26-技能/01-集群运维/cloud-provider/cloud-provider-fta.md|cloud-provider-fta]]
- [[26-技能/01-集群运维/cluster-upgrade/cluster-upgrade-fta.md|cluster-upgrade-fta]]
- [[26-技能/04-工作负载/pod/运维操作/configure-health-probes.md|configure-health-probes]]


<!-- risk-assessed -->
