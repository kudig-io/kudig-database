---
title: Meshery (entities)
description: '## 概述'
summary: 'Meshery 是云原生管理平面，提供服务网格和云原生基础设施的生命周期管理。它支持多种服务网格 (Istio, Linkerd, Consul, Kuma, NSM 等) 的安装、配置、性能测试和运维管理，并提供统一的 Web 界面和 CLI。Meshery 还定义了 MeshModel 标准，用于描述云原生基础设施。'
category: entities
tags:
- k8s
- cncf
- networking
- meshery
- istio
- cilium
- crd
- operator
- kserve
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Meshery 是什么
- 如何 Meshery
trigger_keywords:
- Meshery
prerequisites:
- kubectl-basics
- service-mesh-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Meshery

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go, JavaScript

## 概述

Meshery 是由 Layer5 开源的云原生管理平面，2021 年加入 CNCF Sandbox。它提供服务网格和云原生基础设施的生命周期管理能力，支持 Istio、Linkerd、Consul、Kuma、Cilium、NSM 等 10+ 种服务网格的安装、配置、性能测试和运维管理。Meshery 还定义了 MeshModel 标准，用于描述和管理云原生基础设施组件，并提供统一的 Web 界面和 CLI 工具。

## 核心特性

- **多网格管理**: 统一界面管理 10+ 种服务网格的安装和配置
- **性能基准测试**: 内置负载测试和网格间性能对比工具
- **MeshModel**: 云原生基础设施建模标准，描述组件及其关系
- **设计模式**: 预定义的云原生部署模式（Pattern），可复用和分享
- **生命周期管理**: 安装、升级、卸载、配置变更
- **多集群视图**: 跨集群统一管理和可视化网格拓扑

## 架构

Meshery 采用前后端分离的 Server 架构。Meshery Server（Go 实现）是核心，提供 RESTful API 和 gRPC 接口。每个支持的云原生基础设施通过 Adapter 模式集成（如 meshery-istio、meshery-linkerd），Adapter 负责将 Meshery 操作翻译为目标系统的 API 调用。前端使用 React 实现 Web UI。数据层使用 SQLite/PostgreSQL 存储配置和测试结果。性能测试引擎基于 wrk2，支持可配置的负载模式和指标采集。

## Kubernetes 集成

Meshery 通过 kubeconfig 或 ServiceAccount 连接 Kubernetes 集群，使用标准 Kubernetes API 管理网格组件。每个网格 Adapter 通过 Helm Chart 或 Operator 部署目标网格到集群。MeshModel 将 Kubernetes CRD 和资源映射为标准化的组件模型。支持通过 GitOps 方式（与 ArgoCD/FluxCD 集成）管理网格配置变更。

## 生产使用场景

1. **网格选型评估**: 对比测试 Istio vs Linkerd 的性能和功能
2. **统一运维**: 一个界面管理多集群、多网格的基础设施
3. **性能回归**: 定期运行性能测试，检测网格升级后的性能退化
4. **架构可视化**: 通过 MeshModel 可视化整个云原生架构的组件关系

## 安装

```bash
# Docker 快速启动
docker run -d --name meshery -l meshery \
  -v meshery_config:/home/meshery/.meshery/config \
  -p 9081:9081 -p 10080:10080 layer5/meshery:stable
# Helm 部署到 Kubernetes
helm repo add meshery https://meshery.io/charts
helm install meshery meshery/meshery -n meshery --create-namespace
# 访问 http://localhost:9081
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Meshery** | 多网格统一管理、MeshModel | 学习曲线较陡 |
| Istio Dashboard | Istio 官方、轻量 | 仅限 Istio |
| Kiali | 服务网格可视化优秀 | 仅限 Istio，无多网格能力 |
| Cilium Hubble | eBPF 原生可观测性 | 仅限 Cilium |

## 架构定位

在 CNCF 生态中，Meshery 属于 **Platform / Management Plane** 类别，是云原生基础设施管理的统一入口。它通过 MeshModel 标准化了多网格和多基础设施的管理方式。

## 参考链接

- [[istio]]
- [[cilium]]
- [[deployment]]
- [[概念/service-mesh-architecture.md|service-mesh-architecture]]

## Related

- [[kserve]] — KServe
- [[istio]] — Istio
- [[kuma]] — Kuma
- [[linkerd]] — Linkerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- meshery
- [[实体/cncf-networking.md|[[CNCF 网络与服务网格项目全景|CNCF 网络与服务网格项目全景]]]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
