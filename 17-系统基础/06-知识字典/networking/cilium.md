---
title: Cilium
description: Cilium 是基于 eBPF 技术的 Kubernetes CNI 插件和网络安全解决方案。它替代了传统的 iptables 规则，提供高性能的网络数据平面、...
summary: Cilium 是基于 eBPF 技术的 Kubernetes CNI 插件和网络安全解决方案。它替代了传统的 iptables 规则，提供高性能的网络数据平面、...
category: dictionary
tags:
- k8s
- glossary
- cilium
- cni
- ebpf
- networkpolicy
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cilium 是什么
- Cilium 详解
trigger_keywords:
- Cilium
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Cilium

> **英文名**: Cilium

## 概述

Cilium 是基于 eBPF 技术的 Kubernetes CNI 插件和网络安全解决方案。它替代了传统的 iptables 规则，提供高性能的网络数据平面、细粒度的安全策略和深度可观测性，已成为云原生网络的事实标准之一。

## 核心概念/原理

### 核心架构

- **eBPF 数据平面**：在内核态处理网络包，替代 kube-proxy 的 iptables。
- **Cilium Agent**：每节点 DaemonSet，管理策略和配置。
- **Hubble**：内置的网络可观测性组件，提供流量可视化。
- **Cilium Operator**：集群级管理组件（IPAM、身份管理）。

### 与 iptables 对比

| 特性 | iptables | Cilium (eBPF) |
|------|----------|---------------|
| 规则处理 | O(n) 线性扫描 | O(1) 哈希查找 |
| 策略粒度 | L3/L4 | L3-L7（含 HTTP/gRPC） |
| 性能 | 规则多时性能下降 | 恒定性能 |

## 关键机制或特性

- 完全替代 kube-proxy，使用 eBPF 实现 Service 负载均衡。
- 支持 FQDN Policy（基于域名的网络策略）。
- 支持 Cluster Mesh 实现多集群网络互通。
- Gateway API 原生支持。
- Tetragon 提供运行时安全检测和进程级可观测性。

## 使用场景与最佳实践

- 新集群优先选择 Cilium 作为 CNI。
- 启用 Cilium 的 kube-proxy 替代模式提升 Service 性能。
- 使用 Hubble 进行网络故障排查和流量分析。
- 配合 CiliumNetworkPolicy 实现 L7 层安全策略。
- 使用 Cilium CLI 进行安装和诊断。

## 参考链接

- [Cilium Official Documentation](https://docs.cilium.io/)

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────┐
│              Cilium Agent (DaemonSet)               │
├─────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │ Policy      │  │ Identity     │  │ Hubble    │  │
│  │ Compiler    │  │ Manager      │  │ (Flow Log)│  │
│  └──────┬──────┘  └──────┬───────┘  └───────────┘  │
│         │                │                         │
│  ┌──────▼────────────────▼─────────────────────┐  │
│  │         eBPF Programs (Kernel)              │  │
│  │  ┌────────┐ ┌────────┐ ┌────────────────┐  │  │
│  │  │TC/XDP  │ │Socket  │ │ Policy Map     │  │  │
│  │  │hooks   │ │LB      │ │ (BPF hash)     │  │  │
│  │  └────────┘ └────────┘ └────────────────┘  │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（cilium/cilium）

| 模块 | 路径 | 职责 |
|------|------|------|
| Agent 主循环 | `daemon/cmd/` | 初始化、控制器编排 |
| eBPF 程序 | `bpf/` | C 语言 eBPF 源码（TC/XDP） |
| Policy | `pkg/policy/` | 策略编译为 BPF map |
| Identity | `pkg/identity/` | 安全身份分配与管理 |
| Hubble | `pkg/hubble/` | 流量可观测性 |
| K8s Watcher | `pkg/k8s/` | CRD/资源监听 |

### eBPF 数据面流程

1. Pod 发包 → veth → TC eBPF 程序拦截
2. 查找 Policy Map（源 Identity → 目标 Identity）
3. 执行 L3/L4/L7 策略检查
4. 若允许 → 查找 Endpoint Map → 转发到目标
5. 若跨节点 → VXLAN/Geneve 封装 → 物理网卡
6. Hubble 记录 Flow Log（可选）

## 生产案例

### 案例 1：eBPF Map 容量耗尽导致策略失效

| 时间 | 事件 |
|------|------|
| 16:00 | 新 Pod 无法访问任何服务 |
| 16:05 | cilium status 显示 policy map full |
| 16:15 | 根因：CiliumNetworkPolicy 规则组合爆炸，超过 map 容量 |
| 16:30 | 修复：增加 `--bpf-policy-map-max`，合并策略规则 |

**修复命令**：
```bash
# 检查 eBPF map 使用情况 🟢 只读
cilium bpf policy get --all -n kube-system
# 查看 Cilium 状态 🟢 只读
cilium status --verbose
# 调整 map 容量（需重启） 🔴 高风险
kubectl patch cm cilium-config -n kube-system -p '{"data":{"bpf-policy-map-max":"65536"}}'
kubectl rollout restart ds/cilium -n kube-system
```

### 案例 2：Hubble Flow Log 占用过多磁盘

**现象**：节点磁盘使用率告警，/var/run/cilium 目录占用 > 10GB。

**诊断**：Hubble 默认保留所有 Flow Log，未配置轮转。

**修复**：启用 Hubble Relay + 外部存储（ClickHouse），限制本地 Flow Log 保留时间。

## 对比评测

| 维度 | Cilium | Calico | Antrea |
|------|--------|--------|--------|
| 数据面 | eBPF（TC/XDP） | iptables/BGP | OVS |
| 网络策略 | 最强（L7/FQDN/DNS） | 强（L3/L4） | 强（L3/L4） |
| 可观测性 | Hubble（全流量） | 弱 | Traceflow |
| 内核要求 | 5.x+（严格） | 低 | 需 ovs 模块 |
| 服务网格 | 内置（无 Sidecar） | 无 | 无 |

**选型建议**：性能与可观测性优先、内核满足条件选 Cilium；内核受限或团队熟悉 iptables 选 Calico；需要流追踪选 Antrea。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| 节点 NotReady | `cilium status`；`kubectl -n kube-system logs ds/cilium` | eBPF 加载失败、内核版本过低 |
| Pod 不通 | `cilium connectivity test` | 策略误配、隧道/直连模式不一致 |
| 策略不生效 | `cilium policy get`；Hubble flow | 端点身份未更新、策略未编译 |
| 性能下降 | `cilium metrics list` | BPF map 满、CPU 亲和配置 |

## 生产部署清单

- [ ] 内核版本验证（≥ 5.10 生产建议）与 eBPF 特性检查（`cilium install --check`）
- [ ] 数据面模式（隧道/直连/DSR）按云环境选择并压测
- [ ] 网络策略默认拒绝 + Hubble 流量审计已启用
- [ ] 升级走 `cilium upgrade` 并验证 rollback 预案
- [ ] 监控接入（cilium-agent metrics + Hubble UI）

## 常见误区与设计要点

- **误区 1**：直接开 DSR 模式不验证——需要云环境支持（L2 可达或特殊路由）。
- **误区 2**：忽略 `bpf.masquerade` 与 conntrack 配置——NAT 行为差异导致源 IP 异常。
- **设计要点**：多集群用 ClusterMesh（服务发现 + 全局策略）；用 Hubble 做故障定位第一手段；升级前跑 `cilium connectivity test` 全量回归。

## 性能参考

- 吞吐：eBPF 直连可达线速（XDP 模式 PPS 更高），隧道模式 80-90% 物理带宽。
- 延迟：本地转发 +0.05ms，DSR 模式回程最优（源 IP 保留）。
- 规模：生产验证 5000+ 节点（社区基准），BPF map 需按规模调参。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | eBPF 程序加载失败 / 数据面中断 | 回滚 Cilium 版本，临时切换 CNI |
| P1 | 策略编译错误导致流量拒绝 | 检查 CNP 语法，临时禁用策略 |
| P2 | Hubble 数据丢失 | 检查 Relay 状态，调整 Flow Log 配置 |

## 面试要点

1. **Q：Cilium 的 Identity 机制如何工作？**
   A：Cilium 为每个 Pod 分配 Security Identity（基于标签的哈希）：① Pod 创建时根据 Namespace + Labels 计算 Identity；② Identity 存储在 KVStore（etcd）或 CRD；③ 策略编译为 Identity-based BPF map；④ 数据面通过 Identity 而非 IP 执行策略，支持 Pod 漂移后策略不变。

2. **Q：Cilium 如何实现 L7 策略（如 HTTP method 限制）？**
   A：Cilium 使用 Envoy Sidecar（或嵌入式 Envoy）实现 L7 策略：① TC eBPF 将匹配 L7 策略的流量重定向到 Envoy；② Envoy 解析 HTTP/gRPC 协议；③ 执行 L7 规则（method/path/header）；④ 允许则转发，拒绝则返回 403。实现路径：`pkg/proxy/envoy/`。

3. **Q：Cilium 与 Calico 在 NetworkPolicy 实现上有何差异？**
   A：Cilium 使用 eBPF map 存储策略，查找复杂度 O(1)，无 iptables 规则数量限制；Calico 使用 iptables/IPVS，规则数随策略增长线性增加。Cilium 支持 L7 策略和 Identity-based 策略；Calico 主要支持 L3/L4。性能上 Cilium 在大规模策略场景优势明显。

## Related

- [[17-系统基础/06-知识字典/networking/cni.md|CNI]]
- [[17-系统基础/06-知识字典/networking/networkpolicy.md|NetworkPolicy]]
- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/fundamentals/kube-proxy.md|Kube-proxy]]
- [[17-系统基础/06-知识字典/observability/prometheus.md|Prometheus]]


<!-- risk-assessed -->
