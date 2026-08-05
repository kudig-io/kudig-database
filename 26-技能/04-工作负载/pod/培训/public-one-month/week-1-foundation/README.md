---
title: 'Week 1: 地基建设期 (Days 1-7)'
description: '- "K8s Master/Node 架构"'
summary: '- "K8s Master/Node 架构"'
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
- containerd
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
- 'Week 1: 地基建设期 (Days 1-7) 是什么'
- '如何 Week 1: 地基建设期 (Days 1-7)'
trigger_keywords:
- Week
- '1:'
- 地基建设期
- Days
- 1-7
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




# Week 1: 地基建设期 (Days 1-7)

```yaml
---
id: LEARN-ONE-MONTH-W1-README
title: Week 1 - 地基建设期 (Days 1-7)
topic: kubernetes
type: guide
tags: [week-1, docker, linux, kubernetes, namespace, cgroup, one-month]
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - "Docker 容器本质是什么"
  - "Linux namespace 和 cgroup 区别"
  - "K8s Master/Node 架构"
  - "kubectl 基础命令"
trigger_keywords:
  - 容器
  - Docker
  - namespace
  - cgroup
  - UnionFS
  - 镜像分层
  - Kubernetes
  - Master
  - Node
  - kubectl
  - 集群部署
  - 声明式管理
reading_level: beginner
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 30min
related_domains:
  - 容器运行时
  - 系统基础
  - 集群基础
related_topics:
  - docker
  - linux
  - kubernetes
  - container
related:
  - 生产运维/topic-learn/public-training/one-month/week-2-core-tech/README.md
  - 生产运维/topic-learn/public-training/one-month/projects/p1-k8s-cluster-setup.md
---
```

## 概述

第一周是整个一个月学习计划的基石阶段。Kubernetes 是一个复杂的分布式系统，要真正掌握它，需要先打好三个基础：**容器技术（Docker）**、**Linux 运维基础**和 **K8s 架构全貌**。这三个模块环环相扣——Docker 是 K8s 运行容器的底层技术，Linux 是 K8s 节点的操作系统，K8s 架构则是所有后续学习的技术地图。

本周的学习目标是帮助你建立从"容器是什么"到"能部署第一个应用到 K8s 集群"的完整认知链条。我们不追求面面俱到，而是聚焦于最核心、最实用的知识点，为后续三周的深入学习打下坚实基础。

### 学习目标

- 理解容器的本质（namespace + cgroup + UnionFS）并掌握 Docker 完整生命周期
- 具备 Linux 运维基础能力（进程管理、网络配置、文件系统、性能分析）
- 理解 K8s Master/Node 架构全貌并流利使用 kubectl 命令
- 成功部署一个 K8s 集群并运行第一个 Deployment
- **产出**: 成功部署一个 K8s 集群，跑通第一个 Deployment

---

## 核心概念详解

### 容器技术本质

容器技术的核心是 Linux 内核提供的三项隔离能力：**namespace**、**cgroup** 和 **UnionFS（联合文件系统）**。

**namespace** 提供了资源隔离。Linux 内核支持七种 namespace：

| Namespace | 隔离内容 | 容器中的表现 | K8s 关联 |
|-----------|---------|------------|---------|
| PID | 进程 ID | 容器内进程从 PID 1 开始 | Pod 内进程隔离 |
| NET | 网络栈 | 独立的 IP、端口、路由表 | Pod 网络（CNI） |
| IPC | 进程间通信 | 信号量、消息队列隔离 | Pod 内容器通信 |
| MNT | 文件系统挂载 | 独立的文件系统视图 | Volume 挂载 |
| UTS | 主机名和域名 | 容器有自己的 hostname | [[17-系统基础/06-知识字典/workloads/pod-hostname.md|Pod hostname]] |
| USER | 用户和组 ID | 容器内 root ≠ 宿主机 root | securityContext |
| CGROUP | cgroup 根目录 | cgroup 视图隔离 | 容器内不可见宿主 cgroup |

**cgroup（Control Group）** 提供了资源限制。它可以限制容器使用的 CPU、内存、磁盘 IO、网络带宽等资源。K8s 中的 resources.requests 和 resources.limits 最终就是通过 cgroup 来实现的：

- `requests.cpu`: 用于调度决策（Scheduler 选择资源充足的节点），通过 cgroup 的 `cpu.shares` 实现（相对权重）
- `limits.cpu`: 通过 cgroup 的 `cpu.cfs_quota_us` / `cpu.cfs_period_us` 实现绝对限制
- `limits.memory`: 通过 cgroup 的 `memory.limit_in_bytes` 实现，超限触发 OOMKilled

**UnionFS（联合文件系统）** 提供了镜像分层。Docker 镜像由多个只读层组成，每层代表 Dockerfile 中的一条指令。容器运行时，在镜像顶部添加一个可写层。这种设计带来了几个好处：层可以被多个镜像共享（节省存储和传输时间）、构建缓存加速镜像构建、基础镜像只需要拉取一次。

理解容器与虚拟机的区别是建立正确认知的关键：

| 维度 | 容器 | 虚拟机 |
|------|------|--------|
| 隔离级别 | 进程级（共享内核） | 硬件级（独立内核） |
| 启动速度 | 秒级 | 分钟级 |
| 资源开销 | MB 级 | GB 级 |
| 性能 | 接近原生 | 有虚拟化损耗 |
| 隔离性 | 较弱（共享内核） | 强（独立内核） |
| 适用场景 | 微服务、CI/CD | 强隔离需求、不同 OS |

### Docker 架构与生命周期

Docker 采用 Client-Server 架构。Docker Client（docker 命令）通过 REST API 与 Docker Daemon（dockerd）通信。Docker Daemon 负责管理镜像、容器、网络和存储卷。镜像存储在 Registry（如 Docker Hub、ACR）中，通过 pull/push 操作上传下载。

容器的生命周期包含以下状态：

```
Created → Running → Paused → Stopped → Deleted
                   ↘ Running → Stopped (exit 0: 正常, exit ≠ 0: 异常)

```

Dockerfile 是构建镜像的蓝图。常用指令包括：`FROM`（基础镜像）、`RUN`（执行命令）、`COPY`/`ADD`（复制文件）、`ENV`（设置环境变量）、`EXPOSE`（声明端口）、`CMD`/`ENTRYPOINT`（定义启动命令）。镜像构建优化的核心原则是：减少层数、利用缓存、减小镜像体积。

### Linux 运维基础

K8s 节点运行在 Linux 操作系统上，具备 Linux 运维能力是排障的基础。

**进程管理**: 理解 Linux 进程的生命周期、信号机制和进程间关系。在 K8s 环境中，容器的主进程（PID 1）特别重要——它决定了容器的生命周期。如果 PID 1 进程退出，容器就会停止。这也是为什么 Dockerfile 中的 CMD 应该使用前台运行模式。

**网络基础**: 理解 TCP/IP 协议栈、网络命名空间、网桥、路由、iptables 等概念。K8s 的网络模型基于虚拟网桥和 veth pair，Service 通过 iptables/IPVS 规则实现负载均衡。掌握 `ip`、`tcpdump`、`ss`、`iptables` 等网络排障工具是 K8s 网络排障的基础。

**文件系统**: 理解 Linux 文件系统层级、挂载机制、软硬链接。K8s 中的 Volume 本质上是将存储挂载到容器的文件系统路径中。掌握 `df`、`du`、`mount`、`lsof` 等命令有助于排查存储问题。

**性能分析**: 掌握 `top`、`htop`、`vmstat`、`iostat`、`sar` 等工具的使用。在 K8s 环境中，节点级别的性能问题（CPU 飙高、内存不足、磁盘 IO 瓶颈）会影响该节点上所有 Pod 的性能。

### K8s 架构全貌

Kubernetes 采用 Master-Node 架构。Master 节点运行控制平面组件，负责集群的全局决策和响应集群事件；Worker 节点运行工作负载。

**Master 组件**:

- **kube-apiserver**: 所有操作的入口。提供 RESTful API，处理认证、授权、准入控制。所有组件（kubectl、kubelet、scheduler 等）都通过 API Server 交互
- **etcd**: 分布式键值存储，保存集群的所有状态数据。etcd 的数据是集群的"唯一真实来源"
- **kube-scheduler**: 负责将未调度的 Pod 分配到合适的节点。调度过程包括 Filter（过滤不满足条件的节点）和 Score（对候选节点打分）
- **kube-controller-manager**: 运行多种控制器（Deployment、ReplicaSet、Node、Service Account 等）。每个控制器通过 Reconcile 循环将实际状态向期望状态收敛
- **cloud-controller-manager**: 与云平台交互的控制器（在 ACK 中由阿里云提供）

**Node 组件**:

- **kubelet**: 每个节点上的代理，负责管理 Pod 的生命周期、执行健康检查、汇报节点状态
- **kube-proxy**: 负责实现 Service 的网络转发规则（iptables 或 IPVS 模式）
- **Container Runtime**: 容器运行时（通常是 containerd），负责实际的容器操作

**kubectl** 是与 K8s 交互的命令行工具。掌握以下命令分类是基本要求：

- 资源查看: `kubectl get`、`kubectl describe`、`kubectl logs`
- 资源操作: `kubectl create`、`kubectl apply`、`kubectl delete`、`kubectl edit`
- 调试排障: `kubectl exec`、`kubectl port-forward`、`kubectl debug`
- 集群管理: `kubectl cordon`、`kubectl drain`、`kubectl taint`

---

## 实战演练

### 每日学习导航

| Day | 主题 | 文件 |
|-----|------|------|
| Day 1 | Docker 容器基础 | [day-1-docker-basics.md](./day-1-docker-basics.md) |
| Day 2 | Docker 网络 + 存储 + 安全 | [day-2-docker-advanced.md](./day-2-docker-advanced.md) |
| Day 3 | Linux 核心基础 | [day-3-linux-core.md](./day-3-linux-core.md) |
| Day 4 | Linux 网络 + 性能调优 | [day-4-linux-network.md](./day-4-linux-network.md) |
| Day 5 | K8s 架构全貌 | [day-5-k8s-architecture.md](./day-5-k8s-architecture.md) |
| Day 6 | K8s 架构深化 + 集群配置 | [day-6-k8s-cluster.md](./day-6-k8s-cluster.md) |
| Day 7 | 周复习 + 综合实践 | [day-7-review-practice.md](./day-7-review-practice.md) |

### 本周自测

完成本周学习后，请完成 [checkpoint.md](./checkpoint.md) 中的自测题。

### 本周实践项目

**项目 P1**: [从零搭建一个可运行 nginx 的 K8s 集群](../projects/p1-k8s-cluster-setup.md)

---

## 配置示例

### Dockerfile 最佳实践模板

```dockerfile
FROM node:18-alpine AS builder
WORKDIR /app
COPY package*.json ./
RUN npm ci --only=production
COPY . .
RUN npm run build

FROM node:18-alpine
WORKDIR /app
COPY --from=builder /app/dist ./dist
COPY --from=builder /app/node_modules ./node_modules
USER node
EXPOSE 3000
HEALTHCHECK --interval=30s --timeout=3s \
  CMD wget -qO- http://localhost:3000/health || exit 1
CMD ["node", "dist/server.js"]
```

### 基础 Deployment + Service

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx
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
apiVersion: v1
kind: Service
metadata:
  name: nginx
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

### Q1: 没有 Linux 基础能学 K8s 吗？

可以，但需要同步补 Linux 基础。本周的 Day 3-4 专门安排了 Linux 基础内容。建议在学习 K8s 的过程中持续积累 Linux 知识，特别是网络和文件系统相关的命令。

### Q2: Docker 和 containerd 有什么关系？

containerd 是从 Docker 中拆分出来的容器运行时组件。Docker 内部使用 containerd 来管理容器的生命周期。K8s 从 1.24 开始不再支持 Docker 作为容器运行时（弃用 dockershim），但 containerd 是完全支持的。这意味着 K8s 集群中通常直接使用 containerd 而不安装完整的 Docker。

### Q3: 学习 K8s 一定要用云服务吗？

不一定。本地学习可以使用 kind（Kubernetes in Docker）、minikube 或 k3s 等轻量级方案。但如果你要学习 ACK 特有的功能（如节点池、Terway 网络、ALB Ingress），则需要使用阿里云环境。建议本周使用本地环境学习基础概念，后续周切换到 ACK 环境。

### Q4: 本周内容太多了，学不完怎么办？

本周内容的优先级：Docker 基础（Day 1）> K8s 架构（Day 5-6）> Docker 进阶（Day 2）> Linux 基础（Day 3-4）。Linux 基础可以在后续周中逐步补充。确保至少完成 Day 1 和 Day 5-6 的学习。

### Q5: Docker 镜像构建很慢怎么办？

配置国内镜像加速器：编辑 `/etc/docker/daemon.json`，添加 `{"registry-mirrors": ["https://mirror.ccs.tencentyun.com"]}`。使用多阶段构建减小镜像体积。利用 Docker BuildKit（`DOCKER_BUILDKIT=1 docker build`）加速构建。

### Q6: kind 和 minikube 该选哪个？

kind 使用 Docker 容器模拟节点，启动快、支持多节点、适合 CI/CD。minikube 功能更丰富（支持插件、Dashboard），适合学习单个功能。推荐本周使用 kind（与 Day 7 实践项目一致）。

---

## 要点总结

| 模块 | 核心知识点 | 学习日 |
|------|-----------|--------|
| Docker 基础 | 容器原理、镜像构建、生命周期 | Day 1 |
| Docker 进阶 | 网络、存储、安全 | Day 2 |
| Linux 核心 | 进程、文件系统、用户权限 | Day 3 |
| Linux 网络 | TCP/IP、网络排障、性能调优 | Day 4 |
| K8s 架构 | Master/Node 组件、kubectl | Day 5 |
| 集群部署 | 集群配置、网络规划 | Day 6 |
| 综合实践 | 跑通第一个 Deployment | Day 7 |

---

## 延伸阅读

- [Docker 架构总览](../../../../../../14-%E5%AE%B9%E5%99%A8%E8%BF%90%E8%A1%8C%E6%97%B6/01-Docker/01-docker-architecture-overview.md)
- [Docker 容器生命周期](../../../../../../14-%E5%AE%B9%E5%99%A8%E8%BF%90%E8%A1%8C%E6%97%B6/01-Docker/03-docker-container-lifecycle.md)
- [Docker 网络深入](../../../../../../14-%E5%AE%B9%E5%99%A8%E8%BF%90%E8%A1%8C%E6%97%B6/01-Docker/04-docker-networking-deep-dive.md)
- [Linux 系统架构](../../../../../../17-%E7%B3%BB%E7%BB%9F%E5%9F%BA%E7%A1%80/01-Linux/01-linux-system-architecture.md)
- [Linux 进程管理](../../../../../../17-系统基础/01-Linux/03-linux-process-management.md)
- [Linux 网络配置](../../../../../../17-系统基础/01-Linux/05-linux-networking-configuration.md)
- [K8s 架构总览](../../../../../../01-%E9%9B%86%E7%BE%A4%E5%9F%BA%E7%A1%80/01-%E6%9E%B6%E6%9E%84%E6%80%BB%E8%A7%88/01-kubernetes-architecture-overview.md)
- [K8s 核心组件深入](../../../../../../01-%E9%9B%86%E7%BE%A4%E5%9F%BA%E7%A1%80/01-%E6%9E%B6%E6%9E%84%E6%80%BB%E8%A7%88/02-core-components-deep-dive.md)
- [kubectl 命令参考](../../../../../../01-集群基础/05-kubectl/02-kubectl-commands-reference.md)

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[23-实体/15-参考与索引/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[22-概念/10-最佳实践/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[35-元数据/metadata/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[26-技能/04-工作负载/pod/培训/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[26-技能/04-工作负载/daemonset/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[10-平台工程/02-运维/04-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[23-实体/15-参考与索引/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[21-生态参考/03-领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[21-生态参考/03-领域索引/terway-index.md|Terway 知识图谱索引]]
- [[21-生态参考/03-领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[21-生态参考/03-领域索引/higress-index.md|Higress 知识图谱索引]]

```

<!-- risk-assessed -->
