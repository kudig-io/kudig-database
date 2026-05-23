---
title: 节点监控 — metrics-server / node-exporter / kubelet metrics
description: 'title: 节点监控 metrics-server node-exporter kubelet metrics'
category: general
tags:
- reference
- monitoring
- etcd
- apiserver
- kubelet
- prometheus
- grafana
- coredns
- docker
- opa
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 节点监控 — metrics-server / node-exporter / kubelet metrics 是什么
- 如何 节点监控 — metrics-server / node-exporter / kubelet metrics
- Kubernetes 07 platform engineering 最佳实践
trigger_keywords:
- 节点监控
- metrics-server
- node-exporter
- kubelet
- metrics
- platform
- engineering
- code
prerequisites:
- kubectl-basics
- platform-engineering-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- policy-basics
created: "2026-05-23"
---

title: 节点监控 metrics-server node-exporter kubelet metrics
description: '# 节点监控 — metrics-server / node-exporter / kubelet metrics'
category: functions
tags:
- k8s
- operations
- cluster-management
- etcd
- apiserver
- kubelet
- prometheus
- grafana
- coredns
- docker
last_updated: 2026-05-18
difficulty: intermediate
reading_level: intermediate
audience:
- Kubernetes 运维工程师
- SRE 工程师
- 监控工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes node monitoring setup
- kubelet metrics prometheus scrape
- metrics-server kubectl top
- node-exporter prometheus deployment
- node monitoring dashboard grafana
trigger_keywords:
- monitoring
- metrics
- metrics-server
- node-exporter
- kubelet metrics
- cadvisor
- prometheus
- grafana
- kubectl top
- dashboard
- node_cpu
- node_memory
- kubelet_pod_worker_duration
- eviction
related_domains:
- domain-01-cluster-fundamentals
- domain-10-observability
related_topics:
- node-create/11-eviction
- node-create/08-troubleshooting
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

# 节点监控 — metrics-server / node-exporter / kubelet metrics

## 概述

节点监控是 Kubernetes 集群可观测性的核心组成部分。有效的节点监控能够帮助运维团队及时发现资源瓶颈、预测容量需求、定位性能问题，从而保障集群的稳定运行。

Kubernetes 节点监控主要依赖三个层面的数据源：kubelet 内置的 cAdvisor 指标、metrics-server 提供的资源使用聚合、以及 node-exporter 采集的主机级指标。这三者各有侧重，互补配合，构成了完整的节点监控体系。

- **kubelet metrics**：提供容器级别的 CPU、内存、网络、磁盘 I/O 指标，基于 cAdvisor 实现，是最底层的监控数据源
- **metrics-server**：聚合 kubelet 的数据，为 `kubectl top` 和 HPA（Horizontal Pod Autoscaler）提供数据支撑
- **node-exporter**：Prometheus 生态的主机级监控采集器，提供 CPU、内存、磁盘、网络、文件系统等操作系统级别的指标

本文档详细分析这三个组件的部署配置、核心指标、使用方法和故障排查。

---

## 源码路径

| 组件 | 源码路径 | 说明 |
|------|---------|------|
| kubelet metrics | `pkg/kubelet/metrics/` | kubelet 内置指标 |
| cAdvisor 集成 | `pkg/kubelet/cadvisor/` | 容器指标采集 |
| stats API | `pkg/kubelet/server/stats/` | 统计信息 API |
| metrics-server | `kubernetes-sigs/metrics-server` | 资源指标 API |
| node-exporter | `prometheus/node_exporter` | 主机指标采集 |

---

## 一、kubelet Metrics

### 1.1 kubelet metrics 端点

kubelet 在 10250 端口上暴露了多个 metrics 端点，提供不同维度的监控数据：

| 端点 | 协议 | 说明 | 认证要求 |
|------|------|------|---------|
| `/metrics` | HTTPS | kubelet 自身指标 | 需要证书 |
| `/metrics/cadvisor` | HTTPS | 容器指标（cAdvisor） | 需要证书 |
| `/metrics/probes` | HTTPS | 健康检查指标 | 需要证书 |
| `/metrics/resource` | HTTPS | 资源指标（metrics-server 数据源） | 需要证书 |
| `/stats/summary` | HTTPS | 统计摘要（JSON 格式） | 需要证书 |
| `/healthz` | HTTPS | 健康检查 | 不需要认证 |

### 1.2 访问 kubelet metrics

```bash
# 通过 kubelet API 直接访问 (需要 kubelet 客户端证书)
curl -k --cert /var/lib/kubelet/pki/kubelet-client-current.pem \
     --key /var/lib/kubelet/pki/kubelet-client-current.pem \
     https://localhost:10250/metrics

# 通过 kubectl proxy 访问 (推荐)
kubectl proxy --port=8001 &
curl http://localhost:8001/api/v1/nodes/<node-name>/proxy/metrics

# 使用 kubectl top 间接获取
kubectl top nodes
kubectl top pods --all-namespaces
```

### 1.3 核心 kubelet 指标详解

#### 容器运行时指标

```bash
# 容器运行时操作
kubelet_runtime_operations_total{operation_type="container_status"} 1234
kubelet_runtime_operations_errors_total{operation_type="container_status"} 5
# operation_type: container_status, container_create, container_remove, image_pull, image_pulls

# 容器操作延迟
kubelet_runtime_operations_duration_seconds{operation_type="container_status"}
```

#### Pod 管理指标

```bash
# Pod worker 处理时间
kubelet_pod_worker_duration_seconds_bucket{operation="sync"}
kubelet_pod_worker_duration_seconds_count{operation="sync"}

# Pod 启动耗时（从创建到 Running 的时间）
kubelet_pod_start_duration_seconds_bucket
kubelet_pod_start_duration_seconds_count

# Pod 状态统计
kubelet_running_pods 42                # 当前运行的 Pod 数量
kubelet_running_containers 85          # 当前运行的容器数量
```

#### 卷管理指标

```bash
# 卷统计
kubelet_volume_stats_capacity_bytes{volume="pvc-xxx"} 10737418240
kubelet_volume_stats_used_bytes{volume="pvc-xxx"} 5368709120
kubelet_volume_stats_available_bytes{volume="pvc-xxx"} 5368709120
kubelet_volume_stats_inodes{volume="pvc-xxx"} 6553600
kubelet_volume_stats_inodesFree{volume="pvc-xxx"} 6553000
kubelet_volume_stats_inodesUsed{volume="pvc-xxx"} 600
```

#### 驱逐指标

```bash
# 驱逐统计
kubelet_node_controller_evictions_total 3         # 节点控制器触发的驱逐次数
kubelet_evictions_total{signal="memory.available"} 2  # kubelet 驱逐次数（按信号分类）
```

### 1.4 cAdvisor 指标

cAdvisor（Container Advisor）集成在 kubelet 中，提供容器级别的资源使用指标：

```bash
# 访问 cadvisor metrics
curl -k https://localhost:10250/metrics/cadvisor

# 核心指标:
container_cpu_usage_seconds_total          # CPU 使用量（累计）
container_cpu_load_average_10s             # CPU 负载
container_memory_working_set_bytes         # 内存使用量（含缓存）
container_memory_rss                       # RSS 内存
container_memory_cache                     # 缓存内存
container_memory_swap                      # Swap 使用量
container_network_receive_bytes_total      # 网络接收字节数
container_network_transmit_bytes_total     # 网络发送字节数
container_fs_usage_bytes                   # 文件系统使用量
container_fs_limit_bytes                   # 文件系统总量
container_fs_inodes_free                   # 剩余 inode 数
```

---

## 二、metrics-server

### 2.1 概述

metrics-server 是 Kubernetes 集群核心指标管道（Core Metrics Pipeline）的实现。它从每个节点的 kubelet `/metrics/resource` 端点采集资源使用数据，通过 `metrics.k8s.io` API 在 API Server 中注册，供 `kubectl top` 和 HPA 使用。

```
指标采集流程:
  ┌──────────┐     ┌────────────────┐     ┌──────────────┐     ┌──────────┐
  │ kubelet  │────→│ metrics-server │────→│  API Server  │────→│ kubectl  │
  │ /metrics │     │  (Aggregator)  │     │ (Aggregated  │     │ top/HPA  │
  │ /resource│     │                │     │  API)        │     │          │
  └──────────┘     └────────────────┘     └──────────────┘     └──────────┘
```

### 2.2 部署 metrics-server

```bash
# 使用 kubectl apply 部署
kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml

# 如果使用 kubeadm，可能需要添加 --kubelet-insecure-tls
# (仅用于测试环境，生产环境应配置 kubelet 证书)
```

生产环境推荐配置：

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: metrics-server
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: metrics-server
        args:
        - --cert-dir=/tmp
        - --secure-port=4443
        - --kubelet-preferred-address-types=InternalIP,ExternalIP,Hostname
        - --kubelet-use-node-status-port
        - --metric-resolution=15s
        - --requestheader-client-ca-file=/etc/kubernetes/pki/front-proxy-ca.crt
        resources:
          limits:
            cpu: 200m
            memory: 200Mi
          requests:
            cpu: 100m
            memory: 100Mi
```

### 2.3 使用 kubectl top

```bash
# 查看节点资源使用
kubectl top nodes
# NAME       CPU(cores)   CPU%   MEMORY(bytes)   MEMORY%
# node-1     450m         11%    3200Mi          40%
# node-2     380m         9%     2800Mi          35%

# 查看 Pod 资源使用
kubectl top pods -n kube-system
# NAME                                 CPU(cores)   MEMORY(bytes)
# coredns-5d78c9869d-xxxxx             5m           25Mi
# etcd-node-1                          30m          150Mi
# kube-apiserver-node-1                80m          400Mi

# 查看特定 namespace 的 Pod
kubectl top pods -n production --sort-by=memory

# 查看所有 namespace
kubectl top pods --all-namespaces --sort-by=cpu

# 查看单个 Pod 的容器级指标
kubectl top pod <pod-name> --containers
```

### 2.4 metrics-server 与 HPA

metrics-server 为 HPA（Horizontal Pod Autoscaler）提供资源指标数据：

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: web-app
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: web-app
  minReplicas: 2
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
```

---

## 三、node-exporter

### 3.1 概述

node-exporter 是 Prometheus 生态中的主机级指标采集器。它通过 Linux 内核接口（`/proc`、`/sys`）采集操作系统的 CPU、内存、磁盘、网络、文件系统、硬件等指标。与 kubelet metrics 不同，node-exporter 关注的是主机级别的资源使用情况，而非容器级别。

### 3.2 部署 node-exporter

推荐使用 DaemonSet 方式部署，确保每个节点都运行一个 node-exporter 实例：

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-exporter
  namespace: monitoring
  labels:
    app: node-exporter
spec:
  selector:
    matchLabels:
      app: node-exporter
  template:
    metadata:
      labels:
        app: node-exporter
    spec:
      hostNetwork: true
      hostPID: true
      hostIPC: true
      tolerations:
      - operator: Exists
      containers:
      - name: node-exporter
        image: prom/node-exporter:v1.7.0
        args:
        - --path.procfs=/host/proc
        - --path.sysfs=/host/sys
        - --path.rootfs=/host/root
        - --collector.filesystem.mount-points-exclude=^/(dev|proc|sys|var/lib/docker/.+)($$|/)
        - --collector.filesystem.fs-types-exclude=^(autofs|binfmt_misc|cgroup|configfs|debugfs|devpts|devtmpfs|fusectl|hugetlbfs|mqueue|overlay|proc|procfs|sysfs|tmpfs|tracefs)$$
        ports:
        - containerPort: 9100
          hostPort: 9100
          name: metrics
        resources:
          limits:
            cpu: 200m
            memory: 100Mi
          requests:
            cpu: 100m
            memory: 50Mi
        volumeMounts:
        - name: proc
          mountPath: /host/proc
          readOnly: true
        - name: sys
          mountPath: /host/sys
          readOnly: true
        - name: root
          mountPath: /host/root
          mountPropagation: HostToContainer
          readOnly: true
      volumes:
      - name: proc
        hostPath:
          path: /proc
      - name: sys
        hostPath:
          path: /sys
      - name: root
        hostPath:
          path: /
```

### 3.3 核心 node-exporter 指标

#### CPU 指标

```bash
# CPU 使用率
node_cpu_seconds_total{cpu="0", mode="idle"}        # CPU 空闲时间
node_cpu_seconds_total{cpu="0", mode="user"}        # 用户态时间
node_cpu_seconds_total{cpu="0", mode="system"}      # 内核态时间
node_cpu_seconds_total{cpu="0", mode="iowait"}      # I/O 等待时间

# CPU 核心数
node_cpu_seconds_total  # 按 mode 分组的 CPU 时间累计值
```

#### 内存指标

```bash
node_memory_MemTotal_bytes          # 总内存
node_memory_MemFree_bytes           # 空闲内存
node_memory_MemAvailable_bytes      # 可用内存（含可回收缓存）
node_memory_Buffers_bytes           # 缓冲区
node_memory_Cached_bytes            # 页缓存
node_memory_SwapTotal_bytes         # Swap 总量
node_memory_SwapFree_bytes          # 空闲 Swap
```

#### 磁盘与文件系统指标

```bash
# 磁盘 I/O
node_disk_read_bytes_total{device="sda"}       # 磁盘读取字节数
node_disk_written_bytes_total{device="sda"}    # 磁盘写入字节数
node_disk_io_time_seconds_total{device="sda"}  # 磁盘 I/O 时间

# 文件系统
node_filesystem_size_bytes{mountpoint="/"}       # 文件系统总量
node_filesystem_avail_bytes{mountpoint="/"}      # 文件系统可用空间
node_filesystem_files{mountpoint="/"}            # 文件系统 inode 总数
node_filesystem_files_free{mountpoint="/"}       # 文件系统空闲 inode
```

#### 网络指标

```bash
node_network_receive_bytes_total{device="eth0"}     # 网络接收字节数
node_network_transmit_bytes_total{device="eth0"}    # 网络发送字节数
node_network_receive_drop_total{device="eth0"}      # 接收丢包数
node_network_transmit_drop_total{device="eth0"}     # 发送丢包数
```

### 3.4 Prometheus 指标采集配置

```yaml
# Prometheus scrape 配置
scrape_configs:
- job_name: node-exporter
  kubernetes_sd_configs:
  - role: endpoints
    namespaces:
      names:
      - monitoring
  relabel_configs:
  - source_labels: [__meta_kubernetes_service_name]
    action: keep
    regex: node-exporter
  - source_labels: [__address__]
    action: replace
    regex: ([^:]+)(?::\d+)?
    replacement: $1:9100
    target_label: __address__
```

---

## 四、监控告警规则

### 4.1 节点级别告警

```yaml
# Prometheus 告警规则
groups:
- name: node-alerts
  rules:
  - alert: NodeNotReady
    expr: kube_node_status_condition{condition="Ready",status="true"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Node {{ $labels.node }} is not ready"

  - alert: NodeHighCPU
    expr: 100 - (avg by(instance) (rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 85
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Node {{ $labels.instance }} CPU usage > 85%"

  - alert: NodeHighMemory
    expr: (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100 > 85
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Node {{ $labels.instance }} memory usage > 85%"

  - alert: NodeDiskSpaceLow
    expr: (node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"}) * 100 < 15
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Node {{ $labels.instance }} root disk < 15% free"

  - alert: NodeFilesystemInodesLow
    expr: (node_filesystem_files_free{mountpoint="/"} / node_filesystem_files{mountpoint="/"}) * 100 < 10
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Node {{ $labels.instance }} inode usage > 90%"

  - alert: KubletPodCreationsSlow
    expr: histogram_quantile(0.99, rate(kubelet_pod_worker_duration_seconds_bucket[5m])) > 60
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "Kubelet on {{ $labels.node }} is slow at creating pods"
```

---

## 五、监控仪表盘

### 5.1 推荐 Grafana Dashboard

| Dashboard | ID | 说明 |
|-----------|-----|------|
| Node Exporter Full | 1860 | 主机级全面监控面板 |
| Kubernetes / Compute Resources / Node | 7249 | 节点资源使用概览 |
| Kubernetes / Kubelet | 7243 | kubelet 指标面板 |
| Kubernetes / Networking / Cluster | 7246 | 集群网络监控 |

---

## 六、常见错误与排查

| 错误 | 原因 | 排查命令 | 解决方案 |
|------|------|---------|---------|
| `error: Metrics API not available` | metrics-server 未安装 | `kubectl get pods -n kube-system -l k8s-app=metrics-server` | 安装 metrics-server |
| `kubectl top` 无数据 | metrics-server 未就绪 | `kubectl logs -n kube-system -l k8s-app=metrics-server` | 检查 metrics-server 日志 |
| node-exporter 无法采集指标 | RBAC 权限不足 | `kubectl logs -n monitoring -l app=node-exporter` | 创建 ClusterRole/ClusterRoleBinding |
| kubelet metrics 返回 401 | 匿名访问被禁止 | `curl -k https://localhost:10250/metrics` | 使用有效证书或通过 kubectl proxy |
| metrics-server 日志 `x509: certificate` | kubelet 证书不可信 | `kubectl logs -n kube-system -l k8s-app=metrics-server` | 添加 `--kubelet-insecure-tls`（测试环境）或配置证书 |
| Prometheus 无法 scrape node-exporter | Service/Endpoints 配置错误 | `kubectl get endpoints -n monitoring node-exporter` | 检查 Service selector 和 Pod labels |
| Pod 数量统计不准确 | kubelet PLEG 延迟 | `curl -k https://localhost:10250/metrics | grep pleg` | 检查容器运行时性能 |

### 调试命令速查

```bash
# 检查 metrics-server 状态
kubectl get deployment metrics-server -n kube-system
kubectl logs -n kube-system -l k8s-app=metrics-server

# 检查 metrics API
kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes
kubectl get --raw /apis/metrics.k8s.io/v1beta1/pods

# 检查 node-exporter
kubectl get daemonset node-exporter -n monitoring
kubectl get pods -n monitoring -l app=node-exporter -o wide

# 直接访问 node-exporter
curl http://<node-ip>:9100/metrics

# 检查 kubelet stats
curl -k https://localhost:10250/stats/summary
```

---

## 相关函数

| 函数/组件 | 源码位置 | 说明 |
|----------|---------|------|
| `cAdvisor` | `pkg/kubelet/cadvisor/` | 容器指标采集 |
| `metricsHandler` | `pkg/kubelet/server/server.go` | kubelet metrics 端点 |
| `ResourceMetrics` | `pkg/kubelet/server/stats/` | 资源指标 API |
| `VolumeStats` | `pkg/kubelet/volumemanager/` | 卷统计信息 |
| `PLEG` | `pkg/kubelet/pleg/` | Pod 生命周期事件 |
| `metrics-server` | `kubernetes-sigs/metrics-server` | 指标聚合器 |
| `node-exporter` | `prometheus/node_exporter` | 主机指标采集 |

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/networking|networking]]
- [[domain-17-system-foundation/topic-cheat-sheet/linux|linux]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker|docker]]
