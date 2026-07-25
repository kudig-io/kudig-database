---
title: Kubernetes 网络配置最佳实践
description: '# Kubernetes 网络配置最佳实践'
summary: '本指南提供生产环境 Kubernetes 网络配置的最佳实践，涵盖从 CNI 选型到网络策略的全方位内容 ^[inferred]。'
category: skills
tags:
- k8s
- networking
- cni
- network-policy
- ingress
- cilium
- flannel
- calico
- coredns
- helm
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 网络配置最佳实践 是什么
- 如何 Kubernetes 网络配置最佳实践
trigger_keywords:
- Kubernetes
- 网络配置最佳实践
prerequisites:
- kubectl-basics
- helm-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 网络配置最佳实践

## 概述

本指南提供生产环境 Kubernetes 网络配置的最佳实践，涵盖从 CNI 选型到网络策略的全方位内容 ^[inferred]。

## CNI 插件选型

| 特性 | Calico | [[Cilium|Cilium]] | Flannel | Weave |
|------|--------|--------|---------|-------|
| 网络模式 | BGP/VXLAN | eBPF | VXLAN | VXLAN |
| 网络策略 | 完整 | 增强（eBPF） | 无 | 基础 |
| 性能 | 高 | 极高 | 中 | 中 |
| 可观测性 | 中 | 高 | 低 | 中 |
| 适用场景 | 通用 | 高性能 | 简单 | 小规模 |

**选型建议** ^[inferred]：
- **通用场景**：Calico — 稳定可靠，社区活跃
- **高性能场景**：Cilium — eBPF 加持，性能卓越，支持 kube-proxy 替换
- **简单场景**：Flannel — 配置简单，易于维护

## 网络架构设计

生产环境网络应分层设计：CDN/WAF -> 负载均衡器 -> [[Ingress|Ingress]] Controller -> 服务网格（可选）-> CNI 插件 -> Pod 网络 ^[inferred]。

### 关键配置

- **Pod CIDR** 与 **[[Service|Service]] CIDR** 不可重叠 ^[inferred]
- VXLAN 封装需考虑 MTU 开销（50 字节）^[inferred]
- 内核版本要求：Cilium 需要 >= 5.4 ^[inferred]

## 网络策略

### 默认拒绝所有流量

每个生产命名空间应配置默认拒绝 Ingress 和 Egress 的网络策略 ^[inferred]。

### 允许 DNS 查询

配置默认拒绝策略时，必须允许 Pod 向 kube-system 命名空间的 DNS 查询（UDP/TCP 53），否则 Service 发现会失败 ^[inferred]。

### 命名空间隔离

限制跨命名空间通信，仅允许明确授权的命名空间间流量 ^[ambiguous]。

## 实施步骤

1. **网络规划**：确定 Pod CIDR（如 10.244.0.0/16）、Service CIDR（如 10.96.0.0/12），验证不重叠
2. **安装 CNI 插件**：通过 [[Helm|Helm]] 安装 Calico 或 Cilium
3. **配置网络策略**：默认拒绝 + DNS 允许 + 应用级策略
4. **配置 Ingress**：安装 Nginx Ingress Controller，配置 TLS

## 常见陷阱

### MTU 配置不当

VXLAN 封装会增加 50 字节开销，超过物理网络 MTU 限制会导致数据包分片和性能下降 ^[inferred]。

### 网络策略冲突

多个网络策略同时生效可能导致预期外的流量被阻断。应定期检查所有网络策略并测试连通性 ^[inferred]。

### DNS 配置错误

CoreDNS 配置不当会导致 Service 发现失败。应检查 CoreDNS Pod 状态和 ConfigMap 配置 ^[inferred]。

## 验证方法

- 检查 CNI 插件状态：`kubectl get [[Pods|pods]] -n kube-system | grep -E "calico|cilium"`
- 检查网络策略：`kubectl get networkpolicy --all-namespaces`
- 测试 Pod 网络连通性和 DNS 解析 ^[inferred]

## 相关资源

- [[22-概念/10-最佳实践/k8s-production-best-practices.md|Kubernetes 生产环境最佳实践]]
- [[22-概念/03-网络/service-networking.md|Service Networking]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|CNI Plugins]]
- [[23-实体/02-K8s核心组件/networkpolicy.md|NetworkPolicy]]
- [[26-技能/05-网络/networkpolicy/最佳实践/k8s-network-security-guide.md|Kubernetes 网络安全最佳实践]]

## 生产案例

### 案例 1: Pod CIDR 与 VPC 路由冲突导致跨节点不通

| 时间 | 事件 |
|------|------|
| 09:00 | 新集群部署后跨节点 Pod 通信失败 |
| 09:05 | `ip route` 显示 Pod CIDR 路由指向错误网关 |
| 09:10 | VPC 路由表中已有相同网段路由 |
| 09:15 | 🟡 修改 Pod CIDR 为不冲突网段，重新部署 CNI |

**根因**: Pod CIDR 10.244.0.0/16 与 VPC 内已有路由冲突。

### 案例 2: Service CIDR 过小导致 Service 创建失败

**现象**: 新建 Service 报错 "no available cluster IP"。

**诊断**: `kubectl get svc -A | wc -l` 接近 Service CIDR 上限(/24=254)

**修复**: 🔴 扩展 Service CIDR(需重建集群)或清理无用 Service

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 集群网络完全不通 | 检查 CNI + 路由表 |
| P1 | 部分网段不通 | 检查路由和安全组 |
| P2 | IP 规划优化 | 重新规划 CIDR |

## 面试要点

1. **Q: Kubernetes 的三大网络平面？**
   A: ① Pod Network(Pod CIDR): Pod 间通信 ② Service Network(Service CIDR): 虚拟 IP ③ Node Network: 节点间通信。三者不能重叠。

2. **Q: CNI 插件的选型考虑因素？**
   A: ① 性能需求(Overlay vs Underlay) ② NetworkPolicy 支持 ③ 云环境兼容性 ④ 运维复杂度 ⑤ 社区活跃度。生产: 阿里云 Terway、AWS VPC CNI、通用 Calico/Cilium。

3. **Q: 大规模集群的网络规划建议？**
   A: ① Pod CIDR 足够大(/16 支持 254 节点×254 Pod) ② Service CIDR 预留充足(/20=4094) ③ 使用 IPVS 替代 iptables ④ 规划 MTU 避免封包问题 ⑤ 预留扩展空间。

## Related

- [[coredns]] — CoreDNS
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/10-最佳实践/k8s-production-best-practices.md|k8s-production-best-practices]] — Kubernetes 生产环境最佳实践
- [[22-概念/03-网络/service-networking.md|service-networking]] — Service Networking


<!-- risk-assessed -->
