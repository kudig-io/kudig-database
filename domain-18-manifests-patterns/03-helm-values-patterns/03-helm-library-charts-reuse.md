---
title: Helm Library Chart 复用模式
description: '模板复用 define/include、命名空间隔离、Chart 依赖管理与 Helmfile 多 Chart 编排'
summary: '模板复用 define/include、命名空间隔离、Chart 依赖管理与 Helmfile 多 Chart 编排'
category: manifests-patterns
tags:
- helm
- library-chart
- helmfile
- chart-dependencies
- template-reuse
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
- Helm Library Chart 是什么
- 如何使用 define/include 复用模板
- Helmfile 如何编排多个 Chart
trigger_keywords:
- helm
- library-chart
- define
- include
- helmfile
- chart-dependencies
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


# Helm Library Chart 复用模式

## 1. Library Chart 概述

Library Chart 是不包含实际资源的 Chart，仅提供可复用的模板定义：

```
my-library-chart/
├── Chart.yaml              # type: library
├── templates/
│   ├── _deployment.yaml    # define 定义
│   ├── _service.yaml
│   ├── _configmap.yaml
│   └── _helpers.tpl
└── values.yaml             # 默认值（可选）
```

Library Chart vs Application Chart：

| 特性 | Library Chart | Application Chart |
|------|---------------|-------------------|
| type | library | application |
| 可安装 | 否 | 是 |
| 提供 | 模板定义 | 实际资源 |
| 使用方式 | include 引用 | 直接安装 |

## 2. define/include 模式

### 2.1 基础 define

```yaml
# library-chart/templates/_deployment.yaml
{{/*
定义 Deployment 模板
*/}}
{{- define "common.deployment" -}}
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ .name }}
  labels:
    {{- include "common.labels" . | nindent 4 }}
  {{- with .annotations }}
  annotations:
    {{- toYaml . | nindent 4 }}
  {{- end }}
spec:
  replicas: {{ .replicas | default 1 }}
  selector:
    matchLabels:
      app.kubernetes.io/name: {{ .name }}
  template:
    metadata:
      labels:
        app.kubernetes.io/name: {{ .name }}
        {{- with .podLabels }}
        {{- toYaml . | nindent 8 }}
        {{- end }}
    spec:
      {{- with .imagePullSecrets }}
      imagePullSecrets:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      containers:
        - name: {{ .name }}
          image: "{{ .image.repository }}:{{ .image.tag }}"
          imagePullPolicy: {{ .image.pullPolicy | default "IfNotPresent" }}
          ports:
            {{- range .ports }}
            - containerPort: {{ .containerPort }}
              protocol: {{ .protocol | default "TCP" }}
              name: {{ .name | default "http" }}
            {{- end }}
          {{- with .env }}
          env:
            {{- toYaml . | nindent 12 }}
          {{- end }}
          {{- with .resources }}
          resources:
            {{- toYaml . | nindent 12 }}
          {{- end }}
          {{- with .livenessProbe }}
          livenessProbe:
            {{- toYaml . | nindent 12 }}
          {{- end }}
          {{- with .readinessProbe }}
          readinessProbe:
            {{- toYaml . | nindent 12 }}
          {{- end }}
          {{- with .volumeMounts }}
          volumeMounts:
            {{- toYaml . | nindent 12 }}
          {{- end }}
      {{- with .volumes }}
      volumes:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      {{- with .nodeSelector }}
      nodeSelector:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      {{- with .tolerations }}
      tolerations:
        {{- toYaml . | nindent 8 }}
      {{- end }}
      {{- with .affinity }}
      affinity:
        {{- toYaml . | nindent 8 }}
      {{- end }}
{{- end -}}
```

### 2.2 基础 include

```yaml
# application-chart/templates/deployment.yaml
{{- include "common.deployment" (dict
  "name" (include "myapp.fullname" .)
  "image" .Values.image
  "replicas" .Values.replicaCount
  "ports" .Values.ports
  "resources" .Values.resources
  "env" .Values.env
  "livenessProbe" .Values.livenessProbe
  "readinessProbe" .Values.readinessProbe
  "annotations" .Values.deploymentAnnotations
  "podLabels" .Values.podLabels
  "imagePullSecrets" .Values.imagePullSecrets
  "volumes" .Values.volumes
  "volumeMounts" .Values.volumeMounts
  "nodeSelector" .Values.nodeSelector
  "tolerations" .Values.tolerations
  "affinity" .Values.affinity
) -}}
```

### 2.3 Service 模板

```yaml
# library-chart/templates/_service.yaml
{{- define "common.service" -}}
apiVersion: v1
kind: Service
metadata:
  name: {{ .name }}
  labels:
    {{- include "common.labels" . | nindent 4 }}
  {{- with .annotations }}
  annotations:
    {{- toYaml . | nindent 4 }}
  {{- end }}
spec:
  type: {{ .type | default "ClusterIP" }}
  ports:
    {{- range .ports }}
    - port: {{ .port }}
      targetPort: {{ .targetPort | default .port }}
      protocol: {{ .protocol | default "TCP" }}
      name: {{ .name | default "http" }}
    {{- end }}
  selector:
    app.kubernetes.io/name: {{ .name }}
{{- end -}}
```

### 2.4 ConfigMap 模板

```yaml
# library-chart/templates/_configmap.yaml
{{- define "common.configmap" -}}
apiVersion: v1
kind: ConfigMap
metadata:
  name: {{ .name }}
  labels:
    {{- include "common.labels" . | nindent 4 }}
data:
  {{- range $key, $value := .data }}
  {{ $key }}: {{ $value | quote }}
  {{- end }}
  {{- range $key, $value := .files }}
  {{ $key }}: |
    {{- $value | nindent 4 }}
  {{- end }}
{{- end -}}
```

## 3. 复杂模板模式

### 3.1 条件渲染

```yaml
{{- define "common.deployment" -}}
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ .name }}
spec:
  {{- if .autoscaling }}
  # HPA 管理副本数，不设置 replicas
  {{- else }}
  replicas: {{ .replicas | default 1 }}
  {{- end }}
  # ...
  template:
    spec:
      containers:
        - name: {{ .name }}
          image: "{{ .image.repository }}:{{ .image.tag }}"
          {{- if .metrics }}
          ports:
            - containerPort: {{ .metrics.port | default 9090 }}
              name: metrics
              protocol: TCP
          {{- end }}
{{- end -}}
```

### 3.2 循环渲染

```yaml
{{- define "common.deployment" -}}
# ...
spec:
  template:
    spec:
      containers:
        {{- range .containers }}
        - name: {{ .name }}
          image: "{{ .image.repository }}:{{ .image.tag }}"
          {{- with .ports }}
          ports:
            {{- toYaml . | nindent 12 }}
          {{- end }}
          {{- with .env }}
          env:
            {{- toYaml . | nindent 12 }}
          {{- end }}
          {{- with .resources }}
          resources:
            {{- toYaml . | nindent 12 }}
          {{- end }}
        {{- end }}
{{- end -}}
```

### 3.3 模板继承

```yaml
# 基础模板
{{- define "common.pod" -}}
spec:
  {{- with .imagePullSecrets }}
  imagePullSecrets:
    {{- toYaml . | nindent 4 }}
  {{- end }}
  containers:
    - name: {{ .name }}
      image: "{{ .image.repository }}:{{ .image.tag }}"
      {{- with .resources }}
      resources:
        {{- toYaml . | nindent 8 }}
      {{- end }}
{{- end -}}

# 继承并扩展
{{- define "common.deployment.pod" -}}
{{- include "common.pod" . }}
      livenessProbe:
        httpGet:
          path: /healthz
          port: http
        initialDelaySeconds: 10
      readinessProbe:
        httpGet:
          path: /ready
          port: http
        initialDelaySeconds: 5
{{- end -}}
```

## 4. Library Chart 依赖管理

### 4.1 Chart.yaml 配置

```yaml
# application-chart/Chart.yaml
apiVersion: v2
name: my-application
description: My Application Chart
type: application
version: 1.0.0
appVersion: "1.0.0"

dependencies:
  # Library Chart 依赖
  - name: common
    version: "1.x.x"
    repository: "https://charts.example.com"
    # 或本地路径
    # repository: "file://../common-library"

  # Application Chart 依赖
  - name: postgresql
    version: "12.x.x"
    repository: "https://charts.bitnami.com/bitnami"
    condition: postgresql.enabled
```

### 4.2 依赖更新

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 更新 Chart.lock
helm dependency update ./my-application

# 构建依赖
helm dependency build ./my-application

# 查看依赖
helm dependency list ./my-application
```
### 4.3 私有 Chart 仓库

```yaml
# 添加私有仓库
helm repo add my-repo https://charts.example.com --username user --password pass

# 或使用 OCI 仓库
helm registry login registry.example.com --username user --password pass
```

## 5. 命名空间隔离

### 5.1 命名空间模板

```yaml
# library-chart/templates/_namespace.yaml
{{- define "common.namespace" -}}
apiVersion: v1
kind: Namespace
metadata:
  name: {{ .name }}
  labels:
    {{- include "common.labels" . | nindent 4 }}
  {{- with .annotations }}
  annotations:
    {{- toYaml . | nindent 4 }}
  {{- end }}
  {{- with .labels }}
    {{- toYaml . | nindent 4 }}
  {{- end }}
{{- end -}}
```

### 5.2 多命名空间部署

```yaml
# application-chart/values.yaml
namespaces:
  - name: my-app-frontend
    labels:
      istio-injection: enabled
  - name: my-app-backend
    labels:
      istio-injection: enabled
  - name: my-app-database
    labels:
      network-policy: restricted
```

```yaml
# application-chart/templates/namespaces.yaml
{{- range .Values.namespaces }}
---
{{- include "common.namespace" (dict
  "name" .name
  "labels" .labels
  "annotations" .annotations
) -}}
{{- end }}
```

## 6. 私有 Chart 仓库（Harbor/OCI）

### 6.1 Harbor Chart 仓库

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 推送到 Harbor
helm package ./my-library-chart
helm cm-push my-library-chart-1.0.0.tgz my-repo

# 或使用 ChartMuseum
helm repo add chartmuseum https://charts.example.com
helm cm-push ./my-library-chart chartmuseum
```
### 6.2 OCI 仓库

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 登录 OCI 仓库
helm registry login registry.example.com \
  --username user \
  --password pass

# 打包并推送
helm package ./my-library-chart
helm push my-library-chart-1.0.0.tgz oci://registry.example.com/charts

# 使用 OCI Chart
helm pull oci://registry.example.com/charts/my-library-chart --version 1.0.0
```
### 6.3 OCI 仓库配置

```yaml
# Chart.yaml 使用 OCI
dependencies:
  - name: common
    version: "1.0.0"
    repository: "oci://registry.example.com/charts"
```

## 7. Helmfile 多 Chart 编排

### 7.1 基础 Helmfile

```yaml
# helmfile.yaml
repositories:
  - name: bitnami
    url: https://charts.bitnami.com/bitnami
  - name: ingress-nginx
    url: https://kubernetes.github.io/ingress-nginx
  - name: my-repo
    url: https://charts.example.com

releases:
  - name: nginx-ingress
    namespace: ingress-nginx
    chart: ingress-nginx/ingress-nginx
    version: "4.x.x"
    values:
      - values/nginx-ingress.yaml

  - name: my-app
    namespace: my-app
    chart: my-repo/my-application
    version: "1.x.x"
    values:
      - values/my-app.yaml
    needs:
      - ingress-nginx/nginx-ingress

  - name: postgresql
    namespace: my-app
    chart: bitnami/postgresql
    version: "12.x.x"
    values:
      - values/postgresql.yaml
    set:
      - name: auth.database
        value: myapp
```

### 7.2 多环境 Helmfile

```yaml
# helmfile.yaml
environments:
  dev:
    values:
      - environments/dev.yaml
  staging:
    values:
      - environments/staging.yaml
  prod:
    values:
      - environments/prod.yaml

---

releases:
  - name: my-app
    namespace: {{ .Values.namespace }}
    chart: ./charts/my-application
    values:
      - values/my-app.yaml
      - environments/{{ .Environment.Name }}.yaml
```

### 7.3 Helmfile 高级用法

```yaml
# helmfile.yaml
# 1. 环境变量
{{ $env := env "ENVIRONMENT" | default "dev" }}

# 2. 条件部署
releases:
  - name: monitoring
    namespace: monitoring
    chart: ./charts/monitoring
    installed: {{ ne $env "dev" }}

  - name: logging
    namespace: logging
    chart: ./charts/logging
    installed: {{ eq $env "prod" }}

# 3. 依赖关系
  - name: my-app
    namespace: my-app
    chart: ./charts/my-app
    needs:
      - monitoring/prometheus
      - logging/loki
```

### 7.4 Helmfile 命令

```bash
# 查看所有 release
helmfile list

# 同步所有 release
helmfile sync

# 同步特定环境
helmfile -e prod sync

# 差异比较
helmfile diff

# 销毁所有 release
helmfile destroy

# 锁定依赖
helmfile deps
```

## 8. Library Chart 最佳实践

### 8.1 命名规范

```yaml
# 模板命名
{{- define "common.deployment" -}}    # 公共库
{{- define "myapp.fullname" -}}       # 应用私有

# 参数命名
{{ .name }}         # 必需参数
{{ .replicas }}     # 可选参数（有默认值）
```

### 8.2 文档注释

```yaml
{{/*
通用 Deployment 模板

参数:
  - name (string): 资源名称（必需）
  - image (object): 镜像配置（必需）
    - repository (string): 镜像仓库
    - tag (string): 镜像标签
    - pullPolicy (string): 拉取策略
  - replicas (int): 副本数量（默认: 1）
  - ports (list): 端口列表
  - resources (object): 资源限制

用法:
  {{- include "common.deployment" (dict
    "name" "my-app"
    "image" .Values.image
    "replicas" .Values.replicaCount
  ) -}}
*/}}
{{- define "common.deployment" -}}
# ...
{{- end -}}
```

### 8.3 测试 Library Chart

```yaml
# tests/test-deployment.yaml
apiVersion: v1
kind: Pod
metadata:
  name: test-deployment
  annotations:
    "helm.sh/hook": test
spec:
  restartPolicy: Never
  containers:
    - name: test
      image: bitnami/kubectl:latest
      command:
        - /bin/bash
        - -c
        - |
          kubectl get deployment {{ include "myapp.fullname" . }} -n {{ .Release.Namespace }}
```

## 9. 常见问题排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 问题 1: include 未找到模板
# 确认 Library Chart 依赖已更新
helm dependency update ./my-application

# 问题 2: 模板渲染错误
# 查看详细错误
helm template my-app ./my-application --debug

# 问题 3: Helmfile 同步失败
# 查看日志
helmfile -e prod sync --debug
```
---

## Related

- [[Helm Values 最佳实践|01-helm-values-best-practices]]
- [[Helm Hooks 生命周期|02-helm-hooks-lifecycle]]

## See Also

- [Helm Library Chart](https://helm.sh/docs/topics/library_charts/)
- [Helm Template Functions](https://helm.sh/docs/chart_template_guide/functions_and_pipelines/)
- [Helmfile](https://github.com/helmfile/helmfile)


<!-- risk-assessed -->
