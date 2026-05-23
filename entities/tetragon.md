---
title: Tetragon
description: Tetragon — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- security
- runtime
- ebpf
- tetragon
- monitoring
- cilium
- falco
- networkpolicy
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Tetragon 是什么
- 如何 Tetragon
trigger_keywords:
- Tetragon
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
created: "2026-05-23"
---

# Tetragon

Tetragon (by Isovalent/Cilium team) uses eBPF for real-time security enforcement and observability.

## Key Facts

- **Status**: CNCF Sandbox
- **Technology**: eBPF kernel tracing
- **Policy Format**: TracingPolicy (YAML CRD)
- **Focus**: Process execution, file access, network monitoring

## Detection Capabilities

| Capability | Description |
|------------|-------------|
| Process Execution | Track all process launches in containers |
| File Access | Monitor reads/writes to sensitive paths |
| Network Connections | Detect outbound connections to suspicious hosts |
| Capability Usage | Track privilege escalation via Linux capabilities |

## TracingPolicy

Custom security policies defined as K8s CRDs. Each policy specifies which kernel events to trace and what actions to take (log, enforce, notify).

## Related

- [[confidential-containers]] — Confidential Containers (CoCo)
- [[entities/networkpolicy|networkpolicy]] — NetworkPolicy
- [[bootc]] — bootc
- [[concepts/cilium-ebpf-networking|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[cilium]] — Cilium
- [[concepts/cilium-ebpf-networking|Cilium eBPF Networking]]
- [[falco|Falco]]
- [[cilium|Cilium]]

- 06-tetragon-runtime-security