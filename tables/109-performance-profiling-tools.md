# 性能分析工具

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [Pixie](https://pixielabs.ai/) | [Pyroscope](https://pyroscope.io/)

## 工具对比

| 工具 | 类型 | 数据采集 | 开销 | 语言支持 | 可视化 | K8s集成 | 生产推荐 |
|------|------|---------|------|---------|--------|---------|---------|
| **Pixie** | 全栈观测 | eBPF | ⭐⭐⭐⭐⭐ | 全语言 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 强烈推荐 |
| **Pyroscope** | 持续性能分析 | SDK | ⭐⭐⭐⭐ | 多语言 | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 推荐 |
| **Parca** | 持续性能分析 | eBPF | ⭐⭐⭐⭐⭐ | 全语言 | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 推荐 |
| **Grafana Phlare** | 持续性能分析 | SDK | ⭐⭐⭐⭐ | 多语言 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | Grafana栈 |
| **perf** | 系统性能 | 内核 | ⭐⭐⭐ | 全语言 | ⭐⭐ | ⭐⭐ | 底层分析 |
| **flamegraph** | 可视化 | 后处理 | N/A | 全语言 | ⭐⭐⭐⭐⭐ | ⭐⭐ | 火焰图 |

---

## Pixie - 零侵入全栈观测

### 核心特性

```
Pixie自动采集数据
├── HTTP/HTTPS请求(延迟、状态码)
├── DNS查询(解析时间、记录)
├── MySQL/PostgreSQL查询
├── Redis命令
├── Kafka消息
├── gRPC调用
└── JVM指标(无需SDK)
```

### 安装部署

```bash
# 安装Pixie CLI
bash -c "$(curl -fsSL https://withpixie.ai/install.sh)"

# 注册账号并获取Deploy Key
px auth login

# 部署到集群
px deploy --cluster_name=prod-cluster

# 验证安装
px get viziers

# 访问UI
px live
```

### Pixie查询语言(PxL)示例

#### 1. HTTP请求分析

```python
# HTTP请求延迟P99
import px

df = px.DataFrame('http_events')
df = df[df.ctx['namespace'] == 'production']
df = df.groupby(['service', 'endpoint']).agg(
    latency_p99=('latency_ms', px.quantiles(0.99)),
    error_rate=('resp_status', lambda x: (x >= 400).mean()),
    rps=('time_', px.count)
)
px.display(df, 'http_summary')
```

#### 2. 数据库慢查询

```python
# MySQL慢查询(>100ms)
import px

df = px.DataFrame('mysql_events')
df = df[df.latency_ms > 100]
df = df[df.ctx['namespace'] == 'production']
df = df.groupby('query_text').agg(
    count=('query_text', px.count),
    avg_latency=('latency_ms', px.mean),
    max_latency=('latency_ms', px.max)
)
df = df.sort_values('max_latency', ascending=False)
px.display(df.head(10), 'slow_queries')
```

#### 3. 服务依赖拓扑

```python
# 服务调用拓扑图
import px

df = px.DataFrame('http_events')
df = df[df.ctx['namespace'] == 'production']
df = df.groupby(['source_service', 'dest_service']).agg(
    request_count=('time_', px.count),
    error_count=('resp_status', lambda x: (x >= 400).sum()),
    avg_latency=('latency_ms', px.mean)
)
px.display(df, 'service_graph')
```

#### 4. Pod网络流量

```python
# Pod流入/流出流量
import px

df = px.DataFrame('conn_stats')
df = df[df.ctx['namespace'] == 'production']
df = df.groupby(['pod', 'remote_addr']).agg(
    bytes_sent=('bytes_sent', px.sum),
    bytes_recv=('bytes_recv', px.sum),
    conn_count=('conn_open', px.count)
)
px.display(df, 'network_traffic')
```

### Pixie Dashboard 示例

```yaml
# pixie-dashboard.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: pixie-dashboards
data:
  http-overview.json: |
    {
      "title": "HTTP Overview",
      "panels": [
        {
          "title": "Request Rate",
          "pxl_script": "import px; df = px.DataFrame('http_events'); px.display(df.groupby('time_').agg(rps=('time_', px.count)))"
        }
      ]
    }
```

---

## Pyroscope - 持续性能分析

### 架构部署

```
┌──────────────────────────────────────┐
│        Applications                  │
│  ┌────────┐  ┌────────┐  ┌────────┐ │
│  │  Go    │  │ Python │  │  Java  │ │
│  │ +SDK   │  │  +SDK  │  │  +SDK  │ │
│  └───┬────┘  └────┬───┘  └────┬───┘ │
└──────┼────────────┼─────────────┼────┘
       │            │             │
       └────────────┴─────────────┘
                    │ push profiles
                    v
        ┌───────────────────────┐
        │  Pyroscope Server     │
        │  (存储+查询+可视化)    │
        └───────────────────────┘
```

### Helm 安装

```bash
helm repo add pyroscope-io https://pyroscope-io.github.io/helm-chart
helm install pyroscope pyroscope-io/pyroscope \
  -n monitoring \
  --set persistence.enabled=true \
  --set persistence.size=50Gi \
  --set resources.requests.memory=2Gi \
  --set resources.limits.memory=4Gi
```

### 应用集成

#### Go应用

```go
package main

import (
    "github.com/pyroscope-io/client/pyroscope"
)

func main() {
    pyroscope.Start(pyroscope.Config{
        ApplicationName: "myapp",
        ServerAddress:   "http://pyroscope.monitoring:4040",
        Logger:          pyroscope.StandardLogger,
        
        // CPU分析
        ProfileTypes: []pyroscope.ProfileType{
            pyroscope.ProfileCPU,
            pyroscope.ProfileAllocObjects,
            pyroscope.ProfileAllocSpace,
            pyroscope.ProfileInuseObjects,
            pyroscope.ProfileInuseSpace,
        },
        
        // 标签
        Tags: map[string]string{
            "env":     "production",
            "version": "v1.2.3",
        },
    })
    
    // 应用代码
    // ...
}
```

#### Python应用

```python
import pyroscope

pyroscope.configure(
    application_name="myapp",
    server_address="http://pyroscope.monitoring:4040",
    tags={
        "env": "production",
        "version": "v1.2.3"
    }
)

# 应用代码
# ...
```

#### Java应用

```bash
# 启动时添加agent
java -javaagent:pyroscope.jar \
  -Dpyroscope.application.name=myapp \
  -Dpyroscope.server.address=http://pyroscope.monitoring:4040 \
  -Dpyroscope.format=jfr \
  -jar myapp.jar
```

### Kubernetes自动注入(Operator)

```yaml
apiVersion: pyroscope.io/v1alpha1
kind: PyroscopeProfile
metadata:
  name: myapp-profile
  namespace: production
spec:
  selector:
    matchLabels:
      app: myapp
  profileConfig:
    samplingRate: 100  # Hz
    uploadInterval: 10s
    tags:
      env: production
```

---

## Parca - eBPF持续性能分析

### 安装部署

```bash
# 使用Helm安装
helm repo add parca https://parca-dev.github.io/helm-charts
helm install parca parca/parca \
  -n monitoring \
  --set agent.enabled=true \
  --set agent.mode=daemonset \
  --set server.persistence.enabled=true
```

### Parca Agent 配置

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: parca-agent
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: parca-agent
  template:
    metadata:
      labels:
        app: parca-agent
    spec:
      hostPID: true
      containers:
        - name: parca-agent
          image: ghcr.io/parca-dev/parca-agent:v0.25.0
          args:
            - /bin/parca-agent
            - --node=$(NODE_NAME)
            - --remote-store-address=parca-server:7070
            - --remote-store-insecure
            - --log-level=info
          env:
            - name: NODE_NAME
              valueFrom:
                fieldRef:
                  fieldPath: spec.nodeName
          securityContext:
            privileged: true
          volumeMounts:
            - mountPath: /sys/kernel/debug
              name: debugfs
            - mountPath: /sys/fs/bpf
              name: bpffs
      volumes:
        - name: debugfs
          hostPath:
            path: /sys/kernel/debug
        - name: bpffs
          hostPath:
            path: /sys/fs/bpf
```

### PromQL查询分析数据

```promql
# CPU使用热点函数
topk(10,
  sum by (function_name) (
    parca_agent_cpu_samples_total{namespace="production"}
  )
)

# 特定服务CPU Profile
parca_agent_cpu_samples_total{
  namespace="production",
  pod=~"myapp-.*"
}
```

---

## perf + FlameGraph 经典组合

### perf性能采集

```bash
# 进入Pod容器
kubectl exec -it myapp-pod-xxxx -- /bin/bash

# 安装perf(如果不存在)
apt-get update && apt-get install -y linux-perf

# CPU性能采集(60秒)
perf record -F 99 -a -g -- sleep 60

# 查看报告
perf report

# 导出数据
perf script > perf.out
```

### 生成火焰图

```bash
# 克隆FlameGraph工具
git clone https://github.com/brendangregg/FlameGraph
cd FlameGraph

# 折叠perf数据
./stackcollapse-perf.pl perf.out > perf.folded

# 生成火焰图
./flamegraph.pl perf.folded > flamegraph.svg

# 从Kubernetes Pod复制文件
kubectl cp myapp-pod-xxxx:/tmp/flamegraph.svg ./flamegraph.svg
```

### 使用kubectl-flame自动化

```bash
# 安装kubectl-flame
kubectl krew install flame

# 生成Go应用火焰图
kubectl flame myapp-pod-xxxx \
  --namespace production \
  --lang go \
  --duration 60s

# 生成Java应用火焰图
kubectl flame myapp-pod-xxxx \
  --namespace production \
  --lang java \
  --duration 60s \
  --image=jvm-profiler:latest
```

---

## Grafana Phlare (现为Pyroscope)

### 集成到Grafana LGTM Stack

```yaml
# Grafana数据源配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: grafana-datasources
  namespace: monitoring
data:
  datasources.yaml: |
    apiVersion: 1
    datasources:
      - name: Pyroscope
        type: phlare
        access: proxy
        url: http://pyroscope:4040
        jsonData:
          minStep: 15s
```

### 关联Traces + Profiles

```yaml
# Tempo配置(关联Trace到Profile)
apiVersion: v1
kind: ConfigMap
metadata:
  name: tempo-config
data:
  tempo.yaml: |
    overrides:
      metrics_generator_processors:
        - service-graphs
        - span-metrics
      
      # 关联到Pyroscope
      metrics_generator_external_labels:
        pyroscope: http://pyroscope:4040
```

---

## 性能分析最佳实践

### 1. CPU热点分析流程

```bash
# Step 1: 发现CPU使用率高的Pod
kubectl top pod -n production --sort-by=cpu

# Step 2: 使用Pixie快速定位
px run px/http_data -n production

# Step 3: 持续性能分析
kubectl flame myapp-pod-xxxx --lang go --duration 60s

# Step 4: 深度分析
kubectl exec -it myapp-pod-xxxx -- perf record -F 99 -g -p $(pidof myapp) -- sleep 30
```

### 2. 内存泄漏分析

```bash
# Go应用 - pprof
kubectl port-forward myapp-pod-xxxx 6060:6060
go tool pprof http://localhost:6060/debug/pprof/heap

# Java应用 - jmap
kubectl exec myapp-pod-xxxx -- jmap -dump:format=b,file=/tmp/heap.hprof $(pidof java)
kubectl cp myapp-pod-xxxx:/tmp/heap.hprof ./heap.hprof

# Python应用 - memray
kubectl exec myapp-pod-xxxx -- python -m memray run --output profile.bin app.py
```

### 3. I/O性能分析

```bash
# 使用Inspektor Gadget
kubectl gadget top block-io --namespace production
kubectl gadget trace fsslower --namespace production --min-latency 10ms

# 使用Pixie
px run px/disk_io_stats -n production
```

---

## 性能分析对比

| 工具 | 适用场景 | 数据保留 | 开销 | 实时性 |
|------|---------|---------|------|--------|
| **Pixie** | 全栈快速诊断 | 短期(小时) | <5% | 实时 |
| **Pyroscope** | 长期CPU/内存趋势 | 长期(月) | 2-5% | 近实时 |
| **Parca** | 自动化CPU分析 | 长期(月) | <2% | 近实时 |
| **perf** | 底层内核分析 | 手动 | 5-10% | 实时 |

---

## 告警集成

```yaml
# Prometheus告警规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: performance-alerts
spec:
  groups:
    - name: cpu-hotspot
      rules:
        - alert: HighCPUFunction
          expr: |
            topk(1,
              sum by (function_name) (
                rate(parca_agent_cpu_samples_total[5m])
              )
            ) > 1000
          for: 10m
          annotations:
            summary: "检测到CPU热点函数"
            description: "函数 {{ $labels.function_name }} CPU占用过高"
```

---

## 工具选型建议

| 场景 | 推荐方案 | 原因 |
|------|---------|------|
| **快速问题定位** | Pixie | 零配置、实时、全栈 |
| **持续性能监控** | Pyroscope | 长期存储、趋势分析 |
| **自动化分析** | Parca | eBPF、无SDK |
| **深度内核分析** | perf + FlameGraph | 最底层、最准确 |
| **Grafana用户** | Pyroscope(前Phlare) | 统一可视化 |

---

## 常见问题

**Q: 性能分析会影响应用性能吗?**  
A: Pixie/Parca开销<5%，Pyroscope 2-5%，perf可配置采样率控制开销。

**Q: 如何分析生产环境?**  
A: 优先使用eBPF工具(Pixie/Parca)，无需修改代码，开销低。

**Q: 火焰图如何阅读?**  
A: 横轴=样本占比，纵轴=调用栈深度，宽度大的函数是热点。
