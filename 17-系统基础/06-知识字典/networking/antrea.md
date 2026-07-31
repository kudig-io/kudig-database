---
title: Antrea 网络方案
description: Antrea 是 VMware 开源的 Kubernetes 网络方案（CNI），基于 Open vSwitch（OVS）构建，提供 NetworkPolicy...
summary: Antrea 是 VMware 开源的 Kubernetes 网络方案（CNI），基于 Open vSwitch（OVS）构建，提供 NetworkPolicy...
category: dictionary
tags:
- k8s
- glossary
- networking
- cni
- ebpf
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Antrea 网络方案 是什么
- Antrea 详解
trigger_keywords:
- Antrea 网络方案
- Antrea
- dictionary
prerequisites:
- kubernetes
---



# Antrea 网络方案（Antrea）

## 概述

Antrea 是 VMware 开源的 Kubernetes 网络方案（CNI），基于 Open vSwitch（OVS）构建，提供 NetworkPolicy、流量可视化、多集群网络等企业级功能，是 Calico/Cilium 之外的另一主流 CNI 选择。

## 核心概念/原理

- **OVS 数据面**：基于 Open vSwitch 的高性能转发引擎
- **完整 NetworkPolicy**：支持 K8s NetworkPolicy + Antrea 扩展策略（FQDN 策略、NodeNetworkPolicy）
- **流量可视化**：内置 Flow Exporter 和 ClickHouse 集成
- **多集群支持**：Antrea Multi-cluster 实现跨集群网络互通

## 关键机制或特性

- OVS 流表驱动的转发规则管理
- 支持 WireGuard 加密隧道
- Egress / ExternalIP 管理
- Traceflow 端到端连通性诊断
- 与 Theia 可视化平台集成
- 支持 Antrea Proxy（kube-proxy 替代）

## 使用场景与最佳实践

- 企业级 K8s 网络方案选型
- 需要高级 NetworkPolicy（FQDN、Node 级别）
- 网络流量审计与可视化需求
- 多集群网络互联场景

## 参考链接

- https://antrea.io/
- https://github.com/antrea-io/antrea

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────┐
│                   Antrea Agent (DaemonSet)           │
├─────────────────────────────────────────────────────┤
│  ┌──────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ CNI Plugin│  │ NetworkPolicy│  │ Flow Exporter│  │
│  │ (antrea-  │  │ Controller   │  │ (IPFIX/      │  │
│  │  cni)     │  │ (ANP/ACNP)   │  │  ClickHouse) │  │
│  └─────┬────┘  └──────┬───────┘  └──────┬───────┘  │
│        │               │                  │          │
│  ┌─────▼───────────────▼──────────────────▼───────┐  │
│  │              OVS Bridge (br-int)                │  │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────────────┐  │  │
│  │  │classify │ │pipeline │ │ conntrack/CT    │  │  │
│  │  │ table   │ │ tables  │ │ zone mgmt       │  │  │
│  │  └─────────┘ └─────────┘ └─────────────────┘  │  │
│  └────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（antrea-io/antrea）

| 模块 | 路径 | 职责 |
|------|------|------|
| Agent 主循环 | `cmd/antrea-agent/agent.go` | 初始化 OVS、注册 CNI、启动控制器 |
| OVS 流表管理 | `pkg/agent/openflow/` | 流表编排、Pipeline 阶段定义 |
| NetworkPolicy | `pkg/agent/controller/networkpolicy/` | 策略翻译为 OVS 流表 |
| Flow Exporter | `pkg/agent/flowexporter/` | IPFIX 记录生成与导出 |
| Proxy | `pkg/agent/proxy/` | Service 负载均衡（替代 kube-proxy） |
| Multi-cluster | `pkg/agent/multicluster/` | 跨集群隧道管理 |

### 数据面转发流程

1. Pod 发包 → veth pair → OVS br-int
2. classify table 匹配源 MAC/IP → 标记 tunnel ID
3. pipeline tables 执行 NetworkPolicy 检查（conjunctive match）
4. 若跨节点 → VXLAN/Geneve 封装 → 物理网卡
5. 若本节点 → 直接转发到目标 Pod veth

## 生产案例

### 案例 1：OVS 流表溢出导致网络中断

| 时间 | 事件 |
|------|------|
| 02:15 | 告警：多节点 Pod 间通信超时 |
| 02:18 | 确认 3 个节点 OVS flow count 超过 50000 |
| 02:25 | 定位：NetworkPolicy 规则组合爆炸（200+ Namespace × FQDN 策略） |
| 02:35 | 临时修复：`ovs-ofctl del-flows br-int` 重置流表 |
| 03:00 | 根因修复：合并 FQDN 策略，启用 `antreaAgent.featureGates.AntreaProxy=true` |

**根因**：FQDN NetworkPolicy 为每个域名解析结果生成独立流表，DNS 轮询导致流表持续增长。

**修复命令**：
```bash
# 检查流表数量 🟢 只读
ovs-ofctl dump-flows br-int | wc -l
# 查看 Antrea Agent 日志 🟢 只读
kubectl logs -n kube-system -l app=antrea-agent --tail=100
# 重启 Agent 重建流表 🟡 中风险（短暂网络中断）
kubectl rollout restart daemonset/antrea-agent -n kube-system
```

### 案例 2：Antrea Proxy 与 kube-proxy 冲突

**现象**：启用 Antrea Proxy 后 Service ClusterIP 访问间歇性超时。

**诊断**：
```bash
# 确认 kube-proxy 是否仍在运行 🟢 只读
kubectl get pods -n kube-system -l k8s-app=kube-proxy
# 检查 iptables 规则冲突 🟢 只读
iptables-save | grep -c "KUBE-SERVICES"
```

**修复**：禁用 kube-proxy DaemonSet，确保 Antrea Proxy 独占 Service 转发：
```bash
# 🟡 中风险 - 需先确认 Antrea Proxy 已就绪
kubectl patch ds kube-proxy -n kube-system -p '{"spec":{"template":{"spec":{"nodeSelector":{"non-existing":"true"}}}}}'
```

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | OVS 流表溢出 / 数据面完全中断 | 立即重启 antrea-agent，回滚 OVS 配置 |
| P1 | NetworkPolicy 生效延迟 > 30s | 检查 Controller 队列积压，扩容 antrea-controller |
| P2 | Flow Exporter 数据丢失 < 5% | 调整 IPFIX 导出间隔，检查 ClickHouse 容量 |

## 面试要点

1. **Q：Antrea 与 Cilium 在数据面实现上有何本质区别？**
   A：Antrea 基于 OVS（用户态+内核态 datapath），通过 OpenFlow 流表编排转发逻辑，优势在于成熟的流表生态和硬件卸载（OVS-DPDK/OVS-offload）；Cilium 基于 eBPF 直接在内核网络栈挂载程序，绕过 iptables/netfilter，优势在于零拷贝、无上下文切换。Antrea 的 NetworkPolicy 通过 conjunctive match 实现 O(1) 策略匹配，Cilium 通过 BPF map 查找实现。

2. **Q：Antrea 的 Traceflow 如何实现端到端网络诊断？**
   A：Traceflow 在源 Pod 注入特殊标记包（携带 Traceflow header），沿 OVS pipeline 逐跳记录匹配结果（通过 OVS controller packet-in 上报），最终在目标 Pod 或丢弃点汇总完整路径。实现路径：`pkg/agent/controller/traceflow/` 监听 CRD → 构造 OpenFlow packet-out → 收集各节点 packet-in → 写入 Traceflow CR status。

3. **Q：生产环境从 Calico 迁移到 Antrea 需要注意什么？**
   A：关键步骤：① 确认 BGP 模式差异（Calico BGP vs Antrea 隧道/策略路由）；② NetworkPolicy 兼容性验证（Antrea 支持标准 K8s NP + 扩展 CRD）；③ 滚动迁移需双 CNI 并存期（使用 Multus）；④ 性能基线对比（OVS 流表规模 vs Calico iptables/IPVS）；⑤ 回滚预案：保留 Calico 配置 72h。

## Related

- [[17-系统基础/06-知识字典/networking/cilium.md|Cilium]]
- [[17-系统基础/06-知识字典/networking/cni.md|CNI]]
- [[17-系统基础/06-知识字典/networking/networkpolicy.md|NetworkPolicy]]
