---
title: Kube-OVN CNI
description: Kube-OVN 是阿里云灵骏开源的 CNCF Sandbox 项目，基于 OVN/OVS 的 Kubernetes CNI 实现，提供企业级的网络功能（静态
  ...
summary: Kube-OVN 是阿里云灵骏开源的 CNCF Sandbox 项目，基于 OVN/OVS 的 Kubernetes CNI 实现，提供企业级的网络功能（静态
  ...
category: dictionary
tags:
- k8s
- glossary
- networking
- cni
- ovn
tier: supporting
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kube-OVN CNI 是什么
- Kube-OVN 详解
trigger_keywords:
- Kube-OVN CNI
- Kube-OVN
- dictionary
prerequisites:
- kubernetes
---



# Kube-OVN CNI（Kube-OVN）

## 概述

Kube-OVN 是阿里云灵骏开源的 CNCF Sandbox 项目，基于 OVN/OVS 的 Kubernetes CNI 实现，提供企业级的网络功能（静态 IP/VPC/多子网/安全组等）。

## 核心概念/原理

- **OVN/OVS 数据面**：高性能的虚拟网络
- **企业网络**：VPC/子网/安全组/静态 IP
- **CNCF Sandbox**：阿里云主导
- **多租户网络**：完整的网络隔离能力

## 关键机制或特性

- Subnet CRD（VPC/子网管理）
- 固定 IP（Pod Annotation）
- 安全组（Security Group）
- QoS 带宽限制
- 网络 ACL
- 多网卡支持（Multus）
- DPDK 加速

## 使用场景与最佳实践

- 企业级 K8s 网络方案
- 需要 VPC/固定 IP 的场景
- 多租户网络隔离
- 安全组和 ACL 的精细控制
- 电信/金融行业的网络合规

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────────┐
│                     Kubernetes 集群                       │
│  ┌──────────────────────────────────────────────────┐   │
│  │  kube-ovn-controller（控制面）                     │   │
│  │  - VPC / Subnet / SecurityGroup 管理              │   │
│  │  - 逻辑交换机/路由器（OVN NB）编排                 │   │
│  │  - 固定 IP / QoS / ACL 分配                       │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  ovn-central（OVN 数据库，3 副本）                 │   │
│  │  - OVN NB/SB 数据库（HA）                         │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  每节点：kube-ovn-cni + ovs（Open vSwitch 数据面） │   │
│  │  - ovs 网桥（br-int）承载逻辑流表                 │   │
│  │  - 隧道（Geneve/VXLAN）跨节点通信                 │   │
│  └──────────────────────────────────────────────────┘   │
│  （可选：kube-ovn-monitor / kube-ovn-speaker BGP）     │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubeovn/kube-ovn）

| 模块 | 路径 | 职责 |
|------|------|------|
| Controller | `pkg/controller/` | VPC/Subnet/固定 IP/QoS/ACL 协调 |
| CNI 插件 | `pkg/cni/` | Pod 网络接入（分配 IP、配置 ovs 端口） |
| Daemon | `pkg/daemon/` | 节点侧流表注入与隧道管理 |
| OVN 适配 | `pkg/ovs/` | OVS/OVN 北向接口封装 |
| 网络策略 | `pkg/controller/securitygroup` | 安全组与 NetworkPolicy 翻译 |

### 数据面转发流程

1. Pod 创建时 kube-ovn-cni 从 Subnet 分配 IP（支持固定 IP 池）
2. 节点 ovs 将 Pod veth 接入 br-int，逻辑端口绑定到逻辑交换机
3. 同节点流量：ovs 流表直接转发；跨节点流量：Geneve/VXLAN 隧道封装
4. 出集群流量：经过逻辑路由器 → 网关节点 NAT → 物理网络（或 BGP 宣告）
5. 安全组/ACL 以 OVN 流表形式下发，实现 L2-L4 隔离

## 生产案例

### 案例 1：ovn-central 单点故障导致全网中断

| 时间 | 事件 |
|------|------|
| 09:00 | ovn-central 主节点磁盘故障，集群网络大面积异常 |
| 09:05 | 新 Pod 无法分配 IP，存量 Pod 部分断流 |
| 09:15 | 排查发现 ovn-central 未部署为 HA（单副本），NB/SB 数据库丢失 |
| 09:40 | 恢复备份数据库并重建 ovn-central HA 部署，网络恢复 |

**根因**：ovn-central 是控制面核心（NB/SB 数据库），单副本部署时故障即全损；缺少备份与告警。

**修复命令**：
```bash
# 检查 ovn-central 状态 🟢 只读
kubectl -n kube-system get pods -l app=ovn-central
# 查看 NB/SB 数据库健康 🟢 只读
kubectl -n kube-system exec ovn-central-0 -- ovn-nbctl --db=tcp:127.0.0.1:6641 show
# 部署为 3 副本 StatefulSet 并启用备份 🟡 中风险
# ovn-central 支持 raft 模式：--nb-raft --sb-raft
kubectl -n kube-system scale sts ovn-central --replicas=3
```

### 案例 2：固定 IP 池耗尽导致 StatefulSet 扩不动

**现象**：StatefulSet 扩容时新 Pod 一直 ContainerCreating，kube-ovn 日志报 IP 分配失败。

**诊断**：Subnet 的 `allow` 固定 IP 池数量小于副本数；或旧 Pod 残留导致 IP 未回收。

**修复**：扩大 `allow` 池或改 `release` 策略（Pod 删除即释放）；清理残留命名空间与 orphan 资源；为关键工作负载配置独立 Subnet 隔离 IP 池。

## 对比评测

| 维度 | Kube-OVN | Calico | Cilium |
|------|----------|--------|--------|
| 网络模型 | VPC/Subnet（云网络语义） | 平面网络 + 策略 | 平面网络 + eBPF |
| 固定 IP | ✅ 原生支持 | ❌ | 部分 |
| 多租户隔离 | ✅ VPC 级 | 策略级 | 策略级 |
| 性能 | OVS 用户态（可 DPDK） | eBPF/iptables | eBPF 内核态 |
| 适用场景 | 企业/金融/电信合规 | 通用 | 高性能云原生 |

**选型建议**：需要 VPC/固定 IP/安全组等云网络语义选 Kube-OVN；通用场景选 Calico；追求性能与可观测性选 Cilium。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| Pod 无网络 | `kubectl -n kube-system logs <cni-pod>` | IP 池耗尽或 ovs 端口异常 |
| 跨节点不通 | `kubectl exec pod -- ping <peer>` | Geneve 隧道或 MTU 问题 |
| 控制面异常 | `ovn-nbctl show` | ovn-central 故障或数据不一致 |
| 固定 IP 冲突 | 检查 Subnet 分配记录 | 残留对象或重复声明 |

## 生产部署清单

- [ ] ovn-central 必须 HA 部署（raft 3 副本）并配置备份
- [ ] 关键业务使用独立 Subnet 与固定 IP 池（预留余量）
- [ ] MTU 规划：隧道场景调整物理网卡与 Pod MTU
- [ ] 升级前备份 NB/SB 数据库并演练恢复
- [ ] 监控：隧道数、流表数、IP 池水位告警

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | ovn-central 故障或网络批量中断 | 立即恢复数据库并检查 HA 状态 |
| P1 | Kube-OVN 大版本升级 | 先备份 NB/SB 数据库，灰度节点验证 |
| P2 | 数据面性能瓶颈（OVS 用户态） | 评估 DPDK 加速或迁移 Cilium |

## 面试要点

> 以下 Q&A 覆盖 Kube-OVN 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Kube-OVN 相比 Calico/Cilium 的核心差异化能力是什么？**
   A：Kube-OVN 把云网络语义（VPC、Subnet、安全组、固定 IP、QoS）完整带进 K8s：每个命名空间/租户可拥有独立 VPC 与子网，Pod 可申请固定 IP（生命周期内不变），安全组在 OVN 流表层实现 L2-L4 隔离，这对金融/电信等需要网络合规与精细隔离的场景是刚需。

2. **Q：Kube-OVN 数据面为什么基于 Open vSwitch 而不是 eBPF？**
   A：OVN 提供成熟的逻辑网络抽象（逻辑交换机/路由器/ACL）与分布式网关能力，流表语义完整且可在用户态灵活扩展（QoS、镜像、DPDK 加速）；eBPF 性能更优但网络语义编程门槛高。Kube-OVN 选 OVS 换取"云网络能力 + 可维护性"，性能敏感场景可用 DPDK 抵消用户态开销。

3. **Q：ovn-central 故障的影响范围与恢复要点？**
   A：ovn-central 承载 NB/SB 数据库，故障时控制面不可用：新 Pod 无法分配 IP、策略无法下发，存量数据面（已下发流表）通常可继续转发但无法变更。恢复要点：HA（raft 3 副本）部署、定期备份、恢复后校验 NB/SB 一致性，并验证流表重新下发。

## 参考链接

- https://kubeovn.github.io/
- https://github.com/kubeovn/kube-ovn

## Related

- [[17-系统基础/06-知识字典/networking/ovn-kubernetes.md|OVN-Kubernetes]]
- [[17-系统基础/06-知识字典/networking/cilium.md|Cilium]]
- [[17-系统基础/06-知识字典/networking/antrea.md|Antrea]]
