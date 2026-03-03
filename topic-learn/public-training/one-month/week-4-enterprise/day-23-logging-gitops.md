# Day 23: 企业日志 + GitOps

> **学习时间**: 4-5 小时 | **主题**: ELK 日志 + ArgoCD GitOps

---

## 今日目标

- [ ] 了解 ELK 企业日志方案
- [ ] 掌握 ArgoCD GitOps 实践
- [ ] 配置多环境部署

---

## 理论学习 (2h)

### 必读文档

1. **ELK 企业日志**
   - 文件: `../../domain-21-logging-management-analytics/01-elk-stack-enterprise-logging.md`
   - 重点: 架构设计、日志规范

2. **ArgoCD 企业 GitOps**
   - 文件: `../../domain-23-gitops-ci-cd/01-argo-cd-enterprise-gitops.md`
   - 重点: 声明式部署、多环境管理

---

## 实践任务 - 项目 P4: GitOps 流水线 (2.5h)

详细指南见: [../projects/p4-gitops-pipeline.md](../projects/p4-gitops-pipeline.md)

### Step 1: 安装 ArgoCD (30min)

```bash
# 创建 namespace
kubectl create namespace argocd

# 安装 ArgoCD
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/install.yaml

# 等待就绪
kubectl wait --for=condition=available --timeout=300s deployment/argocd-server -n argocd

# 获取初始密码
kubectl -n argocd get secret argocd-initial-admin-secret -o jsonpath="{.data.password}" | base64 -d

# 访问 UI
kubectl port-forward svc/argocd-server -n argocd 8080:443
```

### Step 2: 创建 Git 仓库结构 (30min)

```bash
# 推荐的 GitOps 仓库结构
mkdir -p gitops-demo/{base,overlays/{dev,staging,prod}}

# base/deployment.yaml
cat > gitops-demo/base/deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app
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
        image: nginx:alpine
        ports:
        - containerPort: 80
EOF

# base/service.yaml
cat > gitops-demo/base/service.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: app
spec:
  selector:
    app: demo
  ports:
  - port: 80
EOF

# base/kustomization.yaml
cat > gitops-demo/base/kustomization.yaml << 'EOF'
resources:
- deployment.yaml
- service.yaml
EOF

# overlays/dev/kustomization.yaml
cat > gitops-demo/overlays/dev/kustomization.yaml << 'EOF'
namespace: dev
resources:
- ../../base
replicas:
- name: app
  count: 1
EOF

# overlays/prod/kustomization.yaml
cat > gitops-demo/overlays/prod/kustomization.yaml << 'EOF'
namespace: prod
resources:
- ../../base
replicas:
- name: app
  count: 3
EOF
```

### Step 3: 创建 ArgoCD Application (30min)

```bash
# 创建 Application
cat > argocd-app.yaml << 'EOF'
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: demo-dev
  namespace: argocd
spec:
  project: default
  source:
    repoURL: https://github.com/<your-repo>/gitops-demo
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

kubectl apply -f argocd-app.yaml

# 或使用 argocd CLI
argocd app create demo-dev \
  --repo https://github.com/<your-repo>/gitops-demo \
  --path overlays/dev \
  --dest-server https://kubernetes.default.svc \
  --dest-namespace dev \
  --sync-policy automated
```

### Step 4: 验证 GitOps 工作流 (30min)

```bash
# 查看应用状态
argocd app get demo-dev

# 修改 Git 仓库中的配置
# ArgoCD 会自动检测并同步

# 手动同步
argocd app sync demo-dev

# 回滚
argocd app history demo-dev
argocd app rollback demo-dev <revision>
```

---

## 费曼复述 (0.5h)

1. **GitOps 的核心原则是什么？**
2. **ArgoCD 的 Sync Policy 中 automated vs manual 的区别？**
3. **如何使用 Kustomize 管理多环境配置？**

---

## 今日检验

- [ ] 能够部署和配置 ArgoCD
- [ ] 能够创建 GitOps 仓库结构
- [ ] 能够配置多环境部署
