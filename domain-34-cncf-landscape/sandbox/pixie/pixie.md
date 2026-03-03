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
