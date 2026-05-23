---
title: Argo Workflows
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- argo
- prometheus
- grafana
- helm
- argocd
- containerd
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Argo Workflows 是什么
- 如何 Argo Workflows
trigger_keywords:
- Argo
- Workflows
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- ebpf-basics
created: "2026-05-23"
---

# [[Argo|Argo]] Workflows

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- **Argo CD**: GitOps 持续交付，声明式应用部署
- **Argo Workflows**: Kubernetes 原生工作流引擎
- **Argo Events**: 事件驱动自动化
- **Argo Rollouts**: 渐进式发布策略（金丝雀、蓝绿）

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 使用 SSO 集成身份认证
- 配置 RBAC 细粒度权限
- 启用 Git webhook 触发同步
- 配置应用健康检查
- 配置 repo server 缓存
- 合理设置同步间隔

## 架构定位

在 CNCF 生态中，argo 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana|prometheus-grafana]]
- [[crossplane]]
- [[entities/vault|vault]]
- [[concepts/controller-pattern|controller-pattern]]
- [[concepts/gitops-principles|gitops-principles]]

## Related

- [[sops]] — SOPS (Secrets OPerationS)
- [[entities/argocd|argocd]] — ArgoCD
- [[ovn-kubernetes]] — OVN-Kubernetes
- [[vitess]] — Vitess
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 01-argo-cd-enterprise-gitops
- 99-argo-cd-gitops-guide
- 36-ecosystem-kustomize-helm-argocd
- 09-gitops-workflow-argocd
- [[domain-10-troubleshooting-diagnostics/38-gitops-argocd-troubleshooting|38-gitops-argocd-troubleshooting]]
- [[domain-02-workloads-applications/06-java-cicd-tekton-argocd|06-java-cicd-tekton-argocd]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/gitops-argocd-fta|GitOps(ArgoCD) 异常故障树分析]]
- RELEASE-NOTES-0.12
- RELEASE-NOTES-0.8
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.12|RELEASE-NOTES-2.12]]
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.13|RELEASE-NOTES-2.13]]
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- RELEASE-NOTES-2.4
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.7
- RELEASE-NOTES-2.0
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-3.1|RELEASE-NOTES-3.1]]
- RELEASE-NOTES-0.6
- RELEASE-NOTES-1.6
- RELEASE-NOTES-2.1
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-3.0|RELEASE-NOTES-3.0]]
- RELEASE-NOTES-0.7
- RELEASE-NOTES-2.5
- RELEASE-NOTES-1.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-1.5
- RELEASE-NOTES-2.2
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-3.3|RELEASE-NOTES-3.3]]
- RELEASE-NOTES-0.4
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.6|RELEASE-NOTES-2.6]]
- RELEASE-NOTES-1.1
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.7|RELEASE-NOTES-2.7]]
- RELEASE-NOTES-1.0
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.4
- RELEASE-NOTES-2.3
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-3.2|RELEASE-NOTES-3.2]]
- RELEASE-NOTES-0.5
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.8|RELEASE-NOTES-2.8]]
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.10|RELEASE-NOTES-2.10]]
- RELEASE-NOTES-0.10
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.14|RELEASE-NOTES-2.14]]
- RELEASE-NOTES-0.11
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.9|RELEASE-NOTES-2.9]]
- [[domain-19-landscape-references/_archived-release-notes/cicd-gitops/argo-cd/RELEASE-NOTES-2.11|RELEASE-NOTES-2.11]]
- [[entities/pixie|Pixie]]
- [[entities/kuberhealthy|Kuberhealthy]]
- [[entities/kubescape|Kubescape]]
- [[entities/perses|Perses]]
- [[entities/03-prometheus-ha-deployment|Prometheus 高可用部署]]
- [[entities/trickster|Trickster]]
- [[entities/distribution|Distribution]]
- [[entities/hami|HAMI]]
- [[entities/06-containerd-observability|containerd 可观测性]]
- [[entities/kubeelasti|KubeElastic]]
- [[journal/digest-2026-05-21-full|Wiki 全量知识库摘要 — 2026-05-21]] — Cross-reference
- [[references/kudig-ecosystem-guide|KUDIG 开源生态指南与深度研究指南]] — Cross-reference
- [[references/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[references/storage-terms|K8s 存储术语参考]] — Cross-reference
- [[references/KUDIG Tag Dictionary|KUDIG Tag Dictionary]] — Cross-reference
- [[references/k8s-deployment-create|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[references/release-notes-cicd-gitops|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[references/k8s-production-operations|生产运维：GitOps、FinOps、灾备恢复与变更管理]] — Cross-reference
- [[references/k8s-ai-infra-domain-guide|AI Infrastructure on Kubernetes Domain Guide]] — Cross-reference
- [[references/k8s-platform-extensions|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[references/platform-engineering-terms|K8s 平台工程术语参考]] — Cross-reference
- [[references/version-upgrade-guide|版本升级指南]] — Cross-reference
- [[concepts/gitops-tool-evolution|GitOps 工具演进]] — Cross-reference
- [[skills/ts-ai-ml-workloads|AI/ML 工作负载排查]] — Cross-reference
- [[skills/deployment-canary-and-bluegreen|金丝雀与蓝绿发布]] — Cross-reference
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/openkruise-index|OpenKruise 全局索引]]
- [[domain-19-landscape-references/topic-index/helm-index|Helm 全局索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
