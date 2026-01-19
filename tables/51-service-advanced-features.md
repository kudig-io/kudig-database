# 124 - Service 高级特性与应用案例

本文档聚焦于 Kubernetes Service 的高级特性，这些特性对于优化生产环境中的流量路由、保证应用高可用性以及降低网络延迟至关重要。

---

## 1. `externalTrafficPolicy`: 保持客户端源 IP

在使用 `NodePort` 或 `LoadBalancer` 类型的 Service 时，默认情况下，流量在被转发到后端 Pod 之前会经过一次源地址转换 (SNAT)，这会导致后端应用无法获取到真实的客户端 IP 地址。`externalTrafficPolicy` 字段就是为了解决这个问题。

| 策略 | `externalTrafficPolicy: Cluster` (默认) | `externalTrafficPolicy: Local` |
|:---|:---|:---|
| **流量路径** | 流量可以从任一节点转发到集群中任一节点上的 Pod。 | 流量只会被转发到**当前节点**上的 Pod。 |
| **优点** | - **负载均衡更均匀**：流量可以在所有后端 Pod 之间平均分配。 | - **保留客户端源 IP**：后端应用可以直接获取到真实的客户端 IP。 |
| **缺点** | - **丢失源 IP**：后端应用看到的源 IP 是数据包离开节点的 IP。 | - **可能导致负载不均**：如果 Pod 分布不均，某些节点可能会接收到更多流量。 <br> - **需要额外的健康检查**：云厂商的 LoadBalancer 需要知道哪些节点上有健康的 Pod，这通常由 `kube-proxy` 实现。 |
| **适用场景** | - 对源 IP 没有要求的应用。 <br> - 优先保证负载均衡的场景。 | - 需要根据客户端 IP 做访问控制、审计或地理位置分析的应用。 <br> - Web 服务器、API 网关等。 |

**实操案例**:
为一个需要记录真实访客 IP 的 Nginx 服务开启源 IP 保留。

```yaml
apiVersion: v1
kind: Service
metadata:
  name: nginx-preserve-ip
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local # 关键配置
  selector:
    app: nginx
  ports:
    - protocol: TCP
      port: 80
      targetPort: 80
```
**专家提示**: 当使用 `Local` 策略时，如果某个节点上没有健康的后端 Pod，发送到该节点 `NodePort` 的流量将会被丢弃。因此，确保 Pod 在节点间均匀分布（例如使用 `podAntiAffinity`）非常重要。

---

## 2. `sessionAffinity`: 会话亲和性

会话亲和性 (Session Affinity)，也称为“粘性会话”(Sticky Session)，可以确保来自同一个客户端的请求始终被转发到同一个后端 Pod。

| `sessionAffinity` | 描述 |
|:---:|:---|
| **`None` (默认)** | 不启用会话亲和性，请求被随机或轮询分发。 |
| **`ClientIP`** | 基于客户端的 IP 地址来保持会话。在一段时间内（由 `sessionAffinityConfig` 定义），来自同一 IP 的请求会发往同一个 Pod。 |

**应用场景**:
- 需要在内存中缓存用户状态的应用，例如一些传统的 Web 应用或有状态的应用。
- 减少后端 Pod 之间状态同步的开销。

**YAML 示例**:
```yaml
apiVersion: v1
kind: Service
metadata:
  name: sticky-service
spec:
  type: ClusterIP
  sessionAffinity: ClientIP # 启用基于客户端 IP 的会話亲和性
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800 # 会话保持超时时间，默认为 10800 秒 (3 小时)
  selector:
    app: my-stateful-app
  ports:
    - port: 80
      targetPort: 8080
```

**注意事项**:
- `ClientIP` 亲和性对客户端 IP 的识别能力依赖于网络拓扑。如果客户端经过了 NAT 网关或 HTTP 代理，多个客户端可能表现为同一个 IP，导致负载集中到单个 Pod。
- 它不能保证绝对的会话保持，因为当后端 Pod 重启或被重新调度时，会话会中断。对于需要强一致性会话的场景，应考虑使用应用层的解决方案（如 Redis 缓存 Session）。

---

## 3. 拓扑感知路由 (Topology Aware Routing)

拓扑感知路由是一个高级特性，旨在将服务流量优先路由到与客户端位于同一拓扑区域（如同一可用区、同一机架）的端点。这可以显著**降低网络延迟**和**跨区流量成本**。

**工作原理** (以可用区为例):
1.  **启用特性门控**: 需要在 `kube-apiserver` 和 `kube-proxy` 上启用 `TopologyAwareRouting` 特性门控。
2.  **Service 注解**: 在 Service 上添加 `service.kubernetes.io/topology-mode: Auto` 注解。
3.  **EndpointSlice 计算**: Kubernetes 控制平面会分析每个区域 (Zone) 的端点分布和容量。
4.  **下发拓扑子集**: 如果条件满足（例如，每个区域都有足够数量的健康端点），控制平面会为每个区域的 `kube-proxy` 生成一个只包含该区域内端点的 `EndpointSlice` 子集。
5.  **区域内路由**: 节点上的 `kube-proxy` 只会将流量路由到本区域内的 Pod，从而避免了昂贵的跨区调用。如果本区域端点全部失败，它会自动故障转移 (failover) 到其他区域。

**YAML 示例**:
```yaml
apiVersion: v1
kind: Service
metadata:
  name: topology-aware-svc
  annotations:
    service.kubernetes.io/topology-mode: Auto # 关键注解
spec:
  type: ClusterIP
  selector:
    app: my-app
  ports:
    - port: 80
      targetPort: 80
```

**适用场景**:
- 部署在多个可用区（AZ）的大型、延迟敏感型应用。
- 希望优化云厂商跨区流量成本的场景。

---

## 4. 无选择器的 Service (Services without selectors)

通常，Service 通过 `selector` 来自动发现和关联后端 Pod。但有时，我们希望手动管理 Service 的端点，这时就可以创建一个不带 `selector` 的 Service。

**工作原理**:
- 如果 Service 没有 `selector`，Kubernetes 不会自动创建 `EndpointSlice` 对象。
- 你必须手动创建一个与 Service 同名的 `EndpointSlice` (或旧版的 `Endpoints`) 对象，并在其中指定后端的 IP 地址和端口。

**实操案例**: 将集群内的服务平滑迁移到 Kubernetes
假设你有一个外部的数据库集群，你想让集群内的应用通过一个稳定的 Service 名称 (`external-db`) 来访问它，而不是硬编码 IP 地址。

**Step 1: 创建无选择器的 Service**
```yaml
apiVersion: v1
kind: Service
metadata:
  name: external-db # Service 名称
spec:
  ports:
    - protocol: TCP
      port: 3306
      targetPort: 3306
# 注意：这里没有 selector
```

**Step 2: 手动创建 EndpointSlice**
```yaml
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: external-db-slice # 名称不重要，但标签必须匹配
  labels:
    kubernetes.io/service-name: external-db # 关键标签，将此 Slice 与 Service 关联
addressType: ipv4
ports:
  - name: ''
    appProtocol: tcp
    protocol: TCP
    port: 3306
endpoints:
  - addresses:
      - "192.168.10.1" # 外部数据库主库 IP
  - addresses:
      - "192.168.10.2" # 外部数据库从库 IP
```

现在，集群内的应用只需要访问 `external-db:3306`，Kubernetes 就会将流量负载均衡到 `192.168.10.1` 和 `192.168.10.2`。当数据库完全迁移到集群内后，只需修改 `external-db` Service，为其添加 `selector`，并删除手动的 `EndpointSlice` 即可，应用代码无需任何改动。
