# HolmesGPT

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://docs.robusta.dev/master/configuration/ai-analysis.html |
| **GitHub** | https://github.com/robusta-dev/holmesgpt |
| **许可证** | MIT |
| **开发语言** | Python |
| **CNCF 状态** | Sandbox |

---

## 项目概述

HolmesGPT 是一个基于大语言模型（LLM）的 Kubernetes 故障排查助手，能够自动分析集群告警和事件，执行运维调查流程，提供根因分析（RCA）和修复建议。它将 AI 推理能力与 Kubernetes 原生工具（kubectl、Helm 等）结合，实现从告警到根因定位的自动化排查闭环。

### 核心特性

- **AI 根因分析**: 利用 LLM 自动分析 Kubernetes 告警，定位根因并提供修复建议
- **工具调用能力**: AI Agent 可自主执行 kubectl、helm 等命令收集诊断信息
- **多 LLM 支持**: 支持 OpenAI、Azure OpenAI、AWS Bedrock、Google Vertex 等多种后端
- **告警集成**: 与 Prometheus、AlertManager、OpsGenie、PagerDuty 等告警系统集成
- **Runbook 自动化**: 支持自定义 Runbook 将运维知识编码为可执行的调查步骤
- **交互模式**: 支持命令行交互和 Slack/Teams 集成

---

## 架构设计

```
┌─────────────────────────────────────────────────────┐
│                    HolmesGPT                         │
│                                                      │
│  ┌───────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │  Alert     │  │   LLM Engine │  │  Toolbox     │ │
│  │  Receiver  │  │  (OpenAI/    │  │  (kubectl/   │ │
│  │  (Prom/    │  │   Bedrock/   │  │   helm/      │ │
│  │   PD/OG)   │  │   Vertex)    │  │   logs)      │ │
│  └─────┬──────┘  └──────┬───────┘  └──────┬───────┘ │
│        │                │                  │         │
│  ┌─────▼────────────────▼──────────────────▼───────┐ │
│  │              AI Agent Framework                  │ │
│  │   (ReAct: 推理 → 工具调用 → 观察 → 推理)        │ │
│  └─────┬────────────────────────────────────┬──────┘ │
│        │                                    │        │
│  ┌─────▼──────────┐              ┌──────────▼──────┐ │
│  │   Runbook       │              │  Output         │ │
│  │   Engine        │              │  (Slack/CLI/    │ │
│  │   (自定义调查)  │              │   Webhook)      │ │
│  └────────────────┘              └─────────────────┘ │
└─────────────────────────────────────────────────────┘
        │
   ┌────▼──────────────────────────────┐
   │       Kubernetes Cluster           │
   │  Pods / Events / Logs / Metrics    │
   └────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 使用 pip 安装
pip install holmesgpt

# 或使用容器运行
docker run -it --rm \
  -v ~/.kube/config:/root/.kube/config \
  -e OPENAI_API_KEY=$OPENAI_API_KEY \
  us-central1-docker.pkg.dev/genuine-flight-317411/devel/holmes:latest
```

### 基本配置

```bash
# 设置 OpenAI API Key
export OPENAI_API_KEY="sk-..."

# 分析当前集群的所有告警
holmes investigate \
  --alertmanager-url http://prometheus-alertmanager:9093

# 交互式排查
holmes ask "Why is my pod CrashLoopBackOff in namespace production?"

# 分析特定告警
holmes investigate \
  --alertmanager-url http://alertmanager:9093 \
  --alert-name KubePodCrashLooping
```

### Kubernetes 部署

```yaml
# holmes-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: holmesgpt
  namespace: monitoring
spec:
  replicas: 1
  selector:
    matchLabels:
      app: holmesgpt
  template:
    metadata:
      labels:
        app: holmesgpt
    spec:
      serviceAccountName: holmesgpt
      containers:
        - name: holmesgpt
          image: us-central1-docker.pkg.dev/genuine-flight-317411/devel/holmes:latest
          env:
            - name: OPENAI_API_KEY
              valueFrom:
                secretKeyRef:
                  name: holmesgpt-secrets
                  key: openai-api-key
          args:
            - "investigate"
            - "--alertmanager-url=http://prometheus-alertmanager:9093"
---
apiVersion: v1
kind: ServiceAccount
metadata:
  name: holmesgpt
  namespace: monitoring
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: holmesgpt
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: cluster-admin
subjects:
  - kind: ServiceAccount
    name: holmesgpt
    namespace: monitoring
```

---

## 高级功能

### 自定义 Runbook

```yaml
# runbooks/crashloop.yaml
apiVersion: holmes/v1
kind: Runbook
metadata:
  name: crashloop-investigation
triggers:
  - alert_name: KubePodCrashLooping
steps:
  - name: get-pod-status
    tool: kubectl
    command: "get pod {{ $pod }} -n {{ $namespace }} -o yaml"
  - name: get-pod-logs
    tool: kubectl
    command: "logs {{ $pod }} -n {{ $namespace }} --previous --tail=100"
  - name: get-events
    tool: kubectl
    command: "get events -n {{ $namespace }} --field-selector involvedObject.name={{ $pod }}"
  - name: check-resources
    tool: kubectl
    command: "top pod {{ $pod }} -n {{ $namespace }}"
analysis:
  prompt: |
    Based on the collected information, determine:
    1. Why is the pod crash looping?
    2. What is the root cause?
    3. What are the recommended fixes?
```

### 多 LLM 后端配置

```yaml
# holmes-config.yaml
llm:
  # OpenAI
  provider: openai
  model: gpt-4o
  api_key: ${OPENAI_API_KEY}

  # Azure OpenAI
  # provider: azure
  # deployment: gpt-4
  # endpoint: https://myendpoint.openai.azure.com/
  # api_key: ${AZURE_OPENAI_KEY}

  # AWS Bedrock
  # provider: bedrock
  # model: anthropic.claude-3-sonnet-20240229-v1:0
  # region: us-east-1
```

### 与 Robusta 集成

```yaml
# Robusta values.yaml 中启用 Holmes
globalConfig:
  ai_analysis: true
  holmes_enabled: true

sinksConfig:
  - slack_sink:
      name: main_slack_sink
      slack_channel: alerts
      api_key: xoxb-...
      # Holmes 会自动为每个告警添加 AI 分析
```

---

## 与其他方案对比

| 特性 | HolmesGPT | K8sGPT | kubectl-ai |
|:---|:---|:---|:---|
| 告警分析 | 自动化根因分析 | 集群扫描 | 命令生成 |
| 工具调用 | 自主执行诊断命令 | 静态分析 | 单次命令 |
| Runbook | 支持自定义 | 不支持 | 不支持 |
| 告警集成 | Prometheus/PD/OG | 有限 | 无 |
| 交互模式 | CLI/Slack/Teams | CLI | CLI |
| LLM 后端 | 多种 | 多种 | OpenAI |

---

## 最佳实践

1. **RBAC 最小权限**: 生产环境中限制 Holmes 的 ServiceAccount 权限，仅授予只读权限
2. **Runbook 积累**: 将团队运维经验编写为 Runbook，提高排查准确率
3. **LLM 选择**: 复杂排查场景使用 GPT-4 级别模型，简单场景可用轻量模型降低成本
4. **敏感数据**: 注意 LLM 调用会将集群信息发送到外部 API，确保不泄露敏感数据
5. **Slack 集成**: 将 Holmes 分析结果推送到告警频道，加速 On-Call 响应

---

## 参考资源

- [HolmesGPT GitHub](https://github.com/robusta-dev/holmesgpt)
- [Robusta AI Analysis 文档](https://docs.robusta.dev/master/configuration/ai-analysis.html)
- [HolmesGPT 使用指南](https://docs.robusta.dev/master/configuration/holmesgpt.html)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
