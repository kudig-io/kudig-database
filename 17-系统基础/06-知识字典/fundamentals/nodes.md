---
title: Nodes（节点）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Nodes（节点） 是什么
- 如何 Nodes（节点）
trigger_keywords:
- Nodes
- 节点
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Nodes（节点）

## 概述

Node（节点）是 [[kubernetes|Kubernetes]] 集群中的工作机器，可以是物理机或虚拟机。Kubernetes 通过将容器放入 Pod 中，并在 Node 上运行这些 Pod 来执行工作负载。一个集群通常包含多个节点，但在学习或资源受限的环境中也可能只有一个节点。每个节点由控制平面管理，包含运行 Pod 所必需的服务组件：[[kubelet|kubelet]]、容器运行时（[[22-概念/15-运行时与系统/container-runtime.md|container runtime]]）和 kube-proxy。

## 核心概念/原理

- **节点管理**：节点加入 API 服务器有两种主要方式：
  1. kubelet 自注册到控制平面（默认方式）
  2. 管理员手动创建 Node 对象
- **节点名称唯一性**：节点名称在同一时间内必须唯一，且 Kubernetes 假设同名节点具有相同的状态和属性。如果节点被替换或大幅更新，需先从 API 服务器删除旧 Node 对象，再重新添加。
- **节点状态（Node Status）**：包含地址（Addresses）、条件（Conditions）、容量与可分配资源（Capacity & Allocatable）、信息（Info）等字段，可通过 `kubectl describe node` 查看。
- **节点心跳（Heartbeats）**：kubelet 通过更新 Node 的 `.status` 以及 `kube-node-lease` 命名空间中的 Lease 对象来发送心跳，帮助集群判断节点可用性。
- **资源容量跟踪**：Node 对象记录节点的资源容量（如内存、CPU 数量），调度器确保节点上所有 Pod 的请求总量不超过节点容量。

## 关键机制或特性

- **节点控制器（Node Controller）**：控制平面组件，负责：
  - 为节点分配 CIDR 块（若启用）
  - 与云提供商同步可用机器列表
  - 监控节点健康，若节点不可达则将其 `Ready` 条件设为 `Unknown`，并触发 Pod 驱逐（默认等待 5 分钟）
- **驱逐速率限制**：默认每秒最多从 0.1 个节点驱逐 Pod（即每 10 秒 1 个节点）。若某个可用区不健康节点比例超过阈值（默认 55%），则降低驱逐速率或停止驱逐，以避免级联问题。
- **拓扑管理（TopologyManager）**：v1.27 起默认启用，kubelet 可基于拓扑提示进行资源分配决策。

## 使用场景

- 运行容器化工作负载的基础设施层
- 通过标签和节点选择器实现 Pod 的定向调度
- 在云环境中与云提供商集成，动态管理节点生命周期
- 节点维护前将其标记为不可调度（unschedulable），防止新 Pod 被调度上去

## 最佳实践/注意事项

- 优先使用 kubelet 自注册方式加入节点
- 更新节点配置（如 `--node-labels`）时，建议重新注册节点，而不是直接重启 kubelet，否则标签变更可能不会生效
- 将节点分布在多个可用区，以提升集群高可用性并优化节点控制器的故障恢复策略
- 手动添加节点时，需正确设置节点的容量信息

## 参考链接

- https://kubernetes.io/docs/concepts/architecture/nodes/

## Related

- [[17-系统基础/06-知识字典/fundamentals/about-cgroup-v2.md|About cgroup v2（关于 cgroup v2）]]
- [[17-系统基础/06-知识字典/fundamentals/annotations.md|注解]]
- [[17-系统基础/06-知识字典/fundamentals/bpfman.md|bpfman eBPF 管理器]]


<!-- risk-assessed -->
