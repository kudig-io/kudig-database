---
title: Cilium eBPF 网络与安全实践指南
description: '# Cilium eBPF 网络与安全实践指南'
summary: 'cilium clustermesh enable --context cluster1'
category: network-fundamentals
tags:
- network
- tcp
- ip
- dns
- etcd
- prometheus
- envoy
- cilium
- helm
- hpa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 网络工程师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Cilium eBPF 网络与安全实践指南 是什么
- 如何 Cilium eBPF 网络与安全实践指南
- Kubernetes 15 network fundamentals 最佳实践
trigger_keywords:
- Cilium
- eBPF
- 网络与安全实践指南
- network
- fundamentals
prerequisites:
- kubectl-basics
- networking-basics
- helm-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- etcd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: fta
  path: ../故障诊断/FTA故障树/list/cilium-fta.md
  label: '故障树: cilium'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[cilium|Cilium]] eBPF 网络与安全实践指南

> **适用版本**: Cilium v1.17.0  
> **最后更新**: 2026-04-24  
> **难度**: 中级 → 高级

---

## 📋 目录

- [一、架构模式](#一架构模式)
- [二、安装部署](#二安装部署)
- [三、网络策略 (L3-L7)](#三网络策略-l3-l7)
- [四、服务网格 (无 Sidecar)](#四服务网格-无-sidecar)
- [五、可观测性 (Hubble)](#五可观测性-hubble)
- [六、Cluster Mesh 多集群](#六cluster-mesh-多集群)
- [七、Gateway API](#七gateway-api)
- [八、WireGuard 加密](#八wireguard-加密)
- [九、性能调优](#九性能调优)

---

## 一、架构模式

```
Cilium 数据平面 (eBPF)
├── Node
│   ├── eBPF Programs (XDP, TC, Socket)
│   ├── BPF Maps (连接跟踪、策略、负载均衡)
│   ├── Cilium Agent (DaemonSet)
│   └── Envoy (L7 代理, 按需)
│
控制平面
├── Cilium Operator (Deployment)
├── etcd / CRD (状态存储)
└── Hubble (可观测性, 可选)
```

---

## 二、安装部署

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Helm 安装 (推荐)
helm repo add cilium https://helm.cilium.io/
helm install cilium cilium/cilium \
  --namespace kube-system \
  --set ipam.mode=kubernetes \
  --set hubble.enabled=true \
  --set hubble.relay.enabled=true \
  --set hubble.ui.enabled=true \
  --set prometheus.enabled=true \
  --set operator.prometheus.enabled=true \
  --version 1.17.0

# 验证
cilium status --wait
```
---

## 三、网络策略 (L3-L7)

### 3.1 L3/L4 CiliumNetworkPolicy

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: api-allow-frontend
  namespace: production
spec:
  endpointSelector:
    matchLabels:
      app: api
  ingress:
  - fromEndpoints:
    - matchLabels:
        app: frontend
    toPorts:
    - ports:
      - port: "8080"
        protocol: TCP
      rules:
        http:
        - method: GET
          path: "/api/v1/.*"
```

### 3.2 DNS 策略 (L7)

```yaml
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: egress-dns
spec:
  endpointSelector:
    matchLabels:
      app: microservice
  egress:
  - toFQDNs:
    - matchName: api.stripe.com
    - matchPattern: "*.googleapis.com"
    toPorts:
    - ports:
      - port: "443"
        protocol: TCP
```

### 3.3 基于身份的策略 (非 IP)

```yaml
# 使用 Cilium 安全身份而非 IP
apiVersion: cilium.io/v2
kind: CiliumClusterwideNetworkPolicy
metadata:
  name: default-deny
spec:
  endpointSelector: {}
  ingressDeny:
  - {}
  egressDeny:
  - {}
```

---

## 四、服务网格 (无 Sidecar)

### 4.1 三种服务模式

| 模式 | 描述 | 性能 |
|:---|:---|:---|
| LoadBalancer + Network Policy | L4 负载均衡 + 安全策略 | 最优 |
| Envoy Extension (per-node) | 节点级 L7 代理 | 优秀 |
| Sidecar (per-pod) | 传统 Sidecar 模式 | 标准 |

### 4.2 Ingress + L7 策略

```yaml
apiVersion: cilium.io/v2
kind: CiliumClusterwideEnvoyConfig
metadata:
  name: l7-traffic-management
spec:
  services:
  - name: my-service
    namespace: default
  resources:
  - "@type": type.googleapis.com/envoy.config.route.v3.RouteConfiguration
    name: listener_0
    virtual_hosts:
    - name: default
      domains: ["*"]
      routes:
      - matchers:
        - prefix="/api/v1"
        - route=""
        - cluster="default/my-service"
        - timeout="10s"
```

---

## 五、可观测性 (Hubble)

```bash
# Hubble CLI
hubble status
hubble observe --namespace production
hubble observe --pod frontend-xxx --protocol http

# 流量拓扑
hubble observe --verdict DROPPED
```

### Hubble UI

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 端口转发访问 UI
kubectl port-forward -n kube-system svc/hubble-ui 12000:80
# 访问 http://localhost:12000
```
---

## 六、Cluster Mesh 多集群

```bash
# 创建 clustermesh
cilium clustermesh enable --context cluster1

# 连接集群
cilium clustermesh connect \
  --context cluster1 \
  --destination-context cluster2

# 验证
cilium clustermesh status --context cluster1
```

**能力**:
- 跨集群服务发现 (`service.cluster1`)
- 全局网络策略
- 跨集群负载均衡
- 故障域隔离

---

## 七、Gateway API

```yaml
apiVersion: gateway.networking.k8s.io/v1
kind: Gateway
metadata:
  name: cilium-gateway
spec:
  gatewayClassName: cilium
  listeners:
  - name: https
    protocol: HTTPS
    port: 443
    tls:
      certificateRefs:
      - name: example-cert
---
apiVersion: gateway.networking.k8s.io/v1
kind: HTTPRoute
metadata:
  name: example-route
spec:
  parentRefs:
  - name: cilium-gateway
  rules:
  - matches:
    - path:
        type: PathPrefix
        value: /api
    backendRefs:
    - name: api-service
      port: 80
```

---

## 八、WireGuard 加密

```yaml
# Helm values
encryption:
  enabled: true
  type: wireguard

# 或 IPsec
encryption:
  enabled: true
  type: ipsec
  ipsec:
    keyFile: /keys/psk
```

- **Node-to-Node**: 自动加密所有 Pod 间流量
- **透明**: 无需应用改动
- **性能**: WireGuard 优于 IPsec

---

## 九、性能调优

| 参数 | 默认值 | 生产建议 |
|:---|:---|:---|
| `bpf.mapDynamicSizeRatio` | 0.0025 | 高负载 0.005 |
| `ipv4.fragmentsMapMax` | 8192 | 大流量 32768 |
| `bandwidthManager.enabled` | false | 启用 (EDT-based) |
| `hostFirewall.enabled` | false | 需要时启用 |
| `loadBalancer.mode` | dsr | snat (兼容性) / dsr (性能) |
| `kubeProxyReplacement` | false | strict (完全替代) |

---

## 参考链接

- [Cilium 官方文档](https://docs.cilium.io/)
- [Cilium Helm 参考](https://docs.cilium.io/en/stable/helm-reference/)
- [Hubble 文档](https://docs.cilium.io/en/stable/observability/hubble/)
- [Cilium Cluster Mesh](https://docs.cilium.io/en/stable/network/clustermesh/)
- [eBPF 文档](https://ebpf.io/what-is-ebpf/)

---

## Obsidian 相关文档

- 网络 KUDIG Database — Global MOC
- [[05-网络/README.md|Domain-15: 网络基础]]
- index.md|Domain-15 网络基础 — 开源项目索引]]
- 网络协议栈详解
- TCP/UDP 协议深度解析
- DNS 原理与配置
- 负载均衡技术
- 网络安全基础
- SDN 与网络虚拟化

## See Also

- 05-network-security-fundamentals
- 06-sdn-network-virtualization
- 01-network-protocols-stack
- 02-tcp-udp-deep-dive

## Related

- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/flannel-index.md|Flannel 知识图谱索引]]
- [[21-生态参考/03-领域索引/network-index.md|Network 网络知识图谱索引]]

```

<!-- risk-assessed -->
