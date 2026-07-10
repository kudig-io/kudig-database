---
title: OPA (Open Policy Agent)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- security
- opa
- crd
- operator
- rag
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- OPA (Open Policy Agent) 是什么
- 如何 OPA (Open Policy Agent)
trigger_keywords:
- OPA
- Open
- Policy
- Agent
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OPA (Open Policy Agent)

> **CNCF 状态**: Graduated | **类别**: Security | **主要语言**: Go

## 概述

title: Open Policy Agent (OPA)

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，opa 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/storage-model.md|storage-model]]
- [[pod-lifecycle]]
- [[concepts/security-defense-depth.md|security-defense-depth]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[modelpack]] — ModelPack
- [[oauth2-proxy]] — OAuth2 Proxy
- [[schemahero]] — SchemaHero
- [[composefs]] — composefs
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 14-policy-engines-opa-kyverno
- 09-opa-gatekeeper-policy
- 99-opa-gatekeeper-policy-guide
- copa
- opa
- RELEASE-NOTES-0.43
- RELEASE-NOTES-0.12
- RELEASE-NOTES-0.26
- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- RELEASE-NOTES-0.67
- RELEASE-NOTES-0.36
- RELEASE-NOTES-0.53
- RELEASE-NOTES-0.22
- RELEASE-NOTES-0.16
- RELEASE-NOTES-0.47
- RELEASE-NOTES-0.57
- RELEASE-NOTES-0.32
- RELEASE-NOTES-0.63
- RELEASE-NOTES-0.23
- RELEASE-NOTES-0.17
- RELEASE-NOTES-0.46
- RELEASE-NOTES-0.56
- RELEASE-NOTES-0.33
- RELEASE-NOTES-0.62
- RELEASE-NOTES-0.42
- RELEASE-NOTES-0.13
- RELEASE-NOTES-0.27
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.66
- RELEASE-NOTES-0.9
- RELEASE-NOTES-0.37
- RELEASE-NOTES-0.52
- RELEASE-NOTES-0.49
- RELEASE-NOTES-0.18
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-0.59
- RELEASE-NOTES-0.28
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-0.38
- RELEASE-NOTES-0.69
- RELEASE-NOTES-0.6
- RELEASE-NOTES-0.29
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-0.39
- RELEASE-NOTES-0.7
- RELEASE-NOTES-0.68
- RELEASE-NOTES-0.48
- RELEASE-NOTES-0.19
- RELEASE-NOTES-1.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-0.58
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-0.4
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.5
- RELEASE-NOTES-0.20
- RELEASE-NOTES-0.45
- RELEASE-NOTES-0.14
- RELEASE-NOTES-0.55
- RELEASE-NOTES-0.61
- RELEASE-NOTES-0.30
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.41
- RELEASE-NOTES-0.24
- RELEASE-NOTES-0.34
- RELEASE-NOTES-0.65
- RELEASE-NOTES-0.51
- RELEASE-NOTES-0.11
- RELEASE-NOTES-0.40
- RELEASE-NOTES-0.25
- RELEASE-NOTES-0.35
- RELEASE-NOTES-0.64
- RELEASE-NOTES-0.50
- RELEASE-NOTES-0.21
- RELEASE-NOTES-0.70
- RELEASE-NOTES-0.44
- RELEASE-NOTES-0.15
- RELEASE-NOTES-0.54
- RELEASE-NOTES-0.60
- RELEASE-NOTES-0.31
- [[entities/release-notes-security.md|发布说明索引 — 安全]] — Cross-reference
- [[concepts/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]] — Cross-reference
- [[concepts/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]] — Cross-reference
- [[concepts/multi-tenancy-isolation.md|Multi-Tenancy Isolation]] — Cross-reference
- [[concepts/security-tool-evolution.md|安全工具演进]] — Cross-reference
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
