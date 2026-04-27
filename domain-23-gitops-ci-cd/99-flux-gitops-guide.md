# Flux GitOps 实践指南

> **适用版本**: Flux v2.5 (Flux CD)  
> **最后更新**: 2026-04-24  
> **难度**: 中级 → 高级

---

## 📋 目录

- [一、Flux 架构概览](#一flux-架构概览)
- [二、Bootstrap 部署](#二bootstrap-部署)
- [三、Source 与 Kustomization](#三source-与-kustomization)
- [四、HelmRelease 管理](#四helmrelease-管理)
- [五、多租户与 RBAC](#五多租户与-rbac)
- [六、镜像自动更新](#六镜像自动更新)
- [七、Flux vs Argo CD 对比](#七flux-vs-argo-cd-对比)
- [八、通知与告警](#八通知与告警)
- [九、与 Terraform / Crossplane 集成](#九与-terraform--crossplane-集成)

---

## 一、Flux 架构概览

```
Flux 核心组件
├── Source Controller
│   ├── GitRepository     ← 从 Git 拉取清单
│   ├── HelmRepository    ← 从 Helm repo 拉取 chart
│   ├── OCIRepository     ← 从 OCI registry 拉取
│   └── Bucket            ← 从 S3/MinIO 拉取
│
├── Kustomize Controller  ← 执行 kustomize build + apply
├── Helm Controller       ← 管理 HelmRelease 生命周期
├── Image Automation
│   ├── ImageRepository   ← 扫描镜像仓库 tag
│   ├── ImagePolicy       ← 定义更新策略 (semver)
│   └── ImageUpdateAutomation ← 自动提交 Git 更新
│
├── Notification Controller ← 事件 Webhook 通知
└── RBAC / ServiceAccount   ← 多租户权限隔离
```

---

## 二、Bootstrap 部署

### 2.1 CLI 安装与初始化

```bash
# 安装 flux CLI
curl -s https://fluxcd.io/install.sh | sudo bash

# 验证集群兼容性
flux check --pre

# Bootstrap (GitHub 示例)
export GITHUB_TOKEN=<your-token>
export GITHUB_USER=<your-username>

flux bootstrap github \
  --owner=$GITHUB_USER \
  --repository=flux-gitops \
  --branch=main \
  --path=clusters/production \
  --personal \
  --components-extra=image-reflector-controller,image-automation-controller

# Bootstrap (GitLab 示例)
flux bootstrap gitlab \
  --owner=$GITLAB_GROUP \
  --repository=flux-gitops \
  --branch=main \
  --path=clusters/production \
  --token-auth
```

### 2.2 目录结构约定

```
flux-gitops/
├── clusters/
│   ├── production/
│   │   ├── flux-system/          # Flux 自身配置 (bootstrap 生成)
│   │   ├── infrastructure.yaml   # 基础设施 Kustomization
│   │   └── apps.yaml             # 应用 Kustomization
│   └── staging/
│       └── ...
├── infrastructure/
│   ├── base/
│   │   ├── ingress-nginx/
│   │   ├── cert-manager/
│   │   ├── monitoring/
│   │   └── kyverno/
│   └── production/
│       ├── kustomization.yaml
│       └── patches/
└── apps/
    ├── base/
    │   ├── frontend/
    │   └── backend/
    └── production/
        ├── kustomization.yaml
        └── patches/
```

---

## 三、Source 与 Kustomization

### 3.1 GitRepository

```yaml
apiVersion: source.toolkit.fluxcd.io/v1
kind: GitRepository
metadata:
  name: app-source
  namespace: flux-system
spec:
  interval: 1m
  url: https://github.com/org/app-manifests
  ref:
    branch: main
  secretRef:
    name: github-token
  ignore: |
    # 忽略路径
    /docs/
    /tests/
```

### 3.2 Kustomization

```yaml
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: infrastructure
  namespace: flux-system
spec:
  interval: 10m
  path: ./infrastructure/production
  prune: true                    # 删除 Git 中不存在的资源
  sourceRef:
    kind: GitRepository
    name: flux-system
  healthChecks:
    - apiVersion: apps/v1
      kind: Deployment
      name: ingress-nginx-controller
      namespace: ingress-nginx
  timeout: 5m
  retryInterval: 2m
  # 多租户: 指定目标 ServiceAccount
  serviceAccountName: flux-infra
  # 依赖管理
  dependsOn:
    - name: cert-manager
```

### 3.3 健康检查与等待

```yaml
spec:
  wait: true                     # 等待所有资源就绪
  healthChecks:
    - apiVersion: apps/v1
      kind: Deployment
      name: myapp
      namespace: production
```

---

## 四、HelmRelease 管理

### 4.1 HelmRepository + HelmRelease

```yaml
apiVersion: source.toolkit.fluxcd.io/v1
kind: HelmRepository
metadata:
  name: prometheus-community
  namespace: flux-system
spec:
  interval: 1h
  url: https://prometheus-community.github.io/helm-charts
---
apiVersion: helm.toolkit.fluxcd.io/v2
kind: HelmRelease
metadata:
  name: kube-prometheus-stack
  namespace: monitoring
spec:
  interval: 30m
  chart:
    spec:
      chart: kube-prometheus-stack
      version: "70.x"             # 语义化版本约束
      sourceRef:
        kind: HelmRepository
        name: prometheus-community
        namespace: flux-system
  install:
    remediation:
      retries: 3
  upgrade:
    remediation:
      retries: 3
      remediateLastFailure: true
    cleanupOnFail: true
  values:
    prometheus:
      prometheusSpec:
        retention: 30d
        storageSpec:
          volumeClaimTemplate:
            spec:
              storageClassName: gp3
              resources:
                requests:
                  storage: 50Gi
```

### 4.2 OCI Chart 支持

```yaml
apiVersion: source.toolkit.fluxcd.io/v1
kind: OCIRepository
metadata:
  name: podinfo
  namespace: flux-system
spec:
  interval: 5m
  url: oci://ghcr.io/stefanprodan/charts/podinfo
  ref:
    semver: "6.x"
---
apiVersion: helm.toolkit.fluxcd.io/v2
kind: HelmRelease
metadata:
  name: podinfo
spec:
  chartRef:
    kind: OCIRepository
    name: podinfo
```

---

## 五、多租户与 RBAC

### 5.1 团队级隔离

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: flux-team-backend
  namespace: flux-system
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: flux-team-backend
  namespace: backend
subjects:
  - kind: ServiceAccount
    name: flux-team-backend
    namespace: flux-system
roleRef:
  kind: ClusterRole
  name: cluster-admin
  apiGroup: rbac.authorization.k8s.io
---
apiVersion: kustomize.toolkit.fluxcd.io/v1
kind: Kustomization
metadata:
  name: backend-apps
  namespace: flux-system
spec:
  serviceAccountName: flux-team-backend
  path: ./apps/backend
  prune: true
```

### 5.2 受限权限模板

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: flux-limited-deployer
rules:
  - apiGroups: ["apps"]
    resources: ["deployments"]
    verbs: ["*"]
  - apiGroups: [""]
    resources: ["services", "configmaps", "secrets"]
    verbs: ["*"]
```

---

## 六、镜像自动更新

### 6.1 配置镜像扫描与策略

```yaml
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImageRepository
metadata:
  name: myapp
  namespace: flux-system
spec:
  image: ghcr.io/org/myapp
  interval: 1m
  exclusionList:
    - "^.*-rc\\..*$"
    - "^.*-alpha\\..*$"
---
apiVersion: image.toolkit.fluxcd.io/v1beta2
kind: ImagePolicy
metadata:
  name: myapp
  namespace: flux-system
spec:
  imageRepositoryRef:
    name: myapp
  policy:
    semver:
      range: "1.x.x"
  filterTags:
    pattern: '^v(?P<version>.*)$'
    extract: '$version'
```

### 6.2 自动提交更新

```yaml
apiVersion: image.toolkit.fluxcd.io/v1beta1
kind: ImageUpdateAutomation
metadata:
  name: flux-system
  namespace: flux-system
spec:
  interval: 1m
  sourceRef:
    kind: GitRepository
    name: flux-system
  git:
    checkout:
      ref:
        branch: main
    commit:
      author:
        name: Flux Bot
        email: flux@example.com
      messageTemplate: |
        Automated image update
        
        Images:
        {{ range .Updated.Images -}}
        - {{.}}
        {{ end }}
      signingKey:
        secretRef:
          name: flux-gpg-signing-key
    push:
      branch: main
```

### 6.3 在 Kustomization 中标记可更新字段

```yaml
spec:
  template:
    spec:
      containers:
      - name: frontend
        image: ghcr.io/org/frontend:v1.0.0
        # flux-image-policy: flux-system:frontend
```

---

## 七、Flux vs Argo CD 对比

| 维度 | Flux | Argo CD |
|:---|:---|:---|
| **架构** | 纯 GitOps (无 UI 必需) | GitOps + UI |
| **多集群** | 每个集群独立实例 | 单实例管理多集群 |
| **镜像自动更新** | 内置 (成熟) | 需 Argo Image Updater |
| **UI** | 可选 Weave GitOps | 内置丰富 UI |
| **Helm 支持** | Helm Controller (原生) | 内置 Helm 支持 |
| **Secret 管理** | SOPS/Mozilla (原生) | Sealed Secrets / External Secrets |
| **通知** | Notification Controller | Argo Notifications |
| **规模** | 适合中小规模 (<100 apps) | 适合大规模 (1000+ apps) |
| **企业支持** | Weaveworks (Flux 创始公司) | Red Hat (收购 Akuity) |
| **多租户** | Namespace 级 RBAC | Project 级 RBAC |

### 选型决策

```
选择 Flux 如果:
  ✅ 偏好纯 GitOps，不依赖 UI
  ✅ 需要内置镜像自动更新
  ✅ 使用 GitHub/GitLab 原生集成
  ✅ 团队规模较小，应用 < 100

选择 Argo CD 如果:
  ✅ 需要集中式多集群管理
  ✅ 需要丰富 UI 和可视化
  ✅ 应用规模 > 100
  ✅ 需要 ApplicationSet 和 Generators
```

---

## 八、通知与告警

### 8.1 Provider + Alert 配置

```yaml
apiVersion: notification.toolkit.fluxcd.io/v1beta3
kind: Provider
metadata:
  name: slack
  namespace: flux-system
spec:
  type: slack
  channel: k8s-alerts
  secretRef:
    name: slack-webhook-url
---
apiVersion: notification.toolkit.fluxcd.io/v1beta3
kind: Alert
metadata:
  name: flux-alerts
  namespace: flux-system
spec:
  summary: "Production Cluster"
  providerRef:
    name: slack
  eventSeverity: error
  eventSources:
    - kind: Kustomization
      name: "*"
    - kind: HelmRelease
      name: "*"
  inclusionList:
    - "Kustomization.*reconciliation failed"
    - "HelmRelease.*install retries exhausted"
```

### 8.2 Prometheus Metrics

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: flux-system
  namespace: monitoring
spec:
  namespaceSelector:
    matchNames:
      - flux-system
  selector:
    matchLabels:
      app.kubernetes.io/part-of: flux
  endpoints:
    - port: http-prom
      interval: 30s
```

| 关键指标 | PromQL |
|:---|:---|
| 同步失败 | gotk_reconcile_condition{status="False",type="Ready"} == 1 |
| 同步耗时 | histogram_quantile(0.95, rate(gotk_reconcile_duration_seconds_bucket[5m])) |
| 源同步状态 | gotk_resource_info{kind="GitRepository"} |

---

## 九、与 Terraform / Crossplane 集成

### 9.1 Terraform Controller

```yaml
apiVersion: infra.contrib.fluxcd.io/v1alpha2
kind: Terraform
metadata:
  name: aws-vpc
  namespace: flux-system
spec:
  interval: 1h
  path: ./terraform/vpc
  sourceRef:
    kind: GitRepository
    name: flux-system
  approvePlan: auto
  vars:
    - name: region
      value: us-east-1
  writeOutputsToSecret:
    name: vpc-outputs
```

### 9.2 与 Crossplane 协作

```
Git Repo
  ├── infrastructure/        ← Flux 管理 (基础设置)
  │   ├── crossplane/
  │   │   ├── providers/
  │   │   └── compositions/
  │   └── flux-system/
  └── platform/              ← Crossplane 管理 (云资源)
      ├── claims/
      └── xrd/
```

---

## 参考链接

- [Flux 官方文档](https://fluxcd.io/flux/)
- [Flux GitHub](https://github.com/fluxcd/flux2)
- [Weave GitOps (UI)](https://docs.gitops.weave.works/)
- [Image Automation Guide](https://fluxcd.io/flux/guides/image-update/)
