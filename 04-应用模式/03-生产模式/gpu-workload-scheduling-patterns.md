---
title: "GPU 工作负载调度模式"
description: "生产级 GPU 调度：资源请求模式、拓扑感知、抢占策略、弹性伸缩与多租户 GPU 共享实践"
summary: "覆盖 Kubernetes GPU 工作负载调度的完整实践，包括 GPU 资源请求与限制、节点亲和性与拓扑感知调度、PriorityClass 抢占策略、GPU 虚拟化与共享、弹性伸缩和成本控制。"
category: 应用模式
tags:
- patterns
- gpu
- scheduling
- topology
- preemption
- autoscaling
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- 应用开发者
- SRE
- 架构师
estimated_read_time: 20min
intent_queries:
- "K8s GPU 工作负载调度最佳实践"
- "GPU 拓扑感知调度怎么配置"
- "多租户 GPU 共享和抢占策略"
trigger_keywords:
- GPU
- 调度
- 拓扑感知
- 抢占
- NVIDIA
- 弹性伸缩
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# GPU 工作负载调度模式

> **适用范围**: Kubernetes v1.28–v1.32 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考

## 概述

GPU 是 AI/ML 工作负载的核心资源，其单价高（单卡数万至数十万元）、供给有限、调度复杂。与 CPU 不同，GPU 不可超卖、不可分数分配（原生模式下）、拓扑敏感（NVLink/PCIe 带宽差异巨大）。生产环境中，GPU 调度需要解决：如何高效利用昂贵的 GPU 资源？如何保证训练任务不被推理任务抢占？如何在多团队间公平共享？如何根据负载弹性伸缩 GPU 节点池？

本文覆盖 GPU 工作负载调度的完整实践，从基础的资源请求到高级的拓扑感知和弹性策略。相关内容可参见 [[scheduling-topology-patterns]]、[[resource-qos-rightsizing]]、[[ai-inference-app-patterns]]。

---

## 模式定义与适用场景

### GPU 调度模式对比

| 模式 | 粒度 | 利用率 | 隔离性 | 适用场景 | 实现方式 |
|------|------|--------|--------|---------|---------|
| **整卡独占** | 1 GPU | 中 | 强 | 训练、大模型推理 | `nvidia.com/gpu: N` |
| **GPU 共享（MPS）** | 分数 GPU | 高 | 弱 | 小模型推理、开发 | NVIDIA MPS |
| **GPU 虚拟化（MIG）** | 切片 | 高 | 强 | 多租户推理 | NVIDIA MIG (A100/H100) |
| **GPU 分时复用** | 时间片 | 高 | 中 | 开发/测试 | GPU Time-slicing |
| **GPU 池化** | 远程 GPU | 极高 | 中 | 推理服务池 | HAMi, GPU Operator |

### 工作负载类型与调度策略

| 工作负载 | 优先级 | GPU 类型 | 调度策略 | 弹性需求 |
|---------|--------|---------|---------|---------|
| 大模型训练 | 高 | A100/H100 × 8 | 拓扑感知、Gang Scheduling | 低（固定资源） |
| 模型微调 | 中 | A100 × 1-4 | 队列排队、可抢占 | 中 |
| 在线推理 | 最高 | T4/L4/A10 | 低延迟、高可用 | 高（HPA） |
| 批量推理 | 低 | 任意 | 填充空闲 GPU | 高（KEDA） |
| 开发调试 | 最低 | 任意 | 可被抢占 | 低 |

---

## 架构设计

### GPU 集群分层架构

```
┌─────────────────────────────────────────────────────────┐
│                    调度与控制层                           │
│  ┌────────────┐  ┌────────────┐  ┌────────────────┐    │
│  │ kube-      │  │ GPU        │  │ Topology       │    │
│  │ scheduler  │  │ Operator   │  │ Manager        │    │
│  └────────────┘  └────────────┘  └────────────────┘    │
├─────────────────────────────────────────────────────────┤
│                    节点池层                              │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │ 训练节点池    │  │ 推理节点池    │  │ 开发节点池    │  │
│  │ 8×H100/node │  │ 4×L4/node   │  │ 1×T4/node   │  │
│  │ NVLink 互联  │  │ PCIe 连接   │  │ 共享模式     │  │
│  │ Taint: train│  │ Taint: infer│  │ Taint: dev  │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
├─────────────────────────────────────────────────────────┤
│                    资源管理层                            │
│  PriorityClass / ResourceQuota / Fair Scheduling        │
└─────────────────────────────────────────────────────────┘
```

### GPU 拓扑示意

```
单节点 8×H100 拓扑（NVSwitch 全互联）：

GPU0 ──┐
GPU1 ──┤
GPU2 ──┼── NVSwitch (900 GB/s)
GPU3 ──┤
GPU4 ──┤
GPU5 ──┤
GPU6 ──┤
GPU7 ──┘

跨节点通信：
Node1 [GPU0-7] ──── InfiniBand (400 Gb/s) ──── Node2 [GPU0-7]
```

---

## K8s 实现

### GPU 训练任务（整卡独占 + 拓扑感知）

```yaml
# 🟡 中风险：GPU 任务消耗高价值资源，配置不当造成资源浪费
apiVersion: batch/v1
kind: Job
metadata:
  name: llm-finetune-llama3
  namespace: ml-training
  labels:
    app.kubernetes.io/name: llm-finetune
    kudig.io/workload-type: training
    kudig.io/gpu-type: h100
spec:
  backoffLimit: 2
  activeDeadlineSeconds: 86400  # 最长 24 小时
  ttlSecondsAfterFinished: 7200
  template:
    metadata:
      labels:
        app.kubernetes.io/name: llm-finetune
    spec:
      restartPolicy: Never
      priorityClassName: training-high
      # 调度到训练专用节点池
      nodeSelector:
        nvidia.com/gpu.product: "NVIDIA-H100-80GB"
        workload-type: training
      tolerations:
        - key: "nvidia.com/gpu"
          operator: "Exists"
          effect: "NoSchedule"
        - key: "workload"
          operator: "Equal"
          value: "training"
          effect: "NoSchedule"
      # 拓扑感知：要求 NVLink 互联的 GPU
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
              - matchExpressions:
                  - key: nvidia.com/gpu.count
                    operator: Gte
                    values: ["8"]
                  - key: nvidia.com/gpu.memory
                    operator: Gte
                    values: ["81920"]  # 80GB
      containers:
        - name: trainer
          image: registry.internal/ml/llm-trainer:v2.1.0
          command: ["torchrun", "--nproc_per_node=8", "train.py"]
          args:
            - "--model=llama3-8b"
            - "--data=/data/train"
            - "--output=/output/checkpoint"
            - "--epochs=3"
          resources:
            limits:
              nvidia.com/gpu: 8  # 请求 8 张 GPU
              cpu: "96"
              memory: "512Gi"
              rdma/rdma_shared_device_a: 1  # RDMA 网络
            requests:
              nvidia.com/gpu: 8
              cpu: "64"
              memory: "256Gi"
          env:
            - name: NVIDIA_VISIBLE_DEVICES
              value: "all"
            - name: NCCL_DEBUG
              value: "INFO"
            - name: NCCL_IB_DISABLE
              value: "0"  # 启用 InfiniBand
            - name: CUDA_DEVICE_MAX_CONNECTIONS
              value: "1"
          volumeMounts:
            - name: training-data
              mountPath: /data
            - name: checkpoint
              mountPath: /output
            - name: shm
              mountPath: /dev/shm
      volumes:
        - name: training-data
          persistentVolumeClaim:
            claimName: training-dataset-pvc
        - name: checkpoint
          persistentVolumeClaim:
            claimName: checkpoint-pvc
        - name: shm
          emptyDir:
            medium: Memory
            sizeLimit: "64Gi"  # PyTorch DataLoader 需要大共享内存
```

### GPU 推理服务（MIG 切片 + HPA）

```yaml
# 🟡 中风险：推理服务配置影响在线服务可用性
apiVersion: apps/v1
kind: Deployment
metadata:
  name: llm-inference-service
  namespace: ml-serving
  labels:
    app.kubernetes.io/name: llm-inference
    kudig.io/workload-type: inference
spec:
  replicas: 4
  selector:
    matchLabels:
      app.kubernetes.io/name: llm-inference
  template:
    metadata:
      labels:
        app.kubernetes.io/name: llm-inference
    spec:
      priorityClassName: inference-critical
      nodeSelector:
        nvidia.com/gpu.product: "NVIDIA-L4-24GB"
        workload-type: inference
      tolerations:
        - key: "workload"
          operator: "Equal"
          value: "inference"
          effect: "NoSchedule"
      containers:
        - name: inference
          image: registry.internal/ml/vllm-server:v0.4.2
          args:
            - "--model=/models/qwen2-7b"
            - "--tensor-parallel-size=1"
            - "--max-model-len=4096"
            - "--gpu-memory-utilization=0.90"
          resources:
            limits:
              nvidia.com/gpu: 1  # 单卡推理
              cpu: "8"
              memory: "32Gi"
            requests:
              nvidia.com/gpu: 1
              cpu: "4"
              memory: "16Gi"
          ports:
            - containerPort: 8000
              name: http
          readinessProbe:
            httpGet:
              path: /health
              port: 8000
            initialDelaySeconds: 120  # 模型加载需要时间
            periodSeconds: 10
            failureThreshold: 3
          livenessProbe:
            httpGet:
              path: /health
              port: 8000
            initialDelaySeconds: 180
            periodSeconds: 30
          volumeMounts:
            - name: model-cache
              mountPath: /models
      volumes:
        - name: model-cache
          persistentVolumeClaim:
            claimName: model-cache-pvc
---
# HPA：基于 GPU 利用率和请求队列弹性伸缩
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: llm-inference-hpa
  namespace: ml-serving
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: llm-inference-service
  minReplicas: 2
  maxReplicas: 16
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
        - type: Pods
          value: 2
          periodSeconds: 120
    scaleDown:
      stabilizationWindowSeconds: 600  # 缩容冷却 10 分钟
  metrics:
    - type: Pods
      pods:
        metric:
          name: gpu_utilization
        target:
          type: AverageValue
          averageValue: "75"  # GPU 利用率 > 75% 扩容
    - type: Pods
      pods:
        metric:
          name: inference_queue_depth
        target:
          type: AverageValue
          averageValue: "10"  # 队列深度 > 10 扩容
```

### PriorityClass 与抢占策略

```yaml
# 🟢 低风险：PriorityClass 声明
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: inference-critical
value: 10000
globalDefault: false
preemptionPolicy: PreemptLowerPriority
description: "在线推理服务，最高优先级，可抢占训练和开发任务"
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: training-high
value: 5000
globalDefault: false
preemptionPolicy: PreemptLowerPriority
description: "生产训练任务，可抢占开发任务"
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: training-normal
value: 3000
globalDefault: false
preemptionPolicy: Never  # 不抢占，排队等待
description: "常规训练任务，不抢占其他任务"
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: dev-experiment
value: 100
globalDefault: false
preemptionPolicy: Never
description: "开发实验，最低优先级，随时可被抢占"
---
# GPU Namespace 资源配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: ml-training-quota
  namespace: ml-training
spec:
  hard:
    requests.nvidia.com/gpu: "32"
    limits.nvidia.com/gpu: "32"
    pods: "20"
---
apiVersion: v1
kind: ResourceQuota
metadata:
  name: ml-serving-quota
  namespace: ml-serving
spec:
  hard:
    requests.nvidia.com/gpu: "16"
    limits.nvidia.com/gpu: "16"
    pods: "30"
```

---

## 生产配置示例

### GPU 节点池自动伸缩（Cluster Autoscaler）

```yaml
# 🟡 中风险：节点池配置影响 GPU 资源供给
apiVersion: v1
kind: ConfigMap
metadata:
  name: cluster-autoscaler-config
  namespace: kube-system
data:
  config: |
    # GPU 节点池自动伸缩配置
    node_groups:
      - name: gpu-inference-pool
        min_size: 2
        max_size: 8
        instance_type: "ecs.gn7i-c8g1.2xlarge"  # 1×L4
        labels:
          workload-type: inference
          nvidia.com/gpu.product: "NVIDIA-L4-24GB"
        taints:
          - key: workload
            value: inference
            effect: NoSchedule
        scale_down_delay: 10m
        max_node_provision_time: 15m

      - name: gpu-training-pool
        min_size: 1
        max_size: 4
        instance_type: "ecs.ebmgn7e.32xlarge"  # 8×H100
        labels:
          workload-type: training
          nvidia.com/gpu.product: "NVIDIA-H100-80GB"
        taints:
          - key: workload
            value: training
            effect: NoSchedule
        scale_down_delay: 30m  # 训练节点缩容更保守
        max_node_provision_time: 20m
```

### GPU 监控与成本追踪

```bash
# 🟢 低风险：查看 GPU 节点资源使用
kubectl top nodes -l nvidia.com/gpu.product=NVIDIA-H100-80GB

# 🟢 低风险：查看 GPU 分配情况
kubectl describe nodes -l workload-type=training | \
  grep -A 5 "Allocated resources" | grep nvidia

# 🟢 低风险：查看 GPU Pod 分布
kubectl get pods -A -o wide | grep -E "gpu|nvidia"

# 🟢 低风险：DCGM 指标查询（GPU 利用率、温度、显存）
# Prometheus query:
# DCGM_FI_DEV_GPU_UTIL{namespace="ml-serving"}
# DCGM_FI_DEV_FB_USED{namespace="ml-serving"} / DCGM_FI_DEV_FB_FREE * 100
```

---

## 运维要点

### GPU 调度问题排查

| 症状 | 可能原因 | 排查命令 |
|------|---------|---------|
| Pod Pending + "Insufficient nvidia.com/gpu" | GPU 资源不足 | `kubectl describe pod` + `kubectl describe node` |
| Pod Pending + "node(s) didn't match node selector" | 节点标签不匹配 | `kubectl get nodes --show-labels` |
| Pod Running 但 GPU 利用率 0% | CUDA 初始化失败 | `kubectl logs` + `nvidia-smi` |
| 训练速度异常慢 | 拓扑不对，走了 PCIe | 检查 NCCL_DEBUG 日志 |
| OOM Killed | 显存不足 | 减小 batch_size 或模型 |

### GPU 资源利用率优化

| 策略 | 适用场景 | 预期提升 | 实现复杂度 |
|------|---------|---------|-----------|
| MIG 切片 | 小模型推理 | 利用率 +200% | 中 |
| GPU Time-slicing | 开发/测试 | 利用率 +50% | 低 |
| 批处理填充 | 离线推理 | 利用率 +30% | 低 |
| 混合部署（推理+训练） | 潮汐负载 | 利用率 +40% | 高 |
| 弹性节点池 | 波动负载 | 成本 -30% | 中 |

---

## 反模式

### 反模式 1：GPU 请求不设置 limits

```yaml
# ❌ 错误：只设 requests 不设 limits
resources:
  requests:
    nvidia.com/gpu: 1
```

**后果**：GPU 是扩展资源，K8s 要求 requests = limits。不设 limits 会导致调度失败。

**修正**：GPU 资源必须 requests 和 limits 一致。

### 反模式 2：训练和推理混部无隔离

**后果**：训练任务占满 GPU，推理服务无法调度；或训练任务被推理抢占，训练中断。

**修正**：独立节点池 + Taint/Toleration + PriorityClass 分层。参见 [[scheduling-topology-patterns]]。

### 反模式 3：忽略共享内存配置

```yaml
# ❌ 错误：不配置 /dev/shm
# PyTorch DataLoader 默认使用 /dev/shm，容器默认只有 64MB
```

**后果**：多 Worker 数据加载时 "Bus error" 或 "No space left on device"。

**修正**：挂载 `emptyDir.medium: Memory` 到 `/dev/shm`，大小设为内存的 50%。

### 反模式 4：GPU 节点不做污点隔离

**后果**：普通 CPU Pod 调度到 GPU 节点，占用 CPU/内存资源，GPU 空闲但节点"满"。

**修正**：GPU 节点添加 Taint，只有 GPU 工作负载通过 Toleration 调度。

### 反模式 5：模型加载无 Startup Probe

**后果**：大模型加载需要 2-5 分钟，默认 livenessProbe 在模型加载完成前就 Kill Pod，进入 CrashLoop。

**修正**：使用 `startupProbe` 保护慢启动，`failureThreshold × periodSeconds > 模型加载时间`。参见 [[ai-inference-app-patterns]]。

---

## Related

- [[scheduling-topology-patterns]] — 调度拓扑与节点池设计
- [[resource-qos-rightsizing]] — 资源 QoS 与 Right-sizing
- [[ai-inference-app-patterns]] — AI 推理应用模式
- [[batch-cron-job-patterns]] — 批处理与定时任务模式
- [[cost-optimization-finops]] — 成本优化与 FinOps
- [[serverless-event-driven-patterns]] — Serverless 与事件驱动模式
