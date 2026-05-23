---
title: FTA Diagnostic Execution Engine
description: '- [[skills/ts-resources-scheduling|ts-resources-scheduling]] — 资源调度故障排查'
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
created: "2026-05-23"
---

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

- [[skills/ts-resources-scheduling|ts-resources-scheduling]] — 资源调度故障排查
- [[rbac-fta]] — RBAC 异常故障树分析
- [[skills/skill-21-statefulset-failure|skill-21-statefulset-failure]] — StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation
- [[skills/troubleshoot-node-issues|troubleshoot-node-issues]] — Troubleshoot Node Issues
- [[score]] — Score
- [[skills/FTA Methodology and Core Principles|FTA Methodology and Core Principles]]
- [[skills/Symptom Vector Matching Engine|Symptom Vector Matching Engine]]
- [[skills/Agent Orchestration Patterns|Agent Orchestration Patterns]]
- [[skills/Kubernetes FTA Top Events Index|Kubernetes FTA Top Events Index]]
- [[concepts/Symptom-SOP-RootCause Mapping|Symptom-SOP-RootCause Mapping]]
- [[synthesis/Production Troubleshooting Playbook|Production Troubleshooting Playbook]]

- [[README]]
- [[nginx-ingress-fta]]
- [[docs/ERROR-FTA-MAP|KUDIG 错误码 → FTA 映射]] — Cross-reference
- [[docs/COMMAND-DOC-MAP|KUDIG 命令 → 文档映射]] — Cross-reference
- [[docs/API-DOC-MAP|KUDIG API → 文档映射]] — Cross-reference
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
