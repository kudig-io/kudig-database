---
title: Kata Containers
description: Kata Containers 是 OpenInfra Foundation 的开源项目，通过轻量级虚拟机提供容器级别的隔离。它兼容 OCI
  运行时规范，可作为...
summary: Kata Containers 是 OpenInfra Foundation 的开源项目，通过轻量级虚拟机提供容器级别的隔离。它兼容 OCI 运行时规范，可作为...
category: dictionary
tags:
- k8s
- glossary
- kata-containers
- sandbox
- security
- oci
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kata Containers 是什么
- Kata Containers 详解
trigger_keywords:
- Kata Containers
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kata Containers

> **英文名**: Kata Containers

## 概述

Kata Containers 是 OpenInfra Foundation 的开源项目，通过轻量级虚拟机提供容器级别的隔离。它兼容 OCI 运行时规范，可作为 runc 的安全替代方案，特别适合多租户和高安全要求的场景。

## 核心概念/原理

### 隔离级别对比

| 运行时 | 隔离方式 | 安全级别 | 开销 |
|--------|----------|----------|------|
| runc | Namespace + Cgroup | 中 | 极低 |
| gVisor | 用户态内核 | 高 | 低 |
| Kata Containers | 轻量 VM | 极高 | 中 |
| Firecracker | microVM | 极高 | 中 |

### 工作原理

每个 Pod 运行在独立的轻量 VM 中，VM 内运行 Linux 内核和容器进程。

## 关键机制或特性

- **OCI 兼容**：可作为 containerd/CRI-O 的 OCI Runtime。
- **RuntimeClass**：通过 K8s RuntimeClass 选择性使用。
- **多种 VMM**：支持 QEMU、Cloud Hypervisor、Firecracker。
- **Direct-Attached Volume**：PV 直通到 VM 中。
- 与 Kubernetes 完全集成（Pod、Service、NetworkPolicy）。

## 使用场景与最佳实践

- 多租户集群使用 Kata 提供 VM 级别的租户隔离。
- 运行不可信代码时使用 Kata 替代 runc。
- 通过 RuntimeClass 为特定 Pod 指定 Kata 运行时。
- 注意 Kata 的额外资源开销（每 Pod ~30MB VM 开销）。
- 监控 Kata Pod 的启动时间（比 runc 慢 1-2 秒）。

## 参考链接

- [Kata Containers Official](https://katacontainers.io/)

## Related

- [[domain-17-system-foundation/topic-dictionary/fundamentals/runc.md|runc]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/containerd.md|Containerd]]
- [[domain-17-system-foundation/topic-dictionary/fundamentals/cri-o.md|CRI-O]]
- [[domain-17-system-foundation/topic-dictionary/security/security-context.md|Security Context]]
- [[domain-17-system-foundation/topic-dictionary/workloads/pod.md|Pod]]


<!-- risk-assessed -->
