---
title: Kustomize Base/Overlay 分层架构
description: '多环境目录组织、base 提取原则、overlay 继承机制与最佳实践'
summary: '多环境目录组织、base 提取原则、overlay 继承机制与最佳实践'
category: manifests-patterns
tags:
- kustomize
- multi-env
- overlay
- base
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
- Kustomize Base/Overlay 是什么
- 如何组织 Kustomize 多环境目录
- Kustomize 继承机制如何工作
trigger_keywords:
- kustomize
- base
- overlay
- multi-env
- kustomization
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


# Kustomize Base/Overlay 分层架构

## 1. 核心概念

Kustomize 采用声明式配置管理，通过 Base 和 Overlay 分层实现多环境配置：

```
base/              # 通用配置（所有环境共享）
  ├── kustomization.yaml
  ├── deployment.yaml
  ├── service.yaml
  └── configmap.yaml

overlays/
  ├── dev/          # 开发环境特有配置
  │   ├── kustomization.yaml
  │   └── patches/
  ├── staging/      # 预发布环境
  │   ├── kustomization.yaml
  │   └── patches/
  └── prod/         # 生产环境
      ├── kustomization.yaml
      └── patches/
```

Base/Overlay 关系：

| 层级 | 职责 | 示例 |
|------|------|------|
| **Base** | 通用资源定义 | Deployment、Service、ConfigMap |
| **Overlay** | 环境差异化 | 副本数、镜像版本、资源限制 |

## 2. 目录结构设计

### 2.1 推荐目录结构

```
kustomize-app/
├── base/
│   ├── kustomization.yaml
│   ├── namespace.yaml
│   ├── deployment.yaml
│   ├── service.yaml
│   ├── configmap.yaml
│   ├── hpa.yaml
│   └── pdb.yaml
├── overlays/
│   ├── dev/
│   │   ├── kustomization.yaml
│   │   ├── namespace-patch.yaml
│   │   └── patches/
│   │       ├── deployment-patch.yaml
│   │       └── resource-patch.yaml
│   ├── staging/
│   │   ├── kustomization.yaml
│   │   ├── namespace-patch.yaml
│   │   └── patches/
│   │       ├── deployment-patch.yaml
│   │       └── resource-patch.yaml
│   └── prod/
│       ├── kustomization.yaml
│       ├── namespace-patch.yaml
│       └── patches/
│           ├── deployment-patch.yaml
│           ├── resource-patch.yaml
│           └── hpa-patch.yaml
└── components/        # 可复用组件
    ├── monitoring/
    │   ├── kustomization.yaml
    │   ├── servicemonitor.yaml
    │   └── prometheusrule.yaml
    └── logging/
        ├── kustomization.yaml
        └── fluentd-config.yaml
```

### 2.2 命名规范

```
目录命名：
  - base/                    # 固定名称
  - overlays/<env>/          # 环境名称：dev/staging/prod
  - components/<feature>/    # 功能名称

文件命名：
  - kustomization.yaml      # 固定名称
  - <resource>-<type>.yaml   # 如 deployment-patch.yaml
```

## 3. Base 配置详解

### 3.1 Base kustomization.yaml

```yaml
# base/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

# 通用标签
commonLabels:
  app.kubernetes.io/name: my-app
  app.kubernetes.io/part-of: my-platform

# 通用注解
commonAnnotations:
  app.kubernetes.io/managed-by: kustomize

# 资源列表
resources:
  - namespace.yaml
  - deployment.yaml
  - service.yaml
  - configmap.yaml
  - hpa.yaml
  - pdb.yaml

# 通用 ConfigMap
configMapGenerator:
  - name: app-config
    literals:
      - APP_NAME=my-app
      - LOG_LEVEL=info
    envs:
      - config.env

# 通用镜像
images:
  - name: my-app
    newName: registry.example.com/my-app
    newTag: v1.0.0
```

### 3.2 Base 资源定义

```yaml
# base/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
  labels:
    app.kubernetes.io/name: my-app
spec:
  replicas: 1    # 由 overlay 覆盖
  selector:
    matchLabels:
      app.kubernetes.io/name: my-app
  template:
    metadata:
      labels:
        app.kubernetes.io/name: my-app
    spec:
      containers:
        - name: my-app
          image: my-app    # 由 kustomization images 覆盖
          ports:
            - containerPort: 8080
          envFrom:
            - configMapRef:
                name: app-config
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 256Mi
          livenessProbe:
            httpGet:
              path: /healthz
              port: 8080
            initialDelaySeconds: 10
          readinessProbe:
            httpGet:
              path: /ready
              port: 8080
```

```yaml
# base/service.yaml
apiVersion: v1
kind: Service
metadata:
  name: my-app
spec:
  ports:
    - port: 80
      targetPort: 8080
      protocol: TCP
  selector:
    app.kubernetes.io/name: my-app
```

```yaml
# base/hpa.yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: my-app
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: my-app
  minReplicas: 1
  maxReplicas: 10
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70
```

## 4. Overlay 配置详解

### 4.1 Dev Overlay

```yaml
# overlays/dev/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

# 引用 base
resources:
  - ../../base

# 命名空间
namespace: my-app-dev

# 名称前缀/后缀
namePrefix: dev-

# 环境标签
commonLabels:
  environment: dev

# 补丁
patches:
  - path: patches/deployment-patch.yaml
    target:
      group: apps
      version: v1
      kind: Deployment
      name: my-app
  - path: patches/resource-patch.yaml
    target:
      group: apps
      version: v1
      kind: Deployment
      name: my-app

# 镜像覆盖
images:
  - name: my-app
    newTag: dev-latest

# ConfigMap 覆盖
configMapGenerator:
  - name: app-config
    behavior: merge
    literals:
      - LOG_LEVEL=debug
      - DEBUG=true
```

```yaml
# overlays/dev/patches/deployment-patch.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  replicas: 1
  template:
    spec:
      containers:
        - name: my-app
          env:
            - name: ENVIRONMENT
              value: development
```

```yaml
# overlays/dev/patches/resource-patch.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  template:
    spec:
      containers:
        - name: my-app
          resources:
            requests:
              cpu: 50m
              memory: 64Mi
            limits:
              cpu: 200m
              memory: 128Mi
```

### 4.2 Staging Overlay

```yaml
# overlays/staging/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

resources:
  - ../../base

namespace: my-app-staging
namePrefix: staging-

commonLabels:
  environment: staging

patches:
  - path: patches/deployment-patch.yaml
    target:
      group: apps
      version: v1
      kind: Deployment
      name: my-app

images:
  - name: my-app
    newTag: v1.0.0-rc.1

configMapGenerator:
  - name: app-config
    behavior: merge
    literals:
      - LOG_LEVEL=info
```

```yaml
# overlays/staging/patches/deployment-patch.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  replicas: 2
  template:
    spec:
      containers:
        - name: my-app
          resources:
            requests:
              cpu: 200m
              memory: 256Mi
            limits:
              cpu: "1"
              memory: 512Mi
```

### 4.3 Prod Overlay

```yaml
# overlays/prod/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

resources:
  - ../../base
  - ../../components/monitoring

namespace: my-app-prod
namePrefix: prod-

commonLabels:
  environment: production

patches:
  - path: patches/deployment-patch.yaml
    target:
      group: apps
      version: v1
      kind: Deployment
      name: my-app
  - path: patches/hpa-patch.yaml
    target:
      group: autoscaling
      version: v2
      kind: HorizontalPodAutoscaler
      name: my-app

images:
  - name: my-app
    newTag: v1.0.0

configMapGenerator:
  - name: app-config
    behavior: merge
    literals:
      - LOG_LEVEL=warn
```

```yaml
# overlays/prod/patches/deployment-patch.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  template:
    spec:
      containers:
        - name: my-app
          resources:
            requests:
              cpu: 500m
              memory: 512Mi
            limits:
              cpu: "2"
              memory: 1Gi
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchLabels:
                    app.kubernetes.io/name: my-app
                topologyKey: kubernetes.io/hostname
```

```yaml
# overlays/prod/patches/hpa-patch.yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: my-app
spec:
  minReplicas: 3
  maxReplicas: 50
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 60
```

## 5. Base 提取原则

### 5.1 何时提取 Base

| 场景 | 策略 |
|------|------|
| 多环境部署 | 必须提取 |
| 多应用复用模板 | 提取为组件 |
| 单一环境 | 不需要 |

### 5.2 提取最佳实践

```yaml
# 好的 base 提取
# 包含：通用资源定义、默认值、标签
# 不包含：环境特定值、副本数、资源限制

# base/kustomization.yaml
resources:
  - deployment.yaml    # 包含默认副本数 1
  - service.yaml
  - configmap.yaml

# overlay 覆盖差异值
# overlays/prod/kustomization.yaml
patches:
  - patch: |
      apiVersion: apps/v1
      kind: Deployment
      metadata:
        name: my-app
      spec:
        replicas: 3
```

### 5.3 避免的反模式

```yaml
# 反模式 1：base 包含环境特定配置
# 错误
apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
data:
  DB_HOST: prod-db.example.com    # 不应在 base 中

# 反模式 2：overlay 重复定义资源
# 错误：overlay 完整定义 deployment.yaml 而非 patch

# 反模式 3：过度提取
# 错误：将每个字段都提取为单独的 patch
```

## 6. Overlay 继承机制

### 6.1 继承规则

```
Base 资源
    ↓ 加载
Overlay 资源（如果存在同名资源，Merge 策略）
    ↓ 应用 Patch
最终资源
```

### 6.2 多层 Overlay

```
base/
overlays/
  ├── dev/
  ├── staging/    # 可基于 dev
  │   └── resources:
  │       - ../dev    # 继承 dev 配置
  └── prod/
```

```yaml
# overlays/staging/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

# 继承 dev overlay
resources:
  - ../dev

# 覆盖差异
namespace: my-app-staging
namePrefix: staging-

patches:
  - patch: |
      apiVersion: apps/v1
      kind: Deployment
      metadata:
        name: dev-my-app
      spec:
        replicas: 2
```

## 7. 使用 Components

### 7.1 定义 Component

```yaml
# components/monitoring/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1alpha1
kind: Component

resources:
  - servicemonitor.yaml
  - prometheusrule.yaml
```

```yaml
# components/monitoring/servicemonitor.yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: my-app
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: my-app
  endpoints:
    - port: http
      path: /metrics
      interval: 30s
```

### 7.2 使用 Component

```yaml
# overlays/prod/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization

resources:
  - ../../base

components:
  - ../../components/monitoring
  - ../../components/logging

namespace: my-app-prod
```

## 8. 构建与验证

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看完整输出
kustomize build overlays/prod

# 应用到集群
kubectl apply -k overlays/prod

# 验证配置
kubectl kustomize overlays/prod | kubectl apply --dry-run=client -f -

# 查看差异
kubectl diff -k overlays/prod
```
## 9. 常见问题排查

```bash
# 问题 1：资源名冲突
# 检查 namePrefix/nameSuffix 是否正确
kustomize build overlays/dev | grep "name:"

# 问题 2：patch 未生效
# 检查 target 选择器是否匹配
kustomize build overlays/prod | grep -A 10 "kind: Deployment"

# 问题 3：ConfigMap 未更新
# 检查 behavior: merge/replace/create
kustomize build overlays/prod | grep -A 20 "kind: ConfigMap"
```

---

## Related

- [[Kustomize Transformers 参考|02-kustomize-transformers-reference]]
- [[Kustomize 高级特性与 GitOps|03-kustomize-remote-build-gitops]]

## See Also

- [Kustomize 官方文档](https://kubernetes.io/docs/tasks/manage-kubernetes-objects/kustomization/)
- [Kustomize GitHub](https://github.com/kubernetes-sigs/kustomize)


<!-- risk-assessed -->
