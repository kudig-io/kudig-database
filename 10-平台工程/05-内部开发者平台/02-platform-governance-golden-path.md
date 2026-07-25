---
title: Platform Engineering Governance — Golden Paths, Scorecards, and Policies
description: 平台工程治理 — Golden Path 设计、服务成熟度评分卡、策略即代码、自助服务门户、平台度量体系
summary: 构建平台工程治理体系，通过 Golden Path 和自动化策略提升开发者体验与合规性
category: practice
tags:
- platform-engineering
- governance
- golden-path
- scorecard
- policy-as-code
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: platform
---
# 平台工程治理体系

> 通过 Golden Path、成熟度评分卡和策略即代码构建可持续的平台治理。

## 治理框架全景

```
┌─────────────────────────────────────────────────────────┐
│  平台治理层                                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │Golden    │  │成熟度    │  │策略即代码│             │
│  │Path      │  │评分卡    │  │(OPA/     │             │
│  │模板      │  │          │  │ Kyverno) │             │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘             │
│       │              │              │                   │
│  ┌────▼──────────────▼──────────────▼────┐             │
│  │  自助服务门户 (Backstage/Port)         │             │
│  └───────────────────┬───────────────────┘             │
│                      │                                  │
│  ┌───────────────────▼───────────────────┐             │
│  │  平台度量 (DORA/SPACE/自定义)          │             │
│  └───────────────────────────────────────┘             │
└─────────────────────────────────────────────────────────┘
```

## Golden Path 设计

### 服务模板（Scaffolder）

```yaml
# Backstage Software Template
apiVersion: scaffolder.backstage.io/v1beta3
kind: Template
metadata:
  name: golden-path-service
  title: 创建标准微服务
  description: 基于 Golden Path 创建生产就绪的微服务
spec:
  owner: platform-team
  type: service
  parameters:
    - title: 服务信息
      required:
        - serviceName
        - owner
        - language
      properties:
        serviceName:
          title: 服务名称
          type: string
          pattern: '^[a-z][a-z0-9-]{2,30}$'
        owner:
          title: 负责团队
          type: string
          ui:field: OwnerPicker
        language:
          title: 编程语言
          type: string
          enum: [go, nodejs, java, python, rust]
        description:
          title: 服务描述
          type: string
    - title: 基础设施选择
      properties:
        cloud:
          title: 目标云
          type: string
          enum: [aws, gcp, azure]
          default: aws
        tier:
          title: 服务等级
          type: string
          enum: [critical, standard, experimental]
          default: standard
        database:
          title: 数据库需求
          type: string
          enum: [none, postgresql, mysql, redis, mongodb]
          default: none
  steps:
    - id: fetch-template
      action: fetch:template
      input:
        url: ./templates/${{ parameters.language }}-service
        values:
          serviceName: ${{ parameters.serviceName }}
          owner: ${{ parameters.owner }}
          tier: ${{ parameters.tier }}
    - id: publish
      action: publish:github
      input:
        repoUrl: github.com?owner=myorg&repo=${{ parameters.serviceName }}
    - id: register
      action: catalog:register
      input:
        repoContentsUrl: ${{ steps.publish.output.repoContentsUrl }}
        catalogInfoPath: /catalog-info.yaml
    - id: create-ci
      action: github:actions:workflow
      input:
        repoUrl: github.com?owner=myorg&repo=${{ parameters.serviceName }}
        workflowPath: .github/workflows/ci.yaml
```

### Golden Path 清单

| 组件 | 标准配置 | 可选升级 |
|------|----------|----------|
| CI/CD | GitHub Actions + ArgoCD | Tekton + 自定义 Pipeline |
| 容器化 | Distroless 镜像 + 多阶段构建 | GraalVM Native |
| 部署 | Deployment + HPA + PDB | Argo Rollouts 金丝雀 |
| 可观测性 | OTel SDK + Prometheus + Grafana | 自定义 Dashboard |
| 日志 | 结构化 JSON + Loki | 自定义解析 |
| 安全 | PSA restricted + NetworkPolicy | mTLS + OPA |
| 文档 | catalog-info.yaml + README | TechDocs |
| 告警 | 基础 SLO 告警 | 自定义 Burn Rate |

## 服务成熟度评分卡

### 评分模型

```yaml
# 服务成熟度评分（自动化检测）
apiVersion: v1
kind: ConfigMap
metadata:
  name: maturity-scorecard
  namespace: platform-system
data:
  scorecard.yaml: |
    levels:
      - name: Bronze
        score: 40
        requirements:
          - has_deployment: true
          - has_health_checks: true
          - has_resource_limits: true
          - has_ci_pipeline: true
      - name: Silver
        score: 70
        requirements:
          - has_hpa: true
          - has_pdb: true
          - has_network_policy: true
          - has_structured_logging: true
          - has_tracing: true
          - has_slo_defined: true
          - has_runbook: true
      - name: Gold
        score: 90
        requirements:
          - has_canary_deployment: true
          - has_chaos_testing: true
          - has_disaster_recovery_plan: true
          - has_security_scan: true
          - has_load_test: true
          - has_cost_labels: true
          - has_oncall_rotation: true
      - name: Platinum
        score: 100
        requirements:
          - has_multi_region: true
          - has_game_day_participation: true
          - has_contribution_to_platform: true
```

### 自动检测脚本

```bash
#!/bin/bash
# check-maturity.sh — 检查服务成熟度
NAMESPACE=$1
SERVICE=$2
SCORE=0

# 基础检查
kubectl get deployment $SERVICE -n $NAMESPACE -o jsonpath='{.spec.template.spec.containers[0].readinessProbe}' | grep -q . && ((SCORE+=10))
kubectl get deployment $SERVICE -n $NAMESPACE -o jsonpath='{.spec.template.spec.containers[0].resources.limits}' | grep -q . && ((SCORE+=10))
kubectl get hpa -n $NAMESPACE | grep -q $SERVICE && ((SCORE+=10))
kubectl get pdb -n $NAMESPACE | grep -q $SERVICE && ((SCORE+=10))
kubectl get networkpolicy -n $NAMESPACE -o yaml | grep -q $SERVICE && ((SCORE+=10))

# 可观测性检查
kubectl get deployment $SERVICE -n $NAMESPACE -o yaml | grep -q "OTEL_EXPORTER" && ((SCORE+=10))
kubectl get servicemonitor -n $NAMESPACE | grep -q $SERVICE && ((SCORE+=10))

# 安全检查
kubectl get deployment $SERVICE -n $NAMESPACE -o jsonpath='{.spec.template.spec.securityContext.runAsNonRoot}' | grep -q true && ((SCORE+=10))
kubectl get deployment $SERVICE -n $NAMESPACE -o yaml | grep -q "readOnlyRootFilesystem: true" && ((SCORE+=10))

echo "Service $SERVICE maturity score: $SCORE/100"
```

## 策略即代码（Policy as Code）

### OPA/Gatekeeper 平台策略

```yaml
# 强制所有 Deployment 必须有资源限制
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequiredResources
metadata:
  name: deployment-must-have-resources
spec:
  match:
    kinds:
      - apiGroups: ["apps"]
        kinds: ["Deployment"]
    excludedNamespaces: ["kube-system", "monitoring"]
  parameters:
    cpu: "10m"
    memory: "64Mi"
---
# 强制镜像必须来自内部 Registry
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sAllowedRepos
metadata:
  name: restrict-image-registries
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
    excludedNamespaces: ["kube-system"]
  parameters:
    repos:
      - "registry.internal.example.com/"
      - "gcr.io/distroless/"
---
# 强制标签（团队/环境/成本中心）
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequiredLabels
metadata:
  name: require-team-labels
spec:
  match:
    kinds:
      - apiGroups: ["apps"]
        kinds: ["Deployment", "StatefulSet"]
  parameters:
    labels:
      - key: team
        allowedRegex: "^[a-z-]+$"
      - key: environment
        allowedRegex: "^(production|staging|development)$"
      - key: cost-center
        allowedRegex: "^CC-[0-9]{4}$"
```

### Kyverno 策略（替代方案）

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-probes
spec:
  validationFailureAction: Enforce
  background: true
  rules:
    - name: check-readiness-probe
      match:
        resources:
          kinds: ["Deployment"]
          namespaces: ["production", "staging"]
      validate:
        message: "生产/预发环境 Deployment 必须配置 readinessProbe"
        pattern:
          spec:
            template:
              spec:
                containers:
                  - readinessProbe:
                      httpGet:
                        path: "?*"
```

## 平台度量体系

### DORA 指标

| 指标 | 定义 | Elite 基准 | 采集方式 |
|------|------|-----------|----------|
| 部署频率 | 每周部署次数 | > 1/天 | ArgoCD sync 事件 |
| 变更前置时间 | 提交到生产的时间 | < 1 小时 | Git commit → ArgoCD sync |
| 变更失败率 | 导致回滚的部署比例 | < 5% | Rollback 事件 |
| 恢复时间 | 故障到恢复的时间 | < 1 小时 | 告警到解决 |

### 平台健康度量

```promql
# 部署频率（ArgoCD）
sum(rate(argocd_app_sync_total{operation="sync"}[7d])) by (dest_namespace)

# 变更前置时间（自定义指标）
platform_deployment_lead_time_seconds

# 平台采纳率
count(kube_deployment_labels{label_platform_managed="true"}) / count(kube_deployment_info)

# 自助服务使用率
sum(rate(backstage_scaffolder_template_executions_total[7d]))
```

## 治理运营节奏

| 频率 | 活动 | 参与者 |
|------|------|--------|
| 每周 | 平台 Office Hours | 平台团队 + 开发者 |
| 双周 | Golden Path 更新评审 | 平台团队 |
| 每月 | 成熟度评分报告 | 各团队 Lead |
| 季度 | 平台路线图规划 | 全组织 |
| 季度 | GameDay / 混沌演练 | SRE + 开发 |

## 故障排查

### 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| Golden Path 模板创建失败 | 参数验证不通过 | 检查 `pattern` 正则表达式 |
| 策略拒绝合法资源 | 策略太严格 | 调整 `excludedNamespaces` |
| 成熟度评分不准确 | 检测脚本未覆盖所有场景 | 更新 `check-maturity.sh` |
| Backstage 模板不显示 | 权限或注册问题 | 检查 `catalog-info.yaml` |
| OPA 策略不生效 | ConstraintTemplate 未应用 | `kubectl get constrainttemplate` |

### 调试命令

```bash
# 检查 Gatekeeper 状态
kubectl get pods -n gatekeeper-system
kubectl get constrainttemplate
kubectl get constraints

# 检查 Kyverno 状态
kubectl get pods -n kyverno
kubectl get clusterpolicy
kubectl get policy -A

# 测试策略
kubectl apply --dry-run=server -f test-deployment.yaml

# 查看策略拒绝事件
kubectl get events -A --field-selector reason=FailedValidation
```

## 最佳实践

| 实践 | 说明 |
|------|------|
| 渐进式强制 | 新策略先 Audit 后 Enforce |
| 豁免机制 | 为特殊场景提供审批豁免 |
| 开发者反馈 | 定期收集 Golden Path 使用体验 |
| 自动化检测 | 成熟度评分自动化运行 |
| 文档同步 | 策略变更同步更新文档 |
| 度量驱动 | 用数据证明平台价值 |

## Related

- [[10-平台工程/05-内部开发者平台/01-idp-architecture-backstage.md|IDP 架构]]
- [[10-平台工程/03-治理/index.md|治理]]
- [[08-安全/04-策略治理/index.md|策略治理]]
