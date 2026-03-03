# Lima

> **成熟度**: Incubating | **加入时间**: 2023-08 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://lima-vm.io |
| **GitHub** | https://github.com/lima-vm/lima |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Provisioning & Development |

---

## 项目概述

Lima（Linux virtual Machine）是 macOS 和 Linux 上的轻量级 Linux VM 管理工具。它类似于 WSL2，提供自动文件共享、端口转发和 containerd 集成，是 Docker Desktop 的开源替代方案。

## 核心特性

- **自动文件共享**: 主机目录自动挂载到 VM
- **自动端口转发**: VM 端口自动映射到主机
- **containerd 集成**: 内置 containerd 和 nerdctl
- **多架构支持**: AMD64、ARM64 (Apple Silicon)
- **多发行版**: Ubuntu、Debian、Fedora、Alpine 等
- **模板系统**: 预配置模板快速启动

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      Lima Architecture                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    Host (macOS/Linux)                     │   │
│  │                                                           │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌────────────────┐   │   │
│  │  │   limactl   │  │    lima     │  │   nerdctl      │   │   │
│  │  │   (CLI)     │  │   (shell)   │  │   (container)  │   │   │
│  │  └─────────────┘  └─────────────┘  └────────────────┘   │   │
│  │         │                │                  │            │   │
│  │         └────────────────┼──────────────────┘            │   │
│  │                          │                               │   │
│  │                    SSH / 9P / vsock                       │   │
│  │                          │                               │   │
│  └──────────────────────────┼───────────────────────────────┘   │
│                             │                                    │
│  ┌──────────────────────────┼───────────────────────────────┐   │
│  │                          ▼                               │   │
│  │                    Lima VM (QEMU)                        │   │
│  │                                                          │   │
│  │  ┌─────────────────────────────────────────────────────┐│   │
│  │  │                  Guest Linux                        ││   │
│  │  │                                                     ││   │
│  │  │  ┌──────────────┐  ┌───────────────────────────┐  ││   │
│  │  │  │  containerd  │  │    Shared Directories     │  ││   │
│  │  │  │  (+ nerdctl) │  │  /Users/xxx -> /Users/xxx │  ││   │
│  │  │  └──────────────┘  └───────────────────────────┘  ││   │
│  │  │                                                     ││   │
│  │  │  ┌──────────────┐  ┌───────────────────────────┐  ││   │
│  │  │  │   sshd       │  │   Port Forwarding         │  ││   │
│  │  │  │              │  │   VM:8080 -> Host:8080    │  ││   │
│  │  │  └──────────────┘  └───────────────────────────┘  ││   │
│  │  └─────────────────────────────────────────────────────┘│   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# macOS (Homebrew)
brew install lima

# Linux
curl -fsSL https://get.lima-vm.io | sh
```

### 创建默认 VM

```bash
# 启动默认 Ubuntu VM
limactl start

# 进入 VM shell
lima

# 或直接执行命令
lima uname -a
lima nerdctl run -it --rm alpine
```

### 使用模板

```bash
# 列出可用模板
limactl start --list-templates

# 使用 Docker 模板
limactl start --name=docker template://docker

# 使用 Kubernetes 模板
limactl start --name=k8s template://k8s

# 使用 Fedora 模板
limactl start --name=fedora template://fedora
```

---

## 配置文件

```yaml
# ~/.lima/default/lima.yaml
images:
  - location: "https://cloud-images.ubuntu.com/releases/22.04/release/ubuntu-22.04-server-cloudimg-amd64.img"
    arch: "x86_64"
  - location: "https://cloud-images.ubuntu.com/releases/22.04/release/ubuntu-22.04-server-cloudimg-arm64.img"
    arch: "aarch64"

cpus: 4
memory: "8GiB"
disk: "100GiB"

mounts:
  - location: "~"
    writable: true
  - location: "/tmp/lima"
    writable: true

containerd:
  system: true
  user: true

portForwards:
  - guestPort: 8080
    hostPort: 8080
  - guestPortRange: [3000, 3999]
    hostPortRange: [3000, 3999]

provision:
  - mode: system
    script: |
      #!/bin/bash
      apt-get update
      apt-get install -y htop
```

---

## 常用命令

```bash
# VM 管理
limactl list                    # 列出所有 VM
limactl start default           # 启动 VM
limactl stop default            # 停止 VM
limactl delete default          # 删除 VM
limactl shell default           # 进入 VM shell

# 容器操作 (使用 nerdctl)
lima nerdctl run -d -p 8080:80 nginx
lima nerdctl ps
lima nerdctl images
lima nerdctl build -t myapp .

# 文件操作
limactl copy default:/etc/hosts ./hosts
limactl copy ./file.txt default:/tmp/
```

---

## Docker 兼容模式

```bash
# 启动 docker 模板
limactl start --name=docker template://docker

# 配置 Docker CLI 使用 Lima
export DOCKER_HOST=unix://$(limactl list docker --format 'unix://{{.Dir}}/sock/docker.sock')

# 现在可以使用 docker 命令
docker run hello-world
docker-compose up
```

### colima 集成

```bash
# colima 是 Lima 的 Docker 封装
brew install colima
colima start

# 自动配置 Docker CLI
docker ps
```

---

## Kubernetes 支持

```bash
# 使用 k8s 模板
limactl start --name=k8s template://k8s

# 配置 kubectl
export KUBECONFIG=$(limactl list k8s --format '{{.Dir}}/copied-from-guest/kubeconfig.yaml')

# 使用 kubectl
kubectl get nodes
kubectl get pods -A
```

### k3s 模板

```bash
limactl start --name=k3s template://k3s

# 访问集群
lima kubectl get nodes
```

---

## 网络配置

```yaml
# 高级网络配置
networks:
  - lima: shared    # 共享网络
  - lima: bridged   # 桥接网络
  - lima: host      # 主机网络

# VDE 网络 (高级)
networks:
  - vzNAT: true
```

---

## 最佳实践

1. **资源配置**: 根据工作负载调整 CPU 和内存
2. **文件共享**: 只挂载必要目录提高性能
3. **模板使用**: 使用预配置模板快速启动
4. **快照备份**: 定期创建 VM 快照
5. **清理**: 定期清理未使用的 VM 和镜像

---

## 参考资源

- [官方文档](https://lima-vm.io/docs)
- [GitHub Repo](https://github.com/lima-vm/lima)
- [模板列表](https://github.com/lima-vm/lima/tree/master/templates)
- [nerdctl 文档](https://github.com/containerd/nerdctl)

---

**维护者**: Kudig Team | **许可证**: MIT
