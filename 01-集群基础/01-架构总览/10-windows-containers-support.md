---
title: 10 - Windows 容器支持与集成指南
description: '# 10 - Windows 容器支持与集成指南'
summary: 'node.kubernetes.io/windows-build: "10.0.17763"'
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- flannel
- calico
- containerd
- docker
- hpa
- statefulset
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Windows 容器支持与集成指南 是什么
- 如何 Windows 容器支持与集成指南
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- Windows
- 容器支持与集成指南
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- cni-basics
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
  path: ../容器运行时/
  label: '相关知识域: 容器运行时'
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
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




# 10 - Windows 容器支持与集成指南

<!-- chunk: Windows节点要求 -->
## Windows节点要求

| 要求 | 最低版本 | 说明 |
|-----|---------|------|
| Windows Server | 2019 LTSC | 长期支持版本 |
| Windows Server | 2022 LTSC | 推荐版本 |
| Docker/containerd | [[containerd|containerd]] 1.6+ | 容器运行时 |
| [[kubernetes\|Kubernetes]] | v1.22+ | 稳定支持 |

<!-- chunk: Windows容器类型 -->
## Windows容器类型

| 类型 | 隔离方式 | 性能 | 兼容性 |
|-----|---------|------|-------|
| Process | 进程隔离 | 高 | 需版本匹配 |
| Hyper-V | 虚拟机隔离 | 中 | 更好兼容性 |

<!-- chunk: 支持的功能对比 -->
## 支持的功能对比

| 功能 | Linux | Windows | 说明 |
|-----|-------|---------|------|
| Pod | ✅ | ✅ | 完全支持 |
| [[service\|Service]] | ✅ | ✅ | ClusterIP/NodePort/LB |
| Deployment | ✅ | ✅ | 完全支持 |
| StatefulSet | ✅ | ✅ | 完全支持 |
| DaemonSet | ✅ | ✅ | 完全支持 |
| ConfigMap/Secret | ✅ | ✅ | 完全支持 |
| PersistentVolume | ✅ | ✅ | 部分CSI驱动 |
| hostPath | ✅ | ✅ | 路径格式不同 |
| emptyDir | ✅ | ✅ | 完全支持 |
| hostNetwork | ✅ | ❌ | 不支持 |
| hostPID | ✅ | ❌ | 不支持 |
| privileged | ✅ | ❌ | 不支持 |
| runAsUser | ✅ | ⚠️ | 有限支持 |
| seccomp | ✅ | ❌ | 不支持 |
| AppArmor | ✅ | ❌ | 不支持 |
| ResourceQuota | ✅ | ✅ | 完全支持 |
| HPA | ✅ | ✅ | 完全支持 |

<!-- chunk: Windows节点配置 -->
## Windows节点配置

```yaml
apiVersion: v1
kind: Node
metadata:
  labels:
    kubernetes.io/os: windows
    node.kubernetes.io/windows-build: "10.0.17763"
spec:
  taints:
  - key: os
    value: windows
    effect: NoSchedule
```

<!-- chunk: Windows Pod配置 -->
## Windows Pod配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: windows-app
spec:
  nodeSelector:
    kubernetes.io/os: windows
  tolerations:
  - key: os
    value: windows
    effect: NoSchedule
  containers:
  - name: iis
    image: mcr.microsoft.com/windows/servercore/iis:windowsservercore-ltsc2022
    ports:
    - containerPort: 80
    resources:
      limits:
        cpu: "2"
        memory: 2Gi
      requests:
        cpu: "1"
        memory: 1Gi
```

<!-- chunk: Windows Service配置 -->
## Windows Service配置

```yaml
apiVersion: v1
kind: Service
metadata:
  name: windows-service
spec:
  type: LoadBalancer
  selector:
    app: windows-app
  ports:
  - port: 80
    targetPort: 80
```

<!-- chunk: 混合集群部署 -->
## 混合集群部署

```yaml
# Linux工作负载
apiVersion: apps/v1
kind: Deployment
metadata:
  name: linux-backend
spec:
  selector:
    matchLabels:
      app: backend
  template:
    metadata:
      labels:
        app: backend
    spec:
      nodeSelector:
        kubernetes.io/os: linux
      containers:
      - name: api
        image: myapi:latest
---
# Windows工作负载
apiVersion: apps/v1
kind: Deployment
metadata:
  name: windows-frontend
spec:
  selector:
    matchLabels:
      app: frontend
  template:
    metadata:
      labels:
        app: frontend
    spec:
      nodeSelector:
        kubernetes.io/os: windows
      tolerations:
      - key: os
        value: windows
        effect: NoSchedule
      containers:
      - name: aspnet
        image: mcr.microsoft.com/dotnet/aspnet:6.0-windowsservercore-ltsc2022
```

<!-- chunk: Windows CNI支持 -->
## Windows CNI支持

| CNI | 支持版本 | 网络模式 |
|-----|---------|---------|
| Flannel | v0.14+ | overlay/host-gw |
| Calico | v3.12+ | vxlan/BGP |
| Azure CNI | - | Azure原生 |

<!-- chunk: Windows存储支持 -->
## Windows存储支持

| 存储类型 | 支持 | 说明 |
|---------|-----|------|
| emptyDir | ✅ | 完全支持 |
| hostPath | ✅ | Windows路径格式 |
| Azure Disk | ✅ | CSI驱动 |
| Azure File | ✅ | SMB协议 |
| AWS EBS | ⚠️ | 有限支持 |
| 本地PV | ✅ | 完全支持 |

<!-- chunk: Windows镜像选择 -->
## Windows镜像选择

| 基础镜像 | 大小 | 用途 |
|---------|-----|------|
| nanoserver | ~100MB | 最小镜像,.NET Core |
| servercore | ~2GB | 完整Windows Server |
| windows | ~4GB | 完整桌面体验 |

<!-- chunk: Windows调试命令 -->
## Windows调试命令

``` powershell
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看Windows节点
kubectl get nodes -l kubernetes.io/os=windows

# 进入Windows Pod
kubectl exec -it <pod-name> -- powershell

# 查看容器日志
kubectl logs <pod-name>

# 查看Windows事件
Get-EventLog -LogName Application -Newest 50
```
<!-- chunk: ACK Windows节点池 -->
## ACK Windows节点池

| 功能 | 支持 |
|-----|------|
| Windows 2019 | ✅ |
| Windows 2022 | ✅ |
| 弹性伸缩 | ✅ |
| 混合集群 | ✅ |

<!-- chunk: 版本变更记录 -->
## 版本变更记录

| 版本 | 变更内容 |
|------|---------|
| v1.25 | Windows HostProcess容器GA |
| v1.26 | Windows特权容器改进 |
| v1.27 | Windows CSI代理改进 |
| v1.28 | Windows网络策略增强 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 MOC
- [[01-集群基础/README.md|Domain-1: Kubernetes架构基础]]
- Domain-1 架构基础 — 开源项目索引
- Kubernetes 架构全景图
- Kubernetes 核心组件深度剖析
- 03 - 功能和API表
- 04 - Kubernetes 源码结构深度解析
- kubectl 命令完整参考
- 06 - 集群配置参数完全参考
- 07 - 升级路径与策略指南
- 08 - 多租户架构设计 (Multi-Tenancy Architecture)
- 09 - 边缘计算集成架构 (KubeEdge/OpenYurt)

## Windows HostProcess 容器

### 概述与用途

HostProcess 容器是 Windows 上的特权容器替代方案（v1.26 GA），用于运行需要主机访问的系统级工作负载。

```yaml
# HostProcess 容器 — 类似 Linux 特权容器
apiVersion: v1
kind: Pod
metadata:
  name: windows-monitoring-agent
  namespace: monitoring
spec:
  securityContext:
    windowsOptions:
      hostProcess: true      # 启用 HostProcess
      runAsUserName: "NT AUTHORITY\\SYSTEM"
  hostNetwork: true          # HostProcess 必须 hostNetwork
  nodeSelector:
    kubernetes.io/os: windows
  tolerations:
    - key: os
      value: windows
      effect: NoSchedule
  containers:
    - name: agent
      image: registry.internal/monitoring/windows-agent:2.1
      securityContext:
        windowsOptions:
          hostProcess: true
      volumeMounts:
        - name: host-fs
          mountPath: /host
  volumes:
    - name: host-fs
      hostPath:
        path: C:\
```

### HostProcess vs Linux 特权容器对比

| 能力 | Linux privileged | Windows HostProcess |
|------|-----------------|--------------------|
| 主机文件系统访问 | ✅ | ✅ (通过 hostPath) |
| 主机网络 | ✅ | ✅ (必须 hostNetwork) |
| 设备访问 | ✅ | ❌ |
| 内核模块加载 | ✅ | ❌ |
| 运行 Windows 服务 | N/A | ✅ |
| 修改注册表 | N/A | ✅ |

## Windows 监控与日志

### Prometheus 指标采集

```yaml
# Windows 节点监控 — windows_exporter DaemonSet
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: windows-exporter
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: windows-exporter
  template:
    metadata:
      labels:
        app: windows-exporter
    spec:
      securityContext:
        windowsOptions:
          hostProcess: true
          runAsUserName: "NT AUTHORITY\\SYSTEM"
      hostNetwork: true
      nodeSelector:
        kubernetes.io/os: windows
      tolerations:
        - key: os
          value: windows
          effect: NoSchedule
      containers:
        - name: exporter
          image: ghcr.io/prometheus-community/windows-exporter:latest
          ports:
            - containerPort: 9182
              name: metrics
          args:
            - --collectors.enabled=cpu,cs,logical_disk,net,os,system,container
---
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: windows-exporter
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: windows-exporter
  endpoints:
    - port: metrics
      interval: 30s
```

### 关键监控指标

| 指标 | 告警阈值 | 含义 |
|------|----------|------|
| `windows_os_physical_memory_free_bytes` | < 500MB | 内存不足 |
| `windows_logical_disk_free_bytes{volume="C:"}` | < 10% | 系统盘将满 |
| `windows_cpu_time_total{mode="idle"}` | < 10% | CPU 过载 |
| `windows_container_count` | > 50/node | 容器密度过高 |
| `windows_system_system_up_time` | < 1h | 节点刚重启 |

## Windows 性能调优

### 资源管理最佳实践

```yaml
# Windows Pod 资源配置建议
apiVersion: apps/v1
kind: Deployment
metadata:
  name: windows-app
spec:
  template:
    spec:
      nodeSelector:
        kubernetes.io/os: windows
      containers:
        - name: app
          image: mcr.microsoft.com/dotnet/aspnet:8.0-nanoserver-ltsc2022
          resources:
            requests:
              cpu: 500m        # Windows CPU 调度粒度较大
              memory: 512Mi    # 基础镜像占用较大
            limits:
              cpu: "2"
              memory: 2Gi
          # Windows 特有: 资源预留考虑基础镜像开销
          # NanoServer 基础 ~100MB, ServerCore ~2GB
```

### Windows 节点资源预留

| 资源 | 建议预留 | 说明 |
|------|----------|------|
| CPU | 1-2 核 | Windows OS + kubelet + containerd |
| 内存 | 2-4 GiB | Windows 系统占用较大 |
| 磁盘 | 50+ GB | 镜像层占用大（ServerCore ~2GB/层） |

### 镜像拉取优化

```yaml
# Kubelet 配置 — Windows 优化
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
maxParallelImagePulls: 5        # 并行拉取（Windows 镜像大）
imageMinimumGCAge: 10m
imageGCHighThresholdPercent: 90  # Windows 镜像占用大，延迟清理
serializeImagePulls: false
```

## Windows 故障排查

### 诊断命令集

```powershell
# 🟢 只读：Windows 节点诊断
# 检查 kubelet 状态
Get-Service kubelet
Get-EventLog -LogName Application -Source kubelet -Newest 20

# 检查 containerd 状态
Get-Service containerd
crictl ps

# 检查网络
ipconfig /all
Get-HNSNetwork | Format-Table Name, Type, Subnets
Get-HNSEndpoint | Select-Object Name, IPAddress, MacAddress

# 检查磁盘空间
Get-PSDrive -PSProvider FileSystem

# 检查 Windows 更新状态
Get-HotFix | Sort-Object InstalledOn -Descending | Select-Object -First 5

# 检查容器日志
Get-EventLog -LogName Application -Source "Docker" -Newest 10 2>$null
Get-WinEvent -LogName "Microsoft-Windows-Containers*" -MaxEvents 20
```

### 常见问题排查表

| 问题 | 可能原因 | 排查命令 | 解决方案 |
|------|----------|----------|----------|
| Pod CrashLoopBackOff | 镜像版本不匹配 | `kubectl describe pod` | 确认 Build 号匹配 |
| 网络不通 | HNS 网络异常 | `Get-HNSNetwork` | 重启 HNS 服务 |
| 镜像拉取失败 | 磁盘空间不足 | `Get-PSDrive C` | 清理镜像/扩容 |
| 节点 NotReady | kubelet 崩溃 | `journalctl -u kubelet` | 重启 kubelet |
| DNS 解析失败 | CoreDNS 兼容性 | `nslookup kubernetes.default` | 检查 DNS 配置 |
| 存储挂载失败 | CSI 驱动不兼容 | `kubectl describe pvc` | 确认 CSI 支持 Windows |

### Windows 版本兼容性矩阵

| 宿主机 Build | 容器 Build | Process 隔离 | Hyper-V 隔离 |
|-------------|-----------|------------|------------|
| 10.0.17763 (2019) | 10.0.17763 | ✅ | ✅ |
| 10.0.20348 (2022) | 10.0.20348 | ✅ | ✅ |
| 10.0.20348 (2022) | 10.0.17763 | ❌ | ✅ |
| 10.0.17763 (2019) | 10.0.20348 | ❌ | ❌ |

> **关键规则**: Process 隔离要求宿主机 Build ≥ 容器 Build。Hyper-V 隔离可向下兼容。

## Windows 安全加固

### 安全最佳实践

```yaml
# Windows Pod 安全配置
apiVersion: v1
kind: Pod
metadata:
  name: secure-windows-app
spec:
  nodeSelector:
    kubernetes.io/os: windows
  securityContext:
    windowsOptions:
      gmsaCredentialSpecName: webapp-gmsa  # GMSA 身份
  containers:
    - name: app
      image: registry.internal/app:latest
      securityContext:
        windowsOptions:
          runAsUserName: "ContainerUser"  # 非管理员运行
      # Windows 不支持: seccomp, AppArmor, SELinux
      # 使用 GMSA 代替 Linux 的 ServiceAccount 集成
```

### Windows 安全限制与替代方案

| Linux 安全机制 | Windows 替代 | 说明 |
|--------------|------------|------|
| seccomp | ❌ 无替代 | 依赖 Hyper-V 隔离 |
| AppArmor | ❌ 无替代 | 使用 Windows Defender |
| SELinux | ❌ 无替代 | 使用 Windows ACL |
| runAsNonRoot | runAsUserName | 指定非管理员用户 |
| ServiceAccount | GMSA | 域身份认证 |
| NetworkPolicy | Calico/Azure | 部分 CNI 支持 |

## 混合集群最佳实践

### 架构设计原则

```
┌──────────────────────────────────────────────┐
│          混合集群 (Linux + Windows)          │
│                                              │
│  ┌────────────────┐  ┌────────────────┐  │
│  │ Linux 节点池   │  │ Windows 节点池  │  │
│  │ • API/微服务    │  │ • .NET 应用     │  │
│  │ • 中间件        │  │ • IIS/ASP.NET   │  │
│  │ • 监控/日志     │  │ • SQL Server    │  │
│  │ • Ingress       │  │ • 传统应用      │  │
│  └────────────────┘  └────────────────┘  │
│                                              │
│  关键规则:                                    │
│  1. 控制平面必须 Linux                        │
│  2. nodeSelector 强制分离                     │
│  3. DaemonSet 需加 nodeAffinity              │
│  4. CNI 必须支持双平台                        │
└──────────────────────────────────────────────┘
```

### DaemonSet 跨平台兼容

```yaml
# DaemonSet 仅运行在 Linux 节点
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: log-collector
spec:
  template:
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
              - matchExpressions:
                  - key: kubernetes.io/os
                    operator: In
                    values: ["linux"]
      containers:
        - name: collector
          image: fluent/fluent-bit:latest
```

## See Also

- 08-multi-tenancy-architecture
- 09-edge-computing-kubeedge
- 11-kubernetes-source-code-architecture
- 12-cluster-deployment-patterns


<!-- risk-assessed -->
