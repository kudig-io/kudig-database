---
title: PromQL 速查卡
description: Prometheus Query Language 快速参考，覆盖 Prometheus v2.40+ 常用查询
summary: Prometheus Query Language 快速参考，覆盖 Prometheus v2.40+ 常用查询
category: cheatsheet
tags:
- prometheus
- promql
- monitoring
- cheatsheet
- quick-reference
- observability
- statefulset
- job
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- PromQL 速查卡 是什么
- 如何 PromQL 速查卡
trigger_keywords:
- PromQL
- 速查卡
- cheat
- sheet
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
authors:
- name: KUDIG Team
  role: contributor
related_docs:
- path: ../可观测性/02-monitoring-metrics-system.md
  desc: 监控指标系统深度文档
- path: ../系统基础/topic-cheat-sheet/k8s.md
  desc: Kubernetes 速查卡
- path: ../可观测性/
  desc: 监控告警专题
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# PromQL 速查表

> [[Prometheus|Prometheus]] Query Language 快速参考 | Prometheus v2.40+ | **最后更新**: 2026-05

---

## 目录

- [基础查询](#基础查询)
- [时间序列选择器](#时间序列选择器)
- [运算符](#运算符)
- [聚合操作](#聚合操作)
- [函数大全](#函数大全)
- [常用查询模式](#常用查询模式)
- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes]] 监控](#kubernetes-监控)
- [告警规则模板](#告警规则模板)

---

## 基础查询

### 瞬时向量 vs 范围向量

```promql
# 瞬时向量 - 单个时间点
cpu_usage_percent

# 范围向量 - 时间范围内的样本
http_requests_total[5m]      # 最近5分钟
http_requests_total[1h]      # 最近1小时
http_requests_total[1d]      # 最近1天
```

**时间单位**:
| 单位 | 含义 |
|:---|:---|
| `ms` | 毫秒 |
| `s` | 秒 |
| `m` | 分钟 |
| `h` | 小时 |
| `d` | 天 |
| `w` | 周 |
| `y` | 年 |

---

## 时间序列选择器

### 标签匹配

```promql
# 完全匹配 (=)
http_requests_total{job="api-server", status="200"}

# 不等于 (!=)
http_requests_total{status!="500"}

# 正则匹配 (=~)
http_requests_total{status=~"2.."}           # 2xx 状态码
http_requests_total{path=~"/api/.*"}         # /api/ 路径
http_requests_total{job=~"api|web"}          # 多个 job

# 正则不匹配 (!~)
http_requests_total{status!~"4..|5.."}       # 排除 4xx 和 5xx
```

### 范围向量修饰符

```promql
# 偏移 (@)
http_requests_total[5m] offset 1h            # 1小时前的5分钟数据

# 当前时间查询
cpu_usage_percent @ 1609459200               # 特定时间戳
```

---

## 运算符

### 算术运算符

```promql
# 基本运算
cpu_usage_percent * 100                      # 转换为百分比
memory_bytes / 1024 / 1024 / 1024            # 转换为 GB
disk_free / disk_total * 100                 # 计算百分比

# 向量与标量运算
http_request_duration_seconds * 1000         # 秒转毫秒

# 向量间运算（相同标签）
memory_used / memory_total                   # 内存使用率
```

### 比较运算符

```promql
# 返回满足条件的样本（值为1）
cpu_usage_percent > 80                       # CPU > 80%
disk_free_gb < 10                            # 磁盘 < 10GB

# 布尔运算（返回0或1）
cpu_usage_percent > bool 80                  # 返回 0 或 1
```

**比较运算符**: `==`, `!=`, `>`, `<`, `>=`, `<=`

### 逻辑/集合运算符

```promql
# 与运算 (and) - 取交集
cpu_usage_percent > 80 and up == 1

# 或运算 (or) - 取并集
cpu_usage_percent > 80 or memory_usage > 80

# 除非 (unless) - 差集
http_requests_total unless http_errors_total
```

---

## 聚合操作

### 基本聚合

```promql
# 按 job 聚合
sum(http_requests_total) by (job)
avg(cpu_usage_percent) by (instance)

# 多标签聚合
sum(http_requests_total) by (job, status)
max(memory_usage) by (namespace, pod)

# 排除标签聚合 (without)
sum(http_requests_total) without (instance)   # 排除 instance 标签
```

**聚合函数**:
| 函数 | 说明 |
|:---|:---|
| `sum()` | 求和 |
| `avg()` | 平均值 |
| `min()` | 最小值 |
| `max()` | 最大值 |
| `count()` | 计数 |
| `count_values()` | 按值计数 |
| `group()` | 结果设为1 |
| `stddev()` | 标准差 |
| `stdvar()` | 方差 |
| `quantile(φ, ...)` | 分位数 (0 ≤ φ ≤ 1) |
| `topk(n, ...)` | 最大的 n 个 |
| `bottomk(n, ...)` | 最小的 n 个 |

### 高级聚合示例

```promql
# 计算百分位数
histogram_quantile(0.95, rate(http_request_duration_seconds_bucket[5m]))

# Top 10 CPU 使用实例
topk(10, cpu_usage_percent)

# 按状态码分布统计
count_values("status", http_requests_status_code)

# 95分位延迟按 job 分组
histogram_quantile(0.95, 
  sum(rate(http_request_duration_seconds_bucket[5m])) by (job, le)
)
```

---

## 函数大全

### 变化率函数

```promql
# rate() - 每秒平均增长率（用于计数器）
rate(http_requests_total[5m])

# irate() - 瞬时增长率（基于最后两个样本）
irate(cpu_seconds_total[5m])

# increase() - 时间范围内的增量
increase(http_requests_total[1h])

# delta() - 差值（用于仪表）
delta(temperature[1h])

# idelta() - 瞬时差值
idelta(memory_usage[5m])
```

### 预测函数

```promql
# predict_linear() - 预测何时达到阈值
predict_linear(disk_free[1h], 3600) < 0      # 1小时内磁盘是否会满

# deriv() - 每秒导数
deriv(memory_usage[10m])
```

### 时间函数

```promql
# time() - 当前 Unix 时间戳
time() - process_start_time_seconds           # 进程运行时间

# timestamp() - 样本的时间戳
timestamp(up)
```

### 直方图函数

```promql
# histogram_quantile() - 计算分位数
histogram_quantile(0.99, 
  sum(rate(http_request_duration_seconds_bucket[5m])) by (le)
)

# 各 bucket 的请求数
rate(http_request_duration_seconds_bucket[5m])
```

### 数学函数

```promql
# abs() - 绝对值
abs(temperature_change)

# floor() / ceil() / round()
floor(memory_gb)                             # 向下取整
ceil(memory_gb)                              # 向上取整
round(memory_gb, 0.5)                        # 四舍五入到0.5

# clamp() / clamp_max() / clamp_min()
clamp(cpu_usage, 0, 100)                     # 限制在 0-100
clamp_max(memory_usage, 80)                  # 最大不超过80

# sort() / sort_desc()
sort(cpu_usage)                              # 升序
sort_desc(memory_usage)                      # 降序
```

### 标签操作函数

```promql
# label_join() - 合并标签
label_join(up{job="api"}, "new_label", "-", "instance", "job")

# label_replace() - 正则替换标签
label_replace(up, "short_instance", "$1", "instance", "(.*):.*")
```

### 其他实用函数

```promql
# absent() - 检查序列是否存在
absent(up{job="critical-service"})           # 服务不存在时返回1

# scalar() - 将单元素向量转为标量
scalar(count(up))

# vector() - 将标量转为向量
vector(1)

# year() / month() / day() / hour() / minute() / second()
hour()

# day_of_week() / day_of_month() / days_in_month()
day_of_week()
```

---

## 常用查询模式

### 系统资源监控

```promql
# CPU 使用率 (100 - 空闲率)
100 - (avg(irate(node_cpu_seconds_total{mode="idle"}[5m])) by (instance) * 100)

# 内存使用率
100 * (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes)

# 磁盘使用率
100 * (1 - node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"})

# 磁盘 IO 利用率
100 - (avg(irate(node_disk_io_time_seconds_total[5m])) by (device)) * 100

# 网络流量
rate(node_network_receive_bytes_total[5m])   # 接收
rate(node_network_transmit_bytes_total[5m])  # 发送
```

### HTTP 服务监控

```promql
# 请求速率 (QPS)
sum(rate(http_requests_total[5m])) by (job)

# 错误率
sum(rate(http_requests_total{status=~"5.."}[5m])) by (job)

# 错误率百分比
100 * sum(rate(http_requests_total{status=~"5.."}[5m])) 
  / sum(rate(http_requests_total[5m]))

# P99 延迟
histogram_quantile(0.99, 
  sum(rate(http_request_duration_seconds_bucket[5m])) by (le, job)
)

# 各状态码分布
sum(rate(http_requests_total[5m])) by (status)
```

### 业务指标监控

```promql
# 每分钟订单数
rate(orders_total[1m])

# 活跃用户（去重计数）
count(count by (user_id) (user_activity[1h]))

# 转化率
rate(checkouts_total[1h]) / rate(carts_created_total[1h])

# SLA 计算
sum(rate(requests_total{status!~"5.."}[24h])) / sum(rate(requests_total[24h]))
```

---

## Kubernetes 监控

### Pod 资源使用

```promql
# Pod CPU 使用率
cpu_usage = container_cpu_usage_seconds_total{container!="POD"}
rate(cpu_usage[5m])

# Pod 内存使用
container_memory_working_set_bytes{container!="POD"}

# Pod 重启次数
kube_pod_container_status_restarts_total

# Pod 状态
kube_pod_status_phase{phase="Running"}
```

### 节点监控

```promql
# 节点 CPU 压力
100 - avg(rate(node_cpu_seconds_total{mode="idle"}[5m])) by (node) * 100

# 节点内存压力
100 * (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes)

# 节点磁盘压力
100 * (1 - node_filesystem_avail_bytes / node_filesystem_size_bytes)

# 节点状态
kube_node_status_condition{condition="Ready", status="true"}
```

### 控制器监控

```promql
# Deployment 期望/可用副本数
kube_deployment_status_replicas
kube_deployment_status_replicas_available

# 副本差异
kube_deployment_status_replicas - kube_deployment_status_replicas_available

# StatefulSet 副本
kube_statefulset_status_replicas

# Job 完成状态
kube_job_status_succeeded
```

### 资源配额

```promql
# CPU 配额使用
kube_resourcequota{resource="requests.cpu", type="used"}
kube_resourcequota{resource="limits.cpu", type="hard"}

# 配额使用百分比
100 * kube_resourcequota{resource="requests.cpu", type="used"} 
  / kube_resourcequota{resource="requests.cpu", type="hard"}
```

---

## 告警规则模板

### 高可用性告警

```yaml
groups:
  - name: high-availability
    rules:
      - alert: HighErrorRate
        expr: |
          sum(rate(http_requests_total{status=~"5.."}[5m])) 
          / sum(rate(http_requests_total[5m])) > 0.05
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "High error rate detected"
          
      - alert: HighLatency
        expr: |
          histogram_quantile(0.99, 
            sum(rate(http_request_duration_seconds_bucket[5m])) by (le)
          ) > 1
        for: 5m
        labels:
          severity: warning
```

### 资源告警

```yaml
groups:
  - name: resource-alerts
    rules:
      - alert: HighCPUUsage
        expr: 100 - (avg(irate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 80
        for: 5m
        
      - alert: HighMemoryUsage
        expr: |
          (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) > 0.85
        for: 5m
        
      - alert: DiskWillFillIn4Hours
        expr: predict_linear(node_filesystem_avail_bytes[1h], 4*3600) < 0
        for: 10m
```

### Kubernetes 告警

```yaml
groups:
  - name: kubernetes
    rules:
      - alert: PodCrashLooping
        expr: rate(kube_pod_container_status_restarts_total[15m]) > 0
        for: 5m
        
      - alert: PodNotReady
        expr: |
          sum by (namespace, pod) (
            kube_pod_status_phase{phase=~"Pending|Unknown|Failed"}
          ) > 0
        for: 15m
        
      - alert: DeploymentReplicasMismatch
        expr: |
          kube_deployment_status_replicas_desired 
          != kube_deployment_status_replicas_available
        for: 15m
```

---

## 查询优化技巧

```promql
# 1. 使用 recording rule 预计算常用查询
# rules.yml:
# - record: job:http_requests:rate5m
#   expr: sum(rate(http_requests_total[5m])) by (job)

# 2. 避免大时间范围查询
rate(metric[1h])        # 慢
rate(metric[5m])        # 快，使用 recording rule

# 3. 限制返回数量
topk(10, metric)        # 只返回前10

# 4. 使用合适的分桶粒度
histogram_quantile(0.99, metric_bucket)   # 确保 bucket 粒度合适
```

---

## 相关文档

- [可观测性/02-monitoring-metrics-system.md](../../09-%E5%8F%AF%E8%A7%82%E6%B5%8B%E6%80%A7/02-%E6%8C%87%E6%A0%87/02-monitoring-metrics-system.md) - Prometheus 监控体系
- [可观测性/](../可观测性/) - 企业监控告警

## Related

- index/observability-index|Observability 可观测性知识图谱索引]]


<!-- risk-assessed -->
