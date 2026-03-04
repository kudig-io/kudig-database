# Kairos

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kairos.io/ |
| **GitHub** | https://github.com/kairos-io/kairos |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Kairos 是一个不可变 Linux 元发行版框架，专注于将任何 Linux 发行版转化为不可变的、基于容器镜像的操作系统，特别适用于边缘计算和 Kubernetes 节点的自动化部署。它支持通过 cloud-init 风格的 YAML 配置实现零接触安装（Zero-Touch Provisioning），内置 P2P 网络自动组建 Kubernetes 集群的能力。

### 核心特性

- **不可变 OS**: 基于容器镜像的不可变操作系统，原子升级和回滚
- **多发行版**: 支持将 Ubuntu、Fedora、openSUSE、Alpine 等转化为 Kairos
- **零接触安装**: 通过 QR 码或 cloud-config 实现自动化安装和配置
- **P2P 组网**: 内置 P2P 网络功能，边缘节点自动发现和组建集群
- **K3s/K0s 集成**: 原生支持在安装时自动部署 K3s 或 K0s 集群
- **A/B 分区**: 双分区方案确保升级失败可自动回滚

---

## 架构设计

```
┌─────────────────────────────────────────────┐
│            Kairos 部署流程                    │
│                                              │
│  ┌──────────┐    ┌──────────────────┐       │
│  │基础发行版 │ +  │ Kairos Framework │       │
│  │(Ubuntu等)│    │ (不可变层/升级器) │       │
│  └────┬─────┘    └────────┬─────────┘       │
│       └──────────┬────────┘                  │
│           ┌──────▼──────┐                    │
│           │ OCI 镜像     │                    │
│           │(可启动 OS)   │                    │
│           └──────┬──────┘                    │
│                  │                            │
│  ┌───────────────▼────────────────┐         │
│  │      安装方式                   │         │
│  │  ┌─────┐ ┌─────┐ ┌─────────┐ │         │
│  │  │ ISO │ │ PXE │ │ QR Code │ │         │
│  │  └──┬──┘ └──┬──┘ └────┬────┘ │         │
│  └─────┼───────┼─────────┼──────┘         │
│        └───────┼─────────┘                  │
│         ┌──────▼───────┐                    │
│         │ cloud-config │                    │
│         │  (YAML 配置) │                    │
│         └──────┬───────┘                    │
│         ┌──────▼──────────────────┐         │
│         │  Target Node            │         │
│         │  ┌────────────────────┐ │         │
│         │  │ A/B 分区 (不可变)  │ │         │
│         │  │ K3s/K0s 集群      │ │         │
│         │  │ P2P 网络          │ │         │
│         │  └────────────────────┘ │         │
│         └─────────────────────────┘         │
└─────────────────────────────────────────────┘
```

---

## 快速开始

### 配置文件

```yaml
# cloud-config.yaml
#cloud-config

install:
  auto: true
  device: /dev/sda
  reboot: true

hostname: edge-node-01

users:
  - name: admin
    passwd: admin
    ssh_authorized_keys:
      - ssh-rsa AAAA...

k3s:
  enabled: true
  args:
    - --disable=traefik
  env:
    K3S_TOKEN: "my-cluster-token"

# P2P 自动组网
p2p:
  network_token: "auto"
  dns: true
  # 第一个节点自动成为 master
  auto:
    enable: true
    ha:
      enable: true
      master_nodes: 3

stages:
  after-install:
    - name: "Install monitoring"
      commands:
        - kubectl apply -f https://raw.githubusercontent.com/prometheus/prometheus/main/manifests/prometheus.yaml
```

### 构建自定义镜像

```dockerfile
FROM quay.io/kairos/ubuntu:24.04-standard

# 安装额外软件包
RUN apt-get update && apt-get install -y \
    wireguard-tools \
    htop \
    && rm -rf /var/lib/apt/lists/*

# 添加自定义配置
COPY custom-config.yaml /system/oem/
```

```bash
# 构建并推送
docker build -t myorg/my-kairos:latest .
docker push myorg/my-kairos:latest
```

### 生成安装介质

```bash
# 生成 ISO
docker run -v $PWD:/output \
  quay.io/kairos/osbuilder-tools:latest \
  build-iso --date=false \
  --cloud-config cloud-config.yaml \
  myorg/my-kairos:latest

# 生成 RAW 磁盘镜像
docker run -v $PWD:/output \
  quay.io/kairos/osbuilder-tools:latest \
  build-disk --cloud-config cloud-config.yaml \
  myorg/my-kairos:latest
```

### 系统升级

```bash
# 升级到新镜像版本
kairos-agent upgrade --image myorg/my-kairos:v2.0

# 查看当前系统状态
kairos-agent state

# 回滚到上一版本
kairos-agent reset
```

---

## 与其他方案对比

| 特性 | Kairos | Talos | Flatcar | bootc |
|:---|:---|:---|:---|:---|
| 基础发行版 | 多发行版可选 | 自研 | Gentoo | Fedora/CentOS |
| 不可变性 | A/B 分区 | 完全不可变 | 双分区 | ostree |
| K8s 集成 | K3s/K0s | Talos API | 手动 | 手动 |
| P2P 组网 | 内置 | 不支持 | 不支持 | 不支持 |
| 零接触安装 | QR/cloud-config | API 驱动 | Ignition | cloud-init |
| 适用场景 | 边缘/IoT/通用 | K8s 专用 | 容器主机 | 通用服务器 |

---

## 最佳实践

1. **镜像精简**: 自定义 Kairos 镜像时只安装必要的包，减小攻击面
2. **P2P 令牌安全**: P2P 网络令牌需要安全存储和分发
3. **升级策略**: 使用蓝绿升级策略，先升级部分节点验证后再全量升级
4. **配置管理**: 将 cloud-config 纳入版本控制，确保配置可追溯
5. **离线部署**: 边缘场景预先下载 K3s 二进制和镜像到 OCI 镜像中

---

## 参考资源

- [Kairos 官方文档](https://kairos.io/docs/)
- [Kairos GitHub](https://github.com/kairos-io/kairos)
- [Kairos 社区](https://github.com/kairos-io/kairos/discussions)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
