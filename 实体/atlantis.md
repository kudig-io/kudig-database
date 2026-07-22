---
title: Atlantis (entities)
description: '## 概述'
summary: 'Atlantis 是一个 Terraform/OpenTofu Pull Request 自动化工具。它监听 Git 仓库的 PR，自动执行 `terraform plan`，并在 PR 中显示执行计划。团队成员可以通过 PR 评论来审查和批准变更，然后通过评论命令执行 `terraform apply`，实现基础设施即代码的协作式工作流。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- atlantis
- prometheus
- grafana
- crd
- operator
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Atlantis 是什么
- 如何 Atlantis
trigger_keywords:
- Atlantis
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- iac-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Atlantis

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

Atlantis 是由 Hootsuite 开源（现由社区维护）的 Terraform/OpenTofu Pull Request 自动化工具。它监听 Git 仓库的 PR，自动执行 `terraform plan`，并在 PR 中显示执行计划。团队成员可以通过 PR 评论审查和批准变更，然后通过评论命令执行 `terraform apply`，实现基础设施即代码（IaC）的协作式工作流。Atlantis 是 Terraform 社区最受欢迎的 PR 自动化方案。

## 核心特性

- **PR 自动化**: PR 创建/更新时自动执行 terraform plan
- **评论驱动**: 通过 `atlantis plan`、`atlantis apply` 等 PR 评论控制工作流
- **多 VCS 支持**: GitHub、GitLab、Bitbucket、Azure DevOps
- **多工作区**: 支持多 Terraform workspace 并行操作
- **项目锁定**: 自动锁定正在操作的项目，防止并发修改
- **自定义工作流**: 支持自定义 plan/apply 脚本和步骤

## 架构

Atlantis 是一个用 Go 编写的 Web 服务。核心流程：Atlantis 配置 Webhook 接收 Git 仓库的 PR 事件，解析 PR 变更中涉及的 Terraform 目录，在本地检出代码执行 `terraform init && terraform plan -out planfile`，将 plan 输出作为 PR 评论发布。当用户评论 `atlantis apply` 时，Atlantis 执行 `terraform apply planfile` 并将结果评论到 PR。Atlantis 在本地维护每个项目的锁状态，防止并发操作。所有 Terraform 操作在 Atlantis 容器内执行。

## Kubernetes 集成

Atlantis 以 Deployment 部署到 Kubernetes。通过 Service 暴露 Webhook 接收端点。使用 PVC 或持久卷存储 Terraform 状态缓存和锁数据。通过 Kubernetes Secret 管理 VCS Token 和云凭证。支持 Helm Chart 部署。与 ArgoCD/FluxCD 配合时，Atlantis 负责 plan/apply，GitOps 工具负责将 Terraform state 变更同步到集群。Ingress 暴露 Webhook 端点。

## 生产使用场景

1. **基础设施 PR 审查**: 团队成员通过 PR 协作审查基础设施变更
2. **自动化 plan/apply**: 消除手动执行 Terraform 命令的繁琐流程
3. **多环境管理**: 为 dev/staging/prod 配置不同的工作区和审批规则
4. **合规审计**: PR 评论中保留完整的 plan 输出和 apply 记录

## 安装与配置

```bash
# Helm 安装
helm repo add atlantis https://runatlantis.github.io/helm-charts
helm install atlantis atlantis/atlantis \
  --namespace atlantis --create-namespace \
  --set github.user=<bot-username> \
  --set github.token=<github-token> \
  --set github.secret=<webhook-secret> \
  --set ingress.enabled=true \
  --set ingress.host=atlantis.example.com \
  --set orgAllowlist=github.com/myorg/* \
  --set serviceAccount.create=true
```

### 服务端配置 (server-side repo config)

```yaml
# repos.yaml - 服务端仓库配置
repos:
  - id: github.com/myorg/infrastructure
    apply_requirements: [approved, mergeable]
    workflow: production
    allowed_overrides: [workflow]
    allow_custom_workflows: false
workflows:
  production:
    plan:
      steps:
        - init
        - plan: -var-file=envs/prod.tfvars
    apply:
      steps:
        - apply
  default:
    plan:
      steps: [init, plan]
    apply:
      steps: [apply]
```

### 仓库端配置 (atlantis.yaml)

```yaml
# atlantis.yaml - 放在仓库根目录
version: 3
automerge: true
delete_source_branch_on_merge: true
projects:
  - dir: infrastructure/networking
    workspace: production
    terraform_version: v1.7.0
    apply_requirements: [approved]
  - dir: infrastructure/compute
    workspace: staging
    workflow: staging
workflows:
  staging:
    plan:
      steps:
        - init
        - plan: -var-file=envs/staging.tfvars
```

## 运维操作

```bash
# 🟢 查看 Atlantis 服务状态
kubectl -n atlantis get pods -l app=atlantis
kubectl -n atlantis logs -l app=atlantis --tail=100

# 🟢 查看当前锁状态
curl https://atlantis.example.com/locks

# 🟡 解锁指定项目（解决死锁）
curl -X DELETE https://atlantis.example.com/locks/<lock-id>

# 🟡 手动触发 plan（通过 PR 评论）
# 在 PR 中评论: atlantis plan -d infrastructure/networking

# 🟡 手动触发 apply（通过 PR 评论）
# 在 PR 中评论: atlantis apply -d infrastructure/networking

# 🟢 查看 Terraform 版本和插件缓存
kubectl -n atlantis exec deploy/atlantis -- terraform version
kubectl -n atlantis exec deploy/atlantis -- ls /atlantis-data/plugin-cache/

# 🔴 重启 Atlantis（会中断正在执行的 plan/apply）
kubectl -n atlantis rollout restart deploy/atlantis

# 🟢 检查 PVC 状态（Terraform 状态缓存）
kubectl -n atlantis get pvc
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Webhook 不触发 | Secret 不匹配 | 检查 GitHub Webhook 配置 | 重新配置 Webhook Secret |
| plan 超时 | 网络/Provider 问题 | `kubectl logs -l app=atlantis` | 检查云凭证和网络连通性 |
| 项目死锁 | apply 失败未释放锁 | `curl /locks` | 手动删除锁 |
| 磁盘空间不足 | 多仓库克隆占用 | `kubectl exec -- df -h` | 增大 PVC 或清理旧克隆 |
| 凭证过期 | Token 轮换 | 检查 Secret 内容 | 更新 K8s Secret |
| 并发冲突 | 多 PR 操作同一目录 | 查看锁状态 | 等待前一个 PR 合并 |

### 排查流程

```
PR 无响应 → 检查 Webhook 投递状态
  ├─ Webhook 失败 → 检查 Secret/网络/Ingress
  └─ Webhook 成功 → 检查 Atlantis 日志
      ├─ 认证失败 → 检查 VCS Token
      ├─ 项目被锁 → 等待或手动解锁
      └─ Terraform 执行失败 → 检查 Provider/凭证/网络
          ├─ init 失败 → 检查 plugin cache/registry 连通性
          └─ plan 失败 → 检查 tfvars/变量定义
```

## 生产案例

### 案例1: 多团队基础设施协作

**场景**: 5个团队共享一个基础设施仓库，频繁并发修改  
**排查**: 多 PR 同时操作同一 Terraform 目录导致状态冲突  
**方案**: 按团队拆分目录 + 项目级锁 + apply_requirements: [approved]  
**效果**: 消除并发冲突，变更审批时间从 2天缩短到 4小时  

### 案例2: Terraform State 损坏恢复

**场景**: apply 中断导致 state 文件损坏，后续所有 plan 失败  
**排查**: `terraform state list` 报错，确认 state 文件 JSON 截断  
**方案**: 从远程后端（S3+DynamoDB）恢复最近版本，`terraform state push`  
**效果**: 30分钟内恢复，后续增加 state 备份策略  

## 替代方案

| 项目 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| **Atlantis** | 开源、多 VCS、成熟 | 需自托管、单点 | 多团队协作 IaC |
| Terraform Cloud | SaaS、远程状态、策略 | 商业产品、成本高 | 企业级托管 |
| Digger | 开源、GitHub Actions | 社区较小 | GitHub 原生工作流 |
| env0 | 多框架、成本管理 | SaaS 依赖 | 多云成本管控 |
| Scalr | 策略引擎、层级继承 | 商业产品 | 大型组织治理 |

## 架构定位

在 DevOps 生态中，Atlantis 属于 **IaC Automation** 类别，是 Terraform PR 协作工作流的标准方案。它填补了 GitOps 工具链中 IaC 自动化的空白。

## 检查清单

- [ ] 配置 apply_requirements 强制审批
- [ ] 使用远程 State 后端（S3/GCS + 锁）
- [ ] 配置 orgAllowlist 限制可操作仓库
- [ ] 设置合理的 Terraform 超时时间
- [ ] 配置 PVC 持久化插件缓存
- [ ] 定期轮换 VCS Token
- [ ] 配置 Ingress TLS 保护 Webhook 端点
- [ ] 监控 Atlantis Pod 资源使用和磁盘空间

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/vault.md|vault]]
- [[deployment]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[opentofu]] — OpenTofu
- [[实体/vault.md|vault]] — HashiCorp Vault
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
