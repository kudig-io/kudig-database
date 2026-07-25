---
title: Node.js Microservices Patterns on Kubernetes
description: K8s 上 Node.js 微服务生产模式 — 优雅关闭、健康检查、连接池、错误处理、性能优化、分布式追踪
summary: Node.js 微服务在 Kubernetes 环境中的生产级设计模式与最佳实践
category: practice
tags:
- nodejs
- microservices
- graceful-shutdown
- health-check
- production
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: workload
---
# Node.js 微服务 Kubernetes 生产模式

> 面向生产环境的 Node.js 微服务设计模式与 K8s 集成最佳实践。

## 优雅关闭（Graceful Shutdown）

### 完整关闭流程

```javascript
// server.js — 生产级优雅关闭
const http = require('http');
const express = require('express');

const app = express();
const server = http.createServer(app);

let isShuttingDown = false;

// 中间件：拒绝新请求
app.use((req, res, next) => {
  if (isShuttingDown) {
    res.set('Connection', 'close');
    return res.status(503).json({ error: 'Server is shutting down' });
  }
  next();
});

// 健康检查端点
app.get('/healthz', (req, res) => {
  if (isShuttingDown) return res.status(503).json({ status: 'shutting_down' });
  res.json({ status: 'ok', uptime: process.uptime() });
});

app.get('/readyz', async (req, res) => {
  try {
    await checkDependencies(); // DB、Redis、MQ 连接检查
    if (isShuttingDown) return res.status(503).json({ status: 'not_ready' });
    res.json({ status: 'ready' });
  } catch (err) {
    res.status(503).json({ status: 'not_ready', error: err.message });
  }
});

server.listen(3000, () => console.log('Listening on :3000'));

// 优雅关闭处理
function gracefulShutdown(signal) {
  console.log(`Received ${signal}, starting graceful shutdown...`);
  isShuttingDown = true;

  // 1. 停止接受新连接
  server.close(async () => {
    console.log('HTTP server closed');
    try {
      // 2. 关闭数据库连接池
      await dbPool.end();
      // 3. 关闭 Redis 连接
      await redis.quit();
      // 4. 关闭消息队列消费者
      await mqConsumer.close();
      // 5. 刷新日志/指标缓冲
      await logger.flush();
      console.log('All connections closed, exiting');
      process.exit(0);
    } catch (err) {
      console.error('Error during shutdown:', err);
      process.exit(1);
    }
  });

  // 强制退出超时（应小于 K8s terminationGracePeriodSeconds）
  setTimeout(() => {
    console.error('Forced shutdown after timeout');
    process.exit(1);
  }, 25000); // 25s < 30s terminationGracePeriod
}

process.on('SIGTERM', () => gracefulShutdown('SIGTERM'));
process.on('SIGINT', () => gracefulShutdown('SIGINT'));

// 未捕获异常处理
process.on('uncaughtException', (err) => {
  console.error('Uncaught Exception:', err);
  gracefulShutdown('uncaughtException');
});

process.on('unhandledRejection', (reason) => {
  console.error('Unhandled Rejection:', reason);
  gracefulShutdown('unhandledRejection');
});
```

### K8s Deployment 配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: node-api
  namespace: production
spec:
  replicas: 3
  template:
    spec:
      terminationGracePeriodSeconds: 30
      containers:
        - name: api
          image: registry.example.com/node-api:v2.1.0
          ports:
            - containerPort: 3000
          lifecycle:
            preStop:
              exec:
                command: ["sh", "-c", "sleep 5"]  # 等待 Service 摘除 Endpoints
          readinessProbe:
            httpGet:
              path: /readyz
              port: 3000
            initialDelaySeconds: 5
            periodSeconds: 5
            failureThreshold: 3
          livenessProbe:
            httpGet:
              path: /healthz
              port: 3000
            initialDelaySeconds: 15
            periodSeconds: 10
            failureThreshold: 3
          startupProbe:
            httpGet:
              path: /healthz
              port: 3000
            initialDelaySeconds: 5
            periodSeconds: 5
            failureThreshold: 30  # 最多等 150s 启动
          resources:
            requests:
              cpu: 200m
              memory: 256Mi
            limits:
              cpu: "1"
              memory: 512Mi
          env:
            - name: NODE_ENV
              value: production
            - name: UV_THREADPOOL_SIZE
              value: "8"
```

## 连接池管理

### PostgreSQL（pg-pool）

```javascript
const { Pool } = require('pg');

const pool = new Pool({
  host: process.env.DB_HOST,
  port: 5432,
  database: process.env.DB_NAME,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
  max: 20,                    // 最大连接数（Pod 数 × max < DB max_connections）
  idleTimeoutMillis: 30000,
  connectionTimeoutMillis: 5000,
  statement_timeout: 30000,
  query_timeout: 30000,
});

pool.on('error', (err) => {
  console.error('Unexpected pool error:', err);
});

// 带超时的查询封装
async function query(text, params, timeout = 10000) {
  const client = await pool.connect();
  try {
    await client.query(`SET statement_timeout = ${timeout}`);
    return await client.query(text, params);
  } finally {
    client.release();
  }
}
```

### HTTP 客户端（连接复用）

```javascript
const http = require('http');
const https = require('https');

// 全局 Agent 复用连接
const httpAgent = new http.Agent({
  keepAlive: true,
  maxSockets: 100,
  maxFreeSockets: 20,
  timeout: 30000,
});

const httpsAgent = new https.Agent({
  keepAlive: true,
  maxSockets: 100,
  maxFreeSockets: 20,
  timeout: 30000,
});

// axios 配置
const axios = require('axios');
const client = axios.create({
  httpAgent,
  httpsAgent,
  timeout: 10000,
  headers: { 'User-Agent': 'node-api/2.1.0' },
});

// 重试拦截器
client.interceptors.response.use(null, async (error) => {
  const config = error.config;
  if (!config || config._retryCount >= 3) throw error;
  
  const status = error.response?.status;
  const retryable = [429, 500, 502, 503, 504];
  if (!retryable.includes(status)) throw error;
  
  config._retryCount = (config._retryCount || 0) + 1;
  const delay = Math.min(1000 * 2 ** config._retryCount, 10000);
  await new Promise(r => setTimeout(r, delay));
  return client(config);
});
```

## 分布式追踪（OpenTelemetry）

```javascript
// tracing.js — 应用启动前加载
const { NodeSDK } = require('@opentelemetry/sdk-node');
const { getNodeAutoInstrumentations } = require('@opentelemetry/auto-instrumentations-node');
const { OTLPTraceExporter } = require('@opentelemetry/exporter-trace-otlp-http');
const { Resource } = require('@opentelemetry/resources');
const {
  ATTR_SERVICE_NAME,
  ATTR_SERVICE_VERSION,
  ATTR_DEPLOYMENT_ENVIRONMENT_NAME,
} = require('@opentelemetry/semantic-conventions');

const sdk = new NodeSDK({
  resource: new Resource({
    [ATTR_SERVICE_NAME]: process.env.SERVICE_NAME || 'node-api',
    [ATTR_SERVICE_VERSION]: process.env.APP_VERSION || '2.1.0',
    [ATTR_DEPLOYMENT_ENVIRONMENT_NAME]: process.env.NODE_ENV || 'production',
  }),
  traceExporter: new OTLPTraceExporter({
    url: process.env.OTEL_EXPORTER_OTLP_ENDPOINT || 'http://otel-collector:4318/v1/traces',
  }),
  instrumentations: [
    getNodeAutoInstrumentations({
      '@opentelemetry/instrumentation-http': { enabled: true },
      '@opentelemetry/instrumentation-express': { enabled: true },
      '@opentelemetry/instrumentation-pg': { enabled: true },
      '@opentelemetry/instrumentation-ioredis': { enabled: true },
      '@opentelemetry/instrumentation-kafkajs': { enabled: true },
    }),
  ],
});

sdk.start();
process.on('SIGTERM', () => sdk.shutdown().then(() => process.exit(0)));
```

## 错误处理模式

### 统一错误中间件

```javascript
// 操作错误 vs 程序员错误
class AppError extends Error {
  constructor(message, statusCode, code, isOperational = true) {
    super(message);
    this.statusCode = statusCode;
    this.code = code;
    this.isOperational = isOperational;
    Error.captureStackTrace(this, this.constructor);
  }
}

// Express 错误处理
app.use((err, req, res, next) => {
  const { statusCode = 500, code = 'INTERNAL_ERROR', isOperational } = err;
  
  // 记录日志（含 traceId）
  logger.error({
    message: err.message,
    code,
    statusCode,
    stack: isOperational ? undefined : err.stack,
    path: req.path,
    method: req.method,
    traceId: req.headers['x-trace-id'],
  });

  // 不向客户端暴露内部错误细节
  res.status(statusCode).json({
    error: {
      code,
      message: isOperational ? err.message : 'Internal server error',
    },
  });
});
```

## 性能优化清单

| 优化项 | 方法 | 预期收益 |
|--------|------|----------|
| 事件循环延迟 | `--max-old-space-size=384` 匹配内存限制 | 避免 OOM |
| 连接复用 | HTTP Agent keepAlive | 减少 50% 延迟 |
| 压缩响应 | `compression` 中间件 | 减少 60-80% 带宽 |
| JSON 序列化 | `fast-json-stringify` / `orjson` | 2-5x 序列化速度 |
| 日志 | pino（异步）替代 console.log | 5x 日志吞吐 |
| 集群模式 | 单 Pod 单进程（K8s 管理副本） | 简化运维 |
| 内存泄漏 | `--heap-prof` + clinic.js 定期分析 | 预防 OOM |

```yaml
# 生产启动命令
command: ["node"]
args:
  - "--max-old-space-size=384"
  - "--enable-source-maps"
  - "--heapsnapshot-near-heap-limit=3"
  - "server.js"
```

## 反模式

| 反模式 | 问题 | 正确做法 |
|--------|------|----------|
| cluster 模块 | 与 K8s HPA 冲突 | 单进程 + 多 Pod |
| 同步文件 I/O | 阻塞事件循环 | 异步 I/O 或 worker_threads |
| 无连接池 | 每次请求新建连接 | 全局 Pool/Agent |
| console.log | 同步写入阻塞 | pino 异步日志 |
| 捕获后忽略错误 | 静默失败 | 记录 + 上报 + 优雅降级 |
| 无 preStop hook | 滚动更新时请求丢失 | sleep 5 + 503 拒绝 |

## Related

- [[02-工作负载/03-Node-js-on-K8s/index.md|Node.js on K8s]]
- [[02-工作负载/03-Node-js-on-K8s/02-nodejs-observability-performance.md|Node.js 可观测性]]
- [[09-可观测性/04-链路追踪/index.md|分布式追踪]]
