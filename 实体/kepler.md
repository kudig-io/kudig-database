---
title: Kepler [entities]
description: '## 概述'
summary: 'Kepler (Kubernetes-based Efficient Power Level Exporter) 使用 eBPF 探测器采集系统计数器，结合机器学习模型估算 Kubernetes Pod 和节点级别的能耗。它将能耗数据导出为 Prometheus 指标，帮助组织了解工作负载的碳足迹，支持可持续计算和绿色IT决策。'
category: entities
tags:
- k8s
- cncf
- cost
- kepler
- prometheus
- grafana
- cilium
- containerd
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kepler 是什么
- 如何 Kepler
trigger_keywords:
- Kepler
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kepler

> **CNCF 状态**: Sandbox | **类别**: Cost | **主要语言**: Go, C (eBPF)

## 概述

Kepler（Kubernetes-based Efficient Power Level Exporter）是由 Red Hat/Sustainable Computing 工作组开发的开源工具，2023 年加入 CNCF Sandbox。它使用 eBPF 探测器采集系统计数器，结合机器学习模型估算 Kubernetes Pod 和节点级别的能耗。Kepler 将能耗数据导出为 Prometheus 指标，帮助组织了解工作负载的碳足迹，支持可持续计算（Sustainable Computing）和绿色 IT 决策。

## 核心特性

- **eBPF 低开销采集**: 内核级系统计数器采集，极低性能开销
- **Pod 级别能耗**: 精确到 Pod 和容器粒度的能耗估算
- **多硬件支持**: CPU（Intel RAPL / AMD Energy）、GPU（NVML）、DRAM
- **ML 估算模型**: 在不支持 RAPL 的环境中使用机器学习模型估算能耗
- **Prometheus 导出**: 标准化的 `kepler_*` Prometheus 指标
- **Grafana 仪表盘**: 预置可视化面板展示能耗和碳足迹

## 架构

Kepler 以 DaemonSet 形式部署在每个节点上。核心组件包括：eBPF 程序（通过 bpftrace/perf 采集 CPU 周期、缓存引用等硬件计数器）、Kepler Exporter（聚合计数器，结合 RAPL 读数或 ML 模型计算能耗）、Power Model（预训练的机器学习模型，根据 CPU 利用率、指令数等特征估算瓦特级功耗）。能耗数据按 Pod 聚合（通过 cgroup ID 关联），导出为 Prometheus 指标（如 `kepler_pod_package_energy_millijoule`）。

## Kubernetes 集成

Kepler 通过 DaemonSet 部署在所有节点，以特权模式运行以加载 eBPF 程序和读取 RAPL（/sys/class/powercap）。自动发现 Pod 和容器元数据（通过 Kubernetes API 和 cgroup）。能耗指标按 Pod、Container、Node 三个维度导出。通过 ServiceMonitor 集成 Prometheus。支持通过 Kepler Operator 或 Helm Chart 部署。可与 Kepler Model Server 配合，动态训练和更新估算模型。

## 生产使用场景

1. **碳足迹追踪**: 量化每个微服务的能耗和碳排放，支持 ESG 报告
2. **能耗优化**: 基于能耗数据优化 Pod 调度，将高能耗工作负载调度到低碳电网区域
3. **FinOps 成本分析**: 结合云成本数据，计算单位产出的能耗效率
4. **可持续 K8s**: 为绿色 Kubernetes 运营提供数据基础

## 安装与配置

```bash
# Helm 安装
helm repo add kepler https://sustainable-computing.io/kepler
helm install kepler kepler/kepler -n kepler --create-namespace
# 验证指标
kubectl port-forward -n kepler svc/kepler-exporter 9102:9102
curl localhost:9102/metrics | grep kepler_pod
# Grafana Dashboard
# 导入 Grafana Dashboard ID: 18122
```

### 高级配置

```yaml
# kepler-values.yaml
powerModel:
  enabled: true
  modelServerEndpoint: "http://kepler-model-server:8100"
  requestInterval: 3000  # ms

exporter:
  port: 9102
  logLevel: info
  exposeHardwareMetrics: true

redfish:
  enabled: false  # 物理服务器 BMC 能耗采集
  # host: "192.168.1.100"
  # user: "admin"
  # pass: "password"

gpu:
  enabled: true  # NVIDIA GPU 能耗采集
```

### Prometheus 集成

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: kepler-monitor
  namespace: kepler
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: kepler-exporter
  endpoints:
  - port: http
    interval: 30s
    path: /metrics
```

## 运维操作

```bash
# 🟢 查看 Kepler DaemonSet 状态
kubectl get ds -n kepler kepler-exporter

# 🟢 查看节点能耗指标
kubectl exec -n kepler ds/kepler-exporter -- curl -s localhost:9102/metrics | grep kepler_node

# 🟢 查看特定 Pod 能耗
curl -s localhost:9102/metrics | grep 'kepler_pod_package_energy_millijoule{pod_name="my-app"}'

# 🟡 重启 Kepler DaemonSet（重新加载 eBPF 程序）
kubectl rollout restart ds/kepler-exporter -n kepler

# 🟢 检查 eBPF 程序是否加载成功
kubectl logs -n kepler ds/kepler-exporter | grep -i "bpf\|ebpf\|probe"

# 🟢 查看 RAPL 设备是否可用
kubectl exec -n kepler ds/kepler-exporter -- ls /sys/class/powercap/
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Pod CrashLoopBackOff | 内核不支持 eBPF | `kubectl logs ds/kepler-exporter -n kepler` | 升级内核至 4.18+ 或使用 ML 模型 |
| 指标全为 0 | RAPL 设备不可用 | `ls /sys/class/powercap/` | 启用 ML 估算模型 |
| 缺少 Pod 维度指标 | cgroup 版本不匹配 | `cat /proc/filesystems \| grep cgroup` | 配置正确的 cgroup 驱动 |
| 高 CPU 使用率 | 采集间隔过短 | `kubectl top pod -n kepler` | 增大 requestInterval |
| GPU 指标缺失 | NVML 库未挂载 | `kubectl logs ds/kepler-exporter \| grep gpu` | 挂载 /usr/lib/x86_64-linux-gnu/libnvidia-ml.so |

### 排查流程

```
Kepler 指标异常
├─ Pod 未运行？
│  ├─ 是 → 检查节点内核版本 (uname -r >= 4.18)
│  │       检查 privileged 权限是否授予
│  └─ 否 → 继续
├─ 指标端点无响应？
│  ├─ 是 → 检查 Service/端口 9102 是否监听
│  └─ 否 → 继续
├─ 指标值为 0？
│  ├─ RAPL 不可用 → 启用 Power Model (ML 估算)
│  └─ cgroup 映射失败 → 检查 cgroup v1/v2 配置
└─ Pod 维度缺失？
   ├─ K8s API 连接失败 → 检查 ServiceAccount/RBAC
   └─ cgroup ID 不匹配 → 确认容器运行时 cgroup 驱动一致
```

## 生产案例

### 案例 1: 云环境 RAPL 不可用导致能耗数据为零

**场景**: 某企业在 AWS EC2 上部署 Kepler，发现所有能耗指标为 0。

**排查**: 云虚拟机不暴露 RAPL 接口（/sys/class/powercap 为空），Kepler 默认依赖硬件计数器。

**方案**: 启用 Kepler Model Server，使用预训练 ML 模型基于 CPU 利用率、内存带宽等软件计数器估算能耗：
```bash
helm upgrade kepler kepler/kepler -n kepler \
  --set powerModel.enabled=true \
  --set powerModel.modelServerEndpoint=http://kepler-model-server:8100
```

**效果**: 估算误差在 ±15% 以内，满足碳足迹报告精度要求。

### 案例 2: 大规模集群 Kepler 性能优化

**场景**: 500 节点集群部署 Kepler 后，Prometheus 抓取超时。

**排查**: 每节点导出 2000+ Pod 指标，总指标量超 100 万条，单次 scrape 超过 30s。

**方案**:
1. 增大 Prometheus scrape_timeout 至 60s
2. 配置 Kepler 仅导出 Top-N 高能耗 Pod
3. 使用 Prometheus relabeling 丢弃低价值指标

**效果**: 抓取时间降至 8s，存储量减少 70%。

## 对比与替代方案

| 维度 | Kepler | Scaphandre | node_exporter+RAPL | 云厂商碳工具 |
|------|--------|------------|--------------------|--------------|
| 粒度 | Pod/Container | 进程级 | 节点级 | 账户/区域 |
| K8s 原生 | ✅ DaemonSet | ❌ 需适配 | ❌ 无 Pod 映射 | 部分 |
| 采集方式 | eBPF+RAPL | /proc | sysfs | API |
| ML 估算 | ✅ 内置 | ❌ | ❌ | N/A |
| GPU 支持 | ✅ NVML | 有限 | ❌ | 部分 |
| CNCF 状态 | Sandbox | 非 CNCF | CNCF Graduated | N/A |

## 检查清单

- [ ] 节点内核版本 >= 4.18（eBPF 支持）
- [ ] Kepler DaemonSet 以 privileged 模式运行
- [ ] /sys/class/powercap 可访问（物理机）或 ML 模型已启用（虚拟机）
- [ ] ServiceMonitor 已创建，Prometheus 可抓取 9102 端口
- [ ] Grafana Dashboard 18122 已导入
- [ ] GPU 节点已启用 NVML 采集
- [ ] 碳足迹计算公式已配置（区域电网碳排放因子）
- [ ] 告警规则：节点能耗异常突增 > 200%

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Kepler** | K8s 原生、eBPF、Pod 级别 | 需内核 RAPL 支持 |
| Scaphandre | 多平台支持 | 非 K8s 原生 |
| node_exporter + RAPL | 简单 | 仅节点级、无 Pod 粒度 |
| 云厂商碳工具 | 云原生集成 | 厂商绑定 |

## 架构定位

在 CNCF 生态中，Kepler 属于 **Sustainability / Observability** 类别，是 CNCF TAG Environmental Sustainability 的旗舰项目。它将能耗可观测性引入 Kubernetes。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[概念/observability-pillars.md|observability-pillars]]
- networking.md|cilium-ebpf-networking]]
- [[pod-lifecycle]]

## Related

- [[openebs]] — OpenEBS
- [[05-containerd-windows-support]] — [[containerd|containerd]]rd Windows 支持|containerd Windows 支持]]
- [[cortex]] — Cortex
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kepler
- [[实体/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
