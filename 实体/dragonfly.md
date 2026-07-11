---
title: Dragonfly (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- dragonfly
- scheduler
- prometheus
- grafana
- containerd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Dragonfly 是什么
- 如何 Dragonfly
trigger_keywords:
- Dragonfly
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Dragonfly

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Go

## 概述

Dragonfly（蜻蜓）是一个 CNCF 孵化项目，由阿里巴巴开源，是一个基于 P2P 技术的智能文件分发系统。它旨在解决大规模容器集群中镜像分发和文件下载的带宽瓶颈问题。在数千节点的集群中，传统的镜像拉取方式会导致 Registry 带宽被打满，Dragonfly 通过 P2P 协议让节点间互相分享数据，将带宽消耗从集中式变为分布式，显著提升大规模部署的效率。

## Key Features（核心能力）

- **P2P 文件分发**：通过 P2P 协议将文件分发负载分散到所有节点
- **镜像预热**：支持在部署前预热镜像到所有节点，加速 Pod 启动
- **多源支持**：支持从 Registry、HTTP、NAS 等多种数据源分发文件
- **智能限速**：支持基于主机级别的速率限制，避免影响业务流量
- **主机级缓存**：通过本地缓存避免重复下载相同文件
- **安全传输**：支持 TLS 加密和镜像签名验证

## 架构与工作原理

Dragonfly v2 架构包含三个核心组件：Scheduler（调度器）负责 P2P 网络的节点管理和调度决策；Seed Peer（种子节点）作为 P2P 网络中的数据源，从 Registry 拉取数据并分发给其他 Peer；Dfdaemon（守护进程）作为 DaemonSet 运行在每个节点，拦截镜像拉取请求并利用 P2P 网络加速下载。通过 Manager 组件提供统一的管理控制台。

## K8s 集成

Dragonfly 通过 Dfget 代理拦截 containerd/docker 的镜像拉取请求。在 K8s 中以 DaemonSet 方式部署 dfdaemon，配置 containerd 使用 Dragonfly 作为镜像代理。Dragonfly 支持 K8s 原生的 Pod 安全策略和 RBAC。Manager 组件通过 Deployment 部署，提供 Web UI 和 API 管理界面。

## 生产用例

- **大规模镜像分发**：数千节点集群的镜像拉取加速，避免 Registry 带宽瓶颈
- **边缘计算节点更新**：在带宽受限的边缘场景高效分发镜像
- **CI/CD 并发部署**：大规模并行构建和部署的镜像拉取加速
- **软件包分发**：大规模集群的软件包和配置文件分发

## 安装与快速开始

```bash
helm repo add dragonfly https://dragonflyoss.github.io/helm-charts
helm install dragonfly dragonfly/dragonfly -n dragonfly-system --create-namespace
```

## 对比替代方案

相比 Kraken（Uber 开源的 P2P 分发系统），Dragonfly 更活跃且 CNCF 社区支持更好。相比直接从 Registry 拉取，Dragonfly 在大规模集群中可将分发时间缩短数倍。

## Related

- [[serverless-workflow]] — Serverless Workflow
- [[cloudnativepg]] — CloudNativePG
- [[strimzi]] — Strimzi
- [[hwameistor]] — HwameiStor
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- dragonfly
- [[实体/cncf-storage.md|CNCF 存储与数据库项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
