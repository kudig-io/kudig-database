---
title: 硬件知识体系、CNCF 全景生态与 eBPF 平台工程
description: '## 硬件故障排查'
summary: '## 硬件故障排查'
category: reference
tags:
- k8s
- hardware
- cncf
- ebpf
- platform-engineering
- edge-computing
- webassembly
- etcd
- prometheus
- istio
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 硬件知识体系、CNCF 全景生态与 eBPF 平台工程 是什么
- 如何 硬件知识体系、CNCF 全景生态与 eBPF 平台工程
trigger_keywords:
- 硬件知识体系
- CNCF
- 全景生态与
- eBPF
- 平台工程
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 硬件知识、CNCF 生态与 eBPF 平台工程

## 概述

本篇涵盖 Kubernetes 运维所需的高级知识体系：硬件故障排查方法论、CNCF 全景生态分层、eBPF 技术在云原生中的应用以及边缘计算场景实践。这些知识是从初级 K8s 运维向高级平台工程迈进的关键。

## 硬件故障排查

在生产环境中，约 20% 的 K8s 节点问题最终追溯到硬件故障。系统化的硬件排查能力是高级 SRE 的必备技能。

| 组件 | 常见问题 | 排查工具 | 关键指标 |
|------|----------|----------|----------|
| CPU | 过热、锁死、软错误 | `lscpu`, `mpstat`, `perf` | temp, context switches |
| 内存 | ECC 错误、DIMM 故障 | `edac-util`, `mcelog`, `dmidecode` | edac_corrected/uncorrected |
| 磁盘 | 坏道、SMART 预警 | `smartctl`, `iostat`, `blktrace` | realloc sectors, UDMA CRC |
| 网卡 | CRC 错误、丢包 | `ethtool -S`, `tcpdump`, `ss` | rx_crc_errors, drops |

排查方法论：先 dmesg 查内核日志，再针对性使用工具定位具体硬件组件。

## CNCF 全景图

CNCF 生态拥有 200+ 个开源项目，按成熟度分层：

| 阶段 | 项目数 | 代表项目 | 特征 |
|------|--------|----------|------|
| Graduated | 30+ | Kubernetes, Prometheus, Envoy, etcd, Helm, Istio | 生产验证、成熟稳定 |
| Incubating | 40+ | Argo, Backstage, Cilium, KubeEdge, Strimzi | 社区活跃、快速增长 |
| Sandbox | 140+ | K8sGPT, SpinKube, Kmesh | 新项目探索、实验性 |

关键趋势：AIOps（K8sGPT/Kagent）、eBPF 无处不在（Cilium/Tetragon）、WASM 运行时（SpinKube）、机密计算（CoCo）、可持续计算（Kepler）。

## eBPF 技术应用

eBPF 在 K8s 生态中已渗透到网络、安全、可观测性和性能分析各个领域：

- **网络**：Cilium（替代 kube-proxy/IPVS，eBPF 数据平面），提供高性能 Pod 网络和 NetworkPolicy
- **安全**：Tetragon（运行时安全可观测和策略执行），实时检测异常行为
- **可观测性**：Hubble（网络流可视化）、Pixie（零侵入应用追踪）、Inspektor Gadget（调试工具集）
- **性能分析**：bpftrace（动态追踪）、Parca（持续 profiling）、Kepler（能耗监控）
- **存储**：bpftracer 追踪文件系统延迟

eBPF 优势：内核态执行无需上下文切换、无需修改内核或应用代码、极低开销（< 1% CPU）。

## 边缘计算

边缘计算是 K8s 向物联网和边缘场景的扩展：

- **KubeEdge**：CNCF Incubating，华为开源。云边协同架构，支持离线自治、设备管理（MQTT/Modbus）
- **OpenYurt**：CNCF Incubating，阿里开源。无侵入式边缘增强，NodePool 管理，跨地域部署
- **SuperEdge**：腾讯开源。分布式节点组、边缘自治、ServiceGroup
- **k0s/k3s**：轻量级 K8s 发行版，适合在资源受限的边缘节点运行

边缘场景核心挑战：网络不稳定（离线自治）、资源受限（轻量化）、设备管理（IoT 协议）、多地域管理（中心-边缘协同）。

## 平台工程趋势

Platform Engineering 是 K8s 生态的最新演进方向：
- **Internal Developer Platform (IDP)**：Backstage 构建开发者门户
- **Crossplane**：K8s 原生的云资源管理（IaC on K8s）
- **GitOps**：ArgoCD/FluxCD 声明式持续交付
- **FinOps**：OpenCost/Kubecost 成本可视化和优化

---

> 来源：.zread/wiki/drafts/25-*.md, .zread/wiki/drafts/26-*.md, .zread/wiki/drafts/27-*.md

## Related

- [[etcd]] — etcd
- [[envoy]] — Envoy
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[argo]] — Argo Workflows

- [[helm]]
- digest-2026-05-21-full

<!-- risk-assessed -->
