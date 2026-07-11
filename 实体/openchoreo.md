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

## 安装

```bash
# Helm 安装 OpenChoreo
helm repo add openchoreo https://openchoreo.github.io/charts/
helm install openchoreo openchoreo/openchoreo -n openchoreo-system --create-namespace

# 创建组织和项目
kubectl apply -f - <<EOF
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
EOF

# 创建应用组件
kubectl apply -f - <<EOF
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
EOF
```

## 对比

| 特性 | OpenChoreo | Backstage | KubeVela | Humanitec |
|------|-----------|-----------|----------|-----------|
| 类型 | IDP 平台 | IDP 框架 | 应用交付 | SaaS IDP |
| GitOps | ✅ ArgoCD/Flux | ⚠️ 插件 | ✅ | ✅ |
| 策略治理 | ✅ OPA | ⚠️ | ⚠️ | ✅ |
| 开源 | ✅ | ✅ | ✅ | ❌ |

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
