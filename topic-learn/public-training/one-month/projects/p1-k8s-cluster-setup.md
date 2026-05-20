---
title: '项目 P1: 从零搭建 K8s 集群'
description: '- k8s 故障排查入门练习'
category: learning
tags:
- k8s
- training
- hands-on
- apiserver
- kubelet
- scheduler
- coredns
- docker
- ingress
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- '项目 P1: 从零搭建 K8s 集群 是什么'
- '如何 项目 P1: 从零搭建 K8s 集群'
trigger_keywords:
- 项目
- 'P1:'
- 从零搭建
- K8s
- 集群
- learn
---


---
title: 项目 P1: 从零搭建 K8s 集群
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - kind kubernetes 集群本地搭建教程
  - kubectl 基本命令操作示例
  - 从零创建 deployment service namespace
  - k8s 故障排查入门练习
trigger_keywords:
  - kind
  - kubectl
  - Deployment
  - Service
  - Namespace
  - 集群搭建
  - 入门
  - 滚动更新
  - 回滚
reading_level: beginner
audience:
  - beginner-devops
  - platform-engineer
  - developer
estimated_read_time: 150min
related_domains:
  - domain-1-architecture-fundamentals
  - domain-4-workloads
  - domain-12-troubleshooting
related_topics:
  - topic-learn/public-training/one-month/projects/p2-production-app-orchestration
  - topic-learn/public-training/one-month/projects/p3-observability-fault-drill
---

# 项目 P1: 从零搭建 K8s 集群

> **所属周**: Week 1 | **预计时间**: 2.5 小时

---

## 概述

本项目将带你从零开始搭建一个完整的 Kubernetes 集群，并在其上部署一个可访问的 nginx Web 应用。通过这个项目，你将实践 Week 1 学到的所有核心知识：Docker 容器化、Kubernetes 架构理解、kubectl 命令行操作、以及 Pod/Deployment/Service 三大核心资源的使用。

项目使用 kind（Kubernetes in Docker）作为本地集群工具，可以在笔记本电脑上快速创建一个多节点的 K8s 集群，无需云资源。完成后你将拥有一个可运行的集群，具备基本的部署、调试和故障排查能力。

**项目目标**：
- 独立搭建本地 K8s 集群
- 创建 Namespace、Deployment、Service
- 使用 kubectl 进行基本操作和调试
- 理解 Pod / Deployment / Service 三者的关系

**前置条件**：
- 已完成 Week 1 Day 1-6 的学习
- 本机已安装 Docker
- 熟悉基本的 kubectl 命令

---

## Step 1: 安装 kind 并创建集群 (30min)

```bash
# Step 1.1: 安装 kind
# macOS
brew install kind

# Linux
curl -Lo ./kind https://kind.sigs.k8s.io/dl/v0.20.0/kind-linux-amd64
chmod +x ./kind
sudo mv ./kind /usr/local/bin/kind

# 验证安装
kind version

# 预期输出:
# kind v0.20.0 go1.21.0 linux/amd64

# Step 1.2: 创建多节点集群配置
cat > kind-config.yaml << 'EOF'
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
name: learn-k8s
nodes:
- role: control-plane
  kubeadmConfigPatches:
  - |
    kind: InitConfiguration
    nodeRegistration:
      kubeletExtraArgs:
        node-labels: "ingress-ready=true"
  extraPortMappings:
  - containerPort: 80
    hostPort: 80
    protocol: TCP
  - containerPort: 443
    hostPort: 443
    protocol: TCP
- role: worker
  labels:
    env: production
    tier: backend
- role: worker
  labels:
    env: staging
    tier: frontend
EOF

# Step 1.3: 创建集群
kind create cluster --name learn-k8s --config kind-config.yaml

# 预期输出:
# Creating cluster "learn-k8s" ...
#  ✓ Ensuring node image (kindest/node:v1.28.0) 🖼
#  ✓ Preparing nodes 📦 📦 📦
#  ✓ Writing configuration 📜
#  ✓ Starting control-plane 🕹️
#  ✓ Installing CNI 🔌
#  ✓ Installing StorageClass 💾
#  ✓ Joining worker nodes 🚜
# Set kubectl context to "kind-learn-k8s"
# You can now use your cluster with:
# kubectl cluster-info

# Step 1.4: 验证集群
kubectl cluster-info

# 预期输出:
# Kubernetes control plane is running at https://127.0.0.1:32768
# CoreDNS is running at https://127.0.0.1:32768/api/v1/namespaces/kube-system/services/kube-dns:dns/proxy

# Step 1.5: 查看节点
kubectl get nodes -o wide

# 预期输出:
# NAME                       STATUS   ROLES           AGE   VERSION   INTERNAL-IP   OS-IMAGE       KERNEL-VERSION
# learn-k8s-control-plane    Ready    control-plane   2m    v1.28.0   172.18.0.3    Ubuntu 22.04   5.15.0-...
# learn-k8s-worker           Ready    <none>          90s   v1.28.0   172.18.0.4    Ubuntu 22.04   5.15.0-...
# learn-k8s-worker2          Ready    <none>          90s   v1.28.0   172.18.0.5    Ubuntu 22.04   5.15.0-...

# Step 1.6: 查看节点标签
kubectl get nodes --show-labels

# 预期输出:
# NAME                       STATUS   ROLES    ...   LABELS
# learn-k8s-control-plane    Ready    ...      ...   ...,ingress-ready=true
# learn-k8s-worker           Ready    ...      ...   ...,env=production,tier=backend
# learn-k8s-worker2          Ready    ...      ...   ...,env=staging,tier=frontend
```

---

## Step 2: 创建 Namespace (10min)

```bash
# Step 2.1: 创建 namespace
kubectl create namespace web-app

# 预期输出:
# namespace/web-app created

# Step 2.2: 验证
kubectl get namespaces

# 预期输出:
# NAME              STATUS   AGE
# default           Active   10m
# kube-node-lease   Active   10m
# kube-public       Active   10m
# kube-system       Active   10m
# local-path-storage Active   10m
# web-app           Active   5s

# Step 2.3: 设置默认 namespace
kubectl config set-context --current --namespace=web-app

# 预期输出:
# Context "kind-learn-k8s" modified.

# Step 2.4: 验证当前 context
kubectl config current-context
```

---

## Step 3: 部署 Deployment (30min)

```bash
# Step 3.1: 创建 Deployment YAML
cat > deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-web
  namespace: web-app
  labels:
    app: nginx-web
    version: v1
spec:
  replicas: 3
  selector:
    matchLabels:
      app: nginx-web
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  template:
    metadata:
      labels:
        app: nginx-web
        version: v1
    spec:
      containers:
      - name: nginx
        image: nginx:1.25-alpine
        ports:
        - containerPort: 80
          name: http
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
        - name: html
          mountPath: /usr/share/nginx/html
      volumes:
      - name: html
        configMap:
          name: nginx-html
EOF

# Step 3.2: 创建 ConfigMap（自定义 HTML 内容）
cat > configmap.yaml << 'EOF'
apiVersion: v1
kind: ConfigMap
metadata:
  name: nginx-html
  namespace: web-app
data:
  index.html: |
    <!DOCTYPE html>
    <html>
    <head><title>K8s Learning</title></head>
    <body>
    <h1>Hello from Kubernetes!</h1>
    <p>Pod: ${HOSTNAME}</p>
    <p>This is a learning cluster built with kind.</p>
    </body>
    </html>
EOF

kubectl apply -f configmap.yaml
kubectl apply -f deployment.yaml

# 预期输出:
# configmap/nginx-html created
# deployment.apps/nginx-web created

# Step 3.3: 观察 Pod 创建过程
kubectl get pods -n web-app -w

# 预期输出:
# NAME                         READY   STATUS              RESTARTS   AGE
# nginx-web-7d9f8b6c4-abc12   0/1     ContainerCreating   0          5s
# nginx-web-7d9f8b6c4-def34   0/1     ContainerCreating   0          5s
# nginx-web-7d9f8b6c4-ghi56   0/1     ContainerCreating   0          5s
# nginx-web-7d9f8b6c4-abc12   1/1     Running             0          15s
# nginx-web-7d9f8b6c4-def34   1/1     Running             0          15s
# nginx-web-7d9f8b6c4-ghi56   1/1     Running             0          15s

# Step 3.4: 验证 Deployment
kubectl get deployment -n web-app

# 预期输出:
# NAME         READY   UP-TO-DATE   AVAILABLE   AGE
# nginx-web    3/3     3            3           30s

# Step 3.5: 查看 Pod 详情
kubectl get pods -n web-app -o wide

# 预期输出:
# NAME                         READY   STATUS    RESTARTS   AGE   IP           NODE
# nginx-web-7d9f8b6c4-abc12   1/1     Running   0          1m    10.244.1.3   learn-k8s-worker
# nginx-web-7d9f8b6c4-def34   1/1     Running   0          1m    10.244.2.3   learn-k8s-worker2
# nginx-web-7d9f8b6c4-ghi56   1/1     Running   0          1m    10.244.1.4   learn-k8s-worker
```

---

## Step 4: 创建 Service (20min)

```bash
# Step 4.1: 创建 Service YAML
cat > service.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: nginx-service
  namespace: web-app
  labels:
    app: nginx-web
spec:
  selector:
    app: nginx-web
  ports:
  - name: http
    port: 80
    targetPort: http
    protocol: TCP
  type: NodePort
EOF

kubectl apply -f service.yaml

# 预期输出:
# service/nginx-service created

# Step 4.2: 验证 Service
kubectl get svc -n web-app

# 预期输出:
# NAME            TYPE       CLUSTER-IP      EXTERNAL-IP   PORT(S)        AGE
# nginx-service   NodePort   10.96.123.45    <none>        80:31234/TCP   10s

# Step 4.3: 查看 Endpoints
kubectl get endpoints -n web-app

# 预期输出:
# NAME            ENDPOINTS                                      AGE
# nginx-service   10.244.1.3:80,10.244.1.4:80,10.244.2.3:80     30s

# Step 4.4: 测试 Service
kubectl run curl-test --image=curlimages/curl -n web-app --rm -it --restart=Never -- curl -s http://nginx-service

# 预期输出:
# <!DOCTYPE html>
# <html>
# <head><title>K8s Learning</title></head>
# <body>
# <h1>Hello from Kubernetes!</h1>
# ...
```

---

## Step 5: 测试和调试 (30min)

```bash
# Step 5.1: 查看 Pod 日志
kubectl logs -l app=nginx-web -n web-app --tail=10

# 预期输出:
# 10.244.1.1 - - [18/May/2026:10:30:00 +0000] "GET / HTTP/1.1" 200 162 "-" "curl/8.1.2"

# Step 5.2: 进入 Pod 调试
kubectl exec -it $(kubectl get pod -l app=nginx-web -n web-app -o jsonpath='{.items[0].metadata.name}') -n web-app -- sh

# 在 Pod 内执行:
ls /usr/share/nginx/html/
cat /usr/share/nginx/html/index.html
curl localhost:80
exit

# Step 5.3: 查看事件
kubectl get events -n web-app --sort-by='.lastTimestamp'

# 预期输出:
# LAST SEEN   TYPE     REASON    OBJECT                         MESSAGE
# 2m          Normal   Pulled    pod/nginx-web-xxx              Successfully pulled image
# 2m          Normal   Created   pod/nginx-web-xxx              Created container nginx
# 2m          Normal   Started   pod/nginx-web-xxx              Started container nginx

# Step 5.4: 模拟故障排查
# 5.4.1: 触发镜像拉取失败
kubectl set image deployment/nginx-web nginx=nginx:nonexistent -n web-app

# 预期输出:
# deployment.apps/nginx-web image updated

# 5.4.2: 观察 Pod 状态
kubectl get pods -n web-app -w

# 预期输出:
# NAME                         READY   STATUS             RESTARTS   AGE
# nginx-web-xxxxx-abc12       1/1     Running            0          5m
# nginx-web-xxxxx-def34       0/1     ImagePullBackOff   0          30s

# 5.4.3: 查看失败原因
kubectl describe pod <failed-pod> -n web-app | grep -A 10 Events

# 预期输出:
# Events:
#   Type     Reason     Age                Message
#   Warning  Failed     10s (x3 over 30s)  Error: ImagePullBackOff
#   Normal   Pulling    30s                Pulling image "nginx:nonexistent"
#   Warning  Failed     25s                Failed to pull image "nginx:nonexistent": not found

# 5.4.4: 回滚修复
kubectl rollout undo deployment/nginx-web -n web-app

# 预期输出:
# deployment.apps/nginx-web rolled back

# 5.4.5: 验证回滚成功
kubectl get pods -n web-app

# 预期输出:
# NAME                         READY   STATUS    RESTARTS   AGE
# nginx-web-7d9f8b6c4-abc12   1/1     Running   0          8m
# nginx-web-7d9f8b6c4-def34   1/1     Running   0          8m
# nginx-web-7d9f8b6c4-ghi56   1/1     Running   0          8m

# Step 5.5: 查看 Deployment 历史
kubectl rollout history deployment/nginx-web -n web-app

# 预期输出:
# REVISION  CHANGE-CAUSE
# 1         <none>
# 2         <none>
# 3         <none>
```

---

## Step 6: 文档输出 (30min)

创建 `~/k8s-setup-doc.md`，记录:

```markdown
# K8s 集群搭建文档

## 1. 集群信息
- 集群工具: kind v0.20.0
- K8s 版本: v1.28.0
- 节点数: 3 (1 control-plane + 2 worker)
- 网络: Kindnet (CNI)
- 存储: local-path-provisioner

## 2. 部署的资源清单
| 资源类型 | 名称 | 命名空间 | 说明 |
|----------|------|---------|------|
| ConfigMap | nginx-html | web-app | 自定义 HTML |
| Deployment | nginx-web | web-app | nginx 3副本 |
| Service | nginx-service | web-app | NodePort 80 |

## 3. 常用命令速查
kubectl get pods -A              # 查看所有Pod
kubectl get svc -n web-app       # 查看Service
kubectl logs -f <pod>            # 查看日志
kubectl exec -it <pod> -- sh     # 进入容器
kubectl describe pod <pod>       # 查看详情
kubectl rollout undo deploy/<n>  # 回滚

## 4. 故障排查经验
- ImagePullBackOff → 检查镜像地址
- CrashLoopBackOff → kubectl logs --previous
- Pending → kubectl describe pod 看 Events
```

---

## 配置参考

### kind 高级配置

```yaml
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
name: advanced-cluster
featureGates:
  EphemeralContainers: true
nodes:
- role: control-plane
  kubeadmConfigPatches:
  - |
    kind: ClusterConfiguration
    apiServer:
      extraArgs:
        audit-log-path: /var/log/audit/audit.log
        audit-log-maxage: "30"
    scheduler:
      extraArgs:
        bind-address: "0.0.0.0"
  extraMounts:
  - hostPath: /tmp/audit
    containerPath: /var/log/audit
- role: worker
  extraMounts:
  - hostPath: /tmp/data
    containerPath: /data
```

### Deployment 参数参考

| 参数 | 说明 | 推荐值 |
|------|------|--------|
| `replicas` | 副本数 | >= 2（生产环境 >= 3） |
| `maxSurge` | 滚动更新时最多多创建的 Pod 数 | 1 或 25% |
| `maxUnavailable` | 滚动更新时最多不可用的 Pod 数 | 0 或 25% |
| `resources.requests.cpu` | CPU 请求 | 50m-200m |
| `resources.limits.cpu` | CPU 限制 | requests 的 2 倍 |
| `livenessProbe.periodSeconds` | 存活检查间隔 | 10s |
| `readinessProbe.periodSeconds` | 就绪检查间隔 | 5s |

---

## 验收清单

- [ ] 集群成功创建，3 个节点正常运行
- [ ] Namespace web-app 创建成功
- [ ] Deployment 部署成功，3 个 Pod 运行正常
- [ ] Service 创建成功，Endpoints 非空
- [ ] 能够通过 Service 访问 nginx
- [ ] 能够查看 Pod 日志
- [ ] 能够进入 Pod 进行调试
- [ ] 完成故障模拟（镜像错误）和回滚
- [ ] 完成搭建文档

---

## 常见问题

### Q1: kind 创建集群超时怎么办？

**A**: 可能原因和解决方法：
1. **网络问题**: Docker 拉取镜像超时，配置 Docker 镜像加速器
2. **资源不足**: 确保 Docker 至少分配 4GB 内存和 2 CPU
3. **端口冲突**: 确保 80/443 端口没有被占用

### Q2: Pod 一直处于 ContainerCreating 怎么办？

**A**:
```bash
kubectl describe pod <pod> -n web-app
# 查看 Events 部分，常见原因:
# - 镜像拉取慢/失败
# - ConfigMap/Secret 不存在
# - PVC 无法挂载
```

### Q3: Service 的 Endpoints 为空怎么办？

**A**: 检查 Service selector 是否匹配 Pod 标签：
```bash
kubectl describe svc nginx-service -n web-app | grep Selector
kubectl get pods -n web-app --show-labels
```

---

## 要点总结

- **kind** 是本地 K8s 集群工具，适合学习和开发测试
- **Pod / Deployment / Service** 是 K8s 三大核心资源
- **Deployment** 管理副本数和更新策略，**Service** 提供稳定的访问入口
- 故障排查三板斧: `describe` → `logs` → `get events`
- `rollout undo` 可以快速回滚到上一个版本

---

## 清理资源

```bash
kubectl delete namespace web-app
kind delete cluster --name learn-k8s
```

---

## 延伸阅读

- [kind 官方文档](https://kind.sigs.k8s.io/)
- [Deployment 文档](https://kubernetes.io/docs/concepts/workloads/controllers/deployment/)
- [Service 文档](https://kubernetes.io/docs/concepts/services-networking/service/)
- [kubectl 速查表](https://kubernetes.io/docs/reference/kubectl/cheatsheet/)
