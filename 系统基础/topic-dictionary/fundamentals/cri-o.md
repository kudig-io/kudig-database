---
title: CRI-O
description: CRI-O 是专为 Kubernetes 设计的轻量级容器运行时，实现了 Kubernetes CRI（Container Runtime
  Interface）...
summary: CRI-O 是专为 Kubernetes 设计的轻量级容器运行时，实现了 Kubernetes CRI（Container Runtime Interface）...
category: dictionary
tags:
- k8s
- glossary
- cri-o
- cri
- container-runtime
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CRI-O 是什么
- CRI-O 详解
trigger_keywords:
- CRI-O
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CRI-O

> **英文名**: CRI-O

## 概述

CRI-O 是专为 Kubernetes 设计的轻量级容器运行时，实现了 Kubernetes CRI（Container Runtime Interface）标准。它是 containerd 的主要替代方案，以最小化攻击面和资源开销著称。

## 核心概念/原理

### 与 containerd 对比

| 特性 | CRI-O | containerd |
|------|-------|------------|
| 定位 | 专为 K8s 设计 | 通用容器运行时 |
| 功能范围 | 仅 CRI | CRI + 独立容器管理 |
| 复杂度 | 更小 | 更大 |
| OCI 兼容 | 完全 | 完全 |
| CNCF 状态 | 未入 CNCF | Graduated |

### 架构

```
kubelet → CRI → CRI-O → OCI Runtime (runc/crun)
                    ↓
              conmon (per-container monitor)
```

## 关键机制或特性

- **CRI 专用**：仅实现 Kubernetes CRI，不暴露额外 API。
- **conmon**：每个容器的监控进程，收集退出码和资源使用。
- **支持多种 OCI 运行时**：runc（标准）、crun（C 实现，更快）、kata（VM 隔离）。
- 配置文件：`/etc/crio/crio.conf`。
- 与 kubelet 版本严格对应。

## 使用场景与最佳实践

- 追求最小攻击面的安全敏感环境优先选择 CRI-O。
- 使用 crun 替代 runc 提升容器启动速度。
- 配置镜像 mirror 加速拉取。
- 监控 CRI-O 的 `crio_operations_latency` 指标。
- 确保 CRI-O 版本与 Kubernetes 版本匹配。

## 参考链接

- [CRI-O Official](https://cri-o.io/)

## Related

- [[系统基础/topic-dictionary/fundamentals/cri.md|CRI]]
- [[系统基础/topic-dictionary/fundamentals/containerd.md|Containerd]]
- [[系统基础/topic-dictionary/fundamentals/kubelet.md|Kubelet]]
- [[系统基础/topic-dictionary/workloads/pod.md|Pod]]
- [[系统基础/topic-dictionary/fundamentals/container.md|Container]]


<!-- risk-assessed -->
