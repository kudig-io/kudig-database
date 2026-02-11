# 10 - Service 与网络事件

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

> **本文档详细记录 Service、LoadBalancer、Endpoint/EndpointSlice 和网络相关的所有事件。**

---

## 目录

- [一、事件总览](#一事件总览)
- [二、Service 网络架构与事件流](#二service-网络架构与事件流)
- [三、LoadBalancer 生命周期事件](#三loadbalancer-生命周期事件)
- [四、Endpoint 与 EndpointSlice 事件](#四endpoint-与-endpointslice-事件)
- [五、网络配置与资源分配事件](#五网络配置与资源分配事件)
- [六、综合排查案例](#六综合排查案例)
- [七、生产环境最佳实践](#七生产环境最佳实践)

---

## 一、事件总览

### 1.1 本文档覆盖的事件列表

| 事件原因 (Reason) | 类型 | 生产频率 | 适用版本 | 简要说明 |
|:---|:---|:---|:---|:---|
| **LoadBalancer 生命周期事件** | | | | |
| `EnsuringLoadBalancer` | Normal | 中频 | v1.0+ | 正在确保 LoadBalancer 存在 |
| `EnsuredLoadBalancer` | Normal | 中频 | v1.0+ | LoadBalancer 已确保存在 |
| `CreatingLoadBalancer` | Normal | 中频 | v1.0+ | 正在创建 LoadBalancer |
| `CreatedLoadBalancer` | Normal | 低频 | v1.0+ | LoadBalancer 创建成功 |
| `DeletingLoadBalancer` | Normal | 低频 | v1.0+ | 正在删除 LoadBalancer |
| `DeletedLoadBalancer` | Normal | 低频 | v1.0+ | LoadBalancer 删除成功 |
| `UpdatedLoadBalancer` | Normal | 低频 | v1.0+ | LoadBalancer 更新成功 |
| `UpdateLoadBalancerFailed` | Warning | 低频 | v1.0+ | LoadBalancer 更新失败 |
| `DeleteLoadBalancerFailed` | Warning | 低频 | v1.0+ | LoadBalancer 删除失败 |
| `UnAvailableLoadBalancer` | Warning | 低频 | v1.0+ | LoadBalancer 无可用后端 |
| `SyncLoadBalancerFailed` | Warning | 低频 | v1.0+ | LoadBalancer 同步失败 |
| **Endpoint/EndpointSlice 事件** | | | | |
| `FailedToCreateEndpoint` | Warning | 低频 | v1.0+ | Endpoint 创建失败 |
| `FailedToUpdateEndpoint` | Warning | 低频 | v1.0+ | Endpoint 更新失败 |
| `FailedToDeleteEndpoint` | Warning | 罕见 | v1.0+ | Endpoint 删除失败 |
| `FailedToCreateEndpointSlice` | Warning | 低频 | v1.17+ | EndpointSlice 创建失败 |
| `FailedToUpdateEndpointSlice` | Warning | 低频 | v1.17+ | EndpointSlice 更新失败 |
| `FailedToDeleteEndpointSlice` | Warning | 罕见 | v1.17+ | EndpointSlice 删除失败 |
| **网络配置事件** | | | | |
| `HostPortConflict` | Warning | 低频 | v1.0+ | 主机端口冲突 |
| `DNSConfigForming` | Warning | 低频 | v1.9+ | DNS 配置生成失败 |
| `IPAllocated` | Normal | 低频 | v1.24+ | IP 地址分配成功 |
| `IPNotAllocated` | Warning | 低频 | v1.24+ | IP 地址分配失败 |

**事件来源组件**:
- **service-controller**: Service 和 LoadBalancer 管理
- **endpoint-controller**: Endpoint 管理
- **endpointslice-controller**: EndpointSlice 管理 (v1.17+)
- **kubelet**: 网络配置相关事件
- **cloud-controller-manager**: 云厂商 LoadBalancer 集成

### 1.2 快速索引

| 问题场景 | 关注事件 | 跳转章节 |
|:---|:---|:---|
| LoadBalancer 无法创建 | `SyncLoadBalancerFailed`, `CreatingLoadBalancer` | [三.1](#31-ensuringloadbalancer--ensuringloadbalancer---确保loadbalancer存在) |
| Service 无后端流量 | `UnAvailableLoadBalancer`, `FailedToUpdateEndpoint` | [三.7](#37-unavailableloadbalancer---loadbalancer-无可用后端) |
| Endpoint 未更新 | `FailedToUpdateEndpoint`, `FailedToUpdateEndpointSlice` | [四.2](#42-failedtoupdateendpoint--failedtoupdateendpointslice---endpoint更新失败) |
| 主机端口冲突 | `HostPortConflict` | [五.1](#51-hostportconflict---主机端口冲突) |
| Pod DNS 解析失败 | `DNSConfigForming` | [五.2](#52-dnsconfigforming---dns-配置生成失败) |

---

## 二、Service 网络架构与事件流

### 2.1 Service 类型与事件关系

Kubernetes 中 Service 有多种类型,每种类型产生的事件不同:

| Service 类型 | 产生的主要事件 | 事件来源 | 特点 |
|:---|:---|:---|:---|
| **ClusterIP** | `FailedToCreateEndpoint`, `FailedToUpdateEndpoint` | endpoint-controller | 仅集群内部访问,不涉及 LoadBalancer 事件 |
| **NodePort** | 同 ClusterIP + `HostPortConflict` (罕见) | endpoint-controller, kubelet | 通过节点端口暴露,可能产生端口冲突 |
| **LoadBalancer** | 所有 LoadBalancer 事件 | service-controller, cloud-controller-manager | 云厂商提供外部负载均衡器 |
| **ExternalName** | 无 | - | 仅 DNS CNAME 映射,不产生网络事件 |

### 2.2 LoadBalancer Service 完整生命周期

```
用户操作                          控制器事件                         外部资源状态
═══════════════════════════════════════════════════════════════════════════════════

┌───────────────┐
│ kubectl apply │ ──▶ Service 创建
│  Service.yaml │
└───────────────┘
                                  ┌─────────────────────┐
                                  │ EnsuringLoadBalancer│  ◀── service-controller 开始协调
                                  └──────────┬──────────┘
                                             │
                                             │ 调用云厂商 API
                                             │
                                  ┌──────────▼──────────┐
                                  │ CreatingLoadBalancer│  ◀── 向云厂商发起创建请求
                                  └──────────┬──────────┘
                                             │
                                             │ 等待云厂商资源创建完成
                                             │ (可能需要 1-5 分钟)
                                             │
                    [云厂商]                 │                   [云厂商]
                 创建 LB 实例 ─────────────▶ │ ◀────────────── 返回 External IP
                 注册后端节点                │                   配置健康检查
                 配置监听端口                │                   开启流量转发
                                             │
                                  ┌──────────▼──────────┐
                                  │ CreatedLoadBalancer │  ◀── LB 创建成功
                                  └──────────┬──────────┘
                                             │
                                  ┌──────────▼──────────┐
                                  │EnsuredLoadBalancer  │  ◀── LB 已就绪,External IP 已分配
                                  └─────────────────────┘
                                             │
                                             │ Service.status.loadBalancer.ingress
                                             │ 字段被更新
                                             │
┌─────────────────┐                          │
│ kubectl get svc │ ──────────────────────────────────────▶ EXTERNAL-IP 字段显示
│  my-service     │                                          (从 <pending> 变为实际 IP)
└─────────────────┘

[运行中状态]
  │
  │ 定期协调 (每 10 秒)
  │
  ├──▶ 检查后端 Endpoints 变化 ──▶ UpdatedLoadBalancer (如需更新)
  │
  └──▶ 检查 Service 配置变化 ──▶ UpdatedLoadBalancer (如需更新)


[删除流程]

┌───────────────┐
│ kubectl delete│ ──▶ Service 删除
│  service      │
└───────────────┘
                                  ┌─────────────────────┐
                                  │ DeletingLoadBalancer│  ◀── 开始删除 LB
                                  └──────────┬──────────┘
                                             │
                                             │ 调用云厂商删除 API
                                             │
                    [云厂商]                 │
                 移除后端节点 ◀───────────── │
                 停止流量转发                │
                 删除 LB 实例                │
                                             │
                                  ┌──────────▼──────────┐
                                  │ DeletedLoadBalancer │  ◀── LB 删除成功
                                  └─────────────────────┘
                                             │
                                             ▼
                                      etcd 中 Service 对象被移除
```

### 2.3 Endpoint vs EndpointSlice

| 对比维度 | Endpoints (v1.0+) | EndpointSlice (v1.17+ GA v1.21) |
|:---|:---|:---|
| **API 版本** | core/v1 | discovery.k8s.io/v1 |
| **扩展性** | 单个对象存储所有端点 | 多个对象分片存储 (默认 100 端点/slice) |
| **适用规模** | < 1000 端点 | 任意规模 (支持 10000+ 端点) |
| **etcd 压力** | 高 (大对象频繁更新) | 低 (仅更新变化的 slice) |
| **控制器** | endpoint-controller | endpointslice-controller |
| **事件前缀** | `FailedToCreateEndpoint`, `FailedToUpdateEndpoint` | `FailedToCreateEndpointSlice`, `FailedToUpdateEndpointSlice` |
| **推荐使用** | 遗留系统兼容 | **新集群推荐** (v1.21+) |

**EndpointSlice 分片示例**:
```yaml
# Service: my-service (100 个 Pod)
# 生成 1 个 EndpointSlice 对象

apiVersion: discovery.k8s.io/v1
kind: EndpointSlice
metadata:
  name: my-service-abc123
  labels:
    kubernetes.io/service-name: my-service
addressType: IPv4
endpoints:
  - addresses: ["10.244.1.10"]
    conditions: {ready: true}
    targetRef: {kind: Pod, name: my-app-1}
  - addresses: ["10.244.2.20"]
    conditions: {ready: true}
    targetRef: {kind: Pod, name: my-app-2}
  # ... 最多 100 个 endpoints
ports:
  - name: http
    port: 8080
    protocol: TCP
```

**当 Service 有 1000 个 Pod 时**:
- **Endpoints**: 1 个巨大对象 (~500KB),每次 Pod 变化都需要更新整个对象
- **EndpointSlice**: 10 个对象 (每个 ~50KB),仅更新变化的那个 slice

### 2.4 Service 事件流程图

```
Service 创建/更新
       │
       ▼
┌─────────────────────────────────────────┐
│   service-controller (watch Service)    │
├─────────────────────────────────────────┤
│ 1. 检测 Service.spec.type               │
│ 2. 如果是 LoadBalancer:                 │
│    ├─ 调用云厂商 API                    │
│    └─ 产生 LoadBalancer 事件            │
│ 3. 更新 Service.status.loadBalancer    │
└─────────────────────────────────────────┘
       │
       ├──▶ EnsuringLoadBalancer (开始)
       ├──▶ CreatingLoadBalancer (进行中)
       ├──▶ CreatedLoadBalancer (成功)
       └──▶ EnsuredLoadBalancer (完成)

┌─────────────────────────────────────────┐
│ endpoint-controller (watch Service+Pod) │
├─────────────────────────────────────────┤
│ 1. 监听 Service 的 selector            │
│ 2. 监听匹配的 Pod 状态                 │
│ 3. 生成/更新 Endpoints 对象            │
│ 4. 产生 Endpoint 事件 (如失败)         │
└─────────────────────────────────────────┘
       │
       ├──▶ FailedToCreateEndpoint (创建失败)
       └──▶ FailedToUpdateEndpoint (更新失败)

┌─────────────────────────────────────────┐
│ endpointslice-controller (v1.17+)      │
├─────────────────────────────────────────┤
│ 1. 监听 Service 和 Pod                 │
│ 2. 生成/更新 EndpointSlice 对象       │
│ 3. 自动分片 (默认 100 端点/slice)     │
│ 4. 产生 EndpointSlice 事件 (如失败)   │
└─────────────────────────────────────────┘
       │
       ├──▶ FailedToCreateEndpointSlice (创建失败)
       └──▶ FailedToUpdateEndpointSlice (更新失败)

┌─────────────────────────────────────────┐
│  kube-proxy (每个节点)                   │
├─────────────────────────────────────────┤
│ 1. 监听 Service 和 Endpoints/Slices    │
│ 2. 配置 iptables/ipvs 规则              │
│ 3. 实现 ClusterIP 和 NodePort 访问     │
└─────────────────────────────────────────┘
       │
       └──▶ 不产生 Event 对象
            (问题排查需查看 kube-proxy 日志)
```

---

## 三、LoadBalancer 生命周期事件

### 3.1 `EnsuringLoadBalancer` / `EnsuredLoadBalancer` - 确保LoadBalancer存在

| 属性 | EnsuringLoadBalancer | EnsuredLoadBalancer |
|:---|:---|:---|
| **事件类型** | Normal | Normal |
| **来源组件** | service-controller | service-controller |
| **关联资源** | Service | Service |
| **适用版本** | v1.0+ | v1.0+ |
| **生产频率** | 中频 | 中频 |

#### 事件含义

**EnsuringLoadBalancer**: service-controller 开始协调 LoadBalancer 类型的 Service,确保云厂商的负载均衡器资源与 Service 定义一致。

**EnsuredLoadBalancer**: LoadBalancer 资源已确保存在且配置正确,External IP 已分配到 Service.status.loadBalancer.ingress 字段。

**触发时机**:
- Service 类型为 `LoadBalancer` 时
- Service 的 `spec.ports`, `spec.sessionAffinity` 等字段变更时
- 定期协调 (默认每 10 秒,由 `--node-sync-period` 控制)
- 集群中 Node 状态变化时 (后端节点列表变化)

#### 典型事件消息

```bash
$ kubectl describe service my-loadbalancer-svc

Events:
  Type    Reason                 Age   From                Message
  ----    ------                 ----  ----                -------
  Normal  EnsuringLoadBalancer   30s   service-controller  Ensuring load balancer
  Normal  EnsuredLoadBalancer    25s   service-controller  Ensured load balancer

$ kubectl get svc my-loadbalancer-svc
NAME                   TYPE           CLUSTER-IP      EXTERNAL-IP     PORT(S)        AGE
my-loadbalancer-svc    LoadBalancer   10.96.100.50    203.0.113.10    80:30080/TCP   1m
```

**Service 状态字段**:
```yaml
status:
  loadBalancer:
    ingress:
      - ip: 203.0.113.10  # AWS ELB 使用 hostname 字段
```

#### 影响面说明

- **用户影响**: 无 - 正常操作流程
- **服务影响**: 
  - `EnsuringLoadBalancer` 期间 External IP 显示为 `<pending>`
  - `EnsuredLoadBalancer` 后 External IP 可用,流量可达
- **集群影响**: 无
- **关联事件链**: `EnsuringLoadBalancer` → `CreatingLoadBalancer` (如 LB 不存在) → `CreatedLoadBalancer` → `EnsuredLoadBalancer`

#### 排查建议

```bash
# 1. 查看 Service 状态
kubectl get svc my-loadbalancer-svc -o wide

# 2. 查看 External IP 分配情况
kubectl get svc my-loadbalancer-svc -o jsonpath='{.status.loadBalancer.ingress}'

# 3. 检查云厂商 LoadBalancer 资源
# AWS ELB 示例:
aws elb describe-load-balancers --load-balancer-names <lb-name>

# Azure Load Balancer 示例:
az network lb show --resource-group <rg> --name <lb-name>

# 4. 查看 service-controller 日志 (在 kube-controller-manager 中)
kubectl logs -n kube-system kube-controller-manager-<node> | grep service-controller
```

#### 解决建议

**该事件为正常操作,无需处理。如果长时间停留在 `EnsuringLoadBalancer` 状态,参考后续的失败事件排查。**

---

### 3.2 `CreatingLoadBalancer` / `CreatedLoadBalancer` - 创建LoadBalancer

| 属性 | CreatingLoadBalancer | CreatedLoadBalancer |
|:---|:---|:---|
| **事件类型** | Normal | Normal |
| **来源组件** | service-controller | service-controller |
| **关联资源** | Service | Service |
| **适用版本** | v1.0+ | v1.0+ |
| **生产频率** | 中频 | 低频 |

#### 事件含义

**CreatingLoadBalancer**: service-controller 正在调用云厂商 API 创建新的 LoadBalancer 资源。

**CreatedLoadBalancer**: LoadBalancer 资源创建成功,External IP 已分配。

**创建流程**:
1. service-controller 检测到新的 LoadBalancer 类型 Service
2. 调用云厂商 API 创建负载均衡器
3. 等待云厂商返回 External IP/Hostname
4. 更新 Service.status.loadBalancer.ingress
5. 配置后端节点 (所有 Ready 状态的 Node)
6. 配置健康检查 (基于 NodePort)

#### 典型事件消息

```bash
$ kubectl describe service my-new-lb-svc

Events:
  Type    Reason                  Age   From                Message
  ----    ------                  ----  ----                -------
  Normal  EnsuringLoadBalancer    60s   service-controller  Ensuring load balancer
  Normal  CreatingLoadBalancer    55s   service-controller  Creating load balancer
  Normal  CreatedLoadBalancer     10s   service-controller  Created load balancer
  Normal  EnsuredLoadBalancer     10s   service-controller  Ensured load balancer
```

**AWS 云厂商示例 (ELB)**:
```bash
Events:
  Normal  CreatingLoadBalancer  60s  service-controller  Creating load balancer
  Normal  CreatedLoadBalancer   5s   service-controller  Created load balancer a1b2c3d4e5f6g7h8.us-west-2.elb.amazonaws.com
```

**Azure 云厂商示例**:
```bash
Events:
  Normal  CreatingLoadBalancer  60s  service-controller  Creating load balancer
  Normal  CreatedLoadBalancer   10s  service-controller  Created load balancer with public IP 52.168.100.50
```

#### 影响面说明

- **用户影响**: 无 - 正常创建流程
- **服务影响**: 创建期间 (通常 1-5 分钟) External IP 不可用
- **集群影响**: 无
- **关联事件链**: `EnsuringLoadBalancer` → `CreatingLoadBalancer` → `CreatedLoadBalancer` → `EnsuredLoadBalancer`

#### 排查建议

```bash
# 1. 查看 Service 创建时间和事件
kubectl describe service my-new-lb-svc

# 2. 检查云厂商 LoadBalancer 创建状态
# AWS:
aws elb describe-load-balancers --query "LoadBalancerDescriptions[?contains(LoadBalancerName, 'k8s')]"

# Azure:
az network lb list --query "[?tags.service=='my-new-lb-svc']"

# GCP:
gcloud compute forwarding-rules list --filter="description~my-new-lb-svc"

# 3. 检查 cloud-controller-manager 日志 (如果使用外部 CCM)
kubectl logs -n kube-system cloud-controller-manager-<pod> | grep my-new-lb-svc
```

#### 解决建议

**该事件为正常操作。如果创建失败,会产生 `SyncLoadBalancerFailed` 事件,参考后续章节排查。**

**创建时间过长 (>5分钟) 的可能原因**:
- 云厂商 API 限流 (Rate Limiting)
- 云厂商资源配额不足
- 网络配置错误 (VPC/子网配置)
- cloud-controller-manager 异常

---

### 3.3 `UpdatedLoadBalancer` - LoadBalancer更新成功

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | service-controller |
| **关联资源** | Service |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 |

#### 事件含义

LoadBalancer 配置已成功更新,通常是因为 Service 的 ports、sessionAffinity 或后端 Nodes 发生变化。

**触发更新的场景**:
- Service.spec.ports 变化 (添加/删除/修改端口)
- Service.spec.sessionAffinity 变化 (None ↔ ClientIP)
- Service.spec.sessionAffinityConfig 变化 (会话超时时间)
- 后端 Node 节点增删 (Node 加入/离开集群)
- Service annotations 变化 (云厂商特定配置)

#### 典型事件消息

```bash
$ kubectl describe service my-lb-svc

Events:
  Type    Reason               Age   From                Message
  ----    ------               ----  ----                -------
  Normal  UpdatedLoadBalancer  10s   service-controller  Updated load balancer with new hosts
  Normal  UpdatedLoadBalancer  5s    service-controller  Updated load balancer with new ports
```

#### 影响面说明

- **用户影响**: 无 - 更新过程对流量影响极小
- **服务影响**: 
  - 端口变更: 旧端口立即不可用,新端口生效 (秒级)
  - 后端节点变更: 流量平滑迁移 (取决于云厂商实现)
- **集群影响**: 无
- **关联事件链**: (Service 配置变更) → `EnsuringLoadBalancer` → `UpdatedLoadBalancer` → `EnsuredLoadBalancer`

#### 排查建议

```bash
# 1. 查看 Service 配置变更历史
kubectl describe service my-lb-svc

# 2. 对比 Service 当前配置与云厂商 LoadBalancer 配置
# AWS:
aws elb describe-load-balancers --load-balancer-names <lb-name> --query "LoadBalancerDescriptions[0].{Listeners:ListenerDescriptions,Instances:Instances}"

# 3. 验证新配置是否生效
# 测试新端口:
curl http://<EXTERNAL-IP>:<NEW-PORT>

# 4. 检查后端节点健康状态
kubectl get nodes -o wide
kubectl get svc my-lb-svc -o jsonpath='{.status.loadBalancer}'
```

#### 解决建议

**该事件为正常操作,表明 LoadBalancer 配置已同步成功。**

---

### 3.4 `DeletingLoadBalancer` / `DeletedLoadBalancer` - 删除LoadBalancer

| 属性 | DeletingLoadBalancer | DeletedLoadBalancer |
|:---|:---|:---|
| **事件类型** | Normal | Normal |
| **来源组件** | service-controller | service-controller |
| **关联资源** | Service | Service |
| **适用版本** | v1.0+ | v1.0+ |
| **生产频率** | 低频 | 低频 |

#### 事件含义

**DeletingLoadBalancer**: service-controller 正在调用云厂商 API 删除 LoadBalancer 资源。

**DeletedLoadBalancer**: LoadBalancer 资源已成功删除,云厂商资源已释放。

**删除流程**:
1. 用户删除 Service 或将类型从 LoadBalancer 改为 ClusterIP
2. service-controller 检测到变更
3. 调用云厂商 API 删除负载均衡器
4. 等待云厂商确认删除完成
5. Service 对象从 etcd 中删除

#### 典型事件消息

```bash
$ kubectl describe service my-lb-svc

Events:
  Type    Reason                  Age   From                Message
  ----    ------                  ----  ----                -------
  Normal  DeletingLoadBalancer    30s   service-controller  Deleting load balancer
  Normal  DeletedLoadBalancer     5s    service-controller  Deleted load balancer
```

#### 影响面说明

- **用户影响**: **高** - External IP 不可用,外部流量无法访问
- **服务影响**: **严重** - LoadBalancer 删除后流量立即中断
- **集群影响**: 无
- **关联事件链**: (Service 删除) → `DeletingLoadBalancer` → `DeletedLoadBalancer`

#### 排查建议

```bash
# 1. 确认 Service 是否被意外删除
kubectl get svc -A | grep my-lb-svc

# 2. 检查 Service 事件历史 (如果 Service 还存在)
kubectl describe service my-lb-svc

# 3. 验证云厂商 LoadBalancer 是否已删除
# AWS:
aws elb describe-load-balancers --query "LoadBalancerDescriptions[?contains(LoadBalancerName, 'my-lb')]"

# 4. 检查审计日志,确认删除操作来源
kubectl get events -A --field-selector involvedObject.kind=Service,involvedObject.name=my-lb-svc,reason=DeletingLoadBalancer
```

#### 解决建议

| 问题场景 | 可能原因 | 解决方案 |
|:---|:---|:---|
| Service 被意外删除 | 误操作或自动化脚本错误 | 从备份恢复 Service YAML 并重新创建 |
| LoadBalancer 删除但云资源残留 | 云厂商 API 错误或权限不足 | 手动删除云厂商 LoadBalancer 资源 |
| Service 类型被改为 ClusterIP | 配置错误 | 修正 Service.spec.type 为 LoadBalancer |

**恢复 LoadBalancer Service**:
```bash
# 1. 重新创建 Service
kubectl apply -f my-lb-svc.yaml

# 2. 等待 LoadBalancer 创建完成
kubectl get svc my-lb-svc -w

# 3. 验证 External IP 已分配
kubectl get svc my-lb-svc -o jsonpath='{.status.loadBalancer.ingress}'
```

---

### 3.5 `UpdateLoadBalancerFailed` - LoadBalancer更新失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | service-controller |
| **关联资源** | Service |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 ⚠️ |

#### 事件含义

service-controller 尝试更新 LoadBalancer 配置时失败,可能是云厂商 API 错误、权限不足或配置冲突。

**常见失败原因**:
1. **云厂商 API 限流** - 请求过于频繁
2. **权限不足** - ServiceAccount 缺少必要的云资源操作权限
3. **配置冲突** - 端口已被其他 LoadBalancer 使用
4. **资源配额超限** - 云账号 LoadBalancer 数量达到上限
5. **网络配置错误** - 子网、安全组配置问题

#### 典型事件消息

```bash
$ kubectl describe service my-lb-svc

Events:
  Type     Reason                    Age   From                Message
  ----     ------                    ----  ----                -------
  Normal   EnsuringLoadBalancer      60s   service-controller  Ensuring load balancer
  Warning  UpdateLoadBalancerFailed  30s   service-controller  Error updating load balancer with new hosts map[10.0.1.5:{}]: failed to ensure load balancer: RequestLimitExceeded: Request limit exceeded
  Warning  UpdateLoadBalancerFailed  10s   service-controller  Error updating load balancer: UnauthorizedOperation: You are not authorized to perform this operation
```

**message 格式变体**:

| message 关键词 | 根本原因 | 云厂商 |
|:---|:---|:---|
| `RequestLimitExceeded` | API 请求限流 | AWS |
| `UnauthorizedOperation` | 权限不足 | AWS |
| `QuotaExceeded` | 配额超限 | GCP |
| `AuthorizationFailed` | 认证失败 | Azure |
| `InvalidParameterValue` | 配置参数错误 | AWS, Azure |

#### 影响面说明

- **用户影响**: 中 - LoadBalancer 配置无法更新,可能导致流量异常
- **服务影响**: 
  - 如果是端口更新失败: 新端口不可用
  - 如果是后端节点更新失败: 新节点不接收流量
- **集群影响**: 无
- **关联事件链**: `EnsuringLoadBalancer` → `UpdateLoadBalancerFailed` (循环重试,每 10 秒)

#### 排查建议

```bash
# 1. 查看完整错误消息
kubectl describe service my-lb-svc | grep -A 5 "UpdateLoadBalancerFailed"

# 2. 检查 cloud-controller-manager 日志
kubectl logs -n kube-system cloud-controller-manager-<pod> --tail=100 | grep "UpdateLoadBalancer"

# 3. 验证云厂商权限
# AWS: 检查 IAM Role 权限
aws iam get-role --role-name <node-role>

# Azure: 检查 Service Principal 权限
az role assignment list --assignee <sp-id>

# 4. 检查云厂商 API 限流状态
# AWS CloudWatch: ThrottleExceptions 指标
# GCP: Stackdriver 日志中的 rateLimitExceeded

# 5. 查看 Service annotations (云厂商特定配置)
kubectl get svc my-lb-svc -o jsonpath='{.metadata.annotations}'
```

#### 解决建议

| 错误消息关键词 | 根本原因 | 排查方法 | 解决方案 |
|:---|:---|:---|:---|
| **RequestLimitExceeded** | API 请求限流 | 查看云厂商 API 调用日志 | 1. 减少 Service 更新频率<br>2. 增加云账号 API 限流配额<br>3. 分散 Service 更新时间 |
| **UnauthorizedOperation** | IAM 权限不足 (AWS) | 检查 Node IAM Role 权限 | 添加必要权限:<br>- `elasticloadbalancing:*`<br>- `ec2:DescribeInstances`<br>- `ec2:DescribeSecurityGroups` |
| **QuotaExceeded** | LoadBalancer 配额超限 | 查看云账号配额使用情况 | 1. 删除不使用的 LoadBalancer<br>2. 申请提升配额 |
| **InvalidParameterValue** | Service annotation 配置错误 | 检查云厂商特定 annotation | 修正 annotation 值,参考云厂商文档 |
| **AuthorizationFailed** | Service Principal 权限不足 (Azure) | 检查 RBAC 分配 | 分配 `Network Contributor` 角色 |

**AWS 权限配置示例**:
```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "elasticloadbalancing:CreateLoadBalancer",
        "elasticloadbalancing:DeleteLoadBalancer",
        "elasticloadbalancing:DescribeLoadBalancers",
        "elasticloadbalancing:ConfigureHealthCheck",
        "elasticloadbalancing:RegisterInstancesWithLoadBalancer",
        "elasticloadbalancing:DeregisterInstancesFromLoadBalancer",
        "ec2:DescribeInstances",
        "ec2:DescribeSecurityGroups"
      ],
      "Resource": "*"
    }
  ]
}
```

**Azure Service Principal 权限配置**:
```bash
# 为 Service Principal 分配 Network Contributor 角色
az role assignment create \
  --assignee <sp-id> \
  --role "Network Contributor" \
  --scope /subscriptions/<subscription-id>/resourceGroups/<rg>
```

---

### 3.6 `DeleteLoadBalancerFailed` - LoadBalancer删除失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | service-controller |
| **关联资源** | Service |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 ⚠️ |

#### 事件含义

service-controller 尝试删除 LoadBalancer 资源时失败,可能导致云厂商资源残留,产生不必要的费用。

**常见失败原因**:
1. **LoadBalancer 仍有活动连接** - 云厂商拒绝删除
2. **权限不足** - 无法删除云资源
3. **云厂商 API 错误** - 临时性故障
4. **LoadBalancer 已被手动删除** - 云资源与 Kubernetes 状态不同步
5. **依赖资源未清理** - 如安全组、目标组等

#### 典型事件消息

```bash
$ kubectl describe service my-lb-svc

Events:
  Type     Reason                    Age   From                Message
  ----     ------                    ----  ----                -------
  Normal   DeletingLoadBalancer      60s   service-controller  Deleting load balancer
  Warning  DeleteLoadBalancerFailed  30s   service-controller  Error deleting load balancer: LoadBalancerNotFound: The specified load balancer does not exist
  Warning  DeleteLoadBalancerFailed  10s   service-controller  Error deleting load balancer: DependencyViolation: Cannot delete load balancer with active targets
```

#### 影响面说明

- **用户影响**: 低 - Service 删除流程受阻
- **服务影响**: 低 - Service 已不可用,但云资源残留
- **集群影响**: 低 - 可能导致云资源泄漏,增加费用
- **关联事件链**: `DeletingLoadBalancer` → `DeleteLoadBalancerFailed` (循环重试)

#### 排查建议

```bash
# 1. 查看完整错误消息
kubectl describe service my-lb-svc | grep -A 5 "DeleteLoadBalancerFailed"

# 2. 检查云厂商 LoadBalancer 是否仍存在
# AWS:
aws elb describe-load-balancers --query "LoadBalancerDescriptions[?contains(LoadBalancerName, 'k8s')]"

# 3. 检查 LoadBalancer 的依赖资源
# AWS ELB 后端实例:
aws elb describe-instance-health --load-balancer-name <lb-name>

# AWS Target Group (NLB/ALB):
aws elbv2 describe-target-health --target-group-arn <tg-arn>

# 4. 检查 Service finalizers (阻止删除的标记)
kubectl get svc my-lb-svc -o jsonpath='{.metadata.finalizers}'
# 输出示例: ["service.kubernetes.io/load-balancer-cleanup"]
```

#### 解决建议

| 错误消息关键词 | 根本原因 | 解决方案 |
|:---|:---|:---|
| **LoadBalancerNotFound** | LoadBalancer 已被手动删除 | 1. 移除 Service finalizer:<br>`kubectl patch svc my-lb-svc -p '{"metadata":{"finalizers":[]}}' --type=merge`<br>2. 删除 Service:<br>`kubectl delete svc my-lb-svc --force --grace-period=0` |
| **DependencyViolation** | 后端实例或目标组未清理 | 1. 手动注销后端实例<br>2. 删除目标组<br>3. 重试删除 Service |
| **UnauthorizedOperation** | 权限不足 | 添加删除 LoadBalancer 的 IAM 权限 |
| **RequestLimitExceeded** | API 限流 | 等待限流恢复,稍后重试 |

**强制删除 Service 并手动清理云资源**:
```bash
# 1. 移除 finalizer (允许 Kubernetes 删除 Service 对象)
kubectl patch svc my-lb-svc -p '{"metadata":{"finalizers":[]}}' --type=merge

# 2. 删除 Service
kubectl delete svc my-lb-svc

# 3. 手动删除云厂商 LoadBalancer
# AWS Classic Load Balancer:
aws elb delete-load-balancer --load-balancer-name <lb-name>

# AWS Network Load Balancer:
aws elbv2 delete-load-balancer --load-balancer-arn <lb-arn>

# 4. 验证删除成功
aws elb describe-load-balancers --load-balancer-names <lb-name>
# 输出: LoadBalancerNotFound (成功)
```

---

### 3.7 `UnAvailableLoadBalancer` - LoadBalancer无可用后端

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | service-controller |
| **关联资源** | Service |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 ⚠️ |

#### 事件含义

LoadBalancer 的后端节点列表为空,或所有后端节点健康检查失败,导致 LoadBalancer 无法转发流量。

**触发条件**:
- 集群中没有 Ready 状态的 Node
- 所有 Node 的 NodePort 健康检查失败
- Service 的 Endpoints 为空 (没有匹配的 Pod)
- Service 的 `externalTrafficPolicy: Local` 但本地节点无 Pod

#### 典型事件消息

```bash
$ kubectl describe service my-lb-svc

Events:
  Type     Reason                   Age   From                Message
  ----     ------                   ----  ----                -------
  Warning  UnAvailableLoadBalancer  30s   service-controller  There are no available nodes for LoadBalancer
  Warning  UnAvailableLoadBalancer  10s   service-controller  All backend nodes are unhealthy
```

#### 影响面说明

- **用户影响**: **高** - 服务完全不可用,外部流量无法访问
- **服务影响**: **严重** - LoadBalancer 无法转发任何流量
- **集群影响**: **警示** - 可能表明集群级别问题 (所有节点故障)
- **关联事件链**: 
  - `NodeNotReady` (Node 事件) → `UnAvailableLoadBalancer` (Service 事件)
  - `FailedToUpdateEndpoint` → `UnAvailableLoadBalancer`

#### 排查建议

```bash
# 1. 检查集群节点状态
kubectl get nodes
# 查看是否有 Ready 状态的节点

# 2. 检查 Service 的 Endpoints
kubectl get endpoints my-lb-svc
# 查看 ENDPOINTS 列是否为空

# 3. 检查匹配 Service selector 的 Pod
kubectl get pods -l app=my-app
# 查看是否有 Running 且 Ready 的 Pod

# 4. 检查云厂商 LoadBalancer 的后端健康状态
# AWS:
aws elb describe-instance-health --load-balancer-name <lb-name>
# 查看 State 字段: InService / OutOfService

# 5. 检查 NodePort 是否可访问
NODE_PORT=$(kubectl get svc my-lb-svc -o jsonpath='{.spec.ports[0].nodePort}')
NODE_IP=$(kubectl get nodes -o jsonpath='{.items[0].status.addresses[?(@.type=="ExternalIP")].address}')
curl http://$NODE_IP:$NODE_PORT
```

#### 解决建议

| 问题场景 | 根本原因 | 排查方法 | 解决方案 |
|:---|:---|:---|:---|
| **Endpoints 为空** | 无匹配的 Pod | `kubectl get pods -l <selector>` | 1. 检查 Service selector 是否正确<br>2. 确保 Deployment 的 Pod 正在运行 |
| **Pod 未 Ready** | Pod 健康检查失败 | `kubectl describe pod <pod>` | 1. 修复 Pod 启动问题<br>2. 调整 readinessProbe 配置 |
| **所有 Node NotReady** | 节点故障 | `kubectl describe node` | 1. 修复节点问题<br>2. 添加新节点 |
| **NodePort 不可访问** | kube-proxy 故障 | 检查 kube-proxy 日志 | 重启 kube-proxy DaemonSet |
| **externalTrafficPolicy: Local 无本地 Pod** | Pod 未调度到节点 | `kubectl get pods -o wide` | 1. 调整 Pod affinity<br>2. 增加 Pod 副本数 |
| **云厂商健康检查失败** | 健康检查配置错误 | 查看云厂商控制台 | 调整健康检查路径/端口/超时 |

**案例 1: Service selector 错误导致 Endpoints 为空**:
```bash
# 现象: LoadBalancer 无可用后端
kubectl get endpoints my-lb-svc
# 输出: NAME         ENDPOINTS   AGE
#       my-lb-svc    <none>      5m

# 排查: 检查 Service selector
kubectl get svc my-lb-svc -o jsonpath='{.spec.selector}'
# 输出: {"app":"my-app"}

# 检查 Pod labels
kubectl get pods --show-labels | grep my-app
# 输出: my-app-7d5bc-xyz12   1/1   Running   app=myapp (注意 label 是 myapp 不是 my-app)

# 解决: 修正 Service selector
kubectl patch svc my-lb-svc -p '{"spec":{"selector":{"app":"myapp"}}}'

# 验证 Endpoints 已生成
kubectl get endpoints my-lb-svc
# 输出: NAME         ENDPOINTS           AGE
#       my-lb-svc    10.244.1.10:8080    5m
```

**案例 2: externalTrafficPolicy: Local 无本地 Pod**:
```bash
# 现象: 某些节点的 LoadBalancer 后端显示 Unhealthy

# 排查:
kubectl get svc my-lb-svc -o jsonpath='{.spec.externalTrafficPolicy}'
# 输出: Local

# 检查 Pod 分布
kubectl get pods -l app=my-app -o wide
# 输出: 
# NAME                READY   STATUS    NODE
# my-app-1            1/1     Running   node-01
# my-app-2            1/1     Running   node-01
# (所有 Pod 都在 node-01,其他节点无 Pod)

# 解决方案 1: 改为 Cluster 模式 (流量可到达任意节点)
kubectl patch svc my-lb-svc -p '{"spec":{"externalTrafficPolicy":"Cluster"}}'

# 解决方案 2: 使用 Pod 反亲和性分散 Pod
apiVersion: apps/v1
kind: Deployment
spec:
  template:
    spec:
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchLabels:
                    app: my-app
                topologyKey: kubernetes.io/hostname
```

---

### 3.8 `SyncLoadBalancerFailed` - LoadBalancer同步失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | service-controller |
| **关联资源** | Service |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 ⚠️ |

#### 事件含义

service-controller 无法同步 LoadBalancer 状态,这是一个通用错误,涵盖创建、更新、删除的所有失败场景。

**常见触发原因**:
- 云厂商 API 不可达 (网络问题)
- 云厂商认证失败 (凭证过期或错误)
- cloud-controller-manager 配置错误
- 云厂商服务暂时不可用

#### 典型事件消息

```bash
$ kubectl describe service my-lb-svc

Events:
  Type     Reason                   Age   From                Message
  ----     ------                   ----  ----                -------
  Warning  SyncLoadBalancerFailed   30s   service-controller  Error syncing load balancer: failed to ensure load balancer: context deadline exceeded
  Warning  SyncLoadBalancerFailed   10s   service-controller  Error syncing load balancer: failed to ensure load balancer: unable to resolve endpoint
```

#### 影响面说明

- **用户影响**: 中高 - LoadBalancer 状态无法同步,可能导致配置不一致
- **服务影响**: 
  - 如果是新创建: Service 无法获取 External IP
  - 如果是已存在: 配置变更无法生效
- **集群影响**: **可能扩散** - 如果是网络或认证问题,影响所有 LoadBalancer Service
- **关联事件链**: (多次重试) → `SyncLoadBalancerFailed` (循环,间隔 10 秒)

#### 排查建议

```bash
# 1. 查看完整错误消息
kubectl describe service my-lb-svc | grep -A 5 "SyncLoadBalancerFailed"

# 2. 检查 kube-controller-manager 日志
kubectl logs -n kube-system kube-controller-manager-<pod> --tail=100 | grep "service-controller"

# 3. 检查 cloud-controller-manager 日志 (如果使用外部 CCM)
kubectl logs -n kube-system cloud-controller-manager-<pod> --tail=100

# 4. 测试云厂商 API 可达性
# AWS:
aws ec2 describe-instances --max-items 1
# 如果超时或认证失败,说明 API 有问题

# 5. 检查 cloud-provider 配置
kubectl get cm -n kube-system cloud-config -o yaml

# 6. 检查是否有网络策略阻止 control plane 访问云 API
kubectl get networkpolicies -A
```

#### 解决建议

| 错误消息关键词 | 根本原因 | 排查方法 | 解决方案 |
|:---|:---|:---|:---|
| **context deadline exceeded** | 云 API 响应超时 | 测试云 API 可达性 | 1. 检查网络连接<br>2. 检查防火墙/安全组<br>3. 重启 cloud-controller-manager |
| **unable to resolve endpoint** | DNS 解析失败 | `nslookup <cloud-api-endpoint>` | 1. 检查 DNS 配置<br>2. 检查 /etc/resolv.conf |
| **AuthFailure** | 云认证失败 | 检查 IAM/ServicePrincipal 配置 | 1. 更新云凭证<br>2. 检查权限范围 |
| **connection refused** | cloud-controller-manager 未运行 | `kubectl get pods -n kube-system` | 启动 cloud-controller-manager |

**检查 AWS 认证配置 (kube-controller-manager)**:
```bash
# 查看 kube-controller-manager 使用的 IAM Role
kubectl get pods -n kube-system kube-controller-manager-<pod> -o yaml | grep -A 5 "AWS_"

# 测试 IAM Role 权限
aws sts get-caller-identity
aws elb describe-load-balancers --max-items 1
```

**检查 Azure 认证配置**:
```bash
# 查看 cloud-config Secret
kubectl get secret -n kube-system azure-cloud-provider -o yaml

# 验证 Service Principal
az login --service-principal \
  --username <appId> \
  --password <password> \
  --tenant <tenantId>
```

---

## 四、Endpoint 与 EndpointSlice 事件

### 4.1 `FailedToCreateEndpoint` / `FailedToCreateEndpointSlice` - Endpoint创建失败

| 属性 | FailedToCreateEndpoint | FailedToCreateEndpointSlice |
|:---|:---|:---|
| **事件类型** | Warning | Warning |
| **来源组件** | endpoint-controller | endpointslice-controller |
| **关联资源** | Service | Service |
| **适用版本** | v1.0+ | v1.17+ |
| **生产频率** | 低频 ⚠️ | 低频 ⚠️ |

#### 事件含义

**FailedToCreateEndpoint**: endpoint-controller 无法为 Service 创建 Endpoints 对象,导致 Service 无法发现后端 Pod。

**FailedToCreateEndpointSlice**: endpointslice-controller 无法创建 EndpointSlice 对象 (v1.17+)。

**常见失败原因**:
1. **API Server 不可达** - endpoint-controller 无法与 API Server 通信
2. **RBAC 权限不足** - endpoint-controller ServiceAccount 缺少权限
3. **etcd 存储问题** - etcd 空间不足或响应超时
4. **Endpoints 对象名称冲突** - 同名对象已存在且不受控制

#### 典型事件消息

```bash
$ kubectl describe service my-svc

Events:
  Type     Reason                   Age   From                 Message
  ----     ------                   ----  ----                 -------
  Warning  FailedToCreateEndpoint   30s   endpoint-controller  Failed to create endpoint for service default/my-svc: endpoints "my-svc" already exists
  Warning  FailedToCreateEndpoint   10s   endpoint-controller  Failed to create endpoint: etcdserver: request timed out
```

#### 影响面说明

- **用户影响**: **高** - Service 无法发现后端 Pod,流量无法转发
- **服务影响**: **严重** - Service 完全不可用,ClusterIP 无法访问
- **集群影响**: **可能扩散** - 如果是 API Server 或 etcd 问题,影响所有 Service
- **关联事件链**: (Service 创建) → `FailedToCreateEndpoint` (循环重试)

#### 排查建议

```bash
# 1. 检查 Endpoints 对象是否存在
kubectl get endpoints my-svc
kubectl describe endpoints my-svc

# 2. 检查 EndpointSlice 对象 (v1.17+)
kubectl get endpointslices -l kubernetes.io/service-name=my-svc
kubectl describe endpointslice <slice-name>

# 3. 检查 endpoint-controller 日志
kubectl logs -n kube-system kube-controller-manager-<pod> | grep endpoint-controller

# 4. 检查 RBAC 权限
kubectl auth can-i create endpoints --as=system:serviceaccount:kube-system:endpoint-controller -n default

# 5. 检查 API Server 健康状态
kubectl get --raw /healthz

# 6. 检查 etcd 健康状态
kubectl get --raw /healthz/etcd
```

#### 解决建议

| 错误消息关键词 | 根本原因 | 解决方案 |
|:---|:---|:---|
| **already exists** | Endpoints 对象名称冲突 | 1. 检查是否有同名 Endpoints 被手动创建<br>2. 删除冲突的 Endpoints: `kubectl delete endpoints my-svc`<br>3. 重建 Service |
| **etcdserver: request timed out** | etcd 响应超时 | 1. 检查 etcd 健康状态<br>2. 检查 etcd 磁盘性能<br>3. 检查 API Server 到 etcd 的网络 |
| **Unauthorized** | RBAC 权限不足 | 1. 检查 endpoint-controller ClusterRole<br>2. 确保有 `create` Endpoints 权限 |
| **connection refused** | API Server 不可达 | 1. 检查 API Server 状态<br>2. 检查网络连接 |

**检查 endpoint-controller 权限**:
```bash
# 查看 endpoint-controller 使用的 ClusterRole
kubectl get clusterrolebinding system:controller:endpoint-controller -o yaml

# 验证权限
kubectl auth can-i create endpoints \
  --as=system:serviceaccount:kube-system:endpoint-controller \
  -n default

# 如果权限不足,添加权限
kubectl create clusterrolebinding endpoint-controller-admin \
  --clusterrole=cluster-admin \
  --serviceaccount=kube-system:endpoint-controller
```

---

### 4.2 `FailedToUpdateEndpoint` / `FailedToUpdateEndpointSlice` - Endpoint更新失败

| 属性 | FailedToUpdateEndpoint | FailedToUpdateEndpointSlice |
|:---|:---|:---|
| **事件类型** | Warning | Warning |
| **来源组件** | endpoint-controller | endpointslice-controller |
| **关联资源** | Service | Service |
| **适用版本** | v1.0+ | v1.17+ |
| **生产频率** | 低频 ⚠️ | 低频 ⚠️ |

#### 事件含义

**FailedToUpdateEndpoint**: endpoint-controller 无法更新 Endpoints 对象,导致后端 Pod 列表与实际状态不一致。

**FailedToUpdateEndpointSlice**: endpointslice-controller 无法更新 EndpointSlice 对象。

**触发场景**:
- Pod 状态变化 (Ready ↔ NotReady) 但 Endpoints 未同步
- Pod 被删除但 Endpoints 仍包含该 Pod IP
- Pod 被创建但 Endpoints 未添加新 IP

**常见失败原因**:
1. **资源版本冲突** - Endpoints 被并发修改 (Conflict)
2. **对象过大** - Endpoints 包含过多 Pod (>1000) 导致 etcd 拒绝 (1.5MB 限制)
3. **API Server 限流** - 请求频率过高被限流
4. **网络问题** - endpoint-controller 与 API Server 通信失败

#### 典型事件消息

```bash
$ kubectl describe service my-svc

Events:
  Type     Reason                   Age   From                 Message
  ----     ------                   ----  ----                 -------
  Warning  FailedToUpdateEndpoint   30s   endpoint-controller  Failed to update endpoint default/my-svc: Operation cannot be fulfilled on endpoints "my-svc": the object has been modified; please apply your changes to the latest version and try again
  Warning  FailedToUpdateEndpoint   10s   endpoint-controller  Failed to update endpoint: etcdserver: request is too large
```

#### 影响面说明

- **用户影响**: **高** - Service 路由到已删除的 Pod 或错过新 Pod
- **服务影响**: 
  - 流量路由到 NotReady 或已删除的 Pod (连接失败)
  - 新 Pod 不接收流量 (容量不足)
- **集群影响**: 无
- **关联事件链**: (Pod 变化) → `FailedToUpdateEndpoint` (重试) → (最终成功或持续失败)

#### 排查建议

```bash
# 1. 检查 Endpoints 当前状态
kubectl get endpoints my-svc -o yaml

# 2. 对比 Endpoints 与实际 Pod 列表
kubectl get pods -l app=my-app -o wide
kubectl get endpoints my-svc -o jsonpath='{.subsets[*].addresses[*].ip}'

# 3. 检查 Endpoints 对象大小
kubectl get endpoints my-svc -o json | wc -c
# 如果超过 1MB,说明 Endpoints 过大

# 4. 检查 endpoint-controller 日志
kubectl logs -n kube-system kube-controller-manager-<pod> | grep "endpoint-controller\|my-svc"

# 5. 检查是否有并发修改
kubectl get endpoints my-svc -o jsonpath='{.metadata.resourceVersion}'
# 快速重复执行,如果版本号快速变化,说明有频繁更新
```

#### 解决建议

| 错误消息关键词 | 根本原因 | 解决方案 |
|:---|:---|:---|
| **the object has been modified** | 资源版本冲突 (并发更新) | 1. 正常现象,controller 会自动重试<br>2. 如果持续失败,检查是否有其他程序修改 Endpoints |
| **request is too large** | Endpoints 对象超过 etcd 限制 (>1.5MB) | **立即迁移到 EndpointSlice**:<br>1. 启用 EndpointSlice 特性 (v1.17+)<br>2. 删除旧 Endpoints 对象<br>3. 重建 Service |
| **rate limited** | API 请求被限流 | 1. 减少 Pod 变化频率<br>2. 增加 API Server QPS 限制 |
| **connection refused** | API Server 不可达 | 检查网络和 API Server 状态 |

**迁移到 EndpointSlice (解决 Endpoints 过大问题)**:
```bash
# 1. 确认集群版本支持 EndpointSlice (v1.17+)
kubectl version --short

# 2. 启用 EndpointSlice 特性 (v1.21+ 默认启用)
# 在 kube-controller-manager 启动参数中添加:
# --feature-gates=EndpointSlice=true

# 3. 检查 EndpointSlice 是否已生成
kubectl get endpointslices -l kubernetes.io/service-name=my-svc

# 4. 验证 EndpointSlice 内容
kubectl get endpointslice <slice-name> -o yaml

# 5. (可选) 手动删除旧 Endpoints 对象
# EndpointSlice 生成后,旧 Endpoints 仍会保留以兼容旧版本客户端
# 如果确认所有组件都支持 EndpointSlice,可以删除 Endpoints
kubectl delete endpoints my-svc
```

**检查 Endpoints 大小并拆分 Service**:
```bash
# 如果 Service 有数千个 Pod,考虑拆分为多个 Service

# 原 Service (1000+ Pod):
apiVersion: v1
kind: Service
metadata:
  name: my-large-svc
spec:
  selector:
    app: my-app
  ports:
    - port: 80

# 拆分为多个 Service (按命名空间/子应用):
---
apiVersion: v1
kind: Service
metadata:
  name: my-app-group-a
spec:
  selector:
    app: my-app
    group: a  # 添加额外的 selector
  ports:
    - port: 80
---
apiVersion: v1
kind: Service
metadata:
  name: my-app-group-b
spec:
  selector:
    app: my-app
    group: b
  ports:
    - port: 80
```

---

### 4.3 `FailedToDeleteEndpoint` / `FailedToDeleteEndpointSlice` - Endpoint删除失败

| 属性 | FailedToDeleteEndpoint | FailedToDeleteEndpointSlice |
|:---|:---|:---|
| **事件类型** | Warning | Warning |
| **来源组件** | endpoint-controller | endpointslice-controller |
| **关联资源** | Service | Service |
| **适用版本** | v1.0+ | v1.17+ |
| **生产频率** | 罕见 ⚠️ | 罕见 ⚠️ |

#### 事件含义

controller 无法删除 Endpoints 或 EndpointSlice 对象,通常发生在 Service 删除流程中。

**常见失败原因**:
- API Server 不可达
- RBAC 权限不足
- Endpoints 对象有 finalizers 阻止删除

#### 典型事件消息

```bash
$ kubectl describe service my-svc

Events:
  Type     Reason                   Age   From                 Message
  ----     ------                   ----  ----                 -------
  Warning  FailedToDeleteEndpoint   30s   endpoint-controller  Failed to delete endpoint default/my-svc: endpoints "my-svc" not found
```

#### 影响面说明

- **用户影响**: 低 - Service 删除流程受阻
- **服务影响**: 低 - Service 已不可用,但 Endpoints 对象残留
- **集群影响**: 低 - 占用少量 etcd 空间
- **关联事件链**: (Service 删除) → `FailedToDeleteEndpoint` (重试)

#### 排查建议

```bash
# 1. 检查 Endpoints 是否仍存在
kubectl get endpoints my-svc

# 2. 检查 Endpoints finalizers
kubectl get endpoints my-svc -o jsonpath='{.metadata.finalizers}'

# 3. 强制删除 Endpoints (如果需要)
kubectl patch endpoints my-svc -p '{"metadata":{"finalizers":[]}}' --type=merge
kubectl delete endpoints my-svc --force --grace-period=0
```

#### 解决建议

| 问题场景 | 解决方案 |
|:---|:---|
| Endpoints 已被手动删除 | 无需处理,controller 会停止重试 |
| Endpoints 有 finalizers | 移除 finalizers 后重新删除 |
| 权限不足 | 添加 `delete` Endpoints 权限 |

---

## 五、网络配置与资源分配事件

### 5.1 `HostPortConflict` - 主机端口冲突

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 ⚠️ |

#### 事件含义

Pod 配置的 `hostPort` 已被节点上的其他 Pod 或进程占用,导致 Pod 无法启动。

**hostPort 机制**:
- 将容器端口直接映射到宿主机端口 (类似 Docker `-p` 参数)
- **不经过 kube-proxy**,直接绑定节点 IP:Port
- 一个节点上只能有一个 Pod 使用同一个 hostPort
- 常用于 DaemonSet (如 ingress-controller, metrics-server)

**与 NodePort 的区别**:

| 特性 | hostPort | NodePort |
|:---|:---|:---|
| **配置位置** | Pod.spec.containers[].ports[].hostPort | Service.spec.ports[].nodePort |
| **端口范围** | 任意 (1-65535) | 30000-32767 (默认) |
| **调度限制** | 同一节点不能有多个 Pod 使用相同 hostPort | 无限制 (kube-proxy 负载均衡) |
| **流量路径** | 直接到 Pod | kube-proxy → Pod |

#### 典型事件消息

```bash
$ kubectl describe pod my-hostport-pod

Events:
  Type     Reason            Age   From     Message
  ----     ------            ----  ----     -------
  Warning  HostPortConflict  30s   kubelet  hostPort 8080 is already in use by another pod or process
  Warning  FailedSync        30s   kubelet  error determining status: rpc error: code = Unknown desc = failed to check port availability: bind: address already in use
```

#### 影响面说明

- **用户影响**: **高** - Pod 无法启动,卡在 Pending 状态
- **服务影响**: **严重** - 如果是 DaemonSet,该节点上的服务不可用
- **集群影响**: 无 - 仅影响该节点
- **关联事件链**: `Scheduled` → `HostPortConflict` → (调度器可能重新调度到其他节点)

#### 排查建议

```bash
# 1. 查看 Pod 的 hostPort 配置
kubectl get pod my-hostport-pod -o jsonpath='{.spec.containers[*].ports[*]}'

# 2. 查看节点上所有使用 hostPort 的 Pod
kubectl get pods -A -o json | \
  jq -r '.items[] | select(.spec.containers[].ports[]?.hostPort != null) | "\(.metadata.namespace)/\(.metadata.name) on \(.spec.nodeName): \(.spec.containers[].ports[].hostPort)"'

# 3. 登录节点检查端口占用
ssh <node>
sudo netstat -tulnp | grep :8080
# 或
sudo ss -tulnp | grep :8080

# 4. 检查是否有非 Kubernetes 进程占用端口
sudo lsof -i :8080
```

#### 解决建议

| 问题场景 | 根本原因 | 解决方案 |
|:---|:---|:---|
| **同一节点多个 Pod 使用相同 hostPort** | DaemonSet 或多副本调度 | 1. 使用 NodePort Service 代替 hostPort<br>2. 使用 PodAntiAffinity 避免调度到同一节点<br>3. 为 DaemonSet 使用不同的 hostPort |
| **非 Kubernetes 进程占用端口** | 节点上有其他服务监听该端口 | 1. 修改 Pod 的 hostPort<br>2. 停止冲突的进程<br>3. 使用节点选择器避开该节点 |
| **旧 Pod 未清理** | Pod 删除后端口未释放 | 1. 手动删除旧 Pod<br>2. 重启 kubelet: `systemctl restart kubelet` |

**避免 hostPort 冲突的 DaemonSet 配置**:
```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: ingress-controller
spec:
  selector:
    matchLabels:
      app: ingress
  template:
    metadata:
      labels:
        app: ingress
    spec:
      # 确保同一节点只有一个 Pod
      # (DaemonSet 默认行为,无需额外配置)
      containers:
        - name: nginx
          image: nginx:1.25
          ports:
            - name: http
              containerPort: 80
              hostPort: 80      # 绑定节点 80 端口
            - name: https
              containerPort: 443
              hostPort: 443     # 绑定节点 443 端口
          # 使用 hostNetwork 代替 hostPort (更推荐)
      # hostNetwork: true  # 容器直接使用节点网络命名空间
```

**推荐替代方案: 使用 NodePort Service**:
```yaml
# 不推荐: 使用 hostPort
apiVersion: v1
kind: Pod
spec:
  containers:
    - name: app
      ports:
        - containerPort: 8080
          hostPort: 8080  # ❌ 容易冲突

---
# 推荐: 使用 NodePort Service
apiVersion: v1
kind: Service
metadata:
  name: my-app
spec:
  type: NodePort
  selector:
    app: my-app
  ports:
    - port: 8080
      targetPort: 8080
      nodePort: 30080  # ✅ 所有节点可用,kube-proxy 负载均衡
```

---

### 5.2 `DNSConfigForming` - DNS配置生成失败

| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | kubelet |
| **关联资源** | Pod |
| **适用版本** | v1.9+ |
| **生产频率** | 低频 ⚠️ |

#### 事件含义

kubelet 无法为 Pod 生成正确的 DNS 配置 (/etc/resolv.conf),导致 Pod 内的 DNS 解析失败。

**DNS 配置来源**:
- ClusterFirst (默认): 使用集群 DNS (CoreDNS/kube-dns)
- Default: 继承节点的 DNS 配置
- None: 完全自定义 (需手动指定 Pod.spec.dnsConfig)

**DNS 配置流程**:
1. kubelet 根据 Pod.spec.dnsPolicy 决定配置策略
2. 查询 kube-dns/CoreDNS Service 的 ClusterIP
3. 生成 /etc/resolv.conf 内容
4. 挂载到容器的 /etc/resolv.conf

#### 典型事件消息

```bash
$ kubectl describe pod my-pod

Events:
  Type     Reason            Age   From     Message
  ----     ------            ----  ----     -------
  Warning  DNSConfigForming  30s   kubelet  Forming DNS configmap with mode ClusterFirst failed: unable to read config path "/etc/resolv.conf": open /etc/resolv.conf: no such file or directory
  Warning  DNSConfigForming  10s   kubelet  Forming DNS configmap failed: cannot find service "kube-dns" in namespace "kube-system"
```

#### 影响面说明

- **用户影响**: **高** - Pod 无法解析域名,网络功能受限
- **服务影响**: 
  - 无法访问其他 Service (如 `my-svc.default.svc.cluster.local`)
  - 无法访问外部域名 (如 `google.com`)
- **集群影响**: **可能扩散** - 如果 CoreDNS 故障,影响所有新 Pod
- **关联事件链**: `Started` → `DNSConfigForming` → (Pod 启动完成但 DNS 异常)

#### 排查建议

```bash
# 1. 检查 Pod 的 DNS 策略
kubectl get pod my-pod -o jsonpath='{.spec.dnsPolicy}'
# 输出: ClusterFirst (默认) / Default / None

# 2. 检查 CoreDNS/kube-dns Service
kubectl get svc -n kube-system kube-dns
kubectl get svc -n kube-system coredns

# 3. 检查 CoreDNS Pod 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl logs -n kube-system -l k8s-app=kube-dns

# 4. 测试 Pod 内 DNS 解析
kubectl exec my-pod -- nslookup kubernetes.default
kubectl exec my-pod -- cat /etc/resolv.conf

# 5. 检查节点的 /etc/resolv.conf
ssh <node>
cat /etc/resolv.conf

# 6. 验证 kubelet 配置
ssh <node>
ps aux | grep kubelet | grep "cluster-dns\|cluster-domain"
# 应包含: --cluster-dns=10.96.0.10 --cluster-domain=cluster.local
```

#### 解决建议

| 错误消息关键词 | 根本原因 | 解决方案 |
|:---|:---|:---|
| **no such file or directory** | 节点 /etc/resolv.conf 不存在 | 1. 在节点上创建 /etc/resolv.conf<br>2. 配置节点 DNS (如 `nameserver 8.8.8.8`) |
| **cannot find service "kube-dns"** | CoreDNS/kube-dns Service 不存在 | 1. 检查 kube-dns Service: `kubectl get svc -n kube-system kube-dns`<br>2. 重新部署 CoreDNS |
| **lookup kube-dns.kube-system: no such host** | CoreDNS Pod 未运行 | 1. 检查 CoreDNS Pod 状态<br>2. 查看 CoreDNS 日志排查启动失败原因 |

**修复 CoreDNS Service 缺失**:
```bash
# 检查 CoreDNS Service
kubectl get svc -n kube-system kube-dns
# 如果不存在,重新创建

# CoreDNS Service YAML:
apiVersion: v1
kind: Service
metadata:
  name: kube-dns
  namespace: kube-system
  labels:
    k8s-app: kube-dns
spec:
  selector:
    k8s-app: kube-dns
  clusterIP: 10.96.0.10  # 替换为你的 DNS ClusterIP (--cluster-dns 参数值)
  ports:
    - name: dns
      port: 53
      protocol: UDP
    - name: dns-tcp
      port: 53
      protocol: TCP
```

**自定义 Pod DNS 配置 (绕过 CoreDNS)**:
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: my-pod
spec:
  dnsPolicy: None  # 禁用自动 DNS 配置
  dnsConfig:
    nameservers:
      - 8.8.8.8    # Google DNS
      - 1.1.1.1    # Cloudflare DNS
    searches:
      - default.svc.cluster.local
      - svc.cluster.local
      - cluster.local
    options:
      - name: ndots
        value: "5"
  containers:
    - name: app
      image: nginx:1.25
```

---

### 5.3 `IPAllocated` / `IPNotAllocated` - IP地址分配

| 属性 | IPAllocated | IPNotAllocated |
|:---|:---|:---|
| **事件类型** | Normal | Warning |
| **来源组件** | kubelet / cloud-controller-manager | kubelet / cloud-controller-manager |
| **关联资源** | Pod / Service | Pod / Service |
| **适用版本** | v1.24+ | v1.24+ |
| **生产频率** | 低频 | 低频 ⚠️ |

#### 事件含义

**IPAllocated**: IP 地址成功分配给 Pod 或 Service。

**IPNotAllocated**: IP 地址分配失败,可能是 IP 地址池耗尽或网络插件故障。

**IP 分配场景**:
- **Pod IP 分配**: CNI 插件从 PodCIDR 分配 IP 给 Pod
- **Service ClusterIP 分配**: kube-apiserver 从 Service CIDR 分配 ClusterIP
- **LoadBalancer External IP 分配**: 云厂商分配公网 IP

#### 典型事件消息

```bash
# Pod IP 分配成功
$ kubectl describe pod my-pod
Events:
  Type    Reason       Age   From     Message
  ----    ------       ----  ----     -------
  Normal  IPAllocated  10s   kubelet  Successfully allocated IP 10.244.1.10 to pod my-pod

# Service External IP 分配失败
$ kubectl describe service my-lb-svc
Events:
  Type     Reason          Age   From                Message
  ----     ------          ----  ----                -------
  Warning  IPNotAllocated  30s   service-controller  Failed to allocate external IP: IP pool exhausted
```

#### 影响面说明

- **用户影响**: 
  - `IPAllocated`: 无 - 正常操作
  - `IPNotAllocated`: **高** - Pod/Service 无法获取 IP,不可用
- **服务影响**: 
  - Pod IP 分配失败: Pod 无法启动
  - Service ClusterIP 分配失败: Service 无法创建
  - External IP 分配失败: LoadBalancer 无法对外提供服务
- **集群影响**: **可能扩散** - 如果 IP 池耗尽,影响所有新 Pod/Service
- **关联事件链**: 
  - Pod: `Scheduled` → `IPNotAllocated` → `FailedCreatePodSandBox`
  - Service: `EnsuringLoadBalancer` → `IPNotAllocated` → `SyncLoadBalancerFailed`

#### 排查建议

```bash
# 1. 检查 Pod CIDR 配置
kubectl get nodes -o jsonpath='{.items[*].spec.podCIDR}'

# 2. 检查节点 IP 分配情况
kubectl get pods -A -o wide | grep <node>

# 3. 检查 Service CIDR 使用情况
kubectl get svc -A -o jsonpath='{.items[*].spec.clusterIP}' | tr ' ' '\n' | sort -u | wc -l
# 对比 Service CIDR 总数 (默认 /12 = 1,048,576 个 IP)

# 4. 检查 CNI 插件状态
kubectl get pods -n kube-system -l app=calico-node
kubectl logs -n kube-system -l app=calico-node | grep "IPAM"

# 5. 检查云厂商 IP 配额
# AWS: 查看 Elastic IP 配额
aws ec2 describe-account-attributes --attribute-names vpc-max-elastic-ips

# 6. 查看 kube-controller-manager 的 Service CIDR 配置
kubectl get pods -n kube-system kube-controller-manager-<node> -o yaml | grep service-cluster-ip-range
```

#### 解决建议

| 分配失败场景 | 根本原因 | 解决方案 |
|:---|:---|:---|
| **Pod IP 池耗尽** | 节点 PodCIDR 过小 (如 /24 仅 254 个 IP) | 1. 扩大 PodCIDR (需重建集群或节点)<br>2. 添加更多节点分散 Pod |
| **Service ClusterIP 池耗尽** | Service CIDR 过小 | 1. 扩大 Service CIDR (需修改 kube-apiserver `--service-cluster-ip-range`)<br>2. 清理不使用的 Service |
| **CNI 插件故障** | Calico/Flannel IPAM 异常 | 1. 重启 CNI 插件 DaemonSet<br>2. 检查 CNI 配置文件 /etc/cni/net.d/ |
| **云厂商 IP 配额超限** | Elastic IP 配额不足 | 1. 申请提升配额<br>2. 释放不使用的 EIP |
| **IP 冲突** | 手动分配的 IP 与自动分配冲突 | 1. 检查 Pod/Service 是否有 `spec.podIP` 或 `spec.clusterIP` 手动指定<br>2. 避免使用静态 IP |

**扩大 Pod CIDR (Calico 示例)**:
```bash
# 查看当前 IP Pool
kubectl get ippool -o yaml

# 添加新的 IP Pool
cat <<EOF | kubectl apply -f -
apiVersion: crd.projectcalico.org/v1
kind: IPPool
metadata:
  name: new-ipv4-pool
spec:
  cidr: 10.245.0.0/16  # 新的 CIDR 范围
  ipipMode: Never
  natOutgoing: true
  disabled: false
  nodeSelector: all()
EOF

# 验证新 Pod 使用新 IP Pool
kubectl run test --image=nginx --restart=Never
kubectl get pod test -o wide
```

**扩大 Service CIDR**:
```bash
# ⚠️ 需要重启 kube-apiserver 和 kube-controller-manager

# 1. 修改 kube-apiserver 启动参数
# /etc/kubernetes/manifests/kube-apiserver.yaml (kubeadm)
--service-cluster-ip-range=10.96.0.0/12,10.112.0.0/12  # 添加第二个 CIDR

# 2. 修改 kube-controller-manager 启动参数
# /etc/kubernetes/manifests/kube-controller-manager.yaml
--service-cluster-ip-range=10.96.0.0/12,10.112.0.0/12

# 3. 等待 control plane 组件重启
kubectl wait --for=condition=Ready pod/kube-apiserver-<node> -n kube-system --timeout=5m

# 4. 验证新 Service 使用新 CIDR
kubectl create svc clusterip test --tcp=80:80
kubectl get svc test -o jsonpath='{.spec.clusterIP}'
```

---

## 六、综合排查案例

### 案例 1: LoadBalancer External IP 长时间 Pending

**问题现象**:
```bash
$ kubectl get svc my-lb-svc
NAME         TYPE           CLUSTER-IP      EXTERNAL-IP   PORT(S)        AGE
my-lb-svc    LoadBalancer   10.96.100.50    <pending>     80:30080/TCP   10m
```

**排查步骤**:
```bash
# 1. 查看 Service 事件
kubectl describe svc my-lb-svc
# 输出: Warning SyncLoadBalancerFailed ... Error syncing load balancer: failed to ensure load balancer: RequestLimitExceeded

# 2. 检查 cloud-controller-manager 日志
kubectl logs -n kube-system cloud-controller-manager-<pod> --tail=50
# 输出: E0210 10:30:00.123456 AWS API rate limit exceeded

# 3. 查看云账号 API 调用量
aws cloudwatch get-metric-statistics \
  --namespace AWS/Usage \
  --metric-name CallCount \
  --dimensions Name=Service,Value=ELB \
  --start-time 2026-02-10T09:00:00Z \
  --end-time 2026-02-10T11:00:00Z \
  --period 300 \
  --statistics Sum
```

**根本原因**: AWS API 请求被限流,通常是因为短时间内创建/更新了大量 LoadBalancer Service。

**解决方案**:
```bash
# 方案 1: 增加 API 重试间隔 (修改 cloud-controller-manager 配置)
kubectl edit deployment -n kube-system cloud-controller-manager
# 添加环境变量:
env:
  - name: AWS_RETRY_MAX_ATTEMPTS
    value: "10"

# 方案 2: 分批创建 Service,避免并发创建
# 使用脚本控制创建速度

# 方案 3: 联系 AWS 提升 API 限流配额
```

---

### 案例 2: Service 无流量但 Endpoints 正常

**问题现象**:
```bash
$ kubectl get svc my-svc
NAME     TYPE        CLUSTER-IP      EXTERNAL-IP   PORT(S)   AGE
my-svc   ClusterIP   10.96.100.100   <none>        80/TCP    5m

$ kubectl get endpoints my-svc
NAME     ENDPOINTS                         AGE
my-svc   10.244.1.10:8080,10.244.2.20:8080 5m

$ curl 10.96.100.100
# 超时无响应
```

**排查步骤**:
```bash
# 1. 测试直接访问 Pod IP
curl 10.244.1.10:8080
# 如果成功,说明 Pod 正常,问题在网络层

# 2. 检查 kube-proxy 模式
kubectl get configmap -n kube-system kube-proxy -o yaml | grep mode
# 输出: mode: "iptables"

# 3. 检查 kube-proxy 日志
kubectl logs -n kube-system kube-proxy-<pod> --tail=50
# 输出: E0210 Failed to sync iptables rules: error executing iptables-save

# 4. 登录节点检查 iptables 规则
ssh <node>
sudo iptables-save | grep my-svc
# 如果没有输出,说明 kube-proxy 未创建 iptables 规则

# 5. 检查 kube-proxy Pod 状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy
```

**根本原因**: kube-proxy 故障,未创建 Service 的 iptables/ipvs 规则。

**解决方案**:
```bash
# 重启 kube-proxy DaemonSet
kubectl rollout restart daemonset/kube-proxy -n kube-system

# 验证规则已创建
ssh <node>
sudo iptables-save | grep -A 5 "my-svc"
# 应包含 DNAT 规则: -A KUBE-SERVICES -d 10.96.100.100/32 -p tcp -m tcp --dport 80 -j KUBE-SVC-...

# 再次测试 ClusterIP
curl 10.96.100.100
```

---

### 案例 3: EndpointSlice 未更新导致流量路由错误

**问题现象**:
```bash
# 删除一个 Pod 后,流量仍路由到已删除的 Pod IP,导致部分请求失败

$ kubectl get pods -l app=my-app -o wide
NAME       READY   STATUS    RESTARTS   AGE   IP
my-app-1   1/1     Running   0          5m    10.244.1.10
my-app-2   1/1     Running   0          5m    10.244.2.20

$ kubectl delete pod my-app-2

$ kubectl get endpointslices -l kubernetes.io/service-name=my-svc -o yaml
# 输出: endpoints 仍包含 10.244.2.20 (已删除的 Pod IP)
```

**排查步骤**:
```bash
# 1. 检查 endpointslice-controller 日志
kubectl logs -n kube-system kube-controller-manager-<pod> | grep endpointslice-controller
# 输出: E0210 Failed to update EndpointSlice: conflict

# 2. 检查 EndpointSlice 的 resourceVersion
kubectl get endpointslices <slice-name> -o jsonpath='{.metadata.resourceVersion}'

# 3. 检查是否有其他控制器修改 EndpointSlice
kubectl get events --field-selector involvedObject.kind=EndpointSlice,involvedObject.name=<slice-name>
```

**根本原因**: endpointslice-controller 遇到资源版本冲突,更新失败后未及时重试。

**解决方案**:
```bash
# 方案 1: 手动触发 EndpointSlice 更新 (添加 annotation)
kubectl annotate endpointslice <slice-name> force-sync="$(date +%s)"

# 方案 2: 删除 EndpointSlice (controller 会自动重建)
kubectl delete endpointslice <slice-name>

# 方案 3: 重启 kube-controller-manager
kubectl delete pod -n kube-system kube-controller-manager-<node>

# 验证 EndpointSlice 已更新
kubectl get endpointslices <slice-name> -o yaml | grep -A 5 endpoints:
# 应不再包含 10.244.2.20
```

---

## 七、生产环境最佳实践

### 7.1 Service 类型选择建议

| 使用场景 | 推荐类型 | 理由 |
|:---|:---|:---|
| **集群内部通信** | ClusterIP (默认) | 无需外部暴露,性能最优 |
| **开发/测试环境外部访问** | NodePort | 简单快速,无需 LoadBalancer 费用 |
| **生产环境外部访问** | LoadBalancer + Ingress | 生产级负载均衡,支持 SSL/TLS 终止 |
| **外部服务集成** | ExternalName | 映射外部 DNS,无需修改应用代码 |

### 7.2 LoadBalancer Service 最佳实践

```yaml
apiVersion: v1
kind: Service
metadata:
  name: my-production-lb
  annotations:
    # AWS ELB 注解
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"  # 使用 NLB (网络负载均衡器)
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-interval: "10"
    service.beta.kubernetes.io/aws-load-balancer-healthcheck-timeout: "5"
    
    # Azure Load Balancer 注解
    service.beta.kubernetes.io/azure-load-balancer-internal: "false"  # 公网 LB
    service.beta.kubernetes.io/azure-load-balancer-health-probe-interval: "10"
    
    # GCP Load Balancer 注解
    cloud.google.com/load-balancer-type: "External"
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local  # 保留源 IP,减少跨节点跳转
  sessionAffinity: ClientIP     # 会话保持 (可选)
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800     # 会话超时 3 小时
  selector:
    app: my-app
  ports:
    - name: http
      port: 80
      targetPort: 8080
      protocol: TCP
    - name: https
      port: 443
      targetPort: 8443
      protocol: TCP
```

**externalTrafficPolicy 对比**:

| 策略 | 优点 | 缺点 | 适用场景 |
|:---|:---|:---|:---|
| **Cluster** (默认) | 流量负载均衡更均匀 | 丢失源 IP (SNAT),增加跨节点跳转 | 对源 IP 无要求的应用 |
| **Local** | 保留源 IP,减少延迟 | 流量不均衡 (仅路由到本地 Pod) | 需要源 IP 的应用 (如日志审计) |

### 7.3 Endpoints vs EndpointSlice 迁移策略

**何时使用 EndpointSlice**:
- ✅ Kubernetes v1.21+ (GA)
- ✅ Service 有 >100 个 Pod
- ✅ 集群规模 >1000 Pods
- ✅ 频繁的 Pod 变化 (如 HPA 扩缩容)

**迁移检查清单**:
```bash
# 1. 确认集群版本
kubectl version --short | grep Server
# 需要 v1.17+ (建议 v1.21+)

# 2. 检查 EndpointSlice 是否已启用
kubectl get endpointslices --all-namespaces
# 如果有输出,说明已启用

# 3. 验证 kube-proxy 支持 EndpointSlice
kubectl logs -n kube-system kube-proxy-<pod> | grep EndpointSlice
# 应包含: Using EndpointSlice informers

# 4. (可选) 禁用 Endpoints 同步 (减少 etcd 压力)
# 修改 kube-controller-manager:
--feature-gates=DisableEndpointsWatcher=true
```

### 7.4 Service 监控告警配置

**Prometheus 告警规则**:
```yaml
groups:
  - name: kubernetes-service
    rules:
      # LoadBalancer 长时间 Pending
      - alert: LoadBalancerStuckPending
        expr: |
          kube_service_info{type="LoadBalancer"} 
          and on (service, namespace) 
          kube_service_status_load_balancer_ingress == 0
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "LoadBalancer Service {{ $labels.service }} 长时间 Pending"
          description: "Service {{ $labels.namespace }}/{{ $labels.service }} 10分钟内未获取 External IP"

      # Service 无可用 Endpoints
      - alert: ServiceHasNoEndpoints
        expr: |
          kube_service_spec_type{type!="ExternalName"}
          and on (service, namespace)
          (kube_endpoint_address_available == 0 or absent(kube_endpoint_address_available))
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "Service {{ $labels.service }} 无可用后端"
          description: "Service {{ $labels.namespace }}/{{ $labels.service }} 5分钟内无可用 Endpoints"

      # Endpoints 更新失败频繁
      - alert: HighEndpointUpdateFailureRate
        expr: |
          rate(apiserver_request_total{verb="update",resource="endpoints",code=~"5.."}[5m]) > 0.1
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Endpoints 更新失败率过高"
          description: "过去10分钟 Endpoints 更新失败 {{ $value }} 次/秒"
```

### 7.5 云厂商特定配置参考

**AWS ELB 最佳实践**:
```yaml
annotations:
  # 使用 Network Load Balancer (高性能)
  service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
  
  # 跨可用区负载均衡
  service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
  
  # 健康检查配置
  service.beta.kubernetes.io/aws-load-balancer-healthcheck-protocol: "http"
  service.beta.kubernetes.io/aws-load-balancer-healthcheck-path: "/healthz"
  service.beta.kubernetes.io/aws-load-balancer-healthcheck-interval: "10"
  service.beta.kubernetes.io/aws-load-balancer-healthcheck-timeout: "5"
  service.beta.kubernetes.io/aws-load-balancer-healthcheck-healthy-threshold: "2"
  service.beta.kubernetes.io/aws-load-balancer-healthcheck-unhealthy-threshold: "2"
  
  # 内部 LoadBalancer (仅 VPC 内访问)
  service.beta.kubernetes.io/aws-load-balancer-internal: "true"
  
  # 指定子网
  service.beta.kubernetes.io/aws-load-balancer-subnets: "subnet-abc123,subnet-def456"
  
  # 访问日志
  service.beta.kubernetes.io/aws-load-balancer-access-log-enabled: "true"
  service.beta.kubernetes.io/aws-load-balancer-access-log-s3-bucket-name: "my-lb-logs"
```

**Azure Load Balancer 最佳实践**:
```yaml
annotations:
  # 标准 SKU (生产推荐)
  service.beta.kubernetes.io/azure-load-balancer-sku: "standard"
  
  # 健康检查配置
  service.beta.kubernetes.io/azure-load-balancer-health-probe-interval: "10"
  service.beta.kubernetes.io/azure-load-balancer-health-probe-num-of-probe: "2"
  service.beta.kubernetes.io/azure-load-balancer-health-probe-request-path: "/healthz"
  
  # 指定公网 IP
  service.beta.kubernetes.io/azure-load-balancer-resource-group: "my-rg"
  service.beta.kubernetes.io/azure-pip-name: "my-public-ip"
```

**GCP Load Balancer 最佳实践**:
```yaml
annotations:
  # 使用 Network Load Balancer
  cloud.google.com/load-balancer-type: "External"
  
  # 会话亲和性
  cloud.google.com/load-balancer-session-affinity: "CLIENT_IP"
  
  # 健康检查
  cloud.google.com/load-balancer-health-check-path: "/healthz"
  cloud.google.com/load-balancer-health-check-interval: "10"
```

### 7.6 Service 排查工具箱

**快速诊断脚本**:
```bash
#!/bin/bash
# service-diagnostics.sh - Service 快速诊断脚本

SERVICE_NAME=$1
NAMESPACE=${2:-default}

if [ -z "$SERVICE_NAME" ]; then
  echo "Usage: $0 <service-name> [namespace]"
  exit 1
fi

echo "=== Service 基本信息 ==="
kubectl get svc -n $NAMESPACE $SERVICE_NAME -o wide

echo ""
echo "=== Service 详细配置 ==="
kubectl get svc -n $NAMESPACE $SERVICE_NAME -o yaml | grep -A 20 "spec:"

echo ""
echo "=== Endpoints 状态 ==="
kubectl get endpoints -n $NAMESPACE $SERVICE_NAME

echo ""
echo "=== EndpointSlices 状态 ==="
kubectl get endpointslices -n $NAMESPACE -l kubernetes.io/service-name=$SERVICE_NAME

echo ""
echo "=== 匹配的 Pods ==="
SELECTOR=$(kubectl get svc -n $NAMESPACE $SERVICE_NAME -o jsonpath='{.spec.selector}' | jq -r 'to_entries | map("\(.key)=\(.value)") | join(",")')
kubectl get pods -n $NAMESPACE -l "$SELECTOR" -o wide

echo ""
echo "=== Service 事件 ==="
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$SERVICE_NAME --sort-by='.lastTimestamp'

echo ""
echo "=== LoadBalancer 详情 (如适用) ==="
kubectl get svc -n $NAMESPACE $SERVICE_NAME -o jsonpath='{.status.loadBalancer}' | jq .

echo ""
echo "=== 网络连通性测试 ==="
CLUSTER_IP=$(kubectl get svc -n $NAMESPACE $SERVICE_NAME -o jsonpath='{.spec.clusterIP}')
PORT=$(kubectl get svc -n $NAMESPACE $SERVICE_NAME -o jsonpath='{.spec.ports[0].port}')
echo "测试 ClusterIP: $CLUSTER_IP:$PORT"
kubectl run test-curl --image=curlimages/curl --rm -i --restart=Never -- curl -m 5 http://$CLUSTER_IP:$PORT || echo "ClusterIP 不可达"

echo ""
echo "=== kube-proxy 状态检查 ==="
kubectl get pods -n kube-system -l k8s-app=kube-proxy -o wide
```

**使用示例**:
```bash
chmod +x service-diagnostics.sh
./service-diagnostics.sh my-loadbalancer-svc default
```

---

## 相关文档交叉引用

- **[02-pod-container-lifecycle-events.md](./02-pod-container-lifecycle-events.md)** - Pod 生命周期事件,了解 Endpoints 后端 Pod 的启动/终止事件
- **[05-scheduling-preemption-events.md](./05-scheduling-preemption-events.md)** - 调度事件,理解 Pod 如何分配到节点影响 Service 后端
- **[06-node-lifecycle-condition-events.md](./06-node-lifecycle-condition-events.md)** - 节点事件,NodeNotReady 影响 LoadBalancer 后端健康检查
- **[15-ecosystem-addon-events.md](./15-ecosystem-addon-events.md)** - Ingress Controller 和 CoreDNS 事件
- **[Domain-5: 网络 - Service 深度解析](../domain-5-networking/20-service-deep-dive.md)** - Service 工作原理和架构
- **[Domain-5: 网络 - Ingress 与 Gateway API](../domain-5-networking/35-gateway-api-overview.md)** - 七层负载均衡方案

---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 10/15
