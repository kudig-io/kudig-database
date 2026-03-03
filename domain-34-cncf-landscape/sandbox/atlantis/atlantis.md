# Atlantis

> **成熟度**: Sandbox | **加入时间**: 2021-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://www.runatlantis.io |
| **GitHub** | https://github.com/runatlantis/atlantis |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Automation & Configuration |
| **适用场景** | Terraform/OpenTofu PR 自动化 |

---

## 项目概述

Atlantis 是一个 Terraform/OpenTofu Pull Request 自动化工具。它监听 Git 仓库的 PR，自动执行 `terraform plan`，并在 PR 中显示执行计划。团队成员可以通过 PR 评论来审查和批准变更，然后通过评论命令执行 `terraform apply`，实现基础设施即代码的协作式工作流。

---

## 核心特性

- **PR 自动化**: PR 创建时自动执行 terraform plan
- **评论驱动**: 通过 PR 评论控制工作流
- **多 VCS 支持**: GitHub、GitLab、Bitbucket、Azure DevOps
- **工作区隔离**: 支持多工作区并行操作
- **锁定机制**: 防止并发修改同一状态
- **审批流程**: 可配置的 apply 前审批要求
- **自定义工作流**: 灵活的 plan/apply 流程定制

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                    Atlantis Architecture                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    Git Provider                           │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │                   Pull Request                       │ │   │
│  │  │  ┌─────────────────────────────────────────────┐    │ │   │
│  │  │  │  infra/main.tf                              │    │ │   │
│  │  │  │  + resource "aws_instance" "web" { ... }   │    │ │   │
│  │  │  └─────────────────────────────────────────────┘    │ │   │
│  │  │                                                      │ │   │
│  │  │  Comments:                                           │ │   │
│  │  │  ┌─────────────────────────────────────────────┐    │ │   │
│  │  │  │ 🤖 Atlantis Plan Output:                    │    │ │   │
│  │  │  │ Plan: 1 to add, 0 to change, 0 to destroy  │    │ │   │
│  │  │  │                                             │    │ │   │
│  │  │  │ 👤 User: atlantis apply                     │    │ │   │
│  │  │  └─────────────────────────────────────────────┘    │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │ Webhook                       │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │                   Atlantis Server                         │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │                  Core Components                     │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │  Webhook    │  │  Command    │  │   Lock     │  │ │   │
│  │  │  │  Handler    │  │  Parser     │  │   Manager  │  │ │   │
│  │  │  └──────┬──────┘  └──────┬──────┘  └─────┬──────┘  │ │   │
│  │  │         │                │               │          │ │   │
│  │  │  ┌──────▼────────────────▼───────────────▼──────┐  │ │   │
│  │  │  │              Workflow Engine                  │  │ │   │
│  │  │  │  ┌─────────┐ ┌─────────┐ ┌─────────────────┐ │  │ │   │
│  │  │  │  │  Plan   │ │  Apply  │ │  Custom Steps   │ │  │ │   │
│  │  │  │  │  Stage  │ │  Stage  │ │  (Pre/Post)     │ │  │ │   │
│  │  │  │  └─────────┘ └─────────┘ └─────────────────┘ │  │ │   │
│  │  │  └──────────────────────────────────────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  │                             │                             │   │
│  │  ┌──────────────────────────▼──────────────────────────┐ │   │
│  │  │              Terraform/OpenTofu CLI                  │ │   │
│  │  │  terraform init → plan → apply                       │ │   │
│  │  └──────────────────────────────────────────────────────┘ │   │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│  ┌───────────────────────────▼───────────────────────────────┐  │
│  │                    Cloud Providers                         │  │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────────┐  │  │
│  │  │   AWS   │  │  Azure  │  │   GCP   │  │   Others    │  │  │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────────┘  │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **Webhook Handler** | 接收 Git 提供商的 Webhook 事件 |
| **Command Parser** | 解析 PR 评论中的命令 |
| **Lock Manager** | 管理工作区锁，防止并发冲突 |
| **Workflow Engine** | 执行 plan/apply 工作流 |

---

## 快速开始

### Docker 部署

```bash
docker run -d \
  --name atlantis \
  -p 4141:4141 \
  -e ATLANTIS_GH_USER=atlantis-bot \
  -e ATLANTIS_GH_TOKEN=ghp_xxx \
  -e ATLANTIS_GH_WEBHOOK_SECRET=your-secret \
  -e ATLANTIS_REPO_ALLOWLIST="github.com/your-org/*" \
  ghcr.io/runatlantis/atlantis:latest server
```

### Kubernetes 部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: atlantis
  namespace: atlantis
spec:
  replicas: 1
  selector:
    matchLabels:
      app: atlantis
  template:
    metadata:
      labels:
        app: atlantis
    spec:
      containers:
        - name: atlantis
          image: ghcr.io/runatlantis/atlantis:latest
          args:
            - server
          ports:
            - containerPort: 4141
          env:
            - name: ATLANTIS_GH_USER
              value: "atlantis-bot"
            - name: ATLANTIS_GH_TOKEN
              valueFrom:
                secretKeyRef:
                  name: atlantis-secrets
                  key: github-token
            - name: ATLANTIS_GH_WEBHOOK_SECRET
              valueFrom:
                secretKeyRef:
                  name: atlantis-secrets
                  key: webhook-secret
            - name: ATLANTIS_REPO_ALLOWLIST
              value: "github.com/your-org/*"
            - name: ATLANTIS_DATA_DIR
              value: /atlantis-data
          volumeMounts:
            - name: atlantis-data
              mountPath: /atlantis-data
      volumes:
        - name: atlantis-data
          persistentVolumeClaim:
            claimName: atlantis-data

---
apiVersion: v1
kind: Service
metadata:
  name: atlantis
  namespace: atlantis
spec:
  type: ClusterIP
  ports:
    - port: 80
      targetPort: 4141
  selector:
    app: atlantis
```

### Helm 安装

```bash
helm repo add runatlantis https://runatlantis.github.io/helm-charts
helm repo update

helm install atlantis runatlantis/atlantis \
  --namespace atlantis \
  --create-namespace \
  --set github.user=atlantis-bot \
  --set github.token=ghp_xxx \
  --set github.secret=webhook-secret \
  --set orgAllowlist="github.com/your-org/*"
```

---

## 配置文件

### Server 端配置 (atlantis.yaml)

```yaml
# atlantis.yaml (可选的服务端配置)
repos:
  - id: github.com/your-org/infra
    branch: main
    apply_requirements:
      - approved
      - mergeable
    allowed_overrides:
      - workflow
      - apply_requirements
    allow_custom_workflows: true
    
  - id: "/.*/infra-.*/"  # 正则匹配
    workflow: custom
    apply_requirements:
      - approved

workflows:
  custom:
    plan:
      steps:
        - init
        - run: terraform validate
        - plan:
            extra_args: ["-var-file", "env/prod.tfvars"]
    apply:
      steps:
        - apply
```

### 仓库端配置 (atlantis.yaml)

```yaml
# 放在仓库根目录
version: 3
automerge: true
delete_source_branch_on_merge: true

projects:
  - name: production
    dir: environments/prod
    workspace: default
    terraform_version: v1.6.0
    autoplan:
      when_modified:
        - "*.tf"
        - "../modules/**/*.tf"
      enabled: true
    apply_requirements:
      - approved
      - mergeable
      
  - name: staging
    dir: environments/staging
    workspace: default
    autoplan:
      enabled: true

workflows:
  custom:
    plan:
      steps:
        - run: terraform fmt -check
        - init
        - plan
    apply:
      steps:
        - run: echo "Applying to production!"
        - apply
```

---

## PR 评论命令

### 基本命令

```bash
# 执行 plan
atlantis plan

# 指定项目 plan
atlantis plan -p production

# 指定目录 plan
atlantis plan -d environments/prod

# 执行 apply
atlantis apply

# 指定项目 apply
atlantis apply -p production

# 解锁
atlantis unlock

# 查看帮助
atlantis help
```

### 高级命令

```bash
# 传递额外参数
atlantis plan -- -var="env=prod" -target=aws_instance.web

# 指定工作区
atlantis plan -w production

# 强制重新 plan
atlantis plan -p production -- -refresh=true

# 批量 apply 所有项目
atlantis apply
```

---

## 多环境配置

### 目录结构

```
infra/
├── atlantis.yaml
├── modules/
│   ├── vpc/
│   └── ec2/
└── environments/
    ├── dev/
    │   ├── main.tf
    │   └── terraform.tfvars
    ├── staging/
    │   ├── main.tf
    │   └── terraform.tfvars
    └── prod/
        ├── main.tf
        └── terraform.tfvars
```

### atlantis.yaml 配置

```yaml
version: 3
projects:
  - name: dev
    dir: environments/dev
    autoplan:
      enabled: true
      when_modified:
        - "*.tf"
        - "*.tfvars"
        - "../../modules/**/*.tf"
        
  - name: staging
    dir: environments/staging
    autoplan:
      enabled: true
    apply_requirements:
      - approved
      
  - name: prod
    dir: environments/prod
    autoplan:
      enabled: true
    apply_requirements:
      - approved
      - mergeable
    workflow: production

workflows:
  production:
    plan:
      steps:
        - run: |
            echo "Planning production environment"
            terraform fmt -check
        - init
        - plan:
            extra_args: ["-var-file=terraform.tfvars"]
    apply:
      steps:
        - run: echo "Applying to PRODUCTION - Please verify!"
        - apply
```

---

## 安全配置

### Webhook 安全

```yaml
# 环境变量配置
ATLANTIS_GH_WEBHOOK_SECRET: "your-complex-secret"
ATLANTIS_GH_TOKEN: "ghp_xxx"

# SSL/TLS 配置
ATLANTIS_SSL_CERT_FILE: /etc/ssl/certs/atlantis.crt
ATLANTIS_SSL_KEY_FILE: /etc/ssl/private/atlantis.key
```

### 仓库白名单

```bash
# 精确匹配
--repo-allowlist="github.com/your-org/infra"

# 通配符
--repo-allowlist="github.com/your-org/*"

# 多仓库
--repo-allowlist="github.com/your-org/infra,github.com/your-org/platform"
```

### 敏感信息处理

```yaml
# 使用环境变量
workflows:
  default:
    plan:
      steps:
        - env:
            name: AWS_ACCESS_KEY_ID
            command: 'aws secretsmanager get-secret-value --secret-id atlantis/aws --query SecretString --output text | jq -r .access_key'
        - init
        - plan
```

---

## 与 OpenTofu 集成

```yaml
# Server 端配置使用 OpenTofu
ATLANTIS_TF_DOWNLOAD_URL: "https://github.com/opentofu/opentofu/releases"

# 或在 atlantis.yaml 中指定
projects:
  - name: production
    dir: environments/prod
    terraform_version: v1.6.0  # OpenTofu 版本
```

---

## 监控和日志

### Prometheus 指标

```yaml
# 启用指标端点
ATLANTIS_STATS_NAMESPACE: atlantis
```

### 关键指标

| 指标 | 说明 |
|:---|:---|
| `atlantis_cmd_count` | 命令执行次数 |
| `atlantis_cmd_duration` | 命令执行时长 |
| `atlantis_locks` | 当前锁数量 |
| `atlantis_webhooks_received` | 接收的 webhook 数量 |

---

## 最佳实践

1. **分支策略**: 只允许从 main/master 分支 apply
2. **审批要求**: 生产环境要求 PR 审批
3. **锁定管理**: 定期清理过期的锁
4. **Secret 管理**: 使用 Vault 或 AWS Secrets Manager
5. **状态后端**: 使用远程状态后端 (S3, GCS)
6. **高可用**: 使用持久存储保存 Atlantis 数据

---

## 参考资源

- [官方文档](https://www.runatlantis.io/docs/)
- [GitHub Repo](https://github.com/runatlantis/atlantis)
- [服务端配置](https://www.runatlantis.io/docs/server-side-repo-config.html)
- [仓库端配置](https://www.runatlantis.io/docs/repo-level-atlantis-yaml.html)
- [自定义工作流](https://www.runatlantis.io/docs/custom-workflows.html)

---

**维护者**: Kudig Team | **许可证**: MIT
