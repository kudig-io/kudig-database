---
title: Develop CRD Operator
description: Develop CRD Operator — Kubernetes 生产运维知识库
category: skills
tags:
- k8s
- operator
- crd
- development
- controller
- kubebuilder
- etcd
- apiserver
- helm
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Develop CRD Operator 是什么
- 如何 Develop CRD Operator
trigger_keywords:
- Develop
- CRD
- Operator
prerequisites:
- kubectl-basics
- helm-basics
- etcd-basics
created: "2026-05-23"
---

# Develop CRD Operator

## Development Workflow

### Step 1: Design the Custom Resource

Define the API (spec and status) focusing on user intent:
- **spec**: What the user wants (e.g., `replicas`, `version`, `storageSize`)
- **status**: What the operator reports (e.g., `ready`, `conditions`, `currentVersion`)

### Step 2: Scaffold with Kubebuilder

```bash
kubebuilder init --domain example.com
kubebuilder create api --group apps --version v1 --kind Database --resource --controller
```

This generates:
- CRD YAML with OpenAPI validation schema
- Controller with Reconcile() method stub
- Types definitions (spec and status structs)

### Step 3: Implement Reconciliation

The Reconcile() function follows the [[concepts/controller-pattern|[[Controller Pattern (Reconciliation Loop)|Controller Pattern]]]]:

1. Fetch the CR instance
2. Determine desired state from spec
3. Compare with actual cluster state
4. Create/update/delete dependent resources
5. Update CR status
6. Return (requeue on error, no-requeue on success)

### Step 4: Add [[Finalizers|Finalizers]]

Finalizers prevent CR deletion before cleanup:
```go
// On creation: add finalizer
if !containsString(finalizers, "finalizer.example.com") {
    finalizers = append(finalizers, "finalizer.example.com")
}
// On deletion with finalizer: clean up resources, then remove finalizer
if !deletionTimestamp.IsZero() {
    cleanup()
    removeFinalizer()
}
```

### Step 5: Test with envtest

Use `envtest` (provided by controller-runtime) for integration testing:
- Spins up a real API Server and etcd
- Tests reconciler against real [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api|Kubernetes API]]
- Fast, no full cluster needed

### Step 6: Package and Deploy

Build container image, generate RBAC manifests from kubebuilder markers, and deploy with kustomize or Helm.

## Key Libraries

| Library | Purpose |
|---------|---------|
| **controller-runtime** | Core controller framework (Informer, workqueue, reconciler) |
| **kubebuilder** | CLI scaffolding tool with code generation |
| **operator-sdk** | Alternative CLI, supports Go, Ansible, and Helm operators |
| **client-go** | Low-level Kubernetes API client (used internally) |

## Best Practices

- Make reconciliation idempotent
- Use finalizers for resource cleanup
- Update status on every reconciliation
- Handle errors gracefully with requeue and backoff
- Add meaningful conditions to status
- Use ownerReferences for garbage collection

## Related

- [[entities/crd-custom-resources|crd-custom-resources]] — CRD (Custom Resource Definition)
- [[helm]] — Helm
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/controller-pattern|controller-pattern]] — Controller Pattern (Reconciliation Loop)
- [[operator-pattern|Operator Pattern]]
- [[concepts/controller-pattern|Controller Pattern]]
- [[entities/crd-custom-resources|CRD Custom Resources]]
- [[entities/kube-apiserver|kube-apiserver]]
