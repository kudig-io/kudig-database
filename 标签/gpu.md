---
title: gpu
description: All pages tagged with gpu
category: tag-index
tags:
- gpu
tier: supporting
created: '2026-07-11'
last_updated: 2026-07
---

# gpu Tag Hub

> GPU 相关页面 — GPU 调度、共享、MIG 分区、AI 推理、GPU 监控、设备插件等。

## AI 基础设施 (AI Infrastructure)

- [[AI基础设施/基础设施/01-ai-infrastructure-overview|AI 基础设施概览]]
- [[AI基础设施/基础设施/02-ai-ml-workloads|AI/ML 工作负载]]
- [[AI基础设施/基础设施/03-gpu-scheduling-management|GPU 调度管理]]
- [[AI基础设施/基础设施/04-gpu-monitoring-dcgm|GPU 监控 DCGM]]
- [[AI基础设施/基础设施/05-distributed-training-frameworks|分布式训练框架]]
- [[AI基础设施/基础设施/10-model-deployment-management|模型部署管理]]
- [[AI基础设施/基础设施/12-ai-cost-analysis-finops|AI 成本分析 FinOps]]
- [[AI基础设施/基础设施/14-troubleshooting-performance|AI 性能故障排查]]
- [[AI基础设施/基础设施/45-ai-ml-ops-runbook|AI/ML Ops Runbook]]
- [[AI基础设施/基础设施/99-kubeflow-ai-platform-guide|Kubeflow AI 平台指南]]

## LLM 推理 (LLM Inference/Serving)

- [[AI基础设施/基础设施/17-llm-inference-serving|LLM 推理服务部署]]
- [[AI基础设施/基础设施/18-llm-serving-architecture|LLM 服务架构]]
- [[AI基础设施/基础设施/19-llm-quantization|LLM 量化]]
- [[AI基础设施/基础设施/25-llm-observability|LLM 可观测性]]
- [[AI基础设施/基础设施/26-cost-optimization-overview|成本优化概览]]

## AI Agent

- [[AI基础设施/AI-Agents/01-ai-agent-fundamentals|AI Agent 基础]]
- [[AI基础设施/AI-Agents/02-llm-foundation-models|LLM 基础模型]]
- [[AI基础设施/AI-Agents/42-model-harness-compatibility-matrix|模型兼容性矩阵]]
- [[AI基础设施/AI-Agents/README|AI Agents 索引]]

## 清单模式 (Manifest Patterns)

- [[清单模式/06-ai-ml-patterns/01-gpu-pod-scheduling|GPU Pod 调度]]
- [[清单模式/06-ai-ml-patterns/02-mig-partitioning-manifests|MIG 分区清单]]
- [[清单模式/06-ai-ml-patterns/03-vllm-deployment-manifest|vLLM 部署清单]]
- [[清单模式/06-ai-ml-patterns/04-triton-deployment-manifest|Triton 部署清单]]
- [[清单模式/06-ai-ml-patterns/05-training-job-pytorch|PyTorch 训练任务]]
- [[清单模式/06-ai-ml-patterns/07-model-serving-hpa|模型服务 HPA]]
- [[清单模式/06-ai-ml-patterns/08-gpu-sharing-time-slicing|GPU 共享与时间切片]]

## 概念 (Concepts)

- [[概念/gpu-scheduling-ai-workloads|GPU 调度与 AI 工作负载]]
- [[概念/k8s-ai-ml-infrastructure|K8s AI/ML 基础设施]]
- [[概念/scheduling-algorithm|调度算法]]
- [[概念/ai-ml-observability|AI/ML 可观测性]]
- [[概念/system-foundation-hardware-kernel|系统基础硬件内核]]
- [[概念/Research: Kubernetes AI-ML Infrastructure 2025-2026|AI/ML 基础设施研究]]
- [[概念/finops-greenops-practices|FinOps/GreenOps 实践]]

## 故障诊断 (Troubleshooting)

- [[故障诊断/高级排障/structural-02-node-components/06-gpu-device-plugin-troubleshooting|GPU 设备插件排障]]
- [[故障诊断/高级排障/structural-09-dra-troubleshooting|DRA 排障]]
- [[故障诊断/高级排障/structural-10-ai-ml-workloads/01-ai-ml-workloads-troubleshooting|AI/ML 工作负载排障]]
- [[故障诊断/FTA故障树/list/gpu-fta|GPU 故障树分析]]
- [[故障诊断/技能体系/skill-set/k8s-node-notready/reference/version-matrix|版本矩阵]]

## 调度 (Scheduling)

- [[技能/learn-15-scheduling-basics|调度基础]]
- [[工作负载/核心工作负载/16-runtime-class-configuration|RuntimeClass 配置]]
- [[工作负载/核心工作负载/05-job-cronjob-advanced|Job/CronJob 高级]]

## 知识字典 (Knowledge Dictionary)

- [[系统基础/知识字典/scheduling/kubernetes-scheduler|Kubernetes 调度器]]
- [[系统基础/知识字典/scheduling/scheduling-framework|调度框架]]
- [[系统基础/知识字典/scheduling/gang-scheduling|Gang Scheduling]]
- [[系统基础/知识字典/scheduling/dynamic-resource-allocation|动态资源分配 (DRA)]]
- [[系统基础/知识字典/scheduling/hami|HAMI]]
- [[系统基础/知识字典/scheduling/taints-and-tolerations|污点与容忍]]
- [[系统基础/知识字典/specialized-workloads/gpu-resource-management-and-partitioning|GPU 资源管理与分区]]
- [[系统基础/知识字典/specialized-workloads/llm-inference-optimization|LLM 推理优化]]
- [[系统基础/知识字典/specialized-workloads/kserve-model-serving|KServe 模型服务]]
- [[系统基础/知识字典/specialized-workloads/kueue-job-queue-management|Kueue 任务队列管理]]

## 平台工程 (Platform Engineering)

- [[平台工程/99-karpenter-node-autoscaling-guide|Karpenter 节点弹性伸缩指南]]
- [[平台工程/治理/03-capacity-planning-resource-assessment|容量规划与资源评估]]
- [[平台工程/代码分析/cluster-create/23-scheduler|kube-scheduler 调度详解]]
- [[平台工程/代码分析/node-create/07-autoscaling|节点弹性伸缩]]

## 硬件 (Hardware)

- [[系统基础/硬件/03-cpu-technology-deep-dive|CPU 技术深度指南]]
- [[系统基础/硬件/05-memory-technology-deep-dive|内存技术深度指南]]
- [[系统基础/硬件/01-cloud-hardware-architecture|云硬件架构]]

## 研究 (Research)

- [[研究/gpu-sharing-scheduling|GPU 共享与调度]]
- [[综合/gpu-scheduling-cost|GPU 调度与成本]]

## 实体 (Entities)

- [[实体/hami|HAMI]]
- [[实体/kaito|KAITO]]
- [[实体/kserve|KServe]]
- [[实体/kubeflow|Kubeflow]]
- [[实体/koordinator|Koordinator]]
- [[实体/k8s-ai-infra-domain-guide|AI Infrastructure on Kubernetes Domain Guide]]
- [[实体/k8s-ai-infrastructure|K8s AI Infrastructure]]

## 应用架构 (Application Architecture)

- [[应用模式/行业架构/08-ai-ml-inference-architecture|AI/ML 推理架构]]
- [[应用模式/行业架构/10-social-media-architecture|社交媒体架构]]
- [[应用模式/行业架构/35-metaverse-digital-twin|元宇宙数字孪生]]
- [[应用模式/行业架构/40-cloud-gaming|云游戏]]
- [[应用模式/行业架构/60-v2x-autonomous-driving|V2X 自动驾驶]]

## 生态参考 (Ecosystem)

- [[生态参考/领域索引/ai-gpu-index|AI/GPU 索引]]
- [[生态参考/论文/17-kubernetes-aiml-gpu-scheduling-llm-inference|AI/ML GPU 调度与 LLM 推理]]

## Related Tags

- [[标签/ai-ml-infra|ai-ml-infra]]
- [[标签/k8s|k8s]]
- [[标签/production|production]]
