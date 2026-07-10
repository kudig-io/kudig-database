---
title: Controller Manager 异常故障树分析 (skills)
description: OR0 --> DEP[依赖与存储异常]
summary: OR0 --> DEP[依赖与存储异常]
category: general
tags:
- k8s
- etcd
- controller-manager
- rbac
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Controller Manager 异常故障树分析 是什么
- 如何 Controller Manager 异常故障树分析
trigger_keywords:
- Controller
- Manager
- 异常故障树分析
prerequisites:
- kubectl-basics
- etcd-basics
fta_id: FTA-CONTROLLER_MANAGER-001
component: Controller Manager
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "Controller Manager 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get [[Pods|pods]] -n kube-system -l component=kube-controller-manager -o jsonpath='{range .items[?(@.status.phase!='Running')]} {.metadata.name}{\'\n\'}{end}' 显示 ..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["故障诊断/topic-fta/list/controller-manager-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# Controller Manager 异常故障树分析

<!-- condition: kubectl get pods -n kube-system -l component=kube-controller-manager -o jsonpath='{range .items[?(@.status.phase!="Running")]} {.metadata.name}{\"\n\"}{end}' 显示 Controller Manager 异常 -->

# Controller Manager 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖控制器失效、控制循环中断与资源状态漂移的关键成因与路径。
- **范围**：控制器进程、Leader 选举、资源配额与扩缩容、对象生命周期、依赖组件。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: Controller Manager 异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> SVC[控制器服务异常]
  OR0 --> LE[Leader 选举异常]
  OR0 --> LOOP[控制循环异常]
  OR0 --> DEP[依赖与存储异常]
  OR0 --> RES[资源管理异常]

  %% 控制器服务异常分支 - 扩展到3-4层
  SVC_OR{{OR}}
  SVC --> SVC_OR
  SVC_OR --> SVC1[进程崩溃/不可用]
  SVC_OR --> SVC2[资源不足导致卡顿]
  SVC_OR --> SVC3[配置加载失败]

  SVC1_OR{{OR}}
  SVC1 --> SVC1_OR
  SVC1_OR --> SVC1A[OOMKilled]
  SVC1_OR --> SVC1B[探针失败重启]
  SVC1_OR --> SVC1C[panic 崩溃]

  SVC2_OR{{OR}}
  SVC2 --> SVC2_OR
  SVC2_OR --> SVC2A[CPU 限流]
  SVC2_OR --> SVC2B[内存压力]
  SVC2_OR --> SVC2C[控制面节点资源不足]

  SVC3_OR{{OR}}
  SVC3 --> SVC3_OR
  SVC3_OR --> SVC3A[参数配置错误]
  SVC3_OR --> SVC3B[证书/kubeconfig 错误]

  %% Leader 选举异常分支 - 扩展到3-4层 + AND 门
  LE_OR{{OR}}
  LE --> LE_OR
  LE_OR --> LE1[选举锁问题]
  LE_OR --> LE2[API Server 连接问题]
  LE_OR --> LE3[多实例冲突]

  LE1_AND{{AND}}
  LE1 --> LE1_AND
  LE1_AND --> LE1A[Lease 获取失败]
  LE1_AND --> LE1B[etcd 延迟高]

  LE2_OR{{OR}}
  LE2 --> LE2_OR
  LE2_OR --> LE2A[API Server 不可达]
  LE2_OR --> LE2B[认证失败]
  LE2_OR --> LE2C[网络分区]

  LE3_OR{{OR}}
  LE3 --> LE3_OR
  LE3_OR --> LE3A[选主频繁切换]
  LE3_OR --> LE3B[Lease 续期失败]

  %% 控制循环异常分支 - 扩展到3-4层
  LOOP_OR{{OR}}
  LOOP --> LOOP_OR
  LOOP_OR --> L1[控制器配置问题]
  LOOP_OR --> L2[队列处理问题]
  LOOP_OR --> L3[对象同步问题]

  L1_OR{{OR}}
  L1 --> L1_OR
  L1_OR --> L1A[控制器被禁用]
  L1_OR --> L1B[参数配置错误]
  L1_OR --> L1C[RBAC 权限不足]

  L2_OR{{OR}}
  L2 --> L2_OR
  L2_OR --> L2A[队列积压严重]
  L2_OR --> L2B[处理速率低]
  L2_OR --> L2C[重试风暴]

  L3_OR{{OR}}
  L3 --> L3_OR
  L3_OR --> L3A[对象更新冲突]
  L3_OR --> L3B[状态不收敛]
  L3_OR --> L3C[级联删除问题]

  %% 依赖与存储异常分支 - 扩展到3-4层
  DEP_OR{{OR}}
  DEP --> DEP_OR
  DEP_OR --> DEP1[etcd/存储异常]
  DEP_OR --> DEP2[API Server 异常]
  DEP_OR --> DEP3[证书/鉴权异常]

  DEP1_OR{{OR}}
  DEP1 --> DEP1_OR
  DEP1_OR --> DEP1A[etcd 不可用]
  DEP1_OR --> DEP1B[etcd 延迟高]
  DEP1_OR --> DEP1C[etcd 空间不足]

  DEP2_OR{{OR}}
  DEP2 --> DEP2_OR
  DEP2_OR --> DEP2A[API Server 不可用]
  DEP2_OR --> DEP2B[API Server 限流]
  DEP2_OR --> DEP2C[API Server 延迟高]

  DEP3_OR{{OR}}
  DEP3 --> DEP3_OR
  DEP3_OR --> DEP3A[证书过期]
  DEP3_OR --> DEP3B[kubeconfig 无效]

  %% 资源管理异常分支 - 扩展到3-4层
  RES_OR{{OR}}
  RES --> RES_OR
  RES_OR --> RES1[Deployment 控制器问题]
  RES_OR --> RES2[ReplicaSet 控制器问题]
  RES_OR --> RES3[Node 控制器问题]
  RES_OR --> RES4[其他控制器问题]

  RES1_OR{{OR}}
  RES1 --> RES1_OR
  RES1_OR --> RES1A[滚动更新卡住]
  RES1_OR --> RES1B[副本数不收敛]

  RES2_OR{{OR}}
  RES2 --> RES2_OR
  RES2_OR --> RES2A[Pod 创建失败]
  RES2_OR --> RES2B[Pod 删除卡住]

  RES3_OR{{OR}}
  RES3 --> RES3_OR
  RES3_OR --> RES3A[节点状态不更新]
  RES3_OR --> RES3B[驱逐延迟]

  RES4_OR{{OR}}
  RE

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## See Also

- [[skills/cluster-upgrade-fta.md|cluster-upgrade-fta]]
- [[skills/configure-health-probes.md|configure-health-probes]]
- [[skills/crd-operator-fta.md|crd-operator-fta]]
- [[skills/csi-fta.md|csi-fta]]


<!-- risk-assessed -->
