---
title: Multi-Environment Deployment Strategy — Dev to Production Pipeline
description: K8s 多环境部署 — 环境拓扑设计、Promotion 策略、配置管理、环境隔离、GitOps 多环境编排
summary: 从开发到生产的多环境部署策略，涵盖环境设计、Promotion 流程与 GitOps 编排
category: practice
tags:
- multi-environment
- deployment
- promotion
- gitops
- pipeline
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: intermediate
domain: release
---
# 多环境部署策略 — 从开发到生产

> 构建标准化的多环境部署流水线与环境管理体系。

## 环境拓扑

```
┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐
│   Dev   │───▶│ Staging │───▶│  Pre-   │───▶│  Prod   │
│ (开发)  │    │ (预发)  │    │  Prod   │    │ (生产)  │
│         │    │         │    │ (灰度)  │    │         │
│ 共享集群│    │ 独立 NS │    │ 镜像Prod│    │ 独立集群│
│ 缩零    │    │ 真实数据│    │ 真实流量│    │ 多 AZ   │
└─────────┘    └─────────┘    └─────────┘    └─────────┘
```

## 环境配置矩阵

| 维度 | Dev | Staging | Pre-Prod | Production |
|------|-----|---------|----------|------------|
| 集群 | 共享 | 共享/独立 | 独立 | 独立多 AZ |
| 副本数 | 1 | 2 | 2 | 3-50 |
| 资源 | 最小 | 中等 | 同 Prod | 按需 |
| 数据 | Mock/Seed | 脱敏副本 | 只读副本 | 真实 |
| 域名 | *.dev.internal | *.staging.example.com | *.pre.example.com | *.example.com |
| 日志级别 | debug | info | info | warn |
| 采样率 | 100% | 50% | 20% | 10% |
| 缩零 | ✅ 非工作时间 | ❌ | ❌ | ❌ |
| 审批 | 无 | 无 | 自动 | 手动/自动 |

## GitOps 多环境编排

### 仓库结构

```
gitops-repo/
├── apps/
│   ├── order-service/
│   │   ├── base/
│   │   │   ├── kustomization.yaml
│   │   │   ├── deployment.yaml
│   │   │   ├── service.yaml
│   │   │   └── hpa.yaml
│   │   └── overlays/
│   │       ├── dev/
│   │       │   └── kustomization.yaml
│   │       ├── staging/
│   │       │   └── kustomization.yaml
│   │       └── production/
│   │           └── kustomization.yaml
│   └── payment-service/
│       └── ...
├── platform/
│   ├── monitoring/
│   ├── logging/
│   └── service-mesh/
└── clusters/
    ├── dev/
    │   └── kustomization.yaml
    ├── staging/
    │   └── kustomization.yaml
    └── production/
        └── kustomization.yaml
```

### ArgoCD ApplicationSet（多环境）

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: order-service
  namespace: argocd
spec:
  generators:
    - list:
        elements:
          - env: dev
            cluster: https://dev-cluster:6443
            namespace: dev
          - env: staging
            cluster: https://staging-cluster:6443
            namespace: staging
          - env: production
            cluster: https://prod-cluster:6443
            namespace: production
  template:
    metadata:
      name: 'order-service-{{env}}'
    spec:
      project: default
      source:
        repoURL: https://github.com/myorg/gitops-repo.git
        targetRevision: main
        path: 'apps/order-service/overlays/{{env}}'
      destination:
        server: '{{cluster}}'
        namespace: '{{namespace}}'
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
          - CreateNamespace=true
```

## Promotion 策略

### 自动 Promotion（CI/CD Pipeline）

```yaml
# GitHub Actions — 多环境 Promotion
name: Deploy Pipeline
on:
  push:
    tags: ['v*']

jobs:
  deploy-dev:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Update Dev
        run: |
          cd gitops-repo/apps/order-service/overlays/dev
          kustomize edit set image registry.example.com/order-service=${GITHUB_REF_NAME}
          git add . && git commit -m "deploy: order-service ${GITHUB_REF_NAME} to dev"
          git push

  deploy-staging:
    needs: deploy-dev
    runs-on: ubuntu-latest
    environment: staging  # 自动（无审批）
    steps:
      - name: Wait for Dev healthy
        run: |
          sleep 120  # 等待 Dev 稳定
          curl -sf https://order-service.dev.internal/health
      - name: Update Staging
        run: |
          cd gitops-repo/apps/order-service/overlays/staging
          kustomize edit set image registry.example.com/order-service=${GITHUB_REF_NAME}
          git add . && git commit -m "promote: order-service ${GITHUB_REF_NAME} to staging"
          git push

  deploy-production:
    needs: deploy-staging
    runs-on: ubuntu-latest
    environment: production  # 需要审批
    steps:
      - name: Smoke test staging
        run: ./scripts/smoke-test.sh staging
      - name: Update Production
        run: |
          cd gitops-repo/apps/order-service/overlays/production
          kustomize edit set image registry.example.com/order-service=${GITHUB_REF_NAME}
          git add . && git commit -m "promote: order-service ${GITHUB_REF_NAME} to production"
          git push
```

### 手动 Promotion（ArgoCD UI）

```bash
# CLI Promotion
argocd app sync order-service-staging
# 验证后
argocd app sync order-service-production

# 带金丝雀的 Promotion
kubectl argo rollouts promote order-service -n production
```

## 环境隔离

### Dev 环境缩零（KEDA Cron）

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: dev-scale-to-zero
  namespace: dev
spec:
  scaleTargetRef:
    name: order-service
  minReplicaCount: 0
  maxReplicaCount: 3
  triggers:
    - type: cron
      metadata:
        timezone: Asia/Shanghai
        start: "0 9 * * 1-5"
        end: "0 20 * * 1-5"
        desiredReplicas: "1"
```

### 环境间网络隔离

```yaml
# 禁止 Dev 访问 Production 数据库
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: deny-cross-env
  namespace: production
spec:
  podSelector:
    matchLabels:
      tier: database
  policyTypes: ["Ingress"]
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              environment: production
```

## 配置管理

### 环境特定配置

```yaml
# overlays/production/kustomization.yaml
configMapGenerator:
  - name: app-config
    behavior: merge
    literals:
      - LOG_LEVEL=warn
      - TRACING_SAMPLE_RATE=0.1
      - DB_POOL_SIZE=50
      - CACHE_TTL=3600
secretGenerator:
  - name: app-secrets
    behavior: merge
    envs:
      - secrets.env  # 通过 helm-secrets/SOPS 加密
```

### 外部配置（ConfigMap + Vault）

```yaml
# 非敏感配置 → ConfigMap
# 敏感配置 → External Secrets Operator → Vault
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: app-secrets
  namespace: production
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: vault-backend
    kind: ClusterSecretStore
  target:
    name: app-secrets
  data:
    - secretKey: DB_PASSWORD
      remoteRef:
        key: secret/data/production/order-service
        property: db_password
    - secretKey: API_KEY
      remoteRef:
        key: secret/data/production/order-service
        property: api_key
```

## 最佳实践

| 实践 | 说明 |
|------|------|
| 镜像不可变 | 同一镜像从 Dev 到 Prod |
| 配置外置 | 不烘焙到镜像中 |
| 自动 Promotion | Dev → Staging 无需人工 |
| 生产审批 | Production 需审批或金丝雀 |
| 环境一致性 | 基础设施用同一模板 |
| 快速回滚 | 任何环境 < 2min 回滚 |
| 数据隔离 | 环境间数据不互通 |
| 成本意识 | Dev 缩零、Spot 实例 |

## 故障排查表

| 问题现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|---------|
| Staging 正常但 Prod 异常 | 配置差异/资源规格不同 | `diff <(kubectl get deploy -n staging -o yaml) <(kubectl get deploy -n prod -o yaml)` | 统一基础模板，仅 overlay 差异 |
| Promotion 流水线卡住 | 审批未通过或 Gate 失败 | `kubectl get analysisrun -A` / CI 面板 | 检查审批人、补充测试覆盖 |
| 环境间镜像版本不一致 | 镜像 tag 被覆盖或缓存 | `kubectl get deploy -o jsonpath='{.spec.template.spec.containers[0].image}'` | 使用不可变 tag（SHA digest） |
| Dev 环境资源浪费 | 缩零策略未配置 | `kubectl get pods -n dev --field-selector=status.phase=Running` | 配置 CronHPA 非工作时间缩零 |
| 数据库迁移阻塞部署 | 长事务锁表 | `SELECT * FROM pg_stat_activity WHERE state='active'` | 使用 expand-contract 模式，避免锁表 |
| 环境 DNS 解析错误 | Service 名称或 Namespace 硬编码 | `nslookup <svc>.<ns>.svc.cluster.local` | 使用 Kustomize 变量替换 |

## 环境 Promotion 流水线设计

```
┌─────────┐    自动     ┌─────────┐   审批+金丝雀  ┌─────────┐
│   Dev   │──────────▶│ Staging │──────────────▶│   Prod  │
└─────────┘  PR merge  └─────────┘  AnalysisRun  └─────────┘
     │                      │                        │
  缩零策略             持久运行                  蓝绿/金丝雀
  Spot 实例           脱敏数据                  全量监控
```

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| 镜像策略 | 不可变镜像 + SHA digest | 杜绝 tag 覆盖导致环境不一致 |
| 配置管理 | Kustomize overlay / Helm values | 基础模板 + 环境差异层 |
| 数据管理 | 独立数据库实例，禁止跨环境访问 | 防止 Dev 误操作影响 Prod |
| 资源规划 | Prod 固定节点池，Dev 使用 Spot | 成本优化同时保障生产稳定 |
| 网络隔离 | NetworkPolicy 限制跨 Namespace 通信 | 环境间零信任 |
| 发布窗口 | Prod 变更避开业务高峰 | 结合 Change Freeze 日历 |
| 回滚 SLA | 任何环境 < 2min 完成回滚 | 预置回滚脚本 + 演练 |

## 相关工具

| 工具 | 用途 | 场景 |
|------|------|------|
| Kustomize | 多环境配置 overlay | 无模板引擎的声明式差异 |
| Helm | 参数化部署 | 复杂应用的值文件管理 |
| ArgoCD Image Updater | 自动镜像版本追踪 | Dev/Staging 自动升级 |
| Flagger | 金丝雀 Promotion 自动化 | Prod 渐进式发布 |
| kubeseal | 加密 Secret 跨环境共享 | 安全地同步凭证 |
| Skaffold | 本地开发到集群部署 | 开发环境快速迭代 |
| Tilt | 开发环境实时同步 | 微服务本地联调 |

## 快速检查脚本

```bash
#!/bin/bash
# 多环境一致性检查
echo "=== 镜像版本对比 ==="
for ns in dev staging prod; do
  echo "[$ns] $(kubectl get deploy -n $ns -o jsonpath='{.items[*].spec.template.spec.containers[*].image}' 2>/dev/null)"
done
echo "=== 副本数对比 ==="
for ns in dev staging prod; do
  echo "[$ns] $(kubectl get deploy -n $ns -o jsonpath='{.items[*].spec.replicas}' 2>/dev/null)"
done
```

## Related

- [[11-发布变更/06-部署方案/index.md|部署方案]]
- [[11-发布变更/01-GitOps/index.md|GitOps]]
- [[11-发布变更/03-Progressive-Delivery/index.md|Progressive Delivery]]
