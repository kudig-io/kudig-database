---
title: SPIFFE (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
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
tier: peripheral
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# SPIFFE

> **CNCF 状态**: Graduated | **类别**: Security | **主要语言**: 规范文档

## 概述

description: '## 项目概述'

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，spiffe 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[istio]]
- [[实体/vault.md|[[HashiCorp Vault|vault]]]]
- [[实体/csi-drivers.md|csi-drivers]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[tikv]] — TiKV
- [[k8gb]] — K8GB
- [[lima]] — Lima
- [[kubeflow]] — Kubeflow
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- spiffe
- [[实体/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]
- [[生态参考/领域索引/cert-index.md|Certificate / TLS 证书知识图谱索引]]
- [[生态参考/领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
