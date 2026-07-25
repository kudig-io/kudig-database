---
title: Network Service Mesh (NSM)
description: '## 概述'
summary: 'Network Service Mesh (NSM) 是一个混合/多云的 IP 服务网格，提供 L2/L3 层的网络服务连接能力。与传统的 Service Mesh（如 Istio、Linkerd 专注于 L4-L7）不同，NSM 专注于为应用提供底层网络服务，例如安全隧道、VPN、防火墙等网络功能的动态连接。'
category: entities
tags:
- k8s
- cncf
- networking
- network-service-mesh
- prometheus
- grafana
- istio
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Network Service Mesh (NSM) 是什么
- 如何 Network Service Mesh (NSM)
trigger_keywords:
- Network
- Service
- Mesh
- NSM
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[23-实体/04-网络/network-service-mesh.md|Network Service Mesh]]rvice]]Service Mesh）|Service Mesh]] (NSM)

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Network Service Mesh（NSM）是一个混合/多云的 IP 服务网格，提供 L2/L3 层的网络服务连接能力，2019 年加入 CNCF Sandbox。与传统的 Service Mesh（如 Istio、Linkerd 专注于 L4-L7）不同，NSM 专注于为应用提供底层网络服务——例如安全隧道、VPN、防火墙、负载均衡等网络功能的动态按需连接。NSM 通过灵活的拓扑设计满足复杂网络需求。

## 核心特性

- **L2/L3 连接**: 在 Pod 之间建立二层/三层网络连接
- **动态拓扑**: 按需创建网络服务链路，支持复杂网络拓扑
- **多数据平面**: 支持 Kernel 和 VPP（Vector Packet Processor）数据平面
- **SPIFFE 身份**: 集成 SPIRE 进行工作负载身份认证
- **跨集群连接**: 支持跨集群、跨云的网络服务连接
- **NSE 模型**: Network Service Endpoint 可由 Pod 动态提供

## 架构

NSM 采用客户端-服务端模型。核心组件包括：NSMgr（Network Service Manager，每个节点运行一个，管理本地连接）、Forwarder（数据平面，Kernel 或 VPP 模式，处理实际数据转发）、NSC（Network Service Client，发起连接请求的 Pod）、NSE（Network Service Endpoint，提供网络服务的 Pod）。NSC 通过 Pod 内的网卡向 NSMgr 发起 Network Service Request。NSMgr 根据请求选择合适的 NSE，在 NSC 和 NSE 之间建立隧道（VXLAN、Geneve 或直接路由）。

## Kubernetes 集成

NSM 通过 Mutating Webhook 自动为 Pod 注入 NSC init container，配置额外的网络接口。NSMgr 和 Forwarder 以 DaemonSet 部署在每个节点上。Network Service 通过 CRD（NetworkService）定义。NSE Pod 通过特定注解注册为网络服务端点。支持标准的 Kubernetes Service 和 Pod API。与 SPIRE/SPIFFE 集成实现 mTLS 工作负载身份。

## 生产使用场景

1. **安全隧道**: 为 Pod 间通信提供动态加密隧道
2. **多集群网络互通**: 建立跨集群的 L2/L3 网络连接
3. **网络功能链**: 将流量按顺序通过防火墙、负载均衡等网络功能
4. **传统应用迁移**: 为需要 L2 网络的传统应用提供连通性

## 安装与配置

```bash
# Helm 安装
helm repo add networkservicemesh https://networkservicemesh.github.io/charts
helm install nsm networkservicemesh/nsm \
  --set spire.enabled=true \
  --set forwarder.type=kernel \
  -n nsm-system --create-namespace
# 验证部署
kubectl get pods -n nsm-system
kubectl get networkServices -A
```

```yaml
# 定义 NetworkService
apiVersion: networkservicemesh.io/v1
kind: NetworkService
metadata:
  name: secure-intranet
  namespace: nsm-system
spec:
  payload: IP
  containerImage: ghcr.io/networkservicemesh/cmd-nse-firewall:latest
---
# NSC Pod 配置（通过注解注入）
apiVersion: v1
kind: Pod
metadata:
  name: app-with-nsm
  annotations:
    networkservicemesh.io: 'kernel://secure-intranet/nsm-1'
spec:
  containers:
  - name: app
    image: nginx:latest
```

```bash
# 检查 NSC 连接状态
kubectl exec -it app-with-nsm -- ip addr show nsm-1
kubectl exec -it app-with-nsm -- ping <nse-ip>
```

## 运维操作

```bash
# 🟢 查看 NSMgr 和 Forwarder 状态
kubectl get pods -n nsm-system -l app=nsmgr
kubectl get pods -n nsm-system -l app=forwarder-vpp

# 🟢 查看网络连接状态
kubectl get networkserviceendpoints -A
kubectl logs -n nsm-system -l app=nsmgr --tail=50

# 🟢 检查数据平面连接
kubectl exec -it <nsc-pod> -- ip route show
kubectl exec -it <nsc-pod> -- cat /proc/net/dev

# 🟡 重启 Forwarder（影响节点上所有连接）
kubectl rollout restart daemonset/forwarder-vpp -n nsm-system

# 🟡 重新注册 NSE
kubectl delete pod -l app=nse-firewall -n nsm-system

# 🔴 卸载 NSM（断开所有网络服务连接）
helm uninstall nsm -n nsm-system
kubectl delete namespace nsm-system
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| NSC Pod 无额外网卡 | Webhook 未注入/注解错误 | `kubectl describe pod <nsc>` | 检查注解格式和 Webhook 状态 |
| 连接超时 | Forwarder 未运行/路由缺失 | `kubectl logs -n nsm-system -l app=forwarder-vpp` | 重启 Forwarder 或检查内核模块 |
| NSE 未注册 | NSE Pod 异常/标签不匹配 | `kubectl get networkserviceendpoints -A` | 检查 NSE Pod 日志和标签 |
| 跨节点连接失败 | VXLAN/Geneve 隧道被防火墙拦截 | `iptables -L -n \| grep 4789` | 开放 UDP 4789/6081 端口 |
| SPIFFE 认证失败 | SPIRE Agent 未就绪 | `kubectl logs -n spire ds/spire-agent` | 检查 SPIRE 注册和证书 |

```
排查流程：
├─ NSC 连接失败
│  ├─ 检查 Pod 注解格式是否正确
│  ├─ 检查 NSMgr 是否 Running
│  └─ 检查 Forwarder 日志是否有连接错误
├─ 数据平面不通
│  ├─ ip addr 确认网卡已创建
│  ├─ ip route 确认路由已添加
│  └─ 检查节点间隧道端口是否开放
└─ 跨集群连接
   ├─ 检查 Interdomain DNS 配置
   └─ 确认远端集群 NSMgr 可达
```

## 生产案例

### 案例 1：金融系统安全隧道

- **场景**: 银行交易系统需要 Pod 间加密 L2 连接，传统 VPN 方案无法动态扩展
- **排查**: 使用 NSM + SPIFFE 为每对 Pod 建立 mTLS 加密隧道
- **方案**: 定义 NetworkService 为加密隧道类型，NSE 提供 IPsec 网关功能
- **效果**: 动态按需建立加密连接，无需预配置静态 VPN

### 案例 2：多集群网络功能链

- **场景**: 跨 3 个集群的流量需要经过防火墙→负载均衡→IDS 链
- **排查**: 传统方案需要静态路由配置，无法适应 Pod 动态调度
- **方案**: NSM 定义 NetworkServiceChain，流量自动按序通过各 NSE
- **效果**: 网络功能链随 Pod 动态编排，运维复杂度降低 70%

## 替代方案对比

| 维度 | NSM | Submariner | Cilium Cluster Mesh | Tailscale |
|------|-----|------------|--------------------|-----------| 
| 网络层级 | L2/L3 | L3 Pod 通信 | L3/L4 | L3 VPN |
| 动态拓扑 | ✅ NSE 模型 | 有限 | 有限 | ❌ |
| 跨集群 | ✅ | ✅ 核心功能 | ✅ | ✅ |
| 数据平面 | Kernel/VPP | 内核 | eBPF | WireGuard |
| 适用场景 | 复杂网络服务 | 多集群互通 | Cilium 环境 | 简单 VPN |

## 架构定位

在 CNCF 生态中，NSM 属于 **Networking** 类别，专注于 L2/L3 层的动态网络服务连接。它与传统 L4-L7 服务网格互补。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[deployment]]
- [[22-概念/03-网络/service-mesh-architecture.md|service-mesh-architecture]]

## Related

- [[chaosblade]] — ChaosBlade
- [[istio]] — Istio
- [[linkerd]] — Linkerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[spire]] — SPIRE

- network-service-mesh
- [[23-实体/15-参考与索引/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
