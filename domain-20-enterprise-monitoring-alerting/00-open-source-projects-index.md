---
title: Domain-20 企业监控与告警 — 开源项目索引
description: '# Domain-20 企业监控与告警 — 开源项目索引'
category: enterprise-monitoring-alerting
tags:
- k8s
- monitoring
- alerting
- prometheus
- grafana
- flux
- elasticsearch
- hpa
- statefulset
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 监控工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Domain-20 企业监控与告警 — 开源项目索引 是什么
- 如何 Domain-20 企业监控与告警 — 开源项目索引
- Kubernetes 20 enterprise monitoring alerting 最佳实践
trigger_keywords:
- Domain-20
- 企业监控与告警
- 开源项目索引
- enterprise
- monitoring
- alerting
cross_refs:
- type: cheatsheet
  path: ../topic-cheat-sheet/promql.md
  label: '速查卡: promql'
---

# Domain-20 企业监控与告警 — 开源项目索引

> **最后更新**: 2026-04-24  
> **适用版本**: Prometheus v3.x / Grafana v11.x / K8s v1.29-v1.33

---

## 📋 目录

- [一、核心项目总览](#一核心项目总览)
- [二、Prometheus 生态详解](#二prometheus-生态详解)
- [三、Grafana 生态详解](#三grafana-生态详解)
- [四、企业级扩展项目](#四企业级扩展项目)
- [五、版本兼容矩阵](#五版本兼容矩阵)
- [六、快速选型指南](#六快速选型指南)

---

## 一、核心项目总览

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars |  License |
|:---|:---|:---|:---|:---|:---|
| **Prometheus** | 时序监控与告警引擎 | Graduated | v3.3.0 | 56k+ | Apache-2.0 |
| **Grafana** | 可视化仪表盘平台 | 非 CNCF | v11.6.0 | 67k+ | AGPL-3.0 |
| **Alertmanager** | 告警路由与静默 | Prometheus | v0.28.0 | 6k+ | Apache-2.0 |
| **kube-state-metrics** | K8s 资源状态指标 | K8s SIG | v2.15.0 | 5.5k+ | Apache-2.0 |
| **node_exporter** | 主机级指标导出 | Prometheus | v1.9.0 | 11k+ | Apache-2.0 |
| **Thanos** | Prometheus 高可用联邦 | 非 CNCF | v0.38.0 | 13k+ | Apache-2.0 |
| **Cortex** | 多租户 Prometheus 存储 | Incubating | v1.18.0 | 5.5k+ | Apache-2.0 |
| **Mimir** | Grafana 企业级指标后端 | 非 CNCF | v3.0.0 | 4k+ | AGPL-3.0 |
| **VictoriaMetrics** | 高性能时序数据库 | 非 CNCF | v1.115.0 | 13k+ | Apache-2.0 |
| **OpenTelemetry** | 标准化遥测数据采集 | Incubating | v1.28.0 | 25k+ | Apache-2.0 |
| **cAdvisor** | 容器资源使用分析 | K8s SIG | v0.51.0 | 16k+ | Apache-2.0 |
| **Kiali** | 服务网格可观测性 | 非 CNCF | v2.7.0 | 5k+ | Apache-2.0 |
| **Datadog Agent** | 商业监控代理 | 商业 | v7.64.0 | - | Apache-2.0 |
| **Grafana Tempo** | 分布式追踪后端 | Grafana | v2.9.0 | 4k+ | AGPL-3.0 |
| **Grafana Pyroscope** | 持续性能分析 | Grafana | v1.13.0 | 9k+ | AGPL-3.0 |
| **Grafana OnCall** | 告警管理与值班 | Grafana | v1.15.0 | 5k+ | Apache-2.0 |
| **Netdata** | 实时系统监控 | Netdata | v2.4.0 | 73k+ | GPL-3.0 |
| **Sentry** | 错误追踪与监控 | Functional Software | v25.0.0 | 39k+ | BSL-1.1 |
| **SigNoz** | 开源 APM (Datadog 替代) | SigNoz | v0.76.0 | 21k+ | MIT |
| **Uptrace** | APM 与分布式追踪 | Uptrace | v1.7.0 | 3k+ | BSL-1.1 |
| **Zipkin** | 分布式追踪 | Apache | v3.5.0 | 17k+ | Apache-2.0 |

---

## 二、Prometheus 生态详解

### 2.1 Prometheus (Graduated)

```yaml
# 核心特性
- 多维数据模型 (time series with labels)
- PromQL 查询语言
- 不依赖分布式存储
- HTTP pull 模式采集
- 支持 push gateway
- 服务发现与动态配置
- 多种可视化与导出方案
```

**版本里程碑**
- **v3.0** (2024.11): 全新 UI (PromLens-style tree view)、Remote Write 2.0、Native Histograms GA、OTLP 接收增强
- **v3.3** (2026.03): 最新稳定版，性能持续优化

**GitHub**: https://github.com/prometheus/prometheus
**文档**: https://prometheus.io/docs/

### 2.2 Alertmanager

- 告警分组、抑制、静默
- 多路由配置 (PagerDuty, Slack, Webhook, 钉钉等)
- 高可用模式 (Gossip 集群)

**GitHub**: https://github.com/prometheus/alertmanager

### 2.3 kube-state-metrics

暴露 Kubernetes API 对象状态为 Prometheus 指标：
- Deployment/StatefulSet/DaemonSet 副本状态
- Pod 状态、节点资源压力
- PVC 绑定状态
- HPA 当前/目标指标

**GitHub**: https://github.com/kubernetes/kube-state-metrics

### 2.4 node_exporter

主机级指标导出器：
- CPU、内存、磁盘、网络、文件系统
- systemd、NFS、ZFS 等扩展采集器
- 文本文件采集器 (自定义指标)

**GitHub**: https://github.com/prometheus/node_exporter

---

## 三、Grafana 生态详解

### 3.1 Grafana OSS

```yaml
# 核心特性
- 多数据源支持 (Prometheus, Loki, Tempo, Mimir, Elasticsearch, InfluxDB, etc.)
- 丰富的可视化面板库
- 告警规则与通知通道
- 插件生态系统
- Grafana Dashboards 社区共享
```

**版本里程碑**
- **v11.x**: 改进的 Explore 界面、AI 辅助查询、增强的告警管理
- **Grafana Agent EOL** (2025.11): 官方停止维护，迁移至 **Grafana Alloy**

**GitHub**: https://github.com/grafana/grafana

### 3.2 Grafana Mimir 3.0

- **发布时间**: 2025.11 (KubeCon NA)
- **核心改进**: 读写路径解耦架构、可靠性提升、成本优化
- **兼容性**: 完全兼容 Prometheus 查询 API

**文档**: https://grafana.com/docs/mimir/latest/

### 3.3 Grafana Alloy (替代 Agent)

- 统一的可观测性采集器 (指标/日志/追踪/性能分析)
- 基于 OpenTelemetry Collector 构建
- 支持静态模式和 Flow 模式

**迁移指南**: https://grafana.com/docs/alloy/latest/set-up/migrate/

---

## 四、企业级扩展项目

### 4.1 Thanos

| 维度 | 说明 |
|:---|:---|
| 场景 | 多集群 Prometheus 联邦、长期存储 |
| 核心组件 | Sidecar, Query, Store, Compactor, Ruler, Receive |
| 存储后端 | S3, GCS, Azure Blob, Swift |
| 优势 | 全局查询视图、降采样、规则评估 |

**GitHub**: https://github.com/thanos-io/thanos

### 4.2 VictoriaMetrics

| 维度 | 说明 |
|:---|:---|
| 场景 | 高写入吞吐量、低成本长期存储 |
| 模式 | Single-node / Cluster (vminsert/vmselect/vmstorage) |
| 特色 | 更好的压缩率、更快的查询、兼容 PromQL |
| 企业版 | 支持多租户、RBAC、告警 |

**GitHub**: https://github.com/VictoriaMetrics/VictoriaMetrics

### 4.3 OpenTelemetry

- **CNCF 状态**: Incubating (2025 贡献者增长 35%)
- **采用率**: 49% 生产环境，26% 评估中
- **核心组件**: Collector, SDK (Java/Node.js/Python/Go/.NET)
- **与 Prometheus 关系**: OTLP 指标可直接写入 Prometheus v3+

**GitHub**: https://github.com/open-telemetry

---

## 五、版本兼容矩阵

| 组件 | K8s v1.29 | v1.30 | v1.31 | v1.32 | v1.33 |
|:---|:---|:---|:---|:---|:---|
| Prometheus v3.3 | ✅ | ✅ | ✅ | ✅ | ✅ |
| Grafana v11.6 | ✅ | ✅ | ✅ | ✅ | ✅ |
| kube-state-metrics v2.15 | ✅ | ✅ | ✅ | ✅ | ⚠️ 待验证 |
| node_exporter v1.9 | ✅ | ✅ | ✅ | ✅ | ✅ |
| Thanos v0.38 | ✅ | ✅ | ✅ | ✅ | ✅ |
| Mimir v3.0 | ✅ | ✅ | ✅ | ✅ | ✅ |
| OTEL Collector v0.121 | ✅ | ✅ | ✅ | ✅ | ✅ |

> ⚠️ 注意: kube-state-metrics 通常滞后 K8s 一个小版本发布，建议查看 Release Notes 确认 API 兼容性。

---

## 六、快速选型指南

```
┌─────────────────────────────────────────────────────────────┐
│                    监控告警技术选型决策树                      │
└─────────────────────────────────────────────────────────────┘

1. 集群规模 < 100 节点?
   └─ Yes ──► 单 Prometheus + Grafana (足够)
   └─ No  ──► 继续...

2. 需要长期存储 (> 15 天)?
   └─ Yes ──► Thanos / Mimir / VictoriaMetrics
   └─ No  ──► 本地 SSD + Prometheus

3. 多集群统一视图?
   └─ Yes ──► Thanos Query / Mimir / Cortex
   └─ No  ──► 单实例联邦

4. 成本敏感且写入量大?
   └─ Yes ──► VictoriaMetrics
   └─ No  ──► Thanos / Mimir

5. 已使用 Grafana Cloud?
   └─ Yes ──► Mimir (原生集成)
   └─ No  ──► 自托管 Thanos 或 VictoriaMetrics

6. 需要统一指标+日志+追踪?
   └─ Yes ──► OpenTelemetry + Grafana Stack
   └─ No  ──► Prometheus + Fluentd/Loki 分别部署
```

---

## 参考链接

- [Prometheus 官方文档](https://prometheus.io/docs/)
- [Grafana 官方文档](https://grafana.com/docs/)
- [OpenTelemetry 官方文档](https://opentelemetry.io/docs/)
- [Thanos 设计文档](https://thanos.io/tip/thanos/design.md/)
- [CNCF 可观测性白皮书](https://github.com/cncf/tag-observability/blob/main/whitepaper.md)
