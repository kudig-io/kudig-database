---
title: Pod 安全标准
description: '# Pod 安全标准'
summary: '# Pod 安全标准'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- opa
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pod 安全标准 是什么
- 如何 Pod 安全标准
trigger_keywords:
- Pod
- 安全标准
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod 安全标准

## 概述

Pod 安全标准（Pod Security Standards）定义了三个不同级别的策略，用于广泛覆盖安全光谱。这些策略是累积的，从高度宽松到高度受限不等。本页面详细说明了每个策略的具体要求，为集群管理员和开发者提供统一的安全语言。

## 核心概念/原理

Pod 安全标准定义了以下三个策略级别：

- **Privileged（特权）**：完全不受限制的策略，提供最大范围的权限。允许已知的特权升级，通常面向由受信任管理员管理的系统和基础设施级工作负载。
- **Baseline（基线）**：最小限制性策略，防止已知的特权升级，同时允许默认的（最小指定的）Pod 配置。面向应用运维人员和非关键应用的开发者。
- **Restricted（受限）**：高度受限的策略，遵循当前的 Pod 加固最佳实践，以牺牲部分兼容性为代价。面向安全关键型应用的运维人员以及低信任度用户。

## 关键机制或特性

### Baseline 策略的关键控制项

- **HostProcess**：禁止 Windows HostProcess 容器获得主机特权访问。
- **Host Namespaces**：禁止共享主机的网络、PID 和 IPC 命名空间（`hostNetwork`、`hostPID`、`hostIPC` 必须为 false 或未定义）。
- **Privileged Containers**：禁止特权容器（`privileged` 必须为 false 或未定义）。
- **Capabilities**：仅允许添加一组有限的 capabilities（如 `AUDIT_WRITE`、`CHOWN`、`DAC_OVERRIDE`、`KILL`、`NET_BIND_SERVICE`、`SETUID` 等）。
- **HostPath Volumes**：禁止使用 HostPath 卷。
- **Host Ports**：禁止或限制使用 HostPort（建议完全禁止）。
- **AppArmor**：防止覆盖或禁用默认的 AppArmor 配置文件。
- **SELinux**：限制 SELinux 类型设置，禁止自定义用户或角色。
- **Seccomp**：禁止显式设置为 `Unconfined`。
- **Sysctls**：仅允许一组“安全”的 sysctl 参数（如 `kernel.shm_rmid_forced`、`net.ipv4.ip_local_port_range` 等）。

### Restricted 策略的额外控制项

在 Baseline 的基础上，Restricted 策略还增加了：

- **Volume Types**：仅允许使用 `configMap`、`csi`、`downwardAPI`、`emptyDir`、`ephemeral`、`persistentVolumeClaim`、`projected`、`secret` 类型的卷。
- **Privilege Escalation**：禁止特权提升（`allowPrivilegeEscalation` 必须为 `false`）。
- **Running as Non-root**：要求容器必须以非 root 用户运行（`runAsNonRoot` 必须为 `true`）。
- **Running as Non-root user**：禁止将 `runAsUser` 设置为 0。
- **Seccomp**：必须显式设置为 `RuntimeDefault` 或 `Localhost`，禁止未设置或 `Unconfined`。
- **Capabilities**：必须丢弃 `ALL` capabilities，仅允许重新添加 `NET_BIND_SERVICE`。

### OS 特定策略

自 [[kubernetes|Kubernetes]] v1.25 起，Restricted 策略会根据 `pod.spec.os.name` 字段进行 OS 特定的策略控制。对于 Windows Pod，以下 Linux 专属控制项会被放宽：

- Privilege Escalation
- Seccomp
- Linux Capabilities

## 使用场景

- **Privileged**：运行需要完全访问主机资源的基础设施组件（如节点监控代理、网络插件）。
- **Baseline**：运行常见的容器化应用，希望在保持兼容性的同时防止已知的特权升级。
- **Restricted**：运行安全关键型应用、面向外部用户的 SaaS 工作负载，或需要遵循严格安全合规的场景。

## 最佳实践/注意事项

- 尽可能为工作负载应用 **Restricted** 策略；如果无法兼容，则使用 **Baseline**。
- **Privileged** 策略应有意识地使用，并仅限于受信任的系统级工作负载。
- 在包含 v1.24 之前节点的集群中，Restricted 策略应固定到 v1.24 之前的版本，因为旧版 kubelet 不强制 pod OS 字段。
- 策略定义与策略实例化（执行机制）解耦，可以使用内置的 Pod Security Admission 控制器，也可以使用 Kyverno、OPA Gatekeeper、Kubewarden 等第三方工具执行。
- 沙箱 Pod（如使用 gVisor 或 Kata Containers）的隔离需求与普通 Pod 不同，没有单一的标准配置文件适用于所有沙箱工作负载。

## 参考链接

- https://kubernetes.io/docs/concepts/security/pod-security-standards/

## Related

- [[21-生态参考/03-领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
