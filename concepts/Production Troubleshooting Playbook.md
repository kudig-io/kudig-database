---
title: Production Troubleshooting Playbook
description: Production Troubleshooting Playbook — Kubernetes 生产运维知识库
category: synthesis
tags:
- k8s
- troubleshooting
- production
- runbook
- sop
- etcd
- coredns
- ingress
- networkpolicy
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Production Troubleshooting Playbook 是什么
- 如何 Production Troubleshooting Playbook
trigger_keywords:
- Production
- Troubleshooting
- Playbook
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
relationships:
  - target: "[[domain-17-system-foundation/topic-dictionary/networking/ingress.md]]"
    type: uses
  - target: "[[entities/kubernetes.md]]"
    type: uses
  - target: "[[skills/Kubernetes Diagnostic Skills Overview.md]]"
    type: uses
  - target: "[[skills/Kubernetes FTA Top Events Index.md]]"
    type: uses
---

# Production Troubleshooting Playbook

## Symptom-to-Diagnosis Mapping

This playbook synthesizes information from the [[entities/kubernetes.md|Kubernetes]] Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]], [[skills/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]], and Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]] into an actionable troubleshooting flow.

### Tier 1: Immediate Assessment (First 2 Minutes)

```
1. Cluster accessible?
   YES -> kubectl get --raw='/readyz'  -> Continue to step 2
   NO  -> FTA path: TE-1 (Cluster Unavailable, P0)
          -> Check: API Server certs, etcd health, network connectivity
          -> Escalate to SRE lead immediately

2. Which services affected?
   All services  -> TE-1 or TE-2 (P0)
   Single service -> TE-2 or TE-4 (P1)
   Single Pod     -> TE-3 (P1)
```

### Tier 2: Pod-Level Diagnosis (5-15 Minutes)

| Symptom | FTA Path | First Command | Common Root Cause |
|---------|----------|---------------|-------------------|
| Pod Pending | TE-3 -> IE-3.1 | `kubectl describe pod` | Resource insufficient, taint, quota |
| CrashLoopBackOff | TE-2 -> IE-2.1 -> BE-2.1 | `kubectl logs --previous` | App crash, config error, dependency down |
| OOMKilled | TE-2 -> IE-2.1 -> BE-2.3 | `kubectl describe pod \| grep 'Last State'` | Memory leak, limits too low |
| ImagePullBackOff | TE-3 -> IE-3.2 | `kubectl describe pod \| grep ImagePull` | Auth error, image missing, network |
| Evicted | TE-2 -> IE-2.1 -> BE-2.4 | `kubectl describe pod \| grep Evicted` | Node disk/memory pressure |

### Tier 3: Network Diagnosis (15-30 Minutes)

| Symptom | FTA Path | First Command | Common Root Cause |
|---------|----------|---------------|-------------------|
| DNS resolution failure | TE-4 -> IE-4.1 | `kubectl get ep kube-dns -n kube-system` | CoreDNS pods down, endpoint missing |
| Pod-to-Pod connectivity | TE-4 -> IE-4.2 | `kubectl exec -it <pod> -- curl <target>` | NetworkPolicy blocking, CNI issue |
| Service unreachable | TE-2 -> IE-2.2 | `kubectl get ep <service>` | Endpoint not populated, selector wrong |
| External access failure | TE-2 -> IE-2.3 | `kubectl get [[domain-17-system-foundation/topic-dictionary/networking/ingress.md|ingress]]` | Ingress config error, cert expiry |

### Tier 4: Storage Diagnosis (15-30 Minutes)

| Symptom | FTA Path | First Command | Common Root Cause |
|---------|----------|---------------|-------------------|
| PVC Pending | TE-5 -> IE-5.1 | `kubectl describe pvc` | No matching PV, StorageClass issue |
| Mount failure | TE-5 -> IE-5.2 | `kubectl describe pod \| grep MountVolume` | CSI driver down, volume not ready |
| I/O performance | TE-5 -> IE-5.3 | `iostat -x 1` on node | Storage backend degradation |

## Escalation Matrix

| Severity | Response Time | Who | Communication |
|----------|--------------|-----|---------------|
| P0 - Service Down | < 5 min | On-call SRE + Tech Lead | Slack #incident + 30-min bridge |
| P1 - Degraded | < 15 min | On-call SRE | Slack #incident |
| P2 - Minor | < 1 hour | On-call engineer | Ticket + Slack notification |

## Rapid Recovery Actions (Low Risk)

These actions can be executed without approval during incidents:

| Action | When | Command |
|--------|------|---------|
| Restart Pod | CrashLoop with known config issue | `kubectl delete pod <pod>` |
| Rollout restart | Deployment-level issues | `kubectl rollout restart deployment/<name>` |
| Rollback | Bad deployment caused issue | `kubectl rollout undo deployment/<name>` |
| Uncordon node | Node recovered from failure | `kubectl uncordon <node>` |
| Increase resource limits | OOMKilled with headroom | `kubectl patch deployment ...` |

## High-Risk Actions (Require Approval)

| Action | Risk | Approval | Examples |
|--------|------|----------|----------|
| Drain node | Disrupts all pods on node | SRE lead | Node maintenance, hardware issue |
| etcd snapshot restore | Potential data loss | Engineering director | Data corruption, cluster recovery |
| Certificate rotation | Brief service interruption | SRE lead + security | Expired certs, compromised keys |
| NetworkPolicy change | May block legitimate traffic | Network team lead | Security incident, policy fix |
| Cluster upgrade | Version incompatibility risk | Platform team + change advisory | Planned upgrade cycle |

## Learning from Incidents

After every P0/P1 incident:
1. Map the incident path to the FTA - was it covered?
2. If new: propose a new FTA path in PROPOSED state
3. If existing but missed: update observability for the bottom event
4. If existing but slow to fix: improve the remediation runbook
5. Update probability weights in the [[skills/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]] based on outcome

## Related

- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[deployment]] — Deployment
- [[coredns]] — CoreDNS
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[concepts/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]
- [[concepts/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]]
- [[skills/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]].md|Kubernetes Diagnostic Skills Overview]]
- [[entities/kubectl Scenario Quick Reference.md|kubectl Scenario Quick Reference]]
- [[skills/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]]

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]
- [[domain-17-system-foundation/topic-dictionary/networking/service.md|Service]]
