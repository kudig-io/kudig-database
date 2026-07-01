---
title: DNS for Services and Pods
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- coredns
- containerd
- cri-o
- redis
- statefulset
- webhook
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- DNS for Services and Pods 是什么
- 如何 DNS for Services and Pods
trigger_keywords:
- DNS
- for
- Services
- and
- Pods
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- redis-basics
---



# DNS for Services and [[Pods|Pods]]

## 概述

[[Kubernetes|Kubernetes]] 通过集群 DNS（通常由 [[CoreDNS|CoreDNS]] 实现）为 [[Service|Service]] 和 Pod 创建 DNS 记录，使工作负载能够通过一致的域名而非易变的 IP 地址进行相互发现。[[kubelet|kubelet]] 会为每个 Pod 配置 DNS 解析设置（`/etc/resolv.conf`），默认搜索域包括 Pod 所在命名空间和集群域。

## 核心概念/原理

- **Service DNS**：
  - **普通 Service**：分配 A/AAAA 记录，格式为 `my-svc.my-namespace.svc.cluster-domain.example`，解析到 Service 的 ClusterIP。
  - **Headless Service**：同样拥有 A/AAAA 记录，但解析到后端所有 Pod IP 的集合，客户端通常采用轮询方式访问。
  - **SRV 记录**：为命名端口创建 SRV 记录，格式为 `_port-name._protocol.my-svc.my-namespace.svc.cluster-domain.example`。普通 Service 解析到端口和域名；Headless Service 解析到多个结果，每个结果包含 Pod 的端口和独立主机名。
- **Pod DNS**：
  - **基于 IP 的 A 记录**：旧格式为 `<pod-ip>.<namespace>.pod.<cluster-domain>`（如 `172-17-0-3.default.pod.cluster.local`）。CoreDNS 等实现也可能提供 Service 作用域的 A 记录。
  - **hostname / subdomain**：Pod 可通过 `spec.hostname` 和 `spec.subdomain` 自定义主机名。若存在同名的 Headless Service，DNS 会为 Pod 的完整 FQDN 提供 A/AAAA 记录。

## 关键机制或特性

- **Pod DNS 策略（dnsPolicy）**：
  - `ClusterFirst`（默认）：优先使用集群 DNS 解析，不匹配集群域的请求转发到上游 DNS。
  - `Default`：继承节点自身的 DNS 配置。
  - `ClusterFirstWithHostNet`：适用于 `hostNetwork: true` 的 Pod，使其仍使用集群 DNS（Windows 不支持）。
  - `None`：完全忽略 Kubernetes 的 DNS 设置，需通过 `dnsConfig` 自行配置。
- **Pod DNS 配置（dnsConfig）**：自 v1.14 起稳定，允许自定义 `nameservers`（最多 3 个）、`searches`（搜索域，最多 32 个）和 `options`（如 `ndots`、`edns0`）。当 `dnsPolicy` 为 `None` 时，`dnsConfig` 必须指定至少一个 nameserver。
- **setHostnameAsFQDN**：自 v1.22 起稳定，设置为 `true` 时，kubelet 会将 Pod 的 FQDN 写入内核主机名。需注意 Linux 内核主机名长度限制为 64 字符，超出会导致 Pod 无法启动。
- **搜索域限制**：自 v1.28 起稳定，Kubernetes 限制 DNS 搜索域数量不超过 32 个，总长度不超过 2048 字符。早期 containerd/CRI-O 版本可能存在更严格的限制。
- **Windows DNS 差异**：Windows Pod 只能配置一个 DNS 后缀（即所在命名空间的后缀），可解析 FQDN 和短名称，但不能解析部分限定名称（如 `kubernetes.default`）。

## 使用场景

- **服务发现**：替代环境变量，通过域名（如 `my-service` 或 `my-service.my-ns`）访问后端服务，避免启动顺序依赖。
- **自定义主机名**：有状态应用（如数据库集群）通过 `hostname` 和 `subdomain` 获取稳定、可解析的 Pod 级 FQDN。
- **自定义 DNS 解析**：需要指向企业内部 DNS 服务器或添加特定搜索域时，通过 `dnsConfig` 精细控制 Pod 解析行为。
- **hostNetwork Pod 的 DNS**：使用 `ClusterFirstWithHostNet` 确保使用 hostNetwork 的 Pod 仍然能够解析集群内部 DNS 记录。

## 最佳实践/注意事项

- **优先使用 DNS 而非环境变量**：环境变量要求 Service 在 Pod 之前创建，DNS 则不受此限制，更适合动态环境。
- **Windows 特殊限制**：Windows 节点上的 Pod 不支持 `ClusterFirstWithHostNet`；且无法解析部分限定域名，编写跨平台应用时需注意。
- **setHostnameAsFQDN 长度检查**：建议在集群中部署 admission webhook，防止用户创建 FQDN 超过 64 字符的 Pod，避免启动失败。
- **搜索域合并**：自定义 `dnsConfig.searches` 时会与集群默认搜索域合并并去重，留意总数量不要超过 32 条。

## 生产 YAML 示例

### Pod DNS 配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: custom-dns-pod
  namespace: production
spec:
  dnsPolicy: None                  # 完全自定义 DNS
  dnsConfig:
    nameservers:
    - 10.96.0.10                   # 集群 DNS
    - 8.8.8.8                      # 备用外部 DNS
    searches:
    - production.svc.cluster.local
    - svc.cluster.local
    - cluster.local
    - corp.example.com             # 企业内部 DNS 域
    options:
    - name: ndots
      value: "5"
    - name: timeout
      value: "2"
    - name: attempts
      value: "3"
  containers:
  - name: app
    image: registry.example.com/apps/service:v1.0
```

### Headless Service + StatefulSet Pod DNS

```yaml
apiVersion: v1
kind: Service
metadata:
  name: redis
  namespace: cache
spec:
  clusterIP: None                  # Headless
  selector:
    app: redis
  ports:
  - port: 6379
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: redis
  namespace: cache
spec:
  serviceName: redis               # 对应 Headless Service
  replicas: 3
  selector:
    matchLabels:
      app: redis
  template:
    metadata:
      labels:
        app: redis
    spec:
      containers:
      - name: redis
        image: redis:7.2
        ports:
        - containerPort: 6379

# 生成的 DNS 记录：
# redis-0.redis.cache.svc.cluster.local → Pod IP
# redis-1.redis.cache.svc.cluster.local → Pod IP
# redis-2.redis.cache.svc.cluster.local → Pod IP
# redis.cache.svc.cluster.local → 所有 Pod IP（轮询）
```

## DNS 记录格式速查

| 记录类型 | 格式 | 解析结果 |
|----------|------|----------|
| Service A/AAAA | `<svc>.<ns>.svc.cluster.local` | ClusterIP |
| Headless A/AAAA | `<svc>.<ns>.svc.cluster.local` | 所有 Pod IP |
| Pod A/AAAA | `<pod-name>.<svc>.<ns>.svc.cluster.local` | 单个 Pod IP |
| SRV | `_<port>._<proto>.<svc>.<ns>.svc.cluster.local` | 端口 + 域名 |
| Pod（基于 IP） | `<a-b-c-d>.<ns>.pod.cluster.local` | Pod IP |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| DNS 解析超时 | CoreDNS Pod 不健康或资源不足 | `kubectl get pods -n kube-system -l k8s-app=kube-dns` |
| 短名解析失败 | `ndots` 设置过低 | 检查 `/etc/resolv.conf` 中的 `ndots` 值（默认 5） |
| 跨命名空间解析失败 | 使用了短名而非 FQDN | 使用 `<svc>.<ns>` 或完整 FQDN |
| `hostname -f` 返回短名 | 未配置 `subdomain` 或 `setHostnameAsFQDN` | 检查 Pod spec 中的 hostname/subdomain 字段 |
| Windows Pod DNS 异常 | 不支持 `ClusterFirstWithHostNet` 和部分限定名 | 使用 FQDN 或短名，避免部分限定名如 `svc.ns` |

## 生产检查清单

- [ ] 优先使用 DNS（而非环境变量）进行服务发现
- [ ] CoreDNS 至少 2 副本 + 资源 request 已配置
- [ ] 高 QPS 场景配置 CoreDNS 自动扩缩（dns-autoscaler）
- [ ] 自定义 `dnsConfig` 时确保 searches 总数不超过 32 条
- [ ] `setHostnameAsFQDN` 启用前检查 FQDN 不超过 64 字符
- [ ] Windows 节点应用使用完整 FQDN 或短名

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看 Pod 的 DNS 配置
kubectl exec <pod> -- cat /etc/resolv.conf

# 测试 DNS 解析
kubectl exec <pod> -- nslookup my-service.my-namespace.svc.cluster.local

# 查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

# 测试 SRV 记录
kubectl exec <pod> -- nslookup -type=SRV _http._tcp.my-service.production.svc.cluster.local

# 检查 CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml

# 从集群内 debug DNS
kubectl run dnsutils --rm -it --image=registry.k8s.io/e2e-test-images/jessie-dnsutils -- nslookup kubernetes.default
```

## 交叉引用

- [Service](service.md) — Service 的 DNS 名称和服务发现
- [Pod Hostname](../workloads/pod-hostname.md) — Pod 主机名和 FQDN 配置
- [EndpointSlices](endpointslices.md) — Headless Service 的 DNS 记录来源
- [IPv4/IPv6 Dual Stack](ipv4-ipv6-dual-stack.md) — 双栈环境下的 DNS 记录

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/

## Related

- [[domain-19-landscape-references/topic-index/dns-index.md|DNS 知识图谱索引]]
