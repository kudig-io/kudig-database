---
title: GPU Scheduling × Cost Optimization
summary: GPU 调度与成本优化的交叉：MIG/MPS/时间分片与抢占式调度如何降低昂贵的加速器闲置成本。
category: synthesis
tags:
- gpu
- scheduling
- cost-optimization
- mig
- mps
- finops
tier: supporting
sources:
- 概念/gpu-scheduling-ai-workloads.md
- 概念/gang-scheduling.md
- 概念/dynamic-resource-allocation.md
- 概念/finops-resource-governance.md
- 概念/capacity-planning-cost-optimization.md
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
provenance:
  extracted: 0.3
  inferred: 0.6
  ambiguous: 0.1
base_confidence: 0.73
lifecycle: draft
lifecycle_changed: '2026-07-11'
---

# GPU Scheduling × Cost Optimization

## The Connection

GPU 是云账单中最昂贵的资源单位，一块 H100 的时薪可达数美元。GPU 调度决定"谁在何时占用哪块卡"，成本优化决定"这些卡是否被榨干"。在 Kubernetes 中，默认调度器把整块 GPU 当作独占扩展资源（`nvidia.com/gpu`）分配，导致大量推理工作负载仅用满 10-30% 算力却独占整卡——这是 AI 基础设施成本流失的最大源头。问题根源在于 Kubernetes 的扩展资源模型是整数级的：`nvidia.com/gpu: 1` 意味着调度器以"独占"语义分配 GPU，不支持分数化分配。把切分技术（MIG/MPS/时间分片）纳入调度链路，需要打破这层整数约束——通过设备插件（NVIDIA GPU Operator）、虚拟化层（HAMi）或原生 API（DRA, Dynamic Resource Allocation）将一块物理 GPU 暴露为多个可调度的逻辑设备，再由调度器按逻辑设备分配。这是 GPU FinOps 的核心抓手：将调度粒度从"物理卡"细化到"GPU 计算实例"，从而把闲置算力转化为可共享的调度单元。^[inferred]

## Where They Co-occur

- **MIG（Multi-Instance GPU）**：硬件级切分 Hopper/Ampere GPU 为多个隔离实例（如 1g.10gb、2g.20gb），适合多租户低延迟推理，单位成本下降但实例数和几何切分受硬件限制。
- **MPS（Multi-Process Service）**：软共享，多进程共用 CUDA 上下文，吞吐高但缺乏故障与安全隔离——单进程 CUDA 异常可能影响全卡共享者。
- **时间分片（Time-slicing）**：通过 `nvidia.com/gpu.shared` 或 HAMi 把 GPU 按时间片分给多 Pod，最灵活但存在尾延迟抖动；适合 batch 推理，不适合实时推理。
- **Gang Scheduling（Volcano/Koordinator）**：训练任务需 all-reduce，必须整组 Pod 同时调度，否则死锁；成本上避免"半调度"导致 GPU 空占——被调度但无法启动的 Pod 占据 GPU 配额却不消耗算力。
- **动态资源分配（DRA）**：K8s 1.26+ 的新 API，让设备厂商自定义 GPU 切分与分配逻辑，是 MIG/MPS 走向一等公民的路径；DRA 的 `ResourceClaim` 支持结构化参数描述设备拓扑。
- **HAMi / KAITO**：HAMi 提供算力隔离与显存限制（vGPU），KAITO 把 GPU 节点供给与推理部署自动化（含自动选择 GPU 机型），间接降低运维与闲置成本。
- **OpenCost GPU 归因**：将 GPU 节点费用按切分实例摊销到各 Pod，使"切分省了多少钱"可度量；GPU 成本通常占 AI 集群总账单的 70%+。
- **GPU 空闲检测与自动缩容**：监控 GPU 利用率（DCGM exporter），当推理队列清空时自动缩减 Pod 或归还节点，避免 GPU 节点空转计费。
- **GPU 优先级与抢占**：通过 PriorityClass + Volcano 抢占机制，高优先级推理任务可抢占低优先级 batch 任务的 GPU 实例，保证 SLA 敏感负载的资源供给。
- **DCGM 指标驱动调度**：NVIDIA DCGM Exporter 暴露 GPU 利用率（`DCGM_FI_DEV_GPU_UTIL`）、显存使用（`DCGM_FI_DEV_FB_USED`）、温度和功耗指标，可作为 KEDA 或自定义调度器的输入信号。
- **GPU 节点池隔离**：GPU 节点通常使用 tainted/label 隔离（`nvidia.com/gpu.present=true`），避免非 GPU Pod 调度到昂贵节点上浪费资源。
- **NVIDIA GPU Operator**：自动管理 GPU 驱动安装、containerd runtime hook（`nvidia-container-runtime`）、DCGM exporter 部署、MIG 配置切片——是 GPU 共享调度的前置依赖。
- **推理框架显存感知调度**：TensorRT-LLM、vLLM 等 LLM 推理引擎支持 PagedAttention 显存管理，可动态分配/释放显存页——与 HAMi 的显存隔离互补，进一步提升单卡可服务的并发模型数。
- **GPU 成本可观测**：DCGM exporter 暴露 `DCGM_FI_DEV_GPU_UTIL`、`DCGM_FI_DEV_FB_USED`（显存）、`DCGM_FI_DEV_POWER_USAGE`（功耗）指标，OpenCost 按这些指标计算"GPU 利用率 × 时薪 = 有效成本"，识别低效使用的 GPU 实例。
- **Spot GPU 策略**：Spot/抢占式 GPU 实例成本可低至 On-demand 的 30-50%，但中断频率更高——训练任务不适合 Spot（中断导致 checkpoint 丢失），推理任务可通过 KEDA + 多副本 + checkpointing 容忍 Spot 中断。
- **GPU 空闲回收**：GPU Pod 在无推理请求时应自动 scale 到零（KEDA + GPU Operator），节点层面的 GPU 实例应通过 Karpenter consolidation 在低峰期归还，避免 GPU 节点 24/7 空转。

## Cross-cutting Insight

GPU 成本优化的本质不是"少买卡"，而是"提高每块卡的并发复用率"。调度系统（切分策略 + 优先级 + 抢占）决定了复用率的上限，FinOps 归因决定了复用收益是否被看见。当推理工作负载能按优先级抢占、按切分实例共享时，单卡可服务的工作负载密度可提升 3-10 倍——这是 AI 平台经济性的分水岭。但从工程视角看，GPU 共享引入了与传统 CPU 调度截然不同的复杂性：GPU 的显存是硬约束（OOM 即崩溃，不像 CPU 内存可以靠 swap 缓冲），CUDA context 切换有不可忽略的延迟开销，而 NVIDIA 驱动对容器化的支持（GPU Operator、container runtime hook）本身也在快速迭代。因此 GPU FinOps 不是"配个监控看利用率"那么简单，而是需要在调度器、设备插件、驱动层和应用框架四个层面协同设计的系统工程。^[inferred]

## Tensions and Trade-offs

| 维度 | 切分共享侧重 | 独占整卡侧重 | 结合注意事项 |
|---|---|---|---|
| 成本 | 高并发复用，单位成本低 | 1:1 占用，单位成本高 | 推理适合切分，训练倾向独占 |
| 隔离 | MPS/时间分片隔离弱 | 硬隔离稳定 | 多租户需 MIG 或 HAMi |
| 性能 | 共享有尾延迟抖动 | 独占延迟可预测 | SLO 敏感场景慎用时间分片 |
| 调度复杂度 | 需 Gang/DRA/优先级编排 | 默认调度即可 | 复杂度推给平台团队 |
| 故障域 | 切分实例互相影响 | 故障独立 | MPS 单进程崩溃影响全卡 |
| 显存管理 | 共享显存需软隔离 | 独占显存无争用 | HAMi 显存隔离有约 5-10% 开销 |

## Open Questions

- MIG 几何切分是静态配置，如何在不停机的前提下按负载形态动态重切？是否需要节点 drain + 重新配置的自动化流程？
- 在共享 GPU 上，如何为每个推理 Pod 定义公平且可度量的 SLI（尾延迟而非平均吞吐）？DCGM 指标是否足够细粒度？
- DRA 普及后，Volcano/Koordinator 的 Gang 调度与 DRA 设备分配如何统一编排？`ResourceClaim` 的生命周期与 Pod 调度如何协调？
- 当推理工作负载从 CPU 迁移到 GPU 时，如何评估"GPU 切分 + 共享"的实际 ROI 而非被厂商 benchmark 误导？

## Related

- [[概念/gpu-scheduling-ai-workloads.md|GPU 调度与 AI 工作负载]]
- [[概念/gang-scheduling.md|Gang 调度]]
- [[概念/dynamic-resource-allocation.md|动态资源分配]]
- [[实体/hami.md|HAMi]]
- [[实体/volcano.md|Volcano]]
- [[实体/koordinator.md|Koordinator]]
- [[实体/kaito.md|KAITO]]
- [[实体/kserve.md|KServe]]
- [[概念/finops-resource-governance.md|FinOps 资源治理]]
- [[概念/capacity-planning-cost-optimization.md|容量规划与成本优化]]
- [[综合/autoscaling-cost-optimization.md|Autoscaling × Cost Optimization]]


<!-- risk-assessed -->
