---
title: GPU Cluster Scheduling and Inference Serving on Kubernetes
description: K8s AI 基础设施 — GPU 调度策略、推理服务部署、模型服务化、资源隔离、弹性扩缩、成本优化
summary: 在 Kubernetes 上构建 GPU 集群的调度策略与 AI 推理服务部署的生产实践
category: practice
tags:
- gpu
- inference
- scheduling
- model-serving
- ai-infrastructure
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: ai-infrastructure
---
# GPU 集群调度与推理服务

> K8s 上 GPU 资源管理与 AI 推理服务的生产实践。

## GPU 集群架构

```
┌─────────────────────────────────────────────────────────────┐
│  AI 平台架构                                                 │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │  推理服务层                                          │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐           │   │
│  │  │Triton    │ │vLLM      │ │TGI       │           │   │
│  │  │Inference │ │(LLM)     │ │(HF)      │           │   │
│  │  └──────────┘ └──────────┘ └──────────┘           │   │
│  ├─────────────────────────────────────────────────────┤   │
│  │  调度层                                              │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐           │   │
│  │  │GPU       │ │Node      │ │Kueue     │           │   │
│  │  │Operator  │ │Affinity  │ │(队列)    │           │   │
│  │  └──────────┘ └──────────┘ └──────────┘           │   │
│  ├─────────────────────────────────────────────────────┤   │
│  │  基础设施层                                          │   │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐           │   │
│  │  │NVIDIA    │ │GPU Node  │ │高速网络  │           │   │
│  │  │Device    │ │Pool      │ │(RDMA)    │           │   │
│  │  │Plugin    │ │          │ │          │           │   │
│  │  └──────────┘ └──────────┘ └──────────┘           │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

## GPU 资源管理

### NVIDIA Device Plugin

```yaml
# 安装 NVIDIA GPU Operator
# helm install gpu-operator nvidia/gpu-operator
apiVersion: v1
kind: ConfigMap
metadata:
  name: gpu-operator-config
  namespace: gpu-operator
data:
  config.yaml: |
    daemonsets:
      tolerations:
        - key: nvidia.com/gpu
          operator: Exists
          effect: NoSchedule
    devicePlugin:
      config:
        name: device-plugin-config
        default: default
---
# GPU 共享配置（MIG 或时间片）
apiVersion: v1
kind: ConfigMap
metadata:
  name: device-plugin-config
  namespace: gpu-operator
data:
  default: |
    version: v1
    sharing:
      timeSlicing:
        resources:
          - name: nvidia.com/gpu
            replicas: 4  # 每个 GPU 虚拟为 4 个
  mig-single: |
    version: v1
    sharing:
      mig:
        strategy: single
```

### GPU 节点池标签

```bash
# 节点标签（按 GPU 类型分组）
kubectl label nodes gpu-node-1 gpu-type=a100 gpu-memory=80g
kubectl label nodes gpu-node-2 gpu-type=a100 gpu-memory=80g
kubectl label nodes gpu-node-3 gpu-type=t4 gpu-memory=16g
kubectl label nodes gpu-node-4 gpu-type=h100 gpu-memory=80g

# 污点（专用 GPU 节点）
kubectl taint nodes gpu-node-1 nvidia.com/gpu=present:NoSchedule
```

## GPU 调度策略

### 基本 GPU 请求

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: inference-pod
spec:
  tolerations:
    - key: nvidia.com/gpu
      operator: Exists
      effect: NoSchedule
  nodeSelector:
    gpu-type: a100
  containers:
    - name: model-server
      image: nvcr.io/nvidia/tritonserver:24.01-py3
      resources:
        requests:
          nvidia.com/gpu: 1
          cpu: "8"
          memory: "32Gi"
        limits:
          nvidia.com/gpu: 1
          cpu: "16"
          memory: "64Gi"
      env:
        - name: NVIDIA_VISIBLE_DEVICES
          value: "all"
        - name: CUDA_VISIBLE_DEVICES
          value: "0"
```

### 多 GPU 训练任务

```yaml
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: llm-finetune
spec:
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      template:
        spec:
          containers:
            - name: pytorch
              image: registry.example.com/training:v1
              resources:
                limits:
                  nvidia.com/gpu: 4
              command:
                - torchrun
                - --nproc_per_node=4
                - train.py
                - --model=llama-7b
                - --data=/data/train.jsonl
          volumes:
            - name: data
              persistentVolumeClaim:
                claimName: training-data
            - name: shm
              emptyDir:
                medium: Memory
                sizeLimit: 16Gi  # 共享内存（NCCL 需要）
```

### Kueue 队列管理（多租户）

```yaml
# 集群队列（资源池）
apiVersion: kueue.x-k8s.io/v1beta1
kind: ClusterQueue
metadata:
  name: gpu-cluster-queue
spec:
  cohort: all-gpus
  resourceGroups:
    - coveredResources: ["nvidia.com/gpu", "cpu", "memory"]
      flavors:
        - name: a100-flavor
          resources:
            - name: nvidia.com/gpu
              nominalQuota: 16
            - name: cpu
              nominalQuota: 128
            - name: memory
              nominalQuota: 512Gi
        - name: t4-flavor
          resources:
            - name: nvidia.com/gpu
              nominalQuota: 8
---
# 本地队列（团队配额）
apiVersion: kueue.x-k8s.io/v1beta1
kind: LocalQueue
metadata:
  name: ml-team-queue
  namespace: ml-team
spec:
  clusterQueue: gpu-cluster-queue
---
# 工作负载
apiVersion: kueue.x-k8s.io/v1beta1
kind: Workload
metadata:
  name: training-job
  namespace: ml-team
spec:
  queueName: ml-team-queue
  podSets:
    - count: 1
      template:
        spec:
          containers:
            - name: trainer
              resources:
                limits:
                  nvidia.com/gpu: 4
```

## 推理服务部署

### vLLM（LLM 推理）

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: llm-inference
  namespace: ai-serving
spec:
  replicas: 2
  selector:
    matchLabels:
      app: llm-inference
  template:
    metadata:
      labels:
        app: llm-inference
    spec:
      tolerations:
        - key: nvidia.com/gpu
          operator: Exists
      nodeSelector:
        gpu-type: a100
      containers:
        - name: vllm
          image: vllm/vllm-openai:latest
          args:
            - --model=/models/llama-3-8b
            - --tensor-parallel-size=1
            - --max-model-len=8192
            - --gpu-memory-utilization=0.90
            - --enable-prefix-caching
            - --max-num-seqs=256
          ports:
            - containerPort: 8000
          resources:
            limits:
              nvidia.com/gpu: 1
              memory: "64Gi"
          volumeMounts:
            - name: model-storage
              mountPath: /models
          readinessProbe:
            httpGet:
              path: /health
              port: 8000
            initialDelaySeconds: 120  # 模型加载慢
            periodSeconds: 10
          startupProbe:
            httpGet:
              path: /health
              port: 8000
            failureThreshold: 60
            periodSeconds: 5
      volumes:
        - name: model-storage
          persistentVolumeClaim:
            claimName: model-pvc
---
apiVersion: v1
kind: Service
metadata:
  name: llm-inference
  namespace: ai-serving
spec:
  selector:
    app: llm-inference
  ports:
    - port: 8000
      targetPort: 8000
```

### Triton Inference Server

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: triton-server
  namespace: ai-serving
spec:
  replicas: 3
  template:
    spec:
      containers:
        - name: triton
          image: nvcr.io/nvidia/tritonserver:24.01-py3
          args:
            - tritonserver
            - --model-repository=/models
            - --strict-model-config=false
          ports:
            - containerPort: 8000  # HTTP
            - containerPort: 8001  # gRPC
            - containerPort: 8002  # Metrics
          resources:
            limits:
              nvidia.com/gpu: 1
          livenessProbe:
            httpGet:
              path: /v2/health/live
              port: 8000
          readinessProbe:
            httpGet:
              path: /v2/health/ready
              port: 8000
```

## 弹性扩缩

### GPU 推理自动扩缩

```yaml
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: llm-inference-scaler
  namespace: ai-serving
spec:
  scaleTargetRef:
    name: llm-inference
  minReplicaCount: 1
  maxReplicaCount: 8
  pollingInterval: 15
  cooldownPeriod: 300
  triggers:
    # 基于请求队列深度
    - type: prometheus
      metadata:
        serverAddress: http://prometheus.monitoring:9090
        metricName: vllm_num_requests_waiting
        query: |
          sum(vllm:num_requests_waiting{app="llm-inference"})
        threshold: "10"
    # 基于 GPU 利用率
    - type: prometheus
      metadata:
        serverAddress: http://prometheus.monitoring:9090
        metricName: gpu_utilization
        query: |
          avg(DCGM_FI_DEV_GPU_UTIL{namespace="ai-serving"})
        threshold: "80"
```

## 成本优化

| 策略 | 节省 | 适用场景 |
|------|------|----------|
| GPU 时间片共享 | 50-70% | 小模型推理 |
| MIG 分区 | 40-60% | 多租户隔离 |
| Spot GPU 实例 | 60-80% | 训练/批处理 |
| 模型量化（INT8/INT4） | 50%（显存） | 推理精度可接受 |
| 自动缩零（Knative） | 90%（空闲时） | 低频推理 |
| 混合精度（FP16） | 50%（显存） | 训练/推理 |

## 监控告警

```yaml
# GPU 监控告警规则
groups:
  - name: gpu-alerts
    rules:
      - alert: GPUMemoryHigh
        expr: DCGM_FI_DEV_FB_USED / DCGM_FI_DEV_FB_TOTAL > 0.95
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "GPU {{ $labels.gpu }} 显存使用 > 95%"
      - alert: GPUTemperatureHigh
        expr: DCGM_FI_DEV_GPU_TEMP > 85
        for: 5m
        labels:
          severity: critical
      - alert: GPUXIDError
        expr: increase(DCGM_FI_DEV_XID_ERRORS[5m]) > 0
        labels:
          severity: critical
        annotations:
          summary: "GPU XID 错误，可能硬件故障"
```

## Related

- [[15-AI基础设施/index.md|AI 基础设施]]
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/skills/training-lecturer/11-workloads/index|GPU 调度]]
- [[02-工作负载/index.md|工作负载]]
- [[13-生产运维/01-成本治理/index.md|成本治理]]
