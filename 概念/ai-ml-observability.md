---
title: AI/ML 工作负载的可观测性
description: '## GPU 监控'
summary: '## GPU 监控'
category: synthesis
tags:
- ai-ml
- observability
- gpu-monitoring
- mLOps
- metrics
- gpu
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- AI/ML 工作负载的可观测性 是什么
- 如何 AI/ML 工作负载的可观测性
trigger_keywords:
- AI
- ML
- 工作负载的可观测性
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
relationships:
- target: '[[最佳实践/best-practices/observability/monitoring.md]]'
  type: related_to
- target: '[[系统基础/知识字典/observability/observability.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# AI/ML 工作负载的可观测性

## 概述

AI/ML 工作负载的可观测性是传统云原生监控的扩展，增加了 GPU 硬件监控、训练实验追踪、推理服务质量监控和模型漂移检测等维度。与传统微服务不同，ML 工作负载需要同时关注基础设施层（GPU/网络/存储）、框架层（PyTorch/TensorFlow）和模型层（准确率/延迟/漂移），形成全栈可观测性。

## GPU 监控

### 关键指标

GPU 监控的核心是 NVIDIA DCGM Exporter，它通过 NVML（NVIDIA Management Library）采集 GPU 硬件指标：

```
关键指标:
├── DCGM_FI_DEV_GPU_UTIL          GPU 计算利用率（0-100%）
├── DCGM_FI_DEV_MEM_COPY_UTIL     GPU 显存带宽利用率
├── DCGM_FI_DEV_FB_USED           GPU 显存使用量（MB）
├── DCGM_FI_DEV_GPU_TEMP          GPU 温度（°C）
├── DCGM_FI_DEV_POWER_USAGE       GPU 功耗（W）
├── DCGM_FI_PROF_PIPE_TENSOR_ACTIVE  Tensor Core 利用率
├── DCGM_FI_DEV_PCIE_TX_THROUGHPUT  PCIe 发送带宽
├── DCGM_FI_DEV_PCIE_RX_THROUGHPUT  PCIe 接收带宽
└── DCGM_FI_DEV_XID_ERRORS        GPU XID 错误事件
```

### GPU 告警规则

```yaml
# PrometheusRule: GPU 健康监控
groups:
  - name: gpu-alerts
    rules:
      # GPU 温度过高
      - alert: GPUTemperatureHigh
        expr: DCGM_FI_DEV_GPU_TEMP > 85
        for: 5m
        labels:
          severity: warning

      # GPU XID 错误（硬件故障）
      - alert: GPUXIDError
        expr: rate(DCGM_FI_DEV_XID_ERRORS[5m]) > 0
        labels:
          severity: critical

      # GPU 利用率过低（可能浪费）
      - alert: LowGPUUtilization
        expr: avg_over_time(DCGM_FI_DEV_GPU_UTIL[2h]) < 30
        labels:
          severity: warning
```

### 多 GPU 拓扑监控

对于分布式训练，GPU 间的 NVLink/NVSwitch 拓扑对性能影响巨大：

```bash
# 🟢 低风险：只读检查
# 查看 GPU 拓扑
nvidia-smi topo -m
# 输出示例:
#       GPU0  GPU1  GPU2  GPU3
# GPU0   X   NV12  NV12  NV12
# GPU1  NV12   X   NV12  NV12
```

监控 NVLink 带宽利用率可识别因拓扑不佳导致的通信瓶颈。

## 训练任务追踪

### MLflow / W&B + Kubernetes 集成

```
实验追踪数据流:
  PyTorch Job (Pod)
    ├── MLflow Tracker → 记录超参数、指标、模型版本
    ├── Pod 标签关联 → experiment_id, run_id
    └── Prometheus → 资源使用（GPU/CPU/Memory）

关联价值:
  → 将资源消耗与实验结果关联
  → 识别资源浪费的训练任务（GPU 利用率 < 20%）
  → 自动发现异常训练（loss 未收敛但持续消耗 GPU）
```

### 训练任务监控标签

```yaml
# 训练 Pod 的标准标签
metadata:
  labels:
    ml/experiment-id: "exp-2026-001"
    ml/run-id: "run-42"
    ml/framework: "pytorch"
    ml/model: "llama-7b-finetune"
    ml/stage: "training"                # training | fine-tuning | evaluation
```

## 推理服务监控

### 推理专用 SLO

模型推理服务有区别于传统微服务的独特 SLO：

| SLO 维度 | 指标 | 目标值 | 说明 |
|----------|------|--------|------|
| TTFT (Time To First Token) | 首 token 延迟 | P99 < 500ms | LLM 推理关键指标 |
| TBT (Time Between Tokens) | token 间延迟 | P99 < 50ms | 流式输出流畅度 |
| 吞吐量 | tokens/sec/GPU | > 500 | GPU 利用效率 |
| 错误率 | 5xx + 超时 | < 0.1% | 推理失败率 |
| GPU 利用率 | GPU util % | > 60% | 资源使用效率 |
| 模型准确率 | 业务指标 | 无回归 | 模型质量保障 |

### KServe 推理服务监控

```yaml
# KServe InferenceService 配置可观测性
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llama-7b
  annotations:
    prometheus.io/scrape: "true"
    prometheus.io/port: "8080"
    prometheus.io/path: "/metrics"
spec:
  predictor:
    containers:
      - name: vllm
        image: vllm/vllm-openai:latest
        env:
          - name: VLLM_METRICS_ENABLED
            value: "true"               # 启用推理指标
```

## 模型漂移检测

模型漂移是 ML 系统特有的可观测性维度——输入数据分布或模型预测质量的缓慢退化：

```
漂移类型:
├── 数据漂移 (Data Drift): 输入特征分布变化
├── 概念漂移 (Concept Drift): 特征与目标关系变化
├── 预测漂移 (Prediction Drift): 输出分布变化
└── 性能漂移 (Performance Drift): 准确率/延迟退化
```

## 最佳实践

- **部署 DCGM Exporter 作为 GPU 监控基线**：所有 GPU 节点必须运行 DCGM Exporter，配置 XID 错误告警以快速发现硬件故障
- **训练任务标记实验元数据**：通过 Pod 标签关联 experiment_id，实现资源消耗与实验效果的联合分析
- **定义推理专用 SLO**：TTFT 和 TBT 是 LLM 推理服务的关键 SLO，不要仅用通用 HTTP 延迟衡量
- **监控 GPU 显存碎片**：长时间运行的推理服务可能出现显存碎片导致 OOM——定期监控 `DCGM_FI_DEV_FB_USED` 趋势
- **模型漂移纳入持续监控**：定期评估模型在最新数据上的表现，设置漂移告警触发模型重训练

## 常见陷阱

- **GPU 利用率 ≠ 模型性能**：GPU 利用率 100% 不代表推理高效——可能是 Kernel 启动开销或内存瓶颈，需结合 Tensor Core 利用率分析
- **XID 错误被忽视**：GPU XID 错误是硬件故障的前兆，忽视会导致训练中断和数据丢失——应配置立即告警
- **训练和推理监控混用**：训练关注吞吐量和收敛性，推理关注延迟和可用性——混用 SLO 会导致误判

## 相关 Domain

- AI基础设施/03-gpu-scheduling/01-gpu-scheduling-management
- [[系统基础/知识字典/observability/observability.md|observability]]/02-metrics/02-[[最佳实践/best-practices/observability/monitoring.md|monitoring]]-metrics-system]]

## 相关页面

- [[概念/ai-agent-ops-patterns.md|AI Agent 运维模式]] — 推理服务部署
- [[概念/observability-finops.md|可观测性与 FinOps]] — GPU 成本监控
- [[概念/dynamic-resource-allocation.md|动态资源分配]] — GPU 资源声明


<!-- risk-assessed -->
