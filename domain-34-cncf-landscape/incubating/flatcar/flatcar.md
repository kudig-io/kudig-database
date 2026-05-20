---
title: Flatcar Container Linux
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- kubelet
- containerd
- docker
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Flatcar Container Linux 是什么
- 如何 Flatcar Container Linux
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Flatcar
- Container
- Linux
- cncf
- landscape
---

# Flatcar Container Linux

> **成熟度**: Incubating | **加入时间**: 2023-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://flatcar.org |
| **GitHub** | https://github.com/flatcar/Flatcar |
| **许可证** | Apache-2.0 |
| **主要语言** | Shell, Go |
| **CNCF 分类** | Provisioning & Container OS |

---

## 项目概述

Flatcar Container Linux 是为容器优化的不可变 Linux 发行版，是 CoreOS Container Linux 的延续和替代品。它提供最小化、自动更新、安全的容器运行环境。

## 核心特性

- **不可变基础设施**: 只读根文件系统，配置通过 Ignition/Cloud-Init
- **自动更新**: 内置 A/B 分区自动更新机制
- **最小化设计**: 只包含运行容器必需的组件
- **安全加固**: SELinux、只读 rootfs、自动安全补丁
- **多平台支持**: AWS、Azure、GCP、VMware、裸金属等
- **兼容性**: 完全兼容 CoreOS Container Linux

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                 Flatcar Container Linux                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    User Containers                          ││
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   ││
│  │  │ App Pod  │  │ App Pod  │  │ System   │  │ Logging  │   ││
│  │  │          │  │          │  │ Services │  │          │   ││
│  │  └──────────┘  └──────────┘  └──────────┘  └──────────┘   ││
│  └─────────────────────────────────────────────────────────────┘│
│                              │                                   │
│                    Container Runtime                             │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │             containerd / Docker                              ││
│  └─────────────────────────────────────────────────────────────┘│
│                              │                                   │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                   Read-Only Root FS                         ││
│  │  ┌───────────┐  ┌───────────┐  ┌───────────────────────┐  ││
│  │  │  systemd  │  │   Linux   │  │   Core Utilities      │  ││
│  │  │           │  │  Kernel   │  │   (bash, coreutils)   │  ││
│  │  └───────────┘  └───────────┘  └───────────────────────┘  ││
│  └─────────────────────────────────────────────────────────────┘│
│                              │                                   │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │              A/B Update Partitions                          ││
│  │  ┌─────────────────┐    ┌─────────────────────────────┐    ││
│  │  │  Partition A    │    │     Partition B             │    ││
│  │  │  (Active)       │    │     (Update Target)         │    ││
│  │  └─────────────────┘    └─────────────────────────────┘    ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### AWS 部署

```bash
# 获取最新 AMI
aws ec2 describe-images \
  --filters "Name=name,Values=Flatcar-stable-*" \
  --owners "075585003325"

# 启动实例
aws ec2 run-instances \
  --image-id ami-xxxxxxxxx \
  --instance-type t3.medium \
  --key-name my-key \
  --user-data file://ignition.json
```

### Ignition 配置

```json
{
  "ignition": { "version": "3.3.0" },
  "passwd": {
    "users": [{
      "name": "core",
      "sshAuthorizedKeys": ["ssh-rsa AAAA..."]
    }]
  },
  "storage": {
    "files": [{
      "path": "/etc/hostname",
      "contents": { "source": "data:,flatcar-node-1" },
      "mode": 420
    }]
  },
  "systemd": {
    "units": [{
      "name": "docker.service",
      "enabled": true
    }]
  }
}
```

### Terraform 部署

```hcl
resource "aws_instance" "flatcar" {
  ami           = data.aws_ami.flatcar.id
  instance_type = "t3.medium"
  
  user_data = file("${path.module}/ignition.json")
  
  tags = {
    Name = "flatcar-node"
  }
}

data "aws_ami" "flatcar" {
  most_recent = true
  owners      = ["075585003325"]
  
  filter {
    name   = "name"
    values = ["Flatcar-stable-*"]
  }
}
```

---

## 自动更新

```yaml
# 更新配置 (/etc/flatcar/update.conf)
GROUP=stable
SERVER=https://public.update.flatcar-linux.net/v1/update/
REBOOT_STRATEGY=etcd-lock
LOCKSMITHD_REBOOT_WINDOW_START=02:00
LOCKSMITHD_REBOOT_WINDOW_LENGTH=1h
```

### 更新通道

| 通道 | 说明 |
|------|------|
| stable | 生产环境推荐 |
| beta | 测试新特性 |
| alpha | 最新特性，可能不稳定 |
| lts | 长期支持版本 |

### 控制更新

```bash
# 暂停更新
sudo systemctl stop update-engine
sudo systemctl mask update-engine

# 手动触发更新
update_engine_client -check_for_update

# 查看更新状态
update_engine_client -status
```

---

## Kubernetes 集群

```yaml
# 使用 kubeadm + Flatcar
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
---
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.28.0
networking:
  podSubnet: "10.244.0.0/16"
```

### Ignition 配置 Kubernetes

```json
{
  "ignition": { "version": "3.3.0" },
  "systemd": {
    "units": [
      {
        "name": "kubelet.service",
        "enabled": true,
        "contents": "[Unit]\nDescription=Kubelet\n[Service]\nExecStart=/opt/bin/kubelet\n[Install]\nWantedBy=multi-user.target"
      }
    ]
  },
  "storage": {
    "files": [
      {
        "path": "/opt/bin/kubelet",
        "source": "https://storage.googleapis.com/kubernetes-release/release/v1.28.0/bin/linux/amd64/kubelet",
        "mode": 493
      }
    ]
  }
}
```

---

## 安全特性

```bash
# SELinux 状态
getenforce

# 只读根文件系统
mount | grep "/ "

# 安全更新日志
journalctl -u update-engine
```

---

## 最佳实践

1. **Ignition 配置**: 使用 Ignition 实现声明式配置
2. **自动更新**: 配置更新窗口避免业务高峰
3. **协调更新**: 使用 locksmith 协调集群节点更新
4. **监控**: 监控更新状态和系统健康
5. **LTS 版本**: 生产环境考虑使用 LTS 通道

---

## 参考资源

- [官方文档](https://flatcar.org/docs)
- [GitHub Repo](https://github.com/flatcar/Flatcar)
- [Ignition 规范](https://coreos.github.io/ignition/)
- [迁移指南](https://flatcar.org/docs/latest/migrating-from-coreos/)

---

**维护者**: Kudig Team | **许可证**: MIT
