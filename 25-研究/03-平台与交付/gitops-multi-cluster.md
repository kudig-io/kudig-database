---
title: GitOps 多集群部署模式研究
summary: 深入研究 ArgoCD 和 Flux 在多集群 GitOps 场景下的架构模式、同步策略和最佳实践。
category: research
tags:
- research
- gitops
- argocd
- flux
- multi-cluster
- deployment
tier: supporting
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
status: done
---

# GitOps 多集群部署模式研究

## 研究背景

GitOps 已成为 Kubernetes 集群中应用部署的事实标准。从单集群 GitOps 扩展到多集群 GitOps 时，架构决策对可维护性、安全性和性能影响深远：

- **Hub 集群瓶颈**：中心 ArgoCD 实例管理数十个集群时性能和可用性如何保障？
- **配置差异化**：不同集群（dev/staging/prod、不同地域）如何管理配置差异？
- **权限隔离**：不同团队的管理范围如何安全隔离？
| **Secret 管理**：跨集群 Secret 分发如何安全实现？

## 核心问题

1. ArgoCD 的 Pull 模型 vs Push 模型在多集群场景的性能差异？
2. ApplicationSet、App-of-Apps、Pull 模式三种多集群部署模式的优劣？
3. 多集群 Secret 管理方案（External Secrets、Sealed Secrets、SOPS）如何选型？
4. Flux 和 ArgoCD 在多集群能力上的差异和选型建议？

## 调研发现

### 发现一：三种多集群 GitOps 模式

```
模式一：ApplicationSet（推荐）
  ┌─────────────────┐
  │ Hub集群 ArgoCD   │
  │ ApplicationSet  │ → 集群 A (dev)
  │ (模板+生成器)    │ → 集群 B (staging)
  └─────────────────┘ → 集群 C (prod)

  优势：声明式、原生支持、模板复用
  劣势：Hub 单点

模式二：App-of-Apps（传统）
  ┌─────────────────┐
  │ Root Application │
  │  ├── App-dev     │ → 集群 A
  │  ├── App-staging │ → 集群 B
  │  └── App-prod    │ → 集群 C
  └─────────────────┘

  优势：灵活、可递归
  劣势：配置爆炸、维护复杂

模式三：Pull 模式（每集群独立 ArgoCD）
  ┌────────┐  ┌────────┐  ┌────────┐
  │集群 A   │  │集群 B   │  │集群 C   │
  │ArgoCD A │  │ArgoCD B │  │ArgoCD C │
  └───┬────┘  └───┬────┘  └───┬────┘
      └──────────┴───────────┘
              Git Repo

  优势：无单点、权限天然隔离
  劣势：运维开销（N 个 ArgoCD 实例）
```

### 发现二：模式选型矩阵

| 条件 | 推荐模式 | 理由 |
|------|---------|------|
| < 20 集群 | ApplicationSet | 简单、够用 |
| 20-100 集群 | ApplicationSet + ArgoCD HA | 需要 HA + 分片 |
| > 100 集群 | Pull 模式 | Hub 瓶颈不可接受 |
| 强权限隔离 | Pull 模式 | 每集群独立控制 |
| Air-gap 环境 | Pull 模式 | 无需 Hub 访问 Spoke |

### 发现三：Secret 管理方案对比

| 方案 | 原理 | Git 安全 | 多集群 | 轮换 | 推荐场景 |
|------|------|---------|--------|------|---------|
| **External Secrets** | 引用外部密钥管理（AWS SM/Vault） | ✅ 仅存引用 | ✅ | ✅ 自动 | 生产首选 |
| **Sealed Secrets** | 非对称加密存储在 Git | ✅ 加密 | ⚠️ 每集群独立密钥 | ❌ 手动 | 中小规模 |
| **SOPS + Flux/ArgoCD** | SOPS 加密 + GitOps 插件解密 | ✅ 加密 | ✅ | ⚠️ 手动 | 已有 SOPS 流程 |
| **HashiCorp Vault + Agent** | Vault 动态密钥注入 | ✅ | ✅ | ✅ 自动 | 已有 Vault |

**External Secrets 架构（推荐）**：

```yaml
# SecretStore 定义外部密钥源
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: aws-secrets-manager
spec:
  provider:
    aws:
      service: SecretsManager
      region: us-east-1
      auth:
        jwt:
          serviceAccountRef:
            name: external-secrets-sa  # IRSA 认证

---
# ExternalSecret 从 AWS 拉取密钥到 K8s Secret
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: database-credentials
spec:
  refreshInterval: 1h        # 自动轮换
  secretStoreRef:
    name: aws-secrets-manager
    kind: SecretStore
  target:
    name: db-secret
  data:
  - secretKey: password
    remoteRef:
      key: prod/db/password
```

### 发现四：ArgoCD vs Flux 多集群能力

| 能力 | ArgoCD | Flux |
|------|--------|------|
| **多集群管理** | ApplicationSet（原生） | FluxInstance + Fleet（v2） |
| **UI** | ✅ 完整 | ⚠️ 基础（需 weave-gitops） |
| **DRift 检测** | ✅ 实时 | ✅ 定时 |
| **同步策略** | Manual/Auto/Auto-Prune | 自动 |
| **差异化管理** | Kustomize/Helm overlays | Kustomize overlays |
| **多租户** | Projects + RBAC | 命名空间隔离 |
| **性能** | ⬤⬤⬤⬤ | ⬤⬤⬤⬤⬤（更轻量） |

### 发现五：生产环境配置差异化模式

```
gitops-repo/
├── apps/
│   └── web-app/
│       ├── base/                    # 通用配置
│       │   ├── deployment.yaml
│       │   ├── service.yaml
│       │   └── kustomization.yaml
│       └── overlays/
│           ├── dev/                 # 开发环境差异化
│           │   ├── patches.yaml
│           │   └── kustomization.yaml
│           ├── staging/
│           │   ├── patches.yaml
│           │   └── kustomization.yaml
│           └── prod/
│               ├── patches.yaml     # 生产副本数、资源、HPA
│               └── kustomization.yaml
├── clusters/                        # 集群级配置
│   ├── cluster-a/
│   └── cluster-b/
└── argocd/
    └── applicationset.yaml          # 多集群部署定义
```

## 结论与建议

1. **ApplicationSet 是中小规模（<20 集群）的最佳选择**：声明式、模板化、原生支持。
2. **Pull 模式适合大规模或强隔离需求**：避免 Hub 单点瓶颈。
3. **External Secrets 是 Secret 管理首选**：Git 仅存储引用，安全且支持自动轮换。
4. **Kustomize overlays 是配置差异化的标准方案**：base + overlays 模式清晰可维护。
5. **ArgoCD 和 Flux 均可**：需要 UI → ArgoCD；追求轻量 → Flux。

## 参考资料

- ArgoCD ApplicationSet: https://argo-cd.readthedocs.io/en/st/operator-manual/applicationset/
- Flux Multi-cluster: https://fluxcd.io/flux/cheatsheets/multi-cluster/
- External Secrets: https://external-secrets.io/
- [[11-发布变更/index.md|发布变更目录]]
- [[10-平台工程/index.md|平台工程目录]]
- [[25-研究/03-平台与交付/multi-cluster-management.md|多集群管理研究]]

## Related

- [[24-综合/02-交付与GitOps/argocd-gitops.md|ArgoCD × GitOps]]
- [[24-综合/02-交付与GitOps/helm-gitops.md|Helm × GitOps]]
