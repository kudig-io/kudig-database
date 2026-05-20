---
title: Antrea
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- helm
- daemonset
- ingress
- gateway
- networkpolicy
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Antrea 是什么
- 如何 Antrea
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Antrea
- cncf
- landscape
---


# Antrea

> **成熟度**: Sandbox | **加入时间**: 2020-04 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://antrea.io |
| **GitHub** | https://github.com/antrea-io/antrea |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Networking (CNI) |
| **维护组织** | VMware (Broadcom) |

---

## 项目概述

Antrea 是基于 Open vSwitch (OVS) 构建的 Kubernetes 网络解决方案，为 Pod 网络提供高性能数据平面。它实现了 Kubernetes NetworkPolicy API，并扩展支持更细粒度的流量控制，包括 ClusterNetworkPolicy、Egress 和流量可观测性功能。

---

## 核心特性

- **高性能网络**: 基于 OVS 的优化数据路径
- **NetworkPolicy**: 完整支持 K8s NetworkPolicy + 扩展策略
- **多集群支持**: 跨集群 Pod 网络互通
- **流量可观测性**: Flow Exporter、IPFIX、Packet Tracing
- **Egress 网关**: 集中管理出站流量
- **二层网络**: 支持 VLAN、Trunk 网络
- **Windows 支持**: Windows 节点原生支持
- **加密隧道**: IPsec/WireGuard 加密

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     Antrea Architecture                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                 Antrea Controller                         │   │
│  │  (Deployment in kube-system)                              │   │
│  │  ┌─────────────────┐  ┌─────────────────────────────┐   │   │
│  │  │ NetworkPolicy   │  │   ClusterNetworkPolicy      │   │   │
│  │  │   Controller    │  │       Controller            │   │   │
│  │  └─────────────────┘  └─────────────────────────────┘   │   │
│  │  ┌─────────────────┐  ┌─────────────────────────────┐   │   │
│  │  │    Egress       │  │      Multi-cluster          │   │   │
│  │  │   Controller    │  │       Controller            │   │   │
│  │  └─────────────────┘  └─────────────────────────────┘   │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │                    Per-Node Components                    │   │
│  │                                                           │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │              Antrea Agent (DaemonSet)                │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │ CNI Plugin  │  │   OVS       │  │   Node     │  │ │   │
│  │  │  │  Interface  │  │  Manager    │  │  Port Sync │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘  │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │ NetworkPolicy│  │   Proxy     │  │   Flow     │  │ │   │
│  │  │  │    Agent    │  │   Agent     │  │  Exporter  │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  │                                                           │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │              Open vSwitch (OVS)                      │ │   │
│  │  │  ┌───────────────────────────────────────────────┐  │ │   │
│  │  │  │              OVS Bridge (br-int)               │  │ │   │
│  │  │  │  ┌─────────┐  ┌─────────┐  ┌─────────────┐   │  │ │   │
│  │  │  │  │ Pod A   │  │ Pod B   │  │  Tunnel     │   │  │ │   │
│  │  │  │  │ Port    │  │ Port    │  │  Port       │   │  │ │   │
│  │  │  │  └────┬────┘  └────┬────┘  └──────┬──────┘   │  │ │   │
│  │  │  │       │            │              │          │  │ │   │
│  │  │  │  ┌────▼────────────▼──────────────▼──────┐   │  │ │   │
│  │  │  │  │           OVS Flow Tables             │   │  │ │   │
│  │  │  │  │  ┌─────────┐ ┌─────────┐ ┌─────────┐  │   │  │ │   │
│  │  │  │  │  │Classifier│ │ L2/L3   │ │  ACL    │  │   │  │ │   │
│  │  │  │  │  │  Table   │ │Forwarding│ │ Table   │  │   │  │ │   │
│  │  │  │  │  └─────────┘ └─────────┘ └─────────┘  │   │  │ │   │
│  │  │  │  └───────────────────────────────────────┘   │  │ │   │
│  │  │  └───────────────────────────────────────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  │                                                           │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │                     Pods                             │ │   │
│  │  │  ┌─────────┐  ┌─────────┐  ┌─────────┐              │ │   │
│  │  │  │ Pod A   │  │ Pod B   │  │ Pod C   │              │ │   │
│  │  │  │ eth0    │  │ eth0    │  │ eth0    │              │ │   │
│  │  │  └─────────┘  └─────────┘  └─────────┘              │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              Node-to-Node Tunnel (Geneve/VXLAN)          │   │
│  │   ┌────────┐                              ┌────────┐     │   │
│  │   │ Node 1 │◄────────── Tunnel ──────────►│ Node 2 │     │   │
│  │   └────────┘       (Encapsulated)         └────────┘     │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **Antrea Controller** | 控制器，管理策略和集群状态 |
| **Antrea Agent** | 节点代理，配置 OVS 规则 |
| **OVS Bridge** | 虚拟交换机，处理数据包转发 |
| **Flow Tables** | OVS 流表，实现网络策略 |
| **CNI Plugin** | CNI 接口，为 Pod 配置网络 |

---

## 快速开始

### 安装 Antrea

```bash
# 下载并应用 Antrea manifest
kubectl apply -f https://github.com/antrea-io/antrea/releases/latest/download/antrea.yml

# 验证安装
kubectl get pods -n kube-system -l app=antrea
kubectl get daemonset antrea-agent -n kube-system

# 检查 Antrea 状态
kubectl exec -n kube-system antrea-controller-xxx -- antctl get agentinfo
```

### Helm 安装

```bash
helm repo add antrea https://charts.antrea.io
helm repo update

helm install antrea antrea/antrea \
  --namespace kube-system \
  --set trafficEncapMode=encap \
  --set tunnelType=geneve
```

---

## 网络模式配置

### Encap 模式 (默认)

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: antrea-config
  namespace: kube-system
data:
  antrea-agent.conf: |
    trafficEncapMode: encap
    tunnelType: geneve  # geneve, vxlan, gre, stt
    enableIPSecTunnel: false
```

### NoEncap 模式

```yaml
# 适用于支持 Pod CIDR 路由的网络
apiVersion: v1
kind: ConfigMap
metadata:
  name: antrea-config
  namespace: kube-system
data:
  antrea-agent.conf: |
    trafficEncapMode: noEncap
    noSNAT: true
```

### IPsec 加密

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: antrea-config
  namespace: kube-system
data:
  antrea-agent.conf: |
    trafficEncapMode: encap
    enableIPSecTunnel: true
    # WireGuard 替代方案
    # trafficEncryptionMode: wireGuard
```

---

## NetworkPolicy

### 基础 NetworkPolicy

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes:
    - Ingress
  ingress:
    - from:
        - podSelector:
            matchLabels:
              app: frontend
      ports:
        - protocol: TCP
          port: 8080
```

### ClusterNetworkPolicy (Antrea 扩展)

```yaml
apiVersion: crd.antrea.io/v1beta1
kind: ClusterNetworkPolicy
metadata:
  name: strict-db-access
spec:
  priority: 10
  tier: SecurityOps
  appliedTo:
    - podSelector:
        matchLabels:
          app: database
  ingress:
    - action: Allow
      from:
        - podSelector:
            matchLabels:
              role: backend
      ports:
        - protocol: TCP
          port: 5432
      name: allow-backend
    - action: Drop
      name: deny-all-others

---
# 自定义 Tier
apiVersion: crd.antrea.io/v1beta1
kind: Tier
metadata:
  name: SecurityOps
spec:
  priority: 50
  description: "Security team policies"
```

### Egress 控制

```yaml
apiVersion: crd.antrea.io/v1beta1
kind: ClusterNetworkPolicy
metadata:
  name: restrict-egress
spec:
  priority: 5
  appliedTo:
    - namespaceSelector:
        matchLabels:
          environment: production
  egress:
    - action: Allow
      to:
        - fqdn: api.example.com
      ports:
        - protocol: TCP
          port: 443
    - action: Allow
      to:
        - ipBlock:
            cidr: 10.0.0.0/8
    - action: Drop
```

---

## Egress Gateway

### 配置 Egress 网关

```yaml
apiVersion: crd.antrea.io/v1beta1
kind: Egress
metadata:
  name: production-egress
spec:
  appliedTo:
    podSelector:
      matchLabels:
        app: backend
  egressIP: 192.168.100.10
  externalIPPool: production-pool

---
apiVersion: crd.antrea.io/v1beta1
kind: ExternalIPPool
metadata:
  name: production-pool
spec:
  ipRanges:
    - start: 192.168.100.10
      end: 192.168.100.20
  nodeSelector:
    matchLabels:
      node-role: egress-gateway
```

---

## 流量可观测性

### Flow Exporter 配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: antrea-config
  namespace: kube-system
data:
  antrea-agent.conf: |
    flowExporter:
      enable: true
      flowCollectorAddr: "flow-aggregator.flow-aggregator:4739:tls"
      flowPollInterval: 5s
      activeFlowExportTimeout: 30s
      idleFlowExportTimeout: 15s
```

### Flow Aggregator 部署

```bash
kubectl apply -f https://github.com/antrea-io/antrea/releases/latest/download/flow-aggregator.yml
```

### 流量跟踪 (Traceflow)

```yaml
apiVersion: crd.antrea.io/v1beta1
kind: Traceflow
metadata:
  name: trace-frontend-backend
spec:
  source:
    namespace: default
    pod: frontend-xxx
  destination:
    namespace: default
    pod: backend-yyy
  packet:
    ipHeader:
      protocol: 6  # TCP
    transportHeader:
      tcp:
        dstPort: 8080
```

```bash
# 使用 antctl 追踪
kubectl exec -n kube-system antrea-controller-xxx -- \
  antctl traceflow -S default/frontend -D default/backend -f tcp,tcp_dst=8080
```

---

## 多集群网络

### 配置多集群网关

```yaml
apiVersion: multicluster.crd.antrea.io/v1alpha1
kind: Gateway
metadata:
  name: cluster-a-gateway
  namespace: antrea-multicluster
spec:
  gatewayIP: 192.168.200.1
  internalIP: 10.96.0.100

---
apiVersion: multicluster.crd.antrea.io/v1alpha1
kind: ClusterSet
metadata:
  name: my-clusterset
  namespace: antrea-multicluster
spec:
  clusterID: cluster-a
  leaders:
    - clusterID: cluster-a
```

---

## antctl 命令

```bash
# 查看代理信息
antctl get agentinfo

# 查看 Pod 接口
antctl get podinterface

# 查看网络策略
antctl get networkpolicy
antctl get appliedtogroup
antctl get addressgroup

# 查看 OVS 流表
antctl get ovsflows

# 调试连接
antctl traceflow -S default/pod-a -D default/pod-b

# 查看流量统计
antctl get featuregatestatus
```

---

## 监控集成

### Prometheus 指标

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: antrea-controller
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: antrea
      component: antrea-controller
  endpoints:
    - port: api
      path: /metrics
```

### 关键指标

| 指标 | 说明 |
|:---|:---|
| `antrea_agent_ovs_flow_count` | OVS 流规则数量 |
| `antrea_agent_networkpolicy_count` | NetworkPolicy 数量 |
| `antrea_agent_pod_count` | 管理的 Pod 数量 |
| `antrea_proxy_sync_proxy_rules_duration` | 代理规则同步延迟 |

---

## 最佳实践

1. **隧道选择**: 一般使用 Geneve，性能敏感场景考虑 noEncap
2. **策略分层**: 使用 Tier 组织策略优先级
3. **流量加密**: 跨区域流量启用 IPsec 或 WireGuard
4. **可观测性**: 生产环境部署 Flow Aggregator
5. **Traceflow**: 使用 Traceflow 调试网络问题
6. **多集群**: 统一 Pod CIDR 规划避免冲突

---

## 参考资源

- [官方文档](https://antrea.io/docs)
- [GitHub Repo](https://github.com/antrea-io/antrea)
- [NetworkPolicy 指南](https://antrea.io/docs/main/docs/network-policy/)
- [多集群文档](https://antrea.io/docs/main/docs/multicluster/)
- [Traceflow 指南](https://antrea.io/docs/main/docs/traceflow/)

---

**维护者**: Kudig Team | **许可证**: MIT
