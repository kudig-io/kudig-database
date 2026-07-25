---
title: Volcano [entities]
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- orchestration
- volcano
- scheduler
- containerd
- harbor
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Volcano 是什么
- 如何 Volcano
trigger_keywords:
- Volcano
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Volcano

> **CNCF 状态**: Incubating | **类别**: Orchestration | **主要语言**: Go

## 概述

Volcano（原 kube-batch）是一个 CNCF 孵化项目，由华为开源，是 Kubernetes 上的高性能批处理工作负载调度器。它专为 AI/ML 训练、大数据分析、HPC（高性能计算）等批处理场景设计，提供了 gang scheduling（成组调度）、公平调度、队列管理、任务依赖等 K8s 默认调度器不具备的高级调度能力。Volcano 已被众多企业用于 GPU 集群管理和大规模 ML 训练平台。

## Key Features（核心能力）

- **Gang Scheduling**：成组调度，确保所有相关 Pod 同时被调度或全部等待
- **Queue 管理**：支持多级队列、权重和抢占，实现公平资源分配
- **任务依赖**：支持 DAG 依赖关系，定义步骤间执行顺序
- **插件化调度算法**：提供多种调度插件（DRF、Binpack、Spread、Topology）
- **GPU 共享**：支持 GPU 细粒度共享和切分，提升 GPU 利用率
- **Job 控制器**：Volcano Job CRD 支持任务重试、生命周期管理

## 架构与工作原理

Volcano 由三个核心组件构成：Volcano Controller 负责管理 Volcano Job CRD 的生命周期，处理任务状态转换；Volcano Scheduler 作为独立调度器，通过调度插件链（Action/Plugin）执行调度决策；Volcano Admission Webhook 负责 API 校验和默认值填充。调度器支持多种调度插件组合，通过 YAML 配置灵活定义调度策略。Volcano 通过 MutatingWebhook 将 Pod 绑定到自身调度器。

## K8s 集成

Volcano 通过 CRD 与 Kubernetes 集成：Volcano Job（vcjob）定义批处理任务，支持 minAvailable、policies、tasks 等高级配置；Volcano Queue 定义资源队列，支持权重和公平调度；Volcano PodGroup 将相关 Pod 组织成调度单元。通过指定 schedulerName: volcano 将 Pod 交给 Volcano 调度器处理。Volcano 复用 K8s 的 Node/PV/PVC 等资源模型。

## 生产用例

- **分布式 ML 训练**：使用 Gang Scheduling 确保 TensorFlow/PyTorch 训练任务的所有 Worker 同时启动
- **Spark/Flink 批处理**：为大数据分析任务提供公平调度和资源排队
- **HPC 计算**：科学计算、基因测序等高性能计算场景
- **CI/CD 并行任务**：大规模并行构建和测试任务调度

## 安装与配置

```bash
# 🟢 Helm 安装
helm repo add volcano-sh https://volcano-sh.github.io/helm-charts
helm install volcano volcano-sh/volcano -n volcano-system --create-namespace

# 🟢 验证安装
kubectl get pods -n volcano-system
kubectl get crd | grep volcano.sh

# 🟢 查看调度器配置
kubectl get configmap volcano-scheduler-configmap -n volcano-system -o yaml

# 🟡 卸载
helm uninstall volcano -n volcano-system
```

### Volcano Job CRD 示例

```yaml
apiVersion: batch.volcano.sh/v1alpha1
kind: Job
metadata:
  name: pytorch-training
  namespace: ml-training
spec:
  minAvailable: 4  # Gang Scheduling: 至少4个 Pod 同时调度
  schedulerName: volcano
  queue: gpu-queue
  policies:
  - event: PodEvicted
    action: RestartJob
  - event: PodFailed
    action: RestartTask
  maxRetry: 3
  plugins:
    ssh: []
    svc: []
  tasks:
  - replicas: 1
    name: master
    template:
      spec:
        containers:
        - name: pytorch-master
          image: pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime
          command: ["python", "train.py", "--role=master"]
          resources:
            requests:
              nvidia.com/gpu: 1
              memory: 16Gi
            limits:
              nvidia.com/gpu: 1
              memory: 32Gi
        restartPolicy: OnFailure
  - replicas: 3
    name: worker
    template:
      spec:
        containers:
        - name: pytorch-worker
          image: pytorch/pytorch:2.1.0-cuda12.1-cudnn8-runtime
          command: ["python", "train.py", "--role=worker"]
          resources:
            requests:
              nvidia.com/gpu: 1
              memory: 16Gi
            limits:
              nvidia.com/gpu: 1
              memory: 32Gi
        restartPolicy: OnFailure
```

### Queue 配置示例

```yaml
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: gpu-queue
spec:
  weight: 5
  capability:
    nvidia.com/gpu: 32
    memory: 512Gi
  reclaimable: true
  guarantee:
    resource:
      nvidia.com/gpu: 8
      memory: 128Gi
---
apiVersion: scheduling.volcano.sh/v1beta1
kind: Queue
metadata:
  name: cpu-queue
spec:
  weight: 3
  capability:
    cpu: 100
    memory: 400Gi
  reclaimable: true
```

## 运维操作

### 常用命令

```bash
# 🟢 查看 Volcano Job
kubectl get vcjob -A
kubectl describe vcjob pytorch-training -n ml-training

# 🟢 查看 Queue 状态
kubectl get queue
kubectl describe queue gpu-queue

# 🟢 查看 PodGroup
kubectl get podgroup -A

# 🟢 查看调度器日志
kubectl logs -n volcano-system -l app=volcano-scheduler --tail=100

# 🟢 查看 Controller 日志
kubectl logs -n volcano-system -l app=volcano-controller --tail=100

# 🟡 删除 Job
kubectl delete vcjob pytorch-training -n ml-training

# 🟡 暂停/恢复 Queue
kubectl patch queue gpu-queue -p '{"spec":{"state":"Closed"}}' --type=merge
kubectl patch queue gpu-queue -p '{"spec":{"state":"Open"}}' --type=merge

# 🟢 查看调度器配置
kubectl get cm volcano-scheduler-configmap -n volcano-system -o yaml
```

### 调度器插件配置

```yaml
# volcano-scheduler-configmap
apiVersion: v1
kind: ConfigMap
metadata:
  name: volcano-scheduler-configmap
  namespace: volcano-system
data:
  volcano-scheduler.conf: |
    actions: "enqueue, allocate, preempt, reclaim, backfill"
    tiers:
    - plugins:
      - name: priority
      - name: gang
        enablePreemptable: false
      - name: conformance
    - plugins:
      - name: drf
        enablePreemptable: false
      - name: predicates
      - name: proportion
      - name: nodeorder
      - name: binpack
      - name: tdm
        arguments:
          tdm.revocable-zone: rz1
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| Job Pending | 资源不足/Gang 未满足 | `kubectl describe vcjob <name>` | 检查 minAvailable 与可用资源 |
| Pod 未被 Volcano 调度 | schedulerName 未指定 | `kubectl get pod -o yaml \| grep schedulerName` | 设置 schedulerName: volcano |
| Queue 状态 Closed | 手动关闭/资源超限 | `kubectl describe queue <name>` | 重新开启 Queue |
| 抢占不生效 | 插件未启用/优先级相同 | 查看 scheduler configmap | 启用 preempt action 和 priority 插件 |
| GPU 分配失败 | Device Plugin 未就绪 | `kubectl describe node \| grep gpu` | 检查 NVIDIA Device Plugin |
| 任务重试过多 | 应用错误/资源竞争 | `kubectl get events --field-selector reason=FailedScheduling` | 检查应用日志和资源请求 |

### 排查流程

```
1. kubectl get vcjob → 确认 Job 状态 (Pending/Running/Completed)
2. kubectl describe vcjob <name> → 查看 Events 和 Pod 状态
3. kubectl get podgroup → 确认 Gang Scheduling 状态
4. kubectl logs -l app=volcano-scheduler → 查看调度决策日志
5. kubectl describe queue → 确认队列资源分配
```

## 生产案例

### 案例1: 大规模 PyTorch 分布式训练
- **场景**: 64 GPU 分布式训练，需要所有 Worker 同时启动
- **方案**: Volcano Gang Scheduling + minAvailable=64，确保所有 Pod 同时获得 GPU
- **效果**: 避免部分调度导致的 GPU 空闲浪费，训练效率提升 30%

### 案例2: 多租户 GPU 集群公平调度
- **场景**: 多个 ML 团队共享 200 GPU 集群
- **方案**: 按团队创建 Queue，配置 weight 和 guarantee，启用 DRF 公平调度
- **效果**: 各团队资源使用公平透明，GPU 利用率从 45% 提升至 78%

## 对比替代方案

| 维度 | Volcano | K8s 默认调度器 | YuniKorn | YARN/Mesos |
|------|---------|----------------|----------|------------|
| Gang Scheduling | 原生支持 | 不支持 | 支持 | 支持 |
| Queue 管理 | 多级队列+权重 | 无 | 多级队列 | 支持 |
| GPU 共享 | 支持 | 不支持 | 有限 | 不支持 |
| K8s 原生 | 是 | 是 | 是 | 否 |
| AI/ML 优化 | 深度优化 | 无 | 有限 | 无 |
| 任务依赖 (DAG) | 支持 | 不支持 | 不支持 | 支持 |

## 检查清单

- [ ] Volcano 组件 (scheduler/controller/admission) 均 Running
- [ ] Queue 已创建并配置合理的 weight 和 capability
- [ ] Job 指定了 schedulerName: volcano
- [ ] minAvailable 设置合理 (不超过可用资源)
- [ ] GPU Device Plugin 已安装并就绪
- [ ] 调度插件配置符合业务需求
- [ ] 监控 Queue 资源使用率和 Job 等待时间
- [ ] 配置了合理的重试策略和超时

## Related

- [[08-containerd-multi-tenant]] — [[containerd|containerd]]rd 多租户|containerd 多租户]]租户|多租户]]
- [[harbor]] — Harbor
- [[opentofu]] — OpenTofu
- [[cartography]] — Cartography
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- volcano
- [[23-实体/15-参考与索引/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
