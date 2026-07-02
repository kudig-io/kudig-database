---
title: 08 - Service 全类型 YAML 配置参考
description: 'title: 08 - Service 全类型 YAML 配置参考'
summary: 'title: 08 - Service 全类型 YAML 配置参考'
category: general
tags:
- yaml
- reference
- apiserver
- controller-manager
- prometheus
- istio
- cilium
- coredns
- helm
- opa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 45min
intent_queries:
- Service是什么？
- 如何使用Service？
- Service的最佳实践是什么？
trigger_keywords:
- Service
- 全类型
- YAML
- 配置参考
- manifests
- patterns
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- cilium-basics
- redis-basics
- mysql-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 08 - [[Service|Service]] 全类型 YAML 配置参考
description: '# 08 - Service 全类型 YAML 配置参考'
category: yaml-manifests
tags:
- k8s
- yaml
- manifest
- template
- apiserver
- controller-manager
- [[Prometheus|prometheus]]
- [[Istio|istio]]
- [[Cilium|cilium]]
- coredns
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 10min
intent_queries:
- Service 全类型 YAML 配置参考 是什么
- 如何 Service 全类型 YAML 配置参考
- Kubernetes 32 yaml manifests 最佳实践
trigger_keywords:
- Service
- 全类型
- YAML
- 配置参考
- yaml
- manifests
cross_refs:
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/service-fta.md
  label: '故障树: service'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 08 - Service 全类型 YAML 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02

<!-- chunk: 概述 -->## 概述

Service 是 Kubernetes 中用于暴露应用程序的核心抽象资源，它为一组 Pod 提供稳定的网络端点和负载均衡能力。Service 通过标签选择器（selector）来定位后端 Pod，并为客户端提供统一的访问入口。

**四种核心类型**：
- **ClusterIP**：集群内部访问（默认类型）
- **NodePort**：通过节点 IP + 静态端口暴露
- **LoadBalancer**：云环境中自动分配外部负载均衡器
- **ExternalName**：返回 CNAME 记录的 DNS 别名

**主要用途**：
- 为动态变化的 Pod 集合提供稳定的网络标识
- 在多个后端 Pod 之间实现负载均衡
- 服务发现与 DNS 集成
- 跨命名空间的服务访问
- 外部服务的内部抽象

---

<!-- chunk: API 信息 -->## API 信息

| API Group | API Version | Kind    | 稳定性 |
|-----------|-------------|---------|--------|
| core      | v1          | Service | GA     |

**完整 API 路径**：
```
GET /api/v1/namespaces/{namespace}/services/{name}
```

**缩写**：`svc`

**命名空间作用域**：是

---

<!-- chunk: 完整字段规格表 -->## 完整字段规格表

## 核心字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 | 版本要求 |
|---------|------|------|--------|------|----------|
| `metadata.name` | string | 是 | - | Service 名称，作为 DNS 名称的一部分 | v1.0+ |
| `metadata.namespace` | string | 否 | default | 命名空间 | v1.0+ |
| `metadata.labels` | map[string]string | 否 | - | 标签集合 | v1.0+ |
| `metadata.annotations` | map[string]string | 否 | - | 注解，常用于云提供商配置 | v1.0+ |
| `spec.selector` | map[string]string | 否 | - | Pod 标签选择器（Headless/ExternalName 可省略） | v1.0+ |
| `spec.type` | string | 否 | ClusterIP | Service 类型 | v1.0+ |
| `spec.ports[]` | array | 是 | - | 端口配置列表 | v1.0+ |
| `spec.clusterIP` | string | 否 | 自动分配 | 集群内部 IP，可设为 None（Headless） | v1.0+ |
| `spec.clusterIPs` | []string | 否 | - | 多 IP 地址（双栈场景） | v1.20+ |
| `spec.externalIPs` | []string | 否 | - | 外部 IP 列表（手动指定） | v1.0+ |
| `spec.sessionAffinity` | string | 否 | None | 会话亲和性（None/ClientIP） | v1.0+ |
| `spec.sessionAffinityConfig` | object | 否 | - | 会话亲和性配置 | v1.7+ |
| `spec.ipFamilies` | []string | 否 | - | IP 协议族（IPv4/IPv6） | v1.20+ |
| `spec.ipFamilyPolicy` | string | 否 | SingleStack | IP 族策略 | v1.20+ |
| `spec.publishNotReadyAddresses` | bool | 否 | false | 是否发布未就绪地址 | v1.9+ |
| `spec.internalTrafficPolicy` | string | 否 | Cluster | 内部流量策略 | v1.26+ |
| `spec.externalTrafficPolicy` | string | 否 | Cluster | 外部流量策略 | v1.4+ |
| `spec.allocateLoadBalancerNodePorts` | bool | 否 | true | LoadBalancer 是否分配 NodePort | v1.24+ |
| `spec.loadBalancerClass` | string | 否 | - | 负载均衡器类别 | v1.24+ |
| `spec.loadBalancerIP` | string | 否 | - | 指定负载均衡器 IP（已废弃） | v1.0-v1.24 |
| `spec.loadBalancerSourceRanges` | []string | 否 | - | 允许访问 LB 的源 CIDR | v1.0+ |

## spec.ports[] 字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 |
|---------|------|------|--------|------|
| `name` | string | 否* | - | 端口名称（多端口时必需） |
| `protocol` | string | 否 | TCP | 协议（TCP/UDP/SCTP） |
| `port` | int32 | 是 | - | Service 暴露的端口 |
| `targetPort` | int/string | 否 | port | Pod 上的目标端口（可用名称） |
| `nodePort` | int32 | 否 | 自动分配 | NodePort 类型的节点端口（30000-32767） |
| `appProtocol` | string | 否 | - | 应用层协议（http/https/grpc等） |

\* 多端口 Service 必须为每个端口提供唯一名称

---

<!-- chunk: ClusterIP 详解 -->## ClusterIP 详解

## 标准 ClusterIP

**特点**：
- 默认类型，仅在集群内部可访问
- 自动分配一个虚拟 IP（从 Service CIDR 池中）
- 通过 kube-proxy 实现负载均衡
- 支持 DNS 解析：`<service-name>.<namespace>.svc.cluster.local`

**配置示例**：
```yaml
apiVersion: v1
kind: Service
metadata:
  name: backend-service
  namespace: production
  labels:
    app: backend
    env: prod
spec:
  type: ClusterIP  # 可省略，这是默认值
  selector:
    app: backend
    tier: api
  ports:
  - name: http        # 端口名称（多端口时必需）
    protocol: TCP     # 协议类型
    port: 80          # Service 端口
    targetPort: 8080  # Pod 容器端口
  - name: metrics
    protocol: TCP
    port: 9090
    targetPort: metrics  # 可使用 Pod 端口的名称
  sessionAffinity: None  # 会话亲和性：None 或 ClientIP
```

## Headless Service（无头服务）

**特点**：
- 设置 `clusterIP: None`
- 不分配虚拟 IP，不进行负载均衡
- DNS 直接返回所有 Pod IP 地址
- 常用于 StatefulSet，实现稳定的网络标识

**配置示例**：
```yaml
apiVersion: v1
kind: Service
metadata:
  name: mysql-headless
  namespace: database
spec:
  clusterIP: None  # 关键配置：设为 None 创建 Headless Service
  selector:
    app: mysql
  ports:
  - name: mysql
    port: 3306
    targetPort: 3306
  publishNotReadyAddresses: true  # 发布未就绪的 Pod 地址（用于主从复制初始化）
```

**DNS 解析行为**：
```bash
# 标准 Service：返回单个 Cluster IP
nslookup backend-service.production.svc.cluster.local
# 返回：10.96.10.20

# Headless Service：返回所有 Pod IP
nslookup mysql-headless.database.svc.cluster.local
# 返回：
# 10.244.1.5
# 10.244.2.8
# 10.244.3.12

# StatefulSet Pod 的稳定 DNS（Headless 特性）
nslookup mysql-0.mysql-headless.database.svc.cluster.local
# 返回：10.244.1.5
```

## 手动指定 ClusterIP

```yaml
apiVersion: v1
kind: Service
metadata:
  name: fixed-ip-service
spec:
  type: ClusterIP
  clusterIP: 10.96.100.50  # 手动指定（必须在 Service CIDR 范围内）
  selector:
    app: myapp
  ports:
  - port: 80
    targetPort: 8080
```

**注意事项**：
- IP 必须在 `--service-cluster-ip-range` 配置的 CIDR 内
- 避免 IP 冲突（手动管理复杂）
- 删除 Service 后 IP 才会释放

---

<!-- chunk: NodePort 详解 -->## NodePort 详解

**特点**：
- 在每个节点上打开一个静态端口（默认范围 30000-32767）
- 自动创建底层 ClusterIP Service
- 外部可通过 `<NodeIP>:<NodePort>` 访问
- 流量路径：NodePort → ClusterIP → Pod

**配置示例**：
```yaml
apiVersion: v1
kind: Service
metadata:
  name: web-nodeport
  namespace: frontend
spec:
  type: NodePort
  selector:
    app: nginx
  ports:
  - name: http
    protocol: TCP
    port: 80          # ClusterIP 端口
    targetPort: 80    # Pod 端口
    nodePort: 30080   # 节点端口（可省略让系统自动分配）
  externalTrafficPolicy: Local  # 流量策略：保留源 IP，减少跳转
```

## externalTrafficPolicy 详解

| 值 | 行为 | 源 IP | 负载均衡 | 健康检查 |
|----|------|-------|----------|----------|
| **Cluster** | 流量可跨节点转发 | SNAT，丢失真实源 IP | 全局负载均衡 | 检查所有节点 |
| **Local** | 仅转发到本地 Pod | 保留源 IP | 本地负载均衡 | 仅检查本地 Pod |

**Local 策略示例**：
```yaml
apiVersion: v1
kind: Service
metadata:
  name: external-api
spec:
  type: NodePort
  externalTrafficPolicy: Local  # 关键配置
  selector:
    app: api
  ports:
  - port: 443
    targetPort: 8443
    nodePort: 30443
```

**影响**：
- ✅ 优点：保留客户端源 IP，减少网络跳转，降低延迟
- ❌ 缺点：可能导致负载不均（只转发到有 Pod 的节点）
- 💡 使用场景：需要源 IP 的应用（如日志审计、地理位置限制）

## 自定义 NodePort 范围

修改 kube-apiserver 参数：
```bash
--service-node-port-range=20000-40000
```

---

<!-- chunk: LoadBalancer 详解 -->## LoadBalancer 详解

**特点**：
- 云环境中自动创建外部负载均衡器（AWS ELB、GCP LB、Azure LB）
- 自动创建底层 NodePort 和 ClusterIP Service
- 分配一个外部可访问的 IP 地址
- 流量路径：External LB → NodePort → ClusterIP → Pod

**基础配置**：
```yaml
apiVersion: v1
kind: Service
metadata:
  name: web-loadbalancer
  namespace: public
  annotations:
    # 云提供商特定注解（以 AWS 为例）
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"  # NLB 类型
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
    service.beta.kubernetes.io/aws-load-balancer-backend-protocol: "http"
spec:
  type: LoadBalancer
  selector:
    app: frontend
  ports:
  - name: http
    port: 80
    targetPort: 8080
  - name: https
    port: 443
    targetPort: 8443
  loadBalancerSourceRanges:  # 限制访问源（白名单）
  - 203.0.113.0/24
  - 198.51.100.0/24
  externalTrafficPolicy: Local  # 保留源 IP
```

## loadBalancerClass（v1.24+）

**用途**：指定特定的负载均衡器实现（支持多 LB 控制器）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: app-with-custom-lb
spec:
  type: LoadBalancer
  loadBalancerClass: example.com/custom-lb  # 自定义 LB 类别
  selector:
    app: myapp
  ports:
  - port: 80
    targetPort: 8080
```

**关联 LoadBalancerClass**：
```yaml
apiVersion: v1
kind: LoadBalancerClass
metadata:
  name: example.com/custom-lb
spec:
  controller: example.com/lb-controller
```

## allocateLoadBalancerNodePorts（v1.24+）

**用途**：禁用自动分配 NodePort（某些 LB 实现直接路由到 Pod）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: direct-lb-service
spec:
  type: LoadBalancer
  allocateLoadBalancerNodePorts: false  # 不分配 NodePort
  selector:
    app: myapp
  ports:
  - port: 80
    targetPort: 8080
```

**适用场景**：
- 使用支持直接路由的 LB（如 MetalLB、Cilium LB IPAM）
- 节省 NodePort 端口资源
- 减少网络跳转提高性能

## 云提供商注解参考

## AWS

```yaml
annotations:
  # 负载均衡器类型
  service.beta.kubernetes.io/aws-load-balancer-type: "nlb"  # nlb 或 elb（默认）
  
  # 内部负载均衡器（私有）
  service.beta.kubernetes.io/aws-load-balancer-internal: "true"
  
  # SSL 证书
  service.beta.kubernetes.io/aws-load-balancer-ssl-cert: "arn:aws:acm:region:account:certificate/id"
  
  # 健康检查
  service.beta.kubernetes.io/aws-load-balancer-healthcheck-interval: "10"
  service.beta.kubernetes.io/aws-load-balancer-healthcheck-timeout: "5"
  
  # 跨可用区负载均衡
  service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
```

## GCP

```yaml
annotations:
  # 内部负载均衡器
  cloud.google.com/load-balancer-type: "Internal"
  
  # 后端服务配置
  cloud.google.com/backend-config: '{"default": "backend-config-name"}'
  
  # NEG（Network Endpoint Group）
  cloud.google.com/neg: '{"ingress": true}'
```

## Azure

```yaml
annotations:
  # 内部负载均衡器
  service.beta.kubernetes.io/azure-load-balancer-internal: "true"
  
  # 资源组
  service.beta.kubernetes.io/azure-load-balancer-resource-group: "myResourceGroup"
  
  # IP 地址
  service.beta.kubernetes.io/azure-load-balancer-ipv4: "10.0.0.10"
```

---

<!-- chunk: ExternalName 详解 -->## ExternalName 详解

**特点**：
- 返回 CNAME 记录的 DNS 别名
- 不使用 selector，不创建 Endpoints
- 不分配 ClusterIP，不进行代理
- 用于将集群外部服务映射到集群内部 DNS 名称

**配置示例**：
```yaml
apiVersion: v1
kind: Service
metadata:
  name: external-database
  namespace: app
spec:
  type: ExternalName
  externalName: db.example.com  # 外部服务的 DNS 名称
  ports:  # ExternalName 不强制要求 ports，但定义后可用于文档化
  - port: 3306
```

**DNS 解析行为**：
```bash
nslookup external-database.app.svc.cluster.local
# 返回 CNAME：db.example.com
```

**使用场景**：
1. **迁移到 Kubernetes**：应用逐步迁移时，引用外部遗留系统
2. **跨命名空间访问**：为其他命名空间的服务创建别名
3. **外部 SaaS 服务**：为云数据库、消息队列等创建内部名称
4. **环境抽象**：开发环境使用外部服务，生产环境切换到内部服务

**跨命名空间别名示例**：
```yaml
apiVersion: v1
kind: Service
metadata:
  name: redis-alias
  namespace: app-team
spec:
  type: ExternalName
  externalName: redis.infrastructure.svc.cluster.local  # 引用其他命名空间的服务
```

---

<!-- chunk: 高级字段 -->## 高级字段

## internalTrafficPolicy（v1.26+）

**用途**：控制集群内部流量路由策略

| 值 | 行为 |
|----|------|
| **Cluster** | 流量转发到所有可用后端 Pod（跨节点） |
| **Local** | 流量仅转发到本地节点的 Pod |

```yaml
apiVersion: v1
kind: Service
metadata:
  name: internal-api
spec:
  type: ClusterIP
  internalTrafficPolicy: Local  # 内部流量策略
  selector:
    app: api
  ports:
  - port: 8080
    targetPort: 8080
```

**与 externalTrafficPolicy 对比**：
- `externalTrafficPolicy`：控制 NodePort/LoadBalancer 外部流量
- `internalTrafficPolicy`：控制 ClusterIP 内部流量

**使用场景**：
- 降低网络延迟（避免跨节点通信）
- 提高数据本地性（存储感知应用）
- 拓扑感知路由（v1.21+ 可用更细粒度的拓扑感知提示）

## sessionAffinity 与 sessionAffinityConfig

**用途**：实现会话保持（同一客户端请求转发到同一 Pod）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: stateful-app
spec:
  selector:
    app: stateful
  ports:
  - port: 80
    targetPort: 8080
  sessionAffinity: ClientIP  # 基于客户端 IP 的会话保持
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800  # 会话超时时间（秒），默认 10800（3小时）
```

**注意事项**：
- 基于 IP 哈希，不适用于 NAT 后的大量客户端
- 可能导致负载不均
- 对于需要强会话保持的应用，建议使用应用层解决方案（如 Cookie、JWT）

## externalIPs

**用途**：手动指定外部 IP（不通过云负载均衡器）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: custom-external-ip
spec:
  selector:
    app: myapp
  ports:
  - port: 80
    targetPort: 8080
  externalIPs:
  - 192.0.2.10   # 手动指定的外部 IP
  - 192.0.2.20
```

**使用场景**：
- 裸金属环境（无云负载均衡器）
- BGP 路由公告（配合 MetalLB）
- 已有外部 IP 需要映射

**安全风险**：
- 可能被滥用劫持流量（建议配置准入控制策略）

---

<!-- chunk: 双栈支持（IPv4/IPv6） -->## 双栈支持（IPv4/IPv6）

## ipFamilies 与 ipFamilyPolicy

**v1.20+ 功能**：支持 IPv4/IPv6 双栈服务

**ipFamilyPolicy 值**：
| 值 | 行为 |
|----|------|
| **SingleStack** | 单栈（默认），仅分配一个 IP 族 |
| **PreferDualStack** | 双栈（如果可用），否则降级到单栈 |
| **RequireDualStack** | 强制双栈，集群不支持则失败 |

**双栈配置示例**：
```yaml
apiVersion: v1
kind: Service
metadata:
  name: dual-stack-service
spec:
  ipFamilyPolicy: PreferDualStack  # 双栈策略
  ipFamilies:  # IP 族列表
  - IPv4
  - IPv6
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 8080
  type: ClusterIP
```

**自动分配的 IP**：
```yaml
status:
  clusterIPs:
  - 10.96.10.50       # IPv4
  - fd00:10:96::1234  # IPv6
```

**仅 IPv6 示例**：
```yaml
apiVersion: v1
kind: Service
metadata:
  name: ipv6-only
spec:
  ipFamilyPolicy: SingleStack
  ipFamilies:
  - IPv6
  selector:
    app: modern-app
  ports:
  - port: 443
```

---

<!-- chunk: 最小配置示例 -->## 最小配置示例

## ClusterIP（最简）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-service
spec:
  selector:
    app: myapp
  ports:
  - port: 80
    targetPort: 8080
```

## NodePort（最简）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-nodeport
spec:
  type: NodePort
  selector:
    app: myapp
  ports:
  - port: 80
    targetPort: 8080
```

## LoadBalancer（最简）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-lb
spec:
  type: LoadBalancer
  selector:
    app: myapp
  ports:
  - port: 80
    targetPort: 8080
```

## ExternalName（最简）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: external-svc
spec:
  type: ExternalName
  externalName: api.example.com
```

---

<!-- chunk: 生产级配置示例 -->## 生产级配置示例

## 示例 1：高可用 Web 应用（ClusterIP + Ingress）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: web-backend
  namespace: production
  labels:
    app: web
    tier: backend
    env: prod
  annotations:
    prometheus.io/scrape: "true"       # Prometheus 监控
    prometheus.io/port: "9090"
    prometheus.io/path: "/metrics"
spec:
  type: ClusterIP
  selector:
    app: web
    tier: backend
  ports:
  - name: http        # HTTP 服务端口
    protocol: TCP
    port: 80
    targetPort: http  # 引用 Pod 的命名端口
    appProtocol: http
  - name: metrics     # 监控指标端口
    protocol: TCP
    port: 9090
    targetPort: metrics
  sessionAffinity: None  # 无状态应用不需要会话保持
  internalTrafficPolicy: Cluster  # 跨节点负载均衡
```

## 示例 2：NodePort 外部访问（带源 IP 保留）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: api-gateway
  namespace: edge
  labels:
    app: gateway
    component: ingress
spec:
  type: NodePort
  selector:
    app: gateway
  ports:
  - name: http
    protocol: TCP
    port: 80
    targetPort: 8080
    nodePort: 30080   # 固定节点端口
  - name: https
    protocol: TCP
    port: 443
    targetPort: 8443
    nodePort: 30443
  externalTrafficPolicy: Local  # 保留源 IP，减少跳转
  # Local 策略注意事项：
  # 1. 只转发到本地 Pod，可能导致负载不均
  # 2. 需确保每个节点都有 Pod 副本（使用 DaemonSet 或 podAntiAffinity）
  # 3. 健康检查只检查本地 Pod
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 3600  # 1 小时会话保持
```

## 示例 3：云负载均衡器（AWS NLB）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: public-web
  namespace: frontend
  annotations:
    # AWS NLB 配置
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    service.beta.kubernetes.io/aws-load-balancer-scheme: "internet-facing"
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
    
    # SSL/TLS 配置
    service.beta.kubernetes.io/aws-load-balancer-ssl-cert: "arn:aws:acm:us-east-1:123456789012:certificate/abcd1234"
    service.beta.kubernetes.io/aws-load-balancer-ssl-ports: "443"
    service.beta.kubernetes.io/aws-load-balancer-backend-protocol: "http"
    
    # 健康检查
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-protocol: "HTTP"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-path: "/health"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-interval: "10"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-timeout: "5"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-healthy-threshold: "2"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-unhealthy-threshold: "2"
    
    # 访问日志
    service.beta.kubernetes.io/aws-load-balancer-access-log-enabled: "true"
    service.beta.kubernetes.io/aws-load-balancer-access-log-s3-bucket-name: "my-lb-logs"
    service.beta.kubernetes.io/aws-load-balancer-access-log-s3-bucket-prefix: "prod-web"
spec:
  type: LoadBalancer
  selector:
    app: web
    tier: frontend
  ports:
  - name: http
    port: 80
    targetPort: 8080
  - name: https
    port: 443
    targetPort: 8080
  loadBalancerSourceRanges:  # IP 白名单
  - 203.0.113.0/24   # 办公网络
  - 198.51.100.0/24  # VPN 网段
  externalTrafficPolicy: Local  # 保留源 IP
  allocateLoadBalancerNodePorts: true
```

## 示例 4：Headless Service + StatefulSet

```yaml
# Headless Service
apiVersion: v1
kind: Service
metadata:
  name: mysql
  namespace: database
  labels:
    app: mysql
spec:
  clusterIP: None  # Headless Service 关键配置
  selector:
    app: mysql
  ports:
  - name: mysql
    port: 3306
    targetPort: 3306
  publishNotReadyAddresses: true  # 发布未就绪地址（用于主从复制初始化）

---
# StatefulSet 使用 Headless Service
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: database
spec:
  serviceName: mysql  # 引用上面的 Headless Service
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
        ports:
        - name: mysql
          containerPort: 3306
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-secret
              key: password
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 100Gi
```

**稳定的网络标识**：
- `mysql-0.mysql.database.svc.cluster.local` → Pod mysql-0
- `mysql-1.mysql.database.svc.cluster.local` → Pod mysql-1
- `mysql-2.mysql.database.svc.cluster.local` → Pod mysql-2

## 示例 5：双栈服务（IPv4 + IPv6）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: modern-web
  namespace: apps
spec:
  type: LoadBalancer
  ipFamilyPolicy: PreferDualStack  # 优先双栈，不可用则降级
  ipFamilies:
  - IPv4  # 主 IP 族
  - IPv6
  selector:
    app: web
    version: v2
  ports:
  - name: https
    protocol: TCP
    port: 443
    targetPort: 8443
  externalTrafficPolicy: Local
  allocateLoadBalancerNodePorts: true
```

**自动分配的 IP**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get svc modern-web -o yaml
# status:
#   clusterIPs:
#   - 10.96.50.100
#   - fd00:10:96::5064
#   loadBalancer:
#     ingress:
#     - ip: 203.0.113.50
#     - ip: 2001:db8::1234
```
## 示例 6：外部服务代理（无 Selector + 手动 Endpoints）

```yaml
# Service 定义（无 selector）
apiVersion: v1
kind: Service
metadata:
  name: external-api
  namespace: integration
spec:
  ports:
  - name: https
    protocol: TCP
    port: 443
    targetPort: 443
  # 没有 selector，Endpoints 需要手动创建

---
# 手动创建 Endpoints
apiVersion: v1
kind: Endpoints
metadata:
  name: external-api  # 名称必须与 Service 一致
  namespace: integration
subsets:
- addresses:
  - ip: 192.0.2.10    # 外部服务 IP 1
  - ip: 192.0.2.20    # 外部服务 IP 2
  ports:
  - name: https
    port: 443
    protocol: TCP
```

**使用场景**：
- 将外部 IP 地址集成到集群 DNS
- 逐步迁移遗留系统
- 跨集群服务通信

---

<!-- chunk: 内部原理 -->## 内部原理

## kube-proxy 实现机制

**三种代理模式**：

## 1. iptables 模式（默认）

**原理**：
- kube-proxy 监听 Service/Endpoints 变化
- 生成 iptables 规则实现 DNAT 和负载均衡
- 纯内核空间处理，性能较好

**规则示例**：
```bash
# 查看 Service 的 iptables 规则
iptables -t nat -L KUBE-SERVICES -n | grep my-service

# 典型规则链：
# KUBE-SERVICES → KUBE-SVC-XXX → KUBE-SEP-XXX（每个 Pod）
```

**特点**：
- ✅ 稳定可靠，成熟方案
- ✅ 无需额外组件
- ❌ 规则数量与 Service/Pod 数量线性增长（大规模集群性能下降）
- ❌ 负载均衡算法简单（随机选择）

## 2. IPVS 模式（推荐用于大规模集群）

**原理**：
- 使用 Linux IPVS（IP Virtual Server）模块
- 支持多种负载均衡算法（rr、lc、dh、sh等）
- 性能优于 iptables（哈希表 vs 链式规则）

**启用方式**：
```bash
# kube-proxy 启动参数
--proxy-mode=ipvs

# 依赖内核模块
modprobe ip_vs
modprobe ip_vs_rr   # Round Robin
modprobe ip_vs_wrr  # Weighted Round Robin
modprobe ip_vs_sh   # Source Hashing
```

**查看 IPVS 规则**：
```bash
ipvsadm -Ln

# 输出示例：
# TCP  10.96.10.20:80 rr
#   -> 10.244.1.5:8080  Masq    1      0          0
#   -> 10.244.2.8:8080  Masq    1      0          0
```

**特点**：
- ✅ 更好的性能和可扩展性
- ✅ 丰富的负载均衡算法
- ✅ 支持更多连接数
- ❌ 需要额外内核模块
- ❌ 调试相对复杂

## 3. nftables 模式（v1.29+ 实验性）

**原理**：
- 使用 nftables 替代 iptables
- 统一的框架，更好的性能

**状态**：
- v1.29+ 开始支持（Alpha）
- 未来可能取代 iptables 模式

## Service CIDR

**定义**：为 ClusterIP 分配的 IP 地址池

**配置位置**：
```bash
# kube-apiserver
--service-cluster-ip-range=10.96.0.0/12

# kube-controller-manager（也需要知道此范围）
--service-cluster-ip-range=10.96.0.0/12
```

**地址分配**：
- 自动从 CIDR 池中分配
- 避免与 Pod CIDR 冲突
- 第一个 IP（10.96.0.1）保留给 kubernetes.default.svc

**双栈配置**：
```bash
--service-cluster-ip-range=10.96.0.0/12,fd00:10:96::/108
```

## DNS 记录生成

**CoreDNS/kube-dns 自动创建 A/AAAA 记录**：

| DNS 名称 | 记录类型 | 解析结果 |
|----------|----------|----------|
| `<service>.<namespace>.svc.cluster.local` | A/AAAA | ClusterIP（标准 Service） |
| `<service>.<namespace>.svc.cluster.local` | A/AAAA | 所有 Pod IP（Headless Service） |
| `<pod-name>.<service>.<namespace>.svc.cluster.local` | A/AAAA | Pod IP（Headless + StatefulSet） |
| `<service>.<namespace>.svc` | A/AAAA | 同上（简短形式） |
| `<service>` | A/AAAA | 同命名空间内可省略 |

**ExternalName 的 CNAME 记录**：
```
external-svc.app.svc.cluster.local → CNAME → api.example.com
```

**SRV 记录**（用于服务发现）：
```
_<port-name>._<protocol>.<service>.<namespace>.svc.cluster.local
```

示例：
```bash
dig SRV _http._tcp.web.production.svc.cluster.local

# 返回：
# _http._tcp.web.production.svc.cluster.local. 30 IN SRV 0 33 80 web.production.svc.cluster.local.
```

---

<!-- chunk: 版本兼容性 -->## 版本兼容性

| 功能特性 | 引入版本 | 稳定版本 | 说明 |
|---------|---------|---------|------|
| Service 基础功能 | v1.0 | v1.0 | ClusterIP、NodePort、LoadBalancer |
| ExternalName | v1.3 | v1.7 | DNS CNAME 映射 |
| externalTrafficPolicy | v1.4 (Beta) | v1.7 (GA) | Local/Cluster 策略 |
| ipvs 代理模式 | v1.8 (Beta) | v1.11 (GA) | IPVS 负载均衡 |
| sessionAffinityConfig | v1.7 (Alpha) | v1.10 (GA) | 会话亲和性配置 |
| IPv4/IPv6 双栈 | v1.16 (Alpha) | v1.23 (GA) | ipFamilies、ipFamilyPolicy |
| loadBalancerClass | v1.21 (Alpha) | v1.24 (GA) | 多 LB 控制器支持 |
| allocateLoadBalancerNodePorts | v1.20 (Alpha) | v1.24 (GA) | 禁用 LB 的 NodePort 分配 |
| internalTrafficPolicy | v1.22 (Alpha) | v1.26 (GA) | 内部流量路由策略 |
| appProtocol | v1.18 (Alpha) | v1.20 (GA) | 应用层协议标识 |
| Service Type=LoadBalancer status.loadBalancer.ingress[].ports | v1.24 (Beta) | v1.26 (GA) | LB 端口状态信息 |

**废弃字段**：
- `spec.loadBalancerIP`（v1.24 废弃）：使用云提供商注解替代
- `spec.externalIPs`（安全风险）：建议配合准入策略使用

---

<!-- chunk: 最佳实践 -->## 最佳实践

## 1. 服务类型选择

| 场景 | 推荐类型 | 理由 |
|------|---------|------|
| 集群内部微服务通信 | ClusterIP | 默认、安全、高效 |
| 开发环境快速测试 | NodePort | 简单直接 |
| 生产环境外部暴露 | LoadBalancer + Ingress | 统一入口、TLS 终结 |
| 有状态应用（数据库） | Headless (ClusterIP: None) | 稳定网络标识 |
| 外部服务引用 | ExternalName | DNS 抽象 |

## 2. 端口命名规范

```yaml
ports:
- name: http       # 使用协议名称
  port: 80
- name: https
  port: 443
- name: metrics    # 功能性命名
  port: 9090
- name: grpc       # 协议名称
  port: 50051
```

**好处**：
- Istio 等服务网格依赖端口名识别协议
- 提高可读性和可维护性
- 支持引用（targetPort: http）

## 3. 使用命名端口

```yaml
# Pod 定义
spec:
  containers:
  - name: app
    ports:
    - name: http     # 命名端口
      containerPort: 8080
    - name: metrics
      containerPort: 9090

---
# Service 引用
spec:
  ports:
  - port: 80
    targetPort: http  # 引用名称而非硬编码端口号
```

**优势**：
- 容器端口变更时无需修改 Service
- 更清晰的意图表达

## 4. 合理设置 externalTrafficPolicy

**选择指南**：

| 需求 | 推荐值 | 说明 |
|------|-------|------|
| 需要源 IP（日志、安全） | Local | 保留客户端源 IP |
| 负载均衡优先 | Cluster | 全局负载均衡 |
| 低延迟 | Local | 减少网络跳转 |
| 跨节点容错 | Cluster | 避免单节点问题 |

**Local 策略注意事项**：
- 使用 DaemonSet 确保每个节点都有 Pod
- 或配合 Pod 反亲和性实现均匀分布
- 监控负载不均情况

## 5. LoadBalancer 注解集中管理

```yaml
# 使用 ConfigMap 管理通用注解
apiVersion: v1
kind: ConfigMap
metadata:
  name: lb-annotations
  namespace: kube-system
data:
  annotations: |
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
```

**通过 Helm/Kustomize 引用**：
```yaml
# Helm values
service:
  annotations:
    {{- include "common-lb-annotations" . | nindent 4 }}
```

## 6. 健康检查配置

```yaml
# 应用容器暴露健康检查端点
spec:
  containers:
  - name: app
    livenessProbe:
      httpGet:
        path: /healthz
        port: 8080
      initialDelaySeconds: 30
      periodSeconds: 10
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080
      initialDelaySeconds: 5
      periodSeconds: 5
```

**Service 层健康检查**：
- NodePort/LoadBalancer 自动基于 Pod readinessProbe
- 云 LB 可通过注解自定义健康检查参数

## 7. 监控与可观测性

**关键指标**：
```yaml
# 使用 Prometheus 监控
annotations:
  prometheus.io/scrape: "true"
  prometheus.io/port: "9090"
  prometheus.io/path: "/metrics"
```

**监控项**：
- Service 的 Endpoints 数量
- 后端 Pod 健康状态
- 网络延迟和错误率
- kube-proxy 规则同步延迟

## 8. 安全加固

**限制 externalIPs 使用**（准入控制）：
```yaml
# OPA/Kyverno 策略示例
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: restrict-external-ips
spec:
  validationFailureAction: enforce
  rules:
  - name: check-external-ips
    match:
      resources:
        kinds:
        - Service
    validate:
      message: "externalIPs is not allowed"
      pattern:
        spec:
          =(externalIPs): null
```

**LoadBalancer 源 IP 限制**：
```yaml
spec:
  loadBalancerSourceRanges:
  - 203.0.113.0/24  # 仅允许特定 CIDR 访问
```

## 9. 资源标签与注解

**标准化标签**：
```yaml
labels:
  app.kubernetes.io/name: myapp
  app.kubernetes.io/instance: myapp-prod
  app.kubernetes.io/version: "1.2.3"
  app.kubernetes.io/component: backend
  app.kubernetes.io/part-of: e-commerce
  app.kubernetes.io/managed-by: helm
```

**有用的注解**：
```yaml
annotations:
  # 文档化
  description: "Production backend API service"
  
  # 监控集成
  prometheus.io/scrape: "true"
  
  # 网络策略
  network-policy.kubernetes.io/ingress: "allow-from-ingress"
  
  # 变更追踪
  last-updated: "2026-02-10"
  updated-by: "platform-team"
```

## 10. 多集群服务

**使用 Multi-Cluster Services（MCS）**：
```yaml
apiVersion: v1
kind: Service
metadata:
  name: global-api
  annotations:
    federation.k8s.io/federated-service: "true"
spec:
  type: ClusterIP
  selector:
    app: api
  ports:
  - port: 80
```

**服务导出**（v1.21+ MCS API）：
```yaml
apiVersion: multicluster.x-k8s.io/v1alpha1
kind: ServiceExport
metadata:
  name: my-service
  namespace: default
```

---

<!-- chunk: FAQ -->## FAQ

## Q1: ClusterIP 无法访问？

**排查步骤**：
``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 Service 是否存在
kubectl get svc my-service -n namespace

# 2. 检查 Endpoints 是否有 IP
kubectl get endpoints my-service -n namespace

# 3. 如果 Endpoints 为空，检查 Pod 标签是否匹配
kubectl get pods -l app=myapp -n namespace --show-labels

# 4. 检查 Pod 是否 Ready
kubectl get pods -n namespace

# 5. 测试 DNS 解析
kubectl run -it --rm debug --image=busybox --restart=Never -- nslookup my-service.namespace.svc.cluster.local

# 6. 测试网络连通性
kubectl run -it --rm debug --image=nicolaka/netshoot --restart=Never -- curl http://my-service.namespace.svc.cluster.local

# 7. 检查 kube-proxy 状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl logs -n kube-system kube-proxy-xxxxx

# 8. 检查网络策略（NetworkPolicy）
kubectl get networkpolicy -n namespace
```
## Q2: NodePort 无法从外部访问？

**常见原因**：
1. **防火墙规则**：云平台安全组未开放端口
2. **节点 IP 不可达**：使用内网 IP，外部无法路由
3. **externalTrafficPolicy=Local** 但节点无 Pod

**解决方案**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 Service
kubectl get svc my-service -o yaml

# 2. 测试从节点访问
ssh node-ip
curl http://localhost:30080

# 3. 检查 iptables 规则
iptables -t nat -L KUBE-NODEPORTS -n

# 4. 检查云安全组（AWS 示例）
aws ec2 describe-security-groups --group-ids sg-xxxxx
```
## Q3: LoadBalancer 一直 Pending？

**原因**：
- 集群未集成云控制器管理器（Cloud Controller Manager）
- 云提供商配额不足
- 子网配置错误

**检查**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Service 事件
kubectl describe svc my-lb

# 查看 cloud-controller-manager 日志
kubectl logs -n kube-system -l app=cloud-controller-manager

# 手动测试（AWS 示例）
aws elbv2 describe-load-balancers
```
**裸金属集群解决方案**：
- 使用 [MetalLB](https://metallb.universe.tf/)
- 使用 [Cilium LB IPAM](https://docs.cilium.io/en/stable/network/lb-ipam/)

## Q4: 如何实现金丝雀发布？

**方案 1：多版本 Service**
```yaml
# 稳定版本 Service
apiVersion: v1
kind: Service
metadata:
  name: app-stable
spec:
  selector:
    app: myapp
    version: v1
  ports:
  - port: 80

---
# 金丝雀版本 Service
apiVersion: v1
kind: Service
metadata:
  name: app-canary
spec:
  selector:
    app: myapp
    version: v2
  ports:
  - port: 80

---
# Ingress 配置流量分割
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "10"  # 10% 流量到金丝雀
spec:
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-canary
            port:
              number: 80
```

**方案 2：使用服务网格**（Istio）
```yaml
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: app
spec:
  hosts:
  - app.example.com
  http:
  - matchers:
    - - headers=""
    - user-agent=""
    - regex=".*Mobile.*"  # 移动端用户使用金丝雀版本"
    - route=""
    - - destination=""
    - host="app"
    - subset="v2"
  - route:
    - destination:
        host: app
        subset: v1
      weight: 90  # 90% 流量
    - destination:
        host: app
        subset: v2
      weight: 10  # 10% 流量
```

## Q5: Service 如何实现跨命名空间访问？

**DNS 全限定名**：
```yaml
# 命名空间 A 中的 Pod 访问命名空间 B 的 Service
apiVersion: v1
kind: Pod
metadata:
  name: client
  namespace: namespace-a
spec:
  containers:
  - name: app
    image: myapp
    env:
    - name: API_URL
      value: "http://api-service.namespace-b.svc.cluster.local:80"
```

**ExternalName 别名**：
```yaml
# 在命名空间 A 中创建 Service
apiVersion: v1
kind: Service
metadata:
  name: api-alias
  namespace: namespace-a
spec:
  type: ExternalName
  externalName: api-service.namespace-b.svc.cluster.local
```

## Q6: 如何查看 Service 背后的 Pod IP？

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 方法 1：查看 Endpoints
kubectl get endpoints my-service -n namespace

# 方法 2：使用 describe
kubectl describe svc my-service -n namespace

# 方法 3：YAML 输出
kubectl get endpoints my-service -n namespace -o yaml

# 方法 4：查看 EndpointSlice（v1.21+）
kubectl get endpointslices -n namespace -l kubernetes.io/service-name=my-service
```
## Q7: Service 的 sessionAffinity 不生效？

**检查点**：
1. **客户端 IP 是否被 NAT**：`externalTrafficPolicy: Cluster` 会导致源 IP 被 SNAT
2. **会话超时**：默认 3 小时后失效
3. **代理模式**：IPVS 模式下行为可能不同

**解决方案**：
```yaml
spec:
  externalTrafficPolicy: Local  # 保留源 IP
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800  # 明确设置超时
```

## Q8: 如何强制删除处于 Terminating 的 Service？

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 正常删除
kubectl delete svc my-service -n namespace

# 强制删除（移除 finalizers）
kubectl patch svc my-service -n namespace -p '{"metadata":{"finalizers":null}}'

# 或直接编辑
kubectl edit svc my-service -n namespace
# 删除 metadata.finalizers 字段
```
---

<!-- chunk: 生产案例 -->## 生产案例

## 案例 1：Headless Service + StatefulSet 部署 MySQL 集群

**场景**：部署一个主从复制的 MySQL 集群，需要稳定的网络标识和存储。

```yaml
# Headless Service
apiVersion: v1
kind: Service
metadata:
  name: mysql
  namespace: database
  labels:
    app: mysql
spec:
  clusterIP: None  # Headless
  selector:
    app: mysql
  ports:
  - name: mysql
    port: 3306
    targetPort: 3306
  publishNotReadyAddresses: true  # 允许访问未就绪的 Pod（用于主从配置）

---
# 客户端访问的标准 Service（读写分离）
apiVersion: v1
kind: Service
metadata:
  name: mysql-read
  namespace: database
  labels:
    app: mysql
    service: read
spec:
  type: ClusterIP
  selector:
    app: mysql
  ports:
  - name: mysql
    port: 3306
    targetPort: 3306

---
# StatefulSet
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: database
spec:
  serviceName: mysql  # 关联 Headless Service
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
    spec:
      initContainers:
      - name: init-mysql
        image: mysql:8.0
        command:
        - bash
        - "-c"
        - |
          set -ex
          # 根据 Pod 序号生成 server-id
          $(hostname) =~ -([0-9]+)$ || exit 1
          ordinal=${BASH_REMATCH[1]}
          echo [mysqld] > /mnt/conf.d/server-id.cnf
          echo server-id=$((100 + $ordinal)) >> /mnt/conf.d/server-id.cnf
          
          # 主节点（mysql-0）配置
          if $ordinal -eq 0; then
            cp /mnt/config-map/master.cnf /mnt/conf.d/
          else
            cp /mnt/config-map/slave.cnf /mnt/conf.d/
          fi
        volumeMounts:
        - name: conf
          mountPath: /mnt/conf.d
        - name: config-map
          mountPath: /mnt/config-map
      containers:
      - name: mysql
        image: mysql:8.0
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mysql-secret
              key: root-password
        ports:
        - name: mysql
          containerPort: 3306
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
        - name: conf
          mountPath: /etc/mysql/conf.d
        livenessProbe:
          exec:
            command: ["mysqladmin", "ping", "-uroot", "-p${MYSQL_ROOT_PASSWORD}"]
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          exec:
            command: ["mysql", "-uroot", "-p${MYSQL_ROOT_PASSWORD}", "-e", "SELECT 1"]
          initialDelaySeconds: 5
          periodSeconds: 2
      volumes:
      - name: conf
        emptyDir: {}
      - name: config-map
        configMap:
          name: mysql-config
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 100Gi

---
# ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: mysql-config
  namespace: database
data:
  master.cnf: |
    [mysqld]
    log-bin=mysql-bin
    binlog_format=ROW
  slave.cnf: |
    [mysqld]
    relay-log=relay-bin
    read_only=1
```

**访问方式**：
```bash
# 主节点（读写）
mysql -h mysql-0.mysql.database.svc.cluster.local -uroot -p

# 从节点（只读）
mysql -h mysql-1.mysql.database.svc.cluster.local -uroot -p

# 负载均衡读请求（所有副本）
mysql -h mysql-read.database.svc.cluster.local -uroot -p
```

---

## 案例 2：双栈服务（IPv4 + IPv6）

**场景**：现代云原生应用需要同时支持 IPv4 和 IPv6 客户端。

```yaml
apiVersion: v1
kind: Service
metadata:
  name: dual-stack-web
  namespace: frontend
  labels:
    app: web
    ipv6-enabled: "true"
spec:
  type: LoadBalancer
  ipFamilyPolicy: RequireDualStack  # 强制双栈
  ipFamilies:
  - IPv4  # 主 IP 族
  - IPv6
  selector:
    app: web
  ports:
  - name: http
    port: 80
    targetPort: 8080
  - name: https
    port: 443
    targetPort: 8443
  externalTrafficPolicy: Local
  allocateLoadBalancerNodePorts: true
```

**验证双栈配置**：
``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看分配的 IP
kubectl get svc dual-stack-web -o yaml | grep -A 5 clusterIPs
# 输出：
# clusterIPs:
# - 10.96.50.100
# - fd00:10:96::5064

# 测试 IPv4 连接
curl http://10.96.50.100

# 测试 IPv6 连接
curl -g -6 "http://[fd00:10:96::5064]"

# DNS 解析（返回 A 和 AAAA 记录）
kubectl run -it --rm debug --image=busybox -- nslookup dual-stack-web.frontend.svc.cluster.local
```
**Pod 配置（确保容器支持双栈）**：
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: web-pod
  labels:
    app: web
spec:
  containers:
  - name: nginx
    image: nginx:latest
    ports:
    - containerPort: 8080
    env:
    - name: LISTEN_IPV6
      value: "true"  # 应用层配置监听 IPv6
```

---

## 案例 3：云 LB 高级配置（AWS NLB 与 TLS 终结）

**场景**：在 AWS EKS 上部署 HTTPS 服务，使用 ACM 证书在 NLB 层终结 TLS。

```yaml
apiVersion: v1
kind: Service
metadata:
  name: secure-web
  namespace: production
  annotations:
    # 负载均衡器类型
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    service.beta.kubernetes.io/aws-load-balancer-scheme: "internet-facing"
    
    # TLS 配置
    service.beta.kubernetes.io/aws-load-balancer-ssl-cert: "arn:aws:acm:us-east-1:123456789012:certificate/abcd-1234"
    service.beta.kubernetes.io/aws-load-balancer-ssl-ports: "443"
    service.beta.kubernetes.io/aws-load-balancer-ssl-negotiation-policy: "ELBSecurityPolicy-TLS-1-2-2017-01"
    service.beta.kubernetes.io/aws-load-balancer-backend-protocol: "http"  # 后端使用 HTTP
    
    # 访问控制
    service.beta.kubernetes.io/aws-load-balancer-target-group-attributes: "deregistration_delay.timeout_seconds=30"
    
    # 跨可用区
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
    
    # 健康检查
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-protocol: "HTTP"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-path: "/health"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-port: "8080"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-interval: "10"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-timeout: "5"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-healthy-threshold: "2"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-unhealthy-threshold: "2"
    
    # 访问日志
    service.beta.kubernetes.io/aws-load-balancer-access-log-enabled: "true"
    service.beta.kubernetes.io/aws-load-balancer-access-log-s3-bucket-name: "my-lb-logs"
    service.beta.kubernetes.io/aws-load-balancer-access-log-s3-bucket-prefix: "prod/secure-web"
    
    # 额外标签
    service.beta.kubernetes.io/aws-load-balancer-additional-resource-tags: "Environment=production,Team=platform"
spec:
  type: LoadBalancer
  selector:
    app: web
    tier: frontend
  ports:
  - name: http
    port: 80
    targetPort: 8080
    protocol: TCP
  - name: https
    port: 443
    targetPort: 8080  # TLS 在 NLB 层终结，后端仍是 HTTP
    protocol: TCP
  externalTrafficPolicy: Local  # 保留源 IP
  loadBalancerSourceRanges:  # IP 白名单
  - 0.0.0.0/0  # 生产环境应限制为已知 IP 段
  allocateLoadBalancerNodePorts: true
```

**验证配置**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 LB 状态
kubectl get svc secure-web -o wide

# 查看 AWS NLB
LB_ARN=$(aws elbv2 describe-load-balancers --query "LoadBalancers[?LoadBalancerName=='xxx'].LoadBalancerArn" --output text)
aws elbv2 describe-load-balancers --load-balancer-arns $LB_ARN

# 测试 HTTPS 连接
LB_DNS=$(kubectl get svc secure-web -o jsonpath='{.status.loadBalancer.ingress[0].hostname}')
curl -I https://$LB_DNS

# 查看访问日志（S3）
aws s3 ls s3://my-lb-logs/prod/secure-web/
```
---

<!-- chunk: 相关资源 -->## 相关资源

## 官方文档
- [Service 概念](https://kubernetes.io/docs/concepts/services-networking/service/)
- [Service API 参考](https://kubernetes.io/docs/reference/kubernetes-api/service-resources/service-v1/)
- [DNS for Services and Pods](https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/)
- [Topology Aware Routing](https://kubernetes.io/docs/concepts/services-networking/topology-aware-routing/)

## 网络组件
- [kube-proxy](https://kubernetes.io/docs/reference/command-line-tools-reference/kube-proxy/)
- [IPVS 代理模式](https://kubernetes.io/blog/2018/07/09/ipvs-based-in-cluster-load-balancing-deep-dive/)
- [CoreDNS](https://coredns.io/)

## 云集成
- [AWS Load Balancer Controller](https://kubernetes-sigs.github.io/aws-load-balancer-controller/)
- [GCP Cloud Controller Manager](https://github.com/kubernetes/cloud-provider-gcp)
- [Azure Cloud Provider](https://cloud-provider-azure.sigs.k8s.io/)

## 裸金属 LB 方案
- [MetalLB](https://metallb.universe.tf/)
- [Cilium LB IPAM](https://docs.cilium.io/en/stable/network/lb-ipam/)
- [Porter (QingCloud)](https://porterlb.io/)

## 多集群服务
- [Kubernetes Multi-Cluster Services (MCS)](https://github.com/kubernetes/enhancements/tree/master/keps/sig-multicluster/1645-multi-cluster-services-api)
- [Submariner](https://submariner.io/)

## 工具
- [kubectl-view-service-tree](https://github.com/knight42/kubectl-view-service-tree) - 可视化 Service 与 Pod 关系
- [kubectl-service-plugin](https://github.com/superbrothers/kubectl-service-plugin) - Service 管理插件

---

**最后更新**: 2026-02  
**维护者**: Kubernetes 运维团队  
**反馈**: 如有问题请提交 Issue

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-32-yaml-manifests MOC
- [[domain-18-manifests-patterns/README.md|Domain-32: Kubernetes YAML 配置完整参考手册]]
- Domain-32 YAML 清单 — 开源项目索引
- 01 - YAML 语法基础与 Kubernetes 资源通用规范
- 02 - Namespace / ResourceQuota / LimitRange YAML 配置参考
- 03 - Pod 完整规格说明书
- 04 - Deployment / ReplicaSet YAML 配置参考
- 05 - StatefulSet YAML 配置参考
- 06 - DaemonSet YAML 配置参考
- 07 - Job / CronJob YAML 配置参考
- 09 - Endpoints / EndpointSlice YAML 配置参考
- 10 - Ingress / IngressClass YAML 配置参考

## See Also

- 06-daemonset-reference
- 07-job-cronjob-reference
- 09-endpoints-endpointslice
- 10-ingress-ingressclass

## Related

- [[reference|#reference Hub]] — tag hub

- [[domain-19-landscape-references/topic-index/network-index.md|Network 网络知识图谱索引]]


<!-- risk-assessed -->
