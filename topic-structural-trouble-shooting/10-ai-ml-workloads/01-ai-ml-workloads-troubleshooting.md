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

### AI/ML 特有故障现象

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

```bash
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
AI/ML 工作负载故障
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

```bash
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

```bash
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

```bash
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
  kubectl logs $pod --tail=200 2>/dev/null | grep -i "model loaded\|loading model\|loaded in" | tail -3
done

# 6. 批处理配置检查
echo "6. 批处理配置检查:"
kubectl get deployments -l app=model-serving --all-namespaces -o json | jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.name): batch_size=\(.spec.template.spec.containers[0].env[] | select(.name=="BATCH_SIZE") .value // "default")"'
```

#### 数据处理诊断

```bash
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

```bash
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

```bash
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

### 典型故障案例

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