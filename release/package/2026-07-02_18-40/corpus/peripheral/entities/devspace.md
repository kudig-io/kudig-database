---
title: DevSpace (entities)
description: '## 概述'
summary: 'DevSpace 是一款开源的 Kubernetes 开发工具，旨在简化云原生应用的开发工作流。它提供热重载、实时同步、远程调试等功能，让开发者可以直接在 Kubernetes 集群中开发和测试应用，而无需在本地环境复现复杂的微服务架构。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- devspace
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
- DevSpace 是什么
- 如何 DevSpace
trigger_keywords:
- DevSpace
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# DevSpace

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

DevSpace 是一款开源的 Kubernetes 开发工具，旨在简化云原生应用的开发工作流。它提供热重载、实时同步、远程调试等功能，让开发者可以直接在 Kubernetes 集群中开发和测试应用，而无需在本地环境复现复杂的微服务架构。

## 核心能力

- **热重载**: 代码更改自动同步并重新部署
- **文件同步**: 双向文件同步到容器
- **端口转发**: 自动管理端口转发
- **远程调试**: 支持 VS Code 和 IDE 远程调试
- **日志流**: 实时聚合多 Pod 日志
- **依赖管理**: 管理服务间依赖顺序

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **镜像策略**: 使用 `rebuildStrategy` 减少不必要的构建
- **同步排除**: 排除 node_modules、.git 等大目录
- **Profile 管理**: 为不同环境创建独立 profile
- **依赖顺序**: 明确定义服务启动顺序
- **资源限制**: 在集群中为开发 Pod 设置资源限制
- **清理策略**: 定期使用 `devspace purge` 清理旧资源

## 架构定位

在 CNCF 生态中，devspace 属于 **CI/CD** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[pod-lifecycle]]
- [[concepts/security-defense-depth.md|security-defense-depth]]

## Related

- [[bpfman]] — bpfman
- [[in-toto]] — in-toto
- [[grpc]] — gRPC
- [[kagent]] — Kagent
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- devspace
- [[entities/cncf-runtime.md|[[CNCF 容器运行时与工具链项目全景|CNCF 容器运行时与工具链项目全景]]]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
