---
title: Flux
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- grafana
- helm
- flux
- opa
- rbac
- webhook
- argocd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Flux 是什么
- 如何 Flux
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Flux
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- policy-basics
---

title: Flux
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- grafana
- helm
- flux
- opa
- rbac
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Flux 是什么
- 如何 Flux
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Flux
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# Flux

> **成熟度**: Graduated | **加入时间**: 2019-08 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://fluxcd.io |
| **GitHub** | https://github.com/fluxcd/flux2 |
| **文档** | https://fluxcd.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | App Definition & Development |

---

## 项目概述

### 简介
Flux 是一组持续交付和渐进式交付的 GitOps 工具，用于保持 Kubernetes 集群与配置源（如 Git 仓库）同步。

### 核心定位
Flux 实现了真正的 GitOps 工作流，将 Git 作为声明式基础设施和应用的单一真实来源，自动化地将变更同步到 Kubernetes 集群。

### 发展历程
- **2016**: Weaveworks 创建 Flux 项目
- **2019-08**: 加入 CNCF 作为沙箱项目
- **2021-03**: 升级为 CNCF 孵化项目
- **2022-11**: 成为 CNCF 毕业项目

---

## 核心功能

### 主要特性
- **GitOps 自动化**: 自动同步 Git 变更到集群
- **多源支持**: Git、Helm、OCI 等多种来源
- **Kustomize 集成**: 原生 Kustomize 支持
- **Helm 控制器**: Helm Release 生命周期管理
- **通知系统**: 事件通知和告警
- **镜像自动更新**: 自动更新容器镜像版本

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                      Flux Controllers                       │
│  ┌─────────────────┐ ┌─────────────────┐ ┌───────────────┐ │
│  │ Source Ctrl     │ │ Kustomize Ctrl  │ │  Helm Ctrl    │ │
│  │ (Git/Helm/OCI)  │ │ (Kustomization) │ │ (HelmRelease) │ │
│  └─────────────────┘ └─────────────────┘ └───────────────┘ │
│  ┌─────────────────┐ ┌─────────────────┐                   │
│  │ Notification    │ │ Image Automation│                   │
│  │ Controller      │ │ Controllers     │                   │
│  └─────────────────┘ └─────────────────┘                   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Kubernetes Cluster                        │
│  ┌─────────────────────────────────────────────────────────┐│
│  │              Deployed Applications                      ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| Source Controller | 源管理 | 管理 Git/Helm/OCI 源 |
| Kustomize Controller | 配置管理 | 应用 Kustomize 配置 |
| Helm Controller | Helm 管理 | 管理 Helm Release |
| Notification Controller | 通知 | 发送事件通知 |
| Image Controllers | 镜像自动化 | 自动更新镜像版本 |

### 工作原理
1. Source Controller 拉取配置源
2. 检测配置变更
3. Kustomize/Helm Controller 生成资源清单
4. 将资源应用到集群
5. Notification Controller 发送状态通知

---

## 使用场景

### 典型应用
- **GitOps 持续交付**: 自动化应用部署
- **多集群管理**: 统一管理多个集群配置
- **环境晋升**: Dev → Staging → Prod 晋升
- **合规审计**: 通过 Git 历史审计变更

### 适用条件
- 需要 GitOps 工作流
- 多环境/多集群管理
- 需要审计和回滚能力
- 团队使用 Git 协作

### 不适用场景
- 命令式操作需求
- 简单的单一应用

---

## 快速开始

### 安装部署
```bash
# 安装 Flux CLI
brew install fluxcd/tap/flux

# 引导安装（使用 GitHub）
flux bootstrap github \
  --owner=my-org \
  --repository=fleet-infra \
  --branch=main \
  --path=./clusters/my-cluster \
  --personal
```

### 基础配置
```yaml
# GitRepository 源
apiVersion: source.toolkit.fluxcd.io/v1
kind: GitRepository
metadata:
  name: my-app
  namespace: flux-system
spec:
  interval: 1m
  url: https://github.com/my-org/my-app
  ref:
    branch: main

---
# Kustomization
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: my-app
  namespace: flux-system
spec:
  interval: 5m
  path: ./deploy
  prune: true
  sourceRef:
    kind: GitRepository
    name: my-app
  targetNamespace: default
```

### 验证测试
```bash
# 检查 Flux 状态
flux check

# 查看所有资源
flux get all

# 强制同步
flux reconcile kustomization my-app --with-source
```

---

## 最佳实践

### 生产环境建议
- 使用 SSH 密钥认证 Git
- 配置多环境目录结构
- 启用 Webhook 触发同步
- 配置通知渠道

### 性能优化
- 合理设置同步间隔
- 使用 Helm OCI 源
- 配置资源限制
- 监控控制器性能

### 安全加固
- 使用 SOPS 加密 Secrets
- 配置 RBAC 权限
- 启用镜像签名验证
- 审计 Git 操作

---

## 生态集成

### 相关 CNCF 项目
- **Helm**: HelmRelease 支持
- **Prometheus**: 指标导出
- **OPA/Kyverno**: 策略验证

### 常见集成方案
- Flux + SOPS/Sealed Secrets
- Flux + Prometheus/Grafana 监控
- Flux + Slack/Teams 通知
- Flux + Weave GitOps UI

---

## 参考资源

- [官方文档](https://fluxcd.io/docs)
- [GitHub Repo](https://github.com/fluxcd/flux2)
- [CNCF 项目页面](https://www.cncf.io/projects/flux/)
- [GitOps 指南](https://fluxcd.io/flux/guides/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/gitops.md|gitops]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[references/release-notes-cicd-gitops|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[references/k8s-production-operations|生产运维：GitOps、FinOps、灾备恢复与变更管理]] — Cross-reference
- [[synthesis/IaC x 多集群管理|基础设施即代码 x 多集群管理]] — Cross-reference
- [[synthesis/GitOps x 平台工程|GitOps x 平台工程]] — Cross-reference
- [[concepts/gitops-principles|GitOps Principles and Practice]] — Cross-reference
- [[concepts/gitops-tool-evolution|GitOps 工具演进]] — Cross-reference
- [[skills/ts-gitops-devops|GitOps/DevOps 排查]] — Cross-reference
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[entities/argocd|ArgoCD]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/helm-index|Helm 全局索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.12|flux v0.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.26|flux v0.26 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.8|flux v0.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.36|flux v0.36 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.22|flux v0.22 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.16|flux v0.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.32|flux v0.32 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.23|flux v0.23 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.17|flux v0.17 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.33|flux v0.33 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.13|flux v0.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.27|flux v0.27 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.9|flux v0.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.37|flux v0.37 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.4|flux v2.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.18|flux v0.18 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.28|flux v0.28 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.0|flux v2.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.38|flux v0.38 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.6|flux v0.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.29|flux v0.29 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.1|flux v2.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.39|flux v0.39 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.7|flux v0.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.5|flux v2.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.19|flux v0.19 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.2|flux v2.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.4|flux v0.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.6|flux v2.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.0|flux v0.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.7|flux v2.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.3|flux v2.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.5|flux v0.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.20|flux v0.20 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-2.8|flux v2.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.14|flux v0.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.30|flux v0.30 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.10|flux v0.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.41|flux v0.41 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.24|flux v0.24 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.34|flux v0.34 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.11|flux v0.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.40|flux v0.40 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.25|flux v0.25 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.35|flux v0.35 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.21|flux v0.21 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.15|flux v0.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cicd-gitops/flux/RELEASE-NOTES-0.31|flux v0.31 Release Notes]]
