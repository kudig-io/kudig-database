---
title: KubeVela [entities]
description: '## 概述'
summary: 'KubeVela 是现代应用交付平台，实现了开放应用模型（OAM）规范。'
category: entities
tags:
- k8s
- cncf
- orchestration
- kubevela
- prometheus
- grafana
- helm
- argocd
- flux
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
- KubeVela 是什么
- 如何 KubeVela
trigger_keywords:
- KubeVela
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KubeVela

> **CNCF 状态**: Incubating | **类别**: Orchestration | **主要语言**: Go

## 概述

KubeVela 是现代应用交付平台，由阿里云和微软联合推动，实现了开放应用模型（OAM，Open Application Model）规范。2021 年加入 CNCF 孵化。它为开发者提供以应用为中心的抽象，屏蔽底层 Kubernetes 的复杂性，简化应用部署、运维和多集群管理。KubeVela 的核心理念是"应用优先"——开发者只需定义应用由哪些组件组成、需要什么运维特征（Trait），平台自动将其翻译为底层 Kubernetes/云资源。KubeVela 内置了强大的 CUE 语言引擎，支持自定义组件和策略。它还提供工作流（Workflow）引擎，可以实现金丝雀发布、多集群灰度等复杂交付流程。

## 核心能力

- **应用抽象**: 以 Application CRD 为中心，屏蔽底层 Kubernetes 复杂性
- **OAM 模型**: 组件（Component）、特征（Trait）、策略（Policy）的标准化定义
- **多集群交付**: 统一管理多个 Kubernetes 集群的应用部署
- **GitOps**: 与 Flux/ArgoCD 深度集成实现 GitOps 工作流
- **CUE 可扩展**: 使用 CUE 语言定义自定义组件和特征
- **工作流引擎**: 内置应用交付工作流，支持金丝雀发布、A/B 测试等

## 架构

KubeVela 围绕 OAM 模型构建：

- **Application CRD**: 核心资源，声明应用的组件、特征、策略和工作流
- **ComponentDefinition**: 定义应用组件类型（如 webservice、worker、helm 等）
- **TraitDefinition**: 定义运维特征（如 scaler、route、gateway 等）
- **PolicyDefinition**: 定义部署策略（如多集群分发、环境差异化）
- **WorkflowStepDefinition**: 定义工作流步骤（如 deploy、suspend、notification）
- **Vela Core Controller**: 调谐 Application，通过 CUE 渲染实际资源并部署

交付流程：`Application → Controller (CUE 渲染) → K8s/Cloud Resources → Workflow 执行`

## K8s 集成

KubeVela 以 Kubernetes Controller 方式运行。Application CRD 定义应用的所有组件和运维特征，Vela Core Controller 通过 CUE 引擎将 Application 渲染为底层 Kubernetes 资源（Deployment、Service、Ingress 等）。多集群交付通过将资源分发到目标集群（通过 Cluster Gateway）实现。与 ArgoCD 集成时，VelaController 渲染输出的资源可以作为 ArgoCD Application 的 source。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 CRD 扩展机制完全兼容。

## 生产场景

1. **企业级应用交付平台**: 用 Application CRD 为开发者提供简化的自助式应用交付
2. **多集群灰度发布**: 通过 Workflow 实现跨集群的金丝雀发布
3. **Helm 应用管理**: 通过 KubeVela 管理 Helm 组件，统一多应用交付流程
4. **多云应用部署**: 将应用部署到阿里云 ACK、AWS EKS 等多个云的集群

## 安装

```bash
# 安装 KubeVela CLI
curl -fsSl https://kubevela.net/script/install.sh | bash

# 安装 Vela Controller
vela install

# 安装 VelaUX (管理界面)
vela addon enable velaux

# 部署应用
vela up -f - <<EOF
apiVersion: core.oam.dev/v1beta1
kind: Application
metadata:
  name: first-vela-app
spec:
  components:
  - name: express-server
    type: webservice
    properties:
      image: crccheck/hello-world
      port: 8000
    traits:
    - type: ingress
      properties:
        domain: testsvc.example.com
        http:
          "/": 8000
EOF

# 查看应用状态
vela status first-vela-app
```

## 对比

| 特性 | KubeVela | ArgoCD | Flux | Crossplane |
|------|----------|--------|------|-----------|
| OAM 模型 | ✅ | ❌ | ❌ | ❌ |
| 工作流 | ✅ | ⚠️ Argo Workflow | ❌ | ❌ |
| 多集群 | ✅ | ⚠️ ApplicationSet | ⚠️ | ✅ |
| CNCF 状态 | Incubating | Graduated | Graduated | Incubating |

## 架构定位

在 CNCF 生态中，KubeVela 属于 **Orchestration** 类别，为云原生应用提供以应用为中心的交付能力。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[flux]]
- [[实体/argocd.md|[[ArgoCD|argocd]]]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]

## Related

- [[flux]] — Flux
- [[helm]] — Helm
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/argocd.md|argocd]] — ArgoCD

- kubevela
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
