---
title: 01 - 本机单机 Demo 部署
description: 'title: 01 - 本机单机 Demo 部署'
category: general
tags:
- deployment
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- coredns
- containerd
- docker
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 本机单机 Demo 部署 是什么
- 如何 本机单机 Demo 部署
- Kubernetes 11 production operations 最佳实践
trigger_keywords:
- 本机单机
- Demo
- 部署
- production
- operations
- best
- practices
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
---

title: 01 - 本机单机 Demo 部署
description: '# 01 - 本机单机 Demo 部署'
category: deployment
tags:
- k8s
- deployment
- rolling-update
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- coredns
- containerd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 本机单机 Demo 部署 是什么
- 如何 本机单机 Demo 部署
trigger_keywords:
- 本机单机
- Demo
- 部署
- deployment
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 01 - 本机单机 Demo 部署

> **适用场景**: 初学者快速体验 | **预计时间**: 30-60 分钟 | **复杂度**: ⭐  
> **目标**: 在本机快速运行一个 K8s 集群，体验核心概念 (Pod、Deployment、Service)

---

<!-- chunk: 概述 -->## 概述

本文档提供在本机 (macOS / Linux / Windows) 上快速搭建 Kubernetes 集群的完整方案。通过 Docker 容器模拟多节点集群，**无需额外虚拟机或物理服务器**，适用于学习、体验和功能验证。

**本文你将学会**:
- 用 kind 或 minikube 在本机创建 K8s 集群
- 用 kubectl 部署第一个 nginx 应用
- 通过 Service 暴露应用并访问
- 基本的故障模拟和回滚操作
- 查看和理解 K8s 系统组件

---

<!-- chunk: 前置条件 -->## 前置条件

#<!-- chunk: 硬件要求 -->## 硬件要求

| 资源 | 最低要求 | 推荐配置 | 说明 |
|------|---------|---------|------|
| CPU | 2 核 | 4 核 | kind 多节点需要更多 CPU |
| 内存 | 4GB | 8GB | Docker + K8s 组件占用约 2GB |
| 磁盘 | 10GB | 20GB | 镜像存储需要空间 |

#<!-- chunk: Docker Desktop 安装与配置 -->## Docker Desktop 安装与配置

> **关键**: kind 和 minikube (Docker 驱动) 都依赖 Docker，这是第一步。

**macOS**:
```bash
# 方式 1: Homebrew 安装 (推荐)
brew install --cask docker

# 方式 2: 官网下载 Docker Desktop
# 访问 https://www.docker.com/products/docker-desktop/ 下载安装

# 安装后启动 Docker Desktop，等待状态栏图标变为 "Running"
```

**Linux (Ubuntu/Debian)**:
```bash
# 安装 Docker Engine
sudo apt-get update
sudo apt-get install -y ca-certificates curl gnupg
sudo install -m 0755 -d /etc/apt/keyrings
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo gpg --dearmor -o /etc/apt/keyrings/docker.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable" | sudo tee /etc/apt/sources.list.d/docker.list
sudo apt-get update
sudo apt-get install -y docker-ce docker-ce-cli containerd.io

# 将当前用户加入 docker 组 (免 sudo)
sudo usermod -aG docker $USER
newgrp docker  # 立即生效，或重新登录
```

**Windows**:
```powershell
# 方式 1: winget 安装
winget install Docker.DockerDesktop

# 方式 2: 官网下载 Docker Desktop for Windows
# 访问 https://www.docker.com/products/docker-desktop/ 下载安装
# 安装后需重启系统，确保 WSL 2 已启用
```

**Docker Desktop 资源配置** (macOS / Windows):
```
打开 Docker Desktop → Settings → Resources：
  - CPUs:     4 (至少 2)
  - Memory:   8 GB (至少 4 GB，多节点集群建议 8GB)
  - Disk:     30 GB (至少 20 GB)
  - 点击 "Apply & Restart"
```

#<!-- chunk: 验证 Docker 可用 -->## 验证 Docker 可用

```bash
# 检查 Docker 版本和运行状态
docker version

# 预期输出 (关键字段):
# Client:
#  Version:           24.0.x
# Server:
#  Version:           24.0.x

docker info | grep "Server Version"
# 预期输出: Server Version: 24.0.x

# 测试 Docker 能否正常运行容器
docker run --rm hello-world
# 预期输出: Hello from Docker! This message shows that your installation appears to be working correctly.
```

> **如果 docker version 报错 "Cannot connect to the Docker daemon"**:  
> macOS/Windows: 确保 Docker Desktop 已启动 (状态栏有鲸鱼图标)  
> Linux: 运行 `sudo systemctl start docker && sudo systemctl enable docker`

#<!-- chunk: 安装 kubectl -->## 安装 kubectl

```bash
# macOS
brew install kubectl

# Linux
curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl"
chmod +x kubectl && sudo mv kubectl /usr/local/bin/

# Windows (PowerShell)
winget install Kubernetes.kubectl
# 或手动下载: https://dl.k8s.io/release/v1.28.0/bin/windows/amd64/kubectl.exe
# 将 kubectl.exe 放到 PATH 中的目录

# 验证 (所有平台)
kubectl version --client
# 预期输出: Client Version: v1.28.x (版本号可能不同，只要有输出即可)
```

---

<!-- chunk: 方案 A: 使用 kind (推荐，轻量) -->## 方案 A: 使用 kind (推荐，轻量)

> **kind (Kubernetes in Docker)** 使用 Docker 容器作为节点运行 Kubernetes 集群。  
> **优势**: 启动速度快 (~30s)、资源占用少、原生支持多节点、非常适合 CI/CD 和快速测试。

#<!-- chunk: A1. 安装 kind -->## A1. 安装 kind

```bash
# macOS
brew install kind

# Linux
# 备注: 下载特定版本二进制文件，替换 v0.20.0 为最新版本
[ $(uname -m) = x86_64 ] && curl -Lo ./kind https://kind.sigs.k8s.io/dl/v0.20.0/kind-linux-amd64
[ $(uname -m) = aarch64 ] && curl -Lo ./kind https://kind.sigs.k8s.io/dl/v0.20.0/kind-linux-arm64
chmod +x ./kind
sudo mv ./kind /usr/local/bin/kind

# Windows (PowerShell)
# 方式 1:
winget install Kubernetes.kind
# 方式 2: 手动下载
# curl.exe -Lo kind-windows-amd64.exe https://kind.sigs.k8s.io/dl/v0.20.0/kind-windows-amd64
# Move-Item .\kind-windows-amd64.exe C:\Windows\kind.exe

# 验证安装 (所有平台)
kind version
# 预期输出: kind v0.20.0 go1.21.1 darwin/arm64 (版本号和平台可能不同)
```

#<!-- chunk: A2. 创建单节点集群 (最快体验) -->## A2. 创建单节点集群 (最快体验)

```bash
# 一键创建集群 (默认名称 "kind"，单节点)
kind create cluster --name learn-k8s

# 预期输出:
# Creating cluster "learn-k8s" ...
#  ✓ Ensuring node image (kindest/node:v1.27.3) 🖼
#  ✓ Preparing nodes 📦
#  ✓ Writing configuration 📜
#  ✓ Starting control-plane 🕹️
#  ✓ Installing CNI 🔌
#  ✓ Installing StorageClass 💾
# Set kubectl context to "kind-learn-k8s"
# You can now use your cluster with:
# kubectl cluster-info --context kind-learn-k8s

# 验证集群信息
kubectl cluster-info
# 预期输出:
# Kubernetes control plane is running at https://127.0.0.1:xxxxx
# CoreDNS is running at https://127.0.0.1:xxxxx/api/v1/namespaces/kube-system/services/kube-dns:dns/proxy

kubectl get nodes
# 预期输出:
# NAME                     STATUS   ROLES           AGE   VERSION
# learn-k8s-control-plane  Ready    control-plane   1m    v1.27.3
```

#<!-- chunk: A3. 创建多节点集群 (1 Master + 2 Worker) -->## A3. 创建多节点集群 (1 Master + 2 Worker)

> **备注**: 多节点集群可以体验 Pod 在不同节点间的调度，更接近真实场景。

```bash
# 先创建配置文件
cat > kind-config.yaml << 'EOF'
# kind 多节点集群配置
# 文档: https://kind.sigs.k8s.io/docs/user/configuration/
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane   # 控制平面节点 (运行 apiserver、etcd、scheduler 等)
- role: worker          # 工作节点 1 (运行用户应用 Pod)
- role: worker          # 工作节点 2 (运行用户应用 Pod)
EOF

# 创建集群 (约 60-90 秒)
kind create cluster --name learn-k8s --config kind-config.yaml

# 验证: 应该看到 3 个节点
kubectl get nodes
# 预期输出:
# NAME                      STATUS   ROLES           AGE   VERSION
# learn-k8s-control-plane   Ready    control-plane   2m    v1.27.3
# learn-k8s-worker          Ready    <none>          1m    v1.27.3
# learn-k8s-worker2         Ready    <none>          1m    v1.27.3

# 查看节点的 Docker 容器
docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
# 预期输出: 3 个 kindest/node 容器正在运行
```

#<!-- chunk: A4. 创建带 Ingress 支持的集群 -->## A4. 创建带 Ingress 支持的集群

> **备注**: 如果你后续要测试 Ingress（HTTP 路由），需要在创建集群时映射端口。

```bash
cat > kind-config-ingress.yaml << 'EOF'
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
  kubeadmConfigPatches:
  - |
    kind: InitConfiguration
    nodeRegistration:
      kubeletExtraArgs:
        node-labels: "ingress-ready=true"  # 标记此节点可运行 Ingress Controller
  extraPortMappings:
  - containerPort: 80     # 容器内端口
    hostPort: 80          # 映射到本机 80 端口
    protocol: TCP
  - containerPort: 443    # HTTPS 端口
    hostPort: 443
    protocol: TCP
- role: worker
- role: worker
EOF

kind create cluster --name dev-cluster --config kind-config-ingress.yaml

# 安装 Nginx Ingress Controller (专为 kind 优化)
kubectl apply -f https://raw.githubusercontent.com/kubernetes/ingress-nginx/main/deploy/static/provider/kind/deploy.yaml

# 等待 Ingress Controller 就绪 (约 30-60 秒)
kubectl wait --namespace ingress-nginx \
  --for=condition=ready pod \
  --selector=app.kubernetes.io/component=controller \
  --timeout=90s
# 预期输出: pod/ingress-nginx-controller-xxxxx condition met
```

#<!-- chunk: A5. 加载本地镜像到 kind 集群 -->## A5. 加载本地镜像到 kind 集群

> **实用技巧**: kind 集群运行在 Docker 内，默认无法访问本地构建的镜像。需要手动加载。

```bash
# 构建本地镜像
docker build -t my-app:v1.0 .

# 将镜像加载到 kind 集群中
kind load docker-image my-app:v1.0 --name learn-k8s

# 验证镜像已加载 (进入节点容器检查)
docker exec -it learn-k8s-control-plane crictl images | grep my-app
# 预期输出: docker.io/library/my-app   v1.0   xxxxx   xxxMB

# 在 Deployment 中使用时，设置 imagePullPolicy: Never 或 IfNotPresent
# 避免 K8s 尝试从远程仓库拉取
```

---

<!-- chunk: 方案 B: 使用 minikube -->## 方案 B: 使用 minikube

> **minikube** 提供更丰富的插件生态和多驱动支持 (Docker/VirtualBox/HyperKit 等)。  
> **优势**: 内置 Dashboard、LoadBalancer tunnel、多插件一键启用。

#<!-- chunk: B1. 安装 minikube -->## B1. 安装 minikube

```bash
# macOS
brew install minikube

# Linux (x86_64)
curl -LO https://storage.googleapis.com/minikube/releases/latest/minikube-linux-amd64
sudo install minikube-linux-amd64 /usr/local/bin/minikube
rm minikube-linux-amd64

# Windows (PowerShell)
winget install minikube

# 验证
minikube version
# 预期输出: minikube version: v1.32.0 (版本号可能不同)
```

#<!-- chunk: B2. 启动集群 -->## B2. 启动集群

```bash
# 基础启动 (使用 Docker 驱动)
minikube start --driver=docker
# 备注: 首次启动会下载 K8s 节点镜像，约 500MB，耐心等待

# 推荐配置: 指定资源和常用插件
minikube start \
  --driver=docker \
  --cpus=4 \               # 分配 4 个 CPU 核心
  --memory=8192 \           # 分配 8GB 内存
  --disk-size=50g \         # 分配 50GB 磁盘
  --kubernetes-version=v1.28.0 \  # 指定 K8s 版本
  --addons=dashboard,ingress,metrics-server  # 一键启用常用插件

# 预期输出:
# 🏄  Done! kubectl is now configured to use "minikube" cluster
# 💡  kubectl now points to "minikube" cluster

# 验证
kubectl get nodes
# 预期输出:
# NAME       STATUS   ROLES           AGE   VERSION
# minikube   Ready    control-plane   1m    v1.28.0
```

#<!-- chunk: B3. minikube 常用操作 -->## B3. minikube 常用操作

```bash
# 查看集群状态
minikube status
# 预期输出:
# minikube
# type: Control Plane
# host: Running
# kubelet: Running
# apiserver: Running
# kubeconfig: Configured

# 查看/启用/禁用插件
minikube addons list         # 列出所有可用插件及状态
minikube addons enable dashboard       # 启用 Web Dashboard
minikube addons enable metrics-server  # 启用指标服务 (支持 kubectl top)
minikube addons enable ingress         # 启用 Ingress Controller

# 打开 Dashboard (自动在浏览器中打开)
minikube dashboard
# 备注: 按 Ctrl+C 停止端口转发

# 访问 Service (minikube 特有功能，自动打开浏览器)
minikube service <service-name> -n <namespace>

# 进入 minikube 节点 (调试用)
minikube ssh
# 退出: exit

# 暂停 / 恢复 / 停止 / 删除
minikube pause      # 暂停集群 (释放 CPU，保留数据)
minikube unpause    # 恢复集群
minikube stop       # 停止集群 (释放所有资源，保留数据)
minikube start      # 重新启动已停止的集群
minikube delete     # 彻底删除集群 (清除所有数据)
```

---

<!-- chunk: Demo 实战：部署第一个应用 -->## Demo 实战：部署第一个应用

> **目标**: 从零开始部署一个 nginx Web 服务器，理解 Namespace → Deployment → Service 的完整流程。

#<!-- chunk: Step 1: 创建 Namespace -->## Step 1: 创建 Namespace

> **为什么要创建 Namespace？** Namespace 是 K8s 的"虚拟集群"，用于隔离不同项目/环境的资源，避免命名冲突。

```bash
# 创建 namespace
kubectl create namespace web-app

# 验证
kubectl get namespaces
# 预期输出 (除了系统自带的，多了 web-app):
# NAME              STATUS   AGE
# default           Active   10m
# kube-node-lease   Active   10m
# kube-public       Active   10m
# kube-system       Active   10m
# web-app           Active   5s    ← 新创建的

# 设置当前 context 的默认 namespace (避免每次都加 -n web-app)
kubectl config set-context --current --namespace=web-app
# 验证当前 namespace
kubectl config view --minify | grep namespace
# 预期输出: namespace: web-app
```

#<!-- chunk: Step 2: 部署 Deployment -->## Step 2: 部署 Deployment

> **Deployment 是什么？** 它管理一组相同的 Pod 副本，负责自动创建、扩缩容、滚动更新和回滚。

```bash
cat > deployment.yaml << 'EOF'
apiVersion: apps/v1        # API 版本 (Deployment 使用 apps/v1)
kind: Deployment           # 资源类型
metadata:
  name: nginx-web          # Deployment 名称
  namespace: web-app       # 所属 Namespace
  labels:
    app: nginx-web         # 标签，用于组织和筛选资源
spec:
  replicas: 3              # 副本数: 始终保持 3 个 Pod 运行
  selector:
    matchLabels:
      app: nginx-web       # 选择器: 管理 label 为 app=nginx-web 的 Pod
  template:                # Pod 模板 (每个副本都按此模板创建)
    metadata:
      labels:
        app: nginx-web     # Pod 标签 (必须匹配上面的 selector)
    spec:
      containers:
      - name: nginx        # 容器名称
        image: nginx:alpine  # 使用 Alpine 版 nginx (体积小，约 40MB)
        ports:
        - containerPort: 80  # 容器监听 80 端口
        resources:           # 资源限制 (好习惯: 始终设置资源限制)
          requests:          # 最低保证资源 (调度依据)
            cpu: 50m         # 50 毫核 = 0.05 个 CPU 核心
            memory: 64Mi     # 64 MiB 内存
          limits:            # 最大使用资源 (超过会被限流/OOM Kill)
            cpu: 100m        # 100 毫核 = 0.1 个 CPU 核心
            memory: 128Mi    # 128 MiB 内存
EOF

# 应用配置
kubectl apply -f deployment.yaml
# 预期输出: deployment.apps/nginx-web created

# 查看 Deployment 状态
kubectl get deployment
# 预期输出:
# NAME        READY   UP-TO-DATE   AVAILABLE   AGE
# nginx-web   3/3     3            3           30s
# 备注: READY 3/3 表示 3 个 Pod 中有 3 个已就绪

# 实时观察 Pod 创建过程 (按 Ctrl+C 退出观察)
kubectl get pods -w
# 预期输出 (会看到 Pod 从 Pending → ContainerCreating → Running):
# NAME                        READY   STATUS    RESTARTS   AGE
# nginx-web-6d4f5b9-abcde    1/1     Running   0          45s
# nginx-web-6d4f5b9-fghij    1/1     Running   0          45s
# nginx-web-6d4f5b9-klmno    1/1     Running   0          45s

# 查看 Pod 详细信息 (包括被调度到哪个节点)
kubectl get pods -o wide
# 预期输出: 多了 IP、NODE、NOMINATED NODE 等列
```

#<!-- chunk: Step 3: 创建 Service -->## Step 3: 创建 Service

> **Service 是什么？** Pod IP 是临时的（Pod 重启就变），Service 提供稳定的访问入口（固定 ClusterIP 或 NodePort），并自动负载均衡到后端 Pod。

```bash
cat > service.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: nginx-service      # Service 名称 (集群内可通过此名称 DNS 解析)
  namespace: web-app
spec:
  selector:
    app: nginx-web         # 选择后端 Pod: label 为 app=nginx-web 的 Pod
  ports:
  - port: 80               # Service 暴露端口
    targetPort: 80          # 转发到 Pod 的端口 (对应 containerPort)
    protocol: TCP
  type: NodePort            # 类型: NodePort 会在每个节点上开一个随机端口 (30000-32767)
                            # 其他类型: ClusterIP (仅集群内访问), LoadBalancer (需要云厂商支持)
EOF

kubectl apply -f service.yaml
# 预期输出: service/nginx-service created

# 查看 Service
kubectl get svc
# 预期输出:
# NAME            TYPE       CLUSTER-IP     EXTERNAL-IP   PORT(S)        AGE
# nginx-service   NodePort   10.96.xxx.xx   <none>        80:3xxxx/TCP   10s
# 备注: 80:3xxxx 中的 3xxxx 就是 NodePort 端口号

# 查看 Endpoints (Service 关联的后端 Pod IP)
kubectl get endpoints nginx-service
# 预期输出:
# NAME            ENDPOINTS                                      AGE
# nginx-service   10.244.0.5:80,10.244.0.6:80,10.244.0.7:80     20s
# 备注: 3 个 Endpoint 对应 3 个 Pod (副本数=3)
```

#<!-- chunk: Step 4: 测试和调试 -->## Step 4: 测试和调试

```bash
# ===== 方式 1: 集群内访问 (推荐，最可靠) =====
# 启动一个临时的 curl Pod，测试 Service 内部连通性
kubectl run curl-test --image=curlimages/curl --rm -it --restart=Never -- \
  curl -s nginx-service.web-app.svc.cluster.local
# 预期输出: nginx 默认页面 HTML
# 备注: DNS 格式为 <service-name>.<namespace>.svc.cluster.local
#        同 namespace 内可以简写为 nginx-service

# ===== 方式 2: kubectl port-forward (本机浏览器访问) =====
kubectl port-forward svc/nginx-service 8080:80
# 然后打开浏览器访问 http://localhost:8080
# 按 Ctrl+C 停止转发

# ===== 方式 3: minikube 直接访问 (仅 minikube) =====
# minikube service nginx-service -n web-app

# ===== 调试命令 =====
# 查看 Pod 日志
kubectl logs -l app=nginx-web --tail=20
# 预期输出: nginx 访问日志

# 查看某个 Pod 的详细日志
kubectl logs $(kubectl get pod -l app=nginx-web -o jsonpath='{.items[0].metadata.name}') -f
# 备注: -f 表示实时跟踪日志 (类似 tail -f)，按 Ctrl+C 退出

# 进入 Pod 调试
kubectl exec -it $(kubectl get pod -l app=nginx-web -o jsonpath='{.items[0].metadata.name}') -- sh
# 进入容器后可以执行:
#   ls /usr/share/nginx/html/   # 查看 nginx 静态文件
#   cat /etc/nginx/nginx.conf   # 查看 nginx 配置
#   wget -qO- localhost         # 内部测试
#   exit                        # 退出容器

# 查看 Namespace 下所有事件 (按时间排序，用于排查问题)
kubectl get events --sort-by='.lastTimestamp'
# 预期输出: 显示 Pod 创建、调度、拉取镜像等事件
```

#<!-- chunk: Step 5: 扩缩容实战 -->## Step 5: 扩缩容实战

```bash
# 扩容到 5 个副本
kubectl scale deployment nginx-web --replicas=5
kubectl get pods -w   # 观察新 Pod 创建过程
# 预期: 2 个新 Pod 被创建，状态变为 Running

# 缩容到 2 个副本
kubectl scale deployment nginx-web --replicas=2
kubectl get pods -w   # 观察多余 Pod 被终止
# 预期: 3 个 Pod 进入 Terminating 状态后消失

# 恢复到 3 个副本
kubectl scale deployment nginx-web --replicas=3
```

#<!-- chunk: Step 6: 滚动更新和回滚 -->## Step 6: 滚动更新和回滚

```bash
# ===== 滚动更新: 将 nginx:alpine 升级到 nginx:latest =====
kubectl set image deployment/nginx-web nginx=nginx:latest
# 备注: 格式为 kubectl set image deployment/<name> <container-name>=<new-image>

# 观察滚动更新过程
kubectl rollout status deployment/nginx-web
# 预期输出:
# Waiting for deployment "nginx-web" rollout to finish: 1 out of 3 new replicas have been updated...
# Waiting for deployment "nginx-web" rollout to finish: 2 out of 3 new replicas have been updated...
# deployment "nginx-web" successfully rolled out

# 查看更新历史
kubectl rollout history deployment/nginx-web
# 预期输出:
# REVISION  CHANGE-CAUSE
# 1         <none>
# 2         <none>

# ===== 模拟错误镜像 (学习回滚) =====
kubectl set image deployment/nginx-web nginx=nginx:nonexistent-tag

# 观察 Pod 状态 (会看到 ImagePullBackOff 错误)
kubectl get pods
# 预期输出:
# NAME                        READY   STATUS             RESTARTS   AGE
# nginx-web-xxx-yyy           0/1     ImagePullBackOff   0          30s  ← 新 Pod 拉取失败
# nginx-web-old-xxx           1/1     Running            0          5m   ← 旧 Pod 仍然运行
# 备注: K8s 滚动更新的安全机制—新 Pod 启动失败时，不会终止旧 Pod

# 查看失败原因
kubectl describe pod $(kubectl get pod -l app=nginx-web --field-selector=status.phase!=Running -o jsonpath='{.items[0].metadata.name}') | tail -20
# 预期输出: 在 Events 中看到 "Failed to pull image" 和 "ImagePullBackOff"

# 回滚到上一个版本
kubectl rollout undo deployment/nginx-web
# 预期输出: deployment.apps/nginx-web rolled back

# 验证恢复
kubectl get pods
# 预期: 所有 Pod 恢复 Running 状态

# 回滚到指定版本 (如果需要)
# kubectl rollout undo deployment/nginx-web --to-revision=1
```

---

<!-- chunk: 探索集群组件 -->## 探索集群组件

> **目的**: 了解 K8s 集群背后运行了哪些系统组件，为后续深入学习打基础。

```bash
# 查看 kube-system 命名空间下的系统 Pod
kubectl get pods -n kube-system
# 预期输出 (kind 集群):
# NAME                                              READY   STATUS    RESTARTS   AGE
# coredns-xxx-yyy                                   1/1     Running   0          30m  ← DNS 服务
# coredns-xxx-zzz                                   1/1     Running   0          30m  ← DNS 服务 (双副本)
# etcd-learn-k8s-control-plane                      1/1     Running   0          30m  ← 数据存储
# kindnet-xxx                                       1/1     Running   0          30m  ← CNI 网络插件
# kube-apiserver-learn-k8s-control-plane            1/1     Running   0          30m  ← API 网关
# kube-controller-manager-learn-k8s-control-plane   1/1     Running   0          30m  ← 控制器管理
# kube-proxy-xxx                                    1/1     Running   0          30m  ← 网络代理
# kube-scheduler-learn-k8s-control-plane            1/1     Running   0          30m  ← 调度器

# 各组件职责说明:
# etcd                  - 集群的"大脑"，存储所有配置和状态数据 (分布式 KV 数据库)
# kube-apiserver        - 集群的"前台"，所有操作的统一入口 (REST API)
# kube-controller-manager - 集群的"管家"，确保资源状态符合期望 (副本数、节点健康等)
# kube-scheduler        - 集群的"调度员"，决定 Pod 运行在哪个节点上
# kube-proxy            - 集群的"邮递员"，实现 Service 到 Pod 的流量转发
# coredns               - 集群的"电话簿"，提供 Service 名称到 IP 的 DNS 解析

# 查看节点详情 (资源使用、系统信息、Conditions)
kubectl describe node | head -60

# 查看 K8s 支持的所有 API 资源
kubectl api-resources --sort-by=name | head -30
# 备注: 了解 K8s 有哪些资源类型可用

# 查看所有 API 版本
kubectl api-versions
```

---

<!-- chunk: 验收清单 -->## 验收清单

完成以下所有项目，说明你已经掌握了本地 Demo 部署的基本技能：

- [ ] Docker 安装成功，`docker run hello-world` 正常
- [ ] kind/minikube 安装成功，集群创建成功
- [ ] 集群节点全部 Ready (`kubectl get nodes`)
- [ ] Namespace 创建成功
- [ ] Deployment 部署成功，所有 Pod 为 Running 状态
- [ ] Service 创建成功，Endpoints 非空
- [ ] 能够通过 port-forward 或 curl 测试 Pod 访问 nginx
- [ ] 能够查看 Pod 日志 (`kubectl logs`)
- [ ] 能够进入 Pod 执行命令 (`kubectl exec`)
- [ ] 能够完成扩缩容操作
- [ ] 能够完成滚动更新和回滚操作
- [ ] 能够识别 kube-system 中的各系统组件

---

<!-- chunk: 清理资源 -->## 清理资源

```bash
# ===== 清理应用资源 =====
kubectl delete namespace web-app
# 备注: 删除 Namespace 会自动删除其下所有资源 (Deployment、Service、Pod 等)

# ===== 删除集群 =====
# kind 集群
kind delete cluster --name learn-k8s
kind delete cluster --name dev-cluster  # 如果创建了第二个集群
# 预期输出: Deleting cluster "learn-k8s" ...

# minikube 集群
minikube delete
# 预期输出: 🔥  Deleting "minikube" ...

# 验证清理完成
kind get clusters       # 应无输出
docker ps              # 应无 kindest/node 容器
kubectl config get-contexts  # 查看是否还有残留的 context
```

---

<!-- chunk: kind vs minikube 对比 -->## kind vs minikube 对比

| 特性 | kind | minikube |
|------|------|---------|
| **底层实现** | Docker 容器 (每个节点=一个容器) | Docker / VM (VirtualBox/HyperKit) |
| **启动速度** | 快 (~30s) | 较慢 (~60-120s) |
| **资源占用** | 低 (~300MB/节点) | 中等 (~1GB) |
| **多节点支持** | 原生支持 (配置文件即可) | 需要 `minikube node add` |
| **Dashboard** | 需手动安装 | 内置插件 `minikube dashboard` |
| **LoadBalancer** | 不支持 (需 MetalLB) | 内置 `minikube tunnel` |
| **Ingress** | 需手动安装 + 端口映射 | 内置 `minikube addons enable ingress` |
| **本地镜像** | `kind load docker-image` | `minikube image load` 或 `eval $(minikube docker-env)` |
| **最佳场景** | CI/CD 测试、多节点模拟、快速迭代 | 本地开发、学习体验、功能探索 |

**选择建议**:
- 想要**快速轻量** → kind
- 想要**功能丰富、开箱即用** → minikube
- 用于 **CI/CD 流水线** → kind (启动快，易自动化)
- 用于 **日常开发调试** → minikube (Dashboard、tunnel 更方便)

#<!-- chunk: macOS 用户推荐：kind -->## macOS 用户推荐：kind

**对于 macOS 用户，kind 是最干净、最易维护的本地 K8s 方案**，理由如下：

| 维度 | kind 优势 |
|------|----------|
| **无 VM 层** | 完全运行在 Docker 容器内，没有 HyperKit/VirtualBox 等虚拟机开销 |
| **生命周期极简** | `kind create cluster` 创建，`kind delete cluster` 彻底删除，零残留 |
| **官方原版 K8s** | `kindest/node` 镜像由 K8s SIG Testing 官方维护，组件完全原版，无魔改 |
| **版本对齐** | 可通过 `--image kindest/node:v1.32.0` 精确指定 K8s 版本 |
| **资源轻量** | 约 300MB/节点 vs minikube 约 1GB |

> **结论**: macOS + Docker Desktop + kind = 最简洁的本地 K8s 组合，维护成本最低。
>
> 如果你需要体验 **kubeadm 手动部署流程**（模拟生产环境），则应使用 Lima + Ubuntu VM + kubeadm，而非 kind 或 minikube。

---

<!-- chunk: 常见问题 (FAQ) -->## 常见问题 (FAQ)

#<!-- chunk: Q1: kind 创建集群失败，提示 "Docker not running" -->## Q1: kind 创建集群失败，提示 "Docker not running"

```bash
# 检查 Docker 状态
docker info
# 如果报错，说明 Docker 没启动

# macOS/Windows: 启动 Docker Desktop 应用
# Linux: sudo systemctl start docker
```

#<!-- chunk: Q2: minikube start 卡住或超时 -->## Q2: minikube start 卡住或超时

```bash
# 1. 删除旧集群重试
minikube delete && minikube start --driver=docker

# 2. 如果是网络问题 (国内用户)，使用镜像加速
minikube start --driver=docker --image-mirror-country=cn

# 3. 如果还是失败，查看详细日志
minikube start --driver=docker --alsologtostderr -v=7
```

#<!-- chunk: Q3: kubectl 无法连接集群 -->## Q3: kubectl 无法连接集群

```bash
# 检查当前 context 是否正确
kubectl config current-context
# 预期: kind-learn-k8s 或 minikube

# 查看所有可用 context
kubectl config get-contexts

# 切换到正确的 context
kubectl config use-context kind-learn-k8s
# 或
kubectl config use-context minikube
```

#<!-- chunk: Q4: Pod 一直处于 Pending 状态 -->## Q4: Pod 一直处于 Pending 状态

```bash
# 查看 Pod 事件，了解为什么没被调度
kubectl describe pod <pod-name>
# 常见原因:
# - Insufficient cpu/memory: 节点资源不足 → 减少 replicas 或 降低 resource requests
# - 0/1 nodes are available: 没有可用节点 → 检查节点状态 kubectl get nodes
```

#<!-- chunk: Q5: 国内拉取镜像慢或失败 -->## Q5: 国内拉取镜像慢或失败

```bash
# kind: 使用预下载的节点镜像
kind create cluster --image kindest/node:v1.27.3

# minikube: 使用国内镜像源
minikube start --image-mirror-country=cn --registry-mirror=https://docker.mirrors.ustc.edu.cn

# 通用: 配置 Docker 镜像加速 (Docker Desktop → Settings → Docker Engine)
# {
#   "registry-mirrors": ["https://docker.mirrors.ustc.edu.cn"]
# }
```

#<!-- chunk: Q6: kind 集群重启后 kubectl 无法连接 -->## Q6: kind 集群重启后 kubectl 无法连接

```bash
# kind 集群在 Docker 重启后会自动恢复，但可能需要等待
docker ps | grep kindest  # 确认容器在运行
kubectl cluster-info      # 测试连接

# 如果 context 丢失
kind export kubeconfig --name learn-k8s
```

---

<!-- chunk: 附录 A：macOS 方案选型思考记录 -->## 附录 A：macOS 方案选型思考记录

> 以下记录了在 macOS 上选择本地 K8s 部署方案的完整决策过程。

#<!-- chunk: 问题 1：Mac 上最干净好维护的部署方式是 kind 吗？ -->## 问题 1：Mac 上最干净好维护的部署方式是 kind 吗？

**结论：是的。** kind 是 Mac 上最干净、最易维护的本地 K8s 方案。

核心理由：
- **没有 VM 层** — kind 完全运行在 Docker 容器内，不依赖 HyperKit/VirtualBox
- **生命周期极简** — `kind create cluster` / `kind delete cluster`，删除即干净，无隐藏状态、无残留文件
- **资源轻量** — 约 300MB/节点（minikube 约 1GB）
- **启动快** — ~30 秒（minikube 60–120 秒）

minikube 的优势在于开箱即用的 Dashboard、`minikube tunnel`（LoadBalancer 模拟）、`minikube addons enable ingress` 等功能，但这些都需要更多系统开销和隐藏状态。

#<!-- chunk: 问题 2：如果需要完整的官方 K8s 发行版，用 kind 还是 minikube？ -->## 问题 2：如果需要完整的官方 K8s 发行版，用 kind 还是 minikube？

**结论：用 kind。**

两者都不是严格意义上的 K8s 官方发行版，但 kind 最接近原版：

- **`kindest/node` 镜像由 Kubernetes SIG Testing 官方维护**，构建自 Kubernetes 源码
- 组件完全原版：`etcd + kube-apiserver + kube-controller-manager + kube-scheduler + kubelet + kube-proxy`
- 没有任何魔改，版本精确对齐官方 release
- 可通过 `--image kindest/node:v1.32.0` 指定精确版本

minikube 做了更多定制：内置驱动层、addon 系统、修改部分默认配置。

> **例外**：如果需要体验 **kubeadm 手动部署流程**（模拟真实生产环境），应使用 Lima + Ubuntu VM + kubeadm，而非 kind 或 minikube。kind 和 minikube 都跳过了 kubeadm 的手动流程。

#<!-- chunk: 问题 3：我需要 1 Master + 1 Worker，怎么部署？ -->## 问题 3：我需要 1 Master + 1 Worker，怎么部署？

**方案**：创建 `kind-config.yaml` 指定多节点拓扑，通过 `--config` 参数传入。

```yaml
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane   # Master 节点
- role: worker           # Worker 节点
```

```bash
kind create cluster --name my-k8s --image kindest/node:v1.32.0 --config kind-config.yaml
```

需要更多 Worker 只需在 `nodes` 列表中追加 `- role: worker` 即可。

#<!-- chunk: 最终选型决策 -->## 最终选型决策

| 决策项 | 选择 | 理由 |
|--------|------|------|
| 部署工具 | **kind** | 最干净、最轻量、官方组件原版 |
| K8s 版本 | **v1.32.0** | 当前最新稳定版 |
| 集群拓扑 | **1 Master + 1 Worker** | 可体验跨节点调度，同时资源占用可控 |
| 运行基座 | **Docker Desktop for Mac** | 已安装，kind 直接复用 |

---

<!-- chunk: 附录 B：macOS (Apple Silicon) 实战部署记录 -->## 附录 B：macOS (Apple Silicon) 实战部署记录

> **环境**: macOS Sequoia / Apple Silicon (arm64) / Docker Desktop 29.x  
> **日期**: 2026-03  
> **目标**: 使用 kind 部署 1 Master + 1 Worker 的官方 K8s v1.32.0 集群

#<!-- chunk: 环境确认 -->## 环境确认

```bash
$ docker -v
Docker version 29.2.1, build a5c7197
```

#<!-- chunk: 安装 kind 和 kubectl -->## 安装 kind 和 kubectl

```bash
$ brew install kind kubectl

# 实际安装版本:
# kind:       0.31.0 (arm64_sequoia)
# kubectl:    1.35.2 (brew formula) / Client Version: v1.34.1

$ kind version
kind v0.31.0 go1.25.5 darwin/arm64

$ kubectl version --client
Client Version: v1.34.1
Kustomize Version: v5.7.1
```

#<!-- chunk: 创建多节点集群配置 -->## 创建多节点集群配置

```bash
$ cat > kind-config.yaml << 'EOF'
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
- role: worker
EOF
```

#<!-- chunk: 创建集群 -->## 创建集群

```bash
$ kind create cluster \
  --name my-k8s \
  --image kindest/node:v1.32.0 \
  --config kind-config.yaml

Creating cluster "my-k8s" ...
 ✓ Ensuring node image (kindest/node:v1.32.0) 🖼
 ✓ Preparing nodes 📦 📦
 ✓ Writing configuration 📜
 ✓ Starting control-plane 🕹️
 ✓ Installing CNI 🔌
 ✓ Installing StorageClass 💾
 ✓ Joining worker nodes 🚜
Set kubectl context to "kind-my-k8s"
You can now use your cluster with:
kubectl cluster-info --context kind-my-k8s
Have a nice day! 👋
```

#<!-- chunk: 验证集群状态 -->## 验证集群状态

```bash
$ kubectl get nodes
NAME                   STATUS   ROLES           AGE   VERSION
my-k8s-control-plane   Ready    control-plane   34s   v1.32.0
my-k8s-worker          Ready    <none>          25s   v1.32.0
```

> **结果**: 1 Master + 1 Worker 全部 Ready，K8s v1.32.0 官方发行版，从 `brew install` 到集群就绪约 3 分钟。

---

**下一步**: 掌握本地 Demo 后，前往 → [02-single-node-deployment.md](./02-single-node-deployment.md) 学习在真实 Linux 上部署 K8s。

---

**来源文档**: `domain-11-production-operations/topic-learn/projects/p1-k8s-cluster-setup.md`, `domain-11-production-operations/topic-learn/week-1-foundation/day-5-k8s-architecture.md`, `domain-01-cluster-fundamentals/12-cluster-deployment-patterns.md`

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-08-release-change-management/topic-deployment/MOC.md|topic-deployment MOC]]
- [[domain-08-release-change-management/topic-deployment/README.md|Kubernetes 部署方案指南 (Deployment Guide)]]
- [[domain-08-release-change-management/topic-deployment/02-single-node-deployment.md|02 - 单节点部署 (Single Node All-in-One)]]
- [[domain-08-release-change-management/topic-deployment/03-development-environment-deployment.md|03 - 研发环境部署 (Development Environment Deployment)]]
- [[domain-08-release-change-management/topic-deployment/04-production-environment-deployment.md|04 - 生产环境部署 (Production Environment Deployment)]]

## Related

- [[README.md|README]]
- [[MOC.md|MOC]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux.md|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
