---
title: 生态组件变更日志索引
description: '# 生态组件变更日志索引'
category: entities
tags:
- k8s
- release-notes
- istio
- cilium
- prometheus
- grafana
- falco
- opa
- trivy
- rook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 生态组件变更日志索引 是什么
- 如何 生态组件变更日志索引
trigger_keywords:
- 生态组件变更日志索引
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- cilium-basics
- cni-basics
- tls-basics
- policy-basics
- backup-basics
- logging-basics
- observability-basics
---

# 生态组件变更日志索引

> 本文档是 Kubernetes 生态组件变更日志的全局索引，覆盖 6 大分类目录共 1010 个版本文件 ^[inferred]

## 分类统计

| 分类 | 组件数 | 文件数 |
|---|---|---|
| CI/CD & GitOps | 3 | 171 |
| Networking | 6 | 157 |
| Observability | 5 | 374 |
| Security | 5 | 218 |
| Storage | 3 | 76 |
| CLI Tools | 5 | 187 |
| **合计** | **27** | **1,010** |

## 快速导航

### CI/CD & GitOps

| 组件 | 文件数 | 演进参考 |
|---|---|---|
| Argo CD | 40 | [[concepts/gitops-tool-evolution.md|gitops-tool-evolution]] |
| Flux | 51 | concepts/gitops-tool-evolution#flux-版本演进 |
| Tekton Pipelines | 80 | concepts/gitops-tool-evolution#tekton-pipelines-版本演进 |

### Networking

| 组件 | 文件数 | 演进参考 |
|---|---|---|
| Istio | 38 | [[concepts/service-mesh-evolution.md|service-mesh-evolution]] |
| Envoy | 38 | concepts/service-mesh-evolution#envoy-版本演进 |
| Cilium | 24 | concepts/service-mesh-evolution#cilium-版本演进 |
| Calico | 35 | concepts/service-mesh-evolution#calico-版本演进 |
| Linkerd | 8 | concepts/service-mesh-evolution#linkerd-版本演进 |
| CNI Plugins | 14 | - |

### Observability

| 组件 | 文件数 | 演进参考 |
|---|---|---|
| OpenTelemetry Collector | 146 | concepts/observability-stack-evolution#opentelemetry-collector-版本演进 |
| Prometheus | 87 | concepts/observability-stack-evolution#prometheus-版本演进 |
| Grafana | 71 | concepts/observability-stack-evolution#grafana-版本演进 |
| Thanos | 41 | concepts/observability-stack-evolution#thanos-版本演进 |
| Loki | 29 | concepts/observability-stack-evolution#loki-版本演进 |

### Security

| 组件 | 文件数 | 演进参考 |
|---|---|---|
| OPA | 86 | [[concepts/security-tool-evolution.md|security-tool-evolution]] |
| Falco | 43 | concepts/security-tool-evolution#falco-版本演进 |
| cert-manager | 37 | concepts/security-tool-evolution#cert-manager-版本演进 |
| Trivy | 28 | concepts/security-tool-evolution#trivy-版本演进 |
| Gatekeeper | 24 | concepts/security-tool-evolution#gatekeeper-版本演进 |

### Storage

| 组件 | 文件数 | 演进参考 |
|---|---|---|
| Rook | 29 | concepts/storage-tool-evolution#rook-版本演进 |
| Velero | 28 | concepts/storage-tool-evolution#velero-版本演进 |
| Longhorn | 19 | concepts/storage-tool-evolution#longhorn-版本演进 |

### CLI Tools

| 组件 | 文件数 | 演进参考 |
|---|---|---|
| Minikube | 74 | [[concepts/cli-tools-evolution.md|cli-tools-evolution]] |
| Helm | 42 | concepts/cli-tools-evolution#helm-版本演进 |
| Kind | 32 | concepts/cli-tools-evolution#kind-版本演进 |
| Kops | 32 | concepts/cli-tools-evolution#kops-版本演进 |
| Kustomize | 7 | concepts/cli-tools-evolution#kustomize-版本演进 |

## 来源文档

全部 source 目录下的 1,010 个发布说明文件。

## Related

- [[concepts/observability-stack-evolution.md|observability-stack-evolution]] — 可观测性栈演进
- [[entities/cni-plugins.md|cni-plugins]] — CNI Plugins
- [[cilium]] — Cilium
- [[rook]] — Rook
- [[istio]] — Istio

- [[README]]
- [[README]]
- [[README]]
- [[README]]