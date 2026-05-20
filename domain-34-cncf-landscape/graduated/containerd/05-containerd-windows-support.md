---
title: containerd Windows 容器支持
description: '## 1. Windows 容器支持概述'
category: cncf-landscape
tags:
- k8s
- containerd
- windows
- windows-container
- hybrid-cluster
- runtime
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 平台工程师
- Windows 运维工程师
- SRE
estimated_read_time: 8min
intent_queries:
- containerd Windows 容器支持
- Kubernetes Windows 节点配置
- Windows 容器运行时 部署
trigger_keywords:
- containerd Windows
- Windows 容器
- Kubernetes Windows
---

# containerd Windows 容器支持

> **版本**: v1.0 | **适用版本**: containerd 1.7+ / 2.0 | **最后更新**: 2026-05

---

## 1. Windows 容器支持概述

### 1.1 背景

Kubernetes 从 1.14 开始正式支持 Windows 节点作为 worker 节点，允许运行 Windows 容器化工作负载。这对于运行 .NET Framework、IIS、SQL Server 等 Windows 专属应用的企业至关重要。

### 1.2 架构对比

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         Linux vs Windows 节点架构                                 │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  Linux 节点                          Windows 节点                               │
│  ──────────                          ─────────────                              │
│                                                                                  │
│  ┌──────────────────┐              ┌──────────────────┐                        │
│  │    kubelet       │              │    kubelet       │                        │
│  │   (Linux)        │              │   (Windows)      │                        │
│  └────────┬─────────┘              └────────┬─────────┘                        │
│           │ CRI                               │ CRI                            │
│           ▼                                   ▼                                 │
│  ┌──────────────────┐              ┌──────────────────┐                        │
│  │   containerd     │              │   containerd     │                        │
│  │   (Linux)        │              │   (Windows)      │                        │
│  └────────┬─────────┘              └────────┬─────────┘                        │
│           │ OCI                              │ OCI                             │
│           ▼                                   ▼                                 │
│  ┌──────────────────┐              ┌──────────────────┐                        │
│  │   runc            │              │   runhcs          │                        │
│  │   (Linux)         │              │   (Windows)       │                        │
│  └──────────────────┘              └──────────────────┘                        │
│           │                                   │                                 │
│           ▼                                   ▼                                 │
│  ┌──────────────────┐              ┌──────────────────┐                        │
│  │  Linux Container  │              │ Windows Container │                        │
│  │  (Namespaces/Cgroups)│           │ (Windows Job Objects)│                     │
│  └──────────────────┘              └──────────────────┘                        │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.3 支持矩阵

| 功能 | Linux | Windows | 说明 |
|------|-------|---------|------|
| **containerd 运行** | ✅ | ✅ | 1.7+ 支持 Windows |
| **Kubernetes Pod** | ✅ | ✅ | Linux/Windows 混合 |
| **Host Networking** | ✅ | ✅ |  |
| **emptyDir** | ✅ | ✅ |  |
| **ConfigMap/Secret** | ✅ | ✅ |  |
| **持久卷 (PVC)** | ✅ | ✅ (部分) | 需要 CSI 驱动 |
| **Security Context** | ✅ | ✅ (部分) | RunAsUser/Privilege 限制 |
| **Resource Limits** | ✅ | ✅ | CPU/Memory |
| **Port Forwarding** | ✅ | ✅ |  |
| **Exec** | ✅ | ✅ |  |
| **Logs** | ✅ | ✅ |  |

---

## 2. Windows 节点配置

### 2.1 系统要求

| 要求 | 最小值 | 推荐值 |
|------|--------|--------|
| **OS** | Windows Server 2019 (1809) | Windows Server 2022 |
| **CPU** | 4 核 | 8 核+ |
| **内存** | 8 GB | 16 GB+ |
| **磁盘** | 100 GB | 200 GB+ |
| **Kubernetes** | 1.14+ | 1.27+ |

### 2.2 安装 containerd (Windows)

```powershell
# 1. 下载 containerd
$version = "1.7.8"
$url = "https://github.com/containerd/containerd/releases/download/v$version/containerd-$version-windows-amd64.tar.gz"
Invoke-WebRequest -Uri $url -OutFile containerd.zip

# 2. 解压
tar -xvf containerd.zip -C C:\

# 3. 下载 runhcs (Windows OCI runtime)
$runhcs_version = "1.0.0"
$runhcs_url = "https://github.com/Microsoft/hcss-run/releases/download/v$runhcs_version/runhcs.exe"
Invoke-WebRequest -Uri $runhcs_url -OutFile C:\containerd\bin\runhcs.exe

# 4. 配置 containerd
containerd config default > C:\containerd\config.toml

# 5. 配置 Windows 特定参数
# 编辑 config.toml 添加:
[plugins."io.containerd.grpc.v1.cri"]
  sandbox_image = "mcr.microsoft.com/oss/kubernetes/pause:3.9"
  
  [plugins."io.containerd.grpc.v1.cri".containerd]
    default_runtime_name = "windows"
    snapshotter = "windows"
    
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.windows]
      runtime_type = "io.containerd.runhcs.v1"
      snapshotter = "windows"
      
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.windows.options]
        RuntimePath = "C:\\containerd\\bin\\runhcs.exe"
        RuntimeEngine = ""
        RuntimeRoot = "C:\\containerd\\runtime-root"
        
# 6. 注册服务
sc.exe create containerd binPath= "C:\containerd\bin\containerd.exe --config C:\containerd\config.toml" start= auto

# 7. 启动服务
sc.exe start containerd
```

### 2.3 Windows CRI 配置

```toml
# C:\containerd\config.toml (Windows 完整配置)
version = 2

root = "C:\\containerd"
state = "C:\\containerd\\state"

[grpc]
  address = "\\\\.\\pipe\\containerd-containerd.sock"
  max_recv_message_size = 16777216
  max_send_message_size = 16777216

[plugins]
  [plugins."io.containerd.grpc.v1.cri"]
    sandbox_image = "mcr.microsoft.com/oss/kubernetes/pause:3.9"
    max_container_log_line_size = 16384
    
    # Windows 特定配置
    [plugins."io.containerd.grpc.v1.cri".containerd]
      default_runtime_name = "windows"
      snapshotter = "windows"
      
      # Windows runtimes
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.windows]
        runtime_type = "io.containerd.runhcs.v1"
        snapshotter = "windows"
        
        [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.windows.options]
          RuntimePath = "C:\\containerd\\bin\\runhcs.exe"
          RuntimeEngine = "process"
          RuntimeRoot = "C:\\containerd\\runtime-root"
          
          # Windows 隔离级别
          Isolation = "process"  # 或 "hyperv"
          
      # Hyper-V 隔离 (更高安全性)
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.hyperv]
        runtime_type = "io.containerd.runhcs.v1"
        snapshotter = "windows"
        
        [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.hyperv.options]
          RuntimePath = "C:\\containerd\\bin\\runhcs.exe"
          RuntimeEngine = "hcs"
          RuntimeRoot = "C:\\containerd\\runtime-root"
          Isolation = "hyperv"

[metrics]
  address = "127.0.0.1:1338"
```

---

## 3. Kubernetes Windows 节点配置

### 3.1 节点标签与选择器

```yaml
# 使用 nodeSelector 选择 Windows 节点
apiVersion: v1
kind: Pod
metadata:
  name: windows-webapp
spec:
  nodeSelector:
    kubernetes.io/os: windows
  containers:
  - name: webapp
    image: mcr.microsoft.com/windows/servercore:ltsc2022
    command: ["cmd", "/c", "ping -t localhost"]
```

### 3.2 Node 亲和性配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: windows-webapp-affinity
spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: kubernetes.io/os
            operator: In
            values:
            - windows
    # 优先调度到 Windows 节点
    preferredDuringSchedulingIgnoredDuringExecution:
    - weight: 100
      preference:
        matchExpressions:
        - key: node.kubernetes.io/windows-d拉
          operator: Exists
  containers:
  - name: webapp
    image: mcr.microsoft.com/windows/servercore:ltsc2022
```

### 3.3 污点容忍

```yaml
# 允许 Windows Pod 调度到带有特定污点的节点
apiVersion: v1
kind: Pod
metadata:
  name: windows-webapp-toleration
spec:
  tolerations:
  - key: "node.kubernetes.io/os"
    operator: "In"
    value: "windows"
    effect: "NoSchedule"
  containers:
  - name: webapp
    image: mcr.microsoft.com/windows/servercore:ltsc2022
```

---

## 4. Windows 容器镜像

### 4.1 基础镜像类型

| 镜像 | 用途 | 标签 |
|------|------|------|
| **mcr.microsoft.com/windows/servercore** | .NET 应用 | ltsc2022, ltsc2019 |
| **mcr.microsoft.com/windows/nanoserver** | 轻量容器 | ltsc2022, ltsc2019 |
| **mcr.microsoft.com/aspnet** | ASP.NET | 4.8, 6.0, 7.0 |
| **mcr.microsoft.com/dotnet/sdk** | .NET SDK | 6.0, 7.0 |
| **mcr.microsoft.com/dotnet/framework/sdk** | .NET Framework | 4.8 |

### 4.2 镜像拉取配置

```yaml
# Pod 使用 Windows 镜像
apiVersion: v1
kind: Pod
metadata:
  name: windows-app
spec:
  containers:
  - name: app
    image: mcr.microsoft.com/windows/servercore:ltsc2022
    env:
    - name: CONTAINER_SANDBOX
      value: "true"
    resources:
      limits:
        memory: "512Mi"
        cpu: "500m"
      requests:
        memory: "256Mi"
        cpu: "250m"
```

### 4.3 私有仓库认证

```toml
# Windows containerd 私有仓库配置
# C:\containerd\certs.d\my-registry\hosts.toml
server = "https://my-registry.example.com"

[host."https://my-registry.example.com"]
  capabilities = ["pull", "resolve"]
  ca = "C:\\containerd\\certs.d\\my-registry\\ca.crt"
  client = [
    ["C:\\containerd\\certs.d\\my-registry\\client.crt", "C:\\containerd\\certs.d\\my-registry\\client.key"]
  ]
```

---

## 5. Windows 隔离模式

### 5.1 Process 隔离 (默认)

```yaml
# 使用进程隔离
apiVersion: v1
kind: Pod
metadata:
  name: windows-process-isolated
spec:
  containers:
  - name: app
    image: mcr.microsoft.com/windows/servercore:ltsc2022
  runtimeClassName: "process"
```

### 5.2 Hyper-V 隔离

```yaml
# 使用 Hyper-V 隔离 (更高安全性)
apiVersion: v1
kind: Pod
metadata:
  name: windows-hyperv-isolated
spec:
  containers:
  - name: app
    image: mcr.microsoft.com/windows/servercore:ltsc2022
  runtimeClassName: "hyperv"
```

### 5.3 RuntimeClass 配置

```yaml
# Kubernetes RuntimeClass 定义
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: process
handler: windows
scheduling:
  nodeSelector:
    kubernetes.io/os: windows
---
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: hyperv
handler: hyperv
scheduling:
  nodeSelector:
    kubernetes.io/os: windows
  # 仅在支持 Hyper-V 的节点上调度
  tolerations:
  - key: "virtualization"
    operator: "Exists"
    effect: "NoSchedule"
```

---

## 6. 混合集群管理

### 6.1 统一管理 Linux 和 Windows

```bash
# 查看混合集群节点
kubectl get nodes -l kubernetes.io/os=windows
kubectl get nodes -l kubernetes.io/os=linux

# 查看 Windows Pod
kubectl get pods -n windows-app --field-selector spec.nodeName=<windows-node>

# 所有节点状态
kubectl get nodes -o wide
```

### 6.2 资源配额跨平台

```yaml
# ResourceQuota 应用于混合集群
apiVersion: v1
kind: ResourceQuota
metadata:
  name: windows-quota
  namespace: windows-app
spec:
  hard:
    pods: "50"
    requests.cpu: "10"
    requests.memory: "20Gi"
    limits.cpu: "20"
    limits.memory: "40Gi"
```

### 6.3 网络配置

```yaml
# Windows Pod 网络配置
apiVersion: v1
kind: Pod
metadata:
  name: windows-webapp
  annotations:
    k8s.v1.cni.cncf.io/networks: '[{"name":"internal"},{"name":"external"}]'
spec:
  containers:
  - name: webapp
    image: mcr.microsoft.com/windows/servercore:ltsc2022
    ports:
    - containerPort: 80
      hostPort: 8080
```

---

## 7. 故障排查

### 7.1 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| **Windows Pod 无法调度** | 无 Windows 节点 | 检查节点标签 |
| **镜像拉取失败** | Windows 版本不匹配 | 确认 OS 版本与镜像 tag 匹配 |
| **容器启动失败** | 隔离模式不兼容 | 检查 RuntimeClass 配置 |
| **网络不通** | CNI 配置错误 | 检查 Windows CNI 插件 |

### 7.2 诊断命令

```powershell
# 检查 containerd 状态
Get-Service containerd

# 检查 containerd 日志
Get-EventLog -LogName Application -Source containerd -Newest 50

# 查看运行中的容器
containerd.exe ps

# 检查 Windows 容器状态
Get-Container | Format-Table Name, Image, State

# 查看 Hyper-V 隔离
Get-VM -Container
```

### 7.3 日志收集

```powershell
# 收集 containerd 日志
$logs = @()
$logs += Get-EventLog -LogName Application -Newest 100 | Where-Object {$_.Source -eq "containerd"}
$logs += Get-EventLog -LogName System -Newest 50 | Where-Object {$_.Source -eq "containerd"}
$logs | Export-Csv -Path "C:\containerd-logs.csv"

# 收集容器运行时信息
containerd.exe info > C:\containerd-info.txt
```

---

## 8. 生产最佳实践

### 8.1 版本匹配

| Kubernetes 版本 | Windows Server 版本 | containerd 版本 |
|-----------------|---------------------|----------------|
| 1.26+ | Windows Server 2022 | 1.7+ |
| 1.23-1.25 | Windows Server 2019 LTSC | 1.5+ |
| 1.14-1.22 | Windows Server 2019 | 1.3+ |

### 8.2 存储优化

```powershell
# Windows 容器存储配置
# 在 containerd config.toml 中配置
[plugins."io.containerd.grpc.v1.cri".containerd]
  # Windows 使用 WindowsLayer 快照器
  snapshotter = "windows"

[plugins."io.containerd.snapshotter.v1.windows"]
  # 存储位置
  root = "C:\\containerd\\windows-layer"
  
  # 层存储优化
  layers = ["C:\\containerd\\layers"]
```

### 8.3 安全加固

```toml
# Windows 容器安全配置
[plugins."io.containerd.grpc.v1.cri"]
  # 禁用 privileged 容器
  enable_tlb = false
  
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.windows]
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.windows.options]
      # 安全选项
      CredentialSpec = ""  # Active Directory gMSA
      AdminPassword = ""   # 容器管理员密码
      ConsoleSize = ""     # 控制台大小限制
```

### 8.4 监控配置

```yaml
# Windows containerd 指标采集
apiVersion: v1
kind: Pod
metadata:
  name: containerd-metrics
spec:
  nodeSelector:
    kubernetes.io/os: windows
  containers:
  - name: metrics
    image: mcr.microsoft.com/windows/servercore:ltsc2022
    command: ["powershell", "-Command", "while($true){ Start-Sleep 5 }"]
```

---

## 9. 迁移指南

### 9.1 从 Docker Desktop 迁移

```powershell
# Docker Desktop 使用 Docker 作为 runtime，迁移到 containerd 需要：

# 1. 停止 Docker Desktop
Stop-Service docker

# 2. 安装 containerd for Windows
# (参考 2.2 节安装步骤)

# 3. 迁移容器数据
Copy-Item -Path "C:\ProgramData\docker" -Destination "C:\containerd-data" -Recurse

# 4. 启动 containerd
Start-Service containerd

# 5. 验证
containerd.exe info
```

### 9.2 镜像兼容性

```powershell
# 检查镜像兼容性
# Windows 容器镜像必须在匹配的 Windows 版本上运行

# 查看容器基础镜像
docker inspect <image> | Select-String -Pattern "OsVersion"

# 跨版本拉取会失败，需要重新构建
docker build -t my-app:ltsc2022 -f Dockerfile.win .
```

---

**维护者**: Kudig Team | **许可证**: MIT