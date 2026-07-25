---
title: 12 - API 网关可观测性：指标、日志与链路追踪
description: '# 12 - API 网关可观测性：指标、日志与链路追踪'
summary: '+ Alertmanager         Elasticsearch        Tempo'
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- envoy
- apisix
- higress
- prometheus
- grafana
- jaeger
- elasticsearch
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 架构师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- API 网关可观测性：指标、日志与链路追踪 是什么
- 如何 API 网关可观测性：指标、日志与链路追踪
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- API
- 网关可观测性：指标
- 日志与链路追踪
- cloud
- native
- api
- gateway
prerequisites:
- kubectl-basics
- networking-basics
- prometheus-basics
- monitoring-basics
- logging-basics
- tracing-basics
- observability-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 12 - API 网关可观测性：指标、日志与链路追踪

> **文档版本**: v1.0 | **适用版本**: [[Kubernetes|Kubernetes]] 1.25+ | **更新日期**: 2026-03-04 | **关键词**: [[Prometheus|Prometheus]], [[OpenTelemetry|OpenTelemetry]], Grafana, 访问日志, 链路追踪, 黄金信号, Loki, [[Jaeger|Jaeger]], Zipkin

<!-- chunk: 目录 -->## 目录

1. [网关可观测性架构](#1-网关可观测性架构)
2. [黄金信号](#2-黄金信号)
3. [Prometheus 指标](#3-prometheus-指标)
4. [结构化访问日志](#4-结构化访问日志)
5. [分布式链路追踪](#5-分布式链路追踪)
6. [Grafana 仪表盘设计](#6-grafana-仪表盘设计)
7. [告警规则](#7-告警规则)
8. [各产品可观测性对比表](#8-各产品可观测性对比表)

---

<!-- chunk: 1. 网关可观测性架构 -->## 1. 网关可观测性架构

## 1.1 可观测性三支柱

```
                      API 网关可观测性全景
                      
  外部请求                  API 网关                  内部系统
     │                  ┌─────────────┐                 │
     │──────────────────►             ├────────────────►│
     │                  │   数据平面   │                 │
     │◄─────────────────┤             ◄────────────────┤
                        └──────┬──────┘
                               │ 产生可观测性数据
                               │
              ┌────────────────┼────────────────┐
              │                │                │
              ▼                ▼                ▼
    ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
    │   指标        │  │   日志        │  │   追踪        │
    │  (Metrics)   │  │  (Logs)      │  │  (Traces)    │
    │              │  │              │  │              │
    │  What 发生了  │  │  Why 为什么   │  │  Where 在哪里 │
    │  请求量/延迟  │  │  详细上下文   │  │  调用链路径   │
    │  错误率/饱和  │  │  错误详情    │  │  跨服务耗时   │
    └──────┬───────┘  └──────┬───────┘  └──────┬───────┘
           │                 │                 │
           ▼                 ▼                 ▼
    ┌──────────────┐  ┌──────────────┐  ┌──────────────┐
    │  Prometheus  │  │  Loki /      │  │  Jaeger /    │
    │  + Grafana   │  │  Elasticsearch│  │  Zipkin /    │
    │              │  │  + Kibana    │  │  Tempo       │
    └──────────────┘  └──────────────┘  └──────────────┘
           │                 │                 │
           └─────────────────┴─────────────────┘
                             │
                    ┌────────▼────────┐
                    │  Grafana 统一    │
                    │  可观测性平台    │
                    └─────────────────┘
```

## 1.2 数据流架构

```
API 网关 Pod
┌──────────────────────────────────────────────────────────┐
│                                                          │
│  ┌─────────────┐   指标（/metrics）   ┌────────────────┐  │
│  │             │ ──────────────────► │  Prometheus    │  │
│  │  数据平面    │                     │  Scrape        │  │
│  │  (Envoy /   │   访问日志（stdout） ┌────────────────┐  │
│  │   Nginx /   │ ──────────────────► │  Fluentd /     │  │
│  │   APISIX)   │                     │  Vector        │  │
│  │             │   追踪（gRPC/HTTP） ┌────────────────┐  │
│  │             │ ──────────────────► │  OTel Collector│  │
│  └─────────────┘                     └────────────────┘  │
│                                                          │
└──────────────────────────────────────────────────────────┘
         │                    │                    │
         ▼                    ▼                    ▼
    Prometheus             Loki /              Jaeger /
    + Alertmanager         Elasticsearch        Tempo
         │                    │                    │
         └────────────────────┴────────────────────┘
                              │
                     Grafana 统一查询
```

---

<!-- chunk: 2. 黄金信号 -->## 2. 黄金信号

## 2.1 四大黄金信号（网关视角）

Google SRE 定义的四大黄金信号在 API 网关场景的具体含义：

```
┌───────────────────────────────────────────────────────────────────┐
│                     API 网关四大黄金信号                            │
├────────────────┬──────────────────────────────────────────────────┤
│  延迟          │  请求从进入网关到返回响应的耗时                     │
│  (Latency)     │                                                  │
│                │  关键分位数：P50 / P95 / P99 / P999              │
│                │  网关侧延迟 vs 上游服务延迟（分离分析）            │
│                │  健康目标：P99 < 100ms（视业务而定）               │
├────────────────┼──────────────────────────────────────────────────┤
│  流量          │  单位时间内通过网关的请求数量                      │
│  (Traffic)     │                                                  │
│                │  维度：总 RPS、按路由 RPS、按状态码 RPS           │
│                │  用途：容量规划、峰值识别、异常突增检测            │
├────────────────┼──────────────────────────────────────────────────┤
│  错误率        │  失败请求占总请求的比例                           │
│  (Error Rate)  │                                                  │
│                │  4xx：客户端错误（鉴权失败、参数错误）            │
│                │  5xx：服务端错误（上游不可用、超时）              │
│                │  健康目标：5xx 错误率 < 0.1%                     │
├────────────────┼──────────────────────────────────────────────────┤
│  饱和度        │  网关资源利用率，接近上限时服务质量下降            │
│  (Saturation)  │                                                  │
│                │  CPU 利用率 / 内存使用率                         │
│                │  Worker 连接数 / 活跃请求数                      │
│                │  健康目标：CPU < 70%，连接数 < 80% 上限          │
└────────────────┴──────────────────────────────────────────────────┘
```

## 2.2 延迟细化分析（P99/P999）

```
请求时间线分解

  客户端           API 网关                    上游服务
     │              │                              │
  t0 │──── 建连 ────► t1                           │
     │              │── 读取请求 ──── t2            │
     │              │── 插件处理 ──── t3            │
     │              │── 转发上游 ──────────────────►│ t4
     │              │                              │── 业务处理 ──── t5
     │              │◄──────────────── 上游响应 ────│ t6
     │              │── 响应插件 ──── t7            │
     │◄─ 响应 ───────│ t8                           │

网关侧延迟 = t8 - t0
  ├── 网络建连延迟 = t1 - t0
  ├── 请求处理延迟 = t3 - t1（插件耗时主要来源）
  ├── 上游连接延迟 = t4 - t3
  ├── 上游响应延迟 = t6 - t4（大头，代表上游性能）
  └── 响应处理延迟 = t8 - t6（响应插件耗时）
```

---

<!-- chunk: 3. Prometheus 指标 -->## 3. Prometheus 指标

## 3.1 Higress 核心指标

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Higress 指标端点
# kubectl port-forward svc/higress-gateway 15020:15020 -n higress-system
# curl http://localhost:15020/stats/prometheus

# ============ 请求流量指标 ============

# 请求总数（Counter）
envoy_cluster_upstream_rq_total{
  envoy_cluster_name="outbound|8080||order-svc.default.svc.cluster.local"
}

# HTTP 状态码分布（Counter）
envoy_http_downstream_rq_xx{
  envoy_http_conn_manager_prefix="ingress_http",
  envoy_response_code_class="2"   # 2=2xx, 4=4xx, 5=5xx
}

# ============ 延迟指标（Histogram）============

# 请求延迟分布（ms）
envoy_cluster_upstream_rq_time_bucket{
  envoy_cluster_name="...",
  le="10"      # 10ms 以内的请求数
}
envoy_cluster_upstream_rq_time_count{...}
envoy_cluster_upstream_rq_time_sum{...}

# ============ 连接指标（Gauge）============

# 当前活跃连接数
envoy_listener_downstream_cx_active{
  envoy_listener_address="0.0.0.0_443"
}

# 上游连接池健康状态
envoy_cluster_membership_healthy{
  envoy_cluster_name="outbound|8080||order-svc.default.svc.cluster.local"
}
envoy_cluster_membership_total{...}
```
## 3.2 APISIX 核心指标

```
# APISIX 指标端点（prometheus 插件）
# curl http://apisix-gateway:9091/apisix/prometheus/metrics

# ============ 请求指标 ============

# 请求总数（Counter）
apisix_http_requests_total{
  route="order-api-v1",
  service="order-svc",
  consumer="alice",
  node="10.0.1.5:8080",
  status_code="200"
}

# 请求延迟（Histogram）
apisix_http_latency_bucket{
  type="request",    # request（总延迟）/ upstream（上游延迟）/ apisix（网关自身延迟）
  route="order-api-v1",
  le="100"
}

# ============ 上游健康检查指标 ============

# 上游节点状态
apisix_upstream_health_check{
  name="order-svc",
  ip="10.0.1.5",
  port="8080",
  type="http"
}  # 1=健康, 0=不健康

# ============ 带宽指标 ============

# 入/出流量字节数
apisix_bandwidth_bytes_total{
  route="order-api-v1",
  direction="ingress"   # ingress / egress
}
```

## 3.3 Kong 核心指标

```
# Kong 指标端点（prometheus 插件）
# curl http://kong-gateway:8001/metrics

# 请求总数
kong_http_requests_total{
  service="order-service",
  route="order-v1",
  status_code="200",
  consumer="alice",
  workspace="default"
}

# 请求延迟
kong_request_latency_ms_bucket{
  service="order-service",
  route="order-v1",
  le="50"
}

# 带宽
kong_bandwidth_bytes_total{
  service="order-service",
  direction="ingress"
}

# 上游健康
kong_upstream_target_health{
  upstream="order-upstream",
  target="10.0.1.5:8080",
  address="10.0.1.5:8080",
  state="healthchecks_off",   # healthy / unhealthy / dns_error
  subsystem="http"
}
```

## 3.4 Envoy Gateway 核心指标

```
# Envoy Gateway（继承 Envoy 指标体系）

# HTTP 下行连接指标
envoy_http_downstream_cx_total{
  envoy_http_conn_manager_prefix="envoy_gateway_system"
}

# 路由级请求指标
envoy_route_upstream_rq_2xx{
  envoy_route_name="httproute/default/order-route/rule/0/match/0/order-svc"
}

# 控制平面指标（xDS 同步）
envoy_control_plane_connected_state  # 1=已连接

# EDS（Endpoint Discovery）
envoy_cluster_membership_degraded{
  envoy_cluster_name="..."
}
```

## 3.5 Traefik 核心指标

```
# Traefik 指标端点
# curl http://traefik:8080/metrics

# 请求总数
traefik_router_requests_total{
  code="200",
  method="GET",
  protocol="http",
  router="default-order-route@kubernetescrd",
  service="default-order-svc@kubernetescrd"
}

# 请求延迟
traefik_router_request_duration_seconds_bucket{
  code="200",
  method="GET",
  router="...",
  le="0.1"
}

# 入口点指标
traefik_entrypoint_requests_total{
  code="200",
  entrypoint="websecure",
  method="GET",
  protocol="https"
}

# 服务重试次数
traefik_service_retries_total{
  service="default-order-svc@kubernetescrd"
}
```

---

<!-- chunk: 4. 结构化访问日志 -->## 4. 结构化访问日志

## 4.1 JSON 日志格式标准化

```json
{
  "@timestamp": "2026-03-04T10:23:45.123Z",
  "log_type": "access",
  "gateway": "higress",

  "request": {
    "id": "a1b2c3d4-e5f6-7890-abcd-ef1234567890",
    "method": "POST",
    "scheme": "https",
    "host": "api.example.com",
    "path": "/api/v1/orders",
    "query": "trace=true",
    "headers": {
      "content-type": "application/json",
      "user-agent": "MyApp/2.0 (iOS)",
      "x-forwarded-for": "203.0.113.45"
    },
    "body_bytes": 256
  },

  "response": {
    "status_code": 201,
    "headers": {
      "content-type": "application/json",
      "x-request-id": "a1b2c3d4-..."
    },
    "body_bytes": 1024
  },

  "upstream": {
    "cluster": "outbound|8080||order-svc.default.svc.cluster.local",
    "host": "10.0.1.5:8080",
    "response_time_ms": 45.3,
    "retries": 0
  },

  "timing": {
    "total_ms": 52.1,
    "gateway_ms": 6.8,
    "upstream_ms": 45.3
  },

  "auth": {
    "consumer": "user-alice",
    "app_id": "app-mobile-ios",
    "auth_type": "jwt"
  },

  "trace": {
    "trace_id": "4bf92f3577b34da6a3ce929d0e0e4736",
    "span_id": "00f067aa0ba902b7",
    "sampled": true
  }
}
```

## 4.2 Higress 访问日志配置

```yaml
# Higress 自定义访问日志格式
apiVersion: v1
kind: ConfigMap
metadata:
  name: higress-config
  namespace: higress-system
data:
  higress: |
    tracing:
      enable: true
      sampling: 100.0
    accessLog:
      enable: true
      # JSON 格式访问日志
      format: |
        {
          "@timestamp": "%START_TIME%",
          "request_id": "%REQ(X-REQUEST-ID)%",
          "method": "%REQ(:METHOD)%",
          "path": "%REQ(X-ENVOY-ORIGINAL-PATH?:PATH)%",
          "protocol": "%PROTOCOL%",
          "response_code": "%RESPONSE_CODE%",
          "response_flags": "%RESPONSE_FLAGS%",
          "response_body_bytes": "%BYTES_SENT%",
          "request_body_bytes": "%BYTES_RECEIVED%",
          "duration_ms": "%DURATION%",
          "upstream_service_time": "%RESP(X-ENVOY-UPSTREAM-SERVICE-TIME)%",
          "upstream_host": "%UPSTREAM_HOST%",
          "upstream_cluster": "%UPSTREAM_CLUSTER%",
          "user_agent": "%REQ(USER-AGENT)%",
          "client_ip": "%REQ(X-FORWARDED-FOR)%",
          "authority": "%REQ(:AUTHORITY)%",
          "trace_id": "%REQ(X-B3-TRACEID)%",
          "span_id": "%REQ(X-B3-SPANID)%"
        }
```

## 4.3 APISIX 结构化日志配置

```yaml
# APISIX http-logger 插件（推送至 HTTP 端点）
plugins:
  http-logger:
    uri: "http://fluentd.logging.svc.cluster.local:9880/apisix.access"
    batch_max_size: 1000
    inactive_timeout: 5
    log_format:
      host: "$host"
      "@timestamp": "$time_iso8601"
      client_ip: "$remote_addr"
      status: "$status"
      latency: "$request_time"
      upstream_latency: "$upstream_response_time"
      request_method: "$request_method"
      request_uri: "$request_uri"
      bytes_sent: "$body_bytes_sent"
      route_id: "$route_id"
      service_id: "$service_id"
      consumer: "$consumer_name"
      trace_id: "$opentelemetry_context_traceparent"
```

## 4.4 ELK / Loki 集成

```yaml
# Fluentd 配置：从 Kubernetes 日志收集并转发
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluentd-config
data:
  fluent.conf: |
    <source>
      @type tail
      path /var/log/containers/higress-gateway-*.log
      pos_file /var/log/fluentd-higress.pos
      tag higress.access
      <parse>
        @type json
        time_key @timestamp
        time_format %Y-%m-%dT%H:%M:%S.%NZ
      </parse>
    </source>

    # 发送至 Loki
    <match higress.access>
      @type loki
      url "http://loki.monitoring.svc.cluster.local:3100"
      extra_labels {"job": "api-gateway", "product": "higress"}
      <label>
        response_code $.response_code
        upstream_cluster $.upstream_cluster
      </label>
      <buffer>
        flush_interval 5s
        chunk_limit_size 1m
      </buffer>
    </match>

    # 同时发送至 Elasticsearch（可选）
    <match higress.access>
      @type elasticsearch
      host elasticsearch.logging.svc.cluster.local
      port 9200
      index_name higress-access-%Y.%m.%d
      type_name _doc
    </match>
```

---

<!-- chunk: 5. 分布式链路追踪 -->## 5. 分布式链路追踪

## 5.1 OpenTelemetry 集成架构

```
  客户端         API 网关              微服务 A              微服务 B
     │              │                     │                     │
     │── 请求 ──────►│                     │                     │
     │              │  生成 Trace-ID       │                     │
     │              │  创建 Root Span      │                     │
     │              │                     │                     │
     │              │── 注入 Trace Header ►│                     │
     │              │   W3C Traceparent    │  创建子 Span        │
     │              │   traceparent:       │                     │
     │              │   00-{trace-id}-     │──────────────────►  │
     │              │   {span-id}-01       │     创建孙子 Span    │
     │              │                     │◄─────────────────── │
     │              │◄──────────────────  │                     │
     │◄── 响应 ──────│                     │                     │
     │              │                     │                     │
     │              ▼                     ▼                     ▼
                OTel Collector ← ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┘
                    │
                    ├──► Jaeger（链路存储与查询）
                    ├──► Tempo（Grafana 生态）
                    └──► Zipkin（兼容协议）
```

## 5.2 Higress 链路追踪配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: higress-config
  namespace: higress-system
data:
  higress: |
    tracing:
      enable: true
      sampling: 10.0    # 采样率 10%（生产环境建议 1-10%）
      timeout: 500      # 追踪数据发送超时（ms）
      maxPathTagLength: 256
      zipkin:
        address: "zipkin.tracing.svc.cluster.local:9411"
      # 或使用 OpenTelemetry Collector
      # opentelemetry:
      #   address: "otel-collector.monitoring.svc.cluster.local:4317"
```

## 5.3 APISIX 链路追踪配置

```yaml
# APISIX opentelemetry 插件
plugins:
  opentelemetry:
    sampler:
      name: "parentbased_traceidratio"
      options:
        fraction: 0.1     # 10% 采样
    additional_attributes:
      - "http.host"
      - "http.method"
    additional_header_prefix_attributes:
      - "X-Custom-"
    batch_span_processor:
      max_export_batch_size: 512
      inactive_timeout: 5
      max_queue_size: 2048

# APISIX 全局配置（config.yaml）
plugin_attr:
  opentelemetry:
    resource:
      service.name: "apisix-gateway"
      service.version: "3.8.0"
    collector:
      address: "otel-collector.monitoring.svc.cluster.local:4317"
      request_timeout: 3
      tls:
        insecure_skip_verify: true
    batch_span_processor:
      max_queue_size: 1024
```

## 5.4 OTel Collector 配置

```yaml
# OpenTelemetry Collector（DaemonSet 模式）
apiVersion: v1
kind: ConfigMap
metadata:
  name: otel-collector-config
data:
  otel-collector-config.yaml: |
    receivers:
      otlp:
        protocols:
          grpc:
            endpoint: "0.0.0.0:4317"
          http:
            endpoint: "0.0.0.0:4318"
      zipkin:
        endpoint: "0.0.0.0:9411"

    processors:
      batch:
        send_batch_size: 1024
        timeout: 10s
      # 过滤健康检查路径的追踪
      filter/health:
        traces:
          exclude:
            match_type: regexp
            attributes:
              - Key: http.url
                Value: ".*/health.*"
      # 资源属性注入
      resource:
        attributes:
          - key: "k8s.cluster.name"
            value: "prod-cluster"
            action: insert

    exporters:
      jaeger:
        endpoint: "jaeger-collector.tracing.svc.cluster.local:14250"
        tls:
          insecure: true
      otlp/tempo:
        endpoint: "tempo.monitoring.svc.cluster.local:4317"
        tls:
          insecure: true
      # 同时保留 Zipkin 兼容性
      zipkin:
        endpoint: "http://zipkin.tracing.svc.cluster.local:9411/api/v2/spans"

    service:
      pipelines:
        traces:
          receivers: [otlp, zipkin]
          processors: [batch, filter/health, resource]
          exporters: [jaeger, otlp/tempo]
```

## 5.5 各产品追踪支持对比

| 产品 | OTel | Zipkin | Jaeger | 采样配置 | Header 传播 |
|------|------|--------|--------|---------|-----------|
| **Higress** | ✅ OTel Collector | ✅ 原生 | ✅ 通过 Zipkin 协议 | 百分比采样 | W3C/B3 |
| **APISIX** | ✅ 原生插件 | ✅ 原生插件 | ✅ 原生插件 | 多策略可选 | W3C/B3/Skywalking |
| **Kong** | ✅ opentelemetry 插件 | ✅ zipkin 插件 | ✅ 通过 OTel | 百分比/头部 | W3C/B3 |
| **Envoy GW** | ✅ Envoy 原生 OTel | ✅ Envoy 原生 | ✅ Envoy 原生 | 多策略 | W3C/B3/Datadog |
| **Traefik** | ✅ 原生 OTel | ✅ 原生 | ✅ 原生 | 百分比 | W3C/B3 |

---

<!-- chunk: 6. Grafana 仪表盘设计 -->## 6. Grafana 仪表盘设计

## 6.1 核心面板布局

```
Grafana 仪表盘布局（API 网关总览）
┌─────────────────────────────────────────────────────────────────────┐
│  行 1：黄金信号摘要（Stat 组件）                                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────┐ │
│  │  总 RPS       │  │  P99 延迟    │  │  5xx 错误率   │  │  上游健康 │ │
│  │  12,450 req/s│  │  87 ms       │  │  0.02%       │  │  18/18   │ │
│  └──────────────┘  └──────────────┘  └──────────────┘  └──────────┘ │
├─────────────────────────────────────────────────────────────────────┤
│  行 2：流量趋势（Time Series）                                        │
│  ┌───────────────────────────┐  ┌─────────────────────────────────┐  │
│  │  每秒请求数（按状态码分色）  │  │  延迟百分位数（P50/P95/P99/P999）│  │
│  └───────────────────────────┘  └─────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────────┤
│  行 3：错误分析与路由 Top N                                           │
│  ┌──────────────────────┐  ┌─────────────────────────────────────┐  │
│  │  状态码分布（Pie Chart）│  │  Top 10 高延迟路由（Table）         │  │
│  └──────────────────────┘  └─────────────────────────────────────┘  │
├─────────────────────────────────────────────────────────────────────┤
│  行 4：上游健康与资源利用                                             │
│  ┌──────────────────────┐  ┌──────────────┐  ┌───────────────────┐  │
│  │  上游节点健康状态 (Table)│  │  CPU 利用率   │  │  内存使用率       │  │
│  └──────────────────────┘  └──────────────┘  └───────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

## 6.2 关键 PromQL 查询示例

```promql
# ===================== 流量指标 =====================

# APISIX：每秒总请求数
sum(rate(apisix_http_requests_total[1m]))

# APISIX：按路由分组的 RPS
sum by (route) (rate(apisix_http_requests_total[1m]))

# Higress/Envoy：每秒请求数
sum(rate(envoy_http_downstream_rq_total{
  envoy_http_conn_manager_prefix="ingress_http"
}[1m]))

# ===================== 延迟指标 =====================

# APISIX：P99 请求延迟（ms）
histogram_quantile(0.99,
  sum by (le, route) (
    rate(apisix_http_latency_bucket{type="request"}[5m])
  )
) * 1000

# APISIX：P99 上游延迟（排除网关自身开销）
histogram_quantile(0.99,
  sum by (le, route) (
    rate(apisix_http_latency_bucket{type="upstream"}[5m])
  )
) * 1000

# Higress/Envoy：P99 上游响应时间（ms）
histogram_quantile(0.99,
  sum by (le, envoy_cluster_name) (
    rate(envoy_cluster_upstream_rq_time_bucket[5m])
  )
)

# ===================== 错误率 =====================

# APISIX：5xx 错误率（%）
sum(rate(apisix_http_requests_total{status_code=~"5.."}[5m]))
/
sum(rate(apisix_http_requests_total[5m]))
* 100

# 按路由计算 4xx 错误率
sum by (route) (rate(apisix_http_requests_total{status_code=~"4.."}[5m]))
/
sum by (route) (rate(apisix_http_requests_total[5m]))
* 100

# ===================== 饱和度 =====================

# 网关 Pod CPU 使用率（%）
sum(rate(container_cpu_usage_seconds_total{
  namespace="higress-system",
  pod=~"higress-gateway-.*",
  container="higress-gateway"
}[5m])) by (pod)
/
sum(kube_pod_container_resource_limits{
  namespace="higress-system",
  resource="cpu",
  container="higress-gateway"
}) by (pod)
* 100

# 活跃连接数
sum(envoy_listener_downstream_cx_active{
  envoy_listener_address="0.0.0.0_443"
}) by (pod)

# ===================== 上游健康 =====================

# APISIX 上游节点健康率（%）
sum(apisix_upstream_health_check == 1)
/
count(apisix_upstream_health_check)
* 100

# Envoy 上游可用节点数
envoy_cluster_membership_healthy
/
envoy_cluster_membership_total
```

## 6.3 Grafana Dashboard as Code（Provisioning）

```yaml
# grafana/dashboards/api-gateway.yaml（Grafana Provisioning）
apiVersion: 1
providers:
  - name: "api-gateway-dashboards"
    orgId: 1
    folder: "API Gateway"
    type: file
    options:
      path: /var/lib/grafana/dashboards/api-gateway
```

---

<!-- chunk: 7. 告警规则 -->## 7. 告警规则

## 7.1 PrometheusRule 示例（关键告警）

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: api-gateway-alerts
  namespace: monitoring
  labels:
    prometheus: kube-prometheus
    role: alert-rules
spec:
  groups:
    # ============ 延迟告警 ============
    - name: api-gateway.latency
      interval: 30s
      rules:
        # P99 延迟过高
        - alert: APIGatewayHighP99Latency
          expr: |
            histogram_quantile(0.99,
              sum by (le, route) (
                rate(apisix_http_latency_bucket{type="request"}[5m])
              )
            ) * 1000 > 500
          for: 5m
          labels:
            severity: warning
            team: platform
          annotations:
            summary: "API 网关 P99 延迟过高（路由: {{ $labels.route }}）"
            description: "路由 {{ $labels.route }} 的 P99 延迟为 {{ $value | humanizeDuration }}，超过 500ms 阈值，持续 5 分钟。"
            runbook_url: "https://wiki.example.com/runbooks/gateway-high-latency"

        # P99 延迟严重告警
        - alert: APIGatewayLatencyCritical
          expr: |
            histogram_quantile(0.99,
              sum by (le) (
                rate(apisix_http_latency_bucket{type="request"}[5m])
              )
            ) * 1000 > 2000
          for: 2m
          labels:
            severity: critical
            team: platform
          annotations:
            summary: "API 网关 P99 延迟严重（超过 2 秒）"
            description: "整体 P99 延迟为 {{ $value | humanizeDuration }}，超过 2 秒严重阈值。"

    # ============ 错误率告警 ============
    - name: api-gateway.errors
      interval: 30s
      rules:
        # 5xx 错误率突增
        - alert: APIGateway5xxErrorSpikeWarning
          expr: |
            sum(rate(apisix_http_requests_total{status_code=~"5.."}[5m]))
            /
            sum(rate(apisix_http_requests_total[5m]))
            * 100 > 1
          for: 3m
          labels:
            severity: warning
            team: platform
          annotations:
            summary: "API 网关 5xx 错误率超过 1%"
            description: "5xx 错误率为 {{ $value | humanize }}%，超过 1% 阈值，持续 3 分钟。"

        # 5xx 错误率严重告警
        - alert: APIGateway5xxErrorSpikeCritical
          expr: |
            sum(rate(apisix_http_requests_total{status_code=~"5.."}[2m]))
            /
            sum(rate(apisix_http_requests_total[2m]))
            * 100 > 5
          for: 1m
          labels:
            severity: critical
            team: platform
          annotations:
            summary: "API 网关 5xx 错误率严重（超过 5%）"
            description: "5xx 错误率为 {{ $value | humanize }}%，超过 5% 严重阈值。"

        # 特定路由错误率高
        - alert: APIGatewayRouteHighErrorRate
          expr: |
            sum by (route) (rate(apisix_http_requests_total{status_code=~"5.."}[5m]))
            /
            sum by (route) (rate(apisix_http_requests_total[5m]))
            * 100 > 10
          for: 5m
          labels:
            severity: warning
            team: platform
          annotations:
            summary: "路由 {{ $labels.route }} 错误率过高"
            description: "路由 {{ $labels.route }} 的 5xx 错误率为 {{ $value | humanize }}%。"

    # ============ 上游健康告警 ============
    - name: api-gateway.upstream
      interval: 30s
      rules:
        # 上游节点全部不可用
        - alert: APIGatewayUpstreamAllDown
          expr: |
            sum by (upstream) (
              apisix_upstream_health_check == 1
            ) == 0
          for: 1m
          labels:
            severity: critical
            team: platform
          annotations:
            summary: "上游服务 {{ $labels.upstream }} 所有节点不可用"
            description: "上游 {{ $labels.upstream }} 的所有后端节点均不健康，服务完全中断。"

        # 上游节点可用率低
        - alert: APIGatewayUpstreamDegraded
          expr: |
            sum by (name) (apisix_upstream_health_check == 1)
            /
            count by (name) (apisix_upstream_health_check)
            * 100 < 50
          for: 3m
          labels:
            severity: warning
            team: platform
          annotations:
            summary: "上游 {{ $labels.name }} 可用节点不足 50%"
            description: "上游 {{ $labels.name }} 仅 {{ $value | humanize }}% 的节点健康。"

    # ============ 资源饱和告警 ============
    - name: api-gateway.saturation
      interval: 60s
      rules:
        # CPU 利用率高
        - alert: APIGatewayHighCPU
          expr: |
            sum(rate(container_cpu_usage_seconds_total{
              namespace=~"higress-system|apisix",
              container=~"higress-gateway|apisix"
            }[5m])) by (pod, namespace)
            /
            sum(kube_pod_container_resource_limits{
              resource="cpu",
              container=~"higress-gateway|apisix"
            }) by (pod, namespace)
            * 100 > 80
          for: 10m
          labels:
            severity: warning
            team: platform
          annotations:
            summary: "网关 Pod {{ $labels.pod }} CPU 使用率超过 80%"
            description: "Pod {{ $labels.pod }} CPU 使用率为 {{ $value | humanize }}%，建议扩容。"

        # 流量骤降（可能是健康检查挂了或断流）
        - alert: APIGatewayTrafficDrop
          expr: |
            sum(rate(apisix_http_requests_total[5m]))
            <
            sum(rate(apisix_http_requests_total[5m] offset 30m)) * 0.3
          for: 5m
          labels:
            severity: warning
            team: platform
          annotations:
            summary: "API 网关流量骤降超过 70%"
            description: "当前 RPS 相较 30 分钟前下降超过 70%，请检查网关状态或上游服务。"
```

## 7.2 Alertmanager 路由配置

```yaml
# Alertmanager 告警路由
apiVersion: v1
kind: ConfigMap
metadata:
  name: alertmanager-config
data:
  alertmanager.yml: |
    global:
      resolve_timeout: 5m

    route:
      receiver: "default-receiver"
      group_by: ["alertname", "namespace"]
      group_wait: 30s
      group_interval: 5m
      repeat_interval: 4h
      routes:
        # Critical 级别：立即通知 + PagerDuty
        - matchers:
          - severity="critical"
          receiver: pagerduty-critical
          group_wait: 0s
          repeat_interval: 30m
        # Warning 级别：Slack + 邮件
        - matchers:
          - severity="warning"
          - team="platform"
          receiver: slack-platform
          repeat_interval: 2h
    receivers:
      - name: "pagerduty-critical"
        pagerduty_configs:
          - routing_key: "<PAGERDUTY_KEY>"
            severity: critical
            description: "{{ .CommonAnnotations.summary }}"

      - name: "slack-platform"
        slack_configs:
          - api_url: "<SLACK_WEBHOOK>"
            channel: "#platform-alerts"
            title: "{{ .GroupLabels.alertname }}"
            text: "{{ range .Alerts }}{{ .Annotations.description }}{{ end }}"
            color: '{{ if eq .Status "firing" }}danger{{ else }}good{{ end }}'
```

---

<!-- chunk: 8. 各产品可观测性对比表 -->## 8. 各产品可观测性对比表

## 8.1 指标能力对比

| 能力 | Higress | APISIX | Kong | Envoy Gateway | Traefik |
|------|---------|-------|------|--------------|---------|
| **Prometheus 原生支持** | ⭐⭐⭐⭐⭐ Envoy 内置 | ⭐⭐⭐⭐⭐ prometheus 插件 | ⭐⭐⭐⭐ 插件支持 | ⭐⭐⭐⭐⭐ Envoy 内置 | ⭐⭐⭐⭐⭐ 原生 /metrics |
| **指标维度丰富度** | ⭐⭐⭐⭐⭐ 百余个 Envoy 指标 | ⭐⭐⭐⭐ 路由/消费者/服务 | ⭐⭐⭐⭐ 完善 | ⭐⭐⭐⭐⭐ 最丰富 | ⭐⭐⭐ 基础指标 |
| **自定义指标（插件注册）** | ⭐⭐⭐⭐ Wasm proxy_define_metric | ⭐⭐⭐ 插件扩展 | ⭐⭐⭐ 插件扩展 | ⭐⭐⭐⭐ Wasm + ext_proc | ⭐⭐ 有限 |
| **按路由细粒度指标** | ⭐⭐⭐⭐ 支持 | ⭐⭐⭐⭐⭐ 完善 | ⭐⭐⭐⭐⭐ 完善 | ⭐⭐⭐⭐ 支持 | ⭐⭐⭐ 基本支持 |

## 8.2 日志能力对比

| 能力 | Higress | APISIX | Kong | Envoy Gateway | Traefik |
|------|---------|-------|------|--------------|---------|
| **访问日志 JSON 格式** | ⭐⭐⭐⭐⭐ Envoy access log 高度自定义 | ⭐⭐⭐⭐⭐ 内置 + 插件 | ⭐⭐⭐⭐ 完善 | ⭐⭐⭐⭐⭐ Envoy 内置 | ⭐⭐⭐⭐ 支持 |
| **HTTP Logger（实时推送）** | ⭐⭐⭐ Wasm 插件 | ⭐⭐⭐⭐⭐ http-logger 等多种 | ⭐⭐⭐⭐⭐ file-log/http-log | ⭐⭐⭐ 通过 ext_proc | ⭐⭐⭐ 支持 |
| **字段动态配置** | ⭐⭐⭐⭐⭐ Envoy 格式符 | ⭐⭐⭐⭐ log_format 配置 | ⭐⭐⭐⭐ 可配置 | ⭐⭐⭐⭐⭐ 灵活 | ⭐⭐⭐ 固定字段为主 |
| **日志采样** | ⭐⭐⭐ 插件支持 | ⭐⭐⭐ 插件支持 | ⭐⭐⭐ 插件支持 | ⭐⭐⭐⭐ 原生 | ⭐⭐ 有限 |

## 8.3 链路追踪能力对比

| 能力 | Higress | APISIX | Kong | Envoy Gateway | Traefik |
|------|---------|-------|------|--------------|---------|
| **OpenTelemetry** | ⭐⭐⭐⭐ 通过 Collector | ⭐⭐⭐⭐⭐ 原生插件 | ⭐⭐⭐⭐ 插件 | ⭐⭐⭐⭐⭐ Envoy 原生 | ⭐⭐⭐⭐⭐ 原生 |
| **Zipkin** | ⭐⭐⭐⭐⭐ 原生支持 | ⭐⭐⭐⭐⭐ 原生插件 | ⭐⭐⭐⭐⭐ 原生 | ⭐⭐⭐⭐⭐ Envoy 原生 | ⭐⭐⭐⭐⭐ 原生 |
| **Skywalking** | ⭐⭐⭐ 插件支持 | ⭐⭐⭐⭐⭐ 原生插件 | ⭐⭐⭐ 插件 | ⭐⭐ 有限 | ⭐⭐ 有限 |
| **采样策略丰富度** | ⭐⭐⭐ 概率采样 | ⭐⭐⭐⭐⭐ 多策略 | ⭐⭐⭐⭐ 完善 | ⭐⭐⭐⭐⭐ Envoy 原生多策略 | ⭐⭐⭐⭐ 完善 |
| **W3C TraceContext 传播** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |

---

<!-- chunk: 参考资料 -->## 参考资料

- [OpenTelemetry 官方文档](https://opentelemetry.io/docs/)
- [Prometheus 最佳实践](https://prometheus.io/docs/practices/)
- [APISIX 可观测性文档](https://apisix.apache.org/docs/apisix/plugins/prometheus/)
- [Higress 可观测性配置](https://higress.io/docs/latest/user/observability/)
- [Envoy 指标参考（envoy_http_*）](https://www.envoyproxy.io/docs/envoy/latest/operations/stats_overview)
- [Grafana 仪表盘最佳实践](https://grafana.com/docs/grafana/latest/dashboards/build-dashboards/best-practices/)
- [Google SRE Book - 四大黄金信号](https://sre.google/sre-book/monitoring-distributed-systems/)
- 关联文档：[可观测性 可观测性基础](../可观测性/)
- 关联文档：[可观测性 企业监控体系](../domain-20-enterprise-monitoring/)
- 关联文档：[01 - API 网关架构总览](./01-api-gateway-architecture-overview.md)
- 关联文档：[12 - API 网关安全体系](./11-api-gateway-security-practices.md)

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-40-cloud-native-api-gateway MOC
- [[05-网络/README.md|Domain 03: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technolo...]]
- Domain-40 云原生 API 网关 — 开源项目索引
- 01 - 云原生 API 网关架构总览
- 02 - Kubernetes Gateway API 标准深度解析
- 03 - API 网关选型指南与对比矩阵
- 04 - Higress 云原生 API 网关企业级实践
- 05 - Apache APISIX 企业级 API 网关实践
- 06 - Kong API 网关企业级实践
- 07 - Envoy Gateway 企业级实践
- 08 - Traefik API 网关企业级实践
- 09 - 传统 Ingress 控制器向云原生 API 网关迁移

## See Also

- 10-wasm-plugin-ecosystem
- 11-api-gateway-security-practices
- 13-api-gateway-performance-benchmarks
- 14-api-gateway-production-operations


<!-- risk-assessed -->
