---
title: KUDIG Tag Dictionary
description: KUDIG Tag Dictionary — Kubernetes 生产运维知识库
summary: KUDIG Tag Dictionary — Kubernetes 生产运维知识库
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
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG Tag Dictionary

## Purpose

Unified tagging system for 3,337+ documents in the KUDIG knowledge base. Ensures consistent tagging for Agent/RAG retrieval. All tags use lowercase English with hyphens for multi-word tags.

## Level 1 Tags (Domain Classification)

| Tag | Description | Applies To |
|-----|-------------|-----------|
| `k8s` | Kubernetes core knowledge | All domain docs |
| `docker` | Docker container tech | domain-13, related |
| `linux` | Linux system | domain-14, related |
| `networking` | Network technology | domain-5, 网络 |
| `storage` | Storage technology | domain-6, 存储 |
| `security` | Security technology | domain-7, domain-25, 安全 |
| `observability` | Observability | domain-8, domain-20, 可观测性 |
| `ai` | AI/ML infrastructure | domain-11, topic-ai-* |
| `devops` | DevOps practices | domain-9, 发布变更 |
| `platform` | Platform engineering | 平台工程 |
| `mesh` | Service Mesh | 网络 |
| `gitops` | GitOps methodology | 发布变更 |
| `iac` | Infrastructure as Code | 发布变更 |
| `cncf` | CNCF ecosystem | 生态参考 |
| `ebpf` | eBPF technology | 网络 |
| `edge` | Edge computing | 专项技术 |
| `wasm` | WebAssembly | 专项技术 |
| `gateway` | API Gateway | 网络 |
| `database` | Database middleware | 数据库中间件 |
| `cloud` | Multi/hybrid cloud | domain-17, 云厂商 |
| `hardware` | Hardware | 系统基础 |
| `paper` | Academic papers | 生态参考 |
| `yaml` | YAML manifests | 清单模式 |
| `events` | Kubernetes events | 系统基础 |
| `quality` | Testing & quality | 发布变更 |
| `disaster-recovery` | DR & BCP | 可靠性 |
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
| `terway` | Terway CNI | 网络 |
| `febm` | FEBM forensics | topic-febm |
| `ai-agent` | AI agents | 02-ai-agents |
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

- [[实体/argocd.md|argocd]] — ArgoCD
- [[argo]] — Argo Workflows
- [[operator-pattern]] — Operator Pattern (CRD + Controller)
- [[概念/infrastructure-as-code.md|infrastructure-as-code]] — Infrastructure as Code
- [[概念/service-networking.md|service-networking]] — Service Networking
- [[实体/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]]
- [[技能/fta-方法论/diagnostic-overview/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]]
- [[实体/KUDIG Frontmatter Spec.md|KUDIG Frontmatter Spec]]
- [[文档/TAG-DICTIONARY.md|KUDIG 全局标签字典]]


<!-- risk-assessed -->
