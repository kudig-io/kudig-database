---
title: 在 Kubernetes 中运行 Windows 容器指南
description: '# 在 Kubernetes 中运行 Windows 容器指南'
summary: '# 在 Kubernetes 中运行 Windows 容器指南'
category: dictionary
tags:
- k8s
- glossary
- terminology
- operator
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 在 Kubernetes 中运行 Windows 容器指南 是什么
- 如何 在 Kubernetes 中运行 Windows 容器指南
trigger_keywords:
- Kubernetes
- 中运行
- Windows
- 容器指南
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 在 [[Kubernetes|Kubernetes]] 中运行 Windows 容器指南

## 概述

本指南提供了在 Kubernetes 集群中运行 Windows 容器的实操步骤和注意事项。在 Kubernetes 上创建和部署服务与工作负载时，Windows 容器与 Linux 容器的行为大体相同，`kubectl` 命令也完全一致。本文通过示例帮助用户快速上手 Windows 容器的部署、调度、可观测性和身份管理。

## 核心概念/原理

### 部署 Windows 工作负载的基础
- 需要拥有一个已包含 Windows Server 工作节点的 Kubernetes 集群。
- Windows 容器只能运行在 Windows 节点上，不能运行在 Linux 节点上。
- 应使用 `nodeSelector`（如 `kubernetes.io/os: windows`）确保 Pod 被调度到正确的节点。
- 建议为每个 Pod 设置 `.spec.os.name` 为 `windows`，以明确标识目标操作系统（Kubernetes 1.24+ 默认支持）。

### 调度机制
- 调度器**不会**根据 `.spec.os.name` 的值来分配 Pod 到节点，因此仍需使用 `nodeSelector` 或污点/容忍（taints/tolerations）来确保 Windows Pod 落在 Windows 节点上。
- Windows 节点默认带有标签：
  - `kubernetes.io/os` = `windows` 或 `linux`
  - `kubernetes.io/arch` = `amd64` 或 `arm64` 等
- Kubernetes 自动为 Windows 节点添加 `node.kubernetes.io/windows-build` 标签，用于匹配 Windows Server 版本：
  - Windows Server 2022：`10.0.20348`
  - Windows Server 2025：`10.0.26100`

## 关键机制或特性

### 1. 部署示例：Windows Web 服务器

以下 YAML 部署了一个简单的 Windows Web 服务器：

```yaml
---
apiVersion: v1
kind: Service
metadata:
  name: win-webserver
  labels:
    app: win-webserver
spec:
  ports:
    - port: 80
      targetPort: 80
  selector:
    app: win-webserver
  type: NodePort
---
apiVersion: apps/v1
kind: Deployment
metadata:
  labels:
    app: win-webserver
  name: win-webserver
spec:
  replicas: 2
  selector:
    matchLabels:
      app: win-webserver
  template:
    metadata:
      labels:
        app: win-webserver
      name: win-webserver
    spec:
      containers:
      - name: windowswebserver
        image: mcr.microsoft.com/windows/servercore:ltsc2019
        command:
        - powershell.exe
        - -command
        - "...（PowerShell HTTP Listener 脚本）..."
      nodeSelector:
        kubernetes.io/os: windows
```

**部署后验证项：**
- 检查所有节点健康状态
- 观察 Pod 是否变为 Ready
- 节点到 Pod 的网络通信（从 Linux 控制平面节点 curl Pod IP）
- Pod 到 Pod 的通信（跨主机 ping）
- Service 到 Pod 的通信（curl Service 虚拟 IP）
- 服务发现（curl 服务名称）
- 入站连接（curl NodePort）
- 出站连接（从 Pod 内 curl 外部 IP）

> **注意**：由于 Windows 网络栈的平台限制，Windows 容器主机无法访问调度在其上的 Service IP，只有 Windows Pod 可以访问 Service IP。

### 2. 可观测性：日志收集

Windows 容器通常将日志写入 ETW（Event Tracing for Windows）或应用程序事件日志，而不是 STDOUT。Microsoft 开源的 **LogMonitor** 是推荐的解决方案：
- 支持监控事件日志、ETW 提供程序和自定义应用日志。
- 将日志管道输出到 STDOUT，从而可以通过 `kubectl logs <pod>` 查看。

### 3. 容器用户配置

- **可配置容器用户名**：Windows 容器可以配置以不同于镜像默认用户的用户名运行入口点和进程。
- **GMSA（Group Managed Service Accounts）**：Windows 容器工作负载可配置使用 GMSA。GMSA 是一种特殊的 Active Directory 账户，提供自动密码管理、简化的 SPN 管理，并支持跨多台服务器委派管理。配置了 GMSA 的容器可以访问外部 Active Directory 域资源。

### 4. 使用污点（Taints）和容忍（Tolerations）

为避免未指定 `nodeSelector` 的 Pod 被错误调度到 Windows 节点，可以使用污点策略：

- 在 Windows 节点注册时添加污点，例如：
  ```
  --register-with-taints='os=windows:NoSchedule'
  ```
- Windows Pod 需要同时指定 `nodeSelector` 和匹配的 `tolerations`：

```yaml
nodeSelector:
    kubernetes.io/os: windows
    node.kubernetes.io/windows-build: '10.0.20348'
tolerations:
    - key: "os"
      operator: "Equal"
      value: "windows"
      effect: "NoSchedule"
```

### 5. 使用 RuntimeClass 简化调度配置

可以通过 `RuntimeClass` 将 `nodeSelector` 和 `tolerations` 封装起来，简化 Pod 配置：

```yaml
apiVersion: node.[[23-实体/02-K8s核心组件/kubernetes.md|k8s]].io/v1
kind: RuntimeClass
metadata:
  name: windows-2019
handler: example-container-runtime-handler
scheduling:
  nodeSelector:
    kubernetes.io/os: 'windows'
    kubernetes.io/arch: 'amd64'
    node.kubernetes.io/windows-build: '10.0.20348'
  tolerations:
  - effect: NoSchedule
    key: os
    operator: Equal
    value: "windows"
```

在 Pod 中只需指定：
```yaml
spec:
  runtimeClassName: windows-2019
```

## 使用场景

- 快速在 Kubernetes 上部署第一个 Windows 容器工作负载。
- 需要为 Windows 容器配置日志收集和可观测性。
- 使用 GMSA 为 Windows 容器提供 Active Directory 身份认证。
- 在多版本 Windows Server 混合集群中进行精细的 Pod 调度控制。
- 通过 RuntimeClass 统一管理 Windows 工作负载的调度策略。

## 最佳实践/注意事项

1. **始终使用 `nodeSelector`**：为 Windows Pod 显式添加 `kubernetes.io/os: windows` 的 `nodeSelector`，防止调度到 Linux 节点。
2. **使用污点保护 Windows 节点**：为 Windows 节点添加 `NoSchedule` 污点，确保只有明确声明的 Windows 工作负载才能运行其上。
3. **版本一致性**：同一集群中使用多个 Windows Server 版本时，利用 `node.kubernetes.io/windows-build` 标签进行精确匹配。
4. **日志收集**：使用 LogMonitor 将 Windows 容器内的 ETW/事件日志重定向到 STDOUT，提升可观测性。
5. **GMSA 安全**：对于需要访问域资源的 Windows 工作负载，使用 GMSA 进行身份管理，避免在容器内硬编码凭据。
6. **网络限制意识**：Windows 容器主机无法访问自身的 Service IP，设计服务调用链时需避开此限制。
7. **RuntimeClass 简化配置**：推荐使用 RuntimeClass 封装 Windows 节点选择和容忍配置，减少重复 YAML 配置。

## 参考链接

- [Guide for Running Windows Containers in Kubernetes - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/windows/user-guide/)

## Related

- [[17-系统基础/06-知识字典/specialized-workloads/ai-infra-specialist.md|08 - AI/ML基础设施专业词典]]
- [[17-系统基础/06-知识字典/specialized-workloads/gpu-resource-management-and-partitioning.md|GPU 资源管理与分区技术]]
- [[17-系统基础/06-知识字典/specialized-workloads/hpc-and-bioinformatics.md|高性能计算与生物信息学（HPC & Bioinformatics）]]


<!-- risk-assessed -->
