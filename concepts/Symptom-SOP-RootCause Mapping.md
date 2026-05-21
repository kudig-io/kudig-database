---
title: Symptom-SOP-RootCause Mapping
description: Symptom-SOP-RootCause Mapping — Kubernetes 生产运维知识库
category: concept
tags:
- k8s
- troubleshooting
- mapping
- decision-tree
- sop
- kubelet
- statefulset
- networkpolicy
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Symptom-SOP-RootCause Mapping 是什么
- 如何 Symptom-SOP-RootCause Mapping
trigger_keywords:
- Symptom-SOP-RootCause
- Mapping
prerequisites:
- kubectl-basics
---

# Symptom-SOP-RootCause Mapping

## Design Principles

The symptom mapping layer bridges between natural language symptom descriptions and structured diagnostic workflows.

```
Symptom Input -> Vectorized Matching -> Candidate Path Ranking -> Execution Verification -> Root Cause Confirmation
```

Design goals:
1. **Machine-readable**: AI Agents can directly parse and execute
2. **Human-readable**: Operators can quickly understand
3. **Extensible**: Easy to add new symptom patterns
4. **Verifiable**: Each mapping has clear validation conditions

## Input Schema

```yaml
symptom_input:
  primary_symptom: string       # e.g., "Pod CrashLoopBackOff"
  secondary_symptoms: [string]  # e.g., ["OOMKilled", "Exit Code 137"]
  context:
    namespace: string
    workload_type: string       # Deployment/StatefulSet/etc.
    cluster_type: string        # ACK/self-managed/hybrid
    cloud_provider: string      # Alibaba/AWS/GCP/Azure
  observable:
    error_logs: [string]
    exit_code: integer
    events: [string]
    metrics: object
  urgency: P0|P1|P2
```

## Output Schema

```yaml
diagnosis_output:
  ranked_paths:
    - path_id: string
      probability: float        # 0.0-1.0 match probability
      root_cause: string        # Inferred root cause
      fta_path: string          # e.g., "TE-2 -> IE-2.1 -> BE-2.3"
      confidence: float         # 0.0-1.0

      diagnostic_steps:
        - step: integer
          command: string
          expected_result: string
          validation: string

      related_docs:
        - path: string
          type: structural|domain|skill|febm
          relevance: float

      auto_heal_actions:
        - action_id: string
          description: string
          risk_level: string
          command: string
```

## Quick Decision Trees

The mapping layer provides 3-step decision trees for rapid diagnosis:

### Example: Pod Not Running

```
Step 1: kubectl get pod <name>
  -> Pending?     -> Check scheduling (TE-3)
  -> Running?     -> Check health probes (TE-2)
  -> CrashLoop?   -> Check logs (TE-2 -> IE-2.1)
  -> Error?       -> Check events (TE-3)
  -> Evicted?     -> Check node resources (TE-2 -> IE-2.1 -> BE-2.4)

Step 2: kubectl describe pod <name>
  -> Read Events section for specific failure reason

Step 3: kubectl logs <name> --previous (if crashed)
  -> Application-level error identification
```

### Example: Service Not Reachable

```
Step 1: kubectl get service <name>
  -> Service exists with correct type?

Step 2: kubectl get endpoints <name>
  -> Endpoints populated? If empty -> selector mismatch or no ready pods

Step 3: kubectl exec -it <pod> -- curl <service-ip>
  -> Pod-to-service connectivity test
  -> If fails: NetworkPolicy, kube-proxy, CNI issues
```

## Common Symptom Patterns

| Symptom Pattern | Likely FTA Path | Confidence | SOP Reference |
|----------------|-----------------|------------|---------------|
| Pod CrashLoop + Exit 1 | TE-2 -> IE-2.1 -> BE-2.1 | 0.85 | Pod CrashLoopBackOff skill |
| Pod OOMKilled + Exit 137 | TE-2 -> IE-2.1 -> BE-2.3 | 0.92 | Pod OOMKilled skill |
| Pod Pending + FailedScheduling | TE-3 -> IE-3.1 | 0.90 | Pod Pending skill |
| Node NotReady + kubelet down | TE-1 -> IE-1.2 -> BE-1.5 | 0.88 | Node NotReady skill |
| DNS lookup failed | TE-4 -> IE-4.1 | 0.80 | DNS Resolution Failure skill |
| PVC Pending | TE-5 -> IE-5.1 | 0.85 | PVC Storage Failure skill |
| Connection refused + cert error | TE-7 -> IE-7.1 | 0.90 | Certificate Expiry skill |

## Related

- [[entities/statefulset.md|statefulset]] — StatefulSet
- [[deployment]] — Deployment
- [[entities/kubelet.md|kubelet]] — kubelet
- [[cni]] — CNI (Container Network Interface)
- [[skills/skill-k8s-node-notready-SKILL.md|skill-k8s-node-notready-SKILL]] — Skill
- [[skills/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[synthesis/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]]
- [[synthesis/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]]

- [[templates/decision-tree-template.md|decision-tree-template]]
- [[docs/ERROR-FTA-MAP|KUDIG 错误码 → FTA 映射]] — Cross-reference
- [[docs/COMMAND-DOC-MAP|KUDIG 命令 → 文档映射]] — Cross-reference
- [[docs/API-DOC-MAP|KUDIG API → 文档映射]] — Cross-reference
