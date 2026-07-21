---
title: Helm Advanced Patterns — Dependencies, Plugins, OCI Registry, and Testing
description: Helm 高级模式 — Chart 依赖管理、OCI Registry 发布、Helm 插件开发、单元测试、多环境管理、Secret 加密
summary: Helm 生产级高级模式，涵盖 Chart 生命周期管理、测试、安全与多环境策略
category: practice
tags:
- helm
- chart
- oci-registry
- testing
- multi-environment
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: manifest
---
# Helm 高级模式 — 依赖、OCI、测试与多环境

> Helm 生产级高级模式，从 Chart 开发到企业级发布管理。

## Chart 依赖管理

### 子 Chart 依赖

```yaml
# Chart.yaml
apiVersion: v2
name: my-platform
version: 1.0.0
dependencies:
  - name: postgresql
    version: "15.x.x"
    repository: "https://charts.bitnami.com/bitnami"
    condition: postgresql.enabled
  - name: redis
    version: "19.x.x"
    repository: "https://charts.bitnami.com/bitnami"
    condition: redis.enabled
  - name: common
    version: "2.x.x"
    repository: "oci://registry.example.com/charts"
    # 内部 Library Chart
```

```yaml
# values.yaml — 条件依赖
postgresql:
  enabled: true
  auth:
    database: myapp
    existingSecret: postgres-credentials
  primary:
    persistence:
      size: 50Gi
      storageClass: gp3-encrypted

redis:
  enabled: false  # 使用外部 Redis
```

### Library Chart（可复用模板）

```yaml
# charts/common/Chart.yaml
apiVersion: v2
name: common
version: 1.0.0
type: library  # 不可直接安装
```

```yaml
# charts/common/templates/_deployment.yaml
{{- define "common.deployment" -}}
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "common.fullname" . }}
  labels:
    {{- include "common.labels" . | nindent 4 }}
spec:
  replicas: {{ .Values.replicaCount | default 2 }}
  selector:
    matchLabels:
      {{- include "common.selectorLabels" . | nindent 6 }}
  template:
    metadata:
      labels:
        {{- include "common.selectorLabels" . | nindent 8 }}
    spec:
      securityContext:
        runAsNonRoot: true
        seccompProfile:
          type: RuntimeDefault
      containers:
        - name: {{ .Chart.Name }}
          image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
          securityContext:
            allowPrivilegeEscalation: false
            readOnlyRootFilesystem: true
            capabilities:
              drop: ["ALL"]
          resources:
            {{- toYaml .Values.resources | nindent 12 }}
{{- end -}}
```

## OCI Registry 发布

### 推送 Chart 到 OCI Registry

```bash
# 登录 OCI Registry
helm registry login registry.example.com -u admin -p $REGISTRY_PASSWORD

# 打包并推送
helm package ./my-chart
helm push my-chart-1.0.0.tgz oci://registry.example.com/charts

# 从 OCI 安装
helm install my-release oci://registry.example.com/charts/my-chart --version 1.0.0

# Harbor 作为 Chart Museum（推荐）
helm push my-chart-1.0.0.tgz oci://harbor.example.com/myproject/charts
```

### CI/CD 自动发布

```yaml
# GitHub Actions — Chart 发布
name: Publish Helm Chart
on:
  push:
    tags: ['v*']
jobs:
  publish:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Lint
        run: helm lint ./chart
      - name: Test
        run: |
          helm unittest ./chart
      - name: Package
        run: helm package ./chart --version ${GITHUB_REF_NAME#v}
      - name: Login to Registry
        run: helm registry login ${{ secrets.REGISTRY_URL }} -u ${{ secrets.REGISTRY_USER }} -p ${{ secrets.REGISTRY_PASS }}
      - name: Push
        run: helm push chart-${GITHUB_REF_NAME#v}.tgz oci://${{ secrets.REGISTRY_URL }}/charts
```

## Helm 单元测试（helm-unittest）

### 安装与配置

```bash
# 安装插件
helm plugin install https://github.com/helm-unittest/helm-unittest.git
```

### 测试用例

```yaml
# tests/deployment_test.yaml
suite: Deployment Tests
templates:
  - deployment.yaml
tests:
  - it: should set correct replicas
    set:
      replicaCount: 5
    asserts:
      - equal:
          path: spec.replicas
          value: 5

  - it: should use distroless image
    set:
      image:
        repository: registry.example.com/app
        tag: v1.0.0
    asserts:
      - equal:
          path: spec.template.spec.containers[0].image
          value: "registry.example.com/app:v1.0.0"

  - it: should enforce security context
    asserts:
      - equal:
          path: spec.template.spec.securityContext.runAsNonRoot
          value: true
      - equal:
          path: spec.template.spec.containers[0].securityContext.readOnlyRootFilesystem
          value: true

  - it: should set resource limits
    set:
      resources:
        requests:
          cpu: 100m
          memory: 128Mi
        limits:
          cpu: "1"
          memory: 512Mi
    asserts:
      - equal:
          path: spec.template.spec.containers[0].resources.limits.memory
          value: 512Mi

  - it: should not allow privileged containers
    asserts:
      - notExists:
          path: spec.template.spec.containers[0].securityContext.privileged

  - it: should render HPA when autoscaling enabled
    template: hpa.yaml
    set:
      autoscaling:
        enabled: true
        minReplicas: 2
        maxReplicas: 10
    asserts:
      - hasDocuments:
          count: 1
      - equal:
          path: spec.minReplicas
          value: 2
```

```bash
# 运行测试
helm unittest ./my-chart
helm unittest --with-subchart ./my-chart
helm unittest -f 'tests/**/*_test.yaml' ./my-chart
```

## 多环境管理

### 环境覆盖策略

```
chart/
├── Chart.yaml
├── values.yaml              # 默认值
├── values-dev.yaml          # 开发环境
├── values-staging.yaml      # 预发环境
├── values-production.yaml   # 生产环境
└── values-production-dr.yaml # DR 环境
```

```yaml
# values-production.yaml
replicaCount: 5
image:
  tag: "v2.1.0"  # 固定版本，不用 latest
resources:
  requests:
    cpu: 500m
    memory: 512Mi
  limits:
    cpu: "2"
    memory: 2Gi
autoscaling:
  enabled: true
  minReplicas: 3
  maxReplicas: 30
podDisruptionBudget:
  enabled: true
  minAvailable: 2
networkPolicy:
  enabled: true
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              name: ingress-nginx
```

```bash
# 部署命令
helm upgrade --install my-app ./chart \
  -f values.yaml \
  -f values-production.yaml \
  --namespace production \
  --atomic \
  --timeout 10m \
  --wait
```

### Helmfile（多 Release 编排）

```yaml
# helmfile.yaml
environments:
  production:
    values:
      - env/production.yaml
  staging:
    values:
      - env/staging.yaml
---
repositories:
  - name: bitnami
    url: https://charts.bitnami.com/bitnami
  - name: internal
    url: oci://registry.example.com/charts

releases:
  - name: postgresql
    namespace: database
    chart: bitnami/postgresql
    version: "15.5.0"
    values:
      - values/postgresql/{{ .Environment.Name }}.yaml
    installed: {{ .Values | get "postgresql.enabled" true }}

  - name: redis
    namespace: database
    chart: bitnami/redis
    version: "19.3.0"
    values:
      - values/redis/{{ .Environment.Name }}.yaml

  - name: my-app
    namespace: production
    chart: internal/my-app
    version: "{{ .Values | get "appVersion" "1.0.0" }}"
    values:
      - values/my-app/{{ .Environment.Name }}.yaml
    hooks:
      - events: ["presync"]
        command: kubectl
        args: ["apply", "-f", "crds/"]
      - events: ["postsync"]
        command: ./scripts/smoke-test.sh
        args: ["{{ .Environment.Name }}"]
```

## Secret 加密（helm-secrets / SOPS）

```bash
# 安装 helm-secrets
helm plugin install https://github.com/jkroepke/helm-secrets

# 使用 SOPS + age 加密
sops --encrypt --age age1ql3z7hjy54pw3hyww5ayyfg7zqgvc7w3j2elw8zmrj2kg5sfn9aqmcac8p \
  values-production-secrets.yaml > values-production-secrets.enc.yaml

# 部署时自动解密
helm secrets upgrade --install my-app ./chart \
  -f values-production.yaml \
  -f values-production-secrets.enc.yaml \
  --namespace production
```

## 最佳实践

| 实践 | 说明 |
|------|------|
| 固定 Chart 版本 | 不用 `latest` 或 `*` |
| OCI Registry | 替代已废弃的 ChartMuseum |
| helm unittest | CI 中强制测试 |
| `--atomic` | 失败自动回滚 |
| `--wait` | 等待资源就绪 |
| Library Chart | 复用通用模板 |
| helm-secrets | Secret 加密存储 |
| helmfile | 多 Release 编排 |
| NOTES.txt | 安装后提示 |
| .helmignore | 排除不必要文件 |

## Related

- [[清单模式/Helm值模式/index.md|Helm 值模式]]
- [[清单模式/Helm值模式/03-helm-library-charts-reuse.md|Library Charts]]
- [[发布变更/GitOps/index.md|GitOps]]
