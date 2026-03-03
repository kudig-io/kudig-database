# K8sGPT

> **成熟度**: Sandbox | **加入时间**: 2023-04 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://k8sgpt.ai |
| **GitHub** | https://github.com/k8sgpt-ai/k8sgpt |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | AI/ML, Observability |
| **适用场景** | Kubernetes 问题诊断与分析 |

---

## 项目概述

K8sGPT 是一款 AI 驱动的 Kubernetes 诊断工具，利用大语言模型 (LLM) 自动分析集群问题并提供人类可读的解释和建议。它扫描 Kubernetes 集群中的问题，结合 AI 能力生成诊断报告，帮助 SRE 和开发者快速定位和解决问题。

---

## 核心特性

- **多 LLM 支持**: OpenAI、Azure OpenAI、Anthropic、LocalAI、Ollama
- **问题扫描**: 自动检测 Pod、Service、Ingress 等资源问题
- **AI 解释**: 使用 LLM 生成问题原因和解决方案
- **多语言输出**: 支持中文、英文等多种语言
- **Operator 模式**: 作为 Kubernetes Operator 持续监控
- **自定义分析器**: 扩展内置分析能力
- **集成友好**: 可与 Prometheus、Slack 等工具集成

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     K8sGPT Architecture                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    User Interface                         │   │
│  │  ┌───────────┐  ┌───────────┐  ┌───────────────────┐    │   │
│  │  │   CLI     │  │  Web UI   │  │  Kubernetes API   │    │   │
│  │  │ k8sgpt    │  │ Dashboard │  │  (K8sGPT CRD)     │    │   │
│  │  └─────┬─────┘  └─────┬─────┘  └─────────┬─────────┘    │   │
│  └────────┼──────────────┼──────────────────┼──────────────┘   │
│           │              │                  │                   │
│  ┌────────▼──────────────▼──────────────────▼──────────────┐   │
│  │                   K8sGPT Core Engine                      │   │
│  │                                                           │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │                   Analyzers                          │ │   │
│  │  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐   │ │   │
│  │  │  │   Pod   │ │ Service │ │ Ingress │ │  Event  │   │ │   │
│  │  │  │Analyzer │ │Analyzer │ │Analyzer │ │Analyzer │   │ │   │
│  │  │  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘   │ │   │
│  │  │       │           │           │           │         │ │   │
│  │  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐   │ │   │
│  │  │  │   PVC   │ │  Node   │ │ Network │ │ Custom  │   │ │   │
│  │  │  │Analyzer │ │Analyzer │ │ Policy  │ │Analyzer │   │ │   │
│  │  │  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘   │ │   │
│  │  │       └───────────┴───────────┴───────────┘         │ │   │
│  │  └──────────────────────────┬──────────────────────────┘ │   │
│  │                             │                             │   │
│  │  ┌──────────────────────────▼──────────────────────────┐ │   │
│  │  │                 AI Backend Integrations              │ │   │
│  │  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐   │ │   │
│  │  │  │ OpenAI  │ │  Azure  │ │Anthropic│ │  Local  │   │ │   │
│  │  │  │  GPT-4  │ │ OpenAI  │ │ Claude  │ │   AI    │   │ │   │
│  │  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘   │ │   │
│  │  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐   │ │   │
│  │  │  │ Ollama  │ │ Cohere  │ │ Amazon  │ │ Google  │   │ │   │
│  │  │  │         │ │         │ │Bedrock  │ │Vertex AI│   │ │   │
│  │  │  └─────────┘ └─────────┘ └─────────┘ └─────────┘   │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └───────────────────────────────────────────────────────────┘  │
│                              │                                   │
│  ┌───────────────────────────▼───────────────────────────────┐  │
│  │                   Kubernetes Cluster                       │  │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────────┐  │  │
│  │  │  Pods   │  │Services │  │ Ingress │  │  ConfigMaps │  │  │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────────┘  │  │
│  │  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────────┐  │  │
│  │  │  PVCs   │  │  Nodes  │  │ Events  │  │ NetworkPolicy│  │  │
│  │  └─────────┘  └─────────┘  └─────────┘  └─────────────┘  │  │
│  └───────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

| 组件 | 说明 |
|:---|:---|
| **CLI** | 命令行工具，本地运行诊断 |
| **Operator** | Kubernetes Operator，持续监控 |
| **Analyzers** | 内置分析器，扫描各类资源 |
| **AI Backend** | LLM 后端，生成诊断解释 |
| **Results CRD** | 存储诊断结果的自定义资源 |

---

## 快速开始

### 安装 CLI

```bash
# macOS
brew install k8sgpt

# Linux
curl -LO https://github.com/k8sgpt-ai/k8sgpt/releases/latest/download/k8sgpt_Linux_x86_64.tar.gz
tar -xzf k8sgpt_Linux_x86_64.tar.gz
sudo mv k8sgpt /usr/local/bin/

# Windows
choco install k8sgpt

# 验证安装
k8sgpt version
```

### 配置 AI 后端

```bash
# 使用 OpenAI
k8sgpt auth add --backend openai --model gpt-4

# 使用 Azure OpenAI
k8sgpt auth add --backend azureopenai \
  --baseurl https://your-resource.openai.azure.com \
  --engine gpt-4 \
  --model gpt-4

# 使用本地 Ollama
k8sgpt auth add --backend localai \
  --baseurl http://localhost:11434/v1 \
  --model llama2

# 列出已配置的后端
k8sgpt auth list

# 设置默认后端
k8sgpt auth default --backend openai
```

---

## CLI 使用

### 基本分析

```bash
# 分析所有问题
k8sgpt analyze

# 使用 AI 解释
k8sgpt analyze --explain

# 指定命名空间
k8sgpt analyze --namespace production --explain

# 指定语言输出
k8sgpt analyze --explain --language chinese

# 过滤分析器
k8sgpt analyze --filter Pod,Service --explain
```

### 分析器管理

```bash
# 列出所有分析器
k8sgpt filters list

# 启用/禁用分析器
k8sgpt filters add NetworkPolicy
k8sgpt filters remove NetworkPolicy

# 仅使用特定分析器
k8sgpt analyze --filter Pod,PersistentVolumeClaim
```

### 输出格式

```bash
# JSON 格式输出
k8sgpt analyze --explain --output json

# YAML 格式输出
k8sgpt analyze --explain --output yaml

# 保存到文件
k8sgpt analyze --explain --output json > report.json
```

---

## Operator 部署

### Helm 安装

```bash
# 添加 Helm 仓库
helm repo add k8sgpt https://charts.k8sgpt.ai/
helm repo update

# 安装 Operator
helm install k8sgpt-operator k8sgpt/k8sgpt-operator \
  --namespace k8sgpt-system \
  --create-namespace
```

### K8sGPT 资源配置

```yaml
apiVersion: core.k8sgpt.ai/v1alpha1
kind: K8sGPT
metadata:
  name: k8sgpt
  namespace: k8sgpt-system
spec:
  ai:
    enabled: true
    model: gpt-4
    backend: openai
    secret:
      name: k8sgpt-openai-secret
      key: openai-api-key
  noCache: false
  version: v0.3.24
  filters:
    - Pod
    - Service
    - Ingress
    - PersistentVolumeClaim
  extraOptions:
    backstage:
      enabled: false
    language: chinese

---
apiVersion: v1
kind: Secret
metadata:
  name: k8sgpt-openai-secret
  namespace: k8sgpt-system
type: Opaque
stringData:
  openai-api-key: "sk-..."
```

### 使用本地模型

```yaml
apiVersion: core.k8sgpt.ai/v1alpha1
kind: K8sGPT
metadata:
  name: k8sgpt-local
  namespace: k8sgpt-system
spec:
  ai:
    enabled: true
    model: llama2
    backend: localai
    baseUrl: http://ollama.default.svc:11434/v1
  filters:
    - Pod
    - Service
```

---

## 查看诊断结果

### 通过 CRD 查看

```bash
# 列出所有诊断结果
kubectl get results -n k8sgpt-system

# 查看详细结果
kubectl describe result my-result -n k8sgpt-system

# YAML 格式
kubectl get result my-result -n k8sgpt-system -o yaml
```

### Result CRD 示例

```yaml
apiVersion: core.k8sgpt.ai/v1alpha1
kind: Result
metadata:
  name: nginx-deployment-pod-error
  namespace: k8sgpt-system
spec:
  backend: openai
  details: |
    The pod nginx-deployment-xxx is in CrashLoopBackOff state.
    This is typically caused by the container exiting immediately 
    after starting.
  error:
    - text: "Back-off restarting failed container"
      sensitive: []
  kind: Pod
  name: nginx-deployment-xxx
  parentObject: Deployment/nginx-deployment
```

---

## 自定义分析器

### 分析器插件

```yaml
apiVersion: core.k8sgpt.ai/v1alpha1
kind: K8sGPT
metadata:
  name: k8sgpt-custom
  namespace: k8sgpt-system
spec:
  ai:
    enabled: true
    backend: openai
  integrations:
    trivy:
      enabled: true
      namespace: trivy-system
    prometheus:
      enabled: true
      namespace: monitoring
```

### Trivy 集成 (漏洞扫描)

```bash
# 启用 Trivy 分析器
k8sgpt integration activate trivy

# 分析包含漏洞信息
k8sgpt analyze --filter VulnerabilityReport --explain
```

---

## 集成配置

### Slack 通知

```yaml
apiVersion: core.k8sgpt.ai/v1alpha1
kind: K8sGPT
metadata:
  name: k8sgpt-slack
spec:
  ai:
    enabled: true
    backend: openai
  sink:
    type: slack
    webhook: "https://hooks.slack.com/services/xxx/yyy/zzz"
```

### Prometheus 指标

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: k8sgpt-operator
  namespace: k8sgpt-system
spec:
  selector:
    matchLabels:
      app.kubernetes.io/name: k8sgpt-operator
  endpoints:
    - port: metrics
      interval: 30s
```

---

## 内置分析器

| 分析器 | 说明 |
|:---|:---|
| **Pod** | 检测 CrashLoopBackOff、ImagePullBackOff 等 |
| **Service** | 检测无端点、选择器不匹配 |
| **Ingress** | 检测无效后端、TLS 配置 |
| **PersistentVolumeClaim** | 检测 Pending 状态、存储类 |
| **Node** | 检测节点状态、资源压力 |
| **NetworkPolicy** | 检测网络策略配置 |
| **ReplicaSet** | 检测副本数异常 |
| **StatefulSet** | 检测有状态应用问题 |
| **CronJob** | 检测任务调度问题 |
| **Deployment** | 检测部署状态问题 |

---

## 最佳实践

1. **后端选择**: 生产环境建议使用 GPT-4 或本地部署的模型
2. **缓存策略**: 启用缓存减少 API 调用成本
3. **过滤优化**: 根据需要启用特定分析器
4. **定时运行**: 配合 CronJob 定期生成诊断报告
5. **敏感信息**: 注意 API Key 安全存储
6. **成本控制**: 监控 AI API 调用量

---

## 参考资源

- [官方文档](https://docs.k8sgpt.ai)
- [GitHub Repo](https://github.com/k8sgpt-ai/k8sgpt)
- [Operator 部署](https://docs.k8sgpt.ai/getting-started/in-cluster-operator/)
- [支持的后端](https://docs.k8sgpt.ai/reference/providers/backend/)
- [分析器列表](https://docs.k8sgpt.ai/reference/analyzers/)

---

**维护者**: Kudig Team | **许可证**: MIT
