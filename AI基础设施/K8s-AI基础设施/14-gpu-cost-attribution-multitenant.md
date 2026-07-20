---
title: "GPU 成本分摊与多租户 AI 平台"
description: "GPU 成本归因、多租户隔离与计费模型：OpenCost/Kubecost GPU 归因、ResourceQuota、NetworkPolicy、showback/chargeback 设计"
summary: "企业级 GPU 成本治理完整方案：GPU 成本模型（按卡/按实例/按 token）、OpenCost GPU 归因配置、Kubecost 多租户报表、Namespace 级 ResourceQuota 与 LimitRange、多租户隔离（NetworkPolicy + RBAC）、计费模型设计（showback vs chargeback）、GPU idle detection 与 auto-suspend"
category: AI基础设施
tags:
- gpu
- cost
- multi-tenant
- opencost
- kubecost
- resourcequota
- finops
- chargeback
- isolation
- rbac
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- MLOps 工程师
- SRE
estimated_read_time: 20min
intent_queries:
- "GPU 成本如何分摊到各团队"
- "多租户 AI 平台如何做资源隔离"
- "OpenCost 如何归因 GPU 成本"
trigger_keywords:
- GPU成本
- 多租户
- 成本分摊
- OpenCost
- ResourceQuota
prerequisites:
- kubectl-basics
- helm-basics
- rbac-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# GPU 成本分摊与多租户 AI 平台

## 概述

GPU 是 AI 基础设施中成本最高的单一资源。一张 NVIDIA A100 80GB 的云端按需价格约为 $3-4/小时，H100 更是高达 $5-8/小时。当多个团队（算法研究、模型训练、推理服务、数据工程）共享 GPU 集群时，如何准确归因成本、公平分配资源、激励高效使用，是 AI 平台治理的核心挑战。

本文覆盖从 GPU 成本建模、OpenCost/Kubecost 归因配置、Namespace 级配额管理、多租户安全隔离、到计费模型设计的完整方案，帮助企业构建可度量、可治理、可优化的多租户 AI 平台。

## 架构与核心概念

### GPU 成本模型

GPU 成本归因存在三种粒度模型，适用于不同管理成熟度阶段：

| 成本模型 | 归因粒度 | 适用场景 | 优点 | 缺点 |
|---------|---------|---------|------|------|
| 按卡计费 | 整张 GPU 卡 × 时间 | 独占 GPU 的训练任务 | 简单直观 | 无法反映共享场景 |
| 按实例计费 | GPU 分片（MIG/vGPU）× 时间 | MIG 切分的推理服务 | 精确到分片 | 需要 MIG 支持 |
| 按 Token 计费 | 输入/输出 token 数 | 推理服务 API 调用 | 贴近业务价值 | 归因链路复杂 |

### 多租户架构分层

```
┌─────────────────────────────────────────────────────────────┐
│                     平台管理层                                │
│  ┌───────────┐  ┌──────────────┐  ┌─────────────────────┐  │
│  │ 成本报表   │  │ 配额管理      │  │ 审批工作流           │  │
│  │ (Kubecost)│  │(ResourceQuota)│  │ (OPA/Gatekeeper)   │  │
│  └───────────┘  └──────────────┘  └─────────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                     租户隔离层                                │
│  ┌───────────┐  ┌──────────────┐  ┌─────────────────────┐  │
│  │ Namespace │  │ NetworkPolicy│  │ RBAC                │  │
│  │ 隔离      │  │ 网络隔离      │  │ 权限隔离             │  │
│  └───────────┘  └──────────────┘  └─────────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                     资源层                                    │
│  ┌───────────┐  ┌──────────────┐  ┌─────────────────────┐  │
│  │ GPU Pool  │  │ MIG 分片      │  │ 存储配额             │  │
│  │ (A100/H100)│ │ (1g.5gb等)   │  │ (PVC LimitRange)   │  │
│  └───────────┘  └──────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## 生产部署

### OpenCost GPU 归因配置

OpenCost 是 CNCF 沙箱项目，支持将 GPU 成本按实际使用量归因到 Namespace/Pod/Label 维度。

🟡 **中风险** — 部署 OpenCost 并配置 GPU 价格：

```bash
# 部署 OpenCost（含 GPU 支持）
helm repo add opencost https://opencost.github.io/opencost-helm-chart
helm repo update

helm install opencost opencost/opencost \
  --namespace opencost \
  --create-namespace \
  --version 1.108.0 \
  --set opencost.exporter.defaultClusterId=ai-cluster-prod \
  --set opencost.prometheus.internal.enabled=true \
  --set opencost.prometheus.internal.serviceName=prometheus-server \
  --set opencost.prometheus.internal.namespaceName=monitoring \
  --set opencost.exporter.cloudProviderApiKey="" \
  --set opencost.ui.enabled=true
```

🟡 **中风险** — 配置 GPU 自定义价格（ConfigMap）：

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: opencost-pricing
  namespace: opencost
data:
  pricing.json: |
    {
      "provider": "custom",
      "description": "AI Cluster GPU Pricing",
      "GPU": {
        "nvidia-a100-80gb": {
          "hourly": "3.67",
          "currency": "USD"
        },
        "nvidia-h100-80gb": {
          "hourly": "6.98",
          "currency": "USD"
        },
        "nvidia-a10g": {
          "hourly": "1.52",
          "currency": "USD"
        },
        "nvidia-l4": {
          "hourly": "1.15",
          "currency": "USD"
        }
      },
      "CPU": {
        "hourly": "0.031611"
      },
      "RAM": {
        "GB": "0.004237"
      },
      "storage": {
        "GB": "0.0000547945"
      }
    }
```

### Namespace 级 GPU 配额

🟡 **中风险** — 为团队 Namespace 设置 GPU ResourceQuota 与 LimitRange：

```yaml
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-nlp-gpu-quota
  namespace: team-nlp
spec:
  hard:
    # GPU 配额
    requests.nvidia.com/gpu: "8"
    limits.nvidia.com/gpu: "8"
    # MIG 分片配额
    requests.nvidia.com/mig-1g.5gb: "4"
    requests.nvidia.com/mig-2g.10gb: "2"
    # CPU/Memory 配额
    requests.cpu: "64"
    requests.memory: "256Gi"
    limits.cpu: "128"
    limits.memory: "512Gi"
    # 存储配额
    requests.storage: "2Ti"
    persistentvolumeclaims: "20"
    # Pod 数量限制
    pods: "50"
    services: "10"
---
apiVersion: v1
kind: LimitRange
metadata:
  name: team-nlp-limits
  namespace: team-nlp
spec:
  limits:
  - type: Container
    default:
      cpu: "8"
      memory: "32Gi"
    defaultRequest:
      cpu: "4"
      memory: "16Gi"
    max:
      nvidia.com/gpu: "4"      # 单容器最多 4 卡
      cpu: "64"
      memory: "256Gi"
    min:
      cpu: "500m"
      memory: "1Gi"
  - type: Pod
    max:
      nvidia.com/gpu: "8"      # 单 Pod 最多 8 卡
```

### 多租户网络隔离

🟡 **中风险** — 配置租户间 NetworkPolicy 隔离：

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: team-nlp-isolation
  namespace: team-nlp
spec:
  podSelector: {}              # 应用于 Namespace 内所有 Pod
  policyTypes:
  - Ingress
  - Egress
  ingress:
  # 允许同 Namespace 内通信
  - from:
    - podSelector: {}
  # 允许平台管理服务访问（监控、日志）
  - from:
    - namespaceSelector:
        matchLabels:
          role: platform-services
    - namespaceSelector:
        matchLabels:
          role: monitoring
  # 允许推理服务被 API Gateway 访问
  - from:
    - namespaceSelector:
        matchLabels:
          role: ingress
      podSelector:
        matchLabels:
          app: api-gateway
  egress:
  # 允许同 Namespace 内通信
  - to:
    - podSelector: {}
  # 允许 DNS 解析
  - to:
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
  # 允许访问模型仓库（内部 Registry）
  - to:
    - namespaceSelector:
        matchLabels:
          role: model-registry
  # 允许访问对象存储（训练数据）
  - to:
    - ipBlock:
        cidr: 10.0.0.0/8
    ports:
    - protocol: TCP
      port: 443
    - protocol: TCP
      port: 9000
```

### 多租户 RBAC 配置

🟡 **中风险** — 为团队创建受限的 Role 与 RoleBinding：

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: team-nlp-member
  namespace: team-nlp
rules:
# 允许管理自己的工作负载
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets", "replicasets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["pods", "pods/log", "pods/exec"]
  verbs: ["get", "list", "watch", "create", "delete"]
- apiGroups: [""]
  resources: ["services", "configmaps", "secrets"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["persistentvolumeclaims"]
  verbs: ["get", "list", "watch", "create", "delete"]
# 禁止修改配额和 LimitRange
- apiGroups: [""]
  resources: ["resourcequotas", "limitranges"]
  verbs: ["get", "list", "watch"]
# 允许查看 GPU 使用情况
- apiGroups: ["metrics.k8s.io"]
  resources: ["pods", "nodes"]
  verbs: ["get", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-nlp-members-binding
  namespace: team-nlp
subjects:
- kind: Group
  name: "team-nlp@company.com"
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: team-nlp-member
  apiGroup: rbac.authorization.k8s.io
```

## 运维操作

### 查看 GPU 成本归因报表

🟢 **只读** — 通过 OpenCost API 查询各团队 GPU 成本：

```bash
# 查询过去 7 天各 Namespace 的 GPU 成本
kubectl port-forward -n opencost svc/opencost 9003:9003 &

curl -s 'http://localhost:9003/allocation/compute?window=7d&aggregate=namespace&filterNamespaces:team-' | \
  jq '.data[0] | to_entries[] | {namespace: .key, gpuCost: .value.gpuCost, totalCost: .value.totalCost}'

# 查询特定团队的 GPU 使用明细
curl -s 'http://localhost:9003/allocation/compute?window=30d&aggregate=controller&filterNamespaces:team-nlp' | \
  jq '.data[0] | to_entries[] | select(.value.gpuCost > 0) | {name: .key, gpuHours: .value.gpuHours, gpuCost: .value.gpuCost}'

# 查看集群 GPU 总览
kubectl get nodes -l nvidia.com/gpu.present=true -o custom-columns=\
NAME:.metadata.name,\
GPU_TYPE:.metadata.labels.nvidia\\.com/gpu\\.product,\
GPU_COUNT:.status.allocatable.nvidia\\.com/gpu
```

### GPU 利用率审计

🟢 **只读** — 检测 idle GPU 资源：

```bash
# 查看各 Namespace GPU 请求 vs 实际利用率
for ns in $(kubectl get ns -l team -o jsonpath='{.items[*].metadata.name}'); do
  echo "=== Namespace: $ns ==="
  echo "GPU Requests:"
  kubectl get pods -n $ns -o json | \
    jq '[.items[].spec.containers[].resources.requests["nvidia.com/gpu"] // "0" | tonumber] | add // 0'
  echo "GPU Utilization (avg 1h):"
  curl -s "http://prometheus-server.monitoring.svc:9090/api/v1/query?query=avg(DCGM_FI_DEV_GPU_UTIL{namespace=\"$ns\"})" | \
    jq '.data.result[0].value[1] // "N/A"'
  echo ""
done

# 检测持续 idle 的 GPU Pod（利用率 < 5% 超过 2 小时）
curl -s 'http://prometheus-server.monitoring.svc:9090/api/v1/query?query=avg_over_time(DCGM_FI_DEV_GPU_UTIL[2h]) < 5' | \
  jq '.data.result[] | {pod: .metric.pod, namespace: .metric.namespace, util: .value[1]}'
```

### 配额超限处理

🟡 **中风险** — 临时调整团队配额（需审批）：

```bash
# 查看当前配额使用情况
kubectl describe resourcequota team-nlp-gpu-quota -n team-nlp

# 临时增加 GPU 配额（需平台管理员权限）
kubectl patch resourcequota team-nlp-gpu-quota -n team-nlp \
  --type merge -p '{"spec":{"hard":{"requests.nvidia.com/gpu":"12","limits.nvidia.com/gpu":"12"}}}'

# 回滚配额
kubectl patch resourcequota team-nlp-gpu-quota -n team-nlp \
  --type merge -p '{"spec":{"hard":{"requests.nvidia.com/gpu":"8","limits.nvidia.com/gpu":"8"}}}'
```

## 故障排查

### 配额超限导致 Pod Pending

**现象**：用户提交训练任务后 Pod 一直处于 Pending 状态，Events 显示 `exceeded quota`。

**排查步骤**：

```bash
# 🟢 查看配额使用详情
kubectl describe resourcequota -n team-nlp

# 🟢 查看 Pod 事件
kubectl describe pod training-job-xxx -n team-nlp | grep -A 5 "Events"

# 🟢 查看 Namespace 内所有 GPU 请求
kubectl get pods -n team-nlp -o json | \
  jq '[.items[] | select(.status.phase != "Succeeded" and .status.phase != "Failed") | {name: .metadata.name, gpu: [.spec.containers[].resources.requests["nvidia.com/gpu"] // "0" | tonumber] | add}]'
```

**修复方案**：
1. 清理已完成但未删除的 Job Pod（`kubectl delete pods --field-selector=status.phase==Succeeded -n team-nlp`）
2. 协调团队释放闲置资源
3. 走审批流程临时提升配额

### 成本归因不准确

**现象**：OpenCost 报表中 GPU 成本为 0 或与实际不符。

**排查步骤**：

```bash
# 🟢 检查 OpenCost 是否正确识别 GPU 节点
kubectl logs -n opencost -l app=opencost --tail=100 | grep -i "gpu\|nvidia"

# 🟢 验证 Prometheus 中 GPU 指标是否存在
curl -s 'http://prometheus-server.monitoring.svc:9090/api/v1/query?query=kube_pod_container_resource_requests{resource="nvidia_com_gpu"}' | \
  jq '.data.result | length'

# 🟢 检查节点 GPU 标签
kubectl get nodes -o json | jq '.items[] | select(.status.allocatable["nvidia.com/gpu"] != null) | {name: .metadata.name, gpu: .status.allocatable["nvidia.com/gpu"], product: .metadata.labels["nvidia.com/gpu.product"]}'
```

**修复方案**：确保 DCGM Exporter 和 NVIDIA Device Plugin 正确部署（参考 [[AI基础设施/基础设施/04-gpu-monitoring-dcgm.md|GPU 监控 DCGM]]）；验证 OpenCost 的 pricing ConfigMap 中 GPU 型号名称与节点标签匹配。

## 最佳实践

### 计费模型设计

| 维度 | Showback（展示） | Chargeback（计费） |
|------|-----------------|-------------------|
| 定义 | 展示各团队资源消耗，不实际扣费 | 按使用量实际从团队预算扣除 |
| 适用阶段 | 平台初期（< 6 个月） | 平台成熟期 |
| 实施复杂度 | 低（只需报表） | 高（需对接财务系统） |
| 用户感知 | 成本意识培养 | 强约束力 |
| 推荐策略 | 先 Showback 3 个月 → 再 Chargeback | 配合预算预警 |

### GPU 利用率优化策略

1. **Idle Detection + Auto-Suspend**：对持续 idle 超过 2 小时的 Notebook/开发环境自动 suspend
2. **时间切片共享**：开发环境使用 GPU Time-Slicing（参考 [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]]），多个轻量任务共享一张卡
3. **MIG 分片**：推理服务使用 MIG 将 A100 切分为多个独立实例
4. **Spot/Preemptible 实例**：非关键训练任务使用抢占式实例降低成本 60-80%
5. **分时段调度**：利用 CronJob 在低峰期运行 batch 推理任务

### 多租户治理清单

- 每个团队独立 Namespace，命名规范 `team-{name}`
- 所有 Namespace 必须配置 ResourceQuota（GPU + CPU + Memory + Storage）
- NetworkPolicy 默认 deny-all，按需开放
- RBAC 最小权限原则，禁止团队修改配额
- 标签规范：所有资源必须携带 `team`、`project`、`cost-center` 标签
- 月度成本报表自动发送到团队负责人
- 设置 GPU 利用率 < 20% 持续 4 小时的告警

## Related

- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]]
- [[AI基础设施/基础设施/12-ai-cost-analysis-finops.md|AI 成本分析 FinOps]]
- [[AI基础设施/基础设施/04-gpu-monitoring-dcgm.md|GPU 监控 DCGM]]
- [[AI基础设施/K8s-AI基础设施/13-model-serving-autoscaling-keda.md|推理服务自动伸缩]]
- [[AI基础设施/K8s-AI基础设施/17-ai-platform-architecture-reference.md|企业 AI 平台参考架构]]
