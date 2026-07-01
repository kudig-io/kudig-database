---
title: "[2026-07-08] [P1] Prometheus 高基数导致 OOM"
category: case-study
tags: [production, incident, observability, prometheus, metrics, oom]
date: "2026-07-08"
severity: P1
mttr: "38min"
status: resolved
created: "2026-05-23"
updated: "2026-05-23"
last_updated: 2026-05-23
---

# [2026-07-08] 用户 ID 标签导致 Prometheus 高基数，TSDB 内存 OOM

## 工单信息
- **工单编号**: INC-2026-0708-015
- **发现时间**: 2026-07-08 11:20 UTC
- **恢复时间**: 2026-07-08 11:58 UTC
- **影响范围**: 监控平台（Prometheus），影响所有依赖 Prometheus 的告警和 Grafana 大盘
- **业务影响**: 11:20-11:58 期间无监控告警，部分自动扩缩容依赖 metrics 的服务受影响

## 问题现象
11:20，Prometheus Pod 反复 OOMKilled：
```bash
kubectl get pods -n monitoring -l app=prometheus
# NAME                     READY   STATUS      RESTARTS
# prometheus-0             0/1     OOMKilled   5
# prometheus-1             0/1     OOMKilled   5
```

Grafana 显示 "DatasourceError"，Alertmanager 停止发送告警。

## 诊断过程

**11:22** — 检查 Prometheus 资源：
```bash
kubectl get sts prometheus -n monitoring -o jsonpath='{.spec.template.spec.containers[0].resources}' | jq .
# {
#   "limits": {"cpu": "4", "memory": "16Gi"},
#   "requests": {"cpu": "2", "memory": "8Gi"}
# }
```

**11:24** — 查看 Prometheus 日志（启动阶段）：
```bash
kubectl logs -n monitoring prometheus-0 --previous | tail -n 30
# ts=2026-07-08T11:23:45.112Z caller=main.go:1234 
#   level=error msg="Out of memory" 
# ts=2026-07-08T11:23:45.113Z caller=head.go:567 
#   level=warn msg="Error on ingesting out-of-order result" 
#   num_dropped=1500000
```

**11:26** — 检查 head chunk 大小和序列数：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n monitoring prometheus-0 -- wget -qO- localhost:9090/api/v1/status/tsdb
# {
#   "data": {
#     "headStats": {
#       "numSeries": 45230000,
#       "chunkCount": 180920000,
#       "minTime": 1751894400000,
#       "maxTime": 1751974400000
#     }
#   }
# }
```

**11:28** — 检查高基数指标：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n monitoring prometheus-0 -- \
  wget -qO- 'localhost:9090/api/v1/status/tsdb?top=10' | jq '.data.topMetrics'
# [
#   {"metric": "http_request_duration_seconds", "count": 8500000},
#   {"metric": "grpc_request_duration_seconds", "count": 4200000},
#   ...
# ]
```

**11:30** — 进一步分析 `http_request_duration_seconds`：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n monitoring prometheus-0 -- \
  wget -qO- 'localhost:9090/api/v1/series?match[]=http_request_duration_seconds' | \
  jq '.data[0:3]'
# [
#   {"__name__":"http_request_duration_seconds","user_id":"12345","endpoint":"/api/order"},
#   {"__name__":"http_request_duration_seconds","user_id":"12346","endpoint":"/api/order"},
#   {"__name__":"http_request_duration_seconds","user_id":"12347","endpoint":"/api/order"}
# ]
```

**11:32** — 确认：metrics 中包含了 `user_id` 标签，每个用户生成一个时间序列。当前在线用户约 850 万，导致 `http_request_duration_seconds` 产生 850 万条序列。

## 根因
开发团队在 07-07 的版本中，为 `http_request_duration_seconds` histogram 添加了 `user_id` 标签，意图"按用户分析请求延迟"。但 `user_id` 是高基数维度（850 万唯一值），每个用户 ID 与每个 bucket 组合生成一个时间序列，导致 Prometheus TSDB 序列数飙升至 4500 万+。Prometheus 16Gi 内存无法容纳如此庞大的 head chunk，反复 OOMKilled。

## 修复动作

**11:35** — 回滚包含高基数标签的应用版本：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout undo deployment order-api -n prod-order
# 回滚到 v2.4.9（无 user_id 标签的版本）
```

**11:38** — 临时提升 Prometheus 内存以恢复监控：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl patch sts prometheus -n monitoring --type='merge' -p '
{
  "spec": {
    "template": {
      "spec": {
        "containers": [{
          "name": "prometheus",
          "resources": {
            "limits": {"cpu": "8", "memory": "32Gi"},
            "requests": {"cpu": "4", "memory": "16Gi"}
          }
        }]
      }
    }
  }
}'
kubectl rollout restart sts prometheus -n monitoring
```

**11:45** — 清理旧数据（删除包含 user_id 的 series）：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 使用 promtool 或 API 删除高基数 series（Prometheus v2.30+ 支持）
kubectl exec -n monitoring prometheus-0 -- \
  wget -qO- --post-data='' 'localhost:9090/api/v1/admin/tsdb/delete_series?match[]=http_request_duration_seconds{user_id=~".+",}'

kubectl exec -n monitoring prometheus-0 -- \
  wget -qO- --post-data='' 'localhost:9090/api/v1/admin/tsdb/clean_tombstones'
```

**11:50** — 恢复 Prometheus 资源到正常水平：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch sts prometheus -n monitoring --type='merge' -p '
{
  "spec": {
    "template": {
      "spec": {
        "containers": [{
          "name": "prometheus",
          "resources": {
            "limits": {"cpu": "4", "memory": "16Gi"},
            "requests": {"cpu": "2", "memory": "8Gi"}
          }
        }]
      }
    }
  }
}'
```

## 验证
- 11:52 — Prometheus Pod Running，内存使用 4.2Gi
- 11:55 — TSDB headStats numSeries 恢复至 120 万
- 11:58 — Grafana 大盘恢复正常，Alertmanager 开始发送告警

## 复盘
- **直接原因**: metrics 添加 `user_id` 高基数标签 → TSDB 序列数 4500 万+ → Prometheus OOM
- **根本原因**: 开发团队不了解 Prometheus 高基数标签的危害，未经 SRE 评审直接上线
- **改进措施**:
  1. **高基数标签黑名单**: `user_id`、`request_id`、`email`、`phone` 等禁止作为 Prometheus label
  2. CI 检查：在 CI 中运行 `promtool check metrics`，检测基数 > 1000 的 label
  3. 添加 Prometheus 内存告警：`prometheus_tsdb_head_series > 5000000`
  4. 开发团队 metrics 培训：哪些维度适合作为 label，哪些不适合
  5. 使用 Thanos/Cortex/VictoriaMetrics 替代单节点 Prometheus，提高基数容忍度
- **相关 Skill**: [[monitor-kubernetes-metrics]]
- **相关 FTA**: [[monitoring-fta]]
