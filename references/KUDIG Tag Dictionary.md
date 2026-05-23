---
title: KUDIG Tag Dictionary
description: KUDIG Tag Dictionary — Kubernetes 生产运维知识库
category: reference
tags:
- k8s
- tags
- metadata
- taxonomy
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Tag Dictionary 是什么
- 如何 KUDIG Tag Dictionary
trigger_keywords:
- KUDIG
- Tag
- Dictionary
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

# KUDIG Tag Dictionary

## Purpose

Unified tagging system for 3,337+ documents in the KUDIG knowledge base. Ensures consistent tagging for Agent/RAG retrieval. All tags use lowercase English with hyphens for multi-word tags.

## Level 1 Tags (Domain Classification)

| Tag | Description | Applies To |
|-----|-------------|-----------|
| `k8s` | Kubernetes core knowledge | All domain docs |
| `docker` | Docker container tech | domain-13, related |
| `linux` | Linux system | domain-14, related |
| `networking` | Network technology | domain-5, domain-03-networking-traffic |
| `storage` | Storage technology | domain-6, domain-04-storage-data |
| `security` | Security technology | domain-7, domain-25, domain-05-security-compliance |
| `observability` | Observability | domain-8, domain-20, domain-06-observability |
| `ai` | AI/ML infrastructure | domain-11, topic-ai-* |
| `devops` | DevOps practices | domain-9, domain-08-release-change-management |
| `platform` | Platform engineering | domain-07-platform-engineering |
| `mesh` | Service Mesh | domain-03-networking-traffic |
| `gitops` | GitOps methodology | domain-08-release-change-management |
| `iac` | Infrastructure as Code | domain-08-release-change-management |
| `cncf` | CNCF ecosystem | domain-19-landscape-references |
| `ebpf` | eBPF technology | domain-03-networking-traffic |
| `edge` | Edge computing | domain-15-specialized-tech |
| `wasm` | WebAssembly | domain-15-specialized-tech |
| `gateway` | API Gateway | domain-03-networking-traffic |
| `database` | Database middleware | domain-16-database-middleware |
| `cloud` | Multi/hybrid cloud | domain-17, domain-12-cloud-providers |
| `hardware` | Hardware | domain-17-system-foundation |
| `paper` | Academic papers | domain-19-landscape-references |
| `yaml` | YAML manifests | domain-18-manifests-patterns |
| `events` | Kubernetes events | domain-17-system-foundation |
| `quality` | Testing & quality | domain-08-release-change-management |
| `disaster-recovery` | DR & BCP | domain-09-reliability-engineering |
| `cheatsheet` | Quick reference | topic-cheat-sheet |
| `fta` | Fault Tree Analysis | topic-fta |
| `skill` | Operational skills | topic-skills |
| `troubleshooting` | Troubleshooting | domain-12, topic-structural-trouble-shooting |
| `learning` | Learning paths | topic-learn |
| `dictionary` | Ops terminology | topic-dictionary |
| `release-notes` | Version release notes | topic-release-notes |
| `migration` | Migration guides | topic-migration |
| `architecture` | Application architecture | topic-application-architecture |
| `deployment` | Deployment strategies | topic-deployment |
| `java` | Java ecosystem | domain-java-kubernetes |
| `terway` | Terway CNI | domain-03-networking-traffic |
| `febm` | FEBM forensics | topic-febm |
| `ai-agent` | AI agents | topic-ai-agent |
| `ai-coding` | AI coding | topic-ai-coding |

## Level 2 Tags (Components/Technology)

| Tag | Description | Parent |
|-----|-------------|--------|
| `architecture` | Architecture design | k8s |
| `control-plane` | Control plane | k8s |
| `etcd` | etcd distributed storage | control-plane |
| `apiserver` | API Server | control-plane |
| `scheduler` | Scheduler | control-plane |
| `controller-manager` | Controller Manager | control-plane |
| `workload` | Workloads | k8s |
| `pod` | Pod | workload |
| `deployment` | Deployment | workload |
| `statefulset` | StatefulSet | workload |
| `daemonset` | DaemonSet | workload |
| `job` | Job/CronJob | workload |
| `service` | Service networking | networking |
| `ingress` | Ingress | networking |
| `cni` | CNI plugin | networking |
| `network-policy` | Network policy | networking, security |
| `dns` | DNS resolution | networking |
| `pv` | PersistentVolume | storage |
| `pvc` | PersistentVolumeClaim | storage |
| `storage-class` | StorageClass | storage |
| `csi` | Container Storage Interface | storage |
| `rbac` | Role-Based Access Control | security |
| `pod-security` | Pod security policy | security |
| `secret` | Secret management | security |
| `certificate` | Certificate management | security |
| `prometheus` | Prometheus monitoring | observability |
| `grafana` | Grafana visualization | observability |
| `alertmanager` | Alertmanager alerts | observability |
| `logging` | Log management | observability |
| `tracing` | Distributed tracing | observability |
| `crd` | Custom Resource Definition | k8s |
| `operator` | Operator pattern | k8s |
| `webhook` | Admission Webhook | k8s |
| `gpu` | GPU scheduling | ai |
| `cuda` | CUDA computing | ai |
| `model-serving` | Model serving | ai |
| `istio` | Istio Mesh | mesh |
| `envoy` | Envoy proxy | mesh |
| `argo` | ArgoCD | gitops |
| `flux` | Flux CD | gitops |
| `helm` | Helm package management | k8s |
| `cilium` | Cilium CNI | ebpf |
| `terway` | Terway CNI | networking |
| `kubelet` | Kubelet | control-plane |
| `kube-proxy` | Kube-Proxy | networking |
| `coredns` | CoreDNS | networking |
| `hpa` | Horizontal Pod Autoscaler | workload |
| `vpa` | Vertical Pod Autoscaler | workload |
| `keda` | KEDA event-driven scaling | workload |

## Level 3 Tags (Scenario/Purpose)

| Tag | Description |
|-----|-------------|
| `troubleshooting` | Fault diagnosis |
| `best-practice` | Best practices |
| `performance` | Performance tuning |
| `configuration` | Configuration reference |
| `deployment` | Deployment guide |
| `monitoring` | Monitoring & alerting |
| `security-hardening` | Security hardening |
| `disaster-recovery` | Disaster recovery |
| `cost-optimization` | Cost optimization |
| `capacity-planning` | Capacity planning |
| `upgrade` | Version upgrade |
| `migration` | Data/platform migration |
| `compliance` | Compliance & audit |

## Related

- [[entities/argocd|argocd]] — ArgoCD
- [[argo]] — Argo Workflows
- [[operator-pattern]] — Operator Pattern (CRD + Controller)
- [[concepts/infrastructure-as-code|infrastructure-as-code]] — Infrastructure as Code
- [[concepts/service-networking|service-networking]] — Service Networking
- [[references/KUDIG Scenario Taxonomy|KUDIG Scenario Taxonomy]]
- [[skills/Kubernetes Diagnostic Skills Overview|Kubernetes Diagnostic Skills Overview]]
- [[references/KUDIG Frontmatter Spec|KUDIG Frontmatter Spec]]
- [[docs/TAG-DICTIONARY|KUDIG 全局标签字典]]
