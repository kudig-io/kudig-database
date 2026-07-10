---
title: Scheduler 异常故障树分析 (skills)
description: '<!-- condition: kubectl get pods -A --field-selector=status.phase=Pending
  显示大量 Pending Pod 或 kubectl get events -A --field-selector reason=FailedScheduling
  显示调度失败 -->'
summary: '<!-- condition: kubectl get pods -A --field-selector=status.phase=Pending
  显示大量 Pending Pod 或 kubectl get events -A --field-selector reason=FailedScheduling
  显示调度失败 -->'
category: skills
tags:
- k8s
- fta
- troubleshooting
- apiserver
- scheduler
- ingress
- networkpolicy
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Scheduler 异常故障树分析 是什么
- 如何 Scheduler 异常故障树分析
trigger_keywords:
- Scheduler
- 异常故障树分析
prerequisites:
- kubectl-basics
fta_id: FTA-SCHEDULER-001
component: Scheduler
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Scheduler 异常故障树分析

<!-- condition: kubectl get [[Pods|pods]] -A --field-selector=status.phase=Pending 显示大量 Pending Pod 或 kubectl get events -A --field-selector reason=FailedScheduling 显示调度失败 -->

# Scheduler 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖调度失败、调度延迟与调度决策异常的关键成因与路径。
- **范围**：调度器服务、过滤/打分插件、资源/配额、拓扑与亲和、扩缩容协同。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: Scheduler 调度异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> SVC[调度器服务异常]
  OR0 --> FILTER[过滤/打分异常]
  OR0 --> RES[资源与配额异常]
  OR0 --> TOPO[拓扑与亲和异常]
  OR0 --> SCALE[扩缩容协同异常]

  %% 调度器服务异常分支 - 扩展到3-4层
  SVC_OR{{OR}}
  SVC --> SVC_OR
  SVC_OR --> SVC1[调度器进程异常]
  SVC_OR --> SVC2[选主/HA 问题]
  SVC_OR --> SVC3[API Server 连接失败]

  SVC1_OR{{OR}}
  SVC1 --> SVC1_OR
  SVC1_OR --> SVC1A[进程崩溃/OOM]
  SVC1_OR --> SVC1B[配置加载失败]
  SVC1_OR --> SVC1C[资源不足无法启动]

  SVC2_OR{{OR}}
  SVC2 --> SVC2_OR
  SVC2_OR --> SVC2A[选主锁获取失败]
  SVC2_OR --> SVC2B[多调度器冲突]
  SVC2_OR --> SVC2C[Lease 续期失败]

  SVC3_OR{{OR}}
  SVC3 --> SVC3_OR
  SVC3_OR --> SVC3A[API Server 不可用]
  SVC3_OR --> SVC3B[网络分区]
  SVC3_OR --> SVC3C[证书/认证问题]

  %% 过滤/打分异常分支 - 扩展到3-4层 + AND 门
  FILTER_OR{{OR}}
  FILTER --> FILTER_OR
  FILTER_OR --> FIL1[过滤插件问题]
  FILTER_OR --> FIL2[打分插件问题]
  FILTER_OR --> FIL3[调度配置错误]

  FIL1_OR{{OR}}
  FIL1 --> FIL1_OR
  FIL1_OR --> FIL1A[所有节点被过滤]
  FIL1_OR --> FIL1B[插件超时]
  FIL1_OR --> FIL1C[自定义插件异常]

  FIL2_AND{{AND}}
  FIL2 --> FIL2_AND
  FIL2_AND --> FIL2A[多个打分插件冲突]
  FIL2_AND --> FIL2B[权重配置不当]

  FIL3_OR{{OR}}
  FIL3 --> FIL3_OR
  FIL3_OR --> FIL3A[KubeSchedulerConfiguration 错误]
  FIL3_OR --> FIL3B[Profile 配置冲突]

  %% 资源与配额异常分支 - 扩展到3-4层
  RES_OR{{OR}}
  RES --> RES_OR
  RES_OR --> RES1[节点资源不足]
  RES_OR --> RES2[配额/限额限制]
  RES_OR --> RES3[资源碎片化]

  RES1_OR{{OR}}
  RES1 --> RES1_OR
  RES1_OR --> RES1A[CPU 不足]
  RES1_OR --> RES1B[内存不足]
  RES1_OR --> RES1C[扩展资源不足]

  RES2_OR{{OR}}
  RES2 --> RES2_OR
  RES2_OR --> RES2A[namespace 配额耗尽]
  RES2_OR --> RES2B[集群级别限制]
  RES2_OR --> RES2C[PriorityClass 限制]

  RES3_OR{{OR}}
  RES3 --> RES3_OR
  RES3_OR --> RES3A[小资源请求无法满足]
  RES3_OR --> RES3B[节点资源利用率不均]

  %% 拓扑与亲和异常分支 - 扩展到3-4层 + AND 门
  TOPO_OR{{OR}}
  TOPO --> TOPO_OR
  TOPO_OR --> TOP1[亲和性问题]
  TOPO_OR --> TOP2[反亲和性问题]
  TOPO_OR --> TOP3[拓扑分布约束问题]

  TOP1_OR{{OR}}
  TOP1 --> TOP1_OR
  TOP1_OR --> TOP1A[nodeAffinity 无匹配节点]
  TOP1_OR --> TOP1B[podAffinity 目标 Pod 不存在]

  TOP2_AND{{AND}}
  TOP2 --> TOP2_AND
  TOP2_AND --> TOP2A[强制反亲和 requiredDuringScheduling]
  TOP2_AND --> TOP2B[所有可用节点已有冲突 Pod]

  TOP3_OR{{OR}}
  TOP3 --> TOP3_OR
  TOP3_OR --> TOP3A[topologySpreadConstraints 无法满足]
  TOP3_OR --> TOP3B[maxSkew 配置过严格]
  TOP3_OR --> TOP3C[拓扑域标签缺失]

  %% 扩缩容协同异常分支 - 扩展到3-4层
  SCALE_OR{{OR}}
  SCALE --> SCALE_OR
  SCALE_OR --> SC1[Cluster Autoscaler 问题]
  SCALE_OR --> SC2[节点池问题]
  SCALE_OR --> SC3[抢占问题]

  SC1_OR{{OR}}
  SC1 --> SC1_OR
  SC1_OR --> SC1A[CA 进程异常]
  SC1_OR --> SC1B[扩容决策延迟]
  SC1_OR --> SC1C[缩容误判]

  SC2_OR{{OR}}
  SC2 --> SC2_OR
  SC2_OR --> SC2A[节点池已达上限]
  SC2_OR --> SC2B[节点启动失败]
  SC2_OR -

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[skills/ts-control-plane.md|控制平面故障排查]]

## Related

- [[skills/skill-20-networkpolicy-connectivity.md|skill-20-networkpolicy-connectivity]] — NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting
- [[skills/assessment-k8s-fundamentals-quiz.md|assessment-k8s-fundamentals-quiz]] — K8S Fundamentals Quiz
- [[skills/ts-cloud-provider.md|ts-cloud-provider]] — 云服务商集成排查
- [[skills/ts-node-components.md|ts-node-components]] — 节点组件故障排查
- [[apiserver-fta]] — API Server 异常故障树分析

- [[nginx-ingress-fta]]
- [[故障诊断/FTA故障树/list/scheduler-fta.md|Scheduler 异常故障树分析]]
- [[skills/assessment-k8s-fundamentals-quiz-answers.md|K8S Fundamentals Quiz Answers]] — Cross-reference


<!-- risk-assessed -->
