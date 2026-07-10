---
title: VS Code Kubernetes Tools (entities)
description: '## 概述'
summary: 'VS Code Kubernetes Tools 是一个功能强大的 Visual Studio Code 扩展，为 Kubernetes 开发者提供完整的开发体验。它集成了集群浏览、YAML 编辑、资源管理、日志查看、调试等功能，让开发者可以在 IDE 中完成几乎所有 Kubernetes 操作，大幅提升开发效率。'
category: entities
tags:
- k8s
- cncf
- platform
- vscode-kubernetes-tools
- prometheus
- grafana
- istio
- crd
- operator
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- VS Code Kubernetes Tools 是什么
- 如何 VS Code Kubernetes Tools
trigger_keywords:
- VS
- Code
- Kubernetes
- Tools
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# VS Code Kubernetes Tools

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: TypeScript

## 概述

VS Code Kubernetes Tools 是一个功能强大的 Visual Studio Code 扩展，为 Kubernetes 开发者提供完整的开发体验。它集成了集群浏览、YAML 编辑、资源管理、日志查看、调试等功能，让开发者可以在 IDE 中完成几乎所有 Kubernetes 操作，大幅提升开发效率。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，vscode-kubernetes-tools 属于 **Platform** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[概念/gitops-principles.md|gitops-principles]]
- [[概念/storage-model.md|storage-model]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[opengitops]] — OpenGitOps
- [[cadence]] — Cadence
- [[openkruise]] — OpenKruise
- [[02-istio-advanced-traffic-management]] — [[Istio|Istio]]io 高级流量管理|Istio 高级流量管理]]
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- vscode-kubernetes-tools
- [[实体/opengitops.md|OpenGitOps]]
- [[实体/kubeclipper.md|KubeClipper]]
- [[实体/cozystack.md|Cozystack]]
- [[实体/kube-rs.md|kube-rs]]
- [[实体/kagent.md|Kagent]]
- [[实体/openchoreo.md|OpenChoreo]]
- [[实体/holmesgpt.md|HolmesGPT]]
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[实体/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
