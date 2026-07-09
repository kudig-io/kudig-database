---
title: '项目 P4: GitOps 流水线'
description: '- argocd gitops 完整部署配置'
summary: '- argocd gitops 完整部署配置'
category: learning
tags:
- k8s
- training
- hands-on
- argocd
- redis
- rbac
- crd
- webhook
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- '项目 P4: GitOps 流水线 是什么'
- '如何 项目 P4: GitOps 流水线'
trigger_keywords:
- 项目
- 'P4:'
- GitOps
- 流水线
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- gitops-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: 项目 P4: GitOps 流水线
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[ArgoCD|argocd]] gitops 完整部署配置
  - kustomize 多环境管理 base overlays
  - argocd application 同步策略配置
  - 多集群 gitops 部署方案
trigger_keywords:
  - ArgoCD
  - GitOps
  - Kustomize
  - Application
  - SyncPolicy
  - multi-environment
  - base
  - overlays
  - 自动同步
  - 回滚
reading_level: intermediate
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 120min
related_domains:
  - 发布变更
  - 生产运维
related_topics:
  - 生产运维/topic-learn/public-training/one-month/week-4-enterprise/day-23-logging-gitops
  - 生产运维/topic-learn/public-training/one-month/projects/p5-graduation-project
---

# 项目 P4: GitOps 流水线

> **所属周**: Week 4 | **预计时间**: 2 小时

---

## 概述

本实践项目要求你使用 ArgoCD 搭建一个完整的 GitOps 流水线，包括多环境部署（dev/staging/prod）、自动同步和手动审批策略、以及回滚操作。GitOps 是现代云原生应用部署的推荐模式，理解其工作原理对于构建自动化、可审计的部署流程至关重要。

### 项目目标

使用 ArgoCD 搭建 GitOps 流水线：
- 部署 ArgoCD 并完成初始配置
- 创建 GitOps 仓库结构（base + overlays）
- 配置多环境部署（dev 自动同步、prod 手动审批）
- 实现声明式持续交付和回滚

### 前置条件

- 已完成 Week 4 Day 22-23 的学习
- 有 Git 仓库（GitHub/GitLab）
- 了解 Kustomize 基础

---

## 核心概念回顾

### GitOps 核心原则

GitOps 遵循以下四个核心原则：

1. **声明式**: 所有系统配置用声明式语言描述（如 YAML）
2. **版本控制**: 所有配置存储在 Git 仓库中，有完整的变更历史
3. **自动部署**: 系统能够自动将 Git 中的配置应用到目标环境
4. **持续协调**: 软件代理持续比对实际状态和期望状态，自动纠正偏差

### Kustomize 目录结构

```
gitops-demo/
├── base/                    # 基础配置（所有环境共享）
│   ├── deployment.yaml
│   ├── service.yaml
│   └── kustomization.yaml
└── overlays/                # 环境覆盖
    ├── dev/
    │   └── kustomization.yaml
    ├── staging/
    │   └── kustomization.yaml
    └── prod/
        ├── kustomization.yaml
        └── replicas-patch.yaml
```

### ArgoCD Application CRD 详解

ArgoCD 的核心资源是 Application CRD，它定义了：
- **source**: 配置来源（Git 仓库路径 + 分支）
- **destination**: 部署目标（集群 + 命名空间）
- **syncPolicy**: 同步策略（自动/手动）
- **ignoreDifferences**: 忽略的字段差异

---

## 项目步骤

### Step 1: 安装 ArgoCD (20min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 namespace
kubectl create namespace argocd
# 预期输出: namespace/argocd created

# 安装 ArgoCD
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/install.yaml
# 预期输出: 大量资源创建（customresourcedefinition, service, deployment 等）

# 等待所有组件就绪
kubectl wait --for=condition=available --timeout=300s deployment/argocd-server -n argocd
kubectl wait --for=condition=available --timeout=300s deployment/argocd-repo-server -n argocd
kubectl wait --for=condition=available --timeout=300s deployment/argocd-application-controller -n argocd
# 预期输出: deployment.apps/argocd-server condition met

# 查看所有 ArgoCD 组件
kubectl get pods -n argocd
# 预期输出:
# NAME                                                READY   STATUS    RESTARTS   AGE
# argocd-application-controller-0                     1/1     Running   0          2m
# argocd-applicationset-controller-789...             1/1     Running   0          2m
# argocd-dex-server-6b9...                            1/1     Running   0          2m
# argocd-notifications-controller-5c7...              1/1     Running   0          2m
# argocd-redis-6f7...                                 1/1     Running   0          2m
# argocd-repo-server-8d6...                           1/1     Running   0          2m
# argocd-server-5b9...                                1/1     Running   0          2m

# 获取初始密码
ARGOCD_PASSWORD=$(kubectl -n argocd get secret argocd-initial-admin-secret -o jsonpath="{.data.password}" | base64 -d)
echo "ArgoCD admin password: $ARGOCD_PASSWORD"

# 访问 UI
kubectl port-forward svc/argocd-server -n argocd 8080:443
# 预期输出: Forwarding from 127.0.0.1:8080 -> 8080

# 浏览器访问: https://localhost:8080
# 用户名: admin
# 密码: $ARGOCD_PASSWORD

# 安装 ArgoCD CLI（可选）
# macOS: brew install argocd
# Linux: curl -sLO https://github.com/argoproj/argo-cd/releases/latest/download/argocd-linux-amd64 && \
#        sudo install -m 555 argocd-linux-amd64 /usr/local/bin/argocd

# 使用 CLI 登录
argocd login localhost:8080 --username admin --password "$ARGOCD_PASSWORD" --insecure
# 预期输出: 'admin' logged in successfully
```
### Step 2: 创建 GitOps 仓库结构 (30min)

```bash
# 创建本地目录结构
mkdir -p gitops-demo/{base,overlays/{dev,staging,prod}}

# base/deployment.yaml
cat > gitops-demo/base/deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: demo-app
spec:
  selector:
    matchLabels:
      app: demo
  template:
    metadata:
      labels:
        app: demo
    spec:
      containers:
      - name: app
        image: nginx:1.24-alpine
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: 50m
            memory: 64Mi
          limits:
            cpu: 100m
            memory: 128Mi
        livenessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 5
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 3
          periodSeconds: 5
EOF

# base/service.yaml
cat > gitops-demo/base/service.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: demo-app
spec:
  selector:
    app: demo
  ports:
  - port: 80
    targetPort: 80
EOF

# base/kustomization.yaml
cat > gitops-demo/base/kustomization.yaml << 'EOF'
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
resources:
- deployment.yaml
- service.yaml
EOF

# overlays/dev/kustomization.yaml
cat > gitops-demo/overlays/dev/kustomization.yaml << 'EOF'
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
namespace: dev
resources:
- ../../base
replicas:
- name: demo-app
  count: 1
patches:
- target: "`kind: Deployment`"
  patch: |
    - op: replace
      path: /spec/template/spec/containers/0/image
      value: nginx:1.25-alpine
EOF

# overlays/staging/kustomization.yaml
cat > gitops-demo/overlays/staging/kustomization.yaml << 'EOF'
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
namespace: staging
resources:
- ../../base
replicas:
- name: demo-app
  count: 2
EOF

# overlays/prod/kustomization.yaml
cat > gitops-demo/overlays/prod/kustomization.yaml << 'EOF'
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
namespace: prod
resources:
- ../../base
replicas:
- name: demo-app
  count: 3
patches:
- target: "`kind: Deployment`"
  patch: |
    - op: add
      path: /spec/template/spec/containers/0/resources/limits/memory
      value: 256Mi
EOF

# 推送到 Git 仓库
cd gitops-demo
git init
git add .
git commit -m "Initial GitOps structure"
git remote add origin <your-repo-url>
git push -u origin main
# 预期输出:
# To <your-repo-url>
#  * [new branch]      main -> main
```

### Step 3: 创建 ArgoCD Application (20min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 dev 环境 Application（自动同步）
cat > argocd-app-dev.yaml << 'EOF'
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: demo-dev
  namespace: argocd
  labels:
    environment: dev
spec:
  project: default
  source:
    repoURL: https://github.com/<your-username>/gitops-demo
    targetRevision: HEAD
    path: overlays/dev
  destination:
    server: https://kubernetes.default.svc
    namespace: dev
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
    - CreateNamespace=true
EOF

kubectl apply -f argocd-app-dev.yaml
# 预期输出: application.argoproj.io/demo-dev created

# 创建 prod 环境 Application（手动同步）
cat > argocd-app-prod.yaml << 'EOF'
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: demo-prod
  namespace: argocd
  labels:
    environment: prod
  finalizers:
  - resources-finalizer.argocd.argoproj.io
spec:
  project: default
  source:
    repoURL: https://github.com/<your-username>/gitops-demo
    targetRevision: HEAD
    path: overlays/prod
  destination:
    server: https://kubernetes.default.svc
    namespace: prod
  syncPolicy:
    syncOptions:
    - CreateNamespace=true
  ignoreDifferences:
  - group: apps
    kind: Deployment
    jsonPointers:
    - /spec/replicas
EOF

kubectl apply -f argocd-app-prod.yaml
# 预期输出: application.argoproj.io/demo-prod created

# 查看 Application 状态
kubectl get applications -n argocd
# 预期输出:
# NAME       SYNC STATUS   HEALTH STATUS
# demo-dev   Synced        Healthy
# demo-prod  OutOfSync     Healthy
```
### Step 4: 验证 GitOps 工作流 (20min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Application 状态
kubectl get applications -n argocd
# 预期输出:
# NAME       SYNC STATUS   HEALTH STATUS
# demo-dev   Synced        Healthy
# demo-prod  OutOfSync     Healthy

# 查看 dev 环境 Pod
kubectl get pods -n dev
# 预期输出:
# NAME                        READY   STATUS    RESTARTS   AGE
# demo-app-6d4f7b8c9d-abc12   1/1     Running   0          2m

# prod 环境需要手动同步
argocd app sync demo-prod
# 预期输出:
# SYNC    REVISION                         STATUS
# SYNCED  abc123def456...                  Synced

kubectl get pods -n prod
# 预期输出:
# NAME                        READY   STATUS    RESTARTS   AGE
# demo-app-6d4f7b8c9d-abc12   1/1     Running   0          30s
# demo-app-6d4f7b8c9d-def34   1/1     Running   0          30s
# demo-app-6d4f7b8c9d-ghi56   1/1     Running   0          30s

# 修改 Git 仓库中的配置（测试自动同步）
# 例如: 更新镜像版本
cd gitops-demo
sed -i 's/nginx:1.24-alpine/nginx:1.25-alpine/' base/deployment.yaml
git add .
git commit -m "Update nginx to 1.25-alpine"
git push

# 等待 ArgoCD 检测到变化（默认每 3 分钟检查一次）
# dev 环境会自动同步
# prod 环境需要手动同步

# 查看 dev 环境 Pod 更新
kubectl get pods -n dev -w
# 预期输出: 新 Pod 使用 nginx:1.25-alpine 镜像

# 手动同步 prod
argocd app sync demo-prod
```
### Step 5: 测试回滚 (10min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看应用历史
argocd app history demo-dev
# 预期输出:
# ID  DATE                           REVISION
# 1   2024-01-15 10:00:00 +0000 UTC  abc123d (main)
# 2   2024-01-15 10:30:00 +0000 UTC  def456g (main)

# 回滚到指定版本
argocd app rollback demo-dev 1
# 预期输出: Rollback to revision abc123d

# 或者使用 Git 回滚（推荐，因为 GitOps 以 Git 为真实来源）
cd gitops-demo
git revert HEAD
git push
# dev 环境自动同步到回滚后的状态

# 验证回滚结果
kubectl get pods -n dev -o jsonpath='{.items[0].spec.containers[0].image}'
# 预期输出: nginx:1.24-alpine（回滚后的镜像版本）
```
---

## 配置示例

### ArgoCD ApplicationSet（多环境自动生成）

```yaml
apiVersion: argoproj.io/v1alpha1
kind: ApplicationSet
metadata:
  name: demo-app
  namespace: argocd
spec:
  generators:
  - list:
      elements:
      - env: dev
        namespace: dev
        replicas: "1"
        autoSync: "true"
      - env: staging
        namespace: staging
        replicas: "2"
        autoSync: "true"
      - env: prod
        namespace: prod
        replicas: "3"
        autoSync: "false"
  template:
    metadata:
      name: "demo-{{env}}"
      labels:
        environment: "{{env}}"
    spec:
      project: default
      source:
        repoURL: https://github.com/<your-username>/gitops-demo
        targetRevision: HEAD
        path: "overlays/{{env}}"
      destination:
        server: https://kubernetes.default.svc
        namespace: "{{namespace}}"
      syncPolicy:
        automated:
          prune: true
          selfHeal: "{{autoSync}}"
        syncOptions:
        - CreateNamespace=true
```

### ArgoCD Project（多团队资源隔离）

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AppProject
metadata:
  name: team-frontend
  namespace: argocd
spec:
  description: "Frontend team project"
  sourceRepos:
  - "https://github.com/frontend-team/*"
  destinations:
  - namespace: "frontend-*"
    server: https://kubernetes.default.svc
  clusterResourceWhitelist:
  - group: ""
    kind: Namespace
  namespaceResourceBlacklist:
  - group: ""
    kind: ResourceQuota
  roles:
  - name: developer
    policies:
    - p, proj:team-frontend:developer, applications, get, team-frontend/*, allow
    - p, proj:team-frontend:developer, applications, sync, team-frontend/*, allow

```

---

## 常见问题

### Q1: ArgoCD 检测 Git 变更的频率是多少？

默认每 3 分钟轮询一次 Git 仓库。可以通过设置 `syncPolicy.automated.prune` 和 webhook 来加速。推荐配置 Git webhook 实现实时通知：在 Git 仓库中配置 webhook URL 指向 ArgoCD 的 `/api/webhook` 端点。

### Q2: dev 环境自动同步会带来风险吗？

自动同步配合 selfHeal 可以确保集群状态始终与 Git 一致。但如果有手动修改（如 kubectl edit），ArgoCD 会自动将其恢复为 Git 中的状态。建议通过 RBAC 限制直接操作集群的权限，所有变更都通过 Git 提交。

### Q3: 如何处理 Secret 的 GitOps 管理？

Secret 不应直接以明文存储在 Git 中。推荐方案：1）使用 Sealed Secret（加密后存储在 Git 中，集群中自动解密）；2）使用 [[SOPS|SOPS]]（Mozilla SOPS 加密文件）；3）使用 Vault 配合 ArgoCD 的 Vault Plugin。

### Q4: 如何实现 GitOps 的多集群管理？

ArgoCD 支持多集群管理。首先通过 `argocd cluster add` 注册多个集群，然后在 Application 的 `destination.server` 中指定目标集群的 API Server 地址。可以使用 ApplicationSet 自动为多个集群生成 Application。

---

## 验收清单

- [ ] ArgoCD 安装成功，所有组件 Running
- [ ] Git 仓库结构创建完成（base + overlays）
- [ ] dev 环境自动同步正常
- [ ] prod 环境手动同步正常
- [ ] 修改 Git 仓库能触发自动同步（dev）
- [ ] 回滚功能正常
- [ ] 了解 ApplicationSet 和 Project 的使用

---

## 要点总结

| 概念 | 说明 | 关键配置 |
|------|------|---------|
| Application | ArgoCD 核心资源，定义部署源和目标 | source + destination + syncPolicy |
| Kustomize | 环境配置差异管理 | base + overlays |
| Sync Policy | 自动同步策略 | automated.prune + selfHeal |
| ApplicationSet | 多环境/多集群自动生成 | generators + template |
| Project | 多团队资源隔离 | sourceRepos + destinations + roles |

---

## 清理资源

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl delete`：删除资源（可由声明式清单重建）

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
kubectl delete application demo-dev demo-prod -n argocd
kubectl delete namespace dev staging prod  # ⚠️ 不可逆：永久删除命名空间及全部资源
# 卸载 ArgoCD（可选）
# kubectl delete namespace argocd  # ⚠️ 不可逆：永久删除命名空间及全部资源
```
---

## 延伸阅读

- [ArgoCD 企业级 GitOps](../../发布变更/01-argo-cd-enterprise-gitops.md)
- [Kustomize 基础](https://kubectl.docs.kubernetes.io/guides/introduction/kustomize/)
- [ArgoCD 官方文档](https://argo-cd.readthedocs.io/)
- [Sealed Secret](https://github.com/bitnami-labs/sealed-secrets)

## Related

- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

```

<!-- risk-assessed -->
