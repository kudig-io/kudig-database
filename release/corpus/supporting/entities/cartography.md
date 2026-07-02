---
title: Cartography (entities)
description: '## 概述'
summary: 'Cartography 是一个基础设施资产图谱工具，能够自动收集多云环境（AWS、GCP、Azure）、SaaS 服务（GitHub、Okta、GSuite）和安全工具（CrowdStrike、Duo）的资产信息，并将其存储在 Neo4j 图数据库中，构建完整的基础设施关系图谱。'
category: entities
tags:
- k8s
- cncf
- security
- cartography
- containerd
- harbor
- job
- cronjob
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
- Cartography 是什么
- 如何 Cartography
trigger_keywords:
- Cartography
prerequisites:
- kubectl-basics
---



# Cartography

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Python

## 概述

Cartography 是一个基础设施资产图谱工具，能够自动收集多云环境（AWS、GCP、Azure）、SaaS 服务（GitHub、Okta、GSuite）和安全工具（CrowdStrike、Duo）的资产信息，并将其存储在 Neo4j 图数据库中，构建完整的基础设施关系图谱。安全团队和运维团队可以通过 Cypher 查询语言进行跨资源的关联分析、攻击面评估和合规审计。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **定期同步**: 配置 [[CronJob|CronJob]] 每 4-6 小时同步一次，保持资产图谱时效性
- **多账号**: 使用 AWS Organization 跨账号角色假设，统一收集所有账号资产
- **安全查询库**: 建立团队共享的安全查询模板库，标准化风险检测流程
- **数据保留**: 配置 Neo4j 节点过期策略，清理历史数据避免存储膨胀
- **权限最小化**: Cartography 使用的凭据应仅授予只读权限

## 架构定位

在 CNCF 生态中，cartography 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[concepts/secrets-management.md|secrets-management]]
- [[concepts/security-defense-depth.md|security-defense-depth]]

## Related

- [[telepresence]] — Telepresence
- [[08-containerd-multi-tenant]] — [[containerd|containerd]]rd 多租户|containerd 多租户]]租户|多租户]]
- [[harbor]] — Harbor
- [[opentofu]] — OpenTofu
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cartography
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
