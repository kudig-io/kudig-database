---
title: 硬件知识体系、CNCF 全景生态与 eBPF 平台工程
description: '## 硬件故障排查'
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
last_updated: 2026-05
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
created: "2026-05-23"
---

# 硬件知识、CNCF 生态与 eBPF 平台工程

## 硬件故障排查

| 组件 | 常见故障 | 排查工具 |
|------|----------|----------|
| CPU | 过热、锁死、软错误 | `lscpu`, `mpstat`, `perf` |
| 内存 | ECC 错误、DIMM 故障 | `edac-util`, `mcelog`, `dmidecode` |
| 磁盘 | 坏道、SMART 预警 | `smartctl`, `iostat`, `blktrace` |
| 网卡 | CRC 错误、丢包 | `ethtool`, `tcpdump`, `ss` |

## CNCF 全景图

218 个开源项目分层：

| 阶段 | 项目数 | 代表项目 |
|------|--------|----------|
| Graduated | 30+ | Kubernetes, Prometheus, Envoy, etcd, [[helm]] |
| Incubating | 40+ | Argo, Backstage, Cilium, Istio, KubeEdge |
| Sandbox | 140+ | 新项目探索 |

## eBPF 技术

eBPF 在 K8s 生态中的应用：
- **网络**：Cilium（替代 kube-proxy/IPVS）
- **安全**：Tetragon（运行时安全可观测）
- **可观测性**：Hubble（网络流可视化）
- **性能分析**：bpftrace, Parca

## 边缘计算

- **KubeEdge**：CNCF 毕业，云边协同
- **SuperEdge**：腾讯开源
- **OpenYurt**：阿里开源，无侵入式边缘增强

---

> 来源：.zread/wiki/drafts/25-*.md, .zread/wiki/drafts/26-*.md, .zread/wiki/drafts/27-*.md

## Related

- [[etcd]] — etcd
- [[envoy]] — Envoy
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[argo]] — Argo Workflows

- [[helm]]
- [[journal/digest-2026-05-21-full.md|digest-2026-05-21-full]]