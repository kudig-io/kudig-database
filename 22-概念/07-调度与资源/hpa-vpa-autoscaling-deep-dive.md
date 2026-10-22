---
title: HPA/VPA 弹性伸缩深挖
summary: 面向面试与生产落地的 HPA/VPA 深度笔记——扩缩公式与 requests 分母、防抖三参数、CFS 限流互骗、VPA 三件套与 updateMode 全谱、HPA×VPA 分维度共存、KEDA 缩零、四层弹性金字塔与选型决策树。
category: concepts
tags:
- k8s
- autoscaling
- hpa
- vpa
- keda
- resource-management
- interview
tier: core
created: '2026-10-20'
last_updated: 2026-10
status: reviewed
---

# HPA/VPA 弹性伸缩深挖

> 定位：基础概念看 [[horizontal-pod-autoscaler]] 与知识字典 [[vertical-pod-autoscaling]]；本文不重复基础，专收"面试追问到第二层"和生产落地的坑。快问快答版见 `36-报告/k8s-高级面试快问快答-沉淀.md` Q94–Q103。

## 一、四层弹性金字塔：先说清"谁管哪一层"

所有弹性问题先分层，再谈参数——这是"你们集群怎么做弹性"的满分开场：

```
            ┌──────────────────────────┐
  请求驱动层 │  Knative/KPA、Serverless │  ← 按请求数/缩到 0，平台层魔法
            ├──────────────────────────┤
  节点层     │  Cluster Autoscaler /    │  ← Pod 们 Pending 才出手，买机器
            │  Karpenter（Q41）        │
            ├──────────────────────────┤
  规格层     │  VPA（requests/limits）  │  ← 单个 Pod 多大合适
            ├──────────────────────────┤
  副本层     │  HPA / KEDA              │  ← 多开几个 Pod
            └──────────────────────────┘
```

判断线：**HPA 管"几个"、VPA 管"多大"、CA/Karpenter 管"多少台"、KEDA/Knative 管"要不要"**。面试只答 HPA 不分层，等于只认识金字塔第一层。

## 二、HPA 深挖

### 2.1 扩缩公式与 Utilization 的分母

```
desiredReplicas = ceil[ currentReplicas × (currentMetricValue / targetMetricValue) ]
```

- 例：4 副本、CPU 80%、目标 50% → `ceil(4 × 1.6) = 7`。
- **多指标取各指标算出的最大值**（都满足才叫满足）；任一指标拉不到就跳过该指标。
- **Utilization（百分比）的分母是 requests，不是 limit**——`实际用量 / requests`。没设 requests 的 Pod 不参与计算，全员没设则该指标直接算不出、HPA 不动手。所以"HPA 不扩容"第一查 requests 有没有设、设得虚不虚。
- 不想被 requests 绑架就用 `AverageValue`（按绝对值平均）。

### 2.2 防抖三参数（面试必背数字）

| 参数 | 默认值 | 作用 |
|------|--------|------|
| `--horizontal-pod-autoscaler-sync-period` | **15s** | 调和周期，最快 15s 扩一次 |
| `--horizontal-pod-autoscaler-tolerance` | **0.1（10%）** | 偏离目标 <10% 直接不动，防抖第一闸 |
| downscale `stabilizationWindowSeconds` | **300s** | 缩容取 5 分钟内的最大期望值，慢性子 |

扩容稳定窗口默认 **0**（扩要快），缩容默认 **300s**（缩要稳）——这个不对称是刻意设计。`behavior` 字段（autoscaling/v2，1.18+）可以逐向配 `stabilizationWindowSeconds` + `policies`（按 Pods/Percent 限每 period 步长），比如"每分钟最多缩 10%"。

### 2.3 指标拉不到时的行为（安全兜底）

Metrics Server 挂了 / 指标缺失：HPA **跳过该指标不动手**，绝不在没指标时缩容——这是"宁可多花钱不可丢容量"的兜底。`kubectl describe hpa` 看 Conditions：`ScalingActive=False` + reason（`FailedGetResourceMetric` 等）就是指标链断了。

### 2.4 作用对象与生效机制

- HPA 通过 **scale subresource** 生效——凡是实现 scale 的对象都能管：Deployment / ReplicaSet / StatefulSet / ReplicationController。
- HPA 只改 `.spec.replicas`，Pod 的生杀仍归 Deployment/RS controller（Q58 三层结构）。
- **手动 scale 与 HPA 冲突时听 HPA 的**——它每个周期都会覆盖 replicas；GitOps 管理的负载必须对 `spec.replicas` 做 ignoreDifferences，否则 Argo/Flux 和 HPA 天天打架（Q66 同款漂移问题）。

### 2.5 CPU 做 HPA 指标的坑：与 CFS 限流互骗（接 Q47）

闭环陷阱：CPU limit 触发 **CFS 限流 → usage 被压平 → HPA"看到"的利用率偏低 → 不扩容 → 继续限流**。平均用量 50% "很健康"和 P99 毛刺完全可以同时成立。规避：

1. 延迟敏感/毛刺型服务改用**自定义指标**（QPS、P99、连接数——`type: Pods` + Prometheus Adapter）；
2. 或用 `AverageValue` + 提前设 threshold，绕开 utilization 的分母戏法；
3. 监控 `container_cpu_cfs_throttled_periods_total` 比例（>10% 就该动），别拿 HPA 的 Happy 报告当容量结论。

### 2.6 HPA 不扩容的排查链（一分钟版）

```
kubectl describe hpa → Conditions
├─ ScalingActive=False / FailedGetResourceMetric → metrics-server 挂了（kubectl top 验证）
├─ 利用率始终低于目标 → requests 设太大（4 核 request 跑 1 核 = 25%）
├─ maxReplicas 已到顶 / ResourceQuota 卡住 / 节点资源不足
└─ behavior 限速或缩容窗口内 → 看 Events 时间线
```

节点不够时 HPA 扩出的 Pod 会 Pending → 触发 CA/Karpenter（Q41）加节点——两层金字塔在这条链上咬合。

## 三、VPA 深挖

### 3.1 三件套架构

```
          ┌──────────────────────────────────────────────┐
历史+实时 │ Recommender ──→ recommendation (CRD status)  │  算账的
metrics   │      ↑ metrics-server / checkpoints          │
          ├──────────────────────────────────────────────┤
          │ Updater ──→ 按模式驱逐存量 Pod               │  动手的
          ├──────────────────────────────────────────────┤
          │ Admission Controller (mutating webhook)      │  塞值的
          │    只拦 Pod CREATE，注入推荐 requests         │
          └──────────────────────────────────────────────┘
```

- **webhook 只拦创建**——这是理解 `Recreate` 模式的钥匙：存量 Pod 改不了（Q63/Q64 的 immutable 语义），Updater 只能驱逐，靠重建触发 webhook 注入新值。
- 推荐值四件套：`target`（建议值）/ `lowerBound` / `upperBound`（边界）/ `uncappedTarget`（未套 minAllowed/maxAllowed 前的原始值，用来检查 cap 是否夹疼了推荐）。
- **OOM 加成**：Recommender 把 OOMKilled 事件计入历史，自动上调内存推荐——VPA 是"吃过亏的容量顾问"。
- 依赖 metrics-server；推荐值需要几分钟历史才出现，刚建的 Pod 没有。

### 3.2 updateMode 全谱（版本敏感，答前先报）

| updateMode | 行为 | 生产安全 |
|------------|------|---------|
| `Off` | 只出推荐不动手 | ✅ 起步姿势 |
| `Initial` | 只在 Pod 创建时注入，存量不管 | ✅ |
| `Recreate` | 推荐偏离超阈值就驱逐重建 | ⚠️ 有中断，配 PDB |
| `InPlaceOrRecreate` | 优先原地 resize（K8s 1.33+ InPlacePodVerticalScaling），不支持再驱逐 | ⚠️ 新模式，特性门默认关 |
| `Auto` | 历史等同 `Recreate`（新版弃用此别名） | ⚠️ |

`resourcePolicy` 是安全阀：`minAllowed`/`maxAllowed` 夹住推荐范围（防极端值），`controlledValues: RequestsOnly` 只动 requests（limit 不动，HPA 共存标配），`mode: Off` 把 sidecar 排除在 VPA 之外。

### 3.3 VPA 边界与红线

- **不适用于 DaemonSet**（每节点一份没法靠驱逐重建均衡）——替代品是 addon-resizer / Cluster Proportional Vertical Autoscaler。
- Updater 驱逐**尊重 PDB**——没配 PDB 的核心服务别开 Recreate。
- 驱逐是**破坏性动作**：Recreate 模式的 VPA 本质上是"按算账结果定期搞发布"，中断敏感服务用 `Initial` 或 `Off` + 人审。

## 四、HPA × VPA 共存：冲突机理与两个方案

**冲突机理**：HPA 的 Utilization 分母是 requests；VPA 恰恰在改 requests。VPA 一上调 requests → HPA 眼里的利用率瞬间"跌下来" → HPA 缩容 → 剩余 Pod 分担更多负载 → 利用率又超 → HPA 扩容……两个控制器互相甩锅，副本数震荡。

**方案一（推荐）：分维度**
- HPA 管**自定义指标**（QPS/P99，`type: Pods`），VPA `RequestsOnly` 或 `Initial` 管 requests——一个看业务量、一个看资源规格，互不踩脚。

**方案二：CPU 归 HPA、内存归 VPA**
- HPA 只配 CPU 指标；VPA `controlledResources: [memory]` 只管内存。CPU 随流量弹性、内存随泄漏/缓存微调，维度天然正交。

官方的多维联合扩缩（HPA×VPA 单对象决策）还在 alpha 探索期，生产主流就是"分维度共存"。答案里能画出冲突机理的那条反馈回路，比背两个方案值钱。

## 五、缩到 0 与事件驱动：KEDA

- KEDA = ScaledObject CRD + 60+ 触发器（Kafka lag、SQS 队列深度、Cron、云事件）+ **缩到 0** 能力。
- 本质：**KEDA 自己生成并托管一个 HPA**（0↔1 的启动由它管，1↔N 交给原生 HPA）——面试一句话："KEDA 不是替代 HPA，是在 HPA 前面补了 0 到 1 这一段"。
- 适用：事件驱动消费者、夜间无人流量服务、成本敏感的批处理。Knative 的 KPA 是另一路（按并发/请求数直接驱动，不需要指标适配器），一句话对比即可。

## 六、选型决策树

```
负载波动吗？
├─ 否 → 定时 HPAScaler（Cron）或静态副本，别过度工程
└─ 是 → 无状态可水平扩？
    ├─ 是 → 有现成业务指标（QPS/队列）？
    │   ├─ 有 → HPA + 自定义指标（毛刺型服务别用 CPU）
    │   └─ 无 → HPA + CPU（先确认 requests 合理、限流不严重）
    ├─ 否（单实例大应用/DB）→ VPA Off 观察推荐 → 手工 rightsizing
    │        （StatefulSet 慎用 Recreate）
    └─ 事件驱动/要缩到 0 → KEDA
    ↓
扩完 Pod 还 Pending？→ CA/Karpenter 补节点（Q41）
长期副本数总在 min/max 边缘？→ 回头改 HPA 目标或 VPA rightsizing
```

## 七、生产落地清单

- [ ] HPA 目标负载**全部设了合理 requests**（Utilization 的分母）
- [ ] 毛刺型服务上自定义指标；`container_cpu_cfs_throttled_periods_total` 进监控
- [ ] 缩容 policy 保守（默认 300s 窗口 + 每分钟 ≤10%），扩容放开
- [ ] GitOps 侧 ignoreDifferences `spec.replicas`
- [ ] VPA 先 `Off` 跑两周看推荐，再考虑 `Initial`/`RequestsOnly`；`Recreate` 必须配 PDB
- [ ] VPA/HPA 共存检查维度是否正交（见第四节）
- [ ] 压测验证整条链：流量上来 → HPA 扩容 → Pending → CA 补节点 → 恢复

## 追问预判

- **HPA 扩容多快？**——理想链路 15s 调和 + Pod 启动时间；端到端瓶颈通常在容器启动/预热，不在 HPA。追问"那预热慢怎么办"→ startupProbe（Q32）+ 提前扩容（KEDA cron / 流量预测）。
- **指标毛刺导致扩了又缩？**——扩容无稳定窗口、缩容有 300s；再不行 behavior 里给 scaleUp 也加窗口。
- **VPA 推荐值一直不变？**——看 recommender 是否活着、Pod 历史够不够（checkpoint 几小时起步）、是否被 minAllowed/maxAllowed 夹住（对比 uncappedTarget）。
- **为什么不用 VPA 直接解决所有问题？**——垂直弹性有上限（单机规格）、要重建 Pod、且解决不了"流量翻十倍"——水平是主力、垂直是精调。

## Related

- [[horizontal-pod-autoscaler]] — HPA 基础概念与排查
- [[autoscaling-strategies]] — 弹性策略总览（HPA/VPA/CA 对比）
- [[resource-management]] — requests/limits/QoS 资源模型
- [[01-集群基础/03-控制平面/13-kube-controller-manager-deep-dive.md|KCM 深挖]] — HPA controller 所在的 KCM
- 快问快答：`36-报告/k8s-高级面试快问快答-沉淀.md` Q94–Q103（含 Q47 CFS 限流、Q41 CA/Karpenter 交叉引用）
