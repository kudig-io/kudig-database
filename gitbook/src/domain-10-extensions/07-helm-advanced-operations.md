# 129 - Helm 高级运维：复杂部署、CI/CD 集成与安全最佳实践

## Helm 高级配置管理

### 多环境配置管理

```bash
# 环境特定值文件
values.yaml              # 默认配置
values-dev.yaml          # 开发环境配置
values-staging.yaml      # 预发布环境配置
values-prod.yaml         # 生产环境配置

# 使用特定环境配置安装
helm install myapp . -f values-prod.yaml
helm upgrade myapp . -f values-staging.yaml

# 覆盖多个值文件
helm install myapp . -f values.yaml -f values-prod.yaml
```

### 高级值引用

| 模板语法 | 说明 | 示例 |
|----------|------|------|
| `{{ .Values.key }}` | 访问值 | `{{ .Values.image.repository }}` |
| `{{ .Release.Name }}` | 发布名称 | `{{ .Release.Name }}-web` |
| `{{ .Release.Namespace }}` | 命名空间 | `{{ .Release.Namespace }}` |
| `{{ .Chart.Version }}` | Chart版本 | `{{ .Chart.Version }}` |
| `{{ .Capabilities.KubeVersion }}` | K8s版本 | `{{ .Capabilities.KubeVersion.Major }}` |
| `{{ .Files.Get "filename" }}` | 读取文件内容 | `{{ .Files.Get "config.txt" }}` |
| `{{ .Template.Name }}` | 模板名称 | `{{ .Template.Name }}` |

### 条件渲染

```yaml
# templates/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "myapp.fullname" . }}
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
        image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
        ports:
        - containerPort: {{ .Values.service.port }}
        {{- if .Values.resources }}
        resources:
          {{- toYaml .Values.resources | nindent 10 }}
        {{- end }}
        {{- if .Values.service.securityContext }}
        securityContext:
          {{- toYaml .Values.service.securityContext | nindent 10 }}
        {{- end }}
```

## 高级 Chart 开发技巧

### 模板辅助函数

```yaml
# templates/_helpers.tpl
{{/*
Common labels
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
Selector labels
*/}}
{{- define "myapp.selectorLabels" -}}
app.kubernetes.io/name: {{ include "myapp.name" . }}
app.kubernetes.io/instance: {{ .Release.Name }}
{{- end }}

{{/*
Create chart name and version as used by the chart label.
*/}}
{{- define "myapp.chart" -}}
{{- printf "%s-%s" .Chart.Name .Chart.Version | replace "+" "_" | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Expand the name of the chart.
*/}}
{{- define "myapp.name" -}}
{{- default .Chart.Name .Values.nameOverride | trunc 63 | trimSuffix "-" }}
{{- end }}

{{/*
Create a default fully qualified app name.
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
```

### 高级模板函数使用

```yaml
# 数组和字典操作
{{- $ports := list 80 443 8080 }}
{{- range $ports }}
- port: {{ . }}
{{- end }}

# 字典合并
{{- $defaults := dict "replicas" 1 "image" "nginx" }}
{{- $override := dict "replicas" 3 }}
{{- $config := merge $defaults $override }}
replicas: {{ $config.replicas }}

# 条件渲染
{{- if and .Values.enabled .Values.secret }}
{{- if or (.Values.env.isDev) (.Values.env.debug) }}
{{- if eq (.Values.environment) "production" }}

# 流程控制
{{- range .Values.items }}
{{- if gt (len .name) 5 }}
name: {{ .name }}
{{- end }}
{{- end }}
```

## CI/CD 集成最佳实践

### GitHub Actions 集成

```yaml
# .github/workflows/helm-release.yml
name: Helm Release

on:
  push:
    branches: [main]
    tags: ['v*']
  pull_request:
    branches: [main]

jobs:
  lint-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Install Helm
        uses: azure/setup-helm@v3
        with:
          version: v3.10.0
      
      - name: Set up chart-testing
        uses: helm/chart-testing-action@v2.6.1
      
      - name: Run chart-testing (list-changed)
        id: list-changed
        run: |
          changed=$(ct list-changed --config ct.yaml)
          if [[ -n "$changed" ]]; then
            echo "changed=true" >> "$GITHUB_OUTPUT"
          fi
      
      - name: Run chart-testing (lint)
        run: ct lint --config ct.yaml
        if: steps.list-changed.outputs.changed == 'true'
      
      - name: Create kind cluster
        uses: helm/kind-action@v1.8.0
        if: steps.list-changed.outputs.changed == 'true'
      
      - name: Run chart-testing (install)
        run: ct install --config ct.yaml
        if: steps.list-changed.outputs.changed == 'true'

  release:
    runs-on: ubuntu-latest
    needs: lint-test
    if: startsWith(github.ref, 'refs/tags/')
    steps:
      - name: Checkout
        uses: actions/checkout@v3
        with:
          fetch-depth: 0
      
      - name: Configure Git
        run: |
          git config user.name "$GITHUB_ACTOR"
          git config user.email "$GITHUB_ACTOR@users.noreply.github.com"
      
      - name: Install Helm
        uses: azure/setup-helm@v3
        with:
          version: v3.10.0
      
      - name: Run chart-releaser
        uses: helm/chart-releaser-action@v1.5.0
        env:
          CR_TOKEN: "${{ secrets.GITHUB_TOKEN }}"
```

### GitLab CI 集成

```yaml
# .gitlab-ci.yml
stages:
  - lint
  - test
  - release

variables:
  HELM_VERSION: "v3.10.0"
  KUBE_VERSION: "v1.25.0"

before_script:
  - apk add --no-cache curl bash git
  - curl -fsSL -o get_helm.sh https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3
  - chmod 700 get_helm.sh
  - ./get_helm.sh

.helm_lint_template: &helm_lint_definition
  stage: lint
  script:
    - helm lint .
  artifacts:
    when: on_failure
    paths:
      - logs/

lint:
  <<: *helm_lint_definition
  rules:
    - if: $CI_COMMIT_BRANCH == "main"

.test_template: &test_definition
  stage: test
  script:
    - export KUBECONFIG="$(mktemp)"
    - curl -Lo kubectl "https://dl.k8s.io/release/${KUBE_VERSION}/bin/linux/amd64/kubectl"
    - chmod +x kubectl
    - mv kubectl /usr/local/bin/
    - kubectl version --client
    - helm dependency build
    - helm install test-release . --dry-run
  artifacts:
    when: on_failure
    paths:
      - logs/

test:
  <<: *test_definition
  rules:
    - if: $CI_COMMIT_BRANCH == "main"

release:
  stage: release
  script:
    - echo "Releasing Helm Chart..."
    - helm package .
    - |
      if [[ -n "$CI_COMMIT_TAG" ]]; then
        echo "Publishing Helm Chart..."
        # 发布到仓库
      fi
  rules:
    - if: $CI_COMMIT_TAG
```

### Jenkins Pipeline 集成

```groovy
// Jenkinsfile
pipeline {
    agent any
    
    parameters {
        choice(
            name: 'ENVIRONMENT',
            choices: ['dev', 'staging', 'prod'],
            description: 'Target environment for deployment'
        )
        booleanParam(
            name: 'DRY_RUN',
            defaultValue: true,
            description: 'Perform dry run without actual deployment'
        )
    }
    
    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }
        
        stage('Validate') {
            steps {
                sh 'helm lint .'
                sh 'helm template test . --dry-run'
            }
        }
        
        stage('Test') {
            when {
                expression { params.ENVIRONMENT == 'dev' }
            }
            steps {
                script {
                    // 创建测试命名空间
                    sh 'kubectl create namespace test-namespace --dry-run=client -o yaml | kubectl apply -f -'
                    
                    // 安装测试版本
                    if (params.DRY_RUN) {
                        sh 'helm install test-app . --namespace test-namespace --dry-run'
                    } else {
                        sh 'helm install test-app . --namespace test-namespace'
                    }
                    
                    // 运行测试
                    sh '''
                        sleep 30
                        kubectl rollout status deployment/test-app --namespace test-namespace
                    '''
                }
            }
        }
        
        stage('Deploy') {
            when {
                anyOf {
                    expression { params.ENVIRONMENT != 'dev' }
                    expression { !params.DRY_RUN }
                }
            }
            steps {
                script {
                    def valuesFile = "values-${params.ENVIRONMENT}.yaml"
                    def namespace = params.ENVIRONMENT
                    
                    sh "kubectl create namespace ${namespace} --dry-run=client -o yaml | kubectl apply -f -"
                    
                    if (params.DRY_RUN) {
                        sh "helm upgrade --install myapp . --namespace ${namespace} -f ${valuesFile} --dry-run"
                    } else {
                        sh "helm upgrade --install myapp . --namespace ${namespace} -f ${valuesFile}"
                    }
                }
            }
        }
    }
    
    post {
        always {
            sh 'kubectl get pods --all-namespaces'
        }
        success {
            echo 'Deployment completed successfully!'
        }
        failure {
            echo 'Deployment failed!'
        }
    }
}
```

## 安全最佳实践

### Chart 安全扫描

```bash
# 使用 kubeval 验证 Kubernetes manifests
helm template mychart/ | kubeval --strict

# 使用 conftest 验证配置策略
helm template mychart/ | conftest test -p policies/ -

# 使用 Datree 检查配置错误
datree test . --schema-version 1.25.0

# 使用 Trivy 扫描容器镜像
trivy image nginx:latest
```

### 安全配置模板

```yaml
# templates/deployment.yaml - 安全配置示例
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "myapp.fullname" . }}
spec:
  replicas: {{ .Values.replicaCount }}
  selector:
    matchLabels:
      {{- include "myapp.selectorLabels" . | nindent 6 }}
  template:
    metadata:
      labels:
        {{- include "myapp.selectorLabels" . | nindent 8 }}
      annotations:
        seccomp.security.alpha.kubernetes.io/pod: 'runtime/default'
    spec:
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        fsGroup: 2000
      containers:
      - name: {{ .Chart.Name }}
        image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
        securityContext:
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: true
          runAsNonRoot: true
          runAsUser: 1000
          capabilities:
            drop:
            - ALL
            add:
            - NET_BIND_SERVICE
        ports:
        - containerPort: {{ .Values.service.port }}
        env:
        - name: DATABASE_HOST
          valueFrom:
            secretKeyRef:
              name: {{ include "myapp.fullname" . }}-db-secret
              key: host
        resources:
          {{- toYaml .Values.resources | nindent 10 }}
        livenessProbe:
          {{- toYaml .Values.livenessProbe | nindent 10 }}
        readinessProbe:
          {{- toYaml .Values.readinessProbe | nindent 10 }}
```

### 机密信息管理

```yaml
# 使用外部机密管理工具
# Chart.yaml
annotations:
  artifacthub.io/images: |
    - name: myapp
      image: myregistry/myapp:1.0.0
  artifacthub.io/links: |
    - name: Documentation
      url: https://my-docs.example.com
  artifacthub.io/operator: "false"
  
# values.yaml
secrets:
  backend: external  # external, vault, sealed-secrets
  externalSecretName: "myapp-secrets"
  
# templates/secrets.yaml
{{- if eq .Values.secrets.backend "external" }}
apiVersion: v1
kind: Secret
metadata:
  name: {{ include "myapp.fullname" . }}-secrets
  annotations:
    helm.sh/resource-policy: keep  # 不随Release删除
type: Opaque
data:
  {{- range $key, $val := .Values.externalSecrets }}
  {{ $key }}: {{ $val | b64enc }}
  {{- end }}
{{- end }}

# 使用 External Secrets Operator
{{- if eq .Values.secrets.backend "external-secrets" }}
apiVersion: external-secrets.io/v1beta1
kind: SecretStore
metadata:
  name: {{ include "myapp.fullname" . }}-vault-backend
spec:
  provider:
    vault:
      server: "https://vault.local"
      path: "secret"
      auth:
        kubernetes:
          mountPath: "kubernetes"
          role: "myapp-role"
{{- end }}
```

## 高级部署策略

### 蓝绿部署

```yaml
# templates/blue-green-deployment.yaml
{{- $blue := dict "name" "blue" "version" .Values.blueVersion }}
{{- $green := dict "name" "green" "version" .Values.greenVersion }}
{{- $activeColor := .Values.activeDeployment | default "blue" }}

{{- if eq $activeColor "blue" }}
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "myapp.fullname" . }}-blue
spec:
  replicas: {{ .Values.replicaCount }}
  selector:
    matchLabels:
      app: {{ include "myapp.name" . }}
      version: blue
  template:
    metadata:
      labels:
        app: {{ include "myapp.name" . }}
        version: blue
    spec:
      containers:
      - name: {{ .Chart.Name }}
        image: "{{ .Values.image.repository }}:{{ .Values.blueVersion }}"
---
apiVersion: v1
kind: Service
metadata:
  name: {{ include "myapp.fullname" . }}
spec:
  selector:
    app: {{ include "myapp.name" . }}
    version: blue
  ports:
    - protocol: TCP
      port: {{ .Values.service.port }}
      targetPort: {{ .Values.service.targetPort }}
{{- else }}
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "myapp.fullname" . }}-green
spec:
  replicas: {{ .Values.replicaCount }}
  selector:
    matchLabels:
      app: {{ include "myapp.name" . }}
      version: green
  template:
    metadata:
      labels:
        app: {{ include "myapp.name" . }}
        version: green
    spec:
      containers:
      - name: {{ .Chart.Name }}
        image: "{{ .Values.image.repository }}:{{ .Values.greenVersion }}"
---
apiVersion: v1
kind: Service
metadata:
  name: {{ include "myapp.fullname" . }}
spec:
  selector:
    app: {{ include "myapp.name" . }}
    version: green
  ports:
    - protocol: TCP
      port: {{ .Values.service.port }}
      targetPort: {{ .Values.service.targetPort }}
{{- end }}
```

### 金丝雀部署

```yaml
# templates/canary-deployment.yaml
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "myapp.fullname" . }}-stable
spec:
  replicas: {{ sub .Values.replicaCount .Values.canary.replicas }}
  selector:
    matchLabels:
      app: {{ include "myapp.name" . }}
      version: stable
  template:
    metadata:
      labels:
        app: {{ include "myapp.name" . }}
        version: stable
    spec:
      containers:
      - name: {{ .Chart.Name }}
        image: "{{ .Values.image.repository }}:{{ .Values.stable.version }}"
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "myapp.fullname" . }}-canary
spec:
  replicas: {{ .Values.canary.replicas }}
  selector:
    matchLabels:
      app: {{ include "myapp.name" . }}
      version: canary
  template:
    metadata:
      labels:
        app: {{ include "myapp.name" . }}
        version: canary
    spec:
      containers:
      - name: {{ .Chart.Name }}
        image: "{{ .Values.image.repository }}:{{ .Values.canary.version }}"
---
apiVersion: v1
kind: Service
metadata:
  name: {{ include "myapp.fullname" . }}-internal
spec:
  selector:
    app: {{ include "myapp.name" . }}
  ports:
    - name: http
      port: {{ .Values.service.port }}
      targetPort: {{ .Values.service.targetPort }}
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: {{ include "myapp.fullname" . }}
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "{{ .Values.canary.weight }}"
spec:
  rules:
  - host: {{ .Values.ingress.host }}
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: {{ include "myapp.fullname" . }}-canary
            port:
              number: {{ .Values.service.port }}
```

## 运维监控和日志

### Helm 部署监控

```bash
# 监控 Helm Release 状态
helm list --all-namespaces
helm status myrelease -n mynamespace

# 获取 Release 信息
helm get manifest myrelease -n mynamespace
helm get values myrelease -n mynamespace
helm get hooks myrelease -n mynamespace
helm get notes myrelease -n mynamespace

# 查看历史版本
helm history myrelease -n mynamespace

# Rollback 操作
helm rollback myrelease 1 -n mynamespace
```

### 自定义资源验证

```yaml
# templates/validation-job.yaml
{{- if .Values.validation.enabled }}
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "myapp.fullname" . }}-validation
  annotations:
    "helm.sh/hook": post-install,post-upgrade
    "helm.sh/hook-delete-policy": before-hook-creation,hook-succeeded
spec:
  template:
    spec:
      serviceAccountName: {{ include "myapp.fullname" . }}-validator
      restartPolicy: OnFailure
      containers:
      - name: validator
        image: "{{ .Values.validation.image.repository }}:{{ .Values.validation.image.tag }}"
        command:
        - /bin/sh
        - -c
        - |
          echo "Validating deployment..."
          kubectl rollout status deployment/{{ include "myapp.fullname" . }} -n {{ .Release.Namespace }}
          kubectl wait --for=condition=ready pod -l app={{ include "myapp.name" . }} -n {{ .Release.Namespace }}
{{- end }}
```

## 故障排除和调试

### 调试命令

```bash
# 模板渲染调试
helm template mychart/ --debug
helm install myrelease . --debug --dry-run

# 值覆盖验证
helm show values mychart/
helm inspect values mychart/

# 依赖管理
helm dependency list .
helm dependency update .
helm dependency build .

# 调试安装问题
helm install myrelease . --debug --dry-run
helm install myrelease . --timeout=10m --wait=false
kubectl describe pods -l app=myapp
```

### 常见问题解决

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| Chart 依赖解析失败 | requirements.yaml 或 Chart.yaml 配置错误 | 检查仓库 URL，运行 `helm dependency update` |
| 模板渲染错误 | Go 模板语法错误 | 使用 `helm template` 调试，检查语法 |
| 权限不足 | RBAC 配置不当 | 检查 ServiceAccount 和 RBAC 规则 |
| 资源已存在 | 上次安装未正确清理 | 使用 `helm uninstall` 清理，或添加 `--no-hooks` |
| 镜像拉取失败 | 私有仓库凭据缺失 | 配置 ImagePullSecrets |

## 生产环境最佳实践

### 生产环境配置示例

```yaml
# production-values.yaml
# 资源限制
resources:
  limits:
    cpu: 500m
    memory: 1Gi
  requests:
    cpu: 250m
    memory: 512Mi

# 副本配置
replicaCount: 3

# 健康检查
livenessProbe:
  httpGet:
    path: /healthz
    port: http
  initialDelaySeconds: 30
  periodSeconds: 10
  timeoutSeconds: 5
  failureThreshold: 3

readinessProbe:
  httpGet:
    path: /ready
    port: http
  initialDelaySeconds: 5
  periodSeconds: 5
  timeoutSeconds: 3
  failureThreshold: 3

# 安全配置
podSecurityContext:
  runAsNonRoot: true
  runAsUser: 1000
  fsGroup: 2000

securityContext:
  allowPrivilegeEscalation: false
  readOnlyRootFilesystem: false  # 根据应用需求调整
  runAsNonRoot: true
  runAsUser: 1000
  capabilities:
    drop:
    - ALL

# 存储配置
persistence:
  enabled: true
  size: 10Gi
  storageClass: fast-ssd
  accessMode: ReadWriteOnce

# 网络策略
networkPolicy:
  enabled: true
  allowExternal: false

# 监控集成
prometheus:
  enabled: true
  serviceMonitor:
    enabled: true
    interval: 30s
```

### 滚动更新策略

```yaml
# 滚动更新配置
updateStrategy:
  type: RollingUpdate
  rollingUpdate:
    maxUnavailable: 1
    maxSurge: 1

# Pod 优化
podAnnotations:
  prometheus.io/scrape: "true"
  prometheus.io/port: "8080"
  prometheus.io/path: "/metrics"

podLabels:
  app.kubernetes.io/part-of: myplatform
  app.kubernetes.io/managed-by: helm
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)