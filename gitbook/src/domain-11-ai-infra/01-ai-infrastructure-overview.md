# AI基础设施架构

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [NVIDIA AI Enterprise](https://www.nvidia.com/en-us/data-center/products/ai-enterprise/) | [Kubeflow](https://www.kubeflow.org/)

## AI Infra 全景架构

```
┌─────────────────────────────────────────────────────────────────┐
│                     AI平台控制平面                               │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  Kubernetes Control Plane (API Server/Scheduler/etcd)   │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  AI调度层: Volcano / Kueue / YuniKorn                    │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              v
┌─────────────────────────────────────────────────────────────────┐
│                     计算资源层                                    │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │  GPU集群     │  │  NPU集群     │  │  RDMA网络    │         │
│  │  A100/H100   │  │  昇腾910B    │  │  InfiniBand  │         │
│  │  (节点池)    │  │  (节点池)    │  │  RoCE        │         │
│  └──────────────┘  └──────────────┘  └──────────────┘         │
└─────────────────────────────────────────────────────────────────┘
                              │
                              v
┌─────────────────────────────────────────────────────────────────┐
│                     AI工作负载编排层                              │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐               │
│  │ 训练框架   │  │ 推理引擎   │  │ 数据处理   │               │
│  │ PyTorch    │  │ vLLM       │  │ Ray        │               │
│  │ DeepSpeed  │  │ TensorRT   │  │ Spark      │               │
│  │ Megatron   │  │ Triton     │  │ Flink      │               │
│  └────────────┘  └────────────┘  └────────────┘               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              v
┌─────────────────────────────────────────────────────────────────┐
│                     存储与数据层                                  │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐               │
│  │ 对象存储   │  │ 向量数据库 │  │ 特征存储   │               │
│  │ S3/OSS     │  │ Milvus     │  │ Feast      │               │
│  │ (模型/数据)│  │ Weaviate   │  │ Tecton     │               │
│  └────────────┘  └────────────┘  └────────────┘               │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐               │
│  │ 分布式存储 │  │ 缓存层     │  │ 数据湖     │               │
│  │ JuiceFS    │  │ Alluxio    │  │ Iceberg    │               │
│  │ CephFS     │  │ Fluid      │  │ Hudi       │               │
│  └────────────┘  └────────────┘  └────────────┘               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              v
┌─────────────────────────────────────────────────────────────────┐
│                     可观测性与治理层                              │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐               │
│  │ 实验跟踪   │  │ 模型管理   │  │ 数据血缘   │               │
│  │ MLflow     │  │ ModelMesh  │  │ DataHub    │               │
│  │ W&B        │  │ Seldon     │  │ Amundsen   │               │
│  └────────────┘  └────────────┘  └────────────┘               │
└─────────────────────────────────────────────────────────────────┘
```

---

## 一、AI专用调度器对比

### 调度器选型矩阵

| 调度器 | Gang调度 | 队列管理 | 优先级抢占 | GPU拓扑感知 | 成熟度 | 生产推荐 |
|-------|---------|---------|-----------|------------|--------|---------|
| **Volcano** | ✅ | ✅ | ✅ | ✅ | ⭐⭐⭐⭐⭐ | 强烈推荐 |
| **Kueue** | ✅ | ✅ | ✅ | ⚠️ 部分 | ⭐⭐⭐⭐ | 推荐 |
| **YuniKorn** | ✅ | ✅ | ✅ | ❌ | ⭐⭐⭐ | 特定场景 |
| **原生K8s Scheduler** | ❌ | ❌ | ✅ | ❌ | ⭐⭐⭐⭐⭐ | 不推荐AI |

---

### 1. Volcano - AI专用调度器

#### 核心特性

**Gang调度**
- 保证分布式训练任务Pod同时调度
- 避免资源死锁和部分失败
- 支持最小成员数配置

**队列管理**
- 多租户资源配额
- 优先级队列
- 公平调度策略

**GPU拓扑感知**
- NVLink拓扑优化
- PCIe亲和性调度
- NUMA感知

#### Helm安装

```bash
helm repo add volcano-sh https://volcano-sh.github.io/helm-charts
helm install volcano volcano-sh/volcano \
  --namespace volcano-system \
  --create-namespace \
  --set basic.image_tag_version=v1.8.2
```

#### Queue配置

```yaml
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: ai-training
spec:
  # 资源配额
  capability:
    cpu: "1000"
    memory: 2Ti
    nvidia.com/gpu: "64"
  
  # 权重(相对优先级)
  weight: 100
  
  # 资源保障(guaranteed资源)
  guarantee:
    cpu: "500"
    memory: 1Ti
    nvidia.com/gpu: "32"
  
  # 队列状态
  state: Open
  
  # 回收策略
  reclaimable: true
---
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: ai-inference
spec:
  capability:
    cpu: "500"
    memory: 1Ti
    nvidia.com/gpu: "32"
  weight: 80
  guarantee:
    cpu: "200"
    memory: 512Gi
    nvidia.com/gpu: "16"
  state: Open
```

#### PyTorchJob Gang调度

```yaml
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: distributed-training
  namespace: ai-training
spec:
  # Volcano调度器
  schedulerName: volcano
  
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      template:
        metadata:
          annotations:
            # Gang调度配置
            scheduling.volcano.sh/group-name: distributed-training
            scheduling.volcano.sh/queue-name: ai-training
        spec:
          containers:
            - name: pytorch
              image: pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime
              command:
                - python
                - -m
                - torch.distributed.launch
                - --nproc_per_node=8
                - train.py
              resources:
                limits:
                  nvidia.com/gpu: 8
                requests:
                  nvidia.com/gpu: 8
          # GPU拓扑亲和性
          affinity:
            nodeAffinity:
              requiredDuringSchedulingIgnoredDuringExecution:
                nodeSelectorTerms:
                  - matchExpressions:
                      - key: nvidia.com/gpu.product
                        operator: In
                        values: ["NVIDIA-A100-SXM4-80GB"]
    
    Worker:
      replicas: 7
      template:
        metadata:
          annotations:
            scheduling.volcano.sh/group-name: distributed-training
            scheduling.volcano.sh/queue-name: ai-training
        spec:
          containers:
            - name: pytorch
              image: pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime
              command:
                - python
                - -m
                - torch.distributed.launch
                - --nproc_per_node=8
                - train.py
              resources:
                limits:
                  nvidia.com/gpu: 8
                requests:
                  nvidia.com/gpu: 8
```

---

### 2. Kueue - K8s原生批处理调度

#### 架构优势

- K8s原生CRD，无需额外组件
- 与K8s调度器深度集成
- 支持多种工作负载(Job/PyTorchJob/RayJob)

#### ClusterQueue配置

```yaml
apiVersion: kueue.x-k8s.io/v1beta1
kind: ResourceFlavor
metadata:
  name: gpu-a100
spec:
  nodeLabels:
    nvidia.com/gpu.product: NVIDIA-A100-SXM4-80GB
---
apiVersion: kueue.x-k8s.io/v1beta1
kind: ClusterQueue
metadata:
  name: cluster-queue-training
spec:
  namespaceSelector: {}
  
  # 资源配额
  resourceGroups:
    - coveredResources: ["cpu", "memory", "nvidia.com/gpu"]
      flavors:
        - name: gpu-a100
          resources:
            - name: cpu
              nominalQuota: 1000
            - name: memory
              nominalQuota: 2Ti
            - name: nvidia.com/gpu
              nominalQuota: 64
              borrowingLimit: 16  # 可借用16个GPU
  
  # 抢占策略
  preemption:
    reclaimWithinCohort: Any
    withinClusterQueue: LowerPriority
---
apiVersion: kueue.x-k8s.io/v1beta1
kind: LocalQueue
metadata:
  name: training-queue
  namespace: ai-training
spec:
  clusterQueue: cluster-queue-training
```

#### 工作负载适配

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: training-job
  namespace: ai-training
  labels:
    kueue.x-k8s.io/queue-name: training-queue  # 关联队列
spec:
  parallelism: 8
  completions: 8
  template:
    spec:
      containers:
        - name: trainer
          image: pytorch/pytorch:2.1.0
          resources:
            requests:
              nvidia.com/gpu: 1
            limits:
              nvidia.com/gpu: 1
      restartPolicy: OnFailure
```

---

## 二、GPU资源管理进阶

### GPU共享方案对比

| 方案 | 隔离级别 | 显存隔离 | 性能开销 | 复杂度 | 适用场景 |
|------|---------|---------|---------|--------|---------|
| **NVIDIA MIG** | 硬件级 | 完全隔离 | 0% | 低 | A100/H100多租户 |
| **vGPU** | 硬件级 | 完全隔离 | <5% | 中 | 虚拟化环境 |
| **Time-Slicing** | 进程级 | 软隔离 | 5-10% | 低 | 推理服务 |
| **cGPU(阿里云)** | 进程级 | 完全隔离 | <3% | 低 | ACK推荐 |
| **vCUDA** | 进程级 | 软隔离 | 10-15% | 高 | 测试环境 |

---

### 1. NVIDIA MIG配置

#### MIG实例划分

```bash
# 查看MIG支持
nvidia-smi mig -lgip

# 创建MIG实例(7个1g.10gb实例)
nvidia-smi mig -cgi 19,19,19,19,19,19,19 -C

# 查看MIG实例
nvidia-smi mig -lgi

# 输出示例:
# +----+--------+------+
# | ID | Memory | SMs |
# +====+========+======+
# |  0 | 10240  |  14  |
# |  1 | 10240  |  14  |
# ...
```

#### K8s设备插件配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: nvidia-device-plugin-config
  namespace: kube-system
data:
  config.yaml: |
    version: v1
    sharing:
      timeSlicing:
        renameByDefault: false
        failRequestsGreaterThanOne: false
        resources:
          - name: nvidia.com/gpu
            replicas: 10  # 单GPU虚拟10个
    
    # MIG策略
    flags:
      migStrategy: mixed  # single/mixed
      failOnInitError: true
    
    # MIG设备命名
    resources:
      gpus:
        - pattern: "*"
          name: nvidia.com/gpu
      mig:
        - pattern: "1g.10gb"
          name: nvidia.com/mig-1g.10gb
        - pattern: "2g.20gb"
          name: nvidia.com/mig-2g.20gb
        - pattern: "3g.40gb"
          name: nvidia.com/mig-3g.40gb
```

#### MIG实例使用

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: mig-workload
spec:
  containers:
    - name: cuda-app
      image: nvidia/cuda:12.2.0-base-ubuntu22.04
      command: ["nvidia-smi"]
      resources:
        limits:
          nvidia.com/mig-1g.10gb: 1  # 请求1个MIG实例
```

---

### 2. GPU Time-Slicing(时间切片)

#### 配置示例

```yaml
# nvidia-device-plugin-config ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: nvidia-device-plugin-config
  namespace: kube-system
data:
  config.yaml: |
    version: v1
    sharing:
      timeSlicing:
        renameByDefault: false
        resources:
          - name: nvidia.com/gpu
            replicas: 8  # 单GPU虚拟为8个逻辑GPU
```

#### Pod使用

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: shared-gpu-pod-1
spec:
  containers:
    - name: inference
      image: pytorch/pytorch:2.1.0
      resources:
        limits:
          nvidia.com/gpu: 1  # 实际使用1/8物理GPU
```

**适用场景**:
- 推理服务(低并发)
- 开发测试环境
- Jupyter Notebook

**限制**:
- 无显存隔离(OOM会影响其他容器)
- 性能波动(时间片竞争)

---

### 3. cGPU(阿里云容器GPU)

#### 核心优势

- **显存隔离**: 内核级显存隔离，OOM不互相影响
- **算力隔离**: cgroup限制GPU算力
- **零修改**: 应用无需修改代码
- **成本降低**: 单GPU支持10+推理容器

#### ACK配置

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cgpu-inference
spec:
  replicas: 20
  template:
    spec:
      containers:
        - name: model-server
          image: registry.cn-hangzhou.aliyuncs.com/acs/vllm:latest
          resources:
            limits:
              aliyun.com/gpu-mem: 8  # 申请8GB显存
              aliyun.com/gpu-core: 30  # 申请30%算力
          env:
            - name: CUDA_VISIBLE_DEVICES
              value: "0"
```

#### 监控指标

```bash
# 查看cGPU使用情况
kubectl get nodes -o custom-columns=NAME:.metadata.name,GPU-MEM:.status.allocatable.'aliyun\.com/gpu-mem',GPU-CORE:.status.allocatable.'aliyun\.com/gpu-core'
```

---

## 三、高速网络方案

### RDMA网络对比

| 方案 | 带宽 | 延迟 | 成本 | 部署复杂度 | AI训练推荐 |
|------|------|------|------|-----------|-----------|
| **InfiniBand** | 400Gb/s | <1μs | 高 | 高 | ⭐⭐⭐⭐⭐ |
| **RoCE v2** | 100-400Gb/s | <5μs | 中 | 中 | ⭐⭐⭐⭐ |
| **TCP/IP** | 10-100Gb/s | 50-100μs | 低 | 低 | ⚠️ 不推荐大规模训练 |

---

### 1. RoCE配置(阿里云ACK)

#### 节点配置

```yaml
# RDMA设备插件DaemonSet
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: rdma-device-plugin
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: rdma-device-plugin
  template:
    metadata:
      labels:
        app: rdma-device-plugin
    spec:
      hostNetwork: true
      containers:
        - name: rdma-device-plugin
          image: mellanox/k8s-rdma-shared-dev-plugin:latest
          securityContext:
            privileged: true
          volumeMounts:
            - name: device-plugin
              mountPath: /var/lib/kubelet/device-plugins
            - name: sys
              mountPath: /sys
      volumes:
        - name: device-plugin
          hostPath:
            path: /var/lib/kubelet/device-plugins
        - name: sys
          hostPath:
            path: /sys
```

#### Pod使用RDMA

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: rdma-training
  annotations:
    k8s.v1.cni.cncf.io/networks: rdma-network
spec:
  containers:
    - name: pytorch
      image: pytorch/pytorch:2.1.0
      command:
        - python
        - -m
        - torch.distributed.launch
        - --use_env
        - train.py
      resources:
        limits:
          rdma/rdma_shared_device: 1  # 请求RDMA设备
          nvidia.com/gpu: 8
      env:
        - name: NCCL_IB_DISABLE
          value: "0"  # 启用InfiniBand/RoCE
        - name: NCCL_DEBUG
          value: "INFO"
```

---

### 2. NCCL优化配置

```bash
# NCCL环境变量优化
export NCCL_SOCKET_IFNAME=eth0
export NCCL_IB_DISABLE=0
export NCCL_IB_HCA=mlx5_0,mlx5_1
export NCCL_IB_GID_INDEX=3
export NCCL_NET_GDR_LEVEL=5
export NCCL_P2P_LEVEL=SYS

# NCCL性能测试
/usr/local/bin/nccl-tests/build/all_reduce_perf -b 8 -e 128M -f 2 -g 8
```

**性能基准**:
- TCP/IP: ~10GB/s
- RoCE: ~40-50GB/s
- InfiniBand: ~90-100GB/s

---

## 四、分布式存储方案

### 存储方案选型

| 方案 | 吞吐量 | IOPS | 延迟 | 成本 | AI训练推荐 |
|------|--------|------|------|------|-----------|
| **本地NVMe** | 7GB/s | 1M | <100μs | 高 | ⭐⭐⭐⭐⭐ 检查点 |
| **JuiceFS** | 2-5GB/s | 100K | 1-5ms | 中 | ⭐⭐⭐⭐⭐ 数据集 |
| **CephFS** | 1-3GB/s | 50K | 5-10ms | 中 | ⭐⭐⭐⭐ 共享存储 |
| **对象存储(S3/OSS)** | 500MB/s | 10K | 10-50ms | 低 | ⭐⭐⭐ 模型归档 |
| **NFS** | 500MB/s | 5K | 10-20ms | 低 | ⚠️ 不推荐训练 |

---

### 1. JuiceFS - 分布式文件系统

#### 架构特点

- **POSIX兼容**: 标准文件系统接口
- **对象存储后端**: S3/OSS/MinIO
- **元数据分离**: Redis/TiKV/etcd
- **缓存加速**: 本地SSD缓存

#### Helm部署

```bash
helm repo add juicefs https://juicedata.github.io/charts/
helm install juicefs-csi-driver juicefs/juicefs-csi-driver \
  --namespace kube-system \
  --set storageClasses[0].enabled=true \
  --set storageClasses[0].name=juicefs \
  --set storageClasses[0].backend.name=minio \
  --set storageClasses[0].backend.metaurl="redis://redis:6379/1" \
  --set storageClasses[0].backend.storage=s3 \
  --set storageClasses[0].backend.bucket=http://minio:9000/juicefs
```

#### StorageClass配置

```yaml
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
  
  # 缓存配置(关键)
  juicefs/mount-cache-size: "102400"  # 100GB本地缓存
  juicefs/mount-cache-dir: "/var/jfsCache"
  juicefs/mount-prefetch: "1"  # 预读优化
  
  # 性能调优
  juicefs/mount-buffer-size: "300"  # 300MB写缓冲
  juicefs/mount-max-uploads: "50"  # 并发上传数
```

#### PVC使用

```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: training-data
  namespace: ai-training
spec:
  accessModes:
    - ReadWriteMany  # 多Pod共享
  storageClassName: juicefs-sc
  resources:
    requests:
      storage: 10Ti
---
apiVersion: v1
kind: Pod
metadata:
  name: data-loader
spec:
  containers:
    - name: loader
      image: pytorch/pytorch:2.1.0
      volumeMounts:
        - name: data
          mountPath: /data
      command:
        - python
        - data_loader.py
  volumes:
    - name: data
      persistentVolumeClaim:
        claimName: training-data
```

---

### 2. Fluid - 数据编排加速

#### 架构价值

- **数据预热**: 训练前将数据缓存到节点
- **亲和性调度**: Pod调度到有缓存的节点
- **多层缓存**: 内存+SSD+远程存储

#### Alluxio Runtime配置

```yaml
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: imagenet
  namespace: ai-training
spec:
  mounts:
    - mountPoint: s3://my-bucket/imagenet/
      name: imagenet
      options:
        s3a.endpoint: oss-cn-hangzhou.aliyuncs.com
        s3a.access.key: <ACCESS_KEY>
        s3a.secret.key: <SECRET_KEY>
  
  # 数据放置策略
  placement: Exclusive  # 独占节点缓存
---
apiVersion: data.fluid.io/v1alpha1
kind: AlluxioRuntime
metadata:
  name: imagenet
  namespace: ai-training
spec:
  replicas: 4  # 4个缓存节点
  
  # Master配置
  master:
    jvmOptions:
      - "-Xmx16G"
      - "-Xms16G"
    resources:
      requests:
        cpu: 4
        memory: 20Gi
  
  # Worker配置
  worker:
    jvmOptions:
      - "-Xmx32G"
      - "-Xms32G"
    resources:
      requests:
        cpu: 8
        memory: 40Gi
  
  # 缓存层级
  tieredstore:
    levels:
      - mediumtype: MEM
        path: /dev/shm
        quota: 30Gi  # 内存缓存30GB
        high: 0.95
        low: 0.7
      - mediumtype: SSD
        path: /var/lib/alluxio
        quota: 500Gi  # SSD缓存500GB
        high: 0.95
        low: 0.7
  
  # 数据预热
  data:
    replicas: 2  # 2副本
    pin: true  # 常驻内存
```

#### 数据预热Job

```yaml
apiVersion: data.fluid.io/v1alpha1
kind: DataLoad
metadata:
  name: imagenet-preload
  namespace: ai-training
spec:
  dataset:
    name: imagenet
    namespace: ai-training
  
  loadMetadata: true
  
  # 预热策略
  target:
    - path: /train
      replicas: 2  # 训练集2副本
    - path: /val
      replicas: 1  # 验证集1副本
```

---

## 五、AI平台组件生态

### MLOps工具栈

| 阶段 | 工具 | 功能 | 集成难度 | 推荐度 |
|------|------|------|---------|--------|
| **实验跟踪** | MLflow | 参数/指标/模型版本 | ⭐⭐ | ⭐⭐⭐⭐⭐ |
| **实验跟踪** | Weights & Biases | 可视化/协作 | ⭐ | ⭐⭐⭐⭐ |
| **特征存储** | Feast | 特征管理 | ⭐⭐⭐ | ⭐⭐⭐⭐ |
| **模型服务** | KServe | 推理服务 | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **工作流** | Kubeflow Pipelines | DAG编排 | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **工作流** | Argo Workflows | 通用工作流 | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **AutoML** | Katib | 超参数调优 | ⭐⭐⭐ | ⭐⭐⭐ |

---

### 1. MLflow on K8s

#### 部署架构

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: mlflow-server
  namespace: mlops
spec:
  replicas: 2
  selector:
    matchLabels:
      app: mlflow
  template:
    metadata:
      labels:
        app: mlflow
    spec:
      containers:
        - name: mlflow
          image: ghcr.io/mlflow/mlflow:v2.9.2
          args:
            - server
            - --host=0.0.0.0
            - --port=5000
            - --backend-store-uri=postgresql://mlflow:password@postgres:5432/mlflow
            - --default-artifact-root=s3://mlflow-artifacts/
          env:
            - name: AWS_ACCESS_KEY_ID
              valueFrom:
                secretKeyRef:
                  name: s3-credentials
                  key: access-key
            - name: AWS_SECRET_ACCESS_KEY
              valueFrom:
                secretKeyRef:
                  name: s3-credentials
                  key: secret-key
          ports:
            - containerPort: 5000
          resources:
            requests:
              cpu: 1000m
              memory: 2Gi
            limits:
              cpu: 2000m
              memory: 4Gi
---
apiVersion: v1
kind: Service
metadata:
  name: mlflow-service
  namespace: mlops
spec:
  selector:
    app: mlflow
  ports:
    - port: 5000
      targetPort: 5000
  type: LoadBalancer
```

#### 训练代码集成

```python
import mlflow
import mlflow.pytorch

# MLflow跟踪配置
mlflow.set_tracking_uri("http://mlflow-service.mlops:5000")
mlflow.set_experiment("llama2-finetuning")

with mlflow.start_run():
    # 记录参数
    mlflow.log_params({
        "learning_rate": 2e-5,
        "batch_size": 32,
        "epochs": 3,
        "model": "meta-llama/Llama-2-7b"
    })
    
    # 训练循环
    for epoch in range(3):
        loss = train_one_epoch()
        
        # 记录指标
        mlflow.log_metrics({
            "train_loss": loss,
            "epoch": epoch
        }, step=epoch)
    
    # 记录模型
    mlflow.pytorch.log_model(model, "model")
    
    # 记录artifacts
    mlflow.log_artifact("training_curve.png")
```

---

### 2. KServe - 模型推理服务

#### InferenceService配置

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama2-7b
  namespace: ai-inference
spec:
  predictor:
    # 最小副本数
    minReplicas: 2
    maxReplicas: 10
    
    # 自动扩缩容
    scaleTarget: 80  # 80%并发利用率触发扩容
    scaleMetric: concurrency
    
    # GPU资源
    resources:
      requests:
        cpu: 4
        memory: 16Gi
        nvidia.com/gpu: 1
      limits:
        cpu: 8
        memory: 32Gi
        nvidia.com/gpu: 1
    
    # 容器配置
    containers:
      - name: kserve-container
        image: vllm/vllm-openai:latest
        args:
          - --model=/mnt/models/llama2-7b
          - --tensor-parallel-size=1
          - --max-num-seqs=256
        volumeMounts:
          - name: model-storage
            mountPath: /mnt/models
    
    volumes:
      - name: model-storage
        persistentVolumeClaim:
          claimName: model-pvc
  
  # 流量分割(金丝雀)
  canaryTrafficPercent: 10
```

---

## 六、AI Infra成本优化

### 成本优化策略矩阵

| 策略 | 节省比例 | 实施难度 | 风险 | 推荐场景 |
|------|---------|---------|------|---------|
| **Spot实例** | 70-90% | ⭐⭐ | 中断风险 | 可容错训练 |
| **GPU共享** | 60-80% | ⭐⭐⭐ | 性能波动 | 推理服务 |
| **模型压缩** | 50-75% | ⭐⭐⭐⭐ | 精度损失 | 边缘部署 |
| **数据缓存** | 30-50% | ⭐⭐ | 缓存命中率 | 重复训练 |
| **资源右sizing** | 20-40% | ⭐⭐⭐ | 需监控调整 | 所有场景 |
| **批量推理** | 40-60% | ⭐⭐ | 延迟增加 | 离线场景 |

---

### Spot实例配置

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: spot-training
spec:
  # 容忍Spot中断
  tolerations:
    - key: "kubernetes.azure.com/scalesetpriority"
      operator: "Equal"
      value: "spot"
      effect: "NoSchedule"
  
  # 节点亲和性
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
          - matchExpressions:
              - key: "kubernetes.azure.com/scalesetpriority"
                operator: In
                values: ["spot"]
  
  containers:
    - name: pytorch
      image: pytorch/pytorch:2.1.0
      command:
        - python
        - train.py
        - --checkpoint-interval=100  # 频繁检查点
      resources:
        limits:
          nvidia.com/gpu: 8
```

---

## 七、生产最佳实践

### AI Infra检查清单

#### 计算资源

- ✅ GPU节点池隔离(训练/推理)
- ✅ 配置GPU拓扑亲和性
- ✅ 启用Gang调度(Volcano/Kueue)
- ✅ 配置资源配额和优先级
- ✅ 部署GPU监控(DCGM Exporter)

#### 网络

- ✅ 启用RDMA(RoCE/InfiniBand)
- ✅ 配置NCCL优化参数
- ✅ 网络带宽监控
- ✅ 配置QoS保障训练流量

#### 存储

- ✅ 使用高性能存储(JuiceFS/Alluxio)
- ✅ 配置数据预热
- ✅ 本地NVMe缓存检查点
- ✅ 对象存储归档模型

#### 可观测性

- ✅ 实验跟踪(MLflow)
- ✅ GPU利用率监控
- ✅ 训练任务告警
- ✅ 成本分析dashboard

#### 安全

- ✅ 模型加密存储
- ✅ 训练数据访问控制
- ✅ NetworkPolicy隔离
- ✅ 镜像安全扫描

---

**表格维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)
