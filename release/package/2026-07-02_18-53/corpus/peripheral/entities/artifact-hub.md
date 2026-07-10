---
title: Artifact Hub [entities]
description: '## 概述'
summary: 'Artifact Hub 是云原生制品的发现和分发平台。它是 CNCF 生态系统的中央枢纽，支持搜索、发现和发布 Helm charts、OPA 策略、Falco 规则、KEDA scalers 等多种制品类型。'
category: entities
tags:
- k8s
- cncf
- supply-chain
- artifact-hub
- helm
- opa
- falco
- crd
- operator
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Artifact Hub 是什么
- 如何 Artifact Hub
trigger_keywords:
- Artifact
- Hub
prerequisites:
- kubectl-basics
- helm-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Artifact Hub

> **CNCF 状态**: Incubating | **类别**: Supply Chain | **主要语言**: Go, TypeScript

## 概述

Artifact Hub 是云原生制品的发现和分发平台。它是 CNCF 生态系统的中央枢纽，支持搜索、发现和发布 Helm charts、OPA 策略、Falco 规则、KEDA scalers 等多种制品类型。

## 核心能力

- **统一搜索**: 跨多种制品类型的全文搜索
- **丰富元数据**: 版本、依赖、安全评级、维护者信息
- **安全扫描**: 自动检测镜像漏洞和安全问题
- **签名验证**: 支持 Cosign 签名的制品验证
- **订阅通知**: 跟踪制品更新，接收变更通知
- **私有仓库**: 支持托管私有制品仓库

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **完善元数据**: 提供详细的描述、截图、安装说明
- **版本语义化**: 遵循 SemVer 规范管理版本
- **安全扫描**: 定期更新镜像，修复漏洞
- **签名制品**: 使用 Cosign 签名增加可信度
- **保持活跃**: 定期更新和响应用户反馈

## 架构定位

在 CNCF 生态中，artifact-hub 属于 **Supply Chain** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[falco]]
- [[operator-pattern]]
- [[concepts/secrets-management.md|secrets-management]]
- [[concepts/security-defense-depth.md|security-defense-depth]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[opa]] — OPA (Open Policy Agent)
- [[helm]] — Helm
- [[keda]] — KEDA
- [[falco]] — Falco
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- artifact-hub
- [[entities/cncf-cicd.md|[[CNCF CI/CD 与发布管理项目全景|CNCF CI/CD 与发布管理项目全景]]]] — Cross-reference
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
