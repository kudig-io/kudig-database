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

## Related

- [[发布变更/部署方案/index.md|部署方案]]
- [[发布变更/GitOps/index.md|GitOps]]
- [[发布变更/Progressive-Delivery/index.md|Progressive Delivery]]
