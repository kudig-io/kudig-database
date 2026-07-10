---
title: Windows 容器在 Kubernetes 中的支持
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- containerd
- docker
- opa
- hpa
tier: peripheral
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Windows 容器在 Kubernetes 中的支持 是什么
- 如何 Windows 容器在 Kubernetes 中的支持
trigger_keywords:
- Windows
- 容器在
- Kubernetes
- 中的支持
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Windows 容器在 [[Kubernetes|Kubernetes]] 中的支持

## 概述

Windows 应用程序在众多组织的服务和应用中占有很大比例。Windows 容器提供了一种封装进程和打包依赖的方式，使得 Windows 应用也能采用 DevOps 实践并遵循云原生模式。通过在现有的 Linux 集群中加入 Windows 节点，组织无需为不同操作系统寻找单独的编排器，从而提升整体运维效率。

Kubernetes 支持在 Windows 节点上运行 Windows 容器（仅支持进程隔离模式，不支持 Hyper-V 隔离模式）。控制平面必须运行在 Linux 上，而工作节点可以是 Windows 或 Linux。

## 核心概念/原理

### Windows 节点要求
- 控制平面只能运行在 Linux 上。
- 工作节点可运行 Windows Server 2022 或 Windows Server 2025。
- 集群必须是多操作系统混合集群。
- 需要安装兼容的容器运行时（如 [[containerd|containerD]] 或 Mirantis [[concepts/container-runtime.md|Container Runtime]]）。

### 操作系统兼容性
- Windows 节点与容器基础镜像之间存在严格的版本兼容性规则：主机的操作系统版本必须与容器基础镜像的操作系统版本匹配。
- Kubernetes v1.35 支持：Windows Server 2022、Windows Server 2025。

### Pause 容器
- Kubernetes 使用 pause 容器作为 Pod 的网络和生命周期基础。
- Kubernetes v1.35.0 推荐的 pause 镜像为 `registry.[[entities/kubernetes.md|[[Kubernetes 生产环境速查卡|k8s]]]].io/pause:3.6`。
- 若生产环境要求签名二进制文件，建议使用 Microsoft 维护的镜像 `mcr.microsoft.com/oss/kubernetes/pause:3.6`。

## 关键机制或特性

### 与 Linux 的功能对比

**支持的 Pod 功能：**
- 单 Pod 多容器（进程隔离、卷共享）
- Pod `status` 字段
- 存活探针（liveness）、就绪探针（readiness）、启动探针（startup）
- `postStart` 和 `preStop` 生命周期钩子
- ConfigMap、Secret（环境变量或卷挂载）
- `emptyDir` 卷
- 命名管道（Named pipe）主机挂载
- 资源限制（Resource limits）
- `kubectl exec`、Pod/容器指标、HPA、资源配额、调度抢占

**必须设置的字段：**
- `.spec.os.name` 应设置为 `windows`，以表明该 Pod 使用 Windows 容器。
- 当设置 `.spec.os.name` 为 `windows` 时，以下字段**禁止**设置：
  - `spec.hostPID`、`spec.hostIPC`、`spec.shareProcessNamespace`
  - `spec.securityContext` 下的 `seLinuxOptions`、`seccompProfile`、`fsGroup`、`fsGroupChangePolicy`、`sysctls`、`runAsUser`、`runAsGroup`、`supplementalGroups`
  - `spec.containers[*].securityContext` 下的 `seLinuxOptions`、`seccompProfile`、`capabilities`、`readOnlyRootFilesystem`、`privileged`、`allowPrivilegeEscalation`、`procMount`、`runAsUser`、`runAsGroup`

### 不支持的特性
- **HugePages**：Windows 容器不支持。
- **Privileged 容器**：不支持，可使用 HostProcess Containers 替代。
- **hostNetwork / hostIPC / hostPID**：Windows 不支持主机命名空间共享。
- **TerminationGracePeriod**：在 Docker 上未完全实现；containerD 支持。
- **volumeDevices（原始块设备）**：Windows 无法将原始块设备附加到 Pod。
- **mountPropagation**：Windows 不支持。
- **只读根文件系统（readOnlyRootFilesystem）**：Windows 不支持，因为注册表和系统进程需要写权限。

### Kubelet 差异
- `--windows-priorityclass`：可设置 kubelet 进程的调度优先级。
- `--kube-reserved`、`--system-reserved`：仅影响 `NodeAllocatable` 计算，不保证为工作负载预留资源。
- `--enforce-node-allocatable` 的驱逐机制未实现。
- `PIDPressure` 条件未实现。
- kubelet 不会执行 OOM 驱逐操作。

### 容器运行时
- **containerD**（v1.20+ stable）：推荐在 Windows 节点上使用 containerD 1.4.0+。
- **Mirantis Container Runtime (MCR)**：适用于 Windows Server 2019 及更高版本。

### 节点问题检测器（Node Problem Detector）
- 对 Windows 提供初步支持，可用于监控节点健康状况。

## 使用场景

- 企业已拥有大量基于 Windows 的应用程序，希望将其现代化并迁移到 Kubernetes 平台。
- 需要统一编排 Linux 和 Windows 工作负载，避免维护两套独立的编排系统。
- 运行 .NET Framework、IIS 等传统 Windows 服务。
- 混合云或多云环境中需要一致的 Windows 应用部署体验。

## 最佳实践/注意事项

1. **版本匹配**：确保 Windows 节点 OS 版本与容器镜像 OS 版本严格匹配，否则容器无法启动。
2. **正确设置 `.spec.os.name`**：将 Pod 的 `.spec.os.name` 显式设为 `windows`，以避免调度到 Linux 节点并触发 API 拒绝。
3. **避免使用不支持的字段**：不要在 Windows Pod 中设置 Linux 特有的安全上下文字段（如 `seLinuxOptions`、`capabilities` 等）。
4. **使用 containerD**：containerD 是当前 Kubernetes 在 Windows 上推荐的稳定容器运行时。
5. **资源规划**：Windows 容器镜像通常比 Linux 镜像大（300MB 至 10GB+），节点磁盘空间应预留充足（建议 50GB+）。
6. **硬件配置**：建议 Windows 工作节点至少具备 64 位 4 核 CPU、8GB 内存、50GB 可用磁盘空间。
7. **日志收集**：Windows 容器日志行为与 Linux 不同，建议配合 LogMonitor 等工具将 ETW/事件日志重定向到 STDOUT。
8. **生产环境镜像**：若要求签名二进制文件，优先使用 Microsoft 维护的 pause 镜像。
9. **问题排查**：遇到问题时，参考 Kubernetes Troubleshooting 页面，并按 SIG Windows 贡献指南收集日志后提交 issue。

## 参考链接

- [Windows containers in Kubernetes - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/windows/intro/)

## Related

- [[domain-17-system-foundation/知识字典/specialized-workloads/ai-infra-specialist.md|08 - AI/ML基础设施专业词典]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/gpu-resource-management-and-partitioning.md|GPU 资源管理与分区技术]]
- [[domain-17-system-foundation/知识字典/specialized-workloads/guide-for-running-windows-containers-in-kubernetes.md|在 Kubernetes 中运行 Windows 容器指南]]


<!-- risk-assessed -->
