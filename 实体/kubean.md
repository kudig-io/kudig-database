---
title: Kubean (entities)
description: '## 概述'
summary: 'Kubean 是一个基于 Kubespray 的 Kubernetes 集群生命周期管理 Operator。它将 Kubespray 的集群部署能力封装为 Kubernetes CRD，使用户可以通过声明式的方式在已有的 Kubernetes 集群（管理集群）上创建、升级和管理多个 Kubernetes 集群。Kubean 支持在线和离线部署，'
category: entities
tags:
- k8s
- cncf
- runtime
- kubean
- etcd
- containerd
- crd
- operator
- ebpf
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubean 是什么
- 如何 Kubean
trigger_keywords:
- Kubean
prerequisites:
- kubectl-basics
- ebpf-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubean

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Go

## 概述

Kubean 是一个基于 Kubespray 的 Kubernetes 集群生命周期管理 Operator。它将 Kubespray 的集群部署能力封装为 Kubernetes CRD，使用户可以通过声明式的方式在已有的 Kubernetes 集群（管理集群）上创建、升级和管理多个 Kubernetes 集群。Kubean 支持在线和离线部署，兼容多种 Linux 发行版和 CPU 架构。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **管理集群**: 使用独立的轻量级 K8s 集群作为管理集群
- **离线镜像**: 预先准备离线包，确保在网络受限环境中可用
- **SSH 密钥**: 使用 SSH 密钥认证而非密码，通过 Secret 管理私钥
- **渐进升级**: 先升级 etcd，再升级控制平面，最后升级工作节点
- **备份**: 升级前备份 etcd 数据，确保可以回滚

## 架构定位

在 CNCF 生态中，kubean 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[containerd]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]

## Related

- [[实体/k8s-advanced-ecosystem.md|k8s-advanced-ecosystem]] — 硬件知识体系、CNCF 全景生态与 eBPF 平台工程
- observability.md|cncf-observability]] — CNCF 可观测性项目全景
- [[chaos-mesh]] — Chaos Mesh
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubean
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
