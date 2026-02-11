# 36 - 生态工具 (Kustomize / Helm / ArgoCD) YAML 配置参考

> **适用版本**: Kustomize v5.x / Helm v3.x / ArgoCD v2.x | **最后更新**: 2026-02 | **难度**: 入门 → 专家全覆盖

## 📋 概述

本文档提供 Kubernetes 生态工具的完整 YAML 配置参考，涵盖：
- **Kustomize**: 声明式配置管理
- **Helm**: 包管理和模板引擎
- **ArgoCD**: GitOps 持续部署

---

## 1️⃣ Kustomize 配置参考

### 1.1 kustomization.yaml 完整字段规范

```yaml
# kustomization.yaml - Kustomize 配置文件
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

# 基础资源引用
resources:
  - deployment.yaml                    # 本地 YAML 文件
  - service.yaml
  - https://example.com/manifest.yaml  # 远程 URL
  - github.com/org/repo?ref=v1.0.0    # Git 仓库

# 基础配置引用（多层 Kustomize）
bases:
  - ../base                            # 相对路径
  - github.com/org/repo/base          # 远程基础

# 组件引用（可选功能模块）
components:
  - components/monitoring              # 添加可选的监控组件
  - components/tls                     # 添加可选的 TLS 配置

# 名称前缀/后缀
namePrefix: dev-                       # 为所有资源添加前缀
nameSuffix: -v2                        # 为所有资源添加后缀

# 命名空间覆盖
namespace: my-app-dev                  # 为所有资源设置命名空间

# 通用标签（添加到所有资源）
commonLabels:
  app: my-application
  environment: production
  managed-by: kustomize

# 通用注解（添加到所有资源）
commonAnnotations:
  contact: sre-team@company.com
  version: "2.1.0"
  deployment-tool: kustomize

# 镜像替换（无需修改原始 YAML）
images:
  - name: nginx                        # 原始镜像名
    newName: my-registry.io/nginx      # 新镜像名
    newTag: 1.21.6                     # 新标签
  - name: redis
    newTag: 7.0-alpine
    digest: sha256:abc123...           # 使用摘要（优先级高于 tag）

# ConfigMap 生成器
configMapGenerator:
  - name: app-config                   # ConfigMap 名称
    namespace: default                 # 目标命名空间
    behavior: create                   # create | replace | merge
    literals:                          # 字面量键值对
      - LOG_LEVEL=info
      - MAX_CONNECTIONS=100
    files:                             # 从文件生成
      - config/app.properties
      - database.conf
    envs:                              # 从 .env 文件生成
      - config/.env
    options:
      labels:                          # 自定义标签
        app: myapp
      annotations:
        note: generated-by-kustomize
      disableNameSuffixHash: false     # 是否禁用哈希后缀

  - name: nginx-config                 # 另一个 ConfigMap
    files:
      - nginx.conf=configs/nginx.conf  # 自定义键名

# Secret 生成器
secretGenerator:
  - name: app-secret
    namespace: default
    type: Opaque                       # Secret 类型
    behavior: create
    literals:
      - DB_PASSWORD=changeme
      - API_KEY=secret123
    files:
      - tls.crt=certs/tls.crt
      - tls.key=certs/tls.key
    envs:
      - secrets/.env.secret
    options:
      labels:
        security: high
      annotations:
        vault-addr: https://vault.example.com
      disableNameSuffixHash: false

  - name: dockerconfig-secret          # Docker 注册表凭证
    type: kubernetes.io/dockerconfigjson
    files:
      - .dockerconfigjson=~/.docker/config.json

# 战略合并补丁（Strategic Merge Patch）
patchesStrategicMerge:
  - patch-deployment.yaml              # 部分 YAML，按键合并
  - |-
    apiVersion: apps/v1
    kind: Deployment
    metadata:
      name: my-app
    spec:
      replicas: 3                      # 覆盖副本数
      template:
        spec:
          containers:
          - name: app
            resources:                 # 合并资源限制
              limits:
                memory: 2Gi

# JSON 补丁（RFC 6902）
patchesJson6902:
  - target:
      group: apps
      version: v1
      kind: Deployment
      name: my-app
      namespace: default
    patch: |-
      - op: replace                    # 操作: add | remove | replace | move | copy | test
        path: /spec/replicas
        value: 5
      - op: add
        path: /spec/template/spec/containers/0/env/-
        value:
          name: NEW_VAR
          value: "new-value"
      - op: remove
        path: /spec/template/spec/containers/0/env/0

  - target:
      kind: Service
      name: my-service
    path: patch-service.json           # 从文件读取补丁

# 通用补丁（Patch）
patches:
  - target:                            # 目标选择器
      group: apps
      version: v1
      kind: Deployment
      name: my-app                     # 精确匹配名称
      labelSelector: app=myapp         # 标签选择器
      annotationSelector: patch=true   # 注解选择器
    patch: |-
      - op: add
        path: /spec/template/spec/nodeSelector
        value:
          disktype: ssd

  - path: patches/add-sidecar.yaml     # 从文件读取
    target:
      kind: Deployment

# 替换器（Replacements - 变量替换）
replacements:
  - source:                            # 数据源
      kind: ConfigMap
      name: env-config
      fieldPath: data.CLUSTER_NAME     # 源字段路径
    targets:                           # 目标列表
      - select:
          kind: Deployment
          name: my-app
        fieldPaths:
          - spec.template.spec.containers.[name=app].env.[name=CLUSTER_NAME].value
        options:
          delimiter: ':'               # 分隔符
          index: 0                     # 分割后的索引

  - source:
      kind: Service
      name: database
      fieldPath: metadata.name
    targets:
      - select:
          kind: Deployment
        fieldPaths:
          - spec.template.spec.containers.*.env.[name=DB_HOST].value

# Helm Chart 集成
helmCharts:
  - name: nginx-ingress                # Chart 名称
    repo: https://kubernetes.github.io/ingress-nginx  # Chart 仓库
    version: 4.7.1                     # Chart 版本
    releaseName: my-ingress            # Release 名称
    namespace: ingress-nginx
    valuesInline:                      # 内联 values
      controller:
        replicaCount: 2
        service:
          type: LoadBalancer
    valuesFile: values/nginx-values.yaml  # 外部 values 文件
    includeCRDs: true                  # 包含 CRDs

# 变量（已废弃，推荐使用 replacements）
# vars:
#   - name: SERVICE_NAME
#     objref:
#       kind: Service
#       name: my-service
#       apiVersion: v1
#     fieldref:
#       fieldpath: metadata.name

# 构建元数据
buildMetadata:
  - managedByLabel                     # 添加 app.kubernetes.io/managed-by 标签
  - originAnnotations                  # 添加来源注解

# 标签和注解转换器配置
configurations:
  - kustomizeconfig.yaml               # 自定义转换器配置

# 生成器选项（全局）
generatorOptions:
  disableNameSuffixHash: false         # 是否禁用所有生成器的哈希后缀
  labels:
    generated: "true"
  annotations:
    generated-by: kustomize

# 副本数覆盖
replicas:
  - name: my-app                       # Deployment 名称
    count: 3                           # 副本数

# OpenAPI 定义
openapi:
  path: https://k8s.io/api/openapi-spec/swagger.json  # OpenAPI schema 路径
```

### 1.2 Base + Overlays 目录结构模式

```bash
# 标准目录结构
my-app/
├── base/                              # 基础配置（环境无关）
│   ├── kustomization.yaml             # 基础 kustomization
│   ├── deployment.yaml                # 基础 Deployment
│   ├── service.yaml                   # 基础 Service
│   └── configmap.yaml                 # 基础 ConfigMap
│
├── overlays/                          # 环境特定覆盖
│   ├── dev/                           # 开发环境
│   │   ├── kustomization.yaml
│   │   ├── patch-replica.yaml         # 开发环境补丁
│   │   └── configmap-dev.yaml
│   │
│   ├── staging/                       # 预发布环境
│   │   ├── kustomization.yaml
│   │   ├── patch-replica.yaml
│   │   └── configmap-staging.yaml
│   │
│   └── production/                    # 生产环境
│       ├── kustomization.yaml
│       ├── patch-replica.yaml
│       ├── patch-resources.yaml       # 资源限制补丁
│       ├── configmap-prod.yaml
│       └── secret-prod.yaml           # 生产环境密钥
│
├── components/                        # 可选组件
│   ├── monitoring/                    # 监控组件
│   │   ├── kustomization.yaml
│   │   └── servicemonitor.yaml
│   │
│   └── tls/                           # TLS 组件
│       ├── kustomization.yaml
│       └── certificate.yaml
│
└── environments/                      # 替代方案：平铺式结构
    ├── dev.yaml
    ├── staging.yaml
    └── production.yaml
```

#### base/kustomization.yaml

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

resources:
  - deployment.yaml
  - service.yaml
  - configmap.yaml

# 基础标签
commonLabels:
  app: my-application
  managed-by: kustomize

# 基础镜像
images:
  - name: my-app
    newName: registry.example.com/my-app
    newTag: latest
```

#### overlays/production/kustomization.yaml

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

# 引用基础配置
bases:
  - ../../base

# 生产环境命名空间
namespace: production

# 生产环境前缀
namePrefix: prod-

# 生产环境标签
commonLabels:
  environment: production
  team: platform

# 生产环境镜像（覆盖基础）
images:
  - name: my-app
    newName: registry.example.com/my-app
    newTag: v1.2.3                     # 使用固定版本
    digest: sha256:abc123...           # 生产环境使用摘要

# 生产环境补丁
patchesStrategicMerge:
  - patch-replica.yaml                 # 增加副本数
  - patch-resources.yaml               # 增加资源限制

# 生产环境 ConfigMap
configMapGenerator:
  - name: app-config
    behavior: merge                    # 合并基础配置
    literals:
      - ENV=production
      - LOG_LEVEL=warn
      - REPLICAS=5

# 生产环境 Secret
secretGenerator:
  - name: app-secret
    files:
      - credentials=secrets/prod-credentials.txt
    options:
      disableNameSuffixHash: true      # 生产环境使用固定名称

# 引用可选组件
components:
  - ../../components/monitoring        # 启用监控
  - ../../components/tls               # 启用 TLS

# 副本数覆盖
replicas:
  - name: my-app
    count: 5
```

#### overlays/production/patch-resources.yaml

```yaml
# 生产环境资源限制补丁
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app                         # 名称匹配（不含前缀）
spec:
  template:
    spec:
      containers:
      - name: app
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: 2000m
            memory: 4Gi
      nodeSelector:
        node-type: production          # 生产节点选择器
      affinity:
        podAntiAffinity:               # 生产环境 Pod 反亲和性
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: my-application
            topologyKey: kubernetes.io/hostname
```

### 1.3 Components（组件）模式

#### components/monitoring/kustomization.yaml

```yaml
apiVersion: kustomize.config.k8s.io/v1alpha1
kind: Component                        # 注意：类型是 Component

# 添加监控资源
resources:
  - servicemonitor.yaml
  - prometheusrule.yaml

# 组件标签
commonLabels:
  monitoring: enabled

# 添加 Pod 注解（用于 Prometheus 抓取）
patches:
  - target:
      kind: Deployment
    patch: |-
      - op: add
        path: /spec/template/metadata/annotations
        value:
          prometheus.io/scrape: "true"
          prometheus.io/port: "8080"
          prometheus.io/path: "/metrics"
```

#### components/monitoring/servicemonitor.yaml

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: app-metrics
spec:
  selector:
    matchLabels:
      app: my-application
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
```

### 1.4 生产案例：多环境 Kustomize 配置

#### 案例：微服务应用多环境部署

```bash
microservice-app/
├── base/
│   ├── kustomization.yaml
│   ├── namespace.yaml
│   ├── api-deployment.yaml
│   ├── api-service.yaml
│   ├── worker-deployment.yaml
│   ├── redis-statefulset.yaml
│   └── redis-service.yaml
│
├── overlays/
│   ├── dev/
│   │   ├── kustomization.yaml
│   │   └── patches/
│   │       ├── api-dev.yaml
│   │       └── redis-dev.yaml
│   │
│   ├── staging/
│   │   ├── kustomization.yaml
│   │   ├── patches/
│   │   │   ├── api-staging.yaml
│   │   │   └── redis-staging.yaml
│   │   └── secrets/
│   │       └── db-credentials.enc.yaml
│   │
│   └── production/
│       ├── kustomization.yaml
│       ├── patches/
│       │   ├── api-prod.yaml
│       │   ├── worker-prod.yaml
│       │   └── redis-prod.yaml
│       ├── secrets/
│       │   └── db-credentials.enc.yaml
│       └── ingress.yaml
│
└── components/
    ├── istio/
    │   ├── kustomization.yaml
    │   ├── virtualservice.yaml
    │   └── destinationrule.yaml
    └── backup/
        ├── kustomization.yaml
        └── cronjob-backup.yaml
```

#### overlays/production/kustomization.yaml（完整生产配置）

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

bases:
  - ../../base

namespace: microservice-prod

namePrefix: prod-

commonLabels:
  environment: production
  team: backend
  cost-center: engineering

commonAnnotations:
  managed-by: kustomize
  contact: sre@example.com
  version: "v2.5.1"

images:
  - name: api-server
    newName: gcr.io/my-project/api-server
    newTag: v2.5.1
    digest: sha256:abc123def456...
  - name: worker
    newName: gcr.io/my-project/worker
    newTag: v2.5.1
    digest: sha256:789xyz012abc...
  - name: redis
    newName: redis
    newTag: 7.0-alpine

replicas:
  - name: api-deployment
    count: 10                          # 生产环境 10 副本
  - name: worker-deployment
    count: 5                           # Worker 5 副本

configMapGenerator:
  - name: api-config
    behavior: merge
    literals:
      - ENV=production
      - LOG_LEVEL=info
      - REDIS_HOST=prod-redis-service
      - API_TIMEOUT=30s
      - MAX_CONNECTIONS=1000
    files:
      - config.json=configs/prod-config.json

secretGenerator:
  - name: db-credentials
    files:
      - username=secrets/db-user.txt
      - password=secrets/db-password.txt
    options:
      disableNameSuffixHash: true      # 固定名称

  - name: api-keys
    envs:
      - secrets/api-keys.env
    options:
      disableNameSuffixHash: true

resources:
  - ingress.yaml                       # 生产环境 Ingress

patchesStrategicMerge:
  - patches/api-prod.yaml
  - patches/worker-prod.yaml
  - patches/redis-prod.yaml

components:
  - ../../components/istio             # 启用 Istio
  - ../../components/backup            # 启用备份

patches:
  - target:
      kind: Deployment
      labelSelector: tier=backend
    patch: |-
      - op: add
        path: /spec/template/spec/securityContext
        value:
          runAsNonRoot: true
          runAsUser: 1000
          fsGroup: 1000
          seccompProfile:
            type: RuntimeDefault

  - target:
      kind: Deployment
    patch: |-
      - op: add
        path: /spec/template/spec/topologySpreadConstraints
        value:
          - maxSkew: 1
            topologyKey: topology.kubernetes.io/zone
            whenUnsatisfiable: DoNotSchedule
            labelSelector:
              matchLabels:
                app: my-application

replacements:
  - source:
      kind: ConfigMap
      name: api-config
      fieldPath: data.REDIS_HOST
    targets:
      - select:
          kind: Deployment
          name: api-deployment
        fieldPaths:
          - spec.template.spec.containers.[name=api].env.[name=REDIS_HOST].value

generatorOptions:
  disableNameSuffixHash: false
  labels:
    generated: "true"
    environment: production
  annotations:
    generated-at: "2026-02-10"
```

---

## 2️⃣ Helm 配置参考

### 2.1 Chart.yaml 完整字段规范

```yaml
# Chart.yaml - Helm Chart 元数据文件
apiVersion: v2                         # Chart API 版本 (v2 for Helm 3)
name: my-application                   # Chart 名称（必需）
version: 1.2.3                         # Chart 版本（必需，SemVer 2）
appVersion: "2.5.1"                    # 应用版本（可选）

# Chart 类型
type: application                      # application | library

# Chart 描述
description: A Helm chart for my production application

# 关键词（用于搜索）
keywords:
  - web
  - api
  - microservice
  - production

# 主页
home: https://example.com

# 源代码仓库
sources:
  - https://github.com/myorg/my-app
  - https://github.com/myorg/helm-charts

# 依赖列表
dependencies:
  - name: postgresql                   # 依赖 Chart 名称
    version: 12.1.5                    # 依赖版本（SemVer 范围）
    repository: https://charts.bitnami.com/bitnami  # 仓库 URL
    condition: postgresql.enabled      # 条件启用（values.yaml 中的键）
    tags:                              # 标签（用于批量启用/禁用）
      - database
      - backend
    import-values:                     # 导入依赖的 values
      - child: postgresql.auth
        parent: auth.postgresql
    alias: postgres                    # 别名（允许多次引用同一 Chart）

  - name: redis
    version: "~17.3.0"                 # 版本范围：~17.3.0 = >=17.3.0 <17.4.0
    repository: https://charts.bitnami.com/bitnami
    condition: redis.enabled
    tags:
      - cache

  - name: nginx-ingress
    version: "^4.7.0"                  # 版本范围：^4.7.0 = >=4.7.0 <5.0.0
    repository: https://kubernetes.github.io/ingress-nginx
    condition: ingress.enabled

  - name: common                       # Library Chart（提供通用模板）
    version: 1.0.0
    repository: https://charts.example.com
    type: library                      # 声明为 library 类型

# 维护者信息
maintainers:
  - name: SRE Team
    email: sre@example.com
    url: https://example.com/sre
  - name: John Doe
    email: john@example.com

# 图标 URL
icon: https://example.com/assets/logo.png

# Kubernetes 版本约束
kubeVersion: ">=1.24.0 <1.29.0"        # 支持的 Kubernetes 版本范围

# 注解（自定义元数据）
annotations:
  category: WebApplications
  licenses: Apache-2.0
  artifacthub.io/changes: |            # Artifact Hub 变更日志
    - kind: added
      description: Added support for horizontal pod autoscaling
    - kind: fixed
      description: Fixed ingress TLS configuration
  artifacthub.io/images: |             # 镜像列表
    - name: app
      image: docker.io/myorg/app:2.5.1
    - name: nginx
      image: docker.io/nginx:1.25
  artifacthub.io/links: |              # 相关链接
    - name: Documentation
      url: https://docs.example.com
    - name: Support
      url: https://support.example.com

# 废弃标记
deprecated: false                      # 是否已废弃
```

### 2.2 values.yaml 设计模式

```yaml
# values.yaml - Helm Chart 默认配置值

# ========================================
# 全局配置（所有子 Chart 共享）
# ========================================
global:
  imageRegistry: docker.io             # 全局镜像仓库
  imagePullSecrets:                    # 全局镜像拉取凭证
    - name: regcred
  storageClass: standard               # 全局存储类
  postgresql:                          # 全局 PostgreSQL 配置
    auth:
      username: myapp
      database: myappdb

# ========================================
# 镜像配置
# ========================================
image:
  registry: docker.io                  # 镜像仓库
  repository: myorg/myapp              # 镜像仓库名
  tag: ""                              # 镜像标签（空则使用 appVersion）
  digest: ""                           # 镜像摘要（优先级高于 tag）
  pullPolicy: IfNotPresent             # 拉取策略：Always | IfNotPresent | Never
  pullSecrets:                         # 镜像拉取凭证
    - name: regcred

# ========================================
# 命名约定
# ========================================
nameOverride: ""                       # 覆盖 Chart 名称
fullnameOverride: ""                   # 覆盖完整名称

# ========================================
# 副本和更新策略
# ========================================
replicaCount: 3                        # 副本数

updateStrategy:
  type: RollingUpdate                  # 更新策略：RollingUpdate | Recreate
  rollingUpdate:
    maxSurge: 1                        # 最大额外 Pod 数
    maxUnavailable: 0                  # 最大不可用 Pod 数

# ========================================
# 服务配置
# ========================================
service:
  enabled: true                        # 是否启用 Service
  type: ClusterIP                      # Service 类型：ClusterIP | NodePort | LoadBalancer
  clusterIP: ""                        # 固定 ClusterIP（可选）
  port: 80                             # Service 端口
  targetPort: 8080                     # 目标容器端口
  nodePort: ""                         # NodePort（type=NodePort 时）
  loadBalancerIP: ""                   # LoadBalancer IP（type=LoadBalancer 时）
  loadBalancerSourceRanges: []         # 允许的源 IP 范围
  externalTrafficPolicy: Cluster       # 外部流量策略：Cluster | Local
  sessionAffinity: None                # 会话亲和性：None | ClientIP
  annotations: {}                      # Service 注解
  labels: {}                           # Service 标签

# ========================================
# Ingress 配置
# ========================================
ingress:
  enabled: false                       # 是否启用 Ingress
  className: nginx                     # Ingress 类
  annotations:
    cert-manager.io/cluster-issuer: letsencrypt-prod
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
  hosts:
    - host: myapp.example.com
      paths:
        - path: /
          pathType: Prefix
  tls:
    - secretName: myapp-tls
      hosts:
        - myapp.example.com

# ========================================
# 资源限制
# ========================================
resources:
  limits:
    cpu: 1000m
    memory: 1Gi
  requests:
    cpu: 500m
    memory: 512Mi

# ========================================
# 自动扩缩容
# ========================================
autoscaling:
  enabled: false                       # 是否启用 HPA
  minReplicas: 2
  maxReplicas: 10
  targetCPUUtilizationPercentage: 80
  targetMemoryUtilizationPercentage: 80
  behavior:                            # HPA 行为策略
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60

# ========================================
# 健康检查
# ========================================
livenessProbe:
  enabled: true
  httpGet:
    path: /healthz
    port: 8080
  initialDelaySeconds: 30
  periodSeconds: 10
  timeoutSeconds: 5
  successThreshold: 1
  failureThreshold: 3

readinessProbe:
  enabled: true
  httpGet:
    path: /ready
    port: 8080
  initialDelaySeconds: 10
  periodSeconds: 5
  timeoutSeconds: 3
  successThreshold: 1
  failureThreshold: 3

startupProbe:
  enabled: false
  httpGet:
    path: /startup
    port: 8080
  initialDelaySeconds: 0
  periodSeconds: 5
  timeoutSeconds: 3
  successThreshold: 1
  failureThreshold: 30

# ========================================
# 持久化存储
# ========================================
persistence:
  enabled: false                       # 是否启用持久化
  storageClass: ""                     # 存储类（空则使用默认）
  accessMode: ReadWriteOnce            # 访问模式
  size: 10Gi                           # 存储大小
  annotations: {}                      # PVC 注解
  existingClaim: ""                    # 使用已存在的 PVC

# ========================================
# 配置和密钥
# ========================================
configMap:
  enabled: true
  data:
    LOG_LEVEL: info
    MAX_CONNECTIONS: "100"
    FEATURE_FLAGS: '{"new_ui":true}'

secret:
  enabled: true
  type: Opaque
  data: {}                             # 由外部注入（不在 values.yaml 中硬编码）

# ========================================
# 环境变量
# ========================================
env:
  - name: ENV
    value: production
  - name: DB_HOST
    valueFrom:
      secretKeyRef:
        name: db-credentials
        key: host

envFrom:
  - configMapRef:
      name: app-config
  - secretRef:
      name: app-secrets

# ========================================
# 挂载卷
# ========================================
volumes:
  - name: config-volume
    configMap:
      name: app-config
  - name: secret-volume
    secret:
      secretName: app-secrets

volumeMounts:
  - name: config-volume
    mountPath: /etc/config
    readOnly: true
  - name: secret-volume
    mountPath: /etc/secrets
    readOnly: true

# ========================================
# 节点调度
# ========================================
nodeSelector: {}                       # 节点选择器
#  disktype: ssd

tolerations: []                        # 容忍
#  - key: "key1"
#    operator: "Equal"
#    value: "value1"
#    effect: "NoSchedule"

affinity: {}                           # 亲和性
#  nodeAffinity:
#    requiredDuringSchedulingIgnoredDuringExecution:
#      nodeSelectorTerms:
#      - matchExpressions:
#        - key: kubernetes.io/hostname
#          operator: In
#          values:
#          - node1

topologySpreadConstraints: []          # 拓扑分布约束
#  - maxSkew: 1
#    topologyKey: topology.kubernetes.io/zone
#    whenUnsatisfiable: DoNotSchedule
#    labelSelector:
#      matchLabels:
#        app: myapp

# ========================================
# 安全上下文
# ========================================
podSecurityContext:
  runAsNonRoot: true
  runAsUser: 1000
  fsGroup: 1000
  seccompProfile:
    type: RuntimeDefault

securityContext:
  allowPrivilegeEscalation: false
  capabilities:
    drop:
      - ALL
  readOnlyRootFilesystem: true

# ========================================
# ServiceAccount
# ========================================
serviceAccount:
  create: true                         # 是否创建 ServiceAccount
  automount: true                      # 是否自动挂载 ServiceAccount token
  annotations: {}                      # ServiceAccount 注解
  name: ""                             # ServiceAccount 名称（空则自动生成）

# ========================================
# RBAC
# ========================================
rbac:
  create: true                         # 是否创建 RBAC 资源
  rules:                               # ClusterRole/Role 规则
    - apiGroups: [""]
      resources: ["configmaps"]
      verbs: ["get", "list", "watch"]

# ========================================
# Pod Disruption Budget
# ========================================
podDisruptionBudget:
  enabled: false
  minAvailable: 1                      # 最小可用 Pod 数
  # maxUnavailable: 1                  # 或最大不可用 Pod 数

# ========================================
# 监控和可观测性
# ========================================
metrics:
  enabled: false                       # 是否启用 Prometheus 指标
  serviceMonitor:
    enabled: false                     # 是否创建 ServiceMonitor
    interval: 30s
    scrapeTimeout: 10s
    labels: {}
    annotations: {}

# ========================================
# 依赖配置（子 Chart）
# ========================================
postgresql:
  enabled: true                        # 是否启用 PostgreSQL 依赖
  auth:
    username: myapp
    password: changeme
    database: myappdb
  primary:
    persistence:
      enabled: true
      size: 20Gi

redis:
  enabled: true                        # 是否启用 Redis 依赖
  architecture: standalone
  auth:
    enabled: true
    password: changeme
  master:
    persistence:
      enabled: false

# ========================================
# 自定义配置（应用特定）
# ========================================
application:
  # 应用特定配置
  features:
    newUI: true
    betaFeatures: false
  
  database:
    poolSize: 20
    timeout: 30s
  
  cache:
    ttl: 3600
    maxSize: 1000
```

### 2.3 Templates 语法和最佳实践

#### templates/deployment.yaml

```yaml
{{- /*
模板注释：此模板创建 Deployment
支持的功能：
- 条件渲染
- 循环
- 命名模板引用
- 变量
- 管道
*/ -}}

apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "myapp.fullname" . }}  # 引用命名模板
  namespace: {{ .Release.Namespace }}     # Helm 内置对象
  labels:
    {{- include "myapp.labels" . | nindent 4 }}  # 引用标签模板并缩进
  {{- with .Values.annotations }}         # with 语句（如果存在则进入作用域）
  annotations:
    {{- toYaml . | nindent 4 }}           # 转换为 YAML 并缩进
  {{- end }}
spec:
  {{- if not .Values.autoscaling.enabled }}  # 条件渲染
  replicas: {{ .Values.replicaCount }}
  {{- end }}
  selector:
    matchLabels:
      {{- include "myapp.selectorLabels" . | nindent 6 }}
  {{- with .Values.updateStrategy }}      # 更新策略
  strategy:
    {{- toYaml . | nindent 4 }}
  {{- end }}
  template:
    metadata:
      annotations:
        checksum/config: {{ include (print $.Template.BasePath "/configmap.yaml") . | sha256sum }}  # 配置变更触发重启
        {{- with .Values.podAnnotations }}
        {{- toYaml . | nindent 8 }}
        {{- end }}
      labels:
        {{- include "myapp.selectorLabels" . | nindent 8 }}
    spec:
      {{- with .Values.image.pullSecrets }}  # 镜像拉取凭证
      imagePullSecrets:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      serviceAccountName: {{ include "myapp.serviceAccountName" . }}
      securityContext:
        {{- toYaml .Values.podSecurityContext | nindent 8 }}
      
      {{- if .Values.initContainers }}    # Init Containers
      initContainers:
        {{- toYaml .Values.initContainers | nindent 8 }}
      {{- end }}
      
      containers:
      - name: {{ .Chart.Name }}
        image: "{{ .Values.image.registry }}/{{ .Values.image.repository }}:{{ .Values.image.tag | default .Chart.AppVersion }}"  # 镜像拼接
        imagePullPolicy: {{ .Values.image.pullPolicy }}
        
        {{- if .Values.command }}         # 自定义命令
        command:
          {{- toYaml .Values.command | nindent 10 }}
        {{- end }}
        
        {{- if .Values.args }}            # 自定义参数
        args:
          {{- toYaml .Values.args | nindent 10 }}
        {{- end }}
        
        ports:
        - name: http
          containerPort: {{ .Values.service.targetPort }}
          protocol: TCP
        
        {{- if .Values.livenessProbe.enabled }}  # 存活探针
        livenessProbe:
          {{- omit .Values.livenessProbe "enabled" | toYaml | nindent 10 }}  # 排除 enabled 字段
        {{- end }}
        
        {{- if .Values.readinessProbe.enabled }}  # 就绪探针
        readinessProbe:
          {{- omit .Values.readinessProbe "enabled" | toYaml | nindent 10 }}
        {{- end }}
        
        {{- if .Values.startupProbe.enabled }}    # 启动探针
        startupProbe:
          {{- omit .Values.startupProbe "enabled" | toYaml | nindent 10 }}
        {{- end }}
        
        resources:
          {{- toYaml .Values.resources | nindent 10 }}
        
        {{- if or .Values.env .Values.envFrom }}  # 环境变量
        env:
          {{- range .Values.env }}        # 循环渲染 env
          - name: {{ .name }}
            {{- if .value }}
            value: {{ .value | quote }}   # 字符串引号
            {{- else if .valueFrom }}
            valueFrom:
              {{- toYaml .valueFrom | nindent 14 }}
            {{- end }}
          {{- end }}
          
          {{- if .Values.extraEnv }}      # 额外环境变量
          {{- toYaml .Values.extraEnv | nindent 10 }}
          {{- end }}
        {{- end }}
        
        {{- with .Values.envFrom }}       # envFrom
        envFrom:
          {{- toYaml . | nindent 10 }}
        {{- end }}
        
        {{- with .Values.volumeMounts }}  # 卷挂载
        volumeMounts:
          {{- toYaml . | nindent 10 }}
        {{- end }}
        
        {{- if .Values.securityContext }}
        securityContext:
          {{- toYaml .Values.securityContext | nindent 10 }}
        {{- end }}
      
      {{- if .Values.sidecars }}          # Sidecar 容器
      {{- toYaml .Values.sidecars | nindent 6 }}
      {{- end }}
      
      {{- with .Values.volumes }}         # 卷定义
      volumes:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      
      {{- with .Values.nodeSelector }}    # 节点选择器
      nodeSelector:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      
      {{- with .Values.affinity }}        # 亲和性
      affinity:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      
      {{- with .Values.tolerations }}     # 容忍
      tolerations:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      
      {{- with .Values.topologySpreadConstraints }}  # 拓扑分布约束
      topologySpreadConstraints:
        {{- toYaml . | nindent 8 }}
      {{- end }}
```

#### templates/_helpers.tpl（命名模板）

```yaml
{{/*
==================================================
Chart 名称
==================================================
*/}}
{{- define "myapp.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
==================================================
完整名称（用于资源命名）
格式：[release-name]-[chart-name]
==================================================
*/}}
{{- define "myapp.fullname" -}}
{{- if .Values.fullnameOverride }}
{{- .Values.fullnameOverride | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- $name := default .Chart.Name .Values.nameOverride }}
{{- if contains $name .Release.Name }}
{{- .Release.Name | trunc 63 | trimSuffix "-" }}
{{- else }}
{{- printf "%s-%s" .Release.Name $name | trunc 63 | trimSuffix "-" }}
{{- end }}
{{- end }}
{{- end }}

{{/*
==================================================
Chart 标签（用于标识 Chart 版本）
==================================================
*/}}
{{- define "myapp.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
==================================================
通用标签（所有资源）
==================================================
*/}}
{{- define "myapp.labels" -}}
helm.sh/chart: {{ include "myapp.chart" . }}
{{ include "myapp.selectorLabels" . }}
{{- if .Chart.AppVersion }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
{{- end }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- with .Values.commonLabels }}
{{ toYaml . }}
{{- end }}
{{- end }}

{{/*
==================================================
选择器标签（用于 Service/Deployment selector）
==================================================
*/}}
{{- define "myapp.selectorLabels" -}}
app.kubernetes.io/name: {{ include "myapp.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}

{{/*
==================================================
ServiceAccount 名称
==================================================
*/}}
{{- define "myapp.serviceAccountName" -}}
{{- if .Values.serviceAccount.create }}
{{- default (include "myapp.fullname" .) .Values.serviceAccount.name }}
{{- else }}
{{- default "default" .Values.serviceAccount.name }}
{{- end }}
{{- end }}

{{/*
==================================================
镜像完整路径（包含 registry/repository:tag）
==================================================
*/}}
{{- define "myapp.image" -}}
{{- $registry := .Values.image.registry | default .Values.global.imageRegistry }}
{{- $repository := .Values.image.repository }}
{{- $tag := .Values.image.tag | default .Chart.AppVersion }}
{{- if .Values.image.digest }}
{{- printf "%s/%s@%s" $registry $repository .Values.image.digest }}
{{- else }}
{{- printf "%s/%s:%s" $registry $repository $tag }}
{{- end }}
{{- end }}

{{/*
==================================================
镜像拉取策略
==================================================
*/}}
{{- define "myapp.imagePullPolicy" -}}
{{- if .Values.image.digest }}
IfNotPresent
{{- else }}
{{- .Values.image.pullPolicy }}
{{- end }}
{{- end }}

{{/*
==================================================
数据库主机（支持外部/内部数据库）
==================================================
*/}}
{{- define "myapp.databaseHost" -}}
{{- if .Values.postgresql.enabled }}
{{- printf "%s-postgresql" (include "myapp.fullname" .) }}
{{- else }}
{{- .Values.externalDatabase.host }}
{{- end }}
{{- end }}

{{/*
==================================================
合并标签（基础标签 + 自定义标签）
==================================================
*/}}
{{- define "myapp.mergeLabels" -}}
{{- $base := include "myapp.labels" . | fromYaml }}
{{- $custom := .custom | default dict }}
{{- toYaml (merge $custom $base) }}
{{- end }}

{{/*
==================================================
条件资源（根据条件返回资源配置）
==================================================
*/}}
{{- define "myapp.resources" -}}
{{- if .Values.resources }}
{{- toYaml .Values.resources }}
{{- else }}
limits:
  cpu: 100m
  memory: 128Mi
requests:
  cpu: 50m
  memory: 64Mi
{{- end }}
{{- end }}

{{/*
==================================================
验证必需值（如果缺失则失败）
==================================================
*/}}
{{- define "myapp.validateValues" -}}
{{- if not .Values.postgresql.enabled }}
{{- if not .Values.externalDatabase.host }}
{{- fail "必须启用 postgresql 或提供 externalDatabase.host" }}
{{- end }}
{{- end }}
{{- end }}

{{/*
==================================================
渲染环境变量（支持字符串/对象）
==================================================
*/}}
{{- define "myapp.renderEnv" -}}
{{- range $key, $value := . }}
{{- if kindIs "string" $value }}
- name: {{ $key }}
  value: {{ $value | quote }}
{{- else }}
- name: {{ $key }}
  {{- toYaml $value | nindent 2 }}
{{- end }}
{{- end }}
{{- end }}

{{/*
==================================================
生成随机密码（用于 Secret）
==================================================
*/}}
{{- define "myapp.randomPassword" -}}
{{- randAlphaNum 16 }}
{{- end }}

{{/*
==================================================
TPL 函数（动态渲染模板字符串）
使用方法：{{ include "myapp.tplValue" (dict "value" .Values.someTemplate "context" $) }}
==================================================
*/}}
{{- define "myapp.tplValue" -}}
{{- if typeIs "string" .value }}
{{- tpl .value .context }}
{{- else }}
{{- tpl (.value | toYaml) .context }}
{{- end }}
{{- end }}
```

#### templates/NOTES.txt（安装后提示）

```
================================================================
✅ {{ .Chart.Name }} 已成功部署！
================================================================

Release 名称: {{ .Release.Name }}
Namespace: {{ .Release.Namespace }}
Chart 版本: {{ .Chart.Version }}
应用版本: {{ .Chart.AppVersion }}

----------------------------------------------------------------
📦 部署的资源
----------------------------------------------------------------

{{- if .Values.serviceAccount.create }}
✓ ServiceAccount: {{ include "myapp.serviceAccountName" . }}
{{- end }}

{{- if not .Values.autoscaling.enabled }}
✓ Deployment: {{ include "myapp.fullname" . }} ({{ .Values.replicaCount }} 副本)
{{- else }}
✓ Deployment: {{ include "myapp.fullname" . }} (由 HPA 管理)
✓ HorizontalPodAutoscaler: {{ include "myapp.fullname" . }}
{{- end }}

{{- if .Values.service.enabled }}
✓ Service: {{ include "myapp.fullname" . }}
  类型: {{ .Values.service.type }}
  端口: {{ .Values.service.port }}
{{- end }}

{{- if .Values.ingress.enabled }}
✓ Ingress: {{ include "myapp.fullname" . }}
{{- end }}

{{- if .Values.postgresql.enabled }}
✓ PostgreSQL: {{ include "myapp.fullname" . }}-postgresql
{{- end }}

{{- if .Values.redis.enabled }}
✓ Redis: {{ include "myapp.fullname" . }}-redis
{{- end }}

----------------------------------------------------------------
🌐 访问应用
----------------------------------------------------------------

{{- if .Values.ingress.enabled }}
应用 URL:
{{- range .Values.ingress.hosts }}
  https://{{ .host }}{{ (index .paths 0).path }}
{{- end }}

{{- else if eq .Values.service.type "LoadBalancer" }}

⏳ 等待 LoadBalancer IP 分配...
运行以下命令获取 IP：

  export SERVICE_IP=$(kubectl get svc --namespace {{ .Release.Namespace }} {{ include "myapp.fullname" . }} -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
  echo "访问地址: http://$SERVICE_IP:{{ .Values.service.port }}"

{{- else if eq .Values.service.type "NodePort" }}

访问 NodePort 服务：

  export NODE_PORT=$(kubectl get --namespace {{ .Release.Namespace }} -o jsonpath="{.spec.ports[0].nodePort}" services {{ include "myapp.fullname" . }})
  export NODE_IP=$(kubectl get nodes --namespace {{ .Release.Namespace }} -o jsonpath="{.items[0].status.addresses[0].address}")
  echo "访问地址: http://$NODE_IP:$NODE_PORT"

{{- else }}

本地端口转发：

  export POD_NAME=$(kubectl get pods --namespace {{ .Release.Namespace }} -l "app.kubernetes.io/name={{ include "myapp.name" . }},app.kubernetes.io/instance={{ .Release.Name }}" -o jsonpath="{.items[0].metadata.name}")
  kubectl --namespace {{ .Release.Namespace }} port-forward $POD_NAME 8080:{{ .Values.service.targetPort }}
  echo "访问地址: http://127.0.0.1:8080"

{{- end }}

----------------------------------------------------------------
🔍 监控和日志
----------------------------------------------------------------

查看 Pod 状态：
  kubectl get pods --namespace {{ .Release.Namespace }} -l "app.kubernetes.io/name={{ include "myapp.name" . }},app.kubernetes.io/instance={{ .Release.Name }}"

查看 Pod 日志：
  kubectl logs --namespace {{ .Release.Namespace }} -l "app.kubernetes.io/name={{ include "myapp.name" . }}" --tail=100 -f

查看 Pod 事件：
  kubectl describe pods --namespace {{ .Release.Namespace }} -l "app.kubernetes.io/name={{ include "myapp.name" . }}"

{{- if .Values.metrics.enabled }}

Prometheus 指标：
  Metrics 端点: http://{{ include "myapp.fullname" . }}:{{ .Values.metrics.port }}/metrics

{{- end }}

----------------------------------------------------------------
⚙️  配置管理
----------------------------------------------------------------

查看当前配置：
  helm get values {{ .Release.Name }} --namespace {{ .Release.Namespace }}

升级 Release：
  helm upgrade {{ .Release.Name }} {{ .Chart.Name }} --namespace {{ .Release.Namespace }} -f custom-values.yaml

回滚到上一版本：
  helm rollback {{ .Release.Name }} --namespace {{ .Release.Namespace }}

----------------------------------------------------------------
📚 更多信息
----------------------------------------------------------------

Chart 文档: {{ .Chart.Home }}
应用文档: https://docs.example.com
技术支持: {{ (index .Chart.Maintainers 0).email }}

{{- if .Values.postgresql.enabled }}

⚠️  数据库凭证
----------------------------------------------------------------

PostgreSQL 用户名: {{ .Values.postgresql.auth.username }}
获取密码：
  kubectl get secret --namespace {{ .Release.Namespace }} {{ include "myapp.fullname" . }}-postgresql -o jsonpath="{.data.password}" | base64 -d

{{- end }}

================================================================
🎉 祝您使用愉快！
================================================================
```

### 2.4 生产案例：Helm 生产级 Values

#### values-production.yaml（生产环境配置）

```yaml
# ========================================
# 生产环境 Helm Values
# ========================================

# 全局配置
global:
  imageRegistry: gcr.io
  imagePullSecrets:
    - name: gcr-credentials
  storageClass: ssd-retain

# 镜像配置（生产环境使用摘要）
image:
  registry: gcr.io
  repository: my-project/my-application
  tag: "v2.5.1"
  digest: sha256:abc123def456789...
  pullPolicy: IfNotPresent

# 生产环境副本数
replicaCount: 10

# 滚动更新策略（保守）
updateStrategy:
  type: RollingUpdate
  rollingUpdate:
    maxSurge: 1                        # 一次只增加 1 个 Pod
    maxUnavailable: 0                  # 确保零停机

# Service 配置
service:
  enabled: true
  type: LoadBalancer
  port: 443
  targetPort: 8080
  annotations:
    cloud.google.com/load-balancer-type: "Internal"
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"

# Ingress 配置（生产域名）
ingress:
  enabled: true
  className: nginx
  annotations:
    cert-manager.io/cluster-issuer: letsencrypt-prod
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/rate-limit: "100"
    nginx.ingress.kubernetes.io/proxy-body-size: "50m"
  hosts:
    - host: api.example.com
      paths:
        - path: /
          pathType: Prefix
  tls:
    - secretName: api-example-com-tls
      hosts:
        - api.example.com

# 生产环境资源配置
resources:
  limits:
    cpu: 2000m
    memory: 4Gi
  requests:
    cpu: 1000m
    memory: 2Gi

# 启用自动扩缩容
autoscaling:
  enabled: true
  minReplicas: 10
  maxReplicas: 50
  targetCPUUtilizationPercentage: 70
  targetMemoryUtilizationPercentage: 80
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 600  # 10 分钟稳定期
      policies:
      - type: Percent
        value: 10                      # 每次缩容 10%
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 50                      # 快速扩容
        periodSeconds: 60

# 健康检查（生产环境调优）
livenessProbe:
  enabled: true
  httpGet:
    path: /healthz
    port: 8080
    httpHeaders:
      - name: X-Health-Check
        value: liveness
  initialDelaySeconds: 60
  periodSeconds: 10
  timeoutSeconds: 5
  successThreshold: 1
  failureThreshold: 3

readinessProbe:
  enabled: true
  httpGet:
    path: /ready
    port: 8080
  initialDelaySeconds: 30
  periodSeconds: 5
  timeoutSeconds: 3
  successThreshold: 1
  failureThreshold: 3

startupProbe:
  enabled: true
  httpGet:
    path: /startup
    port: 8080
  initialDelaySeconds: 0
  periodSeconds: 10
  timeoutSeconds: 5
  failureThreshold: 30               # 最多等待 5 分钟启动

# 持久化存储（生产环境）
persistence:
  enabled: true
  storageClass: ssd-retain           # 使用 SSD 且保留策略
  accessMode: ReadWriteOnce
  size: 100Gi
  annotations:
    volume.beta.kubernetes.io/storage-provisioner: pd.csi.storage.gke.io

# 配置（生产环境）
configMap:
  enabled: true
  data:
    ENV: production
    LOG_LEVEL: info
    LOG_FORMAT: json
    MAX_CONNECTIONS: "1000"
    TIMEOUT: "30s"
    CACHE_TTL: "3600"
    FEATURE_FLAGS: |
      {
        "new_ui": true,
        "beta_features": false,
        "experimental": false
      }

# 密钥（生产环境 - 实际密钥通过外部系统注入）
secret:
  enabled: true
  type: Opaque
  data: {}                           # 密钥由 CI/CD 或 Vault 注入

# 环境变量（生产配置）
env:
  - name: ENV
    value: production
  - name: PORT
    value: "8080"
  - name: DB_HOST
    valueFrom:
      secretKeyRef:
        name: db-credentials
        key: host
  - name: DB_PASSWORD
    valueFrom:
      secretKeyRef:
        name: db-credentials
        key: password
  - name: REDIS_URL
    value: redis://prod-my-application-redis-master:6379

# 节点选择器（生产节点池）
nodeSelector:
  node-type: production
  workload: api

# 容忍（允许调度到专用节点）
tolerations:
  - key: "workload"
    operator: "Equal"
    value: "production"
    effect: "NoSchedule"

# 亲和性（跨可用区分布）
affinity:
  podAntiAffinity:
    requiredDuringSchedulingIgnoredDuringExecution:
    - labelSelector:
        matchLabels:
          app.kubernetes.io/name: my-application
      topologyKey: kubernetes.io/hostname
  nodeAffinity:
    preferredDuringSchedulingIgnoredDuringExecution:
    - weight: 100
      preference:
        matchExpressions:
        - key: topology.kubernetes.io/zone
          operator: In
          values:
          - us-central1-a
          - us-central1-b
          - us-central1-c

# 拓扑分布约束（确保跨可用区均匀分布）
topologySpreadConstraints:
  - maxSkew: 1
    topologyKey: topology.kubernetes.io/zone
    whenUnsatisfiable: DoNotSchedule
    labelSelector:
      matchLabels:
        app.kubernetes.io/name: my-application
  - maxSkew: 1
    topologyKey: kubernetes.io/hostname
    whenUnsatisfiable: ScheduleAnyway
    labelSelector:
      matchLabels:
        app.kubernetes.io/name: my-application

# 安全上下文（生产环境强化）
podSecurityContext:
  runAsNonRoot: true
  runAsUser: 1000
  fsGroup: 1000
  seccompProfile:
    type: RuntimeDefault

securityContext:
  allowPrivilegeEscalation: false
  capabilities:
    drop:
      - ALL
  readOnlyRootFilesystem: true

# ServiceAccount（启用 Workload Identity）
serviceAccount:
  create: true
  automount: true
  annotations:
    iam.gke.io/gcp-service-account: my-app@my-project.iam.gserviceaccount.com

# RBAC
rbac:
  create: true
  rules:
    - apiGroups: [""]
      resources: ["configmaps", "secrets"]
      verbs: ["get", "list", "watch"]

# Pod Disruption Budget（确保高可用）
podDisruptionBudget:
  enabled: true
  minAvailable: 7                    # 至少保持 7 个 Pod 可用

# 监控（启用 Prometheus）
metrics:
  enabled: true
  serviceMonitor:
    enabled: true
    interval: 30s
    scrapeTimeout: 10s
    labels:
      prometheus: kube-prometheus
    annotations:
      monitoring: "true"

# PostgreSQL 依赖（生产配置）
postgresql:
  enabled: false                     # 生产环境使用外部 CloudSQL
  # 如果启用内部 PostgreSQL：
  # enabled: true
  # auth:
  #   username: myapp
  #   existingSecret: postgres-credentials
  # primary:
  #   persistence:
  #     enabled: true
  #     storageClass: ssd-retain
  #     size: 100Gi
  #   resources:
  #     limits:
  #       cpu: 2000m
  #       memory: 4Gi
  #     requests:
  #       cpu: 1000m
  #       memory: 2Gi

# 外部数据库配置
externalDatabase:
  host: 10.20.30.40
  port: 5432
  database: myappdb
  username: myapp
  existingSecret: external-db-credentials  # 密钥由外部系统管理

# Redis 依赖（生产配置）
redis:
  enabled: true
  architecture: replication          # 主从复制
  auth:
    enabled: true
    existingSecret: redis-credentials
  master:
    persistence:
      enabled: true
      storageClass: ssd-retain
      size: 20Gi
    resources:
      limits:
        cpu: 1000m
        memory: 2Gi
      requests:
        cpu: 500m
        memory: 1Gi
  replica:
    replicaCount: 2
    persistence:
      enabled: true
      size: 20Gi
    resources:
      limits:
        cpu: 1000m
        memory: 2Gi
      requests:
        cpu: 500m
        memory: 1Gi

# 应用特定配置
application:
  features:
    newUI: true
    betaFeatures: false
    experimental: false
  
  database:
    poolSize: 50
    maxConnections: 100
    timeout: 30s
    sslMode: require
  
  cache:
    ttl: 3600
    maxSize: 10000
    evictionPolicy: lru
  
  api:
    rateLimit: 1000
    timeout: 30s
    maxBodySize: 50m
  
  logging:
    level: info
    format: json
    outputs:
      - stdout
      - file
  
  monitoring:
    tracingEnabled: true
    tracingSampleRate: 0.1           # 10% 采样率
```

---

## 3️⃣ ArgoCD 配置参考

### 3.1 Application CRD 完整字段规范

```yaml
# ArgoCD Application - GitOps 应用定义
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: my-application-prod           # Application 名称
  namespace: argocd                   # ArgoCD 命名空间（通常是 argocd）
  
  # Finalizers（确保级联删除）
  finalizers:
    - resources-finalizer.argocd.argoproj.io  # 删除 App 时删除其创建的资源
  
  # 标签
  labels:
    environment: production
    team: backend
    app-type: microservice
  
  # 注解
  annotations:
    argocd.argoproj.io/sync-wave: "2"          # 同步波次（控制同步顺序）
    argocd.argoproj.io/sync-options: "Prune=true"  # 同步选项
    notifications.argoproj.io/subscribe.on-deployed.slack: "production-deployments"  # 通知订阅

spec:
  # ========================================
  # 源配置（Git/Helm/OCI）
  # ========================================
  source:
    # Git 仓库（方式 1：原始 YAML）
    repoURL: https://github.com/myorg/k8s-manifests.git
    targetRevision: main               # 分支/标签/提交哈希
    path: apps/my-app/production       # 仓库路径
    
    # Kustomize 配置
    kustomize:
      version: v5.0.0                  # Kustomize 版本
      namePrefix: prod-                # 名称前缀
      nameSuffix: -v2                  # 名称后缀
      images:                          # 镜像覆盖
        - name: my-app
          newName: gcr.io/my-project/my-app
          newTag: v2.5.1
      replicas:                        # 副本数覆盖
        - name: my-app
          count: 10
      commonLabels:                    # 通用标签
        environment: production
      commonAnnotations:               # 通用注解
        managed-by: argocd
      patches:                         # 内联补丁
        - target:
            kind: Deployment
            name: my-app
          patch: |-
            - op: replace
              path: /spec/replicas
              value: 10
      components:                      # Kustomize 组件
        - components/monitoring
      forceCommonLabels: false         # 是否强制覆盖标签
      forceCommonAnnotations: false
    
    # # Helm 配置（方式 2：Helm Chart）
    # repoURL: https://charts.example.com
    # chart: my-application            # Chart 名称
    # targetRevision: 1.2.3            # Chart 版本
    # helm:
    #   releaseName: my-app-prod       # Release 名称（默认为 App 名称）
    #   values: |                      # 内联 values
    #     replicaCount: 10
    #     image:
    #       tag: v2.5.1
    #     ingress:
    #       enabled: true
    #       hosts:
    #         - api.example.com
    #   valueFiles:                    # 外部 values 文件
    #     - values-production.yaml
    #   parameters:                    # 参数覆盖
    #     - name: image.tag
    #       value: v2.5.1
    #     - name: replicaCount
    #       value: "10"
    #       forceString: true          # 强制字符串类型
    #   fileParameters:                # 文件参数
    #     - name: config
    #       path: files/config.yaml
    #   version: v3                    # Helm 版本
    #   passCredentials: false         # 是否传递凭证
    #   skipCrds: false                # 是否跳过 CRDs
    #   valuesObject:                  # values 对象（替代 values 字符串）
    #     replicaCount: 10
    #     image:
    #       tag: v2.5.1
    
    # # Plugin 配置（方式 3：Config Management Plugin）
    # plugin:
    #   name: my-custom-plugin
    #   env:
    #     - name: ENV
    #       value: production
    
    # # Directory 配置（原始 YAML 目录）
    # directory:
    #   recurse: true                  # 递归子目录
    #   jsonnet:                       # Jsonnet 配置
    #     extVars:
    #       - name: environment
    #         value: production
    #     tlas:
    #       - name: namespace
    #         value: production
    #   exclude: "test/*"              # 排除模式
    #   include: "*.yaml"              # 包含模式
    
    # Ref（Git 引用，已废弃，使用 targetRevision）
    # ref: main
  
  # ========================================
  # 多源配置（ArgoCD 2.6+）
  # ========================================
  # sources:                           # 多源支持（替代单个 source）
  #   - repoURL: https://github.com/myorg/k8s-manifests.git
  #     targetRevision: main
  #     path: apps/my-app/base
  #   - repoURL: https://charts.example.com
  #     chart: my-dependency
  #     targetRevision: 1.0.0
  #     helm:
  #       valueFiles:
  #         - $values/apps/my-app/values.yaml  # 引用其他源的文件
  #   - repoURL: https://github.com/myorg/values.git
  #     targetRevision: main
  #     ref: values                    # 定义引用名
  
  # ========================================
  # 目标配置（部署到的集群和命名空间）
  # ========================================
  destination:
    server: https://kubernetes.default.svc  # 集群 API Server（in-cluster）
    # server: https://prod-cluster.example.com  # 或远程集群
    # name: prod-cluster               # 或集群名称（在 ArgoCD 中注册）
    namespace: production              # 目标命名空间
  
  # ========================================
  # 同步策略
  # ========================================
  syncPolicy:
    # 自动同步（GitOps 核心）
    automated:
      prune: true                      # 自动删除不在 Git 中的资源
      selfHeal: true                   # 自动修复漂移（恢复被手动修改的资源）
      allowEmpty: false                # 是否允许空应用
    
    # 同步选项
    syncOptions:
      - CreateNamespace=true           # 自动创建命名空间
      - PrunePropagationPolicy=foreground  # 删除传播策略：foreground | background | orphan
      - PruneLast=true                 # 最后执行删除操作
      - ApplyOutOfSyncOnly=true        # 仅应用不同步的资源
      - Validate=true                  # 验证资源
      - RespectIgnoreDifferences=true  # 遵守 ignoreDifferences 配置
      - ServerSideApply=true           # 使用服务端应用（K8s 1.22+）
      - FailOnSharedResource=false     # 共享资源冲突时是否失败
    
    # 管理策略
    managedNamespaceMetadata:          # 管理命名空间元数据
      labels:
        environment: production
        managed-by: argocd
      annotations:
        contact: sre@example.com
    
    # 重试策略
    retry:
      limit: 5                         # 最大重试次数
      backoff:
        duration: 5s                   # 初始重试间隔
        factor: 2                      # 退避因子
        maxDuration: 3m                # 最大重试间隔
  
  # ========================================
  # 项目（RBAC 和策略）
  # ========================================
  project: production                  # ArgoCD 项目名称（default 或自定义）
  
  # ========================================
  # 忽略差异配置
  # ========================================
  ignoreDifferences:
    # 忽略特定字段的差异（避免误报不同步）
    - group: apps
      kind: Deployment
      jsonPointers:
        - /spec/replicas               # 忽略 HPA 管理的 replicas
    
    - group: ""
      kind: Secret
      name: my-secret
      namespace: production
      jsonPointers:
        - /data                        # 忽略 Secret 数据（由外部系统管理）
    
    - group: "*"
      kind: "*"
      managedFieldsManagers:           # 忽略特定字段管理器的变更
        - kube-controller-manager
    
    - group: apps
      kind: StatefulSet
      jqPathExpressions:               # 使用 JQ 表达式
        - .spec.volumeClaimTemplates[]?.metadata.labels

  # ========================================
  # 资源信息
  # ========================================
  info:
    - name: "URL"
      value: "https://api.example.com"
    - name: "Owner"
      value: "Backend Team"
    - name: "Slack"
      value: "#backend-prod"
  
  # ========================================
  # 修订历史限制
  # ========================================
  revisionHistoryLimit: 10             # 保留的修订历史数量
```

### 3.2 ApplicationSet CRD 完整字段规范

```yaml
# ArgoCD ApplicationSet - 多应用生成器
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: multi-env-apps                 # ApplicationSet 名称
  namespace: argocd
  
  labels:
    team: platform
  
  annotations:
    argocd.argoproj.io/manifest-generate-paths: "."

spec:
  # ========================================
  # 生成器（Generators）- 定义如何生成 Application
  # ========================================
  generators:
    # ----------------------------------------
    # 1. List Generator（列表生成器）
    # ----------------------------------------
    - list:
        elements:
          - cluster: prod-us-west
            url: https://prod-us-west.example.com
            environment: production
            replicas: "10"
          
          - cluster: prod-us-east
            url: https://prod-us-east.example.com
            environment: production
            replicas: "10"
          
          - cluster: staging
            url: https://staging.example.com
            environment: staging
            replicas: "3"
          
          - cluster: dev
            url: https://dev.example.com
            environment: dev
            replicas: "1"
    
    # # ----------------------------------------
    # # 2. Cluster Generator（集群生成器）
    # # ----------------------------------------
    # - cluster:
    #     selector:                      # 集群标签选择器
    #       matchLabels:
    #         environment: production
    #       matchExpressions:
    #         - key: region
    #           operator: In
    #           values:
    #             - us-west
    #             - us-east
    #     values:                        # 附加值
    #       revision: main
    #       project: production
    
    # # ----------------------------------------
    # # 3. Git Generator（Git 目录生成器）
    # # ----------------------------------------
    # - git:
    #     repoURL: https://github.com/myorg/k8s-manifests.git
    #     revision: main
    #     directories:                   # 目录匹配
    #       - path: apps/*               # 匹配 apps 下的所有子目录
    #       - path: environments/*
    #         exclude: true              # 排除
    #     files:                         # 文件匹配（读取文件内容作为参数）
    #       - path: apps/*/config.json
    
    # # Git Files Generator（读取文件生成）
    # - git:
    #     repoURL: https://github.com/myorg/app-configs.git
    #     revision: main
    #     files:
    #       - path: "configs/*.json"     # 匹配多个 JSON 文件
    
    # # ----------------------------------------
    # # 4. Matrix Generator（矩阵生成器 - 组合）
    # # ----------------------------------------
    # - matrix:
    #     generators:
    #       - list:                      # 第一维：环境
    #           elements:
    #             - environment: dev
    #               replicas: "1"
    #             - environment: staging
    #               replicas: "3"
    #             - environment: production
    #               replicas: "10"
    #       
    #       - list:                      # 第二维：地区
    #           elements:
    #             - region: us-west
    #               cluster: prod-us-west
    #             - region: us-east
    #               cluster: prod-us-east
    #       
    #       # 结果：生成 3×2 = 6 个 Application
    #       # dev-us-west, dev-us-east, staging-us-west, staging-us-east, production-us-west, production-us-east
    
    # # ----------------------------------------
    # # 5. Merge Generator（合并生成器）
    # # ----------------------------------------
    # - merge:
    #     mergeKeys:                     # 合并键
    #       - cluster
    #     generators:
    #       - clusters:                  # 基础：集群列表
    #           selector:
    #             matchLabels:
    #               environment: production
    #       
    #       - list:                      # 覆盖：特定配置
    #           elements:
    #             - cluster: prod-us-west
    #               replicas: "15"       # 覆盖默认值
    
    # # ----------------------------------------
    # # 6. SCM Provider Generator（SCM 提供商生成器）
    # # ----------------------------------------
    # - scmProvider:
    #     github:                        # GitHub 组织
    #       organization: myorg
    #       allBranches: false           # 仅主分支
    #       tokenRef:
    #         secretName: github-token
    #         key: token
    #     filters:                       # 仓库过滤
    #       - repositoryMatch: "^app-.*"
    
    # # ----------------------------------------
    # # 7. Pull Request Generator（PR 生成器）
    # # ----------------------------------------
    # - pullRequest:
    #     github:
    #       owner: myorg
    #       repo: my-app
    #       labels:
    #         - preview                  # 仅匹配带 preview 标签的 PR
    #       tokenRef:
    #         secretName: github-token
    #         key: token
    #     requeueAfterSeconds: 60        # 重新检查间隔
    
    # # ----------------------------------------
    # # 8. Cluster Decision Resource Generator（CDR 生成器）
    # # ----------------------------------------
    # - clusterDecisionResource:
    #     configMapRef: my-clusters      # 引用 ConfigMap
    #     labelSelector:
    #       matchLabels:
    #         environment: production
  
  # ========================================
  # 模板（Template）- 定义生成的 Application 结构
  # ========================================
  template:
    metadata:
      name: "{{cluster}}-my-app"       # 使用生成器变量
      namespace: argocd
      
      labels:
        environment: "{{environment}}"
        cluster: "{{cluster}}"
        managed-by: applicationset
      
      annotations:
        argocd.argoproj.io/sync-wave: "0"
      
      # Finalizers
      finalizers:
        - resources-finalizer.argocd.argoproj.io
    
    spec:
      project: production
      
      source:
        repoURL: https://github.com/myorg/k8s-manifests.git
        targetRevision: main
        path: "apps/my-app"            # 固定路径
        
        kustomize:
          namePrefix: "{{cluster}}-"
          images:
            - name: my-app
              newTag: "v2.5.1"
          replicas:
            - name: my-app
              count: "{{replicas}}"    # 动态副本数
          commonLabels:
            environment: "{{environment}}"
            cluster: "{{cluster}}"
      
      destination:
        server: "{{url}}"              # 动态集群 URL
        namespace: my-app
      
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
          - CreateNamespace=true
        retry:
          limit: 3
          backoff:
            duration: 5s
            factor: 2
            maxDuration: 1m
      
      ignoreDifferences:
        - group: apps
          kind: Deployment
          jsonPointers:
            - /spec/replicas
  
  # ========================================
  # ApplicationSet 同步策略
  # ========================================
  syncPolicy:
    # 保留策略（Application 删除行为）
    preserveResourcesOnDeletion: false # false=删除 App 时删除资源，true=保留
    
    # 应用同步策略
    applicationsSync: sync             # sync | create-only | create-update
  
  # ========================================
  # 模板补丁（根据生成器参数动态修改）
  # ========================================
  # templatePatch: |
  #   spec:
  #     {{- if eq .environment "production" }}
  #     source:
  #       kustomize:
  #         images:
  #           - name: my-app
  #             digest: sha256:abc123...  # 生产环境使用摘要
  #     {{- end }}
```

#### 生成器示例：Matrix Generator（环境 × 地区）

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: multi-env-multi-region
  namespace: argocd
spec:
  generators:
    - matrix:
        generators:
          # 第一维：环境
          - list:
              elements:
                - env: dev
                  replicas: "1"
                  syncWave: "0"
                - env: staging
                  replicas: "3"
                  syncWave: "1"
                - env: production
                  replicas: "10"
                  syncWave: "2"
          
          # 第二维：地区
          - list:
              elements:
                - region: us-west
                  cluster: https://us-west.k8s.example.com
                  zone: us-west-1
                - region: us-east
                  cluster: https://us-east.k8s.example.com
                  zone: us-east-1
                - region: eu-central
                  cluster: https://eu-central.k8s.example.com
                  zone: eu-central-1
  
  template:
    metadata:
      name: "myapp-{{env}}-{{region}}"
      annotations:
        argocd.argoproj.io/sync-wave: "{{syncWave}}"
    spec:
      project: default
      source:
        repoURL: https://github.com/myorg/manifests.git
        targetRevision: main
        path: apps/myapp
        kustomize:
          namePrefix: "{{env}}-{{region}}-"
          replicas:
            - name: myapp
              count: "{{replicas}}"
          commonLabels:
            environment: "{{env}}"
            region: "{{region}}"
      destination:
        server: "{{cluster}}"
        namespace: "myapp-{{env}}"
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
          - CreateNamespace=true
```

### 3.3 同步策略和健康检查

#### Sync Waves（同步波次）

```yaml
# 使用注解控制同步顺序（数字越小越先同步）

# Wave -5: 命名空间
apiVersion: v1
kind: Namespace
metadata:
  name: my-app
  annotations:
    argocd.argoproj.io/sync-wave: "-5"

---
# Wave -3: 密钥
apiVersion: v1
kind: Secret
metadata:
  name: db-credentials
  namespace: my-app
  annotations:
    argocd.argoproj.io/sync-wave: "-3"
type: Opaque
data:
  password: Y2hhbmdlbWU=

---
# Wave -1: ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
  namespace: my-app
  annotations:
    argocd.argoproj.io/sync-wave: "-1"
data:
  LOG_LEVEL: info

---
# Wave 0: Deployment（默认）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  namespace: my-app
  annotations:
    argocd.argoproj.io/sync-wave: "0"  # 默认值
spec:
  replicas: 3
  selector:
    matchLabels:
      app: my-app
  template:
    metadata:
      labels:
        app: my-app
    spec:
      containers:
      - name: app
        image: my-app:v1.0.0

---
# Wave 1: Service
apiVersion: v1
kind: Service
metadata:
  name: my-app
  namespace: my-app
  annotations:
    argocd.argoproj.io/sync-wave: "1"
spec:
  selector:
    app: my-app
  ports:
  - port: 80
    targetPort: 8080

---
# Wave 2: Ingress
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: my-app
  namespace: my-app
  annotations:
    argocd.argoproj.io/sync-wave: "2"
spec:
  rules:
  - host: myapp.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: my-app
            port:
              number: 80

---
# Wave 10: 数据库迁移 Job（最后执行）
apiVersion: batch/v1
kind: Job
metadata:
  name: db-migration
  namespace: my-app
  annotations:
    argocd.argoproj.io/sync-wave: "10"
    argocd.argoproj.io/hook: Sync       # Hook：在同步时执行
    argocd.argoproj.io/hook-delete-policy: BeforeHookCreation  # 删除策略
spec:
  template:
    spec:
      containers:
      - name: migrate
        image: migrate-tool:latest
        command: ["migrate", "up"]
      restartPolicy: Never
```

#### Resource Hooks（资源钩子）

```yaml
# PreSync Hook（同步前执行）
apiVersion: batch/v1
kind: Job
metadata:
  name: pre-sync-backup
  annotations:
    argocd.argoproj.io/hook: PreSync              # 钩子类型
    argocd.argoproj.io/hook-delete-policy: HookSucceeded  # 删除策略
    argocd.argoproj.io/sync-wave: "-10"
spec:
  template:
    spec:
      containers:
      - name: backup
        image: backup-tool:latest
        command: ["backup.sh"]
      restartPolicy: Never

---
# Sync Hook（同步时执行）
apiVersion: batch/v1
kind: Job
metadata:
  name: db-migration
  annotations:
    argocd.argoproj.io/hook: Sync
    argocd.argoproj.io/hook-delete-policy: BeforeHookCreation
spec:
  template:
    spec:
      containers:
      - name: migrate
        image: migrate:latest
      restartPolicy: Never

---
# PostSync Hook（同步后执行）
apiVersion: batch/v1
kind: Job
metadata:
  name: smoke-test
  annotations:
    argocd.argoproj.io/hook: PostSync
    argocd.argoproj.io/hook-delete-policy: HookSucceeded
spec:
  template:
    spec:
      containers:
      - name: test
        image: curl:latest
        command: ["curl", "http://my-app/health"]
      restartPolicy: Never

---
# SyncFail Hook（同步失败时执行）
apiVersion: batch/v1
kind: Job
metadata:
  name: rollback-notification
  annotations:
    argocd.argoproj.io/hook: SyncFail
    argocd.argoproj.io/hook-delete-policy: HookSucceeded
spec:
  template:
    spec:
      containers:
      - name: notify
        image: slack-notifier:latest
        env:
          - name: MESSAGE
            value: "部署失败，请检查！"
      restartPolicy: Never
```

#### 自定义健康检查

```yaml
# ArgoCD ConfigMap：自定义资源健康检查
apiVersion: v1
kind: ConfigMap
metadata:
  name: argocd-cm
  namespace: argocd
data:
  # 自定义 CRD 健康检查
  resource.customizations.health.myapp.example.com_MyCustomResource: |
    hs = {}
    if obj.status ~= nil then
      if obj.status.phase == "Running" then
        hs.status = "Healthy"
        hs.message = "Resource is running"
      elseif obj.status.phase == "Failed" then
        hs.status = "Degraded"
        hs.message = "Resource failed: " .. obj.status.reason
      else
        hs.status = "Progressing"
        hs.message = "Resource is " .. obj.status.phase
      end
    else
      hs.status = "Progressing"
      hs.message = "Waiting for status"
    end
    return hs
  
  # Deployment 自定义健康检查（覆盖默认）
  resource.customizations.health.apps_Deployment: |
    hs = {}
    if obj.status ~= nil then
      if obj.status.updatedReplicas == obj.spec.replicas and
         obj.status.replicas == obj.spec.replicas and
         obj.status.availableReplicas == obj.spec.replicas and
         obj.status.observedGeneration >= obj.metadata.generation then
        hs.status = "Healthy"
        hs.message = "All replicas are ready"
      else
        hs.status = "Progressing"
        hs.message = "Waiting for rollout to finish: " .. 
                     obj.status.updatedReplicas .. "/" .. obj.spec.replicas .. " updated"
      end
    else
      hs.status = "Progressing"
      hs.message = "Waiting for status"
    end
    return hs
```

### 3.4 生产案例：ArgoCD 多集群 GitOps

#### 目录结构

```bash
k8s-manifests/
├── argocd/                            # ArgoCD 配置
│   ├── applicationsets/
│   │   ├── apps.yaml                  # 应用 ApplicationSet
│   │   └── infrastructure.yaml        # 基础设施 ApplicationSet
│   └── projects/
│       ├── production.yaml
│       └── development.yaml
│
├── apps/                              # 应用清单
│   ├── frontend/
│   │   ├── base/
│   │   │   ├── kustomization.yaml
│   │   │   ├── deployment.yaml
│   │   │   └── service.yaml
│   │   └── overlays/
│   │       ├── dev/
│   │       ├── staging/
│   │       └── production/
│   │
│   ├── backend/
│   │   └── ...
│   │
│   └── api-gateway/
│       └── ...
│
├── infrastructure/                    # 基础设施
│   ├── ingress-nginx/
│   │   ├── base/
│   │   └── overlays/
│   ├── cert-manager/
│   ├── prometheus/
│   └── istio/
│
└── clusters/                          # 集群特定配置
    ├── prod-us-west/
    │   └── cluster-config.yaml
    ├── prod-us-east/
    │   └── cluster-config.yaml
    └── dev/
        └── cluster-config.yaml
```

#### argocd/projects/production.yaml（ArgoCD 项目）

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AppProject
metadata:
  name: production
  namespace: argocd
  
  finalizers:
    - resources-finalizer.argocd.argoproj.io

spec:
  description: Production applications and infrastructure
  
  # 源仓库白名单
  sourceRepos:
    - https://github.com/myorg/k8s-manifests.git
    - https://charts.bitnami.com/bitnami
    - https://kubernetes.github.io/ingress-nginx
  
  # 目标集群和命名空间白名单
  destinations:
    - namespace: "*"                   # 允许所有命名空间
      server: https://prod-us-west.k8s.example.com
    - namespace: "*"
      server: https://prod-us-east.k8s.example.com
  
  # 集群资源白名单
  clusterResourceWhitelist:
    - group: ""
      kind: Namespace
    - group: rbac.authorization.k8s.io
      kind: ClusterRole
    - group: rbac.authorization.k8s.io
      kind: ClusterRoleBinding
    - group: apiextensions.k8s.io
      kind: CustomResourceDefinition
  
  # 命名空间资源白名单（空=允许所有）
  namespaceResourceWhitelist: []
  
  # 孤儿资源警告（检测不在 Git 中的资源）
  orphanedResources:
    warn: true
    ignore:
      - group: ""
        kind: ConfigMap
        name: kube-root-ca.crt         # 忽略系统 ConfigMap
  
  # 同步窗口（限制部署时间）
  syncWindows:
    - kind: allow                      # allow | deny
      schedule: "0 9-17 * * 1-5"       # Cron 表达式：工作日 9-17 点
      duration: 8h
      applications:
        - "*"
      manualSync: true                 # 允许手动同步
    
    - kind: deny                       # 禁止周末部署
      schedule: "0 0 * * 0,6"          # 周六、周日
      duration: 24h
      applications:
        - "*"
  
  # 角色（RBAC）
  roles:
    - name: sre-team
      description: SRE team full access
      policies:
        - p, proj:production:sre-team, applications, *, production/*, allow
      groups:
        - sre-team
    
    - name: developers
      description: Developers read-only
      policies:
        - p, proj:production:developers, applications, get, production/*, allow
      groups:
        - developers
```

#### argocd/applicationsets/apps.yaml（多环境应用）

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: microservices-apps
  namespace: argocd
spec:
  generators:
    # Matrix: 应用 × 环境 × 地区
    - matrix:
        generators:
          # 第一维：应用列表
          - git:
              repoURL: https://github.com/myorg/k8s-manifests.git
              revision: main
              directories:
                - path: apps/*
          
          # 第二维：环境和地区配置
          - list:
              elements:
                # 开发环境（单集群）
                - env: dev
                  cluster: dev
                  clusterUrl: https://dev.k8s.example.com
                  replicas: "1"
                  project: development
                  syncWave: "0"
                
                # 预发布环境（单集群）
                - env: staging
                  cluster: staging
                  clusterUrl: https://staging.k8s.example.com
                  replicas: "3"
                  project: staging
                  syncWave: "1"
                
                # 生产环境（多集群）
                - env: production
                  cluster: prod-us-west
                  clusterUrl: https://prod-us-west.k8s.example.com
                  region: us-west
                  replicas: "10"
                  project: production
                  syncWave: "2"
                
                - env: production
                  cluster: prod-us-east
                  clusterUrl: https://prod-us-east.k8s.example.com
                  region: us-east
                  replicas: "10"
                  project: production
                  syncWave: "2"
  
  template:
    metadata:
      name: "{{path.basename}}-{{env}}-{{cluster}}"
      namespace: argocd
      
      labels:
        app: "{{path.basename}}"
        environment: "{{env}}"
        cluster: "{{cluster}}"
        team: backend
      
      annotations:
        argocd.argoproj.io/sync-wave: "{{syncWave}}"
        notifications.argoproj.io/subscribe.on-deployed.slack: "deployments-{{env}}"
        notifications.argoproj.io/subscribe.on-sync-failed.slack: "alerts-{{env}}"
      
      finalizers:
        - resources-finalizer.argocd.argoproj.io
    
    spec:
      project: "{{project}}"
      
      source:
        repoURL: https://github.com/myorg/k8s-manifests.git
        targetRevision: main
        path: "{{path}}/overlays/{{env}}"
        
        kustomize:
          namePrefix: "{{cluster}}-"
          replicas:
            - name: "*"                # 覆盖所有 Deployment
              count: "{{replicas}}"
          commonLabels:
            environment: "{{env}}"
            cluster: "{{cluster}}"
            region: "{{region}}"
      
      destination:
        server: "{{clusterUrl}}"
        namespace: "{{path.basename}}-{{env}}"
      
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
          - CreateNamespace=true
          - PrunePropagationPolicy=foreground
          - PruneLast=true
        retry:
          limit: 5
          backoff:
            duration: 5s
            factor: 2
            maxDuration: 3m
      
      ignoreDifferences:
        - group: apps
          kind: Deployment
          jsonPointers:
            - /spec/replicas           # 由 HPA 管理
        - group: ""
          kind: Secret
          name: "*-tls"
          jsonPointers:
            - /data                    # 由 cert-manager 管理
```

#### 生产案例：带同步波次的完整应用

```yaml
# apps/backend/overlays/production/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

bases:
  - ../../base

namespace: backend-production

namePrefix: prod-

commonLabels:
  environment: production
  team: backend

# 为不同资源添加同步波次
patches:
  - target:
      kind: Namespace
    patch: |-
      - op: add
        path: /metadata/annotations/argocd.argoproj.io~1sync-wave
        value: "-10"
  
  - target:
      kind: Secret
    patch: |-
      - op: add
        path: /metadata/annotations/argocd.argoproj.io~1sync-wave
        value: "-5"
  
  - target:
      kind: ConfigMap
    patch: |-
      - op: add
        path: /metadata/annotations/argocd.argoproj.io~1sync-wave
        value: "-3"
  
  - target:
      kind: Deployment
    patch: |-
      - op: add
        path: /metadata/annotations/argocd.argoproj.io~1sync-wave
        value: "0"
  
  - target:
      kind: Service
    patch: |-
      - op: add
        path: /metadata/annotations/argocd.argoproj.io~1sync-wave
        value: "1"
  
  - target:
      kind: Ingress
    patch: |-
      - op: add
        path: /metadata/annotations/argocd.argoproj.io~1sync-wave
        value: "2"
```

---

## 📚 最佳实践总结

### Kustomize 最佳实践
1. **Base + Overlays 分离**：基础配置环境无关，环境特定配置在 overlays
2. **使用 Components**：可选功能模块化（监控、TLS 等）
3. **生产环境使用镜像摘要**：确保不可变部署
4. **避免过度嵌套**：最多 2-3 层 bases 引用
5. **使用 replacements 替代废弃的 vars**

### Helm 最佳实践
1. **values.yaml 结构化**：按功能分组，使用注释
2. **命名模板复用**：`_helpers.tpl` 中定义通用逻辑
3. **生产环境固定版本**：Chart 和镜像使用固定版本/摘要
4. **健康检查必需**：Liveness、Readiness、Startup 探针
5. **NOTES.txt 提供访问信息**：部署后如何访问应用

### ArgoCD 最佳实践
1. **使用 ApplicationSet 管理多环境**：减少重复配置
2. **同步波次控制顺序**：Namespace → Secret → ConfigMap → Deployment → Service → Ingress
3. **ignoreDifferences 避免误报**：忽略 HPA 管理的 replicas、cert-manager 管理的 Secret
4. **自动同步 + Self Heal**：实现真正的 GitOps
5. **AppProject 实现多租户**：不同团队/环境使用不同项目

---

## 🔗 相关文档
- [35 - Gateway API 和 API Gateway](./35-gateway-api-overview.md)
- [34 - Ingress 和 IngressClass](./34-ingress-ingressclass.md)
- [33 - PodMonitor 和 ServiceMonitor](./33-podmonitor-servicemonitor.md)

---

**文档状态**: ✅ 已完成 | **维护者**: SRE Team | **最后审核**: 2026-02-10
