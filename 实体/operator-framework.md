---
title: Operator Framework [entities]
description: '## 概述'
summary: 'Operator Framework 是一个开源工具包，用于以高效、自动化和可扩展的方式管理 Kubernetes 原生应用（Operators）。它提供了构建、测试和分发 Operators 的完整解决方案。'
category: entities
tags:
- k8s
- cncf
- orchestration
- operator-framework
- prometheus
- grafana
- helm
- rbac
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
- Operator Framework 是什么
- 如何 Operator Framework
trigger_keywords:
- Operator
- Framework
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Operator Framework

> **CNCF 状态**: Incubating | **类别**: Orchestration | **主要语言**: Go

## 概述

Operator Framework 是由 Red Hat 开源的工具包，用于构建、测试和分发 Kubernetes 原生应用（Operators），2020 年加入 CNCF Incubating。它提供了完整的 Operator 生命周期管理解决方案，包括 Operator SDK（开发框架）、Operator Lifecycle Manager（OLM，运行时管理）和 OperatorHub.io（发现和分发平台）。Operator Framework 是 Kubernetes 生态中 Operator 模式事实上的标准工具链。

## 核心特性

- **Operator SDK**: 支持 Go、Ansible、Helm 三种方式构建 Operators
- **OLM (Operator Lifecycle Manager)**: 安装、升级、依赖管理和 RBAC 自动化
- **OperatorHub.io**: 类似 App Store 的 Operator 发现和安装平台
- **成熟度模型**: 5 级 Operator 能力模型（Basic Install → Auto Pilot）
- **内置测试**: scorecard 工具评估 Operator 质量
- **Catalog 管理**: 自定义 Operator Catalog 适配企业内部环境

## 架构

Operator Framework 由三个核心组件组成。Operator SDK 是 CLI 工具，提供项目脚手架、API 代码生成、测试框架和打包功能。OLM 在集群中以 Deployment 运行（olm-operator 和 catalog-operator），监听 Subscription 和 ClusterServiceVersion（CSV）CRD，管理 Operator 的安装、升级、依赖解析和 RBAC。OperatorHub.io 是外部 Web 平台，收录社区提交的 Operators。OLM Catalog 以 OCI 镜像或 CatalogSource 形式分发 Operator 列表和元数据。

## Kubernetes 集成

Operator Framework 完全基于 Kubernetes CRD 和 Controller 模式。OLM 通过 ClusterServiceVersion（CSV）描述 Operator 的元数据、安装模式和依赖关系。Subscription CRD 定义用户对某个 Operator 的订阅（频道、更新策略）。InstallPlan 由 OLM 自动生成，列出安装所需的所有资源。OLM 自动管理 RBAC（创建 Role/RoleBinding），确保 Operator 仅获得必要权限。Operator 通过 OLM 安装后，其 CRD 和 Deployment 自动创建。

## 生产使用场景

1. **数据库管理**: 安装 PostgreSQL/Redis Operator，自动化数据库运维
2. **中间件部署**: 通过 OLM 一键安装 Kafka/Elasticsearch Operator
3. **企业内部 Operator**: 构建内部 Operator Catalog，分发公司专有 Operators
4. **自动升级**: 订阅 Operator 更新频道，自动获取安全补丁和新版本

## 安装

```bash
# 安装 OLM
operator-sdk olm install
# 安装 Operator
kubectl operator install postgresql --channel stable-v1 --version 1.2.0
# 使用 SDK 创建新 Operator
operator-sdk init --domain example.com --repo github.com/myorg/my-operator
operator-sdk create api --group app --version v1 --kind MyApp --resource --controller
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Operator Framework** | CNCF Incubating、Red Hat 支持 | OLM 较重 |
| Kubebuilder | 轻量、官方 SIG 维护 | 无生命周期管理 |
| Metacontroller | 无需编写 Controller | 功能受限 |
| kubebuilder + Helm | 简单直接 | 无 OLM 级别的管理能力 |

## 架构定位

在 CNCF 生态中，Operator Framework 属于 **Orchestration** 类别，是 Operator 模式全生命周期管理的标准方案。OLM 是 OpenShift 的核心组件之一。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/controller-pattern.md|controller-pattern]]

## Related

- [[kubeclipper]] — KubeClipper
- [[runme-notebooks]] — Runme
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[helm]] — Helm

- operator-framework
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
