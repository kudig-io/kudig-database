---
title: Headless Service
summary: Headless Service 是 Kubernetes 中一种特殊的 Service 类型，其核心特征是将 spec.clusterIP 显式设置为
  "None"，从而不分配虚拟 ClusterIP。
category: concepts
tags:
- core-concept
- 网络
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---



# Headless Service

Headless Service 是 Kubernetes 中一种特殊的 Service 类型，其核心特征是将 `spec.clusterIP` 显式设置为 `"None"`，从而不分配虚拟 ClusterIP。

## 核心定义

在普通 Service 中，kube-proxy 通过 ClusterIP 提供负载均衡，请求被转发到后端 Pod。而 Headless Service 不做这层抽象，DNS 查询直接返回后端 Pod 的 IP 列表。

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-headless-service
spec:
  clusterIP: "None"  # 关键配置
  selector:
    app: my-app
  ports:
    - port: 80
```

## 适用场景

Headless Service 主要适用于以下场景：

- **StatefulSet 的有序网络标识**：为每个 Pod 提供稳定的、可预测的 DNS 名称
- **直接访问 Pod IP**：客户端需要知道所有后端 Pod 的具体地址，自行决定连接策略
- **自定义服务发现**：应用层需要实现特定的负载均衡或分片逻辑
- **数据库集群**：如 MongoDB、Cassandra、Redis Cluster 等需要节点间直接通信的分布式系统

## DNS 解析行为

与普通 Service 不同，Headless Service 的 DNS 解析返回所有符合条件的后端 Pod IP 的 A 记录：

```
dig my-headless-service.default.svc.cluster.local
# 返回所有 Pod IP，而非单一的 ClusterIP
```

对于设置了 `publishNotReadyAddresses: true` 的 Headless Service，DNS 甚至会包含未就绪的 Pod，适用于需要提前发现所有成员的集群初始化场景。

## StatefulSet + Headless Service

这是 Headless Service 最典型的组合。StatefulSet 配合 Headless Service 为每个 Pod 提供**稳定的网络标识**：

```
<pod-name>.<service-name>.<namespace>.svc.cluster.local
```

例如，名为 `web-0` 的 Pod 在 `default` 命名空间中，通过 Headless Service `web-svc` 可被解析为：

```
web-0.web-svc.default.svc.cluster.local
```

这种命名稳定性对于有状态应用至关重要，即使 Pod 被重新调度到不同节点，其 DNS 名称保持不变。

## 与普通 Service 的对比

| 特性 | 普通 Service | Headless Service |
|------|-------------|------------------|
| ClusterIP | 自动分配 | `None` |
| DNS 解析 | 返回 ClusterIP | 返回 Pod IP 列表 |
| 负载均衡 | kube-proxy 负责 | 客户端自行处理 |
| 适用工作负载 | 无状态 Deployment | 有状态 StatefulSet |

## 远程顾问诊断要点

StatefulSet Pod 域名解析失败时，应按以下顺序排查：

- **确认 Headless Service 已创建**：检查与 StatefulSet 同名的 Service 是否存在，且 `clusterIP` 为 `"None"`
- **检查 Service selector**：确认 Service 的 `selector` 与 StatefulSet Pod 的标签匹配
- **检查 DNS 配置**：确认 CoreDNS 正常运行，`/etc/resolv.conf` 中的搜索域包含 `svc.cluster.local`
- **验证 DNS 解析**：在 Pod 内执行 `nslookup <pod-name>.<service-name>` 测试解析结果
- **检查网络策略**：确认没有 NetworkPolicy 阻止 DNS 查询或 Pod 间通信

更多有状态应用排错方法请参考 [[19-故障诊断/04-高级排障/structural-05-workloads/03-statefulset-troubleshooting.md|statefulset-troubleshooting]]，服务发现相关内容参见 [[22-概念/03-网络/service.md|service-discovery]]。

## 源码实现分析

### kube-proxy 对 Headless Service 的处理

```go
// k8s.io/kubernetes/pkg/proxy/config/config.go
// kube-proxy 监听 Service 变更，对 clusterIP=None 的 Service 跳过 iptables/ipvs 规则创建
func (c *ServiceConfig) handleAddService(service *v1.Service) {
    if service.Spec.ClusterIP == v1.ClusterIPNone {
        // Headless Service: 不创建虚拟 IP 规则
        // DNS 直接返回 Pod IP，无需 kube-proxy 转发
        klog.V(4).InfoS("Skipping headless service", "service", service.Name)
        return
    }
    // 普通 Service: 创建 iptables/ipvs 负载均衡规则
    c.eventHandler.OnServiceAdd(service)
}
```

### CoreDNS 对 Headless Service 的解析

```go
// coredns/plugin/kubernetes/kubernetes.go
// CoreDNS 对 Headless Service 返回所有 Endpoint Pod IP
func (k *Kubernetes) ServeDNS(ctx context.Context, w dns.ResponseWriter, r *dns.Msg) {
    // 解析 <svc>.<ns>.svc.cluster.local
    if svc.Spec.ClusterIP == "None" {
        // Headless: 返回所有 Endpoints 的 A 记录
        endpoints := k.APIConn.Endpoints(svc)
        for _, ep := range endpoints {
            for _, addr := range ep.Addresses {
                // 每个 Pod IP 生成一条 A 记录
                records = append(records, dns.A{A: net.ParseIP(addr.IP)})
            }
        }
    }
}
```

### 架构示意

```
┌─────────────────────────────────────────────────────────┐
│                    DNS 查询流程                           │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Pod (client)                                           │
│    │  nslookup web-0.web-svc.default.svc.cluster.local  │
│    ▼                                                    │
│  CoreDNS                                                │
│    │  查询 Service "web-svc" → clusterIP=None           │
│    │  查询 Endpoints → [10.244.1.5, 10.244.2.8, ...]   │
│    ▼                                                    │
│  返回 A 记录: 10.244.1.5 (web-0)                        │
│                                                         │
│  对比普通 Service:                                       │
│  Pod → CoreDNS → 返回 ClusterIP 10.96.0.100            │
│       → kube-proxy iptables → 随机选择 Pod IP           │
└─────────────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：StatefulSet 数据库集群（🟢 只读配置）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: mysql
  labels:
    app: mysql
spec:
  clusterIP: "None"
  ports:
  - port: 3306
    name: mysql
  selector:
    app: mysql
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  serviceName: mysql  # 必须引用 Headless Service
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-secret
              key: password
```

### 场景二：集群初始化节点发现（🟡 需配置 publishNotReadyAddresses）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: etcd-discovery
spec:
  clusterIP: "None"
  publishNotReadyAddresses: true  # 允许未 Ready Pod 被发现
  ports:
  - port: 2380
    name: peer
  selector:
    app: etcd
# 用途：etcd 集群启动时，所有节点需要互相发现
# 即使某些节点尚未 Ready，也必须能通过 DNS 解析到
```

### 场景三：客户端自定义负载均衡（🟢 应用层控制）

```bash
# 在 Pod 内查询所有后端实例
kubectl exec -it client-pod -- sh -c '
  # DNS 返回所有 Pod IP
  for ip in $(getent hosts my-headless-svc.default.svc.cluster.local | awk "{print \$1}"); do
    echo "Backend: $ip"
    # 应用层可实现：一致性哈希、加权轮询、最少连接等策略
    curl -s http://$ip:8080/health
  done
'
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| Headless Service 没有用 | 它是 StatefulSet 的必要组件 |
| clusterIP: None 就是禁用 | None 表示不分配 VIP，DNS 直接返回 Pod IP |
| 可以用普通 Service 替代 | 普通 Service 无法提供稳定的 Pod DNS 标识 |
| publishNotReadyAddresses 总是开启 | 仅集群初始化场景需要，正常情况应关闭 |
| Headless Service 没有负载均衡 | 客户端可通过 DNS 轮询实现简单负载均衡 |

## 面试要点

1. **Headless Service 与普通 Service 的核心区别？**
   - 普通 Service: 分配 ClusterIP，kube-proxy 负载均衡
   - Headless Service: 无 ClusterIP，DNS 直接返回 Pod IP 列表

2. **为什么 StatefulSet 需要 Headless Service？**
   - 提供稳定的网络标识: pod-0.svc.ns.svc.cluster.local
   - Pod 重建后 DNS 名称不变
   - 支持节点间直接通信

3. **publishNotReadyAddresses 的作用？**
   - 允许 DNS 返回未 Ready 的 Pod IP
   - 用于集群初始化时节点发现
   - 如 Cassandra/etcd 集群启动

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
