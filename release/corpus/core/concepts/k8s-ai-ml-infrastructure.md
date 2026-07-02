---
title: K8S AI/ML 基础设施
summary: K8S AI/ML 基础设施：Kubernetes 已成为 AI/ML 工作负载的标准编排平台。从 GPU 调度到 LLM 推理、从分布式训练到端到端
  ML 平台，K8S 生态正在快速演进以满足大模型时代的需求。
category: concepts
tags:
- ai
- ml
- gpu
- dra
- llm
- kubeflow
- ray
- k8s
tier: core
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---



# K8S AI/ML 基础设施

Kubernetes 已成为 AI/ML 工作负载的标准编排平台。从 GPU 调度到 LLM 推理、从分布式训练到端到端 ML 平台，K8S 生态正在快速演进以满足大模型时代的需求。

## GPU 调度演进

GPU 资源管理是 AI 基础设施的核心挑战。K8S 提供了多层抽象来解决 GPU 共享与隔离问题。

### Device Plugin → DRA (Dynamic Resource Allocation)

| 特性 | Device Plugin (传统) | DRA (v1.34 GA) |
|------|---------------------|----------------|
| 资源发现 | 静态注册 | 动态声明式 |
| 分配粒度 | 整卡 | 灵活切片 |
| 拓扑感知 | 不支持 | 原生支持 |
| 多供应商 | 各自实现 | 统一 `ResourceClaim` API |

**DRA 核心 API 对象：**
- `ResourceSlice` — 节点上可用资源的描述
- `ResourceClaim` — Pod 对资源的声明
- `DeviceClass` — 预定义的设备配置模板
- `ResourceClaimTemplate` — 为每个 Pod 创建独立 Claim

```yaml
# DRA ResourceClaim 示例
apiVersion: resource.k8s.io/v1
kind: ResourceClaim
metadata:
  name: gpu-claim
spec:
  devices:
    requests:
    - name: gpu-request
      deviceClassName: nvidia-gpu
      count: 1
```

### GPU 切片技术

| 方案 | 原理 | 适用场景 | 隔离级别 |
|------|------|----------|----------|
| **MIG (Multi-Instance GPU)** | A100/H100 硬件级分区 | 生产推理、强隔离 | 硬件隔离 |
| **Time-Slicing** | GPU 时间片轮转 | 开发测试、尽力而为 | 软隔离 |
| **MPS (Multi-Process Service)** | CUDA 流共享 | 同构推理、低延迟 | 进程级 |
| **vGPU (NVIDIA GRID)** | 虚拟化层 | 企业桌面/VDI | 虚拟化 |

### In-Place Pod Resize (v1.33 GA)

无需重启 Pod 即可动态调整 CPU/Memory 资源，对 AI 推理场景尤为重要：

```yaml
# In-Place Resize 示例
containers:
- name: inference
  resources:
    requests:
      cpu: "2"
      memory: "4Gi"
    limits:
      cpu: "4"
      memory: "8Gi"
  resizePolicy:
  - resourceName: cpu
    policy: NotRequired   # CPU 可热调整
  - resourceName: memory
    policy: RestartNotRequired
```

## LLM 推理引擎

LLM 推理需要高吞吐、低延迟、大显存管理。主流引擎各有侧重：

### 引擎对比

| 引擎 | 核心技术 | 优势 | 适用场景 |
|------|----------|------|----------|
| **vLLM** | PagedAttention、连续批处理 | 吞吐最高、生态最广 | 通用 LLM 服务化 |
| **SGLang** | 结构化输出、RadixAttention | JSON Schema 约束生成、多轮对话 | Agent、结构化输出 |
| **TensorRT-LLM** | NVIDIA 原生优化 | 延迟最低、FP8/INT4 量化 | NVIDIA 硬件极致优化 |
| **LMDeploy** | Turbomind 引擎 | 国产模型支持好 | 中文模型服务化 |

### KServe — 推理服务标准

[[KServe]] 提供基于 CRD 的推理服务编排：

```yaml
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama-70b
spec:
  predictor:
    model:
      modelFormat:
        name: vllm
      storageUri: "s3://models/llama-70b"
      resources:
        limits:
          nvidia.com/gpu: "2"
        requests:
          memory: "64Gi"
    # 自动扩缩
    minReplicas: 1
    maxReplicas: 8
  # 金丝雀发布
  canaryTrafficPercent: 20
```

KServe 核心能力：
- **模型格式自动检测** — vLLM、TorchServe、Triton、ONNX 等
- **流量分割** — A/B 测试、金丝雀发布
- **自动扩缩** — 基于 QPS/并发/GPU 利用率
- **Transformer 支持** — 预处理/后处理 Pipeline

## 分布式训练

大模型训练需要跨节点并行，K8S 提供了调度和编排能力：

### 训练框架对比

| 框架 | 并行策略 | 核心特性 | 适用规模 |
|------|----------|----------|----------|
| **PyTorch FSDP** | ZeRO-3 全分片 | PyTorch 原生、易用 | 7B-70B |
| **DeepSpeed ZeRO** | ZeRO-1/2/3 + Infinity | NVMe Offload、MoE 支持 | 70B-405B |
| **Megatron-LM** | TP+PP+SP+EP | NVIDIA 官方、极致性能 | 100B+ |
| **ColossalAI** | 自动并行 | 低门槛、自动策略搜索 | 7B-70B |

### K8S 训练编排

```yaml
# PyTorchJob 示例（Kubeflow Training Operator）
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: llm-finetune
spec:
  nprocPerNode: "8"   # 每节点 8 GPU
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      template:
        spec:
          containers:
          - name: pytorch
            image: training:latest
            resources:
              limits:
                nvidia.com/gpu: "8"
    Worker:
      replicas: 3       # 3 个工作节点
      template:
        spec:
          containers:
          - name: pytorch
            image: training:latest
            resources:
              limits:
                nvidia.com/gpu: "8"
```

关键优化：
- **RDMA/InfiniBand** — RoCE v2 或 IB 网络，NCCL 通信
- **拓扑感知调度** — 节点内 NVLink、节点间高速互联
- **检查点** — 定期保存到 S3/PVC，支持故障恢复

## ML 平台

### Kubeflow v1.10

[[Kubeflow]] 是 K8S 上最完整的 ML 平台：

| 组件 | 功能 | 状态 |
|------|------|------|
| **Kubeflow Pipelines** | ML 工作流编排 | 核心 |
| **KServe** | 模型服务化 | 核心 |
| **Training Operator** | 分布式训练 Job | 核心 |
| **Katib** | 超参调优 | 核心 |
| **Notebooks** | Jupyter 环境 | 核心 |
| **Model Registry** | 模型版本管理 | v1.9+ 新增 |
| **Spark Operator** | 大数据处理 | v1.10 集成 |

### KubeRay

KubeRay 已成为 K8S 上运行 Ray 的标准方式：

- **RayCluster** — 自动扩缩 Ray 集群
- **RayJob** — 一次性 Ray 任务
- **RayService** — 长期运行的 Ray Serve 服务
- **GCS Fault Tolerance** — 全局控制存储高可用

```yaml
apiVersion: ray.io/v1
kind: RayCluster
metadata:
  name: ml-cluster
spec:
  headGroupSpec:
    rayStartParams:
      dashboard-host: "0.0.0.0"
    template:
      spec:
        containers:
        - name: ray-head
          image: rayproject/ray:2.40.0
  workerGroupSpecs:
  - groupName: gpu-workers
    replicas: 4
    minReplicas: 1
    maxReplicas: 10
    rayStartParams:
      resources: '"{\"nvidia.com/gpu\": 1}"'
    template:
      spec:
        containers:
        - name: ray-worker
          resources:
            limits:
              nvidia.com/gpu: "1"
```

### MLflow 2.x GenAI

MLflow 2.x 引入 GenAI 支持：

- **AI Gateway** — LLM API 统一代理（OpenAI 兼容接口）
- **Prompt Engineering UI** — 可视化 Prompt 调试
- **Tracing** — LLM 调用链路追踪
- **Evaluate** — LLM 自动评估框架
- **Model Signature** — 强类型输入输出定义

## K8S AI 新倡议

### AI Gateway WG (Working Group)

K8S 社区成立 AI Gateway 工作组，推动：
- LLM 流量路由标准化
- Token 级限流与配额
- 多模型负载均衡
- 与 Envoy Gateway / Gateway API 集成

### Agent Sandbox

为 AI Agent 提供安全隔离执行环境：
- 基于 gVisor/Kata Containers 的沙箱
- 临时文件系统与网络隔离
- 工具调用的权限控制
- 与 [[Istio]] Service Mesh 集成

### PSI Metrics GA (Pressure Stall Information)

PSI 指标在 v1.34 GA，为 AI 调度提供更精细的资源压力信号：

| PSI 指标 | 含义 | AI 场景应用 |
|----------|------|------------|
| `psi_cpu_some` | CPU 争用程度 | 训练任务调度 |
| `psi_memory_some` | 内存压力 | 推理 OOM 预警 |
| `psi_io_some` | IO 阻塞 | 数据加载瓶颈 |
| `psi_gpu_some` | GPU 压力 (扩展) | GPU 过载检测 |

```yaml
# 基于 PSI 的自动扩缩
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
spec:
  metrics:
  - type: Resource
    resource:
      name: kubernetes.io/psi-gpu-some-avg10
      target:
        type: AverageValue
        averageValue: "50"  # GPU 压力 >50% 时扩容
```

## 相关概念

- [[KServe]] — 推理服务平台
- [[Kubeflow]] — ML 平台
- [[KubeRay]] — Ray on K8S
- vLLM — LLM 推理引擎
- MLflow — ML 实验管理
- K8S 调度器 — 调度系统

## Related

- [[concepts/finops-greenops-practices.md|finops greenops practices]] — FinOps 与绿色运维实践
- [[concepts/container-runtime-evolution.md|container runtime evolution]] — 容器运行时演进
- [[concepts/k8s-networking-evolution.md|k8s networking evolution]] — K8S 网络技术演进
