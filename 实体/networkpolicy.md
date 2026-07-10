---
title: NetworkPolicy
description: NetworkPolicy — Kubernetes 生产运维知识库
summary: NetworkPolicy — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- networkpolicy
- security
- firewall
- network-isolation
- cilium
- flannel
- calico
- ingress
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- NetworkPolicy 是什么
- 如何 NetworkPolicy
trigger_keywords:
- NetworkPolicy
prerequisites:
- kubectl-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# NetworkPolicy

## Role

NetworkPolicy is a Kubernetes resource that defines how [[Pods|Pods]] communicate with each other and external network endpoints. It acts as a Pod-level firewall.

**Important**: NetworkPolicy requires CNI plugin support (Calico, Cilium, or other compatible CNIs). Flannel does NOT support NetworkPolicy.

## Policy Structure

NetworkPolicy selects target Pods via `podSelector` and defines:

| Policy Type | Controls |
|-------------|----------|
| **[[Ingress|Ingress]]** | Incoming traffic to selected Pods |
| **Egress** | Outgoing traffic from selected Pods |

Traffic sources/destinations can be specified via:
- `podSelector`: Match Pods by labels (same namespace by default)
- `namespaceSelector`: Match namespaces by labels
- `ipBlock`: CIDR ranges (with optional exceptions)

## Default-Deny Pattern

Apply a default-deny policy to establish zero-trust networking:

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
spec:
  podSelector: {}  # Selects all Pods
  policyTypes:
  - Ingress
  - Egress
```

Then add explicit allow policies for required traffic flows.

## Use Cases

- Isolate tenant namespaces from each other
- Restrict database access to specific application Pods
- Allow egress only to required external endpoints
- Microsegmentation for PCI-DSS or HIPAA compliance

## Related

- [[cilium]] — Cilium
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/service-networking.md|service-networking]] — Service Networking
- [[概念/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security
- [[概念/service-networking.md|Service Networking]]
- [[实体/cni-plugins.md|CNI Plugins]]
- [[概念/security-defense-depth.md|Defense-in-Depth Security]]

- [[概念/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]]
- 22-networkpolicy-reference
- 16-networkpolicy-deep-practice
- [[故障诊断/资源排障/16-networkpolicy-troubleshooting.md|16-networkpolicy-troubleshooting]]
- [[故障诊断/FTA故障树/list/networkpolicy-fta.md|NetworkPolicy 异常故障树分析]]
- [[故障诊断/高级排障/03-networking/04-networkpolicy-troubleshooting.md|04-networkpolicy-troubleshooting]]
- [[技能/networkpolicy-fta.md|NetworkPolicy 异常故障树分析]] — Cross-reference


<!-- risk-assessed -->
