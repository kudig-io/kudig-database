---
title: kpt (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- config
- kpt
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kpt 是什么
- 如何 kpt
trigger_keywords:
- kpt
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# kpt

> **CNCF 状态**: Sandbox | **类别**: Config | **主要语言**: Go

## 概述

kpt 是一个以 Git 为中心的 Kubernetes 配置包管理工具，由 Google 开发。它使用 Git 分发 Kubernetes 资源包（package），通过函数 (KRM Functions) 实现配置的声明式转换、验证和修改，并提供 GitOps 风格的资源管理能力。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **包复用**: 将通用配置封装为 kpt 包，通过 Git 共享
- **函数管道**: 使用 pipeline 串联 mutator 和 validator
- **版本锁定**: 使用 Git 标签锁定包版本
- **Live 管理**: 使用 `kpt live` 替代 `kubectl apply` 实现声明式管理
- **验证优先**: 在 pipeline 中添加 validator 在部署前检查配置

## 架构定位

在 CNCF 生态中，kpt 属于 **Config** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[deployment]]
- [[concepts/gitops-principles|gitops-principles]]

## Related

- [[contour]] — Contour
- [[kured]] — Kured (KUbernetes REboot Daemon)
- [[opengemini]] — openGemini
- [[kmesh]] — Kmesh
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kpt
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
