---
title: Kubernetes FTA Top Events Index
description: Kubernetes FTA Top Events Index — Kubernetes 生产运维知识库
category: skill
tags:
- k8s
- fta
- troubleshooting
- top-events
- index
- etcd
- kubelet
- scheduler
- flannel
- containerd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes FTA Top Events Index 是什么
- 如何 Kubernetes FTA Top Events Index
trigger_keywords:
- Kubernetes
- FTA
- Top
- Events
- Index
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# Kubernetes FTA Top Events Index

## Top Events Overview (TE-1 through TE-16)

| ID | Top Event | Severity | Impact | Typical Symptoms | ACK-Specific |
|----|-----------|----------|--------|-----------------|-------------|
| TE-1 | Cluster completely unavailable | P0 | Entire cluster | kubectl cannot connect, all services down | ECS/ESSD/SLB |
| TE-2 | Application [[Service|service]] unavailable | P0 | Specific app | HTTP 5xx errors, users cannot access | ASM/ARMS/Terway |
| TE-3 | Pod startup failure | P1 | Specific Pod | Pod in Pending/Error state | ACK scheduler |
| TE-4 | Network communication anomaly | P1 | Network layer | DNS failure, Pod communication broken | Terway ENI/IPVLAN |
| TE-5 | Storage access failure | P1 | Storage layer | PVC bind failure, volume mount failure | OSS/CSI/NAS |
| TE-6 | Resource scheduling anomaly | P2 | Scheduling | Pod unschedulable, scheduler abnormal | ACK resource quota |
| TE-7 | Security authentication failure | P1 | Security | Auth/authz failure, certificate expiry | ACK RAM/PSP |
| TE-8 | Monitoring/alerting anomaly | P2 | Observability | Metrics missing, alerts not firing | ARMS/MSP |
| TE-9 | Terway network failure | P1 | Pod network | Pod cannot get IP/network unreachable | Terway exclusive |
| TE-10 | ASM service mesh failure | P1 | Mesh traffic | Sidecar cannot connect, mTLS failure | ASM exclusive |
| TE-11 | ACK-One multi-cluster anomaly | P1 | Multi-cluster | Cluster registration failure, config sync delay | ACK-One exclusive |
| TE-12 | Resource quota exceeded | P2 | Account level | API object creation failure, quota exhausted | ACK exclusive |
| TE-13 | Change management failure | P1 | Change process | Upgrade failure, rollback, config drift | GitOps/RAC |
| TE-14 | Capacity planning failure | P2 | Resource capacity | Node resource exhausted, storage capacity insufficient | Auto-scaling |
| TE-15 | Disaster recovery failure | P0 | Business continuity | Backup/restore failure, DR drill failure | Backup/DR |
| TE-16 | Observability integrity missing | P2 | Monitoring blindspot | Key metrics missing, trace broken | OTel/observability |

## Detailed Fault Tree: TE-1 Cluster Unavailable

```
TE-1: Cluster completely unavailable [OR gate] P0
│
├── IE-1.1 Control plane failure [OR gate]
│   ├── BE-1.1 API Server failure
│   │   ├── API Server OOM (from etcd data volume pressure)
│   │   ├── API Server certificate expiry
│   │   ├── API Server network unreachable (SLB/security group)
│   │   ├── API Server startup parameter error
│   │   └── API Server dependency failure
│   ├── BE-1.2 etcd cluster failure
│   │   ├── etcd disk space exhausted (ESSD downgrade, snapshot accumulation)
│   │   ├── etcd quorum lost (network partition, split-brain)
│   │   ├── etcd data corruption (ESSD write-back inconsistency)
│   │   ├── etcd performance degraded (WAL log accumulation)
│   │   └── etcd certificate issues
│   ├── BE-1.3 Scheduler failure
│   └── BE-1.4 Controller Manager failure
│
├── IE-1.2 Worker node batch failure [AND gate - needs majority]
│   ├── BE-1.5 Kubelet service failure (OOM, API disconnect, cert expiry)
│   ├── BE-1.6 Container runtime failure (containerd, Docker, CNI)
│   └── BE-1.7 Node network failure
│
├── IE-1.3 Network infrastructure failure [OR gate]
│   ├── BE-1.8 CNI plugin failure (Terway ENI/IPVLAN, Flannel)
│   └── BE-1.9 Core network device failure (switch, NAT gateway)
│
└── IE-1.4 Cloud IaaS layer failure [OR gate] (ACK-specific)
    ├── BE-1.10 ECS instance batch failure (spot interruption, ENI disconnect, hardware)
    ├── BE-1.11 SLB failure (backend server health, listener config)
    └── BE-1.12 VPC network failure (route table, security group)
```

## Detailed Fault Tree: TE-2 Application Unavailable

```
TE-2: Application service unavailable [OR gate] P0
├── IE-2.1 Pod runtime anomaly [OR gate]
│   ├── BE-2.1 CrashLoopBackOff
│   ├── BE-2.2 ImagePullBackOff
│   ├── BE-2.3 OOMKilled
│   └── BE-2.4 Evicted
├── IE-2.2 Service/Endpoint anomaly [OR gate]
├── IE-2.3 Ingress/IngressController anomaly [OR gate]
├── IE-2.4 ASM service mesh failure [OR gate]
└── IE-2.5 ARMS application monitoring failure [OR gate]
```

## OOMKilled Bottom Event Structure (Reference)

Each bottom event in the FTA includes structured diagnostic information:

```yaml
bottom_event:
  id: "BE-2.3"
  name: "OOMKilled"
  description: "Container terminated by Linux OOM Killer due to memory exceeding limits"

  observable:
    metrics:
      - "container_memory_usage_bytes / container_spec_memory_limit_bytes > 0.95"
    logs:
      - "OOMKilled in container"
      - "Exit Code: 137"
    events:
      - "kubectl get events --field-selector reason=OOMKilling"

  root_causes:
    - "Application memory leak"
    - "JVM heap size set too large"
    - "Resource limits set too low"
    - "Traffic spike causing memory surge"
    - "Sidecar container memory not accounted"

  diagnosis_commands:
    - "kubectl describe pod <pod> | grep -A5 'Last State'"
    - "kubectl top pod <pod> --containers"
    - "kubectl logs <pod> --previous"

  healing_actions:
    - id: "HA-2.3.1"
      description: "Increase memory limits"
      risk: "low"
      auto_healable: true
    - id: "HA-2.3.2"
      description: "Analyze memory leak"
      risk: "none"
      auto_healable: false
```

## Related

- [[entities/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[skills/FTA Methodology and Core Principles.md|FTA Methodology and Core Principles]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
- [[skills/Symptom Vector Matching Engine.md|Symptom Vector Matching Engine]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]]
- [[concepts/etcd Operational Reference.md|etcd Operational Reference]]
- [[concepts/Kubernetes Fault Distribution and MTTR.md|Kubernetes Fault Distribution and MTTR]]
- [[docs/ERROR-FTA-MAP.md|KUDIG 错误码 → FTA 映射]] — Cross-reference
- [[docs/COMMAND-DOC-MAP.md|KUDIG 命令 → 文档映射]] — Cross-reference
- [[docs/API-DOC-MAP.md|KUDIG API → 文档映射]] — Cross-reference
