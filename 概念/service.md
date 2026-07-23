---
title: Service
summary: Kubernetes Service 是一种抽象，定义了一组 Pod 的逻辑集合和一个访问它们的策略。
category: concepts
tags:
- core-concept
- k8s
- networking
- visibility/public
tier: supporting
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---


# Service

## 概述

Kubernetes Service 是一种将一组逻辑相关的 Pod 暴露为网络服务的抽象。由于 Pod 是易变的（IP 随重建变化、因伸缩增减），直接依赖 Pod IP 既不现实也不稳定。Service 为一组（通过 label selector 匹配的）Pod 提供一个稳定的 ClusterIP、DNS 名称和负载均衡，使调用方无需关心后端 Pod 的变化。

## 架构与工作原理

```
  客户端 (Pod/外部)
        │
        ▼
  ┌───────────────────────┐
  │ Service (ClusterIP)   │   稳定 VIP + DNS: webapp.ns.svc.cluster.local
  │ selector: app=webapp  │
  └──────────┬────────────┘
             │ kube-proxy 维护转发规则
             ▼
  ┌──────── Endpoints / EndpointSlice ───────────┐
  │  10.244.1.5:8080   10.244.2.9:8080   10.244.3.4:8080 │
  └──────────────────────────────────────────────┘
             │
             ▼  （iptables / IPVS / eBPF）
        Pod 副本们
```

**工作流**：
1. Service 通过 `selector` 匹配到对应 Pod，Endpoints Controller 持续维护 Endpoints / EndpointSlice 列表（仅健康且 readiness 通过的 Pod）。
2. kube-proxy 在每个节点上监听 Service/Endpoint 变化，把 VIP 流量通过 **iptables**（默认）或 **IPVS** 规则 DNAT 到后端 Pod。
3. 集群内 DNS（CoreDNS）自动为每个 Service 生成 A/AAAA/SRV 记录，形如 `<service>.<namespace>.svc.cluster.local`。

**Service 类型（spec.type）**：

| 类型 | 暴露范围 | 特点 |
|------|----------|------|
| `ClusterIP`（默认） | 集群内 | 分配内部 VIP，仅集群可达 |
| `NodePort` | 集群外（节点 IP:Port） | 在所有节点开 30000-32767 端口 |
| `LoadBalancer` | 公网 | 由云厂商创建负载均衡器并回调 Service |
| `ExternalName` | CNAME 别名 | 将集群内 DNS 指向外部域名，无代理 |

## 关键组件与特性

| 特性 | 说明 |
|------|------|
| label selector | 通过标签匹配后端 Pod，支持 `matchLabels` / `matchExpressions` |
| ClusterIP | 集群内稳定虚拟 IP（可设 `None` 创建 Headless Service） |
| Endpoints / EndpointSlice | 实际后端 Pod 列表，EndpointSlice 支持大规模扩展 |
| sessionAffinity | ClientIP 会话保持，默认 None |
| externalTrafficPolicy | Cluster（默认）/ Local，Local 保留源 IP 但需逐节点有 Pod |
| multi-port | 一个 Service 暴露多端口，每端口独立 name + targetPort |
| topology aware hints | 按拓扑（区域/节点）优先路由同地域后端 |

## 配置示例

```yaml
---
apiVersion: v1
kind: Service
metadata:
  name: webapp
  namespace: production
  labels:
    app: webapp
spec:
  type: ClusterIP
  selector:
    app: webapp
  ports:
  - name: http
    port: 80
    targetPort: 8080
    protocol: TCP
  - name: metrics
    port: 9090
    targetPort: metrics
  sessionAffinity: None
---
# Headless Service：StatefulSet 常用，每个 Pod 有独立 DNS
apiVersion: v1
kind: Service
metadata:
  name: db-headless
  namespace: production
spec:
  clusterIP: None
  selector:
    app: postgres
  ports:
  - port: 5432
---
# LoadBalancer：对外暴露
apiVersion: v1
kind: Service
metadata:
  name: webapp-lb
  annotations:
    service.beta.kubernetes.io/azure-load-balancer-internal: "false"
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local
  selector:
    app: webapp
  ports:
  - port: 443
    targetPort: 8443
```

## 常用操作与命令

```bash
# 查看 Service 及其后端
kubectl get svc -n production
kubectl describe svc webapp
kubectl get endpoints webapp -n production
kubectl get endpointslices -n production -l kubernetes.io/service-name=webapp

# DNS 解析
kubectl run dns-test --image=busybox:1.36 -it --rm --restart=Never -- \
  nslookup webapp.production.svc.cluster.local

# 临时从集群内访问
kubectl run curl --image=curlimages/curl -it --rm --restart=Never -- \
  curl http://webapp.production:80/healthz

# 端口转发到本地
kubectl port-forward svc/webapp 8080:80 -n production
```

## 最佳实践

1. **优先使用 ClusterIP + Ingress**：对外暴露交给 Ingress/网关，NodePort/LoadBalancer 只在必要时使用。
2. **使用 readinessProbe 保证 Endpoints 准确**：只有就绪的 Pod 才进 Endpoints，避免流量打到未就绪实例。
3. **多端口务必命名**：`port.name` 便于协议推断（Prometheus 抓 metrics、Istion 做 mTLS）。
4. **保留源 IP 用 externalTrafficPolicy: Local**：代价是流量只在本节点后端间转发，需保证每节点有 Pod。
5. **Headless + StatefulSet**：有状态服务用 `clusterIP: None` 获取每副本稳定 DNS（`pod-0.svc.ns.svc`）。

## 常见陷阱

- **Service 无 Endpoints**：selector 拼写与 Pod label 不一致，或 Pod readiness 持续失败。
- **DNS 解析失败**：CoreDNS Pod 异常或 ndots 配置导致超长解析，检查 `/etc/resolv.conf`。
- **NodePort 源 IP 被 SNAT**：默认 externalTrafficPolicy=Cluster 会做 SNAT，丢失真实客户端 IP。
- **会话保持导致负载不均**：sessionAffinity=ClientIP 在 NAT 后地址集中时流量倾斜。
- **Service 与 Mesh 冲突**：启用 Istio/Linkerd 时注意协议探测，建议显式声明端口 name（http/tcp）。

## 源码实现分析

### EndpointSlice Controller

```go
// k8s.io/kubernetes/pkg/controller/endpointslice/endpointslice_controller.go
// EndpointSlice Controller 监听 Pod 变化，更新 EndpointSlice
func (c *Controller) syncPod(ctx context.Context, key string) error {
    // 1. 获取 Pod 信息
    pod := c.getPod(key)
    // 2. 查找匹配的 Service（通过 selector）
    services := c.getMatchingServices(pod)
    for _, svc := range services {
        // 3. 检查 Pod 是否 Ready
        ready := isPodReady(pod)
        // 4. 更新 EndpointSlice
        slice := c.getOrCreateEndpointSlice(svc)
        if ready {
            slice.Endpoints = append(slice.Endpoints, v1.Endpoint{
                Addresses: []string{pod.Status.PodIP},
                Conditions: v1.EndpointConditions{Ready: &ready},
                NodeName:   &pod.Spec.NodeName,
                Zone:       &pod.Labels["topology.kubernetes.io/zone"],
            })
        }
        c.client.Update(ctx, slice)
    }
}
```

### Service 类型与数据路径

```
┌───────────────────────────────────────────────────────────┐
│          Service 类型与数据路径                        │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  ClusterIP (默认):                                       │
│  Pod → ClusterIP:port → kube-proxy → Pod IP:targetPort │
│                                                           │
│  NodePort:                                               │
│  外部 → NodeIP:30000-32767 → kube-proxy → Pod IP     │
│                                                           │
│  LoadBalancer:                                           │
│  外部 → 云 LB → NodePort → kube-proxy → Pod IP       │
│                                                           │
│  ExternalName:                                           │
│  Pod → DNS CNAME → 外部域名 (无 kube-proxy)          │
│                                                           │
│  Headless (clusterIP: None):                             │
│  Pod → DNS A记录 → 直接返回所有 Pod IP              │
│                                                           │
│  关键组件:                                               │
│  • EndpointSlice Controller: Pod → EndpointSlice       │
│  • kube-proxy: Service → iptables/IPVS 规则          │
│  • CoreDNS: Service 名 → ClusterIP 解析             │
└───────────────────────────────────────────────────────────┘
```

### 生产配置示例（🟡 部署到集群）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: web-app
  annotations:
    # 云 LB 注解（AWS 示例）
    service.beta.kubernetes.io/aws-load-balancer-type: nlb
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local  # 保留源 IP
  selector:
    app: web-app
  ports:
  - name: http
    port: 80
    targetPort: 8080
    protocol: TCP
  - name: metrics
    port: 9090
    targetPort: 9090
  sessionAffinity: None
```

## 面试要点

1. **Service 的四种类型及适用场景？**
   - ClusterIP：内部服务间通信（默认）
   - NodePort：无云 LB 的外部访问（开发/测试）
   - LoadBalancer：生产环境外部流量
   - ExternalName：外部服务 DNS 别名

2. **Service 如何发现后端 Pod？**
   - EndpointSlice Controller 监听 Pod 变化
   - 通过 Service selector 匹配 Pod labels
   - 只有 Ready 的 Pod 才加入 Endpoints

3. **externalTrafficPolicy Local vs Cluster？**
   - Cluster：流量可跨节点转发，会 SNAT 丢失源 IP
   - Local：只转发到本节点 Pod，保留源 IP
   - Local 需保证每节点有 Pod，否则流量丢失

4. **Service 与 Ingress 的区别？**
   - Service：L4（TCP/UDP）负载均衡
   - Ingress：L7（HTTP）路由、TLS 终止、路径匹配
   - 生产：Service + Ingress 组合使用

## 相关概念

- [[概念/kubernetes.md|Kubernetes]]
- [[概念/pods.md|Pod]] — Service 的后端
- [[概念/ingress.md|Ingress]] — 七层入口
- [[概念/network-policy.md|NetworkPolicy]] — 流量控制
- [[概念/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
