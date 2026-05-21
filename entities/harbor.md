---
title: Harbor
description: '## 概述'
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

# Harbor

> **CNCF 状态**: Graduated | **类别**: Storage | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- **镜像管理**: 支持 Docker 和 OCI 镜像格式
- **安全扫描**: 集成 Trivy 漏洞扫描
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

- [[domain-13-container-runtime/04-harbor-enterprise-security-scanning.md|04-harbor-enterprise-security-scanning]]
- [[domain-13-container-runtime/99-harbor-enterprise-guide.md|99-harbor-enterprise-guide]]
- [[domain-13-container-runtime/01-harbor-enterprise-image-registry.md|01-harbor-enterprise-image-registry]]
- [[domain-19-landscape-references/graduated/harbor/harbor.md|harbor]]
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
