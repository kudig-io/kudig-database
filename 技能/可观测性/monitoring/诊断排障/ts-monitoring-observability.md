---
title: 监控可观测性排查
description: '# 监控可观测性排查'
summary: '1. **核心组件就绪**：Prometheus/Grafana/Loki/AlertManager Pod 是否 Running。'
category: skills
tags:
- k8s
- troubleshooting
- structural
- monitoring-observability
- prometheus
- grafana
- jaeger
- cilium
- kafka
- elasticsearch
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 监控可观测性排查 是什么
- 如何 监控可观测性排查
trigger_keywords:
- 监控可观测性排查
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- kafka-basics
- gpu-scheduling-basics
- logging-basics
- tracing-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 监控可观测性排查

### 01 Monitoring Observability TroubleshootingObservability）|Observability]] Troubleshooting

#### 可观测性核心组件问题现象

| 问题类型 | 典型现象 | 影响程度 | 紧急级别 |
|---------|---------|---------|---------|
| Prometheus 数据采集失败 | `scrape failed` 持续出现 | ⭐⭐⭐ 高 | P0 |
| Grafana 仪表板无法加载 | `dashboard not found` 或空白页面 | ⭐⭐ 中 | P1 |
| Loki 日志查询超时 | `query timeout` 或 `context deadline exceeded` | ⭐⭐⭐ 高 | P0 |
| Jaeger 链路追踪不完整 | `trace not found` 或 spans 缺失 | ⭐⭐ 中 | P1 |
| AlertManager 告警风暴 | 大量重复告警或告警丢失 | ⭐⭐⭐ 高 | P0 |
| Metrics Server 不可用 | `metrics not available` 导致 HPA 失效 | ⭐⭐⭐ 高 | P0 |
| 监控数据存储爆满 | `disk full` 或 `retention exceeded` | ⭐⭐⭐ 高 | P0 |
| 多集群监控数据孤岛 | 跨集群指标无法聚合查询 | ⭐⭐ 中 | P1 |

#### 排查方法与步骤



#### 10 分钟快速诊断

1. **核心组件就绪**：Prometheus/Grafana/Loki/AlertManager Pod 是否 Running。
2. **采集状态**：Prometheus Targets 是否大量 down；ServiceMonitor/PodMonitor 是否匹配。
3. **存储压力**：TSDB/日志存储磁盘是否接近满水位。
4. **告警链路**：AlertManager 接收与路由是否正常。
5. **Metrics Server**：`kubectl top` 是否可用（影响 HPA）。
6. **快速缓解**：
   - 临时提升资源并缩短采样或保留周期。
   - 对关键告警先设置抑制避免风暴。
7. **证据留存**：保存 targets 状态、存储水位、告警路由与组件日志。

---

### 02 Opentelemetry Troubleshooting

#### 0. 10 分钟快速诊断

1. **Collector Pod 状态**：`kubectl get pods -n observability -l app.kubernetes.io/name=opentelemetry-collector`，确认 Running 且无频繁重启。
2. **组件健康检查**：`curl http://://<collector-pod>:13133/` 或访问 `/health` endpoint，确认 Collector 自身健康。
3. **接收端口连通性**：从客户端 Pod `telnet/nc` 测试 Collector Service 的接收端口（4317 gRPC / 4318 HTTP）。
4. **Exporter 错误**：查看 Collector 日志中的 `error` 和 `refused` 关键字，定位导出失败。
5. **指标自查**：访问 `http://://<collector-pod>:8888/metrics`，查看 `otelcol_exporter_send_failed_*` 和 `otelcol_receiver_refused_*`。
6. **快速缓解**：
   - 后端不可达：临时切换 exporter 到 `debug` 或 `file` 避免数据丢失。
   - 内存溢出：调大 Collector 内存限制，或启用 `memory_limiter` processor。
   - 接收拒绝：扩容 Collector 副本数或增大队列缓冲。
7. **证据留存**：保存 Collector 配置 ConfigMap、Pod 日志、metrics 快照、客户端 SDK 配置。

---

#### 2. 排查方法与步骤



#### 2.1 诊断原理说明

OpenTelemetry Collector 的数据流架构：

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  Receivers  │────▶│  Processors │────▶│   Exporters │────▶│  Backends   │
│  (接收)      │     │  (处理)      │     │  (导出)      │     │  (后端存储)  │
├─────────────┤     ├─────────────┤     ├─────────────┤     ├─────────────┤
│ OTLP/gRPC   │     │ batch       │     │ OTLP        │     │ Prometheus  │
│ OTLP/HTTP   │     │ memory_limit│     │ Jaeger      │     │ Jaeger      │
│ Prometheus  │     │ resource    │     │ Zipkin      │     │ Zipkin      │
│ Jaeger      │     │ attributes  │     │ Kafka       │     │ Elasticsearch│
│ Zipkin      │     │ tail_sampling│    │ file        │     │ Kafka       │
│ Kafka       │     │ probabilistic│    │ debug       │     │ CloudWatch  │
└─────────────┘     └─────────────┘     └─────────────┘     └─────────────┘
        │
        ▼
┌─────────────┐
│ Extensions  │
│ - health_check│
│ - pprof      │
│ - zpages     │
│ - file_storage│
└─────────────┘
```

**关键概念**：
- **Pipeline**：数据在 Collector 中的处理路径，分为 `traces`、`metrics`、`logs` 三种类型
- **Batch Processor**：将数据批处理以减少后端调用次数，但会增加延迟
- **Memory Limiter**：监控内存使用并在达到阈值时拒绝新数据，防止 OOM
- **Sending Queue**：Exporter 内部的队列，用于缓冲和重试

---

### 03 Ebpf Observability Troubleshooting

#### 0. 10 分钟快速诊断

1. **eBPF 程序加载状态**：`bpftool prog show` 或 `cilium status`，确认 eBPF 程序已加载到内核。
2. **内核版本兼容性**：`uname -r`，确认内核版本 >= 5.4（部分功能需 >= 5.10）。
3. **BTF 可用性**：`ls /sys/kernel/btf/vmlinux`，确认内核暴露 BTF 信息。
4. **Cilium/Pixie Pod 状态**：`kubectl get pods -n kube-system -l k8s-app=cilium` 或 `olm/px-operator`。
5. **Hubble/Tetragon 可见性**：`hubble status` 或查看 Tetragon Pod 日志中的 `tetragon` 事件。
6. **快速缓解**：
   - eBPF 加载失败：检查内核配置是否启用 `CONFIG_BPF`、`CONFIG_BPF_SYSCALL`。
   - Hubble 无流量数据：确认 Cilium 的 `hubble.listenAddress` 配置和 relay 连接。
   - Tetragon 事件丢失：增大 ringbuf 大小或调整事件过滤条件。
7. **证据留存**：保存 `bpftool` 输出、内核配置 `/boot/config-$(uname -r)`、Cilium/Pixie 组件日志。

---

#### 2. 排查方法与步骤



#### 2.1 诊断原理说明

eBPF（extended Berkeley Packet Filter）允许在内核中安全执行沙箱程序。可观测性场景中的 eBPF 架构：

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
┌─────────────────────────────────────────────────────────────────┐
│                         用户空间 (User Space)                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐             │
│  │ Cilium Agent │  │ Hubble Relay│  │ Tetragon    │             │
│  │ Pixie PEM    │  │ CLI/UI      │  │ CLI         │             │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘             │
│         │                │                │                    │
│         │  ringbuf/perf  │    gRPC        │  protobuf          │
│         │  buffer        │                │                    │
├─────────┼────────────────┼────────────────┼────────────────────┤
│         ▼                ▼                ▼                    │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │                    eBPF 虚拟机 (eBPF VM)                   │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐       │ │
│  │  │ kprobe/trace│  │ sockops     │  │ XDP/TC      │       │ │
│  │  │ point 程序   │  │ socket 程序  │  │ 网络程序     │       │ │
│  │  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘       │ │
│  │         │                │                │              │ │
│  │         ▼                ▼                ▼              │ │
│  │  ┌─────────────────────────────────────────────────────┐ │ │
│  │  │              eBPF Maps (Key-Value Store)             │ │ │
│  │  │  - 连接跟踪表 (CT)         
...(截断)

---

### 04 Finops Cost Optimization Troubleshooting

#### 0. 10 分钟快速诊断

1. **成本总览**：`kubectl port-forward -n kubecost deployment/kubecost-cost-analyzer 9090`，访问 `/overview.html` 查看成本趋势。
2. **异常飙升检测**：对比本周与上周的 namespace/Deployment 成本，定位异常增长来源。
3. **闲置资源扫描**：查看 Kubecost 的 "Savings" 页面，识别未挂载的 PV、低利用率节点、过度配置的 Pod。
4. **Spot/Preemptible 利用率**：检查 Spot 实例占总计算成本的比例，评估优化空间。
5. **计费对齐**：将 Kubecost 的估算与云厂商账单对比，偏差 >20% 时需检查折扣、预留实例、分摊规则。
6. **快速缓解**：
   - 闲置资源：通过 Kubecost API 获取闲置资源列表并清理。
   - 过度配置：使用 VPA 推荐值调整 Pod requests/limits。
   - 异常飙升：找到导致成本激增的 Pod/Job 并限制资源。
7. **证据留存**：保存成本趋势截图、闲置资源报告、优化前后的资源配置对比。

---

#### 2. 排查方法与步骤



#### 2.1 诊断原理说明

云原生成本管理架构：

```
┌─────────────────────────────────────────────────────────────────┐
│                    云厂商计费层 (Cloud Billing)                   │
│  AWS Cost Explorer / Azure Cost Management / GCP Billing        │
├─────────────────────────────────────────────────────────────────┤
│                    成本汇聚层 (Cost Aggregation)                  │
│  Kubecost / OpenCost / CloudHealth / Vantage                    │
├─────────────────────────────────────────────────────────────────┤
│                    指标采集层 (Metrics Collection)                │
│  Prometheus (kube-state-metrics, cAdvisor, node-exporter)       │
├─────────────────────────────────────────────────────────────────┤
│                    Kubernetes 资源层                              │
│  Nodes | Pods | PVCs | Services | LoadBalancers | Ingress       │
└─────────────────────────────────────────────────────────────────┘
```

**关键概念**：
- **成本分摊 (Cost Allocation)**：将节点成本按 CPU/内存/GPU requests 分摊到各个 Pod/namespace
- **闲置成本 (Idle Cost)**：节点已分配但未使用的资源对应的成本
- **分摊成本 (Shared Cost)**：系统组件、监控、日志等公共资源的成本分摊方式
- **折扣归集**：预留实例 (RI)、Saving Plans、Spot 折扣在成本展示中的处理方式

## 相关链接

- [[技能/可观测性/monitoring/monitor-kubernetes-metrics.md|K8s 监控指标]]

## Related

- [[opencost]] — OpenCost
- [[jaeger]] — Jaeger
- [[pixie]] — Pixie
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[技能/可观测性/monitoring/monitoring-fta.md|监控与告警异常故障树分析]] — Cross-reference


<!-- risk-assessed -->
