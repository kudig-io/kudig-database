---
title: GreenOps 可持续计算与碳足迹优化指南
description: 'title: GreenOps 可持续计算与碳足迹优化指南'
summary: 'title: GreenOps 可持续计算与碳足迹优化指南'
category: general
tags:
- k8s
- production
- best-practice
- guide
- daily-ops
- scheduler
- prometheus
- grafana
- helm
- vpa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- greenops-sustainable-computing-guide是什么？
- greenops-sustainable-computing-guide的使用方法
- greenops-sustainable-computing-guide的最佳实践
trigger_keywords:
- GreenOps
- 可持续计算与碳足迹优化指南
- production
- operations
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: GreenOps 可持续计算与碳足迹优化指南
description: '# GreenOps 可持续计算与碳足迹优化指南'
category: production-operations
tags:
- k8s
- production
- operations
- best-practices
- scheduler
- [[Prometheus|prometheus]]
- grafana
- [[Helm|helm]]
- vpa
- job
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- GreenOps 可持续计算与碳足迹优化指南 是什么
- 如何 GreenOps 可持续计算与碳足迹优化指南
- [[Kubernetes|Kubernetes]] 18 [[实体/k8s-production-operations.md|production operations]] 最佳实践
trigger_keywords:
- GreenOps
- 可持续计算与碳足迹优化指南
- production
- operations
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
# GreenOps 可持续计算与碳足迹优化指南

> **适用版本**: Kepler v0.7.12 / Kube-green v0.6 / Scaphandre v1.0  
> **最后更新**: 2026-04-24  
> **难度**: 中级

---

## 📋 目录

- [一、GreenOps 核心概念](#一greenops-核心概念)
- [二、Kepler 功耗监控部署](#二kepler-功耗监控部署)
- [三、碳足迹计算模型](#三碳足迹计算模型)
- [四、Kube-green 定时关机](#四kube-green-定时关机)
- [五、工作负载能效优化](#五工作负载能效优化)
- [六、绿色调度与位置感知](#六绿色调度与位置感知)
- [七、与 FinOps 的协同](#七与-finops-的协同)
- [八、报告与合规](#八报告与合规)

---

## 一、GreenOps 核心概念

```
GreenOps = FinOps + 可持续性
├── 监控 (Measure)
│   ├── 节点级功耗 (Kepler)
│   ├── 容器级碳排放估算
│   └── PUE (能源使用效率)
│
├── 优化 (Optimize)
│   ├── 定时关机非生产环境 (Kube-green)
│   ├── 绿色调度 (碳感知调度)
│   ├── 资源 rightsizing
│   └── 可再生能源采购
│
└── 报告 (Report)
    ├── 碳足迹报告
    ├── SBTi (科学碳目标倡议)
    └── ESG 合规
```

### 数据中心碳排放构成

| 来源 | 占比 | 优化手段 |
|:---|:---|:---|
| IT 设备功耗 | 50-60% | 资源优化、高效硬件 |
| 冷却系统 | 30-40% | PUE 优化、液冷 |
| 供电损耗 | 5-10% | 高效 PSU、可再生能源 |
| 其他 | 5% | 基础设施优化 |

---

## 二、Kepler 功耗监控部署

### 2.1 安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm repo add kepler https://sustainable-computing-io.github.io/kepler-helm-chart
helm repo update

helm install kepler kepler/kepler \
  --namespace kepler \
  --create-namespace \
  --set serviceMonitor.enabled=true \
  --set serviceMonitor.namespace=monitoring
```
### 2.2 工作原理

```
Kepler 架构
├── eBPF 采集器
│   ├── CPU 周期计数
│   ├── 内存访问频率
│   ├── 磁盘 I/O
│   └── 网络流量
│
├── RAPL (Running Average Power Limit)
│   └── Intel/AMD CPU 功耗寄存器
│
├── ML 模型 (可选)
│   └── 无 RAPL 时的功耗估算
│
└── Prometheus Exporter
    └── 容器级功耗指标
```

### 2.3 关键指标

| 指标 | PromQL | 说明 |
|:---|:---|:---|
| 容器功耗 | `kepler_container_joules_total` | 累计焦耳 |
| Pod 功耗 | `kepler_pod_name_joules_total` | 按 Pod 聚合 |
| 节点功耗 | `kepler_node_package_joules_total` | 节点级 RAPL |
| 动态功耗 | `kepler_container_dyn_joules_total` | CPU/内存动态 |
| 静态功耗 | `kepler_container_idle_joules_total` | 基础功耗 |

### 2.4 Grafana Dashboard

```yaml
apiVersion: grafana.integreatly.org/v1beta1
kind: GrafanaDashboard
metadata:
  name: kepler-dashboard
spec:
  url: https://raw.githubusercontent.com/sustainable-computing-io/kepler/main/grafana-dashboards/Kepler-Exporter.json
```

---

## 三、碳足迹计算模型

### 3.1 碳强度 (Grid Carbon Intensity)

```
碳排放 (gCO2) = 功耗 (kWh) × 电网碳强度 (gCO2/kWh)

各地区碳强度示例 (2025):
├── 中国平均: 550-650 gCO2/kWh
├── 美国平均: 350-450 gCO2/kWh
├── 欧盟平均: 200-300 gCO2/kWh
├── 挪威: 20-30 gCO2/kWh (水电为主)
└── 冰岛: 0 gCO2/kWh (地热+水电)
```

### 3.2 容器碳足迹估算

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: carbon-config
  namespace: monitoring
data:
  carbon-intensity: |
    {
      "default": 550,
      "regions": {
        "cn-north-1": 650,
        "us-east-1": 400,
        "eu-west-1": 250,
        "ap-northeast-1": 500
      }
    }
```

```promql
# 容器碳足迹 (gCO2)
(
  sum by (namespace, pod) (
    rate(kepler_container_joules_total[1h])
  ) / 3600000
) * 550  # 碳强度 gCO2/kWh
```

---

## 四、Kube-green 定时关机

### 4.1 安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
helm repo add kube-green https://kube-green.github.io/charts
helm install kube-green kube-green/kube-green \
  --namespace kube-green \
  --create-namespace
```
### 4.2 SleepInfo 配置

```yaml
apiVersion: kube-green.com/v1alpha1
kind: SleepInfo
metadata:
  name: dev-sleep
  namespace: development
spec:
  # 工作日 18:00 关机
  sleepAt: "0 18 * * 1-5"
  # 工作日 8:00 开机
  wakeUpAt: "0 8 * * 1-5"
  # 时区
  timezone: Asia/Shanghai
  # 排除周末
  excludeRef:
    - apiVersion: apps/v1
      kind: Deployment
      name: critical-service
  # 自定义资源类型
  suspendCronJobs: true
```

### 4.3 节能效果估算

```
开发环境 (假设)
├── 10 个 Namespace
├── 每个 5 个 Deployment
├── 平均 3 个副本
├── 每个 Pod 500m CPU + 512Mi 内存
├── 每天运行 10 小时 (8-18)
└── 每天节省: 14 小时 × 50 Pod × ~50W = 35 kWh/天
    └── 年节省: ~12,775 kWh ≈ 7 吨 CO2
```

---

## 五、工作负载能效优化

### 5.1 资源 Rightsizing

| 优化项 | 工具 | 效果 |
|:---|:---|:---|
| CPU/Memory 调优 | VPA / Goldilocks | 减少过度配置 20-40% |
| Spot 实例 | Karpenter | 降低成本同时提高利用率 |
| ARM/Graviton | 多架构支持 | 能耗降低 20-40% |
| 批处理调度 | Volcano / Yunikorn | 提高集群利用率 |

### 5.2 绿色调度策略

```yaml
apiVersion: scheduler.framework.k8s.io/v1
kind: GreenScheduler
# 碳感知调度 (实验性)
spec:
  plugins:
    score:
      enabled:
      - name: CarbonAware
  pluginConfig:
  - name: CarbonAware
    args:
      carbonIntensityEndpoint: "https://api.electricitymap.org/v3/carbon-intensity"
      preferLowCarbon: true
```

### 5.3 能效标签

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    sustainability/efficiency-target: "A"  # A-F 等级
    sustainability/carbon-budget: "100kg-co2-month"
```

---

## 六、绿色调度与位置感知

### 6.1 碳感知工作负载迁移

```
碳强度实时变化
    │
    ├── 高碳时段 (白天用电高峰)
    │   └── 将非紧急批处理延后
    │
    └── 低碳时段 (夜间风电充足)
        └── 运行训练任务、备份
```

### 6.2 多云碳优化

| 云厂商 | 碳透明度 | 优化功能 |
|:---|:---|:---|
| AWS | Customer Carbon Footprint Tool | Graviton3、Spot |
| GCP | Carbon Footprint | 低碳区域优先 |
| Azure | Emissions Impact Dashboard | ARM VM、Spot |
| 阿里云 | 能耗宝 | 绿色数据中心 |

---

## 七、与 FinOps 的协同

```
GreenOps + FinOps 协同效应
├── 成本优化 → 资源减少 → 能耗降低 → 碳排放降低
├── Spot 实例 → 成本降低 90% → 提高利用率 → 减少新建节点
├── 自动关机 → 成本降低 60-70% → 直接减少碳排放
└── Rightsizing → 消除浪费 → 双 wins
```

### 联合指标

```promql
# 每美元碳排放效率
(
  sum(kepler_container_joules_total) / 3600000 * 550
) / (
  sum(kubecost_container_cost)
)

# 每请求碳排放
(
  sum by (service) (rate(kepler_container_joules_total[5m]))
) / (
  sum by (service) (rate(http_requests_total[5m]))
)
```

---

## 八、报告与合规

### 8.1 自动生成碳报告

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 Kepler + kubectl 导出月度报告
kubectl top pod --all-namespaces > pod-usage.txt
# 结合 Kepler 指标计算碳排放
```
### 8.2 关键合规框架

| 框架 | 要求 | K8s 相关 |
|:---|:---|:---|
| GHG Protocol | 范围 1/2/3 排放核算 | 数据中心范围 2 |
| SBTi | 科学碳目标 | 2030 减排目标 |
| EU CSRD | 企业可持续发展报告 | IT 基础设施披露 |
| ISO 14064 | 温室气体核查 | 组织层级 |

### 8.3 Prometheus 碳预算告警

```yaml
- alert: NamespaceCarbonBudgetExceeded
  expr: |
    (
      sum by (namespace) (rate(kepler_container_joules_total[1d]))
      / 3600000 * 550
    ) > 100000  # 100 kg CO2/天
  for: 1h
  labels:
    severity: warning
  annotations:
    summary: "命名空间 {{ $labels.namespace }} 碳排放超出预算"

- alert: ClusterHighPUE
  expr: |
    (
      sum(kepler_node_package_joules_total)
      / sum(node_energy_stat)
    ) > 1.5
  for: 5m
  labels:
    severity: info
  annotations:
    summary: "集群 PUE 超过 1.5，冷却效率需优化"
```

---

## 参考链接

- [Kepler 文档](https://sustainable-computing.io/)
- [Kube-green 文档](https://kube-green.dev/)
- [Green Software Foundation](https://greensoftware.foundation/)
- [SCI (Software Carbon Intensity)](https://sci.greensoftware.foundation/)
- [Electricity Maps API](https://www.electricitymaps.com/)
- [Climatiq API](https://www.climatiq.io/)

---

## Obsidian 相关文档

- 生产运维 MOC
- [[生产运维/README.md|Domain 11: 生产环境运维最佳实践 (Production Operations Best Practices)]]
- Domain-18 生产运维 — 开源项目索引
- [[集群基础/01-production-architecture-design-principles.md|01-生产架构设计原则]]
- 02-多云混合部署策略
- 03-边缘计算生产部署
- 04-企业级监控体系
- 05-日志收集分析平台
- 06-APM应用性能监控
- 07-零信任安全架构
- 08-CIS基准合规检查
- 09-软件物料清单

## Related

- 19-cloudnative-devops-architecture

## See Also

- 24-capacity-planning-forecasting
- 99-finops-cost-optimization-guide
- 99-karpenter-node-autoscaling-guide
- 99-keda-event-driven-autoscaling-guide

```

<!-- risk-assessed -->
