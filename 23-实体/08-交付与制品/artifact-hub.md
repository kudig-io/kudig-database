---
title: Artifact Hub [entities]
description: '## 概述'
summary: 'Artifact Hub 是云原生制品的发现和分发平台。它是 CNCF 生态系统的中央枢纽，支持搜索、发现和发布 Helm charts、OPA 策略、Falco 规则、KEDA scalers 等多种制品类型。'
category: entities
tags:
- k8s
- cncf
- supply-chain
- artifact-hub
- helm
- opa
- falco
- crd
- operator
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Artifact Hub 是什么
- 如何 Artifact Hub
trigger_keywords:
- Artifact
- Hub
prerequisites:
- kubectl-basics
- helm-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Artifact Hub

> **CNCF 状态**: Incubating | **类别**: Supply Chain | **主要语言**: Go, TypeScript

## 概述

Artifact Hub 是 CNCF 生态系统的中央制品发现和分发平台，2020 年加入 CNCF Sandbox，后晋升为 Incubating。它支持搜索、发现和发布 Helm Charts、OLM Operators、Falco 规则、OPA 策略、KEDA Scalers、Tekton Pipelines、Container Images 等多种云原生制品类型。Artifact Hub 的目标是成为云原生生态的 "npm registry"，让开发者和运维人员能够一站式发现和安装云原生组件。

## 核心特性

- **多制品类型**: 统一搜索 Helm、OPA、Falco、KEDA、Tekton、Tinkerbell Actions、Container Images 等
- **全文搜索**: 跨制品类型的全文搜索和标签过滤
- **丰富元数据**: 版本历史、依赖关系、安全评级、维护者信息、README 文档
- **安全扫描**: 自动检测容器镜像漏洞
- **签名验证**: 支持 Cosign 签名的制品验证状态展示
- **订阅通知**: 跟踪制品更新，接收新版本和变更通知

## 架构

Artifact Hub 采用前后端分离的微服务架构。后端使用 Go 实现，提供 RESTful API，使用 PostgreSQL 存储制品元数据。前端使用 TypeScript/React 构建。制品来源追踪器（Tracker）定期扫描已注册的仓库（GitHub、Helm Registry、OCI Registry 等），解析制品元数据并更新索引。安全扫描器自动对制品中的容器镜像进行漏洞扫描。整个系统支持 Helm Chart 方式部署到 Kubernetes。

## Kubernetes 集成

Artifact Hub 本身作为服务部署，通过 Helm Chart 安装到 Kubernetes。它与 Kubernetes 生态深度集成：Helm Charts 可直接通过 Artifact Hub 发现并安装；OLM Operators 通过 OperatorHub 集成分发。`helm search hub` 命令直接查询 Artifact Hub 的 API。它还支持 Tekton Pipeline 模板发现和 KEDA Scaler 模板浏览。

## 生产使用场景

1. **组件发现**: 团队在 Artifact Hub 搜索可复用的 Helm Charts 和 Operators
2. **制品发布**: 开源项目在 Artifact Hub 注册仓库，增加可见性
3. **安全合规**: 通过安全扫描评级选择可信制品
4. **版本跟踪**: 订阅关键依赖制品的更新通知

## 安装与配置

```bash
# Artifact Hub 本身无需安装到集群，直接访问 artifacthub.io
# 私有部署 (Air-gapped 或内部制品库)
helm repo add artifact-hub https://artifacthub.github.io/helm-charts
helm install artifact-hub artifact-hub/artifact-hub \
  --namespace artifact-hub --create-namespace \
  --set postgresql.auth.postgresPassword=<password> \
  --set service.type=LoadBalancer

# 使用 Artifact Hub 搜索 Helm Charts
helm search hub wordpress
helm search hub --max-col-width 80 nginx ingress

# 从 Artifact Hub 发现的 Chart 安装
helm repo add bitnami https://charts.bitnami.com/bitnami
helm install my-wordpress bitnami/wordpress

# 注册仓库到 Artifact Hub (通过 Web UI 或 API)
curl -X POST https://artifacthub.io/api/v1/repositories \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer <api-key>" \
  -d '{"name":"my-charts","url":"https://charts.example.com","kind":0}'
```

```yaml
# 私有部署 values.yaml
postgresql:
  auth:
    postgresPassword: "secure-password"
  primary:
    persistence:
      size: 10Gi

service:
  type: LoadBalancer
  port: 80

ingress:
  enabled: true
  hostname: hub.internal.example.com
  tls: true

# 配置仓库扫描间隔
tracker:
  interval: 30m

# 安全扫描配置
scanner:
  enabled: true
  interval: 1h
```

## 支持的制品类型

| 制品类型 | 说明 | 来源示例 |
|----------|------|----------|
| Helm Charts | K8s 应用打包 | bitnami, prometheus-community |
| OLM Operators | Operator Lifecycle Manager | OperatorHub |
| Falco Rules | 运行时安全规则 | falco-security |
| OPA Policies | Rego 策略 | open-policy-agent |
| KEDA Scalers | 事件驱动扩缩容器 | kedacore |
| Tekton Tasks/Pipelines | CI/CD 任务 | tektoncd |
| Container Images | OCI 容器镜像 | 各 Registry |
| Kubectl Plugins | kubectl 插件 | 社区 |
| Headlamp Plugins | 仪表盘插件 | headlamp |
| Backstage Plugins | 开发者门户插件 | backstage |

## 运维操作

```bash
# 🟢 搜索 Helm Charts
helm search hub <keyword>
helm search hub --list-repo-url <keyword>

# 🟢 检查私有 Artifact Hub 状态
kubectl get pods -n artifact-hub
kubectl get svc -n artifact-hub

# 🟢 检查 PostgreSQL 状态
kubectl get pods -n artifact-hub -l app.kubernetes.io/name=postgresql
kubectl exec -n artifact-hub <pg-pod> -- pg_isready

# 🟢 查看已注册仓库 (API)
curl -s https://hub.internal.example.com/api/v1/repositories | jq '.[].name'

# 🟡 触发仓库重新扫描
curl -X PUT https://hub.internal.example.com/api/v1/repositories/<repo-name>/scan \
  -H "Authorization: Bearer <api-key>"

# 🟢 检查扫描日志
kubectl logs -n artifact-hub -l app.kubernetes.io/component=tracker --tail=50
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 搜索无结果 | Tracker 未扫描/索引失败 | 检查 tracker 日志 | 触发重新扫描 |
| 私有部署无法访问 | Service/Ingress 配置错误 | `kubectl get svc,ingress` | 修复网络配置 |
| 仓库注册失败 | URL 不可达/格式错误 | 检查仓库 URL 可达性 | 确认 URL 和认证 |
| 安全扫描无数据 | Scanner 未启用 | 检查 scanner 配置 | 启用并配置 scanner |
| 数据库连接失败 | PostgreSQL 未就绪 | `pg_isready` | 检查 PG Pod 状态 |
| Chart 版本缺失 | 仓库索引未更新 | 检查 tracker 日志 | 手动触发扫描 |

### 排查流程

```
Artifact Hub 异常
├── 私有部署无法访问
│   ├── kubectl get pods -n artifact-hub → Pod 状态
│   ├── kubectl get svc,ingress → 网络暴露
│   └── kubectl logs → 启动错误
├── 制品搜索无结果
│   ├── 检查 Tracker 组件状态
│   ├── 检查仓库注册状态
│   ├── 检查仓库 URL 可达性
│   └── 触发手动重新扫描
└── 安全扫描无数据
    ├── 检查 Scanner 配置是否启用
    ├── 检查 Trivy 数据库是否更新
    └── 检查扫描 Job 日志
```

## 生产案例

### 案例 1: 企业内部制品发现平台

- **场景**: 团队内部 Helm Charts 分散在多个 Git 仓库，新成员难以发现可复用组件
- **排查**: 50+ 内部 Charts 分布在 10 个 Git 仓库，无统一搜索入口
- **方案**: 私有部署 Artifact Hub；注册所有内部 Chart 仓库；配置自动扫描和安全检测
- **效果**: 统一发现入口；新成员上手时间从 2 周降至 2 天；安全扫描自动拦截高危 Chart

### 案例 2: 供应链安全合规

- **场景**: 安全团队要求所有生产部署的 Chart 必须经过安全扫描
- **排查**: 部分 Chart 包含已知漏洞的容器镜像，但团队未感知
- **方案**: Artifact Hub 安全扫描 + Cosign 签名验证；CI/CD 管道仅允许已签名且无高危漏洞的 Chart
- **效果**: 供应链攻击面减少 80%；合规审计时间从 1 周降至 1 天

## 对比与替代方案

| 维度 | Artifact Hub | OperatorHub | Kubeapps | Harbor |
|------|-------------|-------------|----------|--------|
| 制品类型 | 10+ 种 | 仅 Operators | Helm Charts | 容器镜像+Helm |
| 安全扫描 | ✅ | ❌ | ❌ | ✅ Trivy |
| 签名验证 | ✅ Cosign | ❌ | ❌ | ✅ Cosign |
| 私有部署 | ✅ | ❌ | ✅ | ✅ |
| 全文搜索 | ✅ | ✅ | 部分 | 部分 |
| CNCF 官方 | ✅ | Red Hat | Bitnami | ✅ |
| 适用场景 | 统一发现 | Operator 市场 | 应用部署 | 制品存储 |

## 检查清单

- [ ] 私有部署 Pod 全部 Running
- [ ] PostgreSQL 数据持久化已配置
- [ ] 内部仓库已注册并定期扫描
- [ ] 安全扫描已启用
- [ ] Ingress/TLS 配置正确
- [ ] 备份策略已配置 (PostgreSQL)
- [ ] 监控覆盖服务健康状态
- [ ] API Key 管理已配置

## 参考链接

- [[falco]]
- [[operator-pattern]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]
- [[22-概念/05-安全/security-defense-depth.md|security-defense-depth]]
- [[22-概念/09-平台与发布/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[opa]] — OPA (Open Policy Agent)
- [[helm]] — Helm
- [[keda]] — KEDA
- [[falco]] — Falco
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[23-实体/15-参考与索引/cncf-cicd.md|CNCF CI/CD 与发布管理项目全景]] — Cross-reference

<!-- risk-assessed -->
