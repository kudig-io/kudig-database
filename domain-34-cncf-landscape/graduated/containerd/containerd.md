---
title: containerd
description: '## 项目概述

containerd 是 CNCF 毕业的高层容器运行时，支持 Kubernetes CRI、容器生命周期管理、镜像存储和快照管理。

## 配套文档

- [02-containerd-v2-features.md](02-containerd-v2-features.md) - containerd 2.0 新特性详解
- [03-containerd-security-hardening.md](03-containerd-security-hardening.md) - 生产安全加固指南
- [04-containerd-upgrade-migration.md](04-containerd-upgrade-migration.md) - 升级迁移指南
- [05-containerd-windows-support.md](05-containerd-windows-support.md) - Windows 容器支持
- [06-containerd-observability.md](06-containerd-observability.md) - 分布式追踪与可观测性
- [07-containerd-disaster-recovery.md](07-containerd-disaster-recovery.md) - 灾难恢复与业务连续性
- [08-containerd-multi-tenant.md](08-containerd-multi-tenant.md) - 多租户与共享集群配置'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- kubelet
- containerd
- containerd-2.0
- cri-o
- docker
- harbor
- security
- upgrade
- windows
- observability
- tracing
- disaster-recovery
- multi-tenant
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- containerd 是什么
- 如何 containerd
- containerd 2.0 新特性 是什么
- containerd 如何升级到 2.0
- containerd 安全加固 如何配置
- containerd 故障排查 步骤
- containerd 架构 原理
- containerd 配置 CRI 多运行时
- containerd 迁移 如何迁移
- containerd 性能调优 生产环境
- containerd Windows 容器 支持
- containerd 分布式追踪 配置
- containerd 灾难恢复 如何恢复
- containerd 多租户 配置
trigger_keywords:
- containerd
- containerd 2.0
- cncf
- landscape
- containerd 升级
- containerd 安全
- containerd 故障排查
- containerd 配置
- containerd 架构
- containerd Windows
- containerd 分布式追踪
- containerd 灾难恢复
- containerd 多租户
---

# containerd

> **成熟度**: Graduated | **加入时间**: 2017-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://containerd.io |
| **GitHub** | https://github.com/containerd/containerd |
| **文档** | https://containerd.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Container Runtime |

---

## 项目概述

### 简介
containerd 是一个行业标准的容器运行时，强调简单性、健壮性和可移植性，是 Docker 和 Kubernetes 的核心容器运行时。

### 核心定位
containerd 作为高层容器运行时，管理容器的完整生命周期，包括镜像传输和存储、容器执行和监控、底层存储和网络附件，是云原生基础设施的关键组件。

### 发展历程
- **2016-12**: Docker 将 containerd 作为独立项目开源
- **2017-03**: 加入 CNCF 作为孵化项目
- **2019-02**: 成为 CNCF 毕业项目
- **2024**: containerd v1.7+ 持续演进

---

## 核心功能

### 主要特性
- **镜像管理**: 拉取、推送、存储 OCI 镜像
- **容器生命周期**: 创建、启动、停止、删除容器
- **快照管理**: 支持多种快照驱动
- **CRI 支持**: Kubernetes 容器运行时接口
- **插件架构**: 可扩展的插件系统
- **命名空间**: 多租户资源隔离

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                        containerd                           │
│  ┌──────────────────────────────────────────────────────── ┐│
│  │                      gRPC API                           ││
│  └─────────────────────────────────────────────────────────┘│
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐│
│  │   Images    │ │  Containers │ │       Content           ││
│  │   Service   │ │   Service   │ │       Store             ││
│  └─────────────┘ └─────────────┘ └─────────────────────────┘│
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐│
│  │  Snapshots  │ │    Tasks    │ │       Events            ││
│  │   Service   │ │   Service   │ │       Service           ││
│  └─────────────┘ └─────────────┘ └─────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                         Shim                                │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                     containerd-shim                     ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Low-level Runtime                        │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                     runc / kata / gVisor                ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 整体架构
containerd 作为守护进程运行，通过 gRPC API 提供服务，使用 shim 进程管理容器生命周期，支持多种底层运行时（runc、kata 等）。

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| Content Store | 内容存储 | 存储镜像层和配置 |
| Snapshotter | 快照管理 | 管理容器文件系统 |
| Container | 容器管理 | 容器元数据和配置 |
| Task | 任务管理 | 容器进程管理 |
| Shim | 运行时适配 | 对接底层运行时 |

### 工作原理
1. 客户端通过 gRPC 调用 containerd API
2. containerd 从 registry 拉取镜像到 Content Store
3. Snapshotter 准备容器的根文件系统
4. 创建 Container 元数据
5. 启动 Task，由 Shim 调用底层运行时创建容器

---

## 使用场景

### 典型应用
- **Kubernetes 运行时**: 作为 kubelet 的 CRI 运行时
- **Docker 后端**: Docker Engine 的容器运行时
- **嵌入式运行时**: 嵌入到其他容器平台
- **边缘计算**: 轻量级边缘容器运行环境

### 适用条件
- 需要生产级容器运行时
- Kubernetes 容器运行时
- 需要与 OCI 标准兼容
- 需要可扩展的容器基础设施

### 不适用场景
- 需要完整 Docker CLI 体验
- 开发者本地开发环境

---

## 快速开始

### 安装部署
```bash
# Ubuntu/Debian
apt-get update
apt-get install containerd

# 二进制安装
wget https://github.com/containerd/containerd/releases/download/v1.7.0/containerd-1.7.0-linux-amd64.tar.gz
tar xvf containerd-1.7.0-linux-amd64.tar.gz -C /usr/local

# 配置 systemd
systemctl enable --now containerd
```

### 基础配置
```toml
# /etc/containerd/config.toml
version = 2

[plugins]
  [plugins."io.containerd.grpc.v1.cri"]
    sandbox_image = "registry.k8s.io/pause:3.9"
    [plugins."io.containerd.grpc.v1.cri".containerd]
      default_runtime_name = "runc"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes]
        [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
          runtime_type = "io.containerd.runc.v2"
          [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
            SystemdCgroup = true
```

### 验证测试
```bash
# 使用 ctr 客户端
ctr version

# 拉取镜像
ctr images pull docker.io/library/nginx:latest

# 运行容器
ctr run -d docker.io/library/nginx:latest nginx

# 查看容器
ctr containers list
ctr tasks list

# 使用 crictl (Kubernetes CRI)
crictl pull nginx
crictl images
```

---

## 最佳实践

### 生产环境建议
- 使用 systemd cgroup 驱动
- 配置镜像 registry 镜像
- 启用 metrics 监控
- 定期清理未使用的资源

### 性能优化
- 使用 overlayfs 快照驱动
- 配置合适的 shim 超时
- 优化镜像层缓存
- 配置并发拉取限制

### 安全加固
- 限制容器权限
- 配置 seccomp 和 AppArmor
- 使用用户命名空间
- 定期更新版本

---

## 生态集成

### 相关 CNCF 项目
- **Kubernetes**: CRI 容器运行时
- **runc**: 默认底层运行时
- **CRI-O**: 替代 CRI 实现
- **Harbor**: 镜像仓库

### 常见集成方案
- Kubernetes + containerd CRI
- containerd + runc/kata/gVisor
- containerd + Harbor 私有仓库
- nerdctl + containerd 本地开发

---

## 社区与支持

### 社区资源
- Slack: https://slack.containerd.io
- 邮件列表: containerd@lists.cncf.io
- GitHub Discussions

### 贡献指南
访问 https://github.com/containerd/containerd/blob/main/CONTRIBUTING.md 了解参与方式

---

## 参考资源

- [官方文档](https://containerd.io/docs)
- [GitHub Repo](https://github.com/containerd/containerd)
- [CNCF 项目页面](https://www.cncf.io/projects/containerd/)
- [CRI 插件文档](https://github.com/containerd/containerd/blob/main/docs/cri/config.md)

---

**维护者**: Kudig Team | **许可证**: MIT
