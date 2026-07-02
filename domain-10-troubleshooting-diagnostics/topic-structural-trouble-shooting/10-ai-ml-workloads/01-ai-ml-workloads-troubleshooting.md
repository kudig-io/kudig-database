---
title: AI/ML 工作负载故障排查指南 [topic-structural-trouble-shooting]
description: 'title: AI/ML 工作负载故障排查指南'
summary: 'title: AI/ML 工作负载故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- kubelet
- scheduler
- prometheus
- docker
- hpa
- daemonset
- job
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 35min
intent_queries:
- AI/ML 工作负载故障排查指南 是什么
- 如何 AI/ML 工作负载故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- AI/ML 工作负载故障排查指南 故障排查
- AI/ML 工作负载故障排查指南 排障步骤
trigger_keywords:
- AI
- ML
- 工作负载故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- pod-lifecycle
- troubleshooting-methodology
- prometheus-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: AI/ML 工作负载故障排查指南
description: '# AI/ML 工作负载故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[kubelet|kubelet]]
- [[Prometheus|prometheus]]
- hpa
- [[DaemonSet|daemonset]]
- job
- operator
- gpu
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- AI/ML 工作负载故障排查指南 是什么
- 如何 AI/ML 工作负载故障排查指南
- AI/ML 工作负载故障排查指南 故障排查
- AI/ML 工作负载故障排查指南 排障步骤
trigger_keywords:
- AI
- ML
- 工作负载故障排查指南
- structural
- trouble
- shooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# AI/ML 工作负载故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: AI基础设施运维保障

## 0. 10 分钟快速诊断

1. **GPU 可见性**：`kubectl get nodes -o jsonpath='{.items[*].status.allocatable.nvidia\.com/gpu}'`，确认资源暴露。
2. **设备插件**：检查 Device Plugin DaemonSet 状态与日志。
3. **训练作业**：查看分布式训练 Pod 事件，关注 NCCL/网络报错。
4. **数据与存储**：确认数据集 PVC 挂载、I/O 吞吐与热点。
5. **资源请求**：核对 GPU/CPU/内存 requests/limits，避免碎片化。
6. **快速缓解**：
   - 降低 batch size 或启用混合精度。
   - 调整亲和性/拓扑，让训练 Pod 同机房/同交换机。
7. **证据留存**：保存训练日志、GPU 指标、Pod 事件与拓扑信息。

## 目录

1. [问题现象与影响分析](#问题现象与影响分析)
2. [排查方法与步骤](#排查方法与步骤)
3. [解决方案与风险控制](#解决方案与风险控制)

## 问题现象与影响分析

### AI/ML 特有问题现象

| 问题类型 | 典型现象 | 影响程度 | 紧急级别 |
|---------|---------|---------|---------|
| GPU 资源调度失败 | `0/5 nodes are available: 5 Insufficient nvidia.com/gpu` | ⭐⭐⭐ 高 | P0 |
| 分布式训练通信失败 | `NCCL error: unhandled cuda error` | ⭐⭐⭐ 高 | P0 |
| 模型服务推理超时 | `model inference timeout after 30s` | ⭐⭐ 中 | P1 |
| 数据集加载性能问题 | `dataset loading took 30+ minutes` | ⭐⭐ 中 | P1 |
| GPU 内存不足崩溃 | `CUDA out of memory` | ⭐⭐⭐ 高 | P0 |
| 模型版本管理混乱 | `serving model version mismatch` | ⭐⭐ 中 | P1 |
| 训练任务资源浪费 | `GPU utilization < 20%` | ⭐⭐ 中 | P1 |
| 成本控制失效 | `unexpected GPU billing spike` | ⭐⭐⭐ 高 | P0 |

### AI/ML 工作负载状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# GPU 资源状态检查
echo "=== GPU 资源状态检查 ==="
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.capacity.nvidia\.com/gpu}{"\n"}{end}'

# NVIDIA Device Plugin 状态
echo "=== NVIDIA Device Plugin 状态 ==="
kubectl get pods -n kube-system -l app=nvidia-device-plugin-daemonset

# GPU 利用率监控
echo "=== GPU 利用率检查 ==="
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.allocatable.nvidia\.com/gpu}{" allocated\n"}{end}'

# 分布式训练作业状态
echo "=== 分布式训练作业状态 ==="
kubectl get jobs -l app=distributed-training --all-namespaces
kubectl get pods -l app=distributed-training --all-namespaces -o wide

# 模型服务状态
echo "=== 模型服务状态 ==="
kubectl get services -l app=model-serving --all-namespaces
kubectl get deployments -l app=model-serving --all-namespaces
```
## 排查方法与步骤

### 诊断原理说明

AI/ML 工作负载故障诊断需要考虑以下特殊因素：

1. **硬件加速层面**：GPU/TPU 资源管理、驱动程序兼容性
2. **分布式计算层面**：节点间通信、数据并行、模型并行
3. **框架特异性**：TensorFlow、PyTorch、MXNet 等框架差异
4. **数据管道层面**：数据加载、预处理、缓存机制
5. **性能优化层面**：批处理大小、混合精度、内存优化

### AI/ML 问题诊断决策树

```
AI/ML 工作负载问题
    ├── GPU 资源问题
    │   ├── 设备插件状态
    │   ├── GPU 驱动兼容性
    │   ├── 资源请求配置
    │   └── GPU 内存分配
    ├── 分布式训练问题
    │   ├── NCCL/RDMA 配置
    │   ├── 网络策略限制
    │   ├── 节点亲和性配置
    │   └── 通信超时设置
    ├── 模型服务问题
    │   ├── 推理性能瓶颈
    │   ├── 模型版本管理
    │   ├── 批处理配置
    │   └── 自动扩缩容设置
    └── 数据处理问题
        ├── 数据加载性能
        ├── 存储 I/O 瓶颈
        ├── 缓存策略配置
        └── 数据预处理效率
```

### 详细诊断命令

#### GPU 资源诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# GPU 资源诊断脚本

echo "=== GPU 资源诊断 ==="

# 1. 检查 GPU 硬件状态
echo "1. GPU 硬件状态检查:"
nvidia-smi --query-gpu=name,memory.total,memory.used,utilization.gpu --format=csv

# 2. 检查 NVIDIA Device Plugin
echo "2. NVIDIA Device Plugin 状态:"
kubectl get pods -n kube-system -l app=nvidia-device-plugin-daemonset -o wide

# 检查 Device Plugin 日志
echo "Device Plugin 日志摘要:"
kubectl logs -n kube-system -l app=nvidia-device-plugin-daemonset --tail=50 | grep -i error

# 3. 检查 GPU 资源暴露情况
echo "3. GPU 资源在节点上的暴露情况:"
kubectl get nodes -o json | jq -r '.items[] | "\(.metadata.name): \(.status.capacity["nvidia.com/gpu"] // "0") GPUs"'

# 4. 检查 GPU 资源分配情况
echo "4. GPU 资源分配情况:"
kubectl get pods --all-namespaces -o json | jq -r '.items[] | select(.spec.containers[].resources.requests["nvidia.com/gpu"] != null) | "\(.metadata.namespace)/\(.metadata.name): \(.spec.containers[].resources.requests["nvidia.com/gpu"]) GPUs"'

# 5. GPU 内存使用检查
echo "5. GPU 内存使用检查:"
for node in $(kubectl get nodes -o name | cut -d/ -f2); do
  echo "节点 $node:"
  kubectl debug node/$node -it --image=ubuntu:20.04 -- chroot /host nvidia-smi --query-gpu=memory.used,memory.free --format=csv,noheader,nounits 2>/dev/null || echo "  无法访问节点 GPU 信息"
done
```
#### 分布式训练诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 分布式训练诊断脚本

echo "=== 分布式训练诊断 ==="

# 1. 检查训练作业状态
echo "1. 分布式训练作业状态:"
kubectl get jobs -l training=distributed --all-namespaces
kubectl get pods -l training=distributed --all-namespaces --field-selector=status.phase!=Running

# 2. 检查 NCCL 配置
echo "2. NCCL 环境变量检查:"
kubectl get pods -l training=distributed --all-namespaces -o json | jq -r '.items[].spec.containers[].env[] | select(.name | startswith("NCCL")) | "\(.name)=\(.value)"' | sort | uniq

# 3. 检查网络策略
echo "3. 网络策略对分布式训练的影响:"
kubectl get networkpolicies --all-namespaces | grep -E "(distributed|training|nccl)" || echo "未找到相关的网络策略"

# 4. 检查节点亲和性和拓扑
echo "4. 节点亲和性配置:"
kubectl get pods -l training=distributed --all-namespaces -o json | jq -r '.items[] | "\(.metadata.name): \(.spec.affinity.nodeAffinity.requiredDuringSchedulingIgnoredDuringExecution.nodeSelectorTerms[].matchExpressions[].values[])"'

# 5. 检查 RDMA/高性能网络配置
echo "5. 高性能网络配置检查:"
kubectl get nodes -o json | jq -r '.items[] | "\(.metadata.name): \(.metadata.labels["feature.node.kubernetes.io/network-sriov.capable"] // "unknown") SR-IOV, \(.metadata.labels["feature.node.kubernetes.io/network-rdma.available"] // "unknown") RDMA"'

# 6. 分布式训练日志分析
echo "6. 分布式训练错误日志分析:"
for pod in $(kubectl get pods -l training=distributed --all-namespaces -o name); do
  echo "检查 $pod:"
  kubectl logs $pod --tail=100 2>/dev/null | grep -i -E "(error|exception|nccl|timeout|connection)" | head -5
done
```
#### 模型服务诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 模型服务诊断脚本

echo "=== 模型服务诊断 ==="

# 1. 模型服务部署状态
echo "1. 模型服务部署状态:"
kubectl get deployments -l app=model-serving --all-namespaces
kubectl get services -l app=model-serving --all-namespaces

# 2. 模型推理性能检查
echo "2. 模型推理性能检查:"
for svc in $(kubectl get services -l app=model-serving --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'); do
  namespace=$(echo $svc | cut -d/ -f1)
  service=$(echo $svc | cut -d/ -f2)
  echo "测试服务 $namespace/$service:"
  
  # 简单的健康检查
  kubectl get service $service -n $namespace -o jsonpath='{.spec.clusterIP}:{.spec.ports[0].port}' | xargs -I {} timeout 10 curl -s http://{}/health 2>/dev/null && echo "  ✓ 健康检查通过" || echo "  ✗ 健康检查失败"
done

# 3. 模型版本管理检查
echo "3. 模型版本管理检查:"
kubectl get configmaps -l model-version --all-namespaces -o json | jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.name): \(.data.version // "unspecified")"'

# 4. 推理资源使用情况
echo "4. 推理资源使用情况:"
kubectl top pods -l app=model-serving --all-namespaces

# 5. 模型加载时间检查
echo "5. 模型加载时间检查:"
for pod in $(kubectl get pods -l app=model-serving --all-namespaces -o name); do
  echo "检查 $pod 模型加载时间:"
  kubectl logs $pod --tail=200 2>/dev/null | grep -i "model loaded|loading model|loaded in" | tail -3
done

# 6. 批处理配置检查
echo "6. 批处理配置检查:"
kubectl get deployments -l app=model-serving --all-namespaces -o json | jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.name): batch_size=\(.spec.template.spec.containers[0].env[] | select(.name=="BATCH_SIZE") .value // "default")"'
```
#### 数据处理诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 数据处理诊断脚本

echo "=== 数据处理诊断 ==="

# 1. 存储性能检查
echo "1. 存储性能检查:"
kubectl get pv -o json | jq -r '.items[] | select(.spec.csi.driver=="pd.csi.storage.gke.io" or .spec.csi.driver=="diskplugin.csi.alibabacloud.com") | "\(.metadata.name): \(.spec.csi.volumeHandle)"'

# 检查存储类性能
kubectl get storageclasses -o json | jq -r '.items[] | "\(.metadata.name): \(.parameters.type // .parameters.diskType // "standard")"'

# 2. 数据加载性能检查
echo "2. 数据加载性能检查:"
for pod in $(kubectl get pods -l data-processing=active --all-namespaces -o name); do
  echo "检查 $pod 数据加载性能:"
  kubectl logs $pod --tail=100 2>/dev/null | grep -i -E "(data loading|dataset|prefetch|cache)" | tail -5
done

# 3. 缓存配置检查
echo "3. 缓存配置检查:"
kubectl get pods -l data-processing=active --all-namespaces -o json | jq -r '.items[].spec.containers[].env[] | select(.name | contains("CACHE") or contains("BUFFER")) | "\(.name)=\(.value)"' | sort | uniq

# 4. 存储 I/O 监控
echo "4. 存储 I/O 监控:"
kubectl get pods -l data-processing=active --all-namespaces -o json | jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.name): \(.spec.volumes[]?.persistentVolumeClaim.claimName // "no PVC")"'

# 5. 数据预处理效率检查
echo "5. 数据预处理效率检查:"
for job in $(kubectl get jobs -l data-preprocessing=active --all-namespaces -o name); do
  echo "检查作业 $job:"
  kubectl describe $job | grep -E "(Active|Succeeded|Failed)"
done
```
## 解决方案与风险控制

### GPU 资源问题解决

#### 方案一：NVIDIA Device Plugin 配置优化

```yaml
# 优化的 NVIDIA Device Plugin 配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: nvidia-device-plugin-daemonset
  namespace: kube-system
spec:
  selector:
    matchLabels:
      name: nvidia-device-plugin-ds
  template:
    metadata:
      labels:
        name: nvidia-device-plugin-ds
    spec:
      tolerations:
      - key: nvidia.com/gpu
        operator: Exists
        effect: NoSchedule
      containers:
      - image: nvcr.io/nvidia/k8s-device-plugin:v0.14.0
        name: nvidia-device-plugin-ctr
        args: 
        - "--fail-on-init-error=false"
        - "--device-discovery-strategy=auto"
        - "--device-list-strategy=envvar"
        - "--pass-device-specs=true"
        securityContext:
          allowPrivilegeEscalation: false
          capabilities:
            drop: ["ALL"]
        volumeMounts:
        - name: device-plugin
          mountPath: /var/lib/kubelet/device-plugins
      volumes:
      - name: device-plugin
        hostPath:
          path: /var/lib/kubelet/device-plugins
```

#### 方案二：GPU 资源请求优化

```yaml
# 优化的 GPU 工作负载配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ml-training-job
spec:
  template:
    spec:
      containers:
      - name: training-container
        image: nvidia/cuda:12.0-devel-ubuntu20.04
        resources:
          limits:
            nvidia.com/gpu: "1"
            memory: "16Gi"
            cpu: "8"
          requests:
            nvidia.com/gpu: "1"
            memory: "8Gi"
            cpu: "4"
        env:
        # GPU 内存优化
        - name: TF_FORCE_GPU_ALLOW_GROWTH
          value: "true"
        - name: PYTORCH_CUDA_ALLOC_CONF
          value: "max_split_size_mb:128"
        # 混合精度训练
        - name: NVIDIA_TF32_OVERRIDE
          value: "0"
        - name: TORCH_CUDNN_V8_API_ENABLED
          value: "1"
        volumeMounts:
        - name: nvidia-install-dir-host
          mountPath: /usr/local/nvidia
          readOnly: true
      volumes:
      - name: nvidia-install-dir-host
        hostPath:
          path: /home/kubernetes/bin/nvidia
          type: Directory
```

### 分布式训练问题解决

#### 方案一：NCCL 优化配置

```yaml
# 分布式训练优化配置
apiVersion: batch/v1
kind: Job
metadata:
  name: distributed-training
spec:
  parallelism: 4
  completions: 4
  template:
    spec:
      containers:
      - name: trainer
        image: pytorch/pytorch:2.0-cuda11.7-cudnn8-runtime
        env:
        # NCCL 优化配置
        - name: NCCL_DEBUG
          value: "INFO"
        - name: NCCL_SOCKET_IFNAME
          value: "eth0"
        - name: NCCL_IB_DISABLE
          value: "0"
        - name: NCCL_IB_CUDA_SUPPORT
          value: "1"
        - name: NCCL_NET_GDR_LEVEL
          value: "2"
        - name: NCCL_BUFFSIZE
          value: "8388608"
        - name: NCCL_NSOCKS_PERTHREAD
          value: "4"
        - name: NCCL_SOCKET_NTHREADS
          value: "2"
        # 分布式训练配置
        - name: WORLD_SIZE
          value: "4"
        - name: MASTER_ADDR
          value: "distributed-training-0.distributed-training-headless"
        - name: MASTER_PORT
          value: "12345"
        resources:
          limits:
            nvidia.com/gpu: "1"
          requests:
            nvidia.com/gpu: "1"
```

#### 方案二：节点亲和性配置

```yaml
# 分布式训练节点亲和性配置
apiVersion: batch/v1
kind: Job
metadata:
  name: distributed-training
spec:
  template:
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: distributed-training
            topologyKey: "kubernetes.io/hostname"
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: nvidia.com/gpu.product
                operator: In
                values:
                - "A100-SXM4-80GB"
                - "H100-PCIE-80GB"
              - key: kubernetes.io/arch
                operator: In
                values:
                - "amd64"
      containers:
      - name: trainer
        # ... 其他配置
```

### 模型服务问题解决

#### 方案一：模型服务优化配置

```yaml
# 优化的模型服务配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: model-serving
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: model-server
        image: tensorflow/serving:2.13.0-gpu
        ports:
        - containerPort: 8501
        env:
        # 模型服务器优化
        - name: MODEL_NAME
          value: "resnet50"
        - name: TENSORFLOW_INTER_OP_PARALLELISM
          value: "0"
        - name: TENSORFLOW_INTRA_OP_PARALLELISM
          value: "0"
        - name: OMP_NUM_THREADS
          value: "4"
        - name: BATCH_SIZE
          value: "32"
        - name: MAX_BATCH_SIZE
          value: "64"
        - name: BATCH_TIMEOUT_MICROS
          value: "10000"
        # GPU 优化
        - name: CUDA_VISIBLE_DEVICES
          value: "0"
        - name: TF_FORCE_GPU_ALLOW_GROWTH
          value: "true"
        resources:
          limits:
            nvidia.com/gpu: "1"
            memory: "8Gi"
            cpu: "4"
          requests:
            nvidia.com/gpu: "1"
            memory: "4Gi"
            cpu: "2"
        readinessProbe:
          httpGet:
            path: /v1/models/resnet50
            port: 8501
          initialDelaySeconds: 30
          periodSeconds: 5
        livenessProbe:
          httpGet:
            path: /v1/models/resnet50
            port: 8501
          initialDelaySeconds: 60
          periodSeconds: 10
```

#### 方案二：自动扩缩容配置

```yaml
# 模型服务自动扩缩容配置
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: model-serving-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: model-serving
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  - type: Pods
    pods:
      metric:
        name: model_inference_latency_seconds
      target:
        type: AverageValue
        averageValue: "0.1"
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60
      - type: Pods
        value: 2
        periodSeconds: 60
```

### 数据处理问题解决

#### 方案一：高性能存储配置

```yaml
# 高性能存储配置
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: ml-dataset-pvc
spec:
  accessModes:
  - ReadWriteMany
  storageClassName: fast-ssd
  resources:
    requests:
      storage: 1Ti

---
# 高性能存储类配置
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: pd.csi.storage.gke.io  # GKE 示例
parameters:
  type: pd-ssd
  replication-type: none
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

#### 方案二：数据加载优化配置

```yaml
# 数据加载优化的训练作业配置
apiVersion: batch/v1
kind: Job
metadata:
  name: ml-training-with-optimized-data-loading
spec:
  template:
    spec:
      initContainers:
      - name: dataset-preparation
        image: busybox
        command: ['sh', '-c']
        args:
        - |
          # 预加载数据到本地缓存
          mkdir -p /shared/dataset-cache
          # 这里添加具体的数据预加载逻辑
        volumeMounts:
        - name: shared-data
          mountPath: /shared
      containers:
      - name: trainer
        image: pytorch/pytorch:2.0-cuda11.7-cudnn8-runtime
        env:
        # 数据加载优化
        - name: DATALOADER_NUM_WORKERS
          value: "4"
        - name: DATALOADER_PREFETCH_FACTOR
          value: "2"
        - name: DATALOADER_PIN_MEMORY
          value: "true"
        - name: TORCH_DISTRIBUTED_DEBUG
          value: "DETAIL"
        # 缓存配置
        - name: DATASET_CACHE_DIR
          value: "/shared/dataset-cache"
        volumeMounts:
        - name: shared-data
          mountPath: /shared
        - name: local-ssd
          mountPath: /local-ssd
        resources:
          limits:
            nvidia.com/gpu: "1"
          requests:
            nvidia.com/gpu: "1"
      volumes:
      - name: shared-data
        persistentVolumeClaim:
          claimName: ml-dataset-pvc
      - name: local-ssd
        hostPath:
          path: /mnt/disks/ssd0
          type: Directory
```

### 安全生产风险提示

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| GPU 驱动更新 | ⭐⭐⭐ 高 | 可能导致节点不可用 | 使用 DaemonSet 滚动更新 |
| 分布式训练配置调整 | ⭐⭐ 中 | 可能影响训练收敛性 | 保留原配置作为备份 |
| 模型服务扩缩容策略调整 | ⭐⭐ 中 | 可能影响服务质量 | 监控指标并及时调整 |
| 存储性能优化 | ⭐⭐ 中 | 可能增加存储成本 | 逐步测试并监控成本 |

### 验证与监控

### AI/ML 工作负载验证脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# AI/ML 工作负载验证脚本

echo "=== AI/ML 工作负载验证 ==="

# 1. GPU 资源验证
echo "1. GPU 资源验证:"
GPU_NODES=$(kubectl get nodes -o jsonpath='{.items[?(@.status.capacity.nvidia\.com/gpu)].metadata.name}')
if [ -n "$GPU_NODES" ]; then
  echo "✓ 发现 GPU 节点: $GPU_NODES"
  
  # 验证 GPU 可用性
  for node in $GPU_NODES; do
    GPU_COUNT=$(kubectl get node $node -o jsonpath='{.status.capacity.nvidia\.com/gpu}')
    echo "  节点 $node: $GPU_COUNT 个 GPU"
  done
else
  echo "❌ 未发现 GPU 节点"
fi

# 2. 分布式训练验证
echo "2. 分布式训练验证:"
TEST_JOB_NAME="test-distributed-training-$(date +%s)"
cat << EOF | kubectl apply -f -
apiVersion: batch/v1
kind: Job
metadata:
  name: $TEST_JOB_NAME
spec:
  parallelism: 2
  completions: 2
  template:
    spec:
      restartPolicy: Never
      containers:
      - name: nccl-test
        image: nvcr.io/nvidia/nccl-test:2.14.3-cuda11.7
        command: ["all_reduce_perf"]
        args: ["-b", "8", "-e", "128M", "-f", "2", "-g", "1"]
        resources:
          limits:
            nvidia.com/gpu: "1"
          requests:
            nvidia.com/gpu: "1"
        env:
        - name: NCCL_DEBUG
          value: "INFO"
EOF

echo "已创建测试作业: $TEST_JOB_NAME"
echo "等待测试完成..."

# 等待测试完成
sleep 60

JOB_STATUS=$(kubectl get job $TEST_JOB_NAME -o jsonpath='{.status.conditions[?(@.type=="Complete")].status}' 2>/dev/null)
if [ "$JOB_STATUS" = "True" ]; then
  echo "✓ 分布式训练测试通过"
else
  echo "⚠ 分布式训练测试可能存在问题"
  kubectl logs job/$TEST_JOB_NAME --tail=20
fi

# 清理测试资源
kubectl delete job $TEST_JOB_NAME

# 3. 模型服务验证
echo "3. 模型服务验证:"
# 这里可以添加具体的模型服务验证逻辑

# 4. 数据处理验证
echo "4. 数据处理验证:"
# 这里可以添加具体的数据处理验证逻辑

echo "AI/ML 工作负载验证完成！"
```
### AI/ML 监控告警配置

```yaml
# Prometheus AI/ML 监控告警
groups:
- name: ai-ml-workloads
  rules:
  - alert: GPUNotAvailable
    expr: kube_node_status_capacity{resource="nvidia_com_gpu"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "GPU 资源不可用"
      description: "节点 {{ $labels.node }} 上没有可用的 GPU 资源"

  - alert: LowGPUUtilization
    expr: avg(rate(DCGM_FI_DEV_GPU_UTIL[5m])) < 20
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "GPU 利用率过低"
      description: "GPU 平均利用率低于 20%，可能存在资源浪费"

  - alert: DistributedTrainingFailure
    expr: kube_job_status_failed{job_name=~"distributed-training.*"} > 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "分布式训练失败"
      description: "分布式训练作业 {{ $labels.job_name }} 失败"

  - alert: ModelInferenceTimeout
    expr: histogram_quantile(0.99, rate(model_inference_duration_seconds_bucket[5m])) > 30
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "模型推理超时"
      description: "99% 的模型推理请求耗时超过 30 秒"

  - alert: DatasetLoadingSlow
    expr: rate(dataset_loading_duration_seconds_sum[5m]) / rate(dataset_loading_duration_seconds_count[5m]) > 1800
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "数据集加载缓慢"
      description: "平均数据集加载时间超过 30 分钟"

  - alert: HighGPUMemoryUsage
    expr: DCGM_FI_DEV_FB_USED / DCGM_FI_DEV_FB_TOTAL * 100 > 90
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "GPU 内存使用率过高"
      description: "GPU 内存使用率超过 90%"

  - alert: UnexpectedGPUCost
    expr: rate(container_accelerator_allocation_cost_usd_per_hour[1h]) > 10
    for: 1h
    labels:
      severity: warning
    annotations:
      summary: "异常 GPU 成本"
      description: "GPU 使用成本异常升高，当前小时费用超过 $10"
```

### 最佳实践与成本优化

### AI/ML 资源管理配置

```yaml
# AI/ML 资源管理最佳实践配置
aiMlBestPractices:
  gpuManagement:
    devicePlugin:
      image: nvcr.io/nvidia/k8s-device-plugin:v0.14.0
      resources:
        limits:
          memory: "128Mi"
          cpu: "100m"
        requests:
          memory: "64Mi"
          cpu: "50m"
    
    resourceQuotas:
      - name: ai-team-gpu-quota
        namespace: ai-team
        limits:
          "requests.nvidia.com/gpu": "32"
          "limits.nvidia.com/gpu": "32"
    
    priorityClasses:
      - name: high-priority-ml
        value: 1000000
        globalDefault: false
        description: "高优先级 ML 工作负载"
  
  distributedTraining:
    frameworks:
      pytorch:
        image: pytorch/pytorch:2.0-cuda11.7-cudnn8-runtime
        ncclSettings:
          socketInterface: eth0
          ibSupport: true
          bufferSize: "8M"
      
      tensorflow:
        image: tensorflow/tensorflow:2.13.0-gpu
        collectiveSettings:
          implementation: nccl
          timeout: "1800"
    
    networkRequirements:
      bandwidth: "10Gbps"
      latency: "<1ms"
      rdma: enabled
  
  modelServing:
    optimization:
      batching:
        enabled: true
        maxSize: 64
        timeout: "10ms"
      
      autoscaling:
        minReplicas: 2
        maxReplicas: 20
        metrics:
          - type: cpu
            target: 70%
          - type: memory
            target: 80%
          - type: custom
            metricName: inference_latency
            target: "100ms"
    
    monitoring:
      metrics:
        - inference_requests_total
        - inference_duration_seconds
        - model_loading_duration_seconds
        - gpu_utilization
```

### AI/ML 成本优化策略

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# AI/ML 成本优化脚本

COST_REPORT="/var/log/kubernetes/ml-cost-optimization-$(date +%Y%m%d).log"

{
  echo "=== AI/ML 成本优化报告 $(date) ==="
  
  # 1. GPU 资源利用率分析
  echo "1. GPU 资源利用率分析:"
  kubectl get nodes -o json | jq -r '
    .items[] | 
    select(.status.capacity."nvidia.com/gpu") |
    "\(.metadata.name): \(.status.capacity."nvidia.com/gpu") GPUs, allocatable: \(.status.allocatable."nvidia.com/gpu")"
  '
  
  # 2. 工作负载资源请求分析
  echo "2. 工作负载资源请求分析:"
  kubectl get pods --all-namespaces -o json | jq -r '
    .items[] |
    select(.spec.containers[].resources.requests."nvidia.com/gpu") |
    "\(.metadata.namespace)/\(.metadata.name): requested \(.spec.containers[].resources.requests."nvidia.com/gpu") GPUs"
  ' | head -10
  
  # 3. Spot 实例使用建议
  echo "3. Spot 实例使用建议:"
  # 分析可迁移到 Spot 实例的工作负载
  kubectl get pods --all-namespaces -o json | jq -r '
    .items[] |
    select(
      .spec.containers[].resources.requests."nvidia.com/gpu" and
      (.metadata.labels.training != "production" or .metadata.labels.tier != "production")
    ) |
    "\(.metadata.namespace)/\(.metadata.name)"
  ' | head -5
  
  # 4. 自动扩缩容优化建议
  echo "4. 自动扩缩容优化建议:"
  kubectl get hpa --all-namespaces -o json | jq -r '
    .items[] |
    "\(.metadata.namespace)/\(.metadata.name): min=\(.spec.minReplicas), max=\(.spec.maxReplicas)"
  '
  
} >> "$COST_REPORT"

echo "成本优化报告已生成: $COST_REPORT"
```
### 典型问题案例

### 案例一：分布式训练 NCCL 错误

**问题描述**：PyTorch 分布式训练作业频繁出现 NCCL 通信错误，训练速度极慢。

**根本原因**：网络策略阻止了节点间的 RDMA 通信，NCCL 回退到 TCP 模式。

**解决方案**：
1. 更新网络策略允许 RDMA 流量
2. 配置 NCCL 使用正确的网络接口
3. 调整 NCCL 缓冲区大小和线程数

### 案例二：GPU 内存碎片化导致 OOM

**问题描述**：长时间运行的训练任务出现周期性的 CUDA Out of Memory 错误。

**根本原因**：PyTorch 内存分配器产生碎片，即使总内存足够也会出现 OOM。

**解决方案**：
1. 启用内存池和碎片整理
2. 调整批处理大小和梯度累积
3. 使用混合精度训练减少内存使用

### 支持资源与参考文档

**框架官方文档**：
- PyTorch: https://pytorch.org/tutorials/beginner/dist_overview.html
- TensorFlow: https://www.tensorflow.org/guide/distributed_training
- NVIDIA: https://docs.nvidia.com/datacenter/cloud-native/kubernetes/install-k8s.html

**社区支持**：
- Kubernetes AI/ML SIG: https://github.com/kubernetes/community/tree/master/sig-ai
- Kubeflow 社区: https://www.kubeflow.org/
- NVIDIA 开发者论坛: https://forums.developer.nvidia.com/

**相关文档**：
| 文档类型 | 路径 | 说明 |
|----------|------|------|
| FTA | `domain-10-troubleshooting-diagnostics/topic-fta/list/gpu-fta.md` | GPU 工作负载故障树 |
| Structural | `domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/02-kubeflow-troubleshooting.md` | Kubeflow 故障排查 |
| Structural | `domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/03-mpi-operator-troubleshooting.md` | MPI Operator 故障排查 |

---

## 7. 多租户 GPU 调度

### 7.1 GPU 资源共享策略

```yaml
gpu_sharing_strategy:
  # Time-slicing (粗粒度共享)
  time_slicing:
    enabled: true
    replicas: 4  # 每个 GPU 被 4 个容器共享
    method: "时间片轮转"
    use_case: "开发/测试环境"

  # MPS (多进程服务)
  mps:
    enabled: false
    compute_percentage: 50
    use_case: "需要低延迟的生产服务"

  # GPU 隔离 (独占)
  isolation:
    enabled: true
    exclusive_mode: "production"
    shared_mode: "non-production"

  # 选择策略
  node_selector:
    gpu_type: "nvidia-tesla-v100"  # 可选 GPU 型号
    min_memory_free: "8Gi"
    min_memory_free: "8Gi"
```

### 7.2 多租户配额管理

```yaml
multi_tenant_gpu_quota:
  # 租户配额配置
  quotas:
    - tenant: "team-ml"
      namespace: "ml-team"
      gpu_limit: 16
      gpu_request: 8
      priority: "high"

    - tenant: "team-data"
      namespace: "data-team"
      gpu_limit: 8
      gpu_request: 4
      priority: "medium"

    - tenant: "team-dev"
      namespace: "dev-team"
      gpu_limit: 4
      gpu_request: 2
      priority: "low"

  # 配额实施
  enforcement:
    method: "LimitRange + ResourceQuota"
    over_quota_action: "Reject Pod creation"
    grace_period: 10m  # 超配额宽限期
```

### 7.3 GPU 拓扑感知调度

```yaml
gpu_topology_aware:
  # NVLink/NVSwitch 拓扑感知
  enabled: true
  topology_hints:
    - topology: "full"
      description: "所有 GPU 在同一 NVSwitch"
      latency: "< 1us"
    - topology: "partial"
      description: "部分 GPU 通过 NVLink 连接"
      latency: "< 5us"
    - topology: "socket"
      description: "GPU 跨 socket"
      latency: "< 10us"

  # 调度器配置
  scheduler:
    plugin: "gpu-topology-aware-scheduler"
    prioritize_topology: "full > partial > socket"
    fallback_enabled: true
```

---

## 8. 成本归因与优化

### 8.1 GPU 成本归因模型

```yaml
gpu_cost_attribution:
  # 成本计算公式
  calculation:
    cost_per_gpu_hour: 3.50  # USD/GPU/小时 (示例价格)
    cost_per_gb_hour: 0.02   # USD/GB/小时
    cost_per_core_hour: 0.05 # USD/vCPU/小时

  # 归因维度
  attribution_dimensions:
    - dimension: "team"
      aggregation: "sum(cost_by_gpu * days)"
    - dimension: "project"
      aggregation: "sum(cost_by_gpu * project_tag)"
    - dimension: "pipeline"
      aggregation: "sum(cost_by_gpu * pipeline_tag)"
    - dimension: "workload_type"
      values: ["training", "inference", "experiment"]

  # 报告生成
  reports:
    daily:
      output: "gs://ml-cost-reports/daily/{date}.csv"
      recipients: ["#ml-platform", "ml-lead"]
    monthly:
      output: "gs://ml-cost-reports/monthly/{YYYY-MM}.csv"
      recipients: ["#finance", "ml-lead"]
```

### 8.2 成本优化策略

```yaml
cost_optimization:
  # Spot/Preemptible 实例使用
  spot_instances:
    enabled: true
    acceptable_interruption_rate: 0.05
    fallback_to_on_demand: true
    workloads:
      - type: "distributed-training"
        checkpoint_frequency: 5m
      - type: "batch-inference"
        restartable: true

  # 训练成本优化
  training_optimization:
    # 早停策略
    early_stopping:
      enabled: true
      monitor: "validation_loss"
      patience: 5
      min_delta: 0.01

    # 检查点策略
    checkpointing:
      frequency: "5m"
      storage: "persistent storage"
      resume_on_interruption: true

    # 混合精度训练
    mixed_precision:
      enabled: true
      backend: "apex"  # 或 "torch.cuda.amp"
      loss_scale: "dynamic"

  # 推理成本优化
  inference_optimization:
    # 自动扩缩容
    autoscaling:
      min_replicas: 1
      max_replicas: 10
      target_gpu_utilization: 70

    # 模型量化
    quantization:
      enabled: true
      method: "int8"
      accuracy_threshold: 0.99
```

### 8.3 成本监控与告警

```yaml
cost_monitoring:
  # 每日成本阈值
  daily_budgets:
    team-ml: 500  # USD/天
    team-data: 200
    team-dev: 50

  # 告警规则
  alerts:
    - name: "Daily Budget Exceeded"
      severity: P2
      condition: "daily_cost > team_budget * 1.1"
      channels: ["slack-ml-cost", "pagerduty:oncall"]

    - name: "Anomalous GPU Usage"
      severity: P3
      condition: "gpu_hours_today > avg_daily_gpu_hours * 2"
      channels: ["slack-ml-cost"]

    - name: "Idle GPU Waste"
      severity: P3
      condition: "gpu_idle_hours > 10"
      channels: ["slack-ml-platform"]
```

---

## 9. 生产 ML Pipeline 可靠性

### 9.1 Pipeline 容错设计

```yaml
pipeline_reliability:
  # 幂等设计
  idempotency:
    enabled: true
    deduplication:
      method: "exactly-once delivery"
      window: 24h
      key: "pipeline_run_id + step_id"

  # 重试策略
  retry:
    max_attempts: 3
    backoff: "exponential"
    initial_interval: 10s
    max_interval: 10m
    retry_on:
      - "transient_error"
      - "network_timeout"
      - "resource_busy"

  # 检查点与恢复
  checkpoint:
    frequency: "every_step"
    storage: "distributed filesystem"
    resume_from: "last_checkpoint"
```

### 9.2 数据质量保障

```yaml
data_quality:
  # 输入验证
  input_validation:
    - check: "data_schema"
      method: "pandera / Great Expectations"
      fail_on_error: true

    - check: "data_range"
      method: "min/max/NaN detection"
      fail_on_error: false
      warn_on_anomaly: true

    - check: "data_lineage"
      method: "追踪数据血缘"
      audit_trail: true

  # 输出验证
  output_validation:
    - check: "model_evaluation_metrics"
      thresholds:
        accuracy: "> 0.85"
        latency_p99: "< 100ms"
      fail_on_error: true

    - check: "model_format"
      format: "ONNX / TensorRT"
      validation: "model loading test"
      fail_on_error: true
```

### 9.3 监控与告警

```yaml
pipeline_monitoring:
  # 训练指标
  training_metrics:
    - metric: "train_loss"
      type: "gauge"
      alert_on_stagnation: true
      alert_on_increase: true

    - metric: "validation_accuracy"
      type: "gauge"
      alert_on_decrease: true

    - metric: "gpu_utilization"
      type: "gauge"
      alert_if_below: 50

  # 推理指标
  inference_metrics:
    - metric: "inference_latency_p99"
      type: "histogram"
      slo: "< 100ms"

    - metric: "request_success_rate"
      type: "gauge"
      slo: "> 99.9%"

    - metric: "model_version_mismatch"
      type: "counter"
      alert_threshold: "> 0"

  # 告警规则
  alert_rules:
    - name: "Training Diverged"
      severity: P1
      condition: "train_loss > previous_loss * 10"
      channels: ["pagerduty:ml-oncall", "slack-ml-alerts"]

    - name: "Inference Latency High"
      severity: P1
      condition: "histogram_quantile(0.99, inference_latency) > 0.1"
      channels: ["pagerduty:ml-oncall", "slack-ml-alerts"]

    - name: "GPU Utilization Low"
      severity: P3
      condition: "avg(gpu_utilization) < 30 for 1h"
      channels: ["slack-ml-platform"]
```

---

## 10. ML Infrastructure 安全

### 10.1 访问控制

```yaml
access_control:
  # RBAC 配置
  rbac:
    roles:
      - name: "ml-developer"
        permissions:
          - "create/read/update training jobs in own namespace"
          - "read models in shared model registry"
      - name: "ml-admin"
        permissions:
          - "* on all ml resources"
          - "manage quota"

  # 服务账户管理
  service_accounts:
    training:
      name: "ml-training"
      image_pull_secrets: "ml-registry-secret"
      env_vars_protected:
        - "AWS_SECRET_ACCESS_KEY"
        - "MLFLOW_TRACKING_URI"

    inference:
      name: "ml-inference"
      readonly_rootfs: true
      capabilities_drop:
        - "ALL"
```

### 10.2 数据安全

```yaml
data_security:
  # 训练数据保护
  training_data:
    encryption:
      at_rest: "AES-256"
      in_transit: "TLS 1.3"
    access_control:
      model: "RBAC"
      audit_logging: true

  # 模型保护
  model:
    storage:
      encryption: true
      replication: 3
    access_control:
      model: "RBAC + MAC"
    export_control:
      restricted: ["model_weights", "training_data"]
```

### 10.3 审计日志

```yaml
audit_logging:
  # 记录的操作
  operations:
    - "training_job_created"
    - "training_job_completed"
    - "model_uploaded"
    - "model_downloaded"
    - "inference_request"
    - "quota_changed"

  # 日志格式
  log_format:
    timestamp: ISO8601
    operation: string
    actor: string
    resource: string
    result: string
    metadata: object

  # 告警规则
  security_alerts:
    - name: "Unauthorized Model Access"
      severity: P1
      condition: "model_downloaded_by_unauthorized_user"
      channels: ["slack-security"]
```

---

## 11. 容量规划与扩展

### 11.1 容量规划模型

```yaml
capacity_planning:
  # 当前容量
  current_capacity:
    total_gpu: 64
    gpu_type: "nvidia-tesla-v100"
    total_memory_tb: 0.5
    max_concurrent_training_jobs: 32

  # 增长预测
  growth_prediction:
    model: "linear_regression"
    monthly_growth_rate: 0.15
    prediction_horizon: 6m

  # 容量需求计算
  demand_calculation:
    training:
      avg_gpu_per_job: 8
      max_concurrent_jobs: 8
      total_demand: 64
    inference:
      avg_gpu_per_instance: 1
      max_instances: 32
      total_demand: 32
    total_demand: 96
    headroom: 1.2  # 20% 缓冲
    recommended_capacity: 115
```

### 11.2 扩展策略

```yaml
scaling_strategy:
  # 水平扩展
  horizontal:
    gpu_nodes:
      min: 4
      max: 32
      scale_up_threshold: 80
      scale_down_threshold: 30
      stabilization_window: 5m

  # 垂直扩展
  vertical:
    gpu_upgrade_path:
      - from: "v100"
        to: "a100"
        trigger: "utilization > 90% for 7d"
      - from: "a100"
        to: "h100"
        trigger: "utilization > 90% for 7d"

  # 多集群扩展
  multi_cluster:
    enabled: true
    clusters:
      - name: "ml-prod-us"
        region: "us-west-2"
        priority: 1
      - name: "ml-prod-eu"
        region: "eu-west-1"
        priority: 2
    failover:
      enabled: true
      rto: 30m
```

---

## 12. 生产问题案例库

### 12.1 高频问题速查

| 问题 | 快速诊断 | 解决方案 |
|------|----------|----------|
| GPU OOM | nvidia-smi 显示 memory > 95% | 减少 batch size 或启用梯度累积 |
| NCCL 超时 | Pod 日志显示 NCCL timeout | 检查网络策略和节点亲和性 |
| 训练loss发散 | 监控显示 loss > 10x 上一步 | 检查学习率、数据归一化 |
| 推理超时 | 服务日志显示 timeout | 增加副本数或优化模型 |
| GPU利用率低 | nvidia-smi 显示 < 30% | 增加 batch size 或 worker 数 |

### 12.2 疑难问题深度分析

```yaml
# 问题: 分布式训练 GPU 利用率不均匀
symptom: "部分 GPU 利用率 100%，其他仅 30%"
root_cause: |
  数据加载器预取不均匀，或 NCCL 通信原语等待导致。
  常见于数据倾斜或批处理大小配置不当。
diagnosis:
  - "kubectl exec 进入每个训练 Pod，执行 nvidia-smi"
  - "检查每个 Pod 的数据加载日志"
  - "查看 NCCL 通信时间统计"
solution: |
  1. 启用分布式数据加载 (DistributedSampler)
  2. 调整 world_size 与 GPU 数量匹配
  3. 增加 num_workers 和预取因子
  4. 使用 GPU 拓扑感知调度
verification: |
  所有 GPU 利用率差异 < 10%

# 问题: Spot 实例中断导致训练失败
symptom: "训练作业不定期失败，日志显示执行节点被回收"
root_cause: |
  使用 Spot 实例但未配置检查点或中断处理。
  AWS/GCP/Azure 会随时回收 Spot 实例。
diagnosis:
  - "检查 Pod 事件: kubectl describe pod | grep -i 'spot|preempted'"
  - "查看云厂商中断通知"
solution: |
  1. 配置训练框架检查点 (every 5 min)
  2. 使用 training-operator 的 fault-tolerance 功能
  3. 配置中断信号处理器
  4. 考虑使用保活实例 + Spot 的混合策略
verification: |
  Spot 中断后训练可自动恢复，无需人工介入
```

---

> **版本**: v2.0
> **维护团队**: ML Platform Team / SRE Team
> **更新日期**: 2026-05-19
> **新增章节**:
> - [x] 多租户 GPU 调度 (资源共享策略、配额管理、拓扑感知)
> - [x] 成本归因与优化 (成本模型、Spot 实例、监控告警)
> - [x] 生产 ML Pipeline 可靠性 (容错、数据质量、监控)
> - [x] ML Infrastructure 安全 (RBAC、数据安全、审计)
> - [x] 容量规划与扩展 (规划模型、扩展策略)
> - [x] 生产问题案例库 (高频问题、疑难问题)

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[scripts/man/INSTALL.md|INSTALL]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-19-landscape-references/topic-index/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/02-kubeflow-troubleshooting.md|02-kubeflow-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/03-mpi-operator-troubleshooting.md|03-mpi-operator-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/02-kubeflow-troubleshooting.md|02-kubeflow-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/03-mpi-operator-troubleshooting.md|03-mpi-operator-troubleshooting]]


<!-- risk-assessed -->
