---
title: Docker 架构概述与核心概念
description: 'title: Docker 架构概述与核心概念'
category: general
tags:
- docker
- container
- best-practice
- deep-dive
- architecture
- kubelet
- scheduler
- containerd
- cri-o
- mysql
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 01-docker-architecture-overview是什么？
- 01-docker-architecture-overview的核心概念有哪些？
- 如何理解01-docker-architecture-overview？
trigger_keywords:
- Docker
- 架构概述与核心概念
- container
- runtime
prerequisites:
- kubectl-basics
- mysql-basics
---

title: Docker 架构概述与核心概念
description: '# Docker 架构概述与核心概念'
category: docker
tags:
- docker
- container
- image
- kubelet
- scheduler
- containerd
- cri-o
- mysql
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 开发工程师
- 运维工程师
- SRE
estimated_read_time: 10min
intent_queries:
- Docker 架构概述与核心概念 是什么
- 如何 Docker 架构概述与核心概念
- Kubernetes 13 docker 最佳实践
trigger_keywords:
- Docker
- 架构概述与核心概念
- docker
cross_refs:
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/docker.md
  label: '速查卡: docker'
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

# Docker 架构概述与核心概念

> **适用版本**: Docker 20.10+ / Docker 24.0+ / Docker 25.0+ | **最后更新**: 2026-01
> 
> **生产环境运维专家注**: 本文档从企业级生产环境部署和运维角度深入解析 Docker 架构，包含大规模集群管理、高可用部署、性能优化等实战经验。

---

<!-- chunk: 目录 -->## 目录

- [Docker 发展历史与版本演进](#docker-发展历史与版本演进)
- [Docker 整体架构](#docker-整体架构)
- [Docker Engine 核心组件](#docker-engine-核心组件)
- [容器运行时层级](#容器运行时层级)
- [OCI 标准规范](#oci-标准规范)
- [Docker 替代方案对比](#docker-替代方案对比)
- [生产环境选型建议](#生产环境选型建议)
- [Docker 命令速查](#docker-命令速查)

---

<!-- chunk: Docker 发展历史与版本演进 -->## Docker 发展历史与版本演进

#<!-- chunk: 发展里程碑 -->## 发展里程碑

| 年份 | 版本/事件 | 关键变化 |
|:---:|:---|:---|
| 2013 | Docker 0.1 发布 | dotCloud 开源容器引擎，基于 LXC |
| 2014 | Docker 1.0 | 生产就绪版本，libcontainer 替代 LXC |
| 2015 | OCI 成立 | Docker 贡献 libcontainer → runC |
| 2016 | Docker 1.12 | 内置 Swarm Mode |
| 2017 | containerd 捐献 CNCF | Docker 分离容器运行时 |
| 2017 | Moby 项目成立 | Docker 开源组件重组 |
| 2019 | Docker 企业版售予 Mirantis | Docker Desktop 保留 |
| 2020 | K8s 弃用 dockershim | 推动 containerd/CRI-O 采用 |
| 2021 | Docker Desktop 商业化 | 企业用户需付费订阅 |
| 2023 | Docker 24.0 | 使用 containerd 1.7 |
| 2024 | Docker 25.0 | containerd 镜像存储正式 GA |
| 2025 | Docker 26.0+ | 增强安全特性、改进构建性能 |

#<!-- chunk: 版本命名规范 -->## 版本命名规范

| 版本类型 | 命名格式 | 发布周期 | 示例 |
|:---|:---|:---|:---|
| **稳定版 (Stable)** | YY.MM.patch | 季度发布 | 25.0.3 |
| **边缘版 (Edge)** | YY.MM-ce | 月度发布 | 已废弃 |
| **企业版 (EE)** | YY.MM-ee | 季度发布 | 已售予 Mirantis |

---

<!-- chunk: Docker 整体架构 -->## Docker 整体架构

#<!-- chunk: 架构概览图 -->## 架构概览图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Docker Client                                   │
│                    (docker CLI / Docker Desktop / SDKs)                      │
└─────────────────────────────────┬───────────────────────────────────────────┘
                                  │ REST API (Unix Socket / TCP)
                                  ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              Docker Daemon (dockerd)                         │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────────────┐│
│  │   Images     │ │  Containers  │ │   Networks   │ │      Volumes         ││
│  │  Management  │ │  Management  │ │  Management  │ │     Management       ││
│  └──────────────┘ └──────────────┘ └──────────────┘ └──────────────────────┘│
└─────────────────────────────────┬───────────────────────────────────────────┘
                                  │ gRPC API (containerd.sock)
                                  ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              containerd                                      │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────────────┐│
│  │   Content    │ │   Snapshots  │ │    Tasks     │ │     Namespaces       ││
│  │    Store     │ │   (Images)   │ │ (Containers) │ │     (Isolation)      ││
│  └──────────────┘ └──────────────┘ └──────────────┘ └──────────────────────┘│
└─────────────────────────────────┬───────────────────────────────────────────┘
                                  │ OCI Runtime Spec
                                  ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              OCI Runtime (runc/crun)                         │
│         ┌─────────────┐    ┌─────────────┐    ┌─────────────┐               │
│         │ Namespaces  │    │   Cgroups   │    │   Seccomp   │               │
│         │ (隔离)      │    │ (资源限制)   │    │ (系统调用)   │               │
│         └─────────────┘    └─────────────┘    └─────────────┘               │
└─────────────────────────────────────────────────────────────────────────────┘
                                  │
                                  ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Linux Kernel                                       │
│     Namespaces | Cgroups | OverlayFS | Netfilter | Seccomp | AppArmor       │
└─────────────────────────────────────────────────────────────────────────────┘
```

#<!-- chunk: 组件交互流程 -->## 组件交互流程

```
docker run nginx:latest

[1] Docker CLI → dockerd (REST API over Unix Socket)
    ↓
[2] dockerd → containerd (gRPC: 拉取镜像、创建容器)
    ↓
[3] containerd → containerd-shim-runc-v2 (创建 shim 进程)
    ↓
[4] containerd-shim → runc (OCI Runtime: 创建实际容器)
    ↓
[5] runc → 配置 namespaces/cgroups → 启动容器进程
    ↓
[6] runc 退出，containerd-shim 接管容器生命周期
```

---

<!-- chunk: Docker Engine 核心组件 -->## Docker Engine 核心组件

#<!-- chunk: 组件详解 -->## 组件详解

| 组件 | 进程名 | 职责 | Socket |
|:---|:---|:---|:---|
| **Docker CLI** | docker | 用户命令行接口 | - |
| **Docker Daemon** | dockerd | API 服务、镜像/网络/卷管理 | /var/run/docker.sock |
| **containerd** | containerd | 容器运行时管理 | /run/containerd/containerd.sock |
| **containerd-shim** | containerd-shim-runc-v2 | 容器进程保持、IO转发 | per-container |
| **runc** | runc | OCI 容器实际创建 | - |

#<!-- chunk: dockerd 配置参数 -->## dockerd 配置参数

| 参数 | 默认值 | 说明 |
|:---|:---|:---|
| `--data-root` | /var/lib/docker | Docker 数据目录 |
| `--exec-root` | /var/run/docker | 执行状态目录 |
| `--storage-driver` | overlay2 | 存储驱动 |
| `--log-driver` | json-file | 默认日志驱动 |
| `--log-level` | info | 日志级别 |
| `--max-concurrent-downloads` | 3 | 并发下载数 |
| `--max-concurrent-uploads` | 5 | 并发上传数 |
| `--default-ulimit` | - | 默认 ulimit |
| `--live-restore` | false | daemon 重启不中断容器 |
| `--userland-proxy` | true | 用户态端口代理 |
| `--iptables` | true | 管理 iptables 规则 |

#<!-- chunk: daemon.json 配置示例 -->## daemon.json 配置示例

```json
{
  "data-root": "/var/lib/docker",
  "storage-driver": "overlay2",
  "storage-opts": [
    "overlay2.override_kernel_check=true"
  ],
  "log-driver": "json-file",
  "log-opts": {
    "max-size": "100m",
    "max-file": "3"
  },
  "exec-opts": ["native.cgroupdriver=systemd"],
  "live-restore": true,
  "max-concurrent-downloads": 10,
  "max-concurrent-uploads": 10,
  "default-ulimits": {
    "nofile": {
      "Name": "nofile",
      "Hard": 65536,
      "Soft": 65536
    }
  },
  "registry-mirrors": [
    "https://registry.docker-cn.com",
    "https://mirror.ccs.tencentyun.com"
  ],
  "insecure-registries": [],
  "debug": false,
  "tls": true,
  "tlscacert": "/etc/docker/ca.pem",
  "tlscert": "/etc/docker/server-cert.pem",
  "tlskey": "/etc/docker/server-key.pem",
  "hosts": ["unix:///var/run/docker.sock", "tcp://0.0.0.0:2376"],
  "default-address-pools": [
    {"base": "172.17.0.0/16", "size": 24}
  ],
  "features": {
    "containerd-snapshotter": true
  }
}
```

---

<!-- chunk: 容器运行时层级 -->## 容器运行时层级

#<!-- chunk: 运行时分层架构 -->## 运行时分层架构

| 层级 | 名称 | 代表 | 职责 |
|:---|:---|:---|:---|
| **高级运行时** | Container Engine | Docker, Podman | 镜像管理、API、网络、存储 |
| **中级运行时** | Container Manager | containerd, CRI-O | 容器生命周期管理、镜像分发 |
| **低级运行时** | OCI Runtime | runc, crun, youki | 实际创建容器进程 |

#<!-- chunk: containerd 架构 -->## containerd 架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        containerd                                │
├─────────────────────────────────────────────────────────────────┤
│  gRPC API                                                        │
│  ├── Images     (镜像拉取/推送/存储)                              │
│  ├── Containers (容器元数据管理)                                  │
│  ├── Tasks      (容器进程管理)                                    │
│  ├── Snapshots  (文件系统快照)                                    │
│  ├── Content    (内容寻址存储)                                    │
│  ├── Namespaces (多租户隔离)                                      │
│  ├── Leases     (资源生命周期)                                    │
│  └── Events     (事件流)                                         │
├─────────────────────────────────────────────────────────────────┤
│  Plugins                                                         │
│  ├── Snapshotter: overlayfs, btrfs, zfs, native                  │
│  ├── Runtime:     io.containerd.runc.v2                          │
│  ├── Differ:      walking                                        │
│  └── GC:          scheduler                                      │
└─────────────────────────────────────────────────────────────────┘
```

#<!-- chunk: containerd 配置 (/etc/containerd/config.toml) -->## containerd 配置 (/etc/containerd/config.toml)

```toml
version = 2
root = "/var/lib/containerd"
state = "/run/containerd"

[grpc]
  address = "/run/containerd/containerd.sock"
  uid = 0
  gid = 0
  max_recv_message_size = 16777216
  max_send_message_size = 16777216

[plugins."io.containerd.grpc.v1.cri"]
  sandbox_image = "registry.k8s.io/pause:3.9"
  
  [plugins."io.containerd.grpc.v1.cri".containerd]
    default_runtime_name = "runc"
    
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"
      
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        SystemdCgroup = true

  [plugins."io.containerd.grpc.v1.cri".registry]
    [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
        endpoint = ["https://registry-1.docker.io"]
```

---

<!-- chunk: OCI 标准规范 -->## OCI 标准规范

#<!-- chunk: OCI 规范组成 -->## OCI 规范组成

| 规范 | 用途 | 当前版本 | 核心内容 |
|:---|:---|:---|:---|
| **Runtime Spec** | 容器运行时 | v1.2.0 | 容器配置、生命周期、执行环境 |
| **Image Spec** | 镜像格式 | v1.1.0 | 镜像层、配置、manifest |
| **Distribution Spec** | 镜像分发 | v1.1.0 | Registry API、认证、推拉 |

#<!-- chunk: OCI Runtime Spec - config.json 结构 -->## OCI Runtime Spec - config.json 结构

```json
{
  "ociVersion": "1.2.0",
  "process": {
    "terminal": true,
    "user": { "uid": 0, "gid": 0 },
    "args": ["/bin/sh"],
    "env": ["PATH=/usr/bin:/bin", "TERM=xterm"],
    "cwd": "/",
    "capabilities": {
      "bounding": ["CAP_AUDIT_WRITE", "CAP_KILL", "CAP_NET_BIND_SERVICE"],
      "effective": ["CAP_AUDIT_WRITE", "CAP_KILL", "CAP_NET_BIND_SERVICE"],
      "permitted": ["CAP_AUDIT_WRITE", "CAP_KILL", "CAP_NET_BIND_SERVICE"]
    },
    "rlimits": [
      { "type": "RLIMIT_NOFILE", "hard": 1024, "soft": 1024 }
    ]
  },
  "root": {
    "path": "rootfs",
    "readonly": true
  },
  "hostname": "container",
  "mounts": [
    { "destination": "/proc", "type": "proc", "source": "proc" },
    { "destination": "/dev", "type": "tmpfs", "source": "tmpfs" },
    { "destination": "/sys", "type": "sysfs", "source": "sysfs", "options": ["ro"] }
  ],
  "linux": {
    "namespaces": [
      { "type": "pid" },
      { "type": "network" },
      { "type": "ipc" },
      { "type": "uts" },
      { "type": "mount" },
      { "type": "cgroup" }
    ],
    "resources": {
      "memory": { "limit": 536870912 },
      "cpu": { "shares": 1024, "quota": 100000, "period": 100000 }
    }
  }
}
```

#<!-- chunk: OCI Image Spec - 镜像结构 -->## OCI Image Spec - 镜像结构

```
Image Index (多架构索引)
    │
    ├── Image Manifest (amd64)
    │       ├── Config Blob (配置 JSON)
    │       └── Layer Blobs (文件系统层)
    │           ├── Layer 1 (base image)
    │           ├── Layer 2 (packages)
    │           └── Layer 3 (application)
    │
    └── Image Manifest (arm64)
            ├── Config Blob
            └── Layer Blobs
```

---

<!-- chunk: Docker 替代方案对比 -->## Docker 替代方案对比

#<!-- chunk: 容器引擎对比 -->## 容器引擎对比

| 特性 | Docker | Podman | nerdctl | CRI-O |
|:---|:---:|:---:|:---:|:---:|
| **守护进程** | 需要 (dockerd) | 无 (Daemonless) | 需要 (containerd) | 需要 |
| **根用户** | 默认需要 | 支持 Rootless | 支持 Rootless | 不支持 |
| **OCI 兼容** | ✓ | ✓ | ✓ | ✓ |
| **Pod 支持** | × | ✓ (原生) | ✓ | ✓ |
| **Compose 支持** | ✓ | podman-compose | nerdctl compose | × |
| **Swarm 支持** | ✓ | × | × | × |
| **K8s CRI** | × (已弃用) | × | ✓ | ✓ |
| **Docker CLI 兼容** | 原生 | alias docker=podman | 高度兼容 | 不适用 |
| **构建工具** | BuildKit | Buildah | BuildKit | × |
| **镜像存储** | 私有格式 | OCI 标准 | containerd | OCI 标准 |
| **适用场景** | 开发/小规模生产 | 开发/安全敏感 | 开发/K8s节点 | K8s 专用 |

#<!-- chunk: Rootless 模式对比 -->## Rootless 模式对比

| 方案 | 实现方式 | 网络 | 性能开销 |
|:---|:---|:---|:---|
| **Docker Rootless** | user namespaces + slirp4netns | slirp4netns (较慢) | 中等 |
| **Podman Rootless** | user namespaces + slirp4netns/pasta | pasta (较快) | 低 |
| **nerdctl Rootless** | RootlessKit + slirp4netns | slirp4netns | 中等 |

#<!-- chunk: 容器运行时对比 -->## 容器运行时对比

| 运行时 | 类型 | 语言 | 特点 |
|:---|:---|:---|:---|
| **runc** | 低级 | Go | OCI 参考实现，最广泛使用 |
| **crun** | 低级 | C | 更轻量、启动更快 |
| **youki** | 低级 | Rust | 内存安全、实验性 |
| **gVisor (runsc)** | 沙箱 | Go | 内核隔离、安全性高 |
| **Kata Containers** | VM | Go | 轻量虚拟机、强隔离 |

---

<!-- chunk: 生产环境选型建议 -->## 生产环境选型建议

#<!-- chunk: 场景选型矩阵 -->## 场景选型矩阵

| 场景 | 推荐方案 | 理由 |
|:---|:---|:---|
| **开发环境 (Mac/Win)** | Docker Desktop | 图形界面、一键安装、跨平台 |
| **开发环境 (Linux)** | Docker CE / Podman | 成熟稳定、社区活跃 |
| **CI/CD 构建** | Docker CE + BuildKit | 构建性能优秀、缓存高效 |
| **K8s 生产节点** | containerd / CRI-O | K8s 原生支持、资源占用低 |
| **安全敏感环境** | Podman Rootless | 无守护进程、无 root |
| **多租户隔离** | Kata Containers | VM 级隔离 |
| **合规环境** | gVisor | 内核隔离、减少攻击面 |

#<!-- chunk: 从 Docker 迁移到 containerd -->## 从 Docker 迁移到 containerd

| 步骤 | 命令/操作 |
|:---|:---|
| 1. 导出镜像 | `docker save -o images.tar $(docker images -q)` |
| 2. 停止 Docker | `systemctl stop docker` |
| 3. 安装 containerd | `apt install containerd.io` |
| 4. 配置 containerd | 编辑 `/etc/containerd/config.toml` |
| 5. 导入镜像 | `ctr images import images.tar` |
| 6. 使用 nerdctl | `nerdctl run nginx` |

#<!-- chunk: 企业 Docker Desktop 替代方案 -->## 企业 Docker Desktop 替代方案

| 方案 | 平台 | 特点 | 许可证 |
|:---|:---|:---|:---|
| **Rancher Desktop** | Mac/Win/Linux | K8s + containerd/dockerd | 开源免费 |
| **Podman Desktop** | Mac/Win/Linux | Podman + Podman Machine | 开源免费 |
| **Colima** | Mac/Linux | Lima + containerd/Docker | 开源免费 |
| **OrbStack** | Mac | 快速、低资源占用 | 商业/个人免费 |

---

<!-- chunk: Docker 命令速查 -->## Docker 命令速查

#<!-- chunk: 镜像操作 -->## 镜像操作

| 命令 | 说明 | 示例 |
|:---|:---|:---|
| `docker pull` | 拉取镜像 | `docker pull nginx:1.25` |
| `docker push` | 推送镜像 | `docker push myrepo/app:v1` |
| `docker build` | 构建镜像 | `docker build -t app:v1 .` |
| `docker images` | 列出镜像 | `docker images --format "{{.Repository}}"` |
| `docker rmi` | 删除镜像 | `docker rmi nginx:1.25` |
| `docker tag` | 标记镜像 | `docker tag nginx myrepo/nginx` |
| `docker save` | 导出镜像 | `docker save nginx -o nginx.tar` |
| `docker load` | 导入镜像 | `docker load -i nginx.tar` |
| `docker history` | 镜像历史 | `docker history nginx` |
| `docker inspect` | 镜像详情 | `docker inspect nginx` |

#<!-- chunk: 容器操作 -->## 容器操作

| 命令 | 说明 | 示例 |
|:---|:---|:---|
| `docker run` | 创建并启动 | `docker run -d -p 80:80 nginx` |
| `docker start` | 启动容器 | `docker start container_id` |
| `docker stop` | 停止容器 | `docker stop container_id` |
| `docker restart` | 重启容器 | `docker restart container_id` |
| `docker rm` | 删除容器 | `docker rm -f container_id` |
| `docker ps` | 列出容器 | `docker ps -a` |
| `docker logs` | 查看日志 | `docker logs -f --tail 100 app` |
| `docker exec` | 执行命令 | `docker exec -it app /bin/sh` |
| `docker cp` | 复制文件 | `docker cp app:/app/log.txt .` |
| `docker stats` | 资源统计 | `docker stats --no-stream` |
| `docker top` | 进程列表 | `docker top container_id` |
| `docker update` | 更新配置 | `docker update --memory 1g app` |
| `docker wait` | 等待退出 | `docker wait container_id` |
| `docker kill` | 强制停止 | `docker kill container_id` |

#<!-- chunk: docker run 常用参数 -->## docker run 常用参数

| 参数 | 说明 | 示例 |
|:---|:---|:---|
| `-d` | 后台运行 | `docker run -d nginx` |
| `-it` | 交互式终端 | `docker run -it alpine sh` |
| `--name` | 容器名称 | `--name myapp` |
| `-p` | 端口映射 | `-p 8080:80` |
| `-P` | 随机端口 | `-P` |
| `-v` | 挂载卷 | `-v /data:/app/data` |
| `--mount` | 高级挂载 | `--mount type=volume,src=vol,dst=/data` |
| `-e` | 环境变量 | `-e MYSQL_ROOT_PASSWORD=secret` |
| `--env-file` | 环境文件 | `--env-file .env` |
| `--network` | 网络模式 | `--network host` |
| `--restart` | 重启策略 | `--restart unless-stopped` |
| `--memory` | 内存限制 | `--memory 512m` |
| `--cpus` | CPU 限制 | `--cpus 1.5` |
| `--privileged` | 特权模式 | `--privileged` |
| `--user` | 运行用户 | `--user 1000:1000` |
| `--read-only` | 只读根文件系统 | `--read-only` |
| `--rm` | 退出后删除 | `--rm` |

#<!-- chunk: 网络操作 -->## 网络操作

| 命令 | 说明 | 示例 |
|:---|:---|:---|
| `docker network ls` | 列出网络 | `docker network ls` |
| `docker network create` | 创建网络 | `docker network create mynet` |
| `docker network rm` | 删除网络 | `docker network rm mynet` |
| `docker network connect` | 连接网络 | `docker network connect mynet app` |
| `docker network disconnect` | 断开网络 | `docker network disconnect mynet app` |
| `docker network inspect` | 网络详情 | `docker network inspect bridge` |

#<!-- chunk: 存储操作 -->## 存储操作

| 命令 | 说明 | 示例 |
|:---|:---|:---|
| `docker volume ls` | 列出卷 | `docker volume ls` |
| `docker volume create` | 创建卷 | `docker volume create mydata` |
| `docker volume rm` | 删除卷 | `docker volume rm mydata` |
| `docker volume inspect` | 卷详情 | `docker volume inspect mydata` |
| `docker volume prune` | 清理未用卷 | `docker volume prune -f` |

#<!-- chunk: 系统操作 -->## 系统操作

| 命令 | 说明 | 示例 |
|:---|:---|:---|
| `docker info` | 系统信息 | `docker info` |
| `docker version` | 版本信息 | `docker version` |
| `docker system df` | 磁盘使用 | `docker system df -v` |
| `docker system prune` | 清理资源 | `docker system prune -af --volumes` |
| `docker events` | 事件流 | `docker events --since 1h` |
| `docker login` | 登录仓库 | `docker login registry.example.com` |
| `docker logout` | 登出仓库 | `docker logout` |

---

<!-- chunk: 与 Kubernetes 的关系 -->## 与 Kubernetes 的关系

#<!-- chunk: 演进历史 -->## 演进历史

```
2014-2020: K8s 使用 dockershim 集成 Docker
              (docker CLI 接口)

2020: K8s 宣布弃用 dockershim
      推荐使用 containerd 或 CRI-O

2021: dockershim 从 kubelet 代码中移除

2022+: K8s 节点直接使用 containerd/CRI-O
       Docker 镜像仍然兼容 (OCI 标准)
```

#<!-- chunk: 当前最佳实践 -->## 当前最佳实践

| 环境 | 推荐运行时 | 说明 |
|:---|:---|:---|
| **K8s 生产节点** | containerd | 轻量、K8s 原生支持 |
| **K8s 生产节点** | CRI-O | 专为 K8s 设计 |
| **开发/构建环境** | Docker | 工具链成熟 |
| **镜像构建** | Docker BuildKit | 性能优秀、功能丰富 |
| **本地测试** | Docker / Podman | 易于使用 |

---

<!-- chunk: 相关文档 -->## 相关文档

- [201-docker-images-management](./201-docker-images-management.md) - 镜像管理详解
- [202-docker-container-lifecycle](./202-docker-container-lifecycle.md) - 容器生命周期
- [203-docker-networking-deep-dive](./203-docker-networking-deep-dive.md) - Docker 网络
- [217-linux-container-fundamentals](./217-linux-container-fundamentals.md) - 容器技术基础
- [165-cri-container-runtime-deep-dive](./165-cri-container-runtime-deep-dive.md) - CRI 详解

## Related

- [[domain-20-application-patterns/45-smart-port-shipping.md|45-smart-port-shipping]]
- [[domain-20-application-patterns/65-autonomous-driving-sim.md|65-autonomous-driving-sim]]
- [[domain-20-application-patterns/84-national-park.md|84-national-park]]
- [[domain-20-application-patterns/83-cultural-digitization.md|83-cultural-digitization]]
- [[domain-20-application-patterns/94-smart-prison.md|94-smart-prison]]
- [[domain-20-application-patterns/30-hrtech-saas.md|30-hrtech-saas]]
- [[domain-20-application-patterns/68-quantum-computing-cloud.md|68-quantum-computing-cloud]]
- [[domain-20-application-patterns/64-ai-drug-discovery.md|64-ai-drug-discovery]]
- [[domain-20-application-patterns/91-urban-air-mobility.md|91-urban-air-mobility]]
- [[domain-20-application-patterns/21-cross-border-ecommerce.md|21-cross-border-ecommerce]]
- [[domain-20-application-patterns/71-smart-tax.md|71-smart-tax]]
- [[domain-20-application-patterns/03-cms-architecture.md|03-cms-architecture]]
- [[domain-20-application-patterns/85-hydrogen-energy.md|85-hydrogen-energy]]
- [[domain-20-application-patterns/18-data-midplatform-architecture.md|18-data-midplatform-architecture]]
- [[domain-20-application-patterns/16-video-shortform-architecture.md|16-video-shortform-architecture]]
- [[domain-20-application-patterns/55-crossborder-dtc.md|55-crossborder-dtc]]
- [[domain-20-application-patterns/27-hospitality-tourism.md|27-hospitality-tourism]]
- [[domain-20-application-patterns/40-cloud-gaming.md|40-cloud-gaming]]
- [[domain-20-application-patterns/87-flexible-manufacturing.md|87-flexible-manufacturing]]
- [[domain-20-application-patterns/34-sportstech.md|34-sportstech]]
- [[domain-20-application-patterns/93-digital-twin-factory.md|93-digital-twin-factory]]
- [[domain-20-application-patterns/28-proptech.md|28-proptech]]
- [[domain-20-application-patterns/09-gaming-backend-architecture.md|09-gaming-backend-architecture]]
- [[domain-20-application-patterns/59-industrial-internet-platform.md|59-industrial-internet-platform]]
- [[domain-20-application-patterns/54-social-gaming-metaverse.md|54-social-gaming-metaverse]]
- [[domain-20-application-patterns/31-instant-retail.md|31-instant-retail]]
- [[domain-20-application-patterns/22-nev-connected-vehicle.md|22-nev-connected-vehicle]]
- [[domain-20-application-patterns/33-crossborder-warehouse.md|33-crossborder-warehouse]]
- [[domain-20-application-patterns/05-online-education-architecture.md|05-online-education-architecture]]
- [[domain-20-application-patterns/70-ecny-cbdc.md|70-ecny-cbdc]]
- [[domain-20-application-patterns/62-distributed-energy.md|62-distributed-energy]]
- [[domain-20-application-patterns/75-affective-computing.md|75-affective-computing]]
- [[domain-20-application-patterns/50-unmanned-retail.md|50-unmanned-retail]]
- [[domain-20-application-patterns/42-secondhand-circular.md|42-secondhand-circular]]
- [[domain-20-application-patterns/26-aviation-travel.md|26-aviation-travel]]
- [[domain-20-application-patterns/43-enterprise-im.md|43-enterprise-im]]
- [[domain-20-application-patterns/73-smart-firefighting.md|73-smart-firefighting]]
- [[domain-20-application-patterns/14-smart-healthcare-architecture.md|14-smart-healthcare-architecture]]
- [[domain-20-application-patterns/96-carbon-capture.md|96-carbon-capture]]
- [[domain-20-application-patterns/60-v2x-autonomous-driving.md|60-v2x-autonomous-driving]]
- [[domain-20-application-patterns/74-immersive-xr.md|74-immersive-xr]]
- [[domain-20-application-patterns/78-deep-sea-exploration.md|78-deep-sea-exploration]]
- [[domain-20-application-patterns/12-smart-logistics-architecture.md|12-smart-logistics-architecture]]
- [[domain-20-application-patterns/51-smart-manufacturing-mes.md|51-smart-manufacturing-mes]]
- [[domain-20-application-patterns/08-ai-ml-inference-architecture.md|08-ai-ml-inference-architecture]]
- [[domain-20-application-patterns/23-xinchuang-it-innovation.md|23-xinchuang-it-innovation]]
- [[domain-20-application-patterns/47-smart-mining.md|47-smart-mining]]
- [[domain-20-application-patterns/58-web3-gamefi.md|58-web3-gamefi]]
- [[domain-20-application-patterns/29-agritech-iot.md|29-agritech-iot]]
- [[domain-20-application-patterns/57-digital-therapeutics.md|57-digital-therapeutics]]
- [[domain-20-application-patterns/92-smart-sports-venue.md|92-smart-sports-venue]]
- [[domain-20-application-patterns/76-synthetic-biology.md|76-synthetic-biology]]
- [[domain-20-application-patterns/61-smart-grid.md|61-smart-grid]]
- [[domain-20-application-patterns/17-saas-multitenant-architecture.md|17-saas-multitenant-architecture]]
- [[domain-20-application-patterns/11-smart-retail-architecture.md|11-smart-retail-architecture]]
- [[domain-20-application-patterns/25-quantitative-trading.md|25-quantitative-trading]]
- [[domain-20-application-patterns/81-smart-customs.md|81-smart-customs]]
- [[domain-20-application-patterns/24-insurtech.md|24-insurtech]]
- [[domain-20-application-patterns/90-neuromorphic-computing.md|90-neuromorphic-computing]]
- [[domain-20-application-patterns/46-satellite-internet.md|46-satellite-internet]]
- [[domain-20-application-patterns/52-smart-water.md|52-smart-water]]
- [[domain-20-application-patterns/86-solid-state-battery.md|86-solid-state-battery]]
- [[domain-20-application-patterns/67-brain-computer-interface.md|67-brain-computer-interface]]
- [[domain-20-application-patterns/82-legaltech.md|82-legaltech]]
- [[domain-20-application-patterns/15-energy-power-architecture.md|15-energy-power-architecture]]
- [[domain-20-application-patterns/37-pet-economy.md|37-pet-economy]]
- [[domain-20-application-patterns/49-livestream-ecommerce.md|49-livestream-ecommerce]]
- [[domain-20-application-patterns/66-space-internet.md|66-space-internet]]
- [[domain-20-application-patterns/06-fintech-architecture.md|06-fintech-architecture]]
- [[domain-20-application-patterns/88-nanomaterials.md|88-nanomaterials]]
- [[domain-20-application-patterns/10-social-media-architecture.md|10-social-media-architecture]]
- [[domain-20-application-patterns/39-smart-campus.md|39-smart-campus]]
- [[domain-20-application-patterns/13-digital-government-architecture.md|13-digital-government-architecture]]
- [[domain-20-application-patterns/48-vocational-edtech.md|48-vocational-edtech]]
- [[domain-20-application-patterns/72-digital-twin-city.md|72-digital-twin-city]]
- [[domain-20-application-patterns/32-smart-restaurant.md|32-smart-restaurant]]
- [[domain-20-application-patterns/89-crispr-gene-editing.md|89-crispr-gene-editing]]
- [[domain-20-application-patterns/56-smart-elderly-care.md|56-smart-elderly-care]]
- [[domain-20-application-patterns/44-martech-adtech.md|44-martech-adtech]]
- [[domain-20-application-patterns/95-industrial-metaverse.md|95-industrial-metaverse]]
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]

## See Also

- [[domain-13-container-runtime/12-java-containerization-guide.md|12-java-containerization-guide]]
- [[domain-13-container-runtime/99-docker-commands-reference.md|99-docker-commands-reference]]
- [[domain-13-container-runtime/02-docker-images-management.md|02-docker-images-management]]
- [[domain-13-container-runtime/03-docker-container-lifecycle.md|03-docker-container-lifecycle]]
