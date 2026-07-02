---
title: Audit RBAC Configurations
description: '- [[domain-19-landscape-references/98-merged-indexes/index.md|release-notes-security]]
  — 发布说明索引 — 安全'
summary: '- [[domain-19-landscape-references/98-merged-indexes/index.md|release-notes-security]]
  — 发布说明索引 — 安全'
category: skills
tags:
- k8s
- rbac
- security
- audit
- access-control
- least-privilege
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Audit RBAC Configurations 是什么
- 如何 Audit RBAC Configurations
trigger_keywords:
- Audit
- RBAC
- Configurations
prerequisites:
- kubectl-basics
---



# Audit RBAC Configurations

## Audit Process

### Step 1: Inventory All Roles and Bindings

```bash
kubectl get clusterroles,roles --all-namespaces -o wide
kubectl get clusterrolebindings,rolebindings --all-namespaces -o wide
```

### Step 2: Check for Dangerous Permissions

Look for:
- **Wildcard verbs** (`verbs: ["*"]`): Grants all operations on resources
- **Wildcard resources** (`resources: ["*"]`): Grants access to all resource types
- **ClusterRoleBinding to default ServiceAccount**: Gives cluster-wide access to all [[Pods|Pods]] in namespace
- **`[[Secrets|secrets]]` access**: Ability to read secrets is equivalent to full cluster access (via kubeconfig in secrets)

### Step 3: Verify ServiceAccount Permissions

```bash
# Check what a ServiceAccount can do
kubectl auth can-i --list --as=system:serviceaccount:default:my-sa
```

### Step 4: Principle of Least Privilege

For each binding, verify:
- Role is namespace-scoped (Role, not ClusterRole) unless cluster-wide access is required
- Verbs are specific (get, list, watch) not wildcard
- Resources are specific (pods, not "*")
- Subjects are specific ServiceAccounts, not groups like `system:authenticated`

### Step 5: Remove Unused Bindings

Identify and remove:
- Bindings to deleted subjects (ServiceAccounts, Users, Groups)
- Roles that are not referenced by any binding
- Overly broad roles where a narrower role would suffice

## Common Anti-Patterns

| Anti-Pattern | Risk | Fix |
|-------------|------|-----|
| `cluster-admin` bound to app ServiceAccount | Full cluster control | Create namespace-scoped Role |
| `verbs: ["*"]` on any resource | Unrestricted operations | Specify exact verbs needed |
| Binding to `system:serviceaccounts` group | All SA access | Bind to specific ServiceAccount |
| No RBAC at all (default permissions) | Uncontrolled access | Enable RBAC, define explicit roles |

## Automation

Use tools like `rbac-lookup` or `kubectl-view-allocations` to audit RBAC at scale. Integrate RBAC review into CI/CD pipelines.

## Related

- [[domain-19-landscape-references/98-merged-indexes/index.md|release-notes-security]] — 发布说明索引 — 安全
- [[skills/k8s-pod-security-guide.md|k8s-pod-security-guide]] — Kubernetes Pod 安全最佳实践
- [[skills/configure-health-probes.md|configure-health-probes]] — Configure Health Probes
- [[concepts/multi-tenancy-isolation.md|multi-tenancy-isolation]] — Multi-Tenancy Isolation
- [[concepts/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security
- [[concepts/security-defense-depth.md|Defense-in-Depth Security]]
- [[concepts/multi-tenancy-isolation.md|Multi-Tenancy Isolation]]
- [[skills/configure-health-probes.md|Configure Health Probes]]
