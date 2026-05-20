---
title: CRI-O
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- kubelet
- prometheus
- containerd
- cri-o
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
- CRI-O 是什么
- 如何 CRI-O
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- CRI-O
- cncf
- landscape
---


# CRI-O

> **成熟度**: Graduated | **加入时间**: 2019-04 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://cri-o.io |
| **GitHub** | https://github.com/cri-o/cri-o |
| **文档** | https://cri-o.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Container Runtime |

---

## 项目概述

### 简介
CRI-O 是专为 Kubernetes 设计的轻量级容器运行时，实现了 Kubernetes Container Runtime Interface (CRI)。它专注于在 Kubernetes 环境中运行 OCI 兼容的容器，不包含额外功能。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2016-09 | 由 Red Hat 发起 (原名 OCID) |
| 2017 | 更名为 CRI-O |
| 2019-04 | 加入 CNCF Incubating |
| 2021-08 | 晋升为 CNCF Graduated |

### 核心定位
CRI-O 是 Kubernetes 的专用容器运行时，只包含 K8s 所需的最小功能集。它是 Red Hat OpenShift 和多个 Kubernetes 发行版的默认运行时，比 Docker 更轻量和安全。

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      Kubernetes 集群                             │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                        kubelet                               ││
│  │                           │                                  ││
│  │                           │ CRI gRPC                         ││
│  │                           ▼                                  ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                        CRI-O                                 ││
│  │  ┌───────────────────────────────────────────────────────┐  ││
│  │  │                CRI-O Daemon                            │  ││
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │  ││
│  │  │  │ Image Mgmt  │  │ Container   │  │  Sandbox    │    │  ││
│  │  │  │ (镜像管理)  │  │ Lifecycle   │  │  (Pod管理)  │    │  ││
│  │  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘    │  ││
│  │  └─────────┼────────────────┼────────────────┼───────────┘  ││
│  │            │                │                │               ││
│  │            ▼                ▼                ▼               ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐          ││
│  │  │ containers/ │  │ conmon      │  │   CNI       │          ││
│  │  │ image       │  │ (监控进程)  │  │  (网络)     │          ││
│  │  └─────────────┘  └──────┬──────┘  └─────────────┘          ││
│  │                          │                                   ││
│  │                          ▼                                   ││
│  │                 ┌─────────────────┐                          ││
│  │                 │  OCI Runtime    │                          ││
│  │                 │ (runc/crun/kata)│                          ││
│  │                 └─────────────────┘                          ││
│  └─────────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 功能 | 说明 |
|:---|:---|:---|
| **CRI-O Daemon** | 服务主进程 | 处理 CRI 请求 |
| **conmon** | 容器监控 | 监控容器进程、处理日志 |
| **containers/image** | 镜像管理 | 拉取、存储 OCI 镜像 |
| **containers/storage** | 存储管理 | 容器和镜像存储 |
| **CNI Plugins** | 网络配置 | Pod 网络设置 |
| **OCI Runtime** | 容器运行 | runc、crun、Kata 等 |

---

## 与 containerd 对比

```
┌─────────────────────────────────────────────────────────────────┐
│                  容器运行时对比                                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  特性              CRI-O                 containerd             │
│  ─────────────────────────────────────────────────────────────  │
│  设计目标          仅 Kubernetes         通用容器运行时          │
│  镜像构建          不支持                不支持 (需 buildkit)    │
│  Docker 兼容       不需要                通过 shim 兼容          │
│  代码量            ~30K LOC              ~100K LOC              │
│  内存占用          较低                  中等                    │
│  OpenShift 支持    默认                  可选                    │
│  GKE/EKS 支持      可选                  默认                    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 安装配置

### 安装方式

```bash
# RHEL/CentOS/Fedora
dnf module enable cri-o:1.28
dnf install cri-o

# Ubuntu/Debian
echo "deb https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/xUbuntu_22.04/ /" \
  | tee /etc/apt/sources.list.d/devel:kubic:libcontainers:stable.list
apt-get update && apt-get install cri-o cri-o-runc

# 启动服务
systemctl enable --now crio
```

### 配置文件

```toml
# /etc/crio/crio.conf
[crio]

# 运行时配置
[crio.runtime]
default_runtime = "runc"
conmon = "/usr/bin/conmon"
conmon_cgroup = "pod"

# 支持多个 OCI 运行时
[crio.runtime.runtimes.runc]
runtime_path = "/usr/bin/runc"
runtime_type = "oci"

[crio.runtime.runtimes.crun]
runtime_path = "/usr/bin/crun"
runtime_type = "oci"

# Kata Containers (安全容器)
[crio.runtime.runtimes.kata]
runtime_path = "/usr/bin/kata-runtime"
runtime_type = "vm"
privileged_without_host_devices = true

# 镜像配置
[crio.image]
pause_image = "registry.k8s.io/pause:3.9"
pause_image_auth_file = ""
pause_command = "/pause"

# 网络配置
[crio.network]
network_dir = "/etc/cni/net.d/"
plugin_dirs = ["/opt/cni/bin/", "/usr/libexec/cni/"]
```

### Kubernetes 配置

```yaml
# kubelet 配置使用 CRI-O
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
containerRuntimeEndpoint: "unix:///var/run/crio/crio.sock"
```

---

## 核心功能

### 1. Pod 和容器管理

```bash
# 使用 crictl 与 CRI-O 交互
# 列出 Pods
crictl pods

# 列出容器
crictl ps

# 查看容器日志
crictl logs <container-id>

# 在容器中执行命令
crictl exec -it <container-id> /bin/sh

# 查看镜像
crictl images

# 拉取镜像
crictl pull nginx:latest
```

### 2. 安全特性

```toml
# 安全配置示例
[crio.runtime]
# 默认 seccomp 配置
seccomp_profile = "/etc/crio/seccomp.json"

# SELinux 支持
selinux = true

# 只读根文件系统
read_only = false

# 禁止特权容器 (可选)
# [crio.runtime.runtimes.runc]
# allowed_annotations = []
```

```yaml
# Pod 安全配置示例
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
spec:
  securityContext:
    runAsNonRoot: true
    seccompProfile:
      type: RuntimeDefault
  containers:
    - name: app
      image: nginx
      securityContext:
        allowPrivilegeEscalation: false
        capabilities:
          drop: ["ALL"]
        readOnlyRootFilesystem: true
```

### 3. 镜像签名验证

```yaml
# /etc/containers/policy.json
{
  "default": [{"type": "reject"}],
  "transports": {
    "docker": {
      "registry.example.com": [
        {
          "type": "signedBy",
          "keyType": "GPGKeys",
          "keyPath": "/etc/pki/rpm-gpg/RPM-GPG-KEY-example"
        }
      ],
      "registry.k8s.io": [{"type": "insecureAcceptAnything"}]
    }
  }
}
```

### 4. 多运行时支持

```yaml
# Kubernetes RuntimeClass 示例
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata-containers
handler: kata

---
apiVersion: v1
kind: Pod
metadata:
  name: kata-pod
spec:
  runtimeClassName: kata-containers
  containers:
    - name: nginx
      image: nginx
```

---

## 监控和调试

### 指标收集

```bash
# CRI-O 暴露 Prometheus 指标
curl http://localhost:9090/metrics

# 关键指标
# crio_operations_total - 操作计数
# crio_operations_latency_seconds - 操作延迟
# crio_image_pulls_by_digest_total - 镜像拉取
# crio_containers_oom_total - OOM 事件
```

### 日志和调试

```bash
# 查看 CRI-O 日志
journalctl -u crio -f

# 调试模式启动
crio --log-level debug

# 检查 CRI-O 状态
crictl info

# 检查 Pod 详情
crictl inspectp <pod-id>
```

---

## 使用场景

### 1. OpenShift 默认运行时
```yaml
# OpenShift 默认使用 CRI-O
# 无需额外配置
oc get nodes -o wide
# CONTAINER-RUNTIME 列显示 cri-o://x.x.x
```

### 2. 安全敏感环境
```toml
# 启用全面安全配置
[crio.runtime]
seccomp_profile = "/etc/crio/seccomp.json"
selinux = true

[crio.runtime.runtimes.kata]
runtime_path = "/usr/bin/kata-runtime"
runtime_type = "vm"
```

### 3. 资源受限环境
```toml
# 最小化资源配置
[crio.runtime]
pids_limit = 1024
log_size_max = 52428800  # 50MB
```

---

## 生态集成

| 项目 | 集成方式 |
|:---|:---|
| **Kubernetes** | CRI 原生支持 |
| **OpenShift** | 默认运行时 |
| **CNI** | 网络插件 |
| **Kata Containers** | 安全容器运行时 |
| **gVisor** | 沙箱运行时 |
| **Prometheus** | 指标监控 |

---

## 参考资源

- [官方文档](https://cri-o.io/docs)
- [GitHub Repo](https://github.com/cri-o/cri-o)
- [CNCF 项目页面](https://www.cncf.io/projects/cri-o/)
- [CRI 规范](https://github.com/kubernetes/cri-api)
- [Red Hat 文档](https://access.redhat.com/documentation/en-us/openshift_container_platform/latest/html/architecture/cri-o)

---

**维护者**: Kudig Team | **许可证**: MIT
