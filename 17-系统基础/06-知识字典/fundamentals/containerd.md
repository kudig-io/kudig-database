---
title: containerd
description: containerd 是一个工业级容器运行时，最初从 Docker 中拆分出来，现为 CNCF 毕业项目。它是 Kubernetes 默认的容器运行时（通过
  C...
summary: containerd 是一个工业级容器运行时，最初从 Docker 中拆分出来，现为 CNCF 毕业项目。它是 Kubernetes 默认的容器运行时（通过
  C...
category: dictionary
tags:
- k8s
- glossary
- containerd
- cri
- container-runtime
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd 是什么
- containerd 详解
trigger_keywords:
- containerd
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# containerd

> **英文名**: containerd

## 概述

containerd 是一个工业级容器运行时，最初从 Docker 中拆分出来，现为 CNCF 毕业项目。它是 Kubernetes 默认的容器运行时（通过 CRI 接口），负责容器的完整生命周期管理。

## 核心概念/原理

### 架构层次

```
kubelet → CRI → containerd → runc → Linux Kernel
                     ↓
              shim (per-container)
```

### 核心组件

| 组件 | 职责 |
|------|------|
| containerd daemon | 容器生命周期管理 |
| containerd-shim | 每个容器的独立进程，与 daemon 解耦 |
| runc | OCI 运行时规范实现 |
| ctr / crictl | 命令行工具 |

## 关键机制或特性

- **CRI 接口**：kubelet 通过 gRPC 调用 containerd 的 CRI 实现。
- **shim 架构**：containerd-shim 为每个容器独立运行，containerd 重启不影响容器。
- **镜像管理**：支持 OCI 和 Docker 镜像格式。
- **快照管理**：overlayfs 等快照驱动管理容器文件系统层。
- 配置文件位于 `/etc/containerd/config.toml`。

## 使用场景与最佳实践

- 使用 `crictl` 而非 `docker` 命令调试容器。
- 配置 mirror 加速镜像拉取（特别是国内环境）。
- 启用 `SystemdCgroup` 与 kubelet 保持一致。
- 监控 containerd 的 gRPC 延迟和容器启动时间指标。

## 参考链接

- [containerd Official](https://containerd.io/)

## Related

- [[17-系统基础/06-知识字典/fundamentals/cri.md|CRI]]
- [[17-系统基础/06-知识字典/fundamentals/kubelet.md|Kubelet]]
- [[17-系统基础/06-知识字典/workloads/pod.md|Pod]]
- [[17-系统基础/06-知识字典/fundamentals/container.md|Container]]
- [[17-系统基础/06-知识字典/fundamentals/worker-node.md|Worker Node]]


<!-- risk-assessed -->
- [[01-集群基础/01-架构总览/11-performance-tuning-guide|13 - Kubernetes 性能调优专项指南]]
- [[01-集群基础/03-控制平面/35-kubeadm-cluster-lifecycle|32 - kubeadm 集群生命周期管理 (Cluster Lifecycle with kubeadm)]]
- [[01-集群基础/03-控制平面/23-container-network-deep-dive|CNI 容器网络接口深度解析 (Container Network Interface Deep Dive)]]
- [[15-AI基础设施/02-AI-Agents/30-agent-harness-engineering|Agent Harness 工程：从模型包装到生产级 Agent 系统设计 (AI基础设施)]]
- [[17-系统基础/04-K8s事件/02-pod-container-lifecycle-events|02 - Pod 与容器生命周期事件]]
- [[17-系统基础/04-K8s事件/06-node-lifecycle-condition-events|06 - 节点生命周期与状态事件]]
- [[17-系统基础/06-知识字典/platform-engineering/webassembly-wasm-workloads|WebAssembly（Wasm）工作负载]]
- [[17-系统基础/06-知识字典/specialized-workloads/windows-containers-in-kubernetes|Windows 容器在 Kubernetes 中的支持]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/reference/diagnostic-workflow|诊断工作流 / Diagnostic Workflow]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/reference/root-cause-catalog|根因分类 / Root Cause Catalog]]
- [[22-概念/01-核心架构/core-dependency-version-matrix|核心依赖版本矩阵]]
- [[22-概念/15-运行时与系统/linux-sysctl-tuning|Linux Sysctl Tuning for Kubernetes]]
- [[23-实体/15-参考与索引/k8s-structured-troubleshooting|结构化排障方法论：配置优先、全组件排障指南]]
- [[23-实体/15-参考与索引/k8s-control-plane-deep-dive|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]]
- [[26-技能/01-集群运维/cluster-upgrade/最佳实践/k8s-cluster-configuration-guide|Kubernetes 集群配置最佳实践]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-lifecycle|kubeadm 集群创建生命周期]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-deletion|kubeadm 集群删除操作]]
- [[26-技能/04-工作负载/pod/方法论/skill-reference-diagnostic-workflow|Diagnostic Workflow]]
- [[26-技能/04-工作负载/pod/方法论/skill-reference-root-cause-catalog|Root Cause Catalog]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes FTA Top Events Index|Kubernetes FTA Top Events Index]]
- [[26-技能/04-工作负载/pod/生命周期与事件/01-pod-container-lifecycle-events|02 - Pod 与容器生命周期事件]]
