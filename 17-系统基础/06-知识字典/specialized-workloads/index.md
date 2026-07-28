---
title: 专项工作负载知识词典
description: 涵盖 Kubernetes 上 AI/ML、GPU、HPC、Serverless、边缘计算、Windows 容器、WebAssembly 等专项工作负载的完整术语体系与技术参考
summary: 专项工作负载领域词典，覆盖 GPU 调度、模型推理、Kubeflow、Knative、KubeVirt、Wasm 等核心概念
category: dictionary
tags:
- dictionary
- specialized-workloads
- ai-ml
- gpu
- serverless
- edge-computing
- wasm
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
audience:
- AI 平台工程师
- SRE
- 架构师
- ML 工程师
---

# 专项工作负载知识词典（Specialized Workloads）

> 本词典覆盖 Kubernetes 上非标准工作负载的核心术语、技术组件及工程实践，包括 AI/ML 训练与推理、GPU 资源管理、HPC、Serverless、边缘计算、Windows 容器及 WebAssembly 等前沿领域。

## 领域概述

专项工作负载是 Kubernetes 生态从“通用容器编排”向“全场景计算平台”演进的关键领域：

- **AI/ML 工作负载**：GPU 调度、分布式训练、模型推理服务、MLOps 流水线
- **高性能计算 (HPC)**：科学计算、生物信息学、气象模拟等 MPI/并行任务
- **Serverless/FaaS**：事件驱动、自动缩容到零、函数计算
- **边缘计算**：资源受限环境、弱网/离线、边缘自治
- **虚拟化工作负载**：KubeVirt 在 K8s 上运行传统 VM
- **WebAssembly**：轻量级、安全沙箱、多语言运行时
- **Windows 容器**：混合 OS 集群、.NET 应用迁移

## 核心术语定义

### AI/ML 与 GPU 计算

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| GPU Partitioning | 将单块 GPU 分割为多个虚拟实例供多 Pod 共享 | MIG (A100/H100), vGPU, Time-slicing |
| MIG (Multi-Instance GPU) | NVIDIA A100/H100 硬件级 GPU 分区技术 | NVIDIA MIG |
| GPU Time-slicing | 时间片轮转方式共享 GPU，无硬件隔离 | K8s Device Plugin |
| KServe | CNCF 模型推理服务框架，支持自动扩缩、金丝雀、A/B | KServe |
| InferenceServer | KServe 中运行模型推理的服务实例 | KServe |
| Kubeflow | K8s 原生 ML 平台，含训练、流水线、Notebook | Kubeflow |
| Ray | 分布式计算框架，支持 RL/LLM 训练与推理 | Ray on K8s (KubeRay) |
| Kueue | K8s 原生作业队列管理，支持配额、优先级、抢占 | Kueue |
| LLM Inference | 大语言模型推理服务，关注吐吐量、延迟、显存 | vLLM, TGI, TensorRT-LLM |
| Model Registry | 模型版本管理与元数据存储 | MLflow, ModelPack |
| MLOps Pipeline | ML 模型从训练到部署的自动化流水线 | Kubeflow Pipelines, Argo |
| Vector Database | 向量数据库，支撑 RAG/语义检索 | Milvus, Weaviate, Qdrant |
| RAG | 检索增强生成，结合向量检索与 LLM 生成 | LangChain, LlamaIndex |

### Serverless 与 FaaS

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| Knative | K8s 原生 Serverless 框架，支持缩容到零、事件驱动 | Knative Serving/Eventing |
| Scale-to-Zero | 无流量时自动缩容到 0 实例，节省资源 | Knative, KEDA |
| OpenFaaS | 轻量级 FaaS 框架，函数即容器 | OpenFaaS |
| KEDA | 基于事件源的 K8s 自动扩缩容 | KEDA |
| Cold Start | Serverless 函数首次调用的初始化延迟 | 通用问题 |

### 边缘计算与轻量运行时

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| OpenYurt | 阿里云云边一体平台，支持节点池、边缘自治 | OpenYurt |
| Edge Autonomy | 边缘节点断网后独立运行的能力 | OpenYurt/KubeEdge |
| NodePool | 边缘节点按地域/功能分组管理 | OpenYurt |
| SpinKube | 在 K8s 上运行 WebAssembly 工作负载的 Operator | SpinKube |
| wasmCloud | 分布式 Wasm 应用运行时，支持多集群 | wasmCloud |

### 虚拟化与混合 OS

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| KubeVirt | 在 K8s Pod 中运行传统虚拟机的扩展 | KubeVirt |
| VirtualMachine (VM) | KubeVirt 中管理 VM 生命周期的 CRD | KubeVirt |
| Windows Container | 在 K8s 中运行 Windows 工作负载 | Windows Server Container |
| 混合 OS 集群 | 同一集群同时运行 Linux 和 Windows 节点 | K8s 原生支持 |

### HPC 与科学计算

| 术语 | 定义 | 典型实现 |
|------|------|----------|
| MPI Job | 基于 MPI 的分布式并行计算任务 | Kubeflow MPI Operator |
| Volcano | K8s 批量调度引擎，支持 Gang Scheduling | Volcano |
| Gang Scheduling | 一组 Pod 必须全部调度成功或全部不调度 | Volcano/Kueue |
| Bioinformatics | 生物信息学计算（基因组测序、蛋白质折叠） | Nextflow, Cromwell |

## 技术组件索引

### AI/ML 平台类

- [[17-系统基础/06-知识字典/specialized-workloads/ai-infra-specialist.md|AI Infra Specialist（AI 基础设施专家）]]
- [[17-系统基础/06-知识字典/specialized-workloads/gpu-resource-management-and-partitioning.md|GPU Resource Management（GPU 资源管理与分区）]]
- [[17-系统基础/06-知识字典/specialized-workloads/kserve.md|KServe（模型推理服务）]]
- [[17-系统基础/06-知识字典/specialized-workloads/kserve-model-serving.md|KServe Model Serving（模型服务部署）]]
- [[17-系统基础/06-知识字典/specialized-workloads/kubeflow.md|Kubeflow（ML 平台）]]
- [[17-系统基础/06-知识字典/specialized-workloads/ray.md|Ray（分布式计算）]]
- [[17-系统基础/06-知识字典/specialized-workloads/kueue-job-queue-management.md|Kueue（作业队列管理）]]
- [[17-系统基础/06-知识字典/specialized-workloads/llm-inference-optimization.md|LLM Inference Optimization（LLM 推理优化）]]
- [[17-系统基础/06-知识字典/specialized-workloads/mlops-pipelines-and-model-registry.md|MLOps Pipelines（ML 流水线）]]
- [[17-系统基础/06-知识字典/specialized-workloads/modelpack.md|ModelPack（模型打包）]]
- [[17-系统基础/06-知识字典/specialized-workloads/kitops.md|KitOps（AI 资产打包）]]
- [[17-系统基础/06-知识字典/specialized-workloads/vector-databases-and-rag-infrastructure.md|Vector DB & RAG（向量数据库与 RAG）]]
- [[17-系统基础/06-知识字典/specialized-workloads/seldon.md|Seldon（模型部署）]]

### Serverless 与 FaaS 类

- [[17-系统基础/06-知识字典/specialized-workloads/knative.md|Knative（Serverless 框架）]]
- [[17-系统基础/06-知识字典/specialized-workloads/openfaas.md|OpenFaaS（函数计算）]]

### 边缘计算与 Wasm 类

- [[17-系统基础/06-知识字典/specialized-workloads/openyurt.md|OpenYurt（云边一体）]]
- [[17-系统基础/06-知识字典/specialized-workloads/spin.md|Spin（Wasm 运行时）]]
- [[17-系统基础/06-知识字典/specialized-workloads/spinkube.md|SpinKube（Wasm on K8s）]]
- [[17-系统基础/06-知识字典/specialized-workloads/wasmcloud.md|wasmCloud（分布式 Wasm）]]

### 虚拟化与混合 OS 类

- [[17-系统基础/06-知识字典/specialized-workloads/kubevirt.md|KubeVirt（VM on K8s）]]
- [[17-系统基础/06-知识字典/specialized-workloads/windows-containers-in-kubernetes.md|Windows Containers]]
- [[17-系统基础/06-知识字典/specialized-workloads/guide-for-running-windows-containers-in-kubernetes.md|Windows 容器实践指南]]

### HPC 与科学计算类

- [[17-系统基础/06-知识字典/specialized-workloads/hpc-and-bioinformatics.md|HPC & Bioinformatics]]

## 架构模式对比

| 场景 | 推荐方案 | 关键考量 |
|------|----------|----------|
| LLM 推理服务 | KServe + vLLM | GPU 显存、吐吐量、自动扩缩 |
| 分布式训练 | Kubeflow + Ray + Volcano | Gang Scheduling、网络带宽 |
| 事件驱动函数 | Knative Serving | 冷启动、缩容到零 |
| 边缘 IoT | OpenYurt + K3s | 弱网、资源受限 |
| 传统 VM 迁移 | KubeVirt | 性能开销、存储兼容 |
| 多语言插件 | SpinKube/wasmCloud | 安全沙箱、启动速度 |
| Windows .NET | Windows Container | 镜像大小、节点池隔离 |

## GPU 资源管理深度解析

### GPU 共享方案对比

| 方案 | 隔离级别 | 性能开销 | 适用场景 |
|------|----------|----------|----------|
| NVIDIA MIG | 硬件级 | 无 | A100/H100，多租户生产 |
| vGPU (GRID) | 虚拟化 | 低 | VDI、多用户共享 |
| Time-slicing | 时间片 | 中 | 开发/测试、小模型推理 |
| GPU 池化 (HAMi) | 软件定义 | 中 | 异构 GPU 统一管理 |

### GPU 调度最佳实践

```yaml
# MIG 分区配置示例
apiVersion: v1
kind: Pod
spec:
  containers:
  - name: inference
    image: nvcr.io/nvidia/tritonserver:24.01-py3
    resources:
      limits:
        nvidia.com/mig-1g.10gb: 1  # 使用 1/7 GPU，10GB 显存
---
# Kueue 队列配置：GPU 配额管理
apiVersion: kueue.x-k8s.io/v1beta1
kind: ClusterQueue
metadata:
  name: gpu-training-queue
spec:
  resourceGroups:
  - coveredResources: ["nvidia.com/gpu"]
    flavors:
    - name: a100-80gb
      resources:
      - name: nvidia.com/gpu
        nominalQuota: 32
  - coveredResources: ["cpu", "memory"]
    flavors:
    - name: default
      resources:
      - name: cpu
        nominalQuota: 256
      - name: memory
        nominalQuota: 1024Gi
```

## 生产最佳实践

### AI/ML 工作负载

1. **GPU 资源规划**：训练用 A100/H100，推理用 L4/T4，避免混用
2. **存储分离**：训练数据用高性能并行文件系统（Lustre/GPFS），模型用对象存储
3. **网络要求**：分布式训练需 RDMA/RoCE，延迟 <5μs
4. **检查点策略**：定期保存训练 checkpoint 到持久化存储
5. **推理服务**：使用 KServe 的 Canary 滚动更新，避免模型版本回退

### Serverless 工作负载

1. **冷启动优化**：使用 Init Container 预热、保持最小实例数 > 0
2. **事件源解耦**：通过 Knative Eventing 解耦生产者/消费者
3. **资源限制**：函数内存上限设置合理，避免 OOM
4. **可观测性**：每次调用记录 trace，监控冷启动率

### 边缘计算

1. **资源精简**：边缘节点只部署必要组件，移除不需要的 Addon
2. **离线能力**：镜像预拉取 + 本地 Registry 缓存
3. **更新策略**：边缘节点滚动更新，每批不超过 10%
4. **监控降级**：边缘监控数据本地缓存，网络恢复后批量上报

## 故障排查要点

| 故障现象 | 可能原因 | 排查方向 |
|----------|----------|----------|
| GPU Pod Pending | GPU 资源不足/Device Plugin 异常 | `kubectl describe pod`、检查 nvidia-device-plugin |
| MIG 分配失败 | MIG 未启用/分区配置错误 | `nvidia-smi mig -lgi`、检查 GPU 模式 |
| KServe 推理超时 | 模型加载慢/GPU 显存不足 | 检查 InferenceService 日志、GPU 显存使用 |
| Knative 缩容失败 | 流量持续/缩容延迟配置 | 检查 KPA 配置、活跃请求数 |
| KubeVirt VM 启动失败 | 虚拟化未开启/资源不足 | 检查节点 KVM 支持、`virtctl` 日志 |
| 边缘节点失联 | 网络中断/EdgeCore 崩溃 | 检查边缘自治状态、手动重启 EdgeCore |

## 学习路径

```
基础: K8s 工作负载基础 → GPU 资源管理
进阶: KServe 推理部署 → Kubeflow 训练流水线
高级: 分布式训练 (Ray/Volcano) → LLM 推理优化
前沿: Wasm 工作负载 → 天基计算 → AI 驱动调度
```

## 参考链接

- https://kserve.github.io/website/
- https://www.kubeflow.org/
- https://knative.dev/
- https://kubevirt.io/
- https://openyurt.io/
- https://www.spinkube.dev/
- https://wasmcloud.com/
- https://kueue.sigs.k8s.io/
- https://docs.ray.io/

## Related

- [[17-系统基础/06-知识字典/scheduling/volcano.md|Volcano 批量调度]]
- [[17-系统基础/06-知识字典/multi-cloud/edge-computing-and-k3s.md|边缘计算与 K3s]]
- [[02-工作负载/01-核心工作负载/05-job-cronjob-advanced|Job/CronJob 工作负载]]
- [[17-系统基础/06-知识字典/configuration/resource-management-for-pods-and-containers.md|资源管理]]

## 深度技术解析

### LLM 推理服务架构

大语言模型推理是当前最热门的专项工作负载场景：

```
LLM 推理服务架构:

Client Request
    │
    ▼
KServe InferenceService (路由层)
    │
    ├── Canary (10% 流量) → vLLM Pod (model-v2)
    │
    └── Stable (90% 流量) → vLLM Pod (model-v1)
                              │
                              ├── GPU: A100-80GB x 2 (Tensor Parallel)
                              ├── KV Cache: PagedAttention
                              ├── Batching: Continuous Batching
                              └── Quantization: AWQ/GPTQ 4bit
```

**关键优化技术：**

| 技术 | 原理 | 效果 |
|------|------|------|
| PagedAttention | 将 KV Cache 分页管理，避免显存碎片 | 显存利用率提升 2-4x |
| Continuous Batching | 动态合并请求，无需等待整批完成 | 吐吐量提升 3-5x |
| Tensor Parallelism | 模型层切分到多 GPU 并行计算 | 支持超大模型 |
| AWQ/GPTQ 量化 | 权重 4bit 量化，减少显存占用 | 显存减少 50-75% |
| Speculative Decoding | 小模型草稿 + 大模型验证 | 延迟降低 2-3x |
| Prefix Caching | 缓存公共前缀的 KV Cache | 多轮对话加速 |

### Knative Serving 架构

```
Knative Serving 组件:

┌─────────────────────────────────────────────┐
│  Knative Serving Control Plane              │
│  ├── Controller (Service/Route/Config)      │
│  ├── Webhook (准入控制 + 默认值注入)      │
│  └── Activator (缩容到零时的流量缓冲)    │
└─────────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────────┐
│  Data Plane                                  │
│  ├── Queue-Proxy (每 Pod 边车)              │
│  │   ├── 请求计数 (并发控制)              │
│  │   ├── 健康检查代理                    │
│  │   └── 指标上报                        │
│  └── User Container (业务容器)             │
└─────────────────────────────────────────────┘

缩容到零流程:
1. Queue-Proxy 报告 0 活跃请求
2. Autoscaler (KPA) 等待 stable-window (60s)
3. 缩容到 0 Pod
4. 新请求到达 → Activator 缓冲
5. Autoscaler 扩容 → 1 Pod
6. Pod Ready → Activator 转发流量
```

### KubeVirt 虚拟化架构

```
KubeVirt 架构:

┌─────────────────────────────────────────┐
│  K8s Control Plane                       │
│  └── KubeVirt Components                │
│      ├── virt-controller (VM 生命周期)  │
│      ├── virt-api (API 服务)            │
│      └── virt-handler (每节点 DaemonSet) │
└─────────────────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────────┐
│  Worker Node                             │
│  ├── virt-handler (DaemonSet)           │
│  │   └── libvirtd (虚拟化管理)        │
│  └── Pod (VM 实例)                      │
│      ├── compute container (QEMU/KVM)  │
│      └── volume containers (磁盘/CD)   │
└─────────────────────────────────────────┘
```

**KubeVirt vs 传统虚拟化：**

| 维度 | KubeVirt | 传统虚拟化 (vSphere) |
|------|----------|---------------------|
| 管理接口 | kubectl/virtctl | vCenter/PowerCLI |
| 调度 | K8s Scheduler | DRS |
| 网络 | CNI (Multus) | vSwitch/NSX |
| 存储 | CSI/PV | VMFS/vSAN |
| 运维模式 | GitOps/声明式 | 手动/脚本 |
| 适用场景 | VM 迁移过渡、混合工作负载 | 纯虚拟化环境 |

## 生产案例研究

### 案例：大型 LLM 推理平台

**背景：** 某互联网公司需要部署 70B 参数 LLM 推理服务，要求：
- P99 延迟 < 2s（首 token）
- 吐吐量 > 1000 tokens/s/用户
- 支持自动扩缩容（0-100 GPU）

**架构方案：**
- KServe + vLLM 推理引擎
- A100-80GB x 4 (Tensor Parallel=4)
- KEDA 基于请求队列深度扩缩容
- 模型存储：S3 + Init Container 拉取
- 监控：Prometheus + Grafana（GPU 利用率、队列深度、延迟分布）

**关键决策：**
- 选择 vLLM 而非 TGI：Continuous Batching 性能更优
- 4 GPU TP 而非 8 GPU：性价比最优，单请求延迟可接受
- 缩容到 2 而非 0：避免冷启动（70B 模型加载需 3-5min）

## 常用运维命令速查

```bash
# === GPU 管理 ===
# 查看节点 GPU 状态
kubectl get nodes -o json | jq '.items[].status.allocatable["nvidia.com/gpu"]'
# 查看 GPU Pod 分布
kubectl get pods -A -o json | jq '.items[] | select(.spec.containers[].resources.limits["nvidia.com/gpu"] != null)'
# 节点 GPU 详情
nvidia-smi -q -d UTILIZATION,MEMORY

# === KServe ===
# 查看推理服务状态
kubectl get inferenceservices -A
# 查看模型服务日志
kubectl logs -l serving.kserve.io/inferenceservice=my-model -c kserve-container
# 测试推理
kubectl run curl --image=curlimages/curl --rm -it -- curl -X POST http://my-model.default.svc/v1/completions

# === Knative ===
# 查看 Knative Service
kubectl get ksvc -A
# 查看 Revision
kubectl get revisions -A
# 查看 Autoscaler 状态
kubectl get podautoscalers -A

# === KubeVirt ===
# 查看 VM 状态
virtctl get vm -A
# 启动/停止 VM
virtctl start my-vm
virtctl stop my-vm
# 控制台访问
virtctl console my-vm

# === Kueue ===
# 查看队列状态
kubectl get clusterqueues
kubectl get localqueues -A
# 查看待处理作业
kubectl get workloads -A --field-selector status.admitted=false
```

## 缩略语表

| 缩写 | 全称 | 说明 |
|------|------|------|
| MIG | Multi-Instance GPU | NVIDIA 多实例 GPU |
| TP | Tensor Parallelism | 张量并行 |
| PP | Pipeline Parallelism | 流水线并行 |
| KV Cache | Key-Value Cache | 注意力机制缓存 |
| KPA | Knative Pod Autoscaler | Knative Pod 自动扩缩 |
| FaaS | Function as a Service | 函数即服务 |
| HPC | High Performance Computing | 高性能计算 |
| MPI | Message Passing Interface | 消息传递接口 |
| RDMA | Remote Direct Memory Access | 远程直接内存访问 |
| MLOps | Machine Learning Operations | ML 运维 |
| RAG | Retrieval Augmented Generation | 检索增强生成 |
| Wasm | WebAssembly | Web 汇编字节码 |

## 版本兼容性矩阵

| 组件 | K8s 1.28 | K8s 1.29 | K8s 1.30 | K8s 1.31 |
|------|-----------|-----------|-----------|----------|
| KServe | v0.12+ | v0.13+ | v0.14+ | v0.15+ |
| Kubeflow | v1.8+ | v1.9+ | v1.10+ | v1.11+ |
| Knative | v1.12+ | v1.13+ | v1.14+ | v1.15+ |
| KubeVirt | v1.1+ | v1.2+ | v1.3+ | v1.4+ |
| Kueue | v0.6+ | v0.7+ | v0.8+ | v0.9+ |
| KubeRay | v1.0+ | v1.1+ | v1.2+ | v1.3+ |
| OpenYurt | v1.3+ | v1.4+ | v1.5+ | v1.6+ |
| SpinKube | v0.1+ | v0.2+ | v0.3+ | v0.4+ |

## 常见问题 FAQ

**Q1: GPU Time-slicing 和 MIG 怎么选？**

A: 生产环境多租户场景优先 MIG（硬件隔离，无性能干扰）。开发/测试环境或小模型推理可用 Time-slicing（配置简单，无硬件要求）。注意：Time-slicing 下 Pod 间可能互相影响性能，不适合 SLA 严格的场景。

**Q2: Knative 缩容到零的冷启动如何优化？**

A: 三种策略：
1. 设置 `min-scale: 1` 保持最小实例（牺牲资源换延迟）
2. 使用 Init Container 预热依赖（减少启动时间）
3. 优化镜像大小（多阶段构建、精简基础镜像）

**Q3: KubeVirt 性能开销大吗？**

A: KubeVirt 基于 KVM 硬件虚拟化，CPU/内存性能开销 <2%。主要开销在：
- 网络：通过 CNI 而非 SR-IOV 时约 10-15% 开销（可用 Multus + SR-IOV 优化）
- 存储：通过 CSI 而非直通时约 5-10% 开销（可用 hostPath 或直通优化）

**Q4: 分布式训练为什么需要 Gang Scheduling？**

A: 分布式训练（如 PyTorch DDP）需要所有 Worker 同时就绪才能开始。如果部分 Pod 调度成功、部分 Pending，已调度的 Pod 会空等浪费 GPU。Gang Scheduling 确保“全有或全无”，避免资源死锁。Volcano 和 Kueue 都支持。

**Q5: WebAssembly 工作负载适合什么场景？**

A: Wasm 适合：
- 多语言插件系统（安全沙箱执行用户代码）
- 边缘计算（启动速度 <1ms，资源占用极小）
- Serverless 函数（冷启动几乎为零）
不适合：GPU 计算、大内存应用、需要完整 Linux API 的应用

## 技术选型决策树

```
你的工作负载是什么类型？
│
├─ AI/ML 推理服务
│   ├─ 单模型、低并发 → KServe + vLLM (1-2 GPU)
│   └─ 多模型、高并发 → KServe + Triton + GPU 池化
│
├─ AI/ML 训练
│   ├─ 单机多卡 → PyTorch DDP + GPU Node
│   └─ 多机多卡 → Kubeflow Training + Volcano + RDMA
│
├─ 事件驱动/函数计算
│   ├─ HTTP 触发 → Knative Serving
│   └─ 事件源触发 → Knative Eventing / KEDA
│
├─ 传统 VM 迁移
│   └─ KubeVirt (保留 VM 运维习惯，统一 K8s 管理)
│
├─ 边缘/IoT
│   ├─ 有稳定网络 → K3s + 中心管理
│   └─ 弱网/离线 → OpenYurt / KubeEdge (边缘自治)
│
└─ 插件/扩展系统
    └─ SpinKube / wasmCloud (Wasm 安全沙箱)
```

