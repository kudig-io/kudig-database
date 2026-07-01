---
title: Multi-Tenancy Isolation
description: '- [[concepts/服务网格 x 零信任安全.md|服务网格 x 零信任安全]] — synthesis'
category: concepts
tags:
- k8s
- multi-tenancy
- namespace
- rbac
- network-policy
- resource-quota
- opa
- networkpolicy
- rag
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Multi-Tenancy Isolation 是什么
- 如何 Multi-Tenancy Isolation
trigger_keywords:
- Multi-Tenancy
- Isolation
prerequisites:
- kubectl-basics
- policy-basics
created: "2026-05-23"
---

# Multi-Tenancy Isolation

## Soft Isolation (Namespace-based)

Multiple tenants share one cluster, isolated by:

| Mechanism | What it isolates |
|-----------|-----------------|
| **Namespace** | Logical grouping, naming scope |
| **RBAC** | Access control (Role + RoleBinding per namespace) |
| **ResourceQuota** | CPU, memory, storage, PVC count limits |
| **LimitRange** | Default/limits for containers without explicit settings |
| **[[NetworkPolicy|NetworkPolicy]]** | Network traffic between namespaces |
| **Pod Security Standards** | Container security enforcement level |

## Hard Isolation

| Approach | Description | Trade-offs |
|----------|-------------|------------|
| **Separate Clusters** | Each tenant gets own cluster | Highest isolation, highest cost |
| **vCluster** | Virtual Kubernetes API Server per tenant | Good isolation, shared underlying cluster |
| **Kamaji** | Kubernetes control plane as a service | Multi-tenant control planes |

## Tenant Isolation Checklist

1. Namespace per tenant with labels for identification
2. RBAC Role/RoleBinding scoped to namespace
3. ResourceQuota limiting total resource consumption
4. NetworkPolicy denying cross-namespace traffic by default
5. Pod Security Standards enforced at Restricted level
6. LimitRange for default resource bounds
7. Audit logging to track cross-tenant access attempts
8. OPA/Gatekeeper policies to prevent privilege escalation

## Related

- [[opa]] — OPA (Open Policy Agent)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[concepts/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security
- [[skills/audit-rbac-configurations.md|audit-rbac-configurations]] — Audit RBAC Configurations
- [[concepts/security-defense-depth.md|Defense-in-Depth Security]]
- [[concepts/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[skills/audit-rbac-configurations.md|Audit RBAC Configurations]]
- [[concepts/服务网格 x 零信任安全.md|服务网格 x 零信任安全]] — synthesis
- [[concepts/IaC x 多集群管理.md|IaC x 多集群管理]] — synthesis
