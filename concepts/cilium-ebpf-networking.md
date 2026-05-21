---
title: Cilium eBPF Networking
description: '- [[synthesis/eBPF x 运行时安全.md|eBPF x 运行时安全]] — synthesis'
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
| Cilium Agent | DaemonSet per node, programs eBPF into kernel |
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
- [[synthesis/Cilium eBPF × 可观测性.md|Cilium eBPF × 可观测性]] — 综合

- [[entities/tetragon.md|tetragon]] — Tetragon
- [[grpc]] — gRPC
- [[cni]] — CNI (Container Network Interface)
- [[concepts/tcp-udp-protocol-stack.md|tcp-udp-protocol-stack]] — TCP/UDP Protocol Stack
- [[concepts/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[concepts/service-mesh-architecture.md|Service Mesh Architecture]]
- [[concepts/tcp-udp-protocol-stack.md|TCP/UDP Protocol Stack]]
- [[concepts/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[cilium|Cilium]]
- [[entities/tetragon.md|Tetragon]]
- Hubble
- [[synthesis/eBPF x 运行时安全.md|eBPF x 运行时安全]] — synthesis

- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.9.md|RELEASE-NOTES-1.9]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-0.8.md|RELEASE-NOTES-0.8]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.18.md|RELEASE-NOTES-1.18]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.19.md|RELEASE-NOTES-1.19]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.8.md|RELEASE-NOTES-1.8]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-0.9.md|RELEASE-NOTES-0.9]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.16.md|RELEASE-NOTES-1.16]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.3.md|RELEASE-NOTES-1.3]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.7.md|RELEASE-NOTES-1.7]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.12.md|RELEASE-NOTES-1.12]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.6.md|RELEASE-NOTES-1.6]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.13.md|RELEASE-NOTES-1.13]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.17.md|RELEASE-NOTES-1.17]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.2.md|RELEASE-NOTES-1.2]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.5.md|RELEASE-NOTES-1.5]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.10.md|RELEASE-NOTES-1.10]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.14.md|RELEASE-NOTES-1.14]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.1.md|RELEASE-NOTES-1.1]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.15.md|RELEASE-NOTES-1.15]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.0.md|RELEASE-NOTES-1.0]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.4.md|RELEASE-NOTES-1.4]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-1.11.md|RELEASE-NOTES-1.11]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-0.10.md|RELEASE-NOTES-0.10]]
- [[domain-19-landscape-references/topic-release-notes/networking/cilium/RELEASE-NOTES-0.11.md|RELEASE-NOTES-0.11]]
- [[entities/inspektor-gadget|Inspektor Gadget]] — Cross-reference
