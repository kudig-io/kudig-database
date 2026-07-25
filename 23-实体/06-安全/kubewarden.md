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

## 安装与配置

```bash
# Helm 安装
helm repo add kubewarden https://charts.kubewarden.io
helm install --wait kubewarden-crds kubewarden/kubewarden-crds
helm install --wait kubewarden-controller kubewarden/kubewarden-controller
helm install --wait kubewarden-defaults kubewarden/kubewarden-defaults
```

### ClusterAdmissionPolicy 配置示例

```yaml
apiVersion: policies.kubewarden.io/v1
kind: ClusterAdmissionPolicy
metadata:
  name: safe-labels
spec:
  module: registry://ghcr.io/kubewarden/policies/safe-labels:v1.0.0
  settings:
    mandatory_labels:
      - team
      - environment
    denied_labels:
      - owner
  rules:
  - apiGroups: ["", "apps"]
    apiVersions: ["v1"]
    resources: ["pods", "deployments"]
    operations: ["CREATE", "UPDATE"]
  mutating: false
  backgroundAudit: true
---
apiVersion: policies.kubewarden.io/v1
kind: ClusterAdmissionPolicy
metadata:
  name: pod-privileged
spec:
  module: registry://ghcr.io/kubewarden/policies/pod-privileged:v0.3.2
  rules:
  - apiGroups: [""]
    apiVersions: ["v1"]
    resources: ["pods"]
    operations: ["CREATE"]
  mutating: false
```

## 运维操作

```bash
# 🟢 查看策略状态
kubectl get clusteradmissionpolicy
kubectl get admissionpolicy -A

# 🟢 查看 PolicyServer 状态
kubectl get policyserver -n kubewarden
kubectl get pods -n kubewarden

# 🟢 查看策略审计结果
kubectl get policyreport -A
kubectl get clusterpolicyreport

# 🟡 测试策略（本地）
kwctl run registry://ghcr.io/kubewarden/policies/safe-labels:v1.0.0 \
  --request-path admission_request.json

# 🟡 拉取策略到本地
kwctl pull registry://ghcr.io/kubewarden/policies/safe-labels:v1.0.0

# 🟢 查看策略详情
kwctl inspect registry://ghcr.io/kubewarden/policies/safe-labels:v1.0.0

# 🟡 切换策略模式（monitor/enforce）
kubectl patch clusteradmissionpolicy safe-labels \
  --type=merge -p '{"spec":{"mode":"monitor"}}'
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 策略未生效 | PolicyServer 未就绪 | `kubectl get policyserver` | 检查 PolicyServer Pod 状态 |
| Wasm 加载失败 | 模块不兼容 | 查看 PolicyServer 日志 | 确认 Wasm 模块架构和版本 |
| 误拦截正常请求 | 策略规则过严 | 切换到 monitor 模式 | 调整策略配置 |
| 策略拉取失败 | Registry 认证问题 | 检查 imagePullSecrets | 配置 Registry 凭证 |
| 审计无结果 | backgroundAudit 未启用 | 检查策略 spec | 启用 backgroundAudit: true |

## 生产案例

### 案例1: 多语言策略开发

**场景**: 需要复杂的业务逻辑校验（检查内部 CMDB 一致性）  
**方案**: 使用 Rust 编写策略，调用外部 API 验证，编译为 Wasm  
**效果**: 策略执行延迟 < 5ms，复杂逻辑表达无限制  

### 案例2: 渐进式策略推广

**场景**: 新策略可能影响现有工作负载，需要安全推广  
**方案**: 先 monitor 模式运行 2 周，分析审计报告，再切换 enforce  
**效果**: 零业务中断完成策略上线  

## 替代方案

| 项目 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| **Kubewarden** | Wasm 隔离、多语言 | 社区较小 | 复杂自定义策略 |
| OPA Gatekeeper | CNCF Graduated、Rego | 仅 Rego、无沙箱 | 通用策略 |
| Kyverno | YAML 原生、易用 | 复杂逻辑有限 | 简单策略 |
| ValidatingAdmissionPolicy | K8s 原生 CEL | 功能有限 | 轻量级验证 |

## 架构定位

在 CNCF 生态中，Kubewarden 属于 **Policy** 类别，是准入控制策略引擎的第三代方案（Gatekeeper → Kyverno → Kubewarden），代表了 Wasm 在 K8s 策略执行中的应用方向。

## 检查清单

- [ ] 新策略先使用 monitor 模式验证
- [ ] 策略模块通过 OCI Registry 分发并签名
- [ ] 配置 backgroundAudit 持续审计
- [ ] 策略变更纳入 GitOps 管理
- [ ] 监控 PolicyServer 资源和延迟
- [ ] 配置策略测试在 CI 中运行

## 参考链接

- [[23-实体/08-交付与制品/argocd.md|argocd]]
- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
- [[pod-lifecycle]]

## Related

- [[23-实体/15-参考与索引/cncf-infrastructure.md|cncf-infrastructure]] — CNCF 基础设施与混沌工程项目全景
- [[kube-burner]] — Kube-burner
- [[eraser]] — Eraser
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[23-实体/09-编排调度/capsule.md|Capsule]]
- [[23-实体/15-参考与索引/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
