---
title: 日志收集与管理故障诊断与修复 / Logging Pipeline Diagnosis & Remediation
description: '# 日志收集与管理故障诊断与修复 / Logging Pipeline Diagnosis & Remediation'
category: observability
tags:
- k8s
- skills
- sop
- runbook
- apiserver
- kubelet
- prometheus
- grafana
- helm
- kafka
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- 日志收集与管理故障诊断与修复 / Logging Pipeline Diagnosis & Remediation 是什么
- 如何 日志收集与管理故障诊断与修复 / Logging Pipeline Diagnosis & Remediation
trigger_keywords:
- log missing
- 日志丢失
- fluentd error
- fluent-bit crash
- log pipeline blocked
- 日志管道阻塞
- elasticsearch full
- ES 磁盘满
- loki ingestion error
- log parsing failed
- 日志解析失败
- log rotation
- 日志轮转
- audit log
- 审计日志
- log delay
- 日志延迟
- vector error
- log buffer overflow
- 缓冲区溢出
- log collection stopped
- 日志采集停止
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- prometheus-basics
- monitoring-basics
- kafka-basics
- logging-basics
skill_id: SKILL-16_LOGGING_PIPELINE_FAILURE-001
skill_name: 日志收集与管理故障诊断与修复 / Logging Pipeline Diagnosis & Remediation
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L2-semi-auto
created: "2026-05-23"
---

<!-- condition: kubectl get [[Pods|pods]] -n logging -o jsonpath='{range .items[?(@.status.phase!="Running")]} {.metadata.name}{"\n"}{end}' 显示日志组件异常 -->

# 日志收集与管理故障诊断与修复 / Logging Pipeline Diagnosis & Remediation

---

## 1. 概述

日志管道问题是 [[Kubernetes|Kubernetes]] 可观测性体系中**影响最广泛**的问题类型之一。当日志采集、传输或存储环节出现问题时，会导致应用日志缺失、审计日志不完整、告警延迟甚至安全事件无法追溯。在云原生环境中，日志管道通常由采集层（[[fluentd|[[Fluentd]]]]/Fluent Bit/Vector）、传输层（Kafka/直接推送）和存储层（Elasticsearch/Loki/ClickHouse）组成，任一环节的问题都可能导致日志数据丢失。

### 典型触发场景

1. **日志采集 Agent 问题**: Fluentd/Fluent Bit/Vector DaemonSet Pod 崩溃、OOM、配置错误，导致节点日志无法采集
2. **Buffer 溢出与背压**: 日志产生速率超过采集器处理能力，或下游存储不可用导致 buffer 积压，最终溢出丢失日志
3. **存储后端问题**: Elasticsearch 集群状态 Red/Yellow、Loki 写入限速、磁盘空间耗尽，导致日志无法写入
4. **日志解析配置错误**: Parser 配置与实际日志格式不匹配，导致时间戳解析错误、多行日志被拆分、字段提取失败
5. **容器日志轮转问题**: kubelet 的 containerLogMaxSize 配置不当，导致节点磁盘被容器日志占满

### 前置条件

- **RBAC 权限**: 需要对 logging namespace 的 pods/logs、daemonsets、configmaps 的 get/list/watch 权限
- **SSH 访问**: 深度诊断（Phase 2+）可能需要对节点的 SSH 访问权限
- **工具要求**: kubectl (v1.28+), curl, jq（可选但推荐）
- **日志平台访问**: 需要 Elasticsearch/Loki/Kibana/Grafana 的查询权限
- **监控系统**: Prometheus + 日志采集器的 metrics exporter

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | 特定 Pod 日志在日志平台中缺失 / Specific Pod logs missing from logging platform | 在 Kibana/Grafana Loki 中查询特定 Pod 名称，无结果或结果不完整 | 0.85 | 应用本身未输出日志（stdout/stderr 为空）；日志查询时间范围或筛选条件错误 |
| S2 | Fluentd/Fluent Bit DaemonSet Pod 处于 CrashLoopBackOff / Log agent DaemonSet Pod in CrashLoopBackOff | `kubectl get pods -n logging` 显示采集器 Pod 状态为 CrashLoopBackOff | 0.95 | 新部署的采集器配置错误导致启动失败（属于配置问题而非运行时问题） |
| S3 | 日志延迟超过 5 分钟 / Log latency exceeds 5 minutes | 对比 Pod 中日志产生时间与日志平台中该日志的 @timestamp，延迟超过 5 分钟 | 0.80 | 应用时区配置错误导致时间戳偏差；日志平台时钟不同步 |
| S4 | Elasticsearch 集群状态 Red/Yellow / Elasticsearch cluster status Red or Yellow | `curl -s localhost:9200/_cluster/health?pretty` 返回 status 为 red 或 yellow | 0.90 | 新创建索引分片正在初始化（短暂 yellow 可能是正常行为） |
| S5 | Loki 写入返回 429 (rate limit) / Loki ingestion returns 429 rate limit | Fluent Bit/Vector 日志中出现 `429 Too Many Requests` 或 `rate limit` | 0.85 | 业务高峰期短暂触发限速后自行恢复；故意配置的限速策略 |
| S6 | 多行日志被拆分为多条记录 / Multiline logs split into separate records | 在日志平台中搜索 Java stacktrace 或其他多行日志，每行显示为独立记录 | 0.90 | 应用配置为单行 JSON 日志输出但含换行符（应用侧问题） |
| S7 | 日志时间戳解析错误 / Log timestamp parsing error | 日志平台中 @timestamp 显示为采集时间而非日志产生时间，或时间格式异常 | 0.85 | 应用使用非标准时间戳格式且未配置自定义 parser |
| S8 | 节点磁盘空间被容器日志占满 / Node disk filled by container logs | 节点 DiskPressure 为 True，`df -h /var/log` 显示使用率 >90% | 0.80 | 非日志文件占用磁盘（如容器镜像层、临时文件）；关联 SKILL-NODE-001 |
| S9 | 审计日志未记录或记录不全 / Audit logs missing or incomplete | `kubectl get events` 显示有敏感操作但审计日志平台无对应记录 | 0.75 | 审计策略故意配置为忽略某些请求级别；审计日志存储路径与查询路径不同 |
| S10 | Vector/Fluentd buffer 目录磁盘爆满 / Log agent buffer directory disk full | `kubectl exec -n logging AGENT_POD -- df -h /var/log/fluentd-buffers` 显示 100% | 0.90 | 磁盘紧张但未达到 100%（buffer 仍可写入） |
| S11 | 日志采集 Agent 内存持续增长 / Log agent memory continuously growing | `kubectl top pods -n logging` 显示采集器 Pod 内存持续增长接近 limits | 0.80 | 正常的 buffer 内存使用波动；OOM 前的短暂内存峰值 |
| S12 | 日志中敏感信息未脱敏 / Sensitive information not masked in logs | 在日志平台中搜索到包含密码、Token、API Key 等敏感信息的日志记录 | 0.70 | 故意配置为不脱敏（非生产环境）；应用侧已经脱敏但格式不规范 |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "XX 服务日志查不到，请排查日志采集"
- "Kibana 里看不到 Pod 日志了"
- "日志有延迟，最近的日志是几分钟前的"
- "Elasticsearch 集群变黄了，日志写入报错"
- "Fluentd Pod 一直在重启"
- "Java 堆栈日志被拆开了，不好看"
- "节点磁盘满了，全是日志文件"
- "审计日志没有记录某某操作"
- "Loki 日志写入被限速了"

**English ticket descriptions**:
- "Logs are missing from Kibana for certain pods"
- "Fluent Bit keeps crashing with OOM"
- "Log latency is too high, seeing 10 minute delays"
- "Elasticsearch cluster health is red"
- "Vector buffer is filling up the disk"
- "Multiline Java exceptions are being split"
- "Audit logs are not capturing delete operations"
- "Log parsing is failing, timestamps are wrong"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| 应用本身未输出任何日志（stdout/stderr 为空） | 应用排查 | 非日志管道问题，需检查应用日志配置 |
| 日志平台（Kibana/Grafana）UI 无法访问 | 服务排查 | 前端服务问题，非日志管道问题 |
| Elasticsearch 全部节点宕机 | SKILL-STORE-001 | 存储集群级问题，超出日志管道范围 |
| 节点 NotReady 导致采集器 Pod 无法运行 | SKILL-NODE-001 | 根因是节点问题，日志问题是症状 |
| 日志格式设计问题（非技术问题） | 日志规范制定 | 属于架构设计范畴，非故障处理 |
| 网络策略阻止采集器访问存储后端 | 网络排查 | 网络配置问题而非日志组件问题 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断问题爆炸半径：

**Step T1**: 检查日志采集 DaemonSet 状态（15 秒）
```bash
# 获取日志采集 DaemonSet 状态
kubectl get ds -n logging -o wide
# 或者在 kube-system 命名空间
kubectl get ds -n kube-system -l app.kubernetes.io/component=logging

# 检查 Pod 就绪状态
kubectl get pods -n logging -o wide | head -20
```
> **判断规则**:
> - DaemonSet READY 数远低于 DESIRED 数（>30% 节点无采集器）→ **P1**
> - 所有采集器 Pod 均 NotReady/CrashLoopBackOff → **P1**
> - 少数采集器 Pod 异常（<30%）→ **P2**
> - 所有 Pod Ready → 继续 T2

**Step T2**: 检查采集器资源使用和日志（30 秒）
```bash
# 检查采集器 Pod 资源使用
kubectl top pods -n logging --sort-by=memory

# 检查最近的采集器日志
kubectl logs -n logging -l app=fluentd --tail=30 --since=5m 2>/dev/null | grep -iE 'error|warn|fatal'
# 或 Fluent Bit
kubectl logs -n logging -l app=fluent-bit --tail=30 --since=5m 2>/dev/null | grep -iE 'error|warn|fatal'
# 或 Vector
kubectl logs -n logging -l app=vector --tail=30 --since=5m 2>/dev/null | grep -iE 'error|warn|fatal'
```
> **判断规则**:
> - 内存使用接近 limits（>90%）→ 可能 OOM，风险升级
> - 日志中出现大量 error → 记录错误类型，继续 T3
> - 日志中出现 `connection refused`、`429`、`disk full` → 继续 T3

**Step T3**: 验证日志平台可用性（60 秒）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# Elasticsearch 健康检查
kubectl exec -n logging $(kubectl get pod -n logging -l app=elasticsearch -o jsonpath='{.items[0].metadata.name}') -- curl -s localhost:9200/_cluster/health?pretty 2>/dev/null
# 或直接访问
curl -s http://elasticsearch.logging:9200/_cluster/health?pretty

# Loki 健康检查
kubectl exec -n logging $(kubectl get pod -n logging -l app=loki -o jsonpath='{.items[0].metadata.name}') -- curl -s localhost:3100/ready 2>/dev/null
# 或直接访问
curl -s http://loki.logging:3100/ready

# 检查最近 5 分钟是否有新日志写入（Elasticsearch）
curl -s "http://elasticsearch.logging:9200/_cat/indices?v&s=store.size:desc" | head -10
```
> **判断规则**:
> - Elasticsearch status=red → **P1**（数据可能丢失）
> - Elasticsearch status=yellow + unassigned_shards > 0 → **P2**
> - Loki not ready → **P1**
> - 最近索引无新文档增长 → 确认日志管道阻塞

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| >30% 节点日志采集器异常 **或** 存储后端 Red/不可用 **或** 审计日志完全丢失 | **P1** | 大面积日志丢失，影响故障排查和合规审计。审计日志丢失可能导致安全事件无法追溯 | 15min 内响应，1h 内恢复 |
| 日志延迟 >5 分钟 **或** 部分节点采集器异常（10-30%）**或** Buffer 溢出告警 | **P2** | 日志延迟影响实时监控告警，部分日志可能丢失。Buffer 溢出有丢失风险 | 30min 内响应，2h 内恢复 |
| 单个采集器 Pod 异常 **或** 日志解析错误 **或** 存储 Yellow（非数据丢失风险） | **P3** | 影响范围有限，不影响核心日志采集功能 | 4h 内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

- **完全失效**: 所有日志采集器 Pod 均处于 CrashLoopBackOff 且无法自动恢复（已重启 >5 次）
- **存储灾难**: Elasticsearch 集群 status=red 且有 unassigned primary shards（数据丢失风险）
- **审计合规**: 审计日志停止记录超过 1 小时且涉及生产环境
- **磁盘危急**: 多个节点因日志占用磁盘空间导致 DiskPressure（关联节点问题）
- **安全事件**: 发现日志中存在大量敏感信息泄露需立即处理

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 日志采集层诊断（只读，零风险）

> **目标**: 检查日志采集 Agent（Fluentd/Fluent Bit/Vector）的运行状态、配置和日志。
> **预计耗时**: 5-10 分钟

**Step D1.1**: 获取采集 Agent DaemonSet 状态
- **命令**:
  ```bash
  # 检查所有日志相关的 DaemonSet
  kubectl get ds -n logging -o wide
  kubectl get ds -n kube-system -l app.kubernetes.io/component=logging -o wide
  
  # 详细查看 DaemonSet 状态
  kubectl describe ds -n logging
  ```
- **超时**: 10s
- **预期输出模式**: DaemonSet 列表包含 DESIRED, CURRENT, READY, UP-TO-DATE, AVAILABLE
- **判断规则**:
  - READY < DESIRED → 部分节点采集器未就绪（RC-001），继续 D1.2
  - READY = 0 → 所有采集器异常（RC-001），立即查看 D1.3
  - READY = DESIRED → 采集器 Pod 数量正常，继续 D1.3 检查运行状态
  - Events 中出现 `FailedCreate` → 可能是资源配额或节点调度问题
- **版本差异**: 无

**Step D1.2**: 检查 Agent Pod 分布
- **命令**:
  ```bash
  # 获取采集器 Pod 分布
  kubectl get pods -n logging -o wide
  
  # 对比节点总数
  kubectl get nodes --no-headers | wc -l
  
  # 找出缺少采集器的节点
  comm -23 <(kubectl get nodes -o jsonpath='{.items[*].metadata.name}' | tr ' ' '\n' | sort) \
           <(kubectl get pods -n logging -o jsonpath='{.items[*].spec.nodeName}' | tr ' ' '\n' | sort)
  ```
- **超时**: 10s
- **预期输出模式**: Pod 列表与节点的对应关系
- **判断规则**:
  - 存在节点无对应采集器 Pod → 检查 nodeSelector、tolerations、affinity 配置（RC-001）
  - 节点数与 Pod 数一致 → 采集器调度正常
  - 特定节点标签的节点缺少 Pod → DaemonSet nodeSelector 配置问题
- **版本差异**: 无

**Step D1.3**: 检查 Agent Pod 日志
- **命令**:
  ```bash
  # Fluentd 日志
  kubectl logs -n logging -l app=fluentd --tail=100 --since=30m 2>/dev/null | grep -iE 'error|warn|fatal|exception'
  
  # Fluent Bit 日志
  kubectl logs -n logging -l app=fluent-bit --tail=100 --since=30m 2>/dev/null | grep -iE 'error|warn|fatal'
  
  # Vector 日志
  kubectl logs -n logging -l app=vector --tail=100 --since=30m 2>/dev/null | grep -iE 'error|warn|fatal'
  
  # 查看 CrashLoopBackOff Pod 的上一次日志
  CRASH_POD=$(kubectl get pods -n logging --field-selector=status.phase!=Running -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
  kubectl logs -n logging $CRASH_POD --previous --tail=50 2>/dev/null
  ```
- **超时**: 15s
- **预期输出模式**: 采集器运行日志，关注错误信息
- **判断规则**:
  - `connection refused` 或 `connection reset` → 存储后端连接问题（RC-006）
  - `429 Too Many Requests` → Loki/ES 限速（RC-008）
  - `buffer space has too many data` 或 `buffer overflow` → Buffer 溢出（RC-002）
  - `Permission denied` 或 `cannot read` → 日志文件访问权限问题
  - `parse error` 或 `invalid format` → 日志解析配置错误（RC-004）
  - `OOMKilled` 或 `memory allocation failed` → 内存不足（RC-005）
  - `certificate verify failed` → TLS 证书问题（RC-006）
- **版本差异**:
  - **Fluent Bit [v2.0+]**: 日志格式变更，错误信息更详细
  - **Vector [v0.30+]**: 新增结构化错误日志

**Step D1.4**: 检查 Agent 资源使用
- **命令**:
  ```bash
  # 采集器 Pod 资源使用
  kubectl top pods -n logging --sort-by=memory
  
  # 查看资源 limits
  kubectl get pods -n logging -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.containers[0].resources.limits.memory}{"\t"}{.spec.containers[0].resources.limits.cpu}{"\n"}{end}'
  ```
- **超时**: 10s
- **预期输出模式**: 各 Pod 的 CPU 和内存使用量
- **判断规则**:
  - 内存使用 > limits 的 80% → 有 OOM 风险（RC-005）
  - CPU 使用持续接近 limits → 可能导致日志处理延迟（RC-011）
  - 内存使用远超其他 Pod → 可能存在内存泄漏或配置问题
- **版本差异**: 无

**Step D1.5**: 检查日志源目录
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 在某个采集器 Pod 中检查日志源目录
  POD_NAME=$(kubectl get pods -n logging -l app=fluent-bit -o jsonpath='{.items[0].metadata.name}' 2>/dev/null || kubectl get pods -n logging -l app=fluentd -o jsonpath='{.items[0].metadata.name}')
  
  # 检查容器日志目录
  kubectl exec -n logging $POD_NAME -- ls -la /var/log/containers/ 2>/dev/null | head -20
  kubectl exec -n logging $POD_NAME -- ls -la /var/log/pods/ 2>/dev/null | head -10
  
  # 检查目录是否正确挂载
  kubectl exec -n logging $POD_NAME -- df -h /var/log
  ```
- **超时**: 10s
- **预期输出模式**: 容器日志文件列表
- **判断规则**:
  - 目录为空或挂载失败 → Volume 挂载问题（RC-001）
  - 日志文件存在但无法读取 → 权限问题
  - 日志文件过大（单文件 >1GB）→ 轮转配置问题（RC-007）
- **版本差异**: 无

**Step D1.6**: 检查 kubelet 容器日志配置
- **命令**:
  ```bash
  # SSH 到节点检查 kubelet 配置
  ssh <node-ip> "cat /var/lib/kubelet/config.yaml | grep -A 5 containerLog"
  
  # 或通过 kubelet configz endpoint
  kubectl get --raw /api/v1/nodes/<node-name>/proxy/configz 2>/dev/null | jq '.kubeletconfig.containerLogMaxSize, .kubeletconfig.containerLogMaxFiles'
  ```
- **超时**: 10s
- **预期输出模式**: containerLogMaxSize 和 containerLogMaxFiles 配置值
- **判断规则**:
  - containerLogMaxSize 未配置（默认 10Mi 可能不够）→ 可能导致日志轮转频繁
  - containerLogMaxFiles 未配置（默认 5）→ 可能导致旧日志丢失过快
  - 配置值过大（如 1Gi）→ 可能导致磁盘空间问题（RC-007）
- **版本差异**:
  - **[v1.28+]**: 默认 containerLogMaxSize=10Mi, containerLogMaxFiles=5
  - **[v1.32+]**: 可通过 KubeletConfiguration CRD 动态调整

---

### Phase 2: 传输与缓冲诊断（只读，零风险）

> **目标**: 检查日志传输链路和缓冲状态，诊断日志积压和延迟问题。
> **预计耗时**: 5-10 分钟

**Step D2.1**: 检查 Buffer 状态
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # Fluentd buffer 状态（通过 metrics）
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluentd -o jsonpath='{.items[0].metadata.name}') -- curl -s localhost:24231/metrics 2>/dev/null | grep -E 'fluentd_output_status_buffer|fluentd_output_status_retry'
  
  # Fluent Bit buffer 状态
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluent-bit -o jsonpath='{.items[0].metadata.name}') -- curl -s localhost:2020/api/v1/metrics 2>/dev/null | grep -E 'storage|buffer'
  
  # Vector buffer 状态
  kubectl exec -n logging $(kubectl get pod -n logging -l app=vector -o jsonpath='{.items[0].metadata.name}') -- curl -s localhost:8686/metrics 2>/dev/null | grep -E 'vector_buffer'
  ```
- **超时**: 10s
- **预期输出模式**: Buffer 相关 metrics 数值
- **判断规则**:
  - `fluentd_output_status_buffer_total_bytes` 持续增长 → Buffer 积压（RC-002）
  - `fluentd_output_status_retry_count` 增加 → 输出目标有问题（RC-006）
  - Fluent Bit `storage_chunks_up` 过高 → 内存 buffer 积压
  - Vector `vector_buffer_events` 持续增长 → 下游阻塞
- **版本差异**: 无

**Step D2.2**: 检查 Buffer 目录磁盘空间
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # Fluentd buffer 目录
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluentd -o jsonpath='{.items[0].metadata.name}') -- df -h /var/log/fluentd-buffers 2>/dev/null
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluentd -o jsonpath='{.items[0].metadata.name}') -- du -sh /var/log/fluentd-buffers/* 2>/dev/null | head -10
  
  # Fluent Bit（通常使用 memory buffer，但也可能配置 filesystem）
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluent-bit -o jsonpath='{.items[0].metadata.name}') -- df -h /var/log/fluent-bit 2>/dev/null
  
  # Vector buffer 目录
  kubectl exec -n logging $(kubectl get pod -n logging -l app=vector -o jsonpath='{.items[0].metadata.name}') -- df -h /var/lib/vector 2>/dev/null
  ```
- **超时**: 10s
- **预期输出模式**: 磁盘使用率和 buffer 文件大小
- **判断规则**:
  - 磁盘使用率 > 90% → Buffer 目录即将满（RC-002）
  - 磁盘使用率 100% → Buffer 无法写入，日志丢失风险（RC-002）
  - 单个 buffer 文件过大 → 可能是特定目标长时间不可用
- **版本差异**: 无

**Step D2.3**: 检查背压状态
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # Fluent Bit backpressure 指标
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluent-bit -o jsonpath='{.items[0].metadata.name}') -- curl -s localhost:2020/api/v1/metrics 2>/dev/null | grep -E 'paused|backpressure'
  
  # 检查 Fluent Bit 日志中的背压信息
  kubectl logs -n logging -l app=fluent-bit --tail=50 2>/dev/null | grep -iE 'paused\|backpressure\|chunk'
  
  # Vector 背压状态
  kubectl logs -n logging -l app=vector --tail=50 2>/dev/null | grep -iE 'backpressure\|blocked'
  ```
- **超时**: 10s
- **预期输出模式**: 背压相关日志和指标
- **判断规则**:
  - 出现 `paused` 或 `backpressure` → 下游处理不过来，需检查 Phase 3
  - 频繁出现 → 持续性能问题，需扩容或优化
  - 偶尔出现 → 可能是流量高峰期的正常行为
- **版本差异**: 无

**Step D2.4**: 检查输出插件错误
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # Fluentd 输出插件状态
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluentd -o jsonpath='{.items[0].metadata.name}') -- curl -s localhost:24231/metrics 2>/dev/null | grep -E 'fluentd_output_status_emit_records|fluentd_output_status_emit_count|fluentd_output_status_num_errors'
  
  # 检查 Fluentd 日志中的输出错误
  kubectl logs -n logging -l app=fluentd --tail=100 2>/dev/null | grep -iE 'output\|emit\|chunk\|retry'
  
  # Vector sink 错误
  kubectl exec -n logging $(kubectl get pod -n logging -l app=vector -o jsonpath='{.items[0].metadata.name}') -- curl -s localhost:8686/metrics 2>/dev/null | grep -E 'component_errors_total|component_sent'
  ```
- **超时**: 10s
- **预期输出模式**: 输出插件的错误计数和发送记录数
- **判断规则**:
  - `num_errors` 持续增加 → 输出目标有问题
  - `emit_records` 为 0 → 没有日志被发送出去
  - Vector `component_errors_total` 增加 → Sink 配置或目标问题
- **版本差异**: 无

**Step D2.5**: 检查到存储后端的连接
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 从采集器 Pod 测试 Elasticsearch 连接
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluent-bit -o jsonpath='{.items[0].metadata.name}') -- \
    curl -sk --max-time 5 https://elasticsearch.logging:9200/_cluster/health 2>/dev/null || \
    curl -s --max-time 5 http://elasticsearch.logging:9200/_cluster/health 2>/dev/null
  
  # 测试 Loki 连接
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluent-bit -o jsonpath='{.items[0].metadata.name}') -- \
    curl -s --max-time 5 http://loki.logging:3100/ready 2>/dev/null
  
  # 检查 TLS 证书（如果使用 HTTPS）
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluent-bit -o jsonpath='{.items[0].metadata.name}') -- \
    openssl s_client -connect elasticsearch.logging:9200 -servername elasticsearch.logging </dev/null 2>/dev/null | openssl x509 -noout -dates
  ```
- **超时**: 15s
- **预期输出模式**: 连接状态和健康检查响应
- **判断规则**:
  - 连接超时 → 网络不通或服务不可用（RC-006）
  - TLS 握手失败 → 证书问题（RC-006）
  - 返回 401/403 → 认证配置错误（RC-006）
  - 返回健康响应 → 连接正常，问题可能在其他层面
- **版本差异**: 无

**Step D2.6**: 检查解析配置
- **命令**:
  ```bash
  # 获取 Fluentd ConfigMap
  kubectl get cm -n logging fluentd-config -o yaml 2>/dev/null | grep -A 20 '<parse>'
  
  # 获取 Fluent Bit ConfigMap
  kubectl get cm -n logging fluent-bit-config -o yaml 2>/dev/null | grep -A 10 '\[PARSER\]'
  
  # 获取 Vector ConfigMap
  kubectl get cm -n logging vector-config -o yaml 2>/dev/null | grep -A 15 'parse'
  ```
- **超时**: 10s
- **预期输出模式**: 解析器配置详情
- **判断规则**:
  - 无解析器配置 → 使用默认解析，可能导致时间戳问题（RC-004）
  - 时间格式与实际不匹配 → 时间戳解析错误（RC-012）
  - 缺少多行配置 → 可能导致多行日志拆分（RC-009）
- **版本差异**: 无

**Step D2.7**: 检查多行日志配置
- **命令**:
  ```bash
  # Fluentd 多行配置
  kubectl get cm -n logging fluentd-config -o yaml 2>/dev/null | grep -A 15 'multiline'
  
  # Fluent Bit 多行配置
  kubectl get cm -n logging fluent-bit-config -o yaml 2>/dev/null | grep -A 10 '\[MULTILINE_PARSER\]'
  kubectl get cm -n logging fluent-bit-config -o yaml 2>/dev/null | grep -i 'multiline'
  
  # Vector 多行配置
  kubectl get cm -n logging vector-config -o yaml 2>/dev/null | grep -A 10 'multiline'
  ```
- **超时**: 10s
- **预期输出模式**: 多行日志合并配置
- **判断规则**:
  - 无多行配置但应用输出多行日志（Java/Python stacktrace）→ RC-009
  - 正则表达式过于宽松 → 可能合并不相关的行
  - 正则表达式过于严格 → 可能漏掉部分多行日志
- **版本差异**:
  - **Fluent Bit [v2.0+]**: 新增原生多行解析器，性能更好

**Step D2.8**: 检查采集速率
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # Fluentd 输入速率
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluentd -o jsonpath='{.items[0].metadata.name}') -- curl -s localhost:24231/metrics 2>/dev/null | grep 'fluentd_input_status_num_records_total'
  
  # Fluent Bit 输入速率
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluent-bit -o jsonpath='{.items[0].metadata.name}') -- curl -s localhost:2020/api/v1/metrics 2>/dev/null | grep 'records'
  
  # 估算日志产生速率（SSH 到节点）
  ssh <node-ip> "tail -f /var/log/containers/*.log | pv -l -a > /dev/null"
  # Ctrl+C 中断后查看每秒行数
  ```
- **超时**: 15s
- **预期输出模式**: 日志输入记录数和速率
- **判断规则**:
  - 输入速率 > 10000 行/秒/节点 → 高负载，需评估采集器资源配置（RC-011）
  - 输入速率突增 → 可能是应用日志爆发，检查是否有异常应用
  - 输入速率正常但输出低 → 下游瓶颈或配置问题
- **版本差异**: 无

---

### Phase 3: 存储后端诊断（只读，零风险）

> **目标**: 检查日志存储后端（Elasticsearch/Loki）的健康状态和容量。
> **预计耗时**: 5-10 分钟

**Step D3.1**: 检查 Elasticsearch 集群健康
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 集群健康状态
  kubectl exec -n logging $(kubectl get pod -n logging -l app=elasticsearch -o jsonpath='{.items[0].metadata.name}' 2>/dev/null) -- \
    curl -s localhost:9200/_cluster/health?pretty 2>/dev/null || \
    curl -s http://elasticsearch.logging:9200/_cluster/health?pretty
  
  # 集群统计信息
  curl -s http://elasticsearch.logging:9200/_cluster/stats?pretty | head -50
  ```
- **超时**: 15s
- **预期输出模式**: 集群健康状态 JSON
- **判断规则**:
  - `status: red` → 有 primary shard 不可用，数据丢失风险（RC-003）
  - `status: yellow` → 有 replica shard 不可用，容错能力下降
  - `unassigned_shards > 0` → 需要检查原因（磁盘、节点数、配置）
  - `number_of_nodes` 低于预期 → ES 节点问题
  - `active_primary_shards` 为 0 → 集群不可用
- **版本差异**:
  - **Elasticsearch 8.x**: 安全默认启用，可能需要认证
  - **OpenSearch 2.x**: API 路径可能略有不同

**Step D3.2**: 检查 ES 索引状态
- **命令**:
  ```bash
  # 按大小排序的索引列表
  curl -s "http://elasticsearch.logging:9200/_cat/indices?v&s=store.size:desc" | head -20
  
  # 检查最近的日志索引状态
  curl -s "http://elasticsearch.logging:9200/_cat/indices/logs-*?v&s=index:desc" | head -10
  
  # 检查索引健康
  curl -s "http://elasticsearch.logging:9200/_cat/indices?v&health=red"
  curl -s "http://elasticsearch.logging:9200/_cat/indices?v&health=yellow"
  ```
- **超时**: 15s
- **预期输出模式**: 索引列表及其状态
- **判断规则**:
  - 有 red 状态索引 → 该索引数据不完整（RC-003）
  - 最近的索引 docs.count 为 0 → 日志未写入
  - 单个索引过大（>50GB）→ 可能影响性能
  - 索引数量过多（>1000）→ 需要 ILM 管理（RC-003）
- **版本差异**: 无

**Step D3.3**: 检查 ES 磁盘空间
- **命令**:
  ```bash
  # 节点磁盘分配
  curl -s "http://elasticsearch.logging:9200/_cat/allocation?v"
  
  # 节点磁盘使用详情
  curl -s "http://elasticsearch.logging:9200/_cat/nodes?v&h=name,disk.total,disk.used,disk.avail,disk.used_percent"
  
  # 检查磁盘水位线设置
  curl -s "http://elasticsearch.logging:9200/_cluster/settings?include_defaults=true&flat_settings=true" | grep -E 'watermark|flood'
  ```
- **超时**: 15s
- **预期输出模式**: 磁盘使用情况
- **判断规则**:
  - `disk.used_percent > 85%` → 接近高水位线，新分片无法分配（RC-003）
  - `disk.used_percent > 90%` → 接近洪水水位线，索引变为只读（RC-003）
  - 节点磁盘使用不均 → 分片分配不均衡
  - 无可用磁盘空间 → 紧急！需要立即清理（RC-003）
- **版本差异**: 无

**Step D3.4**: 检查 Loki 状态
- **命令**:
  ```bash
  # Loki ready 状态
  curl -s http://loki.logging:3100/ready
  
  # Loki metrics
  curl -s http://loki.logging:3100/metrics | grep -E 'loki_ingester|loki_distributor|loki_request_duration'
  
  # Loki 配置
  curl -s http://loki.logging:3100/config | head -100
  
  # 检查 Loki Pod 状态
  kubectl get pods -n logging -l app=loki
  kubectl logs -n logging -l app=loki --tail=50 | grep -iE 'error|warn|rate'
  ```
- **超时**: 15s
- **预期输出模式**: Loki 健康状态和指标
- **判断规则**:
  - `/ready` 返回非 200 → Loki 不健康
  - `loki_ingester_chunks_flushed_total` 停止增长 → 写入阻塞
  - 日志中出现 `429` 或 `rate limit` → 写入限速（RC-008）
  - `loki_request_duration_seconds` p99 过高 → 性能问题
- **版本差异**:
  - **Loki 2.x vs 3.x**: 配置格式和 API 有差异

**Step D3.5**: 检查索引生命周期管理 (ILM)
- **命令**:
  ```bash
  # 获取 ILM 策略
  curl -s "http://elasticsearch.logging:9200/_ilm/policy?pretty" | head -100
  
  # 检查 ILM 执行状态
  curl -s "http://elasticsearch.logging:9200/_ilm/status?pretty"
  
  # 检查特定索引的 ILM 状态
  curl -s "http://elasticsearch.logging:9200/logs-*/_ilm/explain?pretty" | head -50
  ```
- **超时**: 15s
- **预期输出模式**: ILM 策略和执行状态
- **判断规则**:
  - 无 ILM 策略 → 索引不会自动清理，磁盘会逐渐耗尽（RC-003）
  - ILM 状态为 ERROR → 策略执行失败，检查原因
  - 索引停留在某个 phase 过久 → 可能配置问题
  - delete phase 未配置 → 旧索引不会被删除
- **版本差异**: 无

**Step D3.6**: 检查审计日志配置
- **命令**:
  ```bash
  # 检查 kube-apiserver 审计配置
  kubectl get pods -n kube-system -l component=kube-apiserver -o yaml | grep -A 10 'audit'
  
  # 检查审计策略文件（需要访问控制平面节点）
  ssh <control-plane-node> "cat /etc/kubernetes/audit-policy.yaml" 2>/dev/null
  
  # 检查审计日志输出路径
  ssh <control-plane-node> "ls -la /var/log/kubernetes/audit/" 2>/dev/null
  
  # 检查审计日志大小
  ssh <control-plane-node> "du -sh /var/log/kubernetes/audit/*" 2>/dev/null
  ```
- **超时**: 15s
- **预期输出模式**: 审计配置和日志文件状态
- **判断规则**:
  - 无 `--audit-policy-file` 参数 → 审计未启用（RC-010）
  - `--audit-log-path` 为空或文件不存在 → 审计日志未保存（RC-010）
  - 审计策略中 level 过低（None 或 Metadata）→ 记录信息不足（RC-010）
  - 审计日志文件过大 → 需要配置轮转
- **版本差异**:
  - **[v1.28+]**: 审计后端支持更多配置选项
  - **[v1.30+]**: 增强的审计事件类型

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 风险 | 诊断证据 | 备注 |
|--------|------|------|-----|---------|------|
| RC-001 | **日志采集 Agent DaemonSet 调度失败** — 节点 taint/label 不匹配、资源不足、或 nodeSelector 配置错误导致采集器无法在某些节点运行 | ~15% | 🟡 | D1.1 READY < DESIRED；D1.2 存在无采集器的节点 | 可能影响特定节点或节点组的日志采集 |
| RC-002 | **Buffer 溢出导致日志丢失** — 下游存储不可用或处理速度慢，buffer 积压后溢出丢弃数据 | ~15% | 🟡 | D2.1 buffer metrics 持续增长；D2.2 buffer 目录磁盘接近满；D1.3 日志出现 `buffer overflow` | 数据丢失不可恢复，需要尽快处理 |
| RC-003 | **Elasticsearch 磁盘空间耗尽** — ES 节点磁盘使用超过水位线，导致索引变为只读或分片无法分配 | ~12% | 🔴 | D3.1 status=red/yellow；D3.3 disk.used_percent > 85%；D3.5 ILM 未配置或失败 | 紧急！可能导致数据丢失 |
| RC-004 | **日志解析配置错误** — Parser 正则表达式与日志格式不匹配，导致时间戳提取失败或字段解析错误 | ~10% | 🟢 | D2.6 解析配置与日志格式不匹配；D1.3 出现 `parse error` | 影响日志可用性但不丢失原始数据 |
| RC-005 | **Agent 内存不足/OOM** — 采集器配置的内存 limits 不足以处理日志量，频繁 OOM 重启 | ~8% | 🟡 | D1.4 内存接近 limits；D1.3 日志显示 OOMKilled；Pod 状态 CrashLoopBackOff | 重启期间可能丢失日志 |
| RC-006 | **存储后端连接认证失败** — 网络不通、TLS 证书问题、认证凭据过期导致无法连接存储后端 | ~7% | 🟡 | D2.5 连接测试失败；D1.3 日志出现 `connection refused`、`401`、`certificate verify failed` | 采集器运行但日志无法发送 |
| RC-007 | **容器日志轮转配置缺失** — kubelet 未配置或配置不当 containerLogMaxSize/containerLogMaxFiles，导致节点磁盘被日志占满 | ~6% | 🟢 | D1.6 轮转配置缺失或过大；节点 DiskPressure；`/var/log/pods` 占用过大 | 可能触发节点问题 |
| RC-008 | **Loki 写入限速** — Loki 配置的 ingestion rate limit 被触发，拒绝新的日志写入 | ~5% | 🟡 | D3.4 日志出现 `429`；Loki metrics 显示限速；D1.3 出现 `rate limit` | 超限的日志会被丢弃 |
| RC-009 | **多行日志合并失败** — 缺少或配置错误的多行日志解析器，导致 stacktrace 等多行日志被拆分 | ~5% | 🟢 | D2.7 无多行配置；日志平台显示拆分的 stacktrace | 影响日志可读性 |
| RC-010 | **审计日志策略配置不当** — kube-apiserver 审计策略未启用、级别过低或输出路径配置错误 | ~5% | 🟡 | D3.6 审计配置缺失或不完整 | 合规风险 |
| RC-011 | **高吞吐日志源导致采集瓶颈** — 单节点日志产生速率超过采集器处理能力 | ~4% | 🟡 | D2.8 采集速率极高；D1.4 CPU 使用率高；D2.3 出现 backpressure | 需要扩容或限流 |
| RC-012 | **时区/时间戳不一致** — 应用、容器、采集器、存储使用不同时区或时间格式配置 | ~4% | 🟢 | D2.6 时间格式配置错误；日志平台时间戳与实际不符 | 影响日志分析但不丢失数据 |
| RC-013 | **日志中敏感信息泄露** — 日志脱敏规则未配置或配置不完整，导致敏感数据被记录 | ~4% | 🔴 | 日志平台可搜索到密码、Token 等敏感信息 | 安全合规风险 |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可自动执行）

#### REM-001: 修复日志解析配置
- **适用根因**: RC-004, RC-012
- **前置检查**:
  ```bash
  # 获取当前解析配置
  kubectl get cm -n logging fluent-bit-config -o yaml | grep -A 20 'PARSER'
  
  # 获取日志样本确认格式
  kubectl logs <pod-name> --tail=5 | head -3
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 备份当前配置
  kubectl get cm -n logging fluent-bit-config -o yaml > /tmp/fluent-bit-config-backup.yaml
  
  # 编辑配置（示例：修正时间格式）
  kubectl edit cm -n logging fluent-bit-config
  # 修改 Time_Key 和 Time_Format 配置
  
  # 重启 DaemonSet 使配置生效
  kubectl rollout restart ds -n logging fluent-bit
  ```
- **后置验证**:
  ```bash
  # 等待 Pod 重启完成
  kubectl rollout status ds -n logging fluent-bit
  
  # 检查新日志是否正确解析
  # 在日志平台查询最近的日志，确认时间戳正确
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl apply -f /tmp/fluent-bit-config-backup.yaml
  kubectl rollout restart ds -n logging fluent-bit
  ```

#### REM-002: 配置多行日志合并规则
- **适用根因**: RC-009
- **前置检查**:
  ```bash
  # 确认日志中存在多行格式（如 Java stacktrace）
  kubectl logs <java-pod> --tail=50 | grep -A 5 'Exception\|at '
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 备份配置
  kubectl get cm -n logging fluent-bit-config -o yaml > /tmp/fluent-bit-config-backup.yaml
  
  # 添加多行解析器配置（示例：Java stacktrace）
  cat <<EOF | kubectl apply -f -
  apiVersion: v1
  kind: ConfigMap
  metadata:
    name: fluent-bit-config
    namespace: logging
  data:
    parsers.conf: |
      [MULTILINE_PARSER]
          name          multiline-java
          type          regex
          flush_timeout 1000
          # 匹配以日期时间开头的行作为日志起始
          rule          "start_state"   "/^\d{4}-\d{2}-\d{2}/"   "cont"
          rule          "cont"          "/^\s+at\s/"             "cont"
          rule          "cont"          "/^\s+\.\.\.\s/"         "cont"
          rule          "cont"          "/^Caused by:/"          "cont"
  EOF
  
  # 重启采集器
  kubectl rollout restart ds -n logging fluent-bit
  ```
- **后置验证**:
  ```bash
  kubectl rollout status ds -n logging fluent-bit
  # 在日志平台查询 Java exception，确认 stacktrace 已合并为单条记录
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl apply -f /tmp/fluent-bit-config-backup.yaml
  kubectl rollout restart ds -n logging fluent-bit
  ```

#### REM-003: 配置容器日志轮转参数
- **适用根因**: RC-007
- **前置检查**:
  ```bash
  # 检查当前 kubelet 日志轮转配置
  kubectl get --raw /api/v1/nodes/<node-name>/proxy/configz | jq '.kubeletconfig | {containerLogMaxSize, containerLogMaxFiles}'
  
  # 检查当前日志大小
  ssh <node-ip> "du -sh /var/log/pods/*/* | sort -rh | head -10"
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 修改 kubelet 配置（需要在所有节点执行）
  ssh <node-ip> "cat >> /var/lib/kubelet/config.yaml << EOF
  containerLogMaxSize: 50Mi
  containerLogMaxFiles: 5
  EOF"
  
  # 重启 kubelet
  ssh <node-ip> "systemctl restart kubelet"
  ```
- **后置验证**:
  ```bash
  # 验证配置生效
  kubectl get --raw /api/v1/nodes/<node-name>/proxy/configz | jq '.kubeletconfig | {containerLogMaxSize, containerLogMaxFiles}'
  # 预期: containerLogMaxSize: 50Mi, containerLogMaxFiles: 5
  
  # 确认节点状态正常
  kubectl get node <node-name>
  ```
- **回滚命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 恢复原配置并重启 kubelet
  ssh <node-ip> "vim /var/lib/kubelet/config.yaml"
  ssh <node-ip> "systemctl restart kubelet"
  ```

#### REM-004: 修正时间戳解析
- **适用根因**: RC-012
- **前置检查**:
  ```bash
  # 获取日志样本中的时间格式
  kubectl logs <pod-name> --tail=3 | grep -oE '\d{4}-\d{2}-\d{2}[T ]\d{2}:\d{2}:\d{2}'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 备份并更新 ConfigMap
  kubectl get cm -n logging fluent-bit-config -o yaml > /tmp/fluent-bit-config-backup.yaml
  
  # 修改时间解析格式（示例）
  kubectl patch cm -n logging fluent-bit-config --type=json -p='[
    {"op": "replace", "path": "/data/parsers.conf", "value": "[PARSER]\n    Name        json\n    Format      json\n    Time_Key    time\n    Time_Format %Y-%m-%dT%H:%M:%S.%LZ\n    Time_Keep   On"}
  ]'
  
  # 重启采集器
  kubectl rollout restart ds -n logging fluent-bit
  ```
- **后置验证**:
  ```bash
  kubectl rollout status ds -n logging fluent-bit
  # 检查新日志的 @timestamp 是否与应用日志时间一致
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl apply -f /tmp/fluent-bit-config-backup.yaml
  kubectl rollout restart ds -n logging fluent-bit
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批）

#### REM-005: Agent 资源扩容与 buffer 调优
- **适用根因**: RC-002, RC-005, RC-011
- **影响说明**: 修改 DaemonSet 资源配置会导致所有采集器 Pod 滚动重启，期间可能有短暂日志延迟。
- **审批提示**: "建议扩大日志采集器资源配额并调优 buffer 配置，需要重启采集器 Pod（滚动更新），预计影响约 2 分钟。是否批准？"
- **前置检查**:
  ```bash
  # 当前资源使用
  kubectl top pods -n logging -l app=fluent-bit
  
  # 当前资源配置
  kubectl get ds -n logging fluent-bit -o jsonpath='{.spec.template.spec.containers[0].resources}'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 备份当前配置
  kubectl get ds -n logging fluent-bit -o yaml > /tmp/fluent-bit-ds-backup.yaml
  
  # 扩容资源（示例：增加内存到 512Mi）
  kubectl patch ds -n logging fluent-bit --type=json -p='[
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources/limits/memory", "value": "512Mi"},
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources/requests/memory", "value": "256Mi"}
  ]'
  
  # 如果使用 Fluentd，同时调整 buffer 配置
  kubectl edit cm -n logging fluentd-config
  # 修改 buffer 配置：
  # <buffer>
  #   @type file
  #   path /var/log/fluentd-buffers/
  #   chunk_limit_size 16m
  #   total_limit_size 2g
  #   flush_interval 5s
  #   retry_max_interval 30s
  # </buffer>
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 等待滚动更新完成
  kubectl rollout status ds -n logging fluent-bit
  
  # 检查资源使用是否正常
  kubectl top pods -n logging -l app=fluent-bit
  
  # 确认 buffer 不再增长
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluent-bit -o jsonpath='{.items[0].metadata.name}') -- curl -s localhost:2020/api/v1/metrics | grep buffer
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  kubectl apply -f /tmp/fluent-bit-ds-backup.yaml
  ```

#### REM-006: 修复存储后端连接认证
- **适用根因**: RC-006
- **影响说明**: 更新连接配置后需要重启采集器，期间日志会暂时积压在 buffer 中。
- **审批提示**: "建议更新日志存储后端连接配置，需要重启采集器 Pod。是否批准？"
- **前置检查**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 测试当前连接
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluent-bit -o jsonpath='{.items[0].metadata.name}') -- \
    curl -s --max-time 5 http://elasticsearch.logging:9200/_cluster/health
  
  # 检查当前认证配置
  kubectl get secret -n logging es-credentials -o yaml 2>/dev/null
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 更新认证凭据（示例）
  kubectl create secret generic -n logging es-credentials \
    --from-literal=username=elastic \
    --from-literal=password='<new-password>' \
    --dry-run=client -o yaml | kubectl apply -f -
  
  # 如果是 TLS 证书问题，更新证书
  kubectl create secret tls -n logging es-tls \
    --cert=/path/to/new/cert.pem \
    --key=/path/to/new/key.pem \
    --dry-run=client -o yaml | kubectl apply -f -
  
  # 重启采集器使新配置生效
  kubectl rollout restart ds -n logging fluent-bit
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  kubectl rollout status ds -n logging fluent-bit
  
  # 测试连接
  kubectl exec -n logging $(kubectl get pod -n logging -l app=fluent-bit -o jsonpath='{.items[0].metadata.name}') -- \
    curl -s --max-time 5 http://elasticsearch.logging:9200/_cluster/health
  # 预期: 返回集群健康状态，不再是认证错误
  
  # 检查采集器日志无连接错误
  kubectl logs -n logging -l app=fluent-bit --tail=20 | grep -i error
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 恢复旧凭据并重启
  kubectl rollout undo ds -n logging fluent-bit
  ```

#### REM-007: Elasticsearch 索引生命周期管理优化
- **适用根因**: RC-003
- **影响说明**: 配置 ILM 策略会在后台逐步删除旧索引，释放磁盘空间。不会影响新日志写入。
- **审批提示**: "建议配置 ILM 策略自动清理旧索引以释放磁盘空间。历史日志将被逐步删除。是否批准？"
- **前置检查**:
  ```bash
  # 检查当前 ILM 状态
  curl -s http://elasticsearch.logging:9200/_ilm/status
  
  # 检查当前磁盘使用
  curl -s http://elasticsearch.logging:9200/_cat/allocation?v
  ```
- **执行命令**:
  ```bash
  # 创建 ILM 策略（示例：保留 30 天）
  curl -X PUT "http://elasticsearch.logging:9200/_ilm/policy/logs-retention" -H 'Content-Type: application/json' -d '{
    "policy": {
      "phases": {
        "hot": {
          "min_age": "0ms",
          "actions": {
            "rollover": {
              "max_size": "50gb",
              "max_age": "1d"
            }
          }
        },
        "warm": {
          "min_age": "7d",
          "actions": {
            "shrink": {
              "number_of_shards": 1
            },
            "forcemerge": {
              "max_num_segments": 1
            }
          }
        },
        "delete": {
          "min_age": "30d",
          "actions": {
            "delete": {}
          }
        }
      }
    }
  }'
  
  # 将策略应用到索引模板
  curl -X PUT "http://elasticsearch.logging:9200/_index_template/logs" -H 'Content-Type: application/json' -d '{
    "index_patterns": ["logs-*"],
    "template": {
      "settings": {
        "index.lifecycle.name": "logs-retention",
        "index.lifecycle.rollover_alias": "logs"
      }
    }
  }'
  ```
- **后置验证**:
  ```bash
  # 检查策略状态
  curl -s http://elasticsearch.logging:9200/_ilm/policy/logs-retention
  
  # 监控磁盘空间变化（可能需要等待一段时间）
  curl -s http://elasticsearch.logging:9200/_cat/allocation?v
  ```
- **回滚命令**:
  ```bash
  # 删除 ILM 策略
  curl -X DELETE "http://elasticsearch.logging:9200/_ilm/policy/logs-retention"
  ```

#### REM-008: 配置日志脱敏规则
- **适用根因**: RC-013
- **影响说明**: 添加脱敏规则会增加采集器 CPU 开销，但确保敏感信息不被记录。
- **审批提示**: "建议配置日志脱敏规则，将敏感信息（密码、Token 等）替换为占位符。是否批准？"
- **前置检查**:
  ```bash
  # 确认日志中存在敏感信息
  # 在日志平台搜索 password=, token=, apikey= 等
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 备份配置
  kubectl get cm -n logging fluent-bit-config -o yaml > /tmp/fluent-bit-config-backup.yaml
  
  # 添加脱敏 filter（示例：Fluent Bit Lua 脚本）
  cat <<EOF | kubectl apply -f -
  apiVersion: v1
  kind: ConfigMap
  metadata:
    name: fluent-bit-lua-scripts
    namespace: logging
  data:
    redact.lua: |
      function redact_sensitive(tag, timestamp, record)
          for key, val in pairs(record) do
              if type(val) == "string" then
                  -- 脱敏密码
                  record[key] = string.gsub(val, '([Pp]assword[=:]["\']?)([^"\'%s,}]+)', '%1***REDACTED***')
                  -- 脱敏 Token
                  record[key] = string.gsub(record[key], '([Tt]oken[=:]["\']?)([A-Za-z0-9_%-%.]+)', '%1***REDACTED***')
                  -- 脱敏 API Key
                  record[key] = string.gsub(record[key], '([Aa]pi[_-]?[Kk]ey[=:]["\']?)([A-Za-z0-9_%-%.]+)', '%1***REDACTED***')
              end
          end
          return 1, timestamp, record
      end
  EOF
  
  # 更新 Fluent Bit 配置引用 Lua 脚本
  # 重启采集器
  kubectl rollout restart ds -n logging fluent-bit
  ```
- **后置验证**:
  ```bash
  kubectl rollout status ds -n logging fluent-bit
  # 在日志平台搜索，确认敏感信息已被替换为 ***REDACTED***
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  kubectl apply -f /tmp/fluent-bit-config-backup.yaml
  kubectl delete cm -n logging fluent-bit-lua-scripts
  kubectl rollout restart ds -n logging fluent-bit
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导）

#### REM-009: Elasticsearch 紧急磁盘清理
- **适用根因**: RC-003
- **影响说明**: 删除旧索引会导致历史日志永久丢失。清理过程可能影响 ES 性能。
- **操作步骤**:
  1. **评估磁盘状态**:
     ```bash
     curl -s http://elasticsearch.logging:9200/_cat/allocation?v
     curl -s http://elasticsearch.logging:9200/_cat/indices?v&s=store.size:desc | head -20
     ```
  2. **临时禁用只读模式（如果已触发）**:
     ```bash
     curl -X PUT "http://elasticsearch.logging:9200/_cluster/settings" -H 'Content-Type: application/json' -d '{
       "transient": {
         "cluster.routing.allocation.disk.watermark.flood_stage": "99%"
       }
     }'
     
     curl -X PUT "http://elasticsearch.logging:9200/*/_settings" -H 'Content-Type: application/json' -d '{
       "index.blocks.read_only_allow_delete": null
     }'
     ```
  3. **删除最旧的索引释放空间**:
     ```bash
     # 列出可删除的旧索引
     curl -s http://elasticsearch.logging:9200/_cat/indices?v&s=creation.date | head -20
     
     # 删除旧索引（示例：删除 30 天前的）
     curl -X DELETE "http://elasticsearch.logging:9200/logs-2025.01.*"
     ```
  4. **验证空间释放**:
     ```bash
     curl -s http://elasticsearch.logging:9200/_cat/allocation?v
     # 确认磁盘使用率下降到安全水位
     ```
  5. **恢复正常水位线设置**:
     ```bash
     curl -X PUT "http://elasticsearch.logging:9200/_cluster/settings" -H 'Content-Type: application/json' -d '{
       "transient": {
         "cluster.routing.allocation.disk.watermark.flood_stage": null
       }
     }'
     ```
- **安全检查**:
  - 确认要删除的索引确实是可以丢弃的历史数据
  - 在删除前评估合规要求（是否有最低保留期限）
  - 建议先备份关键索引到 snapshot
- **回滚方案**:
  - 索引删除不可逆
  - 如果有 snapshot，可以从 snapshot 恢复

#### REM-010: 审计日志策略重配置
- **适用根因**: RC-010
- **影响说明**: 修改 kube-apiserver 配置需要重启 apiserver，在非 HA 场景下会导致短暂的 API 不可用。
- **操作步骤**:
  1. **备份当前审计配置**:
     ```bash
     ssh <control-plane-node> "cp /etc/kubernetes/audit-policy.yaml /etc/kubernetes/audit-policy.yaml.bak"
     ssh <control-plane-node> "cp /etc/kubernetes/manifests/kube-apiserver.yaml /etc/kubernetes/manifests/kube-apiserver.yaml.bak"
     ```
  2. **创建或更新审计策略**:
     ```bash
     cat <<EOF | ssh <control-plane-node> "cat > /etc/kubernetes/audit-policy.yaml"
     apiVersion: audit.k8s.io/v1
     kind: Policy
     rules:
       # 记录所有请求的元数据
       - level: Metadata
         resources:
         - group: ""
           resources: ["pods", "services", "configmaps", "secrets"]
       # 记录敏感操作的请求体
       - level: RequestResponse
         resources:
         - group: ""
           resources: ["secrets"]
         verbs: ["create", "update", "patch", "delete"]
       # 记录 RBAC 相关操作
       - level: RequestResponse
         resources:
         - group: "rbac.authorization.k8s.io"
       # 其他请求记录元数据
       - level: Metadata
     EOF
     ```
  3. **更新 kube-apiserver 配置**:
     ```bash
     ssh <control-plane-node> "vim /etc/kubernetes/manifests/kube-apiserver.yaml"
     # 添加以下参数：
     # --audit-policy-file=/etc/kubernetes/audit-policy.yaml
     # --audit-log-path=/var/log/kubernetes/audit/audit.log
     # --audit-log-maxage=30
     # --audit-log-maxbackup=10
     # --audit-log-maxsize=100
     ```
  4. **等待 apiserver 重启**:
     ```bash
     kubectl get pods -n kube-system -l component=kube-apiserver -w
     ```
  5. **验证审计日志**:
     ```bash
     ssh <control-plane-node> "tail -20 /var/log/kubernetes/audit/audit.log"
     ```
- **安全检查**:
  - 在 HA 集群中逐节点更新，确保始终有 apiserver 可用
  - 更新前确认 audit-policy.yaml 语法正确
  - 确认日志目录存在且有写入权限
- **回滚方案**:
  ```bash
  ssh <control-plane-node> "cp /etc/kubernetes/audit-policy.yaml.bak /etc/kubernetes/audit-policy.yaml"
  ssh <control-plane-node> "cp /etc/kubernetes/manifests/kube-apiserver.yaml.bak /etc/kubernetes/manifests/kube-apiserver.yaml"
  # 等待 apiserver 自动重启
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-011: 日志管道完全重建
- **适用根因**: 多个根因叠加、配置严重损坏、无法修复的状态
- **审批要求**: 需要高级 SRE + 平台团队审批
- **数据备份**: 确认 Elasticsearch snapshot 已创建，重要日志已导出
- **操作步骤**:
  1. **创建数据备份**:
     ```bash
     # 创建 ES snapshot
     curl -X PUT "http://elasticsearch.logging:9200/_snapshot/backup/pre-rebuild-$(date +%Y%m%d)" -H 'Content-Type: application/json' -d '{
       "indices": "logs-*",
       "ignore_unavailable": true,
       "include_global_state": false
     }'
     ```
  2. **导出当前配置**:
     ```bash
     kubectl get all -n logging -o yaml > /tmp/logging-namespace-backup.yaml
     kubectl get cm -n logging -o yaml > /tmp/logging-cm-backup.yaml
     kubectl get secret -n logging -o yaml > /tmp/logging-secret-backup.yaml
     ```
  3. **删除旧的日志管道组件**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete --all`：批量删除某类全部资源，波及面巨大

     ```bash
     # 警告：此操作会导致日志采集完全停止
     kubectl delete ds -n logging --all  # ⚠️ 批量删除，波及面大
     kubectl delete deployment -n logging --all  # ⚠️ 批量删除，波及面大
     kubectl delete sts -n logging --all  # ⚠️ 批量删除，波及面大
     ```
  4. **清理残留资源**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete --all`：批量删除某类全部资源，波及面巨大

     ```bash
     kubectl delete pvc -n logging --all  # ⚠️ 批量删除，波及面大
     kubectl delete cm -n logging --all  # ⚠️ 批量删除，波及面大
     kubectl delete secret -n logging --all  # ⚠️ 批量删除，波及面大
     ```
  5. **重新部署日志管道**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

     ```bash
     # 使用 Helm 或标准 YAML 重新部署
     helm install fluent-bit fluent/fluent-bit -n logging
     helm install elasticsearch elastic/elasticsearch -n logging
     # 或
     kubectl apply -f logging-stack.yaml
     ```
  6. **恢复配置和数据**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

     ```bash
     # 恢复自定义配置
     kubectl apply -f /tmp/logging-cm-backup.yaml
     
     # 从 snapshot 恢复数据（如果需要）
     curl -X POST "http://elasticsearch.logging:9200/_snapshot/backup/pre-rebuild-$(date +%Y%m%d)/_restore"
     ```
- **回滚方案**:
  - 重建后的系统无法回滚到之前状态
  - 依赖事先创建的配置和数据备份进行恢复

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# V1: 确认所有采集器 Pod Running
kubectl get pods -n logging -o wide
# 预期: 所有 Pod STATUS 为 Running，且 READY 为 1/1

# V2: 确认 DaemonSet 就绪数正确
kubectl get ds -n logging
# 预期: READY = DESIRED = CURRENT

# V3: 确认采集器无错误日志
kubectl logs -n logging -l app=fluent-bit --tail=20 --since=2m | grep -iE 'error|fatal'
# 预期: 无输出或仅有可忽略的警告

# V4: 确认存储后端健康
# Elasticsearch
curl -s http://elasticsearch.logging:9200/_cluster/health?pretty | grep status
# 预期: "status" : "green" 或 "yellow"（无 red）

# Loki
curl -s http://loki.logging:3100/ready
# 预期: ready

# V5: 确认最新日志已写入
# 在日志平台查询最近 1 分钟的日志，确认有新数据

# V6: 确认 buffer 无异常积压
kubectl exec -n logging $(kubectl get pod -n logging -l app=fluent-bit -o jsonpath='{.items[0].metadata.name}') -- curl -s localhost:2020/api/v1/metrics | grep -E 'storage|buffer'
# 预期: buffer 指标稳定，无持续增长
```

### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| 日志延迟 | 对比日志产生时间与平台接收时间 | 延迟 < 30 秒 | 延迟 > 5 分钟 |
| 采集器内存 | `kubectl top pods -n logging` | 稳定在 limits 的 70% 以下 | 持续 > 90% |
| Buffer 大小 | `fluentd_output_status_buffer_total_bytes` | 保持稳定或下降 | 持续增长 |
| ES 集群状态 | `curl elasticsearch:9200/_cluster/health` | status=green/yellow | status=red |
| Loki 写入速率 | `loki_distributor_bytes_received_total` | 稳定 | 突然下降到 0 |
| 采集器 Pod 重启 | `kubectl get pods -n logging` | RESTARTS 不增加 | 频繁重启 |
| 日志丢失率 | 对比产生日志数与平台存储数 | 丢失率 < 0.1% | 丢失率 > 1% |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：

- [ ] 所有日志采集器 Pod Running 且 READY
- [ ] 日志延迟 < 30 秒
- [ ] 无日志丢失（对比特定 Pod 的日志行数）
- [ ] 存储后端健康（ES green/yellow，Loki ready）
- [ ] Buffer 指标稳定无增长
- [ ] 采集器日志无 error/fatal
- [ ] 日志平台可正常查询最新日志
- [ ] 多行日志正确合并（如适用）
- [ ] 时间戳解析正确（如适用）
- [ ] 审计日志正常记录（如适用）

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| 日志延迟趋势 | 监控日志写入延迟指标 | 持续 | 延迟增加 → 检查采集器性能和存储后端 |
| 磁盘空间趋势 | ES `_cat/allocation`，节点 `df -h` | 每小时 | 使用率上升 → 检查 ILM 策略和日志量 |
| 采集器重启 | `kubectl get pods -n logging` | 每小时 | 重启次数增加 → 检查 OOM 和配置 |
| Buffer 积压 | 采集器 buffer metrics | 每 15 分钟 | Buffer 增长 → 检查下游存储 |
| ES 集群状态 | `_cluster/health` | 每 5 分钟 | 状态变化 → 检查分片分配 |
| 日志完整性 | 抽样对比源日志与平台日志 | 每 4 小时 | 发现丢失 → 重新诊断 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **15 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后验证失败 |
| **严重性升级** | 初始分级为 P2/P3 但问题扩大（如更多节点采集器异常） | 诊断过程中问题范围扩大 |
| **未知根因** | 完成 Phase 1-3 所有诊断步骤但无法匹配任何已知根因 | 所有诊断步骤均无明确异常发现 |
| **存储灾难** | Elasticsearch cluster status=red 且有 primary shards 丢失 | D3.1 或 D3.2 发现 |
| **审计合规** | 审计日志停止记录超过 1 小时且为生产环境 | D3.6 发现审计失效 |
| **安全事件** | 发现大量敏感信息泄露需立即处理 | 任何诊断步骤中发现 |

### 8.2 升级消息模板

```
【{severity}】日志收集与管理问题 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {summary}
  - 问题类型: {fault_type}（采集层/传输层/存储层/配置）
  - 影响范围: {impact_scope}
- 影响评估:
  - 受影响节点: {affected_nodes}/{total_nodes}
  - 日志延迟: {log_latency}
  - 估计日志丢失: {estimated_loss}
  - 审计日志状态: {audit_status}
- 已完成诊断:
  - Phase 1 采集层: {phase1_summary}
  - Phase 2 传输层: {phase2_summary}
  - Phase 3 存储层: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-LOG-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤及输出摘要
2. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
3. **可能的根因假设**: 基于已有证据提出的根因假设及置信度
4. **关键资源快照**:
   ```bash
   # 采集器状态
   kubectl get ds,pods -n logging -o wide > logging-status.txt
   # 采集器日志
   kubectl logs -n logging -l app=fluent-bit --tail=200 > fluent-bit-logs.txt
   # 采集器配置
   kubectl get cm -n logging -o yaml > logging-configmaps.txt
   # ES 状态
   curl -s http://elasticsearch.logging:9200/_cluster/health?pretty > es-health.txt
   curl -s http://elasticsearch.logging:9200/_cat/indices?v > es-indices.txt
   curl -s http://elasticsearch.logging:9200/_cat/allocation?v > es-allocation.txt
   ```
5. **事件时间线**: 最近 30 分钟内的关键事件按时间排列

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| containerLogMaxSize/containerLogMaxFiles | 稳定，默认 10Mi/5 | 同左 | 同左 | 同左 | 可通过 KubeletConfiguration CRD 动态配置 |
| 审计日志 Dynamic Webhook | beta | GA | GA | GA | GA |
| 审计事件增强 | 基础 | 增强请求信息 | 同左 | 增强响应信息 | 稳定 |
| Node Log API | alpha | beta | beta | GA | GA |
| kubectl logs --since | 稳定 | 稳定 | 稳定 | 稳定 | 稳定 |
| CEL Admission for audit | - | alpha | beta | beta | GA |

### 9.2 日志采集器版本兼容

| 采集器 | 推荐版本 | K8s v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|-------|---------|-----------|-------|-------|-------|-------|
| Fluent Bit | 2.2.x / 3.0.x | 兼容 | 兼容 | 兼容 | 兼容 | 兼容 |
| Fluentd | 1.16.x | 兼容 | 兼容 | 兼容 | 兼容 | 兼容 |
| Vector | 0.36.x / 0.37.x | 兼容 | 兼容 | 兼容 | 兼容 | 兼容 |
| Promtail (Loki) | 2.9.x / 3.0.x | 兼容 | 兼容 | 兼容 | 兼容 | 兼容 |

### 9.3 存储后端版本兼容

| 存储后端 | 推荐版本 | 备注 |
|---------|---------|------|
| Elasticsearch | 7.17.x / 8.x | 8.x 默认启用安全，需配置认证 |
| OpenSearch | 2.x | Elasticsearch 兼容的开源替代 |
| Loki | 2.9.x / 3.0.x | 3.0 有 breaking changes |
| ClickHouse | 23.x / 24.x | 高性能日志存储替代方案 |

### 9.4 版本相关诊断注意事项

- **[v1.30+]**: Node Log API 可用，支持 `kubectl get --raw /api/v1/nodes/<node>/proxy/logs/` 直接获取节点日志，简化诊断

- **[v1.31+]**: Node Log API GA，更稳定的节点日志访问方式

- **[v1.32+]**: 
  - KubeletConfiguration CRD 支持动态调整日志轮转参数
  - CEL Admission GA，可用于审计策略的高级过滤

- **Fluent Bit 3.0+**: 
  - 新的配置语法，部分旧配置需要迁移
  - 改进的多行日志处理

- **Loki 3.0+**: 
  - 新的 TSDB 存储引擎
  - 部分 API 变更

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **将 buffer 溢出误判为网络问题** | 采集器日志显示发送失败，初步判断网络不通 | 实际是 ES 磁盘满导致拒绝写入，buffer 积压后溢出 | 先检查存储后端状态（D3.1-D3.3），再检查网络（D2.5） |
| **将日志轮转清理误判为日志丢失** | 用户报告某段时间日志查不到 | kubelet 日志轮转正常清理了旧日志，且该时段采集器未运行 | 确认 containerLogMaxFiles 和采集器运行历史，区分采集失败和正常轮转 |
| **将时区问题误判为日志延迟** | 日志平台显示的时间与实际相差数小时 | 应用时区、采集器时区、存储时区不一致 | 检查所有组件的时区配置，使用 UTC 标准化 |
| **将采集器正常背压误判为问题** | 高峰期短暂出现 backpressure 日志 | 流量高峰的正常行为，buffer 可以平滑处理 | 观察 buffer 是否持续增长，短暂波动可以忽略 |
| **将存储 yellow 状态误判为严重问题** | ES 状态 yellow，触发告警 | 新创建索引分片正在初始化，或单节点集群 replica 无法分配 | 检查 unassigned 原因，yellow 不一定影响写入 |
| **将 OOM 误判为配置问题** | 采集器频繁重启，日志显示内存相关错误 | 实际是日志量激增超出 limits，而非配置错误 | 结合 D1.4 资源使用和 D2.8 采集速率综合判断 |

### 10.2 深度知识引用

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| 日志管理架构与最佳实践 | `domain-06-observability/` | 理解日志管道设计原理 |
| 企业级监控告警系统 | `domain-10-troubleshooting-diagnostics/39-enterprise-monitoring-alerting-system.md` | 日志系统与监控集成 |
| Kubernetes 可观测性 | `domain-06-observability/` | 日志、指标、链路追踪的综合方案 |
| Elasticsearch 运维指南 | `domain-16-database-middleware/` | ES 集群深度运维 |
| 节点故障排查 | `SKILL-NODE-001` (01-node-notready.md) | 节点 DiskPressure 导致的日志问题 |
| 存储故障排查 | `SKILL-STORE-001` | 存储后端深度诊断 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-04 | v1.0 | 初始版本发布。覆盖 Fluentd/Fluent Bit/Vector 采集器，ES/Loki 存储后端，包含 13 个根因、11 个修复操作 | 基于日志相关工单分析和运维实践建立 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **Kafka 作为日志缓冲层**: 使用 Kafka 进行日志缓冲的故障诊断
2. **ClickHouse 日志存储**: ClickHouse 作为日志后端的特定问题
3. **多集群日志聚合**: 跨集群日志收集架构的故障排查
4. **日志采样与降级策略**: 高流量场景下的日志采样配置
5. **云厂商托管日志服务**: 阿里云 SLS、AWS CloudWatch Logs 等的集成问题
6. **安全审计日志合规**: 满足等保、GDPR 等合规要求的审计日志配置

## Related

- [[domain-19-landscape-references/topic-index/observability-index.md|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
