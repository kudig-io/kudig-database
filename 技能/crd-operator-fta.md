---
title: CRD/Operator 异常故障树分析 (skills)
description: '- **范围**：CRD 定义/注册、Operator 控制器生命周期、Reconcile 循环、转换/验证 Webhook、RBAC/SA
  认证、依赖组件（API Server / etcd / informer cache）。'
summary: '- **范围**：CRD 定义/注册、Operator 控制器生命周期、Reconcile 循环、转换/验证 Webhook、RBAC/SA 认证、依赖组件（API
  Server / etcd / informer cache）。'
category: general
tags:
- k8s
- etcd
- rbac
- crd
- operator
- webhook
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CRD/Operator 异常故障树分析 是什么
- 如何 CRD/Operator 异常故障树分析
trigger_keywords:
- CRD
- Operator
- 异常故障树分析
prerequisites:
- kubectl-basics
- etcd-basics
fta_id: FTA-CRD_OPERATOR-001
component: Crd Operator
severity: medium
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "CRD/Operator 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get crd -A -o jsonpath='{range .items[?(@.status.conditions[?(@.type!=\'Established\')].type)]} {.metadata.name}{\'\n\'}{end}' 显示 CRD 异常 --> - **目标**：覆盖 ..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["故障诊断/topic-fta/list/crd-operator-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# CRD/Operator 异常故障树分析

<!-- condition: kubectl get crd -A -o jsonpath='{range .items[?(@.status.conditions[?(@.type!=\"Established\")].type)]} {.metadata.name}{\"\n\"}{end}' 显示 CRD 异常 -->

# CRD/Operator 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 CRD/Operator 协调循环失效、版本不兼容、资源漂移、Webhook 转换失败与依赖组件异常的关键成因与路径。
- **范围**：CRD 定义/注册、Operator 控制器生命周期、Reconcile 循环、转换/验证 Webhook、RBAC/SA 认证、依赖组件（API Server / etcd / informer cache）。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE["顶事件: CRD/Operator 异常<br/>资源不收敛 / CR 操作失败"]
  OR0{{OR}}
  TE --> OR0

  %% ======== 一级分类 ========
  OR0 --> CAT_CRD["A. CRD 定义/注册异常"]
  OR0 --> CAT_CTRL["B. Operator/Controller 运行异常"]
  OR0 --> CAT_RECON["C. Reconcile 循环异常"]
  OR0 --> CAT_WH["D. Webhook 转换/验证异常"]
  OR0 --> CAT_RBAC["E. RBAC 与认证异常"]
  OR0 --> CAT_DEP["F. 依赖/控制面异常"]

  %% ======== A. CRD 定义/注册 ========
  A_OR{{OR}}
  CAT_CRD --> A_OR
  A_OR --> A1["A1. CRD 注册失败<br/>apply/create 报错"]
  A_OR --> A2["A2. CRD schema 校验错误<br/>OpenAPI validation 失败"]
  A_OR --> A3["A3. CRD 版本兼容性问题<br/>storedVersions 不一致"]
  A_OR --> A4_AND["A4. CRD 版本升级死锁<br/>(AND 门)"]

  A4_AND_GATE{{"AND"}}
  A4_AND --> A4_AND_GATE
  A4_AND_GATE --> A4C1["对象存储版本为已废弃版本"]
  A4_AND_GATE --> A4C2["转换 Webhook 不可用"]

  %% ======== B. Operator/Controller ========
  B_OR{{OR}}
  CAT_CTRL --> B_OR
  B_OR --> B1["B1. Controller Pod 崩溃/重启<br/>OOM / panic / 配置错误"]
  B_OR --> B2["B2. Leader Election 失败<br/>锁竞争超时"]
  B_OR --> B3["B3. Controller 多副本脑裂<br/>分布式锁异常"]
  B_OR --> B4["B4. Informer Cache 不同步<br/>watch 断连 / 资源过多"]
  B_OR --> B5_AND["B5. Operator 级联删除阻塞<br/>(AND 门)"]

  B5_AND_GATE{{"AND"}}
  B5_AND --> B5_AND_GATE
  B5_AND_GATE --> B5C1["CR 上存在未清理 Finalizer"]
  B5_AND_GATE --> B5C2["负责清理的 Controller 不运行"]

  %% ======== C. Reconcile 循环 ========
  C_OR{{OR}}
  CAT_RECON --> C_OR
  C_OR --> C1["C1. Reconcile 持续报错<br/>子资源创建/更新失败"]
  C_OR --> C2["C2. 队列积压<br/>worker 不足 / 处理时间过长"]
  C_OR --> C3["C3. 资源状态漂移<br/>外部修改覆盖 Operator 期望"]
  C_OR --> C4["C4. Reconcile 无限循环<br/>status 更新触发再次入队"]
  C_OR --> C5_AND["C5. 扩缩容阻塞<br/>(AND 门)"]

  C5_AND_GATE{{"AND"}}
  C5_AND --> C5_AND_GATE
  C5_AND_GATE --> C5C1["子资源 Quota 耗尽"]
  C5_AND_GATE --> C5C2["Reconcile 无退避重试上限"]

  %% ======== D. Webhook ========
  D_OR{{OR}}
  CAT_WH --> D_OR
  D_OR --> D1["D1. 转换 Webhook 失败<br/>版本转换出错"]
  D_OR --> D2["D2. 验证 Webhook 误拒<br/>规则过严"]
  D_OR --> D3["D3. Webhook 服务不可达<br/>Endpoint / Service 异常"]
  D_OR --> D4["D4. Webhook 超时<br/>处理时间 > failurePolicy 超时"]
  D_OR --> D5_AND["D5. Webhook 级联超时<br/>(AND 门)"]

  D5_AND_GATE{{"AND"}}
  D5_AND --> D5_AND_GATE
  D5_AND_GATE --> D5C1["多个 Webhook 串联处理"]
  D5_AND_GATE --> D5C2["单个 Webhook 接近超时阈值"]

  %% ======== E. RBAC ========
  E_OR{{OR}}
  CAT_RBAC --> E_OR
  E_OR --> E1["E1. ServiceAccount 不存在/挂载失败"]
  E_OR --> E2["E2. ClusterRole/Role 权限不足<br/>缺少 verbs 或 resource"]
  E_OR --> E3["E3. Token 过期/轮换失败<br/>BoundServiceAccountToken"]
  E_OR --> E4["E4. Namespace 作用域越界<br/>跨 NS 操作被拒"]

  %% ======== F. 依赖/控制面 =======

## 相关链接

- [[技能/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[etcd]] — etcd


<!-- risk-assessed -->
