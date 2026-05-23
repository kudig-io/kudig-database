---
title: kube-apiserver
description: kube-apiserver — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- apiserver
- control-plane
- api
- authentication
- authorization
- etcd
- kubelet
- scheduler
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kube-apiserver 是什么
- 如何 kube-apiserver
trigger_keywords:
- kube-apiserver
prerequisites:
- kubectl-basics
- etcd-basics
created: "2026-05-23"
---

# kube-apiserver

## Role

kube-apiserver is the single entry point for all Kubernetes operations. Every component (kubectl, controllers, [[kubelet|kubelet]], schedulers) communicates exclusively through the API Server. It is stateless, enabling horizontal scaling.

## Request Processing Pipeline

Each API request flows through:
1. **Authentication**: Verify identity (X.509 certs, tokens, OIDC, webhook)
2. **Authorization**: Check permissions (RBAC, ABAC, Node, webhook)
3. **Admission Control**: Mutating webhooks (modify) then Validating webhooks (reject)
4. **Schema Validation**: Ensure object structure is valid
5. **Persistence**: Write to etcd

## API Priority and Fairness (APF)

APF prevents API Server overload by classifying requests into priority levels and assigning flow schemas:
- Exempt: System-critical requests (no queuing)
- Higher priority: Control plane operations
- Lower priority: Bulk operations (list, watch)

## Key Configuration

| Parameter | Purpose | Production Default |
|-----------|---------|-------------------|
| `--etcd-servers` | etcd cluster endpoints | https://etcd1:2379,etcd2:2379,etcd3:2379 |
| `--max-requests-inflight` | Concurrent read requests | 400 (large clusters) |
| `--max-mutating-requests-inflight` | Concurrent write requests | 200 (large clusters) |
| `--event-ttl` | Event retention time | 1h (reduce etcd load) |
| `--encryption-provider-config` | Secret encryption at rest | KMS v2 or aescbc |

## Ports

| Port | Protocol | Purpose |
|------|----------|---------|
| 6443 | HTTPS | Main API endpoint |
| 8080 | HTTP | Insecure port (deprecated, disabled by default) |

## Related

- [[entities/kubelet.md|kubelet]] — kubelet
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[concepts/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security
- [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[etcd|etcd]]
- [[concepts/security-defense-depth.md|Defense-in-Depth Security]]
- [[operator-pattern|Operator Pattern]]
- [[concepts/Kubernetes Core Concepts.md|Kubernetes Core Concepts]]
