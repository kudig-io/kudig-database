---
title: "AI 工作负载可靠性"
description: "AI/ML 工作负载可靠性保障：GPU 故障处理、推理服务 SLA、模型降级策略、Checkpoint 恢复与训练任务容错"
summary: "系统化的 AI 工作负载可靠性实践，覆盖 GPU 硬件故障检测与自动恢复、推理服务高可用与 SLA 保障、模型降级与 Fallback 策略、分布式训练 Checkpoint 恢复机制以及 Kubernetes 上 AI 工作负载的容错设计模式"
category: 可靠性
tags:
- ai-reliability
- gpu-fault
- inference-sla
- model-degradation
- checkpoint
- training-fault-tolerance
- kubernetes-ai
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
intent_queries:
- "Kubernetes 上 GPU 故障如何自动检测和恢复"
- "AI 推理服务如何保障 SLA"
- "分布式训练任务如何实现 Checkpoint 容错"
trigger_keywords:
- GPU故障
- 推理服务
- 模型降级
- checkpoint
- 训练容错
- AI可靠性
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

# AI 工作负载可靠性

## 概述

AI/ML 工作负载与传统微服务在可靠性保障上存在本质差异：GPU 硬件故障率远高于 CPU（年化故障率 5-15% vs <1%）、单次训练任务可能持续数天到数周、推理服务对延迟极度敏感（P99 < 100ms）、模型文件体积巨大（数 GB 到数百 GB）。这些特性要求专门的可靠性工程实践。

本文覆盖 AI 工作负载可靠性的完整技术栈：GPU 故障检测与自动恢复、推理服务 SLA 保障体系、模型降级与 Fallback 策略、分布式训练 Checkpoint 机制以及 Kubernetes 上的容错设计模式。与 [[15-AI基础设施/README|AI基础设施]] 中的基础部署指南不同，本文聚焦于生产环境的可靠性工程。

## 核心概念

### AI 工作负载可靠性挑战

```
┌─────────────────────────────────────────────────────────────────┐
│                AI 工作负载可靠性挑战全景                           │
│                                                                   │
│  硬件层                                                           │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  GPU 故障模式:                                            │    │
│  │  • ECC 内存错误（可纠正/不可纠正）                         │    │
│  │  • GPU 掉卡（PCIe 链路断开）                              │    │
│  │  • 显存泄漏 / OOM                                        │    │
│  │  • NVLink/InfiniBand 网络故障                            │    │
│  │  • 温度过高降频 (Thermal Throttling)                      │    │
│  │  • 驱动崩溃 (XID Errors)                                 │    │
│  └──────────────────────────────────────────────────────────┘    │
│                                                                   │
│  训练层                                                           │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  分布式训练故障:                                          │    │
│  │  • 单 Worker 失败导致整个 Job 重启                        │    │
│  │  • 梯度同步超时 (AllReduce Timeout)                       │    │
│  │  • Checkpoint 写入失败 / 损坏                            │    │
│  │  • 数据加载瓶颈 (I/O Bound)                              │    │
│  │  • 长时间运行中的累积性错误                               │    │
│  └──────────────────────────────────────────────────────────┘    │
│                                                                   │
│  推理层                                                           │
│  ┌──────────────────────────────────────────────────────────┐    │
│  │  推理服务挑战:                                            │    │
│  │  • 模型加载时间长（分钟级冷启动）                         │    │
│  │  • GPU 利用率波动大（突发请求）                           │    │
│  │  • 多模型版本的资源竞争                                   │    │
│  │  • 延迟敏感（实时推理 < 100ms）                          │    │
│  │  • 模型更新时的零停机切换                                 │    │
│  └──────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────┘
```

### GPU 故障类型与影响

| 故障类型 | 频率 | 影响范围 | 检测方式 | 恢复策略 |
|---------|------|---------|---------|---------|
| ECC 可纠正错误 | 高（日均数次） | 无直接影响 | nvidia-smi / DCGM | 监控趋势，预防性替换 |
| ECC 不可纠正错误 | 中（月均数次） | 单 GPU 计算错误 | XID 48/63/64 | 隔离 GPU，重新调度 |
| GPU 掉卡 | 低（季度级） | 单节点 GPU 不可用 | nvidia-smi 设备缺失 | 节点标记不可调度，Pod 迁移 |
| NVLink 故障 | 低 | 多 GPU 通信降级 | DCGM 诊断 | 降级为 PCIe 通信或隔离 |
| 驱动崩溃 (XID 79) | 中 | 节点所有 GPU 不可用 | 内核日志 / XID | 节点重启，Pod 重新调度 |
| 显存 OOM | 高 | 单任务失败 | CUDA OOM 错误 | 减小 batch size，重新调度 |
| 温度降频 | 中 | 性能下降 20-50% | DCGM 温度指标 | 改善散热，降低负载 |

### 推理服务 SLA 分层

| SLA 等级 | 可用性 | P99 延迟 | 适用场景 | 架构要求 |
|---------|--------|---------|---------|---------|
| Platinum | 99.99% | < 50ms | 实时交易风控、自动驾驶 | 多 AZ + 模型热备 + 硬件冗余 |
| Gold | 99.95% | < 100ms | 在线推荐、搜索排序 | 多副本 + 自动扩缩 + 降级策略 |
| Silver | 99.9% | < 500ms | 内容审核、批量标注 | 多副本 + 队列缓冲 |
| Bronze | 99.5% | < 2s | 离线分析、报表生成 | 单副本 + 重试 |

## 生产部署/实现

### GPU 健康检查与自动恢复

```yaml
# 🟡 中风险：GPU 健康检查配置影响 Pod 调度行为
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: gpu-health-monitor
  namespace: ai-platform
spec:
  selector:
    matchLabels:
      app: gpu-health-monitor
  template:
    metadata:
      labels:
        app: gpu-health-monitor
    spec:
      nodeSelector:
        accelerator: nvidia-gpu
      containers:
      - name: dcgm-exporter
        image: nvcr.io/nvidia/k8s/dcgm-exporter:3.3.5-3.4.1-ubuntu22.04
        ports:
        - containerPort: 9400
          name: metrics
        securityContext:
          privileged: true
        volumeMounts:
        - name: dev
          mountPath: /dev
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 256Mi
      - name: gpu-health-checker
        image: registry.internal/ai-platform/gpu-health-checker:v1.2.0
        env:
        - name: CHECK_INTERVAL
          value: "30s"
        - name: ECC_ERROR_THRESHOLD
          value: "100"
        - name: TEMP_THRESHOLD
          value: "85"
        - name: XID_ERROR_ALERT
          value: "true"
        - name: NODE_NAME
          valueFrom:
            fieldRef:
              fieldPath: spec.nodeName
        securityContext:
          privileged: true
        volumeMounts:
        - name: dev
          mountPath: /dev
        - name: nvidia
          mountPath: /usr/local/nvidia
          readOnly: true
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
      volumes:
      - name: dev
        hostPath:
          path: /dev
      - name: nvidia
        hostPath:
          path: /usr/local/nvidia
      tolerations:
      - key: nvidia.com/gpu
        operator: Exists
        effect: NoSchedule
---
# GPU 故障自动隔离：通过 Node Problem Detector 标记节点
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: gpu-node-problem-detector
  namespace: ai-platform
spec:
  selector:
    matchLabels:
      app: gpu-npd
  template:
    metadata:
      labels:
        app: gpu-npd
    spec:
      nodeSelector:
        accelerator: nvidia-gpu
      serviceAccountName: gpu-npd
      containers:
      - name: node-problem-detector
        image: registry.k8s.io/node-problem-detector/node-problem-detector:v0.8.18
        command:
        - /node-problem-detector
        - --logtostderr
        - --config.custom-plugin-monitor=/config/gpu-custom-plugin-monitor.json
        - --config.system-log-monitor=/config/gpu-kernel-monitor.json
        securityContext:
          privileged: true
        volumeMounts:
        - name: config
          mountPath: /config
        - name: log
          mountPath: /var/log
        - name: kmsg
          mountPath: /dev/kmsg
          readOnly: true
      volumes:
      - name: config
        configMap:
          name: gpu-npd-config
      - name: log
        hostPath:
          path: /var/log
      - name: kmsg
        hostPath:
          path: /dev/kmsg
```

### 推理服务高可用部署

```yaml
# 🟡 中风险：推理服务部署配置影响 SLA
apiVersion: apps/v1
kind: Deployment
metadata:
  name: llm-inference-service
  namespace: ai-serving
  labels:
    app: llm-inference
    model: llama-3-70b
spec:
  replicas: 4
  strategy:
    rollingUpdate:
      maxUnavailable: 0
      maxSurge: 1
  selector:
    matchLabels:
      app: llm-inference
  template:
    metadata:
      labels:
        app: llm-inference
        model: llama-3-70b
    spec:
      # 确保 Pod 分散在不同节点（避免单节点 GPU 故障影响所有副本）
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: llm-inference
            topologyKey: kubernetes.io/hostname
        # 优先调度到 GPU 健康节点
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: nvidia.com/gpu.health
                operator: In
                values:
                - healthy
      containers:
      - name: inference
        image: registry.internal/ai-serving/vllm:0.5.1
        command:
        - python
        - -m
        - vllm.entrypoints.openai.api_server
        - --model=/models/llama-3-70b
        - --tensor-parallel-size=2
        - --max-model-len=8192
        - --gpu-memory-utilization=0.90
        - --disable-log-requests
        ports:
        - containerPort: 8000
          name: http
        resources:
          requests:
            nvidia.com/gpu: 2
            cpu: "8"
            memory: 64Gi
          limits:
            nvidia.com/gpu: 2
            cpu: "16"
            memory: 128Gi
        # 启动探针：模型加载可能需要 5-10 分钟
        startupProbe:
          httpGet:
            path: /health
            port: 8000
          initialDelaySeconds: 30
          periodSeconds: 10
          failureThreshold: 60  # 最多等待 10 分钟
        livenessProbe:
          httpGet:
            path: /health
            port: 8000
          periodSeconds: 15
          failureThreshold: 3
          timeoutSeconds: 10
        readinessProbe:
          httpGet:
            path: /v1/models
            port: 8000
          periodSeconds: 10
          failureThreshold: 2
        volumeMounts:
        - name: model-storage
          mountPath: /models
          readOnly: true
        - name: shm
          mountPath: /dev/shm
      volumes:
      - name: model-storage
        persistentVolumeClaim:
          claimName: model-llama-3-70b-pvc
      - name: shm
        emptyDir:
          medium: Memory
          sizeLimit: 16Gi
      tolerations:
      - key: nvidia.com/gpu
        operator: Exists
        effect: NoSchedule
      # 优雅终止：等待正在处理的请求完成
      terminationGracePeriodSeconds: 120
---
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: llm-inference-hpa
  namespace: ai-serving
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: llm-inference-service
  minReplicas: 4
  maxReplicas: 12
  metrics:
  # 基于 GPU 利用率扩缩
  - type: Pods
    pods:
      metric:
        name: DCGM_FI_DEV_GPU_UTIL
      target:
        type: AverageValue
        averageValue: "70"
  # 基于请求队列深度扩缩
  - type: Pods
    pods:
      metric:
        name: vllm_num_requests_waiting
      target:
        type: AverageValue
        averageValue: "10"
  behavior:
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Pods
        value: 2
        periodSeconds: 120
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Pods
        value: 1
        periodSeconds: 300
```

### 模型降级与 Fallback 策略

```yaml
# 🟡 中风险：降级配置影响推理质量和延迟
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: inference-routing
  namespace: ai-serving
spec:
  hosts:
  - inference.ai-platform.svc.cluster.local
  http:
  # 主路由：大模型（高质量）
  - name: primary-large-model
    match:
    - uri:
        prefix: /v1/chat/completions
      headers:
        x-model-tier:
          exact: premium
    route:
    - destination:
        host: llm-inference-service.ai-serving.svc.cluster.local
        subset: large-model
        port:
          number: 8000
    timeout: 30s
    retries:
      attempts: 1
      perTryTimeout: 25s
      retryOn: "5xx,reset,connect-failure"

  # Fallback 路由：小模型（低延迟，质量略降）
  - name: fallback-small-model
    match:
    - uri:
        prefix: /v1/chat/completions
      headers:
        x-fallback:
          exact: "true"
    route:
    - destination:
        host: llm-inference-small.ai-serving.svc.cluster.local
        subset: small-model
        port:
          number: 8000
    timeout: 10s

  # 最终 Fallback：缓存响应 / 规则引擎
  - name: ultimate-fallback
    match:
    - uri:
        prefix: /v1/chat/completions
      headers:
        x-ultimate-fallback:
          exact: "true"
    directResponse:
      status: 200
      body:
        string: '{"choices":[{"message":{"content":"Service temporarily degraded. Please retry in a few moments."},"finish_reason":"stop"}],"model":"fallback","usage":{"prompt_tokens":0,"completion_tokens":0}}'
---
# DestinationRule：大模型熔断后自动切换到小模型
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: inference-fallback-dr
  namespace: ai-serving
spec:
  host: llm-inference-service.ai-serving.svc.cluster.local
  subsets:
  - name: large-model
    labels:
      model: llama-3-70b
    trafficPolicy:
      outlierDetection:
        consecutive5xxErrors: 3
        interval: 10s
        baseEjectionTime: 60s
        maxEjectionPercent: 50
  - name: small-model
    labels:
      model: llama-3-8b
```

### 分布式训练 Checkpoint 容错

```yaml
# 🔴 高风险：训练任务配置错误可能导致数天计算成果丢失
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: llm-finetune-llama3
  namespace: ai-training
spec:
  # 弹性训练配置：允许 Worker 故障后继续
  elasticPolicy:
    rdzvBackend: c10d
    minReplicas: 6
    maxReplicas: 8
    maxRestarts: 10
    metrics:
    - type: Resource
      resource:
        name: nvidia.com/gpu
        target:
          type: Utilization
          averageUtilization: 80
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      restartPolicy: OnFailure
      template:
        metadata:
          labels:
            training-job: llm-finetune
        spec:
          containers:
          - name: pytorch
            image: registry.internal/ai-training/pytorch-training:2.3.0-cuda12.1
            command:
            - torchrun
            - --nproc_per_node=8
            - --nnodes=8
            - --rdzv_backend=c10d
            - --rdzv_endpoint=llm-finetune-llama3-master:29400
            - --max_restarts=10
            - train.py
            - --model=llama-3-70b
            - --data-path=/data/training-corpus
            - --output-dir=/checkpoints/llama3-finetune
            # Checkpoint 配置
            - --save-interval=500
            - --save-strategy=steps
            - --save-total-limit=5
            - --resume-from-checkpoint=auto
            # 容错配置
            - --heartbeat-timeout=300
            - --max-restarts=10
            resources:
              requests:
                nvidia.com/gpu: 8
                cpu: "32"
                memory: 256Gi
              limits:
                nvidia.com/gpu: 8
                cpu: "64"
                memory: 512Gi
            volumeMounts:
            - name: checkpoint-storage
              mountPath: /checkpoints
            - name: training-data
              mountPath: /data
              readOnly: true
            - name: shm
              mountPath: /dev/shm
            env:
            - name: NCCL_DEBUG
              value: WARN
            - name: NCCL_TIMEOUT
              value: "1800"
            - name: TORCH_DISTRIBUTED_DEBUG
              value: DETAIL
            - name: CHECKPOINT_ASYNC_SAVE
              value: "true"
          volumes:
          - name: checkpoint-storage
            persistentVolumeClaim:
              claimName: training-checkpoint-pvc
          - name: training-data
            persistentVolumeClaim:
              claimName: training-data-pvc
          - name: shm
            emptyDir:
              medium: Memory
              sizeLimit: 64Gi
          tolerations:
          - key: nvidia.com/gpu
            operator: Exists
            effect: NoSchedule
    Worker:
      replicas: 7
      restartPolicy: OnFailure
      template:
        metadata:
          labels:
            training-job: llm-finetune
        spec:
          containers:
          - name: pytorch
            image: registry.internal/ai-training/pytorch-training:2.3.0-cuda12.1
            command:
            - torchrun
            - --nproc_per_node=8
            - --nnodes=8
            - --rdzv_backend=c10d
            - --rdzv_endpoint=llm-finetune-llama3-master:29400
            - --max_restarts=10
            - train.py
            - --model=llama-3-70b
            - --data-path=/data/training-corpus
            - --output-dir=/checkpoints/llama3-finetune
            - --save-interval=500
            - --resume-from-checkpoint=auto
            resources:
              requests:
                nvidia.com/gpu: 8
                cpu: "32"
                memory: 256Gi
              limits:
                nvidia.com/gpu: 8
                cpu: "64"
                memory: 512Gi
            volumeMounts:
            - name: checkpoint-storage
              mountPath: /checkpoints
            - name: training-data
              mountPath: /data
              readOnly: true
            - name: shm
              mountPath: /dev/shm
          volumes:
          - name: checkpoint-storage
            persistentVolumeClaim:
              claimName: training-checkpoint-pvc
          - name: training-data
            persistentVolumeClaim:
              claimName: training-data-pvc
          - name: shm
            emptyDir:
              medium: Memory
              sizeLimit: 64Gi
          tolerations:
          - key: nvidia.com/gpu
            operator: Exists
            effect: NoSchedule
```

## 运维操作

### GPU 状态监控

```bash
# 🟢 低风险：只读监控
# 查看集群 GPU 使用状态
kubectl get nodes -l accelerator=nvidia-gpu -o custom-columns=\
NAME:.metadata.name,GPU_ALLOC:.status.allocatable.nvidia\\.com/gpu,GPU_CAP:.status.capacity.nvidia\\.com/gpu

# 查看 GPU 详细指标（通过 DCGM Exporter）
kubectl port-forward -n ai-platform daemonset/gpu-health-monitor 9400:9400 &
curl -s http://localhost:9400/metrics | grep -E "DCGM_FI_DEV_(GPU_TEMP|GPU_UTIL|MEM_COPY_UTIL|POWER_USAGE|XID_ERRORS)"

# 检查特定节点的 GPU 健康状态
kubectl exec -n ai-platform daemonset/gpu-health-monitor -- \
  nvidia-smi --query-gpu=index,name,temperature.gpu,utilization.gpu,memory.used,memory.total,ecc.errors.uncorrected.volatile.total \
  --format=csv

# 查看 XID 错误（GPU 驱动级错误）
kubectl exec -n ai-platform daemonset/gpu-health-monitor -- \
  dmesg | grep -i "xid\|nvrm\|gpu" | tail -20
```

### 推理服务运维

```bash
# 🟢 低风险：只读诊断
# 查看推理服务状态
kubectl get pods -n ai-serving -l app=llm-inference -o wide
kubectl top pods -n ai-serving -l app=llm-inference

# 查看模型加载状态
kubectl logs -n ai-serving deployment/llm-inference-service --tail=20 | grep -i "model\|loaded\|ready"

# 检查推理延迟和吞吐
curl -s 'http://prometheus-server.monitoring.svc:9090/api/v1/query' \
  --data-urlencode 'query=histogram_quantile(0.99, sum(rate(vllm_e2e_request_latency_seconds_bucket[5m])) by (le))' | \
  jq '.data.result[0].value[1]'

# 查看 GPU 显存使用（接近 OOM 时需要关注）
curl -s 'http://prometheus-server.monitoring.svc:9090/api/v1/query' \
  --data-urlencode 'query=DCGM_FI_DEV_FB_USED / DCGM_FI_DEV_FB_FREE * 100' | \
  jq '.data.result[] | {gpu: .metric.gpu, usage_pct: .value[1]}'
```

### 训练任务管理

```bash
# 🟢 低风险：只读查看
# 查看训练任务状态
kubectl get pytorchjob -n ai-training
kubectl describe pytorchjob llm-finetune-llama3 -n ai-training

# 查看 Checkpoint 保存状态
kubectl exec -n ai-training llm-finetune-llama3-master-0 -- \
  ls -la /checkpoints/llama3-finetune/ | tail -10

# 查看训练进度
kubectl logs -n ai-training llm-finetune-llama3-master-0 --tail=20 | \
  grep -E "step|loss|epoch|checkpoint"

# 🟡 中风险：手动触发 Checkpoint 保存
kubectl exec -n ai-training llm-finetune-llama3-master-0 -- \
  python -c "
import torch.distributed as dist
import os
# 发送信号触发紧急 checkpoint
os.system('kill -USR1 1')
"
```

## 故障排查

### GPU 故障处理

```bash
# 🔴 高风险：隔离节点影响正在运行的任务
# 1. 确认 GPU 故障类型
kubectl exec -n ai-platform daemonset/gpu-health-monitor -- \
  nvidia-smi -q | grep -A5 "ECC Errors\|Retired Pages\|Temperature"

# 2. 查看 XID 错误详情
kubectl exec -n ai-platform daemonset/gpu-health-monitor -- \
  dmesg | grep "Xid" | tail -10
# XID 48: DBE (Double Bit ECC Error) - 需要隔离 GPU
# XID 63: ECC page retirement - GPU 需要替换
# XID 79: GPU fallen off bus - 需要重启节点
# XID 95: ECC page retirement event - 监控即可

# 3. 隔离故障节点（标记为不可调度）
kubectl cordon gpu-node-05

# 4. 驱逐该节点上的 AI 工作负载
kubectl drain gpu-node-05 --ignore-daemonsets --delete-emptydir-data --timeout=300s

# 5. 对于 XID 79（GPU 掉卡），需要重启节点
# kubectl delete node gpu-node-05 (让节点重新注册)
# 或通过 IPMI/BMC 远程重启
```

### 推理服务 OOM 处理

```bash
# 🟡 中风险：调整配置需要重启 Pod
# 检查 OOM 事件
kubectl get events -n ai-serving --field-selector reason=OOMKilling --sort-by='.lastTimestamp'

# 查看 GPU 显存使用详情
kubectl exec -n ai-serving deployment/llm-inference-service -- \
  nvidia-smi --query-gpu=memory.used,memory.total --format=csv

# 临时降低 GPU 显存利用率
kubectl patch deployment llm-inference-service -n ai-serving \
  --type='json' \
  -p='[{"op":"replace","path":"/spec/template/spec/containers/0/command","value":["python","-m","vllm.entrypoints.openai.api_server","--model=/models/llama-3-70b","--tensor-parallel-size=2","--gpu-memory-utilization=0.85","--max-model-len=4096"]}]'
```

### 训练任务 Checkpoint 恢复

```bash
# 🟡 中风险：从 Checkpoint 恢复可能丢失最近的训练进度
# 查看可用的 Checkpoint
kubectl exec -n ai-training llm-finetune-llama3-master-0 -- \
  ls -la /checkpoints/llama3-finetune/

# 验证 Checkpoint 完整性
kubectl exec -n ai-training llm-finetune-llama3-master-0 -- \
  python -c "
import torch
import os
ckpt_dir = '/checkpoints/llama3-finetune/checkpoint-5000'
files = os.listdir(ckpt_dir)
print(f'Checkpoint files: {files}')
# 验证可以加载
state = torch.load(os.path.join(ckpt_dir, 'trainer_state.json'))
print(f'Last step: {state[\"global_step\"]}')
print(f'Best metric: {state.get(\"best_metric\", \"N/A\")}')
"

# 从特定 Checkpoint 恢复训练
# 修改 PyTorchJob 的启动参数：--resume-from-checkpoint=/checkpoints/llama3-finetune/checkpoint-5000
```

## 最佳实践

### GPU 可靠性保障

1. **预防性维护**：监控 ECC 错误趋势，当可纠正错误速率超过阈值时主动替换 GPU，避免不可纠正错误导致训练中断。

2. **节点级隔离**：GPU 故障时立即 cordon 节点，防止新任务调度到故障节点。使用 Node Problem Detector 自动化此流程。

3. **驱动版本锁定**：生产环境锁定 NVIDIA 驱动版本，避免驱动升级引入兼容性问题。

4. **温度监控**：GPU 温度持续 > 80°C 时降低负载，> 90°C 时触发告警。

### 推理服务 SLA 保障

1. **模型预热**：新 Pod 启动后先发送预热请求，确保 KV Cache 和 CUDA Kernel 编译完成后再接收生产流量。

2. **优雅终止**：设置足够的 `terminationGracePeriodSeconds`（120s+），确保正在处理的请求完成。

3. **多模型 Fallback**：大模型不可用时自动切换到小模型，保证服务可用（质量降级但不停服）。

4. **请求队列管理**：设置最大队列深度，超过时快速拒绝（返回 429），避免排队导致延迟雪崩。

### 训练容错设计

1. **Checkpoint 频率**：每 500-1000 步保存一次 Checkpoint，平衡存储成本和恢复粒度。

2. **异步 Checkpoint**：使用异步写入避免 Checkpoint 保存阻塞训练。

3. **Checkpoint 验证**：每次保存后验证文件完整性，保留最近 N 个有效 Checkpoint。

4. **弹性训练**：使用 PyTorch Elastic（torchrun）允许 Worker 动态加入/退出，单 Worker 故障不中断整体训练。

5. **存储可靠性**：Checkpoint 存储使用高可靠 PVC（如 Ceph RBD 3 副本或云盘），避免存储故障导致 Checkpoint 丢失。

### 与现有体系集成

- [[09-可观测性/02-指标/01-prometheus-enterprise-monitoring.md|Prometheus]] 采集 DCGM GPU 指标
- [[09-可观测性/05-告警/01-alertmanager-deep-configuration.md|Alertmanager]] 配置 GPU 故障告警
- [[12-可靠性/06-SRE实践/08-resilience-patterns-circuit-breaker.md|弹性模式]] 应用于推理服务
- [[12-可靠性/06-SRE实践/11-mttr-framework-optimization.md|MTTR 优化]] 框架指导 GPU 故障恢复

## Related

- [[15-AI基础设施/README|AI基础设施]]
- [[12-可靠性/06-SRE实践/08-resilience-patterns-circuit-breaker.md|弹性模式]]
- [[12-可靠性/06-SRE实践/01-availability-calculation-model.md|可用性计算模型]]
- [[09-可观测性/02-指标/01-prometheus-enterprise-monitoring.md|Prometheus 企业级监控]]
- [[09-可观测性/05-告警/07-aiops-intelligent-alerting.md|AIOps 智能告警]]
- [[12-可靠性/06-SRE实践/11-mttr-framework-optimization.md|MTTR 优化框架]]
- [[12-可靠性/06-SRE实践/09-multi-active-architecture.md|多活架构设计]]
