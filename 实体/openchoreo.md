---
title: OpenChoreo (entities)
description: '## 概述'
summary: 'OpenChoreo 是一个云原生的内部开发者平台 (IDP) 框架，提供开箱即用的开发者自助服务门户。它基于 Kubernetes 构建，为开发团队提供应用创建、部署、监控的统一界面，同时让平台团队可以通过声明式配置定义黄金路径 (Golden Path) 和治理策略。'
category: entities
tags:
- k8s
- cncf
- platform
- openchoreo
- prometheus
- grafana
- argocd
- flux
- opa
- crd
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenChoreo 是什么
- 如何 OpenChoreo
trigger_keywords:
- OpenChoreo
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# OpenChoreo

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go

## 概述

OpenChoreo 是由 WSO2 开发的云原生内部开发者平台（IDP）框架，2024 年进入 CNCF Sandbox。它旨在简化平台工程的落地——为开发团队提供开箱即用的自助式应用部署门户，同时让平台团队通过声明式配置定义**黄金路径（Golden Path）**和治理策略。开发者无需了解 Kubernetes 细节，只需描述"我要部署什么应用"，OpenChoreo 自动处理 CI/CD、配置管理、网络路由和监控接入。

OpenChoreo 基于 Kubernetes CRD 构建，集成 ArgoCD/Flux（GitOps 部署）、Prometheus/Grafana（可观测性）、OPA（策略治理），形成完整的开发者体验闭环。它强调**声明式配置优先**——所有平台行为通过 Git 管理的 YAML 定义，确保可审计和可复现。

## Key Features

- **自助式应用部署**：开发者通过 API/Portal 提交应用定义，自动创建 CI/CD 管道
- **黄金路径模板**：平台团队定义标准化的应用模板（技术栈、部署策略、监控配置）
- **多环境管理**：自动管理 dev/staging/prod 环境的配置差异和部署流水线
- **GitOps 集成**：基于 ArgoCD/Flux 实现声明式部署，Git 作为唯一来源
- **策略治理**：通过 OPA/Gatekeeper 在部署前验证合规性
- **统一可观测性**：自动为每个应用配置 Prometheus 监控和 Grafana Dashboard

## Architecture

OpenChoreo 采用 **Choreo Controller** + **Component Controller** 的分层架构。Choreo Controller 管理平台级配置（环境、组织、项目），Component Controller 管理单个应用组件的生命周期。每个组件创建时自动生成 ArgoCD Application（GitOps 部署）、K8s Deployment/Service/Helm Release（运行时）和 Prometheus ServiceMonitor（监控）。平台配置通过 `ChoreoConfig` CRD 集中管理。

## K8s 集成

OpenChoreo 完全基于 Kubernetes CRD 构建。核心 CRD 包括 `Organization`、`Project`、`Component`（应用组件）、`Environment`（部署环境）和 `DeploymentTrack`（部署管道）。它作为 Kubernetes Operator 运行，自动协调应用从定义到部署的完整生命周期。与标准 K8s RBAC、Namespace 和 NetworkPolicy 兼容。

## 生产部署要点

- **模板标准化**：为不同技术栈创建标准化的应用模板
- **渐进式策略**：从宽松的黄金路径规则开始，逐步收紧
- **自助为主**：尽量让开发者通过 Portal 完成所有操作，减少工单
- **可观测性**：确保每个应用都有统一的监控和日志入口
- **版本控制**：所有平台配置都纳入 Git 版本控制

## 生产场景

1. **开发者自助部署**：开发者通过 Portal 创建微服务，自动获得 CI/CD 和监控
2. **标准化技术栈**：平台团队定义 Spring Boot/Node.js 标准模板，确保一致性
3. **多环境流水线**：应用自动从 dev → staging → prod 晋级
4. **合规治理**：OPA 策略确保所有部署满足安全和合规要求

## 安装与配置

```bash
# Helm 安装 OpenChoreo
helm repo add openchoreo https://openchoreo.github.io/charts/
helm install openchoreo openchoreo/openchoreo -n openchoreo-system --create-namespace
kubectl get pods -n openchoreo-system
```

### 组织与项目配置

```yaml
apiVersion: core.openchoreo.io/v1
kind: Organization
metadata:
  name: my-org
---
apiVersion: core.openchoreo.io/v1
kind: Project
metadata:
  name: my-project
spec:
  organizationRef: my-org
---
apiVersion: core.openchoreo.io/v1
kind: Component
metadata:
  name: payment-service
spec:
  type: Service
  source:
    gitRepository:
      url: https://github.com/myorg/payment-service
      branch: main
  build:
    template: java-springboot
  deployment:
    replicas: 2
    resources:
      requests:
        cpu: 250m
        memory: 512Mi
      limits:
        cpu: "1"
        memory: 1Gi
```

### 环境流水线配置

```yaml
apiVersion: core.openchoreo.io/v1
kind: Pipeline
metadata:
  name: payment-pipeline
spec:
  componentRef: payment-service
  stages:
    - name: dev
      autoDeploy: true
    - name: staging
      approval: auto
      healthCheck:
        path: /health
        timeout: 120s
    - name: production
      approval: manual
      strategy:
        type: canary
        steps: [10, 50, 100]
```

## 运维操作

```bash
# 🟢 查看组织和项目
kubectl get organizations,projects,components -A

# 🟢 查看组件部署状态
kubectl describe component payment-service

# 🟡 触发重新部署
kubectl annotate component payment-service openchoreo.io/redeploy=$(date +%s) --overwrite

# 🟡 回滚到上一版本
kubectl patch component payment-service --type=merge -p '{"spec":{"deployment":{"revision":"previous"}}}'

# 🔴 删除组件（影响所有环境）
kubectl delete component payment-service
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 组件构建失败 | Git 仓库不可达 | `kubectl describe component` | 检查 Git URL 和凭据 |
| 部署超时 | 资源不足/镜像拉取失败 | `kubectl get events` | 检查节点资源和 imagePullSecrets |
| 流水线卡住 | 审批未通过/健康检查失败 | `kubectl describe pipeline` | 手动审批/修复健康检查 |
| 环境晋级失败 | OPA 策略拒绝 | `kubectl logs openchoreo-controller` | 检查策略规则 |
| Portal 无法访问 | Ingress 配置错误 | `kubectl get ingress -n openchoreo-system` | 修复 Ingress 规则 |

```
排查流程:
├── 构建失败
│   ├── kubectl describe component → 查看 Build 状态
│   ├── kubectl logs build-pod → 构建日志
│   └── 确认 Git 仓库可访问、分支存在
├── 部署异常
│   ├── kubectl get pods → 检查 Pod 状态
│   ├── kubectl describe deployment → 查看事件
│   └── 检查资源配额和节点容量
└── 流水线异常
    ├── kubectl describe pipeline → 查看各阶段状态
    └── kubectl logs controller → 查看编排错误
```

## 生产案例

### 案例 1: 开发者自助部署效率提升

- **场景**: 开发团队每次部署需要平台团队协助，等待时间 2-4h
- **方案**: 部署 OpenChoreo，开发者通过 Portal 创建组件、触发部署；平台团队仅维护模板和策略
- **效果**: 部署等待时间从 2-4h 缩短到 <5min，平台团队工单减少 80%

### 案例 2: 多环境晋级合规治理

- **场景**: 生产部署缺少审批流程，多次发生未经验证的代码上线
- **方案**: 配置 Pipeline 三阶段(dev→staging→prod)；prod 阶段强制手动审批 + OPA 策略检查
- **效果**: 生产事故率下降 60%，所有部署有完整审计记录

## 对比

| 特性 | OpenChoreo | Backstage | KubeVela | Humanitec | 适用场景 |
|------|-----------|-----------|----------|-----------|----------|
| 类型 | IDP 平台 | IDP 框架 | 应用交付 | SaaS IDP | 平台工程 |
| GitOps | ✅ ArgoCD/Flux | ⚠️ 插件 | ✅ | ✅ | 持续交付 |
| 策略治理 | ✅ OPA | ⚠️ | ⚠️ | ✅ | 合规 |
| 开源 | ✅ | ✅ | ✅ | ❌ | 自主可控 |
| 开发者门户 | ✅ | ✅ 核心 | ⚠️ | ✅ | 自助服务 |

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[flux]]
- [[实体/argocd.md|[[ArgoCD|argocd]]]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]

## Related

- [[copa]] — Copa (Copacetic)
- [[nats]] — NATS
- [[paralus]] — Paralus
- [[hexa]] — Hexa
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- openchoreo
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
