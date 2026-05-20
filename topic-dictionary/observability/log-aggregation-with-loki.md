---
title: 日志聚合与 Loki
description: '# 日志聚合与 Loki'
category: dictionary
tags:
- k8s
- glossary
- terminology
- prometheus
- grafana
- ceph
- minio
- kafka
- elasticsearch
- daemonset
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 日志聚合与 Loki 是什么
- 如何 日志聚合与 Loki
trigger_keywords:
- 日志聚合与
- Loki
- dictionary
title_en: Logging
---


# 日志聚合与 Loki

## 概述

在 Kubernetes 环境中，日志分散在数百甚至数万个 Pod 中，**日志聚合（Log Aggregation）** 是运维排障和审计合规的基石。2026 年的主流方案是 **Grafana Loki** —— 一个受 Prometheus 启发的水平可扩展日志聚合系统。与传统方案（如 Elasticsearch）相比，Loki 只索引日志的**标签（Labels）**而不索引日志内容本身，这使其在存储成本和运维复杂度上具有显著优势，特别适合 Kubernetes 的云原生场景。

## 核心概念/原理

### 1. Loki 架构

Loki 由三个核心组件组成：
- **Loki**：日志聚合服务器，负责接收、存储和查询日志
- **Promtail**：日志收集代理，部署为 DaemonSet，从每个节点采集日志并推送到 Loki
- **Grafana**：可视化平台，通过 Loki 数据源查询和展示日志

```
Pod Container Logs
    ↓
Node 上的 Promtail（读取 /var/log/pods/*/*.log）
    ↓ 添加 K8s 标签（namespace、pod、container、node）
Loki Server
    ↓ 按标签索引，日志内容压缩存储在对象存储（S3/GCS）
Grafana Dashboard / LogQL 查询
```

### 2. 标签驱动索引

Loki 与 Elasticsearch 的核心差异在于索引策略：

| 特性 | Elasticsearch | Loki |
|------|---------------|------|
| 索引粒度 | 全文索引日志内容 | 仅索引标签 |
| 存储成本 | 高（原始日志的 2–3 倍） | 低（与原始日志相当或更低） |
| 查询语言 | Lucene / KQL | LogQL（类 PromQL） |
| 资源占用 | 需要大量内存用于索引 | 内存需求低 |
| 运维复杂度 | 高（需要分片、调优 JVM） | 低 |

** trade-off**：Loki 的标签查询非常快，但对日志内容的全文搜索需要通过对象存储扫描，速度较慢。

### 3. LogQL 查询语言

LogQL 是 Loki 的查询语言，结合了 Prometheus 的 PromQL 和日志过滤语法：

```logql
# 基础选择器：选择特定 Pod 的日志
{namespace="production", app="api-gateway"}

# 过滤包含 "error" 的日志
{namespace="production", app="api-gateway"} |= "error"

# 正则过滤
{namespace="production"} |~ "error|ERROR|Exception"

# 聚合：统计每 5 分钟的错误日志数
sum(rate({namespace="production"} |= "error" [5m])) by (app)

# 解析 JSON 日志并提取字段
{app="payment-service"}
  | json
  | status_code="500"
  | line_format "{{.timestamp}} {{.message}}"
```

### 4. 日志采集模式

#### Promtail（官方推荐）
- 以 DaemonSet 运行在每个节点
- 通过 Kubernetes API 自动发现 Pod 并附加标签
- 支持 Pipeline Stage：解析、过滤、转换日志格式

#### Fluent Bit
- CNCF 毕业项目，轻量级日志处理器
- 可作为 Loki 的替代输入源，通过 OUTPUT 插件将日志发送到 Loki
- 优势：生态更丰富，支持更多输出后端（如 Kafka、S3、Elasticsearch）

#### OpenTelemetry Collector
- 统一的可观测性数据收集器
- 通过 `loki` exporter 将日志发送到 Loki
- 适合已经全面采用 OpenTelemetry 的组织

## 关键机制或特性

### Loki 部署模式

| 模式 | 架构 | 适用场景 |
|------|------|----------|
| **Monolithic** | 单进程运行所有组件 | 小规模、POC、开发测试 |
| **Simple Scalable** | 读写分离，可水平扩展 | 中小规模生产 |
| **Distributed（Microservices）** | 每个组件独立部署和扩展 | 大规模、企业级 |

### 对象存储后端

Loki 的日志数据可持久化到低成本对象存储中：
- **AWS S3 / GCS / Azure Blob**：云原生首选
- **MinIO**：本地部署或边缘场景的 S3 兼容方案
- **Ceph Object Gateway**：私有云的统一对象存储

### 日志保留与分片

- **保留期（Retention）**：按时间或按标签配置日志保留策略，旧日志自动清理
- **Compactor**：压缩和去重历史日志块（Chunks），优化存储效率
- **多租户**：通过 `X-Scope-OrgID` 头部实现多团队日志隔离

### 与 Trace 关联

现代可观测性要求日志、指标、链路追踪三者关联：
- 在应用日志中注入 `trace_id` 和 `span_id`
- 在 Grafana 中从 Loki 日志跳转到 Tempo Trace，或从 Prometheus 指标跳转到 Loki 日志
- 形成 **Metrics → Traces → Logs** 的完整排障链路

## 使用场景

1. **生产故障排查**：某 API 返回 500 错误，通过 Grafana 从 Prometheus 指标钻取到 Loki 日志，快速定位到具体的异常堆栈
2. **安全审计**：安全团队通过 Loki 查询特定 Namespace 中所有包含 `sudo` 或 `kubectl` 命令的容器日志
3. **成本敏感型日志平台**：创业公司从 Elasticsearch 迁移到 Loki，日志存储成本降低 70%
4. **多集群日志统一**：使用 Grafana Cloud 或自托管 Loki 聚合 10 个 Kubernetes 集群的日志到单一查询界面
5. **实时告警**：通过 Loki Ruler 配置 LogQL 告警规则，当错误日志速率超过阈值时通过 Alertmanager 通知值班人员

## 最佳实践/注意事项

- **标签设计是关键**：标签数量不宜过多（建议 < 10 个活跃标签组合），否则索引开销会显著增加
- **避免高基数标签**：不要将 `user_id`、`trace_id` 等极高基数字段作为 Loki 标签，而应留在日志内容中通过 `| json` 过滤
- **结构化日志优先**：应用应输出 JSON 格式日志，便于 LogQL 的 `json` 解析器提取字段
- **日志级别分离**：通过 Pipeline Stage 将 DEBUG 日志过滤掉不发送到 Loki，减少噪音和成本
- **设置合理的保留期**：开发环境保留 7 天，生产环境根据合规要求保留 30–90 天，金融/医疗可能需要数年
- **Promtail 资源限制**：在日志量大的节点上，Promtail 可能消耗较多 CPU，需设置适当的 Resource Limits
- **Loki 查询性能**：对于跨越数天的全文搜索，建议使用 Grafana 的"查询拆分"功能或缩短时间范围
- **备份与容灾**：Loki 的数据存储在对象存储中，对象存储本身应具备跨区域复制能力
- **与 OpenTelemetry 对齐**：新项目建议直接通过 OpenTelemetry Collector 采集日志，未来统一指标、日志、追踪的信号管理

## 参考链接

- [Grafana Loki Documentation](https://grafana.com/docs/loki/latest/)
- [Promtail Configuration](https://grafana.com/docs/loki/latest/send-data/promtail/)
- [Fluent Bit Loki Output](https://docs.fluentbit.io/manual/pipeline/outputs/loki)
- [LogQL Query Language](https://grafana.com/docs/loki/latest/query/)
- [OpenTelemetry Logs](https://opentelemetry.io/docs/concepts/signals/logs/)
