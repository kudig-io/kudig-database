---
title: High Availability Patterns
description: High Availability Patterns — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- ha
- leader-election
- etcd
- anti-affinity
- pod-disruption-budget
- scheduler
- controller-manager
- pdb
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- High Availability Patterns 是什么
- 如何 High Availability Patterns
trigger_keywords:
- High
- Availability
- Patterns
prerequisites:
- kubectl-basics
- etcd-basics
- backup-basics
created: "2026-05-23"
---

# High Availability Patterns

## Control Plane HA

| Component | HA Mechanism | Minimum Replicas |
|-----------|-------------|------------------|
| **API Server** | Stateless, behind Load Balancer | 2 (recommended 3) |
| **etcd** | Raft consensus, odd nodes (2f+1) | 3 (tolerates 1 failure) |
| **Scheduler** | Leader election via Lease | 2 (recommended 3) |
| **Controller Manager** | Leader election via Lease | 2 (recommended 3) |

## etcd Cluster Sizing

| Nodes | Fault Tolerance | Use Case |
|-------|----------------|----------|
| 1 | 0 | Development |
| 3 | 1 | Small production |
| 5 | 2 | Large production |

Adding nodes beyond 5 degrades write performance due to Raft replication overhead.

## Workload HA Patterns

- **PodAntiAffinity**: Spread replicas across nodes or failure domains (topologyKey)
- **PodDisruptionBudget (PDB)**: Limit simultaneous voluntary [[Disruptions|disruptions]] during node drains or cluster upgrades
- **Topology Spread Constraints**: Built-in scheduler feature for even [[Distribution|distribution]] across failure domains (zones, nodes, hostnames)

## Leader Election

Stateful control plane components (scheduler, controller-manager) use [[Kubernetes|Kubernetes]] Lease objects for leader election:
- `leaseDuration`: How long a leader holds lock (default 15s)
- `renewDeadline`: How long leader has to renew (default 10s)
- `retryPeriod`: How often to retry (default 2s)

## Backup and Recovery

- etcd: Regular snapshots with `etcdctl snapshot save`
- Certificates: Backup `/etc/kubernetes/pki` after every change
- Manifests: Store in Git for reproducibility
- Application data: Velero for PV and resource backup to object storage

## Related
- [[synthesis/etcd × Operator 模式|etcd × Operator 模式]] — 综合

- [[concepts/eventual-consistency|eventual-consistency]] — Eventual Consistency in Kubernetes
- [[concepts/kubernetes-architecture-overview|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[concepts/security-defense-depth|security-defense-depth]] — Defense-in-Depth Security
- [[skills/backup-restore-etcd|backup-restore-etcd]] — Backup and Restore etcd
- [[etcd]] — etcd
- [[concepts/kubernetes-architecture-overview|Kubernetes Architecture Overview]]
- [[concepts/eventual-consistency|Eventual Consistency]]
- [[concepts/security-defense-depth|Defense-in-Depth Security]]
- [[skills/backup-restore-etcd|Backup and Restore etcd]]

- 08-high-availability-patterns