# API 优先级与公平性（API Priority and Fairness）

## 概述

FEATURE STATE: `Kubernetes v1.29 [stable]`

在 Kubernetes 集群中，控制 kube-apiserver 在高负载下的行为是集群管理员的关键任务。API 优先级与公平性（API Priority and Fairness，简称 APF）是一种比传统的 `--max-requests-inflight` 和 `--max-mutating-requests-inflight` 更精细的流量控制机制。APF 能够对请求进行分类和隔离，并引入有限的队列机制，使得在短时突发流量下不会被直接拒绝，同时使用公平队列算法防止单个不良控制器饿死其他客户端。

## 核心概念/原理

- **流量分类**：通过 FlowSchema 根据请求属性将入站请求分类。
- **优先级隔离**：通过 PriorityLevelConfiguration 为不同优先级维护独立的并发限制，确保不同优先级的请求互不饿死。
- **公平队列**：在同一优先级内，通过公平队列算法将请求分配到不同的 flow，并使用 shuffle sharding 技术隔离低强度 flow 免受高强度 flow 的影响。
- ** seats（座位）**：表示并发单位。大多数请求占用 1 个 seat，但大型 list 请求可能占用多个 seats。
- **豁免请求**：部分重要请求不受 APF 限制，防止配置错误完全瘫痪 API 服务器。

## 关键机制或特性

### 启用与禁用

APF 默认启用，通过 kube-apiserver 的 `--enable-priority-and-fairness` 标志控制。相关 API Group：

- `flowcontrol.apiserver.k8s.io/v1`（1.29 引入，稳定版，默认启用）
- `flowcontrol.apiserver.k8s.io/v1beta3`（默认启用，1.29 中已弃用）

禁用 APF：

```bash
kube-apiserver --enable-priority-and-fairness=false
```

### PriorityLevelConfiguration

定义可用的优先级级别，包括：

- **名义并发份额（nominal concurrency shares）**：决定该优先级级别的并发预算分配比例。
- **类型**：`Reject`（直接拒绝超额请求，返回 HTTP 429）或 `Queue`（将超额请求排队）。
- **队列配置**：`queues`（队列数）、`queueLengthLimit`（队列长度限制）、`handSize`（shuffle sharding 手牌大小），用于调整公平性、突发容忍度和内存使用之间的权衡。

### FlowSchema

用于将单个入站请求匹配到某个 PriorityLevelConfiguration。匹配规则包括：

- `matchingPrecedence`：数值越小优先级越高，第一个匹配的 FlowSchema 生效。
- `rules`：包含 `subjects`（请求主体）和 `resourceRules`/`nonResourceRules`（资源/非资源规则）。
- `distinguisherMethod.type`：决定如何将匹配到的请求分为不同的 flow：
  - `ByUser`：按请求用户区分。
  - `ByNamespace`：按目标资源命名空间区分。
  - 空/省略：所有匹配请求视为同一个 flow。

### 默认配置

kube-apiserver 维护两类配置对象：

- **强制配置（Mandatory）**：`exempt` 和 `catch-all`，反映内置的兜底行为，不可删除，spec 必须与内置行为一致。
- **建议配置（Suggested）**：提供合理的默认配置，包括以下优先级：
  - `node-high`：节点健康更新
  - `system`：来自 `system:nodes` 的非健康请求
  - `leader-election`：内置控制器的领导者选举请求
  - `workload-high`：内置控制器的其他请求
  - `workload-low`：其他服务账户的请求
  - `global-default`：其余所有流量（如普通用户的 kubectl 命令）

建议配置对象允许用户覆盖 spec，但删除后会被自动重建。

### 递归服务器场景

在递归调用场景（如 apiserver 调用 admission webhook，webhook 又回调 apiserver）中，需要小心处理优先级反转和死锁。常见做法是让附属请求免于 APF 限制，或在 server B 上禁用 APF。

### 健康检查并发豁免

默认配置未对 kubelet 的无凭证健康检查请求给予特殊处理。可以通过添加自定义 FlowSchema 将这些请求豁免：

```yaml
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: health-for-strangers
spec:
  matchingPrecedence: 1000
  priorityLevelConfiguration:
    name: exempt
  rules:
    - nonResourceRules:
      - nonResourceURLs:
          - "/healthz"
          - "/livez"
          - "/readyz"
        verbs:
          - "*"
      subjects:
        - kind: Group
          group:
            name: "system:unauthenticated"
```

注意：这样做也可能被恶意方利用发送大量健康检查请求，建议结合外部安全机制进行防护。

### 可观测性指标

APF 暴露了大量 Prometheus 指标，包括：

- `apiserver_flowcontrol_rejected_requests_total`：被拒绝的请求总数。
- `apiserver_flowcontrol_dispatched_requests_total`：开始执行的请求总数。
- `apiserver_flowcontrol_current_inqueue_requests`：当前排队中的请求数。
- `apiserver_flowcontrol_current_executing_requests`：当前执行中的请求数。
- `apiserver_flowcontrol_request_wait_duration_seconds`：请求在队列中的等待时间。
- `apiserver_flowcontrol_priority_level_seat_utilization`：各优先级的 seat 利用率。

## 使用场景

- **防止 API 服务器过载**：在高并发场景下，通过分类和队列机制保护 apiserver 不被洪水请求冲垮。
- **保护关键流量**：确保领导者选举、节点心跳、内置控制器等关键请求不受普通工作负载影响。
- **隔离不良客户端**：防止单个 buggy 控制器或 Pod 的大量请求饿死其他客户端。
- **精细化流量治理**：为不同租户、命名空间或用户组分配不同的优先级和并发配额。

## 最佳实践/注意事项

- 监控 `apiserver_flowcontrol_rejected_requests_total` 和 `apiserver_flowcontrol_request_wait_duration_seconds`，及时发现被限流或延迟的请求。
- 优化请求：
  - 降低请求速率。
  - 避免大量昂贵的并发 list 请求，使用分页减少 seat 占用。
  - 在合适场景下用 watch 替代轮询 list。
- 调整 APF 设置：
  - 增加 `max-requests-inflight` 和 `max-mutating-requests-inflight` 以提升总体并发。
  - 为高频关键请求创建独立的 FlowSchema 和高并发份额的 PriorityLevelConfiguration。
  - 将非必要或昂贵的请求隔离到低份额的优先级级别，防止其影响其他 flow。
- 修改默认 FlowSchema 或 PriorityLevelConfiguration 时，需要将其 `apf.kubernetes.io/autoupdate-spec` 注解设为 `false`，防止被自动维护覆盖。
- 在递归服务器场景中，确保附属请求免于 APF 限制或合理配置优先级，避免死锁和优先级反转。

## 参考链接

- [API Priority and Fairness - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/flow-control/)
