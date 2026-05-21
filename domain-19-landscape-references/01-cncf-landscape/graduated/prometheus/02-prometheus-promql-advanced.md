---
title: Prometheus 高级 PromQL
description: 'description: Prometheus PromQL 高级查询指南，涵盖向量匹配、聚合运算、子查询、记录规则和性能优化'
category: general
tags:
- cncf
- ecosystem
- prometheus
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Prometheus 高级 PromQL 是什么
- 如何 Prometheus 高级 PromQL
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Prometheus
- 高级
- PromQL
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
---

title: Prometheus 高级 PromQL
description: Prometheus PromQL 高级查询指南，涵盖向量匹配、聚合运算、子查询、记录规则和性能优化
category: cncf-landscape
tags:
- k8s
- cncf
- prometheus
- promql
- recording-rules
- query
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- DevOps
- 监控工程师
estimated_reading_time: 10min
intent_queries:
- Prometheus PromQL 高级查询
- PromQL 聚合运算
- Prometheus 记录规则
trigger_keywords:
- Prometheus
- PromQL
- 聚合
- 查询
estimated_read_time: 10min
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
# Prometheus 高级 PromQL

> **适用版本**: Prometheus 2.50+ | **最后更新**: 2026-05

---

## 1. 向量匹配

### 1.1 一对一匹配

```promql
# 匹配 CPU 使用率和内存使用率
node_cpu_seconds_total{mode="idle"} * on(instance) group_left(node_cpu_core)
node_memory_MemAvailable_bytes

# 使用 ignoring 忽略不匹配的标签
http_requests_total{status=~"5.."} / ignoring(status) group_left
sum(http_requests_total) by (handler)
```

### 1.2 一对多匹配 (group_left/group_right)

```promql
# 一个请求对应多个错误
# 请求总数
sum(rate(http_requests_total[5m])) by (handler)

# 错误数
sum(rate(http_requests_total{status=~"5.."}[5m])) by (handler)

# 错误率 = errors / total
sum(rate(http_requests_total{status=~"5.."}[5m])) by (handler)
  / on(handler) group_left
sum(rate(http_requests_total[5m])) by (handler)
```

### 1.3 多对一匹配

```promql
# 使用 group_right 指定右边是多
sum by (pod) (rate(container_cpu_usage_seconds_total[5m]))
  / on(node) group_left()
kube_node_labels
```

---

## 2. 聚合运算符

### 2.1 基础聚合

```promql
# sum - 求和
sum(http_requests_total)

# min - 最小值
min(http_request_duration_seconds)

# max - 最大值
max(http_request_duration_seconds)

# avg - 平均值
avg(http_request_duration_seconds)

# stddev - 标准差
stddev(http_request_duration_seconds)

# stdvar - 方差
stdvar(http_request_duration_seconds)

# count - 计数
count(http_requests_total)
```

### 2.2 分组聚合

```promql
# 按标签分组
sum(http_requests_total) by (handler, status)
min(http_request_duration_seconds) by (service)
max(container_memory_usage_bytes) by (pod, namespace)
avg(http_request_duration_seconds) by (job)
```

### 2.3 topk/bottomk

```promql
# Top 5 CPU 使用率最高的 Pod
topk(5, sum by (pod, namespace) (rate(container_cpu_usage_seconds_total[5m])))

# Bottom 10 内存使用率最低的服务
bottomk(10, avg by (service) (container_memory_usage_bytes))
```

### 2.4 运算符组合

```promql
# 计算 95 分位数
histogram_quantile(0.95, 
  sum(rate(http_request_duration_seconds_bucket[5m])) by (le, handler)
)

# 计算成功率
sum(rate(http_requests_total{status=~"2.."}[5m])) by (handler)
  /
sum(rate(http_requests_total[5m])) by (handler)
  * 100
```

---

## 3. 子查询

### 3.1 基本子查询

```promql
# 最近 30 分钟内，每 5 分钟窗口的请求率
rate(http_requests_total[5m])

# 子查询：每分钟计算一次最近 30 分钟的请求率
rate(http_requests_total[30m])[30m:1m]
```

### 3.2 复杂子查询

```promql
# 子查询结合外部聚合
max_over_time(
  rate(http_requests_total[5m])[1h:5m]
)

# 计算滑动窗口内的最大值
max_over_time(
  histogram_quantile(0.99, 
    sum(rate(http_request_duration_seconds_bucket[5m])) by (le)
  )[1h:1m]
)
```

### 3.3 偏移量 (offset)

```promql
# 与 1 小时前对比
http_requests_total - http_requests_total offset 1h

# 24 小时同比
rate(http_requests_total[5m]) - rate(http_requests_total offset 24h)[5m]

# 周同比
sum(increase(http_requests_total[1h])) by (handler)
  -
sum(increase(http_requests_total[1h] offset 168h)) by (handler)
```

---

## 4. 直方图与百分位数

### 4.1 histogram_quantile

```promql
# 50 分位数 (中位数)
histogram_quantile(0.50, 
  sum(rate(http_request_duration_seconds_bucket[5m])) by (le)
)

# 90 分位数
histogram_quantile(0.90,
  sum(rate(http_request_duration_seconds_bucket[5m])) by (le, handler)
)

# 99 分位数
histogram_quantile(0.99,
  sum(rate(http_request_duration_seconds_bucket[5m])) by (le, service)
)
```

### 4.2 无_bucket 标签的直方图

```promql
# 对于没有 le 标签的简单计数器
# 使用 increase 近似计算
increase(http_request_duration_sum[5m]) 
  /
increase(http_request_duration_count[5m])
```

### 4.3 动态分位数

```promql
# 通用分位数查询函数
quantile_over_time(0.95,
  http_request_duration_seconds[5m]
) by (handler)
```

---

## 5. 记录规则 (Recording Rules)

### 5.1 记录规则配置

```yaml
groups:
  - name: http_requests
    interval: 30s
    rules:
      # 请求率
      - record: job:http_requests_total:rate5m
        expr: |
          sum(rate(http_requests_total[5m])) by (job, handler, status)
      
      # 请求延迟 P99
      - record: job:http_request_duration_seconds:99p
        expr: |
          histogram_quantile(0.99,
            sum(rate(http_request_duration_seconds_bucket[5m])) by (le, job, handler)
          )
      
      # 服务健康状态
      - record: job:http_requests_success:ratio
        expr: |
          sum(rate(http_requests_total{status=~"2.."}[5m])) by (job)
            /
          sum(rate(http_requests_total[5m])) by (job)
```

### 5.2 常见记录规则模式

```yaml
groups:
  # CPU 聚合
  - name: node_cpu
    interval: 15s
    rules:
      - record: instance:node_cpu_usage:ratio
        expr: |
          1 - avg by (instance) (rate(node_cpu_seconds_total{mode="idle"}[5m]))
      
      - record: cluster:node_cpu_usage:ratio
        expr: |
          1 - avg(rate(node_cpu_seconds_total{mode="idle"}[5m]))

  # 内存使用率
  - name: node_memory
    rules:
      - record: instance:node_memory_usage:ratio
        expr: |
          1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes

  # 网络流量
  - name: node_network
    rules:
      - record: instance:network_bytes_total:rate5m
        expr: |
          sum(rate(node_network_receive_bytes_total[5m]) 
            + rate(node_network_transmit_bytes_total[5m])) by (instance)

  # Pod 资源使用
  - name: pod_cpu
    rules:
      - record: namespace:pod_cpu_usage:rate5m
        expr: |
          sum(rate(container_cpu_usage_seconds_total[5m])) by (namespace)
      
      - record: namespace:pod_cpu_usage:ratio
        expr: |
          sum(rate(container_cpu_usage_seconds_total[5m])) by (namespace)
            /
          sum(container_spec_cpu_quota/container_spec_cpu_period[1m]) by (namespace)
            * 100
```

### 5.3 预计算规则

```yaml
groups:
  - name::sla
    interval: 1m
    rules:
      # SLO: 请求延迟 P99 < 500ms
      - record: slo:http_request_p99_latency:ratio
        expr: |
          histogram_quantile(0.99, 
            sum(rate(http_request_duration_seconds_bucket[5m])) by (le)
          ) 
          / 0.5
      
      # SLO: 可用性 > 99.9%
      - record: slo:http_requests_availability:ratio
        expr: |
          sum(rate(http_requests_total{status!~"5.."}[5m])) by ()
            /
          sum(rate(http_requests_total[5m])) by ()
```

---

## 6. 时间序列操作

### 6.1 rate/irate/increase

```promql
# rate - 计算每秒平均增长率 (用于告警和图形)
rate(http_requests_total[5m])

# irate - 计算瞬时增长率 (用于快速变化的指标)
irate(http_requests_total[5m])

# increase - 计算增长总量
increase(http_requests_total[1h])
```

### 6.2 deriv/_predict_linear

```promql
# deriv - 计算每秒变化率
deriv(container_cpu_usage_seconds_total[5m])

# predict_linear - 预测未来值
predict_linear(node_memory_MemAvailable_bytes[1h], 4 * 3600)
# 预测 4 小时后内存是否耗尽
```

### 6.3 滑动窗口

```promql
# 最近 5 分钟的平均值
avg_over_time(http_requests_total[5m])

# 最近 5 分钟的最大值
max_over_time(http_requests_total[5m])

# 最近 5 分钟的最小值
min_over_time(http_requests_total[5m])

# 滑动窗口内的请求率
rate(http_requests_total[5m])
```

---

## 7. 条件判断

### 7.1 unless (除非)

```promql
# 有标签但没有 running 状态
process_up{job="myapp"} unless on(process) 
  process_start_time_seconds{status="running"}
```

### 7.2 条件聚合

```promql
# 只对 CPU > 80% 的实例计数
count(
  avg by (instance) (rate(node_cpu_seconds_total{mode="idle"}[5m])) < 0.2
)
```

### 7.3 布尔运算符

```promql
# 内存使用率 > 80%
(
  node_memory_MemTotal_bytes 
    - 
  node_memory_MemAvailable_bytes
) 
  / 
node_memory_MemTotal_bytes 
  > 0.8
```

---

## 8. 性能优化

### 8.1 标签基数控制

```promql
# 不推荐：高基数标签导致查询慢
sum by (user_id, request_id) (http_requests_total)

# 推荐：使用低基数标签
sum by (handler, status) (http_requests_total)
```

### 8.2 查询效率

```promql
# 不推荐：在大范围数据上计算百分位数
histogram_quantile(0.99, 
  sum(rate(http_request_duration_seconds_bucket[1h])) by (le)
)

# 推荐：使用短时间窗口
histogram_quantile(0.99, 
  sum(rate(http_request_duration_seconds_bucket[5m])) by (le)
)
```

### 8.3 常见性能问题

| 问题 | 原因 | 解决方案 |
|:-----|:-----|:---------|
| 查询超时 | 时间范围过大 | 缩小查询窗口 |
| 高内存使用 | 复杂聚合 | 使用记录规则 |
| 高 CPU | 大量时间序列 | 优化标签基数 |
| 慢查询 | 缺少索引 | 合理使用 label_values |

---

## 9. 常用 PromQL 模板

### 9.1 服务健康检查

```promql
# 服务是否 UP
up{job="myapp"}

# 服务健康率
sum(up{job="myapp"}) / count(up{job="myapp"})

# Pod 健康数
sum(kube_pod_status_phase{phase="Running"}) by (namespace)
```

### 9.2 性能指标

```promql
# QPS
sum(rate(http_requests_total[5m]))

# 错误率
sum(rate(http_requests_total{status=~"5.."}[5m])) 
  / 
sum(rate(http_requests_total[5m])) * 100

# P99 延迟
histogram_quantile(0.99, 
  sum(rate(http_request_duration_seconds_bucket[5m])) by (le)
)
```

### 9.3 资源使用

```promql
# CPU 使用率
100 * (1 - avg by (instance) (rate(node_cpu_seconds_total{mode="idle"}[5m])))

# 内存使用率
100 * (1 - avg by (instance) (node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes))

# Pod CPU 使用率
sum(rate(container_cpu_usage_seconds_total[5m])) by (pod, namespace)
  /
sum(container_spec_cpu_quota/container_spec_cpu_period) by (pod, namespace) * 100
```

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/promql.md|promql]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/02-prometheus-promql-advanced.md|PromQL 高级查询]]

## See Also

- [[domain-19-landscape-references/graduated/prometheus/03-prometheus-ha-deployment.md|03-prometheus-ha-deployment]]
- [[domain-19-landscape-references/graduated/prometheus/prometheus.md|prometheus]]
- [[domain-19-landscape-references/graduated/prometheus/03-prometheus-ha-deployment.md|03-prometheus-ha-deployment]]
- [[domain-19-landscape-references/graduated/prometheus/prometheus.md|prometheus]]
