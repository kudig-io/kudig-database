---
title: Terway 架构深度解析
description: '# Terway 架构深度解析'
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
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 架构深度解析 是什么
- 如何 Terway 架构深度解析
trigger_keywords:
- Terway
- 架构深度解析
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Terway 架构深度解析

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

title: 02 - Terway 架构原理 (Architecture Deep Dive)

## 技术细节

### 架构总览

```
┌─────────────────────────────────────────────────────────────────┐
│                        Kubernetes Control Plane                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │  API Server │  │  Scheduler  │  │  Terway Controller      │  │
│  └──────┬──────┘  └──────┬──────┘  │  (Deployment)           │  │
│         │                │         │  - PodENI Controller    │  │
│         │                │         │  - IPAM Controller      │  │
│         │                │         │  - NetworkPolicy Sync   │  │
│         │                │         └─────────────────────────┘  │
└─────────┼────────────────┼──────────────────────────────────────┘
          │                │
          ▼                ▼
┌─────────────────────────────────────────────────────────────────┐
│                           Worker Node                            │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    Terway DaemonSet                      │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐  │   │
│  │  │  CNI Plugin │  │  IPAM Pool  │  │  Policy Agent   │  │   │
│  │  │  (terway)   │  │  Manager    │  │  (NetworkPolicy)│  │   │
│  │  └──────┬──────┘  └──────┬──────┘  └─────────────────┘  │   │
│  │         │                │                              │   │
│  │         ▼                ▼                              │   │
│  │  ┌─────────────────────────────────────────────────┐   │   │
│  │  │              ENI Manager                         │   │   │
│  │  │  - ENI 创建/删除/绑定                            │   │   │
│  │  │  - 辅助 IP 分配/释放                             │   │   │
│  │  └─────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              │                                  │
│                              ▼                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    Linux Kernel                          │   │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐    │   │
│  │  │  veth   │  │  bridge │  │  route  │  │  iptables│   │   │
│  │  │  pair   │  │         │  │  table  │  │  /ebpf  │    │   │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────┘    │   │
│  └─────────────────────────────────────────────────────────┘   │
│                              │                                  │
│                              ▼                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    ENI (Elastic Network Interface)       │   │
│  │                    - 主 IP / 辅助 IP                      │   │
│  │                    - 安全组                               │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                        阿里云 VPC                                │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  │
│  │   vSwitch   │  │  Route Table│  │  Security Group         │  │
│  │  (子网)     │  │  (路由表)   │  │  (安全组)               │  │
│  └─────────────┘  └─────────────┘  └─────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件

#### 1. Terway Controller (Deployment)

运行在控制平面，负责集群级别的网络资源管理：

```bash
# 🟢 低风险：查看 Controller 状态
kubectl get deploy -n kube-system terway-controlplane

# 🟢 低风险：查看 Controller 日志
kubectl logs -n kube-system -l app=terway-controlplane --tail=50
```

**主要功能**:
- PodENI CRD 生命周期管理
- ENI 资源配额监控
- NetworkPolicy 同步到各节点

#### 2. Terway DaemonSet

每个节点运行一个实例，负责节点级别的网络配置：

```bash
# 🟢 低风险：查看 DaemonSet 状态
kubectl get ds -n kube-system terway-eniip

# 🟢 低风险：查看特定节点的 Terway Pod
kubectl get pods -n kube-system -l app=terway-eniip -o wide

# 🟢 低风险：查看 Terway 配置
kubectl get cm -n kube-system eni-config -o yaml
```

**主要功能**:
- CNI 插件调用处理
- ENI/辅助 IP 分配与释放
- IP 池预热与管理
- NetworkPolicy 执行

#### 3. CNI Plugin

二进制文件位于 `/opt/cni/bin/terway`，由 kubelet 调用：

```bash
# 🟢 低风险：检查 CNI 配置
cat /etc/cni/net.d/10-terway.conflist

# 🟢 低风险：检查 CNI 二进制
ls -la /opt/cni/bin/terway

# 🟢 低风险：查看 CNI 日志
cat /var/log/terway/cni.log
```

### 网络模式详解

#### ENI 独占模式

```
┌─────────────────────────────────────────────────────────┐
│                      Worker Node                         │
│                                                         │
│  ┌─────────────┐         ┌─────────────────────────┐   │
│  │    Pod      │         │    ENI (独占)            │   │
│  │  ┌───────┐  │  veth   │  ┌───────────────────┐  │   │
│  │  │ eth0  │──┼─────────┼──│ 主 IP: 192.168.1.10│  │   │
│  │  └───────┘  │  pair   │  └───────────────────┘  │   │
│  └─────────────┘         └─────────────────────────┘   │
│                                                         │
│  特点: 每个 Pod 独占一个 ENI，性能最高，密度最低          │
└─────────────────────────────────────────────────────────┘
```

**适用场景**: 数据库、网关、高性能计算

#### ENI 多 IP 模式 (ENIIP)

```
┌─────────────────────────────────────────────────────────┐
│                      Worker Node                         │
│                                                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐     │
│  │   Pod A     │  │   Pod B     │  │   Pod C     │     │
│  │  ┌───────┐  │  │  ┌───────┐  │  │  ┌───────┐  │     │
│  │  │ eth0  │  │  │  │ eth0  │  │  │  │ eth0  │  │     │
│  │  └───┬───┘  │  │  └───┬───┘  │  │  └───┬───┘  │     │
│  └──────┼──────┘  └──────┼──────┘  └──────┼──────┘     │
│         │                │                │             │
│         ▼                ▼                ▼             │
│  ┌─────────────────────────────────────────────────┐   │
│  │              ENI (多 IP 共享)                    │   │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐         │   │
│  │  │辅助 IP 1│  │辅助 IP 2│  │辅助 IP 3│         │   │
│  │  └─────────┘  └─────────┘  └─────────┘         │   │
│  │  主 IP: 192.168.1.10                             │   │
│  └─────────────────────────────────────────────────┘   │
│                                                         │
│  特点: 多个 Pod 共享 ENI 的辅助 IP，性能与密度平衡       │
└─────────────────────────────────────────────────────────┘
```

**适用场景**: 通用工作负载（推荐默认）

#### IPVlan 模式

```
┌─────────────────────────────────────────────────────────┐
│                      Worker Node                         │
│                                                         │
│  ┌─────────────┐  ┌─────────────┐                      │
│  │   Pod A     │  │   Pod B     │                      │
│  │  ┌───────┐  │  │  ┌───────┐  │                      │
│  │  │ eth0  │  │  │  │ eth0  │  │                      │
│  │  └───┬───┘  │  │  └───┬───┘  │                      │
│  └──────┼──────┘  └──────┼──────┘                      │
│         │                │                             │
│         ▼                ▼                             │
│  ┌─────────────────────────────────────────────────┐   │
│  │           IPVlan (L2/L3 模式)                    │   │
│  │           直接桥接到 ENI                         │   │
│  └─────────────────────────────────────────────────┘   │
│         │                                              │
│         ▼                                              │
│  ┌─────────────────────────────────────────────────┐   │
│  │                    ENI                           │   │
│  └─────────────────────────────────────────────────┘   │
│                                                         │
│  特点: 绕过宿主机网络栈，性能接近物理机，需内核 4.19+    │
└─────────────────────────────────────────────────────────┘
```

**适用场景**: 高性能场景、内核版本较新的集群

### 数据流分析

#### Pod 创建流程

```
1. kubectl create pod
        │
        ▼
2. API Server 创建 Pod 对象
        │
        ▼
3. Scheduler 选择节点
        │
        ▼
4. kubelet 调用 CNI 插件
        │
        ▼
5. Terway CNI 请求 IP
        │
        ├─── 从本地 IP 池分配 (快)
        │
        └─── 或调用阿里云 API 分配新 IP (慢)
        │
        ▼
6. 配置网络命名空间
   - 创建 veth pair
   - 配置 IP 地址
   - 设置路由规则
        │
        ▼
7. 创建 PodENI CRD 记录
        │
        ▼
8. Pod 进入 Running 状态
```

#### Pod 删除流程

```
1. kubectl delete pod
        │
        ▼
2. kubelet 调用 CNI DEL
        │
        ▼
3. Terway 释放 IP
        │
        ├─── 归还到本地 IP 池 (快)
        │
        └─── 或调用阿里云 API 释放 IP (慢)
        │
        ▼
4. 清理网络命名空间
        │
        ▼
5. 删除 PodENI CRD
```

### 架构验证命令

```bash
# 🟢 低风险：检查 Terway 组件状态
kubectl get pods -n kube-system -l app=terway-eniip -o wide
kubectl get pods -n kube-system -l app=terway-controlplane

# 🟢 低风险：检查节点网络配置
kubectl get nodes -o custom-columns=NAME:.metadata.name,CIDR:.spec.podCIDR

# 🟢 低风险：检查 ENI 配置
kubectl get cm -n kube-system eni-config -o yaml

# 🟢 低风险：检查节点 ENI 使用情况
kubectl get podeni -A -o wide | grep <node-name>

# 🟢 低风险：检查 Terway 指标
curl -s http://localhost:19090/metrics | grep terway
```

## 参考链接

Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]

## 生产部署建议

- 建议在生产环境中使用 ENI 多 IP 模式以提高 IP 利用率 ^[inferred]
- 密切监控 ENI 资源使用情况，避免 IP 耗尽 ^[inferred]
- 配合 [[networkpolicy|NetworkPolicy]] 实现 Pod 间访问控制 ^[inferred]

## 参考链接

- [[cilium]]
- [[22-概念/03-网络/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]]

## Related

- [[45-terway-crd-operations]] — Terway CRD 资源操作
- [[ovn-kubernetes]] — OVN-Kubernetes
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[cni]] — CNI (Container Network Interface)

- [[46-terway-operations-manual]]
- [[42-terway-product-overview]]
- [[44-terway-usage-guide]]
- [[48-terway-performance-tuning]]
- [[47-terway-testing-validation]]
- [[49-terway-troubleshooting-fta]]
- 41-terway-architecture-deep-dive
- [[23-实体/15-参考与索引/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference


<!-- risk-assessed -->
