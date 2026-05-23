---
title: Defense-in-Depth Security
description: '- [[synthesis/eBPF x 运行时安全.md|eBPF x 运行时安全]] — synthesis'
category: concepts
tags:
- k8s
- security
- rbac
- networkpolicy
- pod-security
- defense-in-depth
- etcd
- kubelet
- istio
- falco
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Defense-in-Depth Security 是什么
- 如何 Defense-in-Depth Security
trigger_keywords:
- Defense-in-Depth
- Security
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- etcd-basics
created: "2026-05-23"
---

# Defense-in-Depth Security

## Security Layers

[[Kubernetes|Kubernetes]] security follows a defense-in-depth model across four layers:

### Layer 1: Cluster Access Control

**Authentication** verifies identity via:
- X.509 client certificates
- Bearer tokens (static or ServiceAccount)
- OpenID Connect (OIDC) integration
- Webhook token authentication

**Authorization** controls access via:
- **RBAC** (Role-Based Access Control): ClusterRole/Role + ClusterRoleBinding/RoleBinding
- **ABAC** (Attribute-Based): Legacy, rarely used
- **Node Authorization**: Restricted [[kubelet|kubelet]] permissions
- **Webhook Authorization**: External authorization [[Service|service]]

**Admission Control** intercepts requests before persistence:
- **Mutating**: Modify requests (e.g., inject sidecar, set defaults)
- **Validating**: Reject non-compliant requests (e.g., resource quotas, policy engines)

### Layer 2: Network Isolation

- **NetworkPolicy**: Pod-level firewall controlling ingress/egress traffic
- **Namespace isolation**: Logical network boundaries
- **Service Mesh mTLS**: Encrypted service-to-service communication (Istio/Linkerd)

### Layer 3: Container/Runtime Security

- **Pod Security Standards**: Three levels -- Privileged, Baseline, Restricted
- **Seccomp/AppArmor/SELinux**: System call and MAC profiles
- **Capabilities**: Drop ALL, add only needed capabilities
- **Image security**: Scanning (Trivy/Clair), signing (Cosign/Notary)
- **Secure containers**: gVisor, Kata Containers for strong isolation

### Layer 4: Data Security

- **Secrets Management**: etcd encryption at rest, External Secrets Operator, Vault integration
- **Audit Logging**: Record all API operations for compliance and forensics

## Zero Trust Architecture

In zero trust, no component is inherently trusted:
- Every API request requires authentication
- Every access requires authorization (least privilege RBAC)
- All network traffic is subject to NetworkPolicy
- Runtime behavior is monitored by Falco or similar tools

## RBAC Best Practices

- Use **Role** (namespace-scoped) over **ClusterRole** when possible
- Bind to **ServiceAccounts**, not Users, for in-cluster workloads
- Apply **least privilege**: grant only required verbs on required resources
- Regularly audit RBAC with `kubectl auth can-i` checks

## Related

- [[falco]] — Falco
- [[entities/trivy.md|trivy]] — Trivy
- [[entities/vault.md|vault]] — HashiCorp Vault
- [[concepts/secrets-management.md|secrets-management]] — Secrets Management
- [[concepts/multi-tenancy-isolation.md|multi-tenancy-isolation]] — Multi-Tenancy Isolation
- [[pod-lifecycle|Pod Lifecycle]]
- [[entities/networkpolicy.md|NetworkPolicy]]
- [[skills/audit-rbac-configurations.md|Audit RBAC Configurations]]
- [[concepts/multi-tenancy-isolation.md|Multi-Tenancy Isolation]]
- [[synthesis/eBPF x 运行时安全.md|eBPF x 运行时安全]] — synthesis
- [[synthesis/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]] — synthesis
- [[synthesis/服务网格 x 零信任安全.md|服务网格 x 零信任安全]] — synthesis

- [[Deployment × Secret 管理]]