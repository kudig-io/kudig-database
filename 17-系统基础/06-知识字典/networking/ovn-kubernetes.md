---
title: OVN-Kubernetes 网络方案
description: OVN-Kubernetes 是基于 OVN（Open Virtual Network）的 Kubernetes CNI 实现，由 Red
  Hat 主导开发，是...
summary: OVN-Kubernetes 是基于 OVN（Open Virtual Network）的 Kubernetes CNI 实现，由 Red Hat
  主导开发，是...
category: dictionary
tags:
- k8s
- glossary
- networking
- cni
- ovn
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OVN-Kubernetes 网络方案 是什么
- OVN-Kubernetes 详解
trigger_keywords:
- OVN-Kubernetes 网络方案
- OVN-Kubernetes
- dictionary
prerequisites:
- kubernetes
---



# OVN-Kubernetes 网络方案（OVN-Kubernetes）

## 概述

OVN-Kubernetes 是基于 OVN（Open Virtual Network）的 Kubernetes CNI 实现，由 Red Hat 主导开发，是 OpenShift 的默认网络方案，提供完整的 L2/L3 网络、NetworkPolicy 和硬件加速能力。

## 核心概念/原理

- **OVN 数据面**：基于 OpenFlow 的虚拟网络，支持硬件卸载
- **完整 NetworkPolicy**：支持 Ingress/Egress 和 FQDN 策略
- **OpenShift 默认**：Red Hat OpenShift 的标准 CNI
- **硬件加速**：支持 SmartNIC/DPU 卸载

## 关键机制或特性

- OVN Northbound/Southbound 数据库架构
- OVS（Open vSwitch）作为节点数据面
- 支持 Hybrid Overlay（Windows + Linux 节点混合）
- EgressFirewall / EgressQoS / EgressService CRD
- AdminNetworkPolicy（K8s 增强网络策略）
- IPAM 管理和多子网支持

## 使用场景与最佳实践

- OpenShift / OCP 集群的标准网络方案
- 需要硬件加速的企业网络
- Windows + Linux 混合节点集群
- 需要 AdminNetworkPolicy 的多租户环境
- 大规模集群的高性能网络

## 参考链接

- https://github.com/ovn-kubernetes/ovn-kubernetes
- https://docs.openshift.com/container-platform/latest/networking/understanding-networking.html

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────┐
│              OVN-Kubernetes CNI                     │
├─────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │ ovnkube-    │  │ ovnkube-node │  │ OVN       │  │
│  │ master      │  │ (DaemonSet)  │  │ Northbound│  │
│  │ (Deployment)│  │              │  │ DB        │  │
│  └──────┬──────┘  └──────┬───────┘  └─────┬─────┘  │
│         │                │                 │        │
│  ┌──────▼────────────────▼─────────────────▼────┐  │
│  │         OVS Bridge (br-int)                  │  │
│  │  + OVN Logical Flows (distributed routing)   │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（ovn-kubernetes/ovn-kubernetes）

| 模块 | 路径 | 职责 |
|------|------|------|
| Master | `go-controller/pkg/ovn/` | 集群级网络编排、LB 管理 |
| Node | `go-controller/pkg/node/` | 节点网络配置、网关管理 |
| OVN Client | `go-controller/pkg/nb/` | Northbound DB 操作 |
| CNI Plugin | `go-controller/pkg/cni/` | Pod 网络配置 |
| Gateway | `go-controller/pkg/gateway/` | 分布式网关逻辑 |

### 分布式路由机制

1. Pod 创建 → CNI 调用 → ovnkube-node 配置 veth + OVS 端口
2. ovnkube-master 在 OVN NB DB 创建 Logical Switch Port
3. OVN 控制器生成 Logical Flows 下发到各节点 OVS
4. 跨节点流量通过 Geneve 隧道（分布式路由，无集中瓶颈）
5. Service 通过 OVN LB 实现（替代 kube-proxy）

## 生产案例

### 案例 1：OVN Northbound DB 连接中断

| 时间 | 事件 |
|------|------|
| 06:00 | 新 Pod 无法获取 IP 地址 |
| 06:05 | 检查 ovnkube-master 日志：NB DB 连接超时 |
| 06:10 | 确认：OVN NB DB Pod OOMKilled（内存限制过低） |
| 06:20 | 修复：增加 NB DB 内存限制到 4Gi，启用 DB 集群模式 |

**修复命令**：
```bash
# 检查 OVN DB 状态 🟢 只读
kubectl exec -n ovn-kubernetes deploy/ovnkube-master -- ovn-nbctl show
# 查看 DB 连接状态 🟢 只读
kubectl logs -n ovn-kubernetes deploy/ovnkube-master -c nb-ovsdb --tail=50
# 重启 DB Pod 🟡 中风险
kubectl rollout restart deploy/ovnkube-db -n ovn-kubernetes
```

### 案例 2：Geneve 隧道 MTU 问题

**现象**：跨节点 Pod 通信正常，但访问外部服务时大包丢失。

**诊断**：Geneve 封装开销（50 字节）未计入 MTU，物理网卡 1500 - 50 = 1450。

**修复**：设置 CNI MTU 为 1400，启用 PMTU Discovery：
```bash
# 检查当前 MTU 🟢 只读
kubectl exec -n ovn-kubernetes ds/ovnkube-node -- ovs-vsctl get interface br-int mtu_request
# 调整 MTU 🟡 中风险
kubectl patch cm ovn-kubernetes-config -n ovn-kubernetes -p '{"data":{"mtu":"1400"}}'
```

## 对比评测

| 维度 | OVN-Kubernetes | Calico | Cilium |
|------|---------------|--------|--------|
| 数据面 | OVS（OpenFlow） | iptables/eBPF/BGP | eBPF |
| 网络策略 | 支持（ACL） | 支持 | 支持（最强） |
| 分布式网关 | 原生 | 需配置 | 原生 |
| 与 OpenStack 集成 | 同源（OVN） | 无 | 无 |
| 内核依赖 | 需 ovs 模块 | 低 | 需 5.x+ |

**选型建议**：OpenStack + K8s 混合环境选 OVN-Kubernetes（共享 OVN 架构）；纯 K8s 且性能敏感选 Cilium；通用场景 Calico。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| Pod 不通 | `ovn-nbctl show`；`ovs-vsctl show` | OVN 数据库异常、逻辑交换机端口缺失 |
| 网关故障 | `kubectl get pods -n ovn-kubernetes` | ovnkube-node 异常、网关节点漂移 |
| 性能下降 | `ovs-ofctl dump-flows` 检查流表 | 流表膨胀、隧道开销 |
| 策略不生效 | `ovn-nbctl list ACL` | ACL 优先级冲突、逻辑端口未绑定 |

## 生产部署清单

- [ ] OVN 数据库（NB/SB）高可用与备份已配置
- [ ] ovnkube-node DaemonSet 资源与健康检查就绪
- [ ] 网关节点打标签并规划（冗余 ≥ 2）
- [ ] MTU 调整（Geneve 隧道 50 字节开销）
- [ ] 监控接入（OVN metrics：`ovnkube_*`）

## 常见误区与设计要点

- **误区 1**：把 OVN 当纯 CNI 部署——它的 NB/SB 数据库是核心，必须 HA。
- **误区 2**：网关节点混跑业务负载——分布式网关承担跨子网转发，应隔离。
- **设计要点**：网络策略优先用 ACL 而非本地规则；日志关注 ovnkube-controller 的 reconcile 错误；升级前先备份 NB/SB 数据库。

## 性能参考

- 吞吐：Geneve 隧道模式约 70-85% 物理带宽，同节点直连接近线速。
- 延迟：隧道一跳增加 ~0.3ms；流表命中 O(1)（OVS 内核 datapath）。
- 规模：社区验证 500 节点/5 万 Pod 级别；更大规模需调整数据库参数。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | OVN DB 集群全部不可用 | 从备份恢复 DB，重建 OVN 网络 |
| P1 | 单节点 OVS 流表异常 | 重启该节点 ovnkube-node，重建流表 |
| P2 | Service LB 同步延迟 | 检查 ovnkube-master 负载，扩容 |

## 面试要点

1. **Q：OVN-Kubernetes 与 Calico/Cilium 的架构差异？**
   A：OVN-Kubernetes 基于 OVN（Open Virtual Network），使用集中式控制器（OVN NB/SB DB）+ 分布式数据面（OVS）；Calico 基于 BGP 路由或 IPIP 隧道，无集中控制器；Cilium 基于 eBPF，无 OVS 依赖。OVN 优势在于成熟的虚拟网络生态和硬件卸载支持；Cilium 优势在于性能和可观测性。

2. **Q：OVN 的分布式网关如何工作？**
   A：OVN 支持两种网关模式：① 集中式（所有出口流量经过指定网关节点）；② 分布式（每个节点本地处理出口流量）。分布式网关通过 OVN Logical Router 实现：每个节点维护本地路由表，SNAT 在源节点完成，无集中瓶颈。实现路径：`go-controller/pkg/gateway/`。

3. **Q：OpenShift 为什么选择 OVN-Kubernetes 作为默认 CNI？**
   A：① OVN 源自 oVirt/OpenStack 虚拟网络，企业级成熟度高；② 支持硬件卸载（OVS-DPDK、SR-IOV）；③ 原生支持 Windows 节点（混合 OS 集群）；④ 与 Red Hat 生态系统深度集成；⑤ 支持 AdminNetworkPolicy 等多租户特性。

## Related

- [[17-系统基础/06-知识字典/networking/antrea.md|Antrea]]
- [[17-系统基础/06-知识字典/networking/cilium.md|Cilium]]
- [[17-系统基础/06-知识字典/networking/cni.md|CNI]]
