---
title: Linkerd (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- linkerd
- prometheus
- grafana
- istio
- gateway
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Linkerd 是什么
- 如何 Linkerd
trigger_keywords:
- Linkerd
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
---



# Linkerd

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Rust, Go

## 概述

description: '## 项目概述'

## 核心能力

- name: GET /api/users
- condition:
- name: POST /api/orders
- name: GET /api/data
- [[Service|service]]: web-stable
- service: web-canary

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，linkerd 属于 **Observability** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/service-mesh-architecture.md|service-mesh-architecture]]

## Related

- [[kgateway]] — kgateway
- [[urunc]] — urunc
- [[connect-rpc]] — Connect RPC
- [[antrea]] — Antrea
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 99-linkerd-service-mesh-guide
- 02-linkerd-enterprise-service-mesh
- linkerd
- [[domain-19-landscape-references/_archived-release-notes/networking/linkerd/RELEASE-NOTES-18.9.md|RELEASE-NOTES-18.9]]
- [[domain-19-landscape-references/_archived-release-notes/networking/linkerd/RELEASE-NOTES-18.8.md|RELEASE-NOTES-18.8]]
- [[domain-19-landscape-references/_archived-release-notes/networking/linkerd/RELEASE-NOTES-18.7.md|RELEASE-NOTES-18.7]]
- RELEASE-NOTES-0.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-0.4
- RELEASE-NOTES-0.1
- RELEASE-NOTES-0.5
- [[entities/networking-terms.md|K8s 网络术语参考]] — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[entities/kudig-ecosystem-guide.md|KUDIG 开源生态指南与深度研究指南]] — Cross-reference
- [[concepts/service-mesh-evolution.md|服务网格演进]] — Cross-reference
- [[concepts/bp-security.md|最佳实践：Security]] — Cross-reference
- [[skills/ts-cloud-provider.md|云服务商集成排查]] — Cross-reference
- [[entities/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[entities/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/service-mesh-index.md|Service Mesh 服务网格知识图谱索引]]
- [[domain-19-landscape-references/topic-index/network-index.md|Network 网络知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
