---
title: 'Domain 35: eBPF 技术体系 (eBPF Technology Stack)'
description: '**适用范围**: 云原生网络、安全、可观测性 | **维护状态**: 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐ | **更新时间**:
  2026-03-03'
summary: '**适用范围**: 云原生网络、安全、可观测性 | **维护状态**: 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐ | **更新时间**:
  2026-03-03'
category: ebpf-technology
tags:
- k8s
- ebpf
- cilium
- networking
- observability
- kafka
- networkpolicy
- operator
- rag
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- SRE
- 网络工程师
- 内核工程师
estimated_read_time: 5min
intent_queries:
- 'Domain 35: eBPF 技术体系 (eBPF Technology Stack) 是什么'
- '如何 Domain 35: eBPF 技术体系 (eBPF Technology Stack)'
- Kubernetes 35 ebpf technology 最佳实践
trigger_keywords:
- Domain
- '35:'
- eBPF
- 技术体系
- eBPF
- Technology
- Stack
- ebpf
prerequisites:
- kubectl-basics
- networking-basics
- ebpf-basics
- cilium-basics
- kafka-basics
---



# Domain 35: eBPF 技术体系 (eBPF Technology Stack)

> **适用范围**: 云原生网络、安全、可观测性 | **维护状态**: 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐ | **更新时间**: 2026-03-03

## 📋 领域概览

eBPF (Extended Berkeley Packet Filter) 是 Linux 内核中的革命性技术，允许在内核空间安全运行沙箱程序，无需修改内核源码或加载内核模块。本领域深入探讨 eBPF 在云原生环境中的应用，涵盖 Cilium CNI、Tetragon 运行时安全、Hubble 网络可观测性等核心技术栈。

## 📚 文档目录

### 🎯 eBPF 基础架构 (01-02)
- **[01-eBPF架构基础与程序类型](./01-ebpf-architecture-fundamentals.md)** - eBPF 虚拟机、程序类型、验证器、JIT编译
- **[02-eBPF Map类型与数据结构](./02-ebpf-map-types-data-structures.md)** - Hash/Array/LRU/RingBuffer/Per-CPU Map

### 🌐 Cilium CNI 深度实践 (03-05)
- **[03-Cilium CNI架构与部署](./03-cilium-cni-architecture.md)** - Cilium Agent/Operator/CNI Plugin 架构
- **[04-Cilium网络策略L3/L4/L7](./04-cilium-network-policy.md)** - CiliumNetworkPolicy、L7 HTTP/gRPC/Kafka 策略
- **[05-Cilium Service Mesh无Sidecar](./05-cilium-service-mesh.md)** - Cilium Service Mesh、eBPF 替代 Sidecar

### 🔒 安全与可观测性 (06-08)
- **[06-Tetragon运行时安全](./06-tetragon-runtime-security.md)** - TracingPolicy、进程/文件/网络监控
- **[07-Hubble网络可观测性](./07-hubble-network-observability.md)** - Hubble UI/CLI/Relay、L3/L4/L7 流可视化
- **[08-bcc与bpftrace工具链](./08-bcc-bpftrace-tools.md)** - bcc 工具、bpftrace 脚本、性能分析

### ⚡ 性能与安全应用 (09-10)
- **[09-eBPF性能优化实践](./09-ebpf-performance-optimization.md)** - XDP 加速、TC 优化、Map 性能调优
- **[10-eBPF安全应用案例](./10-ebpf-security-applications.md)** - 入侵检测、DDoS 防护、容器逃逸检测

## 🎯 学习路径建议

### 🔰 eBPF 入门
1. **01-eBPF架构基础** → 理解 eBPF 核心概念
2. **02-Map类型** → 掌握数据结构与通信机制
3. **08-bcc工具链** → 动手实践 eBPF 开发

### ⭐ Cilium 网络工程师
1. **03-Cilium架构** → 部署与配置 Cilium CNI
2. **04-网络策略** → 实施 L3/L4/L7 策略
3. **07-Hubble** → 建立网络可观测性

### 🔒 安全工程师
1. **06-Tetragon** → 运行时安全监控
2. **10-安全应用** → 威胁检测与响应
3. **09-性能优化** → 大规模安全部署

## 📊 技术深度对比

| 文档 | 技术深度 | 实践价值 | 适用场景 | 复杂度 |
|------|----------|----------|----------|--------|
| 01-eBPF架构 | ⭐⭐⭐⭐⭐ | 高 | 内核开发、深度理解 | 高 |
| 02-Map类型 | ⭐⭐⭐⭐⭐ | 高 | eBPF 程序开发 | 高 |
| 03-Cilium架构 | ⭐⭐⭐⭐⭐ | 很高 | CNI 部署、网络架构 | 中高 |
| 04-网络策略 | ⭐⭐⭐⭐⭐ | 很高 | 网络安全、微隔离 | 中高 |
| 05-Service Mesh | ⭐⭐⭐⭐ | 高 | 服务网格、无 Sidecar | 中 |
| 06-Tetragon | ⭐⭐⭐⭐⭐ | 很高 | 运行时安全 | 中高 |
| 07-Hubble | ⭐⭐⭐⭐ | 很高 | 网络可观测性 | 中 |
| 08-bcc工具 | ⭐⭐⭐⭐ | 高 | 性能分析、调试 | 中高 |
| 09-性能优化 | ⭐⭐⭐⭐⭐ | 高 | 大规模部署 | 高 |
| 10-安全应用 | ⭐⭐⭐⭐⭐ | 很高 | 安全运营 | 高 |

## 🔧 核心技术栈

```bash
# eBPF 核心组件
Cilium CNI (CNCF Graduated)     # 云原生网络
Tetragon (CNCF Sandbox)         # 运行时安全
Hubble                          # 网络可观测性
bcc/bpftrace                    # 开发工具链

# 内核要求
Linux Kernel >= 5.10            # 基础 eBPF 功能
Linux Kernel >= 5.15            # BTF 支持
Linux Kernel >= 6.1             # 高级特性
```

## 📚 相关领域链接

- **[Domain-5: 网络基础](../domain-03-networking-traffic)** - Kubernetes 网络架构
- **[Domain-7: 安全](../domain-05-security-compliance)** - 安全架构基础
- **[Domain-8: 可观测性](../domain-06-observability)** - 监控体系
- **[Domain-19: 高级论文](../domain-19-papers)** - eBPF 与 Cilium 深度实践

---
*本文档由云原生技术专家团队维护，内容基于 2026 年 eBPF 生态最新实践。*

## Related

- [[README]]
