---
title: 可观测性故障排查指南 [topic-structural-trouble-shooting]
description: 'title: 可观测性故障排查指南'
summary: 'title: 可观测性故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- monitoring
- observability
- scheduler
- prometheus
- grafana
- jaeger
- docker
- redis
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 35min
intent_queries:
- 可观测性故障排查指南 是什么
- 如何 可观测性故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 可观测性故障排查指南 故障排查
- 可观测性故障排查指南 排障步骤
trigger_keywords:
- 可观测性故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- monitoring-basics
- ebpf-basics
- redis-basics
- logging-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 可观测性故障排查指南
description: '# 可观测性故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- scheduler
- [[Prometheus|prometheus]]
- grafana
- [[Jaeger|jaeger]]
- docker
- redis
- elasticsearch
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 可观测性故障排查指南 是什么
- 如何 可观测性故障排查指南
- 可观测性故障排查指南 故障排查
- 可观测性故障排查指南 排障步骤
trigger_keywords:
- 可观测性故障排查指南
- structural
- trouble
- shooting
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

# 可观测性故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 全栈可观测性保障

## 目录
1. [问题现象与影响分析](#问题现象与影响分析)
2. [排查方法与步骤](#排查方法与步骤)
3. [解决方案与风险控制](#解决方案与风险控制)

## 问题现象与影响分析

### 可观测性核心组件问题现象

| 问题类型 | 典型现象 | 影响程度 | 紧急级别 |
|---------|---------|---------|---------|
| Prometheus 数据采集失败 | `scrape failed` 持续出现 | ⭐⭐⭐ 高 | P0 |
| Grafana 仪表板无法加载 | `dashboard not found` 或空白页面 | ⭐⭐ 中 | P1 |
| Loki 日志查询超时 | `query timeout` 或 `context deadline exceeded` | ⭐⭐⭐ 高 | P0 |
| Jaeger 链路追踪不完整 | `trace not found` 或 spans 缺失 | ⭐⭐ 中 | P1 |
| AlertManager 告警风暴 | 大量重复告警或告警丢失 | ⭐⭐⭐ 高 | P0 |
| Metrics Server 不可用 | `metrics not available` 导致 HPA 失效 | ⭐⭐⭐ 高 | P0 |
| 监控数据存储爆满 | `disk full` 或 `retention exceeded` | ⭐⭐⭐ 高 | P0 |
| 多集群监控数据孤岛 | 跨集群指标无法聚合查询 | ⭐⭐ 中 | P1 |

## 排查方法与步骤

### 10 分钟快速诊断
1. **核心组件就绪**：Prometheus/Grafana/Loki/AlertManager Pod 是否 Running。
2. **采集状态**：Prometheus Targets 是否大量 down；ServiceMonitor/PodMonitor 是否匹配。
3. **存储压力**：TSDB/日志存储磁盘是否接近满水位。
4. **告警链路**：AlertManager 接收与路由是否正常。
5. **Metrics Server**：`kubectl top` 是否可用（影响 HPA）。
6. **快速缓解**：
   - 临时提升资源并缩短采样或保留周期。
   - 对关键告警先设置抑制避免风暴。
7. **证据留存**：保存 targets 状态、存储水位、告警路由与组件日志。

### 可观测性状态检查命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Prometheus 状态检查
echo "=== Prometheus 状态检查 ==="
kubectl get pods -n monitoring -l app=prometheus
kubectl get servicemonitors -A | wc -l
prometheus_url=$(kubectl get svc prometheus-k8s -n monitoring -o jsonpath='{.spec.clusterIP}:{.spec.ports[0].port}')
curl -s http://$prometheus_url/-/healthy && echo " ✓ Prometheus 健康" || echo " ✗ Prometheus 不健康"

# Grafana 状态检查
echo "=== Grafana 状态检查 ==="
kubectl get pods -n monitoring -l app=grafana
grafana_url=$(kubectl get svc grafana -n monitoring -o jsonpath='{.spec.clusterIP}:{.spec.ports[0].port}')
curl -s http://$grafana_url/api/health && echo " ✓ Grafana 健康" || echo " ✗ Grafana 不健康"

# Loki 状态检查
echo "=== Loki 状态检查 ==="
kubectl get pods -n logging -l app=loki
loki_url=$(kubectl get svc loki -n logging -o jsonpath='{.spec.clusterIP}:{.spec.ports[0].port}')
curl -s http://$loki_url/ready && echo " ✓ Loki 就绪" || echo " ✗ Loki 未就绪"

# Jaeger 状态检查
echo "=== Jaeger 状态检查 ==="
kubectl get pods -n tracing -l app=jaeger
jaeger_query_url=$(kubectl get svc jaeger-query -n tracing -o jsonpath='{.spec.clusterIP}:{.spec.ports[0].port}')
curl -s http://$jaeger_query_url/api/services && echo " ✓ Jaeger 查询服务正常" || echo " ✗ Jaeger 查询服务异常"

# AlertManager 状态检查
echo "=== AlertManager 状态检查 ==="
kubectl get pods -n monitoring -l app=alertmanager
alertmanager_url=$(kubectl get svc alertmanager-main -n monitoring -o jsonpath='{.spec.clusterIP}:{.spec.ports[0].port}')
curl -s http://$alertmanager_url/api/v2/status && echo " ✓ AlertManager 正常" || echo " ✗ AlertManager 异常"
```
### 诊断原理说明

可观测性故障诊断需要从数据流向的角度进行分析：

1. **数据采集层**：Exporter、ServiceMonitor、探针配置
2. **数据存储层**：Prometheus、Loki、Jaeger 存储状态
3. **数据查询层**：Grafana、API 查询性能、缓存机制
4. **告警处理层**：AlertManager 路由、抑制、静默规则
5. **可视化层**：仪表板配置、数据源连接、权限设置

### 可观测性问题诊断决策树

```
可观测性问题
    ├── 数据采集问题
    │   ├── Exporter 状态异常
    │   ├── ServiceMonitor 配置错误
    │   ├── 网络策略阻止采集
    │   └── 目标服务不可达
    ├── 数据存储问题
    │   ├── 存储空间不足
    │   ├── 数据保留策略不当
    │   ├── 存储性能瓶颈
    │   └── 数据损坏或丢失
    ├── 查询性能问题
    │   ├── 查询语句复杂度过高
    │   ├── 缓存命中率低
    │   ├── 并发查询限制
    │   └── 索引效率低下
    └── 告警管理问题
        ├── 告警规则配置错误
        ├── 路由配置不当
        ├── 抑制规则冲突
        └── 通知渠道失效
```

### 详细诊断命令

#### Prometheus 故障诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# Prometheus 故障诊断脚本

echo "=== Prometheus 故障诊断 ==="

# 1. Prometheus 基础状态检查
echo "1. Prometheus 基础状态检查:"
kubectl get pods -n monitoring -l app=prometheus -o wide

# 检查 Prometheus 状态端点
PROMETHEUS_POD=$(kubectl get pods -n monitoring -l app=prometheus -o name | head -1)
echo "Prometheus 状态检查:"
kubectl exec -n monitoring $PROMETHEUS_POD -- wget -qO- http://localhost:9090/-/healthy && echo "✓ Healthy" || echo "✗ Unhealthy"
kubectl exec -n monitoring $PROMETHEUS_POD -- wget -qO- http://localhost:9090/-/ready && echo "✓ Ready" || echo "✗ Not Ready"

# 2. 目标抓取状态检查
echo "2. 目标抓取状态检查:"
TARGETS_DATA=$(kubectl exec -n monitoring $PROMETHEUS_POD -- wget -qO- http://localhost:9090/api/v1/targets)
echo "抓取目标统计:"
echo "$TARGETS_DATA" | jq -r '.data.activeTargets | group_by(.health) | map({health: .[0].health, count: length})[] | "\(.health): \(.count)"'

# 检查不健康的抓取目标
UNHEALTHY_TARGETS=$(echo "$TARGETS_DATA" | jq -r '.data.activeTargets[] | select(.health != "up") | "\(.scrapeUrl): \(.lastError)"' | head -10)
if [ -n "$UNHEALTHY_TARGETS" ]; then
  echo "不健康的抓取目标:"
  echo "$UNHEALTHY_TARGETS"
fi

# 3. 规则评估状态检查
echo "3. 规则评估状态检查:"
RULES_DATA=$(kubectl exec -n monitoring $PROMETHEUS_POD -- wget -qO- http://localhost:9090/api/v1/rules)
echo "告警规则状态:"
echo "$RULES_DATA" | jq -r '.data.groups[] | "\(.name): \(.rules | length) rules, \(.rules | map(select(.state == "firing")) | length) firing"'

# 4. 存储和性能检查
echo "4. 存储和性能检查:"
STORAGE_STATS=$(kubectl exec -n monitoring $PROMETHEUS_POD -- wget -qO- http://localhost:9090/api/v1/status/tsdb)
echo "TSDB 存储统计:"
echo "$STORAGE_STATS" | jq -r '
  "系列数量: \(.data.headStats.numSeries)",
  "区块数量: \(.data.blockStats.numBlocks)",
  "采样率: \(.data.headStats.chunks / .data.headStats.samples * 100)%"
'

# 检查存储使用情况
echo "存储使用情况:"
kubectl exec -n monitoring $PROMETHEUS_POD -- df -h /prometheus

# 5. 配置检查
echo "5. 配置检查:"
CONFIG_DATA=$(kubectl exec -n monitoring $PROMETHEUS_POD -- wget -qO- http://localhost:9090/api/v1/status/config)
GLOBAL_CONFIG=$(echo "$CONFIG_DATA" | jq -r '.data.yaml' | yq '.global')
echo "全局配置摘要:"
echo "$GLOBAL_CONFIG"

# 6. 性能指标检查
echo "6. 性能指标检查:"
PERFORMANCE_METRICS=$(kubectl exec -n monitoring $PROMETHEUS_POD -- wget -qO- http://localhost:9090/metrics | grep -E "(prometheus_tsdb_head_series|prometheus_target_scrapes_sample_out_of_bounds_total|prometheus_rule_evaluation_failures_total)")
echo "$PERFORMANCE_METRICS"
```
#### Grafana 故障诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# Grafana 故障诊断脚本

echo "=== Grafana 故障诊断 ==="

# 1. Grafana 基础状态检查
echo "1. Grafana 基础状态检查:"
kubectl get pods -n monitoring -l app=grafana -o wide

GRAFANA_POD=$(kubectl get pods -n monitoring -l app=grafana -o name | head -1)

# 2. 数据源连接检查
echo "2. 数据源连接检查:"
DATASOURCES=$(kubectl exec -n monitoring $GRAFANA_POD -- curl -s http://admin:admin@localhost:3000/api/datasources)
echo "数据源列表:"
echo "$DATASOURCES" | jq -r '.[] | "\(.name) (\(.type)): \(.url) - \(.jsonData.httpMethod // "GET")"'

# 检查数据源健康状态
echo "数据源健康检查:"
for datasource in $(echo "$DATASOURCES" | jq -r '.[].id'); do
  HEALTH_STATUS=$(kubectl exec -n monitoring $GRAFANA_POD -- curl -s -X GET http://admin:admin@localhost:3000/api/datasources/$datasource/health)
  DS_NAME=$(echo "$DATASOURCES" | jq -r ".[] | select(.id==$datasource) | .name")
  echo "  $DS_NAME: $(echo "$HEALTH_STATUS" | jq -r '.message')"
done

# 3. 仪表板状态检查
echo "3. 仪表板状态检查:"
DASHBOARDS=$(kubectl exec -n monitoring $GRAFANA_POD -- curl -s http://admin:admin@localhost:3000/api/search)
echo "仪表板统计:"
echo "$DASHBOARDS" | jq -r 'group_by(.type) | map({type: .[0].type, count: length})[] | "\(.type): \(.count)"'

# 检查有问题的仪表板
PROBLEM_DASHBOARDS=$(echo "$DASHBOARDS" | jq -r '.[] | select(.type == "dash-db") | select(.folderTitle == null or .folderTitle == "") | .title')
if [ -n "$PROBLEM_DASHBOARDS" ]; then
  echo "未分类的仪表板:"
  echo "$PROBLEM_DASHBOARDS"
fi

# 4. 用户和权限检查
echo "4. 用户和权限检查:"
USERS=$(kubectl exec -n monitoring $GRAFANA_POD -- curl -s http://admin:admin@localhost:3000/api/users)
echo "用户统计:"
echo "$USERS" | jq -r 'length as $total | "总用户数: \($total)"'

ADMIN_USERS=$(echo "$USERS" | jq -r '[.[] | select(.isAdmin == true)] | length')
echo "管理员用户数: $ADMIN_USERS"

# 5. 插件状态检查
echo "5. 插件状态检查:"
PLUGINS=$(kubectl exec -n monitoring $GRAFANA_POD -- curl -s http://admin:admin@localhost:3000/api/plugins)
ENABLED_PLUGINS=$(echo "$PLUGINS" | jq -r '[.[] | select(.enabled == true)] | length')
TOTAL_PLUGINS=$(echo "$PLUGINS" | jq -r 'length')
echo "插件状态: $ENABLED_PLUGINS/$TOTAL_PLUGINS 已启用"

# 6. 性能和日志检查
echo "6. 性能和日志检查:"
echo "Grafana 日志摘要 (最近50行):"
kubectl logs -n monitoring $GRAFANA_POD --tail=50 | grep -i -E "(error|warning|failed)" | tail -10

# 检查内存使用
echo "内存使用情况:"
kubectl top pod -n monitoring $GRAFANA_POD
```
#### Loki 故障诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# Loki 故障诊断脚本

echo "=== Loki 故障诊断 ==="

# 1. Loki 基础状态检查
echo "1. Loki 基础状态检查:"
kubectl get pods -n logging -l app=loki -o wide

LOKI_POD=$(kubectl get pods -n logging -l app=loki -o name | head -1)

# 2. Loki 构建器和读取器状态
echo "2. Loki 组件状态检查:"
kubectl exec -n logging $LOKI_POD -- wget -qO- http://localhost:3100/ready && echo "✓ Loki 就绪" || echo "✗ Loki 未就绪"
kubectl exec -n logging $LOKI_POD -- wget -qO- http://localhost:3100/metrics | grep -E "(loki_ingester|loki_querier)" | head -10

# 3. 日志摄入检查
echo "3. 日志摄入检查:"
INGESTER_METRICS=$(kubectl exec -n logging $LOKI_POD -- wget -qO- http://localhost:3100/metrics | grep loki_ingester)
echo "摄入指标:"
echo "$INGESTER_METRICS" | grep -E "(lines_received_total|bytes_received_total)" | head -5

# 检查摄入错误
INGEST_ERRORS=$(echo "$INGESTER_METRICS" | grep "loki_ingester_chunks_flush_failed_total" | awk '{print $2}')
if [ "$INGEST_ERRORS" != "0" ]; then
  echo "⚠ 发现摄入错误: $INGEST_ERRORS"
fi

# 4. 查询性能检查
echo "4. 查询性能检查:"
QUERY_METRICS=$(kubectl exec -n logging $LOKI_POD -- wget -qO- http://localhost:3100/metrics | grep loki_querier)
echo "查询指标:"
echo "$QUERY_METRICS" | grep -E "(query_duration_seconds|queried_streams)" | head -5

# 检查查询错误
QUERY_ERRORS=$(echo "$QUERY_METRICS" | grep "loki_querier_query_frontend_errors_total" | awk '{print $2}')
if [ "$QUERY_ERRORS" != "0" ]; then
  echo "⚠ 发现查询错误: $QUERY_ERRORS"
fi

# 5. 存储状态检查
echo "5. 存储状态检查:"
STORAGE_METRICS=$(kubectl exec -n logging $LOKI_POD -- wget -qO- http://localhost:3100/metrics | grep loki_storage)
echo "存储指标:"
echo "$STORAGE_METRICS" | head -10

# 检查存储使用情况
echo "存储使用情况:"
kubectl exec -n logging $LOKI_POD -- df -h /var/loki

# 6. 配置检查
echo "6. 配置检查:"
CONFIG=$(kubectl exec -n logging $LOKI_POD -- cat /etc/loki/loki.yaml)
echo "保留策略:"
echo "$CONFIG" | grep -A5 retention_period

echo "存储配置:"
echo "$CONFIG" | grep -A10 storage_config

# 7. 日志流检查
echo "7. 日志流检查:"
# 检查最近的日志流
STREAMS=$(kubectl exec -n logging $LOKI_POD -- wget -qO- "http://localhost:3100/loki/api/v1/series?match[]={job!=\"\"}&start=$(date -d '1 hour ago' +%s)000000000&end=$(date +%s)000000000")
echo "最近1小时内的日志流数量:"
echo "$STREAMS" | jq -r '.data | length'
```
#### Jaeger 故障诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# Jaeger 故障诊断脚本

echo "=== Jaeger 故障诊断 ==="

# 1. Jaeger 组件状态检查
echo "1. Jaeger 组件状态检查:"
kubectl get pods -n tracing -l app=jaeger -o wide

JAEGER_QUERY_POD=$(kubectl get pods -n tracing -l app=jaeger-component,component=query -o name | head -1)
JAEGER_COLLECTOR_POD=$(kubectl get pods -n tracing -l app=jaeger-component,component=collector -o name | head -1)

# 2. 服务发现检查
echo "2. 服务发现检查:"
SERVICES=$(kubectl exec -n tracing $JAEGER_QUERY_POD -- wget -qO- http://localhost:16686/api/services)
SERVICE_COUNT=$(echo "$SERVICES" | jq -r '.data | length')
echo "发现的服务数量: $SERVICE_COUNT"

if [ $SERVICE_COUNT -lt 5 ]; then
  echo "⚠ 服务数量较少，可能存在问题"
  echo "服务列表:"
  echo "$SERVICES" | jq -r '.data[]'
fi

# 3. 追踪数据检查
echo "3. 追踪数据检查:"
# 检查最近1小时的追踪数据
RECENT_TRACES=$(kubectl exec -n tracing $JAEGER_QUERY_POD -- wget -qO- "http://localhost:16686/api/traces?service=jaeger-query&lookback=1h&limit=10")
TRACE_COUNT=$(echo "$RECENT_TRACES" | jq -r '.data | length')
echo "最近1小时追踪数量: $TRACE_COUNT"

# 4. Collector 状态检查
echo "4. Collector 状态检查:"
COLLECTOR_METRICS=$(kubectl exec -n tracing $JAEGER_COLLECTOR_POD -- wget -qO- http://localhost:14269/metrics)
echo "Collector 指标摘要:"
echo "$COLLECTOR_METRICS" | grep -E "(spans_received|batch_size|save_latency)" | head -5

# 检查 Collector 错误
COLLECTOR_ERRORS=$(echo "$COLLECTOR_METRICS" | grep " spans_dropped_total " | awk '{print $2}')
if [ "$COLLECTOR_ERRORS" != "0" ]; then
  echo "⚠ Collector 丢弃的 spans 数量: $COLLECTOR_ERRORS"
fi

# 5. 存储后端检查
echo "5. 存储后端检查:"
# 如果使用 Elasticsearch
if kubectl get pods -n tracing -l app=elasticsearch &>/dev/null; then
  ES_POD=$(kubectl get pods -n tracing -l app=elasticsearch -o name | head -1)
  ES_HEALTH=$(kubectl exec -n tracing $ES_POD -- curl -s http://localhost:9200/_cluster/health)
  echo "Elasticsearch 集群状态:"
  echo "$ES_HEALTH" | jq -r '"状态: \(.status), 节点数: \(.number_of_nodes), 分片数: \(.active_shards)"'
fi

# 6. 采样配置检查
echo "6. 采样配置检查:"
SAMPLING_CONFIG=$(kubectl get configmap jaeger-sampling-strategies -n tracing -o yaml 2>/dev/null)
if [ -n "$SAMPLING_CONFIG" ]; then
  echo "采样策略配置:"
  echo "$SAMPLING_CONFIG" | grep -A20 "default_strategy"
else
  echo "未找到采样配置"
fi
```
## 解决方案与风险控制

### Prometheus 问题解决

#### 方案：Prometheus 配置优化

```yaml
# Prometheus 优化配置
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: prometheus
  namespace: monitoring
spec:
  replicas: 2
  retention: 30d
  retentionSize: "50GB"
  ruleSelector:
    matchLabels:
      prometheus: prometheus
  serviceAccountName: prometheus-k8s
  serviceMonitorSelector:
    matchExpressions:
    - key: prometheus
      operator: In
      values:
      - prometheus
  resources:
    requests:
      memory: "2Gi"
      cpu: "1"
    limits:
      memory: "8Gi"
      cpu: "2"
  storage:
    volumeClaimTemplate:
      spec:
        storageClassName: fast-ssd
        resources:
          requests:
            storage: 100Gi
  # 性能优化配置
  enableAdminAPI: false
  evaluationInterval: 30s
  scrapeInterval: 30s
  scrapeTimeout: 10s
  externalLabels:
    cluster: production
  remoteWrite:
  - url: http://thanos-receive:19291/api/v1/receive
    writeRelabelConfigs:
    - sourceLabels: [__name__]
      regex: '(up|scrape_samples_scraped)'
      action: drop
```

#### 方案：告警规则优化

```yaml
# Prometheus 告警规则优化
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: prometheus-rules
  namespace: monitoring
spec:
  groups:
  - name: prometheus.rules
    rules:
    # 告警抑制规则
    - alert: PrometheusTargetMissing
      expr: up == 0
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Prometheus 目标不可达"
        description: "{{ $labels.instance }} 目标在5分钟内持续不可达"
        
    - alert: PrometheusScrapeFailed
      expr: rate(prometheus_target_scrapes_sample_out_of_bounds_total[5m]) > 0.01
      for: 10m
      labels:
        severity: critical
      annotations:
        summary: "Prometheus 抓取失败率过高"
        description: "抓取失败率超过1%，可能影响监控数据完整性"
        
    - alert: PrometheusStorageFull
      expr: (node_filesystem_avail_bytes{mountpoint="/prometheus"} / node_filesystem_size_bytes{mountpoint="/prometheus"}) * 100 < 10
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Prometheus 存储空间不足"
        description: "Prometheus 存储剩余空间小于10%"
        
    # 抑制规则示例
    - alert: HighMemoryUsage
      expr: (container_memory_usage_bytes / container_spec_memory_limit_bytes * 100) > 90
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "容器内存使用率过高"
        
    - alert: CriticalMemoryUsage
      expr: (container_memory_usage_bytes / container_spec_memory_limit_bytes * 100) > 95
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "容器内存使用率达到临界值"
```

### Grafana 问题解决

#### 方案：Grafana 性能优化配置

```yaml
# Grafana 性能优化配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: grafana-config
  namespace: monitoring
data:
  grafana.ini: |
    [server]
    domain = grafana.example.com
    root_url = %(protocol)s://%(domain)s:%(http_port)s
    
    [database]
    type = postgres
    host = postgres.monitoring:5432
    name = grafana
    user = grafana
    ssl_mode = disable
    
    [remote_cache]
    type = redis
    connstr = addr=redis.monitoring:6379,pool_size=100,db=0,ssl=false
    
    [dataproxy]
    timeout = 30
    keep_alive_seconds = 300
    send_user_header = false
    
    [panels]
    disable_sanitize_html = false
    
    [plugins]
    enable_alpha = false
    app_tls_skip_verify_insecure = false
    
    [rendering]
    server_url = http://grafana-renderer:8081/render
    callback_url = http://grafana:3000/
    
    [analytics]
    reporting_enabled = false
    check_for_updates = false
    
    [log]
    mode = console
    level = info
    
    [auth.anonymous]
    enabled = false
    
    [users]
    allow_sign_up = false
    auto_assign_org = true
    auto_assign_org_role = Viewer

---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: grafana
  namespace: monitoring
spec:
  replicas: 2
  selector:
    matchLabels:
      app: grafana
  template:
    metadata:
      labels:
        app: grafana
    spec:
      containers:
      - name: grafana
        image: grafana/grafana:10.2.2
        ports:
        - containerPort: 3000
        env:
        - name: GF_SECURITY_ADMIN_PASSWORD
          valueFrom:
            secretKeyRef:
              name: grafana-admin-credentials
              key: admin-password
        - name: GF_DATABASE_TYPE
          value: postgres
        - name: GF_DATABASE_HOST
          value: postgres.monitoring:5432
        - name: GF_DATABASE_NAME
          value: grafana
        - name: GF_DATABASE_USER
          value: grafana
        - name: GF_DATABASE_PASSWORD
          valueFrom:
            secretKeyRef:
              name: grafana-db-credentials
              key: password
        resources:
          requests:
            memory: "256Mi"
            cpu: "100m"
          limits:
            memory: "1Gi"
            cpu: "500m"
        volumeMounts:
        - name: config
          mountPath: /etc/grafana/grafana.ini
          subPath: grafana.ini
        - name: storage
          mountPath: /var/lib/grafana
      volumes:
      - name: config
        configMap:
          name: grafana-config
      - name: storage
        persistentVolumeClaim:
          claimName: grafana-storage
```

#### 方案：仪表板优化配置

```json
{
  "dashboard": {
    "id": null,
    "title": "Kubernetes Cluster Overview",
    "timezone": "browser",
    "schemaVersion": 38,
    "version": 1,
    "refresh": "30s",
    "templating": {
      "list": [
        {
          "name": "datasource",
          "type": "datasource",
          "pluginId": "prometheus"
        },
        {
          "name": "cluster",
          "type": "query",
          "datasource": "${datasource}",
          "refresh": 1,
          "query": "label_values(kube_node_info, cluster)"
        }
      ]
    },
    "panels": [
      {
        "type": "graph",
        "title": "Cluster CPU Usage",
        "gridPos": {
          "h": 8,
          "w": 12,
          "x": 0,
          "y": 0
        },
        "targets": [
          {
            "expr": "sum(rate(container_cpu_usage_seconds_total{cluster=\"$cluster\", namespace!=\"\"}[5m])) by (namespace)",
            "legendFormat": "{{namespace}}",
            "refId": "A"
          }
        ],
        "options": {
          "tooltip": {
            "mode": "multi",
            "sort": "desc"
          }
        }
      },
      {
        "type": "stat",
        "title": "Node Status",
        "gridPos": {
          "h": 4,
          "w": 6,
          "x": 12,
          "y": 0
        },
        "targets": [
          {
            "expr": "count(kube_node_status_condition{condition=\"Ready\", status=\"true\", cluster=\"$cluster\"})",
            "refId": "A"
          }
        ],
        "options": {
          "reduceOptions": {
            "calcs": ["lastNotNull"]
          }
        }
      }
    ]
  }
}
```

### Loki 问题解决

#### 方案：Loki 存储和性能优化

```yaml
# Loki 优化配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: loki-config
  namespace: logging
data:
  loki.yaml: |
    auth_enabled: false
    
    server:
      http_listen_port: 3100
      grpc_listen_port: 9096
    
    common:
      path_prefix: /var/loki
      storage:
        filesystem:
          chunks_directory: /var/loki/chunks
          rules_directory: /var/loki/rules
      replication_factor: 1
      ring:
        kvstore:
          store: inmemory
    
    schema_config:
      configs:
        - from: 2020-05-15
          store: boltdb-shipper
          object_store: filesystem
          schema: v11
          index:
            prefix: index_
            period: 24h
    
    storage_config:
      boltdb_shipper:
        active_index_directory: /var/loki/index
        cache_location: /var/loki/cache
        cache_ttl: 24h
        shared_store: filesystem
      filesystem:
        directory: /var/loki/chunks
    
    chunk_store_config:
      max_look_back_period: 0s
      chunk_cache_config:
        embedded_cache:
          enabled: true
          max_size_mb: 100
    
    table_manager:
      retention_deletes_enabled: true
      retention_period: 168h  # 7天
    
    limits_config:
      ingestion_rate_mb: 10
      ingestion_burst_size_mb: 20
      max_entries_limit_per_query: 10000
      max_streams_matchers_per_query: 1000
      max_concurrent_tail_requests: 10
      split_queries_by_interval: 15m
    
    query_scheduler:
      max_outstanding_requests_per_tenant: 2048
    
    frontend:
      max_outstanding_per_tenant: 2048
      compress_responses: true
      tail_proxy_url: http://loki-canary:3100
    
    query_range:
      split_queries_by_interval: 15m
      parallelise_shardable_queries: true

---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: loki
  namespace: logging
spec:
  serviceName: loki-headless
  replicas: 1
  selector:
    matchLabels:
      app: loki
  template:
    metadata:
      labels:
        app: loki
    spec:
      containers:
      - name: loki
        image: grafana/loki:2.9.2
        args:
        - "-config.file=/etc/loki/loki.yaml"
        ports:
        - name: http-metrics
          containerPort: 3100
        - name: grpc
          containerPort: 9096
        readinessProbe:
          httpGet:
            path: /ready
            port: http-metrics
          initialDelaySeconds: 45
          timeoutSeconds: 1
        livenessProbe:
          httpGet:
            path: /ready
            port: http-metrics
          initialDelaySeconds: 45
          timeoutSeconds: 1
        resources:
          requests:
            cpu: "100m"
            memory: "256Mi"
          limits:
            cpu: "2"
            memory: "2Gi"
        volumeMounts:
        - name: config
          mountPath: /etc/loki
        - name: storage
          mountPath: /var/loki
      volumes:
      - name: config
        configMap:
          name: loki-config
  volumeClaimTemplates:
  - metadata:
      name: storage
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 50Gi
```

#### 方案：日志收集优化配置

```yaml
# Promtail 优化配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: promtail-config
  namespace: logging
data:
  promtail.yaml: |
    server:
      http_listen_port: 9080
      grpc_listen_port: 0
    
    clients:
      - url: http://loki-headless.logging.svc.cluster.local:3100/loki/api/v1/push
    
    positions:
      filename: /var/log/promtail/positions.yaml
    
    scrape_configs:
      - job_name: kubernetes-pods-name
        kubernetes_sd_configs:
          - role: pod
        relabel_configs:
          - source_labels:
              - __meta_kubernetes_pod_annotation_promtail_io_scrape
            action: keep
            regex: true
          - source_labels:
              - __meta_kubernetes_pod_label_app
            target_label: app
          - source_labels:
              - __meta_kubernetes_namespace
            target_label: namespace
          - source_labels:
              - __meta_kubernetes_pod_name
            target_label: pod
          - source_labels:
              - __meta_kubernetes_pod_container_name
            target_label: container
          - replacement: /var/log/pods/*$1/*.log
            separator: /
            source_labels:
              - __meta_kubernetes_pod_uid
              - __meta_kubernetes_pod_container_name
            target_label: __path__
    
    limits_config:
      max_streams: 100000
      max_line_size: 1048576
      max_entries_per_query: 10000
    
    tracing:
      enabled: true

---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: promtail
  namespace: logging
spec:
  selector:
    matchLabels:
      app: promtail
  template:
    metadata:
      labels:
        app: promtail
    spec:
      serviceAccountName: promtail
      containers:
      - name: promtail
        image: grafana/promtail:2.9.2
        args:
        - "-config.file=/etc/promtail/promtail.yaml"
        env:
        - name: HOSTNAME
          valueFrom:
            fieldRef:
              fieldPath: spec.nodeName
        volumeMounts:
        - name: config
          mountPath: /etc/promtail
        - name: varlog
          mountPath: /var/log
          readOnly: true
        - name: varlibdockercontainers
          mountPath: /var/lib/docker/containers
          readOnly: true
        - name: runlogjournal
          mountPath: /run/log/journal
          readOnly: true
        - name: positions
          mountPath: /var/log/promtail
        resources:
          requests:
            memory: "128Mi"
            cpu: "100m"
          limits:
            memory: "512Mi"
            cpu: "500m"
      volumes:
      - name: config
        configMap:
          name: promtail-config
      - name: varlog
        hostPath:
          path: /var/log
      - name: varlibdockercontainers
        hostPath:
          path: /var/lib/docker/containers
      - name: runlogjournal
        hostPath:
          path: /run/log/journal
      - name: positions
        hostPath:
          path: /var/log/promtail
```

### Jaeger 问题解决

#### 方案：Jaeger 高可用配置

```yaml
# Jaeger 高可用配置
apiVersion: jaegertracing.io/v1
kind: Jaeger
metadata:
  name: jaeger-prod
  namespace: tracing
spec:
  strategy: production
  collector:
    replicas: 3
    image: jaegertracing/jaeger-collector:1.52
    options:
      collector.queue-size: 2000
      collector.num-workers: 50
      es.server-urls: http://elasticsearch.tracing:9200
      es.username: elastic
      es.password: changeme
      sampling.strategies-file: /etc/jaeger/sampling/sampling.json
    volumeMounts:
    - name: sampling-config
      mountPath: /etc/jaeger/sampling
    resources:
      requests:
        memory: "512Mi"
        cpu: "250m"
      limits:
        memory: "2Gi"
        cpu: "1"
  query:
    replicas: 2
    image: jaegertracing/jaeger-query:1.52
    options:
      query.base-path: /jaeger
      es.server-urls: http://elasticsearch.tracing:9200
      es.username: elastic
      es.password: changeme
    resources:
      requests:
        memory: "256Mi"
        cpu: "100m"
      limits:
        memory: "1Gi"
        cpu: "500m"
  agent:
    strategy: DaemonSet
    image: jaegertracing/jaeger-agent:1.52
    options:
      processor.jaeger-binary.server-host-port: :6832
      processor.jaeger-compact.server-host-port: :6831
      processor.jaeger-thrift.server-host-port: :5775
      reporter.grpc.host-port: jaeger-collector-headless.tracing:14250
    resources:
      requests:
        memory: "64Mi"
        cpu: "50m"
      limits:
        memory: "128Mi"
        cpu: "200m"
  storage:
    type: elasticsearch
    options:
      es:
        server-urls: http://elasticsearch.tracing:9200
        username: elastic
        password: changeme
        use-aliases: true
        create-index-templates: true
  ingress:
    enabled: true
    hosts:
    - jaeger.example.com
    tls:
    - hosts:
      - jaeger.example.com
      secretName: jaeger-tls
  volumeClaimTemplates:
  - metadata:
      name: sampling-config
    spec:
      accessModes: ["ReadOnlyMany"]
      resources:
        requests:
          storage: 1Mi
```

#### 方案：采样策略配置

```json
{
  "service_strategies": [
    {
      "service": "frontend",
      "type": "probabilistic",
      "param": 0.8,
      "operation_strategies": [
        {
          "operation": "health-check",
          "type": "probabilistic",
          "param": 0.0
        },
        {
          "operation": "/api/login",
          "type": "probabilistic",
          "param": 1.0
        }
      ]
    },
    {
      "service": "backend",
      "type": "ratelimiting",
      "param": 10.0
    }
  ],
  "default_strategy": {
    "type": "probabilistic",
    "param": 0.1,
    "operation_strategies": [
      {
        "operation": "health",
        "type": "probabilistic",
        "param": 0.0
      }
    ]
  }
}
```

### 执行风险评估

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| Prometheus 配置调整 | ⭐⭐ 中 | 可能影响监控数据采集 | 恢复原配置文件 |
| Grafana 数据源变更 | ⭐⭐ 中 | 可能影响仪表板显示 | 恢复原数据源配置 |
| Loki 存储策略调整 | ⭐⭐⭐ 高 | 可能影响日志数据完整性 | 谨慎测试后应用 |
| Jaeger 采样策略修改 | ⭐⭐ 中 | 可能影响追踪数据覆盖率 | 逐步调整采样率 |

### 验证与监控

### 可观测性验证脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 可观测性验证脚本

echo "=== 可观测性验证 ==="

# 1. Prometheus 验证
echo "1. Prometheus 验证:"
if kubectl get crd prometheuses.monitoring.coreos.com &>/dev/null; then
  PROMETHEUS_HEALTH=$(kubectl get pods -n monitoring -l app=prometheus -o jsonpath='{.items[*].status.containerStatuses[*].ready}' | tr ' ' '\n' | grep -c true)
  TOTAL_PROMETHEUS=$(kubectl get pods -n monitoring -l app=prometheus --no-headers | wc -l)
  echo "Prometheus 健康状态: $PROMETHEUS_HEALTH/$TOTAL_PROMETHEUS"
  
  if [ $PROMETHEUS_HEALTH -ne $TOTAL_PROMETHEUS ]; then
    echo "不健康的 Prometheus 实例:"
    kubectl get pods -n monitoring -l app=prometheus | grep -v "Running"
  fi
else
  echo "Prometheus 未部署"
fi

# 2. Grafana 验证
echo "2. Grafana 验证:"
if kubectl get deploy grafana -n monitoring &>/dev/null; then
  GRAFANA_READY=$(kubectl get deploy grafana -n monitoring -o jsonpath='{.status.readyReplicas}')
  GRAFANA_TOTAL=$(kubectl get deploy grafana -n monitoring -o jsonpath='{.status.replicas}')
  echo "Grafana 就绪状态: $GRAFANA_READY/$GRAFANA_TOTAL"
else
  echo "Grafana 未部署"
fi

# 3. Loki 验证
echo "3. Loki 验证:"
if kubectl get statefulset loki -n logging &>/dev/null; then
  LOKI_READY=$(kubectl get statefulset loki -n logging -o jsonpath='{.status.readyReplicas}')
  LOKI_TOTAL=$(kubectl get statefulset loki -n logging -o jsonpath='{.status.replicas}')
  echo "Loki 就绪状态: $LOKI_READY/$LOKI_TOTAL"
  
  # 测试日志摄入
  TEST_LOG="{\"timestamp\":\"$(date -Iseconds)\",\"level\":\"info\",\"message\":\"observability test\"}"
  LOKI_URL=$(kubectl get svc loki -n logging -o jsonpath='{.spec.clusterIP}:{.spec.ports[0].port}')
  curl -s -X POST "http://$LOKI_URL/loki/api/v1/push" \
    -H "Content-Type: application/json" \
    -d "{\"streams\":[{\"stream\":{\"job\":\"test\"},\"values\":\"$(date +%s)000000000\",\"$TEST_LOG\"}]}" >/dev/null 2>&1 && echo "✓ Loki 日志摄入测试通过" || echo "✗ Loki 日志摄入测试失败"
else
  echo "Loki 未部署"
fi

# 4. Jaeger 验证
echo "4. Jaeger 验证:"
if kubectl get jaeger jaeger-prod -n tracing &>/dev/null; then
  JAEGER_READY=$(kubectl get pods -n tracing -l app=jaeger -o jsonpath='{.items[*].status.containerStatuses[*].ready}' | tr ' ' '\n' | grep -c true)
  JAEGER_TOTAL=$(kubectl get pods -n tracing -l app=jaeger --no-headers | wc -l)
  echo "Jaeger 就绪状态: $JAEGER_READY/$JAEGER_TOTAL"
  
  # 测试追踪数据
  JAEGER_QUERY_URL=$(kubectl get svc jaeger-query -n tracing -o jsonpath='{.spec.clusterIP}:{.spec.ports[0].port}')
  curl -s "http://$JAEGER_QUERY_URL/api/services" | jq -r '.data | length' >/dev/null 2>&1 && echo "✓ Jaeger 服务发现正常" || echo "✗ Jaeger 服务发现问题"
else
  echo "Jaeger 未部署"
fi

echo "可观测性验证完成！"
```
### 可观测性监控告警配置

```yaml
# Prometheus 可观测性监控告警
groups:
- name: observability
  rules:
  - alert: PrometheusDown
    expr: absent(up{job="prometheus-k8s"})
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Prometheus 实例宕机"
      description: "Prometheus 监控实例不可用"

  - alert: GrafanaDown
    expr: absent(up{job="grafana"})
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Grafana 实例宕机"
      description: "Grafana 可视化实例不可用"

  - alert: LokiDown
    expr: absent(up{job="loki"})
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Loki 实例宕机"
      description: "Loki 日志收集实例不可用"

  - alert: JaegerCollectorDown
    expr: absent(up{job="jaeger-collector"})
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Jaeger Collector 宕机"
      description: "Jaeger 追踪收集器不可用"

  - alert: PrometheusStorageLow
    expr: (node_filesystem_free_bytes{mountpoint="/prometheus"} / node_filesystem_size_bytes{mountpoint="/prometheus"}) * 100 < 15
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Prometheus 存储空间不足"
      description: "Prometheus 存储剩余空间小于15%"

  - alert: LokiHighMemoryUsage
    expr: (container_memory_usage_bytes{container="loki"} / container_spec_memory_limit_bytes{container="loki"}) * 100 > 80
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Loki 内存使用率过高"
      description: "Loki 内存使用率超过80%"

  - alert: GrafanaHighCPUUsage
    expr: rate(container_cpu_usage_seconds_total{container="grafana"}[5m]) > 0.8
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Grafana CPU 使用率过高"
      description: "Grafana CPU 使用率持续超过80%"

  - alert: JaegerTraceLoss
    expr: increase(jaeger_collector_spans_dropped_total[5m]) > 100
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Jaeger 追踪数据丢失"
      description: "Jaeger Collector 丢弃的 spans 数量异常增加"
```

### 最佳实践

### 可观测性架构设计

```yaml
# 可观测性架构最佳实践
observabilityArchitecture:
  monitoring:
    prometheus:
      deployment: "HA with Thanos"
      retention: "90d"
      storage: "Remote write to object storage"
      federation: "Hierarchical federation for multi-cluster"
    
    alerting:
      alertmanager:
        replicas: 3
        clustering: enabled
        receivers:
          - name: "pagerduty"
            pagerduty_configs:
            - routing_key: "<pagerduty-key>"
          - name: "slack"
            slack_configs:
            - channel: "#alerts"
              send_resolved: true
    
    visualization:
      grafana:
        replicas: 2
        persistence: enabled
        authentication: "OAuth with OIDC"
        datasources:
          - name: "Prometheus"
            type: "prometheus"
            url: "http://thanos-query:9090"
          - name: "Loki"
            type: "loki"
            url: "http://loki-gateway:80"
          - name: "Jaeger"
            type: "jaeger"
            url: "http://jaeger-query:16686"
  
  logging:
    collection:
      promtail:
        deployment: "DaemonSet"
        scrapeConfigs:
          - job_name: "kubernetes-pods"
            kubernetes_sd_configs:
            - role: pod
            relabel_configs:
            - source_labels: ['__meta_kubernetes_pod_annotation_promtail_io_scrape']
              action: keep
              regex: true
    
    storage:
      loki:
        deployment: "Single binary mode"
        retention: "30d"
        compression: "snappy"
        indexCache: "in-memory"
    
    processing:
      logql:
        optimization:
          - query_splitting: "enabled"
          - parallelization: "enabled"
          - caching: "enabled"
  
  tracing:
    collection:
      jaeger:
        agent: "DaemonSet"
        collector: "Deployment with HPA"
        sampling:
          strategies: "adaptive"
          default_ratio: 0.1
    
    storage:
      backend: "Elasticsearch"
      retention: "7d"
      indices:
        rollover: "daily"
        lifecycle: "ILM policy"
    
    analysis:
      hotrod: "demo application for tracing"
      traceAnalytics: "enabled in Kibana"
```

### 可观测性数据质量管理

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# 可观测性数据质量管理脚本

QUALITY_REPORT="/var/log/kubernetes/observability-quality-$(date +%Y%m%d).log"

{
  echo "=== 可观测性数据质量报告 $(date) ==="
  
  # 1. 监控数据质量检查
  echo "1. 监控数据质量检查:"
  
  # 检查指标完整性
  METRIC_COMPLETENESS=$(kubectl exec -n monitoring prometheus-k8s-0 -- wget -qO- http://localhost:9090/api/v1/query?query=count(up) | jq -r '.data.result[0].value[1]')
  TOTAL_TARGETS=$(kubectl exec -n monitoring prometheus-k8s-0 -- wget -qO- http://localhost:9090/api/v1/targets | jq -r '.data.activeTargets | length')
  COMPLETENESS_RATE=$((METRIC_COMPLETENESS * 100 / TOTAL_TARGETS))
  echo "监控指标完整性: ${COMPLETENESS_RATE}%"
  
  # 2. 日志数据质量检查
  echo "2. 日志数据质量检查:"
  
  # 检查日志摄入速率
  LOG_INGESTION_RATE=$(kubectl exec -n logging loki-0 -- wget -qO- http://localhost:3100/metrics | grep "loki_ingester_lines_received_total" | awk '{print $2}')
  echo "日志摄入总量: $LOG_INGESTION_RATE 行"
  
  # 检查重复日志
  DUPLICATE_LOGS=$(kubectl exec -n logging loki-0 -- wget -qO- http://localhost:3100/metrics | grep "loki_ingester_duplicate_lines_total" | awk '{print $2}')
  echo "重复日志数量: $DUPLICATE_LOGS"
  
  # 3. 追踪数据质量检查
  echo "3. 追踪数据质量检查:"
  
  # 检查追踪跨度
  TRACE_SPANS=$(kubectl exec -n tracing jaeger-collector-0 -- wget -qO- http://localhost:14269/metrics | grep "jaeger_collector_spans_received_total" | awk '{print $2}')
  echo "接收的追踪跨度: $TRACE_SPANS"
  
  # 检查采样率
  SAMPLED_TRACES=$(kubectl exec -n tracing jaeger-collector-0 -- wget -qO- http://localhost:14269/metrics | grep "jaeger_collector_traces_saved_total" | awk '{print $2}')
  if [ "$TRACE_SPANS" != "0" ]; then
    SAMPLING_RATE=$((SAMPLED_TRACES * 100 / TRACE_SPANS))
    echo "追踪采样率: ${SAMPLING_RATE}%"
  fi
  
  # 4. 存储使用效率检查
  echo "4. 存储使用效率检查:"
  
  # Prometheus 存储效率
  PROM_STORAGE=$(kubectl exec -n monitoring prometheus-k8s-0 -- df /prometheus | tail -1 | awk '{print $5}' | sed 's/%//')
  echo "Prometheus 存储使用率: ${PROM_STORAGE}%"
  
  # Loki 存储效率
  LOKI_STORAGE=$(kubectl exec -n logging loki-0 -- df /var/loki | tail -1 | awk '{print $5}' | sed 's/%//')
  echo "Loki 存储使用率: ${LOKI_STORAGE}%"
  
} >> "$QUALITY_REPORT"

echo "可观测性数据质量报告已生成: $QUALITY_REPORT"
```
### 典型问题案例

#### 案例：Prometheus 数据存储爆满

**问题描述**：Prometheus 实例磁盘空间耗尽，导致数据无法写入，监控告警失效。

**根本原因**：数据保留时间设置过长，加上高基数指标导致存储快速增长。

**解决方案**：
1. 调整数据保留策略，缩短保留时间
2. 优化指标抓取配置，减少高基数指标
3. 启用远程写入，将历史数据归档到长期存储
4. 实施存储容量监控和预警机制

#### 案例：Grafana 查询性能下降

**问题描述**：Grafana 仪表板加载缓慢，复杂查询经常超时。

**根本原因**：数据源连接配置不当，缺乏查询缓存，仪表板设计不合理。

**解决方案**：
1. 优化数据源连接池配置
2. 启用查询结果缓存
3. 重构复杂仪表板，分解查询
4. 实施查询性能监控

### 支持资源

**官方文档**：
- Prometheus: https://prometheus.io/docs/
- Grafana: https://grafana.com/docs/
- Loki: https://grafana.com/docs/loki/latest/
- Jaeger: https://www.jaegertracing.io/docs/

**社区支持**：
- CNCF Observability TAG: https://github.com/cncf/tag-observability
- Prometheus 社区: https://prometheus.io/community/
- Grafana 社区: https://community.grafana.com/

## Related

- 10-monitoring-metrics-prometheus
- 99-prometheus-enterprise-guide
- 01-observability-architecture-overview
- 02-grafana-enterprise-observability
- 08-docker-troubleshooting-guide
- [[domain-19-landscape-references/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

## See Also

- [[32-发布/package/2026-07-02_18-29/profiles/sre/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/03-ebpf-observability-troubleshooting|03-ebpf-observability-troubleshooting]]
- [[32-发布/package/2026-07-02_18-29/profiles/sre/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/04-finops-cost-optimization-troubleshooting|04-finops-cost-optimization-troubleshooting]]
- [[32-发布/package/2026-07-02_18-29/profiles/sre/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/02-opentelemetry-troubleshooting|02-opentelemetry-troubleshooting]]
- [[32-发布/package/2026-07-02_18-29/profiles/sre/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/15-monitoring-observability/03-ebpf-observability-troubleshooting|03-ebpf-observability-troubleshooting]]

```

<!-- risk-assessed -->
