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
