---
title: Tinkerbell [entities]
description: '## 概述'
summary: 'Tinkerbell 是一个裸金属服务器自动化配置（provisioning）框架，用于在物理服务器上自动安装操作系统和执行配置任务。它替代传统的 PXE/Cobbler 方案，通过声明式的工作流定义和容器化的操作步骤实现裸金属服务器的云原生式管理。'
category: entities
tags:
- k8s
- cncf
- metal
- tinkerbell
- prometheus
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Tinkerbell 是什么
- 如何 Tinkerbell
trigger_keywords:
- Tinkerbell
prerequisites:
- kubectl-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Tinkerbell

> **CNCF 状态**: Sandbox | **类别**: Metal | **主要语言**: Go

## 概述

Tinkerbell 是一个裸金属服务器自动化配置（provisioning）框架，用于在物理服务器上自动安装操作系统和执行配置任务。它替代传统的 PXE/Cobbler 方案，通过声明式的工作流定义和容器化的操作步骤实现裸金属服务器的云原生式管理。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **网络规划**: 确保 DHCP/PXE 网络与生产网络适当隔离
- **镜像缓存**: 在本地缓存操作系统镜像加速安装
- **模板复用**: 创建标准化的安装模板，参数化可变部分
- **Action 容器**: 使用官方 Action 容器，需要时自定义扩展
- **硬件清单**: 维护准确的硬件清单（MAC 地址、磁盘信息）

## 架构定位

在 CNCF 生态中，tinkerbell 属于 **Metal** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/crd-custom-resources.md|crd-custom-resources]]

## Related

- [[headlamp]] — Headlamp
- [[entities/cncf-orchestration.md|cncf-orchestration]] — CNCF 编排与应用管理项目全景
- [[prometheus]] — Prometheus
- [[interlink]] — InterLink
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- tinkerbell
- [[entities/cncf-edge-ai.md|[[CNCF 边缘计算与 AI/ML 项目全景|CNCF 边缘计算与 AI/ML 项目全景]]]] — Cross-reference
- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
