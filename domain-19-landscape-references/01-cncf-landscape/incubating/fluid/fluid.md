---
title: Fluid
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- scheduler
- prometheus
- grafana
- helm
- redis
- mysql
- daemonset
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- Fluid 是什么
- 如何 Fluid
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Fluid
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- monitoring-basics
- redis-basics
- mysql-basics
- gpu-scheduling-basics
---

title: Fluid
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- helm
- job
- crd
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Fluid 是什么
- 如何 Fluid
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Fluid
- cncf
- landscape
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
# Fluid

> **成熟度**: Incubating | **加入时间**: 2021-04 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://fluid-cloudnative.github.io |
| **GitHub** | https://github.com/fluid-cloudnative/fluid |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Storage & Data Acceleration |

---

## 项目概述

Fluid 是 Kubernetes 上的数据集编排和加速系统，为数据密集型应用（如 AI/ML、大数据分析）提供数据抽象层。它通过分布式缓存引擎加速数据访问，实现数据与计算的协同调度。

## 核心特性

- **数据抽象**: Dataset CRD 统一管理数据访问
- **数据加速**: 支持 Alluxio、JuiceFS、Vineyard 等缓存引擎
- **数据感知调度**: 将 Pod 调度到数据缓存所在节点
- **弹性伸缩**: 根据负载自动扩缩缓存集群
- **数据预热**: 提前加载数据到缓存层
- **数据迁移**: 支持数据在不同存储间迁移

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      Fluid Architecture                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Application Layer                       │ │
│  │  ┌───────────────────────────────────────────────────────┐│ │
│  │  │    AI/ML Training    │    Data Analytics    │   ETL   ││ │
│  │  └───────────────────────────────────────────────────────┘│ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│                    PVC/CSI Mount                                 │
│                              │                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                     Fluid Runtime                          │ │
│  │                                                            │ │
│  │  ┌─────────────────────────────────────────────────────┐  │ │
│  │  │              Distributed Cache Engine               │  │ │
│  │  │  ┌──────────┐  ┌──────────┐  ┌────────────────┐   │  │ │
│  │  │  │ Alluxio  │  │ JuiceFS  │  │    Vineyard    │   │  │ │
│  │  │  │ Runtime  │  │ Runtime  │  │    Runtime     │   │  │ │
│  │  │  └──────────┘  └──────────┘  └────────────────┘   │  │ │
│  │  │                                                     │  │ │
│  │  │  ┌────────────────────────────────────────────┐   │  │ │
│  │  │  │  Master  │  Workers  │  FUSE  │  Metadata  │   │  │ │
│  │  │  └────────────────────────────────────────────┘   │  │ │
│  │  └─────────────────────────────────────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Data Sources                            │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌───────────┐ │ │
│  │  │   S3     │  │   HDFS   │  │   OSS    │  │   NFS     │ │ │
│  │  └──────────┘  └──────────┘  └──────────┘  └───────────┘ │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心概念

| 概念 | 说明 |
|------|------|
| Dataset | 数据集定义，指向远端存储路径 |
| Runtime | 缓存引擎运行时（Alluxio/JuiceFS/Vineyard） |
| DataLoad | 数据预热任务 |
| DataBackup | 数据备份任务 |
| DataMigrate | 数据迁移任务 |

---

## 快速开始

### 安装 Fluid

```bash
# Helm 安装
helm repo add fluid https://fluid-cloudnative.github.io/charts
helm install fluid fluid/fluid --namespace fluid-system --create-namespace
```

### 创建 Dataset

```yaml
# dataset.yaml
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: my-dataset
spec:
  mounts:
    - mountPoint: s3://my-bucket/data/
      name: s3-data
      options:
        fs.s3a.access.key: <access-key>
        fs.s3a.secret.key: <secret-key>
        fs.s3a.endpoint: s3.amazonaws.com
```

### 创建 Alluxio Runtime

```yaml
# alluxio-runtime.yaml
apiVersion: data.fluid.io/v1alpha1
kind: AlluxioRuntime
metadata:
  name: my-dataset
spec:
  replicas: 2
  tieredstore:
    levels:
      - mediumtype: MEM
        path: /dev/shm
        quota: 4Gi
        high: "0.95"
        low: "0.7"
      - mediumtype: SSD
        path: /mnt/ssd
        quota: 100Gi
        high: "0.95"
        low: "0.7"
  master:
    replicas: 1
    resources:
      requests:
        cpu: 100m
        memory: 1Gi
  worker:
    resources:
      requests:
        cpu: 100m
        memory: 2Gi
  fuse:
    resources:
      requests:
        cpu: 100m
        memory: 1Gi
```

```bash
kubectl apply -f dataset.yaml
kubectl apply -f alluxio-runtime.yaml

# 查看状态
kubectl get dataset my-dataset
kubectl get alluxioruntime my-dataset
```

### 使用 Dataset

```yaml
# training-job.yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: training-job
spec:
  template:
    spec:
      containers:
        - name: trainer
          image: pytorch/pytorch:latest
          command: ["python", "train.py", "--data", "/data"]
          volumeMounts:
            - name: data
              mountPath: /data
      volumes:
        - name: data
          persistentVolumeClaim:
            claimName: my-dataset  # 自动创建的 PVC
      restartPolicy: Never
```

---

## 数据预热

```yaml
# dataload.yaml
apiVersion: data.fluid.io/v1alpha1
kind: DataLoad
metadata:
  name: my-dataload
spec:
  dataset:
    name: my-dataset
    namespace: default
  loadMetadata: true
  target:
    - path: /train
      replicas: 2
    - path: /test
      replicas: 1
```

```bash
kubectl apply -f dataload.yaml

# 查看预热进度
kubectl get dataload my-dataload
kubectl describe dataload my-dataload
```

---

## 数据感知调度

```yaml
# 自动调度到有缓存的节点
apiVersion: batch/v1
kind: Job
metadata:
  name: data-aware-job
spec:
  template:
    spec:
      affinity:
        # Fluid 自动注入亲和性
      containers:
        - name: app
          image: myapp:latest
          volumeMounts:
            - name: data
              mountPath: /data
      volumes:
        - name: data
          persistentVolumeClaim:
            claimName: my-dataset
```

---

## JuiceFS Runtime

```yaml
apiVersion: data.fluid.io/v1alpha1
kind: JuiceFSRuntime
metadata:
  name: juicefs-dataset
spec:
  replicas: 2
  tieredstore:
    levels:
      - mediumtype: SSD
        path: /mnt/ssd/cache
        quota: 100Gi
  configs:
    name: juicefs-config
  fuse:
    image: juicedata/juicefs-fuse
    imageTag: latest
```

---

## 监控

```yaml
# Prometheus 指标
- fluid_dataset_cache_hit_ratio
- fluid_dataset_cache_capacity
- fluid_dataset_cache_usage
- fluid_runtime_worker_count
```

---

## 最佳实践

1. **分层缓存**: 配置 MEM + SSD 多级缓存提高命中率
2. **数据预热**: 训练前预热数据减少首次访问延迟
3. **亲和性调度**: 让计算任务靠近数据缓存
4. **缓存清理**: 定期清理过期缓存释放空间
5. **监控告警**: 监控缓存命中率和使用量

---

## 参考资源

- [官方文档](https://fluid-cloudnative.github.io)
- [GitHub Repo](https://github.com/fluid-cloudnative/fluid)
- [用户指南](https://fluid-cloudnative.github.io/docs/user-guide/)
- [示例](https://github.com/fluid-cloudnative/fluid/tree/master/samples)

---

**维护者**: Kudig Team | **许可证**: MIT

---

## 生产级部署架构

> 适用版本: Fluid v1.0+ | Kubernetes v1.28-v1.33

### JuiceFS Runtime vs Alluxio Runtime 选型对比

| 维度 | Alluxio Runtime | JuiceFS Runtime |
|:---|:---|:---|
| **底层存储** | POSIX / S3 / HDFS / OSS | Redis / TiKV / MySQL (元数据) + S3 / OSS / HDFS (数据) |
| **缓存粒度** | Block 级 (默认 64MB) | Block 级 (默认 4MB) |
| **元数据管理** | Alluxio Master (RocksDB) | 外部数据库 (Redis/TiKV) |
| **POSIX 兼容** | FUSE / JNI | FUSE (完全 POSIX) |
| **多租户隔离** | 通过 Dataset 隔离 | 通过不同 JuiceFS Volume 隔离 |
| **小文件性能** | 优秀 (元数据缓存) | 极优 (元数据引擎独立扩展) |
| **大规模集群** | Master 单点瓶颈需 HA | 元数据引擎水平扩展 |
| **运维复杂度** | 中 (需维护 Master/Worker) | 低 (依赖外部数据库) |
| **适用场景** | 通用大数据 + AI 训练 | AI 训练 (海量小文件)、多云数据共享 |
| **社区活跃度** | CNCF Incubating 核心 Runtime | 社区插件，JuiceFS 商业版功能更全 |

**选型建议**:
- 海量小文件 (如 ImageNet 128万张图片) -> JuiceFS Runtime
- 大文件顺序读取 (如视频、日志) -> Alluxio Runtime
- 需要与已有 HDFS/Hive 集成 -> Alluxio Runtime
- 多云 / 混合云数据共享 -> JuiceFS Runtime

### 数据预热 (Dataset CRD) 配置示例

```yaml
# dataset-with-warmup.yaml — 完整生产级配置 (K8s v1.28-v1.33)
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: imagenet-dataset
  namespace: ml-training
spec:
  mounts:
    - mountPoint: s3://ml-datasets/imagenet/
      name: imagenet
      options:
        fs.s3a.access.key: "${AWS_ACCESS_KEY_ID}"
        fs.s3a.secret.key: "${AWS_SECRET_ACCESS_KEY}"
        fs.s3a.endpoint: "s3.cn-northwest-1.amazonaws.com.cn"
        alluxio.underfs.s3.inherit.acl: "false"
      encryptOptions:
        - name: fs.s3a.secret.key
          valueFrom:
            secretKeyRef:
              name: s3-credentials
              key: secret-key
  # 数据预热策略
  dataLoad:
    # 启动时自动加载元数据
    loadMetadata: true
    target:
      - path: /train
        replicas: 4          # 并行预热 Pod 数
      - path: /validation
        replicas: 2
    # 节点选择器 — 预热到 GPU 节点
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
          - matchExpressions:
              - key: node-role/gpu
                operator: In
                values: ["true"]
  # 数据缓存位置偏好
  placement: "CoLocation"    # 缓存与计算共置
```

### 缓存策略配置

#### ReadOnly (只读缓存 — 推荐训练场景)

```yaml
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: training-readonly
spec:
  # ReadOnly: 数据只从远端读取, 写入只到缓存层
  accessModes:
    - ReadOnlyMany
  mounts:
    - mountPoint: s3://datasets/training-data/
      name: s3-data
      readOnly: true
---
apiVersion: data.fluid.io/v1alpha1
kind: AlluxioRuntime
metadata:
  name: training-readonly
spec:
  replicas: 4
  data:
    # 缓存数据不回写远端
    cacheable: true
  tieredstore:
    levels:
      - mediumtype: MEM
        path: /dev/shm
        quota: 16Gi
        high: "0.95"
        low: "0.7"
      - mediumtype: SSD
        path: /mnt/ssd
        quota: 200Gi
        high: "0.90"
        low: "0.6"
```

#### ReadWrite (读写缓存 — 数据预处理场景)

```yaml
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: preprocessing-rw
spec:
  accessModes:
    - ReadWriteMany
  mounts:
    - mountPoint: s3://datasets/raw-data/
      name: raw
    - mountPoint: s3://datasets/processed-data/
      name: processed
      options:
        # 启用写透 — 写入同时回写远端
        alluxio.user.file.writetype.default: "CACHE_THROUGH"
```

#### WriteBack (回写缓存 — 高性能写入场景)

```yaml
apiVersion: data.fluid.io/v1alpha1
kind: AlluxioRuntime
metadata:
  name: writeback-runtime
spec:
  replicas: 3
  data:
    # 写入缓存后异步回写远端 (需配合定期 flush)
    writeback: true
  properties:
    alluxio.user.file.writetype.default: "ASYNC_THROUGH"
    alluxio.user.file.underfs.hdfs.configuration: "/etc/alluxio/conf/core-site.xml"
```

### 多数据源接入配置

#### HDFS 数据源

```yaml
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: hdfs-dataset
spec:
  mounts:
    - mountPoint: hdfs://namenode:8020/data/warehouse/
      name: hdfs-warehouse
      options:
        alluxio.underfs.hdfs.configuration: "/hdfs-config/core-site.xml"
      # 挂载 HDFS 配置
      encryptOptions:
        - name: HADOOP_CONF_DIR
          valueFrom:
            configMapKeyRef:
              name: hdfs-config
              key: core-site.xml
---
# HDFS 配置 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: hdfs-config
data:
  core-site.xml: |
    <configuration>
      <property>
        <name>fs.defaultFS</name>
        <value>hdfs://namenode:8020</value>
      </property>
      <property>
        <name>hadoop.security.authentication</name>
        <value>kerberos</value>
      </property>
    </configuration>
```

#### S3 数据源

```yaml
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: s3-dataset
spec:
  mounts:
    - mountPoint: s3://data-bucket/datasets/
      name: s3-data
      options:
        fs.s3a.endpoint: "https://s3.cn-north-1.amazonaws.com.cn"
        fs.s3a.path.style.access: "true"
        fs.s3a.connection.ssl.enabled: "true"
        fs.s3a.multipart.size: "128MB"
        fs.s3a.connection.maximum: "200"
        fs.s3a.threads.max: "30"
      encryptOptions:
        - name: fs.s3a.access.key
          valueFrom:
            secretKeyRef:
              name: aws-s3-creds
              key: access-key
        - name: fs.s3a.secret.key
          valueFrom:
            secretKeyRef:
              name: aws-s3-creds
              key: secret-key
```

#### 阿里云 OSS 数据源

```yaml
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: oss-dataset
spec:
  mounts:
    - mountPoint: oss://oss-bucket-name/datasets/
      name: oss-data
      options:
        fs.oss.endpoint: "https://oss-cn-hangzhou.aliyuncs.com"
        fs.oss.accessKeyId: "${OSS_ACCESS_KEY_ID}"
        fs.oss.accessKeySecret: "${OSS_ACCESS_KEY_SECRET}"
        fs.oss.impl: "org.apache.hadoop.fs.aliyun.oss.AliyunOSSFileSystem"
        fs.AbstractFileSystem.oss.impl: "org.apache.hadoop.fs.aliyun.oss.OSS"
      encryptOptions:
        - name: fs.oss.accessKeyId
          valueFrom:
            secretKeyRef:
              name: oss-credentials
              key: access-key-id
        - name: fs.oss.accessKeySecret
          valueFrom:
            secretKeyRef:
              name: oss-credentials
              key: access-key-secret
```

---

## AI 训练场景加速

### PyTorch DataLoader + Fluid 加速配置

```yaml
# 1. 创建 Dataset + Runtime
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: cifar10-dataset
  namespace: ml-training
spec:
  mounts:
    - mountPoint: s3://ml-data/cifar10/
      name: cifar10
  dataLoad:
    loadMetadata: true
    target:
      - path: /
        replicas: 4
---
apiVersion: data.fluid.io/v1alpha1
kind: AlluxioRuntime
metadata:
  name: cifar10-dataset
  namespace: ml-training
spec:
  replicas: 4
  tieredstore:
    levels:
      - mediumtype: MEM
        path: /dev/shm
        quota: 8Gi
        high: "0.95"
        low: "0.7"
  fuse:
    resources:
      requests:
        cpu: 500m
        memory: 2Gi
      limits:
        cpu: "2"
        memory: 4Gi
---
# 2. PyTorch 训练 Job — 挂载 Fluid PVC
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: resnet50-training
  namespace: ml-training
spec:
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      template:
        spec:
          containers:
            - name: pytorch
              image: pytorch/pytorch:2.3-cuda12.1-cudnn8-devel
              command: ["python", "/workspace/train.py"]
              volumeMounts:
                - name: dataset
                  mountPath: /data
                  readOnly: true
              resources:
                limits:
                  nvidia.com/gpu: "1"
          volumes:
            - name: dataset
              persistentVolumeClaim:
                claimName: cifar10-dataset
    Worker:
      replicas: 4
      template:
        spec:
          containers:
            - name: pytorch
              image: pytorch/pytorch:2.3-cuda12.1-cudnn8-devel
              command: ["python", "/workspace/train.py"]
              volumeMounts:
                - name: dataset
                  mountPath: /data
                  readOnly: true
              resources:
                limits:
                  nvidia.com/gpu: "1"
          volumes:
            - name: dataset
              persistentVolumeClaim:
                claimName: cifar10-dataset
```

**PyTorch DataLoader 代码适配**:

```python
import torch
from torch.utils.data import DataLoader, Dataset
from torchvision import datasets, transforms
import os

class FluidCachedDataset(Dataset):
    """从 Fluid 挂载路径读取数据, 首次访问触发缓存"""
    def __init__(self, root_dir, transform=None):
        self.root_dir = root_dir
        self.transform = transform
        self.samples = self._load_samples()

    def _load_samples(self):
        samples = []
        for class_dir in sorted(os.listdir(self.root_dir)):
            class_path = os.path.join(self.root_dir, class_dir)
            if os.path.isdir(class_path):
                for fname in sorted(os.listdir(class_path)):
                    if fname.endswith(('.jpg', '.png', '.JPEG')):
                        samples.append((os.path.join(class_path, fname), class_dir))
        return samples

    def __len__(self):
        return len(self.samples)

    def __getitem__(self, idx):
        from PIL import Image
        path, label = self.samples[idx]
        image = Image.open(path).convert('RGB')
        if self.transform:
            image = self.transform(image)
        return image, int(label)

# 使用 Fluid PVC 路径
train_dataset = FluidCachedDataset(
    root_dir="/data/train",  # Fluid PVC 挂载路径
    transform=transforms.Compose([
        transforms.RandomResizedCrop(224),
        transforms.RandomHorizontalFlip(),
        transforms.ToTensor(),
        transforms.Normalize([0.485, 0.456, 0.406], [0.229, 0.224, 0.225]),
    ])
)

train_loader = DataLoader(
    train_dataset,
    batch_size=64,
    shuffle=True,
    num_workers=8,        # 多进程并行读取
    pin_memory=True,      # 锁页内存加速 GPU 传输
    prefetch_factor=4,    # 预取批次
    persistent_workers=True,
)
```

### 大模型训练数据预热方案

```yaml
# 大模型预训练数据预热 — 分片并行加载
apiVersion: data.fluid.io/v1alpha1
kind: DataLoad
metadata:
  name: llm-pretrain-warmup
  namespace: ml-training
spec:
  dataset:
    name: llm-pretrain-data
    namespace: ml-training
  loadMetadata: true
  target:
    # 预训练数据通常为大文件, 按路径分片预热
    - path: /shard-00
      replicas: 8
    - path: /shard-01
      replicas: 8
    - path: /shard-02
      replicas: 8
    - path: /shard-03
      replicas: 8
  # 资源限制 — 避免预热影响在线服务
  resources:
    requests:
      cpu: 500m
      memory: 1Gi
    limits:
      cpu: "2"
      memory: 4Gi
  # 节点调度策略
  nodeAffinity:
    preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        preference:
          matchExpressions:
            - key: node-role/gpu
              operator: In
              values: ["true"]
  # 超时设置
  ttlSecondsAfterFinished: 3600
```

```yaml
# LLM 预训练 Dataset 完整配置
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: llm-pretrain-data
  namespace: ml-training
spec:
  mounts:
    - mountPoint: s3://llm-data/pretrain/commoncrawl-2024/
      name: commoncrawl
      options:
        alluxio.underfs.s3.multipart.upload.size: "64MB"
        alluxio.underfs.s3.request.timeout: "600000"
        alluxio.underfs.s3.socket.timeout: "600000"
    - mountPoint: s3://llm-data/pretrain/wikipedia/
      name: wikipedia
  owner:
    # 告警配置
    syncPolicy:
      - schedule: "*/5 * * * *"
---
apiVersion: data.fluid.io/v1alpha1
kind: JuiceFSRuntime
metadata:
  name: llm-pretrain-data
  namespace: ml-training
spec:
  replicas: 8
  tieredstore:
    levels:
      - mediumtype: MEM
        path: /dev/shm
        quota: 32Gi
      - mediumtype: SSD
        path: /mnt/nvme
        quota: 2Ti
  fuse:
    image: juicedata/juicefs-csi-driver
    imageTag: v1.2.0
    resources:
      requests:
        cpu: "2"
        memory: 8Gi
      limits:
        cpu: "8"
        memory: 16Gi
```

### 与 Volcano 调度器集成

```yaml
# Volcano Gang Scheduling + Fluid 数据感知
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: ml-training-queue
spec:
  weight: 1
  guarantee:
    resource:
      cpu: "32"
      memory: 128Gi
      nvidia.com/gpu: "8"
---
apiVersion: batch.volcano.sh/v1alpha1
kind: Job
metadata:
  name: distributed-training
  namespace: ml-training
spec:
  minAvailable: 4       # Gang Scheduling 最小 Pod 数
  schedulerName: volcano
  queue: ml-training-queue
  policies:
    - event: PodEvicted
      action: RestartJob
  plugins:
    ssh: []
    env: []
    svc: []
  tasks:
    - replicas: 4
      name: worker
      template:
        spec:
          containers:
            - name: trainer
              image: pytorch/pytorch:2.3-cuda12.1-cudnn8-devel
              command: ["torchrun", "--nproc_per_node=1", "/workspace/train.py"]
              volumeMounts:
                - name: dataset
                  mountPath: /data
                  readOnly: true
              resources:
                limits:
                  nvidia.com/gpu: "1"
                  cpu: "8"
                  memory: 32Gi
          volumes:
            - name: dataset
              persistentVolumeClaim:
                claimName: llm-pretrain-data  # Fluid 自动创建
          # Fluid 注入数据亲和性 — 自动调度到缓存节点
```

### 与 Kueue 集成

```yaml
# Kueue 本地队列 + Fluid 数据集
apiVersion: kueue.x-k8s.io/v1beta1
kind: LocalQueue
metadata:
  name: ml-queue
  namespace: ml-training
spec:
  clusterQueue: gpu-cluster-queue
  resourceGroups:
    - coveredResources: ["cpu", "memory", "nvidia.com/gpu"]
---
apiVersion: kueue.x-k8s.io/v1beta1
kind: WorkloadPriorityClass
metadata:
  name: training-priority
value: 1000
globalDefault: false
description: "ML training workloads with Fluid data caching"
```

### 性能基准 (参考值)

| 场景 | 无 Fluid | Alluxio Runtime | JuiceFS Runtime | 加速比 |
|:---|:---|:---|:---|:---|
| **CIFAR-10 (60K 图片)** 数据加载 | 45s | 3s | 2.5s | 15-18x |
| **ImageNet (1.28M 图片)** 首次加载 | 12min | 2min | 1.5min | 6-8x |
| **ImageNet** 缓存命中后 | 12min | 8s | 6s | 90-120x |
| **BERT 预训练数据 (1TB)** | 45min | 8min | 6min | 5.6-7.5x |
| **多 Worker 并发读取** (x8) | I/O 瓶颈显著 | 无明显瓶颈 | 无明显瓶颈 | 有效消除 I/O 争用 |

> 注: 测试环境为 8x A100 GPU 节点, NVMe SSD 缓存, S3 数据源, 网络带宽 25Gbps。实际性能受网络、存储、数据特征影响。

---

## 大数据场景加速

### Spark on K8s + Fluid 缓存加速

```yaml
# 1. 为 Spark 数据创建 Fluid Dataset
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: spark-warehouse
  namespace: spark
spec:
  mounts:
    - mountPoint: hdfs://namenode:8020/warehouse/
      name: hive-warehouse
    - mountPoint: s3://data-lake/iceberg/
      name: iceberg-tables
  dataLoad:
    loadMetadata: true
---
apiVersion: data.fluid.io/v1alpha1
kind: AlluxioRuntime
metadata:
  name: spark-warehouse
  namespace: spark
spec:
  replicas: 6
  tieredstore:
    levels:
      - mediumtype: MEM
        path: /dev/shm
        quota: 64Gi
        high: "0.9"
        low: "0.6"
      - mediumtype: SSD
        path: /mnt/ssd1,/mnt/ssd2    # 多盘加速
        quota: 2Ti
        high: "0.85"
        low: "0.5"
  # Spark 特定优化
  properties:
    # Shuffle 数据缓存
    alluxio.worker.network.netty.file.transfer: "MAPPED"
    # 大文件顺序读优化
    alluxio.user.streaming.data.size: "64MB"
    alluxio.user.file.readtype.default: "CACHE"
```

```yaml
# 2. SparkApplication 使用 Fluid 缓存
apiVersion: sparkoperator.k8s.io/v1beta2
kind: SparkApplication
metadata:
  name: etl-pipeline
  namespace: spark
spec:
  type: Scala
  mode: cluster
  image: spark:3.5.1
  mainClass: com.example.ETLPipeline
  mainApplicationFile: local:///opt/spark/jobs/etl.jar
  sparkVersion: "3.5.1"
  driver:
    cores: 2
    memory: "4g"
    volumeMounts:
      - name: warehouse
        mountPath: /data/warehouse
        readOnly: true
  executor:
    cores: 4
    memory: "8g"
    instances: 12
    volumeMounts:
      - name: warehouse
        mountPath: /data/warehouse
        readOnly: true
  volumes:
    - name: warehouse
      persistentVolumeClaim:
        claimName: spark-warehouse
  sparkConf:
    # 指向 Fluid 缓存路径
    "spark.sql.warehouse.dir": "/data/warehouse"
    "spark.hadoop.fs.defaultFS": "file:///"
    # 开启列裁剪 + 谓词下推
    "spark.sql.parquet.filterPushdown": "true"
    "spark.sql.parquet.mergeSchema": "false"
```

### Trino/Presto 查询加速

```yaml
# Trino Worker 挂载 Fluid 缓存数据
apiVersion: apps/v1
kind: Deployment
metadata:
  name: trino-worker
  namespace: analytics
spec:
  replicas: 6
  selector:
    matchLabels:
      app: trino-worker
  template:
    metadata:
      labels:
        app: trino-worker
    spec:
      containers:
        - name: trino
          image: trinodb/trino:443
          args: ["--config", "/etc/trino/config.properties"]
          volumeMounts:
            - name: hive-cache
              mountPath: /data/hive
              readOnly: true
            - name: iceberg-cache
              mountPath: /data/iceberg
              readOnly: true
          resources:
            requests:
              cpu: "4"
              memory: 16Gi
            limits:
              cpu: "8"
              memory: 32Gi
      volumes:
        - name: hive-cache
          persistentVolumeClaim:
            claimName: spark-warehouse   # 复用 Fluid PVC
        - name: iceberg-cache
          persistentVolumeClaim:
            claimName: spark-warehouse
      # Fluid 自动注入数据亲和性 — Worker 调度到缓存节点
```

### 数据本地性优化

```yaml
# 强制数据本地性 — 只调度到有缓存的节点
apiVersion: data.fluid.io/v1alpha1
kind: Dataset
metadata:
  name: hot-data
spec:
  # 强制本地性 — 如果节点无缓存则等待而非回源
  placement: "CoLocation"
  # 缓存分片策略 — 均匀分布到所有 Worker 节点
  dataCacheAffinity:
    preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        podAffinityTerm:
          labelSelector:
            matchExpressions:
              - key: app
                operator: In
                values: ["data-worker"]
          topologyKey: kubernetes.io/hostname
```

**Spark 数据本地性调优**:

```yaml
# spark-conf 配合 Fluid 本地性
sparkConf:
  # 调度优先级: PROCESS_LOCAL > NODE_LOCAL > RACK_LOCAL > ANY
  "spark.locality.wait": "3s"
  "spark.locality.wait.node": "3s"
  "spark.locality.wait.process": "1s"
  # 开启 External Shuffle Service 减少 Executor 依赖
  "spark.shuffle.service.enabled": "true"
  # 动态资源分配配合 Fluid 弹性缓存
  "spark.dynamicAllocation.enabled": "true"
  "spark.dynamicAllocation.minExecutors": "2"
  "spark.dynamicAllocation.maxExecutors": "24"
  "spark.dynamicAllocation.executorIdleTimeout": "60s"
```

---

## 故障排查与调优

### 常见问题排查

#### Dataset 状态 Pending

```bash
# 1. 检查 Dataset 状态
kubectl describe dataset <name> -n <namespace>

# 常见原因:
# - mountPoint 配置错误 (网络不通 / 凭证失效)
# - PVC 未绑定 (StorageClass 问题)
# - Runtime 未就绪

# 2. 检查 Runtime Pod 状态
kubectl get pods -n <namespace> -l app=<dataset-name>
kubectl logs <master-pod> -n <namespace> --tail=100

# 3. 检查 FUSE Pod 状态
kubectl get pods -n <namespace> -l role=alluxio-fuse
kubectl logs <fuse-pod> -n <namespace> --tail=50

# 4. 检查数据源连通性
kubectl exec -it <master-pod> -n <namespace> -- \
  alluxio fs mount | grep <mount-name>
kubectl exec -it <master-pod> -n <namespace> -- \
  alluxio fs ls /<mount-path>
```

#### Cache Miss (缓存未命中)

```bash
# 1. 检查缓存命中率
kubectl exec -it <master-pod> -n <namespace> -- \
  alluxio fsadmin report metrics

# 2. 检查缓存空间
kubectl exec -it <master-pod> -n <namespace> -- \
  alluxio fsadmin report capacity

# 3. 常见原因:
# - Worker 缓存空间不足 -> 增大 tieredstore quota
# - 文件被逐出 -> 调整 high/low 水位线
# - 数据未预热 -> 执行 DataLoad
# - Worker 数量不足 -> 增加 replicas

# 4. 手动触发预热
kubectl apply -f - <<EOF
apiVersion: data.fluid.io/v1alpha1
kind: DataLoad
metadata:
  name: re-warmup
spec:
  dataset:
    name: <dataset-name>
    namespace: <namespace>
  target:
    - path: /<hot-path>
      replicas: 4
EOF
```

#### Runtime 异常

```bash
# 1. 检查 Runtime 事件
kubectl get events -n <namespace> --field-selector involvedObject.name=<runtime-name>

# 2. 检查 Worker 健康状态
kubectl get alluxioruntime <name> -n <namespace> -o jsonpath='{.status.cacheStates}'

# 3. Worker OOM 排查
kubectl top pods -n <namespace> -l app=<dataset-name>
kubectl describe pod <worker-pod> -n <namespace> | grep -A5 "Last State"

# 4. Master 选主失败 (HA 模式)
kubectl get endpoints <dataset-name>-master -n <namespace>
kubectl logs <master-pod> -n <namespace> | grep -i "election\|leader"

# 5. FUSE 挂载失败
kubectl get csidrivers
kubectl logs -n kube-system -l app=csi-nodeplugin-fluid
```

### 监控指标

**Prometheus 指标 (端口 19999)**:

| 指标名称 | 类型 | 说明 | 告警阈值建议 |
|:---|:---|:---|:---|
| `fluid_dataset_data_read_throughput_bytes` | Gauge | 数据读取吞吐量 (B/s) | < 100MB/s 需排查 |
| `fluid_dataset_cache_hit_ratio` | Gauge | 缓存命中率 (0-1) | < 0.8 告警 |
| `fluid_dataset_cache_usage_bytes` | Gauge | 缓存已用空间 (Bytes) | > 90% quota 告警 |
| `fluid_dataset_cache_capacity_bytes` | Gauge | 缓存总容量 (Bytes) | — |
| `fluid_runtime_worker_count` | Gauge | 运行中 Worker 数 | < 期望 replicas 告警 |
| `fluid_runtime_master_status` | Gauge | Master 状态 (0/1) | = 0 严重告警 |

**Grafana Dashboard 配置**:

```yaml
# ServiceMonitor for Fluid metrics
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: fluid-monitor
  namespace: fluid-system
spec:
  selector:
    matchLabels:
      app: alluxio-master
  endpoints:
    - port: metrics
      interval: 15s
      path: /metrics/prometheus
  namespaceSelector:
    matchNames:
      - ml-training
      - spark
      - analytics
```

### 调优参数

| 参数 | 位置 | 说明 | 推荐值 |
|:---|:---|:---|:---|
| `spec.replicas` | Runtime | Worker 副本数 | GPU 节点数 / 2 |
| `spec.data.cacheable` | Runtime | 是否缓存 | `true` (训练场景) |
| `spec.placement` | Dataset | 缓存与计算共置策略 | `CoLocation` |
| `spec.tieredstore.levels[].quota` | Runtime | 每级缓存容量 | MEM: 50% 内存, SSD: 磁盘 80% |
| `spec.tieredstore.levels[].high` | Runtime | GC 触发水位 | `0.90` - `0.95` |
| `spec.tieredstore.levels[].low` | Runtime | GC 停止水位 | `0.60` - `0.70` |
| `spec.fuse.resources.limits.cpu` | Runtime | FUSE CPU 限制 | `2` - `4` 核 |
| `spec.fuse.resources.limits.memory` | Runtime | FUSE 内存限制 | `4Gi` - `8Gi` |
| `alluxio.user.file.readtype.default` | Runtime Properties | 读策略 | `CACHE` (训练) / `NO_CACHE` (只查元数据) |
| `alluxio.user.file.writetype.default` | Runtime Properties | 写策略 | `CACHE_THROUGH` (读写) / `ASYNC_THROUGH` (高性能写) |
| `alluxio.worker.network.netty.file.transfer` | Runtime Properties | 文件传输方式 | `MAPPED` (大文件) / `TRANSFER` (小文件) |

---

## 生产检查清单

| # | 检查项 | 类别 | 期望状态 | 检查方法 |
|:---:|:---|:---|:---|:---|
| 1 | Fluid 组件版本与 K8s 版本兼容 | 安装 | 版本矩阵匹配 | `helm list -n fluid-system` |
| 2 | Dataset 状态为 Bound | 资源 | `PHASE=Bound` | `kubectl get dataset -A` |
| 3 | Runtime 所有 Worker 就绪 | 资源 | `Ready=replicas` | `kubectl get alluxioruntime -A` |
| 4 | FUSE Pod 在所有工作节点运行 | 资源 | DaemonSet 全覆盖 | `kubectl get ds -n <ns>` |
| 5 | 缓存命中率 > 80% | 监控 | 持续达标 | Prometheus / Grafana |
| 6 | 缓存使用率 < 90% | 监控 | 无 OOM 风险 | Prometheus / Grafana |
| 7 | 数据预热任务完成 | 运维 | DataLoad Complete | `kubectl get dataload -A` |
| 8 | S3/HDFS 凭证已加密存储 | 安全 | 使用 encryptOptions + Secret | `kubectl get secret -A` |
| 9 | Runtime HA 配置 (Master > 1) | 高可用 | master.replicas >= 3 | Runtime spec |
| 10 | FUSE 资源限制已配置 | 稳定性 | CPU/Memory limits | Runtime spec |
| 11 | PVC StorageClass 配置正确 | 存储 | WaitForFirstConsumer | `kubectl get pvc -A` |
| 12 | 网络策略允许 Runtime 通信 | 安全 | Master-Worker-FUSE 互通 | NetworkPolicy |
| 13 | Prometheus ServiceMonitor 已部署 | 监控 | 指标正常采集 | `kubectl get servicemonitor -A` |
| 14 | 告警规则已配置 (缓存/命中率) | 监控 | 告警规则存在 | PrometheusRule |
| 15 | 节点标签与数据亲和性匹配 | 调度 | GPU 节点有缓存 | `kubectl get nodes --show-labels` |
| 16 | 存储卷 IOPS 满足需求 | 性能 | NVMe > 50K IOPS | 节点 benchmark |
| 17 | Fluid Operator 健康 | 健康 | Pod Running/Ready | `kubectl get pods -n fluid-system` |
| 18 | 定期清理过期缓存 | 运维 | TTL 或 CronJob 已配置 | `kubectl get cronjob -A` |
| 19 | 备份策略 (DataBackup) 配置 | 容灾 | 关键数据集有备份 | `kubectl get databackup -A` |
| 20 | 文档记录 Runtime 选型理由 | 文档 | 架构决策文档齐全 | 内部 Wiki |

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/sql.md|sql]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/ai-gpu-index|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
