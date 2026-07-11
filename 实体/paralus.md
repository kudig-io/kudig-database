---
title: Paralus (entities)
description: '## 概述'
summary: 'Paralus 是一个 Kubernetes 零信任访问管理平台，为多集群环境提供统一的身份认证、授权和审计能力。它作为 kubectl 和 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 之间的安全代理层，'
category: entities
tags:
- k8s
- cncf
- security
- paralus
- istio
- opa
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Paralus 是什么
- 如何 Paralus
trigger_keywords:
- Paralus
prerequisites:
- kubectl-basics
- service-mesh-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Paralus

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

Paralus 是一个 CNCF 沙箱项目，由 RackN 开发，是一个 Kubernetes 集群的零信任访问控制系统。它提供基于角色的访问控制（RBAC）、单点登录（SSO）和审计日志功能，让用户通过统一身份认证安全地访问多个 K8s 集群。Paralus 消除了共享 kubeconfig 和 SSH 访问的安全风险，通过短期 Token 和细粒度权限实现零信任 K8s 访问。

## Key Features（核心能力）

- **统一身份认证**：集成 OIDC/SAML/SSO 提供跨集群统一登录
- **细粒度 RBAC**：基于 Namespace、Role、Cluster 的多层访问控制
- **kubectl 代理**：通过 Paralus Proxy 代理 kubectl 命令，无需直接暴露 K8s API
- **SSO 集成**：支持 GitHub、Google、Azure AD、Okta 等身份提供商
- **审计日志**：记录所有 K8s API 操作，支持合规审计
- **多集群管理**：统一管理多个 K8s 集群的访问权限

## 架构与工作原理

Paralus 由多个组件构成：Paralus Controller 是核心管理平面，管理用户、角色、集群和策略；Paralus Connector（Adapter）部署在目标集群，作为 K8s API 的反向代理执行认证和授权；Paralus CLI（pctl）为用户提供本地 kubectl 代理。用户通过 OIDC SSO 认证后获取短期 Token，Token 通过 Paralus Proxy 转发到目标集群，Proxy 验证 Token 并根据 RBAC 策略授权或拒绝请求。

## K8s 集成

Paralus Controller 部署在管理集群上，通过 CRD 或数据库管理用户/角色/集群映射。Paralus Adapter 以 Deployment 部署在目标集群，作为 K8s API Server 前面的认证代理。用户配置 kubectl 使用 Paralus Proxy 地址而非直接连接 K8s API Server。通过 ValidatingWebhook 确保 Adapter 正确拦截所有 API 请求。

## 生产用例

- **多集群安全访问**：为团队提供统一的多集群 K8s 访问入口
- **合规审计**：满足金融/医疗行业对 K8s 操作审计的要求
- **零信任安全**：消除共享凭据，使用短期 Token 实现最小权限访问
- **外部协作者访问**：安全地为外包团队提供临时 K8s 访问

## 安装与快速开始

```bash
helm repo add paralus https://paralus.github.io/helm-charts
helm install paralus paralus/paralus -n paralus --create-namespace
# 下载 pctl CLI
pctl login paralus.example.com
pctl kubeconfig --cluster production
```

## 对比替代方案

相比 Teleport（通用基础设施访问），Paralus 专注于 K8s 集群访问控制。相比 K8s 原生 RBAC + OIDC，Paralus 提供更丰富的多集群管理和审计能力。

## Related

- [[distribution]] — Distribution
- [[03-istio-security-hardening]] — [[Istio|Istio]]io 安全加固|Istio 安全加固]]
- [[copa]] — Copa (Copacetic)
- [[nats]] — NATS
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- paralus
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
