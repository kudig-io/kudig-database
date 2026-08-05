---
title: FinOps 与 GreenOps 实践
summary: FinOps 与 GreenOps 实践：FinOps 基金会定义的成熟度模型将组织的云财务管理能力划分为三个阶段：
category: concepts
tags:
- finops
- greenops
- cost
- sustainability
- gpu
- k8s
tier: core
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# FinOps 与 GreenOps 实践

> 关联索引：[[32-发布/package/2026-07-02_18-53/corpus/supporting/skills/training-lecturer/11-workloads/index|index]] · [[concepts/capacity-planning-cost-optimization.md|capacity planning cost optimization]]

---

## 1. FinOps 成熟度模型

### 1.1 三阶段演进框架（Crawl → Walk → Run）

FinOps 基金会定义的成熟度模型将组织的云财务管理能力划分为三个阶段：

| 维度 | Crawl（起步） | Walk（成长） | Run（成熟） |
|------|--------------|-------------|------------|
| **可见性** | 月度手动成本报告，缺乏标签覆盖 | 自动化仪表盘，标签覆盖率 >80% | 实时成本异常告警，标签覆盖率 >95% |
| **优化** | 手动 Right-sizing，定期清理闲置资源 | 预留实例/Savings Plan 自动推荐，K8S HPA/VPA 集成 | 持续优化引擎，Spot 混合策略自动化 |
| **运营** | 无预算管控，事后核算 | 预算告警 + Showback，成本中心分配 | 预测性预算管理，Chargeback 精确到团队 |
| **组织** | 无专职 FinOps 团队 | 设立 FinOps Champion，跨部门协作 | FinOps 嵌入工程文化，自动化决策 |
| **频率** | 月度回顾 | 双周回顾 | 实时/每周回顾 |

### 1.2 能力级评估矩阵

FinOps 基金会的能力框架包含六大支柱：

```
┌─────────────────────────────────────────────────────────┐
│                 FinOps 能力级评估                         │
├──────────────┬──────────────────────────────────────────┤
│ 1. 理解云     │ 云支出归因、分摊模型、标签治理              │
│ 2. 量化价值   │ 业务指标关联、单位经济性（$/请求、$/用户）    │
│ 3. 优化使用率  │ Right-sizing、闲置清理、Spot/RI 策略        │
│ 4. 优化速率   │ 折扣计划管理、承诺使用率监控                  │
│ 5. 管理异常   │ 预算告警、异常支出检测、根因分析              │
│ 6. 建立实践   │ 文化建设、跨职能协作、持续改进               │
└──────────────┴──────────────────────────────────────────┘
```

**评估打分**：每项能力按 1-5 分评估，1 分 = 无实践，3 分 = 基本自动化，5 分 = 行业领先。总分 30 分，<12 = Crawl，12-22 = Walk，>22 = Run。

---

## 2. FOCUS 规范 1.0+

### 2.1 规范概述

FOCUS（FinOps Open Cost and Usage Specification）是 FinOps 基金会发布的开放标准，旨在统一多云成本与使用量数据格式，消除供应商锁定。

- **版本**：1.0 GA（2024 年发布），1.1+ 持续演进
- **目标**：统一 AWS、Azure、GCP 等云厂商的计费数据列定义
- **许可证**：Apache 2.0 开源

### 2.2 核心数据字段

| 字段名 | 说明 | 示例 |
|--------|------|------|
| `BillingAccountId` | 计费账户 ID | `123456789012` |
| `BillingAccountName` | 计费账户名称 | `Production-Primary` |
| `ChargeCategory` | 费用类别 | `Usage`, `Purchase`, `Credit` |
| `ChargePeriodStart/End` | 费用区间 | `2026-05-01T00:00:00Z` |
| `EffectiveCost` | 折后实际成本 | `$0.0342` |
| `ListCost` | 标价成本 | `$0.0456` |
| `ResourceId` | 资源唯一标识 | `arn:aws:ec2:...` |
| `ServiceName` | 服务名称 | `Amazon Elastic Compute Cloud` |
| `SkuId` | SKU 编码 | 标准化商品编码 |
| `ConsumedQuantity` | 消费数量 | `720` |
| `ConsumedUnit` | 消费单位 | `Hours` |

### 2.3 多云厂商支持

```
# 🟢 低风险：只读/信息收集，通常无副作用
AWS:  CUR 2.0 → FOCUS 导出（原生支持 2024+）
Azure: Cost Management API → FOCUS 导出（2024+原生支持）
GCP:  BigQuery Billing Export → FOCUS 映射（社区工具 + 官方适配器）
```
### 2.4 实践建议

1. **统一导入管道**：使用 OpenCost 或 CloudZero 等工具将 CUR/ACR/Billing Export 转换为 FOCUS 格式
2. **BigQuery/ClickHouse 存储**：将标准化数据入湖，支持多维聚合分析
3. **标签对齐**：确保三云的标签键名在 FOCUS 映射层统一（如 `team` → `Team`）

---

## 3. Kubernetes 成本分配策略

### 3.1 三层成本分配模型

```
┌──────────────────────────────────────────────┐
│  集群级公共成本（控制平面、共享节点池）          │
│  ┌──────────────────────────────────────────┐ │
│  │  Namespace 级成本（RBAC 边界 = 成本边界）   │ │
│  │  ┌──────────────────────────────────────┐│ │
│  │  │  Pod/工作负载级成本（精确归因）        ││ │
│  │  └──────────────────────────────────────┘│ │
│  └──────────────────────────────────────────┘ │
└──────────────────────────────────────────────┘
```

### 3.2 Namespace-Based 分配

```yaml
# 通过 K8s namespace 作为成本分配的主维度
# 对应 RBAC 隔离边界，天然与团队/产品对齐
apiVersion: v1
kind: Namespace
metadata:
  name: team-ml-platform
  labels:
    cost-center: "ML-2026"
    environment: "production"
    owner: "ml-eng@example.com"
```

**策略**：每个团队/产品独立 namespace，namespace 级汇总 CPU/Memory/PVC/Network 成本。

### 3.3 Label-Based 分配

```yaml
# 推荐标签体系（参考 Kubernetes 推荐标签）
metadata:
  labels:
    app.kubernetes.io/name: inference-service
    app.kubernetes.io/version: "v2.1.0"
    app.kubernetes.io/component: model-server
    app.kubernetes.io/part-of: ml-platform
    cost-allocation/team: ml-platform
    cost-allocation/env: production
    cost-allocation/feature: llm-inference
```

**标签治理**：使用 Kyverno/OPA 强制关键标签存在性，标签缺失的工作负载自动告警。

### 3.4 Pod-Level 精确归因

```yaml
# OpenCost + Prometheus 集成，按 pod 原始请求/实际使用计算成本
# 成本公式：
#   pod_cost = (pod_cpu_request / node_cpu_capacity) * node_hourly_cost
#            + (pod_mem_request / node_mem_capacity) * node_hourly_cost
#            + gpu_hours * gpu_hourly_cost
```

**工具链**：
- **OpenCost**：CNCF 沙箱项目，K8s 原生成本监控
- **Kubecost**：商业版，支持 SaaS 和私有部署
- **Kubernetes Cost Allocator**（Helm Chart）：轻量方案

---

## 4. Resource Quota + LimitRange 最佳实践

### 4.1 纵深防御架构

```
Layer 1: LimitRange（命名空间级默认值兜底）
  ↓
Layer 2: ResourceQuota（命名空间级总量上限）
  ↓
Layer 3: Cluster Autoscaler/Karpenter（集群级资源池约束）
  ↓
Layer 4: Cloud Provider Budget Alerts（账单级终防线）
```

### 4.2 LimitRange 配置

```yaml
apiVersion: v1
kind: LimitRange
metadata:
  name: team-ml-defaults
  namespace: team-ml-platform
spec:
  limits:
  - type: Container
    default:          # 默认 limit
      cpu: "2"
      memory: "4Gi"
    defaultRequest:   # 默认 request
      cpu: "500m"
      memory: "1Gi"
    max:              # 上限
      cpu: "16"
      memory: "64Gi"
    min:              # 下限
      cpu: "100m"
      memory: "128Mi"
  - type: Pod
    max:
      cpu: "32"
      memory: "128Gi"
  - type: PersistentVolumeClaim
    max:
      storage: "500Gi"
    min:
      storage: "1Gi"
```

### 4.3 ResourceQuota 配置

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-ml-quota
  namespace: team-ml-platform
spec:
  hard:
    requests.cpu: "64"
    requests.memory: "256Gi"
    limits.cpu: "128"
    limits.memory: "512Gi"
    requests.storage: "2Ti"
    persistentvolumeclaims: "20"
    pods: "100"
    services: "20"
    # GPU 独立配额（需启用 ExtendedResource 配额）
    requests.nvidia.com/gpu: "16"
    limits.nvidia.com/gpu: "16"
```

### 4.4 GPU 独立配额管理

```yaml
# 对 MIG 分片后的 GPU 资源单独配额
# NVIDIA GPU Operator 注册的扩展资源
apiVersion: v1
kind: ResourceQuota
metadata:
  name: gpu-quota-inference
  namespace: team-ml-inference
spec:
  hard:
    requests.nvidia.com/gpu: "8"
    requests.nvidia.com/mig-1g.5gb: "32"   # MIG 1/7 切片
    requests.nvidia.com/mig-3g.20gb: "8"   # MIG 3/7 切片
```

**关键实践**：
- LimitRange 的 `defaultRequest` 设置合理的 request 值，避免未指定 request 的 Pod 消耗过多配额
- GPU 配额独立于 CPU/Memory 管理，防止普通工作负载抢占 GPU 预算
- 使用 `scopeSelector` 实现不同优先级的差异化配额

---

## 5. GreenOps / kube-green

### 5.1 绿色运营理念

GreenOps 将碳排放和能源效率纳入运营决策，在降成本的同时减少环境影响。核心指标：

- **PUE**（Power Usage Effectiveness）：数据中心能源效率
- **CUE**（Carbon Usage Effectiveness）：碳排放效率
- **K8s 能耗**：Pod 级功耗监控（通过 Kepler）

### 5.2 kube-green 与 SleepInfo CRD

[kube-green](https://github.com/kube-green/kube-green) 是一个 K8s Operator，通过 CRD 定义非工作时间自动休眠策略。

```yaml
apiVersion: kube-green.com/v1alpha1
kind: SleepInfo
metadata:
  name: dev-environment-sleep
  namespace: team-dev
spec:
  weekdays: "1-5"              # 周一到周五
  sleepAt: "20:00"             # 晚 8 点开始休眠
  wakeUpAt: "08:00"            # 早 8 点恢复
  timeZone: "Asia/Shanghai"
  suspendDeployments: true     # 暂停 Deployment（replicas→0）
  suspendCronJobs: true        # 暂停 CronJob
  excludeRef:                  # 排除关键工作负载
  - apiVersion: "apps/v1"
    kind: "Deployment"
    name: "monitoring-stack"
```

**效果**：非工作时间（20:00-08:00 + 周末）自动缩容至 0，开发/测试环境成本降低 **60-70%**。

### 5.3 Kepler 能耗监控

[Kepler](https://sustainable-computing.io/)（Kubernetes Efficient Power Level Exporter）基于 eBPF 估算 Pod 级功耗：

```yaml
# Kepler 部署（Helm）
helm repo add kepler https://sustainable-computing-io.github.io/kepler-helm-chart
helm install kepler kepler/kepler \
  --namespace kepler --create-namespace \
  --set enable.gpu=true
```

**Prometheus 查询示例**：
```promql
# 按 namespace 的能耗（瓦时）
sum(rate(kepler_container_core_joules_total[5m])) by (namespace) * 3600

# 按 workload 的碳排放（假设电网碳强度 0.5 kgCO2/kWh）
sum(rate(kepler_container_package_joules_total{pod=~".*inference.*"}[5m]))
  * 3600 / 1000 * 0.5
```

### 5.4 GreenOps 综合策略

| 策略 | 节省幅度 | 实施难度 |
|------|---------|---------|
| kube-green 开发环境休眠 | 60-70% | 低 |
| Spot 实例替换 | 60-90% | 中 |
| Right-sizing（VPA 推荐） | 20-40% | 低 |
| 架构优化（Serverless/异步） | 30-50% | 高 |
| 区域选择（低碳区域） | 10-20% 碳排放 | 低 |

---

## 6. GPU 成本优化

### 6.1 NVIDIA MIG（Multi-Instance GPU）

MIG 将 A100/H100 物理 GPU 切割为最多 7 个独立实例，每实例有独立的显存、缓存和计算单元：

```yaml
# H100 MIG 切分方案
# 7 个 1g.20gb 实例（适合小模型推理）
# 或 3 个 3g.40gb + 1 个 1g.10gb（混合工作负载）

# 节点标签由 GPU Operator 自动设置
# nvidia.com/mig-1g.20gb.count = "7"
# nvidia.com/mig-3g.40gb.count = "3"

# Pod 请求 MIG 实例
resources:
  requests:
    nvidia.com/mig-1g.20gb: "1"
  limits:
    nvidia.com/mig-1g.20gb: "1"
```

**收益**：小模型推理任务无需独占整块 A100（$2.5/h → $0.36/h per MIG slice），GPU 利用率提升 3-5 倍。

### 6.2 NVIDIA Time-Slicing

对于非 MIG GPU（如 A10G、T4），通过 time-slicing 实现多 Pod 共享：

```yaml
# GPU Operator ClusterPolicy 配置
apiVersion: nvidia.com/v1
kind: ClusterPolicy
metadata:
  name: gpu-cluster-policy
spec:
  migManager:
    enabled: false
  devicePlugin:
    config:
      name: time-slicing-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: time-slicing-config
  namespace: nvidia-gpu-operator
data:
  any: |-
    version: v1
    flags:
      migStrategy: none
    sharing:
      timeSlicing:
        resources:
        - name: nvidia.com/gpu
          replicas: 4  # 每块 GPU 最多 4 个 Pod 共享
```

### 6.3 Spot GPU 实例策略

| 云厂商 | Spot GPU 实例 | 典型折扣 | 中断率 |
|--------|--------------|---------|--------|
| AWS | p4d.24xlarge (A100) | 60-70% | 中 |
| AWS | g5.xlarge (A10G) | 70-80% | 低 |
| Azure | NC A100 v4 | 60-75% | 中 |
| GCP | a2-highgpu (A100) | 60-70% | 中 |
| Lambda Labs | A100/H100 | 按需价 | N/A |

### 6.4 DCGM Right-sizing

NVIDIA DCGM（Data Center GPU Manager）提供 GPU 利用率监控，驱动 Right-sizing 决策：

```promql
# GPU 利用率持续低于 20% 的工作负载 → Right-sizing 候选
avg_over_time(DCGM_FI_DEV_GPU_UTIL{namespace="inference"}[7d]) < 20

# 显存使用率（如果 <30% 可降级到更小实例）
avg_over_time(DCGM_FI_DEV_FB_USED{namespace="inference"}[7d])
  / DCGM_FI_DEV_FB_FREE{namespace="inference"} < 0.3

# 显存带宽利用率（判断是否 I/O bound）
rate(DCGM_FI_DEV_FB_READ_THROUGHPUT[5m]) + rate(DCGM_FI_DEV_FB_WRITE_THROUGHPUT[5m])
```

### 6.5 推理引擎优化

| 引擎 | 优化技术 | 吞吐提升 | 场景 |
|------|---------|---------|------|
| **vLLM** | PagedAttention + Continuous Batching | 2-4x | LLM 推理 |
| **TensorRT-LLM** | 量化（INT8/FP8）+ In-flight Batching | 3-8x | LLM 推理（NVIDIA 优化） |
| **TensorRT** | 图优化 + Kernel Fusion + Dynamic Batching | 2-5x | CV/NLP 模型 |
| **ONNX Runtime** | 图优化 + Execution Provider 选择 | 1.5-3x | 跨平台推理 |
| **DeepSpeed-FastGen** | SplitFuse + Dynamic SplitFuse | 2-3x | 超大模型 |

**量化成本收益**：FP16→INT8 可减少 50% 显存，同一 GPU 可服务更大 batch 或更大模型。

---

## 7. Spot 实例策略

### 7.1 70/30 混合架构

```
生产集群容量分配：
┌─────────────────────────────────────────────────┐
│  On-Demand (30%)          │  Spot (70%)          │
│  ┌───────────────────┐    │  ┌──────────────────┐│
│  │ Stateful 工作负载  │    │  │ 无状态/可中断     ││
│  │ 核心服务 (HA)      │    │  │ 批处理任务        ││
│  │ 数据库主节点       │    │  │ 训练任务          ││
│  │ 消息队列           │    │  │ 推理（多副本）     ││
│  └───────────────────┘    │  └──────────────────┘│
└─────────────────────────────────────────────────┘
```

**定价优势**：综合成本降低 **40-60%**，同时保证核心服务稳定性。

### 7.2 Karpenter 自动分散策略

Karpenter 是 AWS 原生 K8s 节点自动扩缩器，支持 Spot 智能分散：

```yaml
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: spot-general
spec:
  template:
    spec:
      requirements:
      - key: karpenter.sh/capacity-type
        operator: In
        values: ["spot"]
      - key: node.kubernetes.io/instance-type
        operator: In
        values:
        - m5.2xlarge
        - m5a.2xlarge
        - m5d.2xlarge
        - m6i.2xlarge
        - m6a.2xlarge
        - m7i.2xlarge
        - m7a.2xlarge    # 多机型分散，降低同时中断风险
      - key: topology.kubernetes.io/zone
        operator: In
        values:
        - us-east-1a
        - us-east-1b
        - us-east-1c      # 多可用区分散
  disruption:
    consolidationPolicy: WhenUnderutilized
    expireAfter: 720h     # 30 天后自动替换，避免长尾实例
  limits:
    cpu: "1000"
    memory: "4000Gi"
---
# On-Demand 回退池
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: ondemand-fallback
spec:
  weight: 10              # 低优先级，Spot 满足不了时回退
  template:
    spec:
      requirements:
      - key: karpenter.sh/capacity-type
        operator: In
        values: ["on-demand"]
```

### 7.3 PodDisruptionBudget + 优雅终止

```yaml
# PDB 保护：确保 Spot 中断时至少保留 80% 副本
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: inference-pdb
  namespace: inference
spec:
  minAvailable: "80%"
  selector:
    matchLabels:
      app: llm-inference
---
# Deployment 配置优雅终止
apiVersion: apps/v1
kind: Deployment
metadata:
  name: llm-inference
spec:
  template:
    spec:
      terminationGracePeriodSeconds: 120  # 给推理任务足够时间完成
      containers:
      - name: inference
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 15"]  # 从 LB 摘除延迟
        env:
        - name: NVIDIA_DISABLE_REQUIRE
          value: "1"
```

### 7.4 Spot 中断处理最佳实践

1. **SQS/Kafka 缓冲**：请求队列吸收中断导致的容量波动
2. **多副本 + HPA**：维持 3+ 副本，单节点中断影响 <33%
3. **检查点机制**：训练任务定期保存 checkpoint 到 S3/GCS
4. **中断预测**：使用 AWS Spot Interruption Warning（2 分钟预警）触发优雅迁移
5. **优雅降级**：中断时自动切至 on-demand pool（Karpenter weight 机制）

---

## 成本优化综合决策树

```
新工作负载上线
  ├── 是否有状态？─ 是 → On-Demand + 预留实例
  │                  └── 是否可异步？─ 是 → 批处理队列 + Spot
  ├── GPU 工作负载？
  │     ├── 训练 → Spot GPU + Checkpoint + 多机型分散
  │     └── 推理 → MIG 切分 + 时间片共享 + vLLM/TensorRT
  ├── 非生产环境？─ 是 → kube-green 休眠 + 最小实例
  └── 生产无状态？─ 是 → 70/30 Spot/On-Demand + Karpenter
        └── PDB + 优雅终止 + HPA 弹性保护
```

---

## 参考资源

- [FinOps Foundation - Maturity Model](https://www.finops.org/framework/)
- [FOCUS Specification](https://focus.finops.org/)
- [OpenCost](https://www.opencost.io/)
- [kube-green](https://kube-green.dev/)
- [Kepler](https://sustainable-computing.io/)
- [Karpenter](https://karpenter.sh/)
- [NVIDIA GPU Operator](https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/)
- [vLLM](https://docs.vllm.ai/)

## Related

- [[concepts/capacity-planning-cost-optimization.md|capacity planning cost optimization]] — 容量规划与成本优化
- [[concepts/storage-performance-optimization.md|storage performance optimization]] — 存储性能优化策略
- [[concepts/k8s-ai-ml-infrastructure.md|k8s ai ml infrastructure]] — K8S AI/ML 基础设施


<!-- risk-assessed -->
