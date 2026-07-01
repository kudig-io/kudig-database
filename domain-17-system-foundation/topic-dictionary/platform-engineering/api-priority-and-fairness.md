---
title: API 优先级与公平性（API Priority and Fairness）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- kubelet
- prometheus
- argocd
- webhook
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- API 优先级与公平性（API Priority and Fairness） 是什么
- 如何 API 优先级与公平性（API Priority and Fairness）
trigger_keywords:
- API
- 优先级与公平性
- Priority
- and
- Fairness
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
- gitops-basics
- tls-basics
created: "2026-05-23"
created: 2026-05
---

# API 优先级与公平性（API Priority and Fairness）

## 概述

FEATURE STATE: `[[Kubernetes|Kubernetes]] v1.29 [stable]`

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

默认配置未对 [[kubelet|kubelet]] 的无凭证健康检查请求给予特殊处理。可以通过添加自定义 FlowSchema 将这些请求豁免：

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

APF 暴露了大量 [[Prometheus|Prometheus]] 指标，包括：

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

## 故障排查

| 症状 | 可能原因 | 排查命令/方法 |
|------|---------|-------------|
| 客户端收到 HTTP 429 Too Many Requests | 请求被 APF 限流 | `kubectl get --raw /metrics \| grep apiserver_flowcontrol_rejected_requests_total` |
| 特定控制器请求延迟高 | 该 flow 排队拥挤 | 检查 `apiserver_flowcontrol_request_wait_duration_seconds` 按 priority_level 分组 |
| 关键操作（leader election）被限流 | FlowSchema 匹配优先级不正确 | `kubectl get flowschema --sort-by='.spec.matchingPrecedence'` 检查匹配顺序 |
| 自定义 FlowSchema 被覆盖 | `apf.kubernetes.io/autoupdate-spec` 注解未设为 false | `kubectl get flowschema <name> -o yaml \| grep autoupdate` |
| apiserver 整体响应变慢 | 总并发限制不足 | 检查 `--max-requests-inflight` 和 `--max-mutating-requests-inflight` 参数 |
| Webhook 递归调用死锁 | 优先级反转，附属请求被限流 | 为 webhook 回调创建 exempt FlowSchema 或独立高优先级 |
| 建议配置被删除后自动重建 | 这是预期行为 | 如需修改，覆盖 spec 而非删除；设置 `autoupdate-spec: false` |

## 生产检查清单

- [ ] 确认 APF 已启用（1.29+ 默认稳定版启用）
- [ ] 监控 `apiserver_flowcontrol_rejected_requests_total`，配置告警阈值
- [ ] 监控 `apiserver_flowcontrol_request_wait_duration_seconds` P99 延迟
- [ ] 为关键控制器（如 [[cert-manager|cert-manager]]、ArgoCD）创建独立 FlowSchema 和高份额 PriorityLevel
- [ ] 自定义配置的 `apf.kubernetes.io/autoupdate-spec` 注解已设为 `false`
- [ ] 避免大量未分页的 list 请求，使用 `limit` 和 `continue` 参数
- [ ] 递归 webhook 场景已测试，确保无死锁风险
- [ ] 定期审查 FlowSchema 匹配情况，清理无效规则
- [ ] `--max-requests-inflight` 和 `--max-mutating-requests-inflight` 根据集群规模调优
- [ ] 建议配置的 `global-default` 优先级未被过度限制

## 命令快速参考

```bash
# 查看所有 FlowSchema（按匹配优先级排序）
kubectl get flowschema --sort-by='.spec.matchingPrecedence'

# 查看所有 PriorityLevelConfiguration
kubectl get prioritylevelconfiguration

# 查看特定 FlowSchema 详情
kubectl describe flowschema <name>

# 查看 APF 相关指标（被拒绝请求数）
kubectl get --raw /metrics | grep apiserver_flowcontrol_rejected_requests_total

# 查看当前排队和执行中请求数
kubectl get --raw /metrics | grep apiserver_flowcontrol_current_inqueue_requests
kubectl get --raw /metrics | grep apiserver_flowcontrol_current_executing_requests

# 查看各优先级 seat 利用率
kubectl get --raw /metrics | grep apiserver_flowcontrol_priority_level_seat_utilization

# 查看请求等待时间分布
kubectl get --raw /metrics | grep apiserver_flowcontrol_request_wait_duration_seconds

# 检查自定义配置是否被自动更新保护
kubectl get flowschema <name> -o jsonpath='{.metadata.annotations.apf\.kubernetes\.io/autoupdate-spec}'

# 查看请求匹配到的 FlowSchema（通过审计日志）
# 请求响应头中包含：X-Kubernetes-PF-FlowSchema-UID 和 X-Kubernetes-PF-PriorityLevel-UID
```

## 交叉引用

- [proxies-in-kubernetes.md](./proxies-in-kubernetes.md) — apiserver 代理与流量入口
- [admission-webhook-good-practices.md](./admission-webhook-good-practices.md) — webhook 递归调用与 APF 配合
- [extending-the-kubernetes-api.md](./extending-the-kubernetes-api.md) — API 扩展机制
- [coordinated-leader-election.md](./coordinated-leader-election.md) — leader election 请求的优先级保障
- [compatibility-version-for-control-plane.md](./compatibility-version-for-control-plane.md) — 控制平面版本兼容性

## 参考链接

- [API Priority and Fairness - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/flow-control/)

## Related
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
