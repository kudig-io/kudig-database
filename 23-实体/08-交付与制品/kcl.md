---
title: KCL (Kusion Configuration Language)
description: '## 概述'
summary: 'KCL (Kusion Configuration Language) 是一个开源的基于约束的记录与函数式配置语言，专为云原生配置和策略管理设计。'
category: entities
tags:
- k8s
- cncf
- config
- kcl
- argocd
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KCL (Kusion Configuration Language) 是什么
- 如何 KCL (Kusion Configuration Language)
trigger_keywords:
- KCL
- Kusion
- Configuration
- Language
prerequisites:
- kubectl-basics
- gitops-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KCL (Kusion Configuration Language)

> **CNCF 状态**: Sandbox | **类别**: Config | **主要语言**: Rust, Go

## 概述

KCL（Kusion Configuration Language）是一个开源的基于约束的记录与函数式配置语言，由蚂蚁集团开发，2023 年加入 CNCF 沙箱。它专为云原生配置和策略管理设计，提供类型系统、schema 约束、配置合并和覆盖等能力，帮助团队以编程方式管理复杂的 Kubernetes 和云基础设施配置，减少配置错误。与 JSON/YAML 等纯静态配置相比，KCL 提供了类型检查、条件逻辑和模块化能力；与 HCL（Terraform）相比，KCL 的类型系统更严格，更适合定义复杂的嵌套配置。KCL 是 KusionStack 技术栈的核心配置语言，也可以独立使用。

## 核心能力

- **静态类型系统**: 强类型 + Schema 约束，在编译期捕获配置错误
- **配置合并**: 基于属性的覆盖和合并策略（Override/Merge）
- **模块化**: 支持包管理和导入（import），配置可复用
- **策略验证**: 内置验证规则（check），编写安全和合规策略
- **多目标输出**: 编译输出为 JSON/YAML，支持多环境差异化
- **IDE 支持**: VS Code 插件提供智能补全、类型检查和错误提示

## 架构

KCL 的核心是 Rust 实现的编译器：

- **KCL 编译器**: Rust 实现，包含词法分析、语法分析、类型检查、配置渲染
- **Schema**: 类型定义，声明字段类型、默认值、约束和验证规则
- **Module/Package**: KCL 文件组织成模块和包，支持 import
- **kpm (KCL Package Manager)**: KCL 包管理工具，类似 cargo/npm
- **KCL Plugin**: Go 插件系统，支持自定义函数（如 kubectl_query）
- **OCI Registry**: KCL 包可通过 OCI Registry 发布和分发

编译流程：`KCL 源码 → 编译器（类型检查 + 渲染）→ JSON/YAML 输出 → Kubernetes API`

## K8s 集成

KCL 通过 kubectl-kcl 插件或 kusion CLI 与 Kubernetes 集成。开发者编写 KCL 配置定义 Kubernetes 资源（Deployment、Service 等），通过 `kcl` 编译器渲染为 YAML，再用 `kubectl apply` 或 ArgoCD 部署。KCL 的 Schema 定义可以精确约束资源配置（如"replicas 必须为正整数"、"image 必须来自指定 Registry"）。与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的 GitOps 工作流集成——KCL 源码存储在 Git，编译输出由 ArgoCD 同步到集群。

## 生产场景

1. **平台配置模板**: 平台团队用 KCL Schema 定义标准应用模板，开发者填参数即可
2. **多环境配置**: 同一 KCL 配置通过 Overlay 生成 dev/staging/prod 差异化 YAML
3. **配置合规验证**: 在 CI 中运行 KCL check 验证配置是否满足安全和合规策略
4. **模型化 Kubernetes CRD**: 用 KCL Schema 对 CRD 进行类型化建模

## 安装与配置

```bash
# 安装 KCL CLI
curl -fsSL https://kcl-lang.io/script/install.sh | bash
# 或使用 Homebrew
brew install KusionStack/tap/kcl
kcl version

# 安装 VS Code 插件
# 在 VS Code 中搜索 "KCL" 安装

# 安装 kubectl 插件
kubectl krew install kcl
```

### KCL 配置示例

```python
# main.k - 类型安全的 K8s 配置
import k8s.api.apps.v1 as apps
import k8s.api.core.v1 as core

schema Server:
    name: str
    image: str
    replicas: int = 1
    port: int = 8080
    check:
        replicas > 0, "replicas must be positive"
        port > 0 and port < 65536, "invalid port"

server = Server {
    name = "my-app"
    image = "nginx:1.25"
    replicas = 3
    port = 8080
}

# 生成 Deployment
deployment = apps.Deployment {
    metadata.name = server.name
    spec = {
        replicas = server.replicas
        selector.matchLabels.app = server.name
        template = {
            metadata.labels.app = server.name
            spec.containers = [{
                name = server.name
                image = server.image
                ports = [{containerPort = server.port}]
            }]
        }
    }
}
```

```bash
# 编译输出 YAML
kcl main.k

# 与 kubectl 集成
kcl main.k | kubectl apply -f -
kubectl kcl -f main.k

# 配置验证
kcl vet main.k
```

## 运维操作

```bash
# 🟢 编译查看输出
kcl main.k

# 🟢 配置验证
kcl vet main.k

# 🟡 应用配置到集群
kcl main.k | kubectl apply -f -

# 🟡 使用多文件配置
kcl main.k base.k overlay.k

# 🟡 参数化编译
kcl main.k -D env=production -D replicas=5

# 🔴 删除配置对应的资源
kcl main.k | kubectl delete -f -
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 编译错误 | 语法/类型错误 | `kcl main.k` | 修复 KCL 语法 |
| check 失败 | 配置不满足约束 | `kcl vet main.k` | 调整配置值 |
| 输出 YAML 无效 | import 路径错误 | `kcl main.k -o out.yaml` | 检查 import 语句 |
| kubectl apply 失败 | 资源字段缺失 | `kubectl apply --dry-run=client` | 补充必填字段 |
| 插件加载失败 | 版本不兼容 | `kcl plugin list` | 更新 KCL 版本 |

```
排查流程:
├── 编译失败
│   ├── kcl main.k → 查看错误信息
│   ├── 检查 schema 类型定义
│   └── 确认 import 路径正确
├── 验证失败
│   ├── kcl vet main.k → 查看 check 规则
│   └── 调整配置值满足约束
└── 应用失败
    ├── kcl main.k | kubectl apply --dry-run=server -f -
    └── 检查集群 API 版本兼容性
```

## 生产案例

### 案例 1: 多环境配置统一管理

- **场景**: dev/staging/prod 三套环境 YAML 配置分散，修改容易遗漏
- **方案**: 使用 KCL schema 定义基础配置，通过 overlay 文件覆盖环境差异；CI 中 `kcl vet` 强制验证
- **效果**: 配置变更遗漏事故归零，新环境添加从 2h 缩短到 10min

### 案例 2: CI 配置合规门禁

- **场景**: 开发者提交的 YAML 缺少必填标签/资源限制，多次导致生产问题
- **方案**: KCL check 规则强制要求 labels、resources、replicas>1；CI 流水线集成 `kcl vet` 作为门禁
- **效果**: 不合规配置 100% 拦截，生产配置事故减少 95%

## 对比

| 特性 | KCL | HCL (Terraform) | Jsonnet | CUE | 适用场景 |
|------|-----|-----------------|---------|-----|----------|
| 类型系统 | ✅ 强类型 | ⚠️ 弱类型 | ✅ | ✅ | 安全性 |
| 配置合并 | ✅ | ⚠️ | ✅ | ✅ | 多环境 |
| 验证规则 | ✅ check | ❌ | ❌ | ✅ | 合规 |
| K8s 集成 | ✅ 原生 | ⚠️ | ⚠️ | ⚠️ | 云原生 |
| CNCF 状态 | Sandbox | 非 CNCF | 非 CNCF | 非 CNCF | 生态 |

## 架构定位

在 CNCF 生态中，KCL 属于 **Config** 类别，为云原生应用提供类型安全的可编程配置能力。

## 参考链接

- [[23-实体/argocd.md|[[argocd|argocd]]]]
- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[22-概念/09-平台与发布/gitops-principles.md|gitops-principles]]
- [[22-概念/05-安全/security-defense-depth.md|security-defense-depth]]

## Related

- [[parsec]] — Parsec
- [[opencost]] — OpenCost
- [[slimfaas]] — SlimFaas
- [[tuf]] — TUF
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kcl
- [[23-实体/08-交付与制品/kpt.md|kpt]]
- [[23-实体/08-交付与制品/cdk8s.md|cdk8s (Cloud Development Kit for Kubernetes)]]
- [[23-实体/15-参考与索引/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
