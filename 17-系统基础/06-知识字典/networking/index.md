---
title: 网络知识词典
description: 涵盖 Kubernetes 网络全领域的完整术语体系，包括 CNI、Service Mesh、Ingress、Gateway API、DNS、网络策略等
summary: 网络领域词典，覆盖 Cilium、Istio、Envoy、CoreDNS、Gateway API、NetworkPolicy、eBPF 等核心概念
category: dictionary
tags:
- dictionary
- networking
- cni
- service-mesh
- ingress
- gateway-api
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: intermediate
audience:
- 平台工程师
- SRE
- 网络工程师
---

# 网络知识词典（Networking）

> 本词典覆盖 Kubernetes 网络领域的核心术语、技术组件及工程实践，是平台工程师和 SRE 设计、运维集群网络的权威参考。

## 领域概述

Kubernetes 网络是集群通信的基石，包括：

- **Pod 网络**：CNI 插件、Pod 间通信、IP 分配
- **服务发现**：Service、DNS、EndpointSlice
- **流量入口**：Ingress、Gateway API、LoadBalancer
- **服务网格**：Istio、Linkerd、Cilium Service Mesh
- **网络策略**：NetworkPolicy、微分段、零信任
- **多集群网络**：Submariner、Cluster Mesh、跨集群通信

## 核心术语定义

### Pod 网络与 CNI

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| CNI | 容器网络接口规范 | 插件化网络配置 |
| Cilium | 基于 eBPF 的网络/安全/可观测性 | 高性能、无 iptables |
| Calico | BGP/路由反射网络方案 | 网络策略、高性能 |
| Flannel | 简单 Overlay 网络 | VXLAN/host-gw |
| Antrea | 基于 OVS 的 CNI | 企业级网络策略 |
| Kube-OVN | 基于 OVN 的企业级 CNI | 多租户、固定 IP |
| OVN-Kubernetes | OVN 的 K8s 实现 | 分布式路由 |
| Spiderpool | IPAM 插件 | 固定 IP、IP 池管理 |
| eBPF | 内核可编程网络 | 无 iptables、高性能 |
| VXLAN | 虚拟可扩展局域网 | Overlay 封装 |
| IPIP | IP-in-IP 隧道 | 简单封装 |
| BGP | 边界网关协议 | 路由反射、无 Overlay |

### Service 与服务发现

| 术语 | 定义 | 典型场景 |
|------|------|----------|
| ClusterIP | 集群内部虚拟 IP | 内部服务访问 |
| NodePort | 节点端口暴露 | 简单外部访问 |
| LoadBalancer | 云 LB 集成 | 生产外部访问 |
| ExternalName | DNS CNAME 映射 | 外部服务引用 |
| Headless Service | 无 ClusterIP，直接返回 Pod IP | StatefulSet |
| EndpointSlice | 端点分片 | 大规模集群性能优化 |
| kube-proxy | Service 流量转发 | iptables/IPVS/eBPF |
| CoreDNS | 集群 DNS 服务 | 服务发现、外部 DNS |
| Topology Aware Routing | 拓扑感知路由 | 就近访问、降低延迟 |

### Ingress 与 Gateway API

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Ingress | L7 流量入口规则 | HTTP 路由、TLS |
| Ingress Controller | Ingress 规则执行器 | Nginx/Traefik/Contour |
| Gateway API | 下一代流量管理 API | 角色分离、可扩展 |
| Envoy Gateway | Envoy 的 Gateway API 实现 | CNCF 项目 |
| kgateway | Solo.io Gateway API 实现 | 基于 Envoy |
| Traefik | 云原生边缘路由 | 自动发现、Let's Encrypt |
| Contour | VMware Ingress Controller | 基于 Envoy |
| BFE | 百度开源负载均衡 | 高性能 L7 |
| Easegress | 流量编排引擎 | 全场景流量管理 |
| MetalLB | 裸金属 LoadBalancer | BGP/L2 模式 |
| kube-vip | 虚拟 IP 管理 | ARP/BGP 模式 |
| loxilb | eBPF 负载均衡 | 高性能、云原生 |

### 服务网格

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Service Mesh | 服务间通信基础设施层 | 流量管理、可观测性、安全 |
| Istio | 最流行的服务网格 | Envoy sidecar/ambient |
| Linkerd | 轻量级服务网格 | Rust 数据面 |
| Cilium Mesh | 基于 eBPF 的服务网格 | 无 sidecar |
| Kuma | 多运行时服务网格 | Envoy、多区域 |
| Kmesh | 华为内核级服务网格 | eBPF + 可编程内核 |
| Aeraki Mesh | Istio 多协议扩展 | Dubbo/Thrift/Redis |
| Sermant | 华为无侵入服务网格 | Java Agent |
| Consul | HashiCorp 服务网格 | 多数据中心 |
| Envoy | 高性能代理 | xDS API、L4/L7 |

### 网络策略与安全

| 术语 | 定义 | 关键特性 |
|------|------|----------|
| NetworkPolicy | Pod 级别网络访问控制 | 白名单模式 |
| CiliumNetworkPolicy | Cilium 增强网络策略 | L7、FQDN、身份 |
| 微分段 | 网络最小权限原则 | 东西向流量控制 |
| mTLS | 双向 TLS 认证 | 服务间加密通信 |
| FQDN Policy | 基于域名的出口控制 | 外部访问控制 |

### 多集群与跨域网络

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Submariner | 跨集群网络互联 | CNCF Sandbox |
| Cluster Mesh | Cilium 多集群网络 | 服务发现、策略 |
| Clusternet | 多集群应用分发 | 网络透明 |
| KubeSlice | 分布式服务网格 | 跨云互联 |
| K8GB | 全局负载均衡 | DNS 基础 GSLB |
| Kuadrant | 多集群流量管理 | 策略分发 |

## 技术组件索引

### CNI 与 Pod 网络

- [[17-系统基础/06-知识字典/networking/cni.md|CNI]]
- [[17-系统基础/06-知识字典/networking/cilium.md|Cilium]]
- [[17-系统基础/06-知识字典/networking/antrea.md|Antrea]]
- [[17-系统基础/06-知识字典/networking/kube-ovn.md|Kube-OVN]]
- [[17-系统基础/06-知识字典/networking/ovn-kubernetes.md|OVN-Kubernetes]]
- [[17-系统基础/06-知识字典/networking/spiderpool.md|Spiderpool]]
- [[17-系统基础/06-知识字典/networking/vxlan.md|VXLAN]]
- [[17-系统基础/06-知识字典/networking/ipip.md|IPIP]]
- [[17-系统基础/06-知识字典/networking/ebpf-and-cilium-networking.md|eBPF 与 Cilium 网络]]
- [[17-系统基础/06-知识字典/networking/ipv4-ipv6-dual-stack.md|IPv4/IPv6 双栈]]
- [[17-系统基础/06-知识字典/networking/nat.md|NAT]]

### Service 与 DNS

- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/clusterip.md|ClusterIP]]
- [[17-系统基础/06-知识字典/networking/nodeport.md|NodePort]]
- [[17-系统基础/06-知识字典/networking/loadbalancer.md|LoadBalancer]]
- [[17-系统基础/06-知识字典/networking/externalname.md|ExternalName]]
- [[17-系统基础/06-知识字典/networking/headless-service.md|Headless Service]]
- [[17-系统基础/06-知识字典/networking/endpoints.md|Endpoints]]
- [[17-系统基础/06-知识字典/networking/endpointslices.md|EndpointSlice]]
- [[17-系统基础/06-知识字典/networking/endpoint.md|Endpoint]]
- [[17-系统基础/06-知识字典/networking/dns.md|DNS]]
- [[17-系统基础/06-知识字典/networking/coredns.md|CoreDNS]]
- [[17-系统基础/06-知识字典/networking/dns-for-services-and-pods.md|Service/Pod DNS]]
- [[17-系统基础/06-知识字典/networking/dns-resolution.md|DNS 解析]]
- [[17-系统基础/06-知识字典/networking/service-clusterip-allocation.md|ClusterIP 分配]]
- [[17-系统基础/06-知识字典/networking/service-internal-traffic-policy.md|内部流量策略]]
- [[17-系统基础/06-知识字典/networking/topology-aware-routing.md|拓扑感知路由]]

### Ingress 与 Gateway

- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]
- [[17-系统基础/06-知识字典/networking/ingress-controller.md|Ingress Controller]]
- [[17-系统基础/06-知识字典/networking/ingress-controllers.md|Ingress Controllers]]
- [[17-系统基础/06-知识字典/networking/gateway-api.md|Gateway API]]
- [[17-系统基础/06-知识字典/networking/envoy-gateway.md|Envoy Gateway]]
- [[17-系统基础/06-知识字典/networking/kgateway.md|kgateway]]
- [[17-系统基础/06-知识字典/networking/traefik.md|Traefik]]
- [[17-系统基础/06-知识字典/networking/contour.md|Contour]]
- [[17-系统基础/06-知识字典/networking/bfe.md|BFE]]
- [[17-系统基础/06-知识字典/networking/easegress.md|Easegress]]
- [[17-系统基础/06-知识字典/networking/metallb.md|MetalLB]]
- [[17-系统基础/06-知识字典/networking/kube-vip.md|kube-vip]]
- [[17-系统基础/06-知识字典/networking/loxilb.md|loxilb]]

### 服务网格

- [[17-系统基础/06-知识字典/networking/service-mesh.md|Service Mesh]]
- [[17-系统基础/06-知识字典/networking/istio.md|Istio]]
- [[17-系统基础/06-知识字典/networking/linkerd.md|Linkerd]]
- [[17-系统基础/06-知识字典/networking/envoy.md|Envoy]]
- [[17-系统基础/06-知识字典/networking/kuma.md|Kuma]]
- [[17-系统基础/06-知识字典/networking/kmesh.md|Kmesh]]
- [[17-系统基础/06-知识字典/networking/aeraki-mesh.md|Aeraki Mesh]]
- [[17-系统基础/06-知识字典/networking/sermant.md|Sermant]]
- [[17-系统基础/06-知识字典/networking/consul.md|Consul]]
- [[17-系统基础/06-知识字典/networking/cluster-mesh.md|Cluster Mesh]]
- [[17-系统基础/06-知识字典/networking/network-service-mesh.md|Network Service Mesh]]

### 网络策略

- [[17-系统基础/06-知识字典/networking/network-policy.md|Network Policy]]
- [[17-系统基础/06-知识字典/networking/network-policies.md|Network Policies]]
- [[17-系统基础/06-知识字典/networking/networkpolicy.md|NetworkPolicy]]

### 多集群与跨域

- [[17-系统基础/06-知识字典/networking/submariner.md|Submariner]]
- [[17-系统基础/06-知识字典/networking/clusternet.md|Clusternet]]
- [[17-系统基础/06-知识字典/networking/kubeslice.md|KubeSlice]]
- [[17-系统基础/06-知识字典/networking/k8gb.md|K8GB]]
- [[17-系统基础/06-知识字典/networking/kuadrant.md|Kuadrant]]
- [[17-系统基础/06-知识字典/networking/cluster-networking.md|Cluster Networking]]
- [[17-系统基础/06-知识字典/networking/interlink.md|Interlink]]

### 其他

- [[17-系统基础/06-知识字典/networking/akri.md|Akri]]
- [[17-系统基础/06-知识字典/networking/connect-rpc.md|Connect RPC]]
- [[17-系统基础/06-知识字典/networking/networking-on-windows.md|Windows 网络]]
- [[17-系统基础/06-知识字典/networking/telco-cloud-and-5g-mec.md|电信云与 5G MEC]]

## 深度技术解析

### Cilium eBPF 网络架构

Cilium 使用 eBPF 替代 iptables，实现高性能网络：

```
┌─────────────────────────────────────────────────────────┐
│                    Cilium 数据路径                        │
├─────────────────────────────────────────────────────────┤
│  Pod veth → eBPF (tc ingress) → Policy Map → Forward    │
│       │              │              │                    │
│  Endpoint ID    L3/L4 策略     L7 策略 (Envoy)          │
│       │              │              │                    │
│  Identity       Drop/Allow     Proxy redirect           │
└─────────────────────────────────────────────────────────┘
```

**eBPF vs iptables 性能对比**：

| 指标 | iptables | eBPF (Cilium) |
|------|----------|---------------|
| 规则查找 | O(n) 线性 | O(1) 哈希 |
| 10K Service 延迟 | ~5ms | ~0.1ms |
| 更新延迟 | 秒级（全量刷新） | 毫秒级（增量） |
| 连接跟踪 | conntrack 表 | BPF map |
| 可观测性 | 有限 | 内置 Hubble |

### Gateway API 架构

Gateway API 是 Ingress 的下一代替代，核心优势：

```yaml
# Gateway API 角色分离
# 1. 基础设施提供者：GatewayClass
apiVersion: gateway.networking.k8s.io/v1
kind: GatewayClass
metadata:
  name: cilium
spec:
  controllerName: io.cilium/gateway-controller
---
# 2. 集群管理员：Gateway
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: prod-gateway
  namespace: infra
spec:
  gatewayClassName: cilium
  listeners:
    - name: https
      port: 443
      protocol: HTTPS
      tls:
        mode: Terminate
        certificateRefs:
          - name: prod-tls
---
# 3. 应用开发者：HTTPRoute
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: myapp-route
  namespace: app-team
spec:
  parentRefs:
    - name: prod-gateway
      namespace: infra
  rules:
    - matches:
        - path:
            type: PathPrefix
            value: /api
      backendRefs:
        - name: myapp-svc
          port: 8080
```

### NetworkPolicy 实践

```yaml
# 默认拒绝所有入口流量 + 允许特定来源
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}  # 所有 Pod
  policyTypes:
    - Ingress
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-backend
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

## 生产案例

### 案例 1：DNS 解析延迟导致服务超时

**现象**：微服务间调用偶尔超时（5s），日志显示 DNS 解析耗时 4.9s

**根因**：CoreDNS Pod 资源不足，高峰期处理不过来；ndots=5 导致多次无效查询

**解决**：
```yaml
# 1. CoreDNS 扩容 + HPA
# 2. Pod DNS 优化
dnsConfig:
  options:
    - name: ndots
      value: "2"
    - name: timeout
      value: "1"
    - name: attempts
      value: "2"
# 3. 使用 FQDN 尾部加点（跳过 search domain）
# curl http://backend.production.svc.cluster.local.
```

### 案例 2：Cilium 升级后服务不通

**现象**：Cilium 1.14→1.15 升级后，部分 Pod 无法访问 Service

**根因**：BPF map 格式变更，升级过程中 map 未完全迁移

**解决**：
```bash
# 检查 Cilium 状态
cilium status --verbose
# 查看 BPF map
cilium bpf lb list
# 强制重建 endpoint
cilium endpoint regenerate --all
# 必要时滚动重启 Cilium DaemonSet
kubectl rollout restart ds/cilium -n kube-system
```

### 案例 3：NetworkPolicy 导致监控采集失败

**现象**：Prometheus 无法抓取新部署的 Pod 指标

**根因**：默认拒绝策略未放行 Prometheus 命名空间的访问

**解决**：添加允许 monitoring namespace 访问的 NetworkPolicy

## 故障排查要点

| 故障现象 | 可能原因 | 排查方向 |
|----------|----------|----------|
| Pod 无法访问外网 | SNAT 配置/出口策略 | 检查 CNI 配置、NetworkPolicy egress |
| Service 无法访问 | kube-proxy/EndpointSlice | `kubectl get endpoints`、kube-proxy 日志 |
| DNS 解析失败 | CoreDNS 异常 | `kubectl logs -n kube-system -l k8s-app=kube-dns` |
| 跨节点 Pod 不通 | CNI 隧道/路由 | `cilium connectivity test`、路由表 |
| Ingress 502 | 后端 Pod 未就绪 | 检查 Endpoints、健康检查 |
| 网络延迟高 | iptables 规则过多/eBPF | `cilium metrics`、`iptables -L -n \| wc -l` |

## 命令速查

```bash
# Cilium 网络诊断
cilium status
cilium endpoint list
cilium policy get
cilium connectivity test
hubble observe --namespace production

# DNS 调试
kubectl run dns-debug --image=nicolaka/netshoot --rm -it -- bash
nslookup myapp.production.svc.cluster.local
dig @10.96.0.10 myapp.production.svc.cluster.local

# Service/Endpoints 检查
kubectl get svc myapp -o wide
kubectl get endpoints myapp
kubectl get endpointslices -l kubernetes.io/service-name=myapp

# NetworkPolicy 检查
kubectl get networkpolicy -n production
kubectl describe networkpolicy default-deny -n production

# kube-proxy 模式检查
kubectl get cm kube-proxy -n kube-system -o yaml | grep mode
# IPVS 规则
ipvsadm -Ln
```

## FAQ

**Q: Cilium 和 Calico 如何选择？**
A: Cilium 适合需要高性能、可观测性（Hubble）、L7 策略的场景；Calico 适合需要 BGP 路由、与现有网络设备集成的场景。新项目推荐 Cilium（eBPF 趋势）。

**Q: Ingress 和 Gateway API 的关系？**
A: Gateway API 是 Ingress 的下一代替代。Ingress 功能有限（仅 HTTP）、注解混乱；Gateway API 支持多协议、角色分离、可扩展。K8s 1.29+ Gateway API GA，新项目建议直接用 Gateway API。

**Q: 服务网格是否必须？**
A: 不是。服务网格适合：多语言微服务、需要统一 mTLS、细粒度流量控制。简单场景用 Cilium Service Mesh（无 sidecar）或 K8s 原生 NetworkPolicy + Ingress 即可。

**Q: 如何解决大规模集群 iptables 性能问题？**
A: 三种方案：1) 切换到 Cilium eBPF（推荐）；2) kube-proxy IPVS 模式；3) 减少 Service 数量（合并/拆分集群）。

## 版本兼容矩阵

| 组件 | 当前版本 | K8s 兼容 | 关键变更 |
|------|---------|----------|----------|
| Cilium | 1.16 | 1.25+ | Gateway API GA、BPF host routing |
| Istio | 1.23 | 1.27+ | Ambient Mesh GA |
| Linkerd | 2.16 | 1.25+ | 稳定版策略 |
| Gateway API | 1.2 | 1.28+ | GRPCRoute、BackendTLSPolicy |
| CoreDNS | 1.11 | 1.25+ | 性能优化 |
| Calico | 3.29 | 1.25+ | eBPF 数据面增强 |
| Envoy | 1.32 | - | HTTP/3 支持 |
| MetalLB | 0.14 | 1.25+ | FRR BGP |

## 缩略语表

| 缩略语 | 全称 | 说明 |
|--------|------|------|
| CNI | Container Network Interface | 容器网络接口 |
| eBPF | Extended Berkeley Packet Filter | 内核可编程网络 |
| BGP | Border Gateway Protocol | 边界网关协议 |
| VXLAN | Virtual Extensible LAN | 虚拟可扩展局域网 |
| mTLS | Mutual TLS | 双向 TLS |
| SNAT | Source Network Address Translation | 源地址转换 |
| DNAT | Destination NAT | 目的地址转换 |
| IPVS | IP Virtual Server | IP 虚拟服务器 |
| FQDN | Fully Qualified Domain Name | 完全限定域名 |
| GSLB | Global Server Load Balancing | 全局负载均衡 |
| MEC | Multi-access Edge Computing | 多接入边缘计算 |

## 学习路径

```
基础: Service/DNS → Ingress → NetworkPolicy
进阶: CNI 原理 → Cilium → Gateway API
高级: Service Mesh → eBPF → 多集群网络
专家: 自定义 CNI → xDS 协议 → 内核网络栈
```

## 检查清单

### 网络就绪检查

- [ ] CNI 插件版本与 K8s 版本兼容
- [ ] CoreDNS 副本数 ≥ 2（生产）
- [ ] NetworkPolicy 默认拒绝策略已部署
- [ ] Service CIDR 与 Pod CIDR 不冲突
- [ ] Ingress/Gateway TLS 证书自动续期配置
- [ ] kube-proxy 模式适合集群规模（>1000 Service 用 IPVS/eBPF）
- [ ] 跨节点 Pod 连通性已验证
- [ ] DNS ndots 优化已配置（减少无效查询）
- [ ] 服务网格 mTLS 策略已配置（如适用）
- [ ] 多集群网络互联已测试（如适用）

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/
- https://docs.cilium.io/
- https://istio.io/latest/docs/
- https://gateway-api.sigs.k8s.io/
- https://linkerd.io/2/
- https://ebpf.io/
- https://www.cni.dev/

## Related

- [[17-系统基础/06-知识字典/fundamentals/kube-proxy.md|kube-proxy]]
- [[17-系统基础/06-知识字典/security/network-policy.md|NetworkPolicy 安全]]
- [[17-系统基础/06-知识字典/observability/hubble.md|Hubble 可观测性]]
- [[17-系统基础/06-知识字典/multi-cloud/submariner.md|Submariner 多集群]]

