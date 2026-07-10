---
title: KUDIG Man Pages Index
description: KUDIG Man Pages Index — Kubernetes 生产运维知识库
summary: KUDIG Man Pages Index — Kubernetes 生产运维知识库
category: reference
tags:
- k8s
- man-pages
- reference
- components
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Man Pages Index 是什么
- 如何 KUDIG Man Pages Index
trigger_keywords:
- KUDIG
- Man
- Pages
- Index
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- gitops-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- tls-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[实体/kudig-man-pages-index.md|kudig-man-pages-index]]

## Section 1: KUDIG Tools

| Man Page | Description |
|----------|-------------|
| `kudig-fta-viz(1)` | Fault tree visualization tool for rendering FTA diagrams |
| `kudig-quality(1)` | Knowledge base quality assessment and gap analysis |
| `kudig-stats(1)` | Knowledge base statistics and coverage metrics |
| `kudig-validate(1)` | Document validation and frontmatter checking |

## Section 8: Component Guides

| Man Page | Component | Description |
|----------|-----------|-------------|
| `kubernetes(8)` | Kubernetes | Production-grade container orchestration platform. Core components: kube-apiserver, etcd, kube-scheduler, kube-controller-manager, kubelet, kube-proxy. Workloads: Pod, Deployment, StatefulSet, DaemonSet, Job. Network: Service, Ingress, NetworkPolicy. Storage: PV, PVC, StorageClass. |
| `etcd(8)` | etcd | Distributed reliable key-value store. Raft consensus, MVCC storage. Cluster topology (3/5/7 nodes). Key commands: member list, endpoint health, snapshot save/restore. |
| `containerd(8)` | containerd | Container runtime. Industry-standard containerd for running and managing containers in Kubernetes. |
| `helm(8)` | Helm | Kubernetes package manager. Chart management, release management, templating. |
| `istio(8)` | Istio | Service mesh for Kubernetes. Traffic management, security (mTLS), observability, policy enforcement. |
| `cilium(8)` | Cilium | eBPF-based CNI plugin. Network policies, service mesh, observability through eBPF. |
| `prometheus(8)` | Prometheus | Monitoring and alerting toolkit. Time-series database, PromQL, alerting rules, service discovery. |
| `velero(8)` | Velero | Backup and disaster recovery for Kubernetes. Cluster resource backup, volume snapshot, migration. |
| `argocd(8)` | ArgoCD | GitOps continuous delivery tool. Declarative application deployment, sync, health assessment. |
| `cert-manager(8)` | cert-manager | Certificate management controller. Automated TLS certificate provisioning and renewal from various issuers (Let's Encrypt, HashiCorp Vault, etc.) |

## Kubernetes Core Concepts (from kubernetes(8))

### Architecture Components

- **kube-apiserver**: Front-end for the Kubernetes control plane API
- **etcd**: Consistent and highly-available key-value store for cluster data
- **kube-scheduler**: Watches for new Pods and selects nodes for them
- **kube-controller-manager**: Runs controller processes (node, replication, endpoints, etc.)
- **kubelet**: Agent running on each node, ensures containers are running
- **kube-proxy**: Maintains network rules on nodes for Pod communication

### Workload Resources

- **Pod**: Smallest deployable unit, one or more containers
- **Deployment**: Declarative updates for Pods and ReplicaSets
- **StatefulSet**: Workloads for stateful applications
- **DaemonSet**: Ensures all/some nodes run a copy of a Pod
- **Job/CronJob**: One-time and scheduled tasks

### KUDIG Documentation Mapping

| Domain | Path | Focus |
|--------|------|-------|
| Architecture Fundamentals | 集群基础/ | K8s architecture, core components, upgrade strategies |
| Design Principles | 集群基础/ | Declarative API, controller pattern, etcd consensus |
| Control Plane | 集群基础/ | Deep dive into control plane components |
| Workloads | 工作负载/ | Pod lifecycle, scheduling, HPA/VPA |
| Networking | 网络/ | CNI, Service, Ingress, Gateway API |
| Storage | 存储/ | PV/PVC, CSI drivers |
| Security | 安全/ | RBAC, network policy, runtime security |
| Observability | 可观测性/ | Monitoring, logging, distributed tracing |
| Platform Ops | 平台工程/ | Cluster management, GitOps, cost optimization |
| Troubleshooting | 故障诊断/ | Full component troubleshooting |

## Related

- [[reference|#reference Hub]] — tag hub

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/argocd.md|argocd]] — ArgoCD
- [[概念/controller-pattern.md|controller-pattern]] — Controller Pattern (Reconciliation Loop)
- [[pod-lifecycle]] — Pod Lifecycle
- [[概念/declarative-api.md|declarative-api]] — Declarative API
- [[概念/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]]
- [[概念/etcd Operational Reference.md|etcd Operational Reference]]
- [[实体/kubectl Scenario Quick Reference.md|kubectl Scenario Quick Reference]]

- [[系统基础/速查卡/k8s.md|k8s]]
- [[概念/Kubernetes Core Concepts.md|Kubernetes Core Concepts]]
- [[实体/KUDIG Cheat Sheet Index.md|KUDIG Cheat Sheet Index]] — Cross-reference


<!-- risk-assessed -->
