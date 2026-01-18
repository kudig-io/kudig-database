import os

footer = "\n\n---\n\n**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)"

tools_enrichment_2 = {
    '106-observability-tools.md': """# 106 - 全栈可观测性工具 (Observability Stack)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 可观测性三大支柱

| 支柱 (Pillar) | 核心工具 (Tools) | 数据类型 (Data Type) | 生产价值 |
|--------------|----------------|-------------------|---------|
| **Metrics (指标)** | Prometheus, VictoriaMetrics | 时序数据 | 性能监控、告警 |
| **Logs (日志)** | Loki, Elasticsearch | 文本流 | 故障排查、审计 |
| **Traces (链路)** | Jaeger, Tempo | 分布式追踪 | 性能分析、依赖图 |

## Prometheus 生产级部署

### 1. 高可用架构
```yaml
# Prometheus Operator 配置
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: k8s
  namespace: monitoring
spec:
  replicas: 2
  retention: 15d
  retentionSize: "50GB"
  storage:
    volumeClaimTemplate:
      spec:
        storageClassName: alicloud-disk-essd
        resources:
          requests:
            storage: 100Gi
  serviceMonitorSelector:
    matchLabels:
      prometheus: kube-prometheus
  resources:
    requests:
      memory: 4Gi
      cpu: 2
    limits:
      memory: 8Gi
      cpu: 4
```

### 2. 关键告警规则
```yaml
groups:
- name: kubernetes-critical
  rules:
  - alert: KubePodCrashLooping
    expr: rate(kube_pod_container_status_restarts_total[15m]) * 60 * 5 > 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Pod {{ $labels.pod }} 频繁重启"
      
  - alert: KubeNodeNotReady
    expr: kube_node_status_condition{condition="Ready",status="true"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "节点 {{ $labels.node }} NotReady"
```

## Grafana 仪表盘最佳实践

### 推荐 Dashboard ID
- **Kubernetes Cluster**: 7249
- **Node Exporter**: 1860
- **NGINX Ingress**: 9614
- **etcd**: 3070
- **GPU (DCGM)**: 12239

### 变量模板
```json
{
  "templating": {
    "list": [
      {
        "name": "namespace",
        "type": "query",
        "query": "label_values(kube_pod_info, namespace)"
      },
      {
        "name": "pod",
        "type": "query",
        "query": "label_values(kube_pod_info{namespace=~\"$namespace\"}, pod)"
      }
    ]
  }
}
```

## Loki 日志聚合

### 1. Promtail 采集配置
```yaml
scrape_configs:
  - job_name: kubernetes-pods
    kubernetes_sd_configs:
      - role: pod
    relabel_configs:
      - source_labels: [__meta_kubernetes_pod_label_app]
        target_label: app
      - source_labels: [__meta_kubernetes_namespace]
        target_label: namespace
      - source_labels: [__meta_kubernetes_pod_name]
        target_label: pod
```

### 2. LogQL 查询示例
```logql
# 查询错误日志
{namespace="production", app="myapp"} |= "ERROR"

# 统计错误率
rate({namespace="production"} |= "ERROR" [5m])

# JSON 日志解析
{app="api"} | json | level="error" | line_format "{{.message}}"
```

## OpenTelemetry 统一可观测

### Collector 配置
```yaml
receivers:
  otlp:
    protocols:
      grpc:
      http:

processors:
  batch:
    timeout: 10s
    send_batch_size: 1024

exporters:
  prometheus:
    endpoint: "0.0.0.0:8889"
  jaeger:
    endpoint: jaeger-collector:14250
  loki:
    endpoint: http://loki:3100/loki/api/v1/push

service:
  pipelines:
    traces:
      receivers: [otlp]
      processors: [batch]
      exporters: [jaeger]
    metrics:
      receivers: [otlp]
      processors: [batch]
      exporters: [prometheus]
    logs:
      receivers: [otlp]
      processors: [batch]
      exporters: [loki]
```

## 成本优化策略

| 策略 (Strategy) | 实施方式 (Implementation) | 节省比例 |
|----------------|--------------------------|---------|
| **采样率控制** | Traces 采样 10% | 90% 存储 |
| **日志过滤** | 丢弃 DEBUG 级别 | 70% 存储 |
| **指标降采样** | 长期存储降精度 | 50% 存储 |
| **数据分层** | 热数据 SSD, 冷数据 OSS | 60% 成本 |
""",

    '107-log-aggregation-tools.md': """# 107 - 日志聚合与分析工具 (Log Aggregation)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 日志方案技术选型

| 方案 (Solution) | 架构 (Architecture) | 成本 (Cost) | 查询性能 | 适用规模 |
|----------------|-------------------|------------|---------|---------|
| **Loki + Promtail** | 轻量级、索引简化 | 极低 | 中 | 中小型集群 |
| **ELK (Elasticsearch)** | 全文索引 | 高 | 高 | 大型企业 |
| **Fluentd + S3** | 归档存储 | 低 | 低 | 合规审计 |
| **阿里云 SLS** | 托管服务 | 中 | 高 | ACK 推荐 |

## Fluentd 生产级配置

### 1. DaemonSet 部署
```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: fluentd
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: fluentd
  template:
    metadata:
      labels:
        app: fluentd
    spec:
      serviceAccountName: fluentd
      tolerations:
      - key: node-role.kubernetes.io/control-plane
        effect: NoSchedule
      containers:
      - name: fluentd
        image: fluent/fluentd-kubernetes-daemonset:v1.16-debian-elasticsearch7
        env:
        - name: FLUENT_ELASTICSEARCH_HOST
          value: "elasticsearch.logging.svc.cluster.local"
        - name: FLUENT_ELASTICSEARCH_PORT
          value: "9200"
        resources:
          limits:
            memory: 500Mi
          requests:
            cpu: 100m
            memory: 200Mi
        volumeMounts:
        - name: varlog
          mountPath: /var/log
          readOnly: true
        - name: containers
          mountPath: /var/lib/docker/containers
          readOnly: true
      volumes:
      - name: varlog
        hostPath:
          path: /var/log
      - name: containers
        hostPath:
          path: /var/lib/docker/containers
```

### 2. 日志解析与过滤
```conf
<filter kubernetes.**>
  @type parser
  key_name log
  <parse>
    @type json
    time_key time
    time_format %Y-%m-%dT%H:%M:%S.%NZ
  </parse>
</filter>

<filter kubernetes.**>
  @type grep
  <exclude>
    key level
    pattern /^(DEBUG|TRACE)$/
  </exclude>
</filter>
```

## Filebeat 轻量级采集

### 1. 容器日志采集
```yaml
filebeat.inputs:
- type: container
  paths:
    - /var/log/containers/*.log
  processors:
    - add_kubernetes_metadata:
        host: ${NODE_NAME}
        matchers:
        - logs_path:
            logs_path: "/var/log/containers/"

output.elasticsearch:
  hosts: ["elasticsearch:9200"]
  index: "k8s-logs-%{+yyyy.MM.dd}"
```

### 2. 多行日志合并
```yaml
filebeat.inputs:
- type: log
  paths:
    - /var/log/app/*.log
  multiline.pattern: '^\\d{4}-\\d{2}-\\d{2}'
  multiline.negate: true
  multiline.match: after
```

## Vector 高性能日志路由

### 配置示例
```toml
[sources.kubernetes_logs]
type = "kubernetes_logs"

[transforms.parse_json]
type = "remap"
inputs = ["kubernetes_logs"]
source = '''
. = parse_json!(.message)
'''

[sinks.loki]
type = "loki"
inputs = ["parse_json"]
endpoint = "http://loki:3100"
encoding.codec = "json"
labels.namespace = "{{ kubernetes.namespace }}"
labels.pod = "{{ kubernetes.pod_name }}"
```

## 日志分析最佳实践

| 实践 (Practice) | 说明 (Description) |
|----------------|-------------------|
| **结构化日志** | 使用 JSON 格式输出 |
| **统一时间戳** | ISO 8601 格式 |
| **关联 ID** | Trace ID / Request ID |
| **敏感信息脱敏** | 密码、Token 打码 |
| **日志分级** | ERROR/WARN/INFO/DEBUG |
| **保留策略** | 热数据 7d, 冷数据 90d |
""",

    '108-troubleshooting-tools.md': """# 108 - 故障排查增强工具 (Troubleshooting Tools)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 诊断工具箱

| 工具 (Tool) | 类型 (Type) | 核心场景 (Scenario) | 学习曲线 |
|------------|-----------|-------------------|---------|
| **K9s** | TUI 界面 | 实时集群管理 | 低 |
| **Netshoot** | 调试镜像 | 网络连通性诊断 | 低 |
| **kubectl-debug** | kubectl 插件 | 注入临时容器 | 中 |
| **Telepresence** | 本地调试 | 本地连接远程集群 | 中 |
| **Stern** | 日志聚合 | 多 Pod 日志查看 | 低 |

## K9s 高效运维

### 快捷键速查
| 快捷键 | 功能 |
|-------|------|
| `:pod` | 切换到 Pod 视图 |
| `/` | 过滤资源 |
| `d` | 查看详情 (describe) |
| `l` | 查看日志 |
| `s` | 进入 Shell |
| `Ctrl-D` | 删除资源 |
| `?` | 帮助 |

### 自定义视图
```yaml
# ~/.k9s/views.yml
k9s:
  views:
    v1/pods:
      columns:
        - AGE
        - NAMESPACE
        - NAME
        - READY
        - STATUS
        - RESTARTS
        - CPU
        - MEM
```

## Netshoot 网络诊断

### 快速启动
```bash
# 临时 Pod
kubectl run netshoot --rm -it --image=nicolaka/netshoot -- bash

# Ephemeral Container (v1.25+)
kubectl debug -it <pod-name> --image=nicolaka/netshoot --target=<container-name>
```

### 常用诊断命令
```bash
# DNS 测试
nslookup kubernetes.default
dig @10.96.0.10 myservice.default.svc.cluster.local

# 连通性测试
ping <pod-ip>
nc -zv <service-name> <port>
curl -v http://<service>:<port>

# 路由追踪
traceroute <target-ip>
mtr <target-ip>

# 抓包分析
tcpdump -i eth0 -nn port 80 -w capture.pcap
```

## kubectl-debug 深度调试

### 安装与使用
```bash
# 安装插件
kubectl krew install debug

# 调试 Distroless 镜像
kubectl debug <pod-name> -it --image=busybox --target=<container-name>

# 调试节点
kubectl debug node/<node-name> -it --image=ubuntu
```

### 调试场景
- **无 Shell 镜像**: Distroless, Scratch
- **节点问题**: 文件系统、内核参数
- **网络问题**: iptables 规则、路由表

## Stern 多 Pod 日志

### 实时查看
```bash
# 查看所有 Pod
stern -n production myapp

# 正则匹配
stern -n production "^myapp-.*"

# 多命名空间
stern --all-namespaces -l app=nginx

# 输出 JSON
stern myapp --output json

# 时间范围
stern myapp --since 1h
```

## Telepresence 本地调试

### 拦截远程服务
```bash
# 连接集群
telepresence connect

# 拦截服务流量到本地
telepresence intercept myapp --port 8080:80

# 本地运行服务
./myapp --port 8080
```

## 故障排查流程

```mermaid
graph TD
    A[发现问题] --> B{资源层面?}
    B -->|Pod| C[kubectl describe pod]
    B -->|Node| D[kubectl describe node]
    B -->|Service| E[kubectl get endpoints]
    C --> F[查看 Events]
    C --> G[查看 Logs]
    D --> H[检查 Conditions]
    E --> I[检查 Endpoints]
    F --> J[定位根因]
    G --> J
    H --> J
    I --> J
```
""",

    '109-performance-profiling-tools.md': """# 109 - 性能分析与调优工具 (Performance Profiling)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 性能分析工具矩阵

| 工具 (Tool) | 分析对象 (Target) | 技术栈 (Tech) | 适用场景 |
|------------|-----------------|--------------|---------|
| **Inspektor Gadget** | 系统调用、网络 | eBPF | 内核级性能分析 |
| **Pyroscope** | 应用代码 | 持续 Profiling | CPU/内存热点 |
| **Pixie** | 全栈可观测 | eBPF | 零侵入性监控 |
| **Kperf** | API Server | 压测 | 控制平面性能 |
| **pprof** | Go 应用 | 原生 | Go 程序优化 |

## Inspektor Gadget eBPF 分析

### 1. 安装与使用
```bash
# 安装
kubectl gadget deploy

# 查看可用 Gadgets
kubectl gadget list

# 追踪系统调用
kubectl gadget trace exec -n production

# 网络延迟分析
kubectl gadget trace tcpconnect -n production

# 文件 I/O 分析
kubectl gadget trace open -n production
```

### 2. 常用场景
```bash
# 查找 CPU 占用高的进程
kubectl gadget top ebpf

# DNS 查询追踪
kubectl gadget trace dns -n production

# OOM 事件监控
kubectl gadget trace oomkill
```

## Pyroscope 持续性能分析

### 1. Agent 部署
```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  template:
    spec:
      containers:
      - name: myapp
        image: myapp:v1.0
        env:
        - name: PYROSCOPE_SERVER_ADDRESS
          value: "http://pyroscope:4040"
        - name: PYROSCOPE_APPLICATION_NAME
          value: "myapp"
```

### 2. 火焰图分析
- **CPU Profile**: 识别计算密集函数
- **Heap Profile**: 内存分配热点
- **Goroutine Profile**: 并发瓶颈

### 3. 对比分析
```bash
# 对比两个版本性能
pyroscope compare \
  --baseline myapp{version="v1.0"} \
  --comparison myapp{version="v1.1"}
```

## Kperf API Server 压测

### 压测命令
```bash
# 安装
go install sigs.k8s.io/kperf@latest

# 压测 Pod 创建
kperf pod --replicas 100 --duration 60s

# 压测 Service 创建
kperf service --replicas 50 --duration 30s

# 自定义 QPS
kperf pod --replicas 100 --qps 50
```

### 分析指标
- **P50/P95/P99 延迟**
- **吞吐量 (QPS)**
- **错误率**

## pprof Go 应用分析

### 1. 启用 pprof
```go
import _ "net/http/pprof"

func main() {
    go func() {
        log.Println(http.ListenAndServe("localhost:6060", nil))
    }()
    // 应用逻辑
}
```

### 2. 采集 Profile
```bash
# CPU Profile
go tool pprof http://localhost:6060/debug/pprof/profile?seconds=30

# Heap Profile
go tool pprof http://localhost:6060/debug/pprof/heap

# Goroutine Profile
go tool pprof http://localhost:6060/debug/pprof/goroutine
```

### 3. 分析命令
```bash
# 进入交互模式
(pprof) top10
(pprof) list <function-name>
(pprof) web  # 生成火焰图
```

## 性能调优最佳实践

| 实践 (Practice) | 说明 (Description) |
|----------------|-------------------|
| **持续 Profiling** | 生产环境常态化采集 |
| **基线对比** | 版本间性能回归检测 |
| **资源限制** | 合理设置 CPU/Memory Limits |
| **并发优化** | 调整 Worker 数量 |
| **缓存策略** | 减少重复计算 |
| **异步处理** | 非关键路径异步化 |
""",

    '110-cli-enhancement-tools.md': """# 110 - CLI 增强与效率工具 (CLI Enhancement)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## CLI 效率提升工具

| 工具 (Tool) | 核心功能 (Function) | 效率提升 | 安装方式 |
|------------|-------------------|---------|---------|
| **kubectx / kubens** | 快速切换上下文/命名空间 | 90% | brew/apt |
| **kube-capacity** | 资源容量查看 | 80% | kubectl krew |
| **Stern** | 多 Pod 日志聚合 | 85% | brew/apt |
| **kubectl-tree** | 资源依赖树 | 70% | kubectl krew |
| **kubectl-neat** | 清理 YAML 输出 | 75% | kubectl krew |

## kubectx / kubens 快速切换

### 基本用法
```bash
# 列出所有上下文
kubectx

# 切换上下文
kubectx production

# 切换回上一个上下文
kubectx -

# 列出所有命名空间
kubens

# 切换命名空间
kubens kube-system
```

### 别名配置
```bash
# ~/.bashrc 或 ~/.zshrc
alias kx='kubectx'
alias kn='kubens'
```

## kube-capacity 资源余量

### 查看集群容量
```bash
# 查看所有节点
kube-capacity

# 按节点分组
kube-capacity --sort cpu.util

# 查看 Pod 级别
kube-capacity --pods

# 输出 JSON
kube-capacity -o json
```

### 输出示例
```
NODE              CPU REQUESTS   CPU LIMITS    MEMORY REQUESTS   MEMORY LIMITS
node-1            1950m (48%)    3900m (97%)   7Gi (43%)         14Gi (87%)
node-2            1200m (30%)    2400m (60%)   5Gi (31%)         10Gi (62%)
```

## kubectl-tree 资源依赖

### 查看资源树
```bash
# 查看 Deployment 依赖
kubectl tree deployment myapp

# 查看 StatefulSet 依赖
kubectl tree statefulset mysql

# 输出示例
NAMESPACE  NAME                           READY  REASON  AGE
default    Deployment/myapp               -              5d
default    ├─ReplicaSet/myapp-7d8f9c      -              5d
default    │ ├─Pod/myapp-7d8f9c-abc       True           5d
default    │ └─Pod/myapp-7d8f9c-def       True           5d
```

## kubectl-neat 清理输出

### 清理 YAML
```bash
# 清理 managedFields 等冗余字段
kubectl get pod myapp -o yaml | kubectl neat

# 清理并保存
kubectl get deployment myapp -o yaml | kubectl neat > myapp-clean.yaml
```

## kubectl 别名与函数

### 常用别名
```bash
# ~/.bashrc 或 ~/.zshrc
alias k='kubectl'
alias kg='kubectl get'
alias kd='kubectl describe'
alias kdel='kubectl delete'
alias kl='kubectl logs'
alias kex='kubectl exec -it'
alias kaf='kubectl apply -f'

# 快速查看 Pod
alias kgp='kubectl get pods'
alias kgpa='kubectl get pods --all-namespaces'

# 快速查看 Service
alias kgs='kubectl get svc'

# 快速查看 Node
alias kgn='kubectl get nodes'
```

### 实用函数
```bash
# 快速进入 Pod Shell
ksh() {
  kubectl exec -it $1 -- /bin/bash
}

# 快速查看 Pod 日志
klog() {
  kubectl logs -f $1
}

# 快速删除 Evicted Pod
kdele() {
  kubectl get pods --all-namespaces | grep Evicted | awk '{print $2, "-n", $1}' | xargs kubectl delete pod
}
```

## kubectl 插件管理 (Krew)

### 安装 Krew
```bash
(
  set -x; cd "$(mktemp -d)" &&
  OS="$(uname | tr '[:upper:]' '[:lower:]')" &&
  ARCH="$(uname -m | sed -e 's/x86_64/amd64/' -e 's/\\(arm\\)\\(64\\)\\?.*/\\1\\2/' -e 's/aarch64$/arm64/')" &&
  KREW="krew-${OS}_${ARCH}" &&
  curl -fsSLO "https://github.com/kubernetes-sigs/krew/releases/latest/download/${KREW}.tar.gz" &&
  tar zxvf "${KREW}.tar.gz" &&
  ./"${KREW}" install krew
)
```

### 推荐插件
```bash
kubectl krew install ctx        # kubectx
kubectl krew install ns         # kubens
kubectl krew install tree       # 资源树
kubectl krew install neat       # YAML 清理
kubectl krew install capacity   # 容量查看
kubectl krew install debug      # 调试工具
kubectl krew install tail       # 日志追踪
```

## 效率提升技巧

| 技巧 (Tip) | 说明 (Description) |
|-----------|-------------------|
| **自动补全** | `source <(kubectl completion bash)` |
| **别名缩写** | 减少 80% 输入 |
| **插件生态** | Krew 插件市场 |
| **上下文管理** | kubectx 快速切换 |
| **资源模板** | 保存常用 YAML 模板 |
""",
}

# Write Tier 1 remaining files
for filename, content in tools_enrichment_2.items():
    path = os.path.join('tables', filename)
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content + footer)

print(f"Enriched {len(tools_enrichment_2)} tools tables (106-110).")
