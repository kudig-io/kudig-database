---
title: Kubewarden [entities]
description: '## 概述'
summary: 'Kubewarden 是一个 Kubernetes 策略引擎，使用 WebAssembly (Wasm) 运行准入策略。它允许使用任何编译为 Wasm 的编程语言 (Rust、Go、C#、Swift 等) 编写策略，并通过 OCI 镜像仓库分发。Kubewarden 支持动态准入控制和审计模式。'
category: entities
tags:
- k8s
- cncf
- policy
- kubewarden
- argocd
- crd
- operator
- wasm
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubewarden 是什么
- 如何 Kubewarden
trigger_keywords:
- Kubewarden
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubewarden

> **CNCF 状态**: Sandbox | **类别**: Policy | **主要语言**: Rust, Go

## 概述

Kubewarden 是由 SUSE/Rancher 开发的 Kubernetes 策略引擎，2021 年加入 CNCF Sandbox。它使用 WebAssembly（Wasm）运行准入控制策略，允许使用任何编译为 Wasm 的编程语言（Rust、Go、C#、Swift、Rego 等）编写策略，并通过 OCI 镜像仓库分发。Kubewarden 是 OPA Gatekeeper 和 Kyverno 的替代方案，核心优势在于策略的运行时隔离（Wasm 沙箱）和语言灵活性。

## 核心特性

- **WebAssembly 策略**: 策略编译为 Wasm 模块，在沙箱中安全运行
- **多语言支持**: Rust、Go、C#、Swift、Rego、TypeScript 等
- **OCI 分发**: 策略打包为 OCI 制品，通过标准镜像仓库分发
- **动态准入控制**: 支持 Mutating 和 Validating Webhook
- **审计模式**: monitor 模式只记录违规不阻止，安全渐进推广
- **上下文感知**: 策略可查询集群状态做出条件决策

## 架构

Kubewarden 由 PolicyServer 和 Controller 组成。PolicyServer 是策略运行时，以 Deployment 部署在集群中，内部维护一个 Wasm 运行时池。每个策略作为 Wasm 模块加载到 PolicyServer 中，当 Kubernetes API 请求到达时，PolicyServer 将 AdmissionReview 请求转发给对应的 Wasm 策略执行验证。Controller 监听 AdmissionPolicy / ClusterAdmissionPolicy CRD，将策略配置同步到 PolicyServer。kwctl 是 CLI 工具，用于策略开发、测试和拉取。

## Kubernetes 集成

Kubewarden 通过标准的 Kubernetes Admission Webhook 机制集成。Controller 自动注册 MutatingWebhookConfiguration 和 ValidatingWebhookConfiguration，将 API 请求路由到 PolicyServer。通过 AdmissionPolicy CRD（命名空间级）和 ClusterAdmissionPolicy CRD（集群级）声明式管理策略。策略模块通过 OCI 仓库 URL 引用，支持版本锁定。与 ArgoCD/FluxCD 集成实现 GitOps 策略管理。

## 生产使用场景

1. **镜像安全策略**: 强制所有 Pod 使用签名镜像或来自可信 Registry
2. **资源合规检查**: 确保 Pod 设置了 resource limits 和 securityContext
3. **命名空间治理**: 强制命名空间标签和 NetworkPolicy 策略
4. **自定义校验**: 使用 Rust/Go 编写复杂业务逻辑校验策略

## 安装

```bash
# Helm 安装
helm repo add kubewarden https://charts.kubewarden.io
helm install --wait kubewarden-crds kubewarden/kubewarden-crds
helm install --wait kubewarden-controller kubewarden/kubewarden-controller
# 应用策略
kubectl apply -f - <<EOF
apiVersion: policies.kubewarden.io/v1
kind: ClusterAdmissionPolicy
metadata: { name: safe-labels }
spec:
  module: registry://ghcr.io/kubewarden/policies/safe-labels:v1.0.0
  rules:
  - apiVersions: ["v1"]
    resources: ["pods"]
    operations: ["CREATE"]
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Kubewarden** | Wasm 隔离、多语言 | 社区较小 |
| OPA Gatekeeper | CNCF Graduated、Rego 生态 | 仅 Rego 语言、无沙箱 |
| Kyverno | YAML 原生策略、易使用 | 复杂逻辑表达能力有限 |
| Falco | 运行时安全 | 不是准入控制工具 |

## 架构定位

在 CNCF 生态中，Kubewarden 属于 **Policy** 类别，是准入控制策略引擎的第三代方案（Gatekeeper → Kyverno → Kubewarden），代表了 Wasm 在 K8s 策略执行中的应用方向。

## 参考链接

- [[实体/argocd.md|[[ArgoCD|argocd]]]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]

## Related

- [[实体/cncf-infrastructure.md|cncf-infrastructure]] — CNCF 基础设施与混沌工程项目全景
- [[实体/external-secrets.md|secrets]]]] — External Secrets Operator
- [[kube-burner]] — Kube-burner
- [[eraser]] — Eraser
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kubewarden
- [[实体/capsule.md|Capsule]]
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/security-index.md|Security 安全知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
