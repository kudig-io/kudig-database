---
title: FTA-Driven Runbook Automation
description: FTA-Driven Runbook Automation — Kubernetes 生产运维知识库
category: skill
tags:
- k8s
- fta
- runbook
- automation
- remediation
- etcd
- prometheus
- mysql
- daemonset
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- FTA-Driven Runbook Automation 是什么
- 如何 FTA-Driven Runbook Automation
trigger_keywords:
- FTA-Driven
- Runbook
- Automation
prerequisites:
- kubectl-basics
- prometheus-basics
- etcd-basics
- mysql-basics
---

# FTA-Driven Runbook Automation

## Overview

FTA bottom events are the bridge from diagnosis to remediation. Each bottom event (BE) contains structured healing actions with risk assessment, preconditions, and rollback plans, enabling automated runbook execution.

## Healing Action Structure

```yaml
healing_action:
  id: "HA-2.3.1"                     # Links to BE-2.3
  description: "Increase memory limits"
  risk: low | medium | high | critical
  auto_healable: true | false
  preconditions:
    - "Pod is in OOMKilled state"
    - "Current memory limit < namespace ResourceQuota remaining"
  estimated_duration: "2m"
  rollback_plan: "Revert to previous limits via kubectl rollout undo"
  command: |
    kubectl patch deployment <deploy> -n <namespace> --patch \
      '{"spec":{"template":{"spec":{"containers":[{"name":"app","resources":{"limits":{"memory":"2Gi"}}}]}}}}'
```

## Risk-Based Execution Strategy

| Risk Level | Execution Mode | Approval | Examples |
|-----------|---------------|----------|----------|
| **Low** | Auto-executable | None | Restart Pod, increase resource limits, uncordon node |
| **Medium** | Auto + notify | Post-execution review | Scale deployment, modify ConfigMap, restart DaemonSet |
| **High** | Human approval required | Pre-execution sign-off | Drain node, modify NetworkPolicy, change RBAC |
| **Critical** | Human approval + backup | Pre-execution + data backup | etcd restore, cluster upgrade, certificate rotation |

## Automation Pipeline

```
FTA Diagnosis Result
    ↓
[1] Match root_cause BE -> retrieve healing actions
    ↓
[2] Filter by risk level policy (auto vs manual)
    ↓
[3] Check preconditions (resource state, permissions, quotas)
    ↓
[4] Execute healing action (or queue for human approval)
    ↓
[5] Verify recovery (re-check metrics, events)
    ↓
[6] Feedback to learning loop (success/failure rate update)
```

## Typical Scenario: MySQL Split-Brain Self-Healing

```
FTA Path:
  TE: Database service unavailable [OR gate]
  └── IE: Split-brain [AND gate]
      ├── BE: Network partition between primary nodes
      └── BE: Multiple nodes claiming primary role

Agent Self-Healing Flow:
  1. Detect: Prometheus alert mysql_up == 0
  2. Navigate FTA: Locate "split-brain" path
  3. Confirm: Check read_only status on multiple nodes
  4. Repair (requires human approval - HIGH risk):
     a. Identify node with freshest data
     b. SET GLOBAL read_only = ON on other nodes
     c. Repair network partition (if possible)
     d. Rebuild replication relationship
  5. Verify: Check replication sync status, app connection recovery

Note: Database split-brain repair is HIGH risk operation.
Agent behavior: Generate repair plan -> Request human approval -> Execute after approval
```

## Typical Scenario: Multi-Cloud Fault Management

```
FTA Design (Multi-Cloud Extension):
  TE-MC: Multi-cloud application unavailable [OR gate]
  ├── IE-MC.1: AWS EKS cluster failure
  │   ├── ELB health check failure
  │   ├── EBS volume mount failure
  │   └── VPC network failure
  ├── IE-MC.2: Azure AKS cluster failure
  │   ├── Azure LB anomaly
  │   ├── Azure Disk failure
  │   └── VNet connection failure
  ├── IE-MC.3: Self-managed K8s cluster failure
  │   └── (Reference standard FTA: TE-1 ~ TE-8)
  └── IE-MC.4: Cross-cloud network failure
      ├── VPN/dedicated line interruption
      ├── DNS cross-cloud resolution failure
      └── Service Mesh cross-cloud communication failure

Agent capabilities required:
  - Call AWS API (aws eks, aws elb)
  - Call Azure API (az aks, az network)
  - Call kubectl (self-managed clusters)
  - Cross-cloud fault correlation analysis
```

## Runbook Evolution

Runbooks evolve through the learning feedback loop:
1. **Success**: Action effectiveness increases its probability weight
2. **Failure**: Action failure rate increases, triggers review
3. **New patterns**: Previously unknown symptoms trigger new runbook proposals in PROPOSED state
4. **Deprecation**: Actions with consistently low success rates are flagged for removal

## Related

- [[higress-fta]] — Higress 网关异常故障树分析
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[deployment]] — Deployment
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[skills/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[skills/Agent Orchestration Patterns.md|Agent Orchestration Patterns]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]]
- [[synthesis/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]]
- [[synthesis/Structural Troubleshooting Framework.md|Structural Troubleshooting Framework]]
