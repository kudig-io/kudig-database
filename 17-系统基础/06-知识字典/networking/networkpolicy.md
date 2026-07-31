---
title: 网络策略
description: NetworkPolicy 是 Kubernetes 中控制 Pod 之间以及 Pod 与外部网络之间流量的资源对象。它采用默认允许（default
  allow...
summary: NetworkPolicy 是 Kubernetes 中控制 Pod 之间以及 Pod 与外部网络之间流量的资源对象。它采用默认允许（default
  allow...
category: dictionary
tags:
- k8s
- glossary
- networkpolicy
- security
- cni
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 网络策略 是什么
- NetworkPolicy 详解
trigger_keywords:
- 网络策略
- NetworkPolicy
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 网络策略

> **英文名**: NetworkPolicy

## 概述

NetworkPolicy 是 Kubernetes 中控制 Pod 之间以及 Pod 与外部网络之间流量的资源对象。它采用默认允许（default allow）模型，通过定义 ingress 和 egress 规则实现网络隔离。

## 核心概念/原理

### 核心概念

- **Pod Selector**：选择策略作用的目标 Pod。
- **Ingress 规则**：控制入站流量（谁可以访问目标 Pod）。
- **Egress 规则**：控制出站流量（目标 Pod 可以访问谁）。
- **Policy Types**：指定 `Ingress`、`Egress` 或两者。

### 默认行为

| 场景 | 行为 |
|------|------|
| 无 NetworkPolicy | 允许所有流量 |
| 仅有 Ingress 策略 | 入站受限，出站不受限 |
| 同时有 Ingress + Egress | 双向受限 |

## 关键机制或特性

- NetworkPolicy 需要 CNI 插件支持（Calico、Cilium、Weave 等）。
- 不支持的 CNI 会静默忽略 NetworkPolicy 资源。
- 规则中的 `namespaceSelector` 和 `podSelector` 可以组合使用。
- `ipBlock` 支持 CIDR 匹配（除 `except` 子网外）。

## 使用场景与最佳实践

- 为每个命名空间创建默认 deny-all 策略，再按需放行。
- 使用标签选择器精确控制流量，避免过度宽松的策略。
- 定期审计 NetworkPolicy 覆盖情况，确保无遗漏。
- 生产环境建议配合 Cilium 或 Calico 的高级网络策略功能。

## 参考链接

- [NetworkPolicy - Kubernetes Docs](https://kubernetes.io/docs/concepts/services-networking/network-policies/)

## 架构深度解析

### 实现机制对比

```
┌─────────────────────────────────────────────────────┐
│           NetworkPolicy 实现方式                    │
├─────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │ Calico      │  │ Cilium       │  │ Antrea    │  │
│  │ (iptables/  │  │ (eBPF map)   │  │ (OVS      │  │
│  │  IPVS)      │  │              │  │  flow)    │  │
│  └─────────────┘  └──────────────┘  └───────────┘  │
├─────────────────────────────────────────────────────┤
│  K8s API: NetworkPolicy CR (spec.podSelector +     │
│           ingress/egress rules)                     │
└─────────────────────────────────────────────────────┘
```

### 各 CNI 实现路径

| CNI | 实现机制 | 性能特点 | L7 支持 |
|-----|----------|----------|----------|
| Calico | iptables/IPVS 规则 | 规则数线性增长 | 否 |
| Cilium | eBPF Policy Map | O(1) 查找，高性能 | 是（Envoy） |
| Antrea | OVS conjunctive flow | O(1) 匹配 | 部分 |
| OVN-K8s | OVN ACL + OVS flow | 分布式执行 | 否 |

### 策略评估流程（以 Cilium 为例）

1. Pod 创建 → Cilium 计算 Security Identity（基于标签）
2. NetworkPolicy 变更 → 编译为 BPF Policy Map 条目
3. 数据包到达 → TC eBPF 程序拦截
4. 查找 Policy Map（源 Identity + 目标 Identity + 端口）
5. 匹配允许 → 转发；匹配拒绝 → 丢弃 + 日志

## 生产案例

### 案例 1：默认拒绝策略导致 DNS 中断

| 时间 | 事件 |
|------|------|
| 14:00 | 应用部署默认拒绝 NetworkPolicy（deny-all） |
| 14:01 | 所有 Pod 无法解析 DNS，服务全面中断 |
| 14:05 | 确认：egress 规则未允许 UDP 53 到 kube-dns |
| 14:10 | 修复：添加 egress 规则允许 DNS 查询 |

**修复命令**：
```bash
# 检查 NetworkPolicy 🟢 只读
kubectl get networkpolicy -A -o wide
# 测试 DNS 解析 🟢 只读
kubectl exec -it test-pod -- nslookup kubernetes.default
# 添加 DNS egress 规则 🟡 中风险
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns
spec:
  podSelector: {}
  egress:
  - to:
    - namespaceSelector: {}
    ports:
    - protocol: UDP
      port: 53
EOF
```

### 案例 2：策略规则冲突导致流量异常

**现象**：两个 NetworkPolicy 同时匹配同一 Pod，行为不符合预期。

**诊断**：NetworkPolicy 是叠加（additive）模型，多个策略同时生效，任一允许即允许。

**修复**：使用 `kubectl describe networkpolicy` 分析所有匹配策略，合并或调整 podSelector 避免冲突。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 策略导致全集群网络中断 | 紧急删除问题策略，恢复连通性 |
| P1 | 单服务被意外隔离 | 检查策略匹配，添加允许规则 |
| P2 | 策略生效延迟 | 检查 CNI 控制器状态 |

## 面试要点

1. **Q：NetworkPolicy 的默认行为是什么？如何实现零信任网络？**
   A：默认情况下，K8s 没有 NetworkPolicy 时所有 Pod 间流量允许（全通）。实现零信任：① 每个 Namespace 创建 default-deny（ingress+egress）；② 显式允许必要的服务间通信；③ 允许 DNS egress（UDP 53）；④ 配合 mTLS（Istio/Linkerd）实现传输层加密。

2. **Q：为什么 iptables 实现的 NetworkPolicy 在大规模场景性能差？**
   A：iptables 规则是线性链表：① 每条 NetworkPolicy 生成多条 iptables 规则；② 数据包需逐条匹配（O(n) 复杂度）；③ 规则更新需要全量刷新（短暂中断）；④ conntrack 表压力大。Cilium 的 eBPF 使用哈希表（O(1)），Antrea 使用 OVS conjunctive match（O(1)），性能更优。

3. **Q：如何测试 NetworkPolicy 是否生效？**
   A：① 使用 `kubectl exec` 从测试 Pod 发起连接；② 使用 `kubectl describe networkpolicy` 查看匹配 Pod；③ Cilium：`cilium policy trace --src-identity X --dst-identity Y`；④ Calico：`calicoctl node status` + iptables-save；⑤ 使用网络策略测试工具（如 `network-policy-api` 测试套件）。

## Related

- [[17-系统基础/06-知识字典/networking/cni.md|CNI]]
- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/security/rbac.md|RBAC]]
- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]
- [[17-系统基础/06-知识字典/security/security-context.md|Security Context]]


<!-- risk-assessed -->
