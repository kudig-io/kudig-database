---
title: FTA Methodology and Core Principles
description: FTA Methodology and Core Principles — Kubernetes 生产运维知识库
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
created: "2026-05-23"
---

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

## Related

- [[deployment]] — Deployment
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[skills/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[skills/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]]
- [[skills/Agent Orchestration Patterns.md|Agent Orchestration Patterns]]
- [[skills/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[skills/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]]
- [[concepts/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]
- [[skills/dns-fta|DNS 异常故障树分析]] — Cross-reference
- [[skills/node-fta|Node 异常故障树分析]] — Cross-reference
- [[skills/service-mesh-istio-fta|Service Mesh(Istio) 异常故障树分析]] — Cross-reference
- [[skills/deployment-fta|Deployment 异常故障树分析]] — Cross-reference
- [[skills/statefulset-fta|StatefulSet 异常故障树分析]] — Cross-reference
- [[skills/networkpolicy-fta|NetworkPolicy 异常故障树分析]] — Cross-reference
- [[skills/vpa-fta|VPA 异常故障树分析]] — Cross-reference
- [[skills/monitoring-fta|监控与告警异常故障树分析]] — Cross-reference
- [[skills/controller-manager-fta|Controller Manager 异常故障树分析]] — Cross-reference
- [[skills/cluster-autoscaler-fta|Cluster Autoscaler 异常故障树分析]] — Cross-reference
- [[skills/terway-fta|Terway 异常故障树分析]] — Cross-reference
- [[skills/gateway-api-fta|Gateway API 异常故障树分析]] — Cross-reference
- [[skills/daemonset-fta|DaemonSet 异常故障树分析]] — Cross-reference
