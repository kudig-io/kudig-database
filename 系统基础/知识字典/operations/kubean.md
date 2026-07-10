---
title: Kubean 集群部署
description: Kubean 是 DaoCloud 开源的 CNCF Sandbox 项目，基于 Kubespray 提供 Kubernetes 集群的声明式部署和生命周期管理...
summary: Kubean 是 DaoCloud 开源的 CNCF Sandbox 项目，基于 Kubespray 提供 Kubernetes 集群的声明式部署和生命周期管理...
category: dictionary
tags:
- k8s
- glossary
- operations
- deployment
- cluster
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubean 集群部署 是什么
- Kubean 详解
trigger_keywords:
- Kubean 集群部署
- Kubean
- dictionary
prerequisites:
- kubernetes
---



# Kubean 集群部署（Kubean）

## 概述

Kubean 是 DaoCloud 开源的 CNCF Sandbox 项目，基于 Kubespray 提供 Kubernetes 集群的声明式部署和生命周期管理，通过 Operator 模式实现集群的自动化安装和运维。

## 核心概念/原理

- **Kubespray 封装**：将 Kubespray 封装为 K8s Operator
- **声明式管理**：通过 CRD 定义集群规格
- **CNCF Sandbox**：DaoCloud 主导
- **多环境**：支持物理机/VM/云环境部署

## 关键机制或特性

- Cluster / Operation CRD 定义集群和运维操作
- 支持离线安装（Air-gapped）
- 多 CNI 支持（Calico/Cilium/Flannel/Macvlan 等）
- 集群升级和证书轮转
- 节点扩缩容
- 多 OS 支持（CentOS/Ubuntu/Debian/AlmaLinux）

## 使用场景与最佳实践

- 生产级 K8s 集群的自动化部署
- 离线环境的集群安装
- 集群版本升级和运维
- 多集群的统一部署管理
- Kubespray 的 Operator 化使用

## 参考链接

- https://kubean.io/
- https://github.com/kubean-io/kubean

## Related

- [[系统基础/知识字典/tooling/kubeadm.md|kubeadm]]
- [[系统基础/知识字典/tooling/k3s.md|K3s]]
- [[系统基础/知识字典/platform-engineering/rancher.md|Rancher]]
