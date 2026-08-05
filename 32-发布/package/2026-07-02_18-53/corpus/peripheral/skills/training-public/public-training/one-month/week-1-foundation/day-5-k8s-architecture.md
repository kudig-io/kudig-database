---
title: 'Day 5: Kubernetes 架构全貌'
description: '# Day 5: Kubernetes 架构全貌'
summary: '本文是整个 K8s 学习路径中最重要的一课。你将深入理解 Kubernetes 的 Master-Node 架构，掌握每个核心组件的职责和交互方式，并亲手搭建一个本地 K8s 集群。理解 K8s 架构是所有后续学习的基础——无论是部署应用、排查问题还是设计架构，都需要回到这张架构图。'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- coredns
- containerd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 5: Kubernetes 架构全貌 是什么'
- '如何 Day 5: Kubernetes 架构全貌'
trigger_keywords:
- Day
- '5:'
- Kubernetes
- 架构全貌
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 5: [[Kubernetes|Kubernetes]] 架构全貌

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY5
title: Day 5 - Kubernetes 架构全貌
topic: kubernetes
type: hands-on-guide
tags: [kubernetes, architecture, master, node, etcd, apiserver, scheduler, kubelet, hands-on, week-1]
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - "K8s 整体架构是什么"
  - "Master/Node 组件有哪些"
  - "kubectl apply 执行流程"
  - "kind 集群怎么搭建"
trigger_keywords:
  - Kubernetes
  - Master
  - Node
  - etcd
  - API Server
  - Scheduler
  - Controller Manager
  - kubelet
  - kube-proxy
  - kind
  - minikube
  - kubectl
  - 架构图
reading_level: beginner
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 45min
related_domains:
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - kubernetes
  - architecture
  - kubectl
related:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/day-6-k8s-cluster.md
  - domain-01-cluster-fundamentals/01-kubernetes-architecture-overview.md
---

```

> **学习时间**: 4-5 小时 | **主题**: K8s 核心架构与组件 (核心日)

---

## 概述

本文是整个 K8s 学习路径中最重要的一课。你将深入理解 Kubernetes 的 Master-Node 架构，掌握每个核心组件的职责和交互方式，并亲手搭建一个本地 K8s 集群。理解 K8s 架构是所有后续学习的基础——无论是部署应用、排查问题还是设计架构，都需要回到这张架构图。

### 学习目标

- 理解 Kubernetes 整体架构（Master/Node）和组件间的交互方式
- 掌握各核心组件的职责：etcd、API Server、Controller Manager、Scheduler、kubelet、kube-proxy
- 能够使用 kind 或 minikube 搭建本地 K8s 集群
- 理解 `kubectl apply` 的完整事件链

---

## 核心概念详解

### Master-Node 架构

Kubernetes 采用 Master-Node（也叫控制平面-数据平面）架构。Master 负责集群的全局决策（调度、检测和响应集群事件），Node 负责运行实际的工作负载（Pod）。

**控制平面（Master）组件**运行在 Master 节点上（或由云服务托管）：

- **etcd**: 分布式键值存储，保存集群的所有状态数据（Pod、[[Service|Service]]、ConfigMap、Secret 等定义）。etcd 是集群的"唯一真实来源"（Single Source of Truth）。只有 API Server 能直接访问 etcd，其他组件都通过 API Server 间接读写数据。etcd 使用 Raft 共识协议保证数据一致性，对磁盘 IO 延迟非常敏感（建议使用 SSD）

- **kube-apiserver**: 所有操作的入口。它提供 RESTful API，处理认证（Authentication）、授权（Authorization）和准入控制（Admission Control），然后将数据持久化到 etcd。所有组件（kubectl、kubelet、scheduler、controller-manager）都通过 API Server 交互。API Server 支持 Watch 机制，允许客户端订阅资源变化事件

- **kube-controller-manager**: 运行多种控制器的进程。每个控制器遵循 Reconcile（调和）模式：Watch 资源变化 → 对比期望状态与实际状态 → 执行操作。核心控制器包括：
  - Deployment Controller: 管理滚动更新
  - [[ReplicaSet|ReplicaSet]] Controller: 维护 Pod 副本数
  - Node Controller: 监控节点健康
  - Service Account Controller: 管理服务账号
  - Namespace Controller: 清理已删除命名空间的资源

- **kube-scheduler**: 负责将未调度的 Pod 分配到合适的节点。调度过程分两阶段：
  - Filter（过滤）: 排除不满足条件的节点（资源不足、污点不匹配、亲和性不符等）
  - [[Score|Score]]（打分）: 对候选节点打分排序（资源均衡、镜像本地性、亲和性偏好等）

**数据平面（Node）组件**运行在每个工作节点上：

- **kubelet**: 节点代理。它通过 Watch API Server 获取分配到本节点的 Pod 定义，调用容器运行时创建和管理容器，执行探针检查，并汇报节点和 Pod 的状态

- **kube-proxy**: 负责实现 Service 的网络转发规则。它维护节点上的 iptables 或 IPVS 规则，将 Service ClusterIP 的流量转发到实际的 Pod IP

- **Container Runtime**: 容器运行时（通常是 containerd），负责拉取镜像、创建和运行容器。K8s 通过 CRI（Container Runtime Interface）与运行时交互

### 组件间通信流程

理解组件间的通信方式对于排障至关重要：

```
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl → API Server (HTTPS) → etcd (gRPC)
API Server ← Watch ← Scheduler (获取待调度 Pod)
API Server ← Watch ← Controller Manager (监听资源变化)
API Server ← Watch ← kubelet (获取 Pod 定义，上报状态)
kubelet → containerd (gRPC/CRI) → 容器
kube-proxy ← Watch ← API Server (Service/Endpoints 变化) → iptables/IPVS

```
### kubectl apply 的完整事件链

当你执行 `kubectl apply -f deployment.yaml` 时：

1. kubectl 读取 YAML，通过 HTTPS POST 发送到 API Server
2. API Server 执行认证（验证 kubectl 的身份）
3. API Server 执行授权（RBAC 检查是否有权限创建 Deployment）
4. API Server 执行准入控制（如 Namespace 是否存在、资源配额是否充足等）
5. API Server 将资源定义序列化并写入 etcd
6. Controller Manager 通过 Watch 发现新的 Deployment
7. Deployment Controller 创建对应的 ReplicaSet
8. ReplicaSet Controller 创建 Pod 对象
9. Scheduler 通过 Watch 发现未调度的 Pod，选择合适的节点
10. kubelet 通过 Watch 发现分配到本节点的 Pod
11. kubelet 调用 containerd 拉取镜像并启动容器
12. kubelet 通过 readinessProbe 检查容器就绪状态

---

## 实战演练

### 任务 1: 搭建本地 K8s 集群 (1h)

选择一种方式搭建：

**方式 A: 使用 kind（推荐，轻量）**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 kind
# macOS
brew install kind

# Linux
curl -Lo ./kind https://kind.sigs.k8s.io/dl/v0.20.0/kind-linux-amd64
chmod +x ./kind
sudo mv ./kind /usr/local/bin/kind

# 创建多节点集群
kind create cluster --name learn-k8s --config - <<EOF
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
- role: worker
- role: worker
EOF
# 预期输出:
# Creating cluster "learn-k8s" ...
# ✓ Control plane node ready
# ✓ Worker node 1 ready
# ✓ Worker node 2 ready
# Cluster creation complete.

# 验证集群
kubectl cluster-info
# 预期输出:
# Kubernetes control plane is running at https://127.0.0.1:xxxxx
# CoreDNS is running at https://127.0.0.1:xxxxx/api/v1/namespaces/kube-system/services/kube-dns:dns/proxy

kubectl get nodes
# 预期输出:
# NAME                  STATUS   ROLES           AGE   VERSION
# learn-k8s-control-plane Ready  control-plane   1m    v1.28.0
# learn-k8s-worker       Ready   <none>          1m    v1.28.0
# learn-k8s-worker2      Ready   <none>          1m    v1.28.0
```
**方式 B: 使用 minikube**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 minikube
# macOS
brew install minikube

# Linux
curl -LO https://storage.googleapis.com/minikube/releases/latest/minikube-linux-amd64
sudo install minikube-linux-amd64 /usr/local/bin/minikube

# 启动集群
minikube start --driver=docker --nodes=3
# 预期输出:
# Starting control plane node ...
# Started worker node 1 ...
# Started worker node 2 ...

# 验证
kubectl get nodes
```
### 任务 2: 探索集群组件 (45min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看系统组件
kubectl get pods -n kube-system
# 预期输出:
# NAME                                        READY   STATUS    RESTARTS   AGE
# etcd-learn-k8s-control-plane                1/1     Running   0          5m
# kube-apiserver-learn-k8s-control-plane      1/1     Running   0          5m
# kube-controller-manager-learn-k8s-control-plane  1/1 Running 0 5m
# kube-scheduler-learn-k8s-control-plane      1/1     Running   0          5m
# coredns-7f6cb4b4f7-abc12                    1/1     Running   0          5m
# kube-proxy-xxxxx                            1/1     Running   0          5m
# kindnet-xxxxx                               1/1     Running   0          5m

# 查看各组件详情
kubectl describe pod -n kube-system -l component=etcd | head -30
# 预期输出: etcd Pod 的配置和状态

kubectl describe pod -n kube-system -l component=kube-apiserver | head -30
# 预期输出: API Server 的启动参数（--etcd-servers, --service-cluster-ip-range 等）

kubectl describe pod -n kube-system -l component=kube-controller-manager | head -30
# 预期输出: Controller Manager 的配置

kubectl describe pod -n kube-system -l component=kube-scheduler | head -30
# 预期输出: Scheduler 的配置

# 查看节点详情
kubectl describe node
# 重点关注:
# - Labels: 节点的标签
# - Conditions: 节点的健康状态
# - Capacity/Allocatable: 节点的资源总量和可分配量
# - System Info: 操作系统和容器运行时信息
# - Non-terminated Pods: 节点上运行的 Pod

# 查看集群详细信息
kubectl cluster-info dump | head -100
# 预期输出: 集群的详细状态转储

# 查看 API 资源列表
kubectl api-resources | head -20
# 预期输出:
# NAME        SHORTNAMES   APIVERSION        NAMESPACED   KIND
# bindings                 v1                true         Binding
# configmaps   cm          v1                true         ConfigMap
# endpoints    ep          v1                true         Endpoints
# events       ev          v1                true         Event
# namespaces   ns          v1                false        Namespace
# nodes        no          v1                false        Node
# pods         po          v1                true         Pod
# ...

# 查看所有 API 版本
kubectl api-versions
# 预期输出: 所有注册的 APIGroup/Version 组合
```
### 任务 3: kubectl 基础命令 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 基础查询
kubectl get pods                       # 查看 Pod
kubectl get pods -o wide              # 详细信息（含 IP、节点）
kubectl get pods -o yaml              # YAML 格式输出
kubectl get all                       # 查看所有资源类型

# 资源描述
kubectl describe pod <pod-name>        # 查看 Pod 详情（含 Events）
kubectl describe node <node-name>      # 查看节点详情

# 创建资源
kubectl create namespace test-ns
# 预期输出: namespace/test-ns created

kubectl run nginx --image=nginx --dry-run=client -o yaml
# 预期输出: 生成 Pod YAML 模板（不实际创建）

# 应用配置
kubectl apply -f <file.yaml>           # 声明式创建/更新
kubectl delete -f <file.yaml>          # 删除 YAML 定义的所有资源

# 日志和调试
kubectl logs <pod-name>                # 查看 Pod 日志
kubectl logs -f <pod-name>             # 实时跟踪日志
kubectl exec -it <pod-name> -- /bin/sh # 进入容器 Shell

# 资源管理
kubectl scale deployment <name> --replicas=3
kubectl rollout status deployment <name>
kubectl rollout history deployment <name>

# 集群管理
kubectl top nodes                     # 节点资源使用（需要 metrics-server）
kubectl top pods                      # Pod 资源使用

# 输出格式化
kubectl get pods -o jsonpath='{.items[*].metadata.name}'
# 预期输出: 所有 Pod 名称（空格分隔）

kubectl get pods -o custom-columns='NAME:.metadata.name,STATUS:.status.phase,NODE:.spec.nodeName'
# 预期输出: 自定义列格式
```
---

## 配置示例

### kind 多节点集群配置

```yaml
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
name: learning-cluster
nodes:
- role: control-plane
  kubeadmConfigPatches:
  - |
    kind: InitConfiguration
    nodeRegistration:
      kubeletExtraArgs:
        node-labels: "node-role=control-plane"
- role: worker
  kubeadmConfigPatches:
  - |
    kind: JoinConfiguration
    nodeRegistration:
      kubeletExtraArgs:
        node-labels: "node-role=worker,zone=a"
- role: worker
  kubeadmConfigPatches:
  - |
    kind: JoinConfiguration
    nodeRegistration:
      kubeletExtraArgs:
        node-labels: "node-role=worker,zone=b"
networking:
  podSubnet: "10.244.0.0/16"
  serviceSubnet: "10.96.0.0/12"
```

### 基础资源 YAML 模板

```yaml
# Namespace
apiVersion: v1
kind: Namespace
metadata:
  name: demo
---
# Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
  namespace: demo
spec:
  replicas: 2
  selector:
    matchLabels:
      app: nginx
  template:
    metadata:
      labels:
        app: nginx
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
---
# Service
apiVersion: v1
kind: Service
metadata:
  name: nginx
  namespace: demo
spec:
  selector:
    app: nginx
  ports:
  - port: 80
    targetPort: 80
  type: ClusterIP

```

---

## 常见问题

### Q1: 当你执行 kubectl apply 时发生了什么？

kubectl 将 YAML 通过 HTTPS 发送到 API Server → API Server 认证/授权/准入控制 → 数据写入 etcd → Controller Manager 通过 Watch 发现变化并创建 ReplicaSet → ReplicaSet Controller 创建 Pod → Scheduler 为 Pod 选择节点 → kubelet 在节点上启动容器。

### Q2: etcd 为什么这么重要？

etcd 存储了集群的所有状态数据，是唯一的真实来源。如果 etcd 不可用，API Server 无法读写数据，整个集群变为只读——现有的 Pod 继续运行，但无法创建/更新/删除任何资源。如果 etcd 数据丢失，集群需要从备份恢复或重建。

### Q3: API Server 为什么是"唯一入口"？

所有组件（包括其他控制平面组件）都通过 API Server 交互，不直接访问 etcd。这种设计带来了几个好处：统一的认证/授权/审计、缓存和乐观锁减少 etcd 压力、API 版本管理保证向后兼容、Watch 机制提供实时通知。

### Q4: kind 和 minikube 有什么区别？

kind 使用 Docker 容器模拟 K8s 节点，启动快、资源占用少、支持多节点。minikube 使用虚拟机或 Docker 容器，功能更丰富（支持插件、Dashboard、addon）但相对较重。学习 K8s 架构推荐 kind，学习具体功能推荐 minikube。

### Q5: 如何查看组件的日志？

使用 `kubectl logs -n kube-system <pod-name>` 查看控制平面组件日志。在 kind 集群中，还可以使用 `docker exec -it <container> journalctl -u kubelet` 查看 kubelet 日志。在生产环境（ACK 专有版）中，可以直接 SSH 到 Master 节点查看 systemd 日志。

### Q6: Scheduler 的 Filter 和 Score 阶段有哪些具体策略？

Filter 策略包括：PodFitsResources（节点资源充足）、PodFitsHostPorts（端口不冲突）、PodMatchNodeSelector（标签匹配）、PodToleratesNodeTaints（容忍污点）、CheckNodeUnschedulable（节点未 cordon）。Score 策略包括：NodeResourcesFit（资源均衡）、ImageLocality（镜像已存在）、PodTopologySpread（拓扑域分散）、InterPodAffinity（Pod 亲和性）。

---

## 要点总结

| 组件 | 职责 | 关键点 |
|------|------|--------|
| etcd | 存储集群状态 | 只有 API Server 能直接访问，对磁盘 IO 敏感 |
| API Server | 集群网关 | 认证、授权、准入控制，所有组件的入口 |
| Controller Manager | 维护期望状态 | Reconcile 循环，多种控制器 |
| Scheduler | Pod 调度 | Filter（过滤）+ Score（打分） |
| kubelet | 节点代理 | 管理 Pod 生命周期，执行探针 |
| kube-proxy | 网络代理 | Service → Pod 转发（iptables/IPVS） |

---

## 延伸阅读

- [K8s 架构总览](../../domain-01-cluster-fundamentals/01-kubernetes-architecture-overview.md)
- [核心组件深入](32-发布/package/2026-07-02_18-53/corpus/core/domain-01-cluster-fundamentals/01-architecture-overview/01-core-components-deep-dive.md)
- [kubectl 命令参考](32-发布/package/2026-07-02_18-53/corpus/core/domain-01-cluster-fundamentals/04-kubectl/01-kubectl-commands-reference.md)
- [K8s 速查手册](../../domain-17-system-foundation/速查卡/k8s.md)

## Related

- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

```

<!-- risk-assessed -->
