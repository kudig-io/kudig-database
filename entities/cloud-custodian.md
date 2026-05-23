---
title: Cloud Custodian [entities]
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- policy
- cloud-custodian
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cloud Custodian 是什么
- 如何 Cloud Custodian
trigger_keywords:
- Cloud
- Custodian
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Cloud Custodian

> **CNCF 状态**: Incubating | **类别**: Policy | **主要语言**: Python

## 概述

Cloud Custodian 是云资源治理和管理的规则引擎，通过 YAML 策略实现云资源的合规性、成本优化和安全管理。它支持 AWS、Azure、GCP 等主流云平台和 Kubernetes。

## 核心能力

- **声明式策略**: YAML 定义资源筛选和操作规则
- **多云支持**: AWS、Azure、GCP、Kubernetes
- **实时监控**: 事件驱动的实时策略执行
- **成本优化**: 识别闲置资源、调整规格
- **安全合规**: 检测配置违规、自动修复
- **丰富过滤器**: 200+ 资源类型和过滤条件

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **先试运行**: 始终使用 `--dryrun` 验证策略
- **渐进执行**: 先标记 (mark-for-op)，后执行操作
- **细粒度权限**: 为 Custodian 创建最小权限 IAM 角色
- **版本控制**: 策略文件纳入 Git 管理
- **监控告警**: 配置策略执行结果通知

## 架构定位

在 CNCF 生态中，cloud-custodian 属于 **Policy** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[pod-lifecycle]]
- [[concepts/security-defense-depth.md|security-defense-depth]]

## Related

- [[kuasar]] — Kuasar
- [[longhorn]] — Longhorn
- [[open-cluster-management]] — [[entities/open-cluster-management.md|Open Cluster Management (OCM)]]
- [[cdk8s]] — cdk8s (Cloud Development Kit for Kubernetes)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- cloud-custodian
- [[entities/capsule.md|Capsule]]
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
