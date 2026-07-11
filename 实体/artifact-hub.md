---
title: Artifact Hub [entities]
description: '## 概述'
summary: 'Artifact Hub 是云原生制品的发现和分发平台。它是 CNCF 生态系统的中央枢纽，支持搜索、发现和发布 Helm charts、OPA 策略、Falco 规则、KEDA scalers 等多种制品类型。'
category: entities
tags:
- k8s
- cncf
- supply-chain
- artifact-hub
- helm
- opa
- falco
- crd
- operator
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Artifact Hub 是什么
- 如何 Artifact Hub
trigger_keywords:
- Artifact
- Hub
prerequisites:
- kubectl-basics
- helm-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Artifact Hub

> **CNCF 状态**: Incubating | **类别**: Supply Chain | **主要语言**: Go, TypeScript

## 概述

Artifact Hub 是 CNCF 生态系统的中央制品发现和分发平台，2020 年加入 CNCF Sandbox，后晋升为 Incubating。它支持搜索、发现和发布 Helm Charts、OLM Operators、Falco 规则、OPA 策略、KEDA Scalers、Tekton Pipelines、Container Images 等多种云原生制品类型。Artifact Hub 的目标是成为云原生生态的 "npm registry"，让开发者和运维人员能够一站式发现和安装云原生组件。

## 核心特性

- **多制品类型**: 统一搜索 Helm、OPA、Falco、KEDA、Tekton、Tinkerbell Actions、Container Images 等
- **全文搜索**: 跨制品类型的全文搜索和标签过滤
- **丰富元数据**: 版本历史、依赖关系、安全评级、维护者信息、README 文档
- **安全扫描**: 自动检测容器镜像漏洞
- **签名验证**: 支持 Cosign 签名的制品验证状态展示
- **订阅通知**: 跟踪制品更新，接收新版本和变更通知

## 架构

Artifact Hub 采用前后端分离的微服务架构。后端使用 Go 实现，提供 RESTful API，使用 PostgreSQL 存储制品元数据。前端使用 TypeScript/React 构建。制品来源追踪器（Tracker）定期扫描已注册的仓库（GitHub、Helm Registry、OCI Registry 等），解析制品元数据并更新索引。安全扫描器自动对制品中的容器镜像进行漏洞扫描。整个系统支持 Helm Chart 方式部署到 Kubernetes。

## Kubernetes 集成

Artifact Hub 本身作为服务部署，通过 Helm Chart 安装到 Kubernetes。它与 Kubernetes 生态深度集成：Helm Charts 可直接通过 Artifact Hub 发现并安装；OLM Operators 通过 OperatorHub 集成分发。`helm search hub` 命令直接查询 Artifact Hub 的 API。它还支持 Tekton Pipeline 模板发现和 KEDA Scaler 模板浏览。

## 生产使用场景

1. **组件发现**: 团队在 Artifact Hub 搜索可复用的 Helm Charts 和 Operators
2. **制品发布**: 开源项目在 Artifact Hub 注册仓库，增加可见性
3. **安全合规**: 通过安全扫描评级选择可信制品
4. **版本跟踪**: 订阅关键依赖制品的更新通知

## 安装

```bash
# Artifact Hub 本身无需安装到集群，直接访问 artifacthub.io
# 私有部署
helm repo add artifact-hub https://artifacthub.github.io/helm-charts
helm install artifact-hub artifact-hub/artifact-hub
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Artifact Hub** | CNCF 官方、多制品类型、免费 | 定制化能力有限 |
| OperatorHub.io | 专注 Operators、Red Hat 支持 | 仅 Operators |
| Helm Hub (已合并) | Helm 原生 | 已并入 Artifact Hub |
| Kubeapps Hub | 与 Kubeapps 集成 | 仅 Helm Charts |

## 架构定位

在 CNCF 生态中，Artifact Hub 属于 **Supply Chain / Platform** 类别，是云原生制品发现的标准入口。它与 Helm、Operator Framework、Tekton、KEDA 等项目协同工作。

## 参考链接

- [[falco]]
- [[operator-pattern]]
- [[概念/secrets-management.md|secrets-management]]
- [[概念/security-defense-depth.md|security-defense-depth]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[opa]] — OPA (Open Policy Agent)
- [[helm]] — Helm
- [[keda]] — KEDA
- [[falco]] — Falco
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- artifact-hub
- [[实体/cncf-cicd.md|[[CNCF CI/CD 与发布管理项目全景|CNCF CI/CD 与发布管理项目全景]]]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
