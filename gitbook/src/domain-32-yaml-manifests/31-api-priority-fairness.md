# 31 - FlowSchema / PriorityLevelConfiguration YAML 配置参考

> **适用版本**: Kubernetes v1.29 GA (Beta 从 v1.20, Alpha 从 v1.18) | **最后更新**: 2026-02

**本文档全面覆盖 API Priority and Fairness (APF) 的 YAML 配置**,包括 FlowSchema 请求分类、PriorityLevelConfiguration 优先级配置、内置流量控制策略、Shuffle Sharding 机制、生产实践案例等。

---

## 📋 目录

1. [APF 基础概念](#1-apf-基础概念)
2. [FlowSchema 完整字段](#2-flowschema-完整字段)
3. [PriorityLevelConfiguration 完整字段](#3-prioritylevelconfiguration-完整字段)
4. [内置 FlowSchema 列表](#4-内置-flowschema-列表)
5. [内部原理](#5-内部原理)
6. [生产案例](#6-生产案例)
7. [故障排查](#7-故障排查)

---

## 1. APF 基础概念

### 1.1 什么是 API Priority and Fairness

API Priority and Fairness (APF) 是 Kubernetes v1.29 GA 的**流量控制机制**,用于保护 API Server 免受过载:

- **细粒度流量分类**: 根据用户、资源类型、动词等将请求分类到不同的 FlowSchema
- **优先级隔离**: 不同优先级的请求使用独立的队列,高优先级不受低优先级影响
- **公平排队**: 同优先级内使用 Fair Queuing 算法,防止单个客户端占用所有资源
- **动态限流**: 根据 API Server 负载动态调整并发数,避免硬编码限制
- **替代 Max-inflight-requests**: 取代旧的 `--max-requests-inflight` 和 `--max-mutating-requests-inflight` 参数

### 1.2 核心概念

```
客户端请求
    ▼
┌─────────────────────────────────────────────────────────────────┐
│ 1. FlowSchema 匹配(按 matchingPrecedence 排序)                  │
│    - 匹配条件: User, ServiceAccount, Namespace, Resource, Verb  │
│    - 结果: 确定请求所属的 PriorityLevelConfiguration            │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 2. PriorityLevelConfiguration 限流                              │
│    - 类型: Limited(排队) 或 Exempt(豁免)                        │
│    - 并发控制: nominalConcurrencyShares(并发配额)               │
│    - 排队策略: Queue(入队等待) 或 Reject(直接拒绝)              │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 3. Fair Queuing + Shuffle Sharding                              │
│    - 每个 Flow(用户+命名空间)分配独立队列                       │
│    - Shuffle Sharding 隔离故障流量                              │
│    - 超时拒绝或成功执行                                         │
└─────────────────────────────────────────────────────────────────┘
```

### 1.3 与旧机制对比

| 特性 | APF (v1.29+) | Max Inflight Requests (已弃用) |
|------|--------------|-------------------------------|
| **请求分类** | 细粒度(User, Resource, Verb) | 粗粒度(仅 Mutating/Non-Mutating) |
| **优先级隔离** | 支持多级优先级 | 不支持 |
| **公平性** | Fair Queuing 算法 | 无公平性保证 |
| **排队机制** | 支持排队等待 | 直接拒绝(429 Too Many Requests) |
| **动态配置** | 支持(修改 YAML 即生效) | 不支持(需重启 API Server) |
| **故障隔离** | Shuffle Sharding | 无隔离机制 |

---

## 2. FlowSchema 完整字段

### 2.1 基础结构

```yaml
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: example-flowschema
spec:
  # === 优先级配置 ===
  
  # 关联的 PriorityLevelConfiguration
  priorityLevelConfiguration:
    name: example-priority-level
    type: ""  # 始终为空字符串
  
  # 匹配优先级(数值越小优先级越高,范围 1-10000)
  matchingPrecedence: 1000
  
  # === Flow 区分器(用于 Fair Queuing) ===
  
  # 区分方法: ByUser(按用户), ByNamespace(按命名空间), 或不设置(全局共享)
  distinguisherMethod:
    type: ByUser  # ByUser, ByNamespace
  
  # === 匹配规则(至少匹配一条规则即生效) ===
  
  rules:
    # --- 规则 1: 匹配特定 ServiceAccount ---
    - subjects:
        # ServiceAccount 主体
        - kind: ServiceAccount
          serviceAccount:
            name: important-controller
            namespace: kube-system
      # 资源规则(可选,不指定则匹配所有资源)
      resourceRules:
        # 匹配 API 组
        - apiGroups: ["*"]
          # 匹配资源类型
          resources: ["*"]
          # 匹配命名空间(可选)
          namespaces: ["*"]
          # 集群作用域资源标记
          clusterScope: true
          # 匹配动词(get, list, create, update, patch, delete, watch)
          verbs: ["*"]
      # 非资源规则(可选,用于 /healthz, /metrics 等)
      nonResourceRules: []
    
    # --- 规则 2: 匹配用户组 ---
    - subjects:
        # 用户组主体
        - kind: Group
          group:
            name: system:authenticated  # 所有认证用户
      resourceRules:
        - apiGroups: [""]  # 核心 API 组
          resources: ["pods", "services"]
          namespaces: ["production"]
          verbs: ["get", "list", "watch"]
    
    # --- 规则 3: 匹配具体用户 ---
    - subjects:
        # 用户主体
        - kind: User
          user:
            name: admin@example.com
      # 非资源请求(如 /healthz)
      nonResourceRules:
        - nonResourceURLs: ["/healthz", "/livez", "/readyz"]
          verbs: ["get"]
```

### 2.2 Subjects 类型详解

```yaml
subjects:
  # 1. ServiceAccount(服务账户)
  - kind: ServiceAccount
    serviceAccount:
      name: my-controller      # SA 名称
      namespace: kube-system   # SA 命名空间
  
  # 2. User(用户)
  - kind: User
    user:
      name: "system:kube-controller-manager"  # 用户名(来自证书 CN 或 Token)
  
  # 3. Group(用户组)
  - kind: Group
    group:
      name: "system:masters"   # 组名(来自证书 O 或 Token)
  
  # 常见的内置组:
  # - system:authenticated        所有认证用户
  # - system:unauthenticated      所有未认证用户
  # - system:masters              集群管理员
  # - system:nodes                所有节点(Kubelet)
  # - system:serviceaccounts      所有 ServiceAccount
  # - system:serviceaccounts:<ns> 特定命名空间的所有 ServiceAccount
```

### 2.3 ResourceRules 匹配示例

```yaml
resourceRules:
  # 示例 1: 匹配所有资源
  - apiGroups: ["*"]
    resources: ["*"]
    namespaces: ["*"]
    verbs: ["*"]
  
  # 示例 2: 匹配核心 API 的 Pod 读取
  - apiGroups: [""]  # 核心 API 组用空字符串表示
    resources: ["pods", "pods/log", "pods/status"]
    namespaces: ["default", "kube-system"]
    verbs: ["get", "list", "watch"]
  
  # 示例 3: 匹配自定义资源
  - apiGroups: ["apps.example.com"]
    resources: ["databases", "databases/status"]
    namespaces: ["*"]
    verbs: ["get", "list", "watch", "create", "update", "patch"]
  
  # 示例 4: 集群作用域资源(Node, PV, ClusterRole 等)
  - apiGroups: [""]
    resources: ["nodes", "persistentvolumes"]
    clusterScope: true  # 仅匹配集群级资源
    verbs: ["get", "list"]
  
  # 示例 5: 排除特定命名空间(APF 不支持直接排除,需通过多个 FlowSchema 实现)
  - apiGroups: [""]
    resources: ["secrets"]
    namespaces: ["production"]  # 仅匹配 production 命名空间
    verbs: ["get", "list"]
```

### 2.4 NonResourceRules 示例

```yaml
nonResourceRules:
  # 示例 1: 健康检查端点
  - nonResourceURLs: ["/healthz", "/livez", "/readyz"]
    verbs: ["get"]
  
  # 示例 2: Metrics 端点
  - nonResourceURLs: ["/metrics"]
    verbs: ["get"]
  
  # 示例 3: API 发现端点
  - nonResourceURLs: ["/api", "/api/*", "/apis", "/apis/*"]
    verbs: ["get"]
  
  # 示例 4: OpenAPI 规范
  - nonResourceURLs: ["/openapi/v2", "/openapi/v3"]
    verbs: ["get"]
  
  # 示例 5: 版本信息
  - nonResourceURLs: ["/version"]
    verbs: ["get"]
```

---

## 3. PriorityLevelConfiguration 完整字段

### 3.1 基础结构

```yaml
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: example-priority-level
spec:
  # === 类型: Limited(限流) 或 Exempt(豁免) ===
  
  type: Limited  # Limited 或 Exempt
  
  # === Limited 类型配置(仅当 type=Limited 时有效) ===
  
  limited:
    # --- 并发配置 ---
    
    # 并发份额(Nominal Concurrency Shares,相对值)
    # 实际并发数 = (nominalConcurrencyShares / 所有 PLC 总份额) * API Server 总并发数
    nominalConcurrencyShares: 30
    
    # 可借出的并发百分比(0-100,默认 50)
    # 当其他 PLC 过载时,可借出未使用的并发配额
    lendablePercent: 50
    
    # 可借入的并发百分比上限(0-100,可选)
    # 限制从其他 PLC 借入的并发数上限
    borrowingLimitPercent: 100
    
    # --- 超载响应策略 ---
    
    limitResponse:
      # 类型: Queue(排队) 或 Reject(直接拒绝)
      type: Queue
      
      # Queue 类型配置(仅当 type=Queue 时有效)
      queuing:
        # 队列数量(用于 Shuffle Sharding,范围 1-512,推荐 64)
        queues: 64
        
        # 队列长度(每个队列最大等待请求数,范围 1-10000)
        queueLengthLimit: 50
        
        # Hand Size(Shuffle Sharding 参数,范围 1-queues,推荐 8)
        # 每个 Flow 随机分配到 handSize 个队列中的一个
        handSize: 8
```

### 3.2 Exempt 类型(豁免限流)

```yaml
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: exempt-priority
spec:
  # Exempt 类型: 完全豁免限流(用于关键系统组件)
  type: Exempt
  
  # exempt 字段为空对象(v1.29+ 必需字段)
  exempt: {}
```

**注意**: Exempt 类型仅用于极少数关键组件(如 `system:masters` 组),滥用会导致 API Server 过载!

---

## 4. 内置 FlowSchema 列表

Kubernetes 默认提供以下内置 FlowSchema(v1.29):

| FlowSchema 名称 | PriorityLevel | 描述 | matchingPrecedence |
|----------------|---------------|------|-------------------|
| **system-leader-election** | leader-election | Leader Election 请求 | 100 |
| **endpoint-controller** | workload-high | EndpointSlice 控制器 | 150 |
| **workload-leader-election** | leader-election | 工作负载 Leader Election | 200 |
| **system-node-high** | node-high | Kubelet 高优先级请求 | 400 |
| **system-nodes** | system | Kubelet 常规请求 | 500 |
| **kube-controller-manager** | workload-high | KCM 请求 | 800 |
| **kube-scheduler** | workload-high | Scheduler 请求 | 800 |
| **kube-system-service-accounts** | workload-high | kube-system SA | 900 |
| **service-accounts** | workload-low | 其他 SA | 9000 |
| **global-default** | global-default | 默认 FlowSchema(最低优先级) | 9900 |
| **catch-all** | catch-all | 兜底 FlowSchema | 10000 |

### 4.1 内置 PriorityLevelConfiguration

| PriorityLevel 名称 | 并发份额 | 队列数 | 描述 |
|-------------------|---------|-------|------|
| **exempt** | - | - | 豁免限流(system:masters) |
| **node-high** | 40 | 64 | Kubelet 高优先级 |
| **system** | 30 | 64 | 系统组件(Kubelet, KCM) |
| **leader-election** | 10 | 16 | Leader Election |
| **workload-high** | 40 | 128 | 工作负载控制器高优先级 |
| **workload-low** | 100 | 128 | 工作负载控制器低优先级 |
| **global-default** | 20 | 128 | 默认优先级 |
| **catch-all** | 5 | 0(Reject) | 兜底优先级(直接拒绝) |

### 4.2 查看内置配置

```bash
# 查看所有 FlowSchema(按优先级排序)
kubectl get flowschemas --sort-by=.spec.matchingPrecedence

# 查看所有 PriorityLevelConfiguration
kubectl get prioritylevelconfigurations

# 查看特定 FlowSchema 详情
kubectl get flowschema system-nodes -o yaml

# 查看特定 PriorityLevel 详情
kubectl get prioritylevelconfiguration workload-high -o yaml
```

---

## 5. 内部原理

### 5.1 请求分类流程

```
客户端请求: GET /api/v1/namespaces/default/pods
    ▼
┌─────────────────────────────────────────────────────────────────┐
│ 1. 请求属性提取                                                 │
│    - User: system:serviceaccount:default:my-app                 │
│    - Groups: [system:serviceaccounts, system:authenticated]     │
│    - Resource: pods                                             │
│    - Namespace: default                                         │
│    - Verb: get                                                  │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 2. FlowSchema 匹配(按 matchingPrecedence 升序遍历)              │
│    ✗ system-leader-election (precedence=100)                    │
│      - 不匹配: resourceRules 仅包含 leases                      │
│    ✗ system-nodes (precedence=500)                              │
│      - 不匹配: subjects 仅包含 system:nodes 组                  │
│    ✓ service-accounts (precedence=9000)                         │
│      - 匹配: subjects 包含 system:serviceaccounts 组            │
│      - 匹配: resourceRules 包含 pods                            │
│      - 结果: priorityLevelConfiguration=workload-low            │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 3. 确定 Flow ID(用于 Fair Queuing)                              │
│    - distinguisherMethod.type = ByUser                          │
│    - Flow ID = hash(User + Namespace)                           │
│              = hash("system:serviceaccount:default:my-app")     │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 Fair Queuing 算法

```
请求到达 PriorityLevelConfiguration: workload-low
    ▼
┌─────────────────────────────────────────────────────────────────┐
│ 1. 检查当前并发数                                               │
│    - nominalConcurrencyShares = 100                             │
│    - 所有 PLC 总份额 = 245                                      │
│    - API Server 总并发数 = 600 (动态调整)                       │
│    - 实际并发限制 = (100/245) * 600 ≈ 245                       │
│    - 当前并发数 = 240                                           │
│    - 判断: 240 < 245 → 可立即执行                               │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 2. 执行请求                                                     │
│    - 并发计数器 +1 (240 → 241)                                  │
│    - 处理请求...                                                │
│    - 请求完成                                                   │
│    - 并发计数器 -1 (241 → 240)                                  │
└─────────────────────────────────────────────────────────────────┘
```

**如果并发已满(需要排队):**

```
请求到达时并发已满(245/245)
    ▼
┌─────────────────────────────────────────────────────────────────┐
│ 1. Shuffle Sharding 分配队列                                    │
│    - queues = 128 (总队列数)                                    │
│    - handSize = 8 (每个 Flow 对应的队列数)                      │
│    - Flow ID = hash(User + Namespace)                           │
│    - 随机选择 8 个队列: [12, 45, 67, 89, 101, 112, 120, 125]    │
│    - 选择最短队列: Queue 45 (当前 5 个请求等待)                 │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 2. 入队等待                                                     │
│    - queueLengthLimit = 50                                      │
│    - 当前队列长度 = 5 < 50 → 可入队                             │
│    - 设置超时: 默认 1 分钟(不可配置)                            │
│    - 等待...                                                    │
└────────────────────┬────────────────────────────────────────────┘
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│ 3. 出队执行                                                     │
│    - 前一个请求完成,释放 1 个并发位                             │
│    - 从队列中取出请求(FIFO)                                     │
│    - 执行请求                                                   │
└─────────────────────────────────────────────────────────────────┘
```

**如果队列已满:**

```yaml
# 返回 429 Too Many Requests
HTTP/1.1 429 Too Many Requests
Retry-After: 1
X-Retry-After: 1

{
  "kind": "Status",
  "apiVersion": "v1",
  "status": "Failure",
  "message": "too many requests",
  "reason": "TooManyRequests",
  "code": 429
}
```

### 5.3 Shuffle Sharding 隔离原理

**目标**: 防止单个故障 Flow(如故障控制器循环调用)影响其他 Flow

```
假设:
- 总队列数 queues = 128
- Hand Size handSize = 8
- 3 个 Flow: A, B, C

Flow A: 正常请求(每秒 10 个)
  → 随机分配到队列: [10, 25, 40, 55, 70, 85, 100, 115]

Flow B: 正常请求(每秒 10 个)
  → 随机分配到队列: [5, 20, 35, 50, 65, 80, 95, 110]

Flow C: 故障循环(每秒 1000 个!)
  → 随机分配到队列: [15, 30, 45, 60, 75, 90, 105, 120]

结果:
- Flow A 和 B 与 C 的队列重叠概率 = 8/128 = 6.25%(很低!)
- 即使 Flow C 填满其 8 个队列,Flow A/B 的其他队列仍可正常使用
- 隔离效果: 故障 Flow 不影响正常 Flow
```

**计算公式**:

- 重叠概率 ≈ `handSize / queues`
- 推荐配置: `queues=128, handSize=8` → 重叠概率 6.25%

---

## 6. 生产案例

### 6.1 案例 1: 租户隔离(多团队共享集群)

**场景**: 3 个团队共享集群,防止某团队的控制器故障影响其他团队

```yaml
# 团队 A(高优先级业务) - 高优先级
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: team-a-high-priority
spec:
  priorityLevelConfiguration:
    name: team-a-priority
  matchingPrecedence: 500
  distinguisherMethod:
    type: ByUser
  rules:
    - subjects:
        # 团队 A 的所有 ServiceAccount
        - kind: Group
          group:
            name: system:serviceaccounts:team-a
      resourceRules:
        - apiGroups: ["*"]
          resources: ["*"]
          namespaces: ["team-a"]
          verbs: ["*"]

---
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: team-a-priority
spec:
  type: Limited
  limited:
    nominalConcurrencyShares: 100  # 最高份额
    lendablePercent: 30             # 限制借出(保证自身资源)
    borrowingLimitPercent: 50       # 限制借入(避免挤占其他团队)
    limitResponse:
      type: Queue
      queuing:
        queues: 128
        queueLengthLimit: 100
        handSize: 8

---
# 团队 B 和 C(常规业务) - 中优先级
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: team-bc-normal-priority
spec:
  priorityLevelConfiguration:
    name: team-bc-priority
  matchingPrecedence: 1000
  distinguisherMethod:
    type: ByUser
  rules:
    - subjects:
        - kind: Group
          group:
            name: system:serviceaccounts:team-b
        - kind: Group
          group:
            name: system:serviceaccounts:team-c
      resourceRules:
        - apiGroups: ["*"]
          resources: ["*"]
          namespaces: ["team-b", "team-c"]
          verbs: ["*"]

---
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: team-bc-priority
spec:
  type: Limited
  limited:
    nominalConcurrencyShares: 50   # 中等份额
    lendablePercent: 50
    borrowingLimitPercent: 100
    limitResponse:
      type: Queue
      queuing:
        queues: 64
        queueLengthLimit: 50
        handSize: 8
```

### 6.2 案例 2: 保护 API Server - 限制 List 大请求

**场景**: 防止 `kubectl get pods --all-namespaces` 类大查询压垮 API Server

```yaml
# 为大查询单独分配低优先级
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: limit-large-list-requests
spec:
  priorityLevelConfiguration:
    name: large-query-limited
  matchingPrecedence: 800
  distinguisherMethod:
    type: ByUser
  rules:
    # 匹配 list 所有命名空间的请求
    - subjects:
        - kind: Group
          group:
            name: system:authenticated
      resourceRules:
        - apiGroups: ["", "apps", "batch"]
          resources: ["*"]
          namespaces: ["*"]   # 所有命名空间
          verbs: ["list"]     # 仅 list 动词

---
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: large-query-limited
spec:
  type: Limited
  limited:
    nominalConcurrencyShares: 10   # 低份额(限制并发)
    lendablePercent: 0              # 不借出
    borrowingLimitPercent: 0        # 不借入
    limitResponse:
      type: Queue
      queuing:
        queues: 32          # 较少队列
        queueLengthLimit: 10   # 短队列(快速拒绝)
        handSize: 4

---
# 正常 get/watch 请求保持高优先级
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: normal-read-requests
spec:
  priorityLevelConfiguration:
    name: workload-high   # 使用内置高优先级
  matchingPrecedence: 900
  distinguisherMethod:
    type: ByUser
  rules:
    - subjects:
        - kind: Group
          group:
            name: system:authenticated
      resourceRules:
        - apiGroups: ["*"]
          resources: ["*"]
          namespaces: ["*"]
          verbs: ["get", "watch"]  # 小查询
```

### 6.3 案例 3: Leader Election 优先级提升

**场景**: 确保控制器的 Leader Election 请求不受限流影响

```yaml
# 为自定义控制器的 Leader Election 创建高优先级 FlowSchema
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: custom-controller-leader-election
spec:
  priorityLevelConfiguration:
    name: leader-election  # 使用内置高优先级
  matchingPrecedence: 150
  distinguisherMethod:
    type: ByUser
  rules:
    - subjects:
        # 自定义控制器的 ServiceAccount
        - kind: ServiceAccount
          serviceAccount:
            name: my-controller
            namespace: my-system
      resourceRules:
        # Leader Election 使用 coordination.k8s.io/v1 Lease
        - apiGroups: ["coordination.k8s.io"]
          resources: ["leases"]
          namespaces: ["my-system"]
          verbs: ["get", "create", "update"]
        # 旧版本使用 ConfigMap 或 Endpoints
        - apiGroups: [""]
          resources: ["configmaps", "endpoints"]
          namespaces: ["my-system"]
          verbs: ["get", "create", "update"]
```

### 6.4 案例 4: Webhook 超时保护

**场景**: Admission Webhook 调用 API Server 时避免死锁(Webhook 等待 API Server,API Server 等待 Webhook)

```yaml
# 为 Webhook ServiceAccount 创建豁免或超高优先级
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: webhook-high-priority
spec:
  priorityLevelConfiguration:
    name: webhook-priority
  matchingPrecedence: 200
  distinguisherMethod:
    type: ByUser
  rules:
    - subjects:
        - kind: ServiceAccount
          serviceAccount:
            name: my-webhook
            namespace: webhook-system
      resourceRules:
        - apiGroups: ["*"]
          resources: ["*"]
          namespaces: ["*"]
          verbs: ["get", "list"]  # Webhook 通常只读取数据

---
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: webhook-priority
spec:
  type: Limited
  limited:
    nominalConcurrencyShares: 50
    lendablePercent: 0        # 不借出(确保 Webhook 始终有资源)
    borrowingLimitPercent: 0  # 不借入(避免挤占其他优先级)
    limitResponse:
      type: Reject  # 直接拒绝(避免排队导致 Webhook 超时)
```

### 6.5 案例 5: 监控指标收集优先级

**场景**: Prometheus 等监控系统定期抓取 `/metrics` 端点

```yaml
# 为监控系统创建专用 FlowSchema
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: monitoring-metrics
spec:
  priorityLevelConfiguration:
    name: monitoring-priority
  matchingPrecedence: 1500
  distinguisherMethod:
    type: ByUser
  rules:
    - subjects:
        # Prometheus ServiceAccount
        - kind: ServiceAccount
          serviceAccount:
            name: prometheus
            namespace: monitoring
      # 非资源请求(metrics 端点)
      nonResourceRules:
        - nonResourceURLs: ["/metrics"]
          verbs: ["get"]
    - subjects:
        # Prometheus 还需要读取 Pod/Service 信息
        - kind: ServiceAccount
          serviceAccount:
            name: prometheus
            namespace: monitoring
      resourceRules:
        - apiGroups: [""]
          resources: ["pods", "services", "endpoints"]
          namespaces: ["*"]
          verbs: ["get", "list", "watch"]

---
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: monitoring-priority
spec:
  type: Limited
  limited:
    nominalConcurrencyShares: 20
    lendablePercent: 50
    borrowingLimitPercent: 100
    limitResponse:
      type: Queue
      queuing:
        queues: 32
        queueLengthLimit: 20
        handSize: 4
```

---

## 7. 故障排查

### 7.1 请求被限流(429 Too Many Requests)

**症状**: 客户端频繁收到 429 响应

```bash
# 查看 APF 指标
kubectl get --raw /metrics | grep apiserver_flowcontrol

# 关键指标:
# apiserver_flowcontrol_rejected_requests_total  # 被拒绝的请求数
# apiserver_flowcontrol_current_inqueue_requests # 当前排队请求数
# apiserver_flowcontrol_request_concurrency_limit # 并发限制

# 查看特定 PriorityLevel 的状态
kubectl get prioritylevelconfiguration workload-low -o yaml

# 检查 FlowSchema 匹配
kubectl get flowschemas --sort-by=.spec.matchingPrecedence
```

**解决方案**:

1. **提高并发份额**:

```bash
kubectl patch prioritylevelconfiguration workload-low --type=json -p='[
  {"op": "replace", "path": "/spec/limited/nominalConcurrencyShares", "value": 150}
]'
```

2. **增加队列长度**:

```bash
kubectl patch prioritylevelconfiguration workload-low --type=json -p='[
  {"op": "replace", "path": "/spec/limited/limitResponse/queuing/queueLengthLimit", "value": 100}
]'
```

3. **创建专用 FlowSchema**(如果特定用户需要更高优先级):

```yaml
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: special-controller-high-priority
spec:
  priorityLevelConfiguration:
    name: workload-high  # 使用更高优先级
  matchingPrecedence: 700  # 优先匹配
  distinguisherMethod:
    type: ByUser
  rules:
    - subjects:
        - kind: ServiceAccount
          serviceAccount:
            name: special-controller
            namespace: default
      resourceRules:
        - apiGroups: ["*"]
          resources: ["*"]
          namespaces: ["*"]
          verbs: ["*"]
```

### 7.2 查看请求匹配的 FlowSchema

```bash
# 启用 API Server 审计日志(需要配置 audit policy)
# 审计日志会包含 flowSchema 和 priorityLevel 信息

# 示例审计日志:
{
  "kind": "Event",
  "apiVersion": "audit.k8s.io/v1",
  "level": "Metadata",
  "auditID": "xxx",
  "stage": "ResponseComplete",
  "requestURI": "/api/v1/namespaces/default/pods",
  "verb": "list",
  "user": {
    "username": "system:serviceaccount:default:my-app"
  },
  "annotations": {
    "authorization.k8s.io/decision": "allow",
    "flowcontrol.apiserver.k8s.io/flowSchema": "service-accounts",
    "flowcontrol.apiserver.k8s.io/priorityLevel": "workload-low"
  }
}
```

### 7.3 FlowSchema 未生效

**症状**: 创建了 FlowSchema 但请求仍匹配到 `global-default`

```bash
# 检查 FlowSchema 状态
kubectl get flowschema my-flowschema -o yaml

# 常见问题:
# 1. matchingPrecedence 太大(被其他 FlowSchema 提前匹配)
# 2. subjects 不匹配(用户名/组名错误)
# 3. resourceRules 不匹配(apiGroups/resources/verbs 配置错误)

# 调试技巧: 逐步简化匹配条件
subjects:
  - kind: Group
    group:
      name: system:authenticated  # 匹配所有认证用户
resourceRules:
  - apiGroups: ["*"]
    resources: ["*"]
    namespaces: ["*"]
    verbs: ["*"]

# 如果上述配置生效,再逐步添加具体限制
```

### 7.4 监控 APF 性能

```bash
# 查看 APF 相关指标
kubectl get --raw /metrics | grep apiserver_flowcontrol

# 关键指标:
# apiserver_flowcontrol_rejected_requests_total{priority_level="xxx"}
#   → 被拒绝的请求数(如果持续增长,说明配额不足)

# apiserver_flowcontrol_dispatched_requests_total{priority_level="xxx"}
#   → 已执行的请求数

# apiserver_flowcontrol_current_inqueue_requests{priority_level="xxx"}
#   → 当前排队的请求数(如果持续很高,说明并发不足)

# apiserver_flowcontrol_request_queue_length_after_enqueue_bucket{priority_level="xxx"}
#   → 入队后队列长度分布(直方图)

# apiserver_flowcontrol_request_wait_duration_seconds_bucket{priority_level="xxx"}
#   → 请求等待时间分布(直方图)

# apiserver_flowcontrol_request_execution_seconds_bucket{priority_level="xxx"}
#   → 请求执行时间分布(直方图)
```

**Prometheus 告警规则示例**:

```yaml
# prometheus-rules.yaml
groups:
  - name: apiserver-apf
    interval: 30s
    rules:
      # 告警: 某 PriorityLevel 拒绝率超过 5%
      - alert: HighAPFRejectionRate
        expr: |
          rate(apiserver_flowcontrol_rejected_requests_total[5m])
          /
          rate(apiserver_flowcontrol_dispatched_requests_total[5m])
          > 0.05
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "API Server APF rejection rate high"
          description: "PriorityLevel {{ $labels.priority_level }} rejection rate is {{ $value | humanizePercentage }}"
      
      # 告警: 队列长度持续很高
      - alert: HighAPFQueueLength
        expr: |
          apiserver_flowcontrol_current_inqueue_requests > 50
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "API Server APF queue length high"
          description: "PriorityLevel {{ $labels.priority_level }} has {{ $value }} requests queued"
```

### 7.5 迁移旧配置(从 Max-inflight-requests)

如果集群仍使用旧的 `--max-requests-inflight` 参数:

```bash
# 查看当前配置
kubectl -n kube-system describe pod kube-apiserver-xxx | grep max-requests-inflight

# 旧参数:
# --max-requests-inflight=400
# --max-mutating-requests-inflight=200

# 迁移到 APF:
# 1. 启用 APF(v1.29+ 默认启用)
# 2. 移除旧参数(逐步移除,观察影响)
# 3. 根据监控数据调整 PriorityLevelConfiguration
```

---

## 📚 参考资源

- **官方文档**:
  - [API Priority and Fairness](https://kubernetes.io/docs/concepts/cluster-administration/flow-control/)
  - [FlowSchema API Reference](https://kubernetes.io/docs/reference/kubernetes-api/cluster-resources/flow-schema-v1/)
  - [PriorityLevelConfiguration API Reference](https://kubernetes.io/docs/reference/kubernetes-api/cluster-resources/priority-level-configuration-v1/)
- **KEP (Kubernetes Enhancement Proposal)**:
  - [KEP-1040: API Priority and Fairness](https://github.com/kubernetes/enhancements/tree/master/keps/sig-api-machinery/1040-priority-and-fairness)

---

**最佳实践总结**:

1. **保留内置配置**: 不要删除或修改内置 FlowSchema/PriorityLevel,避免影响系统组件
2. **优先级规划**: 关键业务使用低 `matchingPrecedence`(高优先级),避免被 `global-default` 捕获
3. **Shuffle Sharding**: 使用推荐配置 `queues=64-128, handSize=8`,避免故障 Flow 影响全局
4. **监控告警**: 监控 `apiserver_flowcontrol_rejected_requests_total`,及时调整配额
5. **避免滥用 Exempt**: `type: Exempt` 仅用于极少数关键组件(如 `system:masters`)
6. **区分器选择**: 使用 `ByUser` 隔离不同用户/控制器,使用 `ByNamespace` 隔离租户
7. **测试验证**: 在非生产环境充分测试 APF 配置,避免意外限流影响业务

---

🚀 **APF 是保护 API Server 免受过载的核心机制,合理配置是大规模集群稳定运行的关键!**
