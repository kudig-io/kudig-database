---
title: Domain-24 基础设施即代码 — 开源项目索引
description: '- [五、Ansible 与配置管理](#五ansible-与配置管理)'
category: infrastructure-as-code
tags:
- k8s
- iac
- terraform
- pulumi
- helm
- flux
- mysql
- crd
- rag
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- SRE
- DevOps 工程师
estimated_read_time: 5min
intent_queries:
- Domain-24 基础设施即代码 — 开源项目索引 是什么
- 如何 Domain-24 基础设施即代码 — 开源项目索引
- Kubernetes 24 infrastructure as code 最佳实践
trigger_keywords:
- Domain-24
- 基础设施即代码
- 开源项目索引
- infrastructure
- as
- code
prerequisites:
- kubectl-basics
- gitops-basics
- helm-basics
- iac-basics
- mysql-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# Domain-24 基础设施即代码 — 开源项目索引

> **最后更新**: 2026-04-24  
> **适用版本**: Terraform v1.11 / Crossplane v1.19 / Pulumi v3.160

---

## 📋 目录

- [一、核心项目总览](#一核心项目总览)
- [二、Terraform & OpenTofu](#二terraform--opentofu)
- [三、Crossplane (CNCF Graduated)](#三crossplane-cncf-graduated)
- [四、Pulumi](#四pulumi)
- [五、Ansible 与配置管理](#五ansible-与配置管理)
- [六、云厂商原生工具](#六云厂商原生工具)
- [七、版本与兼容矩阵](#七版本与兼容矩阵)
- [八、IaC 选型指南](#八iac-选型指南)

---

## 一、核心项目总览

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Terraform** | 多云 IaC 标准 | HashiCorp | v1.11.0 | 44k+ | BSL/Apache-2.0 |
| **OpenTofu** | Terraform 开源分叉 | Linux 基金会 | v1.10.0 | 25k+ | MPL-2.0 |
| **Crossplane** | K8s 原生 IaC | CNCF Graduated | v1.19.0 | 10k+ | Apache-2.0 |
| **Pulumi** | 编程式 IaC | Pulumi | v3.160.0 | 22k+ | Apache-2.0 |
| **Ansible** | 配置管理与编排 | Red Hat | v2.18.0 | 63k+ | GPL-3.0 |
| **AWS CDK** | AWS 云开发套件 | AWS | v2.188.0 | 12k+ | Apache-2.0 |
| **Azure Bicep / ARM** | Azure 原生 IaC | Microsoft | v0.34.0 | 3k+ | MIT |
| **Google Config Controller** | GCP 原生 K8s 控制 | Google | - | - | 商业 |
| **CDK for Terraform (CDKTF)** | 编程式 Terraform | HashiCorp | v0.20.0 | 5k+ | MPL-2.0 |
| **Terragrunt** | Terraform 封装工具 | Gruntwork | v0.77.0 | 8k+ | MIT |
| **Checkov** | IaC 安全扫描 | Bridgecrew | v3.2.0 | 7k+ | Apache-2.0 |
| **Terraform-docs** | 自动生成文档 | 社区 | v0.19.0 | 4k+ | MIT |

---

## 二、Terraform & OpenTofu

### 2.1 Terraform

```yaml
# 核心概念
- Providers: 云厂商插件 (AWS/Azure/GCP/K8s)
- Resources: 基础设施对象
- State: 状态文件 (本地/S3/Consul/Terraform Cloud)
- Modules: 可复用组件
- Workspaces: 环境隔离
```

**License 变更**
- v1.5.x 及之前: MPL-2.0 (开源)
- v1.6+ : BSL (Business Source License, 生产使用受限)
- **建议**: 新项目评估 OpenTofu 或 Pulumi

### 2.2 OpenTofu

- **来源**: Linux 基金会 2023 年从 Terraform v1.5.x 分叉
- **目标**: 保持 MPL-2.0 开源，社区驱动
- **兼容性**: 兼容 Terraform v1.5.x 语法与模块
- **Registry**: OpenTofu Registry (opentofu.org)

```bash
# 安装 OpenTofu
curl -fsSL https://get.opentofu.org/install-opentofu.sh | sh

# 使用 (与 Terraform 命令一致)
tofu init
tofu plan
tofu apply
```

**GitHub**: https://github.com/opentofu/opentofu

---

## 三、Crossplane (CNCF Graduated)

> **2025.11 新晋 CNCF Graduated 项目**

### 3.1 K8s 原生 IaC

```yaml
# 核心理念
- 所有云资源 = K8s CRD
- 平台工程师构建平台 API (CompositeResourceDefinition)
- 开发者使用简化抽象 (Claim)
- GitOps 原生 (与 Argo CD/Flux 无缝集成)
```

### 3.2 架构组件

| 组件 | 作用 |
|:---|:---|
| Provider | 云厂商资源提供者 (AWS/GCP/Azure/Helm/K8s) |
| Managed Resource (MR) | 底层云资源的 K8s 表示 |
| CompositeResourceDefinition (XRD) | 平台 API 模式定义 |
| Composition | XRD → MR 的映射逻辑 |
| Claim (XR) | 开发者提交的简化请求 |

### 3.3 示例

```yaml
# 平台工程师定义平台 API (XRD)
apiVersion: apiextensions.crossplane.io/v1
kind: CompositeResourceDefinition
metadata:
  name: xdatabases.example.org
spec:
  group: example.org
  names:
    kind: XDatabase
    plural: xdatabases
  versions:
  - name: v1alpha1
    schema:
      openAPIV3Schema:
        type: object
        properties:
          spec:
            type: object
            properties:
              region:
                type: string
              engine:
                type: string
                enum: ["postgres", "mysql"]
---
# 开发者使用
apiVersion: example.org/v1alpha1
kind: Database
metadata:
  name: mydb
  namespace: dev
spec:
  region: us-east-1
  engine: postgres
```

**GitHub**: https://github.com/crossplane/crossplane
**文档**: https://docs.crossplane.io/

---

## 四、Pulumi

### 4.1 编程式 IaC

```yaml
# 支持语言
- TypeScript/JavaScript
- Python
- Go
- C#
- Java
- YAML (Pulumi YAML)
```

### 4.2 与 Terraform 对比

| 维度 | Pulumi | Terraform |
|:---|:---|:---|
| 语法 | 编程语言 | HCL |
| 抽象能力 | 强 (循环、条件、函数、类) | 中等 (HCL 2 改进) |
| 状态管理 | Pulumi Service / S3 / 本地 | 本地 / S3 / Terraform Cloud |
| 生态 | 丰富的 Provider (含 Terraform Bridge) | 最庞大的 Provider 生态 |
| 测试 | 单元测试、集成测试 | terraform test (有限) |
| 团队协作 | 强 (Policy as Code, Environments) | 依赖外部工具 |

**GitHub**: https://github.com/pulumi/pulumi
**文档**: https://www.pulumi.com/docs/

---

## 五、Ansible 与配置管理

### 5.1 Ansible

- 无代理 (SSH/WinRM)
- 声明式 Playbook
- 丰富的模块生态
- Ansible Tower / AWX 作为控制平面

### 5.2 Ansible + K8s

```yaml
- name: Deploy to K8s
  kubernetes.core.k8s:
    state: present
    definition:
      apiVersion: apps/v1
      kind: Deployment
      metadata:
        name: myapp
      spec:
        replicas: 3
```

---

## 六、云厂商原生工具

| 工具 | 云厂商 | 特点 |
|:---|:---|:---|
| AWS CDK | AWS | 编程式 (TypeScript/Python/Java/Go/C#)，L1/L2/L3 构造层 |
| Azure Bicep | Azure | ARM 模板的 DSL 简化版 |
| Azure Resource Manager | Azure | JSON 模板，原生集成 |
| Google Cloud Deployment Manager | GCP | YAML/Python 模板 |
| Config Connector | GCP | K8s 原生 GCP 资源管理 (类似 Crossplane) |

---

## 七、版本与兼容矩阵

| 组件 | K8s v1.31 | v1.32 | v1.33 | 备注 |
|:---|:---|:---|:---|:---|
| Terraform v1.11 | ✅ | ✅ | ✅ | Provider 独立更新 |
| OpenTofu v1.10 | ✅ | ✅ | ✅ | 兼容 TF v1.5 |
| Crossplane v1.19 | ✅ | ✅ | ✅ | Provider 独立更新 |
| Pulumi v3.160 | ✅ | ✅ | ✅ | 通过 Provider 兼容 |
| Ansible v2.18 | ✅ | ✅ | ✅ | kubernetes.core 集合 |
| AWS CDK v2.188 | ✅ | ✅ | ✅ | EKS 构造库 |

---

## 八、IaC 选型指南

```
┌─────────────────────────────────────────────────────────────┐
│                    IaC 技术选型决策树                          │
└─────────────────────────────────────────────────────────────┘

1. 团队熟悉编程语言 (Go/Python/TS) 且需要复杂逻辑?
   └─ Yes ──► Pulumi 或 CDK
   └─ No  ──► 继续...

2. 已在 K8s 上全面 GitOps?
   └─ Yes ──► Crossplane (K8s 原生资源管理)
   └─ No  ──► 继续...

3. 多云/混合云统一管理层?
   └─ Yes ──► Terraform / OpenTofu / Crossplane
   └─ No  ──► 云厂商原生工具 (CDK/Bicep)

4. 关注开源 License 纯粹性?
   └─ Yes ──► OpenTofu / Pulumi / Crossplane
   └─ No  ──► Terraform (生态最成熟)

5. 需要配置管理 + 编排混合?
   └─ Yes ──► Ansible + Terraform 组合
   └─ No  ──► 纯 IaC 工具

6. 平台工程/内部开发者平台?
   └─ Yes ──► Crossplane (抽象层 + 自助服务)
   └─ No  ──► Terraform / Pulumi

7. 已有大量 Terraform 模块投资?
   └─ Yes ──► OpenTofu (直接迁移) 或 Pulumi Terraform Bridge
   └─ No  ──► 自由选择
```

---

## 参考链接

- [Terraform 官方文档](https://developer.hashicorp.com/terraform/docs)
- [OpenTofu 官方文档](https://opentofu.org/docs/)
- [Crossplane 官方文档](https://docs.crossplane.io/)
- [Pulumi 官方文档](https://www.pulumi.com/docs/)
- [Ansible 官方文档](https://docs.ansible.com/)
- [AWS CDK 文档](https://docs.aws.amazon.com/cdk/)

---

## Obsidian 相关文档

- [[domain-08-release-change-management/MOC.md|domain-24-infrastructure-as-code MOC]]
- [[domain-08-release-change-management/README.md|Domain 24: 基础设施即代码 (Infrastructure as Code)]]
- [[domain-08-release-change-management/01-terraform-enterprise-iac.md|Terraform企业级基础设施即代码实践]]
- [[domain-08-release-change-management/02-ansible-enterprise-automation.md|Ansible企业级自动化运维深度实践]]
- [[domain-08-release-change-management/03-pulumi-enterprise-iac.md|Pulumi Enterprise Infrastructure as Code Platform]]
- [[domain-08-release-change-management/04-azure-resource-manager-enterprise.md|Azure Resource Manager (ARM) Enterprise 深度实践]]
- [[domain-08-release-change-management/05-crossplane-enterprise-orchestration.md|Crossplane Enterprise Infrastructure Orchestration 深度实践]]
- [[domain-08-release-change-management/99-crossplane-platform-guide.md|Crossplane 平台工程实践指南]]
