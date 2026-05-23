---
title: 发布说明索引 — CLI 工具
description: '| [[helm]] | 42 | v4.1 | v4.1 | Kubernetes 包管理器 |'
category: references
tags:
- k8s
- release-notes
- cli-tools
- helm
- kind
- kops
- kustomize
- minikube
- rbac
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布说明索引 — CLI 工具 是什么
- 如何 发布说明索引 — CLI 工具
trigger_keywords:
- 发布说明索引
- CLI
- 工具
prerequisites:
- kubectl-basics
- helm-basics
created: "2026-05-23"
---

# 发布说明索引 — CLI 工具

> 本文档汇总 CLI 工具领域 5 个项目的发布说明索引，共覆盖 **187 篇**发布说明。

---

## 项目总览

| 项目 | 文件数 | 最新版本 | 最近 Breaking Changes | 说明 |
|------|--------|----------|----------------------|------|
| [[helm]] | 42 | v4.1 | v4.1 | Kubernetes 包管理器 |
| Kind | 32 | v0.31 | v0.31 | 本地 K8s 集群工具 |
| Kops | 32 | v1.35 | v1.31 | 集群生命周期管理 |
| Kustomize | 7 | v3.3 | — | 配置定制工具 |
| Minikube | 74 | v1.38 | v1.37 | 本地开发环境 |

---

## 项目详情

### Helm

- **实体页面**: [[helm|Helm]]
- **最新版本**: v4.1
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/cli-tools/helm/`
- **版本覆盖**: v0.1 → v4.1（42 个版本）
- **Breaking Changes 提醒**:
  - v4.1: Chart API 版本和依赖管理行为变更
  - v3.0 (历史): Tiller 组件移除，RBAC 模型重构
- **升级要点**: v4.x 引入 OCI 镜像仓库作为默认 Chart 存储

### Kind

- **最新版本**: v0.31
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/cli-tools/kind/`
- **版本覆盖**: v0.0 → v0.31（32 个版本）
- **Breaking Changes 提醒**:
  - v0.31: 默认 CNI 和容器运行时配置变更
- **升级要点**: 持续跟进 Kubernetes 最新版本支持

### Kops

- **最新版本**: v1.35
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/cli-tools/kops/`
- **版本覆盖**: v0.1 → v1.35（32 个版本）
- **Breaking Changes 提醒**:
  - v1.31: 云提供商配置格式变更
- **升级要点**: 支持 AWS/GCP 多集群管理

### Kustomize

- **最新版本**: v3.3
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/cli-tools/kustomize/`
- **版本覆盖**: v0.1 → v3.3（7 个版本）
- **升级要点**: 已内置到 kubectl，独立版本更新较少

### Minikube

- **最新版本**: v1.38
- **发布说明目录**: `domain-19-landscape-references/_archived-release-notes/cli-tools/minikube/`
- **版本覆盖**: v0.1 → v1.38（74 个版本）
- **Breaking Changes 提醒**:
  - v1.37: 驱动程序默认配置变更
- **升级要点**: 支持多种容器运行时和虚拟化驱动

---

## 跨项目 Breaking Changes 汇总

| 版本 | 项目 | 变更摘要 |
|------|------|----------|
| v4.1 | Helm | Chart API 版本和依赖管理变更 |
| v0.31 | Kind | 默认 CNI 和运行时配置变更 |
| v1.31 | Kops | 云提供商配置格式变更 |
| v1.37 | Minikube | 驱动程序默认配置变更 |

---

## 相关导航

- [[domain-19-landscape-references/98-merged-indexes/index|发布说明阅读指南]]
- [[MOC|发布说明总目录]]

## Related

- [[references/k8s-platform-extensions|k8s-platform-extensions]] — 平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格
- [[domain-19-landscape-references/98-merged-indexes/index|release-notes-core-deps]] — 发布说明索引 — 核心依赖
- [[helm]] — Helm
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
