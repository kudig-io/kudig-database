---
title: eBPF Observability
description: eBPF 可观测性深度指南 — Cilium Hubble、Pixie、bpftrace、内核级可观测、零侵入监控
summary: eBPF 可观测性完整指南，涵盖 Cilium Hubble 流量可视化、Pixie 自动遥测、bpftrace 动态追踪、内核级性能分析、零侵入监控架构
tags:
- ebpf
- observability
- cilium
- hubble
- pixie
- bpftrace
difficulty: advanced
domain: 可观测性
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
---
# eBPF 可观测性深度指南

## 1. eBPF 可观测性概述

### 1.1 为什么选择 eBPF

传统可观测性方案的局限：
- **侵入式**：需要修改应用代码或添加 Sidecar
- **性能开销**：Agent 采集消耗 CPU/内存
- **覆盖不全**：无法观测内核层、网络层细节
- **语言依赖**：不同语言需要不同 SDK

eBPF 的优势：
- **零侵入**：在内核层运行，无需修改应用
- **低开销**：JIT 编译，接近原生性能
- **全覆盖**：网络、存储、调度、安全全栈可观测
- **语言无关**：对所有应用透明

### 1.2 eBPF 可观测性架构

```
┌─────────────────────────────────────────────────────────┐
│                    应用层 (L7)                           │
│  HTTP/gRPC/DNS 协议解析（无需 Sidecar）                  │
├─────────────────────────────────────────────────────────┤
│                    传输层 (L4)                           │
│  TCP 连接追踪、延迟分析、重传检测                        │
├─────────────────────────────────────────────────────────┤
│                    网络层 (L3)                           │
│  包过滤、流量统计、策略执行                              │
├─────────────────────────────────────────────────────────┤
│                    内核层                                │
│  调度延迟、文件系统 I/O、内存分配                        │
└─────────────────────────────────────────────────────────┘
```

## 2. Cilium Hubble

### 2.1 架构

Hubble 是 Cilium 的可观测性组件：
- **Hubble Server**：聚合 eBPF 事件
- **Hubble Relay**：跨节点数据聚合
- **Hubble UI**：实时流量可视化

### 2.2 部署

```yaml
# 启用 Hubble（Cilium Helm values）
hubble:
  enabled: true
  relay:
    enabled: true
  ui:
    enabled: true
  metrics:
    enabled:
      - dns
      - drop
      - tcp
      - flow
      - port-distribution
      - icmp
      - http
```

### 2.3 流量可视化

```bash
# 安装 Hubble CLI
hubble observe --namespace production

# 按服务过滤
hubble observe --namespace production --to-service backend-api

# 查看 HTTP 错误
hubble observe --namespace production --http-status 5xx

# 查看 DNS 查询
hubble observe --namespace production --protocol dns

# 导出为 JSON
hubble observe --namespace production -o json > flows.json
```

### 2.4 Prometheus 指标

```yaml
# Hubble 暴露的指标
- hubble_flows_processed_total
- hubble_http_requests_total
- hubble_http_response_duration_seconds
- hubble_dns_queries_total
- hubble_drop_total
```

Grafana 仪表板查询：
```promql
# HTTP 请求速率
sum(rate(hubble_http_requests_total{namespace="production"}[5m])) by (method, status)

# P99 延迟
histogram_quantile(0.99,
  sum(rate(hubble_http_response_duration_seconds_bucket{
    namespace="production"
  }[5m])) by (le, service)
)

# DNS 解析延迟
histogram_quantile(0.95,
  sum(rate(hubble_dns_response_duration_seconds_bucket[5m])) by (le)
)
```

## 3. Pixie

### 3.1 架构

Pixie 是 New Relic 开源的 eBPF 可观测性平台：
- **Vizier**：集群内数据平面（eBPF 采集）
- **Cloud**：控制平面（查询、存储）
- **PxL**：Pixie 查询语言

### 3.2 自动遥测

Pixie 自动采集（无需配置）：
- HTTP/HTTP2/gRPC/MySQL/PostgreSQL/Cassandra 请求
- TCP 连接统计
- DNS 查询
- 进程级资源使用
- 网络流量

### 3.3 PxL 查询示例

```pxl
# 查看 HTTP 请求
import px

df = px.DataFrame(table='http_events', start_time='-5m')
df = df[df.namespace == 'production']
df = df.groupby(['service_name', 'req_method'], as_=df.agg(
    count=('latency', px.count),
    p99_latency=('latency', px.quantiles, 0.99),
    error_rate=('resp_status', lambda x: (x >= 500).sum() / x.count())
))
px.display(df)
```

```pxl
# 服务依赖图
import px

df = px.DataFrame(table='http_events', start_time='-1h')
edges = df.groupby(['client_service', 'server_service'], as_=df.agg(
    request_count=('latency', px.count),
    avg_latency=('latency', px.mean)
))
px.display(edges)
```

### 3.4 与 Prometheus 集成

```yaml
# Pixie 暴露 Prometheus 指标
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: pixie-metrics
spec:
  selector:
    matchLabels:
      app: pixie-metrics
  endpoints:
    - port: metrics
      interval: 30s
```

## 4. bpftrace 动态追踪

### 4.1 基础语法

```bpftrace
// 追踪系统调用
tracepoint:syscalls:sys_enter_openat {
    printf("%s %s\n", comm, str(args->filename));
}

// 统计调度延迟
tracepoint:sched:sched_switch {
    @start[args->prev_pid] = nsecs;
}
tracepoint:sched:sched_switch /args->prev_state == 0/ {
    if (@start[args->next_pid]) {
        @latency = hist(nsecs - @start[args->next_pid]);
        delete(@start[args->next_pid]);
    }
}
```

### 4.2 网络诊断

```bpftrace
// TCP 重传追踪
tracepoint:tcp:tcp_retransmit_skb {
    printf("Retransmit: %s:%d -> %s:%d seq=%u\n",
        saddr, sport, daddr, dport, args->seq);
    @retrans[comm] = count();
}

// TCP 连接建立延迟
kprobe:tcp_v4_connect {
    @start[tid] = nsecs;
}
kretprobe:tcp_v4_connect /@start[tid]/ {
    @connect_latency = hist(nsecs - @start[tid]);
    delete(@start[tid]);
}
```

### 4.3 文件系统 I/O

```bpftrace
// 文件 I/O 延迟分布
tracepoint:syscalls:sys_enter_read {
    @start[tid] = nsecs;
}
tracepoint:syscalls:sys_exit_read /@start[tid]/ {
    @read_latency = hist(nsecs - @start[tid]);
    delete(@start[tid]);
}

// 按进程统计 I/O
tracepoint:syscalls:sys_exit_read {
    @bytes[comm] = sum(args->ret);
}
```

### 4.4 容器级追踪

```bpftrace
// 按 cgroup 统计 CPU 使用
tracepoint:sched:sched_switch {
    @cpu[cgroup] = count();
}

// 容器 OOM 追踪
tracepoint:oom:oom_score_adj_update {
    printf("OOM: pid=%d comm=%s score=%d\n",
        args->pid, args->comm, args->oom_score_adj);
}
```

## 5. 生产实践

### 5.1 性能开销控制

| 工具 | 典型开销 | 优化建议 |
|------|----------|----------|
| Cilium Hubble | 1-3% CPU | 限制 flow 日志速率 |
| Pixie | 2-5% CPU | 调整采样率 |
| bpftrace | 可变 | 避免高频事件追踪 |

### 5.2 安全考虑

```yaml
# 限制 eBPF 程序加载（Pod Security）
apiVersion: v1
kind: Pod
metadata:
  name: ebpf-collector
spec:
  containers:
    - name: collector
      securityContext:
        privileged: true  # 或 capabilities: [BPF, PERFMON]
      volumeMounts:
        - name: sys
          mountPath: /sys
        - name: proc
          mountPath: /proc
  volumes:
    - name: sys
      hostPath:
        path: /sys
    - name: proc
      hostPath:
        path: /proc
```

### 5.3 与 OpenTelemetry 集成

```yaml
# OTel Collector 接收 eBPF 指标
receivers:
  prometheus:
    config:
      scrape_configs:
        - job_name: 'hubble'
          static_configs:
            - targets: ['hubble-relay:9965']
        - job_name: 'pixie'
          static_configs:
            - targets: ['pixie-metrics:8080']

exporters:
  otlp:
    endpoint: tempo:4317
  prometheus:
    endpoint: 0.0.0.0:8889
```

## 6. 故障排查

### 6.1 网络丢包

```bash
# Hubble 查看丢包
hubble observe --verdict DROPPED --namespace production

# bpftrace 追踪丢包原因
bpftrace -e '
tracepoint:skb:kfree_skb {
    printf("Drop: reason=%d location=%s\n", args->reason, probe);
    @drops[probe] = count();
}'
```

### 6.2 延迟分析

```bash
# Pixie 查看服务延迟
px.run_script("px/cluster/service_info", service="backend-api")

# bpftrace TCP 延迟
bpftrace -e '
kprobe:tcp_sendmsg { @start[tid] = nsecs; }
kretprobe:tcp_sendmsg /@start[tid]/ {
    @tcp_latency = hist(nsecs - @start[tid]);
    delete(@start[tid]);
}'
```

## Related

- [[09-可观测性/07-工具/index.md|工具索引]]
- [[09-可观测性/04-链路追踪/index.md|链路追踪]]
- [[05-网络/05-eBPF/index.md|eBPF 网络]]
- [[09-可观测性/02-指标/index.md|指标监控]]
