---
title: Helm (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- storage
- helm
- jaeger
- argocd
- flux
- crd
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Helm 是什么
- 如何 Helm
trigger_keywords:
- Helm
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
- ebpf-basics
- tracing-basics
created: "2026-05-23"
---

# Helm

> **CNCF 状态**: Graduated | **类别**: Storage | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- **Chart 打包**: 将 Kubernetes 资源打包为可重用的 Chart
- **模板引擎**: Go 模板语法支持动态配置
- **版本管理**: Release 版本控制和回滚
- **依赖管理**: Chart 依赖声明和管理
- **仓库系统**: Chart 分发和共享
- **钩子机制**: 生命周期钩子支持

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 使用语义化版本管理 Chart
- 将 Chart 存储在私有仓库
- 使用 values 文件管理环境配置
- 实施 Chart 签名和验证
- 使用 `--atomic` 保证原子性部署
- 合理设置 `--timeout` 超时时间

## 架构定位

在 CNCF 生态中，helm 属于 **Storage** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[flux]]
- [[entities/argocd|[[ArgoCD|argocd]]]]
- [[entities/vault|[[HashiCorp Vault|vault]]]]
- [[deployment]]
- [[concepts/gitops-principles|gitops-principles]]

## Related

- [[konveyor]] — Konveyor
- [[bfe]] — BFE
- [[score]] — Score
- [[jaeger]] — Jaeger
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 36-ecosystem-kustomize-helm-argocd
- 07-helm-advanced-operations
- 06-helm-charts-management
- [[domain-10-troubleshooting-diagnostics/36-helm-chart-troubleshooting|36-helm-chart-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/helm-fta|Helm 发布异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/08-cluster-operations/03-helm-troubleshooting|03-helm-troubleshooting]]
- helm
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-4.0|RELEASE-NOTES-4.0]]
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-3.18|RELEASE-NOTES-3.18]]
- RELEASE-NOTES-2.16
- RELEASE-NOTES-2.12
- RELEASE-NOTES-2.13
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-4.1|RELEASE-NOTES-4.1]]
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-3.19|RELEASE-NOTES-3.19]]
- RELEASE-NOTES-2.17
- RELEASE-NOTES-2.4
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-3.12|RELEASE-NOTES-3.12]]
- RELEASE-NOTES-3.5
- RELEASE-NOTES-2.0
- RELEASE-NOTES-3.1
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-3.16|RELEASE-NOTES-3.16]]
- RELEASE-NOTES-2.1
- RELEASE-NOTES-3.0
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-3.17|RELEASE-NOTES-3.17]]
- RELEASE-NOTES-2.5
- RELEASE-NOTES-1.2
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-3.13|RELEASE-NOTES-3.13]]
- RELEASE-NOTES-3.4
- RELEASE-NOTES-2.2
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-3.14|RELEASE-NOTES-3.14]]
- RELEASE-NOTES-3.3
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-3.20|RELEASE-NOTES-3.20]]
- RELEASE-NOTES-2.6
- RELEASE-NOTES-3.7
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-3.10|RELEASE-NOTES-3.10]]
- RELEASE-NOTES-2.7
- RELEASE-NOTES-3.6
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-3.11|RELEASE-NOTES-3.11]]
- RELEASE-NOTES-2.3
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-3.15|RELEASE-NOTES-3.15]]
- RELEASE-NOTES-3.2
- RELEASE-NOTES-2.8
- RELEASE-NOTES-2.10
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-3.9|RELEASE-NOTES-3.9]]
- RELEASE-NOTES-2.14
- RELEASE-NOTES-2.15
- RELEASE-NOTES-2.9
- RELEASE-NOTES-2.11
- [[domain-19-landscape-references/_archived-release-notes/cli-tools/helm/RELEASE-NOTES-3.8|RELEASE-NOTES-3.8]]
- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[references/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[references/release-notes-cicd-gitops|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[references/release-notes-cli-tools|发布说明索引 — CLI 工具]] — Cross-reference
- [[references/k8s-cluster-create|Kubernetes 集群创建操作指南]] — Cross-reference
- [[references/k8s-platform-extensions|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[synthesis/控制器模式 × Operator 模式|控制器模式 × Operator 模式]] — Cross-reference
- [[synthesis/GitOps x 平台工程|GitOps x 平台工程]] — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/cli-tools-evolution|CLI 工具演进]] — Cross-reference
- [[concepts/infrastructure-as-code|Infrastructure as Code]] — Cross-reference
- [[concepts/gitops-tool-evolution|GitOps 工具演进]] — Cross-reference
- [[skills/learn-05-ingress-basics|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference
- [[skills/develop-crd-operator|Develop CRD Operator]] — Cross-reference
- [[skills/k8s-distributed-tracing-guide|Kubernetes 分布式追踪最佳实践]] — Cross-reference
- [[skills/Agent Orchestration Patterns|Agent Orchestration Patterns for FTA]] — Cross-reference
- [[skills/ts-gitops-devops|GitOps/DevOps 排查]] — Cross-reference
- [[skills/learn-04-debug-tools-setup|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[skills/ts-cluster-operations|集群运维故障排查]] — Cross-reference
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/openkruise-index|OpenKruise 全局索引]]
- [[domain-19-landscape-references/topic-index/helm-index|Helm 全局索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
