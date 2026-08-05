---
title: containerd Windows 支持
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- runtime
- 05-containerd-windows-support
- containerd
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd Windows 支持 是什么
- 如何 containerd Windows 支持
trigger_keywords:
- containerd
- Windows
- 支持
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# containerd Windows 支持

> **CNCF 状态**: Graduated | **类别**: Runtime | **主要语言**: Go

## 概述

Containerd Windows 支持是 containerd 运行时的核心功能之一，使 containerd 能够在 Windows Server 2019/2022 上运行 Windows 容器。随着 Kubernetes 在 Windows 工作负载场景中的需求增长，containerd 从 v1.6 开始全面支持 Windows 平台，替代了之前的 Docker EE 作为 K8s 的默认容器运行时。Windows 支持包括 Windows 进程隔离容器和 Hyper-V 隔离容器两种模式，支持 .NET 应用、IIS、SQL Server 等 Windows 原生工作负载在 K8s 中运行。

## Key Features（核心能力）

- **Windows 进程隔离**：支持 Windows Server Containers（进程级隔离），轻量高效
- **Hyper-V 隔离**：通过 Hyper-V 虚拟化提供更强的隔离边界，兼容不同内核版本
- **GMSA 支持**：支持 Group Managed Service Accounts 实现 Active Directory 域认证
- **RunHCS 运行时**：基于 Windows Host Compute Service (HCS) 的运行时实现
- **镜像格式兼容**：支持 Docker 镜像格式和 OCI 镜像格式
- **网络支持**：集成 Windows CNI 插件，支持 overlay 网络和 L2bridge 网络

## 架构与工作原理

Containerd 在 Windows 上的架构与 Linux 类似，但使用 runhcs 替代 runc 作为低层运行时。runhcs 通过 Windows HCS (Host Compute Service) API 创建和管理容器。containerd-shim-runhcs-v1 作为 shim 进程，负责容器进程的生命周期管理。镜像层通过 Windows Container Storage (WCStorage) 管理，支持 NTFS 和 SAS 磁盘作为容器存储后端。

## K8s 集成

在 Kubernetes 中，containerd 通过 CRI (Container Runtime Interface) 与 kubelet 交互。Windows 节点需要运行 kubelet、kube-proxy 和 containerd，通过 taint/toleration 机制将 Windows Pod 调度到 Windows 节点。Pod 网络通过 CNI 插件（如 Calico for Windows、Antrea）配置，Service 和 Ingress 支持 Windows 兼容的代理规则。

## 生产用例

- **Windows 遗留应用现代化**：将 ASP.NET、WCF 等 Windows 应用迁移到 K8s 平台
- **混合 Linux/Windows 集群**：在同一集群中运行 Linux 和 Windows 工作负载
- **SQL Server 容器化**：在 K8s 中运行容器化 SQL Server 实例
- **CI/CD 构建节点**：提供 Windows 容器化的构建和测试环境

## 安装与配置

### Windows Server 2022 安装 containerd

```powershell
# 🟢 下载并安装 containerd
$Version = "1.7.22"
curl.exe -L "https://github.com/containerd/containerd/releases/download/v$Version/containerd-$Version-windows-amd64.tar.gz" -o containerd.tar.gz
tar -xzf containerd.tar.gz
mkdir -Force "$env:ProgramFiles\containerd"
Move-Item -Force .\bin\* "$env:ProgramFiles\containerd"

# 🟢 生成默认配置
& "$env:ProgramFiles\containerd\containerd.exe" config default | Out-File "$env:ProgramFiles\containerd\config.toml" -Encoding ascii

# 🟢 注册并启动服务
& "$env:ProgramFiles\containerd\containerd.exe" --register-service
Start-Service containerd

# 🟢 验证运行状态
Get-Service containerd
ctr.exe version
```

### containerd Windows 配置 (config.toml)

```toml
version = 2
root = "C:\\ProgramData\\containerd\\root"
state = "C:\\ProgramData\\containerd\\state"

[plugins."io.containerd.grpc.v1.cri"]
  sandbox_image = "registry.k8s.io/pause:3.9"
  [plugins."io.containerd.grpc.v1.cri".containerd]
    default_runtime_name = "runhcs-wcow-process"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runhcs-wcow-process]
      runtime_type = "io.containerd.runhcs.v1"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runhcs-wcow-process.options]
        Debug = false
        # Hyper-V 隔离（更强安全性）
        # SandboxIsolation = 1
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runhcs-wcow-hyperv]
      runtime_type = "io.containerd.runhcs.v1"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runhcs-wcow-hyperv.options]
        SandboxIsolation = 1
  [plugins."io.containerd.grpc.v1.cri".cni]
    bin_dir = "C:\\k\\cni\\bin"
    conf_dir = "C:\\k\\cni\\conf"
```

### K8s Windows 节点加入集群

```powershell
# 🟢 安装 kubelet 和 kube-proxy
$K8sVersion = "1.30.0"
curl.exe -L "https://dl.k8s.io/v$K8sVersion/bin/windows/amd64/kubelet.exe" -o "$env:ProgramFiles\containerd\kubelet.exe"
curl.exe -L "https://dl.k8s.io/v$K8sVersion/bin/windows/amd64/kubeproxy.exe" -o "$env:ProgramFiles\containerd\kubeproxy.exe"

# 🟡 注册 kubelet 服务
kubelet.exe --register-service --config=C:\k\kubelet-config.yaml --container-runtime-endpoint=npipe://./pipe/containerd-containerd

# 🟢 验证节点加入
kubectl get nodes -o wide
kubectl get nodes -l kubernetes.io/os=windows
```

### RuntimeClass 配置

```yaml
# Windows 进程隔离（默认）
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: windows-process
handler: runhcs-wcow-process
---
# Windows Hyper-V 隔离
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: windows-hyperv
handler: runhcs-wcow-hyperv
---
# 使用 RuntimeClass 的 Pod
apiVersion: v1
kind: Pod
metadata:
  name: iis-app
spec:
  runtimeClassName: windows-process
  nodeSelector:
    kubernetes.io/os: windows
  containers:
  - name: iis
    image: mcr.microsoft.com/windows/servercore/iis:ltsc2022
    ports:
    - containerPort: 80
    resources:
      requests:
        memory: "2Gi"
        cpu: "1"
      limits:
        memory: "4Gi"
        cpu: "2"
```

## 运维操作

```bash
# 🟢 检查 Windows 节点状态
kubectl get nodes -l kubernetes.io/os=windows -o wide
kubectl describe node <windows-node> | findstr "Conditions"

# 🟢 查看 containerd 服务日志（Windows）
Get-WinEvent -LogName "Microsoft-Windows-Containers*" -MaxEvents 50
journalctl -u containerd  # 若使用 Linux 风格日志

# 🟢 检查容器运行时状态
ctr.exe containers ls
ctr.exe tasks ls
ctr.exe images ls

# 🟡 重启 containerd 服务（会中断节点上所有容器）
Restart-Service containerd -Force

# 🟢 检查 CNI 网络配置
Get-Content C:\k\cni\conf\*.conf
Get-HnsNetwork | Format-Table Name, Type, Subnets
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 处于 ContainerCreating | CNI 配置错误/网络未就绪 | `Get-HnsNetwork`; `ctr.exe tasks ls` | 检查 CNI conf 目录、重启 HNS |
| 镜像拉取失败 | Windows 版本不匹配 | `ctr.exe images pull <img>` | 确认镜像 tag 匹配主机版本 |
| kubelet NotReady | containerd 服务停止 | `Get-Service containerd` | `Restart-Service containerd` |
| Hyper-V 容器启动失败 | Hyper-V 功能未启用 | `Get-WindowsFeature Hyper-V` | `Install-WindowsFeature Hyper-V` |
| GMSA 认证失败 | 凭据规格配置错误 | `Get-ADServiceAccount` | 重新生成 CredentialSpec |

### 排查流程

```
Windows Pod 异常
├── ContainerCreating 超时？
│   ├── 检查 CNI: Get-HnsNetwork
│   ├── 检查镜像: ctr.exe images ls | findstr <image>
│   └── 检查 containerd: Get-Service containerd
├── CrashLoopBackOff？
│   ├── kubectl logs <pod> → 应用错误
│   └── 检查 Windows 版本兼容性
└── 节点 NotReady？
    ├── kubelet 日志: Get-WinEvent -LogName "Microsoft-Windows-Kubelet*"
    └── containerd 状态: ctr.exe version
```

## 生产案例

### 案例1：混合集群 Windows 节点网络不通

- **场景**：Windows Pod 无法访问 Linux Service，DNS 解析正常但 TCP 连接超时
- **排查**：`Get-HnsNetwork` 发现 overlay 网络未正确创建；CNI 配置中 clusterCIDR 与 Linux 节点不匹配
- **方案**：修正 Calico CNI 配置，确保 Windows/Linux 使用相同的 IP Pool 和 VXLAN 设置
- **效果**：跨平台 Pod 通信恢复正常

### 案例2：Windows 容器镜像版本不兼容

- **场景**：在 Windows Server 2022 节点上拉取 ltsc2019 镜像后容器启动失败
- **排查**：`ctr.exe tasks start` 报 "kernel version mismatch"；进程隔离要求容器与主机内核版本一致
- **方案**：统一使用 ltsc2022 镜像，对必须使用旧版镜像的场景启用 Hyper-V 隔离
- **效果**：通过 RuntimeClass 区分隔离模式，兼容不同版本需求

## 对比替代方案

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| containerd + runhcs | 轻量、CRI原生集成、CNCF标准 | Windows专属功能仍在演进 | K8s 标准部署 |
| Docker EE (Mirantis) | 成熟稳定、企业支持 | 已停止K8s集成、额外许可费 | 遗留环境 |
| Hyper-V VM | 完全隔离、无版本限制 | 启动慢、资源开销大 | 强隔离需求 |
| WSL2 容器 | 开发体验好 | 不支持生产、无K8s集成 | 本地开发 |

## 检查清单

- [ ] Windows Server 版本与 K8s 版本兼容（2019/2022）
- [ ] containerd 服务正常运行且配置正确
- [ ] CNI 插件已安装且配置与 Linux 节点一致
- [ ] RuntimeClass 已创建（process/hyperv）
- [ ] 节点 taint/toleration 配置正确
- [ ] 镜像版本与主机 Windows 版本匹配
- [ ] GMSA 凭据规格已配置（如需 AD 认证）
- [ ] PodDisruptionBudget 保护关键 Windows 工作负载

## Related

- [[microcks]] — Microcks
- [[keylime]] — Keylime
- [[openebs]] — OpenEBS
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 05-containerd-windows-support


<!-- risk-assessed -->
