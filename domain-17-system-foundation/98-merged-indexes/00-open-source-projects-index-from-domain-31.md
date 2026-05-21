---
title: Domain-31 硬件 — 开源项目索引
description: '| **Cluster API Provider Metal3** | 裸金属 K8s 管理 | CNCF Incubating | v1.9.0 | 300+ | Apache-2.0 |'
category: hardware
tags:
- k8s
- hardware
- server
- gpu
- network
- operator
- nvidia
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 基础设施工程师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Domain-31 硬件 — 开源项目索引 是什么
- 如何 Domain-31 硬件 — 开源项目索引
- Kubernetes 31 hardware 最佳实践
trigger_keywords:
- Domain-31
- 硬件
- 开源项目索引
- hardware
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# Domain-31 硬件 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **MetalLB** | 裸金属 LoadBalancer | 社区 | v0.14.0 | 7k+ | Apache-2.0 |
| **kube-vip** | 高可用虚拟 IP | 社区 | v0.8.0 | 2k+ | Apache-2.0 |
| **Keepalived** | LVS 高可用 | 社区 | v2.3.0 | 2k+ | GPL-2.0+ |
| **Cluster API Provider Metal3** | 裸金属 K8s 管理 | CNCF Incubating | v1.9.0 | 300+ | Apache-2.0 |
| **Tinkerbell** | 裸金属工作流引擎 | 非 CNCF | v0.10.0 | 1k+ | Apache-2.0 |
| **MAAS** | 裸金属自动化 (Canonical) | Canonical | v3.6.0 | 3k+ | AGPL-3.0 |
| **OpenStack Ironic** | 裸金属即服务 | OpenStack | v27.0.0 | 1k+ | Apache-2.0 |
| **NVIDIA GPU Operator** | GPU 驱动与管理 | NVIDIA | v24.9.0 | 2k+ | Apache-2.0 |
| **AMD GPU Operator** | AMD GPU K8s 管理 | AMD | v1.2.0 | 200+ | MIT |
| **Intel GPU Plugin** | Intel GPU 设备插件 | Intel | v0.32.0 | 500+ | Apache-2.0 |
| **SR-IOV Network Operator** | SR-IOV 网络虚拟化 | Intel | v1.4.0 | 500+ | Apache-2.0 |
| **Node Feature Discovery** | 硬件特性发现 | K8s SIG | v0.17.0 | 1k+ | Apache-2.0 |
| **Intel Device Plugins** | Intel 硬件设备插件集 | Intel | v0.32.0 | 500+ | Apache-2.0 |
| **DPDK** | 数据平面开发套件 | Intel/Linux | v24.11.0 | 1k+ | BSD-3 |
| **SPDK** | 存储性能开发套件 | Intel/Linux | v24.09.0 | 3k+ | BSD-3 |

---

## 参考链接

- [MetalLB 文档](https://metallb.io/)
- [kube-vip 文档](https://kube-vip.io/)
- [Tinkerbell 文档](https://tinkerbell.org/)
- [NVIDIA GPU Operator](https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/)

---

## Obsidian 相关文档

- [[domain-17-system-foundation/MOC.md|domain-31-hardware MOC]]
- [[domain-17-system-foundation/README.md|Domain 31 - 硬件基础设施]]
- [[domain-17-system-foundation/01-cloud-hardware-architecture.md|云平台硬件基础架构]]
- [[domain-17-system-foundation/02-server-architecture-principles.md|服务器架构原理]]
- [[domain-17-system-foundation/03-cpu-technology-deep-dive.md|CPU技术深度解析]]
- [[domain-17-system-foundation/04-motherboard-chipset-technology.md|主板与芯片组技术]]
- [[domain-17-system-foundation/05-memory-technology-deep-dive.md|内存技术深度解析]]
- [[domain-17-system-foundation/06-storage-hdd-technology.md|机械硬盘技术]]
- [[domain-17-system-foundation/07-storage-ssd-technology.md|SSD固态硬盘技术]]
- [[domain-17-system-foundation/08-network-hardware-technology.md|网络硬件技术]]
- [[domain-17-system-foundation/09-hardware-vendors-ecosystem.md|硬件厂商生态]]
- [[domain-17-system-foundation/10-hardware-troubleshooting-methodology.md|硬件故障排查方法论]]
