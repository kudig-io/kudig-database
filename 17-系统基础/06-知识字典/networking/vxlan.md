---
title: VXLAN
description: VXLAN（Virtual Extensible LAN）是一种网络虚拟化技术，通过在 UDP 报文中封装二层以太网帧，实现跨三层的虚拟网络。Kubernete...
summary: VXLAN（Virtual Extensible LAN）是一种网络虚拟化技术，通过在 UDP 报文中封装二层以太网帧，实现跨三层的虚拟网络。Kubernete...
category: dictionary
tags:
- k8s
- glossary
- networking
- vxlan
tier: peripheral
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- VXLAN 是什么
- VXLAN 详解
trigger_keywords:
- VXLAN
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# VXLAN

> **英文名**: VXLAN

## 概述

VXLAN（Virtual Extensible LAN）是一种网络虚拟化技术，通过在 UDP 报文中封装二层以太网帧，实现跨三层的虚拟网络。Kubernetes CNI 插件广泛使用 VXLAN 实现 Pod 间的跨节点通信。

## 核心概念/原理

### 核心概念

- **VTEP（VXLAN Tunnel Endpoint）**：封装和解封装的端点（通常是节点上的虚拟网络设备）。
- **VNI（VXLAN Network Identifier）**：24 位的网络标识，支持最多 1600 万个虚拟网络。
- **封装方式**：原始 Pod 数据包 → 以太网帧 → UDP（端口 4789）→ 外层 IP 包。

### 在 Kubernetes 中的应用

- **Flannel**：VXLAN 后端是最常用的模式。
- **Calico**：支持 VXLAN 封装模式。
- **Cilium**：支持 VXLAN 隧道模式。

## 关键机制或特性

- VXLAN 增加了约 50 字节的头部开销。
- 相比 IPIP 封装，VXLAN 支持跨三层的虚拟网络。
- 硬件卸载（checksum offload）可以提升 VXLAN 性能。

## 使用场景与最佳实践

- 大规模集群中 VXLAN 的封装开销需要考虑。
- 高性能场景考虑使用 eBPF（Cilium）替代 VXLAN。
- 确保 UDP 4789 端口在节点间可达。

## 架构深度解析

### 封装格式

```
┌─────────────────────────────────────────────────────────┐
│                    VXLAN 报文结构（42 字节开销）            │
│  ┌──────────┬──────────┬──────────┬─────────────────┐   │
│  │ 外层 MAC  │ 外层 IP   │ UDP      │ VXLAN Header   │   │
│  │ (14B)    │ (20B)    │ (8B)     │ (8B)           │   │
│  │          │          │          │ Flags(1) VNI(3) │   │
│  │          │          │ dst:4789 │ (24 位网络标识)  │   │
│  └──────────┴──────────┴──────────┴─────────────────┘   │
│  ┌─────────────────────────────────────────────────┐    │
│  │              原始以太网帧（内层）                   │    │
│  └─────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘

          VTEP（隧道端点）模型
  ┌──────────────┐                    ┌──────────────┐
  │ 节点 A VTEP   │─── UDP 4789 ────▶│ 节点 B VTEP   │
  │ 10.0.0.1     │    （underlay）    │ 10.0.0.2     │
  │ Pod: 10.244.1.2                 │ Pod: 10.244.2.3│
  └──────────────┘                    └──────────────┘
```

### 内核实现路径（linux 内核 net/ipv4/udp_tunnel* 与 VXLAN 模块）

| 模块 | 路径 | 职责 |
|------|------|------|
| VXLAN 驱动 | `drivers/net/vxlan/vxlan_core.c` | VXLAN 设备与封解封装 |
| UDP 隧道 | `net/ipv4/udp_tunnel_core.c` | UDP 隧道通用处理 |
| FDB 学习 | `drivers/net/vxlan/vxlan_core.c` | 泛洪学习（类似交换机） |
| 路由查找 | `net/ipv4/route.c` | 外层 IP 路由选择 |

### 转发流程

1. Pod A 发包，节点 A 的 VTEP 收到（VXLAN 设备）
2. 查 FDB（MAC → VTEP IP 映射）：命中直接封装；未命中泛洪到同 VNI 组播组
3. 封装：内层帧 + VXLAN Header（VNI）+ UDP（4789）+ 外层 IP/MAC
4. 外层 IP 路由到节点 B，VTEP 解封装还原内层帧
5. 内层帧按目标 Pod MAC 交付（本机路由/网桥）

## 生产案例

### 案例 1：MTU 不一致导致跨节点大包丢失

| 时间 | 事件 |
|------|------|
| 14:00 | 应用反馈跨节点传输大文件失败，小包正常 |
| 14:10 | ping 带大包（1472+）失败，小包成功 |
| 14:20 | 定位为物理网卡 MTU 1500，VXLAN 封装后 Pod MTU 需 1450 |
| 14:30 | 统一配置 Pod MTU=1450，大包恢复 |

**根因**：VXLAN 42 字节开销未被计入 Pod MTU；节点间路径 MTU 未探测（黑盒网络禁止 ICMP 时更隐蔽）。

**修复命令**：
```bash
# 检查各节点 MTU 🟢 只读
ip link show | grep -E "vxlan|eth0" | awk '{print $2, $5}'
# 探测路径 MTU（节点间）🟢 只读
ping -M do -s 1450 10.0.0.2
# 调整 CNI 的 Pod MTU 配置（如 Calico）🟡 中风险
# calico-config.yaml: "veth_mtu": "1450"
```

### 案例 2：UDP 4789 被防火墙阻断导致跨节点不通

**现象**：新增安全策略后，部分节点间 Pod 通信中断，其他节点正常。

**诊断**：VXLAN 依赖 UDP 4789（或自定义端口）在节点间可达；防火墙规则放行不完整导致特定网段被阻。

**修复**：检查节点防火墙放行 UDP 4789（自定义端口则同步调整）；配置 VXLAN 端口为集群专用端口并纳入安全组模板。

## 对比评测

| 维度 | VXLAN | Geneve | GENEVE 扩展 | eBPF 直连（无隧道） |
|------|-------|--------|------------|-------------------|
| 开销 | 50B（+8B UDP） | 64B（可变选项） | 灵活选项 | 无 |
| 组播依赖 | 依赖（泛洪模式） | 依赖 | 依赖 | 无 |
| 兼容性 | 最广泛 | 主流（OVN/Cilium） | 高 | 需 eBPF 内核 |
| 适用场景 | 传统网络 | 云原生网络 | 云原生 | 高性能同网络 |

**选型建议**：兼容性优先选 VXLAN；云原生隧道选 Geneve；同网段高性能场景考虑 eBPF 免封装（Cilium Direct Routing）。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| 大包不通 | `ping -M do -s 1450` | MTU 不一致或路径黑洞 |
| 跨节点不通 | `nc -u -z <peer> 4789` | UDP 4789 被防火墙阻断 |
| 泛洪风暴 | `bridge fdb show dev vxlan0` | FDB 未学习或组播配置错误 |
| 性能差 | `ethtool -k eth0` 查 offload | 硬件卸载未开启 |

## 生产部署清单

- [ ] 集群统一 MTU 规划（物理 1500 → Pod 1450 等）
- [ ] 防火墙放行 UDP 4789（或自定义端口）并纳入安全组模板
- [ ] 大集群评估组播依赖，必要时切 Geneve/eBPF
- [ ] 开启网卡 checksum offload 提升性能
- [ ] 定期巡检 FDB 表规模与泛洪比例

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 隧道大面积中断（MTU/防火墙） | 立即统一 MTU 并检查端口连通性 |
| P1 | 从 VXLAN 迁移 Geneve/eBPF | 制定灰度迁移计划，验证双栈并存 |
| P2 | 集群规模增长泛洪加剧 | 评估分布式路由（免泛洪）方案 |

## 面试要点

> 以下 Q&A 覆盖 VXLAN 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：VXLAN 的报文开销是多少，如何影响 MTU 规划？**
   A：VXLAN 封装增加 50 字节（外层 MAC 14B + IP 20B + UDP 8B + VXLAN Header 8B，有时含外层 VLAN 4B 为 54B）。物理网卡 MTU 1500 时 Pod MTU 应设为 1450（或 1442 考虑 VLAN），否则大包分片或被丢弃。

2. **Q：VTEP 如何学习对端 MAC 地址（FDB）？**
   A：初始未知 MAC 通过泛洪（组播或单播复制）学习：目标 MAC 未知时，VTEP 将帧泛洪到同一 VNI 的所有 VTEP，对端 VTEP 回复后记录"MAC → 源 VTEP IP"映射并缓存，后续流量直接单播封装，泛洪仅在冷启动/新终端时发生。

3. **Q：VXLAN 相比直接路由（如 Cilium 原生路由）的优缺点？**
   A：VXLAN 优点：三层网络透明（跨 L3 域部署）、无需修改物理网络；缺点：50B 封装开销、依赖组播（或配置 BGP EVPN 控制面）、CPU 开销。直接路由性能最优但要求节点二层互通或路由可达，适合同机房/同云 VPC 场景。

## 参考链接

- [VXLAN - Official Documentation](https://datatracker.ietf.org/doc/html/rfc7348)

## Related

- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]
- [[17-系统基础/06-知识字典/networking/clusterip.md|Clusterip]]
- [[17-系统基础/06-知识字典/networking/nodeport.md|Nodeport]]
- [[17-系统基础/06-知识字典/networking/loadbalancer.md|Loadbalancer]]


<!-- risk-assessed -->
