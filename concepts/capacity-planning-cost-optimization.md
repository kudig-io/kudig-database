---
title: 容量规划与成本优化
category: concepts
tags:
  - capacity
  - cost
  - finops
  - karpenter
  - k8s
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

# 容量规划与成本优化

## 概述

容量规划与成本优化是 Kubernetes 运营的核心挑战。随着云原生生态的成熟，AI/ML 驱动的预测式容量规划与 FinOps 实践相结合，正在从被动式"超配-浪费"模式转向主动式"精准供给-持续优化"模式。本文系统梳理从预测、供给到成本归因的全链路方法论与工具生态。

---

## AI/ML 驱动的容量规划

### 时序预测模型

| 模型 | 适用场景 | 特点 |
|------|----------|------|
| Facebook Prophet | 季节性/趋势明显的业务指标 | 自动处理节假日效应，可解释性强 |
| LSTM / DeepAR | 高频、多维指标预测 | 捕获非线性依赖，适合复杂模式 |
| VictoriaMetrics Anomaly Detection | 实时异常检测与预测 | 内置 ML 引擎，与 Prometheus 兼容 |
| Kubecost Forecast | 成本趋势预测 | 基于历史使用量推算未来开支 |

**预测驱动扩容流程：**

```
指标采集 → 时序预测 → 容量缺口计算 → 扩容决策 → 自动执行
   ↓            ↓            ↓              ↓           ↓
Prometheus   Prophet      diff(预测,      KEDA ScaledObject  Karpenter
Thanos       LSTM         当前容量)       预测式 Cron        预测式 NodePool
```

### KEDA 预测式扩容

[[keda|KEDA]] 的 `Cron` 和 `External` scaler 支持基于预测的提前扩容：

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: predictive-scaler
spec:
  scaleTargetRef:
    name: api-server
  triggers:
    - type: external
      metadata:
        scalerAddress: predictive-scaler.default:9090  # 自定义预测 scaler
        lookAheadMinutes: "30"  # 提前 30 分钟预扩容
    - type: cron
      timezone: Asia/Shanghai
      start: "0 8 * * 1-5"
      end: "0 22 * * 1-5"
      desiredReplicas: 10
```

**关键模式：**
- **Cron + HPA 混合**：基线流量用 Cron 预设，峰值叠加 HPA 弹性
- **外部预测 Scaler**：将 ML 模型推理结果暴露为 KEDA 外部指标
- **预测式冷却**：设置更长的 `cooldownPeriod` 避免预测窗口内的抖动

### Karpenter 预测式节点供应

Karpenter 结合预测实现集群级资源预供应：

```yaml
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: predictive-pool
spec:
  template:
    spec:
      requirements:
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["on-demand"]  # 预测扩容用按需实例保底
        - key: node.kubernetes.io/instance-type
          operator: In
          values: ["m6i.xlarge", "m6i.2xlarge", "m5.xlarge"]
  disruption:
    consolidationPolicy: WhenEmptyOrUnderutilized
    consolidateAfter: 30m
  limits:
    cpu: "1000"
    memory: 2000Gi
```

**预测式供应策略：**
1. 通过外部控制器在预测高峰期前 patch NodePool `limits` 或添加预热 NodeClaim
2. 利用 Karpenter 的 `spec.template.metadata.annotations` 传递预测标签
3. 结合 `disruption.budgets` 控制节点替换速率，避免预测波动导致频繁增删

---

## 成本优化生态

### 工具矩阵

| 工具 | 类型 | 核心能力 | 最新动态 |
|------|------|----------|----------|
| Kubecost | 商业 | 实时成本分配、告警、优化建议 | 2026 被 Finout 整合，成为其 K8s 成本引擎 |
| OpenCost 2.0 | CNCF Sandbox | 开源成本核算标准实现 | 2026 v2.0 GA，支持多云统一视图、FOCUS 兼容 |
| Cast AI | 商业 | 自主优化（Autopilot）| 声称 50-65% 降本，自动选择最优实例类型/定价模型 |
| Finout | 商业 | 全栈 FinOps 平台 | 整合 Kubecost 后成为 K8s + 云成本统一平台 |
| kube-green | 开源 | 非工作时间自动休眠资源 | 适合 dev/staging 环境，节省 60-70% 非生产成本 |

### 成本优化层次

```
┌─────────────────────────────────────────────────┐
│  L3: 自主优化（Cast AI Autopilot）              │  ← 自动执行，人工审核
├─────────────────────────────────────────────────┤
│  L2: 建议驱动（Kubecost/Finout 推荐）          │  ← 人工确认后执行
├─────────────────────────────────────────────────┤
│  L1: 可见性（OpenCost 成本归因）                │  ← 知道钱花在哪
├─────────────────────────────────────────────────┤
│  L0: 基础标签（namespace/team/app 标签）        │  ← 成本分摊前提
└─────────────────────────────────────────────────┘
```

### OpenCost 2.0 核心改进

- **多云统一视图**：原生支持 AWS/Azure/GCP 账单数据摄入
- **FOCUS 规范兼容**：输出符合 FinOps Open Cost and Usage Specification
- **自定义定价**：支持 negotiated rates、reserved instances 折扣计算
- **CNCF 生态集成**：与 Prometheus/Grafana/Thanos 原生对接

---

## Right-sizing 推荐

### 工具对比

| 工具 | 方法 | 推荐粒度 | 特点 |
|------|------|----------|------|
| Goldilocks | VPA 历史数据统计 | Deployment 级 | Dashboard 直观，使用 VPA recommendation |
| VPA (Vertical Pod Autoscaler) | 实时监控 + 推荐/自动调整 | Pod 级 | 三种模式：Off/Recommendation/Auto |
| StormForge (原 StormForge Optimize) | ML 驱动 + 实验 | 应用级 | 多目标优化（性能+成本），支持 K6 负载测试 |

### Right-sizing 最佳实践

```
1. 基线建立
   └─ Goldilocks Dashboard → 查看 namespace 级建议
   └─ VPA (Off 模式) → 积累 7-14 天数据

2. 渐进调整
   └─ 先对低风险 workload 应用 VPA Recommendation 模式
   └─ 确认稳定后切换 Auto 模式
   └─ 关键服务用 StormForge 做负载测试验证

3. 持续监控
   └─ 设置 Kubecost 告警：实际/请求 比值 < 0.5 触发告警
   └─ 定期审查 OOMKilled 事件，调优 VPA bounds
```

**VPA + HPA 协调要点：**
- VPA 管资源 request/limit（垂直方向）
- HPA 管副本数（水平方向）
- 二者同时作用于 CPU 时需避免冲突 → 建议 VPA 只管 Memory，HPA 管 CPU

---

## FinOps 实践

### FOCUS 规范

FOCUS（FinOps Open Cost and Usage Specification）是 Linux Foundation 发布的云成本数据标准：

- **统一成本格式**：跨云厂商标准化列名、单位、计费周期
- **Kubernetes 成本融合**：与 OpenCost 输出对齐
- **2026 状态**：AWS CUR 2.0 已原生支持 FOCUS 输出，Azure/GCP 跟进中

### 标签策略

```yaml
# 推荐标签体系
app.kubernetes.io/name: api-gateway        # 应用名
app.kubernetes.io/part-of: platform         # 所属产品线
app.kubernetes.io/managed-by: team-foo      # 负责团队
cost-center: CC-1234                        # 成本中心
environment: production                     # 环境
criticality: high                           # 业务关键度
```

**标签治理要点：**
- 使用 Kyverno/OPA 强制标签存在性
- 定期审计无标签资源（Kubecost 提供 Unallocated 视图）
- 建立标签与预算/告警的自动关联

### Showback vs Chargeback

| 模式 | 含义 | 适用场景 |
|------|------|----------|
| **Showback** | 只展示各团队成本，不实际扣费 | 平台团队推动成本意识 |
| **Chargeback** | 成本实际回扣到业务部门预算 | 成熟 FinOps 组织，有独立预算体系 |

**实施路径：**
1. 先推 Showback（成本透明化 → 培养意识）
2. 设定各团队预算阈值 + 告警
3. 逐步过渡到 Chargeback（需要财务系统对接）

---

## 参考架构

```
┌──────────────────────────────────────────────────────────┐
│                    FinOps 平台层                          │
│  Finout / OpenCost Dashboard / Grafana Cost Dashboard     │
├──────────────────────────────────────────────────────────┤
│                    推荐与执行层                            │
│  Goldilocks │ VPA │ Karpenter │ Cast AI Autopilot         │
├──────────────────────────────────────────────────────────┤
│                    预测层                                  │
│  Prophet/LSTM │ KEDA Predictive Scaler │ Kubecost Forecast│
├──────────────────────────────────────────────────────────┤
│                    数据层                                  │
│  Prometheus │ Thanos │ Cloud Billing APIs │ CUR/FOCUS     │
├──────────────────────────────────────────────────────────┤
│                    执行层                                  │
│  Karpenter NodePools │ KEDA ScaledObjects │ kube-green    │
└──────────────────────────────────────────────────────────┘
```

---

## 关键指标

| 指标 | 计算方式 | 目标值 |
|------|----------|--------|
| 集群利用率 (CPU) | 实际使用 / allocatable | > 65% |
| 集群利用率 (Memory) | 实际使用 / allocatable | > 70% |
| 资源效率比 | 实际使用 / 请求量 | > 0.7 |
| 闲置成本 | 请求但未使用的资源成本 | < 总成本 20% |
| 预测准确率 | 1 - MAPE | > 85% |
| Spot 使用率 | Spot 节点占比 | > 40%（非关键 workload）|

---

## 相关概念

- [[horizontal-pod-autoscaler|HPA 水平自动扩缩]]
- 集群自动扩缩
- Karpenter 节点自动供应
- [[keda|KEDA 事件驱动自动扩缩]]
- [[multi-cluster-dr-automation|多集群灾备与自动化]]
- 平台工程

## Related

- [[concepts/finops-greenops-practices.md|finops greenops practices]] — FinOps 与绿色运维实践
- [[concepts/gitops-production-operations.md|gitops production operations]] — GitOps 生产运维
- [[concepts/k8s-ai-ml-infrastructure.md|k8s ai ml infrastructure]] — K8S AI/ML 基础设施
