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



---
title: "Cluster Autoscaler 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get [[Pods|pods]] -A --field-selector=status.phase=Pending -o jsonpath='{range .items[?(@.spec.nodeName==null)]} {.metadata.namespace}/{.metadata.name}{\'\n\'}{en..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["domain-10-troubleshooting-diagnostics/topic-fta/list/cluster-autoscaler-fta.md"]
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

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## See Also

- [[skills/cilium-fta.md|cilium-fta]]
- [[skills/cloud-provider-fta.md|cloud-provider-fta]]
- [[skills/cluster-upgrade-fta.md|cluster-upgrade-fta]]
- [[skills/configure-health-probes.md|configure-health-probes]]
