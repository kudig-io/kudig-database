# 包管理工具

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [Helm Docs](https://helm.sh/docs/) | [Kustomize](https://kustomize.io/)

## 工具对比

| 工具 | 类型 | 适用场景 | 学习曲线 | 生态成熟度 | 生产推荐度 |
|------|------|----------|----------|-----------|-----------|
| **Helm** | 模板化包管理 | 复杂应用、多环境部署 | 中等 | ⭐⭐⭐⭐⭐ | 强烈推荐 |
| **Kustomize** | 声明式配置 | 原生YAML管理、GitOps | 低 | ⭐⭐⭐⭐ | 推荐 |
| **Carvel ytt** | YAML模板 | 复杂逻辑、多层覆盖 | 高 | ⭐⭐⭐ | 特定场景 |
| **Jsonnet** | 配置语言 | 大规模标准化配置 | 高 | ⭐⭐⭐ | 特定场景 |

---

## Helm 生产实践

### 核心概念

| 概念 | 说明 | 最佳实践 |
|------|------|----------|
| **Chart** | 应用包结构 | 遵循标准目录结构、版本语义化 |
| **Release** | 部署实例 | 命名规范、环境标识 |
| **Repository** | Chart仓库 | 私有仓库+OCI镜像仓库 |
| **Values** | 配置参数 | 分层管理、敏感信息外置 |

### Helm Chart 标准结构

```yaml
mychart/
  Chart.yaml          # Chart元数据
  values.yaml         # 默认配置
  values.schema.json  # 配置校验规则(推荐)
  charts/             # 依赖Chart
  templates/          # 模板文件
    NOTES.txt         # 安装提示
    deployment.yaml
    service.yaml
    ingress.yaml
    _helpers.tpl      # 模板函数
  crds/               # CRD定义(v1.15+)
  .helmignore
```

### Chart.yaml 生产配置

```yaml
apiVersion: v2
name: myapp
version: 1.2.3        # Chart版本
appVersion: "2.5.1"   # 应用版本
description: 生产级应用Chart
type: application
keywords:
  - production
  - microservice
home: https://github.com/org/myapp
sources:
  - https://github.com/org/myapp
maintainers:
  - name: SRE Team
    email: sre@company.com
dependencies:
  - name: postgresql
    version: 12.x.x
    repository: https://charts.bitnami.com/bitnami
    condition: postgresql.enabled  # 可选依赖
annotations:
  category: Application
  licenses: Apache-2.0
```

### 多环境配置管理

```bash
# 目录结构
envs/
  values-dev.yaml
  values-staging.yaml
  values-prod.yaml

# 分层安装
helm install myapp ./mychart \
  -f values.yaml \
  -f envs/values-prod.yaml \
  --set image.tag=v2.5.1 \
  -n production
```

### values-prod.yaml 示例

```yaml
replicaCount: 3

image:
  repository: registry.company.com/myapp
  pullPolicy: IfNotPresent
  tag: ""  # 通过--set覆盖

resources:
  limits:
    cpu: 2000m
    memory: 4Gi
  requests:
    cpu: 1000m
    memory: 2Gi

autoscaling:
  enabled: true
  minReplicas: 3
  maxReplicas: 10
  targetCPUUtilizationPercentage: 70

ingress:
  enabled: true
  className: nginx
  annotations:
    cert-manager.io/cluster-issuer: letsencrypt-prod
  hosts:
    - host: app.company.com
      paths:
        - path: /
          pathType: Prefix
  tls:
    - secretName: app-tls
      hosts:
        - app.company.com

postgresql:
  enabled: false  # 生产使用外部数据库
  
externalDatabase:
  host: postgres.prod.svc.cluster.local
  port: 5432
  database: myapp
  existingSecret: myapp-db-secret
```

### Helm 运维命令

```bash
# 查看Chart可配置参数
helm show values bitnami/postgresql

# 模板渲染测试(不安装)
helm template myapp ./mychart -f values-prod.yaml

# Dry-run安装
helm install myapp ./mychart --dry-run --debug

# 升级并回滚
helm upgrade myapp ./mychart -f values-prod.yaml
helm rollback myapp 2  # 回滚到版本2

# 查看历史版本
helm history myapp -n production

# 导出已部署配置
helm get values myapp -n production > current-values.yaml
```

---

## Kustomize 生产实践

### 核心概念

| 概念 | 说明 | 优势 |
|------|------|------|
| **Base** | 基础配置 | 通用模板、DRY原则 |
| **Overlay** | 环境覆盖 | 环境差异化、配置继承 |
| **Patch** | 局部修改 | 精确控制、最小变更 |
| **Generator** | 动态生成 | ConfigMap/Secret版本化 |

### 标准目录结构

```
kustomize/
  base/
    kustomization.yaml
    deployment.yaml
    service.yaml
  overlays/
    dev/
      kustomization.yaml
      patch-replicas.yaml
    staging/
      kustomization.yaml
      patch-resources.yaml
    production/
      kustomization.yaml
      patch-replicas.yaml
      patch-resources.yaml
      sealed-secret.yaml
```

### base/kustomization.yaml

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

resources:
  - deployment.yaml
  - service.yaml
  - ingress.yaml

commonLabels:
  app: myapp
  managed-by: kustomize

commonAnnotations:
  company.com/team: platform

images:
  - name: myapp
    newName: registry.company.com/myapp
    newTag: latest  # Overlay中覆盖

configMapGenerator:
  - name: app-config
    files:
      - config.properties
    options:
      disableNameSuffixHash: false  # 自动添加hash后缀
```

### overlays/production/kustomization.yaml

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

namespace: production

bases:
  - ../../base

patchesStrategicMerge:
  - patch-replicas.yaml
  - patch-resources.yaml

patchesJson6902:
  - target:
      group: apps
      version: v1
      kind: Deployment
      name: myapp
    patch: |-
      - op: add
        path: /spec/template/spec/containers/0/env/-
        value:
          name: ENVIRONMENT
          value: production

images:
  - name: myapp
    newTag: v2.5.1  # 生产版本锁定

replicas:
  - name: myapp
    count: 3

secretGenerator:
  - name: db-credentials
    envs:
      - secrets.env
    options:
      disableNameSuffixHash: true

resources:
  - sealed-secret.yaml  # SealedSecrets加密配置
```

### patch-resources.yaml

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  template:
    spec:
      containers:
        - name: myapp
          resources:
            limits:
              cpu: 2000m
              memory: 4Gi
            requests:
              cpu: 1000m
              memory: 2Gi
```

### Kustomize 部署命令

```bash
# 本地渲染预览
kubectl kustomize overlays/production

# 直接部署
kubectl apply -k overlays/production

# 配合Helm使用
helm install myapp ./chart \
  --post-renderer kustomize

# 验证配置差异
diff <(kubectl kustomize overlays/dev) \
     <(kubectl kustomize overlays/production)
```

---

## Carvel 工具套件

### ytt - YAML模板工具

```yaml
#@ load("@ytt:data", "data")
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: #@ data.values.appName
spec:
  replicas: #@ data.values.replicas
  template:
    spec:
      containers:
        - name: app
          image: #@ "{}/{}:{}".format(
            data.values.registry,
            data.values.image,
            data.values.tag
          )
```

### values.yaml

```yaml
#@data/values
---
appName: myapp
replicas: 3
registry: registry.company.com
image: myapp
tag: v2.5.1
```

### 渲染与部署

```bash
ytt -f config/ -f values.yaml | kubectl apply -f-
```

---

## 工具选型决策树

```
是否需要复杂模板逻辑?
├─ 是 → Helm (成熟生态、Chart Hub)
│   └─ 需要更强逻辑? → ytt (Starlark编程)
└─ 否 → Kustomize (原生集成、GitOps友好)
    └─ 需要更复杂配置? → Jsonnet (大规模标准化)
```

---

## 生产建议

| 场景 | 推荐方案 | 理由 |
|------|---------|------|
| 新项目 | Helm + Kustomize | Helm管理依赖，Kustomize差异化 |
| GitOps | Kustomize优先 | ArgoCD/Flux原生支持 |
| 多团队 | Helm Chart标准化 | 可复用、易分发 |
| 复杂逻辑 | ytt或Jsonnet | 编程能力强 |
| 快速迁移 | Helm (现有Chart丰富) | 减少开发成本 |

---

## 常见问题

**Q: Helm vs Kustomize如何选择?**  
A: 不冲突，可组合使用。Helm适合打包分发，Kustomize适合环境差异化。

**Q: Helm Chart如何版本管理?**  
A: Chart版本独立于应用版本，使用语义化版本(SemVer)。

**Q: Kustomize如何管理Secret?**  
A: 配合Sealed Secrets或External Secrets Operator使用。
