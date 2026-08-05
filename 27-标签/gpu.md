---
title: gpu
description: GPU 调度标签枢纽 — 涵盖 GPU 调度、共享、MIG 分区、AI 推理、GPU 监控、设备插件、GPU Operator 等全部 GPU 领域知识
category: tag-index
tags:
- gpu
- nvidia
- cuda
- device-plugin
- gpu-sharing
tier: core
difficulty: intermediate-to-advanced
domain: ai-infrastructure
created: '2026-07-11'
last_updated: '2026-07-21'
---

# gpu Tag Hub

> GPU 相关页面 — GPU 调度、共享、MIG 分区、AI 推理、GPU 监控、设备插件等。

## 核心定义

**GPU 调度**是 Kubernetes 中管理 GPU 资源分配的核心能力，通过 Device Plugin 机制将 GPU 暴露为可扩展资源（nvidia.com/gpu），支持独占、共享、MIG 分区等多种分配模式。

### GPU 调度模式

| 模式 | 描述 | 适用场景 |
|------|------|----------|
| 独占 (Exclusive) | 整卡分配给单个 Pod | 训练、大模型推理 |
| 共享 (Time-slicing) | 多 Pod 时间片轮转 | 开发、小模型推理 |
| MIG 分区 | 硬件级切分 (A100/H100) | 多租户隔离 |
| vGPU | 虚拟化切分 | 云环境 |


## AI 基础设施 (AI Infrastructure)

- [[15-AI基础设施/01-基础设施/01-ai-infrastructure-overview|AI 基础设施概览]]
- [[15-AI基础设施/01-基础设施/02-ai-ml-workloads|AI/ML 工作负载]]
- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management|GPU 调度管理]]
- [[15-AI基础设施/01-基础设施/04-gpu-monitoring-dcgm|GPU 监控 DCGM]]
- [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks|分布式训练框架]]
- [[15-AI基础设施/01-基础设施/10-model-deployment-management|模型部署管理]]
- [[15-AI基础设施/01-基础设施/12-ai-cost-analysis-finops|AI 成本分析 FinOps]]
- [[15-AI基础设施/01-基础设施/14-troubleshooting-performance|AI 性能故障排查]]
- [[15-AI基础设施/01-基础设施/38-ai-ml-ops-runbook|AI/ML Ops Runbook]]
- [[15-AI基础设施/01-基础设施/39-kubeflow-ai-platform-guide|Kubeflow AI 平台指南]]

## LLM 推理 (LLM Inference/Serving)

- [[15-AI基础设施/01-基础设施/17-llm-inference-serving|LLM 推理服务部署]]
- [[15-AI基础设施/01-基础设施/18-llm-serving-architecture|LLM 服务架构]]
- [[15-AI基础设施/01-基础设施/19-llm-quantization|LLM 量化]]
- [[15-AI基础设施/01-基础设施/25-llm-observability|LLM 可观测性]]
- [[15-AI基础设施/01-基础设施/26-cost-optimization-overview|成本优化概览]]

## AI Agent

- [[15-AI基础设施/02-AI-Agents/01-ai-agent-fundamentals|AI Agent 基础]]
- [[15-AI基础设施/02-AI-Agents/02-llm-foundation-models|LLM 基础模型]]
- [[15-AI基础设施/02-AI-Agents/42-model-harness-compatibility-matrix|模型兼容性矩阵]]
- [[15-AI基础设施/02-AI-Agents/README|AI Agents 索引]]

## 清单模式 (Manifest Patterns)

- [[03-清单模式/07-AI-ML模式/01-gpu-pod-scheduling|GPU Pod 调度]]
- [[03-清单模式/07-AI-ML模式/02-mig-partitioning-manifests|MIG 分区清单]]
- [[03-清单模式/07-AI-ML模式/03-vllm-deployment-manifest|vLLM 部署清单]]
- [[03-清单模式/07-AI-ML模式/04-triton-deployment-manifest|Triton 部署清单]]
- [[03-清单模式/07-AI-ML模式/05-training-job-pytorch|PyTorch 训练任务]]
- [[03-清单模式/07-AI-ML模式/07-model-serving-hpa|模型服务 HPA]]
- [[03-清单模式/07-AI-ML模式/08-gpu-sharing-time-slicing|GPU 共享与时间切片]]

## 概念 (Concepts)

- [[22-概念/07-调度与资源/gpu-scheduling-ai-workloads|GPU 调度与 AI 工作负载]]
- [[22-概念/12-研究/k8s-ai-ml-infrastructure|K8s AI/ML 基础设施]]
- [[22-概念/07-调度与资源/scheduling-algorithm|调度算法]]
- [[22-概念/06-可观测性/ai-ml-observability|AI/ML 可观测性]]
- [[22-概念/15-运行时与系统/system-foundation-hardware-kernel|系统基础硬件内核]]
- [[22-概念/13-research-2025-2026/01-AI-ML-Infrastructure|AI/ML 基础设施研究]]
- [[22-概念/08-可靠性与运维/finops-greenops-practices|FinOps/GreenOps 实践]]

## 故障诊断 (Troubleshooting)

- [[19-故障诊断/04-高级排障/structural-02-node-components/06-gpu-device-plugin-troubleshooting|GPU 设备插件排障]]
- [[19-故障诊断/04-高级排障/structural-09-dra-troubleshooting|DRA 排障]]
- [[19-故障诊断/04-高级排障/structural-10-ai-ml-workloads/01-ai-ml-workloads-troubleshooting|AI/ML 工作负载排障]]
- [[19-故障诊断/06-FTA故障树/list/gpu-fta|GPU 故障树分析]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/reference/version-matrix|版本矩阵]]

## 调度 (Scheduling)

- [[26-技能/02-控制面/scheduler/培训/learn-15-scheduling-basics.md|调度基础]]
- [[02-工作负载/01-核心工作负载/16-runtime-class-configuration|RuntimeClass 配置]]
- [[02-工作负载/01-核心工作负载/05-job-cronjob-advanced|Job/CronJob 高级]]

## 知识字典 (Knowledge Dictionary)

- [[17-系统基础/06-知识字典/scheduling/kubernetes-scheduler|Kubernetes 调度器]]
- [[17-系统基础/06-知识字典/scheduling/scheduling-framework|调度框架]]
- [[17-系统基础/06-知识字典/scheduling/gang-scheduling|Gang Scheduling]]
- [[17-系统基础/06-知识字典/scheduling/dynamic-resource-allocation|动态资源分配 (DRA)]]
- [[17-系统基础/06-知识字典/scheduling/hami|HAMI]]
- [[17-系统基础/06-知识字典/scheduling/taints-and-tolerations|污点与容忍]]
- [[17-系统基础/06-知识字典/specialized-workloads/gpu-resource-management-and-partitioning|GPU 资源管理与分区]]
- [[17-系统基础/06-知识字典/specialized-workloads/llm-inference-optimization|LLM 推理优化]]
- [[17-系统基础/06-知识字典/specialized-workloads/kserve-model-serving|KServe 模型服务]]
- [[17-系统基础/06-知识字典/specialized-workloads/kueue-job-queue-management|Kueue 任务队列管理]]

## 平台工程 (Platform Engineering)

- [[10-平台工程/02-运维/17-karpenter-node-autoscaling-guide|Karpenter 节点弹性伸缩指南]]
- [[10-平台工程/03-治理/01-capacity-planning-resource-assessment|容量规划与资源评估]]
- [[10-平台工程/06-代码分析/cluster-create/23-scheduler|kube-scheduler 调度详解]]
- [[10-平台工程/06-代码分析/node-create/07-autoscaling|节点弹性伸缩]]

## 硬件 (Hardware)

- [[17-系统基础/02-硬件/03-cpu-technology-deep-dive|CPU 技术深度指南]]
- [[17-系统基础/02-硬件/05-memory-technology-deep-dive|内存技术深度指南]]
- [[17-系统基础/02-硬件/01-cloud-hardware-architecture|云硬件架构]]

## 研究 (Research)

- [[25-研究/01-AI与边缘/gpu-sharing-scheduling|GPU 共享与调度]]
- [[24-综合/01-AI与机器学习/gpu-scheduling-cost|GPU 调度与成本]]

## 实体 (Entities)

- [[23-实体/11-AI与边缘/hami|HAMI]]
- [[23-实体/11-AI与边缘/kaito|KAITO]]
- [[23-实体/11-AI与边缘/kserve|KServe]]
- [[23-实体/11-AI与边缘/kubeflow|Kubeflow]]
- [[23-实体/09-编排调度/koordinator|Koordinator]]
- [[23-实体/15-参考与索引/k8s-ai-infra-domain-guide|AI Infrastructure on Kubernetes Domain Guide]]
- [[23-实体/15-参考与索引/k8s-ai-infrastructure|K8s AI Infrastructure]]

## 应用架构 (Application Architecture)

- [[04-应用模式/02-行业架构/08-ai-ml-inference-architecture|AI/ML 推理架构]]
- [[04-应用模式/02-行业架构/10-social-media-architecture|社交媒体架构]]
- [[04-应用模式/02-行业架构/35-metaverse-digital-twin|元宇宙数字孪生]]
- [[04-应用模式/02-行业架构/40-cloud-gaming|云游戏]]
- [[04-应用模式/02-行业架构/60-v2x-autonomous-driving|V2X 自动驾驶]]

## 生态参考 (Ecosystem)

- [[21-生态参考/03-领域索引/ai-gpu-index|AI/GPU 索引]]
- [[21-生态参考/02-论文/17-kubernetes-aiml-gpu-scheduling-llm-inference|AI/ML GPU 调度与 LLM 推理]]

## GPU 技术全景

### GPU 在 K8s 中的管理

| 组件 | 功能 |
|---|---|
| Device Plugin | GPU 资源注册与分配 |
| GPU Operator | 驱动/运行时自动部署 |
| DCGM Exporter | GPU 指标导出 |
| MPS | 多进程共享 GPU |

### GPU 调度策略

| 策略 | 说明 | 适用场景 |
|---|---|---|
| 整卡分配 | 独占 GPU | 训练任务 |
| GPU 共享 | 多 Pod 共享 | 推理服务 |
| 拓扑感知 | NVLink 优先 | 分布式训练 |
| 抢占调度 | 优先级抢占 | 混合负载 |

## 面试要点

1. **Q：K8s 中 GPU 资源如何管理？**
   A：Device Plugin 注册→调度器分配→kubelet 挂载→容器使用。nvidia.com/gpu 资源类型。

2. **Q：GPU 共享方案有哪些？**
   A：MPS(多进程)、MIG(多实例)、vGPU(虚拟化)、时间片轮转。

3. **Q：GPU 集群调度的挑战？**
   A：拓扑感知(NVLink)、显存管理、故障检测、资源碎片、多租户隔离。

## Related Tags

- [[27-标签/ai-ml-infra|ai-ml-infra]]
- [[27-标签/k8s|k8s]]
- [[27-标签/production|production]]
