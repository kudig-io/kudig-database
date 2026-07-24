---
title: Fluentd (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- observability
- fluentd
- containerd
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Fluentd 是什么
- 如何 Fluentd
trigger_keywords:
- Fluentd
prerequisites:
- kubectl-basics
- logging-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Fluentd

> **CNCF 状态**: Graduated | **类别**: Observability | **主要语言**: Ruby, C

## 概述

Fluentd 是一个 CNCF 毕业项目，由 Treasure Data（现 Arm Treasure Data）创建，是云原生领域最广泛使用的开源日志采集和处理工具。它提供统一的日志采集层（Unified Logging Layer），能够从多种数据源（文件、HTTP、Syslog、K8s 日志等）采集日志，经过过滤、解析、缓冲后输出到多种目标（Elasticsearch、S3、Kafka、Datadog 等）。Fluentd 拥有超过 800 个插件，覆盖了几乎所有主流数据源和目标系统。

## Key Features（核心能力）

- **统一日志层**：支持 800+ 插件，覆盖几乎所有数据源和输出目标
- **JSON 统一格式**：将所有日志统一为 JSON 格式，便于后续处理
- **高可靠性**：支持基于文件和内存的缓冲机制，防止日志丢失
- **灵活的路由**：通过 Tag 和 Match 实现日志的灵活路由和分发
- **Fluent Bit 集成**：与 Fluent Bit 配合，轻量采集 + 重度处理分层部署
- **高性能**：C 扩展的核心引擎支持高吞吐日志处理

## 架构与工作原理

Fluentd 的事件处理流水线由 Input、Parser、Filter、Buffer、Output 五个阶段组成。Input Plugin 从数据源读取原始日志；Parser 将非结构化日志解析为 JSON；Filter 对日志进行过滤、富化（如添加 K8s 元数据）；Buffer 提供可靠的缓冲机制（文件/内存）；Output Plugin 将处理后的日志发送到目标系统。通过 Event Loop（基于 Cool.io）高效处理并发 I/O。

## K8s 集成

在 Kubernetes 中，Fluentd 通常以 DaemonSet 部署到每个节点，自动采集节点上所有容器的 stdout/stderr 日志（/var/log/containers/）。通过 in_tail 插件读取容器日志文件，通过 kubernetes_metadata_filter 插件富化 Pod/Namespace/Label 元数据。输出通常发送到 Elasticsearch、Loki 或 Kafka。Fluentd ConfigMap 定义采集规则和路由策略。

## 生产用例

- **K8s 集群日志聚合**：统一采集所有容器日志发送到 Elasticsearch/Loki
- **多源日志统一**：将应用日志、Nginx 访问日志、系统日志统一到同一平台
- **日志流处理**：实时过滤敏感信息、解析结构化日志、添加元数据
- **合规日志归档**：将审计日志长期存储到 S3/对象存储满足合规要求

## 安装与配置

### Helm 部署 Fluentd

```bash
# 🟢 添加 Helm 仓库
helm repo add fluent https://fluent.github.io/helm-charts
helm repo update

# 🟢 安装 Fluentd (DaemonSet 模式)
helm install fluentd fluent/fluentd \
  -n logging --create-namespace \
  --set kind=DaemonSet \
  --set resources.requests.memory=256Mi \
  --set resources.limits.memory=512Mi \
  --set persistence.enabled=true \
  --set persistence.size=10Gi

# 🟢 或安装 Fluent Bit（轻量采集层）
helm install fluent-bit fluent/fluent-bit \
  -n logging --create-namespace \
  --set config.outputs="[OUTPUT]\n    Name es\n    Match *\n    Host elasticsearch\n    Port 9200"

# 🟢 验证部署
kubectl get pods -n logging -l app.kubernetes.io/name=fluentd
kubectl logs -n logging -l app.kubernetes.io/name=fluentd --tail=20
```

### Fluentd 配置示例 (fluent.conf)

```conf
# 输入：采集容器日志
<source>
  @type tail
  path /var/log/containers/*.log
  pos_file /var/log/fluentd-containers.log.pos
  tag kubernetes.*
  read_from_head true
  <parse>
    @type cri
  </parse>
</source>

# 过滤：添加 K8s 元数据
<filter kubernetes.**>
  @type kubernetes_metadata
  @id filter_kube_metadata
  kubernetes_url "#{ENV['KUBERNETES_SERVICE_HOST']}"
  cache_size 1000
  watch true
</filter>

# 过滤：解析 JSON 日志
<filter kubernetes.**>
  @type parser
  key_name log
  reserve_data true
  remove_key_name_field true
  <parse>
    @type multi_format
    <pattern>
      format json
    </pattern>
    <pattern>
      format none
    </pattern>
  </parse>
</filter>

# 过滤：移除敏感字段
<filter kubernetes.**>
  @type record_transformer
  remove_keys $.kubernetes.annotations."kubectl.kubernetes.io/last-applied-configuration"
</filter>

# 路由：按命名空间分流
<match kubernetes.var.log.containers.**kube-system**>
  @type relabel
  @label @SYSTEM_LOGS
</match>

<match kubernetes.**>
  @type relabel
  @label @APP_LOGS
</match>

# 输出：应用日志到 Elasticsearch
<label @APP_LOGS>
  <match **>
    @type elasticsearch
    host elasticsearch.logging.svc
    port 9200
    logstash_format true
    logstash_prefix app-logs
    include_tag_key true
    type_name _doc
    <buffer>
      @type file
      path /var/log/fluentd-buffers/app.buffer
      flush_mode interval
      flush_interval 5s
      chunk_limit_size 8M
      total_limit_size 2G
      retry_max_interval 30
      retry_forever false
      retry_max_times 5
      overflow_action block
    </buffer>
  </match>
</label>

# 输出：系统日志到 S3 归档
<label @SYSTEM_LOGS>
  <match **>
    @type s3
    s3_bucket k8s-system-logs
    s3_region us-east-1
    path system-logs/%Y/%m/%d/
    <buffer time>
      @type file
      path /var/log/fluentd-buffers/s3.buffer
      timekey 1h
      timekey_wait 10m
      timekey_use_utc true
    </buffer>
  </match>
</label>
```

### Fluent Bit 轻量采集配置

```yaml
# Fluent Bit ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-config
  namespace: logging
data:
  fluent-bit.conf: |
    [SERVICE]
        Flush         5
        Log_Level     info
        Daemon        off
        Parsers_File  parsers.conf
        HTTP_Server   On
        HTTP_Listen   0.0.0.0
        HTTP_Port     2020

    [INPUT]
        Name              tail
        Tag               kube.*
        Path              /var/log/containers/*.log
        Parser            cri
        DB                /var/log/flb_kube.db
        Mem_Buf_Limit     50MB
        Skip_Long_Lines   On
        Refresh_Interval  10

    [FILTER]
        Name                kubernetes
        Match               kube.*
        Kube_URL            https://kubernetes.default.svc:443
        Kube_CA_File        /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
        Kube_Token_File     /var/run/secrets/kubernetes.io/serviceaccount/token
        Merge_Log           On
        K8S-Logging.Parser  On
        K8S-Logging.Exclude On

    [OUTPUT]
        Name            forward
        Match           *
        Host            fluentd.logging.svc
        Port            24224
```

## 运维操作

```bash
# 🟢 检查 Fluentd 状态
kubectl get pods -n logging -l app.kubernetes.io/name=fluentd
kubectl top pods -n logging

# 🟢 查看日志处理指标
curl -s http://fluentd:24231/api/plugins.json | jq '.plugins[] | select(.type=="output") | {type, buffer_queue_length, buffer_total_queued_size}'

# 🟢 检查缓冲区状态
kubectl exec -n logging fluentd-0 -- ls -la /var/log/fluentd-buffers/
kubectl exec -n logging fluentd-0 -- du -sh /var/log/fluentd-buffers/

# 🟢 测试配置语法
kubectl exec -n logging fluentd-0 -- fluentd --dry-run -c /fluentd/etc/fluent.conf

# 🟡 重新加载配置（无需重启）
kill -HUP $(kubectl exec -n logging fluentd-0 -- cat /var/run/fluentd/fluentd.pid)

# 🟢 查看 Fluent Bit 指标
curl -s http://fluent-bit:2020/api/v1/metrics | jq '.input.records, .output.records'
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 日志延迟 > 5min | 缓冲区积压/输出端慢 | 检查 buffer_queue_length | 增加 flush 并发/扩容输出端 |
| 日志丢失 | 缓冲区溢出/磁盘满 | `du -sh buffers/`; `df -h` | 增大 total_limit_size/清理磁盘 |
| Pod OOMKilled | 内存缓冲过大 | `kubectl describe pod` | 调整 Mem_Buf_Limit/resources |
| 元数据缺失 | K8s API 连接失败 | 检查 ServiceAccount/RBAC | 修复 RBAC 权限 |
| 解析错误 | 日志格式不匹配 | 查看 fluentd 错误日志 | 调整 parser 配置 |

### 排查流程

```
日志采集异常
├── 日志完全缺失？
│   ├── Fluentd Pod 运行？→ kubectl get pods -n logging
│   ├── 日志文件存在？→ ls /var/log/containers/
│   ├── pos_file 位置正确？→ 检查是否跳过了旧日志
│   └── 输出端可达？→ curl elasticsearch:9200/_cluster/health
├── 日志延迟高？
│   ├── 缓冲区积压？→ 检查 buffer metrics
│   ├── 输出端慢？→ 检查 ES/Kafka 负载
│   └── 采集速率过高？→ 检查 input 指标
└── 日志内容异常？
    ├── 元数据缺失 → 检查 kubernetes_metadata_filter
    ├── 解析失败 → 检查 parser 配置
    └── 重复日志 → 检查 pos_file 是否丢失
```

## 生产案例

### 案例1：大规模集群日志采集延迟

- **场景**：200 节点集群，高峰期日志延迟超过 10 分钟
- **排查**：Fluentd buffer_queue_length 持续增长；ES 写入速度跟不上采集速度
- **方案**：采用 Fluent Bit（采集）+ Fluentd（聚合）分层架构；增加 Fluentd 副本数；ES 增加 data 节点
- **效果**：延迟降至 < 10s，采集层 CPU 降低 60%

### 案例2：节点磁盘被日志缓冲区填满

- **场景**：输出端 ES 宕机 2 小时，Fluentd 缓冲区将节点磁盘写满，导致节点 NotReady
- **排查**：`df -h` 显示 /var/log 100%；Fluentd 缓冲区文件占用 50GB
- **方案**：设置 `total_limit_size 2G` 限制缓冲上限；`overflow_action drop_oldest_chunk`；添加磁盘使用率告警
- **效果**：即使输出端故障，缓冲区也不会超过 2GB，保护节点磁盘

## 对比替代方案

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| Fluentd | 插件丰富(800+)、生态成熟 | Ruby 性能较低、内存占用大 | 复杂日志处理/聚合层 |
| Fluent Bit | 轻量(C语言)、低资源 | 插件较少、处理能力有限 | 边缘/采集层 |
| Vector (Rust) | 高性能、单二进制、VRL转换 | 生态较新、插件少 | 高性能场景 |
| Logstash | ELK原生、功能强大 | JVM重、资源占用大 | 纯 Elastic 环境 |
| OTel Collector | 统一遥测数据、CNCF标准 | 日志功能较新 | 统一可观测性 |

## 检查清单

- [ ] Fluentd/Fluent Bit DaemonSet 在所有节点运行
- [ ] 缓冲区配置了大小限制和溢出策略
- [ ] 输出端连接配置了重试和超时
- [ ] K8s 元数据富化已启用
- [ ] 敏感信息过滤已配置
- [ ] 监控指标已接入 Prometheus
- [ ] 磁盘使用率告警已配置
- [ ] 配置变更有 dry-run 验证流程

## Related

- [[06-containerd-observability]] — [[containerd|containerd]]rd 可观测性|containerd 可观测性]]
- [[stacker]] — Stacker
- [[opentelemetry]] — OpenTelemetry
- [[kusionstack]] — KusionStack
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 02-fluentd-enterprise-log-processing
- fluentd
- [[实体/k8s-observability-ecosystem.md|可观测性体系：指标、日志、链路追踪与混沌工程]] — Cross-reference
- [[概念/bp-observability.md|最佳实践：Observability]] — Cross-reference
- [[技能/可观测性/monitoring/最佳实践/k8s-logging-management-guide.md|Kubernetes 日志管理最佳实践]] — Cross-reference
- [[技能/工作负载/deployment/deployment-workload-selection.md|工作负载控制器选型]] — Cross-reference
- [[实体/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/领域索引/observability-index.md|Observability 可观测性知识图谱索引]]


<!-- risk-assessed -->
