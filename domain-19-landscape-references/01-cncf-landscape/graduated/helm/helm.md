---
title: Helm
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- argocd
- flux
- harbor
- rbac
- ingress
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Helm 是什么
- 如何 Helm
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Helm
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- gitops-basics
- ebpf-basics
---

title: Helm
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- argocd
- flux
- harbor
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
- Helm 是什么
- 如何 Helm
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Helm
- cncf
- landscape
cross_refs:
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/helm-fta.md
  label: '故障树: helm'
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
# Helm

> **成熟度**: Graduated | **加入时间**: 2018-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://helm.sh |
| **GitHub** | https://github.com/helm/helm |
| **文档** | https://helm.sh/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | App Definition & Development |

---

## 项目概述

### 简介
Helm 是 Kubernetes 的包管理器，帮助用户定义、安装和升级复杂的 Kubernetes 应用。

### 核心定位
Helm 通过 Chart 机制将 Kubernetes 资源打包、版本化和分发，解决了 Kubernetes 应用的复杂部署、配置管理和版本升级问题，是 Kubernetes 生态中最流行的应用管理工具。

### 发展历程
- **2015**: Deis（后被微软收购）创建 Helm
- **2016**: Helm v2 发布，引入 Tiller
- **2018-06**: 加入 CNCF 作为孵化项目
- **2019-11**: Helm v3 发布，移除 Tiller
- **2020-04**: 成为 CNCF 毕业项目

---

## 核心功能

### 主要特性
- **Chart 打包**: 将 Kubernetes 资源打包为可重用的 Chart
- **模板引擎**: Go 模板语法支持动态配置
- **版本管理**: Release 版本控制和回滚
- **依赖管理**: Chart 依赖声明和管理
- **仓库系统**: Chart 分发和共享
- **钩子机制**: 生命周期钩子支持

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                       Helm CLI                              │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────┐│
│  │   install    │ │   upgrade    │ │     rollback         ││
│  │   uninstall  │ │   list       │ │     history          ││
│  └──────────────┘ └──────────────┘ └──────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                        Chart                                │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────┐│
│  │  Chart.yaml  │ │ values.yaml  │ │     templates/       ││
│  │  (元数据)    │ │  (默认值)    │ │  (K8s 资源模板)      ││
│  └──────────────┘ └──────────────┘ └──────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Kubernetes Cluster                        │
│  ┌──────────────────────────────────────────────────────── ┐│
│  │           Release (已部署的 Chart 实例)                 ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 整体架构
Helm v3 采用纯客户端架构，直接与 Kubernetes API Server 交互，Release 信息存储在 Kubernetes Secret 中。

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| Helm CLI | 命令行工具 | 用户交互的主要界面 |
| Chart | 应用包 | Kubernetes 资源的打包格式 |
| Release | 部署实例 | Chart 的一次部署 |
| Repository | 仓库 | Chart 的存储和分发 |
| Values | 配置值 | 用于自定义 Chart 的参数 |

### 工作原理
1. 用户编写 Chart（模板 + 默认值）
2. 用户提供自定义 values 覆盖默认值
3. Helm 渲染模板生成 Kubernetes 资源清单
4. Helm 将资源提交到 Kubernetes API
5. Helm 将 Release 信息存储为 Secret

---

## 使用场景

### 典型应用
- **应用部署**: 标准化 Kubernetes 应用部署
- **配置管理**: 环境差异化配置
- **版本升级**: 应用版本管理和回滚
- **应用分发**: 共享可重用的应用包

### 适用条件
- 需要标准化 Kubernetes 应用部署
- 需要管理复杂的多资源应用
- 需要跨环境的配置差异化
- 需要应用版本控制和回滚

### 不适用场景
- 简单的单一资源部署
- 需要声明式 GitOps 工作流（考虑 Flux/ArgoCD）
- 需要细粒度的资源管理

---

## 快速开始

### 安装部署
```bash
# macOS
brew install helm

# Linux
curl https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3 | bash

# Windows
choco install kubernetes-helm
```

### 基础配置
```yaml
# Chart.yaml
apiVersion: v2
name: my-app
description: A Helm chart for my application
type: application
version: 0.1.0
appVersion: "1.0.0"

# values.yaml
replicaCount: 3
image:
  repository: nginx
  tag: "1.25"
  pullPolicy: IfNotPresent
service:
  type: ClusterIP
  port: 80

# templates/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ .Release.Name }}
spec:
  replicas: {{ .Values.replicaCount }}
  selector:
    matchLabels:
      app: {{ .Release.Name }}
  template:
    metadata:
      labels:
        app: {{ .Release.Name }}
    spec:
      containers:
      - name: {{ .Chart.Name }}
        image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
        ports:
        - containerPort: 80
```

### 验证测试
```bash
# 添加仓库
helm repo add bitnami https://charts.bitnami.com/bitnami
helm repo update

# 搜索 Chart
helm search repo nginx

# 安装 Chart
helm install my-release bitnami/nginx

# 查看 Release
helm list
helm status my-release

# 升级 Release
helm upgrade my-release bitnami/nginx --set replicaCount=3

# 回滚 Release
helm rollback my-release 1

# 卸载 Release
helm uninstall my-release
```

---

## 最佳实践

### 生产环境建议
- 使用语义化版本管理 Chart
- 将 Chart 存储在私有仓库
- 使用 values 文件管理环境配置
- 实施 Chart 签名和验证

### 性能优化
- 使用 `--atomic` 保证原子性部署
- 合理设置 `--timeout` 超时时间
- 使用 `--wait` 等待资源就绪
- 优化模板减少渲染时间

### 安全加固
- 使用 Helm Secrets 管理敏感信息
- 启用 Chart 签名验证
- 限制 Helm 操作的 RBAC 权限
- 审计 Helm 操作日志

---

## 生态集成

### 相关 CNCF 项目
- **Artifact Hub**: Chart 发现和分发平台
- **Flux**: Helm Release 的 GitOps 管理
- **ArgoCD**: Helm Chart 的 GitOps 部署
- **Harbor**: Helm Chart 存储

### 常见集成方案
- Helm + ArgoCD GitOps 工作流
- Helm + Harbor Chart 仓库
- Helm + Vault 密钥管理
- Helm + CI/CD 自动化部署

---

## 社区与支持

### 社区资源
- Slack: https://slack.k8s.io #helm-users
- 邮件列表: helm-users@lists.cncf.io
- Twitter: @HelmPack

### 贡献指南
访问 https://github.com/helm/helm/blob/main/CONTRIBUTING.md 了解参与方式

---

## 参考资源

- [官方文档](https://helm.sh/docs)
- [GitHub Repo](https://github.com/helm/helm)
- [CNCF 项目页面](https://www.cncf.io/projects/helm/)
- [Artifact Hub](https://artifacthub.io/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-19-landscape-references/04-cncf-fta-index.md|04-cncf-fta-index]]
- [[CONTRIBUTING.md|CONTRIBUTING]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-19-landscape-references/98-merged-indexes/README-from-domain-19-landscape-references|Domain-34: CNCF Landscape 开源项目]] — Cross-reference
- [[references/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[references/release-notes-cicd-gitops|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[references/release-notes-cli-tools|发布说明索引 — CLI 工具]] — Cross-reference
- [[references/k8s-cluster-create|Kubernetes 集群创建操作指南]] — Cross-reference
- [[references/k8s-platform-extensions|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[synthesis/控制器模式 × Operator 模式|控制器模式 × Operator 模式]] — Cross-reference
- [[synthesis/GitOps x 平台工程|GitOps x 平台工程]] — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/cli-tools-evolution|CLI 工具演进]] — Cross-reference
- [[concepts/infrastructure-as-code|Infrastructure as Code]] — Cross-reference
- [[concepts/gitops-tool-evolution|GitOps 工具演进]] — Cross-reference
- [[skills/learn-05-ingress-basics|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference
- [[skills/develop-crd-operator|Develop CRD Operator]] — Cross-reference
- [[skills/k8s-distributed-tracing-guide|Kubernetes 分布式追踪最佳实践]] — Cross-reference
- [[skills/Agent Orchestration Patterns|Agent Orchestration Patterns for FTA]] — Cross-reference
- [[skills/ts-gitops-devops|GitOps/DevOps 排查]] — Cross-reference
- [[skills/learn-04-debug-tools-setup|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[skills/ts-cluster-operations|集群运维故障排查]] — Cross-reference
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[entities/argocd|ArgoCD]] — Cross-reference
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/openkruise-index|OpenKruise 全局索引]]
- [[domain-19-landscape-references/topic-index/helm-index|Helm 全局索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-4.0|helm v4.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.18|helm v3.18 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.16|helm v2.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.12|helm v2.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.13|helm v2.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-4.1|helm v4.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.19|helm v3.19 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.17|helm v2.17 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.4|helm v2.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.12|helm v3.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.5|helm v3.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.0|helm v2.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.16|helm v3.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.0|helm v3.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.17|helm v3.17 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.5|helm v2.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-1.2|helm v1.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.13|helm v3.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.4|helm v3.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.2|helm v2.2 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.14|helm v3.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.3|helm v3.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.20|helm v3.20 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.6|helm v2.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.7|helm v3.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.10|helm v3.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.7|helm v2.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.6|helm v3.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.11|helm v3.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.3|helm v2.3 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.15|helm v3.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.8|helm v2.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.10|helm v2.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.9|helm v3.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.14|helm v2.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.15|helm v2.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.9|helm v2.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-2.11|helm v2.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/helm/RELEASE-NOTES-3.8|helm v3.8 Release Notes]]
