# bootc

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://containers.github.io/bootc/ |
| **GitHub** | https://github.com/containers/bootc |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust |
| **CNCF 状态** | Sandbox |

---

## 项目概述

bootc 是一个基于 OCI 容器镜像的 Linux 系统启动和升级工具，将容器镜像作为操作系统的部署单元。它允许使用标准的容器构建工具（如 Dockerfile）来定义和构建可启动的 Linux 系统，并通过事务性更新机制实现系统的原子升级和回滚。bootc 将容器工作流的优势（镜像注册中心、版本标签、CI/CD 流水线）引入操作系统管理领域。

### 核心特性

- **容器即操作系统**: 使用 OCI 容器镜像定义完整的可启动 Linux 系统
- **原子更新**: 事务性系统升级，失败可自动回滚
- **Dockerfile 构建**: 使用标准 Dockerfile 构建操作系统镜像
- **OCI 分发**: 通过容器镜像注册中心分发和管理系统镜像
- **就地升级**: 将现有系统切换到基于 bootc 的镜像管理模式
- **与 ostree 集成**: 底层使用 ostree 实现文件系统的事务性管理

---

## 架构设计

```
┌─────────────────────────────────────────────┐
│              构建流程                         │
│                                              │
│  Dockerfile ──► Container Build ──► OCI 镜像 │
│                                   │          │
│                         Push ◄────┘          │
│                           │                  │
│                    ┌──────▼──────┐           │
│                    │ OCI Registry│           │
│                    └──────┬──────┘           │
└───────────────────────────┼──────────────────┘
                            │ Pull
┌───────────────────────────▼──────────────────┐
│              目标主机                          │
│                                               │
│  ┌──────────────────────────────────┐        │
│  │         bootc daemon              │        │
│  │  (镜像拉取 / 更新管理 / 回滚)     │        │
│  └──────────────┬───────────────────┘        │
│                 │                              │
│  ┌──────────────▼───────────────────┐        │
│  │     ostree / composefs            │        │
│  │  (事务性文件系统 / 原子切换)       │        │
│  └──────────────┬───────────────────┘        │
│                 │                              │
│  ┌──────────────▼───────────────────┐        │
│  │    Boot Loader (GRUB/systemd-boot)│        │
│  │  (引导到新/旧系统)                │        │
│  └──────────────────────────────────┘        │
└───────────────────────────────────────────────┘
```

---

## 快速开始

### 构建可启动镜像

```dockerfile
# Containerfile - 定义操作系统镜像
FROM quay.io/fedora/fedora-bootc:40

# 安装系统软件包
RUN dnf install -y nginx postgresql-server tmux && dnf clean all

# 配置系统服务
RUN systemctl enable nginx postgresql

# 添加配置文件
COPY nginx.conf /etc/nginx/nginx.conf
COPY sshd_config /etc/ssh/sshd_config

# 添加用户
RUN useradd -m admin && echo "admin:changeme" | chpasswd
```

```bash
# 构建镜像
podman build -t quay.io/myorg/my-server:latest .

# 推送到注册中心
podman push quay.io/myorg/my-server:latest
```

### 安装到裸金属/虚拟机

```bash
# 使用 bootc-image-builder 生成可安装的磁盘镜像
sudo podman run --rm -it --privileged \
  --pull=newer \
  -v ./output:/output \
  quay.io/centos-bootc/bootc-image-builder:latest \
  --type qcow2 \
  quay.io/myorg/my-server:latest

# 生成 ISO 安装介质
sudo podman run --rm -it --privileged \
  -v ./output:/output \
  quay.io/centos-bootc/bootc-image-builder:latest \
  --type iso \
  quay.io/myorg/my-server:latest
```

### 系统升级

```bash
# 检查当前镜像状态
bootc status

# 切换到新镜像版本
bootc switch quay.io/myorg/my-server:v2.0

# 从当前镜像拉取最新更新
bootc upgrade

# 回滚到上一版本
bootc rollback
```

---

## 高级用法

### 自动更新配置

```ini
# /etc/bootc/bootc.toml
[updates]
# 自动检查和应用更新
enabled = true
# 检查间隔
check_interval = "1h"
```

### 就地迁移现有系统

```bash
# 将现有 Fedora/CentOS 系统迁移到 bootc 管理
bootc install to-existing-root \
  --source-imgref quay.io/myorg/my-server:latest
```

### 多架构构建

```bash
# 构建多架构镜像
podman manifest create quay.io/myorg/my-server:latest
podman build --platform linux/amd64 -t quay.io/myorg/my-server:latest-amd64 .
podman build --platform linux/arm64 -t quay.io/myorg/my-server:latest-arm64 .
podman manifest add quay.io/myorg/my-server:latest quay.io/myorg/my-server:latest-amd64
podman manifest add quay.io/myorg/my-server:latest quay.io/myorg/my-server:latest-arm64
podman manifest push quay.io/myorg/my-server:latest
```

---

## 与其他方案对比

| 特性 | bootc | Flatcar | Talos | 传统 RPM/DEB |
|:---|:---|:---|:---|:---|
| 构建方式 | Dockerfile | 专用工具 | 专用配置 | 包管理器 |
| 更新方式 | 容器镜像拉取 | 分区切换 | API 驱动 | 包更新 |
| 回滚能力 | 原子回滚 | 原子回滚 | 原子回滚 | 困难 |
| 自定义 | 高 (Dockerfile) | 中等 | 低 (不可变) | 高 |
| 分发方式 | OCI Registry | 专用渠道 | 专用渠道 | 包仓库 |
| 适用场景 | 服务器/边缘 | 容器主机 | K8s 节点 | 通用 |

---

## 最佳实践

1. **基础镜像选择**: 使用官方 bootc 基础镜像（Fedora/CentOS bootc）作为起点
2. **分层构建**: 将通用配置放在基础层，应用特定配置放在上层
3. **CI/CD 集成**: 将系统镜像构建集成到 CI/CD 流水线，自动测试和发布
4. **版本标签**: 使用语义版本标签管理系统镜像，保留回滚路径
5. **最小化镜像**: 只安装必要的包，减小镜像大小和攻击面

---

## 参考资源

- [bootc 官方文档](https://containers.github.io/bootc/)
- [bootc GitHub](https://github.com/containers/bootc)
- [bootc-image-builder](https://github.com/osbuild/bootc-image-builder)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
