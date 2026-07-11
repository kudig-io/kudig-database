---
title: 容器技术、Linux 系统与网络存储基础
description: '# 容器技术、Linux 系统与网络存储基础'
summary: '# 容器技术、Linux 系统与网络存储基础'
category: reference
tags:
- k8s
- docker
- containerd
- linux
- networking-basics
- storage-basics
- cilium
- calico
- ceph
- minio
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 容器技术、Linux 系统与网络存储基础 是什么
- 如何 容器技术、Linux 系统与网络存储基础
trigger_keywords:
- 容器技术
- Linux
- 系统与网络存储基础
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 容器技术、Linux 系统与网络存储基础

> **CNCF 状态**: 基础知识 | **类别**: Linux Fundamentals | **主要语言**: Shell, YAML

## 概述

Kubernetes 底层的 Linux 基础是理解容器和 K8s 工作原理的关键知识领域。它涵盖 Linux Namespace（命名空间隔离）、Cgroups（资源限制）、UnionFS（联合文件系统）、Seccomp/AppArmor/SELinux（安全策略）、网络虚拟化（veth、bridge、iptables/eBPF）等核心技术。理解这些 Linux 内核机制对于 K8s 运维排障、性能调优和安全加固至关重要。本文档系统梳理容器化工作负载依赖的核心 Linux 技术栈。

## Key Features（核心能力）

- **Namespace 隔离**：PID、Network、Mount、UTS、IPC、User 六大命名空间提供容器隔离
- **Cgroups v2**：统一的资源控制层级，管理 CPU、内存、IO、PID 等资源限制
- **UnionFS**：OverlayFS/AUFS 提供镜像分层存储机制
- **网络虚拟化**：veth pair、bridge、iptables/eBPF 构建容器网络
- **安全机制**：Seccomp、AppArmor、SELinux 提供多层安全防护
- **存储管理**：Device Mapper、LVM、OverlayFS 等存储驱动

## 架构与工作原理

容器技术的 Linux 基础由三个核心机制构成：Namespace 提供 process 级别的隔离（进程看到的系统环境是独立的）；Cgroups 提供资源限制和计量（限制 CPU、内存、IO 等）；UnionFS 提供镜像分层存储（只读层 + 可写层叠加）。网络方面，每个容器通过 veth pair 连接到虚拟网桥，通过 iptables 或 eBPF 程序实现 Service 代理和 NetworkPolicy。

## K8s 集成

Kubernetes 完全依赖这些 Linux 机制运行：kubelet 通过 CRI 调用 containerd，containerd 通过 runc 创建基于 Namespace+Cgroups 的容器；kube-proxy 通过 iptables/ipvs/eBPF 实现 Service 负载均衡；Pod Security Standards 通过 Seccomp/AppArmor 策略约束容器行为；CSI 驱动通过 Linux 块设备和文件系统提供存储。

## 生产用例

- **容器排障**：通过 nsenter、ip netns 等工具进入容器网络命名空间诊断网络问题
- **性能调优**：通过 cgroups 和 CPU Manager 优化容器性能
- **安全加固**：配置 Seccomp/AppArmor 策略限制容器权限
- **网络理解**：理解 Pod 间通信链路（veth → bridge → iptables → Service）

## 安装与快速开始

```bash
# 进入容器的 network namespace
nsenter -t $(docker inspect -f '{{.State.Pid}}' mycontainer) -n ip addr

# 查看 cgroups
cat /sys/fs/cgroup/cpu/kubepods/burstable/*/cpu.cfs_quota_us

# 查看容器 namespace
lsns -t net,pid,mnt
```

## 对比替代方案

相比 VM 级隔离（KVM/Hyper-V），Linux 容器隔离依赖内核共享，隔离性较弱但效率更高。Kata Containers/gVisor 通过轻量级 VM 在两者间取得平衡。

## Related

- [[docker]] — Docker
- [[cilium]] — Cilium
- [[containerd]] — containerd


<!-- risk-assessed -->
