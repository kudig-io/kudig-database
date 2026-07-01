---
title: 10 - Windows 容器支持与集成指南
description: '# 10 - Windows 容器支持与集成指南'
summary: '# 10 - Windows 容器支持与集成指南'
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
  path: ../domain-13-container-runtime/
  label: '相关知识域: domain-13-container-runtime'
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---



# 10 - Windows 容器支持与集成指南

<!-- chunk: Windows节点要求 -->
## Windows节点要求

| 要求 | 最低版本 | 说明 |
|-----|---------|------|
| Windows Server | 2019 LTSC | 长期支持版本 |
| Windows Server | 2022 LTSC | 推荐版本 |
| Docker/containerd | [[containerd|containerd]] 1.6+ | 容器运行时 |
| [[Kubernetes|Kubernetes]] | v1.22+ | 稳定支持 |

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
| [[Service|Service]] | ✅ | ✅ | ClusterIP/NodePort/LB |
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

```powershell
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

- domain-01-cluster-fundamentals MOC
- [[domain-01-cluster-fundamentals/README.md|Domain-1: Kubernetes架构基础]]
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

## See Also

- 08-multi-tenancy-architecture
- 09-edge-computing-kubeedge
- 11-kubernetes-source-code-architecture
- 12-cluster-deployment-patterns
