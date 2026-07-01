---
title: 'Day 23: 企业日志 + GitOps'
description: 'title: Day 23: 企业日志 + GitOps'
category: learning
tags:
- k8s
- training
- hands-on
- helm
- argocd
- opa
- redis
- kafka
- elasticsearch
- hpa
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 23: 企业日志 + GitOps 是什么'
- '如何 Day 23: 企业日志 + GitOps'
trigger_keywords:
- Day
- '23:'
- 企业日志
- GitOps
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- gitops-basics
- kafka-basics
- redis-basics
- policy-basics
- logging-basics
created: "2026-05-23"
---

---
title: Day 23: 企业日志 + GitOps
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - ELK 企业日志架构
  - [[ArgoCD|ArgoCD]] GitOps 实践
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] Kustomize
  - 多环境配置管理
trigger_keywords:
  - ELK
  - Loki
  - GitOps
  - ArgoCD
  - Kustomize
  - 多环境
  - 持续部署
  - 声明式
reading_level: intermediate
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 240min
related_domains:
  - domain-21-logging-management-analytics
  - domain-08-release-change-management
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-22-enterprise-monitoring
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-24-security-compliance
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p4-gitops-pipeline
---

# Day 23: 企业日志 + GitOps

> **学习时间**: 4-5 小时 | **主题**: ELK 日志 + ArgoCD GitOps

---

## 概述

企业级日志管理和 GitOps 是现代 Kubernetes 运维的两个关键能力。日志管理帮助你在分布式环境中快速定位问题，而 GitOps 则将基础设施和应用配置的管理标准化、自动化，确保所有变更都有迹可循、可审计、可回滚。

本课程将深入探讨两个核心主题：基于 ELK（Elasticsearch + Logstash + Kibana）的企业级日志方案，以及基于 ArgoCD 的 GitOps 持续交付实践。你将学习如何设计适合企业规模的日志架构，如何使用 Kustomize 管理多环境配置，以及如何通过 ArgoCD 实现声明式的持续部署。

**学习目标**：
- 了解 ELK 企业日志方案的架构设计
- 掌握 ArgoCD GitOps 实践和多环境管理
- 配置 Kustomize 多环境部署策略

**前置条件**：
- 已完成 Week 3 的监控告警学习
- 有 [[Helm|Helm]] 和 YAML 基础
- 了解 Deployment/Service/Ingress 等资源

---

## 核心概念

### ELK 企业日志架构

ELK Stack（Elasticsearch + Logstash + Kibana）是企业级日志管理的经典方案。在 Kubernetes 环境中，ELK 的部署架构需要考虑日志采集、传输、存储和展示四个环节。

#### ELK vs Loki 对比

| 维度 | ELK Stack | Loki Stack |
|------|-----------|------------|
| **存储引擎** | Elasticsearch（Lucene） | 对象存储/本地磁盘 |
| **查询语言** | KQL / Lucene | LogQL |
| **全文索引** | 支持（高精度搜索） | 不支持（仅标签索引） |
| **资源消耗** | 高（CPU/内存/存储） | 低 |
| **聚合分析** | 强大 | 基础 |
| **适用规模** | 大型企业、合规要求 | 中小型、运维为主 |
| **学习曲线** | 陡峭 | 平缓 |
| **成本** | 高 | 低 |

#### ELK Kubernetes 部署架构

```
Pod (stdout/stderr)
       │
       ▼
Filebeat (DaemonSet)  ──>  Kafka/Redis (缓冲)
                              │
                              ▼
                         Logstash (处理/转换)
                              │
                              ▼
                    Elasticsearch (存储+索引)
                              │
                              ▼
                         Kibana (可视化)
```

### GitOps 核心原则

GitOps 是一种现代化的持续交付方法，其核心原则包括：

1. **声明式**: 所有系统配置都是声明式的，描述"要什么"而非"怎么做"
2. **版本控制**: 所有配置存储在 Git 仓库中，有完整的变更历史
3. **自动拉取**: 系统状态通过自动拉取 Git 仓库的变更来更新
4. **持续协调**: 软件代理持续比较实际状态和期望状态，自动修正偏差

#### ArgoCD 工作流程

```
Git Repository ──> ArgoCD (Controller) ──> Kubernetes Cluster
     │                    │                        │
     │                    │                        │
     └── 监控变更 ←────────┘                        │
                          └── 比较期望 vs 实际 ──────┘
                               │
                          有差异则同步
```

### Kustomize 多环境管理

Kustomize 是 Kubernetes 原生的配置管理工具，通过 base + overlays 的模式管理不同环境的配置差异。

```
gitops-repo/
├── base/                  # 基础配置（所有环境共享）
│   ├── deployment.yaml
│   ├── service.yaml
│   ├── configmap.yaml
│   └── kustomization.yaml
├── overlays/
│   ├── dev/               # 开发环境覆盖
│   │   ├── kustomization.yaml
│   │   └── patches/
│   ├── staging/           # 预发环境覆盖
│   │   ├── kustomization.yaml
│   │   └── patches/
│   └── prod/              # 生产环境覆盖
│       ├── kustomization.yaml
│       └── patches/
└── argocd-apps/           # ArgoCD Application 定义
    ├── dev.yaml
    ├── staging.yaml
    └── prod.yaml
```

---

## 实战演练

### Step 1: 安装 ArgoCD (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# Step 1.1: 创建 namespace
kubectl create namespace argocd

# 预期输出:
# namespace/argocd created

# Step 1.2: 安装 ArgoCD
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/install.yaml

# 预期输出 (部分):
# customresourcedefinition.apiextensions.k8s.io/applications.argoproj.io created
# customresourcedefinition.apiextensions.k8s.io applicationsets.argoproj.io created
# serviceaccount/argocd-application-controller created
# serviceaccount/argocd-server created
# deployment.apps/argocd-repo-server created
# deployment.apps/argocd-server created
# deployment.apps/argocd-redis created

# Step 1.3: 等待就绪
kubectl wait --for=condition=available --timeout=300s deployment/argocd-server -n argocd

# 预期输出:
# deployment.apps/argocd-server condition met

# Step 1.4: 验证所有组件运行
kubectl get pods -n argocd

# 预期输出:
# NAME                                                READY   STATUS    RESTARTS   AGE
# argocd-application-controller-0                     1/1     Running   0          2m
# argocd-applicationset-controller-xxx                1/1     Running   0          2m
# argocd-dex-server-xxx                               1/1     Running   0          2m
# argocd-notifications-controller-xxx                 1/1     Running   0          2m
# argocd-redis-xxx                                    1/1     Running   0          2m
# argocd-repo-server-xxx                              1/1     Running   0          2m
# argocd-server-xxx                                   1/1     Running   0          2m

# Step 1.5: 获取初始密码
kubectl -n argocd get secret argocd-initial-admin-secret -o jsonpath="{.data.password}" | base64 -d && echo

# 预期输出:
# xYz123AbC456  (随机生成的密码)

# Step 1.6: 访问 UI
kubectl port-forward svc/argocd-server -n argocd 8080:443

# 浏览器访问: https://localhost:8080
# 用户名: admin
# 密码: 上一步获取的密码

# Step 1.7: 安装 argocd CLI
brew install argocd
# 或: curl -sSL -o /usr/local/bin/argocd https://github.com/argoproj/argo-cd/releases/latest/download/argocd-darwin-amd64

# Step 1.8: CLI 登录
argocd login localhost:8080 --username admin --password <password> --insecure

# 预期输出:
# 'admin:login' logged in successfully
# Context 'localhost:8080' updated
```

### Step 2: 创建 Git 仓库结构 (30min)

```bash
# Step 2.1: 创建目录结构
mkdir -p gitops-demo/{base,overlays/{dev,staging,prod},argocd-apps}

# Step 2.2: 创建基础 Deployment
cat > gitops-demo/base/deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
  labels:
    app: web-app
spec:
  selector:
    matchLabels:
      app: web-app
  template:
    metadata:
      labels:
        app: web-app
    spec:
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        fsGroup: 2000
      containers:
      - name: app
        image: nginx:1.25-alpine
        ports:
        - containerPort: 80
          name: http
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
        livenessProbe:
          httpGet:
            path: /
            port: http
          initialDelaySeconds: 10
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /
            port: http
          initialDelaySeconds: 5
          periodSeconds: 5
        volumeMounts:
        - name: tmp
          mountPath: /tmp
      volumes:
      - name: tmp
        emptyDir: {}
EOF

# Step 2.3: 创建基础 Service
cat > gitops-demo/base/service.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: web-app
  labels:
    app: web-app
spec:
  selector:
    app: web-app
  ports:
  - port: 80
    targetPort: http
    name: http
  type: ClusterIP
EOF

# Step 2.4: 创建基础 ConfigMap
cat > gitops-demo/base/configmap.yaml << 'EOF'
apiVersion: v1
kind: ConfigMap
metadata:
  name: web-app-config
data:
  APP_ENV: "base"
  LOG_LEVEL: "info"
EOF

# Step 2.5: 创建基础 kustomization.yaml
cat > gitops-demo/base/kustomization.yaml << 'EOF'
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
- deployment.yaml
- service.yaml
- configmap.yaml
commonLabels:
  app.kubernetes.io/name: web-app
  app.kubernetes.io/managed-by: argocd
EOF

# Step 2.6: 创建 dev 环境覆盖
cat > gitops-demo/overlays/dev/kustomization.yaml << 'EOF'
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
namespace: dev
resources:
- ../../base
replicas:
- name: web-app
  count: 1
patches:
- target: "`kind: Deployment`"
  patch: |
    - op: replace
      path: /spec/template/spec/containers/0/resources/limits/cpu
      value: 100m
    - op: replace
      path: /spec/template/spec/containers/0/resources/limits/memory
      value: 128Mi
- target: "`kind: ConfigMap`"
    name: web-app-config
  patch: |
    - op: replace
      path: /data/APP_ENV
      value: "development"
    - op: replace
      path: /data/LOG_LEVEL
      value: "debug"
EOF

# Step 2.7: 创建 staging 环境覆盖
cat > gitops-demo/overlays/staging/kustomization.yaml << 'EOF'
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
namespace: staging
resources:
- ../../base
replicas:
- name: web-app
  count: 2
patches:
- target: "`kind: ConfigMap`"
    name: web-app-config
  patch: |
    - op: replace
      path: /data/APP_ENV
      value: "staging"
    - op: replace
      path: /data/LOG_LEVEL
      value: "info"
EOF

# Step 2.8: 创建 prod 环境覆盖
cat > gitops-demo/overlays/prod/kustomization.yaml << 'EOF'
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
namespace: prod
resources:
- ../../base
replicas:
- name: web-app
  count: 3
patches:
- target: "`kind: Deployment`"
  patch: |
    - op: replace
      path: /spec/template/spec/containers/0/resources/requests/cpu
      value: 200m
    - op: replace
      path: /spec/template/spec/containers/0/resources/requests/memory
      value: 256Mi
    - op: replace
      path: /spec/template/spec/containers/0/resources/limits/cpu
      value: "1"
    - op: replace
      path: /spec/template/spec/containers/0/resources/limits/memory
      value: 512Mi
- target: "`kind: ConfigMap`"
    name: web-app-config
  patch: |
    - op: replace
      path: /data/APP_ENV
      value: "production"
    - op: replace
      path: /data/LOG_LEVEL
      value: "warn"
EOF

# Step 2.9: 验证配置渲染
kubectl kustomize gitops-demo/overlays/dev/
kubectl kustomize gitops-demo/overlays/prod/
```

### Step 3: 创建 ArgoCD Application (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# Step 3.1: 创建 dev 环境 Application
cat > gitops-demo/argocd-apps/dev.yaml << 'EOF'
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: web-app-dev
  namespace: argocd
  labels:
    environment: dev
spec:
  project: default
  source:
    repoURL: https://github.com/<your-org>/gitops-demo
    targetRevision: main
    path: overlays/dev
  destination:
    server: https://kubernetes.default.svc
    namespace: dev
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
      allowEmpty: false
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
    - /spec/replicas
EOF

# Step 3.2: 创建 prod 环境 Application
cat > gitops-demo/argocd-apps/prod.yaml << 'EOF'
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: web-app-prod
  namespace: argocd
  labels:
    environment: prod
  annotations:
    notifications.argoproj.io/subscribe.on-deployed.slack: deploy-notifications
    notifications.argoproj.io/subscribe.on-health-degraded.pagerduty: prod-oncall
spec:
  project: default
  source:
    repoURL: https://github.com/<your-org>/gitops-demo
    targetRevision: main
    path: overlays/prod
  destination:
    server: https://kubernetes.default.svc
    namespace: prod
  syncPolicy:
    automated:
      prune: false
      selfHeal: true
    syncOptions:
    - CreateNamespace=true
  ignoreDifferences:
  - group: apps
    kind: Deployment
    jsonPointers:
    - /spec/replicas
EOF

# Step 3.3: 应用 Application
kubectl apply -f gitops-demo/argocd-apps/dev.yaml
kubectl apply -f gitops-demo/argocd-apps/prod.yaml

# 预期输出:
# application.argoproj.io/web-app-dev created
# application.argoproj.io/web-app-prod created

# Step 3.4: 或使用 argocd CLI 创建
argocd app create web-app-dev \
  --repo https://github.com/<your-org>/gitops-demo \
  --path overlays/dev \
  --dest-server https://kubernetes.default.svc \
  --dest-namespace dev \
  --sync-policy automated \
  --auto-prune \
  --self-heal

# 预期输出:
# application 'web-app-dev' created
```

### Step 4: 验证 GitOps 工作流 (30min)

```bash
# Step 4.1: 查看应用状态
argocd app get web-app-dev

# 预期输出:
# Name:               argocd/web-app-dev
# Project:            default
# Server:             https://kubernetes.default.svc
# Namespace:          dev
# URL:                https://localhost:8080/applications/web-app-dev
# Source:             https://github.com/<your-org>/gitops-demo
# Path:               overlays/dev
# Sync Status:        Synced to main (abc1234)
# Health Status:      Healthy
#
# GROUP  KIND        NAMESPACE  NAME        STATUS  HEALTH   HOOK  MESSAGE
#        Service     dev        web-app     Synced  Healthy        service/web-app created
# apps   Deployment  dev        web-app     Synced  Healthy        deployment.apps/web-app created
#        ConfigMap   dev        web-app-config Synced  Healthy    configmap/web-app-config created

# Step 4.2: 查看所有应用列表
argocd app list

# 预期输出:
# NAME            CLUSTER                         NAMESPACE  PROJECT  STATUS  HEALTH   SYNCPOLICY  CONDITIONS
# web-app-dev     https://kubernetes.default.svc  dev        default  Synced  Healthy  Auto-Prune  <none>
# web-app-prod    https://kubernetes.default.svc  prod       default  Synced  Healthy  Auto        <none>

# Step 4.3: 手动同步
argocd app sync web-app-dev

# 预期输出:
# TIMESTAMP  GROUP  KIND       NAMESPACE  NAME             STATUS   RESULT
# 10:30:00   apps   Deployment  dev        web-app          Synced   configmap/web-app-config configured
# 10:30:01          Service     dev        web-app          Synced   service/web-app configured
# 10:30:02          ConfigMap   dev        web-app-config   Synced   deployment.apps/web-app configured

# Step 4.4: 查看同步历史
argocd app history web-app-dev

# 预期输出:
# ID  DATE                           REVISION
# 1   2026-05-18 10:30:00 +0000 UTC  abc1234 (main)
# 2   2026-05-18 11:15:00 +0000 UTC  def5678 (main)

# Step 4.5: 回滚到之前的版本
argocd app rollback web-app-dev 1

# 预期输出:
# web-app-dev rollback to 1

# Step 4.6: 测试自动同步（修改 Git 并 push）
# 修改 overlays/dev/kustomization.yaml 中的副本数
# ArgoCD 会在 syncPolicy 配置的间隔内自动检测并同步

# Step 4.7: 查看 ArgoCD 应用详细状态
kubectl get application web-app-dev -n argocd -o yaml
```

---

## 配置参考

### ArgoCD ApplicationSet（多集群/多环境）

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: web-app
  namespace: argocd
spec:
  generators:
  - list:
      elements:
      - cluster: https://kubernetes.default.svc
        name: dev
        namespace: dev
        replicas: 1
        resources_cpu: 100m
        resources_memory: 128Mi
      - cluster: https://kubernetes.default.svc
        name: staging
        namespace: staging
        replicas: 2
        resources_cpu: 200m
        resources_memory: 256Mi
      - cluster: https://prod-cluster.example.com
        name: prod
        namespace: prod
        replicas: 3
        resources_cpu: "1"
        resources_memory: 512Mi
  template:
    metadata:
      name: 'web-app-{{name}}'
      labels:
        environment: '{{name}}'
    spec:
      project: default
      source:
        repoURL: https://github.com/<your-org>/gitops-demo
        targetRevision: main
        path: 'overlays/{{name}}'
      destination:
        server: '{{cluster}}'
        namespace: '{{namespace}}'
      syncPolicy:
        automated:
          prune: true
          selfHeal: true
        syncOptions:
        - CreateNamespace=true
```

### ArgoCD SyncPolicy 参数说明

| 参数 | 说明 | dev 推荐 | prod 推荐 |
|------|------|---------|----------|
| `automated.prune` | 自动删除多余资源 | true | false（手动确认） |
| `automated.selfHeal` | 自动修复配置漂移 | true | true |
| `automated.allowEmpty` | 允许删除所有资源 | false | false |
| `syncOptions.CreateNamespace` | 自动创建命名空间 | true | true |
| `syncOptions.PruneLast` | 最后删除资源 | false | true |
| `retry.limit` | 重试次数 | 3 | 5 |
| `retry.backoff.duration` | 初始退避时间 | 5s | 5s |
| `retry.backoff.factor` | 退避倍数 | 2 | 2 |
| `retry.backoff.maxDuration` | 最大退避时间 | 1m | 3m |

### ELK Helm Values 参考

```yaml
elasticsearch:
  enabled: true
  replicas: 3
  resources:
    requests:
      cpu: "1"
      memory: 2Gi
    limits:
      cpu: "2"
      memory: 4Gi
  persistence:
    enabled: true
    size: 100Gi
    storageClass: alicloud-disk-ssd
  esJavaOpts: "-Xms1g -Xmx1g"
  extraEnvs:
  - name: ES_JAVA_OPTS
    value: "-Xms1g -Xmx1g"

logstash:
  enabled: true
  replicas: 2
  resources:
    requests:
      cpu: 500m
      memory: 1Gi
    limits:
      cpu: "1"
      memory: 2Gi

kibana:
  enabled: true
  resources:
    requests:
      cpu: 200m
      memory: 512Mi
    limits:
      cpu: 500m
      memory: 1Gi
  ingress:
    enabled: true
    annotations:
      kubernetes.io/ingress.class: nginx
    hosts:
    - kibana.example.com

filebeat:
  enabled: true
  filebeatConfig:
    filebeat.yml: |
      filebeat.autodiscover:
        providers:
        - type: kubernetes
          hints.enabled: true
      output.elasticsearch:
        hosts: ["elasticsearch-master:9200"]
      logging.level: info

```

---

## 常见问题

### Q1: GitOps 的核心原则是什么？

**A**: GitOps 有四个核心原则：
1. **声明式系统描述**: 所有配置都是声明式的 YAML
2. **Git 作为唯一可信源**: Git 仓库是期望状态的唯一定义
3. **自动拉取变更**: 系统自动从 Git 拉取变更（不是 Push）
4. **持续协调**: Agent 持续比较 Git 状态和集群状态，自动修正偏差

### Q2: ArgoCD 的 Sync Policy 中 automated vs manual 的区别？

**A**:
- **automated**: Git 变更后自动同步到集群，适合 dev/staging 环境
- **manual**: 需要 `argocd app sync` 手动触发同步，适合需要审批的生产环境
- **最佳实践**: dev 用 automated（快速验证），prod 用 manual 或 automated + 审批流程

### Q3: 如何使用 Kustomize 管理多环境配置差异？

**A**: 使用 base + overlays 模式：
- **base**: 定义所有环境共享的基础配置
- **overlays/dev**: 开发环境覆盖（1 副本、debug 日志、小资源）
- **overlays/staging**: 预发环境覆盖（2 副本、info 日志）
- **overlays/prod**: 生产环境覆盖（3+ 副本、warn 日志、大资源、HPA）
- 每个环境通过 patches 修改差异部分，避免重复配置

### Q4: ArgoCD 应用一直 OutOfSync 怎么办？

**A**: 排查步骤：
1. `argocd app diff <app-name>` 查看具体差异
2. 检查是否是 Helm/Kustomize 渲染差异（生成器产生的随机值）
3. 使用 `ignoreDifferences` 忽略不需要比较的字段
4. 检查是否有人手动修改了集群资源（kubectl apply 直接操作）
5. 考虑启用 `selfHeal` 自动修正配置漂移

### Q5: 生产环境 ArgoCD 的最佳实践是什么？

**A**:
1. **使用 App of Apps 模式**: 用一个 ArgoCD Application 管理所有其他 Application
2. **启用 RBAC**: 限制不同团队只能操作自己的 Application
3. **配置 Webhook**: Git 仓库变更时立即通知 ArgoCD，减少同步延迟
4. **使用 ApplicationSet**: 自动化多集群/多环境的 Application 创建
5. **配置通知**: 使用 ArgoCD Notifications 发送部署状态通知
6. **备份 ArgoCD 配置**: 定期备份 argocd namespace 中的资源

---

## 要点总结

- **GitOps** 的核心是 Git 作为唯一可信源，所有变更通过 Git PR 触发
- **ArgoCD** 是 Kubernetes 原生的 GitOps 工具，支持自动同步和配置漂移检测
- **Kustomize** 通过 base + overlays 模式管理多环境配置，避免 YAML 重复
- **ELK** 适合需要全文搜索和复杂聚合分析的企业日志场景
- **App of Apps** 和 **ApplicationSet** 是管理大量 ArgoCD 应用的推荐模式
- 生产环境建议 **手动同步 + 审批流程**，开发环境可以自动同步

---

## 延伸阅读

- [ArgoCD 官方文档](https://argo-cd.readthedocs.io/)
- [GitOps 原则](https://opengitops.dev/)
- [Kustomize 官方文档](https://kustomize.io/)
- [ELK on Kubernetes](https://www.elastic.co/guide/en/cloud-on-k8s/current/index.html)
- [文件: `../../domain-06-observability/01-elk-stack-enterprise-logging.md`](../../domain-06-observability/01-elk-stack-enterprise-logging.md)
- [文件: `../../domain-08-release-change-management/01-argo-cd-enterprise-gitops.md`](../../domain-08-release-change-management/01-argo-cd-enterprise-gitops.md)

```