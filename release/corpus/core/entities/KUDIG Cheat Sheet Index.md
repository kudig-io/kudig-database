---
title: KUDIG Cheat Sheet Index
description: KUDIG Cheat Sheet Index — Kubernetes 生产运维知识库
summary: KUDIG Cheat Sheet Index — Kubernetes 生产运维知识库
category: reference
tags:
- k8s
- cheatsheet
- quick-reference
- commands
- etcd
- kubelet
- prometheus
- coredns
- helm
- argocd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG Cheat Sheet Index 是什么
- 如何 KUDIG Cheat Sheet Index
trigger_keywords:
- KUDIG
- Cheat
- Sheet
- Index
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- gitops-basics
- etcd-basics
---



# KUDIG Cheat Sheet Index

## Available Cheat Sheets

| File | Topic | Focus |
|------|-------|-------|
| `k8s.md` | Kubernetes Production | Core K8s commands for v1.25-v1.32, cluster info, workload management, troubleshooting, etcd operations, API Server management |
| `kubectl-scene-cheatsheet.md` | kubectl Scenario-Based | Fault-scenario organized commands: node, Pod, network, storage, deployment troubleshooting |
| `docker.md` | Docker | Container lifecycle, image management, networking, volumes, Docker Compose |
| `helm.md` | Helm | Chart operations, release management, templating, repository management |
| `gitops.md` | GitOps | ArgoCD, Flux, GitOps workflows, sync strategies |
| `go.md` | Go Programming | Go reference for K8s operators and tooling development |
| `linux.md` | Linux | System administration, networking, storage, performance commands |
| `networking.md` | Networking | TCP/IP, DNS, HTTP, routing, firewall rules, network troubleshooting |
| `promql.md` | PromQL | Prometheus Query Language reference for monitoring and alerting |
| `sql.md` | SQL | SQL reference for database operations on K8s |
| `tls-pki.md` | TLS/PKI | Certificate management, PKI infrastructure, TLS troubleshooting |
| `gateway-api.md` | Gateway API | Kubernetes Gateway API reference for ingress/egress routing |

## Most Used: kubectl Scenario-Based Cheatsheet

The scenario-based cheatsheet is organized by **fault scenario** rather than resource type, designed for on-call engineers:

### Quick Diagnostic Patterns

| Scenario | First Command | Key Indicator |
|----------|--------------|---------------|
| Node NotReady | `kubectl get nodes -o wide` | Conditions column, kubelet logs |
| Pod Pending | `kubectl describe pod | grep Events` | FailedScheduling events |
| CrashLoopBackOff | `kubectl logs --previous` | Last container exit reason |
| OOMKilled | `kubectl describe pod | grep 'Last State'` | Exit code 137, reason: OOMKilled |
| ImagePullBackOff | `kubectl describe pod | grep ImagePull` | Auth error, image not found |
| DNS Failure | `kubectl get ep kube-dns -n kube-system` | Endpoint presence, CoreDNS pods |
| Storage Failure | `kubectl get pvc` | Pending/Binding status |

## Related

- [[entities/k8s-container-linux-fundamentals.md|k8s-container-linux-fundamentals]]

- [[entities/kudig-man-pages-index.md|kudig-man-pages-index]]

- [[entities/k8s-supply-chain-yaml-cheatsheet.md|k8s-supply-chain-yaml-cheatsheet]]

- [[domain-12-cloud-providers/05-alicloud-ack/241-ack-slb-nlb-alb.md|241-ack-slb-nlb-alb]]

- [[entities/kudig-gitbook-mac-plan.md|kudig-gitbook-mac-plan]]

- [[entities/kudig-operations-reports.md|kudig-operations-reports]]

- [[entities/kudig-documentation-specs.md|kudig-documentation-specs]]

- [[entities/k8s-skill-library-overview.md|k8s-skill-library-overview]]

- [[entities/kudig-metadata-index.md|kudig-metadata-index]]

- [[entities/k8s-ai-corpus-configuration.md|k8s-ai-corpus-configuration]]

- [[domain-19-landscape-references/98-merged-indexes/index.md|release-notes-kubernetes]]

- [[entities/k8s-glossary-index.md|k8s-glossary-index]]

- [[entities/kubernetes-port-reference.md|kubernetes-port-reference]]

- [[entities/k8s-design-principles-deep-dive.md|k8s-design-principles-deep-dive]]

- [[243-ack-ram-authorization]]

- [[entities/linux-sysctl-reference.md|linux-sysctl-reference]]

- [[coredns]] — CoreDNS
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[entities/argocd.md|argocd]] — ArgoCD
- [[entities/kubectl Scenario Quick Reference.md|kubectl Scenario Quick Reference]]
- [[kudig-man-pages-index]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]]
