---
title: kube-vip
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- docker
- daemonset
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- kube-vip 是什么
- 如何 kube-vip
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- kube-vip
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

title: kube-vip
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- docker
- daemonset
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- kube-vip 是什么
- 如何 kube-vip
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- kube-vip
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

# kube-vip

> **成熟度**: Sandbox | **加入时间**: 2022-09 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://kube-vip.io |
| **GitHub** | https://github.com/kube-vip/kube-vip |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Networking |
| **适用场景** | Kubernetes VIP 和负载均衡 |

---

## 项目概述

kube-vip 为 Kubernetes 集群提供虚拟 IP (VIP) 和负载均衡功能。它可以作为控制平面的高可用解决方案，提供浮动 VIP 确保 API Server 始终可访问。同时也可以作为 LoadBalancer 类型 Service 的实现，为裸金属环境提供服务负载均衡。

---

## 核心特性

- **控制平面 HA**: 为 Kubernetes API Server 提供 VIP
- **Service LoadBalancer**: 裸金属 LoadBalancer 实现
- **ARP/BGP**: 支持 Layer 2 (ARP) 和 Layer 3 (BGP) 模式
- **Leader 选举**: 基于 Raft 或 Kubernetes Lease 的选举
- **轻量级**: 单一二进制，无外部依赖
- **IPv4/IPv6**: 双栈支持

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     kube-vip Architecture                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                Control Plane VIP Mode                     │   │
│  │                                                           │   │
│  │  Clients ──────────► VIP: 192.168.1.100:6443             │   │
│  │                          │                                │   │
│  │    ┌─────────────────────┼─────────────────────┐         │   │
│  │    │                     │                     │         │   │
│  │    ▼                     ▼                     ▼         │   │
│  │  ┌──────────┐   ┌──────────┐   ┌──────────┐            │   │
│  │  │ Master 1 │   │ Master 2 │   │ Master 3 │            │   │
│  │  │ kube-vip │   │ kube-vip │   │ kube-vip │            │   │
│  │  │ [Leader] │   │[Follower]│   │[Follower]│            │   │
│  │  │ VIP ✓    │   │          │   │          │            │   │
│  │  └──────────┘   └──────────┘   └──────────┘            │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │              Service LoadBalancer Mode                     │   │
│  │                                                           │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │  kube-vip Cloud Provider (Deployment)                │ │   │
│  │  │  Watches Service objects, assigns IPs from pool      │ │   │
│  │  └────────────────────────┬────────────────────────────┘ │   │
│  │                           │                               │   │
│  │  ┌────────────────────────▼────────────────────────────┐ │   │
│  │  │  kube-vip DaemonSet (on each node)                  │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │   Node 1    │  │   Node 2    │  │   Node 3   │  │ │   │
│  │  │  │ VIP: .200   │  │             │  │ VIP: .201  │  │ │   │
│  │  │  │ (Svc A)     │  │             │  │ (Svc B)    │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

---

## 控制平面 VIP

### 静态 Pod 配置

```yaml
# /etc/kubernetes/manifests/kube-vip.yaml
apiVersion: v1
kind: Pod
metadata:
  name: kube-vip
  namespace: kube-system
spec:
  containers:
    - name: kube-vip
      image: ghcr.io/kube-vip/kube-vip:latest
      args:
        - manager
      env:
        - name: vip_arp
          value: "true"
        - name: port
          value: "6443"
        - name: vip_interface
          value: eth0
        - name: vip_cidr
          value: "32"
        - name: cp_enable
          value: "true"
        - name: cp_namespace
          value: kube-system
        - name: vip_leaderelection
          value: "true"
        - name: vip_leasename
          value: plndr-cp-lock
        - name: vip_leaseduration
          value: "5"
        - name: vip_renewdeadline
          value: "3"
        - name: vip_retryperiod
          value: "1"
        - name: address
          value: "192.168.1.100"
      securityContext:
        capabilities:
          add: ["NET_ADMIN", "NET_RAW"]
      volumeMounts:
        - name: kubernetes
          mountPath: /etc/kubernetes/admin.conf
  hostNetwork: true
  volumes:
    - name: kubernetes
      hostPath:
        path: /etc/kubernetes/admin.conf
```

### 使用 kube-vip CLI 生成配置

```bash
# 生成静态 Pod manifest
docker run --network host --rm ghcr.io/kube-vip/kube-vip:latest \
  manifest pod \
  --interface eth0 \
  --address 192.168.1.100 \
  --controlplane \
  --arp \
  --leaderElection | tee /etc/kubernetes/manifests/kube-vip.yaml
```

---

## Service LoadBalancer

### 安装 Cloud Provider

```bash
kubectl apply -f https://raw.githubusercontent.com/kube-vip/kube-vip-cloud-provider/main/manifest/kube-vip-cloud-controller.yaml
```

### 配置 IP 地址池

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: kubevip
  namespace: kube-system
data:
  # 全局 IP 范围
  range-global: 192.168.1.200-192.168.1.250
  
  # 命名空间特定范围
  range-production: 10.0.0.100-10.0.0.150
  range-staging: 10.0.1.100-10.0.1.150
  
  # CIDR 格式
  cidr-global: 192.168.2.0/24
```

### 安装 kube-vip DaemonSet

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: kube-vip-ds
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: kube-vip-ds
  template:
    metadata:
      labels:
        app: kube-vip-ds
    spec:
      containers:
        - name: kube-vip
          image: ghcr.io/kube-vip/kube-vip:latest
          args: ["manager"]
          env:
            - name: vip_arp
              value: "true"
            - name: svc_enable
              value: "true"
            - name: svc_election
              value: "true"
            - name: vip_interface
              value: eth0
          securityContext:
            capabilities:
              add: ["NET_ADMIN", "NET_RAW"]
      hostNetwork: true
      tolerations:
        - effect: NoSchedule
          operator: Exists
```

### 使用 LoadBalancer Service

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-app-lb
spec:
  type: LoadBalancer
  selector:
    app: my-app
  ports:
    - port: 80
      targetPort: 8080

# 指定 IP
---
apiVersion: v1
kind: Service
metadata:
  name: my-app-specific-ip
spec:
  type: LoadBalancer
  loadBalancerIP: 192.168.1.200
  selector:
    app: my-app
  ports:
    - port: 80
```

---

## BGP 模式

```yaml
env:
  - name: vip_arp
    value: "false"
  - name: bgp_enable
    value: "true"
  - name: bgp_routerid
    value: "192.168.1.10"
  - name: bgp_as
    value: "65000"
  - name: bgp_peeraddress
    value: "192.168.1.1"
  - name: bgp_peeras
    value: "65001"
  - name: bgp_peers
    value: "192.168.1.1:65001::false,192.168.1.2:65001::false"
```

---

## 最佳实践

1. **接口选择**: 指定正确的网络接口
2. **IP 规划**: 确保 VIP 不与 DHCP 范围冲突
3. **HA 模式**: 控制平面至少 3 个节点
4. **Lease 调优**: 根据网络质量调整选举参数
5. **BGP 场景**: 大规模集群推荐 BGP 模式
6. **监控**: 监控 VIP 漂移和选举事件

---

## 参考资源

- [官方文档](https://kube-vip.io)
- [GitHub Repo](https://github.com/kube-vip/kube-vip)
- [控制平面 HA](https://kube-vip.io/docs/installation/static/)
- [Service LB](https://kube-vip.io/docs/usage/kubernetes-services/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[man/INSTALL.md|INSTALL]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/networking.md|networking]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[references/k8s-cluster-delete|Kubernetes 集群删除操作指南]] — Cross-reference
- [[skills/kubeadm-ha-cluster-setup|kubeadm 高可用集群搭建]] — Cross-reference
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
