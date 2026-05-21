---
title: Telepresence
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- networking
- telepresence
- containerd
- docker
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Telepresence 是什么
- 如何 Telepresence
trigger_keywords:
- Telepresence
prerequisites:
- kubectl-basics
---

# Telepresence

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Telepresence 是一个 Kubernetes 本地开发工具，它在本地开发环境和远程 Kubernetes 集群之间创建网络隧道。开发者可以在本地运行服务，同时访问集群中的其他服务和资源，也可以将集群流量拦截到本地进行调试。

## 核心能力

- **流量拦截**: 将 K8s 服务的请求重定向到本地
- **双向代理**: 本地访问集群服务，集群流量到本地
- **选择性拦截**: 基于 Header 条件拦截特定请求
- **DNS 代理**: 自动解析集群内服务 DNS
- **卷挂载**: 远程 Pod 卷挂载到本地
- **环境变量**: 同步远程 Pod 环境变量

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **选择性拦截**: 使用 Header 条件避免影响其他开发者
- **环境变量**: 同步远程环境变量保持一致性
- **Docker 模式**: 使用 Docker 模式确保环境一致
- **DNS 配置**: 合理配置 DNS 排除规则
- **资源清理**: 开发完成后及时 leave 和 quit
- **安全**: 注意拦截的流量可能包含敏感数据

## 架构定位

在 CNCF 生态中，telepresence 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[03-containerd-security-hardening]] — containerd 安全加固
- [[k0s]] — K0s
- [[kubeedge]] — KubeEdge
- [[docker]] — Docker
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-19-landscape-references/sandbox/telepresence/telepresence.md|telepresence]]
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
