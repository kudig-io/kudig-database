---
title: "可观测性成本优化"
description: "可观测性数据全生命周期成本优化：指标基数控制、采样策略、存储分层、Recording Rules、指标裁剪与成本归因"
summary: "系统化的可观测性成本治理方案，覆盖 Prometheus 指标基数爆炸治理、Traces/Logs 采样策略优化、存储分层与数据生命周期管理、Recording Rules 预聚合、指标裁剪白名单机制以及按团队/服务的成本归因模型"
category: 可观测性
tags:
- cost-optimization
- cardinality
- sampling
- storage-tiering
- recording-rules
- metric-pruning
- cost-attribution
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- "Prometheus 指标基数爆炸如何治理"
- "可观测性数据存储成本如何优化"
- "如何按团队归因可观测性成本"
trigger_keywords:
- 成本优化
- 基数控制
- cardinality
- 采样策略
- 存储分层
- recording-rules
- 指标裁剪
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 可观测性成本优化

## 概述

可观测性系统的成本正在成为企业 IT 支出中增长最快的部分之一。在大规模 Kubernetes 集群中，一个不经治理的可观测性平台可能产生每月数万美元的存储和计算费用。成本爆炸的三大元凶：指标基数失控（高基数标签导致时间序列数量指数增长）、全量数据存储（未分层的存储策略）、以及缺乏成本归因（无人对数据量负责）。

本文提供系统化的可观测性成本治理框架，从数据产生（采集端）到数据存储（后端）到数据消费（查询端）的全生命周期优化策略。与 [[09-可观测性/02-指标/17-monitoring-cost-optimization.md|监控成本优化]] 侧重 Prometheus 特定优化不同，本文覆盖 Metrics、Traces、Logs 三种信号类型的统一成本治理。

## 核心概念

### 可观测性成本构成

```
┌─────────────────────────────────────────────────────────────────┐
│                  可观测性成本构成模型                              │
│                                                                   │
│  采集层 (20-30%)          存储层 (40-60%)        查询层 (10-20%)  │
│  ┌──────────────┐        ┌──────────────┐       ┌──────────────┐ │
│  │ Agent 资源    │        │ 热存储 (SSD)  │       │ 查询计算      │ │
│  │ 网络带宽      │        │ 温存储 (HDD)  │       │ 聚合计算      │ │
│  │ 预处理 CPU   │        │ 冷存储 (S3)   │       │ Dashboard 渲染│ │
│  └──────────────┘        │ 索引存储      │       └──────────────┘ │
│                          └──────────────┘                         │
│                                                                   │
│  成本驱动因素:                                                    │
│  • 时间序列数量 (基数)     • 数据保留时长      • 查询频率          │
│  • 采集频率 (间隔)        • 压缩率           • 聚合复杂度          │
│  • 标签数量              • 副本数            • 并发用户数          │
└─────────────────────────────────────────────────────────────────┘
```

### 指标基数问题解析

指标基数（Cardinality）是指唯一时间序列的数量。每个唯一的 `{metric_name, label1=value1, label2=value2, ...}` 组合构成一条独立的时间序列。基数爆炸的典型原因：

| 问题类型 | 示例 | 影响 | 检测方法 |
|---------|------|------|---------|
| 高基数标签 | `user_id`, `request_id`, `pod_ip` | 序列数随用户/请求线性增长 | `count by (__name__)({__name__=~".+"})` |
| 标签组合爆炸 | 5 个标签各 10 个值 = 100K 序列 | 序列数呈笛卡尔积增长 | TSDB Status 页面 |
| 未清理的旧序列 | 已删除 Pod 的指标残留 | 序列数随时间单调递增 | `count({__name__=~".+"})` 趋势 |
| 过度细粒度 | 每个 HTTP 路径一条序列 | REST API 路径参数导致爆炸 | 按 `__name__` 排序 Top N |

### 存储分层策略

| 存储层 | 介质 | 保留期 | 查询延迟 | 成本 (相对) | 适用数据 |
|--------|------|--------|---------|------------|---------|
| 热存储 (Hot) | SSD / 内存 | 0-7 天 | <1s | 1x (基准) | 实时告警、当前 Dashboard |
| 温存储 (Warm) | HDD / 对象存储 | 7-30 天 | 5-30s | 0.3x | 历史趋势、周报 |
| 冷存储 (Cold) | S3/GCS 归档 | 30-365 天 | 分钟级 | 0.05x | 合规审计、年度分析 |
| 删除 | - | >365 天 | - | 0 | 过期数据 |

## 生产部署/实现

### Prometheus 指标基数治理

通过 metric_relabel_configs 在采集端裁剪高基数指标：

```yaml
# 🟡 中风险：修改 scrape 配置会停止采集被裁剪的指标
apiVersion: v1
kind: ConfigMap
metadata:
  name: prometheus-cost-optimization
  namespace: monitoring
data:
  cost-optimization-rules.yaml: |
    # 在 Prometheus scrape config 中应用
    # 用于裁剪已知的高基数指标
    metric_relabel_configs:
    # 裁剪 Go runtime 的细粒度内存指标（通常不需要）
    - source_labels: [__name__]
      regex: 'go_memstats_.*'
      action: drop

    # 裁剪 etcd 的调试级指标
    - source_labels: [__name__]
      regex: 'etcd_debugging_.*'
      action: drop

    # 裁剪 apiserver 的请求级细粒度指标（保留聚合）
    - source_labels: [__name__]
      regex: 'apiserver_request_duration_seconds_bucket'
      action: drop

    # 移除高基数标签（保留指标但降低基数）
    - source_labels: [__name__]
      regex: 'http_request_duration_seconds_bucket'
      target_label: pod
      action: replace
      replacement: ''

    # 裁剪包含用户 ID 的自定义指标
    - source_labels: [__name__]
      regex: '.*_user_specific_.*'
      action: drop

  recording-rules.yaml: |
    groups:
    - name: cost-optimization-preaggregation
      interval: 30s
      rules:
      # 预聚合 HTTP 请求指标（替代存储原始高基数数据）
      - record: job:http_requests:rate5m
        expr: sum(rate(http_requests_total[5m])) by (job, code, method)

      - record: job:http_request_duration:p50
        expr: |
          histogram_quantile(0.5,
            sum(rate(http_request_duration_seconds_bucket[5m])) by (job, le)
          )

      - record: job:http_request_duration:p95
        expr: |
          histogram_quantile(0.95,
            sum(rate(http_request_duration_seconds_bucket[5m])) by (job, le)
          )

      - record: job:http_request_duration:p99
        expr: |
          histogram_quantile(0.99,
            sum(rate(http_request_duration_seconds_bucket[5m])) by (job, le)
          )

      # 预聚合资源使用指标
      - record: namespace:container_cpu:rate5m
        expr: |
          sum(rate(container_cpu_usage_seconds_total{container!=""}[5m])) by (namespace)

      - record: namespace:container_memory:bytes
        expr: |
          sum(container_memory_working_set_bytes{container!=""}) by (namespace)

      # 预聚合节点级指标
      - record: cluster:node_cpu:utilization
        expr: |
          1 - avg(rate(node_cpu_seconds_total{mode="idle"}[5m]))

      - record: cluster:node_memory:utilization
        expr: |
          1 - sum(node_memory_MemAvailable_bytes) / sum(node_memory_MemTotal_bytes)
```

### OTel Collector 采样策略优化

在采集端通过 [[09-可观测性/04-链路追踪/05-otel-collector-deep-configuration.md|OTel Collector]] 实施分层采样：

```yaml
# 🟡 中风险：修改采样配置会影响 Trace 数据完整性
apiVersion: v1
kind: ConfigMap
metadata:
  name: otel-collector-cost-sampling
  namespace: observability
data:
  cost-sampling-config.yaml: |
    processors:
      # 第一层：概率采样（大幅降低数据量）
      probabilistic_sampler:
        sampling_percentage: 20
        hash_seed: 42

      # 第二层：属性过滤（丢弃无价值的 Span）
      filter/cost:
        spans:
          exclude:
            match_type: strict
            attributes:
            - key: http.route
              value: /healthz
            - key: http.route
              value: /readyz
            - key: http.route
              value: /metrics
            - key: http.status_code
              value: 200

      # 第三层：速率限制（控制绝对数据量）
      rate_limiting:
        spans_per_second: 5000

      # 保留关键 Trace 的策略（在 Gateway 层 Tail Sampling）
      tail_sampling:
        decision_wait: 10s
        policies:
        - name: keep-errors
          type: status_code
          status_code:
            status_codes: [ERROR]
        - name: keep-slow
          type: latency
          latency:
            threshold_ms: 1000
        - name: base-rate
          type: probabilistic
          probabilistic:
            sampling_percentage: 5

    service:
      pipelines:
        traces/cost-optimized:
          receivers: [otlp]
          processors:
          - memory_limiter
          - filter/cost
          - probabilistic_sampler
          - batch
          exporters: [otlp/tempo]
```

### Thanos/Mimir 存储分层配置

使用 Thanos 实现指标数据的自动分层存储：

```yaml
# 🟡 中风险：修改存储配置影响数据保留和查询能力
apiVersion: v1
kind: ConfigMap
metadata:
  name: thanos-storage-tiering
  namespace: monitoring
data:
  thanos-rule.yaml: |
    # Thanos Rule: 数据降采样与分层
    groups:
    - name: downsampling-rules
      rules:
      # 7 天后的数据降采样为 5 分钟粒度
      - record: :http_requests:rate5m_downsampled
        expr: avg_over_time(job:http_requests:rate5m[5m])

  objstore-config.yaml: |
    type: S3
    config:
      bucket: thanos-metrics-prod
      endpoint: s3.internal:9000
      insecure: true

  # Thanos Store Gateway 配置（分层查询）
  store-gateway-args: |
    --data-dir=/var/thanos/store
    --objstore.config-file=/etc/thanos/objstore-config.yaml
    # 热数据：本地 SSD 缓存
    --block-sync-concurrency=20
    # 查询时自动选择最近的副本
    --selector.relabel-config-file=/etc/thanos/relabel.yaml

  # Thanos Compactor 配置（自动降采样）
  compactor-args: |
    --data-dir=/var/thanos/compact
    --objstore.config-file=/etc/thanos/objstore-config.yaml
    # 降采样策略：
    # 原始数据保留 7 天
    --retention.resolution-raw=7d
    # 5 分钟粒度保留 30 天
    --retention.resolution-5m=30d
    # 1 小时粒度保留 365 天
    --retention.resolution-1h=365d
    # 自动删除过期 Block
    --delete-delay=48h
```

### 成本归因 Dashboard 与告警

```yaml
# 🟢 低风险：只读配置，用于成本可视化
apiVersion: v1
kind: ConfigMap
metadata:
  name: observability-cost-attribution
  namespace: monitoring
data:
  cost-dashboard.json: |
    {
      "title": "可观测性成本归因",
      "panels": [
        {
          "title": "各 Namespace 时间序列数量",
          "expr": "count by (namespace) ({__name__=~\".+\"}) ",
          "description": "按 Namespace 统计活跃时间序列数，用于成本分摊"
        },
        {
          "title": "Top 20 高基数指标",
          "expr": "topk(20, count by (__name__) ({__name__=~\".+\"}))",
          "description": "识别基数最高的指标，优先治理"
        },
        {
          "title": "每日新增时间序列",
          "expr": "increase(prometheus_tsdb_head_series[24h])",
          "description": "监控序列增长趋势，预警基数爆炸"
        },
        {
          "title": "存储使用量趋势",
          "expr": "prometheus_tsdb_storage_bytes",
          "description": "TSDB 磁盘使用量"
        }
      ]
    }

  cost-alerts.yaml: |
    groups:
    - name: observability-cost-alerts
      rules:
      - alert: HighCardinalityMetric
        expr: |
          count by (__name__) ({__name__=~".+"}) > 10000
        for: 1h
        labels:
          severity: warning
          team: platform
        annotations:
          summary: "指标 {{ $labels.__name__ }} 基数超过 10000"
          description: "该指标产生了 {{ $value }} 条时间序列，请检查标签设计"

      - alert: TSDBGrowthAnomaly
        expr: |
          increase(prometheus_tsdb_head_series[1h]) > 50000
        for: 30m
        labels:
          severity: critical
          team: platform
        annotations:
          summary: "时间序列数量异常增长"
          description: "过去 1 小时新增 {{ $value }} 条时间序列，可能存在基数爆炸"

      - alert: StorageCapacityWarning
        expr: |
          prometheus_tsdb_storage_bytes / prometheus_tsdb_storage_max_bytes > 0.8
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "TSDB 存储使用率超过 80%"
```

## 运维操作

### 基数审计与诊断

```bash
# 🟢 低风险：只读诊断
# 查看 TSDB 状态（Top 10 高基数指标）
kubectl port-forward -n monitoring svc/prometheus-server 9090:9090 &
curl -s 'http://localhost:9090/api/v1/status/tsdb' | \
  jq '.data.seriesCountByMetricName[:10]'

# 统计总时间序列数
curl -s 'http://localhost:9090/api/v1/query' \
  --data-urlencode 'query=count({__name__=~".+"})' | \
  jq '.data.result[0].value[1]'

# 按指标名称统计序列数（Top 20）
curl -s 'http://localhost:9090/api/v1/query' \
  --data-urlencode 'query=topk(20, count by (__name__) ({__name__=~".+"}))' | \
  jq '.data.result[] | {metric: .metric.__name__, count: .value[1]}'

# 查找包含高基数标签的指标
curl -s 'http://localhost:9090/api/v1/query' \
  --data-urlencode 'query=count by (__name__) ({pod=~".+", __name__=~".+"})' | \
  jq '[.data.result[] | select(.value[1] | tonumber > 5000)]'

# 查看 Prometheus 内存使用（基数直接影响内存）
kubectl top pods -n monitoring -l app=prometheus
```

### 指标裁剪操作

```bash
# 🔴 高风险：删除指标数据不可恢复，务必确认指标无下游依赖
# 通过 Admin API 删除特定指标的历史数据
curl -X POST 'http://localhost:9090/api/v1/admin/tsdb/delete_series' \
  --data-urlencode 'match[]={__name__="deprecated_metric_name"}'

# 清理已删除序列的磁盘空间
curl -X POST 'http://localhost:9090/api/v1/admin/tsdb/clean_tombstones'

# 验证删除结果
curl -s 'http://localhost:9090/api/v1/query' \
  --data-urlencode 'query=count({__name__="deprecated_metric_name"})'
```

### 存储容量管理

```bash
# 🟢 低风险：只读查看
# 查看 Prometheus 数据目录大小
kubectl exec -n monitoring statefulset/prometheus-server -- \
  du -sh /prometheus/

# 查看各 Block 的时间范围和大小
kubectl exec -n monitoring statefulset/prometheus-server -- \
  ls -la /prometheus/ | grep "01"

# 查看 Thanos 对象存储使用量
aws s3 ls s3://thanos-metrics-prod --recursive --summarize | tail -5

# 🟡 中风险：手动触发 Compaction
kubectl exec -n monitoring statefulset/thanos-compactor -- \
  /bin/thanos tools bucket ls --objstore.config=/etc/thanos/objstore.yaml
```

## 故障排查

### 基数爆炸应急

当 Prometheus 内存使用率突增、查询变慢时，可能是基数爆炸：

```bash
# 🟢 低风险：只读诊断
# 1. 确认内存使用趋势
kubectl top pods -n monitoring -l app=prometheus

# 2. 查看最近新增的序列（按 Job 分组）
curl -s 'http://localhost:9090/api/v1/query' \
  --data-urlencode 'query=count by (job) ({__name__=~".+"})' | \
  jq '.data.result | sort_by(.value[1] | tonumber) | reverse | .[:10]'

# 3. 检查是否有新部署引入了高基数指标
kubectl get events -n monitoring --sort-by='.lastTimestamp' | grep -i "prometheus\|oom"

# 4. 查看 Prometheus 日志中的 OOM 警告
kubectl logs -n monitoring statefulset/prometheus-server --tail=100 | grep -i "memory\|oom\|series"
```

### 存储成本异常增长

```bash
# 🟢 低风险：只读诊断
# 查看每日数据摄入量
curl -s 'http://localhost:9090/api/v1/query' \
  --data-urlencode 'query=increase(prometheus_tsdb_head_samples_appended_total[24h])'

# 对比各数据源的采集量
curl -s 'http://localhost:9090/api/v1/query' \
  --data-urlencode 'query=sum by (job) (rate(prometheus_tsdb_head_samples_appended_total[1h]))' | \
  jq '.data.result | sort_by(.value[1] | tonumber) | reverse'

# 检查是否有 scrape 间隔配置错误（过短的间隔导致数据量翻倍）
kubectl get configmap prometheus-server -n monitoring -o yaml | grep scrape_interval
```

### 降采样后查询无数据

```bash
# 🟢 低风险：只读诊断
# 检查 Thanos Store Gateway 是否正常运行
kubectl get pods -n monitoring -l app=thanos-store-gateway

# 验证降采样 Block 是否已生成
kubectl exec -n monitoring statefulset/thanos-compactor -- \
  /bin/thanos tools bucket ls --objstore.config=/etc/thanos/objstore.yaml | grep "5m\|1h"

# 检查查询是否命中正确的 Store
curl -s 'http://thanos-query.monitoring.svc:9090/api/v1/stores' | jq '.data'
```

## 最佳实践

### 成本治理优先级矩阵

1. **快速见效（1-2 周）**：
   - 裁剪已知无用指标（go_memstats、etcd_debugging 等）
   - 移除高基数标签（pod_ip、request_id）
   - 调整非关键 Job 的 scrape_interval（15s → 60s）

2. **中期优化（1-2 月）**：
   - 部署 Recording Rules 预聚合，删除原始高基数数据
   - 实施 Traces 分层采样（Head 20% + Tail 保留错误/慢请求）
   - 配置 Logs 保留策略（热 7 天、冷 30 天）

3. **长期治理（季度）**：
   - 建立成本归因模型，按团队/服务分摊
   - 实施指标白名单制度，新指标需审批
   - 自动化存储分层（Thanos Compactor 自动降采样）

### 指标设计规范

- 禁止使用 `user_id`、`request_id`、`trace_id` 作为指标标签
- 标签值数量控制在 100 以内
- 新指标上线前评估基数：`预期序列数 = 标签值组合数 × 实例数`
- 使用 Recording Rules 替代运行时高基数查询
- Histogram bucket 数量控制在 10-15 个

### 成本归因模型

按以下维度分摊可观测性成本：
- **Metrics**：按 Namespace 的活跃时间序列数占比
- **Traces**：按服务的 Span 产生量占比
- **Logs**：按 Pod 的日志摄入量占比
- **存储**：按数据保留策略（热/温/冷）的加权成本

与 [[09-可观测性/02-指标/15-enterprise-scale-monitoring.md|企业级规模监控]] 和 [[09-可观测性/02-指标/04-thanos-enterprise-metrics-federation.md|Thanos 企业级指标联邦]] 配合，实现大规模环境下的成本可控。

## Related

- [[09-可观测性/02-指标/17-monitoring-cost-optimization.md|监控成本优化]]
- [[09-可观测性/02-指标/01-prometheus-enterprise-monitoring.md|Prometheus 企业级监控]]
- [[09-可观测性/02-指标/04-thanos-enterprise-metrics-federation.md|Thanos 企业级指标联邦]]
- [[09-可观测性/04-链路追踪/05-otel-collector-deep-configuration.md|OTel Collector 深度配置]]
- [[09-可观测性/02-指标/15-enterprise-scale-monitoring.md|企业级规模监控]]
- [[09-可观测性/01-总览/01-observability-architecture-overview.md|可观测性架构总览]]
- [[09-可观测性/05-告警/07-aiops-intelligent-alerting.md|AIOps 智能告警]]
