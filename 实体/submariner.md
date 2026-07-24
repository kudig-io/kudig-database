---
title: Submariner (entities)
description: '## 概述'
summary: 'Submariner 实现 Kubernetes 多集群之间的 Pod 和 Service 网络直连。'
category: entities
tags:
- k8s
- cncf
- networking
- submariner
- gateway
- crd
- operator
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Submariner 是什么
- 如何 Submariner
trigger_keywords:
- Submariner
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Submariner

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Submariner 实现 Kubernetes 多集群之间的 Pod 和 [[Service|Service]] 网络直连，由 Red Hat 推动开发，2021 年加入 CNCF 沙箱。它在集群之间建立加密隧道（IPsec/WireGuard），允许跨集群的 Pod 直接通信和 Service 发现，解决了多集群环境下的网络连通性问题。Submariner 的核心组件包括 Gateway Engine（管理网关节点和隧道）、Route Agent（配置节点路由）和 Lighthouse（跨集群 DNS 解析）。Globalnet 组件处理跨集群 CIDR 重叠的场景——通过分配全局 IP 映射解决地址冲突。Submariner 是 OpenShift ACM 多集群解决方案的网络基础组件。

## 核心能力

- **跨集群 Pod 网络**: Pod 到 Pod 跨集群直接通信（无需 NAT）
- **跨集群 Service 发现**: 通过 ServiceImport/ServiceExport 实现跨集群 DNS 解析
- **加密隧道**: IPsec 或 WireGuard 加密保护跨集群流量
- **Globalnet**: 处理跨集群 CIDR 重叠场景，分配全局 IP
- **Gateway 选举**: 自动选举和切换网关节点，实现高可用
- **Lighthouse DNS**: 跨集群 Service DNS 解析（`<service>.<namespace>.svc.clusterset.local`）

## 架构

Submariner 采用 Gateway + Route Agent 架构：

- **Gateway Engine**: 运行在网关节点上的组件，管理 IPsec/WireGuard 隧道
- **Route Agent (DaemonSet)**: 每个节点上的路由代理，配置跨集群路由
- **Service Discovery (Lighthouse)**: 跨集群 DNS 解析服务
- **Broker**: 运行在一个集群中的 CRD 代理，存储所有集群的元数据
- **Globalnet Controller**: 处理 CIDR 重叠，分配全局 IP
- **Gateway Node**: 专门用于跨集群流量的节点（通过 label 选择）

数据流：`Pod A (集群1) → Route Agent → Gateway → IPsec 隧道 → Gateway → Route Agent → Pod B (集群2)`

## K8s 集成

Submariner 通过 Operator 模式部署在 Kubernetes 集群中。每个集群部署 Submariner Operator 和 Gateway Engine、Route Agent（DaemonSet）。通过 Broker（运行在中心集群中的 CRD）协调所有集群的状态。ServiceExport CRD 标记需要跨集群暴露的 Service，Lighthouse 自动将其注册到跨集群 DNS。Pod 通过 DNS 名称（`<service>.<namespace>.svc.clusterset.local`）可以访问其他集群的 Service。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准 Service、Endpoint 和 DNS 机制集成。

## 生产场景

1. **多集群微服务**: 跨集群的微服务直接通信，实现多集群应用协同
2. **数据库跨集群访问**: 应用集群访问数据集群中的数据库 Pod
3. **灾难恢复**: 故障转移时跨集群访问关键 Service
4. **混合云网络**: 私有云和公有云集群之间的网络直连

## 安装与配置

```bash
# 安装 subctl CLI
curl -L https://github.com/submariner-io/releases/releases/download/v0.18.0/subctl-v0.18.0-linux-amd64 -o subctl
chmod +x subctl && mv subctl /usr/local/bin/

# 在集群 A 上创建 Broker
subctl deploy-broker --kubeconfig cluster-a.kubeconfig

# 将集群 A 加入 Submariner
subctl join --kubeconfig cluster-a.kubeconfig broker-info.subm \
  --clusterid cluster-a --natt --cable-driver wireguard

# 将集群 B 加入 Submariner
subctl join --kubeconfig cluster-b.kubeconfig broker-info.subm \
  --clusterid cluster-b --natt --cable-driver wireguard

# 验证连接
subctl show all --kubeconfig cluster-a.kubeconfig
subctl verify --kubeconfig cluster-a.kubeconfig --to-kubeconfig cluster-b.kubeconfig
```

```yaml
# ServiceExport（跨集群暴露 Service）
apiVersion: multicluster.x-k8s.io/v1alpha1
kind: ServiceExport
metadata:
  name: payment-service
  namespace: production
---
# 或使用 subctl 命令
# subctl export service payment-service -n production --kubeconfig cluster-a.kubeconfig
---
# Gateway 节点配置（通过 label 选择）
# kubectl label node worker-1 submariner.io/gateway=true
---
# Globalnet 配置（处理 CIDR 重叠）
# subctl deploy-broker --globalnet
# subctl join broker-info.subm --globalnet --clusterid cluster-b
```

## 运维操作

```bash
# 🟢 低风险：查看 Submariner 状态
subctl show all --kubeconfig cluster-a.kubeconfig
subctl show connections --kubeconfig cluster-a.kubeconfig
subctl show endpoints --kubeconfig cluster-a.kubeconfig

# 🟢 低风险：查看跨集群 DNS
kubectl get serviceexports -A
kubectl get serviceimports -A

# 🟡 中风险：导出/取消导出 Service
subctl export service my-svc -n default --kubeconfig cluster-a.kubeconfig
subctl unexport service my-svc -n default --kubeconfig cluster-a.kubeconfig

# 🟡 中风险：切换 Gateway 节点
subctl gateway failover --kubeconfig cluster-a.kubeconfig

# 🔴 高风险：从 Submariner 移除集群
subctl uninstall --kubeconfig cluster-b.kubeconfig

# 🟢 低风险：运行连接性测试
subctl verify --kubeconfig cluster-a.kubeconfig --to-kubeconfig cluster-b.kubeconfig
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 跨集群 Pod 不通 | Gateway 隧道未建立 | `subctl show connections` | 检查 NAT/防火墙，确认 UDP 4500/500 开放 |
| DNS 解析失败 | Lighthouse 未就绪 | `kubectl get pods -n submariner-operator -l app=lighthouse` | 检查 ServiceExport 是否创建 |
| Gateway 频繁切换 | 节点网络不稳定 | `subctl show endpoints` | 使用专用 Gateway 节点，配置健康检查 |
| Globalnet IP 不分配 | GlobalCIDR 耗尽 | `kubectl get globalingressips -A` | 扩大 GlobalCIDR 范围 |
| 连接延迟高 | MTU 不匹配 | `subctl show connections` | 调整 cable MTU（通常 1400） |

```
排查流程：
├── 跨集群不通？
│   ├── subctl show connections → 检查隧道状态
│   ├── subctl verify → 运行连接性测试
│   └── 检查防火墙（UDP 4500/500、ESP）
├── Service 发现失败？
│   ├── kubectl get serviceexports → 确认已导出
│   ├── kubectl get serviceimports → 确认已导入
│   └── nslookup <svc>.<ns>.svc.clusterset.local
└── 性能问题？
    ├── 检查 Gateway 节点网络带宽
    ├── 调整 MTU 设置
    └── 考虑使用 WireGuard 替代 IPsec
```

## 生产案例

### 案例 1：混合云微服务跨集群通信

- **场景**：应用服务在私有云 K8s 集群，数据库在公有云 K8s 集群，需要直接通信
- **排查**：传统方案需要 VPN + 手动路由，配置复杂且延迟高
- **方案**：使用 Submariner 建立 WireGuard 隧道，通过 ServiceExport 暴露数据库 Service，应用通过 clusterset.local DNS 直接访问
- **效果**：跨集群延迟 < 5ms，配置时间从 2 天缩短至 30 分钟

### 案例 2：CIDR 重叠集群互联

- **场景**：两个集群都使用 10.244.0.0/16 Pod CIDR，无法直接路由
- **排查**：传统方案需要重新规划集群网络，影响所有工作负载
- **方案**：启用 Submariner Globalnet，为每个集群分配全局 CIDR，自动 NAT 转换
- **效果**：无需修改现有集群网络，10 分钟内实现 CIDR 重叠集群互联

## 对比

| 特性 | Submariner | Cilium Cluster Mesh | Skupper | KubeFed (deprecated) |
|------|-----------|--------------------|---------|---------------------|
| L3 Pod 通信 | ✅ | ✅ | ❌ | ❌ |
| 加密隧道 | ✅ IPsec/WG | ⚠️ WireGuard | ✅ TLS | ❌ |
| Service 发现 | ✅ Lighthouse | ✅ | ✅ | ❌ |
| CNCF 状态 | Sandbox | Graduated | Sandbox | Archived |

## 架构定位

在 CNCF 生态中，Submariner 属于 **Networking** 类别，为云原生应用提供多集群网络直连能力。

## 参考链接

- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[pod-lifecycle]]

## Related

- [[hwameistor]] — HwameiStor
- [[dragonfly]] — Dragonfly
- [[aeraki-mesh]] — Aeraki Mesh
- [[atlantis]] — Atlantis
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- submariner
- [[技能/集群运维/cloud-provider/诊断排障/ts-cloud-provider.md|云服务商集成排查]] — Cross-reference
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference


<!-- risk-assessed -->
