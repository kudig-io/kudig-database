---
title: Production Troubleshooting Playbook
description: Production Troubleshooting Playbook — Kubernetes 生产运维知识库
summary: Production Troubleshooting Playbook — Kubernetes 生产运维知识库
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
tier: core
created: '2026-05-23'
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
relationships:
- target: '[[系统基础/知识字典/networking/ingress.md]]'
  type: uses
- target: '[[实体/kubernetes.md]]'
  type: uses
- target: '[[技能/Kubernetes Diagnostic Skills Overview.md]]'
  type: uses
- target: '[[技能/Kubernetes FTA Top Events Index.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Production Troubleshooting Playbook

## Symptom-to-Diagnosis Mapping

This playbook synthesizes information from the [[实体/kubernetes.md|Kubernetes]] Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]], [[技能/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]], and Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]] into an actionable troubleshooting flow.

### Tier 1: Immediate Assessment (First 2 Minutes)

```
# 🟢 低风险：只读/信息收集，通常无副作用
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
| OOMKilled | TE-2 -> IE-2.1 -> BE-2.3 | `kubectl describe pod | grep 'Last State'` | Memory leak, limits too low |
| ImagePullBackOff | TE-3 -> IE-3.2 | `kubectl describe pod | grep ImagePull` | Auth error, image missing, network |
| Evicted | TE-2 -> IE-2.1 -> BE-2.4 | `kubectl describe pod | grep Evicted` | Node disk/memory pressure |

### Tier 3: Network Diagnosis (15-30 Minutes)

| Symptom | FTA Path | First Command | Common Root Cause |
|---------|----------|---------------|-------------------|
| DNS resolution failure | TE-4 -> IE-4.1 | `kubectl get ep kube-dns -n kube-system` | CoreDNS pods down, endpoint missing |
| Pod-to-Pod connectivity | TE-4 -> IE-4.2 | `kubectl exec -it <pod> -- curl <target>` | NetworkPolicy blocking, CNI issue |
| Service unreachable | TE-2 -> IE-2.2 | `kubectl get ep <service>` | Endpoint not populated, selector wrong |
| External access failure | TE-2 -> IE-2.3 | `kubectl get [[系统基础/知识字典/networking/ingress.md|ingress]]` | Ingress config error, cert expiry |

### Tier 4: Storage Diagnosis (15-30 Minutes)

| Symptom | FTA Path | First Command | Common Root Cause |
|---------|----------|---------------|-------------------|
| PVC Pending | TE-5 -> IE-5.1 | `kubectl describe pvc` | No matching PV, StorageClass issue |
| Mount failure | TE-5 -> IE-5.2 | `kubectl describe pod | grep MountVolume` | CSI driver down, volume not ready |
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
5. Update probability weights in the [[技能/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]] based on outcome

## Related

- [[实体/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[deployment]] — Deployment
- [[coredns]] — CoreDNS
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[概念/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]
- [[概念/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]]
- [[技能/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- [[技能/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]].md|Kubernetes Diagnostic Skills Overview]]
- [[实体/kubectl Scenario Quick Reference.md|kubectl Scenario Quick Reference]]
- [[技能/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]]

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]
- [[系统基础/知识字典/networking/service.md|Service]]


<!-- risk-assessed -->
