---
title: xRegistry (entities)
description: '## 概述'
summary: 'xRegistry 是一个通用的元数据注册中心规范，用于管理和发现事件驱动架构中的各类资源。它定义了一种标准化的 API 来注册、存储和查询消息定义、模式（Schema）、端点等元数据，支持 CloudEvents、AsyncAPI、OpenAPI 等多种规范，是构建可互操作事件驱动系统的基础设施。'
category: entities
tags:
- k8s
- cncf
- image
- xregistry
- crd
- operator
- kubeflow
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- xRegistry 是什么
- 如何 xRegistry
trigger_keywords:
- xRegistry
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# xRegistry

> **CNCF 状态**: Sandbox | **类别**: Image | **主要语言**: Go

## 概述

xRegistry 是一个 CNCF 沙箱项目，提供通用的资源注册中心抽象层。它旨在为云原生应用提供统一的注册表服务，支持多种资源类型的存储和发现，包括容器镜像、Helm Chart、OCI Artifact、API 定义等。xRegistry 基于 OCI Distribution Specification 构建，提供可扩展的存储后端和权限控制机制，适合作为私有 Registry 或多租户注册中心使用。

## Key Features（核心能力）

- **OCI 规范兼容**：完全兼容 OCI Distribution Specification v1.1
- **多类型资源**：支持容器镜像、Helm Chart、WASM 模块、SBOM 等 OCI Artifact
- **可扩展存储**：支持本地文件系统、S3、Azure Blob、GCS 等多种存储后端
- **认证授权**：支持 OAuth2、OIDC、Bearer Token 等认证方式
- **镜像复制**：支持跨 Registry 镜像复制和同步
- **API 兼容**：兼容 Docker Registry API，无需修改客户端

## 架构与工作原理

xRegistry 采用微服务架构，核心组件包括：API Server 处理 OCI 兼容的 REST API 请求；Storage Driver 层抽象不同后端存储（文件系统、对象存储）；Auth Service 处理认证和授权；Garbage Collector 定期清理未引用的镜像层。通过插件化设计，可灵活扩展存储后端和认证方式。支持水平扩展，通过共享存储后端实现无状态部署。

## K8s 集成

xRegistry 可通过 Helm Chart 部署到 Kubernetes 集群，作为集群内部的私有镜像仓库。通过 Ingress 暴露 Registry API，使用 PVC 或 S3 作为存储后端。可与 K8s ImagePullSecret 集成实现 Pod 拉取私有镜像的认证。支持与 ArgoCD、Flux 等 GitOps 工具集成，作为 Helm Chart 仓库使用。

## 生产用例

- **私有容器镜像仓库**：为企业内部提供安全可控的镜像分发服务
- **Air-gapped 环境**：为离线环境提供本地 Registry 服务
- **多集群镜像同步**：在多个 K8s 集群间同步镜像
- **OCI Artifact 存储**：存储 Helm Chart、WASM 模块等非镜像 OCI 资产

## 安装与快速开始

```bash
helm repo add xregistry https://xregistry.github.io/charts
helm install xregistry xregistry/xregistry -n registry --create-namespace
```

## 对比替代方案

相比 Harbor，xRegistry 更轻量，专注于 OCI 规范兼容的 Registry 功能。相比 Docker Distribution（registry:2），xRegistry 提供更好的扩展性和多租户支持。

## Related

- [[kubeflow]] — Kubeflow
- [[spiffe]] — SPIFFE
- [[kubeelasti]] — [[实体/kubeelasti.md|KubeElastic]]
- [[cloudevents]] — CloudEvents
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- xregistry
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
