# Podman Desktop

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://podman-desktop.io/ |
| **GitHub** | https://github.com/containers/podman-desktop |
| **许可证** | Apache-2.0 |
| **开发语言** | TypeScript |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Podman Desktop 是一个开源的桌面容器管理工具，为开发者提供图形化界面来管理容器、镜像、Pod 和 Kubernetes 集群。它支持 Podman、Docker 和 Kubernetes 等多种容器引擎，让开发者可以在本地无缝地开发、测试和调试容器化应用，并轻松迁移到 Kubernetes 环境。

### 核心特性

- **多引擎支持**: 同时管理 Podman、Docker、Lima、KIND、Minikube 等
- **图形化管理**: 直观的 UI 管理容器、镜像、卷和网络
- **Kubernetes 集成**: 内置 KIND/Minikube，一键部署本地 K8s 集群
- **Pod 管理**: 支持 Podman Pod 的创建和管理
- **镜像构建**: 集成 Dockerfile/Containerfile 构建
- **扩展生态**: 可安装扩展插件增强功能

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                  Podman Desktop                       │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │              Electron UI Layer                 │    │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────────┐  │    │
│  │  │Containers│ │ Images   │ │  Pods        │  │    │
│  │  │ View     │ │ View     │ │  View        │  │    │
│  │  └──────────┘ └──────────┘ └──────────────┘  │    │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────────┐  │    │
│  │  │Kubernetes│ │ Volumes  │ │  Extensions  │  │    │
│  │  │ View     │ │ View     │ │  View        │  │    │
│  │  └──────────┘ └──────────┘ └──────────────┘  │    │
│  └─────────────────────┬────────────────────────┘    │
│                        │ API                          │
│  ┌─────────────────────▼────────────────────────┐    │
│  │            Provider Abstraction               │    │
│  │  ┌──────────────────────────────────────────┐│    │
│  │  │           Provider Manager                ││    │
│  │  └──────────────────────────────────────────┘│    │
│  └─────────────────────┬────────────────────────┘    │
└────────────────────────┼─────────────────────────────┘
                         │
       ┌─────────────────┼─────────────────┐
       │                 │                 │
 ┌─────▼─────┐    ┌──────▼─────┐    ┌─────▼──────┐
 │  Podman    │    │  Docker    │    │ Kubernetes │
 │  Machine   │    │  Engine    │    │  (KIND/    │
 │  (Linux VM)│    │            │    │  Minikube) │
 └───────────┘    └────────────┘    └────────────┘
```

---

## 快速开始

### 安装

```bash
# macOS (Homebrew)
brew install podman-desktop

# Windows (Winget)
winget install RedHat.Podman-Desktop

# Linux (Flatpak)
flatpak install flathub io.podman_desktop.PodmanDesktop
```

### 初始设置

1. 启动 Podman Desktop
2. 点击 "Install" 安装 Podman Engine
3. 启动 Podman Machine (macOS/Windows 需要 VM)
4. 开始使用容器功能

### 运行容器

```bash
# 通过 UI 或命令行
# 1. 点击 Images > Pull an Image
# 2. 输入 nginx:latest
# 3. 点击 Pull
# 4. 点击 Run 启动容器

# 或使用 Podman CLI
podman run -d -p 8080:80 nginx:latest
```

### 创建 Pod

```yaml
# 在 Podman Desktop 中可视化创建 Pod
# 或使用 YAML
apiVersion: v1
kind: Pod
metadata:
  name: web-pod
spec:
  containers:
    - name: nginx
      image: nginx:latest
      ports:
        - containerPort: 80
    - name: php
      image: php:fpm
```

---

## 高级功能

### Kubernetes 本地集群

```bash
# 在 Podman Desktop 中:
# 1. Settings > Resources
# 2. 点击 "Create new Kubernetes cluster"
# 3. 选择 KIND 或 Minikube
# 4. 配置节点数和资源
# 5. 点击 Create

# 验证集群
kubectl get nodes
```

### 部署到 Kubernetes

```bash
# 在 Containers 视图中:
# 1. 右键点击容器
# 2. 选择 "Deploy to Kubernetes"
# 3. 选择目标集群和命名空间
# 4. 配置 Deployment/Service 选项
# 5. 点击 Deploy
```

### 扩展插件

```bash
# 安装扩展
# 1. Extensions > Catalog
# 2. 搜索并安装扩展

# 常用扩展:
# - Bootc: 管理 bootable containers
# - AI Lab: 本地运行 AI 模型
# - Headlamp: Kubernetes 仪表盘
# - Lima: macOS Linux VM 管理
```

### Compose 支持

```yaml
# docker-compose.yml
version: '3'
services:
  web:
    image: nginx:latest
    ports:
      - "8080:80"
  db:
    image: postgres:15
    environment:
      POSTGRES_PASSWORD: secret
```

```bash
# 在 Podman Desktop 中:
# 1. Compose > Import
# 2. 选择 docker-compose.yml
# 3. 点击 Start
```

---

## 与其他方案对比

| 特性 | Podman Desktop | Docker Desktop | Rancher Desktop | Lens |
|:---|:---|:---|:---|:---|
| 容器引擎 | Podman/Docker | Docker | containerd/dockerd | N/A |
| K8s 支持 | KIND/Minikube | K3s | K3s/RKE2 | 仅管理 |
| Pod 支持 | 原生 | 不支持 | 不支持 | N/A |
| 根权限 | 无 (Rootless) | 需要 | 需要 | N/A |
| 许可证 | Apache-2.0 | 商业 | Apache-2.0 | 商业 |
| 扩展 | 开放 | 有限 | 有限 | 插件 |

---

## 最佳实践

1. **Rootless 优先**: 使用 Podman 的 rootless 模式提高安全性
2. **资源限制**: 为 Podman Machine 配置合适的 CPU 和内存
3. **镜像清理**: 定期清理未使用的镜像和卷
4. **本地 K8s**: 使用 KIND 快速创建一次性测试集群
5. **扩展生态**: 探索扩展目录，增强开发体验

---

## 参考资源

- [Podman Desktop 官方文档](https://podman-desktop.io/docs/)
- [Podman Desktop GitHub](https://github.com/containers/podman-desktop)
- [Podman 项目](https://podman.io/)
- [Podman Desktop 扩展](https://podman-desktop.io/extensions/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
