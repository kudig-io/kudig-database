---
title: SPIFFE (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- security
- spiffe
- istio
- crd
- operator
- kubeflow
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- SPIFFE 是什么
- 如何 SPIFFE
trigger_keywords:
- SPIFFE
prerequisites:
- kubectl-basics
- service-mesh-basics
created: "2026-05-23"
---

# SPIFFE

> **CNCF 状态**: Graduated | **类别**: Security | **主要语言**: 规范文档

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，spiffe 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[istio]]
- [[entities/vault|[[HashiCorp Vault|vault]]]]
- [[entities/csi-drivers|csi-drivers]]
- [[concepts/security-defense-depth|security-defense-depth]]

## Related

- [[tikv]] — TiKV
- [[k8gb]] — K8GB
- [[lima]] — Lima
- [[kubeflow]] — Kubeflow
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- spiffe
- [[entities/cncf-security|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/cert-index|Certificate / TLS 证书知识图谱索引]]
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
