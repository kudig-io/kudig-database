---
title: "Volcano 调度器、KAIOPS 与 GPU Operator 2025 更新"
description: "2025-2026 年 Kubernetes AI/ML 工作负载调度最新进展：Volcano v1.10+、KAIOPS、NVIDIA GPU Operator v24/v25 关键特性与生产实践"
summary: "深入解析 Volcano v1.10+ 新特性（ElasticJob、DRA 集成、调度插件框架）、KAIOPS 智能 AI 运维平台、NVIDIA GPU Operator v24/v25 MIG 自动化与 CDI 支持，涵盖 2025 年 Kubernetes AI 调度最佳实践"
category: AI基础设施
tags:
- volcano
- kaiops
- gpu-operator
- ai-scheduling
- kubernetes
- elastic-job
- dra
- mig
- cdi
- gang-scheduling
- fair-share
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- MLOps 工程师
- 平台工程师
- SRE
estimated_read_time: 25min
intent_queries:
- "Volcano v1.10 有哪些新特性"
- "KAIOPS 是什么平台"
- "GPU Operator 2025 如何配置 MIG"
- "K8s AI 工作负载调度最佳实践 2025"
trigger_keywords:
- Volcano
- KAIOPS
- GPU Operator
- ElasticJob
- DRA
- MIG
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
- volcano-basics
sources:
- https://volcano.sh/docs/
- https://github.com/volcano-sh/volcano
- https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/
- https://github.com/NVIDIA/gpu-operator
---

# Volcano 调度器、KAIOPS 与 GPU Operator 2025 更新

> 2025-2026 年 Kubernetes AI/ML 基础设施调度层的关键进展，面向大规模 GPU 集群生产运营。

## Volcano v1.10+ 核心新特性

### ElasticJob 弹性训练

Volcano v1.10 正式 GA 的 ElasticJob 支持训练任务在 GPU 资源不足时动态缩容、资源充足时自动扩容，无需重启作业。

```yaml
apiVersion: batch.volcano.sh/v1alpha1
kind: Job
metadata:
  name: elastic-pytorch-job
spec:
  minAvailable: 4        # 最少副本数，低于此值挂起
  maxRetry: 5
  plugins:
    pytorch:
      - --master
      - --worker
  tasks:
  - replicas: 8
    minAvailable: 4      # 弹性范围：4~8
    name: worker
    template:
      spec:
        containers:
        - name: pytorch
          image: pytorch/pytorch:2.3-cuda12.1-cudnn8-runtime
          resources:
            limits:
              nvidia.com/gpu: "1"
          env:
          - name: ELASTIC_RANK
            valueFrom:
              fieldRef:
                fieldPath: metadata.annotations['volcano.sh/task-index']
```

**关键配置项：**

| 参数 | 说明 | 推荐值 |
|------|------|--------|
| `minAvailable` | 最小可用 Pod 数 | 训练所需最低 rank 数 |
| `maxRetry` | 作业最大重试次数 | 3-5 |
| `ttlSecondsAfterFinished` | 完成后 TTL | 3600 |
| `queue` | 绑定 Queue | 按业务划分 |

### DRA（Dynamic Resource Allocation）集成

Kubernetes 1.32+ 将 DRA 提升为 Beta，Volcano 已完成深度集成，支持更细粒度的 GPU 分配：

```yaml
# DRA ResourceClaim 示例（K8s 1.32+）
apiVersion: resource.k8s.io/v1beta1
kind: ResourceClaim
metadata:
  name: gpu-claim-training
spec:
  devices:
    requests:
    - name: gpu
      deviceClassName: nvidia.com/gpu
      count: 4
      selectors:
      - cel:
          expression: device.attributes["nvidia.com/gpu"].memory >= "40Gi"
---
# Volcano Job 引用 DRA 资源
apiVersion: batch.volcano.sh/v1alpha1
kind: Job
metadata:
  name: dra-training-job
spec:
  tasks:
  - name: worker
    replicas: 2
    template:
      spec:
        resourceClaims:
        - name: gpu-claim-training
          resourceClaimName: gpu-claim-training
        containers:
        - name: trainer
          resources:
            claims:
            - name: gpu-claim-training
```

### 调度插件框架增强

Volcano 1.10 重构了调度插件接口，对齐 K8s 调度框架扩展点：

```go
// 自定义调度插件示例
type AIWorkloadPlugin struct {
    handle framework.Handle
}

func (p *AIWorkloadPlugin) Name() string {
    return "AIWorkloadOptimizer"
}

// PreFilter：检查 GPU 亲和性要求
func (p *AIWorkloadPlugin) PreFilter(ctx context.Context,
    state *framework.CycleState, job *api.JobInfo) *framework.Status {
    // 检查 GPU 拓扑要求
    if requiresNVLink(job) {
        state.Write(NVLinkKey, &NVLinkState{Required: true})
    }
    return framework.NewStatus(framework.Success)
}

// Score：优化 GPU 拓扑分配分数
func (p *AIWorkloadPlugin) Score(ctx context.Context,
    state *framework.CycleState, job *api.JobInfo, nodeName string) (float64, *framework.Status) {
    score := calculateTopologyScore(state, nodeName)
    return score, framework.NewStatus(framework.Success)
}
```

**内置插件 2025 更新：**

| 插件 | 功能 | 更新内容 |
|------|------|---------|
| `gang` | Gang Scheduling | 支持部分就绪触发 |
| `binpack` | 资源装箱 | GPU 感知打分 |
| `proportion` | 公平比例队列 | 层级 Queue 权重 |
| `priority` | 优先级抢占 | 支持作业级别优先级 |
| `tdm` | 时间分片 | GPU 时间片调度 |
| `numa-aware` | NUMA 感知 | NUMA 亲和性优化 |

### Queue 层级管理

```yaml
# 根 Queue
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: root
spec:
  weight: 1
  capability:
    nvidia.com/gpu: "128"
---
# 子 Queue：生产环境
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: production
  annotations:
    scheduling.volcano.sh/queue-parent: root
spec:
  weight: 7
  reclaimable: true
  guarantee:
    resource:
      nvidia.com/gpu: "64"
  capability:
    nvidia.com/gpu: "96"
---
# 子 Queue：研发环境
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: research
  annotations:
    scheduling.volcano.sh/queue-parent: root
spec:
  weight: 3
  reclaimable: true
  guarantee:
    resource:
      nvidia.com/gpu: "32"
```

---

## KAIOPS：智能 AI 基础设施运维平台

KAIOPS（Kubernetes AI Infrastructure Operations）是 2024-2025 年兴起的专为大规模 GPU 集群设计的智能运维平台，整合了调度、监控、成本分析和自动化运维。

### 核心能力

```
┌─────────────────────────────────────────────────────┐
│                   KAIOPS 平台架构                    │
├──────────────┬──────────────┬───────────────────────┤
│ 智能调度引擎  │  GPU 资源治理  │    AI 工作负载洞察     │
│ • 拓扑感知   │ • MIG 自动化  │ • 训练进度预测        │
│ • 弹性调度   │ • 共享 GPU    │ • 异常检测           │
│ • 抢占优化   │ • 配额管理    │ • 成本归因           │
├──────────────┴──────────────┴───────────────────────┤
│           统一 GPU 集群控制平面                       │
│  Kubernetes API + Volcano + NVIDIA GPU Operator      │
└─────────────────────────────────────────────────────┘
```

### KAIOPS 调度策略配置

```yaml
apiVersion: kaiops.io/v1alpha1
kind: AISchedulingPolicy
metadata:
  name: gpu-cluster-policy
spec:
  topologyAware:
    enabled: true
    preferNVLink: true        # 优先使用 NVLink 互联的 GPU
    preferSameHost: true      # 多卡训练优先同一主机
  elasticScaling:
    enabled: true
    scaleDownDelay: 300s      # 空闲 5 分钟后缩容
    utilizationThreshold: 0.2 # GPU 利用率低于 20% 触发缩容
  costOptimization:
    enabled: true
    spotInstanceWeight: 0.6   # 优先使用竞价实例
    reservedInstanceWeight: 0.4
  preemption:
    enabled: true
    gracePeriod: 60s
    checkpointBefore: true    # 抢占前自动 Checkpoint
```

---

## NVIDIA GPU Operator v24/v25 关键更新

### v24.x 重要特性

**1. MIG（Multi-Instance GPU）完全自动化**

```yaml
# ClusterPolicy：启用 MIG 自动配置
apiVersion: nvidia.com/v1
kind: ClusterPolicy
metadata:
  name: gpu-cluster-policy
spec:
  mig:
    strategy: mixed          # mixed / single / none
  migManager:
    enabled: true
    config:
      name: mig-parted-config
      default: "all-balanced"  # 默认 MIG 分区策略
---
# MIG 分区配置 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: mig-parted-config
  namespace: gpu-operator
data:
  config.yaml: |
    version: v1
    mig-configs:
      all-1g.10gb:
        - devices: all
          mig-enabled: true
          mig-devices:
            "1g.10gb": 7
      all-2g.20gb:
        - devices: all
          mig-enabled: true
          mig-devices:
            "2g.20gb": 3
      all-balanced:
        - devices: [0,1,2,3]
          mig-enabled: true
          mig-devices:
            "1g.10gb": 2
            "2g.20gb": 1
            "3g.40gb": 1
        - devices: [4,5,6,7]
          mig-enabled: true
          mig-devices:
            "4g.40gb": 1
            "3g.40gb": 1
```

**2. CDI（Container Device Interface）全面支持**

```yaml
apiVersion: nvidia.com/v1
kind: ClusterPolicy
metadata:
  name: gpu-cluster-policy
spec:
  cdi:
    enabled: true
    default: true     # 使用 CDI 作为默认设备注入方式
  devicePlugin:
    config:
      name: device-plugin-config
      default: "default"
```

**3. 时间切片（Time-Slicing）增强**

```yaml
# 每张 GPU 虚拟化为 8 个时间片设备
apiVersion: v1
kind: ConfigMap
metadata:
  name: device-plugin-config
  namespace: gpu-operator
data:
  default: |
    version: v1
    flags:
      migStrategy: none
    sharing:
      timeSlicing:
        resources:
        - name: nvidia.com/gpu
          replicas: 8
        - name: nvidia.com/mig-1g.10gb
          replicas: 2
```

### v25.x 展望特性

| 特性 | 状态 | 说明 |
|------|------|------|
| IMEX 通道支持 | GA | 跨节点 GPU 内存扩展 |
| NIM 集成 | Beta | NVIDIA Inference Microservice 原生集成 |
| Grace-Hopper 优化 | GA | ARM + GPU 统一内存架构支持 |
| Confidential Computing | Beta | H100 TEE 模式支持 |
| vGPU 云原生 | Alpha | 虚拟化 GPU 在 K8s 原生管理 |

### GPU Operator 监控配置

```yaml
apiVersion: nvidia.com/v1
kind: ClusterPolicy
metadata:
  name: gpu-cluster-policy
spec:
  dcgmExporter:
    enabled: true
    config:
      name: dcgm-exporter-config
    serviceMonitor:
      enabled: true
      interval: 15s
      honorLabels: false
      additionalLabels:
        monitoring: prometheus
---
# DCGM 自定义指标
apiVersion: v1
kind: ConfigMap
metadata:
  name: dcgm-exporter-config
  namespace: gpu-operator
data:
  dcgm-metrics.csv: |
    DCGM_FI_DEV_GPU_UTIL,     gauge, GPU utilization (percent)
    DCGM_FI_DEV_MEM_COPY_UTIL,gauge, Memory utilization (percent)
    DCGM_FI_DEV_FB_FREE,      gauge, Framebuffer memory free (MB)
    DCGM_FI_DEV_FB_USED,      gauge, Framebuffer memory used (MB)
    DCGM_FI_DEV_GPU_TEMP,     gauge, GPU temperature (C)
    DCGM_FI_DEV_POWER_USAGE,  gauge, Power usage (W)
    DCGM_FI_DEV_SM_CLOCK,     gauge, SM clock frequency (MHz)
    DCGM_FI_DEV_MEM_CLOCK,    gauge, Memory clock frequency (MHz)
    DCGM_FI_DEV_NVLINK_BANDWIDTH_TOTAL, counter, NVLink bandwidth
    DCGM_FI_PROF_GR_ENGINE_ACTIVE, gauge, Compute engine active ratio
    DCGM_FI_PROF_PIPE_TENSOR_ACTIVE, gauge, Tensor core active ratio
```

---

## 2025 AI 调度最佳实践

### 生产环境调度策略矩阵

| 场景 | 调度器 | 策略 | 优先级 |
|------|--------|------|--------|
| LLM 预训练（百亿+） | Volcano | Gang + NVLink 拓扑感知 | 最高 |
| SFT 微调（数十亿） | Volcano | ElasticJob + 弹性 | 高 |
| 推理服务 | K8s 原生 | HPA + KEDA | 中 |
| 批量推理 | Kueue | BestEffortFIFO | 低 |
| 研究实验 | Volcano | Fair-share Queue | 最低 |

### 多租户 GPU 配额管理

```yaml
# ResourceQuota 结合 Volcano Queue
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-a-gpu-quota
  namespace: team-a
spec:
  hard:
    requests.nvidia.com/gpu: "32"
    limits.nvidia.com/gpu: "32"
    requests.nvidia.com/mig-1g.10gb: "48"
---
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: team-a
spec:
  weight: 3
  guarantee:
    resource:
      nvidia.com/gpu: "16"
  capability:
    nvidia.com/gpu: "48"     # 允许借用空闲资源上限
```

### 调度性能调优

```yaml
# volcano-scheduler ConfigMap 调优参数
apiVersion: v1
kind: ConfigMap
metadata:
  name: volcano-scheduler-configmap
  namespace: volcano-system
data:
  volcano-scheduler.conf: |
    actions: "enqueue, allocate, backfill, preempt"
    tiers:
    - plugins:
      - name: priority
      - name: gang
        enablePreemptable: true
      - name: conformance
    - plugins:
      - name: overused
      - name: allocatable
    - plugins:
      - name: predicates
      - name: proportion
      - name: nodeorder
        weight: 3
      - name: binpack
        arguments:
          binpack.weight: 10
          binpack.cpu: 1
          binpack.memory: 1
          binpack.resources: nvidia.com/gpu
          binpack.resources.nvidia.com/gpu: 10  # GPU 优先装箱
```

---

## 故障排查

### Volcano 调度常见问题

```bash
# 查看 Job 调度状态
kubectl get vcjob -n ai-training -o wide

# 查看作业事件
kubectl describe vcjob my-training-job -n ai-training

# 查看 Queue 资源使用
kubectl get queue -o custom-columns=\
  NAME:.metadata.name,\
  WEIGHT:.spec.weight,\
  GPU_CAPABILITY:.spec.capability."nvidia\.com/gpu",\
  GPU_ALLOCATED:.status.allocated."nvidia\.com/gpu",\
  GPU_PENDING:.status.pending."nvidia\.com/gpu"

# 查看调度器日志
kubectl logs -n volcano-system deployment/volcano-scheduler --tail=100 | \
  grep -E "Error|Failed|Preempt|Evict"

# 检查 Gang Scheduling 等待状态
kubectl get pods -n ai-training -l volcano.sh/job-name=my-training-job \
  --field-selector=status.phase=Pending
```

### GPU Operator 故障排查

```bash
# 检查 GPU Operator 所有组件状态
kubectl get pods -n gpu-operator

# 查看节点 GPU 分配状态
kubectl get node <node-name> -o jsonpath='{.status.allocatable}' | jq '
  . | {
    "gpu": .["nvidia.com/gpu"],
    "mig_1g": .["nvidia.com/mig-1g.10gb"],
    "mig_3g": .["nvidia.com/mig-3g.40gb"]
  }'

# 检查 MIG 配置状态
kubectl exec -n gpu-operator ds/nvidia-mig-manager -- nvidia-smi mig -lgi

# 验证 DCGM 指标采集
kubectl port-forward -n gpu-operator svc/nvidia-dcgm-exporter 9400:9400 &
curl -s localhost:9400/metrics | grep DCGM_FI_DEV_GPU_UTIL
```

---

## 参考资源

- [Volcano 官方文档](https://volcano.sh/docs/)
- [NVIDIA GPU Operator 文档](https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/)
- [Kubernetes DRA KEP](https://github.com/kubernetes/enhancements/tree/master/keps/sig-node/3063-dynamic-resource-allocation)
- [MIG 用户指南](https://docs.nvidia.com/datacenter/tesla/mig-user-guide/)
