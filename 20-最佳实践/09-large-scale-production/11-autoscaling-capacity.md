---
title: 弹性伸缩与容量规划深化
description: 大规模 Kubernetes 集群弹性伸缩深化实践：HPA 指标选择、VPA 模式、Karpenter 与 Cluster Autoscaler 对比选型、云资源配额治理、分批扩容与容量水位管理
summary: 覆盖 HPA 业务指标选型、VPA 推荐模式、Karpenter vs Cluster Autoscaler 决策矩阵、云配额预申请、扩容速率治理与容量规划方法论
category: references
tags:
- k8s
- autoscaling
- karpenter
- capacity-planning
- production
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
---

# 弹性伸缩与容量规划深化

> [[03-workload#2. 弹性伸缩]] 给出工作负载侧规范，本文深化**节点级弹性**与**容量规划方法论**，并补充大规模场景下容易踩坑的云配额与扩容速率治理。

## 1. HPA 指标选择深化

CPU 利用率是最差的弹性指标之一——它不反映真实请求饱和度。指标优先级：

| 指标类型 | 示例 | 适用 |
|---|---|---|
| 队列深度 | MQ lag、任务队列长度 | 消费者类负载，最优信号 |
| 时延/饱和度 | p95 latency、活跃连接数 | Web/API 服务 |
| 吞吐量 | QPS / 单副本承载 QPS | 无状态服务 |
| CPU/内存 | 兜底指标 | 无业务指标时 |

- 使用 `autoscaling/v2` API；业务指标经 Prometheus Adapter / KEDA 暴露
- **KEDA** 适合事件驱动负载（MQ 消费、定时批处理），支持缩到 0
- 大促前必须压测验证"HPA 扩容速度 ≥ 流量爬升速度"，否则提前手动扩容

## 2. VPA 使用模式

| 模式 | 行为 | 生产建议 |
|---|---|---|
| Off（Recommender） | 只出建议 | **默认起步**：先跑 2 周收集 P95 真实用量 |
| Initial | 仅创建时注入建议值 | 适合无状态服务的半自动化 |
| Auto/Recreate | 自动驱逐重建调整 | 有状态服务禁用（重建有风险）；无状态服务谨慎开启 |

- VPA 建议值应经过 review 再应用，纳入 GitOps 变更流程
- 官方建议：集群关键 Addon 也可用 VPA Recommender 获取 requests/limits 建议——大集群中 Addon 默认值偏小是常见的 OOM/限流根因

## 3. 节点级弹性：Karpenter vs Cluster Autoscaler

| 维度 | Karpenter | Cluster Autoscaler |
|---|---|---|
| 供给速度 | 约 30–60 秒（直调云 API） | 约 3–5 分钟（经 ASG/伸缩组） |
| 机型选择 | 按约束动态选择任意机型 | 节点组预定义机型 |
| 成本优化 | Consolidation 持续装箱，主动替换低效节点 | 缩容慢，无主动装箱 |
| Spot 处理 | 原生中断处理 + 自动回退按需 | 需额外配置 |
| 节点规模上限 | 不受节点组配额（如每 ASG 450 节点）限制 | 受节点组配额约束 |
| 运维模型 | NodePool/NodeClass CRD 声明式 | 节点组 + ASG 双层 |

**选型建议：**

- 新建大规模集群默认 Karpenter（2023 起已成 AWS 等场景主流，已捐赠 CNCF）
- 需要严格节点组边界（合规/审计）或平台不支持时保留 CA
- CA 大规模调优：每 AZ 一个节点组（供给信号准确）、`--scale-down-unneeded-time` 适度调长防抖动、开启 `balance-similar-node-groups`

### Karpenter 生产要点

- NodePool 中声明多机型族 + 多规格，降低 Spot 容量枯竭风险
- `spec.disruption.budgets` 限制同时被替换的节点比例，保护 PDB 约束下的业务
- 关键业务 NodePool 配置按需实例回退，Spot 中断不扩散
- Karpenter 自身 ≥ 2 副本 + 监控告警（供给失败率、供给时延）

## 4. 云配额与扩容速率治理（官方大规模注意事项）

大集群扩容失败的第一大原因不是 K8s，而是**云配额与 API 限流**：

| 配额项 | 动作 |
|---|---|
| 云主机实例数 / vCPU 总量 | 按峰值 ×1.5 提前申请提额 |
| 弹性 IP / 网卡 / 内网 IP 数 | VPC CNI 场景重点核对 |
| 负载均衡器数量 | 大量 LoadBalancer Service 场景 |
| 安全组规则数 / 路由表条目 | 网络策略复杂时 |
| 云盘数量 / 容量 | 有状态负载 |
| 云 API 调用速率（创建实例等） | **分批扩容**：扩容动作分批、批间暂停，避免触发云 API 限流 |

> 依据：Kubernetes 官方大规模集群文档明确建议——提前申请云资源配额提升，并将集群扩容分批进行、批间留出间隔，因为云厂商对实例创建有速率限制。

## 5. 容量规划方法论

### 5.1 水位管理

| 水位（allocatable 使用率） | 动作 |
|---|---|
| < 60% | 健康 |
| 60–70% | 关注，评估是否扩容 |
| 70–80% | 扩容窗口，评估成本与风险 |
| > 80% | 危险：headroom 不足以承受单节点/AZ 故障转移，立即扩容 |

Headroom 计算必须考虑**故障转移冗余**：N 个节点的集群至少要能承受 1 个节点（多 AZ 集群则要能承受 1 个 AZ）损失后业务仍有地方调度。

### 5.2 容量预测流程

1. 业务侧收集：峰值 QPS 预测 → 单副本承载能力（压测值）→ 所需副本数
2. 平台侧换算：副本数 × requests → 所需 allocatable → 所需节点数（按装箱率 70–80% 折算）
3. 提前扩容：大促/活动前 T-3 天完成扩容并验证，T-1 天冻结变更
4. 事后回归：实际峰值 vs 预测偏差 > 20% 时复盘校准模型

### 5.3 装箱率优化

- requests 精准度是装箱率的前提（见 VPA 节）
- 大小 Pod 混布提升碎片利用；超规格 Pod（单容器 > 节点 1/4 资源）会造成严重碎片
- 生产不建议使用 ≤ 2C4G 的小规格节点（网络/ENI 受限、守护进程占比高、碎片多）——阿里云 ACK 官方亦明确不推荐

## 6. 常见反模式

| 反模式 | 后果 |
|---|---|
| 只用 CPU 指标做 HPA | 扩容信号失真，高峰期扩容不及或抖动 |
| CA 单节点组跨多 AZ | 某 AZ 缺容量时扩容信号失真 |
| 扩容不申请云配额 | 大促时扩不出节点，眼睁睁看着雪崩 |
| VPA Auto 作用于数据库 | 自动重建有状态 Pod 引发事故 |
| 水位长期 > 85% 硬撑 | 任何节点故障都演化为容量事故 |

## Related

- [[03-workload|工作负载最佳实践（弹性伸缩）]]
- [[14-cost-finops|成本治理 FinOps（Consolidation/Spot）]]
- [[07-pre-production-checklist|生产上线前检查项（容量压测）]]
- [[20-最佳实践/07-scenarios/capacity-planning|容量规划场景]]
