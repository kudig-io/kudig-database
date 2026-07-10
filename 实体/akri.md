---
title: Akri (entities)
description: '## 概述'
summary: 'Akri 是一个 Kubernetes 资源接口项目，用于在边缘环境中自动发现和使用异构叶设备（Leaf Devices）。它将 IP 摄像头、USB 传感器、OPC UA 服务器等物理设备抽象为 Kubernetes 原生资源，使 Pod 能够像使用 PersistentVolume 一样使用这些边缘设备。'
category: entities
tags:
- k8s
- cncf
- edge
- akri
- crd
- operator
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Akri 是什么
- 如何 Akri
trigger_keywords:
- Akri
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Akri

> **CNCF 状态**: Sandbox | **类别**: Edge | **主要语言**: Rust

## 概述

Akri 是一个 Kubernetes 资源接口项目，用于在边缘环境中自动发现和使用异构叶设备（Leaf Devices）。它将 IP 摄像头、USB 传感器、OPC UA 服务器等物理设备抽象为 Kubernetes 原生资源，使 Pod 能够像使用 PersistentVolume 一样使用这些边缘设备。Akri 通过 Discovery Handler 插件机制持续发现网络中的设备变化...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **网络规划**: 确保 Akri Agent 节点能访问设备网络，配置正确的 IP 段和协议端口
- **资源限制**: 为 Broker Pod 设置合理的资源限制，避免视频流处理消耗过多节点资源
- **设备容量**: 根据设备处理能力设置 capacity，避免过多 Broker 同时访问同一设备
- **安全配置**: ONVIF 摄像头配置认证凭据，OPC UA 设备配置证书
- **高可用**: 在多节点部署 Agent，确保设备发现不因单节点问题中断

## 架构定位

在 CNCF 生态中，akri 属于 **Edge** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]
- [[概念/security-defense-depth.md|security-defense-depth]]

## Related

- [[podman-desktop]] — Podman Desktop
- [[openyurt]] — OpenYurt
- [[carina]] — Carina
- [[spire]] — SPIRE
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- akri
- [[实体/cncf-edge-ai.md|[[CNCF 边缘计算与 AI/ML 项目全景|CNCF 边缘计算与 AI/ML 项目全景]]]] — Cross-reference
- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
