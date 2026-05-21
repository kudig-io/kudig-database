---
title: Kubernetes Core Concepts
description: Kubernetes Core Concepts — Kubernetes 生产运维知识库
category: concept
tags:
- k8s
- architecture
- control-plane
- workloads
- networking
- storage
- etcd
- apiserver
- scheduler
- controller-manager
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes Core Concepts 是什么
- 如何 Kubernetes Core Concepts
trigger_keywords:
- Kubernetes
- Core
- Concepts
prerequisites:
- kubectl-basics
- cilium-basics
- cni-basics
- etcd-basics
---

# Kubernetes Core Concepts

## Design Principles

Kubernetes is built on several foundational design principles:

- **Declarative API**: Desired state is declared, controllers reconcile actual state to match
- **Controller Pattern**: Continuous reconciliation loops maintain system state
- **etcd Consensus**: Raft algorithm ensures consistent distributed state
- **Immutability**: Pods are immutable once created; changes create new Pods
- **Loose Coupling**: Components communicate only through the API Server

## Control Plane

| Component | Role | Key Failure Symptoms |
|-----------|------|---------------------|
| **kube-apiserver** | API front-end, authentication, admission control | kubectl cannot connect, 5xx errors, certificate errors |
| **etcd** | Cluster state storage, Raft consensus | Cluster read-only, quorum lost, data corruption |
| **kube-scheduler** | Pod-to-node assignment | Pods stuck in Pending, scheduling anomalies |
| **kube-controller-manager** | Controller loops (node, replication, endpoints) | Stale endpoints, unrecovered node failures |

## Workload Resources

| Resource | Use Case | Key Characteristics |
|----------|----------|---------------------|
| **Pod** | Smallest unit | Shared network/IPC/volumes, ephemeral |
| **Deployment** | Stateless apps | Rolling updates, rollback, scaling |
| **StatefulSet** | Stateful apps | Stable network identity, ordered deployment, persistent storage |
| **DaemonSet** | Node-level agents | One pod per node, auto-scheduling on new nodes |
| **Job** | One-time tasks | Runs to completion, parallelism control |
| **CronJob** | Scheduled tasks | Cron syntax, concurrency policy |

## Networking

| Concept | Purpose | Key Points |
|---------|---------|-----------|
| **Pod Network** | Pod-to-Pod communication | Flat network model, all Pods can reach each other |
| **Service** | Stable network endpoint | ClusterIP, NodePort, LoadBalancer, ExternalName |
| **Ingress** | HTTP/HTTPS routing | Host/path-based routing, TLS termination |
| **NetworkPolicy** | Traffic control | Pod-level firewall, ingress/egress rules |
| **CNI** | Container networking | Flannel, Calico, Cilium, Terway plugins |
| **CoreDNS** | Service discovery | DNS resolution for Services, external names |

## Storage

| Concept | Purpose | Key Points |
|---------|---------|-----------|
| **PersistentVolume (PV)** | Cluster storage resource | Provisioned by admin, independent of Pod lifecycle |
| **PersistentVolumeClaim (PVC)** | Storage request | Bound to PV, consumed by Pods |
| **StorageClass** | Storage provisioning template | Dynamic provisioning, reclaim policy |
| **CSI** | Container Storage Interface | Plugin standard for storage providers |

## Pod Lifecycle

```
Pending -> (Init Containers) -> ContainerCreating -> Running
  -> (termination signal) -> Terminating -> Terminated

Failure states:
  -> CrashLoopBackOff (container exits repeatedly)
  -> OOMKilled (container exceeds memory limit)
  -> Evicted (node resource pressure)
  -> ImagePullBackOff (cannot pull container image)
  -> Error (container start failure)
```

## Version Support in KUDIG

KUDIG covers Kubernetes v1.25 through v1.32, including:
- API deprecations and removals per version
- Feature gate changes
- Component status deprecation (v1.19+, use /livez and /readyz)
- `kubectl version --short` deprecation (v1.28+)

## Related

- [[concepts/declarative-api.md|declarative-api]] — Declarative API
- [[entities/kube-scheduler.md|kube-scheduler]] — kube-scheduler
- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]]
- [[concepts/etcd Operational Reference.md|etcd Operational Reference]]
- [[references/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]]
