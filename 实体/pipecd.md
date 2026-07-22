---
title: PipeCD [entities]
description: '## 概述'
summary: 'PipeCD 是一个统一的持续交付平台，为 Kubernetes、Terraform、CloudRun、Lambda、ECS 等多种应用平台提供一致的 GitOps 部署体验。它采用控制平面（Control Plane）+ 代理（Piped）架构，支持渐进式交付策略（金丝雀、蓝绿、滚动）和自动回滚。'
category: entities
tags:
- k8s
- cncf
- ci-cd
- pipecd
- prometheus
- grafana
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
- PipeCD 是什么
- 如何 PipeCD
trigger_keywords:
- PipeCD
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- iac-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# PipeCD

> **CNCF 状态**: Sandbox | **类别**: CI/CD | **主要语言**: Go

## 概述

PipeCD 是由 mercari（日本电商平台）开发的统一持续交付平台，2021 年进入 CNCF Sandbox。它为 Kubernetes、Terraform、AWS Lambda、Google Cloud Run、AWS ECS 等多种应用平台提供**一致的 GitOps 部署体验**。与 ArgoCD/Flux 主要面向 Kubernetes 不同，PipeCD 通过统一的部署抽象覆盖多云多平台的交付需求。

PipeCD 采用**控制平面（Control Plane）+ 代理（Piped）**架构。Piped 是轻量级代理，部署在每个目标环境/集群中，与控制平面通信拉取部署配置。这种设计使 PipeCD 无需直连目标集群的 API Server，特别适合网络隔离的多环境场景。它原生支持渐进式交付策略（金丝雀、蓝绿、滚动）和基于 Prometheus 指标的自动分析和回滚。

## Key Features

- **多平台 GitOps**：统一的 GitOps 部署覆盖 K8s、Terraform、Lambda、CloudRun、ECS
- **Piped 代理架构**：无需直连集群，Piped 在目标环境内执行部署操作
- **渐进式交付**：金丝雀、蓝绿、滚动发布，支持自定义阶段和等待审批
- **自动化分析**：金丝雀阶段自动分析 Prometheus 指标，异常自动回滚
- **可视化部署**：Web UI 展示每个部署的阶段、时间和状态
- **Secret 管理**：集成 Sealed Secrets/SOPS，安全管理 Git 中的敏感配置

## Architecture

PipeCD 由 **Control Plane**（包含 API Server、Deployment Controller、Data Store（MySQL）和 File Store（对象存储））和 **Piped Agent**（运行在每个目标环境的轻量级代理）组成。Piped 定期从控制平面拉取待执行的部署计划，在本地执行部署操作（kubectl apply、terraform apply 等），并将状态和日志回传。部署配置（`.piped.yaml` 和 `app.pipecd.yaml`）存储在 Git 仓库中。

## K8s 集成

PipeCD 的 Piped 代理在 Kubernetes 集群中运行，通过 kubeconfig 操作集群资源。Piped 从 Git 拉取应用配置（Helm/Kustomize/Plain YAML），执行 `kubectl apply` 或 Helm 部署。对于多集群场景，每个集群部署一个 Piped，控制平面统一管理所有 Piped 的部署流水线。也支持通过 Piped 部署 Terraform 管理的云基础设施。

## 生产部署要点

- **渐进式交付**：所有生产部署使用金丝雀或蓝绿策略，避免一次性全量发布
- **自动分析**：配置 Prometheus 指标分析，在金丝雀阶段自动检测异常
- **审批门控**：关键阶段设置 WAIT_APPROVAL，确保人工确认
- **Piped 隔离**：每个环境/集群部署独立的 Piped，缩小爆炸半径
- **Secret 管理**：使用 Sealed Secrets 或 SOPS 加密 Git 中的敏感配置
- **多集群**：通过 Piped 代理实现多集群部署，无需直连集群 API

## 生产场景

1. **多平台统一交付**：K8s 服务 + Lambda 函数 + Terraform 基础设施统一管理
2. **金丝雀+自动回滚**：金丝雀阶段分析错误率，异常自动回滚避免故障
3. **多环境流水线**：dev → staging → prod 自动晋级，关键阶段人工审批
4. **隔离网络部署**：网络隔离的环境中通过 Piped 代理安全部署

## 安装与配置

### Control Plane 部署

```bash
# 安装 PipeCD Control Plane
helm repo add pipecd https://pipe-cd.github.io/charts
helm install pipecd pipecd/pipecd -n pipecd --create-namespace \
  --set controller.enabled=true \
  --set mysql.enabled=true

# 验证部署
kubectl get pods -n pipecd
kubectl port-forward svc/pipecd 8080:8080 -n pipecd
```

### Piped Agent 安装

```bash
# 在目标集群安装 Piped Agent
helm install piped pipecd/piped -n pipecd \
  --set config.apiAddress=https://pipecd.example.com \
  --set config.projectId=my-project \
  --set config.pipedId=piped-cluster-1 \
  --set config.pipedKeySecret=piped-key

# 验证 Piped 连接状态
kubectl get pods -n pipecd -l app=piped
```

### 应用配置

```yaml
# app.pipecd.yaml - 应用仓库中的部署配置
apiVersion: pipecd.dev/v1beta1
kind: KubernetesApp
spec:
  name: myapp
  labels:
    team: backend
  pipeline:
    stages:
      - name: K8S_CANARY_ROLLOUT
        with:
          replicas: 10%
          wait: 5m
      - name: ANALYSIS
        with:
          duration: 5m
          metrics:
            - name: error-rate
              provider: prometheus
              query: rate(http_requests_total{status=~"5.."}[5m])
              threshold: 0.01
      - name: WAIT_APPROVAL
        with:
          approvers: ["admin@example.com"]
      - name: K8S_PRIMARY_ROLLOUT
---
# Terraform 应用示例
apiVersion: pipecd.dev/v1beta1
kind: TerraformApp
spec:
  name: infrastructure
  pipeline:
    stages:
      - name: TERRAFORM_PLAN
      - name: WAIT_APPROVAL
      - name: TERRAFORM_APPLY
```

## 运维操作

```bash
# 🟢 查看应用部署状态
kubectl get applications -n pipecd

# 🟢 查看 Piped 连接状态
kubectl get pipeds -n pipecd

# 🟡 手动触发部署
pipectl application sync --app-id=<app-id>

# 🟡 回滚到上一版本
pipectl application rollback --app-id=<app-id>

# 🟡 跳过当前阶段
pipectl application skip-stage --app-id=<app-id> --stage-id=<stage-id>

# 🔴 取消正在进行的部署
pipectl application cancel --app-id=<app-id>

# 🔴 删除应用配置
kubectl delete application <app-id> -n pipecd
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Piped 断连 | API 地址不可达 | `kubectl logs -n pipecd -l app=piped` | 检查 apiAddress 和网络 |
| 部署卡在 ANALYSIS | Prometheus 查询失败 | `pipectl application get --app-id=<id>` | 检查 Prometheus 连接和查询语句 |
| 金丝雀回滚 | 错误率超阈值 | 查看 PipeCD Web UI 日志 | 检查应用日志修复错误 |
| Helm 安装失败 | Chart 仓库不可达 | `kubectl logs -n pipecd -l app=piped` | 检查 Helm repo URL 和认证 |
| 审批超时 | 审批人未响应 | PipeCD Web UI | 配置超时自动处理 |

**排查流程：**
```
部署失败
├── 检查 Piped 状态 → kubectl get pipeds -n pipecd
├── 查看应用日志 → pipectl application get --app-id=<id>
├── 检查 Piped 日志 → kubectl logs -n pipecd -l app=piped
├── 检查目标集群 → kubectl get pods -n <target-ns>
└── 检查 Git 源 → 确认 app.pipecd.yaml 配置正确
```

## 生产案例

### 案例一：多平台统一交付

- **场景**: 团队同时管理 K8s 微服务、AWS Lambda 和 Terraform 基础设施，需要统一交付平台
- **排查**: 之前使用 3 个不同工具，流程不一致，审批分散
- **方案**: PipeCD 统一管理，K8s 用金丝雀发布，Lambda 用流量切换，Terraform 用 plan+approval
- **效果**: 交付流程统一，审批集中，部署频率从每周 2 次提升至每天 5 次

### 案例二：隔离网络安全部署

- **场景**: 生产集群在隔离网络中，无法从外部直接访问
- **排查**: ArgoCD/Flux 需要集群可访问，无法用于隔离环境
- **方案**: 使用 PipeCD Piped 代理架构，Piped 主动向外连接 Control Plane，无需入站连接
- **效果**: 隔离网络集群安全纳管，无需开放任何入站端口

## 对比

| 特性 | PipeCD | ArgoCD | Flux | Spinnaker | 适用场景 |
|------|--------|--------|------|-----------|----------|
| 多平台 | ✅ K8s/Lambda/Terraform | ❌ K8s only | ❌ K8s only | ✅ | PipeCD 多平台 |
| 代理架构 | ✅ Piped | ❌ 直连 | ❌ 直连 | ✅ | 隔离网络 |
| 自动分析 | ✅ Prometheus | ⚠️ Argo Rollouts | ❌ | ✅ Kayenta | - |
| 复杂度 | ⭐⭐ | ⭐ | ⭐ | ⭐⭐⭐ | - |
| 审批流程 | ✅ 内置 | ⚠️ 插件 | ❌ | ✅ | - |

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[概念/gitops-principles.md|gitops-principles]]
- [[概念/secrets-management.md|secrets-management]]

## Related

- [[cubefs]] — CubeFS
- [[artifact-hub]] — Artifact Hub
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[sops]] — SOPS (Secrets OPerationS)

- pipecd
- [[实体/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
