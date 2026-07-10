---
title: 本地零成本 K8s 实验环境搭建手册
description: 面向小白的本地 K8s 实验环境完整指南，覆盖 kind、minikube、k3d 三种方案，支持 Windows/macOS/Linux。零云厂商依赖、零费用，笔记本上跑通全部
  K8s 核心概念
summary: 面向小白的本地 K8s 实验环境完整指南，覆盖 kind、minikube、k3d 三种方案，支持 Windows/macOS/Linux。零云厂商依赖、零费用，笔记本上跑通全部
  K8s 核心概念
category: learning
tags:
- tutorial
- beginner
- lab
- kind
- minikube
- hands-on
- etcd
- apiserver
- kubelet
- scheduler
tier: supporting
created: '2026-05-23'
last_updated: 2026-05-21
difficulty: beginner
reading_level: beginner
audience:
- 零基础初学者
- 无云账号的学生
- 在职自学者
estimated_read_time: 30min
intent_queries:
- 本地搭建 K8s 集群
- kind 安装教程
- minikube 安装
- 零成本学 K8s
trigger_keywords:
- 本地环境
- kind
- minikube
- 实验环境
- 零成本
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 本地零成本 K8s 实验环境搭建手册

> **目标**: 在你的笔记本上，30 分钟内跑起一个完整的 K8s 集群，零费用、零云账号。  
> **适用系统**: Windows 11 / macOS 13+ / Linux (Ubuntu 22.04+)  
> **前提**: 能执行基本命令行操作（复制粘贴即可）

---

## 方案选择速查

| 方案 | 资源占用 | 启动速度 | 多节点支持 | 推荐场景 |
|------|---------|---------|-----------|---------|
| **kind** | 低（2GB 内存） | 快（30 秒） | ✅ 原生支持 | **首选推荐** — 学习、测试、CI |
| **minikube** | 中（2-4GB 内存） | 中（1-2 分钟） | ⚠️ 需驱动 | 需要完整 K8s 特性 |
| **k3d** | 极低（1GB 内存） | 极快（15 秒） | ✅ 原生支持 | 资源紧张的老电脑 |

**小白建议**: 直接选 **kind**。它轻量、快、稳定，CNCF 官方推荐。

---

## 前置依赖安装

### 1. 安装 Docker Desktop

所有方案都需要 Docker 作为容器运行时。

**Windows / macOS**:
1. 访问 https://www.docker.com/products/docker-desktop/
2. 下载对应系统版本，双击安装
3. 启动 Docker Desktop，确保左下角显示 🟢 "Engine running"

**Linux (Ubuntu)**:
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 一键安装 Docker
curl -fsSL https://get.docker.com | sh

# 将当前用户加入 docker 组（免 sudo）
sudo usermod -aG docker $USER
newgrp docker

# 验证
docker run hello-world
```
> 💡 **验证**: 在终端输入 `docker version`，看到 Client 和 Server 信息即成功。

### 2. 安装 kubectl

kubectl 是操作 K8s 集群的命令行工具。

**Windows (PowerShell)**:
``` powershell
# 🟢 低风险：只读/信息收集，通常无副作用
# 用 winget 安装
winget install -e --id Kubernetes.kubectl

# 验证
kubectl version --client
```
**macOS**:
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 用 Homebrew 安装
brew install kubectl

# 验证
kubectl version --client
```
**Linux**:
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 下载最新版
curl -LO "https://dl.k8s/release/$(curl -L -s https://dl.k8s/release/stable.txt)/bin/linux/amd64/kubectl"

# 安装
sudo install -o root -g root -m 0755 kubectl /usr/local/bin/kubectl

# 验证
kubectl version --client
```
### 3. 配置 kubectl 自动补全（强烈推荐）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Bash 用户
echo 'source <(kubectl completion bash)' >> ~/.bashrc
source ~/.bashrc

# Zsh 用户
echo 'source <(kubectl completion zsh)' >> ~/.zshrc
source ~/.zshrc
```
> 💡 按 `<Tab>` 键自动补全命令，考试和日常都能救命。

---

## 方案 A：kind（推荐 ⭐）

kind = **K**ubernetes **in** **D**ocker。它在 Docker 容器里运行 K8s 节点，轻量且快。

### 安装 kind

**Windows/macOS/Linux**:
```bash
# 通用安装脚本（curl）
curl -Lo ./kind https://kind.sigs.k8s.io/dl/v0.23.0/kind-$(uname)-amd64
chmod +x ./kind
sudo mv ./kind /usr/local/bin/kind

# macOS 也可以用 Homebrew
brew install kind

# Windows 也可以用 Chocolatey
choco install kind

# 验证
kind version
```

### 创建单节点集群（最简单）

```bash
# 创建一个名为 k8s-lab 的集群
kind create cluster --name k8s-lab
```

输出示例：
```
# 🟢 低风险：只读/信息收集，通常无副作用
Creating cluster "k8s-lab" ...
 ✓ Ensuring node image (kindest/node:v1.30.0) 🖼
 ✓ Preparing nodes 📦
 ✓ Writing configuration 📜
 ✓ Starting control-plane 🕹️
 ✓ Installing CNI 🔌
 ✓ Installing StorageClass 💾
Set kubectl context to "kind-k8s-lab"
```
> 💡 默认使用最新稳定版 K8s（如 v1.30）。如需特定版本：
> ```bash
> kind create cluster --name k8s-lab --image kindest/node:v1.29.0
> ```

### 验证集群

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点
kubectl get nodes

# 输出
NAME                    STATUS   ROLES           AGE   VERSION
k8s-lab-control-plane   Ready    control-plane   2m    v1.30.0

# 查看系统 Pod
kubectl get pods -n kube-system

# 输出（所有 Pod 状态为 Running）
NAME                                         READY   STATUS
coredns-...                                  1/1     Running
etcd-k8s-lab-control-plane                   1/1     Running
kindnet-...                                  1/1     Running
kube-apiserver-k8s-lab-control-plane         1/1     Running
kube-controller-manager-k8s-lab-control-plane 1/1    Running
kube-proxy-...                               1/1     Running
kube-scheduler-k8s-lab-control-plane         1/1     Running
```
🎉 **恭喜你！你的第一台 K8s 集群已经跑起来了！**

### 创建多节点集群（进阶）

创建配置文件 `kind-multi-node.yaml`：

```yaml
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
- role: worker
- role: worker
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 删除旧集群（如需）
kind delete cluster --name k8s-lab

# 创建多节点集群
kind create cluster --name k8s-lab --config kind-multi-node.yaml

# 验证
kubectl get nodes
# NAME                    STATUS   ROLES           AGE
# k8s-lab-control-plane   Ready    control-plane   1m
# k8s-lab-worker          Ready    <none>          1m
# k8s-lab-worker2         Ready    <none>          1m
```
### kind 常用命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有 kind 集群
kind get clusters

# 删除集群
kind delete cluster --name k8s-lab

# 导出 kubeconfig
kind export kubeconfig --name k8s-lab

# 加载本地镜像到集群（不用推送到仓库）
kind load docker-image my-app:v1 --name k8s-lab

# 进入节点容器排查
docker exec -it k8s-lab-control-plane bash
```
### kind 的 [[Ingress|Ingress]]（本地访问服务）

kind 集群默认无法从宿主机直接访问 NodePort。需要额外配置：

```yaml
# kind-ingress.yaml
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
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
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kind create cluster --name k8s-lab --config kind-ingress.yaml
kubectl apply -f https://raw.githubusercontent.com/kubernetes/ingress-nginx/main/deploy/static/provider/kind/deploy.yaml
```
---

## 方案 B：minikube

minikube 是 K8s 官方维护的本地集群工具，历史最久、文档最全。

### 安装 minikube

**macOS**:
```bash
brew install minikube
```

**Windows**:
```powershell
winget install -e --id Kubernetes.minikube
```

**Linux**:
```bash
curl -LO https://storage.googleapis.com/minikube/releases/latest/minikube-linux-amd64
sudo install minikube-linux-amd64 /usr/local/bin/minikube
```

### 启动集群

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启动（自动选择最佳驱动：Docker > Hyper-V > VirtualBox）
minikube start

# 指定驱动（推荐 Docker）
minikube start --driver=docker

# 指定 K8s 版本
minikube start --kubernetes-version=v1.29.0

# 多节点（实验性功能）
minikube start --nodes=3
```
### minikube 特色功能

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 打开 K8s 仪表盘（图形化管理界面）
minikube dashboard

# 快速部署一个应用（自带示例）
minikube kubectl -- create deployment hello-minikube --image=kicbase/echo-server:1.0
minikube kubectl -- expose deployment hello-minikube --type=NodePort --port=8080

# 获取服务访问地址
minikube service hello-minikube --url
# http://192.168.49.2:30001

# 挂载宿主机目录到集群
minikube mount /host/path:/minikube-host

# SSH 进入节点
minikube ssh

# 暂停/恢复（省电）
minikube pause
minikube unpause

# 删除集群
minikube delete
```
### minikube 常用插件

```bash
# 启用 Ingress 控制器
minikube addons enable ingress

# 启用 Metrics Server（HPA 需要）
minikube addons enable metrics-server

# 启用 Dashboard
minikube addons enable dashboard

# 列出所有插件
minikube addons list
```

---

## 方案 C：k3d（轻量之选）

k3d 在 Docker 里跑 [[实体/k3s.md|k3s]]](https://k3s.io/)（Rancher 出品的轻量 K8s 发行版），资源占用极低。

### 安装 k3d

```bash
# 通用安装
curl -s https://raw.githubusercontent.com/k3d-io/k3d/main/install.sh | bash

# macOS Homebrew
brew install k3d

# Windows Chocolatey
choco install k3d
```

### 创建集群

```bash
# 单节点
k3d cluster create k8s-lab

# 多节点（1 控制平面 + 2 工作节点）
k3d cluster create k8s-lab --servers 1 --agents 2

# 带端口映射（本地访问）
k3d cluster create k8s-lab -p "80:80@loadbalancer" -p "443:443@loadbalancer"
```

### k3d 常用命令

```bash
# 列出集群
k3d cluster list

# 删除集群
k3d cluster delete k8s-lab

# 启动/停止
k3d cluster start k8s-lab
k3d cluster stop k8s-lab

# 加载镜像
k3d image load my-app:v1 -c k8s-lab
```

---

## 环境验证清单

完成安装后，逐条验证：

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl apply/create/replace`：创建/变更集群资源

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
# ✅ 1. 集群状态正常
kubectl get nodes
# 期望：所有节点 STATUS = Ready

# ✅ 2. 核心组件运行正常
kubectl get pods -n kube-system
# 期望：所有 Pod STATUS = Running

# ✅ 3. 能创建资源
kubectl create namespace test
kubectl get namespace test
# 期望：test namespace 存在

# ✅ 4. 能部署应用
kubectl run nginx --image=nginx -n test
kubectl get pods -n test
# 期望：nginx Pod Running

# ✅ 5. 能暴露服务
kubectl expose pod nginx --port=80 --type=NodePort -n test
kubectl get svc -n test
# 期望：看到 NodePort 服务

# ✅ 6. 能访问服务（minikube）
# minikube service nginx -n test --url

# ✅ 7. 清理测试资源
kubectl delete namespace test  # ⚠️ 不可逆：永久删除命名空间及全部资源
```
全部通过 = 你的实验环境 100% 就绪！

---

## 实验环境最佳实践

### 1. 固定 kubectl 上下文

如果你同时有多个集群（本地 kind + 公司集群），注意切换上下文：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有上下文
kubectl config get-contexts

# 切换到本地集群
kubectl config use-context kind-k8s-lab
# 或
kubectl config use-context minikube

# 设置当前上下文别名（推荐）
alias k=kubectl
alias klocal='kubectl --context kind-k8s-lab'
```
### 2. 资源监控

本地电脑资源有限，注意监控：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点资源使用
kubectl top nodes

# 查看 Pod 资源使用
kubectl top pods --all-namespaces
```
> 💡 如果 `top` 命令报错，说明没装 metrics-server。minikube 用 `minikube addons enable metrics-server`，kind 需手动部署。

### 3. 定期清理

实验产生的资源要及时清理，避免资源泄漏：

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 删除所有实验 namespace
kubectl delete ns $(kubectl get ns -o jsonpath='{.items[*].metadata.name}' | tr ' ' '\n' | grep -E 'test|demo|lab')  # ⚠️ 不可逆：永久删除命名空间及全部资源

# 或者暴力重建集群（kind 30 秒搞定）
kind delete cluster --name k8s-lab
kind create cluster --name k8s-lab
```
### 4. 镜像加速

国内网络拉取镜像慢，建议配置镜像加速：

**Docker Desktop**:
- Settings → Docker Engine → 添加 `"registry-mirrors": ["https://docker.mirrors.ustc.edu.cn"]`

**kind 配置镜像仓库**:
```yaml
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
containerdConfigPatches:
- |-
  [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
    endpoint = ["https://docker.mirrors.ustc.edu.cn"]
```

---

## 常见问题排错

### Q1: Docker 未运行
```
# 🟢 低风险：只读/信息收集，通常无副作用
ERROR: failed to create cluster: docker failed to start
```
**解决**: 启动 Docker Desktop，确认左下角是绿色状态。

### Q2: 端口被占用
```
ERROR: failed to create cluster: node already exists
```
**解决**: 

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `docker prune/rm -f`：强制清理镜像/容器/卷，运行中容器会被杀

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
kind delete cluster --name k8s-lab
# 或
docker ps -a | grep k8s-lab
docker rm -f $(docker ps -a | grep k8s-lab | awk '{print $1}')  # ⚠️ 强制清理，可能杀运行中容器
```
### Q3: 内存不足
```
The node was low on resource: memory
```
**解决**: 
- 关闭不必要的应用
- 使用 k3d 替代 kind/minikube
- 限制集群资源：`kind create cluster --name k8s-lab --config <限制内存的 yaml>`

### Q4: Windows 上 kubectl 命令找不到
**解决**: 把 kubectl 所在目录加入系统 PATH 环境变量。

### Q5: 权限不足（Linux）
```
# 🟢 低风险：只读/信息收集，通常无副作用
permission denied while trying to connect to Docker daemon
```
**解决**: 
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
sudo usermod -aG docker $USER
# 退出重新登录，或执行：
newgrp docker
```
---

## 下一步

环境搭好了，现在可以开始 K8s 核心概念实验：

| 序号 | 实验 | 对应文档 |
|------|------|---------|
| 1 | 跑第一个 Pod | [../fundamentals/02-pod-basics.md](../fundamentals/02-pod-basics.md) |
| 2 | 用 Deployment 部署应用 | [../fundamentals/03-deployment-basics.md](../fundamentals/03-deployment-basics.md) |
| 3 | 用 Service 暴露服务 | [../fundamentals/04-service-basics.md](../fundamentals/04-service-basics.md) |
| 4 | 配置 Ingress | [../fundamentals/05-ingress-basics.md](../fundamentals/05-ingress-basics.md) |
| 5 | 挂载存储 | [../fundamentals/08-pv-pvc-basics.md](../fundamentals/08-pv-pvc-basics.md) |

---

**关联文档**:
- [[01-cloud-native-evolution-story]] — 上一课：为什么需要 K8s
- ../fundamentals/02-pod-basics.md — 下一课：跑第一个 Pod
- ../public-training/one-month/resources/commands-cheatsheet.md — kubectl 命令速查表


<!-- risk-assessed -->
