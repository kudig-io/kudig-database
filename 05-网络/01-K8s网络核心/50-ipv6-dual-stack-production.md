---
title: "IPv6 双栈生产实践"
description: "K8s IPv6 双栈生产实践：双栈配置、Service 双栈、DNS AAAA、CNI 支持与迁移策略"
summary: "面向 SRE 与网络工程师的 Kubernetes IPv6 双栈完整落地指南，覆盖集群初始化、Pod/Service 双栈、DNS AAAA、CNI 选型与从单栈到双栈的迁移策略。"
category: 网络
tags:
- ipv6
- dual-stack
- kubernetes
- cni
- dns
- networking
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 网络工程师
estimated_read_time: 20min
intent_queries:
- "Kubernetes 如何配置 IPv6 双栈"
- "双栈 Service 如何暴露 IPv4 和 IPv6"
- "现有集群如何迁移到 IPv6 双栈"
trigger_keywords:
- ipv6
- dual-stack
- aaaa
- ipfamily
- cni
- migration
prerequisites:
- kubectl-basics
- networking-basics
- cni-fundamentals
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# IPv6 双栈生产实践

> **适用版本**: Kubernetes v1.28+（IPv6 双栈 GA 自 1.23）
> **最后更新**: 2026-07

---

## 概述

随着全球 IPv4 地址资源的彻底枯竭，以及 5G、物联网、工业互联网等新兴场景对海量地址的需求爆发，IPv6 已经从"可选项"变为生产网络的"必选项"。在中国，"十四五"规划明确提出要深入推进 IPv6 规模部署，各大云厂商和运营商也在加速 IPv6 基础设施建设。对于运行在 Kubernetes 上的服务而言，支持 IPv6 不再是远期规划，而是当下就需要面对的工程挑战。

Kubernetes 自 1.23 版本起将 IPv6 双栈（Dual-Stack）列为 GA（General Availability）特性，允许 Pod 和 Service 同时拥有 IPv4 和 IPv6 地址。然而，双栈的实际落地远非"在启动参数里加一个 IPv6 网段"那么简单。它涉及 CNI 插件的双栈支持、节点操作系统的内核参数配置、DNS 的 AAAA 记录生成、Service 的 ipFamilyPolicy 设置，以及从存量单栈集群向双栈平滑迁移的完整策略。任何一个环节配置不当，都可能导致部分流量不通或性能下降。

本文系统覆盖双栈的完整生产实践，从集群初始化配置到运行时验证，从故障排查到迁移策略。CNI 层面的双栈支持细节可以参考 [[05-网络/01-K8s网络核心/04b-flannel-ipv6-dual-stack.md|Flannel IPv6 双栈]] 与 [[05-网络/01-K8s网络核心/03-cni-architecture-fundamentals.md|CNI 架构基础]]。

---

## 核心概念

### 1. 双栈网络模型

双栈（Dual-Stack）的核心含义是：每个 Pod 同时获得一个 IPv4 地址和一个 IPv6 地址，集群内的通信可以走任一协议族。这与"IPv6-only"（纯 IPv6，完全不需要 IPv4）是不同的概念——双栈是一种过渡方案，它保留了 IPv4 的兼容性，同时引入了 IPv6 的能力。

理解双栈需要掌握几个关键术语。ipFamily 指地址族，取值为 IPv4 或 IPv6。ipFamilyPolicy 是 Service 级别的地址族策略，决定了 Service 如何分配 ClusterIP：SingleStack 表示仅分配主地址族的 ClusterIP；PreferDualStack 表示尽量分配双栈 ClusterIP，但如果集群不支持双栈则回退到单栈；RequireDualStack 表示强制要求双栈，如果集群不支持则直接报错拒绝创建。

### 2. 双栈 vs 单栈对比

| 维度 | IPv4 单栈 | IPv6 单栈 | IPv6 双栈 |
|------|----------|----------|----------|
| 地址空间 | 受限 | 充足 | 充足 |
| 兼容性 | 最好 | 需全链路支持 | 兼顾两者 |
| 迁移成本 | - | 高（破坏性） | 中（渐进） |
| 配置复杂度 | 低 | 低 | 高 |
| 适用场景 | 存量系统 | 纯新建 | 过渡期/混合 |

对于绝大多数已有生产系统的团队，IPv6 双栈是最务实的选择。它允许渐进式迁移——先让新服务支持双栈，再逐步将存量服务切换，而不需要一次性完成所有改造。纯 IPv6 单栈虽然配置更简单，但要求整个访问链路（客户端、负载均衡、CDN、上游依赖）都支持 IPv6，在当前的互联网环境下很难满足。

### 3. CNI 双栈支持矩阵

CNI 插件是双栈落地的关键依赖，不同 CNI 的双栈支持程度差异很大。

| CNI | 双栈支持 | 备注 |
|-----|---------|------|
| Calico | ✅ 完整 | 支持 IPv6-only 与双栈 |
| Cilium | ✅ 完整 | 1.12+ 双栈 GA |
| Flannel | ✅ 部分 | 需特定 backend，见专题 |
| Terway | ✅ 完整 | 阿里云 ENI 双栈 |
| Weave | ⚠️ 有限 | 不推荐生产双栈 |
| Antrea | ✅ 完整 | 1.5+ |

在选择 CNI 时，如果双栈是明确需求，应优先选择 Calico、Cilium 或 Terway 这些经过大规模生产验证的方案。Flannel 虽然也支持双栈，但仅限于特定 backend（如 VXLAN），且功能相对有限。Weave 的双栈支持不够成熟，不建议在生产环境使用。

---

## 生产部署/实现

### 1. 集群初始化双栈配置 🔴

双栈配置最理想的时机是集群初始化阶段。在已有集群上追加双栈支持虽然可行，但涉及更多兼容性风险和回滚复杂度。

```bash
# 🔴 高风险：集群初始化参数，错误将导致集群不可用
# kubeadm 初始化（IPv4 主栈 + IPv6 副栈）
cat > kubeadm-config.yaml <<EOF
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.30.0
networking:
  podSubnet: "10.244.0.0/16,fd00:10:244::/56"
  serviceSubnet: "10.96.0.0/12,fd00:10:96::/112"
---
apiVersion: kubeadm.k8s.io/v1beta3
kind: InitConfiguration
nodeRegistration:
  kubeletExtraArgs:
    node-ip: "::,0.0.0.0"
EOF

# kube-apiserver / controller-manager 关键参数
# --service-cluster-ip-range=10.96.0.0/12,fd00:10:96::/112
# --cluster-cidr=10.244.0.0/16,fd00:10:244::/56
# --service-node-port-range=30000-32767

kubeadm init --config=kubeadm-config.yaml
```

配置中有几个关键点需要注意。podSubnet 和 serviceSubnet 都包含两个 CIDR，用逗号分隔，第一个是主地址族（IPv4），第二个是副地址族（IPv6）。IPv6 地址段使用 ULA（Unique Local Address）前缀 fd00::/8，这是私有网络的标准做法。node-ip 参数中 IPv6 在前（::），表示节点间通信优先使用 IPv6。

节点内核与 sysctl 前置要求：

```bash
# 🟡 中风险：修改节点内核参数
cat >> /etc/sysctl.d/99-k8s-ipv6.conf <<EOF
net.ipv6.conf.all.disable_ipv6 = 0
net.ipv6.conf.default.disable_ipv6 = 0
net.ipv6.conf.all.forwarding = 1
net.ipv6.conf.all.accept_ra = 0
EOF
sysctl --system
```

这些内核参数是双栈正常工作的基础。disable_ipv6 必须设为 0 以启用 IPv6 协议栈；forwarding 必须设为 1 以允许 IPv6 包转发（Pod 跨节点通信的前提）；accept_ra 设为 0 是为了禁止接受路由器通告，防止节点的 IPv6 配置被外部 RA 消息干扰。

### 2. 双栈 Pod 与 Service 🟡

```yaml
# 🟡 中风险：Service 地址族策略影响可达性
apiVersion: v1
kind: Service
metadata:
  name: web-dualstack
  namespace: production
spec:
  type: LoadBalancer
  ipFamilyPolicy: PreferDualStack
  ipFamilies:
  - IPv4
  - IPv6
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 8080
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web
  namespace: production
spec:
  replicas: 2
  selector:
    matchLabels:
      app: web
  template:
    metadata:
      labels:
        app: web
    spec:
      containers:
      - name: web
        image: registry.example.com/web:v1.0
        ports:
        - containerPort: 8080
```

Service 的 ipFamilyPolicy 设置为 PreferDualStack 是最安全的选择——它会尝试分配双栈 ClusterIP，但如果集群不支持也不会报错。ipFamilies 数组的顺序决定了主地址族，这里 IPv4 在前表示 IPv4 是主栈。对于 LoadBalancer 类型的 Service，云厂商的负载均衡器也需要支持双栈，否则 IPv6 的 ClusterIP 虽然分配了但外部无法通过 IPv6 访问。

验证 Pod 双栈地址：

```bash
# 🟢 低风险：只读
kubectl -n production get pod -l app=web -o wide
kubectl -n production exec deploy/web -- ip -6 addr show
kubectl -n production get svc web-dualstack -o yaml | grep -A5 ipFamilies
```

### 3. Headless Service 双栈（StatefulSet） 🟡

对于 StatefulSet 使用的 Headless Service，双栈配置同样重要，因为它直接影响 Pod 的 DNS 记录。

```yaml
# 🟡 中风险
apiVersion: v1
kind: Service
metadata:
  name: db-headless
  namespace: production
spec:
  clusterIP: None
  ipFamilyPolicy: RequireDualStack
  ipFamilies:
  - IPv4
  - IPv6
  selector:
    app: postgres
  ports:
  - port: 5432
```

Headless Service 配合 RequireDualStack 确保每个 StatefulSet Pod 的 DNS 记录同时包含 A 和 AAAA 记录，使得客户端可以通过任一协议族访问数据库实例。

---

## 运维操作

### 1. DNS AAAA 记录验证 🟢

DNS 是双栈体验的关键环节。如果 CoreDNS 没有正确生成 AAAA 记录，即使 Pod 有 IPv6 地址，服务发现也无法通过 IPv6 进行。

```bash
# 🟢 低风险：只读
# CoreDNS 会自动为双栈 Service 生成 A 与 AAAA 记录
kubectl -n production run dns-test --rm -it --image=nicolaka/netshoot -- bash

# 容器内验证
dig web-dualstack.production.svc.cluster.local A
dig web-dualstack.production.svc.cluster.local AAAA
dig -x <pod-ipv6>     # 反向解析
```

CoreDNS 需要确保 kubernetes 插件正常处理 AAAA 记录的生成。在双栈集群中，CoreDNS 会自动为拥有 IPv6 地址的 Service 和 Pod 生成对应的 AAAA 记录，无需额外配置。但如果发现 AAAA 记录缺失，需要检查 CoreDNS 的日志和配置，参考 [[05-网络/01-K8s网络核心/12-dns-service-discovery-coredns.md|DNS 服务发现与 CoreDNS]]。

### 2. 双栈连通性测试 🟢

```bash
# 🟢 低风险
kubectl -n production exec deploy/web -- curl -6 -g http://[fd00:10:244::5]:8080
kubectl -n production exec deploy/web -- curl -4 http://10.244.1.5:8080

# 跨节点 IPv6 ping
kubectl -n production exec deploy/web -- ping6 -c 3 <other-pod-ipv6>
```

连通性测试应该覆盖四个维度：同节点 IPv4、同节点 IPv6、跨节点 IPv4、跨节点 IPv6。特别注意 IPv6 地址在 URL 中需要用方括号包裹（如 [fd00::1]），这是很多开发者容易犯的错误。

### 3. 检查集群双栈状态 🟢

```bash
# 🟢 低风险
kubectl cluster-info dump | grep -E "service-cluster-ip-range|cluster-cidr"
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDRs}{"\n"}{end}'
```

通过检查 apiserver 和 controller-manager 的启动参数，可以确认集群是否正确配置了双栈网段。每个节点的 podCIDRs 字段应该包含两个 CIDR（一个 IPv4、一个 IPv6），如果只有一个，说明该节点的双栈配置有问题。

---

## 故障排查

### 症状 1：Pod 仅有 IPv4 地址，无 IPv6

```bash
# 🟢 低风险
kubectl get pod <pod> -o yaml | grep -A3 podIPs
kubectl -n kube-system logs ds/<cni-pod> | grep -i ipv6
```

根因通常是 CNI 未启用双栈配置、节点内核的 disable_ipv6 仍为 1、或者集群的 cluster-cidr 未配置 IPv6 网段。处置方法是确认 CNI 的双栈配置已启用、检查并修正节点 sysctl 参数、验证 kubeadm 的 networking 配置包含 IPv6 段。

### 症状 2：Service 无 IPv6 ClusterIP

根因是 ipFamilyPolicy 设置为 SingleStack，或者 apiserver 的 service-cluster-ip-range 参数缺少 IPv6 网段。处置方法是将 ipFamilyPolicy 改为 PreferDualStack，并确认 apiserver 启动参数包含 IPv6 service 网段。

### 症状 3：IPv6 跨节点不通

这是双栈最常见的故障之一。根因可能是节点 IPv6 forwarding 未开启、CNI 的路由或隧道不支持 IPv6 转发、或者节点的 ip6tables 防火墙规则拦截了 Pod 流量。处置方法是确保 net.ipv6.conf.all.forwarding=1、检查 CNI backend 是否支持 IPv6（如 Flannel 的某些 backend 不支持）、放行 ip6tables 中的 FORWARD 链。

### 症状 4：DNS AAAA 解析失败

根因是 CoreDNS 配置问题或 Service 未正确配置为双栈。处置方法是验证 Service 的 ipFamilies 包含 IPv6、检查 CoreDNS 日志是否有错误、确认 Pod 的 resolv.conf 配置正确，参考 [[05-网络/01-K8s网络核心/29-coredns-troubleshooting-optimization.md|CoreDNS 故障排查]]。

### 排查决策树

```
双栈异常
├── Pod 无 IPv6?    → CNI/节点 sysctl/cluster-cidr
├── Service 无 IPv6? → ipFamilyPolicy/service-range
├── 跨节点不通?     → forwarding/CNI/iptables
└── AAAA 失败?      → CoreDNS/Service ipFamilies
```

---

## 最佳实践

第一，规划先行，双栈最好在集群创建时就规划好，存量集群迁移成本高且风险大。第二，CNI 选型上，生产双栈优先选择 Calico、Cilium 或 Terway，避免使用 Weave。第三，主栈选择上，过渡期以 IPv4 为主栈（ipFamilies 中 IPv4 在前），降低兼容性风险。第四，Service 策略上，内部服务用 PreferDualStack 保证兼容，有明确双栈需求的用 RequireDualStack。第五，DNS 方面确保 CoreDNS 正确生成 AAAA 记录，应用层实现 Happy Eyeballs 算法（优先 IPv6，快速回退 IPv4）。第六，监控方面分别监控 IPv4 和 IPv6 的流量与错误率，及时发现单栈异常。第七，迁移策略分四个阶段推进：阶段一在节点启用 IPv6 并配置 CNI 双栈；阶段二新建 Service 使用双栈；阶段三存量 Service 逐步开启 PreferDualStack；阶段四评估是否将主栈切换为 IPv6。

```yaml
# 🟢 低风险：NetworkPolicy 双栈（同时匹配 IPv4/IPv6）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-web-dualstack
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: web
  policyTypes:
  - Ingress
  ingress:
  - from:
    - ipBlock:
        cidr: 10.0.0.0/8
    - ipBlock:
        cidr: fd00::/8
    ports:
    - protocol: TCP
      port: 8080
```

---

## Related

- [[05-网络/01-K8s网络核心/04b-flannel-ipv6-dual-stack.md|Flannel IPv6 双栈]]
- [[05-网络/01-K8s网络核心/03-cni-architecture-fundamentals.md|CNI 架构基础]]
- [[05-网络/01-K8s网络核心/04-cni-plugins-comparison.md|CNI 插件对比]]
- [[05-网络/01-K8s网络核心/12-dns-service-discovery-coredns.md|DNS 服务发现与 CoreDNS]]
- [[05-网络/01-K8s网络核心/29-coredns-troubleshooting-optimization.md|CoreDNS 故障排查]]
- [[05-网络/01-K8s网络核心/17-networkpolicy-deep-practice.md|NetworkPolicy 深度实践]]
