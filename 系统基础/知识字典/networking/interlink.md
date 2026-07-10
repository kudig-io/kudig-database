---
title: InterLink HPC 互联
description: InterLink 是 INFN（意大利国家核物理研究所）开源的 CNCF Sandbox 项目，基于 Virtual Kubelet 将
  HPC（高性能计算）...
summary: InterLink 是 INFN（意大利国家核物理研究所）开源的 CNCF Sandbox 项目，基于 Virtual Kubelet 将 HPC（高性能计算）...
category: dictionary
tags:
- k8s
- glossary
- networking
- hpc
- virtual-kubelet
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- InterLink HPC 互联 是什么
- InterLink 详解
trigger_keywords:
- InterLink HPC 互联
- InterLink
- dictionary
prerequisites:
- kubernetes
---



# InterLink HPC 互联（InterLink）

## 概述

InterLink 是 INFN（意大利国家核物理研究所）开源的 CNCF Sandbox 项目，基于 Virtual Kubelet 将 HPC（高性能计算）资源接入 Kubernetes，实现 K8s 工作负载在 HPC 集群上运行。

## 核心概念/原理

- **HPC 集成**：将 HPC 集群（Slurm/HTCondor）接入 K8s
- **Virtual Kubelet**：基于 VK Provider 模式实现
- **CNCF Sandbox**：INFN 主导
- **科学计算**：为科学研究提供 HPC 资源

## 关键机制或特性

- Virtual Kubelet Provider for HPC
- 支持 Slurm/HTCondor/Kubernetes 后端
- Pod 到 HPC Job 的转换
- 数据管理（输入/输出文件传输）
- GPU/大内存节点的调度
- Sidecar 容器支持
- HPC 资源配额管理

## 使用场景与最佳实践

- AI/ML 训练的 HPC 资源利用
- 科学计算工作负载的 K8s 管理
- 混合云+HPC 的资源调度
- 大规模模拟任务的资源弹性
- 科研机构的计算资源统一管理

## 参考链接

- https://interlink-expect.github.io/
- https://github.com/intertwin-eu/interLink

## Related

- [[系统基础/知识字典/fundamentals/virtual-kubelet.md|Virtual Kubelet]]
- [[系统基础/知识字典/scheduling/volcano.md|Volcano]]
- [[系统基础/知识字典/scheduling/hami.md|HAMi]]
