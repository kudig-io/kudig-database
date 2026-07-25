---
title: K3s
description: K3s 是 Rancher（现 SUSE）开发的轻量级 Kubernetes 发行版，现为 CNCF 沙箱项目。它将整个 K8s 控制平面打包为单个二进制文件（...
summary: K3s 是 Rancher（现 SUSE）开发的轻量级 Kubernetes 发行版，现为 CNCF 沙箱项目。它将整个 K8s 控制平面打包为单个二进制文件（...
category: dictionary
tags:
- k8s
- glossary
- k3s
- lightweight-k8s
- edge
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K3s 是什么
- K3s 详解
trigger_keywords:
- K3s
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K3s

> **英文名**: K3s

## 概述

K3s 是 Rancher（现 SUSE）开发的轻量级 Kubernetes 发行版，现为 CNCF 沙箱项目。它将整个 K8s 控制平面打包为单个二进制文件（<100MB），特别适合边缘计算、IoT 和资源受限环境。

## 核心概念/原理

### 与标准 K8s 对比

| 特性 | K3s | 标准 K8s |
|------|-----|----------|
| 安装 | 单命令（curl） | kubeadm 多步 |
| 二进制大小 | <100MB | ~1GB |
| 内存占用 | ~512MB | ~2GB+ |
| 默认 CNI | Flannel | 无 |
| 默认存储 | Local Path | 无 |
| 数据库 | SQLite/etcd | etcd |

### 内置组件

Flannel (CNI)、CoreDNS、Traefik (Ingress)、Local Path Provisioner、Klipper (Service LB)。

## 关键机制或特性

- **单二进制**：所有组件编译为单一二进制文件。
- **自动 TLS**：所有组件间通信自动启用 TLS。
- **Server/Agent 模式**：Server 运行控制平面，Agent 运行工作负载。
- **Helm Controller**：通过 CRD 声明式管理 Helm Chart。
- 支持 ARM64 架构（树莓派等）。

## 使用场景与最佳实践

- 边缘/IoT 场景使用 K3s 部署轻量级 K8s。
- 开发/测试环境快速搭建使用 K3s。
- CI/CD 流水线中使用 K3s 运行集成测试。
- 配合 Rancher 统一管理大规模 K3s 集群。
- 生产环境考虑替换 Flannel 为 Cilium 提升网络性能。

## 参考链接

- [K3s Official](https://k3s.io/)

## Related

- [[17-系统基础/06-知识字典/fundamentals/kubernetes.md|Kubernetes]]
- [[17-系统基础/06-知识字典/tooling/minikube.md|Minikube]]
- [[17-系统基础/06-知识字典/tooling/kubeadm.md|Kubeadm]]
- [[17-系统基础/06-知识字典/platform-engineering/rancher.md|Rancher]]
- [[17-系统基础/06-知识字典/platform-engineering/kubeedge.md|KubeEdge]]


<!-- risk-assessed -->
