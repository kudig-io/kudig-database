---
title: "存储 × AI 工作负载 × 数据管线"
summary: "AI 训练与推理对存储系统的极端 I/O 需求重塑了 K8s 存储架构：从 Checkpoint 写入到模型 Artifact 管理，数据管线决定了 AI 平台的效率上限"
category: synthesis
tags:
- storage
- ai-workload
- data-pipeline
- checkpoint
- model-artifact
- csi
- training
tier: supporting
sources:
- 概念/cloud-native-storage-systems.md
- 概念/csi-drivers.md
- 概念/k8s-ai-ml-infrastructure.md
- 概念/gpu-scheduling-ai-workloads.md
- 实体/rook.md
- 实体/longhorn.md
created: '2026-07-19'
updated: '2026-07-19'
last_updated: '2026-07-19'
---

# 存储 × AI 工作负载 × 数据管线

## The Connection（为什么这两个领域交叉）

AI 工作负载（训练、微调、推理）对存储系统的需求与传统 Web 服务截然不同。训练任务需要高吞吐的顺序读取（TB 级数据集）、高带宽的 Checkpoint 写入（每次数十 GB 模型参数）、低延迟的元数据操作（数百万小文件的随机访问）。推理服务需要快速加载模型权重（冷启动时间直接影响 SLA）、高并发的特征查询、流式的日志写入。

Kubernetes 的存储抽象（PV/PVC/StorageClass/CSI）为 AI 工作负载提供了声明式的存储接入，但底层存储系统的选型和配置直接决定了 AI 平台的效率上限。一个常见的瓶颈是：GPU 利用率只有 30-50%，不是因为计算不足，而是因为数据加载（I/O）跟不上计算速度——GPU 在等待数据。

数据管线（Data Pipeline）连接了存储与计算：数据收集 → 清洗 → 特征工程 → 训练数据准备 → 模型训练 → Checkpoint 保存 → 模型评估 → 模型部署 → 推理服务。每个环节都有特定的存储需求，存储选型错误会在整个管线中产生级联性能问题。

## Where They Co-occur（生产中的交叉场景）

### 场景一：大规模训练数据加载

LLM 预训练数据集达数十 TB（如 Common Crawl 清洗后）。训练框架（PyTorch DataLoader、NVIDIA DALI）需要从存储系统以 >10 GB/s 的吞吐读取数据。本地 NVMe 容量有限，网络存储（NFS、Lustre、GPFS、对象存储）成为必然选择。K8s 中通过 CSI 驱动挂载高性能并行文件系统，或通过 FUSE 客户端（s3fs、juicefs）接入对象存储。

### 场景二：Checkpoint 存储与恢复

大模型训练每隔 N 步保存 Checkpoint（模型参数 + 优化器状态），单次 Checkpoint 可达 50-200 GB。Checkpoint 写入不能阻塞训练太久（否则 GPU 空闲浪费），需要高带宽写入存储。同时 Checkpoint 需要持久保存（训练中断后恢复），通常写入对象存储（S3/OSS）或分布式文件系统。K8s 中 Checkpoint 通常写入 PVC（高性能）+ 异步同步到对象存储（持久化）。

### 场景三：模型 Artifact 管理

训练产出的模型文件（权重、tokenizer、配置）需要版本化管理和快速分发。模型仓库（MLflow Model Registry、Hugging Face Hub）存储模型 Artifact，推理服务启动时拉取。K8s 中模型文件可通过 PVC（预加载）、Init Container（启动时下载）、或 CSI 驱动（按需加载）获取。大模型（>10 GB）的冷启动时间是推理服务弹性的关键瓶颈。

### 场景四：特征存储与在线服务

Feature Store（Feast/Tecton）维护离线特征（训练用，存储在数据湖）和在线特征（推理用，存储在 Redis/DynamoDB）。训练时从离线存储批量读取特征，推理时从在线存储低延迟查询。K8s 中 Feature Store 的在线服务需要高可用、低延迟的存储后端，离线计算需要高吞吐的数据湖接入。

### 场景五：数据版本化与可复现性

ML 实验可复现性要求：相同的数据 + 相同的代码 = 相同的结果。数据版本化工具（DVC、LakeFS、Delta Lake）跟踪数据集版本，与 Git 代码版本关联。K8s 中训练 Job 通过 PVC 挂载特定版本的数据快照，确保实验可复现。

### 场景六：多租户存储隔离

AI 平台服务多个团队，每个团队有独立的数据集、模型和实验。存储层面需要隔离：不同团队的 PVC 不能互相访问，配额限制防止单团队耗尽存储资源。K8s 的 StorageClass + ResourceQuota + NetworkPolicy 组合实现存储层多租户隔离。

## Production Patterns（生产模式与架构）

### 模式一：分层存储架构

```
┌─────────────────────────────────────────────────────────┐
│  AI Platform Storage Architecture                        │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  Hot Tier (本地 NVMe / 高性能并行文件系统)              │
│  ├── 训练数据缓存 (当前 epoch 数据)                    │
│  ├── Checkpoint 写入 (高带宽)                          │
│  └── 模型权重加载 (低延迟)                             │
│  性能: >10 GB/s 吞吐, <1ms 延迟                       │
│                                                         │
│  Warm Tier (分布式文件系统 / 对象存储)                  │
│  ├── 完整训练数据集                                    │
│  ├── Checkpoint 持久化                                 │
│  ├── 模型 Artifact 仓库                                │
│  └── 特征数据 (离线)                                   │
│  性能: 1-10 GB/s 吞吐, 10-100ms 延迟                 │
│                                                         │
│  Cold Tier (归档对象存储)                               │
│  ├── 历史实验数据                                      │
│  ├── 过期 Checkpoint                                   │
│  └── 合规归档                                          │
│  性能: 按需访问, 秒级延迟                              │
│                                                         │
│  数据流动:                                              │
│  Cold → Warm: 训练前预加载 (提前数小时)                │
│  Warm → Hot: 训练时缓存 (DataLoader prefetch)          │
│  Hot → Warm: Checkpoint 异步上传                       │
│  Warm → Cold: 生命周期策略 (30天后归档)                │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

### 模式二：CSI 驱动选型

```yaml
# 高性能训练：Lustre/GPFS CSI
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: lustre-training
provisioner: lustre.csi.k8s.io
parameters:
  server: "lustre-mgs.internal"
  filesystem: "training-fs"
  subpath: "/datasets"
reclaimPolicy: Retain
volumeBindingMode: Immediate
---
# Checkpoint 存储：Ceph RBD (Rook)
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ceph-checkpoint
provisioner: rook-ceph.rbd.csi.ceph.com
parameters:
  clusterID: rook-ceph
  pool: checkpoint-pool
  imageFormat: "2"
  imageFeatures: layering
reclaimPolicy: Retain
allowVolumeExpansion: true
---
# 模型 Artifact：对象存储 CSI (S3/OSS)
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: model-artifacts
provisioner: s3.csi.aws.com
parameters:
  bucketName: model-registry
  prefix: artifacts/
```

### 模式三：Checkpoint 写入优化

```python
# 训练代码中的 Checkpoint 策略
# 1. 异步写入：不阻塞训练
# 2. 分层写入：先写本地 NVMe，再异步上传对象存储
# 3. 保留策略：只保留最近 N 个 Checkpoint

# K8s Job 配置
apiVersion: batch/v1
kind: Job
metadata:
  name: llm-training
spec:
  template:
    spec:
      containers:
      - name: trainer
        image: training:v1
        volumeMounts:
        - name: local-nvme
          mountPath: /checkpoint/local  # 快速写入
        - name: shared-storage
          mountPath: /data              # 训练数据
        resources:
          limits:
            nvidia.com/gpu: 8
      volumes:
      - name: local-nvme
        emptyDir:
          medium: ""  # 使用节点本地 NVMe
          sizeLimit: 500Gi
      - name: shared-storage
        persistentVolumeClaim:
          claimName: training-dataset-pvc
```

### 模式四：模型加载优化（推理冷启动）

```
方案对比:
  1. PVC 预加载: 模型文件预存 PVC，Pod 启动直接挂载
     冷启动: <5s (本地读取)
     适用: 固定模型、少量版本

  2. Init Container 下载: 启动时从对象存储下载
     冷启动: 30s-5min (取决于模型大小和网络)
     适用: 多版本模型、按需加载

  3. 模型缓存 DaemonSet: 每节点预缓存热门模型
     冷启动: <5s (本地缓存命中)
     适用: 推理服务、多模型场景

  4. 流式加载 (Tensorizer/CoreWeave): 边下载边加载
     冷启动: 10-30s (首 token 延迟)
     适用: 超大模型 (>50GB)

  推荐: 模型缓存 DaemonSet + PVC 预加载 (热门模型)
       Init Container 下载 (长尾模型)
```

### 模式五：数据管线编排

```
数据管线 (Kubeflow Pipelines / Argo Workflows):

  数据采集 → 数据清洗 → 特征工程 → 数据验证 → 训练数据准备
     │           │           │           │            │
     ▼           ▼           ▼           ▼            ▼
  对象存储   Spark/Flink  Feature    Great        TFRecord/
  (S3/OSS)  (计算集群)   Store      Expectations  Parquet
                                        │            │
                                        ▼            ▼
                                    数据质量报告  训练 PVC
```

## Trade-offs & Decision Matrix（权衡与决策）

| 维度 | 本地 NVMe | Ceph (Rook) | Lustre/GPFS | 对象存储 (S3) | JuiceFS |
|------|----------|-------------|-------------|--------------|---------|
| 吞吐量 | >20 GB/s | 1-5 GB/s | 10-100 GB/s | 1-10 GB/s | 1-5 GB/s |
| 延迟 | <0.1ms | 1-5ms | 1-10ms | 10-100ms | 1-10ms |
| 容量 | 受限于节点 | PB 级 | PB 级 | 无限 | 无限 |
| 持久性 | 节点故障丢失 | 3 副本 | 取决于配置 | 11个9 | 取决于后端 |
| 共享访问 | 不支持 | RWX 支持 | RWX 原生 | RWX 原生 | RWX 原生 |
| 成本 | 高（本地盘） | 中 | 高（专用硬件） | 低 | 中 |
| K8s 集成 | emptyDir/hostPath | CSI (Rook) | CSI | CSI/FUSE | CSI |
| 适用场景 | Checkpoint 缓存 | 通用持久化 | 大规模训练 | 归档/Artifact | 混合场景 |

### 决策矩阵

- **训练数据读取（>10 TB）** → Lustre/GPFS（最高吞吐）或 JuiceFS（性价比）
- **Checkpoint 写入** → 本地 NVMe（快速写入）+ 异步上传对象存储（持久化）
- **模型 Artifact 存储** → 对象存储（S3/OSS）+ 本地缓存
- **特征存储（在线）** → Redis/DynamoDB（低延迟）
- **特征存储（离线）** → 数据湖（Delta Lake/Iceberg）
- **通用持久化（中小规模）** → Ceph (Rook)（功能全面）
- **成本敏感 + 共享访问** → JuiceFS（对象存储后端 + POSIX 接口）

## Anti-patterns & Pitfalls（反模式）

### 反模式一：所有数据都放对象存储

训练数据直接从 S3 读取，I/O 延迟成为瓶颈，GPU 利用率 <30%。对象存储的延迟（10-100ms）和吞吐限制（单连接 ~100 MB/s）无法满足训练需求。**正确做法**：训练前将数据预加载到高性能存储（Lustre/本地 NVMe），或使用 JuiceFS 等缓存层加速。

### 反模式二：Checkpoint 写入阻塞训练

同步写入 Checkpoint 到网络存储，每次写入耗时 30-60 秒，期间 GPU 完全空闲。**正确做法**：异步 Checkpoint（先写本地 NVMe，后台线程上传）；或使用 PyTorch 的 `torch.distributed.checkpoint` 并行写入。

### 反模式三：忽略存储配额和清理

训练实验产生大量 Checkpoint 和中间数据，PVC 和对象存储无限膨胀，成本失控。**正确做法**：设置 PVC 配额（ResourceQuota）；Checkpoint 保留策略（只保留最近 N 个）；对象存储生命周期规则（30 天后归档/删除）。

### 反模式四：模型文件与代码耦合

模型权重文件打包在容器镜像中，每次模型更新都要重新构建和推送数十 GB 的镜像。**正确做法**：模型文件存储在 PVC 或对象存储中，容器镜像只包含代码；通过 Init Container 或 CSI 挂载获取模型文件。

### 反模式五：训练和推理共用同一存储

训练的高吞吐 I/O 与推理的低延迟需求冲突，共享存储导致推理 P99 延迟抖动。**正确做法**：训练和推理使用不同的 StorageClass（不同后端或不同 QoS 配置）；推理模型使用本地缓存。

### 反模式六：忽略数据局部性

训练 Pod 调度到任意节点，但数据只预加载在特定节点。Pod 需要跨网络读取数据，吞吐下降 5-10 倍。**正确做法**：使用节点亲和性（nodeAffinity）将训练 Pod 调度到数据所在节点；或使用分布式文件系统（Lustre）消除局部性约束。

## Operational Checklist（运维检查清单）

### 存储基础设施

- [ ] 评估训练数据规模和 I/O 需求（吞吐量、IOPS、延迟）
- [ ] 选择存储后端：Lustre（大规模训练）/ Ceph（通用）/ 对象存储（归档）
- [ ] 部署 CSI 驱动并验证 RWX 支持（多 Pod 共享读取）
- [ ] 配置 StorageClass QoS（训练高吞吐 vs 推理低延迟）
- [ ] 设置 PVC 配额和 StorageQuota（按团队/项目）
- [ ] 配置对象存储生命周期规则（自动归档/删除）

### 训练工作负载

- [ ] 训练数据预加载到高性能存储（训练开始前）
- [ ] Checkpoint 策略：异步写入 + 保留最近 N 个 + 异步上传
- [ ] 数据加载优化：DataLoader num_workers、prefetch、内存映射
- [ ] 监控 GPU 利用率：如果 <70% 排查 I/O 瓶颈
- [ ] 训练 Job 配置节点亲和性（数据局部性）

### 推理工作负载

- [ ] 模型缓存策略：热门模型预加载到节点本地
- [ ] 冷启动优化：Init Container 并行下载、流式加载
- [ ] 模型版本管理：MLflow/Hugging Face Hub + 快速拉取
- [ ] 推理存储与训练存储隔离（不同 StorageClass）

### 监控与告警

- [ ] 存储使用率告警：PVC > 80%、对象存储桶 > 预算
- [ ] I/O 性能监控：吞吐、延迟、IOPS（per PVC）
- [ ] GPU 利用率与 I/O 关联分析（识别 I/O 瓶颈）
- [ ] Checkpoint 写入时间监控（> 60s 告警）
- [ ] 数据管线延迟监控（数据新鲜度）

## Related

- [[22-概念/04-存储/cloud-native-storage-systems.md|云原生存储系统]]
- [[22-概念/04-存储/csi-drivers.md|CSI 驱动]]
- [[22-概念/12-研究/k8s-ai-ml-infrastructure.md|K8s AI/ML 基础设施]]
- [[22-概念/07-调度与资源/gpu-scheduling-ai-workloads.md|GPU 调度与 AI 工作负载]]
- [[23-实体/05-存储/rook.md|Rook]]
- [[23-实体/05-存储/longhorn.md|Longhorn]]
- [[24-综合/01-AI与机器学习/gpu-scheduling-cost.md|GPU 调度 × 成本]]
- [[24-综合/01-AI与机器学习/ai-workload-cost-optimization-finops.md|AI 工作负载 × 成本优化 × FinOps]]
- [[24-综合/01-AI与机器学习/feature-store-rag-ml-platform.md|Feature Store × RAG × ML 平台]]
