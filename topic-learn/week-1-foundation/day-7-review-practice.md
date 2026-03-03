# Day 7: 周复习 + 综合实践

> **学习时间**: 4-5 小时 | **主题**: Week 1 总结与实践项目

---

## 今日目标

- [ ] 复习本周所学，构建知识图谱
- [ ] 完成实践项目 P1: 从零搭建 K8s 集群
- [ ] 通过 checkpoint 自测检验学习效果

---

## 知识复习 (2h)

### 主动回忆练习

不看文档，在纸上/白板上画出:

1. **Docker 架构图**
   - Docker Client
   - Docker Daemon
   - Registry
   - 镜像分层结构

2. **Linux 容器原理图**
   - namespace 类型 (pid, net, mnt, uts, ipc, user, cgroup)
   - cgroup 资源控制 (cpu, memory, io)
   - 容器 = namespace + cgroup + rootfs

3. **Kubernetes 架构图**
   - 控制平面: etcd, API Server, Controller Manager, Scheduler
   - 数据平面: kubelet, kube-proxy, Container Runtime
   - 组件间通信流

画完后对照文档检查遗漏。

### 核心概念速查

| 主题 | 核心概念 | 自评掌握程度 |
|------|----------|--------------|
| Docker | 镜像、容器、Volume、Network | ⬜⬜⬜ |
| Linux | namespace、cgroup、进程、网络 | ⬜⬜⬜ |
| K8s 架构 | etcd、API Server、Scheduler、kubelet | ⬜⬜⬜ |
| K8s 资源 | Pod、Deployment、Service、Namespace | ⬜⬜⬜ |
| kubectl | get、describe、apply、logs、exec | ⬜⬜⬜ |

---

## 实践项目 P1 (2.5h)

### 项目: 从零搭建一个可运行 nginx 的 K8s 集群

详细指南见: [../projects/p1-k8s-cluster-setup.md](../projects/p1-k8s-cluster-setup.md)

#### Step 1: 创建集群 (30min)

```bash
# 使用 kind 创建集群
kind create cluster --name production-sim --config - <<EOF
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
- role: worker
- role: worker
EOF

# 验证集群
kubectl get nodes
kubectl get pods -n kube-system
```

#### Step 2: 创建 Namespace (10min)

```bash
# 创建项目 namespace
kubectl create namespace web-app

# 设置默认 namespace
kubectl config set-context --current --namespace=web-app
```

#### Step 3: 部署 Deployment (30min)

```bash
cat > deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-web
  namespace: web-app
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
          initialDelaySeconds: 5
          periodSeconds: 5
EOF

kubectl apply -f deployment.yaml
kubectl get pods -w
```

#### Step 4: 创建 Service (20min)

```bash
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
kubectl get svc
```

#### Step 5: 测试和调试 (30min)

```bash
# 测试 Service
kubectl run curl --image=curlimages/curl -it --rm -- curl nginx-service

# 查看 Pod 日志
kubectl logs -l app=nginx-web

# 进入 Pod 调试
kubectl exec -it $(kubectl get pod -l app=nginx-web -o jsonpath='{.items[0].metadata.name}') -- sh

# 查看事件
kubectl get events --sort-by='.lastTimestamp'

# 模拟常见问题并排查
# 1. 镜像拉取失败
kubectl set image deployment/nginx-web nginx=nginx:nonexistent
kubectl get pods
kubectl describe pod <pod-name> | grep -A5 Events
kubectl rollout undo deployment/nginx-web

# 2. 资源不足
# 修改 requests 超过节点容量，观察 Pending 状态
```

#### Step 6: 产出文档 (30min)

创建 `~/k8s-setup-doc.md`，记录:

1. 集群搭建步骤
2. 部署的资源清单
3. 遇到的问题和解决方法
4. 常用命令速查

---

## 自测检验

完成 [checkpoint.md](./checkpoint.md) 中的自测题，评估本周学习效果。

---

## 本周回顾总结

### 学习路径

```
Day 1-2: Docker 基础 → 容器本质
    ↓
Day 3-4: Linux 基础 → 容器底层原理
    ↓
Day 5-6: K8s 架构 → 集群管理
    ↓
Day 7: 综合实践 → 技能整合
```

### 关键收获

1. **Docker**: 容器是进程级隔离，不是虚拟机
2. **Linux**: namespace 隔离资源视图，cgroup 限制资源使用
3. **K8s**: 声明式管理，控制器模式，所有组件通过 API Server 通信

### 下周预告

Week 2 将深入 K8s 核心技术:
- 控制平面组件详解 (etcd、API Server、Scheduler)
- 工作负载管理 (Deployment、StatefulSet、DaemonSet)
- 网络栈 (CNI、Service、DNS、Ingress)
- 存储体系 (PV、PVC、StorageClass)

---

## 清理资源

```bash
# 删除测试资源
kubectl delete namespace web-app

# 如果不再需要集群
kind delete cluster --name production-sim
```

恭喜完成 Week 1 的学习!
