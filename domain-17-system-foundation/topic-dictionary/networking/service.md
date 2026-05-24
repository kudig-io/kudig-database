---
summary: "Service 是 [[Kubernetes|Kubernetes]] 中用于将运行在一组 Pod 上的网络应用暴露给集群内外的核心抽象对象。由于 Pod 是临时的、会被动态创建和销毁的，其 IP 地址也随之变化，Service 通过稳定的虚拟 IP（ClusterIP）和 DNS 名称，解耦了前端客户端与后端 Pod 的耦合，使现有应用无需改造即可在 Kubernetes 中运行。"
title: Service
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- statefulset
- ingress
- gateway
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Service 是什么
- 如何 Service
trigger_keywords:
- Service
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
created: "2026-05-23"
---

# Service

## 概述

Service 是 [[Kubernetes|Kubernetes]] 中用于将运行在一组 Pod 上的网络应用暴露给集群内外的核心抽象对象。由于 Pod 是临时的、会被动态创建和销毁的，其 IP 地址也随之变化，Service 通过稳定的虚拟 IP（ClusterIP）和 DNS 名称，解耦了前端客户端与后端 Pod 的耦合，使现有应用无需改造即可在 Kubernetes 中运行。

## 核心概念/原理

- **Selector 与 EndpointSlice**：Service 通过 `selector` 匹配标签相同的 Pod，控制平面自动创建并维护对应的 EndpointSlice，记录所有后端 Pod 的 IP 与端口。无 selector 的 Service 可配合手动创建的 EndpointSlice，将流量转发到集群外部地址或其他命名空间。
- **端口映射**：Service 的 `port` 是暴露的端口，`targetPort` 是 Pod 容器的实际监听端口，支持按名称引用容器端口，便于后端升级时平滑切换。
- **Headless Service**：将 `.spec.clusterIP` 显式设为 `"None"`，不再分配虚拟 IP，DNS 直接返回后端 Pod 的 IP 列表（A/AAAA 记录），适用于需要直接访问特定 Pod 或有状态服务场景。
- **服务发现**：集群内的 Pod 可通过环境变量（创建顺序有要求）或 DNS 发现 Service，推荐使用 DNS 方式以避免依赖启动顺序。

## 关键机制或特性

- **Service 类型**：
  - `ClusterIP`（默认）：集群内部可访问的虚拟 IP。
  - `NodePort`：在每个节点上开放固定端口（默认 30000–32767），将流量代理到 Service。
  - `LoadBalancer`：在云厂商环境中自动创建外部负载均衡器。
  - `ExternalName`：通过 DNS CNAME 将 Service 映射到外部域名，不做任何代理。
- **[[EndpointSlices|EndpointSlices]]**：自 v1.21 起稳定，是 kube-proxy 路由内部流量的真实来源，默认每个 Slice 最多 100 个端点（最大可配 1000）。旧版 Endpoints API 已弃用。
- **流量策略**：支持 `internalTrafficPolicy` 与 `externalTrafficPolicy`（Cluster/Local），控制流量在集群内部或外部进入时的路由范围。
- **会话保持（Session Affinity）**：可基于客户端 IP 配置会话亲和性，使同一客户端流量始终到达同一 Pod。
- **应用协议（appProtocol）**：自 v1.20 起稳定，用于为端口声明应用层协议（如 `kubernetes.io/h2c`、`kubernetes.io/ws`），供实现方提供更丰富的行为。

## 使用场景

- **微服务间通信**：通过 ClusterIP + DNS 实现服务间稳定调用。
- **外部访问入口**：使用 NodePort 或 LoadBalancer 将 Web 应用暴露到公网。
- **连接集群外服务**：利用无 selector Service + 手动 EndpointSlice 或 ExternalName 对接外部数据库、 legacy 系统。
- **有状态服务发现**：Headless Service 配合 [[StatefulSet|StatefulSet]]，为每个 Pod 提供独立 DNS 记录。

## 最佳实践/注意事项

- **优先使用 DNS 发现**：相比环境变量，DNS 不依赖 Pod 与 Service 的创建顺序，更灵活可靠。
- **无 selector Service 需手动维护 EndpointSlice**：创建或更新 EndpointSlice 时，避免使用 loopback、link-local 或其他 Service 的 ClusterIP 作为 endpoint 地址。
- **NodePort 端口冲突**：可指定 `nodePort` 使用静态段（默认 30000–30085）以降低冲突概率；动态分配使用 30086–32767。
- **LoadBalancer IP 弃用**：`.spec.loadBalancerIP` 在 v1.24 已弃用，建议改用云厂商特定的注解或迁移到 Gateway API。
- **ExternalName 的协议兼容性**：对 HTTP/HTTPS 等依赖 Host 头的协议，ExternalName 可能导致 TLS 证书不匹配或 Host 头错误，需谨慎使用。

## 生产 YAML 示例

### 各类型 Service 对照

```yaml
# 1. ClusterIP（默认 — 集群内部访问）
apiVersion: v1
kind: Service
metadata:
  name: backend-api
  namespace: production
spec:
  type: ClusterIP
  selector:
    app: backend-api
  ports:
  - name: http
    port: 80
    targetPort: 8080
    protocol: TCP
    appProtocol: http             # 声明应用协议
  - name: grpc
    port: 9090
    targetPort: 9090
    protocol: TCP
    appProtocol: kubernetes.io/h2c
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800       # 会话保持 3 小时
---
# 2. Headless Service（StatefulSet 直接 Pod DNS）
apiVersion: v1
kind: Service
metadata:
  name: cassandra
  namespace: data
spec:
  clusterIP: None                  # Headless
  selector:
    app: cassandra
  ports:
  - port: 9042
# DNS 返回所有 Pod IP：cassandra-0.cassandra.data.svc.cluster.local
---
# 3. NodePort（开发/测试暴露）
apiVersion: v1
kind: Service
metadata:
  name: web-nodeport
  namespace: staging
spec:
  type: NodePort
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 8080
    nodePort: 30080                # 静态端口（可选）
---
# 4. LoadBalancer（云环境生产暴露）
apiVersion: v1
kind: Service
metadata:
  name: web-public
  namespace: production
  annotations:
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    service.beta.kubernetes.io/aws-load-balancer-scheme: "internet-facing"
spec:
  type: LoadBalancer
  selector:
    app: web
  ports:
  - port: 443
    targetPort: 8443
  externalTrafficPolicy: Local     # 保留源 IP
---
# 5. ExternalName（CNAME 映射外部服务）
apiVersion: v1
kind: Service
metadata:
  name: legacy-db
  namespace: production
spec:
  type: ExternalName
  externalName: db.legacy.example.com    # DNS CNAME
---
# 6. 无 Selector Service + 手动 EndpointSlice（对接外部地址）
apiVersion: v1
kind: Service
metadata:
  name: external-payment
  namespace: production
spec:
  ports:
  - port: 443
    targetPort: 443
```

## Service 类型决策树

```
需要暴露到集群外部？
  │
  ├─ 否 → ClusterIP（默认）
  │       └─ 需要直接访问 Pod？ → Headless (clusterIP: None)
  │
  └─ 是 → 有云 LB？
          │
          ├─ 是 → LoadBalancer
          │       └─ 需要保留源 IP？ → externalTrafficPolicy: Local
          │
          └─ 否 → NodePort
                  └─ 仅做 DNS 映射？ → ExternalName
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Service ClusterIP 无响应 | 无后端 Pod 或 Pod 未 Ready | `kubectl get endpointslices -l kubernetes.io/service-name=<svc>` |
| LoadBalancer 一直 Pending | 云控制器未安装或配额不足 | `kubectl describe svc <name>` 查看 Events |
| ExternalName 解析失败 | DNS CNAME 链不通或 TLS 证书不匹配 | `kubectl exec -- nslookup <external-name>` |
| NodePort 无法从外部访问 | 防火墙/安全组未开放端口 | 检查节点安全组规则（30000-32767） |
| 会话保持不生效 | `sessionAffinity` 未配置或超时过短 | `kubectl get svc -o yaml` 检查 sessionAffinity 字段 |

## 生产检查清单

- [ ] 生产 Service 使用 DNS 服务发现（而非环境变量）
- [ ] LoadBalancer Service 配置 `externalTrafficPolicy` 根据需求
- [ ] 无 selector Service 手动维护 EndpointSlice
- [ ] NodePort 范围由集群管理员统一规划
- [ ] 避免 ExternalName 用于依赖 Host 头的 HTTP/HTTPS 服务
- [ ] 使用 `appProtocol` 声明端口协议便于控制器优化

## 命令快速参考

```bash
# 查看 Service 列表
kubectl get svc -n production -o wide

# 查看 Service 详情
kubectl describe svc backend-api -n production

# 查看 Service Endpoints
kubectl get endpointslices -l kubernetes.io/service-name=backend-api

# 从集群内测试 Service
kubectl run test --rm -it --image=busybox -- wget -qO- http://backend-api.production.svc:80

# 查看 NodePort 分配
kubectl get svc -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.type}{"\t"}{.spec.ports[*].nodePort}{"\n"}{end}'

# 临时端口转发
kubectl port-forward svc/backend-api 8080:80 -n production
```

## 交叉引用

- [EndpointSlices](endpointslices.md) — Service 后端端点的管理和条件
- [DNS for Services](dns-for-services-and-[[Pods|pods]].md) — Service DNS 记录格式
- [Service ClusterIP Allocation](service-clusterip-allocation.md) — ClusterIP 分配策略
- [Service Internal Traffic Policy](service-internal-traffic-policy.md) — 内部流量节点本地路由
- [Ingress](ingress.md) — HTTP/HTTPS 层的 Service 暴露
- [Gateway API](gateway-api.md) — 下一代 Service 暴露方案

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/service/

## Related

- [[domain-19-landscape-references/topic-index/dns-index|DNS 知识图谱索引]]
