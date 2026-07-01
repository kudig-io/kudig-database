---
title: Lima (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- runtime
- lima
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
- Lima 是什么
- 如何 Lima
trigger_keywords:
- Lima
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Lima

> **CNCF 状态**: Incubating | **类别**: Runtime | **主要语言**: Go

## 概述

Lima（Linux virtual Machine）是 macOS 和 Linux 上的轻量级 Linux VM 管理工具。它类似于 WSL2，提供自动文件共享、端口转发和 containerd 集成，是 Docker Desktop 的开源替代方案。

## 核心能力

- **自动文件共享**: 主机目录自动挂载到 VM
- **自动端口转发**: VM 端口自动映射到主机
- **containerd 集成**: 内置 containerd 和 nerdctl
- **多架构支持**: AMD64、ARM64 (Apple Silicon)
- **多发行版**: Ubuntu、Debian、Fedora、Alpine 等
- **模板系统**: 预配置模板快速启动

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **资源配置**: 根据工作负载调整 CPU 和内存
- **文件共享**: 只挂载必要目录提高性能
- **模板使用**: 使用预配置模板快速启动
- **快照备份**: 定期创建 VM 快照
- **清理**: 定期清理未使用的 VM 和镜像

## 架构定位

在 CNCF 生态中，lima 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[containerd]]
- [[pod-lifecycle]]

## Related

- [[tikv]] — TiKV
- [[k8gb]] — K8GB
- [[docker]] — Docker
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- lima
- [[entities/cncf-runtime.md|[[CNCF 容器运行时与工具链项目全景|CNCF 容器运行时与工具链项目全景]]]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
