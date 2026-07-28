---
title: MetalLB (entities)
description: '## 概述'
summary: 'MetalLB 是为裸金属 Kubernetes 集群提供的负载均衡器实现。在云环境中，Kubernetes LoadBalancer 类型的 [[service|Service]] 由云提供商自动配置。MetalLB 填补了裸金属环境的空白，通过 Layer 2 (ARP/NDP) 或 BGP 协议为 Service 分配和公告外部 IP 地址。'
category: entities
tags:
- k8s
- cncf
- networking
- metallb
- prometheus
- grafana
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- MetalLB 是什么
- 如何 MetalLB
trigger_keywords:
- MetalLB
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# MetalLB

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

MetalLB 是为裸金属 Kubernetes 集群提供的 LoadBalancer 实现，2021 年加入 CNCF Sandbox，后晋升为 Incubating。在云环境中，Kubernetes LoadBalancer 类型的 [[service|Service]] 由云提供商自动配置。MetalLB 填补了裸金属环境的空白，通过 Layer 2（ARP/NDP）或 BGP 协议为 Service 分配和公告外部 IP 地址。它是裸金属集群使用 LoadBalancer Service 的标准方案。

## 核心特性

- **Layer 2 模式**: 使用 ARP（IPv4）/ NDP（IPv6）响应本地网络请求
- **BGP 模式**: 与网络路由器建立 BGP 会话公告 Service IP
- **IP 地址池**: 灵活配置可分配的 IP 地址范围和分配策略
- **自动故障转移**: Leader 选举确保 L2 模式高可用
- **双栈支持**: 同时支持 IPv4 和 IPv6
- **CRD 配置**: 使用 IPAddressPool、L2Advertisement、BGPAdvertisement CRD

## 架构

MetalLB 由两个组件组成。MetalLB Controller（Deployment，集群级单实例）监听 Kubernetes Service 变更，当发现 `type: LoadBalancer` 的 Service 时，从 IPAddressPool 中分配一个 IP 并更新 Service 的 `status.loadBalancer.ingress`。MetalLB Speaker（DaemonSet，每个节点一个）负责公告 IP 地址。Layer 2 模式下，Leader 节点的 Speaker 发送 Gratuitous ARP/NDP 响应，使局域网将流量发到该节点。BGP 模式下，所有 Speaker 节点与上游路由器建立 BGP 会话，公告 Service VIP，路由器通过 ECMP 将流量分发到多个节点。

## Kubernetes 集成

MetalLB 通过 CRD 和 Cloud Controller Manager 接口集成。IPAddressPool CRD 定义 IP 地址池。L2Advertisement/BGPAdvertisement CRD 定义公告模式。Controller 作为 Kubernetes Service 的 LoadBalancer Controller 运行，自动为 `type: LoadBalancer` Service 分配 IP。Speaker 通过 DaemonSet 运行。支持与 kube-proxy 协同工作——MetalLB 负责将外部流量引入集群节点，kube-proxy 负责将流量路由到目标 Pod。

## 生产使用场景

1. **裸金属集群入口**: 为裸金属 K8s 集群的 Ingress Controller 提供 LoadBalancer IP
2. **BGP 负载均衡**: 与数据中心交换机建立 BGP，实现多节点流量分发
3. **多 VIP 管理**: 为多个服务分配和管理外部 IP
4. **混合网络**: 在非云环境中实现类似云 ELB 的流量入口

## 安装与配置

```bash
# Helm 安装
helm repo add metallb https://metallb.github.io/metallb
helm install metallb metallb/metallb -n metallb-system --create-namespace
# 验证部署
kubectl get pods -n metallb-system
```

```yaml
# Layer 2 模式配置
apiVersion: metallb.io/v1beta1
kind: IPAddressPool
metadata:
  name: production-pool
  namespace: metallb-system
spec:
  addresses:
    - 192.168.1.240-192.168.1.250
  autoAssign: true
---
apiVersion: metallb.io/v1beta1
kind: L2Advertisement
metadata:
  name: l2-adv
  namespace: metallb-system
spec:
  ipAddressPools: ["production-pool"]
  nodeSelectors:
    - matchLabels:
        node-role.kubernetes.io/worker: "true"
---
# BGP 模式配置
apiVersion: metallb.io/v1beta1
kind: BGPPeer
metadata:
  name: router-peer
  namespace: metallb-system
spec:
  myASN: 64512
  peerASN: 64513
  peerAddress: 10.0.0.1
---
apiVersion: metallb.io/v1beta1
kind: BGPAdvertisement
metadata:
  name: bgp-adv
  namespace: metallb-system
spec:
  ipAddressPools: ["production-pool"]
  aggregationLength: 32
```

```bash
# 创建 LoadBalancer Service
kubectl expose deployment web --port=80 --type=LoadBalancer
kubectl get svc web -o jsonpath='{.status.loadBalancer.ingress[0].ip}'
```

## 运维操作

```bash
# 🟢 查看 IP 分配状态
kubectl get ipaddresspools -n metallb-system
kubectl get svc -A -o wide | grep LoadBalancer

# 🟢 查看 Speaker 状态和 Leader 选举
kubectl get pods -n metallb-system -l app=metallb,component=speaker
kubectl logs -n metallb-system -l component=speaker --tail=50

# 🟢 检查 BGP 会话状态
kubectl logs -n metallb-system -l component=speaker | grep -i bgp

# 🟡 强制释放 Service IP
kubectl annotate svc <name> metallb.io/address-pool- --overwrite

# 🟡 重启 Speaker（触发重新选举）
kubectl rollout restart daemonset/speaker -n metallb-system

# 🔴 删除 IPAddressPool（影响已分配 Service）
kubectl delete ipaddresspool <name> -n metallb-system
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Service 无 External IP | IP 池耗尽/未创建 | `kubectl get ipaddresspools -n metallb-system` | 扩展地址范围或新建 Pool |
| L2 模式流量中断 | Leader 节点宕机/ARP 未刷新 | `kubectl logs -l component=speaker` | 等待重新选举或重启 Speaker |
| BGP 会话未建立 | ASN/密码配置错误 | `kubectl logs -l component=speaker \| grep bgp` | 核对 BGPPeer CRD 配置 |
| IP 冲突 | 地址池与现有 IP 重叠 | `arping -I eth0 <vip>` | 调整 IPAddressPool 范围 |
| 多节点流量不均 | BGP ECMP 未启用 | 检查路由器 ECMP 配置 | 启用路由器 ECMP 或用 L2 模式 |

```
排查流程：
├─ Service 无 IP
│  ├─ 检查 IPAddressPool 是否存在且 autoAssign=true
│  └─ 检查 Controller 日志是否有分配错误
├─ IP 已分配但不可达
│  ├─ L2: 检查 Leader Speaker 是否运行 + ARP 响应
│  └─ BGP: 检查 BGP 会话状态 + 路由表
└─ 间歇性中断
   ├─ 检查 Speaker Pod 是否频繁重启
   └─ 检查节点网络接口是否稳定
```

## 生产案例

### 案例 1：裸金属集群 Ingress 入口高可用

- **场景**: 3 节点裸金属集群，需要为 Nginx Ingress 提供稳定 VIP
- **排查**: L2 模式下单节点故障导致 30s 流量中断（ARP 缓存过期时间）
- **方案**: 切换为 BGP 模式 + ECMP，3 节点同时公告 VIP，单节点故障 <1s 切换
- **效果**: 入口可用性从 99.9% 提升至 99.99%

### 案例 2：多租户 IP 地址池隔离

- **场景**: 多团队共享集群，需要不同 Namespace 使用不同 IP 段
- **排查**: 默认 Pool 被某团队大量 Service 耗尽
- **方案**: 创建多个 IPAddressPool + L2Advertisement，通过 namespaceSelector 隔离
- **效果**: 各团队 IP 资源独立管理，互不影响

## 替代方案对比

| 维度 | MetalLB | kube-vip | Porter | Cloud LB |
|------|---------|----------|--------|----------|
| 协议支持 | L2+BGP | L2+BGP | VXLAN | 云私有 |
| CNCF 状态 | Incubating | 非 CNCF | Sandbox | N/A |
| 多节点 LB | BGP ECMP | 仅 Active | 有限 | 原生 |
| 配置复杂度 | CRD 声明式 | 简单 | 简单 | 自动 |
| 适用场景 | 裸金属生产 | 轻量/双用途 | 简单场景 | 云环境 |

## 架构定位

在 CNCF 生态中，MetalLB 属于 **Networking / Load Balancing** 类别，是裸金属 Kubernetes 集群 LoadBalancer 的事实标准。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[22-概念/01-核心架构/controller-pattern.md|controller-pattern]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]

## Related

- [[cortex]] — Cortex
- [[kepler]] — Kepler
- [[kubestellar]] — KubeStellar
- [[athenz]] — Athenz
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- metallb
- [[23-实体/15-参考与索引/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
