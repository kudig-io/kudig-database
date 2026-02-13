# 132 - AI/ML工作负载运维 (AI/ML Workloads Operations)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-01 | **参考**: [Kubeflow](https://www.kubeflow.org/), [Ray](https://ray.io/)

---

## 一、AI工作负载全景 (AI Workloads Overview)

### 1.1 AI/ML工作负载生命周期

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    AI/ML 工作负载生命周期                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐   ┌─────────┐      │
│  │ 数据准备 │──→│ 模型训练 │──→│ 模型评估 │──→│ 模型部署 │──→│ 模型监控 │      │
│  │  Data   │   │ Training│   │  Eval   │   │ Serving │   │Monitor │      │
│  └────┬────┘   └────┬────┘   └────┬────┘   └────┬────┘   └────┬────┘      │
│       │             │             │             │             │            │
│       ▼             ▼             ▼             ▼             ▼            │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                        资源需求特征                                  │  │
│  ├──────────┬──────────┬──────────┬──────────┬──────────────────────────┤  │
│  │ 高I/O    │ 大量GPU  │ 中等GPU  │ 稳定GPU  │ 低资源(监控)             │  │
│  │ 高存储   │ 高带宽   │ 批量处理 │ 低延迟   │ 高频采样                 │  │
│  │ 批处理   │ 长时间   │ 短时间   │ 持续运行 │ 长期存储                 │  │
│  └──────────┴──────────┴──────────┴──────────┴──────────────────────────┘  │
│       │             │             │             │             │            │
│       ▼             ▼             ▼             ▼             ▼            │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                        Kubernetes资源                                │  │
│  ├──────────┬──────────┬──────────┬──────────┬──────────────────────────┤  │
│  │ Spark    │PyTorchJob│   Job    │Deployment│ Prometheus               │  │
│  │ Ray Data │ TFJob    │  CronJob │ KServe   │ Grafana                  │  │
│  │ Argo     │ MPIJob   │          │ Triton   │ MLflow                   │  │
│  └──────────┴──────────┴──────────┴──────────┴──────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 工作负载类型对比

| 工作负载类型 | CPU需求 | GPU需求 | 内存需求 | 存储需求 | 网络需求 | 运行时长 | 容错要求 |
|-------------|---------|---------|---------|---------|---------|---------|---------|
| **数据预处理** | 高 | 低/无 | 高 | 非常高 | 中 | 小时级 | 中 |
| **特征工程** | 高 | 中 | 高 | 高 | 中 | 小时级 | 中 |
| **模型预训练** | 中 | 非常高 | 高 | 高 | 非常高 | 天/周级 | 高 |
| **模型微调** | 中 | 高 | 高 | 中 | 高 | 小时/天级 | 高 |
| **超参搜索** | 高 | 高 | 中 | 中 | 中 | 天级 | 中 |
| **模型评估** | 中 | 中 | 中 | 中 | 低 | 分钟/小时 | 低 |
| **在线推理** | 中 | 中/高 | 高 | 低 | 高 | 持续 | 非常高 |
| **批量推理** | 中 | 高 | 高 | 高 | 中 | 小时级 | 中 |

---

## 二、分布式训练架构 (Distributed Training)

### 2.1 分布式训练范式

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      分布式训练并行策略                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  数据并行 (Data Parallel)                                                   │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  Worker 0      Worker 1      Worker 2      Worker 3                 │   │
│  │  ┌───────┐    ┌───────┐    ┌───────┐    ┌───────┐                  │   │
│  │  │Model  │    │Model  │    │Model  │    │Model  │                  │   │
│  │  │Copy   │    │Copy   │    │Copy   │    │Copy   │                  │   │
│  │  └───┬───┘    └───┬───┘    └───┬───┘    └───┬───┘                  │   │
│  │      │            │            │            │                       │   │
│  │  ┌───┴───┐    ┌───┴───┐    ┌───┴───┐    ┌───┴───┐                  │   │
│  │  │Data   │    │Data   │    │Data   │    │Data   │                  │   │
│  │  │Shard 0│    │Shard 1│    │Shard 2│    │Shard 3│                  │   │
│  │  └───────┘    └───────┘    └───────┘    └───────┘                  │   │
│  │                    ↓ All-Reduce 梯度同步 ↓                          │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  模型并行 (Model Parallel) - 张量并行                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                         Model Layer                                  │   │
│  │  ┌───────────┬───────────┬───────────┬───────────┐                  │   │
│  │  │ Tensor    │ Tensor    │ Tensor    │ Tensor    │                  │   │
│  │  │ Shard 0   │ Shard 1   │ Shard 2   │ Shard 3   │                  │   │
│  │  │ (GPU 0)   │ (GPU 1)   │ (GPU 2)   │ (GPU 3)   │                  │   │
│  │  └───────────┴───────────┴───────────┴───────────┘                  │   │
│  │                    ↔ All-Gather/Reduce-Scatter ↔                    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  流水线并行 (Pipeline Parallel)                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  Stage 0        Stage 1        Stage 2        Stage 3               │   │
│  │  (Layers 0-7)   (Layers 8-15)  (Layers 16-23) (Layers 24-31)       │   │
│  │  ┌───────┐     ┌───────┐      ┌───────┐      ┌───────┐             │   │
│  │  │ GPU 0 │ ──→ │ GPU 1 │ ──→  │ GPU 2 │ ──→  │ GPU 3 │             │   │
│  │  └───────┘     └───────┘      └───────┘      └───────┘             │   │
│  │     ↑                                            │                  │   │
│  │     └────────────── Backward ────────────────────┘                  │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Kubeflow Training Operator

```yaml
# PyTorchJob 生产级配置
apiVersion: "kubeflow.org/v1"
kind: PyTorchJob
metadata:
  name: llama-70b-finetune
  namespace: ml-training
  labels:
    app: llm-training
    model: llama-70b
    stage: finetune
spec:
  # 弹性训练配置
  elasticPolicy:
    rdzvBackend: c10d
    minReplicas: 4
    maxReplicas: 8
    maxRestarts: 100
    metrics:
    - type: Resource
      resource:
        name: nvidia.com/gpu
        target:
          type: Utilization
          averageUtilization: 80
          
  # 任务完成策略
  runPolicy:
    cleanPodPolicy: None
    backoffLimit: 3
    activeDeadlineSeconds: 604800   # 7天超时
    
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      restartPolicy: OnFailure
      template:
        metadata:
          labels:
            role: master
          annotations:
            prometheus.io/scrape: "true"
            prometheus.io/port: "8080"
        spec:
          priorityClassName: high-priority
          
          # 调度约束
          nodeSelector:
            nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
          tolerations:
          - key: "nvidia.com/gpu"
            operator: "Exists"
            effect: "NoSchedule"
            
          containers:
          - name: pytorch
            image: nvcr.io/nvidia/pytorch:24.01-py3
            imagePullPolicy: Always
            
            command: ["torchrun"]
            args:
            - "--nproc_per_node=8"
            - "--nnodes=$(WORLD_SIZE)"
            - "--node_rank=$(RANK)"
            - "--master_addr=$(MASTER_ADDR)"
            - "--master_port=$(MASTER_PORT)"
            - "train.py"
            - "--model_name=meta-llama/Llama-2-70b-hf"
            - "--dataset=/data/finetune_data"
            - "--output_dir=/output"
            - "--per_device_train_batch_size=1"
            - "--gradient_accumulation_steps=8"
            - "--learning_rate=2e-5"
            - "--num_train_epochs=3"
            - "--fp16"
            - "--deepspeed=/config/ds_config.json"
            
            resources:
              requests:
                cpu: "64"
                memory: "512Gi"
                nvidia.com/gpu: "8"
              limits:
                cpu: "64"
                memory: "512Gi"
                nvidia.com/gpu: "8"
                
            env:
            # PyTorch分布式
            - name: WORLD_SIZE
              value: "4"
            - name: NCCL_DEBUG
              value: "INFO"
            - name: NCCL_IB_DISABLE
              value: "0"
            - name: NCCL_NET_GDR_LEVEL
              value: "5"
            # CUDA优化
            - name: CUDA_DEVICE_MAX_CONNECTIONS
              value: "1"
            - name: PYTORCH_CUDA_ALLOC_CONF
              value: "max_split_size_mb:512"
              
            volumeMounts:
            - name: shm
              mountPath: /dev/shm
            - name: data
              mountPath: /data
            - name: output
              mountPath: /output
            - name: config
              mountPath: /config
              
            # 健康检查
            livenessProbe:
              exec:
                command:
                - python
                - -c
                - "import torch; assert torch.cuda.is_available()"
              initialDelaySeconds: 60
              periodSeconds: 60
              
          volumes:
          - name: shm
            emptyDir:
              medium: Memory
              sizeLimit: "128Gi"
          - name: data
            persistentVolumeClaim:
              claimName: training-data-pvc
          - name: output
            persistentVolumeClaim:
              claimName: model-output-pvc
          - name: config
            configMap:
              name: deepspeed-config
              
    Worker:
      replicas: 3
      restartPolicy: OnFailure
      template:
        spec:
          # 与Master相同配置...
          nodeSelector:
            nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
          tolerations:
          - key: "nvidia.com/gpu"
            operator: "Exists"
            effect: "NoSchedule"
          containers:
          - name: pytorch
            image: nvcr.io/nvidia/pytorch:24.01-py3
            resources:
              limits:
                nvidia.com/gpu: "8"
            # 其他配置同Master...
```

### 2.3 DeepSpeed配置

```yaml
# DeepSpeed ZeRO-3配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: deepspeed-config
  namespace: ml-training
data:
  ds_config.json: |
    {
      "train_batch_size": "auto",
      "train_micro_batch_size_per_gpu": "auto",
      "gradient_accumulation_steps": "auto",
      
      "zero_optimization": {
        "stage": 3,
        "offload_optimizer": {
          "device": "cpu",
          "pin_memory": true
        },
        "offload_param": {
          "device": "cpu",
          "pin_memory": true
        },
        "overlap_comm": true,
        "contiguous_gradients": true,
        "sub_group_size": 1e9,
        "reduce_bucket_size": "auto",
        "stage3_prefetch_bucket_size": "auto",
        "stage3_param_persistence_threshold": "auto",
        "stage3_max_live_parameters": 1e9,
        "stage3_max_reuse_distance": 1e9,
        "stage3_gather_16bit_weights_on_model_save": true
      },
      
      "fp16": {
        "enabled": true,
        "loss_scale": 0,
        "loss_scale_window": 1000,
        "initial_scale_power": 16,
        "hysteresis": 2,
        "min_loss_scale": 1
      },
      
      "bf16": {
        "enabled": false
      },
      
      "gradient_clipping": 1.0,
      
      "optimizer": {
        "type": "AdamW",
        "params": {
          "lr": "auto",
          "betas": [0.9, 0.999],
          "eps": 1e-8,
          "weight_decay": "auto"
        }
      },
      
      "scheduler": {
        "type": "WarmupDecayLR",
        "params": {
          "warmup_min_lr": 0,
          "warmup_max_lr": "auto",
          "warmup_num_steps": "auto",
          "total_num_steps": "auto"
        }
      },
      
      "activation_checkpointing": {
        "partition_activations": true,
        "cpu_checkpointing": true,
        "contiguous_memory_optimization": true,
        "number_checkpoints": null,
        "synchronize_checkpoint_boundary": false,
        "profile": false
      },
      
      "wall_clock_breakdown": false,
      "tensorboard": {
        "enabled": true,
        "output_path": "/output/tensorboard",
        "job_name": "llama-70b-finetune"
      }
    }
```

---

## 三、模型推理服务 (Model Serving)

### 3.1 推理服务架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      模型推理服务架构                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        流量入口层                                    │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │   │
│  │  │   Ingress    │  │ Gateway API  │  │   Istio      │              │   │
│  │  │   (NGINX)    │  │              │  │  VirtualSvc  │              │   │
│  │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘              │   │
│  │         └─────────────────┼─────────────────┘                       │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        推理网关层                                    │   │
│  │  ┌──────────────────────────────────────────────────────────────┐   │   │
│  │  │  KServe Predictor / Triton Inference Server                  │   │   │
│  │  │  ├── 请求路由                                                 │   │   │
│  │  │  ├── 负载均衡                                                 │   │   │
│  │  │  ├── 流量镜像                                                 │   │   │
│  │  │  ├── 金丝雀发布                                               │   │   │
│  │  │  └── A/B测试                                                  │   │   │
│  │  └──────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                              │                                              │
│          ┌───────────────────┼───────────────────┐                         │
│          ▼                   ▼                   ▼                         │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                    │
│  │  Predictor  │    │  Predictor  │    │  Predictor  │                    │
│  │  (vLLM)     │    │  (TGI)      │    │  (Triton)   │                    │
│  │  ┌───────┐  │    │  ┌───────┐  │    │  ┌───────┐  │                    │
│  │  │LLaMA  │  │    │  │Falcon │  │    │  │BERT   │  │                    │
│  │  │70B    │  │    │  │40B    │  │    │  │Base   │  │                    │
│  │  └───────┘  │    │  └───────┘  │    │  └───────┘  │                    │
│  │  4x A100    │    │  2x A100    │    │  1x T4      │                    │
│  └─────────────┘    └─────────────┘    └─────────────┘                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 KServe部署配置

```yaml
# KServe InferenceService (生产级配置)
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama-70b-chat
  namespace: ml-serving
  annotations:
    # 自动扩缩配置
    autoscaling.knative.dev/class: "kpa.autoscaling.knative.dev"
    autoscaling.knative.dev/metric: "concurrency"
    autoscaling.knative.dev/target: "10"
    autoscaling.knative.dev/minScale: "2"
    autoscaling.knative.dev/maxScale: "10"
    # GPU调度
    serving.kserve.io/enable-prometheus-scraping: "true"
spec:
  predictor:
    # 金丝雀发布
    canaryTrafficPercent: 10
    
    # 模型配置
    model:
      modelFormat:
        name: pytorch
      runtime: kserve-vllm
      storageUri: "s3://models/llama-2-70b-chat"
      
    # 容器配置
    containers:
    - name: kserve-container
      image: vllm/vllm-openai:latest
      args:
      - "--model=/mnt/models"
      - "--tensor-parallel-size=4"
      - "--max-model-len=4096"
      - "--gpu-memory-utilization=0.9"
      - "--quantization=awq"
      
      resources:
        requests:
          cpu: "16"
          memory: "64Gi"
          nvidia.com/gpu: "4"
        limits:
          cpu: "32"
          memory: "128Gi"
          nvidia.com/gpu: "4"
          
      env:
      - name: CUDA_VISIBLE_DEVICES
        value: "0,1,2,3"
      - name: VLLM_WORKER_MULTIPROC_METHOD
        value: "spawn"
        
      # 健康检查
      readinessProbe:
        httpGet:
          path: /health
          port: 8080
        initialDelaySeconds: 120
        periodSeconds: 10
        failureThreshold: 3
        
      livenessProbe:
        httpGet:
          path: /health
          port: 8080
        initialDelaySeconds: 180
        periodSeconds: 30
        
    # 节点选择
    nodeSelector:
      nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
    tolerations:
    - key: "nvidia.com/gpu"
      operator: "Exists"
      effect: "NoSchedule"
      
  # Transformer (可选的预处理)
  transformer:
    containers:
    - name: transformer
      image: ml-platform/llm-transformer:latest
      resources:
        requests:
          cpu: "2"
          memory: "4Gi"
          
---
# HPA配置 (GPU利用率扩缩)
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: llama-70b-chat-hpa
  namespace: ml-serving
spec:
  scaleTargetRef:
    apiVersion: serving.kserve.io/v1beta1
    kind: InferenceService
    name: llama-70b-chat
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: External
    external:
      metric:
        name: "prometheus-adapter/gpu_utilization"
        selector:
          matchLabels:
            service: llama-70b-chat
      target:
        type: AverageValue
        averageValue: "80"
```

### 3.3 vLLM高性能推理

```yaml
# vLLM Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-llama-70b
  namespace: ml-serving
spec:
  replicas: 2
  selector:
    matchLabels:
      app: vllm-llama-70b
  template:
    metadata:
      labels:
        app: vllm-llama-70b
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "8000"
    spec:
      containers:
      - name: vllm
        image: vllm/vllm-openai:latest
        
        args:
        - "--model=/models/llama-2-70b-chat-hf"
        - "--tensor-parallel-size=4"
        - "--pipeline-parallel-size=1"
        - "--max-model-len=8192"
        - "--max-num-seqs=256"
        - "--gpu-memory-utilization=0.92"
        - "--quantization=awq"
        - "--dtype=float16"
        # 性能优化
        - "--enable-prefix-caching"
        - "--use-v2-block-manager"
        - "--enable-chunked-prefill"
        # API配置
        - "--host=0.0.0.0"
        - "--port=8000"
        - "--api-key=$(API_KEY)"
        
        ports:
        - containerPort: 8000
          name: http
          
        resources:
          requests:
            cpu: "32"
            memory: "128Gi"
            nvidia.com/gpu: "4"
          limits:
            cpu: "64"
            memory: "256Gi"
            nvidia.com/gpu: "4"
            
        env:
        - name: API_KEY
          valueFrom:
            secretKeyRef:
              name: vllm-secrets
              key: api-key
        - name: CUDA_VISIBLE_DEVICES
          value: "0,1,2,3"
        - name: NCCL_DEBUG
          value: "WARN"
        - name: VLLM_ATTENTION_BACKEND
          value: "FLASH_ATTN"
          
        volumeMounts:
        - name: models
          mountPath: /models
        - name: shm
          mountPath: /dev/shm
          
        readinessProbe:
          httpGet:
            path: /health
            port: 8000
          initialDelaySeconds: 180
          periodSeconds: 10
          
        livenessProbe:
          httpGet:
            path: /health
            port: 8000
          initialDelaySeconds: 300
          periodSeconds: 30
          timeoutSeconds: 10
          
      volumes:
      - name: models
        persistentVolumeClaim:
          claimName: llm-models-pvc
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: "32Gi"
          
      nodeSelector:
        nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
      tolerations:
      - key: "nvidia.com/gpu"
        operator: "Exists"
        effect: "NoSchedule"
```

---

## 四、数据处理管道 (Data Pipeline)

### 4.1 Spark on Kubernetes

```yaml
# SparkApplication for Data Processing
apiVersion: sparkoperator.k8s.io/v1beta2
kind: SparkApplication
metadata:
  name: data-preprocessing
  namespace: ml-data
spec:
  type: Python
  pythonVersion: "3"
  mode: cluster
  image: "spark-py:3.5.0"
  imagePullPolicy: Always
  mainApplicationFile: "s3a://scripts/preprocess.py"
  
  arguments:
  - "--input=s3a://raw-data/dataset"
  - "--output=s3a://processed-data/dataset"
  - "--partitions=1000"
  
  sparkVersion: "3.5.0"
  
  # Spark配置
  sparkConf:
    "spark.kubernetes.allocation.batch.size": "10"
    "spark.sql.shuffle.partitions": "1000"
    "spark.sql.adaptive.enabled": "true"
    "spark.sql.adaptive.coalescePartitions.enabled": "true"
    "spark.serializer": "org.apache.spark.serializer.KryoSerializer"
    # S3配置
    "spark.hadoop.fs.s3a.impl": "org.apache.hadoop.fs.s3a.S3AFileSystem"
    "spark.hadoop.fs.s3a.fast.upload": "true"
    "spark.hadoop.fs.s3a.fast.upload.buffer": "bytebuffer"
    
  # Driver配置
  driver:
    cores: 4
    coreLimit: "4"
    memory: "16g"
    labels:
      version: "3.5.0"
    serviceAccount: spark-sa
    
  # Executor配置
  executor:
    cores: 4
    instances: 50
    memory: "32g"
    labels:
      version: "3.5.0"
    
  # 动态分配
  dynamicAllocation:
    enabled: true
    initialExecutors: 10
    minExecutors: 5
    maxExecutors: 100
    
  restartPolicy:
    type: OnFailure
    onFailureRetries: 3
    onFailureRetryInterval: 10
    onSubmissionFailureRetries: 5
    onSubmissionFailureRetryInterval: 20
```

### 4.2 Ray Data处理

```yaml
# RayJob for Data Processing
apiVersion: ray.io/v1
kind: RayJob
metadata:
  name: data-pipeline
  namespace: ml-data
spec:
  entrypoint: python /app/data_pipeline.py
  
  runtimeEnvYAML: |
    pip:
      - pandas==2.0.0
      - pyarrow==14.0.0
      - datasets==2.16.0
    env_vars:
      DATA_PATH: "s3://raw-data"
      OUTPUT_PATH: "s3://processed-data"
      
  rayClusterSpec:
    rayVersion: '2.9.0'
    
    headGroupSpec:
      rayStartParams:
        dashboard-host: '0.0.0.0'
      template:
        spec:
          containers:
          - name: ray-head
            image: rayproject/ray:2.9.0-py310
            resources:
              limits:
                cpu: "8"
                memory: "32Gi"
              requests:
                cpu: "4"
                memory: "16Gi"
            volumeMounts:
            - name: app-code
              mountPath: /app
          volumes:
          - name: app-code
            configMap:
              name: data-pipeline-code
              
    workerGroupSpecs:
    - replicas: 10
      minReplicas: 5
      maxReplicas: 50
      groupName: data-workers
      rayStartParams: {}
      template:
        spec:
          containers:
          - name: ray-worker
            image: rayproject/ray:2.9.0-py310
            resources:
              limits:
                cpu: "16"
                memory: "64Gi"
              requests:
                cpu: "8"
                memory: "32Gi"
                
  submitterPodTemplate:
    spec:
      restartPolicy: Never
      containers:
      - name: submitter
        image: rayproject/ray:2.9.0-py310
```

---

## 五、实验管理与MLOps (Experiment Management)

### 5.1 MLflow部署

```yaml
# MLflow Tracking Server
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mlflow-tracking
  namespace: mlops
spec:
  replicas: 2
  selector:
    matchLabels:
      app: mlflow-tracking
  template:
    metadata:
      labels:
        app: mlflow-tracking
    spec:
      containers:
      - name: mlflow
        image: ghcr.io/mlflow/mlflow:v2.10.0
        
        command: ["mlflow", "server"]
        args:
        - "--host=0.0.0.0"
        - "--port=5000"
        - "--backend-store-uri=postgresql://$(DB_USER):$(DB_PASSWORD)@postgres:5432/mlflow"
        - "--default-artifact-root=s3://mlflow-artifacts"
        - "--serve-artifacts"
        
        ports:
        - containerPort: 5000
          name: http
          
        resources:
          requests:
            cpu: "2"
            memory: "4Gi"
          limits:
            cpu: "4"
            memory: "8Gi"
            
        env:
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: mlflow-secrets
              key: db-user
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: mlflow-secrets
              key: db-password
        - name: AWS_ACCESS_KEY_ID
          valueFrom:
            secretKeyRef:
              name: mlflow-secrets
              key: aws-access-key
        - name: AWS_SECRET_ACCESS_KEY
          valueFrom:
            secretKeyRef:
              name: mlflow-secrets
              key: aws-secret-key
              
        livenessProbe:
          httpGet:
            path: /health
            port: 5000
          initialDelaySeconds: 30
          periodSeconds: 10
          
        readinessProbe:
          httpGet:
            path: /health
            port: 5000
          initialDelaySeconds: 10
          periodSeconds: 5
```

### 5.2 Kubeflow Pipelines

```yaml
# Kubeflow Pipeline Component
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  generateName: ml-training-pipeline-
  namespace: kubeflow
spec:
  entrypoint: ml-pipeline
  
  templates:
  - name: ml-pipeline
    dag:
      tasks:
      # 数据预处理
      - name: data-preprocessing
        template: preprocess
        
      # 特征工程
      - name: feature-engineering
        template: feature-eng
        dependencies: [data-preprocessing]
        
      # 模型训练
      - name: model-training
        template: train
        dependencies: [feature-engineering]
        
      # 模型评估
      - name: model-evaluation
        template: evaluate
        dependencies: [model-training]
        
      # 模型注册
      - name: model-registration
        template: register
        dependencies: [model-evaluation]
        when: "{{tasks.model-evaluation.outputs.result}} > 0.85"
        
  - name: preprocess
    container:
      image: ml-platform/preprocessor:latest
      command: [python, preprocess.py]
      resources:
        requests:
          cpu: "8"
          memory: "32Gi"
          
  - name: feature-eng
    container:
      image: ml-platform/feature-eng:latest
      command: [python, feature_engineering.py]
      resources:
        requests:
          cpu: "16"
          memory: "64Gi"
          
  - name: train
    container:
      image: ml-platform/trainer:latest
      command: [python, train.py]
      resources:
        requests:
          cpu: "32"
          memory: "128Gi"
          nvidia.com/gpu: "8"
    nodeSelector:
      nvidia.com/gpu.present: "true"
      
  - name: evaluate
    container:
      image: ml-platform/evaluator:latest
      command: [python, evaluate.py]
      resources:
        requests:
          cpu: "4"
          memory: "16Gi"
          nvidia.com/gpu: "1"
          
  - name: register
    container:
      image: ml-platform/model-registry:latest
      command: [python, register_model.py]
```

---

## 六、监控与告警 (Monitoring & Alerting)

### 6.1 AI工作负载监控指标

| 指标类别 | 指标名称 | 说明 | 告警阈值 |
|---------|---------|------|---------|
| **训练进度** | epoch_progress | 当前Epoch进度 | 停滞>1h |
| **训练进度** | training_loss | 训练损失 | 异常波动 |
| **训练进度** | validation_loss | 验证损失 | 持续上升 |
| **资源效率** | gpu_utilization | GPU利用率 | < 50% |
| **资源效率** | gpu_memory_used | 显存使用 | > 95% |
| **资源效率** | samples_per_second | 训练吞吐量 | 下降>20% |
| **推理性能** | inference_latency_p99 | P99推理延迟 | > SLA |
| **推理性能** | tokens_per_second | Token生成速度 | < 基准 |
| **推理性能** | queue_depth | 请求队列深度 | > 100 |
| **系统健康** | pod_restart_count | Pod重启次数 | > 3 |
| **系统健康** | oom_kill_count | OOM次数 | > 0 |
| **系统健康** | nccl_errors | NCCL通信错误 | > 0 |

### 6.2 Prometheus告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ai-workload-alerts
  namespace: monitoring
spec:
  groups:
  - name: ai-training-alerts
    rules:
    # 训练任务卡住
    - alert: TrainingStalled
      expr: |
        rate(training_step_total[30m]) == 0
        and on(job_name) training_job_running == 1
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "训练任务停滞"
        description: "任务 {{ $labels.job_name }} 30分钟内无进展"
        
    # 训练Loss异常
    - alert: TrainingLossAnomaly
      expr: |
        (training_loss - training_loss offset 1h) / training_loss offset 1h > 0.5
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "训练Loss异常上升"
        description: "任务 {{ $labels.job_name }} Loss上升超过50%"
        
    # GPU利用率低
    - alert: TrainingGPUUnderutilized
      expr: |
        avg by (job_name) (DCGM_FI_DEV_GPU_UTIL{job_type="training"}) < 50
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "训练GPU利用率低"
        description: "任务 {{ $labels.job_name }} GPU利用率 {{ $value }}%"
        
    # Checkpoint保存失败
    - alert: CheckpointSaveFailed
      expr: |
        increase(checkpoint_save_errors_total[1h]) > 0
      for: 0m
      labels:
        severity: critical
      annotations:
        summary: "Checkpoint保存失败"
        description: "任务 {{ $labels.job_name }} Checkpoint保存异常"
        
  - name: ai-inference-alerts
    rules:
    # 推理延迟高
    - alert: InferenceLatencyHigh
      expr: |
        histogram_quantile(0.99, rate(inference_latency_seconds_bucket[5m])) > 2
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "推理P99延迟超过2秒"
        description: "服务 {{ $labels.service }} P99延迟: {{ $value }}s"
        
    # 推理队列积压
    - alert: InferenceQueueBacklog
      expr: |
        inference_queue_depth > 100
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "推理请求队列积压"
        description: "服务 {{ $labels.service }} 队列深度: {{ $value }}"
        
    # 推理错误率高
    - alert: InferenceErrorRateHigh
      expr: |
        rate(inference_errors_total[5m]) / rate(inference_requests_total[5m]) > 0.01
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "推理错误率超过1%"
        description: "服务 {{ $labels.service }} 错误率: {{ $value | humanizePercentage }}"
        
    # GPU OOM
    - alert: InferenceGPUOOM
      expr: |
        increase(inference_oom_errors_total[5m]) > 0
      for: 0m
      labels:
        severity: critical
      annotations:
        summary: "推理服务GPU OOM"
        description: "服务 {{ $labels.service }} 发生GPU显存不足"
```

---

## 七、存储与网络加速 (Storage & Network Acceleration)

### 7.1 高性能存储配置

```yaml
# JuiceFS for AI Data
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: ai-training-data
  namespace: ml-training
spec:
  accessModes:
  - ReadWriteMany
  resources:
    requests:
      storage: 10Ti
  storageClassName: juicefs-sc
  
---
# JuiceFS StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: juicefs-sc
provisioner: csi.juicefs.com
parameters:
  csi.storage.k8s.io/provisioner-secret-name: juicefs-secret
  csi.storage.k8s.io/provisioner-secret-namespace: kube-system
  csi.storage.k8s.io/node-publish-secret-name: juicefs-secret
  csi.storage.k8s.io/node-publish-secret-namespace: kube-system
  
---
# Fluid数据集预热
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: training-dataset
  namespace: ml-training
spec:
  mounts:
  - mountPoint: "s3://ml-datasets/imagenet"
    name: imagenet
    options:
      region: us-east-1
      
---
apiVersion: data.fluid.io/v1alpha1
kind: AlluxioRuntime
metadata:
  name: training-dataset
  namespace: ml-training
spec:
  replicas: 10
  tieredstore:
    levels:
    - mediumtype: MEM
      path: /dev/shm
      quota: 64Gi
      high: "0.95"
      low: "0.7"
    - mediumtype: SSD
      path: /mnt/ssd
      quota: 500Gi
      high: "0.95"
      low: "0.7"
  master:
    replicas: 3
  worker:
    resources:
      requests:
        cpu: "8"
        memory: "32Gi"
      limits:
        cpu: "16"
        memory: "64Gi"
```

### 7.2 RDMA网络配置

```yaml
# Multus RDMA网络配置
apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: rdma-net
  namespace: ml-training
spec:
  config: |
    {
      "cniVersion": "0.3.1",
      "type": "host-device",
      "device": "mlx5_0",
      "ipam": {
        "type": "whereabouts",
        "range": "192.168.100.0/24"
      }
    }
    
---
# 使用RDMA网络的训练Pod
apiVersion: v1
kind: Pod
metadata:
  name: rdma-training-pod
  namespace: ml-training
  annotations:
    k8s.v1.cni.cncf.io/networks: rdma-net
spec:
  containers:
  - name: trainer
    image: nvcr.io/nvidia/pytorch:24.01-py3
    resources:
      limits:
        nvidia.com/gpu: 8
        rdma/rdma_shared_device_a: 1
    env:
    - name: NCCL_IB_DISABLE
      value: "0"
    - name: NCCL_NET_GDR_LEVEL
      value: "5"
    - name: NCCL_IB_HCA
      value: "mlx5"
    - name: NCCL_DEBUG
      value: "INFO"
```

---

## 八、快速参考 (Quick Reference)

### 8.1 常用命令

```bash
# ========== 训练任务管理 ==========

# 查看PyTorchJob
kubectl get pytorchjobs -n ml-training
kubectl describe pytorchjob <name> -n ml-training

# 查看训练日志
kubectl logs -n ml-training <master-pod> -f

# 查看训练指标
kubectl exec -it <pod> -- nvidia-smi dmon -s pucvmet

# ========== 推理服务管理 ==========

# 查看InferenceService
kubectl get inferenceservices -n ml-serving
kubectl describe inferenceservice <name> -n ml-serving

# 测试推理服务
curl -X POST http://<service>/v1/completions \
  -H "Content-Type: application/json" \
  -d '{"prompt": "Hello", "max_tokens": 100}'

# ========== 队列管理 ==========

# Kueue队列状态
kubectl get clusterqueues
kubectl get localqueues -n ml-training
kubectl get workloads -n ml-training

# Volcano队列状态
kubectl get queues -n volcano-system
kubectl get podgroups -n ml-training
```

### 8.2 资源需求速查

| 模型规模 | GPU类型 | GPU数量 | 显存需求 | 训练时长 |
|---------|--------|--------|---------|---------|
| 1B | A10G | 1-2 | 24GB | 1-2天 |
| 7B | A100 40GB | 2-4 | 80-160GB | 3-7天 |
| 13B | A100 80GB | 4-8 | 160-320GB | 1-2周 |
| 70B | A100 80GB | 16-32 | 640-1280GB | 2-4周 |
| 175B | H100 | 64-128 | 2.5-5TB | 1-2月 |

---

**AI工作负载原则**: 资源预估充分 → 监控覆盖完整 → 检查点策略健全 → 弹性伸缩配置

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
