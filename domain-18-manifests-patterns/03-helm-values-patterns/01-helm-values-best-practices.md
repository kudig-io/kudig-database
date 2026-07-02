---
title: Helm Values 最佳实践
description: '多环境 values 文件组织、Secret 值管理与 values 校验实战指南'
summary: '多环境 values 文件组织、Secret 值管理与 values 校验实战指南'
category: manifests-patterns
tags:
- helm
- values
- sealed-secrets
- external-secrets
- json-schema
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Helm Values 最佳实践是什么
- 如何管理多环境 Helm Values
- Helm Secret 值如何管理
trigger_keywords:
- helm
- values
- sealed-secrets
- external-secrets
- json-schema
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Helm Values 最佳实践

## 1. 多环境 Values 文件组织

### 1.1 推荐目录结构

```
my-chart/
├── Chart.yaml
├── Chart.lock
├── values.yaml              # 默认值（base）
├── values/
│   ├── dev.yaml
│   ├── staging.yaml
│   └── prod.yaml
├── templates/
│   ├── deployment.yaml
│   ├── service.yaml
│   └── _helpers.tpl
├── tests/
│   └── test-connection.yaml
└── ci/
    ├── dev-values.yaml
    ├── staging-values.yaml
    └── prod-values.yaml
```

### 1.2 values.yaml（默认值）

```yaml
# values.yaml - 默认配置
replicaCount: 1

image:
  repository: my-app
  pullPolicy: IfNotPresent
  tag: ""

service:
  type: ClusterIP
  port: 80

ingress:
  enabled: false
  className: nginx
  annotations: {}
  hosts:
    - host: chart-example.local
      paths:
        - path: /
          pathType: ImplementationSpecific
  tls: []

resources:
  limits:
    cpu: 500m
    memory: 128Mi
  requests:
    cpu: 100m
    memory: 128Mi

autoscaling:
  enabled: false
  minReplicas: 1
  maxReplicas: 10
  targetCPUUtilizationPercentage: 80

nodeSelector: {}
tolerations: []
affinity: {}

# 数据库配置
database:
  host: localhost
  port: 5432
  name: mydb
  # credentials 通过 Secret 管理
  existingSecret: ""

# Redis 配置
redis:
  host: localhost
  port: 6379
  existingSecret: ""
```

### 1.3 Dev Values

```yaml
# values/dev.yaml
replicaCount: 1

image:
  tag: dev-latest

ingress:
  enabled: true
  hosts:
    - host: my-app.dev.example.com
      paths:
        - path: /
          pathType: Prefix

resources:
  limits:
    cpu: 200m
    memory: 256Mi
  requests:
    cpu: 50m
    memory: 128Mi

autoscaling:
  enabled: false

database:
  host: postgres-dev.database.svc.cluster.local
  existingSecret: my-app-db-dev

redis:
  host: redis-dev.database.svc.cluster.local
  existingSecret: my-app-redis-dev

# 开发环境特有配置
debug: true
logLevel: debug
```

### 1.4 Staging Values

```yaml
# values/staging.yaml
replicaCount: 2

image:
  tag: v1.0.0-rc.1

ingress:
  enabled: true
  annotations:
    cert-manager.io/cluster-issuer: letsencrypt-staging
  hosts:
    - host: my-app.staging.example.com
      paths:
        - path: /
          pathType: Prefix
  tls:
    - secretName: my-app-staging-tls
      hosts:
        - my-app.staging.example.com

resources:
  limits:
    cpu: "1"
    memory: 512Mi
  requests:
    cpu: 200m
    memory: 256Mi

autoscaling:
  enabled: true
  minReplicas: 2
  maxReplicas: 5
  targetCPUUtilizationPercentage: 70

database:
  host: postgres-staging.database.svc.cluster.local
  existingSecret: my-app-db-staging

redis:
  host: redis-staging.database.svc.cluster.local
  existingSecret: my-app-redis-staging

logLevel: info
```

### 1.5 Prod Values

```yaml
# values/prod.yaml
replicaCount: 3

image:
  tag: v1.0.0
  pullPolicy: Always

ingress:
  enabled: true
  annotations:
    cert-manager.io/cluster-issuer: letsencrypt-prod
    nginx.ingress.kubernetes.io/rate-limit: "100"
    nginx.ingress.kubernetes.io/rate-limit-window: "1m"
  hosts:
    - host: my-app.example.com
      paths:
        - path: /
          pathType: Prefix
  tls:
    - secretName: my-app-prod-tls
      hosts:
        - my-app.example.com

resources:
  limits:
    cpu: "2"
    memory: 1Gi
  requests:
    cpu: 500m
    memory: 512Mi

autoscaling:
  enabled: true
  minReplicas: 3
  maxReplicas: 50
  targetCPUUtilizationPercentage: 60

database:
  host: postgres-prod.database.svc.cluster.local
  existingSecret: my-app-db-prod

redis:
  host: redis-prod.database.svc.cluster.local
  existingSecret: my-app-redis-prod

# 生产环境配置
podDisruptionBudget:
  enabled: true
  minAvailable: 2

nodeSelector:
  kubernetes.io/os: linux

tolerations:
  - key: "dedicated"
    operator: "Equal"
    value: "application"
    effect: "NoSchedule"

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
                  - my-app
          topologyKey: kubernetes.io/hostname

logLevel: warn
```

### 1.6 多环境安装命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 开发环境
helm install my-app ./my-chart \
  -f values.yaml \
  -f values/dev.yaml \
  --namespace dev

# 预发布环境
helm install my-app ./my-chart \
  -f values.yaml \
  -f values/staging.yaml \
  --namespace staging

# 生产环境
helm install my-app ./my-chart \
  -f values.yaml \
  -f values/prod.yaml \
  --namespace production

# 覆盖特定值
helm install my-app ./my-chart \
  -f values.yaml \
  -f values/prod.yaml \
  --set image.tag=v1.2.3 \
  --set replicaCount=5
```
## 2. Secret 值管理

### 2.1 External Secrets Operator

```yaml
# 安装 External Secrets
helm repo add external-secrets https://charts.external-secrets.io
helm install external-secrets external-secrets/external-secrets \
  -n external-secrets \
  --create-namespace
```

```yaml
# SecretStore 配置（AWS Secrets Manager）
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: aws-secrets-manager
  namespace: production
spec:
  provider:
    aws:
      service: SecretsManager
      region: us-east-1
      auth:
        jwt:
          serviceAccountRef:
            name: external-secrets-sa
---
# ExternalSecret 定义
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: my-app-db-creds
  namespace: production
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: aws-secrets-manager
    kind: SecretStore
  target:
    name: my-app-db-prod
    creationPolicy: Owner
  data:
    - secretKey: username
      remoteRef:
        key: prod/my-app/database
        property: username
    - secretKey: password
      remoteRef:
        key: prod/my-app/database
        property: password
```

### 2.2 Sealed Secrets

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 Sealed Secrets
helm repo add sealed-secrets https://bitnami-labs.github.io/sealed-secrets
helm install sealed-secrets sealed-secrets/sealed-secrets \
  -n kube-system

# 安装 kubeseal CLI
kubeseal --version
```
```yaml
# 创建 SealedSecret
# 原始 Secret
apiVersion: v1
kind: Secret
metadata:
  name: my-app-db-prod
  namespace: production
type: Opaque
stringData:
  username: admin
  password: P@ssw0rd123
```

```bash
# 加密为 SealedSecret
kubeseal --format yaml \
  --controller-namespace kube-system \
  --controller-name sealed-secrets \
  < secret.yaml > sealed-secret.yaml
```

```yaml
# 生成的 SealedSecret（安全提交到 Git）
apiVersion: bitnami.com/v1alpha1
kind: SealedSecret
metadata:
  name: my-app-db-prod
  namespace: production
spec:
  encryptedData:
    password: AgBY3...加密数据...
    username: AgABC...加密数据...
  template:
    metadata:
      name: my-app-db-prod
      namespace: production
    type: Opaque
```

### 2.3 Helm Secrets 插件

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 helm-secrets 插件
helm plugin install https://github.com/jkroepke/helm-secrets

# 使用 sops 加密
sops --encrypt --in-place secrets.yaml
```
```yaml
# secrets.yaml（加密前）
database:
  password: P@ssw0rd123
api_key: sk-abc123
```

```yaml
# secrets.yaml（加密后，提交到 Git）
database:
  password: ENC[AES256_GCM,data:xyz...,type:str]
api_key: ENC[AES256_GCM,data:abc...,type:str]
sops:
  kms:
    - arn: aws:kms:us-east-1:123456789:key/abc-123
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用加密 values 安装
helm secrets install my-app ./my-chart \
  -f values.yaml \
  -f values/prod.yaml \
  -f secrets.yaml
```
### 2.4 Secret 管理最佳实践

```yaml
# 推荐：使用 existingSecret 引用
database:
  existingSecret: my-app-db-prod
  existingSecretKeys:
    username: username
    password: password

# 不推荐：直接在 values 中存储密码
database:
  username: admin
  password: P@ssw0rd123    # 不要这样做
```

## 3. Values 校验（JSON Schema）

### 3.1 基础 Schema

```json
{
  "$schema": "https://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["replicaCount", "image"],
  "properties": {
    "replicaCount": {
      "type": "integer",
      "minimum": 1,
      "description": "副本数量"
    },
    "image": {
      "type": "object",
      "required": ["repository"],
      "properties": {
        "repository": {
          "type": "string",
          "pattern": "^[a-z0-9-]+(/[a-z0-9-]+)*$"
        },
        "tag": {
          "type": "string"
        },
        "pullPolicy": {
          "type": "string",
          "enum": ["Always", "IfNotPresent", "Never"]
        }
      }
    },
    "service": {
      "type": "object",
      "properties": {
        "type": {
          "type": "string",
          "enum": ["ClusterIP", "NodePort", "LoadBalancer"]
        },
        "port": {
          "type": "integer",
          "minimum": 1,
          "maximum": 65535
        }
      }
    }
  }
}
```

### 3.2 高级 Schema（带条件验证）

```json
{
  "$schema": "https://json-schema.org/draft-07/schema#",
  "type": "object",
  "properties": {
    "autoscaling": {
      "type": "object",
      "properties": {
        "enabled": { "type": "boolean" },
        "minReplicas": { "type": "integer", "minimum": 1 },
        "maxReplicas": { "type": "integer", "minimum": 1 }
      },
      "if": {
        "properties": { "enabled": { "const": true } }
      },
      "then": {
        "required": ["minReplicas", "maxReplicas"]
      }
    },
    "ingress": {
      "type": "object",
      "properties": {
        "enabled": { "type": "boolean" },
        "tls": {
          "type": "array",
          "items": {
            "type": "object",
            "required": ["secretName", "hosts"]
          }
        }
      },
      "if": {
        "properties": { "enabled": { "const": true } }
      },
      "then": {
        "required": ["hosts"]
      }
    }
  }
}
```

### 3.3 Schema 文件位置

```
# 🟢 低风险：只读/信息收集，通常无副作用
my-chart/
├── Chart.yaml
├── values.yaml
├── values.schema.json    # Helm 3+ 自动识别
└── templates/
```
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证 values
helm lint ./my-chart -f values/prod.yaml

# 模板渲染（包含 schema 校验）
helm template my-app ./my-chart -f values/prod.yaml
```
### 3.4 CI/CD 中的 Schema 校验

```yaml
# GitHub Actions
name: Helm Chart Validation
on:
  pull_request:
    paths:
      - "charts/**"

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Validate Chart
        run: |
          helm lint ./charts/my-chart
          helm template test ./charts/my-chart -f ./charts/my-chart/values/dev.yaml
          helm template test ./charts/my-chart -f ./charts/my-chart/values/staging.yaml
          helm template test ./charts/my-chart -f ./charts/my-chart/values/prod.yaml

      - name: Validate Schema
        run: |
          for env in dev staging prod; do
            echo "Validating $env..."
            helm template test ./charts/my-chart \
              -f ./charts/my-chart/values/$env.yaml \
              --dry-run
          done
```

## 4. Values 文件组织最佳实践

### 4.1 分层 Values

```yaml
# values.yaml（默认值）
# values/base.yaml（基础配置，可选）
# values/<env>.yaml（环境特定）
# values/<env>/<region>.yaml（区域特定，可选）
```

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 多层覆盖
helm install my-app ./chart \
  -f values.yaml \
  -f values/prod.yaml \
  -f values/prod/us-east-1.yaml \
  --namespace production
```
### 4.2 Values 命名规范

```yaml
# 好的命名
database:
  host: localhost
  port: 5432
  credentials:
    existingSecret: ""

# 不好的命名
db_host: localhost
db_port: 5432
db_secret: ""
```

### 4.3 敏感值处理

```yaml
# 推荐模式
database:
  host: localhost
  port: 5432
  existingSecret: "my-app-db"    # 引用 Secret
  existingSecretKeys:
    username: DB_USERNAME
    password: DB_PASSWORD

# 避免模式
database:
  host: localhost
  port: 5432
  username: admin
  password: P@ssw0rd    # 明文
```

## 5. Values 与模板集成

### 5.1 在模板中使用 Values

```yaml
# templates/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "myapp.fullname" . }}
  labels:
    {{- include "myapp.labels" . | nindent 4 }}
spec:
  replicas: {{ .Values.replicaCount }}
  selector:
    matchLabels:
      {{- include "myapp.selectorLabels" . | nindent 6 }}
  template:
    metadata:
      labels:
        {{- include "myapp.selectorLabels" . | nindent 8 }}
    spec:
      containers:
        - name: {{ .Chart.Name }}
          image: "{{ .Values.image.repository }}:{{ .Values.image.tag | default .Chart.AppVersion }}"
          ports:
            - containerPort: {{ .Values.service.port }}
          env:
            - name: DB_HOST
              value: {{ .Values.database.host | quote }}
            - name: DB_USERNAME
              valueFrom:
                secretKeyRef:
                  name: {{ .Values.database.existingSecret }}
                  key: username
```

### 5.2 _helpers.tpl 辅助函数

```yaml
# templates/_helpers.tpl
{{/*
生成完整应用名称
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
通用标签
*/}}
{{- define "myapp.labels" -}}
helm.sh/chart: {{ .Chart.Name }}-{{ .Chart.Version }}
app.kubernetes.io/name: {{ .Chart.Name }}
app.kubernetes.io/instance: {{ .Release.Name }}
app.kubernetes.io/version: {{ .Chart.AppVersion }}
app.kubernetes.io/managed-by: {{ .Release.Service }}
{{- end }}

{{/*
选择器标签
*/}}
{{- define "myapp.selectorLabels" -}}
app.kubernetes.io/name: {{ .Chart.Name }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}
```

## 6. 常见问题排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 问题 1：values 文件优先级混乱
# 查看合并后的 values
helm get values my-app -n production

# 问题 2：Secret 未找到
# 检查 Secret 是否存在
kubectl get secret my-app-db-prod -n production

# 问题 3：Schema 校验失败
# 查看详细错误
helm template my-app ./chart -f values/prod.yaml --debug

# 问题 4：values 文件语法错误
# 验证 YAML 语法
yamllint values/prod.yaml
```
---

## Related

- [[Helm Hooks 生命周期|02-helm-hooks-lifecycle]]
- [[Helm Library Chart 复用模式|03-helm-library-charts-reuse]]

## See Also

- [Helm Values 文档](https://helm.sh/docs/chart_template_guide/values_files/)
- [External Secrets](https://external-secrets.io/)
- [Sealed Secrets](https://github.com/bitnami-labs/sealed-secrets)


<!-- risk-assessed -->
