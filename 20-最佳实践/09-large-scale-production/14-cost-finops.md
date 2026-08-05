---
title: 成本治理 FinOps
description: 大规模 Kubernetes 集群成本治理实践：成本泄漏点排查、right-sizing、Spot 与机型优化、Karpenter Consolidation、标签分摊与月度成本评审机制
summary: 覆盖 K8s 成本泄漏点、VPA right-sizing、Spot/Graviton/倚天策略、Karpenter 装箱、成本标签分摊体系与 FinOps 运营节奏
category: references
tags:
- k8s
- finops
- cost-optimization
- production
- best-practices
tier: supporting
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: intermediate
audience:
- SRE
- 运维负责人
- 平台工程师
estimated_read_time: 15min
---

# 成本治理 FinOps

> 大集群的成本失控通常不是"机器太贵"，而是**闲置节点组、拍脑袋的 requests、无归属的孤儿资源**。FinOps 的核心是把成本变成可归属、可预测、可优化的运营指标，与稳定性平衡（不为省钱击穿容量红线，见 [[11-autoscaling-capacity#5. 容量规划方法论]]）。

## 1. 成本泄漏点排查清单

| 泄漏点 | 排查方法 | 典型占比 |
|---|---|---|
| 闲置节点组（水位 < 20% 持续） | 节点组维度利用率报表 | 大 |
| requests 过度申请（P95 实际用量 << requests） | VPA Recommender 报告 | 最大头 |
| 可中断负载跑按需实例 | 负载分级审计 | 大 |
| 孤儿资源：未挂载 PVC、闲置 LB、快照残留、EIP | 定期扫描脚本 | 中 |
| 无节制 LoadBalancer Service | LB 数量审计 + 配额 | 中 |
| 日志/监控数据无限留存 | 存储账单分析 | 中 |
| 僵尸工作负载（无流量副本照跑） | 流量 × 副本交叉分析 | 小中 |
| 跨 AZ/跨地域流量费 | 流量拓扑分析（容易被忽视的隐形大头） | 中 |

## 2. Right-sizing（收益最高的动作）

标准流程：

1. VPA `Off` 模式运行 **≥ 2 周**收集建议值（覆盖业务周期）
2. 按"建议 P95 + 20% 缓冲"生成新 requests，优先处理**成本 Top 10 工作负载**
3. 经 review 后走 GitOps 变更灰度应用
4. 每季度回归——requests 会随业务变化再次漂移

配套动作：LimitRange 提供合理默认值；准入策略强制 requests 存在且不超过 Namespace 上限。

## 3. 实例与计费模式优化

### 3.1 负载分级 × 计费模式

| 负载类型 | 计费模式 |
|---|---|
| 核心在线服务（不可中断） | 按需 + Savings Plan/预留实例覆盖基线 |
| 弹性在线服务（容忍重启） | Spot/竞价实例为主 + 按需回退 |
| 批处理/CI/大数据 | 全 Spot |
| 有状态中间件 | 按需（本地盘型实例常更优） |

### 3.2 机型优化

- ARM 实例（Graviton / 倚天）：同性能价格低约 20%，前提是应用完成 ARM 适配验证
- 按负载画像选规格：内存敏感选 1:8 配比、GPU 训练 GPU:CPU 配比 1:8–1:12（阿里云 ACK 选型建议）
- 生产避免 ≤ 2C4G 小规格节点：守护进程固定开销占比高、网络资源受限、调度碎片多

### 3.3 Karpenter Consolidation

- 开启 consolidation 后 Karpenter 持续评估"能否用更少/更便宜的节点承载同样负载"并自动替换——是把 right-sizing 从"项目"变"常态"的关键机制
- 用 `disruption.budgets` 限制替换速率，配合 PDB 保护业务
- NodePool 声明多机型族多规格，提升 Spot 供给成功率

> 案例参考（社区实践，供估算量级）：某 EKS 集群从 Cluster Autoscaler 迁移 Karpenter 并引入 Spot 后，月度节点成本下降约 58%（22 节点按需 → 12 节点含 70% Spot）。实际收益取决于负载画像与 Spot 策略，勿直接套用。

## 4. 成本归属与分摊

1. **标签制度**：强制 `team` / `cost-center` / `env` 标签（准入策略卡控，见 [[12-security-hardening-baseline#3. 准入控制选型]]）
2. **计量工具**：OpenCost / Kubecost / 云厂商成本套件，按 Namespace/标签/工作负载分摊节点、存储、网络成本
3. **分摊原则**：以 requests 为主分摊口径（谁占坑谁付钱），辅以实际用量修正
4. **预算与告警**：按 cost-center 设预算，超 80% 预警

## 5. 运营节奏（FinOps 是流程不是工具）

| 频率 | 动作 |
|---|---|
| 每周 | 异常账单波动 review（环比 > 15% 必须归因） |
| 每月 | 成本评审会：分团队账单、Top 浪费项、优化任务认领 |
| 每季 | right-sizing 回归；Spot 覆盖率与机型配比复核；预留实例/Savings Plan 覆盖率调整 |
| 每半年 | 集群架构级成本审查：是否有该合并/下线的集群 |

**文化机制**：让团队看到自己名下的账单——能看到自己花费的团队 right-sizing 速度显著快于共享一张总账的团队。

## 6. 成本与稳定性的平衡红线

- 容量水位红线不破：allocatable 使用率 > 80% 时不做缩容类优化
- 核心在线服务不强制 Spot；Spot 池必须有按需回退
- 大促/护网期间冻结成本优化操作
- 任何成本优化变更走正常变更流程，灰度 + 观察 + 可回滚

## Related

- [[11-autoscaling-capacity|弹性伸缩与容量规划深化]]
- [[03-workload|工作负载最佳实践（资源管理）]]
- [[10-multi-cluster|多集群与联邦管理（集群合并）]]
- [[20-最佳实践/07-scenarios/cost-optimization|成本优化场景]]
