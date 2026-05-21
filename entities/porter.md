---
title: Porter
description: 'summary: "Porter 是一个 CNAB (Cloud Native Application Bundle) 包管理器，用于将复杂的云原生应用及其所有依赖（Helm Charts、Terraform 模块、Kubernetes
  manifests、脚本等）打包为可分发、可安装的 Bundle。它解决了"我的应用需要先部署数据库，再配置网络，最后部署应用"这类多步骤安装流程的自动化问题，支'
category: general
tags:
- k8s
- helm
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Porter 是什么
- 如何 Porter
trigger_keywords:
- Porter
prerequisites:
- kubectl-basics
- helm-basics
- iac-basics
---

---
title: "Porter"
category: entities
summary: "Porter 是一个 CNAB (Cloud Native Application Bundle) 包管理器，用于将复杂的云原生应用及其所有依赖（Helm Charts、Terraform 模块、Kubernetes manifests、脚本等）打包为可分发、可安装的 Bundle。它解决了"我的应用需要先部署数据库，再配置网络，最后部署应用"这类多步骤安装流程的自动化问题，支持安装、升级、..."
tags: [k8s, cncf, config, porter]
sources: ["docs/domain-19-landscape-references/sandbox/porter/porter.md", "domain-19-landscape-references/sandbox/porter/porter.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: draft
lifecycle_changed: "2026-05-21"
tier: reference
base_confidence: 0.7
---

# Porter

> **CNCF 状态**: Sandbox | **类别**: Config | **主要语言**: Go

## 概述

Porter 是一个 CNAB (Cloud Native Application Bundle) 包管理器，用于将复杂的云原生应用及其所有依赖（Helm Charts、Terraform 模块、Kubernetes manifests、脚本等）打包为可分发、可安装的 Bundle。它解决了"我的应用需要先部署数据库，再配置网络，最后部署应用"这类多步骤安装流程的自动化问题，支持安装、升级、...

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **Mixin 选择**: 优先使用官方 Mixin，自定义逻辑用 exec Mixin
- **凭证管理**: 使用 Porter 的凭证集管理敏感信息，不要硬编码在 Bundle 中
- **参数校验**: 使用 enum 和类型约束确保参数值合法
- **幂等性**: 确保 install 和 upgrade 步骤是幂等的
- **版本化**: 为每个 Bundle 版本打标签，确保可复现的部署

## 架构定位

在 CNCF 生态中，porter 属于 **Config** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]

## Related

- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
