# 09 - Endpoints / EndpointSlice YAML 配置参考

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02

## 概述

**Endpoints** 和 **EndpointSlice** 是 Kubernetes 中用于跟踪 Service 后端 Pod 网络端点的资源对象。它们记录了符合 Service 标签选择器的 Pod IP 地址和端口信息,为服务发现和负载均衡提供基础数据。

### 核心概念

**Endpoints (v1)**:
- Kubernetes 早期的端点跟踪机制
- 每个 Service 对应一个 Endpoints 对象(同名)
- 包含所有后端 Pod 的 IP 和端口列表
- 存在可扩展性问题(大规模集群性能瓶颈)

**EndpointSlice (discovery.k8s.io/v1)**:
- v1.21+ GA 的新一代端点跟踪机制
- 将大型 Endpoints 分片为多个小对象
- 更好的可扩展性和性能
- 减少网络流量和 API 负载
- 支持双栈(IPv4/IPv6)和多种拓扑结构

### 主要区别

| 特性 | Endpoints | EndpointSlice |
|------|-----------|---------------|
| **引入版本** | v1.0 | v1.16 (Alpha), v1.21 (GA) |
| **可扩展性** | 单个对象存储所有端点(受 etcd 大小限制) | 分片存储(每个 Slice 默认 100 个端点) |
| **性能** | 大规模集群性能差 | 显著提升网络和 API 性能 |
| **拓扑感知** | 不支持 | 支持拓扑信息(zone、node) |
| **双栈支持** | 有限 | 原生支持 IPv4/IPv6 |
| **更新粒度** | 全量更新 | 增量更新(仅变更的 Slice) |

### 使用场景

**自动管理(常规场景)**:
- Service 带 `selector` 时,Endpoint Controller 自动创建和维护 Endpoints
- EndpointSlice Controller 同时维护 EndpointSlice 对象
- 大多数情况下无需手动操作

**手动管理(特殊场景)**:
1. **无选择器 Service**: 代理外部服务或固定 IP
2. **跨集群服务**: 手动添加其他集群的 Pod IP
3. **外部数据库集成**: 将外部数据库地址映射为集群内 Service
4. **迁移场景**: 混合云迁移期间的流量切换
5. **自定义负载均衡**: 实现特殊的流量分配逻辑

---

## Endpoints API 信息

| API Group | API Version | Kind      | 稳定性 |
|-----------|-------------|-----------|--------|
| core      | v1          | Endpoints | GA     |

**完整 API 路径**:
```
GET /api/v1/namespaces/{namespace}/endpoints/{name}
```

**缩写**: `ep`

**命名空间作用域**: 是

**重要特性**:
- Endpoints 对象名称必须与 Service 名称相同
- Service 带 `selector` 时由系统自动管理
- Service 无 `selector` 时可手动创建

---

## Endpoints 完整字段规格表

### 顶层字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 | 版本要求 |
|---------|------|------|--------|------|----------|
| `apiVersion` | string | 是 | - | API 版本(v1) | v1.0+ |
| `kind` | string | 是 | - | 资源类型(Endpoints) | v1.0+ |
| `metadata.name` | string | 是 | - | 名称(必须与 Service 同名) | v1.0+ |
| `metadata.namespace` | string | 否 | default | 命名空间 | v1.0+ |
| `subsets[]` | array | 否 | [] | 端点子集列表 | v1.0+ |

### subsets[] 字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 |
|---------|------|------|--------|------|
| `addresses[]` | array | 否 | [] | 就绪状态的端点地址列表 |
| `notReadyAddresses[]` | array | 否 | [] | 未就绪状态的端点地址列表 |
| `ports[]` | array | 否 | [] | 端口信息列表 |

### addresses[] / notReadyAddresses[] 字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 |
|---------|------|------|--------|------|
| `ip` | string | 是 | - | 端点 IP 地址 |
| `hostname` | string | 否 | - | 主机名 |
| `nodeName` | string | 否 | - | Pod 所在节点名称 |
| `targetRef` | object | 否 | - | 引用的对象(通常是 Pod) |

### targetRef 字段

| 字段路径 | 类型 | 说明 |
|---------|------|------|
| `kind` | string | 对象类型(通常是 Pod) |
| `namespace` | string | 命名空间 |
| `name` | string | 对象名称 |
| `uid` | string | 对象 UID |
| `resourceVersion` | string | 资源版本 |

### ports[] 字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 |
|---------|------|------|--------|------|
| `name` | string | 否 | - | 端口名称(对应 Service 端口名) |
| `port` | int32 | 是 | - | 端口号 |
| `protocol` | string | 否 | TCP | 协议(TCP/UDP/SCTP) |
| `appProtocol` | string | 否 | - | 应用层协议 |

---

## EndpointSlice API 信息

| API Group | API Version | Kind          | 稳定性 |
|-----------|-------------|---------------|--------|
| discovery.k8s.io | v1    | EndpointSlice | GA (v1.21+) |

**完整 API 路径**:
```
GET /apis/discovery.k8s.io/v1/namespaces/{namespace}/endpointslices
```

**命名空间作用域**: 是

**命名规则**:
- 由 EndpointSlice Controller 自动生成
- 格式: `{service-name}-{随机字符串}`
- 通过标签 `kubernetes.io/service-name` 关联到 Service

---

## EndpointSlice 完整字段规格表

### 顶层字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 | 版本要求 |
|---------|------|------|--------|------|----------|
| `apiVersion` | string | 是 | - | discovery.k8s.io/v1 | v1.21+ |
| `kind` | string | 是 | - | EndpointSlice | v1.21+ |
| `metadata.name` | string | 是 | - | 名称(自动生成或手动指定) | v1.21+ |
| `metadata.namespace` | string | 否 | default | 命名空间 | v1.21+ |
| `metadata.labels` | map | 推荐 | - | 必须包含 `kubernetes.io/service-name` | v1.21+ |
| `addressType` | string | 是 | - | 地址类型(IPv4/IPv6/FQDN) | v1.21+ |
| `endpoints[]` | array | 否 | [] | 端点列表 | v1.21+ |
| `ports[]` | array | 否 | [] | 端口信息列表 | v1.21+ |

### 必需标签

| 标签键 | 说明 | 示例值 |
|-------|------|--------|
| `kubernetes.io/service-name` | 关联的 Service 名称 | my-service |
| `endpointslice.kubernetes.io/managed-by` | 管理者标识 | endpointslice-controller.k8s.io |

### addressType 值

| 值 | 说明 | 使用场景 |
|----|------|----------|
| `IPv4` | IPv4 地址 | 默认,标准单栈 IPv4 集群 |
| `IPv6` | IPv6 地址 | IPv6 单栈或双栈集群 |
| `FQDN` | 完全限定域名 | ExternalName 或外部端点 |

### endpoints[] 字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 |
|---------|------|------|--------|------|
| `addresses[]` | []string | 是 | - | 端点地址列表 |
| `conditions` | object | 否 | - | 端点状态条件 |
| `hostname` | string | 否 | - | 主机名 |
| `targetRef` | object | 否 | - | 引用对象(Pod) |
| `nodeName` | string | 否 | - | Pod 所在节点 |
| `zone` | string | 否 | - | 可用区(拓扑信息) |
| `hints` | object | 否 | - | 拓扑路由提示 |
| `deprecatedTopology` | map | 否 | - | 已废弃的拓扑信息 |

### conditions 字段

| 字段路径 | 类型 | 默认值 | 说明 |
|---------|------|--------|------|
| `ready` | bool | - | 端点是否就绪(对应 Pod readinessProbe) |
| `serving` | bool | - | 端点是否在服务(即使未就绪也可能接收流量) |
| `terminating` | bool | - | 端点是否正在终止 |

**状态解释**:
- `ready=true`: 端点健康,接收正常流量
- `ready=false, serving=true`: 端点未就绪但仍提供服务(如设置 `publishNotReadyAddresses: true`)
- `terminating=true`: Pod 正在删除,即将移除

### ports[] 字段

| 字段路径 | 类型 | 必需 | 默认值 | 说明 |
|---------|------|------|--------|------|
| `name` | string | 否 | - | 端口名称 |
| `port` | int32 | 否 | - | 端口号(可为空表示所有端口) |
| `protocol` | string | 否 | TCP | 协议(TCP/UDP/SCTP) |
| `appProtocol` | string | 否 | - | 应用层协议 |

---

## 手动创建 Endpoints(无 selector Service)

### 场景 1: 代理外部固定 IP 服务

**需求**: 将外部数据库(固定 IP)映射为集群内部 Service

```yaml
# Service 定义(无 selector)
apiVersion: v1
kind: Service
metadata:
  name: external-database
  namespace: production
spec:
  # 注意: 没有 selector 字段
  ports:
  - name: mysql
    protocol: TCP
    port: 3306        # Service 端口
    targetPort: 3306  # 后端端口(可省略,默认与 port 相同)
  type: ClusterIP

---
# 手动创建 Endpoints
apiVersion: v1
kind: Endpoints
metadata:
  name: external-database  # 必须与 Service 同名
  namespace: production
subsets:
- addresses:
  - ip: 192.0.2.10  # 外部数据库 IP 1
  - ip: 192.0.2.20  # 外部数据库 IP 2(多主或高可用)
  ports:
  - name: mysql
    port: 3306
    protocol: TCP
```

**验证**:
```bash
# 查看 Endpoints
kubectl get endpoints external-database -n production

# 测试连接
kubectl run -it --rm mysql-client --image=mysql:8.0 --restart=Never -- \
  mysql -h external-database.production.svc.cluster.local -uroot -p
```

### 场景 2: 跨集群服务代理

**需求**: 在集群 A 中访问集群 B 的服务

```yaml
# 集群 A: Service(无 selector)
apiVersion: v1
kind: Service
metadata:
  name: remote-api
  namespace: integration
spec:
  ports:
  - name: https
    port: 443
    targetPort: 443
  type: ClusterIP

---
# 集群 A: Endpoints 指向集群 B 的节点 IP
apiVersion: v1
kind: Endpoints
metadata:
  name: remote-api
  namespace: integration
subsets:
- addresses:
  - ip: 198.51.100.10  # 集群 B 节点 1 IP
  - ip: 198.51.100.20  # 集群 B 节点 2 IP
  ports:
  - name: https
    port: 30443  # 集群 B 的 NodePort
    protocol: TCP
```

### 场景 3: 混合端点(集群内 + 外部)

**需求**: 部分流量转发到外部服务(如迁移场景)

```yaml
# Service(有 selector,但会被手动 Endpoints 覆盖)
apiVersion: v1
kind: Service
metadata:
  name: hybrid-api
  namespace: app
spec:
  # 注意: 如果有 selector,手动创建的 Endpoints 会被覆盖
  # 因此混合场景需要两个 Service
  ports:
  - port: 80
    targetPort: 8080

---
# 内部 Service(自动 Endpoints)
apiVersion: v1
kind: Service
metadata:
  name: hybrid-api-internal
  namespace: app
spec:
  selector:
    app: api
    version: v2
  ports:
  - port: 80
    targetPort: 8080

---
# 外部 Service(手动 Endpoints)
apiVersion: v1
kind: Service
metadata:
  name: hybrid-api-external
  namespace: app
spec:
  # 无 selector
  ports:
  - port: 80
    targetPort: 80

---
apiVersion: v1
kind: Endpoints
metadata:
  name: hybrid-api-external
  namespace: app
subsets:
- addresses:
  - ip: 203.0.113.50  # 外部遗留系统
  ports:
  - port: 80

---
# Ingress 流量分割(90% 内部, 10% 外部)
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: hybrid-api-split
  namespace: app
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "10"
spec:
  rules:
  - host: api.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: hybrid-api-external
            port:
              number: 80
```

---

## 手动创建 EndpointSlice

### 基础示例

```yaml
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: external-db-slice-1
  namespace: database
  labels:
    kubernetes.io/service-name: external-db  # 关联 Service 的标签(必需)
    endpointslice.kubernetes.io/managed-by: manual  # 管理者标识
addressType: IPv4  # 地址类型
endpoints:
- addresses:
  - 192.0.2.10  # 数据库主节点
  conditions:
    ready: true
    serving: true
    terminating: false
  nodeName: external-node-1  # 可选: 节点名称
  zone: us-east-1a           # 可选: 可用区
- addresses:
  - 192.0.2.20  # 数据库从节点
  conditions:
    ready: true
    serving: true
    terminating: false
  zone: us-east-1b
ports:
- name: mysql
  protocol: TCP
  port: 3306
  appProtocol: mysql
```

### 多 Slice 分片示例

**场景**: 超过 100 个端点时分片

```yaml
# 第一个分片
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: large-service-1
  namespace: production
  labels:
    kubernetes.io/service-name: large-service
addressType: IPv4
endpoints:
# 包含 100 个端点(0-99)
- addresses: ["10.244.0.10"]
  conditions: {ready: true}
# ... 省略 98 个 ...
- addresses: ["10.244.0.109"]
  conditions: {ready: true}
ports:
- name: http
  port: 8080

---
# 第二个分片
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: large-service-2
  namespace: production
  labels:
    kubernetes.io/service-name: large-service
addressType: IPv4
endpoints:
# 包含剩余端点(100-149)
- addresses: ["10.244.0.110"]
  conditions: {ready: true}
# ... 省略 48 个 ...
- addresses: ["10.244.0.159"]
  conditions: {ready: true}
ports:
- name: http
  port: 8080
```

### 双栈 EndpointSlice

```yaml
# IPv4 Slice
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: dual-stack-service-ipv4
  namespace: app
  labels:
    kubernetes.io/service-name: dual-stack-service
addressType: IPv4
endpoints:
- addresses: ["10.244.1.5"]
  conditions: {ready: true}
- addresses: ["10.244.2.8"]
  conditions: {ready: true}
ports:
- name: http
  port: 80

---
# IPv6 Slice
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: dual-stack-service-ipv6
  namespace: app
  labels:
    kubernetes.io/service-name: dual-stack-service
addressType: IPv6
endpoints:
- addresses: ["fd00:10:244:1::5"]
  conditions: {ready: true}
- addresses: ["fd00:10:244:2::8"]
  conditions: {ready: true}
ports:
- name: http
  port: 80
```

---

## 最小配置示例

### Endpoints 最简配置

```yaml
apiVersion: v1
kind: Endpoints
metadata:
  name: my-service
subsets:
- addresses:
  - ip: 192.0.2.10
  ports:
  - port: 80
```

### EndpointSlice 最简配置

```yaml
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: my-service-slice
  labels:
    kubernetes.io/service-name: my-service
addressType: IPv4
endpoints:
- addresses: ["192.0.2.10"]
ports:
- port: 80
```

---

## 生产级配置示例

### 示例 1: 高可用外部数据库集成

```yaml
# Service 定义
apiVersion: v1
kind: Service
metadata:
  name: postgres-primary
  namespace: database
  labels:
    app: postgres
    role: primary
  annotations:
    description: "Production PostgreSQL primary cluster"
    external-service: "true"
spec:
  ports:
  - name: postgres
    protocol: TCP
    port: 5432
    targetPort: 5432
  - name: metrics
    protocol: TCP
    port: 9187
    targetPort: 9187
  sessionAffinity: ClientIP  # 保持会话亲和性
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 3600
  type: ClusterIP

---
# Endpoints 定义(多主节点)
apiVersion: v1
kind: Endpoints
metadata:
  name: postgres-primary
  namespace: database
  labels:
    app: postgres
    role: primary
subsets:
- addresses:
  # 主节点 1(就绪)
  - ip: 192.0.2.10
    hostname: pg-master-1
    nodeName: db-node-1  # 虚拟节点名称(文档化)
  # 主节点 2(就绪)
  - ip: 192.0.2.20
    hostname: pg-master-2
    nodeName: db-node-2
  notReadyAddresses:
  # 维护中的节点(未就绪)
  - ip: 192.0.2.30
    hostname: pg-master-3
    nodeName: db-node-3
  ports:
  - name: postgres
    port: 5432
    protocol: TCP
  - name: metrics
    port: 9187
    protocol: TCP

---
# 对应的 EndpointSlice(推荐)
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: postgres-primary-1
  namespace: database
  labels:
    kubernetes.io/service-name: postgres-primary
    app: postgres
    role: primary
    endpointslice.kubernetes.io/managed-by: manual
addressType: IPv4
endpoints:
- addresses: ["192.0.2.10"]
  conditions:
    ready: true
    serving: true
    terminating: false
  hostname: pg-master-1
  nodeName: db-node-1
  zone: us-east-1a  # 可用区信息
- addresses: ["192.0.2.20"]
  conditions:
    ready: true
    serving: true
    terminating: false
  hostname: pg-master-2
  nodeName: db-node-2
  zone: us-east-1b
- addresses: ["192.0.2.30"]
  conditions:
    ready: false     # 未就绪
    serving: false
    terminating: false
  hostname: pg-master-3
  nodeName: db-node-3
  zone: us-east-1c
ports:
- name: postgres
  protocol: TCP
  port: 5432
  appProtocol: postgresql
- name: metrics
  protocol: TCP
  port: 9187
  appProtocol: http
```

**监控配置(ServiceMonitor)**:
```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: postgres-primary
  namespace: database
spec:
  selector:
    matchLabels:
      app: postgres
      role: primary
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
```

### 示例 2: 跨集群服务桥接(多集群联邦)

```yaml
# 本地集群: Service 定义
apiVersion: v1
kind: Service
metadata:
  name: federated-api
  namespace: federation
  annotations:
    multicluster.kubernetes.io/exported: "true"
spec:
  ports:
  - name: https
    port: 443
    targetPort: 8443
  type: ClusterIP

---
# EndpointSlice: 集群 A 的端点
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: federated-api-cluster-a
  namespace: federation
  labels:
    kubernetes.io/service-name: federated-api
    multicluster.kubernetes.io/source-cluster: cluster-a
addressType: IPv4
endpoints:
- addresses: ["10.100.1.10"]
  conditions: {ready: true, serving: true}
  zone: cluster-a-zone-1
  hints:
    forZones:
    - name: cluster-a-zone-1
- addresses: ["10.100.1.20"]
  conditions: {ready: true, serving: true}
  zone: cluster-a-zone-2
ports:
- name: https
  port: 8443

---
# EndpointSlice: 集群 B 的端点
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: federated-api-cluster-b
  namespace: federation
  labels:
    kubernetes.io/service-name: federated-api
    multicluster.kubernetes.io/source-cluster: cluster-b
addressType: IPv4
endpoints:
- addresses: ["10.200.1.10"]
  conditions: {ready: true, serving: true}
  zone: cluster-b-zone-1
- addresses: ["10.200.1.20"]
  conditions: {ready: true, serving: true}
  zone: cluster-b-zone-2
ports:
- name: https
  port: 8443
```

**拓扑感知路由配置**:
```yaml
apiVersion: v1
kind: Service
metadata:
  name: federated-api
  namespace: federation
  annotations:
    service.kubernetes.io/topology-aware-hints: "auto"
spec:
  ports:
  - name: https
    port: 443
    targetPort: 8443
  internalTrafficPolicy: Local  # 优先本地流量
  type: ClusterIP
```

### 示例 3: FQDN 类型的 EndpointSlice

**场景**: 引用外部 SaaS 服务的 DNS 名称

```yaml
apiVersion: v1
kind: Service
metadata:
  name: external-saas
  namespace: integration
spec:
  ports:
  - name: https
    port: 443
  type: ClusterIP

---
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: external-saas-fqdn
  namespace: integration
  labels:
    kubernetes.io/service-name: external-saas
addressType: FQDN  # 使用 FQDN 类型
endpoints:
- addresses: ["api.example-saas.com"]  # 外部服务域名
  conditions: {ready: true, serving: true}
- addresses: ["api-backup.example-saas.com"]  # 备用域名
  conditions: {ready: true, serving: true}
ports:
- name: https
  protocol: TCP
  port: 443
  appProtocol: https
```

**注意事项**:
- FQDN 类型的 EndpointSlice 由 kube-proxy 解析 DNS
- DNS 解析缓存可能导致延迟(取决于 TTL)
- 建议用于不频繁变化的外部服务

---

## 内部原理

### Endpoint Controller 工作流程

**自动管理(带 selector 的 Service)**:

```
1. 监听事件
   ↓
   Service、Pod、Node 变化
   ↓
2. 过滤 Pod
   ↓
   根据 Service.spec.selector 匹配 Pod
   ↓
3. 收集端点
   ↓
   提取 Pod IP、containerPort、nodeName、readiness 状态
   ↓
4. 更新 Endpoints
   ↓
   创建或更新同名 Endpoints 对象
   ↓
5. 触发 kube-proxy 同步
   ↓
   更新 iptables/ipvs 规则
```

**关键逻辑**:
- 仅包含 `Ready` 状态的 Pod(除非 Service 设置 `publishNotReadyAddresses: true`)
- Pod 必须有 `PodIP`(未分配 IP 的 Pod 不会加入)
- containerPort 需要匹配 Service.spec.ports[].targetPort

### EndpointSlice Controller 工作流程

**分片策略**:
- 默认每个 Slice 最多 100 个端点(`--max-endpoints-per-slice`)
- 当端点超过限制时自动创建新 Slice
- 删除端点时可能合并 Slice(减少对象数量)

**命名规则**:
```
{service-name}-{5位随机字符串}
例如: my-service-abc12
```

**标签自动添加**:
```yaml
labels:
  kubernetes.io/service-name: my-service  # 关联 Service
  endpointslice.kubernetes.io/managed-by: endpointslice-controller.k8s.io
```

**更新优化**:
- Endpoints: 任何端点变化都需要更新整个对象(全量)
- EndpointSlice: 仅更新变化的 Slice(增量)

**性能对比(1000 Pod 的 Service)**:
| 操作 | Endpoints | EndpointSlice |
|------|-----------|---------------|
| 添加 1 个 Pod | 更新整个对象(~50KB) | 更新 1 个 Slice(~5KB) |
| 删除 1 个 Pod | 更新整个对象 | 更新 1 个 Slice |
| 网络流量 | 高 | 低(~90% 减少) |
| API 负载 | 高 | 低 |

### 拓扑感知路由(Topology Aware Hints)

**v1.23+ GA 功能**:
- EndpointSlice 包含拓扑信息(zone、node)
- kube-proxy 优先路由到同区域/同节点的端点
- 减少跨区域网络流量和成本

**启用方式**:
```yaml
apiVersion: v1
kind: Service
metadata:
  name: topology-aware-svc
  annotations:
    service.kubernetes.io/topology-aware-hints: "auto"
spec:
  selector:
    app: myapp
  ports:
  - port: 80
```

**EndpointSlice hints 字段**:
```yaml
endpoints:
- addresses: ["10.244.1.5"]
  zone: us-east-1a
  hints:
    forZones:  # 提示此端点应服务哪些区域
    - name: us-east-1a
```

---

## 版本兼容性

| 功能特性 | 引入版本 | 稳定版本 | 说明 |
|---------|---------|---------|------|
| Endpoints | v1.0 | v1.0 (GA) | 原始端点跟踪机制 |
| EndpointSlice | v1.16 (Alpha) | v1.21 (GA) | 新一代分片端点 |
| EndpointSlice 默认启用 | v1.17 | v1.21 | 与 Endpoints 并存 |
| 拓扑感知路由 | v1.21 (Beta) | v1.23 (GA) | TopologyAwareHints |
| EndpointSlice 条件字段 | v1.19 | v1.20 | ready、serving、terminating |
| addressType: FQDN | v1.20 (Beta) | v1.21 (GA) | 支持域名端点 |
| EndpointSlice 双栈支持 | v1.20 | v1.23 | IPv4/IPv6 分离的 Slice |

**废弃计划**:
- Endpoints 不会被移除,但大规模集群推荐使用 EndpointSlice
- kube-proxy 优先使用 EndpointSlice(v1.19+ 默认启用)

---

## 最佳实践

### 1. 优先使用 EndpointSlice

**原因**:
- 更好的性能和可扩展性
- 原生支持拓扑感知
- 未来的增强功能优先在 EndpointSlice 实现

**迁移策略**:
```bash
# 检查集群是否启用 EndpointSlice
kubectl get endpointslices -A

# 检查 kube-proxy 配置
kubectl get cm kube-proxy -n kube-system -o yaml | grep EndpointSlice
```

### 2. 手动 Endpoints 维护规范

**命名一致性**:
```yaml
# Service 名称
metadata:
  name: external-db

# Endpoints 必须同名
metadata:
  name: external-db  # 与 Service 完全一致
```

**添加文档化标签**:
```yaml
metadata:
  labels:
    app: database
    external: "true"
    managed-by: platform-team
  annotations:
    description: "External production database cluster"
    contact: "database-team@example.com"
    last-updated: "2026-02-10"
```

### 3. 监控 Endpoints 健康状态

**关键指标**:
- Endpoints 对象的端点数量
- 就绪 vs 未就绪端点比例
- Endpoints 更新频率(频繁更新可能表明 Pod 不稳定)

**Prometheus 查询示例**:
```promql
# Endpoints 总数
sum(kube_endpoint_address_available) by (namespace, endpoint)

# 未就绪端点数量
sum(kube_endpoint_address_not_ready) by (namespace, endpoint)

# Endpoints 更新频率
rate(kube_endpoint_updates_total[5m])
```

**告警规则**:
```yaml
groups:
- name: endpoints
  rules:
  - alert: EndpointsAllNotReady
    expr: |
      sum(kube_endpoint_address_available{}) by (namespace, endpoint) == 0
      and
      sum(kube_endpoint_address_not_ready{}) by (namespace, endpoint) > 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Service {{ $labels.namespace }}/{{ $labels.endpoint }} has no ready endpoints"
  
  - alert: EndpointsMissing
    expr: |
      absent(kube_endpoint_address_available{namespace="production"})
    for: 2m
    labels:
      severity: warning
    annotations:
      summary: "Production endpoints disappeared"
```

### 4. 外部服务健康检查

**问题**: 手动 Endpoints 不会自动检查外部服务健康状态

**解决方案**: 使用 Operator 或 CronJob 动态更新

```yaml
# 健康检查 CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: external-db-healthcheck
  namespace: database
spec:
  schedule: "*/1 * * * *"  # 每分钟检查
  jobTemplate:
    spec:
      template:
        spec:
          serviceAccountName: endpoint-manager
          containers:
          - name: healthcheck
            image: curlimages/curl:latest
            command:
            - /bin/sh
            - -c
            - |
              # 检查外部数据库是否健康
              if curl -f http://192.0.2.10:9187/health; then
                # 健康: 添加到 Endpoints
                kubectl patch endpoints external-db -n database --type=json \
                  -p='[{"op": "add", "path": "/subsets/0/addresses/-", "value": {"ip": "192.0.2.10"}}]'
              else
                # 不健康: 移除
                kubectl patch endpoints external-db -n database --type=json \
                  -p='[{"op": "remove", "path": "/subsets/0/addresses/0"}]'
              fi
          restartPolicy: OnFailure
```

### 5. EndpointSlice 分片策略

**自动分片配置**:
```yaml
# kube-controller-manager 启动参数
--max-endpoints-per-slice=100  # 默认值,可调整
```

**手动分片建议**:
- 小型服务(< 50 端点): 单个 Slice
- 中型服务(50-500 端点): 按区域分片
- 大型服务(> 500 端点): 按区域和节点分片

**按区域分片示例**:
```yaml
# Zone A Slice
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: large-service-zone-a
  labels:
    kubernetes.io/service-name: large-service
    topology.kubernetes.io/zone: us-east-1a
addressType: IPv4
endpoints:
- addresses: ["10.244.1.5"]
  zone: us-east-1a
# ... 更多 zone-a 端点

---
# Zone B Slice
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: large-service-zone-b
  labels:
    kubernetes.io/service-name: large-service
    topology.kubernetes.io/zone: us-east-1b
addressType: IPv4
endpoints:
- addresses: ["10.244.2.8"]
  zone: us-east-1b
# ... 更多 zone-b 端点
```

### 6. 拓扑感知路由最佳实践

**启用条件**:
1. 集群节点有拓扑标签(`topology.kubernetes.io/zone`)
2. 端点在多个区域分布相对均匀
3. Service 设置注解 `service.kubernetes.io/topology-aware-hints: "auto"`

**检查拓扑标签**:
```bash
# 查看节点标签
kubectl get nodes --show-labels | grep topology

# 为节点添加区域标签
kubectl label node node-1 topology.kubernetes.io/zone=us-east-1a
```

**限制**:
- 仅适用于无状态应用(不需要会话保持)
- 端点数量需要足够(每个区域至少 2-3 个)
- 可能与 `externalTrafficPolicy: Local` 冲突

### 7. 双栈 Endpoints 管理

**创建双栈 Service 的 EndpointSlice**:
```yaml
# IPv4 Slice
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: dual-stack-ipv4
  labels:
    kubernetes.io/service-name: dual-stack-svc
addressType: IPv4
endpoints:
- addresses: ["10.244.1.5"]

---
# IPv6 Slice
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: dual-stack-ipv6
  labels:
    kubernetes.io/service-name: dual-stack-svc
addressType: IPv6
endpoints:
- addresses: ["fd00:10:244:1::5"]
```

**验证双栈解析**:
```bash
# 查看所有 EndpointSlice
kubectl get endpointslices -l kubernetes.io/service-name=dual-stack-svc

# DNS 解析测试
kubectl run -it --rm debug --image=busybox -- nslookup dual-stack-svc
# 应返回 A 和 AAAA 记录
```

### 8. 自动化管理工具

**使用 Operator 管理外部端点**:

```yaml
# 自定义资源定义
apiVersion: external.example.com/v1
kind: ExternalService
metadata:
  name: legacy-api
spec:
  endpoints:
  - address: 192.0.2.10
    port: 8080
    healthCheckURL: http://192.0.2.10:8080/health
  - address: 192.0.2.20
    port: 8080
    healthCheckURL: http://192.0.2.20:8080/health
  healthCheckInterval: 30s
  targetService:
    name: legacy-api
    namespace: integration
```

Operator 将:
1. 定期执行健康检查
2. 自动更新 Endpoints/EndpointSlice
3. 发送告警通知

---

## FAQ

### Q1: Service 有 selector 时能否手动创建 Endpoints?

**回答**: 不能。带 `selector` 的 Service 的 Endpoints 由系统自动管理,手动创建会被覆盖。

**解决方案**:
1. 移除 Service 的 `selector` 字段
2. 或创建两个 Service(一个自动,一个手动)

### Q2: 如何查看 Service 的所有 EndpointSlice?

```bash
# 方法 1: 通过标签过滤
kubectl get endpointslices -n namespace \
  -l kubernetes.io/service-name=my-service

# 方法 2: 使用 kubectl describe
kubectl describe svc my-service -n namespace

# 方法 3: 查看详细信息
kubectl get endpointslices -n namespace \
  -l kubernetes.io/service-name=my-service \
  -o yaml
```

### Q3: Endpoints 为空但 Pod 正常运行?

**排查步骤**:

```bash
# 1. 检查 Service selector
kubectl get svc my-service -o yaml | grep -A 5 selector

# 2. 检查 Pod 标签
kubectl get pods -n namespace --show-labels

# 3. 验证标签匹配
kubectl get pods -n namespace -l app=myapp

# 4. 检查 Pod 就绪状态
kubectl get pods -n namespace -o wide

# 5. 查看 Pod readinessProbe
kubectl describe pod pod-name -n namespace | grep -A 10 Readiness

# 6. 检查 Pod IP 分配
kubectl get pods -n namespace -o jsonpath='{.items[*].status.podIP}'
```

**常见原因**:
1. **标签不匹配**: Service selector 与 Pod labels 不一致
2. **Pod 未就绪**: readinessProbe 失败
3. **Pod 无 IP**: 网络插件问题
4. **命名空间错误**: Service 和 Pod 不在同一命名空间

### Q4: 如何强制刷新 Endpoints?

```bash
# 方法 1: 触发 Service 更新
kubectl annotate svc my-service force-refresh="$(date)"

# 方法 2: 重启 Endpoint Controller
kubectl rollout restart deployment kube-controller-manager -n kube-system

# 方法 3: 手动删除 Endpoints(会自动重建)
kubectl delete endpoints my-service -n namespace
```

### Q5: EndpointSlice 和 Endpoints 数据不一致?

**原因**: kube-proxy 可能同时监听两种资源

**检查 kube-proxy 配置**:
```bash
kubectl get cm kube-proxy -n kube-system -o yaml | grep -i endpoint
```

**预期配置**(v1.21+):
```yaml
mode: ""  # 或 iptables/ipvs
detectLocalMode: ""
featureGates:
  EndpointSliceProxying: true  # 启用 EndpointSlice
```

**解决方案**:
- 确保 kube-proxy 启用 `EndpointSliceProxying` 特性门控
- 升级 kube-proxy 到 v1.21+

### Q6: 手动 Endpoints 的端口匹配规则?

**规则**: Endpoints 的端口名称应与 Service 端口名称匹配(可选但推荐)

```yaml
# Service 定义
spec:
  ports:
  - name: http  # 端口名称
    port: 80

# Endpoints 应匹配
subsets:
- ports:
  - name: http  # 相同名称(推荐)
    port: 8080
```

**不匹配的影响**:
- 单端口 Service: 无影响
- 多端口 Service: 可能导致端口映射错误

### Q7: 如何实现 Endpoints 的灰度更新?

**场景**: 外部服务从旧 IP 迁移到新 IP

```yaml
# 阶段 1: 同时包含新旧 IP
apiVersion: v1
kind: Endpoints
metadata:
  name: migrating-service
subsets:
- addresses:
  - ip: 192.0.2.10  # 旧 IP
  - ip: 192.0.2.100 # 新 IP
  ports:
  - port: 80

# 阶段 2: 观察一段时间后,移除旧 IP
# (编辑 Endpoints,删除旧 IP)

# 阶段 3: 最终仅保留新 IP
apiVersion: v1
kind: Endpoints
metadata:
  name: migrating-service
subsets:
- addresses:
  - ip: 192.0.2.100  # 仅新 IP
  ports:
  - port: 80
```

**监控流量分布**:
```bash
# 查看连接数(需要在 kube-proxy 节点执行)
conntrack -L | grep 192.0.2.10  # 旧 IP
conntrack -L | grep 192.0.2.100 # 新 IP
```

### Q8: EndpointSlice 的 serving 和 ready 条件区别?

| 条件 | 含义 | 影响 |
|------|------|------|
| `ready: true` | Pod readinessProbe 成功 | 接收正常流量 |
| `ready: false, serving: true` | 未就绪但仍服务(如设置 publishNotReadyAddresses) | 接收流量(可能不稳定) |
| `ready: false, serving: false` | 未就绪且不服务 | 不接收流量 |
| `terminating: true` | Pod 正在终止 | 逐步移除,不接收新连接 |

**示例**:
```yaml
endpoints:
- addresses: ["10.244.1.5"]
  conditions:
    ready: false      # Pod 未就绪
    serving: true     # 但允许服务(适用于有状态应用初始化)
    terminating: false
```

---

## 生产案例

### 案例 1: 外部 PostgreSQL 数据库集成

**需求**: 将现有的外部 PostgreSQL 集群映射为 Kubernetes Service

```yaml
# Service 定义
apiVersion: v1
kind: Service
metadata:
  name: postgres
  namespace: database
  labels:
    app: postgres
    external: "true"
  annotations:
    description: "External PostgreSQL production cluster"
    contact: "dba-team@example.com"
spec:
  ports:
  - name: postgres
    protocol: TCP
    port: 5432
    targetPort: 5432
  - name: metrics
    protocol: TCP
    port: 9187
    targetPort: 9187
  sessionAffinity: ClientIP  # 重要: 保持连接到同一节点
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 7200  # 2 小时会话超时
  type: ClusterIP

---
# EndpointSlice 定义(推荐)
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: postgres-primary
  namespace: database
  labels:
    kubernetes.io/service-name: postgres
    role: primary
addressType: IPv4
endpoints:
# 主节点(读写)
- addresses: ["192.0.2.10"]
  conditions:
    ready: true
    serving: true
    terminating: false
  hostname: pg-primary
  nodeName: external-db-1
  zone: us-east-1a
# 从节点 1(只读)
- addresses: ["192.0.2.20"]
  conditions:
    ready: true
    serving: true
    terminating: false
  hostname: pg-replica-1
  nodeName: external-db-2
  zone: us-east-1b
# 从节点 2(只读)
- addresses: ["192.0.2.30"]
  conditions:
    ready: true
    serving: true
    terminating: false
  hostname: pg-replica-2
  nodeName: external-db-3
  zone: us-east-1c
ports:
- name: postgres
  protocol: TCP
  port: 5432
  appProtocol: postgresql
- name: metrics
  protocol: TCP
  port: 9187
  appProtocol: http

---
# 读写分离: 主节点 Service
apiVersion: v1
kind: Service
metadata:
  name: postgres-primary
  namespace: database
spec:
  ports:
  - name: postgres
    port: 5432
  type: ClusterIP

---
apiVersion: v1
kind: Endpoints
metadata:
  name: postgres-primary
  namespace: database
subsets:
- addresses:
  - ip: 192.0.2.10  # 仅主节点
  ports:
  - name: postgres
    port: 5432

---
# 只读副本 Service
apiVersion: v1
kind: Service
metadata:
  name: postgres-readonly
  namespace: database
spec:
  ports:
  - name: postgres
    port: 5432
  type: ClusterIP

---
apiVersion: v1
kind: Endpoints
metadata:
  name: postgres-readonly
  namespace: database
subsets:
- addresses:
  - ip: 192.0.2.20  # 从节点 1
  - ip: 192.0.2.30  # 从节点 2
  ports:
  - name: postgres
    port: 5432
```

**应用使用**:
```yaml
env:
- name: DB_WRITE_HOST
  value: "postgres-primary.database.svc.cluster.local"
- name: DB_READ_HOST
  value: "postgres-readonly.database.svc.cluster.local"
```

---

### 案例 2: 多集群服务桥接(Submariner 风格)

**场景**: 集群 A 访问集群 B 的服务

```yaml
# 集群 A: 创建桥接 Service
apiVersion: v1
kind: Service
metadata:
  name: remote-api
  namespace: multicluster
  annotations:
    multicluster.kubernetes.io/remote-cluster: cluster-b
spec:
  ports:
  - name: https
    port: 443
    targetPort: 8443
  type: ClusterIP

---
# 集群 A: EndpointSlice 指向集群 B 的 Gateway 节点
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: remote-api-cluster-b
  namespace: multicluster
  labels:
    kubernetes.io/service-name: remote-api
    multicluster.kubernetes.io/source-cluster: cluster-b
addressType: IPv4
endpoints:
# 集群 B Gateway 节点 1
- addresses: ["198.51.100.10"]
  conditions: {ready: true, serving: true}
  nodeName: cluster-b-gateway-1
  zone: cluster-b-zone-1
# 集群 B Gateway 节点 2
- addresses: ["198.51.100.20"]
  conditions: {ready: true, serving: true}
  nodeName: cluster-b-gateway-2
  zone: cluster-b-zone-2
ports:
- name: https
  protocol: TCP
  port: 8443  # 集群 B 的 Gateway 暴露端口
```

**集群 B: 实际服务(标准 Service)**:
```yaml
apiVersion: v1
kind: Service
metadata:
  name: api
  namespace: app
  annotations:
    multicluster.kubernetes.io/exported: "true"  # 导出标记
spec:
  selector:
    app: api
  ports:
  - name: https
    port: 8443
  type: ClusterIP
```

---

### 案例 3: 拓扑感知的全球分布式服务

**场景**: 跨多个地理区域的服务,优先路由到本地端点

```yaml
# 全局 Service
apiVersion: v1
kind: Service
metadata:
  name: global-api
  namespace: global
  annotations:
    service.kubernetes.io/topology-aware-hints: "auto"  # 启用拓扑感知
spec:
  selector:
    app: global-api
  ports:
  - name: https
    port: 443
    targetPort: 8443
  internalTrafficPolicy: Local  # 优先本地流量
  type: ClusterIP

---
# EndpointSlice: 美国东部区域
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: global-api-us-east
  namespace: global
  labels:
    kubernetes.io/service-name: global-api
    topology.kubernetes.io/region: us-east
addressType: IPv4
endpoints:
- addresses: ["10.100.1.10"]
  conditions: {ready: true, serving: true}
  zone: us-east-1a
  hints:
    forZones:
    - name: us-east-1a
    - name: us-east-1b
- addresses: ["10.100.1.20"]
  conditions: {ready: true, serving: true}
  zone: us-east-1b
  hints:
    forZones:
    - name: us-east-1a
    - name: us-east-1b
ports:
- name: https
  port: 8443

---
# EndpointSlice: 欧洲西部区域
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: global-api-eu-west
  namespace: global
  labels:
    kubernetes.io/service-name: global-api
    topology.kubernetes.io/region: eu-west
addressType: IPv4
endpoints:
- addresses: ["10.200.1.10"]
  conditions: {ready: true, serving: true}
  zone: eu-west-1a
  hints:
    forZones:
    - name: eu-west-1a
    - name: eu-west-1b
- addresses: ["10.200.1.20"]
  conditions: {ready: true, serving: true}
  zone: eu-west-1b
  hints:
    forZones:
    - name: eu-west-1a
    - name: eu-west-1b
ports:
- name: https
  port: 8443

---
# EndpointSlice: 亚太区域
apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: global-api-ap-south
  namespace: global
  labels:
    kubernetes.io/service-name: global-api
    topology.kubernetes.io/region: ap-south
addressType: IPv4
endpoints:
- addresses: ["10.300.1.10"]
  conditions: {ready: true, serving: true}
  zone: ap-south-1a
  hints:
    forZones:
    - name: ap-south-1a
    - name: ap-south-1b
- addresses: ["10.300.1.20"]
  conditions: {ready: true, serving: true}
  zone: ap-south-1b
  hints:
    forZones:
    - name: ap-south-1a
    - name: ap-south-1b
ports:
- name: https
  port: 8443
```

**路由行为**:
- 位于 `us-east-1a` 的 Pod 访问 Service → 路由到 `10.100.1.10` 或 `10.100.1.20`
- 位于 `eu-west-1b` 的 Pod 访问 Service → 路由到 `10.200.1.10` 或 `10.200.1.20`
- 跨区域访问仅在本地端点不可用时发生

**验证拓扑路由**:
```bash
# 在不同区域的 Pod 中测试
kubectl run test-us -n global --image=curlimages/curl --rm -it \
  --overrides='{"spec":{"nodeSelector":{"topology.kubernetes.io/zone":"us-east-1a"}}}' \
  -- curl -v https://global-api.global.svc.cluster.local
# 应连接到 10.100.1.x

kubectl run test-eu -n global --image=curlimages/curl --rm -it \
  --overrides='{"spec":{"nodeSelector":{"topology.kubernetes.io/zone":"eu-west-1a"}}}' \
  -- curl -v https://global-api.global.svc.cluster.local
# 应连接到 10.200.1.x
```

---

## 相关资源

### 官方文档
- [Endpoints 概念](https://kubernetes.io/docs/concepts/services-networking/service/#endpoints)
- [EndpointSlice 概念](https://kubernetes.io/docs/concepts/services-networking/endpoint-slices/)
- [EndpointSlice API 参考](https://kubernetes.io/docs/reference/kubernetes-api/service-resources/endpoint-slice-v1/)
- [拓扑感知路由](https://kubernetes.io/docs/concepts/services-networking/topology-aware-routing/)

### 控制器
- [Endpoint Controller 代码](https://github.com/kubernetes/kubernetes/tree/master/pkg/controller/endpoint)
- [EndpointSlice Controller 代码](https://github.com/kubernetes/kubernetes/tree/master/pkg/controller/endpointslice)

### 多集群方案
- [Kubernetes Multi-Cluster Services (MCS)](https://github.com/kubernetes/enhancements/tree/master/keps/sig-multicluster/1645-multi-cluster-services-api)
- [Submariner](https://submariner.io/) - 跨集群服务发现
- [Cilium Cluster Mesh](https://docs.cilium.io/en/stable/network/clustermesh/) - 多集群网络

### 工具
- [kubectl-view-service-endpoints](https://github.com/kube-tools/kubectl-view-service-endpoints) - 可视化端点
- [kubectl-slice](https://github.com/patrickdappollonio/kubectl-slice) - EndpointSlice 管理工具

### 性能分析
- [Scaling Kubernetes to 7,500 Nodes](https://openai.com/research/scaling-kubernetes-to-7500-nodes) - EndpointSlice 性能提升案例
- [EndpointSlice Performance Analysis](https://kubernetes.io/blog/2020/09/02/scaling-kubernetes-networking-endpointslices/)

---

**最后更新**: 2026-02  
**维护者**: Kubernetes 运维团队  
**反馈**: 如有问题请提交 Issue
