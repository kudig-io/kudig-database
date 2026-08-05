---
title: "Kueue 与 YuniKorn 批量调度"
description: "K8s 批量任务调度器 Kueue 与 YuniKorn 的架构、部署、配额管理及 AI 训练任务调度实践"
summary: "深入 Kueue（ClusterQueue/LocalQueue/Cohort）与 YuniKorn（Queue hierarchy/Gang scheduling/Fair-share）的架构对比、生产部署、资源配额借用、优先级抢占及 AI 训练任务批量调度最佳实践"
category: AI基础设施
tags:
- kueue
- yunikorn
- batch-scheduling
- gang-scheduling
- quota-management
- ai-training
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
- "Kueue 和 YuniKorn 有什么区别"
- "如何配置 Kueue ClusterQueue 资源配额"
- "AI 训练任务 Gang Scheduling 怎么实现"
trigger_keywords:
- kueue
- yunikorn
- batch-scheduling
- gang-scheduling
- cohort
- quota
prerequisites:
- kubectl-basics
- helm-basics
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

# Kueue 与 YuniKorn 批量调度

## 概述

在 AI/ML 工作负载日益增长的背景下，Kubernetes 默认调度器无法满足批量任务（Batch Jobs）的复杂调度需求：Gang Scheduling（全有或全无调度）、多级队列管理、跨团队资源配额借用、公平共享等。Kueue 和 YuniKorn 是当前 K8s 生态中两大主流批量调度解决方案。

Kueue 是 Kubernetes SIG-Scheduling 官方项目，以 Job 队列管理和配额控制为核心，采用"准入控制"模式——不替换默认调度器，而是在 Job 创建后决定何时允许其进入调度流程。YuniKorn 则是 Apache 基金会项目，作为完整的调度器插件运行，提供层次化队列、Gang Scheduling、Fair-share 等能力。

本文覆盖两者的架构设计、生产部署、AI 训练任务调度实践、资源配额管理以及故障排查，帮助平台工程师为多租户 AI 集群选择合适的调度方案。

相关页面：[[15-AI基础设施/05-K8s-AI基础设施/02-gpu-cluster-scheduling-inference-serving|GPU调度与资源管理]]、[[23-实体/11-AI与边缘/kubeflow|Kubeflow训练平台]]、[[23-实体/09-编排调度/volcano|Volcano批量调度]]、[[17-系统基础/06-知识字典/configuration/resource-quota|K8s资源配额与LimitRange]]、[[15-AI基础设施/05-K8s-AI基础设施/15-gpu-cost-attribution-multitenant|AI集群多租户隔离]]

## 架构与核心概念

### Kueue 架构

Kueue 的核心设计哲学是"管理 Job 的准入（Admission）而非调度（Scheduling）"。它与 kube-scheduler 协作而非替代：

```
Kueue 核心组件:

ClusterQueue（集群队列）:
  - 集群级别的资源配额池
  - 定义 CPU/GPU/Memory 的 nominalQuota 和 borrowingLimit
  - 支持 Cohort 分组实现队列间资源借用
  - 配置 preemption 策略（优先级抢占）

LocalQueue（本地队列）:
  - Namespace 级别，绑定到特定 ClusterQueue
  - 用户提交 Job 时指定 LocalQueue
  - 实现多租户隔离：team-a 只能访问自己的 LocalQueue

Workload（工作负载）:
  - Kueue 内部 CRD，代表一个待调度的 Job
  - 由 JobSet/Job/MPIJob 等自动创建
  - 状态：Pending → Admitted → Finished

Cohort（队列组）:
  - 一组 ClusterQueue 的逻辑分组
  - 组内队列可互相借用空闲资源
  - borrowingLimit 控制最大借用量

ResourceFlavor（资源风味）:
  - 描述不同类型的计算资源（如 A100 vs H100）
  - ClusterQueue 按 flavor 分配配额
  - 支持节点亲和性匹配
```

### YuniKorn 架构

YuniKorn 作为 kube-scheduler 的插件（Scheduler Plugin）运行，完全接管 Pod 调度决策：

```
YuniKorn 核心组件:

Queue Hierarchy（层次化队列）:
  - root → tenant → team 多级队列树
  - 每级队列配置 guaranteed/max capacity
  - 支持 FIFO/Fair/DRF 排序策略

Gang Scheduling:
  - 通过 task group annotation 定义
  - minMember 指定最小启动成员数
  - 超时后允许部分调度或拒绝

Fair-share 调度:
  - 基于 DRF（Dominant Resource Fairness）
  - 多资源维度公平分配
  - 防止单一用户/团队垄断资源

Placement Rules:
  - 自动将 Pod 放入正确队列
  - 基于 namespace/user/group 匹配
  - 减少手动 annotation 负担
```

### 架构对比

| 维度 | Kueue | YuniKorn |
|------|-------|----------|
| 调度模式 | 准入控制（Admission） | 完整调度器替换 |
| 与 kube-scheduler 关系 | 协作，不替换 | 替换（作为 scheduler plugin） |
| Gang Scheduling | 依赖 JobSet/ Volcano 集成 | 原生支持 |
| 队列模型 | 扁平 ClusterQueue + Cohort | 层次化多级队列树 |
| 资源借用 | Cohort 内 borrowingLimit | 队列间自动 fair-share |
| 抢占 | 基于 PriorityClass | 基于队列优先级 + Fair-share |
| 适用场景 | Job 队列管理、配额控制 | 复杂多租户、混合负载 |
| 社区成熟度 | K8s SIG 官方，快速迭代 | Apache 顶级项目，稳定 |
| 部署复杂度 | 低（CRD + Controller） | 中（Scheduler Plugin） |

## 生产部署

### Kueue 部署

```bash
# 🟢 低风险：只读检查当前集群是否已安装 Kueue
kubectl get crd | grep kueue
kubectl get deployment -n kueue-system

# 🟡 中风险：安装 Kueue（会创建 CRD 和 Controller）
KUEUE_VERSION="v0.9.1"
kubectl apply --server-side -f \
  "https://github.com/kubernetes-sigs/kueue/releases/download/${KUEUE_VERSION}/manifests.yaml"

# 验证安装
kubectl wait --for=condition=Available deployment/kueue-controller-manager \
  -n kueue-system --timeout=120s
```

### Kueue 资源配额配置

```yaml
# 🟡 中风险：创建 ResourceFlavor 和 ClusterQueue 会修改集群调度行为
apiVersion: kueue.x-k8s.io/v1beta1
kind: ResourceFlavor
metadata:
  name: gpu-a100
spec:
  nodeLabels:
    nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
---
apiVersion: kueue.x-k8s.io/v1beta1
kind: ResourceFlavor
metadata:
  name: gpu-h100
spec:
  nodeLabels:
    nvidia.com/gpu.product: "NVIDIA-H100-SXM5-80GB"
---
apiVersion: kueue.x-k8s.io/v1beta1
kind: Cohort
metadata:
  name: ai-training-cohort
spec:
  resourceGroups:
  - coveredResources: ["nvidia.com/gpu", "cpu", "memory"]
    flavors:
    - name: gpu-a100
      resources:
      - name: "nvidia.com/gpu"
        nominalQuota: 64
        borrowingLimit: 32
      - name: "cpu"
        nominalQuota: "256"
      - name: "memory"
        nominalQuota: "1024Gi"
    - name: gpu-h100
      resources:
      - name: "nvidia.com/gpu"
        nominalQuota: 32
        borrowingLimit: 16
---
apiVersion: kueue.x-k8s.io/v1beta1
kind: ClusterQueue
metadata:
  name: team-ml-clusterqueue
spec:
  cohort: ai-training-cohort
  queueingStrategy: BestEffortFIFO
  preemption:
    reclaimWithinCohort: LowerPriority
    withinClusterQueue: LowerPriority
  resourceGroups:
  - coveredResources: ["nvidia.com/gpu", "cpu", "memory"]
    flavors:
    - name: gpu-a100
      resources:
      - name: "nvidia.com/gpu"
        nominalQuota: 32
        borrowingLimit: 16
      - name: "cpu"
        nominalQuota: "128"
      - name: "memory"
        nominalQuota: "512Gi"
    - name: gpu-h100
      resources:
      - name: "nvidia.com/gpu"
        nominalQuota: 16
        borrowingLimit: 8
  stopPolicy: None
---
apiVersion: kueue.x-k8s.io/v1beta1
kind: LocalQueue
metadata:
  name: ml-training-queue
  namespace: team-ml
spec:
  clusterQueue: team-ml-clusterqueue
```

### YuniKorn 部署

```bash
# 🟡 中风险：安装 YuniKorn 会替换默认调度器
helm repo add yunikorn https://apache.github.io/yunikorn-release
helm repo update

helm install yunikorn yunikorn/yunikorn \
  --namespace yunikorn \
  --create-namespace \
  --set image.tag=1.5.0 \
  --set pluginImage.tag=1.5.0 \
  --set admissionController.image.tag=1.5.0 \
  --set configuration.policyGroup=default \
  --wait --timeout 300s

# 验证调度器就绪
kubectl get pods -n yunikorn -l app=yunikorn
```

### YuniKorn 队列配置

```yaml
# 🟡 中风险：修改 YuniKorn 调度配置影响全集群 Pod 调度
apiVersion: v1
kind: ConfigMap
metadata:
  name: yunikorn-configs
  namespace: yunikorn
data:
  queues.yaml: |
    partitions:
    - name: default
      placementrules:
      - name: tag
        value: namespace
        create: true
      queues:
      - name: root
        submitacl: "*"
        queues:
        - name: ai-training
          properties:
            guaranteed:
              memory: 2048Gi
              vcore: "512"
              nvidia.com/gpu: "64"
            max:
              memory: 4096Gi
              vcore: "1024"
              nvidia.com/gpu: "128"
          queues:
          - name: team-ml
            properties:
              guaranteed:
                memory: 1024Gi
                vcore: "256"
                nvidia.com/gpu: "32"
              max:
                memory: 2048Gi
                vcore: "512"
                nvidia.com/gpu: "64"
            applications:
              max: 50
          - name: team-nlp
            properties:
              guaranteed:
                memory: 1024Gi
                vcore: "256"
                nvidia.com/gpu: "32"
              max:
                memory: 2048Gi
                vcore: "512"
                nvidia.com/gpu: "64"
        - name: inference
          properties:
            guaranteed:
              memory: 512Gi
              vcore: "128"
              nvidia.com/gpu: "16"
            max:
              memory: 1024Gi
              vcore: "256"
              nvidia.com/gpu: "32"
```

## 运维操作

### Kueue 日常运维

```bash
# 🟢 低风险：查看 ClusterQueue 状态和配额使用情况
kubectl get clusterqueues -o wide
kubectl describe clusterqueue team-ml-clusterqueue

# 🟢 低风险：查看 Workload 排队状态
kubectl get workloads -A --sort-by=.metadata.creationTimestamp
kubectl get workloads -n team-ml -o custom-columns=\
NAME:.metadata.name,\
QUEUE:.spec.queueName,\
STATUS:.status.conditions[0].type,\
AGE:.metadata.creationTimestamp

# 🟢 低风险：查看 LocalQueue 使用情况
kubectl get localqueues -A
kubectl describe localqueue ml-training-queue -n team-ml

# 🟡 中风险：暂停 ClusterQueue（停止新任务准入，已运行任务不受影响）
kubectl patch clusterqueue team-ml-clusterqueue \
  --type=merge -p '{"spec":{"stopPolicy":"Hold"}}'

# 🟡 中风险：恢复 ClusterQueue
kubectl patch clusterqueue team-ml-clusterqueue \
  --type=merge -p '{"spec":{"stopPolicy":"None"}}'
```

### YuniKorn 运维

```bash
# 🟢 低风险：查看 YuniKorn 队列状态（通过 REST API）
YUNIKORN_POD=$(kubectl get pod -n yunikorn -l app=yunikorn -o jsonpath='{.items[0].metadata.name}')
kubectl exec -n yunikorn $YUNIKORN_POD -- curl -s http://localhost:9080/ws/v1/queues | jq .

# 🟢 低风险：查看应用调度状态
kubectl exec -n yunikorn $YUNIKORN_POD -- curl -s http://localhost:9080/ws/v1/apps | jq '.applications[] | {appID, queueName, applicationState}'

# 🟢 低风险：查看 Gang Scheduling 状态
kubectl exec -n yunikorn $YUNIKORN_POD -- curl -s http://localhost:9080/ws/v1/partition/default/queue/root.ai-training | jq .
```

### AI 训练任务提交（Kueue + JobSet）

```yaml
# 🟡 中风险：提交 GPU 训练任务消耗集群资源
apiVersion: jobset.x-k8s.io/v1alpha2
kind: JobSet
metadata:
  name: llama-finetune-7b
  namespace: team-ml
  labels:
    kueue.x-k8s.io/queue-name: ml-training-queue
spec:
  failurePolicy:
    maxRestarts: 3
  replicatedJobs:
  - name: worker
    replicas: 4
    template:
      spec:
        parallelism: 8
        completions: 8
        backoffLimit: 0
        suspend: true  # Kueue 管理 suspend 状态
        template:
          spec:
            schedulerName: default-scheduler
            containers:
            - name: trainer
              image: registry.internal/ai/pytorch-training:2.3-cuda12.4
              command:
              - torchrun
              - --nproc_per_node=8
              - --nnodes=4
              - train.py
              - --model=llama-7b
              - --data=/data/sft-corpus
              resources:
                limits:
                  nvidia.com/gpu: "8"
                  memory: "512Gi"
                requests:
                  nvidia.com/gpu: "8"
                  cpu: "64"
                  memory: "256Gi"
              volumeMounts:
              - name: training-data
                mountPath: /data
              - name: checkpoint
                mountPath: /checkpoints
            volumes:
            - name: training-data
              persistentVolumeClaim:
                claimName: sft-corpus-pvc
            - name: checkpoint
              persistentVolumeClaim:
                claimName: llama-checkpoint-pvc
            tolerations:
            - key: nvidia.com/gpu
              operator: Exists
              effect: NoSchedule
            nodeSelector:
              nvidia.com/gpu.product: "NVIDIA-A100-SXM4-80GB"
```

## 故障排查

### Workload 长时间 Pending

```bash
# 🟢 低风险：诊断 Workload 挂起原因
# Step 1: 查看 Workload 事件
kubectl describe workload <workload-name> -n team-ml
# 关注 Events 中的 "QuotaExceeded" 或 "Pending" 原因

# Step 2: 检查 ClusterQueue 配额使用
kubectl get clusterqueue team-ml-clusterqueue -o yaml | \
  yq '.status.flavorsUsage'

# Step 3: 检查是否有更高优先级任务占用配额
kubectl get workloads -A -o custom-columns=\
NAME:.metadata.name,PRIORITY:.spec.priority,STATUS:.status.conditions[0].type | \
  sort -k2 -rn

# Step 4: 检查 Cohort 内其他队列是否占满可借用资源
kubectl get clusterqueues -o custom-columns=\
NAME:.metadata.name,COHORT:.spec.cohort,ADMITTED:.status.admittedWorkloads
```

### 常见故障模式与解决方案

| 故障现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|---------|
| Workload 一直 Pending | ClusterQueue 配额用尽 | `kubectl describe clusterqueue` | 等待资源释放或增加 nominalQuota |
| Workload 被抢占 | 高优先级任务进入 | `kubectl get events --field-selector reason=Preempted` | 调整 PriorityClass 或增加配额 |
| Cohort 借用失败 | borrowingLimit 达到上限 | 检查 ClusterQueue status.flavorsUsage | 调整 borrowingLimit 或等待资源归还 |
| YuniKorn Gang 超时 | 集群资源不足无法满足 minMember | 查看 YuniKorn 日志中的 gang scheduling 事件 | 减小 minMember 或增加节点 |
| LocalQueue 不存在 | 未创建或 namespace 不匹配 | `kubectl get localqueues -n <ns>` | 创建对应 LocalQueue |
| Job 未被 Kueue 管理 | 缺少 queue-name label | 检查 Job/JobSet labels | 添加 `kueue.x-k8s.io/queue-name` label |

### Kueue Controller 异常

```bash
# 🟢 低风险：检查 Kueue Controller 健康状态
kubectl get pods -n kueue-system
kubectl logs -n kueue-system deployment/kueue-controller-manager --tail=100

# 🔴 高风险：重启 Kueue Controller（已准入的 Workload 不受影响，但新任务暂停准入）
kubectl rollout restart deployment/kueue-controller-manager -n kueue-system
kubectl rollout status deployment/kueue-controller-manager -n kueue-system --timeout=120s
```

## 最佳实践

### 配额规划

1. **分层配额设计**：按组织 → 团队 → 项目三级设置 ClusterQueue，利用 Cohort 实现团队间弹性借用
2. **GPU Flavor 分离**：不同 GPU 型号（A100/H100/L40S）使用独立 ResourceFlavor，避免配额混用
3. **预留缓冲**：nominalQuota 设为实际需求的 80%，留 20% 给 Cohort 借用
4. **StopPolicy 策略**：维护窗口使用 `Hold` 暂停新任务准入，避免直接删除 ClusterQueue

### 优先级与抢占策略

```yaml
# 🟡 中风险：配置优先级抢占策略
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: ai-training-critical
value: 1000
globalDefault: false
description: "关键 AI 训练任务，可抢占低优先级任务"
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: ai-training-normal
value: 100
globalDefault: false
description: "普通 AI 训练任务"
---
apiVersion: scheduling.k8s.io/v1
kind: PriorityClass
metadata:
  name: ai-experiment
value: 10
globalDefault: false
preemptionPolicy: Never  # 实验任务不抢占他人
description: "实验性任务，可被抢占"
```

### 选型建议

- **选 Kueue**：已有 kube-scheduler 工作良好，主要需求是 Job 队列管理和配额控制；使用 JobSet/MPIJob 等 K8s 原生 Job API；希望低侵入性部署
- **选 YuniKorn**：需要复杂层次化队列和 Fair-share；混合负载（训练 + 推理 + 数据处理）需要统一调度；需要原生 Gang Scheduling 且不依赖 Volcano
- **与 Volcano 对比**：Volcano 更侧重 HPC/AI 训练的高性能调度（如拓扑感知），Kueue 侧重配额和队列管理，YuniKorn 侧重多租户 Fair-share。三者可组合使用（如 Kueue 管配额 + Volcano 管调度）

### 监控指标

```bash
# 🟢 低风险：关键 Prometheus 指标
# Kueue 指标
kueue_admitted_workloads_total{cluster_queue="team-ml-clusterqueue"}
kueue_pending_workloads{cluster_queue="team-ml-clusterqueue"}
kueue_quota_utilization{cluster_queue="team-ml-clusterqueue", resource="nvidia.com/gpu"}
kueue_admission_wait_time_seconds_bucket{cluster_queue="team-ml-clusterqueue"}

# YuniKorn 指标
yunikorn_queue_app_count{queue="root.ai-training.team-ml", state="Running"}
yunikorn_queue_resource{queue="root.ai-training.team-ml", resource="nvidia.com/gpu", state="allocated"}
yunikorn_scheduling_latency_seconds_bucket
```

## Related

- [[15-AI基础设施/05-K8s-AI基础设施/02-gpu-cluster-scheduling-inference-serving|GPU调度与资源管理]]
- [[23-实体/11-AI与边缘/kubeflow|Kubeflow训练平台]]
- [[23-实体/09-编排调度/volcano|Volcano批量调度]]
- [[17-系统基础/06-知识字典/configuration/resource-quota|K8s资源配额与LimitRange]]
- [[15-AI基础设施/05-K8s-AI基础设施/15-gpu-cost-attribution-multitenant|AI集群多租户隔离]]
