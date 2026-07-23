---
title: DaemonSet 异常故障树分析 (skills)
description: NODE_OR --> NODE2[污点/容忍配置问题]
summary: NODE_OR --> NODE2[污点/容忍配置问题]
category: general
tags:
- k8s
- controller-manager
- daemonset
- rbac
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- DaemonSet 异常故障树分析 是什么
- 如何 DaemonSet 异常故障树分析
trigger_keywords:
- DaemonSet
- 异常故障树分析
prerequisites:
- kubectl-basics
fta_id: FTA-DAEMONSET-001
component: Daemonset
severity: critical
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "[[DaemonSet|DaemonSet]] 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get daemonset -A -o jsonpath='{range .items[?(@.status.desiredNumberScheduled != @.status.numberAvailable)]} {.metadata.namespace}/{.metadata.name}{\'\n\..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["故障诊断/topic-fta/list/daemonset-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# DaemonSet 异常故障树分析

<!-- condition: kubectl get daemonset -A -o jsonpath='{range .items[?(@.status.desiredNumberScheduled != @.status.numberAvailable)]} {.metadata.namespace}/{.metadata.name}{\"\n\"}{end}' 显示节点覆盖不全 -->

# DaemonSet 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 DaemonSet Pod 未覆盖、更新失败与节点绑定异常的关键成因与路径。
- **范围**：节点选择与污点、镜像与探针、滚动更新、资源与配额、控制器依赖。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: DaemonSet 异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> NODE[节点匹配异常]
  OR0 --> POD[Pod 启动异常]
  OR0 --> ROLL[滚动更新异常]
  OR0 --> RES[资源与配额异常]
  OR0 --> CTRL[控制器异常]

  %% 节点匹配异常分支 - 扩展到3-4层
  NODE_OR{{OR}}
  NODE --> NODE_OR
  NODE_OR --> NODE1[节点选择器不匹配]
  NODE_OR --> NODE2[污点/容忍配置问题]
  NODE_OR --> NODE3[节点状态异常]

  NODE1_OR{{OR}}
  NODE1 --> NODE1_OR
  NODE1_OR --> NODE1A[nodeSelector 标签不存在]
  NODE1_OR --> NODE1B[nodeAffinity 规则不满足]
  NODE1_OR --> NODE1C[节点标签被误修改]

  NODE2_OR{{OR}}
  NODE2 --> NODE2_OR
  NODE2_OR --> NODE2A[缺少关键污点容忍]
  NODE2_OR --> NODE2B[tolerationSeconds 过期]
  NODE2_OR --> NODE2C[NoExecute 污点驱逐]

  NODE3_OR{{OR}}
  NODE3 --> NODE3_OR
  NODE3_OR --> NODE3A[节点 NotReady]
  NODE3_OR --> NODE3B[节点 SchedulingDisabled]
  NODE3_OR --> NODE3C[节点网络不可达]

  %% Pod 启动异常分支 - 扩展到3-4层
  POD_OR{{OR}}
  POD --> POD_OR
  POD_OR --> POD1[镜像拉取失败]
  POD_OR --> POD2[探针失败]
  POD_OR --> POD3[容器启动失败]

  POD1_OR{{OR}}
  POD1 --> POD1_OR
  POD1_OR --> POD1A[ImagePullBackOff]
  POD1_OR --> POD1B[私有仓库认证失败]
  POD1_OR --> POD1C[镜像不存在/tag 错误]

  POD2_OR{{OR}}
  POD2 --> POD2_OR
  POD2_OR --> POD2A[livenessProbe 失败]
  POD2_OR --> POD2B[readinessProbe 失败]
  POD2_OR --> POD2C[startupProbe 超时]

  POD3_OR{{OR}}
  POD3 --> POD3_OR
  POD3_OR --> POD3A[CrashLoopBackOff]
  POD3_OR --> POD3B[OOMKilled]
  POD3_OR --> POD3C[配置/挂载错误]

  %% 滚动更新异常分支 - 扩展到3-4层 + AND 门
  ROLL_OR{{OR}}
  ROLL --> ROLL_OR
  ROLL_OR --> ROLL1[更新卡住]
  ROLL_OR --> ROLL2[回滚失败]
  ROLL_OR --> ROLL3[版本不一致]

  ROLL1_AND{{AND}}
  ROLL1 --> ROLL1_AND
  ROLL1_AND --> ROLL1A[新 Pod 启动失败]
  ROLL1_AND --> ROLL1B[maxUnavailable=0]

  ROLL2_OR{{OR}}
  ROLL2 --> ROLL2_OR
  ROLL2_OR --> ROLL2A[无可用历史版本]
  ROLL2_OR --> ROLL2B[回滚镜像也失败]

  ROLL3_OR{{OR}}
  ROLL3 --> ROLL3_OR
  ROLL3_OR --> ROLL3A[部分节点未更新]
  ROLL3_OR --> ROLL3B[updateStrategy 配置错误]

  %% 资源与配额异常分支 - 扩展到3-4层 + AND 门
  RES_OR{{OR}}
  RES --> RES_OR
  RES_OR --> RES1[节点资源不足]
  RES_OR --> RES2[配额限制]
  RES_OR --> RES3[优先级驱逐]

  RES1_AND{{AND}}
  RES1 --> RES1_AND
  RES1_AND --> RES1A[CPU/内存请求高]
  RES1_AND --> RES1B[节点可分配资源低]

  RES2_OR{{OR}}
  RES2 --> RES2_OR
  RES2_OR --> RES2A[namespace 配额耗尽]
  RES2_OR --> RES2B[LimitRange 限制]

  RES3_OR{{OR}}
  RES3 --> RES3_OR
  RES3_OR --> RES3A[低优先级被抢占]
  RES3_OR --> RES3B[节点压力驱逐]

  %% 控制器异常分支 - 扩展到3-4层
  CTRL_OR{{OR}}
  CTRL --> CTRL_OR
  CTRL_OR --> CTRL1[DaemonSet 控制器异常]
  CTRL_OR --> CTRL2[API Server 连接问题]
  CTRL_OR --> CTRL3[RBAC 权限不足]

  CTRL1_OR{{OR}}
  CTRL1 --> CTRL1_OR
  CTRL1_OR --> CTRL1A[controller-manager 异常]
  CTRL1_OR --> CTRL1B[控制器队列积压]
  CTRL1_OR --> CTRL1C[选主失败]



## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[ts-workloads|工作负载故障排查]]

## See Also

- [[技能/crd-operator-fta.md|crd-operator-fta]]
- [[技能/csi-fta.md|csi-fta]]
- [[技能/deployment-canary-and-bluegreen.md|deployment-canary-and-bluegreen]]
- [[技能/deployment-fta.md|deployment-fta]]


<!-- risk-assessed -->
