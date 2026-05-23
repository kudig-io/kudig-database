---
title: 工具授权注册表 (02-ai-agents)
description: 'title: 工具授权注册表'
category: general
tags:
- ai
- ai-agent
- etcd
- apiserver
- kubelet
- prometheus
- grafana
- istio
- helm
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 工具授权注册表 是什么
- 如何 工具授权注册表
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- 工具授权注册表
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- tls-basics
- logging-basics
created: "2026-05-23"
---

title: 工具授权注册表
description: K8S 运维诊断 Agent 的工具授权注册表、调用参数规范与安全约束
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[etcd|etcd]]
- apiserver
- [[kubelet|kubelet]]
- [[Prometheus|prometheus]]
- grafana
last_updated: 2026-04
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- 工具授权注册表 是什么
- 如何 工具授权注册表
trigger_keywords:
- 工具授权注册表
- ai
- agent
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
# 工具授权注册表

## 1. 授权工具清单

### 1.1 信息采集工具（只读，默认授权）

| 工具 | 用途 | 权限级别 | 输出格式 |
|------|------|---------|---------|
| `kubectl get` | 查看资源列表和状态 | 只读 | table/json/yaml |
| `kubectl describe` | 查看资源详细信息和事件 | 只读 | text |
| `kubectl logs` | 查看 Pod 日志 | 只读 | text |
| `kubectl top` | 查看资源使用率 | 只读 | table |
| `kubectl events` | 查看集群事件 | 只读 | table |
| `kubectl api-resources` | 查看可用 API 资源 | 只读 | table |
| `kubectl cluster-info` | 查看集群基本信息 | 只读 | text |
| `kubectl version` | 查看版本信息 | 只读 | json |

### 1.2 监控查询工具（只读，默认授权）

| 工具 | 用途 | 权限级别 | 参数规范 |
|------|------|---------|---------|
| `prometheus_query` | 执行 PromQL 即时查询 | 只读 | `query`: PromQL 表达式 |
| `prometheus_query_range` | 执行 PromQL 范围查询 | 只读 | `query`, `start`, `end`, `step` |
| `loki_search` | 搜索日志 | 只读 | `query`: LogQL 表达式, `limit` |
| `grafana_dashboard` | 查看 Dashboard 面板 | 只读 | `dashboard_uid`, `panel_id` |

### 1.3 辅助工具（只读，默认授权）

| 工具 | 用途 | 权限级别 | 说明 |
|------|------|---------|------|
| `view_text_file` | 读取文本文件 | 只读 | 用于读取 SKILL.md 等参考文件 |
| `execute_shell_command` | 执行 Shell 命令 | 受限只读 | 只允许信息采集类命令 |
| `helm_info` | 查看 Helm Release 信息 | 只读 | `helm list`, `helm get values` |
| `etcdctl_status` | 查看 etcd 集群状态 | 只读 | 仅限 `endpoint status/health` |

### 1.4 有限写操作工具（需用户确认）

| 工具 | 用途 | 权限级别 | 确认要求 |
|------|------|---------|---------|
| `kubectl apply` | 应用配置变更 | 有限写 | 必须先展示 diff，等待用户确认 |
| `kubectl scale` | 调整副本数 | 有限写 | 显示当前值和目标值，等待确认 |
| `kubectl rollout` | 管理滚动更新 | 有限写 | 仅允许 `restart` 和 `undo` |
| `kubectl label` | 管理标签 | 有限写 | 显示变更内容，等待确认 |
| `kubectl annotate` | 管理注解 | 有限写 | 显示变更内容，等待确认 |

## 2. 工具使用优先级

```
诊断场景的工具调用顺序:

Level 1: 宏观状态（必做）
  kubectl get pods -n <ns> -o wide
  kubectl get nodes -o wide
  kubectl get events -n <ns> --sort-by=.lastTimestamp

Level 2: 微观详情（按需）
  kubectl describe <resource> -n <ns>
  kubectl logs <pod> -n <ns> [--previous] [--tail=100]
  kubectl top pods/nodes

Level 3: 监控数据（深度分析）
  prometheus_query（资源使用趋势、错误率）
  loki_search（日志关键词搜索）

Level 4: 写操作（仅在用户确认后）
  kubectl apply/scale/rollout
```

## 3. 参数规范

### 3.1 kubectl 通用参数

```
必须包含的参数:
  -n <namespace>          # 所有命令必须显式指定 namespace
  --context <context>      # 多集群环境必须指定 context

推荐的输出格式:
  -o wide                  # 列表查询用 wide 格式
  -o json                  # 需要解析时用 json
  -o jsonpath='{...}'      # 精确提取单个字段
  -o custom-columns=...    # 自定义列输出

日志查询参数:
  --tail=100               # 默认限制 100 行
  --since=30m              # 默认最近 30 分钟
  --previous               # CrashLoop 时查看上一次日志
  --timestamps             # 包含时间戳
```

### 3.2 PromQL 常用模板

```yaml
# Pod CPU 使用率
sum(rate(container_cpu_usage_seconds_total{namespace="<ns>", pod=~"<pod>.*"}[5m])) by (pod)

# Pod 内存使用量
sum(container_memory_working_set_bytes{namespace="<ns>", pod=~"<pod>.*"}) by (pod)

# Node CPU 使用率
100 - (avg by(instance) (rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100)

# API Server 请求延迟 P99
histogram_quantile(0.99, sum(rate(apiserver_request_duration_seconds_bucket[5m])) by (le, verb))

# etcd 延迟
histogram_quantile(0.99, sum(rate(etcd_disk_wal_fsync_duration_seconds_bucket[5m])) by (le))

# Pod 重启次数
sum(increase(kube_pod_container_status_restarts_total{namespace="<ns>"}[1h])) by (pod)
```

### 3.3 LogQL 常用模板

```yaml
# Pod 错误日志
{namespace="<ns>", pod=~"<pod>.*"} |= "error" | logfmt

# kubelet 日志
{unit="kubelet"} |= "failed" | logfmt

# API Server 审计日志
{job="apiserver-audit"} | json | verb="delete"
```

## 4. 安全约束

### 4.1 命令黑名单

```
绝对禁止的命令模式（正则匹配）:

kubectl\s+delete\s+(namespace|ns|node|pv)\b
kubectl\s+delete\s+.*--all\b
kubectl\s+drain\s+.*--force
kubectl\s+cordon\b
kubectl\s+taint\b
kubectl\s+edit\b
kubectl\s+exec\s+.*--\s+(rm|mv|dd|mkfs|fdisk)
kubectl\s+create\s+clusterrolebinding
helm\s+uninstall\b
etcdctl\s+del\b
```

### 4.2 Namespace 白名单模式

```
默认策略: 允许所有非系统 Namespace 的只读操作

受保护的 Namespace（写操作需要额外审批）:
  - kube-system
  - kube-public
  - kube-node-lease
  - monitoring
  - istio-system
  - cert-manager

完全禁止操作的 Namespace:
  - default（生产集群不应使用 default）
```

### 4.3 输出脱敏规则

```
自动脱敏的内容:

Secret 类型:
  kubectl get secret -o yaml → data 字段用 "***REDACTED***" 替代
  kubectl get secret -o json → 同上

环境变量:
  包含 KEY/TOKEN/PASSWORD/SECRET/CREDENTIAL 关键词的值 → "***"

ConfigMap:
  包含连接字符串、密码、API Key 的值 → 部分脱敏（保留前 4 位）
```

## 5. 工具组合模板

### 5.1 Pod Pending 诊断工具链

```bash
# Step 1: 确认状态
kubectl get pod <pod> -n <ns> -o wide

# Step 2: 查看事件
kubectl describe pod <pod> -n <ns> | grep -A 20 "Events:"

# Step 3: 检查节点资源
kubectl top nodes
kubectl get nodes -o custom-columns=NAME:.metadata.name,CPU:.status.allocatable.cpu,MEM:.status.allocatable.memory

# Step 4: 检查调度约束
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.nodeSelector}'
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.affinity}'
```

### 5.2 Node NotReady 诊断工具链

```bash
# Step 1: 确认节点状态
kubectl get nodes -o wide
kubectl describe node <node> | grep -A 10 "Conditions:"

# Step 2: 检查 kubelet
kubectl get --raw /api/v1/nodes/<node>/proxy/healthz

# Step 3: 检查资源压力
kubectl top node <node>
kubectl describe node <node> | grep -A 5 "Allocated resources:"

# Step 4: 查看节点事件
kubectl get events --field-selector involvedObject.name=<node> --sort-by=.lastTimestamp
```

### 5.3 OOM 诊断工具链

```bash
# Step 1: 确认 OOM
kubectl describe pod <pod> -n <ns> | grep -A 5 "Last State:"
kubectl get events -n <ns> --field-selector reason=OOMKilling

# Step 2: 查看资源配置
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.containers[*].resources}'

# Step 3: 查看实际使用
kubectl top pod <pod> -n <ns> --containers

# Step 4: 查看历史趋势（Prometheus）
# sum(container_memory_working_set_bytes{namespace="<ns>", pod="<pod>"}) by (container)
```

## 6. MCP 工具集成

### 6.1 MCP Server 配置

```yaml
# 可通过 MCP 协议集成的远程工具
mcp_servers:
  - name: kubectl-mcp
    transport: stdio
    command: kubectl-mcp-server
    capabilities:
      - kubectl_get
      - kubectl_describe
      - kubectl_logs

  - name: prometheus-mcp
    transport: streamable_http
    url: http://prometheus-mcp:8080/mcp
    capabilities:
      - prometheus_query
      - prometheus_query_range

  - name: loki-mcp
    transport: streamable_http
    url: http://loki-mcp:8080/mcp
    capabilities:
      - loki_search
```

### 6.2 AGENTScope Toolkit 注册示例

```python
from agentscope.tool import Toolkit, execute_shell_command, view_text_file

toolkit = Toolkit()

# 基础工具
toolkit.register_tool_function(execute_shell_command)
toolkit.register_tool_function(view_text_file)

# MCP 远程工具
# await toolkit.register_mcp_client(kubectl_mcp_client)
# await toolkit.register_mcp_client(prometheus_mcp_client)

# Agent Skill（领域知识）
toolkit.register_agent_skill("openclaw-workspace")
```

---

*本文件定义 Agent 的工具授权边界。添加新工具需要安全评审，删除工具需要评估影响范围。*

## Related

- 29-agentscope-studio-skill-demo
- [[log|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm|helm]]

## See Also

- SKILL
- SOUL
- USER
- AGENTS
