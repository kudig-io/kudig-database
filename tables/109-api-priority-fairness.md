# 68 - API 优先级与公平性 (API Priority and Fairness)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 高级

## APF 架构概览

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                    API Priority and Fairness (APF) 架构                             │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                          请求入口 (Incoming Requests)                         │   │
│  │                                                                               │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │   │
│  │  │   kubectl   │  │ Controllers │  │   Kubelet   │  │    Custom Clients   │  │   │
│  │  │   用户请求  │  │  控制器请求  │  │  节点请求   │  │      自定义客户端    │  │   │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘  │   │
│  └─────────┼────────────────┼────────────────┼───────────────────┼─────────────┘   │
│            │                │                │                   │                  │
│            └────────────────┴────────────────┴───────────────────┘                  │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                        FlowSchema 匹配层                                      │   │
│  │                                                                               │   │
│  │  请求 ──▶ 按 matchingPrecedence 排序 ──▶ 逐个匹配 ──▶ 首个匹配的 FlowSchema   │   │
│  │                                                                               │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │  FlowSchema 1 (precedence: 100)   → exempt (免除限流)                   │ │   │
│  │  │  FlowSchema 2 (precedence: 200)   → system (系统组件)                   │ │   │
│  │  │  FlowSchema 3 (precedence: 800)   → leader-election (选举)             │ │   │
│  │  │  FlowSchema 4 (precedence: 1000)  → workload-high (高优先级)           │ │   │
│  │  │  FlowSchema 5 (precedence: 2000)  → workload-low (低优先级)            │ │   │
│  │  │  FlowSchema N (precedence: 10000) → catch-all (兜底)                   │ │   │
│  │  └─────────────────────────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                      PriorityLevelConfiguration 层                            │   │
│  │                                                                               │   │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐ │   │
│  │  │                        并发控制模型                                      │ │   │
│  │  │                                                                          │ │   │
│  │  │  总并发限制 (--max-requests-inflight + --max-mutating-requests-inflight) │ │   │
│  │  │           │                                                              │ │   │
│  │  │           ▼                                                              │ │   │
│  │  │  按 nominalConcurrencyShares 比例分配到各 PriorityLevel                  │ │   │
│  │  │                                                                          │ │   │
│  │  │  exempt: 无限制 (跳过限流)                                                │ │   │
│  │  │  system: 30% ──┐                                                         │ │   │
│  │  │  leader: 10% ──┼──▶ 公平排队 (Fair Queuing)                              │ │   │
│  │  │  high:   20% ──┤                                                         │ │   │
│  │  │  low:    20% ──┤    每个 PriorityLevel 内部:                             │ │   │
│  │  │  default:20% ──┘    - N 个队列 (queues)                                  │ │   │
│  │  │                     - 请求按 flow 分配到队列                              │ │   │
│  │  │                     - 公平轮询出队执行                                    │ │   │
│  │  └─────────────────────────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│  ┌──────────────────────────────────────────────────────────────────────────────┐   │
│  │                           执行层 (Execution)                                  │   │
│  │                                                                               │   │
│  │  排队等待 ──▶ 获取并发槽位 ──▶ 执行请求 ──▶ 释放槽位                         │   │
│  │      │                                                                        │   │
│  │      └──▶ 超时拒绝 (429 Too Many Requests)                                   │   │
│  │                                                                               │   │
│  └──────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

## APF 核心概念

| 概念 | 说明 | 作用 |
|-----|------|------|
| **PriorityLevel** | 定义请求优先级和资源配额 | 控制各类请求的并发限制和排队策略 |
| **FlowSchema** | 将请求分类到优先级 | 基于请求属性匹配到对应 PriorityLevel |
| **公平排队** | 防止某类请求独占资源 | 在同一 PriorityLevel 内公平分配资源 |
| **借用机制** | 空闲配额可被其他级别借用 | 提高整体资源利用率 |
| **Seat** | 并发计量单位 | 不同请求类型消耗不同 Seat 数 |

## 内置 PriorityLevel 详解

| 名称 | 类型 | 份额 | 队列数 | 用途 | 配置建议 |
|-----|------|------|-------|------|---------|
| **exempt** | Exempt | 无限制 | - | 免除限流 (system:masters) | 仅关键组件 |
| **system** | Limited | 30 | 64 | 系统组件请求 | 保持默认 |
| **leader-election** | Limited | 10 | 16 | Leader 选举请求 | 保持默认 |
| **node-high** | Limited | 40 | 64 | 节点高优先级 (心跳) | 大集群可增加 |
| **workload-high** | Limited | 40 | 128 | 高优先级工作负载 | 按需调整 |
| **workload-low** | Limited | 100 | 64 | 低优先级工作负载 | 按需调整 |
| **global-default** | Limited | 20 | 128 | 默认优先级 | 保持默认 |
| **catch-all** | Limited | 5 | 64 | 兜底 (未匹配请求) | 保持最低 |

### 内置 FlowSchema 匹配顺序

```
优先级 (matchingPrecedence) 从低到高:

1. exempt (1)           → PriorityLevel: exempt
   匹配: system:masters 组

2. system-nodes (500)   → PriorityLevel: system  
   匹配: system:nodes 组

3. system-leader-election (100) → PriorityLevel: leader-election
   匹配: kube-system 下的 leader election 请求

4. kube-controller-manager (800) → PriorityLevel: workload-high
   匹配: system:kube-controller-manager 用户

5. kube-scheduler (800) → PriorityLevel: workload-high
   匹配: system:kube-scheduler 用户

6. service-accounts (9000) → PriorityLevel: workload-low
   匹配: 所有 ServiceAccount

7. global-default (9900) → PriorityLevel: global-default
   匹配: 所有认证用户

8. catch-all (10000) → PriorityLevel: catch-all
   匹配: 所有请求 (兜底)
```

## PriorityLevelConfiguration 配置

### 完整配置示例

```yaml
# priority-level-high-throughput.yaml
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: high-throughput-controllers
spec:
  type: Limited
  limited:
    # 名义并发份额 (相对于总并发的比例)
    # 实际并发 = 总并发 × (此份额 / 所有Limited级别份额之和)
    nominalConcurrencyShares: 100
    
    # 限制响应配置
    limitResponse:
      type: Queue  # Queue 或 Reject
      queuing:
        # 队列数量 (影响公平性粒度)
        queues: 64
        
        # 每请求分发队列数 (Shuffle Sharding)
        # 请求会被散列到 handSize 个队列中的一个
        handSize: 6
        
        # 每队列长度限制
        queueLengthLimit: 50
    
    # 借用配置 (v1.29+)
    # 允许将空闲份额借给其他 PriorityLevel
    lendablePercent: 50  # 最多借出 50% 的空闲份额
    
    # 允许从其他 PriorityLevel 借入的上限
    borrowingLimitPercent: 100  # 最多借入自身份额的 100%
---
# priority-level-batch-jobs.yaml
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: batch-jobs
spec:
  type: Limited
  limited:
    nominalConcurrencyShares: 20
    limitResponse:
      type: Queue
      queuing:
        queues: 32
        handSize: 4
        queueLengthLimit: 100
    # 批量作业可以借出更多
    lendablePercent: 80
    borrowingLimitPercent: 50
---
# priority-level-reject-overflow.yaml
# 拒绝模式 - 不排队，直接拒绝超额请求
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: strict-limit
spec:
  type: Limited
  limited:
    nominalConcurrencyShares: 10
    limitResponse:
      type: Reject  # 超过并发限制直接返回 429
```

### 参数计算公式

```
实际并发限制计算:

总并发 = max-requests-inflight + max-mutating-requests-inflight
       = 400 + 200 = 600 (默认值)

某 PriorityLevel 的名义并发:
  nominalConcurrency = 总并发 × (nominalConcurrencyShares / 所有Limited份额之和)

示例:
  - system: shares=30, 实际 = 600 × (30/225) ≈ 80
  - workload-high: shares=40, 实际 = 600 × (40/225) ≈ 107
  - workload-low: shares=100, 实际 = 600 × (100/225) ≈ 267

借用后的最大并发:
  maxConcurrency = nominalConcurrency × (1 + borrowingLimitPercent/100)

Seat 消耗 (v1.29+):
  - 简单请求: 1 seat
  - LIST 请求: 1 + ceil(estimatedObjectCount / 1000) seats
  - WATCH 请求: 初始消耗较多 seat，后续释放
```

## FlowSchema 配置

### 完整配置示例

```yaml
# flowschema-high-priority.yaml
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: high-priority-controllers
spec:
  # 匹配优先级 (数字越小越先匹配)
  matchingPrecedence: 800
  
  # 关联的 PriorityLevel
  priorityLevelConfiguration:
    name: high-throughput-controllers
  
  # 区分 Flow 的字段 (用于公平排队)
  distinguisherMethod:
    type: ByUser  # ByUser 或 ByNamespace
  
  # 请求匹配规则
  rules:
    # 规则 1: 匹配特定 ServiceAccount
    - subjects:
        - kind: ServiceAccount
          serviceAccount:
            name: important-controller
            namespace: production
      resourceRules:
        - verbs: ["*"]
          apiGroups: ["*"]
          resources: ["*"]
          namespaces: ["production", "staging"]
    
    # 规则 2: 匹配特定用户组
    - subjects:
        - kind: Group
          group:
            name: platform-controllers
      resourceRules:
        - verbs: ["get", "list", "watch"]
          apiGroups: ["*"]
          resources: ["*"]
---
# flowschema-batch-operations.yaml
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: batch-operations
spec:
  matchingPrecedence: 2000
  priorityLevelConfiguration:
    name: batch-jobs
  distinguisherMethod:
    type: ByNamespace
  rules:
    # 匹配批量 list 操作
    - subjects:
        - kind: ServiceAccount
          serviceAccount:
            name: "*"
            namespace: batch-system
      resourceRules:
        - verbs: ["list", "deletecollection"]
          apiGroups: ["*"]
          resources: ["*"]
---
# flowschema-readonly-users.yaml
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: readonly-users
spec:
  matchingPrecedence: 5000
  priorityLevelConfiguration:
    name: workload-low
  distinguisherMethod:
    type: ByUser
  rules:
    - subjects:
        - kind: Group
          group:
            name: viewers
      resourceRules:
        - verbs: ["get", "list", "watch"]
          apiGroups: ["*"]
          resources: ["*"]
      nonResourceRules:
        - verbs: ["get"]
          nonResourceURLs: ["/healthz", "/readyz", "/livez"]
```

### 匹配规则详解

| 规则字段 | 说明 | 示例 |
|---------|------|------|
| `subjects.kind` | 请求发起者类型 | ServiceAccount, User, Group |
| `subjects.serviceAccount` | 匹配 ServiceAccount | name, namespace (支持通配符 *) |
| `subjects.user` | 匹配用户 | name |
| `subjects.group` | 匹配用户组 | name |
| `resourceRules.verbs` | 操作动词 | get, list, watch, create, update, delete, patch |
| `resourceRules.apiGroups` | API 组 | "", apps, batch, networking.k8s.io |
| `resourceRules.resources` | 资源类型 | pods, deployments, services |
| `resourceRules.namespaces` | 命名空间 | default, production (不指定表示所有) |
| `resourceRules.clusterScope` | 集群级资源 | true/false |
| `nonResourceRules.verbs` | 非资源操作 | get |
| `nonResourceRules.nonResourceURLs` | 非资源路径 | /healthz, /metrics, /openapi/* |

## APF 监控指标详解

### Prometheus 指标

| 指标 | 类型 | 说明 | 告警阈值建议 |
|-----|-----|------|-------------|
| `apiserver_flowcontrol_request_concurrency_limit` | Gauge | 并发限制 | - |
| `apiserver_flowcontrol_current_executing_requests` | Gauge | 当前执行请求数 | > limit × 0.9 |
| `apiserver_flowcontrol_current_inqueue_requests` | Gauge | 当前排队请求数 | > queueLength × 0.8 |
| `apiserver_flowcontrol_dispatched_requests_total` | Counter | 已分发请求总数 | - |
| `apiserver_flowcontrol_rejected_requests_total` | Counter | 拒绝请求总数 | > 0 持续 5m |
| `apiserver_flowcontrol_request_wait_duration_seconds` | Histogram | 等待时间 | P99 > 1s |
| `apiserver_flowcontrol_request_execution_seconds` | Histogram | 执行时间 | P99 > 5s |
| `apiserver_flowcontrol_nominal_limit_seats` | Gauge | 名义并发限制 (seats) | - |
| `apiserver_flowcontrol_lower_limit_seats` | Gauge | 最低并发限制 | - |
| `apiserver_flowcontrol_upper_limit_seats` | Gauge | 最高并发限制 (借用后) | - |
| `apiserver_flowcontrol_demand_seats` | Gauge | 当前需求 seats | - |
| `apiserver_flowcontrol_demand_seats_high_watermark` | Gauge | 需求高水位 | - |

### Prometheus 告警规则

```yaml
# apf-alerts.yaml
groups:
  - name: api-priority-fairness
    interval: 30s
    rules:
      # 请求拒绝率告警
      - alert: APIServerHighRejectionRate
        expr: |
          sum(rate(apiserver_flowcontrol_rejected_requests_total[5m])) by (priority_level, flow_schema)
          / 
          sum(rate(apiserver_flowcontrol_dispatched_requests_total[5m])) by (priority_level, flow_schema)
          > 0.01
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "API Server 请求拒绝率过高"
          description: "PriorityLevel {{ $labels.priority_level }} FlowSchema {{ $labels.flow_schema }} 拒绝率: {{ $value | humanizePercentage }}"
          runbook_url: "https://wiki.example.com/runbooks/apf-rejection"

      # 队列饱和告警
      - alert: APIServerQueueSaturation
        expr: |
          sum by (priority_level) (apiserver_flowcontrol_current_inqueue_requests)
          / 
          sum by (priority_level) (apiserver_flowcontrol_nominal_limit_seats)
          > 0.8
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "API Server 队列接近饱和"
          description: "PriorityLevel {{ $labels.priority_level }} 队列使用率: {{ $value | humanizePercentage }}"

      # 并发使用率过高
      - alert: APIServerHighConcurrencyUsage
        expr: |
          sum by (priority_level) (apiserver_flowcontrol_current_executing_requests)
          / 
          sum by (priority_level) (apiserver_flowcontrol_request_concurrency_limit)
          > 0.9
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "API Server 并发使用率过高"
          description: "PriorityLevel {{ $labels.priority_level }} 并发使用率: {{ $value | humanizePercentage }}"

      # 请求等待时间过长
      - alert: APIServerHighRequestLatency
        expr: |
          histogram_quantile(0.99,
            sum(rate(apiserver_flowcontrol_request_wait_duration_seconds_bucket[5m])) by (le, priority_level)
          ) > 2
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "API Server 请求等待时间过长"
          description: "PriorityLevel {{ $labels.priority_level }} P99 等待时间: {{ $value | humanizeDuration }}"

      # 高优先级请求被限流
      - alert: HighPriorityRequestsThrottled
        expr: |
          sum(rate(apiserver_flowcontrol_rejected_requests_total{priority_level=~"system|workload-high"}[5m])) > 0
        for: 2m
        labels:
          severity: critical
        annotations:
          summary: "高优先级请求被限流"
          description: "系统关键请求正在被限流，可能影响集群稳定性"

      # 借用机制触发
      - alert: APFBorrowingActive
        expr: |
          sum by (priority_level) (apiserver_flowcontrol_current_executing_requests)
          > 
          sum by (priority_level) (apiserver_flowcontrol_nominal_limit_seats)
        for: 10m
        labels:
          severity: info
        annotations:
          summary: "APF 借用机制已激活"
          description: "PriorityLevel {{ $labels.priority_level }} 正在借用其他级别的配额"
```

### Grafana Dashboard 配置

```json
{
  "dashboard": {
    "title": "API Priority and Fairness",
    "uid": "apf-dashboard",
    "panels": [
      {
        "title": "并发使用率 by PriorityLevel",
        "type": "timeseries",
        "gridPos": {"x": 0, "y": 0, "w": 12, "h": 8},
        "targets": [
          {
            "expr": "sum by (priority_level) (apiserver_flowcontrol_current_executing_requests) / sum by (priority_level) (apiserver_flowcontrol_request_concurrency_limit)",
            "legendFormat": "{{ priority_level }}"
          }
        ]
      },
      {
        "title": "请求拒绝率",
        "type": "timeseries",
        "gridPos": {"x": 12, "y": 0, "w": 12, "h": 8},
        "targets": [
          {
            "expr": "sum(rate(apiserver_flowcontrol_rejected_requests_total[5m])) by (priority_level)",
            "legendFormat": "{{ priority_level }}"
          }
        ]
      },
      {
        "title": "队列长度",
        "type": "timeseries",
        "gridPos": {"x": 0, "y": 8, "w": 12, "h": 8},
        "targets": [
          {
            "expr": "sum by (priority_level) (apiserver_flowcontrol_current_inqueue_requests)",
            "legendFormat": "{{ priority_level }}"
          }
        ]
      },
      {
        "title": "请求等待时间 P99",
        "type": "timeseries",
        "gridPos": {"x": 12, "y": 8, "w": 12, "h": 8},
        "targets": [
          {
            "expr": "histogram_quantile(0.99, sum(rate(apiserver_flowcontrol_request_wait_duration_seconds_bucket[5m])) by (le, priority_level))",
            "legendFormat": "{{ priority_level }}"
          }
        ]
      }
    ]
  }
}
```

## APF 调试命令

```bash
# ==================== 查看配置 ====================

# 查看所有 PriorityLevel
kubectl get prioritylevelconfigurations

# 查看 PriorityLevel 详情
kubectl get prioritylevelconfiguration workload-high -o yaml

# 查看所有 FlowSchema
kubectl get flowschemas

# 查看 FlowSchema 按优先级排序
kubectl get flowschemas -o custom-columns=\
'NAME:.metadata.name,PRECEDENCE:.spec.matchingPrecedence,PL:.spec.priorityLevelConfiguration.name'

# 查看特定 FlowSchema 详情
kubectl get flowschema kube-controller-manager -o yaml

# ==================== 调试接口 ====================

# 查看 APF 状态概览
kubectl get --raw /debug/api_priority_and_fairness/dump_priority_levels

# 查看队列详情
kubectl get --raw /debug/api_priority_and_fairness/dump_queues

# 查看请求分布
kubectl get --raw /debug/api_priority_and_fairness/dump_requests

# ==================== 验证请求匹配 ====================

# 发送测试请求，查看响应头中的 APF 信息
kubectl get pods -v=8 2>&1 | grep -E 'X-Kubernetes-PF'

# 响应头说明:
# X-Kubernetes-PF-FlowSchema-UID: 匹配的 FlowSchema
# X-Kubernetes-PF-PriorityLevel-UID: 使用的 PriorityLevel

# ==================== 监控脚本 ====================

# 实时监控各 PriorityLevel 的使用情况
watch -n 1 'kubectl get --raw /debug/api_priority_and_fairness/dump_priority_levels 2>/dev/null | jq -r ".[] | \"\(.name): executing=\(.currentlyExecuting)/\(.nominalCL) queued=\(.queuedRequests)\""'

# 查看最近被拒绝的请求
kubectl get --raw /metrics | grep apiserver_flowcontrol_rejected_requests_total
```

### APF 诊断脚本

```bash
#!/bin/bash
# apf-diagnose.sh - APF 诊断脚本

echo "=== PriorityLevel 配置 ==="
kubectl get prioritylevelconfigurations -o custom-columns=\
'NAME:.metadata.name,TYPE:.spec.type,SHARES:.spec.limited.nominalConcurrencyShares,QUEUES:.spec.limited.limitResponse.queuing.queues'

echo ""
echo "=== FlowSchema 配置 (按优先级排序) ==="
kubectl get flowschemas -o custom-columns=\
'NAME:.metadata.name,PRECEDENCE:.spec.matchingPrecedence,PL:.spec.priorityLevelConfiguration.name' \
--sort-by='.spec.matchingPrecedence'

echo ""
echo "=== 当前 APF 状态 ==="
kubectl get --raw /debug/api_priority_and_fairness/dump_priority_levels 2>/dev/null | \
  jq -r '["PriorityLevel", "Executing", "Nominal", "Queued", "Rejected"],
         ["------------", "---------", "-------", "------", "--------"],
         (.[] | [.name, .currentlyExecuting, .nominalCL, .queuedRequests, .rejected]) | @tsv' | \
  column -t

echo ""
echo "=== 拒绝请求统计 ==="
kubectl get --raw /metrics 2>/dev/null | \
  grep apiserver_flowcontrol_rejected_requests_total | \
  grep -v "^#" | \
  sort -t'=' -k2 -rn | head -10

echo ""
echo "=== 高延迟请求 (P99 > 1s) ==="
kubectl get --raw /metrics 2>/dev/null | \
  grep 'apiserver_flowcontrol_request_wait_duration_seconds_bucket{.*le="1"' | \
  head -5
```

## APF 最佳实践

### 配置建议矩阵

| 场景 | 建议配置 |
|-----|---------|
| **保护控制器** | 为关键控制器创建高优先级 FlowSchema |
| **限制批量操作** | 大规模 list/watch 使用低优先级 |
| **租户隔离** | 不同租户使用不同 FlowSchema，按命名空间区分 |
| **监控系统** | Prometheus 等监控组件使用专用 PriorityLevel |
| **CI/CD 系统** | 部署管道使用中等优先级，避免影响生产请求 |

### 调优建议

```yaml
# 针对大规模集群的 APF 调优

# 1. 增加控制器并发
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: PriorityLevelConfiguration
metadata:
  name: controllers-enhanced
spec:
  type: Limited
  limited:
    nominalConcurrencyShares: 150  # 增加份额
    limitResponse:
      type: Queue
      queuing:
        queues: 128  # 增加队列数
        handSize: 8
        queueLengthLimit: 100

# 2. 限制大规模 list 操作
---
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: limit-large-lists
spec:
  matchingPrecedence: 1500
  priorityLevelConfiguration:
    name: workload-low
  rules:
    - subjects:
        - kind: ServiceAccount
          serviceAccount:
            name: "*"
            namespace: "*"
      resourceRules:
        - verbs: ["list"]
          apiGroups: ["*"]
          resources: ["pods", "events", "configmaps", "secrets"]

# 3. 保护关键系统组件
---
apiVersion: flowcontrol.apiserver.k8s.io/v1
kind: FlowSchema
metadata:
  name: protect-critical-components
spec:
  matchingPrecedence: 100
  priorityLevelConfiguration:
    name: system
  rules:
    - subjects:
        - kind: ServiceAccount
          serviceAccount:
            name: "*"
            namespace: kube-system
      resourceRules:
        - verbs: ["*"]
          apiGroups: ["*"]
          resources: ["*"]
```

### 最佳实践清单

- [ ] **监控拒绝率**: 配置告警，及时发现配置问题
- [ ] **渐进调整**: 小步迭代优化配额，避免大幅变更
- [ ] **借用机制**: 合理配置 lendablePercent 提高利用率
- [ ] **区分 Flow**: 使用 distinguisherMethod 实现租户公平
- [ ] **保护控制平面**: 为关键组件设置高优先级
- [ ] **限制批量操作**: 大规模 list/deletecollection 使用低优先级
- [ ] **监控指标**: 配置 Grafana Dashboard 监控 APF 状态
- [ ] **定期审查**: 定期检查 FlowSchema 匹配是否符合预期
- [ ] **测试验证**: 变更前使用 dry-run 验证配置

## 版本变更记录

| 版本 | 变更内容 |
|------|---------|
| v1.20 | APF Beta，默认启用 |
| v1.26 | Seat 计算改进 |
| v1.29 | APF GA，借用机制 GA |
| v1.30 | 监控指标增强，支持更细粒度的 Seat 计算 |
| v1.31 | Seat 消耗优化，LIST 请求估算改进 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
