---
title: Konveyor (entities)
description: '## 概述'
category: entities
tags:
- k8s
- cncf
- ci-cd
- konveyor
- crd
- operator
- kserve
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Konveyor 是什么
- 如何 Konveyor
trigger_keywords:
- Konveyor
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Konveyor

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go, TypeScript

## 概述

Konveyor 是一个应用现代化平台，帮助组织将传统应用（如 Java EE、Spring）迁移和重构到 Kubernetes 平台。它提供应用清单管理、依赖分析、迁移评估、自动化代码重构等能力。Konveyor 通过 AI 辅助分析识别迁移障碍，生成迁移路径建议，并提供 IDE 插件帮助开发者自动化完成代码变更。

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **评估优先**: 先用问卷评估确定迁移优先级，再做深度分析
- **分批迁移**: 从低复杂度应用开始迁移，积累经验后处理复杂应用
- **自定义规则**: 根据组织技术栈添加自定义分析规则
- **AI 审查**: AI 生成的代码修改建议需要人工审查后再应用
- **持续跟踪**: 利用应用清单功能跟踪整个迁移组合的进度

## 架构定位

在 CNCF 生态中，konveyor 属于 **CI/CD** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[operator-pattern]]

## Related

- [[network-service-mesh]] — [[entities/network-service-mesh.md|Network Service Mesh (NSM)]]]Service Mesh）|Service Mesh]] (NSM)
- [[kserve]] — KServe
- [[meshery]] — Meshery
- [[knative]] — Knative
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- konveyor
- [[entities/shipwright.md|Shipwright]]
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
