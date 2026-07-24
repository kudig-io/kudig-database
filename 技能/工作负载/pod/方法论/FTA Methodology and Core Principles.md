---
title: FTA Methodology and Core Principles
description: FTA Methodology and Core Principles — Kubernetes 生产运维知识库
summary: FTA Methodology and Core Principles — Kubernetes 生产运维知识库
category: skill
tags:
- k8s
- fta
- troubleshooting
- root-cause
- etcd
- scheduler
- prometheus
- opa
- pdb
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- FTA Methodology and Core Principles 是什么
- 如何 FTA Methodology and Core Principles
trigger_keywords:
- FTA
- Methodology
- and
- Core
- Principles
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- etcd-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# FTA Methodology and Core Principles

## Overview

Fault Tree Analysis (FTA) is a **deductive (top-down)** methodology for systematically identifying root causes of system failures. In Kubernetes operations, FTA is combined with **FMEA (inductive, bottom-up)** to cover both component-level failure modes and system-level fault propagation paths.

## Core Principles

### 1. MECE Completeness

**Mutually Exclusive, Collectively Exhaustive** - the cornerstone of FTA quality.

- **Mutually Exclusive**: Sub-events under the same logic gate must not overlap. DNS resolution failure should not appear alongside "network failure" as a sibling - it belongs under a separate OSI layer.
- **Collectively Exhaustive**: Sub-events must cover all possibilities. "Pod cannot schedule" must include: resource insufficiency, node selector mismatch, taints, resource quotas, PDB constraints, and scheduler failure.

### 2. Observability Principle

Every bottom event must be **observable**:

| Dimension | Requirement | Example |
|-----------|------------|---------|
| Detectable | Monitoring can perceive the event | etcd latency > 100ms via Prometheus |
| Measurable | Clear quantitative thresholds | "Memory usage > 95%" not "low memory" |
| Alertable | Threshold breaches trigger alerts | Prometheus AlertRule -> PagerDuty |

**Observability Matrix**:

| Event Category | Metrics | Logs | Traces | Events |
|---------------|---------|------|--------|--------|
| Resource Exhaustion | Primary | Auxiliary | N/A | OOMKilled Event |
| Process Crash | up metric | Primary | N/A | Pod Event |
| Network Issues | Packet loss | Auxiliary | Primary | Warning |
| Config Error | N/A | Primary | N/A | Warning Event |
| Certificate Expiry | Expiry time | Error log | N/A | Event |
| Storage Failure | IO latency | Primary | N/A | PVC Event |

### 3. Hierarchical Design

Each layer must have consistent abstraction granularity:

1. **Layer 1 - Business Impact**: "User cannot checkout" -> SLO violation
2. **Layer 2 - Service Failure**: "Order service Pod unavailable" -> K8s workload layer
3. **Layer 3 - Component Failure**: "DB connection pool exhausted" -> middleware/infra
4. **Layer 4 - Resource/Config**: "Memory usage > 95%" -> observable bottom-level metrics

### 4. Independence Principle

Sub-events under the same logic gate must not have causal dependency. If CPU > 95% and response latency > 1s appear together, latency is a *symptom* of CPU - not an independent event.

## FTA Construction: 5-Phase Process

```
Phase 1          Phase 2          Phase 3          Phase 4          Phase 5
System Def.      Failure Mode     Tree Build       Qual/Quant       Verify & Optimize
(20%)            Identification   (30%)            Analysis (15%)   (5%)
                  (30%)

System boundary  FMEA analysis    Top events       Minimal cut sets Expert review
Scope definition Historical data  Intermediate     Probability calc Fault backtracking
Depth determination Arch analysis  Bottom events    Importance rating Chaos validation
                  Architecture map Logic gates      RPN ranking      Iterative optimization
```

## Kubernetes Top Events (TE-1 through TE-8)

| ID | Top Event | Severity | SLO Mapping |
|----|-----------|----------|-------------|
| TE-1 | Cluster completely unavailable | P0 | Cluster availability < 100% |
| TE-2 | Application service unavailable | P0 | Service availability SLO breach |
| TE-3 | Pod startup failure | P1 | Deployment success rate SLO breach |
| TE-4 | Network communication anomaly | P1 | Network latency/packet loss SLO breach |
| TE-5 | Storage access failure | P1 | Storage IOPS/latency SLO breach |
| TE-6 | Resource scheduling anomaly | P2 | Scheduling latency SLO breach |
| TE-7 | Security authentication failure | P1 | Security compliance SLO breach |
| TE-8 | Monitoring/alerting anomaly | P2 | Observability SLO breach |

## FMEA + FTA Synergy

Recommended approach: use FMEA to identify component failure modes, then feed them as bottom events into FTA for fault propagation analysis.

Example for etcd:
- **FMEA**: Disk full (RPN=135), quorum loss (RPN=120), data corruption (RPN=120), high latency (RPN=84), cert expiry (RPN=72)
- **FTA**: etcd disk full -> etcd timeout -> API Server unavailable -> Cluster unavailable

## Industry Fault Distribution

| Category | Share | MTTR | Typical Root Cause |
|----------|-------|------|-------------------|
| App config error | 35% | 45m | YAML errors, resource limit misconfig |
| Resource exhaustion | 22% | 30m | Memory leak, CPU spike, disk full |
| Network issues | 18% | 60m | DNS failure, CNI anomaly, policy error |
| Control plane | 10% | 90m | etcd failure, API Server overload |
| Storage | 8% | 75m | PVC binding, CSI driver failure |
| Security/auth | 5% | 40m | Cert expiry, RBAC misconfig |

## FTA 方法论实践指南

### FTA 在 Kubernetes 中的应用流程

```
1. 定义顶事件 (Top Event)
   └── 例: "Pod 无法访问 Service"

2. 构建故障树
   ├── OR 门: 任一子事件导致父事件
   ├── AND 门: 所有子事件同时发生
   └── 基本事件: 不可再分解的根因

3. 定性分析
   ├── 最小割集 (Minimal Cut Sets)
   └── 识别单点故障

4. 定量分析
   ├── 基本事件概率
   └── 顶事件发生概率

5. 制定对策
   ├── 预防措施
   ├── 检测措施
   └── 恢复措施
```

### FTA 与 FEBM 的关系

| 维度 | FTA | FEBM |
|------|-----|------|
| 方向 | 自上而下(演绎) | 自下而上(归纳) |
| 输入 | 顶事件(故障现象) | 证据(命令输出) |
| 输出 | 根因集合 | 诊断结论 |
| 适用 | 已知故障模式 | 未知/复杂故障 |
| 互补 | FTA 提供假设 | FEBM 验证假设 |

### FTA 文件编写规范

1. 每个 FTA 文件对应一个组件/场景
2. 包含完整的诊断命令速查表
3. 提供 Mermaid 可视化故障树
4. 包含生产案例和面试要点
5. 标注命令风险等级

## Related

- [[deployment]] — Deployment
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[技能/工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[技能/工作负载/pod/方法论/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]]
- [[技能/工作负载/pod/方法论/agent/Agent Orchestration Patterns.md|Agent Orchestration Patterns]]
- [[技能/工作负载/pod/方法论/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[技能/工作负载/pod/方法论/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]]
- [[概念/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]
- [[技能/网络/dns/dns-fta.md|DNS 异常故障树分析]] — Cross-reference
- [[技能/节点/node-fta.md|Node 异常故障树分析]] — Cross-reference
- [[技能/网络/service-mesh/service-mesh-istio-fta.md|Service Mesh(Istio) 异常故障树分析]] — Cross-reference
- [[技能/工作负载/deployment/deployment-fta.md|Deployment 异常故障树分析]] — Cross-reference
- [[技能/工作负载/statefulset/statefulset-fta.md|StatefulSet 异常故障树分析]] — Cross-reference
- [[技能/网络/networkpolicy/networkpolicy-fta.md|NetworkPolicy 异常故障树分析]] — Cross-reference
- [[技能/工作负载/hpa-vpa/vpa-fta.md|VPA 异常故障树分析]] — Cross-reference
- [[技能/可观测性/monitoring/monitoring-fta.md|监控与告警异常故障树分析]] — Cross-reference
- [[技能/控制面/controller-manager/controller-manager-fta.md|Controller Manager 异常故障树分析]] — Cross-reference
- [[技能/集群运维/cluster-autoscaler/cluster-autoscaler-fta.md|Cluster Autoscaler 异常故障树分析]] — Cross-reference
- [[技能/网络/cni/terway-fta.md|Terway 异常故障树分析]] — Cross-reference
- [[技能/网络/gateway-api/gateway-api-fta.md|Gateway API 异常故障树分析]] — Cross-reference
- [[技能/工作负载/daemonset/daemonset-fta.md|DaemonSet 异常故障树分析]] — Cross-reference


<!-- risk-assessed -->
