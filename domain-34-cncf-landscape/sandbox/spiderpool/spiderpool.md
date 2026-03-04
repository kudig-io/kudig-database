# Spiderpool

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **GitHub** | https://github.com/spidernet-io/spiderpool |
| **官网** | https://spidernet-io.github.io/spiderpool/ |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Networking / IPAM |
| **支持 CNI** | Macvlan / IPvlan / SR-IOV / Calico / Cilium |

---

## 项目概述

Spiderpool 是一个 Kubernetes 的 Underlay 网络 IPAM (IP Address Management) 解决方案，专为数据中心和云原生环境设计。它支持固定 IP、多网卡、双栈网络等高级特性，能够与多种 CNI 插件无缝集成，特别适合需要 Pod 与物理网络直接通信的场景。

### 核心价值

- **固定 IP 分配**: 支持 StatefulSet、Deployment 等工作负载的 IP 保持
- **多网卡管理**: 单 Pod 多网卡，支持不同子网
- **双栈网络**: 原生支持 IPv4/IPv6 双栈
- **Underlay 网络**: 与物理网络无缝集成
- **弹性扩展**: 支持大规模集群的 IP 管理

---

## 核心特性

### IP 分配模式

```
┌─────────────────────────────────────────────────────────────────┐
│                    Spiderpool IP 分配模式                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌───────────────────────────────────────────────────────────┐  │
│  │                    SpiderIPPool                            │  │
│  │                                                            │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │  名称: production-pool                               │  │  │
│  │  │  网段: 10.6.0.0/16                                   │  │  │
│  │  │  范围: 10.6.100.1-10.6.100.254                       │  │  │
│  │  │  网关: 10.6.0.1                                      │  │  │
│  │  │  VLAN: 100                                           │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  │                          │                                 │  │
│  │              ┌───────────┴───────────┐                    │  │
│  │              │                       │                     │  │
│  │              ▼                       ▼                     │  │
│  │  ┌──────────────────┐   ┌──────────────────┐              │  │
│  │  │    Auto Pool     │   │   Fixed Pool     │              │  │
│  │  │                  │   │                  │              │  │
│  │  │  动态分配 IP     │   │  固定 IP 绑定    │              │  │
│  │  │  Deployment 适用 │   │  StatefulSet 适用│              │  │
│  │  └──────────────────┘   └──────────────────┘              │  │
│  │                                                            │  │
│  └───────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 功能矩阵

| 特性 | 支持状态 | 描述 |
|:---|:---|:---|
| **固定 IP** | ✅ | Pod 重建后保持相同 IP |
| **多 IP Pool** | ✅ | 支持多个 IP 池管理 |
| **双栈网络** | ✅ | IPv4 + IPv6 同时支持 |
| **多网卡** | ✅ | Pod 可配置多张网卡 |
| **子网管理** | ✅ | SpiderSubnet 资源 |
| **路由管理** | ✅ | 自定义 Pod 路由表 |
| **命名空间隔离** | ✅ | 按命名空间分配 IP 池 |
| **亲和性调度** | ✅ | 按节点/可用区分配 |

---

## 架构设计

```
┌───────────────────────────────────────────────────────────────────┐
│                        Spiderpool Architecture                     │
├───────────────────────────────────────────────────────────────────┤
│                                                                    │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                     Kubernetes API Server                     │ │
│  └──────────────────────────────────────────────────────────────┘ │
│                              │                                     │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                   Spiderpool Components                       │ │
│  │                                                                │ │
│  │  ┌────────────────────┐      ┌────────────────────┐          │ │
│  │  │  Spiderpool Agent  │      │ Spiderpool Controller│         │ │
│  │  │   (DaemonSet)      │      │    (Deployment)     │          │ │
│  │  │                    │      │                     │          │ │
│  │  │  - IPAM Plugin     │      │  - IP Pool Mgmt    │          │ │
│  │  │  - CNI Interface   │      │  - Subnet Mgmt     │          │ │
│  │  │  - Route Config    │      │  - GC Controller   │          │ │
│  │  │  - Network Config  │      │  - Webhook Server  │          │ │
│  │  └─────────┬──────────┘      └──────────┬─────────┘          │ │
│  │            │                            │                     │ │
│  └────────────│────────────────────────────│─────────────────────┘ │
│               │                            │                       │
│               ▼                            ▼                       │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                      CRD Resources                            │ │
│  │                                                                │ │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐ │ │
│  │  │SpiderIPPool│ │SpiderSubnet│ │SpiderEndpt │ │SpiderCoord │ │ │
│  │  │            │ │            │ │            │ │ inator     │ │ │
│  │  └────────────┘ └────────────┘ └────────────┘ └────────────┘ │ │
│  └──────────────────────────────────────────────────────────────┘ │
│                              │                                     │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                    CNI Plugin Chain                           │ │
│  │                                                                │ │
│  │  ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐   │ │
│  │  │ Macvlan │    │ IPvlan  │    │  SR-IOV │    │ Multus  │   │ │
│  │  └─────────┘    └─────────┘    └─────────┘    └─────────┘   │ │
│  │                                                                │ │
│  └──────────────────────────────────────────────────────────────┘ │
│                              │                                     │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                    Physical Network                           │ │
│  │                                                                │ │
│  │        ┌───────────────────────────────────────────┐         │ │
│  │        │  VLAN 100: 10.6.0.0/16                    │         │ │
│  │        │  VLAN 200: 10.7.0.0/16                    │         │ │
│  │        │  Gateway: 10.6.0.1 / 10.7.0.1            │         │ │
│  │        └───────────────────────────────────────────┘         │ │
│  │                                                                │ │
│  └──────────────────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装部署

```bash
# 使用 Helm 安装
helm repo add spiderpool https://spidernet-io.github.io/spiderpool
helm repo update

# 安装 Spiderpool
helm install spiderpool spiderpool/spiderpool \
  --namespace kube-system \
  --set multus.multusCNI.install=true \
  --set coordinator.enabled=true

# 验证安装
kubectl get pods -n kube-system | grep spiderpool

# 输出:
# spiderpool-agent-xxxxx      1/1     Running
# spiderpool-controller-xxx   1/1     Running
```

### 创建 IP Pool

```yaml
# ippool.yaml
apiVersion: spiderpool.spidernet.io/v2beta1
kind: SpiderIPPool
metadata:
  name: production-v4-pool
spec:
  # IPv4 配置
  ipVersion: 4
  subnet: "10.6.0.0/16"
  ips:
    - "10.6.100.1-10.6.100.254"
  gateway: "10.6.0.1"
  vlan: 100
  
  # 默认路由
  routes:
    - dst: "0.0.0.0/0"
      gw: "10.6.0.1"
  
  # 命名空间绑定（可选）
  namespaceAffinity:
    matchLabels:
      environment: production

---
apiVersion: spiderpool.spidernet.io/v2beta1
kind: SpiderIPPool
metadata:
  name: production-v6-pool
spec:
  # IPv6 配置
  ipVersion: 6
  subnet: "fd00::/64"
  ips:
    - "fd00::100-fd00::1ff"
  gateway: "fd00::1"
  vlan: 100
```

```bash
kubectl apply -f ippool.yaml
kubectl get spiderippool
```

### 创建 Pod 使用 IP Pool

```yaml
# deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-underlay
spec:
  replicas: 3
  selector:
    matchLabels:
      app: nginx-underlay
  template:
    metadata:
      labels:
        app: nginx-underlay
      annotations:
        # 指定 IP 池
        ipam.spidernet.io/ippool: '{"ipv4": ["production-v4-pool"], "ipv6": ["production-v6-pool"]}'
    spec:
      containers:
        - name: nginx
          image: nginx:latest
          ports:
            - containerPort: 80
```

```bash
kubectl apply -f deployment.yaml

# 查看 IP 分配
kubectl get spiderendpoint
# NAME                          INTERFACE   IPV4            IPV6
# nginx-underlay-xxx-aaa        eth0        10.6.100.1      fd00::100
# nginx-underlay-xxx-bbb        eth0        10.6.100.2      fd00::101
# nginx-underlay-xxx-ccc        eth0        10.6.100.3      fd00::102
```

---

## 高级功能

### 固定 IP (StatefulSet)

```yaml
# statefulset-fixed-ip.yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-cluster
spec:
  serviceName: mysql
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
      annotations:
        # 启用固定 IP
        ipam.spidernet.io/ippool: '{"ipv4": ["database-pool"]}'
        ipam.spidernet.io/fixed-ip: "true"
    spec:
      containers:
        - name: mysql
          image: mysql:8.0
          env:
            - name: MYSQL_ROOT_PASSWORD
              value: "password"
```

### 子网自动管理 (SpiderSubnet)

```yaml
# spidersubnet.yaml
apiVersion: spiderpool.spidernet.io/v2beta1
kind: SpiderSubnet
metadata:
  name: production-subnet
spec:
  ipVersion: 4
  subnet: "10.6.0.0/16"
  ips:
    - "10.6.0.1-10.6.255.254"
  gateway: "10.6.0.1"
  vlan: 100
  
  # 自动创建 IP Pool
  autoPoolSpec:
    # IP Pool 名称前缀
    namePrefix: "auto-"
    # 每个 Pool 分配的 IP 数量
    ipNum: 10
```

```yaml
# 使用 SpiderSubnet
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  template:
    metadata:
      annotations:
        # 自动从 Subnet 获取 IP Pool
        ipam.spidernet.io/subnet: '{"ipv4": ["production-subnet"]}'
        ipam.spidernet.io/ippool-ip-number: "+5"  # 额外分配 5 个 IP
```

### 多网卡配置

```yaml
# multus-network.yaml
apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: macvlan-overlay
  namespace: default
spec:
  config: |
    {
      "cniVersion": "0.3.1",
      "name": "macvlan-overlay",
      "type": "macvlan",
      "master": "eth0",
      "mode": "bridge",
      "ipam": {
        "type": "spiderpool"
      }
    }

---
apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: ipvlan-management
  namespace: default
spec:
  config: |
    {
      "cniVersion": "0.3.1",
      "name": "ipvlan-management",
      "type": "ipvlan",
      "master": "eth1",
      "mode": "l2",
      "ipam": {
        "type": "spiderpool"
      }
    }
```

```yaml
# multi-nic-pod.yaml
apiVersion: v1
kind: Pod
metadata:
  name: multi-nic-app
  annotations:
    # 多网卡配置
    k8s.v1.cni.cncf.io/networks: |
      [
        {
          "name": "macvlan-overlay",
          "namespace": "default",
          "ipam": {
            "ipv4": ["overlay-pool"]
          }
        },
        {
          "name": "ipvlan-management",
          "namespace": "default",
          "ipam": {
            "ipv4": ["management-pool"]
          }
        }
      ]
spec:
  containers:
    - name: app
      image: nginx
```

### 路由协调 (Coordinator)

```yaml
# coordinator.yaml
apiVersion: spiderpool.spidernet.io/v2beta1
kind: SpiderCoordinator
metadata:
  name: default
spec:
  # 检测网关可达性
  mode: underlay
  
  # 多网卡路由协调
  tunePodRoutes: true
  
  # 主机路由配置
  hostRPFilter: 0
  hostRuleTable: 500
  
  # 检测配置
  detectGateway: true
  detectIPConflict: true
```

---

## 与 CNI 集成

### Macvlan 集成

```yaml
# macvlan-config.yaml
apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: macvlan-net
spec:
  config: |
    {
      "cniVersion": "0.3.1",
      "name": "macvlan-net",
      "type": "macvlan",
      "master": "eth0",
      "mode": "bridge",
      "ipam": {
        "type": "spiderpool",
        "default_ipv4_ippool": ["default-v4-pool"],
        "default_ipv6_ippool": ["default-v6-pool"]
      }
    }
```

### SR-IOV 集成

```yaml
# sriov-config.yaml
apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: sriov-net
spec:
  config: |
    {
      "cniVersion": "0.3.1",
      "name": "sriov-net",
      "type": "sriov",
      "vlan": 100,
      "ipam": {
        "type": "spiderpool",
        "default_ipv4_ippool": ["sriov-pool"]
      }
    }
```

### Calico/Cilium Overlay + Spiderpool Underlay

```yaml
# hybrid-network.yaml
apiVersion: v1
kind: Pod
metadata:
  name: hybrid-network-pod
  annotations:
    # Calico/Cilium 作为默认网络 (Overlay)
    # Spiderpool 提供额外 Underlay 网卡
    k8s.v1.cni.cncf.io/networks: macvlan-underlay
spec:
  containers:
    - name: app
      image: nginx
```

---

## 运维管理

### IP Pool 监控

```bash
# 查看 IP Pool 使用情况
kubectl get spiderippool -o wide

# NAME                  IPVERSION   SUBNET         TOTAL   ALLOCATED   AVAILABLE
# production-v4-pool    4           10.6.0.0/16    254     45          209
# production-v6-pool    6           fd00::/64      256     45          211

# 查看 IP 分配详情
kubectl get spiderendpoint -A

# 查看特定 Pod 的 IP
kubectl get spiderendpoint -n default nginx-xxx -o yaml
```

### IP 回收与 GC

```yaml
# 配置 GC 策略
# values.yaml (Helm)
gcConfig:
  # GC 扫描间隔
  gcScanInterval: "10m"
  
  # IP 释放延迟
  gcIPReleaseDelay: "5m"
  
  # 清理孤儿 IP
  gcOrphanedIPEnabled: true
```

### 故障排查

```bash
# 检查 Agent 日志
kubectl logs -n kube-system -l app.kubernetes.io/component=spiderpool-agent

# 检查 Controller 日志
kubectl logs -n kube-system -l app.kubernetes.io/component=spiderpool-controller

# 检查 IP 分配状态
kubectl describe spiderendpoint <endpoint-name>

# 检查 IPAM 配置
kubectl get spiderippools.spiderpool.spidernet.io -o yaml
```

---

## 最佳实践

### 生产环境配置

```yaml
# production-values.yaml
spiderpoolController:
  replicas: 2
  resources:
    requests:
      cpu: 100m
      memory: 128Mi
    limits:
      cpu: 500m
      memory: 512Mi

spiderpoolAgent:
  resources:
    requests:
      cpu: 50m
      memory: 64Mi
    limits:
      cpu: 200m
      memory: 256Mi

ipam:
  # 启用 IP 冲突检测
  enableIPConflictDetection: true
  
  # 启用网关可达性检测
  enableGatewayDetection: true

coordinator:
  enabled: true
  mode: underlay
  tunePodRoutes: true
```

### IP Pool 规划建议

```yaml
# 按业务域划分 IP Pool
---
# 生产环境
apiVersion: spiderpool.spidernet.io/v2beta1
kind: SpiderIPPool
metadata:
  name: prod-web-pool
spec:
  ipVersion: 4
  subnet: "10.6.0.0/16"
  ips: ["10.6.10.1-10.6.10.254"]
  namespaceAffinity:
    matchLabels:
      environment: production
      tier: web

---
# 数据库专用
apiVersion: spiderpool.spidernet.io/v2beta1
kind: SpiderIPPool
metadata:
  name: prod-db-pool
spec:
  ipVersion: 4
  subnet: "10.6.0.0/16"
  ips: ["10.6.20.1-10.6.20.50"]
  namespaceAffinity:
    matchLabels:
      environment: production
      tier: database
```

---

## 参考资源

- [GitHub 仓库](https://github.com/spidernet-io/spiderpool)
- [官方文档](https://spidernet-io.github.io/spiderpool/)
- [安装指南](https://spidernet-io.github.io/spiderpool/usage/install/)
- [Multus CNI](https://github.com/k8snetworkplumbingwg/multus-cni)
- [CNI 规范](https://github.com/containernetworking/cni)
- [CNCF Sandbox](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
