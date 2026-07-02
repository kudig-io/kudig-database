---
title: cdk8s (Cloud Development Kit for Kubernetes)
description: '## 概述'
summary: 'cdk8s (Cloud Development Kit for Kubernetes) 是一个开源软件开发框架，允许使用熟悉的编程语言定义 Kubernetes 应用和可重用抽象。它生成标准的 Kubernetes YAML 清单，可与任何 Kubernetes 集群配合使用。cdk8s 借鉴了 AWS CDK 的理念，'
category: entities
tags:
- k8s
- cncf
- config
- cdk8s
- helm
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
- cdk8s (Cloud Development Kit for Kubernetes) 是什么
- 如何 cdk8s (Cloud Development Kit for Kubernetes)
trigger_keywords:
- cdk8s
- Cloud
- Development
- Kit
- for
- Kubernetes
prerequisites:
- kubectl-basics
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[cdk8s|cdk8s]] (Cloud Development Kit for Kubernetes)

> **CNCF 状态**: Sandbox | **类别**: Config | **主要语言**: TypeScript, Python, Go, Java

## 概述

cdk8s (Cloud Development Kit for Kubernetes) 是一个开源软件开发框架，允许使用熟悉的编程语言定义 Kubernetes 应用和可重用抽象。它生成标准的 Kubernetes YAML 清单，可与任何 Kubernetes 集群配合使用。cdk8s 借鉴了 AWS CDK 的理念，将基础设施即代码提升到使用真正编程语言的高度。

## 核心能力

- **多语言支持**: TypeScript、Python、Go、Java
- **类型安全**: 编译时类型检查和 IDE 支持
- **可复用组件**: Constructs 抽象层实现代码复用
- **导入 CRD**: 自动从 CRD 生成类型化 API
- **Helm 支持**: 将 Helm Chart 作为 Construct 使用
- **测试友好**: 支持单元测试和快照测试

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **模块化**: 将复杂逻辑封装为 Construct
- **类型安全**: 充分利用 TypeScript 类型检查
- **测试覆盖**: 使用快照测试和单元测试
- **版本管理**: 锁定 cdk8s 和 K8s API 版本
- **复用组件**: 发布 Construct 库供团队使用
- **CI/CD 集成**: 在管道中运行 synth 和测试

## 架构定位

在 CNCF 生态中，cdk8s 属于 **Config** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[entities/crd-custom-resources.md|crd-custom-resources]]
- [[concepts/controller-pattern.md|controller-pattern]]
- [[concepts/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[kuasar]] — Kuasar
- [[longhorn]] — Longhorn
- [[open-cluster-management]] — [[entities/open-cluster-management.md|Open Cluster Management (OCM)]]
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[helm]] — Helm

- cdk8s
- [[entities/kpt.md|kpt]]
- [[entities/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
