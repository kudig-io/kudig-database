---
title: Harbor (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- storage
- harbor
- helm
- containerd
- docker
- redis
- postgresql
- rbac
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Harbor 是什么
- 如何 Harbor
trigger_keywords:
- Harbor
prerequisites:
- kubectl-basics
- helm-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Harbor

> **CNCF 状态**: Graduated | **类别**: Storage | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- **镜像管理**: 支持 Docker 和 OCI 镜像格式
- **安全扫描**: 集成 [[Trivy|Trivy]] 漏洞扫描
- **访问控制**: RBAC 和项目级权限
- **镜像复制**: 跨仓库镜像同步
- **内容签名**: Cosign/Notation 镜像签名
- **Helm Chart**: Helm Chart 仓库支持

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 使用外部 PostgreSQL 和 Redis
- 配置对象存储后端（S3、GCS）
- 启用 HTTPS 和证书
- 配置高可用部署
- 使用 CDN 加速镜像分发
- 配置镜像缓存代理

## 架构定位

在 CNCF 生态中，harbor 属于 **Storage** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/trivy.md|trivy]]
- [[concepts/secrets-management.md|secrets-management]]
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[08-containerd-multi-tenant]] — containerd 多租户
- [[docker]] — Docker
- [[helm]] — Helm
- [[entities/trivy.md|trivy]] — Trivy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 04-harbor-enterprise-security-scanning
- 99-harbor-enterprise-guide
- 01-harbor-enterprise-image-registry
- harbor
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[entities/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
