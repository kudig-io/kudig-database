---
title: Kagent (Kubernetes AI Agent)
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- prometheus
- grafana
- helm
- rbac
- crd
- operator
- vllm
- llm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kagent (Kubernetes AI Agent) 是什么
- 如何 Kagent (Kubernetes AI Agent)
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Kagent
- Kubernetes
- AI
- Agent
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- monitoring-basics
---

title: Kagent (Kubernetes AI Agent)
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- grafana
- helm
- rbac
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Kagent (Kubernetes AI Agent) 是什么
- 如何 Kagent (Kubernetes AI Agent)
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Kagent
- Kubernetes
- AI
- Agent
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# Kagent (Kubernetes AI Agent)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kagent.dev/ |
| **GitHub** | https://github.com/kagent-dev/kagent |
| **许可证** | Apache-2.0 |
| **开发语言** | Python, Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Kagent 是一个 Kubernetes 原生的 AI Agent 平台，使开发者能够在 Kubernetes 上构建、部署和管理 AI Agent。它基于 AutoGen 框架，通过 CRD 声明式定义 AI Agent 的工具集、模型配置和对话流程，将 AI Agent 作为 Kubernetes 资源进行管理。Kagent 内置了丰富的 Kubernetes 运维工具，使 AI Agent 能够直接与集群交互执行故障排查、监控分析等任务。

### 核心特性

- **Kubernetes CRD**: 通过 Agent、Tool、ModelConfig 等 CRD 声明式管理 AI Agent
- **内置 K8s 工具**: 预集成 kubectl、Helm、Prometheus 查询等运维工具
- **多模型支持**: 支持 OpenAI、Azure OpenAI、本地 LLM 等多种模型后端
- **对话式运维**: AI Agent 理解自然语言指令执行 Kubernetes 运维操作
- **安全控制**: 通过 RBAC 和工具白名单控制 Agent 的操作权限
- **Web UI**: 内置对话界面与 Agent 交互

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│           Kagent Platform                          │
│                                                    │
│  ┌──────────┐  ┌───────────────┐  ┌───────────┐  │
│  │ Web UI   │  │ Kagent API    │  │ Operator  │  │
│  │ (对话界面)│  │ Server        │  │ (CRD 控制)│  │
│  └────┬─────┘  └───────┬───────┘  └─────┬─────┘  │
│       │                │                 │         │
│  ┌────▼────────────────▼─────────────────▼────┐   │
│  │            AutoGen Runtime                  │   │
│  │                                              │   │
│  │  ┌─────────┐  ┌──────────┐  ┌───────────┐  │   │
│  │  │ Agent 1 │  │ Agent 2  │  │ Agent N   │  │   │
│  │  │(K8s Ops)│  │(监控分析)│  │(自定义)   │  │   │
│  │  └────┬────┘  └────┬─────┘  └─────┬─────┘  │   │
│  │       │             │              │         │   │
│  │  ┌────▼─────────────▼──────────────▼────┐   │   │
│  │  │          Tool Registry               │   │   │
│  │  │ kubectl│ helm │ prometheus │ custom  │   │   │
│  │  └─────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────┘   │
│                                                    │
│  ┌────────────────────────────────────────────┐   │
│  │         Model Backends                      │   │
│  │  OpenAI │ Azure OpenAI │ Ollama │ vLLM    │   │
│  └────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 安装 Kagent CLI
curl -fsSL https://kagent.dev/install.sh | bash

# 在集群中安装 Kagent
kagent install \
  --set modelConfig.openai.apiKey=$OPENAI_API_KEY

# 或使用 Helm
helm repo add kagent https://kagent-dev.github.io/kagent/
helm install kagent kagent/kagent \
  --namespace kagent \
  --create-namespace \
  --set apiKey=$OPENAI_API_KEY
```

### 定义 AI Agent

```yaml
apiVersion: kagent.dev/v1alpha1
kind: Agent
metadata:
  name: k8s-troubleshooter
spec:
  description: "Kubernetes cluster troubleshooting agent"
  systemMessage: |
    You are a Kubernetes expert. Help users diagnose and fix
    cluster issues. Always explain what you're doing before
    executing commands. Be cautious with destructive operations.
  modelConfigRef: gpt-4
  tools:
    - name: kubectl-get
    - name: kubectl-describe
    - name: kubectl-logs
    - name: prometheus-query
    - name: helm-list
```

### 配置模型

```yaml
apiVersion: kagent.dev/v1alpha1
kind: ModelConfig
metadata:
  name: gpt-4
spec:
  provider: openai
  model: gpt-4
  apiKeySecretRef:
    name: openai-credentials
    key: api-key
  parameters:
    temperature: 0.1
    maxTokens: 4096
```

### 自定义工具

```yaml
apiVersion: kagent.dev/v1alpha1
kind: Tool
metadata:
  name: check-pod-health
spec:
  description: "Check the health status of pods in a namespace"
  type: shell
  command: |
    kubectl get pods -n {{ .namespace }} \
      --field-selector=status.phase!=Running \
      -o wide
  parameters:
    - name: namespace
      type: string
      description: "Target namespace"
      required: true
```

### 与 Agent 对话

```bash
# 通过 CLI 对话
kagent chat k8s-troubleshooter

# 用户: "为什么 production 命名空间中有 Pod 处于 CrashLoopBackOff?"
# Agent 会:
# 1. 运行 kubectl get pods -n production 查看状态
# 2. 运行 kubectl describe pod <failing-pod> 查看事件
# 3. 运行 kubectl logs <failing-pod> 查看日志
# 4. 分析问题原因并给出修复建议
```

---

## 内置工具集

| 工具类别 | 工具 | 说明 |
|:---|:---|:---|
| Kubernetes | kubectl-get/describe/logs | 资源查询和日志 |
| Kubernetes | kubectl-apply/delete | 资源管理（需授权） |
| Helm | helm-list/status/history | Helm Release 管理 |
| 监控 | prometheus-query | PromQL 查询 |
| 监控 | grafana-search | Grafana 面板搜索 |
| 网络 | curl | HTTP 请求测试 |
| 自定义 | shell | 自定义 Shell 命令 |

---

## 最佳实践

1. **最小权限**: 仅赋予 Agent 必要的工具和 RBAC 权限，避免操作过度
2. **只读优先**: 排查类 Agent 默认只授予只读工具（get/describe/logs）
3. **审计日志**: 启用 Agent 操作审计，记录所有工具调用和结果
4. **模型温度**: 运维类 Agent 使用低 temperature (0.1)，确保输出稳定可靠
5. **人工确认**: 对于删除、重启等破坏性操作，配置需要人工确认的审批流

---

## 参考资源

- [Kagent 官方文档](https://kagent.dev/docs/)
- [Kagent GitHub](https://github.com/kagent-dev/kagent)
- [AutoGen 框架](https://github.com/microsoft/autogen)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[log.md|log]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/promql.md|promql]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
