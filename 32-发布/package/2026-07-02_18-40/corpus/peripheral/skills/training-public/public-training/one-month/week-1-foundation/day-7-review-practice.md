---
title: 'Day 7: 周复习 + 综合实践'
description: '# Day 7: 周复习 + 综合实践'
summary: '今天是第一周的收官日，通过主动回忆、知识图谱构建和综合实践项目来巩固本周学习成果。你将从零搭建一个运行 nginx 的 K8s 集群，体验从集群创建、应用部署、服务暴露到故障排查的完整流程。这是将本周理论知识转化为实际操作能力的关键环节。'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- scheduler
- containerd
- docker
- statefulset
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 7: 周复习 + 综合实践 是什么'
- '如何 Day 7: 周复习 + 综合实践'
trigger_keywords:
- Day
- '7:'
- 周复习
- 综合实践
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 7: 周复习 + 综合实践

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY7
title: Day 7 - 周复习 + 综合实践
topic: kubernetes
type: hands-on-guide
tags: [review, practice, kind, deployment, service, kubectl, troubleshooting, hands-on, week-1]
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - "K8s 综合实践项目"
  - "Kind 集群怎么创建"
  - "Deployment 完整部署流程"
  - "声明式管理怎么理解"
trigger_keywords:
  - 综合实践
  - kind
  - 集群搭建
  - kubectl
  - 声明式管理
  - 故障排查
  - 产出文档
  - 滚动更新
  - 回滚
  - 资源清单
reading_level: beginner
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 50min
related_domains:
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - kubernetes
  - kubectl
  - deployment
  - troubleshooting
related:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/checkpoint.md
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p1-k8s-cluster-setup.md
---

```

> **学习时间**: 4-5 小时 | **主题**: Week 1 总结与实践项目

---

## 概述

今天是第一周的收官日，通过主动回忆、知识图谱构建和综合实践项目来巩固本周学习成果。你将从零搭建一个运行 nginx 的 K8s 集群，体验从集群创建、应用部署、服务暴露到故障排查的完整流程。这是将本周理论知识转化为实际操作能力的关键环节。

### 学习目标

- 复习本周所学，构建 Docker → Linux → K8s 知识图谱
- 完成实践项目 P1：从零搭建 K8s 集群并运行 nginx 应用
- 通过 checkpoint 自测检验学习效果
- 理解声明式管理的完整工作流（YAML → kubectl apply → Reconcile）

---

## 核心概念详解

### 知识体系回顾

本周的学习路径是一条层层递进的知识链：Docker 提供了容器运行的基础能力，Linux 提供了容器隔离的底层原理（namespace + cgroup），K8s 在这两者之上构建了容器编排平台。

**Docker 核心概念回顾**: Docker 采用 Client-Server 架构，Docker Client 通过 REST API 与 Docker Daemon 通信。镜像（Image）是只读的分层文件系统，容器（Container）是镜像的运行实例（在镜像顶部添加可写层）。Dockerfile 是构建镜像的蓝图，常用指令包括 FROM、RUN、COPY、ENV、EXPOSE、CMD/ENTRYPOINT。容器生命周期包括 Created → Running → Paused → Stopped → Deleted 五个状态。

**Linux 容器基础回顾**: 容器的本质是 Linux 内核提供的三项隔离能力。namespace 提供资源隔离（7 种类型：PID、NET、IPC、MNT、UTS、USER、CGROUP），cgroup 提供资源限制（CPU、内存、IO、网络），UnionFS 提供镜像分层（Docker 镜像的核心技术）。K8s 中的 resources.requests 和 resources.limits 最终就是通过 cgroup 来实现的。

**K8s 架构回顾**: K8s 采用 Master-Node 架构。Master 运行控制平面（[[etcd|etcd]] + API Server + Controller Manager + Scheduler），Node 运行数据平面（kubelet + kube-proxy + Container Runtime）。所有组件通过 API Server 通信，API Server 将数据持久化到 etcd。声明式管理的核心思想是"描述期望状态，系统自动收敛"——Deployment 描述期望的 Pod 副本数，Controller Manager 确保 Pod 数量与期望一致。

### 声明式管理工作流

当你执行 `kubectl apply -f deployment.yaml` 时，发生了以下事件链：

1. kubectl 将 YAML 通过 HTTP POST 发送到 API Server
2. API Server 执行认证、授权、准入控制检查
3. API Server 将资源定义写入 etcd
4. Controller Manager 通过 Watch 机制发现新的 Deployment
5. Deployment Controller 创建对应的 ReplicaSet
6. ReplicaSet Controller 创建 Pod 对象
7. Scheduler 监听到未调度的 Pod，为其选择合适的节点
8. 节点上的 kubelet 发现分配到本节点的 Pod
9. kubelet 调用 containerd 拉取镜像并启动容器
10. kubelet 通过 readinessProbe 检查容器就绪状态

理解这个流程对于排查"Pod 为什么没有启动"等问题至关重要——你可以通过 kubectl 逐步检查每个阶段的状态。

---

## 实战演练

### 知识复习 (2h)

#### 主动回忆练习

不看文档，在纸上/白板上画出以下三张图，画完后对照文档检查遗漏：

1. **Docker 架构图**：Docker Client → Docker Daemon → Container Runtime → Images/Containers/Networks/Volumes → Registry
2. **Linux 容器原理图**：namespace（7种类型）+ cgroup（CPU/Memory/IO）+ UnionFS（分层挂载）= 容器
3. **Kubernetes 架构图**：Master（etcd ↔ API Server ↔ Controller Manager ↔ Scheduler）↔ Node（kubelet ↔ kube-proxy ↔ containerd）

#### 核心概念速查

| 主题 | 核心概念 | 关键命令 | 自评掌握程度 |
|------|----------|---------|-------------|
| Docker | 镜像、容器、Volume、Network | docker build/run/ps/logs | ⬜⬜⬜ |
| Linux | namespace、cgroup、进程、网络 | ps/top/free/ip/lsof | ⬜⬜⬜ |
| K8s 架构 | etcd、API Server、Scheduler、kubelet | kubectl get/describe/logs | ⬜⬜⬜ |
| K8s 资源 | Pod、Deployment、Service、Namespace | kubectl apply/delete | ⬜⬜⬜ |
| kubectl | get、describe、apply、logs、exec | kubectl rollout/scale | ⬜⬜⬜ |

### 实践项目 P1 (2.5h)

#### 项目：从零搭建一个可运行 nginx 的 K8s 集群

详细指南见: [../projects/p1-k8s-cluster-setup.md](../projects/p1-k8s-cluster-setup.md)

#### Step 1: 创建集群 (30min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 kind 创建集群（多节点模拟生产环境）
kind create cluster --name production-sim --config - <<EOF
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
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
EOF

# 预期输出:
# Creating cluster "production-sim" ...
# ✓ Control plane node ready
# ✓ Worker node 1 ready
# ✓ Worker node 2 ready
# Cluster creation complete.

# 验证集群
kubectl get nodes
# 预期输出:
# NAME                         STATUS   ROLES           AGE   VERSION
# production-sim-control-plane Ready    control-plane   1m    v1.28.0
# production-sim-worker        Ready    <none>          1m    v1.28.0
# production-sim-worker2       Ready    <none>          1m    v1.28.0

kubectl get pods -n kube-system
# 预期输出: 所有系统 Pod 为 Running 状态
```
#### Step 2: 创建 Namespace (10min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建项目 namespace
kubectl create namespace web-app
# 预期输出: namespace/web-app created

# 设置默认 namespace（简化后续命令）
kubectl config set-context --current --namespace=web-app
# 预期输出: Context "kind-production-sim" modified.

# 验证
kubectl get namespace web-app
# 预期输出:
# NAME      STATUS   AGE
# web-app   Active   30s
```
#### Step 3: 部署 Deployment (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建完整的 Deployment YAML
cat > deployment.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-web
  namespace: web-app
  labels:
    app: nginx-web
    environment: production
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
        environment: production
    spec:
      containers:
      - name: nginx
        image: nginx:alpine
        ports:
        - containerPort: 80
          protocol: TCP
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
          failureThreshold: 3
        readinessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 3
          periodSeconds: 5
          failureThreshold: 3
        volumeMounts:
        - name: html-content
          mountPath: /usr/share/nginx/html
      volumes:
      - name: html-content
        configMap:
          name: nginx-html
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: nginx-html
  namespace: web-app
data:
  index.html: |
    <!DOCTYPE html>
    <html>
    <head><title>K8s Practice</title></head>
    <body>
    <h1>Hello from Kubernetes!</h1>
    <p>Hostname: $HOSTNAME</p>
    <p>This page is served from a ConfigMap volume.</p>
    </body>
    </html>
EOF

kubectl apply -f deployment.yaml
# 预期输出:
# deployment.apps/nginx-web created
# configmap/nginx-html created

# 观察 Pod 创建过程
kubectl get pods -w
# 预期输出（动态更新）:
# NAME                         READY   STATUS    RESTARTS   AGE
# nginx-web-6d4f7b8c9d-abc12   0/1     Pending   0          0s
# nginx-web-6d4f7b8c9d-abc12   0/1     ContainerCreating   0          1s
# nginx-web-6d4f7b8c9d-abc12   1/1     Running   0          5s
# nginx-web-6d4f7b8c9d-def34   1/1     Running   0          6s
# nginx-web-6d4f7b8c9d-ghi56   1/1     Running   0          7s

# 查看 Deployment 详情
kubectl get deployment nginx-web -o wide
# 预期输出:
# NAME         READY   UP-TO-DATE   AVAILABLE   AGE   CONTAINERS   IMAGES         SELECTOR
# nginx-web    3/3     3            3           1m    nginx        nginx:alpine   app=nginx-web

# 查看 ReplicaSet
kubectl get replicaset
# 预期输出: 一个 ReplicaSet，3 个 Pod
```
#### Step 4: 创建 Service (20min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建 ClusterIP 和 NodePort 两种 Service
cat > service.yaml << 'EOF'
apiVersion: v1
kind: Service
metadata:
  name: nginx-service
  namespace: web-app
  labels:
    app: nginx-web
spec:
  type: NodePort
  selector:
    app: nginx-web
  ports:
  - name: http
    port: 80
    targetPort: 80
    protocol: TCP
    nodePort: 30080
---
apiVersion: v1
kind: Service
metadata:
  name: nginx-clusterip
  namespace: web-app
spec:
  type: ClusterIP
  selector:
    app: nginx-web
  ports:
  - name: http
    port: 80
    targetPort: 80
EOF

kubectl apply -f service.yaml
# 预期输出:
# service/nginx-service created
# service/nginx-clusterip created

# 查看 Service
kubectl get svc
# 预期输出:
# NAME              TYPE        CLUSTER-IP      EXTERNAL-IP   PORT(S)        AGE
# nginx-service     NodePort    10.96.123.456   <none>        80:30080/TCP   10s
# nginx-clusterip   ClusterIP   10.96.234.567   <none>        80/TCP         10s

# 查看 Endpoints
kubectl get endpoints nginx-service
# 预期输出:
# NAME            ENDPOINTS                                      AGE
# nginx-service   10.244.1.2:80,10.244.2.2:80,10.244.1.3:80     1m
```
#### Step 5: 测试和调试 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 测试 ClusterIP Service（集群内部访问）
kubectl run curl-test --image=curlimages/curl -it --rm --restart=Never -- \
  curl -s http://nginx-clusterip.web-app.svc.cluster.local
# 预期输出: 返回 HTML 页面内容

# 测试 NodePort Service（通过节点端口访问）
NODE_IP=$(kubectl get nodes -o jsonpath='{.items[0].status.addresses[?(@.type=="InternalIP")].address}')
curl http://${NODE_IP}:30080
# 预期输出: 返回 HTML 页面内容

# 查看 Pod 日志
kubectl logs -l app=nginx-web --tail=10
# 预期输出: nginx 访问日志

# 进入 Pod 调试
kubectl exec -it $(kubectl get pod -l app=nginx-web -o jsonpath='{.items[0].metadata.name}') -- sh
# 在容器内执行:
# cat /etc/nginx/nginx.conf
# curl localhost:80
# hostname
# exit

# 查看事件
kubectl get events --sort-by='.lastTimestamp'
# 预期输出: 列出近期所有事件（Pod 调度、拉取镜像、探针检查等）

# 模拟问题场景 1: 镜像拉取失败
kubectl set image deployment/nginx-web nginx=nginx:nonexistent-tag
kubectl get pods
# 预期输出: 新 Pod 处于 ImagePullBackOff 状态
# NAME                         READY   STATUS             RESTARTS   AGE
# nginx-web-7a8b9c0d1e-xxx     0/1     ImagePullBackOff   0          30s
# nginx-web-6d4f7b8c9d-abc12   1/1     Running            0          10m

# 查看错误详情
kubectl describe pod -l app=nginx-web | grep -A 10 "Events:"
# 预期输出: Failed to pull image "nginx:nonexistent-tag": ...

# 回滚修复
kubectl rollout undo deployment/nginx-web
# 预期输出: deployment.apps/nginx-web rolled back

# 验证回滚
kubectl rollout status deployment/nginx-web
# 预期输出: deployment "nginx-web" successfully rolled out

# 模拟问题场景 2: 资源不足导致 Pending
# 创建一个请求超大资源的 Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: resource-hog
spec:
  containers:
  - name: app
    image: nginx:alpine
    resources:
      requests:
        cpu: "32"
        memory: "64Gi"
EOF

kubectl get pod resource-hog
# 预期输出: Pending 状态

kubectl describe pod resource-hog | grep -A 5 "Events:"
# 预期输出: 0/3 nodes are available: insufficient cpu...

# 清理测试 Pod
kubectl delete pod resource-hog
```
#### Step 6: 产出文档 (30min)

创建 `~/k8s-setup-doc.md`，记录以下内容：

```markdown
# K8s 集群搭建文档

## 1. 集群信息
- 集群名称: production-sim
- K8s 版本: v1.28.0
- 节点数: 3 (1 control-plane + 2 worker)
- CNI: kindnet

## 2. 部署资源清单
- Namespace: web-app
- Deployment: nginx-web (3 replicas)
- ConfigMap: nginx-html
- Service: nginx-service (NodePort:30080), nginx-clusterip (ClusterIP)

## 3. 遇到的问题和解决方法
| 问题 | 原因 | 解决方法 |
|------|------|---------|
| ImagePullBackOff | 镜像标签不存在 | rollout undo 回滚 |
| Pod Pending | 资源请求过大 | 调整 resources.requests |

## 4. 常用命令速查
| 操作 | 命令 |
|------|------|
| 查看所有 Pod | kubectl get pods -A |
| 查看 Pod 详情 | kubectl describe pod <name> |
| 查看日志 | kubectl logs <pod-name> -f |
| 进入容器 | kubectl exec -it <pod-name> -- sh |
| 滚动更新 | kubectl set image deployment/<name> <container>=<image> |
| 回滚 | kubectl rollout undo deployment/<name> |
| 扩缩容 | kubectl scale deployment/<name> --replicas=N |
| 查看事件 | kubectl get events --sort-by='.lastTimestamp' |
```

---

## 配置示例

### 完整的 Nginx 部署清单

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: web-app
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: nginx-html
  namespace: web-app
data:
  index.html: |
    <!DOCTYPE html>
    <html>
    <head><title>K8s Practice</title></head>
    <body>
    <h1>Hello from Kubernetes!</h1>
    <p>Served by: $HOSTNAME</p>
    </body>
    </html>
---
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
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
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
          initialDelaySeconds: 3
          periodSeconds: 5
        volumeMounts:
        - name: html-content
          mountPath: /usr/share/nginx/html
      volumes:
      - name: html-content
        configMap:
          name: nginx-html
---
apiVersion: v1
kind: Service
metadata:
  name: nginx-service
  namespace: web-app
spec:
  type: NodePort
  selector:
    app: nginx-web
  ports:
  - port: 80
    targetPort: 80
    nodePort: 30080
```

---

## 常见问题

### Q1: kind 集群创建失败怎么办？

常见原因：Docker 未启动（执行 `docker info` 检查）、端口被占用（检查 6443 端口）、磁盘空间不足。如果持续失败，尝试删除旧集群：`kind delete cluster --name production-sim`，然后重新创建。

### Q2: Pod 一直处于 ContainerCreating 怎么办？

使用 `kubectl describe pod <name>` 查看 Events 部分。常见原因：镜像拉取慢（配置镜像加速器）、存储卷挂载失败、ConfigMap/Secret 不存在。

### Q3: Service 的 Endpoints 为空怎么办？

检查 Service 的 selector 是否与 Pod 的 labels 完全匹配（注意缩进和拼写）。使用 `kubectl get pods --show-labels` 查看 Pod 的实际标签，与 Service 的 selector 对比。

### Q4: 如何查看 Pod 内的容器日志？

使用 `kubectl logs <pod-name>` 查看标准输出。如果 Pod 有多个容器，使用 `-c` 指定容器名：`kubectl logs <pod-name> -c <container-name>`。如果容器已重启，使用 `--previous` 查看上一次的日志。

### Q5: 滚动更新卡住怎么排查？

检查 `kubectl rollout status deployment/<name>` 的输出。常见原因：新 Pod 的 readinessProbe 检查失败、镜像拉取失败、资源不足导致新 Pod Pending。使用 `kubectl get pods` 和 `kubectl describe pod` 逐一排查。

### Q6: 如何清理 kind 集群中的所有资源？

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复

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
# 删除命名空间（会删除其中所有资源）
kubectl delete namespace web-app  # ⚠️ 不可逆：永久删除命名空间及全部资源

# 删除整个集群
kind delete cluster --name production-sim

# 查看 kind 管理的所有集群
kind get clusters
```
---

## 要点总结

本周建立了从容器到 K8s 的完整知识体系：

| 学习日 | 主题 | 关键收获 |
|--------|------|---------|
| Day 1 | Docker 容器基础 | 容器是进程级隔离，镜像分层构建 |
| Day 2 | Docker 进阶 | 网络、存储、多阶段构建 |
| Day 3 | Linux 核心 | namespace 隔离 + cgroup 限制 = 容器 |
| Day 4 | Linux 网络 | TCP/IP、网络排障、性能调优 |
| Day 5 | K8s 架构 | Master/Node 组件、声明式管理 |
| Day 6 | 集群配置 | API 版本、Deployment、Service |
| Day 7 | 综合实践 | 从零搭建集群，跑通完整流程 |

### 下周预告

Week 2 将深入 K8s 核心技术：控制平面组件详解（etcd、API Server、Scheduler）、工作负载管理（Deployment、StatefulSet、DaemonSet）、网络栈（CNI、Service、DNS、Ingress）、存储体系（PV、PVC、StorageClass）。

---

## 延伸阅读

- [K8s 架构总览](../../domain-01-cluster-fundamentals/01-kubernetes-architecture-overview.md)
- [kubectl 命令参考](../../domain-01-cluster-fundamentals/05-kubectl-commands-reference.md)
- [Deployment 生产模式](../../domain-02-workloads-applications/02-deployment-production-patterns.md)
- [Service 概念与类型](../../domain-03-networking-traffic/06-service-concepts-types.md)
- [K8s 速查手册](../../domain-17-system-foundation/速查卡/k8s.md)

## Related

- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

```

<!-- risk-assessed -->
