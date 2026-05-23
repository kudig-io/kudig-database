---
title: k3s 轻量级 Kubernetes
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- runtime
- k3s
- etcd
- prometheus
- grafana
- cilium
- flannel
- coredns
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- k3s 轻量级 Kubernetes 是什么
- 如何 k3s 轻量级 Kubernetes
trigger_keywords:
- k3s
- 轻量级
- Kubernetes
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- mysql-basics
created: "2026-05-23"
---

# [[k3s|k3s]] 轻量级 Kubernetes

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Go

## 概述

k3s 是经过 CNCF 认证的轻量级 Kubernetes 发行版，专为资源受限环境设计。它将 Kubernetes 所需的所有组件打包到单个小于 100MB 的二进制文件中，非常适合 IoT、边缘计算、CI/CD 和开发环境。k3s 移除了遗留和可选组件，同时保持完全兼容标准 [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api.md|Kubernetes API]]。

## 核心能力

- **轻量级部署**: 单二进制文件，内存占用约 512MB
- **快速安装**: 30 秒内完成安装，开箱即用
- **内置组件**: 包含 containerd、Flannel、CoreDNS、Traefik
- **SQLite/etcd**: 默认 SQLite，支持 etcd、MySQL、PostgreSQL
- **ARM 支持**: 原生支持 ARM64 和 ARMv7
- **自动证书**: TLS 证书自动生成和轮换

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **生产环境**: 使用外部数据库 (MySQL/PostgreSQL/etcd) 替代 SQLite
- **高可用**: 部署至少 3 个 Server 节点
- **网络**: 根据场景选择 Flannel 后端 (vxlan/wireguard/host-gw)
- **安全**: 轮换 Node Token，限制 API Server 访问
- **备份**: 定期备份数据存储和证书
- **升级**: 使用自动升级控制器管理版本

## 架构定位

在 CNCF 生态中，k3s 属于 **Runtime** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[etcd]]
- [[entities/prometheus-grafana.md|prometheus-grafana]]
- [[containerd]]
- networking.md|cilium-ebpf-networking]]
- [[entities/cni-plugins.md|cni-plugins]]

## Related

- [[podman-container-tools]] — Podman Desktop
- [[containerd]] — containerd
- [[coredns]] — CoreDNS
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[domain-10-troubleshooting-diagnostics/44-kind-k3s-single-node-troubleshooting.md|44-kind-k3s-single-node-troubleshooting]]
- k3s
- [[references/multi-cloud-terms|K8s 多云架构术语参考]] — Cross-reference
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/node-index|Node 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
