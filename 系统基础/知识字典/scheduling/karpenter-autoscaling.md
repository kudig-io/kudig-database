---
title: Karpenter 自动扩缩容
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- pdb
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
- Karpenter 自动扩缩容 是什么
- 如何 Karpenter 自动扩缩容
trigger_keywords:
- Karpenter
- 自动扩缩容
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Karpenter 自动扩缩容

## 概述

**Karpenter** 是 AWS 开源的 [[Kubernetes|Kubernetes]] 节点自动扩缩容项目，已成为 2026 年替代传统 **Cluster Autoscaler** 的主流方案。与 Cluster Autoscaler 相比，Karpenter 不再依赖预配置的节点组（Node Group/Auto Scaling Group），而是直接观察 Pending Pod 的资源需求，实时选择最优的实例类型和购买选项（On-Demand / Spot），并在秒级内启动新节点。这种"**直接调度到云**"的架构显著提升了资源利用率、降低了成本和启动延迟。

## 核心概念/原理

### 1. Karpenter vs Cluster Autoscaler

| 维度 | Cluster Autoscaler | Karpenter |
|------|-------------------|-----------|
| **节点组依赖** | 必须预定义节点组（ASG/MIG） | 无需节点组，直接启动实例 |
| **实例选择** | 在预定义组中选择 | 从数百种实例类型中全局优化选择 |
| **启动速度** | 较慢（通常 2–5 分钟） | 更快（通常 30–90 秒） |
| **Spot 集成** | 需要单独的节点组 | 原生支持混合 On-Demand + Spot |
| **调度感知** | 较弱，可能导致资源碎片 | 深度集成 Kubernetes 调度器，减少碎片 |
| **多云支持** | 通用 | 目前主要支持 AWS，Azure/GCP 在发展中 |

### 2. Karpenter 核心组件

- **Karpenter Controller**：运行在集群中的 Deployment，持续监听 Pending Pod 和节点利用率
- **Provisioner / NodePool**：定义节点配置模板，包括实例类型、购买选项、标签、污点、启动模板等
- **NodeClaim**：Karpenter 创建的节点请求抽象，对应云厂商实际启动的 EC2 实例
- **Consolidation（整合）**：自动检测并替换利用率低的节点，优化成本和布局
- **Drift（漂移）**：当节点配置（如 AMI、实例类型可用性）与 NodePool 定义不一致时，自动替换节点

```yaml
# Karpenter NodePool 示例
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: default
spec:
  template:
    spec:
      requirements:
        - key: karpenter.k8s.aws/instance-category
          operator: In
          values: ["c", "m", "r"]
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["spot", "on-demand"]
        - key: kubernetes.io/arch
          operator: In
          values: ["amd64", "arm64"]
      nodeClassRef:
        name: default
  limits:
    cpu: 1000
    memory: 1000Gi
  disruption:
    consolidationPolicy: WhenUnderutilized
    expireAfter: 720h
```

### 3. 整合（Consolidation）机制

Karpenter 的 Consolidation 是其成本优化能力的核心：
- **WhenUnderutilized**：当节点利用率持续低于阈值时，Karpenter 会尝试将其上的 Pod 迁移到更小或更少的节点上，然后终止原节点
- **WhenEmpty**：仅当节点完全为空时才进行整合
- **Spot 替换**：当 Spot 实例被中断时，Karpenter 会自动启动替代实例（优先 Spot，必要时 On-Demand）

### 4. 调度感知扩缩容

Karpenter 在启动节点前会模拟 Kubernetes 调度器的决策：
- 不仅考虑 CPU/Memory 请求，还考虑 Pod 的 NodeSelector、亲和性/反亲和性、拓扑分布约束
- 选择能够满足所有 Pending Pod 约束的最小实例组合
- 支持 GPU、ARM 架构、本地 NVMe SSD 等特殊资源需求

## 关键机制或特性

### 实例多样性策略

Karpenter 的 EC2NodeClass 允许指定广泛的实例类型，而不是局限于一个节点组：
- 自动选择当前可用区中价格最优且库存充足的实例
- 支持 ARM（Graviton）和 x86 混合部署
- 支持从 t3.micro 到 u-24tb1.metal 的全系列 EC2 实例

### Spot 与 On-Demand 混合

通过在 NodePool 中同时指定 `spot` 和 `on-demand`，Karpenter 会：
- 优先尝试分配 Spot 实例
- 当 Spot 容量不足时，自动回退到 On-Demand
- 无需维护两套节点组配置

### 快速节点启动

Karpenter 直接调用 AWS EC2 RunInstances API，无需等待 Auto Scaling Group 的复杂生命周期：
- 启动时间通常比 Cluster Autoscaler 快 30%–60%
- 使用预置的 AMI 和 UserData 快速完成节点注册
- 支持自定义启动模板（Launch Template）预加载容器镜像

### 终止保护与安全

- **Pod Disruption Budget（PDB）尊重**：在驱逐 Pod 进行整合时，严格遵守 PDB
- **Termination Grace Period**：给予 Pod 充足的时间完成优雅终止
- **Do Not Evict Annotation**：为关键 Pod 添加 `karpenter.sh/do-not-evict: "true"` 防止被中断

## 使用场景

1. **AI 训练集群弹性扩缩**：训练任务提交后，Karpenter 在 1 分钟内启动 100 台 P4d GPU 实例；训练完成后自动整合并释放节点
2. **Web 服务波峰波谷应对**：电商大促期间流量激增 10 倍，Karpenter 自动扩展 Spot 实例应对峰值，活动结束后缩容至基线
3. **ARM 迁移成本优化**：Karpenter 自动将无状态工作负载调度到 AWS Graviton 实例，降低 40% 计算成本
4. **批处理队列驱动扩缩**：Kueue 队列中积累了大量待处理作业，Karpenter 自动启动 cheapest available 实例消化队列
5. **节点配置自动更新**：当发布新的 hardened AMI 或 [[kubelet|kubelet]] 版本时，Karpenter 的 Drift 机制自动滚动替换旧节点

## 最佳实践/注意事项

- **为不同工作负载创建多个 NodePool**：如 `gpu-pool`、`spot-pool`、`arm-pool`，避免相互干扰
- **设置合理的资源上限（Limits）**：防止 Karpenter 在异常情况下无限扩容导致巨额账单
- **AMI 镜像预加载**：在 EC2NodeClass 中使用自定义 AMI，预置常用容器镜像和 OS 安全补丁
- **监控 Consolidation 行为**：过度整合可能导致频繁的 Pod 迁移，影响有状态应用稳定性
- **IAM 权限最小化**：Karpenter Controller 只需要 EC2、SSM、Pricing、IAM PassRole 等有限权限
- **Spot 容忍度设置**：为能容忍中断的工作负载配置 Spot NodePool，关键服务保留 On-Demand
- **避免与 Cluster Autoscaler 共存**：同一集群不要同时运行 Karpenter 和 Cluster Autoscaler，避免冲突
- **定期审查实例选型**：Karpenter 的自动选择通常是优化的，但仍需定期通过 Cost Explorer 审查账单构成
- **使用 Reserved Instances / Savings Plans 标签**：为长期稳定的工作负载配置特定标签，让其优先调度到已购买预留实例的实例族

## 生产 YAML 示例

### 多 NodePool 生产配置

```yaml
# 1. 通用工作负载 — Spot 优先
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: general-spot
spec:
  template:
    metadata:
      labels:
        workload-type: general
    spec:
      requirements:
        - key: karpenter.k8s.aws/instance-category
          operator: In
          values: ["c", "m", "r"]
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["spot"]
        - key: kubernetes.io/arch
          operator: In
          values: ["amd64", "arm64"]
        - key: karpenter.k8s.aws/instance-generation
          operator: Gt
          values: ["5"]                    # 只使用第 6 代及以上实例
      nodeClassRef:
        name: default
      taints:
        - key: workload-type
          value: general
          effect: NoSchedule
  limits:
    cpu: 500
    memory: 500Gi
  disruption:
    consolidationPolicy: WhenUnderutilized
    expireAfter: 336h                      # 14 天后强制轮换
  weight: 10                               # 低权重 — 优先使用 Spot
---
# 2. 通用工作负载 — On-Demand 兜底
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: general-ondemand
spec:
  template:
    metadata:
      labels:
        workload-type: general
    spec:
      requirements:
        - key: karpenter.k8s.aws/instance-category
          operator: In
          values: ["c", "m", "r"]
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["on-demand"]
        - key: kubernetes.io/arch
          operator: In
          values: ["amd64"]
      nodeClassRef:
        name: default
      taints:
        - key: workload-type
          value: general
          effect: NoSchedule
  limits:
    cpu: 200
    memory: 200Gi
  disruption:
    consolidationPolicy: WhenUnderutilized
    expireAfter: 720h
  weight: 50                               # 高权重 — Spot 不足时使用
---
# 3. GPU 工作负载 — 独立 NodePool
apiVersion: karpenter.sh/v1beta1
kind: NodePool
metadata:
  name: gpu-pool
spec:
  template:
    metadata:
      labels:
        workload-type: gpu
    spec:
      requirements:
        - key: karpenter.k8s.aws/instance-category
          operator: In
          values: ["p", "g"]
        - key: karpenter.sh/capacity-type
          operator: In
          values: ["on-demand"]            # GPU 用 On-Demand 保证稳定性
        - key: karpenter.k8s.aws/instance-gpu-count
          operator: Gt
          values: ["0"]
      nodeClassRef:
        name: gpu-nodeclass
      taints:
        - key: nvidia.com/gpu
          value: "present"
          effect: NoSchedule
  limits:
    cpu: 1000
    memory: 4000Gi
  disruption:
    consolidationPolicy: WhenEmpty         # GPU 节点仅空闲时整合
    expireAfter: 720h
```

### EC2NodeClass 配置

```yaml
apiVersion: karpenter.k8s.aws/v1beta1
kind: EC2NodeClass
metadata:
  name: default
spec:
  amiFamily: AL2023
  subnetSelectorTerms:
    - tags:
        karpenter.sh/discovery: "my-cluster"
  securityGroupSelectorTerms:
    - tags:
        karpenter.sh/discovery: "my-cluster"
  instanceProfile: "KarpenterNodeInstanceProfile-my-cluster"
  blockDeviceMappings:
    - deviceName: /dev/xvda
      ebs:
        volumeSize: 100Gi
        volumeType: gp3
        iops: 3000
        throughput: 125
        encrypted: true
  tags:
    Environment: production
    ManagedBy: karpenter
  metadataOptions:
    httpEndpoint: enabled
    httpTokens: required                   # 强制使用 IMDSv2
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Pending Pod 但 Karpenter 不启动新节点 | NodePool limits 已达上限 | `kubectl get nodepools -o yaml` 检查 limits 与当前使用量 |
| 新节点启动但 Pod 仍未调度 | 节点未 Ready 或污点不匹配 | `kubectl get nodes` 检查新节点状态；检查 Pod tolerations |
| Spot 实例频繁被中断 | Spot 容量不足，实例类型太少 | 扩展 requirements 中的实例类型范围；增加 instance-category 和 arch |
| 整合过于激进导致 Pod 迁移频繁 | WhenUnderutilized 阈值过敏感 | 改用 WhenEmpty；为有状态 Pod 添加 `karpenter.sh/do-not-disrupt: "true"` |
| Karpenter 启动了非预期的实例类型 | requirements 过于宽泛 | 收紧 instance-category / instance-generation / arch 约束 |
| Drift 替换节点导致服务中断 | AMI 更新触发大规模轮换 | 确认 PDB 配置；使用 `disruption.budgets` 限制并发替换数量 |

## 生产检查清单

- [ ] 为不同工作负载创建独立 NodePool（general / gpu / arm / spot）
- [ ] 设置合理的 `limits.cpu` 和 `limits.memory` 防止无限扩容
- [ ] 配置 `expireAfter` 实现节点定期轮换（AMI 更新、安全补丁）
- [ ] GPU 节点使用 `consolidationPolicy: WhenEmpty` 避免频繁迁移
- [ ] EC2NodeClass 强制使用 IMDSv2（`httpTokens: required`）
- [ ] 最小化 Karpenter Controller IAM 权限
- [ ] 为关键有状态 Pod 添加 `karpenter.sh/do-not-disrupt: "true"` annotation
- [ ] 配置 Spot 中断处理：`karpenter.sh/capacity-type` 同时包含 spot + on-demand
- [ ] 监控 Karpenter 指标：`karpenter_nodes_created`、`karpenter_nodes_terminated`、`karpenter_pods_startup_duration_seconds`
- [ ] 避免与 Cluster Autoscaler 同时运行
- [ ] 定期审查 AWS Cost Explorer 验证成本优化效果

## 命令快速参考

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 NodePool 状态
kubectl get nodepools

# 查看 NodePool 详情（含当前资源使用量）
kubectl describe nodepool general-spot

# 查看 NodeClaim（Karpenter 创建的节点请求）
kubectl get nodeclaims

# 查看 Karpenter 控制器日志
kubectl logs -n kube-system -l app.kubernetes.io/name=karpenter -c controller --tail=100

# 查看由 Karpenter 管理的节点
kubectl get nodes -l karpenter.sh/nodepool

# 查看节点的 Karpenter 标签和注解
kubectl get node <node-name> -o yaml | grep -A 20 "karpenter"

# 查看 EC2NodeClass
kubectl get ec2nodeclasses

# 手动触发节点整合（删除 NodeClaim）
kubectl delete nodeclaim <nodeclaim-name>

# 查看 Karpenter 指标
kubectl port-forward -n kube-system svc/karpenter 8080:8080
curl localhost:8080/metrics | grep karpenter_nodes
```
## 交叉引用

- [资源装箱](./resource-bin-packing.md) — 调度器侧的装箱策略与 Karpenter 整合互补
- [Pod 拓扑分布约束](./pod-topology-spread-constraints.md) — Karpenter 感知拓扑约束进行节点选型
- [将 Pod 分配给节点](./assigning-pods-to-nodes.md) — Karpenter 支持 nodeSelector / affinity
- [污点与容忍度](./taints-and-tolerations.md) — NodePool 中的 taints 配置
- [API 发起驱逐](./api-initiated-eviction.md) — Karpenter 整合时使用 API 驱逐并尊重 PDB
- [动态资源分配](./dynamic-resource-allocation.md) — GPU NodePool 与 DRA 的配合

## 参考链接

- [Karpenter Official Documentation](https://karpenter.sh/docs/)
- [Karpenter GitHub Repository](https://github.com/aws/karpenter-provider-aws)
- [AWS Blog - Karpenter Best Practices](https://aws.amazon.com/blogs/containers/getting-started-with-karpenter/)
- [Cluster Autoscaler Documentation](https://github.com/kubernetes/autoscaler/tree/master/cluster-autoscaler)

## Related

- [[生态参考/topic-index/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]


<!-- risk-assessed -->
