# 项目 P1: 从零搭建 K8s 集群

> **所属周**: Week 1 | **预计时间**: 2.5 小时

---

## 项目目标

从零开始搭建一个可运行 nginx 的 Kubernetes 集群，完成后能够:
- 独立搭建本地 K8s 集群
- 创建 Namespace、Deployment、Service
- 使用 kubectl 进行基本操作和调试

---

## 前置条件

- 已完成 Week 1 Day 1-6 的学习
- 本机已安装 Docker
- 熟悉基本的 kubectl 命令

---

## 项目步骤

### Step 1: 安装 kind 并创建集群 (30min)

```bash
# 安装 kind
# macOS
brew install kind

# Linux
curl -Lo ./kind https://kind.sigs.k8s.io/dl/v0.20.0/kind-linux-amd64
chmod +x ./kind
sudo mv ./kind /usr/local/bin/kind

# 创建多节点集群
cat > kind-config.yaml << 'EOF'
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
- role: worker
- role: worker
EOF

kind create cluster --name learn-k8s --config kind-config.yaml

# 验证集群
kubectl cluster-info
kubectl get nodes
```

### Step 2: 创建 Namespace (10min)

```bash
# 创建 namespace
kubectl create namespace web-app

# 验证
kubectl get namespaces

# 设置默认 namespace (可选)
kubectl config set-context --current --namespace=web-app
```

### Step 3: 部署 Deployment (30min)

```bash
# 创建 Deployment YAML
cat > deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-web
  namespace: web-app
  labels:
    app: nginx-web
spec:
  replicas: 3
  selector:
    matchLabels:
      app: nginx-web
  template:
    metadata:
      labels:
        app: nginx-web
    spec:
      containers:
      - name: nginx
        image: nginx:alpine
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

kubectl apply -f deployment.yaml

# 验证
kubectl get deployment -n web-app
kubectl get pods -n web-app -w
```

### Step 4: 创建 Service (20min)

```bash
# 创建 Service YAML
cat > service.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: nginx-service
  namespace: web-app
spec:
  selector:
    app: nginx-web
  ports:
  - port: 80
    targetPort: 80
  type: NodePort
EOF

kubectl apply -f service.yaml

# 验证
kubectl get svc -n web-app
kubectl get endpoints -n web-app
```

### Step 5: 测试和调试 (30min)

```bash
# 测试 Service
kubectl run curl --image=curlimages/curl -n web-app -it --rm -- curl nginx-service

# 查看 Pod 日志
kubectl logs -l app=nginx-web -n web-app

# 进入 Pod 调试
kubectl exec -it $(kubectl get pod -l app=nginx-web -n web-app -o jsonpath='{.items[0].metadata.name}') -n web-app -- sh

# 查看事件
kubectl get events -n web-app --sort-by='.lastTimestamp'

# 模拟故障排查
kubectl set image deployment/nginx-web nginx=nginx:nonexistent -n web-app
kubectl get pods -n web-app
kubectl describe pod <pod-name> -n web-app
kubectl rollout undo deployment/nginx-web -n web-app
```

### Step 6: 文档输出 (30min)

创建 `~/k8s-setup-doc.md`，记录:

1. 集群搭建步骤
2. 部署的资源清单
3. 遇到的问题和解决方法
4. 常用命令速查

---

## 验收清单

- [ ] 集群成功创建，3 个节点正常运行
- [ ] Namespace 创建成功
- [ ] Deployment 部署成功，3 个 Pod 运行正常
- [ ] Service 创建成功，Endpoints 非空
- [ ] 能够通过 Service 访问 nginx
- [ ] 能够查看 Pod 日志
- [ ] 能够进入 Pod 进行调试
- [ ] 完成搭建文档

---

## 清理资源

```bash
kubectl delete namespace web-app
kind delete cluster --name learn-k8s
```
