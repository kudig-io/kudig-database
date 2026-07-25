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

## 安装与配置

```bash
# Helm 安装 Kagent 控制平面
helm repo add kagent https://kagent.dev/charts
helm install kagent kagent/kagent \
  -n kagent --create-namespace \
  --set controller.replicas=2 \
  --set openai.apiKey="${OPENAI_API_KEY}"
# 等待 Controller 就绪
kubectl wait --for=condition=available deployment/kagent-controller -n kagent --timeout=120s
```

```yaml
# Agent CRD 完整示例
apiVersion: kagent.dev/v1alpha1
kind: Agent
metadata:
  name: ops-assistant
  namespace: kagent
spec:
  description: "Kubernetes 运维诊断助手"
  modelConfig:
    provider: openai
    model: gpt-4o
    temperature: 0.1
    maxTokens: 4096
  tools:
  - name: kubectl_get
    config:
      allowedResources: ["pods", "deployments", "services", "nodes"]
      allowedNamespaces: ["default", "production"]
  - name: kubectl_describe
  - name: kubectl_logs
    config:
      maxLines: 200
  - name: prometheus_query
    config:
      endpoint: http://prometheus.monitoring:9090
  systemMessage: |
    你是一个 Kubernetes 运维专家。当用户描述问题时：
    1. 先使用 kubectl_get 查看相关资源状态
    2. 使用 kubectl_describe 获取详细事件
    3. 使用 kubectl_logs 查看容器日志
    4. 基于以上信息给出诊断和修复建议
---
# ModelConfig CRD
apiVersion: kagent.dev/v1alpha1
kind: ModelConfig
metadata:
  name: gpt4o-config
spec:
  provider: openai
  model: gpt-4o
  apiKeySecret:
    name: openai-secret
    key: api-key
---
# Tool CRD（自定义 HTTP 工具）
apiVersion: kagent.dev/v1alpha1
kind: Tool
metadata:
  name: alertmanager-query
spec:
  type: http
  config:
    url: http://alertmanager.monitoring:9093/api/v2/alerts
    method: GET
    headers:
      Content-Type: application/json
```

## 运维操作

```bash
# 🟢 低风险：查看 Agent 状态
kubectl get agents -A
kubectl describe agent ops-assistant -n kagent
kubectl get modelconfigs -A

# 🟢 低风险：查看 Agent Runtime 日志
kubectl logs -l app=kagent-runtime -n kagent -f

# 🟡 中风险：触发 Agent 执行
kubectl exec -it deploy/kagent-controller -n kagent -- \
  kagent run --agent ops-assistant --query "检查 production 命名空间所有 Pod 状态"

# 🟡 中风险：更新 Agent 配置
kubectl patch agent ops-assistant -n kagent --type merge \
  -p '{"spec":{"modelConfig":{"model":"gpt-4o-mini"}}}'

# 🔴 高风险：删除 Agent（停止所有 AI 运维操作）
kubectl delete agent ops-assistant -n kagent
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Agent 无响应 | LLM API Key 无效/过期 | `kubectl logs deploy/kagent-controller -n kagent` | 更新 Secret 中的 API Key |
| 工具调用失败 | RBAC 权限不足 | `kubectl auth can-i get pods --as=system:serviceaccount:kagent:agent-sa` | 扩展 ServiceAccount RBAC |
| Agent Runtime CrashLoop | 模型配置错误 | `kubectl describe pod -l app=kagent-runtime` | 检查 ModelConfig CRD 参数 |
| 响应超时 | LLM 服务不可达 | `kubectl exec deploy/kagent-controller -- curl -s https://api.openai.com/v1/models` | 检查网络策略和代理配置 |
| 工具返回空结果 | 资源名称/命名空间错误 | `kubectl get agent <name> -o yaml` | 修正 tools 配置中的资源范围 |

```
排查流程：
├── Agent 不响应？
│   ├── kubectl get agents → 检查 Agent 状态
│   ├── kubectl logs controller → 查看编排错误
│   └── 检查 ModelConfig 和 API Key Secret
├── 工具执行失败？
│   ├── kubectl auth can-i → 验证 RBAC
│   ├── 检查工具配置中的资源范围
│   └── 查看 Agent Runtime 日志中的工具调用记录
└── 响应质量差？
    ├── 调整 systemMessage 提示词
    ├── 增加/减少工具集范围
    └── 调整 temperature 和 maxTokens
```

## 生产案例

### 案例 1：AI Agent 自动诊断 Pod CrashLoopBackOff

- **场景**：生产环境多个服务频繁 CrashLoop，值班工程师使用 Kagent Agent 辅助诊断
- **排查**：Agent 自动执行 `kubectl get pods`、`kubectl describe`、`kubectl logs`，发现是 ConfigMap 更新后环境变量未生效（缺少滚动重启）
- **方案**：Agent 建议执行 `kubectl rollout restart deployment`，工程师确认后 Agent 执行修复，同时建议添加 configmap-reload sidecar 防止复发
- **效果**：诊断时间从 30min 缩短至 3min，后续配置 ConfigMap 变更自动触发滚动更新

### 案例 2：RBAC 安全审计自动化

- **场景**：安全团队要求每周审计集群 RBAC 配置，检查过度授权
- **排查**：配置 Agent 定期扫描 ClusterRoleBinding，识别 `cluster-admin` 绑定和通配符权限
- **方案**：Agent 生成审计报告，标记 12 个过度授权的 ServiceAccount，建议最小权限原则修改
- **效果**：审计时间从 2 天缩短至 10 分钟，RBAC 违规减少 85%

## 替代方案

| 维度 | Kagent | K8sGPT | HolmesGPT | AutoGen (non-K8s) |
|------|--------|--------|-----------|-------------------|
| K8s 原生 | ✅ CRD | ✅ | ⚠️ | ❌ |
| 多 Agent | ✅ | ❌ 单 Agent | ⚠️ | ✅ |
| 自定义工具 | ✅ Tool CRD | ❌ | ⚠️ | ✅ |
| 成熟度 | 较新 | 成熟 | 中等 | 成熟 |
| 适用场景 | K8s AI 运维平台 | 快速诊断 | Robusta 集成 | 通用 AI 编排 |

## 架构定位

在 CNCF 生态中，Kagent 属于 **Platform / AI Operations** 类别，代表了 AI Agent 在 Kubernetes 运维领域的应用方向。它与 K8sGPT、HolmesGPT 互补。

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[volcano]] — Volcano
- [[bpfman]] — bpfman
- [[in-toto]] — in-toto
- [[grpc]] — gRPC
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kagent
- [[23-实体/cncf-edge-ai.md|[[CNCF 边缘计算与 AI/ML 项目全景|CNCF 边缘计算与 AI/ML 项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
