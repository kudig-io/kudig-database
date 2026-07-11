---
title: Hexa
description: '## 概述'
summary: 'Hexa 是一个统一的策略编排引擎，使用 IDQL (Identity Query Language) 作为通用策略语言，实现跨多个云平台和授权系统的访问控制策略管理。它支持将策略从一个授权系统（如 AWS IAM、Azure RBAC、Google IAP）翻译和同步到另一个系统，避免了在不同平台上重复维护相似策略的问题。'
category: entities
tags:
- k8s
- cncf
- security
- hexa
- istio
- opa
- rbac
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Hexa 是什么
- 如何 Hexa
trigger_keywords:
- Hexa
prerequisites:
- kubectl-basics
- service-mesh-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Hexa

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

Hexa 是一个 CNCF 沙箱项目，旨在提供 Kubernetes 多集群应用编排和策略管理能力。它专注于解决多集群环境下的应用一致性部署、配置同步和生命周期管理问题。Hexa 通过统一的控制平面管理跨集群的应用部署状态，支持基于策略的工作负载分发和差异化配置。项目特别关注多租户和多团队场景下的集群资源管理。

## Key Features（核心能力）

- **多集群应用编排**：将应用部署到多个 K8s 集群并保持一致性
- **策略驱动分发**：基于标签、容量、地域的策略控制部署目标
- **配置同步**：跨集群的 ConfigMap、Secret 等配置资源同步
- **健康监控**：监控多集群应用部署健康状态
- **差异化配置**：支持不同集群的定制化配置覆盖
- **GitOps 兼容**：与 Git 仓库集成的声明式管理

## 架构与工作原理

Hexa 采用 Hub-Spoke 控制平面架构：Hub Controller 运行在管理集群，监听应用部署 CRD 并根据分发策略将工作负载推送到目标集群；Spoke Agent 运行在目标集群，接收部署指令并协调本地 K8s 资源。分发策略通过 Policy CRD 定义，支持基于集群标签、资源容量和地理位置的智能分发。

## K8s 集成

Hexa 通过 CRD 与 Kubernetes 集成：ApplicationDistribution CRD 定义应用的多集群部署配置；ClusterPolicy CRD 定义分发策略。Hub Controller 管理这些 CRD 的生命周期，通过 K8s API 连接到各目标集群，推送工作负载配置。Spoke Agent 在目标集群中执行实际的资源创建和状态上报。

## 生产用例

- **多集群应用部署**：将应用一致性部署到生产、DR、边缘集群
- **多租户资源管理**：为不同团队分配集群资源并管理部署
- **灾难恢复**：跨集群的应用快速切换和恢复
- **地理分布部署**：按地域策略将应用部署到最近的集群

## 安装与快速开始

```bash
kubectl apply -f https://github.com/hexa-org/hexa/releases/latest/download/hexa-operator.yaml
```

## 对比替代方案

相比 Karmada（CNCF 孵化），Hexa 更轻量但功能集更小。相比 KubeFed v2（已归档），Hexa 设计更现代化且维护活跃。

## Related

- [[03-istio-security-hardening]] — Istio 安全加固
- [[copa]] — Copa (Copacetic)
- [[nats]] — NATS
- [[paralus]] — Paralus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- hexa
- [[实体/cncf-infrastructure.md|CNCF 基础设施与混沌工程项目全景]] — Cross-reference


<!-- risk-assessed -->
