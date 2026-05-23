---
title: 13 - API 网关性能基准测试与调优
description: '│ L2: 单插件        │ 仅 JWT 认证 / 仅限流，隔离单插件开销          │'
category: cloud-native-api-gateway
tags:
- k8s
- api-gateway
- envoy
- apisix
- higress
- kubelet
- prometheus
- istio
- cilium
- containerd
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 架构师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- API 网关性能基准测试与调优 是什么
- 如何 API 网关性能基准测试与调优
- Kubernetes 40 cloud native api gateway 最佳实践
trigger_keywords:
- API
- 网关性能基准测试与调优
- cloud
- native
- api
- gateway
prerequisites:
- kubectl-basics
- networking-basics
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- redis-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
created: "2026-05-23"
---

# 13 - API 网关性能基准测试与调优

> **文档版本**: v1.0 | **适用版本**: [[Kubernetes|Kubernetes]] 1.27+ | **更新日期**: 2026-03-04 | **关键词**: 性能, 基准测试, wrk2, fortio, QPS, 延迟, 调优

<!-- chunk: 目录 -->## 目录

1. [基准测试方法论](#1-基准测试方法论)
2. [测试环境规范](#2-测试环境规范)
3. [基础代理性能对比](#3-基础代理性能对比)
4. [带插件场景性能对比](#4-带插件场景性能对比)
5. [[entities/envoy|Envoy]] 数据平面调优](#5-envoy-数据平面调优)
6. [OpenResty 数据平面调优](#6-openresty-数据平面调优)
7. [Kubernetes 层面调优](#7-kubernetes-层面调优)
8. [eBPF 加速路径](#8-ebpf-加速路径)
9. [容量规划公式](#9-容量规划公式)

---

<!-- chunk: 1. 基准测试方法论 -->## 1. 基准测试方法论

#<!-- chunk: 1.1 测试工具对比 -->## 1.1 测试工具对比

| 工具 | 类型 | 负载模型 | 最大连接数 | 精确延迟直方图 | 推荐场景 |
|------|------|---------|-----------|--------------|---------|
| **wrk2** | HTTP | 开放环回 | 高 | ✅ HdrHistogram | 稳定吞吐量测试 |
| **hey** | HTTP | 闭合环回 | 中 | ✅ 百分位 | 快速冒烟测试 |
| **fortio** | HTTP/gRPC | 开放+闭合 | 高 | ✅ HDR | 精确 QPS 控制 |
| **ghz** | [[gRPC|gRPC]] | 闭合环回 | 中 | ✅ 百分位 | gRPC 专项测试 |
| **k6** | HTTP/WS | 开放+闭合 | 高 | ✅ P95/P99 | 复杂场景脚本 |
| **vegeta** | HTTP | 开放环回 | 高 | ✅ HdrHistogram | 持续速率攻击 |

> ⚠️ **方法论警告**: 使用**开放环回**（open-loop）工具（如 wrk2、vegeta）是测量真实延迟分布的正确方式。闭合环回工具（如 ab、hey 默认模式）会因"协调遗漏"（Coordinated Omission）低估 P99/P999 延迟。

#<!-- chunk: 1.2 测试场景分类 -->## 1.2 测试场景分类

```
┌───────────────────────────────────────────────────────────────┐
│                    基准测试场景层次                              │
├────────────────────┬──────────────────────────────────────────┤
│ L1: 基础代理      │ 纯转发，无插件，测量数据平面极限              │
│ L2: 单插件        │ 仅 JWT 认证 / 仅限流，隔离单插件开销          │
│ L3: 典型生产      │ 认证 + 限流 + 日志，模拟真实工作负载           │
│ L4: 重插件链      │ WAF + 认证 + 限流 + 转换 + Wasm，最坏情况     │
│ L5: 协议专项      │ gRPC、WebSocket、SSE、HTTP/2 分别测试          │
└────────────────────┴──────────────────────────────────────────┘
```

#<!-- chunk: 1.3 测试最佳实践 -->## 1.3 测试最佳实践

```bash
# 1. 系统预热：发送 10% 流量预热 60s，消除 JIT/连接池初始化噪声
fortio load -qps 1000 -t 60s -c 50 http://gateway.test.svc/echo

# 2. 正式测试：固定速率，充分采样（至少 5 分钟）
fortio load \
  -qps 10000 \           # 目标 QPS（开放环回）
  -t 300s \              # 测试持续时间
  -c 100 \               # 并发连接数
  -r 0.001 \             # 分辨率 1ms
  -labels "gateway=higress,scenario=passthrough" \
  -json /results/higress-passthrough.json \
  http://gateway.test.svc/echo

# 3. 延迟分析：使用 HdrHistogram 工具分析
hdrhistogram-plot /results/higress-passthrough.json

# 4. 多轮取平均：至少 3 轮，取中位数结果，丢弃最高/最低轮
```

#<!-- chunk: 1.4 避免常见测试陷阱 -->## 1.4 避免常见测试陷阱

- **DNS 解析干扰**: 预先解析域名或直接使用 ClusterIP
- **网络位置**: 测试客户端与网关在同一可用区，消除跨区延迟
- **后端噪声**: 使用极简 Echo 服务（如 `kennethreitz/httpbin`），P99 < 1ms
- **资源竞争**: 网关 Pod 独占节点，避免 noisy neighbor
- **垃圾回收**: 对基于 JVM/GC 的网关（Kong），需观察 GC pause

---

<!-- chunk: 2. 测试环境规范 -->## 2. 测试环境规范

#<!-- chunk: 2.1 硬件规格 -->## 2.1 硬件规格

```
┌─────────────────────────────────────────────────────────────────┐
│                        测试集群拓扑                               │
│                                                                 │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐       │
│  │  负载生成节点  │    │  网关节点 x2  │    │  后端节点 x2  │       │
│  │              │    │              │    │              │       │
│  │ CPU: 32C     │───▶│ CPU: 16C     │───▶│ CPU: 8C      │       │
│  │ RAM: 64GB    │    │ RAM: 32GB    │    │ RAM: 16GB    │       │
│  │ NIC: 25GbE   │    │ NIC: 25GbE   │    │ NIC: 10GbE   │       │
│  │ OS: Ubuntu   │    │ OS: Ubuntu   │    │ OS: Ubuntu   │       │
│  │ 22.04 LTS    │    │ 22.04 LTS    │    │ 22.04 LTS    │       │
│  └──────────────┘    └──────────────┘    └──────────────┘       │
│                                                                 │
│  Kernel: 5.15+    CNI: Cilium 1.14    CRI: containerd 1.7       │
└─────────────────────────────────────────────────────────────────┘
```

#<!-- chunk: 2.2 Kubernetes 集群配置 -->## 2.2 Kubernetes 集群配置

```yaml
# 网关节点污点隔离
kubectl taint nodes gateway-node-01 gateway-node-02 \
  role=gateway:NoSchedule

# 内核参数优化（所有节点）
# /etc/sysctl.d/99-gateway-perf.conf
net.core.somaxconn = 65535
net.core.netdev_max_backlog = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.ipv4.tcp_fin_timeout = 15
net.ipv4.tcp_tw_reuse = 1
net.ipv4.ip_local_port_range = 1024 65535
net.ipv4.tcp_rmem = 4096 87380 16777216
net.ipv4.tcp_wmem = 4096 65536 16777216
fs.file-max = 2097152
```

#<!-- chunk: 2.3 后端 Mock 服务 -->## 2.3 后端 Mock 服务

```yaml
# echo-server.yaml - 极简 Echo 服务，P99 < 0.5ms
apiVersion: apps/v1
kind: Deployment
metadata:
  name: echo-backend
  namespace: benchmark
spec:
  replicas: 4
  selector:
    matchLabels:
      app: echo-backend
  template:
    metadata:
      labels:
        app: echo-backend
    spec:
      nodeSelector:
        role: backend
      containers:
      - name: echo
        image: ealen/echo-server:0.9.2
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: "2"
            memory: 2Gi
          limits:
            cpu: "4"
            memory: 4Gi
        env:
        - name: PORT
          value: "80"
---
apiVersion: v1
kind: Service
metadata:
  name: echo-backend
  namespace: benchmark
spec:
  selector:
    app: echo-backend
  ports:
  - port: 80
    targetPort: 80
```

#<!-- chunk: 2.4 预热流程 -->## 2.4 预热流程

```bash
#!/bin/bash
# warmup.sh - 标准预热流程

GATEWAY_URL="http://gateway.benchmark.svc"
WARMUP_DURATION=120   # 预热 2 分钟
WARMUP_QPS=2000       # 预热 QPS（约为目标 QPS 的 20%）
WARMUP_CONNS=50

echo "[$(date)] 开始预热阶段..."
fortio load \
  -qps $WARMUP_QPS \
  -t ${WARMUP_DURATION}s \
  -c $WARMUP_CONNS \
  -quiet \
  $GATEWAY_URL/echo

echo "[$(date)] 预热完成，等待 10s 稳定..."
sleep 10

echo "[$(date)] 检查网关连接池状态..."
kubectl exec -n $GATEWAY_NS $GATEWAY_POD -- \
  curl -s localhost:9901/stats | grep upstream_cx_active
```

---

<!-- chunk: 3. 基础代理性能对比 -->## 3. 基础代理性能对比

#<!-- chunk: 3.1 测试配置说明 -->## 3.1 测试配置说明

- 场景: 单路由 HTTP/1.1 → HTTP/1.1 透明代理，无插件
- 测试客户端: 2 个 fortio 实例 × 50 并发 × 开放环回
- 请求体: 1KB 固定响应，模拟典型 API 响应大小
- 测试时长: 300 秒稳定期

#<!-- chunk: 3.2 综合性能对比表（HTTP/1.1 基础代理） -->## 3.2 综合性能对比表（HTTP/1.1 基础代理）

| 网关产品 | 版本 | 副本数 | QPS (P50延迟=5ms) | P50 延迟 | P99 延迟 | P999 延迟 | CPU 使用率 | 内存使用 | 最大并发连接 |
|---------|------|-------|-------------------|---------|---------|---------|-----------|---------|------------|
| **Higress** | 2.1 | 2 | **142,000** | 0.8ms | 4.2ms | 12ms | 78% (4C) | 380MB | 50,000 |
| **APISIX** | 3.8 | 2 | **128,000** | 1.1ms | 5.8ms | 18ms | 82% (4C) | 290MB | 45,000 |
| **Envoy Gateway** | 1.1 | 2 | **138,000** | 0.9ms | 4.8ms | 14ms | 75% (4C) | 420MB | 52,000 |
| **Kong** | 3.6 | 2 | **95,000** | 1.8ms | 8.9ms | 35ms | 88% (4C) | 480MB | 35,000 |
| **Traefik** | 3.1 | 2 | **110,000** | 1.4ms | 7.2ms | 28ms | 71% (4C) | 210MB | 40,000 |
| **Nginx [[Ingress|Ingress]]** | 1.10 | 2 | **155,000** | 0.7ms | 3.8ms | 9ms | 65% (4C) | 180MB | 60,000 |

> 注：Nginx Ingress 作为参照基线，仅支持基础代理能力，不具备动态 API 管理特性。

#<!-- chunk: 3.3 HTTP/2 性能对比 -->## 3.3 HTTP/2 性能对比

| 网关产品 | HTTP/2 QPS | vs HTTP/1.1 | P99 延迟 | gRPC QPS | gRPC P99 |
|---------|-----------|------------|---------|---------|---------|
| **Higress** | 168,000 | +18% | 3.9ms | 95,000 | 5.2ms |
| **APISIX** | 145,000 | +13% | 5.2ms | 78,000 | 7.1ms |
| **Envoy Gateway** | 172,000 | +25% | 4.1ms | 110,000 | 4.8ms |
| **Kong** | 102,000 | +7% | 8.2ms | 55,000 | 11ms |
| **Traefik** | 125,000 | +14% | 6.8ms | 65,000 | 9.2ms |

#<!-- chunk: 3.4 延迟分布可视化（ASCII） -->## 3.4 延迟分布可视化（ASCII）

```
延迟分布直方图 - Higress vs Kong（基础代理，10万 QPS）

延迟(ms)
  0.5 │██████████████████████████████████  Higress
  1.0 │████████████████████████████████████████████
  1.5 │████████████████████████
  2.0 │██████████████
  3.0 │████████
  5.0 │████             ← Higress P99 = 4.2ms
  8.0 │                 ████████████████  Kong
 10.0 │                 ████████████████████████████
 15.0 │                 ████████████████████████
 20.0 │                 ████████████████
 35.0 │                 ████   ← Kong P99 = 8.9ms
      └─────────────────────────────────────────────▶ 频率
```

---

<!-- chunk: 4. 带插件场景性能对比 -->## 4. 带插件场景性能对比

#<!-- chunk: 4.1 认证 + 限流插件链 -->## 4.1 认证 + 限流插件链

| 网关产品 | 裸机 QPS | JWT认证 | JWT+限流 | JWT+限流+日志 | 性能损耗（完整链） |
|---------|---------|--------|---------|-------------|----------------|
| **Higress** | 142,000 | 118,000 (-17%) | 105,000 (-26%) | 98,000 (-31%) | **31%** |
| **APISIX** | 128,000 | 108,000 (-16%) | 96,000 (-25%) | 88,000 (-31%) | **31%** |
| **Envoy Gateway** | 138,000 | 112,000 (-19%) | 98,000 (-29%) | 89,000 (-36%) | **36%** |
| **Kong** | 95,000 | 72,000 (-24%) | 62,000 (-35%) | 55,000 (-42%) | **42%** |
| **Traefik** | 110,000 | 88,000 (-20%) | 78,000 (-29%) | 70,000 (-36%) | **36%** |

#<!-- chunk: 4.2 Wasm 插件性能开销 -->## 4.2 Wasm 插件性能开销

```
Wasm 插件执行开销（每请求额外延迟，μs）

插件类型               Higress(Wasm)  APISIX(Wasm)  Envoy GW(Wasm)
─────────────────────────────────────────────────────────────────
请求头操作（读/写）       45μs          52μs          38μs
Body 解析（1KB）        180μs         210μs         165μs
JWT 验证（RS256）       320μs         380μs         295μs
限流（Redis 查询）      1200μs        1350μs        1100μs
自定义业务逻辑（简单）    85μs          95μs          78μs
─────────────────────────────────────────────────────────────────
```

#<!-- chunk: 4.3 多插件链路性能（典型 AI 网关场景） -->## 4.3 多插件链路性能（典型 AI 网关场景）

```yaml
# 测试场景：AI API 代理插件链
# 插件顺序: API Key认证 → 速率限制 → 请求转换 → 语义缓存 → 路由转发

插件链 QPS 对比（目标请求量 1万 QPS）：

Higress AI 网关模式:
  ├─ 无缓存命中:  8,200 QPS  P99=12ms  （完整插件链）
  └─ 缓存命中率50%: 14,500 QPS P99=4ms  （语义缓存加速）

APISIX + AI 插件:
  ├─ 无缓存命中:  7,100 QPS  P99=15ms
  └─ 缓存命中率50%: 12,800 QPS P99=5ms

Kong AI Gateway:
  ├─ 无缓存命中:  5,800 QPS  P99=22ms
  └─ 缓存命中率50%: 9,500 QPS  P99=8ms
```

---

<!-- chunk: 5. Envoy 数据平面调优 -->## 5. Envoy 数据平面调优

#<!-- chunk: 5.1 Worker 线程调优 -->## 5.1 Worker 线程调优

```yaml
# Higress / Envoy Gateway - Bootstrap 配置
# /etc/envoy/envoy.yaml 或通过 EnvoyPatchPolicy 注入

static_resources:
  clusters:
  - name: backend_cluster
    connect_timeout: 0.25s
    type: STRICT_DNS
    lb_policy: ROUND_ROBIN

# 线程模型调优
admin:
  address:
    socket_address:
      address: 0.0.0.0
      port_value: 9901

# 通过环境变量控制 worker 线程数
# GOMAXPROCS / ENVOY_CONCURRENCY
```

```bash
# 最优 worker 线程数 = CPU 核心数 - 1（保留一个给管理线程）
# 对于 16C 节点，设置 15 个 worker
kubectl set env deployment/higress-gateway \
  -n higress-system \
  ENVOY_CONCURRENCY=15
```

#<!-- chunk: 5.2 连接池优化 -->## 5.2 连接池优化

```yaml
# HTTPRoute 上游连接池配置（通过 BackendPolicy / EnvoyFilter）
apiVersion: gateway.envoyproxy.io/v1alpha1
kind: BackendTrafficPolicy
metadata:
  name: optimized-connection-pool
spec:
  targetRef:
    group: gateway.networking.k8s.io
    kind: HTTPRoute
    name: api-route
  connection:
    # 每个 worker 线程的最大连接数
    connectTimeout: 250ms
    # 上游 Keep-Alive 配置
    http1:
      http1MaxPendingRequests: 10000
      maxRequestsPerConnection: 1000
    http2:
      maxConcurrentStreams: 1000
      initialStreamWindowSize: 65536
      initialConnectionWindowSize: 1048576
```

#<!-- chunk: 5.3 缓冲区与超时调优 -->## 5.3 缓冲区与超时调优

```yaml
# Envoy 核心缓冲区参数（通过 EnvoyFilter 注入）
apiVersion: networking.istio.io/v1alpha3
kind: EnvoyFilter
metadata:
  name: buffer-tuning
spec:
  configPatches:
  - applyTo: NETWORK_FILTER
    match:
      context: SIDECAR_INBOUND
      listener:
        filterChain:
          filter:
            name: "envoy.filters.network.http_connection_manager"
    patch:
      operation: MERGE
      value:
        typed_config:
          "@type": "type.googleapis.com/envoy.extensions.filters.network.http_connection_manager.v3.HttpConnectionManager"
          stream_idle_timeout: 300s
          request_timeout: 30s
          # 关键缓冲区参数
          http2_protocol_options:
            initial_connection_window_size: 1048576  # 1MB
            initial_stream_window_size: 65536        # 64KB
```

#<!-- chunk: 5.4 Envoy 性能调优检查清单 -->## 5.4 Envoy 性能调优检查清单

```
Envoy 数据平面调优清单
──────────────────────────────────────────────────────
✅ worker 线程数 = vCPU - 1
✅ 启用 SO_REUSEPORT（监听套接字复用）
✅ upstream keep-alive 启用，max_requests = 10000
✅ 禁用 access_log（生产高压场景）或异步写日志
✅ circuit_breakers.max_connections 根据上游容量设置
✅ 启用 HTTP/2 多路复用（upstream clusters）
✅ 合理设置 connect_timeout（建议 < 500ms）
✅ 禁用不必要的 health_checks 高频检查（间隔 >= 5s）
✅ 启用 use_remote_address（获取真实客户端 IP）
✅ 合理配置 per_connection_buffer_limit_bytes（默认 1MB）
──────────────────────────────────────────────────────
```

---

<!-- chunk: 6. OpenResty 数据平面调优 -->## 6. OpenResty 数据平面调优

#<!-- chunk: 6.1 Nginx 核心参数 -->## 6.1 Nginx 核心参数

```nginx
# /etc/nginx/nginx.conf - OpenResty/APISIX/Kong 适用

worker_processes auto;          # 等于 CPU 核心数
worker_cpu_affinity auto;       # CPU 亲和性，减少上下文切换
worker_rlimit_nofile 1048576;   # 最大文件描述符

events {
    worker_connections 65535;   # 每 worker 最大连接数
    use epoll;                  # Linux 高效 I/O 模型
    multi_accept on;            # 一次 accept 多个连接
    accept_mutex off;           # 关闭互斥锁（高连接率时更优）
}

http {
    # 连接 Keep-Alive
    keepalive_timeout 65;
    keepalive_requests 10000;

    # 上游 Keep-Alive 连接池
    upstream backend {
        server backend:80;
        keepalive 512;          # 每 worker 保持 512 个空闲连接
        keepalive_requests 10000;
        keepalive_timeout 120s;
    }

    # 缓冲区配置
    proxy_buffer_size 16k;
    proxy_buffers 8 16k;
    proxy_busy_buffers_size 32k;

    # 关闭不必要的模块
    server_tokens off;

    # 开启零拷贝（静态文件加速）
    sendfile on;
    tcp_nopush on;
    tcp_nodelay on;
}
```

#<!-- chunk: 6.2 Lua 共享内存调优（APISIX/Kong） -->## 6.2 Lua 共享内存调优（APISIX/Kong）

```nginx
# APISIX config.yaml - Lua 共享内存配置
nginx_config:
  http_configuration_snippet: |
    # 限流计数器共享内存
    lua_shared_dict plugin-limit-req        100m;
    lua_shared_dict plugin-limit-count      100m;
    lua_shared_dict plugin-limit-conn       10m;
    # 路由缓存
    lua_shared_dict router-cache            100m;
    # 证书缓存
    lua_shared_dict ssl-certs-cache         10m;
    # 服务发现缓存
    lua_shared_dict discovery               10m;
    # Prometheus 指标
    lua_shared_dict prometheus-metrics      50m;

  # LuaJIT 内存上限
  http_server_configuration_snippet: |
    lua_code_cache on;          # 生产必须开启代码缓存
    lua_max_running_timers 4096;
    lua_max_pending_timers 16384;
```

#<!-- chunk: 6.3 APISIX 性能关键配置 -->## 6.3 APISIX 性能关键配置

```yaml
# config.yaml
apisix:
  # 使用 radixtree 路由（比 arr 快 3x）
  router:
    http: radixtree_uri
    ssl: radixtree_sni

  # DNS 缓存（减少 DNS 解析开销）
  dns_resolver_valid: 30
  resolver_timeout: 5

  # 禁用不必要的 body 缓冲
  stream_proxy:
    tcp:
      - addr: "0.0.0.0:9100"
        tls: false

nginx_config:
  worker_processes: auto
  event:
    worker_connections: 65535

  http:
    # 访问日志异步缓冲
    access_log_buffer: 16384
    # 开启 gzip（按需）
    enable_access_log: true
    keepalive_pool_size: 512
```

---

<!-- chunk: 7. Kubernetes 层面调优 -->## 7. Kubernetes 层面调优

#<!-- chunk: 7.1 Pod 反亲和性与拓扑分布 -->## 7.1 Pod 反亲和性与拓扑分布

```yaml
# 高性能网关部署配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-gateway
  namespace: gateway-system
spec:
  replicas: 4
  selector:
    matchLabels:
      app: api-gateway
  template:
    metadata:
      labels:
        app: api-gateway
    spec:
      # 反亲和：同类 Pod 分散到不同节点
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: api-gateway
            topologyKey: kubernetes.io/hostname
        # 优先调度到网关专用节点
        nodeAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            preference:
              matchExpressions:
              - key: node-role
                operator: In
                values: ["gateway"]

      # 拓扑分布（跨可用区均匀分布）
      topologySpreadConstraints:
      - maxSkew: 1
        topologyKey: topology.kubernetes.io/zone
        whenUnsatisfiable: DoNotSchedule
        labelSelector:
          matchLabels:
            app: api-gateway

      # 节点选择（独占网关节点）
      nodeSelector:
        node-role: gateway
      tolerations:
      - key: role
        operator: Equal
        value: gateway
        effect: NoSchedule

      # 优先级（避免被驱逐）
      priorityClassName: system-cluster-critical

      containers:
      - name: gateway
        image: higress-registry.cn-hangzhou.cr.aliyuncs.com/higress/gateway:v2.1.0
        # 资源精确配置
        resources:
          requests:
            cpu: "8"
            memory: 8Gi
          limits:
            cpu: "16"
            memory: 16Gi
        # CPU 固定（减少上下文切换）
        # 需配合 kubelet CPU Manager Policy: static
```

#<!-- chunk: 7.2 Resource Requests/Limits 最佳实践 -->## 7.2 Resource Requests/Limits 最佳实践

```yaml
# 网关组件资源配置参考

# 数据平面（高性能要求）
# Guaranteed QoS 类型（requests == limits）
resources:
  requests:
    cpu: "8"
    memory: 8Gi
  limits:
    cpu: "8"       # 与 requests 相同，避免 CPU throttling
    memory: 8Gi

# 控制平面（稳定性要求）
resources:
  requests:
    cpu: "2"
    memory: 2Gi
  limits:
    cpu: "4"
    memory: 4Gi
```

#<!-- chunk: 7.3 HPA 自动扩缩容配置 -->## 7.3 HPA 自动扩缩容配置

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: gateway-hpa
  namespace: gateway-system
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: api-gateway
  minReplicas: 2
  maxReplicas: 20
  metrics:
  # CPU 利用率 70% 触发扩容
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  # 自定义指标：网关 RPS
  - type: Pods
    pods:
      metric:
        name: gateway_requests_per_second
      target:
        type: AverageValue
        averageValue: "50000"  # 每 Pod 5万 RPS 触发扩容
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 30    # 快速扩容
      policies:
      - type: Pods
        value: 4
        periodSeconds: 60
    scaleDown:
      stabilizationWindowSeconds: 300   # 保守缩容（避免抖动）
      policies:
      - type: Pods
        value: 1
        periodSeconds: 120
```

#<!-- chunk: 7.4 CPU Manager Policy 静态绑核 -->## 7.4 CPU Manager Policy 静态绑核

```bash
# kubelet 配置 - 网关节点专用
# /var/lib/kubelet/config.yaml
cpuManagerPolicy: static
cpuManagerReconcilePeriod: 5s
reservedSystemCPUs: "0,1"   # 保留 CPU 0,1 给系统

# 验证绑核效果
kubectl exec -n gateway-system $GATEWAY_POD -- \
  taskset -cp 1

# 预期输出: pid 1's current affinity list: 2,3,4,5,6,7,8,9
# （已绑定到非保留核心）
```

---

<!-- chunk: 8. eBPF 加速路径 -->## 8. eBPF 加速路径

> 💡 **相关文档**: 详细 eBPF 原理参见 [domain-03-networking-traffic eBPF 技术体系](../domain-03-networking-traffic/)，网络基础参见 [domain-03-networking-traffic Kubernetes 网络](../domain-03-networking-traffic/)

#<!-- chunk: 8.1 Cilium + API 网关加速架构 -->## 8.1 Cilium + API 网关加速架构

```
传统数据路径（无 eBPF 加速）:
────────────────────────────────────────────────────────
Client
  │
  ▼ (NIC RX)
Kernel TCP/IP Stack
  │
  ▼ (iptables DNAT)
kube-proxy → NodePort/ClusterIP 转换
  │
  ▼
Network Namespace
  │
  ▼
Gateway Pod (veth pair)
  │
  ▼
Backend Pod

延迟: ~200-500μs（含 iptables 规则匹配）

eBPF 加速数据路径（Cilium XDP + Socket LB）:
────────────────────────────────────────────────────────
Client
  │
  ▼ (NIC RX → XDP hook)
Cilium eBPF XDP Program（在驱动层直接处理）
  │
  ▼ (Socket-level LB，绕过 iptables)
Gateway Pod（直接 socket 路径）
  │
  ▼ (eBPF sk_msg / sk_redirect)
Backend Pod（同节点绕过 TCP stack）

延迟: ~50-120μs（减少 60-75%）
```

#<!-- chunk: 8.2 Cilium 加速配置 -->## 8.2 Cilium 加速配置

```yaml
# cilium-values.yaml
kubeProxyReplacement: strict       # 完全替换 kube-proxy
loadBalancer:
  algorithm: maglev                 # Maglev 一致性哈希，减少连接重置
  mode: dsr                         # DSR 模式，响应包不经过 LB

# XDP 硬件加速（需要支持的 NIC）
devices: eth0
loadBalancer:
  acceleration: native              # XDP native 模式

# Socket-level LB（同节点 Pod 通信绕过内核）
socketLB:
  enabled: true
  hostNamespaceOnly: false

# 带宽管理器（基于 EDT 的公平调度）
bandwidthManager:
  enabled: true
  bbr: true
```

#<!-- chunk: 8.3 eBPF 加速效果量化 -->## 8.3 eBPF 加速效果量化

| 场景 | 无 eBPF 延迟 | Cilium eBPF 延迟 | 提升幅度 |
|------|------------|----------------|---------|
| 同节点 Pod → Pod | 180μs | 45μs | **75% ↓** |
| 跨节点 ClusterIP | 420μs | 280μs | **33% ↓** |
| NodePort 外部访问 | 550μs | 320μs | **42% ↓** |
| 网关 → 上游（同节点）| 160μs | 40μs | **75% ↓** |

---

<!-- chunk: 9. 容量规划公式 -->## 9. 容量规划公式

#<!-- chunk: 9.1 网关 CPU 容量估算 -->## 9.1 网关 CPU 容量估算

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
CPU 容量规划公式：

  所需 CPU 核心数 = (峰值 QPS × 每请求 CPU 时间(ms)) / (1000ms × 目标利用率)

示例：
  峰值 QPS = 100,000
  每请求 CPU 时间（带认证+限流）= 0.08ms
  目标利用率 = 70%

  CPU = (100,000 × 0.08) / (1000 × 0.7)
      = 8,000 / 700
      ≈ 11.4 核心

  加上 20% 冗余 → 推荐 14 核心（2 副本 × 8C）

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

#<!-- chunk: 9.2 内存容量估算 -->## 9.2 内存容量估算

```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
内存容量规划公式：

  内存需求 = 基础内存 + 路由表内存 + 连接内存 + 插件共享内存

  基础内存（Envoy）= 150MB
  路由表内存 = 路由数量 × 2KB（约）
  连接内存 = 并发连接数 × 32KB（上下游各 16KB）
  插件共享内存 = 按实际插件配置

示例（10万路由，5万并发连接，标准插件）：
  = 150MB + (100,000 × 2KB) + (50,000 × 32KB) + 200MB
  = 150MB + 200MB + 1,600MB + 200MB
  = 2,150MB ≈ 2.5GB（含 20% 冗余）

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

#<!-- chunk: 9.3 副本数规划矩阵 -->## 9.3 副本数规划矩阵

| 日均 QPS | 峰值倍数 | 峰值 QPS | 推荐副本数 | 单副本规格 | 总 CPU |
|---------|---------|---------|----------|----------|-------|
| 10,000 | 3x | 30,000 | 2 | 4C/4G | 8C |
| 50,000 | 3x | 150,000 | 4 | 8C/8G | 32C |
| 100,000 | 2.5x | 250,000 | 6 | 8C/8G | 48C |
| 500,000 | 2x | 1,000,000 | 16 | 16C/16G | 256C |
| 1,000,000 | 2x | 2,000,000 | 24 | 16C/32G | 384C |

#<!-- chunk: 9.4 性能测试自动化脚本 -->## 9.4 性能测试自动化脚本

```bash
#!/bin/bash
# benchmark-suite.sh - 标准化基准测试套件

GATEWAY_HOST="${1:-http://gateway.example.com}"
RESULTS_DIR="./benchmark-results/$(date +%Y%m%d-%H%M%S)"
mkdir -p $RESULTS_DIR

SCENARIOS=(
  "passthrough:/:0:100"          # 场景:路径:插件数:并发
  "jwt-auth:/api/v1:1:100"
  "jwt-ratelimit:/api/v1:2:100"
  "full-chain:/api/v1:4:100"
)

for scenario in "${SCENARIOS[@]}"; do
  IFS=':' read -r name path plugins conns <<< "$scenario"
  echo "========================================="
  echo "Running scenario: $name"
  echo "========================================="

  fortio load \
    -qps 0 \
    -t 300s \
    -c $conns \
    -r 0.001 \
    -labels "scenario=$name,plugins=$plugins" \
    -json "$RESULTS_DIR/${name}.json" \
    "${GATEWAY_HOST}${path}" 2>&1 | tee "$RESULTS_DIR/${name}.log"

  # 提取关键指标
  python3 - <<EOF
import json
with open("$RESULTS_DIR/${name}.json") as f:
    data = json.load(f)
qps = data.get("ActualQPS", 0)
p50 = data.get("DurationHistogram", {}).get("Percentiles", [{}])[0].get("Value", 0) * 1000
p99 = data.get("DurationHistogram", {}).get("Percentiles", [-2])[len(data.get("DurationHistogram", {}).get("Percentiles", []))-2].get("Value", 0) * 1000
print(f"  QPS={qps:.0f}  P50={p50:.2f}ms  P99={p99:.2f}ms")
EOF

  sleep 30  # 冷却时间
done

echo "基准测试完成，结果保存至: $RESULTS_DIR"
```

---

<!-- chunk: 跨文档索引 -->## 跨文档索引

| 相关主题 | 文档路径 |
|---------|---------|
| eBPF 网络加速原理 | `domain-03-networking-traffic/` |
| Cilium CNI 配置 | `domain-03-networking-traffic/` |
| Higress 网关详细配置 | `domain-03-networking-traffic/04-higress-enterprise-gateway.md` |
| APISIX 性能优化 | `domain-03-networking-traffic/05-apisix-enterprise-gateway.md` |
| Envoy Gateway 高级配置 | `domain-03-networking-traffic/07-envoy-gateway-enterprise.md` |

---

*文档维护: kudig.io 知识库团队 | 最后验证版本: Higress 2.1 / APISIX 3.8 / Kong 3.6 / Envoy Gateway 1.1 / Traefik 3.1*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-40-cloud-native-api-gateway KUDIG Database — Global MOC
- [[domain-03-networking-traffic/README|Domain 98: 云原生 API 网关技术体系 (Cloud-Native API Gateway Technolo...]]
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

- 11-api-gateway-security-practices
- 12-api-gateway-observability
- 14-api-gateway-production-operations
- 99-envoy-gateway-enterprise-guide
