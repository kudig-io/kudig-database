---
title: OpenTofu (entities)
description: '## 概述'
summary: 'OpenTofu 是 Terraform 的开源分支，在 Terraform 转向 BSL 许可后由社区创建。'
category: entities
tags:
- k8s
- cncf
- config
- opentofu
- containerd
- harbor
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenTofu 是什么
- 如何 OpenTofu
trigger_keywords:
- OpenTofu
prerequisites:
- kubectl-basics
- iac-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# OpenTofu

> **CNCF 状态**: Sandbox | **类别**: Config/IaC | **主要语言**: Go

## 概述

OpenTofu 是 Terraform 的开源分支，由 Linux Foundation 托管，2023 年在 Terraform 从 MPL-2.0 转向 BSL（Business Source License）后由社区创建。它是一个基础设施即代码（IaC）工具，允许使用声明式 HCL（HashiCorp Configuration Language）定义和管理云资源。OpenTofu 完全兼容 Terraform 1.5.x 及更早版本的配置文件和 Provider 生态，实现了无缝迁移。作为完全社区驱动的开源项目，OpenTofu 得到了 Linux Foundation、Spacelift、env0、Scalr 等公司的支持。OpenTofu 保持 MPL-2.0 许可证，确保工具永久保持开源和免费。2024 年加入 CNCF 沙箱。

## 核心能力

- **完全兼容**: 与 Terraform 1.5.x 配置 100% 兼容，支持所有 Terraform Providers
- **MPL-2.0 开源许可**: 真正的开源许可证，永久免费可用
- **声明式 IaC**: 使用 HCL 声明式语言定义基础设施期望状态
- **状态管理**: 支持本地、S3、Consul、HTTP 等多种 state backend
- **模块系统**: 可复用的基础设施模块，支持版本化和共享
- **Provider 生态**: 兼容所有 Terraform Registry 中的 Providers（AWS、Azure、K8s 等）
- **社区驱动**: Linux Foundation 托管，完全社区治理和透明决策

## 架构

OpenTofu 遵循 Terraform 经典架构：

- **OpenTofu CLI**: 核心命令行工具，执行 init/plan/apply/destroy 操作
- **HCL 配置**: `.tf` 文件，定义 Provider、Resource、Variable、Output
- **State File**: `terraform.tfstate`，记录当前基础设施的实际状态
- **Provider Plugin**: 通过 gRPC 与云厂商 API 交互的插件（AWS、K8s、 Helm 等）
- **Module**: 可复用的配置包，通过 source 参数引用
- **Provisioner**: 资源创建后的配置脚本（如远程 SSH 执行命令）

执行流程：`init (加载 Provider) → plan (diff) → apply (变更) → state 更新`

## K8s 集成

OpenTofu 通过 Kubernetes Provider 与 Kubernetes 集群集成。可以在 HCL 配置中直接声明 Kubernetes 资源（Deployment、Service、Namespace 等），OpenTofu 自动管理资源的创建、更新和删除。通过 Helm Provider 可以管理 Helm Release，通过 Kustomize Provider 管理 Kustomize 应用。state 文件存储在 Kubernetes Secret 或 S3 后端。在 GitOps 场景中，OpenTofu 可以运行在 CI/CD 流水线中，从 Git 拉取配置并 apply 到 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 的集群中。

## 生产场景

1. **云基础设施管理**: 使用 OpenTofu 统一管理 AWS/GCP/阿里云的 VPC、EKS、RDS 等资源
2. **Kubernetes 资源管理**: 通过 K8s Provider 声明式管理集群中的 CRD 和原生资源
3. **多环境基础设施**: 通过 module 和 tfvars 实现 dev/staging/prod 的基础设施差异管理
4. **GitOps IaC**: 在 CI/CD 中运行 OpenTofu plan/apply，实现基础设施的版本控制和自动化

## 安装与配置

```bash
# 安装 OpenTofu
curl -fsSL https://get.opentofu.org/install-opentofu.sh | bash
# 或使用 Homebrew
brew install opentofu

# 初始化项目
tofu init

# 编写基础设施配置
cat > main.tf <<'HCL'
terraform {
  required_providers {
    kubernetes = {
      source = "hashicorp/kubernetes"
      version = "~> 2.23"
    }
  }
}

provider "kubernetes" {
  config_path = "~/.kube/config"
}

resource "kubernetes_namespace" "example" {
  metadata {
    name = "my-app"
  }
}

resource "kubernetes_deployment" "example" {
  metadata {
    name = "nginx"
    namespace = kubernetes_namespace.example.metadata[0].name
  }
  spec {
    replicas = 3
    selector {
      match_labels = { app = "nginx" }
    }
    template {
      metadata { labels = { app = "nginx" } }
      spec {
        container {
          image = "nginx:latest"
          name  = "nginx"
        }
      }
    }
  }
}
HCL

# 预览变更
tofu plan

# 应用变更
tofu apply
```

### 远程状态后端

```hcl
# backend.tf - S3 后端
terraform {
  backend "s3" {
    bucket         = "opentofu-state"
    key            = "production/k8s/terraform.tfstate"
    region         = "us-east-1"
    dynamodb_table = "opentofu-locks"
    encrypt        = true
  }
}

# 或使用 Kubernetes Secret 后端
terraform {
  backend "kubernetes" {
    secret_suffix    = "opentofu-state"
    namespace        = "opentofu-system"
    in_cluster_config = true
  }
}
```

## 运维操作

```bash
# 🟢 查看当前状态
tofu show
tofu state list
tofu output

# 🟢 预览变更（不执行）
tofu plan -out=plan.tfplan
tofu show -json plan.tfplan | jq '.resource_changes[]'

# 🟡 应用变更
tofu apply plan.tfplan
tofu apply -target=kubernetes_deployment.example

# 🟡 导入已有资源
tofu import kubernetes_namespace.example my-app

# 🟡 移除状态中的资源（不删除实际资源）
tofu state rm kubernetes_deployment.example

# 🔴 销毁所有资源
tofu destroy

# 🔴 强制解锁状态
tofu force-unlock <lock-id>
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| State lock 冲突 | 并发执行/上次崩溃 | `tofu force-unlock <id>` | 确认无并发后解锁 |
| Provider 下载失败 | 网络/Registry 不可达 | `tofu init -upgrade` | 配置镜像源/代理 |
| Plan 显示意外删除 | 状态与实际不一致 | `tofu refresh` | 先 refresh 再 plan |
| Apply 超时 | API Server 响应慢 | `kubectl get --raw /healthz` | 检查集群健康 |
| 状态文件损坏 | 存储后端异常 | `tofu state pull > backup.json` | 从备份恢复 |

```
排查流程:
├── 初始化失败
│   ├── tofu init -upgrade → 重新下载 Provider
│   ├── 检查网络/代理 → Registry 可达性
│   └── 检查 .terraform.lock.hcl → 版本锁定
├── 状态异常
│   ├── tofu state list → 当前状态资源
│   ├── tofu refresh → 同步实际状态
│   └── tofu state pull → 导出状态备份
└── Apply 失败
    ├── tofu plan -detailed-exitcode → 变更预览
    ├── kubectl get events → K8s 事件
    └── 检查 RBAC → Provider 权限
```

## 生产案例

### 案例1: 状态文件并发写入导致损坏

- **场景**: 两个 CI Job 同时执行 `tofu apply`，状态文件写入冲突导致 JSON 损坏
- **排查**: `tofu plan` 报错 "state file is corrupt"，无法解析
- **方案**:
  1. 从 S3 版本控制恢复上一版状态文件
  2. 配置 DynamoDB 状态锁防止并发
  3. CI/CD 添加 `concurrency` 限制确保串行执行
- **效果**: 状态损坏事故归零，并发安全得到保障

### 案例2: 从 Terraform 迁移至 OpenTofu

- **场景**: Terraform BSL 许可证变更，公司要求迁移至开源替代方案
- **排查**: 评估 200+ 模块的兼容性，确认 Provider 生态支持
- **方案**:
  1. `tofu init` 直接读取现有 .tf 文件和状态
  2. CI/CD 中 `terraform` 命令替换为 `tofu`
  3. 状态后端保持不变（S3 + DynamoDB）
- **效果**: 200+ 模块 2 周内完成迁移，零功能损失

## 对比

| 特性 | OpenTofu | Terraform | Pulumi | Crossplane |
|------|----------|-----------|--------|------------|
| 许可证 | MPL-2.0 | BSL | Apache-2.0 | Apache-2.0 |
| 配置语言 | HCL | HCL | TS/Go/Python | YAML/CRD |
| K8s 原生 | ❌ CLI | ❌ CLI | ❌ CLI | ✅ Controller |
| CNCF 状态 | Sandbox | 非 CNCF | 非 CNCF | Incubating |

## 架构定位

在 CNCF 生态中，OpenTofu 属于 **Config/IaC** 类别，为云原生应用提供开源基础设施即代码能力。

## 参考链接

- [[22-概念/05-安全/secrets-management.md|secrets-management]]
- [[22-概念/09-平台与发布/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[kubeedge]] — KubeEdge
- [[telepresence]] — Telepresence
- [[08-containerd-multi-tenant]] — containerd 多租户
- [[harbor]] — Harbor
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- opentofu
- [[23-实体/08-交付与制品/cdk8s.md|cdk8s (Cloud Development Kit for Kubernetes)]]
- [[23-实体/15-参考与索引/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
