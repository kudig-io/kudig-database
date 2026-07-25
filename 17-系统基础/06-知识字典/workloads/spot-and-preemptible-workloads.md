---
title: Spot 与可抢占工作负载
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kafka
- pdb
- daemonset
- job
- operator
- gpu
- nvidia
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Spot 与可抢占工作负载 是什么
- 如何 Spot 与可抢占工作负载
trigger_keywords:
- Spot
- 与可抢占工作负载
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- kafka-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Spot 与可抢占工作负载

## 概述

在云原生环境中，**Spot 实例（AWS）、Preemptible VM（GCP）和 Low-priority VM（Azure）** 是云厂商以大幅折扣出售的闲置计算容量。2026 年的最佳实践表明，通过将**容错型工作负载**（如 AI 训练、批处理、CI/CD）部署到 Spot 实例上，企业可将计算成本降低 **50%–90%**。[[Kubernetes|Kubernetes]] 结合 Kueue、Cluster Autoscaler 和 checkpoint 机制，已能安全、自动化地管理可抢占工作负载的生命周期。

## 核心概念/原理

### 1. 可抢占实例的工作机制

云厂商在需要容量时会提前通知（通常提前 **30 秒–2 分钟**）并回收 Spot 实例：
- **AWS Spot Instances**：提供最多 2 分钟的终止通知（ITN, Instance Termination Notice）
- **GCP Preemptible VMs**：提供 30 秒的抢占通知
- **Azure Low-priority VMs**：可被随时回收

Kubernetes 中的 Pod 会收到 `SIGTERM` 信号，随后进入优雅终止期（默认 30 秒），超时后收到 `SIGKILL`。

### 2. Kubernetes 对中断的响应

Kubernetes 提供了多个机制配合 Spot 实例：
- **Pod Disruption Budget（PDB）**：控制同时中断的 Pod 数量，保护有状态服务
- **Node Termination Handler**：部署为 [[DaemonSet|DaemonSet]]，监听 IMDS（Instance Metadata [[Service|Service]]）获取中断通知，提前将节点标记为 `NoSchedule` 并驱逐 Pod
- **Cluster Autoscaler / Karpenter**：在 Spot 实例被回收后，自动在其他可用区或实例类型上补充新节点

### 3. Checkpoint 与容错设计

将工作负载迁移到 Spot 的前提是应用必须具备**中断恢复能力**：
- **AI 训练**：定期保存模型权重和优化器状态（如每 N 个 epoch 或每 X 分钟）
- **批处理作业**：将中间结果写入对象存储（S3），支持断点续传
- **CI/CD Pipeline**：将构建产物和缓存持久化到外部存储
- **数据处理**：使用支持 exactly-once 语义的消息队列（如 Kafka）保证数据处理不丢失

### 4. Spot 与按需实例的混合架构

2026 年的最佳实践推荐**混合节点池（Mixed Instance Pools）**：
- **按需/Reserved 实例**：运行核心控制平面、有状态数据库、关键在线服务
- **Spot 实例**：运行训练任务、大数据批处理、开发测试环境、无状态 Worker
- **可回退策略**：当 Spot 容量不足时，自动回退到按需实例（AWS Spot Fleet / Karpenter 支持）

## 关键机制或特性

### Node Termination Handler 工作流程

```
1. 云厂商发出 Spot 中断通知
        ↓
2. Node Termination Handler 检测到 IMDS 信号
        ↓
3. 立即对节点执行 `cordon`（禁止新 Pod 调度）
        ↓
4. 发起 Pod 驱逐（Eviction），给应用 30 秒优雅关闭时间
        ↓
5. 应用执行 checkpoint 并保存状态
        ↓
6. Pod 被重新调度到其他节点（Spot 或 On-Demand）
        ↓
7. 应用从 checkpoint 恢复，继续执行
```

### Kueue 与 Spot 队列

结合 **Kueue** 可实现更精细的 Spot 工作负载管理：
- 为 Spot 实例定义独立的 `ResourceFlavor`
- 只有标记为 `spot-tolerant` 的作业才能进入 Spot 队列
- 当 Spot 容量不足时，Kueue 自动将作业保持在队列中等待，或路由到按需队列

### Spot 实例多样化策略

通过 **Karpenter** 或 **Cluster Autoscaler（Mixed Instance Policy）** 配置多种实例类型：
- 不同实例族（如 m6i、m5、m4）
- 不同可用区
- 这显著提高了获取 Spot 容量的概率

## 使用场景

1. **大模型分布式训练**：在 100+ Spot GPU 节点上训练 LLM，每 15 分钟 checkpoint 一次，中断后自动恢复
2. ** nightly 数据仓库 ETL**：使用 Spot 实例运行数小时的 Spark/Flink 批处理任务
3. **CI/CD 构建农场**：将无状态的编译、测试任务调度到 Spot 实例，大幅降低 DevOps 基础设施成本
4. **渲染农场**：影视特效的分布式渲染任务天然适合 Spot 实例的短时、可中断特性
5. **开发/测试环境**：开发集群全天候运行成本高，使用 Spot 实例配合定时启停策略可节省 70%+

## 最佳实践/注意事项

- **Checkpoint 频率是关键**：Spot 实例可能每小时被中断多次，checkpoint 间隔应控制在 5–15 分钟
- **最小化启动时间**：使用预置镜像和容器缓存，确保 Pod 在迁移后能快速恢复运行
- **PDB 不适用于纯 Spot 批处理**：批处理任务通常不需要 PDB，但混合部署时需注意不要将有状态服务调度到 Spot 节点
- **节点亲和性/反亲和性**：使用 `nodeAffinity` 或污点（Taints）明确区分 Spot 节点和按需节点
- **监控中断率和恢复时间**：核心指标包括 Spot 中断频率、checkpoint 成功率、任务总完成时间（含恢复）
- **存储必须持久化**：Spot 实例的本地磁盘会随实例终止而丢失，所有重要数据必须写入 PVC 或对象存储
- **避免跨可用区通信**：Spot 节点分散在不同 AZ 时，注意控制平面和数据传输的跨 AZ 带宽成本
- **成本建模**：不仅比较实例单价，还需考虑因中断导致的重复计算成本和额外存储开销

## 生产 YAML 示例

### Spot 节点 Taint + Toleration 配置

```yaml
# Karpenter NodePool 定义 Spot 节点
apiVersion: karpenter.sh/v1
kind: NodePool
metadata:
  name: spot-gpu-pool
spec:
  template:
    spec:
      requirements:
      - key: karpenter.sh/capacity-type
        operator: In
        values: ["spot"]               # 仅使用 Spot 实例
      - key: node.kubernetes.io/instance-type
        operator: In
        values:                        # 多实例类型提高 Spot 可用性
        - p3.2xlarge
        - p3.8xlarge
        - g5.2xlarge
        - g5.4xlarge
      - key: topology.kubernetes.io/zone
        operator: In
        values: ["us-east-1a", "us-east-1b", "us-east-1c"]
      taints:
      - key: spot-instance
        value: "true"
        effect: NoSchedule
  limits:
    cpu: "1000"
    nvidia.com/gpu: "64"
---
# 训练 Job 容忍 Spot 节点 taint
apiVersion: batch/v1
kind: Job
metadata:
  name: llm-finetune
  namespace: ml-team
spec:
  parallelism: 8
  completions: 8
  completionMode: Indexed
  template:
    spec:
      tolerations:
      - key: spot-instance
        operator: Equal
        value: "true"
        effect: NoSchedule
      nodeSelector:
        karpenter.sh/capacity-type: spot
      containers:
      - name: trainer
        image: registry.example.com/ml/trainer:v5.0
        env:
        - name: CHECKPOINT_DIR
          value: "s3://ml-checkpoints/llm-finetune/"
        - name: CHECKPOINT_INTERVAL_MINUTES
          value: "10"                    # 每 10 分钟 checkpoint
        resources:
          requests:
            nvidia.com/gpu: "1"
            memory: "32Gi"
            cpu: "8"
          limits:
            nvidia.com/gpu: "1"
            memory: "32Gi"
      restartPolicy: Never
  backoffLimit: 10                      # Spot 中断需要多次重试
```

### Node Termination Handler DaemonSet

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: aws-node-termination-handler
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: aws-node-termination-handler
  template:
    metadata:
      labels:
        app: aws-node-termination-handler
    spec:
      nodeSelector:
        karpenter.sh/capacity-type: spot    # 仅在 Spot 节点运行
      tolerations:
      - key: spot-instance
        operator: Exists
      serviceAccountName: nth-sa
      hostNetwork: true
      containers:
      - name: handler
        image: public.ecr.aws/aws-ec2/aws-node-termination-handler:1.22
        env:
        - name: ENABLE_SPOT_INTERRUPTION_DRAINING
          value: "true"
        - name: ENABLE_REBALANCE_DRAINING
          value: "true"
        - name: GRACE_PERIOD
          value: "120"                      # 与 Pod terminationGracePeriodSeconds 匹配
        resources:
          requests:
            cpu: "50m"
            memory: "64Mi"
      priorityClassName: system-node-critical
```

### Kueue Spot 队列配置

```yaml
# ResourceFlavor 标记 Spot 容量
apiVersion: kueue.x-k8s.io/v1beta1
kind: ResourceFlavor
metadata:
  name: spot-gpu
spec:
  nodeLabels:
    karpenter.sh/capacity-type: spot
  tolerations:
  - key: spot-instance
    operator: Exists
---
# ClusterQueue 定义 Spot 资源配额
apiVersion: kueue.x-k8s.io/v1beta1
kind: ClusterQueue
metadata:
  name: spot-training-queue
spec:
  resourceGroups:
  - coveredResources: ["cpu", "memory", "nvidia.com/gpu"]
    flavors:
    - name: spot-gpu
      resources:
      - name: "nvidia.com/gpu"
        nominalQuota: 64
      - name: "cpu"
        nominalQuota: 512
      - name: "memory"
        nominalQuota: "2Ti"
  preemption:
    reclaimWithinCohort: Any
    withinClusterQueue: LowerPriority
```

## Spot 中断处理时间线

```
# 🟢 低风险：只读/信息收集，通常无副作用
T=0s   云厂商发出中断通知
       ├─ AWS: 2 分钟（ITN）
       ├─ GCP: 30 秒
       └─ Azure: 随时
          │
T+2s   Node Termination Handler 检测到 IMDS 信号
          │
T+3s   节点被 cordon（禁止新 Pod 调度）
          │
T+5s   Pod 驱逐开始 → PreStop Hook 执行
          │
T+5~35s 应用执行 checkpoint 并保存到对象存储
          │
T+35s  SIGTERM 发送给容器进程
          │
T+60s  宽限期到期 → SIGKILL（如果还未退出）
          │
T+120s  Spot 实例被回收
          │
T+125s Cluster Autoscaler/Karpenter 开始补充新节点
          │
T+180s 新节点就绪，Pod 被重新调度
          │
T+185s Pod 从 checkpoint 恢复，继续执行
```
## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Spot 中断后 Pod 未被重新调度 | 无可用节点或 Cluster Autoscaler 未触发 | `kubectl get nodes`；检查 CA/Karpenter 日志 |
| Checkpoint 数据丢失 | 使用了本地存储而非对象存储 | 确认 checkpoint 写入 S3/GCS/Azure Blob |
| 训练从头开始而非恢复 | 应用未正确实现 checkpoint 恢复逻辑 | 在测试环境手动模拟 Pod 重启验证恢复 |
| Spot 节点频繁被回收 | 实例类型过于热门或未多样化 | 增加可选实例类型和可用区数量 |
| 有状态服务被调度到 Spot 节点 | 缺少 nodeSelector 或 taint 隔离 | 为 Spot 节点配置专用 taint；有状态服务不容忍该 taint |

## 生产检查清单

- [ ] Spot 节点配置专用 taint，防止非容错工作负载调度
- [ ] 有状态服务（数据库、消息队列）不调度到 Spot 节点
- [ ] 训练/批处理任务实现 checkpoint 机制，间隔 ≤ 15 分钟
- [ ] Checkpoint 数据写入持久化对象存储（S3/GCS/PVC）
- [ ] 部署 Node Termination Handler 监听中断通知
- [ ] 配置多实例类型和多可用区提高 Spot 可用性
- [ ] Cluster Autoscaler/Karpenter 配置 Spot 到 On-Demand 的回退策略
- [ ] 监控核心指标：Spot 中断频率、checkpoint 成功率、任务总完成时间
- [ ] 成本分析考虑重复计算和跨 AZ 数据传输开销
- [ ] PreStop + terminationGracePeriodSeconds 足够完成 checkpoint

## 命令快速参考

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 查看 Spot 节点
kubectl get nodes -l karpenter.sh/capacity-type=spot

# 查看 Spot 节点上运行的 Pod
kubectl get pods -A --field-selector spec.nodeName=<spot-node>

# 手动模拟 Spot 中断（测试）
kubectl drain <node> --grace-period=30 --delete-emptydir-data --ignore-daemonsets

# 查看 Node Termination Handler 日志
kubectl logs -n kube-system -l app=aws-node-termination-handler --tail=50

# 检查 Karpenter 节点池状态
kubectl get nodepools
kubectl describe nodepool spot-gpu-pool

# 查看最近的节点驱逐事件
kubectl get events -A --field-selector reason=Evicted --sort-by='.lastTimestamp'
```
## 交叉引用

- [Disruptions](disruptions.md) — PDB 在 Spot 中断时的保护作用
- [Jobs](jobs.md) — 批处理 Job 的 backoffLimit 和失败策略
- [DaemonSet](daemonset.md) — Node Termination Handler 的 DaemonSet 部署
- [自动扩缩工作负载](autoscaling-workloads.md) — KEDA 与 Spot 队列的结合

## 参考链接

- [AWS Spot Instances Best Practices](https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/spot-best-practices.html)
- [GCP Preemptible VMs](https://cloud.google.com/compute/docs/instances/preemptible)
- [Azure Spot Virtual Machines](https://docs.microsoft.com/en-us/azure/virtual-machines/spot-vms)
- [AWS Node Termination Handler](https://github.com/aws/aws-node-termination-handler)
- [Karpenter Documentation](https://karpenter.sh/docs/)

## Related

- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
