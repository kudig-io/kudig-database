---
title: zot (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- image
- zot
- envoy
- opa
- crd
- operator
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- zot 是什么
- 如何 zot
trigger_keywords:
- zot
prerequisites:
- kubectl-basics
- tls-basics
- policy-basics
created: "2026-05-23"
---

# zot

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

zot 是一个生产就绪的、OCI 原生的容器镜像注册表，完全基于 OCI Distribution Specification 构建。它以单一二进制文件的形式提供，内置镜像存储、搜索、签名验证、漏洞扫描等功能，无需依赖外部数据库或缓存服务。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **TLS 加密**: 生产环境始终启用 TLS 加密通信
- **垃圾回收**: 启用 GC 定期清理未引用的镜像层
- **访问控制**: 配置细粒度的仓库级别访问策略
- **镜像同步**: 使用 onDemand 模式减少不必要的镜像拉取
- **漏洞扫描**: 启用 [[Trivy|Trivy]] 集成，定期扫描镜像漏洞
- **高可用**: 使用 S3 等共享存储后端实现多副本部署

## 架构定位

在 CNCF 生态中，zot 属于 **Image** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/trivy|trivy]]
- [[deployment]]
- [[concepts/storage-model|storage-model]]
- [[concepts/security-defense-depth|security-defense-depth]]

## Related

- [[envoy]] — Envoy
- [[cert-manager]] — cert-manager
- [[entities/trivy|trivy]] — Trivy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[distribution]] — Distribution

- zot
- [[entities/modelpack|[[ModelPack|ModelPack]]]]
- [[entities/kitops|KitOps]]
- [[entities/copa|Copa (Copacetic)]]
- [[entities/stacker|Stacker]]
- [[entities/xregistry|xRegistry]]
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
