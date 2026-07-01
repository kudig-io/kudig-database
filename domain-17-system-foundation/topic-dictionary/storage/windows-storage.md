---
title: Windows Storage（Windows 存储）
description: '# Windows Storage（Windows 存储）'
category: dictionary
tags:
- k8s
- glossary
- terminology
- containerd
- docker
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Windows Storage（Windows 存储） 是什么
- 如何 Windows Storage（Windows 存储）
trigger_keywords:
- Windows
- Storage
- 存储
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
created: "2026-05-23"
---

# Windows Storage（Windows 存储）

## 概述

Windows 节点上的存储行为与 Linux 节点存在显著差异，主要是由于 Windows 的文件系统架构、NTFS、注册表和 SAM（Security Account Manager）数据库的隔离机制。[[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 在 Windows 上支持部分卷类型和功能，但也有一些 Linux 特有的功能不被支持。

## 核心概念/原理

- **分层文件系统驱动**：Windows 使用分层文件系统驱动来挂载容器层，并基于 NTFS 创建副本文件系统。容器内的所有文件路径仅在容器上下文中解析。
- **权限隔离**：主机与容器之间的 SAM 数据库不共享，因此 UID/GID、用户掩码和 Linux 文件权限等概念在 Windows 容器中不适用。
- **只读限制**：Windows 容器不支持只读根文件系统，因为注册表和 SAM 数据库始终需要写访问；但支持将卷以 `readOnly` 方式挂载。

## 关键机制或特性

### 支持的卷类型

Windows 节点上支持的持久化存储插件类别：
- **FlexVolume 插件**（已弃用，v1.23 起不推荐）
- **CSI 插件**（推荐）

支持 persistent storage 的 in-tree 插件：
- `azureFile`
- `vsphereVolume`

### 不支持的存储功能

以下功能在 Windows 节点上**不受支持**：
- `subPath` 卷挂载（只能挂载整个卷）
- Secret 的 `subPath` 挂载
- `hostPath` 卷的主机挂载投射
- 只读根文件系统（`readOnlyRootFilesystem`）
- 块设备映射（Block device mapping）
- `emptyDir.medium: Memory`（内存作为存储介质）
- UID/GID、按用户的 Linux 文件系统权限
- 使用 `DefaultMode` 设置 Secret 权限
- NFS 存储/卷支持
- 已挂载卷的扩展（resizefs）

### Docker 与 containerd 差异

- **Docker**：卷挂载只能 targeting 容器内的目录，不能 targeting 单个文件。
- **containerd**：没有此限制，支持挂载单个文件。

### Volume 挂载行为

- 卷挂载无法将文件或目录投射回主机文件系统。
- 所有权限都在容器上下文中解析，主机无法识别容器内的虚拟用户账户。

## 使用场景

- **Windows 工作负载持久化**：在 Windows 容器上运行 .NET、IIS 等应用时，通过 CSI 或 `azureFile`/`vsphereVolume` 实现数据持久化。
- **配置注入**：通过 ConfigMap 和 Secret 卷将配置文件注入 Windows 容器（注意不支持 `subPath`）。
- **云原生 Windows 应用**：在混合操作系统集群（Linux + Windows）中，为 Windows 节点提供与云平台集成的存储能力。

## 最佳实践/注意事项

- 在 Windows Pod 中避免使用 `subPath`，因为它不受支持。
- 不要为 Windows Pod 设置 `readOnlyRootFilesystem: true`，否则会导致容器启动失败。
- 避免依赖 Linux 文件权限模型（如 `runAsUser`、UID/GID、DefaultMode）来管理 Windows 容器中的文件访问。
- 尽量使用 CSI 插件来管理 Windows 节点的持久存储，避免依赖已弃用的 in-tree 和 FlexVolume 插件。
- 如需在 Windows 上运行有状态应用，优先选择支持 Windows 的 CSI 驱动（如 Azure Disk CSI、SMB CSI 等）。

## 生产 YAML 示例

### Windows Pod 使用 CSI PVC

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: dotnet-app
  namespace: production
spec:
  replicas: 2
  selector:
    matchLabels:
      app: dotnet-app
  template:
    metadata:
      labels:
        app: dotnet-app
    spec:
      nodeSelector:
        kubernetes.io/os: windows
      containers:
        - name: app
          image: mcr.microsoft.com/dotnet/aspnet:8.0-nanoserver-ltsc2022
          volumeMounts:
            - name: app-data
              mountPath: "C:\\data"          # Windows 路径格式
            - name: config
              mountPath: "C:\\config"
              readOnly: true
          resources:
            requests:
              cpu: "500m"
              memory: 1Gi
            limits:
              cpu: "2"
              memory: 2Gi
      volumes:
        - name: app-data
          persistentVolumeClaim:
            claimName: dotnet-data
        - name: config
          configMap:
            name: dotnet-config              # 注意：不支持 subPath
      tolerations:
        - key: "os"
          value: "windows"
          effect: "NoSchedule"
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Windows Pod 使用 subPath 失败 | Windows 不支持 subPath | 改为挂载整个卷目录 |
| readOnlyRootFilesystem 导致启动失败 | Windows 不支持只读根文件系统 | 移除 `readOnlyRootFilesystem: true` |
| NFS 卷挂载失败 | Windows 不支持 NFS | 改用 SMB CSI 或 Azure File |

## 生产检查清单

- [ ] 使用 CSI 驱动（Azure Disk CSI / SMB CSI）而非 in-tree 插件
- [ ] 不使用 subPath、readOnlyRootFilesystem、DefaultMode
- [ ] 不使用 emptyDir `medium: Memory`
- [ ] 使用 nodeSelector 确保调度到 Windows 节点

## 命令快速参考

```bash
# 查看 Windows 节点
kubectl get nodes -l kubernetes.io/os=windows

# 查看 Windows Pod 卷挂载
kubectl get pod <pod> -o jsonpath='{.spec.containers[0].volumeMounts}' | jq .
```

## 交叉引用

- [卷](./volumes.md) — 卷类型总览
- [持久卷](./persistent-volumes.md) — PV/PVC 通用概念

## 参考链接

- https://kubernetes.io/docs/concepts/storage/windows-storage/

## Related

- [[domain-17-system-foundation/topic-dictionary/storage/ceph.md|Ceph]]
- [[domain-17-system-foundation/topic-dictionary/storage/cloudnativepg.md|CloudNativePG 云原生 PostgreSQL]]
- [[domain-17-system-foundation/topic-dictionary/storage/composefs.md|ComposeFS 只读文件系统]]
