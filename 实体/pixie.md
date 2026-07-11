---
title: Pixie [entities]
description: '## 概述'
summary: 'Pixie 是一个 Kubernetes 原生的可观测性平台，使用 eBPF 自动采集遥测数据，无需代码变更或手动 instrumentation。它提供对服务通信 (HTTP、gRPC、DNS、MySQL、PostgreSQL、Redis、Kafka)、资源使用和应用性能的即时可见性。Pixie 数据在集群内处理，支持 PxL 查询语言进行分析。'
category: entities
tags:
- k8s
- cncf
- observability
- pixie
- prometheus
- grafana
- istio
- redis
- mysql
- postgresql
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pixie 是什么
- 如何 Pixie
trigger_keywords:
- Pixie
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- kafka-basics
- redis-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pixie

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: C++, Go

## 概述

Pixie 是由 New Relic 开源（现 CNCF Sandbox）的 Kubernetes 原生可观测性平台，使用 eBPF 自动采集遥测数据，无需代码变更或手动 instrumentation。它提供对服务通信（HTTP、gRPC、DNS、MySQL、PostgreSQL、Redis、Kafka）、资源使用和应用性能的即时可见性。Pixie 数据在集群内处理（Edge Processing），支持 PxL 查询语言进行自定义分析。

## 核心特性

- **零 Instrumentation**: 基于 eBPF 自动采集，无需修改应用代码或注入 Sidecar
- **协议自动解析**: HTTP、gRPC、MySQL、PostgreSQL、Redis、DNS、Kafka
- **PxL 查询语言**: Python 风格的查询语言进行数据分析和可视化
- **边缘计算**: 数据在集群内处理，不外传，满足数据驻留合规
- **即时 Service Map**: 安装即可获得服务拓扑图和请求追踪
- **CPU 火焰图**: 自动采集 CPU 性能分析数据

## 架构

Pixie 由 Vizier（集群内数据采集和处理）和 Cloud（管理和可视化）组成。Vizier 以 DaemonSet 部署在每个节点，包含 PEM（Pixie Edge Module——eBPF 采集器）和 Query Broker（PxL 查询执行）。PEM 通过 eBPF 挂载到内核跟踪点（tracepoints、kprobes、uprobes），自动采集系统调用级别的协议数据。数据在节点本地缓冲和处理，通过 PxL 查询时聚合返回。Kelvin（集群级聚合器）处理跨节点数据。Cloud 端提供 Web UI 和管理界面，不存储遥测数据本身。

## Kubernetes 集成

Pixie 通过 DaemonSet 在每个节点部署 PEM。PEM 以特权模式运行以加载 eBPF 程序。通过 Kubernetes API 自动发现 Pod、Service 和命名空间元数据。与 CNI 插件无关——在内核层采集，兼容所有网络方案。Pixie CLI 通过 Pixie Auth 连接集群，支持 PDK（Pixie Development Kit）开发自定义脚本。数据保留在集群内，可通过 Pixie CLI 或 Web UI 查询。

## 生产使用场景

1. **零侵入监控**: 对已有应用无需修改代码即可获得全链路追踪
2. **协议级诊断**: 分析 HTTP 请求延迟、数据库查询性能
3. **安全合规**: 数据不出集群，适合金融/医疗等敏感场景
4. **CPU 分析**: 自动采集 CPU 火焰图，定位性能瓶颈

## 安装

```bash
# 安装 Pixie CLI
brew install pixie
# 部署到集群
px deploy
# 查看数据
px live   # 实时交互式界面
px script run px/service_stats   # 运行预置脚本
# PxL 脚本示例
import px
df = px.DataFrame(table='http_events', start_time='-5m')
df = df.groupby(['service', 'req_path']).agg(latency_p99=('latency', px.percentile(99)))
display(df)
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Pixie** | 零侵入、eBPF、PxL | 数据保留短期、功能受限 |
| Jaeger | 分布式追踪标准 | 需手动 instrumentation |
| Cilium Hubble | eBPF 网络可观测 | 仅网络层 |
| Honeycomb | 强大分析能力 | 商业产品、数据外传 |

## 架构定位

在 CNCF 生态中，Pixie 属于 **Observability** 类别，是 eBPF 驱动的零侵入可观测性代表。它与 Prometheus（指标）、Jaeger（追踪）互补。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[operator-pattern]]
- [[概念/service-mesh-architecture.md|service-mesh-architecture]]
- [[概念/observability-pillars.md|observability-pillars]]

## Related

- [[02-istio-advanced-traffic-management]] — Istio 高级流量管理
- [[vscode-kubernetes-tools]] — VS Code Kubernetes Tools
- [[litmus]] — LitmusChaos
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[grpc]] — gRPC

- pixie
- [[实体/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
