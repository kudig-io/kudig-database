---
title: 负载均衡基础
description: L4/L7 负载均衡原理、算法、健康检查、连接保持、K8s Service/Ingress/Gateway API 负载均衡实现
summary: 负载均衡完整知识，覆盖 L4/L7 对比、6种算法、健康检查、K8s 实现、生产调优
category: knowledge
tags:
- networking
- load-balancing
- service
- ingress
- gateway-api
domain: 系统基础
difficulty: intermediate
audience:
- SRE
- 平台工程师
- 网络工程师
---

# 负载均衡基础

> 负载均衡是分布式系统的核心组件，Kubernetes 通过 Service、Ingress、Gateway API 提供多层负载均衡能力。理解其原理是优化服务可用性和性能的关键。

## 负载均衡分类

### 按 OSI 层次

| 层次 | 名称 | 工作层 | 路由依据 | K8s 实现 |
|------|------|--------|----------|----------|
| L4 | 传输层 LB | TCP/UDP | IP:Port | Service/kube-proxy |
| L7 | 应用层 LB | HTTP/gRPC | URL/Header/Cookie | Ingress/Gateway API |

### L4 负载均衡

```
客户端 → L4 LB (IP:Port) → 后端服务器组
              │
              ├── 不解析应用层内容
              ├── 基于连接转发
              ├── 性能极高 (百万 CPS)
              └── 支持 TCP/UDP 任意协议
```

**K8s L4 LB 实现：**
- **ClusterIP Service**: kube-proxy iptables/IPVS 实现
- **NodePort Service**: 节点端口 + iptables DNAT
- **LoadBalancer Service**: 云 LB + NodePort
- **MetalLB**: 裸金属 L4 LB

### L7 负载均衡

```
客户端 → L7 LB (HTTP 解析) → 后端服务器组
              │
              ├── 解析 HTTP 请求
              ├── 基于 URL/Header/Cookie 路由
              ├── TLS 终止
              ├── 请求/响应修改
              └── 支持限流/认证/缓存
```

**K8s L7 LB 实现：**
- **Ingress**: Nginx Ingress / Traefik / HAProxy
- **Gateway API**: Envoy / Cilium / Istio
- **Service Mesh**: Istio/Linkerd sidecar

## 负载均衡算法

### 算法对比

| 算法 | 原理 | 优点 | 缺点 | 适用场景 |
|------|------|------|------|----------|
| 轮询 (RR) | 依次分配 | 简单公平 | 不考虑性能差异 | 后端均匀 |
| 加权轮询 (WRR) | 按权重分配 | 考虑性能 | 权重需手动 | 异构后端 |
| 最少连接 (LC) | 选连接最少的 | 自适应 | 需维护计数 | 长连接 |
| 加权最少连接 (WLC) | 连接数/权重 | 更精确 | 复杂 | 异构+长连接 |
| IP Hash | 源 IP 哈希 | 会话保持 | 分布不均 | 有状态 |
| 一致性哈希 | 环形哈希 | 节点变化影响小 | 实现复杂 | 缓存/分片 |
| 随机 | 随机选择 | 最简单 | 不均匀 | 大规模 |

### K8s 中的算法支持

| 组件 | 支持算法 |
|------|----------|
| kube-proxy (iptables) | 随机概率 (等价轮询) |
| kube-proxy (IPVS) | rr, wrr, lc, wlc, sh, dh, sed, nq |
| Nginx Ingress | round_robin, ewma, random |
| Envoy/Istio | round_robin, least_request, random, ring_hash, maglev |
| Gateway API | 由实现决定 |

### 配置示例

```yaml
# IPVS 模式配置
apiVersion: kubeproxy.config.k8s.io/v1alpha1
kind: KubeProxyConfiguration
mode: "ipvs"
ipvs:
  scheduler: "wrr"  # 加权轮询
  strictARP: true
---
# Istio 负载均衡
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: app-lb
spec:
  host: app-service.default.svc.cluster.local
  trafficPolicy:
    loadBalancer:
      simple: LEAST_REQUEST  # 最少请求
---
# Nginx Ingress 负载均衡
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  annotations:
    nginx.ingress.kubernetes.io/upstream-hash-by: "$request_uri"  # 一致性哈希
```

## 健康检查

### 健康检查类型

| 类型 | 原理 | 优点 | 缺点 |
|------|------|------|------|
| TCP 端口 | 尝试建立 TCP 连接 | 简单快速 | 不验证应用 |
| HTTP GET | 发送 HTTP 请求 | 验证应用逻辑 | 有开销 |
| gRPC | gRPC Health Check | 原生支持 | 仅 gRPC |
| 被动检查 | 观察实际请求结果 | 无额外开销 | 滞后 |

### K8s 健康检查机制

```yaml
spec:
  containers:
  - name: app
    # 启动探针 - 判断应用是否启动完成
    startupProbe:
      httpGet:
        path: /healthz
        port: 8080
      initialDelaySeconds: 5
      periodSeconds: 5
      failureThreshold: 30  # 最多等 150s
    # 存活探针 - 判断是否需要重启
    livenessProbe:
      httpGet:
        path: /healthz
        port: 8080
      periodSeconds: 10
      failureThreshold: 3
    # 就绪探针 - 判断是否接收流量
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080
      periodSeconds: 5
      failureThreshold: 2
      successThreshold: 1
```

### 健康检查与负载均衡的关系

```
Readiness Probe 失败
    │
    ▼
Pod 从 Endpoints 移除
    │
    ▼
kube-proxy 更新 iptables/IPVS 规则
    │
    ▼
新请求不再路由到该 Pod
    │
    ▼
(已建立的连接不受影响！)
```

## 连接保持与会话亲和

### 会话亲和配置

```yaml
# Service 级别会话亲和
apiVersion: v1
kind: Service
metadata:
  name: app-service
spec:
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800  # 3小时
  ports:
  - port: 80
    targetPort: 8080
  selector:
    app: myapp
```

### Ingress 会话保持

```yaml
# Nginx Ingress Cookie 会话保持
metadata:
  annotations:
    nginx.ingress.kubernetes.io/affinity: "cookie"
    nginx.ingress.kubernetes.io/session-cookie-name: "route"
    nginx.ingress.kubernetes.io/session-cookie-expires: "172800"
    nginx.ingress.kubernetes.io/session-cookie-max-age: "172800"
```

## K8s 负载均衡架构

### 完整请求路径

```
外部用户
    │
    ▼
DNS 解析 (域名 → LB IP)
    │
    ▼
云 LB / MetalLB (L4)
    │
    ▼
NodePort (30000-32767)
    │
    ▼ (iptables DNAT)
Ingress Controller Pod (L7)
    │
    ├── TLS 终止
    ├── 路由匹配
    ├── 限流/认证
    │
    ▼
Service ClusterIP (L4)
    │
    ▼ (iptables/IPVS DNAT)
后端 Pod
```

### 各层负载均衡对比

| 层 | 组件 | 功能 | 性能 |
|------|------|------|------|
| L4 外部 | 云 LB/MetalLB | 入口流量分发 | 极高 |
| L4 内部 | kube-proxy | Service → Pod | 高 |
| L7 入口 | Ingress/Gateway | HTTP 路由 | 中 |
| L7 内部 | Service Mesh | 服务间路由 | 中 |

## 生产调优

### 连接复用

```yaml
# Nginx Ingress 上游 Keep-Alive
annotations:
  nginx.ingress.kubernetes.io/upstream-keepalive-connections: "200"
  nginx.ingress.kubernetes.io/upstream-keepalive-timeout: "60"
  nginx.ingress.kubernetes.io/proxy-http-version: "1.1"
```

### 超时配置最佳实践

```yaml
# 超时层级（从外到内递增）
云 LB 超时: 120s
  Ingress 超时: 60s
    应用超时: 30s
      数据库超时: 10s

# Nginx Ingress 超时
annotations:
  nginx.ingress.kubernetes.io/proxy-connect-timeout: "5"
  nginx.ingress.kubernetes.io/proxy-read-timeout: "60"
  nginx.ingress.kubernetes.io/proxy-send-timeout: "60"
```

### 重试策略

```yaml
# Istio 重试
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
spec:
  http:
  - route:
    - destination:
        host: app-service
    retries:
      attempts: 3
      perTryTimeout: 10s
      retryOn: "5xx,connect-failure,refused-stream"
      retryRemoteLocalities: true
```

## 生产案例

### 案例1：负载不均

**症状：** 部分 Pod 负载 80%，部分仅 10%

**根因：** HTTP Keep-Alive 长连接导致连接固定到少数 Pod

**解决：**
```yaml
# 使用 least_request 算法
# 或设置连接最大请求数
nginx.ingress.kubernetes.io/upstream-keepalive-requests: "1000"
```

### 案例2：滚动更新流量丢失

**症状：** 部署期间少量 502 错误

**根因：** Pod 终止 → Endpoints 更新 → iptables 更新 有延迟

**解决：** preStop sleep + PodDisruptionBudget

### 案例3：跨可用区延迟

**症状：** P99 延迟异常高

**根因：** 流量跨 AZ 转发

**解决：** 启用拓扑感知路由 (Topology Aware Hints)

## 负载均衡监控

### 关键指标

| 指标 | 含义 | 告警阈值 |
|------|------|----------|
| 活跃连接数 | 当前连接总数 | 接近上限 |
| 新建连接率 | CPS (Connections/s) | 突增 |
| 后端健康数 | 健康后端比例 | < 50% |
| 请求延迟 P99 | 后端响应时间 | > SLA |
| 5xx 率 | 服务端错误比例 | > 1% |
| 吐量 | 请求/秒 | 接近容量 |

### Prometheus 告警规则

```yaml
groups:
- name: load-balancer-alerts
  rules:
  - alert: HighBackendErrorRate
    expr: |
      sum(rate(nginx_ingress_controller_requests{status=~"5.."}[5m]))
      / sum(rate(nginx_ingress_controller_requests[5m])) > 0.05
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Ingress 5xx 错误率超过 5%"

  - alert: NoHealthyBackends
    expr: kube_endpoint_address_available{endpoint!=""} == 0
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "Service {{ $labels.endpoint }} 无健康后端"

  - alert: HighIngressLatency
    expr: |
      histogram_quantile(0.99,
        sum(rate(nginx_ingress_controller_request_duration_seconds_bucket[5m])) by (le)
      ) > 5
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Ingress P99 延迟超过 5s"
```

## 常见问题 FAQ

**Q1: K8s Service 的负载均衡是客户端还是服务端？**
A: 是客户端负载均衡。kube-proxy 在每个节点上配置 iptables/IPVS 规则，由发起请求的 Pod 所在节点执行 DNAT，随机选择一个后端 Pod。

**Q2: 为什么 iptables 模式是“随机”而不是“轮询”？**
A: iptables 使用 `statistic` 模块的 `random` 模式，通过概率实现等价轮询。例如 3 个后端，第1个概率 1/3，第2个 1/2(剩余)，第3个 1/1。

**Q3: 如何避免滚动更新时的流量丢失？**
A: 三层防护：1) preStop sleep 等待 Endpoints 更新；2) PodDisruptionBudget 保证最小可用数；3) 就绪探针确保新 Pod 真正就绪后才接收流量。

**Q4: 什么时候需要 Service Mesh？**
A: 当需要：细粒度流量控制（金丝雀/镜像）、mTLS 加密、分布式追踪、熔断/重试策略、跨集群流量管理时。简单场景用 Ingress + Service 即可。

**Q5: 负载均衡器如何处理 WebSocket？**
A: WebSocket 是长连接，L4 LB 透明转发即可。L7 LB 需要支持 HTTP Upgrade，Nginx Ingress 需配置 `proxy-http-version: "1.1"` 和 Upgrade 头。

## 版本兼容矩阵

| 组件 | 版本 | LB 相关变化 |
|------|------|-------------|
| Kubernetes | 1.21+ | 拓扑感知提示 (Alpha) |
| Kubernetes | 1.27+ | 拓扑感知路由 GA |
| Kubernetes | 1.28+ | Gateway API v1 |
| kube-proxy | 1.31+ | nftables 模式 GA |
| Nginx Ingress | 1.8+ | EWMA 算法 |
| Envoy | 1.28+ | HTTP/3 支持 |

## 检查清单

- [ ] 理解 L4 vs L7 负载均衡区别
- [ ] 掌握各算法适用场景
- [ ] 能配置健康检查（三种探针）
- [ ] 理解会话保持机制
- [ ] 掌握超时配置层级
- [ ] 能排查负载不均问题
- [ ] 理解滚动更新流量处理
- [ ] 了解拓扑感知路由
- [ ] 掌握 LB 监控指标和告警
- [ ] 理解连接复用对 LB 的影响

## 参考链接

- [[17-系统基础/03-网络基础/index.md|网络基础总索引]]
- [[17-系统基础/03-网络基础/04-http-https-protocols.md|HTTP/HTTPS 协议]]
- [[17-系统基础/05-速查卡/gateway-api.md|Gateway API 速查卡]]
- [[17-系统基础/04-K8s事件/10-service-networking-events.md|Service 网络事件]]
