# Kepler

> **成熟度**: Sandbox | **加入时间**: 2023-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://sustainable-computing.io |
| **GitHub** | https://github.com/sustainable-computing-io/kepler |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, C (eBPF) |
| **CNCF 分类** | Observability |
| **适用场景** | Kubernetes 能耗监控 |

---

## 项目概述

Kepler (Kubernetes-based Efficient Power Level Exporter) 使用 eBPF 探测器采集系统计数器，结合机器学习模型估算 Kubernetes Pod 和节点级别的能耗。它将能耗数据导出为 Prometheus 指标，帮助组织了解工作负载的碳足迹，支持可持续计算和绿色IT决策。

---

## 核心特性

- **eBPF 采集**: 低开销的内核级能耗数据采集
- **Pod 级别能耗**: 精确到 Pod 和容器的能耗估算
- **多硬件支持**: CPU (RAPL)、GPU (NVML)、DRAM
- **ML 模型**: 机器学习辅助能耗估算
- **Prometheus 导出**: 标准 Prometheus 指标格式
- **Grafana 仪表板**: 预置可视化面板

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      Kepler Architecture                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   Visualization Layer                     │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │  Grafana Dashboard                                  │ │   │
│  │  │  ┌──────────────┐  ┌──────────────┐  ┌───────────┐ │ │   │
│  │  │  │ Pod Energy   │  │ Node Energy  │  │ Carbon    │ │ │   │
│  │  │  │ Consumption  │  │ Breakdown    │  │ Footprint │ │ │   │
│  │  │  └──────────────┘  └──────────────┘  └───────────┘ │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │                   Prometheus                              │   │
│  │  kepler_container_joules_total{...}                       │   │
│  │  kepler_node_package_joules_total{...}                    │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │              Kepler Exporter (DaemonSet)                  │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │                Core Components                       │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │   eBPF      │  │   Energy    │  │   ML       │  │ │   │
│  │  │  │   Probes    │  │   Estimator │  │   Model    │  │ │   │
│  │  │  └──────┬──────┘  └──────┬──────┘  └─────┬──────┘  │ │   │
│  │  │         │                │               │          │ │   │
│  │  │  ┌──────▼──────┐  ┌─────▼──────┐  ┌─────▼──────┐  │ │   │
│  │  │  │ Hardware    │  │ cgroup     │  │ Prometheus │  │ │   │
│  │  │  │ Counters    │  │ Metrics    │  │ Exporter   │  │ │   │
│  │  │  │ (RAPL,NVML) │  │            │  │            │  │ │   │
│  │  │  └─────────────┘  └────────────┘  └────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────────────────────────────────┘  │
│                              │                                   │
│  ┌───────────────────────────▼───────────────────────────────┐  │
│  │                   Hardware Layer                           │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌───────────┐  │  │
│  │  │ CPU RAPL │  │  GPU     │  │  DRAM    │  │ Platform  │  │  │
│  │  │ Counters │  │  NVML    │  │  Energy  │  │ Power     │  │  │
│  │  └──────────┘  └──────────┘  └──────────┘  └───────────┘  │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### Helm 安装

```bash
helm repo add kepler https://sustainable-computing-io.github.io/kepler-helm-chart
helm repo update

helm install kepler kepler/kepler \
  --namespace kepler \
  --create-namespace \
  --set serviceMonitor.enabled=true

kubectl get pods -n kepler
```

### Manifest 安装

```bash
kubectl apply -f https://raw.githubusercontent.com/sustainable-computing-io/kepler/main/manifests/kubernetes/deployment.yaml
```

---

## Prometheus 指标

### 关键指标

| 指标 | 说明 |
|:---|:---|
| `kepler_container_joules_total` | 容器总能耗 (焦耳) |
| `kepler_container_core_joules_total` | 容器 CPU 核能耗 |
| `kepler_container_dram_joules_total` | 容器 DRAM 能耗 |
| `kepler_container_gpu_joules_total` | 容器 GPU 能耗 |
| `kepler_node_package_joules_total` | 节点 CPU 包能耗 |
| `kepler_node_platform_joules_total` | 节点平台总能耗 |

### PromQL 查询

```promql
# Pod 每秒功耗 (瓦特)
rate(kepler_container_joules_total{container_namespace="production"}[5m])

# 节点总功耗
sum(rate(kepler_node_platform_joules_total[5m])) by (instance)

# 命名空间能耗排行
topk(10, sum(rate(kepler_container_joules_total[1h])) by (container_namespace))

# GPU 功耗
sum(rate(kepler_container_gpu_joules_total[5m])) by (container_name)
```

---

## Grafana 仪表板

```bash
# 导入预置仪表板
# Grafana Dashboard ID: 15654 (Kepler Exporter)

# 或下载 JSON
curl -LO https://raw.githubusercontent.com/sustainable-computing-io/kepler/main/grafana-dashboards/Kepler-Exporter.json
```

---

## 最佳实践

1. **RAPL 支持**: 确保内核支持 Intel RAPL 或 AMD Energy
2. **权限配置**: Kepler 需要特权访问 /sys 和 /proc
3. **ML 模型**: 在不支持 RAPL 的环境中使用 ML 估算
4. **碳转换**: 结合区域碳强度数据计算碳足迹
5. **告警规则**: 设置能耗异常告警
6. **优化决策**: 基于能耗数据优化工作负载调度

---

## 参考资源

- [官方文档](https://sustainable-computing.io)
- [GitHub Repo](https://github.com/sustainable-computing-io/kepler)
- [指标参考](https://sustainable-computing.io/design/metrics/)
- [Grafana Dashboard](https://grafana.com/grafana/dashboards/15654)

---

**维护者**: Kudig Team | **许可证**: MIT
