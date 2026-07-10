---
title: Tinkerbell 裸金属部署
description: Tinkerbell 是 Equinix Metal 开源的 CNCF Sandbox 项目，提供裸金属服务器的声明式操作系统部署和生命周期管理，是
  PXE/K...
summary: Tinkerbell 是 Equinix Metal 开源的 CNCF Sandbox 项目，提供裸金属服务器的声明式操作系统部署和生命周期管理，是
  PXE/K...
category: dictionary
tags:
- k8s
- glossary
- operations
- provisioning
- bare-metal
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Tinkerbell 裸金属部署 是什么
- Tinkerbell 详解
trigger_keywords:
- Tinkerbell 裸金属部署
- Tinkerbell
- dictionary
prerequisites:
- kubernetes
---



# Tinkerbell 裸金属部署（Tinkerbell）

## 概述

Tinkerbell 是 Equinix Metal 开源的 CNCF Sandbox 项目，提供裸金属服务器的声明式操作系统部署和生命周期管理，是 PXE/Kickstart 的现代化替代方案。

## 核心概念/原理

- **裸金属部署**：自动化裸金属服务器的 OS 安装
- **声明式**：CRD 定义硬件配置和安装工作流
- **CNCF Sandbox**：Equinix Metal 主导
- **容器化操作**：使用容器镜像执行安装步骤

## 关键机制或特性

- Hardware CRD 定义硬件资源
- Template CRD 定义安装工作流
- Workflow CRD 执行状态
- Action 容器镜像（安装步骤）
- Hook（OS 安装镜像）
- Tink Server/Worker 架构
- iPXE 引导

## 使用场景与最佳实践

- 裸金属服务器的自动化部署
- 数据中心的服务器生命周期管理
- 边缘节点的 OS 安装
- 裸金属 K8s 节点的自动化部署
- PXE/Kickstart 的现代化替代

## 参考链接

- https://tinkerbell.org/
- https://github.com/tinkerbell/tink

## Related

- [[系统基础/topic-dictionary/tooling/kubeadm.md|kubeadm]]
- [[系统基础/topic-dictionary/operations/kubean.md|Kubean]]
- [[系统基础/topic-dictionary/fundamentals/flatcar.md|Flatcar]]
