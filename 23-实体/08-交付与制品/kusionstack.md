---
title: KusionStack (entities)
description: '## 概述'
summary: 'KusionStack 是一个云原生可编程技术栈，提供以应用为中心的配置管理和交付能力。'
category: entities
tags:
- k8s
- cncf
- platform
- kusionstack
- containerd
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KusionStack 是什么
- 如何 KusionStack
trigger_keywords:
- KusionStack
prerequisites:
- kubectl-basics
- iac-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KusionStack

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go, KCL

## 概述

KusionStack 是一个云原生可编程技术栈，由蚂蚁集团（Ant Group）开源，2023 年加入 CNCF 沙箱。它提供以应用为中心的配置管理和交付能力，使用 KCL（Kusion Configuration Language）作为配置语言，结合 Kusion 引擎实现从应用配置到多云/多环境的一致性交付。KusionStack 支持 Kubernetes、Terraform 等多种 IaC 后端，让平台团队可以为开发者提供简化的自助式应用交付体验。其核心理念是"配置即代码"（Configuration as Code），通过 KCL 的类型系统和约束验证，在配置编写阶段捕获错误，而不是等到部署时才发现。

## 核心能力

- **KCL 配置语言**: 基于约束的记录与函数式配置语言，支持类型系统、schema 约束和配置合并
- **多云/多后端**: 支持 Kubernetes、Terraform、AWS、阿里云等多种基础设施后端
- **应用为中心**: 以 App 为单位组织配置，屏蔽底层基础设施复杂性
- **Konfig 仓库**: 可复用的配置模块仓库，支持团队间配置共享
- **Preview 审查**: apply 前自动 diff 变更，可视化展示影响范围
- **CI/CD 集成**: 与 ArgoCD/Flux 等 GitOps 工具无缝集成

## 架构

KusionStack 围绕 KCL 语言和 Kusion 引擎构建：

- **KCL 编译器**: Rust 实现的 KCL 语言编译器，解析、类型检查并渲染配置
- **Konfig 仓库**: 按项目（Project）、栈（Stack）组织的配置模块层次结构
- **Kusion 引擎**: 执行配置渲染、状态管理和资源编排
- **State Backend**: 存储资源配置状态（支持本地/远程 state），支持 diff 和收敛
- **Executor**: 通过 Kubernetes API、Terraform Provider 或云 SDK 执行实际资源操作
- **KCL OCI Registry**: 将配置模块打包为 OCI 制品，版本化分发

交付流程：`KCL 配置 → 编译 → 资源 Spec → Preview (diff) → Apply → K8s/Cloud`

## K8s 集成

KusionStack 通过 Kusion 引擎与 Kubernetes 集成。KCL 配置编译后生成标准 Kubernetes 资源 YAML，通过 Kubernetes API 直接 apply。Kusion 引擎管理资源状态，支持三向 diff（配置 vs State vs 集群）。通过 `kusion preview` 可以在应用前检查变更，`kusion apply` 执行实际部署。KusionStack 支持与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 ArgoCD 集成——KCL 渲染输出可以作为 ArgoCD Application 的 source。

## 生产场景

1. **企业平台工程**: 平台团队用 KCL 定义标准化应用模板，开发者填写参数即可部署
2. **多云统一交付**: 同一 KCL 配置同时管理 Kubernetes 资源和云基础设施（RDS、SLB）
3. **多环境管理**: dev/staging/prod 共享基础配置，通过 KCL overlay 实现差异化
4. **配置合规**: 利用 KCL schema 约束在编写阶段拦截不安全/不合规的配置

## 安装与配置

```bash
# 安装 Kusion CLI
curl -fsSL https://www.kusionstack.io/scripts/install.sh | bash
# 或使用 Homebrew
brew install KusionStack/tap/kusion

# 安装 KCL CLI
brew install KusionStack/tap/kcl

# 初始化项目
mkdir my-platform && cd my-platform
kusion init

# 编译配置并预览
kcl run .
kusion preview

# 部署
kusion apply --yes
```

```yaml
# KCL 配置示例（应用模板）
# app.k
import models

app: models.App {
    name = "payment-service"
    replicas = 3
    image = "registry.company.com/payment:v2.1.0"
    resources = {
        cpu = "2"
        memory = "4Gi"
    }
    ports = [{port = 8080, protocol = "TCP"}]
    env = [
        {name = "DB_HOST", value = "pg-primary.database.svc"}
        {name = "REDIS_URL", value = "redis://cache.svc:6379"}
    ]
    readinessProbe = {
        httpGet = {path = "/health", port = 8080}
        initialDelaySeconds = 10
        periodSeconds = 5
    }
}
---
# project.yaml（项目配置）
apiVersion: kusionstack.io/v1
kind: Project
metadata:
  name: payment-platform
spec:
  stacks:
    - name: dev
      path: ./stacks/dev
    - name: prod
      path: ./stacks/prod
---
# stack.yaml（环境差异化）
apiVersion: kusionstack.io/v1
kind: Stack
metadata:
  name: prod
spec:
  desiredState:
    app:
      replicas: 5
      resources:
        cpu: "4"
        memory: "8Gi"
```

## 运维操作

```bash
# 🟢 预览变更（不实际执行）
kusion preview

# 🟡 应用配置变更
kusion apply --yes

# 🟡 回滚到上一个版本
kusion rollback

# 🟢 查看当前状态
kusion show

# 🟢 编译 KCL 配置（检查语法和类型）
kcl run . --format json

# 🟡 销毁资源（谨慎操作）
kusion destroy --yes

# 🟢 查看 KCL 模块依赖
kcl mod metadata
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| kusion apply 失败 | K8s API 不可达或权限不足 | `kusion apply --verbose` | 检查 kubeconfig 和 RBAC |
| KCL 编译错误 | Schema 约束不满足或类型错误 | `kcl run . 2>&1` | 修复 KCL 语法/类型错误 |
| 状态不一致 | 手动修改了集群资源 | `kusion preview` 查看 diff | `kusion apply` 收敛状态 |
| 模块拉取失败 | OCI Registry 不可达或认证失败 | `kcl mod pull <module>` | 检查网络和 Registry 凭据 |
| 多环境配置冲突 | overlay 覆盖顺序错误 | `kcl run ./stacks/prod` | 检查 stack 配置层次结构 |

```
排查流程：
├── 配置编译失败
│   ├── kcl run . 查看详细错误信息
│   ├── 检查 schema 约束是否满足
│   ├── 确认 import 路径正确
│   └── kcl lint . 检查代码风格
├── 部署失败
│   ├── kusion preview 检查生成的 YAML
│   ├── 确认 kubeconfig 指向正确集群
│   ├── 检查 RBAC 权限
│   └── 查看 kusion state 状态文件
└── 状态漂移
    ├── kusion preview 查看 diff
    ├── 确认是否有人手动修改了资源
    └── kusion apply 收敛到期望状态
```

## 生产案例

### 案例 1：企业平台工程自助式交付

- **场景**：平台团队管理 200+ 微服务，开发者需要填写工单等待平台团队创建 K8s 资源，交付周期 3-5 天
- **排查**：平台团队成为瓶颈，YAML 配置散落各处，环境不一致问题频发
- **方案**：用 KCL 定义标准化应用模板（schema 约束），开发者只填参数，kusion apply 自动交付
- **效果**：应用交付从 3-5 天缩短到 10 分钟，配置错误减少 95%，平台团队从执行者转为模板维护者

### 案例 2：多云统一配置管理

- **场景**：业务同时运行在阿里云和 AWS，K8s 资源和云基础设施（RDS/SLB）需要统一管理
- **排查**：Terraform 管理云资源、kubectl 管理 K8s，两套工具链状态不同步，变更容易遗漏
- **方案**：KusionStack 统一管理 K8s + Terraform 后端，一个 KCL 配置同时生成两种资源
- **效果**：变更一致性从 70% 提升至 99%，多云环境配置管理时间减少 60%

## 对比

| 特性 | KusionStack | Crossplane | Terraform CDK | Pulumi | 适用场景 |
|------|-------------|------------|---------------|--------|----------|
| 配置语言 | KCL | YAML/CRD | TS/Python | TS/Go/Python | 团队技能匹配 |
| K8s 原生 | ✅ | ✅ | ❌ | ❌ | 云原生优先 |
| 类型约束 | ✅ Schema | ❌ | ⚠️ | ✅ | 配置安全 |
| 多后端 | ✅ | ✅ | ✅ | ✅ | 多云管理 |
| 学习曲线 | 低（KCL 简洁） | 中 | 高 | 中 | 快速上手 |

## 架构定位

在 CNCF 生态中，KusionStack 属于 **Platform** 类别，为云原生应用提供可编程配置和交付能力。

## 参考链接

- [[crossplane]]
- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[22-概念/09-平台与发布/gitops-principles.md|gitops-principles]]
- [[22-概念/04-存储/storage-model.md|storage-model]]

## Related

- [[05-containerd-observability]] — containerd 可观测性
- [[stacker]] — Stacker
- [[opentelemetry]] — OpenTelemetry
- [[kcl]] — KCL (Kusion Configuration Language)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kusionstack
- [[23-实体/15-参考与索引/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
