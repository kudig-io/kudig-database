---
title: Antrea [entities]
description: '## 概述'
summary: 'Antrea 是基于 Open vSwitch (OVS) 构建的 Kubernetes 网络解决方案，为 Pod 网络提供高性能数据平面。它实现了 Kubernetes [[NetworkPolicy|NetworkPolicy]] API，并扩展支持更细粒度的流量控制，包括 ClusterNetworkPolicy、Egress 和流量可观测性功能。'
category: entities
tags:
- k8s
- cncf
- networking
- antrea
- prometheus
- grafana
- gateway
- networkpolicy
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
- Antrea 是什么
- 如何 Antrea
trigger_keywords:
- Antrea
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Antrea

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go

## 概述

Antrea 是由 VMware 开源的 Kubernetes CNI 网络插件，基于 Open vSwitch（OVS）构建，2021 年加入 CNCF Sandbox。它为 Pod 网络提供高性能数据平面，完整实现 Kubernetes [[NetworkPolicy|NetworkPolicy]] API，并扩展支持更细粒度的流量控制，包括 ClusterNetworkPolicy、Egress 和流量可观测性功能。Antrea 是 VMware Tanzu 的默认 CNI，适合需要高级网络功能的企业场景。

## 核心特性

- **高性能数据平面**: 基于 OVS 的内核态数据路径，支持 DPDK 加速
- **NetworkPolicy**: 完整支持 K8s NetworkPolicy + Antrea ClusterNetworkPolicy 扩展
- **Egress 网关**: 集中管理 Pod 出站流量，支持固定 Source IP
- **流量可观测性**: Flow Exporter (IPFIX)、Traceflow 网络诊断
- **多集群互联**: Antrea Multi-cluster 实现跨集群 Pod 网络互通
- **加密通信**: 支持 IPsec 和 WireGuard 加密 Pod 间流量

## 架构

Antrea 以 OVS 为核心数据平面。每个节点运行 antrea-agent（控制器）和 antrea-ovs（OVS 守护进程）。antrea-agent 监听 K8s API 获取 Pod、NetworkPolicy 和 Service 变更，将配置转换为 OVS flow rules。OVS 在内核态处理 Pod 间流量，支持 Geneve 隧道（跨节点）或 noEncap（同节点直连）模式。antrea-controller（集群级，HA 部署）负责 IPAM 和 NetworkPolicy 全局视图。Flow Aggregator 收集各节点的流量记录（IPFIX 格式）。

## Kubernetes 集成

Antrea 作为标准 CNI 插件集成。通过 antrea-agent Pod（DaemonSet）监听 K8s API，自动配置 OVS 管理网络。支持标准 NetworkPolicy API 和扩展的 ClusterNetworkPolicy/Tier CRD。Egress CRD 定义 Pod 出站流量的 Source IP 和目标限制。支持 Service LoadBalancer 和 NodePort。通过 NetworkPolicy CRD 和 Antrea ClusterNetworkPolicy 提供超越 K8s 原生策略的高级规则（如 ICMP、FQDN）。

## 生产使用场景

1. **企业网络隔离**: 使用 ClusterNetworkPolicy 实现多团队网络安全隔离
2. **Egress 控制**: 为 Pod 出站流量指定固定 IP 或 SNAT 策略
3. **网络诊断**: 使用 Traceflow 快速定位 Pod 间通信问题
4. **多集群互联**: 使用 Antrea Multi-cluster 打通多个集群的 Pod 网络

## 安装与配置

```bash
# 一键安装
kubectl apply -f https://github.com/antrea-io/antrea/releases/download/v1.16.0/antrea.yml
# 验证安装
kubectl get pods -n kube-system | grep antrea
kubectl get nodes -o wide  # 确认所有节点 Ready
```

```yaml
# ClusterNetworkPolicy 示例：默认拒绝 + 允许特定流量
apiVersion: crd.antrea.io/v1beta1
kind: ClusterNetworkPolicy
metadata:
  name: default-deny-ingress
spec:
  tier: emergency
  priority: 100
  appliedTo:
  - podSelector: {}
  ingress: []
---
apiVersion: crd.antrea.io/v1beta1
kind: ClusterNetworkPolicy
metadata:
  name: allow-dns
spec:
  tier: securityops
  priority: 10
  appliedTo:
  - podSelector: {}
  egress:
  - action: Allow
    to:
    - namespaceSelector: {}
    ports:
    - protocol: UDP
      port: 53
---
# Egress CRD：固定出站 IP
apiVersion: crd.antrea.io/v1beta1
kind: Egress
metadata:
  name: egress-prod
spec:
  appliedTo:
    namespaceSelector:
      matchLabels:
        env: production
  egressIP: 10.10.0.100
```

```bash
# Traceflow 网络诊断
kubectl apply -f - <<EOF
apiVersion: crd.antrea.io/v1beta1
kind: Traceflow
metadata:
  name: tf-test
spec:
  source:
    namespace: default
    pod: web-1
  destination:
    namespace: default
    pod: api-1
  packet:
    ipHeader:
      protocol: 6
    transportHeader:
      tcp:
        dstPort: 8080
EOF
kubectl get traceflow tf-test -o yaml
```

## 运维操作

```bash
# 🟢 查看 Antrea 组件状态
kubectl get pods -n kube-system -l app=antrea
kubectl get pods -n kube-system -l app=antrea-agent

# 🟢 查看 NetworkPolicy 状态
kubectl get networkpolicies -A
kubectl get acnp -A  # Antrea ClusterNetworkPolicy
kubectl get anp -A   # Antrea NetworkPolicy

# 🟢 查看 OVS 流表
kubectl exec -n kube-system <antrea-agent-pod> -c antrea-ovs -- ovs-ofctl dump-flows br-int

# 🟢 查看流量记录
kubectl get egress -A
kubectl logs -n kube-system -l app=antrea -c antrea-agent --tail=50

# 🟡 重启 Antrea Agent
kubectl rollout restart daemonset/antrea-agent -n kube-system

# 🔴 卸载 Antrea（影响所有 Pod 网络）
kubectl delete -f antrea.yml
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 间通信失败 | OVS 流表缺失/隧道异常 | `ovs-ofctl dump-flows br-int` | 重启 antrea-agent |
| NetworkPolicy 不生效 | ACNP 优先级冲突 | `kubectl get acnp -o yaml` | 检查 tier 和 priority |
| 跨节点流量丢失 | Geneve 隧道被防火墙拦截 | `iptables -L -n \| grep 6081` | 开放 UDP 6081 端口 |
| Egress IP 不生效 | IP 未分配/路由缺失 | `kubectl get egress -o yaml` | 检查 Egress CRD 状态 |
| antrea-agent CrashLoop | OVS 版本不兼容 | `kubectl logs -n kube-system <agent>` | 检查 OVS 版本兼容性 |

```
排查流程：
├─ Pod 网络不通
│  ├─ Traceflow 诊断流量路径
│  ├─ 检查 OVS 流表是否正确
│  └─ 检查 Geneve 隧道状态
├─ NetworkPolicy 问题
│  ├─ kubectl get acnp 检查策略状态
│  ├─ 检查 tier/priority 优先级
│  └─ 确认 appliedTo 选择器匹配
└─ Egress 问题
   ├─ 检查 Egress CRD 状态
   ├─ 确认 egressIP 可路由
   └─ 检查节点网络配置
```

## 生产案例

### 案例 1：企业多租户网络隔离

- **场景**: 多团队共享集群，需要严格的网络隔离和审计
- **排查**: 标准 NetworkPolicy 不支持跨 Namespace 全局策略
- **方案**: Antrea ClusterNetworkPolicy + Tier 实现分层策略（Emergency/SecurityOps/App）
- **效果**: 统一网络安全策略管理，审计合规通过率 100%

### 案例 2：网络故障快速定位

- **场景**: 微服务间歇性超时，传统工具无法定位网络层问题
- **排查**: 使用 Antrea Traceflow 追踪数据包路径，发现某节点 OVS 流表异常
- **方案**: 重启异常节点 antrea-agent，启用 Flow Exporter 持续监控
- **效果**: 网络故障定位时间从小时级降至分钟级

## 替代方案对比

| 维度 | Antrea | Calico | Cilium | Flannel |
|------|--------|--------|--------|----------|
| 数据平面 | OVS | BGP/VXLAN | eBPF | VXLAN |
| NetworkPolicy | ✅ + ACNP | ✅ | ✅ + CiliumNP | ❌ |
| Egress 网关 | ✅ | ❌ | ✅ | ❌ |
| 可观测性 | Traceflow/IPFIX | 基础 | Hubble | 无 |
| 适用场景 | 企业高级网络 | 高性能路由 | eBPF 全功能 | 简单场景 |

## 架构定位

在 CNCF 生态中，Antrea 属于 **Networking** 类别，是 OVS-based CNI 的代表性项目。它在 NetworkPolicy 和流量可观测性方面提供了超越标准 K8s 的能力。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]]
- [[deployment]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]

## Related

- [[kgateway]] — kgateway
- [[urunc]] — urunc
- [[connect-rpc]] — Connect RPC
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- antrea
- [[23-实体/15-参考与索引/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/network-index.md|Network 网络知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
