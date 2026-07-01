---
title: KubeVirt 虚拟化
description: 'KubeVirt 是 Red Hat 开源的 CNCF 孵化项目，在 Kubernetes 上提供虚拟机管理能力，通过 CRD 定义和运行 VM，实现容器和虚拟...'
category: dictionary
tags:
- k8s
- glossary
- specialized-workloads
- virtualization
- cncf
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KubeVirt 虚拟化 是什么
- KubeVirt 详解
trigger_keywords:
- KubeVirt 虚拟化
- KubeVirt
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# KubeVirt 虚拟化（KubeVirt）

## 概述

KubeVirt 是 Red Hat 开源的 CNCF 孵化项目，在 Kubernetes 上提供虚拟机管理能力，通过 CRD 定义和运行 VM，实现容器和虚拟机工作负载在同一集群中的统一管理。

## 核心概念/原理

- **K8s 上的 VM**：通过 VirtualMachine CRD 管理虚拟机
- **容器和 VM 统一**：VM 和 Pod 共享同一集群的调度和网络
- **CNCF 孵化**：Red Hat 主导，OpenShift Virtualization 的上游
- **成熟生态**：与 CDI、Kubevirt-CSI 等组件配合

## 关键机制或特性

- VirtualMachine / VirtualMachineInstance CRD
- CDI（Containerized Data Importer）镜像管理
- DataVolume 声明 VM 磁盘
- 热迁移（Live Migration）
- 模板和实例类型（InstanceType）
- GPU/SRIOV 直通
- 与 Multus CNI 配合的多网络 VM

## 使用场景与最佳实践

- 传统 VM 工作负载迁移到 K8s
- 容器和 VM 混合工作负载管理
- 数据库等不适合容器化的 VM 工作负载
- VDI（虚拟桌面）在 K8s 上的部署
- 云 VM 与容器应用的统一编排

## 参考链接

- https://kubevirt.io/
- https://github.com/kubevirt/kubevirt

## Related

- [[domain-17-system-foundation/topic-dictionary/workloads/pod.md|Pod]]
- [[domain-17-system-foundation/topic-dictionary/networking/cni.md|CNI]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/kata-containers.md|Kata Containers]]
