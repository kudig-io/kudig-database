---
title: Windows 节点接入与管理
description: 系统介绍 Kubernetes Windows 节点的接入流程、containerd 配置要求、Windows 与 Linux 节点的调度隔离策略，以及 Windows 容器特有限制与常见问题排查。
category: node-create
tags:
- windows-node
- windows-container
- containerd-windows
- node-selector
- taint-toleration
- hybrid-cluster
- etcd
- apiserver
- kubelet
- scheduler
last_updated: 2026-05-21
difficulty: advanced
reading_level: advanced
audience:
- platform-engineer
- kubernetes-administrator
- sre
estimated_read_time: 6min
intent_queries:
- kubernetes windows node join cluster
- windows container kubernetes node configuration
- windows node taint nodeselector kubernetes
- containerd windows kubernetes setup
- kubernetes hybrid linux windows cluster
trigger_keywords:
- Windows Node
- windows-container
- node.kubernetes.io/os=windows
- kubeadm windows
- containerd runtime windows
- kubernetes.io/os: windows
- WindowsOnly taint
- HostProcess container
- Windows node join
prerequisites:
- kubectl-basics
- platform-engineering-basics
- cni-basics
- etcd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
related_domains:
- domain-01-cluster-fundamentals
- domain-01-cluster-fundamentals
related_topics:
- node-create
- registration
- cni-node
created: "2026-05-23"
---

# Windows 节点接入与管理

## 架构概述

```
┌─────────────────────────────────────────────────────────────────────┐
│              Kubernetes 混合集群（Linux + Windows）                   │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  Control Plane（必须是 Linux）                                        │
│  ├── kube-apiserver                                                 │
│  ├── kube-scheduler                                                 │
│  ├── kube-controller-manager                                        │
│  └── etcd                                                           │
│                                                                     │
│  Linux Worker Nodes                    Windows Worker Nodes         │
│  ├── containerd                        ├── containerd (Windows 版)  │
│  ├── kube-proxy (iptables/ipvs)        ├── kube-proxy (HNS 模式)   │
│  └── Linux 容器工作负载               └── Windows 容器工作负载      │
│                                                                     │
│  关键约束：                                                           │
│  • Windows 节点不能运行 Linux 容器                                   │
│  • Windows 不支持 init containers（部分场景）                        │
│  • Windows 不支持 privileged containers（用 HostProcess 替代）      │
└─────────────────────────────────────────────────────────────────────┘
```

## Windows 节点支持矩阵

| Kubernetes 版本 | Windows Server 版本 | 支持状态 |
|----------------|-------------------|---------|
| v1.32 | Windows Server 2022 LTSC | 生产就绪 |
| v1.32 | Windows Server 2019 LTSC | 生产就绪 |
| v1.32 | Windows Server 2016 LTSC | 不推荐（EOL） |
| v1.28+ | Windows Server 23H2 | 支持 |

## 前置条件

### Windows 节点系统要求

```powershell
# 检查 Windows 版本（需要 Windows Server 2019 或更高）
[System.Environment]::OSVersion.Version
# 应输出 10.0.17763 或更高

# 检查系统架构（仅支持 amd64）
[System.Environment]::Is64BitOperatingSystem

# 检查 Hyper-V 功能（某些 CNI 需要）
Get-WindowsOptionalFeature -FeatureName Microsoft-Hyper-V-All -Online
```

### 安装 containerd（Windows 版）

```powershell
# 下载 containerd for Windows
$Version = "1.7.13"
curl.exe -L "https://github.com/containerd/containerd/releases/download/v$Version/containerd-$Version-windows-amd64.tar.gz" -o containerd.tar.gz

tar.exe -xvf containerd.tar.gz -C "$Env:ProgramFiles\containerd" --strip-components 1

# 注册 containerd 服务
containerd.exe config default | Out-File "$Env:ProgramFiles\containerd\config.toml" -Encoding ASCII
& "$Env:ProgramFiles\containerd\containerd.exe" --register-service

# 启动 containerd 服务
Start-Service containerd
Set-Service -Name containerd -StartupType Automatic
```

### 安装 CNI 插件（以 Calico Windows 为例）

```powershell
# Calico Windows 安装（支持 BGP 和 VXLAN 模式）
# 1. 下载 Calico for Windows
Invoke-WebRequest -Uri "https://docs.tigera.io/files/calicoctl.exe" -OutFile "calicoctl.exe"

# 2. 配置 Calico 环境变量
$env:CALICO_NETWORKING_BACKEND = "vxlan"  # 或 "bgp"
$env:CALICO_DATASTORE_TYPE = "kubernetes"
$env:KUBECONFIG = "C:\k\config"

# 3. 执行安装脚本
.\install-calico-windows.ps1
```

## 节点加入集群

### 安装 kubelet 和 kubeadm（Windows）

```powershell
# 方法一：使用 Kubernetes Windows 节点脚本
# 下载预编译的 Windows 二进制
$K8S_VERSION = "v1.30.0"

Invoke-WebRequest -Uri "https://dl.k8s.io/release/$K8S_VERSION/bin/windows/amd64/kubelet.exe" -OutFile "C:\k\kubelet.exe"
Invoke-WebRequest -Uri "https://dl.k8s.io/release/$K8S_VERSION/bin/windows/amd64/kube-proxy.exe" -OutFile "C:\k\kube-proxy.exe"
Invoke-WebRequest -Uri "https://dl.k8s.io/release/$K8S_VERSION/bin/windows/amd64/kubeadm.exe" -OutFile "C:\k\kubeadm.exe"

# 配置 kubelet 服务参数
$kubeletArgs = @"
--config=C:\k\kubelet-config.yaml
--bootstrap-kubeconfig=C:\k\bootstrap-kubeconfig
--kubeconfig=C:\k\kubeconfig
--cert-dir=C:\var\lib\kubelet\pki
--node-labels=kubernetes.io/os=windows
"@

# 注册 kubelet 为 Windows 服务
New-Service -Name kubelet -BinaryPathName "C:\k\kubelet.exe $kubeletArgs" -StartupType Automatic
```

### 执行 kubeadm join

```powershell
# 从 Linux 控制面获取 join token
# （在 Linux 控制面执行）
# kubeadm token create --print-join-command

# 在 Windows 节点执行 join
C:\k\kubeadm.exe join <control-plane-ip>:6443 `
  --token <token> `
  --discovery-token-ca-cert-hash sha256:<hash> `
  --node-name windows-worker-1
```

## 调度隔离：防止 Linux 工作负载调度到 Windows 节点

### 为 Windows 节点添加 Taint

```bash
# 在 Linux 控制面执行
kubectl taint nodes windows-worker-1 \
  node.kubernetes.io/os=windows:NoSchedule

# 批量为所有 Windows 节点添加 taint
kubectl get nodes -l kubernetes.io/os=windows -o name | \
  xargs -I {} kubectl taint {} node.kubernetes.io/os=windows:NoSchedule
```

### Windows 工作负载配置（nodeSelector + toleration）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: iis-web
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: iis-web
  template:
    metadata:
      labels:
        app: iis-web
    spec:
      # 必须：指定调度到 Windows 节点
      nodeSelector:
        kubernetes.io/os: windows
        # 可选：指定 Windows Server 版本
        node.kubernetes.io/windows-build: "10.0.20348"  # Server 2022
      # 必须：容忍 Windows 节点的 taint
      tolerations:
      - key: node.kubernetes.io/os
        operator: Equal
        value: windows
        effect: NoSchedule
      containers:
      - name: iis
        image: mcr.microsoft.com/windows/servercore/iis:windowsservercore-ltsc2022
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: "2"
            memory: 2Gi
```

### 防止 Linux 工作负载调度到 Windows 节点

```yaml
# 为 Linux 工作负载添加 nodeSelector（推荐做法）
spec:
  nodeSelector:
    kubernetes.io/os: linux
```

## Windows 容器特有限制

| 特性 | Linux | Windows | 说明 |
|------|-------|---------|------|
| Privileged Container | 支持 | 不支持 | 用 HostProcess 替代 |
| HostNetwork | 支持 | 部分支持 | Windows 网络隔离模式不同 |
| Init Container | 支持 | v1.28+ 支持 | 需要较新版本 |
| CRI-O | 支持 | 不支持 | 仅 containerd |
| HostPath 挂载 | 支持 | 支持（需注意路径格式） | Windows 路径：`C:\data` |
| Linux 信号（SIGTERM） | 支持 | 不支持（用 CTRL_SHUTDOWN） | 影响优雅终止 |
| securityContext.runAsUser | 支持 | 不支持 | 使用 runAsUsername |

### Windows HostProcess Container（替代 privileged）

```yaml
# HostProcess Container 用于节点管理任务（类似 Linux privileged）
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: windows-node-agent
spec:
  selector:
    matchLabels:
      app: windows-node-agent
  template:
    metadata:
      labels:
        app: windows-node-agent
    spec:
      nodeSelector:
        kubernetes.io/os: windows
      tolerations:
      - key: node.kubernetes.io/os
        operator: Equal
        value: windows
        effect: NoSchedule
      securityContext:
        windowsOptions:
          hostProcess: true    # 启用 HostProcess 模式
          runAsUserName: "NT AUTHORITY\\System"
      hostNetwork: true
      containers:
      - name: agent
        image: myregistry/windows-agent:v1.0
```

## 实战示例

### 验证 Windows 节点状态

```bash
# 查看节点列表和 OS 标签
kubectl get nodes -o custom-columns=\
'NAME:.metadata.name,STATUS:.status.conditions[-1].type,OS:.metadata.labels.kubernetes\.io/os,VERSION:.status.nodeInfo.kubeletVersion'
```

```
NAME                STATUS   OS        VERSION
linux-master-1      Ready    linux     v1.30.0
linux-worker-1      Ready    linux     v1.30.0
windows-worker-1    Ready    windows   v1.30.0
windows-worker-2    Ready    windows   v1.30.0
```

### 检查 Windows 节点详细信息

```bash
kubectl describe node windows-worker-1 | grep -A5 "System Info"
```

```
System Info:
  OS Image:                    Windows Server 2022 Datacenter
  Operating System:            windows
  Architecture:                amd64
  Container Runtime Version:   containerd://1.7.13
  Kubelet Version:             v1.30.0
```

## 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| Windows Pod 无法调度 | 未设置 nodeSelector | 添加 `kubernetes.io/os: windows` |
| Pod 镜像拉取失败 | 镜像 OS 版本不匹配 | 确认镜像 Windows Server 版本与节点一致 |
| Windows 节点 NotReady | kubelet 服务未启动 | `Get-Service kubelet` 检查服务状态 |
| CNI 网络异常 | kube-proxy HNS 策略问题 | 重启 kube-proxy 并检查 HNS 规则 |
| HostProcess 权限不足 | 未设置 runAsUserName | 设置为 `NT AUTHORITY\\System` |

## 相关函数

- [`节点注册`](02-registration.md) — kubelet 注册流程，Windows 与 Linux 共用机制
- [`CNI 节点网络`](09-cni-node.md) — 网络插件配置，Windows 支持 Calico/Flannel
- [`节点安全`](13-security.md) — 节点安全加固，Windows 专有配置

## 版本说明

- Windows 容器自 Kubernetes v1.14 起进入 beta，v1.18+ 生产就绪
- HostProcess Container 自 v1.26 起 GA
- Init Container for Windows 自 v1.28 起 GA
- 基于 Kubernetes v1.28 – v1.32 文档

## Related

- [[entities/kubernetes.md|kubernetes]]
- [[entities/cni.md|cni]]
- [[entities/cri-o.md|CRI-O]]
- [[entities/containerd.md|containerd]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux.md|linux]]
