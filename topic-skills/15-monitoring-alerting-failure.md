---
title: 监控告警体系故障诊断与修复 / Monitoring & Alerting System Diagnosis & Remediation
description: '# 监控告警体系故障诊断与修复 / Monitoring & Alerting System Diagnosis & Remediation'
category: observability
tags:
- k8s
- skills
- sop
- runbook
- kubelet
- prometheus
- grafana
- helm
- statefulset
- job
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- 监控告警体系故障诊断与修复 / Monitoring & Alerting System Diagnosis & Remediation 是什么
- 如何 监控告警体系故障诊断与修复 / Monitoring & Alerting System Diagnosis & Remediation
trigger_keywords:
- prometheus down
- alertmanager not firing
- grafana dashboard empty
- metrics missing
- target down
- scrape failed
- alert storm
- notification failed
- thanos query error
- servicemonitor not working
- pushgateway stale
- recording rule error
- 监控不可用
- 告警不发送
- 指标丢失
- 仪表盘无数据
- 告警风暴
- 通知失败
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L1
skill_metadata:
  skill_id: SKILL-15
  category: observability
  subcategory: monitoring
  severity: P1
  time_to_diagnosis_minutes: 15
  time_to_remediation_minutes: 25
  escalation_required: false
  control_plane_impact: false
agent_notes:
  decision_tree_entry: "kubectl get pods -n monitoring -o wide 检查监控组件状态"
  critical_commands:
    - "kubectl get pods -n monitoring -o wide"
    - "kubectl logs -n monitoring -l app=prometheus --tail=100"
    - "kubectl get servicemonitor -A"
    - "kubectl get prometheus -A -o wide"
    - "kubectl exec -it prometheus-<pod> -n monitoring -- promtool tsdb analyze"
  danger_operations:
    - action: "kubectl delete pod -n monitoring -l app=prometheus --force"
      risk: "强制删除 Prometheus 会导致监控数据丢失，可能影响告警触发"
      requires_confirmation: true
---

<!-- condition: kubectl get pods -n monitoring -o jsonpath='{range .items[?(@.status.phase!="Running")]} {.metadata.name}{"\n"}{end}' 显示监控组件异常 -->

# 监控告警体系故障诊断与修复 / Monitoring & Alerting System Diagnosis & Remediation

---

## 1. 概述

监控告警体系是 Kubernetes 集群可观测性的核心基础设施。当 Prometheus、AlertManager、Grafana 或长期存储组件（Thanos/VictoriaMetrics/Cortex）出现故障时，会直接导致**监控盲区**——运维团队无法感知集群状态变化，业务故障无法及时发现和响应。这是**元故障**（monitoring the monitoring）场景，其严重性往往被低估。

### 典型触发场景

1. **Prometheus 采集故障**: Prometheus Pod OOM/CrashLoopBackOff、Target 大面积 Down、ServiceMonitor 配置未被发现、TSDB 存储空间耗尽
2. **AlertManager 通知故障**: 告警规则触发但通知未送达、告警路由配置错误、通知渠道（Webhook/Email/钉钉/Slack）不可达、告警被错误静默或抑制
3. **告警风暴**: 短时间内大量重复告警触发，导致通知渠道过载、值班人员疲劳、真正的关键告警被淹没
4. **Grafana Dashboard 异常**: 数据源连接失败、Dashboard 显示 "No Data"、查询超时
5. **长期存储故障**: Thanos Query/Store/Sidecar 通信故障、数据查询不完整或超时

### 前置条件

- **RBAC 权限**:
  - 最小权限: 对 monitoring namespace 内 `pods`, `pods/log`, `services`, `configmaps`, `secrets`, `deployments`, `statefulsets`, `persistentvolumeclaims`, `servicemonitors` (monitoring.coreos.com), `prometheusrules` (monitoring.coreos.com) 的 `get/list/watch`
  - 修复权限: `configmaps`, `deployments`, `statefulsets`, `servicemonitors`, `prometheusrules` 的 `patch/update`
  - 验证命令: `kubectl auth can-i list servicemonitors -n monitoring`
- **访问方式**: 能够访问 Prometheus/AlertManager/Grafana 的 Web UI 或 API
- **工具要求**: kubectl (v1.28+), curl, jq（可选但推荐）, promtool, amtool
- **监控栈部署**: 本 Skill 假设使用 kube-prometheus-stack（Prometheus Operator）部署模式

> ⚠️ **重要**: 监控系统故障属于**元故障**——当监控本身不可用时，其他故障可能无法被发现。P0 级监控故障应优先于其他 P1/P2 故障处理。

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| SP-01 | Prometheus Target 状态为 Down / Prometheus Target shows Down status | Prometheus `/targets` 页面显示特定 Target 为 `DOWN`；`up{job="xxx"} == 0` | 0.95 | Target 服务正在进行计划内维护或重启；新部署的服务尚未暴露 metrics 端点 |
| SP-02 | Grafana Dashboard 显示 "No Data" / Grafana Dashboard shows "No Data" | Grafana 面板显示 "No Data" 或空白图表；查询 API 返回空结果 | 0.85 | 查询时间范围选择错误（如选择了未来时间）；指标名称变更导致 Dashboard 查询失效 |
| SP-03 | AlertManager 不发送告警通知 / AlertManager not sending notifications | 告警在 Prometheus `/alerts` 页面处于 firing 状态，但 AlertManager `/#/alerts` 中未显示或通知渠道未收到 | 0.90 | 告警被手动静默（silence）；告警处于 pending 状态尚未达到 `for` 持续时间 |
| SP-04 | 告警风暴（短时间大量重复告警）/ Alert storm (massive repeated alerts) | AlertManager `alertmanager_alerts{state="active"}` 指标急剧上升；通知渠道收到大量相似告警 | 0.90 | 真实的大规模基础设施故障（如网络分区导致多节点 Down）；计划内批量操作触发预期告警 |
| SP-05 | Prometheus OOM/CrashLoopBackOff / Prometheus OOM or CrashLoopBackOff | `kubectl get pods -n monitoring` 显示 Prometheus Pod 状态为 OOMKilled/CrashLoopBackOff | 0.95 | 首次部署资源配置不足；高基数指标导致的预期内存增长 |
| SP-06 | ServiceMonitor 配置未被 Prometheus 发现 / ServiceMonitor not discovered by Prometheus | `kubectl get servicemonitor -A` 有记录但 Prometheus `/service-discovery` 或 `/config` 中未显示对应配置 | 0.85 | ServiceMonitor 刚创建，Prometheus 尚未重载配置；ServiceMonitor 位于 Prometheus Operator 未监控的 namespace |
| SP-07 | 自定义指标查询返回空结果 / Custom metrics query returns empty | PromQL 查询 `{__name__=~"custom_.*"}` 返回空；Grafana 中相关 Panel 无数据 | 0.80 | 指标名称/标签拼写错误；应用尚未产生该指标（如功能未被调用） |
| SP-08 | Thanos Query 超时或数据不完整 / Thanos Query timeout or incomplete data | Thanos Query UI 查询超时；Grafana 通过 Thanos 数据源查询时部分时间段无数据 | 0.85 | Thanos Store 正在执行 compaction；跨 Region 查询网络延迟高 |
| SP-09 | Pushgateway 指标过期/堆积 / Pushgateway stale or accumulated metrics | `push_time_seconds` 指标显示最后推送时间很久之前；Pushgateway 上存在大量已失效的 job 指标 | 0.80 | 批处理任务周期性运行，指标更新间隔较长；任务正常完成后未清理指标 |
| SP-10 | Recording Rule 评估失败 / Recording Rule evaluation failed | Prometheus `/rules` 页面显示规则状态为 `error`；`prometheus_rule_evaluation_failures_total` 指标上升 | 0.90 | 规则依赖的指标临时不可用；规则表达式引用了已删除的指标 |
| SP-11 | Grafana 数据源连接失败 / Grafana datasource connection failed | Grafana 数据源配置页面 "Test" 失败；Dashboard 显示 "Error" 而非 "No Data" | 0.90 | Prometheus/Thanos 服务正在重启；网络策略阻止 Grafana 访问数据源 |
| SP-12 | AlertManager 集群成员不一致 / AlertManager cluster members inconsistent | `amtool cluster show` 显示成员数量与预期不符；`alertmanager_cluster_members` 指标异常 | 0.85 | AlertManager Pod 正在滚动更新；集群正在扩容/缩容 |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "Prometheus 挂了，监控看不到数据"
- "告警不发了，钉钉/企微收不到通知"
- "Grafana 看板全是 No Data"
- "监控系统收到大量重复告警"
- "指标采集不到，Target 全是 Down"
- "Thanos 查历史数据超时"
- "新服务的 ServiceMonitor 没生效"
- "Recording Rule 报错"
- "AlertManager 三个副本只有一个在工作"

**English ticket descriptions**:
- "Prometheus is down, no metrics available"
- "AlertManager not sending alerts to Slack/PagerDuty"
- "Grafana dashboard shows No Data everywhere"
- "Receiving alert storm, thousands of notifications"
- "ServiceMonitor created but targets not showing up"
- "Thanos query timeout when querying historical data"
- "Custom metrics not being scraped"
- "Pushgateway metrics are stale"
- "Recording rules failing with evaluation errors"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| 监控系统正常，但特定应用的 metrics 端点未暴露 | 应用部署问题 | 需要应用侧添加 metrics 端点暴露，非监控系统故障 |
| Grafana 正常但 Dashboard JSON 导入失败 | Grafana 配置问题 | Dashboard 定义文件格式问题，非系统故障 |
| 告警规则逻辑不正确导致误报/漏报 | 告警规则优化 | 属于告警治理范畴，非技术故障 |
| Prometheus 指标存储策略导致的数据过期 | 存储策略配置 | 正常的数据生命周期管理 |
| 监控组件资源扩容需求（非故障） | 容量规划 | 容量不足但系统仍在运行，属于规划问题 |
| PromQL 查询语法错误 | 用户培训/文档 | 查询使用问题，非系统故障 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断故障爆炸半径：

**Step T1**: 检查监控核心组件健康状态 (15s)
```bash
# 检查监控命名空间下所有 Pod 状态
kubectl get pods -n monitoring --no-headers | awk '{print $3}' | sort | uniq -c
# 检查是否有核心组件异常
kubectl get pods -n monitoring | grep -E "prometheus|alertmanager|grafana" | grep -v "Running"
```
> **判断规则**:
> - Prometheus Pod 不在 Running 状态 → **P0**（监控核心不可用）
> - AlertManager Pod 不在 Running 状态 → **P1**（告警通知可能失败）
> - 仅 Grafana 异常 → **P2**（可视化受影响，但数据采集和告警正常）
> - 仅附属组件异常（如 kube-state-metrics） → **P2/P3**

**Step T2**: 检查 Prometheus Target 健康状态 (30s)
```bash
# 通过 API 获取 Target 状态（假设 Prometheus 可访问）
kubectl port-forward -n monitoring svc/prometheus-operated 9090:9090 &
sleep 2
curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets | group_by(.health) | map({health: .[0].health, count: length})'
# 或直接访问 Prometheus UI /targets 页面
```
> **判断规则**:
> - 所有 Target 均为 Down → **P0**（完全监控盲区）
> - >50% Target 为 Down → **P1**（大面积采集故障）
> - 部分关键服务 Target Down → **P2**
> - 仅个别非关键 Target Down → **P3**

**Step T3**: 检查 AlertManager 和告警触发状态 (60s)
```bash
# 检查 Prometheus 中的 firing alerts
curl -s http://localhost:9090/api/v1/alerts | jq '.data.alerts | length'
# 检查 AlertManager 中的 active alerts
kubectl port-forward -n monitoring svc/alertmanager-operated 9093:9093 &
sleep 2
curl -s http://localhost:9093/api/v2/alerts | jq 'length'
# 检查 AlertManager 集群状态
curl -s http://localhost:9093/api/v2/status | jq '.cluster'
```
> **判断规则**:
> - Prometheus 有 firing alerts 但 AlertManager 中未显示 → 告警路由/接收问题
> - AlertManager 集群成员数量异常 → 高可用受损
> - 大量 alerts (>100) 在短时间内触发 → 可能是告警风暴

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| Prometheus 核心组件不可用（OOM/Crash） | **P0** | 完全监控盲区，任何业务故障都无法被发现 | 立即响应，15min 内恢复采集能力 |
| AlertManager 完全不可用或通知渠道全部失败 | **P0** | 告警无法触达值班人员，可能错过关键故障 | 立即响应，15min 内恢复通知能力 |
| >50% Target Down 或多个关键服务无监控 | **P1** | 大面积监控盲区，部分业务故障可能被遗漏 | 15min 内响应，30min 内修复 |
| 告警风暴（短时间 >100 条重复告警） | **P1** | 通知过载可能导致关键告警被忽视 | 15min 内响应，实施降噪措施 |
| 部分 Target Down 或 Grafana 不可用 | **P2** | 监控能力部分受损，但核心功能正常 | 30min 内响应，2h 内修复 |
| 单个 ServiceMonitor 未生效/Recording Rule 失败 | **P3** | 局部功能异常，影响有限 | 4h 内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

- **完全监控盲区**: Prometheus 所有副本均不可用，且无法通过任何方式采集集群指标
- **告警通道全部失效**: AlertManager 可用但所有通知渠道（Webhook、Email、SMS、IM）均不可达
- **数据丢失风险**: Prometheus TSDB 损坏、WAL 损坏，存在数据不可恢复风险
- **安全事件**: 发现监控配置被恶意篡改、Prometheus 被未授权访问
- **级联故障**: 监控故障与其他 P0 业务故障同时发生，需要人工判断优先级

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 监控栈健康快检（只读，零风险）

> **目标**: 快速确认监控核心组件的基本健康状态
> **预计耗时**: 2-5 分钟

**Step D1.1**: 检查监控组件 Pod 状态
- **命令**:
  ```bash
  kubectl get pods -n monitoring -o wide
  ```
- **超时**: 10s
- **预期输出模式**: 所有 Pod 状态为 Running，READY 列显示完整副本数
- **判断规则**:
  - Prometheus Pod 状态为 `OOMKilled` → 根因为 RC-002（内存不足），跳转 Section 5
  - Prometheus Pod 状态为 `CrashLoopBackOff` → 继续 D1.2 检查日志
  - AlertManager Pod 异常 → 继续 D3.x 进行告警链路诊断
  - 所有 Pod 正常 → 继续 D1.2 检查内部状态
- **版本差异**: 无

**Step D1.2**: 检查 Prometheus 存活状态
- **命令**:
  ```bash
  # 端口转发或直接访问
  kubectl port-forward -n monitoring svc/prometheus-operated 9090:9090 &
  curl -s http://localhost:9090/-/healthy
  curl -s http://localhost:9090/-/ready
  ```
- **超时**: 10s
- **预期输出模式**: `Prometheus Server is Healthy` 和 `Prometheus Server is Ready`
- **判断规则**:
  - healthy 返回非 200 → Prometheus 内部异常，查看 D2.x 日志
  - ready 返回非 200 → Prometheus 正在加载数据或恢复中，等待或检查 TSDB
  - 连接被拒绝 → Pod 可能未正常启动或端口未暴露
- **版本差异**: 无

**Step D1.3**: 检查 AlertManager 存活状态
- **命令**:
  ```bash
  kubectl port-forward -n monitoring svc/alertmanager-operated 9093:9093 &
  curl -s http://localhost:9093/-/healthy
  curl -s http://localhost:9093/-/ready
  ```
- **超时**: 10s
- **预期输出模式**: 返回 200 OK
- **判断规则**:
  - 健康检查失败 → AlertManager 异常，继续 D3.x
  - 正常 → AlertManager 基本健康，如有通知问题，检查配置和路由
- **版本差异**: 无

**Step D1.4**: 检查 Grafana 存活状态
- **命令**:
  ```bash
  kubectl port-forward -n monitoring svc/grafana 3000:80 &
  curl -s http://localhost:3000/api/health
  ```
- **超时**: 10s
- **预期输出模式**: `{"commit":"...","database":"ok","version":"..."}`
- **判断规则**:
  - `database` 不是 `ok` → Grafana 数据库连接问题
  - 连接失败 → Grafana Pod 异常
  - 正常 → 如有 Dashboard 问题，检查数据源配置
- **版本差异**: 无

**Step D1.5**: 检查 Target 发现状态
- **命令**:
  ```bash
  # 获取所有 Target 状态
  curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | {job: .labels.job, health: .health, lastError: .lastError}' | head -50
  # 统计各状态数量
  curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets | group_by(.health) | map({health: .[0].health, count: length})'
  ```
- **超时**: 15s
- **预期输出模式**: 大部分 Target 的 health 为 "up"
- **判断规则**:
  - 特定 job 的所有 Target 为 down → 该服务的 metrics 端点问题或网络问题（RC-005）
  - 所有 Target 为 down → Prometheus 网络隔离或全局配置问题
  - lastError 包含 "connection refused" → 目标服务 metrics 端口未监听
  - lastError 包含 "context deadline exceeded" → 网络延迟或超时配置过小
- **版本差异**: 无

**Step D1.6**: 检查 Prometheus TSDB 存储状态
- **命令**:
  ```bash
  # 检查 TSDB 状态
  curl -s http://localhost:9090/api/v1/status/tsdb | jq '.data'
  # 检查 PVC 使用情况
  kubectl exec -n monitoring prometheus-prometheus-kube-prometheus-prometheus-0 -- df -h /prometheus
  ```
- **超时**: 15s
- **预期输出模式**: TSDB 状态正常，磁盘使用率低于 80%
- **判断规则**:
  - `headStats.numSeries` 异常高（>5M）→ 高基数指标问题（RC-008）
  - 磁盘使用率 >90% → 存储空间即将耗尽（RC-006）
  - `headStats.numLabelPairs` 异常高 → 标签爆炸，需要治理
- **版本差异**: 无

---

### Phase 2: Prometheus 深度诊断（只读，零风险）

> **目标**: 深入排查 Prometheus 采集、存储、规则相关问题
> **预计耗时**: 5-10 分钟

**Step D2.1**: 检查 ServiceMonitor 发现情况
- **命令**:
  ```bash
  # 获取所有 ServiceMonitor
  kubectl get servicemonitor -A -o custom-columns=NAMESPACE:.metadata.namespace,NAME:.metadata.name,SELECTOR:.spec.selector
  # 获取 Prometheus 实际加载的 scrape configs
  curl -s http://localhost:9090/api/v1/status/config | jq -r '.data.yaml' | grep -A 5 "job_name:"
  ```
- **超时**: 15s
- **预期输出模式**: ServiceMonitor 列表与 Prometheus scrape configs 对应
- **判断规则**:
  - ServiceMonitor 存在但 Prometheus 配置中无对应 job → RC-001（selector 不匹配）
  - 继续 D2.2 深入检查 selector
- **版本差异**: 无

**Step D2.2**: 检查 ServiceMonitor label selector 匹配
- **命令**:
  ```bash
  # 获取 Prometheus Operator 的 serviceMonitorSelector
  kubectl get prometheus -n monitoring -o jsonpath='{.items[*].spec.serviceMonitorSelector}'
  # 检查特定 ServiceMonitor 的 labels
  kubectl get servicemonitor <name> -n <namespace> -o jsonpath='{.metadata.labels}'
  # 验证 Service 的 labels 与 ServiceMonitor selector 是否匹配
  kubectl get svc <service-name> -n <namespace> -o jsonpath='{.metadata.labels}'
  ```
- **超时**: 15s
- **预期输出模式**: labels 匹配
- **判断规则**:
  - Prometheus 的 `serviceMonitorSelector` 要求特定 label（如 `release: prometheus`），但 ServiceMonitor 未设置 → RC-001
  - ServiceMonitor 的 selector 与 Service labels 不匹配 → RC-001
  - Namespace selector 限制了可发现的 namespace → 检查 `serviceMonitorNamespaceSelector`
- **版本差异**: 无

**Step D2.3**: 检查 Prometheus Scrape 配置
- **命令**:
  ```bash
  # 获取完整配置
  curl -s http://localhost:9090/api/v1/status/config | jq -r '.data.yaml' > /tmp/prometheus-config.yaml
  # 检查配置重载状态
  curl -s http://localhost:9090/api/v1/status/runtimeinfo | jq '.data.reloadConfigSuccess'
  # 检查最后一次配置重载时间
  curl -s http://localhost:9090/api/v1/status/runtimeinfo | jq '.data.lastConfigTime'
  ```
- **超时**: 15s
- **预期输出模式**: `reloadConfigSuccess` 为 true
- **判断规则**:
  - `reloadConfigSuccess` 为 false → 配置有语法错误或无效配置
  - 配置重载时间很久之前 → 新配置可能未生效，检查 Prometheus Operator 日志
- **版本差异**: 无

**Step D2.4**: 检查 TSDB 详细状态
- **命令**:
  ```bash
  # 获取 TSDB 详细状态
  curl -s http://localhost:9090/api/v1/status/tsdb | jq '{
    headSeries: .data.headStats.numSeries,
    headChunks: .data.headStats.numChunks,
    headMinTime: .data.headStats.minTime,
    headMaxTime: .data.headStats.maxTime,
    seriesCountByMetricName: .data.seriesCountByMetricName[:10]
  }'
  # 检查 WAL 状态
  curl -s http://localhost:9090/api/v1/status/walreplay
  ```
- **超时**: 15s
- **预期输出模式**: series 数量在合理范围内（通常 <3M）
- **判断规则**:
  - `numSeries` >5M → 高基数问题（RC-008），查看 `seriesCountByMetricName` 找出罪魁祸首
  - WAL replay 进行中 → Prometheus 正在恢复，需要等待
  - head 时间范围异常 → 可能有时钟问题或数据损坏
- **版本差异**: 无

**Step D2.5**: 检查 Prometheus 内存使用趋势
- **命令**:
  ```bash
  # 查询 Prometheus 自身的内存使用
  curl -s "http://localhost:9090/api/v1/query?query=process_resident_memory_bytes" | jq '.data.result[0].value[1]'
  # 查询内存使用趋势（最近 1 小时）
  curl -s "http://localhost:9090/api/v1/query_range?query=process_resident_memory_bytes&start=$(date -d '1 hour ago' +%s)&end=$(date +%s)&step=60" | jq '.data.result[0].values | .[-1][1] as $last | .[0][1] as $first | "Growth: \(($last|tonumber) - ($first|tonumber) | . / 1024 / 1024 | floor)MB"'
  ```
- **超时**: 15s
- **预期输出模式**: 内存使用稳定，无明显增长趋势
- **判断规则**:
  - 内存持续上涨 → 可能是高基数指标或 query 缓存膨胀
  - 内存接近 Pod limits → OOM 风险（RC-002）
- **版本差异**: 无

**Step D2.6**: 检查 Recording Rule 状态
- **命令**:
  ```bash
  # 获取所有规则状态
  curl -s http://localhost:9090/api/v1/rules | jq '.data.groups[] | {name: .name, rules: [.rules[] | select(.health != "ok") | {name: .name, health: .health, lastError: .lastError}]}'
  # 检查评估失败计数
  curl -s "http://localhost:9090/api/v1/query?query=prometheus_rule_evaluation_failures_total" | jq '.data.result'
  ```
- **超时**: 15s
- **预期输出模式**: 所有规则 health 为 "ok"
- **判断规则**:
  - 规则 health 为 "err" → RC-007（规则表达式错误），查看 lastError
  - 评估失败计数持续上升 → 规则依赖的指标可能缺失
- **版本差异**: 无

**Step D2.7**: 检查远程写入状态
- **命令**:
  ```bash
  # 检查远程写入配置
  curl -s http://localhost:9090/api/v1/status/config | jq -r '.data.yaml' | grep -A 10 "remote_write:"
  # 检查远程写入指标
  curl -s "http://localhost:9090/api/v1/query?query=prometheus_remote_storage_samples_failed_total" | jq '.data.result'
  curl -s "http://localhost:9090/api/v1/query?query=prometheus_remote_storage_samples_pending" | jq '.data.result'
  ```
- **超时**: 15s
- **预期输出模式**: failed 为 0 或很小，pending 不持续增长
- **判断规则**:
  - `samples_failed_total` 持续增加 → 远程存储写入失败，检查目标可达性
  - `samples_pending` 持续增长 → 写入速度跟不上采集速度，可能积压
- **版本差异**: 无

**Step D2.8**: 检查 Prometheus Operator 日志
- **命令**:
  ```bash
  kubectl logs -n monitoring deploy/prometheus-operator --tail=50 | grep -iE "error|warn|fail"
  ```
- **超时**: 15s
- **预期输出模式**: 无持续的 error 日志
- **判断规则**:
  - 包含 "failed to sync" → ServiceMonitor/PrometheusRule 同步失败
  - 包含 "invalid configuration" → CRD 配置错误
  - 包含 "secret not found" → 引用的 Secret 不存在（如 TLS 证书）
- **版本差异**: 无

---

### Phase 3: AlertManager 与通知链路诊断（只读，零风险）

> **目标**: 诊断告警路由和通知渠道问题
> **预计耗时**: 5-10 分钟

**Step D3.1**: 检查 AlertManager 路由配置
- **命令**:
  ```bash
  # 获取 AlertManager 配置
  kubectl get secret -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d
  # 使用 amtool 显示路由
  kubectl exec -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager-0 -- amtool config routes show
  ```
- **超时**: 15s
- **预期输出模式**: 路由配置正确，有 receiver 定义
- **判断规则**:
  - 无 receiver 定义 → 告警无处发送（RC-003）
  - 路由匹配规则过于宽泛/严格 → 可能导致告警被错误路由
  - `continue: false` 导致告警只发送到第一个匹配的 receiver → 可能遗漏通知
- **版本差异**: 无

**Step D3.2**: 检查告警抑制和静默规则
- **命令**:
  ```bash
  # 获取当前活跃的静默
  curl -s http://localhost:9093/api/v2/silences | jq '.[] | select(.status.state == "active") | {id: .id, matchers: .matchers, createdBy: .createdBy, endsAt: .endsAt}'
  # 检查抑制规则
  kubectl get secret -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d | grep -A 10 "inhibit_rules:"
  ```
- **超时**: 15s
- **预期输出模式**: 静默和抑制规则合理
- **判断规则**:
  - 存在过宽的静默（如 `alertname=~".*"`） → RC-012（静默规则过宽）
  - 抑制规则配置可能误抑制了需要的告警 → 检查 source/target matcher
- **版本差异**: 无

**Step D3.3**: 测试通知渠道连通性
- **命令**:
  ```bash
  # 从 AlertManager Pod 测试 Webhook 可达性（示例）
  kubectl exec -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager-0 -- wget -q --spider --timeout=5 <webhook-url> && echo "Webhook reachable" || echo "Webhook unreachable"
  # 检查通知失败指标
  curl -s "http://localhost:9093/api/v1/alerts" | jq '. | length'
  curl -s "http://localhost:9090/api/v1/query?query=alertmanager_notifications_failed_total" | jq '.data.result'
  ```
- **超时**: 30s
- **预期输出模式**: Webhook 可达，failed_total 为 0 或很小
- **判断规则**:
  - Webhook 不可达 → RC-004（通知渠道不可达），检查网络和目标服务
  - `notifications_failed_total` 持续上升 → 通知发送失败，检查具体 receiver 日志
- **版本差异**: 无

**Step D3.4**: 检查 AlertManager 集群状态
- **命令**:
  ```bash
  # 检查集群成员
  kubectl exec -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager-0 -- amtool cluster show
  # 检查集群指标
  curl -s "http://localhost:9093/api/v1/status" | jq '.data.clusterStatus'
  ```
- **超时**: 15s
- **预期输出模式**: 所有成员状态正常
- **判断规则**:
  - 成员数量少于预期副本数 → RC-012（集群成员不一致）
  - 某成员状态异常 → 检查该 Pod 的网络和健康状态
- **版本差异**: 无

**Step D3.5**: 分析告警分组和通知统计
- **命令**:
  ```bash
  # 查看通知统计
  curl -s "http://localhost:9090/api/v1/query?query=alertmanager_notifications_total" | jq '.data.result'
  curl -s "http://localhost:9090/api/v1/query?query=alertmanager_notifications_failed_total" | jq '.data.result'
  # 计算失败率
  curl -s "http://localhost:9090/api/v1/query?query=rate(alertmanager_notifications_failed_total[5m])/rate(alertmanager_notifications_total[5m])" | jq '.data.result'
  ```
- **超时**: 15s
- **预期输出模式**: 失败率接近 0
- **判断规则**:
  - 失败率 >10% → 通知渠道有问题
  - 特定 integration 失败率高 → 该渠道配置或可达性问题
- **版本差异**: 无

**Step D3.6**: Thanos/VictoriaMetrics 查询路径诊断
- **命令**:
  ```bash
  # 检查 Thanos Sidecar 状态（如果使用 Thanos）
  kubectl get pods -n monitoring -l app.kubernetes.io/name=thanos-sidecar
  # 检查 Thanos Query 状态
  kubectl get pods -n monitoring -l app.kubernetes.io/name=thanos-query
  # 测试 Thanos Query 连通性
  kubectl port-forward -n monitoring svc/thanos-query 10902:10902 &
  curl -s http://localhost:10902/-/healthy
  # 检查 Store 端点
  curl -s http://localhost:10902/api/v1/stores | jq '.data'
  ```
- **超时**: 30s
- **预期输出模式**: 所有组件健康，Store 端点可见
- **判断规则**:
  - Sidecar 不健康 → 检查与 Prometheus 的连接
  - Query 无法发现 Store → RC-009（Thanos 通信故障）
  - Store 数量少于预期 → 部分存储节点不可达
- **版本差异**: 无

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | 风险等级 |
|--------|------|------|---------|---------|
| RC-001 | **ServiceMonitor label selector 不匹配** — Prometheus Operator 的 serviceMonitorSelector 与 ServiceMonitor 的 labels 不匹配，或 ServiceMonitor 的 selector 与目标 Service 的 labels 不匹配，导致 Target 未被发现 | 高 (~20%) | D2.1、D2.2 显示 selector 不匹配 | 🟢 |
| RC-002 | **Prometheus 内存不足/OOM** — Prometheus 因高基数指标、大量 Target 或复杂查询导致内存使用超过限制，触发 OOMKilled | 高 (~15%) | D1.1 显示 OOMKilled；D2.5 内存持续增长；D2.4 series 数量过高 | 🟡 |
| RC-003 | **AlertManager 路由配置错误** — AlertManager 配置中路由规则不正确，导致告警无法匹配到正确的 receiver，或 receiver 未定义 | 中 (~12%) | D3.1 显示路由配置问题；告警在 Prometheus firing 但 AlertManager 无通知 | 🟢 |
| RC-004 | **通知渠道配置错误/不可达** — Webhook URL 错误、认证信息过期、目标服务不可达，导致通知发送失败 | 中 (~10%) | D3.3 通知测试失败；D3.5 failed_total 上升 | 🟡 |
| RC-005 | **Target 网络不可达/端口错误** — 被监控服务的 metrics 端口未开放、网络策略阻止访问、Service selector 不正确 | 中 (~8%) | D1.5 Target 状态 Down；lastError 包含 connection refused/timeout | 🟡 |
| RC-006 | **TSDB 存储空间耗尽** — Prometheus 的 PVC 存储空间不足，无法写入新数据，导致采集失败或数据丢失 | 中 (~7%) | D1.6 磁盘使用率 >90%；Prometheus 日志包含 "no space left on device" | 🔴 |
| RC-007 | **告警规则表达式语法错误** — PrometheusRule 中的 PromQL 表达式语法错误或引用了不存在的指标 | 低 (~6%) | D2.6 规则 health 为 err；lastError 显示具体错误 | 🟢 |
| RC-008 | **高基数指标导致性能劣化** — 指标 labels 值过多（如用户 ID、请求 ID 作为 label），导致 series 数量爆炸，内存和存储消耗剧增 | 低 (~5%) | D2.4 series 数量 >5M；seriesCountByMetricName 显示问题指标 | 🟡 |
| RC-009 | **Thanos Sidecar/Store 通信故障** — Thanos 组件之间网络不通、gRPC 超时、证书问题，导致历史数据查询失败 | 低 (~5%) | D3.6 显示 Store 不可达；Thanos Query 查询超时 | 🟡 |
| RC-010 | **Grafana 数据源认证/配置错误** — Grafana 数据源的 URL、认证信息配置错误，或 Prometheus/Thanos 服务发生变更 | 低 (~4%) | D1.4 Grafana 健康但 Dashboard 报错；数据源测试失败 | 🟢 |
| RC-011 | **Pushgateway 过期指标堆积** — Pushgateway 上存在大量已完成任务的陈旧指标，占用资源且可能导致误导性监控数据 | 低 (~4%) | D1.5 Pushgateway 相关 Target；push_time_seconds 很久未更新 | 🟢 |
| RC-012 | **告警抑制/静默规则过宽** — AlertManager 中配置了过于宽泛的静默或抑制规则，导致合法告警被错误屏蔽 | 低 (~4%) | D3.2 显示宽泛的静默；告警在 Prometheus firing 但无通知 | 🟢 |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可建议自动执行）

#### REM-001: 修复 ServiceMonitor label selector
- **适用根因**: RC-001
- **前置检查**:
  ```bash
  # 获取 Prometheus 的 serviceMonitorSelector
  kubectl get prometheus -n monitoring -o jsonpath='{.items[*].spec.serviceMonitorSelector}' | jq .
  # 获取问题 ServiceMonitor 的 labels
  kubectl get servicemonitor <name> -n <namespace> -o jsonpath='{.metadata.labels}'
  ```
- **执行命令**:
  ```bash
  # 方案 A: 为 ServiceMonitor 添加缺失的 label
  kubectl label servicemonitor <name> -n <namespace> release=prometheus
  
  # 方案 B: 修改 Prometheus 的 serviceMonitorSelector 以匹配更多 ServiceMonitor
  # 需要编辑 Prometheus CRD，通常通过 Helm values 或直接编辑
  kubectl edit prometheus -n monitoring prometheus-kube-prometheus-prometheus
  # 将 serviceMonitorSelector: {} 设置为空以匹配所有 ServiceMonitor
  ```
- **后置验证**:
  ```bash
  # 等待配置重载
  sleep 30
  # 检查 Target 是否出现
  curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | select(.labels.job == "<expected-job>")'
  ```
- **回滚命令**:
  ```bash
  kubectl label servicemonitor <name> -n <namespace> release-
  ```

#### REM-002: 修正 AlertManager 路由配置
- **适用根因**: RC-003
- **前置检查**:
  ```bash
  # 获取当前配置
  kubectl get secret -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d
  # 使用 amtool 检查配置语法
  kubectl exec -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager-0 -- amtool check-config /etc/alertmanager/config/alertmanager.yaml
  ```
- **执行命令**:
  ```bash
  # 创建修正后的配置文件
  cat <<EOF > /tmp/alertmanager-fixed.yaml
  global:
    resolve_timeout: 5m
  route:
    receiver: 'default-receiver'
    group_by: ['alertname', 'namespace']
    group_wait: 30s
    group_interval: 5m
    repeat_interval: 4h
    routes:
    - match:
        severity: critical
      receiver: 'critical-receiver'
  receivers:
  - name: 'default-receiver'
    webhook_configs:
    - url: '<your-webhook-url>'
  - name: 'critical-receiver'
    webhook_configs:
    - url: '<your-critical-webhook-url>'
  EOF
  
  # 更新 Secret
  kubectl create secret generic alertmanager-prometheus-kube-prometheus-alertmanager \
    --from-file=alertmanager.yaml=/tmp/alertmanager-fixed.yaml \
    -n monitoring \
    --dry-run=client -o yaml | kubectl apply -f -
  ```
- **后置验证**:
  ```bash
  # 等待 AlertManager 重载配置
  sleep 30
  # 检查配置重载状态
  curl -s http://localhost:9093/api/v2/status | jq '.config'
  # 使用 amtool 验证路由
  kubectl exec -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager-0 -- amtool config routes show
  ```
- **回滚命令**:
  ```bash
  # 恢复原配置（假设已备份）
  kubectl create secret generic alertmanager-prometheus-kube-prometheus-alertmanager \
    --from-file=alertmanager.yaml=/tmp/alertmanager-backup.yaml \
    -n monitoring \
    --dry-run=client -o yaml | kubectl apply -f -
  ```

#### REM-003: 修复告警规则表达式
- **适用根因**: RC-007
- **前置检查**:
  ```bash
  # 获取失败的规则
  curl -s http://localhost:9090/api/v1/rules | jq '.data.groups[] | .rules[] | select(.health == "err") | {name: .name, lastError: .lastError}'
  # 获取 PrometheusRule 资源
  kubectl get prometheusrule -A
  ```
- **执行命令**:
  ```bash
  # 编辑有问题的 PrometheusRule
  kubectl edit prometheusrule <name> -n <namespace>
  # 修正 PromQL 表达式语法
  # 使用 promtool 本地验证
  promtool check rules /path/to/rules.yaml
  ```
- **后置验证**:
  ```bash
  # 等待规则重载
  sleep 30
  # 检查规则状态
  curl -s http://localhost:9090/api/v1/rules | jq '.data.groups[] | .rules[] | select(.name == "<rule-name>") | {name: .name, health: .health}'
  ```
- **回滚命令**:
  ```bash
  # 恢复原 PrometheusRule
  kubectl apply -f /tmp/prometheusrule-backup.yaml
  ```

#### REM-004: 清理 Pushgateway 过期指标
- **适用根因**: RC-011
- **前置检查**:
  ```bash
  # 获取 Pushgateway 上的所有 job
  curl -s http://<pushgateway>:9091/api/v1/metrics | grep "job="
  # 检查各 job 的最后推送时间
  curl -s "http://localhost:9090/api/v1/query?query=push_time_seconds" | jq '.data.result[] | {job: .metric.job, last_push: (.value[1] | tonumber | . - now | . / 3600 | floor | tostring + " hours ago")}'
  ```
- **执行命令**:
  ```bash
  # 删除特定 job 的过期指标
  curl -X DELETE "http://<pushgateway>:9091/metrics/job/<job-name>"
  # 或删除特定 job+instance 组合
  curl -X DELETE "http://<pushgateway>:9091/metrics/job/<job-name>/instance/<instance>"
  ```
- **后置验证**:
  ```bash
  # 确认指标已删除
  curl -s http://<pushgateway>:9091/api/v1/metrics | grep "<job-name>" || echo "Metrics cleaned"
  ```
- **回滚命令**:
  ```bash
  # Pushgateway 指标删除后无法回滚
  # 需要相关任务重新推送指标
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-005: Prometheus 资源扩容与存储优化
- **适用根因**: RC-002, RC-006
- **影响说明**: 修改 Prometheus 资源配置需要 Pod 重启，期间会有短暂的指标采集中断。增加存储可能需要 PVC 扩容（取决于 StorageClass 是否支持）。
- **审批提示**: "建议将 Prometheus 内存限制从 `{current}` 增加到 `{target}`，并扩容存储。Prometheus Pod 将重启，预计中断 2-5 分钟。是否批准？"
- **前置检查**:
  ```bash
  # 获取当前资源配置
  kubectl get prometheus -n monitoring -o jsonpath='{.items[*].spec.resources}'
  # 检查当前使用量
  kubectl top pod -n monitoring -l app.kubernetes.io/name=prometheus
  # 检查 PVC 大小
  kubectl get pvc -n monitoring -l app.kubernetes.io/name=prometheus
  ```
- **执行命令**:
  ```bash
  # 方案 A: 通过 Helm upgrade 增加资源（推荐）
  helm upgrade prometheus-stack prometheus-community/kube-prometheus-stack \
    --namespace monitoring \
    --set prometheus.prometheusSpec.resources.requests.memory=4Gi \
    --set prometheus.prometheusSpec.resources.limits.memory=8Gi \
    --set prometheus.prometheusSpec.storageSpec.volumeClaimTemplate.spec.resources.requests.storage=100Gi \
    --reuse-values
  
  # 方案 B: 直接编辑 Prometheus CRD
  kubectl patch prometheus -n monitoring prometheus-kube-prometheus-prometheus --type='merge' -p '
  {
    "spec": {
      "resources": {
        "requests": {"memory": "4Gi"},
        "limits": {"memory": "8Gi"}
      }
    }
  }'
  ```
- **后置验证**:
  ```bash
  # 等待 Pod 重启完成
  kubectl rollout status statefulset/prometheus-prometheus-kube-prometheus-prometheus -n monitoring --timeout=300s
  # 验证资源配置
  kubectl get pod -n monitoring -l app.kubernetes.io/name=prometheus -o jsonpath='{.items[*].spec.containers[*].resources}'
  # 验证 Prometheus 健康
  curl -s http://localhost:9090/-/healthy
  ```
- **回滚命令**:
  ```bash
  # 回滚到之前的配置
  helm rollback prometheus-stack -n monitoring
  ```

#### REM-006: 修复通知渠道配置
- **适用根因**: RC-004
- **影响说明**: 修改 AlertManager 配置会触发配置热重载，通常不需要重启 Pod。但配置错误可能导致所有通知中断。
- **审批提示**: "即将更新 AlertManager 的通知渠道配置。配置将热重载，如有错误可能影响告警通知。是否批准？"
- **前置检查**:
  ```bash
  # 测试目标 Webhook 可达性
  curl -X POST <webhook-url> -d '{"text": "test"}' -H "Content-Type: application/json" -v
  # 获取当前配置
  kubectl get secret -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d > /tmp/alertmanager-backup.yaml
  ```
- **执行命令**:
  ```bash
  # 更新 Webhook URL 或认证信息
  # 使用 alertmanager.yaml 模板创建新配置
  cat <<EOF > /tmp/alertmanager-new.yaml
  global:
    resolve_timeout: 5m
  route:
    receiver: 'default'
    group_by: ['alertname']
  receivers:
  - name: 'default'
    webhook_configs:
    - url: '<new-webhook-url>'
      send_resolved: true
  EOF
  
  # 验证配置语法
  kubectl create secret generic alertmanager-test \
    --from-file=alertmanager.yaml=/tmp/alertmanager-new.yaml \
    -n monitoring --dry-run=client -o yaml | kubectl apply -f - --dry-run=client
  
  # 应用配置
  kubectl create secret generic alertmanager-prometheus-kube-prometheus-alertmanager \
    --from-file=alertmanager.yaml=/tmp/alertmanager-new.yaml \
    -n monitoring \
    --dry-run=client -o yaml | kubectl apply -f -
  ```
- **后置验证**:
  ```bash
  # 等待配置重载
  sleep 30
  # 验证配置已加载
  curl -s http://localhost:9093/api/v2/status | jq '.config.original'
  # 测试告警发送（如果有测试告警）
  kubectl exec -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager-0 -- \
    amtool alert add test-alert severity=info
  ```
- **回滚命令**:
  ```bash
  kubectl create secret generic alertmanager-prometheus-kube-prometheus-alertmanager \
    --from-file=alertmanager.yaml=/tmp/alertmanager-backup.yaml \
    -n monitoring \
    --dry-run=client -o yaml | kubectl apply -f -
  ```

#### REM-007: 高基数指标治理（relabeling/drop）
- **适用根因**: RC-008
- **影响说明**: 添加 metric_relabel_configs 会导致部分指标被丢弃。确保不会影响关键业务监控。修改后需要 Prometheus 重载配置。
- **审批提示**: "即将配置指标丢弃规则，以下指标将不再被采集: `{metrics_to_drop}`。是否批准？"
- **前置检查**:
  ```bash
  # 分析高基数指标
  curl -s http://localhost:9090/api/v1/status/tsdb | jq '.data.seriesCountByMetricName[:20]'
  # 查看问题指标的 label 分布
  curl -s "http://localhost:9090/api/v1/query?query=count({__name__='<high_cardinality_metric>'}) by (label_name)" | jq '.data.result'
  ```
- **执行命令**:
  ```bash
  # 方案 A: 在应用侧修复（推荐，需要应用团队配合）
  # 减少高基数 label 的使用
  
  # 方案 B: 在 Prometheus 侧丢弃或 relabel
  # 编辑 Prometheus CRD 或创建 PodMonitor/ServiceMonitor 的 metricRelabelings
  kubectl edit servicemonitor <name> -n <namespace>
  # 添加以下配置:
  # spec:
  #   endpoints:
  #   - metricRelabelings:
  #     - sourceLabels: [__name__]
  #       regex: "high_cardinality_metric.*"
  #       action: drop
  #     - sourceLabels: [high_cardinality_label]
  #       action: labeldrop
  
  # 方案 C: 全局配置（通过 additionalScrapeConfigs）
  helm upgrade prometheus-stack prometheus-community/kube-prometheus-stack \
    --namespace monitoring \
    --set prometheus.prometheusSpec.additionalScrapeConfigs[0].job_name=<job> \
    --set prometheus.prometheusSpec.additionalScrapeConfigs[0].metric_relabel_configs[0].source_labels=\[__name__\] \
    --set prometheus.prometheusSpec.additionalScrapeConfigs[0].metric_relabel_configs[0].regex="drop_me.*" \
    --set prometheus.prometheusSpec.additionalScrapeConfigs[0].metric_relabel_configs[0].action=drop \
    --reuse-values
  ```
- **后置验证**:
  ```bash
  # 等待配置重载
  sleep 60
  # 验证 series 数量下降
  curl -s http://localhost:9090/api/v1/status/tsdb | jq '.data.headStats.numSeries'
  # 验证指标已被丢弃
  curl -s "http://localhost:9090/api/v1/query?query=count({__name__='<dropped_metric>'})" | jq '.data.result'
  ```
- **回滚命令**:
  ```bash
  # 移除 metricRelabelings 配置
  kubectl edit servicemonitor <name> -n <namespace>
  # 或回滚 Helm release
  helm rollback prometheus-stack -n monitoring
  ```

#### REM-008: 修复 Thanos 组件通信
- **适用根因**: RC-009
- **影响说明**: 修复 Thanos 组件可能需要重启相关 Pod，期间历史数据查询可能不可用或不完整。
- **审批提示**: "即将重启 Thanos 组件以修复通信问题。历史数据查询可能短暂不可用。是否批准？"
- **前置检查**:
  ```bash
  # 检查 Thanos 组件状态
  kubectl get pods -n monitoring -l app.kubernetes.io/name=thanos
  # 检查 Sidecar 与 Prometheus 的连接
  kubectl logs -n monitoring -l app.kubernetes.io/name=thanos-sidecar --tail=50 | grep -iE "error|fail"
  # 检查 Store 与 Query 的通信
  curl -s http://localhost:10902/api/v1/stores | jq '.data'
  ```
- **执行命令**:
  ```bash
  # 方案 A: 重启 Thanos Sidecar
  kubectl rollout restart statefulset/prometheus-prometheus-kube-prometheus-prometheus -n monitoring
  
  # 方案 B: 重启 Thanos Query
  kubectl rollout restart deployment/thanos-query -n monitoring
  
  # 方案 C: 检查并修复网络策略
  kubectl get networkpolicy -n monitoring
  # 确保 Thanos 组件之间的 gRPC 端口（10901）互通
  
  # 方案 D: 检查并修复 Service 配置
  kubectl get svc -n monitoring -l app.kubernetes.io/name=thanos
  ```
- **后置验证**:
  ```bash
  # 等待 Pod 就绪
  kubectl wait --for=condition=ready pod -l app.kubernetes.io/name=thanos-query -n monitoring --timeout=300s
  # 验证 Store 发现
  curl -s http://localhost:10902/api/v1/stores | jq '.data | length'
  # 测试历史数据查询
  curl -s "http://localhost:10902/api/v1/query?query=up&time=$(date -d '7 days ago' +%s)" | jq '.data.result | length'
  ```
- **回滚命令**:
  ```bash
  # Thanos 组件重启通常不需要回滚
  # 如有配置变更，使用 helm rollback
  helm rollback thanos -n monitoring
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-009: Prometheus TSDB 数据修复/重建
- **适用根因**: RC-006（存储损坏）
- **影响说明**: TSDB 修复或重建可能导致数据丢失。在重建过程中，Prometheus 将无法采集新数据。这是**数据破坏性操作**，需要充分评估影响。
- **操作步骤**:
  1. **评估数据损坏程度**:
     ```bash
     # 检查 TSDB 块状态
     kubectl exec -n monitoring prometheus-prometheus-kube-prometheus-prometheus-0 -- ls -la /prometheus/
     # 检查 WAL 目录
     kubectl exec -n monitoring prometheus-prometheus-kube-prometheus-prometheus-0 -- ls -la /prometheus/wal/
     ```
  2. **备份现有数据（如果可能）**:
     ```bash
     # 创建数据快照（如果 TSDB 仍可访问）
     curl -X POST http://localhost:9090/api/v1/admin/tsdb/snapshot
     # 快照将保存在 /prometheus/snapshots/
     ```
  3. **尝试 TSDB 修复**:
     ```bash
     # 停止 Prometheus（需要修改副本数为 0 或删除 Pod）
     kubectl scale statefulset/prometheus-prometheus-kube-prometheus-prometheus --replicas=0 -n monitoring
     
     # 使用 promtool 修复 TSDB（需要在 Pod 内或挂载 PVC）
     # promtool tsdb analyze /prometheus
     # promtool tsdb list /prometheus
     
     # 如果 WAL 损坏，可以删除 WAL 目录（会丢失最近 2 小时数据）
     # rm -rf /prometheus/wal/*
     ```
  4. **重建 TSDB（最后手段）**:
     ```bash
     # 删除所有数据，从零开始
     # kubectl exec -n monitoring prometheus-prometheus-kube-prometheus-prometheus-0 -- rm -rf /prometheus/*
     # 注意：这将丢失所有历史数据
     ```
  5. **恢复 Prometheus**:
     ```bash
     kubectl scale statefulset/prometheus-prometheus-kube-prometheus-prometheus --replicas=1 -n monitoring
     ```
- **安全检查**:
  - 确认 Thanos 或其他远程存储有数据备份
  - 与业务方确认数据丢失的可接受范围
  - 记录数据丢失的时间范围用于事后报告
- **回滚方案**:
  ```bash
  # TSDB 删除后无法回滚
  # 如有快照，可尝试从快照恢复
  # cp -r /prometheus/snapshots/<snapshot-id>/* /prometheus/
  ```

#### REM-010: AlertManager 集群修复
- **适用根因**: RC-012（集群成员不一致）
- **影响说明**: AlertManager 集群修复可能涉及 Pod 重建、数据同步。在修复期间，告警去重和静默功能可能不稳定。
- **操作步骤**:
  1. **诊断集群状态**:
     ```bash
     # 检查各成员状态
     for i in 0 1 2; do
       echo "=== alertmanager-$i ==="
       kubectl exec -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager-$i -- amtool cluster show
     done
     ```
  2. **检查网络连通性**:
     ```bash
     # 确保成员之间可以通信
     kubectl exec -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager-0 -- \
       nc -zv alertmanager-prometheus-kube-prometheus-alertmanager-1.alertmanager-operated 9094
     ```
  3. **尝试重建问题成员**:
     ```bash
     # 删除状态异常的 Pod，让 StatefulSet 重建
     kubectl delete pod alertmanager-prometheus-kube-prometheus-alertmanager-<N> -n monitoring
     # 等待重建
     kubectl wait --for=condition=ready pod/alertmanager-prometheus-kube-prometheus-alertmanager-<N> -n monitoring --timeout=300s
     ```
  4. **如果集群仍不健康，考虑完全重建**:
     ```bash
     # 删除所有 AlertManager Pod
     kubectl delete pod -l app.kubernetes.io/name=alertmanager -n monitoring
     # 等待 StatefulSet 重建所有 Pod
     kubectl rollout status statefulset/alertmanager-prometheus-kube-prometheus-alertmanager -n monitoring --timeout=300s
     ```
- **安全检查**:
  - 确保有至少一个 AlertManager 实例可用
  - 记录当前的静默规则（会在重建后丢失）
  - 通知值班人员在修复期间密切关注告警
- **回滚方案**:
  ```bash
  # AlertManager 集群修复通常不需要回滚
  # 如有静默规则需要恢复，使用 amtool 重新创建
  amtool silence add --alertname="<alert>" --duration=1h --comment="重建后恢复"
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-011: 监控栈完全重建
- **适用根因**: 多个根因同时存在、配置严重混乱、无法定位具体问题
- **审批要求**: 需要高级 SRE + 平台 Team Lead 审批
- **数据备份**: 
  - 备份所有 PrometheusRule、ServiceMonitor、PodMonitor 资源
  - 备份 AlertManager 配置和静默规则
  - 备份 Grafana Dashboard 和数据源配置
  - 导出 Prometheus TSDB 快照（如可能）
- **操作步骤**:
  1. **完整备份**:
     ```bash
     # 备份所有 CRD 资源
     kubectl get prometheusrule -A -o yaml > prometheusrules-backup.yaml
     kubectl get servicemonitor -A -o yaml > servicemonitors-backup.yaml
     kubectl get podmonitor -A -o yaml > podmonitors-backup.yaml
     kubectl get alertmanagerconfig -A -o yaml > alertmanagerconfigs-backup.yaml
     
     # 备份 Secret
     kubectl get secret -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager -o yaml > alertmanager-secret-backup.yaml
     
     # 备份 Grafana
     kubectl exec -n monitoring deploy/grafana -- grafana-cli admin data-migration export > grafana-backup.json
     ```
  2. **卸载现有监控栈**:
     ```bash
     helm uninstall prometheus-stack -n monitoring
     # 等待资源清理
     kubectl delete pvc -l app.kubernetes.io/name=prometheus -n monitoring
     kubectl delete pvc -l app.kubernetes.io/name=alertmanager -n monitoring
     ```
  3. **重新安装监控栈**:
     ```bash
     helm repo update
     helm install prometheus-stack prometheus-community/kube-prometheus-stack \
       --namespace monitoring \
       --create-namespace \
       --values /path/to/custom-values.yaml
     ```
  4. **恢复配置**:
     ```bash
     kubectl apply -f prometheusrules-backup.yaml
     kubectl apply -f servicemonitors-backup.yaml
     kubectl apply -f alertmanager-secret-backup.yaml
     ```
  5. **验证**:
     ```bash
     kubectl get pods -n monitoring
     curl -s http://localhost:9090/-/healthy
     curl -s http://localhost:9093/-/healthy
     ```
- **回滚方案**:
  ```bash
  # 如果重建失败，使用备份恢复
  # 或恢复到之前的 Helm release（如有）
  helm rollback prometheus-stack -n monitoring
  ```

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

```bash
# V1: 确认 Prometheus 核心健康
curl -s http://localhost:9090/-/healthy
# 预期: Prometheus Server is Healthy

# V2: 确认 Target 采集恢复
curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets | group_by(.health) | map({health: .[0].health, count: length})'
# 预期: 大部分 Target 为 "up"

# V3: 确认 AlertManager 健康
curl -s http://localhost:9093/-/healthy
# 预期: 返回 200

# V4: 确认配置重载成功
curl -s http://localhost:9090/api/v1/status/runtimeinfo | jq '.data.reloadConfigSuccess'
# 预期: true

# V5: 确认关键指标可查询
curl -s "http://localhost:9090/api/v1/query?query=up" | jq '.data.result | length'
# 预期: >0

# V6: 确认 Grafana 数据源连接正常
curl -s http://localhost:3000/api/health
# 预期: database: ok
```

### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| Prometheus 内存使用 | `process_resident_memory_bytes{job="prometheus"}` | 稳定，无持续增长 | 接近 limits 或持续快速增长 |
| Target 健康率 | `count(up == 1) / count(up) * 100` | >95% | <80% |
| Scrape 成功率 | `rate(prometheus_target_scrape_pools_failed_total[5m])` | 0 或接近 0 | >1/min |
| AlertManager 通知成功率 | `rate(alertmanager_notifications_total[5m]) - rate(alertmanager_notifications_failed_total[5m])` | >0 | failed 持续上升 |
| Recording Rule 评估 | `prometheus_rule_evaluation_failures_total` | 不增长 | 持续增长 |
| TSDB 压缩 | `prometheus_tsdb_compactions_failed_total` | 不增长 | 持续增长 |
| 磁盘使用 | `kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes` | <85% | >90% |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认故障已解决：

- [ ] Prometheus 所有副本均为 Running 状态，且持续稳定 >5 分钟
- [ ] AlertManager 集群成员数量正确，状态一致
- [ ] Target 健康率 >95%，无关键服务 Down
- [ ] 告警通知测试成功送达各渠道
- [ ] Grafana Dashboard 显示数据正常
- [ ] 最近 5 分钟无 Warning/Error 级别事件
- [ ] 内存和存储使用在安全水位
- [ ] Recording Rule 和 Alerting Rule 评估正常

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| Prometheus 内存趋势 | `process_resident_memory_bytes` 24h 趋势 | 每 4 小时 | 持续上涨 → 排查高基数指标 |
| TSDB 存储增长 | `prometheus_tsdb_head_series` 趋势 | 每日 | 异常增长 → 实施 REM-007 |
| 通知失败率 | `alertmanager_notifications_failed_total` | 每小时 | 失败率上升 → 检查通知渠道 |
| Target 健康度 | `up` 聚合查询 | 持续 | 关键服务 Down → 重新诊断 |
| Pod 重启次数 | `kube_pod_container_status_restarts_total{namespace="monitoring"}` | 每 4 小时 | 频繁重启 → 深入排查 |
| 配置重载状态 | `prometheus_config_last_reload_successful` | 每小时 | 为 0 → 检查配置有效性 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **15 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后验证失败 |
| **严重性升级** | 初始分级为 P2 但影响面扩大（如 Prometheus 从部分异常变为完全不可用） | 诊断过程中症状恶化 |
| **未知根因** | 完成 Phase 1-3 所有诊断步骤但无法匹配任何已知根因 | 所有诊断步骤均无明确异常发现 |
| **数据丢失风险** | 诊断发现 TSDB 损坏或存储即将耗尽，存在不可逆数据丢失风险 | D1.6 或 D2.4 发现严重异常 |
| **级联故障** | 监控故障与其他 P0 故障同时发生，需要人工判断优先级 | 多个高严重性告警同时触发 |

### 8.2 升级消息模板

```
【{severity}】监控告警体系故障诊断与修复 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 故障概述: {component}({instance}) 状态异常，持续 {duration}
- 影响范围:
  - 受影响组件: {affected_components}
  - Target 健康率: {target_health_rate}%
  - 告警通知状态: {notification_status}
  - Grafana 可用性: {grafana_status}
- 已完成诊断:
  - Phase 1 快速检查: {phase1_summary}
  - Phase 2 Prometheus 深度诊断: {phase2_summary}
  - Phase 3 AlertManager 诊断: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-MONITOR-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤（D1.1 → D1.2 → ... → D3.6）及每步输出摘要
2. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
   - 例: "RC-001 已排除 — D2.2 显示 ServiceMonitor selector 匹配正确"
3. **可能的根因假设**: 基于已有证据提出的根因假设及置信度
   - 例: "疑似 RC-008（高基数指标）— D2.4 显示 series 数量 6.2M，远超正常水平"
4. **关键资源快照**:
   ```bash
   # 监控组件状态
   kubectl get pods -n monitoring -o wide > monitoring-pods.txt
   # Prometheus 配置
   curl -s http://localhost:9090/api/v1/status/config | jq -r '.data.yaml' > prometheus-config.yaml
   # AlertManager 配置
   kubectl get secret -n monitoring alertmanager-prometheus-kube-prometheus-alertmanager -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d > alertmanager-config.yaml
   # TSDB 状态
   curl -s http://localhost:9090/api/v1/status/tsdb > prometheus-tsdb.json
   # Prometheus 日志
   kubectl logs -n monitoring prometheus-prometheus-kube-prometheus-prometheus-0 --tail=200 > prometheus-logs.txt
   ```
5. **事件时间线**: 最近 30 分钟内的关键事件按时间排列
   - `HH:MM:SS` - 首次检测到异常
   - `HH:MM:SS` - 开始诊断
   - `HH:MM:SS` - 发现异常 [描述]
   - `HH:MM:SS` - 尝试修复 [操作]
   - `HH:MM:SS` - 修复结果 [成功/失败]
   - `HH:MM:SS` - 决定升级

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/组件 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| Prometheus Operator | >=0.66 | >=0.68 | >=0.70 | >=0.72 | >=0.74 |
| kube-prometheus-stack Helm | >=48.x | >=52.x | >=56.x | >=60.x | >=65.x |
| PodMonitor/ServiceMonitor CRD | v1 | v1 | v1 | v1 | v1 |
| PrometheusRule CRD | v1 | v1 | v1 | v1 | v1 |
| AlertmanagerConfig CRD | v1alpha1 | v1alpha1 | v1beta1 | v1beta1 | v1 |
| Thanos Sidecar 兼容 | >=0.31 | >=0.32 | >=0.33 | >=0.34 | >=0.35 |
| kube-state-metrics | >=2.9 | >=2.10 | >=2.11 | >=2.12 | >=2.13 |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl get servicemonitor` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get prometheusrule` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get alertmanagerconfig` | v1alpha1 | v1alpha1 | v1beta1 | v1beta1 | v1 |
| promtool 版本要求 | >=2.45 | >=2.48 | >=2.51 | >=2.54 | >=2.55 |
| amtool 版本要求 | >=0.25 | >=0.26 | >=0.27 | >=0.27 | >=0.28 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Prometheus | monitoring.coreos.com/v1 | v1 | v1 | v1 | v1 |
| Alertmanager | monitoring.coreos.com/v1 | v1 | v1 | v1 | v1 |
| ServiceMonitor | monitoring.coreos.com/v1 | v1 | v1 | v1 | v1 |
| PodMonitor | monitoring.coreos.com/v1 | v1 | v1 | v1 | v1 |
| PrometheusRule | monitoring.coreos.com/v1 | v1 | v1 | v1 | v1 |
| AlertmanagerConfig | monitoring.coreos.com/v1alpha1 | v1alpha1 | v1beta1 | v1beta1 | v1 |
| ThanosRuler | monitoring.coreos.com/v1 | v1 | v1 | v1 | v1 |

### 9.4 版本相关的诊断注意事项

- **[v1.28+]**: kube-prometheus-stack 默认启用 ServiceMonitor 自动发现：
  - `serviceMonitorSelectorNilUsesHelmValues: false` 时会发现所有 namespace 的 ServiceMonitor
  - 需要检查是否与预期行为一致

- **[v1.30+]**: AlertmanagerConfig CRD 升级到 v1beta1：
  - 新增更多字段支持，如 `muteTimeIntervals`
  - 升级时需要迁移 v1alpha1 资源

- **[v1.31+]**: Prometheus Operator 改进了配置同步机制：
  - 配置重载更可靠，减少配置丢失风险
  - 新增 `PrometheusAgent` 资源支持轻量级采集模式

- **[v1.32+]**: AlertmanagerConfig CRD GA (v1)：
  - 完整功能支持，建议迁移到 v1 版本
  - 弃用 v1alpha1 和 v1beta1

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **将 ServiceMonitor selector 不匹配误判为网络问题** | Target 显示 Down，怀疑是 Pod 网络不通 | ServiceMonitor 的 selector 与 Service labels 不匹配，Prometheus 根本未尝试采集 | 先检查 D2.1/D2.2 确认 ServiceMonitor 已被 Prometheus 发现，再排查网络 |
| **将高基数指标误判为 Prometheus Bug** | Prometheus 频繁 OOM，怀疑是 Prometheus 版本问题 | 应用侧指标设计不合理，将高基数字段（如 user_id、request_id）作为 label | D2.4 检查 seriesCountByMetricName，定位问题指标，与应用团队协作修复 |
| **将告警静默误判为 AlertManager 故障** | 告警不发送，但 AlertManager Pod 正常运行 | 运维人员之前设置了宽泛的静默规则但未记录 | D3.2 先检查活跃的静默规则，确认是否有覆盖过宽的匹配 |
| **将 Thanos 查询慢误判为 Thanos Bug** | Thanos Query 查询历史数据超时 | 存储桶数据量过大或 Store 节点资源不足，非 Thanos 本身问题 | 检查 Store 节点资源使用，考虑增加 Store 副本或优化查询范围 |
| **将 Grafana "No Data" 误判为 Prometheus 问题** | Grafana 面板显示 No Data | Dashboard 查询语法错误或时间范围选择不当，Prometheus 数据正常 | 先在 Prometheus UI 直接查询相同指标，确认数据是否存在 |
| **将通知延迟误判为 AlertManager 故障** | 告警通知延迟到达 | AlertManager 的 group_wait、group_interval 配置过长，或通知渠道本身有延迟 | D3.1 检查 AlertManager 配置中的时间参数，理解告警分组机制 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| Prometheus 架构与 TSDB | `domain-8-observability/` | 理解 TSDB 存储机制、高基数问题根因 |
| AlertManager 告警路由 | `domain-20-enterprise-monitoring-alerting/` | 深入理解路由、抑制、静默机制 |
| 监控告警排障方法论 | `domain-12-troubleshooting/30-monitoring-alerting-troubleshooting.md` | 系统化排障方法 |
| 告警风暴治理 | `domain-20-enterprise-monitoring-alerting/05-alert-noise-reduction.md` | 告警降噪最佳实践 |
| Thanos 架构 | `domain-8-observability/` | 理解 Thanos 组件交互和查询路径 |
| 高基数指标治理 | `domain-8-observability/` | 指标设计最佳实践和治理方案 |
| ServiceMonitor 配置 | `domain-10-extensions/` | Prometheus Operator CRD 使用指南 |
| Grafana 数据源管理 | `domain-8-observability/` | Grafana 配置和排障 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-04 | v1.0 | 初始版本发布。覆盖 Prometheus + AlertManager + Grafana + Thanos，包含 12 个根因、11 个修复操作 | 首批 Skill 库建设，基于监控告警工单分析确定为高优先级场景 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **VictoriaMetrics 集成**: VictoriaMetrics 作为 Prometheus 替代/补充的诊断差异
2. **Cortex 长期存储**: Cortex 特定的故障模式和诊断方法
3. **Loki 日志系统**: Grafana Loki 集成后的联动故障诊断
4. **OpenTelemetry Collector**: OTLP 协议采集的故障诊断
5. **多集群联邦**: Prometheus Federation 或 Thanos 多集群场景的诊断
6. **自定义 Exporter**: 第三方 Exporter 的通用诊断方法
7. **云厂商托管监控**: 阿里云 ARMS、AWS CloudWatch 等托管服务的集成排障
