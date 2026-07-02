---
title: Prometheus 数据丢失与查询响应慢
description: 专有云 ACK 集群 Prometheus 监控出现历史数据缺失与查询超时，影响故障定位的工单闭环样本。
summary: 专有云 ACK 集群 Prometheus 监控出现历史数据缺失与查询超时，影响故障定位的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- prometheus
- observability
- tsdb
- p2
- performance
tier: peripheral
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:30:00+08:00'
incident_id: INC-2026-ACK-015
priority: P2
severity: medium
affected_cluster: ack-zyy-prod-06
affected_namespace: monitoring
ticket_type: 可观测性异常
skill_ref:
- Prometheus 排障指南
- Prometheus 存储优化
fta_ref:
- 'FTA: Prometheus 数据缺失'
last_updated: 2026-06-26 16:30:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Prometheus 数据丢失与查询响应慢 如何处理
trigger_keywords:
- Prometheus
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
relationships:
- target: '[[entities/prometheus.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
- target: '[[concepts/bp-observability.md]]'
  type: related_to
---



# 工单描述

客户发现 `ack-zyy-prod-06` 集群的 Grafana 大盘中部分指标出现断点，且查询近 7 天数据时经常超时。客户描述如下：

> “我们 monitoring 命名空间里的 Prometheus 最近总是丢数据，Grafana 上很多面板显示‘No data’，查 7 天的曲线要转半天，有时候直接 504。kubectl 看 Prometheus Pod 没有重启，但是日志里有很多 compact 和 WAL 相关的报错。监控看不了，排障很受限制。帮忙看看是不是磁盘或者内存不够。”

影响范围为 `monitoring` 命名空间中的 Prometheus Server，以及依赖该实例的多个业务 Grafana 大盘。

## 分类与优先级判定

- **工单类型**：可观测性异常 / 性能问题。
- **优先级**：P2。
- **严重级别**：medium。

判定依据：
1. 监控数据丢失影响故障定位，但当前业务服务本身未中断。
2. 问题集中在 Prometheus TSDB 存储与查询性能，排查方向明确。
3. 需在 30 分钟内定位原因并给出优化方案。

## 诊断步骤

按“先看 Prometheus 状态、再看存储与 TSDB、最后分析查询负载”的顺序排查：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 查看 Prometheus Pod 状态与资源使用
kubectl get pod -n monitoring -l app=prometheus
kubectl describe pod -n monitoring -l app=prometheus | grep -A 20 Events
kubectl top pod -n monitoring -l app=prometheus

# 2. 查看 Prometheus 日志
kubectl logs -n monitoring -l app=prometheus -c prometheus --tail=500 | grep -iE "compact|wal|checkpoint|out of|too many|slow"

# 3. 检查 PVC 使用情况
kubectl get pvc -n monitoring
kubectl describe pvc prometheus-data-prometheus-0 -n monitoring
kubectl exec -n monitoring prometheus-0 -- df -h /prometheus

# 4. 检查 TSDB 状态与数据块
kubectl exec -n monitoring prometheus-0 -- wget -qO- http://localhost:9090/api/v1/status/tsdb
kubectl exec -n monitoring prometheus-0 -- ls -lh /prometheus/wal /prometheus/chunks_head

# 5. 查看当前抓取目标与样本量
kubectl exec -n monitoring prometheus-0 -- wget -qO- http://localhost:9090/api/v1/targets | jq '.data.activeTargets | length'
kubectl exec -n monitoring prometheus-0 -- wget -qO- http://localhost:9090/api/v1/status/runtimeinfo | jq '.data'

# 6. 检查 recording rules 与告警规则数量
kubectl get prometheusrules -n monitoring --no-headers | wc -l
kubectl get servicemonitor -A --no-headers | wc -l
kubectl get podmonitor -A --no-headers | wc -l
```

## 根因分析

Prometheus Server 运行在单副本 StatefulSet 中，PVC 容量为 100Gi，当前已使用 96Gi。TSDB 日志中出现以下错误：

```
level=error ts=... caller=compact.go:... msg="compact blocks" err="persist head block: write chunks: no space left on device"
level=warn ts=... caller=wal.go:... msg="WAL truncation completed", duration=...
```

根本原因：
1. **存储空间不足**：随着抓取目标增加到 800+，日均写入样本量超过 50GB，100Gi PVC 无法满足保留 15 天的需求；
2. **WAL 文件过大**：异常退出或 compaction 阻塞导致 WAL 文件持续增长，重启后 replay 耗时极长；
3. **高基数指标**：部分业务暴露了大量无边界 label（如 user_id、request_id），导致 TSDB 索引膨胀，查询时内存与 CPU 飙升；
4. **单实例瓶颈**：所有监控数据集中在一个 Prometheus 实例，未做水平分片（federation 或 Thanos）。

## 修复命令

**第一步：扩容 PVC 以恢复 compaction 与 WAL 写入**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl patch pvc prometheus-data-prometheus-0 -n monitoring -p '{"spec":{"resources":{"requests":{"storage":"300Gi"}}}}'
# 若存储类不支持在线扩容，需滚动重启 StatefulSet
kubectl rollout restart statefulset/prometheus -n monitoring
kubectl rollout status statefulset/prometheus -n monitoring --timeout=600s
```

**第二步：调整 Prometheus 保留时间与压缩参数**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch prometheus prometheus -n monitoring --type='json' -p='[
  {"op": "replace", "path": "/spec/retention", "value": "10d"},
  {"op": "add", "path": "/spec/retentionSize", "value": "250GB"}
]'
```

**第三步：识别并过滤高基数指标**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查询 top10 高基数指标
kubectl exec -n monitoring prometheus-0 -- wget -qO- 'http://localhost:9090/api/v1/status/tsdb' | jq '.data.headStats'

# 在 ServiceMonitor/PodMonitor 中增加 metricRelabelings 丢弃高基数 label
kubectl apply -n monitoring -f - <<EOF
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: app-metrics-filtered
spec:
  endpoints:
  - port: metrics
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'http_requests_total|grpc_requests_total'
      action: keep
    - regex: 'user_id|request_id|session_id'
      action: labeldrop
EOF
```

**第四步：提升 Prometheus 资源限制**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch prometheus prometheus -n monitoring --type='json' -p='[
  {"op": "replace", "path": "/spec/resources/limits/memory", "value": "16Gi"},
  {"op": "replace", "path": "/spec/resources/limits/cpu", "value": "8"},
  {"op": "replace", "path": "/spec/resources/requests/memory", "value": "8Gi"},
  {"op": "replace", "path": "/spec/resources/requests/cpu", "value": "4"}
]'
```

**第五步：启用 Thanos Sidecar 实现长期存储与查询分流（可选长期方案）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
kubectl apply -n monitoring -f - <<EOF
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: prometheus
spec:
  thanos:
    image: quay.io/thanos/thanos:v0.34.0
    objectStorageConfig:
      key: thanos.yaml
      name: thanos-objstore-config
EOF
```

## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. PVC 扩容成功
kubectl get pvc prometheus-data-prometheus-0 -n monitoring
kubectl exec -n monitoring prometheus-0 -- df -h /prometheus

# 2. Prometheus 状态正常
kubectl get pod -n monitoring -l app=prometheus -o wide
kubectl logs -n monitoring prometheus-0 -c prometheus --tail=100 | grep -iE "compact|wal|checkpoint"

# 3. TSDB 健康检查
kubectl exec -n monitoring prometheus-0 -- wget -qO- http://localhost:9090/api/v1/status/tsdb | jq '.data'

# 4. 查询验证
kubectl exec -n monitoring prometheus-0 -- wget -qO- 'http://localhost:9090/api/v1/query?query=up&time=2026-06-26T00:00:00Z' | jq '.data.result | length'

# 5. Grafana 大盘刷新后无断点
```

## 回复客户话术

> 您好，经排查，Prometheus 数据丢失与查询缓慢的根因是 **TSDB 存储空间不足，加上高基数指标导致索引膨胀**。当前 Prometheus PVC 100Gi 已使用 96Gi，compaction 因磁盘满失败，WAL 文件持续增长；同时部分业务指标带有 `user_id`、`request_id` 等无边界 label，查询时消耗大量内存与 CPU。我们已完成以下处置：
>
> 1. 将 Prometheus PVC 扩容至 300Gi，恢复 compaction 与 WAL 写入；
> 2. 调整保留策略为 10 天或 250GB，避免存储再次打满；
> 3. 提升 Prometheus 内存 limit 至 16Gi、CPU limit 至 8 核；
> 4. 在 ServiceMonitor 中增加 metricRelabelings，过滤高基数 label。
>
> 当前 Grafana 大盘已能正常加载近 7 天数据，查询超时问题明显改善。建议后续：
> - 建立指标治理规范，禁止在监控指标中使用用户级、请求级 label；
> - 评估引入 Thanos 或 VictoriaMetrics 实现监控数据长期存储与水平扩展；
> - 配置 Prometheus 存储使用率与查询延迟告警，提前发现容量瓶颈；
> - 每月审查一次抓取目标数量与 TSDB head series，及时调整资源与保留策略。
>
> 请继续观察，如有断点可及时反馈。

## 复盘与沉淀

Prometheus 数据丢失往往并非 Prometheus 本身崩溃，而是存储或资源瓶颈导致 TSDB 无法正常工作。本次案例表明，单纯提升副本数无法解决单实例 TSDB 的存储与查询瓶颈，需要从指标治理、存储扩容、架构升级三个维度综合处理。

关键经验：
1. **指标治理是基础**：高基数 label 是 TSDB 性能杀手，必须在应用层或 relabel 阶段进行过滤；
2. **保留策略要匹配存储容量**：保留时间与存储大小必须联动配置，否则必然有一天会打满；
3. **单 Prometheus 有上限**：当抓取目标超过 500 或日样本量超过 50GB 时，应考虑 Thanos、Cortex、VictoriaMetrics 等分布式方案；
4. **监控告警不能缺失**：必须对 Prometheus 自身的磁盘、内存、查询延迟、compaction 失败进行监控；
5. **定期审查 scrape 配置**：随着业务增长，ServiceMonitor 和 PodMonitor 数量会快速膨胀，需要定期清理无用指标源。

建议将以下告警纳入日常运维：
- Prometheus 磁盘使用率 > 80% 触发 P2；
- Prometheus 内存使用率 > 85% 触发 P2；
- Prometheus query 99th 延迟 > 5s 触发 P3；
- TSDB head series 超过阈值触发 P3；
- Compaction 失败次数 > 0 触发 P2。

同时，可参考 Prometheus 高可用架构 规划从单实例到 Thanos 的演进路线，避免监控平台成为排障瓶颈。在演进过程中，可以先按业务域或命名空间拆分 Prometheus 实例，再通过 Thanos Query 做统一查询，实现渐进式扩展。对于短期无法接入分布式方案的团队，至少应启用 Prometheus 的远程读写（remote_write）将关键指标发送到外部存储，确保核心监控数据不会因单实例故障而丢失。此外，建议为 Prometheus 配置专用的存储类，使用 SSD 云盘提升 TSDB 的写入与查询性能，降低磁盘 IO 成为瓶颈的概率。

## 是否需要升级及交接信息

- **是否升级**：已定位并优化，暂不需要升级；若 300Gi 存储在预期保留期内仍无法满足，或业务指标基数持续失控，需升级至 **可观测性架构团队**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-015`
  - 根因：`Prometheus PVC 存储不足 + 高基数指标导致 TSDB 性能下降`
  - 影响集群：`ack-zyy-prod-06`
  - 影响命名空间：`monitoring`
  - 临时修复：扩容 PVC、提升资源、过滤高基数 label
  - 长期方案：引入 Thanos/VictoriaMetrics，建立指标治理规范
  - 待跟进：确认 compaction 完成，评估 Thanos 接入计划

## Related

- Prometheus (entities)
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- Observability
- [[concepts/bp-observability.md|最佳实践：Observability]]
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- Observability
- [[concepts/bp-observability.md|最佳实践：Observability]]
