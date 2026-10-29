---
title: AI/ML Infrastructure
description: AI 基础设施知识域 — GPU 调度、分布式训练、MLOps、AI Agent、推理服务、RAG、AI 编码
summary: AI 基础设施知识域入口，涵盖 GPU 调度与资源管理、分布式训练框架、MLOps 流水线、AI Agent 架构、推理服务部署、RAG 系统、AI 编码工具
category: domain
tags:
- ai
- ml
- gpu
- mlops
- distributed-training
- agent
- rag
tier: core
created: '2026-05-23'
last_updated: '2026-07-21'
difficulty: advanced
audience:
- AI 工程师
- 平台团队
- 架构师
estimated_read_time: 10min
---
# AI 基础设施 AI Infra

> GPU 调度、分布式训练、MLOps、AI Agent、推理服务与 AI 编码。

## 二级子目录

| 子目录 | 内容 | 核心话题 |
|--------|------|----------|
| [[wiki/15-AI基础设施/01-基础设施/index.md\|基础设施/]] | GPU/硬件 | GPU 调度、NCCL、存储、网络拓扑 |
| [[wiki/15-AI基础设施/05-K8s-AI基础设施/index.md\|K8s-AI基础设施/]] | K8s AI | Volcano/Kueue、Training Operator、MIG |
| [[wiki/15-AI基础设施/02-AI-Agents/README.md\|AI-Agents/]] | Agent | LLM Agent、多智能体、Tool Use、Memory |
| [[wiki/15-AI基础设施/03-Agent运行时/index.md\|Agent运行时/]] | 运行时 | Agent 容器化、沙箱、资源隔离、调度 |
| [[wiki/15-AI基础设施/04-AI编码/README.md\|AI编码/]] | AI 编码 | Copilot/Cursor、代码生成、AI 辅助开发 |

## 跨域导航

- [[wiki/16-专项技术/README.md|专项技术]]
- [[wiki/18-云厂商/README.md|云厂商]]
- [[wiki/11-发布变更/README.md|发布变更]]
- [[wiki/09-可观测性/README.md|可观测性]]
- [[wiki/12-可靠性/README.md|可靠性]]
- [[wiki/06-存储/README.md|存储]]
- [[wiki/08-安全/README.md|安全]]
- [[wiki/14-容器运行时/README.md|容器运行时]]
- [[wiki/02-工作负载/README.md|工作负载]]
- [[wiki/10-平台工程/README.md|平台工程]]
- [[wiki/04-应用模式/README.md|应用模式]]
- [[wiki/19-故障诊断/README.md|故障诊断]]
- [[wiki/07-数据库中间件/README.md|数据库中间件]]
- [[wiki/03-清单模式/README.md|清单模式]]
- [[wiki/13-生产运维/README.md|生产运维]]
- [[wiki/21-生态参考/README.md|生态参考]]
- [[wiki/17-系统基础/README.md|系统基础]]
- [[wiki/05-网络/README.md|网络]]
- [[wiki/01-集群基础/README.md|集群基础]]

## Related

- [[wiki/02-工作负载/04-多语言运行时/02-python-on-kubernetes-production|Python 应用 Kubernetes 生产实践]]
- [[wiki/02-工作负载/04-多语言运行时/01-go-on-kubernetes-production|Go 应用 Kubernetes 生产实践]]
- [[wiki/02-工作负载/04-多语言运行时/03-rust-on-kubernetes-production|Rust 应用 Kubernetes 生产实践]]
- [[wiki/02-工作负载/04-多语言运行时/04-gpu-workload-management|GPU 工作负载管理]]
- [[wiki/09-可观测性/00-总览/13-ebpf-observability-deep-dive|eBPF 可观测性深度实践]]
- [[wiki/12-可靠性/06-SRE实践/13-ai-workload-reliability|AI 工作负载可靠性]]
- [[wiki/19-故障诊断/04-高级排障/structural-README|Kubernetes 结构化故障排查知识库 [故障诊断]]]
- [[skills/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine|FTA Diagnostic Execution Engine]]
- [[wiki/26-标签/06-AI与专项/research|research]]
