---
title: Node.js 生产级 Kubernetes 部署
summary: Node.js 在 Kubernetes 上的生产级部署实践，涵盖 V8 内存管理与容器 limit 对齐、优雅关闭、健康检查设计、集群模式选择、安全加固。
category: domain
tags:
- nodejs
- workloads
- production
- kubernetes
- javascript
- performance
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: intermediate
domain: workloads-applications
---

# Node.js 生产级 Kubernetes 部署

## 概述与核心概念

Node.js 基于 V8 引擎的单线程事件循环模型与 Kubernetes 的容器化部署模型存在独特的交互挑战。核心问题在于：V8 的堆内存管理（GC）与容器的 cgroup memory limit 必须精确对齐，否则会导致 OOMKilled 或内存浪费。

### 核心架构原则

| 原则 | 说明 |
|------|------|
| 单进程模型 | 容器内运行单个 Node.js 进程，水平扩展交给 K8s |
| 内存对齐 | `--max-old-space-size` < 容器 memory limit × 0.75 |
| 优雅关闭 | 处理 SIGTERM，排空连接，配合 preStop hook |
| 无状态设计 | 会话外置（Redis），文件存储外置（S3/PVC） |
| 健康检查 | 区分 liveness（进程存活）和 readiness（可接收流量） |

## 内存管理：V8 与容器对齐

### 问题本质

```
容器 memory limit = V8 Heap + V8 Off-heap + Libuv buffers + Native addons + OS overhead
```

V8 默认堆大小约为系统内存的 1.5-2GB（64位），如果不设置 `--max-old-space-size`，在 512MB 容器中会直接被 OOMKilled。

### 生产配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nodejs-api
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: app
        image: nodejs-api:1.2.3
        resources:
          requests:
            cpu: "250m"
            memory: "256Mi"
          limits:
            cpu: "1000m"
            memory: "512Mi"
        env:
        - name: NODE_OPTIONS
          value: "--max-old-space-size=384"  # 512Mi × 0.75
        - name: UV_THREADPOOL_SIZE
          value: "4"  # 默认4，I/O密集可增至8
```

### 内存计算公式

| 容器 Limit | 推荐 max-old-space-size | 预留空间 |
|-----------|------------------------|----------|
| 256Mi | 192MB | 64MB (off-heap + buffers) |
| 512Mi | 384MB | 128MB |
| 1Gi | 768MB | 256MB |
| 2Gi | 1536MB | 512MB |

## 优雅关闭

### 完整关闭流程

```javascript
const http = require('http');
const server = http.createServer(app);

let isShuttingDown = false;

// 健康检查感知关闭状态
app.get('/healthz', (req, res) => {
  if (isShuttingDown) {
    res.status(503).json({ status: 'shutting_down' });
  } else {
    res.status(200).json({ status: 'ok' });
  }
});

// 优雅关闭
process.on('SIGTERM', async () => {
  console.log('SIGTERM received, starting graceful shutdown');
  isShuttingDown = true;

  // 1. 停止接受新连接
  server.close(() => {
    console.log('HTTP server closed');
  });

  // 2. 等待现有请求完成（超时强制退出）
  const forceTimeout = setTimeout(() => {
    console.error('Forced shutdown after timeout');
    process.exit(1);
  }, 10000); // 10s 超时

  // 3. 关闭数据库连接池
  await db.pool.end();

  // 4. 关闭消息队列连接
  await mq.disconnect();

  clearTimeout(forceTimeout);
  process.exit(0);
});
```

### K8s 配合配置

```yaml
spec:
  terminationGracePeriodSeconds: 30
  containers:
  - name: app
    lifecycle:
      preStop:
        exec:
          command: ["sh", "-c", "sleep 5"]  # 等待 Service 端点更新
    livenessProbe:
      httpGet:
        path: /healthz
        port: 3000
      initialDelaySeconds: 10
      periodSeconds: 10
    readinessProbe:
      httpGet:
        path: /ready
        port: 3000
      initialDelaySeconds: 5
      periodSeconds: 5
    startupProbe:
      httpGet:
        path: /healthz
        port: 3000
      failureThreshold: 30
      periodSeconds: 2
```

## 集群模式选择

| 模式 | 适用场景 | 优势 | 劣势 |
|------|----------|------|------|
| 单进程 + HPA | 生产推荐 | 简单、K8s 原生扩展 | 单核利用 |
| node:cluster | CPU 密集 | 多核利用 | 与 K8s 扩展冲突 |
| PM2 cluster | 遗留迁移 | 进程管理 | 容器内冗余 |
| Worker Threads | CPU 密集子任务 | 不阻塞事件循环 | 复杂度高 |

**生产推荐**：单进程模式 + K8s HPA 水平扩展。每个 Pod 使用一个 CPU 核心，通过 HPA 根据 CPU/自定义指标扩展副本数。

## 安全加固

### 生产 Dockerfile

```dockerfile
FROM node:20-alpine AS base
RUN apk add --no-cache dumb-init

# 非 root 用户
RUN addgroup -g 1001 -S nodejs && \
    adduser -S nodejs -u 1001

WORKDIR /app
COPY package*.json ./
RUN npm ci --only=production && npm cache clean --force

COPY --chown=nodejs:nodejs . .
USER nodejs

EXPOSE 3000
ENTRYPOINT ["dumb-init", "--"]
CMD ["node", "server.js"]
```

### 安全检查清单

- [ ] 非 root 用户运行
- [ ] 只读文件系统 (`readOnlyRootFilesystem: true`)
- [ ] 无特权容器
- [ ] 最小化基础镜像 (alpine/distroless)
- [ ] 依赖漏洞扫描 (npm audit / Snyk)
- [ ] 网络策略限制出入流量
- [ ] Secret 通过 K8s Secret 或 Vault 注入

## 故障排查

### 常见问题速查

| 症状 | 可能原因 | 诊断命令 |
|------|----------|----------|
| OOMKilled | V8 堆超限 | `kubectl describe pod` 查看 Last State |
| CrashLoopBackOff | 启动失败/端口冲突 | `kubectl logs --previous` |
| 高延迟 | Event Loop 阻塞 | 检查 Event Loop Lag 指标 |
| 内存泄漏 | 未释放引用 | `node --inspect` + heapdump |
| 连接耗尽 | 未正确关闭连接池 | 检查 fd 数量 `ls /proc/PID/fd` |

### 关键诊断命令

```bash
# 查看 Pod 内存使用
kubectl top pod -l app=nodejs-api

# 查看容器内进程
kubectl exec -it <pod> -- ps aux

# 查看 V8 堆统计
kubectl exec -it <pod> -- node -e "console.log(process.memoryUsage())"

# 查看 Event Loop 延迟（需暴露指标）
curl http://<pod-ip>:3000/metrics | grep event_loop
```

## 最佳实践与反模式

| 最佳实践 | 反模式 |
|----------|--------|
| 单进程 + HPA 扩展 | 容器内 PM2 cluster 模式 |
| `--max-old-space-size` 对齐 limit | 不设置 V8 堆限制 |
| preStop sleep + SIGTERM 处理 | 直接 kill -9 |
| 区分 liveness/readiness | 同一个 endpoint 做所有检查 |
| 结构化日志 (JSON) | console.log 纯文本 |
| 连接池复用 | 每次请求新建连接 |

## HPA 自动伸缩配置

### 基于 CPU/内存的 HPA

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: nodejs-api-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: nodejs-api
  minReplicas: 3
  maxReplicas: 20
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: 70  # CPU 使用率 70% 时扩容
    - type: Resource
      resource:
        name: memory
        target:
          type: Utilization
          averageUtilization: 80
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60  # 扩容稳定窗口
      policies:
        - type: Pods
          value: 2
          periodSeconds: 60  # 每分钟最多扩 2 个 Pod
    scaleDown:
      stabilizationWindowSeconds: 300  # 缩容稳定窗口 5 分钟
      policies:
        - type: Pods
          value: 1
          periodSeconds: 120  # 每 2 分钟最多缩 1 个 Pod
```

### 基于自定义指标的 HPA（Event Loop Lag）

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: nodejs-api-custom-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: nodejs-api
  minReplicas: 3
  maxReplicas: 20
  metrics:
    # 基于 Event Loop Lag 扩容（Node.js 特有）
    - type: Pods
      pods:
        metric:
          name: nodejs_event_loop_lag_seconds
        target:
          type: AverageValue
          averageValue: "0.1"  # 平均 Event Loop Lag > 100ms 时扩容
    # 基于活跃连接数
    - type: Pods
      pods:
        metric:
          name: nodejs_active_connections
        target:
          type: AverageValue
          averageValue: "100"  # 每 Pod 平均连接数 > 100 时扩容
```

## 多阶段构建优化

### 生产级 Dockerfile（完整）

```dockerfile
# 阶段 1: 依赖安装
FROM node:20-alpine AS deps
WORKDIR /app
COPY package*.json ./
RUN npm ci --only=production && npm cache clean --force

# 阶段 2: 构建（如有 TypeScript）
FROM node:20-alpine AS builder
WORKDIR /app
COPY package*.json ./
RUN npm ci
COPY . .
RUN npm run build

# 阶段 3: 生产镜像
FROM node:20-alpine AS runner

# 安装 dumb-init 处理 PID 1 信号
RUN apk add --no-cache dumb-init tini

# 安全加固：创建非 root 用户
ENV NODE_ENV=production
RUN addgroup -g 1001 -S nodejs && \
    adduser -S nodejs -u 1001

WORKDIR /app

# 复制生产依赖
COPY --from=deps /app/node_modules ./node_modules

# 复制构建产物
COPY --from=builder /app/dist ./dist
COPY --from=builder /app/package*.json ./

# 设置权限
RUN chown -R nodejs:nodejs /app
USER nodejs

# 环境变量
ENV PORT=3000
ENV NODE_OPTIONS="--max-old-space-size=384"

EXPOSE 3000

# 使用 dumb-init 作为入口点
ENTRYPOINT ["dumb-init", "--"]
CMD ["node", "dist/server.js"]
```

### 镜像大小对比

| 基础镜像 | 大小 | 适用场景 |
|----------|------|----------|
| node:20 | ~1GB | 开发/调试 |
| node:20-slim | ~200MB | 通用生产 |
| node:20-alpine | ~180MB | 生产推荐 |
| gcr.io/distroless/nodejs20 | ~120MB | 高安全要求 |

## 配置管理

### ConfigMap + Secret 注入

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: nodejs-api-config
data:
  NODE_ENV: "production"
  LOG_LEVEL: "info"
  PORT: "3000"
  UV_THREADPOOL_SIZE: "4"
---
apiVersion: v1
kind: Secret
metadata:
  name: nodejs-api-secrets
type: Opaque
stringData:
  DATABASE_URL: "postgresql://user:pass@postgres:5432/db"
  REDIS_URL: "redis://redis:6379"
  JWT_SECRET: "your-secret-key"
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nodejs-api
spec:
  template:
    spec:
      containers:
        - name: app
          envFrom:
            - configMapRef:
                name: nodejs-api-config
            - secretRef:
                name: nodejs-api-secrets
          # 或者单独注入
          env:
            - name: POD_NAME
              valueFrom:
                fieldRef:
                  fieldPath: metadata.name
            - name: POD_NAMESPACE
              valueFrom:
                fieldRef:
                  fieldPath: metadata.namespace
            - name: NODE_IP
              valueFrom:
                fieldRef:
                  fieldPath: status.hostIP
```

### 配置热加载

```javascript
// 监听 ConfigMap 变更（通过文件挂载）
const fs = require('fs');
const path = '/etc/config/app-config.json';

let config = JSON.parse(fs.readFileSync(path, 'utf8'));

// 监听文件变更
fs.watchFile(path, { interval: 5000 }, () => {
  console.log('Config change detected, reloading...');
  config = JSON.parse(fs.readFileSync(path, 'utf8'));
  // 应用新配置
  applyConfig(config);
});
```

## 依赖服务连接池

### 数据库连接池配置

```javascript
const { Pool } = require('pg');

const pool = new Pool({
  host: process.env.DB_HOST || 'postgres',
  port: parseInt(process.env.DB_PORT) || 5432,
  database: process.env.DB_NAME || 'app',
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
  
  // 连接池配置
  max: 20,                    // 最大连接数
  idleTimeoutMillis: 30000,   // 空闲连接超时
  connectionTimeoutMillis: 5000,  // 连接超时
  
  // 健康检查
  allowExitOnIdle: true,
});

// 连接池监控
pool.on('connect', () => {
  console.log('New DB connection established');
});

pool.on('error', (err) => {
  console.error('Unexpected DB pool error', err);
  // 触发告警
});

// 优雅关闭
async function closePool() {
  await pool.end();
  console.log('DB pool closed');
}

module.exports = { pool, closePool };
```

### Redis 连接配置

```javascript
const Redis = require('ioredis');

const redis = new Redis({
  host: process.env.REDIS_HOST || 'redis',
  port: parseInt(process.env.REDIS_PORT) || 6379,
  
  // 重连策略
  retryStrategy: (times) => {
    const delay = Math.min(times * 100, 3000);
    return delay;
  },
  
  // 连接池
  maxRetriesPerRequest: 3,
  enableReadyCheck: true,
  
  // 哨兵模式（高可用）
  // sentinels: [
  //   { host: 'sentinel-1', port: 26379 },
  //   { host: 'sentinel-2', port: 26379 },
  // ],
  // name: 'mymaster',
});

redis.on('error', (err) => {
  console.error('Redis error:', err);
});

module.exports = redis;
```

## 生产部署检查清单

### 上线前检查

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| 镜像标签 | `kubectl get deploy -o jsonpath='{.spec.template.spec.containers[0].image}'` | 非 latest |
| 资源限制 | `kubectl get deploy -o yaml \| grep -A5 resources` | 已配置 |
| 健康检查 | `kubectl get deploy -o yaml \| grep -A10 Probe` | 三种探针 |
| PDB 配置 | `kubectl get pdb` | 已配置 |
| HPA 配置 | `kubectl get hpa` | 已配置 |
| 网络策略 | `kubectl get networkpolicy` | 已配置 |
| Secret 管理 | `kubectl get externalsecrets` | 非明文 |
| 日志格式 | 检查应用日志 | JSON 结构化 |

### 部署验证脚本

```bash
#!/bin/bash
# 🟢 低风险：只读检查
# Node.js 服务部署验证

SERVICE=${1:-nodejs-api}
NAMESPACE=${2:-production}

echo "=== 部署验证: $SERVICE ==="

# 1. Pod 状态
echo "--- 1. Pod 状态 ---"
kubectl get pods -n $NAMESPACE -l app=$SERVICE -o wide

# 2. 健康检查
echo ""
echo "--- 2. 健康检查 ---"
POD=$(kubectl get pods -n $NAMESPACE -l app=$SERVICE -o jsonpath='{.items[0].metadata.name}')
kubectl exec -n $NAMESPACE $POD -- wget -qO- http://localhost:3000/healthz 2>/dev/null || echo "健康检查失败"

# 3. 内存使用
echo ""
echo "--- 3. 内存使用 ---"
kubectl top pod -n $NAMESPACE -l app=$SERVICE

# 4. V8 堆状态
echo ""
echo "--- 4. V8 堆状态 ---"
kubectl exec -n $NAMESPACE $POD -- node -e "
const used = process.memoryUsage();
console.log('RSS:', Math.round(used.rss / 1024 / 1024), 'MB');
console.log('Heap Total:', Math.round(used.heapTotal / 1024 / 1024), 'MB');
console.log('Heap Used:', Math.round(used.heapUsed / 1024 / 1024), 'MB');
"

# 5. 最近日志
echo ""
echo "--- 5. 最近日志 ---"
kubectl logs -n $NAMESPACE -l app=$SERVICE --tail=10

echo ""
echo "=== 验证完成 ==="
```

## Related

- [[02-工作负载/01-核心工作负载/01-workload-overview-architecture.md|工作负载架构总览]]
- [[02-工作负载/01-核心工作负载/21-hpa-vpa-autoscaling.md|HPA/VPA 自动伸缩]]
- [[02-工作负载/04-多语言运行时/01-go-on-kubernetes-production.md|Go on K8s]]
- [[27-标签/production|production 标签枢纽]]
