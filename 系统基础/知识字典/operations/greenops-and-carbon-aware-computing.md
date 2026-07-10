---
title: GreenOps 与碳感知计算
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- scheduler
- prometheus
- gpu
- ebpf
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- GreenOps 与碳感知计算 是什么
- 如何 GreenOps 与碳感知计算
trigger_keywords:
- GreenOps
- 与碳感知计算
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
- ebpf-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# GreenOps 与碳感知计算

## 概述

随着全球对气候变化的重视和企业 ESG（环境、社会与治理）合规要求的提升，**GreenOps** 正在成为云原生运维的重要分支。GreenOps 将环境可持续性纳入 IT 运营决策，通过**碳感知调度（Carbon-aware Scheduling）、资源效率优化和可再生能源优先**等手段，降低 [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 工作负载的碳足迹。2026 年，欧盟 CSRD 等法规已要求大型企业披露数字基础设施的碳排放数据。

## 核心概念/原理

### 1. IT 碳排放来源

云计算和数据中心是显著的能源消耗者，Kubernetes 集群的碳排放主要来自：
- **电力消耗**：服务器、存储、网络设备和冷却系统的用电
- **碳强度（Carbon Intensity）**：电网每度电的 CO₂ 排放量，因地区和时段差异巨大
- **资源浪费**：过度配置、空闲运行和低效调度导致的无效能耗
- **硬件生命周期**：服务器制造、运输和报废处理产生的隐含碳排放

### 2. 碳感知调度（Carbon-aware Scheduling）

**碳感知调度**是指根据电网的实时碳强度数据，将工作负载调度到碳排放更低的时间段或地理区域：
- **时间偏移（Temporal Shifting）**：将非紧急的批处理任务推迟到夜间（风电充足时）执行
- **地理偏移（Spatial Shifting）**：将工作负载从煤电为主的区域迁移到水电/光伏为主的区域
- **Kubernetes 集成**：通过自定义调度器或扩展（如 [[Kepler|Kepler]] + Carbon-aware plugins）实现自动碳感知决策

### 3. Kepler：Kubernetes 能耗监控

**Kepler（Kubernetes Efficient Power Level Exporter）** 是 CNCF 沙箱项目，利用 eBPF 和 RAPL（Running Average Power Limit）接口：
- 按 Pod 级别采集能耗数据（瓦特）
- 将能耗数据导出为 [[Prometheus|Prometheus]] 指标
- 支持计算每个 Pod 的碳排放量估算

```
kepler_container_joules_total{container="my-app", namespace="production"}
```

### 4. Green Software Foundation（GSF）

GSF 提出了绿色软件开发的八大原则：
1. 碳效率（Carbon Efficiency）
2. 能源效率（Energy Efficiency）
3. 碳感知（Carbon Awareness）
4. 硬件效率（Hardware Efficiency）
5. 测量（Measurement）
6. 气候承诺（Climate Commitments）
7. 加速去碳化（Fossil-free Energy）
8. 开源协作（Open-source Collaboration）

## 关键机制或特性

### 碳感知 Kubernetes 调度器

通过扩展 [[系统基础/知识字典/scheduling/kubernetes-scheduler.md|Kubernetes Scheduler]] 或自定义控制器实现：
- 读取电网碳强度 API（如 Electricity Maps、WattTime）
- 在节点标签中标记当前区域的碳强度分数
- 调度器优先将可延迟批处理任务分配到碳强度低的节点/时段
- 对于紧急交互式任务，保持默认调度以保证 SLA

### 资源效率与能耗的关系

Kubernetes 资源优化直接带来能耗降低：
- **Right-sizing**：减少过度配置意味着减少活跃服务器数量
- **自动休眠**：非生产环境夜间关闭可减少 30%–60% 的运行时能耗
- **Spot 实例**：复用云厂商的闲置容量，避免新建物理服务器
- **ARM 架构**：AWS Graviton、Azure Cobalt 等 ARM 处理器在相同性能下能效比 x86 高 40%+

### 硬件层面的能效优化

- **液冷（Liquid Cooling）**：高密度 GPU 集群采用液冷可比风冷降低 40% 的冷却能耗
- **电源使用效率（PUE）**：现代数据中心的 PUE 目标为 1.1–1.2，老旧数据中心可能高达 1.6+
- **服务器延寿**：延长服务器使用寿命可减少隐含碳排放，但需平衡性能和安全补丁支持

## 使用场景

1. **夜间批处理调度**：将机器学习训练、数据仓库 ETL 等批处理任务调度到凌晨 2–6 点（风电/核电高峰）
2. **跨区域负载转移**：在欧盟和美国西部之间动态切换可延迟工作负载，优先使用可再生能源丰富的区域
3. **数据中心选址**：新建集群时优先选择 PUE 低、可再生能源比例高的数据中心
4. **ESG 合规报告**：通过 Kepler 和云厂商碳报告工具生成每个季度 Kubernetes 工作负载的碳排放数据
5. **ARM 迁移**：将无状态的 Web 服务迁移到 ARM 节点池，降低单位请求的能耗

## 最佳实践/注意事项

- **区分任务优先级**：只有容错、非实时的批处理任务才应进行碳感知调度，避免影响用户体验
- **结合 FinOps 实践**：降低能耗通常同时降低成本，GreenOps 和 FinOps 目标高度一致
- **使用云厂商碳足迹工具**：AWS Customer Carbon Footprint Tool、Azure Sustainability Calculator、Google Carbon Sense Suite
- **监控真实能耗而非利用率**：CPU 利用率 10% 和 50% 的能耗差异可能只有 20%，真正重要的是关闭不必要的节点
- **优先减少资源浪费**：最有效的减碳措施是消除空闲资源和过度配置，其次才是硬件和区域优化
- **透明的碳指标看板**：为每个团队提供其工作负载的碳排放仪表盘，培养绿色文化
- **考虑隐含碳排放**：硬件采购决策应考虑设备制造过程中的碳排放，优先选择碳中和认证的云服务
- **定期评估供应商可持续性**：审查云服务商和硬件供应商的可再生能源使用比例和碳中和承诺

## 故障排查

| 症状 | 可能原因 | 排查命令 | 解决方案 |
|------|----------|----------|----------|
| Kepler 指标为零 | RAPL 接口不可用或 eBPF 未加载 | `kubectl logs -n kepler kepler-*` | 确认节点支持 RAPL，内核启用 eBPF |
| 碳感知调度器未生效 | 碳强度 API 不可达 | 检查 Electricity Maps / WattTime API 连通性 | 配置 API key 和网络出口策略 |
| 能耗数据不准确 | 虚拟机环境无法直接读取 RAPL | `cat /sys/class/powercap/intel-rapl:0/energy_uj` | 使用 Kepler 的估算模型替代直接读取 |
| 批处理任务未在低碳时段运行 | 调度器插件优先级配置错误 | 查看自定义调度器配置 | 确认碳感知插件的 [[Score|Score]] 权重设置 |
| kube-green 环境未按时关闭 | SleepInfo CR 时区不匹配 | `kubectl get sleepinfo -A -o yaml` | 设置正确的 timezone 字段 |

## 生产检查清单

- [ ] Kepler 已部署并导出 Pod 级能耗 Prometheus 指标
- [ ] 碳强度 API（Electricity Maps / WattTime）已集成
- [ ] 非紧急批处理任务配置了碳感知调度策略
- [ ] 非生产环境配置了自动休眠（kube-green）
- [ ] ARM 节点池已创建用于能效优化的无状态工作负载
- [ ] 能耗和碳排放仪表盘对各团队可见
- [ ] 季度碳排放报告流程已建立（ESG 合规）
- [ ] swap 加密已启用以降低数据安全风险

## 命令快速参考

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Kepler Pod 级能耗指标
kubectl port-forward -n kepler svc/kepler 9103:9103
curl -s localhost:9103/metrics | grep kepler_container_joules_total

# 查看节点能耗汇总（PromQL）
# sum(rate(kepler_container_joules_total[5m])) by (node)

# 查看 kube-green SleepInfo 状态
kubectl get sleepinfo -A

# 检查 ARM 节点
kubectl get nodes -l kubernetes.io/arch=arm64

# 查看云厂商碳足迹
# AWS: aws ce get-cost-and-usage --granularity MONTHLY --metrics "CO2e"
# GCP: bq query --use_legacy_sql=false 'SELECT * FROM carbon_footprint.carbon_footprint'
```
## 交叉引用

- [Kepler GitHub Repository](https://github.com/sustainable-computing-io/kepler)
- [Green Software Foundation](https://greensoftware.foundation/)
- [SCI Specification - Software Carbon Intensity](https://sci.greensoftware.foundation/)
- [Electricity Maps](https://app.electricitymaps.com/)
- [WattTime API](https://www.watttime.org/)
- 相关主题：[FinOps 与成本优化](finops-and-cost-optimization.md) · [Kubernetes Scheduler](../scheduling/kubernetes-scheduler.md) · [Karpenter Autoscaling](../scheduling/karpenter-autoscaling.md)

## 参考链接

- [Greenops And Carbon Aware Computing]()

## Related

- [[系统基础/知识字典/operations/argo.md|Argo]]
- [[系统基础/知识字典/operations/backup-disaster-recovery.md|备份与灾难恢复（Backup & Disaster Recovery）]]
- [[系统基础/知识字典/operations/capacity-planning-forecasting.md|13 - 容量规划与资源预测]]


<!-- risk-assessed -->
