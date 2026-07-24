---
title: Helm (entities)
description: '## 概述'
summary: 'description: ''## 项目概述'''
category: entities
tags:
- k8s
- cncf
- storage
- helm
- jaeger
- argocd
- flux
- crd
- operator
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Helm 是什么
- 如何 Helm
trigger_keywords:
- Helm
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
- ebpf-basics
- tracing-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Helm

> **CNCF 状态**: Graduated | **类别**: Storage | **主要语言**: Go

## 概述

description: '## 项目概述'

## 核心能力

- **Chart 打包**: 将 Kubernetes 资源打包为可重用的 Chart
- **模板引擎**: Go 模板语法支持动态配置
- **版本管理**: Release 版本控制和回滚
- **依赖管理**: Chart 依赖声明和管理
- **仓库系统**: Chart 分发和共享
- **钩子机制**: 生命周期钩子支持

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 使用语义化版本管理 Chart
- 将 Chart 存储在私有仓库
- 使用 values 文件管理环境配置
- 实施 Chart 签名和验证
- 使用 `--atomic` 保证原子性部署
- 合理设置 `--timeout` 超时时间

## 安装与配置

```bash
# 安装 Helm 3
curl https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3 | bash

# 添加常用仓库
helm repo add bitnami https://charts.bitnami.com/bitnami
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update

# 部署示例（原子性安装）
helm install my-nginx bitnami/nginx \
  --namespace web --create-namespace \
  --atomic --timeout 5m \
  --values production-values.yaml
```

### Chart 结构规范

```yaml
# Chart.yaml
apiVersion: v2
name: my-service
description: A production-ready microservice chart
type: application
version: 1.2.0
appVersion: "2.1.0"
dependencies:
- name: redis
  version: "17.x"
  repository: https://charts.bitnami.com/bitnami
  condition: redis.enabled

# values.yaml 分层管理
# values.yaml          → 默认值
# values-staging.yaml  → 预发环境
# values-production.yaml → 生产环境
```

### 私有 Chart 仓库

```bash
# 使用 ChartMuseum 搭建私有仓库
helm plugin install https://github.com/chartmuseum/helm-push
helm cm-push my-chart/ https://chartmuseum.internal.company.com

# OCI 仓库（Helm 3.8+）
helm registry login registry.company.com
helm push my-chart-1.2.0.tgz oci://registry.company.com/charts
helm pull oci://registry.company.com/charts/my-chart --version 1.2.0
```

## 运维操作

```bash
# 🟢 查看已部署 Release
helm list -A
helm status my-release -n web
helm history my-release -n web

# 🟢 渲染模板（不实际部署）
helm template my-release ./chart -f values.yaml --debug

# 🟢 查看当前 values
helm get values my-release -n web
helm get manifest my-release -n web

# 🟡 升级 Release
helm upgrade my-release ./chart -f values-production.yaml --atomic -n web

# 🟡 回滚到指定版本
helm rollback my-release 3 -n web

# 🔴 卸载 Release（删除所有资源）
helm uninstall my-release -n web

# 🔴 卸载并保留 CRD
helm uninstall my-release -n web --keep-history
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Install 超时 | 资源未 Ready/拉镜像失败 | `kubectl get events -n web` | 检查镜像地址和资源配额 |
| Upgrade 失败回滚 | 模板渲染错误 | `helm template ./chart --debug` | 修复模板语法 |
| Hook 执行失败 | Job 权限不足 | `kubectl logs job/pre-install-hook` | 检查 RBAC 配置 |
| 依赖解析失败 | 仓库不可达/版本不存在 | `helm dependency update ./chart` | 检查网络和版本号 |
| Release 状态 pending | 上次操作未完成 | `helm history my-release` | `helm rollback` 后重试 |

```
排查流程:
├── 部署失败
│   ├── helm status → Release 状态
│   ├── kubectl get events → Pod 创建失败原因
│   ├── helm template --debug → 模板渲染验证
│   └── helm get manifest → 实际提交的资源
├── 升级异常
│   ├── helm diff upgrade → 变更差异预览
│   ├── helm history → 版本历史
│   └── helm rollback → 回滚到稳定版本
└── 依赖问题
    ├── helm dependency list → 依赖状态
    ├── helm repo update → 刷新仓库索引
    └── 检查 Chart.lock → 版本锁定
```

## 生产案例

### 案例1: Helm Upgrade 导致生产服务中断

- **场景**: 升级 ingress chart 时因模板错误导致 Ingress 规则被删除，外部流量中断 5 分钟
- **排查**: `helm history` 显示 upgrade 状态 failed，`kubectl get ingress` 确认规则丢失
- **方案**:
  1. 立即 `helm rollback my-release 5` 回滚
  2. 后续升级强制使用 `--atomic` 参数（失败自动回滚）
  3. CI/CD 中添加 `helm template --validate` 预检查
- **效果**: 服务 2 分钟恢复，后续升级零事故

### 案例2: 多环境 Values 管理混乱

- **场景**: 50+ 微服务 × 3 环境 = 150+ values 文件，配置漂移严重
- **排查**: 对比发现 staging 和 production 的资源配置不一致
- **方案**:
  1. 采用基础 values + 环境 overlay 分层架构
  2. 使用 Helmfile 统一管理多环境部署
  3. GitOps 仓库存储所有 values，PR 审批变更
- **效果**: 配置一致率达 100%，变更可审计

## 架构定位

在 CNCF 生态中，Helm 属于 **Application Definition & Image Build** 类别，是 Kubernetes 应用包管理的事实标准。

## 参考链接

- [[flux]]
- [[实体/argocd.md|[[ArgoCD|argocd]]]]
- [[实体/vault.md|[[HashiCorp Vault|vault]]]]
- [[deployment]]
- [[概念/gitops-principles.md|gitops-principles]]

## Related

- [[konveyor]] — Konveyor
- [[bfe]] — BFE
- [[score]] — Score
- [[jaeger]] — Jaeger
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 36-ecosystem-kustomize-helm-argocd
- 07-helm-advanced-operations
- 06-helm-charts-management
- [[故障诊断/高级排障/36-helm-chart-troubleshooting.md|36-helm-chart-troubleshooting]]
- [[故障诊断/FTA故障树/list/helm-fta.md|Helm 发布异常故障树分析]]
- [[故障诊断/高级排障/08-cluster-operations/03-helm-troubleshooting.md|03-helm-troubleshooting]]
- helm
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-4.0.md|RELEASE-NOTES-4.0]]
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.18.md|RELEASE-NOTES-3.18]]
- RELEASE-NOTES-2.16
- RELEASE-NOTES-2.12
- RELEASE-NOTES-2.13
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-4.1.md|RELEASE-NOTES-4.1]]
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.19.md|RELEASE-NOTES-3.19]]
- RELEASE-NOTES-2.17
- RELEASE-NOTES-2.4
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.12.md|RELEASE-NOTES-3.12]]
- RELEASE-NOTES-3.5
- RELEASE-NOTES-2.0
- RELEASE-NOTES-3.1
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.16.md|RELEASE-NOTES-3.16]]
- RELEASE-NOTES-2.1
- RELEASE-NOTES-3.0
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.17.md|RELEASE-NOTES-3.17]]
- RELEASE-NOTES-2.5
- RELEASE-NOTES-1.2
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.13.md|RELEASE-NOTES-3.13]]
- RELEASE-NOTES-3.4
- RELEASE-NOTES-2.2
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.14.md|RELEASE-NOTES-3.14]]
- RELEASE-NOTES-3.3
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.20.md|RELEASE-NOTES-3.20]]
- RELEASE-NOTES-2.6
- RELEASE-NOTES-3.7
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.10.md|RELEASE-NOTES-3.10]]
- RELEASE-NOTES-2.7
- RELEASE-NOTES-3.6
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.11.md|RELEASE-NOTES-3.11]]
- RELEASE-NOTES-2.3
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.15.md|RELEASE-NOTES-3.15]]
- RELEASE-NOTES-3.2
- RELEASE-NOTES-2.8
- RELEASE-NOTES-2.10
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.9.md|RELEASE-NOTES-3.9]]
- RELEASE-NOTES-2.14
- RELEASE-NOTES-2.15
- RELEASE-NOTES-2.9
- RELEASE-NOTES-2.11
- [[归档/release-notes/cli-tools/helm/RELEASE-NOTES-3.8.md|RELEASE-NOTES-3.8]]
- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[实体/k8s-advanced-ecosystem.md|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[实体/release-notes-cicd-gitops.md|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[实体/release-notes-cli-tools.md|发布说明索引 — CLI 工具]] — Cross-reference
- [[实体/k8s-cluster-create.md|Kubernetes 集群创建操作指南]] — Cross-reference
- [[实体/k8s-platform-extensions.md|平台运维与扩展生态：Helm、CI/CD、Operator 开发与服务网格]] — Cross-reference
- [[概念/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]] — Cross-reference
- [[概念/GitOps × 平台工程.md|GitOps x 平台工程]] — Cross-reference
- [[概念/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[概念/cli-tools-evolution.md|CLI 工具演进]] — Cross-reference
- [[概念/infrastructure-as-code.md|Infrastructure as Code]] — Cross-reference
- [[概念/gitops-tool-evolution.md|GitOps 工具演进]] — Cross-reference
- [[技能/网络/ingress/培训/learn-05-ingress-basics.md|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference
- [[技能/控制面/crd-operator/运维操作/develop-crd-operator.md|Develop CRD Operator]] — Cross-reference
- [[技能/可观测性/monitoring/最佳实践/k8s-distributed-tracing-guide.md|Kubernetes 分布式追踪最佳实践]] — Cross-reference
- [[技能/工作负载/pod/方法论/agent/Agent Orchestration Patterns.md|Agent Orchestration Patterns for FTA]] — Cross-reference
- [[技能/集群运维/gitops-argocd/诊断排障/ts-gitops-devops.md|GitOps/DevOps 排查]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-04-debug-tools-setup.md|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[技能/集群运维/cluster-upgrade/诊断排障/ts-cluster-operations.md|集群运维故障排查]] — Cross-reference
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/openkruise-index.md|OpenKruise 全局索引]]
- [[生态参考/领域索引/helm-index.md|Helm 全局索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
