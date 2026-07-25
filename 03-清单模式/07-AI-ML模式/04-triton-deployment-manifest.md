---
title: Triton 推理部署清单
description: NVIDIA Triton Inference Server 部署配置
summary: Triton Inference Server 部署清单，支持多框架（TensorRT/PyTorch/ONNX）、动态批处理与多模型服务
category: manifests-patterns
tags:
- k8s
- manifests
- ai-ml-infra
- triton
- inference
- nvidia
- tensorrt
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 平台工程师
- SRE
estimated_read_time: 12min
intent_queries:
- Triton 如何部署
- Triton Inference Server Kubernetes
- 多框架推理服务
trigger_keywords:
- triton
- inference
- tensorrt
- onnx
- dynamic-batching
prerequisites:
- gpu-basics
- k8s-deployment-basics
authors:
- name: KUDIG Team
  role: contributor
---

# Triton 推理部署清单

## 1. Triton 优势

NVIDIA Triton Inference Server 是生产级推理服务：

| 特性 | 说明 |
|------|------|
| **多框架** | TensorRT、PyTorch、ONNX、TensorFlow |
| **动态批处理** | 自动合并请求提高吞吐 |
| **多模型** | 单实例服务多个模型 |
| **模型版本管理** | 无需重启即可切换版本 |
| **GPU/CPU 混合** | 同实例可服务 CPU 和 GPU 模型 |

## 2. 基础部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: triton-server
  namespace: ai-inference
spec:
  replicas: 1
  selector:
    matchLabels:
      app: triton-server
  template:
    metadata:
      labels:
        app: triton-server
    spec:
      nodeSelector:
        nvidia.com/gpu.product: "NVIDIA-A100-SXM4-40GB"
      tolerations:
        - key: nvidia.com/gpu
          operator: Exists
      containers:
        - name: triton
          image: nvcr.io/nvidia/tritonserver:24.05-py3
          args:
            - tritonserver
            - --model-repository=/models
            - --strict-model-config=false
            - --log-verbose=1
          ports:
            - containerPort: 8000
              name: http
            - containerPort: 8001
              name: grpc
            - containerPort: 8002
              name: metrics
          resources:
            limits:
              nvidia.com/gpu: 1
              memory: 16Gi
              cpu: "4"
          env:
            - name: OMP_NUM_THREADS
              value: "4"
          volumeMounts:
            - name: model-repository
              mountPath: /models
              readOnly: true
            - name: dshm
              mountPath: /dev/shm
          livenessProbe:
            httpGet:
              path: /v2/health/live
              port: 8000
            initialDelaySeconds: 30
            periodSeconds: 10
          readinessProbe:
            httpGet:
              path: /v2/health/ready
              port: 8000
            initialDelaySeconds: 10
            periodSeconds: 5
      volumes:
        - name: model-repository
          persistentVolumeClaim:
            claimName: model-repo-pvc
        - name: dshm
          emptyDir:
            medium: Memory
            sizeLimit: 8Gi
```

## 3. 模型仓库结构

```
models/
├── resnet50/
│   ├── 1/
│   │   └── model.plan          # TensorRT 引擎
│   ├── config.pbtxt            # 模型配置
│   └── labels.txt
├── bert-base/
│   ├── 1/
│   │   └── model.onnx          # ONNX 模型
│   └── config.pbtxt
└── ensemble/
    ├── 1/
    │   └── model.py            # Python 后端
    └── config.pbtxt
```

## 4. 模型配置（config.pbtxt）

```
name: "resnet50"
platform: "tensorrt_plan"
max_batch_size: 32              # 动态批处理最大批次

input [
  {
    name: "input"
    data_type: TYPE_FP32
    dims: [ 3, 224, 224 ]
  }
]

output [
  {
    name: "output"
    data_type: TYPE_FP32
    dims: [ 1000 ]
  }
]

dynamic_batching {
  preferred_batch_size: [ 4, 8, 16, 32 ]
  max_queue_delay_microseconds: 50000   # 50ms 等待凑批
  preserve_ordering: false
}

instance_group [
  {
    kind: KIND_GPU
    count: 1
    gpus: [ 0 ]
  }
]

optimization {
  execution_accelerators {
    gpu_execution_accelerator : [
      {
        name : "tensorrt"
      }
    ]
  }
}
```

## 5. 模型仓库 PVC（NFS/对象存储）

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: model-repo-pvc
  namespace: ai-inference
spec:
  accessModes:
    - ReadWriteMany              # 多 Pod 共享
  resources:
    requests:
      storage: 100Gi
  storageClassName: nfs-client
```

## 6. 多模型 GPU 共享部署

```yaml
# 通过 MIG 或时间切片让多个 Triton 共享 GPU
apiVersion: apps/v1
kind: Deployment
metadata:
  name: triton-multi-model
spec:
  replicas: 2
  template:
    spec:
      containers:
        - name: triton
          image: nvcr.io/nvidia/tritonserver:24.05-py3
          args:
            - tritonserver
            - --model-repository=/models
            - --model-load-mode=explicit   # 按需加载
            - --pinned-memory-pool-byte-size=1073741824  # 1GB
            - --cuda-memory-pool-byte-size=0:2147483648  # GPU0: 2GB
          resources:
            limits:
              nvidia.com/gpu: 1
              memory: 8Gi
```

## 7. Service 与 GRPC

```yaml
apiVersion: v1
kind: Service
metadata:
  name: triton-service
  namespace: ai-inference
spec:
  selector:
    app: triton-server
  ports:
    - name: http
      port: 8000
      targetPort: http
    - name: grpc
      port: 8001
      targetPort: grpc
    - name: metrics
      port: 8002
      targetPort: metrics
```

## 8. ServiceMonitor

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: triton-monitor
  namespace: ai-inference
spec:
  selector:
    matchLabels:
      app: triton-server
  endpoints:
    - port: metrics
      path: /metrics
      interval: 15s
```

关键指标：

| 指标 | 说明 |
|------|------|
| `nv_inference_request_success` | 成功推理次数 |
| `nv_inference_request_failure` | 失败推理次数 |
| `nv_inference_exec_count` | 执行计数 |
| `nv_inference_request_duration_us` | 请求延迟（微秒） |
| `nv_inference_queue_duration_us` | 队列等待时间 |
| `nv_inference_input_throughput` | 输入吞吐 |
| `nv_gpu_memory_used_bytes` | GPU 内存使用 |

## 9. 优雅升级（模型版本切换）

```bash
# 🟡 中风险：模型版本操作
# Triton 支持热加载新模型版本
# 只需在模型目录创建新版本子目录
mkdir -p models/resnet50/2/
cp new_model.plan models/resnet50/2/

# Triton 自动检测并加载
# 通过 API 显式加载/卸载
curl -X POST http://triton-service:8000/v2/repository/models/resnet50/load

# 卸载旧版本
curl -X DELETE http://triton-service:8000/v2/repository/models/reslion50
```

## 10. 生产实践

| 实践 | 说明 |
|------|------|
| 使用 TensorRT 优化 | 比 PyTorch 快 3-5 倍 |
| 配置 `max_batch_size` | 根据模型和 GPU 显存调整 |
| 启用动态批处理 | 提升吞吐量 |
| 使用 `strict-model-config=false` | 自动从模型文件推断配置 |
| 模型仓库用共享存储 | NFS/S3，多 Pod 共享 |
| 监控队列延迟 | 过高说明需要扩容 |

## Related

- [[03-清单模式/07-AI-ML模式/03-vllm-deployment-manifest|vLLM 部署]]
- [[03-清单模式/07-AI-ML模式/07-model-serving-hpa|KEDA + HPA 弹性]]

## See Also

- [Triton Inference Server 文档](https://docs.nvidia.com/deeplearning/triton-inference-server/)
- [Triton 模型仓库](https://github.com/triton-inference-server/server)

<!-- risk-assessed -->
