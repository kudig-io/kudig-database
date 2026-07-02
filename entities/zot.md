---
title: zot (entities)
description: '## 概述'
summary: 'zot 是一个生产就绪的、OCI 原生的容器镜像注册表，完全基于 OCI Distribution Specification 构建。它以单一二进制文件的形式提供，内置镜像存储、搜索、签名验证、漏洞扫描等功能，无需依赖外部数据库或缓存服务。'
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
tier: supporting
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# zot

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

zot 是一个生产就绪的、OCI 原生的容器镜像注册表，完全基于 OCI Distribution Specification 构建。它以单一二进制文件的形式提供，内置镜像存储、搜索、签名验证、漏洞扫描等功能，无需依赖外部数据库或缓存服务。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

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

- [[entities/trivy.md|trivy]]
- [[deployment]]
- [[concepts/storage-model.md|storage-model]]
- [[concepts/security-defense-depth.md|security-defense-depth]]

## Related

- [[envoy]] — Envoy
- [[cert-manager]] — cert-manager
- [[entities/trivy.md|trivy]] — Trivy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[distribution]] — Distribution

- zot
- [[entities/modelpack.md|[[ModelPack|ModelPack]]]]
- [[entities/kitops.md|KitOps]]
- [[entities/copa.md|Copa (Copacetic)]]
- [[entities/stacker.md|Stacker]]
- [[entities/xregistry.md|xRegistry]]
- [[entities/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
