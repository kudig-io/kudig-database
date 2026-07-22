---
title: K8sGPT (entities)
description: '## 概述'
summary: 'K8sGPT 是一款 AI 驱动的 Kubernetes 诊断工具，利用大语言模型 (LLM) 自动分析集群问题并提供人类可读的解释和建议。它扫描 Kubernetes 集群中的问题，结合 AI 能力生成诊断报告，帮助 SRE 和开发者快速定位和解决问题。'
category: entities
tags:
- k8s
- cncf
- platform
- k8sgpt
- prometheus
- grafana
- job
- cronjob
- ingress
- networkpolicy
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8sGPT 是什么
- 如何 K8sGPT
trigger_keywords:
- K8sGPT
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8sGPT

> **CNCF 状态**: Sandbox | **类别**: Platform | **主要语言**: Go

## 概述

K8sGPT 是一款 AI 驱动的 Kubernetes 诊断工具，由 Alex Jones 创建，2023 年加入 CNCF Sandbox。它利用大语言模型（LLM）自动分析集群问题并提供人类可读的解释和建议。K8sGPT 扫描 Kubernetes 集群中的问题，结合 AI 能力生成诊断报告，帮助 SRE 和开发者快速定位和解决问题。K8sGPT 将 SRE 知识编码为分析器（Analyzer），覆盖 Pod、Service、Ingress、Deployment 等核心资源。

## 核心特性

- **多 LLM 后端**: 支持 OpenAI、Azure OpenAI、Amazon Bedrock、LocalAI、Ollama 等
- **内置分析器**: 覆盖 Pod 崩溃、Service 配置、Ingress 后端、网络策略等 30+ 场景
- **AI 解释**: 使用 LLM 将技术错误转化为人类可读的因果分析和修复建议
- **Operator 模式**: 作为 K8s Operator 持续监控集群健康状态
- **多语言**: 支持中文、英文、日文等多种语言输出
- **自定义分析器**: 通过插件架构扩展内置分析能力

## 架构

K8sGPT 分为 CLI 和 Operator 两种运行模式。CLI 模式下，它通过 Kubernetes API 收集资源状态，经分析器（Analyzer）识别问题模式，再将问题发送给 LLM 后端生成自然语言解释。Operator 模式下，K8sGPT 以 Deployment 彐式运行在集群内，通过 CRD（K8sGPT / Result）管理扫描任务和结果。结果对象包含问题描述、严重级别和建议操作，可集成到告警系统。

## Kubernetes 集成

K8sGPT 通过 Kubernetes API Server 读取集群资源状态（Pod、Service、Ingress、Node 等），无需在节点上部署代理。Operator 模式通过 CRD 声明式管理扫描配置和结果，支持与 Prometheus AlertManager 集成实现自动化告警。CLI 模式使用 kubeconfig 连接集群，可作为 kubectl 插件使用。扫描结果以 Kubernetes 自定义资源（Result CRD）存储，便于 GitOps 管理。

## 生产使用场景

1. **日常巡检**: 配合 CronJob 定期运行 K8sGPT 扫描，生成集群健康报告
2. **故障诊断**: 快速分析 Pod CrashLoopBackOff、Service 无端点等问题
3. **告警增强**: 将 K8sGPT 结果集成到 AlertManager，在告警中附加 AI 解释
4. **新人培训**: 利用 AI 生成的解释帮助新 SRE 理解集群问题

## 安装与配置

```bash
# 安装 CLI
brew install k8sgpt
# 或
curl -fsSL https://get.k8sgpt.ai | bash

# 初始化（配置 AI 后端）
k8sgpt auth add --backend openai --model gpt-4
# 或使用本地 LLM (Ollama)
k8sgpt auth add --backend localai --model llama2 --endpoint http://localhost:8080

# 扫描集群
k8sgpt analyze
k8sgpt analyze --explain  # 带 AI 解释
k8sgpt analyze --filter=Pod,Service  # 过滤资源类型
k8sgpt analyze --namespace=production
k8sgpt analyze --output json  # JSON 输出

# Operator 模式
helm repo add k8sgpt https://charts.k8sgpt.ai/
helm install k8sgpt k8sgpt/k8sgpt-operator \
  -n k8sgpt-system --create-namespace \
  --set backend=openai \
  --set model=gpt-4

# 验证安装
kubectl get pods -n k8sgpt-system
kubectl get k8sgpt -A
```

```yaml
# K8sGPT CRD 示例 (Operator 模式)
apiVersion: core.k8sgpt.ai/v1alpha1
kind: K8sGPT
metadata:
  name: k8sgpt-sample
  namespace: k8sgpt-system
spec:
  backend:
    backend: openai
    model: gpt-4
    baseUrl: ""  # 自定义 API 端点
  filters:
    - Pod
    - Service
    - Ingress
    - Deployment
    - StatefulSet
  sink:
    type: slack
    webhook: "https://hooks.slack.com/services/xxx"
  extraOptions:
    backstage:
      enabled: false
---
# Result CRD (自动生成)
apiVersion: core.k8sgpt.ai/v1alpha1
kind: Result
metadata:
  name: pod-crashloop-abc123
  namespace: production
spec:
  kind: Pod
  name: payment-service-xyz
  error:
    - text: "Pod is in CrashLoopBackOff state"
      sensitive:
        - unmasked: "payment-service-xyz"
  details: "Container 'payment' is crashing repeatedly..."
  parentObject: "Deployment/payment-service"
```

## 运维操作

```bash
# 🟢 运行集群扫描
k8sgpt analyze
k8sgpt analyze --explain --language=Chinese

# 🟢 查看特定资源问题
k8sgpt analyze --filter=Pod --namespace=production
k8sgpt analyze --filter=Service,Ingress

# 🟢 查看 Operator 状态
kubectl get pods -n k8sgpt-system
kubectl get k8sgpt -A
kubectl get results -A

# 🟢 查看扫描结果
kubectl get results -n production -o yaml
kubectl get results -A --field-selector spec.kind=Pod

# 🟢 检查 AI 后端连接
k8sgpt auth list
k8sgpt auth test

# 🟡 触发重新扫描 (Operator)
kubectl delete results -A --all  # 清除旧结果，触发重新扫描

# 🟢 导出报告
k8sgpt analyze --output json > cluster-report.json
k8sgpt analyze --output json | jq '.results[] | select(.severity=="critical")'
```

## 内置分析器

| 分析器 | 检测问题 | 严重级别 |
|--------|----------|----------|
| Pod | CrashLoopBackOff, ImagePullBackOff, OOMKilled | High/Critical |
| Service | 无 Endpoint, 端口不匹配 | Medium |
| Ingress | 后端不存在, TLS 配置错误 | Medium |
| Deployment | 副本不足, 滚动更新卡住 | High |
| StatefulSet | PVC 未绑定, Pod 未就绪 | High |
| Node | NotReady, 资源压力 | Critical |
| NetworkPolicy | 策略冲突, 无匹配 Pod | Low |
| HPA | 指标不可用, 达到上限 | Medium |
| PVC | 未绑定, 容量不足 | High |
| Log | 错误日志模式识别 | Medium |

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| AI 解释不可用 | LLM API 连接失败 | `k8sgpt auth test` | 检查 API Key/网络 |
| 扫描无结果 | 集群无问题/过滤器错误 | `k8sgpt analyze --filter=Pod` | 调整过滤器 |
| Operator 未生成 Result | CRD 未安装 | `kubectl get crd \| grep k8sgpt` | 重新安装 Operator |
| 扫描超时 | 集群资源过多 | 检查 API Server 负载 | 缩小扫描范围 |
| 解释质量低 | 模型能力不足 | 尝试更强模型 | 使用 gpt-4 或本地大模型 |
| 敏感信息泄露 | 未配置脱敏 | 检查 Result 内容 | 配置 sensitive 字段脱敏 |

### 排查流程

```
K8sGPT 异常
├── CLI 扫描失败
│   ├── k8sgpt auth test → 检查 AI 后端连接
│   ├── kubectl cluster-info → 检查集群连接
│   ├── 检查 kubeconfig 权限
│   └── 检查网络连接 (LLM API)
├── Operator 模式异常
│   ├── kubectl get pods -n k8sgpt-system → Pod 状态
│   ├── kubectl logs → 检查错误日志
│   ├── kubectl get k8sgpt → 检查 CR 状态
│   └── 检查 RBAC 权限
└── 结果质量问题
    ├── 尝试不同 LLM 模型
    ├── 调整过滤器缩小范围
    └── 结合人工判断使用
```

## 生产案例

### 案例 1: 日常巡检自动化

- **场景**: SRE 团队每天手动检查集群状态，耗时 30 分钟
- **排查**: 手动检查容易遗漏；新人不熟悉常见问题模式
- **方案**: 部署 K8sGPT Operator 持续扫描；CronJob 每天生成报告；Slack 推送关键问题
- **效果**: 巡检时间从 30 分钟降至 5 分钟；问题发现率提升 60%

### 案例 2: 告警增强与根因分析

- **场景**: Prometheus 告警只告知“什么坏了”，不告知“为什么”
- **排查**: 告警风暴时 SRE 难以快速定位根因
- **方案**: K8sGPT 结果集成到 AlertManager；告警中附加 AI 生成的根因分析和修复建议
- **效果**: MTTR 降低 40%；新人上手时间缩短

## 对比与替代方案

| 维度 | K8sGPT | HolmesGPT | Robusta | Prometheus Alerts |
|------|--------|-----------|---------|-------------------|
| AI 驱动 | ✅ | ✅ | 部分 | ❌ |
| 根因分析 | ✅ | ✅ | ✅ | ❌ |
| 自然语言解释 | ✅ | ✅ | 部分 | ❌ |
| 多 LLM 支持 | ✅ | 部分 | ❌ | ❌ |
| K8s Operator | ✅ | ❌ | ✅ | N/A |
| 开源 | ✅ | ✅ | ✅ | ✅ |
| 适用场景 | 智能诊断 | 告警分析 | 全面可观测 | 指标告警 |

## 检查清单

- [ ] K8sGPT CLI/Operator 已安装
- [ ] AI 后端已配置并连接正常
- [ ] 过滤器已配置 (避免扫描过多资源)
- [ ] 敏感信息脱敏已配置
- [ ] 扫描结果已接入告警系统
- [ ] 定期扫描已配置 (CronJob/Operator)
- [ ] LLM API 成本已监控
- [ ] 结果已验证 (AI 建议需人工确认)

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[实体/trivy.md|trivy]]
- [[实体/networkpolicy.md|networkpolicy]]
- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]

## Related

- [[spire]] — SPIRE
- [[akri]] — Akri
- [[实体/cncf-edge-ai.md|cncf-edge-ai]] — CNCF 边缘计算与 AI/ML 项目全景
- [[confidential-containers]] — Confidential Containers (CoCo)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

<!-- risk-assessed -->
