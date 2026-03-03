# 项目 P4: GitOps 流水线

> **所属周**: Week 4 | **预计时间**: 2 小时

---

## 项目目标

使用 ArgoCD 搭建 GitOps 流水线:
- 部署 ArgoCD
- 创建 GitOps 仓库结构
- 配置多环境部署
- 实现声明式持续交付

---

## 前置条件

- 已完成 Week 4 Day 22-23 的学习
- 有 Git 仓库 (GitHub/GitLab)
- 了解 Kustomize 基础

---

## 项目步骤

### Step 1: 安装 ArgoCD (20min)

```bash
# 创建 namespace
kubectl create namespace argocd

# 安装 ArgoCD
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/stable/manifests/install.yaml

# 等待就绪
kubectl wait --for=condition=available --timeout=300s deployment/argocd-server -n argocd

# 获取初始密码
ARGOCD_PASSWORD=$(kubectl -n argocd get secret argocd-initial-admin-secret -o jsonpath="{.data.password}" | base64 -d)
echo "ArgoCD admin password: $ARGOCD_PASSWORD"

# 访问 UI
kubectl port-forward svc/argocd-server -n argocd 8080:443

# 登录: admin / $ARGOCD_PASSWORD
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
EOF

# 推送到 Git 仓库
cd gitops-demo
git init
git add .
git commit -m "Initial GitOps structure"
git remote add origin <your-repo-url>
git push -u origin main
```

### Step 3: 创建 ArgoCD Application (20min)

```bash
# 创建 dev 环境 Application
cat > argocd-app-dev.yaml << 'EOF'
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: demo-dev
  namespace: argocd
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

# 创建 prod 环境 Application (手动同步)
cat > argocd-app-prod.yaml << 'EOF'
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: demo-prod
  namespace: argocd
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
EOF

kubectl apply -f argocd-app-prod.yaml
```

### Step 4: 验证 GitOps 工作流 (20min)

```bash
# 查看 Application 状态
kubectl get applications -n argocd

# 在 ArgoCD UI 中查看同步状态

# 修改 Git 仓库中的配置
# 例如: 更新镜像版本为 nginx:1.25-alpine

# dev 环境会自动同步
# prod 环境需要手动同步

# 使用 CLI 同步
argocd app sync demo-prod

# 查看部署结果
kubectl get pods -n dev
kubectl get pods -n prod
```

### Step 5: 测试回滚 (10min)

```bash
# 查看历史
argocd app history demo-dev

# 回滚到指定版本
argocd app rollback demo-dev <revision>
```

---

## 验收清单

- [ ] ArgoCD 安装成功
- [ ] Git 仓库结构创建完成
- [ ] dev 环境自动同步正常
- [ ] prod 环境手动同步正常
- [ ] 修改 Git 仓库能触发同步
- [ ] 回滚功能正常

---

## 最佳实践

1. **分支策略**
   - main/master: 生产环境
   - develop: 开发环境
   - feature/*: 功能分支

2. **同步策略**
   - dev: automated + selfHeal
   - staging: automated + 需要 PR 审批
   - prod: manual + 需要审批

3. **安全考虑**
   - 使用 SSH key 或 Token 认证
   - 限制 ArgoCD 访问的仓库
   - 启用 SSO 集成

---

## 清理资源

```bash
kubectl delete application demo-dev demo-prod -n argocd
kubectl delete namespace dev staging prod
```
