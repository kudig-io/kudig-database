---
title: OpenTofu
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- job
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OpenTofu 是什么
- 如何 OpenTofu
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- OpenTofu
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- iac-basics
---

title: OpenTofu
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- job
- gateway
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- OpenTofu 是什么
- 如何 OpenTofu
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- OpenTofu
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# OpenTofu

> **成熟度**: Sandbox | **加入时间**: 2023-09 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://opentofu.org |
| **GitHub** | https://github.com/opentofu/opentofu |
| **许可证** | MPL-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Provisioning & IaC |

---

## 项目概述

OpenTofu 是 Terraform 的开源分支，在 Terraform 转向 BSL 许可后由社区创建。它是一个基础设施即代码 (IaC) 工具，允许使用声明式配置语言定义和管理云资源。

## 核心特性

- **完全兼容**: 与 Terraform 1.5.x 配置兼容
- **开源许可**: MPL-2.0 开源许可证
- **状态管理**: 支持本地和远程状态后端
- **模块系统**: 可复用的基础设施模块
- **Provider 生态**: 兼容所有 Terraform Providers
- **社区驱动**: Linux Foundation 托管，社区治理

---

## 与 Terraform 对比

| 特性 | OpenTofu | Terraform |
|------|----------|-----------|
| 许可证 | MPL-2.0 | BSL 1.1 |
| 治理 | Linux Foundation | HashiCorp |
| 兼容性 | 1.5.x 兼容 | N/A |
| Provider | 共享生态 | 共享生态 |

---

## 快速开始

### 安装

```bash
# macOS
brew install opentofu

# Linux (Debian/Ubuntu)
curl -fsSL https://get.opentofu.org/install-opentofu.sh | sh

# 或使用包管理器
curl -fsSL https://packages.opentofu.org/opentofu/tofu/gpgkey | sudo gpg --dearmor -o /etc/apt/keyrings/opentofu.gpg
echo "deb [signed-by=/etc/apt/keyrings/opentofu.gpg] https://packages.opentofu.org/opentofu/tofu/any any main" | sudo tee /etc/apt/sources.list.d/opentofu.list
sudo apt update && sudo apt install tofu

# 验证安装
tofu version
```

### 基本配置

```hcl
# main.tf
terraform {
  required_providers {
    aws = {
      source  = "hashicorp/aws"
      version = "~> 5.0"
    }
  }
}

provider "aws" {
  region = "us-east-1"
}

resource "aws_instance" "web" {
  ami           = "ami-0c55b159cbfafe1f0"
  instance_type = "t3.micro"
  
  tags = {
    Name = "web-server"
  }
}

output "instance_ip" {
  value = aws_instance.web.public_ip
}
```

### 工作流

```bash
# 初始化
tofu init

# 预览变更
tofu plan

# 应用变更
tofu apply

# 查看状态
tofu show

# 销毁资源
tofu destroy
```

---

## 状态管理

### S3 远程后端

```hcl
terraform {
  backend "s3" {
    bucket         = "my-tofu-state"
    key            = "prod/terraform.tfstate"
    region         = "us-east-1"
    encrypt        = true
    dynamodb_table = "tofu-locks"
  }
}
```

### Kubernetes 后端

```hcl
terraform {
  backend "kubernetes" {
    secret_suffix    = "state"
    config_path      = "~/.kube/config"
    namespace        = "tofu-state"
  }
}
```

---

## 模块使用

### 使用公共模块

```hcl
module "vpc" {
  source  = "terraform-aws-modules/vpc/aws"
  version = "5.0.0"

  name = "my-vpc"
  cidr = "10.0.0.0/16"

  azs             = ["us-east-1a", "us-east-1b"]
  private_subnets = ["10.0.1.0/24", "10.0.2.0/24"]
  public_subnets  = ["10.0.101.0/24", "10.0.102.0/24"]

  enable_nat_gateway = true
}
```

### 自定义模块

```hcl
# modules/web-server/main.tf
variable "instance_type" {
  default = "t3.micro"
}

resource "aws_instance" "this" {
  ami           = var.ami
  instance_type = var.instance_type
}

output "instance_id" {
  value = aws_instance.this.id
}

# 使用模块
module "web" {
  source        = "./modules/web-server"
  instance_type = "t3.small"
  ami           = "ami-xxx"
}
```

---

## 迁移指南

### 从 Terraform 迁移

```bash
# 1. 备份状态文件
cp terraform.tfstate terraform.tfstate.backup

# 2. 初始化 OpenTofu
tofu init

# 3. 验证状态
tofu plan  # 应显示 "No changes"

# 4. 更新 CI/CD 脚本
# 将 terraform 命令替换为 tofu
```

### 配置文件兼容

```hcl
# OpenTofu 支持 .tf 和 .tofu 文件扩展名
# 现有 .tf 文件无需修改
```

---

## CI/CD 集成

### GitHub Actions

```yaml
name: OpenTofu Plan
on: [pull_request]

jobs:
  plan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Setup OpenTofu
        uses: opentofu/setup-opentofu@v1
        with:
          tofu_version: "1.6.0"
      
      - name: Init
        run: tofu init
      
      - name: Plan
        run: tofu plan -out=tfplan
      
      - name: Upload Plan
        uses: actions/upload-artifact@v3
        with:
          name: tfplan
          path: tfplan
```

---

## 最佳实践

1. **状态锁定**: 使用 DynamoDB 或等效机制防止并发修改
2. **版本控制**: 将配置文件纳入 Git 管理
3. **模块化**: 使用模块组织可复用的基础设施
4. **变量管理**: 使用 tfvars 文件管理环境差异
5. **敏感数据**: 使用 sensitive 标记或外部密钥管理

---

## 参考资源

- [官方文档](https://opentofu.org/docs/)
- [GitHub Repo](https://github.com/opentofu/opentofu)
- [迁移指南](https://opentofu.org/docs/intro/migration/)
- [Registry](https://registry.opentofu.org/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux.md|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
