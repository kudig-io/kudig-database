---
title: 可观测性 生产就绪运维指南
description: 面向生产环境的 Kubernetes 可观测性体系检查、风险缓解与日常运维完整手册
summary: 面向生产环境的 Kubernetes 可观测性体系检查、风险缓解与日常运维完整手册
category: observability
tags:
- production
- best-practices
- observability
- operations
- monitoring
- logging
- tracing
- alerting
- slo
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- 可观测性 生产就绪运维指南是什么
- 如何按生产环境要求运维 可观测性
trigger_keywords:
- 生产就绪
- 运维指南
- 可观测性
- observability
- Prometheus
- Grafana
- SLO
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 可观测性 生产就绪运维指南

> **适用版本**: v1.28 - v1.33 | **最后更新**: 2026-07 | **适用角色**: SRE / 平台工程师 / 监控工程师

本指南从生产就绪（Production Readiness）视角，梳理 Kubernetes 可观测性体系在上线前、日常运维、故障响应和跨团队协作中的关键检查点与操作命令。核心覆盖指标、日志、链路、告警、SLO/SLI 五大支柱，并强调可观测性平台自身的稳定性与可恢复性。

生产就绪的三条铁律在此域尤为重要：第一，可观测性平台不能依赖被观测对象来发现自身故障，因此必须建立自监控与外部探测；第二，任何配置变更都必须可回滚，Dashboard、告警规则、采集配置全部纳入 GitOps；第三，数据保留、成本、合规需要在设计阶段明确，而不是事后补救。

---

## 1. 生产环境检查清单

在将可观测性体系接入生产集群前，必须逐项确认以下内容。清单按支柱分组，便于在 PRR（Production Readiness Review）中使用。

### 1.1 指标（Metrics）

- [ ] **Prometheus 高可用**：至少 2 个副本，通过 Thanos Sidecar / Remote Write 实现数据持久化，避免单点故障导致历史指标丢失。
- [ ] **抓取目标完整性**：kube-state-metrics、node-exporter、cAdvisor、APIServer、etcd、kubelet、CoreDNS 等核心组件全部被采集，无 `down` 目标。
- [ ] **标签基数治理**：单指标 cardinality 不超过 10 万，job 维度异常增长时触发告警。参考后续第 2 节风险 1。
- [ ] **保留策略与容量匹配**：`retentionSize` 小于 PVC 容量的 80%，本地盘 Prometheus 配置 15-30 天，长期存储使用 Thanos / Cortex / VictoriaMetrics。
- [ ] **Recording Rules 已启用**：核心大盘查询使用预聚合规则，P99 查询延迟 < 1 秒。

### 1.2 日志（Logging）

- [ ] **日志收集率 > 99%**：kube-system、业务命名空间、审计日志（Audit Log）均接入采集，不存在因采集 Agent Crash 导致的空窗期。
- [ ] **日志解析规范化**：容器标准输出按 `timestamp level msg` 结构化，关键业务字段统一提取为索引标签。
- [ ] **日志保留与分级存储**：热存储 7 天、温存储 30 天、冷存储按合规要求保留 180 天以上，且成本可量化。
- [ ] **采集 Agent 资源限制**：Fluent Bit / Fluentd / Promtail 设置 CPU/Memory limits，避免节点日志洪峰拖垮节点。

### 1.3 链路追踪（Tracing）

- [ ] **OpenTelemetry Collector 高可用**：Deployment 多副本 + HPA，接收端配置批处理与背压（Backpressure）。
- [ ] **采样策略可配置**：生产环境默认 1%-10% 头部采样，错误链路 100% 保留，避免存储爆炸。
- [ ] **Trace ID 贯通**：入口网关、业务 Pod、数据库/中间件调用链 Trace ID 一致，能够在日志中关联跳转。

### 1.4 告警（Alerting）

- [ ] **告警分级与路由**：critical / warning / info 三级，critical 在五分钟内到达 On-Call 工程师（PagerDuty/电话）。
- [ ] **告警可行动化**：每个告警必须附带 Runbook 链接、确认命令和影响面评估，禁止“CPU 高”这类无上下文告警。
- [ ] **抑制与去重生效**：Alertmanager `inhibit_rules`、`group_by` 配置经过演练，避免告警风暴。

### 1.5 SLO/SLI 与平台自身可观测性

- [ ] **SLO 已定义并落地**：核心服务 Latency、Error Rate、Throughput SLI 已接入 Prometheus，错误预算（Error Budget）政策已发布。
- [ ] **可观测性栈自监控**：Prometheus、Grafana、Alertmanager、Loki、Collector 自身均被监控，自身故障先于业务故障被发现。
- [ ] **Runbook 与 Dashboard 一一对应**：每个 critical 告警至少对应 1 个 Grafana Dashboard 和 1 个排障 Runbook。
- [ ] **成本与配额可视化**：按命名空间、团队、环境维度展示可观测性存储与查询成本，避免账单失控。
- [ ] **灾备与可恢复性验证**：每季度执行一次 Prometheus/Loki/Grafana 配置恢复演练，确认备份可用、RTO < 30 分钟。

---

## 2. 关键风险与缓解措施

### 风险 1：指标基数（Cardinality）爆炸

**影响**：Prometheus 内存、磁盘和查询延迟急剧上升，可能导致 OOMKilled 或查询超时，进而丢失所有监控能力。

**缓解命令与配置**：

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 1. 扫描高基数指标（top 20）
curl -sG 'http://prometheus:9090/api/v1/label/__name__/values' | jq -r '.data[]' | \
  while read m; do
    count=$(curl -sG 'http://prometheus:9090/api/v1/series' --data-urlencode "match[]=$m" | jq '.data | length')
    echo "$count $m"
  done | sort -rn | head -20

# 2. 在 Prometheus 中限制标签数量
prometheus:
  prometheusSpec:
    tsdb:
      outOfOrderTimeWindow: 0
    # Prometheus v2.53+ / v3.x 支持启用指标限制
    enableFeatures:
    - memory-snapshot-on-shutdown
```
**生产建议**：

- 禁止客户端暴露无界标签（如 `user_id`、`request_id`、`trace_id`）。
- 使用 `metric_relabel_configs` 丢弃或聚合高基数指标。
- 在 CI 中启用 `promtool check metrics`，对应用暴露的指标做基数扫描。

### 风险 2：可观测性平台自身单点故障

**影响**：监控、日志、告警自身不可用，导致生产故障无法被及时发现和处理。

**缓解措施**：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 Prometheus / Alertmanager / Grafana Pod 副本分布
kubectl get pods -n monitoring -o wide -l "app.kubernetes.io/name in (prometheus,alertmanager,grafana)"

# 2. 确认 Alertmanager 集群成员状态
kubectl exec -n monitoring alertmanager-0 -- amtool --alertmanager.url=http://localhost:9093 cluster status

# 3. 检查对象存储备份（Thanos / Loki / Tempo）
kubectl get secret -n monitoring thanos-objstore -o yaml | grep bucket
```
**生产建议**：

- Prometheus 使用 StatefulSet + PVC，至少 2 副本跨可用区部署，单个实例故障时由 Service 自动切换查询流量。
- Alertmanager 3 副本组成 Gossip 集群，配置 Pod AntiAffinity，避免单节点故障导致告警通知中断。
- Grafana 使用外部 PostgreSQL/MySQL 存储配置，Dashboard 使用 GitOps 版本化管理，禁止直接在生产环境手动修改大盘。
- 在对象存储侧启用跨区域复制或版本控制，确保 Thanos、Loki、Tempo 的长期数据在区域级故障时可恢复。

### 风险 3：日志采集延迟或丢失

**影响**：故障现场日志缺失，无法定位根因；审计日志缺失可能带来合规风险。

**缓解命令与配置**：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查日志采集 Agent 运行状态
kubectl get ds -n logging
kubectl top pods -n logging --sort-by=memory

# 2. 检查 Loki / Elasticsearch 索引与写入速率
kubectl logs -n logging -l app=loki-distributor --tail=100 | grep -i "rate\|error"

# 3. Fluent Bit 输出缓冲区限制示例
[OUTPUT]
    Name            loki
    Match           kube.*
    Host            loki-gateway
    Port            80
    Retry_Limit     5
    storage.type    filesystem
    storage.path    /var/log/flb-storage
```
**生产建议**：

- 采集 Agent 使用 DaemonSet，配置 `resources.limits` 和 `priorityClassName: system-node-critical`。
- 后端写入失败时启用本地磁盘缓冲，避免日志丢失。
- 审计日志独立存储，禁止与业务日志共用同一后端租户。

### 风险 4：告警疲劳导致关键告警被淹没

**影响**：工程师对告警麻木，P0 告警响应延迟，最终造成事故升级。

**缓解命令与配置**：

```yaml
# Alertmanager 分组与抑制示例
route:
  group_by: ['alertname', 'namespace', 'severity']
  group_wait: 30s
  group_interval: 5m
  repeat_interval: 12h
  routes:
  - match:
      severity: critical
    receiver: pagerduty-critical
    continue: false
inhibit_rules:
- source_match:
    severity: critical
  target_match:
    severity: warning
  equal: ['namespace', 'alertname']
```

**生产建议**：

- 每周召开告警质量会议，目标 critical 告警 < 5 条/周，false positive < 5%，对连续 3 次误报的告警必须修改阈值或下线。
- 使用 `alertmanager-config-reloader` 做配置校验，避免路由错误；重大变更先在 staging Alertmanager 验证再切流量。
- 对高频 warning 告警优先落地自动化修复（如自动重启卡死 Pod、自动扩容 HPA），而不是依赖人工响应。
- 建立“静默告警”审计机制，所有手工 silences 必须记录工单号、预期恢复时间和责任人，防止静默过期后问题复发。

---

## 3. 日常运维操作

### 3.1 每日巡检

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 可观测性组件健康状态
kubectl get pods -n monitoring -o wide
kubectl get pods -n logging -n tracing -o wide 2>/dev/null

# 2. Prometheus 目标状态
kubectl port-forward svc/prometheus-k8s 9090:9090 -n monitoring &
curl -s http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | {job, health, lastError}'

# 3. 今日告警统计（按严重程度）
curl -s 'http://alertmanager:9093/api/v1/alerts' | \
  jq -r '.data[] | .labels.severity' | sort | uniq -c | sort -rn

# 4. SLO 错误预算消耗
kubectl port-forward svc/grafana 3000:3000 -n monitoring &
# 在 Grafana 打开 SLO Dashboard，检查本周 error budget 消耗是否 < 20%
```
### 3.2 容量管理

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. Prometheus TSDB 大小
kubectl exec -n monitoring prometheus-k0 -- du -sh /prometheus

# 2. Loki / Elasticsearch 存储增长趋势
kubectl exec -n logging loki-0 -- df -h /data

# 3. 查询负载 Top 10
curl -s 'http://prometheus:9090/api/v1/status/runtimeinfo' | jq '.data | {queryEngine: .queryEngine}'
```
### 3.3 配置变更

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. Helm 升级前 diff
helm diff upgrade kube-prometheus-stack prometheus-community/kube-prometheus-stack \
  -n monitoring -f values-production.yaml

# 2. 更新 PrometheusRule
kubectl apply -f rules/kubernetes-apps.yaml
promtool check rules rules/kubernetes-apps.yaml

# 3. 重新加载 Grafana Dashboard
# 推荐通过 GitOps（Argo CD/Flux）自动同步，避免手动导入
```
### 3.4 备份与恢复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 备份 Grafana 配置（如使用外部数据库则按 DB 备份策略）
kubectl get configmaps -n monitoring -l grafana_dashboard=1 -o yaml > grafana-dashboards-backup.yaml

# 2. 备份 Alertmanager 配置
kubectl get secret alertmanager-kube-prometheus-stack-alertmanager -n monitoring -o yaml > alertmanager-secret-backup.yaml

# 3. Thanos / Loki 对象存储 bucket 备份策略由云厂商生命周期管理覆盖
```
### 3.5 应急演练与混沌工程

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 模拟 Prometheus 单副本故障，验证 Thanos Query 是否仍然返回完整数据
kubectl scale sts prometheus-k8s --replicas=1 -n monitoring
# 观察 5 分钟后恢复为 2 副本

# 2. 模拟 Alertmanager 全故障，验证是否能在 5 分钟内通过备用通道（如云监控）收到核心告警
kubectl scale sts alertmanager --replicas=0 -n monitoring

# 3. 使用 Litmus/Chaos Mesh 对 logging namespace 注入 Pod 故障
kubectl apply -f experiments/logging-pod-kill.yaml
```
**演练目标**：核心告警在平台组件单点故障时仍能发出；历史指标和日志查询在单副本降级时不受影响；演练后更新 Runbook 和 On-Call Playbook。

---

## 4. 故障排查速查

| 现象 | 可能原因 | 确认命令 | 修复/缓解 |
|------|---------|---------|----------|
| Grafana 所有 Dashboard 无数据 | Prometheus 宕机或数据源配置错误 | `kubectl get pods -n monitoring -l app.kubernetes.io/name=prometheus`<br>`curl http://prometheus:9090/api/v1/status/runtimeinfo` | 恢复 Prometheus Pod；检查 Grafana DataSource URL 与 TLS |
| Prometheus OOMKilled | 高基数指标 / 抓取目标过多 / retention 过大 | `kubectl describe pod -n monitoring prometheus-k0`<br>`curl .../api/v1/label/__name__/values` 后计算 cardinality | 增加 memory limit；添加 relabel 过滤；缩短 retention；分片 Prometheus |
| Alertmanager 未发送通知 | 路由不匹配 / receiver 配置错 / inhibit 误杀 | `kubectl logs -n monitoring alertmanager-0`<br>`amtool config routes test` | 校验 alertmanager.yml；关闭错误 inhibit_rule；测试 receiver |
| 日志搜索为空 | Fluent Bit 未启动 / Loki 索引过期 / 标签不匹配 | `kubectl get ds -n logging`<br>`kubectl logs -n logging fluent-bit-xxxxx`<br>`logcli labels` | 重启 DaemonSet；检查 retention；确认 label selector |
| 链路追踪采样为 0 | Collector 采样策略 0% / Span 未上报 / 网络策略拦截 | `kubectl logs -n tracing otel-collector-0`<br>`curl otel-collector:8888/metrics` | 调整 sampling 配置；检查 OTLP endpoint；放行网络策略 |
| SLO Dashboard 显示错误预算耗尽 | 真实服务降级 / SLI 计算错误 / 阈值过严 | `kubectl get prometheusrules -n monitoring`<br>`promtool query instant ...` | 按真实业务影响 review SLI；如确为服务问题则启动事故响应 |
| 某个节点指标缺失 | node-exporter Pod 未就绪 / kubelet 指标接口不可达 | `kubectl get pods -n monitoring -l app.kubernetes.io/name=node-exporter -o wide`<br>`curl https://<node>:10250/metrics` | 重启 node-exporter；检查 kubelet 证书与网络策略 |

---

## 5. 与其他域的协作边界

可观测性不是孤立体系，必须与相邻域保持清晰边界和协作接口。

- **与 [[集群基础/README.md|集群基础域]] 协作**：控制平面组件（API Server、etcd、Scheduler、Controller Manager）的健康指标由集群域负责部署和升级，可观测性域负责采集、告警和 Dashboard。参考 [[可观测性/总览/13-cluster-health-check.md|集群健康检查指南]]。
- **与 [[网络/README.md|网络域]] 协作**：CNI、CoreDNS、Ingress、Service Mesh 的网络延迟与丢包指标由网络域提供解释，可观测性域负责统一呈现和跨域关联。网络策略变更前需确认不会阻断 Prometheus / Loki / OTLP 流量。
- **与 [[安全/README.md|安全合规域]] 协作**：审计日志、Falco 运行时事件、RBAC 变更日志需要进入 SIEM/SOAR。可观测性域负责采集与转发，安全域负责策略、归档与合规响应。参考 [[可观测性/日志/08-logging-audit-compliance.md|日志审计合规]]。
- **与 [[平台工程/README.md|平台工程域]] 协作**：平台工程负责可观测性平台的部署、租户隔离、成本分摊和 GitOps 版本管理；可观测性域负责使用规范、SLO 定义和告警质量治理。
- **与 [[可靠性/README.md|可靠性工程域]] 协作**：SLO/SLI、错误预算、混沌工程实验由可靠性域主导设计，可观测性域提供数据基础和告警触发能力。参考 [[可观测性/SLO-SLI/18-slo-sli-system.md|SLO/SLI 体系建设]]。
- **与 [[故障诊断/README.md|故障排查域]] 协作**：可观测性数据是故障排查的入口，复杂场景（如内核、多租户、网络分区）由排查域提供深度工具链（eBPF、kubectl debug、inspektor-gadget）。

---

## 6. 推荐阅读

### 本域核心文档

- [[可观测性/README.md|Domain 06 — Observability（可观测性）]]
- [[可观测性/总览/13-cluster-health-check.md|集群健康检查指南]]
- [[可观测性/指标/99-prometheus-enterprise-guide.md|Prometheus 企业级监控部署指南]]
- [[可观测性/告警/21-monitoring-playbooks.md|监控 Playbooks]]
- [[可观测性/SLO-SLI/18-slo-sli-system.md|SLO/SLI 体系建设与管理]]
- [[可观测性/工具/26-troubleshooting-tools.md|可观测性排障工具]]

### 相关域文档

- [[集群基础/README.md|集群基础架构]]
- [[安全/README.md|安全合规]]
- [[可靠性/README.md|可靠性工程]]
- [[故障诊断/README.md|故障排查与诊断]]

### 待补充方向（参考 Gap Analysis）

以下主题在本 Domain 当前覆盖中仍为缺口，建议后续补充为独立文档：Prometheus 指标基数治理、eBPF 可观测性、合成监控（Blackbox Exporter）、可观测性平台自监控 Runbook、GPU/AI 工作负载监控、多租户可观测性隔离。

---

## See Also

- [[可观测性/README.md|返回 Domain 06 目录]]
- [[生产运维/README.md|生产运维域]]
- [[_reports/domain-content-gap-analysis-2026-07-01.md|Domain Content Gap Analysis 2026-07-01]]


<!-- risk-assessed -->
