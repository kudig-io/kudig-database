---
title: CNCF CI/CD 与发布管理项目全景
description: '# CNCF CI/CD 与发布管理项目全景'
summary: 'CNCF CI/CD 生态围绕 **GitOps 持续交付**、**渐进式发布**、**应用构建与打包**、**开发者平台** 四大领域构建。'
category: entities
tags:
- k8s
- cncf
- cicd
- gitops
- progressive-delivery
- developer-platform
- prometheus
- istio
- helm
- flux
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNCF CI/CD 与发布管理项目全景 是什么
- 如何 CNCF CI/CD 与发布管理项目全景
trigger_keywords:
- CNCF
- CI
- CD
- 与发布管理项目全景
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- iac-basics
---



# CNCF CI/CD 与发布管理项目全景

> 聚合页面 | 涵盖 15 个 CNCF CI/CD 项目

## 概述

CNCF CI/CD 生态围绕 **GitOps 持续交付**、**渐进式发布**、**应用构建与打包**、**开发者平台** 四大领域构建。

---

## GitOps 持续交付

### [[argo]] — 毕业项目

Argo 是一组 Kubernetes 原生 CI/CD 工具。

- **Argo CD**: GitOps 持续交付引擎，自动同步 K8s 资源
- **Argo Workflows**: 工作流引擎，DAG 和步骤编排
- **Argo Events**: 事件驱动自动化
- **Argo Rollouts**: 渐进式发布（金丝雀、蓝绿）
- 与 Helm/Kustomize 深度集成

### [[flux]] — 毕业项目

Flux 是 CNCF GitOps 工具包。

- 基于 K8s 原生 CRD 控制器
- Helm/Kustomize 自动同步
- 镜像自动化（自动更新镜像版本）
- 多集群 GitOps

### [[openchoreo]] — 沙箱项目

OpenChoreo 是开源的内部开发者平台。

### [[opengitops]] — 沙箱项目

OpenGitOps 定义 GitOps 原则和标准。

---

## 渐进式发布

### Argo Rollouts（Argo 子项目）

- 金丝雀发布：按比例逐步切流
- 蓝绿发布：快速切换
- 分析模板：自动回滚策略
- 与 Prometheus/Istio/Linkerd 集成

### [[openkruise]] — 孵化项目

OpenKruise 提供增强的 K8s 工作负载控制器。

- 原地更新（in-place update）
- SidecarSet 管理 sidecar 容器
- 高级发布策略

---

## 应用构建与打包

### [[helm]] — 毕业项目（编排类）

Helm 是 K8s 应用包管理器（详见 [[entities/cncf-orchestration.md|cncf-orchestration]]）。

### [[carvel]] — 沙箱项目

Carvel 是 K8s 应用打包和部署工具集。

- **kapp**: 原子化应用部署
- **kbld**: 镜像解析
- **imgpkg**: OCI 打包
- **ytt**: YAML 模板

### [[shipwright]] — 沙箱项目

Shipwright 是 K8s 原生的容器镜像构建框架。

### [[werf]] — 沙箱项目

Werf 是 GitOps 友好的 CI/CD 工具。

### [[kubean]] — 沙箱项目

Kubean 是 K8s 集群生命周期管理工具。

---

## 平台工程与开发者体验

### [[backstage]] — 孵化项目

Backstage 是开源开发者门户。

- 软件目录（[[Service|Service]] Catalog）
- 技术文档（TechDocs）
- 插件生态丰富
- Spotify 开源并广泛采用

### [[artifact-hub]] — 孵化项目

Artifact Hub 是 CNCF 项目和 Helm Chart 的发现平台。

### [[score]] — 沙箱项目

Score 定义平台无关的工作负载规范。

### [[pipecd]] — 沙箱项目

PipeCD 是声明式的 CI/CD 平台。

### [[atlantis]] — 沙箱项目

Atlantis 通过 Pull Request 自动执行 Terraform/基础设施变更。

### [[konveyor]] — 沙箱项目

Konveyor 帮助应用迁移到 K8s。

---

## 架构选型建议

| 场景 | 推荐方案 |
|---|---|
| GitOps 持续交付 | Argo CD 或 Flux |
| 渐进式发布 | Argo Rollouts |
| K8s 应用打包 | Helm 或 Carvel |
| 开发者门户 | Backstage |
| 应用迁移 | Konveyor |

---

## 相关页面

- [[entities/cncf-orchestration.md|cncf-orchestration]] — 编排与应用管理
- [[entities/cncf-observability.md|cncf-observability]] — 可观测性
- [[concepts/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]] — CI/CD 流水线模式

## Related

- [[helm]] — Helm
- [[linkerd]] — Linkerd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[argo]] — Argo Workflows
