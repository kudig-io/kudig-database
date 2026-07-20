---
title: "eBPF 可观测性深度实践"
description: "基于 eBPF 的零侵入可观测性：Cilium Hubble 网络可视化、Pixie 应用监控、Tetragon 安全审计与 K8s 生产部署"
summary: "深入解析 eBPF 探针原理及其在 Kubernetes 可观测性中的应用，覆盖 Cilium Hubble 网络流量可视化、Pixie 全栈应用监控、Tetragon 运行时安全审计，以及零侵入监控的生产部署与故障排查"
category: 可观测性
tags:
- ebpf
- cilium-hubble
- pixie
- tetragon
- network-observability
- security-audit
- zero-instrumentation
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- "eBPF 如何实现零侵入 Kubernetes 监控"
- "Cilium Hubble 网络流量可视化部署配置"
- "Tetragon 安全审计策略配置"
trigger_keywords:
- ebpf
- hubble
- pixie
- tetragon
- cilium
- 零侵入
- 网络可视化
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# eBPF 可观测性深度实践

## 概述

eBPF（extended Berkeley Packet Filter）是 Linux 内核提供的可编程运行时框架，允许在内核空间安全地执行自定义程序而无需修改内核源码或加载内核模块。在 Kubernetes 可观测性领域，eBPF 实现了真正的"零侵入"监控——无需修改应用代码、无需注入 Sidecar、无需重启 Pod，即可获取网络流量、系统调用、性能剖析等深层遥测数据。

本文覆盖三大 eBPF 可观测性工具：Cilium Hubble（网络流量可视化与策略验证）、Pixie（全栈应用性能监控）、Tetragon（运行时安全审计与威胁检测），并提供生产环境的部署模式、性能影响评估和故障排查方法。

与 [[可观测性/总览/01-observability-architecture-overview.md|可观测性架构总览]] 中描述的传统 Agent 采集模式不同，eBPF 方案直接在内核层捕获事件，避免了应用层 SDK 集成的侵入性和维护成本。

## 核心概念

### eBPF 探针原理

eBPF 程序通过挂载到内核 Hook 点来捕获事件。核心 Hook 类型包括：

```
┌─────────────────────────────────────────────────────────────┐
│                    eBPF 探针挂载点                            │
│                                                               │
│  用户空间                                                     │
│  ┌─────────────────────────────────────────────────────┐     │
│  │  Application Process                                │     │
│  │  • uprobe: 函数入口/出口追踪                         │     │
│  │  • uretprobe: 函数返回值捕获                         │     │
│  └─────────────────────────────────────────────────────┘     │
│                          │ syscall                            │
│  ────────────────────────┼────────────────────────────────── │
│  内核空间                 ▼                                   │
│  ┌─────────────────────────────────────────────────────┐     │
│  │  • tracepoint: 静态内核追踪点 (sched, net, fs)       │     │
│  │  • kprobe/kretprobe: 动态内核函数追踪                │     │
│  │  • XDP: 网络包最早处理点 (DDoS 过滤)                │     │
│  │  • TC (Traffic Control): 网络包入/出口处理           │     │
│  │  • cgroup: 容器级资源与网络控制                      │     │
│  │  • LSM: 安全模块 Hook (权限检查)                     │     │
│  └─────────────────────────────────────────────────────┘     │
│                          │                                    │
│  ┌───────────────────────▼─────────────────────────────┐     │
│  │  eBPF Map (数据共享): Hash / Array / Ring Buffer     │     │
│  └─────────────────────────────────────────────────────┘     │
│                          │                                    │
│  ────────────────────────┼────────────────────────────────── │
│  用户空间                 ▼                                   │
│  ┌─────────────────────────────────────────────────────┐     │
│  │  Userspace Agent (Hubble/Pixie/Tetragon)            │     │
│  │  • 从 eBPF Map 读取事件                             │     │
│  │  • 聚合、富化、导出遥测数据                          │     │
│  └─────────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

### 三大工具定位对比

| 维度 | Cilium Hubble | Pixie | Tetragon |
|------|--------------|-------|----------|
| 核心定位 | 网络可观测性与策略验证 | 全栈 APM（网络+应用+基础设施） | 运行时安全审计与威胁检测 |
| 数据来源 | TC/XDP Hook（L3-L7） | kprobe/uprobe/tracepoint | tracepoint/LSM/kprobe |
| 协议解析 | HTTP, gRPC, DNS, Kafka, MySQL | HTTP, gRPC, DNS, MySQL, PG, Redis, Kafka | 进程、文件、网络、权限事件 |
| 侵入性 | 零（依赖 Cilium CNI） | 零（内核级采集） | 零（内核级采集） |
| 数据存储 | 本地 Ring Buffer + Prometheus | 集群内边缘存储（Vizier） | 导出至 SIEM/日志系统 |
| 性能开销 | 1-3% CPU | 2-5% CPU | 1-3% CPU |
| 适用场景 | 微服务网络拓扑、DNS 诊断、NetworkPolicy 验证 | 无需 SDK 的 APM、协议级延迟分析 | 合规审计、入侵检测、数据泄露防护 |

### 零侵入监控的价值

传统可观测性方案（如 [[可观测性/链路追踪/03-opentelemetry-distributed-tracing.md|OpenTelemetry SDK]]）需要修改应用代码或注入 Agent，存在以下痛点：多语言 SDK 维护成本高、遗留系统无法接入、SDK 版本升级需要重新部署。eBPF 方案完全绕过这些限制，在内核层统一采集所有语言、所有框架的遥测数据。

## 生产部署/实现

### Cilium Hubble 部署

Hubble 作为 Cilium CNI 的可观测性组件，在已部署 Cilium 的集群中可直接启用：

```yaml
# 🟡 中风险：修改 Cilium 配置需要滚动重启所有节点上的 Cilium Agent
# 通过 Helm values 启用 Hubble
# helm upgrade cilium cilium/cilium -n kube-system --reuse-values -f hubble-values.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: hubble-enable-values
  namespace: kube-system
data:
  values.yaml: |
    hubble:
      enabled: true
      relay:
        enabled: true
        replicas: 2
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 512Mi
      ui:
        enabled: true
        replicas: 2
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
      metrics:
        enabled:
        - dns
        - http
        - kafka
        - grpc
        serviceMonitor:
          enabled: true
      tls:
        auto:
          enabled: true
          method: cronJob
      export:
        dynamic:
          enabled: true
          config:
            content: |
              - name: all-events
                filePath: /var/run/cilium/hubble-events.log
                includeFilters: []
                excludeFilters: []
```

### Pixie 生产部署

Pixie Vizier 部署在集群内部，数据默认存储在边缘（不离开集群），满足数据主权要求：

```yaml
# 🟡 中风险：Pixie 需要特权容器和内核头文件访问
apiVersion: v1
kind: Namespace
metadata:
  name: pl
---
# 使用 Pixie Operator 部署（推荐方式）
# px deploy --cluster_name prod-cluster --deploy_key $PIXIE_DEPLOY_KEY
#
# 关键配置项（通过 px deploy 参数或 Helm values）：
# - --data_collector_resources: 控制采集器资源限制
# - --pem_memory_limit: 每个 PEM（Pixie Edge Module）的内存限制
# - --data_retention: 数据保留时间（默认 24h）
#
# 生产环境推荐配置：
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: pixie-pem
  namespace: pl
  labels:
    app: pixie-pem
spec:
  selector:
    matchLabels:
      app: pixie-pem
  template:
    metadata:
      labels:
        app: pixie-pem
    spec:
      hostPID: true
      hostNetwork: true
      containers:
      - name: pem
        image: pixie/pixie-oss-pem:latest
        securityContext:
          privileged: true
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: "2"
            memory: 4Gi
        env:
        - name: PL_DATA_RETENTION
          value: "24h"
        - name: PL_MAX_DATA_PER_TABLE
          value: "1000000"
        volumeMounts:
        - name: sys
          mountPath: /sys
          readOnly: true
        - name: proc
          mountPath: /host/proc
          readOnly: true
        - name: debugfs
          mountPath: /sys/kernel/debug
          readOnly: true
      volumes:
      - name: sys
        hostPath:
          path: /sys
      - name: proc
        hostPath:
          path: /proc
      - name: debugfs
        hostPath:
          path: /sys/kernel/debug
      tolerations:
      - operator: Exists
```

### Tetragon 安全审计部署

Tetragon 专注于运行时安全，通过 eBPF 追踪进程执行、文件访问、网络连接和权限变更：

```yaml
# 🟡 中风险：Tetragon 需要内核级权限，TracingPolicy 可能影响性能
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: security-audit-policy
  namespace: kube-system
spec:
  kprobes:
  # 追踪敏感文件读取
  - call: "fd_install"
    syscall: false
    args:
    - index: 0
      type: "int"
    - index: 1
      type: "file"
    selectors:
    - matchArgs:
      - index: 1
        operator: "In"
        values:
        - "/etc/shadow"
        - "/etc/passwd"
        - "/root/.ssh/*"
        - "/var/run/secrets/kubernetes.io/*"
  # 追踪特权进程执行
  - call: "execve"
    syscall: false
    args:
    - index: 0
      type: "string"
    selectors:
    - matchArgs:
      - index: 0
        operator: "In"
        values:
        - "/usr/bin/curl"
        - "/usr/bin/wget"
        - "/usr/bin/nc"
        - "/usr/bin/python*"
        - "/bin/sh"
        - "/bin/bash"
  # 追踪网络连接建立
  - call: "tcp_connect"
    syscall: false
    args:
    - index: 0
      type: "sock"
---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: tetragon
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: tetragon
  template:
    metadata:
      labels:
        app.kubernetes.io/name: tetragon
    spec:
      hostPID: true
      containers:
      - name: tetragon
        image: quay.io/cilium/tetragon:v1.1.0
        securityContext:
          privileged: true
        env:
        - name: TETRAGON_EXPORT_FILENAME
          value: /var/log/tetragon/events.json
        - name: TETRAGON_EXPORT_FILE_MAX_SIZE_MB
          value: "100"
        - name: TETRAGON_EXPORT_FILE_ROTATION_INTERVAL
          value: "24h"
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: "1"
            memory: 1Gi
        volumeMounts:
        - name: bpf-maps
          mountPath: /sys/fs/bpf
          mountPropagation: Bidirectional
        - name: debugfs
          mountPath: /sys/kernel/debug
        - name: logs
          mountPath: /var/log/tetragon
      volumes:
      - name: bpf-maps
        hostPath:
          path: /sys/fs/bpf
      - name: debugfs
        hostPath:
          path: /sys/kernel/debug
      - name: logs
        hostPath:
          path: /var/log/tetragon
          type: DirectoryOrCreate
      tolerations:
      - operator: Exists
```

## 运维操作

### Hubble 网络流量查询

```bash
# 🟢 低风险：只读查询操作
# 安装 Hubble CLI
# brew install hubble (macOS) 或从 GitHub Releases 下载

# 端口转发 Hubble Relay
kubectl port-forward -n kube-system svc/hubble-relay 4245:80 &

# 查看集群网络流量拓扑
hubble observe --all-namespace --since 5m

# 按服务过滤 HTTP 请求
hubble observe --namespace production --to-service payment-service --protocol http

# 查看 DNS 查询（诊断 DNS 解析问题）
hubble observe --namespace production --type l7 --protocol dns

# 查看被 NetworkPolicy 拒绝的流量
hubble observe --verdict DROPPED --namespace production

# 导出流量数据用于离线分析
hubble observe --namespace production --since 1h -o json > traffic-audit.json
```

### Pixie 查询与诊断

```bash
# 🟢 低风险：只读查询
# 通过 Pixie CLI (px) 执行 PxL 查询
px run px/cluster --cluster prod-cluster

# 查看 HTTP 服务延迟分布（无需 SDK）
# 在 Pixie UI 中执行 PxL 脚本：
# import px
# df = px.DataFrame(table='http_events', start_time='-5m')
# df = df[df.namespace == 'production']
# df = df.groupby('service').agg(
#   p50_latency=('latency', px.quantile(0.5)),
#   p99_latency=('latency', px.quantile(0.99)),
#   error_rate=('resp_status', lambda x: (x >= 500).mean())
# )
# px.display(df)
```

### Tetragon 安全事件查看

```bash
# 🟢 低风险：只读查看安全事件
# 实时查看安全事件流
kubectl logs -n kube-system -l app.kubernetes.io/name=tetragon -f --tail=50

# 查看特定 Pod 的进程执行事件
kubectl exec -n kube-system daemonset/tetragon -- \
  tetra getevents -o json | jq 'select(.process.pod == "payment-service-xxx")'

# 导出审计日志到 SIEM
kubectl exec -n kube-system daemonset/tetragon -- \
  cat /var/log/tetragon/events.json | \
  curl -X POST -H "Content-Type: application/json" \
  -d @- https://siem.internal/api/v1/events
```

## 故障排查

### eBPF 程序加载失败

eBPF 程序对内核版本有最低要求（通常 4.19+，推荐 5.10+），加载失败是部署阶段最常见问题：

```bash
# 🟢 低风险：只读诊断
# 检查节点内核版本
kubectl get nodes -o custom-columns=NAME:.metadata.name,KERNEL:.status.nodeInfo.kernelVersion

# 检查 BPF 文件系统是否挂载
kubectl exec -n kube-system daemonset/tetragon -- mount | grep bpf

# 检查 eBPF 程序是否成功加载
kubectl exec -n kube-system daemonset/tetragon -- bpftool prog list

# 查看内核日志中的 eBPF 验证器错误
kubectl exec -n kube-system daemonset/tetragon -- dmesg | grep -i "bpf\|ebpf" | tail -20

# 确认内核配置（需要 CONFIG_BPF_SYSCALL=y, CONFIG_BPF_JIT=y）
kubectl exec -n kube-system daemonset/tetragon -- cat /boot/config-$(uname -r) | grep BPF
```

### Hubble 数据不完整

```bash
# 🟢 低风险：只读诊断
# 检查 Hubble Ring Buffer 是否溢出（数据丢失）
kubectl exec -n kube-system daemonset/cilium -- \
  cilium metrics list | grep hubble_drop

# 检查 Cilium Agent 的 Hubble 组件状态
kubectl exec -n kube-system daemonset/cilium -- cilium status --verbose | grep -A5 Hubble

# 验证 Relay 连接状态
kubectl logs -n kube-system deployment/hubble-relay --tail=50 | grep -i "error\|disconnect"
```

### 性能影响评估

```bash
# 🟢 低风险：只读性能评估
# 对比启用 eBPF 前后的节点 CPU 使用
kubectl top nodes --sort-by=cpu

# 检查 eBPF 程序的 CPU 占用
kubectl exec -n kube-system daemonset/tetragon -- \
  bpftool prog list --json | jq '.[] | {name, run_time_ns, run_cnt}'

# 监控 eBPF Map 内存使用
kubectl exec -n kube-system daemonset/cilium -- \
  cilium bpf metrics list
```

## 最佳实践

### 部署决策矩阵

1. **已使用 Cilium CNI**：直接启用 Hubble，零额外部署成本。Hubble 提供 L3-L7 网络可观测性，与 [[网络]] 策略深度集成。

2. **需要无 SDK 的 APM**：部署 Pixie，特别适合多语言微服务环境（Java、Go、Python、Node.js 混合部署），无需逐个服务接入 SDK。

3. **安全合规要求**：部署 Tetragon，满足运行时安全审计需求。与 [[安全]] 体系集成，将事件导出至 SIEM。

### 内核版本与兼容性

- 最低要求：Linux Kernel 4.19（基本 eBPF 功能）
- 推荐版本：5.10+（完整 BTF 支持、Ring Buffer、BPF LSM）
- 最佳版本：5.15+（所有 eBPF 特性可用，性能最优）
- 注意：部分云厂商的定制内核可能需要额外配置

### 生产环境注意事项

1. **资源预留**：eBPF 程序运行在内核空间，其 CPU 消耗计入系统 CPU 而非进程 CPU。为节点预留 5-10% 的额外 CPU 容量。

2. **数据保留策略**：Pixie 默认在边缘存储 24 小时数据。对于合规场景，配置 Tetragon 将事件实时导出至外部存储。

3. **与现有监控集成**：Hubble 指标通过 ServiceMonitor 接入 [[可观测性/指标/01-prometheus-enterprise-monitoring.md|Prometheus]]，安全事件通过 Fluent Bit 接入 [[可观测性/总览/06-elastic-stack-enterprise-observability.md|ELK/Loki]]。

4. **渐进式部署**：先在非关键节点池启用，观察 48 小时性能影响后再全集群推广。使用节点标签和 tolerations 控制部署范围。

### 安全审计策略设计

Tetragon TracingPolicy 应覆盖以下关键安全事件：
- 敏感文件访问（Secrets、SSH 密钥、系统密码文件）
- 异常进程执行（容器内不应出现的 shell、网络工具）
- 权限提升操作（setuid、setgid、capabilities 变更）
- 异常网络连接（连接已知恶意 IP、非标准端口外连）

## Related

- [[可观测性/总览/01-observability-architecture-overview.md|可观测性架构总览]]
- [[可观测性/链路追踪/03-opentelemetry-distributed-tracing.md|OpenTelemetry 分布式追踪]]
- [[可观测性/指标/01-prometheus-enterprise-monitoring.md|Prometheus 企业级监控]]
- [[可观测性/总览/06-elastic-stack-enterprise-observability.md|Elastic Stack 企业可观测性]]
- [[可观测性/告警/01-alertmanager-deep-configuration.md|Alertmanager 深度配置]]
- [[可观测性/总览/19-security-compliance-governance.md|安全合规治理]]
- [[可观测性/链路追踪/05-otel-collector-deep-configuration.md|OTel Collector 深度配置]]
