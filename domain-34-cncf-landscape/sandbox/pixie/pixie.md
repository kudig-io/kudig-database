---
title: Pixie
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- redis
- mysql
- postgresql
- kafka
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Pixie 是什么
- 如何 Pixie
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Pixie
- cncf
- landscape
---


# Pixie

> **成熟度**: Sandbox | **加入时间**: 2021-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://px.dev |
| **GitHub** | https://github.com/pixie-io/pixie |
| **许可证** | Apache-2.0 |
| **开发语言** | C++, Go |
| **CNCF 分类** | Observability |
| **维护组织** | New Relic |

---

## 项目概述

Pixie 是一个 Kubernetes 原生的可观测性平台，使用 eBPF 自动采集遥测数据，无需代码变更或手动 instrumentation。它提供对服务通信 (HTTP、gRPC、DNS、MySQL、PostgreSQL、Redis、Kafka)、资源使用和应用性能的即时可见性。Pixie 数据在集群内处理，支持 PxL 查询语言进行分析。

---

## 核心特性

- **零 instrumentation**: 基于 eBPF 自动采集，无需代码变更
- **协议解析**: 自动解析 HTTP、gRPC、MySQL、PostgreSQL、Redis、DNS、Kafka
- **PxL 查询语言**: Python 风格的数据查询和分析
- **边缘计算**: 数据在集群内处理，不外传
- **即时可见性**: 安装即可获得 Service Map、请求追踪
- **Flamegraph**: CPU 性能分析火焰图
- **实时视图**: 毫秒级实时数据更新

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      Pixie Architecture                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    User Interface                         │   │
│  │  ┌─────────────────┐  ┌────────────────────────────────┐ │   │
│  │  │   Pixie UI      │  │   Pixie CLI (px)               │ │   │
│  │  │  (Web Console)  │  │                                │ │   │
│  │  └────────┬────────┘  └──────────────┬─────────────────┘ │   │
│  └───────────┼──────────────────────────┼──────────────────┘   │
│              │                          │                       │
│  ┌───────────▼──────────────────────────▼──────────────────┐   │
│  │                   Pixie Cloud (Optional)                  │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │  Authentication │ Cluster Registry │ Query Proxy    │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │               Kubernetes Cluster                          │   │
│  │                                                           │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │              Vizier (Control Plane)                  │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │   Query     │  │  Metadata   │  │   Cloud    │  │ │   │
│  │  │  │  Broker     │  │  Service    │  │  Connector │  │ │   │
│  │  │  └──────┬──────┘  └─────────────┘  └────────────┘  │ │   │
│  │  │         │                                           │ │   │
│  │  │  ┌──────▼──────────────────────────────────────┐   │ │   │
│  │  │  │         PEM (Pixie Edge Module)             │   │ │   │
│  │  │  │              DaemonSet                       │   │ │   │
│  │  │  │  ┌─────────────────────────────────────┐   │   │ │   │
│  │  │  │  │          eBPF Probes                 │   │   │ │   │
│  │  │  │  │  ┌──────┐ ┌──────┐ ┌──────────────┐ │   │   │ │   │
│  │  │  │  │  │ HTTP │ │ DNS  │ │  MySQL/PG    │ │   │   │ │   │
│  │  │  │  │  │Tracer│ │Tracer│ │  Tracer      │ │   │   │ │   │
│  │  │  │  │  └──────┘ └──────┘ └──────────────┘ │   │   │ │   │
│  │  │  │  │  ┌──────┐ ┌──────┐ ┌──────────────┐ │   │   │ │   │
│  │  │  │  │  │ gRPC │ │Redis │ │  Kafka       │ │   │   │ │   │
│  │  │  │  │  │Tracer│ │Tracer│ │  Tracer      │ │   │   │ │   │
│  │  │  │  │  └──────┘ └──────┘ └──────────────┘ │   │   │ │   │
│  │  │  │  │  ┌──────────────────────────────────┐│   │   │ │   │
│  │  │  │  │  │  CPU / Memory / Network Profiler ││   │   │ │   │
│  │  │  │  │  └──────────────────────────────────┘│   │   │ │   │
│  │  │  │  └─────────────────────────────────────┘   │   │ │   │
│  │  │  │                                             │   │ │   │
│  │  │  │  ┌─────────────────────────────────────┐   │   │ │   │
│  │  │  │  │       In-Memory Data Store          │   │   │ │   │
│  │  │  │  │  (Short-term data retention)        │   │   │ │   │
│  │  │  │  └─────────────────────────────────────┘   │   │ │   │
│  │  │  └─────────────────────────────────────────────┘   │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **Vizier** | 集群内控制平面 |
| **PEM** | 边缘模块，运行在每个节点，采集 eBPF 数据 |
| **Query Broker** | 分布式查询引擎 |
| **Pixie Cloud** | 可选的云端管理和认证 |

---

## 快速开始

### 安装 CLI

```bash
# macOS/Linux
bash -c "$(curl -fsSL https://withpixie.ai/install.sh)"

# 或使用 Homebrew
brew install pixie
```

### 部署 Pixie

```bash
# 使用 CLI 部署 (开源版)
px deploy --dev_cloud_namespace plc

# Helm 部署
helm repo add pixie-operator https://pixie-operator-charts.storage.googleapis.com
helm install pixie pixie-operator/pixie-operator-chart \
  --namespace pl \
  --create-namespace \
  --set deployOLM=true \
  --set deployPixieVizier=true

# 验证
px get viziers
kubectl get pods -n pl
```

---

## PxL 查询语言

### 基本查询

```python
# 查看 HTTP 请求
import px

df = px.DataFrame('http_events', start_time='-5m')
df = df[['time_', 'pod', 'req_path', 'resp_status', 'latency']]
df = df[df.resp_status >= 400]  # 过滤错误请求
px.display(df, 'HTTP Errors')
```

### Service 请求统计

```python
import px

df = px.DataFrame('http_events', start_time='-5m')
df.service = df.ctx['service']
df = df.groupby('service').agg(
    throughput=('latency', px.count),
    avg_latency=('latency', px.mean),
    p99_latency=('latency', px.quantiles, 0.99),
    error_rate=('resp_status', lambda x: px.count(x >= 400) / px.count(x))
)
px.display(df, 'Service Stats')
```

### DNS 查询分析

```python
import px

df = px.DataFrame('dns_events', start_time='-5m')
df = df[['time_', 'pod', 'dns_query', 'dns_resp_latency']]
df = df[df.dns_resp_latency > 100000000]  # > 100ms
px.display(df, 'Slow DNS')
```

### MySQL 查询分析

```python
import px

df = px.DataFrame('mysql_events', start_time='-5m')
df = df[['time_', 'pod', 'req_body', 'resp_latency_ns']]
df.latency_ms = df.resp_latency_ns / 1e6
df = df[df.latency_ms > 100]  # 慢查询 > 100ms
px.display(df, 'Slow MySQL Queries')
```

---

## CLI 使用

```bash
# 运行预定义脚本
px run px/cluster           # 集群概览
px run px/namespace          # 命名空间概览
px run px/pod                # Pod 详情
px run px/service            # Service 统计
px run px/http_data          # HTTP 请求数据
px run px/dns_data           # DNS 查询数据
px run px/mysql_data         # MySQL 查询数据

# 带参数运行
px run px/pod -- --pod=default/nginx-xxx

# 实时模式
px live px/cluster

# 运行自定义脚本
px run -f my_script.pxl

# 导出数据
px run px/http_data -o json > http_data.json
```

---

## 预置仪表板

| 视图 | 说明 |
|:---|:---|
| **Cluster** | 集群总览、资源使用、服务拓扑 |
| **Namespace** | 命名空间级别的服务统计 |
| **Service Map** | 服务间依赖和通信可视化 |
| **Pod** | Pod 级 CPU、内存、网络详情 |
| **HTTP** | HTTP 请求分析、延迟分布 |
| **Database** | MySQL/PostgreSQL 查询分析 |
| **DNS** | DNS 查询和延迟分析 |
| **Flamegraph** | CPU 性能火焰图 |

---

## 数据导出

### 导出到 OpenTelemetry

```yaml
apiVersion: px.dev/v1alpha1
kind: OTelExportConfig
metadata:
  name: otel-export
spec:
  endpoint: otel-collector.observability:4317
  insecure: true
  traces:
    enabled: true
  metrics:
    enabled: true
```

---

## 最佳实践

1. **内核版本**: 确保 Linux 内核 >= 4.14 (推荐 5.3+)
2. **资源预留**: PEM 约需 2GB 内存/节点
3. **数据保留**: 默认短期保留，重要数据导出外部存储
4. **PxL 复用**: 编写通用 PxL 脚本供团队复用
5. **安全**: 数据不出集群，适合合规环境
6. **TLS**: Pixie 自动追踪 TLS 加密通信

---

## 参考资源

- [官方文档](https://docs.px.dev)
- [GitHub Repo](https://github.com/pixie-io/pixie)
- [PxL 语言参考](https://docs.px.dev/reference/pxl/)
- [预置脚本](https://docs.px.dev/tutorials/pxl-scripts/)
- [API 参考](https://docs.px.dev/reference/api/)

---

**维护者**: Kudig Team | **许可证**: MIT

## 生产实战与调优

### 典型生产场景

1. **无侵入式全栈可观测** — Pixie 通过 eBPF 自动捕获 HTTP/gRPC/MySQL/PostgreSQL/Kafka 等协议流量，无需手动埋点即可获得服务间的请求拓扑和延迟分布。
2. **K8s 网络流量分析** — 实时监控 Pod 间通信，快速定位网络策略配置错误、DNS 解析失败、连接超时等问题。
3. **安全审计与异常检测** — 通过 `openssl` eBPF hook 捕获加密流量的元数据（不捕获明文），检测异常的 API 调用模式和未授权访问。
4. **分布式追踪自动采集** — 自动从 HTTP 头中提取 trace context（支持 B3/W3C TraceContext），无需修改业务代码即可构建调用链。
5. **生产环境 Debug** — 使用 PxL 脚本在线查询特定时间窗口的请求详情，替代传统的 "加日志重发布" 模式。

### 配置调优参数

```yaml
# pixie-cloud Helm values 关键配置
# vizier (集群内采集组件)
vizier:
  pem:
    resources:
      requests:
        cpu: 200m
        memory: 256Mi
      limits:
        cpu: "1"
        memory: 1Gi
    flags:
      # eBPF 表大小限制，影响可追踪的并发连接数
      tables_store: "percpu_cache"
      # 数据保留时间（内存中）
      data_access_ttl: "24h"
  # Cloud 接收端
  cloud:
    resources:
      requests:
        cpu: 500m
        memory: 1Gi
      limits:
        cpu: "2"
        memory: 4Gi

# 关键环境变量
# PL_TABLE_STORE_DATA_TTL: 数据保留时间，默认 24h
# PL_MAX_HTTP_EVENTS: 最大 HTTP 事件缓存数，默认 1000000
```

关键调优点：
- PEM (Pixie Edge Module) 资源限制：每节点一个 PEM，CPU/Memory 需根据节点流量规模调整
- `data_access_ttl`：内存中的数据保留时间，降低可减少内存占用但缩短可查询时间窗口
- 收集策略：可通过 `PixieConfig` 选择性关闭某些协议的采集以降低开销

### 性能基准数据（参考值）

| 节点规模 | 业务 Pod 数 | PEM CPU 开销 | PEM 内存开销 | 数据采集延迟 |
|----------|------------|-------------|-------------|-------------|
| 100 节点 | 2000 | 0.2-0.5 core | 300-500 Mi | < 1s |
| 500 节点 | 10000 | 0.3-0.8 core | 500-800 Mi | < 2s |
| 1000 节点 | 30000 | 0.5-1.0 core | 800Mi-1.2Gi | < 3s |

> 注：PEM 开销与节点上的活跃连接数和协议类型相关。数据库密集型节点开销更高。

### 常见坑和注意事项

1. **内核版本要求** — Pixie 的 eBPF 程序需要 Linux Kernel >= 4.14，推荐 >= 5.4 以获得最佳协议解析支持。部分老版本内核的 eBPF verifier 限制较多。
2. **内存 OOM 风险** — PEM 默认内存限制 1Gi，在高流量节点上可能 OOM。建议根据 `pl_stats` 指标监控 PEM 内存使用，必要时提升 limit 或关闭部分协议采集。
3. **ARM64 支持** — Pixie 从 v0.12+ 开始支持 ARM64，但部分 eBPF 程序在 ARM 上的兼容性仍需验证，建议先在测试环境确认。
4. **数据不出集群** — Pixie 默认数据存储在集群内 PEM 的内存中，不上传到外部（除非部署了 Pixie Cloud）。这对数据合规有优势，但受限于内存容量，历史数据查询时间窗口较短。
5. **加密流量限制** — Pixie 只能解析非加密的 L7 流量（HTTP/MySQL 等）。对于 TLS 流量，需要配合 service mesh 的 sidecar（如 Istio 的 mTLS 终止）或使用 Pixie 的 OpenSSL eBPF hook（仅获取元数据，不获取 payload）。
6. **与 Prometheus/Viz 的关系** — Pixie 是 CNCF sandbox 项目，已被 New Relic 收购后开源。长期社区活跃度需关注。
