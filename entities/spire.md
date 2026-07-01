---
title: SPIRE (entities)
description: '## 概述'
summary: '## 概述'
category: entities
tags:
- k8s
- cncf
- security
- spire
- kubelet
- istio
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SPIRE 是什么
- 如何 SPIRE
trigger_keywords:
- SPIRE
prerequisites:
- kubectl-basics
- service-mesh-basics
---



# SPIRE

> **CNCF 状态**: Graduated | **类别**: Security | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，spire 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[istio]]
- [[operator-pattern]]
- [[pod-lifecycle]]
- [[entities/kubelet.md|[[kubelet|kubelet]]]]
- [[concepts/security-defense-depth.md|security-defense-depth]]

## Related

- [[openchoreo]] — OpenChoreo
- [[podman-desktop]] — Podman Desktop
- [[openyurt]] — OpenYurt
- [[carina]] — Carina
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- spire
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cert-index.md|Certificate / TLS 证书知识图谱索引]]
- [[domain-19-landscape-references/topic-index/security-index.md|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
