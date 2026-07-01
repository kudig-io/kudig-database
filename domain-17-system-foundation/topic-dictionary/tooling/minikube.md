---
title: Minikube
description: 'Minikube 是本地运行单节点 Kubernetes 集群的工具，支持 Docker、HyperKit、VirtualBox 等多种驱动。它是 K8s 学习...'
category: dictionary
tags:
- k8s
- glossary
- minikube
- local-development
- tooling
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Minikube 是什么
- Minikube 详解
trigger_keywords:
- Minikube
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# Minikube

> **英文名**: Minikube

## 概述

Minikube 是本地运行单节点 Kubernetes 集群的工具，支持 Docker、HyperKit、VirtualBox 等多种驱动。它是 K8s 学习、开发和测试的标准工具，可在个人电脑上快速启动完整的 K8s 环境。

## 核心概念/原理

### 支持的驱动

| 驱动 | 平台 | 说明 |
|------|------|------|
| Docker | macOS/Linux/Windows | 推荐，使用 Docker 容器模拟节点 |
| HyperKit | macOS | 轻量级 VM |
| Hyper-V | Windows | Windows 原生虚拟化 |
| KVM2 | Linux | Linux 原生虚拟化 |

### 常用命令

```bash
minikube start                    # 启动集群
minikube start --cpus=4 --memory=8192  # 自定义资源
minikube dashboard                # 打开 Web UI
minikube addons enable ingress    # 启用插件
minikube tunnel                   # 暴露 LoadBalancer Service
```

## 关键机制或特性

- **Addons**：一键启用 Ingress、Metrics Server、Dashboard 等。
- **Multi-Node**：`--nodes=3` 模拟多节点集群。
- **Mount**：将本地目录挂载到集群中。
- **Registry**：内置私有镜像仓库。
- **Profile**：管理多个 Minikube 集群实例。

## 使用场景与最佳实践

- K8s 新人学习使用 Minikube 快速搭建本地环境。
- 开发调试使用 `minikube tunnel` 测试 LoadBalancer 服务。
- CI/CD 流水线中使用 Minikube 运行集成测试。
- 使用 Addons 快速启用 Ingress 和 Metrics Server。
- 考虑使用 Kind 作为更轻量的替代方案。

## 参考链接

- [Minikube Official](https://minikube.sigs.k8s.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl.md|Kubectl]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kubeadm.md|Kubeadm]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/cluster.md|Cluster]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/node.md|Node]]
- [[domain-17-system-foundation/topic-dictionary/networking/ingress.md|Ingress]]
