---
title: Terway 性能调优
description: '## 概述'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- prometheus
- grafana
- cilium
- networkpolicy
- crd
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 性能调优 是什么
- 如何 Terway 性能调优
trigger_keywords:
- Terway
- 性能调优
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Terway 性能调优

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

Terway 是阿里云 ACK 的默认 CNI 插件，通过 ENI 将 Pod 直接接入 VPC 网络。本页涵盖 Terway 网络性能调优的方法——从 ENI 模式选择、MTU 配置、NetworkPolicy 性能优化到 conntrack 表和内核参数调优，最大化 Terway 网络的吞吐量和降低延迟。

Terway 提供两种网络模式，性能特征差异显著：**独占 ENI 模式**（Exclusive ENI）每个 Pod 独占一个 ENI，绕过节点内核网络栈，性能最优（可达 ENI 线速）；**共享 ENI 模式**（Shared ENI）多个 Pod 共享 ENI 的辅助 IP，通过 Veth Pair + 节点路由转发，性能略低但 IP 利用率高。

## 调优领域

- **ENI 模式选择**：独占 ENI（高性能）vs 共享 ENI（高密度）
- **MTU 配置**：VPC 网络 MTU 通常为 1500，使用 Jumbo Frame（9000）可提升吞吐
- **NetworkPolicy 引擎**：eBPF 模式（高性能）vs iptables 模式（兼容性好）
- **conntrack 表**：高并发场景调大 `nf_conntrack_max`
- **内核网络参数**：TCP 缓冲区、网卡队列、RPS/RFS

## Architecture

Terway 性能优化的核心在于减少 Pod 到 VPC 网络的路径中的开销。独占 ENI 模式下，Pod 流量直接通过 ENI 硬件转发，无需经过节点内核网络栈，延迟最低。共享 ENI 模式下，流量经过 Veth Pair → 节点路由 → ENI 转发，节点内核成为潜在瓶颈。NetworkPolicy 的 eBPF 实现绕过 iptables，在高规则数量下性能优势明显。

## K8s 集成

Terway 性能调优通过修改 `terway-config` ConfigMap 和节点内核参数实现。NetworkPolicy 引擎模式在 Terway 配置中切换。Prometheus + Grafana 监控网络指标（吞吐、延迟、丢包率）。`terway-cli` 工具用于诊断和验证调优效果。

## 生产部署要点

- **ENI 模式**：延迟敏感型工作负载使用独占 ENI，高密度场景使用共享 ENI
- **MTU 一致性**：确保 Pod、节点和 VPC 网络 MTU 一致，避免分片
- **eBPF NetworkPolicy**：规则数 >100 时使用 eBPF 模式避免 iptables 性能下降
- **conntrack 调优**：高并发短连接场景增大 conntrack 表
- **监控覆盖**：监控 PPS、带宽、延迟和重传率指标

## 生产场景

1. **高吞吐数据传输**：大数据/AI 场景使用独占 ENI + Jumbo Frame 获得最大吞吐
2. **低延迟交易系统**：金融交易系统使用独占 ENI 最小化网络延迟
3. **大规模 NetworkPolicy**：数百条 NetworkPolicy 规则的场景使用 eBPF 模式
4. **高并发 API 服务**：调优 conntrack 和 TCP 缓冲区支持高并发短连接

## 操作命令

```bash
# 🟢 检查当前 Terway 配置
kubectl get cm eni-config -n kube-system -o yaml
kubectl get cm terway-config -n kube-system -o yaml

# 🟢 检查节点 ENI 状态
terway-cli show eni    # 在节点上运行

# 🟡 切换 NetworkPolicy 引擎为 eBPF（修改 ConfigMap）
kubectl edit cm terway-config -n kube-system
# 设置: network_policy_provider: "ebpf"

# 🟢 conntrack 表使用情况
cat /proc/sys/net/netfilter/nf_conntrack_count
cat /proc/sys/net/netfilter/nf_conntrack_max

# 🟡 调大 conntrack 表（节点级）
sysctl -w net.netfilter.nf_conntrack_max=1048576

# 🟢 检查 MTU
kubectl exec -it pod -- cat /sys/class/net/eth0/mtu
# 建议设置为 1500（标准）或 9000（Jumbo，需 VPC 支持）

# 🟢 性能基准测试
kubectl exec -it iperf-client -- iperf3 -c <server-ip> -t 60 -P 8 -l 1M
```

## 对比

| 调优维度 | Terway 独占 ENI | Terway 共享 ENI | Cilium eBPF | Calico |
|----------|----------------|----------------|------------|--------|
| 延迟 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ |
| 吞吐 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| IP 效率 | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| NetworkPolicy | ⚠️ iptables/eBPF | ⚠️ iptables/eBPF | ✅ eBPF | ⚠️ iptables/eBPF |

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[cilium]]
- [[实体/cni-plugins.md|cni-plugins]]
- [[实体/networkpolicy.md|networkpolicy]]

## Related

- [[45-terway-testing-validation]] — Terway 测试验证
- [[telepresence]] — Telepresence
- [[实体/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[41-terway-architecture-deep-dive]]
- [[43-terway-crd-operations]]
- [[44-terway-operations-manual]]
- [[40-terway-product-overview]]
- [[42-terway-usage-guide]]
- [[47-terway-troubleshooting-fta]]
- 46-terway-performance-tuning

<!-- risk-assessed -->
