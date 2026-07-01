---
title: 容器技术、Linux 系统与网络存储基础
description: '# 容器技术、Linux 系统与网络存储基础'
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
last_updated: 2026-05
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
created: "2026-05-23"
---

# 容器技术、Linux 系统与网络存储基础

## [[docker]]/containerd 架构

Docker 架构演进：
- Docker 1.x：docker daemon 单体
- Docker 18.09+：containerd 独立进程
- K8s 1.24+：dockershim 移除，原生 CRI 接口

containerd 是 CNCF 毕业项目，K8s 默认容器运行时。

## Linux 容器基础

三大核心技术：
- **Namespace**：隔离（PID/NET/MNT/UTS/IPC/USER/CGROUP/TIME）
- **Cgroups**：资源限制（CPU/Memory/IO/PID 数量）
- **OverlayFS**：分层文件系统，镜像层共享

## 网络基础

| 概念 | 作用 |
|------|------|
| VLAN | 二层隔离 |
| VXLAN | 大二层 overlay 网络 |
| BGP | 路由协议，Calico 使用 |
| iptables/nftables | 包过滤/转发 |
| eBPF | 内核可编程，Cilium 使用 |

## 存储基础

三类存储模型：
- **块存储**：Raw device，高性能（EBS/Ceph RBD）
- **文件存储**：POSIX 接口，共享访问（NFS/CephFS）
- **对象存储**：HTTP API，海量数据（S3/OSS/MinIO）

---

> 来源：.zread/wiki/drafts/23-docker-*.md, .zread/wiki/drafts/24-linux-*.md

## Related

- [[docker]] — Docker
- [[cilium]] — Cilium
- [[containerd]] — containerd
