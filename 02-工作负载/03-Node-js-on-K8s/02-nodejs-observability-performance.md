---
title: Node.js 可观测性与性能调优
summary: Node.js 在 Kubernetes 上的可观测性集成（OpenTelemetry/Prometheus）和性能调优（Event Loop/GC/Profiling），
  涵盖指标暴露、链路追踪、日志规范、性能诊断工具链。
category: domain
tags:
- nodejs
- observability
- performance
- opentelemetry
- prometheus
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: workloads-applications
---

# Node.js 可观测性与性能调优

## 概述

Node.js 的单线程事件循环模型使性能瓶颈集中在 Event Loop 阻塞和内存泄漏两个方面。在 Kubernetes 环境中，需要将 Node.js 运行时指标与 K8s 基础设施指标关联，形成完整的可观测性链路。

## 三大支柱集成

### 指标 (Metrics) — Prometheus 暴露

```javascript
const client = require('prom-client');

// 默认指标（CPU、内存、Event Loop）
const collectDefaultMetrics = client.collectDefaultMetrics;
collectDefaultMetrics({ prefix: 'nodejs_' });

// 自定义业务指标
const httpRequestDuration = new client.Histogram({
  name: 'http_request_duration_seconds',
  help: 'HTTP request duration in seconds',
  labelNames: ['method', 'route', 'status_code'],
  buckets: [0.01, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5]
});

const activeConnections = new client.Gauge({
  name: 'nodejs_active_connections',
  help: 'Number of active connections'
});

// Event Loop Lag（关键指标）
const eventLoopLag = new client.Gauge({
  name: 'nodejs_event_loop_lag_seconds',
  help: 'Event loop lag in seconds'
});

// 定期采集 Event Loop Lag
setInterval(() => {
  const start = process.hrtime.bigint();
  setImmediate(() => {
    const lag = Number(process.hrtime.bigint() - start) / 1e9;
    eventLoopLag.set(lag);
  });
}, 1000);

// /metrics 端点
app.get('/metrics', async (req, res) => {
  res.set('Content-Type', client.register.contentType);
  res.end(await client.register.metrics());
});
```

### 链路追踪 (Tracing) — OpenTelemetry

```javascript
const { NodeSDK } = require('@opentelemetry/sdk-node');
const { getNodeAutoInstrumentations } = require('@opentelemetry/auto-instrumentations-node');
const { OTLPTraceExporter } = require('@opentelemetry/exporter-trace-otlp-http');

const sdk = new NodeSDK({
  traceExporter: new OTLPTraceExporter({
    url: process.env.OTEL_EXPORTER_OTLP_ENDPOINT || 'http://otel-collector:4318/v1/traces'
  }),
  instrumentations: [
    getNodeAutoInstrumentations({
      '@opentelemetry/instrumentation-http': { enabled: true },
      '@opentelemetry/instrumentation-express': { enabled: true },
      '@opentelemetry/instrumentation-pg': { enabled: true },
      '@opentelemetry/instrumentation-redis': { enabled: true },
    })
  ],
  serviceName: 'nodejs-api'
});

sdk.start();
```

### 日志 (Logging) — 结构化 JSON

```javascript
const pino = require('pino');
const logger = pino({
  level: process.env.LOG_LEVEL || 'info',
  formatters: {
    level: (label) => ({ level: label })
  },
  timestamp: pino.stdTimeFunctions.isoTime,
  // K8s 环境自动关联 Pod 信息
  mixin: () => ({
    service: 'nodejs-api',
    version: process.env.APP_VERSION,
    pod: process.env.HOSTNAME  // K8s 注入的 Pod 名
  })
});
```

## 关键性能指标

### 必须监控的 Node.js 指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| `nodejs_event_loop_lag_seconds` | 事件循环延迟 | > 100ms 警告, > 500ms 严重 |
| `nodejs_heap_used_bytes` | V8 堆使用量 | > 80% max-old-space-size |
| `nodejs_heap_size_total_bytes` | V8 堆总量 | 持续增长（泄漏信号） |
| `nodejs_active_handles_total` | 活跃句柄数 | 持续增长 |
| `nodejs_active_requests_total` | 活跃请求数 | 突增 |
| `process_resident_memory_bytes` | RSS 内存 | > 容器 limit × 0.9 |
| `http_request_duration_seconds` | 请求延迟 P99 | > 1s |
| `nodejs_gc_duration_seconds` | GC 暂停时间 | > 100ms |

### Grafana Dashboard 关键面板

```
Row 1: Event Loop Lag | Active Connections | Request Rate
Row 2: Heap Used/Total | RSS Memory | GC Duration
Row 3: P50/P95/P99 Latency | Error Rate | Throughput
Row 4: CPU Usage | File Descriptors | Active Handles
```

## 性能调优

### Event Loop 阻塞诊断

```bash
# 使用 clinic.js 诊断
npx clinic doctor -- node server.js
npx clinic flame -- node server.js
npx clinic bubbleprof -- node server.js

# 生产环境：0x 火焰图
npx 0x -- node server.js
```

### V8 GC 调优

```bash
# 查看 GC 日志
NODE_OPTIONS="--trace-gc --max-old-space-size=384" node server.js

# GC 日志输出示例：
# [44:0x7f8b8c000000] 1234 ms: Scavenge 128.5 (160.0) -> 112.3 (160.0) MB, 2.1 ms
```

| GC 类型 | 触发条件 | 暂停时间 | 优化方向 |
|---------|----------|----------|----------|
| Scavenge (Minor) | 新生代满 | 1-10ms | 减少短生命周期对象 |
| Mark-Sweep (Major) | 老生代满 | 50-500ms | 避免大对象频繁晋升 |
| Incremental Marking | 老生代接近满 | 分片 < 10ms | V8 默认启用 |

### 常见性能问题与解决

| 问题 | 症状 | 解决方案 |
|------|------|----------|
| 同步 I/O | Event Loop Lag 飙升 | 改用 async API |
| 大 JSON 序列化 | CPU 100% | 流式处理 / worker_threads |
| 内存泄漏 | RSS 持续增长 | heapdump + 对比分析 |
| 连接池耗尽 | 请求排队 | 增大池 / 修复未释放 |
| 正则回溯 | 特定输入卡死 | 避免嵌套量词 |

## 生产监控配置

### ServiceMonitor (Prometheus Operator)

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: nodejs-api
  labels:
    release: prometheus
spec:
  selector:
    matchLabels:
      app: nodejs-api
  endpoints:
  - port: http
    path: /metrics
    interval: 15s
    scrapeTimeout: 10s
```

### 告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: nodejs-alerts
spec:
  groups:
  - name: nodejs.rules
    rules:
    - alert: NodeJSEventLoopLagHigh
      expr: nodejs_event_loop_lag_seconds > 0.1
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "Node.js Event Loop 延迟过高"
    - alert: NodeJSMemoryLeak
      expr: |
        increase(process_resident_memory_bytes{job="nodejs-api"}[1h]) > 100*1024*1024
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "Node.js 疑似内存泄漏（1h 增长 > 100MB）"
    - alert: NodeJSHeapNearLimit
      expr: |
        nodejs_heap_used_bytes / nodejs_heap_size_total_bytes > 0.9
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Node.js V8 堆接近上限，可能 OOM"
```

## 最佳实践

| 实践 | 说明 |
|------|------|
| 启动时初始化 SDK | OpenTelemetry 在应用入口最早初始化 |
| 结构化日志 | JSON 格式，便于 Loki/ES 解析 |
| 关联 Trace ID | 日志中注入 traceId，实现日志-链路关联 |
| 自定义指标命名 | 遵循 Prometheus 命名规范 |
| Event Loop 监控 | 必须监控，是 Node.js 健康度的核心指标 |
| 定期 heapdump | 生产环境定时采集（低峰期），用于趋势分析 |

## 内存泄漏诊断实战

### 内存泄漏检测流程

```
1. 确认泄漏
   ├── RSS 持续增长（不回落）
   ├── heapUsed 持续增长
   └── GC 后内存不释放

2. 采集 Heap Snapshot
   ├── 生产环境：定时采集（低峰期）
   ├── 测试环境：手动触发
   └── 至少采集 3 次（间隔 5-10 分钟）

3. 对比分析
   ├── 使用 Chrome DevTools 打开 .heapsnapshot
   ├── 对比两次快照的 Delta
   └── 查找持续增长的对象

4. 定位根因
   ├── 全局变量/缓存未清理
   ├── 事件监听器未移除
   ├── 闭包引用
   └── 定时器未清除
```

### 生产环境 Heap Snapshot 采集

```javascript
const v8 = require('v8');
const fs = require('fs');
const path = require('path');

// 定时采集 Heap Snapshot（低峰期）
function takeHeapSnapshot() {
  const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
  const filename = `/tmp/heapsnapshot/heap-${timestamp}.heapsnapshot`;
  
  // 确保目录存在
  fs.mkdirSync('/tmp/heapsnapshot', { recursive: true });
  
  const snapshotStream = v8.writeHeapSnapshot(filename);
  console.log(`Heap snapshot written to: ${snapshotStream}`);
  
  // 上传到 S3/OSS（可选）
  // uploadToS3(filename);
  
  return filename;
}

// 每天凌晨 3 点采集
const cron = require('node-cron');
cron.schedule('0 3 * * *', () => {
  console.log('Taking scheduled heap snapshot...');
  takeHeapSnapshot();
});

// 手动触发端点（仅内网访问）
app.post('/debug/heap-snapshot', (req, res) => {
  const filename = takeHeapSnapshot();
  res.json({ message: 'Heap snapshot taken', file: filename });
});
```

### 常见内存泄漏模式

| 泄漏模式 | 示例 | 修复方法 |
|----------|------|----------|
| 全局缓存无限增长 | `const cache = {}` 无过期 | 使用 LRU Cache + TTL |
| 事件监听器未移除 | `emitter.on()` 无 `off()` | 组件销毁时 `removeListener` |
| 闭包引用大对象 | 函数闭包引用外部大数组 | 及时置 null |
| 定时器未清除 | `setInterval()` 无 `clearInterval()` | 组件销毁时清除 |
| 未处理的 Promise | `Promise` 链未 catch | 添加 `.catch()` |
| Stream 未关闭 | HTTP 响应未 end | 确保 `res.end()` |

## CPU Profiling

### 生产环境 CPU 采样

```javascript
const inspector = require('inspector');
const fs = require('fs');

// CPU Profile 采集
async function takeCpuProfile(durationMs = 30000) {
  const session = new inspector.Session();
  session.connect();
  
  await new Promise((resolve) => {
    session.post('Profiler.enable', resolve);
  });
  
  await new Promise((resolve) => {
    session.post('Profiler.start', resolve);
  });
  
  // 采集指定时长
  await new Promise(resolve => setTimeout(resolve, durationMs));
  
  const { profile } = await new Promise((resolve) => {
    session.post('Profiler.stop', (err, { profile }) => resolve({ profile }));
  });
  
  // 保存 profile
  const filename = `/tmp/cpu-profile-${Date.now()}.cpuprofile`;
  fs.writeFileSync(filename, JSON.stringify(profile));
  console.log(`CPU profile saved to: ${filename}`);
  
  session.disconnect();
  return filename;
}

// 触发端点
app.post('/debug/cpu-profile', async (req, res) => {
  const duration = parseInt(req.query.duration) || 30000;
  const filename = await takeCpuProfile(duration);
  res.json({ message: 'CPU profile taken', file: filename });
});
```

### 使用 clinic.js 诊断

```bash
# 本地诊断
npx clinic doctor -- node server.js
npx clinic flame -- node server.js
npx clinic bubbleprof -- node server.js

# 生产环境（需要进入容器）
kubectl exec -it <pod> -- sh
npx clinic doctor -- node dist/server.js
```

## 分布式追踪最佳实践

### Trace 上下文传播

```javascript
const { context, trace } = require('@opentelemetry/api');

// 手动创建 Span
async function processOrder(orderId) {
  const tracer = trace.getTracer('order-service');
  
  return tracer.startActiveSpan('process-order', async (span) => {
    try {
      span.setAttribute('order.id', orderId);
      span.setAttribute('order.status', 'processing');
      
      // 子操作自动继承 Trace 上下文
      await validateOrder(orderId);
      await chargePayment(orderId);
      await shipOrder(orderId);
      
      span.setStatus({ code: SpanStatusCode.OK });
    } catch (error) {
      span.setStatus({ code: SpanStatusCode.ERROR, message: error.message });
      span.recordException(error);
      throw error;
    } finally {
      span.end();
    }
  });
}

// 日志关联 Trace ID
const { context } = require('@opentelemetry/api');

function logWithTrace(message, data = {}) {
  const span = trace.getActiveSpan();
  const traceId = span?.spanContext().traceId;
  
  logger.info({
    message,
    ...data,
    traceId,  // 关联 Trace
    spanId: span?.spanContext().spanId,
  });
}
```

### 采样策略配置

```javascript
const { NodeSDK } = require('@opentelemetry/sdk-node');
const { TraceIdRatioBasedSampler } = require('@opentelemetry/sdk-trace-base');

const sdk = new NodeSDK({
  // 生产环境采样率 10%
  sampler: new TraceIdRatioBasedSampler(0.1),
  
  // 或者使用父级采样（继承上游决策）
  // sampler: new ParentBasedSampler({
  //   root: new TraceIdRatioBasedSampler(0.1)
  // }),
});
```

## 日志聚合配置

### Fluentd 采集配置

```yaml
# Fluentd ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluentd-config
data:
  fluent.conf: |
    <source>
      @type tail
      path /var/log/containers/*nodejs*.log
      pos_file /var/log/fluentd-containers.log.pos
      tag kubernetes.*
      format json
      time_key time
      time_format %Y-%m-%dT%H:%M:%S.%NZ
    </source>
    
    <filter kubernetes.**>
      @type kubernetes_metadata
    </filter>
    
    <match kubernetes.**>
      @type elasticsearch
      host elasticsearch.logging.svc
      port 9200
      logstash_format true
      logstash_prefix nodejs
      include_tag_key true
    </match>
```

### 日志查询示例（Loki）

```logql
# 查询特定服务的错误日志
{app="nodejs-api", level="error"} |= "error"

# 查询特定 Trace ID 的日志
{app="nodejs-api"} |= "traceId=abc123"

# 统计每分钟错误数
sum(rate({app="nodejs-api", level="error"}[1m]))

# 查询 P99 延迟日志
{app="nodejs-api"} | json | duration > 1000
```

## 性能基准测试

### 负载测试脚本（k6）

```javascript
// k6-load-test.js
import http from 'k6/http';
import { check, sleep } from 'k6';
import { Rate, Trend } from 'k6/metrics';

// 自定义指标
const errorRate = new Rate('errors');
const apiLatency = new Trend('api_latency');

export const options = {
  stages: [
    { duration: '2m', target: 100 },   // 预热到 100 VU
    { duration: '5m', target: 100 },   // 保持 100 VU
    { duration: '2m', target: 200 },   // 加压到 200 VU
    { duration: '5m', target: 200 },   // 保持 200 VU
    { duration: '2m', target: 0 },     // 降压
  ],
  thresholds: {
    http_req_duration: ['p(95)<500', 'p(99)<1000'],  // P95 < 500ms
    errors: ['rate<0.01'],  // 错误率 < 1%
  },
};

export default function () {
  const res = http.get('http://nodejs-api:3000/api/health');
  
  check(res, {
    'status is 200': (r) => r.status === 200,
    'response time < 200ms': (r) => r.timings.duration < 200,
  });
  
  errorRate.add(res.status !== 200);
  apiLatency.add(res.timings.duration);
  
  sleep(1);
}
```

### 性能基线表

| 场景 | QPS | P50 延迟 | P99 延迟 | CPU 使用 | 内存使用 |
|------|-----|----------|----------|----------|----------|
| 健康检查 | 1000 | < 5ms | < 20ms | 10% | 100MB |
| 简单 API | 500 | < 20ms | < 100ms | 30% | 150MB |
| 数据库查询 | 200 | < 50ms | < 200ms | 50% | 200MB |
| 复杂计算 | 50 | < 200ms | < 500ms | 80% | 250MB |

## Related

- [[02-工作负载/03-Node-js-on-K8s/01-nodejs-production-kubernetes.md|Node.js 生产部署]]
- [[09-可观测性/README.md|可观测性知识域]]
- [[27-标签/observability|observability 标签枢纽]]
- [[24-综合/05-可观测性/opentelemetry-prometheus.md|OpenTelemetry × Prometheus]]
