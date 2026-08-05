---
title: 02 - Terway 架构原理 (Architecture Deep Dive)
description: '# 02 - Terway 架构原理 (Architecture Deep Dive)'
summary: 'Terway 是阿里云 ACK 的容器网络接口 (CNI) 插件, 核心设计目标是将 [[Kubernetes|Kubernetes]] Pod 直接接入阿里云 VPC 网络, 使 Pod 获得 VPC 级别的网络连通性, 同时保持与原生 VPC 网络策略和安全组的一致性.'
category: terway
tags:
- k8s
- terway
- networking
- alicloud
- kubelet
- cilium
- flannel
- daemonset
- ingress
- gateway
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
- Terway 架构原理 (Architecture Deep Dive) 是什么
- 如何 Terway 架构原理 (Architecture Deep Dive)
trigger_keywords:
- Terway
- 架构原理
- Architecture
- Deep
- Dive
- terway
prerequisites:
- kubectl-basics
- networking-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 02 - Terway 架构原理 (Architecture Deep Dive)

> **适用版本**: 阿里云 ACK v1.25 - v1.32+ | **Terway 版本**: v1.5+ | **最后更新**: 2026-05

---

## 目录

1. [整体架构](#1-整体架构)
2. [控制面组件](#2-控制面组件)
3. [数据面模式详解](#3-数据面模式详解)
4. [IPAM 机制](#4-ipam-机制)
5. [CRD 资源模型](#5-crd-资源模型)
6. [安全模型](#6-安全模型)
7. CNI 规范集成](#7-cni-规范集成)
8. [持久化与状态管理](#8-持久化与状态管理)
9. [交叉引用](#9-交叉引用)

---

## 1. 整体架构

Terway 是阿里云 ACK 的容器网络接口 (CNI) 插件, 核心设计目标是将 [[Kubernetes|Kubernetes]] Pod 直接接入阿里云 VPC 网络, 使 Pod 获得 VPC 级别的网络连通性, 同时保持与原生 VPC 网络策略和安全组的一致性.

### 1.1 网络拓扑全景图

```
                         阿里云 VPC (Virtual Private Cloud)
                         ┌─────────────────────────────────────────────────┐
                         │  VPC CIDR: 192.168.0.0/16                       │
                         │                                                  │
                         │  ┌──────────────────┐  ┌──────────────────┐     │
                         │  │   vSwitch-A       │  │   vSwitch-B       │     │
                         │  │ 192.168.0.0/24    │  │ 192.168.1.0/24    │     │
                         │  │ (可用区 cn-hangzhou-a) │ (可用区 cn-hangzhou-b) │
                         │  └────────┬─────────┘  └────────┬─────────┘     │
                         │           │                      │               │
                         │    ┌──────┴───────┐       ┌──────┴───────┐       │
                         │    │  ECS Node-1  │       │  ECS Node-2  │       │
                         │    │              │       │              │       │
                         │    │  ┌────────┐  │       │  ┌────────┐  │       │
                         │    │  │ ENI-0  │  │       │  │ ENI-0  │  │       │
                         │    │  │(主 ENI)│  │       │  │(主 ENI)│  │       │
                         │    │  └───┬────┘  │       │  └───┬────┘  │       │
                         │    │      │       │       │      │       │       │
                         │    │  ┌───┴────┐  │       │  ┌───┴────┐  │       │
                         │    │  │ ENI-1  │  │       │  │ ENI-1  │  │       │
                         │    │  │(辅 ENI)│  │       │  │(辅 ENI)│  │       │
                         │    │  │ .10    │  │       │  │ .50    │  │       │
                         │    │  │ .11    │  │       │  │ .51    │  │       │
                         │    │  │ .12    │  │       │  │ .52    │  │       │
                         │    │  └───┬────┘  │       │  └───┬────┘  │       │
                         │    │      │       │       │      │       │       │
                         │    │  ┌───┴───┐   │       │  ┌───┴───┐   │       │
                         │    │  │Pod A  │   │       │  │Pod C  │   │       │
                         │    │  │.10    │   │       │  │.50    │   │       │
                         │    │  ├───────┤   │       │  ├───────┤   │       │
                         │    │  │Pod B  │   │       │  │Pod D  │   │       │
                         │    │  │.11    │   │       │  │.51    │   │       │
                         │    │  └───────┘   │       │  └───────┘   │       │
                         │    └──────────────┘       └──────────────┘       │
                         │                                                  │
                         │         VPC 路由表 (Route Entry)                  │
                         │  ┌──────────────────────────────────────────┐    │
                         │  │ 192.168.0.0/24 → Node-1 (ENI-0)          │    │
                         │  │ 192.168.1.0/24 → Node-2 (ENI-0)          │    │
                         │  └──────────────────────────────────────────┘    │
                         └─────────────────────────────────────────────────┘
```

### 1.2 核心设计原则

| 原则 | 说明 |
|------|------|
| 原生 VPC 直通 | Pod IP 直接来自 VPC CIDR, 无 NAT 转发, 无 Overlay 封装开销 |
| 弹性网卡映射 | 通过阿里云 ENI (Elastic Network Interface) 实现底层网络连接 |
| 多模式适配 | 支持 VPC 路由 / ENI 独占 / ENIIP / Trunking / IPVlan 五种数据面模式 |
| 声明式管理 | 通过 CRD 描述网络资源需求, 控制器自动完成资源编排 |

---

## 2. 控制面组件

### 2.1 组件总览表

| 组件 | 形态 | 命名空间 | 职责 | 关键配置 |
|------|------|----------|------|----------|
| **Terway [[DaemonSet|DaemonSet]]** | DaemonSet (每个 Node 一个 Pod) | `kube-system` | 运行 CNI 插件二进制, 执行 IPAM, 管理 ENI/IP 资源池, 处理 CNI ADD/DEL/CHECK 请求 | `terway-eniip` (ENIIP 模式) / `terway-eni` (ENI 独占模式) |
| **Terway Controller** | Deployment (默认 1 副本, 可选 HA) | `kube-system` | Watch CRD 变更, 管理 ENI 生命周期, 节点网络资源协调, 垃圾回收, 状态同步 | 由 `terway-controller` Deployment 管理 |
| **eni-config ConfigMap** | ConfigMap | `kube-system` | 全局网络配置, 包括: VPC ID, vSwitch ID 列表, 安全组, 网络模式, IP 池大小, 是否启用 Trunk 等 | 所有 Terway 组件启动时读取 |

### 2.2 Terway DaemonSet 详解

DaemonSet 以 `hostNetwork: true` 方式运行, 确保 Node 本身的网络栈可用于 ENI 管理. 每个 DaemonSet Pod 内包含以下关键进程:

```
┌─────────────────────────────────────────────────────────┐
│                  Terway DaemonSet Pod                    │
│                                                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ terway-agent  │  │ terway-daemon │  │  terway-cni  │  │
│  │              │  │              │  │   (binary)    │  │
│  │ - gRPC server│  │ - ENI 管理   │  │              │  │
│  │ - IPAM 服务  │  │ - IP 池维护  │  │ 挂载至:       │  │
│  │ - 配置同步   │  │ - OpenAPI    │  │ /opt/cni/bin/ │  │
│  │              │  │   调用       │  │ /etc/cni/net.d│  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
│                                                          │
│  挂载卷:                                                  │
│  - /etc/cni/net.d     → CNI 配置文件                     │
│  - /opt/cni/bin       → CNI 二进制                       │
│  - /var/lib/cni       → 状态数据库                       │
│  - /run/terway        → Unix socket                      │
└─────────────────────────────────────────────────────────┘
```

**关键启动参数**:

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `--enable-eni-trunking` | false | 是否启用 Trunk ENI 模式 |
| `--pool-size` | 5 (ENIIP) | 本地 IP 预热池大小 |
| `--vswitch` | 从 ConfigMap 读取 | vSwitch ID, 支持多可用区配置 |
| `--security-group` | 从 ConfigMap 读取 | 安全组 ID 列表 |
| `--max-ip-per-eni` | 取决于实例规格 | 每个 ENI 可分配的辅助 IP 上限 |

### 2.3 Terway Controller 详解

Controller 是集群级别的中央协调器, 主要职责:

- **CRD Reconciliation**: Watch `PodENI`, `NodeNetworking`, `PodNetworking` 等 CRD 资源, 执行调和循环
- **ENI 生命周期管理**: 在需要时创建/附加 ENI 到 ECS 实例, 不需要时分离/释放
- **节点状态同步**: 维护每个节点的 ENI 和 IP 资源清单, 写入 Node Annotation
- **垃圾回收**: 清理残留的 ENI 资源, 回收泄漏的 IP 地址
- **多可用区调度**: 根据 Node 所在可用区选择对应的 vSwitch

### 2.4 eni-config ConfigMap 示例

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "access_key": "",
      "access_secret": "",
      "security_group": "sg-2zexxxxx",
      "service_cidr": "172.16.0.0/16",
      "vswitches": {
        "cn-hangzhou-a": ["vsw-2zexxxxx"],
        "cn-hangzhou-b": ["vsw-2zeyyyyy"]
      },
      "max_pool_size": 5,
      "min_pool_size": 0,
      "eni_type": "eniip",
      "enable_eni_trunking": false,
      "ip_type": "ipv4"
    }
```

---

## 3. 数据面模式详解

Terway 支持五种数据面模式, 每种模式在性能、密度、功能上各有取舍, 可根据业务场景灵活选择.

### 3.1 模式对比总览

| 模式 | 网络开销 | Pod 密度 | 性能 | 适用场景 | 内核要求 |
|------|----------|----------|------|----------|----------|
| VPC 路由模式 | 低 (无封装) | 高 | 中 (经主机转发) | 通用场景, 兼容性好 | 4.x |
| ENI 独占模式 | 最低 (直通) | 最低 (受 ENI 数限制) | 最高 | 高性能 / 低延迟业务 | 4.x |
| ENIIP 模式 (推荐) | 最低 (直通) | 中高 | 高 | 大多数生产场景 | 4.x |
| ENIIP-Trunking | 最低 (直通) | 最高 | 高 | Serverless / 高密度 | 4.19+ |
| IPVlan 模式 | 极低 (绕过 veth) | 高 | 极高 | 高性能计算 / eBPF 场景 | 4.19+ |

---

### 3.2 VPC 路由模式

#### 工作原理

VPC 路由模式类似于 Flannel host-gw, 通过在 VPC 路由表中添加路由条目实现跨节点 Pod 通信. 同节点的 Pod 共享节点的主 ENI, Pod 的流量通过主机网络栈转发.

```
    ┌───────────────────────────────────────────────────────┐
    │                     VPC 路由表                         │
    │                                                       │
    │  目的 CIDR          下一跳类型      下一跳             │
    │  ─────────────────  ──────────     ──────             │
    │  192.168.0.0/24     ECS Instance   i-2zecxxxx (Node-1)│
    │  192.168.1.0/24     ECS Instance   i-2zecyyyy (Node-2)│
    │  192.168.2.0/24     ECS Instance   i-2zeczzzz (Node-3)│
    └───────────────────────────────────────────────────────┘

    数据流:
    ┌──────┐    ┌──────────┐    ┌────────┐   VPC 路由    ┌────────┐    ┌──────────┐    ┌──────┐
    │Pod A ├───>│ Node-1   ├───>│ ENI-0  │══════════════>│ ENI-0  ├───>│ Node-2   ├───>│Pod B │
    │.0.10 │    │ 路由表   │    │ 主 ENI │   (VPC 网络)   │ 主 ENI │    │ 路由表   │    │.1.20 │
    └──────┘    └──────────┘    └────────┘               └────────┘    └──────────┘    └──────┘
         同节点: 直接通过 veth pair + 网桥转发
```

#### 限制与注意事项

| 限制项 | 值 | 说明 |
|--------|---|------|
| VPC 路由条目上限 | 默认 48 (可申请提升至 200) | 等于集群最大节点数 |
| 跨节点通信 | 依赖 VPC 路由转发 | 增加一跳延迟 |
| Pod 网段规划 | 每节点分配一个 /24 子网 | 需要提前规划 CIDR |
| 安全组 | 节点级别生效 | 无法实现 Pod 级别安全组 |

---

### 3.3 ENI 独占模式

#### 工作原理

每个 Pod 独占一个 ENI (弹性网卡), Pod 直接持有 ENI 的网络设备, 获得最高网络性能, 无任何转发开销.

```
    ┌─────────────────────────────────────────────────────────────┐
    │                      ECS Node                               │
    │                                                              │
    │   eth0 (ENI-0, 主 ENI, 节点自身使用)                         │
    │   192.168.0.100                                              │
    │                                                              │
    │   ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
    │   │   eth1      │  │   eth2      │  │   eth3      │        │
    │   │  (ENI-1)    │  │  (ENI-2)    │  │  (ENI-3)    │  ...   │
    │   │ 192.168.0.10│  │ 192.168.0.11│  │ 192.168.0.12│        │
    │   │             │  │             │  │             │        │
    │   │  ┌──────┐   │  │  ┌──────┐   │  │  ┌──────┐   │        │
    │   │  │Pod A │   │  │  │Pod B │   │  │  │Pod C │   │        │
    │   │  │独占  │   │  │  │独占  │   │  │  │独占  │   │        │
    │   │  └──────┘   │  │  └──────┘   │  │  └──────┘   │        │
    │   └─────────────┘  └─────────────┘  └─────────────┘        │
    │                                                              │
    └─────────────────────────────────────────────────────────────┘

    数据流:
    ┌──────┐     ┌───────┐     ┌──────┐
    │Pod A │────>│ ENI-1 │════>│ VPC  │    直通, 零额外跳数
    └──────┘     └───────┘     └──────┘
```

#### ENI 数量限制 (按实例规格)

| 实例规格族 | 最大 ENI 数 (含主 ENI) | 可分配给 Pod 的 ENI 数 |
|-----------|----------------------|----------------------|
| ecs.g7.large | 3 | 2 |
| ecs.g7.xlarge | 4 | 3 |
| ecs.g7.2xlarge | 6 | 5 |
| ecs.g7.4xlarge | 8 | 7 |
| ecs.g7.8xlarge | 16 | 15 |

> **密度公式**: 可调度 Pod 数 = 实例最大 ENI 数 - 1 (主 ENI 保留给节点自身)

**优势**: 性能最优, 延迟最低, 带宽最大, Pod 独享网卡带宽.
**劣势**: 密度极低, 成本高, 不适合大规模微服务场景.

---

### 3.4 ENIIP 模式 (推荐)

#### 工作原理

ENIIP 模式是 Terway 的默认和推荐模式. 核心思路是在每个辅助 ENI 上创建多个辅助私有 IP (Secondary IP), 每个 Pod 分配一个辅助 IP, 通过 veth pair 将 Pod 网络命名空间与 ENI 上的 IP 映射.

#### 详细数据流

```
    ┌─────────────────────────────────────────────────────────────────────────────┐
    │                            ECS Node                                         │
    │                                                                              │
    │  ┌────────────────────────────────────────────────────────────────────────┐ │
    │  │                       主机网络命名空间 (Host Netns)                      │ │
    │  │                                                                        │ │
    │  │  eth0 (ENI-0, 主 ENI)              节点管理 IP: 192.168.0.100           │ │
    │  │       │                                                                │ │
    │  │       ├── 192.168.0.100 (节点自身)                                     │ │
    │  │                                                                        │ │
    │  │  eth1 (ENI-1)  辅助 ENI                                                │ │
    │  │       │                                                                │ │
    │  │       ├── 192.168.0.101 (辅助 IP-1, Secondary IP) ── veth-A ── Pod A  │ │
    │  │       ├── 192.168.0.102 (辅助 IP-2, Secondary IP) ── veth-B ── Pod B  │ │
    │  │       ├── 192.168.0.103 (辅助 IP-3, Secondary IP) ── veth-C ── Pod C  │ │
    │  │       ├── 192.168.0.104 (辅助 IP-4, Secondary IP) ── (IP 池预分配)     │ │
    │  │       ├── 192.168.0.105 (辅助 IP-5, Secondary IP) ── (IP 池预分配)     │ │
    │  │                                                                        │ │
    │  │  eth2 (ENI-2)  辅助 ENI                                                │ │
    │  │       │                                                                │ │
    │  │       ├── 192.168.0.106 (辅助 IP-1, Secondary IP) ── veth-D ── Pod D  │ │
    │  │       ├── 192.168.0.107 (辅助 IP-2, Secondary IP) ── (IP 池预分配)     │ │
    │  │       ├── 192.168.0.108 (辅助 IP-3, Secondary IP) ── (IP 池预分配)     │ │
    │  │       ...                                                              │ │
    │  │                                                                        │ │
    │  └────────────────────────────────────────────────────────────────────────┘ │
    │                                                                              │
    │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │
    │  │   Pod A      │  │   Pod B      │  │   Pod C      │  │   Pod D      │   │
    │  │  (netns)     │  │  (netns)     │  │  (netns)     │  │  (netns)     │   │
    │  │              │  │              │  │              │  │              │   │
    │  │ eth0         │  │ eth0         │  │ eth0         │  │ eth0         │   │
    │  │ 192.168.0.101│  │ 192.168.0.102│  │ 192.168.0.103│  │ 192.168.0.106│   │
    │  │      │       │  │      │       │  │      │       │  │      │       │   │
    │  │ veth-A       │  │ veth-B       │  │ veth-C       │  │ veth-D       │   │
    │  │ (pair 另一端) │  │ (pair 另一端) │  │ (pair 另一端) │  │ (pair 另一端) │   │
    │  └──────────────┘  └──────────────┘  └──────────────┘  └──────────────┘   │
    │                                                                              │
    └──────────────────────────────────────────────────────────────────────────────┘

    veth pair 映射关系详解:

    Pod A (netns)                    Host (netns)
    ┌─────────────┐                 ┌──────────────────┐
    │  eth0       │ <── veth pair ─>│ veth-A (Host端)  │
    │ 192.168.0.101│                │ 192.168.0.101    │  ←── 绑定到 eth1 (ENI-1)
    │             │                 │ (配置在 ENI 辅助IP)│      辅助 IP #1
    │  路由:       │                 └──────────────────┘
    │  default →  │
    │  eth0       │
    └─────────────┘

    数据包流 (Pod A → 外部):
    Pod A eth0 → veth pair → veth-A (host端)
    → 通过路由规则匹配 eth1 (ENI-1)
    → ENI-1 发出 → VPC 网络

    数据包流 (外部 → Pod A):
    VPC → ENI-1 eth1 → 匹配 192.168.0.101
    → 路由/转发规则 → veth-A → veth pair → Pod A eth0
```

#### 容量计算

```
节点可调度 Pod 数量 = (最大 ENI 数 - 1) * 每 ENI 最大辅助 IP 数

示例 (ecs.g7.4xlarge):
  最大 ENI 数 = 8
  每 ENI 最大辅助 IP 数 = 30
  可调度 Pod 数 = (8 - 1) * 30 = 210 个 Pod

示例 (ecs.g7.2xlarge):
  最大 ENI 数 = 6
  每 ENI 最大辅助 IP 数 = 15
  可调度 Pod 数 = (6 - 1) * 15 = 75 个 Pod
```

> 完整容量速查表见 [01-product.md 第 7 节](./01-product.md#7-ecs-实例规格-eni-限制速查)。

#### 常见实例规格容量表

> 以下数据为 ENIIP 模式参考值，完整数据见 [01-product.md](./01-product.md)。

| 实例规格 | 最大 ENI | 每 ENI 辅助 IP | 可调度 Pod 数 | 推荐场景 |
|----------|---------|---------------|-------------|---------|
| ecs.g7.xlarge | 4 | 10 | 30 | 测试 / 小规模 |
| ecs.g7.2xlarge | 6 | 15 | 75 | 中等规模 |
| ecs.g7.4xlarge | 8 | 30 | 210 | 大规模生产 |
| ecs.g7.8xlarge | 16 | 30 | 450 | 计算密集型 / 超大规模 |

> **注意**: ENI 数需减 1, 因为 eth0 (主 ENI) 保留给节点管理网络.

---

### 3.5 ENIIP-Trunking 模式

#### 工作原理

Trunking 模式利用阿里云 ENI Trunk 能力, 在单个 Trunk ENI 上承载多个 Member ENI, 每个 Member ENI 对应一个 Pod, 支持独立的 VLAN Tag 和安全组.

```
    ┌────────────────────────────────────────────────────────────────────┐
    │                          ECS Node                                  │
    │                                                                     │
    │  eth0 (ENI-0, 主 ENI)                                              │
    │  192.168.0.100                                                     │
    │                                                                     │
    │  eth1 (Trunk ENI)  ←── 特殊的 ENI, 支持 VLAN Trunk                 │
    │       │                                                             │
    │       ├── VLAN 100 ── Member ENI-1 ── Pod A (独立安全组 sg-web)     │
    │       ├── VLAN 101 ── Member ENI-2 ── Pod B (独立安全组 sg-db)      │
    │       ├── VLAN 102 ── Member ENI-3 ── Pod C (独立安全组 sg-cache)   │
    │       ├── VLAN 103 ── Member ENI-4 ── Pod D                        │
    │       ├── ...                                                      │
    │       └── VLAN NNN ── Member ENI-N ── Pod N                        │
    │                                                                     │
    │  关键: 每个 Member ENI 是独立的虚拟网络设备                          │
    │       - 拥有独立 MAC 地址                                            │
    │       - 拥有独立安全组                                                │
    │       - 拥有独立带宽配额                                              │
    │       - 通过 VLAN Tag 在 Trunk ENI 上复用物理链路                    │
    │                                                                     │
    └────────────────────────────────────────────────────────────────────┘

    Trunk ENI 容量:
    ┌───────────────────────────────────────────┐
    │ Trunk ENI 最大 Member 数: 取决于实例规格    │
    │                                           │
    │ ecs.g7.xlarge:    最大 ~30 Member ENI     │
    │ ecs.g7.4xlarge:   最大 ~60 Member ENI     │
    │ ecs.g7.8xlarge:   最大 ~100+ Member ENI   │
    │                                           │
    │ 优势: 密度远超普通 ENIIP 模式              │
    └───────────────────────────────────────────┘
```

#### 核心优势

| 特性 | 说明 |
|------|------|
| 最高 Pod 密度 | 单节点可支持数十至上百 Pod, 远超普通 ENIIP |
| Pod 级安全组 | 每个 Member ENI 可绑定不同安全组, 实现 Pod 粒度网络策略 |
| 独立带宽 | 每个 Member ENI 有独立带宽配额, 避免争抢 |
| Serverless 友好 | ACK Serverless (ASK) / ECI 底层使用 Trunking 模式 |
| 内核要求 | Linux Kernel 4.19+ (需要 VLAN 设备支持) |

#### 配置示例

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "network_type": "ENIIP",
      "vswitches": {"cn-hangzhou-b": ["vsw-xxx"]},
      "security_group": "sg-xxx",
      "enable_eni_trunking": true
    }
```

> 注意: Trunking 模式需要 ECS 实例规格支持 ENI Trunk 能力，且内核 >= 4.19。

---

### 3.6 IPVlan 模式

#### 工作原理

IPVlan 模式使用 Linux 内核的 IPVlan L2 子功能, 在底层 ENI 上创建多个 IPVlan 子设备, 每个 Pod 持有一个 IPVlan 子设备, 完全绕过 veth pair 开销.

```
    ┌────────────────────────────────────────────────────────────────────────┐
    │                           ECS Node                                     │
    │                                                                        │
    │  eth0 (ENI-0, 主 ENI)         192.168.0.100                            │
    │                                                                        │
    │  eth1 (ENI-1, 辅助 ENI)      物理网卡设备                               │
    │    │                                                                   │
    │    ├── eth1.1 (IPVlan L2 子设备)  192.168.0.101  ──> Pod A (netns)     │
    │    ├── eth1.2 (IPVlan L2 子设备)  192.168.0.102  ──> Pod B (netns)     │
    │    ├── eth1.3 (IPVlan L2 子设备)  192.168.0.103  ──> Pod C (netns)     │
    │    ├── eth1.4 (IPVlan L2 子设备)  192.168.0.104  ──> (预热池)          │
    │    ├── eth1.5 (IPVlan L2 子设备)  192.168.0.105  ──> (预热池)          │
    │    └── ...                                                            │
    │                                                                        │
    │  eth2 (ENI-2, 辅助 ENI)                                               │
    │    │                                                                   │
    │    ├── eth2.1 (IPVlan L2 子设备)  192.168.0.201  ──> Pod D (netns)     │
    │    └── ...                                                            │
    │                                                                        │
    │  IPVlan L2 关键特性:                                                   │
    │  - 子设备与父设备共享 MAC 地址                                          │
    │  - L2 层广播/多播由内核处理 (不经过父设备)                              │
    │  - 子设备直接处理 L3 及以上数据包                                       │
    │  - 无 veth pair, 无 netfilter 开销                                     │
    │                                                                        │
    └────────────────────────────────────────────────────────────────────────┘

    性能对比:

    ENIIP 模式 (veth pair):
    Pod → veth → Netfilter → 路由 → veth (host端) → ENI → 网络栈
           ↑                    ↑
           中断开销             Conntrack 查找

    IPVlan 模式 (绕过 veth):
    Pod → IPVlan 子设备 → ENI → 网络栈
           ↑                ↑
           直接映射          无额外中断

    eBPF 加速 (可选):
    Pod → IPVlan 子设备 → eBPF 程序 (TC/XDP) → ENI → 网络栈
                             ↑
                             绕过内核协议栈, 直接转发
```

#### IPVlan 模式要求与限制

| 项目 | 要求 |
|------|------|
| 内核版本 | Linux 4.19+ (推荐 5.10+) |
| IPVlan L2 支持 | 内核配置 `CONFIG_IPVLAN` |
| eBPF 加速 | 内核 5.4+, 需开启 Cilium/eBPF 集成 |
| 兼容性 | 不支持内核模块依赖较重的网络功能 (如某些 iptables 场景) |
| 性能提升 | 相比 veth pair: P99 延迟降低 20-40%, 吞吐提升 15-30% |
| 部署方式 | 集群创建时选定, 不支持在线切换 |

---

## 4. IPAM 机制

Terway 实现了本地 IP 池 + 云端 OpenAPI 两级 IPAM, 兼顾分配速度和资源利用率.

### 4.1 IP 分配流程 (CNI ADD)

```
    Kubernetes API Server
           │
           │ 1. Pod 创建请求 (调度到 Node-A)
           ▼
    ┌──────────────┐
    │   kubelet    │ (Node-A)
    └──────┬───────┘
           │
           │ 2. 调用 CNI ADD (通过 /opt/cni/bin/terway)
           │    CNI 配置: /etc/cni/net.d/10-terway.conflist
           ▼
    ┌──────────────────────────────────────┐
    │       Terway CNI 插件               │
    │                                      │
    │   3. 通过 Unix Socket 连接本地       │
    │      Terway Daemon (gRPC)            │
    └──────────────┬───────────────────────┘
                   │
                   ▼
    ┌──────────────────────────────────────┐
    │       Terway Daemon (IPAM)           │
    │                                      │
    │   4. 查询本地 IP 池                  │
    │      (Local Pool)                    │
    └──────────────┬───────────────────────┘
                   │
            ┌──────┴──────┐
            │  池中有 IP?  │
            └──────┬──────┘
              Yes/ │ \No
                /  │  \
    ┌─────────┐   │   ┌─────────────────────────────┐
    │ 5a. 分配 │   │   │ 5b. 调用阿里云 OpenAPI       │
    │ 本地 IP  │   │   │     AllocateEniSecondaryIp   │
    │ 返回 Pod │   │   │     或 CreateEni + 分配 IP   │
    └────┬────┘   │   └────────────┬────────────────┘
         │        │                │
         │        │         ┌──────┴──────┐
         │        │         │ 6b. 新 IP 加入│
         │        │         │ 本地池, 再分配│
         │        │         └──────┬──────┘
         │        │                │
         ▼        ▼                ▼
    ┌──────────────────────────────────────┐
    │   6. 配置 Pod 网络命名空间           │
    │      - 创建 veth pair / IPVlan 设备  │
    │      - 绑定 IP 到网络设备             │
    │      - 配置路由规则                   │
    │      - 写入 resourceDB               │
    └──────────────┬───────────────────────┘
                   │
                   ▼
    ┌──────────────────────────────────────┐
    │   7. 返回 CNI ADD 结果               │
    │      { cniVersion, interfaces,       │
    │        ips: [{ address, gateway }] }  │
    └──────────────┬───────────────────────┘
                   │
                   ▼
    ┌──────────────┐
    │   Pod Ready  │  IP: 192.168.0.x, 直连 VPC
    └──────────────┘
```

### 4.2 IP 回收流程 (CNI DEL)

```
    Kubernetes API Server
           │
           │ 1. Pod 删除请求
           ▼
    ┌──────────────┐
    │   kubelet    │
    └──────┬───────┘
           │
           │ 2. 调用 CNI DEL
           ▼
    ┌──────────────────────────────────────┐
    │       Terway CNI 插件               │
    └──────────────┬───────────────────────┘
                   │
                   ▼
    ┌──────────────────────────────────────┐
    │       Terway Daemon (IPAM)           │
    │                                      │
    │   3. 解配置 Pod 网络命名空间         │
    │      - 删除 veth pair / IPVlan 设备  │
    │      - 清除路由规则                   │
    │      - 从 resourceDB 移除记录        │
    │                                      │
    │   4. IP 归还本地池 (Local Pool)      │
    │      标记为 available                 │
    └──────────────┬───────────────────────┘
                   │
                   ▼
    ┌──────────────────────────────────────┐
    │   5. 周期性 GC (Garbage Collection)  │
    │                                      │
    │   定期检查本地池中的空闲 IP:          │
    │   - 如果空闲 IP 数 > min_pool_size   │
    │     → 调用 OpenAPI 释放辅助 IP       │
    │     → ReleaseEniSecondaryIp          │
    │   - 如果 ENI 上无辅助 IP 且非保留    │
    │     → 调用 OpenAPI 分离/删除 ENI     │
    │     → DetachEni / DeleteEni          │
    │                                      │
    │   GC 周期: 默认每 60 秒              │
    └──────────────────────────────────────┘
```

### 4.3 IP 池管理策略

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `max_pool_size` | 5 | 本地池最大预分配 IP 数 |
| `min_pool_size` | 0 | 本地池最小保留 IP 数 (低于此值触发预分配) |
| 预分配触发 | 异步 | 池中 IP 低于阈值时异步调用 OpenAPI 补充 |
| 回收策略 | 惰性 | IP 归还池中, 不立即释放, 由 GC 定期清理 |

---

## 5. CRD 资源模型

Terway 通过自定义资源 (CRD) 声明式管理网络资源, 实现与 Kubernetes 控制循环的深度集成.

### 5.1 CRD 总览表

| CRD 名称 | API 版本 | 作用域 | 功能说明 | 状态 |
|-----------|----------|--------|----------|------|
| `PodENI` | `network.alibabacloud.com/v1beta1` | Namespaced | 记录 Pod 与 ENI 的绑定关系, 包括 ENI ID, MAC 地址, 分配的 IP 地址, 所属节点等信息 | 稳定 |
| `NodeNetworking` | `network.alibabacloud.com/v1beta1` | Cluster | 描述节点的网络资源配置, 包括已附加的 ENI 列表, 已分配的 IP 池, 可用容量等 | 稳定 |
| `PodNetworking` | `network.alibabacloud.com/v1beta1` | Cluster | 定义 Pod 网络配置模板, 包括: 使用哪种网络模式 (ENI/ENIIP/Trunking), 绑定哪个安全组, vSwitch 选择策略等 | 稳定 |
| `ReservedIP` | `network.alibabacloud.com/v1beta1` | Cluster | IP 预留, 防止指定 IP 被 GC 回收, 用于固定 IP 场景 (如数据库 VIP, 有状态服务) | 稳定 |
| `IPInstance` | `network.alibabacloud.com/v1beta1` | Cluster | 记录每个 IP 实例的完整生命周期状态, 包括: IP 地址, 所属 ENI, 绑定的 Pod, 状态 (Available/InUse/Deleting) | 稳定 |

### 5.2 PodENI 示例

```yaml
apiVersion: network.alibabacloud.com/v1beta1
kind: PodENI
metadata:
  name: default-myapp-pod-a1b2c3
  namespace: default
spec:
  pod:
    name: myapp-pod-a1b2c3
    namespace: default
  eni:
    id: eni-2ze8xxxxx
    mac: "00:16:3e:12:34:56"
    type: eniip
    vSwitchId: vsw-2zexxxxx
    securityGroupIDs:
      - sg-2zeaaaaa
status:
  eni:
    ip:
      - 192.168.0.101
    ipv6:
      - 2001:db8::101
  phase: Bound
  node: cn-hangzhou.ack-node-1
```

### 5.3 IPInstance 状态流转

```
    ┌───────────┐     CNI ADD      ┌────────┐     Pod Running     ┌────────┐
    │ Available │ ──────────────> │ InUse  │ ──────────────> │ InUse  │
    │ (池中空闲) │                  │ (已分配) │                  │ (运行中) │
    └───────────┘                  └────────┘                  └───┬────┘
         ▲                                                        │
         │                     CNI DEL                            │
         │                   ◄──────────────                       │
         │                                                        │
         │                                                        ▼
    ┌───────────┐           GC 回收         ┌──────────┐     Pod 删除     ┌───────────┐
    │ Available │ ◄─────────────────────── │ Deleting │ ◄────────────── │ InUse     │
    │ (回到池中) │                           │ (回收中)  │                 │ (待回收)   │
    └───────────┘                            └──────────┘                 └───────────┘
```

> 参考: 详细的 CRD API 定义和控制器逻辑见 [domain-03-networking-traffic/37](../domain-03-networking-traffic/37-crd-networking.md)

---

## 6. 安全模型

Terway 在多个层面提供网络安全能力, 与阿里云 VPC 安全体系深度集成.

### 6.1 安全层级总览

| 层级 | 机制 | 资源粒度 | 说明 |
|------|------|----------|------|
| **节点级** | Security Group (安全组) | ECS 实例 / ENI | 绑定在 ENI 上的安全组, 控制进出节点的所有流量. 主 ENI 和辅助 ENI 可绑定不同安全组 |
| **Pod 级** | Member ENI Security Group | Pod (Trunking 模式) | Trunking 模式下, 每个 Member ENI 可绑定独立安全组, 实现 Pod 粒度访问控制 |
| **策略级** | NetworkPolicy | Pod / Namespace | Terway 原生支持 Kubernetes NetworkPolicy, 基于 ENI 安全组和底层 ebpf/iptables 实现. 支持 Ingress/Egress 规则 |
| **权限级** | RAM (Resource Access Management) | ECS 实例角色 | 通过 ECS 实例角色 (Instance RAM Role) 授予 Terway 调用阿里云 OpenAPI 的权限, 避免在 ConfigMap 中硬编码 AK/SK |
| **网络隔离** | VPC / vSwitch | 集群级别 | 通过 VPC 和 vSwitch 实现集群间网络隔离, 不同集群使用不同 VPC 或不同子网 |

### 6.2 RAM 权限策略

Terway 需要以下最小 RAM 权限来管理 ENI 和 IP 资源:

| API 操作 | 用途 |
|----------|------|
| `ecs:CreateNetworkInterface` | 创建辅助 ENI |
| `ecs:DeleteNetworkInterface` | 删除 ENI |
| `ecs:AttachNetworkInterface` | 将 ENI 附加到 ECS 实例 |
| `ecs:DetachNetworkInterface` | 从 ECS 实例分离 ENI |
| `ecs:AssignPrivateIpAddresses` | 为 ENI 分配辅助私有 IP |
| `ecs:UnassignPrivateIpAddresses` | 释放 ENI 上的辅助 IP |
| `ecs:DescribeNetworkInterfaces` | 查询 ENI 信息 |
| `ecs:DescribeInstances` | 查询 ECS 实例规格 (确定 ENI/IP 配额) |
| `ecs:CreateNetworkInterfacePermission` | Trunking 模式下的 ENI 权限管理 |
| `ecs:DescribeSecurityGroups` | 查询安全组信息 |

### 6.3 NetworkPolicy 实现方式

```
    ┌──────────────────────────────────────────────────────────┐
    │                   NetworkPolicy 实现                      │
    │                                                          │
    │  Kubernetes NetworkPolicy API                            │
    │       │                                                  │
    │       ▼                                                  │
    │  Terway Controller (Watch NetworkPolicy)                 │
    │       │                                                  │
    │       ├── 模式 1: ENI 安全组规则                          │
    │       │   将 NetworkPolicy 规则转换为安全组规则           │
    │       │   适用于 ENI 独占 / Trunking 模式                │
    │       │                                                  │
    │       ├── 模式 2: iptables / ip6tables                   │
    │       │   在节点上配置 iptables 规则                      │
    │       │   适用于 ENIIP 模式                              │
    │       │                                                  │
    │       └── 模式 3: eBPF (可选, 性能更优)                  │
    │           通过 TC/eBPF 程序实现策略过滤                   │
    │           适用于 IPVlan + eBPF 场景                      │
    │                                                          │
    └──────────────────────────────────────────────────────────┘
```

---

## 7. CNI 规范集成

Terway 完整实现 Kubernetes CNI (Container Network Interface) 规范, 是 kubelet 与容器网络之间的桥梁.

### 7.1 CNI 插件文件布局

```
    节点文件系统:
    /etc/cni/net.d/
    └── 10-terway.conflist          ← CNI 配置文件 (kubelet 读取)
        内容示例:
        {
          "cniVersion": "0.4.0",
          "name": "terway",
          "plugins": [
            {
              "type": "terway",
              "eniip_virtual_type": "ipvlan",
              "ipam": { "type": "terway" },
              "network_policy_enable": true
            },
            {
              "type": "portmap",
              "capabilities": { "portMappings": true }
            }
          ]
        }

    /opt/cni/bin/
    ├── terway                       ← Terway CNI 二进制 (由 DaemonSet 挂载)
    ├── portmap                      ← 端口映射插件 (CNI 社区标准)
    ├── bandwidth                    ← 带宽控制插件
    └── loopback                     ← 回环设备插件
```

### 7.2 CNI 操作生命周期

| 操作 | 触发条件 | Terway 行为 |
|------|----------|-------------|
| **CNI ADD** | Pod 创建, kubelet 调用 CNI 插件 | 分配 IP, 配置网络设备 (veth/IPVlan), 设置路由, 配置安全组规则, 返回结果 |
| **CNI DEL** | Pod 删除, kubelet 调用 CNI 插件 | 回收 IP 到本地池, 清理网络设备, 移除路由规则 |
| **CNI CHECK** | kubelet 周期性检查 Pod 网络状态 | 验证网络设备存在, IP 配置正确, 路由规则有效. 异常时返回错误触发重建 |

### 7.3 CNI ADD 详细流程

```
    kubelet
       │
       │ exec /opt/cni/bin/terway ADD
       │ stdin: CNI_NETNS, CNI_IFNAME, CNI_ARGS (K8S_POD_NAME, K8S_POD_NAMESPACE, ...)
       ▼
    ┌────────────────────────────────────────────────────────────────────┐
    │ Terway CNI Binary                                                  │
    │                                                                    │
    │  1. 解析 CNI 环境变量和 stdin JSON                                 │
    │  2. 通过 Unix Socket (/run/terway/terway.sock) 连接 Terway Daemon  │
    │  3. 发送 gRPC 请求: AllocIP(PodName, Namespace, NetNS, IfName)     │
    │  4. 等待 Daemon 返回分配结果 (IP, Gateway, Routes, DNS)             │
    │  5. 配置 Pod 网络命名空间:                                         │
    │     a. 创建 veth pair / IPVlan 子设备                              │
    │     b. 将一端移入 Pod netns, 重命名为 eth0                         │
    │     c. 配置 IP 地址和路由                                          │
    │     d. 配置 sysctl 参数 (rp_filter, forward 等)                    │
    │  6. 输出 JSON 结果到 stdout                                        │
    │     {                                                              │
    │       "cniVersion": "0.4.0",                                      │
    │       "interfaces": [...],                                        │
    │       "ips": [{"address": "192.168.0.101/24", "gateway": "..."}],  │
    │       "routes": [{"dst": "0.0.0.0/0", "gw": "..."}],              │
    │       "dns": {"nameservers": [...]}                                │
    │     }                                                              │
    └────────────────────────────────────────────────────────────────────┘
```

---

## 8. 持久化与状态管理

Terway 需要在节点本地维护网络资源的状态映射关系, 确保节点重启后能正确恢复.

### 8.1 状态存储架构

```
    ┌─────────────────────────────────────────────────────────────────┐
    │                    节点本地状态                                  │
    │                                                                 │
    │  /var/lib/cni/terway/                                           │
    │  ├── ResRelation.db      ← BoltDB 数据库                       │
    │  │   存储: Pod ↔ ENI ↔ IP ↔ veth 设备 映射关系               │
    │  │   用途: CNI CHECK 验证, 节点重启恢复, 状态审计              │
    │  │                                                              │
    │  └── terway.cni.conf     ← CNI 配置缓存                       │
    │                                                                 │
    │  /run/terway/                                                   │
    │  └── terway.sock         ← gRPC Unix Socket                    │
    │     CNI Binary 与 Daemon 之间的通信通道                        │
    │                                                                 │
    └─────────────────────────────────────────────────────────────────┘

    ┌─────────────────────────────────────────────────────────────────┐
    │                    Node Annotation 状态                          │
    │                                                                 │
    │  metadata.annotations:                                          │
    │    "network.alibabacloud.com/eni": |                            │
    │      [                                                          │
    │        {                                                        │
    │          "id": "eni-2ze8xxxx",                                  │
    │          "mac": "00:16:3e:12:34:56",                            │
    │          "type": "eniip",                                       │
    │          "status": "inuse",                                     │
    │          "ips": [                                               │
    │            {"address": "192.168.0.101", "status": "inuse"},    │
    │            {"address": "192.168.0.102", "status": "available"} │
    │          ]                                                      │
    │        }                                                        │
    │      ]                                                          │
    │                                                                 │
    │    "network.alibabacloud.com/ipv4": "192.168.0.100"            │
    │    "network.alibabacloud.com/node-capacity": "105"              │
    │    "network.alibabacloud.com/used": "42"                        │
    │                                                                 │
    └─────────────────────────────────────────────────────────────────┘
```

### 8.2 ResRelation.db 数据结构

BoltDB 是一个轻量级嵌入式 Key-Value 数据库, Terway 使用它存储以下映射关系:

| Bucket | Key | Value | 用途 |
|--------|-----|-------|------|
| `podEniMap` | `{namespace}/{podName}` | ENI ID, IP 地址, 设备名 | Pod 到 ENI 的绑定关系 |
| `eniIPMap` | ENI ID | 辅助 IP 列表, 状态 | ENI 上的 IP 分配状态 |
| `podIPMap` | `{namespace}/{podName}` | IP 地址, veth 设备名 | Pod 到 IP 和设备的映射 |
| `resourcePool` | 资源类型 | 可用资源列表 | 本地资源池快照 |

### 8.3 状态恢复流程

```
    节点重启
       │
       ▼
    ┌────────────────────────────────────────────────────┐
    │ Terway DaemonSet Pod 启动                          │
    │                                                    │
    │  1. 读取 ResRelation.db, 重建本地资源映射          │
    │  2. 调用 OpenAPI 查询当前 ENI/IP 状态             │
    │  3. 对比本地状态与云端实际状态:                     │
    │     - 不一致 → 以云端为准, 修复本地状态            │
    │     - 残留资源 → 标记为可回收, 等待 GC 清理        │
    │  4. 补充本地 IP 池至 min_pool_size                 │
    │  5. 更新 Node Annotation                           │
    │  6. 就绪, 开始处理 CNI 请求                        │
    │                                                    │
    └────────────────────────────────────────────────────┘
```

---

## 9. 交叉引用

### 9.1 本主题内引用

| 文件 | 说明 |
|------|------|
| [01-product.md](./01-product.md) | Terway 产品概览, 版本历史, CNI 对比, ECS 规格速查 |
| [03-usage.md](./[[32-发布/package/2026-07-02_18-29/corpus/core/domain-03-networking-traffic/topic-terway/01-usage|03-usage]].md) | 安装配置, 模式切换, NetworkPolicy, 固定 IP, Annotation 速查 |
| [04-operations.md](03-operations.md) | 运维操作, GC 机制, 健康检查, 故障排查, 升级流程 |
| [05-testing.md](04-testing.md) | 端到端测试套件, NetworkPolicy 测试, 性能基准验证 |
| [06-performance.md](32-发布/package/2026-07-02_18-29/corpus/supporting/domain-03-networking-traffic/topic-terway/01-performance.md) | 性能基准测试, 各模式对比, 内核调优, 生产基线 |

### 9.2 跨域引用

| 文件 | 说明 |
|------|------|
| [domain-03-networking-traffic/05-terway-advanced-guide.md](32-发布/package/2026-07-02_18-29/corpus/supporting/domain-03-networking-traffic/00-core-k8s-networking/01-terway-advanced-guide.md) | Terway 高级指南, 模式对比, ENIIP 详解 |
| [domain-03-networking-traffic/37-terway-resources-crud-operations.md](32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-03-networking-traffic/00-core-k8s-networking/29-terway-resources-crud-operations.md) | 网络相关 CRD 的详细 API 定义与 CRUD 操作 |
| [domain-03-networking-traffic/38-terway-gc-mechanism.md](32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-03-networking-traffic/00-core-k8s-networking/30-terway-gc-mechanism.md) | GC 垃圾回收机制详解, 设计原则, 触发链路 |
| [domain-03-networking-traffic/02-cni-architecture-fundamentals.md](32-发布/package/2026-07-02_18-29/corpus/core/domain-03-networking-traffic/00-core-k8s-networking/01-cni-architecture-fundamentals.md) | CNI 架构基础与核心原理 |
| [domain-03-networking-traffic/34-network-performance-tuning.md](32-发布/package/2026-07-02_18-29/corpus/peripheral/domain-03-networking-traffic/00-core-k8s-networking/26-network-performance-tuning.md) | 网络性能调优通用指南 |
| [domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/07-terway-troubleshooting.md](../domain-10-troubleshooting-diagnostics/高级排障/03-networking/07-terway-troubleshooting.md) | Terway 结构化故障排查 |
| [domain-10-troubleshooting-diagnostics/topic-fta/list/terway-fta.md](../domain-10-troubleshooting-diagnostics/FTA故障树/list/terway-fta.md) | Terway 异常 FTA 故障树 |

---

## 附录 A: 术语表

| 术语 | 全称 | 说明 |
|------|------|------|
| ENI | Elastic Network Interface | 阿里云弹性网卡, VPC 中的虚拟网络设备 |
| ENIIP | ENI Secondary IP | ENI 上的辅助私有 IP 地址 |
| IPAM | IP Address Management | IP 地址分配与管理机制 |
| vSwitch | Virtual Switch | 阿里云虚拟交换机, VPC 子网 |
| CNI | Container Network Interface | Kubernetes 容器网络接口规范 |
| CRD | Custom Resource Definition | Kubernetes 自定义资源定义 |
| GC | Garbage Collection | 垃圾回收, 清理残留网络资源 |
| Trunk ENI | Trunk Elastic Network Interface | 支持承载多个 Member ENI 的特殊 ENI |
| Member ENI | Member Elastic Network Interface | Trunk ENI 上的子 ENI, 对应单个 Pod |
| RAM | Resource Access Management | 阿里云访问控制服务 |

## 附录 B: 内核版本兼容性

| 数据面模式 | 最低内核版本 | 推荐内核版本 | 操作系统 |
|-----------|-------------|-------------|---------|
| VPC 路由 | 4.x | 4.19+ | CentOS 7/8, Alibaba Cloud Linux 2/3 |
| ENI 独占 | 4.x | 4.19+ | 同上 |
| ENIIP | 4.x | 4.19+ | 同上 |
| ENIIP-Trunking | 4.19 | 5.10+ | Alibaba Cloud Linux 2/3 |
| IPVlan | 4.19 | 5.10+ | Alibaba Cloud Linux 2/3 |

> **推荐**: 生产环境建议使用 Alibaba Cloud Linux 3 (内核 5.10+), 获得最佳兼容性和性能.

## Related

- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]


<!-- risk-assessed -->
