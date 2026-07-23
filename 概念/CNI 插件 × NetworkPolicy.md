---
title: CNI 插件 × NetworkPolicy
description: '[[实体/cni-plugins.md|cni plugins]] 实现 Pod 的网络连接，[[实体/networkpolicy.md|networkpolicy]]
  定义 Pod 级别的网络安全策略。wiki 将两者分属网络实现层和安全策略层，但它们是** inseparable 的共生关系**：NetworkPolicy
  只是 K8s API 中的一个资源定义——它描述了"哪些 Pod 可以通信"的意图，但完全没有定义"如何实现这'
summary: '[[实体/cni-plugins.md|cni plugins]] 实现 Pod 的网络连接，[[实体/networkpolicy.md|networkpolicy]]
  定义 Pod 级别的网络安全策略。wiki 将两者分属网络实现层和安全策略层，但它们是** inseparable 的共生关系**：NetworkPolicy
  只是 K8s API 中的一个资源定义—...'
category: synthesis
tags:
- k8s
- cni
- networkpolicy
- security
- networking
- firewall
- microsegmentation
- kubelet
- istio
- cilium
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNI 插件 × NetworkPolicy 是什么
- 如何 CNI 插件 × NetworkPolicy
trigger_keywords:
- CNI
- 插件
- NetworkPolicy
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
- cni-basics
relationships:
- target: '[[实体/cilium.md]]'
  type: uses
- target: '[[实体/istio.md]]'
  type: uses
- target: '[[实体/kubelet.md]]'
  type: uses
- target: '[[系统基础/知识字典/networking/service-mesh.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CNI 插件 × NetworkPolicy

## 连接点

[[实体/cni-plugins.md|cni plugins]] 实现 Pod 的网络连接，[[实体/networkpolicy.md|networkpolicy]] 定义 Pod 级别的网络安全策略。wiki 将两者分属网络实现层和安全策略层，但它们是** inseparable 的共生关系**：NetworkPolicy 只是 K8s API 中的一个资源定义——它描述了"哪些 Pod 可以通信"的意图，但完全没有定义"如何实现这种隔离"。真正执行策略的是 CNI 插件：Calico 用 iptables/eBPF 实现，[[实体/cilium.md|Cilium]] 用 eBPF 实现，Flannel 完全不实现。

这个分离设计是 K8s 网络模型的核心特征：**策略声明与实现解耦**。它允许用户在不改变策略定义的情况下切换 CNI 插件，但也带来了严重的碎片化问题——同一个 NetworkPolicy YAML 在 Calico 和 Cilium 下的行为可能不同，在 Flannel 下则完全无效。

## 共现场景

两者在以下场景中共现：

- **默认拒绝策略**：应用 default-deny NetworkPolicy 后，只有支持 NetworkPolicy 的 CNI（Calico、Cilium、Weave）才能真正阻断流量。使用 Flannel 的集群应用了 default-deny 后，Pod 之间仍然自由通信——策略变成了虚假的安全感
- **L3/L4 策略**：标准 NetworkPolicy 只支持基于 IP/CIDR 和端口的规则。Calico 通过扩展 CRD（GlobalNetworkPolicy）支持更丰富的策略，Cilium 通过 CiliumNetworkPolicy 支持 L7 策略——但这些都是厂商特定的扩展，不是标准 NetworkPolicy
- **日志与审计**：NetworkPolicy 的 deny 行为默认不生成日志。Cilium 通过 Hubble 提供策略命中可视化，Calico 通过 Felix 日志提供审计——但标准 NetworkPolicy API 没有暴露这些可观测性接口
- **策略冲突排障**：当 Pod 无法通信时，问题可能在 NetworkPolicy（规则配置错误）、CNI（策略未正确下发到数据面）、或者两者之间的同步延迟。缺乏统一的排障工具，工程师需要同时理解策略语义和 CNI 实现细节

## 交叉洞察

**核心洞察：NetworkPolicy 的"可移植性"是一个美好的谎言——现实中策略行为高度依赖 CNI 实现。**

K8s 文档声称 NetworkPolicy 是"集群无关的"，但生产实践表明：

| 功能 | Calico | Cilium | Flannel | 标准 NetworkPolicy |
|------|--------|--------|---------|-------------------|
| L3/L4 策略 | 支持 | 支持 | 不支持 | 支持 |
| L7 策略 | 不支持 | CiliumNetworkPolicy | 不支持 | 不支持 |
| 日志/审计 | Felix 日志 | Hubble | 无 | 无 |
| 性能 | iptables O(n) | eBPF O(1) | N/A | N/A |
| 策略数量上限 | iptables 链限制 | eBPF Map 限制 | N/A | N/A |
| 多集群策略 | GlobalNetworkPolicy | CiliumClusterwidePolicy | 不支持 | 不支持 |

**这意味着：迁移 CNI 不仅仅是网络连通性的迁移，更是安全策略的重新实现。**

**零信任网络的实现悖论：**

零信任要求"默认拒绝所有，显式允许必要"。但在 K8s 中：
1. NetworkPolicy 的默认行为是**允许所有**（没有策略 = 全部放行）
2. 要实现零信任，必须先应用 default-deny 策略
3. 但 default-deny 只在支持 NetworkPolicy 的 CNI 下有效
4. 因此，零信任的前提不是"应用了策略"，而是"使用了正确的 CNI"

这揭示了 K8s 网络安全的一个根本性设计缺陷：**安全基线不是内置的，而是可选的插件功能**。

**CNI 作为安全边界的可靠性问题：**

NetworkPolicy 依赖 CNI 插件正确实现策略。但 CNI 插件本身：
- 可能因升级而引入策略执行漏洞（如 Calico 某版本中的 iptables 规则竞态）
- 可能因节点资源耗尽而无法更新策略（[[实体/kubelet.md|kubelet]] 在 CPU 压力下跳过 CNI 调用）
- 可能在节点重启后短暂失效（CNI 守护进程启动前，新 Pod 可能无策略运行）

**NetworkPolicy 不是防火墙，而是"有最佳努力的建议"。**

## 张力与权衡

| 张力 | 详情 |
|------|------|
| **标准 vs 扩展** | 标准 NetworkPolicy 功能有限（无 L7、无日志、无审计）。厂商扩展（CiliumNetworkPolicy、Calico GlobalNetworkPolicy）功能强大但不兼容。选择标准意味着功能受限，选择扩展意味着厂商锁定 |
| **策略下发延迟** | NetworkPolicy 变更后，CNI 需要将其转化为数据面规则（iptables/eBPF）。在大规模集群中，这个下发过程可能需要数秒到数分钟。在此期间，新旧策略可能共存或冲突 |
| **CNI 升级的策略盲区** | CNI 插件升级期间，策略执行可能短暂中断。如果升级过程中新 Pod 被创建，它们可能在无策略保护的状态下运行。滚动升级 CNI 是集群运维中最危险的操作之一 |
| **跨命名空间策略的复杂度** | NetworkPolicy 的 namespaceSelector 允许基于标签匹配命名空间，但命名空间标签的变更不会自动触发策略重新评估。这导致"策略允许 namespace A 访问，但 namespace A 被重新标记后策略仍然有效"的意外行为 |
| **IPBlock 与动态 IP 的冲突** | NetworkPolicy 的 ipBlock 使用 CIDR 范围，但云厂商的负载均衡器、数据库服务的 IP 可能动态变化。使用 ipBlock 允许外部访问的策略在目标 IP 变化后将失效 |

## 开放问题

- **NetworkPolicy 的一致性测试**：K8s 社区缺乏官方的 NetworkPolicy 一致性测试套件。不同 CNI 对同一策略的实现差异（如 ICMP 处理、连接跟踪超时）没有标准化。是否应该有一个类似 CNI 一致性认证的网络安全认证？
- **策略即代码的验证**：NetworkPolicy 作为 YAML 可以通过 CI/CD 部署，但策略冲突检测（两个策略互相矛盾）、可达性分析（哪些 Pod 实际可以通信）需要专门工具（如 network-policy-api 的模拟器）。这些工具与 CNI 的绑定程度如何？
- **CNI 问题时的安全降级**：当 CNI 守护进程问题时，kubelet 仍然可以创建 Pod（使用默认网络配置）。这些 Pod 将完全绕过 NetworkPolicy。如何检测和响应这种安全降级？是否应该有一个独立的策略执行监督层？
- **eBPF 对策略执行的统一**：Cilium 的 eBPF 实现展示了 L3/L4/L7 策略统一执行的可能性。未来是否所有 CNI 都会转向 eBPF？iptables 在策略数量超过阈值后的性能劣化是否会迫使 Calico 等插件全面 eBPF 化？
- **NetworkPolicy 与 [[系统基础/知识字典/networking/service-mesh.md|Service Mesh]] 策略的层级关系**：NetworkPolicy（CNI 层，L3/L4）与 [[实体/istio.md|Istio]] AuthorizationPolicy（Service Mesh 层，L7）同时存在时，哪个优先？两者的冲突如何检测？当前没有标准工具可以回答"这个请求是否会被允许"的问题

## 相关

- [[实体/cni-plugins.md|cni plugins]]
- [[实体/networkpolicy.md|networkpolicy]]
- [[概念/service-networking.md|service networking]]
- [[概念/security-defense-depth.md|security defense depth]]
- [[实体/cilium.md|cilium]]
- [[概念/cilium-ebpf-networking.md|cilium ebpf networking]]
- [[概念/服务网格 × 零信任安全.md|服务网格 x 零信任安全]]

> *This page synthesizes patterns across multiple sources and domains.* ^[inferred]

## See Also

- [[概念/纵深防御 × 供应链安全.md|纵深防御 x 供应链安全]]
- [[概念/CI-CD 流水线 × Secret 管理.md|CI-CD 流水线 × Secret 管理]]
- [[概念/CRD × 可观测性.md|CRD × 可观测性]]
- [[概念/Cilium eBPF × 可观测性.md|Cilium eBPF × 可观测性]]
## Related

- [[系统基础/知识字典/fundamentals/namespaces.md|命名空间]]


<!-- risk-assessed -->
