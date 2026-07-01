---
title: 'Day 18: 可观测性 - 日志 + 分布式追踪'
description: 'title: Day 18: 可观测性 - 日志 + 分布式追踪'
category: learning
tags:
- k8s
- training
- hands-on
- apiserver
- prometheus
- grafana
- helm
- containerd
- docker
- opa
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 18: 可观测性 - 日志 + 分布式追踪 是什么'
- '如何 Day 18: 可观测性 - 日志 + 分布式追踪'
trigger_keywords:
- Day
- '18:'
- 可观测性
- 日志
- 分布式追踪
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- policy-basics
- logging-basics
created: "2026-05-23"
---

---
title: Day 18: 可观测性 - 日志 + 分布式追踪
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[Kubernetes|kubernetes]] 日志聚合方案
  - Loki 日志系统
  - ELK 企业日志
  - 分布式链路追踪
  - Alertmanager 告警路由
trigger_keywords:
  - Loki
  - ELK
  - 日志
  - LogQL
  - 分布式追踪
  - Trace
  - Alertmanager
  - 可观测性
reading_level: intermediate
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 240min
related_domains:
  - domain-06-observability
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-17-observability-1
  - domain-11-production-operations/topic-learn/public-training/one-month/week-4-enterprise/day-22-enterprise-monitoring
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
---

# Day 18: 可观测性 - 日志 + 分布式追踪

> **学习时间**: 4-5 小时 | **主题**: 日志聚合与链路追踪

---

## 概述

可观测性（Observability）是现代云原生运维的三大支柱之一，而日志和分布式追踪是其中最关键的两个组成部分。在 Kubernetes 生产环境中，应用运行在大量动态变化的 Pod 中，传统的 SSH 登录查看日志的方式已经完全不可行。你需要一套集中化的日志采集、存储和查询系统，以及能够追踪请求跨多个微服务流转路径的分布式追踪系统。

本课程将深入讲解 Kubernetes 日志架构的设计选择，手把手指导你部署 Loki + Promtail 日志系统，配置 Grafana 进行日志可视化查询，以及设置 Alertmanager 告警路由实现分级告警通知。通过本课程的学习，你将具备构建企业级日志平台和告警体系的能力。

**学习目标**：
- 理解 K8s 日志架构和采集方式选择
- 部署 Loki + Promtail 日志系统
- 掌握 LogQL 日志查询语言
- 配置 Alertmanager 告警路由规则

**前置条件**：
- 已完成 Day 17 的 [[Prometheus|Prometheus]] + Grafana 监控部署
- 有 Helm 基本操作能力
- 了解 Kubernetes 存储和网络基础

---

## 核心概念

### K8s 日志架构

Kubernetes 集群中的日志主要来自三个层面：容器标准输出/错误输出、容器内文件日志、以及 Kubernetes 系统组件日志。理解这些日志的来源和采集方式，是设计高效日志系统的基础。

#### 日志采集方式对比

| 采集方式 | 原理 | 优点 | 缺点 | 适用场景 |
|----------|------|------|------|---------|
| **DaemonSet** | 每个节点部署一个采集 Agent | 资源开销低、管理简单 | 无法精确控制采集配置 | 大多数生产场景 |
| **Sidecar** | 每个 Pod 注入一个采集容器 | 采集配置精细、支持文件日志 | 资源开销高、配置复杂 | 需要采集容器内文件的场景 |
| **应用直推** | 应用直接发送日志到中心 | 无额外组件、延迟低 | 需要改造应用 | 新项目、Serverless |

#### 日志存储方案对比

| 方案 | 存储引擎 | 查询语言 | 资源消耗 | 全文索引 | 适用规模 |
|------|---------|---------|---------|---------|---------|
| **Loki** | 对象存储/本地 | LogQL | 低 | 无（标签索引） | 中大型 |
| **Elasticsearch** | Lucene | KQL/Lucene | 高 | 有 | 大型/合规要求 |
| **ClickHouse** | 列式存储 | SQL | 中 | 无 | 结构化日志 |
| **阿里云 SLS** | 云存储 | SPL | 按量付费 | 有 | 阿里云用户 |

### Loki 架构详解

Grafana Loki 是一个受 Prometheus 启发的水平可扩展日志聚合系统。与 ELK 不同的是，Loki 不对日志全文建立索引，而是只对日志流的标签（labels）建立索引，这大大降低了存储成本和运维复杂度。

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Pod Logs  │────>│  Promtail   │────>│    Loki     │
│ (stdout/err)│     │ (DaemonSet) │     │  (Stateful) │
└─────────────┘     └─────────────┘     └──────┬──────┘
                                               │
                    ┌─────────────┐             │
                    │   Grafana   │<────────────┘
                    │ (Dashboard) │
                    └─────────────┘
```

### Alertmanager 路由机制

Alertmanager 是 Prometheus 生态中的告警管理和路由组件。它接收来自 Prometheus 的告警，经过分组（grouping）、抑制（inhibition）、静默（silencing）和路由（routing）处理后，发送到不同的接收端。

```
Prometheus ──> Alertmanager ──> Route ──> Receiver (钉钉/企微/Slack/Email)
                                  │
                                  ├── match: severity=critical ──> critical receiver
                                  ├── match: severity=warning ───> warning receiver
                                  └── default ──────────────────> default receiver
```

---

## 实战演练

### 任务 1: 部署 Loki Stack (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

```bash
# Step 1: 添加 Grafana Helm repo
helm repo add grafana https://grafana.github.io/helm-charts
helm repo update

# 预期输出:
# Hang tight while we grab the latest from your chart repositories...
# ...Successfully got an update from the "grafana" chart repository
# Update Complete. ⎈Happy Helming!⎈

# Step 2: 查看可用的 chart 版本
helm search repo grafana/loki-stack --versions | head -10

# Step 3: 创建自定义 values 文件
cat > loki-values.yaml << 'EOF'
loki:
  enabled: true
  persistence:
    enabled: true
    size: 10Gi
    storageClassName: alicloud-disk-ssd
  config:
    limits_config:
      max_query_length: 721h
      max_entries_limit_per_query: 5000
      retention_period: 744h
    chunk_store_config:
      max_look_back_period: 720h
    table_manager:
      retention_deletes_enabled: true
      retention_period: 720h
  resources:
    requests:
      cpu: 200m
      memory: 256Mi
    limits:
      cpu: "1"
      memory: 1Gi

promtail:
  enabled: true
  config:
    lokiAddress: http://loki:3100/loki/api/v1/push
  resources:
    requests:
      cpu: 50m
      memory: 64Mi
    limits:
      cpu: 200m
      memory: 128Mi

grafana:
  enabled: false
EOF

# Step 4: 安装 Loki Stack
helm install loki grafana/loki-stack \
  --namespace monitoring \
  --create-namespace \
  --values loki-values.yaml \
  --set promtail.enabled=true

# 预期输出:
# NAME: loki
# LAST DEPLOYED: Mon May 18 10:30:00 2026
# NAMESPACE: monitoring
# STATUS: deployed
# REVISION: 1
# NOTES:
# The Loki stack has been deployed.

# Step 5: 验证部署
kubectl get pods -n monitoring -l app=loki

# 预期输出:
# NAME           READY   STATUS    RESTARTS   AGE
# loki-0         1/1     Running   0          2m

kubectl get pods -n monitoring -l app=promtail

# 预期输出:
# NAME              READY   STATUS    RESTARTS   AGE
# promtail-xxxxx    1/1     Running   0          2m
# promtail-yyyyy    1/1     Running   0          2m

# Step 6: 检查 Loki 服务
kubectl get svc -n monitoring -l app=loki

# 预期输出:
# NAME    TYPE        CLUSTER-IP      EXTERNAL-IP   PORT(S)    AGE
# loki    ClusterIP   10.96.200.50    <none>        3100/TCP   3m
```

### 任务 2: 配置 Grafana Loki 数据源 (30min)

```bash
# Step 1: 访问 Grafana
kubectl port-forward -n monitoring svc/prometheus-grafana 3000:80

# 浏览器访问: http://localhost:3000
# 默认用户名: admin
# 默认密码: prom-operator

# Step 2: 在 Grafana UI 中添加 Loki 数据源
# Configuration -> Data Sources -> Add data source -> Loki
# URL: http://loki:3100
# 点击 "Save & Test"
# 预期输出: "Data source connected and labels found."

# Step 3: 或者使用 Grafana API 配置数据源
LOKI_URL="http://loki.monitoring.svc:3100"

cat > loki-datasource.yaml << 'EOF'
apiVersion: 1
datasources:
- name: Loki
  type: loki
  access: proxy
  url: http://loki:3100
  isDefault: false
  editable: true
  jsonData:
    maxLines: 1000
    derivedFields:
    - datasourceUid: prometheus
      matcherRegex: 'traceID=(\w+)'
      name: TraceID
      url: '$${__value.raw}'
EOF

# Step 4: LogQL 查询实践

# 查询 default 命名空间的所有日志
# {namespace="default"}

# 查询特定容器的日志
# {namespace="kube-system", container="kube-apiserver"}

# 过滤包含 error 的日志
# {app="nginx"} |= "error"

# 过滤不包含 debug 的日志
# {app="nginx"} != "debug"

# 正则过滤
# {app="nginx"} |~ "error|warn|fatal"

# 统计过去5分钟的日志条数
# count_over_time({namespace="default"}[5m])

# 统计错误日志的速率
# rate({app="nginx"} |= "error" [5m])

# 聚合查询: 按Pod统计错误数
# sum(count_over_time({app="nginx"} |= "error" [1h])) by (pod)
```

### 任务 3: Alertmanager 路由配置 (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
# Step 1: 查看当前 Alertmanager 配置
kubectl get secret -n monitoring \
  alertmanager-prometheus-kube-prometheus-alertmanager \
  -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d

# Step 2: 创建自定义 Alertmanager 配置
cat > alertmanager-config.yaml << 'EOF'
apiVersion: v1
kind: Secret
metadata:
  name: alertmanager-prometheus-kube-prometheus-alertmanager
  namespace: monitoring
type: Opaque
stringData:
  alertmanager.yaml: |
    global:
      resolve_timeout: 5m
      http_config:
        follow_redirects: true
      smtp_smarthost: 'smtp.company.com:587'
      smtp_from: 'alertmanager@company.com'
      smtp_auth_username: 'alertmanager@company.com'
      smtp_auth_password: 'your-password'

    templates:
    - '/etc/alertmanager/config/*.tmpl'

    route:
      group_by: ['alertname', 'namespace', 'severity']
      group_wait: 30s
      group_interval: 5m
      repeat_interval: 4h
      receiver: 'default-receiver'
      routes:
      - match:
          severity: critical
        receiver: 'critical-receiver'
        group_wait: 10s
        repeat_interval: 1h
        continue: false
      - matchers:
        - severity="warning"
        receiver: 'warning-receiver'
        group_wait: 30s
        repeat_interval: 4h
      - matchers:
        - namespace=~"^(production|prod)$"
        receiver: 'production-receiver'
        group_wait: 15s
        repeat_interval: 2h

    inhibit_rules:
    - source_matchers:
      - severity="critical"
      target_matchers:
      - severity="warning"
      equal: ['alertname', 'namespace']

    receivers:
    - name: 'default-receiver'
      webhook_configs:
      - url: 'http://alertmanager-webhook:5001/'
        send_resolved: true
      dingtalk_configs:
      - webhook_url: 'https://oapi.dingtalk.com/robot/send?access_token=xxx'
        message: |
          {{ range .Alerts }}
          **告警名称**: {{ .Labels.alertname }}
          **严重级别**: {{ .Labels.severity }}
          **命名空间**: {{ .Labels.namespace }}
          **详情**: {{ .Annotations.message }}
          {{ end }}

    - name: 'critical-receiver'
      webhook_configs:
      - url: 'http://alertmanager-webhook:5001/critical'
        send_resolved: true
      dingtalk_configs:
      - webhook_url: 'https://oapi.dingtalk.com/robot/send?access_token=xxx'
        message: |
          @所有人 【严重告警】
          {{ range .Alerts }}
          **告警**: {{ .Labels.alertname }}
          **级别**: {{ .Labels.severity }}
          **详情**: {{ .Annotations.message }}
          **开始时间**: {{ .StartsAt }}
          {{ end }}

    - name: 'warning-receiver'
      webhook_configs:
      - url: 'http://alertmanager-webhook:5001/warning'

    - name: 'production-receiver'
      webhook_configs:
      - url: 'http://alertmanager-webhook:5001/production'
        send_resolved: true
      email_configs:
      - to: 'sre-team@company.com'
        headers:
          Subject: '[生产告警] {{ .GroupLabels.alertname }}'
EOF

# Step 3: 应用配置
kubectl apply -f alertmanager-config.yaml

# Step 4: 验证配置已更新
kubectl get secret -n monitoring \
  alertmanager-prometheus-kube-prometheus-alertmanager \
  -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d | head -20

# Step 5: 重启 Alertmanager 使配置生效
kubectl rollout restart statefulset -n monitoring \
  alertmanager-prometheus-kube-prometheus-alertmanager

# Step 6: 验证 Alertmanager 运行状态
kubectl get pods -n monitoring -l app.kubernetes.io/name=alertmanager
```

### 任务 4: 日志查询实践 (30min)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘

```bash
# Step 1: 生成测试日志
kubectl run log-test --image=busybox --restart=Never -- sh -c 'for i in $(seq 1 1000); do echo "[$(date '+%Y-%m-%d %H:%M:%S')] Log message $i - status: $([ $((i % 10)) -eq 0 ] && echo ERROR || echo INFO)"; sleep 1; done'

# Step 2: 验证 Pod 运行
kubectl get pod log-test

# 预期输出:
# NAME        READY   STATUS    RESTARTS   AGE
# log-test    1/1     Running   0          30s

# Step 3: 在 Grafana Explore 中查询

# 基本查询: 查看log-test Pod的所有日志
# {pod="log-test"}

# 预期输出:
# 2026-05-18 10:30:00 [2026-05-18 10:30:00] Log message 1 - status: INFO
# 2026-05-18 10:30:01 [2026-05-18 10:30:01] Log message 2 - status: INFO
# 2026-05-18 10:30:10 [2026-05-18 10:30:10] Log message 10 - status: ERROR

# 过滤错误日志
# {pod="log-test"} |= "ERROR"

# 统计每分钟日志数
# sum(count_over_time({pod="log-test"}[1m]))

# 统计ERROR日志的速率
# rate({pod="log-test"} |= "ERROR" [5m])

# 提取状态码并统计
# {pod="log-test"} | logfmt | line_format "{{.status}}"

# Step 4: 清理
kubectl delete pod log-test --force  # ⚠️ 跳过优雅终止，可能丢数据
```

---

## 配置参考

### Promtail DaemonSet 配置

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: promtail
  namespace: monitoring
  labels:
    app: promtail
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
        image: grafana/promtail:2.9.4
        args:
        - -config.file=/etc/promtail/promtail.yaml
        - -config.expand-env=true
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
        - name: positions
          mountPath: /run/promtail
        resources:
          requests:
            cpu: 50m
            memory: 64Mi
          limits:
            cpu: 200m
            memory: 128Mi
        securityContext:
          runAsUser: 0
          readOnlyRootFilesystem: true
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
      - name: positions
        hostPath:
          path: /run/promtail
          type: DirectoryOrCreate
      tolerations:
      - key: node-role.kubernetes.io/master
        effect: NoSchedule
      - key: node-role.kubernetes.io/control-plane
        effect: NoSchedule
```

### Promtail 配置文件

```yaml
server:
  http_listen_port: 3101
  grpc_listen_port: 0

positions:
  filename: /run/promtail/positions.yaml

clients:
  - url: http://loki:3100/loki/api/v1/push

scrape_configs:
  - job_name: kubernetes-pods
    kubernetes_sd_configs:
    - role: pod
    pipeline_stages:
    - cri: {}
    - labels:
        stream:
    - matchers:
      - selector="{app=~"nginx.+"}"
      - stages=""
      - - regex=""
      - expression="^(?P<remote_addr>[\w\.]+) - (?P<remote_user>\S+) \[(?P<time>[^\]]+)\] "(?P<method>\w+) (?P<path>\S+) (?P<protocol>\S+)" (?P<status>\d+) (?P<body_bytes_sent>\d+)"
      - - labels=""
      - method=""
      - status=""
      - - metrics=""
      - http_requests_total=""
      - type="Counter"
      - description="Total HTTP requests"
      - config=""
      - action="inc"
    relabel_configs:
    - source_labels:
      - __meta_kubernetes_pod_node_name
      target_label: node
    - source_labels:
      - __meta_kubernetes_namespace
      target_label: namespace
    - source_labels:
      - __meta_kubernetes_pod_name
      target_label: pod
    - source_labels:
      - __meta_kubernetes_pod_container_name
      target_label: container
    - source_labels:
      - __meta_kubernetes_pod_label_app
      target_label: app

```

### Alertmanager 路由参数说明

| 参数 | 说明 | 推荐值 |
|------|------|--------|
| `group_by` | 告警分组依据 | `['alertname', 'namespace']` |
| `group_wait` | 首次发送等待时间 | `30s` (critical: `10s`) |
| `group_interval` | 同组新告警间隔 | `5m` |
| `repeat_interval` | 重复发送间隔 | `4h` (critical: `1h`) |
| `resolve_timeout` | 未收到恢复通知的超时 | `5m` |
| `inhibit_rules` | 告警抑制规则 | critical 抑制同组 warning |

### Loki 存储配置参数

| 参数 | 说明 | 默认值 | 推荐值 |
|------|------|--------|--------|
| `persistence.enabled` | 持久化存储 | false | true |
| `persistence.size` | 存储大小 | 10Gi | 根据日志量调整 |
| `retention_period` | 日志保留期 | 无限制 | `720h` (30天) |
| `max_query_length` | 最大查询时间范围 | 721h | 721h |
| `max_entries_limit_per_query` | 单次查询最大条目 | 5000 | 10000 |
| `chunk_target_size` | 块目标大小 | 1572864 | 1572864 |

---

## 常见问题

### Q1: Loki 和 ELK 相比有什么优势？

**A**: Loki 的核心优势在于资源消耗低：
1. **不建立全文索引**：只索引标签，存储成本比 ELK 低 5-10 倍
2. **与 Grafana 深度集成**：同一界面查看指标和日志，支持从指标跳转到日志
3. **与 Prometheus 标签一致**：使用相同的标签体系，方便关联查询
4. **运维简单**：单个二进制 + 对象存储，不需要维护 Elasticsearch 集群
5. **LogQL 类似 PromQL**：学习曲线平滑

缺点是不支持复杂的全文搜索和聚合分析，适合以运维为主要目的的日志场景。

### Q2: Promtail 采集不到某些 Pod 的日志怎么办？

**A**: 排查步骤：
1. 检查 Promtail 是否在每个节点运行：`kubectl get pods -n monitoring -l app=promtail -o wide`
2. 检查目标 Pod 所在节点是否有 Promtail：对比 Pod 节点和 Promtail 节点
3. 检查 Promtail 日志：`kubectl logs -n monitoring <promtail-pod>`
4. 检查容器运行时是否是 containerd（需要 CRI stage）
5. 检查日志路径是否正确：`/var/log/pods/` 或 `/var/lib/docker/containers/`

### Q3: Alertmanager 配置更新后不生效？

**A**: 确保以下步骤：
1. Secret 已更新：`kubectl get secret -n monitoring ... -o jsonpath='{.data}' | base64 -d`
2. 重启 Alertmanager Pod：`kubectl rollout restart statefulset ...`
3. 等待 Pod 就绪：`kubectl rollout status statefulset ...`
4. 检查 Alertmanager UI：访问 `http://alertmanager:9093` 查看配置
5. 使用 `amtool check-config` 验证配置文件语法

### Q4: LogQL 查询很慢怎么办？

**A**: 优化 LogQL 查询的技巧：
1. **缩小时间范围**：尽量使用短时间范围查询
2. **精确标签选择器**：使用 `{namespace="xxx", app="yyy"}` 而非 `{namespace=~".*"}`
3. **先过滤再聚合**：`rate({...} |= "error" [5m])` 比 `rate({...} [5m])` 快
4. **避免正则**：`|=` 比正则过滤 `|~` 快很多
5. **增加标签基数**：不要在高基数值上添加标签（如 user_id）

### Q5: 如何实现告警静默（不删除配置的情况下临时关闭）？

**A**: 使用 Alertmanager 的 Silence 功能：
```bash
# 通过 amtool 创建静默
amtool silence add \
  --alertmanager.url=http://alertmanager:9093 \
  --author="ops-team" \
  --comment="计划维护窗口" \
  'alertname=PodCrashLooping namespace=staging'

# 查看所有静默
amtool silence query --alertmanager.url=http://alertmanager:9093

# 删除静默
amtool silence expire <silence-id>
```

---

## 要点总结

- **Loki** 是轻量级日志聚合方案，只索引标签不索引全文，存储成本低
- **Promtail** 以 DaemonSet 方式部署，采集每个节点的容器日志
- **LogQL** 是 Loki 的查询语言，支持过滤、提取和聚合操作
- **Alertmanager 路由** 支持基于标签的分级告警，critical 告警需要更快的响应
- **告警抑制**（inhibit_rules）可以避免低优先级告警干扰高优先级告警
- 日志保留策略需要根据 **合规要求** 和 **存储成本** 平衡设置

---

## 延伸阅读

- [Loki 官方文档](https://grafana.com/docs/loki/latest/)
- [LogQL 查询语言参考](https://grafana.com/docs/loki/latest/query/)
- [Alertmanager 配置文档](https://prometheus.io/docs/alerting/latest/configuration/)
- [Kubernetes 日志架构](https://kubernetes.io/docs/concepts/cluster-administration/logging/)
- [文件: `../../domain-06-observability/03-logging-architecture.md`](../../domain-06-observability/03-logging-architecture.md)
- [文件: `../../domain-06-observability/04-distributed-tracing.md`](../../domain-06-observability/04-distributed-tracing.md)

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

```