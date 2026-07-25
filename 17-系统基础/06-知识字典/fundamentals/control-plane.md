---
title: 控制平面
description: 控制平面（Control Plane）是 Kubernetes 集群的管理层，负责维护集群状态、处理 API 请求、执行调度和协调所有组件的工作。控制平面由一组...
summary: 控制平面（Control Plane）是 Kubernetes 集群的管理层，负责维护集群状态、处理 API 请求、执行调度和协调所有组件的工作。控制平面由一组...
category: dictionary
tags:
- k8s
- glossary
- control-plane
- architecture
tier: supporting
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 控制平面 是什么
- Control Plane 详解
trigger_keywords:
- 控制平面
- Control Plane
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 控制平面

> **英文名**: Control Plane

## 概述

控制平面（Control Plane）是 Kubernetes 集群的管理层，负责维护集群状态、处理 API 请求、执行调度和协调所有组件的工作。控制平面由一组核心组件构成，通常部署在专用节点上并采用高可用架构。

## 核心概念/原理

### 控制平面的核心组件

- **kube-apiserver**：集群的唯一入口，提供 RESTful API，所有操作（包括用户请求、内部组件通信）都通过 API Server。
- **etcd**：分布式键值存储，保存集群的完整状态数据，是集群的"大脑"。
- **kube-scheduler**：负责将未调度的 Pod 分配到最合适的节点上。
- **kube-controller-manager**：运行一组控制器（如 Deployment Controller、ReplicaSet Controller），维护集群的期望状态。
- **cloud-controller-manager**：将云厂商特定的控制逻辑（节点管理、负载均衡器、路由）从核心控制平面中解耦。

### 高可用架构

生产环境中，控制平面通常采用多副本部署：
- **etcd 集群**：至少 3 个成员，支持多数派写入的容错。
- **多 API Server 实例**：前端配置负载均衡器。
- **Controller Manager / Scheduler**：通过 Leader Election 机制实现主备切换。

## 关键机制或特性

- 控制平面组件通过 [[Leases|Lease]] 对象实现领导者选举。
- API Server 支持水平扩展，通过负载均衡器对外提供服务。
- etcd 的 compaction 和 defragmentation 需要定期执行以保证性能。
- 控制平面节点通常添加 `node-role.kubernetes.io/control-plane` 标签并设置污点以阻止普通工作负载调度。

## 使用场景与最佳实践

- 生产集群应至少部署 3 个控制平面节点以实现高可用。
- etcd 应与 API Server 分开部署或使用专用 SSD，避免 I/O 竞争。
- 定期备份 etcd 数据（使用 `etcdctl snapshot save`）。
- 使用 RBAC 严格控制对控制平面 API 的访问权限。
- 监控控制平面组件的健康状态和资源使用。

## 参考链接

- [Control Plane - Official Documentation](https://kubernetes.io/docs/concepts/architecture/)

## Related

[[17-系统基础/06-知识字典/fundamentals/kubernetes-components.md|Kubernetes 组件]] | [[17-系统基础/06-知识字典/fundamentals/etcd.md|etcd]]


<!-- risk-assessed -->
