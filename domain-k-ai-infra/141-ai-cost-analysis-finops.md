# 141 - AI成本分析与FinOps实践 (AI Cost Analysis & FinOps)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-01 | **参考**: [FinOps Foundation](https://www.finops.org/)

---

## 一、AI成本结构全景 (Cost Structure Overview)

### 1.1 成本构成分析

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        AI/ML 基础设施成本构成                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │  计算成本 (Compute) 60-75%                                           │  │
│  │  ├── GPU实例 (A100/H100/L40S)                    45-55%             │  │
│  │  ├── CPU实例 (数据处理/编排)                      10-15%             │  │
│  │  └── Spot/Preemptible实例                        5-10%              │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │  存储成本 (Storage) 15-25%                                           │  │
│  │  ├── 对象存储 (S3/GCS/OSS)                       8-12%              │  │
│  │  ├── 块存储 (高性能SSD)                          5-8%               │  │
│  │  └── 文件存储 (NFS/Lustre/GPFS)                  2-5%               │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │  网络成本 (Network) 5-10%                                            │  │
│  │  ├── 跨区域/跨AZ传输                             3-5%               │  │
│  │  ├── 公网出流量                                  1-3%               │  │
│  │  └── 专线/VPN                                    1-2%               │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │  其他成本 (Others) 5-10%                                             │  │
│  │  ├── 监控/日志/APM                               2-3%               │  │
│  │  ├── 备份/灾备                                   1-2%               │  │
│  │  └── 软件许可证                                  2-5%               │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 各阶段成本分布

| 阶段 | 计算占比 | 存储占比 | 网络占比 | 特点 |
|-----|---------|---------|---------|------|
| **数据准备** | 30% | 55% | 15% | I/O密集,大量数据迁移 |
| **模型训练** | 85% | 10% | 5% | GPU密集,checkpoint频繁 |
| **模型微调** | 75% | 15% | 10% | 中等GPU需求 |
| **模型推理** | 70% | 10% | 20% | 稳定GPU,高网络吞吐 |
| **MLOps平台** | 40% | 30% | 30% | 元数据/模型仓库 |

---

## 二、GPU实例成本对比 (GPU Instance Pricing)

### 2.1 主流云厂商GPU实例定价

| GPU型号 | 显存 | AWS On-Demand | AWS Spot | GCP On-Demand | Azure On-Demand | 阿里云 |
|--------|------|---------------|----------|---------------|-----------------|-------|
| **H100 80GB** | 80GB | $32.77/h | ~$12/h | $37.20/h | $31.58/h | ¥185/h |
| **A100 80GB** | 80GB | $32.77/h | ~$10/h | $25.20/h | $24.48/h | ¥120/h |
| **A100 40GB** | 40GB | $19.50/h | ~$6/h | $15.12/h | $14.69/h | ¥85/h |
| **A10G 24GB** | 24GB | $1.006/h | ~$0.30/h | $1.02/h | $0.90/h | ¥12/h |
| **L4 24GB** | 24GB | $0.81/h | ~$0.25/h | $0.72/h | $0.85/h | ¥10/h |
| **T4 16GB** | 16GB | $0.526/h | ~$0.16/h | $0.35/h | $0.45/h | ¥5/h |
| **V100 32GB** | 32GB | $3.06/h | ~$0.92/h | $2.48/h | $2.48/h | ¥35/h |

### 2.2 训练任务GPU选型指南

| 模型规模 | 推荐GPU | 数量 | 预估训练成本/天 | 适用场景 |
|---------|--------|------|----------------|---------|
| **<1B参数** | A10G/L4 | 1-4 | $25-100 | 小型模型/微调 |
| **1-7B参数** | A100 40GB | 4-8 | $400-800 | 中型LLM |
| **7-13B参数** | A100 80GB | 8-16 | $1,500-3,000 | 大型LLM微调 |
| **13-70B参数** | A100 80GB/H100 | 32-64 | $6,000-15,000 | 大模型预训练 |
| **>70B参数** | H100 | 128-512 | $50,000+ | 超大规模预训练 |

### 2.3 推理服务GPU选型

| 模型类型 | 推荐GPU | QPS/卡 | 成本/1M tokens | 适用场景 |
|---------|--------|--------|---------------|---------|
| **7B量化模型** | T4/L4 | 50-100 | $0.05-0.10 | 低成本在线服务 |
| **13B量化模型** | A10G/L4 | 30-60 | $0.10-0.20 | 中等质量服务 |
| **70B量化模型** | A100 40GB | 10-20 | $0.50-1.00 | 高质量服务 |
| **70B FP16** | A100 80GB | 5-10 | $2.00-4.00 | 最高质量服务 |

---

## 三、FinOps实践框架 (FinOps Framework)

### 3.1 FinOps成熟度模型

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         FinOps 成熟度模型                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Level 3: 优化 (Optimize)                                                   │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ • 自动化成本优化策略                                                  │   │
│  │ • 预测性容量规划                                                      │   │
│  │ • 持续成本工程                                                        │   │
│  │ • ROI驱动的资源决策                                                   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                              ↑                                              │
│  Level 2: 运营 (Operate)                                                    │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ • 成本分配和showback/chargeback                                       │   │
│  │ • 预算管理和告警                                                      │   │
│  │ • 异常检测和分析                                                      │   │
│  │ • 定期成本审计                                                        │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                              ↑                                              │
│  Level 1: 感知 (Inform)                                                     │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ • 成本可见性建立                                                      │   │
│  │ • 资源标签体系                                                        │   │
│  │ • 基础成本报表                                                        │   │
│  │ • 团队成本意识培养                                                    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Kubernetes资源标签体系

```yaml
# 成本追踪标签规范
apiVersion: v1
kind: Pod
metadata:
  labels:
    # 业务维度
    app.kubernetes.io/name: "llm-inference"
    app.kubernetes.io/component: "serving"
    
    # 成本中心
    cost-center: "ai-platform"
    business-unit: "search"
    project: "chatbot-v2"
    
    # 环境
    environment: "production"
    
    # 责任人
    owner: "ml-team"
    contact: "ml-lead@company.com"
    
    # 工作负载类型
    workload-type: "inference"       # training/inference/data-processing
    gpu-type: "a100"
    
    # 生命周期
    lifecycle: "persistent"          # persistent/ephemeral/spot
    
    # 成本优先级
    cost-priority: "p1"              # p0(关键)/p1(重要)/p2(普通)/p3(可中断)
```

### 3.3 成本分配模型

```yaml
# Kubecost 成本分配配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: kubecost-allocation
  namespace: kubecost
data:
  allocation.yaml: |
    # 共享成本分摊规则
    sharedCosts:
      - name: "control-plane"
        type: "cluster"
        allocation: "proportional"  # 按资源使用比例分摊
        
      - name: "monitoring"
        type: "namespace"
        namespaces: ["monitoring", "logging"]
        allocation: "even"          # 平均分摊
        
      - name: "gpu-operator"
        type: "namespace"
        namespaces: ["gpu-operator-resources"]
        allocation: "gpu-weighted"  # 按GPU使用量分摊
    
    # Idle资源成本
    idleCosts:
      cpuIdleCost: 0.5              # 50%分配到用户
      gpuIdleCost: 1.0              # 100%分配到用户
      shareWithNamespaces: true
    
    # 网络成本
    networkCosts:
      enabled: true
      zoneCostMultiplier: 0.01
      regionCostMultiplier: 0.05
      internetCostMultiplier: 0.12
```

---

## 四、成本优化策略 (Cost Optimization Strategies)

### 4.1 计算资源优化

#### Spot/Preemptible实例策略

```yaml
# Karpenter Spot实例配置
apiVersion: karpenter.sh/v1alpha5
kind: Provisioner
metadata:
  name: gpu-spot-provisioner
spec:
  requirements:
    - key: "karpenter.sh/capacity-type"
      operator: In
      values: ["spot", "on-demand"]
    - key: "node.kubernetes.io/instance-type"
      operator: In
      values: ["p4d.24xlarge", "p3.16xlarge", "g5.48xlarge"]
  
  # Spot实例优先
  weight: 100
  
  # 中断处理
  ttlSecondsAfterEmpty: 30
  ttlSecondsUntilExpired: 86400
  
  # 混合策略: 70% Spot + 30% On-Demand
  limits:
    resources:
      nvidia.com/gpu: 100
  
  providerRef:
    name: default
    
---
# Volcano调度器Spot感知配置
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: training-spot
spec:
  weight: 10
  capability:
    nvidia.com/gpu: 80
  reclaimable: true        # 可被抢占
  
  # 优先使用Spot节点
  nodeSelector:
    karpenter.sh/capacity-type: spot
```

#### GPU利用率优化

```yaml
# GPU Time-Slicing配置 (提升利用率)
apiVersion: v1
kind: ConfigMap
metadata:
  name: nvidia-device-plugin-config
  namespace: gpu-operator-resources
data:
  config.yaml: |
    version: v1
    sharing:
      timeSlicing:
        renameByDefault: false
        resources:
        - name: nvidia.com/gpu
          replicas: 4           # 每张GPU模拟4张
          
---
# MIG配置 (A100/H100)
apiVersion: v1
kind: ConfigMap
metadata:
  name: mig-config
  namespace: gpu-operator-resources
data:
  config.yaml: |
    version: v1
    mig-configs:
      # A100 80GB: 7个10GB实例
      a100-80gb:
        - devices: all
          mig-enabled: true
          mig-devices:
            "1g.10gb": 7
      
      # A100 80GB: 3个20GB + 1个40GB
      a100-80gb-mixed:
        - devices: all
          mig-enabled: true
          mig-devices:
            "2g.20gb": 3
            "4g.40gb": 1
```

### 4.2 训练成本优化

| 优化技术 | 显存节省 | 速度影响 | 成本节省 | 实现复杂度 |
|---------|---------|---------|---------|----------|
| **混合精度训练(AMP)** | 40-50% | +10-30% | 40-50% | 低 |
| **梯度累积** | 60-80% | -10-20% | 30-40% | 低 |
| **梯度检查点** | 50-70% | -20-30% | 25-35% | 中 |
| **ZeRO-Offload** | 80%+ | -30-50% | 40-60% | 中 |
| **模型并行** | N/A | -10-20% | 扩展性 | 高 |
| **Early Stopping** | N/A | +20-40% | 20-40% | 低 |
| **学习率调度** | N/A | +10-20% | 10-20% | 低 |

```python
# DeepSpeed ZeRO配置示例
deepspeed_config = {
    "zero_optimization": {
        "stage": 3,
        "offload_optimizer": {
            "device": "cpu",
            "pin_memory": True
        },
        "offload_param": {
            "device": "cpu",
            "pin_memory": True
        },
        "overlap_comm": True,
        "contiguous_gradients": True,
        "reduce_bucket_size": 5e7,
        "stage3_prefetch_bucket_size": 5e7,
        "stage3_param_persistence_threshold": 1e5
    },
    "fp16": {
        "enabled": True,
        "loss_scale": 0,
        "initial_scale_power": 16
    },
    "gradient_accumulation_steps": 8,
    "gradient_clipping": 1.0,
    "train_batch_size": "auto",
    "train_micro_batch_size_per_gpu": "auto"
}
```

### 4.3 推理成本优化

| 优化技术 | 延迟影响 | 吞吐提升 | 成本节省 | 精度损失 |
|---------|---------|---------|---------|---------|
| **INT8量化** | -10% | +50-100% | 50-60% | <1% |
| **INT4量化** | -20% | +100-200% | 70-80% | 1-3% |
| **KV Cache优化** | 0% | +30-50% | 20-30% | 0% |
| **Continuous Batching** | -5% | +200-400% | 60-75% | 0% |
| **Speculative Decoding** | +20% | +50-100% | 30-40% | 0% |
| **Flash Attention** | -10% | +100-200% | 40-50% | 0% |
| **PagedAttention** | 0% | +200-300% | 50-70% | 0% |

```yaml
# vLLM高效推理配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-inference
spec:
  template:
    spec:
      containers:
      - name: vllm
        image: vllm/vllm-openai:latest
        args:
        - "--model=/models/llama-2-70b"
        - "--tensor-parallel-size=4"
        - "--quantization=awq"            # AWQ量化
        - "--max-model-len=4096"
        - "--gpu-memory-utilization=0.9"  # 显存利用率90%
        - "--enable-prefix-caching"       # 前缀缓存
        - "--max-num-seqs=256"            # 最大并发
        resources:
          limits:
            nvidia.com/gpu: 4
          requests:
            memory: "64Gi"
            cpu: "16"
```

### 4.4 存储成本优化

```yaml
# 存储分层策略
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ai-data-tiered
provisioner: csi.juicefs.com
parameters:
  # 热数据: NVMe SSD
  juicefs-secret-name: "juicefs-secret"
  juicefs-secret-namespace: "default"
  
  # 存储分层配置
  storage-tiers: |
    tier-hot:
      backend: "nvme-ssd"
      capacity: "1Ti"
      ttl: "7d"
    tier-warm:
      backend: "ssd"
      capacity: "10Ti"
      ttl: "30d"
    tier-cold:
      backend: "s3"
      capacity: "unlimited"
      
---
# 数据生命周期管理
apiVersion: batch/v1
kind: CronJob
metadata:
  name: data-lifecycle-manager
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: lifecycle
            image: data-lifecycle-manager:latest
            env:
            - name: HOT_TO_WARM_DAYS
              value: "7"
            - name: WARM_TO_COLD_DAYS
              value: "30"
            - name: DELETE_AFTER_DAYS
              value: "365"
```

---

## 五、成本监控与告警 (Cost Monitoring & Alerting)

### 5.1 Kubecost部署配置

```yaml
# Kubecost Helm Values
kubecostModel:
  # 云账单集成
  cloudIntegration:
    enabled: true
    aws:
      athenaProjectID: "aws-cost-project"
      athenaBucketName: "s3://cur-bucket"
      athenaRegion: "us-east-1"
      athenaDatabase: "cur_database"
      athenaTable: "cur_table"
  
  # GPU成本
  gpuCost:
    enabled: true
    # 自定义GPU单价 ($/h)
    prices:
      nvidia.com/gpu:
        a100-80gb: 32.77
        a100-40gb: 19.50
        a10g: 1.006
        t4: 0.526
  
  # 预算配置
  budgets:
    enabled: true
    configs:
      - namespace: "ml-training"
        monthly: 50000
        alertThresholds: [50, 75, 90, 100]
      - namespace: "ml-inference"
        monthly: 30000
        alertThresholds: [50, 75, 90, 100]

prometheus:
  server:
    retention: "30d"
    
grafana:
  enabled: true
  dashboards:
    enabled: true
```

### 5.2 成本告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ai-cost-alerts
  namespace: monitoring
spec:
  groups:
  - name: ai-cost-alerts
    rules:
    # 日成本超预算
    - alert: DailyCostOverBudget
      expr: |
        sum(increase(kubecost_cluster_costs_daily[24h])) > 5000
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "日成本超过$5000"
        description: "当前日成本: ${{ $value | humanize }}"
    
    # GPU空闲告警
    - alert: GPUIdleHigh
      expr: |
        (1 - avg(DCGM_FI_DEV_GPU_UTIL) / 100) > 0.5
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "GPU空闲率超过50%"
        description: "平均GPU利用率: {{ $value | humanizePercentage }}"
    
    # Spot实例中断风险
    - alert: SpotInstanceInterruptionRisk
      expr: |
        sum(karpenter_interruption_actions_performed) > 5
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Spot实例频繁中断"
        description: "5分钟内中断次数: {{ $value }}"
    
    # 存储成本异常增长
    - alert: StorageCostSpike
      expr: |
        (sum(kubecost_pv_hourly_cost) - sum(kubecost_pv_hourly_cost offset 1d)) 
        / sum(kubecost_pv_hourly_cost offset 1d) > 0.2
      for: 1h
      labels:
        severity: warning
      annotations:
        summary: "存储成本日增长超过20%"
    
    # 网络成本异常
    - alert: NetworkCostAnomaly
      expr: |
        sum(rate(kubecost_network_zone_egress_cost[1h])) > 100
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "跨区网络成本异常"
        description: "每小时网络成本: ${{ $value | humanize }}"
```

### 5.3 成本Dashboard (Grafana)

```json
{
  "title": "AI Infrastructure Cost Overview",
  "panels": [
    {
      "title": "总成本趋势 (Daily)",
      "type": "timeseries",
      "targets": [{
        "expr": "sum(increase(kubecost_cluster_costs_daily[24h]))",
        "legendFormat": "Daily Cost"
      }]
    },
    {
      "title": "成本分布 (By Namespace)",
      "type": "piechart",
      "targets": [{
        "expr": "sum by (namespace) (kubecost_namespace_hourly_cost * 24 * 30)",
        "legendFormat": "{{namespace}}"
      }]
    },
    {
      "title": "GPU成本效率",
      "type": "gauge",
      "targets": [{
        "expr": "avg(DCGM_FI_DEV_GPU_UTIL) / 100 * 100"
      }],
      "thresholds": {
        "steps": [
          {"color": "red", "value": 0},
          {"color": "yellow", "value": 50},
          {"color": "green", "value": 75}
        ]
      }
    },
    {
      "title": "Spot节省金额",
      "type": "stat",
      "targets": [{
        "expr": "sum(kubecost_savings_spot_monthly)"
      }]
    }
  ]
}
```

---

## 六、成本优化案例 (Optimization Case Studies)

### 6.1 训练任务优化案例

| 优化项 | 优化前 | 优化后 | 节省 | 年化节省 |
|-------|-------|-------|------|---------|
| **Spot实例** | 100% On-Demand | 70% Spot | 49% | $180,000 |
| **混合精度训练** | FP32 | AMP | 45% | $120,000 |
| **GPU利用率** | 35% | 75% | 53% | $150,000 |
| **存储分层** | 全SSD | 热/温/冷分层 | 60% | $48,000 |
| **预留实例** | 按需 | 1年预留 | 35% | $80,000 |
| **调度优化** | 手动 | Kueue自动 | 20% | $40,000 |
| **总计** | - | - | **51%** | **$618,000** |

### 6.2 推理服务优化案例

| 服务 | 原配置 | 优化后 | 延迟影响 | 成本节省 |
|-----|-------|-------|---------|---------|
| **ChatBot** | 8x A100 FP16 | 4x A100 INT4 | +5ms | 65% |
| **搜索排序** | 16x V100 | 8x A10G | -2ms | 70% |
| **图像生成** | 4x A100 | 2x A100 MIG | +10ms | 50% |
| **语音识别** | 8x T4 | 4x T4 Batch | +50ms | 50% |

---

## 七、FinOps工具生态 (FinOps Tools)

### 7.1 成本管理工具对比

| 工具 | 类型 | 云支持 | K8s原生 | GPU成本 | 开源 | 价格 |
|-----|------|-------|--------|--------|------|------|
| **Kubecost** | 成本分析 | AWS/GCP/Azure | 是 | 是 | 是 | 免费/企业版 |
| **OpenCost** | 成本分析 | 多云 | 是 | 是 | 是 | 免费 |
| **Vantage** | FinOps平台 | 多云 | 是 | 是 | 否 | 付费 |
| **CloudHealth** | FinOps平台 | 多云 | 部分 | 否 | 否 | 付费 |
| **Spot.io** | 优化 | AWS/GCP/Azure | 是 | 是 | 否 | 付费 |
| **CAST AI** | 优化 | 多云 | 是 | 是 | 否 | 付费 |

### 7.2 OpenCost快速部署

```bash
# 部署OpenCost
helm install opencost opencost/opencost \
  --namespace opencost \
  --create-namespace \
  --set opencost.prometheus.internal.enabled=true \
  --set opencost.ui.enabled=true \
  --set opencost.exporter.defaultClusterId="production" \
  --set opencost.customPricing.enabled=true \
  --set opencost.customPricing.configmapName="opencost-custom-pricing"

# 自定义GPU价格
kubectl create configmap opencost-custom-pricing -n opencost --from-file=pricing.json
```

---

## 八、成本治理最佳实践 (Governance Best Practices)

### 8.1 成本治理流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        成本治理周期                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐  │
│  │   计划      │───→│   执行      │───→│   检查      │───→│   改进      │  │
│  │   Plan      │    │   Do        │    │   Check     │    │   Act       │  │
│  └─────────────┘    └─────────────┘    └─────────────┘    └─────────────┘  │
│        │                  │                  │                  │          │
│        ▼                  ▼                  ▼                  ▼          │
│  • 制定预算          • 资源标签         • 成本报告         • 优化策略      │
│  • 成本预测          • 成本分配         • 异常分析         • 策略调整      │
│  • 目标设定          • 自动化执行       • KPI评估          • 流程改进      │
│                                                                             │
│  ════════════════════════════════════════════════════════════════════════  │
│                              月度循环                                        │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 8.2 成本优化检查清单

```markdown
## 月度成本审计检查清单

### 计算资源
- [ ] GPU利用率 > 70%
- [ ] CPU利用率 > 50%
- [ ] Spot实例占比 > 60% (可中断任务)
- [ ] 无长期空闲GPU实例
- [ ] 预留实例覆盖率达标

### 存储资源
- [ ] 存储分层策略生效
- [ ] 无孤儿PV/PVC
- [ ] 快照保留策略合理
- [ ] 跨区域复制必要性验证

### 网络资源
- [ ] 跨区流量最小化
- [ ] CDN覆盖率优化
- [ ] 专线利用率合理

### 治理合规
- [ ] 所有资源100%标签覆盖
- [ ] 成本分配准确性验证
- [ ] 预算告警有效性测试
- [ ] 成本异常根因分析完成
```

---

## 九、快速参考 (Quick Reference)

### 9.1 成本计算公式

```
# GPU小时成本
GPU小时成本 = GPU单价 × 使用时长 × (1 - Spot折扣)

# 训练总成本
训练总成本 = GPU小时成本 × GPU数量 × 训练时长 + 存储成本 + 网络成本

# 推理服务月成本
月成本 = (GPU单价 × 24 × 30) × 实例数 × (1 + 冗余系数)

# 成本效率
成本效率 = 模型性能提升 / 成本增加
ROI = (业务价值 - 总成本) / 总成本 × 100%
```

### 9.2 常用命令

```bash
# Kubecost成本查询
kubectl cost namespace --window 7d --show-all-resources

# OpenCost API查询
curl http://opencost.opencost:9003/allocation/compute?window=7d

# GPU利用率查询
kubectl exec -it dcgm-exporter-xxx -- dcgmi dmon -e 203,204

# Spot实例状态
kubectl get nodes -l karpenter.sh/capacity-type=spot

# 成本标签覆盖率检查
kubectl get pods -A -o json | jq '[.items[] | select(.metadata.labels["cost-center"] == null)] | length'
```

---

**成本优化原则**: Spot优先 → 提升利用率 → 存储分层 → 持续监控

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
