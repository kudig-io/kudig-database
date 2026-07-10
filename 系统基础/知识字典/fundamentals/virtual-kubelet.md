---
title: Virtual Kubelet 虚拟节点
description: Virtual Kubelet 是 CNCF Sandbox 项目，通过 Kubelet 接口将外部服务（如 Serverless 平台/云
  API/VM）伪装...
summary: Virtual Kubelet 是 CNCF Sandbox 项目，通过 Kubelet 接口将外部服务（如 Serverless 平台/云 API/VM）伪装...
category: dictionary
tags:
- k8s
- glossary
- fundamentals
- node
- serverless
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Virtual Kubelet 虚拟节点 是什么
- Virtual Kubelet 详解
trigger_keywords:
- Virtual Kubelet 虚拟节点
- Virtual Kubelet
- dictionary
prerequisites:
- kubernetes
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Virtual Kubelet 虚拟节点（Virtual Kubelet）

## 概述

Virtual Kubelet 是 CNCF Sandbox 项目，通过 Kubelet 接口将外部服务（如 Serverless 平台/云 API/VM）伪装为 Kubernetes 节点，实现 Pod 调度到非 K8s 基础设施。

## 核心概念/原理

- **虚拟节点**：将外部资源伪装为 K8s 节点
- **CNCF Sandbox**：Microsoft/VMware 联合推动
- **透明调度**：Pod 可透明调度到 Serverless 平台
- **Provider 模式**：插件式 Provider 支持多种后端

## 关键机制或特性

- Provider 接口（实现 Node/Pod 生命周期管理）
- 内置 Provider：Azure ACI、AWS Fargate 等
- Taints 控制 Pod 调度到虚拟节点
- 与 K8s Service/Ingress 集成
- Metrics/Log 代理
- 自定义 Provider 开发

## 使用场景与最佳实践

- Serverless 容器的 K8s 调度
- 突发扩容到云服务（burst to cloud）
- 异构计算资源的统一管理
- 多集群的 Pod 调度
- 开发和测试环境的虚拟节点

## 参考链接

- https://virtual-kubelet.io/
- https://github.com/virtual-kubelet/virtual-kubelet

## Related

- [[系统基础/知识字典/scheduling/cluster-autoscaler.md|Cluster Autoscaler]]
- [[系统基础/知识字典/specialized-workloads/knative.md|Knative]]
- [[系统基础/知识字典/workloads/pod.md|Pod]]


<!-- risk-assessed -->
