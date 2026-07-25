---
title: Terway 测试验证
description: '## 概述'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- cilium
- networkpolicy
- crd
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 测试验证 是什么
- 如何 Terway 测试验证
trigger_keywords:
- Terway
- 测试验证
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Terway 测试验证

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

Terway 是阿里云 ACK（Aliyun Container Service for Kubernetes）的默认 CNI（Container Network Interface）插件。它通过 ENI（Elastic Network Interface）将 Pod 直接接入 VPC 网络，提供高性能的容器网络。本页涵盖 Terway 网络方案的测试验证方法——从基础连通性测试、NetworkPolicy 验证到性能基准测试，确保 Terway 网络在生产环境中稳定运行。

Terway 支持两种主要网络模式：**共享 ENI 模式**（ENI 多 IP，多个 Pod 共享一个 ENI 的辅助 IP）和**独占 ENI 模式**（每个 Pod 绑定独立 ENI，性能最优但 IP 消耗大）。测试验证需要覆盖两种模式下的连通性、安全策略和性能表现。

## 测试场景

- **基础连通性**：同节点 Pod、跨节点 Pod、Pod 到 Service 的网络连通性
- **NetworkPolicy 验证**：ingress/egress 策略生效性测试
- **性能基准**：吞吐量（bandwidth）、延迟（latency）、PPS（Packet Per Second）
- **ENI 资源**：ENI 配额、辅助 IP 耗尽场景
- **故障注入**：节点故障下的 Pod 网络恢复

## Architecture

Terway 架构由 **terway-daemon**（运行在每个节点的 CNI agent，管理 ENI 分配和 IPAM）、**terway-controller**（集群级控制器，管理 ENI 生命周期）、**CNI binary**（kubelet 调用的二进制，创建 Pod 网络命名空间）组成。测试验证需要覆盖这三层的协同工作。Pod 网络通过 Veth Pair 连接到节点的网络栈，再通过 ENI 路由到 VPC 网络。

## K8s 集成

Terway 实现 Kubernetes CNI 规范。kubelet 在创建 Pod 时调用 Terway CNI binary，分配 VPC IP 并配置网络命名空间。NetworkPolicy 由 Terway 的 eBPF 或 iptables 实现。测试命令使用标准的 `kubectl exec`、`kubectl get pod -o wide` 和 `ping`/`iperf3` 工具。

## 生产部署要点

- **测试集群**：在生产环境前使用专用测试集群验证 Terway 网络配置
- **多模式测试**：共享 ENI 和独占 ENI 两种模式都需要覆盖测试
- **安全组验证**：确保 Pod 安全组规则正确，允许必要流量
- **IP 容量测试**：验证 ENI 辅助 IP 配额是否满足 Pod 密度需求

## 生产场景

1. **新集群网络验证**：ACK 集群创建后，运行全套连通性和 NetworkPolicy 测试
2. **升级回归测试**：Terway 版本升级后验证网络功能无回归
3. **性能基准**：部署前评估不同 ENI 模式的吞吐和延迟表现
4. **故障演练**：模拟 ENI 故障、节点宕机，验证 Pod 网络恢复时间

## 操作命令

```bash
# 🟢 连通性测试：同节点 Pod
kubectl exec -it pod-a -- ping -c 3 <pod-b-ip>

# 🟢 连通性测试：跨节点 Pod
kubectl exec -it pod-a -- ping -c 3 <pod-on-other-node-ip>

# 🟢 NetworkPolicy 验证（创建 deny-all 策略后验证）
kubectl exec -it pod-a -- curl -m 3 http://pod-b:8080  # 应该失败

# 🟢 性能测试：iperf3 带宽基准
kubectl exec -it iperf-server -- iperf3 -s &
kubectl exec -it iperf-client -- iperf3 -c <iperf-server-ip> -t 30 -P 4

# 🟢 延迟测试
kubectl exec -it pod-a -- ping -c 100 -i 0.1 <pod-b-ip> | tail -5

# 🟢 检查 Terway 状态
kubectl get pod -n kube-system -l app=terway -o wide
kubectl logs -n kube-system terway-daemon-xxxxx
```

## 对比

| 测试维度 | Terway | Cilium | Calico | Flannel |
|----------|--------|--------|--------|---------|
| 连通性测试 | 标准工具 | 标准工具 | 标准工具 | 标准工具 |
| NetworkPolicy | ✅ iptables/eBPF | ✅ eBPF | ✅ iptables/eBPF | ⚠️ |
| 性能测试 | iperf3/netperf | iperf3/netperf | iperf3/netperf | iperf3 |
| 诊断工具 | terway-cli | cilium-cli | calicoctl | - |

## 参考链接

- [[cilium]]
- [[22-概念/03-网络/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]]

## Related

- [[aeraki-mesh]] — Aeraki Mesh
- [[submariner]] — Submariner
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[41-terway-architecture-deep-dive]]
- [[43-terway-crd-operations]]
- [[44-terway-operations-manual]]
- [[40-terway-product-overview]]
- [[42-terway-usage-guide]]
- [[46-terway-performance-tuning]]
- 45-terway-testing-validation

<!-- risk-assessed -->
