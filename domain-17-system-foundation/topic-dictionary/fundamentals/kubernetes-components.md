---
title: Kubernetes 组件
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- coredns
- containerd
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 组件 是什么
- 如何 Kubernetes 组件
trigger_keywords:
- Kubernetes
- 组件
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- etcd-basics
created: "2026-05-23"
---

# [[Kubernetes|Kubernetes]] 组件

## 概述

Kubernetes 集群由控制平面（Control Plane）和一组工作节点（Worker Nodes）组成。每个组件各司其职，协同工作以维护集群的期望状态。本文档对 Kubernetes 的核心组件进行高层概述。

## 核心概念/原理

### 控制平面组件（Control Plane Components）

控制平面负责管理集群的整体状态，决策全局操作：

- **kube-apiserver**：Kubernetes 的核心组件，暴露 HTTP API，是用户和集群内部组件交互的入口。
- **[[etcd|etcd]]**：一致且高可用的键值存储，保存所有 API 服务器的数据，是集群状态的单一事实来源。
- **kube-scheduler**：负责监听未绑定到节点的 Pod，并根据资源需求、策略约束等为每个 Pod 分配合适的节点。
- **kube-controller-manager**：运行多个控制器（如节点控制器、副本控制器、端点控制器等），将集群状态驱动到期望状态。
- **cloud-controller-manager**（可选）：与底层云提供商集成，负责节点、路由、负载均衡器等云资源的生命周期管理。

### 节点组件（Node Components）

节点组件运行在每个工作节点上，负责维护运行中的 Pod 并提供 Kubernetes 运行时环境：

- **[[kubelet|kubelet]]**：确保 Pod 及其容器按照规范运行，并报告节点和 Pod 的状态。
- **kube-proxy**（可选）：维护节点上的网络规则，实现 [[Service|Service]] 的网络代理和负载均衡。
- **容器运行时（Container Runtime）**：负责运行容器，例如 containerd、CRI-O 等。

## 关键机制或特性

- **Addons（扩展组件）**：如 CoreDNS（集群 DNS）、Dashboard（Web UI）、容器资源监控、集群级日志收集等，扩展 Kubernetes 的核心功能。
- **架构灵活性**：Kubernetes 允许以多种方式部署这些组件，从小型开发环境到大规模生产集群均可适配。

## 使用场景

- 构建高可用的容器编排平台。
- 需要自动扩展、自愈和负载均衡的生产级应用部署。
- 混合云或多云场景下的统一资源管理。

## 最佳实践/注意事项

- 控制平面组件应部署在高可用模式下，避免单点故障。
- etcd 应配置定期备份，因为所有集群状态数据都存储在其中。
- 节点上除核心组件外，可能还需要 systemd 等软件来管理本地进程。

## 参考链接

- [Kubernetes Components - Official Documentation](https://kubernetes.io/docs/concepts/overview/components/)

## Related

- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
