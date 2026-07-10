---
title: 01 - Terway 产品概览 (Product Overview)
description: Terway 是阿里云容器服务 ACK (Alibaba Cloud Kubernetes) 自研的 Container Network
  Interface (CNI) 插件，深度集成阿里云 VPC/ENI 网络基础设施，将 Pod 直接接入 VPC 网络平面。作为 ACK 集群的默认 CNI 方案，Terway
  替代了早期基于 Flannel 的网络方案，提供更高性能和更丰富的云原生网络能力。
summary: Terway 是阿里云容器服务 ACK (Alibaba Cloud Kubernetes) 自研的 Container Network Interface
  (CNI) 插件，深度集成阿里云 VPC/ENI 网络基础设施，将 Pod 直接接入 VPC 网络平面。作为 ACK 集群的默认 CNI 方案，Terway 替代了早期基于
  Flannel 的网络方案，提供更高性能和更丰富的云原生网络能力。
category: terway
tags:
- k8s
- terway
- networking
- alicloud
- cilium
- flannel
- calico
- redis
- mysql
- kafka
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
estimated_read_time: 10min
intent_queries:
- Terway 产品概览 (Product Overview) 是什么
- 如何 Terway 产品概览 (Product Overview)
trigger_keywords:
- Terway
- 产品概览
- Product
- Overview
- terway
prerequisites:
- kubectl-basics
- networking-basics
- ebpf-basics
- cilium-basics
- cni-basics
- kafka-basics
- redis-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 01 - Terway 产品概览 (Product Overview)

> **适用版本**: 阿里云 ACK v1.25 - v1.32+ | **Terway 版本**: v1.5+ | **最后更新**: 2026-05

---

## 1. 产品定位

Terway 是阿里云容器服务 ACK (Alibaba Cloud [[Kubernetes|Kubernetes]]) 自研的 Container Network Interface (CNI) 插件，深度集成阿里云 VPC/ENI 网络基础设施，将 Pod 直接接入 VPC 网络平面。作为 ACK 集群的默认 CNI 方案，Terway 替代了早期基于 Flannel 的网络方案，提供更高性能和更丰富的云原生网络能力。

### 核心价值

| 维度 | 说明 |
|:---|:---|
| **Pod 直通 VPC** | Pod IP 即 VPC 内网 IP，无需 NAT 即可被 VPC 内其他资源（ECS、RDS、SLB）直接访问，简化网络拓扑 |
| **[[NetworkPolicy|NetworkPolicy]] 原生支持** | 完整实现 Kubernetes NetworkPolicy API，支持三层/四层网络策略，无需额外部署策略引擎 |
| **接近原生性能** | ENI/ENIIP 模式网络延迟和吞吐接近物理机水平，性能损耗控制在 5% 以内 |
| **SLB/ALB 深度集成** | 与阿里云 SLB (Server Load Balancer) 和 ALB (Application Load Balancer) 无缝联动，支持 LoadBalancer 类型 [[Service|Service]] 自动联动 |
| **安全组联动** | 支持节点级和 Pod 级安全组，实现精细化的网络访问控制 |
| **多模式灵活选择** | 提供 VPC/ENI/ENIIP/ENIIP-Trunking/IPVlan 五种网络模式，适配不同规模和性能需求 |

### 产品边界

- Terway 为阿里云专属 CNI 方案，仅支持阿里云 ECS 作为节点，不适用于混合云/多云场景的非阿里云节点
- 集群外访问 Pod IP 需要 VPC 网络互通（CEN/云企业网 或 VPN 网关）
- 部分高级特性（如 Trunk ENI、IPVlan）依赖特定 ECS 实例规格族和内核版本

---

## 2. 版本历史

| 版本 | 发布时间 | 关键特性 | 备注 |
|:---|:---|:---|:---|
| **v1.0** | 2019-Q2 | 初始发布，ENI 独占模式；VPC 路由模式；基础 NetworkPolicy 支持 | ACK 标准版默认 CNI |
| **v1.1** | 2020-Q1 | ENIIP 共享模式 (Beta)；Pod 独立安全组；Terway DaemonSet 架构定型 | 解决 ENI 独占模式容量受限问题 |
| **v1.2** | 2021-Q2 | ENIIP 模式 GA (正式稳定)；IPVlan 模式 (Beta)；IPAMD 本地缓存优化；ENI 预热池 | 推荐默认使用 ENIIP |
| **v1.3** | 2023-Q1 | Trunk ENI / ENIIP-Trunking 模式；固定 IP 地址 (Pod 保留 IP)；多 PodNetworking CRD；eBPF 加速初步支持 | 支持超大规模集群 (5000+ 节点) |
| **v1.4** | 2024-Q2 | GC 机制增强 (基于 CRD 的垃圾回收)；IPv6/IPv4 双栈支持；Policy 规则加速；多 vSwitch 支持 | 大幅减少 IP 泄漏问题 |
| **v1.5** | 2025-Q1 | 完整 eBPF 数据面 (可选)；Pod 级带宽限速；ENI 亲和性调度优化；OpenAPI 批量化操作；IPv6 双栈 GA | 当前推荐版本 |

> 版本策略: Terway 跟随 ACK 集群版本发布，ACK v1.26+ 默认安装 Terway v1.3+，ACK v1.30+ 默认安装 Terway v1.5+

---

## 3. 网络模式总览

Terway 提供五种网络模式，按性能和容量密度递增排列：

| 模式 | Pod IP 来源 | 网络接口 | 性能 (相对物理机) | 容量密度 | 内核要求 | 适用场景 |
|:---|:---|:---|:---:|:---:|:---|:---|
| **VPC** | VPC 路由表条目 | veth pair + Node 网络栈 | ~70% | 低 (受路由条目 48 条限制) | 无特殊要求 | 小规模集群、兼容性优先、已有 Flannel 迁移过渡 |
| **ENI** | 独占 ENI 主 IP | ENI 直通 | ~95% | 低 (受 ENI 配额限制) | 无特殊要求 | 核心数据库、网关、高性能隔离需求 |
| **ENIIP** | ENI 辅助 IP (Secondary IP) | veth pair + ENI | ~90% | 高 (推荐默认) | 无特殊要求 | 大规模通用场景、微服务、在线业务 |
| **ENIIP-Trunking** | Trunk ENI 辅助 IP | veth pair + Trunk ENI | ~88% | 最高 (单节点 200+ Pod) | 4.19+ | 超大规模集群、Serverless、高密度部署 |
| **IPVlan** | ENI 辅助 IP + IPVlan L2 | IPVlan 接口 + ENI | ~95% | 高 | 4.19+ 且开启 eBPF | 极致性能、低延迟、高性能计算 |

### 模式选择决策

```
是否需要 Pod 直通 VPC？
  └─ 否 → 考虑 Flannel / Calico / Cilium
  └─ 是
      ├─ 节点规模 < 50，Pod 密度 < 30/节点 → VPC 模式（最简单）
      ├─ 需要极致性能 + 低密度 → ENI 模式（独占）
      ├─ 通用大规模场景 → ENIIP 模式（推荐默认）★★★
      ├─ 超大规模 / Serverless → ENIIP-Trunking 模式
      └─ 极致性能 + 高密度 → IPVlan 模式（内核 4.19+）
```

> 详细架构分析参考: [02-architecture.md](./02-architecture.md)

---

## 4. 与其他 CNI 对比

| 特性 | Terway | Flannel | Calico | Cilium |
|:---|:---|:---|:---|:---|
| **Pod 直通 VPC** | 原生支持 | 不支持 (Overlay) | 不支持 (需 BGP/Overlay) | 不支持 (需 Overlay/eBPF) |
| **NetworkPolicy** | 原生支持 (L3/L4) | 不支持 | 原生支持 (L3/L4) | 原生支持 (L3-L7) |
| **性能损耗** | ~5% (ENI/ENIIP) | ~30% (VXLAN) | ~10% (IPIP/VXLAN) | ~10% (eBPF 优化后更低) |
| **eBPF 加速** | v1.5 部分支持 | 否 | 可选 (eBPF dataplane) | 原生核心特性 |
| **L7 策略** | 不支持 | 不支持 | 不支持 | 支持 (HTTP/gRPC/kafka 等) |
| **云厂商绑定** | 阿里云 (强绑定) | 无 | 无 | 无 |
| **多集群网络** | 通过 CEN 打通 | 需额外方案 | Submariner | Cluster Mesh |
| **IPAM** | 云端 ENI IP 分配 | 节点 CIDR 分配 | 节点 CIDR / IP Pool | 节点 CIDR / CRD IP Pool |
| **固定 IP** | v1.3+ 原生支持 | 不支持 | 支持 (IP Pool Reservation) | 支持 (IP Pool Reservation) |
| **带宽限速** | v1.5+ Pod 级限速 | 不支持 | 支持 (tc/iptables) | 支持 (EDT + eBPF) |
| **可观测性** | 依赖阿里云监控 | 基础 | 较丰富 (BGP metrics) | 最丰富 (Hubble) |
| **部署复杂度** | 低 (ACK 托管) | 最低 | 中等 | 较高 |

### 选型建议

| 场景 | 推荐 CNI |
|:---|:---|
| 纯阿里云 ACK 集群 | **Terway** |
| 多云/混合云 | Calico / Cilium |
| 需要 L7 网络策略 | Cilium |
| 极致可观测性需求 | Cilium + Hubble |
| 简单 Overlay 网络 | Flannel |

> 详细对比参考: [domain-03-networking-traffic/03-cni-plugins-comparison.md](../domain-03-networking-traffic/03-cni-plugins-comparison.md)

---

## 5. 核心依赖

Terway 深度依赖以下阿里云基础设施和服务：

| 依赖 | 服务 | 说明 | 必需性 |
|:---|:---|:---|:---:|
| **VPC (专有网络)** | 阿里云 VPC | Pod 网络的底层承载平面，vSwitch 为 Pod 分配 VPC 内网 IP | 必需 |
| **ENI (弹性网卡)** | 阿里云 ECS ENI | ENI/ENIIP/IPVlan 模式的网络接口载体，每个 Pod 通过 ENI 接入 VPC | ENI 模式必需 |
| **OpenAPI** | 阿里云 ECS API | ENI 创建/删除/绑定/解绑，辅助 IP 分配/释放等操作 | 必需 |
| **RAM 角色** | 阿里云 RAM | Terway 通过 ECS 实例角色 (Instance RAM Role) 获取访问云资源的临时凭证 | 必需 |
| **安全组** | 阿里云 ECS 安全组 | 节点级和 Pod 级网络出入站访问控制 | 必需 |
| **vSwitch** | 阿里云 VPC vSwitch | Pod IP 地址段分配，支持多 vSwitch 多可用区部署 | 必需 |
| **SLB/ALB** | 阿里云负载均衡 | LoadBalancer 类型 Service 自动关联，外部流量接入 | 可选 |
| **CEN** | 阿里云云企业网 | 多集群/多 VPC 网络互通 | 可选 |

### RAM 最小权限策略

Terway 需要以下最小权限（通过 ECS 实例角色授予）：

| API 操作 | 权限说明 |
|:---|:---|
| `CreateNetworkInterface` | 创建弹性网卡 |
| `DeleteNetworkInterface` | 删除弹性网卡 |
| `AttachNetworkInterface` | 绑定弹性网卡到 ECS 实例 |
| `DetachNetworkInterface` | 从 ECS 实例解绑弹性网卡 |
| `AssignPrivateIpAddresses` | 分配辅助私有 IP |
| `UnassignPrivateIpAddresses` | 释放辅助私有 IP |
| `DescribeNetworkInterfaces` | 查询弹性网卡信息 |
| `DescribeInstances` | 查询 ECS 实例信息 |

---

## 6. 产品限制

### 6.1 ECS 实例 ENI 配额

| 限制项 | 说明 | 影响 |
|:---|:---|:---|
| 单实例最大 ENI 数 | 由 ECS 实例规格决定 (通常 2-15 块) | 直接决定 ENI 模式的 Pod 密度上限 |
| 单 ENI 最大辅助 IP 数 | 由 ECS 实例规格决定 (通常 5-50 个) | 直接决定 ENIIP 模式的 Pod 密度上限 |
| Trunk ENI 支持 | 仅部分规格族支持 | ENIIP-Trunking 模式的先决条件 |

### 6.2 VPC 网络限制

| 限制项 | 默认配额 | 说明 |
|:---|:---:|:---|
| 单 VPC 路由表条目数 | 48 | VPC 模式下每节点消耗一条路由，限制集群规模 |
| 单 vSwitch IP 数 | 由 CIDR 决定 | /16 提供 65536 个 IP，/24 提供 256 个 IP |
| 单 VPC vSwitch 数 | 150 | 多可用区部署时的限制 |
| 单地域 ENI 总数 | 5000 (可申请提升) | 大规模集群可能触及上限 |

### 6.3 OpenAPI 速率限制

| API 类别 | 速率限制 | 说明 |
|:---|:---|:---|
| ENI 操作 (Create/Delete) | 单账号 ~100 QPS | Pod 快速扩容时可能成为瓶颈 |
| IP 操作 (Assign/Unassign) | 单账号 ~100 QPS | ENIIP 模式下高频调用 |
| 查询操作 (Describe*) | 单账号 ~500 QPS | Terway 定期同步 ENI 状态 |

### 6.4 其他限制

| 限制项 | 说明 |
|:---|:---|
| 内核版本 | IPVlan 模式要求 Linux 4.19+，Trunk ENI 要求 4.19+ |
| 集群规模 | 单集群最大 5000 节点 (ENIIP/Trunking 模式) |
| 跨 VPC 通信 | Pod IP 仅在本 VPC 内可直接路由，跨 VPC 需 CEN/VPN |
| Windows 节点 | 不支持 Terway，Windows 节点需使用 Flannel (Overlay) |

---

## 7. ECS 实例规格 ENI 限制速查

以下为常用 ECS 实例规格族的 ENI 和辅助 IP 配额，直接影响单节点可承载的 Pod 数量。

### 7.1 第七代实例 (推荐)

| 实例规格族 | 典型规格 | 最大 ENI | 单 ENI 最大辅助 IP | 总 IP 容量 (理论最大 Pod 数) |
|:---|:---|:---:|:---:|:---:|
| **ecs.g7** (通用) | ecs.g7.xlarge (4C16G) | 4 | 10 | 40 |
| **ecs.g7** | ecs.g7.2xlarge (8C32G) | 6 | 15 | 90 |
| **ecs.g7** | ecs.g7.4xlarge (16C64G) | 8 | 30 | 240 |
| **ecs.g7** | ecs.g7.8xlarge (32C128G) | 16 | 30 | 480 |
| **ecs.c7** (计算) | ecs.c7.xlarge (4C8G) | 4 | 10 | 40 |
| **ecs.c7** | ecs.c7.4xlarge (16C32G) | 8 | 30 | 240 |
| **ecs.r7** (内存) | ecs.r7.xlarge (4C32G) | 4 | 10 | 40 |
| **ecs.r7** | ecs.r7.4xlarge (16C128G) | 8 | 30 | 240 |

### 7.2 第六代实例

| 实例规格族 | 典型规格 | 最大 ENI | 单 ENI 最大辅助 IP | 总 IP 容量 |
|:---|:---|:---:|:---:|:---:|
| **ecs.g6** | ecs.g6.xlarge (4C16G) | 4 | 10 | 40 |
| **ecs.g6** | ecs.g6.4xlarge (16C64G) | 8 | 20 | 160 |
| **ecs.c6** | ecs.c6.xlarge (4C8G) | 4 | 10 | 40 |
| **ecs.r6** | ecs.r6.xlarge (4C32G) | 4 | 10 | 40 |

### 7.3 第六代增强实例

| 实例规格族 | 典型规格 | 最大 ENI | 单 ENI 最大辅助 IP | 总 IP 容量 |
|:---|:---|:---:|:---:|:---:|
| **ecs.g6e** | ecs.g6e.xlarge (4C16G) | 5 | 10 | 50 |
| **ecs.c6e** | ecs.c6e.xlarge (4C8G) | 5 | 10 | 50 |

### 7.4 第五代实例

| 实例规格族 | 典型规格 | 最大 ENI | 单 ENI 最大辅助 IP | 总 IP 容量 |
|:---|:---|:---:|:---:|:---:|
| **ecs.g5** | ecs.g5.xlarge (4C16G) | 4 | 10 | 40 |
| **ecs.c5** | ecs.c5.xlarge (4C8G) | 4 | 10 | 40 |
| **ecs.r5** | ecs.r5.xlarge (4C32G) | 4 | 10 | 40 |

### 7.5 经济型实例

| 实例规格族 | 典型规格 | 最大 ENI | 单 ENI 最大辅助 IP | 总 IP 容量 |
|:---|:---|:---:|:---:|:---:|
| **ecs.u1** | ecs.u1-c1m2.xlarge (4C8G) | 2 | 6 | 12 |
| **ecs.u1** | ecs.u1-c1m4.4xlarge (16C64G) | 4 | 10 | 40 |

### 容量计算公式

```
ENIIP 模式单节点最大 Pod 数 = (最大 ENI 数 - 1) * 单 ENI 最大辅助 IP 数
                              ↑
                    保留一块 ENI 供节点自身使用

例: ecs.g7.8xlarge → (16 - 1) * 30 = 450 个 Pod (理论值)
实际推荐预留 10-20% 余量用于 IPAM 缓冲
```

> 完整规格表参考阿里云官方文档: ECS 实例规格族

---

## 8. 适用场景

### 8.1 Web 服务 / API 网关

- **需求**: Pod 需要被 VPC 内其他服务直接访问，低延迟
- **推荐模式**: ENIIP
- **优势**: Pod IP 直通 VPC，无需 DNAT/SNAT，简化服务发现和负载均衡

### 8.2 核心数据库 (Redis / MySQL / MongoDB)

- **需求**: 网络性能要求极高，需要固定 IP 地址
- **推荐模式**: ENI (独占) 或 ENIIP + 固定 IP
- **优势**: 接近物理机网络性能，支持固定 IP 避免主从切换后 IP 变化

### 8.3 高密度微服务集群

- **需求**: 单节点运行大量 Pod (50-200+)，网络性能要求中等
- **推荐模式**: ENIIP-Trunking 或 IPVlan
- **优势**: 最高 Pod 密度，单节点可运行 200+ Pod，配合大规格 ECS 实例

### 8.4 网关 / 代理 / Ingress Controller

- **需求**: 高吞吐、低延迟、需要暴露服务到外部
- **推荐模式**: ENI (独占) 或 IPVlan
- **优势**: 最高网络性能，结合 SLB/ALB 实现外部流量接入

### 8.5 Serverless / 弹性伸缩

- **需求**: Pod 快速创建销毁，大规模弹性伸缩
- **推荐模式**: ENIIP-Trunking
- **优势**: Trunk ENI 预分配机制加速 Pod 创建，减少 OpenAPI 调用延迟

### 8.6 不适用场景

| 场景 | 原因 | 替代方案 |
|:---|:---|:---|
| 非阿里云环境 | Terway 深度绑定阿里云 VPC/ENI | Calico / Cilium / Flannel |
| Windows 节点 | Terway 不支持 Windows | Flannel (Overlay) |
| 需要 L7 网络策略 | Terway 仅支持 L3/L4 NetworkPolicy | Cilium |
| 多云统一网络 | Terway 无法跨云厂商 | Calico + Submariner / Cilium + Cluster Mesh |

---

## 9. 许可与开源

| 项目 | 说明 |
|:---|:---|
| **开源仓库** | [github.com/AliyunContainerService/terway](https://github.com/AliyunContainerService/terway) |
| **开源协议** | Apache License 2.0 |
| **语言** | Go |
| **维护方** | 阿里云容器服务团队 |
| **代码结构** | CNI Plugin Binary + DaemonSet (terway-eniip) + Controller Deployment |
| **依赖** | containernetworking/plugins (CNI 规范库), Alibaba Cloud SDK for Go |

Terway 以 Apache 2.0 协议开源，允许自由使用、修改和分发。ACK 托管版/专有版集群默认安装经过阿里云验证的 Terway 版本。

---

## 10. 交叉引用

### 本专题 (domain-03-networking-traffic/topic-terway/)

| 文档 | 说明 |
|:---|:---|
| [02-architecture.md](./02-architecture.md) | Terway 架构原理深度解析 (数据面/控制面/IPAM/CRD) |
| [03-usage.md](./[[domain-03-networking-traffic/Terway/03-usage.md|03-usage]].md) | 使用指南 (安装配置、模式切换、NetworkPolicy、固定 IP) |
| [04-operations.md](./04-operations.md) | 运维手册 (健康检查、GC 机制、升级策略、故障排查) |
| [05-testing.md](./05-testing.md) | 测试验证 (Pod 网络连通性、NetworkPolicy 测试、ENI 配额验证) |
| [06-performance.md](./06-performance.md) | 性能调优 (模式对比、内核调优、基准测试) |

### Domain 知识库 (domain-03-networking-traffic/)

| 文档 | 说明 |
|:---|:---|
| [05-terway-advanced-guide.md](../domain-03-networking-traffic/05-terway-advanced-guide.md) | Terway 高级指南 (模式对比、ENIIP 详解、容量规划) |
| [37-terway-resources-crud-operations.md](../domain-03-networking-traffic/37-terway-resources-crud-operations.md) | Terway CRD 资源 CRUD 操作指南 |
| [38-terway-gc-mechanism.md](../domain-03-networking-traffic/38-terway-gc-mechanism.md) | Terway GC 垃圾回收机制详解 |
| [02-cni-architecture-fundamentals.md](../domain-03-networking-traffic/02-cni-architecture-fundamentals.md) | CNI 架构基础与核心原理 |
| [03-cni-plugins-comparison.md](../domain-03-networking-traffic/03-cni-plugins-comparison.md) | CNI 插件对比与选型指南 |
| [04-flannel-complete-guide.md](../domain-03-networking-traffic/04-flannel-complete-guide.md) | Flannel 完整指南 (Terway 前身对比参考) |
| [34-network-performance-tuning.md](../domain-03-networking-traffic/34-network-performance-tuning.md) | 网络性能调优实践 |
| [domain-12-cloud-providers/04-alicloud-ack/242-ack-vpc-network.md](../domain-12-cloud-providers/04-alicloud-ack/242-ack-vpc-network.md) | ACK VPC 网络规划 |

### 其他关联专题

| 文档 | 说明 |
|:---|:---|
| [domain-11-production-operations/topic-presentations/kubernetes-terway-presentation.md](../domain-11-production-operations/topic-presentations/kubernetes-terway-presentation.md) | Terway 全栈进阶培训演示 |
| [domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni.md](../domain-11-production-operations/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni.md) | Terway CNI 入门学习材料 |
| [domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/07-terway-troubleshooting.md](../domain-10-troubleshooting-diagnostics/高级排障/03-networking/07-terway-troubleshooting.md) | Terway 结构化故障排查 |
| [domain-10-troubleshooting-diagnostics/topic-fta/list/terway-fta.md](../domain-10-troubleshooting-diagnostics/FTA故障树/list/terway-fta.md) | Terway 异常 FTA 故障树分析 |

---

**Kusheet Project** | 作者: Allen Galler (allengaller@gmail.com)

## Related

- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]


<!-- risk-assessed -->
