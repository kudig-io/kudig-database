---
title: "GPU 集群治理：资源配额、成本分摊与准入控制"
description: "AI 平台 GPU 集群的治理体系，涵盖资源配额、使用策略、成本分摊、准入控制及合规审计"
summary: "系统讲解 GPU 集群治理的完整框架：从资源配额设计、多租户使用策略、FinOps 成本分摊模型，到准入控制策略和合规审计机制，为 AI 平台提供可落地的治理方案"
category: 平台工程
tags:
- gpu
- governance
- quota
- finops
- admission-control
- multi-tenant
- ai-platform
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- AI 基础设施工程师
estimated_read_time: 20min
intent_queries:
- "GPU 集群如何做资源配额管理"
- "AI 平台 GPU 成本如何分摊"
- "GPU 使用准入控制怎么配置"
trigger_keywords:
- gpu-governance
- gpu-quota
- gpu-cost
- admission-control
- ai-platform
prerequisites:
- kubectl-basics
- k8s-rbac
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

# GPU 集群治理

## 概述

GPU 是 AI 基础设施中最昂贵且最稀缺的资源。一块 NVIDIA H100 的云端时薪可达 $3-5，一个中等规模的 AI 集群（100+ GPU）月度成本轻松超过百万美元。缺乏治理的 GPU 集群会迅速陷入"公地悲剧"：团队抢占资源、GPU 利用率低下（行业平均仅 30-40%）、成本归属不清、安全合规失控。

GPU 集群治理的目标是在保障各团队资源需求的前提下，最大化 GPU 利用率、明确成本归属、确保使用合规。本文从资源配额、使用策略、成本分摊、准入控制和合规审计五个维度构建完整的治理框架。

## 核心概念

### 治理层次模型

```
Layer 5: 合规审计（谁在什么时候用了什么资源，是否合规）
Layer 4: 准入控制（什么样的工作负载可以使用 GPU）
Layer 3: 成本分摊（GPU 费用如何归属到团队/项目）
Layer 2: 使用策略（GPU 如何使用：独占/共享/抢占）
Layer 1: 资源配额（每个团队/项目能用多少 GPU）
Layer 0: 物理资源（GPU 节点池、MIG 实例、网络拓扑）
```

### GPU 资源类型

| 资源类型 | K8s 资源名 | 粒度 | 适用场景 |
|---------|-----------|------|---------|
| 整卡 | nvidia.com/gpu | 1 GPU | 大模型训练 |
| MIG 实例 | nvidia.com/mig-2g.20gb | 1/7 卡 | 小模型推理 |
| 时间片 | nvidia.com/gpu (replicas) | 分数卡 | 开发/测试 |
| vGPU（HAMi） | nvidia.com/gpumem | MB 显存 | 多租户推理 |
| DRA ResourceClaim | 自定义 | 灵活 | 未来标准 |

### 配额模型设计

GPU 配额需要考虑多维度：
- **数量配额**：团队最多可使用 N 块 GPU
- **时间配额**：训练任务最长运行 T 小时
- **优先级配额**：高优先级任务可抢占低优先级
- **预算配额**：月度 GPU 费用上限

## 生产部署

### 多层级 ResourceQuota

```yaml
# 🟡 中风险：配置 GPU 资源配额
# 组织级配额（所有 AI 团队共享）
apiVersion: v1
kind: ResourceQuota
metadata:
  name: org-ai-gpu-quota
  namespace: ai-platform
spec:
  hard:
    nvidia.com/gpu: "64"
    nvidia.com/mig-2g.20gb: "32"
    requests.cpu: "512"
    requests.memory: "2Ti"
    limits.cpu: "1024"
    limits.memory: "4Ti"
    pods: "200"
---
# 团队级配额（NLP 团队）
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-nlp-gpu-quota
  namespace: team-nlp
spec:
  hard:
    nvidia.com/gpu: "16"
    nvidia.com/mig-2g.20gb: "8"
    requests.cpu: "128"
    requests.memory: "512Gi"
    persistentvolumeclaims: "20"
  scopeSelector:
    matchExpressions:
    - operator: In
      scopeName: PriorityClass
      values:
      - gpu-training
      - gpu-inference
---
# 项目级配额（特定训练项目）
apiVersion: v1
kind: ResourceQuota
metadata:
  name: project-llm-training-quota
  namespace: project-llm
spec:
  hard:
    nvidia.com/gpu: "8"
    requests.cpu: "64"
    requests.memory: "256Gi"
    pods: "20"
```

### LimitRange 与默认值

```yaml
# 🟡 中风险：设置 GPU Pod 默认资源限制
apiVersion: v1
kind: LimitRange
metadata:
  name: gpu-limit-range
  namespace: team-nlp
spec:
  limits:
  - type: Container
    default:
      cpu: "4"
      memory: "16Gi"
    defaultRequest:
      cpu: "2"
      memory: "8Gi"
    max:
      nvidia.com/gpu: "8"
      cpu: "64"
      memory: "256Gi"
    min:
      cpu: "100m"
      memory: "256Mi"
  - type: Pod
    max:
      nvidia.com/gpu: "8"
```

### 准入控制：GPU 使用策略

```yaml
# 🟡 中风险：ValidatingAdmissionPolicy 限制 GPU 使用
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: gpu-usage-policy
spec:
  failurePolicy: Fail
  matchConstraints:
    resourceRules:
    - apiGroups: [""]
      apiVersions: ["v1"]
      operations: ["CREATE", "UPDATE"]
      resources: ["pods"]
  validations:
  - expression: |
      // 所有 GPU Pod 必须设置 resource limits
      !has(object.spec.containers) ||
      object.spec.containers.all(c,
        !has(c.resources.limits) ||
        !('nvidia.com/gpu' in c.resources.limits) ||
        (has(c.resources.limits) && has(c.resources.requests))
      )
    message: "GPU Pod must set both resource requests and limits"
  - expression: |
      // GPU Pod 必须设置运行时限注解
      !object.spec.containers.exists(c,
        has(c.resources.limits) && 'nvidia.com/gpu' in c.resources.limits
      ) || has(object.metadata.annotations) &&
      'gpu.platform.io/max-runtime-hours' in object.metadata.annotations
    message: "GPU Pod must have max-runtime-hours annotation"
  - expression: |
      // 单 Pod 最多请求 8 块 GPU
      !object.spec.containers.exists(c,
        has(c.resources.limits) && 'nvidia.com/gpu' in c.resources.limits &&
        int(c.resources.limits['nvidia.com/gpu']) > 8
      )
    message: "Single Pod cannot request more than 8 GPUs"
---
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: gpu-usage-policy-binding
spec:
  policyName: gpu-usage-policy
  validationActions: [Deny]
  matchResources:
    namespaceSelector:
      matchLabels:
        gpu-governance: "enabled"
```

### 优先级与抢占策略

```yaml
# 🟡 中风险：GPU 工作负载优先级配置
# 推理服务（最高优先级，不可抢占）
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: gpu-inference-critical
value: 1000000
globalDefault: false
preemptionPolicy: PreemptLowerPriority
description: "Production inference services - highest priority"
---
# 训练任务（中优先级，可被推理抢占）
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: gpu-training
value: 500000
globalDefault: false
preemptionPolicy: PreemptLowerPriority
description: "Model training jobs - can be preempted by inference"
---
# 开发/实验（低优先级，随时可被抢占）
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: gpu-experiment
value: 100000
globalDefault: false
preemptionPolicy: Never
description: "Development and experimentation - lowest priority, no preemption"
---
# 使用优先级类的 GPU Pod
apiVersion: v1
kind: Pod
metadata:
  name: llm-inference
  namespace: team-nlp
  labels:
    app: llm-inference
    workload-type: inference
  annotations:
    gpu.platform.io/max-runtime-hours: "0"  # 0 = 无限制（长期服务）
    gpu.platform.io/cost-center: "CC-NLP-001"
spec:
  priorityClassName: gpu-inference-critical
  containers:
  - name: vllm
    image: registry.example.com/vllm/vllm-openai:v0.6.0
    resources:
      requests:
        nvidia.com/gpu: "2"
        cpu: "8"
        memory: "64Gi"
      limits:
        nvidia.com/gpu: "2"
        cpu: "16"
        memory: "128Gi"
```

### 成本分摊：OpenCost GPU 归因

```yaml
# 🟡 中风险：部署 OpenCost 进行 GPU 成本归因
apiVersion: apps/v1
kind: Deployment
metadata:
  name: opencost
  namespace: cost-management
spec:
  replicas: 1
  selector:
    matchLabels:
      app: opencost
  template:
    metadata:
      labels:
        app: opencost
    spec:
      serviceAccountName: opencost
      containers:
      - name: opencost
        image: ghcr.io/opencost/opencost:1.108.0
        env:
        - name: PROMETHEUS_SERVER_ENDPOINT
          value: "http://prometheus.monitoring:9090"
        - name: CLOUD_PROVIDER_API_KEY
          valueFrom:
            secretKeyRef:
              name: cloud-cost-key
              key: api-key
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: "1"
            memory: 1Gi
```

```bash
# 🟢 低风险：查询 GPU 成本分摊数据
# 按命名空间查询 GPU 成本
curl -s "http://opencost.cost-management:9003/allocation/compute?window=7d&aggregate=namespace&filterLabels=workload-type=gpu" | jq '.data[0]'

# 按团队标签查询
curl -s "http://opencost.cost-management:9003/allocation/compute?window=30d&aggregate=label:cost-center" | jq '.data[0]'

# 查看 GPU 利用率与成本关联
curl -s "http://opencost.cost-management:9003/assets?window=7d&filter=assetType:node&labels=gpu:true" | jq '.data[0]'
```

### 合规审计：GPU 使用日志

```yaml
# 🟡 中风险：配置 GPU 使用审计
apiVersion: v1
kind: ConfigMap
metadata:
  name: gpu-audit-policy
  namespace: kube-system
data:
  audit-policy.yaml: |
    apiVersion: audit.k8s.io/v1
    kind: Policy
    rules:
    # 记录所有 GPU Pod 的创建和删除
    - level: RequestResponse
      resources:
      - group: ""
        resources: ["pods"]
      verbs: ["create", "delete"]
      namespaces: ["team-nlp", "team-cv", "team-rec"]
    # 记录 GPU 节点标签变更
    - level: RequestResponse
      resources:
      - group: ""
        resources: ["nodes"]
      verbs: ["patch", "update"]
    # 记录 ResourceQuota 变更
    - level: RequestResponse
      resources:
      - group: ""
        resources: ["resourcequotas"]
      verbs: ["create", "update", "delete"]
```

## 运维操作

### GPU 配额使用率监控

```bash
# 🟢 低风险：查看 GPU 配额使用情况
# 查看各命名空间 GPU 配额使用
kubectl get resourcequota -A -o custom-columns=\
NS:.metadata.namespace,NAME:.metadata.name,\
GPU-USED:.status.used.nvidia\\.com/gpu,GPU-LIMIT:.status.hard.nvidia\\.com/gpu

# 查看节点 GPU 分配情况
kubectl describe nodes -l nvidia.com/gpu.present=true | \
  grep -A5 "Allocated resources" | grep nvidia

# 查看 GPU Pod 分布
kubectl get pods -A -o json | jq -r '.items[] |
  select(.spec.containers[].resources.limits["nvidia.com/gpu"] != null) |
  [.metadata.namespace, .metadata.name, .spec.nodeName] | @tsv'

# 检查 GPU 利用率（通过 DCGM Exporter）
kubectl query 'avg(DCGM_FI_DEV_GPU_UTIL) by (namespace, pod)' --endpoint http://prometheus:9090
```

### 空闲 GPU 回收

```bash
# 🟡 中风险：回收空闲 GPU 资源
# 查找 GPU 利用率低于 10% 超过 1 小时的 Pod
kubectl query 'avg_over_time(DCGM_FI_DEV_GPU_UTIL[1h]) < 10' \
  --endpoint http://prometheus:9090

# 标记空闲 Pod（添加注解，不直接删除）
kubectl annotate pod idle-training-job -n team-nlp \
  gpu.platform.io/idle-detected="2026-07-19T10:00:00Z" --overwrite

# 通知团队（通过告警系统）
# 超过 24 小时未响应则自动缩容
```

### 配额调整流程

```bash
# 🟡 中风险：调整团队 GPU 配额
# 1. 查看当前配额使用率
kubectl get resourcequota team-nlp-gpu-quota -n team-nlp -o yaml

# 2. 调整配额（需要审批流程）
kubectl patch resourcequota team-nlp-gpu-quota -n team-nlp \
  --type merge -p '{"spec":{"hard":{"nvidia.com/gpu":"24"}}}'

# 3. 验证配额生效
kubectl get resourcequota team-nlp-gpu-quota -n team-nlp -o jsonpath='{.spec.hard}'

# 4. 记录变更审计
kubectl annotate resourcequota team-nlp-gpu-quota -n team-nlp \
  governance.platform.io/change-reason="Q3 training capacity expansion" \
  governance.platform.io/approved-by="platform-admin" --overwrite
```

## 故障排查

### 配额相关问题

```bash
# 🟢 低风险：配额问题诊断
# 问题 1：Pod 因配额不足无法创建
# 错误：exceeded quota: team-nlp-gpu-quota, requested: nvidia.com/gpu=2, used: nvidia.com/gpu=16, limited: nvidia.com/gpu=16
kubectl describe resourcequota -n team-nlp
kubectl get pods -n team-nlp -o json | jq '[.items[] | select(.spec.containers[].resources.limits["nvidia.com/gpu"] != null)] | length'

# 问题 2：配额已用但 GPU 实际空闲
# 检查是否有 Completed/Failed Pod 仍占用配额
kubectl get pods -n team-nlp --field-selector=status.phase!=Running

# 问题 3：准入控制拒绝合法请求
kubectl get events -n team-nlp --field-selector reason=FailedCreate
# 检查 ValidatingAdmissionPolicy 日志
kubectl logs -n kube-system -l app=kube-apiserver --tail=100 | grep "gpu-usage-policy"
```

### 成本异常排查

```bash
# 🟢 低风险：成本异常诊断
# 检查 GPU 节点是否有非 GPU Pod（资源浪费）
kubectl get pods -A -o json | jq -r '.items[] |
  select(.spec.nodeName != null) |
  select(.spec.containers[].resources.limits["nvidia.com/gpu"] == null) |
  [.metadata.namespace, .metadata.name, .spec.nodeName] | @tsv' | \
  grep gpu-node

# 检查 GPU 节点 CPU/内存使用率（是否有非 GPU 负载占用）
kubectl top nodes -l nvidia.com/gpu.present=true
```

## 最佳实践

### 治理框架设计

1. **分层配额**：组织 → 部门 → 团队 → 项目四级配额，上层配额 ≥ 下层之和
2. **弹性配额**：使用 Volcano/Koordinator 的弹性配额，允许团队在空闲时借用其他团队配额
3. **时间窗口**：训练任务设置最大运行时间，超时自动终止（通过 ActiveDeadlineSeconds）
4. **成本可视化**：每周向团队发送 GPU 使用报告，包含利用率、费用、趋势
5. **审批流程**：超过配额的 GPU 申请走审批流程，平台团队评估后调整

### 与平台工程集成

- 配额管理集成到 [[平台工程/构建/01-platform-engineering-overview|内部开发者平台]]
- 成本数据接入 [[平台工程/治理/09-cost-optimization-finops|FinOps 平台]]
- 准入策略通过 [[平台工程/治理/17-multi-tenant-management|多租户管理]] 统一管理
- GPU 监控接入 [[可观测性/prometheus|Prometheus]] + Grafana 治理看板
- 参考 [[综合/multitenancy-resource-isolation-governance|多租户资源隔离治理]] 了解完整治理体系

## Related

- [[综合/gpu-scheduling-cost|GPU 调度与成本优化]]
- [[综合/gpu-operator-device-plugin-ecosystem|GPU Operator 生态]]
- [[平台工程/治理/17-multi-tenant-management|多租户管理]]
- [[平台工程/治理/09-cost-optimization-finops|成本优化与 FinOps]]
- [[容器运行时/containerd-CRI-O/16-gpu-runtime-nvidia-cdi|GPU 运行时]]
- [[综合/multitenancy-resource-isolation-governance|多租户资源隔离治理]]
