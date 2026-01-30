# 144 - LLM推理服务部署

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 高级 | **参考**: [vLLM](https://docs.vllm.ai/) | [TGI](https://huggingface.co/docs/text-generation-inference) | [TensorRT-LLM](https://nvidia.github.io/TensorRT-LLM/)

## 一、推理服务架构概览

### 1.1 生产级LLM推理架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                           LLM Inference Service Architecture                         │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────────────────────────────────┐  │
│  │   Client    │    │   Gateway   │    │           Kubernetes Cluster            │  │
│  │  Requests   │───▶│  (Istio/    │───▶│                                         │  │
│  │             │    │  Kong/NGINX)│    │  ┌─────────────────────────────────────┐│  │
│  └─────────────┘    └─────────────┘    │  │       Load Balancer (Service)       ││  │
│                           │            │  └───────────────────┬─────────────────┘│  │
│                           │            │                      │                   │  │
│                           ▼            │  ┌───────────────────▼─────────────────┐│  │
│  ┌─────────────────────────────────┐   │  │          Inference Router           ││  │
│  │         Rate Limiter            │   │  │    (Request Routing & Batching)     ││  │
│  │  ┌───────────────────────────┐  │   │  └───────────────────┬─────────────────┘│  │
│  │  │ Per-User: 100 req/min     │  │   │                      │                   │  │
│  │  │ Per-API-Key: 1000 req/min │  │   │  ┌──────────┬────────┴─────┬──────────┐│  │
│  │  │ Global: 10000 req/min     │  │   │  │          │              │          ││  │
│  │  └───────────────────────────┘  │   │  ▼          ▼              ▼          ▼│  │
│  └─────────────────────────────────┘   │┌─────┐  ┌─────┐       ┌─────┐  ┌─────┐│  │
│                                        ││vLLM │  │vLLM │  ...  │vLLM │  │vLLM ││  │
│  ┌─────────────────────────────────┐   ││Pod 1│  │Pod 2│       │Pod N│  │Pod M││  │
│  │      Request Queue              │   │└──┬──┘  └──┬──┘       └──┬──┘  └──┬──┘│  │
│  │  ┌─────────────────────────────┐│   │   │        │             │        │   │  │
│  │  │ Priority: Critical > Normal ││   │   └────────┼─────────────┼────────┘   │  │
│  │  │ Queue Depth: 50-100 requests││   │            │             │            │  │
│  │  │ Timeout: 30-60 seconds      ││   │  ┌────────▼─────────────▼────────┐   │  │
│  │  └─────────────────────────────┘│   │  │         GPU Nodes              │   │  │
│  └─────────────────────────────────┘   │  │  A100/H100/L40S/A10G           │   │  │
│                                        │  └───────────────────────────────────┘   │  │
│  ┌─────────────────────────────────┐   │                                         │  │
│  │        Model Registry           │   │  ┌─────────────────────────────────────┐│  │
│  │  ┌───────────────────────────┐  │   │  │         Model Storage              ││  │
│  │  │ S3/GCS/Azure Blob         │  │   │  │   PVC (NVMe SSD) + S3 Cache        ││  │
│  │  │ HuggingFace Hub           │  │◀─▶│  │   JuiceFS / Alluxio / FSx          ││  │
│  │  │ MLflow Registry           │  │   │  └─────────────────────────────────────┘│  │
│  │  └───────────────────────────┘  │   │                                         │  │
│  └─────────────────────────────────┘   └─────────────────────────────────────────┘  │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 推理引擎全面对比

| 推理引擎 | 吞吐量 | 延迟 | 显存效率 | 量化支持 | 分布式推理 | 流式输出 | 适用GPU | 生产就绪 |
|---------|--------|------|----------|----------|-----------|----------|---------|---------|
| **vLLM** | ★★★★★ | 中 | 95% | AWQ/GPTQ/FP8 | TP | ✓ | A100/H100/A10G | ✓ |
| **TGI** | ★★★★☆ | 低 | 90% | GPTQ/AWQ/EETQ | TP | ✓ | 全系列 | ✓ |
| **TensorRT-LLM** | ★★★★★ | 极低 | 98% | INT8/FP8/INT4 | TP+PP | ✓ | NVIDIA专用 | ✓ |
| **llama.cpp** | ★★☆☆☆ | 中 | 85% | GGUF Q2-Q8 | ✗ | ✓ | CPU/低端GPU | ✓ |
| **DeepSpeed-MII** | ★★★★☆ | 低 | 92% | ZeroQuant | TP | ✓ | A100/H100 | ✓ |
| **SGLang** | ★★★★★ | 极低 | 95% | AWQ/GPTQ | TP | ✓ | A100/H100 | ✓ |
| **LMDeploy** | ★★★★☆ | 低 | 93% | W4A16/W8A8 | TP | ✓ | 全系列 | ✓ |

### 1.3 关键技术特性对比

| 技术特性 | vLLM | TGI | TensorRT-LLM | SGLang |
|---------|------|-----|--------------|--------|
| **PagedAttention** | ✓ | ✓ | ✓ | ✓ |
| **Continuous Batching** | ✓ | ✓ | ✓ | ✓ |
| **Speculative Decoding** | ✓ | ✓ | ✓ | ✓ |
| **Flash Attention 2** | ✓ | ✓ | ✓ | ✓ |
| **Prefix Caching** | ✓ | ✗ | ✓ | ✓ |
| **RadixAttention** | ✗ | ✗ | ✗ | ✓ |
| **Multi-LoRA** | ✓ | ✓ | ✓ | ✓ |
| **Structured Output** | ✓ | ✓ | ✗ | ✓ |
| **Vision Models** | ✓ | ✓ | ✓ | ✓ |
| **OpenAI Compatible API** | ✓ | ✓ | ✓ | ✓ |

---

## 二、vLLM生产部署

### 2.1 完整Deployment配置

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: llm-inference
  labels:
    istio-injection: enabled
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: vllm-config
  namespace: llm-inference
data:
  # vLLM服务器配置
  VLLM_HOST: "0.0.0.0"
  VLLM_PORT: "8000"
  
  # 性能优化
  VLLM_GPU_MEMORY_UTILIZATION: "0.90"
  VLLM_MAX_MODEL_LEN: "8192"
  VLLM_MAX_NUM_SEQS: "256"
  VLLM_MAX_NUM_BATCHED_TOKENS: "32768"
  
  # 并行配置
  VLLM_TENSOR_PARALLEL_SIZE: "1"
  VLLM_PIPELINE_PARALLEL_SIZE: "1"
  
  # 量化配置
  VLLM_QUANTIZATION: "awq"
  VLLM_DTYPE: "float16"
  
  # 缓存配置
  VLLM_ENABLE_PREFIX_CACHING: "true"
  VLLM_BLOCK_SIZE: "16"
  
  # 日志配置
  VLLM_LOG_LEVEL: "info"
---
apiVersion: v1
kind: Secret
metadata:
  name: vllm-secrets
  namespace: llm-inference
type: Opaque
stringData:
  # HuggingFace Token (私有模型访问)
  HF_TOKEN: "hf_xxxxxxxxxxxxxxxxxxxx"
  # API密钥 (可选)
  VLLM_API_KEY: "sk-xxxxxxxxxxxxxxxxxxxx"
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-llama3-70b
  namespace: llm-inference
  labels:
    app: vllm
    model: llama3-70b
    version: v1
spec:
  replicas: 4
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  selector:
    matchLabels:
      app: vllm
      model: llama3-70b
  template:
    metadata:
      labels:
        app: vllm
        model: llama3-70b
        version: v1
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "8000"
        prometheus.io/path: "/metrics"
    spec:
      # 调度约束
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: nvidia.com/gpu.product
                operator: In
                values:
                - NVIDIA-A100-SXM4-80GB
                - NVIDIA-H100-SXM5-80GB
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchLabels:
                  app: vllm
              topologyKey: kubernetes.io/hostname
      
      # 容忍GPU节点污点
      tolerations:
      - key: nvidia.com/gpu
        operator: Exists
        effect: NoSchedule
      - key: dedicated
        operator: Equal
        value: gpu-inference
        effect: NoSchedule
      
      # 初始化容器 - 模型预热
      initContainers:
      - name: model-downloader
        image: python:3.11-slim
        command:
        - /bin/bash
        - -c
        - |
          pip install huggingface_hub
          python -c "
          from huggingface_hub import snapshot_download
          import os
          snapshot_download(
              repo_id='meta-llama/Meta-Llama-3-70B-Instruct-AWQ',
              local_dir='/models/llama3-70b-awq',
              token=os.environ.get('HF_TOKEN'),
              local_dir_use_symlinks=False
          )
          "
        env:
        - name: HF_TOKEN
          valueFrom:
            secretKeyRef:
              name: vllm-secrets
              key: HF_TOKEN
        volumeMounts:
        - name: model-cache
          mountPath: /models
        resources:
          requests:
            cpu: "2"
            memory: "8Gi"
          limits:
            cpu: "4"
            memory: "16Gi"
      
      containers:
      - name: vllm
        image: vllm/vllm-openai:v0.4.2
        imagePullPolicy: Always
        
        args:
        - --model=/models/llama3-70b-awq
        - --served-model-name=llama3-70b
        - --host=0.0.0.0
        - --port=8000
        # 性能参数
        - --tensor-parallel-size=4
        - --dtype=float16
        - --quantization=awq
        - --gpu-memory-utilization=0.90
        - --max-model-len=8192
        - --max-num-seqs=256
        - --max-num-batched-tokens=32768
        # 优化参数
        - --enable-prefix-caching
        - --block-size=16
        - --swap-space=16
        - --disable-log-requests
        # API参数
        - --api-key=$(VLLM_API_KEY)
        - --chat-template=/etc/vllm/chat_template.jinja
        
        env:
        - name: VLLM_API_KEY
          valueFrom:
            secretKeyRef:
              name: vllm-secrets
              key: VLLM_API_KEY
        - name: CUDA_VISIBLE_DEVICES
          value: "0,1,2,3"
        - name: NCCL_DEBUG
          value: "WARN"
        - name: NCCL_IB_DISABLE
          value: "0"
        - name: NCCL_NET_GDR_LEVEL
          value: "5"
        
        ports:
        - containerPort: 8000
          name: http
          protocol: TCP
        
        # 资源配置 - 4x A100 80GB
        resources:
          requests:
            cpu: "16"
            memory: "64Gi"
            nvidia.com/gpu: "4"
          limits:
            cpu: "32"
            memory: "128Gi"
            nvidia.com/gpu: "4"
        
        # 健康检查
        startupProbe:
          httpGet:
            path: /health
            port: 8000
          initialDelaySeconds: 60
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 30  # 允许5分钟启动时间
        
        livenessProbe:
          httpGet:
            path: /health
            port: 8000
          initialDelaySeconds: 10
          periodSeconds: 30
          timeoutSeconds: 10
          failureThreshold: 3
        
        readinessProbe:
          httpGet:
            path: /health
            port: 8000
          initialDelaySeconds: 5
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 3
        
        volumeMounts:
        - name: model-cache
          mountPath: /models
          readOnly: true
        - name: chat-template
          mountPath: /etc/vllm
        - name: shm
          mountPath: /dev/shm
      
      volumes:
      - name: model-cache
        persistentVolumeClaim:
          claimName: model-storage-pvc
      - name: chat-template
        configMap:
          name: vllm-chat-template
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: 32Gi
      
      terminationGracePeriodSeconds: 120
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: vllm-chat-template
  namespace: llm-inference
data:
  chat_template.jinja: |
    {% for message in messages %}
    {% if message['role'] == 'system' %}
    <|begin_of_text|><|start_header_id|>system<|end_header_id|>
    {{ message['content'] }}<|eot_id|>
    {% elif message['role'] == 'user' %}
    <|start_header_id|>user<|end_header_id|>
    {{ message['content'] }}<|eot_id|>
    {% elif message['role'] == 'assistant' %}
    <|start_header_id|>assistant<|end_header_id|>
    {{ message['content'] }}<|eot_id|>
    {% endif %}
    {% endfor %}
    <|start_header_id|>assistant<|end_header_id|>
```

### 2.2 Service与Ingress配置

```yaml
apiVersion: v1
kind: Service
metadata:
  name: vllm-service
  namespace: llm-inference
  labels:
    app: vllm
  annotations:
    prometheus.io/scrape: "true"
    prometheus.io/port: "8000"
spec:
  type: ClusterIP
  sessionAffinity: None
  ports:
  - name: http
    port: 8000
    targetPort: 8000
    protocol: TCP
  selector:
    app: vllm
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: vllm-ingress
  namespace: llm-inference
  annotations:
    kubernetes.io/ingress.class: nginx
    nginx.ingress.kubernetes.io/proxy-body-size: "100m"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "600"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "600"
    nginx.ingress.kubernetes.io/proxy-connect-timeout: "60"
    # WebSocket支持 (流式输出)
    nginx.ingress.kubernetes.io/proxy-http-version: "1.1"
    nginx.ingress.kubernetes.io/upstream-hash-by: "$request_uri"
    # SSL配置
    cert-manager.io/cluster-issuer: letsencrypt-prod
spec:
  tls:
  - hosts:
    - llm-api.example.com
    secretName: llm-api-tls
  rules:
  - host: llm-api.example.com
    http:
      paths:
      - path: /v1
        pathType: Prefix
        backend:
          service:
            name: vllm-service
            port:
              number: 8000
---
# Istio VirtualService (可选)
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: vllm-vs
  namespace: llm-inference
spec:
  hosts:
  - llm-api.example.com
  gateways:
  - istio-system/main-gateway
  http:
  - match:
    - uri:
        prefix: /v1
    route:
    - destination:
        host: vllm-service
        port:
          number: 8000
    timeout: 600s
    retries:
      attempts: 3
      perTryTimeout: 200s
      retryOn: "5xx,reset,connect-failure"
```

### 2.3 自动扩缩容配置

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: vllm-hpa
  namespace: llm-inference
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: vllm-llama3-70b
  minReplicas: 2
  maxReplicas: 16
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Pods
        value: 2
        periodSeconds: 60
      - type: Percent
        value: 50
        periodSeconds: 60
      selectPolicy: Max
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Pods
        value: 1
        periodSeconds: 120
      selectPolicy: Min
  metrics:
  # GPU利用率
  - type: External
    external:
      metric:
        name: DCGM_FI_DEV_GPU_UTIL
        selector:
          matchLabels:
            app: vllm
      target:
        type: AverageValue
        averageValue: "70"
  # 请求队列深度
  - type: Pods
    pods:
      metric:
        name: vllm_num_requests_waiting
      target:
        type: AverageValue
        averageValue: "10"
  # 正在处理的请求数
  - type: Pods
    pods:
      metric:
        name: vllm_num_requests_running
      target:
        type: AverageValue
        averageValue: "50"
---
# KEDA ScaledObject (更精细控制)
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: vllm-scaledobject
  namespace: llm-inference
spec:
  scaleTargetRef:
    name: vllm-llama3-70b
  minReplicaCount: 2
  maxReplicaCount: 16
  pollingInterval: 15
  cooldownPeriod: 300
  triggers:
  # Prometheus指标
  - type: prometheus
    metadata:
      serverAddress: http://prometheus.monitoring:9090
      metricName: vllm_queue_depth
      threshold: "20"
      query: |
        sum(vllm_num_requests_waiting{namespace="llm-inference"}) / 
        count(vllm_num_requests_waiting{namespace="llm-inference"})
  # GPU显存利用率
  - type: prometheus
    metadata:
      serverAddress: http://prometheus.monitoring:9090
      metricName: gpu_memory_util
      threshold: "85"
      query: |
        avg(DCGM_FI_DEV_MEM_COPY_UTIL{namespace="llm-inference"})
  # 推理延迟
  - type: prometheus
    metadata:
      serverAddress: http://prometheus.monitoring:9090
      metricName: inference_latency_p99
      threshold: "5"
      query: |
        histogram_quantile(0.99, sum(rate(vllm_request_duration_seconds_bucket{namespace="llm-inference"}[5m])) by (le))
```

---

## 三、TensorRT-LLM高性能部署

### 3.1 TensorRT-LLM引擎构建

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: trtllm-engine-builder
  namespace: llm-inference
spec:
  ttlSecondsAfterFinished: 86400
  template:
    spec:
      restartPolicy: OnFailure
      containers:
      - name: builder
        image: nvcr.io/nvidia/tritonserver:24.01-trtllm-python-py3
        command:
        - /bin/bash
        - -c
        - |
          set -ex
          
          # 下载模型
          huggingface-cli download meta-llama/Meta-Llama-3-70B-Instruct \
            --local-dir /models/llama3-70b-hf
          
          # 转换为TensorRT-LLM格式
          python /opt/tensorrt_llm/examples/llama/convert_checkpoint.py \
            --model_dir /models/llama3-70b-hf \
            --output_dir /models/llama3-70b-ckpt \
            --dtype float16 \
            --tp_size 4 \
            --pp_size 1
          
          # 构建TensorRT引擎
          trtllm-build \
            --checkpoint_dir /models/llama3-70b-ckpt \
            --output_dir /engines/llama3-70b \
            --gemm_plugin float16 \
            --gpt_attention_plugin float16 \
            --max_batch_size 64 \
            --max_input_len 4096 \
            --max_output_len 4096 \
            --max_num_tokens 32768 \
            --use_paged_context_fmha enable \
            --multiple_profiles enable \
            --workers 4
          
          echo "Engine build completed!"
        
        env:
        - name: HF_TOKEN
          valueFrom:
            secretKeyRef:
              name: vllm-secrets
              key: HF_TOKEN
        
        resources:
          requests:
            cpu: "32"
            memory: "256Gi"
            nvidia.com/gpu: "4"
          limits:
            cpu: "64"
            memory: "512Gi"
            nvidia.com/gpu: "4"
        
        volumeMounts:
        - name: model-storage
          mountPath: /models
        - name: engine-storage
          mountPath: /engines
      
      volumes:
      - name: model-storage
        persistentVolumeClaim:
          claimName: model-storage-pvc
      - name: engine-storage
        persistentVolumeClaim:
          claimName: engine-storage-pvc
      
      nodeSelector:
        nvidia.com/gpu.product: NVIDIA-A100-SXM4-80GB
```

### 3.2 Triton Inference Server部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: triton-llama3-70b
  namespace: llm-inference
spec:
  replicas: 4
  selector:
    matchLabels:
      app: triton
      model: llama3-70b
  template:
    metadata:
      labels:
        app: triton
        model: llama3-70b
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "8002"
    spec:
      containers:
      - name: triton
        image: nvcr.io/nvidia/tritonserver:24.01-trtllm-python-py3
        command:
        - tritonserver
        args:
        - --model-repository=/models
        - --http-port=8000
        - --grpc-port=8001
        - --metrics-port=8002
        - --log-verbose=1
        - --strict-model-config=false
        - --backend-config=tensorrtllm,decoupled_mode=true
        - --backend-config=tensorrtllm,batching_type=inflight_fused
        - --backend-config=tensorrtllm,max_queue_delay_microseconds=100000
        
        ports:
        - containerPort: 8000
          name: http
        - containerPort: 8001
          name: grpc
        - containerPort: 8002
          name: metrics
        
        resources:
          requests:
            cpu: "16"
            memory: "64Gi"
            nvidia.com/gpu: "4"
          limits:
            cpu: "32"
            memory: "128Gi"
            nvidia.com/gpu: "4"
        
        volumeMounts:
        - name: model-repository
          mountPath: /models
        - name: shm
          mountPath: /dev/shm
        
        livenessProbe:
          httpGet:
            path: /v2/health/live
            port: 8000
          initialDelaySeconds: 60
          periodSeconds: 30
        
        readinessProbe:
          httpGet:
            path: /v2/health/ready
            port: 8000
          initialDelaySeconds: 30
          periodSeconds: 10
      
      volumes:
      - name: model-repository
        persistentVolumeClaim:
          claimName: triton-model-repo-pvc
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: 64Gi
      
      nodeSelector:
        nvidia.com/gpu.product: NVIDIA-H100-SXM5-80GB
```

### 3.3 Triton模型配置

```protobuf
# config.pbtxt - Triton模型配置
name: "llama3-70b"
backend: "tensorrtllm"
max_batch_size: 64

model_transaction_policy {
  decoupled: true
}

dynamic_batching {
  preferred_batch_size: [8, 16, 32, 64]
  max_queue_delay_microseconds: 100000
}

input [
  {
    name: "input_ids"
    data_type: TYPE_INT32
    dims: [-1]
  },
  {
    name: "input_lengths"
    data_type: TYPE_INT32
    dims: [1]
    reshape { shape: [] }
  },
  {
    name: "request_output_len"
    data_type: TYPE_INT32
    dims: [1]
    reshape { shape: [] }
  },
  {
    name: "streaming"
    data_type: TYPE_BOOL
    dims: [1]
    reshape { shape: [] }
    optional: true
  },
  {
    name: "temperature"
    data_type: TYPE_FP32
    dims: [1]
    reshape { shape: [] }
    optional: true
  },
  {
    name: "top_p"
    data_type: TYPE_FP32
    dims: [1]
    reshape { shape: [] }
    optional: true
  },
  {
    name: "top_k"
    data_type: TYPE_INT32
    dims: [1]
    reshape { shape: [] }
    optional: true
  }
]

output [
  {
    name: "output_ids"
    data_type: TYPE_INT32
    dims: [-1, -1]
  },
  {
    name: "sequence_length"
    data_type: TYPE_INT32
    dims: [-1]
  }
]

instance_group [
  {
    count: 1
    kind: KIND_GPU
    gpus: [0, 1, 2, 3]
  }
]

parameters: {
  key: "gpt_model_type"
  value: {
    string_value: "inflight_fused_batching"
  }
}

parameters: {
  key: "gpt_model_path"
  value: {
    string_value: "/models/llama3-70b/engines"
  }
}

parameters: {
  key: "max_tokens_in_paged_kv_cache"
  value: {
    string_value: "2621440"
  }
}

parameters: {
  key: "batch_scheduler_policy"
  value: {
    string_value: "max_utilization"
  }
}

parameters: {
  key: "kv_cache_free_gpu_mem_fraction"
  value: {
    string_value: "0.90"
  }
}
```

---

## 四、Text Generation Inference (TGI) 部署

### 4.1 TGI完整部署配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: tgi-llama3-70b
  namespace: llm-inference
spec:
  replicas: 4
  selector:
    matchLabels:
      app: tgi
      model: llama3-70b
  template:
    metadata:
      labels:
        app: tgi
        model: llama3-70b
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "80"
    spec:
      containers:
      - name: tgi
        image: ghcr.io/huggingface/text-generation-inference:2.0.3
        
        args:
        # 模型配置
        - --model-id=meta-llama/Meta-Llama-3-70B-Instruct
        - --quantize=awq
        - --dtype=float16
        
        # 并行配置
        - --num-shard=4
        - --sharded=true
        
        # 性能配置
        - --max-batch-size=64
        - --max-batch-prefill-tokens=4096
        - --max-total-tokens=8192
        - --max-input-length=4096
        - --max-concurrent-requests=256
        
        # 优化配置
        - --enable-cuda-graphs
        - --cuda-memory-fraction=0.90
        - --waiting-served-ratio=0.3
        
        # 服务配置
        - --hostname=0.0.0.0
        - --port=80
        - --json-output
        
        env:
        - name: HUGGING_FACE_HUB_TOKEN
          valueFrom:
            secretKeyRef:
              name: vllm-secrets
              key: HF_TOKEN
        - name: CUDA_VISIBLE_DEVICES
          value: "0,1,2,3"
        - name: NCCL_DEBUG
          value: "WARN"
        
        ports:
        - containerPort: 80
          name: http
        
        resources:
          requests:
            cpu: "16"
            memory: "64Gi"
            nvidia.com/gpu: "4"
          limits:
            cpu: "32"
            memory: "128Gi"
            nvidia.com/gpu: "4"
        
        volumeMounts:
        - name: model-cache
          mountPath: /data
        - name: shm
          mountPath: /dev/shm
        
        livenessProbe:
          httpGet:
            path: /health
            port: 80
          initialDelaySeconds: 120
          periodSeconds: 30
          timeoutSeconds: 10
        
        readinessProbe:
          httpGet:
            path: /health
            port: 80
          initialDelaySeconds: 60
          periodSeconds: 10
      
      volumes:
      - name: model-cache
        persistentVolumeClaim:
          claimName: model-storage-pvc
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: 32Gi
      
      nodeSelector:
        nvidia.com/gpu.product: NVIDIA-A100-SXM4-80GB
---
apiVersion: v1
kind: Service
metadata:
  name: tgi-service
  namespace: llm-inference
spec:
  ports:
  - port: 80
    targetPort: 80
    name: http
  selector:
    app: tgi
```

---

## 五、Multi-LoRA动态加载

### 5.1 LoRA Adapter管理

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: lora-adapter-config
  namespace: llm-inference
data:
  adapters.yaml: |
    adapters:
      # 代码生成LoRA
      code-assistant:
        path: /adapters/code-assistant
        base_model: llama3-70b
        enabled: true
        
      # 客服对话LoRA
      customer-service:
        path: /adapters/customer-service
        base_model: llama3-70b
        enabled: true
        
      # 医疗问答LoRA
      medical-qa:
        path: /adapters/medical-qa
        base_model: llama3-70b
        enabled: true
        
      # 法律咨询LoRA
      legal-advisor:
        path: /adapters/legal-advisor
        base_model: llama3-70b
        enabled: true
    
    # 默认适配器
    default_adapter: null
    
    # 最大同时加载适配器数
    max_loaded_adapters: 8
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-multi-lora
  namespace: llm-inference
spec:
  replicas: 4
  selector:
    matchLabels:
      app: vllm-multi-lora
  template:
    metadata:
      labels:
        app: vllm-multi-lora
    spec:
      containers:
      - name: vllm
        image: vllm/vllm-openai:v0.4.2
        args:
        - --model=/models/llama3-70b
        - --enable-lora
        - --max-loras=8
        - --max-lora-rank=64
        - --lora-modules
        - code-assistant=/adapters/code-assistant
        - customer-service=/adapters/customer-service
        - medical-qa=/adapters/medical-qa
        - legal-advisor=/adapters/legal-advisor
        - --tensor-parallel-size=4
        - --gpu-memory-utilization=0.85
        
        resources:
          limits:
            nvidia.com/gpu: "4"
        
        volumeMounts:
        - name: base-model
          mountPath: /models
        - name: lora-adapters
          mountPath: /adapters
      
      volumes:
      - name: base-model
        persistentVolumeClaim:
          claimName: base-model-pvc
      - name: lora-adapters
        persistentVolumeClaim:
          claimName: lora-adapters-pvc
```

### 5.2 LoRA动态切换API

```python
# lora_manager.py - LoRA适配器管理服务
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from typing import Optional, Dict, List
import httpx
import asyncio

app = FastAPI(title="LoRA Adapter Manager")

class LoRARequest(BaseModel):
    """LoRA请求模型"""
    model: str = "llama3-70b"
    adapter: Optional[str] = None  # LoRA适配器名称
    messages: List[Dict[str, str]]
    max_tokens: int = 1024
    temperature: float = 0.7
    stream: bool = False

class AdapterInfo(BaseModel):
    """适配器信息"""
    name: str
    path: str
    loaded: bool
    requests_count: int

# 适配器统计
adapter_stats: Dict[str, int] = {}

@app.post("/v1/chat/completions")
async def chat_completions(request: LoRARequest):
    """带LoRA适配器的聊天补全"""
    
    # 构建vLLM请求
    vllm_request = {
        "model": request.model,
        "messages": request.messages,
        "max_tokens": request.max_tokens,
        "temperature": request.temperature,
        "stream": request.stream,
    }
    
    # 如果指定了适配器,添加到模型名称
    if request.adapter:
        vllm_request["model"] = f"{request.model}:{request.adapter}"
        adapter_stats[request.adapter] = adapter_stats.get(request.adapter, 0) + 1
    
    # 转发到vLLM
    async with httpx.AsyncClient() as client:
        response = await client.post(
            "http://vllm-service:8000/v1/chat/completions",
            json=vllm_request,
            timeout=120.0
        )
    
    if response.status_code != 200:
        raise HTTPException(
            status_code=response.status_code,
            detail=response.text
        )
    
    return response.json()

@app.get("/v1/adapters")
async def list_adapters() -> List[AdapterInfo]:
    """列出所有可用的LoRA适配器"""
    
    # 查询vLLM获取已加载的模型
    async with httpx.AsyncClient() as client:
        response = await client.get(
            "http://vllm-service:8000/v1/models"
        )
    
    models_data = response.json()
    adapters = []
    
    for model in models_data.get("data", []):
        model_id = model["id"]
        if ":" in model_id:
            base, adapter = model_id.split(":", 1)
            adapters.append(AdapterInfo(
                name=adapter,
                path=f"/adapters/{adapter}",
                loaded=True,
                requests_count=adapter_stats.get(adapter, 0)
            ))
    
    return adapters

@app.post("/v1/adapters/{adapter_name}/load")
async def load_adapter(adapter_name: str):
    """动态加载LoRA适配器"""
    # 实现动态加载逻辑
    return {"status": "loaded", "adapter": adapter_name}

@app.delete("/v1/adapters/{adapter_name}")
async def unload_adapter(adapter_name: str):
    """卸载LoRA适配器"""
    # 实现动态卸载逻辑
    return {"status": "unloaded", "adapter": adapter_name}

@app.get("/v1/adapters/stats")
async def adapter_stats_endpoint():
    """获取适配器使用统计"""
    return {
        "total_requests": sum(adapter_stats.values()),
        "per_adapter": adapter_stats
    }
```

---

## 六、Speculative Decoding加速

### 6.1 投机解码架构

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    Speculative Decoding Architecture                     │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  ┌─────────────────┐                      ┌─────────────────────────┐   │
│  │   Draft Model   │                      │     Target Model        │   │
│  │  (Small, Fast)  │                      │   (Large, Accurate)     │   │
│  │                 │                      │                         │   │
│  │  Llama-3-8B     │                      │    Llama-3-70B          │   │
│  │  ~1ms/token     │                      │    ~50ms/token          │   │
│  └────────┬────────┘                      └────────────┬────────────┘   │
│           │                                            │                 │
│           │ Generate k tokens                          │ Verify          │
│           │ speculatively                              │ in parallel     │
│           ▼                                            ▼                 │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │                     Verification Process                         │    │
│  │                                                                   │    │
│  │  Draft tokens:    [t1, t2, t3, t4, t5]                           │    │
│  │                    ↓   ↓   ↓   ↓   ↓                             │    │
│  │  Target verify:   [✓   ✓   ✓   ✗   -]                            │    │
│  │                                                                   │    │
│  │  Accept 3 tokens, reject from t4                                 │    │
│  │  Sample new token from target model                               │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│                                                                          │
│  Performance Gain:                                                       │
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │  Standard decoding:  50ms × 100 tokens = 5000ms                  │    │
│  │  Speculative (k=5):  (50ms + 5ms) × 25 iterations = 1375ms       │    │
│  │  Speedup: ~3.6x                                                   │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 6.2 vLLM Speculative Decoding配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-speculative
  namespace: llm-inference
spec:
  replicas: 4
  selector:
    matchLabels:
      app: vllm-speculative
  template:
    metadata:
      labels:
        app: vllm-speculative
    spec:
      containers:
      - name: vllm
        image: vllm/vllm-openai:v0.4.2
        args:
        # 目标模型
        - --model=/models/llama3-70b
        - --served-model-name=llama3-70b
        
        # 投机解码配置
        - --speculative-model=/models/llama3-8b
        - --num-speculative-tokens=5
        - --speculative-draft-tensor-parallel-size=1
        - --use-v2-block-manager
        
        # 性能配置
        - --tensor-parallel-size=4
        - --gpu-memory-utilization=0.90
        - --max-model-len=8192
        - --enable-prefix-caching
        
        resources:
          limits:
            nvidia.com/gpu: "4"
        
        volumeMounts:
        - name: models
          mountPath: /models
      
      volumes:
      - name: models
        persistentVolumeClaim:
          claimName: model-storage-pvc
```

### 6.3 性能对比基准

| 配置 | 模型 | 吞吐量 (tok/s) | P50延迟 | P99延迟 | 加速比 |
|-----|------|---------------|---------|---------|--------|
| 标准解码 | Llama3-70B | 45 | 22ms/tok | 35ms/tok | 1.0x |
| Speculative (k=3) | 70B + 8B | 95 | 11ms/tok | 18ms/tok | 2.1x |
| Speculative (k=5) | 70B + 8B | 120 | 8ms/tok | 15ms/tok | 2.7x |
| Speculative (k=7) | 70B + 8B | 135 | 7ms/tok | 14ms/tok | 3.0x |
| Speculative + AWQ | 70B + 8B | 180 | 5ms/tok | 10ms/tok | 4.0x |

---

## 七、KV Cache优化

### 7.1 PagedAttention配置

```python
# kv_cache_config.py - KV Cache配置
from dataclasses import dataclass
from typing import Optional

@dataclass
class KVCacheConfig:
    """KV Cache配置参数"""
    
    # Block配置
    block_size: int = 16  # 每个block的token数
    num_gpu_blocks: Optional[int] = None  # GPU blocks数量,自动计算
    num_cpu_blocks: int = 512  # CPU swap blocks数量
    
    # 内存配置
    gpu_memory_utilization: float = 0.90  # GPU显存利用率
    swap_space_gb: float = 16  # CPU交换空间 (GB)
    
    # 缓存策略
    enable_prefix_caching: bool = True  # 启用前缀缓存
    sliding_window: Optional[int] = None  # 滑动窗口大小
    
    # 高级配置
    cache_dtype: str = "auto"  # 缓存数据类型
    max_num_seqs: int = 256  # 最大并发序列数
    max_num_batched_tokens: int = 32768  # 最大批处理tokens

# vLLM启动参数
VLLM_KV_CACHE_ARGS = [
    "--block-size=16",
    "--gpu-memory-utilization=0.90",
    "--swap-space=16",
    "--enable-prefix-caching",
    "--max-num-seqs=256",
    "--max-num-batched-tokens=32768",
]
```

### 7.2 Prefix Caching策略

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: prefix-cache-config
  namespace: llm-inference
data:
  # 系统提示缓存
  system_prompts.yaml: |
    prompts:
      # 通用助手
      general_assistant:
        hash: "sha256:abc123..."
        text: |
          You are a helpful AI assistant. You provide accurate, 
          helpful, and harmless responses to user queries.
        cache_priority: high
        
      # 代码助手
      code_assistant:
        hash: "sha256:def456..."
        text: |
          You are an expert programmer. You write clean, efficient, 
          and well-documented code. You follow best practices.
        cache_priority: high
        
      # 客服助手
      customer_service:
        hash: "sha256:ghi789..."
        text: |
          You are a professional customer service representative.
          You are polite, helpful, and solution-oriented.
        cache_priority: medium
    
    # 缓存配置
    cache_config:
      max_cached_prompts: 100
      eviction_policy: lru
      preload_on_startup: true
```

### 7.3 KV Cache监控

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: kv-cache-alerts
  namespace: llm-inference
spec:
  groups:
  - name: kv-cache
    rules:
    # KV Cache利用率
    - record: vllm:kv_cache_utilization
      expr: |
        vllm_gpu_cache_usage_perc
    
    # Cache命中率
    - record: vllm:prefix_cache_hit_rate
      expr: |
        rate(vllm_prefix_cache_hit_total[5m]) / 
        (rate(vllm_prefix_cache_hit_total[5m]) + rate(vllm_prefix_cache_miss_total[5m]))
    
    # 告警: KV Cache接近满
    - alert: KVCacheHighUtilization
      expr: vllm_gpu_cache_usage_perc > 95
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "KV Cache利用率过高"
        description: "Pod {{ $labels.pod }} KV Cache利用率 {{ $value }}%"
    
    # 告警: 前缀缓存命中率低
    - alert: LowPrefixCacheHitRate
      expr: vllm:prefix_cache_hit_rate < 0.3
      for: 15m
      labels:
        severity: info
      annotations:
        summary: "前缀缓存命中率低"
        description: "命中率 {{ $value | humanizePercentage }}"
    
    # 告警: 频繁Swap
    - alert: FrequentKVCacheSwap
      expr: rate(vllm_cache_swap_out_total[5m]) > 100
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "KV Cache频繁交换到CPU"
```

---

## 八、负载均衡与流量管理

### 8.1 智能路由配置

```yaml
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: llm-routing
  namespace: llm-inference
spec:
  hosts:
  - llm-api.example.com
  gateways:
  - llm-gateway
  http:
  # 长上下文请求路由到专用实例
  - match:
    - headers:
        x-max-tokens:
          regex: "^([89][0-9]{3}|[1-9][0-9]{4,})$"  # >8000 tokens
    route:
    - destination:
        host: vllm-long-context
        port:
          number: 8000
    timeout: 600s
  
  # 流式请求启用WebSocket
  - match:
    - headers:
        x-stream:
          exact: "true"
    route:
    - destination:
        host: vllm-service
        port:
          number: 8000
    timeout: 300s
  
  # 特定模型路由
  - match:
    - headers:
        x-model:
          exact: "llama3-70b-code"
    route:
    - destination:
        host: vllm-code-model
        port:
          number: 8000
  
  # 默认路由 - 加权负载均衡
  - route:
    - destination:
        host: vllm-service
        port:
          number: 8000
      weight: 70
    - destination:
        host: tgi-service
        port:
          number: 80
      weight: 30
    timeout: 120s
    retries:
      attempts: 3
      perTryTimeout: 45s
      retryOn: "5xx,reset,connect-failure,retriable-4xx"
---
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: vllm-destination
  namespace: llm-inference
spec:
  host: vllm-service
  trafficPolicy:
    connectionPool:
      tcp:
        maxConnections: 1000
        connectTimeout: 30s
      http:
        http1MaxPendingRequests: 500
        http2MaxRequests: 1000
        maxRequestsPerConnection: 100
        maxRetries: 3
    loadBalancer:
      simple: LEAST_REQUEST
    outlierDetection:
      consecutive5xxErrors: 5
      interval: 30s
      baseEjectionTime: 60s
      maxEjectionPercent: 30
```

### 8.2 请求优先级队列

```python
# priority_queue.py - 请求优先级队列
from fastapi import FastAPI, Request, HTTPException
from pydantic import BaseModel
from typing import Optional, List, Dict
import asyncio
import heapq
from datetime import datetime
from enum import IntEnum

class Priority(IntEnum):
    """请求优先级"""
    CRITICAL = 0    # 最高优先级
    HIGH = 1
    NORMAL = 2
    LOW = 3
    BATCH = 4       # 最低优先级

class InferenceRequest(BaseModel):
    """推理请求"""
    request_id: str
    model: str
    messages: List[Dict]
    max_tokens: int = 1024
    priority: Priority = Priority.NORMAL
    user_tier: str = "free"  # free, pro, enterprise
    created_at: float = None

class PriorityQueue:
    """优先级请求队列"""
    
    def __init__(self, max_size: int = 1000):
        self.max_size = max_size
        self.queue: List[tuple] = []  # (priority, timestamp, request)
        self.lock = asyncio.Lock()
        
        # 每个优先级的配额
        self.quotas = {
            Priority.CRITICAL: 100,
            Priority.HIGH: 200,
            Priority.NORMAL: 500,
            Priority.LOW: 150,
            Priority.BATCH: 50
        }
        self.current_counts = {p: 0 for p in Priority}
    
    async def enqueue(self, request: InferenceRequest) -> bool:
        """入队请求"""
        async with self.lock:
            # 检查队列是否已满
            if len(self.queue) >= self.max_size:
                # 尝试淘汰低优先级请求
                if not await self._evict_lower_priority(request.priority):
                    raise HTTPException(
                        status_code=429,
                        detail="Queue full, please retry later"
                    )
            
            # 检查优先级配额
            if self.current_counts[request.priority] >= self.quotas[request.priority]:
                raise HTTPException(
                    status_code=429,
                    detail=f"Priority {request.priority.name} quota exceeded"
                )
            
            # 入队
            timestamp = request.created_at or datetime.now().timestamp()
            heapq.heappush(self.queue, (request.priority, timestamp, request))
            self.current_counts[request.priority] += 1
            
            return True
    
    async def dequeue(self) -> Optional[InferenceRequest]:
        """出队请求"""
        async with self.lock:
            if not self.queue:
                return None
            
            priority, _, request = heapq.heappop(self.queue)
            self.current_counts[priority] -= 1
            
            return request
    
    async def _evict_lower_priority(self, min_priority: Priority) -> bool:
        """淘汰低优先级请求"""
        # 查找并移除最低优先级的请求
        for i in range(len(self.queue) - 1, -1, -1):
            if self.queue[i][0] > min_priority:
                evicted = self.queue.pop(i)
                self.current_counts[evicted[0]] -= 1
                heapq.heapify(self.queue)
                return True
        return False
    
    def get_stats(self) -> Dict:
        """获取队列统计"""
        return {
            "total_queued": len(self.queue),
            "by_priority": dict(self.current_counts),
            "utilization": len(self.queue) / self.max_size
        }

# 根据用户等级确定优先级
USER_TIER_PRIORITY = {
    "enterprise": Priority.HIGH,
    "pro": Priority.NORMAL,
    "free": Priority.LOW
}

app = FastAPI()
queue = PriorityQueue(max_size=1000)

@app.post("/v1/chat/completions")
async def chat_completions(request: Request):
    """带优先级的聊天补全"""
    body = await request.json()
    
    # 获取用户等级
    user_tier = request.headers.get("x-user-tier", "free")
    api_key = request.headers.get("authorization", "")
    
    # 创建推理请求
    inference_request = InferenceRequest(
        request_id=str(uuid.uuid4()),
        model=body.get("model", "llama3-70b"),
        messages=body.get("messages", []),
        max_tokens=body.get("max_tokens", 1024),
        priority=USER_TIER_PRIORITY.get(user_tier, Priority.NORMAL),
        user_tier=user_tier,
        created_at=datetime.now().timestamp()
    )
    
    # 入队
    await queue.enqueue(inference_request)
    
    # 处理请求...
    # (实际实现中会异步处理队列)

@app.get("/v1/queue/stats")
async def queue_stats():
    """获取队列统计"""
    return queue.get_stats()
```

---

## 九、监控与可观测性

### 9.1 完整监控Dashboard

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: llm-inference-rules
  namespace: llm-inference
spec:
  groups:
  - name: llm-inference-slos
    rules:
    # SLO: 99%请求延迟<5s
    - record: llm:request_latency_p99
      expr: |
        histogram_quantile(0.99, 
          sum(rate(vllm_request_duration_seconds_bucket[5m])) by (le, model)
        )
    
    # SLO: 吞吐量
    - record: llm:throughput_tokens_per_second
      expr: |
        sum(rate(vllm_generation_tokens_total[5m])) by (model)
    
    # SLO: 可用性
    - record: llm:availability
      expr: |
        sum(rate(vllm_request_success_total[5m])) by (model) /
        sum(rate(vllm_request_total[5m])) by (model)
    
    # 错误率
    - record: llm:error_rate
      expr: |
        sum(rate(vllm_request_error_total[5m])) by (model, error_type) /
        sum(rate(vllm_request_total[5m])) by (model)
  
  - name: llm-inference-alerts
    rules:
    # 高延迟告警
    - alert: LLMHighLatency
      expr: llm:request_latency_p99 > 10
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "LLM推理延迟过高"
        description: "模型 {{ $labels.model }} P99延迟 {{ $value }}秒"
    
    # 低吞吐量告警
    - alert: LLMLowThroughput
      expr: llm:throughput_tokens_per_second < 100
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "LLM吞吐量下降"
        description: "模型 {{ $labels.model }} 吞吐量 {{ $value }} tok/s"
    
    # 高错误率告警
    - alert: LLMHighErrorRate
      expr: llm:error_rate > 0.05
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "LLM错误率过高"
        description: "模型 {{ $labels.model }} 错误率 {{ $value | humanizePercentage }}"
    
    # GPU显存不足
    - alert: LLMGPUMemoryHigh
      expr: DCGM_FI_DEV_MEM_COPY_UTIL > 95
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "GPU显存利用率过高"
        description: "GPU {{ $labels.gpu }} 显存利用率 {{ $value }}%"
    
    # 队列积压
    - alert: LLMQueueBacklog
      expr: vllm_num_requests_waiting > 100
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "推理请求队列积压"
        description: "等待队列 {{ $value }} 个请求"
    
    # Pod不健康
    - alert: LLMPodUnhealthy
      expr: |
        kube_pod_status_ready{namespace="llm-inference", condition="true"} == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "LLM Pod不健康"
        description: "Pod {{ $labels.pod }} 不可用"
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: grafana-llm-dashboard
  namespace: monitoring
  labels:
    grafana_dashboard: "1"
data:
  llm-inference.json: |
    {
      "dashboard": {
        "title": "LLM Inference Monitoring",
        "panels": [
          {
            "title": "Requests per Second",
            "type": "graph",
            "targets": [
              {
                "expr": "sum(rate(vllm_request_total[5m])) by (model)",
                "legendFormat": "{{ model }}"
              }
            ]
          },
          {
            "title": "P50/P95/P99 Latency",
            "type": "graph",
            "targets": [
              {
                "expr": "histogram_quantile(0.50, sum(rate(vllm_request_duration_seconds_bucket[5m])) by (le))",
                "legendFormat": "P50"
              },
              {
                "expr": "histogram_quantile(0.95, sum(rate(vllm_request_duration_seconds_bucket[5m])) by (le))",
                "legendFormat": "P95"
              },
              {
                "expr": "histogram_quantile(0.99, sum(rate(vllm_request_duration_seconds_bucket[5m])) by (le))",
                "legendFormat": "P99"
              }
            ]
          },
          {
            "title": "Throughput (tokens/s)",
            "type": "graph",
            "targets": [
              {
                "expr": "sum(rate(vllm_generation_tokens_total[5m])) by (model)",
                "legendFormat": "{{ model }}"
              }
            ]
          },
          {
            "title": "GPU Utilization",
            "type": "graph",
            "targets": [
              {
                "expr": "DCGM_FI_DEV_GPU_UTIL",
                "legendFormat": "GPU {{ gpu }}"
              }
            ]
          },
          {
            "title": "KV Cache Utilization",
            "type": "gauge",
            "targets": [
              {
                "expr": "avg(vllm_gpu_cache_usage_perc)"
              }
            ]
          },
          {
            "title": "Queue Depth",
            "type": "graph",
            "targets": [
              {
                "expr": "vllm_num_requests_waiting",
                "legendFormat": "Waiting"
              },
              {
                "expr": "vllm_num_requests_running",
                "legendFormat": "Running"
              }
            ]
          }
        ]
      }
    }
```

### 9.2 请求追踪配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: otel-collector-config
  namespace: llm-inference
data:
  config.yaml: |
    receivers:
      otlp:
        protocols:
          grpc:
            endpoint: 0.0.0.0:4317
          http:
            endpoint: 0.0.0.0:4318
    
    processors:
      batch:
        timeout: 1s
        send_batch_size: 1000
      
      attributes:
        actions:
        - key: service.name
          value: llm-inference
          action: upsert
        - key: deployment.environment
          value: production
          action: upsert
    
    exporters:
      jaeger:
        endpoint: jaeger-collector:14250
        tls:
          insecure: true
      
      prometheus:
        endpoint: 0.0.0.0:8889
        namespace: llm_inference
    
    service:
      pipelines:
        traces:
          receivers: [otlp]
          processors: [batch, attributes]
          exporters: [jaeger]
        metrics:
          receivers: [otlp]
          processors: [batch]
          exporters: [prometheus]
```

---

## 十、性能基准与调优

### 10.1 模型性能基准 (A100 80GB)

| 模型 | 配置 | 吞吐量 (tok/s) | P50延迟 | P99延迟 | 显存占用 | 成本/1M tokens |
|-----|------|---------------|---------|---------|----------|---------------|
| **Llama3-8B** |
| | FP16, 1×A100 | 2,800 | 8ms | 15ms | 18GB | $0.02 |
| | INT8, 1×A100 | 3,500 | 6ms | 12ms | 10GB | $0.015 |
| | AWQ-4bit, 1×A100 | 4,200 | 5ms | 10ms | 6GB | $0.012 |
| **Llama3-70B** |
| | FP16, 4×A100 | 350 | 65ms | 120ms | 160GB | $0.18 |
| | INT8, 2×A100 | 420 | 55ms | 100ms | 80GB | $0.12 |
| | AWQ-4bit, 1×A100 | 480 | 48ms | 90ms | 42GB | $0.08 |
| **Mixtral-8x7B** |
| | FP16, 2×A100 | 800 | 28ms | 55ms | 100GB | $0.08 |
| | AWQ-4bit, 1×A100 | 1,100 | 20ms | 40ms | 28GB | $0.045 |
| **Llama3-405B** |
| | FP16, 8×H100 | 120 | 180ms | 350ms | 810GB | $0.65 |
| | FP8, 4×H100 | 180 | 120ms | 230ms | 420GB | $0.35 |
| | INT4, 2×H100 | 220 | 100ms | 200ms | 220GB | $0.22 |

### 10.2 推理引擎性能对比

| 引擎 | 模型 | Batch Size | 吞吐量 (tok/s) | P99延迟 | 显存效率 |
|-----|------|-----------|---------------|---------|----------|
| **vLLM** | Llama3-70B-AWQ | 64 | 4,800 | 95ms | 92% |
| **TGI** | Llama3-70B-AWQ | 64 | 4,200 | 110ms | 88% |
| **TensorRT-LLM** | Llama3-70B-FP8 | 64 | 6,500 | 72ms | 95% |
| **SGLang** | Llama3-70B-AWQ | 64 | 5,200 | 85ms | 93% |
| **DeepSpeed-MII** | Llama3-70B | 64 | 3,800 | 125ms | 85% |

### 10.3 调优参数指南

```yaml
# 性能调优参数参考
performance_tuning:
  # 吞吐量优化
  high_throughput:
    max_num_seqs: 512
    max_num_batched_tokens: 65536
    gpu_memory_utilization: 0.95
    enable_prefix_caching: true
    block_size: 32
    swap_space: 32
    
  # 延迟优化
  low_latency:
    max_num_seqs: 64
    max_num_batched_tokens: 8192
    gpu_memory_utilization: 0.80
    enable_prefix_caching: true
    block_size: 16
    use_speculative_decoding: true
    num_speculative_tokens: 5
    
  # 成本优化
  cost_optimized:
    quantization: awq
    dtype: float16
    tensor_parallel_size: 1  # 最小化GPU使用
    gpu_memory_utilization: 0.95
    enable_prefix_caching: true
    
  # 长上下文优化
  long_context:
    max_model_len: 128000
    max_num_seqs: 32
    enable_chunked_prefill: true
    max_num_batched_tokens: 32768
    gpu_memory_utilization: 0.90
```

---

## 十一、故障排查

### 11.1 常见问题诊断

| 问题 | 症状 | 可能原因 | 解决方案 |
|-----|------|---------|---------|
| **OOM错误** | CUDA out of memory | 模型太大/batch太大 | 降低gpu_memory_utilization,减少max_num_seqs |
| **启动慢** | 模型加载>10分钟 | 网络慢/存储慢 | 使用本地NVMe SSD,预下载模型 |
| **吞吐量低** | <预期50% | CPU瓶颈/IO瓶颈 | 增加CPU核心,使用更快存储 |
| **延迟抖动** | P99>>P50 | GC/Swap | 增加swap_space,调整block_size |
| **请求超时** | 504错误 | 队列积压/资源不足 | 增加副本,调整HPA阈值 |
| **NCCL错误** | 多GPU通信失败 | 网络配置错误 | 检查NCCL环境变量,IB配置 |

### 11.2 诊断命令

```bash
#!/bin/bash
# llm-diagnosis.sh - LLM推理服务诊断脚本

# 1. 检查Pod状态
echo "=== Pod状态 ==="
kubectl get pods -n llm-inference -o wide

# 2. 检查GPU状态
echo "=== GPU状态 ==="
kubectl exec -n llm-inference deployment/vllm-llama3-70b -- nvidia-smi

# 3. 检查vLLM健康
echo "=== vLLM健康检查 ==="
kubectl exec -n llm-inference deployment/vllm-llama3-70b -- \
  curl -s localhost:8000/health | jq .

# 4. 检查队列状态
echo "=== 队列状态 ==="
kubectl exec -n llm-inference deployment/vllm-llama3-70b -- \
  curl -s localhost:8000/metrics | grep -E "vllm_num_requests"

# 5. 检查KV Cache
echo "=== KV Cache状态 ==="
kubectl exec -n llm-inference deployment/vllm-llama3-70b -- \
  curl -s localhost:8000/metrics | grep -E "vllm_.*cache"

# 6. 检查错误日志
echo "=== 最近错误 ==="
kubectl logs -n llm-inference deployment/vllm-llama3-70b --tail=100 | grep -i error

# 7. 检查资源使用
echo "=== 资源使用 ==="
kubectl top pods -n llm-inference

# 8. 检查HPA状态
echo "=== HPA状态 ==="
kubectl get hpa -n llm-inference

# 9. 性能测试
echo "=== 性能测试 ==="
curl -X POST http://vllm-service.llm-inference:8000/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "llama3-70b",
    "messages": [{"role": "user", "content": "Hello"}],
    "max_tokens": 100
  }' -w "\nTotal time: %{time_total}s\n"
```

---

## 十二、快速参考

### 12.1 vLLM启动参数速查

```bash
# 基础启动
python -m vllm.entrypoints.openai.api_server \
  --model meta-llama/Meta-Llama-3-70B-Instruct \
  --tensor-parallel-size 4

# 生产配置
python -m vllm.entrypoints.openai.api_server \
  --model /models/llama3-70b-awq \
  --quantization awq \
  --tensor-parallel-size 4 \
  --dtype float16 \
  --max-model-len 8192 \
  --max-num-seqs 256 \
  --gpu-memory-utilization 0.90 \
  --enable-prefix-caching \
  --api-key $API_KEY

# 投机解码
python -m vllm.entrypoints.openai.api_server \
  --model /models/llama3-70b \
  --speculative-model /models/llama3-8b \
  --num-speculative-tokens 5 \
  --tensor-parallel-size 4

# Multi-LoRA
python -m vllm.entrypoints.openai.api_server \
  --model /models/llama3-70b \
  --enable-lora \
  --max-loras 8 \
  --lora-modules adapter1=/adapters/adapter1 adapter2=/adapters/adapter2
```

### 12.2 API调用示例

```python
# OpenAI兼容API调用
import openai

client = openai.OpenAI(
    base_url="http://vllm-service:8000/v1",
    api_key="sk-xxx"
)

# 同步调用
response = client.chat.completions.create(
    model="llama3-70b",
    messages=[{"role": "user", "content": "Hello"}],
    max_tokens=1024,
    temperature=0.7
)

# 流式调用
stream = client.chat.completions.create(
    model="llama3-70b",
    messages=[{"role": "user", "content": "Write a story"}],
    max_tokens=2048,
    stream=True
)
for chunk in stream:
    print(chunk.choices[0].delta.content, end="")

# 使用LoRA适配器
response = client.chat.completions.create(
    model="llama3-70b:code-assistant",  # 指定适配器
    messages=[{"role": "user", "content": "Write a Python function"}],
    max_tokens=1024
)
```

### 12.3 性能监控指标

| 指标 | Prometheus Query | 健康阈值 |
|-----|------------------|----------|
| 请求延迟P99 | `histogram_quantile(0.99, rate(vllm_request_duration_seconds_bucket[5m]))` | <5s |
| 吞吐量 | `sum(rate(vllm_generation_tokens_total[5m]))` | >1000 tok/s |
| 队列深度 | `vllm_num_requests_waiting` | <50 |
| GPU利用率 | `DCGM_FI_DEV_GPU_UTIL` | 60-90% |
| KV Cache使用率 | `vllm_gpu_cache_usage_perc` | <95% |
| 错误率 | `rate(vllm_request_error_total[5m])/rate(vllm_request_total[5m])` | <1% |

---

## 十三、最佳实践总结

### 生产部署检查清单

- [ ] **高可用**: 最少2个副本,跨AZ部署
- [ ] **资源隔离**: GPU节点专用污点,Pod亲和性
- [ ] **健康检查**: 配置startup/liveness/readiness探针
- [ ] **自动扩缩容**: HPA基于GPU利用率和队列深度
- [ ] **监控告警**: Prometheus指标,Grafana Dashboard
- [ ] **日志追踪**: 结构化日志,分布式追踪
- [ ] **安全**: API认证,网络策略,Secret管理
- [ ] **成本优化**: 量化模型,Spot实例,自动缩容
- [ ] **灾备**: 模型备份,快速恢复流程

---

**相关文档**: [145-LLM Serving架构](145-llm-serving-architecture.md) | [146-LLM量化技术](146-llm-quantization.md) | [133-GPU调度管理](133-gpu-scheduling-management.md)

**版本**: vLLM 0.4.2+ | TGI 2.0+ | TensorRT-LLM 0.9+ | Kubernetes v1.27+
