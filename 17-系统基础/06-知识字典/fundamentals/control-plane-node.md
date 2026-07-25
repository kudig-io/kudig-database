---
title: 控制平面节点
description: Control Plane Node（控制平面节点）是运行 Kubernetes 控制平面组件（kube-apiserver、kube-scheduler、ku...
summary: Control Plane Node（控制平面节点）是运行 Kubernetes 控制平面组件（kube-apiserver、kube-scheduler、ku...
category: dictionary
tags:
- k8s
- glossary
- control-plane
- node
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 控制平面节点 是什么
- Control Plane Node 详解
trigger_keywords:
- 控制平面节点
- Control Plane Node
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 控制平面节点

> **英文名**: Control Plane Node

## 概述

Control Plane Node（控制平面节点）是运行 Kubernetes 控制平面组件（kube-apiserver、kube-scheduler、kube-controller-manager、etcd）的专用节点。在高可用集群中通常部署 3 个或 5 个控制平面节点。

## 核心概念/原理

### 与 Master Node 的关系

Control Plane Node 是 Master Node 的现代称谓，强调该节点运行控制平面组件而非「主从」关系。

### 控制平面组件

| 组件 | 职责 |
|------|------|
| kube-apiserver | API 入口，REST 请求处理 |
| etcd | 集群状态存储 |
| kube-scheduler | Pod 调度决策 |
| kube-controller-manager | 控制器循环运行 |
| cloud-controller-manager | 云厂商 API 交互 |

## 关键机制或特性

- 控制平面节点通常标记 `node-role.kubernetes.io/control-plane` 污点，默认不接受用户 Pod。
- 高可用部署使用 kubeadm 的 `--control-plane-endpoint` 配置负载均衡。
- etcd 可以堆叠（stacked）在控制平面节点上，也可以外部独立部署。

## 使用场景与最佳实践

- 生产集群至少部署 3 个控制平面节点实现高可用。
- 控制平面节点应有独立的计算资源，不与工作负载混用。
- 使用 `kubeadm init --upload-certs` 加入额外控制平面节点。
- 定期检查 etcd 集群健康状态和证书过期时间。

## 参考链接

- [Control Plane Node - Kubernetes Docs](https://kubernetes.io/docs/concepts/architecture/nodes/#control-plane-node)

## Related

- [[17-系统基础/06-知识字典/fundamentals/master-node.md|Master Node]]
- [[17-系统基础/06-知识字典/fundamentals/control-plane.md|Control Plane]]
- [[17-系统基础/06-知识字典/fundamentals/worker-node.md|Worker Node]]
- [[17-系统基础/06-知识字典/fundamentals/node.md|Node]]
- [[17-系统基础/06-知识字典/fundamentals/etcd.md|Etcd]]


<!-- risk-assessed -->
