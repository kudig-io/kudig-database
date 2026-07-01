---
title: Cloud Native Release Notes Archive
description: Cloud Native Release Notes Archive — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- prometheus
- grafana
- istio
- envoy
- cilium
- calico
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cloud Native Release Notes Archive 是什么
- 如何 Cloud Native Release Notes Archive
trigger_keywords:
- Cloud
- Native
- Release
- Notes
- Archive
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- cilium-basics
- cni-basics
- etcd-basics
- gpu-scheduling-basics
- tls-basics
- policy-basics
- backup-basics
- logging-basics
- observability-basics
created: "2026-05-23"
---

# Cloud Native Release Notes Archive

Comprehensive archive of release notes for [[Kubernetes|Kubernetes]] and its ecosystem.
**33 projects, 1321 release note files**


## Kubernetes

| Project | Versions | Directory |
|---------|----------|-----------|
| Kubernetes | 55 | [kubernetes/](kubernetes/) |

## Core Dependencies

| Project | Versions | Directory |
|---------|----------|-----------|
| [[containerd|containerd]] | 13 | [core-deps/containerd/](core-deps/containerd/) |
| [[CoreDNS|CoreDNS]] | 16 | [core-deps/coredns/](core-deps/coredns/) |
| CRI-O | 32 | [core-deps/cri-o/](core-deps/cri-o/) |
| [[etcd|etcd]] | 15 | [core-deps/etcd/](core-deps/etcd/) |
| runc | 7 | [core-deps/runc/](core-deps/runc/) |

## CLI & Tools

| Project | Versions | Directory |
|---------|----------|-----------|
| Helm | 42 | [cli-tools/helm/](cli-tools/helm/) |
| kind | 32 | [cli-tools/kind/](cli-tools/kind/) |
| kops | 32 | [cli-tools/kops/](cli-tools/kops/) |
| kustomize | 7 | [cli-tools/kustomize/](cli-tools/kustomize/) |
| minikube | 74 | [cli-tools/minikube/](cli-tools/minikube/) |

## Networking & Service Mesh

| Project | Versions | Directory |
|---------|----------|-----------|
| Calico | 35 | [networking/calico/](networking/calico/) |
| Cilium | 24 | [networking/cilium/](networking/cilium/) |
| CNI Plugins | 14 | [networking/cni-plugins/](networking/cni-plugins/) |
| Envoy | 38 | [networking/envoy/](networking/envoy/) |
| Istio | 38 | [networking/istio/](networking/istio/) |
| Linkerd | 8 | [networking/linkerd/](networking/linkerd/) |

## Observability

| Project | Versions | Directory |
|---------|----------|-----------|
| Grafana | 71 | [observability/grafana/](observability/grafana/) |
| Loki | 29 | [observability/loki/](observability/loki/) |
| OpenTelemetry Collector | 146 | [observability/opentelemetry-collector/](observability/opentelemetry-collector/) |
| Prometheus | 87 | [observability/prometheus/](observability/prometheus/) |
| Thanos | 41 | [observability/thanos/](observability/thanos/) |

## CI/CD & GitOps

| Project | Versions | Directory |
|---------|----------|-----------|
| Argo CD | 40 | [cicd-gitops/argo-cd/](cicd-gitops/argo-cd/) |
| Flux | 51 | [cicd-gitops/flux/](cicd-gitops/flux/) |
| Tekton Pipelines | 80 | [cicd-gitops/tekton/](cicd-gitops/tekton/) |

## Security & Policy

| Project | Versions | Directory |
|---------|----------|-----------|
| cert-manager | 37 | [security/cert-manager/](security/cert-manager/) |
| Falco | 43 | [security/falco/](security/falco/) |
| Gatekeeper | 24 | [security/gatekeeper/](security/gatekeeper/) |
| OPA | 86 | [security/opa/](security/opa/) |
| Trivy | 28 | [security/trivy/](security/trivy/) |

## Storage & CSI

| Project | Versions | Directory |
|---------|----------|-----------|
| Longhorn | 19 | [storage/longhorn/](storage/longhorn/) |
| Rook | 29 | [storage/rook/](storage/rook/) |
| Velero | 28 | [storage/velero/](storage/velero/) |

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index.md|Higress 知识图谱索引]]
