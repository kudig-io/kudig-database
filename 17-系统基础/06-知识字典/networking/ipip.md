---
title: IPIP
description: IPIP（IP-in-IP）是一种网络隧道协议，将一个 IP 数据包封装在另一个 IP 数据包中传输。在 Kubernetes 网络中，IPIP
  常用于跨节点 ...
summary: IPIP（IP-in-IP）是一种网络隧道协议，将一个 IP 数据包封装在另一个 IP 数据包中传输。在 Kubernetes 网络中，IPIP
  常用于跨节点 ...
category: dictionary
tags:
- k8s
- glossary
- ipip
- tunnel
- networking
- cni
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- IPIP 是什么
- IPIP 详解
trigger_keywords:
- IPIP
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# IPIP

> **英文名**: IPIP

## 概述

IPIP（IP-in-IP）是一种网络隧道协议，将一个 IP 数据包封装在另一个 IP 数据包中传输。在 Kubernetes 网络中，IPIP 常用于跨节点 Pod 通信，是 Calico 等 CNI 插件支持的封装模式之一。

## 核心概念/原理

### IPIP 封装原理

```
原始包: [IP Header | Payload (Pod→Pod)]
封装后: [Outer IP Header (Node→Node) | IP Header | Payload]
```

### 与其他隧道协议对比

| 协议 | 封装层 | 开销 | MTU 影响 | 典型使用 |
|------|--------|------|----------|----------|
| IPIP | IP-in-IP | 20 bytes | -20 | Calico IPIP 模式 |
| VXLAN | Ethernet-in-UDP | 50+ bytes | -50 | Calico/Cilium VXLAN |
| Geneve | 类似 VXLAN | 可变 | 可变 | OVN-Kubernetes |

## 关键机制或特性

- IPIP 模式的 MTU 比 VXLAN 小 20 字节（外层 IP 头开销）。
- IPIP 不支持跨子网（不同 L2 域）通信，仅限同子网节点。
- Calico 支持 IPIP Always（所有跨节点流量）和 CrossSubnet（仅跨子网）两种模式。
- IPIP 流量在节点上是 `tunl0` 接口。

## 使用场景与最佳实践

- 同子网集群优先使用 IPIP 模式，开销最小。
- 跨子网或需要 L2 隔离时使用 VXLAN 模式。
- 排查 IPIP 问题时检查 `tunl0` 接口状态和路由表。
- 注意 IPIP 与 IPsec 的兼容性。

## 架构深度解析

### IPIP 封装原理

```
┌──────────────────────────────────────────────────────────────┐
│  Pod A（10.244.1.5）──→ Pod B（10.244.2.7）                    │
│       │                                                       │
│       ▼                                                       │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 源节点 eth0（172.16.1.10）                               │  │
│  │  ├─ 原始包：src=10.244.1.5 dst=10.244.2.7               │  │
│  │  ├─ IPIP 封装：外层 IP src=172.16.1.10 dst=172.16.2.10  │  │
│  │  │   Protocol=4（IP-in-IP）                             │  │
│  │  └─ 从 tunl0 接口发出                                    │  │
│  └─────────────────────────────────────────────────────────┘  │
│       │                                                       │
│       ▼                                                       │
│  物理网络（外层 IP 路由转发，无需理解内层 Pod IP）             │
│       │                                                       │
│       ▼                                                       │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 目标节点 eth0（172.16.2.10）→ tunl0 解封装              │  │
│  │  ├─ 校验外层源 IP ∈ 已知节点 IP 列表（rp_filter）       │  │
│  │  └─ 还原内层包，路由至 Pod B 的 veth                    │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（projectcalico/felix）

| 模块 | 路径 | 职责 |
|------|------|------|
| felix 数据面 | `dataplane/linux/` | 管理 tunl0 接口、路由表（proto 17 内核路由）与 iptables |
| confd/BIRD | Calico node 镜像 `bird/` | BGP 会话：向对端通告 Pod CIDR，使外层路由可达 |
| IPIP 配置 | `ipip.go` | 读取 IPPool `ipipMode`（Always/CrossSubnet/Never），创建/销毁 tunl0 |
| calico-node | `node/` | 守护进程：同步 Felix 配置、维护 BIRD 状态 |

### 流程步骤

1. 数据包从 Pod A 发出，经 veth 到根命名空间，命中 Calico 注入的路由（Pod CIDR 下一跳为对端节点）。
2. 路由设备选择 tunl0（IPIP 隧道），内核执行 IP-in-IP 封装：外层头 src/dst 为两端节点 IP。
3. 封装包沿物理网络普通三层路由到达目标节点（BGP 通告的节点路由）。
4. 目标节点 tunl0 解封装，校验源地址后还原内层包。
5. 内层包按本地路由转交 Pod B veth。整个过程无需 overlay MAC 学习，开销低于 VXLAN。

## 生产案例

### 案例 1：IPIP 模式跨网段丢包

| 时间 | 事件 |
|------|------|
| 周一 09:00 | 集群扩容新增一个子网 172.16.3.0/24 的节点组 |
| 09:30 | 新节点上 Pod 与旧节点 Pod 互 ping 丢包 50% |
| 09:45 | `ip route` 显示新子网无路由；`birdcl show protocol` BGP 会话 Established 但路由缺失 |
| 10:00 | 检查发现新子网未加入 Calico BGP Peer 的 IP 池通告（`calicoctl get bgppeer`） |
| 10:15 | 添加 BGP Peer 配置并重置 BIRD 后恢复 |

**根因**：IPIP 外层包依赖 BGP 通告的节点路由，新子网未与 BGP 邻居建立对等关系导致路由黑洞。
**修复命令**：
```bash
# 查看 BGP 会话状态 🟢 只读
kubectl exec -n kube-system calico-node-xxxx -- birdcl show protocols
# 查看 IPPool 配置 🟢 只读
calicoctl get ippool -o yaml
# 修正 IPPool 的 cidr 与 ipipMode 后应用 🟡 中风险
calicoctl apply -f ippool.yaml
```

### 案例 2：tunl0 接口异常导致同子网 Pod 不通

**现象**：某节点上所有 Pod 出向流量失败，`curl` 超时，但该节点上 Pod 互相访问正常。
**诊断**：`ip link show tunl0` 显示接口 DOWN；`ip addr show tunl0` 无 `tunl0` 地址（应为节点 IP）；Felix 日志报 `failed to ensure tunl0 up`。
**修复**：重启 calico-node Pod（重新初始化 tunl0）；若为内核模块问题（`modprobe ipip`），在节点启动脚本中预加载并设置 `net.ipv4.ip_forward=1`。

## 对比评测

| 维度 | IPIP | VXLAN | 直接路由（无封装） |
|------|------|-------|-------------------|
| 封装开销 | 20 字节（IP 头） | 50 字节（UDP+VXLAN 头） | 0 |
| 跨子网 | 支持（需 BGP 路由） | 支持（UDP 组播/单播） | 需底层路由支持 |
| MTU 要求 | 1480（1500-20） | 1450（1500-50） | 1500 |
| 性能 | 高 | 中 | 最高 |
| 依赖 | 内核 ipip 模块 + BGP | UDP 4789 端口 | 物理网络路由 |

**选型建议**：同子网/可控路由网络用 IPIP（Calico 默认 CrossSubnet 模式）；公有云 VPC 内跨可用区可用 VXLAN 规避底层路由限制。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| 跨节点 Pod 不通 | `ip route \| grep tunl0`；`ping -c3 <nodeIP>` | BGP 路由缺失、IPIP 模块未加载 |
| 同节点正常跨节点失败 | `ip link show tunl0` | tunl0 DOWN、Felix 异常 |
| 大包不通小包通 | `ping -s 1472 <podIP>` | MTU 不一致（需 1480） |
| 封装后丢包 | `tcpdump -i tunl0`；`birdcl show route` | 外层路由黑洞、rp_filter 误杀 |
| 新节点加入后不通 | `calicoctl get bgppeer`；`birdcl show protocols` | 未配置 BGP Peer/网格模式未自动发现 |

## 生产部署清单

- [ ] 内核已加载 ipip 模块且 `net.ipv4.ip_forward=1`
- [ ] BGP 对等关系（node-to-node mesh 或 RR）已建立并验证路由
- [ ] MTU 已按封装开销调整（节点网卡 1500 → Pod 1480）
- [ ] rp_filter 配置与 IPIP 兼容（`net.ipv4.conf.all.rp_filter=0` 或按需）
- [ ] 云环境安全组已放行 IPIP 协议（protocol 4）

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | BGP 路由黑洞导致大面积不通 | 修复 Peer 配置，检查 ippool 通告 |
| P1 | 性能敏感业务受封装开销影响 | 评估直接路由/云原生网卡方案 |
| P1 | 跨云互联需加密 | 评估 WireGuard/IPsec 隧道替代 IPIP |
| P2 | 现有 IPIP 稳定运行 | 保持配置，随 Calico 升级验证 |

## 面试要点

> 以下 Q&A 覆盖 IPIP 封装面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：IPIP 与 VXLAN 封装的区别是什么？**
   A：IPIP 将整个内层 IP 包作为负载包进一个新的 IP 头（Protocol=4），开销仅 20 字节，无端口概念，性能更高但只能承载 IP 流量且依赖底层 IP 路由可达；VXLAN 将二层帧封装进 UDP（端口 4789），开销约 50 字节，可跨三层构建大二层网络，支持多租户 VNI 隔离。选型上：三层路由可控用 IPIP，需要 L2 广播域/多云互通用 VXLAN。

2. **Q：Calico 的 IPPool ipipMode 三种取值有何区别？**
   A：Always：所有跨节点流量都走 IPIP 隧道；CrossSubnet：仅当源目节点不在同一子网时才封装，同子网直接路由（性能最优，生产推荐）；Never：永不封装，要求底层网络对 Pod CIDR 可直接路由。CrossSubnet 本质是在"封装开销"与"底层路由不可控"之间取平衡。

3. **Q：IPIP 隧道下 MTU 如何计算？遇到大包不通怎么排查？**
   A：Pod 网卡 MTU = 物理网卡 MTU - IPIP 头（20 字节），即 1500-20=1480。大包不通时：① `ping -s 1472`（不含 ICMP 头 28 字节，1472+28=1500）测试是否分片问题；② 用 `tcpdump -i tunl0` 观察是否出现"需分片但 DF 置位"的 ICMP 错误；③ 检查隧道两端 MTU 是否一致，必要时调小 Pod MTU（如 1400）并同步所有节点。

## 参考链接

- [Calico IPIP Mode - Project Calico](https://docs.tigera.io/calico/latest/networking/configure-ip-addresses/ipip)

## Related

- [[17-系统基础/06-知识字典/networking/vxlan.md|VXLAN]]
- [[17-系统基础/06-知识字典/networking/cni.md|CNI]]
- [[17-系统基础/06-知识字典/networking/networkpolicy.md|NetworkPolicy]]
- [[17-系统基础/06-知识字典/networking/clusterip.md|ClusterIP]]
- [[17-系统基础/06-知识字典/networking/nodeport.md|NodePort]]


<!-- risk-assessed -->
