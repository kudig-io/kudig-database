---
title: CCE网络模型与VPC路由深度解析
description: 'CCE容器网络架构：VPC网络模型、VXLAN隧道、Cloud Native 2.0独占弹性网卡及Service网格集成'
summary: 'CCE容器网络架构：VPC网络模型、VXLAN隧道、Cloud Native 2.0独占弹性网卡及Service网格集成'
category: cloud-providers
tags:
- cloud
- k8s
- huawei-cce
- networking
- vpc
- vxlan
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- CCE网络模型 是什么
- 如何配置CCE VPC网络
- CCE Cloud Native 2.0网络是什么
trigger_keywords:
- CCE
- VPC
- VXLAN
- Cloud Native 2.0
- 弹性网卡
- 容器隧道网络
prerequisites:
- kubectl-basics
- cloud-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# CCE网络模型与VPC路由深度解析

## 1. CCE网络架构总览

CCE 提供三种容器网络模型，覆盖从简单测试到高性能生产的全场景需求：

| 网络模型 | 封装方式 | 性能 | 适用场景 |
|---------|---------|------|---------|
| 容器隧道网络 | VXLAN | 中 | 通用场景，快速部署 |
| VPC 网络 | VPC 路由 | 高 | 需要与 VPC 原生互通 |
| Cloud Native 2.0 | 独占弹性网卡 | 极高 | 低延迟、高吞吐、大规模集群 |

选择网络模型在创建集群时确定，**集群创建后不可更改**。

### 1.1 IP 地址规划原则

```
VPC CIDR:       10.0.0.0/16          # VPC 总网段
├── 子网 A:     10.0.1.0/24          # 节点子网 (254 可用 IP)
├── 子网 B:     10.0.2.0/24          # 节点子网
└── 容器 CIDR:  172.16.0.0/16        # 容器网段 (与 VPC 不重叠)
    ├── Pod-A:  172.16.0.0/18        # Node 1 Pod 段
    └── Pod-B:  172.16.64.0/18       # Node 2 Pod 段

Service CIDR:   192.168.0.0/16       # Service 网段 (与 VPC/容器不重叠)
```

**关键约束**：
- 容器 CIDR 与 VPC CIDR、Service CIDR 不能重叠
- 容器 CIDR 掩码范围 `/12` ~ `/24`，推荐 `/16`
- Service CIDR 掩码范围 `/16` ~ `/28`
- 每个节点默认分配 `/24` 的容器子网（254 个 Pod IP）

## 2. 容器隧道网络 (VXLAN)

### 2.1 工作原理

容器隧道网络基于 VXLAN 协议，在节点之间构建 overlay 隧道：

```
┌─────────────────┐         VXLAN Tunnel         ┌─────────────────┐
│   Node A        │◄────────────────────────────►│   Node B        │
│  eth0:10.0.1.5  │     UDP:4789 封装            │  eth0:10.0.2.5  │
│                 │                               │                 │
│  cbr0:172.16.0.1│     Pod A ←── VXLAN ──► Pod B│  cbr0:172.16.1.1│
│  Pod:172.16.0.5 │     (内层: 容器IP)           │  Pod:172.16.1.5 │
│                 │     (外层: VPC IP)           │                 │
└─────────────────┘                               └─────────────────┘
```

**数据路径**：
1. Pod A (172.16.0.5) 发包给 Pod B (172.16.1.5)
2. 包到达 cbr0 网桥，路由匹配到 VXLAN 设备
3. VXLAN 设备封装：外层源/目的为节点 VPC IP，UDP 目标端口 4789
4. 经 VPC 路由送达 Node B
5. Node B 解封装，投递到目标 Pod

### 2.2 隧道网络配置

```bash
# 查看节点 VXLAN 设备
kubectl get node <node-name> -o jsonpath='{.status.addresses}'

# 查看节点的 Pod CIDR 分配
kubectl get node <node-name> -o jsonpath='{.spec.podCIDR}'

# 查看 VXLAN 转发表 (节点上执行)
bridge fdb show dev vxlan0
```

### 2.3 性能优化

VXLAN 封装带来约 50 字节开销和一定的 CPU 消耗。优化手段：

```yaml
# 开启 VXLAN checksum offload (节点上执行)
ethtool -K vxlan0 tx-checksum-ipv4 on
ethtool -K vxlan0 rx-checksum-ipv4 on

# 开启 GRO/GSO (节点上执行)
ethtool -K eth0 gro on gso on tso on
```

**MTU 设置**：VXLAN 封装需要 50 字节开销，建议将节点网卡 MTU 设为 1550，或在集群配置中设置 Pod MTU 为 1450。

## 3. VPC 网络模型

### 3.1 工作原理

VPC 网络模型直接使用 VPC 路由表转发容器流量，无封装开销：

```
┌─────────────────┐       VPC Route Table        ┌─────────────────┐
│   Node A        │◄────────────────────────────►│   Node B        │
│  eth0:10.0.1.5  │    路由: 172.16.1.0/24       │  eth0:10.0.2.5  │
│                 │    → 下一跳: 10.0.2.5        │                 │
│  eth1:172.16.0.1│                               │  eth1:172.16.1.1│
│  Pod:172.16.0.5 │    直接路由, 无封装          │  Pod:172.16.1.5 │
└─────────────────┘                               └─────────────────┘
```

### 3.2 路由表管理

CCE 自动管理 VPC 路由表，每个节点加入集群时自动添加路由：

```bash
# 查看 VPC 路由表 (通过控制台或 API)
# 路由条目示例:
# 目的: 172.16.0.0/24  下一跳: 10.0.1.5 (Node A)
# 目的: 172.16.1.0/24  下一跳: 10.0.2.5 (Node B)

# 查看节点路由 (节点上执行)
ip route show | grep 172.16
```

### 3.3 限制与注意事项

- VPC 路由表条目有配额限制（默认 200 条），大规模集群需申请扩容
- 每个节点消耗一条路由表条目，因此集群最大节点数受限于路由表配额
- 与 VPC 内其他资源天然互通，无需额外配置
- 不支持 Pod 级别的网络策略（使用 VPC 安全组替代）

## 4. Cloud Native 2.0 网络 (独占弹性网卡)

### 4.1 架构原理

Cloud Native 2.0 是 CCE 最高性能的网络模型，为每个 Pod 分配独立的弹性网卡 (ENI)：

```
┌───────────────────────────────────────────────────┐
│                    Node                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐        │
│  │  Pod A   │  │  Pod B   │  │  Pod C   │        │
│  │ eth0:ENI │  │ eth0:ENI │  │ eth0:ENI │        │
│  │10.0.1.10 │  │10.0.1.11 │  │10.0.1.12 │        │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘        │
│       │              │              │              │
│  ┌────▼──────────────▼──────────────▼────┐        │
│  │          弹性网卡 (ENI) 池             │        │
│  │  独占模式: 每 Pod 一张 ENI             │        │
│  └───────────────────────────────────────┘        │
│       │              │              │              │
└───────┼──────────────┼──────────────┼──────────────┘
        │              │              │
   ┌────▼──────────────▼──────────────▼────┐
   │              VPC 网络                   │
   │   Pod IP 直接暴露在 VPC 中             │
   └───────────────────────────────────────┘
```

### 4.2 核心优势

| 特性 | 说明 |
|------|------|
| 零封装 | 无 VXLAN/GRE 开销，原生 VPC 性能 |
| 独立网卡 | 每个 Pod 拥有独立 ENI，网络隔离性最强 |
| 安全组 | 支持 Pod 级别安全组策略 |
| 带宽 | 独享带宽，不受节点网卡共享影响 |
| 延迟 | 与 ECS 实例一致的网络延迟 |

### 4.3 ENI 池管理

```bash
# 查看节点的 ENI 池状态
kubectl describe node <node-name> | grep -A 5 "Capacity"

# 查看 Pod 的弹性网卡
kubectl get pod <pod-name> -o jsonpath='{.metadata.annotations}'

# 关键 annotations:
# k8s.v1.cni.cncf.io/networks: 所属网络
# everest.io/elastic-network-interfaces: ENI 列表
```

### 4.4 节点规格与 ENI 配额

不同 ECS 规格支持的 ENI 数量不同，直接影响单节点 Pod 容量：

```
规格示例:
  c6.large.2    → 2 ENI  → 1 个基础设施 ENI + 1 个 Pod ENI
  c6.xlarge.2   → 3 ENI  → 2 个 Pod ENI (约 2 个独占 Pod)
  c6.2xlarge.2  → 4 ENI  → 3 个 Pod ENI
  c6.4xlarge.2  → 8 ENI  → 7 个 Pod ENI
  c6.16xlarge.2 → 36 ENI → 35 个 Pod ENI
```

**规划建议**：Cloud Native 2.0 模式下，单节点 Pod 数受 ENI 配额限制。大规模部署需选择高 ENI 配额的规格，或配合弹性伸缩。

### 4.5 创建集群时启用

```bash
# 通过 CCE 控制台或 API 创建集群时选择:
# 网络模型: Cloud Native 2.0 - 增强型 (独占弹性网卡)
# 容器 CIDR: 172.16.0.0/16
# Service CIDR: 192.168.0.0/16

# 通过 kubectl 验证网络模式
kubectl get configmap -n kube-system eni-config -o yaml
```

## 5. Service 网络与负载均衡

### 5.1 ClusterIP Service

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-service
  namespace: default
spec:
  type: ClusterIP
  selector:
    app: my-app
  ports:
    - port: 80
      targetPort: 8080
```

CCE 使用 kube-proxy (iptables 或 IPVS 模式) 实现 ClusterIP 负载均衡。

### 5.2 LoadBalancer Service (ELB 集成)

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-lb-service
  annotations:
    # 指定 ELB 实例 (可选，不指定则自动创建)
    kubernetes.io/elb.id: "<elb-id>"
    # 负载均衡器类型: 公网 / 内网
    kubernetes.io/elb.class: "performance"
    # 后端协议
    kubernetes.io/elb.protocol: "tcp"
spec:
  type: LoadBalancer
  selector:
    app: my-app
  ports:
    - port: 80
      targetPort: 8080
      protocol: TCP
```

### 5.3 CCE Ingress (ELB + Nginx)

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: my-ingress
  annotations:
    kubernetes.io/elb.id: "<elb-id>"
    kubernetes.io/ingress.class: "cce"
spec:
  rules:
    - host: app.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: my-service
                port:
                  number: 80
```

## 6. 网络策略

### 6.1 基于 NetworkPolicy 的访问控制

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: deny-all-ingress
  namespace: production
spec:
  podSelector: {}
  policyTypes:
    - Ingress
---
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
        - port: 8080
```

### 6.2 VPC 安全组 (Cloud Native 2.0)

Cloud Native 2.0 模式下可对每个 Pod 绑定安全组：

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: secured-pod
  annotations:
    # 指定 Pod 安全组
    k8s.v1.cni.cncf.io/networks: |
      [{
        "name": "security-group",
        "interface": "eth0",
        "securityGroups": ["sg-xxxxxxxx"]
      }]
spec:
  containers:
    - name: app
      image: nginx:1.25
```

## 7. DNS 解析

CCE 使用 CoreDNS 提供集群内 DNS 解析：

```bash
# 查看 CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml

# CoreDNS 核心配置
# .:53 {
#     forward . /etc/resolv.conf
#     kubernetes cluster.local in-addr.arpa ip6.arpa
# }
```

### 7.1 DNS 调优

```yaml
# 大规模集群 CoreDNS 扩容
apiVersion: apps/v1
kind: Deployment
metadata:
  name: coredns
  namespace: kube-system
spec:
  replicas: 3  # 根据节点数调整: <100 节点 2 副本, >500 节点 5 副本
  template:
    spec:
      containers:
        - name: coredns
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 512Mi
```

## 8. 故障排查

### 8.1 跨节点 Pod 不通

```bash
# 1. 检查节点间连通性
ping <node-b-ip>

# 2. 检查 VXLAN 隧道 (容器隧道网络)
tcpdump -i eth0 udp port 4789 -nn

# 3. 检查路由表 (VPC 网络)
# 控制台查看 VPC 路由表是否有对应路由条目

# 4. 检查安全组/ACL 规则
# 确认安全组放行容器 CIDR 之间的流量

# 5. 检查 kube-proxy 规则
iptables -t nat -L KUBE-SERVICES | grep <service-name>
```

### 8.2 Pod 无法访问外部网络

```bash
# 1. 检查 SNAT 规则
iptables -t nat -L POSTROUTING | grep MASQUERADE

# 2. 检查 CoreDNS
kubectl logs -n kube-system -l k8s-app=kube-dns

# 3. 检查节点 NAT 网关
# 确认 VPC NAT 网关配置正确，且路由表指向 NAT 网关
```

## 9. 最佳实践

1. **CIDR 规划**：生产环境使用 `/16` 容器 CIDR，预留足够扩展空间
2. **网络模型选择**：高吞吐低延迟场景优先 Cloud Native 2.0
3. **MTU 配置**：VXLAN 场景设置 Pod MTU 为 1450
4. **CoreDNS 高可用**：至少 2 副本，配置 Pod 反亲和避免单点
5. **NetworkPolicy**：生产环境必须启用默认拒绝策略
6. **监控**：部署 CCE 网络监控插件，关注丢包率和延迟指标

---

*本文档描述 CCE 容器网络的架构、配置与运维。具体参数以华为云官方文档为准。*
