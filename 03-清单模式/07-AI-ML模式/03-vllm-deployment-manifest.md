---
title: vLLM 推理部署清单
description: vLLM 高性能 LLM 推理服务部署配置
summary: vLLM 部署清单，包括 PagedAttention、连续批处理、多 GPU 张量并行及生产级监控配置
category: manifests-patterns
tags:
- k8s
- manifests
- ai-ml-infra
- vllm
- llm
- inference
- gpu
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
- vLLM 如何部署
- vLLM Kubernetes 部署
- LLM 推理服务
trigger_keywords:
- vllm
- llm
- inference
- pagedattention
- tensor-parallel
prerequisites:
- gpu-basics
- k8s-deployment-basics
authors:
- name: KUDIG Team
  role: contributor
---

# vLLM 推理部署清单

## 1. vLLM 优势

vLLM 是高性能 LLM 推理引擎，核心特性：
- **PagedAttention**：减少 KV Cache 内存碎片，吞吐量提升 2-4 倍
- **连续批处理**：动态合并请求，最大化 GPU 利用率
- **张量并行**：支持多 GPU 推理大模型
- **兼容 OpenAI API**：直接替换 OpenAI 接口

## 2. 单 GPU 部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: vllm-server
  namespace: ai-inference
spec:
  replicas: 1
  selector:
    matchLabels:
      app: vllm-server
  template:
    metadata:
      labels:
        app: vllm-server
    spec:
      nodeSelector:
        nvidia.com/gpu.product: "NVIDIA-A100-SXM4-40GB"
      tolerations:
        - key: nvidia.com/gpu
          operator: Exists
      containers:
        - name: vllm
          image: vllm/vllm-openai:v0.5.0
          ports:
            - containerPort: 8000
              name: http
          resources:
            limits:
              nvidia.com/gpu: 1
              memory: 32Gi
              cpu: "8"
          args:
            - --model=meta-llama/Llama-2-7b-chat-hf
            - --tensor-parallel-size=1
            - --gpu-memory-utilization=0.9
            - --max-model-len=4096
            - --quantization=awq         # 量化（可选）
            - --dtype=half
            - --host=0.0.0.0
            - --port=8000
          env:
            - name: HUGGING_FACE_HUB_TOKEN
              valueFrom:
                secretKeyRef:
                  name: hf-token
                  key: token
          volumeMounts:
            - name: model-cache
              mountPath: /root/.cache/huggingface
            - name: dshm
              mountPath: /dev/shm
          livenessProbe:
            httpGet:
              path: /health
              port: 8000
            initialDelaySeconds: 120     # 模型加载较慢
            periodSeconds: 30
          readinessProbe:
            httpGet:
              path: /health
              port: 8000
            initialDelaySeconds: 60
            periodSeconds: 10
      volumes:
        - name: model-cache
          persistentVolumeClaim:
            claimName: model-cache-pvc
        - name: dshm
          emptyDir:
            medium: Memory
            sizeLimit: 16Gi
```

## 3. 多 GPU 张量并行

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: vllm-70b
  namespace: ai-inference
spec:
  serviceName: vllm-70b
  replicas: 1
  selector:
    matchLabels:
      app: vllm-70b
  template:
    metadata:
      labels:
        app: vllm-70b
    spec:
      nodeSelector:
        nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
      containers:
        - name: vllm
          image: vllm/vllm-openai:v0.5.0
          resources:
            limits:
              nvidia.com/gpu: 4         # 4 GPU 张量并行
              memory: 256Gi
              cpu: "32"
          args:
            - --model=meta-llama/Llama-2-70b-chat-hf
            - --tensor-parallel-size=4  # 4 路张量并行
            - --gpu-memory-utilization=0.9
            - --max-model-len=8192
            - --host=0.0.0.0
            - --port=8000
          env:
            - name: HF_TOKEN
              valueFrom:
                secretKeyRef:
                  name: hf-token
                  key: token
            - name: NCCL_DEBUG
              value: "WARN"
            - name: NCCL_SOCKET_IFNAME
              value: "eth0"
          ports:
            - containerPort: 8000
              name: http
```

## 4. 模型缓存 PVC

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: model-cache-pvc
  namespace: ai-inference
spec:
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 200Gi                 # 足够缓存多个模型
  storageClassName: fast-ssd
```

## 5. Service 与 Ingress

```yaml
apiVersion: v1
kind: Service
metadata:
  name: vllm-service
  namespace: ai-inference
spec:
  selector:
    app: vllm-server
  ports:
    - port: 80
      targetPort: 8000
      name: http
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: vllm-ingress
  namespace: ai-inference
  annotations:
    nginx.ingress.kubernetes.io/proxy-body-size: "100m"
    nginx.ingress.kubernetes.io/proxy-read-timeout: "300"
    nginx.ingress.kubernetes.io/proxy-send-timeout: "300"
spec:
  rules:
    - host: llm-api.example.com
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: vllm-service
                port:
                  number: 80
```

## 6. HPConfig（基于队列长度）

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: vllm-hpa
  namespace: ai-inference
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: vllm-server
  minReplicas: 1
  maxReplicas: 4
  metrics:
    - type: Pods
      pods:
        metric:
          name: vllm_pending_requests
        target:
          type: AverageValue
          averageValue: "10"          # 每实例 10 个排队请求时扩容
```

## 7. ServiceMonitor（Prometheus）

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: vllm-monitor
  namespace: ai-inference
spec:
  selector:
    matchLabels:
      app: vllm-server
  endpoints:
    - port: http
      path: /metrics
      interval: 15s
```

关键指标：

| 指标 | 说明 |
|------|------|
| `vllm:num_requests_running` | 正在运行的请求数 |
| `vllm:num_requests_waiting` | 等待中的请求数 |
| `vllm:gpu_cache_usage_perc` | KV Cache 使用率 |
| `vllm:time_to_first_token_seconds` | 首 Token 延迟 |
| `vllm:time_per_output_token_seconds` | 每 Token 生成时间 |

## 8. 生产实践

| 实践 | 说明 |
|------|------|
| 预加载模型 | 使用 PVC 缓存，避免每次启动都下载 |
| 设置 `gpu-memory-utilization` | 0.85-0.9，留余量给系统 |
| 使用 AWQ/GPTQ 量化 | 减少显存占用，提升吞吐 |
| 配置合理的 `max-model-len` | 根据实际需求设置，避免内存浪费 |
| 监控 KV Cache 使用率 | 过高说明并发过多 |
| 使用 Spot Instance（非关键） | 降低成本 |

## 9. 测试验证

```bash
# 🟢 低风险：推理测试
# 发送 OpenAI 兼容请求
curl http://vllm-service/v1/chat/completions \
  -H "Content-Type: application/json" \
  -d '{
    "model": "meta-llama/Llama-2-7b-chat-hf",
    "messages": [{"role": "user", "content": "你好"}],
    "max_tokens": 100
  }'

# 压测
python -m vllm.entrypoints.openai.api_server_benchmark \
  --backend vllm \
  --host vllm-service \
  --port 80 \
  --model meta-llama/Llama-2-7b-chat-hf \
  --num-prompts 100
```

## Related

- [[03-清单模式/07-AI-ML模式/04-triton-deployment-manifest|Triton 推理部署]]
- [[03-清单模式/07-AI-ML模式/07-model-serving-hpa|KEDA + HPA 弹性]]

## See Also

- [vLLM GitHub](https://github.com/vllm-project/vllm)
- [vLLM 部署文档](https://docs.vllm.ai/en/latest/)

<!-- risk-assessed -->
