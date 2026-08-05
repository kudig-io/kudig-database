---
title: Athenz (entities)
description: '## 概述'
summary: 'Athenz 是由 Yahoo (Verizon Media) 开发的开源平台，提供基于 X.509 证书的服务身份认证和细粒度的基于角色的访问控制 (RBAC)。它为微服务架构提供零信任安全模型，每个服务都获得唯一的 X.509 身份证书，所有服务间通信通过 mTLS 加密和验证。Athenz 同时支持集中式和去中心化的授权模式。'
category: entities
tags:
- k8s
- cncf
- security
- athenz
- containerd
- rbac
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
- Athenz 是什么
- 如何 Athenz
trigger_keywords:
- Athenz
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Athenz

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Java, Go

## 概述

Athenz 是由 Yahoo (Verizon Media) 开发的开源平台，提供基于 X.509 证书的服务身份认证和细粒度的基于角色的访问控制 (RBAC)。它为微服务架构提供零信任安全模型，每个服务都获得唯一的 X.509 身份证书，所有服务间通信通过 mTLS 加密和验证。Athenz 同时支持集中式和去中心化的授权模式。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **域规划**: 按组织/产品线划分域，保持域的边界清晰
- **最小权限**: 策略遵循最小权限原则，避免使用通配符
- **证书轮换**: 配置自动证书轮换，通常 24 小时更新一次
- **本地授权**: 使用 ZPE 进行本地授权决策，减少对中心服务的依赖
- **审计日志**: 启用 ZMS 审计日志，记录所有策略变更操作

## 架构定位

在 CNCF 生态中，athenz 属于 **Security** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[pod-lifecycle]]
- [[entities/csi-drivers.md|csi-drivers]]

## Related

- [[32-发布/package/2026-07-02_18-53/corpus/core/entities/07-containerd-windows-support]] — [[containerd|containerd]]rd Windows 支持|containerd Windows 支持]]
- [[cortex]] — Cortex
- [[kepler]] — Kepler
- [[kubestellar]] — KubeStellar
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- athenz
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
