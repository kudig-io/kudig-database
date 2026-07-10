---
title: Tetragon
description: Tetragon — Kubernetes 生产运维知识库
summary: Tetragon — Kubernetes 生产运维知识库
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
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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
- [[实体/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[bootc]] — bootc
- [[概念/cilium-ebpf-networking.md|cilium-ebpf-networking]] — Cilium eBPF Networking
- [[cilium]] — Cilium
- [[概念/cilium-ebpf-networking.md|Cilium eBPF Networking]]
- [[falco|Falco]]
- [[cilium|Cilium]]

- 06-tetragon-runtime-security

<!-- risk-assessed -->
