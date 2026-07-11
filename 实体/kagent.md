---
title: Kagent
description: '## 概述'
summary: 'Kagent 是一个 Kubernetes 原生的 AI Agent 平台，使开发者能够在 Kubernetes 上构建、部署和管理 AI Agent。它基于 AutoGen 框架，通过 CRD 声明式定义 AI Agent 的工具集、模型配置和对话流程，将 AI Agent 作为 Kubernetes 资源进行管理。'
category: entities
tags:
- k8s
- cncf
- platform
- kagent
- prometheus
- grafana
- rbac
- crd
- operator
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kagent 是什么
- 如何 Kagent
trigger_keywords:
- Kagent
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kagent

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Python, Go

## 概述

Kagent 是一个 Kubernetes 原生的 AI Agent 平台，2024 年加入 CNCF Sandbox。它使开发者能够在 Kubernetes 上构建、部署和管理 AI Agent。Kagent 基于 Microsoft AutoGen 框架，通过 CRD 声明式定义 AI Agent 的工具集、模型配置和对话流程，将 AI Agent 作为 Kubernetes 资源进行管理。Kagent 内置了丰富的 Kubernetes 运维工具，使 AI Agent 能够自主诊断和操作集群。

## 核心特性

- **CRD 声明式**: Agent、Tool、ModelConfig 等 CRD 定义 Agent 行为
- **AutoGen 框架**: 基于 Microsoft AutoGen 的多 Agent 对话编排
- **内置 K8s 工具**: 预置 kubectl、日志查询、资源检查等运维工具
- **多模型支持**: OpenAI、Anthropic、Azure、Ollama、LocalAI
- **工具链**: Python 函数工具、HTTP API 工具、数据库查询工具
- **工作流编排**: 支持多 Agent 协作和 Pipeline 工作流

## 架构

Kagent 由 Controller、Agent Runtime 和 Tools 组成。Controller 监听 Agent CRD，为每个 Agent 部署 Runtime（Python Deployment）。Runtime 基于 AutoGen 框架，加载 Agent 定义中的模型配置和工具列表，在收到请求时编排 Agent 对话。工具（Tool）是 Python 函数或 HTTP 端点，Agent 通过函数调用（Function Calling）使用工具执行操作。模型配置（ModelConfig）定义 LLM 后端参数。所有组件以 Kubernetes 原生资源运行。

## Kubernetes 集成

Kagent 通过 CRD（Agent、Tool、ModelConfig、AgentWorkflow）声明式管理 AI Agent。Controller 以 Deployment 运行，管理 Agent Runtime 的生命周期。内置的 Kubernetes 工具通过 ServiceAccount 和 RBAC 访问集群 API。Agent 可以执行只读操作（get/describe/logs）或写操作（apply/delete），权限通过 RBAC 控制。支持通过 Webhook 或 CLI 触发 Agent 执行。

## 生产使用场景

1. **运维 AI 助手**: Agent 自主分析集群告警并执行诊断命令
2. **自动化修复**: Agent 检测 Pod 故障并尝试自动修复（如重启、扩容）
3. **安全审计**: Agent 定期审计 RBAC 配置和网络安全策略
4. **DevOps 助手**: Agent 辅助开发者创建部署配置和排障

## 安装

```bash
# Helm 安装
helm repo add kagent https://kagent.dev/charts
helm install kagent kagent/kagent -n kagent --create-namespace
# 定义 Agent
kubectl apply -f - <<EOF
apiVersion: kagent.dev/v1alpha1
kind: Agent
metadata: { name: ops-assistant }
spec:
  description: "Kubernetes 运维助手"
  modelConfig:
    provider: openai
    model: gpt-4o
  tools:
  - kubectl_get
  - kubectl_describe
  - kubectl_logs
  systemMessage: "你是一个 Kubernetes 运维专家..."
EOF
```

## 替代方案

| 项目 | 优势 | 劣势 |
|------|------|------|
| **Kagent** | K8s 原生、CRD 管理 | 较新、社区小 |
| K8sGPT | 成熟、简单 | 单 Agent、无自定义工具 |
| HolmesGPT | 诊断能力强 | 与 Robusta 绑定 |
| AutoGen (非 K8s) | 灵活、多 Agent | 非 K8s 原生 |

## 架构定位

在 CNCF 生态中，Kagent 属于 **Platform / AI Operations** 类别，代表了 AI Agent 在 Kubernetes 运维领域的应用方向。它与 K8sGPT、HolmesGPT 互补。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[volcano]] — Volcano
- [[bpfman]] — bpfman
- [[in-toto]] — in-toto
- [[grpc]] — gRPC
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kagent
- [[实体/cncf-edge-ai.md|[[CNCF 边缘计算与 AI/ML 项目全景|CNCF 边缘计算与 AI/ML 项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
