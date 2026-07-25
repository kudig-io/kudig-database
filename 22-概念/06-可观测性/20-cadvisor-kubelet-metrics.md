---
sources:
- "可观测性/指标/20-cadvisor-kubelet-metrics.md"
title: cAdvisor 与 kubelet 指标采集
summary: 解析 kubelet 内置 cAdvisor 的三个 metrics 端点、容器级资源指标分类与 Prometheus 采集配置。
category: concepts
tags:
- cadvisor
- kubelet
- metrics
- container-metrics
- resource-usage
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 可观测性工程师
estimated_read_time: 20min
intent_queries:
- cAdvisor 是什么
- kubelet metrics 端点有哪些
- 如何采集容器 CPU 内存指标
- container_cpu 与 container_fs 指标含义
trigger_keywords:
- cAdvisor
- kubelet metrics
- /metrics/cadvisor
- 容器指标
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# cAdvisor 与 kubelet 指标采集

> **适用版本**: v1.28 - v1.33 | **最后更新**: 2026-07 | **参考**: [kubernetes.io/docs/concepts/cluster-administration/system-metrics](https://kubernetes.io/docs/concepts/cluster-administration/system-metrics/)、[github.com/google/cadvisor](https://github.com/google/cadvisor)

## 概述

**cAdvisor（Container Advisor）** 是 Google 开源的容器资源监控与性能分析工具，负责采集**每个容器**（含 Pod infra container / pause container）的 CPU、内存、文件系统、网络指标。在 Kubernetes 中，cAdvisor **不再作为独立 DaemonSet 部署**，而是以库的形式**原生集成进 kubelet**（代码位于 `cadvisor.ContainerManager` 接口实现内），由 kubelet 进程直接调用。

这意味着每个节点的 kubelet 既是 Pod 生命周期管理器，又是该节点上所有容器的指标采集器。kubelet 在其 `10250` HTTPS 端口上暴露多个 metrics 子路径，其中 `/metrics/cadvisor` 即是 cAdvisor 数据的对外出口。

本文做**从零建立**：端点 → 架构 → 指标分类 → 采集机制 → 与 KSM/Metrics Server 的关系 → 生产实践 → 排障。

### 为什么需要独立的 cAdvisor 专题

全库中 cAdvisor 与 kubelet metrics 此前仅散见于 SLO 文档（如 `04-sli-definition-selection.md` 提到 `kubelet_runtime_operations_errors_total`）与 Prometheus 主文档的指标速查表，缺乏对**采集链路**本身的系统说明。本篇聚焦"指标从哪里来、如何被抓取、抓不到时如何排障"这一运维核心问题。

---

## kubelet 的三个 metrics 端点

kubelet 在 `https://<node>:10250/` 下暴露多个端点，每个端点对应不同的数据源与用途。理解端点划分是正确配置 Prometheus scrape、定位指标缺失问题的前提。

### 端点对比总表

| 端点 | 完整路径 | 数据源 | 典型指标前缀 | 默认认证 | 主要消费者 |
|------|----------|--------|--------------|----------|------------|
| **kubelet 自身** | `https://<node>:10250/metrics` | kubelet 内部子系统 | `kubelet_*`、`volume_*`、`storage_*` | 需认证（client cert / bearer） | Prometheus |
| **cAdvisor** | `https://<node>:10250/metrics/cadvisor` | cAdvisor（kubelet 内嵌） | `container_*`、`machine_*` | 需认证 | Prometheus、Metrics Server（间接） |
| **probes** | `https://<node>:10250/metrics/probes` | Probe Manager | `prober_*` | 需认证 | Prometheus |
| **resource（summary）** | `https://<node>:10250/stats/summary` | 聚合 cAdvisor + kubelet | JSON（非 Prometheus 格式） | 需认证 | Metrics Server、`kubectl top` |

### 10250 vs 10255（read-only 端口已废弃）

历史背景：早期 kubelet 同时监听两个端口：

| 端口 | 用途 | 当前状态 |
|------|------|----------|
| **10250** | kubelet HTTPS API（含所有 metrics 端点、Pod exec/attach） | **保留**，生产唯一入口 |
| **10255** | kubelet read-only HTTP 端口（`/stats`、`/metrics` 无需认证） | **自 v1.18 起逐步废弃，已移除** |

> **重要**：`--read-only-port=10255` 在新版本中默认为 `0`（关闭）。任何基于 10255 的旧文档、旧告警、旧 scrape 配置都已失效。所有 metrics 抓取必须走 10250 并配置认证。

### 端点访问示例

```bash
# 🟢 低风险：只读查看 kubelet 自身指标（通过 API server 代理，复用 kubectl 凭证）
kubectl get --raw "/api/v1/nodes/<node-name>/proxy/metrics" | head -20

# 🟢 低风险：只读查看 cAdvisor 指标
kubectl get --raw "/api/v1/nodes/<node-name>/proxy/metrics/cadvisor" | grep "^container_cpu" | head

# 🟢 低风险：只读查看 summary（Metrics Server 读取的 JSON 接口）
kubectl get --raw "/api/v1/nodes/<node-name>/proxy/stats/summary" | jq '.pods[0].containers[0]'
```

通过 `kubectl proxy` + API server 代理访问是排障时最安全的方式：复用 kubeconfig 凭证，无需直接暴露 10250。

---

## cAdvisor 架构与 kubelet 集成

### cAdvisor 不是独立进程

社区版 cAdvisor（`google/cadvisor`）可独立部署为容器，但 **Kubernetes 集群中不采用这种方式**。kubelet 源码通过 import cAdvisor 作为 Go 库，由 kubelet 主进程持有 `cadvisor.ContainerManager` 实例。因此：

- 你不会在 `kubectl get pods -n kube-system` 中找到一个名为 `cadvisor` 的 Pod。
- cAdvisor 的生命周期与 kubelet 完全绑定：kubelet 重启即 cAdvisor 重启，cAdvisor 崩溃会导致 kubelet 重新初始化它。
- 节点上**没有**独立的 cAdvisor 端口（如独立的 `4194`，该端口在老版本中存在，现已移除）。

### 数据采集链路：cgroup → cAdvisor → kubelet → 端点

cAdvisor 的数据采集基于 Linux 内核的 cgroup 与 procfs：

```
┌─────────────────────────────────────────────────────────────────┐
│  Linux Kernel                                                   │
│  ┌───────────────┐  ┌───────────────┐  ┌────────────────────┐  │
│  │ /sys/fs/cgroup│  │ /proc/<pid>/* │  │ /sys/class/net/*   │  │
│  │ (CPU/Mem/IO)  │  │ (进程级数据)   │  │ (网络统计)          │  │
│  └──────┬────────┘  └───────┬───────┘  └─────────┬──────────┘  │
└─────────┼───────────────────┼────────────────────┼─────────────┘
          │                   │                    │
          ▼                   ▼                    ▼
┌─────────────────────────────────────────────────────────────────┐
│  cAdvisor (kubelet 内嵌库)                                       │
│  - root cgroup → 节点总量 (machine_*)                            │
│  - pod cgroup (kubepods.slice) → Pod 级聚合                      │
│  - container cgroup (kubepods-<id>.slice/<container>) → 容器级   │
└─────────────────────────────┬───────────────────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  kubelet 进程                                                    │
│  - 在内存中维护 cAdvisor 采集的 Stats                            │
│  - 通过 /metrics/cadvisor 端点以 Prometheus 格式暴露             │
│  - 通过 /stats/summary 端点以 JSON 格式暴露（给 Metrics Server）  │
└─────────────────────────────┬───────────────────────────────────┘
                              ▼
                    Prometheus / Metrics Server
```

### cgroup 层级映射

cAdvisor 按 cgroup 树自上而下采集，对应到 Kubernetes 的层级：

| cgroup 层级 | 路径示例（systemd cgroup driver） | 对应 metrics |
|-------------|-----------------------------------|--------------|
| root（节点） | `/` | `machine_cpu_cores`、`machine_memory_bytes` |
| QoS 类 | `/kubepods.slice`、`/kubepods-burstable.slice` | 可按 QoS 聚合 |
| Pod | `/kubepods-pod<uid>.slice` | `container_*{pod_name=...}` 中 pod 级聚合 |
| Container | `/kubepods-pod<uid>.slice:docker:<id>` | `container_*{container=...}` |

> **cgroup v1 vs v2**：Kubernetes v1.25+ 全面支持 cgroup v2。cAdvisor 在 cgroup v2 下读取 `/sys/fs/cgroup/` 的统一层级文件。少数旧指标（如某些 `cpuset` 细分）在 v2 下语义不同或缺失，排障章节会展开。

### 为什么网络指标是 Pod 级而非容器级

cAdvisor 从 `/sys/class/net/<interface>/statistics/` 读取网卡计数。在 Kubernetes 中，**每个 Pod 共享一个 network namespace**（由 pause container 创建），所有业务容器共用同一组虚拟网卡。因此：

- `container_network_receive_bytes_total` 实际按 **Pod**（即 `pod_name` label）聚合，同一 Pod 内多个容器的网络指标数值相同。
- 不能用该指标区分 Pod 内某单个容器的流量——这是 cAdvisor 的设计限制，需用 sidecar/ebpf 才能做容器级网络细分。

---

## kubelet 自身指标分类

`/metrics` 端点暴露 kubelet 各子系统的指标，前缀多为 `kubelet_`。下表按子系统分类，给出代表指标与运维含义。

### Pod / 容器生命周期

| 指标 | 类型 | 含义 | 排障场景 |
|------|------|------|----------|
| `kubelet_running_pods` | Gauge | 当前运行中的 Pod 数 | 接近 `--max-pods`(默认 110) 需扩容或调高上限 |
| `kubelet_running_containers` | Gauge | 当前运行中的容器数（按 state 分） | 容器密度评估 |
| `kubelet_running_pod_count` | Gauge | 同上的另一口径（含 phase label） | 节点负载基线 |

### CRI 运行时操作

| 指标 | 类型 | 含义 | 排障场景 |
|------|------|------|----------|
| `kubelet_runtime_operations_total` | Counter | CRI 操作总数（按 operation_type：create/run/remove/stop...） | 运行时负载趋势 |
| `kubelet_runtime_operations_errors_total` | Counter | CRI 操作错误数（按 operation_type） | **容器运行时健康核心指标**，错误率升高通常指向 containerd/docker 故障 |
| `kubelet_runtime_operations_duration_seconds` | Histogram | CRI 操作耗时 | containerd 卡顿诊断 |

> **运维要点**：`kubelet_runtime_operations_errors_total` / `kubelet_runtime_operations_total` 是 SLI 文档中 kubelet "运行时错误率" SLI 的直接来源，建议阈值 P99 < 0.1%。

### PLEG（Pod Lifecycle Event Generator）

| 指标 | 类型 | 含义 | 排障场景 |
|------|------|------|----------|
| `kubelet_pleg_relist_duration_seconds` | Histogram | 单次 relist（重新列出所有容器状态）耗时 | **PLEG 健康核心指标**，P99 > 3s 视为不健康 |
| `kubelet_pleg_relist_interval_seconds` | Histogram | 两次 relist 的间隔 | 正常约 1s，间隔拉长说明 kubelet 卡住 |
| `kubelet_pleg_last_seen_seconds` | Gauge | 距上次 PLEG 事件的秒数 | 该指标不更新是 kubelet 卡死的早期信号 |

> **PLEG 不健康的根因**：通常是容器运行时（containerd/CRI-O）响应慢、节点负载过高、或 cgroup 读取阻塞。长期 PLEG 高延迟会导致节点被标记 NotReady。

### 启动 / Worker 性能

| 指标 | 类型 | 含义 | 排障场景 |
|------|------|------|----------|
| `kubelet_pod_start_duration_seconds` | Histogram | Pod 从被分配到 Running 的总耗时 | Pod 启动慢诊断（含镜像拉取、卷挂载、CNI） |
| `kubelet_pod_worker_duration_seconds` | Histogram | Pod worker 单次同步耗时 | kubelet 同步循环是否积压 |
| `kubelet_container_status_duration_seconds` | Histogram | 获取容器状态耗时 | CRI status 调用是否成为瓶颈 |

### 卷与存储

| 指标 | 类型 | 含义 | 排障场景 |
|------|------|------|----------|
| `kubelet_volume_metric_collection_duration_seconds` | Histogram | 收集单个卷指标的耗时 | 卷指标采集阻塞 kubelet（CSI 驱动慢） |
| `kubelet_volume_metric_collection_errors_total` | Counter | 卷指标采集错误数 | CSI 驱动异常 |
| `volume_manager_total_volumes` | Gauge | 节点上的卷数量 | 卷密度评估 |
| `storage_operation_duration_seconds` | Histogram | 存储操作（attach/detach/mount/unmount）耗时 | CSI 操作慢导致 Pod 启动卡 |

### 日志与其他

| 指标 | 类型 | 含义 | 排障场景 |
|------|------|------|----------|
| `kubelet_container_log_filesystem_used_bytes` | Gauge | 容器日志文件占用 | 日志膨胀导致节点磁盘满 |
| `kubelet_container_log_filesystem_limit_bytes` | Gauge | 容器日志上限 | 通常由 `--container-log-max-files` 控制 |
| `kubelet_cgroup_manager_duration_seconds` | Histogram | cgroup 管理操作耗时 | cgroup v2 迁移期常见升高 |
| `kubelet_node_status_update_duration_seconds` | Histogram | 节点状态上报耗时 | 上报慢导致节点状态滞后 |

### 快速验证指标存在性

```bash
# 🟢 低风险：列出 kubelet 暴露的所有指标名称
kubectl get --raw "/api/v1/nodes/<node-name>/proxy/metrics" \
  | grep "^# HELP kubelet_" | awk '{print $3}' | sort -u
```

---

## cAdvisor 指标分类（`/metrics/cadvisor`）

cAdvisor 端点暴露的指标以 `container_` 和 `machine_` 为前缀。这是**容器级资源监控的唯一权威数据源**。下表按资源维度分类。

### CPU 类指标

| 指标 | 类型 | 含义 | 使用方式 |
|------|------|------|----------|
| `container_cpu_usage_seconds_total` | Counter | 容器累计占用 CPU 时间（秒） | 用 `rate()` 求每秒 CPU 核数，最常用的容器 CPU 利用率指标 |
| `container_cpu_cfs_periods_total` | Counter | CFS 调度周期总数 | CPU 限流的分母 |
| `container_cpu_cfs_throttled_periods_total` | Counter | 被限流的周期数 | CPU 限流的分子，**关键** |
| `container_cpu_cfs_throttled_seconds_total` | Counter | 累计被限流的秒数 | 限流严重程度 |
| `container_cpu_system_seconds_total` | Counter | 内核态 CPU 时间 | 区分 user/sys |
| `container_cpu_user_seconds_total` | Counter | 用户态 CPU 时间 | 区分 user/sys |
| `container_cpu_load_average_10s` | Gauge | 10 秒负载均值 | 容器负载趋势 |
| `container_cpu_cfs_quota_seconds` / `_period_seconds` | Gauge | CFS 配额/周期 | 反推 `cpu.limit` 配置 |

**容器 CPU 利用率 PromQL**：

```promql
# 单容器 CPU 使用率（核数）
sum(rate(container_cpu_usage_seconds_total{container!="POD",container!=""}[5m])) by (namespace, pod, container)

# 容器 CPU 占 limit 的百分比
sum(rate(container_cpu_usage_seconds_total{container!=""}[5m])) by (namespace, pod, container)
/
sum(container_spec_cpu_quota{container!=""} / container_spec_cpu_period{container!=""}) by (namespace, pod, container)
* 100
```

**CPU 限流率（throttle ratio）**：

```promql
# 限流周期占比 > 25% 即告警
sum(rate(container_cpu_cfs_throttled_periods_total[5m])) by (namespace, pod, container)
/
sum(rate(container_cpu_cfs_periods_total[5m])) by (namespace, pod, container)
```

> **运维要点**：CPU throttle 是 Java/Go 应用延迟毛刺的常见根因，即便 CPU 利用率不高也可能严重 throttle（CFS burst 未开启时尤其明显）。该比值是饱和度 SLI 的核心。

### 内存类指标

| 指标 | 类型 | 含义 | 使用方式 |
|------|------|------|----------|
| `container_memory_usage_bytes` | Gauge | 容器总内存（含 cache） | 含 page cache，不适合做 OOM 判断 |
| `container_memory_working_set_bytes` | Gauge | **工作集内存**（usage - inactive_file） | **OOM kill 的实际判定依据，最关键** |
| `container_memory_rss` | Gauge | 常驻内存（RSS） | 应用真实内存占用 |
| `container_memory_cache` | Gauge | 页缓存 | 评估 cache 占比 |
| `container_memory_swap_usage_bytes` | Gauge | swap 使用量 | 通常为 0（K8s 默认禁 swap） |
| `container_memory_failcnt` | Counter | 触发 cgroup 内存上限的次数 | 接近 OOM 的早期信号 |
| `container_memory_max_usage_bytes` | Gauge | 历史最大内存使用 | 峰值评估 |
| `container_spec_memory_limit_bytes` | Gauge | 内存 limit | 算占比的分母 |

> **关键区分**：OOM killer 判定基于 `working_set_bytes`，而非 `usage_bytes`。监控内存压力必须用前者。差异在于 inactive file-backed page cache 会被回收，不计入工作集。

**内存压力告警 PromQL**：

```promql
# 工作集内存占 limit 90% 告警
container_memory_working_set_bytes{container!=""}
/
container_spec_memory_limit_bytes{container!=""} > 0.9
```

### 网络类指标（Pod 级）

| 指标 | 类型 | 含义 | 注意事项 |
|------|------|------|----------|
| `container_network_receive_bytes_total` | Counter | 累计接收字节 | **Pod 级**，同 Pod 多容器数值相同 |
| `container_network_transmit_bytes_total` | Counter | 累计发送字节 | 同上 |
| `container_network_receive_packets_total` | Counter | 累计接收包数 | 含 `dropped` 子指标 |
| `container_network_receive_packets_dropped_total` | Counter | 累计丢包数 | 网络问题诊断 |
| `container_network_errors_total` | Counter | 网络错误数 | 网卡/CNI 异常 |

> **再次强调**：`container_network_*` 的 `container` label 通常是 `POD`（pause container），实际语义是 Pod 级。按 `pod_name` 聚合是正确用法。

### 文件系统类指标

| 指标 | 类型 | 含义 | 使用方式 |
|------|------|------|----------|
| `container_fs_usage_bytes` | Gauge | 容器可写层 + 镜像层占用 | 评估容器本地存储 |
| `container_fs_limit_bytes` | Gauge | 文件系统上限 | 算占比分母 |
| `container_fs_reads_bytes_total` | Counter | 累计读字节 | IO 负载 |
| `container_fs_writes_bytes_total` | Counter | 累计写字节 | IO 负载 |
| `container_fs_io_time_seconds_total` | Counter | 累计 IO 时间 | IO 饱和度 |
| `container_fs_io_current` | Gauge | 当前进行中的 IO 数 | IO 队列深度 |
| `container_fs_inodes_free` / `_total` | Gauge | inode 空闲/总数 | inode 耗尽（小文件多） |

> **存储分层**：`container_fs_*` 反映容器 rootfs（通常是 overlayfs）。Pod 使用的 PVC 用量由 kubelet 通过 CSI 接口采集，对应的是 `kubelet_volume_stats_used_bytes`（在 `/metrics` 端点，不在 cAdvisor 端点）。两者不要混淆。

### machine_* 类指标（节点级）

| 指标 | 类型 | 含义 |
|------|------|------|
| `machine_cpu_cores` | Gauge | 节点 CPU 核数 |
| `machine_memory_bytes` | Gauge | 节点总内存 |
| `machine_nvidia_gpu_*` | Gauge | GPU 相关（有 GPU 节点时） |
| `machine_dm_*` | Gauge | device mapper（devicemapper 存储驱动） |

这些是节点硬件基线，可作为校验 node-exporter 数据的参照。

### 验证指标存在性

```bash
# 🟢 低风险：列出 cAdvisor 暴露的指标前缀分布
kubectl get --raw "/api/v1/nodes/<node-name>/proxy/metrics/cadvisor" \
  | grep "^# HELP" | awk '{print $3}' \
  | awk -F'_' '{print $1"_"$2}' | sort | uniq -c | sort -rn | head
```

---

## 采集机制：Prometheus 如何抓取 kubelet/cAdvisor

本节给出生产可用的 Prometheus scrape 配置，并解释每个关键配置项的含义。

### 采集架构总览

```
┌──────────────┐   kubernetes_sd_configs    ┌──────────────────┐
│              │   (role=node, 自动发现)      │  每个节点的 kubelet │
│  Prometheus  │ ──────────────────────────► │  :10250          │
│              │   scrape /metrics           │  /metrics        │
│              │   scrape /metrics/cadvisor  │  /metrics/cadvisor│
│              │   scrape /metrics/probes    │  /metrics/probes │
└──────────────┘                             └──────────────────┘
```

Prometheus 推荐使用 **`kubernetes_sd_configs` 的 `role=node`** 自动发现节点，再通过 relabel 把节点地址填入 `__address__` 并拼上 10250 端口。每个端点拆成独立 job，便于差异化配置。

### 完整 prometheus.yml job 片段

```yaml
scrape_configs:
  # ============ Job 1: kubelet 自身指标 ============
  - job_name: 'kubernetes-kubelet'
    kubernetes_sd_configs:
      - role: node
    scheme: https
    tls_config:
      # 信任 kubelet 自签证书（生产建议用 ca_file 校验）
      ca_file: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
      # 生产环境应配置 insecure_skip_verify: false 并提供 ca_file
      # insecure_skip_verify: true   # 仅测试环境
    bearer_token_file: /var/run/secrets/kubernetes.io/serviceaccount/token
    relabel_configs:
      # 把 __address__ 替换为 <node-ip>:10250
      - action: replace
        source_labels: [__address__]
        regex: ([^:]+)(?::\d+)?            # 去掉原端口
        replacement: $1:10250
        target_label: __address__
      # 保留节点名作为 label
      - action: labelmap
        regex: __meta_kubernetes_node_label_(.+)
      # 标记节点名
      - source_labels: [__meta_kubernetes_node_name]
        target_label: node
    # 只抓 /metrics，不抓 cadvisor 子路径
    metric_relabel_configs:
      - source_labels: [__name__]
        regex: 'kubelet_.*|volume_.*|storage_.*|sync_proxy_rules_.*'
        action: keep

  # ============ Job 2: cAdvisor 指标 ============
  - job_name: 'kubernetes-cadvisor'
    kubernetes_sd_configs:
      - role: node
    scheme: https
    tls_config:
      ca_file: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
    bearer_token_file: /var/run/secrets/kubernetes.io/serviceaccount/token
    relabel_configs:
      - action: replace
        source_labels: [__address__]
        regex: ([^:]+)(?::\d+)?
        replacement: $1:10250
        target_label: __address__
      - action: labelmap
        regex: __meta_kubernetes_node_label_(.+)
      - source_labels: [__meta_kubernetes_node_name]
        target_label: node
    # 抓 /metrics/cadvisor 子路径
    params:
      # 通过 kubelet 内部路径覆盖
    metrics_path: /metrics/cadvisor
    metric_relabel_configs:
      # 只保留 container_ 和 machine_ 前缀
      - source_labels: [__name__]
        regex: 'container_.*|machine_.*'
        action: keep
      # 可选：丢弃高基数 label（见生产实践章节）

  # ============ Job 3: probes 指标 ============
  - job_name: 'kubernetes-probes'
    kubernetes_sd_configs:
      - role: node
    scheme: https
    tls_config:
      ca_file: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
    bearer_token_file: /var/run/secrets/kubernetes.io/serviceaccount/token
    relabel_configs:
      - action: replace
        source_labels: [__address__]
        regex: ([^:]+)(?::\d+)?
        replacement: $1:10250
        target_label: __address__
    metrics_path: /metrics/probes
```

> **关键说明**：`metrics_path: /metrics/cadvisor` 使 Prometheus 抓取该子路径。注意 `scheme: https` + `bearer_token_file` 是生产标配；`insecure_skip_verify: true` 仅用于测试，因为它会让 Prometheus 接受任意证书，存在中间人风险。

### 认证方式对比

| 方式 | 配置 | 适用场景 | 安全性 |
|------|------|----------|--------|
| **Bearer Token** | `bearer_token_file: /var/run/secrets/kubernetes.io/serviceaccount/token` | Prometheus 以 Pod 形式部署，使用 ServiceAccount token | 中（token 泄漏风险） |
| **Client Certificate** | `cert_file` + `key_file` | Prometheus 独立部署或需双向 TLS | 高 |
| **insecure_skip_verify** | `tls_config.insecure_skip_verify: true` | 仅测试/排障 | **低，禁用于生产** |
| **kubectl proxy 转发** | 人工 `kubectl proxy` 后访问 localhost:8001 | 临时排障 | 复用 kubeconfig，安全 |

### RBAC：Prometheus 抓 kubelet 所需权限

Prometheus 用 ServiceAccount token 访问 kubelet `/metrics` 端点，需要绑定 `system:monitoring` 或等价 ClusterRole。kubelet 默认对 `/metrics`、`/metrics/cadvisor` 启用鉴权（`--authorization-mode=Webhook` + `--authentication-token-webhook=true`）。

```yaml
# ServiceAccount
apiVersion: v1
kind: ServiceAccount
metadata:
  name: prometheus
  namespace: monitoring
---
# ClusterRoleBinding：授予 system:monitoring（内置 ClusterRole，允许访问 /metrics）
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: prometheus-kubelet
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: system:monitoring        # 内置角色，含 nodes/metrics 的 get/proxy 权限
subjects:
  - kind: ServiceAccount
    name: prometheus
    namespace: monitoring
```

> **常见坑**：`system:monitoring` 是内置 ClusterRole，但**需要显式绑定**到你的 ServiceAccount。仅创建 ServiceAccount 而不绑定，会导致 scrape 返回 403。

### 基于 API server 的 discovery（替代方案）

除直接抓 kubelet 10250，也可让 Prometheus 通过 API server proxy 抓取：

```yaml
- job_name: 'kubelet-via-apiserver'
  kubernetes_sd_configs:
    - role: node
  scheme: https
  # 直接用 API server 地址
  relabel_configs:
    - source_labels: [__meta_kubernetes_node_name]
      target_label: __address__
      replacement: kubernetes.default.svc:443
    - source_labels: [__meta_kubernetes_node_name]
      regex: (.+)
      target_label: __metrics_path__
      replacement: /api/v1/nodes/$1/proxy/metrics
```

此方式所有流量过 API server，适合 API server 负载可控的小集群；大集群推荐直连 10250。

---

## 与 kube-state-metrics / Metrics Server 的关系

容器监控领域有三个常被混淆的数据源。理解它们的分工是设计监控体系的基础。

### 三者定位对比

| 维度 | cAdvisor（kubelet 内嵌） | Metrics Server | kube-state-metrics (KSM) |
|------|--------------------------|----------------|--------------------------|
| **数据性质** | 实时资源指标（CPU/Mem/网络/FS） | 聚合后的资源指标（CPU/Mem） | **对象状态**（Pod/Node/Deployment 的期望 vs 实际） |
| **数据来源** | cgroup / procfs | cAdvisor（通过 kubelet summary API） | kube-apiserver（list/watch） |
| **指标前缀** | `container_*`、`machine_*` | （不直接暴露 Prometheus 指标） | `kube_pod_*`、`kube_node_*`、`kube_deployment_*` |
| **历史保留** | 无（瞬时值，靠 Prometheus 存） | 仅内存中 1 分钟窗口 | 无（瞬时值） |
| **主要消费者** | Prometheus | HPA、VPA、`kubectl top` | Prometheus、Dashboard |
| **采样间隔** | 由 Prometheus scrape 控制（通常 30s-60s） | 默认 60s | 由 Prometheus scrape 控制 |
| **是否反映资源用量** | 是（原始值） | 是（聚合值） | 否（只反映对象存在性/状态） |

### 数据流向

```
   cAdvisor (kubelet 内嵌)
        │
        │ raw container metrics
        ▼
   ┌─────────────┐         ┌──────────────────┐
   │  kubelet     │         │  kube-apiserver   │
   │ /metrics/*   │         │ (对象状态源)       │
   └──────┬───────┘         └────────┬──────────┘
          │                          │
   ┌──────┴────────┬─────────┐        │
   │               │         │        │
   ▼               ▼         ▼        ▼
 Prometheus   Metrics     kubectl   kube-state-metrics
  (scrape)    Server       top         (watch/list)
   │           │                        │
   │           ▼                        ▼
   │       HPA / VPA               Prometheus (scrape)
   │
   ▼
 告警 / Dashboard / 长期存储 (Thanos)
```

### 为什么监控用 Prometheus 抓 cAdvisor，而不是查 Metrics Server

| 维度 | Prometheus + cAdvisor | Metrics Server |
|------|-----------------------|----------------|
| **粒度** | 完整容器级（含 FS/网络/throttle） | 仅 CPU/Mem，且聚合到 Pod |
| **历史** | 长期存储（数月/数年） | 内存中 1 分钟，无历史 |
| **查询灵活性** | 任意 PromQL 聚合 | 仅提供当前值 API |
| **可用性** | 独立链路，不受 HPA 影响数值 | 为 HPA 优化，精度受限 |

结论：**Metrics Server 服务于 HPA/VPA 的实时决策，Prometheus + cAdvisor 服务于监控与告警**。两者互补，不可互相替代。详见 [[09-可观测性/02-指标/19-kube-state-metrics-deep-dive.md|kube-state-metrics 深度解析]] 了解 KSM 如何补充对象状态维度。

---

## 生产实践

### 关键告警规则

以下告警规则可直接加入 Prometheus `groups`，覆盖 cAdvisor/kubelet 最常见故障。

```yaml
groups:
- name: cadvisor-kubelet.rules
  rules:
  # ---------- kubelet PLEG 不健康 ----------
  - alert: KubeletPlegHighLatency
    expr: histogram_quantile(0.99, sum(rate(kubelet_pleg_relist_duration_seconds_bucket[5m])) by (instance, le)) > 3
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "kubelet PLEG relist 延迟过高 ({{ $labels.instance }})"
      description: "PLEG P99 延迟 {{ $value }}s 超过 3s，节点可能即将 NotReady。"

  # ---------- kubelet 运行时错误率高 ----------
  - alert: KubeletRuntimeErrors
    expr: |
      sum(rate(kubelet_runtime_operations_errors_total[5m])) by (instance, operation_type)
      /
      sum(rate(kubelet_runtime_operations_total[5m])) by (instance, operation_type)
      > 0.01
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "kubelet 运行时操作错误率高 ({{ $labels.instance }} {{ $labels.operation_type }})"

  # ---------- 容器 CPU 限流 > 25% ----------
  - alert: ContainerCpuThrottlingHigh
    expr: |
      sum(rate(container_cpu_cfs_throttled_periods_total[5m])) by (namespace, pod, container)
      /
      sum(rate(container_cpu_cfs_periods_total[5m])) by (namespace, pod, container)
      > 0.25
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "容器 CPU 限流严重 ({{ $labels.namespace }}/{{ $labels.pod }} {{ $labels.container }})"
      description: "限流周期占比 {{ $value | humanizePercentage }}，建议调高 CPU limit 或开启 CFS burst。"

  # ---------- 容器内存接近 limit ----------
  - alert: ContainerMemoryPressure
    expr: |
      container_memory_working_set_bytes{container!="",container!="POD"}
      /
      container_spec_memory_limit_bytes{container!="",container!="POD"} > 0
      and on(namespace, pod, container)
      container_memory_working_set_bytes{container!="",container!="POD"}
      /
      container_spec_memory_limit_bytes{container!="",container!="POD"} > 0.9
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "容器内存接近 limit ({{ $labels.namespace }}/{{ $labels.pod }} {{ $labels.container }})"

  # ---------- 卷指标采集阻塞 ----------
  - alert: KubeletVolumeMetricCollectionSlow
    expr: histogram_quantile(0.99, sum(rate(kubelet_volume_metric_collection_duration_seconds_bucket[5m])) by (instance, le)) > 5
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "kubelet 卷指标采集耗时过长 ({{ $labels.instance }})"
      description: "可能是 CSI 驱动响应慢，会拖慢整个 kubelet。"
```

### 指标基数（cardinality）管理

cAdvisor 是典型的高基数指标源。每个 `(namespace, pod, container, node)` 组合都会产生一组 `container_*` 序列。大规模集群必须控制基数，否则 Prometheus 内存与查询性能会崩溃。

**高基数 label 来源**：

| label | 基数贡献 | 处理建议 |
|-------|----------|----------|
| `pod` / `pod_name` | 每个实例一个 | 保留，但避免对每个 Pod 单独告警 |
| `container` | 每个容器一个 | 保留 |
| `id`（容器 ID） | 每次重启变化，**极高** | **必须 drop**：`metric_relabel_configs` 中 `action: labeldrop` |
| `image_id` | 同上 | **建议 drop** |
| `name`（cAdvisor 内部名） | 类似 id | **建议 drop** |

**基数裁剪示例**（加在 cadvisor job 的 `metric_relabel_configs`）：

```yaml
metric_relabel_configs:
  # 丢弃会随容器重启变化的极高基数 label
  - action: labeldrop
    regex: 'id|image_id|name'
  # 丢弃无意义的 root cgroup / 机器级冗余序列（可选）
  - source_labels: [__name__]
    regex: 'container_tasks_state|container_memory_failcnt'
    action: drop
```

> **验证基数**：用 Prometheus 的 `prometheus_tsdb_head_series` 与 `prometheus_tsdb_head_active_appenders` 监控序列增长趋势；用 TSDB 头部分析（`/api/v1/status/tsdb`）定位哪些 label 占用最多。

### Recording Rules 提速

容器级原始指标查询（尤其跨节点聚合）很重，建议预计算成 recording rule：

```yaml
groups:
- name: cadvisor-recording.rules
  interval: 1m
  rules:
  # 容器 CPU 使用率（核数）
  - record: container:cpu_usage_seconds:rate5m
    expr: |
      sum(rate(container_cpu_usage_seconds_total{container!="POD",container!=""}[5m]))
      by (namespace, pod, container, node)

  # 容器 CPU 限流率
  - record: container:cpu_throttling:ratio5m
    expr: |
      sum(rate(container_cpu_cfs_throttled_periods_total[5m])) by (namespace, pod, container)
      /
      sum(rate(container_cpu_cfs_periods_total[5m])) by (namespace, pod, container)

  # 容器内存工作集占 limit 比例
  - record: container:memory_working_set:ratio_to_limit
    expr: |
      container_memory_working_set_bytes{container!="",container!="POD"}
      /
      container_spec_memory_limit_bytes{container!="",container!="POD"}

  # 节点 CPU 利用率（所有容器求和 / 节点核数）
  - record: node:container_cpu_usage:ratio
    expr: |
      sum(rate(container_cpu_usage_seconds_total{id="/"}[5m])) by (node)
      /
      machine_cpu_cores
```

Dashboard 与告警直接引用这些预计算指标，查询延迟可降一到两个数量级。

### CFS Burst（减少误判限流）

Kubernetes v1.25+ 支持在节点上开启 CFS burst（`cpu.cfs_burst_us`），允许容器在短时突发下突破 limit，减少 throttle。可通过 kubelet feature gate 或运行时配置开启。若告警发现 throttle 持续但应用无延迟感知，可评估开启 burst 而非盲目调高 limit。

---

## 排障

### 端点连通性验证（只读，最常用）

```bash
# 🟢 低风险：通过 kubectl proxy 访问 kubelet 指标（复用 kubeconfig 凭证，最安全）
# 先在另一个终端启动 proxy
kubectl proxy --port=8001 &
# 然后查询
curl -s http://localhost:8001/api/v1/nodes/<node-name>/proxy/metrics/cadvisor | head -30

# 🟢 低风险：直接用 kubectl get --raw（等价于上一条，无需单独 proxy）
kubectl get --raw "/api/v1/nodes/<node-name>/proxy/metrics/cadvisor" | grep "^# HELP container_cpu"

# 🟢 低风险：列出某节点所有 metrics 端点返回的指标计数
for path in metrics metrics/cadvisor metrics/probes; do
  echo "=== /$path ==="
  kubectl get --raw "/api/v1/nodes/<node-name>/proxy/$path" | grep -c "^[a-z]"
done
```

### 直接访问 10250（需 kubeconfig 凭证）

```bash
# 🟡 中风险：直接 curl 10250，需手动提供证书/token；生产环境避免在 CI 中使用
# 获取节点 IP
NODE_IP=$(kubectl get nodes <node-name> -o jsonpath='{.status.addresses[?(@.type=="InternalIP")].address}')

# 用 client cert（从 kubeconfig 提取）
curl -s --cert /path/to/client.crt --key /path/to/client.key \
  --cacert /path/to/ca.crt \
  https://$NODE_IP:10250/metrics/cadvisor | head

# 用 bearer token（从 ServiceAccount）
SA_TOKEN=$(kubectl create token prometheus -n monitoring)
curl -s -k -H "Authorization: Bearer $SA_TOKEN" \
  https://$NODE_IP:10250/metrics/cadvisor | head
```

> **安全提示**：直接访问 10250 暴露节点凭证，且 `insecure (-k)` 仅用于排障。生产监控配置必须用 CA 校验。

### 验证容器层（crictl）

当 cAdvisor 指标与预期不符（如某容器无 CPU 数据），用 `crictl` 从 CRI 层验证：

```bash
# 🟢 低风险：列出节点上所有容器（需 SSH 到节点）
crictl ps -v | grep -A5 "Name: <container-name>"

# 🟢 低风险：查看容器 cgroup 路径，确认 cAdvisor 读取来源
crictl inspect <container-id> | grep cgroupsPath
```

### 检查 Prometheus 抓取状态

```bash
# 🟢 低风险：查看 Prometheus 是否成功抓取 kubelet/cadvisor target
# 访问 Prometheus Web UI: http://<prometheus>/targets
# 或用 API：
curl -s http://<prometheus>:9090/api/v1/targets \
  | jq '.data.activeTargets[] | select(.labels.job|test("kubelet|cadvisor")) | {job: .labels.job, health, lastError, scrapeUrl}'
```

常见 `lastError`：

| lastError | 含义 | 修复 |
|-----------|------|------|
| `server returned HTTP status 401` | 未认证 | 检查 `bearer_token_file` / client cert 是否生效 |
| `server returned HTTP status 403` | RBAC 拒绝 | 绑定 `system:monitoring` 到 Prometheus ServiceAccount |
| `x509: certificate signed by unknown authority` | TLS 校验失败 | 配置正确的 `ca_file`（用 ServiceAccount 的 ca.crt） |
| `context deadline exceeded` | 网络不通/超时 | 检查网络安全策略、kubelet 是否存活 |

### 常见问题速查

| 现象 | 可能根因 | 排查 |
|------|----------|------|
| `/metrics/cadvisor` 返回空 | cAdvisor 初始化失败 | 查 kubelet 日志 `grep -i cadvisor`；通常 cgroup v2 兼容问题 |
| `container_cpu_*` 缺失 | cgroup v2 下某些文件路径不同 | 确认 kubelet 版本 ≥ v1.25 且 cAdvisor 版本匹配 |
| 指标有但 Pod 维度不全 | `id`/`image_id` label 暴涨被 drop | 检查 metric_relabel 是否误删 `pod` label |
| 抓取 401/403 | RBAC 缺失 | 绑定 `system:monitoring`，确认 SA token 有效 |
| PLEG 延迟突增 | 容器运行时卡顿 | `crictl info`、查 containerd 日志、节点负载 |
| 卷指标阻塞 kubelet | CSI 驱动慢 | 查 `kubelet_volume_metric_collection_duration_seconds`，临时关闭 `--enable-volume-snapshotter` 相关采集 |

### cgroup v2 字段缺失

cgroup v2 统一层级下，cAdvisor 部分指标语义变化：

| 指标（v1） | cgroup v2 行为 |
|------------|----------------|
| `container_cpu_cfs_*` | 路径变为 `cpu.max`，数值可获取 |
| `container_memory_*` | 路径变为 `memory.current` / `memory.max`，working_set 计算略不同 |
| `container_cpu_load_average_10s` | v2 下可能为空（依赖 `cpu.stat`） |
| 部分 `container_fs_*` | overlayfs 统计口径变化 |

若升级到 cgroup v2 后某些历史告警失效，优先检查指标是否仍存在。

### kubelet 日志排查

```bash
# 🟢 低风险：查看 kubelet 日志中的 cAdvisor / metrics 相关错误
kubectl debug node/<node-name> -it --image=busybox -- chroot /host journalctl -u kubelet --no-pager | grep -iE "cadvisor|metrics|pleg"

# 🟢 低风险：静态 Pod 形式部署的 kubelet，直接看容器日志
crictl logs $(crictl ps --name kubelet -q) 2>&1 | grep -iE "cadvisor|metrics" | tail -50
```

---

## 检查清单

### 部署 cAdvisor/kubelet 指标采集前

- [ ] 确认所有节点 kubelet 监听 10250（`--port=10250`），10255 已关闭
- [ ] kubelet 启用鉴权：`--authorization-mode=Webhook`、`--authentication-token-webhook=true`
- [ ] Prometheus ServiceAccount 已绑定 `system:monitoring` ClusterRole
- [ ] scrape config 使用 `scheme: https` + `bearer_token_file`，未用 `insecure_skip_verify`
- [ ] cadvisor job 的 `metrics_path` 设为 `/metrics/cadvisor`
- [ ] 已 drop 高基数 label（`id`、`image_id`、`name`）
- [ ] 关键 recording rule 已配置（CPU 使用率、限流率、内存占比）
- [ ] 告警规则覆盖 PLEG 延迟、运行时错误率、CPU throttle、内存压力

### 上线后验证

- [ ] Prometheus `/targets` 中 cadvisor/kubelet job 全部 UP
- [ ] `container_cpu_usage_seconds_total` 在每个业务 Pod 都有数据
- [ ] `container_memory_working_set_bytes` 与应用自报内存量级一致
- [ ] `kubelet_pleg_relist_duration_seconds` 在正常范围（P99 < 3s）
- [ ] 指标基数稳定（`prometheus_tsdb_head_series` 不随容器重启无限增长）

---

## 相关文档

- [[09-可观测性/02-指标/01-prometheus-enterprise-monitoring.md|Prometheus 企业级监控]] — Prometheus 部署、联邦与长期存储
- [[09-可观测性/02-指标/02-monitoring-metrics-system.md|监控指标体系]] — 指标体系总览与本篇的位置
- [[09-可观测性/02-指标/19-kube-state-metrics-deep-dive.md|kube-state-metrics 深度解析]] — 对象状态指标，与 cAdvisor 互补
- [[01-集群基础/03-控制平面/15-kubelet-deep-dive.md|kubelet 深度剖析]] — kubelet 架构、PLEG、CRI 交互细节
- [[09-可观测性/06-SLO-SLI/04-sli-definition-selection.md|SLI 定义与选择方法论]] — kubelet 运行时错误率 SLI 的上游定义
- [[09-可观测性/02-指标/10-monitoring-metrics-prometheus.md|监控和指标表]] — 各组件关键指标速查表

---

<!-- risk-assessed -->
