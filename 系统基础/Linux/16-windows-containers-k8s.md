---
title: "Windows 容器与 K8s 混合集群运维"
description: "Windows 节点加入 K8s 集群、RuntimeClass 配置、限制约束及混合集群运维实践"
summary: "系统讲解 Windows 容器在 Kubernetes 中的部署：Windows 节点加入集群流程、RuntimeClass 与节点选择、Windows 容器限制与约束、Linux/Windows 混合集群运维及故障排查"
category: 系统基础
tags:
- windows-containers
- mixed-cluster
- runtimeclass
- windows-node
- hybrid
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- "如何在 K8s 中运行 Windows 容器"
- "Windows 节点怎么加入 K8s 集群"
- "Windows 容器有什么限制"
trigger_keywords:
- windows-container
- windows-node
- mixed-cluster
- runtimeclass
- ltsc
prerequisites:
- kubectl-basics
- k8s-architecture
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Windows 容器与 K8s 混合集群

## 概述

尽管云原生生态以 Linux 为中心，但企业仍有大量 .NET Framework、IIS、Windows Service 等遗留工作负载需要容器化。Kubernetes 从 1.14 版本开始正式支持 Windows 节点，允许在同一集群中同时运行 Linux 和 Windows 容器。

Windows 容器与 Linux 容器存在根本性架构差异：Windows 容器依赖 Windows 内核（非 Linux 内核），不支持 Linux namespace 的全部特性，镜像体积大（基础镜像 4-15GB），且功能受限（无 privileged 容器、无 hostPath 等）。理解这些限制是成功运维混合集群的前提。

## 核心概念

### Windows 容器架构

```
Linux 容器：
  容器进程 → Linux Kernel（namespace + cgroup）

Windows 容器（Process Isolation）：
  容器进程 → Windows Kernel（Job Object + Silo）
  # 共享宿主内核，类似 Linux 容器

Windows 容器（Hyper-V Isolation）：
  容器进程 → Utility VM → Windows Kernel
  # 每个容器运行在轻量 VM 中，更强隔离
```

### Windows 容器 vs Linux 容器

| 维度 | Windows 容器 | Linux 容器 |
|------|------------|-----------|
| 内核 | Windows NT Kernel | Linux Kernel |
| 隔离机制 | Job Object + Silo / Hyper-V | namespace + cgroup |
| 基础镜像大小 | 4-15GB（Server Core）/ 1-2GB（Nano Server） | 5-200MB |
| 启动时间 | 10-60s | 1-5s |
| 特权模式 | 不支持 | 支持 |
| hostPath | 不支持 | 支持 |
| 网络模式 | NAT / L2Bridge / Transparent | bridge / host / overlay |
| 存储驱动 | Windows Filter | overlayfs / devicemapper |
| GPU 支持 | 有限（Windows GPU） | 完整 |
| 资源限制 | CPU/Memory（Job Object） | CPU/Memory/IO/PID |
| 日志 | Event Log + stdout | stdout/stderr |
| 适用场景 | .NET Framework, IIS, Windows Service | 通用 |

### Windows Server 版本兼容性

| Windows Server 版本 | 容器兼容性 | 说明 |
|-------------------|-----------|------|
| Windows Server 2019 LTSC | 进程隔离（同版本） | 长期支持，推荐生产 |
| Windows Server 2022 LTSC | 进程隔离（同版本）+ Hyper-V | 当前推荐 |
| Windows Server 2025 | 进程隔离 + Hyper-V | 最新版本 |
| Windows 11 (客户端) | 仅 Hyper-V 隔离 | 开发用 |

**关键约束**：Windows 容器的宿主版本必须与容器基础镜像版本匹配（或通过 Hyper-V 隔离兼容）。

## 生产部署

### Windows 节点加入集群

```powershell
# 🟡 中风险：Windows 节点加入 K8s 集群
# 在 Windows Server 2022 上执行（PowerShell 管理员）

# 1. 安装容器功能
Install-WindowsFeature -Name Containers -Restart

# 2. 安装 containerd
# 下载 containerd Windows 二进制
curl.exe -LO https://github.com/containerd/containerd/releases/download/v1.7.18/containerd-1.7.18-windows-amd64.tar.gz
tar xzf containerd-1.7.18-windows-amd64.tar.gz
mkdir -Force "$env:ProgramFiles\containerd"
Move-Item .\bin\* "$env:ProgramFiles\containerd\"

# 3. 配置 containerd
& "$env:ProgramFiles\containerd\containerd.exe" config default | Out-File "$env:ProgramFiles\containerd\config.toml" -Encoding ascii

# 4. 安装 kubelet 和 kubeadm
curl.exe -LO https://dl.k8s.io/v1.30.2/bin/windows/amd64/kubelet.exe
curl.exe -LO https://dl.k8s.io/v1.30.2/bin/windows/amd64/kubeadm.exe
Move-Item kubelet.exe, kubeadm.exe "C:\k\"

# 5. 加入集群
C:\k\kubeadm.exe join <api-server>:6443 --token <token> --discovery-token-ca-cert-hash sha256:<hash>

# 6. 启动 kubelet
Start-Service kubelet
```

### 节点标签与 RuntimeClass

```yaml
# 🟡 中风险：Windows 节点配置
# Windows 节点自动获得标签：
# kubernetes.io/os: windows
# kubernetes.io/arch: amd64
# node.kubernetes.io/windows-build: 10.0.20348

# 为 Windows 节点添加自定义标签
# kubectl label node win-node-01 workload-type=dotnet team=legacy-apps

# RuntimeClass 配置（Windows 容器）
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: windows-2022
handler: runhcs-wcow-process  # Windows process isolation
scheduling:
  nodeSelector:
    kubernetes.io/os: windows
    node.kubernetes.io/windows-build: "10.0.20348"
---
# Windows 工作负载 Pod
apiVersion: v1
kind: Pod
metadata:
  name: dotnet-app
  namespace: legacy-apps
spec:
  runtimeClassName: windows-2022
  nodeSelector:
    kubernetes.io/os: windows
  tolerations:
  - key: os
    operator: Equal
    value: windows
    effect: NoSchedule
  containers:
  - name: dotnet-app
    image: mcr.microsoft.com/dotnet/framework/aspnet:4.8-windowsservercore-ltsc2022
    ports:
    - containerPort: 80
    resources:
      requests:
        cpu: "2"
        memory: "4Gi"
      limits:
        cpu: "4"
        memory: "8Gi"
    volumeMounts:
    - name: config
      mountPath: C:\inetpub\wwwroot\Web.config
      subPath: Web.config
  volumes:
  - name: config
    configMap:
      name: dotnet-config
```

### 混合集群 Deployment 策略

```yaml
# 🟢 低风险：Linux/Windows 混合部署
# Linux 服务
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-gateway
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: api-gateway
  template:
    metadata:
      labels:
        app: api-gateway
    spec:
      nodeSelector:
        kubernetes.io/os: linux  # 明确指定 Linux
      containers:
      - name: gateway
        image: registry.example.com/api-gateway:v2
        resources:
          requests:
            cpu: "1"
            memory: "2Gi"
---
# Windows 服务（.NET Framework）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: legacy-reporting
  namespace: production
spec:
  replicas: 2
  selector:
    matchLabels:
      app: legacy-reporting
  template:
    metadata:
      labels:
        app: legacy-reporting
    spec:
      nodeSelector:
        kubernetes.io/os: windows  # 明确指定 Windows
      tolerations:
      - key: os
        operator: Equal
        value: windows
        effect: NoSchedule
      containers:
      - name: reporting
        image: registry.example.com/legacy-reporting:v5
        ports:
        - containerPort: 8080
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
          limits:
            cpu: "4"
            memory: "8Gi"
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 60  # Windows 容器启动慢
          periodSeconds: 30
          timeoutSeconds: 10
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 15
```

### Windows 容器网络配置

```yaml
# 🟡 中风险：Windows 容器网络
# Windows 支持的网络模式：
# 1. NAT（默认）：容器通过宿主 NAT 访问外部
# 2. L2Bridge：容器获得与宿主同子网 IP
# 3. Transparent：容器直接接入物理网络

# 使用 Calico 的 Windows 网络策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: windows-app-policy
  namespace: legacy-apps
spec:
  podSelector:
    matchLabels:
      app: legacy-reporting
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: api-gateway
    ports:
    - protocol: TCP
      port: 8080
---
# Windows 节点 Taint（防止 Linux Pod 误调度）
# 节点加入时自动添加：
# kubectl taint nodes win-node-01 os=windows:NoSchedule
```

## 运维操作

### 混合集群状态检查

```bash
# 🟢 低风险：混合集群状态
# 查看节点 OS 分布
kubectl get nodes -o custom-columns=\
NAME:.metadata.name,OS:.metadata.labels.kubernetes\\.io/os,ARCH:.metadata.labels.kubernetes\\.io/arch,STATUS:.status.conditions[-1].type

# 查看 Windows 节点详情
kubectl describe node win-node-01 | grep -A5 "System Info"

# 查看 Windows Pod
kubectl get pods -A -o wide --field-selector spec.nodeName=win-node-01

# 检查 Windows 容器运行时
kubectl get nodes win-node-01 -o jsonpath='{.status.nodeInfo.containerRuntimeVersion}'
# 应输出：containerd://1.7.x
```

### Windows 节点维护

```powershell
# 🟡 中风险：Windows 节点维护
# 排空 Windows 节点
kubectl drain win-node-01 --ignore-daemonsets --delete-emptydir-data

# Windows 更新（需要重启）
# 在 Windows 节点上：
Install-Module PSWindowsUpdate -Force
Get-WindowsUpdate -Install -AcceptAll -AutoReboot

# 恢复调度
kubectl uncordon win-node-01

# 验证节点状态
kubectl get node win-node-01
kubectl get pods -A --field-selector spec.nodeName=win-node-01
```

### 镜像管理

```powershell
# 🟢 低风险：Windows 容器镜像管理
# 拉取 Windows 基础镜像
crictl pull mcr.microsoft.com/windows/servercore:ltsc2022
crictl pull mcr.microsoft.com/windows/nanoserver:ltsc2022

# 查看本地镜像
crictl images

# 清理未使用镜像（Windows 镜像很大，注意磁盘空间）
crictl rmi --prune

# 检查磁盘使用
Get-PSDrive C | Select-Object Used, Free
```

## 故障排查

### Windows 容器常见问题

```bash
# 🟢 低风险：Windows 容器诊断
# 问题 1：Pod 一直 Pending
# 原因：没有 Windows 节点或 nodeSelector 不匹配
kubectl describe pod dotnet-app -n legacy-apps
# 检查 Events 中的调度失败原因

# 问题 2：镜像拉取失败
# 错误：failed to pull image: no match for platform
# 原因：镜像不支持 Windows 或版本不匹配
kubectl describe pod dotnet-app -n legacy-apps | grep -A5 "Events"

# 问题 3：容器启动超时
# Windows 容器启动慢（10-60s），需要增大 probe 超时
kubectl get pod dotnet-app -o yaml | grep -A10 "livenessProbe"

# 问题 4：网络不通
# 检查 Windows HNS 网络
# 在 Windows 节点上：
Get-HnsNetwork
Get-HnsEndpoint

# 问题 5：DNS 解析失败
kubectl exec -it dotnet-app -- nslookup kubernetes.default
# Windows 容器 DNS 配置在 C:\Windows\System32\drivers\etc\resolv.conf
```

### 性能问题

```bash
# 🟢 低风险：Windows 容器性能诊断
# Windows 容器资源使用
kubectl top pod dotnet-app -n legacy-apps

# 检查 Windows 节点资源
kubectl top node win-node-01

# 在 Windows 节点上检查进程
# PowerShell:
Get-Process | Sort-Object CPU -Descending | Select-Object -First 10

# 检查 Windows 事件日志
Get-EventLog -LogName Application -Newest 20 -EntryType Error
```

## 最佳实践

### 混合集群设计

1. **明确 OS 选择**：所有 Pod 必须设置 `nodeSelector: kubernetes.io/os`，避免调度到错误 OS 节点
2. **Windows 节点 Taint**：Windows 节点添加 `os=windows:NoSchedule` taint，防止 Linux DaemonSet 调度
3. **Probe 超时加大**：Windows 容器启动慢，`initialDelaySeconds` 至少 60s
4. **镜像瘦身**：使用 Nano Server 基础镜像（~1GB）替代 Server Core（~4GB），减少拉取时间
5. **存储限制**：Windows 容器不支持 hostPath，使用 PVC（Azure Disk/AWS EBS）或 emptyDir
6. **日志收集**：Windows 容器日志通过 stdout 输出，使用 Fluent Bit Windows 版本收集
7. **版本锁定**：Windows Server 版本与基础镜像版本严格匹配，避免兼容性问题
8. **参考 [[系统基础/Linux/08-linux-container-fundamentals|Linux 容器基础]] 理解容器原理差异**

### 迁移路径

- **.NET Framework → .NET 6/8**：如果可以迁移到 .NET 8（跨平台），则可以使用 Linux 容器，获得更好的性能和生态支持
- **渐进式迁移**：先将新服务用 Linux 容器，遗留 .NET Framework 服务用 Windows 容器
- **长期目标**：逐步将 Windows 容器工作负载迁移到 Linux（.NET 8 + Kestrel 替代 IIS）

## Related

- [[系统基础/Linux/08-linux-container-fundamentals|Linux 容器基础]]
- [[容器运行时/containerd-CRI-O/01-containerd-production-operations|containerd 生产运维]]
- [[集群基础/节点管理|节点管理]]
- [[网络/cni-plugins|CNI 插件]]
- [[平台工程/治理/17-multi-tenant-management|多租户管理]]
- [[系统基础/Linux/14-arm-architecture-k8s-optimization|ARM 架构优化]]
