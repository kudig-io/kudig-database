# 133 - GPU调度与管理 (GPU Scheduling & Management)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-01 | **参考**: [NVIDIA Device Plugin](https://github.com/NVIDIA/k8s-device-plugin)

---

## 一、GPU资源管理架构 (Architecture Overview)

### 1.1 Kubernetes GPU调度架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    Kubernetes GPU 调度架构                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      Control Plane                                   │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │   │
│  │  │ kube-scheduler│  │ Device Plugin│  │ GPU Operator │              │   │
│  │  │   (调度决策)  │  │  Manager     │  │  (生命周期)   │              │   │
│  │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘              │   │
│  │         │                 │                 │                       │   │
│  │         │    Extended Resources API         │                       │   │
│  │         └─────────────────┼─────────────────┘                       │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                              │                                              │
│  ════════════════════════════╪══════════════════════════════════════════   │
│                              │                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                        GPU Node                                      │   │
│  │  ┌──────────────────────────────────────────────────────────────┐   │   │
│  │  │  NVIDIA Device Plugin (DaemonSet)                             │   │   │
│  │  │  ├── GPU发现与注册                                            │   │   │
│  │  │  ├── 健康检查                                                 │   │   │
│  │  │  ├── 设备分配 (nvidia.com/gpu)                                │   │   │
│  │  │  └── MIG/Time-Slicing管理                                     │   │   │
│  │  └──────────────────────────────────────────────────────────────┘   │   │
│  │                              │                                       │   │
│  │  ┌──────────────┐  ┌────────┴────────┐  ┌──────────────┐           │   │
│  │  │ DCGM Exporter│  │ Container Runtime│  │ GPU Driver   │           │   │
│  │  │ (监控指标)    │  │ (nvidia-container)│  │ (CUDA/cuDNN) │           │   │
│  │  └──────────────┘  └─────────────────┘  └──────────────┘           │   │
│  │                              │                                       │   │
│  │  ┌──────────────────────────────────────────────────────────────┐   │   │
│  │  │                    Physical GPUs                              │   │   │
│  │  │  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐              │   │   │
│  │  │  │ GPU 0  │  │ GPU 1  │  │ GPU 2  │  │ GPU 3  │              │   │   │
│  │  │  │ A100   │  │ A100   │  │ A100   │  │ A100   │              │   │   │
│  │  │  └────────┘  └────────┘  └────────┘  └────────┘              │   │   │
│  │  └──────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 GPU虚拟化技术对比

| 技术 | 隔离级别 | 粒度 | 显存隔离 | 算力隔离 | 适用GPU | 适用场景 |
|-----|---------|------|---------|---------|--------|---------|
| **Passthrough** | 强(硬件) | 整卡 | 完全 | 完全 | 所有 | 训练/大模型推理 |
| **MIG** | 强(硬件) | 1/7卡 | 完全 | 完全 | A100/H100 | 多租户/混合推理 |
| **Time-Slicing** | 弱(时分) | 模拟多卡 | 软限制 | 无 | 所有 | 开发/小模型推理 |
| **vGPU** | 中(软件) | 百分比 | 软隔离 | 软隔离 | 企业版 | 虚拟化/VDI |
| **MPS** | 弱(进程) | 共享 | 无 | 无 | 所有 | 小任务混部 |
| **DRA** (v1.31+) | 灵活 | 动态 | 取决于驱动 | 取决于驱动 | 所有 | 下一代资源管理 |

---

## 二、GPU Operator部署 (GPU Operator Deployment)

### 2.1 GPU Operator架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      NVIDIA GPU Operator 组件                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────┐                                                        │
│  │  GPU Operator   │ ← 控制器,管理所有组件生命周期                           │
│  │  (Deployment)   │                                                        │
│  └────────┬────────┘                                                        │
│           │                                                                  │
│  ┌────────┴────────────────────────────────────────────────────────────┐   │
│  │                    Managed Components (DaemonSet)                    │   │
│  │                                                                      │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                 │   │
│  │  │ Driver      │  │ Container   │  │ Device      │                 │   │
│  │  │ (驱动安装)   │  │ Toolkit     │  │ Plugin      │                 │   │
│  │  │             │  │ (运行时)     │  │ (设备发现)   │                 │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                 │   │
│  │                                                                      │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                 │   │
│  │  │ DCGM        │  │ MIG Manager │  │ Node Feature│                 │   │
│  │  │ Exporter    │  │ (MIG配置)    │  │ Discovery   │                 │   │
│  │  │ (监控)      │  │             │  │ (节点标签)   │                 │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                 │   │
│  │                                                                      │   │
│  │  ┌─────────────┐  ┌─────────────┐                                  │   │
│  │  │ GPU Feature │  │ Validator   │                                  │   │
│  │  │ Discovery   │  │ (验证)      │                                  │   │
│  │  └─────────────┘  └─────────────┘                                  │   │
│  └──────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 部署配置

```yaml
# GPU Operator Helm Values (生产级配置)
apiVersion: v1
kind: ConfigMap
metadata:
  name: gpu-operator-values
data:
  values.yaml: |
    operator:
      defaultRuntime: containerd
      
    driver:
      enabled: true
      version: "535.104.12"
      repository: nvcr.io/nvidia
      
      # 驱动升级策略
      upgradePolicy:
        autoUpgrade: false
        maxParallelUpgrades: 1
        maxUnavailable: "25%"
        waitForCompletion:
          timeoutSeconds: 0
          
      # RDMA支持
      rdma:
        enabled: true
        useHostMofed: true
        
    toolkit:
      enabled: true
      version: "v1.14.3-ubuntu20.04"
      
    devicePlugin:
      enabled: true
      version: "v0.14.3"
      
      # Time-Slicing配置
      config:
        name: "time-slicing-config"
        default: "any"
        
    dcgm:
      enabled: true
      
    dcgmExporter:
      enabled: true
      version: "3.3.0-3.2.0-ubuntu22.04"
      serviceMonitor:
        enabled: true
        
    gfd:
      enabled: true
      version: "v0.8.2"
      
    migManager:
      enabled: true
      
    nodeStatusExporter:
      enabled: true
      
    validator:
      enabled: true
```

```bash
# 部署GPU Operator
helm repo add nvidia https://helm.ngc.nvidia.com/nvidia
helm repo update

helm install gpu-operator nvidia/gpu-operator \
  --namespace gpu-operator \
  --create-namespace \
  --version v23.9.1 \
  -f gpu-operator-values.yaml
```

---

## 三、Time-Slicing配置 (Time-Slicing Configuration)

### 3.1 Time-Slicing原理

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     Time-Slicing 工作原理                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  物理GPU (1张 A100)                                                         │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                                                                     │   │
│  │    Time Slice 1    Time Slice 2    Time Slice 3    Time Slice 4    │   │
│  │  ┌──────────────┐┌──────────────┐┌──────────────┐┌──────────────┐  │   │
│  │  │   Pod A      ││   Pod B      ││   Pod C      ││   Pod D      │  │   │
│  │  │   (25%)      ││   (25%)      ││   (25%)      ││   (25%)      │  │   │
│  │  └──────────────┘└──────────────┘└──────────────┘└──────────────┘  │   │
│  │        ↓               ↓               ↓               ↓          │   │
│  │  ══════════════════════════════════════════════════════════════   │   │
│  │                    时间片轮转 (Context Switch)                     │   │
│  │  ══════════════════════════════════════════════════════════════   │   │
│  │                                                                     │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  Kubernetes视角: 4个 nvidia.com/gpu 资源                                     │
│  实际硬件: 1张物理GPU,显存共享                                               │
│                                                                             │
│  注意事项:                                                                   │
│  - 显存不隔离,所有Pod共享80GB                                                │
│  - 算力按时间片轮转,非真实隔离                                               │
│  - 适合开发调试,不适合生产训练                                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Time-Slicing配置

```yaml
# Time-Slicing ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: time-slicing-config
  namespace: gpu-operator
data:
  any: |
    version: v1
    flags:
      migStrategy: none
    sharing:
      timeSlicing:
        renameByDefault: true
        failRequestsGreaterThanOne: false
        resources:
        - name: nvidia.com/gpu
          replicas: 4           # 每张GPU模拟4张
          
  inference: |
    version: v1
    sharing:
      timeSlicing:
        renameByDefault: true
        resources:
        - name: nvidia.com/gpu
          replicas: 8           # 推理场景更多切片
          
  development: |
    version: v1
    sharing:
      timeSlicing:
        renameByDefault: true
        resources:
        - name: nvidia.com/gpu
          replicas: 10          # 开发环境最大切片

---
# 节点标签应用不同配置
# 生产训练节点: 不启用Time-Slicing
kubectl label node gpu-train-01 nvidia.com/device-plugin.config=none

# 推理节点: 8倍切片
kubectl label node gpu-infer-01 nvidia.com/device-plugin.config=inference

# 开发节点: 10倍切片
kubectl label node gpu-dev-01 nvidia.com/device-plugin.config=development
```

### 3.3 使用Time-Slicing资源

```yaml
# 使用Time-Slicing GPU的Pod
apiVersion: v1
kind: Pod
metadata:
  name: inference-pod
spec:
  containers:
  - name: inference
    image: nvcr.io/nvidia/pytorch:24.01-py3
    resources:
      limits:
        nvidia.com/gpu: 1       # 使用1个虚拟GPU切片
    env:
    - name: CUDA_VISIBLE_DEVICES
      value: "0"
    - name: NVIDIA_VISIBLE_DEVICES
      value: "all"
      
---
# 验证Time-Slicing效果
apiVersion: v1
kind: Pod
metadata:
  name: gpu-test
spec:
  containers:
  - name: test
    image: nvcr.io/nvidia/cuda:12.2.0-base-ubuntu22.04
    command: ["nvidia-smi", "-L"]
    resources:
      limits:
        nvidia.com/gpu: 1
```

---

## 四、MIG配置与管理 (MIG Configuration)

### 4.1 MIG架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     A100 80GB MIG 配置示例                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  配置模式1: 7x 1g.10gb (最大实例数)                                          │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐          │   │
│  │ │10GB │ │10GB │ │10GB │ │10GB │ │10GB │ │10GB │ │10GB │          │   │
│  │ │1/7SM│ │1/7SM│ │1/7SM│ │1/7SM│ │1/7SM│ │1/7SM│ │1/7SM│          │   │
│  │ └─────┘ └─────┘ └─────┘ └─────┘ └─────┘ └─────┘ └─────┘          │   │
│  │ Instance 0-6: nvidia.com/mig-1g.10gb                              │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  配置模式2: 3x 2g.20gb + 1x 1g.10gb (混合配置)                               │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌─────┐                  │   │
│  │ │   20GB    │ │   20GB    │ │   20GB    │ │10GB │                  │   │
│  │ │   2/7SM   │ │   2/7SM   │ │   2/7SM   │ │1/7SM│                  │   │
│  │ └───────────┘ └───────────┘ └───────────┘ └─────┘                  │   │
│  │ nvidia.com/mig-2g.20gb x3   nvidia.com/mig-1g.10gb x1             │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  配置模式3: 1x 4g.40gb + 3x 1g.10gb (大小混合)                               │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ ┌───────────────────────┐ ┌─────┐ ┌─────┐ ┌─────┐                  │   │
│  │ │        40GB           │ │10GB │ │10GB │ │10GB │                  │   │
│  │ │        4/7SM          │ │1/7SM│ │1/7SM│ │1/7SM│                  │   │
│  │ └───────────────────────┘ └─────┘ └─────┘ └─────┘                  │   │
│  │ nvidia.com/mig-4g.40gb x1   nvidia.com/mig-1g.10gb x3             │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  配置模式4: 1x 7g.80gb (单实例)                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ ┌─────────────────────────────────────────────────────────────────┐ │   │
│  │ │                          80GB                                   │ │   │
│  │ │                          7/7SM (Full GPU)                       │ │   │
│  │ └─────────────────────────────────────────────────────────────────┘ │   │
│  │ nvidia.com/mig-7g.80gb x1 (等同于整卡)                              │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 MIG配置管理

```yaml
# MIG Manager ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: mig-parted-config
  namespace: gpu-operator
data:
  config.yaml: |
    version: v1
    mig-configs:
      # 推理优化配置: 7个小实例
      all-1g.10gb:
        - devices: all
          mig-enabled: true
          mig-devices:
            "1g.10gb": 7
            
      # 混合推理配置: 3中+1小
      all-2g.20gb:
        - devices: all
          mig-enabled: true
          mig-devices:
            "2g.20gb": 3
            "1g.10gb": 1
            
      # 大模型推理: 2大+1小
      all-3g.40gb:
        - devices: all
          mig-enabled: true
          mig-devices:
            "3g.40gb": 2
            "1g.10gb": 1
            
      # 训练配置: 单大实例
      all-7g.80gb:
        - devices: all
          mig-enabled: true
          mig-devices:
            "7g.80gb": 1
            
      # 禁用MIG
      all-disabled:
        - devices: all
          mig-enabled: false

---
# 应用MIG配置到节点
apiVersion: v1
kind: Node
metadata:
  name: gpu-node-01
  labels:
    nvidia.com/mig.config: "all-1g.10gb"    # 7个小实例
    
---
# 节点选择器使用MIG资源
apiVersion: v1
kind: Pod
metadata:
  name: mig-inference-pod
spec:
  nodeSelector:
    nvidia.com/mig.config: "all-1g.10gb"
  containers:
  - name: inference
    image: nvcr.io/nvidia/pytorch:24.01-py3
    resources:
      limits:
        nvidia.com/mig-1g.10gb: 1           # 请求1个MIG实例
```

### 4.3 MIG实例类型规格

| MIG Profile | 显存 | SM数 | 显存带宽 | 适用场景 |
|------------|------|------|---------|---------|
| **1g.10gb** | 10GB | 14 | ~285GB/s | 小模型推理/开发 |
| **2g.20gb** | 20GB | 28 | ~570GB/s | 中型模型推理 |
| **3g.40gb** | 40GB | 42 | ~855GB/s | 大模型推理 |
| **4g.40gb** | 40GB | 56 | ~1140GB/s | 大模型推理/微调 |
| **7g.80gb** | 80GB | 98 | ~2000GB/s | 训练/大模型 |

---

## 五、高级调度策略 (Advanced Scheduling)

### 5.1 GPU拓扑感知调度

```yaml
# GPU拓扑感知调度 (多GPU任务)
apiVersion: scheduling.volcano.sh/v1beta1
kind: PodGroup
metadata:
  name: distributed-training
spec:
  minMember: 4
  queue: training
  
---
apiVersion: batch.volcano.sh/v1alpha1
kind: Job
metadata:
  name: pytorch-distributed
spec:
  minAvailable: 4
  schedulerName: volcano
  plugins:
    env: []
    svc: []
  policies:
  - event: PodEvicted
    action: RestartJob
  tasks:
  - replicas: 4
    name: worker
    template:
      spec:
        schedulerName: volcano
        containers:
        - name: pytorch
          image: nvcr.io/nvidia/pytorch:24.01-py3
          resources:
            limits:
              nvidia.com/gpu: 8
          env:
          - name: NCCL_TOPO_FILE
            value: "/etc/nccl/topo.xml"
        # GPU拓扑亲和性
        affinity:
          podAntiAffinity:
            preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchLabels:
                    job-name: pytorch-distributed
                topologyKey: kubernetes.io/hostname
```

### 5.2 Kueue GPU队列管理

```yaml
# Kueue ResourceFlavor定义GPU类型
apiVersion: kueue.x-k8s.io/v1beta1
kind: ResourceFlavor
metadata:
  name: a100-80gb
spec:
  nodeLabels:
    nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
  nodeTaints:
  - key: nvidia.com/gpu
    value: "true"
    effect: NoSchedule
    
---
# ClusterQueue定义GPU配额
apiVersion: kueue.x-k8s.io/v1beta1
kind: ClusterQueue
metadata:
  name: gpu-cluster-queue
spec:
  namespaceSelector: {}
  resourceGroups:
  - coveredResources: ["cpu", "memory", "nvidia.com/gpu"]
    flavors:
    - name: a100-80gb
      resources:
      - name: "cpu"
        nominalQuota: 1000
      - name: "memory"
        nominalQuota: 4Ti
      - name: "nvidia.com/gpu"
        nominalQuota: 64
        borrowingLimit: 32            # 可借用上限
        
  # 抢占策略
  preemption:
    reclaimWithinCohort: Any
    withinClusterQueue: LowerPriority

---
# LocalQueue绑定到namespace
apiVersion: kueue.x-k8s.io/v1beta1
kind: LocalQueue
metadata:
  name: ml-team-queue
  namespace: ml-training
spec:
  clusterQueue: gpu-cluster-queue
  
---
# Workload提交
apiVersion: kueue.x-k8s.io/v1beta1
kind: Workload
metadata:
  name: training-job
  namespace: ml-training
spec:
  queueName: ml-team-queue
  priority: 100
  podSets:
  - name: main
    count: 4
    template:
      spec:
        containers:
        - name: trainer
          resources:
            requests:
              cpu: "32"
              memory: "128Gi"
              nvidia.com/gpu: "8"
```

### 5.3 Volcano批调度

```yaml
# Volcano Queue配置
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: training-queue
spec:
  weight: 10
  reclaimable: true
  capability:
    cpu: "1000"
    memory: "4Ti"
    nvidia.com/gpu: "64"
    
---
# Gang Scheduling训练任务
apiVersion: batch.volcano.sh/v1alpha1
kind: Job
metadata:
  name: llm-training
spec:
  minAvailable: 8                     # 最少需要8个Pod同时运行
  schedulerName: volcano
  queue: training-queue
  
  # 调度策略
  policies:
  - event: PodEvicted
    action: RestartJob
  - event: TaskCompleted
    action: CompleteJob
    
  # 插件配置
  plugins:
    env: []
    svc: []
    ssh: []                           # SSH互联
    
  tasks:
  - replicas: 8
    name: worker
    template:
      spec:
        restartPolicy: OnFailure
        containers:
        - name: pytorch
          image: nvcr.io/nvidia/pytorch:24.01-py3
          command: ["torchrun"]
          args:
          - "--nproc_per_node=8"
          - "--nnodes=8"
          - "--node_rank=$(VC_TASK_INDEX)"
          - "--master_addr=$(VC_MASTER_HOST)"
          - "--master_port=29500"
          - "train.py"
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
          - name: NCCL_DEBUG
            value: "INFO"
          - name: NCCL_IB_DISABLE
            value: "0"
```

---

## 六、GPU监控与诊断 (Monitoring & Diagnostics)

### 6.1 DCGM监控指标

| 指标名称 | Field ID | 说明 | 告警阈值 |
|---------|----------|------|---------|
| `DCGM_FI_DEV_GPU_UTIL` | 203 | GPU利用率% | < 50% 低效 |
| `DCGM_FI_DEV_MEM_COPY_UTIL` | 204 | 显存拷贝利用率% | - |
| `DCGM_FI_DEV_FB_USED` | 252 | 已用显存(MB) | > 95% |
| `DCGM_FI_DEV_FB_FREE` | 251 | 空闲显存(MB) | < 5% |
| `DCGM_FI_DEV_GPU_TEMP` | 150 | GPU温度(°C) | > 83°C |
| `DCGM_FI_DEV_POWER_USAGE` | 155 | 功耗(W) | > TDP |
| `DCGM_FI_DEV_SM_CLOCK` | 100 | SM时钟(MHz) | 降频告警 |
| `DCGM_FI_DEV_MEM_CLOCK` | 101 | 显存时钟(MHz) | 降频告警 |
| `DCGM_FI_DEV_PCIE_TX_THROUGHPUT` | 409 | PCIe发送(MB/s) | - |
| `DCGM_FI_DEV_PCIE_RX_THROUGHPUT` | 410 | PCIe接收(MB/s) | - |
| `DCGM_FI_DEV_NVLINK_BANDWIDTH_TOTAL` | 450 | NVLink带宽(GB/s) | - |
| `DCGM_FI_DEV_XID_ERRORS` | 230 | XID错误计数 | > 0 |
| `DCGM_FI_DEV_ECC_SBE_VOL_TOTAL` | 310 | 单比特ECC错误 | 增长趋势 |
| `DCGM_FI_DEV_ECC_DBE_VOL_TOTAL` | 313 | 双比特ECC错误 | > 0 |

### 6.2 Prometheus告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: gpu-alerts
  namespace: monitoring
spec:
  groups:
  - name: gpu-alerts
    rules:
    # GPU利用率低
    - alert: GPULowUtilization
      expr: |
        avg_over_time(DCGM_FI_DEV_GPU_UTIL[10m]) < 30
      for: 30m
      labels:
        severity: warning
      annotations:
        summary: "GPU利用率过低"
        description: "GPU {{ $labels.gpu }} 利用率 {{ $value }}%"
        
    # GPU显存即将满
    - alert: GPUMemoryNearFull
      expr: |
        DCGM_FI_DEV_FB_USED / (DCGM_FI_DEV_FB_USED + DCGM_FI_DEV_FB_FREE) > 0.95
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "GPU显存使用率超过95%"
        
    # GPU温度过高
    - alert: GPUHighTemperature
      expr: |
        DCGM_FI_DEV_GPU_TEMP > 83
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "GPU温度超过83°C"
        description: "GPU {{ $labels.gpu }} 温度 {{ $value }}°C"
        
    # XID错误
    - alert: GPUXIDError
      expr: |
        increase(DCGM_FI_DEV_XID_ERRORS[5m]) > 0
      for: 0m
      labels:
        severity: critical
      annotations:
        summary: "GPU XID错误"
        description: "GPU {{ $labels.gpu }} 发生XID错误"
        
    # ECC错误
    - alert: GPUECCError
      expr: |
        increase(DCGM_FI_DEV_ECC_DBE_VOL_TOTAL[1h]) > 0
      for: 0m
      labels:
        severity: critical
      annotations:
        summary: "GPU双比特ECC错误"
        description: "GPU {{ $labels.gpu }} 发生不可纠正的ECC错误,需要更换"
        
    # GPU掉卡
    - alert: GPUNotAvailable
      expr: |
        absent(DCGM_FI_DEV_GPU_UTIL{gpu=~".+"})
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "GPU不可用"
        description: "无法获取GPU指标,GPU可能已掉卡"
```

### 6.3 故障诊断命令

```bash
# ========== 基础诊断 ==========

# GPU状态概览
nvidia-smi

# 详细GPU信息
nvidia-smi -q

# GPU拓扑
nvidia-smi topo -m

# NVLink状态
nvidia-smi nvlink -s

# ========== 性能诊断 ==========

# 实时监控
nvidia-smi dmon -s pucvmet -d 1

# GPU进程
nvidia-smi pmon -s um -d 1

# 时钟频率
nvidia-smi -q -d CLOCK

# 功耗限制
nvidia-smi -q -d POWER

# ========== 故障诊断 ==========

# XID错误
dmesg | grep -i "nvrm\|xid"

# ECC错误
nvidia-smi -q -d ECC

# GPU重置历史
nvidia-smi -q -d PAGE_RETIREMENT

# 驱动版本
cat /proc/driver/nvidia/version

# ========== MIG诊断 ==========

# MIG状态
nvidia-smi mig -lgi
nvidia-smi mig -lci

# MIG实例详情
nvidia-smi mig -lgip
nvidia-smi mig -lcip

# ========== Kubernetes诊断 ==========

# GPU节点资源
kubectl describe node <gpu-node> | grep -A 10 "Allocated resources"

# Device Plugin日志
kubectl logs -n gpu-operator -l app=nvidia-device-plugin-daemonset

# GPU Operator状态
kubectl get pods -n gpu-operator

# 节点GPU标签
kubectl get nodes -L nvidia.com/gpu.product,nvidia.com/gpu.count,nvidia.com/mig.config
```

### 6.4 常见XID错误代码

| XID | 错误类型 | 原因 | 解决方案 |
|-----|---------|------|---------|
| **13** | Graphics Engine Exception | CUDA kernel错误 | 检查CUDA代码 |
| **31** | GPU memory page fault | 显存访问越界 | 检查显存分配 |
| **43** | GPU stopped processing | GPU挂起 | 重置GPU |
| **45** | Preemptive cleanup | 显存清理超时 | 检查驱动版本 |
| **48** | Double Bit ECC Error | 不可纠正ECC | 更换GPU |
| **61** | Internal micro-controller breakpoint | 固件问题 | 重启节点 |
| **62** | Internal micro-controller halt | 固件严重错误 | 更换GPU |
| **63** | ECC page retirement | ECC页面退役 | 监控趋势 |
| **64** | ECC page retirement | 页面退役达上限 | 更换GPU |
| **74** | NVLink Error | NVLink故障 | 检查硬件连接 |
| **79** | GPU access to memory denied | 内存访问拒绝 | 检查驱动/BIOS |
| **94** | Contained ECC error | ECC错误已隔离 | 监控 |
| **95** | Uncontained ECC error | ECC错误未隔离 | 更换GPU |

---

## 七、GPU资源最佳实践 (Best Practices)

### 7.1 资源请求配置

```yaml
# 训练任务配置 (完整资源声明)
apiVersion: v1
kind: Pod
metadata:
  name: training-pod
spec:
  # 调度约束
  nodeSelector:
    nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
  tolerations:
  - key: "nvidia.com/gpu"
    operator: "Exists"
    effect: "NoSchedule"
    
  containers:
  - name: trainer
    image: nvcr.io/nvidia/pytorch:24.01-py3
    
    # 资源请求=限制 (保证QoS)
    resources:
      requests:
        cpu: "64"
        memory: "512Gi"
        nvidia.com/gpu: "8"
      limits:
        cpu: "64"
        memory: "512Gi"
        nvidia.com/gpu: "8"
        
    # GPU环境变量
    env:
    - name: CUDA_VISIBLE_DEVICES
      value: "0,1,2,3,4,5,6,7"
    - name: NVIDIA_VISIBLE_DEVICES
      value: "all"
    - name: NVIDIA_DRIVER_CAPABILITIES
      value: "compute,utility"
      
    # NCCL优化
    - name: NCCL_DEBUG
      value: "WARN"
    - name: NCCL_IB_DISABLE
      value: "0"
    - name: NCCL_NET_GDR_LEVEL
      value: "5"
    - name: NCCL_P2P_LEVEL
      value: "NVL"
      
    # 显存优化
    - name: PYTORCH_CUDA_ALLOC_CONF
      value: "max_split_size_mb:512"
      
    # 卷挂载
    volumeMounts:
    - name: shm
      mountPath: /dev/shm
    - name: data
      mountPath: /data
      
  volumes:
  - name: shm
    emptyDir:
      medium: Memory
      sizeLimit: "64Gi"           # 共享内存
  - name: data
    persistentVolumeClaim:
      claimName: training-data
```

### 7.2 多GPU训练优化

```yaml
# 分布式训练配置
apiVersion: "kubeflow.org/v1"
kind: PyTorchJob
metadata:
  name: distributed-training
spec:
  elasticPolicy:
    rdzvBackend: c10d
    minReplicas: 4
    maxReplicas: 8
    maxRestarts: 3
    
  pytorchReplicaSpecs:
    Worker:
      replicas: 4
      restartPolicy: OnFailure
      template:
        spec:
          containers:
          - name: pytorch
            image: nvcr.io/nvidia/pytorch:24.01-py3
            imagePullPolicy: Always
            
            resources:
              limits:
                nvidia.com/gpu: 8
                rdma/rdma_shared_device_a: 1   # RDMA设备
                
            env:
            # torchrun配置
            - name: MASTER_ADDR
              valueFrom:
                fieldRef:
                  fieldPath: status.podIP
            - name: NPROC_PER_NODE
              value: "8"
              
            # 性能优化
            - name: OMP_NUM_THREADS
              value: "8"
            - name: MKL_NUM_THREADS
              value: "8"
            - name: NCCL_SOCKET_IFNAME
              value: "eth0"
              
            volumeMounts:
            - name: shm
              mountPath: /dev/shm
              
          volumes:
          - name: shm
            emptyDir:
              medium: Memory
              sizeLimit: "128Gi"
              
          # 拓扑亲和性
          affinity:
            podAntiAffinity:
              requiredDuringSchedulingIgnoredDuringExecution:
              - labelSelector:
                  matchLabels:
                    training.kubeflow.org/job-name: distributed-training
                topologyKey: kubernetes.io/hostname
```

---

## 八、快速参考 (Quick Reference)

### 8.1 GPU资源类型

```bash
# 整卡资源
nvidia.com/gpu: 1

# MIG资源 (A100/H100)
nvidia.com/mig-1g.10gb: 1
nvidia.com/mig-2g.20gb: 1
nvidia.com/mig-3g.40gb: 1
nvidia.com/mig-4g.40gb: 1
nvidia.com/mig-7g.80gb: 1

# Time-Slicing (虚拟切片)
nvidia.com/gpu: 1  # 实际为1/N物理卡
```

### 8.2 常用kubectl命令

```bash
# 查看GPU节点
kubectl get nodes -l nvidia.com/gpu.present=true

# 查看GPU分配
kubectl describe node <node> | grep -A 5 "nvidia.com/gpu"

# 查看GPU Pod
kubectl get pods -A -o wide --field-selector spec.nodeName=<gpu-node>

# GPU Operator状态
kubectl get clusterpolicy

# MIG配置状态
kubectl get nodes -L nvidia.com/mig.config

# Device Plugin日志
kubectl logs -n gpu-operator -l app=nvidia-device-plugin-daemonset --tail=100
```

---

**GPU管理原则**: 合理切分资源 → 监控利用率 → 及时处理XID错误 → 定期健康检查

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
