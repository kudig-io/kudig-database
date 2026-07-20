---
title: "DNS 高级与外部集成"
description: "DNS 高级与外部集成：CoreDNS 调优、ExternalDNS、DNS 策略、NDots 优化与 DNS 监控"
summary: "面向 SRE 与网络工程师的 Kubernetes DNS 高级实践，覆盖 CoreDNS 性能调优、ExternalDNS 自动同步、dnsPolicy 选型、NDots 优化与 DNS 可观测性。"
category: 网络
tags:
- dns
- coredns
- externaldns
- ndots
- networking
- observability
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
- "CoreDNS 如何调优提升性能"
- "ExternalDNS 如何自动同步 DNS 记录"
- "NDots 导致 DNS 慢如何优化"
trigger_keywords:
- dns
- coredns
- externaldns
- ndots
- dnspolicy
- dns monitoring
prerequisites:
- kubectl-basics
- networking-basics
- coredns-basics
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

# DNS 高级与外部集成

> **适用版本**: CoreDNS 1.11+ / ExternalDNS 0.14+ / Kubernetes v1.28+
> **最后更新**: 2026-07

---

## 概述

DNS 是 Kubernetes 服务发现的基石，每一个 Service 访问、每一次跨 Pod 通信，背后都依赖 DNS 解析的正确工作。然而，DNS 也是最容易被忽视的性能瓶颈和故障源。在我们的生产实践中，见过太多因为 DNS 配置不当导致的诡异问题：应用访问外部 API 的延迟莫名其妙地翻了几倍、服务间调用偶发性超时、Pod 启动时 DNS 解析失败导致 CrashLoopBackOff。

一个最典型也最普遍的反模式是 NDot 问题：应用访问外部域名 api.example.com 时，由于 Kubernetes 默认的 ndots:5 配置，DNS 客户端会先尝试 api.example.com.default.svc.cluster.local、api.example.com.svc.cluster.local、api.example.com.cluster.local 等多个无效后缀，最后才查询真实的 api.example.com。这意味着每一次外部 DNS 查询都被放大了数倍，不仅增加了延迟，还给 CoreDNS 带来了不必要的负载。

本文聚焦 DNS 的高级主题：CoreDNS 性能调优、ExternalDNS 自动化、dnsPolicy 选型、NDots 优化与 DNS 监控。CoreDNS 的基础架构与 Corefile 配置见 [[网络/K8s网络核心/13-coredns-architecture-principles.md|CoreDNS 架构原理]] 与 [[网络/K8s网络核心/14-coredns-configuration-corefile.md|CoreDNS Corefile 配置]]。

---

## 核心概念

### 1. DNS 查询放大问题（NDots）

NDot 问题是 Kubernetes DNS 中最普遍的性能陷阱，理解它的机制是优化的前提。

/etc/resolv.conf 中的 ndots:5 表示：当域名中的点号数量少于 5 个时，DNS 客户端会先尝试将域名与 search domain 列表中的每个后缀拼接查询，只有所有拼接查询都失败后，才会查询原始域名。Kubernetes 默认为每个 Pod 配置了 3 个 search domain：

```
search default.svc.cluster.local svc.cluster.local cluster.local
options ndots:5
```

当应用访问 api.example.com（包含 2 个点，少于 5）时，DNS 客户端会依次查询：api.example.com.default.svc.cluster.local（NXDOMAIN）、api.example.com.svc.cluster.local（NXDOMAIN）、api.example.com.cluster.local（NXDOMAIN），最后才查询 api.example.com 获得正确答案。一次外部查询变成了四次，延迟增加了三倍，CoreDNS 的负载也增加了三倍。对于频繁访问外部服务的 Pod，这个放大效应是巨大的。

### 2. dnsPolicy 选型

dnsPolicy 决定了 Pod 的 DNS 解析行为，不同场景需要不同的策略。

| dnsPolicy | 行为 | 适用场景 |
|-----------|------|---------|
| `ClusterFirst`（默认） | 先查集群 DNS | 普通 Pod |
| `ClusterFirstWithHostNet` | hostNetwork Pod 也用集群 DNS | hostNetwork 服务 |
| `Default` | 继承节点 resolv.conf | 仅需外部 DNS |
| `None` | 完全由 dnsConfig 指定 | 自定义 DNS |

大多数 Pod 使用默认的 ClusterFirst 即可。但有一个常见的陷阱：使用 hostNetwork: true 的 Pod（如某些 DaemonSet 监控组件）默认不会使用集群 DNS，而是继承节点的 resolv.conf，导致无法解析集群内的 Service 名称。这种情况下需要显式设置 dnsPolicy 为 ClusterFirstWithHostNet。对于完全不需要访问集群内服务、只访问外部地址的 Pod，可以考虑使用 Default 策略绕过集群 DNS，减少 CoreDNS 的负载。

### 3. ExternalDNS 工作原理

ExternalDNS 解决的是 Kubernetes 服务与外部 DNS 系统的同步问题。在没有 ExternalDNS 的情况下，当你创建一个 LoadBalancer 类型的 Service 或 Ingress 时，需要手动到 DNS 提供商（如 Route53、Cloudflare）的控制台创建对应的 DNS 记录。ExternalDNS 将这个流程自动化——它持续监听 Kubernetes 中的 Service、Ingress、Gateway 等资源的变化，自动在外部 DNS 提供商中创建、更新和删除对应的 DNS 记录，实现 DNS 即代码（DNS-as-Code）。

---

## 生产部署/实现

### 1. CoreDNS 性能调优 🟡

CoreDNS 的性能调优涉及 Corefile 配置优化和部署架构优化两个层面。

```yaml
# 🟡 中风险：修改 CoreDNS 配置影响全集群 DNS
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health {
            lameduck 5s
        }
        ready
        kubernetes cluster.local in-addr.arpa ip6.arpa {
            pods insecure
            fallthrough in-addr.arpa ip6.arpa
            ttl 30
        }
        prometheus :9153
        forward . /etc/resolv.conf {
            max_concurrent 1000
        }
        cache 30 {
            success 9984 30
            denial 9984 5
        }
        loop
        reload
        loadbalance
        autopath @kubernetes      # 服务端自动路径，减少 Ndots 放大
    }
---
# CoreDNS 水平扩容 + 反亲和
apiVersion: apps/v1
kind: Deployment
metadata:
  name: coredns
  namespace: kube-system
spec:
  replicas: 4                    # 大规模集群增加副本
  template:
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  k8s-app: kube-dns
              topologyKey: kubernetes.io/hostname
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: ScheduleAnyway
        labelSelector:
          matchLabels:
            k8s-app: kube-dns
```

Corefile 中有几个关键的优化点。autopath @kubernetes 是最重要的一项，它让 CoreDNS 在服务端自动处理 search domain 的拼接逻辑，客户端只需发送一次查询，CoreDNS 会在内部完成所有后缀的尝试，从根本上解决了 NDot 放大问题。cache 配置中，success 9984 30 表示成功响应缓存 9984 条、TTL 30 秒，denial 9984 5 表示否定响应（NXDOMAIN）缓存 9984 条、TTL 5 秒——否定响应的 TTL 设短一些，确保新创建的 Service 能快速被发现。forward 的 max_concurrent 1000 限制了同时转发到上游 DNS 的查询数，防止上游故障时 CoreDNS 被拖垮。

部署架构方面，CoreDNS 副本数应该根据集群规模调整，一般每 1000 个 Pod 配置 1-2 个 CoreDNS 副本。podAntiAffinity 确保 CoreDNS 副本分散在不同节点上，topologySpreadConstraints 进一步确保跨可用区分布，避免单节点或单 zone 故障导致全集群 DNS 不可用。

### 2. Pod 级 NDot 优化 🟡

对于无法启用 autopath 或需要更精细控制的场景，可以在 Pod 级别调整 DNS 配置。

```yaml
# 🟡 中风险：dnsConfig 影响 Pod DNS 行为
apiVersion: apps/v1
kind: Deployment
metadata:
  name: external-caller
  namespace: production
spec:
  template:
    spec:
      dnsPolicy: ClusterFirst
      dnsConfig:
        options:
        - name: ndots
          value: "2"            # 降低 ndots，减少外部域名查询放大
        - name: single-request-reopen
        - name: timeout
          value: "1"
        - name: attempts
          value: "3"
      containers:
      - name: app
        image: registry.example.com/app:v1.0
```

将 ndots 从默认的 5 降低到 2，意味着只有点号少于 2 个的域名（如 kubernetes）才会尝试 search domain 拼接，而 api.example.com（2 个点）会直接查询原始域名。这大幅减少了外部域名的查询次数。single-request-reopen 选项用于解决一个已知的 conntrack 竞态问题——当 A 和 AAAA 查询同时发出时，某些内核版本的 conntrack 会混淆两个 UDP 流，导致其中一个查询超时。timeout 和 attempts 控制单次查询的超时时间和重试次数，避免 DNS 故障时应用长时间阻塞。

对于纯外部访问的服务，还有一个更彻底的方案：在代码中访问域名时加上尾部点号（如 api.example.com.），这会让 DNS 客户端将其视为 FQDN（完全限定域名），完全跳过 search domain 拼接。

### 3. ExternalDNS 部署（以 Cloudflare 为例） 🔴

ExternalDNS 会真实修改外部 DNS 记录，配置错误可能导致生产域名被误删，因此是高风险操作。

```yaml
# 🔴 高风险：ExternalDNS 会真实修改外部 DNS 记录，配置错误可能删除生产记录
apiVersion: v1
kind: ServiceAccount
metadata:
  name: external-dns
  namespace: kube-system
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: external-dns
rules:
- apiGroups: [""]
  resources: ["services", "endpoints", "pods"]
  verbs: ["get", "watch", "list"]
- apiGroups: ["networking.k8s.io"]
  resources: ["ingresses"]
  verbs: ["get", "watch", "list"]
- apiGroups: ["gateway.networking.k8s.io"]
  resources: ["gateways", "httproutes"]
  verbs: ["get", "watch", "list"]
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: external-dns
  namespace: kube-system
spec:
  strategy:
    type: Recreate
  selector:
    matchLabels:
      app: external-dns
  template:
    metadata:
      labels:
        app: external-dns
    spec:
      serviceAccountName: external-dns
      containers:
      - name: external-dns
        image: registry.k8s.io/external-dns/external-dns:v0.14.2
        args:
        - --source=ingress
        - --source=gateway-httproute
        - --domain-filter=example.com      # 仅管理指定域名，防止误删
        - --provider=cloudflare
        - --policy=sync                    # sync 会删除多余记录；upsert-only 更安全
        - --registry=txt
        - --txt-owner-id=k8s-prod
        env:
        - name: CF_API_TOKEN
          valueFrom:
            secretKeyRef:
              name: cloudflare-token
              key: token
```

这个配置中有两个关键的安全设计。第一是 --domain-filter=example.com，它将 ExternalDNS 的管理范围限定在 example.com 域名下，即使配置出错也不会影响其他域名。第二是 --policy 参数：upsert-only 模式只创建和更新记录，从不删除，是最安全的选择；sync 模式会删除不在 Kubernetes 中的记录，实现完全同步，但如果 Kubernetes 资源被误删，对应的 DNS 记录也会被删除。我们的建议是生产环境先用 upsert-only 运行一段时间，确认行为符合预期后再切换到 sync。--registry=txt 和 --txt-owner-id 用于在 DNS 记录中创建 TXT 所有权标记，防止多个 ExternalDNS 实例管理同一域名时互相冲突。

---

## 运维操作

### 1. DNS 查询诊断 🟢

```bash
# 🟢 低风险：只读
kubectl -n production run dns-test --rm -it --image=nicolaka/netshoot -- bash

# 容器内
dig kubernetes.default.svc.cluster.local
dig api.example.com +search          # 观察 search domain 行为
dig api.example.com. +noall +stats   # 尾部点号，对比查询次数
nslookup -debug web.production.svc.cluster.local

# 查看 Pod resolv.conf
kubectl -n production exec deploy/app -- cat /etc/resolv.conf
```

netshoot 镜像包含了 dig、nslookup、host、tcpdump 等完整的网络诊断工具，是 DNS 排查的瑞士军刀。通过对比 dig api.example.com +search 和 dig api.example.com. 的查询统计，可以直观看到 NDot 放大带来的额外查询次数。

### 2. CoreDNS 指标与日志 🟢

```bash
# 🟢 低风险
kubectl -n kube-system port-forward deploy/coredns 9153:9153
curl -s http://localhost:9153/metrics | grep coredns_dns_request_duration

# 关键指标
# coredns_dns_requests_total          请求总量（按 zone/type）
# coredns_dns_request_duration_seconds 延迟分布
# coredns_cache_hits_total / misses_total 缓存命中率
# coredns_forward_requests_total      转发到上游的请求
```

CoreDNS 通过 prometheus 插件暴露丰富的指标。缓存命中率（hits / (hits + misses)）是最重要的健康指标之一，正常情况下应该在 90% 以上。如果命中率过低，说明缓存配置不合理或者查询模式异常。forward 请求量反映了外部 DNS 查询的压力，如果过高说明 NDot 放大问题严重。

### 3. ExternalDNS 同步状态 🟢

```bash
# 🟢 低风险
kubectl -n kube-system logs deploy/external-dns --tail=100 | grep -i "creating\|updating\|deleting"
# 查看 TXT 所有权记录
dig TXT api.example.com
```

---

## 故障排查

### 症状 1：外部域名解析慢

```bash
# 🟢 低风险
kubectl -n production exec deploy/app -- time nslookup api.example.com
```

根因是 ndots:5 导致查询放大、CoreDNS 缓存未命中、或者上游 DNS 响应慢。处置方法是降低 ndots 至 2 或在域名后加尾部点号、增大 CoreDNS 缓存容量、启用 autopath 插件。

### 症状 2：DNS 查询间歇超时（5s 延迟）

这是一个经典的 Kubernetes DNS 问题，表现为 DNS 查询偶发性地精确超时 5 秒。根因是 Linux conntrack 表在处理 UDP 时的竞态条件——当 A 和 AAAA 查询几乎同时发出时，conntrack 可能将两个 UDP 包误判为同一个流，导致其中一个被丢弃。处置方法是添加 single-request-reopen 选项、扩容 CoreDNS 副本、部署 NodeLocal DNSCache 将 DNS 查询本地化。

### 症状 3：ExternalDNS 未创建记录

根因可能是 domain-filter 不匹配目标域名、RBAC 权限不足无法读取 Ingress、Ingress 缺少必要的注解、或者 DNS 提供商的 API 凭证错误。处置方法是检查 domain-filter 配置、确认 RBAC 包含所需资源的读取权限、为 Ingress 添加 external-dns.alpha.kubernetes.io/hostname 注解、验证 API token 有效性。

### 症状 4：Service 删除后 DNS 记录残留

根因是 --policy=upsert-only 模式下 ExternalDNS 不会删除记录，或者 TXT 所有权记录冲突导致无法确认记录归属。处置方法是评估切换到 sync 模式（需谨慎）、手动清理残留记录、确认 txt-owner-id 配置一致。

### 排查决策树

```
DNS 异常
├── 解析慢?       → ndots/cache/上游
├── 间歇超时?     → conntrack/CoreDNS 副本/NodeLocal
├── 记录未创建?   → ExternalDNS filter/RBAC/注解
└── 记录残留?     → policy/TXT 所有权
```

---

## 最佳实践

第一，NDots 优化是投入产出比最高的 DNS 调优手段，外部访问密集的服务应降低 ndots 至 2 或使用尾部点号。第二，CoreDNS 扩容遵循每 1000 Pod 配 1-2 副本的经验值，配合反亲和和跨 zone 分布。第三，合理设置缓存 TTL，启用 autopath 从根本上消除 NDot 放大。第四，部署 NodeLocal DNSCache 是解决 DNS 性能和稳定性问题的终极方案，它在每个节点运行本地 DNS 缓存，大幅降低 CoreDNS 压力并消除 conntrack 竞态问题，参考 [[网络/K8s网络核心/28-coredns-troubleshooting-optimization.md|CoreDNS 故障排查]]。第五，ExternalDNS 安全配置要用 domain-filter 限定管理范围，生产先用 upsert-only 验证后再切 sync。第六，建立 DNS 监控体系，采集 CoreDNS 延迟、缓存命中率、forward 错误率并设置告警。第七，hostNetwork Pod 使用 ClusterFirstWithHostNet 策略，纯外部服务评估 Default 策略。

```yaml
# 🟢 低风险：CoreDNS 延迟告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: coredns-alerts
spec:
  groups:
  - name: coredns
    rules:
    - alert: CoreDNSHighLatency
      expr: histogram_quantile(0.99, rate(coredns_dns_request_duration_seconds_bucket[5m])) > 0.1
      for: 10m
      labels:
        severity: warning
    - alert: CoreDNSForwardErrors
      expr: rate(coredns_forward_responses_total{rcode="SERVFAIL"}[5m]) > 1
      for: 5m
      labels:
        severity: warning
```

---

## Related

- [[网络/K8s网络核心/13-coredns-architecture-principles.md|CoreDNS 架构原理]]
- [[网络/K8s网络核心/14-coredns-configuration-corefile.md|CoreDNS Corefile 配置]]
- [[网络/K8s网络核心/11-dns-service-discovery-coredns.md|DNS 服务发现与 CoreDNS]]
- [[网络/K8s网络核心/28-coredns-troubleshooting-optimization.md|CoreDNS 故障排查]]
- [[网络/K8s网络核心/15-coredns-plugins-reference.md|CoreDNS 插件参考]]
- [[网络/K8s网络核心/49-multicluster-network-federation.md|多集群网络联邦]]
