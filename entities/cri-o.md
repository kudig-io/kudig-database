---
title: CRI-O (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- observability
- cri-o
- prometheus
- grafana
- containerd
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
- CRI-O 是什么
- 如何 CRI-O
trigger_keywords:
- CRI-O
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
created: "2026-05-23"
---

# CRI-O

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- name: app
- name: nginx

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，cri-o 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[containerd]]
- [[entities/cni-plugins.md|cni-plugins]]
- [[concepts/container-runtime-comparison.md|container-runtime-comparison]]
- [[concepts/storage-model.md|storage-model]]

## Related

- [[kubeslice]] — KubeSlice
- [[hyperlight]] — Hyperlight
- [[kubescape]] — Kubescape
- [[cedar]] — Cedar
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cri-o
- RELEASE-NOTES-1.9
- RELEASE-NOTES-1.28
- RELEASE-NOTES-1.18
- RELEASE-NOTES-1.19
- RELEASE-NOTES-1.8
- RELEASE-NOTES-1.29
- RELEASE-NOTES-1.16
- RELEASE-NOTES-1.22
- RELEASE-NOTES-0.2
- RELEASE-NOTES-1.32
- RELEASE-NOTES-1.26
- RELEASE-NOTES-1.12
- RELEASE-NOTES-1.27
- RELEASE-NOTES-1.13
- RELEASE-NOTES-1.17
- RELEASE-NOTES-1.23
- RELEASE-NOTES-0.3
- RELEASE-NOTES-1.33
- RELEASE-NOTES-1.24
- RELEASE-NOTES-1.10
- RELEASE-NOTES-1.34
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.20
- RELEASE-NOTES-1.30
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.21
- RELEASE-NOTES-1.31
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.25
- RELEASE-NOTES-1.11
- [[domain-19-landscape-references/_archived-release-notes/core-deps/cri-o/RELEASE-NOTES-1.35.md|RELEASE-NOTES-1.35]]
- troubleshooting|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[entities/k8s-control-plane-deep-dive.md|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[entities/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[concepts/core-dependency-version-matrix.md|核心依赖版本矩阵]] — Cross-reference
- [[concepts/docker-architecture.md|Docker Architecture and Container Runtime]] — Cross-reference
- [[concepts/node-lifecycle-management.md|节点生命周期管理]] — Cross-reference
- [[skills/ts-node-components.md|节点组件故障排查]] — Cross-reference
- [[skills/kubeadm-cluster-lifecycle.md|kubeadm 集群创建生命周期]] — Cross-reference
- [[entities/core-deps-changelog.md|核心依赖变更日志索引]] — Cross-reference
- [[entities/cncf-runtime.md|CNCF 容器运行时与工具链项目全景]] — Cross-reference
- [[entities/container-runtime.md|Container Runtime]] — Cross-reference
- [[domain-19-landscape-references/topic-index/node-index.md|Node 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
