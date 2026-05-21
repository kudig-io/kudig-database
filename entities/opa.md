---
title: OPA (Open Policy Agent)
description: '## 概述'
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

- [[domain-05-security-compliance/14-policy-engines-opa-kyverno.md|14-policy-engines-opa-kyverno]]
- [[domain-05-security-compliance/09-opa-gatekeeper-policy.md|09-opa-gatekeeper-policy]]
- [[domain-05-security-compliance/99-opa-gatekeeper-policy-guide.md|99-opa-gatekeeper-policy-guide]]
- [[domain-19-landscape-references/sandbox/copa/copa.md|copa]]
- [[domain-19-landscape-references/graduated/opa/opa.md|opa]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.43.md|RELEASE-NOTES-0.43]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.12.md|RELEASE-NOTES-0.12]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.26.md|RELEASE-NOTES-0.26]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.9.md|RELEASE-NOTES-1.9]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.8.md|RELEASE-NOTES-0.8]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.67.md|RELEASE-NOTES-0.67]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.36.md|RELEASE-NOTES-0.36]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.53.md|RELEASE-NOTES-0.53]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.22.md|RELEASE-NOTES-0.22]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.16.md|RELEASE-NOTES-0.16]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.47.md|RELEASE-NOTES-0.47]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.57.md|RELEASE-NOTES-0.57]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.32.md|RELEASE-NOTES-0.32]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.63.md|RELEASE-NOTES-0.63]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.23.md|RELEASE-NOTES-0.23]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.17.md|RELEASE-NOTES-0.17]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.46.md|RELEASE-NOTES-0.46]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.56.md|RELEASE-NOTES-0.56]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.33.md|RELEASE-NOTES-0.33]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.62.md|RELEASE-NOTES-0.62]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.42.md|RELEASE-NOTES-0.42]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.13.md|RELEASE-NOTES-0.13]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.27.md|RELEASE-NOTES-0.27]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.8.md|RELEASE-NOTES-1.8]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.66.md|RELEASE-NOTES-0.66]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.9.md|RELEASE-NOTES-0.9]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.37.md|RELEASE-NOTES-0.37]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.52.md|RELEASE-NOTES-0.52]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.49.md|RELEASE-NOTES-0.49]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.18.md|RELEASE-NOTES-0.18]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.3.md|RELEASE-NOTES-1.3]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.2.md|RELEASE-NOTES-0.2]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.59.md|RELEASE-NOTES-0.59]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.28.md|RELEASE-NOTES-0.28]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.7.md|RELEASE-NOTES-1.7]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.12.md|RELEASE-NOTES-1.12]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.38.md|RELEASE-NOTES-0.38]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.69.md|RELEASE-NOTES-0.69]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.6.md|RELEASE-NOTES-0.6]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.29.md|RELEASE-NOTES-0.29]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.6.md|RELEASE-NOTES-1.6]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.13.md|RELEASE-NOTES-1.13]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.39.md|RELEASE-NOTES-0.39]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.7.md|RELEASE-NOTES-0.7]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.68.md|RELEASE-NOTES-0.68]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.48.md|RELEASE-NOTES-0.48]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.19.md|RELEASE-NOTES-0.19]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.2.md|RELEASE-NOTES-1.2]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.3.md|RELEASE-NOTES-0.3]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.58.md|RELEASE-NOTES-0.58]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.5.md|RELEASE-NOTES-1.5]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.10.md|RELEASE-NOTES-1.10]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.4.md|RELEASE-NOTES-0.4]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.14.md|RELEASE-NOTES-1.14]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.1.md|RELEASE-NOTES-1.1]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.15.md|RELEASE-NOTES-1.15]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.0.md|RELEASE-NOTES-1.0]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.1.md|RELEASE-NOTES-0.1]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.4.md|RELEASE-NOTES-1.4]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-1.11.md|RELEASE-NOTES-1.11]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.5.md|RELEASE-NOTES-0.5]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.20.md|RELEASE-NOTES-0.20]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.45.md|RELEASE-NOTES-0.45]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.14.md|RELEASE-NOTES-0.14]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.55.md|RELEASE-NOTES-0.55]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.61.md|RELEASE-NOTES-0.61]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.30.md|RELEASE-NOTES-0.30]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.10.md|RELEASE-NOTES-0.10]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.41.md|RELEASE-NOTES-0.41]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.24.md|RELEASE-NOTES-0.24]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.34.md|RELEASE-NOTES-0.34]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.65.md|RELEASE-NOTES-0.65]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.51.md|RELEASE-NOTES-0.51]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.11.md|RELEASE-NOTES-0.11]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.40.md|RELEASE-NOTES-0.40]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.25.md|RELEASE-NOTES-0.25]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.35.md|RELEASE-NOTES-0.35]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.64.md|RELEASE-NOTES-0.64]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.50.md|RELEASE-NOTES-0.50]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.21.md|RELEASE-NOTES-0.21]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.70.md|RELEASE-NOTES-0.70]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.44.md|RELEASE-NOTES-0.44]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.15.md|RELEASE-NOTES-0.15]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.54.md|RELEASE-NOTES-0.54]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.60.md|RELEASE-NOTES-0.60]]
- [[domain-19-landscape-references/topic-release-notes/security/opa/RELEASE-NOTES-0.31.md|RELEASE-NOTES-0.31]]
- [[references/release-notes-security|发布说明索引 — 安全]] — Cross-reference
- [[synthesis/纵深防御 x 供应链安全|纵深防御 x 供应链安全]] — Cross-reference
- [[synthesis/控制器模式 × Operator 模式|控制器模式 × Operator 模式]] — Cross-reference
- [[concepts/multi-tenancy-isolation|Multi-Tenancy Isolation]] — Cross-reference
- [[concepts/security-tool-evolution|安全工具演进]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
