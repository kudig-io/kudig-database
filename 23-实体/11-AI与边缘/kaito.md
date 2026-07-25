---
title: KAITO
description: '## 概述'
summary: 'KAITO 是一个 Kubernetes Operator，简化在 Kubernetes 集群上运行 AI/ML 推理和微调工作负载的流程。'
category: entities
tags:
- k8s
- cncf
- ai-ml
- kaito
- falco
- crd
- operator
- gpu
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KAITO 是什么
- 如何 KAITO
trigger_keywords:
- KAITO
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中盐险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# KAITO

> **CNCF 状态**: Sandbox | **类别**: AI/ML | **主要语言**: Go, Python

## 概述

KAITO（Kubernetes AI Toolchain Operator）是微软开源的 Kubernetes Operator，2023 年加入 CNCF 沙箱。它简化在 Kubernetes 集群上运行 AI/ML 推理和微调工作负载的流程，自动化了 GPU 节点的配置、模型下载和推理服务部署。开发者只需指定模型名称（如 `falcon-7b`、`llama-2-7b`），KAITO 自动完成：节点 GPU 资源调度、模型权重下载、推理引擎（vLLM/TGI）部署和 Service 暴露。与传统手动部署相比，KAITO 将 AI 模型部署时间从数小时缩短到分钟级。它还内置了预设模型预设（Preset），支持 Falcon、LLaMA、Mistral、Phi 等主流开源大模型。

## 核心能力

- **预设模型（Preset）**: 内置主流开源大模型配置（Falcon、LLaMA、Mistral、Phi 等）
- **GPU 自动调度**: 根据模型需求自动申请和配置 GPU 节点
- **模型自动下载**: 从 Hugging Face 自动下载模型权重
- **推理引擎集成**: 支持 vLLM、TGI、ONNX Runtime 等推理后端
- **分布式推理**: 支持多 GPU 张量并行（Tensor Parallelism）推理
- **Karpenter 集成**: 配合 Karpenter 实现 GPU 节点自动扩缩容

## 架构

KAITO 基于 Kubernetes Operator 模式构建：

- **Workspace CRD**: 核心自定义资源，声明模型名称、推理参数、GPU 需求
- **KAITO Controller**: 监听 Workspace CRD，执行调谐逻辑
- **Preset Templates**: 预置的模型部署模板，包含推理引擎配置和资源要求
- **Node Provisioner**: 与 Karpenter/Cluster Autoscaler 交互，自动申请 GPU 节点
- **Model Downloader**: Init Container 从 Hugging Face 下载模型权重
- **Inference Engine**: 推理 Pod 运行 vLLM/TGI，暴露 gRPC/HTTP API

部署流程：`Workspace CRD → Controller → GPU 节点申请 → 模型下载 → 推理 Pod → Service`

## K8s 集成

KAITO 通过 Workspace CRD 与 Kubernetes 深度集成。用户创建 Workspace 资源指定模型名称和 GPU 数量，KAITO Controller 解析预设配置，自动创建 Deployment、Service 和 ConfigMap。GPU 资源通过 Kubernetes Device Plugin（NVIDIA GPU Operator）分配。与 Karpenter 集成时，KAITO 通过 nodeSelector/taint 标识 GPU 需求，Karpenter 自动拉起 GPU 节点。推理服务通过 Kubernetes Service（LoadBalancer/ClusterIP）暴露，与 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的标准网络和服务发现机制完全兼容。

## 生产场景

1. **LLM 推理服务**: 在 Kubernetes 上快速部署 LLaMA/Falcon 等大语言模型推理 API
2. **GPU 弹性推理**: 配合 Karpenter 实现 GPU 节点按需扩缩容，降低闲置成本
3. **模型 A/B 测试**: 同时部署多个模型版本，通过 Ingress 进行流量切分
4. **边缘 AI 部署**: 在边缘集群部署量化后的轻量模型（如 Phi-3）

## 安装与配置

### Operator 部署

```bash
# 安装 KAITO Operator
helm repo add kaito https://azure.github.io/kaito/
helm install kaito kaito/kaito --namespace kaito-system --create-namespace

# 确保已安装 NVIDIA GPU Operator
helm install gpu-operator nvidia/gpu-operator -n gpu-operator --create-namespace

# 验证部署
kubectl get pods -n kaito-system
kubectl get crd | grep kaito
```

### 模型部署

```yaml
# 部署 Falcon-7B 推理服务
apiVersion: kaito.sh/v1alpha1
kind: Workspace
metadata:
  name: falcon-7b
spec:
  preset:
    name: presets.falcon-7b-instruct
  resource:
    instanceType: "Standard_NC12s_v3"
    labelSelector:
      matchLabels:
        node.kubernetes.io/instance-type: Standard_NC12s_v3
---
# 部署 Phi-3 轻量模型
apiVersion: kaito.sh/v1alpha1
kind: Workspace
metadata:
  name: phi-3-mini
spec:
  preset:
    name: presets.phi-3-mini-4k-instruct
  resource:
    instanceType: "Standard_NC6s_v3"
    count: 1
    labelSelector:
      matchLabels:
        gpu: "true"
```

```bash
# 查看部署状态
kubectl get workspace -w
kubectl describe workspace falcon-7b

# 测试推理服务
kubectl port-forward svc/falcon-7b 8080:80
curl http://localhost:8080/v1/completions -d '{"prompt":"Hello"}'
```

## 运维操作

```bash
# 🟢 查看 Workspace 状态
kubectl get workspaces -A
kubectl describe workspace falcon-7b

# 🟢 查看 GPU 使用情况
kubectl top pods -n default --containers | grep gpu
nvidia-smi  # 节点上

# 🟡 扩容推理副本
kubectl patch workspace falcon-7b --type merge -p '{"spec":{"resource":{"count":2}}}'

# 🟡 更新模型版本
kubectl apply -f workspace-v2.yaml

# 🔴 删除 Workspace（释放 GPU）
kubectl delete workspace falcon-7b
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Workspace Pending | GPU 节点不足 | `kubectl describe workspace <name>` | 增加 GPU 节点或调整 instanceType |
| 模型下载失败 | 网络/存储问题 | `kubectl logs -l app=kaito` | 检查网络和 PVC 配置 |
| 推理 OOM | GPU 内存不足 | `nvidia-smi` | 使用更小模型或增加 GPU |
| 服务无响应 | Pod 未就绪 | `kubectl get pods -l workspace=<name>` | 检查 Pod 状态和日志 |
| GPU Operator 异常 | 驱动不兼容 | `kubectl get pods -n gpu-operator` | 检查驱动版本和内核兼容性 |

**排查流程：**
```
模型部署失败
├── 检查 Workspace 状态 → kubectl describe workspace <name>
├── 检查 GPU 节点 → kubectl get nodes -l gpu=true
├── 检查 GPU Operator → kubectl get pods -n gpu-operator
├── 检查 PVC → kubectl get pvc -l workspace=<name>
└── 检查 Pod 日志 → kubectl logs -l workspace=<name>
```

## 生产案例

### 案例一：企业 LLM 服务化

- **场景**: 企业需要在私有集群部署大语言模型，提供内部 AI 服务
- **排查**: 手动部署复杂，GPU 调度困难，模型版本管理混乱
- **方案**: KAITO 一键部署预置模型，自动 GPU 调度，Workspace CRD 管理生命周期
- **效果**: 模型部署时间从 2 天降至 10 分钟，GPU 利用率提升 40%

### 案例二：边缘 AI 推理

- **场景**: 边缘集群部署轻量模型（Phi-3），资源受限
- **排查**: 使用量化模型减少 GPU 内存需求
- **方案**: KAITO 部署 Phi-3-mini 量化版，单 GPU 即可运行
- **效果**: 边缘推理延迟 < 100ms，单 GPU 支持 10+ 并发请求

## 对比

| 特性 | KAITO | KServe | Seldon Core | Ray Serve | 适用场景 |
|------|-------|--------|-------------|-----------|----------|
| 模型预设 | ✅ 预置大模型 | ❌ 需自定义 | ❌ 需自定义 | ❌ | KAITO 最简 |
| GPU 自动调度 | ✅ | ⚠️ 需 Knative | ⚠️ 需手动 | ⚠️ | - |
| 大模型原生支持 | ✅ | ⚠️ | ⚠️ | ⚠️ | - |
| 部署复杂度 | 低 | 中 | 高 | 高 | - |
| 多模型管理 | ✅ | ✅ | ✅ | ✅ | - |

## 架构定位

在 CNCF 生态中，KAITO 属于 **AI/ML** 类别，为云原生应用提供大模型推理自动化部署能力。

## 参考链接

- [[falco]]
- [[23-实体/02-K8s核心组件/crd-custom-resources.md|crd-custom-resources]]
- [[operator-pattern]]
- [[22-概念/04-存储/storage-model.md|storage-model]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]

## Related

- [[kcl]] — KCL (Kusion Configuration Language)
- [[kube-vip]] — kube-vip
- [[kitops]] — KitOps
- [[kairos]] — Kairos
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- kaito
- [[23-实体/cncf-edge-ai.md|[[CNCF 边缘计算与 AI/ML 项目全景|CNCF 边缘计算与 AI/ML 项目全景]]]] — Cross-reference
- [[21-生态参考/03-领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
