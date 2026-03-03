# MetalLB

> **成熟度**: Sandbox | **加入时间**: 2021-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://metallb.universe.tf |
| **GitHub** | https://github.com/metallb/metallb |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Networking |
| **适用场景** | 裸金属 Kubernetes 负载均衡 |

---

## 项目概述

MetalLB 是为裸金属 Kubernetes 集群提供的负载均衡器实现。在云环境中，Kubernetes LoadBalancer 类型的 Service 由云提供商自动配置。MetalLB 填补了裸金属环境的空白，通过 Layer 2 (ARP/NDP) 或 BGP 协议为 Service 分配和公告外部 IP 地址。

---

## 核心特性

- **Layer 2 模式**: 使用 ARP (IPv4) 或 NDP (IPv6) 响应本地网络请求
- **BGP 模式**: 与网络路由器建立 BGP 会话公告服务 IP
- **IP 地址池**: 灵活配置可分配的 IP 地址范围
- **自动故障转移**: Leader 选举确保 L2 模式高可用
- **双栈支持**: 同时支持 IPv4 和 IPv6
- **CRD 配置**: 使用 Kubernetes 原生资源配置
- **Prometheus 指标**: 内置监控指标导出

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     MetalLB Architecture                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   External Network                        │   │
│  │                                                           │   │
│  │     Client  ──────────────>  VIP: 192.168.1.100         │   │
│  │                                     │                     │   │
│  └─────────────────────────────────────┼───────────────────┘   │
│                                        │                        │
│  ┌─────────────────────────────────────┼───────────────────┐   │
│  │             Layer 2 Mode            │                    │   │
│  │                                     │                    │   │
│  │   ┌─────────────┐           ┌──────▼──────┐             │   │
│  │   │   Speaker   │  Leader   │   Speaker   │             │   │
│  │   │   (Node 1)  │◄─────────►│   (Node 2)  │             │   │
│  │   │             │ Election  │   [Leader]  │             │   │
│  │   │  ARP Reply  │           │  ARP Reply  │             │   │
│  │   └─────────────┘           └──────┬──────┘             │   │
│  │                                    │                     │   │
│  └────────────────────────────────────┼─────────────────────┘   │
│                                       │                         │
│  ┌────────────────────────────────────┼─────────────────────┐   │
│  │                Kubernetes Cluster  │                      │   │
│  │                                    ▼                      │   │
│  │  ┌────────────────────────────────────────────────────┐  │   │
│  │  │                 Controller Pod                      │  │   │
│  │  │  ┌────────────────┐  ┌────────────────────────┐   │  │   │
│  │  │  │ IP Assignment  │  │ Service Watcher        │   │  │   │
│  │  │  │ from IPPool    │  │ LoadBalancer Type      │   │  │   │
│  │  │  └────────────────┘  └────────────────────────┘   │  │   │
│  │  └────────────────────────────────────────────────────┘  │   │
│  │                                                           │   │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │
│  │  │ Speaker DS │  │ Speaker DS │  │ Speaker DS │         │   │
│  │  │  (Node 1)  │  │  (Node 2)  │  │  (Node 3)  │         │   │
│  │  │ ┌────────┐ │  │ ┌────────┐ │  │ ┌────────┐ │         │   │
│  │  │ │L2/BGP  │ │  │ │L2/BGP  │ │  │ │L2/BGP  │ │         │   │
│  │  │ │Protocol│ │  │ │Protocol│ │  │ │Protocol│ │         │   │
│  │  │ └────────┘ │  │ └────────┘ │  │ └────────┘ │         │   │
│  │  └────────────┘  └────────────┘  └────────────┘         │   │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **Controller** | Deployment，监听 Service 并分配 IP 地址 |
| **Speaker** | DaemonSet，运行在每个节点，负责协议公告 |
| **IPAddressPool** | CRD，定义可分配的 IP 地址范围 |
| **L2Advertisement** | CRD，Layer 2 模式配置 |
| **BGPAdvertisement** | CRD，BGP 模式配置 |
| **BGPPeer** | CRD，BGP 路由器对等体配置 |

---

## 快速开始

### Helm 安装

```bash
# 添加 Helm 仓库
helm repo add metallb https://metallb.github.io/metallb
helm repo update

# 安装 MetalLB
helm install metallb metallb/metallb \
  --namespace metallb-system \
  --create-namespace

# 等待 Pod 就绪
kubectl wait --namespace metallb-system \
  --for=condition=ready pod \
  --selector=app.kubernetes.io/name=metallb \
  --timeout=120s
```

### Manifest 安装

```bash
kubectl apply -f https://raw.githubusercontent.com/metallb/metallb/v0.14.3/config/manifests/metallb-native.yaml
```

---

## Layer 2 模式配置

### IP 地址池

```yaml
apiVersion: metallb.io/v1beta1
kind: IPAddressPool
metadata:
  name: default-pool
  namespace: metallb-system
spec:
  addresses:
    - 192.168.1.100-192.168.1.200  # IP 范围
    - 192.168.2.0/24               # CIDR 格式
    - fc00:f853:0ccd:e799::/124    # IPv6 支持
  autoAssign: true
  avoidBuggyIPs: true  # 避免 .0 和 .255

---
apiVersion: metallb.io/v1beta1
kind: L2Advertisement
metadata:
  name: default-l2
  namespace: metallb-system
spec:
  ipAddressPools:
    - default-pool
  nodeSelectors:              # 可选：限制节点
    - matchLabels:
        node-role: loadbalancer
```

### 多地址池配置

```yaml
apiVersion: metallb.io/v1beta1
kind: IPAddressPool
metadata:
  name: production-pool
  namespace: metallb-system
spec:
  addresses:
    - 10.0.0.100-10.0.0.150

---
apiVersion: metallb.io/v1beta1
kind: IPAddressPool
metadata:
  name: development-pool
  namespace: metallb-system
spec:
  addresses:
    - 10.0.1.100-10.0.1.150

---
apiVersion: metallb.io/v1beta1
kind: L2Advertisement
metadata:
  name: production-l2
  namespace: metallb-system
spec:
  ipAddressPools:
    - production-pool
  interfaces:
    - eth0  # 限制网络接口
```

---

## BGP 模式配置

### BGP 对等体

```yaml
apiVersion: metallb.io/v1beta2
kind: BGPPeer
metadata:
  name: router-1
  namespace: metallb-system
spec:
  myASN: 64500        # MetalLB ASN
  peerASN: 64501      # 路由器 ASN
  peerAddress: 10.0.0.1
  peerPort: 179
  holdTime: 90s
  keepaliveTime: 30s
  routerID: 10.0.0.10  # 可选
  nodeSelectors:
    - matchLabels:
        node-role: bgp-speaker

---
apiVersion: metallb.io/v1beta1
kind: IPAddressPool
metadata:
  name: bgp-pool
  namespace: metallb-system
spec:
  addresses:
    - 203.0.113.0/24

---
apiVersion: metallb.io/v1beta1
kind: BGPAdvertisement
metadata:
  name: bgp-advertisement
  namespace: metallb-system
spec:
  ipAddressPools:
    - bgp-pool
  localPref: 100
  communities:
    - 64500:100
  aggregationLength: 32
```

### BGP 高级配置

```yaml
apiVersion: metallb.io/v1beta2
kind: BGPPeer
metadata:
  name: router-with-auth
  namespace: metallb-system
spec:
  myASN: 64500
  peerASN: 64501
  peerAddress: 10.0.0.1
  passwordSecret:
    name: bgp-password
    namespace: metallb-system
  ebgpMultiHop: true
  bfdProfile: default-bfd  # BFD 支持

---
apiVersion: metallb.io/v1beta1
kind: BFDProfile
metadata:
  name: default-bfd
  namespace: metallb-system
spec:
  receiveInterval: 300
  transmitInterval: 300
  detectMultiplier: 3
  echoInterval: 50
  echoMode: false
  passiveMode: false
  minimumTtl: 254
```

---

## Service 配置

### 基本 LoadBalancer Service

```yaml
apiVersion: v1
kind: Service
metadata:
  name: nginx-lb
  annotations:
    metallb.universe.tf/ip-allocated-from-pool: production-pool
spec:
  type: LoadBalancer
  selector:
    app: nginx
  ports:
    - port: 80
      targetPort: 80
```

### 指定 IP 地址

```yaml
apiVersion: v1
kind: Service
metadata:
  name: nginx-specific-ip
spec:
  type: LoadBalancer
  loadBalancerIP: 192.168.1.100  # 指定 IP
  selector:
    app: nginx
  ports:
    - port: 80
```

### 共享 IP 地址

```yaml
apiVersion: v1
kind: Service
metadata:
  name: service-a
  annotations:
    metallb.universe.tf/allow-shared-ip: "shared-key-1"
spec:
  type: LoadBalancer
  loadBalancerIP: 192.168.1.100
  ports:
    - port: 80

---
apiVersion: v1
kind: Service
metadata:
  name: service-b
  annotations:
    metallb.universe.tf/allow-shared-ip: "shared-key-1"
spec:
  type: LoadBalancer
  loadBalancerIP: 192.168.1.100  # 同一 IP
  ports:
    - port: 443
```

---

## 监控

### Prometheus 指标

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: metallb-controller
  namespace: metallb-system
spec:
  selector:
    matchLabels:
      app.kubernetes.io/component: controller
  endpoints:
    - port: monitoring
      interval: 30s

---
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: metallb-speaker
  namespace: metallb-system
spec:
  selector:
    matchLabels:
      app.kubernetes.io/component: speaker
  endpoints:
    - port: monitoring
```

### 关键指标

| 指标 | 说明 |
|:---|:---|
| `metallb_allocator_addresses_in_use_total` | 已使用 IP 数量 |
| `metallb_allocator_addresses_total` | 可用 IP 总数 |
| `metallb_bgp_session_up` | BGP 会话状态 |
| `metallb_layer2_requests_pending` | L2 待处理请求 |

---

## 最佳实践

1. **网络规划**: 确保 IP 地址池与现有网络不冲突
2. **L2 限制**: Layer 2 模式下单节点承载所有流量，考虑带宽瓶颈
3. **BGP 优先**: 生产环境推荐 BGP 模式实现真正负载均衡
4. **故障排查**: 使用 `speaker` Pod 日志排查网络公告问题
5. **IP 预留**: 为关键服务预分配固定 IP
6. **监控告警**: 配置 IP 池耗尽告警

---

## 参考资源

- [官方文档](https://metallb.universe.tf)
- [GitHub Repo](https://github.com/metallb/metallb)
- [配置参考](https://metallb.universe.tf/configuration/)
- [BGP 模式指南](https://metallb.universe.tf/configuration/bgp/)
- [故障排查](https://metallb.universe.tf/troubleshooting/)

---

**维护者**: Kudig Team | **许可证**: MIT
