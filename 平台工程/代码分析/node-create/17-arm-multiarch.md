---
title: ARM 节点与多架构混合集群
description: 系统介绍 Kubernetes ARM64 节点接入流程、多架构镜像构建策略、混合架构集群的调度配置，以及常见的 ARM 兼容性问题排查方法。
summary: 系统介绍 Kubernetes ARM64 节点接入流程、多架构镜像构建策略、混合架构集群的调度配置，以及常见的 ARM 兼容性问题排查方法。
category: node-create
tags:
- arm64
- multi-arch
- heterogeneous-cluster
- node-affinity
- image-manifest-list
- buildx
- cross-compilation
- kubelet
- containerd
- docker
tier: supporting
created: '2026-05-23'
last_updated: 2026-05-21
difficulty: advanced
reading_level: advanced
audience:
- platform-engineer
- kubernetes-administrator
- sre
estimated_read_time: 5min
intent_queries:
- kubernetes arm64 node setup
- kubernetes multi-arch mixed node cluster
- arm64 kubernetes node join kubeadm
- multi-architecture docker image kubernetes
- kubernetes node affinity arm amd64
trigger_keywords:
- arm64
- aarch64
- multi-arch
- manifest list
- docker buildx
- kubernetes.io/arch=arm64
- node affinity arm
- heterogeneous
- Graviton
- Apple M1/M2
prerequisites:
- kubectl-basics
- platform-engineering-basics
- ebpf-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
related_domains:
- 集群基础
- 集群基础
related_topics:
- node-create
- registration
- cloud-node
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# ARM 节点与多架构混合集群

## 架构概述

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────────────┐
│               多架构 Kubernetes 集群                                  │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  amd64 节点（控制面 + 通用工作负载）                                  │
│  ├── 高性能计算密集型工作负载                                         │
│  ├── 遗留应用（未提供 ARM 镜像）                                      │
│  └── 特定 x86 依赖组件                                               │
│                                                                     │
│  arm64 节点（成本优化工作负载）                                       │
│  ├── AWS Graviton2/3 (C6g/C7g/M6g 等)                              │
│  ├── Ampere Altra（云上 ARM 服务器）                                  │
│  └── 云原生无状态应用（天然支持多架构）                               │
│                                                                     │
│  关键优势：ARM arm64 的性价比通常比 x86 高 20%-40%                   │
└─────────────────────────────────────────────────────────────────────┘
```
## ARM 节点前置条件

### 检查节点架构

```bash
# 在 ARM 节点上确认架构
uname -m
# 应输出：aarch64（即 arm64）

arch
# 应输出：aarch64

# 查看 CPU 信息
lscpu | grep Architecture
# Architecture: aarch64
```

### 安装 ARM 版 kubelet/kubeadm/kubectl

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 以 Ubuntu/Debian ARM64 为例
KUBE_VERSION="1.30.0"
ARCH="arm64"

# 方法一：通过 apt 安装（推荐）
# apt 会自动选择正确的架构包
apt-get update
apt-get install -y kubelet=${KUBE_VERSION}-* kubeadm=${KUBE_VERSION}-* kubectl=${KUBE_VERSION}-*

# 方法二：手动下载 arm64 二进制
curl -LO "https://dl.k8s.io/release/v${KUBE_VERSION}/bin/linux/arm64/kubelet"
curl -LO "https://dl.k8s.io/release/v${KUBE_VERSION}/bin/linux/arm64/kubeadm"
curl -LO "https://dl.k8s.io/release/v${KUBE_VERSION}/bin/linux/arm64/kubectl"

chmod +x kubelet kubeadm kubectl
mv kubelet kubeadm kubectl /usr/local/bin/
```
### 安装 containerd（ARM64）

```bash
# ARM64 版 containerd 安装
CONTAINERD_VERSION="1.7.13"

# apt 自动选择 arm64 包
apt-get install -y containerd

# 或手动下载
curl -L "https://github.com/containerd/containerd/releases/download/v${CONTAINERD_VERSION}/containerd-${CONTAINERD_VERSION}-linux-arm64.tar.gz" \
  -o containerd.tar.gz

tar -xvf containerd.tar.gz -C /usr/local

# 验证架构
containerd --version
# containerd github.com/containerd/containerd 1.7.13 linux/arm64
```

## 节点加入集群

### ARM 节点 kubeadm join

```bash
# ARM 节点的 join 命令与 amd64 完全相同
# kubeadm 自动检测架构
kubeadm join <control-plane-ip>:6443 \
  --token <token> \
  --discovery-token-ca-cert-hash sha256:<hash>

# 加入后，节点的架构标签会自动设置：
# kubernetes.io/arch: arm64
```

## 多架构镜像构建

ARM 节点要运行应用，首要条件是镜像必须支持 arm64 架构。

### 使用 Docker Buildx 构建多架构镜像

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 buildx builder 并启用多平台支持
docker buildx create --name multiarch-builder --use --platform linux/amd64,linux/arm64

# 构建并推送多架构镜像（包含 amd64 和 arm64）
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  --tag myregistry/myapp:v1.0.0 \
  --push \
  .

# 验证 manifest list（查看镜像支持的架构列表）
docker buildx imagetools inspect myregistry/myapp:v1.0.0
```
### 查看镜像架构支持

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看镜像 manifest list
docker manifest inspect nginx:latest | grep -A5 '"architecture"'
```
```json
{
  "mediaType": "application/vnd.docker.distribution.manifest.v2+json",
  "digest": "sha256:xxx",
  "platform": {
    "architecture": "amd64",
    "os": "linux"
  }
},
{
  "mediaType": "application/vnd.docker.distribution.manifest.v2+json",
  "digest": "sha256:yyy",
  "platform": {
    "architecture": "arm64",
    "os": "linux",
    "variant": "v8"
  }
}
```

## 调度策略

### 方案一：nodeSelector（硬限制）

```yaml
# 将工作负载强制调度到 arm64 节点
spec:
  nodeSelector:
    kubernetes.io/arch: arm64
```

### 方案二：Node Affinity（软/硬混合）

```yaml
# 优先调度到 arm64，如无 arm64 节点则调度到 amd64
spec:
  affinity:
    nodeAffinity:
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 80
        preference:
          matchExpressions:
          - key: kubernetes.io/arch
            operator: In
            values:
            - arm64
      - weight: 20
        preference:
          matchExpressions:
          - key: kubernetes.io/arch
            operator: In
            values:
            - amd64
```

### 方案三：混合架构 Deployment（多架构同时运行）

```yaml
# 多个 Deployment 分别对应不同架构，共享同一 Service
---
# amd64 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-amd64
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web
      arch: amd64
  template:
    metadata:
      labels:
        app: web
        arch: amd64
    spec:
      nodeSelector:
        kubernetes.io/arch: amd64
      containers:
      - name: web
        image: myregistry/myapp:v1.0.0  # 多架构镜像自动选择
---
# arm64 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-arm64
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web
      arch: arm64
  template:
    metadata:
      labels:
        app: web
        arch: arm64
    spec:
      nodeSelector:
        kubernetes.io/arch: arm64
      containers:
      - name: web
        image: myregistry/myapp:v1.0.0  # 同一镜像，自动选择 arm64 layer
---
# 统一 Service
apiVersion: v1
kind: Service
metadata:
  name: web
spec:
  selector:
    app: web  # 同时覆盖两个 Deployment 的 Pod
  ports:
  - port: 80
```

## 验证多架构节点状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有节点的架构标签
kubectl get nodes -o custom-columns=\
'NAME:.metadata.name,ARCH:.metadata.labels.kubernetes\.io/arch,OS:.metadata.labels.kubernetes\.io/os,STATUS:.status.conditions[-1].type'
```
```
# 🟢 低风险：只读/信息收集，通常无副作用
NAME              ARCH    OS      STATUS
master-amd64      amd64   linux   Ready
worker-amd64-1    amd64   linux   Ready
worker-arm64-1    arm64   linux   Ready  ← AWS Graviton
worker-arm64-2    arm64   linux   Ready  ← AWS Graviton
```
### 验证 Pod 运行在正确架构的节点上

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -o wide | awk '{print $1, $7}'
# NAME              NODE
# web-amd64-xxx     worker-amd64-1
# web-arm64-yyy     worker-arm64-1
```
## ARM 常见兼容性问题

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| 镜像不支持 arm64 | 镜像只有 amd64 层 | 构建多架构镜像或寻找 arm64 替代镜像 |
| 原生库编译失败 | CGO 依赖 x86 指令集 | 使用 GOARCH=arm64 交叉编译 |
| JVM 性能差异 | JIT 编译策略不同 | 更新到支持 ARM 优化的 JDK 版本（如 JDK 17+） |
| SIMD 加速缺失 | x86 SSE/AVX 指令不可用 | 使用 ARM NEON 指令集优化版本 |
| 部分 eBPF 程序不兼容 | eBPF JIT 差异 | 检查 CNI/安全插件的 ARM 支持情况 |

## 成本优化参考

```
# 🟢 低风险：只读/信息收集，通常无副作用
以 AWS 为例（2024 参考价格，实际以官网为准）：

c6i.2xlarge (amd64)  : $0.34/h  8 vCPU 16GB
c6g.2xlarge (arm64)  : $0.272/h 8 vCPU 16GB

Graviton 节省比例约 20%-40%（视工作负载特性）
Spot 实例叠加 Graviton = 最高性价比组合
```
## 相关函数

- [`节点注册`](02-registration.md) — kubelet 注册流程，自动检测架构
- [`云厂商节点集成`](15-cloud-node.md) — AWS Graviton、阿里云倚天等 ARM 云节点
- [`节点自动扩缩容`](07-autoscaling.md) — 多架构节点池的 Cluster Autoscaler 配置

## 版本说明

- ARM64 节点自 Kubernetes v1.16 起正式支持
- Docker Buildx 多架构构建自 Docker 19.03 起支持
- 基于 Kubernetes v1.28 – v1.32 和 containerd v1.7 文档

## Related

- [[entities/kubernetes.md|kubernetes]]
- [[entities/cni.md|cni]]
- [[entities/containerd.md|containerd]]
- [[系统基础/速查卡/go.md|go]]
- [[系统基础/速查卡/docker.md|docker]]


<!-- risk-assessed -->
