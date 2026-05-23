---
title: Kubernetes API Versions Reference
description: Kubernetes API Versions Reference — Kubernetes 生产运维知识库
category: references
tags:
- k8s
- api
- versions
- reference
- resources
- statefulset
- daemonset
- ingress
- rbac
- networkpolicy
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes API Versions Reference 是什么
- 如何 Kubernetes API Versions Reference
trigger_keywords:
- Kubernetes
- API
- Versions
- Reference
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Kubernetes API Versions Reference

## Core API Groups

### Workload Resources (apps/v1)

| Resource | Stable Since | Purpose |
|---|---|---|
| Deployment | v1.9 | Declarative pod management |
| StatefulSet | v1.9 | Stateful application management |
| DaemonSet | v1.9 | Node-level pod deployment |
| ReplicaSet | v1.9 | Pod replica management |

### Core Resources (v1)

| Resource | Stable Since | Purpose |
|---|---|---|
| Pod | v1.0 | Smallest deployable unit |
| Service | v1.0 | Network abstraction |
| ConfigMap | v1.2 | Configuration data |
| Secret | v1.0 | Sensitive data storage |
| PersistentVolumeClaim | v1.0 | Storage request |
| Namespace | v1.0 | Resource isolation |
| Node | v1.0 | Cluster node representation |
| ServiceAccount | v1.0 | Pod identity |

### Network Resources

| Resource | API Group | Stable Since | Purpose |
|---|---|---|---|
| Ingress | networking.k8s.io/v1 | v1.19 | HTTP routing |
| NetworkPolicy | networking.k8s.io/v1 | v1.9 | Pod network isolation |
| IngressClass | networking.k8s.io/v1 | v1.19 | Ingress controller selection |

### Storage Resources

| Resource | API Group | Stable Since | Purpose |
|---|---|---|---|
| StorageClass | storage.k8s.io/v1 | v1.6 | Dynamic provisioning |
| VolumeAttachment | storage.k8s.io/v1 | v1.13 | Volume-node binding |
| CSINode | storage.k8s.io/v1 | v1.17 | CSI node info |
| CSIDriver | storage.k8s.io/v1 | v1.18 | CSI driver config |

### Scheduling Resources

| Resource | API Group | Stable Since | Purpose |
|---|---|---|---|
| PriorityClass | scheduling.k8s.io/v1 | v1.14 | Pod priority |

## RBAC Resources (rbac.authorization.k8s.io/v1)

| Resource | Stable Since | Purpose |
|---|---|---|
| Role | v1.8 | Namespace-scoped permissions |
| ClusterRole | v1.8 | Cluster-scoped permissions |
| RoleBinding | v1.8 | Bind Role to subject |
| ClusterRoleBinding | v1.8 | Bind ClusterRole to subject |

## Extension Resources

| Resource | API Group | Stable Since | Purpose |
|---|---|---|---|
| CustomResourceDefinition | apiextensions.k8s.io/v1 | v1.16 | Define custom resources |
| MutatingWebhookConfiguration | admissionregistration.k8s.io/v1 | v1.16 | Mutating admission |
| ValidatingWebhookConfiguration | admissionregistration.k8s.io/v1 | v1.16 | Validating admission |

## Notable Deprecated APIs

| Old API | Replacement | Removed In |
|---|---|---|
| extensions/v1beta1 Deployment | apps/v1 Deployment | v1.16 |
| extensions/v1beta1 Ingress | networking.k8s.io/v1 Ingress | v1.22 |
| networking.k8s.io/v1beta1 Ingress | networking.k8s.io/v1 Ingress | v1.22 |
| extensions/v1beta1 DaemonSet | apps/v1 DaemonSet | v1.16 |
| scheduling.k8s.io/v1beta1 PriorityClass | scheduling.k8s.io/v1 PriorityClass | v1.17 |
| policy/v1beta1 PodSecurityPolicy | Pod Security Admission | v1.25 |
| policy/v1beta1 PodDisruptionBudget | policy/v1 PodDisruptionBudget | v1.25 |

## Version Nomenclature

- **v1**: Stable, GA -- backward compatible, no breaking changes
- **v1beta1**: Beta -- well-tested, may have breaking changes
- **v1alpha1**: Alpha -- experimental, likely to change
- **v2beta1, v2beta2**: Versioned beta -- API evolution path

## Checking API Availability

```bash
# List all available API resources
kubectl api-resources

# Check specific resource versions
kubectl api-resources | grep -i deployment

# Check if API version exists
kubectl api-versions | grep networking.k8s.io/v1
```

## Related

- [[entities/statefulset|statefulset]] — StatefulSet
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/eventual-consistency|eventual-consistency]] — Eventual Consistency in Kubernetes
- [[concepts/kubernetes-architecture-overview|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[concepts/kubernetes-architecture-overview|Kubernetes Architecture Overview]]
- [[concepts/declarative-api|Declarative API]]
- [[concepts/eventual-consistency|Eventual Consistency]]
