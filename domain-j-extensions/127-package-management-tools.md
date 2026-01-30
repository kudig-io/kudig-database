# 101 - 包管理与应用分发工具 (Package Management & Distribution)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 中级-高级

## 包管理工具生态架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                      Kubernetes 应用包管理与分发生态                                  │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                          源代码 / 配置层 (Source)                             │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │   GitHub    │  │   GitLab    │  │   Gitea     │  │      Bitbucket      │  │   │
│  │  │    Repo     │  │    Repo     │  │    Repo     │  │        Repo         │  │   │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘  │   │
│  └─────────┼────────────────┼────────────────┼───────────────────┼─────────────┘   │
│            │                │                │                   │                  │
│            └────────────────┴────────────────┴───────────────────┘                  │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                         打包工具层 (Packaging Tools)                          │   │
│  │                                                                               │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                           模板化方案                                     │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────────┐  │ │   │
│  │  │  │    Helm     │  │  Kustomize  │  │   Jsonnet   │  │     CUE       │  │ │   │
│  │  │  │  (Charts)   │  │  (Overlays) │  │  (Library)  │  │  (Schema)     │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘  └───────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────────────────────────┘ │   │
│  │                                                                               │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                           扩展管理方案                                   │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────────┐  │ │   │
│  │  │  │  Operator   │  │  Crossplane │  │   Carvel    │  │    Timoni     │  │ │   │
│  │  │  │    SDK      │  │ (Platform)  │  │   (Suite)   │  │   (Module)    │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘  └───────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                          制品仓库层 (Artifact Registry)                       │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │   Harbor    │  │ ChartMuseum │  │    ACR      │  │  阿里云 CR / ECR    │  │   │
│  │  │(OCI Charts) │  │ (HTTP API)  │  │  (Azure)    │  │  (OCI Registry)     │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                           GitOps 部署层 (Deployment)                          │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │   ArgoCD    │  │    Flux     │  │  Jenkins X  │  │     Rancher Fleet   │  │   │
│  │  │(Application)│  │(HelmRelease)│  │  (Pipeline) │  │   (GitOps at Scale) │  │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────────────┘  │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                         Kubernetes 集群 (Cluster)                             │   │
│  │                                                                               │   │
│  │  ┌───────────────────────────────────────────────────────────────────────┐   │   │
│  │  │  Deployments │ Services │ ConfigMaps │ Secrets │ CRDs │ Operators     │   │   │
│  │  └───────────────────────────────────────────────────────────────────────┘   │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

## 核心工具对比矩阵

| 工具 (Tool) | 核心定位 (Position) | 生产优势 (Production Advantages) | 学习曲线 | ACK 集成 |
|------------|-------------------|--------------------------------|---------|---------|
| **Helm** | 应用包管理器 | Chart 版本控制、回滚、依赖管理、生态丰富 | 中 | ✅ 原生支持 |
| **Kustomize** | 声明式配置管理 | 无模板、原生集成、多环境叠加、易审计 | 低 | ✅ kubectl 内置 |
| **Operator SDK** | 自动化运维框架 | 生命周期管理、自愈能力、领域知识封装 | 高 | ✅ OLM 支持 |
| **ArgoCD** | GitOps 持续部署 | 声明式、自动同步、审计、多集群 | 中 | ✅ 可集成 |
| **Flux** | GitOps 工具套件 | 轻量、模块化、Helm/Kustomize 原生支持 | 中 | ✅ 可集成 |
| **Carvel** | 工具套件 | ytt模板、kapp部署、vendir依赖 | 中 | ⚠️ 需配置 |
| **Crossplane** | 基础设施即代码 | 云资源统一管理、Composition | 高 | ⚠️ 需配置 |
| **Timoni** | CUE 模块分发 | 强类型、模块化、OCI 原生 | 高 | ⚠️ 新兴 |

## Helm 生产最佳实践

### 1. Chart 仓库管理架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                           Helm Chart 仓库架构                                    │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌──────────────────────────────────────────────────────────────────────────┐   │
│  │                          Chart 来源分类                                   │   │
│  │                                                                           │   │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────────────┐   │   │
│  │  │   公共仓库       │  │   私有仓库       │  │     OCI 仓库             │   │   │
│  │  │                 │  │                 │  │                         │   │   │
│  │  │ • ArtifactHub   │  │ • Harbor        │  │ • ACR (阿里云)          │   │   │
│  │  │ • Bitnami       │  │ • ChartMuseum   │  │ • ECR (AWS)             │   │   │
│  │  │ • Helm Stable   │  │ • GitLab Pkg    │  │ • GCR (GCP)             │   │   │
│  │  │ • Prometheus    │  │ • JFrog Artifct │  │ • Harbor (OCI)          │   │   │
│  │  └─────────────────┘  └─────────────────┘  └─────────────────────────┘   │   │
│  └──────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                          │
│                                      ▼                                          │
│  ┌──────────────────────────────────────────────────────────────────────────┐   │
│  │                       Chart 版本管理策略                                   │   │
│  │                                                                           │   │
│  │  开发环境 ←─ 0.1.0-dev.xxx ─→ 测试环境 ←─ 0.1.0-rc.x ─→ 生产环境 ←─ 0.1.0 │   │
│  │                                                                           │   │
│  │  SemVer 规范:                                                             │   │
│  │  • MAJOR.MINOR.PATCH (1.2.3)                                              │   │
│  │  • Pre-release: 1.2.3-alpha.1, 1.2.3-beta.2, 1.2.3-rc.1                   │   │
│  │  • Build metadata: 1.2.3+build.12345                                       │   │
│  └──────────────────────────────────────────────────────────────────────────┘   │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 2. Chart 仓库操作大全

```bash
# ==================== 仓库管理 ====================

# 添加公共仓库
helm repo add bitnami https://charts.bitnami.com/bitnami
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
helm repo add jetstack https://charts.jetstack.io
helm repo add grafana https://grafana.github.io/helm-charts

# 添加私有仓库 (带认证)
helm repo add my-private https://charts.example.com \
  --username admin \
  --password "$HELM_REPO_PASSWORD" \
  --ca-file /path/to/ca.crt

# 更新仓库索引
helm repo update

# 列出所有仓库
helm repo list

# 搜索 Chart
helm search repo nginx
helm search repo nginx --versions  # 显示所有版本
helm search hub nginx              # 搜索 ArtifactHub

# 获取 Chart 信息
helm show chart bitnami/nginx
helm show values bitnami/nginx
helm show readme bitnami/nginx
helm show all bitnami/nginx

# ==================== OCI 仓库操作 (推荐) ====================

# 登录 OCI 仓库
helm registry login registry.cn-hangzhou.aliyuncs.com
helm registry login ghcr.io -u $GITHUB_USER -p $GITHUB_TOKEN

# 推送 Chart 到 OCI 仓库
helm package ./mychart
helm push mychart-0.1.0.tgz oci://registry.cn-hangzhou.aliyuncs.com/mycharts

# 从 OCI 仓库拉取
helm pull oci://registry.cn-hangzhou.aliyuncs.com/mycharts/mychart --version 0.1.0

# 直接从 OCI 仓库安装
helm install myrelease oci://registry.cn-hangzhou.aliyuncs.com/mycharts/mychart \
  --version 0.1.0

# ==================== Chart 开发 ====================

# 创建新 Chart
helm create mychart

# Chart 目录结构
# mychart/
# ├── Chart.yaml          # Chart 元数据
# ├── Chart.lock          # 依赖锁定文件
# ├── values.yaml         # 默认配置值
# ├── values.schema.json  # 值验证 Schema
# ├── .helmignore         # 打包忽略规则
# ├── templates/          # 模板目录
# │   ├── _helpers.tpl    # 模板助手函数
# │   ├── deployment.yaml
# │   ├── service.yaml
# │   ├── ingress.yaml
# │   ├── hpa.yaml
# │   ├── serviceaccount.yaml
# │   ├── NOTES.txt       # 安装后提示
# │   └── tests/          # Chart 测试
# │       └── test-connection.yaml
# └── charts/             # 子 Chart 目录

# 验证 Chart
helm lint ./mychart
helm lint ./mychart --strict  # 严格模式

# 模板渲染调试
helm template myrelease ./mychart
helm template myrelease ./mychart -f values-prod.yaml
helm template myrelease ./mychart --debug  # 显示详细信息

# 打包 Chart
helm package ./mychart
helm package ./mychart --version 1.0.0 --app-version 2.0.0

# ==================== 依赖管理 ====================

# Chart.yaml 中定义依赖
# dependencies:
#   - name: postgresql
#     version: "12.x.x"
#     repository: https://charts.bitnami.com/bitnami
#     condition: postgresql.enabled
#   - name: redis
#     version: "17.x.x"
#     repository: https://charts.bitnami.com/bitnami
#     condition: redis.enabled

# 更新依赖
helm dependency update ./mychart

# 构建依赖
helm dependency build ./mychart

# 列出依赖
helm dependency list ./mychart
```

### 3. 生产级 Values 文件策略

```yaml
# values-base.yaml - 基础配置 (所有环境共用)
replicaCount: 1

image:
  repository: myapp
  pullPolicy: IfNotPresent
  tag: ""  # 由 CI/CD 设置

imagePullSecrets:
  - name: regcred

serviceAccount:
  create: true
  annotations: {}
  name: ""

podAnnotations:
  prometheus.io/scrape: "true"
  prometheus.io/port: "8080"
  prometheus.io/path: "/metrics"

podSecurityContext:
  runAsNonRoot: true
  runAsUser: 1000
  runAsGroup: 1000
  fsGroup: 1000

securityContext:
  allowPrivilegeEscalation: false
  readOnlyRootFilesystem: true
  capabilities:
    drop:
      - ALL

service:
  type: ClusterIP
  port: 80
  targetPort: 8080

resources:
  requests:
    cpu: 100m
    memory: 128Mi
  limits:
    cpu: 500m
    memory: 512Mi

nodeSelector: {}

tolerations: []

affinity: {}

# 存活探针
livenessProbe:
  httpGet:
    path: /healthz
    port: http
  initialDelaySeconds: 10
  periodSeconds: 10
  timeoutSeconds: 5
  failureThreshold: 3

# 就绪探针
readinessProbe:
  httpGet:
    path: /ready
    port: http
  initialDelaySeconds: 5
  periodSeconds: 5
  timeoutSeconds: 3
  failureThreshold: 3

# 启动探针 (慢启动应用)
startupProbe:
  httpGet:
    path: /healthz
    port: http
  initialDelaySeconds: 0
  periodSeconds: 10
  timeoutSeconds: 5
  failureThreshold: 30

# 配置管理
config:
  # 从 ConfigMap 加载
  fromConfigMap: true
  configMapName: ""  # 自动生成或指定

# 密钥管理
secrets:
  # 从 Secret 加载
  fromSecret: true
  secretName: ""  # 自动生成或指定
  externalSecret:
    enabled: false
    refreshInterval: 1h
    secretStoreRef:
      name: vault-backend
      kind: ClusterSecretStore

# 持久化存储
persistence:
  enabled: false
  storageClass: ""
  accessModes:
    - ReadWriteOnce
  size: 10Gi
  annotations: {}

# 依赖组件
postgresql:
  enabled: false
  auth:
    existingSecret: ""
    database: myapp
    username: myapp

redis:
  enabled: false
  architecture: standalone
  auth:
    existingSecret: ""
```

```yaml
# values-dev.yaml - 开发环境覆盖
replicaCount: 1

image:
  tag: "dev-latest"

resources:
  requests:
    cpu: 50m
    memory: 64Mi
  limits:
    cpu: 200m
    memory: 256Mi

# 开发环境禁用探针加快启动
livenessProbe:
  initialDelaySeconds: 30

# 开发环境额外配置
extraEnv:
  - name: DEBUG
    value: "true"
  - name: LOG_LEVEL
    value: "debug"

# 开启本地依赖
postgresql:
  enabled: true
  primary:
    persistence:
      enabled: false

redis:
  enabled: true
  master:
    persistence:
      enabled: false
```

```yaml
# values-staging.yaml - 预发布环境覆盖
replicaCount: 2

image:
  tag: ""  # 由 CI/CD 设置

resources:
  requests:
    cpu: 200m
    memory: 256Mi
  limits:
    cpu: 500m
    memory: 512Mi

ingress:
  enabled: true
  className: nginx
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
  hosts:
    - host: myapp-staging.example.com
      paths:
        - path: /
          pathType: Prefix
  tls:
    - secretName: myapp-staging-tls
      hosts:
        - myapp-staging.example.com

# 使用外部数据库
postgresql:
  enabled: false

# 外部数据库配置通过 ExternalSecret 注入
secrets:
  externalSecret:
    enabled: true
    refreshInterval: 1h
```

```yaml
# values-prod.yaml - 生产环境覆盖
replicaCount: 3

image:
  tag: ""  # 由 CI/CD 设置精确版本

resources:
  requests:
    cpu: 500m
    memory: 512Mi
  limits:
    cpu: 2
    memory: 2Gi

# 高可用配置
podDisruptionBudget:
  enabled: true
  minAvailable: 2

# 反亲和性 - 分散到不同节点
affinity:
  podAntiAffinity:
    preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        podAffinityTerm:
          labelSelector:
            matchExpressions:
              - key: app.kubernetes.io/name
                operator: In
                values:
                  - myapp
          topologyKey: kubernetes.io/hostname

# 拓扑分布约束
topologySpreadConstraints:
  - maxSkew: 1
    topologyKey: topology.kubernetes.io/zone
    whenUnsatisfiable: ScheduleAnyway
    labelSelector:
      matchLabels:
        app.kubernetes.io/name: myapp

# 生产 Ingress
ingress:
  enabled: true
  className: nginx
  annotations:
    cert-manager.io/cluster-issuer: letsencrypt-prod
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
    nginx.ingress.kubernetes.io/proxy-body-size: "50m"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
    nginx.ingress.kubernetes.io/limit-rps: "100"
  hosts:
    - host: myapp.example.com
      paths:
        - path: /
          pathType: Prefix
  tls:
    - secretName: myapp-prod-tls
      hosts:
        - myapp.example.com

# 自动扩缩
autoscaling:
  enabled: true
  minReplicas: 3
  maxReplicas: 10
  targetCPUUtilizationPercentage: 70
  targetMemoryUtilizationPercentage: 80
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
        - type: Pods
          value: 1
          periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 0
      policies:
        - type: Percent
          value: 100
          periodSeconds: 15
        - type: Pods
          value: 4
          periodSeconds: 15
      selectPolicy: Max

# 持久化 (如需要)
persistence:
  enabled: true
  storageClass: alicloud-disk-ssd
  size: 50Gi

# 监控
metrics:
  enabled: true
  serviceMonitor:
    enabled: true
    interval: 30s
    scrapeTimeout: 10s

# 外部密钥管理
secrets:
  externalSecret:
    enabled: true
    refreshInterval: 30m
    secretStoreRef:
      name: vault-backend
      kind: ClusterSecretStore
```

### 4. Helm 版本管理与回滚

```bash
# ==================== 部署管理 ====================

# 安装 Release
helm install myapp ./mychart \
  -n production \
  -f values-base.yaml \
  -f values-prod.yaml \
  --set image.tag=v1.2.3

# 升级 Release
helm upgrade myapp ./mychart \
  -n production \
  -f values-base.yaml \
  -f values-prod.yaml \
  --set image.tag=v1.2.4

# 安装或升级 (推荐)
helm upgrade --install myapp ./mychart \
  -n production \
  --create-namespace \
  -f values-base.yaml \
  -f values-prod.yaml \
  --set image.tag=v1.2.4 \
  --atomic \
  --timeout 10m \
  --wait

# 参数说明:
# --atomic: 失败时自动回滚
# --timeout: 等待超时时间
# --wait: 等待所有资源就绪
# --wait-for-jobs: 等待所有 Jobs 完成

# ==================== 历史与回滚 ====================

# 查看发布历史
helm history myapp -n production

# 输出示例:
# REVISION  UPDATED                   STATUS      CHART         APP VERSION  DESCRIPTION
# 1         Mon Jan 15 10:00:00 2024  superseded  myapp-0.1.0   1.0.0        Install complete
# 2         Tue Jan 16 11:00:00 2024  superseded  myapp-0.1.1   1.0.1        Upgrade complete
# 3         Wed Jan 17 12:00:00 2024  deployed    myapp-0.1.2   1.0.2        Upgrade complete

# 回滚到上一个版本
helm rollback myapp -n production

# 回滚到指定版本
helm rollback myapp 1 -n production

# 回滚并等待
helm rollback myapp 2 -n production --wait --timeout 5m

# ==================== 状态查看 ====================

# 查看 Release 状态
helm status myapp -n production

# 查看 Release 的 manifests
helm get manifest myapp -n production

# 查看 Release 的 values
helm get values myapp -n production
helm get values myapp -n production -a  # 包含默认值

# 查看 Release 的 notes
helm get notes myapp -n production

# 查看所有 Release
helm list -n production
helm list -A  # 所有命名空间

# ==================== 测试 ====================

# 运行 Chart 测试
helm test myapp -n production

# 测试并保留测试 Pod 日志
helm test myapp -n production --logs

# ==================== 卸载 ====================

# 卸载 Release
helm uninstall myapp -n production

# 卸载但保留历史
helm uninstall myapp -n production --keep-history

# 彻底删除 (包括保留的历史)
helm uninstall myapp -n production --no-hooks

# ==================== Diff 插件 (推荐) ====================

# 安装 helm-diff 插件
helm plugin install https://github.com/databus23/helm-diff

# 查看升级差异
helm diff upgrade myapp ./mychart \
  -n production \
  -f values-base.yaml \
  -f values-prod.yaml \
  --set image.tag=v1.2.5

# 查看回滚差异
helm diff rollback myapp 2 -n production

# 查看与指定版本的差异
helm diff revision myapp 2 3 -n production
```

### 5. Helm 模板高级技巧

```yaml
# templates/_helpers.tpl - 助手函数
{{/*
扩展标签选择器
*/}}
{{- define "myapp.selectorLabels" -}}
app.kubernetes.io/name: {{ include "myapp.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}

{{/*
公共标签
*/}}
{{- define "myapp.labels" -}}
helm.sh/chart: {{ include "myapp.chart" . }}
{{ include "myapp.selectorLabels" . }}
{{- if .Chart.AppVersion }}
app.kubernetes.io/version: {{ .Chart.AppVersion | quote }}
{{- end }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- end }}

{{/*
创建镜像引用
*/}}
{{- define "myapp.image" -}}
{{- $registry := .Values.image.registry | default "docker.io" -}}
{{- $repository := .Values.image.repository -}}
{{- $tag := .Values.image.tag | default .Chart.AppVersion -}}
{{- printf "%s/%s:%s" $registry $repository $tag -}}
{{- end }}

{{/*
生成资源名称 (带长度限制)
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
生成 checksum annotation - 配置变更时触发重启
*/}}
{{- define "myapp.configChecksum" -}}
checksum/config: {{ include (print $.Template.BasePath "/configmap.yaml") . | sha256sum }}
checksum/secret: {{ include (print $.Template.BasePath "/secret.yaml") . | sha256sum }}
{{- end }}
```

```yaml
# templates/deployment.yaml - 高级模板示例
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "myapp.fullname" . }}
  labels:
    {{- include "myapp.labels" . | nindent 4 }}
spec:
  {{- if not .Values.autoscaling.enabled }}
  replicas: {{ .Values.replicaCount }}
  {{- end }}
  selector:
    matchLabels:
      {{- include "myapp.selectorLabels" . | nindent 6 }}
  template:
    metadata:
      annotations:
        {{- include "myapp.configChecksum" . | nindent 8 }}
        {{- with .Values.podAnnotations }}
        {{- toYaml . | nindent 8 }}
        {{- end }}
      labels:
        {{- include "myapp.selectorLabels" . | nindent 8 }}
        {{- with .Values.podLabels }}
        {{- toYaml . | nindent 8 }}
        {{- end }}
    spec:
      {{- with .Values.imagePullSecrets }}
      imagePullSecrets:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      serviceAccountName: {{ include "myapp.serviceAccountName" . }}
      securityContext:
        {{- toYaml .Values.podSecurityContext | nindent 8 }}
      
      {{- if .Values.initContainers }}
      initContainers:
        {{- toYaml .Values.initContainers | nindent 8 }}
      {{- end }}
      
      containers:
        - name: {{ .Chart.Name }}
          securityContext:
            {{- toYaml .Values.securityContext | nindent 12 }}
          image: {{ include "myapp.image" . }}
          imagePullPolicy: {{ .Values.image.pullPolicy }}
          ports:
            - name: http
              containerPort: {{ .Values.service.targetPort }}
              protocol: TCP
          
          {{- if .Values.livenessProbe }}
          livenessProbe:
            {{- toYaml .Values.livenessProbe | nindent 12 }}
          {{- end }}
          
          {{- if .Values.readinessProbe }}
          readinessProbe:
            {{- toYaml .Values.readinessProbe | nindent 12 }}
          {{- end }}
          
          {{- if .Values.startupProbe }}
          startupProbe:
            {{- toYaml .Values.startupProbe | nindent 12 }}
          {{- end }}
          
          env:
            - name: POD_NAME
              valueFrom:
                fieldRef:
                  fieldPath: metadata.name
            - name: POD_NAMESPACE
              valueFrom:
                fieldRef:
                  fieldPath: metadata.namespace
            {{- if .Values.extraEnv }}
            {{- toYaml .Values.extraEnv | nindent 12 }}
            {{- end }}
          
          {{- if or .Values.config.fromConfigMap .Values.secrets.fromSecret }}
          envFrom:
            {{- if .Values.config.fromConfigMap }}
            - configMapRef:
                name: {{ .Values.config.configMapName | default (include "myapp.fullname" .) }}
            {{- end }}
            {{- if .Values.secrets.fromSecret }}
            - secretRef:
                name: {{ .Values.secrets.secretName | default (printf "%s-secret" (include "myapp.fullname" .)) }}
            {{- end }}
          {{- end }}
          
          resources:
            {{- toYaml .Values.resources | nindent 12 }}
          
          volumeMounts:
            {{- if .Values.persistence.enabled }}
            - name: data
              mountPath: /data
            {{- end }}
            {{- if .Values.extraVolumeMounts }}
            {{- toYaml .Values.extraVolumeMounts | nindent 12 }}
            {{- end }}
      
      volumes:
        {{- if .Values.persistence.enabled }}
        - name: data
          persistentVolumeClaim:
            claimName: {{ .Values.persistence.existingClaim | default (include "myapp.fullname" .) }}
        {{- end }}
        {{- if .Values.extraVolumes }}
        {{- toYaml .Values.extraVolumes | nindent 8 }}
        {{- end }}
      
      {{- with .Values.nodeSelector }}
      nodeSelector:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      
      {{- with .Values.affinity }}
      affinity:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      
      {{- with .Values.tolerations }}
      tolerations:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      
      {{- with .Values.topologySpreadConstraints }}
      topologySpreadConstraints:
        {{- toYaml . | nindent 8 }}
      {{- end }}
```

## Kustomize 多环境管理

### 1. 完整目录结构

```
├── base/
│   ├── kustomization.yaml
│   ├── deployment.yaml
│   ├── service.yaml
│   ├── configmap.yaml
│   ├── hpa.yaml
│   └── ingress.yaml
│
├── components/
│   ├── monitoring/
│   │   ├── kustomization.yaml
│   │   └── servicemonitor.yaml
│   ├── logging/
│   │   ├── kustomization.yaml
│   │   └── fluent-bit-sidecar.yaml
│   └── security/
│       ├── kustomization.yaml
│       ├── network-policy.yaml
│       └── pod-security-policy.yaml
│
└── overlays/
    ├── dev/
    │   ├── kustomization.yaml
    │   ├── namespace.yaml
    │   └── patches/
    │       └── replica-count.yaml
    ├── staging/
    │   ├── kustomization.yaml
    │   ├── namespace.yaml
    │   └── patches/
    │       ├── replica-count.yaml
    │       └── resources.yaml
    └── prod/
        ├── kustomization.yaml
        ├── namespace.yaml
        ├── secrets/
        │   └── database-credentials.enc.yaml
        └── patches/
            ├── replica-count.yaml
            ├── resources.yaml
            ├── hpa.yaml
            └── node-affinity.yaml
```

### 2. 基础配置 (base/)

```yaml
# base/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

metadata:
  name: myapp-base

# 公共标签
commonLabels:
  app.kubernetes.io/name: myapp
  app.kubernetes.io/part-of: myapp-system

# 公共注解
commonAnnotations:
  app.kubernetes.io/managed-by: kustomize

resources:
  - deployment.yaml
  - service.yaml
  - configmap.yaml
  - hpa.yaml
  - ingress.yaml

# 配置生成器
configMapGenerator:
  - name: myapp-config
    literals:
      - LOG_LEVEL=info
      - MAX_CONNECTIONS=100
    files:
      - configs/app.properties

# 镜像配置 (可被 overlay 覆盖)
images:
  - name: myapp
    newName: myregistry.example.com/myapp
    newTag: latest
```

```yaml
# base/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      containers:
        - name: myapp
          image: myapp
          ports:
            - containerPort: 8080
              name: http
          envFrom:
            - configMapRef:
                name: myapp-config
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 512Mi
          livenessProbe:
            httpGet:
              path: /healthz
              port: http
            initialDelaySeconds: 10
            periodSeconds: 10
          readinessProbe:
            httpGet:
              path: /ready
              port: http
            initialDelaySeconds: 5
            periodSeconds: 5
```

### 3. 环境覆盖 (overlays/)

```yaml
# overlays/prod/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

metadata:
  name: myapp-prod

namespace: production

resources:
  - ../../base
  - namespace.yaml

# 引入可复用组件
components:
  - ../../components/monitoring
  - ../../components/security

# 名称前缀/后缀
namePrefix: prod-
# nameSuffix: -v1

# 生产环境标签
commonLabels:
  environment: production
  tier: backend

# 生产环境注解
commonAnnotations:
  owner: platform-team@example.com

# 副本数覆盖
replicas:
  - name: myapp
    count: 5

# 镜像覆盖
images:
  - name: myapp
    newName: myregistry.example.com/myapp
    digest: sha256:abc123...  # 使用 digest 确保可复现

# 配置替换
configMapGenerator:
  - name: myapp-config
    behavior: merge
    literals:
      - LOG_LEVEL=warn
      - MAX_CONNECTIONS=500
      - ENVIRONMENT=production

# Secret 生成 (SOPS 加密)
secretGenerator:
  - name: myapp-secrets
    files:
      - secrets/database-credentials.enc.yaml
    options:
      disableNameSuffixHash: true

# JSON 补丁
patches:
  # Strategic Merge Patch
  - path: patches/replica-count.yaml
  - path: patches/resources.yaml
  - path: patches/hpa.yaml
  - path: patches/node-affinity.yaml
  
  # Inline Patch
  - patch: |-
      apiVersion: apps/v1
      kind: Deployment
      metadata:
        name: myapp
      spec:
        template:
          spec:
            containers:
              - name: myapp
                env:
                  - name: ENVIRONMENT
                    value: production
  
  # JSON6902 Patch
  - target:
      group: apps
      version: v1
      kind: Deployment
      name: myapp
    patch: |-
      - op: add
        path: /spec/template/spec/containers/0/env/-
        value:
          name: FEATURE_FLAG
          value: "enabled"
      - op: replace
        path: /spec/template/spec/containers/0/resources/limits/memory
        value: 2Gi

# 转换器配置
transformers:
  - |-
    apiVersion: builtin
    kind: PrefixSuffixTransformer
    metadata:
      name: customPrefixSuffix
    prefix: mycompany-
    fieldSpecs:
      - path: metadata/name

# 验证器
validators:
  - |-
    apiVersion: builtin
    kind: ReplicaCountTransform
    metadata:
      name: replicaValidator
    replica: 3
    fieldSpecs:
      - path: spec/replicas
        kind: Deployment
```

```yaml
# overlays/prod/patches/resources.yaml
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
            requests:
              cpu: 500m
              memory: 512Mi
            limits:
              cpu: 2
              memory: 2Gi
```

```yaml
# overlays/prod/patches/hpa.yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: myapp
spec:
  minReplicas: 3
  maxReplicas: 20
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70
    - type: Resource
      resource:
        name: memory
        target:
          type: Utilization
          averageUtilization: 80
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
        - type: Pods
          value: 1
          periodSeconds: 60
```

```yaml
# overlays/prod/patches/node-affinity.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  template:
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchLabels:
                    app: myapp
                topologyKey: kubernetes.io/hostname
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
              - matchExpressions:
                  - key: node-type
                    operator: In
                    values:
                      - production
                      - high-memory
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: myapp
```

### 4. Kustomize 操作命令

```bash
# ==================== 构建与预览 ====================

# 构建并输出 YAML
kustomize build overlays/prod

# 使用 kubectl kustomize
kubectl kustomize overlays/prod

# 构建并应用
kustomize build overlays/prod | kubectl apply -f -

# 或直接使用 kubectl
kubectl apply -k overlays/prod

# 预览删除
kubectl delete -k overlays/prod --dry-run=client

# ==================== 编辑操作 ====================

# 设置镜像
kustomize edit set image myapp=myregistry.example.com/myapp:v1.2.3

# 添加标签
kustomize edit add label env:production --without-selector

# 添加注解
kustomize edit add annotation owner:platform-team

# 设置命名空间
kustomize edit set namespace production

# 添加资源
kustomize edit add resource deployment.yaml

# 添加 ConfigMap 生成器
kustomize edit add configmap my-config --from-file=config.properties

# ==================== 差异对比 ====================

# 查看与集群当前状态的差异
kubectl diff -k overlays/prod

# 对比两个 overlay
diff <(kustomize build overlays/staging) <(kustomize build overlays/prod)

# 使用 dyff 进行语义差异对比
kustomize build overlays/staging > staging.yaml
kustomize build overlays/prod > prod.yaml
dyff between staging.yaml prod.yaml

# ==================== 验证 ====================

# 客户端验证
kustomize build overlays/prod | kubectl apply --dry-run=client -f -

# 服务端验证
kustomize build overlays/prod | kubectl apply --dry-run=server -f -

# 使用 kubeval 验证
kustomize build overlays/prod | kubeval --strict

# 使用 kubeconform 验证
kustomize build overlays/prod | kubeconform -strict -summary
```

## Operator 开发模式

### 1. Operator 类型对比

| 模式 (Pattern) | 适用场景 (Use Case) | 优势 | 复杂度 | 推荐场景 |
|---------------|-------------------|------|-------|---------|
| **Helm Operator** | 简单应用打包 | 快速、利用现有 Chart | 低 | 基础应用部署 |
| **Ansible Operator** | 运维自动化 | 复用 Ansible Playbook | 中 | 传统运维迁移 |
| **Go Operator** | 复杂状态管理 | 完全控制、高性能 | 高 | 有状态应用、数据库 |

### 2. Go Operator 完整示例

```go
// api/v1/database_types.go
package v1

import (
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
)

// DatabaseSpec 定义 Database 的期望状态
type DatabaseSpec struct {
    // Size 是数据库实例数量
    // +kubebuilder:validation:Minimum=1
    // +kubebuilder:validation:Maximum=10
    // +kubebuilder:default=1
    Size int32 `json:"size"`
    
    // Version 是数据库版本
    // +kubebuilder:validation:Enum=14;15;16
    // +kubebuilder:default="15"
    Version string `json:"version,omitempty"`
    
    // Storage 配置
    Storage StorageSpec `json:"storage"`
    
    // Resources 资源配置
    Resources ResourcesSpec `json:"resources,omitempty"`
    
    // Backup 备份配置
    // +optional
    Backup *BackupSpec `json:"backup,omitempty"`
}

// StorageSpec 存储配置
type StorageSpec struct {
    // StorageClassName 存储类
    StorageClassName string `json:"storageClassName"`
    
    // Size 存储大小
    // +kubebuilder:validation:Pattern=^[0-9]+[KMGT]i$
    Size string `json:"size"`
}

// ResourcesSpec 资源配置
type ResourcesSpec struct {
    // CPU 请求和限制
    CPU string `json:"cpu,omitempty"`
    
    // Memory 请求和限制
    Memory string `json:"memory,omitempty"`
}

// BackupSpec 备份配置
type BackupSpec struct {
    // Enabled 是否启用备份
    Enabled bool `json:"enabled"`
    
    // Schedule Cron 表达式
    Schedule string `json:"schedule,omitempty"`
    
    // Retention 保留天数
    // +kubebuilder:validation:Minimum=1
    // +kubebuilder:default=7
    Retention int `json:"retention,omitempty"`
    
    // S3Bucket 备份存储桶
    S3Bucket string `json:"s3Bucket,omitempty"`
}

// DatabaseStatus 定义 Database 的观察状态
type DatabaseStatus struct {
    // Phase 当前阶段
    // +kubebuilder:validation:Enum=Pending;Creating;Running;Failed;Upgrading;Deleting
    Phase string `json:"phase,omitempty"`
    
    // Conditions 状态条件
    Conditions []metav1.Condition `json:"conditions,omitempty"`
    
    // ReadyReplicas 就绪副本数
    ReadyReplicas int32 `json:"readyReplicas,omitempty"`
    
    // Endpoint 连接端点
    Endpoint string `json:"endpoint,omitempty"`
    
    // LastBackupTime 最后备份时间
    LastBackupTime *metav1.Time `json:"lastBackupTime,omitempty"`
}

// +kubebuilder:object:root=true
// +kubebuilder:subresource:status
// +kubebuilder:printcolumn:name="Phase",type="string",JSONPath=".status.phase"
// +kubebuilder:printcolumn:name="Size",type="integer",JSONPath=".spec.size"
// +kubebuilder:printcolumn:name="Ready",type="integer",JSONPath=".status.readyReplicas"
// +kubebuilder:printcolumn:name="Age",type="date",JSONPath=".metadata.creationTimestamp"

// Database 是 databases API 的 Schema
type Database struct {
    metav1.TypeMeta   `json:",inline"`
    metav1.ObjectMeta `json:"metadata,omitempty"`

    Spec   DatabaseSpec   `json:"spec,omitempty"`
    Status DatabaseStatus `json:"status,omitempty"`
}

// +kubebuilder:object:root=true

// DatabaseList 包含 Database 列表
type DatabaseList struct {
    metav1.TypeMeta `json:",inline"`
    metav1.ListMeta `json:"metadata,omitempty"`
    Items           []Database `json:"items"`
}
```

```go
// controllers/database_controller.go
package controllers

import (
    "context"
    "fmt"
    "time"

    "github.com/go-logr/logr"
    appsv1 "k8s.io/api/apps/v1"
    corev1 "k8s.io/api/core/v1"
    "k8s.io/apimachinery/pkg/api/errors"
    "k8s.io/apimachinery/pkg/api/meta"
    metav1 "k8s.io/apimachinery/pkg/apis/meta/v1"
    "k8s.io/apimachinery/pkg/runtime"
    "k8s.io/apimachinery/pkg/types"
    ctrl "sigs.k8s.io/controller-runtime"
    "sigs.k8s.io/controller-runtime/pkg/client"
    "sigs.k8s.io/controller-runtime/pkg/controller/controllerutil"

    dbv1 "github.com/example/database-operator/api/v1"
)

const (
    databaseFinalizer = "database.example.com/finalizer"
    
    // 状态常量
    PhaseRunning  = "Running"
    PhasePending  = "Pending"
    PhaseCreating = "Creating"
    PhaseFailed   = "Failed"
)

// DatabaseReconciler 调和 Database 资源
type DatabaseReconciler struct {
    client.Client
    Log    logr.Logger
    Scheme *runtime.Scheme
}

// +kubebuilder:rbac:groups=db.example.com,resources=databases,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=db.example.com,resources=databases/status,verbs=get;update;patch
// +kubebuilder:rbac:groups=db.example.com,resources=databases/finalizers,verbs=update
// +kubebuilder:rbac:groups=apps,resources=statefulsets,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=core,resources=services,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=core,resources=secrets,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=core,resources=configmaps,verbs=get;list;watch;create;update;patch;delete
// +kubebuilder:rbac:groups=core,resources=persistentvolumeclaims,verbs=get;list;watch;create;update;patch;delete

func (r *DatabaseReconciler) Reconcile(ctx context.Context, req ctrl.Request) (ctrl.Result, error) {
    log := r.Log.WithValues("database", req.NamespacedName)

    // 获取 Database 实例
    database := &dbv1.Database{}
    if err := r.Get(ctx, req.NamespacedName, database); err != nil {
        if errors.IsNotFound(err) {
            log.Info("Database resource not found, ignoring")
            return ctrl.Result{}, nil
        }
        log.Error(err, "Failed to get Database")
        return ctrl.Result{}, err
    }

    // 检查是否正在删除
    if database.GetDeletionTimestamp() != nil {
        if controllerutil.ContainsFinalizer(database, databaseFinalizer) {
            // 执行清理逻辑
            if err := r.finalizeDatabase(ctx, database); err != nil {
                return ctrl.Result{}, err
            }

            // 移除 finalizer
            controllerutil.RemoveFinalizer(database, databaseFinalizer)
            if err := r.Update(ctx, database); err != nil {
                return ctrl.Result{}, err
            }
        }
        return ctrl.Result{}, nil
    }

    // 添加 finalizer
    if !controllerutil.ContainsFinalizer(database, databaseFinalizer) {
        controllerutil.AddFinalizer(database, databaseFinalizer)
        if err := r.Update(ctx, database); err != nil {
            return ctrl.Result{}, err
        }
    }

    // 调和主要资源
    result, err := r.reconcileResources(ctx, database)
    if err != nil {
        // 更新失败状态
        r.updateStatus(ctx, database, PhaseFailed, err.Error())
        return result, err
    }

    // 更新状态
    if err := r.updateDatabaseStatus(ctx, database); err != nil {
        log.Error(err, "Failed to update Database status")
        return ctrl.Result{RequeueAfter: time.Second * 10}, err
    }

    // 定期重新调和 (检查健康状态)
    return ctrl.Result{RequeueAfter: time.Minute * 5}, nil
}

func (r *DatabaseReconciler) reconcileResources(ctx context.Context, database *dbv1.Database) (ctrl.Result, error) {
    log := r.Log.WithValues("database", database.Name)

    // 1. 调和 ConfigMap
    if err := r.reconcileConfigMap(ctx, database); err != nil {
        log.Error(err, "Failed to reconcile ConfigMap")
        return ctrl.Result{}, err
    }

    // 2. 调和 Secret (密码)
    if err := r.reconcileSecret(ctx, database); err != nil {
        log.Error(err, "Failed to reconcile Secret")
        return ctrl.Result{}, err
    }

    // 3. 调和 Service
    if err := r.reconcileService(ctx, database); err != nil {
        log.Error(err, "Failed to reconcile Service")
        return ctrl.Result{}, err
    }

    // 4. 调和 StatefulSet
    if err := r.reconcileStatefulSet(ctx, database); err != nil {
        log.Error(err, "Failed to reconcile StatefulSet")
        return ctrl.Result{}, err
    }

    // 5. 调和备份 CronJob (如果启用)
    if database.Spec.Backup != nil && database.Spec.Backup.Enabled {
        if err := r.reconcileBackupCronJob(ctx, database); err != nil {
            log.Error(err, "Failed to reconcile Backup CronJob")
            return ctrl.Result{}, err
        }
    }

    return ctrl.Result{}, nil
}

func (r *DatabaseReconciler) reconcileStatefulSet(ctx context.Context, database *dbv1.Database) error {
    sts := &appsv1.StatefulSet{}
    stsName := types.NamespacedName{Name: database.Name, Namespace: database.Namespace}
    
    err := r.Get(ctx, stsName, sts)
    if err != nil && errors.IsNotFound(err) {
        // 创建新的 StatefulSet
        sts = r.statefulSetForDatabase(database)
        if err := controllerutil.SetControllerReference(database, sts, r.Scheme); err != nil {
            return err
        }
        
        r.Log.Info("Creating StatefulSet", "name", sts.Name)
        return r.Create(ctx, sts)
    } else if err != nil {
        return err
    }

    // 检查是否需要更新
    if *sts.Spec.Replicas != database.Spec.Size {
        sts.Spec.Replicas = &database.Spec.Size
        r.Log.Info("Updating StatefulSet replicas", "name", sts.Name, "replicas", database.Spec.Size)
        return r.Update(ctx, sts)
    }

    return nil
}

func (r *DatabaseReconciler) statefulSetForDatabase(database *dbv1.Database) *appsv1.StatefulSet {
    labels := map[string]string{
        "app":        "database",
        "database":   database.Name,
        "controller": database.Name,
    }

    return &appsv1.StatefulSet{
        ObjectMeta: metav1.ObjectMeta{
            Name:      database.Name,
            Namespace: database.Namespace,
            Labels:    labels,
        },
        Spec: appsv1.StatefulSetSpec{
            ServiceName: database.Name,
            Replicas:    &database.Spec.Size,
            Selector: &metav1.LabelSelector{
                MatchLabels: labels,
            },
            Template: corev1.PodTemplateSpec{
                ObjectMeta: metav1.ObjectMeta{
                    Labels: labels,
                },
                Spec: corev1.PodSpec{
                    Containers: []corev1.Container{{
                        Name:  "postgres",
                        Image: fmt.Sprintf("postgres:%s", database.Spec.Version),
                        Ports: []corev1.ContainerPort{{
                            ContainerPort: 5432,
                            Name:          "postgres",
                        }},
                        EnvFrom: []corev1.EnvFromSource{{
                            SecretRef: &corev1.SecretEnvSource{
                                LocalObjectReference: corev1.LocalObjectReference{
                                    Name: fmt.Sprintf("%s-secret", database.Name),
                                },
                            },
                        }},
                        VolumeMounts: []corev1.VolumeMount{{
                            Name:      "data",
                            MountPath: "/var/lib/postgresql/data",
                        }},
                        LivenessProbe: &corev1.Probe{
                            ProbeHandler: corev1.ProbeHandler{
                                Exec: &corev1.ExecAction{
                                    Command: []string{"pg_isready", "-U", "postgres"},
                                },
                            },
                            InitialDelaySeconds: 30,
                            PeriodSeconds:       10,
                        },
                        ReadinessProbe: &corev1.Probe{
                            ProbeHandler: corev1.ProbeHandler{
                                Exec: &corev1.ExecAction{
                                    Command: []string{"pg_isready", "-U", "postgres"},
                                },
                            },
                            InitialDelaySeconds: 5,
                            PeriodSeconds:       5,
                        },
                    }},
                },
            },
            VolumeClaimTemplates: []corev1.PersistentVolumeClaim{{
                ObjectMeta: metav1.ObjectMeta{
                    Name: "data",
                },
                Spec: corev1.PersistentVolumeClaimSpec{
                    AccessModes: []corev1.PersistentVolumeAccessMode{corev1.ReadWriteOnce},
                    StorageClassName: &database.Spec.Storage.StorageClassName,
                    Resources: corev1.VolumeResourceRequirements{
                        Requests: corev1.ResourceList{
                            corev1.ResourceStorage: resource.MustParse(database.Spec.Storage.Size),
                        },
                    },
                },
            }},
        },
    }
}

func (r *DatabaseReconciler) updateDatabaseStatus(ctx context.Context, database *dbv1.Database) error {
    // 获取 StatefulSet 状态
    sts := &appsv1.StatefulSet{}
    if err := r.Get(ctx, types.NamespacedName{Name: database.Name, Namespace: database.Namespace}, sts); err != nil {
        if errors.IsNotFound(err) {
            database.Status.Phase = PhasePending
            database.Status.ReadyReplicas = 0
        } else {
            return err
        }
    } else {
        database.Status.ReadyReplicas = sts.Status.ReadyReplicas
        
        if sts.Status.ReadyReplicas == database.Spec.Size {
            database.Status.Phase = PhaseRunning
            database.Status.Endpoint = fmt.Sprintf("%s.%s.svc.cluster.local:5432", database.Name, database.Namespace)
            
            // 设置 Ready condition
            meta.SetStatusCondition(&database.Status.Conditions, metav1.Condition{
                Type:    "Ready",
                Status:  metav1.ConditionTrue,
                Reason:  "DatabaseReady",
                Message: "All database replicas are ready",
            })
        } else {
            database.Status.Phase = PhaseCreating
            meta.SetStatusCondition(&database.Status.Conditions, metav1.Condition{
                Type:    "Ready",
                Status:  metav1.ConditionFalse,
                Reason:  "ReplicasNotReady",
                Message: fmt.Sprintf("%d/%d replicas ready", sts.Status.ReadyReplicas, database.Spec.Size),
            })
        }
    }

    return r.Status().Update(ctx, database)
}

func (r *DatabaseReconciler) finalizeDatabase(ctx context.Context, database *dbv1.Database) error {
    r.Log.Info("Finalizing Database", "name", database.Name)
    
    // 可以在这里执行清理逻辑
    // 例如: 创建最终备份、清理外部资源等
    
    return nil
}

func (r *DatabaseReconciler) SetupWithManager(mgr ctrl.Manager) error {
    return ctrl.NewControllerManagedBy(mgr).
        For(&dbv1.Database{}).
        Owns(&appsv1.StatefulSet{}).
        Owns(&corev1.Service{}).
        Owns(&corev1.Secret{}).
        Owns(&corev1.ConfigMap{}).
        Complete(r)
}
```

## ArgoCD GitOps 集成

### 1. ArgoCD Application 定义

```yaml
# argocd-application.yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: myapp
  namespace: argocd
  finalizers:
    - resources-finalizer.argocd.argoproj.io
  annotations:
    # 通知配置
    notifications.argoproj.io/subscribe.on-sync-succeeded.slack: deployments
    notifications.argoproj.io/subscribe.on-sync-failed.slack: alerts
spec:
  project: default
  
  source:
    repoURL: https://github.com/myorg/myapp-config.git
    targetRevision: main
    path: overlays/prod
    
    # 如果使用 Helm
    # helm:
    #   valueFiles:
    #     - values-base.yaml
    #     - values-prod.yaml
    #   parameters:
    #     - name: image.tag
    #       value: v1.2.3
    
    # 如果使用 Kustomize
    kustomize:
      images:
        - myapp=myregistry.example.com/myapp:v1.2.3
      commonLabels:
        managed-by: argocd
  
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  
  syncPolicy:
    automated:
      prune: true           # 删除不在 Git 中的资源
      selfHeal: true        # 自动修复漂移
      allowEmpty: false     # 不允许删除所有资源
    syncOptions:
      - CreateNamespace=true
      - PrunePropagationPolicy=foreground
      - PruneLast=true
      - ApplyOutOfSyncOnly=true
      - ServerSideApply=true
    retry:
      limit: 5
      backoff:
        duration: 5s
        factor: 2
        maxDuration: 3m
  
  # 忽略特定字段的差异
  ignoreDifferences:
    - group: apps
      kind: Deployment
      jsonPointers:
        - /spec/replicas  # 忽略 HPA 管理的副本数
    - group: autoscaling
      kind: HorizontalPodAutoscaler
      jqPathExpressions:
        - .status
  
  # 健康检查
  revisionHistoryLimit: 10
```

```yaml
# argocd-applicationset.yaml - 多集群/多环境
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: myapp
  namespace: argocd
spec:
  generators:
    # 基于 Git 目录生成
    - git:
        repoURL: https://github.com/myorg/myapp-config.git
        revision: HEAD
        directories:
          - path: overlays/*
    
    # 或基于集群列表
    # - clusters:
    #     selector:
    #       matchLabels:
    #         env: production
  
  template:
    metadata:
      name: 'myapp-{{path.basename}}'
    spec:
      project: default
      source:
        repoURL: https://github.com/myorg/myapp-config.git
        targetRevision: HEAD
        path: '{{path}}'
      destination:
        server: '{{server}}'
        namespace: '{{path.basename}}'
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
          - CreateNamespace=true
```

## 故障排查指南

### 包管理常见问题

| 问题 (Issue) | 诊断 (Diagnosis) | 解决方案 (Solution) |
|-------------|-----------------|-------------------|
| Helm 安装超时 | `helm status <release>` | 检查 Pod 事件: `kubectl describe pod` |
| Chart 依赖缺失 | `helm dependency list` | `helm dependency update ./chart` |
| Values 未生效 | `helm get values <release>` | 检查 values 文件路径和格式 |
| 模板渲染错误 | `helm template --debug` | 检查 Go 模板语法 |
| 回滚失败 | `helm history <release>` | 检查历史版本是否存在 |
| Kustomize 构建失败 | `kustomize build --enable-alpha-plugins` | 验证 YAML 语法和引用路径 |
| Operator 无法调谐 | 查看 Operator 日志 | 检查 RBAC 权限和 CRD 版本 |
| ArgoCD 同步失败 | ArgoCD UI 查看详情 | 检查 Git 凭证和仓库访问 |

### 诊断脚本

```bash
#!/bin/bash
# package-troubleshoot.sh - 包管理诊断脚本

RELEASE_NAME=$1
NAMESPACE=${2:-default}

echo "=== Helm Release 状态 ==="
helm status $RELEASE_NAME -n $NAMESPACE

echo ""
echo "=== Release 历史 ==="
helm history $RELEASE_NAME -n $NAMESPACE

echo ""
echo "=== Release Manifest ==="
helm get manifest $RELEASE_NAME -n $NAMESPACE | head -100

echo ""
echo "=== 相关 Pod 状态 ==="
kubectl get pods -n $NAMESPACE -l app.kubernetes.io/instance=$RELEASE_NAME

echo ""
echo "=== 异常 Pod 事件 ==="
for pod in $(kubectl get pods -n $NAMESPACE -l app.kubernetes.io/instance=$RELEASE_NAME -o name); do
    echo "--- $pod ---"
    kubectl describe $pod -n $NAMESPACE | grep -A 20 "Events:"
done

echo ""
echo "=== Kustomize 构建测试 ==="
if [ -d "overlays/$NAMESPACE" ]; then
    kustomize build overlays/$NAMESPACE 2>&1 | head -50
fi
```

## 速查表

### Helm 命令速查

```bash
# 仓库管理
helm repo add NAME URL          # 添加仓库
helm repo update                # 更新索引
helm search repo KEYWORD        # 搜索 Chart

# Release 管理
helm install NAME CHART         # 安装
helm upgrade NAME CHART         # 升级
helm upgrade --install NAME     # 安装或升级
helm rollback NAME REVISION     # 回滚
helm uninstall NAME             # 卸载

# 调试
helm template NAME CHART        # 渲染模板
helm lint CHART                 # 语法检查
helm diff upgrade NAME CHART    # 对比差异

# 状态查看
helm list                       # 列出 Release
helm status NAME                # 查看状态
helm history NAME               # 查看历史
helm get values NAME            # 获取 Values
```

### Kustomize 命令速查

```bash
# 构建
kustomize build DIR             # 构建 manifests
kubectl apply -k DIR            # 构建并应用
kubectl diff -k DIR             # 对比差异

# 编辑
kustomize edit set image        # 设置镜像
kustomize edit add resource     # 添加资源
kustomize edit set namespace    # 设置命名空间
```

## 最佳实践清单

### Helm 最佳实践

- [ ] **使用 OCI 仓库**: 优先使用 OCI 仓库存储 Chart
- [ ] **锁定依赖版本**: 使用 Chart.lock 锁定依赖
- [ ] **Values 分层**: 基础配置 + 环境覆盖
- [ ] **启用 --atomic**: 失败时自动回滚
- [ ] **使用 helm-diff**: 升级前预览变更
- [ ] **配置 Checksum**: 配置变更触发重启

### Kustomize 最佳实践

- [ ] **组件化**: 使用 components 复用配置
- [ ] **避免 patch 冲突**: 使用 Strategic Merge Patch
- [ ] **验证输出**: 使用 kubeconform 验证
- [ ] **镜像标签**: 使用 images 字段统一管理
- [ ] **Secret 管理**: 结合 SOPS 加密敏感数据

### Operator 最佳实践

- [ ] **Finalizer**: 正确处理资源清理
- [ ] **Status 更新**: 实时反映资源状态
- [ ] **重试策略**: 合理设置重试间隔
- [ ] **健康检查**: 定期重新调和
- [ ] **RBAC 最小权限**: 只申请必要权限

### GitOps 最佳实践

- [ ] **声明式配置**: 所有配置存储在 Git
- [ ] **自动同步**: 启用 automated sync
- [ ] **漂移检测**: 启用 selfHeal
- [ ] **多环境分离**: 使用 ApplicationSet
- [ ] **通知集成**: 配置同步状态通知

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
