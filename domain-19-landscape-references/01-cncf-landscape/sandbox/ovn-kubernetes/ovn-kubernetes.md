---
title: OVN-Kubernetes
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- cilium
- flannel
- calico
- helm
- ingress
- networkpolicy
- ebpf
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- OVN-Kubernetes 是什么
- 如何 OVN-Kubernetes
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- OVN-Kubernetes
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

title: OVN-Kubernetes
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- cilium
- flannel
- calico
- helm
- ingress
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
- OVN-Kubernetes 是什么
- 如何 OVN-Kubernetes
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- OVN-Kubernetes
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# OVN-Kubernetes

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://ovn-kubernetes.io/ |
| **GitHub** | https://github.com/ovn-org/ovn-kubernetes |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

OVN-Kubernetes 是一个基于 OVN (Open Virtual Network) 的 Kubernetes CNI 网络插件，提供企业级的虚拟网络功能。它利用 OVN 的分布式虚拟路由、负载均衡、ACL 和 NAT 能力，为 Kubernetes 提供高性能、可扩展的 L2/L3/L4 网络。OVN-Kubernetes 是 OpenShift 的默认网络插件，已在大规模生产环境中验证。

### 核心特性

- **分布式虚拟路由**: OVN 逻辑路由器实现跨节点的 Pod 三层通信
- **Network Policy**: 基于 OVN ACL 的高性能 Network Policy 实现
- **Egress IP**: Pod 出向流量使用指定的出口 IP，便于防火墙策略
- **EgressFirewall**: 控制 Pod 访问外部网络的目的地址
- **多网络支持**: 支持多网卡 Pod (Multus) 和多租户隔离
- **硬件卸载**: 支持 SR-IOV 和 OVS 硬件卸载，提升网络性能

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                   Kubernetes Cluster                   │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │            OVN-Kubernetes Control Plane        │    │
│  │  ┌───────────────┐  ┌─────────────────────┐  │    │
│  │  │  ovnkube-     │  │  ovnkube-           │  │    │
│  │  │  master       │  │  cluster-manager    │  │    │
│  │  │  (HA)         │  │                     │  │    │
│  │  └───────┬───────┘  └──────────┬──────────┘  │    │
│  └──────────┼─────────────────────┼─────────────┘    │
│             │                     │                   │
│  ┌──────────▼─────────────────────▼─────────────┐    │
│  │              OVN Control Plane                 │    │
│  │  ┌──────────┐  ┌──────────┐  ┌────────────┐  │    │
│  │  │ ovn-     │  │ ovn-     │  │ OVN        │  │    │
│  │  │ northd   │  │ sbdb     │  │ nbdb       │  │    │
│  │  │ (Raft)   │  │ (Raft)   │  │ (Raft)     │  │    │
│  │  └──────────┘  └──────────┘  └────────────┘  │    │
│  └────────────────────┬─────────────────────────┘    │
│                       │                               │
│     ┌─────────────────┼─────────────────┐            │
│     │                 │                 │            │
│  ┌──▼───────┐   ┌─────▼─────┐   ┌──────▼─────┐     │
│  │  Node 1   │   │  Node 2   │   │  Node 3    │     │
│  │┌─────────┐│   │┌─────────┐│   │┌─────────┐│     │
│  ││ovnkube- ││   ││ovnkube- ││   ││ovnkube- ││     │
│  ││node     ││   ││node     ││   ││node     ││     │
│  │└────┬────┘│   │└────┬────┘│   │└────┬────┘│     │
│  │┌────▼────┐│   │┌────▼────┐│   │┌────▼────┐│     │
│  ││ovs-     ││   ││ovs-     ││   ││ovs-     ││     │
│  ││vswitchd ││   ││vswitchd ││   ││vswitchd ││     │
│  │└─────────┘│   │└─────────┘│   │└─────────┘│     │
│  └───────────┘   └───────────┘   └───────────┘     │
└──────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 OVN-Kubernetes

```bash
# 使用 Helm 安装
helm repo add ovn-kubernetes https://ovn-org.github.io/ovn-kubernetes/helm-charts
helm install ovn-kubernetes ovn-kubernetes/ovn-kubernetes \
  --namespace ovn-kubernetes \
  --create-namespace \
  --set global.clusterCIDR=10.244.0.0/16 \
  --set global.serviceCIDR=10.96.0.0/12
```

### 使用 KIND 测试

```bash
# 使用 OVN-Kubernetes 的 KIND 配置
git clone https://github.com/ovn-org/ovn-kubernetes.git
cd ovn-kubernetes/contrib

# 创建集群
./kind.sh

# 验证
kubectl get pods -n ovn-kubernetes
```

### 配置 Network Policy

```yaml
# network-policy.yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-web-traffic
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: web
  policyTypes:
    - Ingress
    - Egress
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              env: production
        - podSelector:
            matchLabels:
              role: frontend
      ports:
        - protocol: TCP
          port: 8080
  egress:
    - to:
        - podSelector:
            matchLabels:
              role: database
      ports:
        - protocol: TCP
          port: 5432
```

---

## 高级功能

### Egress IP

```yaml
# 为 Namespace 配置 Egress IP
apiVersion: k8s.ovn.org/v1
kind: EgressIP
metadata:
  name: egressip-prod
spec:
  egressIPs:
    - 192.168.1.100
    - 192.168.1.101
  namespaceSelector:
    matchLabels:
      env: production
  podSelector:
    matchLabels:
      egress: external
```

### Egress Firewall

```yaml
# 限制 Namespace 只能访问特定外部地址
apiVersion: k8s.ovn.org/v1
kind: EgressFirewall
metadata:
  name: default
  namespace: production
spec:
  egress:
    - type: Allow
      to:
        cidrSelector: 10.0.0.0/8  # 内部网络
    - type: Allow
      to:
        dnsName: "*.example.com"  # 允许的域名
    - type: Deny
      to:
        cidrSelector: 0.0.0.0/0  # 拒绝其他所有
```

### 多网络 (Multus)

```yaml
# 定义附加网络
apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: storage-network
  namespace: default
spec:
  config: '{
    "cniVersion": "0.3.1",
    "type": "ovn-k8s-cni-overlay",
    "topology": "layer2",
    "netAttachDefName": "default/storage-network",
    "vlanID": 100,
    "subnets": "192.168.100.0/24"
  }'
---
# Pod 使用多网络
apiVersion: v1
kind: Pod
metadata:
  name: multi-network-pod
  annotations:
    k8s.v1.cni.cncf.io/networks: storage-network
spec:
  containers:
    - name: app
      image: nginx
```

### 硬件卸载 (SR-IOV)

```yaml
# 启用 SR-IOV 硬件卸载
apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: sriov-net
spec:
  config: '{
    "type": "ovn-k8s-cni-overlay",
    "topology": "layer2",
    "netAttachDefName": "default/sriov-net",
    "deviceType": "vfio-pci"
  }'
```

---

## 与其他方案对比

| 特性 | OVN-Kubernetes | Calico | Cilium | Flannel |
|:---|:---|:---|:---|:---|
| 数据面 | OVS | eBPF/iptables | eBPF | VXLAN |
| Network Policy | OVN ACL | 原生 | 原生 | 不支持 |
| Egress IP | 原生支持 | 企业版 | 支持 | 不支持 |
| 多网络 | Multus 集成 | 支持 | 支持 | 不支持 |
| 硬件卸载 | SR-IOV/OVS | 有限 | 支持 | 不支持 |
| 生产验证 | OpenShift | 广泛 | 广泛 | 广泛 |

---

## 最佳实践

1. **高可用部署**: OVN 数据库 (NBDB/SBDB) 使用 Raft 集群，至少 3 节点
2. **Network Policy**: 使用 OVN ACL 实现高性能策略，避免 iptables 规则膨胀
3. **Egress 管理**: 使用 Egress IP 和 EgressFirewall 控制出向流量
4. **监控**: 监控 OVN 数据库大小和 OVS 流表规模
5. **硬件卸载**: 高吞吐场景启用 SR-IOV 或 OVS 硬件卸载

---

## 参考资源

- [OVN-Kubernetes 文档](https://ovn-kubernetes.io/docs/)
- [OVN-Kubernetes GitHub](https://github.com/ovn-org/ovn-kubernetes)
- [OVN 项目](https://www.ovn.org/)
- [Open vSwitch](https://www.openvswitch.org/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/ovn-kubernetes.md|OVN-Kubernetes]]
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
