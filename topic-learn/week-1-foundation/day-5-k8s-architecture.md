# Day 5: Kubernetes 架构全貌

> **学习时间**: 4-5 小时 | **主题**: K8s 核心架构与组件 (核心日)

---

## 今日目标

- [ ] 理解 Kubernetes 整体架构 (Master/Node)
- [ ] 掌握各核心组件的职责和交互方式
- [ ] 能够搭建本地 K8s 集群

---

## 理论学习 (2h) - 精读

### 必读文档 (核心)

1. **Kubernetes 架构总览** ⭐
   - 文件: `../../domain-1-architecture-fundamentals/01-kubernetes-architecture-overview.md`
   - 重点: Master/Node 架构、控制平面组件、数据平面组件
   - **这是本周最重要的文档，请精读**

2. **核心组件深入**
   - 文件: `../../domain-1-architecture-fundamentals/02-core-components-deep-dive.md`
   - 重点: etcd、API Server、Controller Manager、Scheduler、kubelet、kube-proxy

### 阅读要点

**控制平面组件:**
- **etcd**: 分布式键值存储，存储所有集群状态
- **API Server**: 集群网关，所有组件通信的枢纽
- **Controller Manager**: 运行各种控制器 (Deployment、ReplicaSet 等)
- **Scheduler**: 决定 Pod 运行在哪个节点

**数据平面组件:**
- **kubelet**: 节点代理，管理 Pod 生命周期
- **kube-proxy**: 实现 Service 网络代理
- **Container Runtime**: 运行容器 (containerd、CRI-O)

---

## 实践任务 (2.5h)

### 任务 1: 搭建本地 K8s 集群 (1h)

选择一种方式搭建:

**方式 A: 使用 kind (推荐，轻量)**

```bash
# 安装 kind
# macOS
brew install kind

# Linux
curl -Lo ./kind https://kind.sigs.k8s.io/dl/v0.20.0/kind-linux-amd64
chmod +x ./kind
sudo mv ./kind /usr/local/bin/kind

# 创建集群
kind create cluster --name learn-k8s

# 验证集群
kubectl cluster-info
kubectl get nodes
```

**方式 B: 使用 minikube**

```bash
# 安装 minikube
# macOS
brew install minikube

# Linux
curl -LO https://storage.googleapis.com/minikube/releases/latest/minikube-linux-amd64
sudo install minikube-linux-amd64 /usr/local/bin/minikube

# 启动集群
minikube start --driver=docker

# 验证
kubectl get nodes
```

### 任务 2: 探索集群组件 (45min)

```bash
# 查看系统组件
kubectl get pods -n kube-system

# 查看各组件详情
kubectl describe pod -n kube-system -l component=etcd
kubectl describe pod -n kube-system -l component=kube-apiserver
kubectl describe pod -n kube-system -l component=kube-controller-manager
kubectl describe pod -n kube-system -l component=kube-scheduler

# 查看节点详情
kubectl describe node

# 查看集群信息
kubectl cluster-info dump | head -100

# 查看 API 资源
kubectl api-resources | head -20
kubectl api-versions
```

### 任务 3: kubectl 基础命令 (45min)

参考 `../../domain-1-architecture-fundamentals/05-kubectl-commands-reference.md`:

```bash
# 基础查询
kubectl get pods                      # 查看 Pod
kubectl get pods -o wide              # 详细信息
kubectl get pods -o yaml              # YAML 格式
kubectl get all                       # 所有资源

# 资源描述
kubectl describe pod <pod-name>
kubectl describe node <node-name>

# 创建资源
kubectl create namespace test-ns
kubectl run nginx --image=nginx --dry-run=client -o yaml

# 应用配置
kubectl apply -f <file.yaml>
kubectl delete -f <file.yaml>

# 日志和调试
kubectl logs <pod-name>
kubectl logs -f <pod-name>            # 实时日志
kubectl exec -it <pod-name> -- /bin/sh

# 资源管理
kubectl scale deployment <name> --replicas=3
kubectl rollout status deployment <name>
kubectl rollout history deployment <name>

# 集群管理
kubectl top nodes                     # 节点资源使用 (需要 metrics-server)
kubectl top pods                      # Pod 资源使用
```

---

## 费曼复述 (0.5h)

**核心任务: 画出 K8s 架构图**

在纸上或白板上画出完整的 K8s 架构图，包含:

1. **控制平面**:
   - etcd (数据存储)
   - API Server (通信中心)
   - Controller Manager (控制循环)
   - Scheduler (调度决策)

2. **数据平面 (Worker Node)**:
   - kubelet
   - kube-proxy
   - Container Runtime

3. **组件间通信箭头**:
   - 所有组件都通过 API Server 通信
   - kubelet 向 API Server 汇报状态
   - Scheduler 从 API Server 获取待调度 Pod

用自己的语言回答:

1. **当你执行 `kubectl apply -f deployment.yaml` 时，发生了什么？**
   - kubectl -> API Server -> etcd (存储)
   - Controller Manager 发现新 Deployment，创建 ReplicaSet
   - ReplicaSet Controller 创建 Pod
   - Scheduler 为 Pod 分配节点
   - kubelet 在节点上创建容器

2. **etcd 为什么这么重要？如果 etcd 挂了会怎样？**

3. **API Server 为什么是"唯一入口"？**

---

## 今日检验

- [ ] 成功搭建本地 K8s 集群
- [ ] 能够使用 kubectl 查看集群状态
- [ ] 能够画出 K8s 架构图并解释各组件职责
- [ ] 理解 kubectl 命令执行的完整流程

---

## 核心概念总结

| 组件 | 职责 | 关键点 |
|------|------|--------|
| etcd | 存储集群状态 | 只有 API Server 能直接访问 |
| API Server | 集群网关 | 认证、授权、准入控制 |
| Controller Manager | 维护期望状态 | Reconcile 循环 |
| Scheduler | Pod 调度 | Filter + Score |
| kubelet | 节点代理 | 管理 Pod 生命周期 |
| kube-proxy | 网络代理 | Service -> Pod 转发 |

---

## 明日预告

Day 6 将深入 K8s 架构，学习集群配置参数，并部署第一个 Deployment。
