---
title: OpenTelemetry Collector 故障排查指南 [topic-structural-trouble-shooting]
description: 'title: OpenTelemetry Collector 故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- prometheus
- jaeger
- docker
- kafka
- elasticsearch
- hpa
- job
- gateway
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- OpenTelemetry Collector 故障排查指南 是什么
- 如何 OpenTelemetry Collector 故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- OpenTelemetry Collector 故障排查指南 故障排查
- OpenTelemetry Collector 故障排查指南 排障步骤
trigger_keywords:
- OpenTelemetry
- Collector
- 故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- ebpf-basics
- kafka-basics
- tls-basics
- tracing-basics
- observability-basics
created: "2026-05-23"
---

title: [[OpenTelemetry|OpenTelemetry]] Collector 故障排查指南
description: '# OpenTelemetry Collector 故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[Prometheus|prometheus]]
- [[Jaeger|jaeger]]
- kafka
- elasticsearch
- hpa
- job
- gateway
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- OpenTelemetry Collector 故障排查指南 是什么
- 如何 OpenTelemetry Collector 故障排查指南
- OpenTelemetry Collector 故障排查指南 故障排查
- OpenTelemetry Collector 故障排查指南 排障步骤
trigger_keywords:
- OpenTelemetry
- Collector
- 故障排查指南
- structural
- trouble
- shooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# OpenTelemetry Collector 故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | OpenTelemetry Collector v0.90+ | **最后更新**: 2026-04 | **难度**: 高级

---

## 0. 10 分钟快速诊断

1. **Collector Pod 状态**：`kubectl get pods -n observability -l app.kubernetes.io/name=opentelemetry-collector`，确认 Running 且无频繁重启。
2. **组件健康检查**：`curl http://://<collector-pod>:13133/` 或访问 `/health` endpoint，确认 Collector 自身健康。
3. **接收端口连通性**：从客户端 Pod `telnet/nc` 测试 Collector Service 的接收端口（4317 gRPC / 4318 HTTP）。
4. **Exporter 错误**：查看 Collector 日志中的 `error` 和 `refused` 关键字，定位导出失败。
5. **指标自查**：访问 `http://://<collector-pod>:8888/metrics`，查看 `otelcol_exporter_send_failed_*` 和 `otelcol_receiver_refused_*`。
6. **快速缓解**：
   - 后端不可达：临时切换 exporter 到 `debug` 或 `file` 避免数据丢失。
   - 内存溢出：调大 Collector 内存限制，或启用 `memory_limiter` processor。
   - 接收拒绝：扩容 Collector 副本数或增大队列缓冲。
7. **证据留存**：保存 Collector 配置 ConfigMap、Pod 日志、metrics 快照、客户端 SDK 配置。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 数据接收失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 客户端发送被拒绝 | `rpc error: code = Unavailable desc = connection refused` | OTLP Client SDK | 应用日志 |
| gRPC 握手失败 | `transport: Error while dialing` | OTLP Client SDK | 应用日志 |
| HTTP 429 限流 | `429 Too Many Requests` | OTLP HTTP Client | 应用日志 |
| 端口不可达 | `dial tcp <ip>:4317: connect: connection refused` | 应用日志 | Pod 内 telnet 测试 |
| mTLS 握手失败 | `certificate signed by unknown authority` | OTLP Client SDK | 应用日志 |

#### 1.1.2 数据处理异常

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 内存溢出 | `OOMKilled` / `fatal error: runtime: out of memory` | Collector Pod | `kubectl describe pod` |
| 处理器丢弃数据 | `processor dropped data` | Collector 日志 | Collector Pod 日志 |
| 批处理失败 | `batch processor failed` | Collector 日志 | Collector Pod 日志 |
| 属性提取失败 | `extracting attributes failed` | Collector 日志 | Collector Pod 日志 |
| 采样配置错误 | `invalid sampling configuration` | Collector 日志 | Collector Pod 日志 |

#### 1.1.3 数据导出失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Exporter 连接超时 | `context deadline exceeded` | Collector Exporter | Collector 日志 |
| 后端返回 5xx | `error exporting items, request to http://... failed` | Collector Exporter | Collector 日志 |
| 认证失败 | `authentication handshake failed` | Collector Exporter | Collector 日志 |
| 队列满导致丢弃 | `sending_queue is full` | Collector Exporter | Collector 日志 |
| 持久队列损坏 | `file storage corruption detected` | Collector Exporter | Collector 日志 |

#### 1.1.4 生产环境典型场景

| 场景 | 典型现象 | 根本原因 | 解决方向 |
|------|----------|----------|----------|
| **大促期间 Collector OOM** | 流量激增后 Collector Pod 反复重启 | 未配置 `memory_limiter`，批处理积压 | 启用 memory_limiter + 增大内存 + HPA |
| **链路追踪数据不完整** | Jaeger 中只能看到部分服务的 span | 部分服务未正确配置 OTLP endpoint | 统一 SDK 配置，验证所有服务上报 |
| **指标标签爆炸导致后端卡死** | Prometheus remote write 超时，Cardinality 过高 | 未过滤的高基数标签（如 user_id） | 配置 `resource` / `attributes` processor 过滤 |
| **跨集群 Collector 级联问题** | Region A Collector 问题后 Region B 也过载 | 未配置 exporter 失败回退 + 本地队列 | 配置 persistent_queue + 降级策略 |

### 1.2 报错查看方式汇总

```bash
# Collector Pod 状态
kubectl get pods -n observability -l app.kubernetes.io/name=opentelemetry-collector

# Collector 日志
kubectl logs -n observability deployment/opentelemetry-collector --tail=500

# Collector 自身指标
curl -s http://opentelemetry-collector.observability.svc.cluster.local:8888/metrics

# 从客户端测试连通性
kubectl exec -it <client-pod> -- nc -zv otel-collector.observability.svc.cluster.local 4317
kubectl exec -it <client-pod> -- nc -zv otel-collector.observability.svc.cluster.local 4318

# 查看 OTLP 接收统计
curl -s http://opentelemetry-collector.observability.svc.cluster.local:8888/metrics | \
  grep -E "otelcol_receiver_accepted|otelcol_receiver_refused"

# 查看 Exporter 发送统计
curl -s http://opentelemetry-collector.observability.svc.cluster.local:8888/metrics | \
  grep -E "otelcol_exporter_sent|otelcol_exporter_send_failed"
```

---

## 2. 排查方法与步骤

### 2.1 诊断原理说明

OpenTelemetry Collector 的数据流架构：

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  Receivers  │────▶│  Processors │────▶│   Exporters │────▶│  Backends   │
│  (接收)      │     │  (处理)      │     │  (导出)      │     │  (后端存储)  │
├─────────────┤     ├─────────────┤     ├─────────────┤     ├─────────────┤
│ OTLP/gRPC   │     │ batch       │     │ OTLP        │     │ Prometheus  │
│ OTLP/HTTP   │     │ memory_limit│     │ Jaeger      │     │ Jaeger      │
│ Prometheus  │     │ resource    │     │ Zipkin      │     │ Zipkin      │
│ Jaeger      │     │ attributes  │     │ Kafka       │     │ Elasticsearch│
│ Zipkin      │     │ tail_sampling│    │ file        │     │ Kafka       │
│ Kafka       │     │ probabilistic│    │ debug       │     │ CloudWatch  │
└─────────────┘     └─────────────┘     └─────────────┘     └─────────────┘
        │
        ▼
┌─────────────┐
│ Extensions  │
│ - health_check│
│ - pprof      │
│ - zpages     │
│ - file_storage│
└─────────────┘
```

**关键概念**：
- **Pipeline**：数据在 Collector 中的处理路径，分为 `traces`、`metrics`、`logs` 三种类型
- **Batch Processor**：将数据批处理以减少后端调用次数，但会增加延迟
- **Memory Limiter**：监控内存使用并在达到阈值时拒绝新数据，防止 OOM
- **Sending Queue**：Exporter 内部的队列，用于缓冲和重试

### 2.2 排查逻辑决策树

```
OpenTelemetry Collector 问题
    ├── 客户端无法上报
    │   ├── 网络不可达？
    │   │   ├── Service DNS 解析失败？──► 检查 Service 和 DNS
    │   │   ├── 端口未监听？──► 检查 Collector 配置中的 receivers
    │   │   └── NetworkPolicy 阻断？──► 放通客户端到 Collector 的流量
    │   ├── 协议不匹配？
    │   │   ├── 客户端发送 gRPC 但 Collector 只开 HTTP？──► 统一协议
    │   │   └── mTLS 配置不一致？──► 检查证书和 TLS 配置
    │   └── 客户端 SDK 配置错误？
    │       ├── Endpoint 地址错误？──► 检查 OTEL_EXPORTER_OTLP_ENDPOINT
    │       └── Headers/Token 缺失？──► 检查认证配置
    ├── Collector 自身异常
    │   ├── OOMKilled？
    │   │   ├── 未启用 memory_limiter？──► 启用并调低 limit
    │   │   └── batch size 过大？──► 调小 batch processor 的 size
    │   ├── CPU Throttling？
    │   │   └── limits.cpu 过低？──► 增大 CPU limits
    │   └── 配置加载失败？
    │       └── YAML 语法错误？──► 验证 otelcol config
    ├── 数据处理丢失
    │   ├── Processor 丢弃？
    │   │   ├── memory_limiter 触发？──► 调大内存或降低采样率
    │   │   └── sampling 过滤过多？──► 调整采样策略
    │   └── Exporter 队列满？
    │       ├── 后端写入慢？──► 优化后端或扩容
    │       └── 队列大小不足？──► 增大 sending_queue.queue_size
    └── 后端接收不到数据
        ├── Exporter 连接失败？
        │   ├── 后端地址错误？──► 检查 exporter endpoint
        │   ├── 后端认证失败？──► 检查 exporter headers/api_key
        │   └── 后端限流？──► 申请提升后端限额
        └── 数据格式不兼容？
            └── Collector 与后端版本不匹配？──► 升级 Collector 或后端
```

### 2.3 详细诊断命令

#### Collector 全景诊断

```bash
#!/bin/bash
# OpenTelemetry Collector 全景诊断脚本

NAMESPACE=${1:-observability}

echo "=== OpenTelemetry Collector 全景诊断 ==="

# 1. Pod 状态
echo "1. Collector Pod 状态:"
kubectl get pods -n $NAMESPACE -l app.kubernetes.io/name=opentelemetry-collector -o json | jq -r '
  .items[] | "  \(.metadata.name): phase=\(.status.phase), restarts=\(.status.containerStatuses[0].restartCount), ready=\(.status.containerStatuses[0].ready)"
'

# 2. 配置检查
echo ""
echo "2. Collector 配置:"
CONFIGMAP=$(kubectl get pods -n $NAMESPACE -l app.kubernetes.io/name=opentelemetry-collector -o json | \
  jq -r '.items[0].spec.volumes[] | select(.configMap != null) | .configMap.name' | head -1)
if [ -n "$CONFIGMAP" ]; then
  echo "  ConfigMap: $CONFIGMAP"
  kubectl get configmap $CONFIGMAP -n $NAMESPACE -o jsonpath='{.data.otel-collector-config\.yaml}' 2>/dev/null | \
    head -50
else
  echo "  无法确定 ConfigMap"
fi

# 3. 健康检查
echo ""
echo "3. 健康检查:"
COLLECTOR_POD=$(kubectl get pods -n $NAMESPACE -l app.kubernetes.io/name=opentelemetry-collector -o jsonpath='{.items[0].metadata.name}')
if [ -n "$COLLECTOR_POD" ]; then
  kubectl exec -n $NAMESPACE $COLLECTOR_POD -- wget -qO- http://localhost:13133/ 2>/dev/null || \
    echo "  健康检查失败"
else
  echo "  未找到 Collector Pod"
fi

# 4. 接收器指标
echo ""
echo "4. 接收器统计:"
if [ -n "$COLLECTOR_POD" ]; then
  kubectl exec -n $NAMESPACE $COLLECTOR_POD -- wget -qO- http://localhost:8888/metrics 2>/dev/null | \
    grep -E "otelcol_receiver_(accepted|refused)" | head -10
fi

# 5. 导出器指标
echo ""
echo "5. 导出器统计:"
if [ -n "$COLLECTOR_POD" ]; then
  kubectl exec -n $NAMESPACE $COLLECTOR_POD -- wget -qO- http://localhost:8888/metrics 2>/dev/null | \
    grep -E "otelcol_exporter_(sent|send_failed)" | head -10
fi

# 6. 错误日志摘要
echo ""
echo "6. 错误日志摘要:"
kubectl logs -n $NAMESPACE -l app.kubernetes.io/name=opentelemetry-collector --tail=200 2>/dev/null | \
  grep -iE "error|fail|refused|dropped|timeout" | tail -15
```

#### 客户端上报诊断

```bash
#!/bin/bash
# 客户端 OTLP 上报诊断

echo "=== 客户端 OTLP 上报诊断 ==="

# 1. 环境变量检查
echo "1. OTLP 环境变量:"
env | grep -E "^OTEL_" | sort

# 2. Collector 连通性测试
echo ""
echo "2. Collector 连通性测试:"
OTLP_ENDPOINT=${OTEL_EXPORTER_OTLP_ENDPOINT:-"http://localhost:4317"}
echo "  OTLP Endpoint: $OTLP_ENDPOINT"

# 解析 host 和 port
HOST=$(echo $OTLP_ENDPOINT | sed -E 's|.*://||' | cut -d'/' -f1 | cut -d':' -f1)
PORT=$(echo $OTLP_ENDPOINT | sed -E 's|.*://||' | cut -d'/' -f1 | cut -d':' -s -f2)
PORT=${PORT:-4317}

echo "  Host: $HOST, Port: $PORT"

# TCP 连通性
if command -v nc &>/dev/null; then
  timeout 3 nc -zv $HOST $PORT 2>&1 && echo "  ✓ TCP 连通" || echo "  ✗ TCP 不通"
else
  timeout 3 bash -c "echo > /dev/tcp/$HOST/$PORT" 2>/dev/null && echo "  ✓ TCP 连通" || echo "  ✗ TCP 不通"
fi

# 3. gRPC 健康检查（如安装 grpcurl）
echo ""
echo "3. gRPC 健康检查:"
if command -v grpcurl &>/dev/null; then
  grpcurl -plaintext $HOST:$PORT grpc.health.v1.Health/Check 2>/dev/null || \
    echo "  gRPC 健康检查失败"
else
  echo "  grpcurl 未安装，跳过"
fi

# 4. HTTP 健康检查（OTLP/HTTP）
echo ""
echo "4. HTTP 端口测试:"
HTTP_PORT=${OTEL_EXPORTER_OTLP_TRACES_ENDPOINT:-"$OTLP_ENDPOINT"}
if echo "$HTTP_PORT" | grep -q ":4318"; then
  curl -s -o /dev/null -w "HTTP Status: %{http_code}\n" $HTTP_PORT/v1/traces 2>/dev/null || \
    echo "  HTTP 端口测试失败"
fi
```

---

## 3. 解决方案与风险控制

### 3.1 Collector 配置优化

#### 方案一：生产级 Collector 配置

```yaml
# OpenTelemetry Collector 生产级配置
receivers:
  otlp:
    protocols:
      grpc:
        endpoint: 0.0.0.0:4317
        max_recv_msg_size_mib: 64   # 增大消息大小限制
        keepalive:
          server_parameters:
            max_connection_age: 120s
      http:
        endpoint: 0.0.0.0:4318
        cors:
          allowed_origins: ["*"]
          allowed_headers: ["*"]
  prometheus:
    config:
      scrape_configs:
      - job_name: 'otel-collector'
        scrape_interval: 10s
        static_configs:
        - targets: ['0.0.0.0:8888']

processors:
  # 内存限制器：防止 OOM
  memory_limiter:
    check_interval: 1s
    limit_mib: 4000       # 内存上限 4GB
    spike_limit_mib: 800  # 突发限制 800MB

  # 批处理器：合并数据减少后端调用
  batch:
    timeout: 1s
    send_batch_size: 1024
    send_batch_max_size: 2048

  # 资源处理器：添加/修改资源属性
  resource:
    attributes:
    - key: k8s.cluster.name
      value: production
      action: upsert
    - key: environment
      value: prod
      action: upsert

  # 属性处理器：过滤高基数标签
  attributes/filter:
    actions:
    - key: user_id
      action: delete          # 删除高基数标签
    - key: request_id
      action: delete
    - key: trace_id
      action: delete

  # 尾部采样：减少 trace 数据量
  tail_sampling:
    decision_wait: 10s
    num_traces: 100000
    expected_new_traces_per_sec: 10000
    policies:
    - name: errors
      type: status_code
      status_code: {status_codes: [ERROR]}
    - name: slow_requests
      type: latency
      latency: {threshold_ms: 500}
    - name: probabilistic
      type: probabilistic
      probabilistic: {sampling_percentage: 10}

exporters:
  # OTLP 导出到 Jaeger
  otlp/jaeger:
    endpoint: jaeger-collector.observability.svc.cluster.local:4317
    tls:
      insecure: true
    sending_queue:
      enabled: true
      num_consumers: 10
      queue_size: 10000
    retry_on_failure:
      enabled: true
      initial_interval: 1s
      max_interval: 10s
      max_elapsed_time: 30s

  # Prometheus Remote Write
  prometheusremotewrite:
    endpoint: http://prometheus.observability.svc.cluster.local:9090/api/v1/write
    target_info:
      enabled: true
    max_batch_size_bytes: 32768

  # 调试导出器（故障排查时使用）
  debug:
    verbosity: detailed

  # 文件导出器（紧急备份）
  file/backup:
    path: /tmp/otel-backup.json

extensions:
  health_check:
    endpoint: 0.0.0.0:13133
  pprof:
    endpoint: 0.0.0.0:1777
  zpages:
    endpoint: 0.0.0.0:55679

service:
  extensions: [health_check, pprof, zpages]
  pipelines:
    traces:
      receivers: [otlp]
      processors: [memory_limiter, resource, tail_sampling, batch]
      exporters: [otlp/jaeger, debug]
    metrics:
      receivers: [otlp, prometheus]
      processors: [memory_limiter, resource, attributes/filter, batch]
      exporters: [prometheusremotewrite, debug]
    logs:
      receivers: [otlp]
      processors: [memory_limiter, resource, batch]
      exporters: [debug]
```

#### 方案二：Collector Deployment 高可用配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: opentelemetry-collector
  namespace: observability
spec:
  replicas: 3                    # 多副本负载均衡
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  selector:
    matchLabels:
      app.kubernetes.io/name: opentelemetry-collector
  template:
    metadata:
      labels:
        app.kubernetes.io/name: opentelemetry-collector
    spec:
      containers:
      - name: otel-collector
        image: otel/opentelemetry-collector-contrib:0.96.0
        args:
        - --config=/conf/otel-collector-config.yaml
        ports:
        - containerPort: 4317      # OTLP gRPC
          name: otlp-grpc
        - containerPort: 4318      # OTLP HTTP
          name: otlp-http
        - containerPort: 8888      # Metrics
          name: metrics
        - containerPort: 13133     # Health check
          name: health
        resources:
          limits:
            cpu: "2"
            memory: "5Gi"          # 比 memory_limiter 的 limit 大 1Gi
          requests:
            cpu: "1"
            memory: "2Gi"
        volumeMounts:
        - name: config
          mountPath: /conf
        - name: tmp
          mountPath: /tmp
        livenessProbe:
          httpGet:
            path: /
            port: 13133
          initialDelaySeconds: 10
          periodSeconds: 5
        readinessProbe:
          httpGet:
            path: /
            port: 13133
          initialDelaySeconds: 5
          periodSeconds: 5
      volumes:
      - name: config
        configMap:
          name: otel-collector-config
      - name: tmp
        emptyDir:
          sizeLimit: 1Gi
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: kubernetes.io/hostname
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app.kubernetes.io/name: opentelemetry-collector
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: otel-collector-hpa
  namespace: observability
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: opentelemetry-collector
  minReplicas: 3
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 100
        periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
```

### 3.2 客户端 SDK 配置

```yaml
# 应用 Deployment 中的 OTLP 环境变量配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  template:
    spec:
      containers:
      - name: app
        image: my-app:v1.0
        env:
        # OTLP Endpoint
        - name: OTEL_EXPORTER_OTLP_ENDPOINT
          value: "http://opentelemetry-collector.observability.svc.cluster.local:4317"
        # 协议选择
        - name: OTEL_EXPORTER_OTLP_PROTOCOL
          value: "grpc"           # 或 "http/protobuf"
        # 超时配置
        - name: OTEL_EXPORTER_OTLP_TIMEOUT
          value: "10000"          # 10 秒
        # 批量导出
        - name: OTEL_BSP_MAX_QUEUE_SIZE
          value: "2048"
        - name: OTEL_BSP_MAX_EXPORT_BATCH_SIZE
          value: "512"
        - name: OTEL_BSP_SCHEDULE_DELAY
          value: "1000"           # 1 秒
        # 采样率
        - name: OTEL_TRACES_SAMPLER
          value: "parentbased_traceidratio"
        - name: OTEL_TRACES_SAMPLER_ARG
          value: "0.1"            # 10% 采样
        # 服务属性
        - name: OTEL_SERVICE_NAME
          value: "my-app"
        - name: OTEL_RESOURCE_ATTRIBUTES
          value: "deployment.environment=prod,host.name=$(HOSTNAME)"
```

### 3.3 风险控制与回滚

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 修改 Collector 配置 | ⭐ 低 | 滚动更新，短暂中断 | 恢复原始 ConfigMap |
| 调整采样策略 | ⭐ 低 | 影响数据量，不影响功能 | 恢复原始采样配置 |
| 更换 Exporter 后端 | ⭐ 低 | 数据路由到不同后端 | 恢复原始 exporter 配置 |
| 启用/禁用 Processor | ⭐ 低 | 影响数据处理流程 | 恢复原始 pipeline 配置 |
| 升级 Collector 版本 | ⭐⭐ 中 | 可能存在配置格式变更 | 使用旧版本镜像回滚 |
| 调整 memory_limiter | ⭐ 低 | 影响数据丢弃阈值 | 恢复原始限制值 |

### 3.4 验证与监控

#### Collector 健康检查脚本

```bash
#!/bin/bash
# Collector 健康检查脚本

NAMESPACE=${1:-observability}

echo "=== Collector 健康检查 ==="

# 1. Pod 健康
kubectl get pods -n $NAMESPACE -l app.kubernetes.io/name=opentelemetry-collector -o json | jq -r '
  .items[] | "  \(.metadata.name): \(.status.phase) (restarts: \(.status.containerStatuses[0].restartCount))"
'

# 2. 指标自查
COLLECTOR_SVC="opentelemetry-collector.$NAMESPACE.svc.cluster.local:8888"
echo ""
echo "2. 接收器统计:"
curl -s http://$COLLECTOR_SVC/metrics 2>/dev/null | grep -E "otelcol_receiver_accepted_span" | head -5

echo ""
echo "3. 导出器统计:"
curl -s http://$COLLECTOR_SVC/metrics 2>/dev/null | grep -E "otelcol_exporter_sent_span" | head -5

echo ""
echo "4. 丢弃统计:"
curl -s http://$COLLECTOR_SVC/metrics 2>/dev/null | grep -E "otelcol_processor_dropped|otelcol_exporter_send_failed" | head -5
```

#### Prometheus 监控告警

```yaml
# OpenTelemetry Collector 监控告警
groups:
- name: opentelemetry-collector
  rules:
  - alert: OtelCollectorDown
    expr: |
      up{job="opentelemetry-collector"} == 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "OpenTelemetry Collector 不可用"
      description: "Collector {{ $labels.instance }} 已宕机"

  - alert: OtelCollectorHighMemoryUsage
    expr: |
      process_memory_rss{job="opentelemetry-collector"} / 1024 / 1024 / 1024 > 4
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Collector 内存使用过高"
      description: "Collector {{ $labels.instance }} 内存使用超过 4GB"

  - alert: OtelExporterSendFailed
    expr: |
      rate(otelcol_exporter_send_failed_spans[5m]) > 0
    for: 2m
    labels:
      severity: warning
    annotations:
      summary: "Collector Exporter 发送失败"
      description: "Exporter {{ $labels.exporter }} 发送 spans 失败"

  - alert: OtelReceiverRefused
    expr: |
      rate(otelcol_receiver_refused_spans[5m]) > 0
    for: 2m
    labels:
      severity: warning
    annotations:
      summary: "Collector 拒绝接收数据"
      description: "Receiver {{ $labels.receiver }} 正在拒绝 spans"

  - alert: OtelCollectorRestartLoop
    expr: |
      rate(kube_pod_container_status_restarts_total{container="otel-collector"}[10m]) > 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Collector 容器频繁重启"
      description: "Collector Pod 在过去 10 分钟内发生重启"
```

### 3.5 最佳实践

1. **分层部署**：应用集群部署 Agent Collector（无状态），中心集群部署 Gateway Collector（有状态 + 队列）
2. **采样策略**：开发环境 100% 采样，生产环境使用尾部采样保留错误和慢请求
3. **标签管控**：在 Collector 中统一添加 `cluster`、`namespace`、`environment` 等标签，禁止客户端随意添加高基数标签
4. **队列持久化**：对关键链路启用 `persistent_queue`，使用 `file_storage` extension 防止数据丢失
5. **证书管理**：使用 cert-manager 自动轮转 Collector 与后端之间的 mTLS 证书
6. **配置验证**：使用 `otelcol validate --config=...` 在 CI 中验证配置变更
7. **降级策略**：Exporter 配置多个后端，主后端失败时自动切换到备用 `debug` 或 `file` exporter

### 典型问题案例

#### 案例一：未配置 memory_limiter 导致生产 Collector 雪崩

**问题描述**：业务高峰期间 Collector Pod 连续 OOMKilled，重启后瞬间再次 OOM。

**根本原因**：未配置 `memory_limiter` processor，大量并发 trace 请求导致内存无限制增长。

**解决方案**：
1. 启用 `memory_limiter` processor，设置 `limit_mib` 为容器 limit 的 80%
2. 配置 HPA 基于内存使用率自动扩容
3. 在客户端启用批处理和压缩，减少单请求大小

#### 案例二：高基数 `user_id` 标签导致 Prometheus 卡死

**问题描述**：接入 OpenTelemetry metrics 后，Prometheus 查询超时，内存占用飙升到 64GB。

**根本原因**：应用 SDK 在每个 metric 上都添加了 `user_id` 标签，导致 cardinality 爆炸。

**解决方案**：
1. 在 Collector 的 `attributes` processor 中删除 `user_id` 标签
2. 修改应用 SDK，将用户级指标改为使用 Logs 或 Traces
3. 在 Prometheus 中配置 `metric_relabel_configs` 丢弃高基数标签

#### 案例三：跨集群 mTLS 证书过期导致数据中断

**问题描述**：Region B 的 Collector 无法将数据发送到 Region A 的 Jaeger，日志显示证书错误。

**根本原因**：Collector 与 Jaeger 之间的 mTLS 证书由自签 CA 签发，有效期 90 天，过期后未自动更新。

**解决方案**：
1. 使用 cert-manager 管理 Collector 的客户端证书和 Jaeger 的服务端证书
2. 配置 `cert-manager.io/inject-ca-from` annotation 自动注入 CA
3. 设置证书过期前 30 天的告警

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[log|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s|k8s]]
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/04-finops-cost-optimization-troubleshooting|04-finops-cost-optimization-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/01-monitoring-observability-troubleshooting|01-monitoring-observability-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/03-ebpf-observability-troubleshooting|03-ebpf-observability-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-monitoring-observability/04-finops-cost-optimization-troubleshooting|04-finops-cost-optimization-troubleshooting]]
