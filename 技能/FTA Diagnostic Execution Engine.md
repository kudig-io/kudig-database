---
title: FTA Diagnostic Execution Engine
description: '- [[技能/ts-resources-scheduling.md|ts-resources-scheduling]] — 资源调度故障排查'
summary: '- [[技能/ts-resources-scheduling.md|ts-resources-scheduling]] — 资源调度故障排查'
category: skill
tags:
- k8s
- fta
- troubleshooting
- automation
- agent
- statefulset
- ingress
- rbac
- controller-manager
- istio
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- FTA Diagnostic Execution Engine 是什么
- 如何 FTA Diagnostic Execution Engine
trigger_keywords:
- FTA
- Diagnostic
- Execution
- Engine
prerequisites:
- kubectl-basics
- service-mesh-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# FTA Diagnostic Execution Engine

## Architecture

The FTA execution engine transforms theoretical fault trees into executable diagnostic pipelines.

### Core Components

| Component | Responsibility |
|-----------|---------------|
| **FTATraversalEngine** | FTA tree traversal, path selection, intelligent pruning |
| **EvidenceCollector** | Multi-source evidence gathering, temporal validation, cross-validation |
| **ConfidenceEvaluator** | Multi-dimensional confidence calculation, Bayesian posterior probability |
| **HealingExecutor** | Remediation execution, precondition checks, rollback mechanism |
| **FTALearningEngine** | Learning feedback, probability updates, new pattern discovery |

### Pipeline Flow

```
Input Processing → FTA Traversal → Evidence Collection → Confidence Evaluation
     ↓                  ↓                  ↓                    ↓
Symptom parsing    Path selection     Multi-source verify   Bayesian posterior
Context injection  Pruning strategy   Temporal constraints  Confidence score
                                                        ↓
Repair Controller ← Decision Generator ← Root Cause Aggregator
  - Pre-check         - Probability sort    - Temporal validation
  - Risk assessment   - Threshold judgment  - Evidence chain
  - Rollback          - Auto/manual         - Multi-source confirm
```

## Dynamic Probability Calculation

Static fault probabilities are adjusted in real-time based on contextual factors:

```
dynamic_prob = base_prob × time_factor × load_factor × trend_factor × season_factor
```

**Example - OOMKilled during high memory load**:
- `base_prob = 0.05` (static probability)
- `time_factor = 1.5` (off-hours, fewer staff)
- `load_factor = 2.0` (memory usage > 85%)
- `trend_factor = 1.5` (3+ occurrences in 30 days)
- `current_prob = 0.05 × 1.5 × 2.0 × 1.5 = 0.225` (4.5x increase)

## Input/Output Schema

### Diagnosis Request

```yaml
diagnosis_request:
  primary_symptom: string          # e.g., "Pod CrashLoopBackOff"
  timestamp: datetime
  cluster_id: string
  secondary_symptoms: [string]     # e.g., ["OOMKilled", "Exit 137"]
  error_logs: [string]
  exit_code: integer
  events: [object]                 # K8s Events
  metrics: object                  # Real-time metrics
  context:
    namespace: string
    workload_type: string
    cloud_provider: string         # ACK/AWS/GCP
    environment: string            # prod/staging
```

### Diagnosis Result

```yaml
diagnosis_result:
  confirmed_root_cause:
    event_id: string               # e.g., BE-2.3
    name: string                   # e.g., OOMKilled
    probability: float
    confidence: float
    evidence_chain: [object]
  candidate_paths: [object]        # When not confirmed
  healing_plan:
    - action_id: string
      description: string
      risk_level: low|medium|high|critical
      preconditions: [object]
      auto_executable: boolean
      estimated_duration: string
      rollback_plan: string
```

## Intelligent Pruning

The traversal engine prunes unlikely paths to reduce diagnosis time:

- **Confidence threshold pruning**: Skip paths with posterior confidence < 0.1
- **Evidence contradiction pruning**: Eliminate paths contradicted by collected evidence
- **Time-based pruning**: Deprioritize paths requiring long-running checks during P0 incidents

## Learning Feedback Loop

- **Success feedback**: Increase probability of confirmed paths
- **Failure feedback**: Decrease probability, propose alternative paths
- **New pattern discovery**: Track patterns not in existing FTA, flag as PROPOSED for review

## Related

- [[技能/ts-resources-scheduling.md|ts-resources-scheduling]] — 资源调度故障排查
- [[rbac-fta]] — RBAC 异常故障树分析
- [[技能/skill-21-statefulset-failure.md|skill-21-statefulset-failure]] — StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation
- [[技能/troubleshoot-node-issues.md|troubleshoot-node-issues]] — Troubleshoot Node Issues
- [[score]] — Score
- [[技能/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- [[技能/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]]
- [[技能/Agent Orchestration Patterns.md|Agent Orchestration Patterns]]
- [[技能/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[概念/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]
- [[概念/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]]

- [[README]]
- [[nginx-ingress-fta]]
- [[文档/ERROR-FTA-MAP.md|KUDIG 错误码 → FTA 映射]] — Cross-reference
- [[文档/COMMAND-DOC-MAP.md|KUDIG 命令 → 文档映射]] — Cross-reference
- [[文档/API-DOC-MAP.md|KUDIG API → 文档映射]] — Cross-reference
- [[技能/dns-fta.md|DNS 异常故障树分析]] — Cross-reference
- [[技能/node-fta.md|Node 异常故障树分析]] — Cross-reference
- [[技能/service-mesh-istio-fta.md|Service Mesh(Istio) 异常故障树分析]] — Cross-reference
- [[技能/deployment-fta.md|Deployment 异常故障树分析]] — Cross-reference
- [[技能/statefulset-fta.md|StatefulSet 异常故障树分析]] — Cross-reference
- [[技能/networkpolicy-fta.md|NetworkPolicy 异常故障树分析]] — Cross-reference
- [[技能/vpa-fta.md|VPA 异常故障树分析]] — Cross-reference
- [[技能/monitoring-fta.md|监控与告警异常故障树分析]] — Cross-reference
- [[技能/controller-manager-fta.md|Controller Manager 异常故障树分析]] — Cross-reference
- [[技能/cluster-autoscaler-fta.md|Cluster Autoscaler 异常故障树分析]] — Cross-reference
- [[技能/terway-fta.md|Terway 异常故障树分析]] — Cross-reference
- [[技能/gateway-api-fta.md|Gateway API 异常故障树分析]] — Cross-reference
- [[技能/daemonset-fta.md|DaemonSet 异常故障树分析]] — Cross-reference


<!-- risk-assessed -->
