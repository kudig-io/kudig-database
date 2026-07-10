---
title: Cilium eBPF Networking
description: '- [[概念/eBPF x 运行时安全.md|eBPF x 运行时安全]] — synthesis'
summary: '- [[概念/eBPF x 运行时安全.md|eBPF x 运行时安全]] — synthesis'
category: concepts
tags:
- k8s
- ebpf
- cilium
- networking
- security
- hubble
- tetragon
- kubelet
- envoy
- kafka
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cilium eBPF Networking 是什么
- 如何 Cilium eBPF Networking
trigger_keywords:
- Cilium
- eBPF
- Networking
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- kafka-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cilium eBPF Networking

## What is eBPF

eBPF (Extended Berkeley Packet Filter) is a revolutionary Linux kernel technology that allows running sandboxed programs in kernel space without modifying kernel source code or loading kernel modules. A verifier ensures programs are safe before execution, and a JIT compiler optimizes them to native machine code.

## eBPF Architecture

eBPF programs attach to kernel hooks at:
- **XDP** (eXpress Data Path): Earliest packet processing point, before network driver
- **TC** (Traffic Control): After network driver, before protocol stack
- **Socket**: At socket layer for connection monitoring
- **Kprobe/Tracepoint**: At kernel function entry/exit for syscall monitoring
- **Cgroup**: At cgroup level for container-level monitoring

Data passes between kernel and user space through eBPF Maps (Hash, Array, LRU, RingBuffer, Per-CPU variants).

## Cilium CNI Architecture

Cilium replaces iptables-based networking with eBPF programs:

| Component | Role |
|-----------|------|
| Cilium Agent | [[DaemonSet|DaemonSet]] per node, programs eBPF into kernel |
| Cilium Operator | Cluster-wide operations (IPAM, node management) |
| CNI Plugin | Integrates with kubelet for Pod networking |
| eBPF dataplane | In-kernel packet processing, policy enforcement |
| Hubble Relay | Collects and serves network flow telemetry |

### L3/L4/L7 Network Policies

CiliumNetworkPolicy extends K8s NetworkPolicy to L7 (HTTP, gRPC, Kafka):
- L3: IP-based policies (source/destination CIDR)
- L4: Port/protocol-based policies (TCP/UDP/SCTP)
- L7: Application-layer policies (HTTP path, method, gRPC service, Kafka topic)

### Cilium Service Mesh

Cilium provides a sidecar-less service mesh using eBPF for L4 mTLS and optional Envoy proxy for L7 processing. Performance advantage over sidecar meshes: lower memory, lower latency, no per-Pod proxy overhead.

## Tetragon Runtime Security

Tetragon uses eBPF for real-time runtime security monitoring:
- **Process execution monitoring**: Detect unauthorized process launches in containers
- **File access monitoring**: Track sensitive file reads/writes
- **Network monitoring**: Detect anomalous network connections
- **TracingPolicy**: Declarative policy format for custom security event detection

## Hubble Network Observability

Hubble provides L3/L4/L7 flow visibility:
- **Hubble CLI**: Command-line flow analysis
- **Hubble UI**: Visual service dependency map and flow exploration
- **Hubble Relay**: Aggregates flow data from all nodes

## Kernel Requirements

| Feature | Minimum Kernel |
|---------|---------------|
| Basic eBPF | 5.10 |
| BTF (BPF Type Format) | 5.15 |
| Advanced features | 6.1+ |

## Related
- [[概念/Cilium eBPF × 可观测性.md|Cilium eBPF × 可观测性]] — 综合

- [[实体/tetragon.md|tetragon]] — Tetragon
- [[grpc]] — gRPC
- [[cni]] — CNI (Container Network Interface)
- [[概念/tcp-udp-protocol-stack.md|tcp-udp-protocol-stack]] — TCP/UDP Protocol Stack
- [[概念/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[概念/service-mesh-architecture.md|Service Mesh Architecture]]
- [[概念/tcp-udp-protocol-stack.md|TCP/UDP Protocol Stack]]
- [[概念/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[cilium|Cilium]]
- [[实体/tetragon.md|Tetragon]]
- Hubble
- [[概念/eBPF x 运行时安全.md|eBPF x 运行时安全]] — synthesis

- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- RELEASE-NOTES-1.18
- RELEASE-NOTES-1.19
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- RELEASE-NOTES-1.16
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-1.17
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.11
- [[实体/inspektor-gadget.md|Inspektor Gadget]] — Cross-reference


<!-- risk-assessed -->
