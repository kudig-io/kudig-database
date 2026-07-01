---
title: kubeadm
description: 'kubeadm 是 Kubernetes 官方提供的集群初始化和升级工具。它简化了集群的引导过程，是快速搭建 Kubernetes 集群的推荐方式。...'
category: dictionary
tags:
- k8s
- glossary
- kubeadm
- tooling
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubeadm 是什么
- kubeadm 详解
trigger_keywords:
- kubeadm
- dictionary
prerequisites:
- kubectl-basics
created: "2026-06-24"
---

# kubeadm

> **英文名**: kubeadm

## 概述

kubeadm 是 Kubernetes 官方提供的集群初始化和升级工具。它简化了集群的引导过程，是快速搭建 Kubernetes 集群的推荐方式。

## 核心概念/原理

### 核心命令

| 命令 | 用途 |
|------|------|
| `kubeadm init` | 初始化控制平面节点 |
| `kubeadm join` | 将工作节点加入集群 |
| `kubeadm upgrade` | 升级集群版本 |
| `kubeadm reset` | 重置节点，移除 kubeadm 安装的组件 |
| `kubeadm token` | 管理加入令牌 |

### 初始化流程

```
kubeadm init → 生成证书 → 启动 etcd → 启动 API Server →
启动 Controller Manager → 启动 Scheduler → 安装 CNI → 就绪
```

## 关键机制或特性

- kubeadm 只负责引导集群，不负责 CNI 插件安装（需用户自行部署）。
- 支持配置文件（`kubeadm-config.yaml`）自定义集群参数。
- `kubeadm upgrade` 支持安全的集群版本升级。

## 使用场景与最佳实践

- 学习/开发环境使用 kubeadm 快速搭建集群。
- 生产环境使用 kubeadm 配合自动化工具（Ansible/Terraform）。
- 升级前使用 `kubeadm upgrade plan` 检查兼容性。
- 始终备份 etcd 数据后再执行升级。

## 参考链接

- [kubeadm - Official Documentation](https://kubernetes.io/docs/reference/setup-tools/kubeadm/)

## Related

- [[domain-17-system-foundation/topic-dictionary/tooling/kubectl.md|Kubectl]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kubectx.md|Kubectx]]
- [[domain-17-system-foundation/topic-dictionary/tooling/kubens.md|Kubens]]
- [[domain-17-system-foundation/topic-dictionary/tooling/k9s.md|K9S]]
- [[domain-17-system-foundation/topic-dictionary/tooling/stern.md|Stern]]
