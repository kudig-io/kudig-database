# Tinkerbell

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://tinkerbell.org/ |
| **GitHub** | https://github.com/tinkerbell |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Tinkerbell 是一个裸金属服务器自动化配置（provisioning）框架，用于在物理服务器上自动安装操作系统和执行配置任务。它替代传统的 PXE/Cobbler 方案，通过声明式的工作流定义和容器化的操作步骤实现裸金属服务器的云原生式管理。

### 核心特性

- **声明式工作流**: 使用 YAML 定义服务器配置流程
- **容器化操作**: 每个配置步骤在 Docker 容器中执行
- **硬件发现**: 自动发现和注册裸金属服务器
- **DHCP/PXE/iPXE**: 内置网络引导服务
- **模板化配置**: 支持参数化的操作系统安装模板
- **Kubernetes 集成**: 通过 CRD 管理硬件和工作流

---

## 架构设计

```
┌───────────────────────────────────────────────┐
│              Tinkerbell Stack                   │
│                                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────────┐│
│  │  Smee    │  │  Hegel   │  │   Tink       ││
│  │ (DHCP/   │  │(Metadata)│  │  Server      ││
│  │  iPXE)   │  │          │  │ (Workflows)  ││
│  └────┬─────┘  └────┬─────┘  └──────┬───────┘│
│       │              │               │         │
│  ┌────┴──────────────┴───────────────┴──────┐ │
│  │           Kubernetes API / CRDs           │ │
│  │  Hardware │ Template │ Workflow           │ │
│  └───────────────────────────────────────────┘ │
└───────────────────┬───────────────────────────┘
                    │ PXE Boot / iPXE
                    ▼
            ┌──────────────┐
            │ Bare Metal   │
            │ Server       │
            │ (Tink Worker)│
            └──────────────┘
```

### 核心组件

| 组件 | 说明 |
|:---|:---|
| **Smee** | DHCP/iPXE 服务，引导裸金属服务器网络启动 |
| **Hegel** | 元数据服务，为工作节点提供配置信息 |
| **Tink Server** | 工作流引擎，管理 Hardware/Template/Workflow CRD |
| **Tink Worker** | 在目标服务器上执行工作流步骤的代理 |
| **Hook** | iPXE 引导的最小 Linux 环境 (LinuxKit) |

---

## 快速开始

### 安装

```bash
# 使用 Helm 安装
helm repo add tinkerbell https://tinkerbell.github.io/charts
helm install tinkerbell tinkerbell/tinkerbell-stack \
  --namespace tinkerbell \
  --create-namespace \
  --set smee.publicIP=192.168.1.1 \
  --set smee.dhcp.enabled=true
```

### 注册硬件

```yaml
apiVersion: tinkerbell.org/v1alpha1
kind: Hardware
metadata:
  name: server-01
spec:
  disks:
    - device: /dev/sda
  metadata:
    facility:
      facility_code: onprem
    instance:
      hostname: server-01
      operatingSystem:
        distro: ubuntu
        version: "22.04"
  interfaces:
    - dhcp:
        mac: "00:11:22:33:44:55"
        ip:
          address: 192.168.1.100
          netmask: 255.255.255.0
          gateway: 192.168.1.1
        hostname: server-01
```

### 定义模板

```yaml
apiVersion: tinkerbell.org/v1alpha1
kind: Template
metadata:
  name: ubuntu-install
spec:
  data: |
    version: "0.1"
    name: ubuntu-install
    global_timeout: 6000
    tasks:
      - name: "os-installation"
        worker: "{{.device_1}}"
        volumes:
          - /dev:/dev
          - /dev/console:/dev/console
          - /lib/firmware:/lib/firmware:ro
        actions:
          - name: "disk-wipe"
            image: quay.io/tinkerbell-actions/disk-wipe:v1.0.0
            timeout: 90
            environment:
              MIRROR_HOST: 192.168.1.1
          - name: "disk-partition"
            image: quay.io/tinkerbell-actions/disk-partition:v1.0.0
            timeout: 600
            environment:
              MIRROR_HOST: 192.168.1.1
              DEST_DISK: /dev/sda
              PARTITION_LAYOUT: '[{"size":512,"type":"ef00"},{"size":0,"type":"8300"}]'
          - name: "install-rootfs"
            image: quay.io/tinkerbell-actions/image2disk:v1.0.0
            timeout: 600
            environment:
              DEST_DISK: /dev/sda2
              IMG_URL: "http://192.168.1.1/ubuntu-22.04.img.gz"
              COMPRESSED: true
          - name: "install-grub"
            image: quay.io/tinkerbell-actions/grub-install:v1.0.0
            timeout: 600
            volumes:
              - /statedir:/statedir
```

### 创建工作流

```yaml
apiVersion: tinkerbell.org/v1alpha1
kind: Workflow
metadata:
  name: install-server-01
spec:
  templateRef: ubuntu-install
  hardwareRef: server-01
  hardwareMap:
    device_1: "00:11:22:33:44:55"
```

---

## 最佳实践

1. **网络规划**: 确保 DHCP/PXE 网络与生产网络适当隔离
2. **镜像缓存**: 在本地缓存操作系统镜像加速安装
3. **模板复用**: 创建标准化的安装模板，参数化可变部分
4. **Action 容器**: 使用官方 Action 容器，需要时自定义扩展
5. **硬件清单**: 维护准确的硬件清单（MAC 地址、磁盘信息）

---

## 参考资源

- [Tinkerbell 官方文档](https://tinkerbell.org/docs/)
- [Tinkerbell GitHub](https://github.com/tinkerbell)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
