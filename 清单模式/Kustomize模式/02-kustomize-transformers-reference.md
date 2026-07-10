---
title: Kustomize Transformers 参考手册
description: 'namePrefix/nameSuffix/commonLabels/patches/ConfigMapGenerator/images 字段详解'
summary: 'namePrefix/nameSuffix/commonLabels/patches/ConfigMapGenerator/images 字段详解'
category: manifests-patterns
tags:
- kustomize
- transformers
- patches
- generators
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
- Kustomize Transformers 是什么
- 如何使用 Kustomize patches
- Kustomize ConfigMapGenerator 如何工作
trigger_keywords:
- kustomize
- transformers
- namePrefix
- patches
- configMapGenerator
- secretGenerator
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


# Kustomize Transformers 参考手册

## 1. 命名 Transformers

### 1.1 namePrefix

```yaml
# 添加名称前缀
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

namePrefix: dev-

# 效果：
# my-app → dev-my-app
# my-app-service → dev-my-app-service
```

### 1.2 nameSuffix

```yaml
# 添加名称后缀
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

nameSuffix: -v2

# 效果：
# my-app → my-app-v2
# my-app-service → my-app-service-v2
```

### 1.3 命名注意事项

```yaml
# namePrefix/nameSuffix 会自动更新引用关系
# Service → Deployment 引用
# ConfigMap → Deployment 引用
# Secret → Deployment 引用

# 示例
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

namePrefix: prod-

resources:
  - deployment.yaml    # name: my-app → prod-my-app
  - service.yaml       # selector: my-app → prod-my-app（自动更新）
  - configmap.yaml     # name: app-config → prod-app-config（自动更新）
```

## 2. 标签与注解 Transformers

### 2.1 commonLabels

```yaml
# 添加通用标签
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

commonLabels:
  environment: production
  team: platform
  managed-by: kustomize

# 效果：所有资源添加以下标签
# metadata.labels
# spec.selector.matchLabels
# spec.template.metadata.labels
```

### 2.2 commonAnnotations

```yaml
# 添加通用注解
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

commonAnnotations:
  company.example.com/owner: platform-team
  company.example.com/cost-center: "12345"

# 效果：所有资源添加注解（不影响 selector）
```

### 2.3 labelSelector 注意事项

```yaml
# 警告：commonLabels 会修改 spec.selector
# 已部署的资源修改 selector 会导致问题

# 解决方案：使用 labels 替代
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

labels:
  - pairs:
      environment: production
    includeSelectors: false    # 不修改 selector
```

## 3. Patches 详解

### 3.1 Strategic Merge Patch

```yaml
# 使用 YAML 格式的 patch
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

patches:
  # 方式 1：直接 inline
  - patch: |
      apiVersion: apps/v1
      kind: Deployment
      metadata:
        name: my-app
      spec:
        replicas: 3
        template:
          spec:
            containers:
              - name: my-app
                resources:
                  limits:
                    cpu: "2"
                    memory: 2Gi

  # 方式 2：引用文件
  - path: patches/deployment-patch.yaml

  # 方式 3：带 target 选择器
  - path: patches/resource-patch.yaml
    target:
      group: apps
      version: v1
      kind: Deployment
      name: my-app
```

### 3.2 JSON 6902 Patch

```yaml
# 使用 JSON Patch 格式
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

patches:
  - target:
      group: apps
      version: v1
      kind: Deployment
      name: my-app
    patch: |-
      - op: replace
        path: /spec/replicas
        value: 3
      - op: add
        path: /spec/template/spec/containers/0/env/-
        value:
          name: NEW_VAR
          value: "hello"
      - op: remove
        path: /spec/template/spec/containers/0/env/0
```

### 3.3 Patch Target 选择器

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

patches:
  # 按名称匹配
  - path: patches/deployment-patch.yaml
    target:
      kind: Deployment
      name: my-app

  # 按标签匹配
  - path: patches/resource-patch.yaml
    target:
      kind: Deployment
      labelSelector: "app=backend"

  # 按组/版本/种类匹配
  - path: patches/all-deployments.yaml
    target:
      group: apps
      version: v1
      kind: Deployment

  # 匹配所有资源
  - path: patches/namespace-patch.yaml
    target:
      kind: ".*"
      name: ".*"
```

### 3.4 复杂 Patch 示例

```yaml
# 添加 sidecar 容器
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

patches:
  - target:
      kind: Deployment
      name: my-app
    patch: |-
      apiVersion: apps/v1
      kind: Deployment
      metadata:
        name: my-app
      spec:
        template:
          spec:
            containers:
              - name: my-app
                # 保留原配置
              - name: sidecar
                image: sidecar:latest
                ports:
                  - containerPort: 9090
                resources:
                  requests:
                    cpu: 100m
                    memory: 128Mi
```

```yaml
# 添加 Volume 和 VolumeMount
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

patches:
  - target:
      kind: Deployment
      name: my-app
    patch: |-
      apiVersion: apps/v1
      kind: Deployment
      metadata:
        name: my-app
      spec:
        template:
          spec:
            containers:
              - name: my-app
                volumeMounts:
                  - name: config-volume
                    mountPath: /etc/config
            volumes:
              - name: config-volume
                configMap:
                  name: app-config
```

## 4. ConfigMapGenerator

### 4.1 基础用法

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

configMapGenerator:
  # 从字面量生成
  - name: app-config
    literals:
      - APP_NAME=my-app
      - LOG_LEVEL=info
      - DEBUG=false

  # 从文件生成
  - name: app-properties
    files:
      - config/application.properties
      - config/logback.xml

  # 从 env 文件生成
  - name: app-env
    envs:
      - config.env
```

### 4.2 ConfigMap 行为模式

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

configMapGenerator:
  # 创建模式（默认）
  - name: app-config
    behavior: create
    literals:
      - KEY1=value1

  # 合并模式（overlay 中）
  - name: app-config
    behavior: merge
    literals:
      - KEY2=value2    # 新增
      - KEY1=new-value # 覆盖

  # 替换模式
  - name: app-config
    behavior: replace
    literals:
      - COMPLETELY=new
```

### 4.3 ConfigMapGenerator 高级选项

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

configMapGenerator:
  - name: app-config
    # 命名空间
    namespace: production
    # 标签
    options:
      labels:
        app.kubernetes.io/component: config
      annotations:
        description: "Application configuration"
    # 文件内容
    files:
      - application.yaml=config/application.yaml
    literals:
      - APP_ENV=production
    envs:
      - .env.production

# 生成的 ConfigMap 名称会自动添加哈希后缀
# app-config → app-config-8k2m5hd9fg
# 这确保配置变更时 Deployment 自动滚动更新
```

### 4.4 禁用哈希后缀

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

generatorOptions:
  disableNameSuffixHash: true

configMapGenerator:
  - name: app-config
    literals:
      - KEY=value
# 生成：app-config（无哈希后缀）
```

## 5. SecretGenerator

### 5.1 基础用法

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

secretGenerator:
  # 从字面量生成
  - name: app-secrets
    type: Opaque
    literals:
      - username=admin
      - password=P@ssw0rd

  # 从文件生成
  - name: tls-secret
    type: kubernetes.io/tls
    files:
      - tls.crt=certs/server.crt
      - tls.key=certs/server.key

  # 从 env 文件生成
  - name: db-credentials
    type: Opaque
    envs:
      - secrets.env
```

### 5.2 Secret 类型

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

secretGenerator:
  # Opaque（默认）
  - name: generic-secret
    type: Opaque
    literals:
      - key=value

  # TLS
  - name: tls-secret
    type: kubernetes.io/tls
    files:
      - tls.crt=cert.pem
      - tls.key=key.pem

  # Docker Registry
  - name: docker-registry
    type: kubernetes.io/dockerconfigjson
    literals:
      - .dockerconfigjson={"auths":{"registry.example.com":{"username":"user","password":"pass"}}}
```

### 5.3 SecretGenerator 配置

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

generatorOptions:
  # 禁用哈希后缀
  disableNameSuffixHash: true
  # 标签
  labels:
    app.kubernetes.io/component: secrets
  # 注解
  annotations:
    description: "Application secrets"

secretGenerator:
  - name: app-secrets
    type: Opaque
    literals:
      - DB_PASSWORD=secret123
```

## 6. Images 字段覆盖

### 6.1 基础镜像覆盖

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

images:
  # 覆盖镜像名称和标签
  - name: my-app
    newName: registry.example.com/my-app
    newTag: v2.0.0

  # 仅覆盖标签
  - name: nginx
    newTag: 1.25.3

  # 仅覆盖名称（用于私有仓库）
  - name: postgres
    newName: registry.example.com/postgres

  # 使用摘要（不可变）
  - name: my-app
    newName: registry.example.com/my-app
    digest: sha256:abc123...
```

### 6.2 镜像选择器

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

images:
  # 精确匹配
  - name: my-app
    newName: registry.example.com/my-app
    newTag: v1.0.0

  # 匹配多个容器
  - name: sidecar
    newName: registry.example.com/sidecar
    newTag: v2.0.0

  # 匹配 Init 容器
  - name: init-db
    newName: registry.example.com/init-db
    newTag: v1.0.0
```

### 6.3 多环境镜像管理

```yaml
# base/kustomization.yaml
images:
  - name: my-app
    newName: registry.example.com/my-app
    newTag: v1.0.0

# overlays/dev/kustomization.yaml
images:
  - name: my-app
    newTag: dev-latest

# overlays/prod/kustomization.yaml
images:
  - name: my-app
    newTag: v1.2.3
    digest: sha256:abc123...
```

## 7. 其他 Transformers

### 7.1 namespace

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

namespace: production

# 效果：所有资源的 namespace 设为 production
# 包括 ClusterRoleBinding 等集群级资源的 subject.namespace
```

### 7.2 Sort Order

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

sortOptions:
  order: fifo    # 先创建的先输出

# 或
sortOptions:
  order: legacy  # 按类型排序（默认）
```

### 7.3 Replacement Transformer

```yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

replacements:
  # 将 ConfigMap 的值注入到 Deployment
  - source:
      kind: ConfigMap
      name: app-config
      fieldPath: data.APP_VERSION
    targets:
      - select:
          kind: Deployment
          name: my-app
        fieldPaths:
          - spec.template.metadata.labels.app-version

  # 跨资源引用
  - source:
      kind: Service
      name: my-app
      fieldPath: spec.clusterIP
    targets:
      - select:
          kind: ConfigMap
          name: service-config
        fieldPaths:
          - data.SERVICE_IP
```

## 8. 完整示例

```yaml
# overlays/prod/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

resources:
  - ../../base

namespace: my-app-prod
namePrefix: prod-

commonLabels:
  environment: production
  team: platform

commonAnnotations:
  company.example.com/owner: platform-team

patches:
  - target:
      kind: Deployment
      name: my-app
    patch: |-
      apiVersion: apps/v1
      kind: Deployment
      metadata:
        name: my-app
      spec:
        replicas: 3
        template:
          spec:
            containers:
              - name: my-app
                resources:
                  limits:
                    cpu: "2"
                    memory: 2Gi

images:
  - name: my-app
    newName: registry.example.com/my-app
    newTag: v1.2.3

configMapGenerator:
  - name: app-config
    behavior: merge
    literals:
      - LOG_LEVEL=warn
      - DEBUG=false

secretGenerator:
  - name: app-secrets
    type: Opaque
    literals:
      - DB_PASSWORD=prod-secret

generatorOptions:
  disableNameSuffixHash: true
```

---

## Related

- [[清单模式/Kustomize模式/01-kustomize-base-overlay-structure|Kustomize Base/Overlay 分层架构]]
- [[清单模式/Kustomize模式/03-kustomize-remote-build-gitops|Kustomize 高级特性与 GitOps]]

## See Also

- [Kustomize Transformers](https://kubectl.docs.kubernetes.io/references/kustomize/)
- [Kustomize Patch](https://kubectl.docs.kubernetes.io/references/kustomize/builtins/)


<!-- risk-assessed -->
