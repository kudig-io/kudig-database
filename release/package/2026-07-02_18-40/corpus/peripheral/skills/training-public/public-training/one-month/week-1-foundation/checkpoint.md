---
title: 'Week 1 Checkpoint: 自测检验'
description: '- "K8s 架构组件题"'
summary: '- "K8s 架构组件题"'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- scheduler
- flannel
- containerd
- cri-o
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Week 1 Checkpoint: 自测检验 是什么'
- '如何 Week 1 Checkpoint: 自测检验'
trigger_keywords:
- Week
- 'Checkpoint:'
- 自测检验
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- etcd-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Week 1 Checkpoint: 自测检验

```yaml
---
id: LEARN-ONE-MONTH-W1-CHECKPOINT
title: Week 1 Checkpoint - 自测检验
topic: kubernetes
type: checkpoint
tags: [checkpoint, self-test, week-1, docker, linux, kubernetes, namespace, cgroup]
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - "K8s Week 1 自测题"
  - "Docker 容器原理题"
  - "namespace cgroup 区别"
  - "K8s 架构组件题"
trigger_keywords:
  - 自测
  - checkpoint
  - 概念理解
  - 命令实操
  - 场景分析
  - 综合设计
  - 评分标准
  - 薄弱点
  - 知识点速查
reading_level: beginner
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 90min
related_domains:
  - domain-13-container-runtime
  - domain-17-system-foundation
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - docker
  - linux
  - kubernetes
  - troubleshooting
related:
  - domain-11-production-operations/topic-learn/public-training/one-month/[[domain-04-storage-data/README.md|README]].md
  - domain-11-production-operations/topic-learn/public-training/one-month/week-1-foundation/day-1-docker-basics.md
---
```

> 完成本周学习后，请独立完成以下自测题，不要查阅资料。

---

## 概述

本测验覆盖 Week 1 全部核心知识点，包括 Docker 基础、Linux 容器原理、K8s 架构组件和基础操作。测验分为四个部分：概念理解、命令实操、场景分析和综合设计，共计 80 分。请严格控制时间在 90 分钟内完成，答题过程中不得查阅任何参考资料。

---

## 一、概念理解 (每题 3 分，共 30 分)

### 1. Docker 容器和虚拟机的本质区别是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| 维度 | 容器 | 虚拟机 |
|------|------|--------|
| 内核 | 共享宿主机内核 | 拥有独立内核 |
| 隔离级别 | 进程级隔离 (namespace + cgroup) | 硬件级隔离 (Hypervisor) |
| 启动速度 | 秒级启动 | 分钟级启动 |
| 资源占用 | MB 级别 | GB 级别 |
| 镜像大小 | 通常几十到几百 MB | 通常几 GB |
| 性能损耗 | 接近原生 | 有虚拟化开销 |
| 适用场景 | 微服务、CI/CD、弹性伸缩 | 强隔离需求、异构 OS |

评分标准: 提到 3 个以上维度得满分，每个正确维度 1 分。

---

### 2. Linux namespace 有哪几种类型？各自隔离什么资源？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| Namespace | 隔离资源 | 系统调用参数 | 示例用途 |
|-----------|---------|-------------|---------|
| PID | 进程 ID | CLONE_NEWPID | 容器内进程从 PID 1 开始 |
| Network | 网络栈 (网卡、端口、路由表) | CLONE_NEWNET | 容器独立 IP 和端口空间 |
| Mount | 文件系统挂载点 | CLONE_NEWNS | 容器独立文件系统视图 |
| UTS | 主机名和域名 | CLONE_NEWUTS | 容器独立 hostname |
| IPC | 进程间通信 (信号量、消息队列) | CLONE_NEWIPC | 隔离共享内存段 |
| User | 用户和组 ID | CLONE_NEWUSER | 容器内 root 映射到宿主机普通用户 |
| Cgroup | cgroup 根目录视图 | CLONE_NEWCGROUP | 隔离 cgroup 层级视图 |

查看进程 namespace 示例:

```bash
ls -la /proc/$$/ns
# lrwxrwxrwx 1 root root 0 cgroup -> 'cgroup:[4026531835]'
# lrwxrwxrwx 1 root root 0 ipc -> 'ipc:[4026531839]'
# lrwxrwxrwx 1 root root 0 mnt -> 'mnt:[4026531840]'
# lrwxrwxrwx 1 root root 0 net -> 'net:[4026531992]'
# lrwxrwxrwx 1 root root 0 pid -> 'pid:[4026531836]'
# lrwxrwxrwx 1 root root 0 user -> 'user:[4026531837]'
# lrwxrwxrwx 1 root root 0 uts -> 'uts:[4026531838]'
```

---

### 3. cgroup 可以限制哪些资源？在 K8s 中如何体现？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

cgroup (Control Group) 子系统与 K8s 资源映射:

| cgroup 子系统 | 限制资源 | K8s 对应字段 | 示例值 |
|--------------|---------|-------------|--------|
| cpu | CPU 使用时间 | resources.requests.cpu / limits.cpu | 100m / 200m |
| memory | 内存使用量 | resources.requests.memory / limits.memory | 128Mi / 256Mi |
| blkio | 块设备 IO | 无直接字段 (通过 device manager) | - |
| devices | 设备访问 | securityContext | - |
| pids | 进程数量 | 无直接字段 ([[kubelet|kubelet]] 配置) | - |

K8s QoS 等级:

| QoS 等级 | 条件 | 驱逐优先级 | cgroup 配置 |
|----------|------|-----------|------------|
| Guaranteed | requests == limits (CPU 和内存都设置) | 最后被驱逐 | cpuShares=1024*cores, memory=limits |
| Burstable | 至少一个容器设置了 requests 或 limits | 中等优先级 | cpuShares=1024*(requests/1), memory=limits |
| BestEffort | 未设置 requests 和 limits | 最先被驱逐 | cpuShares=2, memory=无限制 |

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: qos-demo
spec:
  containers:
  - name: app
    image: nginx
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 200m
        memory: 256Mi
```

---

### 4. K8s 的 [[etcd|etcd]]、API Server、Scheduler、Controller Manager 各做什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| 组件 | 核心职责 | 监听端口 | 数据存储 |
|------|---------|---------|---------|
| etcd | 分布式键值存储，保存所有集群状态 | 2379 (client), 2380 (peer) | 磁盘 (建议 SSD) |
| API Server | 集群网关，认证授权准入控制 | 6443 (HTTPS) | etcd |
| Scheduler | 为 Pod 选择最优节点 | 10251 (HTTP) | 无状态 |
| Controller Manager | 运行控制器循环，维护期望状态 | 10252 (HTTP) | 无状态 |

组件交互流程:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply → API Server → etcd
                   ↓
           Controller Manager → 创建 ReplicaSet → API Server → etcd
                                                      ↓
                                              Scheduler → 绑定 Node → API Server → etcd
                                                                            ↓
                                                                    kubelet → 创建容器
```
---

### 5. 某 Pod 一直处于 Pending 状态，你的排查步骤是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

Pending 排查决策树:

```
# 🟢 低风险：只读/信息收集，通常无副作用
Pod Pending
├── 资源不足
│   ├── CPU 不足 → kubectl describe pod → Insufficient cpu
│   ├── Memory 不足 → kubectl describe pod → Insufficient memory
│   └── GPU 不足 → 检查 GPU 资源配额
├── 调度约束
│   ├── nodeSelector 无匹配 → 检查节点标签
│   ├── nodeAffinity 不满足 → 检查亲和性规则
│   ├── taints/tolerations 阻止 → 检查节点污点
│   └── podAntiAffinity 冲突 → 检查反亲和性规则
├── 存储问题
│   ├── PVC Pending → StorageClass 不存在或无可用 PV
│   └── PVC 绑定失败 → 检查 PV 容量和访问模式
└── 集群问题
    ├── Scheduler 不可用 → kubectl get componentstatuses
    └── API Server 异常 → 检查 API Server 日志
```
排查命令序列:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe pod <name> | grep -A 20 Events
kubectl get nodes -o wide
kubectl describe node <node-name> | grep -A 5 Allocatable
kubectl get pvc
kubectl get sc
kubectl get clusterrolebindings -o wide
```
---

### 6. 解释 Docker 镜像分层原理及其优势

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

Docker 镜像分层结构:

```
┌─────────────────────────┐
│  可写层 (Container Layer) │  ← 容器运行时写入
├─────────────────────────┤
│  Layer N: RUN apt update │  ← 只读
├─────────────────────────┤
│  Layer 2: COPY app /app  │  ← 只读
├─────────────────────────┤
│  Layer 1: FROM ubuntu    │  ← 基础镜像，只读
└─────────────────────────┘
     Union Filesystem (Overlay2)
```

| 优势 | 说明 |
|------|------|
| 层复用 | 相同基础镜像只存储一份，多个容器共享 |
| 构建效率 | 未修改的层使用缓存，加速构建 |
| 存储节省 | 层去重，减少磁盘占用 |
| 分发高效 | 只传输缺失的层，加速镜像拉取 |
| 安全审计 | 每层可追溯，便于安全扫描 |

---

### 7. 为什么 K8s 节点需要开启 `net.ipv4.ip_forward`？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

- Pod 网络需要跨节点通信，节点充当路由器角色
- Flannel VXLAN 模式下，封装的数据包需要内核转发
- Terway ENI 模式下，节点需要转发 Pod 到外部的流量
- 不开启会导致: 同节点 Pod 可通信，跨节点 Pod 不通

验证命令:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

```bash
sysctl net.ipv4.ip_forward
# net.ipv4.ip_forward = 1

sysctl -w net.ipv4.ip_forward=1
echo "net.ipv4.ip_forward = 1" >> /etc/sysctl.d/99-k8s.conf
```

---

### 8. Service 是如何将流量转发到 Pod 的？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

Service 流量转发机制对比:

| 模式 | 实现方式 | 优势 | 劣势 |
|------|---------|------|------|
| iptables | DNAT 规则随机选择后端 | 性能好 | 规则多时 O(n) |
| IPVS | 内核负载均衡器 | O(1) 查找 | 配置复杂 |

iptables 模式流量路径:

```
Client → Service ClusterIP:Port
  → iptables PREROUTING Chain
    → KUBE-SERVICES Chain
      → KUBE-SVC-XXX Chain (随机概率)
        → KUBE-SEP-XXX Chain (DNAT)
          → Pod IP:Port
```

---

### 9. 什么是 Docker 的 Union Filesystem？支持的实现有哪些？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| 实现 | 特点 | Docker 默认 |
|------|------|------------|
| overlay2 | 性能好，层共享 | 是 (Docker 18.09+) |
| devicemapper | 块级别操作 | 否 |
| btrfs | 快照支持 | 否 |
| zfs | 数据完整性 | 否 |

查看当前存储驱动:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
docker info | grep "Storage Driver"
# Storage Driver: overlay2
```
---

### 10. K8s 中 Deployment、ReplicaSet、Pod 三者之间的关系是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

层级关系:

```
Deployment (声明期望状态)
  └── ReplicaSet (版本管理，维护副本数)
        └── Pod (实际运行的容器组)

```

| 资源 | 职责 | 生命周期 |
|------|------|---------|
| Deployment | 滚动更新策略、回滚 | 最长 |
| ReplicaSet | 维护特定版本的 Pod 副本数 | 随版本变更 |
| Pod | 运行容器 | 最短 (随时可被重建) |

---

## 二、命令实操 (每题 2 分，共 16 分)

### 11. `kubectl rollout undo deployment/nginx` 做什么？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** 回滚到上一个版本的 Deployment

相关命令:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl rollout history deployment/nginx
kubectl rollout undo deployment/nginx
kubectl rollout undo deployment/nginx --to-revision=2
kubectl rollout status deployment/nginx
kubectl rollout pause deployment/nginx
kubectl rollout resume deployment/nginx
```
---

### 12. `kubectl port-forward pod/nginx 8080:80` 做什么？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** 将本地 8080 端口转发到 Pod 的 80 端口

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl port-forward pod/nginx 8080:80
kubectl port-forward svc/nginx 8080:80
kubectl port-forward deploy/nginx 8080:80
kubectl port-forward pod/nginx 8080:80 9090:9090
```
---

### 13. `kubectl top node` 做什么？需要什么前置条件？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** 显示节点资源使用情况，需要安装 metrics-server

安装 metrics-server:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml

kubectl top nodes
# NAME       CPU(cores)   CPU%   MEMORY(bytes)   MEMORY%
# node-01    500m         25%    2048Mi          32%
# node-02    300m         15%    1536Mi          24%
```
---

### 14. 如何查看 Pod 的实时日志？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** `kubectl logs -f <pod-name>`

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl logs -f nginx-pod
kubectl logs -f nginx-pod -c sidecar
kubectl logs nginx-pod --previous
kubectl logs nginx-pod --since=1h
kubectl logs -l app=nginx --all-containers
kubectl logs nginx-pod --tail=100
```
---

### 15. 如何进入一个正在运行的 Pod 执行命令？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** `kubectl exec -it <pod-name> -- /bin/sh`

---

### 16. 如何查看所有 Namespace 的 Pod 资源使用排行？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:** `kubectl top pods -A --sort-by=cpu | head -20`

---

### 17. 如何使用 JSONPath 获取所有 Pod 的名称和节点？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.nodeName}{"\n"}{end}'
```
---

### 18. 如何查看 kubeconfig 中当前上下文的集群地址？

**你的回答:**

```
(在此写下你的答案)

```

**参考答案:**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl config view --minify -o jsonpath='{.clusters[0].cluster.server}'

```
---

## 三、场景分析 (每题 5 分，共 20 分)

### 19. 当你执行 `kubectl apply -f deployment.yaml` 时，K8s 内部发生了什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

完整流程:

```
# 🟢 低风险：只读/信息收集，通常无副作用
Step 1: kubectl 本地操作
  ├── 读取 YAML 文件
  ├── 校验 YAML 格式和字段合法性
  ├── 计算 3-way merge (last-applied + current + new)
  └── 发送 HTTP PUT/PATCH 请求到 API Server

Step 2: API Server 处理
  ├── Authentication: 验证客户端身份 (证书/Token)
  ├── Authorization: RBAC 检查用户是否有权限
  ├── Admission Control:
  │   ├── MutatingWebhook: 修改资源 (注入 sidecar)
  │   ├── ValidationWebhook: 校验资源 (策略检查)
  │   ├── ResourceQuota: 检查配额
  │   └── LimitRanger: 检查资源限制
  └── 写入 etcd

Step 3: Controller Manager 响应
  ├── Deployment Controller 检测到变化
  ├── 创建新 ReplicaSet (或更新已有)
  └── 根据 strategy 滚动更新

Step 4: Scheduler 调度
  ├── 过滤 (Filter): 排除不满足条件的节点
  ├── 评分 (Score): 对候选节点打分
  └── 绑定 (Bind): 将 Pod 分配到最优节点

Step 5: kubelet 执行
  ├── 拉取镜像 (containerd/CRI-O)
  ├── 创建容器 (CRI)
  ├── 配置网络 (CNI)
  └── 挂载存储 (CSI)
```
---

### 20. 设计一个 Nginx 应用的部署方案，要求: 高可用、滚动更新零宕机、资源限制合理。

**你的回答:**

```
(在此写下你的答案)


```

**参考答案:**

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-ha
  labels:
    app: nginx
spec:
  replicas: 3
  selector:
    matchLabels:
      app: nginx
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  template:
    metadata:
      labels:
        app: nginx
    spec:
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: nginx
      containers:
      - name: nginx
        image: nginx:1.25-alpine
        ports:
        - containerPort: 80
        readinessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 5
          periodSeconds: 3
        livenessProbe:
          httpGet:
            path: /
            port: 80
          initialDelaySeconds: 15
          periodSeconds: 10
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 256Mi
---
apiVersion: v1
kind: Service
metadata:
  name: nginx-svc
spec:
  selector:
    app: nginx
  ports:
  - port: 80
    targetPort: 80
  type: ClusterIP
```

---

### 21. 解释 K8s Service 的三种类型 (ClusterIP/NodePort/LoadBalancer) 及适用场景。

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| Service 类型 | 访问范围 | 端口范围 | 典型场景 |
|-------------|---------|---------|---------|
| ClusterIP | 集群内部 | 任意 | 微服务间通信 |
| NodePort | 集群外部通过 NodeIP:Port | 30000-32767 | 测试环境暴露服务 |
| LoadBalancer | 公网/内网 | 任意 | 生产环境对外服务 |

---

### 22. 一个 Pod 处于 CrashLoopBackOff 状态，请写出完整排查步骤。

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod <name> -o wide
kubectl describe pod <name>
kubectl logs <name> --previous
kubectl logs <name> -c <container>
kubectl get events --field-selector involvedObject.name=<name>
```
常见原因:

| 原因 | 症状 | 修复方法 |
|------|------|---------|
| 应用启动失败 | exit code 1 | 检查应用日志和配置 |
| OOMKilled | exit code 137 | 增大 memory limits |
| 健康检查失败 | Restart Count 增加 | 调整 probe 参数 |
| 缺少配置文件 | File not found | 检查 ConfigMap/Secret 挂载 |

---

## 四、综合设计 (每题 7 分，共 14 分)

### 23. 设计一个包含 3 个微服务 (前端/API/数据库) 的 K8s 部署方案。

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

```
前端 (Deployment + ClusterIP Service)
  ↓
API (Deployment + ClusterIP Service + HPA)
  ↓
数据库 (StatefulSet + Headless Service + PVC)
```

关键设计点:

| 组件 | 工作负载类型 | Service 类型 | 存储需求 |
|------|------------|-------------|---------|
| 前端 | Deployment | ClusterIP (通过 Ingress 暴露) | 无 |
| API | Deployment + HPA | ClusterIP | ConfigMap/Secret |
| 数据库 | StatefulSet | Headless Service | PVC (SSD StorageClass) |

---

### 24. 请写出一个 K8s NetworkPolicy，实现以下安全策略: 只有 frontend 命名空间的 Pod 可以访问 api 命名空间的 8080 端口。

**你的回答:**

```
(在此写下你的答案)


```

**参考答案:**

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-allow-frontend
  namespace: api
spec:
  podSelector:
    matchLabels: {}
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: frontend
    ports:
    - protocol: TCP
      port: 8080
```

前提: frontend 命名空间需要有 `name: frontend` 标签:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl label namespace frontend name=frontend
```
---

## 五、评分统计

| 部分 | 得分 | 满分 |
|------|------|------|
| 概念理解 | __ | 30 |
| 命令实操 | __ | 16 |
| 场景分析 | __ | 20 |
| 综合设计 | __ | 14 |
| **总分** | __ | **80** |

### 评估标准

- **72-80 分**: 优秀，完全掌握本周内容，具备独立操作能力
- **56-71 分**: 良好，基本掌握，部分细节需加强
- **40-55 分**: 及格，核心概念理解，需要复习薄弱环节
- **< 40 分**: 不及格，建议重新学习本周内容

---

## 六、薄弱点记录

记录自测中暴露的薄弱点，下周重点复习:

```
1.


2.


3.


```

---

## 七、下周计划调整

基于自测结果，调整下周学习重点:

```
需要加强的领域:


下周额外复习:


```

---

## 八、知识点速查表

| 知识点 | 关键命令/概念 | 对应测验题 |
|--------|-------------|-----------|
| 容器 vs VM | 共享内核、进程隔离、秒级启动 | Q1 |
| Namespace | 7 种类型、各隔离资源 | Q2 |
| cgroup | cpu/memory 限制、QoS 三级 | Q3 |
| K8s 组件 | etcd/API Server/Scheduler/CM | Q4 |
| Pod Pending | describe events、资源/调度/存储 | Q5 |
| 镜像分层 | Overlay2、Union FS、层复用 | Q6 |
| ip_forward | 内核转发、跨节点通信 | Q7 |
| Service 转发 | iptables DNAT / IPVS | Q8 |
| Union FS | overlay2/devicemapper/btrfs | Q9 |
| 资源层级 | Deployment → RS → Pod | Q10 |

---

## 延伸阅读

- [Docker 基础概念](../../domain-13-container-runtime/01-docker-fundamentals-concepts.md)
- [K8s 架构总览](../../domain-01-cluster-fundamentals/01-kubernetes-architecture-overview.md)
- [K8s 核心组件](../../domain-01-cluster-fundamentals/02-core-components-deep-dive.md)
- [kubectl 命令参考](../../domain-01-cluster-fundamentals/05-kubectl-commands-reference.md)
- [Pod 排障指南](../../domain-10-troubleshooting-diagnostics/08-pod-comprehensive-troubleshooting.md)

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

```

<!-- risk-assessed -->
