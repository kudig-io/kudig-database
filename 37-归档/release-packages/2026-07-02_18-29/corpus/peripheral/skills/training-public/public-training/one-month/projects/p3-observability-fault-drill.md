---
title: '项目 P3: 可观测性体系搭建 + 故障演练'
description: '- prometheus grafana loki alertmanager 完整部署'
summary: '- prometheus grafana loki alertmanager 完整部署'
category: learning
tags:
- k8s
- training
- hands-on
- apiserver
- kubelet
- prometheus
- grafana
- jaeger
- coredns
- networkpolicy
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- '项目 P3: 可观测性体系搭建 + 故障演练 是什么'
- '如何 项目 P3: 可观测性体系搭建 + 故障演练'
trigger_keywords:
- 项目
- 'P3:'
- 可观测性体系搭建
- 故障演练
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
- logging-basics
- tracing-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: 项目 P3: 可观测性体系搭建 + 故障演练
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[Prometheus|prometheus]]us and Grafana|prometheus grafana]] loki alertmanager 完整部署
  - k8s 可观测性体系搭建步骤
  - 故障注入演练 fta febm 方法论
  - kube-prometheus-stack 部署配置
trigger_keywords:
  - Prometheus
  - Grafana
  - Loki
  - Alertmanager
  - PrometheusRule
  - 可观测性
  - 故障演练
  - FTA
  - FEBM
  - 监控告警
reading_level: intermediate
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 150min
related_domains:
  - domain-06-observability
  - domain-10-troubleshooting-diagnostics
  - domain-20-enterprise-monitoring-alerting
  - topic-fta
  - topic-febm
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-17-observability-1
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-18-observability-2
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-19-troubleshooting-methodology
---

# 项目 P3: 可观测性体系搭建 + 故障演练

> **所属周**: Week 3 | **预计时间**: 2.5 小时

---

## 概述

本实践项目要求你搭建一个完整的可观测性体系（监控 + 日志 + 告警），然后通过故障注入演练来验证监控告警的有效性，并运用 FTA/FEBM 方法论进行结构化排查。可观测性是生产运维的"眼睛"——没有完善的监控和日志体系，故障排查就如同盲人摸象。

### 项目目标

搭建完整的可观测性体系，并进行故障注入演练：
- 监控: Prometheus + Grafana Dashboard
- 日志: Loki + Promtail
- 告警: Alertmanager + PrometheusRule
- 故障演练: 注入 3 类问题并按 FTA/FEBM 方法排查

### 前置条件

- 已完成 Week 3 Day 15-20 的学习
- 已部署 kube-prometheus-stack
- 已部署 Loki

---

## 核心概念回顾

### 可观测性三支柱

| 支柱 | 工具 | 关注点 | 典型使用场景 |
|------|------|--------|-------------|
| 指标（Metrics） | Prometheus | 系统状态的时间序列数据 | CPU 使用率、请求延迟、错误率 |
| 日志（Logs） | Loki + Promtail | 事件记录的文本数据 | 错误日志、访问日志、审计日志 |
| 追踪（Traces） | Jaeger / Tempo | 请求在分布式系统中的流转路径 | 微服务调用链、性能瓶颈定位 |

### PrometheusRule 告警设计原则

- **for 子句**: 持续时间阈值，避免瞬时抖动触发告警
- **severity 标签**: 区分告警级别（critical / warning / info）
- **annotations**: 包含摘要和详细描述，方便值班人员快速理解
- **表达式**: 使用 PromQL，关注比率而非绝对值

### FTA/FEBM 排查方法论

**FTA（故障树分析）** 将问题分解为层次化的因果关系，从顶层问题向下追溯可能的根本原因。

**FEBM（取证循证方法）** 强调先收集证据（日志、指标、事件），然后基于证据形成假设，再通过实验验证假设。

---

## 项目步骤

### Step 1: 确认监控组件 (15min)

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 Prometheus
kubectl get pods -n monitoring -l app.kubernetes.io/name=prometheus
# 预期输出:
# NAME                                   READY   STATUS    RESTARTS   AGE
# prometheus-monitoring-kube-prometheus-0 2/2     Running   0          1d

# 检查 Grafana
kubectl get pods -n monitoring -l app.kubernetes.io/name=grafana
# 预期输出:
# NAME                                   READY   STATUS    RESTARTS   AGE
# monitoring-grafana-6b9c8d8f7c-abc12    1/1     Running   0          1d

# 检查 Loki
kubectl get pods -n monitoring -l app=loki
# 预期输出:
# NAME         READY   STATUS    RESTARTS   AGE
# loki-0       1/1     Running   0          1d

# 访问 Grafana
kubectl port-forward -n monitoring svc/prometheus-grafana 3000:80
# 浏览器访问: http://localhost:3000
# 默认用户名: admin
# 默认密码: prom-operator（或安装时设置的密码）

# 验证 Prometheus 采集目标
kubectl get servicemonitors -n monitoring
# 预期输出:
# NAME                                  AGE
# monitoring-kube-prometheus-alertmanager  1d
# monitoring-kube-prometheus-apiserver     1d
# monitoring-kube-prometheus-coredns       1d
# monitoring-kube-prometheus-kubelet       1d
# monitoring-kube-prometheus-nodeexporter  1d
# monitoring-kube-prometheus-prometheus    1d
```
### Step 2: 配置告警规则 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > core-alerts.yaml << 'EOF'
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: core-alerts
  namespace: monitoring
  labels:
    prometheus: kube-prometheus
    role: alert-rules
spec:
  groups:
  - name: pod-alerts
    rules:
    - alert: PodCrashLooping
      expr: rate(kube_pod_container_status_restarts_total[15m]) * 60 * 5 > 0
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} is crash looping"
        description: "Pod {{ $labels.namespace }}/{{ $labels.pod }} (container {{ $labels.container }}) has restarted {{ $value }} times in the last 5 minutes."
    
    - alert: PodNotReady
      expr: kube_pod_status_phase{phase="Running"} == 1 and on(pod, namespace) kube_pod_status_ready{condition="false"} == 1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} is not ready"
        description: "Pod {{ $labels.namespace }}/{{ $labels.pod }} has been in non-ready state for more than 10 minutes."
    
    - alert: PodOOMKilled
      expr: kube_pod_container_status_terminated_reason{reason="OOMKilled"} == 1
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} was OOMKilled"
        description: "Container {{ $labels.container }} in Pod {{ $labels.namespace }}/{{ $labels.pod }} was terminated due to OOM."
  
  - name: node-alerts
    rules:
    - alert: HighCPUUsage
      expr: (1 - avg(rate(node_cpu_seconds_total{mode="idle"}[5m])) by (instance)) > 0.9
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Node {{ $labels.instance }} CPU usage > 90%"
        description: "Node {{ $labels.instance }} has CPU usage of {{ $value | humanizePercentage }} for the last 5 minutes."
    
    - alert: HighMemoryUsage
      expr: (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) > 0.9
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Node {{ $labels.instance }} memory usage > 90%"
        description: "Node {{ $labels.instance }} has memory usage of {{ $value | humanizePercentage }} for the last 5 minutes."
    
    - alert: DiskSpaceLow
      expr: (1 - node_filesystem_avail_bytes{fstype!~"tmpfs|overlay"} / node_filesystem_size_bytes{fstype!~"tmpfs|overlay"}) > 0.85
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Node {{ $labels.instance }} disk usage > 85%"
        description: "Filesystem {{ $labels.mountpoint }} on {{ $labels.instance }} is {{ $value | humanizePercentage }} full."
  
  - name: application-alerts
    rules:
    - alert: HighErrorRate
      expr: sum(rate(http_requests_total{code=~"5.."}[5m])) / sum(rate(http_requests_total[5m])) > 0.05
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "HTTP error rate > 5%"
        description: "The HTTP 5xx error rate is {{ $value | humanizePercentage }}."
    
    - alert: HighLatency
      expr: histogram_quantile(0.99, sum(rate(http_request_duration_seconds_bucket[5m])) by (le)) > 2
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "P99 latency > 2s"
        description: "The 99th percentile latency is {{ $value }}s."
EOF

kubectl apply -f core-alerts.yaml
# 预期输出: prometheusrule.monitoring.coreos.com/core-alerts created

# 验证规则已加载
kubectl get prometheusrule -n monitoring
# 预期输出:
# NAME           AGE
# core-alerts    10s

# 在 Prometheus UI 中验证规则
kubectl port-forward -n monitoring svc/prometheus-operated 9090:9090
# 浏览器访问: http://localhost:9090/alerts
```
### Step 3: 配置 Grafana Dashboard (30min)

在 Grafana 中导入以下 Dashboard（通过 "+" → "Import" 输入 ID）：

1. **K8s 集群监控** (ID: 315)
   - 节点资源使用（CPU/内存/磁盘/网络）
   - Pod 数量统计
   - 网络流量和连接数

2. **K8s Pod 监控** (ID: 6417)
   - Pod CPU/内存使用趋势
   - 容器重启次数
   - 网络 IO 统计
   - 存储 IO 统计

3. **Loki 日志** (ID: 13639)
   - 日志查询面板
   - 日志速率统计
   - 错误日志过滤

4. **Alertmanager 状态** (ID: 15760)
   - 活跃告警列表
   - 告警触发趋势
   - 告警静默状态

### Step 4: 创建故障演练环境 (15min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建测试 namespace
kubectl create namespace fault-drill
# 预期输出: namespace/fault-drill created

# 部署测试应用
kubectl create deployment app --image=nginx:alpine -n fault-drill --replicas=3
# 预期输出: deployment.apps/app created

kubectl expose deployment app --port=80 -n fault-drill
# 预期输出: service/app exposed

# 验证测试环境
kubectl get all -n fault-drill
# 预期输出:
# NAME                       READY   STATUS    RESTARTS   AGE
# pod/app-6d4f7b8c9d-abc12   1/1     Running   0          1m
# pod/app-6d4f7b8c9d-def34   1/1     Running   0          1m
# pod/app-6d4f7b8c9d-ghi56   1/1     Running   0          1m
# 
# NAME          TYPE        CLUSTER-IP      EXTERNAL-IP   PORT(S)   AGE
# service/app   ClusterIP   10.96.100.200   <none>        80/TCP    30s
```
### Step 5: 故障注入与排查 (45min)

#### 问题 1: OOMKilled

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 注入问题：创建一个内存使用超过 limits 的 Pod
cat > oom-inject.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: oom-inject
  namespace: fault-drill
  labels:
    fault-type: oom
spec:
  containers:
  - name: stress
    image: polinux/stress
    command: ["stress"]
    args: ["--vm", "1", "--vm-bytes", "200M", "--vm-hang", "5"]
    resources:
      limits:
        memory: 100Mi
      requests:
        memory: 50Mi
EOF

kubectl apply -f oom-inject.yaml
# 预期输出: pod/oom-inject created

# 排查步骤（使用 FEBM 方法）:
# 1. 收集证据
kubectl get pod oom-inject -n fault-drill
# 预期输出: CrashLoopBackOff 或 OOMKilled

kubectl describe pod oom-inject -n fault-drill
# 重点关注: Last State: Terminated, Reason: OOMKilled, Exit Code: 137

# 2. 在 Grafana 中验证（证据链）:
#    - Container memory usage → 看到 usage 达到 100Mi limit
#    - OOMKilled 事件在 Events 面板中可见
#    - Prometheus 告警 PodOOMKilled 应被触发

# 3. 在 Loki 中查询日志:
#    {namespace="fault-drill", pod="oom-inject"}

# 4. 根因: limits.memory=100Mi < 应用实际需求 200M
# 5. 修复: 增加内存限制到 256Mi
```
#### 问题 2: CrashLoopBackOff

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 注入问题：创建一个启动即退出的容器
kubectl run crash-app --image=busybox -n fault-drill -- /bin/sh -c "echo 'Error: something went wrong' >&2 && exit 1"
# 预期输出: pod/crash-app created

# 排查步骤:
# 1. 收集证据 - 查看 Pod 状态
kubectl get pod crash-app -n fault-drill
# 预期输出:
# NAME        READY   STATUS             RESTARTS   AGE
# crash-app   0/1     CrashLoopBackOff   3          2m

# 2. 查看上一次运行的日志（关键！）
kubectl logs crash-app -n fault-drill --previous
# 预期输出: Error: something went wrong

# 3. 查看详情
kubectl describe pod crash-app -n fault-drill | grep -A 10 "Last State"
# 预期输出:
# Last State:     Terminated
#   Reason:       Error
#   Exit Code:    1
#   Started:      ...
#   Finished:     ...

# 4. 在 Prometheus 中验证: PodCrashLooping 告警应被触发
# 5. 根因: 容器启动命令执行失败（exit code 1）
# 6. 修复: 修正容器启动命令
```
#### 问题 3: Service 不可访问

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 注入问题：删除 Endpoints（模拟 selector 不匹配）
kubectl delete endpoints app -n fault-drill
# 预期输出: endpoints "app" deleted

# 排查步骤:
# 1. 收集证据 - 测试 Service 可达性
kubectl run curl-test --image=busybox -n fault-drill --rm -it --restart=Never -- \
  wget -qO- --timeout=5 http://app.fault-drill.svc.cluster.local 2>&1 || echo "Connection failed"
# 预期输出: Connection failed 或 wget: download error

# 2. 检查 Endpoints
kubectl get endpoints app -n fault-drill
# 预期输出: No resources found 或 ENDPOINTS 为空

# 3. 检查 Service selector
kubectl describe svc app -n fault-drill | grep Selector
# 预期输出: Selector:          app=app

# 4. 检查 Pod 标签
kubectl get pods -n fault-drill -l app=app --show-labels
# 预期输出: 应该能看到匹配的 Pod

# 5. 检查是否有网络策略阻止
kubectl get networkpolicy -n fault-drill

# 6. 根因: Endpoints 被手动删除（或 selector 与 Pod labels 不匹配）
# 7. 修复: 重新创建 Service 或修正 selector
# 快速恢复: 删除并重建 Service
kubectl delete svc app -n fault-drill
kubectl expose deployment app --port=80 -n fault-drill
```
### Step 6: 编写故障排查报告 (15min)

使用 FEBM 方法记录排查过程：

```markdown
## 问题报告: OOMKilled

### 1. 问题现象
- **发现时间**: 2024-01-15 10:30
- **发现方式**: Prometheus PodOOMKilled 告警
- **症状**: Pod crash-app 处于 CrashLoopBackOff 状态
- **影响范围**: fault-drill namespace

### 2. 证据收集
| 证据来源 | 证据内容 |
|---------|---------|
| kubectl get pod | 状态 CrashLoopBackOff，RESTARTS=5 |
| kubectl describe pod | Last State: OOMKilled, Exit Code: 137 |
| Grafana | Container memory usage 达到 100Mi limit |
| Prometheus 告警 | PodOOMKilled (severity: critical) |
| Loki 日志 | 无应用错误日志（进程被 OOM Kill 直接终止） |

### 3. 根因分析
- **直接原因**: 容器使用内存超过 limits.memory (100Mi)
- **根本原因**: stress 工具配置 --vm-bytes=200M 超过容器限制
- **触发条件**: stress --vm-bytes=200M 在 100Mi limit 的容器中运行

### 4. 修复方案
- 短期: 增加 memory limits 到 256Mi
- 长期: 通过压力测试确定应用的实际内存需求，设置合理的 limits

### 5. 预防措施
- 添加内存使用率告警（80% warning, 90% critical）
- 新应用上线前进行内存压力测试
- 在 CI/CD 中检查 limits 是否设置合理
```

---

## 配置示例

### ServiceMonitor 自定义应用指标采集

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: app-metrics
  namespace: fault-drill
  labels:
    release: monitoring
spec:
  selector:
    matchLabels:
      app: app
  endpoints:
  - port: http
    path: /metrics
    interval: 15s
  namespaceSelector:
    matchNames:
    - fault-drill
```

### PodMonitor 自定义指标采集

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PodMonitor
metadata:
  name: custom-app
  namespace: monitoring
  labels:
    release: monitoring
spec:
  selector:
    matchLabels:
      app: custom-app
  podMetricsEndpoints:
  - port: metrics
    path: /metrics
    interval: 30s
```

---

## 常见问题

### Q1: PrometheusRule 创建了但告警没触发？

检查步骤：1）`kubectl get prometheusrule -n monitoring` 确认规则已创建；2）访问 Prometheus UI → Alerts 页面确认规则已加载；3）检查 PromQL 表达式是否正确（在 Prometheus UI → Graph 中测试）；4）检查 `for` 持续时间是否太长。

### Q2: Loki 查询语法是什么？

Loki 使用 LogQL 查询语言。基本语法：`{label="value"}`（日志流选择器），例如 `{namespace="fault-drill", pod="oom-inject"}`。支持过滤：`{app="nginx"} |= "error"`（包含 error），`{app="nginx"} |~ "error|warn"`（正则匹配），`{app="nginx"} != "debug"`（不包含 debug）。

### Q3: 如何模拟网络问题？

可以使用 `tc`（Traffic Control）工具在节点上注入网络延迟和丢包。或者使用 Chaos Mesh / Litmus 等混沌工程工具来注入更复杂的问题场景（如 Pod 删除、网络分区、IO 问题等）。

### Q4: 告警太多怎么降噪？

推荐策略：1）合理设置 `for` 持续时间（避免瞬时抖动触发告警）；2）使用 Alertmanager 的 `group_by` 将相关告警聚合；3）配置 `inhibit_rules` 让高优先级告警抑制低优先级告警；4）在非关键时段使用静默（silence）。

---

## 验收清单

- [ ] 告警规则配置成功并已加载到 Prometheus
- [ ] Grafana Dashboard 可以展示监控数据
- [ ] Loki 可以查询日志
- [ ] 成功注入 3 类问题（OOMKilled、CrashLoopBackOff、Service 不可达）
- [ ] 按 FTA/FEBM 方法完成排查
- [ ] 完成故障排查报告

---

## 要点总结

| 步骤 | 内容 | 工具 |
|------|------|------|
| 监控 | 指标采集和可视化 | Prometheus + Grafana |
| 日志 | 日志收集和查询 | Loki + Promtail |
| 告警 | 异常检测和通知 | PrometheusRule + Alertmanager |
| 故障注入 | 模拟生产问题 | kubectl + stress |
| 排查 | 结构化故障分析 | FTA/FEBM 方法论 |

---

## 清理资源

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl delete`：删除资源（可由声明式清单重建）

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
kubectl delete namespace fault-drill  # ⚠️ 不可逆：永久删除命名空间及全部资源
kubectl delete prometheusrule core-alerts -n monitoring
```
---

## 延伸阅读

- [Prometheus 企业级监控](../../domain-06-observability/01-prometheus-enterprise-monitoring.md)
- [FTA 故障树分析](../../../domain-10-troubleshooting-diagnostics/FTA故障树/04-fta-core-principles.md)
- [FEBM 取证循证方法](../../../domain-10-troubleshooting-diagnostics/FEBM方法论/01-febm-theory-foundations.md)
- [Pod 综合排障](32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/00-core-troubleshooting/07-pod-comprehensive-troubleshooting.md)

```

<!-- risk-assessed -->
