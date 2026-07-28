---
title: 32 - kubeadm 集群生命周期管理 (Cluster Lifecycle with kubeadm)
description: '# 32 - kubeadm 集群生命周期管理 (Cluster Lifecycle with kubeadm)'
summary: '4. [集群配置管理 (kubeadm config)](#4-集群配置管理-kubeadm-config)'
category: control-plane
tags:
- k8s
- control-plane
- etcd
- apiserver
- scheduler
- controller-manager
- kubelet
- cilium
- flannel
- calico
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 10min
intent_queries:
- kubeadm 集群生命周期管理 (Cluster Lifecycle with kubeadm) 是什么
- 如何 kubeadm 集群生命周期管理 (Cluster Lifecycle with kubeadm)
- Kubernetes 3 control plane 最佳实践
trigger_keywords:
- kubeadm
- 集群生命周期管理
- Cluster
- Lifecycle
- with
- kubeadm
- control
- plane
prerequisites:
- kubectl-basics
- kubernetes-concepts
- helm-basics
- cilium-basics
- cni-basics
- etcd-basics
- gpu-scheduling-basics
- tls-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../工作负载/
  label: '相关知识域: 工作负载'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../存储/
  label: '相关知识域: 存储'
- type: domain
  path: ../安全/
  label: '相关知识域: 安全'
- type: fta
  path: ../故障诊断/FTA故障树/list/kubeadm-fta.md
  label: '故障树: kubeadm'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 32 - kubeadm 集群生命周期管理 (Cluster Lifecycle with kubeadm)

> **适用版本**: [[kubernetes|Kubernetes]] v1.25 - v1.32+ | **最后更新**: 2026-04 | **文档类型**: 运维操作指南

---

<!-- chunk: 目录 -->
## 目录

1. [kubeadm 架构概述](#1-kubeadm-架构概述)
2. [集群初始化 (kubeadm init)](#2-集群初始化-kubeadm-init)
3. [节点加入 (kubeadm join)](#3-节点加入-kubeadm-join)
4. [集群配置管理 (kubeadm config)](#4-集群配置管理-kubeadm-config)
5. [证书管理 (kubeadm certs)](#5-证书管理-kubeadm-certs)
6. [令牌管理 (kubeadm token)](#6-令牌管理-kubeadm-token)
7. [集群升级 (kubeadm upgrade)](#7-集群升级-kubeadm-upgrade)
8. [节点重置与清理 (kubeadm reset)](#8-节点重置与清理-kubeadm-reset)
9. [高可用集群管理](#9-高可用集群管理)
10. [故障排查](#10-故障排查)
11. [生产环境 Checklist](#11-生产环境-checklist)

---

<!-- chunk: 1. kubeadm 架构概述 -->
## 1. kubeadm 架构概述

### 1.1 kubeadm 核心职责

kubeadm 是 Kubernetes 官方提供的集群生命周期管理工具，遵循 **"做好一件事"** 的 Unix 哲学，专注于集群的引导 (bootstrapping) 和基本管理。

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                        kubeadm Architecture & Scope                              │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                        kubeadm Core Commands                               │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │    │
│  │  │   init      │ │   join      │ │   upgrade   │ │   reset     │       │    │
│  │  │  (初始化)   │ │  (加入)     │ │  (升级)     │ │  (重置)     │       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐       │    │
│  │  │   config    │ │   token     │ │   certs     │ │   version   │       │    │
│  │  │  (配置)     │ │  (令牌)     │ │  (证书)     │ │  (版本)     │       │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘       │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
│  ┌─────────────────────────────────────────────────────────────────────────┐    │
│  │                    kubeadm Design Philosophy                               │    │
│  │  ┌─────────────────────────────────────────────────────────────────┐    │    │
│  │  │ 1. 只做引导 (Bootstrapping)                                      │    │    │
│  │  │    - 安装控制平面组件                                           │    │    │
│  │  │    - 生成必要证书和配置                                         │    │    │
│  │  │    - 不管理 CNI / Ingress / 存储等                              │    │    │
│  │  │                                                                  │    │    │
│  │  │ 2. 可组合性 (Composability)                                      │    │    │
│  │  │    - 每个阶段可独立执行                                         │    │    │
│  │  │    - 支持自定义配置覆盖                                         │    │    │
│  │  │                                                                  │    │    │
│  │  │ 3. 可移植性 (Portability)                                        │    │    │
│  │  │    - 跨 Linux 发行版支持                                        │    │    │
│  │  │    - 支持多种容器运行时                                         │    │    │
│  │  │                                                                  │    │    │
│  │  │ 4. 安全性 (Security by Default)                                  │    │    │
│  │  │    - 默认启用 RBAC                                              │    │    │
│  │  │    - 自动生成 TLS 证书                                          │    │    │
│  │  │    - 安全引导令牌机制                                           │    │    │
│  │  └─────────────────────────────────────────────────────────────────┘    │    │
│  └─────────────────────────────────────────────────────────────────────────┘    │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 kubeadm 工作流程

```
kubeadm 集群创建完整流程:

Phase 1: 预检查 (preflight)
├── 检查系统要求 (CPU/内存/磁盘)
├── 检查网络连通性
├── 检查容器运行时
├── 检查端口占用
└── 检查内核模块 (overlay, bridge-nf-call-iptables)

Phase 2: 证书生成 (certs)
├── 生成 CA 证书 (如果缺失)
├── 生成 API Server 证书
├── 生成 etcd 证书
├── 生成 front-proxy 证书
└── 生成 SA (ServiceAccount) 密钥对

Phase 3: 控制平面配置 (control-plane)
├── 生成静态 Pod manifests (/etc/kubernetes/manifests/)
├── kube-apiserver.yaml
├── kube-controller-manager.yaml
├── kube-scheduler.yaml
└── 可选: etcd.yaml (stacked 模式)

Phase 4: 数据平面配置 (kubelet)
├── 生成 kubelet 配置
├── 生成 kubeconfig 文件
└── 启动 kubelet 服务

Phase 5: 附加组件 (addon)
├── CoreDNS 部署
└── kube-proxy 部署
```

### 1.3 版本兼容性说明

| kubeadm 版本 | 支持的 Kubernetes 版本 | 最低 etcd 版本 | 最低 [[containerd|containerd]] 版本 |
|-------------|----------------------|---------------|-------------------|
| v1.25.x | v1.25 | 3.5.4 | 1.6.0 |
| v1.26.x | v1.26 | 3.5.6 | 1.6.0 |
| v1.27.x | v1.27 | 3.5.7 | 1.6.0 |
| v1.28.x | v1.28 | 3.5.9 | 1.6.0 |
| v1.29.x | v1.29 | 3.5.10 | 1.7.0 |
| v1.30.x | v1.30 | 3.5.12 | 1.7.0 |
| v1.31.x | v1.31 | 3.5.15 | 1.7.0 |
| v1.32.x | v1.32 | 3.5.16 | 1.7.0 |

---

<!-- chunk: 2. 集群初始化 (kubeadm init) -->
## 2. 集群初始化 (kubeadm init)

### 2.1 初始化前检查清单

#### 系统环境要求

| 检查项 | 要求 | 验证命令 | 不满足时的影响 |
|--------|------|----------|--------------|
| **操作系统** | Linux (Ubuntu 20.04+/CentOS 7+/RHEL 8+) | `cat /etc/os-release` | 可能无法安装依赖包 |
| **CPU** | 控制平面 >= 2核, Worker >= 1核 | `nproc` | 调度失败 |
| **内存** | 控制平面 >= 2GB, Worker >= 1GB | `free -h` | OOM Kill |
| **磁盘** | 控制平面 >= 20GB, Worker >= 10GB | `df -h` | 镜像拉取失败 |
| **内核版本** | >= 3.10 (推荐 >= 5.4) | `uname -r` | 部分功能不可用 |
| **容器运行时** | containerd / CRI-O / docker (已弃用) | `crictl version` | Pod 无法创建 |
| **网络** | 所有节点互通, 关闭 swap | `ping <node-ip>` | 节点无法加入 |

#### 端口要求

| 组件 | 端口 | 协议 | 方向 | 说明 |
|------|------|------|------|------|
| **API Server** | 6443 | TCP | Inbound | [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 入口 |
| **etcd** | 2379-2380 | TCP | Inbound | etcd 客户端/对等通信 |
| **kubelet** | 10250 | TCP | Inbound | Kubelet API |
| **Scheduler** | 10259 | TCP | Inbound | 调度器度量端点 |
| **Controller** | 10257 | TCP | Inbound | 控制器度量端点 |
| **kube-proxy** | 10256 | TCP | Inbound | kube-proxy 健康检查 |

#### 初始化前系统配置脚本

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
#!/bin/bash
# kubeadm 初始化前系统配置脚本
# 适用于 Ubuntu 20.04/22.04, RHEL 8/9, CentOS 7/8

set -euo pipefail

# 1. 关闭 Swap
swapoff -a
sed -i '/swap/d' /etc/fstab

# 2. 加载必要的内核模块
cat > /etc/modules-load.d/k8s.conf <<MODEOF
overlay
br_netfilter
MODEOF

modprobe overlay
modprobe br_netfilter

# 3. 设置 sysctl 参数
cat > /etc/sysctl.d/k8s.conf <<SYSEOF
net.bridge.bridge-nf-call-iptables  = 1
net.bridge.bridge-nf-call-ip6tables = 1
net.ipv4.ip_forward                 = 1
SYSEOF

sysctl --system

# 4. 安装 containerd (如未安装)
if ! command -v containerd &> /dev/null; then
    echo "Installing containerd..."
    apt-get update
    apt-get install -y containerd
    mkdir -p /etc/containerd
    containerd config default | tee /etc/containerd/config.toml
    sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml
    systemctl restart containerd
    systemctl enable containerd
fi

# 5. 配置 crictl
cat > /etc/crictl.yaml <<CRIEOF
runtime-endpoint: unix:///run/containerd/containerd.sock
image-endpoint: unix:///run/containerd/containerd.sock
timeout: 10
debug: false
CRIEOF

# 6. 安装 kubeadm, kubelet, kubectl
KUBE_VERSION="1.32.0-1.1"
apt-get update
apt-get install -y apt-transport-https ca-certificates curl gpg

mkdir -p -m 755 /etc/apt/keyrings
curl -fsSL https://pkgs.k8s.io/core:/stable:/v1.32/deb/Release.key | \
    gpg --dearmor -o /etc/apt/keyrings/kubernetes-apt-keyring.gpg

echo 'deb [signed-by=/etc/apt/keyrings/kubernetes-apt-keyring.gpg] \
    https://pkgs.k8s.io/core:/stable:/v1.32/deb/ /' | \
    tee /etc/apt/sources.list.d/kubernetes.list

apt-get update
apt-get install -y kubelet="${KUBE_VERSION}" kubeadm="${KUBE_VERSION}" kubectl="${KUBE_VERSION}"
apt-mark hold kubelet kubeadm kubectl

systemctl enable --now kubelet

echo "=== Pre-flight configuration completed ==="
```
---

### 2.2 完整配置示例 (kubeadm Config v1beta3/v1beta4)

kubeadm 使用 Configuration API 进行声明式配置。v1beta4 在 v1.31 中引入，v1beta3 仍然支持。

#### v1beta4 完整配置示例

```yaml
# kubeadm-config-v1beta4.yaml
apiVersion: kubeadm.k8s.io/v1beta4
kind: InitConfiguration
bootstrapTokens:
- token: "9a08jv.c0izixklcxtmnze7"
  description: "kubeadm bootstrap token"
  ttl: "24h"
  usages:
  - authentication
  - signing
  groups:
  - system:bootstrappers:kubeadm:default-node-token
localAPIEndpoint:
  advertiseAddress: "192.168.1.10"
  bindPort: 6443
nodeRegistration:
  name: "control-plane-1"
  criSocket: "unix:///run/containerd/containerd.sock"
  taints:
  - key: "node-role.kubernetes.io/control-plane"
    value: ""
    effect: "NoSchedule"
  kubeletExtraArgs:
    - name: "node-ip"
      value: "192.168.1.10"
    - name: "pod-infra-container-image"
      value: "registry.k8s.io/pause:3.10"
timeout:
  controlPlaneComponentHealthCheck: "4m"
  etcdAPICall: "2m"
  kubernetesAPICall: "1m"
  tlsBootstrap: "5m"
  upgradeManifests: "5m"
---
apiVersion: kubeadm.k8s.io/v1beta4
kind: ClusterConfiguration
clusterName: "production-cluster"
kubernetesVersion: "v1.32.0"
controlPlaneEndpoint: "192.168.1.100:6443"
networking:
  serviceSubnet: "10.96.0.0/12"
  podSubnet: "10.244.0.0/16"
  dnsDomain: "cluster.local"
apiServer:
  timeoutForControlPlane: "4m"
  extraArgs:
    - name: "authorization-mode"
      value: "Node,RBAC"
    - name: "audit-log-path"
      value: "/var/log/kubernetes/audit.log"
    - name: "audit-log-maxage"
      value: "30"
    - name: "audit-log-maxbackup"
      value: "10"
    - name: "audit-log-maxsize"
      value: "100"
    - name: "request-timeout"
      value: "300s"
    - name: "service-node-port-range"
      value: "30000-32767"
    - name: "enable-admission-plugins"
      value: "NodeRestriction,LimitRanger,ServiceAccount,DefaultStorageClass,ResourceQuota"
  certSANs:
    - "192.168.1.10"
    - "192.168.1.11"
    - "192.168.1.12"
    - "192.168.1.100"
    - "api.k8s.example.com"
    - "kubernetes.default.svc"
    - "kubernetes.default"
    - "kubernetes"
    - "localhost"
    - "127.0.0.1"
  extraVolumes:
    - name: "audit-logs"
      hostPath: "/var/log/kubernetes"
      mountPath: "/var/log/kubernetes"
      readOnly: false
      pathType: DirectoryOrCreate
controllerManager:
  extraArgs:
    - name: "node-monitor-grace-period"
      value: "40s"
    - name: "pod-eviction-timeout"
      value: "5m0s"
    - name: "terminated-pod-gc-threshold"
      value: "12500"
    - name: "bind-address"
      value: "0.0.0.0"
scheduler:
  extraArgs:
    - name: "bind-address"
      value: "0.0.0.0"
    - name: "config"
      value: "/etc/kubernetes/scheduler-config.yaml"
certificatesDir: "/etc/kubernetes/pki"
imageRepository: "registry.k8s.io"
etcd:
  local:
    dataDir: "/var/lib/etcd"
    extraArgs:
      - name: "listen-client-urls"
        value: "https://127.0.0.1:2379,https://192.168.1.10:2379"
      - name: "quota-backend-bytes"
        value: "8589934592"
      - name: "auto-compaction-retention"
        value: "1h"
dns:
  imageRepository: "registry.k8s.io/coredns"
  imageTag: "v1.11.3"
proxy:
  disabled: false
---
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
cgroupDriver: "systemd"
clusterDNS:
  - "10.96.0.10"
clusterDomain: "cluster.local"
rotateCertificates: true
serverTLSBootstrap: true
authentication:
  anonymous:
    enabled: false
  webhook:
    enabled: true
  x509:
    clientCAFile: "/etc/kubernetes/pki/ca.crt"
authorization:
  mode: Webhook
healthzBindAddress: "127.0.0.1"
healthzPort: 10248
readOnlyPort: 0
protectKernelDefaults: true
makeIPTablesUtilChains: true
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"
evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"
  imagefs.available: "20%"
evictionSoftGracePeriod:
  memory.available: "1m"
  nodefs.available: "1m30s"
  imagefs.available: "1m30s"
evictionMaxPodGracePeriod: 120
systemReserved:
  cpu: "100m"
  memory: "256Mi"
  ephemeral-storage: "1Gi"
kubeReserved:
  cpu: "100m"
  memory: "256Mi"
  ephemeral-storage: "1Gi"
---
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration
mode: "ipvs"
ipvs:
  scheduler: "rr"
  strictARP: true
  tcpTimeout: "0"
  tcpFinTimeout: "0"
  udpTimeout: "0"
metricsBindAddress: "0.0.0.0:10249"
healthzBindAddress: "0.0.0.0:10256"
clusterCIDR: "10.244.0.0/16"
```

#### v1beta3 配置示例 (兼容版本)

```yaml
# kubeadm-config-v1beta3.yaml
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
bootstrapTokens:
- token: "9a08jv.c0izixklcxtmnze7"
  description: "kubeadm bootstrap token"
  ttl: "24h"
localAPIEndpoint:
  advertiseAddress: "192.168.1.10"
  bindPort: 6443
nodeRegistration:
  name: "control-plane-1"
  criSocket: "unix:///run/containerd/containerd.sock"
  taints:
  - key: "node-role.kubernetes.io/control-plane"
    effect: "NoSchedule"
  kubeletExtraArgs:
    node-ip: "192.168.1.10"
---
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
clusterName: "production-cluster"
kubernetesVersion: "v1.30.0"
controlPlaneEndpoint: "192.168.1.100:6443"
networking:
  serviceSubnet: "10.96.0.0/12"
  podSubnet: "10.244.0.0/16"
  dnsDomain: "cluster.local"
apiServer:
  extraArgs:
    authorization-mode: "Node,RBAC"
    audit-log-path: "/var/log/kubernetes/audit.log"
    audit-log-maxage: "30"
    enable-admission-plugins: "NodeRestriction,LimitRanger,ServiceAccount"
  certSANs:
    - "192.168.1.100"
    - "api.k8s.example.com"
  extraVolumes:
    - name: "audit-logs"
      hostPath: "/var/log/kubernetes"
      mountPath: "/var/log/kubernetes"
imageRepository: "registry.k8s.io"
---
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
cgroupDriver: "systemd"
clusterDNS:
  - "10.96.0.10"
rotateCertificates: true
serverTLSBootstrap: true
```

---

### 2.3 高可用初始化方案

#### Stacked etcd 模式 (etcd 与控制平面同节点)

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                    Stacked etcd HA Architecture                                   │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│                    ┌─────────────────────┐                                       │
│                    │   Load Balancer     │                                       │
│                    │   (VIP: 192.168.1.100)                                     │
│                    │   Port: 6443          │                                       │
│                    └──────────┬──────────┘                                       │
│                               │                                                  │
│              ┌────────────────┼────────────────┐                                │
│              │                │                │                                │
│              ▼                ▼                ▼                                │
│    ┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐                 │
│    │  Control Plane  │ │  Control Plane  │ │  Control Plane  │                 │
│    │     Node 1      │ │     Node 2      │ │     Node 3      │                 │
│    │  192.168.1.10   │ │  192.168.1.11   │ │  192.168.1.12   │                 │
│    │ ┌─────────────┐ │ │ ┌─────────────┐ │ │ ┌─────────────┐ │                 │
│    │ │  kube-apisrv│ │ │ │  kube-apisrv│ │ │ │  kube-apisrv│ │                 │
│    │ │  kube-ctrl  │ │ │ │  kube-ctrl  │ │ │ │  kube-ctrl  │ │                 │
│    │ │  kube-sched │ │ │ │  kube-sched │ │ │ │  kube-sched │ │                 │
│    │ │  etcd (local)│ │ │ │  etcd (local)│ │ │ │  etcd (local)│ │                 │
│    │ └─────────────┘ │ │ └─────────────┘ │ │ └─────────────┘ │                 │
│    └─────────────────┘ └─────────────────┘ └─────────────────┘                 │
│                                                                                  │
│  优势: 节点少, 管理简单, 适合中小规模                                            │
│  劣势: etcd 与 API Server 资源竞争, 故障域不隔离                                 │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

**第一个控制平面节点初始化**:

```bash
# 使用配置文件初始化
kubeadm init --config=kubeadm-config-v1beta4.yaml --upload-certs

# 或使用命令行参数初始化 (单节点测试环境)
kubeadm init \
  --pod-network-cidr=10.244.0.0/16 \
  --service-cidr=10.96.0.0/12 \
  --control-plane-endpoint=192.168.1.100:6443 \
  --upload-certs \
  --kubernetes-version=v1.32.0
```

**其他控制平面节点加入**:

```bash
kubeadm join 192.168.1.100:6443 \
  --token 9a08jv.c0izixklcxtmnze7 \
  --discovery-token-ca-cert-hash sha256:abc123... \
  --control-plane \
  --certificate-key def456...
```

#### External etcd 模式 (etcd 独立部署)

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                   External etcd HA Architecture                                   │
│                                                                                  │
│     etcd Cluster (3-5 节点)              Control Plane Cluster (3 节点)         │
│    ┌─────────────────────┐              ┌─────────────────────┐                 │
│    │   etcd-1  10.0.1.1  │              │   CP-1   10.0.2.1   │                 │
│    │   etcd-2  10.0.1.2  │◄────────────│   CP-2   10.0.2.2   │                 │
│    │   etcd-3  10.0.1.3  │              │   CP-3   10.0.2.3   │                 │
│    └─────────────────────┘              └─────────────────────┘                 │
│                                                  │                               │
│                                                  ▼                               │
│                                         ┌─────────────────────┐                 │
│                                         │   Load Balancer     │                 │
│                                         │   VIP: 10.0.2.100   │                 │
│                                         └─────────────────────┘                 │
│                                                                                  │
│  优势: etcd 与控制平面资源隔离, 可独立扩展                                       │
│  劣势: 节点多, 配置复杂, 适合大规模生产环境                                      │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

**External etcd 配置示例**:

```yaml
# external-etcd-kubeadm-config.yaml
apiVersion: kubeadm.k8s.io/v1beta4
kind: ClusterConfiguration
kubernetesVersion: "v1.32.0"
controlPlaneEndpoint: "192.168.1.100:6443"
etcd:
  external:
    endpoints:
      - "https://10.0.1.1:2379"
      - "https://10.0.1.2:2379"
      - "https://10.0.1.3:2379"
    caFile: "/etc/kubernetes/pki/etcd/ca.crt"
    certFile: "/etc/kubernetes/pki/etcd/server.crt"
    keyFile: "/etc/kubernetes/pki/etcd/server.key"
```

---

### 2.4 自定义 CRI 和镜像仓库

#### 自定义容器运行时 (CRI)

```yaml
# containerd 配置 (默认)
nodeRegistration:
  name: "control-plane-1"
  criSocket: "unix:///run/containerd/containerd.sock"

# CRI-O 配置
nodeRegistration:
  name: "control-plane-1"
  criSocket: "unix:///var/run/crio/crio.sock"

# Docker (已弃用, 仅兼容旧版本)
nodeRegistration:
  name: "control-plane-1"
  criSocket: "unix:///var/run/dockershim.sock"
```

#### 自定义镜像仓库 (私有镜像仓库/国内镜像加速)

```yaml
# 使用阿里云镜像加速器
kind: ClusterConfiguration
imageRepository: "registry.aliyuncs.com/google_containers"

# 使用私有 Harbor 仓库
kind: ClusterConfiguration
imageRepository: "harbor.example.com/k8s-mirror"

# 完全自定义镜像前缀
kind: ClusterConfiguration
imageRepository: "mycustomregistry.io/kubernetes"
```

**离线环境镜像准备脚本**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 离线环境镜像准备脚本

REGISTRY="harbor.example.com/k8s-mirror"
KUBE_VERSION="v1.32.0"

# 定义所需镜像列表
images=(
  "kube-apiserver:${KUBE_VERSION}"
  "kube-controller-manager:${KUBE_VERSION}"
  "kube-scheduler:${KUBE_VERSION}"
  "kube-proxy:${KUBE_VERSION}"
  "pause:3.10"
  "etcd:3.5.16-0"
  "coredns/coredns:v1.11.3"
)

# 拉取并推送镜像到私有仓库
for img in "${images[@]}"; do
    echo "Processing: ${img}"
    docker pull "registry.k8s.io/${img}" || \
        ctr image pull "registry.k8s.io/${img}"
    
    docker tag "registry.k8s.io/${img}" "${REGISTRY}/${img}" || \
        ctr image tag "registry.k8s.io/${img}" "${REGISTRY}/${img}"
    
    docker push "${REGISTRY}/${img}" || \
        ctr image push "${REGISTRY}/${img}"
done

echo "All images pushed to ${REGISTRY}"
```
#### 配置镜像仓库 CA 证书

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 1. 获取私有仓库 CA 证书
openssl s_client -showcerts -connect harbor.example.com:443 < /dev/null \
    | sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' \
    > /etc/containerd/certs.d/harbor.example.com/ca.crt

# 2. 配置 containerd 镜像仓库
cat > /etc/containerd/certs.d/harbor.example.com/hosts.toml <<HOSTEOF
server = "https://harbor.example.com"

[host."https://harbor.example.com"]
  capabilities = ["pull", "resolve"]
  ca = "/etc/containerd/certs.d/harbor.example.com/ca.crt"
HOSTEOF

systemctl restart containerd
```
---

### 2.5 初始化输出和证书管理

#### kubeadm init 标准输出解析

```bash
$ kubeadm init --config=kubeadm-config-v1beta4.yaml --upload-certs

[init] Using Kubernetes version: v1.32.0
[preflight] Running pre-flight checks
[preflight] Pulling images required for setting up a Kubernetes cluster
[certs] Generating "ca" certificate and key
[certs] Generating "apiserver" certificate and key
[certs] apiserver serving cert is signed for DNS names [...]
[kubeconfig] Writing "admin.conf" kubeconfig file
[kubeconfig] Writing "kubelet.conf" kubeconfig file
[control-plane] Creating static Pod manifest for "kube-apiserver"
[control-plane] Creating static Pod manifest for "kube-controller-manager"
[control-plane] Creating static Pod manifest for "kube-scheduler"
[etcd] Creating static Pod manifest for local etcd in "/etc/kubernetes/manifests"
[wait-control-plane] Waiting for the kubelet to boot up the control plane...
[apiclient] All control plane components are healthy after 15.502306 seconds
[upload-config] Storing the configuration used in ConfigMap "kubeadm-config"
[upload-certs] Storing the certificates in Secret "kubeadm-certs"
[mark-control-plane] Marking the node as control-plane
[bootstrap-token] Configuring bootstrap tokens, cluster-info ConfigMap
[addons] Applied essential addon: CoreDNS
[addons] Applied essential addon: kube-proxy

Your Kubernetes control-plane has initialized successfully!

To start using your cluster:
  mkdir -p $HOME/.kube
  sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
  sudo chown $(id -u):$(id -g) $HOME/.kube/config

You can now join any number of control-plane nodes by running:
  kubeadm join 192.168.1.100:6443 --token 9a08jv.c0izixklcxtmnze7 \
    --discovery-token-ca-cert-hash sha256:abc123... \
    --control-plane --certificate-key def456...

Then you can join any number of worker nodes by running:
  kubeadm join 192.168.1.100:6443 --token 9a08jv.c0izixklcxtmnze7 \
    --discovery-token-ca-cert-hash sha256:abc123...
```

#### 证书文件清单

| 证书文件 | 路径 | 用途 | 有效期 |
|----------|------|------|--------|
| **CA** | `/etc/kubernetes/pki/ca.crt` | 集群根证书 | 10年 |
| **API Server** | `/etc/kubernetes/pki/apiserver.crt` | API Server TLS | 1年 |
| **etcd CA** | `/etc/kubernetes/pki/etcd/ca.crt` | etcd 根证书 | 10年 |
| **etcd Server** | `/etc/kubernetes/pki/etcd/server.crt` | etcd 服务端 | 1年 |
| **front-proxy CA** | `/etc/kubernetes/pki/front-proxy-ca.crt` | 前端代理根证书 | 10年 |
| **front-proxy-client** | `/etc/kubernetes/pki/front-proxy-client.crt` | 前端代理客户端 | 1年 |
| **SA 公钥** | `/etc/kubernetes/pki/sa.pub` | ServiceAccount 签名 | 无期限 |
| **SA 私钥** | `/etc/kubernetes/pki/sa.key` | ServiceAccount 签名 | 无期限 |

---

<!-- chunk: 3. 节点加入 (kubeadm join) -->
## 3. 节点加入 (kubeadm join)

### 3.1 控制平面节点加入

```bash
# 方法 1: 使用命令行参数 (推荐)
kubeadm join 192.168.1.100:6443 \
  --token 9a08jv.c0izixklcxtmnze7 \
  --discovery-token-ca-cert-hash sha256:abc123def456... \
  --control-plane \
  --certificate-key def456ghi789...

# 方法 2: 使用配置文件
```

```yaml
# join-control-plane.yaml
apiVersion: kubeadm.k8s.io/v1beta4
kind: JoinConfiguration
discovery:
  bootstrapToken:
    token: "9a08jv.c0izixklcxtmnze7"
    apiServerEndpoint: "192.168.1.100:6443"
    caCertHashes:
      - "sha256:abc123def456..."
  timeout: "5m"
controlPlane:
  localAPIEndpoint:
    advertiseAddress: "192.168.1.11"
    bindPort: 6443
certificateKey: "def456ghi789..."
nodeRegistration:
  name: "control-plane-2"
  criSocket: "unix:///run/containerd/containerd.sock"
```

```bash
kubeadm join --config=join-control-plane.yaml
```

### 3.2 Worker 节点加入

```bash
# 方法 1: 使用命令行参数
kubeadm join 192.168.1.100:6443 \
  --token 9a08jv.c0izixklcxtmnze7 \
  --discovery-token-ca-cert-hash sha256:abc123def456...

# 方法 2: 使用配置文件
```

```yaml
# join-worker.yaml
apiVersion: kubeadm.k8s.io/v1beta4
kind: JoinConfiguration
discovery:
  bootstrapToken:
    token: "9a08jv.c0izixklcxtmnze7"
    apiServerEndpoint: "192.168.1.100:6443"
    caCertHashes:
      - "sha256:abc123def456..."
  timeout: "5m"
nodeRegistration:
  name: "worker-1"
  criSocket: "unix:///run/containerd/containerd.sock"
  kubeletExtraArgs:
    - name: "node-ip"
      value: "192.168.1.20"
```

```bash
kubeadm join --config=join-worker.yaml
```

### 3.3 使用 discovery token / bootstrap token

#### Token 工作机制

```
Bootstrap Token 工作流程:

┌─────────────┐      ┌─────────────────┐      ┌─────────────────┐
│   Worker    │      │  API Server     │      │  kubeadm-certs  │
│   Node      │─────►│  (验证 Token)   │─────►│  Secret (可选)  │
└─────────────┘      └─────────────────┘      └─────────────────┘
        │                     │
        │ 1. 使用 Token 认证   │
        │ 2. 获取 CA 证书哈希  │
        │ 3. 下载 kubeconfig   │
        │ 4. (控制平面)获取证书 │
        ▼                     ▼
┌─────────────────────────────────────────────────────────────┐
│  加入成功 → 生成 kubelet.conf → 启动 kubelet → 注册节点      │
└─────────────────────────────────────────────────────────────┘
```

#### Token 获取方法

```bash
# 方法 1: 在已有控制平面节点上获取
kubeadm token list

# 方法 2: 如果 token 过期, 创建新 token
kubeadm token create --print-join-command

# 输出示例:
# kubeadm join 192.168.1.100:6443 --token abcdef.0123456789abcdef \
#   --discovery-token-ca-cert-hash sha256:abc123...

# 方法 3: 获取控制平面加入所需的 certificate-key
kubeadm init phase upload-certs --upload-certs
# 输出: certificate-key: a1b2c3d4e5f6...
```

#### 使用文件发现 (File Discovery)

```bash
# 将集群 CA 证书复制到新节点
scp /etc/kubernetes/pki/ca.crt root@new-node:/etc/kubernetes/pki/

# 使用文件发现加入
kubeadm join \
  --discovery-file=/etc/kubernetes/pki/ca.crt \
  --token abcdef.0123456789abcdef
```

### 3.4 配置示例

#### 加入带有自定义标签和污点的节点

```yaml
# join-worker-with-labels.yaml
apiVersion: kubeadm.k8s.io/v1beta4
kind: JoinConfiguration
discovery:
  bootstrapToken:
    token: "9a08jv.c0izixklcxtmnze7"
    apiServerEndpoint: "192.168.1.100:6443"
    caCertHashes:
      - "sha256:abc123def456..."
nodeRegistration:
  name: "gpu-worker-1"
  criSocket: "unix:///run/containerd/containerd.sock"
  taints:
    - key: "nvidia.com/gpu"
      value: "true"
      effect: "NoSchedule"
  kubeletExtraArgs:
    - name: "node-labels"
      value: "gpu=true,nvidia.com/gpu.product=Tesla-V100"
    - name: "register-with-taints"
      value: "nvidia.com/gpu=true:NoSchedule"
```

---

<!-- chunk: 4. 集群配置管理 (kubeadm config) -->
## 4. 集群配置管理 (kubeadm config)

### 4.1 config print init-defaults / join-defaults

```bash
# 打印默认初始化配置
kubeadm config print init-defaults

# 打印默认加入配置
kubeadm config print join-defaults

# 打印默认配置并保存到文件
kubeadm config print init-defaults --component-configs=KubeletConfiguration > init-defaults.yaml

# 打印所有组件的默认配置
kubeadm config print init-defaults --component-configs=ALL

# 支持的组件配置
# - KubeletConfiguration
# - KubeProxyConfiguration
```

**init-defaults 输出示例**:

```yaml
apiVersion: kubeadm.k8s.io/v1beta4
bootstrapTokens:
- groups:
  - system:bootstrappers:kubeadm:default-node-token
  token: abcdef.0123456789abcdef
  ttl: 24h0m0s
  usages:
  - signing
  - authentication
kind: InitConfiguration
localAPIEndpoint:
  advertiseAddress: 1.2.3.4
  bindPort: 6443
nodeRegistration:
  criSocket: unix:///var/run/containerd/containerd.sock
  imagePullPolicy: IfNotPresent
  name: node
  taints: null
---
apiServer:
  timeoutForControlPlane: 4m0s
apiVersion: kubeadm.k8s.io/v1beta4
certificatesDir: /etc/kubernetes/pki
clusterName: kubernetes
controllerManager: {}
dns: {}
etcd:
  local:
    dataDir: /var/lib/etcd
imageRepository: registry.k8s.io
kind: ClusterConfiguration
kubernetesVersion: 1.32.0
networking:
  dnsDomain: cluster.local
  serviceSubnet: 10.96.0.0/12
scheduler: {}
```

### 4.2 config images list / pull

```bash
# 列出所需镜像
kubeadm config images list

# 输出:
# registry.k8s.io/kube-apiserver:v1.32.0
# registry.k8s.io/kube-controller-manager:v1.32.0
# registry.k8s.io/kube-scheduler:v1.32.0
# registry.k8s.io/kube-proxy:v1.32.0
# registry.k8s.io/coredns/coredns:v1.11.3
# registry.k8s.io/pause:3.10
# registry.k8s.io/etcd:3.5.16-0

# 指定 Kubernetes 版本
kubeadm config images list --kubernetes-version=v1.31.0

# 使用自定义镜像仓库
kubeadm config images list --image-repository=harbor.example.com/k8s-mirror

# 拉取所有镜像
kubeadm config images pull

# 使用配置文件拉取
kubeadm config images pull --config=kubeadm-config-v1beta4.yaml

# 使用自定义 CRI socket
kubeadm config images pull --cri-socket=unix:///var/run/crio/crio.sock
```

### 4.3 config migrate

```bash
# 将旧版本配置迁移到最新版本
kubeadm config migrate --old-config=old-config.yaml --new-config=new-config.yaml

# 示例: v1beta3 -> v1beta4
kubeadm config migrate \
  --old-config=kubeadm-v1beta3.yaml \
  --new-config=kubeadm-v1beta4.yaml

# 验证迁移后的配置
kubeadm init --dry-run --config=kubeadm-v1beta4.yaml
```

### 4.4 配置验证

```bash
# 验证配置文件语法
kubeadm init --dry-run --config=kubeadm-config-v1beta4.yaml

# 验证加入配置
kubeadm join --dry-run --config=join-worker.yaml

# 使用 validate 子命令 (v1.32+)
kubeadm config validate --config=kubeadm-config-v1beta4.yaml
```

---

<!-- chunk: 5. 证书管理 (kubeadm certs) -->
## 5. 证书管理 (kubeadm certs)

### 5.1 查看证书有效期

```bash
# 查看所有证书有效期
kubeadm certs check-expiration

# 输出示例:
# CERTIFICATE                EXPIRES                  RESIDUAL TIME   CERTIFICATE AUTHORITY   EXTERNALLY MANAGED
# admin.conf                 Apr 23, 2027 08:15 UTC   364d            ca                      no
# apiserver                  Apr 23, 2027 08:15 UTC   364d            ca                      no
# apiserver-etcd-client      Apr 23, 2027 08:15 UTC   364d            etcd-ca                 no
# apiserver-kubelet-client   Apr 23, 2027 08:15 UTC   364d            ca                      no
# controller-manager.conf    Apr 23, 2027 08:15 UTC   364d            ca                      no
# etcd-healthcheck-client    Apr 23, 2027 08:15 UTC   364d            etcd-ca                 no
# etcd-peer                  Apr 23, 2027 08:15 UTC   364d            etcd-ca                 no
# etcd-server                Apr 23, 2027 08:15 UTC   364d            etcd-ca                 no
# front-proxy-client         Apr 23, 2027 08:15 UTC   364d            front-proxy-ca          no
# scheduler.conf             Apr 23, 2027 08:15 UTC   364d            ca                      no
#
# CERTIFICATE AUTHORITY   EXPIRES                  RESIDUAL TIME   EXTERNALLY MANAGED
# ca                      Apr 20, 2036 08:15 UTC   9y              no
# etcd-ca                 Apr 20, 2036 08:15 UTC   9y              no
# front-proxy-ca          Apr 20, 2036 08:15 UTC   9y              no

# 查看特定证书
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates -subject

# 监控脚本: 检查证书即将过期
#!/bin/bash
DAYS_THRESHOLD=30
for cert in /etc/kubernetes/pki/*.crt; do
    expiry=$(openssl x509 -in "$cert" -noout -enddate | cut -d= -f2)
    expiry_epoch=$(date -d "$expiry" +%s)
    now_epoch=$(date +%s)
    days_left=$(( (expiry_epoch - now_epoch) / 86400 ))
    if [ $days_left -lt $DAYS_THRESHOLD ]; then
        echo "WARNING: $(basename $cert) expires in $days_left days"
    fi
done
```

### 5.2 手动轮换证书

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器
> - `kubectl delete`：删除资源（可由声明式清单重建）

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 1. 备份现有证书
mkdir -p /etc/kubernetes/pki/backup-$(date +%Y%m%d)
cp -r /etc/kubernetes/pki/*.crt /etc/kubernetes/pki/*.key /etc/kubernetes/pki/backup-$(date +%Y%m%d)/
cp /etc/kubernetes/*.conf /etc/kubernetes/pki/backup-$(date +%Y%m%d)/

# 2. 轮换所有证书 (保持 CA 不变)
kubeadm certs renew all

# 3. 轮换特定证书
kubeadm certs renew apiserver
kubeadm certs renew apiserver-etcd-client
kubeadm certs renew apiserver-kubelet-client
kubeadm certs renew etcd-healthcheck-client
kubeadm certs renew etcd-peer
kubeadm certs renew etcd-server
kubeadm certs renew front-proxy-client

# 4. 更新 kubeconfig 文件
kubeadm kubeconfig user --org system:masters --client-name kubernetes-admin > /etc/kubernetes/admin.conf
kubeadm kubeconfig user --client-name system:kube-controller-manager > /etc/kubernetes/controller-manager.conf
kubeadm kubeconfig user --client-name system:kube-scheduler > /etc/kubernetes/scheduler.conf

# 5. 重启控制平面组件
# 由于是静态 Pod, 删除 Pod 让 kubelet 重新创建
kubectl delete pod -n kube-system \
  -l component in (kube-apiserver, kube-controller-manager, kube-scheduler)

# 或者重启 kubelet
systemctl restart kubelet

# 6. 验证证书更新
kubeadm certs check-expiration
```
### 5.3 自动轮换配置

#### 使用 kubeadm 自动轮换

```bash
# kubeadm 本身不内置自动轮换, 但可通过 CronJob 实现
# 创建证书轮换 CronJob
```

```yaml
# cert-renewal-cronjob.yaml
apiVersion: batch/v1
kind: CronJob
metadata:
  name: cert-auto-renewal
  namespace: kube-system
spec:
  schedule: "0 2 1 * *"  # 每月 1 日凌晨 2 点
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: cert-renewal
            image: registry.k8s.io/kube-apiserver:v1.32.0
            command:
            - /bin/sh
            - -c
            - |
              if kubeadm certs check-expiration | grep -q "RESIDUAL TIME.*[0-9]d"; then
                kubeadm certs renew all
                # 触发控制平面 Pod 重启
                kubectl delete pod -n kube-system \
                  -l component in (kube-apiserver,kube-controller-manager,kube-scheduler)
              fi
            volumeMounts:
            - name: k8s-certs
              mountPath: /etc/kubernetes/pki
            - name: k8s-conf
              mountPath: /etc/kubernetes
          volumes:
          - name: k8s-certs
            hostPath:
              path: /etc/kubernetes/pki
          - name: k8s-conf
            hostPath:
              path: /etc/kubernetes
          restartPolicy: OnFailure
          nodeSelector:
            node-role.kubernetes.io/control-plane: ""
          tolerations:
          - key: node-role.kubernetes.io/control-plane
            effect: NoSchedule
```

#### 使用 cert-manager 外部管理

```yaml
# 使用 cert-manager 签发 API Server 证书
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: kubernetes-api-server
  namespace: kube-system
spec:
  secretName: apiserver-tls
  issuerRef:
    name: kubernetes-ca-issuer
    kind: ClusterIssuer
  dnsNames:
    - kubernetes
    - kubernetes.default
    - kubernetes.default.svc
    - kubernetes.default.svc.cluster.local
    - api.k8s.example.com
  ipAddresses:
    - 192.168.1.100
    - 10.96.0.1
  duration: 8760h  # 1年
  renewBefore: 720h  # 30天前自动续期
```

### 5.4 CA 证书轮换

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 1. 备份所有证书
mkdir -p /etc/kubernetes/pki/backup-ca-$(date +%Y%m%d)
cp -r /etc/kubernetes/pki/* /etc/kubernetes/pki/backup-ca-$(date +%Y%m%d)/

# 2. 生成新的 CA 证书 (保留旧 CA)
kubeadm certs renew ca

# 3. 使用旧 CA 和新 CA 双信任 (过渡阶段)
# 合并旧 CA 和新 CA 到 ca.crt
cat /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/backup-ca-*/ca.crt > /etc/kubernetes/pki/ca-bundle.crt
mv /etc/kubernetes/pki/ca-bundle.crt /etc/kubernetes/pki/ca.crt

# 4. 更新所有依赖 CA 的证书
kubeadm certs renew all

# 5. 分发更新后的证书到所有节点
for node in control-plane-2 control-plane-3 worker-1 worker-2; do
    scp /etc/kubernetes/pki/ca.crt root@$node:/etc/kubernetes/pki/
    scp /etc/kubernetes/admin.conf root@$node:/etc/kubernetes/
done

# 6. 重启所有 kubelet
systemctl restart kubelet

# 7. 验证集群状态
kubectl get nodes
kubectl get pods -n kube-system
```
---

<!-- chunk: 6. 令牌管理 (kubeadm token) -->
## 6. 令牌管理 (kubeadm token)

### 6.1 创建/列出/删除 bootstrap token

```bash
# 列出所有 token
kubeadm token list

# 创建新 token (默认 24 小时有效期)
kubeadm token create

# 创建指定有效期的 token
kubeadm token create --ttl 48h

# 创建永久有效的 token (不推荐用于生产)
kubeadm token create --ttl 0

# 创建带有特定用途的 token
kubeadm token create --usages signing,authentication

# 创建并打印完整加入命令
kubeadm token create --print-join-command

# 删除 token
kubeadm token delete abcdef.0123456789abcdef

# 删除所有过期 token
kubeadm token delete --all
```

### 6.2 Token 过期策略

| Token 类型 | 默认 TTL | 最大 TTL | 使用场景 |
|-----------|---------|---------|---------|
| **默认 Token** | 24 小时 | 无限制 | 临时节点加入 |
| **短期 Token** | 1 小时 | 24 小时 | 安全敏感环境 |
| **长期 Token** | 8760 小时 (1年) | 无限制 | 自动化/CI 环境 |
| **永久 Token** | 永久 | 永久 | 仅用于测试环境 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安全 Token 管理策略

# 1. 创建短期 Token 用于手动加入
kubeadm token create --ttl 2h --description "Manual node join for maintenance"

# 2. 创建长期 Token 用于自动化 (需加密存储)
kubeadm token create --ttl 8760h --description "Automation token - stored in Vault"

# 3. 定期清理过期 Token (CronJob)
#!/bin/bash
# cleanup-expired-tokens.sh
for token in $(kubeadm token list -o jsonpath='{.token}'); do
    ttl=$(kubeadm token list | grep "$token" | awk '{print $3}')
    if [ "$ttl" = "<invalid>" ]; then
        kubeadm token delete "$token"
    fi
done

# 4. Token 使用审计
kubectl get secret -n kube-system | grep bootstrap-token
kubectl get secret bootstrap-token-abcdef -n kube-system -o yaml
```
---

<!-- chunk: 7. 集群升级 (kubeadm upgrade) -->
## 7. 集群升级 (kubeadm upgrade)

### 7.1 升级前检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 升级前检查脚本

VERSION="v1.32.0"

echo "=== Pre-Upgrade Checks ==="

# 1. 检查当前版本
echo "Current Kubernetes version:"
kubectl version --short

# 2. 检查 kubeadm 版本
echo "kubeadm version:"
kubeadm version

# 3. 检查可升级版本
echo "Available upgrades:"
kubeadm upgrade plan

# 4. 备份 etcd
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-pre-upgrade-$(date +%Y%m%d).db \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/server.crt \
    --key=/etc/kubernetes/pki/etcd/server.key

# 5. 备份 Kubernetes 配置
cp -r /etc/kubernetes /backup/kubernetes-$(date +%Y%m%d)

# 6. 检查节点状态
echo "Node status:"
kubectl get nodes -o wide

# 7. 检查 Pod 状态
echo "System pod status:"
kubectl get pods -n kube-system

# 8. 检查废弃 API
kubectl get --raw /api/v1 | grep -E '"name":' | grep -i deprecated || true

echo "=== Pre-Upgrade Checks Complete ==="
```
### 7.2 控制平面升级步骤

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
控制平面升级顺序 (重要!):

1. 第一个控制平面节点 (持有 kubeadm-certs)
   ├─ 升级 kubeadm
   ├─ kubeadm upgrade plan
   ├─ kubeadm upgrade apply v1.32.0
   └─ 验证控制平面健康

2. 其他控制平面节点 (逐个升级)
   ├─ 升级 kubeadm
   ├─ kubeadm upgrade node
   └─ 验证

3. Worker 节点 (逐个升级)
   ├─ kubectl drain <node>
   ├─ 升级 kubeadm, kubelet, kubectl
   ├─ kubeadm upgrade node
   ├─ systemctl restart kubelet
   └─ kubectl uncordon <node>
```
**第一个控制平面节点升级**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 1. 升级 kubeadm
apt-mark unhold kubeadm
apt-get update
apt-get install -y kubeadm=1.32.0-1.1
apt-mark hold kubeadm

# 2. 验证升级计划
kubeadm upgrade plan

# 3. 执行升级
kubeadm upgrade apply v1.32.0 \
  --certificate-renewal=true \
  --etcd-upgrade=true \
  --patches=/etc/kubernetes/patches

# 4. 升级 kubelet 和 kubectl
apt-mark unhold kubelet kubectl
apt-get install -y kubelet=1.32.0-1.1 kubectl=1.32.0-1.1
apt-mark hold kubelet kubectl
systemctl daemon-reload
systemctl restart kubelet
```
**其他控制平面节点升级**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 1. 升级 kubeadm
apt-mark unhold kubeadm
apt-get update
apt-get install -y kubeadm=1.32.0-1.1
apt-mark hold kubeadm

# 2. 升级节点
kubeadm upgrade node

# 3. 升级 kubelet
apt-mark unhold kubelet kubectl
apt-get install -y kubelet=1.32.0-1.1 kubectl=1.32.0-1.1
apt-mark hold kubelet kubectl
systemctl daemon-reload
systemctl restart kubelet
```
### 7.3 kubelet/kubectl 升级

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# Worker 节点升级 (逐个进行)

# 1. 驱逐节点上的 Pod
kubectl drain worker-1 \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --force \
  --grace-period=30

# 2. 升级 kubeadm
apt-mark unhold kubeadm
apt-get update && apt-get install -y kubeadm=1.32.0-1.1
apt-mark hold kubeadm

# 3. 升级 kubelet 配置
kubeadm upgrade node

# 4. 升级 kubelet 和 kubectl
apt-mark unhold kubelet kubectl
apt-get install -y kubelet=1.32.0-1.1 kubectl=1.32.0-1.1
apt-mark hold kubelet kubectl

# 5. 重启 kubelet
systemctl daemon-reload
systemctl restart kubelet

# 6. 恢复节点可调度
kubectl uncordon worker-1
```
### 7.4 CNI 插件兼容性检查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查当前 CNI 版本
kubectl get daemonset -n kube-system | grep -E 'calico|cilium|flannel|weave'

# 2. 检查 CNI 兼容性矩阵
# Calico v3.26+ 支持 Kubernetes v1.32
# Cilium v1.14+ 支持 Kubernetes v1.32
# Flannel v0.22+ 支持 Kubernetes v1.32

# 3. 升级前备份 CNI 配置
cp -r /etc/cni /backup/cni-$(date +%Y%m%d)

# 4. 升级 CNI (以 Calico 为例)
kubectl apply -f https://raw.githubusercontent.com/projectcalico/calico/v3.27.0/manifests/calico.yaml

# 5. 验证 CNI 状态
kubectl get pods -n kube-system -l k8s-app=calico-node
```
### 7.5 升级后验证

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# 升级后验证脚本

echo "=== Post-Upgrade Verification ==="

# 1. 验证集群版本
echo "1. Cluster Version:"
kubectl version --short

# 2. 验证节点状态
echo "2. Node Status:"
kubectl get nodes -o wide

# 3. 验证系统 Pod
echo "3. System Pods:"
kubectl get pods -n kube-system

# 4. 验证 CoreDNS
echo "4. CoreDNS:"
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl run -it --rm debug --image=busybox:1.36 --restart=Never -- nslookup kubernetes.default

# 5. 验证网络连通性
echo "5. Network Connectivity:"
kubectl run -it --rm netshoot --image=nicolaka/netshoot --restart=Never -- ping -c 3 8.8.8.8

# 6. 验证存储 (如果有)
echo "6. Storage:"
kubectl get sc,pvc,pv

# 7. 运行 sonobuoy 快速测试 (可选)
# sonobuoy run --mode=quick --wait

echo "=== Verification Complete ==="
```
### 7.6 回滚策略

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
#!/bin/bash
# 升级回滚脚本

OLD_VERSION="v1.31.0"
BACKUP_DIR="/backup/kubernetes-$(date +%Y%m%d)"

echo "=== Starting Rollback to ${OLD_VERSION} ==="

# 1. 停止 kubelet
systemctl stop kubelet

# 2. 恢复旧版本二进制文件
apt-mark unhold kubeadm kubelet kubectl
apt-get install -y kubeadm=${OLD_VERSION}-1.1 kubelet=${OLD_VERSION}-1.1 kubectl=${OLD_VERSION}-1.1
apt-mark hold kubeadm kubelet kubectl

# 3. 恢复 etcd 数据 (如果 etcd 已升级)
# ETCDCTL_API=3 etcdctl snapshot restore ${BACKUP_DIR}/etcd-pre-upgrade.db \
#     --data-dir=/var/lib/etcd-restored
# mv /var/lib/etcd /var/lib/etcd-upgraded
# mv /var/lib/etcd-restored /var/lib/etcd

# 4. 恢复静态 Pod manifests
cp ${BACKUP_DIR}/manifests/*.yaml /etc/kubernetes/manifests/

# 5. 重启 kubelet
systemctl daemon-reload
systemctl start kubelet

# 6. 验证
sleep 30
kubectl get nodes
kubectl get pods -n kube-system

echo "=== Rollback Complete ==="

```
---

<!-- chunk: 8. 节点重置与清理 (kubeadm reset) -->
## 8. 节点重置与清理 (kubeadm reset)

### 8.1 安全移除节点

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据
> - `iptables -F/-P DROP`：清空/改防火墙规则，可能立即断网(含SSH)
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 1. 驱逐节点上的工作负载 (在控制平面执行)
kubectl drain worker-1 \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --force \
  --grace-period=30

# 2. 从集群中删除节点
kubectl delete node worker-1

# 3. 在待移除节点上执行重置
kubeadm reset  # ⚠️ 清理节点所有 K8s 配置

# 4. 清理 CNI 配置和网桥
rm -rf /etc/cni/net.d  # ⚠️ 删除系统/数据文件
ip link delete cni0 2>/dev/null || true
ip link delete flannel.1 2>/dev/null || true
ip link delete vxlan.calico 2>/dev/null || true

# 5. 清理 iptables/nftables
iptables -F && iptables -t nat -F && iptables -t mangle -F && iptables -X
ip6tables -F && ip6tables -t nat -F && ip6tables -t mangle -F && ip6tables -X

# 6. 清理容器和镜像 (可选)
ctr -n k8s.io containers list | awk '{print $1}' | xargs -r ctr -n k8s.io containers delete
ctr -n k8s.io images list | awk '{print $1}' | xargs -r ctr -n k8s.io images delete

# 7. 清理 kubelet 数据
rm -rf /var/lib/kubelet/*  # ⚠️ 删除系统/数据文件
rm -rf /var/lib/cni/  # ⚠️ 删除系统/数据文件
```
### 8.2 清理 etcd 成员

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl member remove`：移除 etcd 成员，误删多数派会致集群不可用/丢数据

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 在控制平面节点查看 etcd 成员列表
ETCDCTL_API=3 etcdctl member list \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/server.crt \
    --key=/etc/kubernetes/pki/etcd/server.key

# 输出示例:
# 12345, started, control-plane-1, https://192.168.1.10:2380, https://192.168.1.10:2379, false
# 12346, started, control-plane-2, https://192.168.1.11:2380, https://192.168.1.11:2379, false
# 12347, started, control-plane-3, https://192.168.1.12:2380, https://192.168.1.12:2379, false

# 2. 移除 etcd 成员 (在移除控制平面节点前)
ETCDCTL_API=3 etcdctl member remove 12347 \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/server.crt \
    --key=/etc/kubernetes/pki/etcd/server.key

# 3. 验证成员移除
ETCDCTL_API=3 etcdctl member list \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/server.crt \
    --key=/etc/kubernetes/pki/etcd/server.key

# 4. 验证 etcd 集群健康
ETCDCTL_API=3 etcdctl endpoint health --cluster \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/server.crt \
    --key=/etc/kubernetes/pki/etcd/server.key
```
### 8.3 保留/删除数据选项

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```bash
# 标准重置 (保留 /var/lib/etcd 中的 etcd 数据)
kubeadm reset  # ⚠️ 清理节点所有 K8s 配置

# 强制重置 (不提示确认)
kubeadm reset --force  # ⚠️ 清理节点所有 K8s 配置

# 完全重置并删除 etcd 数据
kubeadm reset --force --cert-dir=/etc/kubernetes/pki  # ⚠️ 清理节点所有 K8s 配置

# 重置并清理 CRI socket
kubeadm reset --cri-socket=unix:///var/run/crio/crio.sock  # ⚠️ 清理节点所有 K8s 配置

# 重置并跳过某些阶段
kubeadm reset --skip-phases=remove-etcd-member  # ⚠️ 清理节点所有 K8s 配置

# 自定义重置 (分阶段执行)
kubeadm reset phase preflight  # ⚠️ 清理节点所有 K8s 配置
kubeadm reset phase remove-etcd-member  # ⚠️ 清理节点所有 K8s 配置
kubeadm reset phase cleanup-node  # ⚠️ 清理节点所有 K8s 配置
```

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

```
kubeadm reset 清理内容:  # ⚠️ 清理节点所有 K8s 配置

默认清理:
├── /etc/kubernetes/manifests/ (静态 Pod manifests)
├── /etc/kubernetes/pki/ (证书 - 除非自定义 --cert-dir)
├── /etc/kubernetes/*.conf (kubeconfig 文件)
├── /var/lib/kubelet/ (kubelet 配置和状态)
├── kubelet 生成的 iptables 规则
└── kubelet 挂载的卷

可选清理 (--cleanup-tmp-dir):
├── /var/lib/dockershim (已弃用)
├── /var/run/kubernetes
└── /var/lib/cni

保留 (需手动清理):
├── /var/lib/etcd (etcd 数据)
├── 容器镜像
├── CNI 插件配置
└── 自定义 kubelet 配置

```

---

<!-- chunk: 9. 高可用集群管理 -->
## 9. 高可用集群管理

### 9.1 添加/移除控制平面节点

#### 添加控制平面节点

```bash
# 1. 在现有控制平面节点生成新的 certificate-key
kubeadm init phase upload-certs --upload-certs
# 输出: [upload-certs] Certificate key: a1b2c3d4e5f6...

# 2. 获取加入命令
kubeadm token create --print-join-command

# 3. 在新节点执行 (替换为实际的 token, hash 和 certificate-key)
kubeadm join 192.168.1.100:6443 \
  --token abcdef.0123456789abcdef \
  --discovery-token-ca-cert-hash sha256:abc123... \
  --control-plane \
  --certificate-key a1b2c3d4e5f6...

# 4. 更新负载均衡器配置，添加新节点到后端池
```

#### 移除控制平面节点

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl member remove`：移除 etcd 成员，误删多数派会致集群不可用/丢数据
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群
> - `kubectl delete`：删除资源（可由声明式清单重建）

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 1. 确认集群至少有 3 个控制平面节点
kubectl get nodes -l node-role.kubernetes.io/control-plane

# 2. 移除 etcd 成员
ETCDCTL_API=3 etcdctl member list \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/server.crt \
    --key=/etc/kubernetes/pki/etcd/server.key

# 记录要移除的成员 ID，然后移除
ETCDCTL_API=3 etcdctl member remove <MEMBER_ID> \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/server.crt \
    --key=/etc/kubernetes/pki/etcd/server.key

# 3. 在待移除节点上执行重置
kubeadm reset --force  # ⚠️ 清理节点所有 K8s 配置

# 4. 从集群中删除节点对象
kubectl delete node <node-name>

# 5. 更新负载均衡器配置，移除该节点
```
### 9.2 etcd 成员管理

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl member remove`：移除 etcd 成员，误删多数派会致集群不可用/丢数据

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# etcd 成员管理脚本

ENDPOINTS="https://127.0.0.1:2379"
CA="/etc/kubernetes/pki/etcd/ca.crt"
CERT="/etc/kubernetes/pki/etcd/server.crt"
KEY="/etc/kubernetes/pki/etcd/server.key"

# 1. 查看 etcd 集群状态
show_status() {
    echo "=== etcd Cluster Status ==="
    ETCDCTL_API=3 etcdctl endpoint status --cluster \
        --endpoints=$ENDPOINTS \
        --cacert=$CA --cert=$CERT --key=$KEY \
        --write-out=table
}

# 2. 查看 etcd 成员列表
show_members() {
    echo "=== etcd Members ==="
    ETCDCTL_API=3 etcdctl member list \
        --endpoints=$ENDPOINTS \
        --cacert=$CA --cert=$CERT --key=$KEY \
        --write-out=table
}

# 3. 添加新成员 (External etcd 模式)
add_member() {
    local peer_url=$1
    echo "Adding member with peer URL: $peer_url"
    ETCDCTL_API=3 etcdctl member add new-etcd-node \
        --peer-urls=$peer_url \
        --endpoints=$ENDPOINTS \
        --cacert=$CA --cert=$CERT --key=$KEY
}

# 4. 移除成员
remove_member() {
    local member_id=$1
    echo "Removing member: $member_id"
    ETCDCTL_API=3 etcdctl member remove $member_id \
        --endpoints=$ENDPOINTS \
        --cacert=$CA --cert=$CERT --key=$KEY
}

# 5. 检查集群健康
check_health() {
    echo "=== etcd Health ==="
    ETCDCTL_API=3 etcdctl endpoint health --cluster \
        --cacert=$CA --cert=$CERT --key=$KEY
}

# 执行
case "$1" in
    status) show_status ;;
    members) show_members ;;
    add) add_member "$2" ;;
    remove) remove_member "$2" ;;
    health) check_health ;;
    *) echo "Usage: $0 {status|members|add <peer_url>|remove <member_id>|health}" ;;
esac
```
### 9.3 负载均衡器配置

#### HAProxy 配置示例

```
global
    log /dev/log local0
    log /dev/log local1 notice
    chroot /var/lib/haproxy
    stats socket /run/haproxy/admin.sock mode 660 level admin
    stats timeout 30s
    user haproxy
    group haproxy
    daemon

defaults
    log global
    mode tcp
    option tcplog
    option dontlognull
    timeout connect 5000
    timeout client 50000
    timeout server 50000

frontend kubernetes-apiserver
    bind *:6443
    default_backend kubernetes-apiserver

backend kubernetes-apiserver
    balance roundrobin
    option tcp-check
    server cp-1 192.168.1.10:6443 check fall 3 rise 2
    server cp-2 192.168.1.11:6443 check fall 3 rise 2
    server cp-3 192.168.1.12:6443 check fall 3 rise 2
```

#### Keepalived + HAProxy (高可用负载均衡)

```bash
# keepalived.conf (主节点)
vrrp_instance VI_1 {
    state MASTER
    interface eth0
    virtual_router_id 51
    priority 101
    advert_int 1
    authentication {
        auth_type PASS
        auth_pass K8sHaProxy123
    }
    virtual_ipaddress {
        192.168.1.100/24
    }
}

# keepalived.conf (备节点)
vrrp_instance VI_1 {
    state BACKUP
    interface eth0
    virtual_router_id 51
    priority 100
    advert_int 1
    authentication {
        auth_type PASS
        auth_pass K8sHaProxy123
    }
    virtual_ipaddress {
        192.168.1.100/24
    }
}
```

#### Nginx 负载均衡配置

```nginx
stream {
    upstream kubernetes {
        server 192.168.1.10:6443 max_fails=3 fail_timeout=30s;
        server 192.168.1.11:6443 max_fails=3 fail_timeout=30s;
        server 192.168.1.12:6443 max_fails=3 fail_timeout=30s;
    }

    server {
        listen 6443;
        proxy_pass kubernetes;
        proxy_timeout 30s;
        proxy_connect_timeout 5s;
    }
}

```

---

<!-- chunk: 10. 故障排查 -->
## 10. 故障排查

### 10.1 初始化失败排查

#### 常见错误及解决方案

| 错误信息 | 原因 | 解决方案 |
|---------|------|---------|
| `[ERROR CRI]` | 容器运行时未安装或配置错误 | 检查 containerd/cri-o 状态, 配置 crictl |
| `[ERROR Swap]` | Swap 未关闭 | `swapoff -a` 并修改 fstab |
| `[ERROR NumCPU]` | CPU 核数不足 | 使用 `--ignore-preflight-errors=NumCPU` 或升级硬件 |
| `[ERROR Port-10250]` | 端口被占用 | `lsof -i :10250` 并停止占用进程 |
| `[ERROR FileContent--proc-sys-net-ipv4-ip_forward]` | IP 转发未启用 | `sysctl -w net.ipv4.ip_forward=1` |
| `kubelet isn't running` | kubelet 未启动 | `journalctl -u kubelet -f` 查看日志 |
| `connection refused` | API Server 未就绪 | 等待控制平面 Pod 启动, 检查 kubelet 日志 |

#### 初始化故障排查命令

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 kubeadm 详细日志
kubeadm init --v=5  # 日志级别 0-9

# 2. 查看 kubelet 日志
journalctl -u kubelet -f --no-pager

# 3. 查看静态 Pod 状态
kubectl get pods -n kube-system

# 4. 查看容器运行时日志
journalctl -u containerd -f --no-pager

# 5. 检查网络插件 (CNI)
ls -la /etc/cni/net.d/
cat /etc/cni/net.d/*.conf

# 6. 跳过特定预检查 (临时)
kubeadm init --ignore-preflight-errors=Swap,NumCPU

# 7. 重置后重试
kubeadm reset --force  # ⚠️ 清理节点所有 K8s 配置
kubeadm init --config=kubeadm-config.yaml
```
### 10.2 加入失败排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 验证 Token 有效性
kubeadm token list

# 2. 验证 CA 证书哈希
echo "Expected hash:"
openssl x509 -pubkey -in /etc/kubernetes/pki/ca.crt | \
    openssl rsa -pubin -outform der 2>/dev/null | \
    openssl dgst -sha256 -hex | sed 's/^.* //'

# 3. 验证 API Server 可达性
curl -vk https://192.168.1.100:6443/healthz

# 4. 验证 bootstrap token 权限
kubectl get secret -n kube-system | grep bootstrap-token

# 5. 查看 join 详细日志
kubeadm join 192.168.1.100:6443 --token xxx --discovery-token-ca-cert-hash sha256:xxx --v=5

# 6. 检查节点防火墙
iptables -L -n | grep 6443
```
### 10.3 网络插件安装问题

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 CNI 插件是否安装
ls -la /opt/cni/bin/

# 2. 检查 CNI 配置文件
ls -la /etc/cni/net.d/

# 3. 检查 CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl logs -n kube-system -l k8s-app=kube-dns

# 4. 测试 DNS 解析
kubectl run -it --rm debug --image=busybox:1.36 --restart=Never -- nslookup kubernetes.default

# 5. 检查 kube-proxy
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl logs -n kube-system -l k8s-app=kube-proxy

# 6. 常见 CNI 安装命令
# Calico
kubectl apply -f https://raw.githubusercontent.com/projectcalico/calico/v3.27.0/manifests/calico.yaml

# Cilium
helm install cilium cilium/cilium --version 1.15.0 --namespace kube-system

# Flannel
kubectl apply -f https://github.com/flannel-io/flannel/releases/latest/download/kube-flannel.yml
```
### 10.4 证书问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 证书过期诊断
kubeadm certs check-expiration

# 2. 证书不匹配诊断
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -text | grep -A1 "Subject Alternative Name"

# 3. kubeconfig 证书问题
kubectl config view --raw

# 4. 手动验证证书链
openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver.crt

# 5. 证书权限问题
ls -la /etc/kubernetes/pki/
# 确保权限为 600 (key) 和 644 (crt)
chmod 600 /etc/kubernetes/pki/*.key
chmod 644 /etc/kubernetes/pki/*.crt

# 6. 重新生成 kubeconfig
kubeadm kubeconfig user --client-name kubernetes-admin \
  --config /etc/kubernetes/admin.conf > /tmp/admin.conf
```
---

<!-- chunk: 11. 生产环境 Checklist -->
## 11. 生产环境 Checklist

### 11.1 部署前 Checklist

```yaml
production_deployment_checklist:
  infrastructure:
    - [ ] 所有节点满足最低硬件要求
    - [ ] 网络连通性验证通过 (所有节点间)
    - [ ] Swap 已禁用并持久化配置
    - [ ] 必要的内核模块已加载 (overlay, br_netfilter)
    - [ ] sysctl 参数已优化配置
    - [ ] 防火墙规则已配置 (放行必要端口)
    - [ ] 时间同步已配置 (NTP/Chrony)
    - [ ] 主机名和 DNS 解析正确配置
    
  software:
    - [ ] 容器运行时版本兼容
    - [ ] kubeadm/kubelet/kubectl 版本一致
    - [ ] 版本已固定 (apt-mark hold / yum versionlock)
    - [ ] crictl 已配置并可正常工作
    - [ ] 私有镜像仓库可访问 (如需要)
    
  security:
    - [ ] SELinux/AppArmor 策略已配置
    - [ ] 非 root 用户管理 kubeconfig
    - [ ] 审计日志目录已创建并有足够空间
    - [ ] etcd 数据目录使用高速存储 (SSD/NVMe)
```

### 11.2 初始化 Checklist

```yaml
cluster_initialization_checklist:
  control_plane:
    - [ ] 使用配置文件而非命令行参数
    - [ ] controlPlaneEndpoint 配置正确 (高可用环境)
    - [ ] certSANs 包含所有可能的访问地址
    - [ ] --upload-certs 已使用 (高可用环境)
    - [ ] 网络 CIDR 规划不冲突
    - [ ] 镜像仓库配置正确 (离线/私有环境)
    
  verification:
    - [ ] 所有控制平面 Pod 运行正常
    - [ ] CoreDNS 运行正常
    - [ ] kube-proxy 运行正常
    - [ ] 节点状态 Ready
    - [ ] CNI 插件安装成功
    - [ ] Pod 间网络互通
    - [ ] DNS 解析正常
    - [ ] Service 访问正常
```

### 11.3 日常运维 Checklist

```yaml
daily_operations_checklist:
  monitoring:
    - [ ] 节点资源使用率监控
    - [ ] 控制平面组件健康检查
    - [ ] etcd 集群健康检查
    - [ ] 证书有效期监控 (设置 30 天告警)
    
  maintenance:
    - [ ] 定期备份 etcd 数据
    - [ ] 定期备份 Kubernetes 配置
    - [ ] 定期轮换证书 (建议自动化)
    - [ ] 定期清理过期 bootstrap token
    - [ ] 监控磁盘空间 (日志、etcd、镜像)
    
  security:
    - [ ] 审计日志定期归档
    - [ ] RBAC 权限定期审查
    - [ ] Secret 加密状态检查
    - [ ] 网络策略生效检查
```

### 11.4 升级 Checklist

```yaml
upgrade_checklist:
  preparation:
    - [ ] 阅读目标版本 CHANGELOG
    - [ ] 验证版本兼容性矩阵
    - [ ] 在测试环境验证升级流程
    - [ ] 完整备份 etcd 数据
    - [ ] 完整备份 Kubernetes 配置
    - [ ] 确认回滚方案可用
    - [ ] 通知相关团队升级窗口
    
  execution:
    - [ ] 先升级控制平面节点
    - [ ] 验证控制平面健康后再升级 Worker
    - [ ] 逐个升级 Worker 节点 (drain -> upgrade -> uncordon)
    - [ ] 验证 CNI 兼容性
    - [ ] 验证 CoreDNS 兼容性
    - [ ] 验证存储插件兼容性
    
  verification:
    - [ ] 所有节点版本一致
    - [ ] 所有系统 Pod 运行正常
    - [ ] 应用工作负载运行正常
    - [ ] 网络连通性验证
    - [ ] 存储访问验证
    - [ ] 监控告警系统正常
```

---

<!-- chunk: 总结 -->
## 总结

kubeadm 作为 Kubernetes 官方推荐的集群生命周期管理工具，在生产环境中具有不可替代的价值。掌握 kubeadm 的完整使用流程是 Kubernetes 运维工程师的核心技能。

关键要点回顾:

1. **初始化规划**: 提前设计网络 CIDR、高可用架构、证书 SANs
2. **配置管理**: 使用声明式配置文件 (v1beta4) 而非命令行参数
3. **证书管理**: 建立自动轮换机制，监控证书有效期
4. **升级策略**: 遵循 "先控制平面后 Worker" 的原则，逐个节点升级
5. **备份优先**: 任何变更前完整备份 etcd 和配置
6. **安全基线**: 禁用 Swap、启用 RBAC、配置审计日志、定期轮换 Token

通过遵循本指南的最佳实践和 Checklist，可以确保 Kubernetes 集群的稳定运行和安全合规。

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 KUDIG Database — Global MOC
- [[01-集群基础/README.md|Domain-3: Kubernetes控制平面]]
- Domain-3 控制平面 — 开源项目索引
- Kubernetes 控制平面架构总览 (Control Plane Architecture Overview)
- 控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)
- 控制平面高可用部署模式 (Control Plane High Availability Deployment Patt...
- 控制平面安全加固指南 (Control Plane Security Hardening Guide)
- 控制平面监控与可观测性 (Control Plane Monitoring & Observability)
- 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)
- 控制平面升级与迁移策略 (Control Plane Upgrade & Migration Strategy)
- 控制平面性能基准测试 (Control Plane Performance Benchmarking)
- 控制平面扩缩容指南 (Control Plane Scalability Guide)

## See Also

- 30-dynamic-resource-allocation
- 31-kubectl-complete-reference
- 32-kubeadm-upgrade-complete-guide
- 33-kubelet-eviction-thresholds

## Related

- [[21-生态参考/03-领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]

```

<!-- risk-assessed -->
