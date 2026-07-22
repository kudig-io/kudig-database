---
title: HolmesGPT [entities]
description: '## 概述'
summary: 'HolmesGPT 是一个基于大语言模型（LLM）的 Kubernetes 故障排查助手，能够自动分析集群告警和事件，执行运维调查流程，提供根因分析（RCA）和修复建议。'
category: entities
tags:
- k8s
- cncf
- platform
- holmesgpt
- prometheus
- grafana
- helm
- rbac
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- HolmesGPT 是什么
- 如何 HolmesGPT
trigger_keywords:
- HolmesGPT
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# HolmesGPT

> **CNCF 状态**: Sandbox | **类别**: Platform/AIOps | **主要语言**: Go, Python

## 概述

HolmesGPT 是一个基于大语言模型（LLM）的 Kubernetes 故障排查助手，由 Robusta 开发并开源，2024 年加入 CNCF 沙箱。它能够自动分析集群告警和事件，执行运维调查流程，提供根因分析（RCA）和修复建议。HolmesGPT 将 AI 推理能力与 Kubernetes 原生工具（kubectl、Helm、Prometheus 等）深度结合，实现从告警到根因定位的自动化排查闭环。当告警触发时（如 Pod CrashLoopBackOff），HolmesGPT 自动收集相关上下文（Pod 日志、事件、Deployment 状态、资源使用），调用 LLM 进行分析，并在 Slack/PagerDuty 中返回结构化的根因分析和修复建议。它还支持团队编写自定义 Runbook 作为 AI 的知识库。

## 核心能力

- **自动根因分析**: 接收集管集群告警，自动调查并提供根因分析
- **多源数据采集**: kubectl 日志/事件、Prometheus 指标、Helm 状态、Grafana 面板
- **Runbook 知识库**: 将团队运维经验编写为 Runbook，提高 AI 排查准确率
- **LLM 集成**: 支持 OpenAI GPT-4、Azure OpenAI、本地 Ollama 等多种 LLM 后端
- **告警集成**: Prometheus Alertmanager、Grafana、PagerDuty、Slack 等
- **交互式排查**: 支持 `holmes ask` 命令进行交互式问题排查

## 架构

HolmesGPT 采用 Agent + LLM 的智能运维架构：

- **Holmes Core**: 核心服务，接收告警/问题，协调调查流程
- **Tool Engine**: 工具调用引擎，执行 kubectl、Helm、Prometheus 等查询操作
- **Context Collector**: 自动收集告警关联资源（Pod、Deployment、Node）的上下文信息
- **LLM Orchestrator**: 将上下文和问题组织为 Prompt，调用 LLM 进行推理
- **Runbook Store**: 存储团队运维知识，注入到 LLM 上下文中增强准确性
- **Integration Layer**: 与 Alertmanager、Slack、Teams 等告警和通知系统集成

排查流程：`告警触发 → Context 收集 → Tool 查询 → LLM 推理 → 根因+建议 → Slack/PagerDuty`

## K8s 集成

HolmesGPT 以 Helm Chart 方式部署在 Kubernetes 集群中。通过 ServiceAccount 和 RBAC 授权，HolmesGPT 可以查询集群资源（Pod、Deployment、Node 等）的日志、事件和状态。它与 Prometheus Alertmanager 集成——告警触发时自动调用 Holmes 进行分析。Holmes 还支持通过 Webhook 接收 Grafana、PagerDuty 的告警。生产环境推荐只授予只读权限（get/list/watch），避免 AI 执行破坏性操作。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准可观测性栈（Prometheus + Grafana + Alertmanager）深度集成。

## 生产场景

1. **On-Call 告警增强**: 告警触发时自动附加根因分析，减少工程师调查时间
2. **自动故障诊断**: 新手工程师通过 `holmes ask` 快速定位问题
3. **批量事件分析**: 对批量告警进行去重和关联分析，减少告警疲劳
4. **知识传承**: 将团队运维经验编码为 Runbook，AI 自动引用

## 安装与配置

```bash
# Helm 安装 HolmesGPT
helm repo add robusta https://robusta-chamaeleon.github.io/chamaeleon/
helm install holmes robusta/holmes -n holmes --create-namespace \
  --set openai.apiKey=$OPENAI_API_KEY \
  --set config.alertmanager.url=http://alertmanager.monitoring.svc:9093

# 等待就绪
kubectl wait --for=condition=available deployment/holmes -n holmes --timeout=120s

# 配置 Alertmanager Webhook
# 在 Alertmanager 配置中添加：
# webhook_configs:
#   - url: http://holmes-service.holmes.svc.cluster.local/alerts

# 安装 Holmes CLI
pip install robusta-holmes

# 交互式排查
holmes ask "why is my pod in namespace default crashing?"
holmes ask "investigate the high latency on my-service"

# 从文件排查告警
holmes investigate --alert-file alert.json
```

```yaml
# Holmes 配置示例（含 Runbook）
apiVersion: v1
kind: ConfigMap
metadata:
  name: holmes-config
  namespace: holmes
data:
  config.yaml: |
    llm:
      provider: openai
      model: gpt-4
      temperature: 0.1
    runbooks:
      - name: pod-crashloop
        description: Pod CrashLoopBackOff 排查指南
        steps:
          - 检查 Pod 日志: kubectl logs <pod> --previous
          - 检查事件: kubectl describe pod <pod>
          - 检查资源使用: kubectl top pod <pod>
          - 检查镜像拉取: kubectl get events --field-selector reason=Failed
      - name: high-latency
        description: 服务高延迟排查指南
        steps:
          - 检查 Pod 资源: kubectl top pod -l app=<service>
          - 检查 HPA 状态: kubectl get hpa
          - 检查网络: kubectl exec <pod> -- curl -w "%{time_total}" http://localhost:8080/health
---
# RBAC 配置（只读权限）
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: holmes-readonly
rules:
- apiGroups: ["*"]
  resources: ["*"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["pods/log"]
  verbs: ["get"]
```

## 运维操作

```bash
# 🟢 查看 Holmes 状态
kubectl get pods -n holmes
kubectl logs -n holmes -l app=holmes --tail=50

# 🟢 交互式排查
holmes ask "why is pod payment-service-xxx in namespace production restarting?"

# 🟢 查看排查历史
kubectl get configmap -n holmes -l app=holmes-history

# 🟡 更新 Runbook
kubectl edit configmap holmes-config -n holmes
kubectl rollout restart deployment/holmes -n holmes

# 🟡 切换 LLM 后端（如使用本地 Ollama）
kubectl patch configmap holmes-config -n holmes --type merge \
  -p '{"data":{"config.yaml":"llm:\n  provider: ollama\n  model: llama3\n  baseUrl: http://ollama:11434"}}'

# 🔴 清除排查历史
kubectl delete configmap -n holmes -l app=holmes-history
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Holmes 无响应 | LLM API 不可达或 Key 无效 | `kubectl logs -n holmes` | 检查 API Key 和网络连接 |
| 分析结果不准确 | Runbook 缺失或上下文不足 | 查看 Holmes 日志中的 Prompt | 添加相关 Runbook 知识 |
| Alertmanager 集成失败 | Webhook URL 配置错误 | `kubectl get svc -n holmes` | 检查 Webhook URL 和 Service |
| RBAC 权限不足 | ServiceAccount 缺少权限 | `kubectl auth can-i --as=system:serviceaccount:holmes:holmes` | 更新 ClusterRole 权限 |
| 响应超时 | LLM 推理时间过长或上下文过大 | 查看 Holmes 日志 | 减少上下文大小或切换更快的模型 |

```
排查流程：
├── Holmes 服务异常
│   ├── kubectl get pods -n holmes 检查 Pod 状态
│   ├── kubectl logs 查看错误日志
│   ├── 检查 LLM API 连接和 Key
│   └── 确认 RBAC 权限配置
├── 分析质量问题
│   ├── 检查 Runbook 是否覆盖该场景
│   ├── 查看 Holmes 收集的上下文是否完整
│   ├── 调整 LLM temperature 参数
│   └── 添加更具体的 Runbook 步骤
└── 集成问题
    ├── 检查 Alertmanager Webhook 配置
    ├── 确认 Service 端点可达
    ├── 检查 Slack/PagerDuty Token
    └── 查看 Holmes 接收告警日志
```

## 生产案例

### 案例 1：On-Call 告警智能增强

- **场景**：On-Call 工程师每天处理 50+ 告警，平均每个告警调查需要 15 分钟
- **排查**：告警信息有限，工程师需要手动执行多个 kubectl 命令收集上下文
- **方案**：部署 HolmesGPT 集成 Alertmanager，告警触发时自动收集上下文并分析根因
- **效果**：告警调查时间从 15 分钟降至 3 分钟，70% 的告警可直接根据建议修复

### 案例 2：新手工程师快速排障

- **场景**：新加入的 SRE 不熟悉 K8s 排障流程，处理故障需要老员工指导
- **排查**：新手不知道从哪里开始排查，经常遗漏关键信息，排障效率低
- **方案**：使用 holmes ask 交互式排查，AI 引导执行正确的排查步骤，Runbook 提供团队经验
- **效果**：新手排障效率提升 3x，老员工指导时间减少 80%，团队知识有效传承

## 对比

| 特性 | HolmesGPT | K8sGPT | Robusta | Botkube | 适用场景 |
|------|-----------|--------|---------|---------|----------|
| LLM 排查 | ✅ | ✅ | ⚠️ 规则 | ❌ | 智能根因分析 |
| Runbook | ✅ | ❌ | ✅ | ❌ | 知识传承 |
| 告警增强 | ✅ | ❌ | ✅ | ⚠️ | On-Call 效率 |
| CNCF 状态 | Sandbox | Sandbox | 非 CNCF | 非 CNCF | 开源生态 |
| 生产成熟度 | 中（新项目） | 中 | 高 | 高 | 稳定性要求 |

## 架构定位

在 CNCF 生态中，HolmesGPT 属于 **Platform/AIOps** 类别，为云原生应用提供 AI 驱动的故障排查能力。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[deployment]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[kubeelasti]] — [[实体/kubeelasti.md|KubeElastic]]
- [[xregistry]] — xRegistry
- [[carvel]] — Carvel
- [[helm]] — Helm
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- holmesgpt
- observability|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
