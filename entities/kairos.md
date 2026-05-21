---
title: Kairos
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- edge
- kairos
- prometheus
- grafana
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kairos 是什么
- 如何 Kairos
trigger_keywords:
- Kairos
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

# Kairos

> **CNCF 状态**: Sandbox | **类别**: Edge | **主要语言**: Go

## 概述

Kairos 是一个不可变 Linux 元发行版框架，专注于将任何 Linux 发行版转化为不可变的、基于容器镜像的操作系统，特别适用于边缘计算和 Kubernetes 节点的自动化部署。它支持通过 cloud-init 风格的 YAML 配置实现零接触安装（Zero-Touch Provisioning），内置 P2P 网络自动组建 Kubernetes 集群的能力。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **镜像精简**: 自定义 Kairos 镜像时只安装必要的包，减小攻击面
- **P2P 令牌安全**: P2P 网络令牌需要安全存储和分发
- **升级策略**: 使用蓝绿升级策略，先升级部分节点验证后再全量升级
- **配置管理**: 将 cloud-config 纳入版本控制，确保配置可追溯
- **离线部署**: 边缘场景预先下载 K3s 二进制和镜像到 OCI 镜像中

## 架构定位

在 CNCF 生态中，kairos 属于 **Edge** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[entities/prometheus-grafana.md|prometheus-grafana]]

## Related

- [[kcl]] — KCL (Kusion Configuration Language)
- [[kube-vip]] — kube-vip
- [[kitops]] — KitOps
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[k3s]] — k3s 轻量级 Kubernetes

- [[domain-19-landscape-references/sandbox/kairos/kairos.md|kairos]]
- [[entities/interlink.md|InterLink]]
- [[entities/akri.md|Akri]]
- [[entities/openyurt.md|OpenYurt]]
- [[entities/cncf-runtime|CNCF 容器运行时与工具链项目全景]] — Cross-reference
