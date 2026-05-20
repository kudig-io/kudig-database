---
title: 'Day 1: Docker 容器基础'
description: '- "镜像构建优化怎么做"'
category: learning
tags:
- k8s
- training
- hands-on
- containerd
- cri-o
- docker
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 1: Docker 容器基础 是什么'
- '如何 Day 1: Docker 容器基础'
trigger_keywords:
- Day
- '1:'
- Docker
- 容器基础
- learn
---


# Day 1: Docker 容器基础

```yaml
---
id: LEARN-ONE-MONTH-W1-DAY1
title: Day 1 - Docker 容器基础
topic: docker
type: hands-on-guide
tags: [docker, container, image, dockerfile, build, run, namespace, cgroup, hands-on, week-1]
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - "Docker 容器本质是什么"
  - "Dockerfile 怎么写"
  - "镜像构建优化怎么做"
  - "Docker 和 K8s 什么关系"
trigger_keywords:
  - Docker
  - 容器
  - 镜像
  - Dockerfile
  - docker build
  - docker run
  - docker pull
  - docker ps
  - docker logs
  - docker exec
  - Namespace
  - Cgroup
  - UnionFS
  - 镜像分层
  - 容器生命周期
reading_level: beginner
audience:
  - sre
  - ops-engineer
  - developer
estimated_read_time: 45min
related_domains:
  - domain-13-docker
  - domain-14-linux
related_topics:
  - docker
  - container
  - image
related:
  - topic-learn/public-training/one-month/week-1-foundation/day-2-docker-advanced.md
  - domain-13-docker/01-docker-fundamentals-concepts.md
---
```

## 概述

Docker 是学习 Kubernetes 的第一块基石。Kubernetes 的核心功能就是编排和管理容器，而 Docker 是目前最流行的容器运行时之一（准确地说，K8s 1.24+ 使用 containerd 作为运行时，但 Docker 的概念和操作仍然是理解容器技术的基础）。

今天的学习目标不是成为 Docker 专家，而是理解容器技术的本质、掌握 Docker 的核心操作，并建立"容器与 K8s 的关系"的认知框架。后续学习中遇到容器相关的问题时，你能够快速定位和解决。

### 学习目标

- 理解 Docker 与 Kubernetes 的关系（CRI、容器运行时）
- 掌握容器生命周期管理（创建、运行、停止、删除）
- 能够构建和运行自定义 Docker 镜像
- 理解 Docker 镜像分层原理和构建优化
- 掌握容器资源查看和调试方法

---

## 核心概念详解

### 容器的本质：Namespace + Cgroup + UnionFS

容器并不是一种全新的虚拟化技术，而是 Linux 内核已有特性的组合应用。理解这三个概念是理解容器技术的基础。

**Namespace（命名空间）** 提供了资源隔离。Linux 内核提供了七种 Namespace：

- **PID Namespace**: 进程隔离。容器内的进程只能看到容器内的其他进程，看不到宿主机上的进程。容器内的第一个进程 PID 为 1，但它实际上是宿主机上的某个 PID
- **NET Namespace**: 网络隔离。每个容器有独立的网络栈，包括网卡、IP 地址、路由表、iptables 规则。K8s 中每个 Pod 有独立的 NET Namespace（Pause 容器创建），Pod 内的容器共享这个 Namespace
- **MNT Namespace**: 文件系统挂载点隔离。每个容器看到的是独立的文件系统视图
- **UTS Namespace**: 主机名隔离。每个容器可以有独立的 hostname
- **IPC Namespace**: 进程间通信隔离。容器间的信号量、消息队列、共享内存互不影响
- **USER Namespace**: 用户隔离。容器内的 root 用户可以映射到宿主机的非 root 用户，提高安全性
- **CGROUP Namespace**: cgroup 根目录隔离

**Cgroup（Control Group）** 提供了资源限制。它限制容器可以使用的 CPU、内存、磁盘 IO 等资源。K8s 中的 `resources.limits` 和 `resources.requests` 最终就是通过 cgroup 来实现的。当容器使用的内存超过 limits 时，内核会触发 OOM Killer 终止容器进程（在 K8s 中表现为 OOMKilled）。

**UnionFS（联合文件系统）** 提供了镜像分层。Docker 镜像由多个只读层（Layer）组成，每层对应 Dockerfile 中的一条指令。容器运行时，在镜像顶部添加一个可写层（Container Layer）。这种设计带来了几个关键好处：

- **层共享**: 多个镜像可以共享相同的基础层，节省磁盘空间和拉取时间。例如，所有基于 `nginx:alpine` 的镜像都共享 Alpine Linux 的基础层
- **构建缓存**: Dockerfile 中未修改的指令可以复用缓存层，加速镜像构建
- **快速启动**: 因为大部分数据（只读层）已经在本地，创建新容器只需要添加薄薄的可写层

### 容器 vs 虚拟机

理解容器和虚拟机的区别有助于建立正确的技术认知：

| 维度 | 容器 | 虚拟机 |
|------|------|--------|
| 隔离方式 | Namespace + Cgroup（进程级隔离） | Hypervisor（硬件级隔离） |
| 内核 | 共享宿主机内核 | 独立的 Guest OS 内核 |
| 启动速度 | 毫秒级（创建进程） | 分钟级（启动操作系统） |
| 资源开销 | 极低（无需运行 Guest OS） | 较高（每个 VM 需要完整 OS） |
| 镜像大小 | MB 级（分层复用） | GB 级（完整 OS） |
| 隔离强度 | 较弱（共享内核，内核漏洞影响所有容器） | 较强（独立内核） |
| 适用场景 | 微服务、CI/CD、弹性扩缩容 | 强隔离需求、不同操作系统 |

### Docker 架构

Docker 采用 Client-Server 架构：

- **Docker Client**（docker 命令行）: 用户与 Docker 交互的接口。Client 将命令通过 REST API 发送给 Docker Daemon
- **Docker Daemon**（dockerd）: Docker 的核心服务进程，负责管理镜像、容器、网络和存储卷。它监听 Docker Client 的请求并执行相应操作
- **Registry**（镜像仓库）: 存储和分发 Docker 镜像的服务。Docker Hub 是最大的公共仓库，ACR（阿里云容器镜像服务）是企业级私有仓库

镜像和容器的关系类似"类"与"实例"的关系：镜像是只读的模板（类），容器是镜像的可运行实例（实例）。一个镜像可以创建多个容器，每个容器有独立的状态和可写层。

### 镜像分层与 Dockerfile

Dockerfile 是构建镜像的蓝图。每条指令会创建一个新的镜像层：

```dockerfile
FROM nginx:alpine          # 基础镜像层（Alpine Linux + Nginx）
COPY index.html /usr/share/nginx/html/  # 新增一层（复制文件）
EXPOSE 80                  # 元数据（不增加层大小）
CMD ["nginx", "-g", "daemon off;"]  # 默认启动命令
```

构建优化的核心原则：

- **减少层数**: 合并多个 RUN 指令（使用 `&&` 连接），减少不必要的层
- **利用缓存**: 将不常变化的指令放在前面（如安装系统包），常变化的指令放在后面（如复制源代码）
- **减小体积**: 使用多阶段构建（multi-stage build），只保留运行时需要的文件
- **清理缓存**: 在 RUN 指令中清理包管理器缓存（如 `rm -rf /var/cache/apk/*`）

### Docker 与 Kubernetes 的关系

在 K8s 中，容器的实际运行由**容器运行时（Container Runtime）**负责。K8s 通过 **CRI（Container Runtime Interface）**与容器运行时交互。Docker 曾经是 K8s 的默认运行时，但从 K8s 1.24 开始，dockershim 已被移除。

但这并不意味着 Docker 的知识不再重要：

- **Dockerfile 仍然是构建镜像的标准方式**，无论底层运行时是 containerd 还是 CRI-O
- **Docker CLI 的概念和命令与 K8s 的操作有对应关系**：docker run → Pod 创建，docker compose → Deployment 定义
- **Docker 的网络和存储概念是理解 K8s 网络和存储的基础**：bridge → CNI，volume → PersistentVolume
- **镜像仓库（如 ACR）是 K8s 和 Docker 共用的基础设施**

---

## 实战演练

### 任务 1: 基础容器操作 (45min)

```bash
# 拉取镜像
docker pull nginx:latest
docker pull alpine:latest

# 查看本地镜像
docker images
docker images --format "table {{.Repository}}\t{{.Tag}}\t{{.Size}}"

# 运行容器（后台模式）
docker run -d --name my-nginx -p 8080:80 nginx:latest

# 查看运行中的容器
docker ps
docker ps -a

# 进入容器执行命令
docker exec -it my-nginx /bin/bash
# 在容器内:
# ls /usr/share/nginx/html/
# cat /etc/nginx/nginx.conf
# exit

# 查看容器日志
docker logs my-nginx
docker logs -f my-nginx --tail 20

# 查看容器详细信息
docker inspect my-nginx | jq '.[0].NetworkSettings.IPAddress'

# 停止和删除容器
docker stop my-nginx
docker rm my-nginx

# 清理所有停止的容器
docker container prune -f
```

### 任务 2: 构建自定义镜像 (45min)

```bash
# 创建练习目录
mkdir -p ~/docker-practice && cd ~/docker-practice

# 创建 Dockerfile（优化版本）
cat > Dockerfile << 'EOF'
FROM nginx:alpine

LABEL maintainer="devops-team"
LABEL description="Custom nginx with static content"

COPY index.html /usr/share/nginx/html/
COPY nginx.conf /etc/nginx/conf.d/default.conf

EXPOSE 80

HEALTHCHECK --interval=30s --timeout=3s --retries=3 \
  CMD wget -qO- http://localhost/ || exit 1

CMD ["nginx", "-g", "daemon off;"]
EOF

# 创建自定义首页
cat > index.html << 'EOF'
<!DOCTYPE html>
<html>
<head><title>My First Docker App</title></head>
<body>
  <h1>Hello from Docker!</h1>
  <p>This container was built from a custom Dockerfile.</p>
  <p>Hostname: <span id="hostname"></span></p>
  <script>document.getElementById('hostname').textContent = window.location.hostname;</script>
</body>
</html>
EOF

# 创建自定义 Nginx 配置
cat > nginx.conf << 'EOF'
server {
    listen 80;
    server_name localhost;
    root /usr/share/nginx/html;
    index index.html;

    location / {
        try_files $uri $uri/ =404;
    }

    location /health {
        return 200 'OK';
        add_header Content-Type text/plain;
    }
}
EOF

# 构建镜像
docker build -t my-nginx:v1 .

# 查看构建历史（理解分层）
docker history my-nginx:v1

# 运行自定义镜像
docker run -d --name my-app -p 8080:80 my-nginx:v1

# 测试
curl http://localhost:8080
curl http://localhost:8080/health

# 查看健康检查状态
docker inspect my-app | jq '.[0].State.Health'
```

### 任务 3: 镜像管理 (30min)

```bash
# 镜像标签管理
docker tag my-nginx:v1 my-nginx:latest
docker tag my-nginx:v1 registry.cn-hangzhou.aliyuncs.com/my-namespace/my-nginx:v1

# 导出镜像（离线传输）
docker save -o my-nginx.tar my-nginx:v1
ls -lh my-nginx.tar

# 导入镜像
docker load -i my-nginx.tar

# 查看镜像层详情
docker inspect my-nginx:v1 | jq '.[0].RootFS.Layers'

# 清理未使用的资源
docker system df
docker system prune -f
docker image prune -a -f
```

### 任务 4: 容器资源查看与调试 (30min)

```bash
# 实时监控容器资源使用
docker stats
docker stats --no-stream

# 查看特定容器的资源限制
docker inspect my-app | jq '.[0].HostConfig.Memory'
docker inspect my-app | jq '.[0].HostConfig.CpuShares'

# 查看容器内进程
docker top my-app

# 查看容器文件系统变化（对比镜像）
docker diff my-app

# 查看容器端口映射
docker port my-app

# 容器内网络调试
docker exec my-app wget -qO- http://localhost/health
docker exec my-app netstat -tlnp 2>/dev/null || docker exec my-app ss -tlnp
```

---

## 常见问题

### Q1: Docker 和 containerd 有什么区别？

containerd 是从 Docker 中拆分出来的容器运行时组件。Docker 的架构是：Docker CLI → Docker Daemon → containerd → runc。K8s 1.24+ 直接使用 containerd，跳过了 Docker Daemon 层。这意味着 K8s 集群中不再需要安装完整的 Docker，但 Dockerfile 和镜像格式是完全兼容的。

### Q2: 容器退出码的含义是什么？

常见退出码：0（正常退出）、1（应用错误）、137（OOMKilled，被 SIGKILL 终止）、139（段错误，被 SIGSEGV 终止）、143（被 SIGTERM 优雅终止）。在 K8s 中可以通过 `kubectl describe pod` 查看容器的 Last State 和 Exit Code。

### Q3: 镜像构建太慢怎么优化？

优化策略：1) 使用更小的基础镜像（alpine < slim < full）；2) 合并 RUN 指令减少层数；3) 将不常变化的层放在前面利用缓存；4) 使用多阶段构建只保留最终产物；5) 使用 BuildKit（`DOCKER_BUILDKIT=1 docker build`）加速构建。

### Q4: Docker 网络模式有哪些？

四种主要模式：1) **bridge**（默认）：容器连接到虚拟网桥 docker0，通过 NAT 访问外部网络；2) **host**：容器直接使用宿主机网络栈，无网络隔离；3) **none**：容器没有网络接口；4) **overlay**：用于跨主机的容器通信（Docker Swarm）。K8s 使用 CNI 插件管理 Pod 网络，与 Docker 的网络模式是不同的实现方式。

---

## 要点总结

| 知识点 | 要点 |
|--------|------|
| 容器本质 | Namespace（隔离）+ Cgroup（限制）+ UnionFS（分层） |
| 镜像分层 | 只读层 + 可写层，层共享和缓存加速构建 |
| 容器 vs VM | 共享内核、启动快、开销低、隔离弱 |
| Dockerfile | 构建镜像的蓝图，优化原则：减层数、用缓存、小体积 |
| Docker 与 K8s | K8s 用 containerd 运行容器，Dockerfile 仍是构建标准 |

---

## 延伸阅读

- [Docker 架构总览](../../domain-13-docker/01-docker-architecture-overview.md)
- [Docker 容器生命周期](../../domain-13-docker/03-docker-container-lifecycle.md)
- [Docker 镜像构建优化](../../domain-13-docker/02-docker-image-build-optimization.md)
- [Docker 网络深入](../../domain-13-docker/04-docker-networking-deep-dive.md)
- [Docker 安全最佳实践](../../domain-13-docker/07-docker-security-best-practices.md)
- [Docker 命令参考](../../domain-13-docker/99-docker-commands-reference.md)
