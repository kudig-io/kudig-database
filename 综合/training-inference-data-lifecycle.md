---
title: "训练 × 推理 × 数据生命周期全链路交叉"
summary: "AI 工作负载从数据准备、模型训练到推理服务的全生命周期在 Kubernetes 上的编排、存储和治理交叉点"
category: synthesis
tags:
- training
- inference
- data-lifecycle
- mlops
- gpu
- storage
tier: supporting
sources:
- 平台工程/构建/25-ai-platform-engineering.md
- 综合/gpu-scheduling-cost.md
- 平台工程/治理/18-gpu-cluster-governance-ai-platform.md
created: '2026-07-19'
updated: '2026-07-19'
last_updated: '2026-07-19'
---

# 训练 × 推理 × 数据生命周期全链路交叉

## The Connection

AI 工作负载的生命周期是一条从数据到价值的流水线：数据准备 → 模型训练 → 模型评估 → 推理部署 → 监控反馈 → 数据回流。在 Kubernetes 上，这条流水线的每个阶段都有独特的资源需求、存储模式和调度约束，而它们之间的**衔接点**——数据如何从存储流向训练、模型如何从训练流向推理、推理反馈如何回流为训练数据——是 AI 平台工程的核心挑战。

训练和推理是 AI 工作负载的两极：训练是批处理、GPU 密集、容错性高（checkpoint 恢复）；推理是在线服务、延迟敏感、高可用要求。数据是连接两极的纽带：训练数据的质量决定模型上限，推理产生的日志和反馈是下一轮训练的输入。在 K8s 上，这三者通过共享存储（PVC/对象存储）、模型仓库（Model Registry）和流水线编排（Argo Workflows/Kubeflow Pipelines）串联为闭环。^[inferred]

## Where They Co-occur

- **数据准备 → 训练**：数据集存储在共享文件系统（SFS/NAS/CephFS）或对象存储（S3/OSS），训练 Pod 通过 PVC 或 CSI 驱动挂载。数据版本管理（DVC/LakeFS）确保训练可复现。数据预处理（tokenization、augmentation）通常作为训练流水线的第一个 Step，与训练共享存储卷。

- **训练 → 模型仓库**：训练完成后，Checkpoint 文件（通常 10-100GB）写入模型仓库（MLflow Model Registry / S3 + 元数据）。模型仓库是训练和推理之间的"气闸"——只有通过评估的模型才能注册为可部署版本。

- **模型仓库 → 推理**：推理服务（vLLM/Triton/TGI）从模型仓库拉取模型文件，加载到 GPU 显存。模型更新通过 Canary 部署（KServe InferenceService 的 canaryTrafficPercent）渐进切换。模型文件通常通过 initContainer 预下载到本地 SSD，避免推理启动时的网络延迟。

- **推理 → 数据回流**：推理服务产生的日志（请求/响应/延迟/用户反馈）通过 Fluent Bit 收集到数据湖，经清洗和标注后成为下一轮训练的数据集。这是"数据飞轮"的核心：推理越多 → 数据越多 → 模型越好 → 推理越多。

- **GPU 资源在训练/推理间的流动**：白天推理负载高（用户活跃），GPU 优先分配给推理；夜间推理负载低，GPU 释放给训练任务。通过 PriorityClass + 抢占机制实现：推理 Priority=1000000，训练 Priority=500000，推理可抢占训练的 GPU。

- **存储层次**：热数据（当前训练 batch）→ 本地 NVMe SSD；温数据（近期数据集）→ 分布式文件系统（CephFS/Lustre）；冷数据（历史 checkpoint）→ 对象存储（S3/OSS）。训练和推理共享温/冷存储，但热存储各自独立。

- **流水线编排**：Argo Workflows / Kubeflow Pipelines 将数据验证 → 训练 → 评估 → 注册 → 部署编排为 DAG，每个 Step 是独立的 Pod，通过 PVC 或 S3 传递中间产物。

- **多租户隔离**：不同团队的训练任务共享 GPU 集群但隔离存储（Namespace + PVC 配额）；推理服务按团队划分 InferenceService，通过 NetworkPolicy 隔离流量。^[inferred]

## Cross-cutting Insight

AI 全链路的核心矛盾是**训练和推理对基础设施的需求截然相反**：训练要"大吞吐、可中断、批处理"，推理要"低延迟、高可用、在线服务"。但两者共享同一套 GPU 硬件、同一套存储系统、同一个 K8s 集群。AI 平台工程的核心任务是在共享基础设施上为两种工作负载提供各自最优的体验——这需要通过调度策略（优先级/抢占/Gang）、存储分层（热/温/冷）、网络隔离（训练 RDMA vs 推理 HTTP）和治理机制（配额/成本/审计）四个维度协同设计。

数据生命周期是连接训练和推理的"第三条线"。没有数据回流，模型会退化（data drift）；没有版本管理，训练不可复现；没有质量门控，垃圾数据会污染模型。在 K8s 上，数据生命周期管理体现为：存储 CSI 驱动（数据访问）、Argo Workflows（流水线编排）、MLflow（版本管理）和 Evidently AI（质量监控）的组合。^[inferred]

## Tensions and Trade-offs

| 张力 | 训练侧 | 推理侧 | 平衡策略 |
|------|--------|--------|---------|
| GPU 使用 | 整卡独占、长时间 | 共享（MIG）、短请求 | 优先级抢占 + 时间窗口 |
| 存储 | 大吞吐（读数据集） | 低延迟（加载模型） | 分层存储 + 本地缓存 |
| 网络 | RDMA（NCCL all-reduce） | HTTP/gRPC（API 调用） | 网络隔离（训练用 RDMA 网络） |
| 容错 | Checkpoint 恢复 | 多副本 + 健康检查 | 训练用 Volcano，推理用 KServe |
| 成本 | 按任务计费（批处理） | 按时间计费（在线服务） | OpenCost 分账 |
| 更新频率 | 周/月级（重训练） | 分钟级（Canary 部署） | 模型仓库解耦 |

## Practical Patterns

```yaml
# 🟢 低风险：全链路状态检查
# 1. 训练任务状态
kubectl get pytorchjobs -A -o custom-columns=\
NS:.metadata.namespace,NAME:.metadata.name,STATUS:.status.conditions[-1].type

# 2. 推理服务状态
kubectl get inferenceservices -A -o custom-columns=\
NS:.metadata.namespace,NAME:.metadata.name,READY:.status.conditions[?(@.type=="Ready")].status,URL:.status.url

# 3. GPU 在训练/推理间的分配
kubectl get pods -A -o json | jq -r '.items[] |
  select(.spec.containers[].resources.limits["nvidia.com/gpu"] != null) |
  [.metadata.namespace, .metadata.labels["workload-type"] // "unknown",
   .spec.containers[0].resources.limits["nvidia.com/gpu"]] | @tsv' | \
  awk '{gpu[$2]+=$3} END {for(k in gpu) print k, gpu[k]}'

# 4. 存储使用
kubectl get pvc -A -o custom-columns=\
NS:.metadata.namespace,NAME:.metadata.name,SIZE:.spec.resources.requests.storage,STATUS:.status.phase | \
  grep -E "dataset|checkpoint|model"

# 5. 流水线运行状态
kubectl get workflows -n ai-platform --sort-by=.metadata.creationTimestamp | tail -5
```

## Related

- [[平台工程/构建/25-ai-platform-engineering|AI 平台工程]]
- [[综合/gpu-scheduling-cost|GPU Scheduling × Cost Optimization]]
- [[综合/gpu-operator-device-plugin-ecosystem|GPU Operator × Device Plugin × CDI]]
- [[平台工程/治理/18-gpu-cluster-governance-ai-platform|GPU 集群治理]]
- [[综合/multitenancy-resource-isolation-governance|多租户 × 资源隔离 × 治理]]
- [[AI基础设施/K8s-AI基础设施|K8s AI 基础设施]]
